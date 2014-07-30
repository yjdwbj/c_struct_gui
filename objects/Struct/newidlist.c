#include "struct_class.h"
#include "object.h"
#define WIDTH 300
#define HEIGHT 500

static GHashTable *idtable = NULL;
static gchar *subopt[] = {"添加","插入","删除",NULL};

static GQuark idopts[]= {0,0,0,0}; /* 这里其实要对应mfmo的数量的 */

static GtkWidget* factory_sublist_create_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview);
static GtkWidget* factory_sublist_edit_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview);

static GtkWidget* factory_sublist_insert_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview);

static GtkWidget* factory_sublist_delete_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview);

static GroupOfOperation mfmo[] =
{
    {NULL,"添加",NULL,factory_sublist_create_dialog},
    {NULL,"编辑",NULL,factory_sublist_edit_dialog},
    {NULL,"插入",NULL,factory_sublist_insert_dialog},
    {NULL,"删除",NULL,factory_sublist_delete_dialog}

};



static twoWidget stw= {0,0};

static GtkWidget *factory_create_action_list_widget()
{

    GtkTreeModel *act_model;
    GtkTreeView *act_treeview;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);
//    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (sid->id_treeview), TRUE);


    act_model = gtk_list_store_new(1,G_TYPE_STRING);
    act_treeview = gtk_tree_view_new_with_model(act_model);
    gtk_tree_view_set_grid_lines (act_treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);

    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (act_treeview)),
                                 GTK_SELECTION_MULTIPLE|GTK_SELECTION_BROWSE);

    g_object_set_data(wid_idlist,"act_treeview",act_treeview);
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("行为列表"),
             renderer,
             "text",
             0,
             NULL);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (act_treeview, column);
    GList *actlist = factory_get_objects_from_layer(curLayer);
    factory_append_iten_to_cbmodal(act_model,g_quark_to_string(empty_quark));
    for(; actlist; actlist = actlist->next)
    {
        factory_append_iten_to_cbmodal(act_model,actlist->data);
    }
    gtk_container_add (GTK_CONTAINER (wid_idlist), act_treeview);
//    gtk_box_pack_start(GTK_BOX(vbox),sid->id_treeview,FALSE,FALSE,0);
    g_object_unref (act_model);

    return wid_idlist;
}


/*** 这下面就是sublist 函数 ***/

void factory_sublist_append_item_to_model(GtkListStore *store,
        gchar *str)
{
    GtkTreeIter iter;
    gint n  = gtk_tree_model_iter_n_children(store,NULL);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set(store,&iter,
                       COLUMN_ITEM_SEQUENCE,n,
                       COLUMN_ITEM_ADDR,n*2,
                       COLUMN_ITEM_IDNAME,str,
                       -1);
}


GtkTreeIter* factory_sublist_insert_item_to_model(GtkListStore *store,
        gchar *str,
        GtkTreeIter *sibling)
{
    GtkTreeIter iter;
    gint n  = gtk_tree_model_iter_n_children(store,NULL);
    gtk_list_store_insert_before (store,&iter,sibling);
    gtk_list_store_set(store,&iter,
                       COLUMN_ITEM_SEQUENCE,n,
                       COLUMN_ITEM_ADDR,n*2,
                       COLUMN_ITEM_IDNAME,str,
                       -1);
    return &iter;
}

gboolean factory_sublist_revalue_foreach(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    int pos = gtk_tree_path_get_indices(path)[0];
    gtk_list_store_set(GTK_LIST_STORE(model),iter,COLUMN_ITEM_SEQUENCE,pos,-1);
    gtk_list_store_set(GTK_LIST_STORE(model),iter,COLUMN_ITEM_ADDR,pos*2,-1);
    return FALSE;
}

/* 添加行为 */
static void factory_sublist_append_callback(GtkWidget *btn,
        gpointer user_data)
{

    GtkTreeIter iter;
    twoWidget *tw = (twoWidget*)user_data;
    GtkTreeView *act_treeview = g_object_get_data(tw->right,"act_treeview");
    GtkTreeModel *act_model = gtk_tree_view_get_model(act_treeview);


    GtkTreeView *sub_treeview = g_object_get_data(tw->left,"sub_treeview");
    GtkTreeModel *sub_model = gtk_tree_view_get_model(sub_treeview);

    GtkTreeSelection *selection = gtk_tree_view_get_selection (act_treeview);
    int scount = gtk_tree_selection_count_selected_rows (selection);
    if (scount >= 1 )
    {

        GList *treelist = gtk_tree_selection_get_selected_rows (selection,
                          &act_model);
        for(; treelist; treelist = treelist->next)
        {
            gchar *txt = g_strdup("");
            gtk_tree_model_get_iter (act_model,&iter,treelist->data);
            gtk_tree_model_get(act_model,&iter,0,&txt,-1);
            factory_sublist_append_item_to_model(sub_model,txt);
            g_free(txt);
        }
    }

}

