/* End-to-end regression test for HiDPI startup scaling: spawn the real
 * binary with Xft.dpi injected through the RESOURCE_MANAGER property
 * (what desktops write at scaled configurations) and assert the mapped
 * main window geometry — on the ambient X display and on private Xvfb
 * screens exercising the display-resolution default. */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
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
#define SIZE_SETTLE_ATTEMPTS 25

#define SINGLE_SIZE_WIDTH 275
#define SINGLE_SIZE_HEIGHT 116
#define DOUBLE_SIZE_WIDTH 550
#define DOUBLE_SIZE_HEIGHT 232
#define TRIPLE_SIZE_WIDTH 825
#define TRIPLE_SIZE_HEIGHT 348
/* Seeded first-run playlist height (the read_config default). */
#define FIRST_RUN_PLAYLIST_HEIGHT 232

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

/* Mirrors the startup decision in xmms/util.c
 * (util_skin_scale_for_display) for the ambient screen so assertions
 * stay correct whatever geometry `make check` runs under. */
static gint expected_startup_scale(const gchar *dpi)
{
	Display *display = XOpenDisplay(NULL);
	gdouble coefficient;

	g_assert_nonnull(display);
	coefficient = MIN(DisplayWidth(display, DefaultScreen(display)) / 2560.0,
			  DisplayHeight(display, DefaultScreen(display)) / 1440.0);
	if (dpi)
		coefficient = MAX(coefficient,
				  g_ascii_strtod(dpi, NULL) / 96.0);
	XCloseDisplay(display);
	return CLAMP((gint) (coefficient + 0.5), 1, 3);
}

/* Launches XMMS with an isolated HOME on the given display (NULL for
 * the ambient one) and the given Xft.dpi injected (NULL removes the
 * property), measures the stable main window size, and always restores
 * the previous RESOURCE_MANAGER value. With create_config set, a
 * minimal config file is seeded first; otherwise the run starts with
 * no config at all (first run). When config_contents is non-NULL it
 * receives the contents of ~/.xmms/config as XMMS left it. When
 * child_out is non-NULL the instance is left running on the opened
 * display (windows, runtime HOME returned) and the caller tears it
 * down with finish_launch(); otherwise the function fully cleans up. */
static void launch_and_measure_on(const gchar *display_name,
				  gboolean create_config,
				  const gchar *config_extra,
				  const gchar *dpi, guint *width,
				  guint *height, gchar **config_contents,
				  GPid *child_out, Display **display_out,
				  Window *main_out, gchar **runtime_dir_out)
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

	runtime_directory = g_dir_make_tmp("xmms-hidpi-startup-XXXXXX", &error);
	g_assert_no_error(error);
	g_assert_nonnull(runtime_directory);
	if (create_config)
		write_config(runtime_directory, config_extra);

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
	if (main_window != None)
		wait_for_stable_size(display, main_window, width, height);

	if (config_contents)
	{
		gchar *path = g_build_filename(runtime_directory, ".xmms",
					       "config", NULL);
		gboolean read_config = g_file_get_contents(path,
							   config_contents,
							   NULL, NULL);

		g_assert_true(read_config);
		g_free(path);
	}

	write_resource_manager(display, resource_backup);
	g_free(resource_backup);
	g_free(resource_dpi);

	if (child_out)
	{
		*child_out = child;
		*display_out = display;
		*main_out = main_window;
		*runtime_dir_out = runtime_directory;
		return;
	}

	terminate_child(child);
	XCloseDisplay(display);
	remove_tree(runtime_directory);
	g_free(runtime_directory);
}

/* Tears down an instance left running by launch_and_measure_on. */
static void finish_launch(GPid child, Display *display,
			  gchar *runtime_directory)
{
	terminate_child(child);
	XCloseDisplay(display);
	remove_tree(runtime_directory);
	g_free(runtime_directory);
}

