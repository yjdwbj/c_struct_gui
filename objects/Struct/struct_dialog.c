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
 * File:    class_dialog.c
 *
 * Purpose: This file contains the code the draws and handles the class
 *          dialog. This is the dialog box that is displayed when the
 *          class Icon is double clicked.
 */
/*--------------------------------------------------------------------------**
 * Copyright(c) 2005 David Klotzbach
**                                                                          **
** Multi-Line Comments May 10, 2005 - Dave Klotzbach                        **
** dklotzbach@foxvalley.net                                                 **
**                                                                          **
**--------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#undef GTK_DISABLE_DEPRECATED /* GtkList, GtkOprionMenu, ... */
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "diagram.h"
#include "object.h"
#include "objchange.h"
#include "intl.h"
#include "struct_class.h"
#include "sheet.h"
#include "diagramdata.h"
#include "connpoint_line.h"



/* hide this functionality before rewrite;) */
void
structclass_dialog_free (STRUCTClassDialog *dialog)
{
//  g_list_free(dialog->deleted_connections);
    gtk_widget_destroy(dialog->dialog);
    dialog->dialog = NULL;
    gtk_widget_destroy(dialog->mainTable); // 2014-3-19 lcy 这里是回收内存.
//  g_list_free(dialog->EnumsAndStructs->enumList);
//  g_list_free(dialog->EnumsAndStructs->structList);
    /* destroy-signal destroy_properties_dialog already does 'g_free(dialog);' and more */
}

typedef struct _Disconnect
{
    ConnectionPoint *cp;
    DiaObject *other_object;
    Handle *other_handle;
} Disconnect;

typedef struct _STRUCTClassState STRUCTClassState;

struct _STRUCTClassState
{
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
//  char *stereotype;
//  char *comment;

//  int abstract;
//  int suppress_attributes;
//  int suppress_operations;
//  int visible_attributes;
//  int visible_operations;
//  int visible_comments;
//
//  int wrap_operations;
//  int wrap_after_char;
//  int comment_line_length;
//  int comment_tagging;

    real line_width;
    Color line_color;
    Color fill_color;
    Color text_color;

    /* Attributes: */
//  GList *attributes;

    /* Operators: */
//  GList *operations;

    /* Template: */
//  int template;
//  GList *formal_params;
    GHashTable *widgetmap; // 2014-3-22 lcy 这里用一个链表来保存界面上所有的值。
    FactoryStructItemAll *EnumsAndStructs ;// 2014-3-21 lcy 这里包含一个文件里的所有结构体.
};


typedef struct _STRUCTClassChange STRUCTClassChange;

struct _STRUCTClassChange
{
    ObjectChange obj_change;

    STRUCTClass *obj;

//  GList *added_cp;
//  GList *deleted_cp;
//  GList *disconnected;

    int applied;

    STRUCTClassState *saved_state;
};

static STRUCTClassState *structclass_get_state(STRUCTClass *structclass);

static STRUCTClassState *factory_get_state(STRUCTClass *structclass);
static ObjectChange *
factory_new_change(STRUCTClass *obj, STRUCTClassState *saved_state);

static ObjectChange *new_structclass_change(STRUCTClass *obj, STRUCTClassState *saved_state,
        GList *added, GList *deleted,
        GList *disconnected);
static  const gchar *get_comment(GtkTextView *);
static void set_comment(GtkTextView *, gchar *);
void factory_save_value_from_widget(SaveStruct *sss);

/**** Utility functions ******/
//static void
//structclass_store_disconnects(STRUCTClassDialog *prop_dialog,
//                              ConnectionPoint *cp)
//{
//    Disconnect *dis;
//    DiaObject *connected_obj;
//    GList *list;
//    int i;
//
//    list = cp->connected;
//    while (list != NULL)
//    {
//        connected_obj = (DiaObject *)list->data;
//
//        for (i=0; i<connected_obj->num_handles; i++)
//        {
//            if (connected_obj->handles[i]->connected_to == cp)
//            {
//                dis = g_new0(Disconnect, 1);
//                dis->cp = cp;
//                dis->other_object = connected_obj;
//                dis->other_handle = connected_obj->handles[i];
//
////	prop_dialog->disconnected_connections =
////	  g_list_prepend(prop_dialog->disconnected_connections, dis);
//            }
//        }
//        list = g_list_next(list);
//    }
//}

/** Add an option to an option menu item for a class.
 * @param menu The GtkMenu to add an item to.
 * @param label The I18N'd label to show in the menu.
 * @param structclass The class object that the dialog is being built for.
 * @param user_data Arbitrary data, here typically an integer indicating the
 * option internally.
 */
//static void
//add_option_menu_item(GtkMenu *menu, gchar *label, GtkSignalFunc update_func,
//		     STRUCTClass *structclass, gpointer user_data)
//{
//  GtkWidget *menuitem = gtk_menu_item_new_with_label (label);
//  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
//		      update_func, structclass);
//  gtk_object_set_user_data(GTK_OBJECT(menuitem), user_data);
//  gtk_menu_append (GTK_MENU (menu), menuitem);
//  gtk_widget_show (menuitem);
//}

/********************************************************
 ******************** CLASS *****************************
 ********************************************************/

//static void
//class_read_from_dialog(STRUCTClass *structclass, STRUCTClassDialog *prop_dialog)
//{
//  const gchar *s;
//
//  if (structclass->name != NULL)
//    g_free(structclass->name);
//
//  s = gtk_entry_get_text (prop_dialog->classname);
//  if (s && s[0])
//    structclass->name = g_strdup (s);
//  else
//    structclass->name = NULL;
//
//  if (structclass->stereotype != NULL)
//    g_free(structclass->stereotype);
//
//  s = gtk_entry_get_text(prop_dialog->stereotype);
//  if (s && s[0])
//    structclass->stereotype = g_strdup (s);
//  else
//    structclass->stereotype = NULL;
//
//  if (structclass->comment != NULL)
//    g_free (structclass->comment);
//
//  s = get_comment(prop_dialog->comment);
//  if (s && s[0])
//    structclass->comment = g_strdup (s);
//  else
//    structclass->comment = NULL;
//
//  structclass->abstract = prop_dialog->abstract_class->active;
//  structclass->visible_attributes = prop_dialog->attr_vis->active;
//  structclass->visible_operations = prop_dialog->op_vis->active;
//  structclass->wrap_operations = prop_dialog->op_wrap->active;
//  structclass->wrap_after_char = gtk_spin_button_get_value_as_int(prop_dialog->wrap_after_char);
//  structclass->comment_line_length = gtk_spin_button_get_value_as_int(prop_dialog->comment_line_length);
//  structclass->comment_tagging = prop_dialog->comment_tagging->active;
//  structclass->visible_comments = prop_dialog->comments_vis->active;
//  structclass->suppress_attributes = prop_dialog->attr_supp->active;
//  structclass->suppress_operations = prop_dialog->op_supp->active;
//  structclass->line_width = gtk_spin_button_get_value_as_float(prop_dialog->line_width);
//  dia_color_selector_get_color(GTK_WIDGET(prop_dialog->text_color), &structclass->text_color);
//  dia_color_selector_get_color(GTK_WIDGET(prop_dialog->line_color), &structclass->line_color);
//  dia_color_selector_get_color(GTK_WIDGET(prop_dialog->fill_color), &structclass->fill_color);
//
//  structclass->normal_font = dia_font_selector_get_font (prop_dialog->normal_font);
//  structclass->polymorphic_font = dia_font_selector_get_font (prop_dialog->polymorphic_font);
//  structclass->abstract_font = dia_font_selector_get_font (prop_dialog->abstract_font);
//  structclass->classname_font = dia_font_selector_get_font (prop_dialog->classname_font);
//  structclass->abstract_classname_font = dia_font_selector_get_font (prop_dialog->abstract_classname_font);
//  structclass->comment_font = dia_font_selector_get_font (prop_dialog->comment_font);
//
//  structclass->font_height = gtk_spin_button_get_value_as_float (prop_dialog->normal_font_height);
//  structclass->abstract_font_height = gtk_spin_button_get_value_as_float (prop_dialog->abstract_font_height);
//  structclass->polymorphic_font_height = gtk_spin_button_get_value_as_float (prop_dialog->polymorphic_font_height);
//  structclass->classname_font_height = gtk_spin_button_get_value_as_float (prop_dialog->classname_font_height);
//  structclass->abstract_classname_font_height = gtk_spin_button_get_value_as_float (prop_dialog->abstract_classname_font_height);
//  structclass->comment_font_height = gtk_spin_button_get_value_as_float (prop_dialog->comment_font_height);
//}
//
//static void
//class_fill_in_dialog(STRUCTClass *structclass)
//{
//  STRUCTClassDialog *prop_dialog;
//
//  prop_dialog = structclass->properties_dialog;
//
//  if (structclass->name)
//    gtk_entry_set_text(prop_dialog->classname, structclass->name);
//  if (structclass->stereotype != NULL)
//    gtk_entry_set_text(prop_dialog->stereotype, structclass->stereotype);
//  else
//    gtk_entry_set_text(prop_dialog->stereotype, "");
//
//  if (structclass->comment != NULL)
//    set_comment(prop_dialog->comment, structclass->comment);
//  else
//    set_comment(prop_dialog->comment, "");
//
//  gtk_toggle_button_set_active(prop_dialog->abstract_class, structclass->abstract);
//  gtk_toggle_button_set_active(prop_dialog->attr_vis, structclass->visible_attributes);
//  gtk_toggle_button_set_active(prop_dialog->op_vis, structclass->visible_operations);
//  gtk_toggle_button_set_active(prop_dialog->op_wrap, structclass->wrap_operations);
//  gtk_spin_button_set_value (prop_dialog->wrap_after_char, structclass->wrap_after_char);
//  gtk_spin_button_set_value (prop_dialog->comment_line_length, structclass->comment_line_length);
//  gtk_toggle_button_set_active(prop_dialog->comment_tagging, structclass->comment_tagging);
//  gtk_toggle_button_set_active(prop_dialog->comments_vis, structclass->visible_comments);
//  gtk_toggle_button_set_active(prop_dialog->attr_supp, structclass->suppress_attributes);
//  gtk_toggle_button_set_active(prop_dialog->op_supp, structclass->suppress_operations);
//  gtk_spin_button_set_value (prop_dialog->line_width, structclass->line_width);
//  dia_color_selector_set_color(GTK_WIDGET(prop_dialog->text_color), &structclass->text_color);
//  dia_color_selector_set_color(GTK_WIDGET(prop_dialog->line_color), &structclass->line_color);
//  dia_color_selector_set_color(GTK_WIDGET(prop_dialog->fill_color), &structclass->fill_color);
//  dia_font_selector_set_font (prop_dialog->normal_font, structclass->normal_font);
//  dia_font_selector_set_font (prop_dialog->polymorphic_font, structclass->polymorphic_font);
//  dia_font_selector_set_font (prop_dialog->abstract_font, structclass->abstract_font);
//  dia_font_selector_set_font (prop_dialog->classname_font, structclass->classname_font);
//  dia_font_selector_set_font (prop_dialog->abstract_classname_font, structclass->abstract_classname_font);
//  dia_font_selector_set_font (prop_dialog->comment_font, structclass->comment_font);
//  gtk_spin_button_set_value (prop_dialog->normal_font_height, structclass->font_height);
//  gtk_spin_button_set_value (prop_dialog->polymorphic_font_height, structclass->polymorphic_font_height);
//  gtk_spin_button_set_value (prop_dialog->abstract_font_height, structclass->abstract_font_height);
//  gtk_spin_button_set_value (prop_dialog->classname_font_height, structclass->classname_font_height);
//  gtk_spin_button_set_value (prop_dialog->abstract_classname_font_height, structclass->abstract_classname_font_height);
//  gtk_spin_button_set_value (prop_dialog->comment_font_height, structclass->comment_font_height);
//}

//static void
//create_font_props_row (GtkTable   *table,
//                       const char *kind,
//                       gint        row,
//                       DiaFont    *font,
//                       real        height,
//                       DiaFontSelector **fontsel,
//                       GtkSpinButton   **heightsel)
//{
//  GtkWidget *label;
//  GtkObject *adj;
//
//  label = gtk_label_new (kind);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach_defaults (table, label, 0, 1, row, row+1);
//  *fontsel = DIAFONTSELECTOR (dia_font_selector_new ());
//  dia_font_selector_set_font (DIAFONTSELECTOR (*fontsel), font);
//  gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET(*fontsel), 1, 2, row, row+1);
//
//  adj = gtk_adjustment_new (height, 0.1, 10.0, 0.1, 1.0, 0);
//  *heightsel = GTK_SPIN_BUTTON (gtk_spin_button_new (GTK_ADJUSTMENT(adj), 1.0, 2));
//  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (*heightsel), TRUE);
//  gtk_table_attach_defaults (table, GTK_WIDGET (*heightsel), 2, 3, row, row+1);
//}


gboolean factory_find_array_flag(const gchar *data)
{
    /* 2014-3-27 lcy 这里是查找[]标志 */
    gboolean  lflag = FALSE;
    gboolean  rflag = FALSE;
    int len  = strlen(data);
    int i =0;
    for(; i <len ; i++)
    {
        if(data[i] == '[')
        {
            lflag = TRUE;
        }
        else if(data[i] == ']')
        {
            rflag = TRUE;
        }
    }
    return rflag & lflag;
}






static void
factory_set_twoxtwo_table(GtkWidget* table ,GtkWidget* first,GtkWidget *second,int row)
{
    if(  row == 0 || !(row % 2 ))
    {
        gtk_table_attach_defaults(GTK_TABLE(table),first,0,1,row,row+1);
        gtk_table_attach_defaults(GTK_TABLE(table),second,1,2,row,row+1);
    }
    else
    {
        gtk_table_attach_defaults(GTK_TABLE(table),first,2,3,row-1,row); // 2014-3-20 lcy 第三列
        gtk_table_attach_defaults(GTK_TABLE(table),second,3,4,row-1,row);
    }
}

//static void
//factory_create_struct_dialog(STRUCTClass *class, FactoryStructItem *item,int row )   // 创建结构体的每一个项.
//{
//
//  SaveStruct *sss = g_new0(SaveStruct,1);
//  STRUCTClassDialog *dialog = class->properties_dialog;
//  GtkWidget *Name;
//  GtkWidget *columTwo;
//  GtkTooltips *tool_tips = gtk_tooltips_new();
//  CellType datatype;
//   /* 2014-3-26 lcy 通过名字去哈希表里找链表*/
//    GList *targettable = NULL;
//    if(targettable)
//    {
//
//              columTwo = gtk_combo_box_new_text();
//              gtk_combo_box_popdown(GTK_COMBO_BOX(columTwo));
//              //g_hash_table_foreach(targettable,factory_comobox_appen_text,columTwo);
//              GList *t = targettable;
//              for(; t != NULL ; t = t->next)
//              {
//                  FactoryStructEnum *kvmap = t->data;
//                  if(!g_ascii_strncasecmp(item->Value,kvmap->key,strlen(item->Value)))
//                  {
//                     sss->value.senum.index = g_list_index(targettable,kvmap);
//                     sss->value.senum.width = item->Max;
//                     sss->value.senum.evalue = kvmap->value;
//                  }
//                  /*把列表加进下拉框*/
//                  gtk_combo_box_append_text(GTK_COMBO_BOX(columTwo),kvmap->key);
//              }
//              gtk_combo_box_set_active(GTK_COMBO_BOX(columTwo), sss->value.senum.index );
//    }
//
//    if(datatype != ENUM ) // 不是枚举型的。
//      {
//
//            if( factory_find_array_flag(item->Name))
//              {
//                       /* 2014-3-25 lcy 这里是字符串，需用文本框显示了*/
//                       datatype = ENTRY;
//                       gchar **ooo = g_strsplit_set (item->Name,"[]",-1);
//                       gdouble maxlen = g_strtod(ooo[1],NULL); // 得到文本框的大小。
//                       g_strfreev(ooo);
//
//                       columTwo = gtk_entry_new();
//                       gtk_entry_set_max_length (GTK_ENTRY(columTwo),maxlen);
////                       gtk_entry_ (GTK_ENTRY(columTwo),GTK_INPUT_PURPOSE_ALPHA|
////                                                   GTK_INPUT_PURPOSE_DIGITS);
//                       gtk_entry_set_text(GTK_ENTRY(columTwo),item->Value);  // set default value;
////                       sss->value.text = g_locale_to_utf8(item->Value,-1,NULL,NULL,NULL);
//               }
//               else if (datatype == SPINBOX ){
//                 GtkObject *adj;
//                 adj = gtk_adjustment_new( g_strtod(item->Value,NULL) , g_strtod(item->Min,NULL),
//                                           g_strtod(item->Max,NULL), 1.0, 5.0, 0);
//                 columTwo = GTK_SPIN_BUTTON(gtk_spin_button_new( GTK_ADJUSTMENT( adj), 0.1, 0));
//                 gtk_spin_button_set_numeric( GTK_SPIN_BUTTON( columTwo), TRUE);
//                 gtk_spin_button_set_snap_to_ticks( GTK_SPIN_BUTTON(columTwo), TRUE);
//                 sss->value.number = g_strtod(item->Value,NULL);
//               }
//      }
//
//  gtk_tooltips_set_tip(tool_tips,columTwo,_(item->Comment),NULL);
//
//  Name = gtk_label_new(item->Cname);
//  gtk_tooltips_set_tip(tool_tips,Name,_(item->Comment),NULL);
//
//  factory_set_twoxtwo_table(dialog->mainTable,Name,columTwo,row);
//
//
//  sss->widget = (gpointer)columTwo;
//  sss->name = item->Name;
//  sss->type = item->FType;
//  sss->celltype = datatype;
//
//  g_hash_table_insert(class->widgetmap,g_strjoin("##",item->FType,item->Name,NULL),sss);
//  //g_free(key);
//#ifdef DEBUG
//  int s = g_hash_table_size(class->widgetmap);
//#endif // DEBUG
//
////  class->widgetmap =  g_list_append(class->widgetmap,sss);
//  gtk_container_add(GTK_OBJECT(dialog->dialog),dialog->mainTable);
//}




