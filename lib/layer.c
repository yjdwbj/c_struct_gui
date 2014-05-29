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

#include "intl.h"
#include "diagramdata.h"
#include "diarenderer.h"
#include "dynamic_obj.h"

static const Rectangle invalid_extents = { -1.0,-1.0,-1.0,-1.0 };

/** The default object renderer.
 * @param obj An object to render.
 * @param renderer The renderer to render on.
 * @param active_layer The layer containing the object.
 * @param data The diagram containing the layer.
 * @bug The active_layer and data variables can be inferred from the object.
 */
static void
normal_render(DiaObject *obj, DiaRenderer *renderer,
	      int active_layer,
	      gpointer data)
{
  DIA_RENDERER_GET_CLASS(renderer)->draw_object(renderer, obj);
}


int render_bounding_boxes = FALSE;

/** Render all components of a single layer.  This function also handles
 *  rendering of bounding boxes for debugging purposes.
 * @param layer The layer to render.
 * @param renderer The renderer to draw things with.
 * @param update The rectangle that requires update.  Only objects that
 *  intersect with this rectangle will actually be get rendered.
 * @param obj_renderer A function that will render an object.
 * @param data The diagram that the layer belongs to.
 * @param active_layer Which number layer in the diagram is currently active.
 */
void
layer_render(Layer *layer, DiaRenderer *renderer, Rectangle *update,
	     ObjectRenderer obj_renderer,
	     gpointer data,
	     int active_layer)
{
  GList *list;
  DiaObject *obj;

  if (obj_renderer == NULL)
    obj_renderer = normal_render;

  /* Draw all objects: */
  list = layer->objects;
  while (list!=NULL) {
    obj = (DiaObject *) list->data;

    if (update==NULL || rectangle_intersects(update, &obj->bounding_box)) {
      if ((render_bounding_boxes) && (renderer->is_interactive)) {
	Point p1, p2;
	Color col;
	p1.x = obj->bounding_box.left;
	p1.y = obj->bounding_box.top;
	p2.x = obj->bounding_box.right;
	p2.y = obj->bounding_box.bottom;
	col.red = 1.0;
	col.green = 0.0;
	col.blue = 1.0;

        DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer,0.01);
	DIA_RENDERER_GET_CLASS(renderer)->draw_rect(renderer, &p1, &p2, &col);
      }
      (*obj_renderer)(obj, renderer, active_layer, data);
    }

    list = g_list_next(list);
  }
}

/** Set the parent layer of an object.
 * @param element A DiaObject that should be part of a layer.
 * @param user_data The layer it should be part of.
 */
static void
set_parent_layer(gpointer element, gpointer user_data)
{
  ((DiaObject*)element)->parent_layer = (Layer*)user_data;
  /* FIXME: even group members need a parent_layer and what about parent objects  ???
      * Now I know again why I always try to avoid back-pointers )-; --hb.
      * If the group objects didn't actually leave the diagram, this wouldn't
      * be a problem.  --LC */
}

/** Get the index of an object in a layer.
 * @param layer The layer the object is (should be) in.
 * @param obj The object to look for.
 * @return The index of the object in the layers list of objects.  This is also
 *  the vertical position of the object.
 * @bug This should be in a separate layer.c file.
 * @bug The layer could be inferred from the object, in which case the
 *  layer arg is not needed, and we would be sure we always looked in the
 *  right layer.
 */
int
layer_object_index(Layer *layer, DiaObject *obj)
{
  return (int)g_list_index(layer->objects, (gpointer) obj);
}

/** Add an object to the top of a layer.
 * @param layer The layer to add the object to.
 * @param obj The object to add.  This must not already be part of another
 *  layer.
 * @bug This should be in a separate layer.c file.
 * @bug This should just call layer_add_object_at().
 */
