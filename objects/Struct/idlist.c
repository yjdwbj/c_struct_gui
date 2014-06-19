#include "idlist.h"


static void
factory_idcell_edited (GtkCellRendererText *cell,
                       const gchar         *path_string,
                       const gchar         *new_text,
                       gpointer             data)
{

    GtkTreeModel *model = (GtkTreeModel *)data;
    GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
    GtkTreeIter iter;

    gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

    gtk_tree_model_get_iter (model, &iter, path);

    if(column == COLUMN_ITEM_IDNAME)
    {

        gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                            new_text, -1);
    }

    gtk_tree_path_free (path);
}



static void factory_add_idlist_columns (GtkTreeView *treeview,GtkTreeModel *cbmodel)
{

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore *modal  = gtk_tree_view_get_model(treeview);

    /* column for bug numbers */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("序号"),
             renderer,
             "text",
             COLUMN_ITEM_SEQUENCE,
             NULL);
//    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ITEM_SEQUENCE);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);

    /* column for severities */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("地址"),
             renderer,
             "text",
             COLUMN_ITEM_ADDR,
             NULL);
//    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ITEM_ADDR);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);

    /* Combo */

    renderer = gtk_cell_renderer_combo_new ();
//    gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(renderer),1);
//    gtk_combo_box_popdown (GTK_COMBO_BOX(renderer));
    g_object_set (renderer,
                  "model", cbmodel,
                  "text-column",0,
                  "has-entry", FALSE,
                  "editable", TRUE,
                  NULL);
    g_signal_connect (renderer, "edited",
                      G_CALLBACK (factory_idcell_edited), modal);
//    g_signal_connect (renderer, "editing-started",
//                      G_CALLBACK (factory_idcell_edited), items_model);


    g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_ITEM_IDNAME));

    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
            -1,factory_utf8("行为(点击本列空白处)"), renderer,
            "text", COLUMN_ITEM_IDNAME,
            NULL);

}



static void factory_append_item_to_idlist_model(GtkListStore *store,IDListStore *idsave)
{

    GtkTreeIter iter;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set(store,&iter,
                       COLUMN_ITEM_SEQUENCE,idsave->sequence,
                       COLUMN_ITEM_ADDR,idsave->id_addr,
                       COLUMN_ITEM_IDNAME,idsave->id_text,
                       -1);
}


static void factory_add_item_to_idlist_model(GtkWidget *btn, gpointer user_data)
{
    SaveIdDialog *sid = (SaveIdDialog *)user_data;
    gint len = GTK_LIST_STORE(sid->id_store)->length ;
    IDListStore *idsave = g_new0(IDListStore,1);
    idsave->sequence = len;
    idsave->id_addr = len*2;
    idsave->id_text = g_strdup("");

    factory_append_item_to_idlist_model(sid->id_store,idsave);
    sid->idlists = g_list_append(sid->idlists,idsave);
}

GtkTreeModel *factory_create_idcombox_model (GList *idlist)
{
    GtkListStore *model;
    GtkTreeIter iter;
    /* create list store */
    model = gtk_list_store_new (1, G_TYPE_STRING);

    GList *flist = idlist;
    factory_append_iten_to_cbmodal(model,"");
    for(; flist; flist = flist->next)
    {
        factory_append_iten_to_cbmodal(model,flist->data);
    }
    return GTK_TREE_MODEL(model);
}

static void factory_delete_last_model_item(GtkWidget *btn,gpointer user_data)
{
    /* 删除最后一个*/
    GtkTreeIter iter;
    SaveIdDialog *sid = (SaveIdDialog *)user_data;
    GtkTreeModel *model = sid->id_store;
//   GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
    gint rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL);
    if(!rows)
        return;
    GtkTreePath *path;
    path = gtk_tree_path_new_from_indices(rows - 1, -1);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    gtk_tree_path_free (path);
    gpointer lastptr = g_list_nth_data(sid->idlists,rows-1);
    sid->idlists = g_list_remove(sid->idlists,lastptr);
}

