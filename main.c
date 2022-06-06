#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <scsi/scsi_ioctl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "erasmo.h"
#include "build_device.h"
#include "sg_lib.h"
#include "sg_cmds.h"
#include "sg_io_linux.h"
#include "llseek.h"

/*
    gcc -o erasmo main.c build_device.c sg_io_linux.c sg_cmds.c sg_pt_linux.c sg_lib.c `pkg-config --cflags --libs gtk+-3.0` -rdynamic -I./include -ludev -lblkid  -Wall
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

//Write Module
#define DEF_SCSI_CDBSZ 10
#define DEF_TIMEOUT 60000       /* 60,000 millisecs == 60 seconds */
#define MAX_SCSI_CDBSZ 16
#define SENSE_BUFF_LEN 64       /* Arbitrary, could be larger */
#define ME "erasmo"
#define SG_DD_BYPASS 999        /* failed but coe set */
#define SG_PATH_SIZE 512


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
GtkWidget *txt_device_info;
GtkTextBuffer *buffer;
GtkBuilder *builder;
GtkTreeView *erasing_tree_view;
GtkTreeSelection *selection;
GtkTreeStore *treestore;
GtkTreeIter toplevel, child;
GtkTreeModel *model;

storage_device_list_t erasing_devices;

char seleccionado[256];
static int recovered_errs = 0;
char device_selected[512];

void *erase_thread_init();

//write module
static struct flags_t oflag;


struct flags_t {
    bool append;
    bool dio;
    bool direct;
    bool dpo;
    bool dsync;
    bool excl;
    bool flock;
    bool ff;
    bool fua;
    bool nocreat;
    bool random;
    bool sgio;
    bool sparse;
    bool zero;
    int cdbsz;
    int cdl;
    int coe;
    int nocache;
    int pdt;
    int retries;
};

int main(int argc, char *argv[])
{
    // window
    gtk_init(&argc, &argv);
    builder = gtk_builder_new_from_file(BUILDER_FILE);
    init_storage_device_list(&erasing_devices);

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

    // hw
    // libhw_init();

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

    /* --- Columna Serial --- 

    GtkCellRenderer *serail_renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(serail_renderer), "font", font_desc, NULL);
    GtkTreeViewColumn *serail_column = gtk_tree_view_column_new_with_attributes("Serial No.", serail_renderer, "text", STORAGE_DEVICE_SERIAL, NULL);
    gtk_tree_view_column_set_min_width(serail_column, 250);
    gtk_tree_view_append_column(erasing_tree_view, serail_column);*/

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

   // creamos un treestore con la cantidad de columnas y los tipos de campos que se usan
    treestore = gtk_tree_store_new(STORAGE_DEVICE_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_STRING,
                                   G_TYPE_POINTER);

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


void erase_device()
{
    pthread_t h_eraser;

    storage_device_t device_erasing;
    init_storage_device(&device_erasing);
    
    find_device_by_serial(&device_erasing,seleccionado);

    pthread_create(&h_eraser, NULL, erase_thread_init ,NULL );

    //pthread_join(h_eraser, NULL ) ;

}



void append_erasing_device(storage_device_t *const device){
    

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

    txt_device_info = GTK_WIDGET(gtk_builder_get_object(builder,INFO_BOX));
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_device_info));

    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(c), &model, &iter) == FALSE)
        return;

    storage_device_t disk_selected;
    init_storage_device(&disk_selected);

    gtk_tree_model_get(model, &iter, 4, &value, -1);
    const long double gb_koeff = (1000.0 * 1000.0) * 1000.0;

    find_device_by_serial(&disk_selected, value);

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

void quit_aplication()
{
    
}