static void factory_sublist_insert_callback(GtkWidget *btn,gpointer user_data)
{

    twoWidget *tw = (twoWidget*)user_data;
    GtkTreeView *act_treeview = g_object_get_data(tw->right,"act_treeview");
    GtkTreeModel *act_model = gtk_tree_view_get_model(act_treeview);


    GtkTreeView *sub_treeview = g_object_get_data(tw->left,"sub_treeview");
    GtkTreeModel *sub_model = gtk_tree_view_get_model(sub_treeview);

    GtkTreeSelection *selection = gtk_tree_view_get_selection (act_treeview);
    int scount = gtk_tree_selection_count_selected_rows (selection);
    if (scount >= 1 )
    {
        GtkTreeSelection *subsel = gtk_tree_view_get_selection(sub_treeview);

        GtkTreeIter iter,liter;
        GList *firstsub = gtk_tree_selection_get_selected_rows(subsel,&sub_model);
        if(!firstsub) /* 没有选择一个点 */
            return;

        gtk_tree_model_get_iter (sub_model,&liter,firstsub->data);
        g_list_foreach(firstsub,(GFunc)gtk_tree_path_free,NULL);
        g_list_free(firstsub);

        gtk_tree_selection_select_iter (subsel,&liter);

        GList *snodes = gtk_tree_selection_get_selected_rows (selection,
                        &act_model);
        GList *treelist = snodes;
        for(; treelist; treelist = treelist->next)
        {
            gchar *txt;
            gtk_tree_model_get_iter (act_model,&iter,treelist->data);
            GtkTreeIter siter;
            gtk_tree_model_get(act_model,&iter,0,&txt,-1);
            factory_sublist_insert_item_to_model(sub_model,txt,&liter);
//           gtk_tree_model_iter_next(sub_model,&liter);
            GtkTreePath *path = gtk_tree_model_get_path(sub_model,&liter);
            gtk_tree_path_prev(path);
            gtk_tree_path_free(path);
            g_free(txt);
        }

        g_list_foreach(snodes,(GFunc)gtk_tree_path_free,NULL);
        g_list_free(snodes);

        gtk_tree_model_foreach(sub_model,factory_sublist_revalue_foreach,NULL);
    }
}


static void factory_sublist_delete_callback(GtkWidget *btn,gpointer user_data)
{
    GtkTreeView *sub_treeview = g_object_get_data(user_data,"sub_treeview");
    GtkTreeModel *sub_model = gtk_tree_view_get_model(sub_treeview);

    GtkTreeSelection *selection = gtk_tree_view_get_selection (sub_treeview);
    int scount = gtk_tree_selection_count_selected_rows (selection);
    if (scount >= 1 )
    {
        GList *treelist = gtk_tree_selection_get_selected_rows (selection,
                          &sub_model);
        GList *reflist = NULL;
        GList *slist = treelist;
        for(; slist; slist = slist->next)
        {
            GtkTreeRowReference  *rowref;
            rowref = gtk_tree_row_reference_new(sub_model, slist->data);
            reflist = g_list_append(reflist,rowref);
        }

        g_list_foreach(slist,(GFunc)gtk_tree_path_free,NULL);
        g_list_free(slist);

        GList *rnode = reflist;
        for(; rnode; rnode = rnode->next)
        {
            GtkTreePath *path;
            path = gtk_tree_row_reference_get_path(rnode->data);
            if(path)
            {
                GtkTreeIter iter;
                gtk_tree_model_get_iter (sub_model,&iter,path);
                gtk_list_store_remove (GTK_LIST_STORE(sub_model),&iter);
                gtk_tree_path_free(path);
            }
        }

        g_list_foreach(reflist,(GFunc)gtk_tree_row_reference_free,NULL);
        g_list_free(reflist);
        gtk_tree_model_foreach(sub_model,factory_sublist_revalue_foreach,NULL);
    }

    scount = gtk_tree_selection_count_selected_rows (selection);
    if(scount < 1)
    {
        GtkWidget *bparent = gtk_widget_get_parent(btn);
        gtk_widget_set_sensitive(btn,FALSE);
        GtkWidget *isert = g_object_get_data(G_OBJECT(bparent),"opt_ist");
        gtk_widget_set_sensitive(isert,FALSE);
    }
}


/*中间的操作 */

static GtkWidget *factory_sublist_operators(GtkWidget *left,
        GtkWidget *right)
{
    stw.left = left;
    stw.right = right;
    GtkWidget *vsplit = gtk_vseparator_new();

    GtkWidget *mainBox = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(mainBox),vsplit,TRUE,TRUE,0);

    GtkWidget *btn =
        gtk_button_new_with_label(factory_utf8("添加"));
    g_object_set_data(G_OBJECT(mainBox),"opt_apd",btn);
    gtk_box_pack_start(GTK_BOX(mainBox),btn,FALSE,TRUE,0);

    g_signal_connect(btn,"clicked",
                     G_CALLBACK(factory_sublist_append_callback),&stw);

    btn = gtk_button_new_with_label(factory_utf8("插入"));
    g_object_set_data(G_OBJECT(mainBox),"opt_ist",btn);
    gtk_box_pack_start(GTK_BOX(mainBox),btn,FALSE,TRUE,15);
    g_signal_connect(btn,"clicked",
                     G_CALLBACK(factory_sublist_insert_callback),&stw);
    gtk_widget_set_sensitive(btn,FALSE);

    btn = gtk_button_new_with_label(factory_utf8("删除"));
    g_object_set_data(G_OBJECT(mainBox),"opt_del",btn);
    gtk_box_pack_start(GTK_BOX(mainBox),btn,FALSE,TRUE,0);
    g_signal_connect(btn,"clicked",
                     G_CALLBACK(factory_sublist_delete_callback),left);
    gtk_widget_set_sensitive(btn,FALSE);
    vsplit = gtk_vseparator_new();

    gtk_box_pack_start(GTK_BOX(mainBox),vsplit,TRUE,TRUE,0);
    return mainBox;
}