//static void
//class_create_page(GtkNotebook *notebook,  STRUCTClass *structclass)
//{
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *page_label;
//  GtkWidget *label;
//  GtkWidget *hbox;
//  GtkWidget *hbox2;
//  GtkWidget *vbox;
//  GtkWidget *entry;
//  GtkWidget *scrolledwindow;
//  GtkWidget *checkbox;
//  GtkWidget *table;
//  GtkObject *adj;
//
//  prop_dialog = structclass->properties_dialog;
//
//  /* Class page: */
//  page_label = gtk_label_new_with_mnemonic (_("_Class"));
//
//  vbox = gtk_vbox_new(FALSE, 5);
//  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10); // 添加一个label , 在lable 上布局控件, 然后加到table 上去
//
//  table = gtk_table_new (3, 2, FALSE);
//  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
//
//  label = gtk_label_new(_("Class name:"));
//  entry = gtk_entry_new();
//  prop_dialog->classname = GTK_ENTRY(entry);
//  gtk_widget_grab_focus(entry);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  label = gtk_label_new(_("Stereotype:"));
//  entry = gtk_entry_new();
//  prop_dialog->stereotype = GTK_ENTRY(entry);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  label = gtk_label_new(_("Comment:"));
//  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
//  gtk_table_attach (GTK_TABLE (table), scrolledwindow, 1, 2, 2, 3,
//		    (GtkAttachOptions) (GTK_FILL),
//		    (GtkAttachOptions) (GTK_FILL), 0, 0);
//  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
//				       GTK_SHADOW_IN);
//  entry = gtk_text_view_new ();
//  prop_dialog->comment = GTK_TEXT_VIEW(entry);
//  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
//
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
//  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
//
//  hbox = gtk_hbox_new(FALSE, 5);
//  checkbox = gtk_check_button_new_with_label(_("Abstract"));
//  prop_dialog->abstract_class = GTK_TOGGLE_BUTTON( checkbox );
//  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
//  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
//
//  hbox = gtk_hbox_new(FALSE, 5);
//  checkbox = gtk_check_button_new_with_label(_("Attributes visible"));
//  prop_dialog->attr_vis = GTK_TOGGLE_BUTTON( checkbox );
//  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
//  checkbox = gtk_check_button_new_with_label(_("Suppress Attributes"));
//  prop_dialog->attr_supp = GTK_TOGGLE_BUTTON( checkbox );
//  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
//  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
//
//  hbox = gtk_hbox_new(FALSE, 5);
//  checkbox = gtk_check_button_new_with_label(_("Operations visible"));
//  prop_dialog->op_vis = GTK_TOGGLE_BUTTON( checkbox );
//  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
//  checkbox = gtk_check_button_new_with_label(_("Suppress operations"));
//  prop_dialog->op_supp = GTK_TOGGLE_BUTTON( checkbox );
//  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
//  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
//
//  hbox  = gtk_hbox_new(TRUE, 5);
//  hbox2 = gtk_hbox_new(FALSE, 5);
//  checkbox = gtk_check_button_new_with_label(_("Wrap Operations"));
//  prop_dialog->op_wrap = GTK_TOGGLE_BUTTON( checkbox );
//  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
//  adj = gtk_adjustment_new( structclass->wrap_after_char, 0.0, 200.0, 1.0, 5.0, 0);
//  prop_dialog->wrap_after_char = GTK_SPIN_BUTTON(gtk_spin_button_new( GTK_ADJUSTMENT( adj), 0.1, 0));
//  gtk_spin_button_set_numeric( GTK_SPIN_BUTTON( prop_dialog->wrap_after_char), TRUE);
//  gtk_spin_button_set_snap_to_ticks( GTK_SPIN_BUTTON( prop_dialog->wrap_after_char), TRUE);
//  prop_dialog->max_length_label = GTK_LABEL( gtk_label_new( _("Wrap after this length: ")));
//  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET( prop_dialog->max_length_label), FALSE, TRUE, 0);
//  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET( prop_dialog->wrap_after_char), TRUE, TRUE, 0);
//  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET( hbox2), TRUE, TRUE, 0);
//  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
//
//  hbox = gtk_hbox_new(TRUE, 5);
//  hbox2 = gtk_hbox_new(FALSE, 5);
//  checkbox = gtk_check_button_new_with_label(_("Comments visible"));
//  prop_dialog->comments_vis = GTK_TOGGLE_BUTTON( checkbox );
//  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
//  adj = gtk_adjustment_new( structclass->comment_line_length, 17.0, 200.0, 1.0, 5.0, 0);
//  prop_dialog->comment_line_length = GTK_SPIN_BUTTON(gtk_spin_button_new( GTK_ADJUSTMENT( adj), 0.1, 0));
//  gtk_spin_button_set_numeric( GTK_SPIN_BUTTON( prop_dialog->comment_line_length), TRUE);
//  gtk_spin_button_set_snap_to_ticks( GTK_SPIN_BUTTON( prop_dialog->comment_line_length), TRUE);
//  prop_dialog->Comment_length_label = GTK_LABEL( gtk_label_new( _("Wrap comment after this length: ")));
//  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET( prop_dialog->Comment_length_label), FALSE, TRUE, 0);
//  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET( prop_dialog->comment_line_length), TRUE, TRUE, 0);
//  gtk_box_pack_start (GTK_BOX (hbox),  GTK_WIDGET( hbox2), TRUE, TRUE, 0);
//  gtk_box_pack_start (GTK_BOX (vbox),  hbox, FALSE, TRUE, 0);
//
//  hbox = gtk_hbox_new(FALSE, 5);
//  checkbox = gtk_check_button_new_with_label(_("Show documentation tag111111"));
//  prop_dialog->comment_tagging = GTK_TOGGLE_BUTTON( checkbox );
//  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
//  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
//
//  gtk_widget_show_all (vbox);
//  gtk_widget_show (page_label);
//  gtk_notebook_append_page(notebook, vbox, page_label);
//
//}
//
//
//static void
//style_create_page(GtkNotebook *notebook,  STRUCTClass *structclass)
//{
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *page_label;
//  GtkWidget *label;
//  GtkWidget *vbox;
//  GtkWidget *line_width;
//  GtkWidget *text_color;
//  GtkWidget *fill_color;
//  GtkWidget *line_color;
//  GtkWidget *table;
//  GtkObject *adj;
//
//  prop_dialog = structclass->properties_dialog;
//
//  /** Fonts and Colors selection **/
//  page_label = gtk_label_new_with_mnemonic (_("_Style"));
//
//  vbox = gtk_vbox_new(FALSE, 5);
//  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
//
//  table = gtk_table_new (5, 6, TRUE);
//  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
//  gtk_table_set_homogeneous (GTK_TABLE (table), FALSE);
//
//  /* head line */
//  label = gtk_label_new (_("Kind"));
//                                                    /* L, R, T, B */
//  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
//  label = gtk_label_new (_("Font"));
//  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
//  label = gtk_label_new (_("Size"));
//  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);
//
//  /* property rows */
//  create_font_props_row (GTK_TABLE (table), _("Normal"), 1,
//                         structclass->normal_font,
//                         structclass->font_height,
//                         &(prop_dialog->normal_font),
//                         &(prop_dialog->normal_font_height));
//  create_font_props_row (GTK_TABLE (table), _("Polymorphic"), 2,
//                         structclass->polymorphic_font,
//                         structclass->polymorphic_font_height,
//                         &(prop_dialog->polymorphic_font),
//                         &(prop_dialog->polymorphic_font_height));
//  create_font_props_row (GTK_TABLE (table), _("Abstract"), 3,
//                         structclass->abstract_font,
//                         structclass->abstract_font_height,
//                         &(prop_dialog->abstract_font),
//                         &(prop_dialog->abstract_font_height));
//  create_font_props_row (GTK_TABLE (table), _("Class Name"), 4,
//                         structclass->classname_font,
//                         structclass->classname_font_height,
//                         &(prop_dialog->classname_font),
//                         &(prop_dialog->classname_font_height));
//  create_font_props_row (GTK_TABLE (table), _("Abstract Class"), 5,
//                         structclass->abstract_classname_font,
//                         structclass->abstract_classname_font_height,
//                         &(prop_dialog->abstract_classname_font),
//                         &(prop_dialog->abstract_classname_font_height));
//  create_font_props_row (GTK_TABLE (table), _("Comment"), 6,
//                         structclass->comment_font,
//                         structclass->comment_font_height,
//                         &(prop_dialog->comment_font),
//                         &(prop_dialog->comment_font_height));
//
//
//
//  table = gtk_table_new (2, 4, TRUE);
//  gtk_box_pack_start (GTK_BOX (vbox),
//		      table, FALSE, TRUE, 0);
//  /* should probably be refactored too. */
//  label = gtk_label_new(_("Line Width"));
//  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 2);
//  adj = gtk_adjustment_new(structclass->line_width, 0.0, G_MAXFLOAT, 0.1, 1.0, 0);
//  line_width = gtk_spin_button_new (GTK_ADJUSTMENT(adj), 1.0, 2);
//  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (line_width), TRUE);
//  prop_dialog->line_width = GTK_SPIN_BUTTON(line_width);
//  gtk_table_attach (GTK_TABLE (table), line_width, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 3, 2);
//
//  label = gtk_label_new(_("Text Color"));
//  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 2);
//  text_color = dia_color_selector_new();
//  dia_color_selector_set_color(text_color, &structclass->text_color);
//  prop_dialog->text_color = (DiaColorSelector *)text_color;
//  gtk_table_attach (GTK_TABLE (table), text_color, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 3, 2);
//
//  label = gtk_label_new(_("Foreground Color"));
//  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 2);
//  line_color = dia_color_selector_new();
//  dia_color_selector_set_color(line_color, &structclass->line_color);
//  prop_dialog->line_color = (DiaColorSelector *)line_color;
//  gtk_table_attach (GTK_TABLE (table), line_color, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 3, 2);
//
//  label = gtk_label_new(_("Background Color"));
//  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 2);
//  fill_color = dia_color_selector_new();
//  dia_color_selector_set_color(fill_color, &structclass->fill_color);
//  prop_dialog->fill_color = (DiaColorSelector *)fill_color;
//  gtk_table_attach (GTK_TABLE (table), fill_color, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, 0, 3, 2);
//
//  gtk_widget_show_all (vbox);
//  gtk_widget_show (page_label);
//  gtk_notebook_append_page(notebook, vbox, page_label);
//
//}
//
//
///************************************************************
// ******************** ATTRIBUTES ****************************
// ************************************************************/
//
//static void
//attributes_set_sensitive(STRUCTClassDialog *prop_dialog, gint val)
//{
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_name), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_type), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_value), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_comment), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_visible_button), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_visible), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_class_scope), val);
//}
//
//static void
//attributes_set_values(STRUCTClassDialog *prop_dialog, STRUCTAttribute *attr)
//{
//  gtk_entry_set_text(prop_dialog->attr_name, attr->name);
//  gtk_entry_set_text(prop_dialog->attr_type, attr->type);
//  if (attr->value != NULL)
//    gtk_entry_set_text(prop_dialog->attr_value, attr->value);
//  else
//    gtk_entry_set_text(prop_dialog->attr_value, "");
//
//  if (attr->comment != NULL)
//    set_comment(prop_dialog->attr_comment, attr->comment);
//  else
//    set_comment(prop_dialog->attr_comment, "");
//
//
//  gtk_option_menu_set_history(prop_dialog->attr_visible_button,
//			      (gint)attr->visibility);
//  gtk_toggle_button_set_active(prop_dialog->attr_class_scope, attr->class_scope);
//}
//
//static void
//attributes_clear_values(STRUCTClassDialog *prop_dialog)
//{
//  gtk_entry_set_text(prop_dialog->attr_name, "");
//  gtk_entry_set_text(prop_dialog->attr_type, "");
//  gtk_entry_set_text(prop_dialog->attr_value, "");
//  set_comment(prop_dialog->attr_comment, "");
//  gtk_toggle_button_set_active(prop_dialog->attr_class_scope, FALSE);
//}
//
//static void
//attributes_get_values (STRUCTClassDialog *prop_dialog, STRUCTAttribute *attr)
//{
//  g_free (attr->name);
//  g_free (attr->type);
//  if (attr->value != NULL)
//    g_free (attr->value);
//
//  attr->name = g_strdup (gtk_entry_get_text (prop_dialog->attr_name));
//  attr->type = g_strdup (gtk_entry_get_text (prop_dialog->attr_type));
//
//  attr->value = g_strdup (gtk_entry_get_text(prop_dialog->attr_value));
//  attr->comment = g_strdup (get_comment(prop_dialog->attr_comment));
//
//  attr->visibility = (STRUCTVisibility)
//		GPOINTER_TO_INT (gtk_object_get_user_data (
//					 GTK_OBJECT (gtk_menu_get_active (prop_dialog->attr_visible))));
//
//  attr->class_scope = prop_dialog->attr_class_scope->active;
//}
//
//static void
//attributes_get_current_values(STRUCTClassDialog *prop_dialog)
//{
//  STRUCTAttribute *current_attr;
//  GtkLabel *label;
//  char *new_str;
//
//  if (prop_dialog != NULL && prop_dialog->current_attr != NULL) {
//    current_attr = (STRUCTAttribute *)
//      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_attr));
//    if (current_attr != NULL) {
//      attributes_get_values(prop_dialog, current_attr);
//      label = GTK_LABEL(GTK_BIN(prop_dialog->current_attr)->child);
//      new_str = struct_get_attribute_string(current_attr);
//      gtk_label_set_text (label, new_str);
//      g_free (new_str);
//    }
//  }
//}
//
//static void
//attribute_list_item_destroy_callback(GtkWidget *list_item,
//				     gpointer data)
//{
//  STRUCTAttribute *attr;
//
//  attr = (STRUCTAttribute *) gtk_object_get_user_data(GTK_OBJECT(list_item));
//
//  if (attr != NULL) {
//    struct_attribute_destroy(attr);
//    /*printf("Destroying list_item's user_data!\n");*/
//  }
//}
//
//static void
//attributes_list_selection_changed_callback(GtkWidget *gtklist,
//					   STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkObject *list_item;
//  STRUCTAttribute *attr;
//
//  /* Due to GtkList oddities, this may get called during destroy.
//   * But it'll reference things that are already dead and crash.
//   * Thus, we stop it before it gets that bad.  See bug #156706 for
//   * one example.
//   */
//  if (structclass->destroyed)
//    return;
//
//  prop_dialog = structclass->properties_dialog;
//
//  if (!prop_dialog)
//    return;
//
//  attributes_get_current_values(prop_dialog);
//
//  list = GTK_LIST(gtklist)->selection;
//  if (!list && prop_dialog) { /* No selected */
//    attributes_set_sensitive(prop_dialog, FALSE);
//    attributes_clear_values(prop_dialog);
//    prop_dialog->current_attr = NULL;
//    return;
//  }
//
//  list_item = GTK_OBJECT(list->data);
//  attr = (STRUCTAttribute *)gtk_object_get_user_data(list_item);
//  attributes_set_values(prop_dialog, attr);
//  attributes_set_sensitive(prop_dialog, TRUE);
//
//  prop_dialog->current_attr = GTK_LIST_ITEM(list_item);
//  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->attr_name));
//}
//
//static void
//attributes_list_new_callback(GtkWidget *button,
//			     STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *list_item;
//  STRUCTAttribute *attr;
//  char *utfstr;
//  prop_dialog = structclass->properties_dialog;
//
//  attributes_get_current_values(prop_dialog);
//
//  attr = struct_attribute_new();
//  /* need to make the new ConnectionPoint valid and remember them */
//  struct_attribute_ensure_connection_points (attr, &structclass->element.object);
//  prop_dialog->added_connections =
//    g_list_prepend(prop_dialog->added_connections, attr->left_connection);
//  prop_dialog->added_connections =
//    g_list_prepend(prop_dialog->added_connections, attr->right_connection);
//
//  utfstr = struct_get_attribute_string (attr);
//  list_item = gtk_list_item_new_with_label (utfstr);
//  gtk_widget_show (list_item);
//  g_free (utfstr);
//
//  gtk_object_set_user_data(GTK_OBJECT(list_item), attr);
//  gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
//		      GTK_SIGNAL_FUNC (attribute_list_item_destroy_callback),
//		      NULL);
//
//  list = g_list_append(NULL, list_item);
//  gtk_list_append_items(prop_dialog->attributes_list, list);
//
//  if (prop_dialog->attributes_list->children != NULL)
//    gtk_list_unselect_child(prop_dialog->attributes_list,
//			    GTK_WIDGET(prop_dialog->attributes_list->children->data));
//  gtk_list_select_child(prop_dialog->attributes_list, list_item);
//}
//
//static void
//attributes_list_delete_callback(GtkWidget *button,
//				STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkList *gtklist;
//  STRUCTAttribute *attr;
//
//  prop_dialog = structclass->properties_dialog;
//  gtklist = GTK_LIST(prop_dialog->attributes_list);
//
//  if (gtklist->selection != NULL) {
//    attr = (STRUCTAttribute *)
//      gtk_object_get_user_data(GTK_OBJECT(gtklist->selection->data));
//
//    if (attr->left_connection != NULL) {
//      prop_dialog->deleted_connections =
//	g_list_prepend(prop_dialog->deleted_connections,
//		       attr->left_connection);
//      prop_dialog->deleted_connections =
//	g_list_prepend(prop_dialog->deleted_connections,
//		       attr->right_connection);
//    }
//
//    list = g_list_prepend(NULL, gtklist->selection->data);
//    gtk_list_remove_items(gtklist, list);
//    g_list_free(list);
//    attributes_clear_values(prop_dialog);
//    attributes_set_sensitive(prop_dialog, FALSE);
//  }
//}
//
//static void
//attributes_list_move_up_callback(GtkWidget *button,
//				 STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkList *gtklist;
//  GtkWidget *list_item;
//  int i;
//
//  prop_dialog = structclass->properties_dialog;
//  gtklist = GTK_LIST(prop_dialog->attributes_list);
//
//  if (gtklist->selection != NULL) {
//    list_item = GTK_WIDGET(gtklist->selection->data);
//
//    i = gtk_list_child_position(gtklist, list_item);
//    if (i>0)
//      i--;
//
//    gtk_widget_ref(list_item);
//    list = g_list_prepend(NULL, list_item);
//    gtk_list_remove_items(gtklist, list);
//    gtk_list_insert_items(gtklist, list, i);
//    gtk_widget_unref(list_item);
//
//    gtk_list_select_child(gtklist, list_item);
//  }
//}

//static void
//attributes_list_move_down_callback(GtkWidget *button,
//				   STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkList *gtklist;
//  GtkWidget *list_item;
//  int i;
//
//  prop_dialog = structclass->properties_dialog;
//  gtklist = GTK_LIST(prop_dialog->attributes_list);
//
//  if (gtklist->selection != NULL) {
//    list_item = GTK_WIDGET(gtklist->selection->data);
//
//    i = gtk_list_child_position(gtklist, list_item);
//    if (i<(g_list_length(gtklist->children)-1))
//      i++;
//
//
//    gtk_widget_ref(list_item);
//    list = g_list_prepend(NULL, list_item);
//    gtk_list_remove_items(gtklist, list);
//    gtk_list_insert_items(gtklist, list, i);
//    gtk_widget_unref(list_item);
//
//    gtk_list_select_child(gtklist, list_item);
//  }
//}
//
//static void
//attributes_read_from_dialog(STRUCTClass *structclass,
//			    STRUCTClassDialog *prop_dialog,
//			    int connection_index)
//{
//  GList *list;
//  STRUCTAttribute *attr;
//  GtkWidget *list_item;
//  GList *clear_list;
//  DiaObject *obj;
//
//  obj = &structclass->element.object;
//
//  /* if the currently select attribute is changed, update the state in the
//   * dialog info from widgets */
//  attributes_get_current_values(prop_dialog);
//  /* Free current attributes: */
//  list = structclass->attributes;
//  while (list != NULL) {
//    attr = (STRUCTAttribute *)list->data;
//    struct_attribute_destroy(attr);
//    list = g_list_next(list);
//  }
//  g_list_free (structclass->attributes);
//  structclass->attributes = NULL;
//
//  /* Insert new attributes and remove them from gtklist: */
//  list = GTK_LIST (prop_dialog->attributes_list)->children;
//  clear_list = NULL;
//  while (list != NULL) {
//    list_item = GTK_WIDGET(list->data);
//
//    clear_list = g_list_prepend (clear_list, list_item);
//    attr = (STRUCTAttribute *)
//      gtk_object_get_user_data(GTK_OBJECT(list_item));
//    gtk_object_set_user_data(GTK_OBJECT(list_item), NULL);
//    structclass->attributes = g_list_append(structclass->attributes, attr);
//
//    if (attr->left_connection == NULL) {
//      struct_attribute_ensure_connection_points (attr, obj);
//
//      prop_dialog->added_connections =
//	g_list_prepend(prop_dialog->added_connections,
//		       attr->left_connection);
//      prop_dialog->added_connections =
//	g_list_prepend(prop_dialog->added_connections,
//		       attr->right_connection);
//    }
//
//    if ( (prop_dialog->attr_vis->active) &&
//	 (!prop_dialog->attr_supp->active) ) {
//      obj->connections[connection_index] = attr->left_connection;
//      connection_index++;
//      obj->connections[connection_index] = attr->right_connection;
//      connection_index++;
//    } else {
//      structclass_store_disconnects(prop_dialog, attr->left_connection);
//      object_remove_connections_to(attr->left_connection);
//      structclass_store_disconnects(prop_dialog, attr->right_connection);
//      object_remove_connections_to(attr->right_connection);
//    }
//
//    list = g_list_next(list);
//  }
//  clear_list = g_list_reverse (clear_list);
//  gtk_list_remove_items (GTK_LIST (prop_dialog->attributes_list), clear_list);
//  g_list_free (clear_list);
//
//#if 0 /* STRUCTClass is *known* to be in an incositent state here, check later or crash ... */
//  structclass_sanity_check(structclass, "Read from dialog");
//#endif
//}
//
//
//static void
//attributes_fill_in_dialog(STRUCTClass *structclass)
//{
//  STRUCTClassDialog *prop_dialog;
//  STRUCTAttribute *attr_copy;
//  GtkWidget *list_item;
//  GList *list;
//  int i;
//
//#ifdef DEBUG
//  structclass_sanity_check(structclass, "Filling in dialog");
//#endif
//
//  prop_dialog = structclass->properties_dialog;
//
//  /* copy in new attributes: */
//  if (prop_dialog->attributes_list->children == NULL) {
//    i = 0;
//    list = structclass->attributes;
//    while (list != NULL) {
//      STRUCTAttribute *attr = (STRUCTAttribute *)list->data;
//      gchar *attrstr = struct_get_attribute_string(attr);
//
//      list_item = gtk_list_item_new_with_label (attrstr);
//      attr_copy = struct_attribute_copy(attr);
//      /* looks wrong but required for complicated ConnectionPoint memory management */
//      attr_copy->left_connection = attr->left_connection;
//      attr_copy->right_connection = attr->right_connection;
//      gtk_object_set_user_data(GTK_OBJECT(list_item), (gpointer) attr_copy);
//      gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
//			  GTK_SIGNAL_FUNC (attribute_list_item_destroy_callback),
//			  NULL);
//      gtk_container_add (GTK_CONTAINER (prop_dialog->attributes_list), list_item);
//      gtk_widget_show (list_item);
//
//      list = g_list_next(list); i++;
//      g_free (attrstr);
//    }
//    /* set attributes non-sensitive */
//    prop_dialog->current_attr = NULL;
//    attributes_set_sensitive(prop_dialog, FALSE);
//    attributes_clear_values(prop_dialog);
//  }
//}
//
//static void
//attributes_update(GtkWidget *widget, STRUCTClass *structclass)
//{
//  attributes_get_current_values(structclass->properties_dialog);
//}
//
//static int
//attributes_update_event(GtkWidget *widget, GdkEventFocus *ev, STRUCTClass *structclass)
//{
//  attributes_get_current_values(structclass->properties_dialog);
//  return 0;
//}
//
//static void
//attributes_create_page(GtkNotebook *notebook,  STRUCTClass *structclass)
//{
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *page_label;
//  GtkWidget *label;
//  GtkWidget *hbox;
//  GtkWidget *vbox;
//  GtkWidget *vbox2;
//  GtkWidget *hbox2;
//  GtkWidget *table;
//  GtkWidget *entry;
//  GtkWidget *checkbox;
//  GtkWidget *scrolled_win;
//  GtkWidget *button;
//  GtkWidget *list;
//  GtkWidget *frame;
//  GtkWidget *omenu;
//  GtkWidget *menu;
//  GtkWidget *submenu;
//  GtkWidget *scrolledwindow;
//  GSList *group;
//
//  prop_dialog = structclass->properties_dialog;
//
//  /* Attributes page: */
//  page_label = gtk_label_new_with_mnemonic (_("_Attributes"));
//
//  vbox = gtk_vbox_new(FALSE, 5);
//  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
//
//  hbox = gtk_hbox_new(FALSE, 5);
//
//  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
//  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
//				  GTK_POLICY_AUTOMATIC,
//				  GTK_POLICY_AUTOMATIC);
//  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
//  gtk_widget_show (scrolled_win);
//
//  list = gtk_list_new ();
//  prop_dialog->attributes_list = GTK_LIST(list);
//  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
//  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
//  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
//				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
//  gtk_widget_show (list);
//
//  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
//		      GTK_SIGNAL_FUNC(attributes_list_selection_changed_callback),
//		      structclass);
//
//  vbox2 = gtk_vbox_new(FALSE, 5);
//
//  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(attributes_list_new_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(attributes_list_delete_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//  button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(attributes_list_move_up_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//  button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(attributes_list_move_down_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//
//  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);
//
//  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
//
//  frame = gtk_frame_new(_("Attribute data"));
//  vbox2 = gtk_vbox_new(FALSE, 5);
//  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 10);
//  gtk_container_add (GTK_CONTAINER (frame), vbox2);
//  gtk_widget_show(frame);
//  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
//
//  table = gtk_table_new (5, 2, FALSE);
//  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
//
//  label = gtk_label_new(_("Name:"));
//  entry = gtk_entry_new();
//  prop_dialog->attr_name = GTK_ENTRY(entry);
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (attributes_update_event), structclass);
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (attributes_update), structclass);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  label = gtk_label_new(_("Type:"));
//  entry = gtk_entry_new();
//  prop_dialog->attr_type = GTK_ENTRY(entry);
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (attributes_update_event), structclass);
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (attributes_update), structclass);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  label = gtk_label_new(_("Value:"));
//  entry = gtk_entry_new();
//  prop_dialog->attr_value = GTK_ENTRY(entry);
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (attributes_update_event), structclass);
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (attributes_update), structclass);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  label = gtk_label_new(_("Comment:"));
//  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
//  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
//				       GTK_SHADOW_IN);
//  entry = gtk_text_view_new ();
//  prop_dialog->attr_comment = GTK_TEXT_VIEW(entry);
//  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
//  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
//  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (entry),TRUE);
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (attributes_update_event), structclass);
//#if 0 /* while the GtkEntry has a "activate" signal, GtkTextView does not.
//       * Maybe we should connect to "set-focus-child" instead?
//       */
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (attributes_update), structclass);
//#endif
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,3,4, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), scrolledwindow, 1,2,3,4, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//
//  label = gtk_label_new(_("Visibility:"));
//
//  omenu = gtk_option_menu_new ();
//  menu = gtk_menu_new ();
//  prop_dialog->attr_visible = GTK_MENU(menu);
//  prop_dialog->attr_visible_button = GTK_OPTION_MENU(omenu);
//  submenu = NULL;
//  group = NULL;
//
//  add_option_menu_item(GTK_MENU(menu), _("Public"),
//		       GTK_SIGNAL_FUNC (attributes_update),
//		       structclass, GINT_TO_POINTER(STRUCT_PUBLIC));
//  add_option_menu_item(GTK_MENU(menu), _("Private"),
//		       GTK_SIGNAL_FUNC (attributes_update),
//		       structclass, GINT_TO_POINTER(STRUCT_PRIVATE) );
//  add_option_menu_item(GTK_MENU(menu), _("Protected"),
//		       GTK_SIGNAL_FUNC (attributes_update),
//		       structclass, GINT_TO_POINTER(STRUCT_PROTECTED) );
//  add_option_menu_item(GTK_MENU(menu), _("Implementation"),
//		       GTK_SIGNAL_FUNC (attributes_update),
//		       structclass, GINT_TO_POINTER(STRUCT_IMPLEMENTATION) );
//
//  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
//
//  {
//    GtkWidget * align;
//    align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
//    gtk_container_add(GTK_CONTAINER(align), omenu);
//    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//    gtk_table_attach (GTK_TABLE (table), label, 0,1,4,5, GTK_FILL,0, 0,3);
//    gtk_table_attach (GTK_TABLE (table), align, 1,2,4,5, GTK_FILL,0, 0,3);
//  }
//
//  hbox2 = gtk_hbox_new(FALSE, 5);
//  checkbox = gtk_check_button_new_with_label(_("Class scope"));
//  prop_dialog->attr_class_scope = GTK_TOGGLE_BUTTON(checkbox);
//  gtk_box_pack_start (GTK_BOX (hbox2), checkbox, TRUE, TRUE, 0);
//  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);
//
//  gtk_widget_show(vbox2);
//
//  gtk_widget_show_all (vbox);
//  gtk_widget_show (page_label);
//  gtk_notebook_append_page(notebook, vbox, page_label);
//
//}

/*************************************************************
 ******************** OPERATIONS *****************************
 *************************************************************/

/* Forward declaration: */


