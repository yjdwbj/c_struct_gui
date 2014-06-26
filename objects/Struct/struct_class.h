/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * File:    class.h
 *
 * Purpose: This is the interface file for the class icon and dialog.
 */

/** \file objects/STRUCT/class.h  Declaration of the 'STRUCT - Class' type */
#ifndef CLASS_H
#define CLASS_H

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "connection.h"
#include "widgets.h"

#include "struct.h"
#include "display.h"
#define DIA_OBJECT(x) (DiaObject*)(x)

/** The number of regular connectionpoints on the class (not cps for
 * attributes and operands and not the mainpoint). */
#define STRUCTCLASS_CONNECTIONPOINTS 8
/** default wrap length for member functions */
#define STRUCTCLASS_WRAP_AFTER_CHAR 40
/** default wrap length for comments */
#define STRUCTCLASS_COMMENT_LINE_LENGTH 40

/* The code behind the following preprocessor symbol should stay disabled until
 * the dynamic relocation of connection points (caused by attribute and
 * operation changes) is taken into account. It probably has other issues we are
 * not aware of yet. Some more information maybe available at
 * http://bugzilla.gnome.org/show_bug.cgi?id=303301
 *
 * Enabling 29/7 2005: Not known to cause any problems.
 * 7/11 2005: Still seeing problems after dialog update, needs work.  --LC
 * 18/1 2006: Can't make it break, enabling.
 */
#define STRUCT_MAINPOINT 1






Layer *curLayer;
static GQuark item_reserverd = 0;
typedef struct _STRUCTClass STRUCTClass;
typedef struct _STRUCTClassDialog STRUCTClassDialog;

/**
 * \brief Very special user interface for STRUCTClass parametrization
 *
 * There is a (too) tight coupling between the STRUCTClass and it's user interface.
 * And the dialog is too huge in code as well as on screen.
 */
struct _STRUCTClassDialog
{
    GtkWidget *dialog;
    GtkWidget *mainTable; // 2014-3-19 lcy 这里添一个表格,用来布局显示.
};

typedef enum{
    N_COLOR,  /* 正常的 */
    H_COLOR /* 高亮显示的 */
}ViewColor;

typedef struct  _FactoryClassDialog  FactoryClassDialog;

struct _FactoryClassDialog
{
    GtkWidget *dialog;

//  GList *itemsData;   // 2014-3-19 lcy 这里是自定项,用存储从文件读到的条目.
//  GList *enumList;    // 2014-3-19 lcy 这里用存储枚举的链表.
    FactoryStructItemAll *EnumsAndStructs ;// 2014-3-21 lcy 这里包含一个文件里的所有结构体.
    GtkWidget *mainTable; // 2014-3-19 lcy 这里添一个表格,用来布局显示.

};


typedef struct _SaveStruct SaveStruct;

typedef void  (*CloseWidgetAndSave)(GtkWidget *widget,gint response_id,gpointer user_data);
typedef void  (*CreateNewDialog)(gpointer item,SaveStruct *sst);



typedef enum
{
    ECOMBO, /* 下拉框  enum comobox */
    UCOMBO, /* union comobox */
    OCOMBO, /* object combox*/
    ENTRY, /* 文本 */
    SPINBOX,
    BBTN,
    UBTN,
    OBTN,/* 这里是按键按钮 */
    EBTN,/* 枚举也有数组 */
    LBTN /* 文件管理与ID管理 数组形式的按键 */
} CellType;

typedef struct _ActionId ActionID;
struct _ActionId
{
    int index; // comobox index
    gchar* value;
    gchar *pre_name;
    gchar *title_name;
    gpointer conn_ptr; /* 就指向对像指针，名字与ID都不能对应更改名字操作 */
};


typedef struct _ArrayBaseProp ArrayBaseProp ; /* 数组的基本属性 */
struct _ArrayBaseProp
{
    int row;
    int col;   /* default is 1 */
    int reallen;
};

typedef struct _IDListArg IDListArg; /* 创建ID list 的参数 */
struct _IDListArg
{
    gchar *value;  /* 这是一个指向其它指针*/
    GList *filist; /* 最后一列的要填充的链表 */
};


typedef struct _NextId NextID;
struct _NextId
{
    GList *itemlist;
    GList *actlist;
    GList *wlist; /* widget list */
    ArrayBaseProp *arr_base;
};