static gboolean factory_save_idlist_lastcolumn_foreach(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    SaveMusicDialog *smd = curLayer->smd;
    gint pos = gtk_tree_path_get_indices (path)[0];
    IDListStore *idsave = g_list_nth_data(data,pos);
    gtk_tree_model_get(model,iter,COLUMN_ITEM_IDNAME,&idsave->id_text,-1);
//    gtk_tree_model_foreach(smd->id_cbmodal,factory_cboxmodel_foreach,smt);
    return FALSE;
}


void factory_new_idlist_dialog(GtkWidget *button,SaveStruct *sst)
{
    GtkWidget *mainBox = gtk_vbox_new(FALSE,0);
    SaveIdDialog *sid = (SaveIdDialog *)curLayer->sid;
    GtkWidget *parent = gtk_widget_get_toplevel(button);
    GtkWidget *subdig = factory_create_new_dialog_with_buttons(factory_utf8("ID列表"),gtk_widget_get_toplevel(button));
    GtkWidget *dialog_vbox = GTK_DIALOG(subdig)->vbox;
    gtk_window_set_modal(GTK_WINDOW(subdig),TRUE);
    gtk_window_set_resizable (GTK_WINDOW (subdig),TRUE);
    gtk_widget_set_size_request (subdig,300,500);
    gtk_container_add(GTK_CONTAINER(dialog_vbox),mainBox);
    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);

    GList *p = g_hash_table_get_keys(curLayer->defnames);
    p = g_list_sort(p,factory_str_compare);
    sid->flist = p;

    sid->id_cbmodel = factory_create_idcombox_model(p); /* 创建要填充的下拉表的链表 */

    gtk_box_pack_start(GTK_BOX(mainBox),wid_idlist,TRUE,TRUE,0);

    /* 创建IDlist 主界面的模型 */
    sid->id_store = gtk_list_store_new(NUM_OF_COLS,G_TYPE_INT,G_TYPE_INT,G_TYPE_STRING);

    sid->id_treeview = gtk_tree_view_new_with_model (sid->id_store);
    gtk_tree_view_set_grid_lines (sid->id_treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);

    factory_add_idlist_columns(sid->id_treeview,sid->id_cbmodel); /* 添加列 */


    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (sid->id_treeview), TRUE);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (sid->id_treeview)),
                                 GTK_SELECTION_SINGLE);

    GtkTreeIter iter;
    if(sid->idlists) /* 原来的值 */
    {
        GtkTreePath *path;
        GList *tlist = sid->idlists;
        for(; tlist ; tlist = tlist->next)
        {
            IDListStore *idsave = tlist->data;

            factory_append_item_to_idlist_model(sid->id_store,tlist->data);
        }
        path = gtk_tree_path_new_from_string(sst->value.vnumber);
        gtk_tree_model_get_iter(GTK_TREE_MODEL(sid->id_store), &iter, path);
        GtkTreeSelection *selection = gtk_tree_view_get_selection (sid->id_treeview);
        gtk_tree_selection_select_iter (selection,&iter); /* 选择到上次 */
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (sid->id_treeview), path, NULL, TRUE, 1.0, 0);
        gtk_tree_path_free(path);
    }

    gtk_container_add (GTK_CONTAINER (wid_idlist), sid->id_treeview);
//    gtk_box_pack_start(GTK_BOX(vbox),sid->id_treeview,FALSE,FALSE,0);
    g_object_unref (sid->id_store);
    g_object_unref(sid->id_cbmodel);

    GtkWidget *opthbox = gtk_hbox_new(TRUE,10);
    gtk_box_pack_start(GTK_BOX(opthbox),
                       factory_new_add_button(factory_add_item_to_idlist_model,sid),FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(opthbox),
                       factory_delete_last_button(factory_delete_last_model_item,sid),FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(mainBox),opthbox,FALSE,FALSE,0);