//static void
//parameters_set_sensitive(STRUCTClassDialog *prop_dialog, gint val)
//{
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_name), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_type), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_value), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_comment), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_kind), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_kind_button), val);
//}
//
//static void
//parameters_set_values(STRUCTClassDialog *prop_dialog, STRUCTParameter *param)
//{
//  gtk_entry_set_text(prop_dialog->param_name, param->name);
//  gtk_entry_set_text(prop_dialog->param_type, param->type);
//  if (param->value != NULL)
//    gtk_entry_set_text(prop_dialog->param_value, param->value);
//  else
//    gtk_entry_set_text(prop_dialog->param_value, "");
//  if (param->comment != NULL)
//    set_comment(prop_dialog->param_comment, param->comment);
//  else
//    set_comment(prop_dialog->param_comment, "");
//
//  gtk_option_menu_set_history(prop_dialog->param_kind_button,
//			      (gint)param->kind);
//}
//
//static void
//parameters_clear_values(STRUCTClassDialog *prop_dialog)
//{
//  gtk_entry_set_text(prop_dialog->param_name, "");
//  gtk_entry_set_text(prop_dialog->param_type, "");
//  gtk_entry_set_text(prop_dialog->param_value, "");
//  set_comment(prop_dialog->param_comment, "");
//  gtk_option_menu_set_history(prop_dialog->param_kind_button,
//			      (gint) STRUCT_UNDEF_KIND);
//
//}
//
//static void
//parameters_get_values (STRUCTClassDialog *prop_dialog, STRUCTParameter *param)
//{
//  g_free(param->name);
//  g_free(param->type);
//  g_free(param->comment);
//  if (param->value != NULL)
//    g_free(param->value);
//
//  param->name = g_strdup (gtk_entry_get_text (prop_dialog->param_name));
//  param->type = g_strdup (gtk_entry_get_text (prop_dialog->param_type));
//
//  param->value = g_strdup (gtk_entry_get_text(prop_dialog->param_value));
//  param->comment = g_strdup (get_comment(prop_dialog->param_comment));
//
//  param->kind = (STRUCTParameterKind) GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(gtk_menu_get_active(prop_dialog->param_kind))));
//}
//
//static void
//parameters_get_current_values(STRUCTClassDialog *prop_dialog)
//{
//  STRUCTParameter *current_param;
//  GtkLabel *label;
//  char *new_str;
//
//  if (prop_dialog->current_param != NULL) {
//    current_param = (STRUCTParameter *)
//      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_param));
//    if (current_param != NULL) {
//      parameters_get_values(prop_dialog, current_param);
//      label = GTK_LABEL(GTK_BIN(prop_dialog->current_param)->child);
//      new_str = struct_get_parameter_string(current_param);
//      gtk_label_set_text(label, new_str);
//      g_free(new_str);
//    }
//  }
//}
//
//
//static void
//parameters_list_selection_changed_callback(GtkWidget *gtklist,
//					   STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkObject *list_item;
//  STRUCTParameter *param;
//
//  prop_dialog = structclass->properties_dialog;
//
//  if (!prop_dialog)
//    return; /* maybe hiding a bug elsewhere */
//
//  parameters_get_current_values(prop_dialog);
//
//  list = GTK_LIST(gtklist)->selection;
//  if (!list) { /* No selected */
//    parameters_set_sensitive(prop_dialog, FALSE);
//    parameters_clear_values(prop_dialog);
//    prop_dialog->current_param = NULL;
//    return;
//  }
//
//  list_item = GTK_OBJECT(list->data);
//  param = (STRUCTParameter *)gtk_object_get_user_data(list_item);
//  parameters_set_values(prop_dialog, param);
//  parameters_set_sensitive(prop_dialog, TRUE);
//
//  prop_dialog->current_param = GTK_LIST_ITEM(list_item);
//  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->param_name));
//}
//
//static void
//parameters_list_new_callback(GtkWidget *button,
//			     STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *list_item;
//  STRUCTOperation *current_op;
//  STRUCTParameter *param;
//  char *utf;
//
//  prop_dialog = structclass->properties_dialog;
//
//  parameters_get_current_values(prop_dialog);
//
//  current_op = (STRUCTOperation *)
//    gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
//
//  param = struct_parameter_new();
//
//  utf = struct_get_parameter_string (param);
//  list_item = gtk_list_item_new_with_label (utf);
//  gtk_widget_show (list_item);
//  g_free (utf);
//
//  gtk_object_set_user_data(GTK_OBJECT(list_item), param);
//
//  current_op->parameters = g_list_append(current_op->parameters,
//					 (gpointer) param);
//
//  list = g_list_append(NULL, list_item);
//  gtk_list_append_items(prop_dialog->parameters_list, list);
//
//  if (prop_dialog->parameters_list->children != NULL)
//    gtk_list_unselect_child(prop_dialog->parameters_list,
//			    GTK_WIDGET(prop_dialog->parameters_list->children->data));
//  gtk_list_select_child(prop_dialog->parameters_list, list_item);
//
//  prop_dialog->current_param = GTK_LIST_ITEM(list_item);
//}
//
//static void
//parameters_list_delete_callback(GtkWidget *button,
//				STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkList *gtklist;
//  STRUCTOperation *current_op;
//  STRUCTParameter *param;
//
//  prop_dialog = structclass->properties_dialog;
//  gtklist = GTK_LIST(prop_dialog->parameters_list);
//
//
//  if (gtklist->selection != NULL) {
//    /* Remove from current operations parameter list: */
//    current_op = (STRUCTOperation *)
//      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
//    param = (STRUCTParameter *)
//      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_param));
//
//    current_op->parameters = g_list_remove(current_op->parameters,
//					   (gpointer) param);
//    struct_parameter_destroy(param);
//
//    /* Remove from gtk list: */
//    list = g_list_prepend(NULL, prop_dialog->current_param);
//
//    prop_dialog->current_param = NULL;
//
//    gtk_list_remove_items(gtklist, list);
//    g_list_free(list);
//  }
//}
//
//static void
//parameters_list_move_up_callback(GtkWidget *button,
//				 STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkList *gtklist;
//  GtkWidget *list_item;
//  STRUCTOperation *current_op;
//  STRUCTParameter *param;
//  int i;
//
//  prop_dialog = structclass->properties_dialog;
//  gtklist = GTK_LIST(prop_dialog->parameters_list);
//
//  if (gtklist->selection != NULL) {
//    list_item = GTK_WIDGET(gtklist->selection->data);
//
//    i = gtk_list_child_position(gtklist, list_item);
//    if (i>0)
//      i--;
//
//    param = (STRUCTParameter *) gtk_object_get_user_data(GTK_OBJECT(list_item));
//
//    /* Move parameter in current operations list: */
//    current_op = (STRUCTOperation *)
//      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
//
//    current_op->parameters = g_list_remove(current_op->parameters,
//					   (gpointer) param);
//    current_op->parameters = g_list_insert(current_op->parameters,
//					   (gpointer) param,
//					   i);
//
//    /* Move parameter in gtk list: */
//    gtk_widget_ref(list_item);
//    list = g_list_prepend(NULL, list_item);
//    gtk_list_remove_items(gtklist, list);
//    gtk_list_insert_items(gtklist, list, i);
//    gtk_widget_unref(list_item);
//
//    gtk_list_select_child(gtklist, list_item);
//
//    operations_get_current_values(prop_dialog);
//  }
//}
//
//static void
//parameters_list_move_down_callback(GtkWidget *button,
//				   STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkList *gtklist;
//  GtkWidget *list_item;
//  STRUCTOperation *current_op;
//  STRUCTParameter *param;
//  int i;
//
//  prop_dialog = structclass->properties_dialog;
//  gtklist = GTK_LIST(prop_dialog->parameters_list);
//
//  if (gtklist->selection != NULL) {
//    list_item = GTK_WIDGET(gtklist->selection->data);
//
//    i = gtk_list_child_position(gtklist, list_item);
//    if (i<(g_list_length(gtklist->children)-1))
//      i++;
//
//    param = (STRUCTParameter *) gtk_object_get_user_data(GTK_OBJECT(list_item));
//
//    /* Move parameter in current operations list: */
//    current_op = (STRUCTOperation *)
//      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
//
//    current_op->parameters = g_list_remove(current_op->parameters,
//					   (gpointer) param);
//    current_op->parameters = g_list_insert(current_op->parameters,
//					   (gpointer) param,
//					   i);
//
//    /* Move parameter in gtk list: */
//    gtk_widget_ref(list_item);
//    list = g_list_prepend(NULL, list_item);
//    gtk_list_remove_items(gtklist, list);
//    gtk_list_insert_items(gtklist, list, i);
//    gtk_widget_unref(list_item);
//
//    gtk_list_select_child(gtklist, list_item);
//
//    operations_get_current_values(prop_dialog);
//  }
//}
//
//static void
//operations_set_sensitive(STRUCTClassDialog *prop_dialog, gint val)
//{
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_name), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_type), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_stereotype), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_comment), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_visible_button), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_visible), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_class_scope), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_inheritance_type), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_inheritance_type_button), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_query), val);
//
//  gtk_widget_set_sensitive(prop_dialog->param_new_button, val);
//  gtk_widget_set_sensitive(prop_dialog->param_delete_button, val);
//  gtk_widget_set_sensitive(prop_dialog->param_down_button, val);
//  gtk_widget_set_sensitive(prop_dialog->param_up_button, val);
//}
//
//static void
//operations_set_values(STRUCTClassDialog *prop_dialog, STRUCTOperation *op)
//{
//  GList *list;
//  STRUCTParameter *param;
//  GtkWidget *list_item;
//  gchar *str;
//
//  gtk_entry_set_text(prop_dialog->op_name, op->name);
//  if (op->type != NULL)
//    gtk_entry_set_text(prop_dialog->op_type, op->type);
//  else
//    gtk_entry_set_text(prop_dialog->op_type, "");
//
//  if (op->stereotype != NULL)
//    gtk_entry_set_text(prop_dialog->op_stereotype, op->stereotype);
//  else
//    gtk_entry_set_text(prop_dialog->op_stereotype, "");
//
//  if (op->comment != NULL)
//    set_comment(prop_dialog->op_comment, op->comment);
//  else
//    set_comment(prop_dialog->op_comment, "");
//
//  gtk_option_menu_set_history(prop_dialog->op_visible_button,
//			      (gint)op->visibility);
//  gtk_toggle_button_set_active(prop_dialog->op_class_scope, op->class_scope);
//  gtk_toggle_button_set_active(prop_dialog->op_query, op->query);
//  gtk_option_menu_set_history(prop_dialog->op_inheritance_type_button,
//			      (gint)op->inheritance_type);
//
//  gtk_list_clear_items(prop_dialog->parameters_list, 0, -1);
//  prop_dialog->current_param = NULL;
//  parameters_set_sensitive(prop_dialog, FALSE);
//
//  list = op->parameters;
//  while (list != NULL) {
//    param = (STRUCTParameter *)list->data;
//
//    str = struct_get_parameter_string (param);
//    list_item = gtk_list_item_new_with_label (str);
//    g_free (str);
//
//    gtk_object_set_user_data(GTK_OBJECT(list_item), (gpointer) param);
//    gtk_container_add (GTK_CONTAINER (prop_dialog->parameters_list), list_item);
//    gtk_widget_show (list_item);
//
//    list = g_list_next(list);
//  }
//}
//
//static void
//operations_clear_values(STRUCTClassDialog *prop_dialog)
//{
//  gtk_entry_set_text(prop_dialog->op_name, "");
//  gtk_entry_set_text(prop_dialog->op_type, "");
//  gtk_entry_set_text(prop_dialog->op_stereotype, "");
//  set_comment(prop_dialog->op_comment, "");
//  gtk_toggle_button_set_active(prop_dialog->op_class_scope, FALSE);
//  gtk_toggle_button_set_active(prop_dialog->op_query, FALSE);
//
//  gtk_list_clear_items(prop_dialog->parameters_list, 0, -1);
//  prop_dialog->current_param = NULL;
//  parameters_set_sensitive(prop_dialog, FALSE);
//}
//
//
//static void
//operations_get_values(STRUCTClassDialog *prop_dialog, STRUCTOperation *op)
//{
//  const gchar *s;
//
//  g_free(op->name);
//  if (op->type != NULL)
//	  g_free(op->type);
//
//  op->name = g_strdup(gtk_entry_get_text(prop_dialog->op_name));
//  op->type = g_strdup (gtk_entry_get_text(prop_dialog->op_type));
//  op->comment = g_strdup(get_comment(prop_dialog->op_comment));
//
//  s = gtk_entry_get_text(prop_dialog->op_stereotype);
//  if (s && s[0])
//    op->stereotype = g_strdup (s);
//  else
//    op->stereotype = NULL;
//
//  op->visibility = (STRUCTVisibility)
//    GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(gtk_menu_get_active(prop_dialog->op_visible))));
//
//  op->class_scope = prop_dialog->op_class_scope->active;
//  op->inheritance_type = (STRUCTInheritanceType)
//    GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(gtk_menu_get_active(prop_dialog->op_inheritance_type))));
//
//  op->query = prop_dialog->op_query->active;
//
//}
//
//static void
//operations_get_current_values(STRUCTClassDialog *prop_dialog)
//{
//  STRUCTOperation *current_op;
//  GtkLabel *label;
//  char *new_str;
//
//  parameters_get_current_values(prop_dialog);
//
//  if (prop_dialog->current_op != NULL) {
//    current_op = (STRUCTOperation *)
//      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
//    if (current_op != NULL) {
//      operations_get_values(prop_dialog, current_op);
//      label = GTK_LABEL(GTK_BIN(prop_dialog->current_op)->child);
//      new_str = struct_get_operation_string(current_op);
//      gtk_label_set_text (label, new_str);
//      g_free (new_str);
//    }
//  }
//}
//
//static void
//operations_list_item_destroy_callback(GtkWidget *list_item,
//				      gpointer data)
//{
//  STRUCTOperation *op;
//
//  op = (STRUCTOperation *) gtk_object_get_user_data(GTK_OBJECT(list_item));
//
//  if (op != NULL) {
//    struct_operation_destroy(op);
//    /*printf("Destroying operation list_item's user_data!\n");*/
//  }
//}
//
//static void
//operations_list_selection_changed_callback(GtkWidget *gtklist,
//					   STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkObject *list_item;
//  STRUCTOperation *op;
//
//  prop_dialog = structclass->properties_dialog;
//
//  if (!prop_dialog)
//    return; /* maybe hiding a bug elsewhere */
//
//  operations_get_current_values(prop_dialog);
//
//  list = GTK_LIST(gtklist)->selection;
//  if (!list) { /* No selected */
//    operations_set_sensitive(prop_dialog, FALSE);
//    operations_clear_values(prop_dialog);
//    prop_dialog->current_op = NULL;
//    return;
//  }
//
//  list_item = GTK_OBJECT(list->data);
//  op = (STRUCTOperation *)gtk_object_get_user_data(list_item);
//  operations_set_values(prop_dialog, op);
//  operations_set_sensitive(prop_dialog, TRUE);
//
//  prop_dialog->current_op = GTK_LIST_ITEM(list_item);
//  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->op_name));
//}
//
//static void
//operations_list_new_callback(GtkWidget *button,
//			     STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *list_item;
//  STRUCTOperation *op;
//  char *utfstr;
//
//  prop_dialog = structclass->properties_dialog;
//
//  operations_get_current_values(prop_dialog);
//
//  op = struct_operation_new();
//  /* need to make new ConnectionPoints valid and remember them */
//  struct_operation_ensure_connection_points (op, &structclass->element.object);
//  prop_dialog->added_connections =
//    g_list_prepend(prop_dialog->added_connections, op->left_connection);
//  prop_dialog->added_connections =
//    g_list_prepend(prop_dialog->added_connections, op->right_connection);
//
//
//  utfstr = struct_get_operation_string (op);
//  list_item = gtk_list_item_new_with_label (utfstr);
//  gtk_widget_show (list_item);
//  g_free (utfstr);
//
//  gtk_object_set_user_data(GTK_OBJECT(list_item), op);
//  gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
//		      GTK_SIGNAL_FUNC (operations_list_item_destroy_callback),
//		      NULL);
//
//  list = g_list_append(NULL, list_item);
//  gtk_list_append_items(prop_dialog->operations_list, list);
//
//  if (prop_dialog->operations_list->children != NULL)
//    gtk_list_unselect_child(prop_dialog->operations_list,
//			    GTK_WIDGET(prop_dialog->operations_list->children->data));
//  gtk_list_select_child(prop_dialog->operations_list, list_item);
//}
//
//static void
//operations_list_delete_callback(GtkWidget *button,
//				STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkList *gtklist;
//  STRUCTOperation *op;
//
//
//  prop_dialog = structclass->properties_dialog;
//  gtklist = GTK_LIST(prop_dialog->operations_list);
//
//  if (gtklist->selection != NULL) {
//    op = (STRUCTOperation *)
//      gtk_object_get_user_data(GTK_OBJECT(gtklist->selection->data));
//
//    if (op->left_connection != NULL) {
//      prop_dialog->deleted_connections =
//	g_list_prepend(prop_dialog->deleted_connections,
//		       op->left_connection);
//      prop_dialog->deleted_connections =
//	g_list_prepend(prop_dialog->deleted_connections,
//		       op->right_connection);
//    }
//
//    list = g_list_prepend(NULL, gtklist->selection->data);
//    gtk_list_remove_items(gtklist, list);
//    g_list_free(list);
//    operations_clear_values(prop_dialog);
//    operations_set_sensitive(prop_dialog, FALSE);
//  }
//}
//
//static void
//operations_list_move_up_callback(GtkWidget *button,
//				 STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkList *gtklist;
//  GtkWidget *list_item;
//  int i;
//
//  prop_dialog = structclass->properties_dialog;
//  gtklist = GTK_LIST(prop_dialog->operations_list);
//
//  if (gtklist->selection != NULL) {
//    list_item = GTK_WIDGET(gtklist->selection->data);
//
//    i = gtk_list_child_position(gtklist, list_item);
//    if (i>0)
//      i--;
//
//    gtk_widget_ref(list_item);
//    list = g_list_prepend(NULL, list_item);
//    gtk_list_remove_items(gtklist, list);
//    gtk_list_insert_items(gtklist, list, i);
//    gtk_widget_unref(list_item);
//
//    gtk_list_select_child(gtklist, list_item);
//  }
//
//}
//
//static void
//operations_list_move_down_callback(GtkWidget *button,
//				   STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkList *gtklist;
//  GtkWidget *list_item;
//  int i;
//
//  prop_dialog = structclass->properties_dialog;
//  gtklist = GTK_LIST(prop_dialog->operations_list);
//
//  if (gtklist->selection != NULL) {
//    list_item = GTK_WIDGET(gtklist->selection->data);
//
//    i = gtk_list_child_position(gtklist, list_item);
//    if (i<(g_list_length(gtklist->children)-1))
//      i++;
//
//    gtk_widget_ref(list_item);
//    list = g_list_prepend(NULL, list_item);
//    gtk_list_remove_items(gtklist, list);
//    gtk_list_insert_items(gtklist, list, i);
//    gtk_widget_unref(list_item);
//
//    gtk_list_select_child(gtklist, list_item);
//  }
//}
//
//static void
//operations_read_from_dialog(STRUCTClass *structclass,
//			    STRUCTClassDialog *prop_dialog,
//			    int connection_index)
//{
//  GList *list;
//  STRUCTOperation *op;
//  GtkWidget *list_item;
//  GList *clear_list;
//  DiaObject *obj;
//
//  obj = &structclass->element.object;
//
//  /* if currently select op is changed in the entries, update from widgets */
//  operations_get_current_values(prop_dialog);
//
//  /* Free current operations: */
//  list = structclass->operations;
//  while (list != NULL) {
//    op = (STRUCTOperation *)list->data;
//    struct_operation_destroy(op);
//    list = g_list_next(list);
//  }
//  g_list_free (structclass->operations);
//  structclass->operations = NULL;
//
//  /* Insert new operations and remove them from gtklist: */
//  list = GTK_LIST (prop_dialog->operations_list)->children;
//  clear_list = NULL;
//  while (list != NULL) {
//    list_item = GTK_WIDGET(list->data);
//
//    clear_list = g_list_prepend (clear_list, list_item);
//    op = (STRUCTOperation *)
//      gtk_object_get_user_data(GTK_OBJECT(list_item));
//    gtk_object_set_user_data(GTK_OBJECT(list_item), NULL);
//    structclass->operations = g_list_append(structclass->operations, op);
//
//    if (op->left_connection == NULL) {
//      struct_operation_ensure_connection_points (op, obj);
//
//      prop_dialog->added_connections =
//	g_list_prepend(prop_dialog->added_connections,
//		       op->left_connection);
//      prop_dialog->added_connections =
//	g_list_prepend(prop_dialog->added_connections,
//		       op->right_connection);
//    }
//
//    if ( (prop_dialog->op_vis->active) &&
//	 (!prop_dialog->op_supp->active) ) {
//      obj->connections[connection_index] = op->left_connection;
//      connection_index++;
//      obj->connections[connection_index] = op->right_connection;
//      connection_index++;
//    } else {
//      structclass_store_disconnects(prop_dialog, op->left_connection);
//      object_remove_connections_to(op->left_connection);
//      structclass_store_disconnects(prop_dialog, op->right_connection);
//      object_remove_connections_to(op->right_connection);
//    }
//
//    list = g_list_next(list);
//  }
//  clear_list = g_list_reverse (clear_list);
//  gtk_list_remove_items (GTK_LIST (prop_dialog->operations_list), clear_list);
//  g_list_free (clear_list);
//}
//
//static void
//operations_fill_in_dialog(STRUCTClass *structclass)
//{
//  STRUCTClassDialog *prop_dialog;
//  STRUCTOperation *op_copy;
//  GtkWidget *list_item;
//  GList *list;
//  int i;
//
//  prop_dialog = structclass->properties_dialog;
//
//  if (prop_dialog->operations_list->children == NULL) {
//    i = 0;
//    list = structclass->operations;
//    while (list != NULL) {
//      STRUCTOperation *op = (STRUCTOperation *)list->data;
//      gchar *opstr = struct_get_operation_string (op);
//
//      list_item = gtk_list_item_new_with_label (opstr);
//      op_copy = struct_operation_copy (op);
//      /* Looks wrong but is required for the complicate connections memory management */
//      op_copy->left_connection = op->left_connection;
//      op_copy->right_connection = op->right_connection;
//      gtk_object_set_user_data(GTK_OBJECT(list_item), (gpointer) op_copy);
//      gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
//			  GTK_SIGNAL_FUNC (operations_list_item_destroy_callback),
//			  NULL);
//      gtk_container_add (GTK_CONTAINER (prop_dialog->operations_list), list_item);
//      gtk_widget_show (list_item);
//
//      list = g_list_next(list); i++;
//      g_free (opstr);
//    }
//
//    /* set operations non-sensitive */
//    prop_dialog->current_op = NULL;
//    operations_set_sensitive(prop_dialog, FALSE);
//    operations_clear_values(prop_dialog);
//  }
//}
//
//static void
//operations_update(GtkWidget *widget, STRUCTClass *structclass)
//{
//  operations_get_current_values(structclass->properties_dialog);
//}
//
//static int
//operations_update_event(GtkWidget *widget, GdkEventFocus *ev, STRUCTClass *structclass)
//{
//  operations_get_current_values(structclass->properties_dialog);
//  return 0;
//}
//
//static GtkWidget*
//operations_data_create_hbox (STRUCTClass *structclass)
//{
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *hbox;
//  GtkWidget *vbox2;
//  GtkWidget *table;
//  GtkWidget *label;
//  GtkWidget *entry;
//  GtkWidget *omenu;
//  GtkWidget *menu;
//  GtkWidget *submenu;
//  GtkWidget *scrolledwindow;
//  GtkWidget *checkbox;
//  GSList *group;
//
//  prop_dialog = structclass->properties_dialog;
//
//  hbox = gtk_hbox_new(FALSE, 5);
//  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
//
//  vbox2 = gtk_vbox_new(FALSE, 0);
//
//  /* table containing operation 'name' up to 'query' and also the comment */
//  table = gtk_table_new (5, 3, FALSE);
//  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
//  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
//
//  label = gtk_label_new(_("Name:"));
//  entry = gtk_entry_new();
//  prop_dialog->op_name = GTK_ENTRY(entry);
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (operations_update), structclass);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  label = gtk_label_new(_("Type:"));
//  entry = gtk_entry_new();
//  prop_dialog->op_type = GTK_ENTRY(entry);
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (operations_update), structclass);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  label = gtk_label_new(_("Stereotype:"));
//  entry = gtk_entry_new();
//  prop_dialog->op_stereotype = GTK_ENTRY(entry);
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (operations_update), structclass);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//
//  label = gtk_label_new(_("Visibility:"));
//
//  omenu = gtk_option_menu_new ();
//  menu = gtk_menu_new ();
//  prop_dialog->op_visible = GTK_MENU(menu);
//  prop_dialog->op_visible_button = GTK_OPTION_MENU(omenu);
//  submenu = NULL;
//  group = NULL;
//
//  add_option_menu_item(GTK_MENU(menu), _("Public"),
//		       GTK_SIGNAL_FUNC (operations_update), structclass,
//		       GINT_TO_POINTER(STRUCT_PUBLIC) );
//  add_option_menu_item(GTK_MENU(menu), _("Private"),
//		       GTK_SIGNAL_FUNC (operations_update), structclass,
//		       GINT_TO_POINTER(STRUCT_PRIVATE) );
//  add_option_menu_item(GTK_MENU(menu), _("Protected"),
//		       GTK_SIGNAL_FUNC (operations_update), structclass,
//		       GINT_TO_POINTER(STRUCT_PROTECTED) );
//  add_option_menu_item(GTK_MENU(menu), _("Implementation"),
//		       GTK_SIGNAL_FUNC (operations_update), structclass,
//		       GINT_TO_POINTER(STRUCT_IMPLEMENTATION) );
//
//  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
//
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//					/* left, right, top, bottom */
//  gtk_table_attach (GTK_TABLE (table), label, 2,3,0,1, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), omenu, 3,4,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);
//  /* end: Visibility */
//
//  label = gtk_label_new(_("Inheritance type:"));
//
//  omenu = gtk_option_menu_new ();
//  menu = gtk_menu_new ();
//  prop_dialog->op_inheritance_type = GTK_MENU(menu);
//  prop_dialog->op_inheritance_type_button = GTK_OPTION_MENU(omenu);
//  submenu = NULL;
//  group = NULL;
//
//  add_option_menu_item(GTK_MENU(menu), _("Abstract"),
//		       GTK_SIGNAL_FUNC (operations_update),  structclass,
//		       GINT_TO_POINTER(STRUCT_ABSTRACT) );
//  add_option_menu_item(GTK_MENU(menu), _("Polymorphic (virtual)"),
//		       GTK_SIGNAL_FUNC (operations_update), structclass,
//		       GINT_TO_POINTER(STRUCT_POLYMORPHIC) );
//  add_option_menu_item(GTK_MENU(menu), _("Leaf (final)"),
//		       GTK_SIGNAL_FUNC (operations_update), structclass,
//		       GINT_TO_POINTER(STRUCT_LEAF) );
//
//  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
//
//  gtk_table_attach (GTK_TABLE (table), label, 2,3,1,2, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), omenu, 3,4,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);
//  /* end: Inheritance type */
//
//  checkbox = gtk_check_button_new_with_label(_("Class scope"));
//  prop_dialog->op_class_scope = GTK_TOGGLE_BUTTON(checkbox);
//  gtk_table_attach (GTK_TABLE (table), checkbox, 2,3,2,3, GTK_FILL,0, 0,2);
//
//  checkbox = gtk_check_button_new_with_label(_("Query"));
//  prop_dialog->op_query = GTK_TOGGLE_BUTTON(checkbox);
//  gtk_table_attach (GTK_TABLE (table), checkbox, 3,4,2,3, GTK_FILL,0, 2,0);
//
//  label = gtk_label_new(_("Comment:"));
//  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
//  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);
//  /* with GTK_POLICY_NEVER the comment filed gets smaller unti l text is entered; than it would resize the dialog! */
//  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
//
//  entry = gtk_text_view_new ();
//  prop_dialog->op_comment = GTK_TEXT_VIEW(entry);
//  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
//  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
//  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (entry),TRUE);
//
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
//#if 0 /* no "activate" signal on GtkTextView */
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (operations_update), structclass);
//#endif
//  gtk_table_attach (GTK_TABLE (table), label, 4,5,0,1, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), scrolledwindow, 4,5,1,3, GTK_FILL | GTK_EXPAND,0, 0,0);
//  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
//
//  return hbox;
//}
//
//static GtkWidget*
//operations_parameters_editor_create_vbox (STRUCTClass *structclass)
//{
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *vbox2;
//  GtkWidget *hbox2;
//  GtkWidget *vbox3;
//  GtkWidget *label;
//  GtkWidget *scrolled_win;
//  GtkWidget *list;
//  GtkWidget *button;
//
//  prop_dialog = structclass->properties_dialog;
//
//  vbox2 = gtk_vbox_new(FALSE, 5);
//  /* Parameters list label */
//  hbox2 = gtk_hbox_new(FALSE, 5);
//
//  label = gtk_label_new(_("Parameters:"));
//  gtk_box_pack_start( GTK_BOX(hbox2), label, FALSE, TRUE, 0);
//
//  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);
//
//  /* Parameters list editor - with of list at least width of buttons*/
//  hbox2 = gtk_hbox_new(TRUE, 5);
//
//  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
//  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
//				  GTK_POLICY_AUTOMATIC,
//				  GTK_POLICY_AUTOMATIC);
//  gtk_box_pack_start (GTK_BOX (hbox2), scrolled_win, TRUE, TRUE, 0);
//  gtk_widget_show (scrolled_win);
//
//  list = gtk_list_new ();
//  prop_dialog->parameters_list = GTK_LIST(list);
//  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
//  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
//  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
//				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
//  gtk_widget_show (list);
//
//  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
//		      GTK_SIGNAL_FUNC(parameters_list_selection_changed_callback),
//		      structclass);
//
//  vbox3 = gtk_vbox_new(FALSE, 5);
//
//  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
//  prop_dialog->param_new_button = button;
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(parameters_list_new_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
//  prop_dialog->param_delete_button = button;
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(parameters_list_delete_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//  button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
//  prop_dialog->param_up_button = button;
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(parameters_list_move_up_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//  button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
//  prop_dialog->param_down_button = button;
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(parameters_list_move_down_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//
//  gtk_box_pack_start (GTK_BOX (hbox2), vbox3, FALSE, TRUE, 0);
//
//  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);
//  /* end: Parameter list editor */
//
//  return vbox2;
//}
//
//static GtkWidget*
//operations_parameters_data_create_vbox (STRUCTClass *structclass)
//{
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *vbox2;
//  GtkWidget *frame;
//  GtkWidget *vbox3;
//  GtkWidget *table;
//  GtkWidget *label;
//  GtkWidget *entry;
//  GtkWidget *scrolledwindow;
//  GtkWidget *omenu;
//  GtkWidget *menu;
//  GtkWidget *submenu;
//  GSList *group;
//
//  prop_dialog = structclass->properties_dialog;
//
//  vbox2 = gtk_vbox_new(FALSE, 5);
//  frame = gtk_frame_new(_("Parameter data"));
//  vbox3 = gtk_vbox_new(FALSE, 5);
//  gtk_container_set_border_width (GTK_CONTAINER (vbox3), 5);
//  gtk_container_add (GTK_CONTAINER (frame), vbox3);
//  gtk_widget_show(frame);
//  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, TRUE, 0);
//
//  table = gtk_table_new (3, 4, FALSE);
//  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
//  gtk_box_pack_start (GTK_BOX (vbox3), table, FALSE, FALSE, 0);
//
//  label = gtk_label_new(_("Name:"));
//  entry = gtk_entry_new();
//  prop_dialog->param_name = GTK_ENTRY(entry);
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (operations_update), structclass);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  label = gtk_label_new(_("Type:"));
//  entry = gtk_entry_new();
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (operations_update), structclass);
//  prop_dialog->param_type = GTK_ENTRY(entry);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  label = gtk_label_new(_("Def. value:"));
//  entry = gtk_entry_new();
//  prop_dialog->param_value = GTK_ENTRY(entry);
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (operations_update), structclass);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  label = gtk_label_new(_("Comment:"));
//  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
//  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
//				       GTK_SHADOW_IN);
//  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
//				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
//
//  entry = gtk_text_view_new ();
//  prop_dialog->param_comment = GTK_TEXT_VIEW(entry);
//  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
//  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
//  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (entry),TRUE);
//
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
//#if 0 /* no "activate" signal on GtkTextView */
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (operations_update), structclass);
//#endif
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 2,3,1,2, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), scrolledwindow, 3,4,1,3, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  label = gtk_label_new(_("Direction:"));
//
//  omenu = gtk_option_menu_new ();
//  menu = gtk_menu_new ();
//  prop_dialog->param_kind = GTK_MENU(menu);
//  prop_dialog->param_kind_button = GTK_OPTION_MENU(omenu);
//  submenu = NULL;
//  group = NULL;
//
//  add_option_menu_item(GTK_MENU(menu), _("Undefined"),
//		       GTK_SIGNAL_FUNC (operations_update), structclass,
//		       GINT_TO_POINTER(STRUCT_UNDEF_KIND) );
//  add_option_menu_item(GTK_MENU(menu), _("In"),
//		       GTK_SIGNAL_FUNC (operations_update), structclass,
//		       GINT_TO_POINTER(STRUCT_IN) );
//  add_option_menu_item(GTK_MENU(menu), _("Out"),
//		       GTK_SIGNAL_FUNC (operations_update), structclass,
//		       GINT_TO_POINTER(STRUCT_OUT) );
//  add_option_menu_item(GTK_MENU(menu), _("In & Out"),
//		       GTK_SIGNAL_FUNC (operations_update), structclass,
//		       GINT_TO_POINTER(STRUCT_INOUT) );
//
//  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
//
//  {
//    GtkWidget * align;
//    align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
//    gtk_container_add(GTK_CONTAINER(align), omenu);
//    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//    gtk_table_attach (GTK_TABLE (table), label, 2,3,0,1, GTK_FILL,0, 0,3);
//    gtk_table_attach (GTK_TABLE (table), align, 3,4,0,1, GTK_FILL,0, 0,3);
//  }
//
//  return vbox2;
//}
//
//static void
//operations_create_page(GtkNotebook *notebook,  STRUCTClass *structclass)
//{
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *page_label;
//  GtkWidget *hbox;
//  GtkWidget *vbox;
//  GtkWidget *vbox2;
//  GtkWidget *vbox3;
//  GtkWidget *scrolled_win;
//  GtkWidget *button;
//  GtkWidget *list;
//  GtkWidget *frame;
//
//  prop_dialog = structclass->properties_dialog;
//
//  /* Operations page: */
//  page_label = gtk_label_new_with_mnemonic (_("_Operations"));
//
//  vbox = gtk_vbox_new(FALSE, 5);
//  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
//
//  hbox = gtk_hbox_new(FALSE, 5);
//
//  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
//  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
//				  GTK_POLICY_AUTOMATIC,
//				  GTK_POLICY_AUTOMATIC);
//  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
//  gtk_widget_show (scrolled_win);
//
//  list = gtk_list_new ();
//  prop_dialog->operations_list = GTK_LIST(list);
//  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
//  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
//  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
//				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
//  gtk_widget_show (list);
//
//  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
//		      GTK_SIGNAL_FUNC(operations_list_selection_changed_callback),
//		      structclass);
//
//  vbox2 = gtk_vbox_new(FALSE, 5);
//
//  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(operations_list_new_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(operations_list_delete_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//  button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(operations_list_move_up_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//  button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(operations_list_move_down_callback),
//		      structclass);
//
//  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//
//  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);
//
//  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
//
//  frame = gtk_frame_new(_("Operation data"));
//  vbox2 = gtk_vbox_new(FALSE, 0);
//  hbox = operations_data_create_hbox (structclass);
//  gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 0);
//  gtk_container_add (GTK_CONTAINER (frame), vbox2);
//  gtk_widget_show(frame);
//  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
//
//  /* parameter stuff below operation stuff */
//  hbox = gtk_hbox_new (FALSE, 5);
//  vbox3 = operations_parameters_editor_create_vbox (structclass);
//  gtk_box_pack_start (GTK_BOX (hbox), vbox3, TRUE, TRUE, 5);
//
//  vbox3 = operations_parameters_data_create_vbox (structclass);
//  gtk_box_pack_start (GTK_BOX (hbox), vbox3, TRUE, TRUE, 5);
//
//  gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 5);
//
//  gtk_widget_show_all (vbox);
//  gtk_widget_show (page_label);
//  gtk_notebook_append_page (notebook, vbox, page_label);
//}

