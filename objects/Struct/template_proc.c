#include "template_proc.h"
#include "sheet.h"


enum
{
    ITEM_NAME,
    ITEM_BOOL,
    ITEM_VISIBLE,
    ITEM_COUNT
} ;

//typedef struct
//{
//
//     gchar    *name;
//     gboolean  fixed;
//     gchar    *tooltips;
//} NewItem;


typedef struct _NewItem NewItem;
struct _NewItem
{
  const gchar    *label;
  gboolean        fixed;
  gboolean        expand;
  GArray *children;
  gchar *tooltips;
  gint pos; /* 在原结构内的位置 */
};


static GArray *items = NULL;


static gboolean
query_tooltip_tree_view_cb (GtkWidget  *widget,
			    gint        x,
			    gint        y,
			    gboolean    keyboard_tip,
			    GtkTooltip *tooltip,
			    gpointer    data)
{
  GtkTreeIter iter;
  GtkTreeView *tree_view = GTK_TREE_VIEW (widget);
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreePath *path = NULL;
  gint num;
  gchar *pathstring;

  if (!gtk_tree_view_get_tooltip_context (tree_view, &x, &y,
					  keyboard_tip,
					  &model, &path, &iter))
    return FALSE;

  gtk_tree_model_get (model, &iter, 0, &num, -1);/* 找到行号 */
  pathstring = gtk_tree_path_to_string (path);
  gchar **split = g_strsplit(pathstring,":",-1);
  if(g_strv_length(split) > 1)
  {
      int n = g_strtod(split[0],NULL);
      gtk_tooltip_set_markup (tooltip,"test"); /*找出这一行的tooltips 字符串*/
  }
  else
  {

  }



  gtk_tree_view_set_tooltip_row (tree_view, tooltip, path);

  gtk_tree_path_free (path);
  g_free (pathstring);


  return TRUE;
}


GList *newlist = NULL;

static void factory_template_get_editable_item(GList *oldlist,GArray *array)
{

    GList *tlist = oldlist;
    int n = 0;
    for(; tlist; tlist = tlist->next,n++)
    {
        SaveStruct *sst = tlist->data;
        GQuark quark = g_quark_from_string(sst->org->Cname);
        if( sst->org->isSensitive && sst->org->isVisible) /* 保留与固定值就过滤掉了 */
        {
            newlist = g_list_append(newlist,sst->org);
            NewItem *nitem = g_new0(NewItem,1);
            nitem->label = g_strdup(sst->org->Cname);
            nitem->fixed = TRUE;
            nitem->expand = TRUE;
            nitem->children = NULL;
            nitem->tooltips = g_strdup(sst->org->Comment);
            nitem->pos = n;
            g_array_append_vals(array,nitem,1);
        }
    }
}


static void
item_toggled (GtkCellRendererToggle *cell,
	      gchar                 *path_str,
	      gpointer               data)
{
  GtkTreeModel *model = (GtkTreeModel *)data;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  GtkTreeIter iter;
  gboolean toggle_item;

  gint *column;

  column = g_object_get_data (G_OBJECT (cell), "column");

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, column, &toggle_item, -1);

  /* do something with the value */
  toggle_item ^= 1;

  /* set new value */
  gtk_tree_store_set (GTK_TREE_STORE (model), &iter, column,
		      toggle_item, -1);

  /* clean up */
  gtk_tree_path_free (path);
}


static void factory_add_newstruct_columns (GtkTreeView *treeview)
{
    GtkCellRenderer *renderer;
    gint col_offset;
    GtkTreeViewColumn *column;
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);



   renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);

  col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
							    -1, factory_utf8("行为名字"),
							    renderer, "text",
							    ITEM_NAME,
							    NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
  gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);



    renderer = gtk_cell_renderer_toggle_new ();
//    gtk_cell_renderer_toggle_set_radio(renderer,TRUE);

    g_object_set_data (G_OBJECT (renderer), "column", (gint *)ITEM_BOOL);


    g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

    col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
							    -1, factory_utf8("是否可编辑?"),
							    renderer,
							    "active",
							    ITEM_BOOL,
							    "visible",
							    ITEM_VISIBLE,
							    NULL);

  column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
				   GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);

}




static void factory_append_struct_to_model(GtkTreeModel *model,NewItem *topitem)
{
    GtkTreeIter iter;
    gtk_tree_store_append (model, &iter,NULL);
    gtk_tree_store_set(model,&iter,
                       ITEM_NAME,topitem->label,
                       ITEM_BOOL,FALSE,
                       ITEM_VISIBLE,FALSE,
                       -1);
    int n = 0;
    for(;n < topitem->children->len;n++)
    {
        GtkTreeIter child_iter;
        NewItem childitem = g_array_index(topitem->children,NewItem,n);
	    gtk_tree_store_append (model, &child_iter, &iter);
	     gtk_tree_store_set(model,&child_iter,
                       ITEM_NAME,childitem.label,
                       ITEM_BOOL,childitem.fixed,
                       ITEM_VISIBLE,TRUE,
                       -1);
    }
}

