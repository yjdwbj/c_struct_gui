#include "musicfilemanager.h"


#define WIDTH 300
#define HEIGHT 500
const gchar *chvaild = "0123456789abcdefghijklmnopqrstuvwxyz_.";
typedef struct
{
    GQuark opt_quark;
    GSList *file_list;
    GtkTreeView *treeview;
    SaveMusicDialog *smd;
    GQuark add_quark;
    gchar *old_title;
    gint idle_id;

} BackThread;

static guint mfile_filled_signals = 0;

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
        g_object_set_data(G_OBJECT(item),"defstable",stable);
        factory_mfile_sublist_create_dialog(item,ptreeview);

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
        smd->midlists = g_list_remove(smd->midlists,stable);
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
                          &m_model);
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
                gtk_tree_model_get(m_model,&iter,COLUMN_FNAME,
                                   &bscname,-1);
                g_hash_table_remove(smd->mtable,g_quark_from_string(bscname));
                gtk_list_store_remove (GTK_LIST_STORE(m_model),&iter);
                gtk_tree_path_free(path);
            }
        }

        g_list_foreach(reflist,(GFunc)gtk_tree_row_reference_free,NULL);
        g_list_free(reflist);

        /*删除之后要重新建立链顺序*/
        g_list_free1(smd->mflist);
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
             "fixed-width",
             5,
             "alignment",
             PANGO_ALIGN_CENTER,
             NULL);
//    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ITEM_SEQUENCE);
//    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_append_column (treeview, column);

    /* column for severities */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("物理地址"),
             renderer,
             "text",
             COLUMN_PHY,
              "fixed-width",
             5,
             "alignment",
             PANGO_ALIGN_CENTER,
             NULL);
//    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ITEM_ADDR);
//    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_append_column (treeview, column);

    /* Combo */

    renderer = renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("源文件名"),
             renderer,
             "text",
             COLUMN_FNAME,
              "fixed-width",
             16,
             NULL);
//    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);


    renderer = renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("下载文件名(双击重命名)"),
             renderer,
             "text",
             COLUMN_DNAME,
            "fixed-width",
             16,
             NULL);
    g_object_set (renderer,
                  "editable", TRUE,
                  NULL);
    g_signal_connect (renderer, "edited",
                      G_CALLBACK (factory_mfile_manager_changed_dname), model);
    g_object_set_data (G_OBJECT (renderer), "column",
                       GINT_TO_POINTER (COLUMN_DNAME));

//    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
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
        g_signal_connect(wid,"clicked",G_CALLBACK(mflistopt[n].mfmf),
                         mtreeview);
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
    gtk_widget_set_size_request ( window,800,500);
    GtkWidget *mlist = factory_mfile_music_list_dialog(window,smd);
    gtk_container_add(GTK_CONTAINER(dialog_vbox),mlist);

    gtk_widget_show_all(window);
