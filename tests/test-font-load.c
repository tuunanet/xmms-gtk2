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

static void test_interim_semicondensed_playlist_migrates(void)
{
	GdkFont *font, *current;

	/* The previous default (13px bold semicondensed) resolves to the
	 * current native 6x12 default through the migration list. */
	cfg.use_fontsets = FALSE;
	font = util_font_load((gchar *) XMMS_INTERIM_SEMICONDENSED_PLAYLIST_FONT);
	current = util_font_load((gchar *) XMMS_DEFAULT_PLAYLIST_FONT);
	g_assert_nonnull(font);
	g_assert_nonnull(current);
	g_assert_cmpint(font->ascent, ==, current->ascent);
	g_assert_cmpint(font->descent, ==, current->descent);
	g_assert_cmpint(gdk_text_width(font, "XMMS GTK2", 9), ==,
	                gdk_text_width(current, "XMMS GTK2", 9));
	gdk_font_unref(current);
	gdk_font_unref(font);
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
	g_assert_cmpint(gdk_text_width(font, "XMMS GTK2", 9), ==,
	                gdk_text_width(classic, "XMMS GTK2", 9));
	gdk_font_unref(classic);
	gdk_font_unref(font);
}

static void test_preferred_playlist_default(void)
{
	GdkFont *font;

	g_assert_nonnull(strstr(XMMS_DEFAULT_PLAYLIST_FONT, "-medium-"));
	g_assert_nonnull(strstr(XMMS_DEFAULT_PLAYLIST_FONT, "-semicondensed-"));
	cfg.use_fontsets = FALSE;
	font = util_font_load((gchar *) XMMS_DEFAULT_PLAYLIST_FONT);
	g_assert_nonnull(font);
	/* Native 6x12 bitmap: 12 pixels tall, one 6px cell per character.
	 * A server-scaled substitute would report different metrics. */
	g_assert_cmpint(font->ascent + font->descent, ==, 12);
	g_assert_cmpint(gdk_text_width(font, "XMMS GTK2", 9), ==, 54);
	gdk_font_unref(font);
}

static void test_preferred_main_window_default(void)
{
	g_assert_nonnull(strstr(XMMS_DEFAULT_MAINWIN_FONT, "-medium-"));
	assert_default_matches_classic_fixed(XMMS_DEFAULT_MAINWIN_FONT);
}

static void test_scale_default_playlist_font(void)
{
	gchar *scaled;

	scaled = util_font_scale_name(XMMS_DEFAULT_PLAYLIST_FONT,
	                              XMMS_DEFAULT_PLAYLIST_FONT_SCALE);
	g_assert_null(scaled);

	scaled = util_font_scale_name(XMMS_DEFAULT_PLAYLIST_FONT, 90);
	g_assert_nonnull(scaled);
	g_assert_cmpstr(scaled, ==,
	                "-misc-fixed-medium-r-semicondensed--11-99-75-75-c-60-iso8859-1");
	g_free(scaled);
}

static void test_scale_identity_is_null(void)
{
	g_assert_null(util_font_scale_name(XMMS_DEFAULT_PLAYLIST_FONT, 100));
}

static void test_scale_alias_is_null(void)
{
	g_assert_null(util_font_scale_name(XMMS_INTERIM_CLASSIC_FONT,
	                                   XMMS_DEFAULT_PLAYLIST_FONT_SCALE));
}

static void test_scale_wildcard_is_null(void)
{
	g_assert_null(util_font_scale_name(XMMS_INTERIM_PLAYLIST_FONT,
	                                   XMMS_DEFAULT_PLAYLIST_FONT_SCALE));
}

static void test_scale_keeps_dashed_encoding(void)
{
	gchar *scaled;

	scaled = util_font_scale_name(
		"-misc-fixed-bold-r-normal--10-100-75-75-c-60-iso10646-1",
		90);
	g_assert_nonnull(scaled);
	g_assert_cmpstr(scaled, ==,
	                "-misc-fixed-bold-r-normal--9-90-75-75-c-60-iso10646-1");
	g_free(scaled);
}

static void test_scale_is_clamped(void)
{
	gchar *scaled;

	scaled = util_font_scale_name(XMMS_DEFAULT_PLAYLIST_FONT, 5);
	g_assert_nonnull(scaled);
	g_assert_cmpstr(scaled, ==,
	                "-misc-fixed-medium-r-semicondensed--3-28-75-75-c-60-iso8859-1");
	g_free(scaled);

	scaled = util_font_scale_name(XMMS_DEFAULT_PLAYLIST_FONT, 100000);
	g_assert_nonnull(scaled);
	g_assert_cmpstr(scaled, ==,
	                "-misc-fixed-medium-r-semicondensed--48-440-75-75-c-60-iso8859-1");
	g_free(scaled);
}

static void test_scaled_playlist_font_loads_smaller(void)
{
	GdkFont *unscaled, *scaled;
	gchar *name;

	cfg.use_fontsets = FALSE;
	unscaled = util_font_load((gchar *) XMMS_DEFAULT_PLAYLIST_FONT);
	g_assert_nonnull(unscaled);
	name = util_font_scale_name(XMMS_DEFAULT_PLAYLIST_FONT, 90);
	g_assert_nonnull(name);
	scaled = util_font_load(name);
	g_free(name);
	g_assert_nonnull(scaled);
	g_assert_cmpint(scaled->ascent + scaled->descent, <,
	                unscaled->ascent + unscaled->descent);
	gdk_font_unref(scaled);
	gdk_font_unref(unscaled);
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
	g_test_add_func("/font/interim-semicondensed-playlist-migrates",
	                test_interim_semicondensed_playlist_migrates);
	g_test_add_func("/font/preferred-playlist-default",
	                test_preferred_playlist_default);
	g_test_add_func("/font/preferred-main-window-default",
	                test_preferred_main_window_default);
	g_test_add_func("/font/scale-default-playlist-font",
	                test_scale_default_playlist_font);
	g_test_add_func("/font/scale-identity-is-null",
	                test_scale_identity_is_null);
	g_test_add_func("/font/scale-alias-is-null",
	                test_scale_alias_is_null);
	g_test_add_func("/font/scale-wildcard-is-null",
	                test_scale_wildcard_is_null);
	g_test_add_func("/font/scale-keeps-dashed-encoding",
	                test_scale_keeps_dashed_encoding);
	g_test_add_func("/font/scale-is-clamped",
	                test_scale_is_clamped);
	g_test_add_func("/font/scaled-playlist-font-loads-smaller",
	                test_scaled_playlist_font_loads_smaller);
	return g_test_run();
}
