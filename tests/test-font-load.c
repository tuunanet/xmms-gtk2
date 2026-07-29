#include <glib.h>
#include <gtk/gtk.h>

#include "../xmms/xmms.h"
#include "../xmms/util.h"

Config cfg;

static void assert_default_loads_byte_compatible(const gchar *name)
{
	GdkFont *font;

	cfg.use_fontsets = FALSE;
	font = util_font_load((gchar *) name);
	g_assert_nonnull(font);
	g_assert_cmpint(gdk_text_width(font, "ABC", 3), ==,
	                gdk_char_width(font, 'A') +
	                gdk_char_width(font, 'B') +
	                gdk_char_width(font, 'C'));
	gdk_font_unref(font);
}

static void test_legacy_playlist_default(void)
{
	assert_default_loads_byte_compatible(XMMS_LEGACY_PLAYLIST_FONT);
}

static void test_legacy_main_window_default(void)
{
	assert_default_loads_byte_compatible(XMMS_LEGACY_MAINWIN_FONT);
}

static void test_interim_playlist_default(void)
{
	assert_default_loads_byte_compatible(XMMS_INTERIM_PLAYLIST_FONT);
}

static void test_interim_main_window_default(void)
{
	assert_default_loads_byte_compatible(XMMS_INTERIM_MAINWIN_FONT);
}

static void test_interim_fixed_playlist_default(void)
{
	assert_default_loads_byte_compatible(XMMS_INTERIM_FIXED_PLAYLIST_FONT);
}

static void test_interim_fixed_main_window_default(void)
{
	assert_default_loads_byte_compatible(XMMS_INTERIM_FIXED_MAINWIN_FONT);
}

static void test_interim_proportional_playlist_default(void)
{
	assert_default_loads_byte_compatible(
		XMMS_INTERIM_PROPORTIONAL_PLAYLIST_FONT);
}

static void test_interim_proportional_main_window_default(void)
{
	assert_default_loads_byte_compatible(
		XMMS_INTERIM_PROPORTIONAL_MAINWIN_FONT);
}

static void test_interim_classic_default(void)
{
	assert_default_loads_byte_compatible(XMMS_INTERIM_CLASSIC_FONT);
}

static void assert_default_matches_classic_fixed(const gchar *name)
{
	GdkFont *font, *classic;

	cfg.use_fontsets = FALSE;
	font = util_font_load((gchar *) name);
	classic = gdk_font_load("fixed");
	g_assert_nonnull(font);
	g_assert_nonnull(classic);
	g_assert_cmpint(font->ascent, ==, classic->ascent);
	g_assert_cmpint(font->descent, ==, classic->descent);
	g_assert_cmpint(gdk_text_width(font, "XMMS Classic", 12), ==,
	                gdk_text_width(classic, "XMMS Classic", 12));
	gdk_font_unref(classic);
	gdk_font_unref(font);
}

static void test_preferred_playlist_default(void)
{
	g_assert_nonnull(strstr(XMMS_DEFAULT_PLAYLIST_FONT, "-bold-"));
	assert_default_matches_classic_fixed(XMMS_DEFAULT_PLAYLIST_FONT);
}

static void test_preferred_main_window_default(void)
{
	g_assert_nonnull(strstr(XMMS_DEFAULT_MAINWIN_FONT, "-medium-"));
	assert_default_matches_classic_fixed(XMMS_DEFAULT_MAINWIN_FONT);
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);
	g_test_init(&argc, &argv, NULL);
	g_log_set_always_fatal(G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL |
	                       G_LOG_LEVEL_WARNING);
	g_test_add_func("/font/legacy-playlist-default",
	                test_legacy_playlist_default);
	g_test_add_func("/font/legacy-main-window-default",
	                test_legacy_main_window_default);
	g_test_add_func("/font/interim-playlist-default",
	                test_interim_playlist_default);
	g_test_add_func("/font/interim-main-window-default",
	                test_interim_main_window_default);
	g_test_add_func("/font/interim-fixed-playlist-default",
	                test_interim_fixed_playlist_default);
	g_test_add_func("/font/interim-fixed-main-window-default",
	                test_interim_fixed_main_window_default);
	g_test_add_func("/font/interim-proportional-playlist-default",
	                test_interim_proportional_playlist_default);
	g_test_add_func("/font/interim-proportional-main-window-default",
	                test_interim_proportional_main_window_default);
	g_test_add_func("/font/interim-classic-default",
	                test_interim_classic_default);
	g_test_add_func("/font/preferred-playlist-default",
	                test_preferred_playlist_default);
	g_test_add_func("/font/preferred-main-window-default",
	                test_preferred_main_window_default);
	return g_test_run();
}