//    SaveMusicFileMan *smfm = smd->smfm;
    gint ret =  gtk_dialog_run(GTK_DIALOG(window)); /* 阻塞运行,不用信号连接其它函数,调用其它函数传参有问题 */
    if(ret == GTK_RESPONSE_OK) /* 2014-6-19 改成这种方式调用.原来信号连接函数保留*/
    {
        gint v = -1;
//        GList *list = smd->itemlist;
        GtkTreeView *mlist_treeview = g_object_get_data(G_OBJECT(mlist),
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


static gboolean factory_mfile_mlist_append_item(GtkListStore *store,
                                                SaveMusicFile *smf)
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

static guint timeout_id = 0;

/* 添加与插入文件上的后台线程函数 */
static gboolean factory_mfile_mlist_thread_func(gpointer user_data)
{
    BackThread *btd = user_data;
    if(btd->idle_id)
    {
        g_source_remove(btd->idle_id);
        btd->idle_id = 0;
    }
    GtkWidget *parent = g_object_get_data(G_OBJECT(btd->treeview),
                                          "dlg_parent");
    SaveMusicDialog *smd = curLayer->smd;
    GtkTreeView *mtreeview = btd->treeview;

    GList *duplist = NULL; /* 重复添的加文件名 */
    GtkTreeModel *model = gtk_tree_view_get_model(btd->treeview);

    GSList *flists = btd->file_list;
    GQuark fquark = 0,bquark = 0;
    if(btd->opt_quark == btd->add_quark)
    {
        for(; flists; flists = flists->next)
        {
            g_usleep(2);
            GDK_THREADS_ENTER();
            SaveMusicFile *smf = g_new0(SaveMusicFile,1);
            smf->isexists = TRUE;
            smf->full_quark = g_quark_from_string((gchar *)flists->data);
            gchar **split = g_strsplit((gchar *)flists->data,"\\",-1);
            int len = g_strv_length(split);

            smf->base_quark =  g_quark_from_string(split[len-1]);

            /*不能重复添加*/
            gpointer hval = g_hash_table_lookup(smd->mtable,
                                                (gpointer)smf->base_quark);
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
            smf->offset = smd->offset;
            smf->down_name = g_strdup("");
            factory_mfile_mlist_append_item(GTK_LIST_STORE(model),smf);
            gtk_widget_queue_draw(parent);
            gdk_window_process_all_updates();
             GDK_THREADS_LEAVE();
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
        gtk_tree_path_free(path);
        for(; flists; flists = flists->next)
        {
              g_usleep(2);
            GDK_THREADS_ENTER();
            SaveMusicFile *smf = g_new0(SaveMusicFile,1);
            smf->isexists = TRUE;
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
            smf->offset = smd->offset;
            smf->down_name = g_strdup("");
            factory_mfile_mlist_insert_item(model,smf,&sibling);
            gtk_widget_queue_draw(parent);
            gdk_window_process_all_updates();
            GtkTreePath *path = gtk_tree_model_get_path(model,&sibling);
            gtk_tree_path_prev(path);
            gtk_tree_path_free(path);
            GDK_THREADS_LEAVE();
        }
        g_list_free(smd->mflist);
        smd->mflist = NULL; /* insert操作要把链表清空重排 */
        gtk_tree_model_foreach(GTK_TREE_MODEL(model),
                               factory_mfile_mlist_insert_foreach,smd);
    }
    g_slist_foreach( btd->file_list,(GFunc)g_free,NULL);
    g_slist_free( btd->file_list);
     btd->file_list = 0;

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
        g_list_free(duplist);
        factory_message_dialoag(NULL,fmt);
        g_free(fmt);
    }
    GtkWidget *dla_vbox = g_object_get_data(G_OBJECT(mtreeview),
                                                    "dla_vbox");
    gtk_widget_set_sensitive(dla_vbox,TRUE);
    gtk_window_set_title(GTK_WINDOW(parent),
                                 btd->old_title);
    gtk_widget_queue_draw(parent);
    gdk_window_process_all_updates();

    return FALSE;
}


void factory_mfile_open_file_response(GtkWidget *widget,
                                      gint response_id,
                                      gpointer user_data)
{
        GtkWidget *opt_btn = g_object_get_data(G_OBJECT(user_data),
                                                   "opt_btn");
        gtk_widget_set_sensitive(opt_btn,TRUE);
        if(response_id == GTK_RESPONSE_OK)
        {
            GSList *slist = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(widget));
            gtk_widget_destroy(widget);
            gtk_widget_queue_draw(widget);

            GtkWidget *parent = g_object_get_data(user_data,"dlg_parent");
            GtkWidget *dla_vbox = g_object_get_data(G_OBJECT(user_data),
                                                    "dla_vbox");
            gtk_widget_set_sensitive(dla_vbox,FALSE);
            gtk_widget_queue_draw(parent);
            gdk_window_process_all_updates();

            gchar *oldtitle = g_strdup(gtk_window_get_title(GTK_WINDOW(parent)));
            const gchar *ntitle = factory_utf8("请耐心等待,文件处理中...");
            gtk_window_set_title(GTK_WINDOW(parent),ntitle);

            const gchar *label = gtk_button_get_label(GTK_BUTTON(opt_btn));
            BackThread *btd = g_new0(BackThread,1);

            btd->file_list =slist;
            btd->opt_quark = g_quark_from_string(label);
            btd->treeview = user_data;
            btd->smd = curLayer->smd;
            btd->add_quark = add_quark;
            btd->old_title =oldtitle;
            btd->idle_id =g_idle_add(factory_mfile_mlist_thread_func,btd);
//            timeout_id = g_timeout_add(60,factory_mfile_mlist_timeout,btd);
        }
        else
         gtk_widget_destroy(widget);
}


void factory_open_file_dialog(GtkWidget *widget,gpointer user_data)
{
    gtk_widget_set_sensitive(widget,FALSE);
    GtkWidget *parent = g_object_get_data(user_data,"dlg_parent");
    SaveMusicDialog *smd = curLayer->smd;
     g_object_set_data(G_OBJECT(user_data),"opt_btn",widget);
    GtkWidget *opendlg = NULL;
    if(!opendlg)
    {
        opendlg = gtk_file_chooser_dialog_new(_("打开音乐文件"),
                                              GTK_WINDOW(parent),
                                              GTK_FILE_CHOOSER_ACTION_OPEN,
//                        "default", /* default, not gnome-vfs */
                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                              GTK_STOCK_OPEN, GTK_RESPONSE_OK,
                                              NULL);
        gtk_dialog_set_default_response(GTK_DIALOG(opendlg), GTK_RESPONSE_ACCEPT);
        gtk_window_set_modal(GTK_WINDOW(opendlg),TRUE);
        gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (opendlg), TRUE);

        if (!gtk_file_chooser_get_extra_widget(GTK_FILE_CHOOSER(opendlg)))
        {
//            GtkWidget *omenu= gtk_combo_box_new();
            GtkFileFilter* filter;

            /* set up the gtk file (name) filters */
            /* 0 = by extension */
            filter = gtk_file_filter_new ();
            gtk_file_filter_set_name (filter, _("All Files"));
            gtk_file_filter_add_pattern (filter, "*");
            gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (opendlg), filter);

            filter = gtk_file_filter_new ();
            gtk_file_filter_set_name (filter, _("Music Files"));
            gtk_file_filter_add_pattern (filter, "*.wav");
            gtk_file_filter_add_pattern (filter, "*.wma");
            gtk_file_filter_add_pattern (filter, "*.mp3");
            gtk_file_filter_add_pattern (filter, "*.f1a");
            gtk_file_filter_add_pattern (filter, "*.a");
            gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (opendlg), filter);
        }
    }
    else
    {
        gtk_widget_set_sensitive(opendlg, TRUE);
        if (GTK_WIDGET_VISIBLE(opendlg))
            return;

    }
    g_signal_connect(G_OBJECT(opendlg),"response",
                    G_CALLBACK(factory_mfile_open_file_response),user_data);
    gtk_widget_show_all(opendlg);
}





