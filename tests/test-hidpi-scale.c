/* Unit tests for the Xft.dpi helpers used by the HiDPI startup
 * decision: strict parsing with a 96 DPI fallback, RESOURCE_MANAGER
 * database lookup, and the skin scale threshold. */
#include <glib.h>
#include <gtk/gtk.h>

#include "../xmms/util.h"

static void test_parse_accepts_integer(void)
{
	g_assert_cmpint(util_xft_dpi_parse("192"), ==, 192);
	g_assert_cmpint(util_xft_dpi_parse("96"), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse("144"), ==, 144);
}

static void test_parse_strips_whitespace(void)
{
	g_assert_cmpint(util_xft_dpi_parse("  192  "), ==, 192);
	g_assert_cmpint(util_xft_dpi_parse("192\n"), ==, 192);
}

static void test_parse_rounds_fractional(void)
{
	g_assert_cmpint(util_xft_dpi_parse("96.5"), ==, 97);
	g_assert_cmpint(util_xft_dpi_parse("144.4"), ==, 144);
	g_assert_cmpint(util_xft_dpi_parse("144.5"), ==, 145);
}

static void test_parse_rejects_unusable_values(void)
{
	g_assert_cmpint(util_xft_dpi_parse(NULL), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse(""), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse("bogus"), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse("192x"), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse("0"), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse("-3"), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse("4097"), ==, 96);
}

static void test_rdb_looks_up_xft_dpi(void)
{
	g_assert_cmpint(util_xft_dpi_from_rdb("Xft.dpi: 192\n"), ==, 192);
	g_assert_cmpint(util_xft_dpi_from_rdb("foo.bar: 1\n"
					      "Xft.dpi:\t168\n"), ==, 168);
}

static void test_rdb_falls_back_to_default(void)
{
	g_assert_cmpint(util_xft_dpi_from_rdb(NULL), ==, 96);
	g_assert_cmpint(util_xft_dpi_from_rdb(""), ==, 96);
	g_assert_cmpint(util_xft_dpi_from_rdb("Xft.dpi: bogus\n"), ==, 96);
	g_assert_cmpint(util_xft_dpi_from_rdb("Xft.hinting: true\n"), ==, 96);
}

static void test_skin_scale_threshold(void)
{
	g_assert_cmpint(util_skin_scale_for_dpi(95), ==, 1);
	g_assert_cmpint(util_skin_scale_for_dpi(96), ==, 1);
	g_assert_cmpint(util_skin_scale_for_dpi(143), ==, 1);
	g_assert_cmpint(util_skin_scale_for_dpi(XMMS_HIDPI_DOUBLE_DPI), ==, 2);
	g_assert_cmpint(util_skin_scale_for_dpi(192), ==, 2);
	g_assert_cmpint(util_skin_scale_for_dpi(XMMS_MAX_SANE_DPI), ==, 2);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/hidpi-scale/parse-accepts-integer",
			test_parse_accepts_integer);
	g_test_add_func("/hidpi-scale/parse-strips-whitespace",
			test_parse_strips_whitespace);
	g_test_add_func("/hidpi-scale/parse-rounds-fractional",
			test_parse_rounds_fractional);
	g_test_add_func("/hidpi-scale/parse-rejects-unusable-values",
			test_parse_rejects_unusable_values);
	g_test_add_func("/hidpi-scale/rdb-looks-up-xft-dpi",
			test_rdb_looks_up_xft_dpi);
	g_test_add_func("/hidpi-scale/rdb-falls-back-to-default",
			test_rdb_falls_back_to_default);
	g_test_add_func("/hidpi-scale/skin-scale-threshold",
			test_skin_scale_threshold);

	return g_test_run();
}
