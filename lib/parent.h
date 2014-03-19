/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 2003 Vadim Berezniker
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

#ifndef PARENT_H
#define PARENT_H

#include <glib.h>
#include "geometry.h"

DIAVAR GList *parent_list_affected(GList *obj_list);
DIAVAR gboolean parent_handle_extents(DiaObject *obj, Rectangle *extents);
DIAVAR Point parent_move_child_delta(Rectangle *p_ext, Rectangle *c_text, Point *delta);
DIAVAR void parent_point_extents(Point *point, Rectangle *extents);
gboolean parent_list_expand(GList *obj_list);
DIAVAR GList *parent_list_affected_hierarchy(GList *obj_list);
DIAVAR gboolean parent_handle_move_out_check(DiaObject *object, Point *to);
DIAVAR gboolean parent_handle_move_in_check(DiaObject *object, Point *to, Point *start_at);
DIAVAR void parent_apply_to_children(DiaObject *obj, DiaObjectFunc func);

#endif /* PARENT_H  */