void factory_read_mfile_filelist_from_xml(ObjectNode obj_node,
        const gchar *filename)
{
    SaveMusicDialog *smd = curLayer->smd;
    GList *nexists = NULL; /* 不存在的文件 */
    gchar *dirname = g_strdup(filename);
    gchar *pth = strrchr((char *)dirname,G_DIR_SEPARATOR);
    if (pth)
    {
        *(pth+1) = 0;
    }
    else
    {
        g_free(dirname);
    }
    gchar *mpath = g_strconcat(dirname,"music",G_DIR_SEPARATOR_S,NULL);
    if(!g_file_test(mpath,G_FILE_TEST_IS_DIR))
    {
        gchar *msg  = factory_utf8(g_strdup_printf("资源文件夹不存在!\n文件名:%s",mpath));
        factory_message_dialoag(NULL,msg);
        g_free(msg);
        return;
    }


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
                for( ; n < len; n++,cglist = cglist->next)
                {
                    stable->sub_list =
                        g_list_append(stable->sub_list,
                                      g_quark_from_string(cglist->data));
                }
                xmlFree(key);
            }
            smd->midlists = g_list_append(smd->midlists ,stable);
        }
    }

    ObjectNode mnode = factory_find_custom_node(obj_node,MUSIC_FILE);
    if(mnode)
    {
        GList *cglist = glist;
        ObjectNode cnode = mnode->xmlChildrenNode;
        while(cnode = data_next(cnode))
        {
            SaveMusicFile *smf = g_new0(SaveMusicFile,1);
            smf->offset = smf->offset;
            key = xmlGetProp(cnode,"bname");
            if(key)
            {
                smf->base_quark = g_quark_from_string((gchar*)key);
                gchar *fname = g_strconcat(mpath,key,NULL);
                if(!g_file_test(fname,G_FILE_TEST_EXISTS))
                {
                     nexists = g_list_append(nexists,filename);
                     smf->isexists = FALSE;
                }

                smf->full_quark = g_quark_from_string(fname);
                g_free(fname);
                xmlFree(key);
            }
            key = xmlGetProp(cnode,"dname");
            if(key)
            {
                smf->down_name = g_strdup((gchar*)key);
                xmlFree(key);
            }
            smd->mflist = g_list_append(smd->mflist,smf);
            g_hash_table_insert(smd->mtable,smf->base_quark,smf);
        }
    }

    if(nexists)
    {
       GList *tmplist = nexists;
       gchar *flist = "\n";
       for(;tmplist;tmplist = tmplist->next)
       {
           gchar *p = g_strdup(flist);
           g_free(flist);
           flist = g_strconcat(p,"\n",NULL);
           g_free(p);
       }

       gchar *msg  =
       factory_utf8(g_strdup_printf("下列文件不存在,"
                    "请手动复制它们到工程目录下的music目录!\n文件列表:%s",flist));
      factory_message_dialoag(NULL,msg);
      g_free(msg);
      g_free(flist);
      g_list_foreach(nexists,(GFunc)g_free,NULL);
      g_list_free(nexists);
    }

}


