/* End-to-end regression test for the docked-window layout across HiDPI
 * autoscale and DoubleSize toggles: the three skinned windows in a
 * vertical dock chain must stay flush in every state — at an autoscaled
 * start (windows come up doubled from 1x saved positions) and across
 * Ctrl+D toggles. */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
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

/* Vertical dock chain used by every case: player at (20,20), equalizer
 * flush below it, playlist flush below the equalizer (1x geometry). */
#define VERTICAL_CHAIN_CONFIG \
	"[xmms]\n" \
	"player_x=20\n" \
	"player_y=20\n" \
	"equalizer_x=20\n" \
	"equalizer_y=136\n" \
	"playlist_x=20\n" \
	"playlist_y=252\n" \
	"playlist_width=300\n" \
	"playlist_height=232\n" \
	"playlist_visible=TRUE\n" \
	"equalizer_visible=TRUE\n"

/* Same vertical chain, started at the 3x scale via a skin_scale
 * override on a private screen large enough for the 3x stack. */
#define TRIPLE_CHAIN_CONFIG \
	"[xmms]\n" \
	"player_x=20\n" \
	"player_y=20\n" \
	"equalizer_x=20\n" \
	"equalizer_y=136\n" \
	"playlist_x=20\n" \
	"playlist_y=252\n" \
	"playlist_width=300\n" \
	"playlist_height=232\n" \
	"skin_scale=3.0\n" \
	"playlist_visible=TRUE\n" \
	"equalizer_visible=TRUE\n"

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
					    DefaultRootWindow(display), title);
		if (window != None)
			break;
		g_usleep(WINDOW_WAIT_DELAY_US);
	}

	return window;
}

typedef struct
{
	gint x, y, width, height;
} WindowBox;

/* Waits until position and size of all three windows have stopped
 * changing, then returns the final boxes. */
static void wait_for_settled_layout(Display *display, Window main_window,
				    Window eq_window, Window playlist_window,
				    WindowBox *main_box, WindowBox *eq_box,
				    WindowBox *playlist_box)
{
	Window windows[3] = { main_window, eq_window, playlist_window };
	WindowBox *boxes[3] = { main_box, eq_box, playlist_box };
	XWindowAttributes previous[3];
	XWindowAttributes current;
	gint stable_reads = 0;
	gint attempt, i;

	memset(previous, 0, sizeof(previous));
	memset(&current, 0, sizeof(current));

	for (attempt = 0; attempt < WINDOW_WAIT_ATTEMPTS; attempt++)
	{
		gboolean same = TRUE;

		for (i = 0; i < 3; i++)
		{
			if (windows[i] == None ||
			    !XGetWindowAttributes(display, windows[i],
						  &current))
			{
				same = FALSE;
				continue;
			}
			if (current.x != previous[i].x ||
			    current.y != previous[i].y ||
			    current.width != previous[i].width ||
			    current.height != previous[i].height)
				same = FALSE;
			previous[i] = current;
			boxes[i]->x = current.x;
			boxes[i]->y = current.y;
			boxes[i]->width = current.width;
			boxes[i]->height = current.height;
		}
		stable_reads = same ? stable_reads + 1 : 0;
		if (stable_reads >= 10)
			return;
		g_usleep(WINDOW_WAIT_DELAY_US);
	}
}

static void send_ctrl_d(Display *display, Window window)
{
	XKeyEvent event;

	memset(&event, 0, sizeof(event));
	event.display = display;
	event.window = window;
	event.root = DefaultRootWindow(display);
	event.time = CurrentTime;
	event.x = 1;
	event.y = 1;
	event.x_root = 1;
	event.y_root = 1;
	event.same_screen = True;
	event.keycode = XKeysymToKeycode(display, XK_d);
	event.state = ControlMask;

	XSetInputFocus(display, window, RevertToParent, CurrentTime);
	event.type = KeyPress;
	XSendEvent(display, window, True, KeyPressMask, (XEvent *) &event);
	event.type = KeyRelease;
	XSendEvent(display, window, True, KeyPressMask, (XEvent *) &event);
	XSync(display, False);
}

static gchar *read_resource_manager(Display *display)
{
	Atom type;
	int format;
	gulong items, remaining;
	guchar *data = NULL;
	gchar *value = NULL;

	if (XGetWindowProperty(display, DefaultRootWindow(display),
			       XA_RESOURCE_MANAGER, 0, 8192, False, XA_STRING,
			       &type, &format, &items, &remaining, &data)
	    == Success && type == XA_STRING && data)
	{
		value = g_strdup((gchar *) data);
		XFree(data);
	}
	return value;
}