static void factory_add_sublist_columns (GtkTreeView *treeview,GtkTreeModel *cbmodel)
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
            "fixed-width",
             5,
             NULL);

    gtk_tree_view_append_column (treeview, column);

    /* column for severities */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("地址"),
             renderer,
             "text",
             COLUMN_ITEM_ADDR,
            "fixed-width",
             5,
             NULL);

    gtk_tree_view_append_column (treeview, column);

    /* Combo */

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("行为名称"),
             renderer,
             "text",
             COLUMN_ITEM_IDNAME,
             NULL);
//    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ITEM_ADDR);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);

}


static gboolean factory_sublist_saveitem_foreach(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    subTable *stable = (subTable*)data;
    gchar *txt;
    gtk_tree_model_get(model,iter,COLUMN_ITEM_IDNAME,&txt,-1);
    stable->sub_list = g_list_append(stable->sub_list,
                                       (gpointer)g_quark_from_string(txt));
    g_free(txt);
    return FALSE;
}

static void factory_sublist_dialog_response(GtkWidget *widget,
        int response_id,
        gpointer user_data)
{
    if(response_id == GTK_RESPONSE_OK)
    {
        SaveIdDialog *sid = curLayer->sid;
        GtkTreeView *sub_treeview = g_object_get_data(widget,"sub_treeview");
        GtkTreeModel *sub_model = gtk_tree_view_get_model(sub_treeview);
        gint n  = gtk_tree_model_iter_n_children(sub_model,NULL);
        if(0 == n)
        {
            goto HIDE;
        }

        GtkTreeView *idtreeview = g_object_get_data(G_OBJECT(widget),"idtreeview");
        GtkWidget *entry = g_object_get_data(widget,"table_name");

        if(entry)
        {
            gchar *text = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
            if(0 == strlen(g_strstrip(text)) )
            {
                gchar *msg = factory_utf8("请输入表名称!");
                factory_message_dialoag(widget,msg);
                g_free(msg);
                return;
            }
            GQuark nquark = g_quark_from_string(text);
            GtkMenuItem *item = g_object_get_data(G_OBJECT(widget),"id_opt");
            GList *lplist = sid->idlists;
            subTable *curtable = g_object_get_data(G_OBJECT(item),
                                                   "defstable");
            for(;lplist;lplist = lplist->next)
            {
                subTable *st = lplist->data;
                if(st == curtable) continue;
                if(st->nquark == nquark)
                {
                    gchar *msg = factory_utf8("表名已经存在请更名!");
                    factory_message_dialoag(widget,msg);
                    g_free(msg);
                    return;
                }
            }
            subTable *stable = g_new0(subTable,1);
            stable->nquark = nquark;
            /* 这里用了一个遍历函数保存子表的所有值 */

            gtk_tree_model_foreach(sub_model,factory_sublist_saveitem_foreach,
                                   stable);


            const gchar *opt_lab = gtk_menu_item_get_label(item);
            GtkTreeModel *idmodel = gtk_tree_view_get_model(idtreeview);
            if(!g_ascii_strcasecmp(opt_lab,factory_utf8("添加")))
            {
                /* 这里添加一行 */
                factory_idlist_append_item_to_model(idmodel,text);
                sid->idlists = g_list_append(sid->idlists,stable);
            }
            else if(!g_ascii_strcasecmp(opt_lab,factory_utf8("编辑")))
            {
                GtkTreeIter iter;
                GtkTreeSelection *idsel = gtk_tree_view_get_selection(idtreeview);
                if(gtk_tree_selection_get_selected(idsel,NULL,&iter))
                {
                    /* 改表名*/
                    if(curtable->nquark != nquark)
                    {
                        gtk_list_store_set(idmodel,&iter,COLUMN_IDNAME,text,-1);
                        curtable->nquark = nquark;
                    }

                    /* 改链表的数据 */
                    g_list_free(curtable->sub_list);
                    curtable->sub_list = g_list_copy(stable->sub_list);
                    g_list_free(stable->sub_list);
                    g_free(stable);
                }
            }
            else /* 插入操作 */
            {
                GtkTreeIter iter;
                GtkTreeSelection *idsel = gtk_tree_view_get_selection(idtreeview);
                if(gtk_tree_selection_get_selected(idsel,NULL,&iter))
                {
                    GtkTreePath *path;
                    path = gtk_tree_model_get_path(idmodel,&iter);
                    int pos = gtk_tree_path_get_indices(path)[0];
                    factory_idlist_insert_item_to_model(idmodel,text,&iter);
                    sid->idlists = g_list_insert(sid->idlists,stable,pos);
                    gtk_tree_path_free(path);
                    gtk_tree_model_foreach(idmodel,
                                           factory_idlist_delete_item_update_foreach,NULL);
                }
            }

            g_free(text);
        }
    }
HIDE:
    gtk_widget_hide(widget);
}



/* 编辑一个存在的id 项 */

