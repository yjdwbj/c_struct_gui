#include "musicfilemanager.h"


#define WIDTH 300
#define HEIGHT 500
const gchar *chvaild = "0123456789abcdefghijklmnopqrstuvwxyz_.";

extern FactoryStructItemAll *factoryContainer;


static GtkWidget* factory_mfile_sublist_create_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview);
static GtkWidget* factory_mfile_sublist_edit_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview);
static GtkWidget* factory_mfile_sublist_insert_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview);
static GtkWidget* factory_mfile_sublist_delete_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview);

static GtkWidget* factory_mfile_mlist_delete_operator(GtkWidget *btn,
        gpointer user_data);

static GtkWidget* factory_mfile_mlist_unselect(GtkWidget *btn,gpointer user_data);
static gboolean factory_mfile_mlist_insert_foreach(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data);

static twoWidget stw= {0,0};


static GQuark add_quark = 0;
static GQuark insert_quark =0;

static GroupOfOperation menuitems[] =
{
    {NULL,"添加",NULL,factory_mfile_sublist_create_dialog},
    {NULL,"编辑",NULL,factory_mfile_sublist_edit_dialog},
    {NULL,"插入",NULL,factory_mfile_sublist_insert_dialog},
    {NULL,"删除",NULL,factory_mfile_sublist_delete_dialog}

};


GroupOfOperation mflistopt[] =
{
    {NULL,"添加",NULL,factory_open_file_dialog},
    {NULL,"插入",NULL,factory_open_file_dialog},
    {NULL,"删除",NULL,factory_mfile_mlist_delete_operator},
    {NULL,"不选",NULL,factory_mfile_mlist_unselect}
//        {NULL,factory_utf8("清空"),NULL,factory_cleanall_mfile_modal},

};

//static GroupOfOperation sublist_operators[] =
//{
//    {NULL,"添加",NULL,factory_mfile_sublist_append_callback},
//    {NULL,"插入",NULL,factory_mfile_sublist_insert_callback},
//    {NULL,"删除",NULL,factory_mfile_sublist_delete_callback}
//}

static gpointer factory_mfile_sublist_hash_lookup(GHashTable *table,
        GQuark key)
{
    return g_hash_table_lookup(table,key);
}

static GtkWidget* factory_mfile_sublist_edit_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview)
{
    GtkTreeModel *idmodel = gtk_tree_view_get_model(ptreeview);
    GtkTreeSelection *idsel = gtk_tree_view_get_selection(ptreeview);
    GtkTreeIter iter;

    SaveMusicDialog *smd = curLayer->smd;
    if(gtk_tree_selection_get_selected (idsel,NULL,&iter))
    {
        GtkTreePath *path;
        path = gtk_tree_model_get_path(idmodel,&iter);
        int pos = gtk_tree_path_get_indices(path)[0];
        subTable *stable = g_list_nth_data(smd->midlists,pos);
        gtk_tree_path_free (path);
        GtkWidget *subdlg = factory_mfile_sublist_create_dialog(item,ptreeview);
        factory_sublist_setback_values(subdlg,stable,
                                       factory_mfile_sublist_hash_lookup,
                                       smd->mtable);
    }
    return NULL;


}

static GtkWidget* factory_mfile_sublist_insert_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview)
{

     GtkTreeModel *idmodel = gtk_tree_view_get_model(ptreeview);
    GtkTreeSelection *idsel = gtk_tree_view_get_selection(ptreeview);
    GtkTreeIter iter;

    GtkWidget *subdlg = factory_mfile_sublist_create_dialog(item,ptreeview);
    gtk_tree_model_foreach(idmodel,
                            factory_idlist_delete_item_update_foreach,NULL);

    return NULL;

}

static GtkWidget* factory_mfile_sublist_delete_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview)
{
    GtkTreeModel *idmodel = gtk_tree_view_get_model(ptreeview);
    GtkTreeSelection *idsel = gtk_tree_view_get_selection(ptreeview);
    GtkTreeIter iter;

    SaveMusicDialog *smd = curLayer->smd;
    if(gtk_tree_selection_get_selected (idsel,NULL,&iter))
    {
        GtkTreePath *path;
        path = gtk_tree_model_get_path(idmodel,&iter);
        int pos = gtk_tree_path_get_indices(path)[0];
        subTable *stable = g_list_nth_data(smd->midlists,pos);
        gtk_tree_path_free (path);
        g_list_remove(smd->midlists,stable);
        gtk_list_store_remove(GTK_LIST_STORE(idmodel),&iter);
        gtk_tree_model_foreach(idmodel,
                               factory_idlist_delete_item_update_foreach,NULL);
    }
    return NULL;
}

static GtkWidget* factory_mfile_mlist_unselect(GtkWidget *widget,
        gpointer user_data)
{
    GtkTreeView *mtreeview = user_data;
    GtkTreeModel *m_model = gtk_tree_view_get_model(mtreeview);
    GtkTreeSelection *msel = gtk_tree_view_get_selection(mtreeview);
    gtk_tree_selection_unselect_all(msel);
    GtkWidget *mainbox = gtk_widget_get_parent(widget);
    gtk_widget_set_sensitive(widget, FALSE);
    GtkWidget *btn = g_object_get_data(G_OBJECT(mainbox),"btn_1");
    gtk_widget_set_sensitive(btn, FALSE);
    btn = g_object_get_data(G_OBJECT(mainbox),"btn_2");
    gtk_widget_set_sensitive(btn, FALSE);
    return NULL;
}


/* 删除音乐文件 */
static GtkWidget* factory_mfile_mlist_delete_operator(GtkWidget *btn,
        gpointer user_data)
{
    SaveMusicDialog *smd = curLayer->smd;
    GtkTreeView *mtreeview = user_data;
    GtkTreeModel *m_model = gtk_tree_view_get_model(mtreeview);
    GtkTreeSelection *msel = gtk_tree_view_get_selection(mtreeview);
    /* 第一行空行不能删除 */

    int scount = gtk_tree_selection_count_selected_rows (msel);
    if (scount >= 1 )
    {
        GList *treelist = gtk_tree_selection_get_selected_rows (msel,
                          NULL);
        GList *reflist = NULL;
        GList *slist = treelist;
        for(; slist; slist = slist->next)
        {
            GtkTreeRowReference  *rowref;
            rowref = gtk_tree_row_reference_new(m_model, slist->data);
            gint pos = gtk_tree_path_get_indices(slist->data)[0];
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
                gtk_tree_model_get_iter (m_model,&iter,path);
                gchar *bscname ;
                gtk_tree_model_get(m_model,&iter,COLUMN_DNAME,
                                   &bscname,-1);
                g_hash_table_remove(smd->mtable,g_quark_from_string(bscname));
                gtk_list_store_remove (GTK_LIST_STORE(m_model),&iter);
                gtk_tree_path_free(path);
            }
        }

        g_list_foreach(reflist,(GFunc)gtk_tree_row_reference_free,NULL);
        g_list_free(reflist);

        /*删除之后要重新建立链顺序*/
        g_list_free(smd->mflist);
        smd->mflist = NULL;
        gtk_tree_model_foreach(m_model,
                               factory_mfile_mlist_insert_foreach,NULL);
    }
    scount = gtk_tree_selection_count_selected_rows (msel);
    GtkWidget *bparent = gtk_widget_get_parent(btn);
    gtk_widget_set_sensitive(btn,scount < 1 ? FALSE : TRUE);
    GtkWidget *isert = g_object_get_data(G_OBJECT(bparent),"btn_1");
    gtk_widget_set_sensitive(isert,scount < 1 ? FALSE : TRUE);
}


//static void factory_add_file_manager_item(GtkWidget *widget,
//        gpointer user_data)
//{
//    SaveMusicDialog *smd = user_data;
//    smd->smfm->man_opt = OPT_APPEND;
//    factory_open_file_dialog(widget,user_data);
//}
//
//void factory_insert_file_manager_item(GtkWidget *widget,gpointer user_data)
//{
//    SaveMusicDialog *smd = user_data;
//    smd->smfm->man_opt = OPT_INSERT;
//    factory_open_file_dialog(NULL,user_data);
//}
//
//
//static void factory_unselect_mfile_modal(GtkWidget *widget,gpointer user_data)
//{
//    SaveMusicDialog *smd = ( SaveMusicDialog *)user_data;
//    GtkTreeSelection *selection = gtk_tree_view_get_selection (smd->smfm->wid_treeview);
//    gtk_tree_selection_unselect_all (selection);
//}

static void factory_mfile_manager_add_columns (GtkTreeView *treeview)
{

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    g_assert(model);

    /* column for bug numbers */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("序号"),
             renderer,
             "text",
             COLUMN_SEQUENCE,
             NULL);
//    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ITEM_SEQUENCE);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);

    /* column for severities */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("物理地址"),
             renderer,
             "text",
             COLUMN_PHY,
             NULL);
//    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ITEM_ADDR);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);

    /* Combo */

    renderer = renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("源文件名"),
             renderer,
             "text",
             COLUMN_FNAME,
             NULL);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);


    renderer = renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("下载文件名(双击重命名)"),
             renderer,
             "text",
             COLUMN_DNAME,
             NULL);
    g_object_set (renderer,
                  "editable", TRUE,
                  NULL);
    g_signal_connect (renderer, "edited",
                      G_CALLBACK (factory_mfile_manager_changed_dname), model);
    g_object_set_data (G_OBJECT (renderer), "column",
                       GINT_TO_POINTER (COLUMN_DNAME));

    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);
}

