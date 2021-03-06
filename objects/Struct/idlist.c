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


static void factory_append_item_to_idlist_model(GtkListStore *store,
                                                IDListStore *idsave)
{

    GtkTreeIter iter;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set(store,&iter,
                       COLUMN_ITEM_SEQUENCE,idsave->sequence,
                       COLUMN_ITEM_ADDR,idsave->id_addr,
                       COLUMN_ITEM_IDNAME,idsave->id_text,
                       -1);
}


static void factory_append_stritem_to_idlist_model(GtkListStore *store,
                                                   gchar *str)
{
    GtkTreeIter iter;
     gint n = GTK_LIST_STORE(store)->length ;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set(store,&iter,
                       COLUMN_ITEM_SEQUENCE,n,
                       COLUMN_ITEM_ADDR,n*2,
                       COLUMN_ITEM_IDNAME,str,
                       -1);
}


static void factory_add_stritem_to_idlist_model(gpointer user_data)
{
    SaveIdDialog *sid = (SaveIdDialog *)user_data;

}


/* 打开子菜单对话框  */
static gchar* factory_idlist_sublist_dialog()
{
//    GtkWidget *subdig = factory_create_new_dialog_with_buttons(factory_utf8("IDb子列表"),
//                                                               gtk_widget_get_toplevel(button));
//    GtkWidget *dialog_vbox = GTK_DIALOG(subdig)->vbox;
}