static void write_resource_manager(Display *display, const gchar *value)
{
	if (value)
		XChangeProperty(display, DefaultRootWindow(display),
				XA_RESOURCE_MANAGER, XA_STRING, 8,
				PropModeReplace, (guchar *) value,
				strlen(value));
	else
		XDeleteProperty(display, DefaultRootWindow(display),
				XA_RESOURCE_MANAGER);
	XSync(display, False);
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

/* Spawns a private Xvfb with the given screen geometry; on success the
 * caller receives the pid and a display name (":N") that the server
 * picked via -displayfd. Returns FALSE when Xvfb is unavailable. */
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

/* Spawns XMMS with an isolated HOME and Xft.dpi injected, waits for the
 * three windows and the layout to settle, measures everything and
 * restores the RESOURCE_MANAGER property. display_name selects the X
 * server (NULL = ambient display). */
static void launch_and_measure_on(const gchar *display_name,
				  const gchar *config_content,
				  const gchar *dpi,
			       WindowBox *main_box, WindowBox *eq_box,
			       WindowBox *playlist_box, GPid *child_out,
			       Display **display_out, Window *main_out,
			       Window *eq_out, Window *playlist_out,
			       gchar **runtime_dir_out)
{
	gchar *runtime_directory;
	gchar *resource_backup;
	gchar *resource_dpi;
	gchar **environment;
	gchar *arguments[] = { (gchar *) XMMS_BINARY, NULL };
	GError *error = NULL;
	GPid child = 0;
	Display *display;
	Window main_window, eq_window, playlist_window;

	runtime_directory = g_dir_make_tmp("xmms-dock-resnap-XXXXXX", &error);
	g_assert_no_error(error);
	g_assert_nonnull(runtime_directory);
	write_config(runtime_directory, config_content);

	display = XOpenDisplay(display_name);
	g_assert_nonnull(display);
	XSetErrorHandler(record_x_error);
	resource_backup = read_resource_manager(display);
	resource_dpi = dpi ? g_strdup_printf("Xft.dpi: %s\n", dpi) : NULL;
	write_resource_manager(display, resource_dpi);

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

	main_window = wait_for_window(display, "XMMS");
	eq_window = wait_for_window(display, "XMMS Equalizer");
	playlist_window = wait_for_window(display, "XMMS Playlist");
	wait_for_settled_layout(display, main_window, eq_window,
				playlist_window, main_box, eq_box,
				playlist_box);

	write_resource_manager(display, resource_backup);
	g_free(resource_backup);
	g_free(resource_dpi);

	*child_out = child;
	*display_out = display;
	*main_out = main_window;
	*eq_out = eq_window;
	*playlist_out = playlist_window;
	*runtime_dir_out = runtime_directory;
}

static void launch_and_measure(const gchar *config_content, const gchar *dpi,
			       WindowBox *main_box, WindowBox *eq_box,
			       WindowBox *playlist_box, GPid *child_out,
			       Display **display_out, Window *main_out,
			       Window *eq_out, Window *playlist_out,
			       gchar **runtime_dir_out)
{
	launch_and_measure_on(NULL, config_content, dpi, main_box, eq_box,
			      playlist_box, child_out, display_out, main_out,
			      eq_out, playlist_out, runtime_dir_out);
}

static void finish_child(GPid child, Display *display,
			 gchar *runtime_directory)
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
	XCloseDisplay(display);
	remove_tree(runtime_directory);
	g_free(runtime_directory);
}

static void assert_flush_doubled_stack(const WindowBox *main_box,
				       const WindowBox *eq_box,
				       const WindowBox *playlist_box)
{
	/* Doubled player at (20,20); equalizer flush below it; playlist
	 * flush below the equalizer, doubled too (it follows the
	 * player's scale). */
	g_assert_cmpint(main_box->x, ==, 20);
	g_assert_cmpint(main_box->y, ==, 20);
	g_assert_cmpint(main_box->width, ==, 550);
	g_assert_cmpint(main_box->height, ==, 232);
	g_assert_cmpint(eq_box->x, ==, 20);
	g_assert_cmpint(eq_box->y, ==, main_box->y + main_box->height);
	g_assert_cmpint(eq_box->width, ==, 550);
	g_assert_cmpint(eq_box->height, ==, 232);
	g_assert_cmpint(playlist_box->x, ==, 20);
	g_assert_cmpint(playlist_box->y,
			==, eq_box->y + eq_box->height);
	g_assert_cmpint(playlist_box->width, ==, 600);
	g_assert_cmpint(playlist_box->height, ==, 464);
}

static void assert_flush_single_stack(const WindowBox *main_box,
				      const WindowBox *eq_box,
				      const WindowBox *playlist_box)
{
	g_assert_cmpint(main_box->x, ==, 20);
	g_assert_cmpint(main_box->y, ==, 20);
	g_assert_cmpint(main_box->width, ==, 275);
	g_assert_cmpint(main_box->height, ==, 116);
	g_assert_cmpint(eq_box->x, ==, 20);
	g_assert_cmpint(eq_box->y, ==, main_box->y + main_box->height);
	g_assert_cmpint(eq_box->width, ==, 275);
	g_assert_cmpint(eq_box->height, ==, 116);
	g_assert_cmpint(playlist_box->x, ==, 20);
	g_assert_cmpint(playlist_box->y,
			==, eq_box->y + eq_box->height);
	g_assert_cmpint(playlist_box->width, ==, 300);
	g_assert_cmpint(playlist_box->height, ==, 232);
}