/************************************************************
 ******************** TEMPLATES *****************************
 ************************************************************/

//static void
//templates_set_sensitive(STRUCTClassDialog *prop_dialog, gint val)
//{
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->templ_name), val);
//  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->templ_type), val);
//}
//
//static void
//templates_set_values (STRUCTClassDialog *prop_dialog,
//		      STRUCTFormalParameter *param)
//{
//  if (param->name)
//    gtk_entry_set_text(prop_dialog->templ_name, param->name);
//  if (param->type != NULL)
//    gtk_entry_set_text(prop_dialog->templ_type, param->type);
//}
//
//static void
//templates_clear_values(STRUCTClassDialog *prop_dialog)
//{
//  gtk_entry_set_text(prop_dialog->templ_name, "");
//  gtk_entry_set_text(prop_dialog->templ_type, "");
//}
//
//static void
//templates_get_values(STRUCTClassDialog *prop_dialog, STRUCTFormalParameter *param)
//{
//  g_free(param->name);
//  if (param->type != NULL)
//    g_free(param->type);
//
//  param->name = g_strdup (gtk_entry_get_text (prop_dialog->templ_name));
//  param->type = g_strdup (gtk_entry_get_text (prop_dialog->templ_type));
//}
//
//
//static void
//templates_get_current_values(STRUCTClassDialog *prop_dialog)
//{
//  STRUCTFormalParameter *current_param;
//  GtkLabel *label;
//  gchar* new_str;
//
//  if (prop_dialog->current_templ != NULL) {
//    current_param = (STRUCTFormalParameter *)
//      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_templ));
//    if (current_param != NULL) {
//      templates_get_values(prop_dialog, current_param);
//      label = GTK_LABEL(GTK_BIN(prop_dialog->current_templ)->child);
//      new_str = struct_get_formalparameter_string (current_param);
//      gtk_label_set_text(label, new_str);
//      g_free(new_str);
//    }
//  }
//}
//
//static void
//templates_list_item_destroy_callback(GtkWidget *list_item,
//				     gpointer data)
//{
//  STRUCTFormalParameter *param;
//
//  param = (STRUCTFormalParameter *)
//    gtk_object_get_user_data(GTK_OBJECT(list_item));
//
//  if (param != NULL) {
//    struct_formalparameter_destroy(param);
//    /*printf("Destroying list_item's user_data!\n"); */
//  }
//}
//
//static void
//templates_list_selection_changed_callback(GtkWidget *gtklist,
//					  STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkObject *list_item;
//  STRUCTFormalParameter *param;
//
//  prop_dialog = structclass->properties_dialog;
//
//  if (!prop_dialog)
//    return; /* maybe hiding a bug elsewhere */
//
//  templates_get_current_values(prop_dialog);
//
//  list = GTK_LIST(gtklist)->selection;
//  if (!list) { /* No selected */
//    templates_set_sensitive(prop_dialog, FALSE);
//    templates_clear_values(prop_dialog);
//    prop_dialog->current_templ = NULL;
//    return;
//  }
//
//  list_item = GTK_OBJECT(list->data);
//  param = (STRUCTFormalParameter *)gtk_object_get_user_data(list_item);
//  templates_set_values(prop_dialog, param);
//  templates_set_sensitive(prop_dialog, TRUE);
//
//  prop_dialog->current_templ = GTK_LIST_ITEM(list_item);
//  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->templ_name));
//}
//
//static void
//templates_list_new_callback(GtkWidget *button,
//			    STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *list_item;
//  STRUCTFormalParameter *param;
//  char *utfstr;
//
//  prop_dialog = structclass->properties_dialog;
//
//  templates_get_current_values(prop_dialog);
//
//  param = struct_formalparameter_new();
//
//  utfstr = struct_get_formalparameter_string (param);
//  list_item = gtk_list_item_new_with_label (utfstr);
//  gtk_widget_show (list_item);
//  g_free (utfstr);
//
//  gtk_object_set_user_data(GTK_OBJECT(list_item), param);
//  gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
//		      GTK_SIGNAL_FUNC (templates_list_item_destroy_callback),
//		      NULL);
//
//  list = g_list_append(NULL, list_item);
//  gtk_list_append_items(prop_dialog->templates_list, list);
//
//  if (prop_dialog->templates_list->children != NULL)
//    gtk_list_unselect_child(prop_dialog->templates_list,
//			    GTK_WIDGET(prop_dialog->templates_list->children->data));
//  gtk_list_select_child(prop_dialog->templates_list, list_item);
//}
//
//static void
//templates_list_delete_callback(GtkWidget *button,
//			       STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkList *gtklist;
//
//  prop_dialog = structclass->properties_dialog;
//  gtklist = GTK_LIST(prop_dialog->templates_list);
//
//  if (gtklist->selection != NULL) {
//    list = g_list_prepend(NULL, gtklist->selection->data);
//    gtk_list_remove_items(gtklist, list);
//    g_list_free(list);
//    templates_clear_values(prop_dialog);
//    templates_set_sensitive(prop_dialog, FALSE);
//  }
//}
//
//static void
//templates_list_move_up_callback(GtkWidget *button,
//				STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkList *gtklist;
//  GtkWidget *list_item;
//  int i;
//
//  prop_dialog = structclass->properties_dialog;
//  gtklist = GTK_LIST(prop_dialog->templates_list);
//
//  if (gtklist->selection != NULL) {
//    list_item = GTK_WIDGET(gtklist->selection->data);
//
//    i = gtk_list_child_position(gtklist, list_item);
//    if (i>0)
//      i--;
//
//    gtk_widget_ref(list_item);
//    list = g_list_prepend(NULL, list_item);
//    gtk_list_remove_items(gtklist, list);
//    gtk_list_insert_items(gtklist, list, i);
//    gtk_widget_unref(list_item);
//
//    gtk_list_select_child(gtklist, list_item);
//  }
//}
//
//static void
//templates_list_move_down_callback(GtkWidget *button,
//				   STRUCTClass *structclass)
//{
//  GList *list;
//  STRUCTClassDialog *prop_dialog;
//  GtkList *gtklist;
//  GtkWidget *list_item;
//  int i;
//
//  prop_dialog = structclass->properties_dialog;
//  gtklist = GTK_LIST(prop_dialog->templates_list);
//
//  if (gtklist->selection != NULL) {
//    list_item = GTK_WIDGET(gtklist->selection->data);
//
//    i = gtk_list_child_position(gtklist, list_item);
//    if (i<(g_list_length(gtklist->children)-1))
//      i++;
//
//    gtk_widget_ref(list_item);
//    list = g_list_prepend(NULL, list_item);
//    gtk_list_remove_items(gtklist, list);
//    gtk_list_insert_items(gtklist, list, i);
//    gtk_widget_unref(list_item);
//
//    gtk_list_select_child(gtklist, list_item);
//  }
//}
//
//
//static void
//templates_read_from_dialog(STRUCTClass *structclass, STRUCTClassDialog *prop_dialog)
//{
//  GList *list;
//  STRUCTFormalParameter *param;
//  GtkWidget *list_item;
//  GList *clear_list;
//
//  templates_get_current_values(prop_dialog); /* if changed, update from widgets */
//
//  structclass->template = prop_dialog->templ_template->active;
//
//  /* Free current formal parameters: */
//  list = structclass->formal_params;
//  while (list != NULL) {
//    param = (STRUCTFormalParameter *)list->data;
//    struct_formalparameter_destroy(param);
//    list = g_list_next(list);
//  }
//  g_list_free (structclass->formal_params);
//  structclass->formal_params = NULL;
//
//  /* Insert new formal params and remove them from gtklist: */
//  list = GTK_LIST (prop_dialog->templates_list)->children;
//  clear_list = NULL;
//  while (list != NULL) {
//    list_item = GTK_WIDGET(list->data);
//    clear_list = g_list_prepend (clear_list, list_item);
//    param = (STRUCTFormalParameter *)
//      gtk_object_get_user_data(GTK_OBJECT(list_item));
//    gtk_object_set_user_data(GTK_OBJECT(list_item), NULL);
//    structclass->formal_params = g_list_append(structclass->formal_params, param);
//    list = g_list_next(list);
//  }
//  clear_list = g_list_reverse (clear_list);
//  gtk_list_remove_items (GTK_LIST (prop_dialog->templates_list), clear_list);
//  g_list_free (clear_list);
//}
//
//static void
//templates_fill_in_dialog(STRUCTClass *structclass)
//{
//  STRUCTClassDialog *prop_dialog;
//  STRUCTFormalParameter *param_copy;
//  GList *list;
//  GtkWidget *list_item;
//  int i;
//  prop_dialog = structclass->properties_dialog;
//
//  gtk_toggle_button_set_active(prop_dialog->templ_template, structclass->template);
//
//  /* copy in new template-parameters: */
//  if (prop_dialog->templates_list->children == NULL) {
//    i = 0;
//    list = structclass->formal_params;
//    while (list != NULL) {
//      STRUCTFormalParameter *param = (STRUCTFormalParameter *)list->data;
//      gchar *paramstr = struct_get_formalparameter_string(param);
//
//      list_item = gtk_list_item_new_with_label (paramstr);
//      param_copy = struct_formalparameter_copy(param);
//      gtk_object_set_user_data(GTK_OBJECT(list_item),
//			       (gpointer) param_copy);
//      gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
//			  GTK_SIGNAL_FUNC (templates_list_item_destroy_callback),
//			  NULL);
//      gtk_container_add (GTK_CONTAINER (prop_dialog->templates_list),
//			 list_item);
//      gtk_widget_show (list_item);
//
//      list = g_list_next(list); i++;
//      g_free (paramstr);
//    }
//    /* set templates non-sensitive */
//    prop_dialog->current_templ = NULL;
//    templates_set_sensitive(prop_dialog, FALSE);
//    templates_clear_values(prop_dialog);
//  }
//
//}


//static void
//templates_update(GtkWidget *widget, STRUCTClass *structclass)
//{
//  templates_get_current_values(structclass->properties_dialog);
//}
//
//static int
//templates_update_event(GtkWidget *widget, GdkEventFocus *ev, STRUCTClass *structclass)
//{
//  templates_get_current_values(structclass->properties_dialog);
//  return 0;
//}
//
//static void
//templates_create_page(GtkNotebook *notebook,  STRUCTClass *structclass)
//{
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *page_label;
//  GtkWidget *label;
//  GtkWidget *hbox;
//  GtkWidget *vbox;
//  GtkWidget *vbox2;
//  GtkWidget *hbox2;
//  GtkWidget *table;
//  GtkWidget *entry;
//  GtkWidget *checkbox;
//  GtkWidget *scrolled_win;
//  GtkWidget *button;
//  GtkWidget *list;
//  GtkWidget *frame;
//
//  prop_dialog = structclass->properties_dialog;
//
//  /* Templates page: */
//  page_label = gtk_label_new_with_mnemonic (_("_Templates"));
//
//  vbox = gtk_vbox_new(FALSE, 5);
//  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
//
//  hbox2 = gtk_hbox_new(FALSE, 5);
//  checkbox = gtk_check_button_new_with_label(_("Template class"));
//  prop_dialog->templ_template = GTK_TOGGLE_BUTTON(checkbox);
//  gtk_box_pack_start (GTK_BOX (hbox2), checkbox, TRUE, TRUE, 0);
//  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, TRUE, 0);
//
//  hbox = gtk_hbox_new(FALSE, 5);
//
//  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
//  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
//				  GTK_POLICY_AUTOMATIC,
//				  GTK_POLICY_AUTOMATIC);
//  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
//  gtk_widget_show (scrolled_win);
//
//  list = gtk_list_new ();
//  prop_dialog->templates_list = GTK_LIST(list);
//  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
//  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
//  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
//				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
//  gtk_widget_show (list);
//
//  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
//		      GTK_SIGNAL_FUNC(templates_list_selection_changed_callback),
//		      structclass);
//
//  vbox2 = gtk_vbox_new(FALSE, 5);
//
//  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(templates_list_new_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(templates_list_delete_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//  button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(templates_list_move_up_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//  button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
//  gtk_signal_connect (GTK_OBJECT (button), "clicked",
//		      GTK_SIGNAL_FUNC(templates_list_move_down_callback),
//		      structclass);
//  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
//  gtk_widget_show (button);
//
//  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);
//
//  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
//
//  frame = gtk_frame_new(_("Formal parameter data"));
//  vbox2 = gtk_vbox_new(FALSE, 5);
//  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 10);
//  gtk_container_add (GTK_CONTAINER (frame), vbox2);
//  gtk_widget_show(frame);
//  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
//
//  table = gtk_table_new (2, 2, FALSE);
//  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
//
//  label = gtk_label_new(_("Name:"));
//  entry = gtk_entry_new();
//  prop_dialog->templ_name = GTK_ENTRY(entry);
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (templates_update_event), structclass);
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (templates_update), structclass);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  label = gtk_label_new(_("Type:"));
//  entry = gtk_entry_new();
//  prop_dialog->templ_type = GTK_ENTRY(entry);
//  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
//		      GTK_SIGNAL_FUNC (templates_update_event), structclass);
//  gtk_signal_connect (GTK_OBJECT (entry), "activate",
//		      GTK_SIGNAL_FUNC (templates_update), structclass);
//  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
//  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
//  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);
//
//  gtk_widget_show(vbox2);
//
//  /* TODO: Add stuff here! */
//
//  gtk_widget_show_all (vbox);
//  gtk_widget_show (page_label);
//  gtk_notebook_append_page (notebook, vbox, page_label);
//}
//
//
//
///******************************************************
// ******************** ALL *****************************
// ******************************************************/
//
//static void
//switch_page_callback(GtkNotebook *notebook,
//		     GtkNotebookPage *page)
//{
//  STRUCTClass *structclass;
//  STRUCTClassDialog *prop_dialog;
//
//  structclass = (STRUCTClass *)
//    gtk_object_get_user_data(GTK_OBJECT(notebook));
//
//  prop_dialog = structclass->properties_dialog;
//
//  if (prop_dialog != NULL) {
//    attributes_get_current_values(prop_dialog);
//    operations_get_current_values(prop_dialog);
//    templates_get_current_values(prop_dialog);
//  }
//}

static void
destroy_properties_dialog (GtkWidget* widget,
                           gpointer user_data)
{
    /* dialog gone, mark as such */
    STRUCTClass *structclass = (STRUCTClass *)user_data;

    g_free (structclass->properties_dialog);
    structclass->properties_dialog = NULL;
}


/* 2014-3-26 lcy 这里查找含有这个关键字的链表节点*/
gpointer factory_find_list_node(GList *list,gchar *data)
{
    GList *t = list;
    for(; t != NULL ; t = t->next)
    {
        FactoryStructEnum *p = t->data;
        if(!g_ascii_strcasecmp(data,p->key))
            return t->data;
    }
}

static void factory_connectionto_object(DDisplay *ddisp,DiaObject *obj,STRUCTClass *fclass,int num)
{
    Diagram *diagram;
    Handle *handle;
    diagram = ddisp->diagram;

    handle = obj->handles[num];
    handle->id = num == 0 ? HANDLE_MOVE_STARTPOINT : HANDLE_MOVE_ENDPOINT ;
    handle->connected_to = &fclass->connections[8];
    handle->pos = fclass->connections[8].pos;

    // diagram_select(diagram,obj);

    ConnectionPoint *connectionpoint = NULL;
    connectionpoint = object_find_connectpoint_display(ddisp, &fclass->connections[8].pos, obj, TRUE);
    if(connectionpoint != NULL)
    {
        handle->pos = connectionpoint->pos;
        if(connectionpoint->connected)
            connectionpoint->connected  = g_list_append(connectionpoint->connected,obj);
        else
        {
            GList *tmplist = NULL;
            tmplist=   g_list_append(tmplist,obj);
            connectionpoint->connected = tmplist;
        }
        obj->ops->connection_two_obj(obj,connectionpoint,num);
    }

    object_add_updates(obj, diagram);

}

DiaObject *factory_find_same_diaobject_via_glist(GList *flist,GList *comprelist)
{
    GList *samelist = NULL; // 找出相同的一个连接线对像
    DiaObject *obj = NULL;
    for(; flist; flist = flist->next)
    {
        samelist = g_list_find(comprelist,flist->data);
        if(samelist)
        {
            obj = samelist->data;
            break;
        }

    }
    return obj;
}



static void factory_connection_two_object(STRUCTClass *fclass, /* start pointer*/
        gchar *objname /* end pointer */)
{

    Layer *curlayer = fclass->element.object.parent_layer;
    STRUCTClass *objclass = factory_find_diaobject_by_name(curlayer,objname);
    if(!objclass)
        return;
    if(factory_is_connected(&objclass->connections[8],&fclass->connections[8])) /* 已经有连线了 */
        return;

    DDisplay *ddisp = ddisplay_active();
    /* ddisp->diagram->data->active_layer 与  fclass->element.object.parent_layer 是同一指针*/
    int x =0,y=0;
    x = fclass->connections[8].pos.x;
    y = fclass->connections[8].pos.y;
    DiaObject *obj = NULL;
    obj = ddisplay_drop_object(ddisp,x,y,object_get_type("Standard - Line"),6);
    /* 把线条移动到对像中心点且启始端固定在这一个对像上.*/
    //obj->ops->move(obj,&fclass->connections[8].pos);

    factory_connectionto_object(ddisp,obj,fclass,0);

    if(objclass)
    {
        factory_connectionto_object(ddisp,obj,objclass,1);
    }

    diagram_unselect_objects(ddisp->diagram,ddisp->diagram->data->selected);
    diagram_select(ddisp->diagram,fclass);
    diagram_flush(ddisp->diagram);

}

static void factory_read_props_from_widget(gpointer key,gpointer value ,gpointer user_data)
{
    SaveStruct *sss = (SaveStruct *)value;
    switch(sss->celltype)
    {
    case ECOMBO:
        if(gtk_container_child_type(GTK_CONTAINER(sss->widget2)) != GTK_TYPE_NONE)
        {
            GList *p = gtk_container_children(GTK_CONTAINER(sss->widget2));
            if(p)
                sss->value.senum.index = gtk_combo_box_get_active(GTK_COMBO_BOX(p->data));
        }
        else
            sss->value.senum.index = gtk_combo_box_get_active(GTK_COMBO_BOX(sss->widget2));
        FactoryStructEnum *kmap =  g_list_nth_data(sss->value.senum.enumList,sss->value.senum.index);
        sss->value.senum.evalue = kmap->value;
        break;
    case ENTRY:
    {
        SaveEntry *sey = &sss->value.sentry;
//        gdouble maxlength = sey->col * sey->row; // 得到文本框的大小。
        if(sey->isString)
        {
            sey->data =  g_locale_to_utf8(gtk_entry_get_text(GTK_ENTRY (sss->widget2)),-1,NULL,NULL,NULL);
        }
    }

    break;
    case SPINBOX:
    {
        int v = gtk_spin_button_get_value(GTK_SPIN_BUTTON(sss->widget2));
        sss->value.vnumber = g_strdup_printf("%d",v);
    }

    break;
    case OCOMBO:
    {
        NextID *nid = &sss->value.nextid;
        if( g_list_length(nid->actlist)>0)
        {
            ActionID *aid = nid->actlist->data;
            factory_get_value_from_comobox(sss->sclass,sss->widget2,aid);
        }
        else
        {
            /* error */
        }

        /* 2014-4-3 lcy  在指定位置创建一条线的标准控件, 线条是标准控件,这里调用drop 回调函数 */
    }
    break;
    case UCOMBO:
    {
        SaveUnion *suptr = &sss->value.sunion;
        if(!sss->isPointer)
        {
            SaveStruct *tsst  =  g_hash_table_lookup(suptr->saveVal,suptr->curkey);
            if(tsst)
            {
                factory_save_value_from_widget(tsst);
            }
        }
    }
    break;
    default:
        break;
    }

}

