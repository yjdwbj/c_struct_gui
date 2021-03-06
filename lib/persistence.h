/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 2003 Lars Clausen
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

/* persistence.h -- definitions for persistent storage.
 */

#ifndef PERSISTENCE_H
#define PERSISTENCE_H
#include "config.h"
#include "geometry.h"

#include <gtk/gtk.h>

typedef struct {
  int x, y;
  int width, height;
  gboolean isopen;
  GtkWindow *window;
} PersistentWindow;

typedef void (NullaryFunc)();

DIAVAR void persistence_load(void);
DIAVAR void persistence_save(void);
DIAVAR void persistence_register_window(GtkWindow *window);
DIAVAR void persistence_register_window_create(gchar *role, NullaryFunc *func);
DIAVAR void persistence_register_string_entry(gchar *role, GtkWidget *entry);
DIAVAR gboolean persistence_change_string_entry(gchar *role, gchar *string,
					 GtkWidget *widget);

/** A persistently stored list of strings.
 * The list contains no duplicates.
 * If sorted is FALSE, any string added will be placed in front of the list
 * (possibly removing it from further down), thus making it an LRU list.
 * The list is not tied to any particular GTK widget, as it has uses
 * in a number of different places (though mostly in menus)
 */
typedef struct _PersistentList {
  const gchar *role;
  gboolean sorted;
  gint max_members;
  GList *glist;
  GList *listeners;
} PersistentList;

typedef void (*PersistenceCallback)(GObject *, gpointer);

DIAVAR PersistentList *persistence_register_list(const gchar *role);
DIAVAR PersistentList *persistent_list_get(const gchar *role);
DIAVAR GList *persistent_list_get_glist(const gchar *role);
DIAVAR gboolean persistent_list_add(const gchar *role, const gchar *item);
DIAVAR void persistent_list_set_max_length(const gchar *role, gint max);
DIAVAR gboolean persistent_list_remove(const gchar *role, const gchar *item);
DIAVAR void persistent_list_remove_all(const gchar *role);
void persistent_list_add_listener(const gchar *role, PersistenceCallback func,
				  GObject *watch, gpointer userdata);

DIAVAR gboolean persistence_is_registered(gchar *role);

DIAVAR gint persistence_register_integer(gchar *role, int defaultvalue);
DIAVAR gint persistence_get_integer(gchar *role);
DIAVAR void persistence_set_integer(gchar *role, gint newvalue);

DIAVAR real persistence_register_real(gchar *role, real defaultvalue);
DIAVAR real persistence_get_real(gchar *role);
DIAVAR void persistence_set_real(gchar *role, real newvalue);

gboolean persistence_boolean_is_registered(const gchar *role);
DIAVAR gboolean persistence_register_boolean(const gchar *role, gboolean defaultvalue);
gboolean persistence_get_boolean(const gchar *role);
DIAVAR void persistence_set_boolean(const gchar *role, gboolean newvalue);

DIAVAR gchar *persistence_register_string(gchar *role, gchar *defaultvalue);
DIAVAR gchar *persistence_get_string(gchar *role);
DIAVAR void persistence_set_string(gchar *role, const gchar *newvalue);

DIAVAR Color *persistence_register_color(gchar *role, Color *defaultvalue);
Color *persistence_get_color(gchar *role);
DIAVAR void persistence_set_color(gchar *role, Color *newvalue);

#endif
