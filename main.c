#include <stdio.h>
#include <stdlib.h>
#include <scsi/scsi_ioctl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "erasmo.h"
#include "build_device.h"
#include "sg_lib.h"
#include "sg_cmds.h"
#include "llseek.h"

/*
    sudo gcc -o erasmo main.c build_device.c sg_io_linux.c sg_cmds.c sg_pt_linux.c sg_lib.c erasing_device.c `pkg-config --cflags --libs gtk+-3.0` -rdynamic -I./include -ludev -lblkid 
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
#define INFO_BOX "txt_device_info"
#define G_N_ELEMENTS(arr)(sizeof (arr) / sizeof ((arr)[0]))

GtkWidget *window;
GtkWidget *txt_device_info;
GtkWidget *device_tree_selection;
GtkTextBuffer *buffer;
GtkBuilder *builder;
GtkTreeView *erasing_tree_view;
GtkTreeSelection *selection;
GtkTreeStore *treestore;
GtkTreeIter toplevel, child;
GtkTreeModel *model;

storage_device_list_t device_list;

pthread_mutex_t mutex;

char seleccionado[256];
char device_selected[512];

void *erase_thread_init();
void *write_on_device(void *device_selected);

//write module


int main(int argc, char *argv[])
{
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

    	GtkCssProvider *provider;
	GError *error;

	GdkDisplay *display;
    GdkScreen *screen;

	display = gdk_display_get_default ();
    screen = gdk_display_get_default_screen (display);

	provider = gtk_css_provider_new();
    gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	error = NULL;
    gtk_css_provider_load_from_file (provider, g_file_new_for_path("erasmo.css"), &error);

    gtk_widget_show(window);

    gtk_main();

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(erasing_tree_view));

    printf("ERASMO");
    return 0;
}

void create_treeview_columns()
{
    const char *font_desc = "font-family: sans-serif; font-size: 20px";
    erasing_tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, DEVICE_TREEVIEW));

    /* --- Columna #1 --- */
    GtkCellRenderer *device_renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(device_renderer), "font", font_desc, NULL);
    GtkTreeViewColumn *device_column = gtk_tree_view_column_new_with_attributes("Device", device_renderer, "text", STORAGE_DEVICE_NAME, NULL);
    gtk_tree_view_column_set_min_width(device_column, 250);
    gtk_tree_view_append_column(erasing_tree_view, device_column);

    /* --- Columna Model --- */

    GtkCellRenderer *model_renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(model_renderer), "font", font_desc, NULL);
    GtkTreeViewColumn *model_column = gtk_tree_view_column_new_with_attributes("Model", model_renderer, "text", STORAGE_DEVICE_MODEL, NULL);
    gtk_tree_view_column_set_min_width(model_column, 350);
    gtk_tree_view_append_column(erasing_tree_view, model_column);

    /* --- Columna #3 --- */
    GtkCellRenderer *status_renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(status_renderer), "font", font_desc, NULL);
    GtkTreeViewColumn *state_column = gtk_tree_view_column_new_with_attributes("State", status_renderer, "text", ERASING_STORAGE_DEVICE_STATE_TEXT, NULL);
    gtk_tree_view_column_set_min_width(state_column, 40);
    gtk_tree_view_append_column(erasing_tree_view, state_column);

    GtkCellRenderer *progress_renderer = gtk_cell_renderer_progress_new();
    GtkTreeViewColumn *progress_column = gtk_tree_view_column_new_with_attributes("Progress", progress_renderer,
                                                                                  "value", ERASING_STORAGE_DEVICE_PROGRESS, "text", ERASING_STORAGE_DEVICE_PROGRESS_TEXT, NULL);
    gtk_tree_view_column_set_min_width(state_column, 80);
    gtk_tree_view_append_column(erasing_tree_view, progress_column);
}

static GtkTreeModel *create_and_fill_model(){

    treestore = gtk_tree_store_new(STORAGE_DEVICE_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_STRING,
                                   G_TYPE_POINTER);

    init_storage_device_list(&device_list);

    storage_device_t disk;
    init_storage_device(&disk);

    if (detect_storage_devices(&device_list) == 0)
    {

        for (int dev_num = 0; dev_num < device_list.count; dev_num++)
        {

            set_device_capacity_data(&device_list.device[dev_num]);
            gtk_tree_store_append(treestore, &toplevel, NULL);
            device_list.device[dev_num].is_erased = false;
            gtk_tree_store_set(treestore, &toplevel, STORAGE_DEVICE_NAME, device_list.device[dev_num].name, STORAGE_DEVICE_MODEL,
                               device_list.device[dev_num].model, STORAGE_DEVICE_SERIAL, device_list.device[dev_num].serial,
                               ERASING_STORAGE_DEVICE_STATE_TEXT, device_list.device[dev_num].device_state, ERASING_STORAGE_DEVICE_PROGRESS,
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

void *erase_device_th(void * device_data){

    storage_device_t *device_recived;

    device_recived = (storage_device_t * )device_data;

    printf("tipo: %i\n",device_recived->type);

    switch (device_recived->type)
    {
    case INTERNAL_SG_DEVICE:
        printf("init erase internal device\n");
        break;
    case EXTERNAL_SG_DEVICE:
        printf("init erase external device\n");
        break;
    case NVME_DEVICE:
        printf("init erase NVME device\n");
        break; 
    default:
        break;
    }
    return NULL;
}

int erase_device()
{
    gchar *value;
    GtkTreeIter iter;
    pthread_t th_eraser;
    storage_device_t selected_device;
    init_storage_device(&selected_device);

    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(device_tree_selection), &model, &iter) == FALSE){
        return 1;
    }
        
    gtk_tree_model_get(model, &iter, 4, &value, -1);
    
    find_device_by_serial(device_list,&selected_device,value);
        
    pthread_create(&th_eraser,NULL,erase_device_th,&selected_device);
    
    pthread_join(th_eraser,NULL);

    return 0;
}

void refresh_devices()
{

}

void on_select_changed(GtkWidget *c){
    gchar *value;
    GtkTreeIter iter;
    GtkTreeModel *model;
    char text[720];  
    gchar storage_device_capacity_gb[32];
    const long double gb_koeff = (1000.0 * 1000.0) * 1000.0;

    storage_device_t disk_selected;
    init_storage_device(&disk_selected);

   

    txt_device_info = GTK_WIDGET(gtk_builder_get_object(builder,INFO_BOX));
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_device_info));

    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(c), &model, &iter) == FALSE)
        return;

    device_tree_selection = c;
    gtk_tree_model_get(model, &iter, 4, &value, -1);


    find_device_by_serial(device_list,&disk_selected,value);

    g_snprintf(storage_device_capacity_gb, G_N_ELEMENTS(storage_device_capacity_gb), "%.2Lf GB", (long double)(disk_selected.sector_size * disk_selected.total_sectors)/ gb_koeff);
    sprintf(text, "Name: %s\nVendor: %s\nModel: %s\nBus: %s\nCapacity: %s\nSerial: %s\n", 
    disk_selected.name, disk_selected.vendor, disk_selected.model, disk_selected.bus, storage_device_capacity_gb,disk_selected.serial);
    gtk_text_buffer_set_text(buffer,text ? text : "", -1); 

    strcpy(seleccionado,disk_selected.serial);

}


void on_destroy()
{
    gtk_main_quit();
}