void *erase_thread_init(){

    GtkTreeIter storage_device_itr;
    gboolean is_iter_valid  = gtk_tree_model_get_iter_first(model,&storage_device_itr);
    
    storage_device_t erasing_device;
    init_storage_device(&erasing_device);
    find_device_by_serial(&erasing_device,seleccionado);

    while (is_iter_valid == TRUE)
	{
		gchar *device_serial;
		gtk_tree_model_get(model, &storage_device_itr, STORAGE_DEVICE_SERIAL, &device_serial, -1);

		if (strcmp(seleccionado, device_serial) == 0)
		{
			g_free(device_serial);
            break;
		}
		g_free(device_serial);
		is_iter_valid = gtk_tree_model_iter_next(model, &storage_device_itr);
	}
    
    gtk_tree_store_set(treestore,&storage_device_itr,ERASING_STORAGE_DEVICE_STATE_TEXT,"Erasing...",-1);
    char porcent[32];
    for(int i = 0; i < 101; i++){
        gtk_tree_store_set(treestore,&storage_device_itr,ERASING_STORAGE_DEVICE_PROGRESS,(double)i,-1);
        sprintf(porcent,"Progress... %i%%\n",i);
        gtk_tree_store_set(treestore,&storage_device_itr,ERASING_STORAGE_DEVICE_PROGRESS_TEXT,porcent,-1);
        usleep (1000000);
    }
    gtk_tree_store_set(treestore,&storage_device_itr,ERASING_STORAGE_DEVICE_STATE_TEXT,"Erased",-1);
    return NULL;
} 

void cancel_erasing_device()
{
    pthread_t h1 ;

    pthread_create(&h1, NULL, erase_thread_init ,NULL );

}


void *th_write_disk(){

    int res, dio_tmp;
    int outfd, blocks;

    unsigned char *wrkPos;
    unsigned char *fprint;
    //long long skip = 0;
    long long seek = 1;
    static int blk_sz = 512;
    int scsi_cdbsz_out = DEF_SCSI_CDBSZ;
    char inf[512];
    unsigned char *wrkBuff;
    unsigned char *wrkBuff2;

    uint8_t erasmosign[] = 
    {
    0x51, 0x75, 0x61, 0x6E, 0x74, 0x75, 0x6D, 0x20, 0x65, 0x72, 0x61, 0x73,
    0x6D, 0x6F, 0x28, 0x52, 0x29, 0x20, 0x62, 0x79, 0x20, 0x4D, 0x6F, 0x62,
    0x69, 0x6C, 0x69, 0x74, 0x79, 0x20, 0x54, 0x65, 0x61, 0x6D, 0x0a, 0x0a
    };

    strcpy(inf,"/dev/sg4");

    blocks=128;
    int bpt = 128;
    int device_blocks = 3839999;


    
    size_t psz = getpagesize();
    wrkBuff = malloc(blk_sz * bpt + psz);

    wrkBuff2 = malloc(512);
    long long int tfwide = blk_sz * bpt + psz;
    
    uint8_t data[tfwide];

    memset(data,0x45,sizeof(data));

    wrkPos = wrkBuff;
    memcpy(wrkPos,&data,sizeof(data));
    
    fprint = wrkBuff2;
    memcpy(fprint,&erasmosign,sizeof(erasmosign));

    //open device.
    if ((outfd = sg_cmds_open_device(inf, 1, 0)) < 0)
    {   
        fprintf(stderr, ME " Device %s dont exist\n", inf);
    }

    dio_tmp = 0;

    for(int i = 1;i < (device_blocks / blocks);i++){

        printf("%lli de %i blocks...\n",seek, device_blocks);
        res = sg_write(outfd, wrkPos, blocks, seek, blk_sz, scsi_cdbsz_out, oflag.fua, oflag.dpo, &dio_tmp);
        seek += blocks;
        system("clear");
        
        
    }

    int blk_remains = device_blocks - seek;

    if(seek > 0){

        for(int i = 1;i < blk_remains + 2 ;i++){
            printf("%lli de %i blocks...\n",seek, device_blocks);
            res = sg_write(outfd, wrkPos, 1, seek, blk_sz, scsi_cdbsz_out, oflag.fua, oflag.dpo, &dio_tmp);
            seek++;
            system("clear");
            
        }
    }

    printf("restantes: %lli", device_blocks - seek);

    if (res = sg_write(outfd, fprint, 1, 0, blk_sz, scsi_cdbsz_out, oflag.fua, oflag.dpo, &dio_tmp)){
        printf("OK");
    }

    free(wrkBuff);


}