/* 音乐文件的添加与删除 */
GtkWidget *factory_music_file_manager_operator(GtkTreeView *mtreeview)
{
    /* 要添加操作选项，在这里修改　*/
    GtkWidget *dlg_parent = g_object_get_data(G_OBJECT(mtreeview),"dlg_parent");

    add_quark = g_quark_from_static_string(factory_utf8("添加"));
    insert_quark = g_quark_from_string(factory_utf8("插入"));
    GtkWidget *operatorhbox = gtk_hbox_new(FALSE,2);

    int num =sizeof(mflistopt)/sizeof(GroupOfOperation);
    int n = 0;
    for(; n < num; n++)
    {
        GtkWidget *wid = NULL;
        if(!mflistopt[n].stack)
            wid = gtk_button_new_with_label(factory_utf8(mflistopt[n].name));
        else
            wid = gtk_button_new_from_stock(mflistopt[n].stack);
        g_object_set_data(G_OBJECT(wid),"dlg_parent",dlg_parent);
        g_signal_connect(wid,"clicked",G_CALLBACK(mflistopt[n].mfmf),mtreeview);
        mflistopt[n].btn = wid;
        g_object_set_data(G_OBJECT(operatorhbox),
                          g_strdup_printf("btn_%d",n),wid);
        gtk_widget_set_sensitive(wid,FALSE);
        gtk_box_pack_start(GTK_BOX(operatorhbox),wid,FALSE,FALSE,0);
    }

    return operatorhbox;
}

static gboolean factory_mfile_mlist_ckdup_foreach(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    int pos = gtk_tree_path_get_indices(path)[0];
    gtk_list_store_set(GTK_LIST_STORE(model),iter,COLUMN_ITEM_SEQUENCE,pos,-1);
    gtk_list_store_set(GTK_LIST_STORE(model),iter,COLUMN_ITEM_ADDR,pos*2,-1);
    return FALSE;
}


static gboolean factory_is_have_chinese_code(gchar *str)
{
    int n = strlen(str);
    int ix = 0;
    int chlen = strlen(chvaild);

    for(; ix < n; ix++)
    {
        char *ptr = chvaild;
        do
        {
            if(str[ix] == *ptr)
            {
                break;
            }
        }
        while(ptr++);

        if((ptr - chvaild) > chlen)
        {
            return TRUE;
        }

    }
    return FALSE;
}


static void factory_mfile_manager_changed_dname(GtkCellRendererText *cell,
        const gchar         *path_string,
        const gchar         *new_text,
        gpointer             data)
{
    int pos = g_strtod(path_string,NULL);
    if(0 == strlen(new_text) ||
            factory_is_have_chinese_code(new_text))
    {
        gchar *msg = g_strdup_printf(factory_utf8("请输入有效下载名字!\n有效字符:\n%s\n"),
                                     chvaild);
        factory_message_dialoag(ddisplay_active()->shell,msg);
        g_free(msg);
        return;
    }
    SaveMusicDialog *smd = curLayer->smd;
    GList *mlst = smd->mflist;


    for(; mlst; mlst = mlst->next)
    {
        SaveMusicFile *smf = mlst->data;
        if(pos == g_list_index(smd->mflist,smf)) continue;
        if( !g_ascii_strcasecmp(smf->down_name,new_text))
        {
            gchar *msg = factory_utf8("名字重复!");
            factory_message_dialoag(ddisplay_active()->shell,msg);
            g_free(msg);
            return;
        }
    }

    GtkTreeModel *model = (GtkTreeModel *)data;
    GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
    GtkTreeIter iter;
    gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell),
                                   "column"));
    gtk_tree_model_get_iter (model, &iter, path);
    if( column == COLUMN_DNAME)
    {
        gchar *old_text;
        gtk_tree_model_get (model, &iter, column, &old_text, -1);
        g_free (old_text);
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                            new_text, -1);
    }
    gtk_tree_path_free (path);
}


static void factory_mfile_selection_row(GtkTreeView *treeview,gint row)
{

    GtkTreePath *path;
    GtkTreeIter iter;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    path = gtk_tree_path_new_from_string(g_strdup_printf("%d",row));
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
    gtk_tree_selection_select_iter (selection,&iter);
    gtk_tree_path_free(path);
}

//static void factory_file_manager_refresh_all_iter(SaveMusicDialog *smd)
//{
//    SaveMusicFileMan *smfm = smd->smfm;
//    GtkTreeModel *model = gtk_tree_view_get_model(smfm->wid_treeview);
//    gtk_list_store_clear(model);
//    GList *tlist = smfm->filelist;
//    int num = 0;
//    for(; tlist ; tlist = tlist->next,num++)
//    {
//        SaveMusicFile *smf = tlist->data;
//        smf->index = num;
//        smf->file_addr = smf->index + smfm->offset;
//        factory_mfile_manager_append_iter(model,tlist->data);
//    }
//    smd->mfmos->m_itemadded(smd);
//}

//static gboolean factory_check_mfile_idlist_lastitem(GtkTreeModel *model,
//        GtkTreePath *path,
//        GtkTreeIter *iter,
//        gpointer data)
//{
//    gchar *item = (gchar *)data;
//    gchar *str ;
//    gtk_tree_model_get(model,iter,COLUMN_ITEM_IDNAME,&str,-1);
//    if(!g_strcasecmp(item,str))
//    {
//        gtk_list_store_set(model,iter,COLUMN_ITEM_IDNAME,"",-1);
//    }
//    g_free(str);
//    return FALSE;
//}

//static void factory_delete_mfile_item_modal(GtkWidget *btn,gpointer user_data)
//{
//    SaveMusicDialog *smd = (SaveMusicDialog *)user_data;
//    SaveMusicFileMan *smfm = smd->smfm;
//    GtkTreeView *treeview = smd->smfm->wid_treeview;
//    GtkTreeModel *model = gtk_tree_view_get_model(smd->smfm->wid_treeview);
//    GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
//    GtkTreeIter iter;
//    gchar *item ;
//    gint rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL);
//    if(0 == rows) return;
//
//    int pos;
//    if (gtk_tree_selection_get_selected (selection, NULL, &iter))
//    {
//        gtk_list_store_remove(selection,&iter);
//        gtk_tree_model_get(model,&iter,0,&pos,-1);
//        gtk_tree_model_get(model,&iter,COLUMN_FNAME,&item,-1); /* 删掉名字 */
//    }
//
//    const gpointer  itm =  g_list_nth_data(smfm->filelist,pos);
//    const gpointer c = g_list_nth_data(smd->cboxlist,pos);
//    smd->cboxlist = g_list_remove(smd->cboxlist,c);
//    smfm->filelist = g_list_remove(smfm->filelist,itm);
//
//    gtk_tree_model_foreach(smd->id_store,factory_check_mfile_idlist_lastitem,item);/* 如果左边已经选择,这里要找到它删掉 */
//    g_free(item);
//    factory_file_manager_refresh_all_iter(smd);
//}

//static gboolean factory_clean_idlist_dname(GtkTreeModel *model,
//        GtkTreePath *path,
//        GtkTreeIter *iter,
//        gpointer data)
//{
//    gtk_list_store_set(model,iter,COLUMN_ITEM_IDNAME,"",-1);
//    return FALSE;
//}

//static void factory_cleanall_mfile_modal(GtkWidget *widget,gpointer user_data)
//{
//    SaveMusicDialog *smd = user_data;
//    GtkTreeModel *model = gtk_tree_view_get_model(smd->smfm->wid_treeview);
//    gtk_list_store_clear(model);
//    gtk_list_store_clear(smd->id_cbmodal);
//    g_list_free1(smd->smfm->filelist);
//    smd->smfm->filelist = NULL;
//    gtk_tree_model_foreach(smd->id_store,factory_clean_idlist_dname,NULL); /*清空*/
//}






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


static void factory_mfile_add_idlist_columns (GtkTreeView *treeview,GtkTreeModel *cbmodel)
{

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeModel *modal =  gtk_tree_view_get_model(treeview);
    g_assert(modal);
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
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("文件名"),
             renderer,
             "text",
             COLUMN_ITEM_ADDR,
             NULL);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);
}

void factory_append_item_to_idlist_model(GtkListStore *store,
        SaveMusicItem *smt)
{
    GtkTreeIter iter;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set(store,&iter,
                       COLUMN_ITEM_SEQUENCE,smt->id_index,
                       COLUMN_ITEM_ADDR,smt->id_addr,
                       COLUMN_ITEM_IDNAME,smt->fname,
                       -1);
}


//void factory_add_item_to_mfidlist_model(GtkWidget *btn, gpointer user_data)
//{
//    SaveMusicDialog *smd = (SaveMusicDialog *)user_data;
//    gint len = GTK_LIST_STORE(smd->id_store)->length ;
//    SaveMusicItem *idsave = g_new0(SaveMusicItem,1);
//    idsave->id_index = len;
//    idsave->id_addr = len*2;
//    idsave->fname = g_strdup("");
//    idsave->active = 0;
//
//    factory_append_item_to_idlist_model(smd->id_store,idsave);
//    smd->itemlist = g_list_append(smd->itemlist,idsave);
//}

