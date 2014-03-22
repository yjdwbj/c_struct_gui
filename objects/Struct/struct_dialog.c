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

#include "object.h"
#include "objchange.h"
#include "intl.h"
#include "struct_class.h"
#include "sheet.h"



/* hide this functionality before rewrite;) */
void
structclass_dialog_free (STRUCTClassDialog *dialog)
{
  g_list_free(dialog->deleted_connections);
  gtk_widget_destroy(dialog->dialog);
  dialog->dialog = NULL;
  gtk_widget_destroy(dialog->mainTable); // 2014-3-19 lcy �����ǻ����ڴ�.
  g_list_free(dialog->EnumsAndStructs->enumList);
  g_list_free(dialog->EnumsAndStructs->structList);
  /* destroy-signal destroy_properties_dialog already does 'g_free(dialog);' and more */
}

typedef struct _Disconnect {
  ConnectionPoint *cp;
  DiaObject *other_object;
  Handle *other_handle;
} Disconnect;

typedef struct _STRUCTClassState STRUCTClassState;

struct _STRUCTClassState {
  real font_height;
  real abstract_font_height;
  real polymorphic_font_height;
  real classname_font_height;
  real abstract_classname_font_height;
  real comment_font_height;

  DiaFont *normal_font;
  DiaFont *abstract_font;
  DiaFont *polymorphic_font;
  DiaFont *classname_font;
  DiaFont *abstract_classname_font;
  DiaFont *comment_font;

  char *name;
  char *stereotype;
  char *comment;

  int abstract;
  int suppress_attributes;
  int suppress_operations;
  int visible_attributes;
  int visible_operations;
  int visible_comments;

  int wrap_operations;
  int wrap_after_char;
  int comment_line_length;
  int comment_tagging;

  real line_width;
  Color line_color;
  Color fill_color;
  Color text_color;

  /* Attributes: */
  GList *attributes;

  /* Operators: */
  GList *operations;

  /* Template: */
  int template;
  GList *formal_params;
};


typedef struct _STRUCTClassChange STRUCTClassChange;

struct _STRUCTClassChange {
  ObjectChange obj_change;

  STRUCTClass *obj;

  GList *added_cp;
  GList *deleted_cp;
  GList *disconnected;

  int applied;

  STRUCTClassState *saved_state;
};

static STRUCTClassState *structclass_get_state(STRUCTClass *structclass);
static ObjectChange *new_structclass_change(STRUCTClass *obj, STRUCTClassState *saved_state,
					 GList *added, GList *deleted,
					 GList *disconnected);
static  const gchar *get_comment(GtkTextView *);
static void set_comment(GtkTextView *, gchar *);

/**** Utility functions ******/
static void
structclass_store_disconnects(STRUCTClassDialog *prop_dialog,
			   ConnectionPoint *cp)
{
  Disconnect *dis;
  DiaObject *connected_obj;
  GList *list;
  int i;

  list = cp->connected;
  while (list != NULL) {
    connected_obj = (DiaObject *)list->data;

    for (i=0;i<connected_obj->num_handles;i++) {
      if (connected_obj->handles[i]->connected_to == cp) {
	dis = g_new0(Disconnect, 1);
	dis->cp = cp;
	dis->other_object = connected_obj;
	dis->other_handle = connected_obj->handles[i];

	prop_dialog->disconnected_connections =
	  g_list_prepend(prop_dialog->disconnected_connections, dis);
      }
    }
    list = g_list_next(list);
  }
}

/** Add an option to an option menu item for a class.
 * @param menu The GtkMenu to add an item to.
 * @param label The I18N'd label to show in the menu.
 * @param structclass The class object that the dialog is being built for.
 * @param user_data Arbitrary data, here typically an integer indicating the
 * option internally.
 */
static void
add_option_menu_item(GtkMenu *menu, gchar *label, GtkSignalFunc update_func,
		     STRUCTClass *structclass, gpointer user_data)
{
  GtkWidget *menuitem = gtk_menu_item_new_with_label (label);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      update_func, structclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem), user_data);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
}

/********************************************************
 ******************** CLASS *****************************
 ********************************************************/

static void
class_read_from_dialog(STRUCTClass *structclass, STRUCTClassDialog *prop_dialog)
{
  const gchar *s;

  if (structclass->name != NULL)
    g_free(structclass->name);

  s = gtk_entry_get_text (prop_dialog->classname);
  if (s && s[0])
    structclass->name = g_strdup (s);
  else
    structclass->name = NULL;

  if (structclass->stereotype != NULL)
    g_free(structclass->stereotype);

  s = gtk_entry_get_text(prop_dialog->stereotype);
  if (s && s[0])
    structclass->stereotype = g_strdup (s);
  else
    structclass->stereotype = NULL;

  if (structclass->comment != NULL)
    g_free (structclass->comment);

  s = get_comment(prop_dialog->comment);
  if (s && s[0])
    structclass->comment = g_strdup (s);
  else
    structclass->comment = NULL;

  structclass->abstract = prop_dialog->abstract_class->active;
  structclass->visible_attributes = prop_dialog->attr_vis->active;
  structclass->visible_operations = prop_dialog->op_vis->active;
  structclass->wrap_operations = prop_dialog->op_wrap->active;
  structclass->wrap_after_char = gtk_spin_button_get_value_as_int(prop_dialog->wrap_after_char);
  structclass->comment_line_length = gtk_spin_button_get_value_as_int(prop_dialog->comment_line_length);
  structclass->comment_tagging = prop_dialog->comment_tagging->active;
  structclass->visible_comments = prop_dialog->comments_vis->active;
  structclass->suppress_attributes = prop_dialog->attr_supp->active;
  structclass->suppress_operations = prop_dialog->op_supp->active;
  structclass->line_width = gtk_spin_button_get_value_as_float(prop_dialog->line_width);
  dia_color_selector_get_color(GTK_WIDGET(prop_dialog->text_color), &structclass->text_color);
  dia_color_selector_get_color(GTK_WIDGET(prop_dialog->line_color), &structclass->line_color);
  dia_color_selector_get_color(GTK_WIDGET(prop_dialog->fill_color), &structclass->fill_color);

  structclass->normal_font = dia_font_selector_get_font (prop_dialog->normal_font);
  structclass->polymorphic_font = dia_font_selector_get_font (prop_dialog->polymorphic_font);
  structclass->abstract_font = dia_font_selector_get_font (prop_dialog->abstract_font);
  structclass->classname_font = dia_font_selector_get_font (prop_dialog->classname_font);
  structclass->abstract_classname_font = dia_font_selector_get_font (prop_dialog->abstract_classname_font);
  structclass->comment_font = dia_font_selector_get_font (prop_dialog->comment_font);

  structclass->font_height = gtk_spin_button_get_value_as_float (prop_dialog->normal_font_height);
  structclass->abstract_font_height = gtk_spin_button_get_value_as_float (prop_dialog->abstract_font_height);
  structclass->polymorphic_font_height = gtk_spin_button_get_value_as_float (prop_dialog->polymorphic_font_height);
  structclass->classname_font_height = gtk_spin_button_get_value_as_float (prop_dialog->classname_font_height);
  structclass->abstract_classname_font_height = gtk_spin_button_get_value_as_float (prop_dialog->abstract_classname_font_height);
  structclass->comment_font_height = gtk_spin_button_get_value_as_float (prop_dialog->comment_font_height);
}

static void
class_fill_in_dialog(STRUCTClass *structclass)
{
  STRUCTClassDialog *prop_dialog;

  prop_dialog = structclass->properties_dialog;

  if (structclass->name)
    gtk_entry_set_text(prop_dialog->classname, structclass->name);
  if (structclass->stereotype != NULL)
    gtk_entry_set_text(prop_dialog->stereotype, structclass->stereotype);
  else
    gtk_entry_set_text(prop_dialog->stereotype, "");

  if (structclass->comment != NULL)
    set_comment(prop_dialog->comment, structclass->comment);
  else
    set_comment(prop_dialog->comment, "");

  gtk_toggle_button_set_active(prop_dialog->abstract_class, structclass->abstract);
  gtk_toggle_button_set_active(prop_dialog->attr_vis, structclass->visible_attributes);
  gtk_toggle_button_set_active(prop_dialog->op_vis, structclass->visible_operations);
  gtk_toggle_button_set_active(prop_dialog->op_wrap, structclass->wrap_operations);
  gtk_spin_button_set_value (prop_dialog->wrap_after_char, structclass->wrap_after_char);
  gtk_spin_button_set_value (prop_dialog->comment_line_length, structclass->comment_line_length);
  gtk_toggle_button_set_active(prop_dialog->comment_tagging, structclass->comment_tagging);
  gtk_toggle_button_set_active(prop_dialog->comments_vis, structclass->visible_comments);
  gtk_toggle_button_set_active(prop_dialog->attr_supp, structclass->suppress_attributes);
  gtk_toggle_button_set_active(prop_dialog->op_supp, structclass->suppress_operations);
  gtk_spin_button_set_value (prop_dialog->line_width, structclass->line_width);
  dia_color_selector_set_color(GTK_WIDGET(prop_dialog->text_color), &structclass->text_color);
  dia_color_selector_set_color(GTK_WIDGET(prop_dialog->line_color), &structclass->line_color);
  dia_color_selector_set_color(GTK_WIDGET(prop_dialog->fill_color), &structclass->fill_color);
  dia_font_selector_set_font (prop_dialog->normal_font, structclass->normal_font);
  dia_font_selector_set_font (prop_dialog->polymorphic_font, structclass->polymorphic_font);
  dia_font_selector_set_font (prop_dialog->abstract_font, structclass->abstract_font);
  dia_font_selector_set_font (prop_dialog->classname_font, structclass->classname_font);
  dia_font_selector_set_font (prop_dialog->abstract_classname_font, structclass->abstract_classname_font);
  dia_font_selector_set_font (prop_dialog->comment_font, structclass->comment_font);
  gtk_spin_button_set_value (prop_dialog->normal_font_height, structclass->font_height);
  gtk_spin_button_set_value (prop_dialog->polymorphic_font_height, structclass->polymorphic_font_height);
  gtk_spin_button_set_value (prop_dialog->abstract_font_height, structclass->abstract_font_height);
  gtk_spin_button_set_value (prop_dialog->classname_font_height, structclass->classname_font_height);
  gtk_spin_button_set_value (prop_dialog->abstract_classname_font_height, structclass->abstract_classname_font_height);
  gtk_spin_button_set_value (prop_dialog->comment_font_height, structclass->comment_font_height);
}

static void
create_font_props_row (GtkTable   *table,
                       const char *kind,
                       gint        row,
                       DiaFont    *font,
                       real        height,
                       DiaFontSelector **fontsel,
                       GtkSpinButton   **heightsel)
{
  GtkWidget *label;
  GtkObject *adj;

  label = gtk_label_new (kind);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (table, label, 0, 1, row, row+1);
  *fontsel = DIAFONTSELECTOR (dia_font_selector_new ());
  dia_font_selector_set_font (DIAFONTSELECTOR (*fontsel), font);
  gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET(*fontsel), 1, 2, row, row+1);

  adj = gtk_adjustment_new (height, 0.1, 10.0, 0.1, 1.0, 0);
  *heightsel = GTK_SPIN_BUTTON (gtk_spin_button_new (GTK_ADJUSTMENT(adj), 1.0, 2));
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (*heightsel), TRUE);
  gtk_table_attach_defaults (table, GTK_WIDGET (*heightsel), 2, 3, row, row+1);
}