static void launch_and_measure(const gchar *config_extra, const gchar *dpi,
			       guint *width, guint *height)
{
	launch_and_measure_on(NULL, TRUE, config_extra, dpi, width, height,
			      NULL, NULL, NULL, NULL, NULL);
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

static void test_dpi_96_matches_scale_rule(void)
{
	guint width = 0, height = 0;
	gint expected = expected_startup_scale("96");

	launch_and_measure(NULL, "96", &width, &height);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	/* 96 DPI never forces DoubleSize by itself; on an unusually
	 * large `make check` screen only the resolution rule may. */
	g_assert_cmpuint(width, ==, (guint) expected * SINGLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, (guint) expected * SINGLE_SIZE_HEIGHT);
}

static void test_dpi_unset_matches_scale_rule(void)
{
	guint width = 0, height = 0;
	gint expected = expected_startup_scale(NULL);

	launch_and_measure(NULL, NULL, &width, &height);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	/* No DPI signal: the resolution coefficient decides. */
	g_assert_cmpuint(width, ==, (guint) expected * SINGLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, (guint) expected * SINGLE_SIZE_HEIGHT);
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

static void test_skin_scale_override_pins_single_size(void)
{
	guint width = 0, height = 0;

	launch_and_measure("skin_scale=1.0\n", "192", &width, &height);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	/* An explicit coefficient wins over the DPI autoscale. */
	g_assert_cmpuint(width, ==, SINGLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, SINGLE_SIZE_HEIGHT);
}

static void test_skin_scale_override_forces_doublesize(void)
{
	guint width = 0, height = 0;

	launch_and_measure("skin_scale=1.5\n", NULL, &width, &height);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	/* An explicit coefficient wins even with no DPI signal; 1.5
	 * rounds up to DoubleSize on any screen. */
	g_assert_cmpuint(width, ==, DOUBLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, DOUBLE_SIZE_HEIGHT);
}

static void test_skin_scale_override_triple_size(void)
{
	guint width = 0, height = 0;

	launch_and_measure("skin_scale=3.0\n", NULL, &width, &height);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	/* skin_scale coefficients beyond 2.5 map onto the renderer's 3x
	 * scale (825x348), independent of the screen. */
	g_assert_cmpuint(width, ==, TRIPLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, TRIPLE_SIZE_HEIGHT);
}

/* Runs XMMS on a private Xvfb screen of the given geometry with no
 * Xft.dpi signal so the display resolution alone decides the startup
 * scale. Marks the test skipped when Xvfb is unavailable. */
static void run_on_private_screen(const gchar *geometry, guint *width,
				  guint *height)
{
	GPid xvfb_pid = 0;
	gchar *display_name = NULL;

	if (!start_xvfb(geometry, &xvfb_pid, &display_name))
	{
		g_test_skip("Xvfb is not available");
		return;
	}
	launch_and_measure_on(display_name, TRUE, NULL, NULL, width, height,
			      NULL, NULL, NULL, NULL, NULL);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	terminate_child(xvfb_pid);
	g_free(display_name);
}

static void test_resolution_4k_starts_doublesize(void)
{
	guint width = 0, height = 0;

	/* 3840x2160 is 1.5x the QHD baseline, which rounds up to
	 * DoubleSize even without any DPI signal (for example a 32" 4K
	 * panel on plain X11). */
	run_on_private_screen("3840x2160x24", &width, &height);
	if (width == 0)
		return; /* skipped above */
	g_assert_cmpuint(width, ==, DOUBLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, DOUBLE_SIZE_HEIGHT);
}

static void test_resolution_qhd_starts_single_size(void)
{
	guint width = 0, height = 0;

	/* QHD is the accepted default: the classic 1x UI. */
	run_on_private_screen("2560x1440x24", &width, &height);
	if (width == 0)
		return; /* skipped above */
	g_assert_cmpuint(width, ==, SINGLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, SINGLE_SIZE_HEIGHT);
}

static void test_resolution_8k_starts_triple_size(void)
{
	guint width = 0, height = 0;

	/* 7680x4320 is 3x the QHD baseline: the coefficient rounds up
	 * to the renderer's 3x scale even without any DPI signal. */
	run_on_private_screen("7680x4320x24", &width, &height);
	if (width == 0)
		return; /* skipped above */
	g_assert_cmpuint(width, ==, TRIPLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, TRIPLE_SIZE_HEIGHT);
}

static void test_skin_scale_override_wins_over_persisted_doublesize(void)
{
	guint width = 0, height = 0;

	launch_and_measure("skin_scale=1.0\n"
			   "doublesize=TRUE\n", NULL, &width, &height);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	/* A previous autoscaled session persists doublesize=TRUE in the
	 * config; an explicit skin_scale override must still pin the
	 * classic size. */
	g_assert_cmpuint(width, ==, SINGLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, SINGLE_SIZE_HEIGHT);
}

static void test_autoscale_clears_persisted_doublesize(void)
{
	guint width = 0, height = 0;

	launch_and_measure("doublesize=TRUE\n", "96", &width, &height);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	/* With skin_autoscale enabled (default) the startup decision is
	 * authoritative: a low-DPI screen must not inherit a stale
	 * doublesize=TRUE saved by an earlier autoscaled session. */
	g_assert_cmpuint(width, ==, SINGLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, SINGLE_SIZE_HEIGHT);
}

/* Window geometry in root coordinates (no window manager: the parent
 * is the root window, so x/y are screen coordinates). */
typedef struct
{
	gint x, y;
	guint width, height;
} WindowBox;

/* Grabs a window's geometry repeatedly until stable, so GTK's
 * incremental geometry application never races the assertions. */
static void wait_for_geometry(Display *display, Window window,
			      WindowBox *box)
{
	XWindowAttributes attributes;
	gboolean ok;
	gint stable = 0, attempt;
	gint px = -1, py = -1;
	guint pw = 0, ph = 0;

	for (attempt = 0; attempt < SIZE_SETTLE_ATTEMPTS; attempt++)
	{
		g_usleep(WINDOW_WAIT_DELAY_US);
		ok = XGetWindowAttributes(display, window, &attributes);
		g_assert_true(ok);
		if (attributes.x == px && attributes.y == py &&
		    (guint) attributes.width == pw &&
		    (guint) attributes.height == ph)
		{
			if (++stable >= 2)
				break;
		}
		else
		{
			stable = 0;
			px = attributes.x;
			py = attributes.y;
			pw = attributes.width;
			ph = attributes.height;
		}
	}
	ok = XGetWindowAttributes(display, window, &attributes);
	g_assert_true(ok);
	box->x = attributes.x;
	box->y = attributes.y;
	box->width = attributes.width;
	box->height = attributes.height;
}

/* First run: no config file exists yet. XMMS must write
 * ~/.xmms/config immediately with the seeded skin-scale keys
 * (doublesize=TRUE, skin_autoscale=TRUE, skin_scale=1.5), start
 * DoubleSize on any display, and open the classic first-run layout —
 * equalizer docked to the player's right, playlist spanning their
 * combined width below — centered on the screen. */
static void test_first_run_seeds_doublesize_config(void)
{
	guint width = 0, height = 0;
	gchar *config_contents = NULL;
	const gchar *scale;
	gdouble seeded_scale;
	Window eq_window, playlist_window;
	WindowBox main_box, eq_box, playlist_box;
	gint screen_w, screen_h, group_w, group_h, expect_x, expect_y;
	GPid child;
	Display *display;
	Window main_window;
	gchar *runtime_directory = NULL;

	launch_and_measure_on(NULL, FALSE, NULL, NULL, &width, &height,
			      &config_contents, &child, &display,
			      &main_window, &runtime_directory);
	g_assert_cmpint(unexpected_x_error, ==, 0);
	g_assert_cmpuint(width, ==, DOUBLE_SIZE_WIDTH);
	g_assert_cmpuint(height, ==, DOUBLE_SIZE_HEIGHT);

	g_assert_nonnull(config_contents);
	g_assert_nonnull(strstr(config_contents, "doublesize=TRUE"));
	g_assert_nonnull(strstr(config_contents, "skin_autoscale=TRUE"));
	scale = strstr(config_contents, "skin_scale=");
	g_assert_nonnull(scale);
	seeded_scale = g_ascii_strtod(scale + strlen("skin_scale="), NULL);
	g_assert_cmpfloat(seeded_scale, >=, 1.5);
	g_free(config_contents);

	/* Layout: all three windows mapped, docked flush, centered. */
	eq_window = wait_for_window(display, "XMMS Equalizer");
	playlist_window = wait_for_window(display, "XMMS Playlist");
	g_assert_cmpuint(eq_window, !=, None);
	g_assert_cmpuint(playlist_window, !=, None);
	wait_for_geometry(display, main_window, &main_box);
	wait_for_geometry(display, eq_window, &eq_box);
	wait_for_geometry(display, playlist_window, &playlist_box);

	/* Equalizer flush to the player's right edge. */
	g_assert_cmpint(eq_box.y, ==, main_box.y);
	g_assert_cmpint(eq_box.x, ==, main_box.x + (gint) main_box.width);
	g_assert_cmpuint(eq_box.width, ==, DOUBLE_SIZE_WIDTH);
	g_assert_cmpuint(eq_box.height, ==, DOUBLE_SIZE_HEIGHT);

	/* Playlist flush below, spanning the player and equalizer. */
	g_assert_cmpint(playlist_box.x, ==, main_box.x);
	g_assert_cmpint(playlist_box.y, ==, main_box.y + (gint) main_box.height);
	g_assert_cmpuint(playlist_box.width, ==, 2 * DOUBLE_SIZE_WIDTH);
	g_assert_cmpuint(playlist_box.height, ==,
			 FIRST_RUN_PLAYLIST_HEIGHT * 2);

	/* The whole group is centered on the screen (clamped to the
	 * top-left corner when the screen is smaller than the group). */
	screen_w = DisplayWidth(display, DefaultScreen(display));
	screen_h = DisplayHeight(display, DefaultScreen(display));
	group_w = 2 * DOUBLE_SIZE_WIDTH;
	group_h = (SINGLE_SIZE_HEIGHT + FIRST_RUN_PLAYLIST_HEIGHT) * 2;
	expect_x = (screen_w - group_w) / 2;
	expect_y = (screen_h - group_h) / 2;
	if (expect_x < 0)
		expect_x = 0;
	if (expect_y < 0)
		expect_y = 0;
	g_assert_cmpint(main_box.x, ==, expect_x);
	g_assert_cmpint(main_box.y, ==, expect_y);

	finish_launch(child, display, runtime_directory);
	g_assert_cmpint(unexpected_x_error, ==, 0);
}

int main(int argc, char **argv)
{
	GPid xvfb_pid = 0;
	gchar *xvfb_display = NULL;
	int result;

	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/hidpi-startup/dpi-192-starts-doublesize",
			test_dpi_192_starts_doublesize);
	g_test_add_func("/hidpi-startup/dpi-96-matches-scale-rule",
			test_dpi_96_matches_scale_rule);
	g_test_add_func("/hidpi-startup/dpi-unset-matches-scale-rule",
			test_dpi_unset_matches_scale_rule);
	g_test_add_func("/hidpi-startup/opt-out-starts-single-size",
			test_autoscale_opt_out_starts_single_size);
	g_test_add_func("/hidpi-startup/skin-scale-override-pins-single-size",
			test_skin_scale_override_pins_single_size);
	g_test_add_func("/hidpi-startup/skin-scale-override-forces-doublesize",
			test_skin_scale_override_forces_doublesize);
	g_test_add_func("/hidpi-startup/resolution-4k-starts-doublesize",
			test_resolution_4k_starts_doublesize);
	g_test_add_func("/hidpi-startup/resolution-qhd-starts-single-size",
			test_resolution_qhd_starts_single_size);
	g_test_add_func("/hidpi-startup/skin-scale-override-triple-size",
			test_skin_scale_override_triple_size);
	g_test_add_func("/hidpi-startup/resolution-8k-starts-triple-size",
			test_resolution_8k_starts_triple_size);
	g_test_add_func("/hidpi-startup/skin-scale-override-wins-over-persisted-doublesize",
			test_skin_scale_override_wins_over_persisted_doublesize);
	g_test_add_func("/hidpi-startup/autoscale-clears-persisted-doublesize",
			test_autoscale_clears_persisted_doublesize);
	g_test_add_func("/hidpi-startup/first-run-seeds-doublesize-config",
			test_first_run_seeds_doublesize_config);

	/* Self-host the X server: the suite spawns real XMMS instances
	 * against the ambient display (plus private screens for the
	 * resolution cases), so owning the ambient server keeps the run
	 * deterministic regardless of how the caller provides a
	 * display. */
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
