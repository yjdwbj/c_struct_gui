#include "template_proc.h"
#include "sheet.h"



FactoryTemplateItem *static_temp = NULL;
FactoryStructItemList *static_fsil = NULL;



GtkTreeModel *model = NULL;

TemplateOps templops =
{
    factory_template_edit_callback,
    factory_template_save
};





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
//        int n = g_strtod(split[0],NULL);
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


//GList *newlist = NULL;

static GList* factory_template_get_editable_item(GList *oldlist)
{

    GList *tlist = oldlist;
    GList *nlist = NULL;
    for(; tlist; tlist = tlist->next)
    {
        SaveStruct *sst = tlist->data;
        if( sst->org->isSensitive && sst->org->isVisible) /* 保留与固定值就过滤掉了 */
        {
//            newlist = g_list_append(newlist,sst->org);
            NewItem *nitem = g_new0(NewItem,1);
            nitem->label = g_strdup(sst->org->Cname);
//            nitem->fixed = TRUE;
            nitem->expand = TRUE;
            nitem->children = NULL;
            nitem->tooltips = g_strdup(sst->org->Comment);
            nitem->pos = g_list_index(oldlist,sst);
            nitem->ischecked = TRUE;
            nlist = g_list_append(nlist,nitem);
        }
    }
    return nlist;
}


static void
item_toggled (GtkCellRendererToggle *cell,
              gchar                 *path_str,
              gpointer               data)
{
//  GtkTreeModel *model = (GtkTreeModel *)data;
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
//    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
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
    gtk_tree_store_append (GTK_TREE_STORE(model), &iter,NULL);
    gtk_tree_store_set(GTK_TREE_STORE(model),&iter,
                       ITEM_NAME,topitem->label,
                       ITEM_BOOL,FALSE,
                       ITEM_VISIBLE,FALSE,
                       -1);

    GList *plist = topitem->children;
    for(; plist; plist = plist->next)
    {
        GtkTreeIter child_iter;
        NewItem *childitem = plist->data;
        gtk_tree_store_append (GTK_TREE_STORE(model), &child_iter, &iter);
        gtk_tree_store_set(GTK_TREE_STORE(model),&child_iter,
                           ITEM_NAME,childitem->label,
                           ITEM_BOOL,childitem->ischecked,
                           ITEM_VISIBLE,TRUE,
                           -1);
    }
}

static gboolean factory_inverse_select_foreach(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
    gboolean toggle_item;
    /* get toggled iter */
    gtk_tree_model_get (model, iter, ITEM_BOOL, &toggle_item, -1);
    /* do something with the value */
    toggle_item ^= 1;
    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (model), iter, ITEM_BOOL,
                        toggle_item, -1);
    return FALSE;
}

gboolean factory_select_all_foreach(GtkWidget *model,
                                    GtkWidget *path,
                                    GtkTreeIter *iter,
                                    gpointer data)
{
    gboolean toggle_item = TRUE;
    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (model), iter, ITEM_BOOL,
                        toggle_item, -1);
    return FALSE;
}

void factory_template_inverse_select_callback(GtkWidget *btn,gpointer user_data)
{
    gtk_tree_model_foreach(model,factory_inverse_select_foreach,NULL);
}

void factory_template_select_all_callback(GtkWidget *btn,gpointer user_data)
{
    gtk_tree_model_foreach(model,factory_select_all_foreach,NULL);
}