typedef struct _CheckSave CheckSave;
struct _CheckSave
{
    gchar *ObjName;
    gboolean isChecked;
};



typedef struct _SaveKV SaveKV;
struct _SaveKV
{
    gchar *value;
    gchar *vname;
    int radindex; /* radio index */
};

typedef struct _ListBtnArr ListBtnArr;
struct _ListBtnArr
{
    GtkWidget *widget1;
    gchar **vnumber;
};

typedef void (*OrderDisplayWidget)(GList *srclist); /* 这里要按顺序显示，变态*/

typedef struct _ListDlgArg ListDlgArg;
struct _ListDlgArg
{
    gchar *type;
    gpointer user_data;
    gboolean isArray;
    OrderDisplayWidget  odw_func;
    GList *vlist;

};

typedef struct _ListBtn ListBtn;
struct _ListBtn
{
    GList *vlist; /* save ListBtnArr */
    ArrayBaseProp *arr_base;/* 如果有数组 */
};


struct _SaveIdDialog
{
//    GtkWidget *parent_btn;
    GtkWidget *vbox;
    GList *idlists; /* SaveIdList 内容*/
    gchar *title;
//    SaveKV *skv;
    GList *flist; /* combox list */
    GtkWidget *idlist_menu; /* 这里显示一个共公的现有的列表 */
    GtkListStore *id_store;
    GtkListStore *id_cbmodel;
    GtkWidget *id_treeview;
};

//SaveIdDialog *IdDialog;


/* 文件管理界面的结构体　start*/

typedef struct _SaveMusicFile SaveMusicFile;
struct _SaveMusicFile
{
    int index;
    int file_addr;       /*加上偏移的地址*/
    gchar* base_name;
    gchar* full_name;
    gchar* down_name;   /*小机识别的名字*/

};

typedef struct _SaveMusicFileMan  SaveMusicFileMan;
struct _SaveMusicFileMan
{
//    int number;
    int offset;
//    int selected;
    gchar *lastDir;
    GtkWidget *opendlg;
    GList *filelist; /* 右边界面的链表　内容是　SaveMusicFile　*/
    GtkWidget *wid_offset;
    GtkTreeModel *wid_store;
    GtkTreeView *wid_treeview;
    enum
    {
        OPT_APPEND,
        OPT_INSERT
    } man_opt;
};

typedef   enum{
        SEQUENCE, /*序号*/
        INDEX, /*索引*/
        PHY/*物理号*/
}FMSaveType; /* 索引号 序号或者偏移量　*/;





typedef struct _MusicFileManagerOpts MusicFileManagerOpts; /* 专为这个界面做的一些操作函数集合*/


struct _SaveMusicDialog
{
    gchar *title;
    gchar *btnname;
    FMSaveType fmst;
    gchar **vnumber; /* 这个就是SaveStruct vnumber*/
    GList *itemlist; /* 左边界面的链表　内容是　SaveMusicItem */
    GList *cboxlist; /* 右边下载名的链表　内容是 gchar */
    GtkListStore *id_store;
    GtkTreeView  *id_treeview;
    GtkListStore *id_cbmodal;
    GtkWidget *window; /* 它本身的窗口*/
    SaveMusicFileMan *smfm; /* 右边界面 */
    MusicFileManagerOpts *mfmos;
};




typedef   void (*OpenDialog)(SaveMusicDialog*);
typedef   void (*ApplyDialog)(SaveMusicDialog*);

typedef   void (*Item_Added) (SaveMusicDialog*);
typedef   void (*Clear_All) (SaveMusicFileMan*);


struct _MusicFileManagerOpts
{
    OpenDialog  m_opendilog;
    ApplyDialog m_applydialog;
    Item_Added m_itemadded;
    Clear_All m_clearall;
    void      (*(unused[4]))(gpointer obj,...);
};



//SaveMusicDialog *MusicManagerDialog ;

typedef struct _SaveMusicItem SaveMusicItem;
struct _SaveMusicItem
{
    int id_index;
    int id_addr;
    int active;
    gchar *dname;
    gchar *fname;
};

typedef struct _SaveMusicItem SaveIdItem;


/* 文件管理界面的结构体　end*/

typedef struct _SaveEnum SaveEnum;
struct _SaveEnum
{
    GList *enumList;
    int index;
    gchar* evalue;
    gchar* width;
};

