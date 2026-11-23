/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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
#include "xmms.h"

static const gfloat vis_afalloff_speeds[] = {0.34, 0.5, 1.0, 1.3, 1.6};
static const gfloat vis_pfalloff_speeds[] = {1.2, 1.3, 1.4, 1.5, 1.6};
static const gint vis_redraw_delays[] = {1, 2, 4, 8};
static const guint8 vis_scope_colors[] =
{21, 21, 20, 20, 19, 19, 18, 19, 19, 20, 20, 21, 21};

void vis_timeout_func(Vis * vis, guchar * data)
{
	static GTimer *timer = NULL;
	gulong micros = 9999999;
	gboolean falloff = FALSE;
	gint i;

	if (!timer)
	{
		timer = g_timer_new();
		g_timer_start(timer);
	}
	else
	{
		g_timer_elapsed(timer, &micros);
		if (micros > 14000)
			g_timer_reset(timer);

	}

	if (cfg.vis_type == VIS_ANALYZER)
	{
		if (micros > 14000)
			falloff = TRUE;
		if (data || falloff)
		{
			for (i = 0; i < 75; i++)
			{
				if (data && data[i] > vis->vs_data[i])
				{
					vis->vs_data[i] = data[i];
					if (vis->vs_data[i] > vis->vs_peak[i])
					{
						vis->vs_peak[i] = vis->vs_data[i];
						vis->vs_peak_speed[i] = 0.01;

					}
					else if (vis->vs_peak[i] > 0.0)
					{
						vis->vs_peak[i] -= vis->vs_peak_speed[i];
						vis->vs_peak_speed[i] *= vis_pfalloff_speeds[cfg.peaks_falloff];
						if (vis->vs_peak[i] < vis->vs_data[i])
							vis->vs_peak[i] = vis->vs_data[i];
						if (vis->vs_peak[i] < 0.0)
							vis->vs_peak[i] = 0.0;
					}
				}
				else if (falloff)
				{
					if (vis->vs_data[i] > 0.0)
					{
						vis->vs_data[i] -= vis_afalloff_speeds[cfg.analyzer_falloff];
						if (vis->vs_data[i] < 0.0)
							vis->vs_data[i] = 0.0;
					}
					if (vis->vs_peak[i] > 0.0)
					{
						vis->vs_peak[i] -= vis->vs_peak_speed[i];
						vis->vs_peak_speed[i] *= vis_pfalloff_speeds[cfg.peaks_falloff];
						if (vis->vs_peak[i] < vis->vs_data[i])
							vis->vs_peak[i] = vis->vs_data[i];
						if (vis->vs_peak[i] < 0.0)
							vis->vs_peak[i] = 0.0;
					}
				}
			}
		}
	}
	else if (data)
	{
		for (i = 0; i < 75; i++)
			vis->vs_data[i] = data[i];
	}

	if (micros > 14000)
	{
		if (!vis->vs_refresh_delay)
		{
			vis_draw((Widget *) vis);
			vis->vs_refresh_delay = vis_redraw_delays[cfg.vis_refresh];

		}
		vis->vs_refresh_delay--;
	}
}

/* Render the skin-space analyzer image at the widget's scale. */
static void vis_draw_scaled(Vis *vis, const guchar *rgb_data,
			    const guint32 *colors)
{
	gint sw = vis->vs_widget.width;
	gint sh = vis->vs_widget.height;
	gint dw = sw * vis->vs_scale_num / vis->vs_scale_den;
	gint dh = sh * vis->vs_scale_num / vis->vs_scale_den;
	GdkImage *img, *scaled;
	gint x, y;

	img = gdk_image_new(GDK_IMAGE_NORMAL, gdk_visual_get_best(), sw, sh);
	if (!img)
		return;
	for (y = 0; y < sh; y++)
		for (x = 0; x < sw; x++)
			gdk_image_put_pixel(img, x, y,
					    gdk_rgb_xpixel_from_rgb(
						    colors[rgb_data[y * sw + x] % 24]));
	scaled = scale_image(img, dw, dh);
	gdk_image_destroy(img);
	if (!scaled)
		return;
	gdk_draw_image(vis->vs_window, vis->vs_widget.gc, scaled,
		       vis->vs_widget.x * vis->vs_scale_num / vis->vs_scale_den,
		       vis->vs_widget.y * vis->vs_scale_num / vis->vs_scale_den,
		       0, 0, dw, dh);
	gdk_image_destroy(scaled);
}

