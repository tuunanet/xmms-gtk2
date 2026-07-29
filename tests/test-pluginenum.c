#include <glib.h>

#include "../xmms/xmms.h"

Config cfg;
GList *disabled_iplugins;
struct InputPluginData *ip_data;
struct OutputPluginData *op_data;
struct EffectPluginData *ep_data;
struct GeneralPluginData *gp_data;
struct VisPluginData *vp_data;

void general_enable_from_stringified_list(gchar *list)
{
}

void vis_enable_from_stringified_list(gchar *list)
{
}

void effect_enable_from_stringified_list(gchar *list)
{
}

gint ctrlsocket_get_session_id(void)
{
	return 0;
}

InputVisType input_get_vis_type(void)
{
	return INPUT_VIS_OFF;
}

void input_add_vis_pcm(int time, AFormat fmt, int nch, int length, void *ptr)
{
}

void playlist_set_info(gchar *title, gint length, gint rate, gint freq, gint nch)
{
}

void input_set_info_text(gchar *text)
{
}

void vis_disable_plugin(VisPlugin *plugin)
{
}

static void test_loads_plugins_from_the_build_tree(void)
{
	gchar *input_path;
	gchar *output_path;
	InputPlugin *input;
	OutputPlugin *output;

	input_path = g_build_filename(BUILD_PLUGIN_DIR, "Input", "fixture",
				      ".libs", "libfixture-input.so", NULL);
	output_path = g_build_filename(BUILD_PLUGIN_DIR, "Output", "fixture",
				       ".libs", "libfixture-output.so", NULL);
	cfg.outputplugin = g_strdup(output_path);

	init_plugins();

	g_assert_nonnull(ip_data->input_list);
	g_assert_nonnull(op_data->output_list);
	input = ip_data->input_list->data;
	output = op_data->output_list->data;
	g_assert_cmpstr(input->filename, ==, input_path);
	g_assert_cmpstr(output->filename, ==, output_path);
	g_assert_true(op_data->current_output_plugin == output);

	g_free(input_path);
	g_free(output_path);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/pluginenum/loads-plugins-from-build-tree",
			test_loads_plugins_from_the_build_tree);

	return g_test_run();
}