typedef struct _SaveEnumArr SaveEnumArr;
struct _SaveEnumArr
{
    GtkWidget *widget1;
    GtkWidget *widget2;
    SaveEnum *senum;
};

typedef struct _SaveUbtn SaveUbtn;
struct _SaveUbtn
{
    GList *structlist;
//    GHashTable *htoflist;
    GList* savelist;
};


typedef struct _SaveUnion SaveUnion;
struct _SaveUnion
{
    int index;
    gchar *curtext;
    gchar *curkey;
    GtkWidget *vbox;
    GtkWidget *comobox;
    GList *structlist;
    GHashTable* saveVal; //保存值的哈希表
};

typedef struct _SaveEntry SaveEntry;
struct _SaveEntry
{
    gboolean isString;
//    int row;
//    int col;   /* default is 1 */
//    int reallen; /* 实际长度比 9,10,4 不是8的倍数的. */
    ArrayBaseProp *arr_base;
    int width;
    gpointer data;
    GList *wlist;   /* GtkWidget List  */
};


typedef struct _NameIndex NameIndex;
struct _NameIndex
{
    gchar *title;
    int index;
};

typedef struct _SaveEbtn SaveEbtn;
struct _SaveEbtn
{
    gchar* width;
    ArrayBaseProp *arr_base;
    GList *ebtnslist; /* 枚举的数据源链表 */
    GList *ebtnwlist; /* 全部是枚举的控件  存放 SaveEnumArr的链表  */
};



typedef struct _SaveStruct
{
    GtkWidget *widget1;
    GtkWidget *widget2;
    gchar* type;
    gchar* name;
//    gchar* pname;  /* 上一级名字, NULL 就是最上级 */
    CellType celltype;
    gboolean isPointer; /* FALSE == pointer , TRUE = single*/
    gboolean isSensitive; /* 是否可编辑 */
    union
    {
        SaveEntry sentry; // entry value
        gchar *vnumber; // spinbox value or actionid max items
        SaveEnum senum;  // enum value;
        SaveUnion sunion; // 第二个值,指针类
        NextID nextid;  // 保存连线的 comobox;
        SaveUbtn ssubtn; /* 联合体按键 */
        SaveEbtn ssebtn; /* 枚举数组 */
        ListBtn slbtn;
    } value;
    FactoryStructItem *org;
    STRUCTClass *sclass; /* 它的最上层的对像 */
    CloseWidgetAndSave  *close_func; /* 指向保存函数 */
    CreateNewDialog *newdlg_func; /* 指向显示窗口的函数,也是用来做按键消息回调的函数 */
};

typedef struct _PublicSection PublicSection; /* 显示一些公共的信息 */
struct _PublicSection
{
    GtkWidget *wid_hasfinished;
    gboolean hasfinished;
    GtkWidget *wid_rename_entry;
    gchar *name;
};

/**
 * \brief The most complex object Dia has
 *
 * What should I say? Don't try this at home :)
 */
struct _STRUCTClass
{
    Element element; /**< inheritance */

    /** static connection point storage,  the mainpoint must be behind the dynamics in Element::connections */
#ifdef STRUCT_MAINPOINT
    ConnectionPoint connections[STRUCTCLASS_CONNECTIONPOINTS + 1];
#else
    ConnectionPoint connections[STRUCTCLASS_CONNECTIONPOINTS];
#endif

    /* Class info: */

    real line_width;
    real font_height;
    real abstract_font_height;
    real polymorphic_font_height;
    real classname_font_height;
    real abstract_classname_font_height;
    real comment_font_height;

    DiaFont *normal_font;
//  DiaFont *abstract_font;
//  DiaFont *polymorphic_font;
    DiaFont *classname_font;
//  DiaFont *abstract_classname_font;
//  DiaFont *comment_font;

    char *name;
// char *stereotype; /**< NULL if no stereotype */
// char *comment; /**< Comments on the class */
//  int abstract;
//  int suppress_attributes;
//  int suppress_operations;
//  int visible_attributes; /**< ie. don't draw strings. */
//  int visible_operations;
//  int visible_comments;

//  int wrap_operations; /**< wrap operations with many parameters */
//  int wrap_after_char;
//  int comment_line_length; /**< Maximum line length for comments */
//  int comment_tagging; /**< bool: if the {documentation = }  tag should be used */

    Color line_color;
    Color fill_color;
    Color text_color;


    /** Attributes: aka member variables */
//  GList *attributes;

    /** Operators: aka member functions */
//  GList *operations;