/*界面控件布局*/
static void factory_template_layout(GtkWidget *dialog,GtkWidget *parent)
{
    GtkWidget *tooltips = gtk_tooltips_new();
    GtkWidget *hbox = gtk_hbox_new(FALSE,1);
    GtkWidget *label = gtk_label_new(factory_utf8("模版名字:"));
    GtkWidget *entry = gtk_entry_new();/* 时间戳做默认名字*/
//    GDateTime *gdt =  g_date_time_new_now_utc();
    gchar **split = g_strsplit(ddisplay_active_diagram()->filename,G_DIR_SEPARATOR_S,-1);
    int len = g_strv_length(split);
    gchar*  pth = strrchr(split[len-1],'.');
    if (pth)
    {
        *(pth) = 0;
    }
    gtk_entry_set_text(GTK_ENTRY(entry),split[len-1]);

    g_strfreev(split);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    g_object_set_data(G_OBJECT(dialog),"template_name",entry);
    gtk_box_pack_start (GTK_BOX (parent), hbox, FALSE, TRUE, 0);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),entry,factory_utf8("强烈建议改成有意义的名字,可输入中文!"),NULL);


    hbox = gtk_hbox_new(FALSE,1);
    label = gtk_label_new(factory_utf8("结构体名(全局唯一):"));
    entry = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),entry,factory_utf8("结构体名(全局唯一),结构体命名规则"),NULL);
    gtk_entry_set_text(GTK_ENTRY(entry),"TEMPLATE");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    g_object_set_data(G_OBJECT(dialog),"struct_name",entry);
    gtk_box_pack_start (GTK_BOX (parent), hbox, FALSE, TRUE, 0);


    hbox = gtk_hbox_new(FALSE,1);
    label = gtk_label_new(factory_utf8("保存文件名:"));
    entry = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips),entry,factory_utf8("保存文件时,指定文件名"),NULL);
    gtk_entry_set_text(GTK_ENTRY(entry),"act.inf");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    g_object_set_data(G_OBJECT(dialog),"sfile",entry);
    gtk_box_pack_start (GTK_BOX (parent), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE,1);
    label = gtk_label_new(factory_utf8("模版入口行为:"));
    entry = gtk_combo_box_new_text();
    GList *names = factory_get_objects_from_layer(curLayer);
    for(; names; names = names->next)
    {
        gtk_combo_box_append_text(GTK_COMBO_BOX(entry),names->data);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(entry),0);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    g_object_set_data(G_OBJECT(dialog),"entry_point",entry);
    gtk_box_pack_start (GTK_BOX (parent), hbox, FALSE, TRUE, 0);

    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
    g_object_set_data(G_OBJECT(dialog),"sroll_win",wid_idlist);
    gtk_widget_set_size_request(wid_idlist,300,500);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);

    gtk_box_pack_start (GTK_BOX (parent), wid_idlist, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE,1);
    GtkWidget *btn = gtk_button_new_with_label(factory_utf8("反选"));
    gtk_box_pack_start(GTK_BOX(hbox),btn,FALSE,TRUE,0);
    g_signal_connect(G_OBJECT(btn),"clicked",G_CALLBACK(factory_template_inverse_select_callback),NULL);

    btn = gtk_button_new_with_label(factory_utf8("全选"));
    gtk_box_pack_start(GTK_BOX(hbox),btn,FALSE,TRUE,0);
    g_signal_connect(G_OBJECT(btn),"clicked",G_CALLBACK(factory_template_select_all_callback),NULL);

    gtk_box_pack_start (GTK_BOX (parent), hbox, TRUE, TRUE, 0);
}

void factory_template_update_item(const gchar *act_name)
{
    NewItem *topitem = factory_template_find_old_item(static_temp->modellist,act_name);
    if(topitem)
    {
        static_temp->modellist =  g_slist_remove(static_temp->modellist,topitem);
        g_free(topitem);
    }
}

static NewItem *factory_template_find_old_item(GSList *slist,const gchar *str)
{
    GSList *sslist = slist;
    GQuark q1  = g_quark_from_string(str);
    for(; sslist; sslist = sslist->next)
    {
        NewItem *topitem = sslist->data;
        if(topitem->name_quark == q1)
        {
            return topitem;
        }
    }
    return NULL;
}


GtkTreeView * factory_template_create_treeview(GList *list)
{
    model = GTK_TREE_MODEL(gtk_tree_store_new(ITEM_COUNT,
                           G_TYPE_STRING,
                           G_TYPE_BOOLEAN,
                           G_TYPE_BOOLEAN));

    DiaObjectType *ct = object_get_type(CLASS_LINE);
    GList *objects = list;

    for(; objects; objects = objects->next) /* 按控件个数,树状显示 */
    {
        gchar *name = objects->data;
        NewItem *topitem = factory_template_find_old_item(static_temp->modellist,name);
        if(!topitem)
        {
            topitem =  g_new0(NewItem,1);
            static_temp->modellist = g_slist_append(static_temp->modellist,topitem);
            topitem->label = g_strdup(name);
            topitem->name_quark = g_quark_from_string(topitem->label);
//            topitem->fixed = FALSE;
            topitem->expand = FALSE;
            topitem->pos = g_list_index(list,name);
            STRUCTClass *fclass = g_hash_table_lookup(curLayer->defnames,name);
            topitem->children = factory_template_get_editable_item(fclass->widgetSave);
        }
        factory_append_struct_to_model(model,topitem);
    }

    GtkTreeView *treeview = gtk_tree_view_new_with_model(model);
    g_object_unref (model);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)),
                                 GTK_SELECTION_SINGLE);
    gtk_tree_view_set_grid_lines (treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);
    factory_add_newstruct_columns(treeview);
    gtk_tree_view_set_headers_visible(treeview,TRUE);

    g_signal_connect (treeview, "realize",
                      G_CALLBACK (gtk_tree_view_expand_all), NULL);