static void factory_get_value_from_comobox(STRUCTClass *startclass,GtkWidget *comobox,ActionID *aid)
{
     DDisplay *ddisp =  ddisplay_active();
    int  curindex = gtk_combo_box_get_active(GTK_COMBO_BOX(comobox));
    gchar *pname = gtk_combo_box_get_active_text(GTK_COMBO_BOX(comobox));

    if(!curindex)
        goto DELL;
    else
    {
        Layer *p = startclass->element.object.parent_layer;
        STRUCTClass *objclass =NULL;
        if(!g_ascii_strcasecmp(pname,startclass->name)) /* 是自己就不连线,如果上次有连线现在要删除 */
        {
            objclass = factory_find_diaobject_by_name(p,aid->pre_name);
            if(objclass && factory_is_connected(&objclass->connections[8],&startclass->connections[8]))
             {
                 factory_delete_line_between_two_objects(startclass,aid->pre_name);

             }
            goto SETV ;
        }



        objclass = factory_find_diaobject_by_name(p,pname);
        if(objclass && factory_is_connected(&objclass->connections[8],&startclass->connections[8]))
            goto SETV; /* 已经有连线了 */
        else
        {
            if(objclass && objclass->widgetSave)
            {
                SaveStruct *sst =objclass->widgetSave->data; /* 对像的自身ID */
                aid->value =   g_strdup(sst->value.vnumber);
            }
            else
            {
                aid->value = g_strdup("-1");
            }

            factory_connection_two_object(startclass,g_strdup(pname));
        }

    }

DELL:
    if(aid->index && aid->index != curindex &&
            g_ascii_strcasecmp(aid->pre_name,startclass->name)) /* 上次非零,名字又不是自己,要删除线*/
    {
        factory_delete_line_between_two_objects(startclass,aid->pre_name);
    }

SETV:
    aid->pre_name = g_strdup(pname);
    aid->index  = curindex;
    diagram_flush(ddisp->diagram);
    g_free(pname);
}


void factory_delete_line_between_two_objects(STRUCTClass *startc,const gchar *endc)
{
    DDisplay *ddisp =  ddisplay_active();
//    Layer *curlayer = startc->element.object.parent_layer;
    STRUCTClass *objclass = factory_find_diaobject_by_name(curLayer,endc);
    if(objclass)
    {
        ConnectionPoint *cpstart = &startc->connections[8];
        ConnectionPoint *cpend = &objclass->connections[8];
        DiaObject *line =  factory_find_same_diaobject_via_glist(cpend->connected,cpstart->connected);
        if(line && factory_is_connected(cpend,cpstart))
        {
            /* 上次的连线,到此要删掉它,重新连线 */
//            diagram_select(ddisp->diagram,line);
            cpstart->connected  = g_list_remove(cpstart->connected,line); /* 删掉对方链表里的对像.*/
            cpend->connected = g_list_remove(cpend->connected,line);
            g_list_free1(line->connections[0]->connected);
            line->connections[0]->connected = NULL;
            line->connections[0]->object = NULL;

//            line->ops->move(line,&cpstart->pos);
            layer_remove_object(curLayer,line); // 在当前的画布里面删除连线对像.
             line->ops->destroy(line);
            diagram_flush(ddisp->diagram);
//            line->ops->destroy(line);
        }
        diagram_select(ddisp->diagram,objclass);
        diagram_unselect_objects(ddisp->diagram,ddisp->diagram->data->selected);
        diagram_select(ddisp->diagram,startc);
    }
}


/* 2014-3-25 lcy 这里是更新界面上的值*/
ObjectChange *
factory_apply_props_from_dialog(STRUCTClass *fclass, GtkWidget *widget)
{
//    g_hash_table_foreach(fclass->widgetmap,factory_read_props_from_widget,(gpointer)fclass);
    GList *applist = NULL;

    applist = fclass->widgetSave;
    for(; applist; applist = applist->next)
    {
        factory_read_props_from_widget(NULL,applist->data,NULL);
    }

    PublicSection *pps = fclass->pps;
    if(!pps)
        return NULL;
    pps->hasfinished = gtk_toggle_tool_button_get_active(GTK_TOGGLE_BUTTON(pps->wid_hasfinished));
    gchar *txt = gtk_entry_get_text(GTK_ENTRY(pps->wid_rename_entry));
    if(g_utf8_validate(txt,-1,NULL))
        pps->name = g_strdup(txt);
    else
        pps->name = factory_utf8(txt);
    if(pps->name != NULL && g_ascii_strcasecmp(pps->name,fclass->name))
    {
        GList *exists = NULL;
        if(curLayer)
            exists = curLayer->objects;
        for(; exists; exists = exists->next)
        {
            STRUCTClass *pclass = exists->data;
            if(!factory_is_valid_type(pclass))
                continue;
            if(!g_ascii_strcasecmp(pclass->name,pps->name))
            {
                gchar *msg_err = g_locale_to_utf8(_("找到时有相同名字的控件,请更换名字!\n"),-1,NULL,NULL,NULL);
                message_error(msg_err);
                g_free(msg_err);
                break;
            }
        }

        if(!exists && curLayer)
        {
            fclass->name = g_strdup(pps->name);
            GList *defnames = NULL;
            GList *objlist = NULL;

            curLayer->defnames;
            objlist = curLayer->objects;
            g_list_free1(curLayer->defnames); /* 所有对像的名字链表也要更新 */
            curLayer->defnames = NULL;

            for(; objlist; objlist = objlist->next)
            {
                if(factory_is_valid_type(objlist->data))
                {
                    STRUCTClass *oclass = objlist->data;
                    if(!factory_is_system_data(oclass->element.object.name))
                        curLayer->defnames = g_list_append(curLayer->defnames,oclass->name);
                }
            }

        }
        structclass_calculate_data(fclass);
        structclass_update_data(fclass);

        // g_convert("中国",-1,"UTF-8","GB2312",NULL,NULL,NULL);
    }
    pps->hasfinished = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pps->wid_hasfinished));
    if(pps->hasfinished)
    {
        Color red = {200,0,0};
        fclass->fill_color = red;
    }
    else
    {
        Color w = {255,255,255};
        fclass->fill_color = w;
    }



    return  NULL;
}


//ObjectChange *
//structclass_apply_props_from_dialog(STRUCTClass *structclass, GtkWidget *widget)
//{
//  STRUCTClassDialog *prop_dialog;
//  DiaObject *obj;
//  GList *list;
//  int num_attrib, num_ops;
//  GList *added, *deleted, *disconnected;
//  STRUCTClassState *old_state = NULL;
//
//#ifdef DEBUG
//  structclass_sanity_check(structclass, "Apply from dialog start");
//#endif
//
//  prop_dialog = structclass->properties_dialog;
//
//  old_state = structclass_get_state(structclass);
//
//  /* Allocate enought connection points for attributes and operations. */
//  /* (two per op/attr) */
//  if ( (prop_dialog->attr_vis->active) && (!prop_dialog->attr_supp->active))
//    num_attrib = g_list_length(prop_dialog->attributes_list->children);
//  else
//    num_attrib = 0;
//  if ( (prop_dialog->op_vis->active) && (!prop_dialog->op_supp->active))
//    num_ops = g_list_length(prop_dialog->operations_list->children);
//  else
//    num_ops = 0;
//  obj = &structclass->element.object;
//#ifdef STRUCT_MAINPOINT
//  obj->num_connections =
//    STRUCTCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2 + 1;
//#else
//  obj->num_connections =
//    STRUCTCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2;
//#endif
//  obj->connections =
//    g_realloc(obj->connections,
//	      obj->num_connections*sizeof(ConnectionPoint *));
//
//  /* Read from dialog and put in object: */
//  class_read_from_dialog(structclass, prop_dialog);
//  attributes_read_from_dialog(structclass, prop_dialog, STRUCTCLASS_CONNECTIONPOINTS);
//  /* ^^^ attribs must be called before ops, to get the right order of the
//     connectionpoints. */
//  operations_read_from_dialog(structclass, prop_dialog, STRUCTCLASS_CONNECTIONPOINTS+num_attrib*2);
//  templates_read_from_dialog(structclass, prop_dialog);
//
//  /* Reestablish mainpoint */
//#ifdef STRUCT_MAINPOINT
//  obj->connections[obj->num_connections-1] =
//    &structclass->connections[STRUCTCLASS_CONNECTIONPOINTS];
//#endif
//
//  /* unconnect from all deleted connectionpoints. */
//  list = prop_dialog->deleted_connections;
//  while (list != NULL) {
//    ConnectionPoint *connection = (ConnectionPoint *) list->data;
//
//    structclass_store_disconnects(prop_dialog, connection);
//    object_remove_connections_to(connection);
//
//    list = g_list_next(list);
//  }
//
//  deleted = prop_dialog->deleted_connections;
//  prop_dialog->deleted_connections = NULL;
//
//  added = prop_dialog->added_connections;
//  prop_dialog->added_connections = NULL;
//
//  disconnected = prop_dialog->disconnected_connections;
//  prop_dialog->disconnected_connections = NULL;
//
//  /* Update data: */
//  structclass_calculate_data(structclass);
//  structclass_update_data(structclass);
//
//  /* Fill in class with the new data: */
//  fill_in_dialog(structclass);
//#ifdef DEBUG
//  structclass_sanity_check(structclass, "Apply from dialog end");
//#endif
//  return  new_structclass_change(structclass, old_state, added, deleted, disconnected);
//}



GtkWidget *
factory_get_properties(STRUCTClass *fclass, gboolean is_default)
{

    STRUCTClassDialog *prop_dialog;
    GtkWidget *vbox;
//    GtkTable *mainTable;

    if (fclass->properties_dialog == NULL)
    {
        prop_dialog = g_new(STRUCTClassDialog, 1);
        fclass->properties_dialog = prop_dialog;
    }
    GtkWindow *window=  gtk_widget_get_toplevel(fclass->properties_dialog);
    gtk_window_set_modal(window,TRUE);
    vbox = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = vbox;

    gtk_widget_set_name(vbox,fclass->name); // 2014-3-21 lcy 设置名字，用于区分不同窗体。
    gtk_signal_connect (GTK_OBJECT (prop_dialog->dialog),"destroy",
                        GTK_SIGNAL_FUNC(destroy_properties_dialog),(gpointer) fclass);
//  int mapsize =   g_hash_table_size(class->widgetmap);
    factory_create_and_fill_dialog(fclass,FALSE); /* 准备显示UI控件 */

    gtk_widget_show_all (fclass->properties_dialog->dialog);
    return fclass->properties_dialog->dialog;
}

GtkWidget *factory_create_combo_widget(GList *datalist,gint activeid)
{
    GtkWidget *widget;
    widget = gtk_combo_box_new_text();
    gtk_combo_box_popdown(GTK_COMBO_BOX(widget));
    GList *pp = datalist;
    for(; pp ; pp = pp->next)
    {
        gtk_combo_box_append_text(GTK_COMBO_BOX(widget),pp->data);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget),activeid);
    return  widget;
}

GtkWidget *factory_create_text_widget(gchar *str,gint maxlength)
{
    GtkWidget *widget;
    widget = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(widget), maxlength);
    gtk_entry_set_text(GTK_ENTRY(widget),str);
    return widget;
}

GtkWidget *factory_create_spbinbox_widget(gint value,gint minvalue,gint maxvalue)
{
    GtkWidget *widget;
    widget = GTK_SPIN_BUTTON(gtk_spin_button_new_with_range(minvalue,maxvalue,1));

    gtk_spin_button_set_numeric( GTK_SPIN_BUTTON( widget), TRUE);
    gtk_spin_button_set_snap_to_ticks( GTK_SPIN_BUTTON(widget), TRUE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(widget),value);
    return widget;
}


GtkWidget *factory_create_enum_widget(GList *list,int index)
{
    GtkWidget *columTwo = NULL;
    GList *datalist = NULL;
    GList *p = list;
    for(; p != NULL ; p= p->next)
    {
        FactoryStructEnum *kvmap = p->data;
        datalist = g_list_append(datalist,kvmap->key);
    }
    columTwo =  factory_create_combo_widget(datalist,index);

    return columTwo;
}

static GtkWidget *factory_create_variant_object(SaveStruct *sss)
{
    /* 这个函数可能会被递归调用 */
    FactoryStructItem *item = sss->org;
    GtkWidget *columTwo = NULL;

    switch(sss->celltype)
    {
    case ENTRY:
    {
        SaveEntry *sey = &sss->value.sentry;
//        gdouble maxlength = sey->col * sey->row; // 得到文本框的大小。
        if(sey->isString)
        {
            columTwo =  sey->data ? factory_create_text_widget(sey->data,sey->arr_base->reallen):
                        factory_create_text_widget(item->Value,sey->arr_base->reallen);
        }

    }
    break;
    case SPINBOX:
    {

        gint v = 0;
        if(!g_ascii_strncasecmp(sss->value.vnumber,"-1",2))
            v = -1;
        else
            v = g_strtod(sss->value.vnumber,NULL);

        gint vmin =  g_strtod(item->Min,NULL);
        gint vmax = g_strtod(item->Max,NULL);
        columTwo = factory_create_spbinbox_widget(v,vmin,vmax);


    }
    break;
    case UCOMBO:
    {
        SaveUnion *suptr = &sss->value.sunion;
        columTwo = gtk_vbox_new(FALSE,1) ; /*  这里插入一个VBOX*/
        suptr->comobox = gtk_combo_box_new_text(); /* 显示联合体成员的控件 */
        suptr->vbox = columTwo;

        gtk_combo_box_popdown(GTK_COMBO_BOX(suptr->comobox));
        GList *p = suptr->structlist;
        /* nextobj 就是当前下拉框所显示的 */
        FactoryStructItem *nextobj =  g_list_nth_data(suptr->structlist,suptr->index);
        if(!nextobj)
            break;

        for(; p != NULL ; p= p->next)
        {
            FactoryStructItem *data = p->data; /* 添加联合体里的成员到下拉列表 */
            gtk_combo_box_append_text(GTK_COMBO_BOX(suptr->comobox),data->Name);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(suptr->comobox),suptr->index);
        gtk_box_pack_start_defaults(GTK_BOX(columTwo),suptr->comobox);
        SaveStruct *tsst = NULL;
        suptr->curkey = g_strjoin("##",nextobj->FType,nextobj->Name,NULL);
        suptr->curtext = g_strdup(nextobj->Name);
        if(g_hash_table_size(suptr->saveVal))
        {
            tsst = g_hash_table_lookup(suptr->saveVal,suptr->curkey);
            if(!tsst)
            {
                goto FIRST;
            }
        }
        else
        {
            /*第一次运行*/
FIRST:
            tsst = factory_get_savestruct(nextobj);
            tsst->sclass = sss->sclass;
            factory_strjoin(&tsst->name,sss->name,".");
            /* 把当前选择的成员初始化保存到哈希表 */
            g_hash_table_insert(suptr->saveVal,suptr->curkey,tsst);

        }

        GtkWidget *widget = factory_create_variant_object(tsst); /* 递归调用本函数 */
        if(tsst->isPointer )/* 这个控件是有可能是数组,结构体,所以这里显示所按键 */
        {
            /* 初始化下面这一个有可能按键出来控件要保存的值 */
            SaveUbtn *sbtn = &tsst->value.ssubtn;

            if(/*!g_hash_table_size(sbtn->htoflist)*/ !g_list_length(sbtn->savelist) && sbtn->structlist)
            {
//                sbtn->htoflist = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
                GList *slist = sbtn->structlist;
                for(; slist; slist = slist->next)
                {
                    FactoryStructItem *o = slist->data;
                    SaveStruct *s  = factory_get_savestruct(o);
                    s->sclass = sss->sclass;
                    factory_strjoin(&s->name,sss->name,".");
                    sbtn->savelist = g_list_append(sbtn->savelist,s);
                }
            }
        }
//                           suptr->widgetlist = g_slist_append(suptr->widgetlist,widget);
        gtk_box_pack_start_defaults(GTK_BOX(columTwo),widget);
        gtk_widget_show_all(columTwo);
        g_signal_connect (G_OBJECT (suptr->comobox), "changed",G_CALLBACK (factoy_changed_item), sss);
    }
    break;
    case ECOMBO:
        columTwo =  factory_create_enum_widget(item->datalist,sss->value.senum.index);
        break;
    case OCOMBO:
    {
        STRUCTClass *orgclass = sss->sclass;
        NextID *nextid = &sss->value.nextid;
        columTwo = gtk_combo_box_new_text();
        gtk_combo_box_popdown(GTK_COMBO_BOX(columTwo));
//        Layer *curlayer = orgclass->element.object.parent_layer;
//        GList *objlist = curlayer->objects;
        g_list_free(nextid->itemlist);
        nextid->itemlist = NULL;

        nextid->itemlist = factory_get_objects_from_layer(curLayer);
        nextid->itemlist = g_list_insert(nextid->itemlist,g_strdup(""),0);
        ActionID *aid = nextid->actlist->data;

        GList *tlist = nextid->itemlist;
        columTwo = factory_create_combo_widget(tlist,aid->index);
    }
    break;
    case OBTN:
    case EBTN:
    case UBTN:
    case BBTN:
    {
        columTwo =gtk_button_new_with_label(item->Name);
        FactoryStructItem *fsi = sss->org;
        if(factory_is_special_object(item->SType))
        {
            SaveKV *skv = sss->value.vnumber;
            gtk_button_set_label(GTK_BUTTON(columTwo), skv->value );
            g_signal_connect (G_OBJECT (columTwo), "clicked",G_CALLBACK (sss->newdlg_func),sss);
        }
        else
        {
            g_signal_connect (G_OBJECT (columTwo), "clicked",G_CALLBACK (sss->newdlg_func), sss);
        }



    }
    break;
    default:
        break;
    }
    sss->widget2 = columTwo;
    return columTwo;
}


gboolean factory_music_fm_get_type(const gchar* name)
{
    gboolean flag = FALSE;
    if(!g_ascii_strncasecmp(name,"aIndex_Number",strlen("aIndex_Number")))
    {
        MusicManagerDialog->fmst = SEQUENCE;
        flag = TRUE;
    }

    else if(!g_ascii_strncasecmp(name,"aFile_Number",strlen("aFile_Number")))
    {
        MusicManagerDialog->fmst = INDEX;
        flag = TRUE;
    }

    else if(!g_ascii_strncasecmp(name,"aPhy_Number",strlen("aPhy_Number")))
    {
        MusicManagerDialog->fmst = PHY;
        flag = TRUE;
    }
    return flag;
}



gboolean factory_is_valid_type(gpointer data)
{
    DiaObject *otype = (DiaObject *)data;
    char *n1 = g_strdup("STRUCT - Class");
    gboolean ftype = g_ascii_strncasecmp(otype->type->name,n1,strlen(n1));
    g_free(n1);
    return !ftype;
}
void factory_set_savestruct_widgets(SaveStruct *sss)
{
    GtkWidget *Name ;
    GtkWidget *columTwo;
    GtkTooltips *tool_tips = gtk_tooltips_new();

    /* 2014-3-26 lcy 用两个区间做键值。*/
    if(sss && sss->org )
    {
        gchar *comment = sss->org->Comment;
        Name = gtk_label_new(sss->org->Cname);
        gtk_tooltips_set_tip(tool_tips,Name,g_strdup(comment),NULL);
        /* 创建多样性的对像控件  */

        columTwo = factory_create_variant_object(sss);
        gtk_widget_set_sensitive(columTwo,sss->isSensitive);
        gtk_tooltips_set_tip(tool_tips,columTwo,g_strdup(comment),NULL);
//        sss->widget2 = columTwo;
        sss->widget1 = Name;
    }

}


static FactoryStructItem * factory_find_a_struct_item(GList *itemlist,const gchar *text)
{
    FactoryStructItem *fsi = NULL;
    GList *ulist = itemlist;
    for(; ulist; ulist = ulist->next)
    {
        fsi = ulist->data;
        if(!g_ascii_strcasecmp(fsi->Name,text))
        {
            break;
        }
    }
    return fsi;
}

void factory_save_enumbutton_dialog(GtkWidget *widget,gint response_id,gpointer user_data)
{
    SaveStruct *sst = (SaveStruct *)user_data;
    SaveEbtn *sebtn = &sst->value.ssebtn;
    GList *wlist = sebtn->ebtnwlist;
    if(response_id == GTK_RESPONSE_APPLY
            || response_id == GTK_RESPONSE_OK)
    {
        int n  = 0;
        gchar *key = NULL;
        for(; wlist; wlist = wlist->next)
        {
            SaveEnumArr *sea = wlist->data;
            SaveEnum *se = sea->senum;
            se->index =   gtk_combo_box_get_active(GTK_COMBO_BOX(sea->widget2));
            key = gtk_combo_box_get_active_text(GTK_COMBO_BOX(sea->widget2));
            GList *tlist = sebtn->ebtnslist;
            for(; tlist ; tlist = tlist->next)
            {
                FactoryStructEnum *fse = tlist->data;
                if(!g_ascii_strcasecmp(fse->key,key))
                {
                    se->evalue = fse->value;
                    break;
                }
            }
        }
    }
    gtk_widget_hide(widget);
//    if(widget)
//        gtk_widget_destroy(widget);
}

void factory_save_objectbutton_dialog(GtkWidget *widget,
                                      gint       response_id,
                                      gpointer   user_data)
{
    SaveStruct *sst = (SaveStruct *)user_data;
    NextID *nid = &sst->value.nextid;
    GList *wlist = nid->wlist;
    if (   response_id == GTK_RESPONSE_APPLY
            || response_id == GTK_RESPONSE_OK)
    {

        int n = 0;
        for(; wlist; wlist = wlist->next,n++)
        {
            GtkWidget *wid = wlist->data;
            ActionID *act = g_list_nth_data(nid->actlist,n);
            factory_get_value_from_comobox(sst->sclass,wid,act);
        }
    }
    gtk_widget_hide(widget);
//    gtk_widget_destroy(widget);

    wlist = nid->wlist;
    for(; wlist; wlist = wlist->next)
    {
        gtk_container_remove(GTK_CONTAINER(widget),wlist->data);
    }
    g_list_free(nid->wlist);
    nid->wlist = NULL;
}

static void factory_draw_many_lines_dialog(GtkWidget *widget,
        gint       response_id,
        gpointer   user_data)
{
//    SaveStruct *sst = (SaveStruct *)user_data;
//    NextID *aid = &sst->value.nextid;
//    GList *wlist = aid->wlist;
//    if (   response_id == GTK_RESPONSE_APPLY
//            || response_id == GTK_RESPONSE_OK)
//    {
//        /* 保存整个结构 */
//
//        GList *newvlist = NULL;
//        for(; wlist; wlist = wlist->next)
//        {
//            GtkWidget *chkbtn = wlist->data; /* 检查那些对像被选上了,逐个保存 */
//            gboolean curbool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkbtn));
//            gchar *objname = gtk_button_get_label(GTK_BUTTON(chkbtn));
//            if(!g_ascii_strncasecmp(sst->sclass->name,objname,strlen(objname)))
//                continue; /* 这里显示如果本对像没有选上,就跳过,如果按下面流程就会删除一条连线,调试很难发现 */
//            /* 画连接线 */
//
//            curbool ? factory_connection_two_object(sst->sclass,objname) :
//            /* 删除连接线 */
//            factory_delete_line_between_two_objects(sst->sclass,objname);
//
//            g_free(objname);
//        }
//    }
//    gtk_widget_hide_all(widget);
//
//    wlist = aid->wlist;
//    for(; wlist; wlist = wlist->next)
//    {
//        gtk_container_remove(GTK_CONTAINER(widget),wlist->data);
//
//    }
//    g_list_free(aid->wlist);
//    aid->wlist = NULL;
}


void factory_save_unionbutton_dialog(GtkWidget *widget,
                                     gint       response_id,
                                     gpointer   user_data)
{
    /* 联合体按钮 */


    if (   response_id == GTK_RESPONSE_APPLY
            || response_id == GTK_RESPONSE_OK)
    {
        /* 保存整个结构 */
        SaveStruct *sst = (SaveStruct *)user_data;
//        SaveUnion *suptr = &sst->value.sunion;
        SaveUbtn *sbtn = &sst->value.ssubtn;


//        GList *wlist = g_hash_table_get_values(sbtn->htoflist);
        GList *wlist = sbtn->savelist;
        for(; wlist; wlist = wlist->next)
        {
            SaveStruct *vss = wlist->data;
            if(vss)
                factory_save_value_from_widget(vss);
        }
    }
    gtk_widget_hide(widget);
    gtk_widget_destroy(widget);
//    GList *wlist = gtk_container_children(GTK_CONTAINER(widget));
//    for(; wlist; wlist = wlist->next)
//    {
//        gtk_container_remove(widget,wlist->data);
//    }

}

