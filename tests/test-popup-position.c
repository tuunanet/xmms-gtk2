#include <gtk/gtk.h>

#include "../xmms/util.h"

static void test_popup_window_uses_requested_coordinates(void)
{
	GtkWidget *popup;
	gint x = -1;
	gint y = -1;

	popup = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_set_usize(popup, 25, 54);
	gtk_widget_realize(popup);

	util_move_popup_window(popup, 300, 250);
	gtk_widget_show(popup);
	while (gtk_events_pending())
		gtk_main_iteration();

	gdk_window_get_origin(popup->window, &x, &y);
	g_assert_cmpint(x, ==, 300);
	g_assert_cmpint(y, ==, 250);

	gtk_widget_destroy(popup);
}

static void test_popup_uses_requested_coordinates(void)
{
	GtkItemFactory *factory;
	GtkItemFactoryEntry entry =
		{ "/Test item", NULL, NULL, 0, "<Item>" };
	GtkWidget *menu;
	gint x = -1;
	gint y = -1;

	factory = gtk_item_factory_new(GTK_TYPE_MENU, "<test-popup>", NULL);
	gtk_item_factory_create_items(factory, 1, &entry, NULL);
	menu = factory->widget;

	util_item_factory_popup(factory, 300, 250, 0, GDK_CURRENT_TIME);
	while (gtk_events_pending())
		gtk_main_iteration();

	g_assert_true(GTK_WIDGET_MAPPED(menu));
	gdk_window_get_origin(menu->window, &x, &y);
	g_test_message("popup origin: %d,%d", x, y);
	g_assert_cmpint(x, ==, 298);
	g_assert_cmpint(y, ==, 248);

	gtk_menu_popdown(GTK_MENU(menu));
	g_object_unref(factory);
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/popup-position/popup-window-uses-requested-coordinates",
			test_popup_window_uses_requested_coordinates);
	g_test_add_func("/popup-position/uses-requested-coordinates",
			test_popup_uses_requested_coordinates);

	return g_test_run();
}