//    g_object_set(treeview,"has-tooltip",TRUE,NULL);
//    g_signal_connect(G_OBJECT(treeview),"query-tooltip",G_CALLBACK(query_tooltip_tree_view_cb),NULL);
    return treeview;
}

void factory_template_edit_callback(GtkAction *action)
{
    Diagram *diagram  = ddisplay_active_diagram();

    if(static_temp != diagram->templ_item)
        static_temp = diagram->templ_item;

    if(static_fsil != &diagram->templ_item->fsil)
        static_fsil = &diagram->templ_item->fsil;

    GList *objects = factory_get_objects_from_layer(curLayer) ; /* 选择的控件列表 */
    GtkWidget *vbox = gtk_vbox_new(FALSE,1);
    GtkWidget *newdialog = factory_create_new_dialog_with_buttons(factory_utf8("模版编辑"),ddisplay_active()->shell);
    gtk_window_set_modal(GTK_WINDOW(newdialog),TRUE);
    GtkWidget *dlgvbox = GTK_DIALOG(newdialog)->vbox;
    GtkTreeView *treeview = factory_template_create_treeview(objects);
    factory_template_layout(newdialog,vbox);
    GtkWidget *sroll_win = g_object_get_data (G_OBJECT(newdialog),"sroll_win");
    gtk_container_add(GTK_CONTAINER(sroll_win),GTK_WIDGET(treeview));
    gtk_container_add(GTK_CONTAINER(dlgvbox),vbox);
    // factory_setback_values(newdialog);
    gtk_widget_show_all(vbox);
    g_signal_connect(newdialog,"response",G_CALLBACK(factory_template_save_dialog),model);
}


static gboolean factory_restore_items_values(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
//         GSList *plist = (GSList*)data;
    int *indces = gtk_tree_path_get_indices(path);
    int n1 = indces[0];
    int n2 = indces[1];
    NewItem *item = g_slist_nth_data(static_temp->modellist,n1); /* 所在顶层结点*/
    NewItem *citem = g_list_nth_data(item->children,n2);
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, ITEM_BOOL,
                        citem->ischecked, -1);

}


//static void factory_setback_values(GtkWidget *widget)
//{
//
//    if(static_temp)
//    {
//        GtkWidget *entry = g_object_get_data (G_OBJECT(widget),"template_name");
//        gtk_entry_set_text(GTK_ENTRY(entry),static_fsil->vname);
//        entry = g_object_get_data(G_OBJECT(widget),"struct_name");
//        gtk_entry_set_text(GTK_ENTRY(entry),static_fsil->sname);
//        entry = g_object_get_data(G_OBJECT(widget),"sfile");
//        gtk_entry_set_text(GTK_ENTRY(entry),static_fsil->sfile);
////        factory_setback_model_values(topList);
//        gtk_tree_model_foreach(model,factory_restore_items_values,NULL);
//    }
//}



//static void factory_setback_model_values(GSList *mlist)
//{
//    GSList *plist = mlist;
//    GtkTreeIter iter;
//    int t1=0;
//    for(; plist; plist = plist->next,t1++)
//    {
//        NewItem *topitem  = plist->data;
//        GList *clist = topitem->children;
//
////        FactoryItemInOriginalMap *fiiom = g_list_nth_data(static_temp->templlist,t1);
////        if(!fiiom)
////            continue;
//        int c1=0;
//        for(; clist; clist = clist->next,c1++)
//        {
//            NewItem *citem = clist->data;
//            GtkTreePath *path;
//            path = gtk_tree_path_new_from_indices(t1,c1, -1);
//            gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
//            gtk_tree_store_set (GTK_TREE_STORE (model), &iter, ITEM_BOOL,
//                                citem->ischecked, -1);
//            gtk_tree_path_free(path);
//        }
//    }
//}

static int factory_find_item_pos(GSList *list,const gchar *str)
{
    int pos = -1;
    GList *tlist = list;
    for(; tlist ; tlist = tlist->next)
    {
        if(!g_ascii_strcasecmp(tlist->data,str))
        {
            pos = g_slist_index(list,tlist->data);
            break;
        }
    }
    return pos;
}


