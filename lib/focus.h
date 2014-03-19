/* Dia -- a diagram creation/manipulation program
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
 */
#ifndef FOCUS_H
#define FOCUS_H

#include "diatypes.h"

struct _Focus {
  DiaObject *obj;
  Text *text;
  int has_focus;
  void *user_data; /* To be used by the object using this focus (eg. Text) */

  /* return TRUE if modified object.
     Set change if object is changed. */
  int (*key_event)(Focus *focus, guint keysym, const gchar *str, int strlen,
                   ObjectChange **change);
};

void request_focus(Focus *focus);
DIAVAR Focus *get_active_focus(DiagramData *dia);
DIAVAR void give_focus(Focus *focus);
DIAVAR Focus *focus_get_first_on_object(DiaObject *obj);
DIAVAR Focus *focus_next_on_diagram(DiagramData *dia);
DIAVAR Focus *focus_previous_on_diagram(DiagramData *dia);
DIAVAR void remove_focus_on_diagram(DiagramData *dia);
DIAVAR gboolean remove_focus_object(DiaObject *obj);
DIAVAR void reset_foci_on_diagram(DiagramData *dia);
DIAVAR DiaObject* focus_get_object(Focus *focus);

#endif /* FOCUS_H */




