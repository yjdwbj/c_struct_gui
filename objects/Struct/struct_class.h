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

#define ACTION_ID  "NextID"

#define ACT_SIZE  6




Layer *curLayer;
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
    EBTN /* 枚举也有数组 */
} CellType;

typedef struct _ActionId ActionID;
struct _ActionId
{
    int index; // comobox index
    gchar* value;
    gchar *pre_name;
    gchar *title_name;
};


typedef struct _ArrayBaseProp ArrayBaseProp ; /* 数组的基本属性 */
struct _ArrayBaseProp
{
    int row;
    int col;   /* default is 1 */
    int reallen;
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
//    GSList *uitemsave; /* 所有成员的存储结构指针 */
    GList *structlist;
//    GList *widgetlist;
//    gpointer *sdata; /* 当前的存储结构指针 */

//    gpointer SVal; // 基本类型. SaveStruct pointer;
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
    union
    {
        SaveEntry sentry; // entry value
        gchar *vnumber; // spinbox value or actionid max items
        SaveEnum senum;  // enum value;
        SaveUnion sunion; // 第二个值,指针类
        NextID nextid;  // 保存连线的 comobox;
        SaveUbtn ssubtn; /* 联合体按键 */
        SaveEbtn ssebtn; /* 枚举数组 */
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
    gboolean destroyed;
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
        gchar *objname /* end pointer */);



extern void structclass_sanity_check(STRUCTClass *c, gchar *msg);


gboolean factory_find_array_flag(const gchar *data);

static  void factory_calculate_data(STRUCTClass *structclass);

GtkWidget *
factory_get_properties(STRUCTClass *structclass, gboolean is_default);

GtkWidget *factory_create_many_entry_box(SaveStruct *sss);
GtkWidget *factory_create_many_checkbox(SaveStruct *sss);

void factory_create_basebutton_dialog(gpointer item,SaveStruct *sss);
void factory_create_unionbutton_dialog(gpointer item,SaveStruct *sst);
void factory_create_checkbuttons_by_list(gpointer item,SaveStruct *sst);
void factory_create_objectbutton_dialog(gpointer item,SaveStruct *sst);
void factory_create_enumbutton_dialog(gpointer item,SaveStruct *sst);
void factory_save_enumbutton_dialog(GtkWidget *widget,gint response_id,gpointer user_data);
void factory_save_objectbutton_dialog(GtkWidget *widget,gint  response_id,gpointer   user_data);
void factory_save_unionbutton_dialog(GtkWidget *widget,gint       response_id,gpointer   user_data);
void factory_save_basebutton_dialog(GtkWidget *widget,gint       response_id,gpointer   user_data);


void factory_inital_ebtn(SaveStruct *sss,const FactoryStructItem *fst);


static GList* factory_get_objects_from_layer(Layer *layer);
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

//void factoryReadDataFromFile(STRUCTClass *structclass);


#endif /* CLASS_H */
