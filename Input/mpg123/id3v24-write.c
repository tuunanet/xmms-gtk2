#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <glib/gstdio.h>
#include "id3_header.h"
#include "id3v24-write.h"

#define ID3V24_HEADER_SIZE 10
#define ID3V24_FOOTER_SIZE 10
#define ID3V24_FOOTER_FLAG 0x10

static guint32 id3v24_get_size(const guint8 *bytes)
{
	return ID3_GET_SIZE28(bytes[0], bytes[1], bytes[2], bytes[3]);
}

static void id3v24_set_size(guint8 *bytes, guint32 size)
{
	ID3_SET_SIZE28(size, bytes[0], bytes[1], bytes[2], bytes[3]);
}

static gboolean id3v24_valid_size(const guint8 *bytes)
{
	return !(bytes[0] & 0x80) && !(bytes[1] & 0x80) &&
	       !(bytes[2] & 0x80) && !(bytes[3] & 0x80);
}

static gboolean id3v24_read_all(int fd, void *buffer, size_t length)
{
	guint8 *bytes = buffer;
	ssize_t count;

	while (length)
	{
		count = read(fd, bytes, length);
		if (count <= 0)
			return FALSE;
		bytes += count;
		length -= count;
	}
	return TRUE;
}

static gboolean id3v24_write_all(int fd, const void *buffer, size_t length)
{
	const guint8 *bytes = buffer;
	ssize_t count;

	while (length)
	{
		count = write(fd, bytes, length);
		if (count <= 0)
			return FALSE;
		bytes += count;
		length -= count;
	}
	return TRUE;
}

static gboolean id3v24_copy_all(int input, int output)
{
	guint8 buffer[8192];
	ssize_t count;

	while ((count = read(input, buffer, sizeof (buffer))) > 0)
		if (!id3v24_write_all(output, buffer, count))
			return FALSE;
	return count == 0;
}

static gboolean id3v24_frame_id_is_valid(const guint8 *id)
{
	int i;

	for (i = 0; i < 4; i++)
		if (!g_ascii_isupper(id[i]) && !g_ascii_isdigit(id[i]))
			return FALSE;
	return TRUE;
}

static gboolean id3v24_find_title(const guint8 *data, size_t size,
				  size_t *offset, size_t *length)
{
	size_t pos = 0;
	guint32 frame_size;

	while (pos + ID3_FRAMEHDR_SIZE <= size && data[pos])
	{
		if (!id3v24_frame_id_is_valid(data + pos) ||
		    !id3v24_valid_size(data + pos + 4))
			return FALSE;
		frame_size = id3v24_get_size(data + pos + 4);
		if (frame_size > size - pos - ID3_FRAMEHDR_SIZE)
			return FALSE;
		if (!memcmp(data + pos, "TIT2", 4))
		{
			*offset = pos;
			*length = ID3_FRAMEHDR_SIZE + frame_size;
			return TRUE;
		}
		pos += ID3_FRAMEHDR_SIZE + frame_size;
	}
	if (pos + ID3_FRAMEHDR_SIZE > size && pos != size)
		return FALSE;
	*offset = pos;
	*length = 0;
	return TRUE;
}

static guint8 *id3v24_title_frame(const char *title, size_t *length)
{
	size_t text_length = strlen(title);
	guint8 *frame;

	if (!g_utf8_validate(title, -1, NULL) || text_length > 0x0ffffffe)
		return NULL;
	*length = ID3_FRAMEHDR_SIZE + text_length + 2;
	frame = g_malloc0(*length);
	memcpy(frame, "TIT2", 4);
	id3v24_set_size(frame + 4, text_length + 2);
	frame[ID3_FRAMEHDR_SIZE] = 3;
	memcpy(frame + ID3_FRAMEHDR_SIZE + 1, title, text_length);
	return frame;
}

gboolean mpg123_id3v24_write_title(const char *filename, const char *title,
				  gboolean *found, char **error)
{
	struct stat st;
	guint8 header[ID3V24_HEADER_SIZE], *data = NULL, *new_data = NULL;
	guint8 footer[ID3V24_FOOTER_SIZE], *frame = NULL;
	guint32 data_size, new_data_size;
	size_t title_offset, old_title_length, frame_length;
	int input = -1, output = -1;
	char *temporary = NULL;
	gboolean footer_present;
	gboolean ok = FALSE;

	*found = FALSE;
	input = open(filename, O_RDONLY);
	if (input == -1 || fstat(input, &st) == -1 ||
	    !id3v24_read_all(input, header, sizeof (header)))
	{
		*error = g_strdup_printf("Unable to read file: %s", strerror(errno));
		goto done;
	}
	if (memcmp(header, "ID3", 3))
	{
		ok = TRUE;
		goto done;
	}
	*found = TRUE;
	if (header[3] != 4 || header[4] != 0 || (header[5] & 0xe0) ||
	    !id3v24_valid_size(header + 6))
	{
		*error = g_strdup("Unsupported ID3v2 tag; the file was not changed.");
		goto done;
	}
	footer_present = header[5] & ID3V24_FOOTER_FLAG;
	data_size = id3v24_get_size(header + 6);
	if (st.st_size < ID3V24_HEADER_SIZE +
	    (footer_present ? ID3V24_FOOTER_SIZE : 0) ||
	    data_size > (guint64) (st.st_size - ID3V24_HEADER_SIZE -
	    (footer_present ? ID3V24_FOOTER_SIZE : 0)))
	{
		*error = g_strdup("Invalid ID3v2 tag; the file was not changed.");
		goto done;
	}
	data = g_malloc(data_size);
	if (!id3v24_read_all(input, data, data_size) ||
	    (footer_present && !id3v24_read_all(input, footer, sizeof (footer))) ||
	    !id3v24_find_title(data, data_size, &title_offset, &old_title_length))
	{
		*error = g_strdup("Invalid ID3v2 frame layout; the file was not changed.");
		goto done;
	}
	frame = id3v24_title_frame(title, &frame_length);
	if (!frame || frame_length > G_MAXUINT32 - data_size + old_title_length)
	{
		*error = g_strdup("Invalid title; the file was not changed.");
		goto done;
	}
	new_data_size = data_size - old_title_length + frame_length;
	new_data = g_malloc(new_data_size);
	memcpy(new_data, data, title_offset);
	memcpy(new_data + title_offset, frame, frame_length);
	memcpy(new_data + title_offset + frame_length,
	       data + title_offset + old_title_length,
	       data_size - title_offset - old_title_length);
	id3v24_set_size(header + 6, new_data_size);
	if (footer_present)
		id3v24_set_size(footer + 6, new_data_size);
	temporary = g_strdup_printf("%s.XXXXXX", filename);
	output = g_mkstemp(temporary);
	if (output == -1 || fchmod(output, st.st_mode & 0777) == -1 ||
	    !id3v24_write_all(output, header, sizeof (header)) ||
	    !id3v24_write_all(output, new_data, new_data_size) ||
	    (footer_present && !id3v24_write_all(output, footer, sizeof (footer))) ||
	    !id3v24_copy_all(input, output) || fsync(output) == -1 ||
	    close(output) == -1 || g_rename(temporary, filename) == -1)
	{
		*error = g_strdup_printf("Unable to write ID3v2 tag: %s", strerror(errno));
		goto done;
	}
	output = -1;
	ok = TRUE;

done:
	if (input != -1)
		close(input);
	if (output != -1)
		close(output);
	if (!ok && temporary)
		g_unlink(temporary);
	g_free(temporary);
	g_free(frame);
	g_free(new_data);
	g_free(data);
	return ok;
}