void vis_draw(Widget * w)
{
	Vis *vis = (Vis *) w;
	gint x, y, h = 0, h2;
	guchar vis_color[24][3];
	guchar rgb_data[76 * 16], *ptr, c;
	guint32 colors[24];

	if (!vis->vs_widget.visible)
		return;

	GDK_THREADS_ENTER();

	get_skin_viscolor(vis_color);
	for (y = 0; y < 24; y++)
	{
		colors[y] = vis_color[y][0] << 16 | vis_color[y][1] << 8 | vis_color[y][2];
	}

	memset(rgb_data, 0, 76 * 16);
	for (y = 1; y < 16; y += 2)
	{
		ptr = rgb_data + (y * 76);
		for (x = 0; x < 76; x += 2, ptr += 2)
			*ptr = 1;
	}
	if (cfg.vis_type == VIS_ANALYZER)
	{
		for (x = 0; x < 75; x++)
		{
			if (cfg.analyzer_type == ANALYZER_BARS && (x % 4) == 0)
				h = vis->vs_data[x >> 2];
			else if (cfg.analyzer_type == ANALYZER_LINES)
				h = vis->vs_data[x];
			if (h && (cfg.analyzer_type == ANALYZER_LINES ||
				  (x % 4) != 3))
			{
				ptr = rgb_data + ((16 - h) * 76) + x;
				switch (cfg.analyzer_mode)
				{
					case ANALYZER_NORMAL:
						for (y = 0; y < h; y++, ptr += 76)
							*ptr = 18 - h + y;
						break;
					case ANALYZER_FIRE:
						for (y = 0; y < h; y++, ptr += 76)
							*ptr = y + 2;
						break;
					case ANALYZER_VLINES:
						for (y = 0; y < h; y++, ptr += 76)
							*ptr = 18 - h;
						break;
				}
			}
		}
		if (cfg.analyzer_peaks)
		{
			for (x = 0; x < 75; x++)
			{
				if (cfg.analyzer_type == ANALYZER_BARS && (x % 4) == 0)
					h = vis->vs_peak[x >> 2];
				else if(cfg.analyzer_type == ANALYZER_LINES)
					h = vis->vs_peak[x];
				if (h && (cfg.analyzer_type == ANALYZER_LINES || (x % 4) != 3))
					rgb_data[(16 - h) * 76 + x] = 23;
			}
		}
	}
	else if (cfg.vis_type == VIS_SCOPE)
	{
		for (x = 0; x < 75; x++)
		{
			switch (cfg.scope_mode)
			{
				case SCOPE_DOT:
					h = vis->vs_data[x];
					ptr = rgb_data + ((15 - h) * 76) + x;
					*ptr = vis_scope_colors[h];
					break;
				case SCOPE_LINE:
					if (x != 74)
					{
						h = 15 - vis->vs_data[x];
						h2 = 15 - vis->vs_data[x + 1];
						if (h > h2)
						{
							y = h;
							h = h2;
							h2 = y;
						}
						ptr = rgb_data + (h * 76) + x;
						for (y = h; y <= h2; y++, ptr += 76)
							*ptr = vis_scope_colors[y - 3];

					}
					else
					{
						h = 15 - vis->vs_data[x];
						ptr = rgb_data + (h * 76) + x;
						*ptr = vis_scope_colors[h];
					}
					break;
				case SCOPE_SOLID:
					h = 15 - vis->vs_data[x];
					h2 = 9;
					c = vis_scope_colors[(gint) vis->vs_data[x]];
					if (h > h2)
					{
						y = h;
						h = h2;
						h2 = y;
					}
					ptr = rgb_data + (h * 76) + x;
					for (y = h; y <= h2; y++, ptr += 76)
						*ptr = c;
					break;
			}
		}
	}

	vis_draw_scaled(vis, rgb_data, colors);
	GDK_THREADS_LEAVE();

}

void vis_clear_data(Vis * vis)
{
	gint i;

	for (i = 0; i < 75; i++)
	{
		vis->vs_data[i] = (cfg.vis_type == VIS_SCOPE) ? 6 : 0;
		vis->vs_peak[i] = 0;
	}
}

void vis_set_scale(Vis * vis, gint scale_num, gint scale_den)
{
	vis->vs_scale_num = scale_num;
	vis->vs_scale_den = scale_den;
}

void vis_clear(Vis * vis)
{
	gint x = vis->vs_widget.x * vis->vs_scale_num / vis->vs_scale_den;
	gint y = vis->vs_widget.y * vis->vs_scale_num / vis->vs_scale_den;
	gint w = vis->vs_widget.width * vis->vs_scale_num / vis->vs_scale_den;
	gint h = vis->vs_widget.height * vis->vs_scale_num / vis->vs_scale_den;

	gdk_window_clear_area(vis->vs_window, x, y, w + 1, h + 1);
}

void vis_set_window(Vis * vis, GdkWindow * window)
{
	vis->vs_window = window;
}

Vis *create_vis(GList ** wlist, GdkPixmap * parent, GdkWindow * window, GdkGC * gc, gint x, gint y, gint width, gint scale_num, gint scale_den)
{
	Vis *vis;

	vis = (Vis *) g_malloc0(sizeof (Vis));
	vis->vs_widget.parent = parent;
	vis->vs_window = window;
	vis->vs_widget.gc = gc;
	vis->vs_widget.x = x;
	vis->vs_widget.y = y;
	vis->vs_widget.width = width;
	vis->vs_widget.height = 16;
	vis->vs_widget.visible = 1;
	vis->vs_scale_num = scale_num;
	vis->vs_scale_den = scale_den;
	add_widget(wlist, vis);
	return vis;
}
