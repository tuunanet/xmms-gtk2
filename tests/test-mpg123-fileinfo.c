#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>

#include "../Input/mpg123/id3_header.h"
#include "../Input/mpg123/id3v24-write.h"
#include "../Input/mpg123/fileinfo.c"

static void write_id3v2_title(FILE *file, const char *title)
{
	size_t title_size = strlen(title) + 1;
	size_t tag_size = ID3_FRAMEHDR_SIZE + title_size;

	fwrite("ID3", 1, 3, file);
	fputc(4, file);
	fputc(0, file);
	fputc(0, file);
	fputc((tag_size >> 21) & 0x7f, file);
	fputc((tag_size >> 14) & 0x7f, file);
	fputc((tag_size >> 7) & 0x7f, file);
	fputc(tag_size & 0x7f, file);
	fwrite("TIT2", 1, 4, file);
	fputc((title_size >> 24) & 0xff, file);
	fputc((title_size >> 16) & 0xff, file);
	fputc((title_size >> 8) & 0xff, file);
	fputc(title_size & 0xff, file);
	fputc(0, file);
	fputc(0, file);
	fputc(ID3_ENCODING_ISO_8859_1, file);
	fwrite(title, 1, title_size - 1, file);
}

static void write_id3v24_txxx_frame(FILE *file, size_t frame_size)
{
	size_t i;

	fwrite("TXXX", 1, 4, file);
	fputc((frame_size >> 21) & 0x7f, file);
	fputc((frame_size >> 14) & 0x7f, file);
	fputc((frame_size >> 7) & 0x7f, file);
	fputc(frame_size & 0x7f, file);
	fputc(0, file);
	fputc(0, file);
	fputc(ID3_ENCODING_UTF8, file);
	fputc(0, file);
	for (i = 2; i < frame_size; i++)
		fputc('A', file);
}

static void test_long_id3v2_title_is_loaded(void)
{
	const char *expected = "An ID3v2 title longer than thirty bytes";
	FILE *file = tmpfile();
	char *title;

	g_assert_nonnull(file);
	write_id3v2_title(file, expected);
	rewind(file);

	title = mpg123_file_info_get_id3v2_title(file);
	g_assert_cmpstr(title, ==, expected);
	g_free(title);
	fclose(file);
}

static void test_id3v24_syncsafe_frame_sizes_are_loaded(void)
{
	const char *expected = "Hertz - Electronic Fridays 2026.7";
	size_t title_size = strlen(expected) + 1;
	size_t tag_size = ID3_FRAMEHDR_SIZE + title_size + 10 + 128 + 10 + 300;
	FILE *file = tmpfile();
	char *title;

	g_assert_nonnull(file);
	fwrite("ID3", 1, 3, file);
	fputc(4, file);
	fputc(0, file);
	fputc(0, file);
	fputc((tag_size >> 21) & 0x7f, file);
	fputc((tag_size >> 14) & 0x7f, file);
	fputc((tag_size >> 7) & 0x7f, file);
	fputc(tag_size & 0x7f, file);
	fwrite("TIT2", 1, 4, file);
	fputc(0, file);
	fputc(0, file);
	fputc(0, file);
	fputc(title_size, file);
	fputc(0, file);
	fputc(0, file);
	fputc(ID3_ENCODING_ISO_8859_1, file);
	fwrite(expected, 1, title_size - 1, file);
	write_id3v24_txxx_frame(file, 128);
	write_id3v24_txxx_frame(file, 300);
	rewind(file);

	title = mpg123_file_info_get_id3v2_title(file);
	g_assert_cmpstr(title, ==, expected);
	g_free(title);
	fclose(file);
}

static void test_id3v24_title_write_preserves_audio(void)
{
	char filename[] = "/tmp/xmms-id3v24-XXXXXX";
	const char *expected = "A replacement title that is longer than thirty bytes";
	int fd = g_mkstemp(filename);
	FILE *file;
	char *title, audio[6] = { 0 };
	char *error = NULL;
	size_t audio_length;
	gboolean found = FALSE;

	g_assert_cmpint(fd, !=, -1);
	file = fdopen(fd, "wb");
	g_assert_nonnull(file);
	write_id3v2_title(file, "old title");
	fwrite("audio", 1, 5, file);
	fclose(file);

	g_assert_true(mpg123_id3v24_write_title(filename, expected, &found, &error));
	g_assert_true(found);
	g_assert_null(error);
	file = fopen(filename, "rb");
	g_assert_nonnull(file);
	if (!file)
	{
		g_unlink(filename);
		return;
	}
	title = mpg123_file_info_get_id3v2_title(file);
	g_assert_cmpstr(title, ==, expected);
	fseek(file, -5, SEEK_END);
	audio_length = fread(audio, 1, 5, file);
	g_assert_cmpint(audio_length, ==, 5);
	g_assert_cmpstr(audio, ==, "audio");
	g_free(title);
	fclose(file);
	g_unlink(filename);
}

static void test_id3v1_text_limits_are_byte_based(void)
{
	const char *fifteen_characters =
		"\303\251\303\251\303\251\303\251\303\251"
		"\303\251\303\251\303\251\303\251\303\251"
		"\303\251\303\251\303\251\303\251\303\251";
	const char *sixteen_characters =
		"\303\251\303\251\303\251\303\251\303\251"
		"\303\251\303\251\303\251\303\251\303\251"
		"\303\251\303\251\303\251\303\251\303\251\303\251";

	g_assert_true(mpg123_file_info_text_fits_id3v1(
			      "123456789012345678901234567890", 30));
	g_assert_false(mpg123_file_info_text_fits_id3v1(
			       "1234567890123456789012345678901", 30));
	g_assert_true(mpg123_file_info_text_fits_id3v1(fifteen_characters, 30));
	g_assert_false(mpg123_file_info_text_fits_id3v1(sixteen_characters, 30));
}

static void test_only_escape_is_handled(void)
{
	GdkEventKey event;

	memset(&event, 0, sizeof (event));
	event.keyval = GDK_Left;
	g_assert_false(file_info_box_keypress_cb(NULL, &event, NULL));
	event.keyval = GDK_Right;
	g_assert_false(file_info_box_keypress_cb(NULL, &event, NULL));
	g_assert_true(mpg123_file_info_handles_keypress(GDK_Escape));
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/mpg123-fileinfo/long-id3v2-title",
			test_long_id3v2_title_is_loaded);
	g_test_add_func("/mpg123-fileinfo/id3v24-syncsafe-frame-sizes",
			test_id3v24_syncsafe_frame_sizes_are_loaded);
	g_test_add_func("/mpg123-fileinfo/id3v24-title-write",
			test_id3v24_title_write_preserves_audio);
	g_test_add_func("/mpg123-fileinfo/id3v1-byte-limits",
			test_id3v1_text_limits_are_byte_based);
	g_test_add_func("/mpg123-fileinfo/keypress-handling",
			test_only_escape_is_handled);

	return g_test_run();
}
