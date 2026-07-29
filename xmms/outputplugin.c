#include <string.h>

#include <glib.h>

#include "outputplugin.h"

static gchar *existing_plugin_path(gchar *path)
{
	if (g_file_test(path, G_FILE_TEST_EXISTS))
		return path;
	g_free(path);
	return NULL;
}

gchar *output_plugin_find_alsa(const gchar *output_plugin_dir)
{
	gchar *path;

#ifdef BUILD_PLUGIN_DIR
	if (!g_file_test(PLUGIN_DIR, G_FILE_TEST_IS_DIR))
	{
		path = g_build_filename(BUILD_PLUGIN_DIR, "Output", "alsa",
						".libs", "libALSA.so", NULL);
		path = existing_plugin_path(path);
		if (path)
			return path;
	}
#endif

	path = g_build_filename(PLUGIN_DIR, output_plugin_dir, "libALSA.so", NULL);
	return existing_plugin_path(path);
}

gboolean output_plugin_is_unavailable_oss(const gchar *filename)
{
	const gchar *basename;

	if (filename == NULL || g_file_test("/dev/dsp", G_FILE_TEST_EXISTS))
		return FALSE;

	basename = strrchr(filename, G_DIR_SEPARATOR);
	if (basename)
		basename++;
	else
		basename = filename;

	return !strcmp(basename, "libOSS.so");
}