void factory_save_value_from_widget(SaveStruct *sss)
{

    switch(sss->celltype)
    {
    case ECOMBO:
    {
        sss->value.senum.index = gtk_combo_box_get_active(GTK_COMBO_BOX(sss->widget2));
        FactoryStructEnum *kmap =  g_list_nth_data(sss->value.senum.enumList,sss->value.senum.index);
        if(kmap)
            sss->value.senum.evalue = kmap->value;
    }
    break;
    case ENTRY:
    {
        SaveEntry *sey = &sss->value.sentry;
//        gdouble maxlength = sey->col * sey->row; // 得到文本框的大小。
        if(sey->isString)
        {
            sey->data =  g_locale_to_utf8(gtk_entry_get_text(GTK_ENTRY (sss->widget2)),-1,NULL,NULL,NULL);
        }
    }
    break;
    case SPINBOX:
    {
        guint v = gtk_spin_button_get_value(GTK_SPIN_BUTTON(sss->widget2));
        sss->value.vnumber = g_strdup_printf("%d",v);
    }

    break;
    case UCOMBO:
    {
        SaveUnion *ssu = &sss->value.sunion;
        SaveStruct *p  = g_hash_table_lookup(ssu->saveVal,ssu->curkey);
        if(p)
        {
            switch(p->celltype)
            {
            case SPINBOX:
            case ENTRY:
            case ECOMBO:
                factory_save_value_from_widget(p);
            }
        }

    }
    break;
    default:
        break;
    }
}


void
factory_save_basebutton_dialog(GtkWidget *widget,
                               gint       response_id,
                               gpointer   data)
{
    GList *tmp;
    SaveEntry *sey = data;
    /* 2013-3-31 lcy 这里把每一个控件值保存到链表里*/
    if (   response_id == GTK_RESPONSE_APPLY
            || response_id == GTK_RESPONSE_OK)
    {

        int align = 2+sey->width*2;
        tmp = sey->wlist;
        g_list_free1(sey->data);
        sey->data = NULL;
        for( ; tmp; tmp = tmp->next)
        {
            gchar *str = g_locale_to_utf8(gtk_entry_get_text(tmp->data),-1,NULL,NULL,NULL);
            int n = align - strlen(str);
            if(n)
            {
                /* 0xf4 == 0x00f4  */
                gchar *p =  g_new(char,align);
                memset(p,'\0',align);
                p = strncpy(p,"0x",2);
                while(n--)
                {
                    strncat(p,"0",1);
                }
                strncat(p,&str[2],strlen(&str[2]));
                g_free(str);
                str = p;
            }
            sey->data =  g_list_append(sey->data,str);
        }

    }

    gtk_widget_hide(widget);
    tmp = sey->wlist;
    for(; tmp; tmp = tmp->next)
        gtk_container_remove(GTK_CONTAINER(widget),tmp->data);

//    return response_id;
}


static gint
factory_subdig_checkbtns_respond(GtkWidget *widget,gint       response_id,gpointer   data)
{
    GList *wlist = NULL;
    SaveEntry *sey = data;
    /* 2013-3-31 lcy 这里把每一个控件值保存到链表里*/
    if (   response_id == GTK_RESPONSE_APPLY
            || response_id == GTK_RESPONSE_OK)
    {
        wlist = sey->wlist;
        g_list_free1(sey->data);
        sey->data = NULL;
        for( ; wlist; wlist = wlist->next)
        {
            int v  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wlist->data)) ? 1 : 0;
            sey->data =  g_list_append(sey->data,g_strdup_printf("%d",v));
//            gtk_container_remove(GTK_CONTAINER(widget),wlist->data);
        }
    }

    gtk_widget_hide(widget);
//   gtk_widget_destroy(widget);
    return response_id;
}


void factoy_changed_item(gpointer widget,gpointer user_data)
{
    /* 这里是一个VBOX */
//    GtkWidget *parent = gtk_widget_get_parent (widget);
    SaveStruct *sss  = user_data;
    SaveUnion *suptr = &sss->value.sunion;

    GtkWidget *activeWidget = NULL;

    /* 删除原来的控件,根据类型重新生成控件*/
    GList *wlist = gtk_container_children(GTK_CONTAINER(suptr->vbox));
    for(; wlist; wlist = wlist->next)
    {
        GtkWidget *rwidget = wlist->data;
        if(rwidget != suptr->comobox)
        {
            gtk_container_remove(GTK_CONTAINER(suptr->vbox),rwidget);
            gtk_widget_destroy (rwidget);
        }
    }


    gchar *text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
    suptr->curtext = g_strdup(text);
    g_free(text);
    suptr->index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    // GList *t = sss->value.senum.enumList;


    FactoryStructItem *sfst= factory_find_a_struct_item(suptr->structlist,suptr->curtext);
    if(sfst)
    {
        SaveStruct *existS = NULL;
        suptr->curkey = g_strjoin("##",sfst->FType,sfst->Name,NULL);
        if(g_hash_table_size(suptr->saveVal))
        {
            existS = g_hash_table_lookup(suptr->saveVal,suptr->curkey);
            if(!existS)
            {
                //                                  existS = factory_get_savestruct(sfst);
                //                                  factory_strjoin(existS->name,sss->name,".");
                //                                  g_hash_table_insert(suptr->saveVal,suptr->curkey,existS);
                goto CFIRST;
            }

        }
        else
        {
            /*第一次运行*/
CFIRST:
            existS = factory_get_savestruct(sfst);
            existS->sclass = sss->sclass;
            factory_strjoin(&existS->name,sss->name,".");
            g_hash_table_insert(suptr->saveVal,suptr->curkey,existS);

        }
        activeWidget = factory_create_variant_object(existS);
        if(existS->isPointer )
        {
            /* 初始化要保存的值 */
            SaveUbtn *sbtn = &existS->value.ssubtn;
            g_list_free1(sbtn->savelist);
            sbtn->savelist = NULL;
            if(sbtn->structlist)
            {
                GList *slist = sbtn->structlist;
                for(; slist; slist = slist->next)
                {
                    FactoryStructItem *o = slist->data;
                    SaveStruct *s  = factory_get_savestruct(o);
                    s->sclass = sss->sclass;
                    factory_strjoin(&s->name,sss->name,".");
                    sbtn->savelist = g_list_append(sbtn->savelist,s);
                }
            }
        }
//            suptr->widgetlist = g_list_append(suptr->widgetlist,activeWidget);
        gtk_box_pack_start_defaults(GTK_BOX(suptr->vbox),activeWidget);
        gtk_container_resize_children (GTK_CONTAINER(suptr->vbox));
    }

    gtk_widget_show_all(suptr->vbox);

}

void factory_strjoin(gchar **dst,const gchar *prefix,const gchar *sep)
{
    gchar *oname = g_strdup(*dst);
    g_free(*dst);
    *dst = g_strjoin(sep,prefix,oname,NULL);
    g_free(oname);
}



/* 这里数组显示  */
void factory_create_basebutton_dialog(GtkWidget *button,SaveStruct *sss)
{

    GtkWidget *sdialog = gtk_hbox_new(FALSE,0);
    GtkWidget *subdig = factory_create_new_dialog_with_buttons(sss->name,gtk_widget_get_toplevel(button));
    GtkWidget *dialog_vbox = GTK_DIALOG(subdig)->vbox;
    gtk_container_add(GTK_CONTAINER(dialog_vbox),sdialog);
    gtk_window_set_modal(GTK_WINDOW(subdig),TRUE);


    /* 行列式的控件输入框 */
//    GtkWidget *sdialog = gtk_vbox_new(FALSE,0);
    gtk_dialog_set_default_response (GTK_DIALOG(subdig), GTK_RESPONSE_OK);

//    gtk_container_add(GTK_CONTAINER(dialog_vbox),sdialog);

//    g_signal_connect(G_OBJECT (subdig), "destroy",G_CALLBACK(gtk_widget_destroyed), &subdig);
//    g_signal_connect(G_OBJECT (sdialog), "destroy",G_CALLBACK(gtk_widget_destroyed), &dialog_vbox);

    gtk_window_set_resizable (GTK_WINDOW(subdig),FALSE);
    gtk_window_set_position (GTK_WINDOW(subdig),GTK_WIN_POS_MOUSE);
    gtk_window_present(GTK_WINDOW(subdig));
    gchar **vvv = g_strsplit(sss->type,".",-1);
    int len = g_strv_length(vvv) -1 ;
    SaveEntry *sey = &sss->value.sentry;

//    if(2 == strlen(vvv[len]) && !g_ascii_strncasecmp(vvv[len],"u1",2))
//    {
//
//        gtk_box_pack_start(GTK_BOX(dialog_vbox),factory_create_many_checkbox(sss),FALSE,FALSE,0);
//        g_signal_connect(G_OBJECT(subdig), "response",G_CALLBACK (factory_subdig_checkbtns_respond),sey);
//    }
//    else
    {
        gtk_box_pack_start(GTK_BOX(sdialog),factory_create_many_entry_box(sss),FALSE,FALSE,0);
        g_signal_connect(G_OBJECT(subdig), "response",G_CALLBACK(sss->close_func),sey);
    }
    g_strfreev(vvv);
    gtk_widget_show_all(subdig);
}




void factory_create_enumbutton_dialog(GtkWidget *button,SaveStruct *sst)
{
    GtkWidget *sdialog = gtk_hbox_new(FALSE,0);
    GtkWidget *newdialog = factory_create_new_dialog_with_buttons(sst->name,gtk_widget_get_toplevel(button));
    GtkWidget *dialog_vbox = GTK_DIALOG(newdialog)->vbox;
    gtk_container_add(GTK_CONTAINER(dialog_vbox),sdialog);
    gtk_window_set_modal(GTK_WINDOW(newdialog),TRUE);
    SaveEbtn *sebtn  = &sst->value.ssebtn;
//    GList *nixlist = sebtn->ebtnwlist;
//    int n = 0;
    gchar **title = g_strsplit(sst->name,"[",-1);
    gchar *fmt =  g_strconcat(title[0],"(%d)",NULL);
    g_strfreev(title);

    int r = 0;
    int pos = 0;
    int tnum = 0;
    ArrayBaseProp *abp = sebtn->arr_base;

    for(; r < abp->row ; r++)
    {
        GtkWidget *vbox = gtk_vbox_new(TRUE,0);
        int c = 0;
        tnum = r*abp->col;
        for(; c < abp->col ; c++)
        {
            pos = tnum +c;
            if( pos >= abp->reallen)
                break;
            GtkWidget *hbox = abp->row > 8 ? gtk_vbox_new(FALSE,0): gtk_hbox_new(FALSE,0) ;
            SaveEnumArr *sea = g_list_nth_data(sebtn->ebtnwlist,pos);
            GtkWidget *comobox = factory_create_combo_widget(sea->senum->enumList,sea->senum->index);
            GtkWidget *wid = gtk_label_new(g_strdup_printf(fmt,pos));
            sea->widget1 = wid;
            sea->widget2 = comobox;
            gtk_box_pack_start(GTK_BOX(hbox),wid,FALSE,FALSE,0);
            gtk_box_pack_start(GTK_BOX(hbox),comobox,FALSE,FALSE,0);
            gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
            gtk_container_add(GTK_CONTAINER(vbox),hbox);
        }
        gtk_container_add(GTK_CONTAINER(sdialog),vbox);
    }
    g_signal_connect(G_OBJECT (newdialog), "response",
                     G_CALLBACK (sst->close_func), sst); /* 保存关闭 */
    g_signal_connect(G_OBJECT (newdialog), "destroy",G_CALLBACK(gtk_widget_destroyed), &newdialog);
    g_signal_connect(G_OBJECT (sdialog), "destroy",G_CALLBACK(gtk_widget_destroyed),&dialog_vbox);
    gtk_widget_show_all(newdialog);

}


void factory_create_objectbutton_dialog(GtkWidget *button,SaveStruct *sst)
{
    /* 这里是多个 数组 */
    GtkWidget *sdialog = gtk_hbox_new(FALSE,0);
    GtkWidget *subdig = factory_create_new_dialog_with_buttons(sst->name,button->window);
    GtkWidget *dialog_vbox = GTK_DIALOG(subdig)->vbox;
    gtk_container_add(GTK_CONTAINER(dialog_vbox),sdialog);
    gtk_window_set_modal(GTK_WINDOW(subdig),TRUE);

    NextID *nextid = &sst->value.nextid;
    STRUCTClass *orgclass = sst->sclass;
    nextid->itemlist = factory_get_objects_from_layer(curLayer);
    nextid->itemlist = g_list_insert(nextid->itemlist,g_strdup(""),0);

    int r = 0;
    int pos = 0;
    int tnum = 0;
    ArrayBaseProp *abp = nextid->arr_base;
    for(; r < abp->row ; r++)
    {
        GtkWidget *vbox = gtk_vbox_new(TRUE,0);
        int c = 0;
        tnum = r*abp->col;
        for(; c < abp->col ; c++)
        {
            pos = tnum +c;
            if( pos >= abp->reallen)
                break;
            GtkWidget *hbox = gtk_hbox_new(FALSE,0);
            ActionID *aid = g_list_nth_data(nextid->actlist,pos);
            GtkWidget *comobox = factory_create_combo_widget(nextid->itemlist,aid->index);
            GtkWidget *wid = gtk_label_new(aid->title_name);
            gtk_box_pack_start(GTK_BOX(hbox),wid,FALSE,FALSE,0);
            gtk_box_pack_start(GTK_BOX(hbox),comobox,FALSE,FALSE,0);
            gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
            gtk_container_add(GTK_CONTAINER(vbox),hbox);
        }
        gtk_container_add(GTK_CONTAINER(sdialog),vbox);
    }
    g_signal_connect(G_OBJECT (subdig), "response",
                     G_CALLBACK (sst->close_func), sst); /* 保存关闭 */

    g_signal_connect(G_OBJECT (subdig), "destroy",G_CALLBACK(gtk_widget_destroyed), &subdig);
    g_signal_connect(G_OBJECT (sdialog), "destroy",G_CALLBACK(gtk_widget_destroyed),&dialog_vbox);
    gtk_widget_show_all(subdig);

}


static void factory_create_selectall_and_reverse(GtkWidget *parent,GtkWidget **selectall,GtkWidget **reverse)
{
    GtkWidget *sep1 = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(parent),sep1,TRUE,TRUE,1);
    /* 这里添加全选反选 */
    *selectall = gtk_button_new_with_label(g_locale_to_utf8(_("全选"),-1,NULL,NULL,NULL));
    *reverse = gtk_button_new_with_label(g_locale_to_utf8(_("反选"),-1,NULL,NULL,NULL));
    GtkWidget *hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(parent),hbox,TRUE,TRUE,1);
    gtk_box_pack_end(GTK_BOX(hbox),*selectall,FALSE,FALSE,0);
    gtk_box_pack_end(GTK_BOX(hbox),*reverse,FALSE,FALSE,0);
    GtkWidget *sep2 = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(parent),sep2,TRUE,TRUE,1);
}


static void factory_handle_select_all(GtkWidget *widget,GList *user_data)
{
    GList *wlist = user_data;
    for(; wlist; wlist = wlist->next)
    {
        gtk_toggle_button_set_active(wlist->data,TRUE);
    }
}

static void factory_handle_reverse_select(GtkWidget *widget,GList *user_data)
{
    GList *wlist = user_data;
    for(; wlist; wlist = wlist->next)
    {
        gboolean  b= gtk_toggle_button_get_active(wlist->data);
        gtk_toggle_button_set_active(wlist->data,!b);
    }
}



STRUCTClass *factory_find_diaobject_by_name(Layer *curlayer,const gchar *name)
{
    /* 通过名字在画布上找对像 */
//    gchar *utf8name = g_locale_to_utf8(name,-1,NULL,NULL,NULL);
    GList *objlist = NULL;
    if(curlayer)
        objlist = curlayer->objects;
    STRUCTClass *objclass = NULL;
    for(; objlist ; objlist =  objlist->next )
    {
        if(!factory_is_valid_type(objlist->data))
            continue;
        objclass = objlist->data;
        if(!g_ascii_strcasecmp(objclass->name,name))
        {
            break;
        }

    }
    return objclass;
}

gboolean factory_is_connected(ConnectionPoint *cpend,ConnectionPoint *cpstart)
{
    gboolean isconnected = FALSE;
    DiaObject *obj = factory_find_same_diaobject_via_glist(cpend->connected,cpstart->connected);
    if(obj)
    {
        if((obj->handles[0]->connected_to == cpstart) &&
                (obj->handles[1]->connected_to == cpend))
            isconnected = TRUE;
    }
    return isconnected;
}
STRUCTClass* factory_get_object_from_layer(Layer *layer,const gchar *name)
{
    GList *objlist = NULL;
    STRUCTClass *fclass = NULL;
    if(curLayer)
        objlist = layer->objects ;
    for(; objlist ; objlist =  objlist->next )
    {
        if(!factory_is_valid_type(objlist->data))
            continue;
        fclass = objlist->data;
        if(!g_ascii_strcasecmp(fclass->name,name))
        {
            break;
        }
    }
    return fclass;
}

GList * factory_get_objects_from_layer(Layer *layer)
{
    GList *objlist = NULL;
    if(curLayer)
        objlist = layer->objects ;
    GList *list = NULL;
    for(; objlist ; objlist =  objlist->next )
    {
        if(!factory_is_valid_type(objlist->data))
            continue;
        STRUCTClass* objclass = objlist->data;
        if(!g_ascii_strcasecmp(objclass->element.object.name,"SYS_DATA")
                || factory_is_special_object(objclass->element.object.name))
            continue;
        list = g_list_append(list,objclass->name);
    }
    return list;
}


//static void factory_create_checkbuttons_by_list(gpointer item,SaveStruct *sst)
//{
//    GtkWidget *parent = gtk_widget_get_ancestor(item,GTK_TYPE_WINDOW);
//    ActionID *aid = &sst->value.nextid;
//
//    /* 通按钮事件,创建属性对话框*/
//    GtkWidget*  subdig = gtk_dialog_new_with_buttons(
//                             _(sst->name),
//                             GTK_WINDOW (parent),
//                             GTK_DIALOG_MODAL,
//                             GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
//                             GTK_STOCK_OK, GTK_RESPONSE_OK,
//                             NULL);
//
//    GtkWidget *sdialog = gtk_vbox_new(FALSE,0);
//    gtk_dialog_set_default_response (GTK_DIALOG(subdig), GTK_RESPONSE_OK);
//    GtkWidget *dialog_vbox = GTK_DIALOG(subdig)->vbox;
//    gtk_container_add(GTK_BOX(dialog_vbox),sdialog);
//
//    gtk_window_set_resizable (subdig,FALSE);
//    gtk_window_set_position (subdig,GTK_WIN_POS_MOUSE);
//    gtk_window_present(GTK_WINDOW(subdig));
//    GtkWidget *hbox = gtk_hbox_new(FALSE,0);
//    gtk_box_pack_start(GTK_BOX(sdialog),hbox,FALSE,FALSE,0);
//    g_list_free(aid->itemlist);
//    aid->itemlist = NULL;
//
//        /*  收集界面上所有的对像名 */
//        STRUCTClass *orgclass = sst->sclass;
//        Layer *curlayer = orgclass->element.object.parent_layer;
//        GList *objlist = curlayer->objects;
//        for(; objlist ; objlist =  objlist->next )
//        {
//            if(factory_is_valid_type(objlist->data))
//                continue;
//            STRUCTClass* objclass = objlist->data;
//            aid->itemlist = g_list_append(aid->itemlist,objclass->name);
//        }
//
//
//    int n = 0;
//    GList *ilist = aid->itemlist;
//
//    for(; ilist; ilist = ilist->next,n++)
//    {
//        gchar *key = ilist->data;
//        GtkWidget *wid = gtk_check_button_new_with_label(key);
//        STRUCTClass *orgclass = sst->sclass;
//        Layer *curlayer = orgclass->element.object.parent_layer;
//        STRUCTClass *endclass =  factory_find_diaobject_by_name(curlayer,key);
//        if(endclass)
//        {
//            ConnectionPoint *cpstart = &orgclass->connections[8];
//            ConnectionPoint *cpend = &endclass->connections[8];
//            gboolean isConected = factory_is_connected(cpend,cpstart);
//            gtk_toggle_button_set_active(wid,isConected);
//        }
//
//        if( n &&  !(n % 2) )
//        {
//            hbox = gtk_hbox_new(FALSE,0);
//            gtk_box_pack_start(GTK_BOX(sdialog),hbox,FALSE,FALSE,0);
//            gtk_box_pack_start(GTK_BOX(hbox),wid,FALSE,FALSE,0);
//        }
//        else
//            gtk_box_pack_start(GTK_BOX(hbox),wid,FALSE,FALSE,0);
//
//        aid->wlist = g_list_append(aid->wlist,wid);
//    }
//
//
//
//    GtkWidget *sall = NULL;
//    GtkWidget *rall = NULL;
//    factory_create_selectall_and_reverse(sdialog,&sall,&rall);
//    g_signal_connect(G_OBJECT (sall), "clicked",G_CALLBACK (factory_handle_select_all), aid->wlist);
//    g_signal_connect(G_OBJECT (rall), "clicked",G_CALLBACK (factory_handle_reverse_select), aid->wlist);
//
//    g_signal_connect(G_OBJECT (subdig), "response",
//                     G_CALLBACK (factory_draw_many_lines_dialog), sst); /* 保存关闭 */
//    g_signal_connect(G_OBJECT (subdig), "destroy",
//                     G_CALLBACK(gtk_widget_destroyed), &subdig);
//    g_signal_connect(G_OBJECT (sdialog), "destroy",
//                     G_CALLBACK(gtk_widget_destroyed),&dialog_vbox);
//    gtk_widget_show_all(subdig);
//}


void factory_create_unionbutton_dialog(GtkWidget *button,SaveStruct *sst)
{

    SaveUbtn *sbtn = &sst->value.ssubtn;
    /* 通按钮事件,创建属性对话框*/
//    GtkWidget *sdialog = gtk_hbox_new(FALSE,0);
    GtkWidget *subdig = factory_create_new_dialog_with_buttons(sst->name,button->window);
    GtkWidget *dialog_vbox = GTK_DIALOG(subdig)->vbox;

    gtk_window_set_modal(GTK_WINDOW(subdig),TRUE);

    gtk_dialog_set_default_response (GTK_DIALOG(subdig), GTK_RESPONSE_OK);
    gtk_window_set_resizable (GTK_WINDOW(subdig),FALSE);
    gtk_window_set_position (GTK_WINDOW(subdig),GTK_WIN_POS_MOUSE);

    int num = g_list_length(sbtn->structlist);
    GtkTable *table = gtk_table_new(num,4,FALSE);  // 2014-3-19 lcy 根据要链表的数量,创建多少行列表.

    STRUCTClassDialog *sdialog = g_new0(STRUCTClassDialog,1);
    gtk_container_add(GTK_CONTAINER(dialog_vbox),sdialog);
    sdialog->dialog = gtk_vbox_new(TRUE,0);
    sdialog->mainTable = table;
    gtk_container_add(GTK_CONTAINER(sdialog->dialog),sdialog->mainTable);

//    GList *subitem = g_hash_table_get_values(sbtn->htoflist);
    GList *subitem = sbtn->savelist;
    if(subitem)
        factory_create_struct_dialog(GTK_WIDGET(table),subitem);
    else
    {
        if(sbtn->structlist)
        {
            g_list_free1(sbtn->savelist);
            sbtn->savelist = NULL;
            GList *slist = sbtn->structlist;
            for(; slist; slist = slist->next)
            {
                FactoryStructItem *o = slist->data;
                SaveStruct *s  = factory_get_savestruct(o);
                s->sclass = sst->sclass;
                factory_strjoin(&s->name,sst->name,".");
                sbtn->savelist = g_list_append(sbtn->savelist,s);
            }
            subitem = sbtn->savelist;
            if(subitem)
            {
                factory_create_struct_dialog(GTK_WIDGET(table),subitem);
            }
        }
    }

    gtk_box_pack_start(GTK_BOX(dialog_vbox),sdialog->dialog,FALSE,FALSE,0);
    gtk_table_set_col_spacing(GTK_TABLE(table),1,20);
    g_signal_connect(G_OBJECT (subdig), "response",G_CALLBACK (sst->close_func), sst); /* 保存关闭 */
    g_signal_connect(G_OBJECT (subdig), "destroy",G_CALLBACK(gtk_widget_destroyed), &subdig);
    g_signal_connect(G_OBJECT (sdialog), "destroy",G_CALLBACK(gtk_widget_destroyed),&dialog_vbox);
    gtk_widget_show_all(subdig);
}



GtkWidget *factory_create_many_checkbox(SaveStruct *sss)
{
    SaveEntry *sey = &sss->value.sentry;
    GtkTooltips *tips = gtk_tooltips_new();

    int row = sey->arr_base->row;
    int col = sey->arr_base->col;
    int r = 0 ;
    GtkWidget *vbox  = gtk_vbox_new(FALSE,0);
    gtk_tooltips_set_tip(tips,vbox,sss->org->Comment,NULL);
    g_list_free1(sey->wlist);
    sey->wlist = NULL;
    for(; r < row ; r++)
    {
        GtkWidget *hbox = gtk_hbox_new(FALSE,0);
        gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
        int c = 0;
        for(; c < col ; c++)
        {
            GtkWidget *checkbtn = gtk_check_button_new_with_label(g_strdup_printf(_("p%d"),7-c));
            gchar *val = g_list_nth_data(sey->data,r*col+c);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn),
                                         !g_ascii_strncasecmp(val,_("1"),1) ? TRUE : FALSE);
            gtk_box_pack_start(GTK_BOX(hbox),checkbtn,FALSE,FALSE,1);
            sey->wlist = g_list_append(sey->wlist,checkbtn);
        }
    }
    return vbox;
}