//static void factory_delete_mfidlist_last_model_item(GtkWidget *btn,gpointer user_data)
//{
//    /* 删除最后一个*/
//    GtkTreeIter iter;
//    SaveMusicDialog *smd = (SaveMusicDialog *)user_data;
//    GtkTreeModel *model = smd->id_store;
////   GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
//    gint rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL);
//    if(0 == rows) return;
//    GtkTreePath *path;
//    path = gtk_tree_path_new_from_indices(rows - 1, -1);
//    gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
//    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
//    gtk_tree_path_free (path);
//    gpointer lastptr = g_list_nth_data(smd->itemlist,rows-1);
//    smd->itemlist = g_list_remove(smd->itemlist,lastptr);
//}

//GtkWidget *factory_download_file_manager_with_model(SaveMusicDialog *smd)
//{
//    g_return_if_fail(smd);
////    SaveKV *skv = smd->skv;
//    GtkWidget *mvbox = gtk_vbox_new(FALSE,5);
//    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
//    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
//                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);
//
//    smd->id_store = gtk_list_store_new(NUM_OF_COLS,G_TYPE_INT,G_TYPE_INT,G_TYPE_STRING);
//    smd->id_treeview = gtk_tree_view_new_with_model(smd->id_store);
//    smd->id_cbmodal = factory_create_idcombox_model(smd->cboxlist);
//    gtk_tree_view_set_grid_lines (smd->id_treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);
//
//
//    factory_mfile_add_idlist_columns(smd->id_treeview,smd->id_cbmodal); /* 添加列 */
//    gtk_container_add (GTK_CONTAINER (wid_idlist), smd->id_treeview);
////    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(wid_idlist), smd->id_treeview);
//
////    gtk_box_pack_start(GTK_BOX(mainBox),wid_idlist,TRUE,TRUE,0);
//
//    /* 创建IDlist 主界面的模型 */
//
//    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (smd->id_treeview), TRUE);
//    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (smd->id_treeview)),
//                                 GTK_SELECTION_SINGLE);
//
//    GtkTreeIter iter;
//    if(smd->itemlist) /* 原来的值 */
//    {
//        GtkTreePath *path;
//        GList *tlist = smd->itemlist;
//        for(; tlist ; tlist = tlist->next)
//        {
//            factory_append_item_to_idlist_model(smd->id_store,tlist->data);
//        }
////        path = gtk_tree_path_new_from_string(skv->value);
//        path = gtk_tree_path_new_from_string(*smd->vnumber);
//        gtk_tree_model_get_iter(GTK_TREE_MODEL(smd->id_store), &iter, path);
//        GtkTreeSelection *selection = gtk_tree_view_get_selection (smd->id_treeview);
//        gtk_tree_selection_select_iter (selection,&iter); /* 选择到上次 */
//        gtk_tree_path_free(path);
//    }
//
//    GtkWidget *opthbox = gtk_hbox_new(TRUE,10);
//    gtk_box_pack_start(GTK_BOX(opthbox),
//                       factory_new_add_button(factory_add_item_to_mfidlist_model,smd),FALSE,TRUE,0);
//    gtk_box_pack_start(GTK_BOX(opthbox),
//                       factory_delete_last_button(factory_delete_mfidlist_last_model_item,smd),FALSE,TRUE,0);
//
//    gtk_box_pack_start(GTK_BOX(mvbox),wid_idlist,TRUE,TRUE,0);
//    gtk_box_pack_start(GTK_BOX(mvbox),opthbox,FALSE,FALSE,0);
////    gtk_box_pack_start(GTK_BOX(mainBox),factory_new_add_button(factory_add_item_to_idlist_model,sid),FALSE,FALSE,0);
//    gtk_widget_show(mvbox);
//    return mvbox;
//}

static void factory_order_display_widget(GList *vlist)
{
    /* 顺序选择，只有当前的值有效时，下一个才可以选择． */
    GList *list = vlist;
    for(; list; list = list->next)
    {
        ListBtnArr *lba = list->data;
        if(!g_ascii_strcasecmp(*lba->vnumber,"-1"))
        {
            list = list->next;
            for(; list; list=list->next)
            {
                ListBtnArr *lba = list->data;
                g_free(*lba->vnumber);
                *lba->vnumber = g_strdup("-1");
                gtk_button_set_label(GTK_BUTTON(lba->widget1),*lba->vnumber);
                gtk_widget_set_sensitive(lba->widget1,FALSE);
            }
            break;
        }
        else
        {
            g_return_if_fail(list->next);
            ListBtnArr *lba = list->next->data;
            gtk_widget_set_sensitive(lba->widget1,TRUE);

        }
        g_return_if_fail(list);
    }
}

void factory_create_list_array_manager_dialog(GtkWidget *button,
        SaveStruct *sst)
{
    GtkWidget *sdialog = gtk_hbox_new(FALSE,0);
    GtkWidget *subdig = factory_create_new_dialog_with_buttons(sst->name,gtk_widget_get_toplevel(button));
    GtkWidget *dialog_vbox = GTK_DIALOG(subdig)->vbox;
    gtk_container_add(GTK_CONTAINER(dialog_vbox),sdialog);
    gtk_window_set_modal(GTK_WINDOW(subdig),TRUE);
    gtk_window_set_position (GTK_WINDOW(subdig),GTK_WIN_POS_MOUSE);
    GtkTooltips *tips = gtk_tooltips_new();

    ListBtn *lbtn = &sst->value.slbtn;
    if(!lbtn)
        message_error("结构体%s有错误，为空指针",sst->name);
    g_return_if_fail(lbtn);
    gchar *lastname = factory_get_last_section(sst->name,".");
    gchar **title = g_strsplit(lastname,"[",-1);
    gchar *fmt =  g_strconcat(title[0],"(%d)",NULL);

    g_strfreev(title);

    int r = 0;
    int pos = 0;
    int tnum = 0;
    gboolean onetime = TRUE;

    gboolean sensitive = TRUE;

    ArrayBaseProp *abp = &lbtn->arr_base;
    GtkWidget *vbox = gtk_vbox_new(TRUE,0);
    for(; r < abp->row ; r++)
    {
        GtkWidget *hbox = gtk_hbox_new(TRUE,0);
        int c = 0;
        tnum = r*abp->col;
        for(; c < abp->col ; c++)
        {
            pos = tnum +c;
            if( pos >= abp->reallen)
                break;
            ListBtnArr *lba = g_list_nth_data(lbtn->vlist,pos);
            if(pos && !g_ascii_strcasecmp(*lba->vnumber,"-1"))
                sensitive = FALSE;

            GtkWidget *twovbox = gtk_vbox_new(TRUE,0);
            GtkWidget *lab = gtk_label_new(g_strdup_printf(fmt,pos));
            GtkWidget *btn = gtk_button_new_with_label(*lba->vnumber);
            lba->widget1 = btn;

            gtk_widget_set_sensitive(btn,sensitive);
            if(!g_ascii_strcasecmp(*lba->vnumber,"-1") && onetime )
            {
                gtk_widget_set_sensitive(btn,TRUE);
                onetime = FALSE;
            }

            ListDlgArg *lda = g_new0(ListDlgArg,1);
            lda->type = g_strdup_printf(fmt,pos);
            lda->user_data = lba->vnumber; /* 注意这里是二级指针 */
            lda->isArray = TRUE;
            lda->odw_func = factory_order_display_widget;
            lda->vlist = lbtn->vlist;
            g_signal_connect(G_OBJECT(btn),"clicked",
                             G_CALLBACK(factory_create_file_manager_dialog),lda);
            gtk_box_pack_start(GTK_BOX(twovbox),lab,FALSE,FALSE,0);
            gtk_box_pack_start(GTK_BOX(twovbox),btn,FALSE,FALSE,0);
            gtk_box_pack_start(GTK_BOX(hbox),twovbox,FALSE,FALSE,0);
        }
        gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

    }
    gtk_container_add(GTK_CONTAINER(sdialog),vbox);
//    g_signal_connect(G_OBJECT (subdig), "response",
//                     G_CALLBACK (sst->close_func), sst); /* 保存关闭 */
    gtk_widget_show_all(sdialog);
    gtk_dialog_run(subdig);
    gtk_widget_destroy(subdig);
    g_free(lastname);

}

void factory_create_file_manager_dialog(GtkWidget *btn,ListDlgArg *lda)
{
    factory_music_fm_get_type(lda->type);
    SaveMusicDialog *smd = curLayer->smd;
    smd->vnumber = lda->user_data;
    switch(smd->fmst)
    {
    case INDEX:
    {
        factory_mfile_create_idlist_dialog(btn,lda);
    }
    break;
    case SEQUENCE:
    case PHY:
    {
        factory_mfile_manager_dialog(btn,NULL); /* 单独文件列表 */
    }
    break;
    default:
        break;
    }

    if(lda->isArray)
    {
        (lda->odw_func)(lda->vlist);
    }
}


//static gboolean factory_cboxmodel_foreach(GtkTreeModel *model,
//        GtkTreePath *path,
//        GtkTreeIter *iter,
//        gpointer data)
//{
//    SaveMusicItem *smt = (SaveMusicItem*)data;
//    gchar *newstr = g_strdup("");
//    gtk_tree_model_get(model,iter,0,&newstr,-1);
//    if(!g_strcasecmp(smt->fname,newstr))
//    {
//        smt->active = gtk_tree_path_get_indices (path)[0];
//        g_free(newstr);
//        return TRUE;
//    }
//    g_free(newstr);
//    return FALSE;
//}