static gboolean factory_filter_checked_item(GtkTreeModel *model,
        GtkTreePath *path,
        GtkTreeIter *iter,
        gpointer data)
{
//    FactoryStructItemList *fssl  = (FactoryStructItemList *)data;
    gboolean toggle_item = TRUE;
    gtk_tree_model_get(model,iter,ITEM_BOOL,&toggle_item,-1);
//    gchar *pathstring = gtk_tree_path_to_string(path);
    int depth = gtk_tree_path_get_depth(path);
    int *indces = NULL;
    indces = gtk_tree_path_get_indices(path);
    int n1 = indces[0];
    int n2 = indces[1];
    if(depth==2) /* depth == 2 */
    {
        /* 保存每一个项的状态 */
        NewItem *item = g_slist_nth_data(static_temp->modellist,n1); /* 所在顶层结点*/
        NewItem *citem = g_list_nth_data(item->children,n2);
        citem->ischecked = toggle_item;
    }

    return FALSE;
}


void factory_template_save_dialog(GtkWidget *widget,gint       response_id,gpointer   user_data)
{
    if(response_id == GTK_RESPONSE_OK)
    {
        GtkWidget *entry = g_object_get_data (G_OBJECT(widget),"template_name");
        if(static_fsil->vname)
            g_free(static_fsil->vname);
        static_fsil->vname = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
        entry = g_object_get_data(G_OBJECT(widget),"struct_name");
        if(static_fsil->sname)
            g_free(static_fsil->sname);
        static_fsil->sname = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
        static_fsil->isvisible = TRUE;
        entry = g_object_get_data(G_OBJECT(widget),"sfile");
        if(static_fsil->sfile)
            g_free(static_fsil->sfile);
        static_fsil->sfile = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
        if(static_fsil->list)
            g_list_free1(static_fsil->list);
//        static_fsil->list = structlist;
        static_fsil->number = g_list_length(factoryContainer->structList);
//        static_fsil->isTemplate = TRUE;

//        g_hash_table_insert(factoryContainer->structTable,static_fsil->sname,static_fsil);
//        factoryContainer->structList = g_list_append(factoryContainer->structList,static_fsil);
        gtk_tree_model_foreach(model,factory_filter_checked_item,static_fsil);
//        factory_template_save(static_fsil);
    }
    gtk_widget_destroy(widget);
}

static void factory_template_free_templlist(GList *valist)
{
    GList *vlist = valist;
    for(; vlist; vlist = vlist->next)
    {
        NewItem *topitem = vlist->data;
        GList *chlist = topitem->children;
        for(; chlist; chlist = chlist->next)
        {
            NewItem *chitem = chlist->data;
            g_free(chitem);
        }
        g_list_free1(topitem->children);
        g_free(topitem);
    }
    g_list_free1(valist);
}


GList * factory_template_get_values_to_save()
{
    /* 取得那些是须要保存到文件的 */
    GList *mlist = static_temp->modellist;
    GList *vallist = NULL;
    int index = 0;
    for(; mlist; mlist = mlist->next,index++)
    {
        NewItem *topitem = mlist->data;
        FactoryItemInOriginalMap *fiiom;/* = g_list_nth_data(static_temp->templlist,index);*/

        fiiom = g_new0(FactoryItemInOriginalMap,1);
        fiiom->act_name = g_strdup(topitem->label);
        STRUCTClass *fclass = g_hash_table_lookup(curLayer->defnames,fiiom->act_name);
        if(fclass)
        {
            fiiom->struct_name =  g_strdup(fclass->element.object.name);
        }
        else
        {
            /* error */
            factory_critical_error_exit(factory_utf8(g_strdup_printf("找不到对像!\n名字:%s\n",
                                        fiiom->act_name)));
        }
        vallist = g_list_append(vallist,fiiom);

        GList *chlist = topitem->children;
        int cpos = 0;
        for(; chlist; chlist = chlist->next,cpos++)
        {
            NewItem *chitem = chlist->data;
            if(topitem->ischecked) /* 这一项是要对外开放编辑的 */
            {
                g_list_append(fiiom->itemslist,g_strdup_printf("%d",chitem->pos));
            }
        }
    }
    return vallist;
}


