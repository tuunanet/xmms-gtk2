#include <X11/Xlib.h>
#include <X11/keysym.h>
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

static void send_key(Display *display, Window window, KeySym keysym,
		     guint modifiers)
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
	event.keycode = XKeysymToKeycode(display, keysym);
	event.state = modifiers;

	XSetInputFocus(display, window, RevertToParent, CurrentTime);
	event.type = KeyPress;
	XSendEvent(display, window, True, KeyPressMask, (XEvent *) &event);
	event.type = KeyRelease;
	XSendEvent(display, window, True, KeyReleaseMask, (XEvent *) &event);
	XSync(display, False);
}

static gboolean wait_for_child(GPid child, gint *status)
{
	guint attempt;

	for (attempt = 0; attempt < WINDOW_WAIT_ATTEMPTS; attempt++)
	{
		if (waitpid(child, status, WNOHANG) == child)
			return TRUE;
		g_usleep(WINDOW_WAIT_DELAY_US);
	}

	return FALSE;
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

static void test_main_window_activates_menu_shortcuts(void)
{
	gchar *runtime_directory;
	gchar **environment;
	gchar *arguments[] = { (gchar *) XMMS_BINARY, NULL };
	GError *error = NULL;
	GPid child = 0;
	Display *display;
	Window main_window;
	Window preferences_window = None;
	gboolean exited = FALSE;
	gint status = 0;

	runtime_directory = g_dir_make_tmp("xmms-keyboard-shortcuts-XXXXXX",
					   &error);
	g_assert_no_error(error);
	g_assert_nonnull(runtime_directory);

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

	display = XOpenDisplay(NULL);
	g_assert_nonnull(display);
	XSetErrorHandler(record_x_error);
	main_window = wait_for_window(display, "XMMS");
	if (main_window != None)
	{
		send_key(display, main_window, XK_p, ControlMask);
		preferences_window = wait_for_window(display, "Preferences");
		send_key(display, main_window, XK_q, ControlMask);
		exited = wait_for_child(child, &status);
	}

	if (!exited)
	{
		kill(child, SIGTERM);
		waitpid(child, &status, 0);
	}
	g_spawn_close_pid(child);
	XCloseDisplay(display);
	remove_tree(runtime_directory);
	g_free(runtime_directory);

	g_assert_cmpint(unexpected_x_error, ==, 0);
	g_assert_cmpuint(main_window, !=, None);
	g_assert_cmpuint(preferences_window, !=, None);
	g_assert_true(exited);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/keyboard-shortcuts/main-window-activates-menu-shortcuts",
			test_main_window_activates_menu_shortcuts);

	return g_test_run();
}
