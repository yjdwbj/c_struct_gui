#include "musicfilemanager.h"

extern FactoryStructItemAll *factoryContainer;


static void factory_add_file_manager_item(GtkWidget *widget, gpointer user_data)
{
    SaveMusicDialog *smd = user_data;
    smd->smfm->man_opt = OPT_APPEND;
    factory_open_file_dialog(widget,user_data);
}

//static void factory_unselect_file_manager_item(GtkWidget *widget,gpointer user_data)
//{
//    SaveMusicDialog *smd = user_data;
//    gtk_clist_unselect_all (GTK_CLIST(smd->smfm->wid_clist));
//
//}

static void factory_unselect_mfile_modal(GtkWidget *widget,gpointer user_data)
{
    SaveMusicDialog *smd = ( SaveMusicDialog *)user_data;
    GtkTreeSelection *selection = gtk_tree_view_get_selection (smd->smfm->wid_treeview);
    gtk_tree_selection_unselect_all (selection);
}

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
    g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_DNAME));

    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);
}


GtkWidget *factory_music_file_manager_operator(SaveMusicDialog *smd)
{
    /* 要添加操作选项，在这里修改　*/

    GroupOfOperation mfmo[] =
    {
        {NULL,factory_utf8("添加"),NULL,factory_add_file_manager_item},
        {NULL,factory_utf8("删除"),NULL,factory_delete_mfile_item_modal},
        {NULL,factory_utf8("插入"),NULL,factory_insert_file_manager_item},
        {NULL,factory_utf8("清空"),NULL,factory_cleanall_mfile_modal},
        {NULL,factory_utf8("不选"),NULL,factory_unselect_mfile_modal}
    };

    GtkWidget *operatorhbox = gtk_hbox_new(FALSE,2);

    int num =sizeof(mfmo)/sizeof(GroupOfOperation);
    int n = 0;
    for(; n < num; n++)
    {
        GtkWidget *wid = NULL;
        if(!mfmo[n].stack)
            wid = gtk_button_new_with_label(mfmo[n].name);
        else
            wid = gtk_button_new_from_stock(mfmo[n].stack);
        g_signal_connect(wid,"clicked",G_CALLBACK(mfmo[n].mfmf),smd);
        mfmo[n].btn = wid;
        gtk_box_pack_start(GTK_BOX(operatorhbox),wid,FALSE,FALSE,0);
    }

    return operatorhbox;
}

void factoy_clist_init(GtkWidget *clist)
{
    gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
    gtk_clist_set_column_width(GTK_CLIST(clist),0,50);
    gtk_clist_set_column_width(GTK_CLIST(clist),1,60);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist),2,TRUE);

}

//void factory_music_file_manager_select_callback(GtkWidget *clist,
//        gint row,gint column,
//        GdkEventButton *event,
//        gint  *ret)
//{
//    *ret = row;
//}

static void factory_mfile_manager_changed_dname(GtkCellRendererText *cell,
        const gchar         *path_string,
        const gchar         *new_text,
        gpointer             data)
{
    GtkTreeModel *model = (GtkTreeModel *)data;
    GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
    GtkTreeIter iter;
    gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));
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


static void factory_mfile_selection_row(gint row)
{
    SaveMusicDialog *smd = curLayer->smd;
    g_return_if_fail(smd);
    SaveMusicFileMan *smfm = smd->smfm;
    GtkTreePath *path;
    GtkTreeIter iter;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(smfm->wid_treeview);
    path = gtk_tree_path_new_from_string(g_strdup_printf("%d",row));
    GtkTreeModel *model = gtk_tree_view_get_model(smfm->wid_treeview);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
    gtk_tree_selection_select_iter (selection,&iter);
    gtk_tree_path_free(path);
}

static void factory_file_manager_refresh_all_iter(SaveMusicDialog *smd)
{
    SaveMusicFileMan *smfm = smd->smfm;
    GtkTreeModel *model = gtk_tree_view_get_model(smfm->wid_treeview);
    gtk_list_store_clear(model);
    GList *tlist = smfm->filelist;
    int num = 0;
    for(; tlist ; tlist = tlist->next,num++)
    {
        SaveMusicFile *smf = tlist->data;
        smf->index = num;
        smf->file_addr = smf->index + smfm->offset;
        factory_mfile_manager_append_iter(model,tlist->data);
    }
    smd->mfmos->m_itemadded(smd);
}

