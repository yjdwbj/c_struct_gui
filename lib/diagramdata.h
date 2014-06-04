/* Dia -- an diagram creation/manipulation program -*- c -*-
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
/*! \file diagramdata.h -- Describing the base class of diagrams */
#ifndef DIAGRAMDATA_H
#define DIAGRAMDATA_H

#include <glib.h>
#include <string.h>

#include "diatypes.h"
#include "color.h"
#include "geometry.h"
#include "diavar.h"
#include "paper.h"

G_BEGIN_DECLS

/*!
 * \brief Helper to create new diagram
 */
struct _NewDiagramData {
  gchar *papertype;
  gfloat tmargin, bmargin, lmargin, rmargin;
  gboolean is_portrait;
  gfloat scaling;
  gboolean fitto;
  gint fitwidth, fitheight;
  Color bg_color, pagebreak_color, grid_color;
  int compress_save;
  gchar *unit, *font_unit;
};

DIAVAR  GType diagram_data_get_type (void) G_GNUC_CONST;

#define DIA_TYPE_DIAGRAM_DATA           (diagram_data_get_type ())
#define DIA_DIAGRAM_DATA(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_DIAGRAM_DATA, DiagramData))
#define DIA_DIAGRAM_DATA_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_DIAGRAM_DATA, DiagramDataClass))
#define DIA_IS_DIAGRAM_DATA(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_DIAGRAM_DATA))
#define DIA_DIAGRAM_DATA_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_DIAGRAM_DATA, DiagramDataClass))

/*
 * \brief Base class for diagrams. This is the only stuff plug-ins should see about diagrams.
 */
struct _DiagramData {
  GObject parent_instance; /*!< inheritance in C */

  Rectangle extents;      /*!< The extents of the diagram        */

  Color bg_color;         /*!< The diagrams background color */

  PaperInfo paper;        /*!< info about the page info for the diagram */
  gboolean is_compressed; /*!< TRUE if by default it should be save compressed.
			     The user can override this in Save As... */

  GPtrArray *layers;     /*!< Layers ordered by decreasing z-order */
  Layer *active_layer;   /*!< The active layer, Defensive programmers check for NULL */

  guint selected_count_private; /*!< kept for binary compatibility and sanity, don't use ! */
  GList *selected;        /*!< List of objects that are selected,
			     all from the active layer! */

  /** List of text fields (foci) that can be edited in the diagram.
   *  Updated by focus.c */
  GList *text_edits;
  /** Units and font units used in this diagram.  Default cm and point */
  gchar *unit, *font_unit;
  /** The focus (from text_edits) that's currently being edited, if any.
   *  Updated by focus.c */
  Focus *active_text_edit;
  gboolean readytodownload;
};

/**
 * \brief DiagramData vtable
 */
typedef struct _DiagramDataClass {
  GObjectClass parent_class;

  /* Signals */
  void (* object_add)        (DiagramData*, Layer*, DiaObject*);
  void (* object_remove)     (DiagramData*, Layer*, DiaObject*);

} DiagramDataClass;

/*!
 * \brief A diagram consists of layers holding objects
 *
 * \todo : make this a GObject as well
 */
struct _Layer {
  char *name;             /*!< */
  Rectangle extents;      /*!< The extents of the layer        */

  GList *objects;         /*!< List of objects in the layer,
			     sorted by decreasing z-valued,
			     objects can ONLY be connected to objects
			     in the same layer! */

  gboolean visible;
  gboolean connectable;   /*!< Whether the layer can currently be connected to.
			     The selected layer is by default connectable */

  DiagramData *parent_diagram; /*!< Back-pointer to the diagram.  This
				  must only be set by functions internal
				  to the diagram, and accessed via
				  layer_get_parent_diagram() */
  GHashTable *defnames; /* 所有控件显示的名字的链表.*/
  gpointer smd; /* SaveMusicDialog 这用存放音乐文件管理的数据，这个在当前层是公有的*/
  gpointer sid;  /* SaveIdDialog 这用存放行为ID管理的数据，这个在当前层是公有的*/
};

