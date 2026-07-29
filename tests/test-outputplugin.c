#include <glib.h>

#include "../xmms/outputplugin.h"

static void test_finds_alsa_from_the_build_tree(void)
{
	gchar *expected;
	gchar *actual;

	expected = g_build_filename(BUILD_PLUGIN_DIR, "Output", "alsa",
				    ".libs", "libALSA.so", NULL);
	actual = output_plugin_find_alsa("Output");

	g_assert_cmpstr(actual, ==, expected);
	g_free(actual);
	g_free(expected);
}

static void test_replaces_an_unavailable_oss_plugin(void)
{
	if (g_file_test("/dev/dsp", G_FILE_TEST_EXISTS))
	{
		g_test_skip("OSS device is available");
		return;
	}

	g_assert_true(output_plugin_is_unavailable_oss("/tmp/libOSS.so"));
	g_assert_false(output_plugin_is_unavailable_oss("/tmp/libALSA.so"));
	g_assert_false(output_plugin_is_unavailable_oss("/tmp/not-libOSS.so"));
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/outputplugin/finds-alsa-from-build-tree",
			test_finds_alsa_from_the_build_tree);
	g_test_add_func("/outputplugin/replaces-unavailable-oss",
			test_replaces_an_unavailable_oss_plugin);

	return g_test_run();
}