GtkWidget *factory_create_many_entry_box(SaveStruct *sss)
{

    /* 2014-4-9 lcy 显示N多个控件框*/
    SaveEntry *sey = &sss->value.sentry;
    GtkTooltips *tips = gtk_tooltips_new();

    GList *wlist = NULL;
    int row = sey->arr_base->row;
    int col = sey->arr_base->col;
//    if( (row == 1) && (col > 8))
//    {
//        /* 2014-3-31 lcy 一维数组化成二维数组用来显示*/
//        row = sey->width * 2;
//        col = col / row;
//    }
    int r = 0 ;
    GtkWidget *vbox  = gtk_vbox_new(TRUE,0);
    gtk_tooltips_set_tip(tips,vbox,sss->org->Comment,NULL);
    for(; r < row ; r++)
    {
        GtkWidget *hbox = gtk_hbox_new(TRUE,0);
        int c = 0;
        for(; c < col ; c++)
        {
            GtkWidget *entry = gtk_entry_new();

            int maxlength = 2+sey->width * 2; /* 2014-3-31 lcy  宽度为  0x + 宽度*2  */
            gtk_entry_set_max_length (GTK_ENTRY (entry), maxlength);
            gtk_entry_set_width_chars(GTK_ENTRY (entry), maxlength);
            gchar *str = g_list_nth_data(sey->data,r*col+c);
            if(str)
                gtk_entry_set_text(GTK_ENTRY (entry),str);
            else
            {
                gtk_entry_set_text(GTK_ENTRY (entry),"0x");
                int w  = 0;
                for(w; w < sey->width; w++ ) /* 2014-3-31 lcy 这里设置默认对齐的字符*/
                    gtk_entry_append_text(GTK_ENTRY(entry),"ff");
            }

            g_signal_connect(entry, "delete_text", G_CALLBACK(factory_editable_delete_callback), NULL);
            g_signal_connect(entry, "insert_text", G_CALLBACK(factory_editable_insert_callback), NULL);
            g_signal_connect(GTK_OBJECT(entry), "enter_notify_event", G_CALLBACK(factory_editable_active_callback), NULL);
            g_signal_connect(GTK_OBJECT(entry), "selection_notify_event", G_CALLBACK(factory_editable_active_callback), NULL);
            gtk_box_pack_start(GTK_BOX(hbox),entry,FALSE,FALSE,0);
            wlist = g_list_append(wlist,entry);
        }
        gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    }
    g_list_free(sey->wlist);
    sey->wlist = wlist;
    return (GtkWidget *)vbox;
}

void factory_editable_delete_callback(GtkEditable *edit,
                                      gint start_pos,
                                      gint end_pos)
{
    gchar *txt = gtk_editable_get_chars (edit,0,-1);


    if(  (2==strlen(txt)) && (0 == g_ascii_strncasecmp(txt,"0x",2)))
    {
        g_signal_stop_emission_by_name (G_OBJECT (edit), "delete_text");
    }

    if(start_pos < 2) /* 保证0x开始*/
    {
        g_signal_stop_emission_by_name (G_OBJECT (edit), "delete_text");
        gtk_editable_select_region(edit,2,end_pos);
        gtk_editable_delete_text (edit,2,end_pos);
    }
}

void factory_editable_active_callback(GtkEditable *edit,gpointer data)
{
    gtk_editable_select_region(edit,2,-1);
    g_signal_stop_emission_by_name (G_OBJECT (edit), "selection_clear_event");

}

void factory_editable_insert_callback(GtkEntry *entry,
                                      gchar* new_text,
                                      gint new_length,
                                      gpointer position,
                                      gpointer data)
{
    GtkEditable *edit = GTK_EDITABLE(entry);
    gchar *tmp = new_text;
    gchar *result = g_new(gchar,new_length);
    memset(result,'\0',new_length);
    gchar *ns = result;
    while(*tmp)
    {
        if( *tmp == 'x' || g_ascii_isxdigit(*tmp))
        {
            *ns++ = *tmp++;
        }
        else
            *tmp++;
    }

    /* 2014-3-31 lcy 这里添加字符串跟其它框架不一样,必须按以下步骤操作*/
    if(ns - result)
    {
        g_signal_handlers_block_by_func(G_OBJECT(edit),
                                        G_CALLBACK(factory_editable_insert_callback),
                                        data);
        gtk_editable_insert_text(edit,result,ns-result,position);
        g_signal_handlers_unblock_by_func (G_OBJECT (edit),
                                           G_CALLBACK (factory_editable_insert_callback),data);
    }
    g_signal_stop_emission_by_name (G_OBJECT (edit), "insert_text"); /* 2014-3-31 lcy 这里要停止发射信号,否则会进入无限循环*/
}




void factory_create_struct_dialog(GtkWidget *dialog,GList *datalist)
{
    /* 通链表创建控件 SaveStruct *sst */
    GList *item = datalist;
    int row = 0;
    for(; item  ; item = item->next)
    {
        SaveStruct *sst  = item->data;
        factory_set_savestruct_widgets(sst);
        if(g_ascii_strcasecmp(factory_locale(sst->org->Cname),_("保留"))) /* 保留的不显示出来 */
        {
            factory_set_twoxtwo_table(dialog,sst->widget1,sst->widget2,row++);
        }

    }
}

/* 2014-5-4 lcy 这里要添加一些公共选项 */
void factory_append_public_info(GtkWidget *dialog,STRUCTClass *fclass)
{
    GtkWidget *chbox = gtk_check_button_new_with_label(factory_utf8(_("编辑完成")));
    GtkWidget *entry = gtk_entry_new();

    GtkWidget *sep = gtk_hseparator_new();
    gtk_container_add(GTK_CONTAINER(dialog),sep);
    PublicSection *pps = NULL;
    if(NULL == fclass->pps)
    {
        pps = g_new0(PublicSection,1);
        fclass->pps = pps;
        pps->hasfinished = FALSE;
        pps->name = g_strdup(fclass->name);
    }
    else
    {
        pps = fclass->pps;
    }
    pps->wid_hasfinished  =  chbox	;
    GtkWidget *hbox = gtk_hbox_new(FALSE,0);
    GtkWidget *name_lab = gtk_label_new(g_locale_to_utf8(_("对像重命名:"),-1,NULL,NULL,NULL));
    pps->wid_rename_entry = entry;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(chbox),pps->hasfinished);
    gtk_entry_set_text( GTK_ENTRY(pps->wid_rename_entry),pps->name);
    gtk_entry_set_max_length(GTK_ENTRY(pps->wid_rename_entry),255);
    gtk_box_pack_start(GTK_BOX(hbox), name_lab, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), pps->wid_rename_entry, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(dialog),pps->wid_hasfinished);
    gtk_container_add(GTK_CONTAINER(dialog),hbox);


}

/* 显示控件上的属性对话框 */
void factory_create_and_fill_dialog(STRUCTClass *fclass, gboolean is_default)
{
    DDisplay *ddsp = ddisplay_active();
    curLayer = ddsp->diagram->data->active_layer;
    STRUCTClassDialog *prop_dialog = fclass->properties_dialog;

//    if(!g_ascii_strcasecmp(fclass->element.object.name,"IDLIST"))
//    {
//        factory_idlist_dialog(factory_utf8("ID列表"),prop_dialog->dialog,&fclass->widgetSave);
//    }
//    else if(!g_ascii_strcasecmp(fclass->element.object.name,"FILELIST"))
//    {
//        if(!fclass->widgetSave)
//        {
//            SaveMusicDialog *smd = g_new0(SaveMusicDialog,1);
//
//            smd = g_new0(SaveMusicDialog,1);
//
//            fclass->widgetSave = g_list_append(fclass->widgetSave,smd);
//        }
////        factory_file_manager_dialog(factory_utf8("文件管理"),prop_dialog->dialog,fclass->widgetSave);
//    }
//    else
    {
        GList *targetlist = fclass->widgetSave;
        if(!targetlist)
        {
            factory_read_initial_to_struct(fclass);
        }
        int num = g_list_length(targetlist);
        prop_dialog->mainTable = gtk_table_new(num,4,FALSE);  // 2014-3-19 lcy 根据要链表的数量,创建多少行列表.
        factory_create_struct_dialog(prop_dialog->mainTable,targetlist);
        gtk_container_add(GTK_CONTAINER(prop_dialog->dialog),prop_dialog->mainTable);
        if(g_ascii_strcasecmp(fclass->element.object.name,"SYS_DATA"))
        {
            factory_append_public_info(prop_dialog->dialog,fclass);
            gtk_table_set_col_spacing(GTK_TABLE(prop_dialog->mainTable),1,20);
        }
    }
}



/**** 这里添加了两个特殊控件　***/

void factory_music_file_manager_remove_all(SaveMusicDialog *smd)
{
    GList *tlist = smd->itemlist;
    for(; tlist ; tlist = tlist->next)
    {
        SaveMusicItem *smi = tlist->data;
        GtkTreeModel *store = gtk_combo_box_get_model( smi->wid_colum3 );
        gtk_list_store_clear( GTK_TREE_STORE( store ) ); /* 清空原来的 */
    }
}


void factory_music_file_manager_new_item_changed(SaveMusicDialog *smd)
{
    GList *tlist = smd->itemlist;
    GList *newflist = smd->cboxlist;
    for(; tlist ; tlist = tlist->next)
    {
        SaveMusicItem *smi = tlist->data;
        smi->active = gtk_combo_box_get_active(GTK_COMBO_BOX(smi->wid_colum3));
        GtkTreeModel *store = gtk_combo_box_get_model(GTK_COMBO_BOX(smi->wid_colum3 ));
        gtk_list_store_clear( GTK_TREE_STORE( store ) ); /* 清空原来的 */
        newflist = smd->cboxlist;
        gtk_combo_box_append_text(smi->wid_colum3,"");
        for(; newflist ; newflist = newflist->next)
        {
            gtk_combo_box_append_text(smi->wid_colum3,newflist->data);
            gtk_combo_box_set_active(GTK_COMBO_BOX(smi->wid_colum3),smi->active);
        }

    }
}




gboolean factory_is_system_data(const gchar *name)
{
    gboolean *b = FALSE;
    if(!g_ascii_strcasecmp(name,"SYS_DATA"))
        b = TRUE;
    return b;
}


GtkWidget *factory_get_new_item_head()
{
    GtkWidget *hbox = gtk_hbox_new(FALSE,0);
    GtkWidget *first = gtk_label_new(factory_utf8("序号"));
    GtkWidget *sec = gtk_label_new(factory_utf8("地址"));
    GtkWidget *third = gtk_label_new(factory_utf8("行为ID"));
    GtkWidget *four = gtk_label_new(factory_utf8("操作"));
    gtk_box_pack_start(GTK_BOX(hbox),first,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),sec,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),third,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),four,TRUE,TRUE,0);
    gtk_widget_show_all(hbox);
    return hbox;
}

GtkWidget *factory_file_id_manager_head()
{
    GtkWidget *hbox = gtk_hbox_new(FALSE,0);
    GtkWidget *first = gtk_label_new(factory_utf8("索引号"));
    GtkWidget *sec = gtk_label_new(factory_utf8("地址"));
    GtkWidget *third = gtk_label_new(factory_utf8("音乐文件"));
//    GtkWidget *four = gtk_label_new(factory_utf8("添加"));
    gtk_box_pack_start(GTK_BOX(hbox),first,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),sec,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),third,TRUE,TRUE,0);
//    gtk_box_pack_start(GTK_BOX(hbox),four,TRUE,TRUE,0);
    gtk_widget_show_all(hbox);
    return hbox;
}

GtkWidget *factory_get_new_musicitem(SaveMusicItem *smt,GList *fillist,GSList **grouplist)
{
    /* 这里的文件管理 */
    GtkWidget *hbox = gtk_hbox_new(FALSE,1);
    GtkWidget *radio = gtk_radio_button_new(*grouplist);
    *grouplist = gtk_radio_button_get_group(GTK_TOGGLE_BUTTON(radio));

    GtkWidget *first = gtk_label_new(g_strdup_printf("%d",smt->id_index));
    gtk_widget_set_size_request(first,50,-1);
    GtkWidget *spbox = gtk_spin_button_new_with_range(-1,65535,2);
    gtk_widget_set_size_request(spbox,50,-1);
    gtk_widget_set_sensitive(spbox,FALSE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spbox),smt->id_addr);

    GtkWidget *cbox = gtk_combo_box_text_new();
    gtk_combo_box_append_text(GTK_COMBO_BOX(cbox),"");
    GList *tlist = fillist;
    for(; tlist; tlist = tlist->next)
    {
        gtk_combo_box_append_text(GTK_COMBO_BOX(cbox),tlist->data);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(cbox),smt->active);
    gtk_box_pack_start(GTK_BOX(hbox),radio,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),first,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),spbox,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),cbox ,TRUE,TRUE,0);
    gtk_combo_box_popdown(GTK_COMBO_BOX(cbox));
    smt->wid_colum2 = spbox ;
    smt->wid_colum3 = cbox;
    smt->wid_colum0 = radio;
    gtk_widget_show_all(hbox);
    return hbox;
}



//GtkWidget *opendlg = NULL;

static void factory_file_manager_insert(GtkWidget *clist,SaveMusicFile *smf,gint pos)
{
    gchar *first = g_strdup_printf("%d",smf->index);
    gchar *second = g_strdup_printf("%d",smf->file_addr);
    gchar *third =smf->base_name;
    gchar *four = smf->down_name;
    gchar *row[] = {first,second,third,four};
    gtk_clist_insert(GTK_CLIST(clist),pos,row);
}

static void factory_file_manager_append(GtkWidget *clist,SaveMusicFile *smf)
{
    gchar *first = g_strdup_printf("%d",smf->index);
    gchar *second = g_strdup_printf("%d",smf->file_addr);
    gchar *third =smf->base_name;
    gchar *four = smf->down_name;
    gchar *row[] = {first,second,third,four};

    gtk_clist_append(GTK_CLIST(clist),row);
}

static void factory_choose_musicfile_callback(GtkWidget *dlg,
        gint       response,
        gpointer   user_data)
{
    SaveMusicDialog *smd = user_data;
    SaveMusicFileMan *smfm = smd->smfm;
    if (response == GTK_RESPONSE_ACCEPT)
    {
        int offset = gtk_spin_button_get_value(GTK_SPIN_BUTTON(smfm->wid_offset));
        int num = smfm->number;

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

        if(smfm->number == 0)
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
            int pos = smfm->selected;
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
            factory_file_manager_refresh_all(smd);
        }
        else
        {
            GList *p = filelist;
            for(; p; p = p->next)
                factory_file_manager_append(smfm->wid_clist,p->data);
        }
        smfm->number = g_list_length(smfm->filelist);
        smd->mfmos->m_itemadded(smd); /* 更新左边的界面 */
//        int width =  gtk_clist_columns_autosize (GTK_CLIST(smfm->wid_clist));
//        GtkWidget *cparent = gtk_widget_get_parent(smfm->wid_clist);
//        gtk_widget_set_size_request(cparent,width,-1);
    }
    gtk_widget_destroy(dlg);
}





void factory_open_file_dialog(GtkWidget *widget,gpointer user_data)
{
    SaveMusicDialog *smd =(SaveMusicDialog *) user_data;
    SaveMusicFileMan *smfm = smd->smfm;
    if(!smfm->opendlg)
    {
        smfm->opendlg = gtk_file_chooser_dialog_new_with_backend(_("打开音乐文件"), widget,
                        GTK_FILE_CHOOSER_ACTION_OPEN,
                        "default", /* default, not gnome-vfs */
                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                        NULL);
        gtk_dialog_set_default_response(GTK_DIALOG(smfm->opendlg), GTK_RESPONSE_ACCEPT);
        gtk_window_set_modal(GTK_WINDOW(smfm->opendlg),TRUE);
        gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (smfm->opendlg), TRUE);
        if(smfm->lastDir )
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(smfm->opendlg),smfm->lastDir);
//
//        if(smt->full_name)
//            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(opendlg), smt->full_name);
        g_signal_connect(GTK_OBJECT(smfm->opendlg), "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &smfm->opendlg);

        if (!gtk_file_chooser_get_extra_widget(GTK_FILE_CHOOSER(smfm->opendlg)))
        {
            GtkWidget *omenu= gtk_combo_box_new();
            GtkFileFilter* filter;

            /* set up the gtk file (name) filters */
            /* 0 = by extension */

            filter = gtk_file_filter_new ();
            gtk_file_filter_set_name (filter, _("All Files"));
            gtk_file_filter_add_pattern (filter, "*");

            filter = gtk_file_filter_new ();
            gtk_file_filter_set_name (filter, _("Music Files"));
            gtk_file_filter_add_pattern (filter, "*.wav");
            gtk_file_filter_add_pattern (filter, "*.wma");
            gtk_file_filter_add_pattern (filter, "*.mp3");
            gtk_file_filter_add_pattern (filter, "*.f1a");
            gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (smfm->opendlg), filter);



            gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (smfm->opendlg), filter);

//        gtk_combo_box_set_active (GTK_COMBO_BOX (omenu), persistence_get_integer ("import-filter"));
            g_signal_connect(GTK_FILE_CHOOSER(smfm->opendlg),
                             "response", G_CALLBACK(factory_choose_musicfile_callback), smd);
        }
    }
    else
    {
        if(smfm->lastDir )
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(smfm->opendlg),smfm->lastDir);
//        if(smt->full_name)
//            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(opendlg), smt->full_name);
        gtk_widget_set_sensitive(smfm->opendlg, TRUE);
        if (GTK_WIDGET_VISIBLE(smfm->opendlg))
            return;
    }
    gtk_widget_show(smfm->opendlg);
}


void factory_add_item_to_music_manager(GtkButton *self,gpointer user_data)
{
    SaveMusicDialog *smd = user_data;
    SaveMusicFileMan *smfm = smd->smfm;

    SaveMusicItem *smt = g_new0(SaveMusicItem,1);
    smt->id_index = g_list_length(smd->itemlist);
    smt->id_addr = smt->id_index * 2;

    GtkWidget *nitem = factory_get_new_musicitem(smt,smd->cboxlist,&smd->grouplist);

    smd->itemlist= g_list_append(smd->itemlist,smt);
    gtk_box_pack_start(GTK_BOX(smd->leftvbox),nitem,FALSE,FALSE,0);

    gtk_widget_show_all(smd->leftvbox);

}

GtkWidget *factory_new_add_button(factory_button_callback *callback,gpointer list)
{
    GtkWidget *btn = gtk_button_new_from_stock (GTK_STOCK_ADD);
    gtk_widget_set_name (btn,"add_button");
    g_signal_connect (G_OBJECT (btn), "clicked",
                      G_CALLBACK (callback),
                      list);
    return btn;
}


GtkWidget *factory_get_new_iditem(SaveIdItem *swt,GSList **grouplist)
{
    GtkWidget *hbox = gtk_hbox_new(FALSE,1);
    GtkWidget *radio = gtk_radio_button_new(*grouplist);
    gtk_widget_set_size_request(radio,20,-1);
    *grouplist = gtk_radio_button_get_group(GTK_TOGGLE_BUTTON(radio));
    GtkWidget *first = gtk_label_new(g_strdup_printf("%d",swt->id_index));
    gtk_widget_set_size_request(first,50,-1);
    GtkWidget *spbox = gtk_spin_button_new_with_range(0,65536,2);
    gtk_widget_set_size_request(first,80,-1);
    gtk_widget_set_sensitive(spbox,FALSE);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spbox),swt->id_addr);

    GtkWidget *cbox = gtk_combo_box_new_text();


    gtk_combo_box_popdown(GTK_COMBO_BOX(cbox));
    GList *p = curLayer->defnames;
    gtk_combo_box_append_text(GTK_COMBO_BOX(cbox),"");
    for(; p ; p = p->next)
    {
        gtk_combo_box_append_text(GTK_COMBO_BOX(cbox),p->data);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(cbox),swt->active);

    gtk_box_pack_start(GTK_BOX(hbox),radio,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),first,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),spbox,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),cbox,TRUE,TRUE,0);
//    gtk_box_pack_start(GTK_BOX(hbox),btn_del,TRUE,TRUE,0);

    swt->wid_colum0 = radio;
    swt->wid_colum2 = spbox;
    swt->wid_colum3 = cbox;
    return hbox;
}

void factory_new_idlist_dialog(GtkWidget *parent,SaveStruct *sst)
{
    GtkWidget *mainBox = gtk_vbox_new(FALSE,0);

//    IdDialog->dvalue = &sst->value.vnumber;
    IdDialog->skv = sst->value.vnumber;
     gtk_button_set_label(GTK_BUTTON(parent),IdDialog->skv->value);
    IdDialog->parent_btn = parent;
    GtkWidget *subdig = factory_create_new_dialog_with_buttons(factory_utf8("ID列表"),parent->window);
    GtkWidget *dialog_vbox = GTK_DIALOG(subdig)->vbox;

    gtk_window_set_modal(GTK_WINDOW(subdig),TRUE);
    gtk_dialog_set_default_response (GTK_DIALOG(subdig), GTK_RESPONSE_OK);
    gtk_window_set_resizable (GTK_WINDOW (subdig),TRUE);
    gtk_widget_set_size_request (subdig,300,500);
    gtk_container_add(GTK_CONTAINER(dialog_vbox),mainBox);
    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);

    GtkWidget *vbox  = gtk_vbox_new(FALSE,0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(wid_idlist),vbox);
    gtk_box_pack_start(GTK_BOX(vbox),factory_get_new_item_head(),FALSE,FALSE,0);
    IdDialog->vbox = vbox;
    if(IdDialog->idlists) /* 设置回原来的值　*/
    {
        GList *tlist = IdDialog->idlists;
        int n = 0;
        for(; tlist; tlist = tlist->next,n++)
        {
            SaveIdItem *swt = tlist->data;
            GtkWidget *nbox =  factory_get_new_iditem(swt,&IdDialog->grouplist);
            if(n == IdDialog->skv->radindex)
                gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(swt->wid_colum0),TRUE);
            gtk_box_pack_start(GTK_BOX(vbox),nbox,FALSE,FALSE,0);
        }
    }

    gtk_box_pack_start(GTK_BOX(mainBox),wid_idlist,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(mainBox),
                       factory_new_add_button(factory_add_item_to_idlist,IdDialog),FALSE,FALSE,0);

    g_signal_connect(G_OBJECT (subdig), "response",G_CALLBACK (sst->close_func), IdDialog); /* 保存关闭 */
    g_signal_connect(G_OBJECT (subdig), "destroy",G_CALLBACK(gtk_widget_destroyed), &subdig);
    g_signal_connect(G_OBJECT (mainBox), "destroy",G_CALLBACK(gtk_widget_destroyed),&dialog_vbox);

    gtk_widget_show_all(subdig);
}


void factory_add_item_to_idlist(GtkButton *self,gpointer user_data)
{
    SaveIdDialog *sid = (SaveIdDialog *)user_data;

    GList *ilist =  gtk_container_get_children(GTK_CONTAINER(sid->vbox));
    int len = g_list_length(ilist);
    SaveIdItem *swt = g_new0(SaveIdItem,1);

    swt->id_index =  len-1;
    swt->id_addr = (swt->id_index)*2;
    swt->active = 0;

    GtkWidget *nitem = factory_get_new_iditem(swt,&IdDialog->grouplist);

    sid->idlists = g_list_append(sid->idlists,swt);

    gtk_box_pack_start(GTK_BOX(sid->vbox),nitem,FALSE,TRUE,0);
//    gtk_box_reorder_child(GTK_BOX(vbox),GTK_WIDGET(self),len); /* 交换两行的位置 */

    gtk_widget_show_all(sid->vbox);
}


/*　左边的界面　*/
GtkWidget *factory_download_file_manager(GtkWidget *parent,SaveMusicDialog *smd)
{
    GtkWidget *mvbox = gtk_vbox_new(FALSE,5);

    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),
                                   GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);

    GtkWidget *vbox  = gtk_vbox_new(FALSE,5);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(wid_idlist),vbox);
    gtk_box_pack_start(GTK_BOX(vbox),factory_file_id_manager_head(),FALSE,FALSE,0);
    smd->leftvbox = vbox;
    if(g_list_length(smd->itemlist)>0) /* 显示原有的数据　*/
    {
        GList *tlist = smd->itemlist;
        for(; tlist ; tlist = tlist->next)
        {
            SaveMusicItem *smt = tlist->data;
            GtkWidget *nitem = factory_get_new_musicitem(smt,smd->cboxlist,&smd->grouplist);
            gtk_box_pack_start(GTK_BOX(smd->leftvbox),nitem,FALSE,FALSE,0);
        }
        /* 设置默认值 */
        SaveMusicItem *smit = g_list_nth_data(smd->itemlist,smd->skv->radindex);
        if(smit)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(smit->wid_colum0), TRUE);
    }

    gtk_box_pack_start(GTK_BOX(mvbox),wid_idlist,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(mvbox),factory_new_add_button(factory_add_item_to_music_manager,smd),FALSE,FALSE,0);
    return mvbox;
}

void factory_save_idlist_dialog(GtkWidget *widget,
                                gint response_id,
                                gpointer user_data)
{
   SaveIdDialog *sid = IdDialog;
       if (   response_id == GTK_RESPONSE_APPLY
            || response_id == GTK_RESPONSE_OK)
    {
        GList *p = sid->idlists;
        SaveKV *skv = sid->skv;
        for(;p ; p = p->next)
        {
            SaveIdItem *sil = p->data;
            sil->dname  = gtk_combo_box_get_active_text(GTK_COMBO_BOX(sil->wid_colum3));
            sil->active = gtk_combo_box_get_active(GTK_COMBO_BOX(sil->wid_colum3));
            if(gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(sil->wid_colum0)))
            {
                skv->radindex  = g_list_index(sid->idlists,sil);
                STRUCTClass *fclass = factory_find_diaobject_by_name(curLayer,sil->dname);
                if(fclass){
                SaveStruct *tmp = fclass->widgetSave->data;
//                *sid->dvalue = g_strdup(tmp->value.vnumber);
                skv->value = g_strdup(tmp->value.vnumber);
                }

            }
        }
        gtk_button_set_label(GTK_BUTTON(sid->parent_btn),skv->value);
    }

    gtk_widget_hide_all(widget);
}