void factory_sublist_setback_values(GtkWidget *subdlg,
        subTable *stable,Factory_Hash_Table_Lookup func,GHashTable *table)
{
    GtkWidget *entry = g_object_get_data(G_OBJECT(subdlg),"table_name");
    gtk_entry_set_text(GTK_ENTRY(entry),g_quark_to_string(stable->nquark));

    GtkTreeView *subtreeview = g_object_get_data(G_OBJECT(subdlg),
                               "sub_treeview");
    GtkTreeModel *submodel = gtk_tree_view_get_model(subtreeview);


    GList *p = stable->sub_list;
    for(; p ; p = p->next)
    {
        gchar *txt =  g_quark_to_string(p->data);
//        gpointer exist = g_hash_table_lookup(curLayer->defnames,txt);
        gpointer exist = func(table,p->data);
        if(!exist)
        {
            p->data = empty_quark;
            factory_sublist_append_item_to_model(submodel,
                                                 g_quark_to_string(empty_quark));
        }
        else
            factory_sublist_append_item_to_model(submodel,txt);

    }

}

gboolean
factory_subtreeview_onButtonPressed(GtkWidget *treeview, GdkEventButton *event,
                                   gpointer user_data)
{
    /* single click with the right mouse button? */
    if (event->type == GDK_BUTTON_RELEASE  &&  event->button == 1)
    {
        GtkTreeSelection *selection;
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

        /* Note: gtk_tree_selection_count_selected_rows() does not
         *   exist in gtk+-2.0, only in gtk+ >= v2.2 ! */
        int scount = gtk_tree_selection_count_selected_rows(selection);
        if ( scount > 0)
        {
            GtkWidget *mainbox = user_data;
            GtkWidget *btn = g_object_get_data(G_OBJECT(mainbox),"opt_ist");
            gtk_widget_set_sensitive(btn,TRUE);
            btn = g_object_get_data(G_OBJECT(mainbox),"opt_del");
            gtk_widget_set_sensitive(btn,TRUE);
        }
    }

    return FALSE; /* we did not handle this */
}

gchar *factory_get_subtable_name(GList *glist,gint n)
{
    int c = n;
    gchar *txt = g_strdup_printf(factory_utf8("子表_%d"),c);
    do{
        GList *p  = glist;
        GQuark quark = g_quark_from_string(txt);
        for(;p ; p = p->next)
        {
            subTable *stable = p->data;
            if(quark == stable->nquark)
            {
                break;
            }
        }
        if(!p)
            return g_strdup(txt);
        else
        {
            free(txt);
            txt = g_strdup_printf(factory_utf8("子表_%d"),++c);
        }
    }while(1);
    return txt;
}