static gboolean factory_check_mfile_idlist_lastitem(GtkTreeModel *model,
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

static void factory_delete_mfile_item_modal(GtkWidget *btn,gpointer user_data)
{
    SaveMusicDialog *smd = (SaveMusicDialog *)user_data;
    SaveMusicFileMan *smfm = smd->smfm;
    GtkTreeView *treeview = smd->smfm->wid_treeview;
    GtkTreeModel *model = gtk_tree_view_get_model(smd->smfm->wid_treeview);
    GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
    GtkTreeIter iter;
    gchar *item ;
    gint rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL);
    if(0 == rows) return;

    int pos;
    if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
        gtk_list_store_remove(selection,&iter);
        gtk_tree_model_get(model,&iter,0,&pos,-1);
        gtk_tree_model_get(model,&iter,COLUMN_FNAME,&item,-1); /* 删掉名字 */
    }

    const gpointer  itm =  g_list_nth_data(smfm->filelist,pos);
    const gpointer c = g_list_nth_data(smd->cboxlist,pos);
    smd->cboxlist = g_list_remove(smd->cboxlist,c);
    smfm->filelist = g_list_remove(smfm->filelist,itm);

    gtk_tree_model_foreach(smd->id_store,factory_check_mfile_idlist_lastitem,item);/* 如果左边已经选择,这里要找到它删掉 */
    g_free(item);
    factory_file_manager_refresh_all_iter(smd);
}

static gboolean factory_clean_idlist_dname(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    gtk_list_store_set(model,iter,COLUMN_ITEM_IDNAME,"",-1);
    return FALSE;
}

static void factory_cleanall_mfile_modal(GtkWidget *widget,gpointer user_data)
{
    SaveMusicDialog *smd = user_data;
    GtkTreeModel *model = gtk_tree_view_get_model(smd->smfm->wid_treeview);
    gtk_list_store_clear(model);
    gtk_list_store_clear(smd->id_cbmodal);
    g_list_free1(smd->smfm->filelist);
    smd->smfm->filelist = NULL;
    gtk_tree_model_foreach(smd->id_store,factory_clean_idlist_dname,NULL); /*清空*/
}

void factory_insert_file_manager_item(GtkWidget *widget,gpointer user_data)
{
    SaveMusicDialog *smd = user_data;
    smd->smfm->man_opt = OPT_INSERT;
    factory_open_file_dialog(NULL,user_data);
}




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
    renderer = gtk_cell_renderer_combo_new ();
    g_object_set (renderer,
                  "model", cbmodel,
                  "text-column",0,
                  "has-entry", FALSE,
                  "editable", TRUE,
                  NULL);
    g_signal_connect (renderer, "edited",
                      G_CALLBACK (factory_idcell_edited), modal);
    g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_ITEM_IDNAME));

    column = gtk_tree_view_column_new_with_attributes (factory_utf8("文件(点击本列空白处)"),
             renderer,
             "text",
             COLUMN_ITEM_IDNAME,
             NULL);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);
}

void factory_append_item_to_idlist_model(GtkListStore *store, SaveMusicItem *smt)
{
    GtkTreeIter iter;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set(store,&iter,
                       COLUMN_ITEM_SEQUENCE,smt->id_index,
                       COLUMN_ITEM_ADDR,smt->id_addr,
                       COLUMN_ITEM_IDNAME,smt->fname,
                       -1);
}


void factory_add_item_to_mfidlist_model(GtkWidget *btn, gpointer user_data)
{
    SaveMusicDialog *smd = (SaveMusicDialog *)user_data;
    gint len = GTK_LIST_STORE(smd->id_store)->length ;
    SaveMusicItem *idsave = g_new0(SaveMusicItem,1);
    idsave->id_index = len;
    idsave->id_addr = len*2;
    idsave->fname = g_strdup("");
    idsave->active = 0;

    factory_append_item_to_idlist_model(smd->id_store,idsave);
    smd->itemlist = g_list_append(smd->itemlist,idsave);
}

