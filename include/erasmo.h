#include <gtk/gtk.h>

void on_destroy();
void on_select_changed(GtkWidget *c);
static GtkTreeModel *create_and_fill_model(void);
void create_treeview_columns();
void *th_write_disk();
static int sg_write(int sg_fd, unsigned char *buff, int blocks, long long to_block, int bs, int cdbsz, int fua, int dpo, int *diop);
static int sg_build_scsi_cdb(unsigned char *cdbp, int cdb_sz, unsigned int blocks, long long start_block, int write_true, int fua, int dpo);