void factory_mfile_save_item_to_xml(SaveStruct *sss,ObjectNode obj_node)
{
    SaveMusicDialog *smd = curLayer->smd;
    factory_music_fm_get_type(sss->name);
    ObjectNode ccc = xmlNewChild(obj_node, NULL,
                                 (const xmlChar *)JL_NODE, NULL);
    xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)sss->name);
    xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
    xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)sss->type);

    gchar *idname  = g_strdup("");
    int vnumber = g_strtod(sss->value.vnumber,NULL);

    GtkTreeIter iter;
//        GtkTreeModel *model = gtk_tree_view_get_model(smd->smfm->wid_treeview);
    if(smd->fmst == INDEX)
    {
        if(smd->midlists)
        {
            GArray *garray = g_array_new(FALSE,FALSE,sizeof(gint));
            GList *looplist = smd->midlists;
            int sum = 0;
            for(; looplist ; looplist = looplist->next)
            {
                subTable *stable = looplist->data;
                int len = g_list_length(stable->sub_list);
                g_array_append_val(garray,sum);
                sum += len;
            }

            subTable *stable = g_list_nth_data(smd->midlists,vnumber);
            int newpos = 0;
            if(stable)
            {
                /*  这里不检测子表是否为空.其它的代码保证它一定有数*/
                newpos = g_array_index(garray,gint,vnumber);
                vnumber = newpos;
                idname = g_quark_to_string(stable->nquark);
            }
            g_array_free(garray,TRUE);

        }
    }
    else if(smd->fmst == SEQUENCE)
    {
        SaveMusicFile *smf = g_list_nth_data(smd->mflist,vnumber);
        if(smf)
            idname = g_quark_to_string(smf->base_quark);
    }
    else if(smd->fmst == PHY)
    {
        int pos = vnumber - smd->offset;
        if(pos > 0)
        {
            SaveMusicFile *smf = g_list_nth_data(smd->mflist,pos);
            if(smf)
                idname = g_quark_to_string(smf->base_quark);
        }

    }

    xmlSetProp(ccc, (const xmlChar *)"value",
               (xmlChar *)g_strdup_printf("%d",vnumber));
    xmlSetProp(ccc, (const xmlChar *)"idname", (xmlChar *)idname);
    xmlSetProp(ccc, (const xmlChar *)"org_val", (xmlChar *)sss->value.vnumber);
    if(!strlen(idname) && (smd->fmst == PHY) && (vnumber <  smd->offset))
    {
        g_free(sss->value.vnumber);
        sss->value.vnumber = g_strdup("-1");
        xmlSetProp(ccc, (const xmlChar *)"value",
                    (xmlChar *)sss->value.vnumber);
    }

}