static void
factory_create_struct_dialog(STRUCTClass *class, FactoryStructItem *item,int row )   // �����ṹ���ÿһ����.
{
  STRUCTClassDialog *dialog = class->properties_dialog;
  GtkWidget *itemName;
  GtkWidget *columTwo;
  GtkTooltips *tool_tips;
  tool_tips = gtk_tooltips_new();
  GList *enumList = dialog->EnumsAndStructs->enumList;
  CellType isEnum = ENTRY;
    for (;enumList != NULL; enumList = enumList->next)
       {

           FactoryStructEnumList *fset = enumList->data;
           gchar **split = g_strsplit(item->itemType,".",-1);
           int section = g_strv_length(split);
           if(!g_ascii_strncasecmp(split[section-1],fset->name,strlen(item->itemType))) // 2014-3-21 lcy �Ա����һ����ö������
           {
               isEnum = ENUM;
              columTwo = gtk_combo_box_new_text();
              gtk_combo_box_popdown(GTK_COMBO_BOX(columTwo));

              GList *tenum = fset->list;
              for(;tenum != NULL; tenum = tenum->next)
              {
                  FactoryStructEnum *o = tenum->data;
                  gtk_combo_box_append_text(GTK_COMBO_BOX(columTwo),o->key);
              }
              gtk_combo_box_set_active(GTK_COMBO_BOX(columTwo),0);
                break;
           }
           g_strfreev(split);

       }

       if(isEnum != ENUM) // 2014-3-20 lcy ���ǲ���combobox.
       {
            columTwo = gtk_entry_new();
            gtk_entry_set_text(GTK_ENTRY(columTwo),item->itemValue);  // set default value;

       }
        gtk_tooltips_set_tip(tool_tips,columTwo,_(item->itemComment),NULL);

  itemName = gtk_label_new(item->itemCname);
  gtk_tooltips_set_tip(tool_tips,itemName,_(item->itemComment),NULL);
  if(  row == 0 || !(row % 2 ))
  {
        gtk_table_attach_defaults(GTK_TABLE(dialog->mainTable),itemName,0,1,row,row+1);
        gtk_table_attach_defaults(GTK_TABLE(dialog->mainTable),columTwo,1,2,row,row+1);
  }else
  {
        gtk_table_attach_defaults(GTK_TABLE(dialog->mainTable),itemName,2,3,row-1,row); // 2014-3-20 lcy ������
        gtk_table_attach_defaults(GTK_TABLE(dialog->mainTable),columTwo,3,4,row-1,row);
  }

  WidgetAndValue wav;
  wav.widget = columTwo;
  wav.name = item->itemName;
  wav.type = item->itemType;
  wav.value = item->itemValue;
  wav.celltype = isEnum;
  class->widgetmap = g_list_append(class->widgetmap,&wav);
  gtk_container_add(GTK_OBJECT(dialog->dialog),dialog->mainTable);
}




static void
class_create_page(GtkNotebook *notebook,  STRUCTClass *structclass)
{
  STRUCTClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *scrolledwindow;
  GtkWidget *checkbox;
  GtkWidget *table;
  GtkObject *adj;

  prop_dialog = structclass->properties_dialog;

  /* Class page: */
  page_label = gtk_label_new_with_mnemonic (_("_Class"));

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10); // ���һ��label , ��lable �ϲ��ֿؼ�, Ȼ��ӵ�table ��ȥ

  table = gtk_table_new (3, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Class name:"));
  entry = gtk_entry_new();
  prop_dialog->classname = GTK_ENTRY(entry);
  gtk_widget_grab_focus(entry);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Stereotype:"));
  entry = gtk_entry_new();
  prop_dialog->stereotype = GTK_ENTRY(entry);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Comment:"));
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_table_attach (GTK_TABLE (table), scrolledwindow, 1, 2, 2, 3,
		    (GtkAttachOptions) (GTK_FILL),
		    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_IN);
  entry = gtk_text_view_new ();
  prop_dialog->comment = GTK_TEXT_VIEW(entry);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);

  hbox = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Abstract"));
  prop_dialog->abstract_class = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Attributes visible"));
  prop_dialog->attr_vis = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  checkbox = gtk_check_button_new_with_label(_("Suppress Attributes"));
  prop_dialog->attr_supp = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Operations visible"));
  prop_dialog->op_vis = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  checkbox = gtk_check_button_new_with_label(_("Suppress operations"));
  prop_dialog->op_supp = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox  = gtk_hbox_new(TRUE, 5);
  hbox2 = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Wrap Operations"));
  prop_dialog->op_wrap = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  adj = gtk_adjustment_new( structclass->wrap_after_char, 0.0, 200.0, 1.0, 5.0, 0);
  prop_dialog->wrap_after_char = GTK_SPIN_BUTTON(gtk_spin_button_new( GTK_ADJUSTMENT( adj), 0.1, 0));
  gtk_spin_button_set_numeric( GTK_SPIN_BUTTON( prop_dialog->wrap_after_char), TRUE);
  gtk_spin_button_set_snap_to_ticks( GTK_SPIN_BUTTON( prop_dialog->wrap_after_char), TRUE);
  prop_dialog->max_length_label = GTK_LABEL( gtk_label_new( _("Wrap after this length: ")));
  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET( prop_dialog->max_length_label), FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET( prop_dialog->wrap_after_char), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET( hbox2), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(TRUE, 5);
  hbox2 = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Comments visible"));
  prop_dialog->comments_vis = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  adj = gtk_adjustment_new( structclass->comment_line_length, 17.0, 200.0, 1.0, 5.0, 0);
  prop_dialog->comment_line_length = GTK_SPIN_BUTTON(gtk_spin_button_new( GTK_ADJUSTMENT( adj), 0.1, 0));
  gtk_spin_button_set_numeric( GTK_SPIN_BUTTON( prop_dialog->comment_line_length), TRUE);
  gtk_spin_button_set_snap_to_ticks( GTK_SPIN_BUTTON( prop_dialog->comment_line_length), TRUE);
  prop_dialog->Comment_length_label = GTK_LABEL( gtk_label_new( _("Wrap comment after this length: ")));
  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET( prop_dialog->Comment_length_label), FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET( prop_dialog->comment_line_length), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox),  GTK_WIDGET( hbox2), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),  hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Show documentation tag111111"));
  prop_dialog->comment_tagging = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page(notebook, vbox, page_label);

}


static void
style_create_page(GtkNotebook *notebook,  STRUCTClass *structclass)
{
  STRUCTClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *line_width;
  GtkWidget *text_color;
  GtkWidget *fill_color;
  GtkWidget *line_color;
  GtkWidget *table;
  GtkObject *adj;

  prop_dialog = structclass->properties_dialog;

  /** Fonts and Colors selection **/
  page_label = gtk_label_new_with_mnemonic (_("_Style"));

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  table = gtk_table_new (5, 6, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
  gtk_table_set_homogeneous (GTK_TABLE (table), FALSE);

  /* head line */
  label = gtk_label_new (_("Kind"));
                                                    /* L, R, T, B */
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  label = gtk_label_new (_("Font"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
  label = gtk_label_new (_("Size"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);

  /* property rows */
  create_font_props_row (GTK_TABLE (table), _("Normal"), 1,
                         structclass->normal_font,
                         structclass->font_height,
                         &(prop_dialog->normal_font),
                         &(prop_dialog->normal_font_height));
  create_font_props_row (GTK_TABLE (table), _("Polymorphic"), 2,
                         structclass->polymorphic_font,
                         structclass->polymorphic_font_height,
                         &(prop_dialog->polymorphic_font),
                         &(prop_dialog->polymorphic_font_height));
  create_font_props_row (GTK_TABLE (table), _("Abstract"), 3,
                         structclass->abstract_font,
                         structclass->abstract_font_height,
                         &(prop_dialog->abstract_font),
                         &(prop_dialog->abstract_font_height));
  create_font_props_row (GTK_TABLE (table), _("Class Name"), 4,
                         structclass->classname_font,
                         structclass->classname_font_height,
                         &(prop_dialog->classname_font),
                         &(prop_dialog->classname_font_height));
  create_font_props_row (GTK_TABLE (table), _("Abstract Class"), 5,
                         structclass->abstract_classname_font,
                         structclass->abstract_classname_font_height,
                         &(prop_dialog->abstract_classname_font),
                         &(prop_dialog->abstract_classname_font_height));
  create_font_props_row (GTK_TABLE (table), _("Comment"), 6,
                         structclass->comment_font,
                         structclass->comment_font_height,
                         &(prop_dialog->comment_font),
                         &(prop_dialog->comment_font_height));



  table = gtk_table_new (2, 4, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox),
		      table, FALSE, TRUE, 0);
  /* should probably be refactored too. */
  label = gtk_label_new(_("Line Width"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 2);
  adj = gtk_adjustment_new(structclass->line_width, 0.0, G_MAXFLOAT, 0.1, 1.0, 0);
  line_width = gtk_spin_button_new (GTK_ADJUSTMENT(adj), 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (line_width), TRUE);
  prop_dialog->line_width = GTK_SPIN_BUTTON(line_width);
  gtk_table_attach (GTK_TABLE (table), line_width, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 3, 2);

  label = gtk_label_new(_("Text Color"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 2);
  text_color = dia_color_selector_new();
  dia_color_selector_set_color(text_color, &structclass->text_color);
  prop_dialog->text_color = (DiaColorSelector *)text_color;
  gtk_table_attach (GTK_TABLE (table), text_color, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 3, 2);

  label = gtk_label_new(_("Foreground Color"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 2);
  line_color = dia_color_selector_new();
  dia_color_selector_set_color(line_color, &structclass->line_color);
  prop_dialog->line_color = (DiaColorSelector *)line_color;
  gtk_table_attach (GTK_TABLE (table), line_color, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 3, 2);

  label = gtk_label_new(_("Background Color"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 2);
  fill_color = dia_color_selector_new();
  dia_color_selector_set_color(fill_color, &structclass->fill_color);
  prop_dialog->fill_color = (DiaColorSelector *)fill_color;
  gtk_table_attach (GTK_TABLE (table), fill_color, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, 0, 3, 2);

  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page(notebook, vbox, page_label);

}


/************************************************************
 ******************** ATTRIBUTES ****************************
 ************************************************************/

static void
attributes_set_sensitive(STRUCTClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_name), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_type), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_value), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_comment), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_visible_button), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_visible), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_class_scope), val);
}

static void
attributes_set_values(STRUCTClassDialog *prop_dialog, STRUCTAttribute *attr)
{
  gtk_entry_set_text(prop_dialog->attr_name, attr->name);
  gtk_entry_set_text(prop_dialog->attr_type, attr->type);
  if (attr->value != NULL)
    gtk_entry_set_text(prop_dialog->attr_value, attr->value);
  else
    gtk_entry_set_text(prop_dialog->attr_value, "");

  if (attr->comment != NULL)
    set_comment(prop_dialog->attr_comment, attr->comment);
  else
    set_comment(prop_dialog->attr_comment, "");


  gtk_option_menu_set_history(prop_dialog->attr_visible_button,
			      (gint)attr->visibility);
  gtk_toggle_button_set_active(prop_dialog->attr_class_scope, attr->class_scope);
}

static void
attributes_clear_values(STRUCTClassDialog *prop_dialog)
{
  gtk_entry_set_text(prop_dialog->attr_name, "");
  gtk_entry_set_text(prop_dialog->attr_type, "");
  gtk_entry_set_text(prop_dialog->attr_value, "");
  set_comment(prop_dialog->attr_comment, "");
  gtk_toggle_button_set_active(prop_dialog->attr_class_scope, FALSE);
}

static void
attributes_get_values (STRUCTClassDialog *prop_dialog, STRUCTAttribute *attr)
{
  g_free (attr->name);
  g_free (attr->type);
  if (attr->value != NULL)
    g_free (attr->value);

  attr->name = g_strdup (gtk_entry_get_text (prop_dialog->attr_name));
  attr->type = g_strdup (gtk_entry_get_text (prop_dialog->attr_type));

  attr->value = g_strdup (gtk_entry_get_text(prop_dialog->attr_value));
  attr->comment = g_strdup (get_comment(prop_dialog->attr_comment));

  attr->visibility = (STRUCTVisibility)
		GPOINTER_TO_INT (gtk_object_get_user_data (
					 GTK_OBJECT (gtk_menu_get_active (prop_dialog->attr_visible))));

  attr->class_scope = prop_dialog->attr_class_scope->active;
}