//static gboolean factory_mfile_save_lastcolumn_foreach(GtkTreeModel *model,
//        GtkTreePath *path,
//        GtkTreeIter *iter,
//        gpointer data)
//{
//    SaveMusicDialog *smd = curLayer->smd;
//    gint pos = gtk_tree_path_get_indices (path)[0];
//    SaveMusicItem *smt = g_list_nth_data(data,pos);
//    gtk_tree_model_get(model,iter,COLUMN_ITEM_IDNAME,&smt->fname,-1);
//    gtk_tree_model_foreach(smd->id_cbmodal,factory_cboxmodel_foreach,smt); /* 最后一列的名字有改变 */
//    return FALSE;
//}


//static gboolean factory_mfile_changed_dname_foreach(GtkTreeModel *model,
//        GtkTreePath *path,
//        GtkTreeIter *iter,
//        gpointer data)
//{
//    SaveMusicFileMan *smfm =(SaveMusicFileMan *)data;
//    gint pos = gtk_tree_path_get_indices (path)[0];
//    SaveMusicFile *smf = g_list_nth_data(smfm->filelist,pos);
//    gtk_tree_model_get(model,iter,COLUMN_DNAME,&smf->down_name,-1);
//    return FALSE;
//}

/* 文件列表管理界面 */
/* 文件列表管理界面 */
void factory_mfile_manager_dialog(GtkWidget *btn,SaveStruct *sst)
{
    SaveMusicDialog *smd = curLayer->smd;
    GtkWidget *parent = gtk_widget_get_toplevel(btn);
    GtkWidget *window = factory_create_new_dialog_with_buttons(smd->title,
                        parent);
    GtkWidget *dialog_vbox = GTK_DIALOG(window)->vbox;

    gtk_window_set_modal(GTK_WINDOW(window),TRUE);

    gtk_window_set_resizable (GTK_WINDOW (window),FALSE);
    gtk_widget_set_size_request (GTK_WINDOW (window),800,500);
    GtkWidget *mlist = factory_mfile_music_list_dialog(parent,smd);
    gtk_container_add(GTK_CONTAINER(dialog_vbox),mlist);

    gtk_widget_show_all(window);
//    SaveMusicFileMan *smfm = smd->smfm;
    gint ret =  gtk_dialog_run(window); /* 阻塞运行,不用信号连接其它函数,调用其它函数传参有问题 */
    if(ret == GTK_RESPONSE_OK) /* 2014-6-19 改成这种方式调用.原来信号连接函数保留*/
    {
        gint v = -1;
//        GList *list = smd->itemlist;
        GtkTreeModel *mlist_treeview = g_object_get_data(G_OBJECT(mlist),
                                       "m_treeview");
        GtkTreeModel *mlist_model = gtk_tree_view_get_model(mlist_treeview);

        GtkTreeSelection *msel = gtk_tree_view_get_selection(mlist_treeview);
        GList *sellist = gtk_tree_selection_get_selected_rows(msel,NULL);

        if(!sellist)
        {
            *smd->vnumber = g_strdup("-1");
        }
        else
        {
            GtkTreeIter iter;

            int seq = -1;
            int phy = -1;
            if(gtk_tree_model_get_iter(mlist_model,&iter,sellist->data))
            {
                gtk_tree_model_get (mlist_model,&iter,0,&seq,-1);
                gtk_tree_model_get (mlist_model,&iter,1,&phy,-1);
            }

            if(smd->fmst == SEQUENCE)
            {
                *smd->vnumber = g_strdup_printf("%d",seq);
            }
            else if(smd->fmst == PHY)
            {
                *smd->vnumber = g_strdup_printf("%d",phy);
            }
//            g_list_foreach(sellist,(GFunc)g_free,NULL);
            g_list_free(sellist);
        }

        gtk_button_set_label(GTK_BUTTON(btn), *smd->vnumber );/* 更改按钮标签*/
    }

    gtk_widget_destroy(window);
}

static gboolean factory_mfile_mlist_empty_item(GtkListStore *store)
{
    GtkTreeIter iter;

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                        COLUMN_SEQUENCE,-1,
                        COLUMN_PHY,-1,
                        COLUMN_FNAME,g_strdup(""),
                        COLUMN_DNAME,factory_utf8("空行不能编辑与删除插入"),
                        -1);
    return TRUE;
}


static gboolean factory_mfile_mlist_append_item(GtkListStore *store,SaveMusicFile *smf)
{
    GtkTreeIter iter;

    gint n = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store),NULL);

    if(n - 1 >= OBJECT_MAX)
    {
        gchar *msg = factory_utf8("对像的数量达到65535,不能添加了!");
        factory_message_dialoag(ddisplay_active()->shell,msg);
        g_free(msg);
        return FALSE;
    }
    gtk_list_store_append (store, &iter);
    if(empty_quark == g_quark_from_string(smf->down_name))
        smf->down_name = g_strdup_printf("file_%d",smf->base_quark);
    gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                        COLUMN_SEQUENCE,n,
                        COLUMN_PHY,n+smf->offset,
                        COLUMN_FNAME,g_quark_to_string(smf->base_quark),
                        COLUMN_DNAME,smf->down_name,
                        -1);
    return TRUE;
}

static gboolean factory_mfile_mlist_insert_item(GtkListStore *store,
        SaveMusicFile *smf,
        GtkTreeIter *sibling)
{
    GtkTreeIter iter;
    gint n = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store),NULL);
    if(n - 1 >= OBJECT_MAX)
    {
        gchar *msg = factory_utf8("对像的数量达到65535,不能添加了!");
        factory_message_dialoag(ddisplay_active()->shell,msg);
        g_free(msg);
        return FALSE;
    }
    if(empty_quark == g_quark_from_string(smf->down_name))
        smf->down_name = g_strdup_printf("file_%d",smf->base_quark);
    gtk_list_store_insert_before (store,&iter,sibling);
    gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                        COLUMN_SEQUENCE,n,
                        COLUMN_PHY,n+smf->offset,
                        COLUMN_FNAME,g_quark_to_string(smf->base_quark),
                        COLUMN_DNAME,smf->down_name,
                        -1);
    return TRUE;
}



static gboolean factory_mfile_mlist_insert_foreach(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    SaveMusicDialog *smd = curLayer->smd;
    int pos = gtk_tree_path_get_indices(path)[0];
    gchar *basename;
    gtk_tree_model_get(model,iter,COLUMN_FNAME,&basename,-1);
    GQuark bquark = g_quark_from_string(basename);
    SaveMusicFile *smf = g_hash_table_lookup(smd->mtable,bquark);
    if(smf)
    {
        gtk_list_store_set(GTK_LIST_STORE(model),iter,COLUMN_SEQUENCE,pos,-1);
        gtk_list_store_set(GTK_LIST_STORE(model),iter,COLUMN_PHY,
                           pos+smf->offset,-1);
        smd->mflist = g_list_append(smd->mflist,smf);
    }
    return FALSE;
}



void factory_open_file_dialog(GtkWidget *widget,gpointer user_data)
{

    const gchar *label = gtk_button_get_label(GTK_BUTTON(widget));
    GtkWidget *parent = g_object_get_data(user_data,"dlg_parent");
    GQuark tquar = g_quark_from_string(label);
    SaveMusicDialog *smd = curLayer->smd;

    static GtkWidget *opendlg = NULL;
    if(!opendlg)
    {
        opendlg = gtk_file_chooser_dialog_new(_("打开音乐文件"),
                                              parent,
                                              GTK_FILE_CHOOSER_ACTION_OPEN,
//                        "default", /* default, not gnome-vfs */
                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                              GTK_STOCK_OPEN, GTK_RESPONSE_OK,
                                              NULL);
        gtk_dialog_set_default_response(GTK_DIALOG(opendlg), GTK_RESPONSE_ACCEPT);
        gtk_window_set_modal(GTK_WINDOW(opendlg),TRUE);
        gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (opendlg), TRUE);
//        if(smd->lastDir )
//            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(opendlg),
//                                                smd->lastDir);

        if (!gtk_file_chooser_get_extra_widget(GTK_FILE_CHOOSER(opendlg)))
        {
//            GtkWidget *omenu= gtk_combo_box_new();
            GtkFileFilter* filter;

            /* set up the gtk file (name) filters */
            /* 0 = by extension */
            filter = gtk_file_filter_new ();
            gtk_file_filter_set_name (filter, _("Music Files"));
            gtk_file_filter_add_pattern (filter, "*.wav");
            gtk_file_filter_add_pattern (filter, "*.wma");
            gtk_file_filter_add_pattern (filter, "*.mp3");
            gtk_file_filter_add_pattern (filter, "*.f1a");
            gtk_file_filter_add_pattern (filter, "*.a");
            gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (opendlg), filter);

            filter = gtk_file_filter_new ();
            gtk_file_filter_set_name (filter, _("All Files"));
            gtk_file_filter_add_pattern (filter, "*");
            gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (opendlg), filter);

//        gtk_combo_box_set_active (GTK_COMBO_BOX (omenu), persistence_get_integer ("import-filter"));
            gint ret = gtk_dialog_run(GTK_DIALOG(opendlg));
            GList *duplist = NULL; /* 重复添的加文件名 */
            if(ret == GTK_RESPONSE_OK)
            {
//                factory_choose_musicfile_callback(opendlg,GTK_RESPONSE_OK,user_data);
                GtkTreeView *mtreeview = user_data;

                GtkWidget *spinbox = g_object_get_data(G_OBJECT(user_data),"wid_offset");
                int offset = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinbox));
                GtkTreeModel *model = gtk_tree_view_get_model(mtreeview);
                GSList *getflist;
                GSList *flists = getflist =  gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(opendlg));
