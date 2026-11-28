/* End-to-end regression test for mouse-wheel scrolling in the
 * playlist. Under GTK2 the X wheel buttons (4/5) surface as GDK scroll
 * events, never as button presses, so the playlist must react to
 * scroll-event. Spawns the real binary on a private Xvfb screen with
 * a preloaded playlist, injects Button5/Button4 wheel events through
 * XTEST, and asserts the list content scrolls down and then restores
 * exactly when scrolling back up.
 *
 * Frame comparisons cover the list area only (below the title strip):
 * the title strip repaints continuously even without input, so it
 * carries no wheel signal. Pixels are compared through XGetPixel so
 * undefined padding bits in the 24-on-32 frame data never matter. */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef XMMS_BINARY
#define XMMS_BINARY "../xmms/xmms"
#endif

#define WINDOW_WAIT_ATTEMPTS 100
#define WINDOW_WAIT_DELAY_US 20000
#define WHEEL_SETTLE_ATTEMPTS 30
#define PLAYLIST_ENTRIES 40
/* The playlist list starts at y=20 (playlistwin_create), below the
 * animated title strip. */
#define PLAYLIST_LIST_Y 20

/* 1x geometry: no doublesize, autoscale off so the screen size never
 * influences the startup scale. */
#define WHEEL_CONFIG \
	"[xmms]\n" \
	"player_x=20\n" \
	"player_y=20\n" \
	"playlist_x=20\n" \
	"playlist_y=252\n" \
	"playlist_width=300\n" \
	"playlist_height=232\n" \
	"playlist_visible=TRUE\n" \
	"equalizer_visible=FALSE\n" \
	"skin_autoscale=FALSE\n"

static gint unexpected_x_error;

static int record_x_error(Display *display, XErrorEvent *event)
{
	(void) display;
	if (event->error_code != BadWindow)
		unexpected_x_error = event->error_code;

	return 0;
}

static Window find_mapped_window(Display *display, Window window,
				 const gchar *title)
{
	Window root;
	Window parent;
	Window *children = NULL;
	Window match = None;
	XWindowAttributes attributes;
	guint child_count = 0;
	guint i;
	gchar *name = NULL;

	if (XFetchName(display, window, &name) && name)
	{
		if (!strcmp(name, title) &&
		    XGetWindowAttributes(display, window, &attributes) &&
		    attributes.map_state == IsViewable)
			match = window;
		XFree(name);
	}
	if (match != None)
		return match;

	if (!XQueryTree(display, window, &root, &parent, &children,
			&child_count))
		return None;
	for (i = 0; i < child_count && match == None; i++)
		match = find_mapped_window(display, children[i], title);
	if (children)
		XFree(children);

	return match;
}

static Window wait_for_window(Display *display, const gchar *title)
{
	Window window = None;
	guint attempt;

	for (attempt = 0; attempt < WINDOW_WAIT_ATTEMPTS; attempt++)
	{
		window = find_mapped_window(display,
					    DefaultRootWindow(display),
					    title);
		if (window != None)
			break;
		g_usleep(WINDOW_WAIT_DELAY_US);
	}

	return window;
}

/* Stops a spawned child (XMMS or Xvfb) reliably on tear-down. */
static void terminate_child(GPid child)
{
	gint status = 0;
	guint attempt;

	kill(child, SIGTERM);
	for (attempt = 0; attempt < WINDOW_WAIT_ATTEMPTS; attempt++)
	{
		if (waitpid(child, &status, WNOHANG) == child)
			break;
		g_usleep(WINDOW_WAIT_DELAY_US);
	}
	if (waitpid(child, &status, WNOHANG) != child)
	{
		kill(child, SIGKILL);
		waitpid(child, &status, 0);
	}
	g_spawn_close_pid(child);
}

/* Duplicates the pipe write end onto fd 3 for Xvfb's -displayfd. */
static void x11_displayfd_child_setup(gpointer data)
{
	dup2(GPOINTER_TO_INT(data), 3);
}