    /** Template: if it's a template class */
//  int template;
    /** Template parameters */
//  GList *formal_params;

    /* Calculated variables: */

    real namebox_height;
//  char *stereotype_string;

//  real attributesbox_height;
//
//  real operationsbox_height;
    /*
      GList *operations_wrappos;*/
//  int max_wrapped_line_width;

//  real templates_height;
//  real templates_width;

    /* Dialog: */
    STRUCTClassDialog *properties_dialog;

    /** Until GtkList replaced by something better, set this when being
     * destroyed, and don't do structclass_calculate_data when it is set.
     * This is to avoid a half-way destroyed list being updated.
     */
    gboolean isInitial; /* 初始化属性对话框*/
    gboolean hasIdnumber;
    gboolean destroyed;
    gint depth; /* 这里是查找高亮显示的深度,零就不深入 */
    ViewColor vcolor;
    GHashTable *widgetmap; // 2014-3-22 lcy 这里用一个哈希表来保存界面上所有的值。
    GList *widgetSave; // 2014-3-22 lcy 这里为顺序显示用链表保存.
    FactoryStructItemAll *EnumsAndStructs ;// 2014-3-21 lcy 这里包含一个文件里的所有结构体.
    PublicSection *pps;
};

void structclass_dialog_free (STRUCTClassDialog *dialog);
extern GtkWidget *structclass_get_properties(STRUCTClass *structclass, gboolean is_default);
extern ObjectChange *structclass_apply_props_from_dialog(STRUCTClass *structclass, GtkWidget *widget);
extern void structclass_calculate_data(STRUCTClass *structclass);
extern void structclass_update_data(STRUCTClass *structclass);
static int
attributes_update_event(GtkWidget *widget, GdkEventFocus *ev, STRUCTClass *structclass);
static void
attributes_update(GtkWidget *widget, STRUCTClass *structclass);

static void factory_base_item_save(SaveStruct *sss,ObjectNode ccc);

static void
attributes_list_selection_changed_callback(GtkWidget *gtklist,
        STRUCTClass *structclass);
static void factory_connection_two_object(STRUCTClass *fclass, /* start pointer*/
        STRUCTClass *objclass /* end pointer */);



extern void structclass_sanity_check(STRUCTClass *c, gchar *msg);


gboolean factory_find_array_flag(const gchar *data);

static  void factory_calculate_data(STRUCTClass *structclass);

GtkWidget *
factory_get_properties(STRUCTClass *structclass, gboolean is_default);

GtkWidget *factory_create_many_entry_box(SaveStruct *sss);
GtkWidget *factory_create_many_checkbox(SaveStruct *sss);

void factory_create_basebutton_dialog(GtkWidget *button,SaveStruct *sss);
void factory_create_unionbutton_dialog(GtkWidget *button,SaveStruct *sst);
void factory_create_checkbuttons_by_list(GtkWidget *button,SaveStruct *sst);
void factory_create_objectbutton_dialog(GtkWidget *button,SaveStruct *sst);
void factory_create_enumbutton_dialog(GtkWidget *button,SaveStruct *sst);
void factory_save_enumbutton_dialog(GtkWidget *widget,gint response_id,gpointer user_data);
void factory_save_objectbutton_dialog(GtkWidget *widget,gint  response_id,gpointer   user_data);
void factory_save_unionbutton_dialog(GtkWidget *widget,gint       response_id,gpointer   user_data);
void factory_save_basebutton_dialog(GtkWidget *widget,gint       response_id,gpointer   user_data);
void factory_create_io_port_dialog(GtkWidget *button,SaveStruct *sst);


void factory_inital_ebtn(SaveStruct *sss,const FactoryStructItem *fst);
//void factory_inital_io_port_ebtn(SaveStruct *sss,const FactoryStructItem *fst);


GList* factory_get_objects_from_layer(Layer *layer);
//STRUCTClass* factory_get_object_from_layer(Layer *layer,const gchar *name);
static void factory_get_value_from_comobox(STRUCTClass *startclass,GtkWidget *comobox,ActionID *aid);

void factory_strjoin(gchar **dst,const gchar *prefix,const gchar *sep);
void factoy_changed_item(gpointer item,gpointer user_data);
STRUCTClass *factory_find_diaobject_by_name(Layer *curlayer,const gchar *name);
DiaObject *factory_find_same_diaobject_via_glist(GList *flist,GList *comprelist);