void
layer_add_object(Layer *layer, DiaObject *obj)
{
  layer->objects = g_list_append(layer->objects, (gpointer) obj);
  set_parent_layer(obj, layer);

  /* send a signal that we have added a object to the diagram */
  data_emit (layer_get_parent_diagram(layer), layer, obj, "object_add");
}

/** Add an object to a layer at a specific position.
 * @param layer The layer to add the object to.
 * @param obj The object to add.  This must not be part of another layer.
 * @param pos The top-to-bottom position this object should be inserted at.
 * @bug This should be in a separate layer.c file.
 */
void
layer_add_object_at(Layer *layer, DiaObject *obj, int pos)
{
  layer->objects = g_list_insert(layer->objects, (gpointer) obj, pos);
  set_parent_layer(obj, layer);

  /* send a signal that we have added a object to the diagram */
  data_emit (layer_get_parent_diagram(layer), layer, obj, "object_add");
}

/** Add a list of objects to the end of a layer.
 * @param layer The layer to add objects to.
 * @param obj_list The list of objects to add.  These must not already
 *  be part of another layer.
 * @bug Determine if the list is kept by g_list_concat.
 */
void
layer_add_objects(Layer *layer, GList *obj_list)
{
  GList *list = obj_list;

  layer->objects = g_list_concat(layer->objects, obj_list);
  g_list_foreach(obj_list, set_parent_layer, layer);

  while (list != NULL)
  {
    DiaObject *obj = (DiaObject *)list->data;
    /* send a signal that we have added a object to the diagram */
    data_emit (layer_get_parent_diagram(layer), layer, obj, "object_add");

    list = g_list_next(list);
  }
}

/** Add a list of objects to the top of a layer.
 * @param layer The layer to add objects to.
 * @param obj_list The list of objects to add.  These must not already
 *  be part of another layer.
 * @bug Determine if the list is kept by g_list_concat.
 */
void
layer_add_objects_first(Layer *layer, GList *obj_list)
{
  GList *list = obj_list;

  layer->objects = g_list_concat(obj_list, layer->objects);
  g_list_foreach(obj_list, set_parent_layer, layer);

  /* Send one signal per object added */
  while (list != NULL)
  {
    DiaObject *obj = (DiaObject *)list->data;
    /* send a signal that we have added a object to the diagram */
    data_emit (layer_get_parent_diagram(layer), layer, obj, "object_add");

    list = g_list_next(list);
  }

}

/** Remove an object from a layer.
 * @param layer The layer to remove the object from.
 * @param obj The object to remove.
 * @bug Why don't the layer_add functions deal with dynobj?
 */
void
layer_remove_object(Layer *layer, DiaObject *obj)
{
    if(factory_is_valid_type(obj))
        obj->ops->update_vname(obj);
   /*　这里要更新一下中文名链表　*/

  layer->objects = g_list_remove(layer->objects, obj);

  dynobj_list_remove_object(obj);
  set_parent_layer(obj, NULL);

  /* send a signal that we have removed a object from the diagram */
  data_emit (layer_get_parent_diagram(layer), layer, obj, "object_remove");
}

/** Remove a list of objects from a layer.
 * @param layer The layer to remove the objects from.
 * @param obj The objects to remove.
 * @bug This should call layer_remove_object repeatedly.
 */
void
layer_remove_objects(Layer *layer, GList *obj_list)
{
  DiaObject *obj;
  while (obj_list != NULL) {
    obj = (DiaObject *) obj_list->data;

    if(factory_is_valid_type(obj))
        obj->ops->update_vname(obj);/*　这里要更新一下中文名链表,且同时要删除线条　*/


    layer->objects = g_list_remove(layer->objects, obj);

    obj_list = g_list_next(obj_list);
    dynobj_list_remove_object(obj);
    set_parent_layer(obj, NULL);
    /* send a signal that we have removed a object from the diagram */
    data_emit (layer_get_parent_diagram(layer), layer, obj, "object_remove");
  }
}