static void factory_delete_mfidlist_last_model_item(GtkWidget *btn,gpointer user_data)
{
    /* 删除最后一个*/
    GtkTreeIter iter;
    SaveMusicDialog *smd = (SaveMusicDialog *)user_data;
    GtkTreeModel *model = smd->id_store;
//   GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
    gint rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL);
    if(0 == rows) return;
    GtkTreePath *path;
    path = gtk_tree_path_new_from_indices(rows - 1, -1);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    gtk_tree_path_free (path);
    gpointer lastptr = g_list_nth_data(smd->itemlist,rows-1);
    smd->itemlist = g_list_remove(smd->itemlist,lastptr);
}

GtkWidget *factory_download_file_manager_with_model(SaveMusicDialog *smd)
{
    g_return_if_fail(smd);
//    SaveKV *skv = smd->skv;
    GtkWidget *mvbox = gtk_vbox_new(FALSE,5);
    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);

    smd->id_store = gtk_list_store_new(NUM_OF_COLS,G_TYPE_INT,G_TYPE_INT,G_TYPE_STRING);
    smd->id_treeview = gtk_tree_view_new_with_model(smd->id_store);
    smd->id_cbmodal = factory_create_idcombox_model(smd->cboxlist);
    gtk_tree_view_set_grid_lines (smd->id_treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);


    factory_add_idlist_columns(smd->id_treeview,smd->id_cbmodal); /* 添加列 */
    gtk_container_add (GTK_CONTAINER (wid_idlist), smd->id_treeview);
//    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(wid_idlist), smd->id_treeview);

//    gtk_box_pack_start(GTK_BOX(mainBox),wid_idlist,TRUE,TRUE,0);

    /* 创建IDlist 主界面的模型 */

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (smd->id_treeview), TRUE);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (smd->id_treeview)),
                                 GTK_SELECTION_SINGLE);

    GtkTreeIter iter;
    if(smd->itemlist) /* 原来的值 */
    {
        GtkTreePath *path;
        GList *tlist = smd->itemlist;
        for(; tlist ; tlist = tlist->next)
        {
            factory_append_item_to_idlist_model(smd->id_store,tlist->data);
        }
//        path = gtk_tree_path_new_from_string(skv->value);
        path = gtk_tree_path_new_from_string(*smd->vnumber);
        gtk_tree_model_get_iter(GTK_TREE_MODEL(smd->id_store), &iter, path);
        GtkTreeSelection *selection = gtk_tree_view_get_selection (smd->id_treeview);
        gtk_tree_selection_select_iter (selection,&iter); /* 选择到上次 */
        gtk_tree_path_free(path);
    }

    GtkWidget *opthbox = gtk_hbox_new(TRUE,10);
    gtk_box_pack_start(GTK_BOX(opthbox),
                       factory_new_add_button(factory_add_item_to_mfidlist_model,smd),FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(opthbox),
                       factory_delete_last_button(factory_delete_mfidlist_last_model_item,smd),FALSE,TRUE,0);

    gtk_box_pack_start(GTK_BOX(mvbox),wid_idlist,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(mvbox),opthbox,FALSE,FALSE,0);
//    gtk_box_pack_start(GTK_BOX(mainBox),factory_new_add_button(factory_add_item_to_idlist_model,sid),FALSE,FALSE,0);
    gtk_widget_show(mvbox);
    return mvbox;
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

void factory_create_list_array_manager_dialog(GtkWidget *button,SaveStruct *sst)
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
            g_signal_connect(G_OBJECT(btn),"clicked",G_CALLBACK(factory_create_file_manager_dialog),lda);
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
//    smd->parent_btn = btn;
//    smd->skv = (SaveKV*)(lda->user_data); /* 公共部分 */
    smd->vnumber = lda->user_data;
    factory_file_manager_dialog(btn,NULL);
    if(lda->isArray)
    {
        (lda->odw_func)(lda->vlist);
    }
}


static gboolean factory_cboxmodel_foreach(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    SaveMusicItem *smt = (SaveMusicItem*)data;
    gchar *newstr = g_strdup("");
    gtk_tree_model_get(model,iter,0,&newstr,-1);
    if(!g_strcasecmp(smt->fname,newstr))
    {
        smt->active = gtk_tree_path_get_indices (path)[0];
        g_free(newstr);
        return TRUE;
    }
    g_free(newstr);
    return FALSE;
}