//                smd->lastDir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(opendlg));
                GQuark fquark = 0,bquark = 0;
                if(tquar == add_quark)
                {
                    for(; flists; flists = flists->next)
                    {
                        SaveMusicFile *smf = g_new0(SaveMusicFile,1);
                        smf->full_quark = g_quark_from_string((gchar *)flists->data);

                        gchar **split = g_strsplit((gchar *)flists->data,"\\",-1);
                        int len = g_strv_length(split);

                        smf->base_quark =  g_quark_from_string(split[len-1]);

                        gpointer hval = g_hash_table_lookup(smd->mtable,
                                                            smf->base_quark);
                        if(!hval)
                        {
                            g_hash_table_insert(smd->mtable,smf->base_quark,smf);
                        }
                        else
                        {
                            duplist = g_list_append(duplist,smf->base_quark);
                            continue;
                        }

                        g_strfreev(split);
                        smf->offset = offset;
                        smf->down_name = g_strdup("");
                        factory_mfile_mlist_append_item(model,smf);
                        smd->mflist  = g_list_append(smd->mflist,smf);
                    }
                }
                else /* 插入音乐文件 */
                {
                    GtkTreeSelection *msel = gtk_tree_view_get_selection(mtreeview);
                    GtkTreeIter sibling;
                    GList *firstsub = gtk_tree_selection_get_selected_rows(msel,&model);
                    gtk_tree_model_get_iter (model,&sibling,firstsub->data);
                    g_list_foreach(firstsub,(GFunc)gtk_tree_path_free,NULL);
                    g_list_free(firstsub);

                    gtk_tree_selection_select_iter (msel,&sibling);

                    GtkTreePath *path = gtk_tree_model_get_path(model,&sibling);
                    gint pos = gtk_tree_path_get_indices(path)[0];
                    for(; flists; flists = flists->next)
                    {
                        SaveMusicFile *smf = g_new0(SaveMusicFile,1);
                        smf->full_quark = g_quark_from_string((gchar *)flists->data);
                        gchar **split = g_strsplit((gchar *)flists->data,"\\",-1);
                        int len = g_strv_length(split);
                        smf->base_quark = g_quark_from_string(split[len-1]);
                        gpointer hval = g_hash_table_lookup(smd->mtable,
                                                            smf->base_quark);
                        if(!hval)
                        {
                            g_hash_table_insert(smd->mtable,smf->base_quark,smf);
                        }
                        else
                        {
                            duplist = g_list_append(duplist,smf->base_quark);
                            continue;
                        }

                        g_strfreev(split);
                        smf->offset = offset;
                        smf->down_name = g_strdup("");
                        factory_mfile_mlist_insert_item(model,smf,&sibling);
                        GtkTreePath *path = gtk_tree_model_get_path(model,&sibling);
                        gtk_tree_path_prev(path);
                        gtk_tree_path_free(path);
                    }
                    g_list_free(smd->mflist);
                    smd->mflist = NULL; /* insert操作要把链表清空重排 */
                    gtk_tree_model_foreach(GTK_TREE_MODEL(model),
                                           factory_mfile_mlist_insert_foreach,smd);
                }
                g_slist_foreach(getflist,(GFunc)g_free,NULL);
                g_slist_free(getflist);
            }
            gtk_widget_destroy(opendlg);
            if(g_list_length(duplist) > 0)
            {
                GList *t = duplist;
                gchar *fmt = factory_utf8("下列文件名已经存在,不需要重复添加!\n");
                for(; t; t= t->next)
                {
                    gchar *p = g_strdup(fmt);
                    g_free(fmt);
                    fmt = g_strconcat(p,g_strdup_printf("%s\n",
                                                        g_quark_to_string(t->data),NULL));
                }
//                g_list_foreach(duplist,(GFunc)g_free,NULL);
                g_list_free(duplist);
                factory_message_dialoag(parent,fmt);
                g_free(fmt);
            }


            opendlg = NULL;

        }
    }

}

//static void factory_choose_musicfile_callback(GtkWidget *dlg,
//        gint       response,
//        gpointer   user_data)
//{
//
//    GtkTreeView *mtreeview = user_data;
//    if (response == GTK_RESPONSE_OK)
//    {
//        GtkWidget *spinbox = g_object_get_data(G_OBJECT(user_data),"wid_offset");
//        int offset = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinbox));
//        GtkTreeModel *model = gtk_tree_view_get_model(mtreeview);
//
//
//        GList *strlist = NULL;
//        GList *filelist = NULL;
//
//        GSList *flists =  gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dlg));
//        smfm->lastDir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dlg));
//        for(; flists; flists = flists->next,num++)
//        {
//            SaveMusicFile *smf = g_new0(SaveMusicFile,1);
//            smf->index = num;
//            smf->file_addr = num + offset;
//            smf->full_name = g_strdup((gchar *)flists->data);
//            gchar **split = g_strsplit(smf->full_name,"\\",-1);
//            int len = g_strv_length(split);
//            smf->base_name = g_strdup(split[len-1]);
//            g_strfreev(split);
//            smf->down_name = g_strdup_printf("file_%d",smf->index);
//            filelist = g_list_append(filelist,smf);
//            strlist = g_list_append(strlist,smf->base_name);
////            factory_file_manager_append(smfm->wid_clist,smf);
//        }
//
//        if(num == 0)
//        {
//            smfm->filelist = g_list_copy(filelist);
//            smd->cboxlist = g_list_copy(strlist);
//        }
//        else if(smfm->man_opt == OPT_APPEND)
//        {
//            GList *p = filelist;
//            for(; p ; p = p->next)
//                smfm->filelist = g_list_append(smfm->filelist,p->data);
//            p = strlist;
//            for(; p; p = p->next)
//                smd->cboxlist = g_list_append(smd->cboxlist,p->data);
//        }
//        else
//        {
//            /*　新插入的文件　*/
//            GtkTreeSelection *selection = gtk_tree_view_get_selection (smfm->wid_treeview);
//            GtkTreeIter iter;
//            int pos = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL);;
//            if (gtk_tree_selection_get_selected (selection, NULL, &iter))
//            {
//                gtk_list_store_remove(selection,&iter);
//                gtk_tree_model_get(model,&iter,0,&pos,-1);
//            }
//
////            int pos = smfm->selected;
//            GList *p = filelist;
//            for(; p; p = p->next)
//                smfm->filelist = g_list_insert(smfm->filelist,p->data,pos);
//            g_list_free1(smd->cboxlist);
//            smd->cboxlist = NULL;
//            p = smfm->filelist;
//            for(; p ; p = p->next)
//            {
//                SaveMusicFile *smf = p->data;
//                smd->cboxlist = g_list_append(smd->cboxlist,smf->base_name);
//            }
//
//        }
//        if(smfm->man_opt == OPT_INSERT)
//        {
//            factory_file_manager_refresh_all_iter(smd);
////            factory_file_manager_refresh_all(smd);
//        }
//        else
//        {
//            GList *p = filelist;
//            for(; p; p = p->next)
//                factory_mfile_manager_append_iter(model,p->data);
////                factory_file_manager_append(smfm->wid_clist,p->data);
//        }
////        smfm->number = g_list_length(smfm->filelist);
//        smd->mfmos->m_itemadded(smd); /* 更新左边的界面 */
//
//    }
////    gtk_widget_destroy(dlg);
//}



//void factory_mfile_manager_clean_modal(SaveMusicDialog *smd)
//{
//    SaveMusicFileMan *smfm = smd->smfm;
//    g_list_free1(smfm->filelist);
//    smfm->filelist = NULL;
//    g_list_free1(smd->cboxlist);
//    smd->cboxlist = NULL;
//
//    gtk_list_store_clear( GTK_TREE_STORE( smd->id_cbmodal ) ); /* 清空原来的 */
//
//}
//
//void factory_mfile_manager_update_idmodal(SaveMusicDialog *smd)
//{
//    /* 更新左边最后一个下拉框的数据 */
//    gtk_list_store_clear(smd->id_cbmodal);
//    GList *tlist = smd->cboxlist;
//    factory_append_iten_to_cbmodal(smd->id_cbmodal,"");
//    for(; tlist; tlist= tlist->next)
//    {
//        factory_append_iten_to_cbmodal(smd->id_cbmodal,tlist->data);
//    }
//
//}

//void factory_mfile_manager_append_iter(GtkListStore *model,SaveMusicFile *smf)
//{
//    GtkTreeIter iter; /* 设备每一行文件列表 */
//    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
//    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
//                        COLUMN_SEQUENCE,smf->index,
//                        COLUMN_PHY,smf->file_addr,
//                        COLUMN_FNAME,smf->base_name,
//                        COLUMN_DNAME,smf->down_name,
//                        -1);
//}