//    gtk_box_pack_start(GTK_BOX(mainBox),factory_new_add_button(factory_add_item_to_idlist_model,sid),FALSE,FALSE,0);
    gtk_widget_show_all(subdig);
    gint ret = gtk_dialog_run(subdig); /* 阻塞方式运行 */
    if(ret == GTK_RESPONSE_OK) /* 把判断保存做在这里,可以减变量的数量,与函数之间的传递. */
    {
        GtkTreePath *path;
        GList *tlist = sid->idlists;
        gtk_tree_model_foreach(sid->id_store,factory_save_idlist_lastcolumn_foreach,sid->idlists);
        GtkTreeSelection *selection = gtk_tree_view_get_selection (sid->id_treeview);
        if (gtk_tree_selection_get_selected (selection, NULL, &iter))
        {
            path = gtk_tree_model_get_path (sid->id_store, &iter);
            sst->value.vnumber =g_strdup(gtk_tree_path_to_string (path));
        }
        else
        {
            sst->value.vnumber = g_strdup("-1");
        }

        gtk_button_set_label(GTK_BUTTON(button),sst->value.vnumber );
        gtk_tree_path_free(path);
    }
    gtk_widget_destroy(subdig);
}

void factory_save_idlist_to_xml(SaveStruct *sss,ObjectNode obj_node)
{
    ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_item", NULL);
    xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)sss->name);
    xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
    xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)sss->type);

    xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)sss->value.vnumber);

    SaveIdDialog *sid = (SaveIdDialog *)curLayer->sid;
    gchar *idname = g_strdup("");
    if(sid->id_store)
    {
        GtkTreePath *path = gtk_tree_path_new_from_string (sss->value.vnumber);
        GtkTreeIter iter;
        gtk_tree_model_get_iter (sid->id_store, &iter, path);
        gtk_tree_model_get(sid->id_store,&iter,COLUMN_ITEM_IDNAME,&idname,-1);
    }
    xmlSetProp(ccc, (const xmlChar *)"idname", (xmlChar *)idname);
    g_free(idname);
}



void factory_read_idlist_items(ObjectNode obj_node)
{
    SaveIdDialog *sid = (SaveIdDialog *)curLayer->sid;
    AttributeNode attr_node = obj_node;
    while(attr_node = data_next(attr_node))
    {
        xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"name");
        if(!key) continue;
        IDListStore *idsave = g_new0(IDListStore,1);


        idsave->sequence = g_strtod((gchar*)key,NULL);
        key = xmlGetProp(attr_node,(xmlChar *)"addr");
        if(!key)
        {
            idsave->id_addr = idsave->sequence * 2;
        }
        else
            idsave->id_addr = g_strtod((gchar*)key,NULL);
        key = xmlGetProp(attr_node,(xmlChar *)"dname");
        if(!key)
        {
            idsave->id_text = g_strdup("");
        }
        else
            idsave->id_text = g_strdup((gchar*)key);

        sid->idlists = g_list_append(sid->idlists,idsave);

    }
}


void factory_save_idlist_items(ObjectNode obj_node,GList *savelist)
{
    GList *idlist = savelist;
    for(; idlist; idlist = idlist->next)
    {
        IDListStore *idsave =idlist->data;
        ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_item", NULL);
        xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)g_strdup_printf("%d",idsave->sequence));
        xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
        gchar *val = g_strdup("-1");
        DiaObject *diaobj = g_hash_table_lookup(curLayer->defnames,idsave->id_text);
        if(!diaobj)
        {
            g_free(idsave->id_text);
            idsave->id_text = g_strdup("");
//                sit->active = 0;
        }
        else
        {
            val = g_strdup_printf("%d",diaobj->oindex); /* 保存ID号 */
        }

        xmlSetProp(ccc, (const xmlChar *)"value",(xmlChar*)val);
        xmlSetProp(ccc, (const xmlChar *)"addr", (xmlChar *)g_strdup_printf("%d",idsave->id_addr));
        xmlSetProp(ccc, (const xmlChar *)"idname", (xmlChar *)idsave->id_text);
//            xmlSetProp(ccc, (const xmlChar *)"active", (xmlChar *)g_strdup_printf("%d",sit->active));
        g_free(val);
    }
}
