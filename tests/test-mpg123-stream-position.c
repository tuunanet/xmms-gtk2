#include <glib.h>

#include "../Input/mpg123/stream-position.h"

static void test_offsets_tagged_stream_seeks_from_first_frame(void)
{
	Mpg123StreamPosition position;
	const glong file_size_without_id3v1 = 118465850;
	const glong id3v2_size = 638138;
	const glong xing_audio_size = 117827712;

	mpg123_stream_position_init(&position);
	mpg123_stream_position_set_data_start(&position, id3v2_size);

	g_assert_cmpint(mpg123_stream_position_data_size(
			&position, file_size_without_id3v1), ==, xing_audio_size);
	g_assert_cmpint(mpg123_stream_position_file_offset(
			&position, xing_audio_size), ==, file_size_without_id3v1);
	g_assert_cmpfloat_with_epsilon(mpg123_stream_position_relative(
			&position, id3v2_size + (xing_audio_size / 2),
			xing_audio_size), 0.5, 0.000001);
}

static void test_offsets_untagged_stream_from_file_start(void)
{
	Mpg123StreamPosition position;

	mpg123_stream_position_init(&position);
	mpg123_stream_position_set_data_start(&position, 0);

	g_assert_cmpint(mpg123_stream_position_data_size(&position, 4096), ==,
			4096);
	g_assert_cmpint(mpg123_stream_position_file_offset(&position, 2048), ==,
			2048);
}

static void test_preserves_fractional_xing_seek_percentage(void)
{
	const gint frames = 151775;
	const gdouble seconds_per_frame = 0.024;
	gdouble percent;

	percent = mpg123_seek_percentage(3642, frames, seconds_per_frame);

	g_assert_cmpfloat_with_epsilon(percent, 99.9835273664, 0.000001);
}

static void test_known_length_reaches_eof_with_running_output(void)
{
	g_assert_true(mpg123_output_has_finished(TRUE, 3642600, 3642600));
	g_assert_false(mpg123_output_has_finished(TRUE, 3642500, 3642600));
}

static void test_unknown_length_uses_output_state_for_eof(void)
{
	g_assert_false(mpg123_output_has_finished(TRUE, 1000, -1));
	g_assert_true(mpg123_output_has_finished(FALSE, 1000, -1));
}

static void test_preserves_first_mpeg_frame_offset(void)
{
	Mpg123StreamPosition position;

	mpg123_stream_position_init(&position);
	mpg123_stream_position_set_data_start(&position, 638138);
	mpg123_stream_position_set_data_start(&position, 900000);

	g_assert_cmpint(mpg123_stream_position_file_offset(&position, 0), ==,
			638138);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/mpg123-stream-position/tagged-stream",
			test_offsets_tagged_stream_seeks_from_first_frame);
	g_test_add_func("/mpg123-stream-position/untagged-stream",
			test_offsets_untagged_stream_from_file_start);
	g_test_add_func("/mpg123-stream-position/preserves-first-frame",
			test_preserves_first_mpeg_frame_offset);
	g_test_add_func("/mpg123-stream-position/fractional-xing-percentage",
			test_preserves_fractional_xing_seek_percentage);
	g_test_add_func("/mpg123-stream-position/known-length-eof",
			test_known_length_reaches_eof_with_running_output);
	g_test_add_func("/mpg123-stream-position/unknown-length-eof",
			test_unknown_length_uses_output_state_for_eof);

	return g_test_run();
}
