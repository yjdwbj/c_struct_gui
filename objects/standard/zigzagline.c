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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>

#include "intl.h"
#include "object.h"
#include "orth_conn.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"
#include "properties.h"
#include "autoroute.h"

#include "tool-icons.h"

#define DEFAULT_WIDTH 0.15

#define HANDLE_MIDDLE HANDLE_CUSTOM1

typedef struct _Zigzagline {
  OrthConn orth;

  Color line_color;
  LineStyle line_style;
  real dashlength;
  real line_width;
  real corner_radius;
  Arrow start_arrow, end_arrow;
} Zigzagline;


static ObjectChange* zigzagline_move_handle(Zigzagline *zigzagline, Handle *handle,
					    Point *to, ConnectionPoint *cp,
					    HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* zigzagline_move(Zigzagline *zigzagline, Point *to);
static void zigzagline_select(Zigzagline *zigzagline, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void zigzagline_draw(Zigzagline *zigzagline, DiaRenderer *renderer);
static DiaObject *zigzagline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real zigzagline_distance_from(Zigzagline *zigzagline, Point *point);
static void zigzagline_update_data(Zigzagline *zigzagline);
static void zigzagline_destroy(Zigzagline *zigzagline);
static DiaObject *zigzagline_copy(Zigzagline *zigzagline);
static DiaMenu *zigzagline_get_object_menu(Zigzagline *zigzagline,
					   Point *clickedpoint);

static PropDescription *zigzagline_describe_props(Zigzagline *zigzagline);
static void zigzagline_get_props(Zigzagline *zigzagline, GPtrArray *props);
static void zigzagline_set_props(Zigzagline *zigzagline, GPtrArray *props);

static void zigzagline_save(Zigzagline *zigzagline, ObjectNode obj_node,
			    const char *filename);
static DiaObject *zigzagline_load(ObjectNode obj_node, int version,
			       const char *filename);

static ObjectTypeOps zigzagline_type_ops =
{
  (CreateFunc)zigzagline_create,   /* create */
  (LoadFunc)  zigzagline_load,     /* load */
  (SaveFunc)  zigzagline_save,      /* save */
  (GetDefaultsFunc)   NULL,/*zigzagline_get_defaults*/
  (ApplyDefaultsFunc) NULL /*zigzagline_apply_defaults*/
};

static DiaObjectType zigzagline_type =
{
  "Standard - ZigZagLine",   /* name */
  /* Version 0 had no autorouting and so shouldn't have it set by default. */
  1,                      /* version */
  (char **) zigzagline_icon,      /* pixmap */

  &zigzagline_type_ops       /* ops */
};

DiaObjectType *_zigzagline_type = (DiaObjectType *) &zigzagline_type;


static ObjectOps zigzagline_ops = {
  (DestroyFunc)         zigzagline_destroy,
  (DrawFunc)            zigzagline_draw,
  (DistanceFunc)        zigzagline_distance_from,
  (SelectFunc)          zigzagline_select,
  (CopyFunc)            zigzagline_copy,
  (MoveFunc)            zigzagline_move,
  (MoveHandleFunc)      zigzagline_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      zigzagline_get_object_menu,
  (DescribePropsFunc)   zigzagline_describe_props,
  (GetPropsFunc)        zigzagline_get_props,
  (SetPropsFunc)        zigzagline_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropNumData zigzagline_corner_radius_data = { 0.0, 10.0, 0.1 };

static PropDescription zigzagline_props[] = {
  ORTHCONN_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_START_ARROW,
  PROP_STD_END_ARROW,
  { "corner_radius", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Corner radius"), NULL, &zigzagline_corner_radius_data },
  PROP_DESC_END
};

static PropDescription *
zigzagline_describe_props(Zigzagline *zigzagline)
{
  if (zigzagline_props[0].quark == 0)
    prop_desc_list_calculate_quarks(zigzagline_props);
  return zigzagline_props;
}

static PropOffset zigzagline_offsets[] = {
  ORTHCONN_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Zigzagline, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Zigzagline, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Zigzagline, line_style), offsetof(Zigzagline, dashlength) },
  { "start_arrow", PROP_TYPE_ARROW, offsetof(Zigzagline, start_arrow) },
  { "end_arrow", PROP_TYPE_ARROW, offsetof(Zigzagline, end_arrow) },
  { "corner_radius", PROP_TYPE_REAL, offsetof(Zigzagline, corner_radius) },
  { NULL, 0, 0 }
};

static void
zigzagline_get_props(Zigzagline *zigzagline, GPtrArray *props)
{
  object_get_props_from_offsets(&zigzagline->orth.object, zigzagline_offsets,
				props);
}

static void
zigzagline_set_props(Zigzagline *zigzagline, GPtrArray *props)
{
  object_set_props_from_offsets(&zigzagline->orth.object, zigzagline_offsets,
				props);
  zigzagline_update_data(zigzagline);
}

static real
zigzagline_distance_from(Zigzagline *zigzagline, Point *point)
{
  OrthConn *orth = &zigzagline->orth;
  return orthconn_distance_from(orth, point, zigzagline->line_width);
}

static void
zigzagline_select(Zigzagline *zigzagline, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  orthconn_update_data(&zigzagline->orth);
}

static ObjectChange*
zigzagline_move_handle(Zigzagline *zigzagline, Handle *handle,
		       Point *to, ConnectionPoint *cp,
		       HandleMoveReason reason, ModifierKeys modifiers)
{
  ObjectChange *change;
  assert(zigzagline!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  change = orthconn_move_handle((OrthConn*)zigzagline, handle, to, cp,
				reason, modifiers);

  zigzagline_update_data(zigzagline);

  return change;
}


static ObjectChange*
zigzagline_move(Zigzagline *zigzagline, Point *to)
{
  orthconn_move(&zigzagline->orth, to);
  zigzagline_update_data(zigzagline);

  return NULL;
}

static void
zigzagline_draw(Zigzagline *zigzagline, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  OrthConn *orth = &zigzagline->orth;
  Point *points;
  int n;

  points = &orth->points[0];
  n = orth->numpoints;

  renderer_ops->set_linewidth(renderer, zigzagline->line_width);
  renderer_ops->set_linestyle(renderer, zigzagline->line_style);
  renderer_ops->set_dashlength(renderer, zigzagline->dashlength);
  if (zigzagline->corner_radius > 0)
    renderer_ops->set_linejoin(renderer, LINEJOIN_ROUND);
  else
    renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer_ops->draw_rounded_polyline_with_arrows(renderer,
						  points, n,
						  zigzagline->line_width,
						  &zigzagline->line_color,
						  &zigzagline->start_arrow,
						  &zigzagline->end_arrow,
						  zigzagline->corner_radius);

}

static DiaObject *
zigzagline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Zigzagline *zigzagline;
  OrthConn *orth;
  DiaObject *obj;

  /*zigzagline_init_defaults();*/
  zigzagline = g_malloc0(sizeof(Zigzagline));
  orth = &zigzagline->orth;
  obj = &orth->object;

  obj->type = &zigzagline_type;
  obj->ops = &zigzagline_ops;

  orthconn_init(orth, startpoint);

  zigzagline->line_width =  attributes_get_default_linewidth();
  zigzagline->line_color = attributes_get_foreground();
  attributes_get_default_line_style(&zigzagline->line_style,
				    &zigzagline->dashlength);
  zigzagline->start_arrow = attributes_get_default_start_arrow();
  zigzagline->end_arrow = attributes_get_default_end_arrow();
  zigzagline->corner_radius = 0.0;

  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];

  zigzagline_update_data(zigzagline);

  return &zigzagline->orth.object;
}

static void
zigzagline_destroy(Zigzagline *zigzagline)
{
  orthconn_destroy(&zigzagline->orth);
}

static DiaObject *
zigzagline_copy(Zigzagline *zigzagline)
{
  Zigzagline *newzigzagline;
  OrthConn *orth, *neworth;
  DiaObject *newobj;

  orth = &zigzagline->orth;

  newzigzagline = g_malloc0(sizeof(Zigzagline));
  neworth = &newzigzagline->orth;
  newobj = &neworth->object;

  orthconn_copy(orth, neworth);

  newzigzagline->line_color = zigzagline->line_color;
  newzigzagline->line_width = zigzagline->line_width;
  newzigzagline->line_style = zigzagline->line_style;
  newzigzagline->dashlength = zigzagline->dashlength;
  newzigzagline->start_arrow = zigzagline->start_arrow;
  newzigzagline->end_arrow = zigzagline->end_arrow;
  newzigzagline->corner_radius = zigzagline->corner_radius;

  zigzagline_update_data(newzigzagline);

  return &newzigzagline->orth.object;
}

static void
zigzagline_update_data(Zigzagline *zigzagline)
{
  OrthConn *orth = &zigzagline->orth;
  DiaObject *obj = &orth->object;
  PolyBBExtras *extra = &orth->extra_spacing;

  orthconn_update_data(&zigzagline->orth);

  extra->start_long =
    extra->end_long =
    extra->middle_trans =
    extra->start_trans =
    extra->end_trans = (zigzagline->line_width / 2.0);

  orthconn_update_boundingbox(orth);

  if (zigzagline->start_arrow.type != ARROW_NONE) {
    Rectangle bbox;
    Point move_arrow, move_line;
    Point to = orth->points[0];
    Point from = orth->points[1];
    calculate_arrow_point(&zigzagline->start_arrow, &to, &from,
                          &move_arrow, &move_line, zigzagline->line_width);
    /* move them */
    point_sub(&to, &move_arrow);
    point_sub(&from, &move_line);

    arrow_bbox (&zigzagline->start_arrow, zigzagline->line_width, &to, &from, &bbox);
    rectangle_union (&obj->bounding_box, &bbox);
  }
  if (zigzagline->end_arrow.type != ARROW_NONE) {
    Rectangle bbox;
    Point move_arrow, move_line;
    int n = orth->numpoints;
    Point to = orth->points[n-1];
    Point from = orth->points[n-2];
    calculate_arrow_point(&zigzagline->start_arrow, &to, &from,
                          &move_arrow, &move_line, zigzagline->line_width);
    /* move them */
    point_sub(&to, &move_arrow);
    point_sub(&from, &move_line);

    arrow_bbox (&zigzagline->end_arrow, zigzagline->line_width, &to, &from, &bbox);
    rectangle_union (&obj->bounding_box, &bbox);
  }
}

static ObjectChange *
zigzagline_add_segment_callback(DiaObject *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_add_segment((OrthConn *)obj, clicked);
  zigzagline_update_data((Zigzagline *)obj);
  return change;
}

static ObjectChange *
zigzagline_delete_segment_callback(DiaObject *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_delete_segment((OrthConn *)obj, clicked);
  zigzagline_update_data((Zigzagline *)obj);
  return change;
}

static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), zigzagline_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), zigzagline_delete_segment_callback, NULL, 1 },
  ORTHCONN_COMMON_MENUS,
};