//void factory_read_mfile_filelist_from_xml(ObjectNode obj_node)
//{
//    SaveMusicDialog *smd = curLayer->smd;
//    AttributeNode attr_node = obj_node;
//    while(attr_node = data_next(attr_node))
//    {
//        xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"name");
//        if(key)
//        {
//            if(!g_ascii_strcasecmp((gchar*)key,"Music_File"))
//            {
//
//                factory_read_file_list_from_xml(attr_node->xmlChildrenNode); /* 读文件列表 */
////                    factory_fm_get_cboxlist(smd);
//                break;
//            }
//            /* 这里读取左边布局的数据 ,ID列表*/
//            SaveMusicItem *smi = g_new0(SaveMusicItem,1);
//            smi->id_index = g_strtod((gchar*)key,NULL);
//
//            key = xmlGetProp(attr_node,(xmlChar *)"addr");
//            smi->id_addr = g_strtod((gchar*)key,NULL);
////                    sss->name = g_strdup(fst->Name);
//            key = xmlGetProp(attr_node,(xmlChar *)"active");
//
//            smi->active = g_strtod((gchar*)key,NULL);
//
//
//            key = xmlGetProp(attr_node,(xmlChar *)"idname");
//            if(!key)
//            {
//                smi->fname = g_strdup("");
//            }
//            else
//            {
//                smi->fname = g_strdup((gchar*)key);
//            }
//            smi->dname = g_strdup("");
//            smd->itemlist = g_list_append(smd->itemlist,smi);
//        }
//        xmlFree(key);
//    }
//}
//
//void factory_read_file_list_from_xml(ObjectNode obj_node)
//{
//    /*　这里读文件列表　*/
//    SaveMusicDialog *smd = curLayer->smd;
//    SaveMusicFileMan  *smfm = smd->smfm;
//    AttributeNode attr_node = obj_node;
//    while(attr_node = data_next(attr_node))
//    {
//        xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"name");
//        if(!key) continue;
//
//        SaveMusicFile *smf = g_new0(SaveMusicFile,1);
//        smf->index = g_strtod((gchar*)key,NULL);
//
//        key = xmlGetProp(attr_node,(xmlChar *)"fname");
//        smf->full_name = g_strdup((gchar*)key);
//
//        key = xmlGetProp(attr_node,(xmlChar *)"addr");
//        smf->file_addr = g_strtod((gchar*)key,NULL);
//
//        key = xmlGetProp(attr_node,(xmlChar *)"dname");
//        smf->down_name = g_strdup((gchar*)key);
////                    sss->name = g_strdup(fst->Name);
//        gchar **split = g_strsplit(smf->full_name,"\\",-1);
//        int len = g_strv_length(split);
//        smf->base_name = g_strdup(split[len-1]);
//        g_strfreev(split);
//        smfm->filelist = g_list_append(smfm->filelist,smf);
//        smd->cboxlist = g_list_append(smd->cboxlist,smf->base_name);
//
//        xmlFree(key);
//
//    }
//    if(!attr_node)
//        return;
//}
//
//void factory_save_mfile_dialog_to_xml(SaveStruct *sss,ObjectNode obj_node)
//{
//    SaveMusicDialog *smd = curLayer->smd;
//    gchar *lastsection = factory_get_last_section(sss->name,".");
//    factory_music_fm_get_type(lastsection);
//
//    ObjectNode ccc = xmlNewChild(obj_node, NULL,
//                                 (const xmlChar *)JL_NODE, NULL);
//    xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)sss->name);
//    xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
//    xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)sss->type);
//
//    xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)sss->value.vnumber);
//    gchar *idname  = g_strdup("");
//    int vnumber = g_strtod(sss->value.vnumber,NULL);
//    SaveMusicFileMan  *smfm = smd->smfm;
//    if(smfm->filelist) /* 空指针就不保存这一项 */
//    {
//
//        GtkTreeIter iter;
////        GtkTreeModel *model = gtk_tree_view_get_model(smd->smfm->wid_treeview);
//        if(smd->fmst == INDEX)
//        {
//            SaveMusicItem *smi  = g_list_nth_data(smd->itemlist,g_strtod(sss->value.vnumber,NULL));
//            if(smi)
//                idname = g_strdup(smi->fname);
//        }
//        else if(smd->fmst == SEQUENCE)
//        {
//            SaveMusicFile *smf = g_list_nth_data(smfm->filelist,g_strtod(sss->value.vnumber,NULL));
//            if(smf)
//                idname = g_strdup(smf->base_name);
//        }
//        else if(smd->fmst == PHY)
//        {
//            int pos = vnumber - smd->smfm->offset;
//            if(pos > 0)
//            {
//                SaveMusicFile *smf = g_list_nth_data(smfm->filelist,pos);
//                if(smf)
//                    idname = g_strdup(smf->base_name);
//            }
//
//        }
//
//    }
//    xmlSetProp(ccc, (const xmlChar *)"idname", (xmlChar *)idname);
//    if(!strlen(idname) && (smd->fmst == PHY) && (vnumber <  smd->smfm->offset))
//    {
//        g_free(sss->value.vnumber);
//        sss->value.vnumber = g_strdup("-1");
//        xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)sss->value.vnumber);
//    }
//    g_free(idname);
//}
//
//
//void factory_write_mfile_filelist(ObjectNode obj_node)
//{
//
//    SaveMusicDialog *smd = curLayer->smd;
//    gchar *rows = g_strdup_printf("%d",g_list_length(smd->itemlist));
//    xmlSetProp(obj_node, (const xmlChar *)"rows", (xmlChar *)rows);
//    g_free(rows);
//    GList *flist = smd->itemlist;
//    for(; flist; flist = flist->next)
//    {
//        SaveMusicItem *smi = flist->data;
//        ObjectNode ccc = xmlNewChild(obj_node, NULL,
//                                     (const xmlChar *)JL_NODE, NULL);
//        xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)g_strdup_printf("%d",smi->id_index));
//        xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
//        xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)g_strdup_printf("%d",smi->active ? smi->active-1 : -1));
//        xmlSetProp(ccc, (const xmlChar *)"addr", (xmlChar *)g_strdup_printf("%d",smi->id_addr));
//        xmlSetProp(ccc, (const xmlChar *)"active", (xmlChar *)g_strdup_printf("%d",smi->active));
//        xmlSetProp(ccc, (const xmlChar *)"idname", (xmlChar *)smi->fname);
//    }
//
//    ObjectNode newobj = xmlNewChild(obj_node, NULL, (const xmlChar *)"Music_File", NULL);
//
//    xmlSetProp(newobj, (const xmlChar *)"name",
//               (xmlChar *)"Music_File");
//    g_return_if_fail(smd->smfm); /* 没有音乐文件　*/
////        xmlSetProp(newobj, (const xmlChar *)"offset",
////                   (xmlChar *)g_strdup_printf("%d",smd->smfm->offset));
//    flist = smd->smfm->filelist;
//    for(; flist; flist = flist->next) /* 这里保存文件列表 */
//    {
//        SaveMusicFile *smf = flist->data;
//        ObjectNode tnode = xmlNewChild(newobj, NULL, (const xmlChar *)"file", NULL);
//        xmlSetProp(tnode, (const xmlChar *)"name", (xmlChar *)g_strdup_printf("%d",smf->index));
//        xmlSetProp(tnode, (const xmlChar *)"addr", (xmlChar *)g_strdup_printf("%d",smf->file_addr));
//        xmlSetProp(tnode, (const xmlChar *)"fname", (xmlChar *)smf->full_name);
//        xmlSetProp(tnode, (const xmlChar *)"dname", (xmlChar *)smf->down_name);
//    }
//}

gboolean
factory_mfile_mlist_treeview_onButtonPressed(GtkWidget *treeview, GdkEventButton *event,
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
            GtkWidget *btn = g_object_get_data(G_OBJECT(mainbox),"btn_1");
            gtk_widget_set_sensitive(btn,TRUE);
            btn = g_object_get_data(G_OBJECT(mainbox),"btn_2");
            gtk_widget_set_sensitive(btn,TRUE);
            btn = g_object_get_data(G_OBJECT(mainbox),"btn_3");
            gtk_widget_set_sensitive(btn, TRUE);
        }
    }

    return FALSE; /* we did not handle this */
}

/* 单独的文件列表 */
GtkWidget *factory_mfile_music_list_dialog(GtkWidget *parent,
        SaveMusicDialog *smd)
{
    GtkWidget *sdialog = gtk_vbox_new(FALSE,0);
    GtkWidget *label = gtk_label_new(factory_utf8("物理文件号偏移:"));
    GtkWidget *spbox = gtk_spin_button_new_with_range(0,65536,1);
//    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spbox),5);
    gtk_widget_set_sensitive(spbox,FALSE);
    GtkWidget *hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),spbox,FALSE,FALSE,0);


    GtkWidget *sep = gtk_hseparator_new();

    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);


    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spbox),smd->offset);
//    SaveMusicFileMan *smfm  = smd->smfm;

    GtkListStore *mlist_store  = gtk_list_store_new(NUM_OF_MCOLUMN,G_TYPE_INT,
                                 G_TYPE_INT,G_TYPE_STRING,
                                 G_TYPE_STRING);

//    factory_mfile_mlist_empty_item(mlist_store); /* 添加一个占位的空行 */
    GtkTreeView *mlist_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(mlist_store));
    g_object_set_data(G_OBJECT(sdialog),"m_treeview",mlist_treeview);
    g_object_set_data(G_OBJECT(mlist_treeview),"dlg_parent",parent);
    g_object_set_data(G_OBJECT(mlist_treeview),"wid_offset",spbox);

    GtkWidget *opt_box = factory_music_file_manager_operator(mlist_treeview);
    GtkWidget *btn = g_object_get_data(G_OBJECT(opt_box),"btn_0");
    gtk_widget_set_sensitive(btn,TRUE);
    g_signal_connect(mlist_treeview,"button_release_event",
                     G_CALLBACK(factory_mfile_mlist_treeview_onButtonPressed),
                     opt_box);

    if(smd->mflist)
    {
        GList *tlist = smd->mflist;
        for(; tlist ; tlist = tlist->next)
        {
            SaveMusicFile *smf = tlist->data;
            factory_mfile_mlist_append_item(mlist_store,smf);
        }
        int n = -1;
        if(smd->fmst == SEQUENCE)
        {
            n = g_strtod(*smd->vnumber,NULL);
        }
        else if(smd->fmst == PHY)
        {
            n = g_strtod(*smd->vnumber,NULL)- smd->offset;
        }
        factory_mfile_selection_row(mlist_treeview,n);
    }