static gboolean factory_mfile_save_lastcolumn_foreach(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    SaveMusicDialog *smd = curLayer->smd;
    gint pos = gtk_tree_path_get_indices (path)[0];
    SaveMusicItem *smt = g_list_nth_data(data,pos);
    gtk_tree_model_get(model,iter,COLUMN_ITEM_IDNAME,&smt->fname,-1);
    gtk_tree_model_foreach(smd->id_cbmodal,factory_cboxmodel_foreach,smt); /* 最后一列的名字有改变 */
    return FALSE;
}


static gboolean factory_mfile_changed_dname_foreach(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    SaveMusicFileMan *smfm =(SaveMusicFileMan *)data;
    gint pos = gtk_tree_path_get_indices (path)[0];
    SaveMusicFile *smf = g_list_nth_data(smfm->filelist,pos);
    gtk_tree_model_get(model,iter,COLUMN_DNAME,&smf->down_name,-1);
    return FALSE;
}


void factory_file_manager_dialog(GtkWidget *btn,SaveStruct *sst)
{
    SaveMusicDialog *smd = curLayer->smd;
//    smd->skv = (SaveKV*)(sst->value.vnumber); /* 公共部分 */
    GtkWidget *mainHbox = gtk_hbox_new(FALSE,0);
    GtkWidget *parent = gtk_widget_get_toplevel(btn);
    GtkWidget *window = factory_create_new_dialog_with_buttons(smd->title,gtk_widget_get_toplevel(btn));
    GtkWidget *dialog_vbox = GTK_DIALOG(window)->vbox;
    gtk_container_add(GTK_CONTAINER(dialog_vbox),mainHbox);
    gtk_window_set_modal(GTK_WINDOW(window),TRUE);

    gtk_window_set_resizable (GTK_WINDOW (window),FALSE);
    gtk_widget_set_size_request (GTK_WINDOW (window),800,500);

    /* 这里按三列布局，左一列表框，中间线，右一列表框*/
    if(smd->fmst == INDEX )
    {
        gtk_box_pack_start(GTK_BOX(mainHbox),factory_download_file_manager_with_model(smd),TRUE,TRUE,1);
    }
    gtk_box_pack_start(GTK_BOX(mainHbox),factory_music_file_manager_with_model(NULL,smd),TRUE,TRUE,1);
    if(0 == gtk_tree_model_iter_n_children(GTK_TREE_MODEL(smd->id_cbmodal), NULL))
    {
        smd->mfmos->m_itemadded(smd);
    }
//    g_signal_connect(G_OBJECT (window), "response",
//                     G_CALLBACK (smd->mfmos->m_applydialog), smd); /* 保存关闭 */
    gtk_widget_show_all(window);
    SaveMusicFileMan *smfm = smd->smfm;
    gint ret =  gtk_dialog_run(window); /* 阻塞运行,不用信号连接其它函数,调用其它函数传参有问题 */
    if(ret == GTK_RESPONSE_OK) /* 2014-6-19 改成这种方式调用.原来信号连接函数保留*/
    {
        gint v = -1;
        GList *list = smd->itemlist;
        if(smd->fmst == INDEX) /* 索引号,左边的第一列*/
        {
            gtk_tree_model_foreach(smd->id_store,factory_mfile_save_lastcolumn_foreach,list);
            GtkTreeSelection *selection = gtk_tree_view_get_selection (smd->id_treeview);
            GtkTreeIter iter;
            int n = -1;
            if (gtk_tree_selection_get_selected (selection, NULL, &iter))
            {
                gtk_tree_model_get (smd->id_store,&iter,0,&n,-1);
            }
            *smd->vnumber = g_strdup_printf("%d",n);
        }
        else
        {
            GtkTreeSelection *selection = gtk_tree_view_get_selection (smd->smfm->wid_treeview);
            GtkTreeIter iter;
            gtk_tree_model_foreach(smfm->wid_store,factory_mfile_changed_dname_foreach,smfm);

//            GtkTreePath *path;
            int seq = -1;
            int phy = -1;
            if (gtk_tree_selection_get_selected (selection, NULL, &iter))
            {
                gtk_tree_model_get (smfm->wid_store,&iter,0,&seq,-1);
                gtk_tree_model_get (smfm->wid_store,&iter,1,&phy,-1);
            }
            if(smd->fmst == SEQUENCE)
            {
                *smd->vnumber = g_strdup_printf("%d",seq);
            }
            else if(smd->fmst == PHY)
            {
                *smd->vnumber = g_strdup_printf("%d",phy);
            }
        }
//        skv->value = g_strdup_printf("%d",v);
        gtk_button_set_label(GTK_BUTTON(btn), *smd->vnumber );/* 更改按钮标签*/
    }
    g_object_unref (GTK_TREE_MODEL(smd->id_store));
    g_object_unref(GTK_TREE_MODEL(smd->id_cbmodal));
    g_object_unref(GTK_TREE_MODEL(smd->smfm->wid_store));
    gtk_widget_destroy(window);
}

