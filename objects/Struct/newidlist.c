#include "struct_class.h"
#include "object.h"
#define WIDTH 300
#define HEIGHT 500
static gchar *subopt[] = {"添加","插入","删除",NULL};
static void factory_create_sublist_dialog(GtkWidget *parent);
static GroupOfOperation mfmo[] =
    {
        {NULL,"添加",NULL,factory_create_sublist_dialog},
        {NULL,"编辑",NULL,NULL},
        {NULL,"插入",NULL,NULL},
        {NULL,"删除",NULL,NULL}

    };

typedef struct
{
    GtkWidget *left;
    GtkWidget *right;
} twoWidget;

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
    for(; actlist; actlist = actlist->next)
    {
        factory_append_iten_to_cbmodal(act_model,actlist->data);
    }
    gtk_container_add (GTK_CONTAINER (wid_idlist), act_treeview);
//    gtk_box_pack_start(GTK_BOX(vbox),sid->id_treeview,FALSE,FALSE,0);
    g_object_unref (act_model);

    return wid_idlist;
}




static void factory_append_item_to_sublist_model(GtkListStore *store,
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


static void factory_insert_item_to_sublist_model(GtkListStore *store,
        gchar *str,
        GtkTreeIter *sibling)
{
    GtkTreeIter iter;
    gint n = GTK_LIST_STORE(store)->length ;
    gtk_list_store_insert_after (store,&iter,sibling);
    gtk_list_store_set(store,&iter,
                       COLUMN_ITEM_SEQUENCE,n,
                       COLUMN_ITEM_ADDR,n*2,
                       COLUMN_ITEM_IDNAME,str,
                       -1);
}

static gboolean factory_sublist_revalue_foreach(GtkTreeModel *model,
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
            factory_append_item_to_sublist_model(sub_model,txt);
            g_free(txt);
        }
    }

}

static void factory_sublist_insert_callback(GtkWidget *btn,gpointer user_data)
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
        GtkTreeSelection *subsel = gtk_tree_view_get_selection(sub_treeview);
//        int scount = gtk_tree_selection_count_selected_rows (selection);
//        if(scount < 1)
//            return;
        GtkTreeIter liter;
        if(!gtk_tree_selection_get_selected(subsel,NULL,&liter))
            return;

        GList *treelist = gtk_tree_selection_get_selected_rows (selection,
                          &act_model);
        for(; treelist; treelist = treelist->next)
        {
            gchar *txt;
            gtk_tree_model_get_iter (act_model,&iter,treelist->data);
            GtkTreeIter siter;
            gtk_tree_model_get(act_model,&iter,0,&txt,-1);
            factory_insert_item_to_sublist_model(sub_model,txt,&liter);
            gtk_tree_model_iter_next (sub_model,&liter);

            g_free(txt);
        }
        gtk_tree_model_foreach(sub_model,factory_sublist_revalue_foreach,NULL);
    }
}


static void factory_sublist_delete_callback(GtkWidget *btn,gpointer user_data)
{
    GtkTreeView *sub_treeview = g_object_get_data(user_data,"sub_treeview");
    GtkTreeModel *sub_model = gtk_tree_view_get_model(sub_treeview);
    GtkTreeIter iter;
    GtkTreeSelection *selection = gtk_tree_view_get_selection (sub_treeview);
    if(gtk_tree_selection_get_selected(selection,NULL,&iter))
    {
        gtk_list_store_remove (GTK_LIST_STORE (sub_model), &iter);
    }
    gtk_tree_model_foreach(sub_model,factory_sublist_revalue_foreach,NULL);
}


/*中间的操作 */

static GtkWidget *factory_sublist_operators(GtkWidget *left,
        GtkWidget *right)
{
    stw.left = left;
    stw.right = right;

    GtkWidget *mainBox = gtk_vbox_new(FALSE,0);
    GtkWidget *btn =
        gtk_button_new_with_label(factory_utf8("添加"));
    g_object_set_data(G_OBJECT(mainBox),"append",btn);
    gtk_box_pack_start(GTK_BOX(mainBox),btn,FALSE,TRUE,0);


    g_signal_connect(btn,"clicked",
                     G_CALLBACK(factory_sublist_append_callback),&stw);


    btn = gtk_button_new_with_label(factory_utf8("插入"));
    g_object_set_data(G_OBJECT(mainBox),"insert",btn);
    gtk_box_pack_start(GTK_BOX(mainBox),btn,FALSE,TRUE,0);
    g_signal_connect(btn,"clicked",
                     G_CALLBACK(factory_sublist_insert_callback),&stw);

    btn = gtk_button_new_with_label(factory_utf8("删除"));
    g_object_set_data(G_OBJECT(mainBox),"delete",btn);
    gtk_box_pack_start(GTK_BOX(mainBox),btn,FALSE,TRUE,0);
    g_signal_connect(btn,"clicked",
                     G_CALLBACK(factory_sublist_delete_callback),left);
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
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("行为名称"),
             renderer,
             "text",
             COLUMN_ITEM_IDNAME,
             NULL);