static void factory_mfile_save_index_list(xmlNodePtr obj_node,GList *list)
{
    SaveMusicDialog *smd = curLayer->smd;
    xmlNodePtr node = xmlNewChild(obj_node, NULL,
                       (const xmlChar *)IDLIST_NODE, NULL);
    xmlSetProp(node, (const xmlChar *)"rows",
               (xmlChar *)g_strdup_printf("%d",g_list_length(list)));

    GList *flist = list;
    int n = 0;
    for(; flist; flist = flist->next,n++)
    {
        GQuark bquark = flist->data;
        /* 通名字找它在文件列表上的唯一的位置 */
        xmlNodePtr fnode = xmlNewChild(node, NULL,
                                       (const xmlChar *)JL_NODE, NULL);
        xmlSetProp(fnode, (const xmlChar *)"name", (xmlChar *)g_strdup_printf("%d",n));
        xmlSetProp(fnode, (const xmlChar *)"type", (xmlChar *)"u16");
        SaveMusicFile *smf = g_hash_table_lookup(smd->mtable,bquark);
        gchar *val;
        if(smf)
        {
            int pos = g_list_index(smd->mflist,smf);
            val = g_strdup_printf("%d",pos);
        }
        else
            val = g_strdup("-1");

        xmlSetProp(fnode, (const xmlChar *)"value",(xmlChar *)val);
        g_free(val);
        xmlSetProp(fnode, (const xmlChar *)"addr",
                   (xmlChar *)g_strdup_printf("%d",n*2));
//        xmlSetProp(fnode, (const xmlChar *)"active", (xmlChar *)g_strdup_printf("%d",smi->active));
        xmlSetProp(fnode, (const xmlChar *)"idname",
                   (xmlChar *)g_quark_to_string(bquark));
    }
}

static void factory_mfile_save_files_to_xml(xmlNodePtr obj_node,const gchar*mpath)
{
    SaveMusicDialog *smd = curLayer->smd;
    xmlNodePtr node = xmlNewChild(obj_node, NULL, (const xmlChar *)MUSIC_FILE, NULL);

    xmlSetProp(node, (const xmlChar *)"name",(xmlChar *)MUSIC_FILE);
    xmlSetProp(node, (const xmlChar *)"rows",
               (xmlChar *)g_strdup_printf("%d",g_list_length(smd->mflist)));

    GList* flist = smd->mflist;
    int n = 0;
    for(; flist; flist = flist->next,n++) /* 这里保存文件列表 */
    {
        SaveMusicFile *smf = flist->data;
        /* 是否要把资源文件复制到工具目录下?*/
        gchar *npc = g_strconcat(mpath,g_quark_to_string(smf->base_quark),
                                 NULL);
        GQuark npc_quark = g_quark_from_string(npc);
        if(npc_quark != smf->full_quark) /* 源与目地不一致要复制 */
        {
            GFile *src = g_file_new_for_path(g_quark_to_string(smf->full_quark));
            GFile *dst = g_file_new_for_path(npc);
            int res = g_file_copy(src,dst,
                                  G_FILE_COPY_OVERWRITE,NULL,NULL,NULL,NULL);
            smf->full_quark = npc_quark;
        }
        g_free(npc);

        xmlNodePtr tnode = xmlNewChild(node, NULL, (const xmlChar *)"file", NULL);
        xmlSetProp(tnode, (const xmlChar *)"name", (xmlChar *)g_strdup_printf("%d",n));
        xmlSetProp(tnode, (const xmlChar *)"addr",
                   (xmlChar *)g_strdup_printf("%d",n+smd->offset));
        xmlSetProp(tnode, (const xmlChar *)"dname",
                   (xmlChar *)smf->down_name);
        xmlSetProp(tnode, (const xmlChar *)"bname",
                   (xmlChar *)g_quark_to_string(smf->base_quark));
    }
}