static gboolean start_xvfb(const gchar *geometry, GPid *xvfb_pid,
			   gchar **display_name)
{
	gint fds[2];
	gchar *arguments[] = { (gchar *) "Xvfb", (gchar *) "-displayfd",
			       (gchar *) "3", (gchar *) "-screen",
			       (gchar *) "0", (gchar *) geometry,
			       (gchar *) "-nolisten", (gchar *) "tcp", NULL };
	gchar buffer[16];
	gssize length = 0;
	GError *error = NULL;

	if (pipe(fds) != 0)
		return FALSE;
	if (!g_spawn_async(NULL, arguments, NULL,
			   G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
			   x11_displayfd_child_setup,
			   GINT_TO_POINTER(fds[1]), xvfb_pid, &error))
	{
		g_error_free(error);
		close(fds[0]);
		close(fds[1]);
		return FALSE;
	}
	close(fds[1]);

	/* Xvfb writes the display number once it accepts connections. */
	while (length < (gssize) sizeof(buffer) - 1)
	{
		gssize got = read(fds[0], buffer + length, 1);

		if (got <= 0 || buffer[length] == '\n')
			break;
		length++;
	}
	close(fds[0]);

	if (length <= 0 || !g_ascii_isdigit(buffer[0]))
	{
		terminate_child(*xvfb_pid);
		return FALSE;
	}
	buffer[length] = '\0';
	*display_name = g_strdup_printf(":%s", buffer);
	return TRUE;
}

static void remove_tree(const gchar *path)
{
	GDir *directory;
	const gchar *name;

	directory = g_dir_open(path, 0, NULL);
	if (!directory)
	{
		g_remove(path);
		return;
	}
	while ((name = g_dir_read_name(directory)) != NULL)
	{
		gchar *child = g_build_filename(path, name, NULL);

		if (g_file_test(child, G_FILE_TEST_IS_DIR) &&
		    !g_file_test(child, G_FILE_TEST_IS_SYMLINK))
			remove_tree(child);
		else
			g_remove(child);
		g_free(child);
	}
	g_dir_close(directory);
	g_rmdir(path);
}

static void write_config(const gchar *home, const gchar *content)
{
	gchar *directory = g_build_filename(home, ".xmms", NULL);
	gchar *filename = g_build_filename(directory, "config", NULL);
	GError *error = NULL;
	gboolean written;

	g_mkdir_with_parents(directory, 0700);
	written = g_file_set_contents(filename, content, -1, &error);
	g_assert_true(written);
	if (error)
		g_error_free(error);

	g_free(filename);
	g_free(directory);
}

/* A playlist long enough that the list has somewhere to scroll to;
 * entries do not need to exist as files to render as rows. */
static void write_playlist(const gchar *home, gint count)
{
	gchar *directory = g_build_filename(home, ".xmms", NULL);
	gchar *filename = g_build_filename(directory, "xmms.m3u", NULL);
	GString *content;
	GError *error = NULL;
	gboolean written;
	gint i;

	g_mkdir_with_parents(directory, 0700);
	content = g_string_new("");
	for (i = 0; i < count; i++)
		g_string_append_printf(content,
				       "/tmp/xmms-wheel-entry-%02d.mp3\n", i);
	written = g_file_set_contents(filename, content->str, -1, &error);
	g_assert_true(written);
	if (error)
		g_error_free(error);

	g_string_free(content, TRUE);
	g_free(filename);
	g_free(directory);
}

/* Launches XMMS with an isolated HOME on the given display and
 * returns the playlist window once it is mapped. */
static void launch_on(const gchar *display_name, const gchar *config,
		      Window *playlist_out, GPid *child_out,
		      Display **display_out, gchar **runtime_dir_out)
{
	gchar *runtime_directory;
	gchar **environment;
	gchar *arguments[] = { (gchar *) XMMS_BINARY, NULL };
	GError *error = NULL;
	GPid child = 0;
	Display *display;
	Window playlist_window;

	runtime_directory = g_dir_make_tmp("xmms-playlist-wheel-XXXXXX",
					   &error);
	g_assert_no_error(error);
	g_assert_nonnull(runtime_directory);
	write_config(runtime_directory, config);
	write_playlist(runtime_directory, PLAYLIST_ENTRIES);

	display = XOpenDisplay(display_name);
	g_assert_nonnull(display);
	XSetErrorHandler(record_x_error);

	environment = g_get_environ();
	environment = g_environ_setenv(environment, "HOME", runtime_directory,
				       TRUE);
	environment = g_environ_setenv(environment, "TMPDIR", runtime_directory,
				       TRUE);
	if (display_name)
		environment = g_environ_setenv(environment, "DISPLAY",
					       display_name, TRUE);
	g_assert_true(g_spawn_async(NULL, arguments, environment,
				    G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL,
				    &child, &error));
	g_assert_no_error(error);
	g_strfreev(environment);

	playlist_window = wait_for_window(display, "XMMS Playlist");
	g_assert_cmpuint(playlist_window, !=, None);
	g_usleep(500000); /* let the first full draw settle */

	*playlist_out = playlist_window;
	*child_out = child;
	*display_out = display;
	*runtime_dir_out = runtime_directory;
}

