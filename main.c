#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <stddef.h>

#define BUILDER_FILE "window_main.glade"
#define MAIN_WINDOW "erasmo_main_window"
#define FIXED_WINDOW "fixed1"
#define ERASE_BUTTON "erase_button"
#define REFRESH_BUTTON "refresh_button"
#define CANCEL_BUTTON "cancel_button"
#define DEVICE_TREEVIEW "device_treeview"

enum {
	STORAGE_DEVICE_NAME,
	STORAGE_DEVICE_VENDOR,
	STORAGE_DEVICE_MODEL,
	STORAGE_DEVICE_CAPACITY_TEXT,
	STORAGE_DEVICE_SERIAL,
	STORAGE_DEVICE_BUS,

	ERASING_STORAGE_DEVICE_STATE,
	ERASING_STORAGE_DEVICE_STATE_TEXT,
	ERASING_STORAGE_DEVICE_PROGRESS,
	ERASING_STORAGE_DEVICE_PROGRESS_TEXT,

	STORAGE_DEVICE_N_COLUMNS
};

GtkWidget *window;
GtkBuilder *builder;
GtkTreeView *erasing_tree_view;

static GtkTreeModel *create_and_fill_model(void);
void refresh_tree_view();

int main(int argc, char *argv[])
{

    GtkTreeModel *model;

    // window
    gtk_init(&argc, &argv);
    builder = gtk_builder_new_from_file(BUILDER_FILE);

    //treeview
    refresh_tree_view();
    model = create_and_fill_model();
    gtk_tree_view_set_model(erasing_tree_view, model);
    g_object_unref(model);

    window = GTK_WIDGET(gtk_builder_get_object(builder, MAIN_WINDOW));
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_builder_connect_signals(builder, NULL);

    gtk_widget_show(window);

    gtk_main();

    // hw
    // libhw_init();

    printf("ERASMO");
    return 0;
}

void refresh_tree_view()
{
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;

    const char *font_desc = "font-family: sans-serif; font-size: 16px";
    erasing_tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, DEVICE_TREEVIEW));


    /* --- Columna #1 --- */
    GtkCellRenderer *device_renderer = gtk_cell_renderer_text_new();  
    g_object_set(G_OBJECT(device_renderer), "font", font_desc, NULL);
    GtkTreeViewColumn *device_column = gtk_tree_view_column_new_with_attributes("Device", device_renderer, "text", STORAGE_DEVICE_NAME, NULL);
    gtk_tree_view_column_set_min_width(device_column, 170);
    gtk_tree_view_append_column(erasing_tree_view, device_column);

    /* --- Columna #2 --- */

    GtkCellRenderer *model_renderer = gtk_cell_renderer_text_new();  
    g_object_set(G_OBJECT(model_renderer), "font", font_desc, NULL);
    GtkTreeViewColumn *model_column = gtk_tree_view_column_new_with_attributes("Model", model_renderer, "text", STORAGE_DEVICE_MODEL, NULL);
    gtk_tree_view_column_set_min_width(model_column, 200);
    gtk_tree_view_append_column(erasing_tree_view, model_column);

    
    /* --- Columna #3 --- */
    GtkCellRenderer *status_renderer = gtk_cell_renderer_text_new();  
    g_object_set(G_OBJECT(status_renderer), "font", font_desc, NULL);
    GtkTreeViewColumn *state_column = gtk_tree_view_column_new_with_attributes("State", status_renderer, "text", ERASING_STORAGE_DEVICE_STATE_TEXT, NULL);
    gtk_tree_view_column_set_min_width(state_column, 100);
    gtk_tree_view_append_column(erasing_tree_view, state_column);



	GtkCellRenderer *progress_renderer = gtk_cell_renderer_progress_new();
	GtkTreeViewColumn *progress_column = gtk_tree_view_column_new_with_attributes("Progress", progress_renderer,
                                         "value",ERASING_STORAGE_DEVICE_PROGRESS,"text", ERASING_STORAGE_DEVICE_PROGRESS_TEXT, NULL);
                                         gtk_tree_view_column_set_min_width(state_column, 120);
    gtk_tree_view_append_column(erasing_tree_view, progress_column);
    
}

static GtkTreeModel *create_and_fill_model(void)
{
    GtkTreeStore *treestore;
    GtkTreeIter toplevel, child;

    // creamos un treestore con la cantidad de columnas y los tipos de campos que se usan
    treestore = gtk_tree_store_new(STORAGE_DEVICE_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_STRING);
                                                   
    char device_num[128];

    for (int i = 0; i < 7; i++)
    {
        /* Append a top level row and leave it empty */
        sprintf(device_num, "Device: %i", i);
        gtk_tree_store_append(treestore, &toplevel, NULL);

        gtk_tree_store_set(treestore, &toplevel, STORAGE_DEVICE_NAME, device_num, STORAGE_DEVICE_MODEL, "Samsung 2566g", ERASING_STORAGE_DEVICE_STATE_TEXT,"IDLE",ERASING_STORAGE_DEVICE_PROGRESS, (double)50, ERASING_STORAGE_DEVICE_PROGRESS_TEXT, "Borrando",-1);
    }


    return GTK_TREE_MODEL(treestore);
}
