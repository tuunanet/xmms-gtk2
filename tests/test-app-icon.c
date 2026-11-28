/* End-to-end regression test for the application icon: spawn the real
 * binary on a private Xvfb with an isolated HOME and assert what the X
 * server sees on the main window — the multi-size _NET_WM_ICON list panels
 * scale from, and the legacy 16x16 WM_HINTS pixmap kept for window managers
 * that predate it. */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
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

/* The renditions mainwin_set_icon() publishes, smallest first. */
static const guint icon_sizes[] = { 16, 32, 48, 64, 128 };

typedef struct
{
	guint width, height;
	gulong *pixels; /* row-major ARGB */
}
Icon;

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

static void free_icon(gpointer data)
{
	Icon *icon = data;

	g_free(icon->pixels);
	g_free(icon);
}

/* Reads _NET_WM_ICON: each rendition is its width, its height, then
 * width * height ARGB pixels, in publication order. Returns NULL when the
 * property is absent. */
static GList *read_net_wm_icon(Display *display, Window window)
{
	Atom type;
	int format;
	gulong items, remaining, offset;
	guchar *data = NULL;
	GList *icons = NULL;

	if (XGetWindowProperty(display, window,
			       XInternAtom(display, "_NET_WM_ICON", False),
			       0, 1 << 20, False, XA_CARDINAL, &type, &format,
			       &items, &remaining, &data) != Success || !data)
		return NULL;
	g_assert_cmpint(format, ==, 32);
	g_assert_cmpuint(remaining, ==, 0);
	for (offset = 0; offset + 1 < items;)
	{
		const gulong *values = (const gulong *) data;
		Icon *icon = g_new(Icon, 1);

		icon->width = values[offset];
		icon->height = values[offset + 1];
		offset += 2;
		g_assert_true(offset + icon->width * icon->height <= items);
		icon->pixels = g_new(gulong, icon->width * icon->height);
		memcpy(icon->pixels, values + offset,
		       icon->width * icon->height * sizeof(gulong));
		offset += icon->width * icon->height;
		icons = g_list_append(icons, icon);
	}
	XFree(data);
	return icons;
}

/* Returns the geometry of a pixmap, or FALSE when it cannot be read. */
static gboolean pixmap_geometry(Display *display, Pixmap pixmap,
				guint *width, guint *height, guint *depth)
{
	Window root;
	gint x, y;
	guint border, w, h, d;

	if (!XGetGeometry(display, pixmap, &root, &x, &y, &w, &h, &border,
			  &d))
		return FALSE;
	*width = w;
	*height = h;
	*depth = d;
	return TRUE;
}

static void test_main_window_publishes_multi_size_icon(void)
{
	gchar *runtime_directory;
	gchar **environment;
	gchar *arguments[] = { (gchar *) XMMS_BINARY, NULL };
	gboolean spawned;
	GPid child = 0;
	Display *display;
	Window main_window;
	XWMHints *hints;
	GList *icons, *node;
	guint index, width, height, depth;
	gboolean readable;

	runtime_directory = g_dir_make_tmp("xmms-app-icon-XXXXXX", NULL);
	g_assert_nonnull(runtime_directory);

	display = XOpenDisplay(NULL);
	g_assert_nonnull(display);
	XSetErrorHandler(record_x_error);

	/* HOME and TMPDIR isolate the config and the remote-control socket
	 * from any XMMS session running for the user running the test. */
	environment = g_get_environ();
	environment = g_environ_setenv(environment, "HOME", runtime_directory,
				       TRUE);
	environment = g_environ_setenv(environment, "TMPDIR", runtime_directory,
				       TRUE);
	spawned = g_spawn_async(NULL, arguments, environment,
				G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &child,
				NULL);
	g_strfreev(environment);
	g_assert_true(spawned);

	main_window = wait_for_window(display, "XMMS");
	g_assert_true(main_window != None);
	XSync(display, False);

	/* Modern panels and taskbars scale from _NET_WM_ICON. */
	icons = read_net_wm_icon(display, main_window);
	g_assert_nonnull(icons);
	g_assert_cmpuint(g_list_length(icons), ==, G_N_ELEMENTS(icon_sizes));
	for (index = 0, node = icons; node; index++, node = node->next)
	{
		Icon *icon = node->data;
		GHashTable *colors = g_hash_table_new(NULL, NULL);
		guint pixel, distinct;

		g_assert_cmpuint(icon->width, ==, icon_sizes[index]);
		g_assert_cmpuint(icon->height, ==, icon_sizes[index]);
		for (pixel = 0; pixel < icon->width * icon->height; pixel++)
			g_hash_table_insert(colors,
					    GUINT_TO_POINTER(icon->pixels[pixel] & 0xFFFFFF),
					    GINT_TO_POINTER(1));
		/* A rendition that failed to load would be blank. */
		distinct = g_hash_table_size(colors);
		g_assert_cmpuint(distinct, >, 16);
		g_hash_table_destroy(colors);
	}
	g_list_free_full(icons, free_icon);

	/* The legacy WM hints pixmap stays for window managers predating
	 * _NET_WM_ICON. */
	hints = XGetWMHints(display, main_window);
	g_assert_nonnull(hints);
	g_assert_true((hints->flags & IconPixmapHint) != 0);
	readable = pixmap_geometry(display, hints->icon_pixmap, &width,
				   &height, &depth);
	g_assert_true(readable);
	g_assert_cmpuint(width, ==, 16);
	g_assert_cmpuint(height, ==, 16);
	g_assert_true((hints->flags & IconMaskHint) != 0);
	readable = pixmap_geometry(display, hints->icon_mask, &width, &height,
				   &depth);
	g_assert_true(readable);
	g_assert_cmpuint(width, ==, 16);
	g_assert_cmpuint(height, ==, 16);
	g_assert_cmpuint(depth, ==, 1);
	XFree(hints);

	g_assert_cmpint(unexpected_x_error, ==, 0);

	terminate_child(child);
	XCloseDisplay(display);
	remove_tree(runtime_directory);
	g_free(runtime_directory);
}

int main(int argc, char **argv)
{
	GPid xvfb_pid = 0;
	gchar *xvfb_display = NULL;
	gint result;

	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/app-icon/main-window-publishes-multi-size-icon",
			test_main_window_publishes_multi_size_icon);

	/* Self-host the X server: the test spawns a real XMMS instance, so
	 * owning the server keeps the run deterministic regardless of how the
	 * caller provides a display. */
	if (!start_xvfb("1280x1024x24", &xvfb_pid, &xvfb_display))
	{
		g_print("Xvfb is not available; skipping the e2e suite\n");
		return 0;
	}
	g_setenv("DISPLAY", xvfb_display, TRUE);

	result = g_test_run();

	terminate_child(xvfb_pid);
	g_free(xvfb_display);
	return result;
}