void factory_open_file_dialog(GtkWidget *widget,gpointer user_data)
{
    SaveMusicDialog *smd =(SaveMusicDialog *) user_data;
    g_return_if_fail(smd);
    SaveMusicFileMan *smfm = smd->smfm;
    static GtkWidget *opendlg = NULL;
    if(!opendlg)
    {
        opendlg = gtk_file_chooser_dialog_new(_("打开音乐文件"), NULL,
                                              GTK_FILE_CHOOSER_ACTION_OPEN,
//                        "default", /* default, not gnome-vfs */
                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                              GTK_STOCK_OPEN, GTK_RESPONSE_OK,
                                              NULL);
        gtk_dialog_set_default_response(GTK_DIALOG(opendlg), GTK_RESPONSE_ACCEPT);
        gtk_window_set_modal(GTK_WINDOW(opendlg),TRUE);
        gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (opendlg), TRUE);
        if(smfm->lastDir )
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(opendlg),smfm->lastDir);

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
            if(ret == GTK_RESPONSE_OK)
            {
                factory_choose_musicfile_callback(opendlg,GTK_RESPONSE_OK,smd);
            }
            gtk_widget_destroy(opendlg);
            opendlg = NULL;
//            g_signal_connect(GTK_FILE_CHOOSER(smfm->opendlg),
//                             "response", G_CALLBACK(factory_choose_musicfile_callback), smd);
        }
    }
//    else
//    {
//        if(smfm->lastDir )
//            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(smfm->opendlg),smfm->lastDir);
////        if(smt->full_name)
////            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(opendlg), smt->full_name);
//        gtk_widget_set_sensitive(smfm->opendlg, TRUE);
//        if (GTK_WIDGET_VISIBLE(smfm->opendlg))
//            return;
//    }
//    gtk_widget_show(smfm->opendlg);
}

void factory_mfile_manager_clean_modal(SaveMusicDialog *smd)
{
    SaveMusicFileMan *smfm = smd->smfm;
    g_list_free1(smfm->filelist);
    smfm->filelist = NULL;
    g_list_free1(smd->cboxlist);
    smd->cboxlist = NULL;

    gtk_list_store_clear( GTK_TREE_STORE( smd->id_cbmodal ) ); /* 清空原来的 */

}

void factory_mfile_manager_update_idmodal(SaveMusicDialog *smd)
{
    /* 更新左边最后一个下拉框的数据 */
    gtk_list_store_clear(smd->id_cbmodal);
    GList *tlist = smd->cboxlist;
    factory_append_iten_to_cbmodal(smd->id_cbmodal,"");
    for(; tlist; tlist= tlist->next)
    {
        factory_append_iten_to_cbmodal(smd->id_cbmodal,tlist->data);
    }

}

void factory_mfile_manager_append_iter(GtkListStore *model,SaveMusicFile *smf)
{
    GtkTreeIter iter; /* 设备每一行文件列表 */
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                        COLUMN_SEQUENCE,smf->index,
                        COLUMN_PHY,smf->file_addr,
                        COLUMN_FNAME,smf->base_name,
                        COLUMN_DNAME,smf->down_name,
                        -1);
}


