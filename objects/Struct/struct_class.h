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
    GtkWidget *mainTable; // 2014-3-19 lcy ������һ�����,����������ʾ.
};

typedef enum{
    N_COLOR,  /* ������ */
    H_COLOR /* ������ʾ�� */
}ViewColor;

typedef struct  _FactoryClassDialog  FactoryClassDialog;

struct _FactoryClassDialog
{
    GtkWidget *dialog;

//  GList *itemsData;   // 2014-3-19 lcy �������Զ���,�ô洢���ļ���������Ŀ.
//  GList *enumList;    // 2014-3-19 lcy �����ô洢ö�ٵ�����.
    FactoryStructItemAll *EnumsAndStructs ;// 2014-3-21 lcy �������һ���ļ�������нṹ��.
    GtkWidget *mainTable; // 2014-3-19 lcy ������һ�����,����������ʾ.

};


typedef struct _SaveStruct SaveStruct;

typedef void  (*CloseWidgetAndSave)(GtkWidget *widget,gint response_id,gpointer user_data);
typedef void  (*CreateNewDialog)(gpointer item,SaveStruct *sst);



typedef enum
{
    ECOMBO, /* ������  enum comobox */
    UCOMBO, /* union comobox */
    OCOMBO, /* object combox*/
    ENTRY, /* �ı� */
    SPINBOX,
    BBTN,
    UBTN,
    OBTN,/* �����ǰ�����ť */
    EBTN,/* ö��Ҳ������ */
    LBTN /* �ļ�������ID���� ������ʽ�İ��� */
} CellType;

typedef struct _ActionId ActionID;
struct _ActionId
{
    int index; // comobox index
    gchar* value;
    gchar *pre_name;
    gchar *title_name;
    gpointer conn_ptr; /* ��ָ�����ָ�룬������ID�����ܶ�Ӧ�������ֲ��� */
};


typedef struct _ArrayBaseProp ArrayBaseProp ; /* ����Ļ������� */
struct _ArrayBaseProp
{
    int row;
    int col;   /* default is 1 */
    int reallen;
};

typedef struct _IDListArg IDListArg; /* ����ID list �Ĳ��� */
struct _IDListArg
{
    gchar *value;  /* ����һ��ָ������ָ��*/
    GList *filist; /* ���һ�е�Ҫ�������� */
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

typedef void (*OrderDisplayWidget)(GList *srclist); /* ����Ҫ��˳����ʾ����̬*/

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
    ArrayBaseProp *arr_base;/* ��������� */
};


struct _SaveIdDialog
{
//    GtkWidget *parent_btn;
    GtkWidget *vbox;
    GList *idlists; /* SaveIdList ����*/
    gchar *title;
//    SaveKV *skv;
    GList *flist; /* combox list */
    GtkWidget *idlist_menu; /* ������ʾһ�����������е��б� */
    GtkListStore *id_store;
    GtkListStore *id_cbmodel;
    GtkWidget *id_treeview;
};

//SaveIdDialog *IdDialog;


/* �ļ��������Ľṹ�塡start*/

typedef struct _SaveMusicFile SaveMusicFile;
struct _SaveMusicFile
{
    int index;
    int file_addr;       /*����ƫ�Ƶĵ�ַ*/
    gchar* base_name;
    gchar* full_name;
    gchar* down_name;   /*С��ʶ�������*/

};

typedef struct _SaveMusicFileMan  SaveMusicFileMan;
struct _SaveMusicFileMan
{
//    int number;
    int offset;
//    int selected;
    gchar *lastDir;
    GtkWidget *opendlg;
    GList *filelist; /* �ұ߽�������������ǡ�SaveMusicFile��*/
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
        SEQUENCE, /*���*/
        INDEX, /*����*/
        PHY/*�����*/
}FMSaveType; /* ������ ��Ż���ƫ������*/;





typedef struct _MusicFileManagerOpts MusicFileManagerOpts; /* רΪ�����������һЩ������������*/


struct _SaveMusicDialog
{
    gchar *title;
    gchar *btnname;
    FMSaveType fmst;
    gchar **vnumber; /* �������SaveStruct vnumber*/
    GList *itemlist; /* ��߽�������������ǡ�SaveMusicItem */
    GList *cboxlist; /* �ұ������������������� gchar */
    GtkListStore *id_store;
    GtkTreeView  *id_treeview;
    GtkListStore *id_cbmodal;
    GtkWidget *window; /* ������Ĵ���*/
    SaveMusicFileMan *smfm; /* �ұ߽��� */
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


/* �ļ��������Ľṹ�塡end*/

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
    GHashTable* saveVal; //����ֵ�Ĺ�ϣ��
};

typedef struct _SaveEntry SaveEntry;
struct _SaveEntry
{
    gboolean isString;
//    int row;
//    int col;   /* default is 1 */
//    int reallen; /* ʵ�ʳ��ȱ� 9,10,4 ����8�ı�����. */
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
    GList *ebtnslist; /* ö�ٵ�����Դ���� */
    GList *ebtnwlist; /* ȫ����ö�ٵĿؼ�  ��� SaveEnumArr������  */
};



typedef struct _SaveStruct
{
    GtkWidget *widget1;
    GtkWidget *widget2;
    gchar* type;
    gchar* name;
//    gchar* pname;  /* ��һ������, NULL �������ϼ� */
    CellType celltype;
    gboolean isPointer; /* FALSE == pointer , TRUE = single*/
    gboolean isSensitive; /* �Ƿ�ɱ༭ */
    union
    {
        SaveEntry sentry; // entry value
        gchar *vnumber; // spinbox value or actionid max items
        SaveEnum senum;  // enum value;
        SaveUnion sunion; // �ڶ���ֵ,ָ����
        NextID nextid;  // �������ߵ� comobox;
        SaveUbtn ssubtn; /* �����尴�� */
        SaveEbtn ssebtn; /* ö������ */
        ListBtn slbtn;
    } value;
    FactoryStructItem *org;
    STRUCTClass *sclass; /* �������ϲ�Ķ��� */
    CloseWidgetAndSave  *close_func; /* ָ�򱣴溯�� */
    CreateNewDialog *newdlg_func; /* ָ����ʾ���ڵĺ���,Ҳ��������������Ϣ�ص��ĺ��� */
};

typedef struct _PublicSection PublicSection; /* ��ʾһЩ��������Ϣ */
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
    gboolean isInitial; /* ��ʼ�����ԶԻ���*/
    gboolean hasIdnumber;
    gboolean destroyed;
    gint depth; /* �����ǲ��Ҹ�����ʾ�����,��Ͳ����� */
    ViewColor vcolor;
    GHashTable *widgetmap; // 2014-3-22 lcy ������һ����ϣ����������������е�ֵ��
    GList *widgetSave; // 2014-3-22 lcy ����Ϊ˳����ʾ��������.
    FactoryStructItemAll *EnumsAndStructs ;// 2014-3-21 lcy �������һ���ļ�������нṹ��.
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