/* 0 -> successful, -1 -> unrecoverable error, -2 -> recoverable (ENOMEM),
   -3 -> try again (media changed unit attention) */
static int sg_write(int sg_fd, unsigned char *buff, int blocks, long long to_block, int bs, int cdbsz, int fua, int dpo, int *diop){

    unsigned char wrCmd[MAX_SCSI_CDBSZ];
    unsigned char senseBuff[SENSE_BUFF_LEN];
    struct sg_io_hdr io_hdr;
    int res, k, info_valid;
    unsigned long long io_addr = 0;

    if (sg_build_scsi_cdb(wrCmd, cdbsz, blocks, to_block, 1, fua, dpo))
    {
        fprintf(stderr, ME " bad wr cdb build, to_block=%lld, blocks=%d\n", to_block, blocks);
        return -1;
    }

    memset(&io_hdr, 0, sizeof(struct sg_io_hdr));


    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = cdbsz;
    io_hdr.cmdp = wrCmd;
    io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
    io_hdr.dxfer_len = bs * blocks;
    io_hdr.dxferp = buff;
    io_hdr.mx_sb_len = SENSE_BUFF_LEN;
    io_hdr.sbp = senseBuff;
    io_hdr.timeout = DEF_TIMEOUT;
    io_hdr.pack_id = (int)to_block;

    if (diop && *diop)
        io_hdr.flags |= SG_FLAG_DIRECT_IO;

    if (0 > 2)
    {
        fprintf(stderr, "    write cdb: ");
        for (k = 0; k < cdbsz; ++k)
            fprintf(stderr, "%02x ", wrCmd[k]);
        fprintf(stderr, "\n");
    }
    while (((res = ioctl(sg_fd, SG_IO, &io_hdr)) < 0) && (EINTR == errno))
        ;
    if (res < 0)
    {
        if (ENOMEM == errno)
            return -2;
        perror("writing (SG_IO) on sg device, error");
        return -1;
    }

    if (0 > 2)
        fprintf(stderr, "      duration=%u ms\n", io_hdr.duration);
    switch (sg_err_category3(&io_hdr))
    {
    case SG_LIB_CAT_CLEAN:
        break;
    case SG_LIB_CAT_RECOVERED:
        ++recovered_errs;
        info_valid = sg_get_sense_info_fld(io_hdr.sbp, io_hdr.sb_len_wr, &io_addr);

        if (info_valid)
        {
            fprintf(stderr, "    lba of last recovered error in this WRITE=0x%llx\n", io_addr);
            if (0 > 1)
                sg_chk_n_print3("writing", &io_hdr, 1);
        }
        else
        {
            fprintf(stderr, "Recovered error: [no info] writing to "
                            "block=0x%llx, num=%d\n",
                    to_block, blocks);
            sg_chk_n_print3("writing", &io_hdr, 0 > 1);
        }
        break;
    case SG_LIB_CAT_MEDIA_CHANGED:
        if (0 > 1)
            sg_chk_n_print3("writing", &io_hdr, 1);
        return -3;
    default:
        sg_chk_n_print3("writing", &io_hdr, 0 > 1);
        if (oflag.coe)
        {
            fprintf(stderr, ">> ignored errors for out blk=%lld for "
                            "%d bytes\n",
                    to_block, bs * blocks);
            return 0; /* fudge success */
        }
        else
            return -1;
    }
    if (diop && *diop &&
        ((io_hdr.info & SG_INFO_DIRECT_IO_MASK) != SG_INFO_DIRECT_IO))
        *diop = 0; /* flag that dio not done (completely) */
    return 0;
}


