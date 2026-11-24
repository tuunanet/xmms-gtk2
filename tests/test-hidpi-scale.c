/* Unit tests for the display signals behind the HiDPI startup
 * decision: strict Xft.dpi parsing with a 96 DPI fallback,
 * RESOURCE_MANAGER database lookup, and the skin scale mapping from
 * the QHD resolution baseline and DPI coefficients. */
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
	g_assert_cmpint(util_skin_scale_for_dpi(239), ==, 2);
	/* 2.5x of the default DPI and beyond reach the 3x scale. */
	g_assert_cmpint(util_skin_scale_for_dpi(240), ==, 3);
	g_assert_cmpint(util_skin_scale_for_dpi(XMMS_MAX_SANE_DPI), ==, XMMS_SKIN_MAX_SCALE);
}

static void test_coefficient_resolution_baseline(void)
{
	/* QHD is the accepted default: the classic 1x UI keeps the
	 * coefficient at exactly 1. */
	g_assert_cmpfloat(util_skin_coefficient_for_resolution(2560, 1440), ==, 1.0);
	g_assert_cmpfloat(util_skin_coefficient_for_resolution(2560, 1600), ==, 1.0);
	g_assert_cmpfloat(util_skin_coefficient_for_resolution(3440, 1440), ==, 1.0);
	/* Larger displays scale proportionally. */
	g_assert_cmpfloat(util_skin_coefficient_for_resolution(1920, 1080), ==, 0.75);
	g_assert_cmpfloat(util_skin_coefficient_for_resolution(1280, 1024), ==, 0.5);
	g_assert_cmpfloat(util_skin_coefficient_for_resolution(3840, 2160), ==, 1.5);
	g_assert_cmpfloat(util_skin_coefficient_for_resolution(5120, 2880), ==, 2.0);
	/* The tighter axis wins so ultrawide panels do not over-scale. */
	g_assert_cmpfloat(util_skin_coefficient_for_resolution(2880, 1620), ==, 1.125);
}

static void test_skin_scale_coefficient_mapping(void)
{
	g_assert_cmpint(util_skin_scale_for_coefficient(0.0), ==, 1);
	g_assert_cmpint(util_skin_scale_for_coefficient(0.75), ==, 1);
	g_assert_cmpint(util_skin_scale_for_coefficient(1.0), ==, 1);
	g_assert_cmpint(util_skin_scale_for_coefficient(1.49), ==, 1);
	/* 1.5 rounds up to DoubleSize. */
	g_assert_cmpint(util_skin_scale_for_coefficient(1.5), ==, 2);
	g_assert_cmpint(util_skin_scale_for_coefficient(2.0), ==, 2);
	g_assert_cmpint(util_skin_scale_for_coefficient(2.49), ==, 2);
	/* 2.5 rounds up to the 3x the renderer supports since the N x
	 * integer scaling work. */
	g_assert_cmpint(util_skin_scale_for_coefficient(2.5), ==, 3);
	g_assert_cmpint(util_skin_scale_for_coefficient(3.0), ==, XMMS_SKIN_MAX_SCALE);
	/* Beyond the renderer cap the coefficient saturates. */
	g_assert_cmpint(util_skin_scale_for_coefficient(4.0), ==, XMMS_SKIN_MAX_SCALE);
}

static void test_skin_scale_display_combination(void)
{
	/* The strongest of resolution and DPI decides. */
	g_assert_cmpint(util_skin_scale_for_display(1920, 1080, 96), ==, 1);
	g_assert_cmpint(util_skin_scale_for_display(2560, 1440, 96), ==, 1);
	g_assert_cmpint(util_skin_scale_for_display(3440, 1440, 96), ==, 1);
	g_assert_cmpint(util_skin_scale_for_display(3840, 2160, 96), ==, 2);
	g_assert_cmpint(util_skin_scale_for_display(3840, 2160, 143), ==, 2);
	g_assert_cmpint(util_skin_scale_for_display(2560, 1440, 143), ==, 1);
	g_assert_cmpint(util_skin_scale_for_display(2560, 1440, 144), ==, 2);
	g_assert_cmpint(util_skin_scale_for_display(1920, 1080, 192), ==, 2);
	/* 8K-class resolutions reach the 3x scale. */
	g_assert_cmpint(util_skin_scale_for_display(7680, 4320, 96), ==, 3);
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
	g_test_add_func("/hidpi-scale/coefficient-resolution-baseline",
			test_coefficient_resolution_baseline);
	g_test_add_func("/hidpi-scale/skin-scale-coefficient-mapping",
			test_skin_scale_coefficient_mapping);
	g_test_add_func("/hidpi-scale/skin-scale-display-combination",
			test_skin_scale_display_combination);

	return g_test_run();
}