/** Find the objects that intersect a given rectangle.
 * @param layer The layer to search in.
 * @param rect The rectangle to intersect with.
 * @return List of objects whose bounding box intersect the rectangle.  The
 *  list should be freed by the caller.
 */
GList *
layer_find_objects_intersecting_rectangle(Layer *layer, Rectangle *rect)
{
  GList *list;
  GList *selected_list;
  DiaObject *obj;

  selected_list = NULL;
  list = layer->objects;
  while (list != NULL) {
    obj = (DiaObject *)list->data;

    if (rectangle_intersects(rect, &obj->bounding_box)) {
      if (dia_object_is_selectable(obj)) {
	selected_list = g_list_prepend(selected_list, obj);
      }
      /* Objects in closed groups do not get selected, but their parents do.
      * Since the parents bbox is outside the objects, they will be found
      * anyway and the inner object can just be skipped. */
    }

    list = g_list_next(list);
  }

  return selected_list;
}

/** Find objects entirely contained in a rectangle.
 * @param layer The layer to search for objects in.
 * @param rect The rectangle that the objects should be in.
 * @return A list containing the objects that are entirely contained in the
 *  rectangle.  The list should be freed by the caller.
 */
GList *
layer_find_objects_in_rectangle(Layer *layer, Rectangle *rect)
{
  GList *list;
  GList *selected_list;
  DiaObject *obj;

  selected_list = NULL;
  list = layer->objects;
  while (list != NULL) {
    obj = (DiaObject *)list->data;

    if (rectangle_in_rectangle(rect, &obj->bounding_box)) {
      if (dia_object_is_selectable(obj)) {
	selected_list = g_list_prepend(selected_list, obj);
      }
    }

    list = g_list_next(list);
  }

  return selected_list;
}

/** Find objects entirely containing a rectangle.
 * @param layer The layer to search for objects in.
 * @param rect The rectangle that the objects should surround.
 * @return A list containing the objects that entirely contain the
 *  rectangle.  The list should be freed by the caller.
 */
GList *
layer_find_objects_containing_rectangle(Layer *layer, Rectangle *rect)
{
  GList *list;
  GList *selected_list;
  DiaObject *obj;

  selected_list = NULL;
  list = layer->objects;
  while (list != NULL) {
    obj = (DiaObject *)list->data;

    if (rectangle_in_rectangle(&obj->bounding_box, rect)) {
      if (dia_object_is_selectable(obj)) {
	selected_list = g_list_prepend(selected_list, obj);
      }
    }

    list = g_list_next(list);
  }

  return selected_list;
}


/** Find the object closest to the given point in the layer,
 * no further away than maxdist, and not included in avoid.
 * Stops it if finds an object that includes the point.
 * @param layer The layer to search in.
 * @param pos The point to compare to.
 * @param maxdist The maximum distance the object can be from the point.
 * @param avoid A list of objects that cannot be returned by this search.
 * @return The closest object, or NULL if no allowed objects are closer than
 *  maxdist.
 */
DiaObject *
layer_find_closest_object_except(Layer *layer, Point *pos,
				 real maxdist, GList *avoid)
{
  GList *l;
  DiaObject *closest;
  DiaObject *obj;
  real dist;
  GList *avoid_tmp;

  closest = NULL;

  for (l = layer->objects; l!=NULL; l = g_list_next(l)) {
    obj = (DiaObject *) l->data;

    /* Check bounding box here too. Might give speedup. */
    dist = obj->ops->distance_from(obj, pos);

    if (maxdist-dist > 0.00000001) {
      for (avoid_tmp = avoid; avoid_tmp != NULL; avoid_tmp = avoid_tmp->next) {
	if (avoid_tmp->data == obj) {
	  goto NEXTOBJECT;
	}
      }
      closest = obj;
    }
  NEXTOBJECT:
  ;
  }

  /* If the object is within a closed group, find the group. */
  closest = dia_object_get_parent_with_flags(closest,
					     DIA_OBJECT_GRABS_CHILD_INPUT);

  return closest;
}