static void assert_flush_triple_stack(const WindowBox *main_box,
				       const WindowBox *eq_box,
				       const WindowBox *playlist_box)
{
	/* Triple player at (20,20); equalizer flush below it; playlist
	 * flush below the equalizer, still 1x sized. */
	g_assert_cmpint(main_box->x, ==, 20);
	g_assert_cmpint(main_box->y, ==, 20);
	g_assert_cmpint(main_box->width, ==, 825);
	g_assert_cmpint(main_box->height, ==, 348);
	g_assert_cmpint(eq_box->x, ==, 20);
	g_assert_cmpint(eq_box->y, ==, main_box->y + main_box->height);
	g_assert_cmpint(eq_box->width, ==, 825);
	g_assert_cmpint(eq_box->height, ==, 348);
	g_assert_cmpint(playlist_box->x, ==, 20);
	g_assert_cmpint(playlist_box->y,
			==, eq_box->y + eq_box->height);
	g_assert_cmpint(playlist_box->width, ==, 900);
	g_assert_cmpint(playlist_box->height, ==, 696);
}

static void test_autoscaled_start_is_flush(void)
{
	WindowBox main_box, eq_box, playlist_box;
	GPid child;
	Display *display;
	Window main_window, eq_window, playlist_window;
	gchar *runtime_directory;

	launch_and_measure(VERTICAL_CHAIN_CONFIG, "192", &main_box, &eq_box,
			   &playlist_box, &child, &display, &main_window,
			   &eq_window, &playlist_window, &runtime_directory);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	assert_flush_doubled_stack(&main_box, &eq_box, &playlist_box);
	finish_child(child, display, runtime_directory);
}

static void test_double_toggle_stays_flush(void)
{
	WindowBox main_box, eq_box, playlist_box;
	GPid child;
	Display *display;
	Window main_window, eq_window, playlist_window;
	gchar *runtime_directory;

	launch_and_measure(VERTICAL_CHAIN_CONFIG, "192", &main_box, &eq_box,
			   &playlist_box, &child, &display, &main_window,
			   &eq_window, &playlist_window, &runtime_directory);

	/* Collapse to 1x and expand again, ending at the same doubled
	 * state the user reproduces the cropping with. */
	send_ctrl_d(display, main_window);
	wait_for_settled_layout(display, main_window, eq_window,
				playlist_window, &main_box, &eq_box,
				&playlist_box);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	assert_flush_single_stack(&main_box, &eq_box, &playlist_box);

	send_ctrl_d(display, main_window);
	wait_for_settled_layout(display, main_window, eq_window,
				playlist_window, &main_box, &eq_box,
				&playlist_box);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	assert_flush_doubled_stack(&main_box, &eq_box, &playlist_box);

	finish_child(child, display, runtime_directory);
}

static void test_triple_start_and_toggle_stay_flush(void)
{
	WindowBox main_box, eq_box, playlist_box;
	GPid child, xvfb_pid = 0;
	Display *display;
	Window main_window, eq_window, playlist_window;
	gchar *runtime_directory, *display_name = NULL;

	/* The 3x stack (825x348 windows plus a 900x696 playlist) needs
	 * more than the ambient Xvfb screen provides. */
	if (!start_xvfb("2560x1440x24", &xvfb_pid, &display_name))
	{
		g_test_skip("Xvfb is not available");
		return;
	}
	launch_and_measure_on(display_name, TRIPLE_CHAIN_CONFIG, NULL,
			      &main_box, &eq_box, &playlist_box, &child,
			      &display, &main_window, &eq_window,
			      &playlist_window, &runtime_directory);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	assert_flush_triple_stack(&main_box, &eq_box, &playlist_box);

	/* The DoubleSize toggle switches between 1x and the session
	 * target (3x here); both ends of the cycle must stay flush. */
	send_ctrl_d(display, main_window);
	wait_for_settled_layout(display, main_window, eq_window,
				playlist_window, &main_box, &eq_box,
				&playlist_box);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	assert_flush_single_stack(&main_box, &eq_box, &playlist_box);

	send_ctrl_d(display, main_window);
	wait_for_settled_layout(display, main_window, eq_window,
				playlist_window, &main_box, &eq_box,
				&playlist_box);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	assert_flush_triple_stack(&main_box, &eq_box, &playlist_box);

	finish_child(child, display, runtime_directory);
	terminate_child(xvfb_pid);
	g_free(display_name);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/dock-resnap/autoscaled-start-is-flush",
			test_autoscaled_start_is_flush);
	g_test_add_func("/dock-resnap/double-toggle-stays-flush",
			test_double_toggle_stays_flush);
	g_test_add_func("/dock-resnap/triple-start-and-toggle-stay-flush",
			test_triple_start_and_toggle_stay_flush);

	return g_test_run();
}