static GtkWidget* factory_sublist_create_dialog(GtkMenuItem *item,
        GtkTreeView *idtreeview)
{
    GtkWidget *mainBox = gtk_vbox_new(FALSE,0);
    SaveIdDialog *sid = (SaveIdDialog *)curLayer->sid;
//    GtkTreeView *idtreeview = g_object_get_data(G_OBJECT(item),"idtreeview");
    GtkWidget *parent = g_object_get_data(G_OBJECT(idtreeview),"parent_dlg");
    GtkWidget *subdig = factory_create_new_dialog_with_buttons(factory_utf8("ID列表"),
                        parent);
    GtkWidget *dialog_vbox = GTK_DIALOG(subdig)->vbox;
    gtk_window_set_modal(GTK_WINDOW(subdig),TRUE);
    gtk_window_set_resizable (GTK_WINDOW (subdig),TRUE);
    gtk_widget_set_size_request (subdig,600,500);
    gtk_container_add(GTK_CONTAINER(dialog_vbox),mainBox);
    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);
    /* 创建subidlist 编辑界面的模型 */
    GtkTreeModel *sub_model;
    sub_model = gtk_list_store_new(NUM_OF_COLS,G_TYPE_INT,G_TYPE_INT,G_TYPE_STRING);


    GtkTreeView *sub_treeview  = gtk_tree_view_new_with_model (sub_model);
    g_object_set_data(G_OBJECT(subdig),"sub_treeview",sub_treeview);
    g_object_set_data(G_OBJECT(subdig),"idtreeview",idtreeview);
    g_object_set_data(G_OBJECT(wid_idlist),"sub_treeview",sub_treeview);
    /* id_opt 是用来判断添加和插入的 */
    g_object_set_data(G_OBJECT(subdig),"id_opt",item);

    gtk_tree_view_set_grid_lines (sub_treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);
    factory_add_sublist_columns(sub_treeview,sub_model); /* 添加列 */

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (sub_treeview), TRUE);
    gtk_tree_selection_set_mode (
        gtk_tree_view_get_selection (GTK_TREE_VIEW (sub_treeview)),
        GTK_SELECTION_MULTIPLE);
    gtk_container_add (GTK_CONTAINER (wid_idlist),sub_treeview);


    GtkTreeModel *idmodel = gtk_tree_view_get_model(idtreeview);
    gint idcount  = gtk_tree_model_iter_n_children(idmodel,NULL);

    GtkWidget *namehbox = gtk_hbox_new(FALSE,0);
    GtkWidget *label = gtk_label_new(factory_utf8("子表名称:"));
    GtkWidget *entry = gtk_entry_new();

    subTable *stable = g_object_get_data(G_OBJECT(item),"defstable");
    if(stable)
    {
        GList *p = stable->sub_list;
        GList *emptlist = NULL;
        for(; p; p=p->next)
        {
            gchar *txt =  g_quark_to_string(p->data);
            gpointer exist = g_hash_table_lookup(curLayer->defnames,txt);
            if(!exist)
            {
                if(p->data != empty_quark)
                {
                    emptlist = g_list_append(emptlist,txt);
                    p->data = empty_quark;
                }
                factory_sublist_append_item_to_model(sub_model,
                                                     g_quark_to_string(empty_quark));
            }
            else
                factory_sublist_append_item_to_model(sub_model,txt);

        }


        if(emptlist)
        {
            gchar *msg = g_strdup_printf(factory_utf8("下列对像已经删除,现在清空相应的表项!\n%s"),
                                      factory_concat_list_to_string(emptlist,IS_STRING));
            factory_message_dialoag(subdig,msg);
            g_free(msg);
            g_list_free(emptlist);
            emptlist = NULL;
        }

        gtk_entry_set_text(GTK_ENTRY(entry),g_quark_to_string(stable->nquark));
    }
    else
        gtk_entry_set_text(GTK_ENTRY(entry),factory_get_subtable_name(sid->idlists,idcount));



    g_object_set_data(subdig,"table_name",entry);
    gtk_box_pack_start(GTK_BOX(namehbox),label,FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(namehbox),entry,FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(mainBox),namehbox,FALSE,FALSE,0);


    /*第一行的一个水平布局*/
    GtkWidget *act_widget = factory_create_action_list_widget();
    GtkWidget *opt_box = factory_sublist_operators(wid_idlist,act_widget);
    GtkWidget *midhbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(midhbox),wid_idlist,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(midhbox),opt_box,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(midhbox),act_widget ,TRUE,TRUE,0);/* 右边是行为列表 */

    gtk_box_pack_start(GTK_BOX(mainBox),midhbox,TRUE,TRUE,0);

     g_signal_connect(sub_treeview,"button_release_event",
                     G_CALLBACK(factory_subtreeview_onButtonPressed),opt_box);

    /* 绑定一个组快捷键,保存当前的子表 */
    GtkAccelGroup *accgroup = gtk_accel_group_new();
    gint keyval;
    GdkModifierType mods;
    gtk_accelerator_parse("<Control>S",&keyval,&mods);
    GClosure *gcl = g_cclosure_new_swap(factory_accelerator_to_response,
                                        subdig,NULL);
    gtk_accel_group_connect(accgroup,keyval,mods,GTK_ACCEL_VISIBLE,gcl);

    gtk_window_add_accel_group(GTK_WINDOW(subdig),accgroup);
    g_object_unref(accgroup);
    g_signal_connect(G_OBJECT(subdig),"response",
                     G_CALLBACK(factory_sublist_dialog_response),
                     NULL);
    gtk_widget_show_all(subdig);
    return subdig;
}

static gpointer factory_sublist_hash_lookup(GHashTable *table,
                                            GQuark key)
{
    return g_hash_table_lookup(table,g_quark_to_string(key));
}

static GtkWidget* factory_sublist_edit_dialog(GtkMenuItem *item,
        GtkTreeView *id_treeview)
{
    GtkTreeModel *idmodel = gtk_tree_view_get_model(id_treeview);
    GtkTreeSelection *idsel = gtk_tree_view_get_selection(id_treeview);
    GtkTreeIter iter;
    SaveIdDialog *sid = curLayer->sid;

    if(gtk_tree_selection_get_selected (idsel,NULL,&iter))
    {
        GtkTreePath *path;
        path = gtk_tree_model_get_path(idmodel,&iter);
//        gtk_tree_model_get(idmodel,&iter,COLUMN_IDNAME,&txt,-1);
        int pos = gtk_tree_path_get_indices(path)[0]; /* 用下标法找到这一项编辑 */
        subTable *stable = g_list_nth_data(sid->idlists,pos);
        gtk_tree_path_free (path);
        g_object_set_data(G_OBJECT(item),"defstable",stable);
        factory_sublist_create_dialog(item,id_treeview);
    }
    return NULL;
}

gboolean factory_idlist_delete_item_update_foreach(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    int pos = gtk_tree_path_get_indices(path)[0];
    gtk_list_store_set(model,iter,COLUMN_IDSEQ,pos,-1);
    return FALSE;
}

/* 删除一个id项 */
static GtkWidget* factory_sublist_delete_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview)
{
    GtkTreeModel *idmodel = gtk_tree_view_get_model(ptreeview);
    GtkTreeSelection *idsel = gtk_tree_view_get_selection(ptreeview);
    GtkTreeIter iter;
    SaveIdDialog *sid = curLayer->sid;

    if(gtk_tree_selection_get_selected (idsel,NULL,&iter))
    {
        GtkTreePath *path;
        path = gtk_tree_model_get_path(idmodel,&iter);
        int pos = gtk_tree_path_get_indices(path)[0]; /* 用下标法找到这一项编辑 */
        subTable *stable = g_list_nth_data(sid->idlists,pos);
        sid->idlists = g_list_remove(sid->idlists,stable);
        g_list_free1(stable->sub_list);
        g_free(stable);
        gtk_tree_path_free (path);
        gtk_list_store_remove(idmodel,&iter);
        gtk_tree_model_foreach(idmodel,
                               factory_idlist_delete_item_update_foreach,NULL);
    }
    return NULL;
}