void factory_template_edit_callback(GtkAction *action,GList *selectobjs)
{
    GList *objects = selectobjs; /* 选择的控件列表 */
    GtkWidget *vbox = gtk_vbox_new(FALSE,1);

    GtkWidget *tooltips = gtk_tooltips_new();
    GtkWidget *newdialog = factory_create_new_dialog_with_buttons(factory_utf8("模版编辑"),GTK_WINDOW (interface_get_toolbox_shell ()));
//    gtk_window_set_modal(newdialog,FALSE);
    GtkWidget *dlgvbox = GTK_DIALOG(newdialog)->vbox;
    GtkWidget *hbox = gtk_hbox_new(FALSE,1);
    GtkWidget *label = gtk_label_new(factory_utf8("模版名字:"));
    GtkWidget *entry = gtk_entry_new();/* 时间戳做默认名字*/
    gtk_entry_set_text(GTK_ENTRY(entry),factory_get_current_timestamp());
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    g_object_set_data(vbox,"template_name",entry);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
    gtk_tooltips_set_tip(tooltips,entry,factory_utf8("强烈建议改成有意义的名字!"),NULL);


    hbox = gtk_hbox_new(FALSE,1);
    label = gtk_label_new(factory_utf8("保存文件名:"));
    entry = gtk_entry_new();
    gtk_tooltips_set_tip(tooltips,entry,factory_utf8("保存文件时,指定文件名"),NULL);
    gtk_entry_set_text(GTK_ENTRY(entry),"act.inf");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    g_object_set_data(vbox,"sfile",entry);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_store_new(ITEM_COUNT,
                                                            G_TYPE_STRING,
                                                            G_TYPE_BOOLEAN,
                                                            G_TYPE_BOOLEAN));

    for(; objects; objects = objects->next)
    {
        STRUCTClass *fclass = objects->data;
        NewItem *topitem = g_new0(NewItem,1);
        topitem->label = g_strdup(fclass->name);
        topitem->fixed = FALSE;
        topitem->expand = FALSE;
        topitem->children = g_array_new(FALSE,FALSE,sizeof(NewItem));
        factory_template_get_editable_item(fclass->widgetSave,topitem->children);
        if(topitem->children->len > 0)
            factory_append_struct_to_model(model,topitem);
    }

    GtkTreeView *treeview = gtk_tree_view_new_with_model(model);
    g_object_unref (model);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)),
				   GTK_SELECTION_SINGLE);
    factory_add_newstruct_columns(treeview);

    g_signal_connect (treeview, "realize",
			G_CALLBACK (gtk_tree_view_expand_all), NULL);
    g_object_set(treeview,"has-tooltip",TRUE,NULL);
    g_signal_connect(G_OBJECT(treeview),"query-tooltip",G_CALLBACK(query_tooltip_tree_view_cb),NULL);

    gtk_tree_view_set_headers_visible(treeview,TRUE);

    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
    gtk_widget_set_size_request(wid_idlist,300,500);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);
    gtk_tree_view_set_grid_lines (treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);
    gtk_box_pack_start (GTK_BOX (vbox), wid_idlist, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(wid_idlist),treeview);
    gtk_container_add(GTK_CONTAINER(dlgvbox),vbox);
    gtk_widget_show_all(vbox);
    g_signal_connect(newdialog,"response",G_CALLBACK(factory_template_save_dialog),model);


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


static gboolean factory_filter_checked_item(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    gchar *item = (gchar *)data;
    gchar *str ;
    gtk_tree_model_get(model,iter,COLUMN_ITEM_IDNAME,&str,-1);
    if(!g_strcasecmp(item,str))
    {
        gtk_list_store_set(model,iter,COLUMN_ITEM_IDNAME,"",-1);
    }
    g_free(str);
    return FALSE;
}

void factory_template_save_dialog(GtkWidget *widget,
                                      gint       response_id,
                                      gpointer   user_data)
{
    if(response_id == GTK_RESPONSE_OK)
    {
        GList *objects = user_data;
        FactoryStructItemList *fssl = g_new0(FactoryStructItemList,1);
        fssl->sname = g_strdup("TEMPLATE");
        GtkWidget *entry = g_object_get_data (widget,"template_name");

        fssl->vname = gtk_entry_get_text(entry);
        if(!fssl->vname)
            fssl->vname = g_strdup("nil");
        fssl->isvisible = TRUE;
        fssl->sfile = NULL;
        fssl->list = NULL;
        fssl->number = 0;

//         gtk_tree_model_foreach(smd->id_store,factory_filter_checked_item,item);

    }


    gtk_widget_destroy(widget);
}


