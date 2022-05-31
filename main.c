#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "erasmo.h"
#include "build_device.h"

/*
    gcc -o erasmo main.c build_device.c `pkg-config --cflags --libs gtk+-3.0` -rdynamic -I./include -ludev -lblkid
*/

#define BUILDER_FILE "window_main.glade"
#define MAIN_WINDOW "erasmo_main_window"
#define FIXED_WINDOW "fixed1"
#define ERASE_BUTTON "erase_button"
#define REFRESH_BUTTON "refresh_button"
#define CANCEL_BUTTON "cancel_button"
#define DEVICE_TREEVIEW "device_treeview"
#define DEVICE_SELECTION "device_selecction"
#define EMPTY_STRING ""

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
    ERASING_STORAGE_DEVICE_POINTER_TO_OBJECT,

    STORAGE_DEVICE_N_COLUMNS
};


GtkWidget *window;
GtkBuilder *builder;
GtkTreeView *erasing_tree_view;
GtkTreeSelection *selection;

char device_selected[512];

int main(int argc, char *argv[])
{

    GtkTreeModel *model;

    // window
    gtk_init(&argc, &argv);
    builder = gtk_builder_new_from_file(BUILDER_FILE);

    // treeview
    create_treeview_columns();
    model = create_and_fill_model();
    gtk_tree_view_set_model(erasing_tree_view, model);
    g_object_unref(model);

    window = GTK_WIDGET(gtk_builder_get_object(builder, MAIN_WINDOW));
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_builder_connect_signals(builder, NULL);

    selection = GTK_TREE_SELECTION(gtk_builder_get_object(builder, DEVICE_SELECTION));

    gtk_widget_show(window);

    gtk_main();

    // hw
    // libhw_init();

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(erasing_tree_view));

    printf("ERASMO");
    return 0;
}



void create_treeview_columns()
{
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;

    const char *font_desc = "font-family: sans-serif; font-size: 16px";
    erasing_tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, DEVICE_TREEVIEW));

    /* --- Columna #1 --- */
    GtkCellRenderer *device_renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(device_renderer), "font", font_desc, NULL);
    GtkTreeViewColumn *device_column = gtk_tree_view_column_new_with_attributes("Device", device_renderer, "text", STORAGE_DEVICE_NAME, NULL);
    gtk_tree_view_column_set_min_width(device_column, 160);
    gtk_tree_view_append_column(erasing_tree_view, device_column);

    /* --- Columna Model --- */

    GtkCellRenderer *model_renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(model_renderer), "font", font_desc, NULL);
    GtkTreeViewColumn *model_column = gtk_tree_view_column_new_with_attributes("Model", model_renderer, "text", STORAGE_DEVICE_MODEL, NULL);
    gtk_tree_view_column_set_min_width(model_column, 260);
    gtk_tree_view_append_column(erasing_tree_view, model_column);

    /* --- Columna Serial --- */

    GtkCellRenderer *serail_renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(serail_renderer), "font", font_desc, NULL);
    GtkTreeViewColumn *serail_column = gtk_tree_view_column_new_with_attributes("Serial No.", serail_renderer, "text", STORAGE_DEVICE_SERIAL, NULL);
    gtk_tree_view_column_set_min_width(serail_column, 250);
    gtk_tree_view_append_column(erasing_tree_view, serail_column);

    /* --- Columna #3 --- */
    GtkCellRenderer *status_renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(status_renderer), "font", font_desc, NULL);
    GtkTreeViewColumn *state_column = gtk_tree_view_column_new_with_attributes("State", status_renderer, "text", ERASING_STORAGE_DEVICE_STATE_TEXT, NULL);
    gtk_tree_view_column_set_min_width(state_column, 20);
    gtk_tree_view_append_column(erasing_tree_view, state_column);

    GtkCellRenderer *progress_renderer = gtk_cell_renderer_progress_new();
    GtkTreeViewColumn *progress_column = gtk_tree_view_column_new_with_attributes("Progress", progress_renderer,
                                                                                  "value", ERASING_STORAGE_DEVICE_PROGRESS, "text", ERASING_STORAGE_DEVICE_PROGRESS_TEXT, NULL);
    gtk_tree_view_column_set_min_width(state_column, 120);
    gtk_tree_view_append_column(erasing_tree_view, progress_column);
}



static GtkTreeModel *create_and_fill_model(){

    GtkTreeStore *treestore;
    GtkTreeIter toplevel, child;

    // creamos un treestore con la cantidad de columnas y los tipos de campos que se usan
    treestore = gtk_tree_store_new(STORAGE_DEVICE_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_STRING,
                                   G_TYPE_POINTER);

    char device_num[128];

    storage_device_list_t device_list;
    init_storage_device_list(&device_list);

    storage_device_t disk;
    init_storage_device(&disk);

    if (detect_storage_devices(&device_list) == 0)
    {

        for (int dev_num = 0; dev_num < device_list.count; dev_num++)
        {
            // nueva celda
            set_device_capacity_data(&device_list.device[dev_num]);
            gtk_tree_store_append(treestore, &toplevel, NULL);
            device_list.device[dev_num].is_erased = false;
            gtk_tree_store_set(treestore, &toplevel, STORAGE_DEVICE_NAME, device_list.device[dev_num].name, STORAGE_DEVICE_MODEL,
                               device_list.device[dev_num].model, STORAGE_DEVICE_SERIAL, device_list.device[dev_num].serial,
                               ERASING_STORAGE_DEVICE_STATE_TEXT, device_list.device[dev_num].erasing_status, ERASING_STORAGE_DEVICE_PROGRESS,
                               (double)0, ERASING_STORAGE_DEVICE_PROGRESS_TEXT, EMPTY_STRING, -1);
        }
    }

    return GTK_TREE_MODEL(treestore);
}



void alert(char msg[256])
{
    char cmd[512];
    sprintf(cmd, "zenity --info --text='%s'", msg);
    system(cmd);
}

void erase_device()
{
    alert(device_selected);
}

void refresh_devices()
{
    alert("Refresh devices");
}

void cancel_erasing_device()
{
    alert("cancel erase device");
}

void on_select_changed(GtkWidget *c){
    gchar *value;
    GtkTreeIter iter;
    GtkTreeModel *model;
    char *serial_no;

    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(c), &model, &iter) == FALSE)
        return;

    storage_device_t disk_selected;
    init_storage_device(&disk_selected);
    gtk_tree_model_get(model, &iter, 4, &value, -1);
    find_device_by_serial(&disk_selected, value);
    
    system("clear");
    printf("Name: %s\nVendor: %s\nModel: %s\nBus: %s\n", disk_selected.name, disk_selected.vendor, disk_selected.model, disk_selected.bus);
    printf("Block size: %lli\nBlock count: %lli\nsg dev: %s\n", disk_selected.sector_size,disk_selected.total_sectors,disk_selected.sg_name);
}



void on_destroy()
{
    gtk_main_quit();
}


