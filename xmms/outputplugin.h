#ifndef XMMS_OUTPUTPLUGIN_H
#define XMMS_OUTPUTPLUGIN_H

#include <glib.h>

gchar *output_plugin_find_alsa(const gchar *output_plugin_dir);
gboolean output_plugin_is_unavailable_oss(const gchar *filename);

#endif