static void
attributes_get_current_values(STRUCTClassDialog *prop_dialog)
{
  STRUCTAttribute *current_attr;
  GtkLabel *label;
  char *new_str;

  if (prop_dialog != NULL && prop_dialog->current_attr != NULL) {
    current_attr = (STRUCTAttribute *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_attr));
    if (current_attr != NULL) {
      attributes_get_values(prop_dialog, current_attr);
      label = GTK_LABEL(GTK_BIN(prop_dialog->current_attr)->child);
      new_str = struct_get_attribute_string(current_attr);
      gtk_label_set_text (label, new_str);
      g_free (new_str);
    }
  }
}

static void
attribute_list_item_destroy_callback(GtkWidget *list_item,
				     gpointer data)
{
  STRUCTAttribute *attr;

  attr = (STRUCTAttribute *) gtk_object_get_user_data(GTK_OBJECT(list_item));

  if (attr != NULL) {
    struct_attribute_destroy(attr);
    /*printf("Destroying list_item's user_data!\n");*/
  }
}

static void
attributes_list_selection_changed_callback(GtkWidget *gtklist,
					   STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkObject *list_item;
  STRUCTAttribute *attr;

  /* Due to GtkList oddities, this may get called during destroy.
   * But it'll reference things that are already dead and crash.
   * Thus, we stop it before it gets that bad.  See bug #156706 for
   * one example.
   */
  if (structclass->destroyed)
    return;

  prop_dialog = structclass->properties_dialog;

  if (!prop_dialog)
    return;

  attributes_get_current_values(prop_dialog);

  list = GTK_LIST(gtklist)->selection;
  if (!list && prop_dialog) { /* No selected */
    attributes_set_sensitive(prop_dialog, FALSE);
    attributes_clear_values(prop_dialog);
    prop_dialog->current_attr = NULL;
    return;
  }

  list_item = GTK_OBJECT(list->data);
  attr = (STRUCTAttribute *)gtk_object_get_user_data(list_item);
  attributes_set_values(prop_dialog, attr);
  attributes_set_sensitive(prop_dialog, TRUE);

  prop_dialog->current_attr = GTK_LIST_ITEM(list_item);
  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->attr_name));
}

static void
attributes_list_new_callback(GtkWidget *button,
			     STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkWidget *list_item;
  STRUCTAttribute *attr;
  char *utfstr;
  prop_dialog = structclass->properties_dialog;

  attributes_get_current_values(prop_dialog);

  attr = struct_attribute_new();
  /* need to make the new ConnectionPoint valid and remember them */
  struct_attribute_ensure_connection_points (attr, &structclass->element.object);
  prop_dialog->added_connections =
    g_list_prepend(prop_dialog->added_connections, attr->left_connection);
  prop_dialog->added_connections =
    g_list_prepend(prop_dialog->added_connections, attr->right_connection);

  utfstr = struct_get_attribute_string (attr);
  list_item = gtk_list_item_new_with_label (utfstr);
  gtk_widget_show (list_item);
  g_free (utfstr);

  gtk_object_set_user_data(GTK_OBJECT(list_item), attr);
  gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
		      GTK_SIGNAL_FUNC (attribute_list_item_destroy_callback),
		      NULL);

  list = g_list_append(NULL, list_item);
  gtk_list_append_items(prop_dialog->attributes_list, list);

  if (prop_dialog->attributes_list->children != NULL)
    gtk_list_unselect_child(prop_dialog->attributes_list,
			    GTK_WIDGET(prop_dialog->attributes_list->children->data));
  gtk_list_select_child(prop_dialog->attributes_list, list_item);
}

static void
attributes_list_delete_callback(GtkWidget *button,
				STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkList *gtklist;
  STRUCTAttribute *attr;

  prop_dialog = structclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->attributes_list);

  if (gtklist->selection != NULL) {
    attr = (STRUCTAttribute *)
      gtk_object_get_user_data(GTK_OBJECT(gtklist->selection->data));

    if (attr->left_connection != NULL) {
      prop_dialog->deleted_connections =
	g_list_prepend(prop_dialog->deleted_connections,
		       attr->left_connection);
      prop_dialog->deleted_connections =
	g_list_prepend(prop_dialog->deleted_connections,
		       attr->right_connection);
    }

    list = g_list_prepend(NULL, gtklist->selection->data);
    gtk_list_remove_items(gtklist, list);
    g_list_free(list);
    attributes_clear_values(prop_dialog);
    attributes_set_sensitive(prop_dialog, FALSE);
  }
}

static void
attributes_list_move_up_callback(GtkWidget *button,
				 STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;

  prop_dialog = structclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->attributes_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);

    i = gtk_list_child_position(gtklist, list_item);
    if (i>0)
      i--;

    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}

static void
attributes_list_move_down_callback(GtkWidget *button,
				   STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;

  prop_dialog = structclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->attributes_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);

    i = gtk_list_child_position(gtklist, list_item);
    if (i<(g_list_length(gtklist->children)-1))
      i++;


    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}

static void
attributes_read_from_dialog(STRUCTClass *structclass,
			    STRUCTClassDialog *prop_dialog,
			    int connection_index)
{
  GList *list;
  STRUCTAttribute *attr;
  GtkWidget *list_item;
  GList *clear_list;
  DiaObject *obj;

  obj = &structclass->element.object;

  /* if the currently select attribute is changed, update the state in the
   * dialog info from widgets */
  attributes_get_current_values(prop_dialog);
  /* Free current attributes: */
  list = structclass->attributes;
  while (list != NULL) {
    attr = (STRUCTAttribute *)list->data;
    struct_attribute_destroy(attr);
    list = g_list_next(list);
  }
  g_list_free (structclass->attributes);
  structclass->attributes = NULL;

  /* Insert new attributes and remove them from gtklist: */
  list = GTK_LIST (prop_dialog->attributes_list)->children;
  clear_list = NULL;
  while (list != NULL) {
    list_item = GTK_WIDGET(list->data);

    clear_list = g_list_prepend (clear_list, list_item);
    attr = (STRUCTAttribute *)
      gtk_object_get_user_data(GTK_OBJECT(list_item));
    gtk_object_set_user_data(GTK_OBJECT(list_item), NULL);
    structclass->attributes = g_list_append(structclass->attributes, attr);

    if (attr->left_connection == NULL) {
      struct_attribute_ensure_connection_points (attr, obj);

      prop_dialog->added_connections =
	g_list_prepend(prop_dialog->added_connections,
		       attr->left_connection);
      prop_dialog->added_connections =
	g_list_prepend(prop_dialog->added_connections,
		       attr->right_connection);
    }

    if ( (prop_dialog->attr_vis->active) &&
	 (!prop_dialog->attr_supp->active) ) {
      obj->connections[connection_index] = attr->left_connection;
      connection_index++;
      obj->connections[connection_index] = attr->right_connection;
      connection_index++;
    } else {
      structclass_store_disconnects(prop_dialog, attr->left_connection);
      object_remove_connections_to(attr->left_connection);
      structclass_store_disconnects(prop_dialog, attr->right_connection);
      object_remove_connections_to(attr->right_connection);
    }

    list = g_list_next(list);
  }
  clear_list = g_list_reverse (clear_list);
  gtk_list_remove_items (GTK_LIST (prop_dialog->attributes_list), clear_list);
  g_list_free (clear_list);

#if 0 /* STRUCTClass is *known* to be in an incositent state here, check later or crash ... */
  structclass_sanity_check(structclass, "Read from dialog");
#endif
}


static void
attributes_fill_in_dialog(STRUCTClass *structclass)
{
  STRUCTClassDialog *prop_dialog;
  STRUCTAttribute *attr_copy;
  GtkWidget *list_item;
  GList *list;
  int i;

#ifdef DEBUG
  structclass_sanity_check(structclass, "Filling in dialog");
#endif

  prop_dialog = structclass->properties_dialog;

  /* copy in new attributes: */
  if (prop_dialog->attributes_list->children == NULL) {
    i = 0;
    list = structclass->attributes;
    while (list != NULL) {
      STRUCTAttribute *attr = (STRUCTAttribute *)list->data;
      gchar *attrstr = struct_get_attribute_string(attr);

      list_item = gtk_list_item_new_with_label (attrstr);
      attr_copy = struct_attribute_copy(attr);
      /* looks wrong but required for complicated ConnectionPoint memory management */
      attr_copy->left_connection = attr->left_connection;
      attr_copy->right_connection = attr->right_connection;
      gtk_object_set_user_data(GTK_OBJECT(list_item), (gpointer) attr_copy);
      gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
			  GTK_SIGNAL_FUNC (attribute_list_item_destroy_callback),
			  NULL);
      gtk_container_add (GTK_CONTAINER (prop_dialog->attributes_list), list_item);
      gtk_widget_show (list_item);

      list = g_list_next(list); i++;
      g_free (attrstr);
    }
    /* set attributes non-sensitive */
    prop_dialog->current_attr = NULL;
    attributes_set_sensitive(prop_dialog, FALSE);
    attributes_clear_values(prop_dialog);
  }
}

static void
attributes_update(GtkWidget *widget, STRUCTClass *structclass)
{
  attributes_get_current_values(structclass->properties_dialog);
}

static int
attributes_update_event(GtkWidget *widget, GdkEventFocus *ev, STRUCTClass *structclass)
{
  attributes_get_current_values(structclass->properties_dialog);
  return 0;
}

static void
attributes_create_page(GtkNotebook *notebook,  STRUCTClass *structclass)
{
  STRUCTClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *table;
  GtkWidget *entry;
  GtkWidget *checkbox;
  GtkWidget *scrolled_win;
  GtkWidget *button;
  GtkWidget *list;
  GtkWidget *frame;
  GtkWidget *omenu;
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *scrolledwindow;
  GSList *group;

  prop_dialog = structclass->properties_dialog;

  /* Attributes page: */
  page_label = gtk_label_new_with_mnemonic (_("_Attributes"));

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  hbox = gtk_hbox_new(FALSE, 5);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  list = gtk_list_new ();
  prop_dialog->attributes_list = GTK_LIST(list);
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (list);

  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
		      GTK_SIGNAL_FUNC(attributes_list_selection_changed_callback),
		      structclass);

  vbox2 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(attributes_list_new_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(attributes_list_delete_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(attributes_list_move_up_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(attributes_list_move_down_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new(_("Attribute data"));
  vbox2 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 10);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show(frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

  table = gtk_table_new (5, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->attr_name = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (attributes_update_event), structclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), structclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  prop_dialog->attr_type = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (attributes_update_event), structclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), structclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Value:"));
  entry = gtk_entry_new();
  prop_dialog->attr_value = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (attributes_update_event), structclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), structclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Comment:"));
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_IN);
  entry = gtk_text_view_new ();
  prop_dialog->attr_comment = GTK_TEXT_VIEW(entry);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (entry),TRUE);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (attributes_update_event), structclass);