void factory_mfile_save_to_xml(xmlNodePtr obj_node,const gchar*filename)
{

    SaveMusicDialog *smd = curLayer->smd;
    xmlNodePtr node;
    gchar *dirname = g_strdup(filename);
    gchar *pth = strrchr((char *)dirname,G_DIR_SEPARATOR);
    if (pth)
    {
        *(pth+1) = 0;
    }
    else
    {
        g_free(dirname);
    }
    gchar *mpath = g_strconcat(dirname,"music",G_DIR_SEPARATOR_S,NULL);
    if(!g_file_test(mpath,G_FILE_TEST_IS_DIR))
    {
        g_mkdir(mpath);
    }
    node = xmlNewChild(obj_node, NULL,
                                  (const xmlChar *)IDINDEX_NODE, NULL);
    xmlSetProp(node, (const xmlChar *)"rows",
               (xmlChar *)g_strdup_printf("%d",g_list_length(smd->midlists)));
    GList *rootlist = smd->midlists;
    GList *wlist = NULL;
    for(; rootlist; rootlist = rootlist->next)
    {
        subTable *stable = rootlist->data;
        xmlNodePtr idnode = xmlNewChild(node,NULL,(const xmlChar *)JL_NODE,NULL);
        xmlSetProp(idnode, (const xmlChar *)"name",
                   (xmlChar *)g_quark_to_string(stable->nquark));
        xmlSetProp(idnode, (const xmlChar *)"rows",
                   (xmlChar *)g_strdup_printf("%d",g_list_length(stable->sub_list)));

        wlist = g_list_concat(wlist,stable->sub_list);
    }
     factory_mfile_save_index_list(obj_node,wlist);
   //  g_list_free(wlist);
     factory_mfile_save_files_to_xml(obj_node,mpath);

    g_free(mpath);
}

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

    GtkWidget  *wid_scroll = gtk_scrolled_window_new (NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_scroll),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);


    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spbox),smd->offset);
//    SaveMusicFileMan *smfm  = smd->smfm;

    GtkListStore *mlist_store  = gtk_list_store_new(NUM_OF_MCOLUMN,G_TYPE_INT,
                                 G_TYPE_INT,G_TYPE_STRING,
                                 G_TYPE_STRING);

//    factory_mfile_mlist_empty_item(mlist_store); /* 添加一个占位的空行 */
    GtkTreeView *mlist_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(mlist_store));
    g_object_unref(mlist_store);
    g_object_set_data(G_OBJECT(sdialog),"m_treeview",mlist_treeview);
    g_object_set_data(G_OBJECT(mlist_treeview),"dlg_parent",parent);
    g_object_set_data(G_OBJECT(mlist_treeview),"dla_vbox",sdialog);
    g_object_set_data(G_OBJECT(mlist_treeview),"wid_offset",spbox);
    g_object_set_data(G_OBJECT(mlist_treeview),"pscroll_win",wid_scroll);
//    g_object_set(G_OBJECT(mlist_treeview),"spacing",15,
//                                            NULL);

//    g_idle_add(gdk_window_process_all_updates,NULL);
    GtkWidget *opt_box = factory_music_file_manager_operator(mlist_treeview);
    GtkWidget *btn = g_object_get_data(G_OBJECT(opt_box),"btn_0");
    gtk_widget_set_sensitive(btn,TRUE);
    g_signal_connect(mlist_treeview,"button_release_event",
                     G_CALLBACK(factory_mfile_mlist_treeview_onButtonPressed),
                     opt_box);
//    gtk_tree_view_set_fixed_height_mode(mlist_treeview,TRUE);

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

    gtk_tree_view_set_headers_visible(mlist_treeview,TRUE);

    factory_mfile_manager_add_columns(mlist_treeview);


    gtk_container_add (GTK_CONTAINER (wid_scroll),GTK_WIDGET(mlist_treeview));
    gtk_tree_view_set_grid_lines (mlist_treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(mlist_treeview),
                                GTK_SELECTION_EXTENDED);
//    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(wid_idlist),smfm->wid_treeview);


    /* 文件列表的所有操作*/


    gtk_box_pack_end(GTK_BOX(hbox),opt_box,FALSE,FALSE,0);

    gtk_box_pack_start(GTK_BOX(sdialog),hbox,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(sdialog),sep,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(sdialog),wid_scroll,TRUE,TRUE,0);
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
    GtkTreeView *sub_treeview = g_object_get_data(G_OBJECT(tw->left),
                                "sub_treeview");
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
    factory_sublist_insert_item_to_model(GTK_LIST_STORE(sub_model),"",&liter);
    gtk_tree_model_foreach(sub_model,factory_sublist_revalue_foreach,NULL);

}