static void factory_choose_musicfile_callback(GtkWidget *dlg,
        gint       response,
        gpointer   user_data)
{
    SaveMusicDialog *smd = user_data;
    g_return_if_fail(smd);
    SaveMusicFileMan *smfm = smd->smfm;
    if (response == GTK_RESPONSE_OK)
    {
        int offset = gtk_spin_button_get_value(GTK_SPIN_BUTTON(smfm->wid_offset));
        GtkTreeModel *model = gtk_tree_view_get_model(smfm->wid_treeview);
//        int num = smfm->number;
        gint num = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL);
        GList *strlist = NULL;
        GList *filelist = NULL;

        GSList *flists =  gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dlg));
        smfm->lastDir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dlg));
        for(; flists; flists = flists->next,num++)
        {
            SaveMusicFile *smf = g_new0(SaveMusicFile,1);
            smf->index = num;
            smf->file_addr = num + offset;
            smf->full_name = g_strdup((gchar *)flists->data);
            gchar **split = g_strsplit(smf->full_name,"\\",-1);
            int len = g_strv_length(split);
            smf->base_name = g_strdup(split[len-1]);
            g_strfreev(split);
            smf->down_name = g_strdup_printf("file_%d",smf->index);
            filelist = g_list_append(filelist,smf);
            strlist = g_list_append(strlist,smf->base_name);
//            factory_file_manager_append(smfm->wid_clist,smf);
        }

        if(num == 0)
        {
            smfm->filelist = g_list_copy(filelist);
            smd->cboxlist = g_list_copy(strlist);
        }
        else if(smfm->man_opt == OPT_APPEND)
        {
            GList *p = filelist;
            for(; p ; p = p->next)
                smfm->filelist = g_list_append(smfm->filelist,p->data);
            p = strlist;
            for(; p; p = p->next)
                smd->cboxlist = g_list_append(smd->cboxlist,p->data);
        }
        else
        {
            /*　新插入的文件　*/
            GtkTreeSelection *selection = gtk_tree_view_get_selection (smfm->wid_treeview);
            GtkTreeIter iter;
            int pos = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL);;
            if (gtk_tree_selection_get_selected (selection, NULL, &iter))
            {
                gtk_list_store_remove(selection,&iter);
                gtk_tree_model_get(model,&iter,0,&pos,-1);
            }

//            int pos = smfm->selected;
            GList *p = filelist;
            for(; p; p = p->next)
                smfm->filelist = g_list_insert(smfm->filelist,p->data,pos);
            g_list_free1(smd->cboxlist);
            smd->cboxlist = NULL;
            p = smfm->filelist;
            for(; p ; p = p->next)
            {
                SaveMusicFile *smf = p->data;
                smd->cboxlist = g_list_append(smd->cboxlist,smf->base_name);
            }

        }
        if(smfm->man_opt == OPT_INSERT)
        {
            factory_file_manager_refresh_all_iter(smd);
//            factory_file_manager_refresh_all(smd);
        }
        else
        {
            GList *p = filelist;
            for(; p; p = p->next)
                factory_mfile_manager_append_iter(model,p->data);
//                factory_file_manager_append(smfm->wid_clist,p->data);
        }
//        smfm->number = g_list_length(smfm->filelist);
        smd->mfmos->m_itemadded(smd); /* 更新左边的界面 */

    }
//    gtk_widget_destroy(dlg);
}


void factory_read_mfile_filelist_from_xml(ObjectNode obj_node)
{
    SaveMusicDialog *smd = curLayer->smd;
    AttributeNode attr_node = obj_node;
    while(attr_node = data_next(attr_node))
    {
        xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"name");
        if(key)
        {
            if(!g_ascii_strcasecmp((gchar*)key,"Music_File"))
            {
//                if(!smd->smfm)
//                {
//                    smd->smfm = g_new0(SaveMusicFileMan,1);
//                    gchar **split = g_strsplit(factoryContainer->system_files,",",-1);
//                    smd->smfm->offset = g_strv_length(split);
//                    g_strfreev(split);
////                        smd->smfm->selected = -1;
//                }

                factory_read_file_list_from_xml(attr_node->xmlChildrenNode); /* 读文件列表 */
//                    factory_fm_get_cboxlist(smd);
                break;
            }
            /* 这里读取左边布局的数据 ,ID列表*/
            SaveMusicItem *smi = g_new0(SaveMusicItem,1);
            smi->id_index = g_strtod((gchar*)key,NULL);

            key = xmlGetProp(attr_node,(xmlChar *)"addr");
            smi->id_addr = g_strtod((gchar*)key,NULL);
//                    sss->name = g_strdup(fst->Name);
            key = xmlGetProp(attr_node,(xmlChar *)"active");

            smi->active = g_strtod((gchar*)key,NULL);


            key = xmlGetProp(attr_node,(xmlChar *)"idname");
            if(!key)
            {
                smi->fname = g_strdup("");
            }
            else
            {
                smi->fname = g_strdup((gchar*)key);
            }
            smi->dname = g_strdup("");
            smd->itemlist = g_list_append(smd->itemlist,smi);
        }
        xmlFree(key);
    }
}

