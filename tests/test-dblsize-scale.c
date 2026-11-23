#include <glib.h>
#include <gtk/gtk.h>

#include "../xmms/xmms.h"
#include "../xmms/util.h"

Config cfg;

static GdkImage *make_image(gint width, gint height)
{
	return gdk_image_new(GDK_IMAGE_NORMAL, gdk_visual_get_best(),
			     width, height);
}

static guint32 rgb_pixel(gint r, gint g, gint b)
{
	return gdk_rgb_xpixel_from_rgb((r << 16) | (g << 8) | b);
}

static gint pixel_r(guint32 p)
{
	return (p >> 16) & 0xff;
}

static gint pixel_g(guint32 p)
{
	return (p >> 8) & 0xff;
}

static gint pixel_b(guint32 p)
{
	return p & 0xff;
}

static void skip_unless_rgb24(GdkImage *img)
{
	if (img->bpp != 3 && img->bpp != 4)
		g_test_skip("display is not 24/32-bit TrueColor");
}

static void test_uniform_colour_survives(void)
{
	GdkImage *src, *dst;
	guint32 colour = rgb_pixel(0x12, 0x34, 0x56);
	gint x, y;

	src = make_image(4, 4);
	skip_unless_rgb24(src);
	for (y = 0; y < 4; y++)
		for (x = 0; x < 4; x++)
			gdk_image_put_pixel(src, x, y, colour);

	dst = create_dblsize_image(src);
	g_assert_nonnull(dst);
	g_assert_cmpint(dst->width, ==, 8);
	g_assert_cmpint(dst->height, ==, 8);
	for (y = 0; y < 8; y++)
		for (x = 0; x < 8; x++)
			g_assert_cmpuint(gdk_image_get_pixel(dst, x, y), ==,
					 colour);
	gdk_image_destroy(src);
	gdk_image_destroy(dst);
}

static void test_edge_is_blended_not_duplicated(void)
{
	GdkImage *src, *dst;
	guint32 black = rgb_pixel(0, 0, 0);
	guint32 white = rgb_pixel(255, 255, 255);
	guint32 left, right;
	gint x, y;

	src = make_image(8, 8);
	skip_unless_rgb24(src);
	for (y = 0; y < 8; y++)
		for (x = 0; x < 8; x++)
			gdk_image_put_pixel(src, x, y,
					    (x < 4) ? black : white);

	dst = create_dblsize_image(src);
	g_assert_nonnull(dst);

	/* Destination column 7 maps to source column 3.75:
	 * 3/4 * source[3] + 1/4 * source[4] = 0.25 * white = 63.
	 * Destination column 8 maps to source column 3.75+0.25:
	 * 1/4 * source[3] + 3/4 * source[4] = 191. */
	left = gdk_image_get_pixel(dst, 7, 4);
	right = gdk_image_get_pixel(dst, 8, 4);
	g_assert_cmpint(pixel_r(left), ==, 63);
	g_assert_cmpint(pixel_g(left), ==, 63);
	g_assert_cmpint(pixel_b(left), ==, 63);
	g_assert_cmpint(pixel_r(right), ==, 191);
	g_assert_cmpint(pixel_g(right), ==, 191);
	g_assert_cmpint(pixel_b(right), ==, 191);

	gdk_image_destroy(src);
	gdk_image_destroy(dst);
}

static void test_corners_are_clamped(void)
{
	GdkImage *src, *dst;
	guint32 corner = rgb_pixel(0xaa, 0xbb, 0xcc);
	guint32 other = rgb_pixel(0x11, 0x22, 0x33);
	gint x, y;

	src = make_image(4, 4);
	skip_unless_rgb24(src);
	for (y = 0; y < 4; y++)
		for (x = 0; x < 4; x++)
			gdk_image_put_pixel(src, x, y,
					    (x == 0 && y == 0) ? corner : other);

	dst = create_dblsize_image(src);
	g_assert_nonnull(dst);
	g_assert_cmpuint(gdk_image_get_pixel(dst, 0, 0), ==, corner);
	gdk_image_destroy(src);
	gdk_image_destroy(dst);
}