static void factory_mfile_sublist_insert_callback(GtkWidget *btn,gpointer user_data)
{

    twoWidget *tw = (twoWidget*)user_data;
    GtkTreeView *mlist_treeview = g_object_get_data(G_OBJECT(tw->right),"m_treeview");
    GtkTreeModel *act_model = gtk_tree_view_get_model(mlist_treeview);


    GtkTreeView *sub_treeview = g_object_get_data(G_OBJECT(tw->left),"sub_treeview");
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
            gtk_tree_model_get(act_model,&iter,COLUMN_FNAME,&txt,-1);
            factory_sublist_insert_item_to_model(GTK_LIST_STORE(sub_model),
                                                 txt,&liter);
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
    stable->sub_list = g_list_append(stable->sub_list,
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
        GtkWidget *entry = g_object_get_data(G_OBJECT(widget),"table_name");

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
            GQuark nquark = g_quark_from_string(text);
            GtkMenuItem *item = g_object_get_data(G_OBJECT(widget),"id_opt");

            GList *looplist = smd->midlists;
            subTable *curtable = g_object_get_data(G_OBJECT(item),"defstable");
            for(; looplist; looplist = looplist->next)
            {
                subTable *etab = looplist->data;
                if(etab == curtable) continue;
                if(etab->nquark == nquark)
                {
                    gchar *msg = factory_utf8("表名同名,请更重命名!");
                    factory_message_dialoag(widget,msg);
                    g_free(msg);
                    return;
                }
            }
            subTable *stable = g_new0(subTable,1);
            stable->nquark = nquark;
            /* 这里用了一个遍历函数保存子表的所有值 */

            gtk_tree_model_foreach(sub_model,
                                   factory_mfile_sublist_saveitem_foreach,
                                   stable);


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
                    GList *curptr = g_list_nth(smd->midlists,pos);
                    factory_idlist_insert_item_to_model(GTK_LIST_STORE(idmodel),
                                                        text,&iter);
                    smd->midlists = g_list_insert_before(smd->midlists,
                                                         curptr,stable);
                    gtk_tree_path_free(path);
                    gtk_tree_model_foreach(idmodel,
                                           factory_idlist_delete_item_update_foreach,NULL);
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
    GtkWidget *parent = g_object_get_data(G_OBJECT(ptreeview),"ptreeview");
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
    gtk_container_add (GTK_CONTAINER (wid_idlist),GTK_WIDGET(sub_treeview));


    GtkTreeModel *idmodel = gtk_tree_view_get_model(ptreeview);
    gint idcount  = gtk_tree_model_iter_n_children(idmodel,NULL);

    GtkWidget *subbox  = gtk_vbox_new(FALSE,0);

    GtkWidget *namehbox = gtk_hbox_new(FALSE,0);
    GtkWidget *label = gtk_label_new(factory_utf8("子表名称:"));
    GtkWidget *entry = gtk_entry_new();


    subTable *stable = g_object_get_data(G_OBJECT(item),"defstable");
    if(stable) /* 默认值 */
    {
        GList *p = stable->sub_list;
        for(; p; p=p->next)
        {
            gchar *txt =  g_quark_to_string(p->data);
            gpointer exist = g_hash_table_lookup(smd->mtable,p->data);
            if(!exist)
            {
                p->data = empty_quark;
                factory_sublist_append_item_to_model(sub_model,
                                                     g_quark_to_string(empty_quark));
            }
            else
                factory_sublist_append_item_to_model(sub_model,txt);

        }
        gtk_entry_set_text(GTK_ENTRY(entry),g_quark_to_string(stable->nquark));
    }
    else
        gtk_entry_set_text(GTK_ENTRY(entry),
                           factory_get_subtable_name(smd->midlists,idcount));
    g_object_set_data(G_OBJECT(subdig),"table_name",entry);
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
        g_object_set_data(G_OBJECT(menu),g_strdup_printf("%d_item",n),menuitem);
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
        factory_mfile_idlist_popumenu(GTK_TREE_VIEW(treeview), event, (gpointer)scount);
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