void factory_create_and_fill_dialog(STRUCTClass *structclass, gboolean is_default);
void factory_append_public_info(GtkWidget *dialog,STRUCTClass *structclass);
void factory_create_struct_dialog(GtkWidget *dialog,GList *datalist);

gboolean factory_is_connected(ConnectionPoint *cpend,ConnectionPoint *cpstart);

void factory_set_savestruct_widgets(SaveStruct *sss);
SaveStruct * factory_get_savestruct(FactoryStructItem *fst);


gchar* factory_entry_check(gchar* str);
void factory_editable_insert_callback(GtkEntry *entry,
                                      gchar* new_text,
                                      gint new_length,
                                      gpointer position,
                                      gpointer data);

void factory_editable_delete_callback(GtkEditable *editable,
                                      gint start_pos,
                                      gint end_pos);
void factory_editable_active_callback(GtkEditable *edit,gpointer data);


extern ObjectChange *
factory_apple_props_from_dialog(STRUCTClass *structclass, GtkWidget *widget);


void factory_create_toolbar_button(const gint8 *icon,gchar *tips,GtkToolbar  *toolbar,
                                   gpointer *callback);



void factory_create_file_manager_dialog(GtkWidget *btn,ListDlgArg *lda);
void factory_file_manager_dialog(GtkWidget *btn,SaveStruct *sst);
void factory_create_list_array_manager_dialog(GtkWidget *btn,SaveStruct *sst);
void factory_save_list_array_manager_dialog(GtkWidget *widget,gint       response_id,gpointer user_data);

gboolean factory_search_connected_link(STRUCTClass *structclass,gint depth);


void factory_select(STRUCTClass *structclass, Point *clicked_point,
                           DiaRenderer *interactive_renderer);

typedef void (*factory_button_callback)(GtkWidget *self);

void factory_add_item_to_music_manager(GtkButton *self,gpointer user_data);
GtkWidget *factory_new_add_button(factory_button_callback *callback,gpointer list);
GtkWidget *factory_get_new_iditem(SaveIdItem *swt,GList *flist);
GtkWidget *factory_get_new_musicitem( SaveMusicItem *swt,GList *fillist);


GtkWidget *factory_music_file_manager_with_model(GtkWidget *parent,SaveMusicDialog *smd);

GtkWidget *factory_music_file_manager(GtkWidget *parent,SaveMusicDialog *smd);
GtkWidget *factory_download_file_manager(GtkWidget *parent,SaveMusicDialog *smd);



void factory_add_item_to_idlist(GtkButton *self,gpointer user_data);
void factory_delete_last_item(GtkButton *self,gpointer user_data);




void factory_set_original_class(STRUCTClass *fclass);

gboolean factory_music_fm_get_type(const gchar* str);

void factory_fm_get_cboxlist(SaveMusicDialog *smd);

void factory_update_ActionId_object(GtkWidget *comobox,ActionID *aid,GList *clist);

void factory_setting_systemdata(STRUCTClass *structclass);

void factory_systeminfo_create();
void factory_systeminfo_apply_dialog(GtkWidget *widget,gint       response_id,gpointer   user_data);
void factory_systeminfo_load();
void factory_system_dialog(GList *list,GtkWidget *parent);

void factory_systeminfo_callback(GtkWidget *parent);
GList* factory_get_download_name_list(const gchar *path);

//void factoryReadDataFromFile(STRUCTClass *structclass);

void factory_reset_object_color_to_default();
void factory_set_fill_color();

/*ID LIST*/
void factory_new_idlist_dialog(GtkWidget *parent,SaveStruct *sst);
GtkTreeModel *factory_create_idcombox_model (GList *idlist);
void factory_append_iten_to_cbmodal(GtkListStore *model,gchar *str);
void factory_save_idlist_items(ObjectNode obj_node,GList *savelist);
void factory_read_idlist_items(ObjectNode obj_node);

void factory_save_idlist_to_xml(SaveStruct *sss,ObjectNode obj_node);


void factory_write_mfile_filelist(ObjectNode obj_node);
enum
{
    COLUMN_ITEM_SEQUENCE,
    COLUMN_ITEM_ADDR,
    COLUMN_ITEM_IDNAME,
    NUM_OF_COLS
};



/* Template Func*/
void factory_template_edit_callback(GtkAction *action,GList *objects);

#endif /* CLASS_H */
