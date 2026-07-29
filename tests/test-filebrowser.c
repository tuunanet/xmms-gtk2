#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <locale.h>

#include "../xmms/xmms.h"

Config cfg;
static GList *added_files;

void playlist_ins(gchar *filename, glong pos)
{
	added_files = g_list_append(added_files, g_strdup(filename));
}

void playlistwin_update_list(void)
{
}

GList *input_scan_dir(gchar *directory)
{
	return NULL;
}

void playlist_clear(void)
{
}

void playlist_play(void)
{
}

static void process_events(void)
{
	while (gtk_events_pending())
		gtk_main_iteration();
}

static GtkWidget *find_button(GtkWidget *widget, const gchar *label)
{
	GList *children, *node;
	GtkWidget *button = NULL;
	const gchar *button_label;

	if (GTK_IS_BUTTON(widget))
	{
		button_label = gtk_button_get_label(GTK_BUTTON(widget));
		if (button_label && !strcmp(button_label, label))
			return widget;
	}
	if (!GTK_IS_CONTAINER(widget))
		return NULL;

	children = gtk_container_get_children(GTK_CONTAINER(widget));
	for (node = children; node != NULL; node = g_list_next(node))
		if ((button = find_button(node->data, label)) != NULL)
			break;
	g_list_free(children);

	return button;
}

static void test_add_selected_file(void)
{
	gchar directory[] = "/tmp/xmms-filebrowser-XXXXXX";
	gchar *file;
	GtkFileSelection *filesel;
	GtkTreeSelection *selection;
	GtkWidget *add_selected;

	g_assert_nonnull(g_mkdtemp(directory));
	file = g_build_filename(directory, "Electronic Fridays.mp3", NULL);
	g_assert_true(g_file_set_contents(file, "", 0, NULL));

	filesel = GTK_FILE_SELECTION(util_create_filebrowser(FALSE));
	{
		gchar *directory_with_slash = g_strconcat(directory, "/", NULL);
		gtk_file_selection_set_filename(filesel, directory_with_slash);
		g_free(directory_with_slash);
	}
	gtk_widget_show(GTK_WIDGET(filesel));
	process_events();

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(filesel->file_list));
	gtk_tree_selection_select_all(selection);
	add_selected = find_button(GTK_WIDGET(filesel), "Add selected files");
	g_assert_nonnull(add_selected);
	gtk_button_clicked(GTK_BUTTON(add_selected));

	g_assert_cmpuint(g_list_length(added_files), ==, 1);
	g_assert_cmpstr(added_files->data, ==, file);

	g_list_free_full(added_files, g_free);
	added_files = NULL;
	g_free(cfg.filesel_path);
	cfg.filesel_path = NULL;
	gtk_widget_destroy(GTK_WIDGET(filesel));
	g_remove(file);
	g_rmdir(directory);
	g_free(file);
}

static void test_add_all_files_in_directory(void)
{
	gchar directory[] = "/tmp/xmms-filebrowser-XXXXXX";
	gchar *first_file, *second_file;
	GtkFileSelection *filesel;
	GtkWidget *add_all;

	g_assert_nonnull(g_mkdtemp(directory));
	first_file = g_build_filename(directory, "first.mp3", NULL);
	second_file = g_build_filename(directory, "second.ogg", NULL);
	g_assert_true(g_file_set_contents(first_file, "", 0, NULL));
	g_assert_true(g_file_set_contents(second_file, "", 0, NULL));

	filesel = GTK_FILE_SELECTION(util_create_filebrowser(FALSE));
	{
		gchar *directory_with_slash = g_strconcat(directory, "/", NULL);
		gtk_file_selection_set_filename(filesel, directory_with_slash);
		g_free(directory_with_slash);
	}
	gtk_widget_show(GTK_WIDGET(filesel));
	process_events();

	add_all = find_button(GTK_WIDGET(filesel),
			      "Add all files in directory");
	g_assert_nonnull(add_all);
	gtk_button_clicked(GTK_BUTTON(add_all));

	g_assert_cmpuint(g_list_length(added_files), ==, 2);
	g_assert_cmpstr(added_files->data, ==, first_file);
	g_assert_cmpstr(added_files->next->data, ==, second_file);

	g_list_free_full(added_files, g_free);
	added_files = NULL;
	g_free(cfg.filesel_path);
	cfg.filesel_path = NULL;
	gtk_widget_destroy(GTK_WIDGET(filesel));
	g_remove(first_file);
	g_remove(second_file);
	g_rmdir(directory);
	g_free(first_file);
	g_free(second_file);
}

int main(int argc, char **argv)
{
	setlocale(LC_ALL, "C");
	gtk_init(&argc, &argv);
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/filebrowser/add-selected-file",
			test_add_selected_file);
	g_test_add_func("/filebrowser/add-all-files-in-directory",
			test_add_all_files_in_directory);

	return g_test_run();
}