#if 0 /* while the GtkEntry has a "activate" signal, GtkTextView does not.
       * Maybe we should connect to "set-focus-child" instead?
       */
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), structclass);
#endif
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,3,4, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), scrolledwindow, 1,2,3,4, GTK_FILL | GTK_EXPAND,0, 0,2);


  label = gtk_label_new(_("Visibility:"));

  omenu = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  prop_dialog->attr_visible = GTK_MENU(menu);
  prop_dialog->attr_visible_button = GTK_OPTION_MENU(omenu);
  submenu = NULL;
  group = NULL;

  add_option_menu_item(GTK_MENU(menu), _("Public"),
		       GTK_SIGNAL_FUNC (attributes_update),
		       structclass, GINT_TO_POINTER(STRUCT_PUBLIC));
  add_option_menu_item(GTK_MENU(menu), _("Private"),
		       GTK_SIGNAL_FUNC (attributes_update),
		       structclass, GINT_TO_POINTER(STRUCT_PRIVATE) );
  add_option_menu_item(GTK_MENU(menu), _("Protected"),
		       GTK_SIGNAL_FUNC (attributes_update),
		       structclass, GINT_TO_POINTER(STRUCT_PROTECTED) );
  add_option_menu_item(GTK_MENU(menu), _("Implementation"),
		       GTK_SIGNAL_FUNC (attributes_update),
		       structclass, GINT_TO_POINTER(STRUCT_IMPLEMENTATION) );

  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);

  {
    GtkWidget * align;
    align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
    gtk_container_add(GTK_CONTAINER(align), omenu);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0,1,4,5, GTK_FILL,0, 0,3);
    gtk_table_attach (GTK_TABLE (table), align, 1,2,4,5, GTK_FILL,0, 0,3);
  }

  hbox2 = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Class scope"));
  prop_dialog->attr_class_scope = GTK_TOGGLE_BUTTON(checkbox);
  gtk_box_pack_start (GTK_BOX (hbox2), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);

  gtk_widget_show(vbox2);

  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page(notebook, vbox, page_label);

}

/*************************************************************
 ******************** OPERATIONS *****************************
 *************************************************************/

/* Forward declaration: */
static void operations_get_current_values(STRUCTClassDialog *prop_dialog);

static void
parameters_set_sensitive(STRUCTClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_name), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_type), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_value), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_comment), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_kind), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_kind_button), val);
}

static void
parameters_set_values(STRUCTClassDialog *prop_dialog, STRUCTParameter *param)
{
  gtk_entry_set_text(prop_dialog->param_name, param->name);
  gtk_entry_set_text(prop_dialog->param_type, param->type);
  if (param->value != NULL)
    gtk_entry_set_text(prop_dialog->param_value, param->value);
  else
    gtk_entry_set_text(prop_dialog->param_value, "");
  if (param->comment != NULL)
    set_comment(prop_dialog->param_comment, param->comment);
  else
    set_comment(prop_dialog->param_comment, "");

  gtk_option_menu_set_history(prop_dialog->param_kind_button,
			      (gint)param->kind);
}

static void
parameters_clear_values(STRUCTClassDialog *prop_dialog)
{
  gtk_entry_set_text(prop_dialog->param_name, "");
  gtk_entry_set_text(prop_dialog->param_type, "");
  gtk_entry_set_text(prop_dialog->param_value, "");
  set_comment(prop_dialog->param_comment, "");
  gtk_option_menu_set_history(prop_dialog->param_kind_button,
			      (gint) STRUCT_UNDEF_KIND);

}

static void
parameters_get_values (STRUCTClassDialog *prop_dialog, STRUCTParameter *param)
{
  g_free(param->name);
  g_free(param->type);
  g_free(param->comment);
  if (param->value != NULL)
    g_free(param->value);

  param->name = g_strdup (gtk_entry_get_text (prop_dialog->param_name));
  param->type = g_strdup (gtk_entry_get_text (prop_dialog->param_type));

  param->value = g_strdup (gtk_entry_get_text(prop_dialog->param_value));
  param->comment = g_strdup (get_comment(prop_dialog->param_comment));

  param->kind = (STRUCTParameterKind) GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(gtk_menu_get_active(prop_dialog->param_kind))));
}

static void
parameters_get_current_values(STRUCTClassDialog *prop_dialog)
{
  STRUCTParameter *current_param;
  GtkLabel *label;
  char *new_str;

  if (prop_dialog->current_param != NULL) {
    current_param = (STRUCTParameter *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_param));
    if (current_param != NULL) {
      parameters_get_values(prop_dialog, current_param);
      label = GTK_LABEL(GTK_BIN(prop_dialog->current_param)->child);
      new_str = struct_get_parameter_string(current_param);
      gtk_label_set_text(label, new_str);
      g_free(new_str);
    }
  }
}


static void
parameters_list_selection_changed_callback(GtkWidget *gtklist,
					   STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkObject *list_item;
  STRUCTParameter *param;

  prop_dialog = structclass->properties_dialog;

  if (!prop_dialog)
    return; /* maybe hiding a bug elsewhere */

  parameters_get_current_values(prop_dialog);

  list = GTK_LIST(gtklist)->selection;
  if (!list) { /* No selected */
    parameters_set_sensitive(prop_dialog, FALSE);
    parameters_clear_values(prop_dialog);
    prop_dialog->current_param = NULL;
    return;
  }

  list_item = GTK_OBJECT(list->data);
  param = (STRUCTParameter *)gtk_object_get_user_data(list_item);
  parameters_set_values(prop_dialog, param);
  parameters_set_sensitive(prop_dialog, TRUE);

  prop_dialog->current_param = GTK_LIST_ITEM(list_item);
  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->param_name));
}

static void
parameters_list_new_callback(GtkWidget *button,
			     STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkWidget *list_item;
  STRUCTOperation *current_op;
  STRUCTParameter *param;
  char *utf;

  prop_dialog = structclass->properties_dialog;

  parameters_get_current_values(prop_dialog);

  current_op = (STRUCTOperation *)
    gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));

  param = struct_parameter_new();

  utf = struct_get_parameter_string (param);
  list_item = gtk_list_item_new_with_label (utf);
  gtk_widget_show (list_item);
  g_free (utf);

  gtk_object_set_user_data(GTK_OBJECT(list_item), param);

  current_op->parameters = g_list_append(current_op->parameters,
					 (gpointer) param);

  list = g_list_append(NULL, list_item);
  gtk_list_append_items(prop_dialog->parameters_list, list);

  if (prop_dialog->parameters_list->children != NULL)
    gtk_list_unselect_child(prop_dialog->parameters_list,
			    GTK_WIDGET(prop_dialog->parameters_list->children->data));
  gtk_list_select_child(prop_dialog->parameters_list, list_item);

  prop_dialog->current_param = GTK_LIST_ITEM(list_item);
}

static void
parameters_list_delete_callback(GtkWidget *button,
				STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkList *gtklist;
  STRUCTOperation *current_op;
  STRUCTParameter *param;

  prop_dialog = structclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->parameters_list);


  if (gtklist->selection != NULL) {
    /* Remove from current operations parameter list: */
    current_op = (STRUCTOperation *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
    param = (STRUCTParameter *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_param));

    current_op->parameters = g_list_remove(current_op->parameters,
					   (gpointer) param);
    struct_parameter_destroy(param);

    /* Remove from gtk list: */
    list = g_list_prepend(NULL, prop_dialog->current_param);

    prop_dialog->current_param = NULL;

    gtk_list_remove_items(gtklist, list);
    g_list_free(list);
  }
}

static void
parameters_list_move_up_callback(GtkWidget *button,
				 STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  STRUCTOperation *current_op;
  STRUCTParameter *param;
  int i;

  prop_dialog = structclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->parameters_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);

    i = gtk_list_child_position(gtklist, list_item);
    if (i>0)
      i--;

    param = (STRUCTParameter *) gtk_object_get_user_data(GTK_OBJECT(list_item));

    /* Move parameter in current operations list: */
    current_op = (STRUCTOperation *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));

    current_op->parameters = g_list_remove(current_op->parameters,
					   (gpointer) param);
    current_op->parameters = g_list_insert(current_op->parameters,
					   (gpointer) param,
					   i);

    /* Move parameter in gtk list: */
    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);

    operations_get_current_values(prop_dialog);
  }
}

static void
parameters_list_move_down_callback(GtkWidget *button,
				   STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  STRUCTOperation *current_op;
  STRUCTParameter *param;
  int i;

  prop_dialog = structclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->parameters_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);

    i = gtk_list_child_position(gtklist, list_item);
    if (i<(g_list_length(gtklist->children)-1))
      i++;

    param = (STRUCTParameter *) gtk_object_get_user_data(GTK_OBJECT(list_item));

    /* Move parameter in current operations list: */
    current_op = (STRUCTOperation *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));

    current_op->parameters = g_list_remove(current_op->parameters,
					   (gpointer) param);
    current_op->parameters = g_list_insert(current_op->parameters,
					   (gpointer) param,
					   i);

    /* Move parameter in gtk list: */
    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);

    operations_get_current_values(prop_dialog);
  }
}

static void
operations_set_sensitive(STRUCTClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_name), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_type), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_stereotype), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_comment), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_visible_button), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_visible), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_class_scope), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_inheritance_type), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_inheritance_type_button), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_query), val);

  gtk_widget_set_sensitive(prop_dialog->param_new_button, val);
  gtk_widget_set_sensitive(prop_dialog->param_delete_button, val);
  gtk_widget_set_sensitive(prop_dialog->param_down_button, val);
  gtk_widget_set_sensitive(prop_dialog->param_up_button, val);
}

static void
operations_set_values(STRUCTClassDialog *prop_dialog, STRUCTOperation *op)
{
  GList *list;
  STRUCTParameter *param;
  GtkWidget *list_item;
  gchar *str;

  gtk_entry_set_text(prop_dialog->op_name, op->name);
  if (op->type != NULL)
    gtk_entry_set_text(prop_dialog->op_type, op->type);
  else
    gtk_entry_set_text(prop_dialog->op_type, "");

  if (op->stereotype != NULL)
    gtk_entry_set_text(prop_dialog->op_stereotype, op->stereotype);
  else
    gtk_entry_set_text(prop_dialog->op_stereotype, "");

  if (op->comment != NULL)
    set_comment(prop_dialog->op_comment, op->comment);
  else
    set_comment(prop_dialog->op_comment, "");

  gtk_option_menu_set_history(prop_dialog->op_visible_button,
			      (gint)op->visibility);
  gtk_toggle_button_set_active(prop_dialog->op_class_scope, op->class_scope);
  gtk_toggle_button_set_active(prop_dialog->op_query, op->query);
  gtk_option_menu_set_history(prop_dialog->op_inheritance_type_button,
			      (gint)op->inheritance_type);

  gtk_list_clear_items(prop_dialog->parameters_list, 0, -1);
  prop_dialog->current_param = NULL;
  parameters_set_sensitive(prop_dialog, FALSE);

  list = op->parameters;
  while (list != NULL) {
    param = (STRUCTParameter *)list->data;

    str = struct_get_parameter_string (param);
    list_item = gtk_list_item_new_with_label (str);
    g_free (str);

    gtk_object_set_user_data(GTK_OBJECT(list_item), (gpointer) param);
    gtk_container_add (GTK_CONTAINER (prop_dialog->parameters_list), list_item);
    gtk_widget_show (list_item);

    list = g_list_next(list);
  }
}

static void
operations_clear_values(STRUCTClassDialog *prop_dialog)
{
  gtk_entry_set_text(prop_dialog->op_name, "");
  gtk_entry_set_text(prop_dialog->op_type, "");
  gtk_entry_set_text(prop_dialog->op_stereotype, "");
  set_comment(prop_dialog->op_comment, "");
  gtk_toggle_button_set_active(prop_dialog->op_class_scope, FALSE);
  gtk_toggle_button_set_active(prop_dialog->op_query, FALSE);

  gtk_list_clear_items(prop_dialog->parameters_list, 0, -1);
  prop_dialog->current_param = NULL;
  parameters_set_sensitive(prop_dialog, FALSE);
}


static void
operations_get_values(STRUCTClassDialog *prop_dialog, STRUCTOperation *op)
{
  const gchar *s;

  g_free(op->name);
  if (op->type != NULL)
	  g_free(op->type);

  op->name = g_strdup(gtk_entry_get_text(prop_dialog->op_name));
  op->type = g_strdup (gtk_entry_get_text(prop_dialog->op_type));
  op->comment = g_strdup(get_comment(prop_dialog->op_comment));

  s = gtk_entry_get_text(prop_dialog->op_stereotype);
  if (s && s[0])
    op->stereotype = g_strdup (s);
  else
    op->stereotype = NULL;

  op->visibility = (STRUCTVisibility)
    GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(gtk_menu_get_active(prop_dialog->op_visible))));

  op->class_scope = prop_dialog->op_class_scope->active;
  op->inheritance_type = (STRUCTInheritanceType)
    GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(gtk_menu_get_active(prop_dialog->op_inheritance_type))));

  op->query = prop_dialog->op_query->active;

}

