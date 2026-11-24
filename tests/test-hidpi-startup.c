/* End-to-end regression test for HiDPI startup scaling: spawn the real
 * binary on the ambient X display with Xft.dpi injected through the
 * RESOURCE_MANAGER property (what desktops write at scaled
 * configurations) and assert the mapped main window geometry. */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

#ifndef XMMS_BINARY
#define XMMS_BINARY "../xmms/xmms"
#endif

#define WINDOW_WAIT_ATTEMPTS 100
#define WINDOW_WAIT_DELAY_US 20000
#define SIZE_SETTLE_ATTEMPTS 25

#define SINGLE_SIZE_WIDTH 275
#define SINGLE_SIZE_HEIGHT 116
#define DOUBLE_SIZE_WIDTH 550
#define DOUBLE_SIZE_HEIGHT 232

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

/* Reads the mapped size repeatedly and returns only a stable reading so
 * the test never races GTK's initial geometry application. */
static void wait_for_stable_size(Display *display, Window window,
				 guint *width, guint *height)
{
	XWindowAttributes attributes;
	guint previous_width = 0, previous_height = 0;
	gint stable_reads = 0;
	gint attempt;

	for (attempt = 0; attempt < SIZE_SETTLE_ATTEMPTS; attempt++)
	{
		if (!XGetWindowAttributes(display, window, &attributes))
			continue;
		if ((guint) attributes.width == previous_width &&
		    (guint) attributes.height == previous_height &&
		    attributes.width > 0)
		{
			if (++stable_reads >= 2)
			{
				*width = attributes.width;
				*height = attributes.height;
				return;
			}
		}
		else
		{
			stable_reads = 0;
			previous_width = attributes.width;
			previous_height = attributes.height;
		}
		g_usleep(WINDOW_WAIT_DELAY_US);
	}

	*width = previous_width;
	*height = previous_height;
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

static void write_config(const gchar *home, const gchar *extra)
{
	gchar *directory = g_build_filename(home, ".xmms", NULL);
	gchar *filename = g_build_filename(directory, "config", NULL);
	GError *error = NULL;
	gchar *content = g_strdup_printf("[xmms]\n"
					 "playlist_visible=FALSE\n"
					 "equalizer_visible=FALSE\n"
					 "%s", extra ? extra : "");
	gboolean written;

	g_mkdir_with_parents(directory, 0700);
	written = g_file_set_contents(filename, content, -1, &error);
	g_assert_true(written);
	if (error)
		g_error_free(error);

	g_free(content);
	g_free(filename);
	g_free(directory);
}

/* Launches XMMS with an isolated HOME and the given Xft.dpi injected
 * (NULL removes the property), measures the stable main window size,
 * and always restores the previous RESOURCE_MANAGER value. */
static void launch_and_measure(const gchar *config_extra, const gchar *dpi,
			       guint *width, guint *height)
{
	gchar *runtime_directory;
	gchar *resource_backup;
	gchar *resource_dpi;
	gchar **environment;
	gchar *arguments[] = { (gchar *) XMMS_BINARY, NULL };
	GError *error = NULL;
	GPid child = 0;
	Display *display;
	Window main_window;
	gint status = 0;
	guint attempt;

	runtime_directory = g_dir_make_tmp("xmms-hidpi-startup-XXXXXX", &error);
	g_assert_no_error(error);
	g_assert_nonnull(runtime_directory);
	write_config(runtime_directory, config_extra);

	display = XOpenDisplay(NULL);
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
	g_assert_true(g_spawn_async(NULL, arguments, environment,
				    G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL,
				    &child, &error));
	g_assert_no_error(error);
	g_strfreev(environment);

	main_window = wait_for_window(display, "XMMS");
	if (main_window != None)
		wait_for_stable_size(display, main_window, width, height);

	write_resource_manager(display, resource_backup);

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
	g_free(resource_backup);
	g_free(resource_dpi);
}

static void test_dpi_192_starts_doublesize(void)
{
	guint width = 0, height = 0;

	launch_and_measure(NULL, "192", &width, &height);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	/* GNOME at 200% scaling leaves XWayland unscaled, so XMMS must
	 * adopt the desktop scale itself instead of opening at 275x116. */
	g_assert_cmpuint(width, ==, DOUBLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, DOUBLE_SIZE_HEIGHT);
}

static void test_dpi_96_starts_single_size(void)
{
	guint width = 0, height = 0;

	launch_and_measure(NULL, "96", &width, &height);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	g_assert_cmpuint(width, ==, SINGLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, SINGLE_SIZE_HEIGHT);
}

static void test_dpi_unset_starts_single_size(void)
{
	guint width = 0, height = 0;

	launch_and_measure(NULL, NULL, &width, &height);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	g_assert_cmpuint(width, ==, SINGLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, SINGLE_SIZE_HEIGHT);
}

static void test_autoscale_opt_out_starts_single_size(void)
{
	guint width = 0, height = 0;

	/* Persistent opt-out for compositors that already upscale
	 * XWayland clients themselves (KDE/KWin). */
	launch_and_measure("skin_autoscale=FALSE\n", "192", &width, &height);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	g_assert_cmpuint(width, ==, SINGLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, SINGLE_SIZE_HEIGHT);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/hidpi-startup/dpi-192-starts-doublesize",
			test_dpi_192_starts_doublesize);
	g_test_add_func("/hidpi-startup/dpi-96-starts-single-size",
			test_dpi_96_starts_single_size);
	g_test_add_func("/hidpi-startup/dpi-unset-starts-single-size",
			test_dpi_unset_starts_single_size);
	g_test_add_func("/hidpi-startup/opt-out-starts-single-size",
			test_autoscale_opt_out_starts_single_size);

	return g_test_run();
}