//    gtk_tree_view_set_headers_visible(smfm->wid_treeview,TRUE);

    factory_mfile_manager_add_columns(mlist_treeview);


    gtk_container_add (GTK_CONTAINER (wid_idlist), mlist_treeview);
    gtk_tree_view_set_grid_lines (mlist_treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(mlist_treeview),
                                GTK_SELECTION_EXTENDED);
//    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(wid_idlist),smfm->wid_treeview);


    /* 文件列表的所有操作*/


    gtk_box_pack_end(GTK_BOX(hbox),opt_box,FALSE,FALSE,0);

    gtk_box_pack_start(GTK_BOX(sdialog),hbox,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(sdialog),sep,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(sdialog),wid_idlist,TRUE,TRUE,0);
//    gtk_box_pack_start(GTK_BOX(sdialog),opt_box,FALSE,FALSE,0);

    gtk_widget_show_all(sdialog);
//    g_signal_connect(clist,"select_row",G_CALLBACK(factory_music_file_manager_select_callback),&smd->smfm->selected);
    return sdialog;
}

/** 这里是子列表的函数 **/

static void factory_mfile_sublist_append_empty_callback(GtkWidget *btn,
        gpointer user_data)
{

    twoWidget *tw = (twoWidget*)user_data;
    GtkTreeView *sub_treeview = g_object_get_data(tw->left,"sub_treeview");
    GtkTreeModel *sub_model = gtk_tree_view_get_model(sub_treeview);
    factory_sublist_append_item_to_model(sub_model,"");
}

/* 添加行为 */
static void factory_mfile_sublist_append_callback(GtkWidget *btn,
        gpointer user_data)
{

    GtkTreeIter iter;
    twoWidget *tw = (twoWidget*)user_data;
    GtkTreeView *mlist_treeview = g_object_get_data(tw->right,"m_treeview");
    GtkTreeModel *act_model = gtk_tree_view_get_model(mlist_treeview);


    GtkTreeView *sub_treeview = g_object_get_data(tw->left,"sub_treeview");
    GtkTreeModel *sub_model = gtk_tree_view_get_model(sub_treeview);

    GtkTreeSelection *selection = gtk_tree_view_get_selection (mlist_treeview);
    int scount = gtk_tree_selection_count_selected_rows (selection);
    if (scount >= 1 )
    {
        GList *treelist = gtk_tree_selection_get_selected_rows (selection,
                          &act_model);
        for(; treelist; treelist = treelist->next)
        {
            gchar *txt = g_strdup("");
            gtk_tree_model_get_iter (act_model,&iter,treelist->data);
            gtk_tree_model_get(act_model,&iter,COLUMN_FNAME,&txt,-1);
            factory_sublist_append_item_to_model(sub_model,txt);
            g_free(txt);
        }
    }

}

/* 插入空行 */
static void factory_mfile_sublist_insert_empty_callback(GtkWidget *btn,gpointer user_data)
{

    twoWidget *tw = (twoWidget*)user_data;
    GtkTreeView *sub_treeview = g_object_get_data(tw->left,"sub_treeview");
    GtkTreeModel *sub_model = gtk_tree_view_get_model(sub_treeview);
    GtkTreeSelection *subsel = gtk_tree_view_get_selection(sub_treeview);

    GtkTreeIter iter,liter;
    GList *firstsub = gtk_tree_selection_get_selected_rows(subsel,&sub_model);
    if(!firstsub) /* 没有选择一个点 */
        return;

    gtk_tree_model_get_iter (sub_model,&liter,firstsub->data);
    g_list_foreach(firstsub,(GFunc)gtk_tree_path_free,NULL);
    g_list_free(firstsub);

    gtk_tree_selection_select_iter (subsel,&liter);
    factory_sublist_insert_item_to_model(sub_model,"",&liter);
    gtk_tree_model_foreach(sub_model,factory_sublist_revalue_foreach,NULL);

}

static void factory_mfile_sublist_insert_callback(GtkWidget *btn,gpointer user_data)
{

    twoWidget *tw = (twoWidget*)user_data;
    GtkTreeView *mlist_treeview = g_object_get_data(tw->right,"m_treeview");
    GtkTreeModel *act_model = gtk_tree_view_get_model(mlist_treeview);


    GtkTreeView *sub_treeview = g_object_get_data(tw->left,"sub_treeview");
    GtkTreeModel *sub_model = gtk_tree_view_get_model(sub_treeview);

    GtkTreeSelection *selection = gtk_tree_view_get_selection (mlist_treeview);
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


static void factory_mfile_sublist_delete_callback(GtkWidget *btn,gpointer user_data)
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
        isert = g_object_get_data(G_OBJECT(bparent),"opt_ist_ety");
        gtk_widget_set_sensitive(isert,FALSE);
    }
}


/* 子表中间的人操作 */
static GtkWidget *factory_mfile_sublist_operators(GtkWidget *left,
        GtkWidget *right)
{
    stw.left = left;
    stw.right = right;
    GtkWidget *vsplit = gtk_vseparator_new();

    GtkWidget *mainBox = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(mainBox),vsplit,TRUE,TRUE,0);

    GtkWidget*  btn =gtk_button_new_with_label(factory_utf8("添加"));
    g_object_set_data(G_OBJECT(mainBox),"opt_apd",btn);
    gtk_box_pack_start(GTK_BOX(mainBox),btn,FALSE,TRUE,0);

    g_signal_connect(btn,"clicked",
                     G_CALLBACK(factory_mfile_sublist_append_callback),&stw);

    btn =gtk_button_new_with_label(factory_utf8("添加空行"));
    g_object_set_data(G_OBJECT(mainBox),"opt_apd_ety",btn);
    gtk_box_pack_start(GTK_BOX(mainBox),btn,FALSE,TRUE,15);

    g_signal_connect(btn,"clicked",
                     G_CALLBACK(factory_mfile_sublist_append_empty_callback),&stw);



    btn = gtk_button_new_with_label(factory_utf8("插入"));
    g_object_set_data(G_OBJECT(mainBox),"opt_ist",btn);
    gtk_box_pack_start(GTK_BOX(mainBox),btn,FALSE,TRUE,0);
    g_signal_connect(btn,"clicked",
                     G_CALLBACK(factory_mfile_sublist_insert_callback),&stw);
    gtk_widget_set_sensitive(btn,FALSE);

    btn =gtk_button_new_with_label(factory_utf8("插入空行"));
    g_object_set_data(G_OBJECT(mainBox),"opt_ist_ety",btn);
    gtk_box_pack_start(GTK_BOX(mainBox),btn,FALSE,TRUE,15);
    gtk_widget_set_sensitive(btn,FALSE);
    g_signal_connect(btn,"clicked",
                     G_CALLBACK(factory_mfile_sublist_insert_empty_callback),&stw);

    btn = gtk_button_new_with_label(factory_utf8("删除"));
    g_object_set_data(G_OBJECT(mainBox),"opt_del",btn);
    gtk_box_pack_start(GTK_BOX(mainBox),btn,FALSE,TRUE,0);
    g_signal_connect(btn,"clicked",
                     G_CALLBACK(factory_mfile_sublist_delete_callback),left);
    gtk_widget_set_sensitive(btn,FALSE);
    vsplit = gtk_vseparator_new();

    gtk_box_pack_start(GTK_BOX(mainBox),vsplit,TRUE,TRUE,0);
    return mainBox;
}

static void factory_mfile_add_sublist_columns (GtkTreeView *treeview,GtkTreeModel *cbmodel)
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

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("文件名称"),
             renderer,
             "text",
             COLUMN_ITEM_IDNAME,
             NULL);
//    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ITEM_ADDR);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);

}

static gboolean factory_mfile_sublist_saveitem_foreach(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    subTable *stable = (subTable*)data;
    gchar *txt;
    gtk_tree_model_get(model,iter,COLUMN_ITEM_IDNAME,&txt,-1);
    stable->table_list = g_list_append(stable->table_list,
                                       (gpointer)g_quark_from_string(txt));
    g_free(txt);
    return FALSE;
}