static void factory_add_item_to_idlist_model(GtkWidget *btn, gpointer user_data)
{
    SaveIdDialog *sid = (SaveIdDialog *)user_data;
    gint len = GTK_LIST_STORE(sid->id_store)->length ;
//    IDListStore *idsave = g_new0(IDListStore,1);
//    idsave->sequence = len;
//    idsave->id_addr = len*2;
//    idsave->id_text = g_strdup("");

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
    if(0 == rows) return;
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


void factoy_idlist_response(GtkWidget *dlg,
                            gint       response,
                            gpointer   user_data)
{

    if(response == GTK_RESPONSE_OK)
    {
        SaveIdDialog *sid = curLayer->sid;
        SaveStruct *sst = (SaveStruct *)user_data;


        GtkTreeIter iter;
        GList *tlist = sid->idlists;
        gtk_tree_model_foreach(sid->id_store,factory_save_idlist_lastcolumn_foreach,sid->idlists);
        GtkTreeSelection *selection = gtk_tree_view_get_selection (sid->id_treeview);
        if (gtk_tree_selection_get_selected (selection, NULL, &iter))
        {
            GtkTreePath *path;
            path = gtk_tree_model_get_path (sid->id_store, &iter);
            sst->value.vnumber =g_strdup(gtk_tree_path_to_string (path));
            gtk_tree_path_free(path);
        }
        else
        {
            sst->value.vnumber = g_strdup("-1");
        }

        gtk_button_set_label(GTK_BUTTON(sst->widget2),sst->value.vnumber );

    }
    gtk_widget_destroy(dlg);
}


static void factory_idlist_dbclick(GtkTreeView *treeview,
                                   GtkTreePath *path,
                                   GtkTreeViewColumn *col,
                                   gpointer user_data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkWidget  *dialog = gtk_message_dialog_new (NULL, 0,
                         GTK_MESSAGE_ERROR,
                         GTK_BUTTONS_CLOSE,
                         "Failed to read icon file: %s",
                         "teetetet");
    gtk_dialog_run(dialog);
    gtk_widget_destroy(dialog);
}



void
view_popup_menu_onDoSomething (GtkWidget *menuitem, gpointer userdata)
{
    /* we passed the view as userdata when we connected the signal */
    GtkTreeView *treeview = GTK_TREE_VIEW(userdata);

    g_print ("Do something!\n");
}

static gchar *idopt[] = {"添加","插入","删除",NULL};

void factory_add_menuitem_list(GtkWidget *menu)
{

    GtkWidget *menuitem;
    int n = 0;
    do
    {
        if(NULL==idopt[n])
            break;
        menuitem = gtk_menu_item_new_with_label(factory_utf8(idopt[n]));
        g_object_set_data(menu,g_strdup_printf("%d",n),menuitem);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }
    while(++n);
}

void
view_popup_menu (GtkWidget *treeview, GdkEventButton *event,
                 gpointer userdata)
{
    GtkWidget *menu, *menuitem;
    gint n = GPOINTER_TO_INT(userdata);
    menu = gtk_menu_new();
    factory_add_menuitem_list(menu);
    if(n <= 0)
    {
        GtkWidget *wid = g_object_get_data(menu,
                                           g_strdup_printf("%d",1));
        gtk_widget_set_sensitive(wid,FALSE);
        wid = g_object_get_data(menu,
                                           g_strdup_printf("%d",2));
        gtk_widget_set_sensitive(wid,FALSE);
    }
    gtk_widget_show_all(menu);

    /* Note: event can be NULL here when called from view_onPopupMenu;
     *  gdk_event_get_time() accepts a NULL argument */
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
}

static gboolean
tree_view_get_cell_from_pos(GtkTreeView *view, guint x, guint y, GtkCellRenderer **cell)
{
	GtkTreeViewColumn *col = NULL;
	GList             *node, *columns, *cells;
	guint              colx = 0;

	g_return_val_if_fail ( view != NULL, FALSE );
	g_return_val_if_fail ( cell != NULL, FALSE );

	/* (1) find column and column x relative to tree view coordinates */

	columns = gtk_tree_view_get_columns(view);

	for (node = columns;  node != NULL && col == NULL;  node = node->next)
	{
		GtkTreeViewColumn *checkcol = (GtkTreeViewColumn*) node->data;

		if (x >= colx  &&  x < (colx + checkcol->width))
			col = checkcol;
		else
			colx += checkcol->width;
	}

	g_list_free(columns);

	if (col == NULL)
		return FALSE; /* not found */

	/* (2) find the cell renderer within the column */

	cells = gtk_tree_view_column_get_cell_renderers(col);

	for (node = cells;  node != NULL;  node = node->next)
	{
		GtkCellRenderer *checkcell = (GtkCellRenderer*) node->data;
		guint            width = 0, height = 0;

		/* Will this work for all packing modes? doesn't that
		 *  return a random width depending on the last content
		 * rendered? */
		gtk_cell_renderer_get_size(checkcell, GTK_WIDGET(view), NULL, NULL, NULL, &width, NULL);

		if (x >= colx && x < (colx + width))
		{
			*cell = checkcell;
			g_list_free(cells);
			return TRUE;
		}

		colx += width;
	}

	g_list_free(cells);
	return FALSE; /* not found */
}


gboolean factory_idlist_treeview_buttonpress(GtkWidget *treeview,
        GdkEventButton *event,
        gpointer user_data)
{

    if(event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
        GtkCellRenderer *renderer = NULL;
//        if (tree_view_get_cell_from_pos(GTK_TREE_VIEW(treeview),
//                                         event->x, event->y, &renderer))
//        {
//
//        }
//        else
//        {
//            if(event->button)
//            {
//                       GtkTreeSelection *selection;
//            selection = gtk_tree_view_get_selection(
//                            GTK_TREE_VIEW(treeview));
//            gtk_tree_selection_unselect_all(selection);
//            }
//        }


            GtkTreeSelection *selection;
            selection = gtk_tree_view_get_selection(
                            GTK_TREE_VIEW(treeview));
            gint n = gtk_tree_selection_count_selected_rows(selection);

            view_popup_menu(treeview,event,(gpointer)n);

        return TRUE;
    }
    return FALSE;
}

void factory_new_idlist_dialog(GtkWidget *button,SaveStruct *sst)
{
    GtkWidget *mainBox = gtk_vbox_new(FALSE,0);
    SaveIdDialog *sid = (SaveIdDialog *)curLayer->sid;
    GtkWidget *parent = gtk_widget_get_toplevel(button);
    GtkWidget *subdig = factory_create_new_dialog_with_buttons(factory_utf8("ID列表"),
                                                               gtk_widget_get_toplevel(button));
    GtkWidget *dialog_vbox = GTK_DIALOG(subdig)->vbox;
    gtk_window_set_modal(GTK_WINDOW(subdig),TRUE);
    gtk_window_set_resizable (GTK_WINDOW (subdig),TRUE);
    gtk_widget_set_size_request (subdig,300,500);
    gtk_container_add(GTK_CONTAINER(dialog_vbox),mainBox);
    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);

//    GList *p = g_hash_table_get_keys(curLayer->defnames);
//    p = g_list_sort(p,factory_str_compare);
//    sid->flist = factory_get_objects_from_layer(curLayer);

    sid->id_cbmodel = factory_create_idcombox_model(factory_get_objects_from_layer(curLayer)); /* 创建要填充的下拉表的链表 */

    gtk_box_pack_start(GTK_BOX(mainBox),wid_idlist,TRUE,TRUE,0);

    /* 创建IDlist 主界面的模型 */
    sid->id_store = gtk_list_store_new(NUM_OF_COLS,G_TYPE_INT,G_TYPE_INT,G_TYPE_STRING);

    sid->id_treeview = gtk_tree_view_new_with_model (sid->id_store);
    g_signal_connect(sid->id_treeview,
                     "row-activated",G_CALLBACK(factory_idlist_dbclick),
                     NULL);

    g_signal_connect(sid->id_treeview,
                     "button_press_event",
                     G_CALLBACK(factory_idlist_treeview_buttonpress),
                     NULL);
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

            factory_append_item_to_idlist_model(sid->id_store,
                                                tlist->data);
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

    gtk_box_pack_start(GTK_BOX(mainBox),factory_new_add_button(factory_add_item_to_idlist_model,sid),FALSE,FALSE,0);
    gtk_widget_show_all(subdig);
    g_signal_connect(G_OBJECT(subdig),"response",G_CALLBACK(factoy_idlist_response),sst);
//    gint ret = gtk_dialog_run(subdig); /* 阻塞方式运行 */
//    if(ret == GTK_RESPONSE_OK) /* 把判断保存做在这里,可以减变量的数量,与函数之间的传递. */
//    {
//
//    }
//    gtk_widget_destroy(subdig);
}

void factory_save_idlist_to_xml(SaveStruct *sss,ObjectNode obj_node)
{
    ObjectNode ccc = xmlNewChild(obj_node, NULL,
                                 (const xmlChar *)JL_NODE, NULL);
    xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)sss->name);
    xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
    xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)sss->type);

    xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)sss->value.vnumber);

    SaveIdDialog *sid = (SaveIdDialog *)curLayer->sid;
    gchar *idname = g_strdup("");
    if(sid->idlists)
    {
        int pos  = g_strtod(sss->value.vnumber,NULL);
        IDListStore *idsave  = g_list_nth_data(sid->idlists,pos);
        if(idsave)
        {
            idname = g_strdup(idsave->id_text);
        }
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
        key = xmlGetProp(attr_node,(xmlChar *)"idname");
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
        ObjectNode ccc = xmlNewChild(obj_node, NULL,
                                     (const xmlChar *)JL_NODE, NULL);
        xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)g_strdup_printf("%d",idsave->sequence));
        xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
        gchar *val = g_strdup("-1");
        DiaObject *diaobj = g_hash_table_lookup(curLayer->defnames,idsave->id_text);
        if(!diaobj)
        {
            g_free(idsave->id_text);
            idsave->id_text = g_strdup("");
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
