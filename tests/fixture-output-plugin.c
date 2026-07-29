#include "../xmms/plugin.h"

OutputPlugin *get_oplugin_info(void)
{
	static OutputPlugin plugin;

	plugin.description = "fixture output";
	return &plugin;
}