void factory_template_save(FactoryStructItemList *fssl)
{
    GList *vallist = factory_template_get_values_to_save();

    DiaObjectType *otype = object_get_type(CLASS_STRUCT);
    Point startpoint = {0.0,0.0};
    Handle *h1,*h2;
    Diagram *diagram  = ddisplay_active()->diagram;

    STRUCTClass *ntemp = otype->ops->create(&startpoint,(void*)fssl->number,&h1,&h2);
    ntemp->name = g_strdup(fssl->vname);
    ntemp->element.object.name = g_strdup(fssl->sname);
    ntemp->element.object.isTemplate = TRUE;
    ntemp->widgetSave = vallist;
    gchar *templatepath =  dia_get_lib_directory("template");
    curLayer->objects = g_list_append(curLayer->objects,ntemp);

    gchar*  pth = strrchr((char *)diagram->filename,G_DIR_SEPARATOR);
    if (pth)
    {
        *(pth+1) = 0;
    }
    gchar *newfname  = g_strdup_printf("%s%s.lcy",diagram->filename,fssl->vname);
    g_free(diagram->filename);
    diagram->filename = g_strdup(newfname);
    diagram_update_extents(diagram);
    g_free(newfname);
    diagram_data_raw_save(diagram->data,diagram->filename);
    curLayer->objects = g_list_remove(curLayer->objects,ntemp);
    factory_template_free_templlist(vallist); /* 保存到文件之后就把它干掉 */
}

static gboolean factory_template_has_sheet()
{
    gboolean flag = FALSE;
    GSList *sheetp = get_sheets_list();
    while(sheetp)
    {
        Sheet *sheet = sheetp->data;
        if(!g_ascii_strcasecmp(sheet->name,TYPE_TEMPLATE))
        {
            flag = TRUE;
        }

        sheetp = sheetp->next;
    }
    return flag;
}

void factory_template_manager()
{
    Sheet *sheet = NULL;
    if(factory_template_has_sheet)
    {

    }
    else
    {
        sheet = new_sheet(TYPE_TEMPLATE, "", "", SHEET_SCOPE_SYSTEM, NULL);
    }
}

void factory_template_write_to_xml(GList *templlist,ObjectNode obj_node)
{
    GList *pp = templlist;
    for(; pp ; pp = pp->next)
    {
        FactoryItemInOriginalMap *fiiom = pp->data;
//        if(!fiiom->itemslist) /* 如果没有选择任何一个,就不用保存了*/
//            continue;
        ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_item", NULL);
        xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)fiiom->struct_name);
        xmlSetProp(ccc, (const xmlChar *)"vname", (xmlChar *)fiiom->act_name);

        GList *slist = fiiom->itemslist;
        if(slist)
        {
            gchar *items = "";
            for(; slist; slist = slist->next)
            {
                items = g_strconcat(items,slist->data,",",NULL);
            }
            int l = strlen(items);
            items[l-1] = '\0';
            xmlSetProp(ccc, (const xmlChar *)"items", (xmlChar *)items);
            g_free(items);
        }
        else
            xmlSetProp(ccc, (const xmlChar *)"items", NULL);
    }
}

void factory_template_read_from_xml(STRUCTClass *fclass, ObjectNode attr_node)
{
    xmlChar *key;
    while(attr_node = data_next(attr_node))
    {
        FactoryItemInOriginalMap *fiiom =  g_new0(FactoryItemInOriginalMap ,1);
        key = xmlGetProp(attr_node,(xmlChar *)"name");
        if(key)
        {
            fiiom->act_name = g_strdup((gchar*)key);
            xmlFree(key);
        }
        else
        {
            /*error*/
            factory_critical_error_exit(factory_utf8(g_strdup_printf("读取模版出错,缺少name属性.\n 文件名:%s\t,行号:%d\n",
                                        attr_node->name,attr_node->line)));
        }

        key = xmlGetProp(attr_node,(xmlChar *)"vname");
        if(key)
        {
            fiiom->struct_name = g_strdup((gchar*)key);
            xmlFree(key);
        }
        else
        {
            /*error*/
            factory_critical_error_exit(factory_utf8(g_strdup_printf("读取模版出错,缺少vname属性.\n 文件名:%s\t,行号:%d\n",
                                        attr_node->name,attr_node->line)));
        }

        key = xmlGetProp(attr_node,(xmlChar*)"items");
        if(key)
        {
            gchar **split = g_strsplit((gchar*)key,",",-1);
            int n =0;
            while(split[n])
            {
                fiiom->itemslist =  g_slist_append(fiiom->itemslist,g_strdup(split[n++]));
            }
            xmlFree(key);
        }
        else
        {
//            factory_critical_error_exit(factory_utf8(g_strdup_printf("读取模版出错,缺少items属性.\n 文件名:%s\t,行号:%d\n",
//            attr_node->name,attr_node->line)));
        }
        fclass->widgetSave = g_list_append(fclass->widgetSave,fiiom);
    }
}