static GtkWidget* factory_sublist_insert_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview)
{
    GtkTreeModel *idmodel = gtk_tree_view_get_model(ptreeview);
    GtkTreeSelection *idsel = gtk_tree_view_get_selection(ptreeview);
    GtkTreeIter iter;
    SaveIdDialog *sid = curLayer->sid;
    GtkWidget *subdlg = factory_sublist_create_dialog(item,ptreeview);


    return NULL;
}



/***           下面是idlist的函数                ****/


//static void factory_idlist_after_delete_update(const gchar *name)
//{
//    SaveIdDialog *sid = curLayer->sid;
//    GList *glist = sid->idlists;
//    for(; glist ; glist = glist->next)
//    {
//        subTable *stable = glist->data;
//        GList *sublist = stable->table_list;
////        for(;sublist;)
//    }
//}


void factory_set_idlist_columns (GtkTreeView *treeview,GtkTreeModel *cbmodel)
{

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore *modal  = gtk_tree_view_get_model(treeview);

    /* column for bug numbers */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("序号"),
             renderer,
             "text",
             COLUMN_IDSEQ,
            "fixed-width",
             5,
             NULL);

    gtk_tree_view_append_column (treeview, column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("表名称"),
             renderer,
             "text",
             COLUMN_IDNAME,
             NULL);
    gtk_tree_view_append_column (treeview, column);

}


/* 右键菜单操作 */
static gboolean factory_mfile_idlist_popumenu(GtkTreeView *treeview,
                                 GdkEventButton *event,
                                 gpointer user_data)
{
    GtkWidget *menuitem,*menu;
    menu = gtk_menu_new();
    int num =sizeof(mfmo)/sizeof(GroupOfOperation);
    int n = 0;
    for(; n < num; n++)
    {
        menuitem = gtk_menu_item_new_with_label(factory_utf8(mfmo[n].name));
        g_signal_connect(menuitem, "activate",
                         G_CALLBACK(mfmo[n].mfmf),(gpointer)treeview);
        mfmo[n].btn = menuitem;
        g_object_set_data(menu,g_strdup_printf("%d_item",n),menuitem);
        g_object_set_data(G_OBJECT(menuitem),"idtreeview",treeview);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }

    gint sel = GPOINTER_TO_INT(user_data);
    if(sel == 0)
    {
        /* 使 编辑,插入,删除变灰 */
        GtkWidget *edit = g_object_get_data(G_OBJECT(menu),"1_item");
        gtk_widget_set_sensitive(edit,FALSE);
        edit = g_object_get_data(G_OBJECT(menu),"2_item");
        gtk_widget_set_sensitive(edit,FALSE);
        edit = g_object_get_data(G_OBJECT(menu),"3_item");
        gtk_widget_set_sensitive(edit,FALSE);
    }
    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
    return TRUE;
}

gboolean
factory_idtreeview_onButtonPressed(GtkWidget *treeview, GdkEventButton *event,
                                   gpointer userdata)
{
    /* single click with the right mouse button? */
    if (event->type == GDK_BUTTON_RELEASE  &&  event->button == 3)
    {
        GtkTreeSelection *selection;
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
        /* Note: gtk_tree_selection_count_selected_rows() does not
         *   exist in gtk+-2.0, only in gtk+ >= v2.2 ! */
        int scount = gtk_tree_selection_count_selected_rows(selection);
        factory_mfile_idlist_popumenu(treeview, event, (gpointer)scount);


    }

    return FALSE; /* we did not handle this */
}



/* 这里是按钮的实现方式 */
/* 这里是按钮的实现方式 */
//GtkWidget *factory_idlist_operators(gpointer user_data)
//{
//    /* 要添加操作选项，在这里修改　*/
//
//    GroupOfOperation mfmo[] =
//    {
//        {NULL,factory_utf8("添加"),NULL,NULL},
//        {NULL,factory_utf8("编辑"),NULL,NULL},
//        {NULL,factory_utf8("插入"),NULL,NULL},
//        {NULL,factory_utf8("删除"),NULL,NULL}
//
//    };
//
//    GtkWidget *operatorhbox = gtk_hbox_new(FALSE,2);
//
//    int num =sizeof(mfmo)/sizeof(GroupOfOperation);
//    int n = 0;
//    for(; n < num; n++)
//    {
//        GtkWidget *wid = NULL;
//        if(!mfmo[n].stack)
//            wid = gtk_button_new_with_label(mfmo[n].name);
//        else
//            wid = gtk_button_new_from_stock(mfmo[n].stack);
//        g_signal_connect(wid,"clicked",G_CALLBACK(mfmo[n].mfmf),user_data);
//        mfmo[n].btn = wid;
//        gtk_box_pack_start(GTK_BOX(operatorhbox),wid,FALSE,FALSE,0);
//    }
//
//    return operatorhbox;
//}