/** Find the object closest to the given point in the layer,
 * no further away than maxdist.
 * Stops it if finds an object that includes the point.
 * @param layer The layer to search in.
 * @param pos The point to compare to.
 * @param maxdist The maximum distance the object can be from the point.
 * @return The closest object, or NULL if none are closer than maxdist.
 */
DiaObject *
layer_find_closest_object(Layer *layer, Point *pos, real maxdist)
{
  return layer_find_closest_object_except(layer, pos, maxdist, NULL);
}


/** Find the connectionpoint closest to pos in a layer.
 * @param layer
 * @param closest
 * @param pos
 * @param notthis
 * @return
 */
real
layer_find_closest_connectionpoint(Layer *layer,
					ConnectionPoint **closest,
					Point *pos,
					DiaObject *notthis)
{
  GList *l;
  DiaObject *obj;
  ConnectionPoint *cp;
  real mindist, dist;
  int i;

  mindist = 1000000.0; /* Realy big value... */

  *closest = NULL;

  for (l = layer->objects; l!=NULL; l = g_list_next(l) ) {
    obj = (DiaObject *) l->data;

    if (obj == notthis) continue;
    if (obj != dia_object_get_parent_with_flags(obj, DIA_OBJECT_GRABS_CHILD_INPUT))
      continue;
    for (i=0;i<obj->num_connections;i++) {
      cp = obj->connections[i];
      /* Note: Uses manhattan metric for speed... */
      dist = distance_point_point_manhattan(pos, &cp->pos);
      if (dist<mindist) {
	mindist = dist;
	*closest = cp;
      }
    }

 }

  return mindist;
}

int
layer_update_extents(Layer *layer)
{
  GList *l;
  DiaObject *obj;
  Rectangle new_extents;

  l = layer->objects;
  if (l!=NULL) {
    obj = (DiaObject *) l->data;
    new_extents = obj->bounding_box;
    l = g_list_next(l);

    while(l!=NULL) {
      const Rectangle *bbox;
      obj = (DiaObject *) l->data;
      /* don't consider empty (or broken) objects in the overall extents */
      bbox = &obj->bounding_box;
      if (bbox->right > bbox->left && bbox->bottom > bbox->top)
        rectangle_union(&new_extents, &obj->bounding_box);
      l = g_list_next(l);
    }
  } else {
    new_extents = invalid_extents;
  }

  if (rectangle_equals(&new_extents,&layer->extents)) return FALSE;

  layer->extents = new_extents;
  return TRUE;
}

void
layer_replace_object_with_list(Layer *layer, DiaObject *remove_obj,
			       GList *insert_list)
{
  GList *list;

  list = g_list_find(layer->objects, remove_obj);

  g_assert(list!=NULL);
  set_parent_layer(remove_obj, NULL);
  dynobj_list_remove_object(remove_obj);
  g_list_foreach(insert_list, set_parent_layer, layer);

  if (list->prev == NULL) {
    layer->objects = insert_list;
  } else {
    list->prev->next = insert_list;
    insert_list->prev = list->prev;
  }
  if (list->next != NULL) {
    GList *last;
    last = g_list_last(insert_list);
    last->next = list->next;
    list->next->prev = last;
  }
  g_list_free_1(list);
}

static void
layer_remove_dynobj(gpointer obj, gpointer userdata)
{
  dynobj_list_remove_object((DiaObject*)obj);
}

void layer_set_object_list(Layer *layer, GList *list)
{
  g_list_foreach(layer->objects, set_parent_layer, NULL);
  g_list_foreach(layer->objects, layer_remove_dynobj, NULL);
  g_list_free(layer->objects);
  layer->objects = list;
  g_list_foreach(layer->objects, set_parent_layer, layer);
}

DiagramData *
layer_get_parent_diagram(Layer *layer)
{
  return layer->parent_diagram;
}