void factory_music_file_manager_apply(GtkWidget *widget,
                                      gint       response_id,
                                      SaveMusicDialog *smd)
{

    if (   response_id == GTK_RESPONSE_APPLY
            || response_id == GTK_RESPONSE_OK)
    {

        gint v = -1;
        GList *list = smd->itemlist;
        SaveKV *skv = smd->skv;
        for(; list ; list = list->next)
        {
            SaveMusicItem *smi = list->data;
            smi->active = gtk_combo_box_get_active(GTK_COMBO_BOX(smi->wid_colum3));
            smi->dname = gtk_combo_box_get_active_text(GTK_COMBO_BOX(smi->wid_colum3));

            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(smi->wid_colum0)))
            {
                if(smd->fmst == SEQUENCE)
                {
                    v = g_list_index(smd->itemlist,smi);
                }
                else if(smd->fmst == INDEX)
                {
                    SaveMusicFile *smf = g_list_nth_data(smd->smfm->filelist,smi->active-1);
                    if(smf)
                    v = g_list_index(smd->smfm->filelist,smf);
                }
                else if(smd->fmst == PHY)
                {
                    SaveMusicFile *smf = g_list_nth_data(smd->smfm->filelist,smi->active-1);
                    if(smf)
                    v = smf->file_addr;
                }
                skv->radindex = g_list_index(smd->itemlist,smi);
            }
        }
        skv->value = g_strdup_printf("%d",v);
        gtk_button_set_label(GTK_BUTTON(smd->parent_btn),skv->value);/* 更改按钮标签*/

    }

    gtk_widget_hide_all(widget);
}

void factory_file_manager_dialog(GtkWidget *btn,SaveStruct *sst)
{
    factory_music_fm_get_type(sst->name);
    SaveMusicDialog *smd = MusicManagerDialog;
    smd->parent_btn = btn;

//    smd->dvalue = &sst->value.vnumber;
    smd->skv = sst->value.vnumber;
    GtkWidget *mainHbox = gtk_hbox_new(FALSE,0);
    GtkWidget *window = factory_create_new_dialog_with_buttons(smd->title,smd->parent_btn);
    GtkWidget *dialog_vbox = GTK_DIALOG(window)->vbox;
    gtk_container_add(GTK_CONTAINER(dialog_vbox),mainHbox);
    gtk_window_set_modal(GTK_WINDOW(window),TRUE);

    gtk_window_set_resizable (GTK_WINDOW (window),FALSE);
    gtk_widget_set_size_request (GTK_WINDOW (window),800,500);

    /* 这里按三列布局，左一列表框，中间线，右一列表框*/

    gtk_box_pack_start(GTK_BOX(mainHbox),factory_download_file_manager(NULL,smd),TRUE,TRUE,1);
    gtk_box_pack_start(GTK_BOX(mainHbox),factory_music_file_manager(NULL,smd),TRUE,TRUE,1);

    g_signal_connect(G_OBJECT (window), "response",
                     G_CALLBACK (smd->mfmos->m_applydialog), smd); /* 保存关闭 */

    g_signal_connect(G_OBJECT (window), "destroy",G_CALLBACK(gtk_widget_destroyed), &window);
    g_signal_connect(G_OBJECT (mainHbox), "destroy",G_CALLBACK(gtk_widget_destroyed),&dialog_vbox);

    gtk_widget_show_all(window);
}



GtkWidget *factory_music_table_head()
{
    GtkWidget *hbox_head = gtk_hbox_new(FALSE,5);
    GtkWidget *first = gtk_label_new(factory_utf8("序号"));
    GtkWidget *sec = gtk_label_new(factory_utf8("物理文件号"));
    GtkWidget *third = gtk_label_new(factory_utf8("音乐文件"));
    GtkWidget *four = gtk_label_new(factory_utf8("下载文件名"));

    gtk_box_pack_start(GTK_BOX(hbox_head),first,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox_head),sec,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox_head),third,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox_head),four,TRUE,TRUE,0);
    gtk_widget_show_all(hbox_head);
    return hbox_head;
}

typedef void (*MusicFileManagerFunc)(GtkWidget *btn,gpointer user_data);

typedef struct _MusicFileManagerOperation MusicFileManagerOperation;

struct _MusicFileManagerOperation
{
    GtkWidget *btn;
    gchar *name;
    int stack;
    MusicFileManagerFunc mfmf;
};


void factory_add_file_manager_item(GtkWidget *widget, gpointer user_data)
{
    SaveMusicDialog *smd = user_data;
    smd->smfm->man_opt = OPT_APPEND;
    factory_open_file_dialog(widget,user_data);
}


void factory_file_manager_refresh_all(SaveMusicDialog *smd)
{
    /*　清空重新从链表里加载　*/
    SaveMusicFileMan *smfm = smd->smfm;
    gtk_clist_clear(GTK_CLIST(smd->smfm->wid_clist));
    GList *tlist = smfm->filelist;
    int num = 0;
    for(; tlist ; tlist = tlist->next,num++)
    {
        SaveMusicFile *smf = tlist->data;
        smf->index = num;
        smf->file_addr = smf->index + smfm->offset;
        smf->down_name = g_strdup_printf("file_%d",smf->index);
        factory_file_manager_append(smfm->wid_clist,tlist->data);
    }
    gtk_clist_select_row(smfm->wid_clist,smfm->selected,-1);
    smfm->number = num;
    smd->mfmos->m_itemadded(smd);
}


void factory_delete_file_manager_item(GtkWidget *widget,gpointer user_data)
{
    SaveMusicDialog *smd = user_data;
    SaveMusicFileMan *smfm = smd->smfm;
    if(smfm->selected == -1 ||
            (smfm->selected >= GTK_CLIST(smfm->wid_clist)->rows))
        return;

    const gpointer  itm =  g_list_nth_data(smfm->filelist,smfm->selected);
    const gpointer c = g_list_nth_data(smd->cboxlist,smfm->selected);
    smd->cboxlist = g_list_remove(smd->cboxlist,c);
    smfm->filelist = g_list_remove(smfm->filelist,itm);
    factory_file_manager_refresh_all(smd);


}
void factory_cleanall_file_manager_item(GtkWidget *widget,gpointer user_data)
{

    SaveMusicDialog *smd = user_data;
    gtk_clist_clear(GTK_CLIST(smd->smfm->wid_clist));
    smd->smfm->number  = 0;
    g_list_free1(smd->cboxlist) ;
    smd->cboxlist = NULL;
    smd->mfmos->m_clearall(smd);

}
void factory_insert_file_manager_item(GtkWidget *widget,gpointer user_data)
{
    SaveMusicDialog *smd = user_data;
    smd->smfm->man_opt = OPT_INSERT;
    factory_open_file_dialog(NULL,user_data);
}



GtkWidget *factory_music_file_manager_operator(SaveMusicDialog *smd)
{
    /* 要添加操作选项，在这里修改　*/

    MusicFileManagerOperation mfmo[] = {{NULL,"add",GTK_STOCK_ADD,factory_add_file_manager_item},
        {NULL,"delete",GTK_STOCK_DELETE,factory_delete_file_manager_item},
        {NULL,"Insert",NULL,factory_insert_file_manager_item},
        {NULL,"clean",GTK_STOCK_CLEAR,factory_cleanall_file_manager_item}
    };

    GtkWidget *operatorhbox = gtk_hbox_new(FALSE,2);

    int num =sizeof(mfmo)/sizeof(MusicFileManagerOperation);
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
    gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_MULTIPLE);
    gtk_clist_set_column_width(GTK_CLIST(clist),0,50);
    gtk_clist_set_column_width(GTK_CLIST(clist),1,60);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist),2,TRUE);

}

void factory_music_file_manager_select_callback(GtkWidget *clist,
        gint row,gint column,
        GdkEventButton *event,
        gint  *ret)
{
    *ret = row;
}


GtkWidget *factory_music_file_manager(GtkWidget *parent,SaveMusicDialog *smd)
{
    GtkWidget *sdialog = gtk_vbox_new(FALSE,0);
    GtkWidget *label = gtk_label_new(factory_utf8("物理文件号偏移:"));
    GtkWidget *spbox = gtk_spin_button_new_with_range(0,65536,1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spbox),5);
    GtkWidget *hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),spbox,FALSE,FALSE,0);
    GtkWidget *sep = gtk_hseparator_new();

    gchar *titles[] = {factory_utf8("序号"),
                       factory_utf8("物理文件号"),
                       factory_utf8("音乐文件"),
                       factory_utf8("下载文件名")
                      };
    GtkWidget  *wid_idlist = gtk_scrolled_window_new (NULL,NULL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid_idlist),GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);

    GtkWidget *clist = gtk_clist_new_with_titles(4,titles);
    factoy_clist_init(clist);

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(wid_idlist),clist);
    SaveMusicFileMan *smfm = smd->smfm;
    if(!smfm)
    {
        smfm = g_new0(SaveMusicFileMan,1);
        smd->grouplist = NULL;
        smfm->selected = -1;
        smd->smfm = smfm;
    }
    else
    {
        if(smfm->filelist)
        {
            GList *flist = smfm->filelist;
            for(; flist; flist = flist->next)
            {
                factory_file_manager_append(clist,flist->data);
            }
        }
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(spbox),smfm->offset);
    }
    smfm->wid_clist = clist;
    smfm->wid_offset = spbox;
    smfm->offset = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spbox));
    smfm->number = GTK_CLIST(clist)->rows;

    /* 创建四行布局*/

    gtk_box_pack_start(GTK_BOX(sdialog),hbox,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(sdialog),sep,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(sdialog),wid_idlist,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(sdialog),factory_music_file_manager_operator(smd),FALSE,FALSE,0);

    gtk_widget_show_all(sdialog);

    g_signal_connect(clist,"select_row",G_CALLBACK(factory_music_file_manager_select_callback),&smd->smfm->selected);

    return sdialog;

}



//GtkWidget *
//structclass_get_properties(STRUCTClass *structclass, gboolean is_default)
//{
//  STRUCTClassDialog *prop_dialog;
//  GtkWidget *vbox;
//  GtkWidget *notebook;
//
//#ifdef DEBUG
//  structclass_sanity_check(structclass, "Get properties start");
//#endif
//  if (structclass->properties_dialog == NULL) {
//    prop_dialog = g_new(STRUCTClassDialog, 1);
//    structclass->properties_dialog = prop_dialog;
//
//    vbox = gtk_vbox_new(FALSE, 0);
//    gtk_object_ref(GTK_OBJECT(vbox));
//    gtk_object_sink(GTK_OBJECT(vbox));
//
//    prop_dialog->dialog = vbox;
//
//    prop_dialog->current_attr = NULL;
//    prop_dialog->current_op = NULL;
//    prop_dialog->current_param = NULL;
//    prop_dialog->current_templ = NULL;
//    prop_dialog->deleted_connections = NULL;
//    prop_dialog->added_connections = NULL;
//    prop_dialog->disconnected_connections = NULL;
//
//    notebook = gtk_notebook_new ();
//    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
//    gtk_box_pack_start (GTK_BOX (vbox),	notebook, TRUE, TRUE, 0);
//    gtk_container_set_border_width (GTK_CONTAINER (notebook), 10);
//
//    gtk_object_set_user_data(GTK_OBJECT(notebook), (gpointer) structclass);
//
//    gtk_signal_connect (GTK_OBJECT (notebook),
//			"switch_page",
//			GTK_SIGNAL_FUNC(switch_page_callback),
//			(gpointer) structclass);
//    gtk_signal_connect (GTK_OBJECT (structclass->properties_dialog->dialog),
//		        "destroy",
//			GTK_SIGNAL_FUNC(destroy_properties_dialog),
//			(gpointer) structclass);
//
//    create_dialog_pages(GTK_NOTEBOOK( notebook ), structclass);
//
//    gtk_widget_show (notebook);
//  }
//
//  fill_in_dialog(structclass);
//  gtk_widget_show (structclass->properties_dialog->dialog);
//
//  return structclass->properties_dialog->dialog;
//}
//

/****************** UNDO stuff: ******************/

static void
structclass_free_state(STRUCTClassState *state)
{
//    GList *list;

    g_object_unref (state->normal_font);
//  g_object_unref (state->abstract_font);
//  g_object_unref (state->polymorphic_font);
    g_object_unref (state->classname_font);
//  g_object_unref (state->abstract_classname_font);
//  g_object_unref (state->comment_font);

    g_free(state->name);
//  g_free(state->stereotype);
//  g_free(state->comment);

//  list = state->attributes;
//  while (list) {
//    struct_attribute_destroy((STRUCTAttribute *) list->data);
//    list = g_list_next(list);
//  }
//  g_list_free(state->attributes);

//  list = state->operations;
//  while (list) {
//    struct_operation_destroy((STRUCTOperation *) list->data);
//    list = g_list_next(list);
//  }
//  g_list_free(state->operations);
//
//  list = state->formal_params;
//  while (list) {
//    struct_formalparameter_destroy((STRUCTFormalParameter *) list->data);
//    list = g_list_next(list);
//  }
//  g_list_free(state->formal_params);
}

static STRUCTClassState *
structclass_get_state(STRUCTClass *structclass)
{
    STRUCTClassState *state = g_new0(STRUCTClassState, 1);
//    GList *list;

    state->font_height = structclass->font_height;
    state->abstract_font_height = structclass->abstract_font_height;
    state->polymorphic_font_height = structclass->polymorphic_font_height;
    state->classname_font_height = structclass->classname_font_height;
    state->abstract_classname_font_height = structclass->abstract_classname_font_height;
    state->comment_font_height = structclass->comment_font_height;

    state->normal_font = g_object_ref (structclass->normal_font);
//  state->abstract_font = g_object_ref (structclass->abstract_font);
//  state->polymorphic_font = g_object_ref (structclass->polymorphic_font);
    state->classname_font = g_object_ref (structclass->classname_font);
//  state->abstract_classname_font = g_object_ref (structclass->abstract_classname_font);
//  state->comment_font = g_object_ref (structclass->comment_font);

    state->name = g_strdup(structclass->name);
//  state->stereotype = g_strdup(structclass->stereotype);
//  state->comment = g_strdup(structclass->comment);
//
//  state->abstract = structclass->abstract;
//  state->suppress_attributes = structclass->suppress_attributes;
//  state->suppress_operations = structclass->suppress_operations;
//  state->visible_attributes = structclass->visible_attributes;
//  state->visible_operations = structclass->visible_operations;
//  state->visible_comments = structclass->visible_comments;
//
//  state->wrap_operations = structclass->wrap_operations;
//  state->wrap_after_char = structclass->wrap_after_char;
//  state->comment_line_length = structclass->comment_line_length;
//  state->comment_tagging = structclass->comment_tagging;

    state->line_color = structclass->line_color;
    state->fill_color = structclass->fill_color;
    state->text_color = structclass->text_color;

//  state->attributes = NULL;
//  list = structclass->attributes;
//  while (list != NULL) {
//    STRUCTAttribute *attr = (STRUCTAttribute *)list->data;
//    STRUCTAttribute *attr_copy;
//
//    attr_copy = struct_attribute_copy(attr);
//    /* Looks wrong, but needed fro proper restore */
//    attr_copy->left_connection = attr->left_connection;
//    attr_copy->right_connection = attr->right_connection;
//
//    state->attributes = g_list_append(state->attributes, attr_copy);
//    list = g_list_next(list);
//  }
//
//
//  state->operations = NULL;
//  list = structclass->operations;
//  while (list != NULL) {
//    STRUCTOperation *op = (STRUCTOperation *)list->data;
//    STRUCTOperation *op_copy;
//
//    op_copy = struct_operation_copy(op);
//    op_copy->left_connection = op->left_connection;
//    op_copy->right_connection = op->right_connection;
//    state->operations = g_list_append(state->operations, op_copy);
//    list = g_list_next(list);
//  }
//
//
//  state->template = structclass->template;
//
//  state->formal_params = NULL;
//  list = structclass->formal_params;
//  while (list != NULL) {
//    STRUCTFormalParameter *param = (STRUCTFormalParameter *)list->data;
//    STRUCTFormalParameter *param_copy;
//
//    param_copy = struct_formalparameter_copy(param);
//    state->formal_params = g_list_append(state->formal_params, param_copy);
//
//    list = g_list_next(list);
//  }

    return state;
}


static STRUCTClassState *
factory_get_state(STRUCTClass *structclass)
{
    STRUCTClassState *state = g_new0(STRUCTClassState, 1);
//    GList *list;

    state->font_height = structclass->font_height;
    state->abstract_font_height = structclass->abstract_font_height;
    state->polymorphic_font_height = structclass->polymorphic_font_height;
    state->classname_font_height = structclass->classname_font_height;
    state->abstract_classname_font_height = structclass->abstract_classname_font_height;
    state->comment_font_height = structclass->comment_font_height;

    state->normal_font = g_object_ref (structclass->normal_font);

    state->classname_font = g_object_ref (structclass->classname_font);


    state->name = g_strdup(structclass->name);

    state->line_color = structclass->line_color;
    state->fill_color = structclass->fill_color;
    state->text_color = structclass->text_color;


    return state;
}



static void
structclass_update_connectionpoints(STRUCTClass *structclass)
{
    int num_attrib, num_ops;
    DiaObject *obj;
//    GList *list;
    int connection_index;
    STRUCTClassDialog *prop_dialog;

    prop_dialog = structclass->properties_dialog;

    /* Allocate enought connection points for attributes and operations. */
    /* (two per op/attr) */
//  if ( (structclass->visible_attributes) && (!structclass->suppress_attributes))
//    num_attrib = g_list_length(structclass->attributes);
//  else
    num_attrib = 0;
//  if ( (structclass->visible_operations) && (!structclass->suppress_operations))
//    num_ops = g_list_length(structclass->operations);
//  else
    num_ops = 0;

    obj = &structclass->element.object;
#ifdef STRUCT_MAINPOINT
    obj->num_connections = STRUCTCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2 + 1;
#else
    obj->num_connections = STRUCTCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2;
#endif
    obj->connections =
        g_realloc(obj->connections,
                  obj->num_connections*sizeof(ConnectionPoint *));

    connection_index = STRUCTCLASS_CONNECTIONPOINTS;

//  list = structclass->attributes;
//  while (list != NULL) {
//    STRUCTAttribute *attr = (STRUCTAttribute *) list->data;
//
//    if ( (structclass->visible_attributes) &&
//	 (!structclass->suppress_attributes)) {
//      obj->connections[connection_index] = attr->left_connection;
//      connection_index++;
//      obj->connections[connection_index] = attr->right_connection;
//      connection_index++;
//    }
//
//    list = g_list_next(list);
//  }

//  if (prop_dialog)
//    gtk_list_clear_items (GTK_LIST (prop_dialog->attributes_list), 0, -1);

//  list = structclass->operations;
//  while (list != NULL) {
//    STRUCTOperation *op = (STRUCTOperation *) list->data;
//
//    if ( (structclass->visible_operations) &&
//	 (!structclass->suppress_operations)) {
//      obj->connections[connection_index] = op->left_connection;
//      connection_index++;
//      obj->connections[connection_index] = op->right_connection;
//      connection_index++;
//    }
//
//    list = g_list_next(list);
//  }
//  if (prop_dialog)
//    gtk_list_clear_items (GTK_LIST (prop_dialog->operations_list), 0, -1);

#ifdef STRUCT_MAINPOINT
    obj->connections[connection_index++] = &structclass->connections[STRUCTCLASS_CONNECTIONPOINTS];
#endif

}

static void
structclass_set_state(STRUCTClass *structclass, STRUCTClassState *state)
{
    structclass->font_height = state->font_height;
    structclass->abstract_font_height = state->abstract_font_height;
    structclass->polymorphic_font_height = state->polymorphic_font_height;
    structclass->classname_font_height = state->classname_font_height;
    structclass->abstract_classname_font_height = state->abstract_classname_font_height;
    structclass->comment_font_height = state->comment_font_height;

    /* transfer ownership, but don't leak the previous font */
    g_object_unref (structclass->normal_font);
    structclass->normal_font = state->normal_font;
//  g_object_unref (structclass->abstract_font);
//  structclass->abstract_font = state->abstract_font;
//  g_object_unref (structclass->polymorphic_font);
//  structclass->polymorphic_font = state->polymorphic_font;
    g_object_unref (structclass->classname_font);
    structclass->classname_font = state->classname_font;
//  g_object_unref (structclass->abstract_classname_font);
//  structclass->abstract_classname_font = state->abstract_classname_font;
//  g_object_unref (structclass->comment_font);
//  structclass->comment_font = state->comment_font;

    structclass->name = state->name;
//  structclass->stereotype = state->stereotype;
//  structclass->comment = state->comment;
//
//  structclass->abstract = state->abstract;
//  structclass->suppress_attributes = state->suppress_attributes;
//  structclass->suppress_operations = state->suppress_operations;
//  structclass->visible_attributes = state->visible_attributes;
//  structclass->visible_operations = state->visible_operations;
//  structclass->visible_comments = state->visible_comments;
//
//  structclass->wrap_operations = state->wrap_operations;
//  structclass->wrap_after_char = state->wrap_after_char;
//  structclass->comment_line_length = state->comment_line_length;
//  structclass->comment_tagging = state->comment_tagging;

    structclass->line_color = state->line_color;
    structclass->fill_color = state->fill_color;
    structclass->text_color = state->text_color;

//  structclass->attributes = state->attributes;
//  structclass->operations = state->operations;
//  structclass->template = state->template;
//  structclass->formal_params = state->formal_params;

    g_free(state);

    structclass_update_connectionpoints(structclass);

    structclass_calculate_data(structclass);
    structclass_update_data(structclass);
}

static void
structclass_change_apply(STRUCTClassChange *change, DiaObject *obj)
{
    STRUCTClassState *old_state;
//    GList *list;

    old_state = structclass_get_state(change->obj);

    structclass_set_state(change->obj, change->saved_state);
//
//  list = change->disconnected;
//  while (list) {
//    Disconnect *dis = (Disconnect *)list->data;
//
//    object_unconnect(dis->other_object, dis->other_handle);
//
//    list = g_list_next(list);
//  }

    change->saved_state = old_state;
    change->applied = 1;
}

static void
factory_change_apply(STRUCTClassChange *change, DiaObject *obj)
{
    STRUCTClassState *old_state;
//    GList *list;

    old_state = structclass_get_state(change->obj);

    structclass_set_state(change->obj, change->saved_state);
//
//  list = change->disconnected;
//  while (list) {
//    Disconnect *dis = (Disconnect *)list->data;
//
//    object_unconnect(dis->other_object, dis->other_handle);
//
//    list = g_list_next(list);
//  }

    change->saved_state = old_state;
    change->applied = 1;
}


static void
factory_change_revert(STRUCTClassChange *change, DiaObject *obj)
{
    STRUCTClassState *old_state;
//    GList *list;

    old_state = structclass_get_state(change->obj);

    structclass_set_state(change->obj, change->saved_state);

//  list = change->disconnected;
//  while (list) {
//    Disconnect *dis = (Disconnect *)list->data;
//
//    object_connect(dis->other_object, dis->other_handle, dis->cp);
//
//    list = g_list_next(list);
//  }

    change->saved_state = old_state;
    change->applied = 0;
}

static void
factory_change_free(STRUCTClassChange *change)
{
//    GList *list, *free_list;

    structclass_free_state(change->saved_state);
    g_free(change->saved_state);

    /* Doesn't this mean only one of add, delete can be done in each apply? */
//  if (change->applied)
//    free_list = change->deleted_cp;
//  else
//    free_list = change->added_cp;
//
//  list = free_list;
//  while (list != NULL) {
//    ConnectionPoint *connection = (ConnectionPoint *) list->data;
//
//    g_assert(connection->connected == NULL); /* Paranoid */
//    object_remove_connections_to(connection); /* Shouldn't be needed */
//    g_free(connection);
//
//    list = g_list_next(list);
//  }

//  g_list_free(free_list);

}
static ObjectChange *factory_new_change(STRUCTClass *obj, STRUCTClassState *saved_state)
{
    STRUCTClassChange *change;

    change = g_new0(STRUCTClassChange, 1);

    change->obj_change.apply =
        (ObjectChangeApplyFunc) factory_change_apply;
    change->obj_change.revert =
        (ObjectChangeRevertFunc) factory_change_revert;
    change->obj_change.free =
        (ObjectChangeFreeFunc) factory_change_free;

    change->obj = obj;
    change->saved_state = saved_state;
    change->applied = 1;

//  change->added_cp = added;
//  change->deleted_cp = deleted;
//  change->disconnected = disconnected;

    return (ObjectChange *)change;
}


//static ObjectChange *
//new_structclass_change(STRUCTClass *obj, STRUCTClassState *saved_state,
//		    GList *added, GList *deleted, GList *disconnected)
//{
//  STRUCTClassChange *change;
//
//  change = g_new0(STRUCTClassChange, 1);
//
//  change->obj_change.apply =
//    (ObjectChangeApplyFunc) structclass_change_apply;
//  change->obj_change.revert =
//    (ObjectChangeRevertFunc) structclass_change_revert;
//  change->obj_change.free =
//    (ObjectChangeFreeFunc) structclass_change_free;
//
//  change->obj = obj;
//  change->saved_state = saved_state;
//  change->applied = 1;
//
//  change->added_cp = added;
//  change->deleted_cp = deleted;
//  change->disconnected = disconnected;
//
//  return (ObjectChange *)change;
//}
/*
        get the contents of a comment text view.
*/
//const gchar * get_comment(GtkTextView *view)
//{
//    GtkTextBuffer * buffer = gtk_text_view_get_buffer(view);
//    GtkTextIter start;
//    GtkTextIter end;
//
//    gtk_text_buffer_get_start_iter(buffer, &start);
//    gtk_text_buffer_get_end_iter(buffer, &end);
//
//    return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
//}
//
//void
//set_comment(GtkTextView *view, gchar *text)
//{
//    GtkTextBuffer * buffer = gtk_text_view_get_buffer(view);
//    GtkTextIter start;
//    GtkTextIter end;
//
//    gtk_text_buffer_get_start_iter(buffer, &start);
//    gtk_text_buffer_get_end_iter(buffer, &end);
//    gtk_text_buffer_delete(buffer,&start,&end);
//    gtk_text_buffer_get_start_iter(buffer, &start);
//    gtk_text_buffer_insert( buffer, &start, text, strlen(text));
//}


