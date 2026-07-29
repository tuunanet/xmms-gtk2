/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 2026 Tuomo Tuunanen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */
#include "stream-position.h"

void mpg123_stream_position_init(Mpg123StreamPosition *position)
{
	position->data_start = 0;
	position->initialized = FALSE;
}

void mpg123_stream_position_set_data_start(Mpg123StreamPosition *position,
					   glong data_start)
{
	if (position->initialized)
		return;

	position->data_start = data_start > 0 ? data_start : 0;
	position->initialized = TRUE;
}

glong mpg123_stream_position_file_offset(const Mpg123StreamPosition *position,
					 glong stream_offset)
{
	return position->data_start + stream_offset;
}

guint32 mpg123_stream_position_data_size(const Mpg123StreamPosition *position,
					 guint32 file_size)
{
	if (position->data_start <= 0)
		return file_size;
	if ((guint64) position->data_start >= file_size)
		return 0;

	return file_size - position->data_start;
}

gdouble mpg123_stream_position_relative(const Mpg123StreamPosition *position,
					glong file_offset,
					guint32 data_size)
{
	glong stream_offset;

	if (data_size == 0)
		return 0;

	stream_offset = file_offset - position->data_start;
	if (stream_offset <= 0)
		return 0;

	return (gdouble) stream_offset / data_size;
}

gdouble mpg123_seek_percentage(gint time, gint frame_count,
			       gdouble seconds_per_frame)
{
	gdouble duration = frame_count * seconds_per_frame;

	if (duration <= 0)
		return 0;

	return CLAMP(((gdouble) time * 100) / duration, 0, 100);
}

gboolean mpg123_output_has_finished(gboolean output_playing,
				    gint output_time, gint track_length)
{
	return !output_playing ||
	       (track_length >= 0 && output_time >= track_length);
}