//    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ITEM_ADDR);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);

}


static void factory_set_idlist_columns (GtkTreeView *treeview,GtkTreeModel *cbmodel)
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

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (factory_utf8("行为表名称"),
             renderer,
             "text",
             COLUMN_ITEM_ADDR,
             NULL);
//    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ITEM_ADDR);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (treeview, column);

}

static void factory_create_sublist_dialog(GtkWidget *parent)
{
    GtkWidget *mainBox = gtk_vbox_new(FALSE,0);
    SaveIdDialog *sid = (SaveIdDialog *)curLayer->sid;

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
    g_object_set_data(G_OBJECT(wid_idlist),"sub_treeview",sub_treeview);

    gtk_tree_view_set_grid_lines (sub_treeview,GTK_TREE_VIEW_GRID_LINES_BOTH);

    factory_add_sublist_columns(sub_treeview,sub_model); /* 添加列 */

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (sub_treeview), TRUE);
    gtk_tree_selection_set_mode (
                                 gtk_tree_view_get_selection (GTK_TREE_VIEW (sub_treeview)),
                                 GTK_SELECTION_SINGLE);
    gtk_container_add (GTK_CONTAINER (wid_idlist),sub_treeview);


    GtkWidget *namehbox = gtk_hbox_new(FALSE,0);
    GtkWidget *label = gtk_label_new(factory_utf8("子表名称:"));
    GtkWidget *entry = gtk_entry_new();
    g_object_set_data(mainBox,"table_name",entry);
    gtk_box_pack_start(GTK_BOX(namehbox),label,FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(namehbox),entry,FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(mainBox),namehbox,FALSE,FALSE,0);



    /*第一行的一个水平布局*/
    GtkWidget *act_widget = factory_create_action_list_widget();
    GtkWidget *midhbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(midhbox),wid_idlist,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(midhbox),
                       factory_sublist_operators(wid_idlist,act_widget),
                       FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(midhbox),act_widget ,TRUE,TRUE,0);/* 右边是行为列表 */

    gtk_box_pack_start(GTK_BOX(mainBox),midhbox,TRUE,TRUE,0);
    g_object_unref (sub_model);

//    gtk_box_pack_start(GTK_BOX(mainBox),factory_sublist_response_buttons,FALSE,FALSE,0);

//    gtk_box_pack_start(GTK_BOX(mainBox),factory_new_add_button(factory_add_item_to_idlist_model,sid),FALSE,FALSE,0);
    gtk_widget_show_all(subdig);
}

gboolean factory_idlist_popumenu(GtkTreeView *treeview,
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
                         (GCallback) G_CALLBACK(mfmo[n].mfmf),treeview);
        mfmo[n].btn = menuitem;
        g_object_set_data(menu,g_strdup_printf("%d",n),menuitem);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
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
    if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
    {

        GtkTreeSelection *selection;

        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

        /* Note: gtk_tree_selection_count_selected_rows() does not
         *   exist in gtk+-2.0, only in gtk+ >= v2.2 ! */
        if (gtk_tree_selection_count_selected_rows(selection)  <= 1)
        {
            GtkTreePath *path;

            /* Get tree path for row that was clicked */
            if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
                                              (gint) event->x,
                                              (gint) event->y,
                                              &path, NULL, NULL, NULL))
            {
                gtk_tree_selection_unselect_all(selection);
                gtk_tree_selection_select_path(selection, path);
                gtk_tree_path_free(path);
            }
        }

        factory_idlist_popumenu(treeview, event, userdata);

        return TRUE; /* we handled this */
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