void factory_idlist_read_xml(ObjectNode obj_node)
{

    SaveIdDialog *sid = (SaveIdDialog *)curLayer->sid;
    ObjectNode idnode =  obj_node->xmlChildrenNode;
    xmlChar *key;
    GList *glist = NULL;
    /* 这里先读取全部列表,后再切分. */
    ObjectNode lstnode = factory_find_custom_node(obj_node,IDLIST_NODE);
    if(lstnode)
    {
        ObjectNode cnode = lstnode->xmlChildrenNode;
         while(cnode = data_next(cnode))
         {
             key = xmlGetProp(cnode,"idname");
             if(key)
                glist = g_list_append(glist,(char*)key);
         }
    }

    ObjectNode idxnode = factory_find_custom_node(obj_node,IDINDEX_NODE);
    if(idxnode)
    {
         GList *cglist = glist;
         ObjectNode cnode = idxnode->xmlChildrenNode;
         while(cnode = data_next(cnode))
         {
             subTable *stable = g_new0(subTable,1);
             key = xmlGetProp(cnode,"name");
             if(key)
             {
                 stable->nquark = g_quark_from_string((gchar*)key);
                 xmlFree(key);
             }
             key = xmlGetProp(cnode,"rows");
             if(key)
             {
                 gint len = g_strtod(key,NULL);
                 int n = 0;
                 for( ;n < len; n++,cglist = cglist->next)
                 {
                     stable->sub_list =
                     g_list_append(stable->sub_list,
                                   g_quark_from_string(cglist->data));
                 }
                 xmlFree(key);
             }
             sid->idlists = g_list_append(sid->idlists ,stable);
         }

    }

    g_list_foreach(glist,(GFunc)g_free,NULL);
    g_list_free(glist);

}


void factory_idlist_save_to_old_xml(ObjectNode ccc,GList *glist)
{
    GList *idlist = glist;
    int pos = 0;
    for(; idlist; idlist = idlist->next,pos++)
    {
        gchar  *txt  = g_quark_to_string(idlist->data);
        ObjectNode idnode = xmlNewChild(ccc, NULL,
                                         (const xmlChar *)JL_NODE, NULL);
        xmlSetProp(idnode, (const xmlChar *)"name", (xmlChar *)g_strdup_printf("%d",pos));
        xmlSetProp(idnode, (const xmlChar *)"type", (xmlChar *)"u16");
        gchar *val = g_strdup("-1");
        DiaObject *diaobj = g_hash_table_lookup(curLayer->defnames,txt);
        if(!diaobj)
        {

        }
        else
        {
            val = g_strdup_printf("%d",diaobj->oindex); /* 保存ID号 */
        }

        xmlSetProp(idnode, (const xmlChar *)"value",(xmlChar*)val);
        xmlSetProp(idnode, (const xmlChar *)"addr", (xmlChar *)g_strdup_printf("%d",pos*2));
        xmlSetProp(idnode, (const xmlChar *)"idname", (xmlChar *)txt);
//            xmlSetProp(ccc, (const xmlChar *)"active", (xmlChar *)g_strdup_printf("%d",sit->active));
        g_free(val);
    }
}


void factory_idlist_save_to_xml(ObjectNode obj_node,GList *savelist)
{
    ObjectNode ccc = xmlNewChild(obj_node, NULL,
                                 (const xmlChar *)IDINDEX_NODE, NULL);
    xmlSetProp(ccc, (const xmlChar *)"rows",
               (xmlChar *)g_strdup_printf("%d",g_list_length(savelist)));
    GList *rootlist = savelist;
    GList *glist = NULL;
    for(; rootlist; rootlist = rootlist->next)
    {
        subTable *stable = rootlist->data;
        ObjectNode idnode = xmlNewChild(ccc,NULL,(const xmlChar *)JL_NODE,NULL);
        xmlSetProp(idnode, (const xmlChar *)"name",
                   (xmlChar *)g_quark_to_string(stable->nquark));
        xmlSetProp(idnode, (const xmlChar *)"rows",
                   (xmlChar *)g_strdup_printf("%d",g_list_length(stable->sub_list)));

        glist = g_list_concat(glist,g_list_copy(stable->sub_list));
    }

    ccc = xmlNewChild(obj_node, NULL,
                      (const xmlChar *)IDLIST_NODE, NULL);
    xmlSetProp(ccc, (const xmlChar *)"rows",
               (xmlChar *)g_strdup_printf("%d",g_list_length(glist)));
    factory_idlist_save_to_old_xml(ccc,glist);
    g_list_free(glist);
}


void factory_idlist_item_save_to_xml(SaveStruct *sss,ObjectNode obj_node)
{
    ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)JL_NODE, NULL);
    xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)sss->name);
    xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
    xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)sss->type);
    gint vnumber = g_strtod(sss->value.vnumber,NULL);
    gchar *idname = g_strdup("");
    SaveIdDialog *sid = curLayer->sid;
    if(sid->idlists)
    {       /*重新映射到全局位置*/
            GArray *garray = g_array_new(FALSE,FALSE,sizeof(gint));
            GList *looplist = sid->idlists;
            int sum = 0;
            for(; looplist ; looplist = looplist->next)
            {
                subTable *stable = looplist->data;
                int len = g_list_length(stable->sub_list);
                g_array_append_val(garray,sum);
                sum += len;
            }
             subTable *stable  = g_list_nth_data(sid->idlists,vnumber);
            if(stable)
            {
                idname = g_strdup(g_quark_to_string(stable->nquark));
            }
            int np = g_array_index(garray,gint,vnumber);
            vnumber = np;
            g_array_free(garray,TRUE);
    }
    xmlSetProp(ccc, (const xmlChar *)"value",
               (xmlChar *)g_strdup_printf("%d",vnumber));

    xmlSetProp(ccc, (const xmlChar *)"idname", (xmlChar *)idname);
    xmlSetProp(ccc, (const xmlChar *)"org_val", (xmlChar *)sss->value.vnumber);
    g_free(idname);
}


