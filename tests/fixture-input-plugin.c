#include "../xmms/plugin.h"

InputPlugin *get_iplugin_info(void)
{
	static InputPlugin plugin;

	plugin.description = "fixture input";
	return &plugin;
}