void factory_read_file_list_from_xml(ObjectNode obj_node)
{
    /*　这里读文件列表　*/
    SaveMusicDialog *smd = curLayer->smd;
    SaveMusicFileMan  *smfm = smd->smfm;
    AttributeNode attr_node = obj_node;
    while(attr_node = data_next(attr_node))
    {
        xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"name");
        if(!key) continue;

        SaveMusicFile *smf = g_new0(SaveMusicFile,1);
        smf->index = g_strtod((gchar*)key,NULL);

        key = xmlGetProp(attr_node,(xmlChar *)"fname");
        smf->full_name = g_strdup((gchar*)key);

        key = xmlGetProp(attr_node,(xmlChar *)"addr");
        smf->file_addr = g_strtod((gchar*)key,NULL);

        key = xmlGetProp(attr_node,(xmlChar *)"dname");
        smf->down_name = g_strdup((gchar*)key);
//                    sss->name = g_strdup(fst->Name);
        gchar **split = g_strsplit(smf->full_name,"\\",-1);
        int len = g_strv_length(split);
        smf->base_name = g_strdup(split[len-1]);
        g_strfreev(split);
        smfm->filelist = g_list_append(smfm->filelist,smf);
        smd->cboxlist = g_list_append(smd->cboxlist,smf->base_name);

        xmlFree(key);

    }
    if(!attr_node)
        return;
}

void factory_save_mfile_dialog_to_xml(SaveStruct *sss,ObjectNode obj_node)
{
    SaveMusicDialog *smd = curLayer->smd;
    gchar *lastsection = factory_get_last_section(sss->name,".");
    factory_music_fm_get_type(lastsection);

    ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_item", NULL);
    xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)sss->name);
    xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
    xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)sss->type);

    xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)sss->value.vnumber);
    gchar *idname  = g_strdup("");
    int vnumber = g_strtod(sss->value.vnumber,NULL);
    SaveMusicFileMan  *smfm = smd->smfm;
    if(smfm->filelist) /* 空指针就不保存这一项 */
    {

        GtkTreeIter iter;
//        GtkTreeModel *model = gtk_tree_view_get_model(smd->smfm->wid_treeview);
        if(smd->fmst == INDEX)
        {
            SaveMusicItem *smi  = g_list_nth_data(smd->itemlist,g_strtod(sss->value.vnumber,NULL));
            if(smi)
                idname = g_strdup(smi->fname);
        }
        else if(smd->fmst == SEQUENCE)
        {
            SaveMusicFile *smf = g_list_nth_data(smfm->filelist,g_strtod(sss->value.vnumber,NULL));
            if(smf)
                idname = g_strdup(smf->base_name);
        }
        else if(smd->fmst == PHY)
        {
            int pos = vnumber - smd->smfm->offset;
            if(pos > 0)
            {
                SaveMusicFile *smf = g_list_nth_data(smfm->filelist,pos);
                if(smf)
                    idname = g_strdup(smf->base_name);
            }

        }

    }
    xmlSetProp(ccc, (const xmlChar *)"idname", (xmlChar *)idname);
    if(!strlen(idname) && (smd->fmst == PHY) && (vnumber <  smd->smfm->offset))
    {
        g_free(sss->value.vnumber);
        sss->value.vnumber = g_strdup("-1");
        xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)sss->value.vnumber);
    }
    g_free(idname);
}


