/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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
 */

#include <config.h>

#include <gtk/gtk.h>

#include "defaults.h"
#include "intl.h"
#include "properties-dialog.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "object.h"

static GtkWidget *dialog = NULL;
static GtkWidget *dialog_vbox = NULL;
static GtkWidget *object_part = NULL;
static DiaObjectType *current_objtype = NULL;
static DiaObject *current_object = NULL;

static GtkWidget *no_defaults_dialog = NULL;

static gint defaults_respond(GtkWidget *widget, gint response_id, gpointer data);

static void create_dialog()
{
//  GtkWidget *pwidget = gtk_widget_get_toplevel(ddisplay_active()->menu_bar);
  dialog = gtk_dialog_new_with_buttons(
             _("Object defaults"),
             interface_get_toolbox_shell(), GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
             GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
             GTK_STOCK_OK, GTK_RESPONSE_OK,
             NULL);
//    gtk_window_set_modal (pwidget,TRUE);
  gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  dialog_vbox = GTK_DIALOG (dialog)->vbox;

  gtk_window_set_role (GTK_WINDOW (dialog), "defaults_window");

  g_signal_connect(G_OBJECT (dialog), "response",
		   G_CALLBACK(defaults_respond), NULL);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_widget_hide), NULL);

  no_defaults_dialog = gtk_label_new(_("This object has no defaults."));
  gtk_widget_show (no_defaults_dialog);

#if GLIB_CHECK_VERSION(2,10,0)
  g_object_ref_sink(GTK_OBJECT(no_defaults_dialog));
#else
  g_object_ref(GTK_OBJECT(no_defaults_dialog));
  gtk_object_sink(GTK_OBJECT(no_defaults_dialog));
#endif
}

static gint
defaults_dialog_destroyed(GtkWidget *widget, gpointer data)
{
  if (widget == object_part) {
    object_part = NULL;
    current_objtype = NULL;
    current_object = NULL;
  }
  dialog = NULL;
  return 0;
}

static gint
defaults_respond(GtkWidget *widget, gint response_id, gpointer data)
{
  if (response_id == GTK_RESPONSE_OK ||
      response_id == GTK_RESPONSE_APPLY) {
    if (current_objtype != NULL) {
      if (current_objtype->ops->apply_defaults)
        current_objtype->ops->apply_defaults();
      else if (current_object != NULL)
        object_apply_props_from_dialog (current_object, object_part);
    }
  }
  if (response_id != GTK_RESPONSE_APPLY)
    gtk_widget_hide(widget);
  return 0;
}

void
defaults_show(DiaObjectType *objtype, gpointer user_data)
{
  GtkWidget *defaults;
  DiaObject *def_object = NULL;
  gchar *title = NULL;

  if (objtype != NULL) {
    if (objtype->ops->get_defaults != NULL)
      defaults = objtype->ops->get_defaults();
    else {
      def_object = dia_object_default_get (objtype, user_data);
      defaults = object_create_props_dialog (def_object, TRUE);
    }
    title = g_strconcat(_("Defaults: "), objtype->name, NULL);
  } else {
    defaults = NULL;
  }

  if (dialog == NULL)
    create_dialog();

  if ((objtype==NULL) || (defaults == NULL)) {
    /* No defaults or no object */
    defaults = no_defaults_dialog;
    objtype = NULL;
  }

  if (object_part != NULL) {
    gtk_container_remove(GTK_CONTAINER(dialog_vbox), object_part);
    object_part = NULL;
  }
  g_signal_connect (G_OBJECT (dialog), "destroy",
		      G_CALLBACK(defaults_dialog_destroyed), NULL);
  /* don't destroy dialog when window manager close button pressed */
  g_signal_connect(G_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_widget_hide), NULL);
  g_signal_connect(GTK_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_true), NULL);

  gtk_box_pack_start(GTK_BOX(dialog_vbox), defaults, TRUE, TRUE, 0);
  gtk_widget_show (defaults);

  if (title)
    {
      gtk_window_set_title(GTK_WINDOW(dialog), title);
      g_free(title);
    }
  else
      gtk_window_set_title (GTK_WINDOW (dialog), _("Object defaults"));

  if (object_part != defaults) {
    gtk_window_resize (GTK_WINDOW(dialog), 1, 1); /* shrink to default */
    if (dialog->window)
      gdk_window_invalidate_rect (dialog->window, NULL, TRUE);
  }
  gtk_window_present (GTK_WINDOW(dialog));
  object_part = defaults;
  current_objtype = objtype;
  current_object = def_object;
}