static void
operations_get_current_values(STRUCTClassDialog *prop_dialog)
{
  STRUCTOperation *current_op;
  GtkLabel *label;
  char *new_str;

  parameters_get_current_values(prop_dialog);

  if (prop_dialog->current_op != NULL) {
    current_op = (STRUCTOperation *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
    if (current_op != NULL) {
      operations_get_values(prop_dialog, current_op);
      label = GTK_LABEL(GTK_BIN(prop_dialog->current_op)->child);
      new_str = struct_get_operation_string(current_op);
      gtk_label_set_text (label, new_str);
      g_free (new_str);
    }
  }
}

static void
operations_list_item_destroy_callback(GtkWidget *list_item,
				      gpointer data)
{
  STRUCTOperation *op;

  op = (STRUCTOperation *) gtk_object_get_user_data(GTK_OBJECT(list_item));

  if (op != NULL) {
    struct_operation_destroy(op);
    /*printf("Destroying operation list_item's user_data!\n");*/
  }
}

static void
operations_list_selection_changed_callback(GtkWidget *gtklist,
					   STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkObject *list_item;
  STRUCTOperation *op;

  prop_dialog = structclass->properties_dialog;

  if (!prop_dialog)
    return; /* maybe hiding a bug elsewhere */

  operations_get_current_values(prop_dialog);

  list = GTK_LIST(gtklist)->selection;
  if (!list) { /* No selected */
    operations_set_sensitive(prop_dialog, FALSE);
    operations_clear_values(prop_dialog);
    prop_dialog->current_op = NULL;
    return;
  }

  list_item = GTK_OBJECT(list->data);
  op = (STRUCTOperation *)gtk_object_get_user_data(list_item);
  operations_set_values(prop_dialog, op);
  operations_set_sensitive(prop_dialog, TRUE);

  prop_dialog->current_op = GTK_LIST_ITEM(list_item);
  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->op_name));
}

static void
operations_list_new_callback(GtkWidget *button,
			     STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkWidget *list_item;
  STRUCTOperation *op;
  char *utfstr;

  prop_dialog = structclass->properties_dialog;

  operations_get_current_values(prop_dialog);

  op = struct_operation_new();
  /* need to make new ConnectionPoints valid and remember them */
  struct_operation_ensure_connection_points (op, &structclass->element.object);
  prop_dialog->added_connections =
    g_list_prepend(prop_dialog->added_connections, op->left_connection);
  prop_dialog->added_connections =
    g_list_prepend(prop_dialog->added_connections, op->right_connection);


  utfstr = struct_get_operation_string (op);
  list_item = gtk_list_item_new_with_label (utfstr);
  gtk_widget_show (list_item);
  g_free (utfstr);

  gtk_object_set_user_data(GTK_OBJECT(list_item), op);
  gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
		      GTK_SIGNAL_FUNC (operations_list_item_destroy_callback),
		      NULL);

  list = g_list_append(NULL, list_item);
  gtk_list_append_items(prop_dialog->operations_list, list);

  if (prop_dialog->operations_list->children != NULL)
    gtk_list_unselect_child(prop_dialog->operations_list,
			    GTK_WIDGET(prop_dialog->operations_list->children->data));
  gtk_list_select_child(prop_dialog->operations_list, list_item);
}

static void
operations_list_delete_callback(GtkWidget *button,
				STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkList *gtklist;
  STRUCTOperation *op;


  prop_dialog = structclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->operations_list);

  if (gtklist->selection != NULL) {
    op = (STRUCTOperation *)
      gtk_object_get_user_data(GTK_OBJECT(gtklist->selection->data));

    if (op->left_connection != NULL) {
      prop_dialog->deleted_connections =
	g_list_prepend(prop_dialog->deleted_connections,
		       op->left_connection);
      prop_dialog->deleted_connections =
	g_list_prepend(prop_dialog->deleted_connections,
		       op->right_connection);
    }

    list = g_list_prepend(NULL, gtklist->selection->data);
    gtk_list_remove_items(gtklist, list);
    g_list_free(list);
    operations_clear_values(prop_dialog);
    operations_set_sensitive(prop_dialog, FALSE);
  }
}

static void
operations_list_move_up_callback(GtkWidget *button,
				 STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;

  prop_dialog = structclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->operations_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);

    i = gtk_list_child_position(gtklist, list_item);
    if (i>0)
      i--;

    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }

}

static void
operations_list_move_down_callback(GtkWidget *button,
				   STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;

  prop_dialog = structclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->operations_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);

    i = gtk_list_child_position(gtklist, list_item);
    if (i<(g_list_length(gtklist->children)-1))
      i++;

    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}

static void
operations_read_from_dialog(STRUCTClass *structclass,
			    STRUCTClassDialog *prop_dialog,
			    int connection_index)
{
  GList *list;
  STRUCTOperation *op;
  GtkWidget *list_item;
  GList *clear_list;
  DiaObject *obj;

  obj = &structclass->element.object;

  /* if currently select op is changed in the entries, update from widgets */
  operations_get_current_values(prop_dialog);

  /* Free current operations: */
  list = structclass->operations;
  while (list != NULL) {
    op = (STRUCTOperation *)list->data;
    struct_operation_destroy(op);
    list = g_list_next(list);
  }
  g_list_free (structclass->operations);
  structclass->operations = NULL;

  /* Insert new operations and remove them from gtklist: */
  list = GTK_LIST (prop_dialog->operations_list)->children;
  clear_list = NULL;
  while (list != NULL) {
    list_item = GTK_WIDGET(list->data);

    clear_list = g_list_prepend (clear_list, list_item);
    op = (STRUCTOperation *)
      gtk_object_get_user_data(GTK_OBJECT(list_item));
    gtk_object_set_user_data(GTK_OBJECT(list_item), NULL);
    structclass->operations = g_list_append(structclass->operations, op);

    if (op->left_connection == NULL) {
      struct_operation_ensure_connection_points (op, obj);

      prop_dialog->added_connections =
	g_list_prepend(prop_dialog->added_connections,
		       op->left_connection);
      prop_dialog->added_connections =
	g_list_prepend(prop_dialog->added_connections,
		       op->right_connection);
    }

    if ( (prop_dialog->op_vis->active) &&
	 (!prop_dialog->op_supp->active) ) {
      obj->connections[connection_index] = op->left_connection;
      connection_index++;
      obj->connections[connection_index] = op->right_connection;
      connection_index++;
    } else {
      structclass_store_disconnects(prop_dialog, op->left_connection);
      object_remove_connections_to(op->left_connection);
      structclass_store_disconnects(prop_dialog, op->right_connection);
      object_remove_connections_to(op->right_connection);
    }

    list = g_list_next(list);
  }
  clear_list = g_list_reverse (clear_list);
  gtk_list_remove_items (GTK_LIST (prop_dialog->operations_list), clear_list);
  g_list_free (clear_list);
}

static void
operations_fill_in_dialog(STRUCTClass *structclass)
{
  STRUCTClassDialog *prop_dialog;
  STRUCTOperation *op_copy;
  GtkWidget *list_item;
  GList *list;
  int i;

  prop_dialog = structclass->properties_dialog;

  if (prop_dialog->operations_list->children == NULL) {
    i = 0;
    list = structclass->operations;
    while (list != NULL) {
      STRUCTOperation *op = (STRUCTOperation *)list->data;
      gchar *opstr = struct_get_operation_string (op);

      list_item = gtk_list_item_new_with_label (opstr);
      op_copy = struct_operation_copy (op);
      /* Looks wrong but is required for the complicate connections memory management */
      op_copy->left_connection = op->left_connection;
      op_copy->right_connection = op->right_connection;
      gtk_object_set_user_data(GTK_OBJECT(list_item), (gpointer) op_copy);
      gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
			  GTK_SIGNAL_FUNC (operations_list_item_destroy_callback),
			  NULL);
      gtk_container_add (GTK_CONTAINER (prop_dialog->operations_list), list_item);
      gtk_widget_show (list_item);

      list = g_list_next(list); i++;
      g_free (opstr);
    }

    /* set operations non-sensitive */
    prop_dialog->current_op = NULL;
    operations_set_sensitive(prop_dialog, FALSE);
    operations_clear_values(prop_dialog);
  }
}

static void
operations_update(GtkWidget *widget, STRUCTClass *structclass)
{
  operations_get_current_values(structclass->properties_dialog);
}

static int
operations_update_event(GtkWidget *widget, GdkEventFocus *ev, STRUCTClass *structclass)
{
  operations_get_current_values(structclass->properties_dialog);
  return 0;
}

static GtkWidget*
operations_data_create_hbox (STRUCTClass *structclass)
{
  STRUCTClassDialog *prop_dialog;
  GtkWidget *hbox;
  GtkWidget *vbox2;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *omenu;
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *scrolledwindow;
  GtkWidget *checkbox;
  GSList *group;

  prop_dialog = structclass->properties_dialog;

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

  vbox2 = gtk_vbox_new(FALSE, 0);

  /* table containing operation 'name' up to 'query' and also the comment */
  table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->op_name = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), structclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  prop_dialog->op_type = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), structclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Stereotype:"));
  entry = gtk_entry_new();
  prop_dialog->op_stereotype = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), structclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);


  label = gtk_label_new(_("Visibility:"));

  omenu = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  prop_dialog->op_visible = GTK_MENU(menu);
  prop_dialog->op_visible_button = GTK_OPTION_MENU(omenu);
  submenu = NULL;
  group = NULL;

  add_option_menu_item(GTK_MENU(menu), _("Public"),
		       GTK_SIGNAL_FUNC (operations_update), structclass,
		       GINT_TO_POINTER(STRUCT_PUBLIC) );
  add_option_menu_item(GTK_MENU(menu), _("Private"),
		       GTK_SIGNAL_FUNC (operations_update), structclass,
		       GINT_TO_POINTER(STRUCT_PRIVATE) );
  add_option_menu_item(GTK_MENU(menu), _("Protected"),
		       GTK_SIGNAL_FUNC (operations_update), structclass,
		       GINT_TO_POINTER(STRUCT_PROTECTED) );
  add_option_menu_item(GTK_MENU(menu), _("Implementation"),
		       GTK_SIGNAL_FUNC (operations_update), structclass,
		       GINT_TO_POINTER(STRUCT_IMPLEMENTATION) );

  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
					/* left, right, top, bottom */
  gtk_table_attach (GTK_TABLE (table), label, 2,3,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), omenu, 3,4,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);
  /* end: Visibility */

  label = gtk_label_new(_("Inheritance type:"));

  omenu = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  prop_dialog->op_inheritance_type = GTK_MENU(menu);
  prop_dialog->op_inheritance_type_button = GTK_OPTION_MENU(omenu);
  submenu = NULL;
  group = NULL;

  add_option_menu_item(GTK_MENU(menu), _("Abstract"),
		       GTK_SIGNAL_FUNC (operations_update),  structclass,
		       GINT_TO_POINTER(STRUCT_ABSTRACT) );
  add_option_menu_item(GTK_MENU(menu), _("Polymorphic (virtual)"),
		       GTK_SIGNAL_FUNC (operations_update), structclass,
		       GINT_TO_POINTER(STRUCT_POLYMORPHIC) );
  add_option_menu_item(GTK_MENU(menu), _("Leaf (final)"),
		       GTK_SIGNAL_FUNC (operations_update), structclass,
		       GINT_TO_POINTER(STRUCT_LEAF) );

  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);

  gtk_table_attach (GTK_TABLE (table), label, 2,3,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), omenu, 3,4,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);
  /* end: Inheritance type */

  checkbox = gtk_check_button_new_with_label(_("Class scope"));
  prop_dialog->op_class_scope = GTK_TOGGLE_BUTTON(checkbox);
  gtk_table_attach (GTK_TABLE (table), checkbox, 2,3,2,3, GTK_FILL,0, 0,2);

  checkbox = gtk_check_button_new_with_label(_("Query"));
  prop_dialog->op_query = GTK_TOGGLE_BUTTON(checkbox);
  gtk_table_attach (GTK_TABLE (table), checkbox, 3,4,2,3, GTK_FILL,0, 2,0);

  label = gtk_label_new(_("Comment:"));
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);
  /* with GTK_POLICY_NEVER the comment filed gets smaller unti l text is entered; than it would resize the dialog! */
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  entry = gtk_text_view_new ();
  prop_dialog->op_comment = GTK_TEXT_VIEW(entry);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (entry),TRUE);

  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