static int sg_build_scsi_cdb(unsigned char *cdbp, int cdb_sz,
                             unsigned int blocks, long long start_block,
                             int write_true, int fua, int dpo)
{
    int rd_opcode[] = {0x8, 0x28, 0xa8, 0x88};
    int wr_opcode[] = {0xa, 0x2a, 0xaa, 0x8a};
    int sz_ind;

    memset(cdbp, 0, cdb_sz);
    if (dpo)
        cdbp[1] |= 0x10;
    if (fua)
        cdbp[1] |= 0x8;
    switch (cdb_sz)
    {
    case 6:
        sz_ind = 0;
        cdbp[0] = (unsigned char)(write_true ? wr_opcode[sz_ind] : rd_opcode[sz_ind]);
        cdbp[1] = (unsigned char)((start_block >> 16) & 0x1f);
        cdbp[2] = (unsigned char)((start_block >> 8) & 0xff);
        cdbp[3] = (unsigned char)(start_block & 0xff);
        cdbp[4] = (256 == blocks) ? 0 : (unsigned char)blocks;
        if (blocks > 256)
        {
            fprintf(stderr, ME "for 6 byte commands, maximum number of "
                               "blocks is 256\n");
            return 1;
        }
        if ((start_block + blocks - 1) & (~0x1fffff))
        {
            fprintf(stderr, ME "for 6 byte commands, can't address blocks"
                               " beyond %d\n",
                    0x1fffff);
            return 1;
        }
        if (dpo || fua)
        {
            fprintf(stderr, ME "for 6 byte commands, neither dpo nor fua"
                               " bits supported\n");
            return 1;
        }
        break;
    case 10:
        sz_ind = 1;
        cdbp[0] = (unsigned char)(write_true ? wr_opcode[sz_ind] : rd_opcode[sz_ind]);
        cdbp[2] = (unsigned char)((start_block >> 24) & 0xff);
        cdbp[3] = (unsigned char)((start_block >> 16) & 0xff);
        cdbp[4] = (unsigned char)((start_block >> 8) & 0xff);
        cdbp[5] = (unsigned char)(start_block & 0xff);
        cdbp[7] = (unsigned char)((blocks >> 8) & 0xff);
        cdbp[8] = (unsigned char)(blocks & 0xff);
        if (blocks & (~0xffff))
        {
            fprintf(stderr, ME "for 10 byte commands, maximum number of "
                               "blocks is %d\n",
                    0xffff);
            return 1;
        }
        break;
    case 12:
        sz_ind = 2;
        cdbp[0] = (unsigned char)(write_true ? wr_opcode[sz_ind] : rd_opcode[sz_ind]);
        cdbp[2] = (unsigned char)((start_block >> 24) & 0xff);
        cdbp[3] = (unsigned char)((start_block >> 16) & 0xff);
        cdbp[4] = (unsigned char)((start_block >> 8) & 0xff);
        cdbp[5] = (unsigned char)(start_block & 0xff);
        cdbp[6] = (unsigned char)((blocks >> 24) & 0xff);
        cdbp[7] = (unsigned char)((blocks >> 16) & 0xff);
        cdbp[8] = (unsigned char)((blocks >> 8) & 0xff);
        cdbp[9] = (unsigned char)(blocks & 0xff);
        break;
    case 16:
        sz_ind = 3;
        cdbp[0] = (unsigned char)(write_true ? wr_opcode[sz_ind] : rd_opcode[sz_ind]);
        cdbp[2] = (unsigned char)((start_block >> 56) & 0xff);
        cdbp[3] = (unsigned char)((start_block >> 48) & 0xff);
        cdbp[4] = (unsigned char)((start_block >> 40) & 0xff);
        cdbp[5] = (unsigned char)((start_block >> 32) & 0xff);
        cdbp[6] = (unsigned char)((start_block >> 24) & 0xff);
        cdbp[7] = (unsigned char)((start_block >> 16) & 0xff);
        cdbp[8] = (unsigned char)((start_block >> 8) & 0xff);
        cdbp[9] = (unsigned char)(start_block & 0xff);
        cdbp[10] = (unsigned char)((blocks >> 24) & 0xff);
        cdbp[11] = (unsigned char)((blocks >> 16) & 0xff);
        cdbp[12] = (unsigned char)((blocks >> 8) & 0xff);
        cdbp[13] = (unsigned char)(blocks & 0xff);
        break;
    default:
        fprintf(stderr, ME "expected cdb size of 6, 10, 12, or 16 but got"
                           " %d\n",
                cdb_sz);
        return 1;
    }
    return 0;
}