DIAVAR Layer *new_layer (char *name, DiagramData *parent);
DIAVAR void layer_destroy(Layer *layer);

DIAVAR void data_raise_layer(DiagramData *data, Layer *layer);
DIAVAR void data_lower_layer(DiagramData *data, Layer *layer);

DIAVAR void data_add_layer(DiagramData *data, Layer *layer);
DIAVAR void data_add_layer_at(DiagramData *data, Layer *layer, int pos);
DIAVAR void data_set_active_layer(DiagramData *data, Layer *layer);
DIAVAR void data_delete_layer(DiagramData *data, Layer *layer);
DIAVAR int  data_layer_get_index (const DiagramData *data, const Layer *layer);

DIAVAR void data_select(DiagramData *data, DiaObject *obj);
DIAVAR void data_unselect(DiagramData *data, DiaObject *obj);
DIAVAR void data_remove_all_selected(DiagramData *data);
DIAVAR  gboolean data_update_extents(DiagramData *data); /* returns true if changed. */
DIAVAR GList *data_get_sorted_selected(DiagramData *data);
DIAVAR GList *data_get_sorted_selected_remove(DiagramData *data);
DIAVAR void data_set_unit(DiagramData *data, gchar *unit);
DIAVAR gchar *data_get_unit(DiagramData *data);
DIAVAR float data_get_unit_multiplier(DiagramData *data);
DIAVAR void data_set_font_unit(DiagramData *data, gchar *unit);
DIAVAR gchar *data_get_font_unit(DiagramData *data);
DIAVAR float data_get_font_unit_multiplier(DiagramData *data);
DIAVAR void data_emit(DiagramData *data,Layer *layer,DiaObject* obj,const char *signal_name);

DIAVAR void data_foreach_object (DiagramData *data, GFunc func, gpointer user_data);


typedef void (*ObjectRenderer)(DiaObject *obj, DiaRenderer *renderer,
			       int active_layer,
			       gpointer data);
DIAVAR void data_render(DiagramData *data, DiaRenderer *renderer, Rectangle *update,
		 ObjectRenderer obj_renderer /* Can be NULL */,
		 gpointer gdata);
DIAVAR void layer_render(Layer *layer, DiaRenderer *renderer, Rectangle *update,
		  ObjectRenderer obj_renderer /* Can be NULL */,
		  gpointer data,
		  int active_layer);

DIAVAR int layer_object_index(Layer *layer, DiaObject *obj);
DIAVAR void layer_add_object(Layer *layer, DiaObject *obj);
DIAVAR void layer_add_object_at(Layer *layer, DiaObject *obj, int pos);
DIAVAR void layer_add_objects(Layer *layer, GList *obj_list);
DIAVAR void layer_add_objects_first(Layer *layer, GList *obj_list);
DIAVAR void layer_remove_object(Layer *layer, DiaObject *obj);
DIAVAR void layer_remove_objects(Layer *layer, GList *obj_list);
DIAVAR GList *layer_find_objects_intersecting_rectangle(Layer *layer, Rectangle*rect);
DIAVAR GList *layer_find_objects_in_rectangle(Layer *layer, Rectangle *rect);
DIAVAR GList *layer_find_objects_containing_rectangle(Layer *layer, Rectangle *rect);
DIAVAR DiaObject *layer_find_closest_object(Layer *layer, Point *pos, real maxdist);
DIAVAR DiaObject *layer_find_closest_object_except(Layer *layer, Point *pos,
					 real maxdist, GList *avoid);
DIAVAR real layer_find_closest_connectionpoint(Layer *layer,
					ConnectionPoint **closest,
					Point *pos,
					DiaObject *notthis);
DIAVAR int layer_update_extents(Layer *layer); /* returns true if changed. */
DIAVAR void layer_replace_object_with_list(Layer *layer, DiaObject *obj,
				    GList *list);
DIAVAR void layer_set_object_list(Layer *layer, GList *list);
DIAVAR DiagramData *layer_get_parent_diagram(Layer *layer);
/* Make sure all objects that are in the layer and not in the new
   list eventually gets destroyed. */

G_END_DECLS

#endif /* DIAGRAMDATA_H */






