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






typedef struct _STRUCTClass STRUCTClass;
typedef struct _STRUCTClassDialog STRUCTClassDialog;

/**
 * \brief Very special user interface for STRUCTClass parametrization
 *
 * There is a (too) tight coupling between the STRUCTClass and it's user interface.
 * And the dialog is too huge in code as well as on screen.
 */
struct _STRUCTClassDialog {
  GtkWidget *dialog;

//  GList *itemsData;   // 2014-3-19 lcy 这里是自定项,用存储从文件读到的条目.
//  GList *enumList;    // 2014-3-19 lcy 这里用存储枚举的链表.
   GtkWidget *mainTable; // 2014-3-19 lcy 这里添一个表格,用来布局显示.
//  GtkEntry *classname;
//  GtkEntry *stereotype;
//  GtkTextView *comment;
//
//  GtkToggleButton *abstract_class;
//  GtkToggleButton *attr_vis;
//  GtkToggleButton *attr_supp;
//  GtkToggleButton *op_vis;
//  GtkToggleButton *op_supp;
//  GtkToggleButton *comments_vis;
//  GtkToggleButton *op_wrap;
//  DiaFontSelector *normal_font;
//  DiaFontSelector *abstract_font;
//  DiaFontSelector *polymorphic_font;
//  DiaFontSelector *classname_font;
//  DiaFontSelector *abstract_classname_font;
//  DiaFontSelector *comment_font;
//  GtkSpinButton *normal_font_height;
//  GtkSpinButton *abstract_font_height;
//  GtkSpinButton *polymorphic_font_height;
//  GtkSpinButton *classname_font_height;
//  GtkSpinButton *abstract_classname_font_height;
//  GtkSpinButton *comment_font_height;
//  GtkSpinButton *wrap_after_char;
//  GtkSpinButton *comment_line_length;
//  GtkToggleButton *comment_tagging;
//  GtkSpinButton *line_width;
//  DiaColorSelector *text_color;
//  DiaColorSelector *line_color;
//  DiaColorSelector *fill_color;
//  GtkLabel *max_length_label;
//  GtkLabel *Comment_length_label;
//
//  GList *disconnected_connections;
//  GList *added_connections;
//  GList *deleted_connections;
//
//  GtkList *attributes_list;
//  GtkListItem *current_attr;
//  GtkEntry *attr_name;
//  GtkEntry *attr_type;
//  GtkEntry *attr_value;
//  GtkTextView *attr_comment;
//  GtkMenu *attr_visible;
//  GtkOptionMenu *attr_visible_button;
//  GtkToggleButton *attr_class_scope;
//
//  GtkList *operations_list;
//  GtkListItem *current_op;
//  GtkEntry *op_name;
//  GtkEntry *op_type;
//  GtkEntry *op_stereotype;
//  GtkTextView *op_comment;
//
//  GtkMenu *op_visible;
//  GtkOptionMenu *op_visible_button;
//  GtkToggleButton *op_class_scope;
//  GtkMenu *op_inheritance_type;
//  GtkOptionMenu *op_inheritance_type_button;
//  GtkToggleButton *op_query;
//
//  GtkList *parameters_list;
//  GtkListItem *current_param;
//  GtkEntry *param_name;
//  GtkEntry *param_type;
//  GtkEntry *param_value;
//  GtkTextView *param_comment;
//  GtkMenu *param_kind;
//  GtkOptionMenu *param_kind_button;
//  GtkWidget *param_new_button;
//  GtkWidget *param_delete_button;
//  GtkWidget *param_up_button;
//  GtkWidget *param_down_button;
//
//  GtkList *templates_list;
//  GtkListItem *current_templ;
//  GtkToggleButton *templ_template;
//  GtkEntry *templ_name;
//  GtkEntry *templ_type;
};


typedef struct  _FactoryClassDialog  FactoryClassDialog;

struct _FactoryClassDialog{
  GtkWidget *dialog;

//  GList *itemsData;   // 2014-3-19 lcy 这里是自定项,用存储从文件读到的条目.
//  GList *enumList;    // 2014-3-19 lcy 这里用存储枚举的链表.
  FactoryStructItemAll *EnumsAndStructs ;// 2014-3-21 lcy 这里包含一个文件里的所有结构体.
  GtkWidget *mainTable; // 2014-3-19 lcy 这里添一个表格,用来布局显示.

};




typedef enum{
    ENUM,
    ENTRY,
    SPINBOX
}CellType;

//typedef struct _WidgetAndValue WidgetAndValue;
//
//struct _WidgetAndValue{
//    gpointer *widget;
//
//    gchar* name;
//    CellType celltype;
//    gchar *value;
//};

typedef struct _SaveEnum SaveEnum;
struct _SaveEnum{
    GList *enumList;
    int index;
    gchar* evalue;
    gchar* width;
};



typedef struct _SaveEntry SaveEntry;
struct _SaveEntry
{
    gboolean isString;
    int row;
    int col;   /* default is 1 */
    int width;
    union{
        gchar *text;   /* s8 type, is string */
        GSList *arrlist;  /* 2014-3-31 lcy 这里用单链表来保存所有控件的值. (etc. "0xffff" ) */
    }data;
    GList *wlist;   /* GtkWidget List  */
};

typedef struct _SaveStruct SaveStruct;
typedef struct _SaveStruct{
    gpointer *widget;
    gchar* type;
    gchar* name;
    CellType celltype;
     union{
        SaveEntry sentry; // entry value
        gint number; // spinbox value
        SaveEnum senum;  // enum value;
    }value;

};

/**
 * \brief The most complex object Dia has
 *
 * What should I say? Don't try this at home :)
 */
struct _STRUCTClass {
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
  gboolean destroyed;
  GList *widgetmap; // 2014-3-22 lcy 这里用一个链表来保存界面上所有的值。
  FactoryStructItemAll *EnumsAndStructs ;// 2014-3-21 lcy 这里包含一个文件里的所有结构体.
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

static void
attributes_list_selection_changed_callback(GtkWidget *gtklist,
					   STRUCTClass *structclass);

extern void structclass_sanity_check(STRUCTClass *c, gchar *msg);


gboolean factory_find_array_flag(const gchar *data);

static  void factory_calculate_data(STRUCTClass *structclass);

GtkWidget *
factory_get_properties(STRUCTClass *structclass, gboolean is_default);

GtkWidget *factory_create_many_entry_box(SaveEntry *sey);
void factoy_create_subdialog(GtkButton *buttun,SaveStruct *sss);

void factory_create_and_fill_dialog(STRUCTClass *structclass, gboolean is_default);


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
