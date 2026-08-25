#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>

#include "../Input/mpg123/id3_header.h"
#include "../Input/mpg123/id3v2-write.h"
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

static void write_id3v23_frame(FILE *file, const char *id, guint16 flags,
			       const guint8 *payload, size_t payload_size)
{
	fwrite(id, 1, 4, file);
	fputc((payload_size >> 24) & 0xff, file);
	fputc((payload_size >> 16) & 0xff, file);
	fputc((payload_size >> 8) & 0xff, file);
	fputc(payload_size & 0xff, file);
	fputc((flags >> 8) & 0xff, file);
	fputc(flags & 0xff, file);
	fwrite(payload, 1, payload_size, file);
}

static guint8 *read_test_file(const char *filename, size_t *length)
{
	GError *error = NULL;
	gchar *contents = NULL;

	if (!g_file_get_contents(filename, &contents, length, &error))
		g_error("Unable to read test file: %s", error->message);
	return (guint8 *) contents;
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

	g_assert_true(mpg123_id3v2_write_title(filename, expected, &found, &error));
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

static void test_id3v23_title_write_preserves_other_data(void)
{
	char filename[] = "/tmp/xmms-id3v23-XXXXXX";
	const char *expected =
		"Hertz \342\200\223 Electronic Fridays 2025.20 \303\205";
	const guint8 genre[] = { 0, 'M', 'u', 's', 'i', 'c' };
	const guint8 old_title[] = { 0, 'o', 'l', 'd', ' ', 't', 'i', 't', 'l', 'e' };
	guint8 opaque[160];
	size_t title_offset = ID3_FRAMEHDR_SIZE + sizeof (genre);
	size_t old_frame_length = ID3_FRAMEHDR_SIZE + sizeof (old_title);
	size_t tag_size = title_offset + old_frame_length +
		ID3_FRAMEHDR_SIZE + sizeof (opaque) + 10;
	size_t before_length, after_length, new_frame_length, suffix_length;
	guint32 new_frame_size;
	guint8 *before, *after;
	struct stat before_stat, after_stat;
	int fd = g_mkstemp(filename);
	int chmod_result;
	FILE *file;
	char *title, *error = NULL;
	gboolean found = FALSE;

	g_assert_cmpint(fd, !=, -1);
	chmod_result = fchmod(fd, 0640);
	g_assert_cmpint(chmod_result, ==, 0);
	file = fdopen(fd, "wb");
	g_assert_nonnull(file);
	fwrite("ID3", 1, 3, file);
	fputc(3, file);
	fputc(0, file);
	fputc(0, file);
	fputc((tag_size >> 21) & 0x7f, file);
	fputc((tag_size >> 14) & 0x7f, file);
	fputc((tag_size >> 7) & 0x7f, file);
	fputc(tag_size & 0x7f, file);
	write_id3v23_frame(file, "TCON", 0, genre, sizeof (genre));
	write_id3v23_frame(file, "TIT2", 0, old_title, sizeof (old_title));
	memset(opaque, 'A', sizeof (opaque));
	opaque[0] = ID3_ENCODING_ISO_8859_1;
	opaque[1] = 0;
	write_id3v23_frame(file, "TXXX", 0, opaque, sizeof (opaque));
	fwrite("\0\0\0\0\0\0\0\0\0\0audio", 1, 15, file);
	fclose(file);

	g_assert_cmpint(stat(filename, &before_stat), ==, 0);
	before = read_test_file(filename, &before_length);
	g_assert_true(mpg123_id3v2_write_title(filename, expected,
					       &found, &error));
	g_assert_true(found);
	g_assert_null(error);
	after = read_test_file(filename, &after_length);
	g_assert_cmpint(stat(filename, &after_stat), ==, 0);

	g_assert_cmpint(after[3], ==, 3);
	g_assert_cmpint(after[10 + title_offset + ID3_FRAMEHDR_SIZE], ==,
			ID3_ENCODING_UTF16);
	new_frame_size = (guint32) after[10 + title_offset + 4] << 24 |
			 (guint32) after[10 + title_offset + 5] << 16 |
			 (guint32) after[10 + title_offset + 6] << 8 |
			 after[10 + title_offset + 7];
	new_frame_length = ID3_FRAMEHDR_SIZE + new_frame_size;
	suffix_length = before_length - 10 - title_offset - old_frame_length;
	g_assert_cmpint(after_length, ==, before_length - old_frame_length +
			new_frame_length);
	g_assert_cmpint(memcmp(before, after, 6), ==, 0);
	g_assert_cmpint(memcmp(before + 10, after + 10, title_offset), ==, 0);
	g_assert_cmpint(memcmp(before + 10 + title_offset + old_frame_length,
			      after + 10 + title_offset + new_frame_length,
			      suffix_length), ==, 0);
	g_assert_cmpint(before_stat.st_mode & 0777, ==,
			after_stat.st_mode & 0777);

	file = fopen(filename, "rb");
	g_assert_nonnull(file);
	title = mpg123_file_info_get_id3v2_title(file);
	g_assert_cmpstr(title, ==, expected);
	g_free(title);
	fclose(file);
	g_free(after);
	g_free(before);
	g_unlink(filename);
}

static void test_id3v23_flagged_title_is_unchanged(void)
{
	char filename[] = "/tmp/xmms-id3v23-flags-XXXXXX";
	const guint8 title[] = { 0, 'o', 'l', 'd' };
	size_t tag_size = ID3_FRAMEHDR_SIZE + sizeof (title);
	size_t before_length, after_length;
	guint8 *before, *after;
	int fd = g_mkstemp(filename);
	FILE *file;
	char *error = NULL;
	gboolean found = FALSE;
	gboolean written;

	g_assert_cmpint(fd, !=, -1);
	file = fdopen(fd, "wb");
	g_assert_nonnull(file);
	fwrite("ID3", 1, 3, file);
	fputc(3, file);
	fputc(0, file);
	fputc(0, file);
	fputc((tag_size >> 21) & 0x7f, file);
	fputc((tag_size >> 14) & 0x7f, file);
	fputc((tag_size >> 7) & 0x7f, file);
	fputc(tag_size & 0x7f, file);
	write_id3v23_frame(file, "TIT2", ID3_FHFLAG_COMPRESS,
			  title, sizeof (title));
	fwrite("audio", 1, 5, file);
	fclose(file);

	before = read_test_file(filename, &before_length);
	written = mpg123_id3v2_write_title(filename, "replacement",
					   &found, &error);
	g_assert_false(written);
	g_assert_true(found);
	g_assert_nonnull(error);
	after = read_test_file(filename, &after_length);
	g_assert_cmpint(after_length, ==, before_length);
	g_assert_cmpint(memcmp(before, after, before_length), ==, 0);

	g_free(after);
	g_free(before);
	g_free(error);
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
	setlocale(LC_ALL, "");
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/mpg123-fileinfo/long-id3v2-title",
			test_long_id3v2_title_is_loaded);
	g_test_add_func("/mpg123-fileinfo/id3v24-syncsafe-frame-sizes",
			test_id3v24_syncsafe_frame_sizes_are_loaded);
	g_test_add_func("/mpg123-fileinfo/id3v24-title-write",
			test_id3v24_title_write_preserves_audio);
	g_test_add_func("/mpg123-fileinfo/id3v23-title-write",
			test_id3v23_title_write_preserves_other_data);
	g_test_add_func("/mpg123-fileinfo/id3v23-flagged-title",
			test_id3v23_flagged_title_is_unchanged);
	g_test_add_func("/mpg123-fileinfo/id3v1-byte-limits",
			test_id3v1_text_limits_are_byte_based);
	g_test_add_func("/mpg123-fileinfo/keypress-handling",
			test_only_escape_is_handled);

	return g_test_run();
}
