#include <gtk/gtk.h>

void on_destroy();
void on_select_changed(GtkWidget *c);
static GtkTreeModel *create_and_fill_model(void);
void create_treeview_columns();