#if 0 /* no "activate" signal on GtkTextView */
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), structclass);
#endif
  gtk_table_attach (GTK_TABLE (table), label, 4,5,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), scrolledwindow, 4,5,1,3, GTK_FILL | GTK_EXPAND,0, 0,0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

  return hbox;
}

static GtkWidget*
operations_parameters_editor_create_vbox (STRUCTClass *structclass)
{
  STRUCTClassDialog *prop_dialog;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *vbox3;
  GtkWidget *label;
  GtkWidget *scrolled_win;
  GtkWidget *list;
  GtkWidget *button;

  prop_dialog = structclass->properties_dialog;

  vbox2 = gtk_vbox_new(FALSE, 5);
  /* Parameters list label */
  hbox2 = gtk_hbox_new(FALSE, 5);

  label = gtk_label_new(_("Parameters:"));
  gtk_box_pack_start( GTK_BOX(hbox2), label, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

  /* Parameters list editor - with of list at least width of buttons*/
  hbox2 = gtk_hbox_new(TRUE, 5);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox2), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  list = gtk_list_new ();
  prop_dialog->parameters_list = GTK_LIST(list);
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (list);

  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
		      GTK_SIGNAL_FUNC(parameters_list_selection_changed_callback),
		      structclass);

  vbox3 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
  prop_dialog->param_new_button = button;
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(parameters_list_new_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  prop_dialog->param_delete_button = button;
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(parameters_list_delete_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
  prop_dialog->param_up_button = button;
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(parameters_list_move_up_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
  prop_dialog->param_down_button = button;
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(parameters_list_move_down_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox2), vbox3, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);
  /* end: Parameter list editor */

  return vbox2;
}

static GtkWidget*
operations_parameters_data_create_vbox (STRUCTClass *structclass)
{
  STRUCTClassDialog *prop_dialog;
  GtkWidget *vbox2;
  GtkWidget *frame;
  GtkWidget *vbox3;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *scrolledwindow;
  GtkWidget *omenu;
  GtkWidget *menu;
  GtkWidget *submenu;
  GSList *group;

  prop_dialog = structclass->properties_dialog;

  vbox2 = gtk_vbox_new(FALSE, 5);
  frame = gtk_frame_new(_("Parameter data"));
  vbox3 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox3), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox3);
  gtk_widget_show(frame);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, TRUE, 0);

  table = gtk_table_new (3, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start (GTK_BOX (vbox3), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->param_name = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), structclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), structclass);
  prop_dialog->param_type = GTK_ENTRY(entry);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Def. value:"));
  entry = gtk_entry_new();
  prop_dialog->param_value = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), structclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Comment:"));
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  entry = gtk_text_view_new ();
  prop_dialog->param_comment = GTK_TEXT_VIEW(entry);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (entry),TRUE);

  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), structclass);
#if 0 /* no "activate" signal on GtkTextView */
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), structclass);
#endif
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 2,3,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), scrolledwindow, 3,4,1,3, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Direction:"));

  omenu = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  prop_dialog->param_kind = GTK_MENU(menu);
  prop_dialog->param_kind_button = GTK_OPTION_MENU(omenu);
  submenu = NULL;
  group = NULL;

  add_option_menu_item(GTK_MENU(menu), _("Undefined"),
		       GTK_SIGNAL_FUNC (operations_update), structclass,
		       GINT_TO_POINTER(STRUCT_UNDEF_KIND) );
  add_option_menu_item(GTK_MENU(menu), _("In"),
		       GTK_SIGNAL_FUNC (operations_update), structclass,
		       GINT_TO_POINTER(STRUCT_IN) );
  add_option_menu_item(GTK_MENU(menu), _("Out"),
		       GTK_SIGNAL_FUNC (operations_update), structclass,
		       GINT_TO_POINTER(STRUCT_OUT) );
  add_option_menu_item(GTK_MENU(menu), _("In & Out"),
		       GTK_SIGNAL_FUNC (operations_update), structclass,
		       GINT_TO_POINTER(STRUCT_INOUT) );

  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);

  {
    GtkWidget * align;
    align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
    gtk_container_add(GTK_CONTAINER(align), omenu);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 2,3,0,1, GTK_FILL,0, 0,3);
    gtk_table_attach (GTK_TABLE (table), align, 3,4,0,1, GTK_FILL,0, 0,3);
  }

  return vbox2;
}

static void
operations_create_page(GtkNotebook *notebook,  STRUCTClass *structclass)
{
  STRUCTClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *vbox3;
  GtkWidget *scrolled_win;
  GtkWidget *button;
  GtkWidget *list;
  GtkWidget *frame;

  prop_dialog = structclass->properties_dialog;

  /* Operations page: */
  page_label = gtk_label_new_with_mnemonic (_("_Operations"));

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  hbox = gtk_hbox_new(FALSE, 5);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  list = gtk_list_new ();
  prop_dialog->operations_list = GTK_LIST(list);
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (list);

  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
		      GTK_SIGNAL_FUNC(operations_list_selection_changed_callback),
		      structclass);

  vbox2 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(operations_list_new_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(operations_list_delete_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(operations_list_move_up_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(operations_list_move_down_callback),
		      structclass);

  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new(_("Operation data"));
  vbox2 = gtk_vbox_new(FALSE, 0);
  hbox = operations_data_create_hbox (structclass);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show(frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

  /* parameter stuff below operation stuff */
  hbox = gtk_hbox_new (FALSE, 5);
  vbox3 = operations_parameters_editor_create_vbox (structclass);
  gtk_box_pack_start (GTK_BOX (hbox), vbox3, TRUE, TRUE, 5);

  vbox3 = operations_parameters_data_create_vbox (structclass);
  gtk_box_pack_start (GTK_BOX (hbox), vbox3, TRUE, TRUE, 5);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 5);

  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);
}

/************************************************************
 ******************** TEMPLATES *****************************
 ************************************************************/

static void
templates_set_sensitive(STRUCTClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->templ_name), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->templ_type), val);
}

static void
templates_set_values (STRUCTClassDialog *prop_dialog,
		      STRUCTFormalParameter *param)
{
  if (param->name)
    gtk_entry_set_text(prop_dialog->templ_name, param->name);
  if (param->type != NULL)
    gtk_entry_set_text(prop_dialog->templ_type, param->type);
}

static void
templates_clear_values(STRUCTClassDialog *prop_dialog)
{
  gtk_entry_set_text(prop_dialog->templ_name, "");
  gtk_entry_set_text(prop_dialog->templ_type, "");
}

static void
templates_get_values(STRUCTClassDialog *prop_dialog, STRUCTFormalParameter *param)
{
  g_free(param->name);
  if (param->type != NULL)
    g_free(param->type);

  param->name = g_strdup (gtk_entry_get_text (prop_dialog->templ_name));
  param->type = g_strdup (gtk_entry_get_text (prop_dialog->templ_type));
}


static void
templates_get_current_values(STRUCTClassDialog *prop_dialog)
{
  STRUCTFormalParameter *current_param;
  GtkLabel *label;
  gchar* new_str;

  if (prop_dialog->current_templ != NULL) {
    current_param = (STRUCTFormalParameter *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_templ));
    if (current_param != NULL) {
      templates_get_values(prop_dialog, current_param);
      label = GTK_LABEL(GTK_BIN(prop_dialog->current_templ)->child);
      new_str = struct_get_formalparameter_string (current_param);
      gtk_label_set_text(label, new_str);
      g_free(new_str);
    }
  }
}

static void
templates_list_item_destroy_callback(GtkWidget *list_item,
				     gpointer data)
{
  STRUCTFormalParameter *param;

  param = (STRUCTFormalParameter *)
    gtk_object_get_user_data(GTK_OBJECT(list_item));

  if (param != NULL) {
    struct_formalparameter_destroy(param);
    /*printf("Destroying list_item's user_data!\n"); */
  }
}

static void
templates_list_selection_changed_callback(GtkWidget *gtklist,
					  STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkObject *list_item;
  STRUCTFormalParameter *param;

  prop_dialog = structclass->properties_dialog;

  if (!prop_dialog)
    return; /* maybe hiding a bug elsewhere */

  templates_get_current_values(prop_dialog);

  list = GTK_LIST(gtklist)->selection;
  if (!list) { /* No selected */
    templates_set_sensitive(prop_dialog, FALSE);
    templates_clear_values(prop_dialog);
    prop_dialog->current_templ = NULL;
    return;
  }

  list_item = GTK_OBJECT(list->data);
  param = (STRUCTFormalParameter *)gtk_object_get_user_data(list_item);
  templates_set_values(prop_dialog, param);
  templates_set_sensitive(prop_dialog, TRUE);

  prop_dialog->current_templ = GTK_LIST_ITEM(list_item);
  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->templ_name));
}

static void
templates_list_new_callback(GtkWidget *button,
			    STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkWidget *list_item;
  STRUCTFormalParameter *param;
  char *utfstr;

  prop_dialog = structclass->properties_dialog;

  templates_get_current_values(prop_dialog);

  param = struct_formalparameter_new();

  utfstr = struct_get_formalparameter_string (param);
  list_item = gtk_list_item_new_with_label (utfstr);
  gtk_widget_show (list_item);
  g_free (utfstr);

  gtk_object_set_user_data(GTK_OBJECT(list_item), param);
  gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
		      GTK_SIGNAL_FUNC (templates_list_item_destroy_callback),
		      NULL);

  list = g_list_append(NULL, list_item);
  gtk_list_append_items(prop_dialog->templates_list, list);

  if (prop_dialog->templates_list->children != NULL)
    gtk_list_unselect_child(prop_dialog->templates_list,
			    GTK_WIDGET(prop_dialog->templates_list->children->data));
  gtk_list_select_child(prop_dialog->templates_list, list_item);
}

static void
templates_list_delete_callback(GtkWidget *button,
			       STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkList *gtklist;

  prop_dialog = structclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->templates_list);

  if (gtklist->selection != NULL) {
    list = g_list_prepend(NULL, gtklist->selection->data);
    gtk_list_remove_items(gtklist, list);
    g_list_free(list);
    templates_clear_values(prop_dialog);
    templates_set_sensitive(prop_dialog, FALSE);
  }
}

static void
templates_list_move_up_callback(GtkWidget *button,
				STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;

  prop_dialog = structclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->templates_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);

    i = gtk_list_child_position(gtklist, list_item);
    if (i>0)
      i--;

    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}

static void
templates_list_move_down_callback(GtkWidget *button,
				   STRUCTClass *structclass)
{
  GList *list;
  STRUCTClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;

  prop_dialog = structclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->templates_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);

    i = gtk_list_child_position(gtklist, list_item);
    if (i<(g_list_length(gtklist->children)-1))
      i++;

    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}