static XImage *grab_window(Display *display, Window window)
{
	XWindowAttributes attributes;
	gboolean ok;
	XImage *image;

	ok = XGetWindowAttributes(display, window, &attributes);
	g_assert_true(ok);
	image = XGetImage(display, window, 0, 0, attributes.width,
			  attributes.height, AllPlanes, ZPixmap);
	g_assert_nonnull(image);
	return image;
}

/* Number of pixels whose defined value differs at or below y_start. */
static glong count_region_pixel_diffs(XImage *a, XImage *b, gint y_start)
{
	glong differing = 0;
	gint x, y;

	for (y = y_start; y < a->height; y++)
		for (x = 0; x < a->width; x++)
			if (XGetPixel(a, x, y) != XGetPixel(b, x, y))
				differing++;

	return differing;
}

/* Grabs repeatedly until the list area is unchanged across two
 * consecutive reads, so GTK's incremental redraws never race the
 * comparison. */
static void wait_for_stable_frame(Display *display, Window window,
				  XImage **image_out)
{
	XImage *previous, *current;
	gint stable_reads = 0;
	gint attempt;

	previous = grab_window(display, window);
	for (attempt = 0; attempt < WHEEL_SETTLE_ATTEMPTS; attempt++)
	{
		g_usleep(WINDOW_WAIT_DELAY_US);
		current = grab_window(display, window);
		if (count_region_pixel_diffs(previous, current,
					     PLAYLIST_LIST_Y) == 0)
		{
			stable_reads++;
			XDestroyImage(previous);
			previous = current;
			if (stable_reads >= 2)
				break;
		}
		else
		{
			stable_reads = 0;
			XDestroyImage(previous);
			previous = current;
		}
	}
	*image_out = previous;
}

static void send_wheel_notch(Display *display, Window window, guint button)
{
	XWindowAttributes attributes;
	gboolean ok;

	ok = XGetWindowAttributes(display, window, &attributes);
	g_assert_true(ok);
	/* Parent is the root here (no window manager), so x/y are
	 * screen coordinates. */
	XTestFakeMotionEvent(display, -1, attributes.x + attributes.width / 2,
			     attributes.y + attributes.height / 2,
			     CurrentTime);
	XTestFakeButtonEvent(display, button, True, CurrentTime);
	XTestFakeButtonEvent(display, button, False, CurrentTime);
	XFlush(display);
}

static void region_assert(XImage *a, XImage *b, glong expected_diffs)
{
	glong differing = count_region_pixel_diffs(a, b, PLAYLIST_LIST_Y);

	g_test_message("list-area differing pixels: %ld (expected %s 0)",
		       differing, expected_diffs == 0 ? "" : ">");
	if (expected_diffs == 0)
		g_assert_cmpint(differing, ==, 0);
	else
		g_assert_cmpint(differing, >, 0);
}

static void test_playlist_wheel_scrolls_list(void)
{
	GPid xvfb_pid = 0, child;
	gchar *display_name = NULL, *runtime_directory = NULL;
	Display *display;
	Window playlist_window;
	XImage *baseline, *after_down, *after_up;

	if (!start_xvfb("1280x1024x24", &xvfb_pid, &display_name))
	{
		g_test_skip("Xvfb is not available");
		return;
	}

	launch_on(display_name, WHEEL_CONFIG, &playlist_window, &child,
		  &display, &runtime_directory);
	g_assert_cmpint(unexpected_x_error, ==, 0);

	wait_for_stable_frame(display, playlist_window, &baseline);

	/* Wheel down: the regression is that this did nothing at all,
	 * so the list area must change afterwards. */
	send_wheel_notch(display, playlist_window, Button5);
	wait_for_stable_frame(display, playlist_window, &after_down);
	region_assert(baseline, after_down, 1);

	/* Wheel up: three rows back returns to the exact original list
	 * area (pl_first clamps to 0 at the top). */
	send_wheel_notch(display, playlist_window, Button4);
	wait_for_stable_frame(display, playlist_window, &after_up);
	region_assert(after_down, after_up, 1);
	region_assert(baseline, after_up, 0);

	XDestroyImage(baseline);
	XDestroyImage(after_down);
	XDestroyImage(after_up);

	terminate_child(child);
	XCloseDisplay(display);
	remove_tree(runtime_directory);
	g_free(runtime_directory);
	terminate_child(xvfb_pid);
	g_free(display_name);
	g_assert_cmpint(unexpected_x_error, ==, 0);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/playlist-wheel/scrolls-list-and-restores",
			test_playlist_wheel_scrolls_list);

	return g_test_run();
}