void factory_write_mfile_filelist(ObjectNode obj_node)
{

    SaveMusicDialog *smd = curLayer->smd;
    gchar *rows = g_strdup_printf("%d",g_list_length(smd->itemlist));
    xmlSetProp(obj_node, (const xmlChar *)"rows", (xmlChar *)rows);
    g_free(rows);
    GList *flist = smd->itemlist;
    for(; flist; flist = flist->next)
    {
        SaveMusicItem *smi = flist->data;
        ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_item", NULL);
        xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)g_strdup_printf("%d",smi->id_index));
        xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
        xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)g_strdup_printf("%d",smi->active ? smi->active-1 : -1));
        xmlSetProp(ccc, (const xmlChar *)"addr", (xmlChar *)g_strdup_printf("%d",smi->id_addr));
        xmlSetProp(ccc, (const xmlChar *)"active", (xmlChar *)g_strdup_printf("%d",smi->active));
        xmlSetProp(ccc, (const xmlChar *)"idname", (xmlChar *)smi->fname);
    }

    ObjectNode newobj = xmlNewChild(obj_node, NULL, (const xmlChar *)"Music_File", NULL);

    xmlSetProp(newobj, (const xmlChar *)"name",
               (xmlChar *)"Music_File");
    g_return_if_fail(smd->smfm); /* 没有音乐文件　*/
//        xmlSetProp(newobj, (const xmlChar *)"offset",
//                   (xmlChar *)g_strdup_printf("%d",smd->smfm->offset));
    flist = smd->smfm->filelist;
    for(; flist; flist = flist->next) /* 这里保存文件列表 */
    {
        SaveMusicFile *smf = flist->data;
        ObjectNode tnode = xmlNewChild(newobj, NULL, (const xmlChar *)"file", NULL);
        xmlSetProp(tnode, (const xmlChar *)"name", (xmlChar *)g_strdup_printf("%d",smf->index));
        xmlSetProp(tnode, (const xmlChar *)"addr", (xmlChar *)g_strdup_printf("%d",smf->file_addr));
        xmlSetProp(tnode, (const xmlChar *)"fname", (xmlChar *)smf->full_name);
        xmlSetProp(tnode, (const xmlChar *)"dname", (xmlChar *)smf->down_name);
    }
}

GtkWidget *factory_music_file_manager_with_model(GtkWidget *parent,SaveMusicDialog *smd)
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
//    if(!smd->smfm)
//    {
//        smd->smfm = g_new0(SaveMusicFileMan,1);
//        /* 根据文件个数算偏移数 */
//        gchar **split = g_strsplit(factoryContainer->system_files,",",-1);
//        smd->smfm->offset = g_strv_length(split);
//        g_strfreev(split);
////        smd->smfm->selected = -1;
//    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spbox),smd->smfm->offset);
    SaveMusicFileMan *smfm  = smd->smfm;

    smfm->wid_store  = gtk_list_store_new(NUM_OF_MCOLUMN,G_TYPE_INT,G_TYPE_INT,G_TYPE_STRING,G_TYPE_STRING);
    smfm->wid_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(smfm->wid_store));
    if(smfm->filelist)
    {
        factory_file_manager_refresh_all_iter(smd);
        int n = -1;
        if(smd->fmst == SEQUENCE)
        {
            n = g_strtod(*smd->vnumber,NULL);
        }
        else if(smd->fmst == PHY)
        {
            n = g_strtod(*smd->vnumber,NULL)- smfm->offset;
        }
        factory_mfile_selection_row(n);
    }

    gtk_tree_view_set_headers_visible(smfm->wid_treeview,TRUE);

    factory_mfile_manager_add_columns(smfm->wid_treeview);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);
    gtk_container_add (GTK_CONTAINER (wid_idlist), smfm->wid_treeview);
    gtk_tree_view_set_grid_lines (smfm->wid_treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);

//    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(wid_idlist),smfm->wid_treeview);
    smfm->wid_offset = spbox;

    /* 创建四行布局*/
    gtk_box_pack_start(GTK_BOX(sdialog),hbox,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(sdialog),sep,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(sdialog),wid_idlist,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(sdialog),factory_music_file_manager_operator(smd),FALSE,FALSE,0);

    gtk_widget_show_all(sdialog);
//    g_signal_connect(clist,"select_row",G_CALLBACK(factory_music_file_manager_select_callback),&smd->smfm->selected);
    return sdialog;
}
