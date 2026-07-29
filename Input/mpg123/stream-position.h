/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 2026 Tuomo Tuunanen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */
#ifndef MPG123_STREAM_POSITION_H
#define MPG123_STREAM_POSITION_H

#include <glib.h>

typedef struct
{
	glong data_start;
	gboolean initialized;
} Mpg123StreamPosition;

void mpg123_stream_position_init(Mpg123StreamPosition *position);
void mpg123_stream_position_set_data_start(Mpg123StreamPosition *position,
					   glong data_start);
glong mpg123_stream_position_file_offset(const Mpg123StreamPosition *position,
					 glong stream_offset);
guint32 mpg123_stream_position_data_size(const Mpg123StreamPosition *position,
					 guint32 file_size);
gdouble mpg123_stream_position_relative(const Mpg123StreamPosition *position,
					glong file_offset,
					guint32 data_size);
gdouble mpg123_seek_percentage(gint time, gint frame_count,
			       gdouble seconds_per_frame);
gboolean mpg123_output_has_finished(gboolean output_playing,
				    gint output_time, gint track_length);

#endif