static void
templates_read_from_dialog(STRUCTClass *structclass, STRUCTClassDialog *prop_dialog)
{
  GList *list;
  STRUCTFormalParameter *param;
  GtkWidget *list_item;
  GList *clear_list;

  templates_get_current_values(prop_dialog); /* if changed, update from widgets */

  structclass->template = prop_dialog->templ_template->active;

  /* Free current formal parameters: */
  list = structclass->formal_params;
  while (list != NULL) {
    param = (STRUCTFormalParameter *)list->data;
    struct_formalparameter_destroy(param);
    list = g_list_next(list);
  }
  g_list_free (structclass->formal_params);
  structclass->formal_params = NULL;

  /* Insert new formal params and remove them from gtklist: */
  list = GTK_LIST (prop_dialog->templates_list)->children;
  clear_list = NULL;
  while (list != NULL) {
    list_item = GTK_WIDGET(list->data);
    clear_list = g_list_prepend (clear_list, list_item);
    param = (STRUCTFormalParameter *)
      gtk_object_get_user_data(GTK_OBJECT(list_item));
    gtk_object_set_user_data(GTK_OBJECT(list_item), NULL);
    structclass->formal_params = g_list_append(structclass->formal_params, param);
    list = g_list_next(list);
  }
  clear_list = g_list_reverse (clear_list);
  gtk_list_remove_items (GTK_LIST (prop_dialog->templates_list), clear_list);
  g_list_free (clear_list);
}

static void
templates_fill_in_dialog(STRUCTClass *structclass)
{
  STRUCTClassDialog *prop_dialog;
  STRUCTFormalParameter *param_copy;
  GList *list;
  GtkWidget *list_item;
  int i;
  prop_dialog = structclass->properties_dialog;

  gtk_toggle_button_set_active(prop_dialog->templ_template, structclass->template);

  /* copy in new template-parameters: */
  if (prop_dialog->templates_list->children == NULL) {
    i = 0;
    list = structclass->formal_params;
    while (list != NULL) {
      STRUCTFormalParameter *param = (STRUCTFormalParameter *)list->data;
      gchar *paramstr = struct_get_formalparameter_string(param);

      list_item = gtk_list_item_new_with_label (paramstr);
      param_copy = struct_formalparameter_copy(param);
      gtk_object_set_user_data(GTK_OBJECT(list_item),
			       (gpointer) param_copy);
      gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
			  GTK_SIGNAL_FUNC (templates_list_item_destroy_callback),
			  NULL);
      gtk_container_add (GTK_CONTAINER (prop_dialog->templates_list),
			 list_item);
      gtk_widget_show (list_item);

      list = g_list_next(list); i++;
      g_free (paramstr);
    }
    /* set templates non-sensitive */
    prop_dialog->current_templ = NULL;
    templates_set_sensitive(prop_dialog, FALSE);
    templates_clear_values(prop_dialog);
  }

}


static void
templates_update(GtkWidget *widget, STRUCTClass *structclass)
{
  templates_get_current_values(structclass->properties_dialog);
}

static int
templates_update_event(GtkWidget *widget, GdkEventFocus *ev, STRUCTClass *structclass)
{
  templates_get_current_values(structclass->properties_dialog);
  return 0;
}