static void test_no_seams_for_solid_strips(void)
{
	GdkImage *src, *dst;
	guint32 black = rgb_pixel(0, 0, 0);
	guint32 white = rgb_pixel(255, 255, 255);
	gint x;

	/* A 2x2 checkerboard exercises interpolation in both axes. */
	src = make_image(2, 2);
	skip_unless_rgb24(src);
	gdk_image_put_pixel(src, 0, 0, black);
	gdk_image_put_pixel(src, 1, 0, white);
	gdk_image_put_pixel(src, 0, 1, white);
	gdk_image_put_pixel(src, 1, 1, black);

	dst = create_dblsize_image(src);
	g_assert_nonnull(dst);
	/* The four sources stay at the corners of the doubled image. */
	g_assert_cmpuint(gdk_image_get_pixel(dst, 0, 0), ==, black);
	g_assert_cmpint(pixel_r(gdk_image_get_pixel(dst, 3, 3)), ==, 0);
	/* Interior pixels are combinations of both colours. */
	for (x = 1; x < 3; x++)
	{
		gint r = pixel_r(gdk_image_get_pixel(dst, x, 1));
		g_assert_cmpint(r, >, 0);
		g_assert_cmpint(r, <, 255);
	}
	gdk_image_destroy(src);
	gdk_image_destroy(dst);
}

static void test_three_halves_scale(void)
{
	GdkImage *src, *dst;
	guint32 black = rgb_pixel(0, 0, 0);
	guint32 white = rgb_pixel(255, 255, 255);
	guint32 corner = rgb_pixel(0xaa, 0xbb, 0xcc);
	gint x, y;

	src = make_image(4, 4);
	skip_unless_rgb24(src);
	for (y = 0; y < 4; y++)
		for (x = 0; x < 4; x++)
			gdk_image_put_pixel(src, x, y,
					    (x < 2) ? black : white);
	gdk_image_put_pixel(src, 0, 0, corner);

	dst = scale_image(src, 6, 6);
	g_assert_nonnull(dst);
	g_assert_cmpint(dst->width, ==, 6);
	g_assert_cmpint(dst->height, ==, 6);
	/* Clamped corner is preserved exactly. */
	g_assert_cmpuint(gdk_image_get_pixel(dst, 0, 0), ==, corner);
	/* The black/white boundary lands between destination columns 2
	 * and 3 and must be blended, never a hard duplicate. */
	g_assert_cmpint(pixel_r(gdk_image_get_pixel(dst, 2, 2)), >, 0);
	g_assert_cmpint(pixel_r(gdk_image_get_pixel(dst, 2, 2)), <, 255);
	g_assert_cmpint(pixel_r(gdk_image_get_pixel(dst, 3, 2)), >, 0);
	g_assert_cmpint(pixel_r(gdk_image_get_pixel(dst, 3, 2)), <, 255);
	g_assert_cmpint(pixel_r(gdk_image_get_pixel(dst, 3, 2)), >,
			pixel_r(gdk_image_get_pixel(dst, 2, 2)));
	/* Far flat areas stay exact. */
	g_assert_cmpuint(gdk_image_get_pixel(dst, 5, 5), ==, white);
	gdk_image_destroy(src);
	gdk_image_destroy(dst);
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);
	gdk_rgb_init();
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/dblsize/uniform-colour-survives",
			test_uniform_colour_survives);
	g_test_add_func("/dblsize/edge-is-blended-not-duplicated",
			test_edge_is_blended_not_duplicated);
	g_test_add_func("/dblsize/corners-are-clamped",
			test_corners_are_clamped);
	g_test_add_func("/dblsize/no-seams-for-checkerboard",
			test_no_seams_for_solid_strips);
	g_test_add_func("/dblsize/three-halves-scale",
			test_three_halves_scale);
	return g_test_run();
}
