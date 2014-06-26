#include "template_proc.h"
#include "sheet.h"


enum{
    ITEM_NUM,
    ITEM_NAME,
    ITEM_FIXED,
    ITEM_COUNT
};

typedef struct
{
  const gint      sequence;
  const gchar    *name;
  const gboolean  fixed;
  const gchar    *tooltips;
}NewItem;

static GArray items = NULL;


static void factory_template_get_editable_item(GList *oldlist,GList *newlist)
{
    items = g_array_new(FALSE,FLASE,sizeof(NewItem));
    GList *tlist = oldlist;
    for(;tlist;tlist = tlist->next)
    {
        SaveStruct *sst = tlist->data;
        GQuark quark = g_quark_from_string(sst->org->Cname);
        if(sst->isSensitive || (quark != item_reserverd)) /* 保留与固定值就过滤掉了 */
        {
            newlist = g_list_append(newlist,sst->org);
        }
    }
}

static void
fixed_toggled (GtkCellRendererToggle *cell,
               gchar                 *path_str,
               gpointer               data)
{
  GtkTreeModel *model = (GtkTreeModel *)data;
  GtkTreeIter  iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  gboolean fixed;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, 1, &fixed, -1);

  /* do something with the value */
  fixed ^= 1;

  /* set new value */
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, fixed, -1);

  /* clean up */
  gtk_tree_path_free (path);
}


static void factory_add_newstruct_columns (GtkTreeView *treeview)
{
     GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
      renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("成员名称"),
             renderer,
             "text",
             0,
             NULL);
       gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);

     renderer = gtk_cell_renderer_toggle_new ();
     gtk_cell_renderer_toggle_set_radio(renderer,TRUE);
     g_signal_connect (renderer, "toggled",
                    G_CALLBACK (fixed_toggled), model);

    column = gtk_tree_view_column_new_with_attributes ("是否固定?",
                                                     renderer,
                                                     "active", 1,
                                                     NULL);


       gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);

}

static void factory_append_item_to_idlist_model(GtkListStore *store,NewItem *item)
{

    GtkTreeIter iter;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set(store,&iter,
                       0,item->name,
                       1,item->fixed,-1);
}

void factory_template_edit_callback(GtkAction *action,GList *selectobjs)
{
    GList *objects = selectobjs; /* 选择的控件列表 */
    GtkWidget *vbox = gtk_vbox_new(TRUE,1);

    GtkWidget *newdialog = factory_create_new_dialog_with_buttons(factory_utf8("模版编辑"),ddisplay_active()->shell);
    GtkWidget *dlgvbox = GTK_DIALOG(newdialog)->vbox;
    GtkWidget *hbox = gtk_hbox_new(TRUE,1);
    GtkWidget *label = gtk_label_new(factory_utf8("模版名字"));
    GtkWidget *entry = gtk_entry_new();/* 时间戳做默认名字*/
    gtk_entry_set_text(GTK_ENTRY(entry),factory_get_current_timestamp());
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    g_object_set_data(vbox,"template_name",entry);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    GtkListStore *model = gtk_list_store_new(ITEM_COUNT,G_TYPE_INT,G_TYPE_STRING,G_TYPE_BOOLEAN);
    GList *newlist = NULL;
    for(;objects;objects = objects->next)
    {
        STRUCTClass *fclass = objects->data;
        factory_template_get_editable_item(fclass->widgetSave,newlist);
    }
    GtkTreeView *treeview = gtk_tree_view_new_with_model(model);
     GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);
    gtk_tree_view_set_grid_lines (treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);
    gtk_box_pack_start (GTK_BOX (vbox), wid_idlist, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(wid_idlist),treeview);
    gtk_container_add(GTK_CONTAINER(dlgvbox),vbox);
    gtk_widget_show_all(dlgvbox);
    gtk_dialog_run(newdialog);
//
//
//    fssl = g_new0(FactoryStructItemList,1);
//    fssl->sname = g_strdup("TEMPLATE");
//
//
//    fssl->vname = gtk_entry_get_text(entry);
//    if(!fssl->vname)
//        fssl->vname = g_strdup("nil");
//    fssl->isvisible = TRUE;
//    fssl->sfile = factory_get_utf8_str(isutf8,sbuf[5]);;
//    fssl->list = NULL;
//    fssl->number = n++;
//    dlist = NULL;


}