static void
templates_create_page(GtkNotebook *notebook,  STRUCTClass *structclass)
{
  STRUCTClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *table;
  GtkWidget *entry;
  GtkWidget *checkbox;
  GtkWidget *scrolled_win;
  GtkWidget *button;
  GtkWidget *list;
  GtkWidget *frame;

  prop_dialog = structclass->properties_dialog;

  /* Templates page: */
  page_label = gtk_label_new_with_mnemonic (_("_Templates"));

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  hbox2 = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Template class"));
  prop_dialog->templ_template = GTK_TOGGLE_BUTTON(checkbox);
  gtk_box_pack_start (GTK_BOX (hbox2), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 5);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  list = gtk_list_new ();
  prop_dialog->templates_list = GTK_LIST(list);
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (list);

  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
		      GTK_SIGNAL_FUNC(templates_list_selection_changed_callback),
		      structclass);

  vbox2 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(templates_list_new_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(templates_list_delete_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(templates_list_move_up_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(templates_list_move_down_callback),
		      structclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new(_("Formal parameter data"));
  vbox2 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 10);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show(frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->templ_name = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (templates_update_event), structclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (templates_update), structclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  prop_dialog->templ_type = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (templates_update_event), structclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (templates_update), structclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  gtk_widget_show(vbox2);

  /* TODO: Add stuff here! */

  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);
}



/******************************************************
 ******************** ALL *****************************
 ******************************************************/

static void
switch_page_callback(GtkNotebook *notebook,
		     GtkNotebookPage *page)
{
  STRUCTClass *structclass;
  STRUCTClassDialog *prop_dialog;

  structclass = (STRUCTClass *)
    gtk_object_get_user_data(GTK_OBJECT(notebook));

  prop_dialog = structclass->properties_dialog;

  if (prop_dialog != NULL) {
    attributes_get_current_values(prop_dialog);
    operations_get_current_values(prop_dialog);
    templates_get_current_values(prop_dialog);
  }
}

static void
destroy_properties_dialog (GtkWidget* widget,
			   gpointer user_data)
{
  /* dialog gone, mark as such */
  STRUCTClass *structclass = (STRUCTClass *)user_data;

  g_free (structclass->properties_dialog);
  structclass->properties_dialog = NULL;
}



static void
fill_in_dialog(STRUCTClass *structclass)
{
#ifdef DEBUG
  structclass_sanity_check(structclass, "Filling in dialog before attrs");
#endif
//  class_fill_in_dialog(structclass);
//  attributes_fill_in_dialog(structclass);
//  operations_fill_in_dialog(structclass);
//  templates_fill_in_dialog(structclass);
}

ObjectChange *
factory_apply_props_from_dialog(STRUCTClass *structclass, GtkWidget *widget)
{
  STRUCTClassDialog *prop_dialog;
  DiaObject *obj;
  GList *list;
  int num_attrib, num_ops;
  GList *added, *deleted, *disconnected;
  STRUCTClassState *old_state = NULL;
  prop_dialog = structclass->properties_dialog;
  old_state = structclass_get_state(structclass);
  GList *widgetmap = structclass->widgetmap;
  for(;widgetmap != NULL;widgetmap = widgetmap->next)
  {
      WidgetAndValue *wav = widgetmap->data;
      g_free(wav->value);
      if(wav->celltype == ENTRY)
      {
         wav->value = gtk_entry_get_text (GTK_ENTRY (wav->widget));
      }
      else{

         wav->value =  gtk_combo_box_get_active_text(GTK_COMBO_BOX(wav->widget));
      }
  }

}


ObjectChange *
structclass_apply_props_from_dialog(STRUCTClass *structclass, GtkWidget *widget)
{
  STRUCTClassDialog *prop_dialog;
  DiaObject *obj;
  GList *list;
  int num_attrib, num_ops;
  GList *added, *deleted, *disconnected;
  STRUCTClassState *old_state = NULL;

#ifdef DEBUG
  structclass_sanity_check(structclass, "Apply from dialog start");
#endif

  prop_dialog = structclass->properties_dialog;

  old_state = structclass_get_state(structclass);

  /* Allocate enought connection points for attributes and operations. */
  /* (two per op/attr) */
  if ( (prop_dialog->attr_vis->active) && (!prop_dialog->attr_supp->active))
    num_attrib = g_list_length(prop_dialog->attributes_list->children);
  else
    num_attrib = 0;
  if ( (prop_dialog->op_vis->active) && (!prop_dialog->op_supp->active))
    num_ops = g_list_length(prop_dialog->operations_list->children);
  else
    num_ops = 0;
  obj = &structclass->element.object;
#ifdef STRUCT_MAINPOINT
  obj->num_connections =
    STRUCTCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2 + 1;
#else
  obj->num_connections =
    STRUCTCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2;
#endif
  obj->connections =
    g_realloc(obj->connections,
	      obj->num_connections*sizeof(ConnectionPoint *));

  /* Read from dialog and put in object: */
  class_read_from_dialog(structclass, prop_dialog);
  attributes_read_from_dialog(structclass, prop_dialog, STRUCTCLASS_CONNECTIONPOINTS);
  /* ^^^ attribs must be called before ops, to get the right order of the
     connectionpoints. */
  operations_read_from_dialog(structclass, prop_dialog, STRUCTCLASS_CONNECTIONPOINTS+num_attrib*2);
  templates_read_from_dialog(structclass, prop_dialog);

  /* Reestablish mainpoint */
#ifdef STRUCT_MAINPOINT
  obj->connections[obj->num_connections-1] =
    &structclass->connections[STRUCTCLASS_CONNECTIONPOINTS];
#endif

  /* unconnect from all deleted connectionpoints. */
  list = prop_dialog->deleted_connections;
  while (list != NULL) {
    ConnectionPoint *connection = (ConnectionPoint *) list->data;

    structclass_store_disconnects(prop_dialog, connection);
    object_remove_connections_to(connection);

    list = g_list_next(list);
  }

  deleted = prop_dialog->deleted_connections;
  prop_dialog->deleted_connections = NULL;

  added = prop_dialog->added_connections;
  prop_dialog->added_connections = NULL;

  disconnected = prop_dialog->disconnected_connections;
  prop_dialog->disconnected_connections = NULL;

  /* Update data: */
  structclass_calculate_data(structclass);
  structclass_update_data(structclass);

  /* Fill in class with the new data: */
  fill_in_dialog(structclass);
#ifdef DEBUG
  structclass_sanity_check(structclass, "Apply from dialog end");
#endif
  return  new_structclass_change(structclass, old_state, added, deleted, disconnected);
}

static void
create_dialog_pages(GtkNotebook *notebook, STRUCTClass *structclass)
{

  class_create_page(notebook, structclass);
  attributes_create_page(notebook, structclass);
  operations_create_page(notebook, structclass);
  templates_create_page(notebook, structclass);
  style_create_page(notebook, structclass);
}

GtkWidget *
factory_get_properties(STRUCTClass *class, gboolean is_default)
{

  STRUCTClassDialog *prop_dialog;
  GtkWidget *vbox;
  GtkTable *mainTable;
   if (class->properties_dialog == NULL) {
    prop_dialog = g_new(STRUCTClassDialog, 1);
    class->properties_dialog = prop_dialog;

    class->properties_dialog->EnumsAndStructs = &structList;
   }

      vbox = gtk_vbox_new(FALSE, 0);
//    gtk_object_ref(GTK_OBJECT(vbox));
//    gtk_object_sink(GTK_OBJECT(vbox));

    prop_dialog->dialog = vbox;
    gtk_widget_set_name(vbox,class->name); // 2014-3-21 lcy �������֣��������ֲ�ͬ���塣

    prop_dialog->current_attr = NULL;
    prop_dialog->current_op = NULL;
    prop_dialog->current_param = NULL;
    prop_dialog->current_templ = NULL;
    prop_dialog->deleted_connections = NULL;
    prop_dialog->added_connections = NULL;
    prop_dialog->disconnected_connections = NULL;


    gtk_signal_connect (GTK_OBJECT (prop_dialog->dialog),
		        "destroy",
			GTK_SIGNAL_FUNC(destroy_properties_dialog),
			(gpointer) class);

    GList *datalist = NULL;
    int row =0;
    int col = 0;
    GList *tlist = prop_dialog->EnumsAndStructs->structList;
    for(;tlist != NULL;tlist = tlist->next)
    {
        FactoryStructItemList *fsil  = tlist->data;
        if(0 == strcmp(fsil->name,class->name))
        {
            GList *item = fsil->list;
            int num = g_list_length(item);
            prop_dialog->mainTable = gtk_table_new(num,4,FALSE);  // 2014-3-19 lcy ����Ҫ���������,�����������б�.
            for(;item != NULL ; item = item->next,row++)
            {
                factory_create_struct_dialog(prop_dialog,item->data,row);
            }
            break;
        }
    }

    gtk_table_set_col_spacing(GTK_TABLE(prop_dialog->mainTable),1,20);
  gtk_widget_show_all (class->properties_dialog->dialog);

  return class->properties_dialog->dialog;

}



GtkWidget *
structclass_get_properties(STRUCTClass *structclass, gboolean is_default)
{
  STRUCTClassDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *notebook;

#ifdef DEBUG
  structclass_sanity_check(structclass, "Get properties start");
#endif
  if (structclass->properties_dialog == NULL) {
    prop_dialog = g_new(STRUCTClassDialog, 1);
    structclass->properties_dialog = prop_dialog;

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_object_ref(GTK_OBJECT(vbox));
    gtk_object_sink(GTK_OBJECT(vbox));

    prop_dialog->dialog = vbox;

    prop_dialog->current_attr = NULL;
    prop_dialog->current_op = NULL;
    prop_dialog->current_param = NULL;
    prop_dialog->current_templ = NULL;
    prop_dialog->deleted_connections = NULL;
    prop_dialog->added_connections = NULL;
    prop_dialog->disconnected_connections = NULL;

    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_box_pack_start (GTK_BOX (vbox),	notebook, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (notebook), 10);

    gtk_object_set_user_data(GTK_OBJECT(notebook), (gpointer) structclass);

    gtk_signal_connect (GTK_OBJECT (notebook),
			"switch_page",
			GTK_SIGNAL_FUNC(switch_page_callback),
			(gpointer) structclass);
    gtk_signal_connect (GTK_OBJECT (structclass->properties_dialog->dialog),
		        "destroy",
			GTK_SIGNAL_FUNC(destroy_properties_dialog),
			(gpointer) structclass);

    create_dialog_pages(GTK_NOTEBOOK( notebook ), structclass);

    gtk_widget_show (notebook);
  }

  fill_in_dialog(structclass);
  gtk_widget_show (structclass->properties_dialog->dialog);

  return structclass->properties_dialog->dialog;
}


/****************** UNDO stuff: ******************/

static void
structclass_free_state(STRUCTClassState *state)
{
  GList *list;

  g_object_unref (state->normal_font);
  g_object_unref (state->abstract_font);
  g_object_unref (state->polymorphic_font);
  g_object_unref (state->classname_font);
  g_object_unref (state->abstract_classname_font);
  g_object_unref (state->comment_font);

  g_free(state->name);
  g_free(state->stereotype);
  g_free(state->comment);

  list = state->attributes;
  while (list) {
    struct_attribute_destroy((STRUCTAttribute *) list->data);
    list = g_list_next(list);
  }
  g_list_free(state->attributes);

  list = state->operations;
  while (list) {
    struct_operation_destroy((STRUCTOperation *) list->data);
    list = g_list_next(list);
  }
  g_list_free(state->operations);

  list = state->formal_params;
  while (list) {
    struct_formalparameter_destroy((STRUCTFormalParameter *) list->data);
    list = g_list_next(list);
  }
  g_list_free(state->formal_params);
}

static STRUCTClassState *
structclass_get_state(STRUCTClass *structclass)
{
  STRUCTClassState *state = g_new0(STRUCTClassState, 1);
  GList *list;

  state->font_height = structclass->font_height;
  state->abstract_font_height = structclass->abstract_font_height;
  state->polymorphic_font_height = structclass->polymorphic_font_height;
  state->classname_font_height = structclass->classname_font_height;
  state->abstract_classname_font_height = structclass->abstract_classname_font_height;
  state->comment_font_height = structclass->comment_font_height;

  state->normal_font = g_object_ref (structclass->normal_font);
  state->abstract_font = g_object_ref (structclass->abstract_font);
  state->polymorphic_font = g_object_ref (structclass->polymorphic_font);
  state->classname_font = g_object_ref (structclass->classname_font);
  state->abstract_classname_font = g_object_ref (structclass->abstract_classname_font);
  state->comment_font = g_object_ref (structclass->comment_font);

  state->name = g_strdup(structclass->name);
  state->stereotype = g_strdup(structclass->stereotype);
  state->comment = g_strdup(structclass->comment);

  state->abstract = structclass->abstract;
  state->suppress_attributes = structclass->suppress_attributes;
  state->suppress_operations = structclass->suppress_operations;
  state->visible_attributes = structclass->visible_attributes;
  state->visible_operations = structclass->visible_operations;
  state->visible_comments = structclass->visible_comments;

  state->wrap_operations = structclass->wrap_operations;
  state->wrap_after_char = structclass->wrap_after_char;
  state->comment_line_length = structclass->comment_line_length;
  state->comment_tagging = structclass->comment_tagging;

  state->line_color = structclass->line_color;
  state->fill_color = structclass->fill_color;
  state->text_color = structclass->text_color;

  state->attributes = NULL;
  list = structclass->attributes;
  while (list != NULL) {
    STRUCTAttribute *attr = (STRUCTAttribute *)list->data;
    STRUCTAttribute *attr_copy;

    attr_copy = struct_attribute_copy(attr);
    /* Looks wrong, but needed fro proper restore */
    attr_copy->left_connection = attr->left_connection;
    attr_copy->right_connection = attr->right_connection;

    state->attributes = g_list_append(state->attributes, attr_copy);
    list = g_list_next(list);
  }


  state->operations = NULL;
  list = structclass->operations;
  while (list != NULL) {
    STRUCTOperation *op = (STRUCTOperation *)list->data;
    STRUCTOperation *op_copy;

    op_copy = struct_operation_copy(op);
    op_copy->left_connection = op->left_connection;
    op_copy->right_connection = op->right_connection;
    state->operations = g_list_append(state->operations, op_copy);
    list = g_list_next(list);
  }


  state->template = structclass->template;

  state->formal_params = NULL;
  list = structclass->formal_params;
  while (list != NULL) {
    STRUCTFormalParameter *param = (STRUCTFormalParameter *)list->data;
    STRUCTFormalParameter *param_copy;

    param_copy = struct_formalparameter_copy(param);
    state->formal_params = g_list_append(state->formal_params, param_copy);

    list = g_list_next(list);
  }

  return state;
}

static void
structclass_update_connectionpoints(STRUCTClass *structclass)
{
  int num_attrib, num_ops;
  DiaObject *obj;
  GList *list;
  int connection_index;
  STRUCTClassDialog *prop_dialog;

  prop_dialog = structclass->properties_dialog;

  /* Allocate enought connection points for attributes and operations. */
  /* (two per op/attr) */
  if ( (structclass->visible_attributes) && (!structclass->suppress_attributes))
    num_attrib = g_list_length(structclass->attributes);
  else
    num_attrib = 0;
  if ( (structclass->visible_operations) && (!structclass->suppress_operations))
    num_ops = g_list_length(structclass->operations);
  else
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

  list = structclass->attributes;
  while (list != NULL) {
    STRUCTAttribute *attr = (STRUCTAttribute *) list->data;

    if ( (structclass->visible_attributes) &&
	 (!structclass->suppress_attributes)) {
      obj->connections[connection_index] = attr->left_connection;
      connection_index++;
      obj->connections[connection_index] = attr->right_connection;
      connection_index++;
    }

    list = g_list_next(list);
  }

  if (prop_dialog)
    gtk_list_clear_items (GTK_LIST (prop_dialog->attributes_list), 0, -1);

  list = structclass->operations;
  while (list != NULL) {
    STRUCTOperation *op = (STRUCTOperation *) list->data;

    if ( (structclass->visible_operations) &&
	 (!structclass->suppress_operations)) {
      obj->connections[connection_index] = op->left_connection;
      connection_index++;
      obj->connections[connection_index] = op->right_connection;
      connection_index++;
    }

    list = g_list_next(list);
  }
  if (prop_dialog)
    gtk_list_clear_items (GTK_LIST (prop_dialog->operations_list), 0, -1);

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
  g_object_unref (structclass->abstract_font);
  structclass->abstract_font = state->abstract_font;
  g_object_unref (structclass->polymorphic_font);
  structclass->polymorphic_font = state->polymorphic_font;
  g_object_unref (structclass->classname_font);
  structclass->classname_font = state->classname_font;
  g_object_unref (structclass->abstract_classname_font);
  structclass->abstract_classname_font = state->abstract_classname_font;
  g_object_unref (structclass->comment_font);
  structclass->comment_font = state->comment_font;

  structclass->name = state->name;
  structclass->stereotype = state->stereotype;
  structclass->comment = state->comment;

  structclass->abstract = state->abstract;
  structclass->suppress_attributes = state->suppress_attributes;
  structclass->suppress_operations = state->suppress_operations;
  structclass->visible_attributes = state->visible_attributes;
  structclass->visible_operations = state->visible_operations;
  structclass->visible_comments = state->visible_comments;

  structclass->wrap_operations = state->wrap_operations;
  structclass->wrap_after_char = state->wrap_after_char;
  structclass->comment_line_length = state->comment_line_length;
  structclass->comment_tagging = state->comment_tagging;

  structclass->line_color = state->line_color;
  structclass->fill_color = state->fill_color;
  structclass->text_color = state->text_color;

  structclass->attributes = state->attributes;
  structclass->operations = state->operations;
  structclass->template = state->template;
  structclass->formal_params = state->formal_params;

  g_free(state);

  structclass_update_connectionpoints(structclass);

  structclass_calculate_data(structclass);
  structclass_update_data(structclass);
}

static void
structclass_change_apply(STRUCTClassChange *change, DiaObject *obj)
{
  STRUCTClassState *old_state;
  GList *list;

  old_state = structclass_get_state(change->obj);

  structclass_set_state(change->obj, change->saved_state);

  list = change->disconnected;
  while (list) {
    Disconnect *dis = (Disconnect *)list->data;

    object_unconnect(dis->other_object, dis->other_handle);

    list = g_list_next(list);
  }

  change->saved_state = old_state;
  change->applied = 1;
}

static void
structclass_change_revert(STRUCTClassChange *change, DiaObject *obj)
{
  STRUCTClassState *old_state;
  GList *list;

  old_state = structclass_get_state(change->obj);

  structclass_set_state(change->obj, change->saved_state);

  list = change->disconnected;
  while (list) {
    Disconnect *dis = (Disconnect *)list->data;

    object_connect(dis->other_object, dis->other_handle, dis->cp);

    list = g_list_next(list);
  }

  change->saved_state = old_state;
  change->applied = 0;
}

static void
structclass_change_free(STRUCTClassChange *change)
{
  GList *list, *free_list;

  structclass_free_state(change->saved_state);
  g_free(change->saved_state);

  /* Doesn't this mean only one of add, delete can be done in each apply? */
  if (change->applied)
    free_list = change->deleted_cp;
  else
    free_list = change->added_cp;

  list = free_list;
  while (list != NULL) {
    ConnectionPoint *connection = (ConnectionPoint *) list->data;

    g_assert(connection->connected == NULL); /* Paranoid */
    object_remove_connections_to(connection); /* Shouldn't be needed */
    g_free(connection);

    list = g_list_next(list);
  }

  g_list_free(free_list);

}

static ObjectChange *
new_structclass_change(STRUCTClass *obj, STRUCTClassState *saved_state,
		    GList *added, GList *deleted, GList *disconnected)
{
  STRUCTClassChange *change;

  change = g_new0(STRUCTClassChange, 1);

  change->obj_change.apply =
    (ObjectChangeApplyFunc) structclass_change_apply;
  change->obj_change.revert =
    (ObjectChangeRevertFunc) structclass_change_revert;
  change->obj_change.free =
    (ObjectChangeFreeFunc) structclass_change_free;

  change->obj = obj;
  change->saved_state = saved_state;
  change->applied = 1;

  change->added_cp = added;
  change->deleted_cp = deleted;
  change->disconnected = disconnected;

  return (ObjectChange *)change;
}
/*
        get the contents of a comment text view.
*/
const gchar * get_comment(GtkTextView *view)
{
  GtkTextBuffer * buffer = gtk_text_view_get_buffer(view);
  GtkTextIter start;
  GtkTextIter end;

  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

void
set_comment(GtkTextView *view, gchar *text)
{
  GtkTextBuffer * buffer = gtk_text_view_get_buffer(view);
  GtkTextIter start;
  GtkTextIter end;

  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);
  gtk_text_buffer_delete(buffer,&start,&end);
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_insert( buffer, &start, text, strlen(text));
}