static DiaMenu object_menu = {
  "Zigzagline",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
zigzagline_get_object_menu(Zigzagline *zigzagline, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &zigzagline->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  orthconn_update_object_menu(orth, clickedpoint, &object_menu_items[2]);

  return &object_menu;
}


static void
zigzagline_save(Zigzagline *zigzagline, ObjectNode obj_node,
		const char *filename)
{
  orthconn_save(&zigzagline->orth, obj_node);

  if (!color_equals(&zigzagline->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &zigzagline->line_color);

  if (zigzagline->line_width != 0.1)
    data_add_real(new_attribute(obj_node, PROP_STDNAME_LINE_WIDTH),
		  zigzagline->line_width);

  if (zigzagline->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  zigzagline->line_style);

  if (zigzagline->start_arrow.type != ARROW_NONE) {
    save_arrow(obj_node, &zigzagline->start_arrow, "start_arrow",
	     "start_arrow_length", "start_arrow_width");
  }

  if (zigzagline->end_arrow.type != ARROW_NONE) {
    save_arrow(obj_node, &zigzagline->end_arrow, "end_arrow",
	     "end_arrow_length", "end_arrow_width");
  }

  if (zigzagline->line_style != LINESTYLE_SOLID &&
      zigzagline->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
                  zigzagline->dashlength);

  if (zigzagline->corner_radius > 0.0)
    data_add_real(new_attribute(obj_node, "corner_radius"),
                  zigzagline->corner_radius);
}

static DiaObject *
zigzagline_load(ObjectNode obj_node, int version, const char *filename)
{
  Zigzagline *zigzagline;
  OrthConn *orth;
  DiaObject *obj;
  AttributeNode attr;

  zigzagline = g_malloc0(sizeof(Zigzagline));

  orth = &zigzagline->orth;
  obj = &orth->object;

  obj->type = &zigzagline_type;
  obj->ops = &zigzagline_ops;

  orthconn_load(orth, obj_node);

  zigzagline->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &zigzagline->line_color);

  zigzagline->line_width = 0.1;
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if (attr != NULL)
    zigzagline->line_width = data_real(attribute_first_data(attr));

  zigzagline->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    zigzagline->line_style = data_enum(attribute_first_data(attr));

  load_arrow(obj_node, &zigzagline->start_arrow, "start_arrow",
	     "start_arrow_length", "start_arrow_width");

  load_arrow(obj_node, &zigzagline->end_arrow, "end_arrow",
	     "end_arrow_length", "end_arrow_width");

  zigzagline->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    zigzagline->dashlength = data_real(attribute_first_data(attr));

  zigzagline->corner_radius = 0.0;
  attr = object_find_attribute(obj_node, "corner_radius");
  if (attr != NULL)
    zigzagline->corner_radius =  data_real( attribute_first_data(attr) );

  zigzagline_update_data(zigzagline);

  return &zigzagline->orth.object;
}
