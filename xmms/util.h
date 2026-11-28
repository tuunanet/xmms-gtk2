/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2991  Haavard Kvaalen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef UTIL_H
#define UTIL_H

#include "i18n.h"

#define XMMS_LEGACY_PLAYLIST_FONT "-adobe-helvetica-bold-r-*-*-10-*"
#define XMMS_LEGACY_MAINWIN_FONT "-adobe-helvetica-medium-r-*-*-8-*"
#define XMMS_INTERIM_PLAYLIST_FONT \
	"-misc-fixed-bold-r-normal--10-*-*-*-*-*-iso10646-1"
#define XMMS_INTERIM_MAINWIN_FONT \
	"-misc-fixed-medium-r-normal--8-*-*-*-*-*-iso10646-1"
#define XMMS_INTERIM_FIXED_PLAYLIST_FONT \
	"-misc-fixed-bold-r-normal--10-*-*-*-*-*-iso8859-1"
#define XMMS_INTERIM_FIXED_MAINWIN_FONT \
	"-misc-fixed-medium-r-normal--8-*-*-*-*-*-iso8859-1"
#define XMMS_INTERIM_PROPORTIONAL_PLAYLIST_FONT \
	"-adobe-helvetica-bold-r-normal--10-*-*-*-p-*-iso8859-1"
#define XMMS_INTERIM_PROPORTIONAL_MAINWIN_FONT \
	"-adobe-helvetica-medium-r-normal--8-*-*-*-p-*-iso8859-1"
#define XMMS_INTERIM_CLASSIC_FONT "fixed"
#define XMMS_INTERIM_SEMICONDENSED_PLAYLIST_FONT \
	"-misc-fixed-bold-r-semicondensed--13-120-75-75-c-60-iso8859-1"
/* Shipped briefly as the default playlist font scale, paired with the
 * 13px bold interim font above; migrated to 100 with the native 6x12
 * font so existing configs land on native-size rendering. */
#define XMMS_INTERIM_PLAYLIST_FONT_SCALE 90
#define XMMS_DEFAULT_PLAYLIST_FONT \
	"-misc-fixed-medium-r-semicondensed--12-110-75-75-c-60-iso8859-1"
#define XMMS_DEFAULT_MAINWIN_FONT \
	"-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1"

/* Playlist editor text scale, in percent of the configured font's size.
 * The default playlist font (6x12) is natively available, so the
 * default scale is 100 and renders every pixel from the bitmap font
 * itself. Other values rewrite the XLFD size fields and let the X
 * server bitmap-scale; 90 percent is one visible step smaller (12px
 * -> 11px) and still clean, while some other values can look soft. */
#define XMMS_MIN_PLAYLIST_FONT_SCALE 25
#define XMMS_DEFAULT_PLAYLIST_FONT_SCALE 100
#define XMMS_MAX_PLAYLIST_FONT_SCALE 400

/* Desktop DPI detection for HiDPI startup scaling. */
#define XMMS_DEFAULT_DPI 96
#define XMMS_MAX_SANE_DPI 4096
/* 1.5x or more of the default 96 DPI counts as HiDPI and starts the
 * skin doubled (covers GNOME fractional scales of 150% and up). */
#define XMMS_HIDPI_DOUBLE_DPI 144
/* Display-resolution baseline for the automatic skin scale: QHD
 * (2560x1440) keeps the classic 1x skinned UI, larger displays scale
 * up proportionally. The skinned renderer supports integer scales up
 * to XMMS_SKIN_MAX_SCALE (1x..3x), so the coefficient saturates. */
#define XMMS_SKIN_BASE_WIDTH 2560
#define XMMS_SKIN_BASE_HEIGHT 1440
#define XMMS_SKIN_MAX_SCALE 3
/* First-run default for the skin_scale coefficient: 1.5 rounds up to
 * 2x, so a freshly created ~/.xmms/config starts the skinned UI in
 * DoubleSize on every display. */
#define XMMS_DEFAULT_SKIN_SCALE 1.5

gchar *find_file_recursively(const char *dirname, const char *file);
void del_directory(const char *dirname);
char *read_ini_string(const char *filename, const char *section, const char *key);
char *read_ini_string_no_comment(const char *filename, const char *section, const char *key);
GArray *read_ini_array(const gchar * filename, const gchar * section, const gchar * key);
GArray *string_to_garray(const gchar * str);
void glist_movedown(GList * list);
void glist_moveup(GList * list);
void util_move_popup_window(GtkWidget *window, gint x, gint y);
void util_item_factory_popup(GtkItemFactory * ifactory, guint x, guint y, guint mouse_button, guint32 time);
void util_item_factory_popup_with_data(GtkItemFactory * ifactory, gpointer data, GtkDestroyNotify destroy, guint x, guint y, guint mouse_button, guint32 time);
GtkWidget *util_create_add_url_window(gchar *caption, GtkSignalFunc ok_func, GtkSignalFunc enqueue_func);
GtkWidget *util_create_filebrowser(gboolean clear_pl_on_ok);
gboolean util_filebrowser_is_dir(GtkFileSelection * filesel);
GdkFont *util_font_load(gchar *name);
gchar *util_font_scale_name(const gchar *name, gint scale);
void util_set_cursor(GtkWidget *window);
void util_dump_menu_rc(void);
void util_read_menu_rc(void);
void util_dialog_keypress_cb(GtkWidget *w, GdkEventKey *event, gpointer data);
gint util_xft_dpi_parse(const gchar *value);
gint util_xft_dpi_from_rdb(const gchar *rdb);
gint util_get_xft_dpi(void);
gint util_skin_scale_for_dpi(gint dpi);
gdouble util_skin_coefficient_for_resolution(gint width, gint height);
gint util_skin_scale_for_coefficient(gdouble coefficient);
gint util_skin_scale_for_display(gint width, gint height, gint dpi);
GdkImage *create_scaled_image(GdkImage * img, gint scale);
void util_expand_indexed_frame(const guchar *frame, gint width, gint height,
			       guchar *dest, gint scale);

#if ENABLE_NLS
    gchar* util_menu_translate(const gchar *path, gpointer func_data);
#else
#   define util_menu_translate NULL
#endif



#endif
