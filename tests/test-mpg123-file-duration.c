#include <stdio.h>

#include <glib.h>

#include "../Input/mpg123/mpg123.h"

/* Header decoding assigns these playback functions, but duration scanning does
 * not decode audio. */
int mpg123_do_layer1(struct frame *fr)
{
	return 0;
}

int mpg123_do_layer2(struct frame *fr)
{
	return 0;
}

int mpg123_do_layer3(struct frame *fr)
{
	return 0;
}

void mpg123_init_layer2(gboolean mmx)
{
}

static void write_mpeg1_layer3_frame(FILE *file, guint bitrate_index)
{
	const guint32 header = 0xfffb0400 | (bitrate_index << 12);
	const guint bitrates[] =
		{ 0, 32, 40, 48, 56, 64, 80, 96,
		  112, 128, 160, 192, 224, 256, 320 };
	const guint frame_size = 3 * bitrates[bitrate_index];
	guint i;

	fputc((header >> 24) & 0xff, file);
	fputc((header >> 16) & 0xff, file);
	fputc((header >> 8) & 0xff, file);
	fputc(header & 0xff, file);
	for (i = 4; i < frame_size; i++)
		fputc(0, file);
}

static void test_duration_scans_headerless_vbr_frames(void)
{
	FILE *file = tmpfile();
	guint i;

	g_assert_nonnull(file);
	for (i = 0; i < 10; i++)
		write_mpeg1_layer3_frame(file, i % 2 ? 13 : 1);
	rewind(file);

	/* MPEG-1 Layer III at 48 kHz carries 24 ms per frame. */
	g_assert_cmpint(mpg123_get_file_duration(file), ==, 240);

	fclose(file);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/mpg123-file-duration/headerless-vbr",
			test_duration_scans_headerless_vbr_frames);

	return g_test_run();
}