//static GtkWidget *factory_idlist_operators(GtkTreeView *treeview)
//{
//    GtkWidget *mainBox = gtk_vbox_new(FALSE,0);
//    GtkWidget *btn =
//        gtk_button_new_with_label(factory_utf8("添加"));
//    g_object_set_data(G_OBJECT(mainBox),"append",btn);
//    gtk_box_pack_start(GTK_BOX(mainBox),btn,TRUE,TRUE,0);
//
//    g_signal_connect(btn,"clicked",
//                     G_CALLBACK(factory_sublist_append_callback),&tw);
//
//    btn = gtk_button_new_with_label(factory_utf8("编辑"));
//    g_object_set_data(G_OBJECT(mainBox),"insert",btn);
//    gtk_box_pack_start(GTK_BOX(mainBox),btn,TRUE,TRUE,0);
//    g_signal_connect(btn,"clicked",
//                     G_CALLBACK(factory_sublist_insert_callback),&tw);
//
//    btn = gtk_button_new_with_label(factory_utf8("插入"));
//    g_object_set_data(G_OBJECT(mainBox),"insert",btn);
//    gtk_box_pack_start(GTK_BOX(mainBox),btn,TRUE,TRUE,0);
//    g_signal_connect(btn,"clicked",
//                     G_CALLBACK(factory_sublist_insert_callback),&tw);
//
//    btn = gtk_button_new_with_label(factory_utf8("删除"));
//    g_object_set_data(G_OBJECT(mainBox),"delete",btn);
//    gtk_box_pack_start(GTK_BOX(mainBox),btn,TRUE,TRUE,0);
//    g_signal_connect(btn,"clicked",
//                     G_CALLBACK(factory_sublist_delete_callback),left);
//
//       btn = gtk_button_new_with_label(factory_utf8("确定"));
//    g_object_set_data(G_OBJECT(mainBox),"delete",btn);
//    gtk_box_pack_start(GTK_BOX(mainBox),btn,TRUE,TRUE,0);
//    g_signal_connect(btn,"clicked",
//                     G_CALLBACK(factory_sublist_delete_callback),left);
//
//       btn = gtk_button_new_with_label(factory_utf8("取消"));
//    g_object_set_data(G_OBJECT(mainBox),"delete",btn);
//    gtk_box_pack_start(GTK_BOX(mainBox),btn,TRUE,TRUE,0);
//    g_signal_connect(btn,"clicked",
//                     G_CALLBACK(factory_sublist_delete_callback),left);
//
//    return mainBox;
//}

gboolean
facotory_idtreeview_popup_callback (GtkWidget *treeview, gpointer userdata)
{
    factory_idlist_popumenu(treeview, NULL, userdata);

    return TRUE; /* we handled this */
}


void factory_create_idlist_dialog(GtkWidget *button,SaveStruct *sst)
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

    GtkTreeModel *idmodel;
    idmodel = gtk_list_store_new(NUM_OF_COLS,G_TYPE_INT,G_TYPE_STRING);

    GtkTreeView *idtreeview  = gtk_tree_view_new_with_model (idmodel);
    g_object_set_data(G_OBJECT(wid_idlist),"idtreeview",idtreeview);
    g_signal_connect(idtreeview,"button_press_event",
                     G_CALLBACK(factory_idtreeview_onButtonPressed),NULL);
//    g_signal_connect(idtreeview, "popup-menu",
//                     (GCallback)facotory_idtreeview_popup_callback, NULL);
    gtk_tree_view_set_grid_lines (idtreeview,GTK_TREE_VIEW_GRID_LINES_BOTH);

    factory_set_idlist_columns(idtreeview,idmodel); /* 添加列 */

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (idtreeview), TRUE);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (idtreeview)),
                                 GTK_SELECTION_SINGLE);
    gtk_container_add (GTK_CONTAINER (wid_idlist),idtreeview);
    g_object_unref (idmodel);

    gtk_box_pack_start(GTK_BOX(mainBox),wid_idlist,TRUE,TRUE,0);
//    gtk_box_pack_start(GTK_BOX(mainBox),
//                       factory_idlist_operators(idtreeview),FALSE,TRUE,0);


    gtk_widget_show_all(subdig);
//    g_signal_connect(G_OBJECT(subdig),"response",G_CALLBACK(factoy_idlist_response),sst);
//    gint ret = gtk_dialog_run(subdig); /* 阻塞方式运行 */
//    if(ret == GTK_RESPONSE_OK) /* 把判断保存做在这里,可以减变量的数量,与函数之间的传递. */
//    {
//
//    }
//    gtk_widget_destroy(subdig);
}
