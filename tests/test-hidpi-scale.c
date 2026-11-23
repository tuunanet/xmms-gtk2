#include <glib.h>
#include <gtk/gtk.h>

#include "../xmms/xmms.h"
#include "../xmms/util.h"

Config cfg;

static void test_parse_integer_dpi(void)
{
	g_assert_cmpint(util_xft_dpi_parse("96"), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse("192"), ==, 192);
	g_assert_cmpint(util_xft_dpi_parse("144"), ==, 144);
}

static void test_parse_rounds_fractional_dpi(void)
{
	g_assert_cmpint(util_xft_dpi_parse("96.5"), ==, 97);
	g_assert_cmpint(util_xft_dpi_parse("192.4"), ==, 192);
}

static void test_parse_rejects_unusable_values(void)
{
	g_assert_cmpint(util_xft_dpi_parse(NULL), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse(""), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse("abc"), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse("192abc"), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse("0"), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse("-5"), ==, 96);
	g_assert_cmpint(util_xft_dpi_parse("10000"), ==, 96);
}

static void test_parse_accepts_sane_extremes(void)
{
	g_assert_cmpint(util_xft_dpi_parse("4096"), ==, 4096);
	g_assert_cmpint(util_xft_dpi_parse("1"), ==, 1);
}

static void test_rdb_reads_xft_dpi(void)
{
	g_assert_cmpint(util_xft_dpi_from_rdb("Xft.dpi: 192"), ==, 192);
	g_assert_cmpint(util_xft_dpi_from_rdb("other.setting: 1\n"
					      "Xft.dpi:\t144\n"),
			==, 144);
}

static void test_rdb_falls_back_without_xft_dpi(void)
{
	g_assert_cmpint(util_xft_dpi_from_rdb(NULL), ==, 96);
	g_assert_cmpint(util_xft_dpi_from_rdb(""), ==, 96);
	g_assert_cmpint(util_xft_dpi_from_rdb("unrelated: yes"), ==, 96);
	g_assert_cmpint(util_xft_dpi_from_rdb("Xft.dpi: bogus"), ==, 96);
}

static void test_scale_is_one_for_standard_dpi(void)
{
	g_assert_cmpint(util_skin_scale_for_dpi(0), ==, 1);
	g_assert_cmpint(util_skin_scale_for_dpi(95), ==, 1);
	g_assert_cmpint(util_skin_scale_for_dpi(96), ==, 1);
	g_assert_cmpint(util_skin_scale_for_dpi(143), ==, 1);
}

static void test_scale_is_two_for_hidpi(void)
{
	g_assert_cmpint(util_skin_scale_for_dpi(144), ==, 2);
	g_assert_cmpint(util_skin_scale_for_dpi(192), ==, 2);
	g_assert_cmpint(util_skin_scale_for_dpi(288), ==, 2);
	g_assert_cmpint(util_skin_scale_for_dpi(4096), ==, 2);
}

static void test_get_dpi_returns_a_usable_value(void)
{
	gint dpi = util_get_xft_dpi();

	g_assert_cmpint(dpi, >=, 1);
	g_assert_cmpint(dpi, <=, 4096);
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/hidpi/parse-integer-dpi",
			test_parse_integer_dpi);
	g_test_add_func("/hidpi/parse-rounds-fractional-dpi",
			test_parse_rounds_fractional_dpi);
	g_test_add_func("/hidpi/parse-rejects-unusable-values",
			test_parse_rejects_unusable_values);
	g_test_add_func("/hidpi/parse-accepts-sane-extremes",
			test_parse_accepts_sane_extremes);
	g_test_add_func("/hidpi/rdb-reads-xft-dpi",
			test_rdb_reads_xft_dpi);
	g_test_add_func("/hidpi/rdb-falls-back-without-xft-dpi",
			test_rdb_falls_back_without_xft_dpi);
	g_test_add_func("/hidpi/scale-is-one-for-standard-dpi",
			test_scale_is_one_for_standard_dpi);
	g_test_add_func("/hidpi/scale-is-two-for-hidpi",
			test_scale_is_two_for_hidpi);
	g_test_add_func("/hidpi/get-dpi-returns-a-usable-value",
			test_get_dpi_returns_a_usable_value);
	return g_test_run();
}