static void factory_idlist_dialog_response(GtkWidget *widget,int response_id,
        gpointer user_data)
{
    SaveStruct *sst = (SaveStruct*)user_data;
    if(response_id == GTK_RESPONSE_OK)
    {
        SaveIdDialog *sid = curLayer->sid;
        GtkTreeView *idtreeview = g_object_get_data(G_OBJECT(widget),
                                  "idtreeview");
        GtkTreeModel *idmodel = gtk_tree_view_get_model(idtreeview);
        GtkTreeSelection *idsel = gtk_tree_view_get_selection(idtreeview);
        GtkTreeIter iter;
        g_free(sst->value.vnumber);
        if(gtk_tree_selection_get_selected(idsel,NULL,&iter))
        {
            GtkTreePath *path;
            path = gtk_tree_model_get_path(idmodel,&iter);
            gint pos = gtk_tree_path_get_indices(path)[0];
            sst->value.vnumber = g_strdup_printf("%d",pos);
        }
        else
        {
            sst->value.vnumber = g_strdup("-1");
        }
        gtk_button_set_label(GTK_BUTTON(sst->widget2),sst->value.vnumber);
    }
    gtk_widget_destroy(widget);
}

void factory_idlist_append_item_to_model(GtkListStore *store,const gchar *str)
{
    GtkTreeIter iter;
    gint n  = gtk_tree_model_iter_n_children(store,NULL);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set(store,&iter,
                       COLUMN_IDSEQ,n,COLUMN_IDNAME,str,
                       -1);
}


void factory_idlist_insert_item_to_model(GtkListStore *store,
        gchar *str,
        GtkTreeIter *sibling)
{
    GtkTreeIter iter;
    gint n  = gtk_tree_model_iter_n_children(store,NULL);
    gtk_list_store_insert_before (store,&iter,sibling);
    gtk_list_store_set(store,&iter,
                       COLUMN_IDSEQ,n,
                       COLUMN_IDNAME,str,
                       -1);
}

void factory_idlist_create_dialog(GtkWidget *button,SaveStruct *sst)
{
    GtkWidget *mainBox = gtk_vbox_new(FALSE,0);
    SaveIdDialog *sid = (SaveIdDialog *)curLayer->sid;
    GtkWidget *parent = gtk_widget_get_toplevel(button);
    GtkWidget *subdig = factory_create_new_dialog_with_buttons(factory_utf8("ID列表"),
                        gtk_widget_get_toplevel(button));
    GtkWidget *dialog_vbox = GTK_DIALOG(subdig)->vbox;
    gtk_window_set_modal(GTK_WINDOW(subdig),TRUE);
    gtk_window_set_resizable (GTK_WINDOW (subdig),TRUE);
    gtk_widget_set_size_request (subdig,WIDTH,HEIGHT);
    gtk_container_add(GTK_CONTAINER(dialog_vbox),mainBox);
    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);


    GtkListStore *idmodel;
    idmodel = gtk_list_store_new(NUM_OF_IDLIST,G_TYPE_INT,G_TYPE_STRING);
    GtkTreeView *idtreeview  = gtk_tree_view_new_with_model(
                                   GTK_TREE_MODEL(idmodel));
    GList *exists = sid->idlists; /*把原来的值设置回来*/
    for(; exists; exists = exists->next)
    {
        subTable *s1 = exists->data;
        factory_idlist_append_item_to_model(idmodel,
                                            g_quark_to_string(s1->nquark));
    }
    /* 先中上一次的结果  */
    GtkTreePath *path = gtk_tree_path_new_from_string(sst->value.vnumber);
    gtk_tree_selection_select_path(gtk_tree_view_get_selection(idtreeview),path);
    gtk_tree_path_free(path);

    g_object_set_data(G_OBJECT(subdig),"idtreeview",idtreeview);
    g_object_set_data(G_OBJECT(wid_idlist),"idtreeview",idtreeview);

    /* 这个数据是用来指定上级窗口 */
    g_object_set_data(G_OBJECT(idtreeview),"parent_dlg",subdig);
    g_signal_connect(idtreeview,"button_release_event",
                     G_CALLBACK(factory_idtreeview_onButtonPressed),NULL);
    gtk_tree_view_set_grid_lines (idtreeview,GTK_TREE_VIEW_GRID_LINES_BOTH);

    factory_set_idlist_columns(idtreeview,GTK_TREE_MODEL(idmodel)); /* 添加列 */

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (idtreeview), TRUE);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (idtreeview)),
                                 GTK_SELECTION_SINGLE);
    gtk_container_add (GTK_CONTAINER (wid_idlist),GTK_WIDGET(idtreeview));
    g_object_unref (idmodel);

    gtk_box_pack_start(GTK_BOX(mainBox),wid_idlist,TRUE,TRUE,0);

    gtk_widget_show_all(subdig);
    g_signal_connect(G_OBJECT(subdig),"response",
                     G_CALLBACK(factory_idlist_dialog_response),sst);

}
