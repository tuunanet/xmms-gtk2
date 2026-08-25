#ifndef MPG123_ID3V2_WRITE_H
#define MPG123_ID3V2_WRITE_H

#include <glib.h>

gboolean mpg123_id3v2_write_title(const char *filename, const char *title,
				  gboolean *found, char **error);

#endif