static void factory_mfile_sublist_dialog_response(GtkWidget *widget,
        int response_id,
        gpointer user_data)
{
    if(response_id == GTK_RESPONSE_OK)
    {
        SaveMusicDialog *smd = curLayer->smd;
        GtkTreeView *sub_treeview = g_object_get_data(widget,"sub_treeview");
        GtkTreeModel *sub_model = gtk_tree_view_get_model(sub_treeview);
        gint n  = gtk_tree_model_iter_n_children(sub_model,NULL);
        if(0 == n)
        {
            gtk_widget_destroy(widget);
            return;
        }

        GtkTreeView *idtreeview = (GtkTreeView*)user_data;
        GtkWidget *entry = g_object_get_data(widget,"table_name");
        subTable *stable = g_new0(subTable,1);
        if(entry)
        {
            gchar *text = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
            if(0 == strlen(g_strstrip(text)) ) /* 不能为空 */
            {
                gchar *msg = factory_utf8("请输入表名称!");
                factory_message_dialoag(widget,msg);
                g_free(msg);
                return;
            }
            stable->nquark = g_quark_from_string(text);

            /* 这里用了一个遍历函数保存子表的所有值 */

            gtk_tree_model_foreach(sub_model,factory_mfile_sublist_saveitem_foreach,
                                   stable);

            GtkMenuItem *item = g_object_get_data(G_OBJECT(widget),"id_opt");
            const gchar *opt_lab = gtk_menu_item_get_label(item);
            GtkTreeModel *idmodel = gtk_tree_view_get_model(idtreeview);
            if(!g_ascii_strcasecmp(opt_lab,factory_utf8("添加")))
            {
                /* 这里添加一行 */
                factory_idlist_append_item_to_model(idmodel,text);
                smd->midlists = g_list_append(smd->midlists,stable);
            }
            else if(!g_ascii_strcasecmp(opt_lab,factory_utf8("编辑")))
            {
                GtkTreeIter iter;
                GtkTreeSelection *idsel = gtk_tree_view_get_selection(idtreeview);
                if(gtk_tree_selection_get_selected(idsel,NULL,&iter))
                {
                    /* 改表名*/
                    gtk_list_store_set(idmodel,&iter,COLUMN_IDNAME,text,-1);
                    /* 改链表的数据 */
                    GtkTreePath *path;
                    path = gtk_tree_model_get_path(idmodel,&iter);
                    int pos = gtk_tree_path_get_indices(path)[0];
                    subTable *ostable  = g_list_nth_data(smd->midlists,pos);
                    ostable->nquark = stable->nquark;
                    g_list_free1(ostable->table_list);

                    ostable->table_list = g_list_copy(stable->table_list);
                    g_list_free1(stable->table_list);
                    g_free(stable);

                    gtk_tree_path_free(path);
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
                    smd->midlists = g_list_insert(smd->midlists,stable,pos);
                    gtk_tree_path_free(path);
                }
            }

            g_free(text);
        }
    }

    gtk_widget_destroy(widget);
}



gboolean
factory_mfile_subtreeview_onButtonPressed(GtkWidget *treeview, GdkEventButton *event,
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
            btn = g_object_get_data(G_OBJECT(mainbox),"opt_ist_ety");
            gtk_widget_set_sensitive(btn,TRUE);
        }
    }

    return FALSE; /* we did not handle this */
}


/* 子表编辑与文件列表界面 */
static GtkWidget* factory_mfile_sublist_create_dialog(GtkMenuItem *item,
        GtkTreeView *ptreeview)
{
    GtkWidget *mainBox = gtk_vbox_new(FALSE,0);
    SaveMusicDialog *smd = curLayer->smd;
    GtkWidget *parent = g_object_get_data(ptreeview,"ptreeview");
    GtkWidget *subdig = factory_create_new_dialog_with_buttons(factory_utf8("ID列表"),
                        parent);
    GtkWidget *dialog_vbox = GTK_DIALOG(subdig)->vbox;
    gtk_window_set_modal(GTK_WINDOW(subdig),TRUE);
    gtk_window_set_resizable (GTK_WINDOW (subdig),TRUE);
    gtk_widget_set_size_request (subdig,1000,500);
    gtk_container_add(GTK_CONTAINER(dialog_vbox),mainBox);
    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);
    /* 创建subidlist 编辑界面的模型 */
    GtkTreeModel *sub_model;
    sub_model = gtk_list_store_new(NUM_OF_COLS,G_TYPE_INT,G_TYPE_INT,G_TYPE_STRING);

    GtkTreeView *sub_treeview  = gtk_tree_view_new_with_model (sub_model);
    g_object_set_data(G_OBJECT(subdig),"sub_treeview",sub_treeview);
    g_object_set_data(G_OBJECT(wid_idlist),"sub_treeview",sub_treeview);
    /* id_opt 是用来判断添加和插入的 */
    g_object_set_data(G_OBJECT(subdig),"id_opt",item);

    gtk_tree_view_set_grid_lines (sub_treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);
    factory_mfile_add_sublist_columns(sub_treeview,sub_model); /* 添加列 */

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (sub_treeview), TRUE);
    gtk_tree_selection_set_mode (
        gtk_tree_view_get_selection (GTK_TREE_VIEW (sub_treeview)),
        GTK_SELECTION_MULTIPLE);
    gtk_container_add (GTK_CONTAINER (wid_idlist),sub_treeview);


    GtkTreeModel *idmodel = gtk_tree_view_get_model(ptreeview);
    gint idcount  = gtk_tree_model_iter_n_children(idmodel,NULL);

    GtkWidget *subbox  = gtk_vbox_new(FALSE,0);

    GtkWidget *namehbox = gtk_hbox_new(FALSE,0);
    GtkWidget *label = gtk_label_new(factory_utf8("子表名称:"));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry),
                       g_strdup_printf(factory_utf8("子表_%d"),idcount));
    g_object_set_data(subdig,"table_name",entry);
    gtk_box_pack_start(GTK_BOX(namehbox),label,FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(namehbox),entry,FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(subbox),namehbox,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(subbox),wid_idlist,TRUE,TRUE,0);


    /*第一行的一个水平布局*/
    GtkWidget *mlist_widget = factory_mfile_music_list_dialog(subdig,smd);
    GtkWidget *opt_box = factory_mfile_sublist_operators(wid_idlist,
                         mlist_widget);
    GtkWidget *midhbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(midhbox),subbox,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(midhbox),opt_box,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(midhbox),mlist_widget ,TRUE,TRUE,0);/* 右边是行为列表 */

    gtk_box_pack_start(GTK_BOX(mainBox),midhbox,TRUE,TRUE,0);

    g_signal_connect(sub_treeview,"button_release_event",
                     G_CALLBACK(factory_mfile_subtreeview_onButtonPressed),opt_box);

    g_object_unref (sub_model);

    g_signal_connect(G_OBJECT(subdig),"response",
                     G_CALLBACK(factory_mfile_sublist_dialog_response),
                     (gpointer)ptreeview);
    gtk_widget_show_all(subdig);
    return subdig;
}



/** 这里文件列表的函数 **/
/* 右键菜单操作 */
gboolean factory_mfile_idlist_popumenu(GtkTreeView *treeview,
                                       GdkEventButton *event,
                                       gpointer user_data)
{
    GtkWidget *menuitem,*menu;
    menu = gtk_menu_new();
    int num =sizeof(menuitems)/sizeof(GroupOfOperation);
    int n = 0;
    for(; n < num; n++)
    {
        menuitem = gtk_menu_item_new_with_label(factory_utf8(menuitems[n].name));
        g_signal_connect(menuitem, "activate",
                         G_CALLBACK(menuitems[n].mfmf),(gpointer)treeview);
        menuitems[n].btn = menuitem;
        g_object_set_data(menu,g_strdup_printf("%d_item",n),menuitem);
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
factory_mfile_idtreeview_onButtonPressed(GtkWidget *treeview, GdkEventButton *event,
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


static void factory_mfile_idlist_dialog_response(GtkWidget *widget,
        gint response_id,
        gpointer user_data)
{
    GtkWidget *btn = (GtkWidget*)user_data;
    SaveMusicDialog *smd = curLayer->smd;
    if(response_id == GTK_RESPONSE_OK)
    {
        GtkTreeView *idtreeview = g_object_get_data(G_OBJECT(widget),
                                  "idtreeview");
        GtkTreeModel *idmodel = gtk_tree_view_get_model(idtreeview);
        GtkTreeSelection *idsel = gtk_tree_view_get_selection(idtreeview);
        GtkTreeIter iter;
        g_free(*smd->vnumber);
        if(gtk_tree_selection_get_selected(idsel,NULL,&iter))
        {
            GtkTreePath *path;
            path = gtk_tree_model_get_path(idmodel,&iter);
            gint pos = gtk_tree_path_get_indices(path)[0];
            *smd->vnumber = g_strdup_printf("%d",pos);
        }
        else
        {
            *smd->vnumber = g_strdup("-1");
        }
        gtk_button_set_label(GTK_BUTTON(btn),*smd->vnumber);
    }
    gtk_widget_destroy(widget);
}





void factory_mfile_create_idlist_dialog(GtkWidget *button,
                                        ListDlgArg *lda)
{
    GtkWidget *mainBox = gtk_vbox_new(FALSE,0);
    SaveIdDialog *sid = (SaveIdDialog *)curLayer->sid;
    GtkWidget *parent = gtk_widget_get_toplevel(button);
    GtkWidget *subdig = factory_create_new_dialog_with_buttons(factory_utf8("文件ID列表"),
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

    SaveMusicDialog *smd = curLayer->smd;
    GList *mlist = smd->midlists;

    for(; mlist; mlist = mlist->next)
    {
        subTable *stable = mlist->data;
        factory_idlist_append_item_to_model(idmodel,
                                            g_quark_to_string(stable->nquark));
    }


    /* 先中上一次的结果  */
    GtkTreePath *path = gtk_tree_path_new_from_string(*(gchar**)lda->user_data);
    gtk_tree_selection_select_path(gtk_tree_view_get_selection(idtreeview),path);
    gtk_tree_path_free(path);

    g_object_set_data(G_OBJECT(subdig),"idtreeview",idtreeview);
    g_object_set_data(G_OBJECT(wid_idlist),"idtreeview",idtreeview);

    /* 这个数据是用来指定上级窗口 */
    g_object_set_data(G_OBJECT(idtreeview),"ptreeview",subdig);
    g_signal_connect(idtreeview,"button_release_event",
                     G_CALLBACK(factory_mfile_idtreeview_onButtonPressed),NULL);
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
                     G_CALLBACK(factory_mfile_idlist_dialog_response),button);
}


