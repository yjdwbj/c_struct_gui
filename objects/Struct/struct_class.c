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
 * File:    class.c
 *
 * Purpose: This file contains implementation of the "class" code.
 */

/** \file objects/STRUCT/class.c  Implementation of the 'STRUCT - Class' type */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "intl.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "diamenu.h"

#include "struct_class.h"

#include "pixmaps/structclass.xpm"

#include "debug.h"
#include "sheet.h"
#include "filedlg.h"



extern FactoryStructItemAll *factoryContainer;


#define STRUCTCLASS_BORDER 0.1
#define STRUCTCLASS_UNDERLINEWIDTH 0.05
#define STRUCTCLASS_TEMPLATE_OVERLAY_X 2.3
#define STRUCTCLASS_TEMPLATE_OVERLAY_Y 0.3

extern MusicFileManagerOpts  mfmo_opts;





static real structclass_distance_from(STRUCTClass *structclass, Point *point);
static void structclass_select(STRUCTClass *structclass, Point *clicked_point,
                               DiaRenderer *interactive_renderer);
static ObjectChange* structclass_move_handle(STRUCTClass *structclass, Handle *handle,
        Point *to, ConnectionPoint *cp, HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* structclass_move(STRUCTClass *structclass, Point *to);
static void structclass_draw(STRUCTClass *structclass, DiaRenderer *renderer);
//static DiaObject *structclass_create(Point *startpoint,
//			       void *user_data,
//			       Handle **handle1,
//			       Handle **handle2);
static void structclass_destroy(STRUCTClass *structclass);
static DiaObject *structclass_copy(STRUCTClass *structclass);
void factory_handle_entry_item(SaveEntry* sey,FactoryStructItem *fst);
//static void structclass_save(STRUCTClass *structclass, ObjectNode obj_node,
//			  const char *filename);



//static DiaObject *structclass_load(ObjectNode obj_node, int version,
//			     const char *filename);
void factory_read_initial_to_struct(STRUCTClass *fclass);
void factory_read_object_comobox_value_from_file(AttributeNode attr_node, ActionID *aid);
static DiaMenu * structclass_object_menu(DiaObject *obj, Point *p);
static ObjectChange *structclass_show_comments_callback(DiaObject *obj, Point *pos, gpointer data);

static PropDescription *structclass_describe_props(STRUCTClass *structclass);


static PropDescription *structtest_describe_props(STRUCTClass *structclass); // 2013-3-13 lcy change first function
static void structclass_get_props(STRUCTClass *structclass, GPtrArray *props);
static void structclass_set_props(STRUCTClass *structclass, GPtrArray *props);

static PropDescription *factory_describe_props(STRUCTClass *structclass);
//static void factory_get_props(STRUCTClass *structclass, GPtrArray *props);
//static void factory_set_props(STRUCTClass *structclass, GPtrArray *props);
static void factory_struct_items_save(STRUCTClass *structclass, ObjectNode obj_node,
                                      const char *filename);

static DiaObject * factory_struct_items_load(ObjectNode obj_node, int version,
        const char *filename);


static DiaObject * factory_struct_items_create(Point *startpoint,
        void *user_data,
        Handle **handle1,
        Handle **handle2);



static void factory_update_index(STRUCTClass *fclass);

static void fill_in_fontdata(STRUCTClass *structclass);
static int structclass_num_dynamic_connectionpoints(STRUCTClass *class);
static void factory_read_props_from_widget(gpointer key,gpointer value ,gpointer user_data);

void factory_rename_structclass(STRUCTClass *fclass);
void factory_update_view_names(STRUCTClass *fclass);


//static ObjectChange *_structclass_apply_props_from_dialog(STRUCTClass *structclass, GtkWidget *widget);
ObjectChange *factory_apply_props_from_dialog(STRUCTClass *structclass, GtkWidget *widget);


void factory_change_view_name(STRUCTClass *startc);

static ObjectTypeOps structclass_type_ops =
{
    (CreateFunc) factory_struct_items_create,
//  (LoadFunc)   structclass_load,
// (SaveFunc)   structclass_save
    (LoadFunc)  factory_struct_items_load,
    (SaveFunc)  factory_struct_items_save

};

/**
 * This is the type descriptor for a STRUCT - Class. It contains the
 * information used by Dia to create an object of this type. The structure
 * of this data type is defined in the header file object.h. When a
 * derivation of class is required, then this type can be copied and then
 * change the name and any other fields that are variances from the base
 * type.
*/


DiaObjectType structclass_type =
{
    "STRUCT - Class",   /* name */
    NULL,                      /* version */
    (char **) structclass_xpm,  /* pixmap */

    &structclass_type_ops,       /* ops */
    NULL,
    (void*)0
};


DiaObjectType template_type =
{
    TYPE_TEMPLATE,   /* name */
    NULL,                      /* version */
    (char **) structclass_xpm,  /* pixmap */

    &structclass_type_ops,       /* ops */
    NULL,
    (void*)0
};

/* 这里专为系统信息做了一个类型　*/
FactorySystemType factory_systeminfo_type =
{
    "SystemInfo",
    NULL,
    (char **) structclass_xpm,  /* pixmap */

    factory_systeminfo_callback,       /* ops */
    NULL,
    (void*)0
};

/** \brief vtable for STRUCTClass */
static ObjectOps structclass_ops =
{
    (DestroyFunc)         structclass_destroy,
    (DrawFunc)            structclass_draw,
    (DistanceFunc)        structclass_distance_from,
    (SelectFunc)          factory_select,
    (CopyFunc)            structclass_copy,
    (MoveFunc)            structclass_move,
    (MoveHandleFunc)      structclass_move_handle,
// (GetPropertiesFunc)   structclass_get_properties,
    (GetPropertiesFunc)   factory_get_properties,
//  (ApplyPropertiesDialogFunc) _structclass_apply_props_from_dialog,
    (ApplyPropertiesDialogFunc)   factory_apply_props_from_dialog,
    (ObjectMenuFunc)      structclass_object_menu,
    (DescribePropsFunc)   structclass_describe_props,
    (GetPropsFunc)        structclass_get_props,
    (SetPropsFunc)        structclass_set_props,
    (TextEditFunc) 0,
    (ApplyPropertiesListFunc) object_apply_props,
    (UpdateData) 0,
    (ConnectionTwoObject) 0,
    (UpdateObjectIndex)   factory_update_index,
    (ObjectRename) factory_rename_structclass,
    (UpdateObjectVName) factory_update_view_names,
    (SearchConnectedLink)  factory_search_connected_link,
    (UpdateObjectsFillColor) factory_set_fill_color,
    (ResetObjectsToDefaultColor) factory_reset_object_color_to_default,
    (AddObjectToBTree) factory_add_self_to_btree,
    (RecursionFindOcombox) factory_class_ocombox_foreach
//    (ReNameNewObj) factory_rename_new_obj,
//    (ReConnectionNewObj) factory_reconnection_new_obj
};




//extern PropDescDArrayExtra structattribute_extra;
//extern PropDescDArrayExtra structoperation_extra;
//extern PropDescDArrayExtra structparameter_extra;
//extern PropDescDArrayExtra structformalparameter_extra;




/** Properties of STRUCTClass */
static PropDescription structclass_props[] =
{
    ELEMENT_COMMON_PROPERTIES,
    PROP_STD_LINE_WIDTH_OPTIONAL,
    /* can't use PROP_STD_TEXT_COLOUR_OPTIONAL cause it has PROP_FLAG_DONT_SAVE. It is designed to fill the Text object - not some subset */
    PROP_STD_TEXT_COLOUR_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
    PROP_STD_LINE_COLOUR_OPTIONAL,
    PROP_STD_FILL_COLOUR_OPTIONAL,

    PROP_STD_NOTEBOOK_BEGIN,
    PROP_NOTEBOOK_PAGE("class", PROP_FLAG_DONT_MERGE, N_("Class")),
    {
        "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_NO_DEFAULTS,
        N_("Name"), NULL, NULL
    },
//  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Stereotype"), NULL, NULL },
//  { "comment", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Comment"), NULL, NULL },
//  { "abstract", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Abstract"), NULL, NULL },
//  { "template", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_NO_DEFAULTS,
//  N_("Template"), NULL, NULL },

//  { "suppress_attributes", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Suppress Attributes"), NULL, NULL },
//  { "suppress_operations", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Suppress Operations"), NULL, NULL },
//  { "visible_attributes", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Visible Attributes"), NULL, NULL },
//  { "visible_operations", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Visible Operations"), NULL, NULL },
//  { "visible_comments", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Visible Comments"), NULL, NULL },
//  { "wrap_operations", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Wrap Operations"), NULL, NULL },
//  { "wrap_after_char", PROP_TYPE_INT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Wrap after char"), NULL, NULL },
//  { "comment_line_length", PROP_TYPE_INT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Comment line length"), NULL, NULL},
//  { "comment_tagging", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Comment tagging"), NULL, NULL},

    /* all this just to make the defaults selectable ... */
    PROP_NOTEBOOK_PAGE("font", PROP_FLAG_DONT_MERGE, N_("Font")),
    PROP_MULTICOL_BEGIN("class"),
    PROP_MULTICOL_COLUMN("font"),
    /* FIXME: apparently multicol does not work correctly, this should be FIRST column */
    {
        "normal_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
        N_("Normal"), NULL, NULL
    },
//  { "polymorphic_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Polymorphic"), NULL, NULL },
//  { "abstract_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Abstract"), NULL, NULL },
    {
        "classname_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
        N_("Classname"), NULL, NULL
    },
//  { "abstract_classname_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Abstract Classname"), NULL, NULL },
//  { "comment_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Comment"), NULL, NULL },

    PROP_MULTICOL_COLUMN("height"),
    {
        "normal_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
        N_(" "), NULL, NULL
    },
    {
        "polymorphic_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
        N_(" "), NULL, NULL
    },
    {
        "abstract_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
        N_(" "), NULL, NULL
    },
    {
        "classname_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
        N_(" "), NULL, NULL
    },
    {
        "abstract_classname_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
        N_(" "), NULL, NULL
    },
    {
        "comment_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
        N_(" "), NULL, NULL
    },
    PROP_MULTICOL_END("class"),
    PROP_STD_NOTEBOOK_END,

    /* these are used during load, but currently not during save */
//  { "attributes", PROP_TYPE_DARRAY, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_DONT_MERGE | PROP_FLAG_NO_DEFAULTS,
//  N_("Attributes"), NULL, NULL /* structattribute_extra */ },
//  { "operations", PROP_TYPE_DARRAY, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_DONT_MERGE | PROP_FLAG_NO_DEFAULTS,
//  N_("Operations"), NULL, NULL /* structoperations_extra */ },
//  /* the naming is questionable, but kept for compatibility */
//  { "templates", PROP_TYPE_DARRAY, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_DONT_MERGE | PROP_FLAG_NO_DEFAULTS,
//  N_("Template Parameters"), NULL, NULL /* structformalparameters_extra */ },

    PROP_DESC_END
};

//ObjectChange *
//_structclass_apply_props_from_dialog(STRUCTClass *structclass, GtkWidget *widget)
//{
//  DiaObject *obj = &structclass->element.object;
//  /* fallback, if it isn't our dialog, e.g. during multiple selection change */
//  if (!structclass->properties_dialog)
//    return object_apply_props_from_dialog (obj, widget);
//  else
//    return structclass_apply_props_from_dialog (structclass, widget);
//}


static PropDescription *
structclass_describe_props(STRUCTClass *structclass)
{
    if (structclass_props[0].quark == 0)
    {
        int i = 0;

        prop_desc_list_calculate_quarks(structclass_props);
        while (structclass_props[i].name != NULL)
        {
            /* can't do this static, at least not on win32
             * due to relocation (initializer not a constant)
             */
//      if (0 == strcmp(structclass_props[i].name, "attributes"))
//        structclass_props[i].extra_data = &structattribute_extra;
//      else if (0 == strcmp(structclass_props[i].name, "operations")) {
//        PropDescription *records = structoperation_extra.common.record;
//        int j = 0;
//
//        structclass_props[i].extra_data = &structoperation_extra;
//	while (records[j].name != NULL) {
//          if (0 == strcmp(records[j].name, "parameters"))
//	    records[j].extra_data = &structparameter_extra;
//	  j++;
//	}
//      }
//      else if (0 == strcmp(structclass_props[i].name, "templates"))
//        structclass_props[i].extra_data = &structformalparameter_extra;

            i++;
        }
    }
    return structclass_props;
}

static PropOffset structclass_offsets[] =
{
    ELEMENT_COMMON_PROPERTIES_OFFSETS,

    { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(STRUCTClass, line_width) },
    { "text_colour", PROP_TYPE_COLOUR, offsetof(STRUCTClass, text_color) },
    { "line_colour", PROP_TYPE_COLOUR, offsetof(STRUCTClass, line_color) },
    { "fill_colour", PROP_TYPE_COLOUR, offsetof(STRUCTClass, fill_color) },
    { "name", PROP_TYPE_STRING, offsetof(STRUCTClass, name) },
//  { "stereotype", PROP_TYPE_STRING, offsetof(STRUCTClass, stereotype) },
//  { "comment", PROP_TYPE_STRING, offsetof(STRUCTClass, comment) },
//  { "abstract", PROP_TYPE_BOOL, offsetof(STRUCTClass, abstract) },
//  { "template", PROP_TYPE_BOOL, offsetof(STRUCTClass, template) },
//  { "suppress_attributes", PROP_TYPE_BOOL, offsetof(STRUCTClass , suppress_attributes) },
//  { "visible_attributes", PROP_TYPE_BOOL, offsetof(STRUCTClass , visible_attributes) },
//  { "visible_comments", PROP_TYPE_BOOL, offsetof(STRUCTClass , visible_comments) },
//  { "suppress_operations", PROP_TYPE_BOOL, offsetof(STRUCTClass , suppress_operations) },
//  { "visible_operations", PROP_TYPE_BOOL, offsetof(STRUCTClass , visible_operations) },
//  { "visible_comments", PROP_TYPE_BOOL, offsetof(STRUCTClass , visible_comments) },
//  { "wrap_operations", PROP_TYPE_BOOL, offsetof(STRUCTClass , wrap_operations) },
//  { "wrap_after_char", PROP_TYPE_INT, offsetof(STRUCTClass , wrap_after_char) },
//  { "comment_line_length", PROP_TYPE_INT, offsetof(STRUCTClass, comment_line_length) },
//  { "comment_tagging", PROP_TYPE_BOOL, offsetof(STRUCTClass, comment_tagging) },

    /* all this just to make the defaults selectable ... */
    PROP_OFFSET_MULTICOL_BEGIN("class"),
    PROP_OFFSET_MULTICOL_COLUMN("font"),
    { "normal_font", PROP_TYPE_FONT, offsetof(STRUCTClass, normal_font) },
//  { "abstract_font", PROP_TYPE_FONT, offsetof(STRUCTClass, abstract_font) },
//  { "polymorphic_font", PROP_TYPE_FONT, offsetof(STRUCTClass, polymorphic_font) },
    { "classname_font", PROP_TYPE_FONT, offsetof(STRUCTClass, classname_font) },
//  { "abstract_classname_font", PROP_TYPE_FONT, offsetof(STRUCTClass, abstract_classname_font) },
//  { "comment_font", PROP_TYPE_FONT, offsetof(STRUCTClass, comment_font) },

    PROP_OFFSET_MULTICOL_COLUMN("height"),
    { "normal_font_height", PROP_TYPE_REAL, offsetof(STRUCTClass, font_height) },
    { "abstract_font_height", PROP_TYPE_REAL, offsetof(STRUCTClass, abstract_font_height) },
    { "polymorphic_font_height", PROP_TYPE_REAL, offsetof(STRUCTClass, polymorphic_font_height) },
    { "classname_font_height", PROP_TYPE_REAL, offsetof(STRUCTClass, classname_font_height) },
    { "abstract_classname_font_height", PROP_TYPE_REAL, offsetof(STRUCTClass, abstract_classname_font_height) },
    { "comment_font_height", PROP_TYPE_REAL, offsetof(STRUCTClass, comment_font_height) },
    PROP_OFFSET_MULTICOL_END("class"),

//  { "operations", PROP_TYPE_DARRAY, offsetof(STRUCTClass , operations) },
//  { "attributes", PROP_TYPE_DARRAY, offsetof(STRUCTClass , attributes) } ,
//  { "templates",  PROP_TYPE_DARRAY, offsetof(STRUCTClass , formal_params) } ,

    { NULL, 0, 0 },
};

static void
structclass_get_props(STRUCTClass * structclass, GPtrArray *props)
{
    object_get_props_from_offsets(&structclass->element.object,
                                  structclass_offsets, props);
}

static DiaMenuItem structclass_menu_items[] =
{
    {
        N_("Show Comments"), structclass_show_comments_callback, NULL,
        DIAMENU_ACTIVE|DIAMENU_TOGGLE
    },
};

static DiaMenu structclass_menu =
{
    N_("Class"),
    sizeof(structclass_menu_items)/sizeof(DiaMenuItem),
    structclass_menu_items,
    NULL
};

DiaMenu *
structclass_object_menu(DiaObject *obj, Point *p)
{
//        structclass_menu_items[0].active = DIAMENU_ACTIVE|DIAMENU_TOGGLE|
//                (((STRUCTClass *)obj)->visible_comments?DIAMENU_TOGGLE_ON:0);

    return &structclass_menu;
}

typedef struct _CommentState
{
    ObjectState state;
    gboolean    visible_comments;
} CommentState;
static ObjectState*
_comment_get_state (DiaObject *obj)
{
    CommentState *state = g_new (CommentState,1);
    state->state.free = NULL; /* we don't have any pointers to free */
//  state->visible_comments = ((STRUCTClass *)obj)->visible_comments;
    return (ObjectState *)state;
}
static void
_comment_set_state (DiaObject *obj, ObjectState *state)
{
//  ((STRUCTClass *)obj)->visible_comments = ((CommentState *)state)->visible_comments;
    g_free (state); /* rather strange convention set_state consumes the state */
    structclass_calculate_data((STRUCTClass *)obj);
    structclass_update_data((STRUCTClass *)obj);
}

ObjectChange *
structclass_show_comments_callback(DiaObject *obj, Point *pos, gpointer data)
{
    ObjectState *old_state = _comment_get_state(obj);
    ObjectChange *change = new_object_state_change(obj, old_state, _comment_get_state, _comment_set_state );

//  ((STRUCTClass *)obj)->visible_comments = !((STRUCTClass *)obj)->visible_comments;
    structclass_calculate_data((STRUCTClass *)obj);
    structclass_update_data((STRUCTClass *)obj);
    return change;
}

static void
structclass_set_props(STRUCTClass *structclass, GPtrArray *props)
{
    /* now that operations/attributes can be set here as well we need to
     * take for the number of connections update as well
     * Note that due to a hack in structclass_load, this is called before
     * the normal connection points are set up.
     */
    DiaObject *obj = &structclass->element.object;
//  GList *list;
    int num;

    object_set_props_from_offsets(&structclass->element.object, structclass_offsets,
                                  props);

    num = STRUCTCLASS_CONNECTIONPOINTS + structclass_num_dynamic_connectionpoints(structclass);

#ifdef STRUCT_MAINPOINT
    obj->num_connections = num + 1;
#else
    obj->num_connections = num;
#endif

    obj->connections =  g_realloc(obj->connections, obj->num_connections*sizeof(ConnectionPoint *));

    /* Update data: */
    if (num > STRUCTCLASS_CONNECTIONPOINTS)
    {
//    int i;
        /* this is just updating pointers to ConnectionPoint, the real connection handling is elsewhere.
         * Note: Can't optimize here on number change cause the ops/attribs may have changed regardless of that.
         */
//    i = STRUCTCLASS_CONNECTIONPOINTS;
//    list = (!structclass->visible_attributes || structclass->suppress_attributes) ? NULL : structclass->attributes;
//    while (list != NULL) {
//      STRUCTAttribute *attr = (STRUCTAttribute *)list->data;
//
//      struct_attribute_ensure_connection_points (attr, obj);
//      obj->connections[i] = attr->left_connection;
//      obj->connections[i]->object = obj;
//      i++;
//      obj->connections[i] = attr->right_connection;
//      obj->connections[i]->object = obj;
//      i++;
//      list = g_list_next(list);
//    }
//    list = (!structclass->visible_operations || structclass->suppress_operations) ? NULL : structclass->operations;
//    while (list != NULL) {
//      STRUCTOperation *op = (STRUCTOperation *)list->data;
//
//      struct_operation_ensure_connection_points (op, obj);
//      obj->connections[i] = op->left_connection;
//      obj->connections[i]->object = obj;
//      i++;
//      obj->connections[i] = op->right_connection;
//      obj->connections[i]->object = obj;
//      i++;
//      list = g_list_next(list);
//    }
    }
#ifdef STRUCT_MAINPOINT
    obj->connections[num] = &structclass->connections[STRUCTCLASS_CONNECTIONPOINTS];
    obj->connections[num]->object = obj;
#endif

    structclass_calculate_data(structclass);
    structclass_update_data(structclass);
#ifdef DEBUG
    /* Would like to sanity check here, but the call to object_load_props
     * in structclass_load means we will be called with inconsistent data. */
    structclass_sanity_check(structclass, "After updating data");
#endif
}

static real
structclass_distance_from(STRUCTClass *structclass, Point *point)
{
    DiaObject *obj = &structclass->element.object;
    return distance_rectangle_point(&obj->bounding_box, point);
}



static void
structclass_select(STRUCTClass *structclass, Point *clicked_point,
                   DiaRenderer *interactive_renderer)
{
    element_update_handles(&structclass->element);
}




/*  2014-4-4 lcy 更新界面上所有对像的ID号*/
static void factory_update_index(STRUCTClass *fclass)
{
    structclass_calculate_data(fclass);
    if(!fclass->isInitial)
    {
        fclass->isInitial = TRUE;
        factory_read_initial_to_struct(fclass);
    }
}


void factory_select(STRUCTClass *fclass, Point *clicked_point,
                    DiaRenderer *interactive_renderer)
{
    element_update_handles(&fclass->element);
    factory_update_index(fclass);
}

static ObjectChange*
structclass_move_handle(STRUCTClass *structclass, Handle *handle,
                        Point *to, ConnectionPoint *cp,
                        HandleMoveReason reason, ModifierKeys modifiers)
{
    assert(structclass!=NULL);
    assert(handle!=NULL);
    assert(to!=NULL);

    assert(handle->id < STRUCTCLASS_CONNECTIONPOINTS);

    return NULL;
}

static ObjectChange*
structclass_move(STRUCTClass *structclass, Point *to)
{
    structclass->element.corner = *to;
    structclass_update_data(structclass);

    return NULL;
}
/**
 * underlines the text at the start point using the text to determine
 * the length of the underline. Draw a line under the text represented by
 * string using the selected renderer, color, and line width.  Since
 * drawing this line will change the line width used by DIA, the current
 * line width that DIA is using is also passed so it can be restored once
 * the line has been drawn.
 *
 * @param  renderer     the renderer that will draw the line
 * @param  StartPoint   the start of the line to be drawn
 * @param  font         the font used to draw the text being underlined
 * @param  font_height  the size in the y direction of the font used to draw the text
 * @param  string       the text string that is to be underlined
 * @param  color        the color of the line to draw
 * @param  line_width   default line thickness
 * @param  underline_width   the thickness of the line to draw
 *
 */
//static void
//struct_underline_text(DiaRenderer  *renderer,
//                      Point         StartPoint,
//                      DiaFont      *font,
//                      real          font_height,
//                      gchar        *string,
//                      Color        *color,
//                      real          line_width,
//                      real          underline_width)
//{
//    DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
//    Point    UnderlineStartPoint;
//    Point    UnderlineEndPoint;
//    gchar *whitespaces;
//    int first_non_whitespace = 0;
//
//    UnderlineStartPoint = StartPoint;
//    UnderlineStartPoint.y += font_height * 0.1;
//    UnderlineEndPoint = UnderlineStartPoint;
//
//    whitespaces = string;
//    while (whitespaces &&
//            g_unichar_isspace(g_utf8_get_char(whitespaces)))
//    {
//        whitespaces = g_utf8_next_char(whitespaces);
//    }
//    first_non_whitespace = whitespaces - string;
//    whitespaces = g_strdup(string);
//    whitespaces[first_non_whitespace] = '\0';
//    UnderlineStartPoint.x += dia_font_string_width(whitespaces, font, font_height);
//    g_free(whitespaces);
//    UnderlineEndPoint.x += dia_font_string_width(string, font, font_height);
//    renderer_ops->set_linewidth(renderer, underline_width);
//    renderer_ops->draw_line(renderer, &UnderlineStartPoint, &UnderlineEndPoint, color);
//    renderer_ops->set_linewidth(renderer, line_width);
//}

/**
 * Create a documentation tag from a comment.
 *
 * First a string is created containing only the text
 * "{documentation = ". Then the contents of the comment string
 * are added but wrapped. This is done by first looking for any
 * New Line characters. If the line segment is longer than the
 * WrapPoint would allow, the line is broken at either the
 * first whitespace before the WrapPoint or if there are no
 * whitespaces in the segment, at the WrapPoint.  This
 * continues until the entire string has been processed and
 * then the resulting new string is returned. No attempt is
 * made to rejoin any of the segments, that is all New Lines
 * are treated as hard newlines. No syllable matching is done
 * either so breaks in words will sometimes not make real
 * sense.
 * <p>
 * Finally, since this function returns newly created dynamic
 * memory the caller must free the memory to prevent memory
 * leaks.
 *
 * @param  comment       The comment to be wrapped to the line length limit
 * @param  WrapPoint     The maximum line length allowed for the line.
 * @param  NumberOfLines The number of comment lines after the wrapping.
 * @return               a pointer to the wrapped documentation
 *
 *  NOTE:
 *      This function should most likely be move to a source file for
 *      handling global STRUCT functionallity at some point.
 */
static gchar *
struct_create_documentation_tag (gchar * comment,
                                 gboolean tagging,
                                 gint WrapPoint,
                                 gint *NumberOfLines)
{
    gchar  *CommentTag           = tagging ? "{documentation = " : "";
    gint   TagLength             = strlen(CommentTag);
    /* Make sure that there is at least some value greater then zero for the WrapPoint to
     * support diagrams from earlier versions of Dia. So if the WrapPoint is zero then use
     * the taglength as the WrapPoint. If the Tag has been changed such that it has a length
     * of 0 then use 1.
     */
    gint     WorkingWrapPoint = (TagLength<WrapPoint) ? WrapPoint : ((TagLength<=0)?1:TagLength);
    gint     RawLength        = TagLength + strlen(comment) + (tagging?1:0);
    gint     MaxCookedLength  = RawLength + RawLength/WorkingWrapPoint;
    gchar    *WrappedComment  = g_malloc0(MaxCookedLength+1);
    gint     AvailSpace       = WorkingWrapPoint - TagLength;
    gchar    *Scan;
    gchar    *BreakCandidate;
    gunichar ScanChar;
    gboolean AddNL            = FALSE;

    if (tagging)
        strcat(WrappedComment, CommentTag);
    *NumberOfLines = 1;

    while ( *comment )
    {
        /* Skip spaces */
        while ( *comment && g_unichar_isspace(g_utf8_get_char(comment)) )
        {
            comment = g_utf8_next_char(comment);
        }
        /* Copy chars */
        if ( *comment )
        {
            /* Scan to \n or avalable space exhausted */
            Scan = comment;
            BreakCandidate = NULL;
            while ( *Scan && *Scan != '\n' && AvailSpace > 0 )
            {
                ScanChar = g_utf8_get_char(Scan);
                /* We known, that g_unichar_isspace() is not recommended for word breaking;
                 * but Pango usage seems too complex.
                 */
                if ( g_unichar_isspace(ScanChar) )
                    BreakCandidate = Scan;
                AvailSpace--; /* not valid for nonspacing marks */
                Scan = g_utf8_next_char(Scan);
            }
            if ( AvailSpace==0 && BreakCandidate != NULL )
                Scan = BreakCandidate;
            if ( AddNL )
            {
                strcat(WrappedComment, "\n");
                *NumberOfLines+=1;
            }
            AddNL = TRUE;
            strncat(WrappedComment, comment, Scan-comment);
            AvailSpace = WorkingWrapPoint;
            comment = Scan;
        }
    }
    if (tagging)
        strcat(WrappedComment, "}");
    assert(strlen(WrappedComment)<=MaxCookedLength);
    return WrappedComment;
}

/**
 * Draw the comment at the point, p, using the comment font from the
 * class defined by structclass. When complete update the point to reflect
 * the size of data drawn.
 * The comment will have been word wrapped using the function
 * struct_create_documentation_tag, so it may have more than one line on the
 * display.
 *
 * @param   renderer            The Renderer on which the comment is being drawn
 * @param   font                The font to render the comment in.
 * @param   font_height         The Y size of the font used to render the comment
 * @param   text_color          A pointer to the color to use to render the comment
 * @param   comment             The comment string to render
 * @param   comment_tagging     If the {documentation = } tag should be enforced
 * @param   Comment_line_length The maximum length of any one line in the comment
 * @param   p                   The point at which the comment is to start
 * @param   alignment           The method to use for alignment of the font
 * @see   struct_create_documentation
 */
//static void
//struct_draw_comments(DiaRenderer *renderer,
//                     DiaFont     *font,
//                     real         font_height,
//                     Color       *text_color,
//                     gchar       *comment,
//                     gboolean     comment_tagging,
//                     gint         Comment_line_length,
//                     Point       *p,
//                     gint         alignment)
//{
//    gint      NumberOfLines = 0;
//    gint      Index;
//    real      ascent;
//    gchar     *CommentString = 0;
//    gchar     *NewLineP= NULL;
//    gchar     *RenderP;
//
//    DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
//
//    CommentString =
//        struct_create_documentation_tag(comment, comment_tagging, Comment_line_length, &NumberOfLines);
//    RenderP = CommentString;
//    renderer_ops->set_font(renderer, font, font_height);
//    ascent = dia_font_ascent(RenderP, font, font_height);
//    for ( Index=0; Index < NumberOfLines; Index++)
//    {
//        NewLineP = strchr(RenderP, '\n');
//        if ( NewLineP != NULL)
//        {
//            *NewLineP++ = '\0';
//        }
//        if (Index == 0)
//        {
//            p->y += ascent;
//        }
//        else
//        {
//            p->y += font_height;                    /* Advance to the next line */
//        }
//        renderer_ops->draw_string(renderer, RenderP, p, alignment, text_color);
//        RenderP = NewLineP;
//        if ( NewLineP == NULL)
//        {
//            break;
//        }
//    }
//    p->y += font_height - ascent;
//    g_free(CommentString);
//}


/**
 * Draw the name box of the class icon. According to the STRUCT specification,
 * the Name box or compartment is the top most compartment of the class
 * icon. It may contain one or more stereotype declarations, followed by
 * the name of the class. The name may be rendered to indicate abstraction
 * for abstract classes. Following the name is any tagged values such as
 * the {documentation = } tag.
 * <p>
 * Because the start point is the upper left of the class box, templates
 * tend to get lost when created. By applying an offset, they will not be
 * lost. The offset should only be added if the elem->corner.y = 0.
 *
 * @param structclass  The pointer to the class being drawn
 * @param renderer  The pointer to the rendering object used to draw
 * @param elem      The pointer to the element within the class to be drawn
 * @param offset    offset from start point
 * @return  The offset from the start of the class to the bottom of the namebox
 *
 */
static real
structclass_draw_namebox(STRUCTClass *structclass, DiaRenderer *renderer, Element *elem )
{
    DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
    real     font_height;
    real     ascent;
    DiaFont *font;
    Point   StartPoint;
    Point   LowerRightPoint;
    real    Yoffset;
    Color   *text_color = &structclass->text_color;



    StartPoint.x = elem->corner.x;
    StartPoint.y = elem->corner.y;

    Yoffset = elem->corner.y + structclass->namebox_height;

    LowerRightPoint = StartPoint;
    LowerRightPoint.x += elem->width;
    LowerRightPoint.y  = Yoffset;

    /*
     * First draw the outer box and fill color for the class name
     * object
     */
    renderer_ops->fill_rect(renderer, &StartPoint, &LowerRightPoint, &structclass->fill_color);
    renderer_ops->draw_rect(renderer, &StartPoint, &LowerRightPoint, &structclass->line_color);

    /* Start at the midpoint on the X axis */
    StartPoint.x += elem->width / 2.0;
    StartPoint.y += 0.2;

    /* stereotype: */
//  if (structclass->stereotype != NULL && structclass->stereotype[0] != '\0') {
//    gchar *String = structclass->stereotype_string;
//    ascent = dia_font_ascent(String, structclass->normal_font, structclass->font_height);
//    StartPoint.y += ascent;
//    renderer_ops->set_font(renderer, structclass->normal_font, structclass->font_height);
//    renderer_ops->draw_string(renderer,  String, &StartPoint, ALIGN_CENTER, text_color);
//    StartPoint.y += structclass->font_height - ascent;
//  }
//
//  /* name: */
    if (structclass->name != NULL)
    {
        /* 2014-5-5 lcy 这里支持中文显示*/
//        gchar *utf8name = g_locale_from_utf8(structclass->name,-1,NULL,NULL,NULL);
        gchar *utf8name = structclass->name;
//    if (structclass->abstract) {
//      font = structclass->abstract_classname_font;
//      font_height = structclass->abstract_classname_font_height;
//    } else
        {
            font = structclass->classname_font;
            font_height = structclass->classname_font_height;
        }
        ascent = dia_font_ascent(utf8name, font, font_height);
        StartPoint.y += ascent;

        renderer_ops->set_font(renderer, font, font_height);
        renderer_ops->draw_string(renderer,utf8name ,
                                  &StartPoint, ALIGN_CENTER, text_color);
        StartPoint.y += font_height - ascent;
    }
//
//  /* comment */
//  if (structclass->visible_comments && structclass->comment != NULL && structclass->comment[0] != '\0'){
//    struct_draw_comments(renderer, structclass->comment_font ,structclass->comment_font_height,
//                           &structclass->text_color, structclass->comment, structclass->comment_tagging,
//                           structclass->comment_line_length, &StartPoint, ALIGN_CENTER);
//  }
    return Yoffset;
}

/**
 * Draw the attribute box.
 * This attribute box follows the name box in the class icon. If the
 * attributes are not suppress, draw each of the attributes following the
 * STRUCT specification for displaying attributes. Each attribute is preceded
 * by the visibility character, +, - or # depending on whether it is public
 * private or protected. If the attribute is "abstract" it will be rendered
 * using the abstract font otherwise it will be rendered using the normal
 * font. If the attribute is of class scope, static in C++, then it will be
 * underlined. If there is a comment associated with the attribute, that is
 * within the class description, it will be rendered as a struct comment.
 *
 * @param structclass   The pointer to the class being drawn
 * @param renderer   The pointer to the rendering object used to draw
 * @param elem       The pointer to the element within the class to be drawn
 * @param Yoffset    The Y offset from the start of the class at which to draw the attributebox
 * @return           The offset from the start of the class to the bottom of the attributebox
 * @see struct_draw_comments
 */
//static real
//structclass_draw_attributebox(STRUCTClass *structclass, DiaRenderer *renderer, Element *elem, real Yoffset)
//{
//    DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
////  real     font_height;
////  real     ascent;
//    Point    StartPoint;
//    Point    LowerRight;
////  DiaFont *font;
//    Color   *fill_color = &structclass->fill_color;
//    Color   *line_color = &structclass->line_color;
////  Color   *text_color = &structclass->text_color;
////  GList   *list;
//
//    StartPoint.x = elem->corner.x;
//    StartPoint.y = Yoffset;
////  Yoffset   += structclass->attributesbox_height;
//
//    LowerRight   = StartPoint;
//    LowerRight.x += elem->width;
//    LowerRight.y = Yoffset;
//
//    renderer_ops->fill_rect(renderer, &StartPoint, &LowerRight, fill_color);
//    renderer_ops->draw_rect(renderer, &StartPoint, &LowerRight, line_color);
//
////  if (!structclass->suppress_attributes) {
////    gint i = 0;
////    StartPoint.x += (structclass->line_width/2.0 + 0.1);
////    StartPoint.y += 0.1;
////
////    list = structclass->attributes;
////    while (list != NULL)
////    {
////      STRUCTAttribute *attr   = (STRUCTAttribute *)list->data;
////      gchar        *attstr = struct_get_attribute_string(attr);
////
////      if (attr->abstract)  {
////        font = structclass->abstract_font;
////        font_height = structclass->abstract_font_height;
////      }
////      else  {
////        font = structclass->normal_font;
////        font_height = structclass->font_height;
////      }
////      ascent = dia_font_ascent(attstr, font, font_height);
////      StartPoint.y += ascent;
////      renderer_ops->set_font (renderer, font, font_height);
////      renderer_ops->draw_string(renderer, attstr, &StartPoint, ALIGN_LEFT, text_color);
////      StartPoint.y += font_height - ascent;
////
////      if (attr->class_scope) {
////        struct_underline_text(renderer, StartPoint, font, font_height, attstr, line_color,
////                        structclass->line_width, STRUCTCLASS_UNDERLINEWIDTH );
////      }
////
////      if (structclass->visible_comments && attr->comment != NULL && attr->comment[0] != '\0') {
////        struct_draw_comments(renderer, structclass->comment_font ,structclass->comment_font_height,
////                               &structclass->text_color, attr->comment, structclass->comment_tagging,
////                               structclass->comment_line_length, &StartPoint, ALIGN_LEFT);
////        StartPoint.y += structclass->comment_font_height/2;
////      }
////      list = g_list_next(list);
////      i++;
////      g_free (attstr);
////    }
////  }
//    return Yoffset;
//}


/**
 * Draw the operations box. The operations block follows the attribute box
 * if it is visible. If the operations are not suppressed, they are
 * displayed in the operations box. Like the attributes, operations have
 * visibility characters, +,-, and # indicating whether the are public,
 * private or protected. The operations are rendered in different fonts
 * depending on whether they are abstract (pure virtual), polymorphic
 * (virtual) or leaf (final virtual or non-virtual). The parameters to the
 * operation may be displayed and if they are they may be conditionally
 * wrapped to reduce horizontial size of the icon.
 *
 * @param structclass  The pointer to the class being drawn
 * @param renderer  The pointer to the rendering object used to draw
 * @param elem      The pointer to the element within the class to be drawn
 * @param Yoffset   The Y offset from the start of the class at which to draw the operationbox
 * @return          The offset from the start of the class to the bottom of the operationbox
 *
 */
//static real
//structclass_draw_operationbox(STRUCTClass *structclass, DiaRenderer *renderer, Element *elem, real Yoffset)
//{
//  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
//  real     font_height;
//  Point    StartPoint;
//  Point    LowerRight;
//  DiaFont *font;
//  GList   *list;
//  Color   *fill_color = &structclass->fill_color;
//  Color   *line_color = &structclass->line_color;
//  Color   *text_color = &structclass->text_color;
//
//
//  StartPoint.x = elem->corner.x;
//  StartPoint.y = Yoffset;
//  Yoffset   += structclass->operationsbox_height;
//
//  LowerRight   = StartPoint;
//  LowerRight.x += elem->width;
//  LowerRight.y = Yoffset;
//
//  renderer_ops->fill_rect(renderer, &StartPoint, &LowerRight, fill_color);
//  renderer_ops->draw_rect(renderer, &StartPoint, &LowerRight, line_color);
//
//  if (!structclass->suppress_operations) {
//    gint i = 0;
//    GList *wrapsublist = NULL;
//    gchar *part_opstr = NULL;
//    int wrap_pos, last_wrap_pos, ident, wrapping_needed;
//    int part_opstr_len = 0, part_opstr_need = 0;
//
//    StartPoint.x += (structclass->line_width/2.0 + 0.1);
//    StartPoint.y += 0.1;
//
//    list = structclass->operations;
//    while (list != NULL) {
//      STRUCTOperation *op = (STRUCTOperation *)list->data;
//      gchar* opstr = struct_get_operation_string(op);
//      real ascent;
//
//      switch (op->inheritance_type) {
//      case STRUCT_ABSTRACT:
//        font = structclass->abstract_font;
//        font_height = structclass->abstract_font_height;
//        break;
//      case STRUCT_POLYMORPHIC:
//        font = structclass->polymorphic_font;
//        font_height = structclass->polymorphic_font_height;
//        break;
//      case STRUCT_LEAF:
//      default:
//        font = structclass->normal_font;
//        font_height = structclass->font_height;
//      }
//
//      wrapping_needed = 0;
//      if( structclass->wrap_operations ) {
//	wrapsublist = op->wrappos;
//      }
//
//      ascent = dia_font_ascent(opstr, font, font_height);
//      op->ascent = ascent;
//      renderer_ops->set_font(renderer, font, font_height);
//
//      if( structclass->wrap_operations && op->needs_wrapping) {
//	ident = op->wrap_indent;
//	wrapsublist = op->wrappos;
//        wrap_pos = last_wrap_pos = 0;
//
//        while( wrapsublist != NULL)   {
//          wrap_pos = GPOINTER_TO_INT( wrapsublist->data);
//
//          if( last_wrap_pos == 0)  {
//            part_opstr_need = wrap_pos + 1;
//            if (part_opstr_len < part_opstr_need) {
//              part_opstr_len = part_opstr_need;
//              part_opstr = g_realloc (part_opstr, part_opstr_need);
//            }
//            strncpy( part_opstr, opstr, wrap_pos);
//            memset( part_opstr+wrap_pos, '\0', 1);
//          }
//          else   {
//            part_opstr_need = ident + wrap_pos - last_wrap_pos + 1;
//            if (part_opstr_len < part_opstr_need) {
//              part_opstr_len = part_opstr_need;
//              part_opstr = g_realloc (part_opstr, part_opstr_need);
//            }
//            memset( part_opstr, ' ', ident);
//            memset( part_opstr+ident, '\0', 1);
//            strncat( part_opstr, opstr+last_wrap_pos, wrap_pos-last_wrap_pos);
//          }
//
//          if( last_wrap_pos == 0 ) {
//            StartPoint.y += ascent;
//          }
//          else
//          {
//            StartPoint.y += font_height;
//          }
//          renderer_ops->draw_string(renderer, part_opstr, &StartPoint, ALIGN_LEFT, text_color);
//	  if (op->class_scope) {
//	    struct_underline_text(renderer, StartPoint, font, font_height, part_opstr, line_color,
//			       structclass->line_width, STRUCTCLASS_UNDERLINEWIDTH );
//	  }
//          last_wrap_pos = wrap_pos;
//          wrapsublist = g_list_next( wrapsublist);
//        }
//      }
//      else
//      {
//        StartPoint.y += ascent;
//        renderer_ops->draw_string(renderer, opstr, &StartPoint, ALIGN_LEFT, text_color);
//	if (op->class_scope) {
//	  struct_underline_text(renderer, StartPoint, font, font_height, opstr, line_color,
//			     structclass->line_width, STRUCTCLASS_UNDERLINEWIDTH );
//	}
//      }
//
//
//      StartPoint.y += font_height - ascent;
//
//      if (structclass->visible_comments && op->comment != NULL && op->comment[0] != '\0'){
//        struct_draw_comments(renderer, structclass->comment_font ,structclass->comment_font_height,
//                               &structclass->text_color, op->comment, structclass->comment_tagging,
//                               structclass->comment_line_length, &StartPoint, ALIGN_LEFT);
//        StartPoint.y += structclass->comment_font_height/2;
//      }
//
//      list = g_list_next(list);
//      i++;
//      g_free (opstr);
//    }
//    if (part_opstr){
//      g_free(part_opstr);
//    }
//  }
//  return Yoffset;
//}

/**
 * Draw the template parameters box in the upper right hand corner of the
 * class box for paramertize classes (aka template classes). Fill in this
 * box with the parameters for the class.
 * <p>
 * At this time there is no provision for adding comments or documentation
 * to the display.
 *
 * @param structclass  The pointer to the class being drawn
 * @param renderer  The pointer to the rendering object used to draw
 * @param elem      The pointer to the element within the class to be drawn
 *
 */
//static void
//structclass_draw_template_parameters_box(STRUCTClass *structclass, DiaRenderer *renderer, Element *elem)
//{
//  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
//  Point UpperLeft;
//  Point LowerRight;
//  Point TextInsert;
//  GList *list;
//  gint   i;
//  DiaFont   *font = structclass->normal_font;
//  real       font_height = structclass->font_height;
//  real       ascent;
//  Color     *fill_color = &structclass->fill_color;
//  Color     *line_color = &structclass->line_color;
//  Color     *text_color = &structclass->text_color;
//
//
//  /*
//   * Adjust for the overlay of the template on the class icon
//   */
//  UpperLeft.x = elem->corner.x + elem->width - STRUCTCLASS_TEMPLATE_OVERLAY_X;
//  UpperLeft.y =  elem->corner.y - structclass->templates_height + STRUCTCLASS_TEMPLATE_OVERLAY_Y;
//  TextInsert = UpperLeft;
//  LowerRight = UpperLeft;
//  LowerRight.x += structclass->templates_width;
//  LowerRight.y += structclass->templates_height;
//
//  renderer_ops->fill_rect(renderer, &UpperLeft, &LowerRight, fill_color);
//  renderer_ops->set_linestyle(renderer, LINESTYLE_DASHED);
//  renderer_ops->set_dashlength(renderer, 0.3);
//  renderer_ops->draw_rect(renderer, &UpperLeft, &LowerRight, line_color);
//
//  TextInsert.x += 0.3;
//  TextInsert.y += 0.1;
//  renderer_ops->set_font(renderer, font, font_height);
//  i = 0;
//  list = structclass->formal_params;
//  while (list != NULL)
//  {
//    gchar *paramstr = struct_get_formalparameter_string((STRUCTFormalParameter *)list->data);
//
//    ascent = dia_font_ascent(paramstr, font, font_height);
//    TextInsert.y += ascent;
//    renderer_ops->draw_string(renderer, paramstr, &TextInsert, ALIGN_LEFT, text_color);
//    TextInsert.y += font_height - ascent;
//
//    list = g_list_next(list);
//    i++;
//    g_free (paramstr);
//  }
//}

/**
 * Draw the class icon for the specified STRUCTClass object.
 * Set the renderer to the correct fill and line styles and the appropriate
 * line width.  The object is drawn by the structclass_draw_namebox,
 * structclass_draw_attributebox, structclass_draw_operationbox and
 * structclass_draw_template_parameters_box.
 *
 * @param  structclass   object based on the struct class that is being rendered
 * @param   DiaRenderer  renderer used to draw the object
 *
 */

static void
structclass_draw(STRUCTClass *structclass, DiaRenderer *renderer)
{
    DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
    Element *elem;

    assert(structclass != NULL);
    assert(renderer != NULL);

    renderer_ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
    renderer_ops->set_linewidth(renderer, structclass->line_width);
    renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);

    elem = &structclass->element;

    structclass_draw_namebox(structclass, renderer, elem);
//  if (structclass->visible_attributes) {
//    y = structclass_draw_attributebox(structclass, renderer, elem, y);
//  }
//  if (structclass->visible_operations) {
//    y = structclass_draw_operationbox(structclass, renderer, elem, y);
//  }
//  if (structclass->template) {
//    structclass_draw_template_parameters_box(structclass, renderer, elem);
//  }
}

void
structclass_update_data(STRUCTClass *structclass)
{
    Element *elem = &structclass->element;
    DiaObject *obj = &elem->object;
    real x,y;
//  GList *list;
    int i;
    int pointswide;
    int lowerleftcorner;
    real pointspacing;

    x = elem->corner.x;
    y = elem->corner.y;

    /* Update connections: */
    structclass->connections[0].pos = elem->corner;
    structclass->connections[0].directions = DIR_NORTH|DIR_WEST;

    /* there are four corner points and two side points, thus all
     * remaining points are on the top/bottom width
     */
    pointswide = (STRUCTCLASS_CONNECTIONPOINTS - 6) / 2;
    pointspacing = elem->width / (pointswide + 1.0);

    /* across the top connection points */
    for (i=1; i<=pointswide; i++)
    {
        structclass->connections[i].pos.x = x + (pointspacing * i);
        structclass->connections[i].pos.y = y;
        structclass->connections[i].directions = DIR_NORTH;
    }

    i = (STRUCTCLASS_CONNECTIONPOINTS / 2) - 2;
    structclass->connections[i].pos.x = x + elem->width;
    structclass->connections[i].pos.y = y;
    structclass->connections[i].directions = DIR_NORTH|DIR_EAST;

    i = (STRUCTCLASS_CONNECTIONPOINTS / 2) - 1;
    structclass->connections[i].pos.x = x;
    structclass->connections[i].pos.y = y + structclass->namebox_height / 2.0;
    structclass->connections[i].directions = DIR_WEST;

    i = (STRUCTCLASS_CONNECTIONPOINTS / 2);
    structclass->connections[i].pos.x = x + elem->width;
    structclass->connections[i].pos.y = y + structclass->namebox_height / 2.0;
    structclass->connections[i].directions = DIR_EAST;

    i = (STRUCTCLASS_CONNECTIONPOINTS / 2) + 1;
    structclass->connections[i].pos.x = x;
    structclass->connections[i].pos.y = y + elem->height;
    structclass->connections[i].directions = DIR_WEST|DIR_SOUTH;

    /* across the bottom connection points */
    lowerleftcorner = (STRUCTCLASS_CONNECTIONPOINTS / 2) + 1;
    for (i=1; i<=pointswide; i++)
    {
        structclass->connections[lowerleftcorner + i].pos.x = x + (pointspacing * i);
        structclass->connections[lowerleftcorner + i].pos.y = y + elem->height;
        structclass->connections[lowerleftcorner + i].directions = DIR_SOUTH;
    }

    /* bottom-right corner */
    i = (STRUCTCLASS_CONNECTIONPOINTS) - 1;
    structclass->connections[i].pos.x = x + elem->width;
    structclass->connections[i].pos.y = y + elem->height;
    structclass->connections[i].directions = DIR_EAST|DIR_SOUTH;

#ifdef STRUCT_MAINPOINT
    /* Main point -- lives just after fixed connpoints in structclass array */
    i = STRUCTCLASS_CONNECTIONPOINTS;
    structclass->connections[i].pos.x = x + elem->width / 2;
    structclass->connections[i].pos.y = y + elem->height / 2;
    structclass->connections[i].directions = DIR_ALL;
    structclass->connections[i].flags = CP_FLAGS_MAIN;
#endif

    y += structclass->namebox_height + 0.1 + structclass->font_height/2;

//  list = (!structclass->visible_attributes || structclass->suppress_attributes) ? NULL : structclass->attributes;
//  while (list != NULL) {
//    STRUCTAttribute *attr = (STRUCTAttribute *)list->data;
//
//    attr->left_connection->pos.x = x;
//    attr->left_connection->pos.y = y;
//    attr->left_connection->directions = DIR_WEST;
//    attr->right_connection->pos.x = x + elem->width;
//    attr->right_connection->pos.y = y;
//    attr->right_connection->directions = DIR_EAST;
//
//    y += structclass->font_height;
//    if (structclass->visible_comments && attr->comment != NULL && attr->comment[0] != '\0') {
//      gint NumberOfLines = 0;
//      gchar *CommentString = 0;
//
//      CommentString =
//        struct_create_documentation_tag(attr->comment, structclass->comment_tagging, structclass->comment_line_length, &NumberOfLines);
//      g_free(CommentString);
//      y += structclass->comment_font_height*NumberOfLines + structclass->comment_font_height/2;
//    }
//
//    list = g_list_next(list);
//  }
//
//  y = elem->corner.y + structclass->namebox_height + 0.1 + structclass->font_height/2;
//  if (structclass->visible_attributes) {
//    y += structclass->attributesbox_height;
//  }
//
//  list = (!structclass->visible_operations || structclass->suppress_operations) ? NULL : structclass->operations;
//  while (list != NULL) {
//    STRUCTOperation *op = (STRUCTOperation *)list->data;
//
//    op->left_connection->pos.x = x;
//    op->left_connection->pos.y = y;
//    op->left_connection->directions = DIR_WEST;
//    op->right_connection->pos.x = x + elem->width;
//    op->right_connection->pos.y = y;
//    op->right_connection->directions = DIR_EAST;
//
//    if (op->needs_wrapping) { /* Wrapped */
//      int lines = g_list_length(op->wrappos);
//      y += structclass->font_height * lines;
//    } else {
//      y += structclass->font_height;
//    }
//    if (structclass->visible_comments && op->comment != NULL && op->comment[0] != '\0') {
//      gint NumberOfLines = 0;
//      gchar *CommentString = 0;
//
//      CommentString =
//        struct_create_documentation_tag(op->comment, structclass->comment_tagging, structclass->comment_line_length, &NumberOfLines);
//      g_free(CommentString);
//      y += structclass->comment_font_height*NumberOfLines + structclass->comment_font_height/2;
//    }
//    list = g_list_next(list);
//  }

    element_update_boundingbox(elem);

//  if (structclass->template) {
//    /* fix boundingstructclass for templates: */
//    obj->bounding_box.top -= (structclass->templates_height  - STRUCTCLASS_TEMPLATE_OVERLAY_Y) ;
//    obj->bounding_box.right += (structclass->templates_width - STRUCTCLASS_TEMPLATE_OVERLAY_X);
//    obj->bounding_box.left  -= (elem->width < STRUCTCLASS_TEMPLATE_OVERLAY_X) ?
//				(STRUCTCLASS_TEMPLATE_OVERLAY_X - elem->width) : 0;
//  }

    obj->position = elem->corner;

    element_update_handles(elem);

#ifdef DEBUG
//    structclass_sanity_check(structclass, "After updating data");
#endif
}

/**
 * Calculate the dimensions of the class icons namebox for a given object of STRUCTClass.
 * The height is stored in the class structure. When calculating the
 * comment, if any, the comment is word wrapped and the resulting number of
 * lines is then used to calculate the height of the bounding box.
 *
 * @param   structclass  pointer to the object of STRUCTClass to calculate
 * @return            the horizontal size of the name box.
 *
 */


//static real
//factory_calculate_name_data(STRUCTClass *structclass)
//{
//  real   maxwidth = 0.0;
//  real   width = 0.0;
//  /* name box: */
//
//  if (structclass->name != NULL && structclass->name[0] != '\0') {
//    if (structclass->abstract) {
//      maxwidth = dia_font_string_width(structclass->name,
//                                       structclass->abstract_classname_font,
//                                       structclass->abstract_classname_font_height);
//    } else {
//      maxwidth = dia_font_string_width(structclass->name,
//                                       structclass->classname_font,
//                                       structclass->classname_font_height);
//    }
//  }
//
//  structclass->namebox_height = structclass->classname_font_height + 4*0.1;
//  if (structclass->stereotype_string != NULL) {
//    g_free(structclass->stereotype_string);
//  }
//  if (structclass->stereotype != NULL && structclass->stereotype[0] != '\0') {
//    structclass->namebox_height += structclass->font_height;
//    structclass->stereotype_string = g_strconcat ( STRUCT_STEREOTYPE_START,
//			                                    structclass->stereotype,
//			                                    STRUCT_STEREOTYPE_END,
//			                                    NULL);
//
//    width = dia_font_string_width (structclass->stereotype_string,
//                                   structclass->normal_font,
//                                   structclass->font_height);
//    maxwidth = MAX(width, maxwidth);
//  } else {
//    structclass->stereotype_string = NULL;
//  }
//
//  if (structclass->visible_comments && structclass->comment != NULL && structclass->comment[0] != '\0')
//  {
//    int NumberOfLines = 0;
//    gchar *CommentString = struct_create_documentation_tag (structclass->comment,
//                                                         structclass->comment_tagging,
//                                                         structclass->comment_line_length,
//                                                         &NumberOfLines);
//    width = dia_font_string_width (CommentString,
//                                    structclass->comment_font,
//                                    structclass->comment_font_height);
//
//    g_free(CommentString);
//    structclass->namebox_height += structclass->comment_font_height * NumberOfLines;
//    maxwidth = MAX(width, maxwidth);
//  }
//  return maxwidth;
//}

static real
structclass_calculate_name_data(STRUCTClass *structclass)
{
    real   maxwidth = 0.0;
//  real   width = 0.0;
    /* name box: */

    if (structclass->name != NULL && structclass->name[0] != '\0')
    {
//    if (structclass->abstract) {
//      maxwidth = dia_font_string_width(structclass->name,
//                                       structclass->abstract_classname_font,
//                                       structclass->abstract_classname_font_height);
//    } else
        {
            maxwidth = dia_font_string_width(structclass->name,
                                             structclass->classname_font,
                                             structclass->classname_font_height);
        }
    }

    structclass->namebox_height = structclass->classname_font_height + 4*0.1;
//  if (structclass->stereotype_string != NULL) {
//    g_free(structclass->stereotype_string);
//  }
//  if (structclass->stereotype != NULL && structclass->stereotype[0] != '\0') {
//    structclass->namebox_height += structclass->font_height;
//    structclass->stereotype_string = g_strconcat ( STRUCT_STEREOTYPE_START,
//			                                    structclass->stereotype,
//			                                    STRUCT_STEREOTYPE_END,
//			                                    NULL);
//
//    width = dia_font_string_width (structclass->stereotype_string,
//                                   structclass->normal_font,
//                                   structclass->font_height);
//    maxwidth = MAX(width, maxwidth);
//  } else {
//    structclass->stereotype_string = NULL;
//  }

//  if (structclass->visible_comments && structclass->comment != NULL && structclass->comment[0] != '\0')
//  {
//    int NumberOfLines = 0;
//    gchar *CommentString = struct_create_documentation_tag (structclass->comment,
//                                                         structclass->comment_tagging,
//                                                         structclass->comment_line_length,
//                                                         &NumberOfLines);
//    width = dia_font_string_width (CommentString,
//                                    structclass->comment_font,
//                                    structclass->comment_font_height);
//
//    g_free(CommentString);
//    structclass->namebox_height += structclass->comment_font_height * NumberOfLines;
//    maxwidth = MAX(width, maxwidth);
//  }
    return maxwidth;
}

/**
 * Calculate the dimensions of the attribute box on an object of type STRUCTClass.
 * @param   structclass  a pointer to an object of STRUCTClass
 * @return            the horizontal size of the attribute box
 *
 */

//static real
//structclass_calculate_attribute_data(STRUCTClass *structclass)
//{
//  int    i;
//  real   maxwidth = 0.0;
//  real   width    = 0.0;
//  GList *list;
//
//  structclass->attributesbox_height = 2*0.1;
//
//  if (g_list_length(structclass->attributes) != 0)
//  {
//    i = 0;
//    list = structclass->attributes;
//    while (list != NULL)
//    {
//      STRUCTAttribute *attr   = (STRUCTAttribute *) list->data;
//      gchar        *attstr = struct_get_attribute_string(attr);
//
//      if (attr->abstract)
//      {
//        width = dia_font_string_width(attstr,
//                                      structclass->abstract_font,
//                                      structclass->abstract_font_height);
//        structclass->attributesbox_height += structclass->abstract_font_height;
//      }
//      else
//      {
//        width = dia_font_string_width(attstr,
//                                      structclass->normal_font,
//                                      structclass->font_height);
//        structclass->attributesbox_height += structclass->font_height;
//      }
//      maxwidth = MAX(width, maxwidth);
//
//      if (structclass->visible_comments && attr->comment != NULL && attr->comment[0] != '\0')
//      {
//        int NumberOfLines = 0;
//        gchar *CommentString = struct_create_documentation_tag(attr->comment,
//                                                            structclass->comment_tagging,
//                                                            structclass->comment_line_length,
//                                                            &NumberOfLines);
//        width = dia_font_string_width(CommentString,
//                                       structclass->comment_font,
//                                       structclass->comment_font_height);
//        g_free(CommentString);
//        structclass->attributesbox_height += structclass->comment_font_height * NumberOfLines + structclass->comment_font_height/2;
//        maxwidth = MAX(width, maxwidth);
//      }
//
//      i++;
//      list = g_list_next(list);
//      g_free (attstr);
//    }
//  }
//
//  if ((structclass->attributesbox_height<0.4)|| structclass->suppress_attributes )
//  {
//    structclass->attributesbox_height = 0.4;
//  }
//  return maxwidth;
//}

/**
 * Calculate the dimensions of the operations box of an object of  STRUCTClass.
 * The vertical size or height is stored in the object.
 * @param   structclass  a pointer to an object of STRUCTClass
 * @return         the horizontial size of the operations box
 *
 */

//static real
//structclass_calculate_operation_data(STRUCTClass *structclass)
//{
//  int    i;
//  int    pos_next_comma;
//  int    pos_brace;
//  int    wrap_pos;
//  int    last_wrap_pos;
//  int    indent;
//  int    offset;
//  int    maxlinewidth;
//  int    length;
//  real   maxwidth = 0.0;
//  real   width    = 0.0;
//  GList *list;
//  GList *wrapsublist;
//
//  /* operations box: */
//  structclass->operationsbox_height = 2*0.1;
//
//  if (0 != g_list_length(structclass->operations))
//  {
//    i = 0;
//    list = structclass->operations;
//    while (list != NULL)
//    {
//      STRUCTOperation *op = (STRUCTOperation *) list->data;
//      gchar *opstr = struct_get_operation_string(op);
//      DiaFont   *Font;
//      real       FontHeight;
//
//      length = strlen( (const gchar*)opstr);
//
//      if (op->wrappos != NULL) {
//	g_list_free(op->wrappos);
//      }
//      op->wrappos = NULL;
//
//      switch(op->inheritance_type)
//      {
//	  case STRUCT_ABSTRACT:
//	    Font       =  structclass->abstract_font;
//	    FontHeight =  structclass->abstract_font_height;
//	    break;
//	  case STRUCT_POLYMORPHIC:
//	    Font       =  structclass->polymorphic_font;
//	    FontHeight =  structclass->polymorphic_font_height;
//	    break;
//	  case STRUCT_LEAF:
//	  default:
//	    Font       = structclass->normal_font;
//	    FontHeight = structclass->font_height;
//      }
//      op->ascent = dia_font_ascent(opstr, Font, FontHeight);
//
//      if( structclass->wrap_operations )
//      {
//        if( length > structclass->wrap_after_char)
//        {
//          gchar *part_opstr;
//	  op->needs_wrapping = TRUE;
//
//          /* count maximal line width to create a secure buffer (part_opstr)
//          and build the sublist with the wrapping data for the current operation, which will be used by structclass_draw(), too.
//	  */
//          pos_next_comma = pos_brace = wrap_pos = offset
//	    = maxlinewidth = structclass->max_wrapped_line_width = 0;
//          while( wrap_pos + offset < length)
//          {
//            do
//            {
//              pos_next_comma = strcspn( (const gchar*)opstr + wrap_pos + offset, ",");
//              wrap_pos += pos_next_comma + 1;
//            } while( wrap_pos < structclass->wrap_after_char - pos_brace
//		     && wrap_pos + offset < length);
//
//            if( offset == 0){
//              pos_brace = strcspn( opstr, "(");
//	      op->wrap_indent = pos_brace + 1;
//            }
//	    op->wrappos = g_list_append(op->wrappos,
//					GINT_TO_POINTER(wrap_pos + offset));
//
//            maxlinewidth = MAX(maxlinewidth, wrap_pos);
//
//            offset += wrap_pos;
//            wrap_pos = 0;
//          }
//          structclass->max_wrapped_line_width = MAX( structclass->max_wrapped_line_width, maxlinewidth+1);
//
//	  indent = op->wrap_indent;
//          part_opstr = g_alloca(structclass->max_wrapped_line_width+indent+1);
//
//	  wrapsublist = op->wrappos;
//          wrap_pos = last_wrap_pos = 0;
//
//          while( wrapsublist != NULL){
//            wrap_pos = GPOINTER_TO_INT( wrapsublist->data);
//            if( last_wrap_pos == 0){
//              strncpy( part_opstr, opstr, wrap_pos);
//              memset( part_opstr+wrap_pos, '\0', 1);
//            }
//            else
//            {
//              memset( part_opstr, ' ', indent);
//              memset( part_opstr+indent, '\0', 1);
//              strncat( part_opstr, opstr+last_wrap_pos, wrap_pos-last_wrap_pos);
//            }
//
//            width = dia_font_string_width(part_opstr,Font,FontHeight);
//            structclass->operationsbox_height += FontHeight;
//
//            maxwidth = MAX(width, maxwidth);
//            last_wrap_pos = wrap_pos;
//            wrapsublist = g_list_next( wrapsublist);
//          }
//        }
//        else
//        {
//	  op->needs_wrapping = FALSE;
//        }
//      }
//
//      if (!(structclass->wrap_operations && length > structclass->wrap_after_char)) {
//        switch(op->inheritance_type)
//        {
//        case STRUCT_ABSTRACT:
//          Font       =  structclass->abstract_font;
//          FontHeight =  structclass->abstract_font_height;
//          break;
//        case STRUCT_POLYMORPHIC:
//          Font       =  structclass->polymorphic_font;
//          FontHeight =  structclass->polymorphic_font_height;
//          break;
//        case STRUCT_LEAF:
//        default:
//          Font       = structclass->normal_font;
//          FontHeight = structclass->font_height;
//        }
//        width = dia_font_string_width(opstr,Font,FontHeight);
//        structclass->operationsbox_height += FontHeight;
//
//        maxwidth = MAX(width, maxwidth);
//      }
//
//      if (structclass->visible_comments && op->comment != NULL && op->comment[0] != '\0'){
//        int NumberOfLines = 0;
//        gchar *CommentString = struct_create_documentation_tag(op->comment,
//                                                            structclass->comment_tagging,
//                                                            structclass->comment_line_length,
//                                                            &NumberOfLines);
//        width = dia_font_string_width(CommentString,
//                                       structclass->comment_font,
//                                       structclass->comment_font_height);
//        g_free(CommentString);
//        structclass->operationsbox_height += structclass->comment_font_height * NumberOfLines + structclass->comment_font_height/2;
//        maxwidth = MAX(width, maxwidth);
//      }
//
//      i++;
//      list = g_list_next(list);
//      g_free (opstr);
//    }
//  }
//
//  structclass->element.width = maxwidth + 2*0.3;
//
//  if ((structclass->operationsbox_height<0.4) || structclass->suppress_operations ) {
//    structclass->operationsbox_height = 0.4;
//  }
//
//  return maxwidth;
//}

/**
 * calculate the size of the class icon for an object of STRUCTClass.
 * This is done by calculating the size of the text to be displayed within
 * each of the contained bounding boxes, name, attributes and operations.
 * Because the comments may require wrapping, each comment is wrapped and
 * the resulting number of lines is used to calculate the size of the
 * comment within the box. The various font settings with in the class
 * properties contribute to the overall size of the resulting bounding box.
 *
 *  * @param   structclass  a pointer to an object of STRUCTClass
 *
 */
void
structclass_calculate_data(STRUCTClass *structclass)
{
//  int    i;
    int    num_templates;
    real   maxwidth = 0.0;
//  real   width;
//  GList *list;

    if (!structclass->destroyed)
    {
        maxwidth = MAX(structclass_calculate_name_data(structclass),      maxwidth);

        structclass->element.height = structclass->namebox_height;

//    if (structclass->visible_attributes){
//      maxwidth = MAX(structclass_calculate_attribute_data(structclass), maxwidth);
//      structclass->element.height += structclass->attributesbox_height;
//    }
//    if (structclass->visible_operations){
//      maxwidth = MAX(structclass_calculate_operation_data(structclass), maxwidth);
//      structclass->element.height += structclass->operationsbox_height;
//    }
        structclass->element.width  = maxwidth+0.5;
        /* templates box: */
//    num_templates = g_list_length(structclass->formal_params);

//    structclass->templates_height =
//      structclass->font_height * num_templates + 2*0.1;
//    structclass->templates_height = MAX(structclass->templates_height, 0.4);


        maxwidth = STRUCTCLASS_TEMPLATE_OVERLAY_X;
        if (num_templates != 0)
        {
//      i = 0;
//      list = structclass->formal_params;
//      while (list != NULL)
//      {
//        STRUCTFormalParameter *param = (STRUCTFormalParameter *) list->data;
//	gchar *paramstr = struct_get_formalparameter_string(param);
//
//        width = dia_font_string_width(paramstr,
//                                      structclass->normal_font,
//                                      structclass->font_height);
//        maxwidth = MAX(width, maxwidth);
//
//        i++;
//        list = g_list_next(list);
//	g_free (paramstr);
//      }
        }
//    structclass->templates_width = maxwidth + 2*0.2;
    }
}


//static void
//factory_calculate_data(STRUCTClass *structclass)
//{
//      int    i;
//  int    num_templates;
//  real   maxwidth = 0.0;
//  real   width;
//  GList *list;
//
//  if (!structclass->destroyed)
//  {
//    maxwidth = MAX(factory_calculate_name_data(structclass),maxwidth);
//
//    structclass->element.height = structclass->namebox_height;

//    if (structclass->visible_attributes){
//      maxwidth = MAX(structclass_calculate_attribute_data(structclass), maxwidth);
//      structclass->element.height += structclass->attributesbox_height;
//    }
//    if (structclass->visible_operations){
//      maxwidth = MAX(structclass_calculate_operation_data(structclass), maxwidth);
//      structclass->element.height += structclass->operationsbox_height;
//    }
//    structclass->element.width  = maxwidth+0.5;
/* templates box: */
//    num_templates = g_list_length(structclass->formal_params);

//    structclass->templates_height =
//      structclass->font_height * num_templates + 2*0.1;
//    structclass->templates_height = MAX(structclass->templates_height, 0.4);
//
//
//    maxwidth = STRUCTCLASS_TEMPLATE_OVERLAY_X;
//    if (num_templates != 0)
//    {
//      i = 0;
//      list = structclass->formal_params;
//      while (list != NULL)
//      {
//        STRUCTFormalParameter *param = (STRUCTFormalParameter *) list->data;
//	gchar *paramstr = struct_get_formalparameter_string(param);
//
//        width = dia_font_string_width(paramstr,
//                                      structclass->normal_font,
//                                      structclass->font_height);
//        maxwidth = MAX(width, maxwidth);
//
//        i++;
//        list = g_list_next(list);
//	g_free (paramstr);
//      }
//    }
//    structclass->templates_width = maxwidth + 2*0.2;
//  }
//}

static void
fill_in_fontdata(STRUCTClass *structclass)
{
    if (structclass->normal_font == NULL)
    {
        structclass->font_height = 0.8;
        structclass->normal_font = dia_font_new_from_style(DIA_FONT_MONOSPACE, 0.8);
    }
//   if (structclass->abstract_font == NULL) {
//     structclass->abstract_font_height = 0.8;
//     structclass->abstract_font =
//       dia_font_new_from_style(DIA_FONT_MONOSPACE | DIA_FONT_ITALIC | DIA_FONT_BOLD, 0.8);
//   }
//   if (structclass->polymorphic_font == NULL) {
//     structclass->polymorphic_font_height = 0.8;
//     structclass->polymorphic_font =
//       dia_font_new_from_style(DIA_FONT_MONOSPACE | DIA_FONT_ITALIC, 0.8);
//   }
    if (structclass->classname_font == NULL)
    {
//        factory_debug_to_log(factory_utf8("fill_in_fontdata,structclass->classname_font\n"));
        structclass->classname_font_height = 1.0;
        structclass->classname_font =
            dia_font_new_from_style(DIA_FONT_MONOSPACE | DIA_FONT_BOLD, 1.0);
    }
//   if (structclass->abstract_classname_font == NULL) {
//     structclass->abstract_classname_font_height = 1.0;
//     structclass->abstract_classname_font =
//       dia_font_new_from_style(DIA_FONT_SANS | DIA_FONT_BOLD | DIA_FONT_ITALIC, 1.0);
//   }
//   if (structclass->comment_font == NULL) {
//     structclass->comment_font_height = 0.7;
//     structclass->comment_font = dia_font_new_from_style(DIA_FONT_SANS | DIA_FONT_ITALIC, 0.7);
//   }
}
/**
 * Create an object of type class
 * By default this will create a object of class STRUCTClass. Howerver there
 * are at least two types of STRUCTClass objects, so the user_data is selects
 * the correct STRUCTClass object. Other than that this is quite straight
 * forward. The key to the polymorphic nature of this object is the use of
 * the DiaObjectType record which in conjunction with the user_data
 * controls the specific derived object type.
 *
 * @param  startpoint   the origin of the object being created
 * @param  user_data	Information used by this routine to create the appropriate object
 * @param  handle1		ignored when creating a class object
 * @param  handle2      ignored when creating a class object
 * @return               a pointer to the object created
 *
 *  NOTE:
 *      This function should most likely be move to a source file for
 *      handling global STRUCT functionallity at some point.
 */

//static void factory_set_name(gpointer key,gpointer value,gpointer user_data)
//{
//    STRUCTClass *structclass = (STRUCTClass *)user_data;
//    structclass->name = (gchar*)key;
//}


//static DiaObject *  // 2014-3-19 lcy 这里初始化结构
//structclass_create(Point *startpoint,
//	       void *user_data,
//  	       Handle **handle1,
//	       Handle **handle2)
//{
//  STRUCTClass *structclass;
//  STRUCTClassDialog *properties_dialog;
//  Element *elem;
//  DiaObject *obj;
//  int i;
//
//  structclass = g_malloc0(sizeof(STRUCTClass));
//  elem = &structclass->element;
//  obj = &elem->object;
//
//
//  elem->corner = *startpoint;
//
//#ifdef STRUCT_MAINPOINT
//  element_init(elem, 8, STRUCTCLASS_CONNECTIONPOINTS + 1); /* No attribs or ops => 0 extra connectionpoints. */
//#else
//  element_init(elem, 8, STRUCTCLASS_CONNECTIONPOINTS); /* No attribs or ops => 0 extra connectionpoints. */
//#endif
//
//    fill_in_fontdata(structclass);
//
//  /*
//   * The following block of code may need to be converted to a switch statement if more than
//   * two types of objects can be made - Dave Klotzbach
//   */
//  // structclass->template = (GPOINTER_TO_INT(user_data)==1);
////  structclass->template = FALSE;
//  int index = GPOINTER_TO_INT(user_data);
//  GList *sstruct = structList.structList;
//  for(;sstruct !=NULL;sstruct = sstruct->next)
//  {
//      FactoryStructItemList *i = sstruct->data;
//      if(i->number == index)
//      {
//          structclass->name = g_strdup(_(i->name));
//          break;
//      }
//  }
//
//  /* 2014-3-26 lcy  这里初始哈希表用存widget与它的值*/
// // structclass->widgetmap = g_hash_table_new(g_direct_hash,g_direct_equal);
//  structclass->widgetmap = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
// // structclass->widgetmap = NULL;
//
//  obj->type = &structclass_type;
//  obj->ops = &structclass_ops;
//
////  structclass->stereotype = NULL;
////  structclass->comment = NULL;
////
////  structclass->abstract = FALSE;
////
////  structclass->suppress_attributes = FALSE;
////  structclass->suppress_operations = FALSE;
////
////  structclass->visible_attributes = TRUE;
////  structclass->visible_operations = TRUE;
////  structclass->visible_comments = FALSE;
////
////  structclass->wrap_operations = TRUE;
////  structclass->wrap_after_char = STRUCTCLASS_WRAP_AFTER_CHAR;
////
////  structclass->attributes = NULL;
////
////  structclass->operations = NULL;
////
////  structclass->formal_params = NULL;
////
////  structclass->stereotype_string = NULL;
//
//  structclass->line_width = attributes_get_default_linewidth();
//  structclass->text_color = color_black;
//  structclass->line_color = attributes_get_foreground();
//  structclass->fill_color = attributes_get_background();
//
//  structclass_calculate_data(structclass);
//
//  for (i=0;i<STRUCTCLASS_CONNECTIONPOINTS;i++) {
//    obj->connections[i] = &structclass->connections[i];
//    structclass->connections[i].object = obj;
//    structclass->connections[i].connected = NULL;
//  }
//#ifdef STRUCT_MAINPOINT
//  /* Put mainpoint at the end, after conditional attr/oprn points,
//   * but store it in the local connectionpoint array. */
//  i += structclass_num_dynamic_connectionpoints(structclass);
//  obj->connections[i] = &structclass->connections[STRUCTCLASS_CONNECTIONPOINTS];
//  structclass->connections[STRUCTCLASS_CONNECTIONPOINTS].object = obj;
//  structclass->connections[STRUCTCLASS_CONNECTIONPOINTS].connected = NULL;
//#endif
//
//  elem->extra_spacing.border_trans = structclass->line_width/2.0;
//  structclass_update_data(structclass);
//
//  for (i=0;i<8;i++) {
//    obj->handles[i]->type = HANDLE_NON_MOVABLE;
//  }
//
//  *handle1 = NULL;
//  *handle2 = NULL;
//  structclass->EnumsAndStructs = NULL;
//  structclass->EnumsAndStructs = &structList;
//  factory_read_initial_to_struct(structclass);
//  return &structclass->element.object;
//}

gint factory_gint_compare(gpointer first, gpointer second)
{
    if( GPOINTER_TO_INT(first) == GPOINTER_TO_INT(second) )
        return 0;
    else if(GPOINTER_TO_INT(first) < GPOINTER_TO_INT(second))
        return -1;
    else
        return 1;
}
void factory_get_oindex_for_structclass(STRUCTClass *structclass)
{
    GList *p = g_hash_table_get_values(curLayer->defnames);
    if(!p)
    {
        structclass->element.object.oindex = 0;
        return;
    }

    GList *numlist = NULL;

    for(; p ; p = p->next)
    {
        DiaObject *eobj = (DiaObject *)p->data;
        numlist = g_list_append(numlist,(gpointer)eobj->oindex);
    }

    if(g_list_length(numlist) > 1)
    {
        numlist = g_list_sort(numlist,factory_gint_compare);
    }

    int n = 0;
    while(numlist)
    {
        if(n < GPOINTER_TO_INT(numlist->data))
        {
            break;
        }
        n++;
        numlist = numlist->next;
    }
    structclass->element.object.oindex = n;
}

void factory_rename_structclass(STRUCTClass *structclass)
{
    g_return_if_fail(structclass);
    DiaObject *obj = &structclass->element.object;
    if( !curLayer ||  factory_is_special_object(obj->name)
            || factory_is_system_data(obj->name))
        return ;
    if(!structclass->hasIdnumber)
    {
        structclass->hasIdnumber = TRUE;
        factory_get_oindex_for_structclass(structclass);
    }


    STRUCTClass *oldclass =  g_hash_table_lookup(curLayer->defnames,structclass->name);
    if(!oldclass)
    {
        g_hash_table_insert(curLayer->defnames,structclass->name,structclass);

        return ;
    }


    int n = 0;
    gchar *key = NULL;
    for(; n < 2048; n++)
    {
        gchar **split = g_strsplit(structclass->name,"(",-1);
        key = g_strconcat(split[0],g_strdup_printf("(%d)",n),NULL);
        g_strfreev(split);
        gpointer ptr = g_hash_table_lookup(curLayer->defnames,key);

        if(!ptr)
            break;
        g_free(key);
    }
    if(n >= 2048)
    {
        gchar *msg = factory_utf8("单个控件编号超出2048个限制了");
        factory_message_dialoag(ddisplay_active()->shell,msg);
        factory_waring_to_log(msg);
        g_free(msg);
    }

    g_free(structclass->name);
    structclass->name = g_strdup(key);
    g_hash_table_insert(curLayer->defnames,
                        structclass->name,structclass);
    g_free(key);

}


static DiaObject *  // 2014-3-19 lcy 这里初始化结构
factory_struct_items_create(Point *startpoint,
                            void *user_data,
                            Handle **handle1,
                            Handle **handle2)
{

    if(curLayer != factoryContainer->curLayer)
        curLayer = factoryContainer->curLayer;

//    if(!factoryContainer->fgdn_func)
//        factoryContainer->fgdn_func =(FactoryGetDownloadNameList)factory_get_download_name_list;
    STRUCTClass *structclass;
//  STRUCTClassDialog *properties_dialog;
    Element *elem;
    DiaObject *obj;
    int i;

    structclass = g_malloc0(sizeof(STRUCTClass));
    structclass->isInitial = FALSE;
    structclass->hasIdnumber = FALSE;

    elem = &structclass->element;
    obj = &elem->object;
    obj->name = g_strdup("");

    elem->corner = *startpoint;

#ifdef STRUCT_MAINPOINT
    element_init(elem, 8, STRUCTCLASS_CONNECTIONPOINTS + 1); /* No attribs or ops => 0 extra connectionpoints. */
#else
    element_init(elem, 8, STRUCTCLASS_CONNECTIONPOINTS); /* No attribs or ops => 0 extra connectionpoints. */
#endif
    fill_in_fontdata(structclass);

    /*
     * The following block of code may need to be converted to a switch statement if more than
     * two types of objects can be made - Dave Klotzbach
     */
    // structclass->template = (GPOINTER_TO_INT(user_data)==1);
//  structclass->template = FALSE;
    int index = GPOINTER_TO_INT(user_data);
    GList *sstruct = factoryContainer->structList;


    for(; sstruct ; sstruct = sstruct->next)
    {
        FactoryStructItemList *i = (FactoryStructItemList *)sstruct->data;
        if(i->number == index)
        {
            structclass->name = g_strdup(i->vname) ;
            obj->name = g_strdup(_(i->sname));
            break;
        }
    }

//    if(factory_is_system_data(obj->name))
//    {
//        if(factoryContainer->otp_obj)
//            return NULL;
//        else
//            factoryContainer->otp_obj = structclass;
//    }

    factory_rename_structclass(structclass); /* 保证名称唯一性 */


    /* 2014-3-26 lcy  这里初始哈希表用存widget与它的值*/
// structclass->widgetmap = g_hash_table_new(g_direct_hash,g_direct_equal);
//    structclass->widgetmap = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);

    obj->type = &structclass_type;
    //  obj->type->version  = g_strdup(factoryContainer->file_version);
    obj->type->version = g_strdup_printf("%s@%s.%s",factoryContainer->project_number,factoryContainer->major_version,
                                         factoryContainer->minor_version);
    obj->ops = &structclass_ops;


    structclass->line_width = attributes_get_default_linewidth();
    structclass->text_color = color_black;
    structclass->line_color = attributes_get_foreground();
    structclass->fill_color = attributes_get_background();
    structclass->vcolor = N_COLOR;


    structclass_calculate_data(structclass);

    for (i=0; i<STRUCTCLASS_CONNECTIONPOINTS; i++)
    {
        obj->connections[i] = &structclass->connections[i];
        structclass->connections[i].object = obj;
        structclass->connections[i].connected = NULL;
    }
#ifdef STRUCT_MAINPOINT
    /* Put mainpoint at the end, after conditional attr/oprn points,
     * but store it in the local connectionpoint array. */
    i += structclass_num_dynamic_connectionpoints(structclass);
    obj->connections[i] = &structclass->connections[STRUCTCLASS_CONNECTIONPOINTS];
    structclass->connections[STRUCTCLASS_CONNECTIONPOINTS].object = obj;
    structclass->connections[STRUCTCLASS_CONNECTIONPOINTS].connected = NULL;
#endif

    elem->extra_spacing.border_trans = structclass->line_width/2.0;
//    factory_debug_to_log(factory_utf8("创建对像,structclass_update_data(structclass)"));
    structclass_update_data(structclass);

    for (i=0; i<8; i++)
    {
        obj->handles[i]->type = HANDLE_NON_MOVABLE;
    }

    *handle1 = NULL;
    *handle2 = NULL;
    structclass->EnumsAndStructs = NULL;
    structclass->EnumsAndStructs = factoryContainer;
    if(index >= factoryContainer->act_num)
    {
        /* 这里是从模版区间拖入的 */
        obj->isTemplate = TRUE;
        structclass->isInitial = TRUE;
        structclass->widgetSave =
            factory_template_get_widgetsave(index,structclass);
        structclass->EnumsAndStructs = factoryContainer;
        factory_set_original_class(structclass);
        factory_set_structsave_class(structclass);
    }
    factory_debug_to_log(g_strdup_printf(factory_utf8("创建对像,名字:%s.\n"),structclass->name));

    return &structclass->element.object;
}


FactoryStructItem *factory_get_factorystructitem(GList *inlist,const gchar *name)
{
    FactoryStructItem *fst = NULL;
    GList *p =inlist;
    GQuark qaurk = g_quark_from_string(name);

    for(; p; p=p->next)
    {
        FactoryStructItem *t = p->data;
        GQuark ok = g_quark_from_string(t->Name);
        if(ok == qaurk)
        {
            fst = p->data;
            break;
        }
    }
    return fst;
}

void factory_read_union_button_from_file(STRUCTClass *fclass,
        ObjectNode obtn_node,
        SaveUbtn *sbtn)
{
    xmlChar *key = NULL;
    while(obtn_node = data_next(obtn_node))
    {
        key = xmlGetProp(obtn_node,(xmlChar *)"name");
        if(!key) continue;
        gchar *skey = factory_get_last_section((gchar*)key,".");
        FactoryStructItem *sitem = factory_get_factorystructitem(sbtn->structlist,skey);
        if(!sitem)  continue;
        sitem->orgclass = fclass;

        /* 这里不确定,要注与保存的一致*/
        SaveStruct *nnode = factory_get_savestruct(sitem);
        if(!nnode) return ;

        nnode->widget1 = NULL;
        nnode->widget2 = NULL;
        nnode->org = sitem;
        nnode->sclass = sitem->orgclass;
        nnode->name = g_strdup((gchar*)key);

        factory_read_object_value_from_file(nnode,sitem,obtn_node);
        sbtn->savelist = g_list_append(sbtn->savelist,nnode); /* 这里有可能只有一节点 */
        xmlFree(key);
    }

}

static void factory_read_type_index_item(SaveStruct *sss,ObjectNode attr_node)
{

    SaveMusicDialog *smd = curLayer->smd;
    SaveSel *ssel = NULL;
    if(!sss->value.vnumber)
    {
        ssel = g_new0(SaveSel,1);
        ssel->ntable = NULL;
        ssel->offset_val = 0;
        sss->value.vnumber = ssel;
    }
    else
        ssel = sss->value.vnumber;

    xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"value");
    gint val = -1;
    if(key)
    {
        val = g_strtod(key,NULL);
        xmlFree(key);
    }

    key = xmlGetProp(attr_node,(xmlChar*)"idname");
    if(key && smd->midlists)
    {
        GQuark nquark = g_quark_from_string((gchar*)key);
        subTable *stable = factory_idlist_find_subtable(smd->midlists,nquark);
        if(stable)
        {
            ssel->ntable = &stable->nquark;
            switch(factory_music_fm_get_position_type(sss->name))
            {
            case OFFSET_FST:
                ssel->offset_val = 0;
                break;
            case OFFSET_SEL:
            {
                GList *tlist = smd->midlists;
                for(; tlist; tlist = tlist->next)
                {
                    subTable *st = tlist->data;
                    gint len = g_list_length(st->sub_list);
                    if(val >= len )
                        val -= len;
                    else
                        break;
                }
                ssel->offset_val = val;
            }
            break;
            case OFFSET_END:
                ssel->offset_val = g_list_length(stable->sub_list)-1;
                break;
            default:
                break;
            }

        }
        xmlFree(key);
    }
}


static void factory_read_type_idlist_item(SaveStruct *sss,ObjectNode attr_node)
{
    SaveSel *ssel = NULL;
    if(!sss->value.vnumber)
    {
        ssel = g_new0(SaveSel,1);
        ssel->ntable = NULL;
        ssel->offset_val = 0;
        sss->value.vnumber = ssel;
    }
    else
        ssel = sss->value.vnumber;
    SaveIdDialog *sid = curLayer->sid;
    xmlChar *key = xmlGetProp(attr_node,(xmlChar*)"idname");
    if(key && sid->idlists )
    {
        GQuark nquark  = g_quark_from_string((gchar*)key);
        subTable *stable =
            factory_idlist_find_subtable(sid->idlists,nquark);
        if(stable)
        {
            ssel->ntable = &stable->nquark;
        }
    }
}

void  factory_read_object_value_from_file(SaveStruct *sss,FactoryStructItem *fst,ObjectNode attr_node)
{

    STRUCTClass *fclass = fst->orgclass;
    xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"wtype");
    if( factory_is_special_object(fst->FType) &&
            g_ascii_strncasecmp((gchar*)key,"LBTN",4) )
    {
        /* 这里是id列表与音乐文件列表的数据读取 */
        sss->celltype = UBTN;
        key = xmlGetProp(attr_node,(xmlChar*)"name");
        if(!key)
        {
            gchar *msg = g_strdup_printf(factory_utf8("节点不存在!\n"
                                         "对像名:%s\n成员名:%s\n行数:%d\n"),
                                         fclass->name,sss->name,attr_node->line);
            factory_critical_error_exit(msg);
        }
        gchar *last_sec = factory_get_last_section((gchar*)key,".");
        if(g_ascii_strcasecmp(fst->Name,last_sec))
        {
            gchar *msg = g_strdup_printf(factory_utf8("工程文件的名称与源文件名不符!\n"
                                         "对像名:%s\n成员名:%s\n读取名:%s\n行数:%d\n"),
                                         fclass->name,sss->name,last_sec,attr_node->line);
            factory_critical_error_exit(msg);
        }
        xmlFree(key);

        if(factory_music_fm_item_is_index(last_sec))
        {
            if(factory_music_fm_get_position_type(last_sec) == -1)
            {
                gchar *msg = g_strdup_printf(factory_utf8("无法识别关键字名:%!\n"
                                             "对像名:%s\n成员名:%s\n行数:%d\n"),
                                             last_sec,fclass->name,sss->name,attr_node->line);
                factory_critical_error_exit(msg);

            }
            factory_read_type_index_item(sss,attr_node); /* file index */
        }
        else /* PHY and SEQUENCE IDLST*/
        {
            if(!g_strcasecmp(fst->SType,TYPE_IDLST))
            {
                factory_read_type_idlist_item(sss,attr_node);
            }
            else
            {
                key = xmlGetProp(attr_node,(xmlChar*)"value");
                if(!key)
                {
                    sss->value.vnumber = g_strdup("-1");
                }
                else
                    sss->value.vnumber = g_strdup((gchar*)key);
                xmlFree(key);

            }

        }
        return;
    }

    if(!key) return ;

    if(!g_ascii_strncasecmp((gchar*)key,"ECOMBO",6))
    {
        sss->celltype = ECOMBO;
        /* 2014-3-26 lcy 通过名字去哈希表里找链表*/
        gchar *skey = factory_get_last_section(sss->type,".");

        GList *targettable = g_hash_table_lookup(fclass->EnumsAndStructs->enumTable,skey);
        g_free(skey);
        SaveEnum *sen = &sss->value.senum;
        if(targettable)
        {
            sen->enumList = targettable;
            GList *t  = targettable;
            key = xmlGetProp(attr_node,(xmlChar *)"value");
            GQuark kquark = g_quark_from_string((gchar*)key);
            if((gchar*)key)
                for(; t != NULL ; t = t->next)
                {
                    FactoryStructEnum *fse = t->data;
                    GQuark quark = g_quark_from_string(fse->value);
                    if(quark == kquark)
                    {
                        sen->evalue = factory_utf8((gchar*)key);
                        sen->index = g_list_index(targettable,fse);
                        break;
                    }
                }


            key = xmlGetProp(attr_node,(xmlChar *)"width");
            if(key)
                sen->width =  factory_utf8((gchar*)key);
        }

    }
    else if(0== g_ascii_strncasecmp((gchar*)key,"ENTRY",5))
    {
        key = xmlGetProp(attr_node,(xmlChar *)"value");
        xmlFree(key);
        sss->celltype = ENTRY;
    }
    else if(!g_ascii_strncasecmp((gchar*)key,"OCOMBO",6))
    {
        sss->celltype = OCOMBO;
        ActionID *acid = &sss->value.actid;
        acid->line = NULL;
        factory_read_object_comobox_value_from_file(attr_node,acid);
    }
    else if(!g_ascii_strncasecmp((gchar*)key,"OBTN",4))
    {
        sss->celltype = OBTN;
        sss->isPointer = TRUE;
        sss->newdlg_func = factory_create_objectbutton_dialog;
        sss->close_func = factory_save_objectbutton_dialog;
        ActIDArr *nid = &sss->value.nextid;
        /* 读它下面的子节点 */
        g_list_free1(nid->actlist); /* 这里是清理掉默认值 */
        nid->actlist = NULL;
        AttributeNode obtn_node  =  attr_node->xmlChildrenNode;
        while (obtn_node != NULL)
        {
            if (xmlIsBlankNode(obtn_node))
            {
                obtn_node = obtn_node->next;
                continue;
            }
            if ( obtn_node  && (strcmp((char *) obtn_node->name,JL_NODE)==0) )
            {
                key = xmlGetProp(obtn_node,(xmlChar *)"name");
                if(key)
                {
                    ActionID *aid = g_new0(ActionID,1);
                    aid->line = NULL;
                    factory_read_object_comobox_value_from_file(obtn_node,aid);
                    nid->actlist = g_list_append(nid->actlist,aid);
                }
                xmlFree(key);
            }
            obtn_node = obtn_node->next;
        }

    }
    else if(!g_ascii_strncasecmp((gchar*)key,"LBTN",4))
    {
        sss->celltype = LBTN;
        sss->isPointer = TRUE;
        ListBtn *ltb = &sss->value.slbtn;
        sss->newdlg_func = factory_create_list_array_manager_dialog;
        sss->close_func = factory_save_objectbutton_dialog;
        key = xmlGetProp(attr_node,(xmlChar *)"value");

        if(key)
        {
            gchar **split = g_strsplit((gchar*)key,",",-1);
            int len = g_strv_length(split);

            if(len < ltb->arr_base.reallen)
            {

                factory_critical_error_exit(g_strdup_printf(factory_utf8("读取的数组个数小于当前定义的．当前:%d,读取:%d"),
                                            ltb->arr_base.reallen,len));
            }
            GList *vlist = ltb->vlist;
            int n = 0;
            for(; vlist; vlist = vlist->next,n++)
            {
                ListBtnArr *lba  = vlist->data;
                *lba->vnumber = g_strdup(split[n]);
            }
            g_strfreev(split);
        }
        xmlFree(key);
    }
    else if(!g_ascii_strncasecmp((gchar*)key,"UBTN",4))
    {
        sss->celltype = UCOMBO;
        SaveUnion *suptr = &sss->value.sunion;
        key = xmlGetProp(attr_node,(xmlChar *)"index");
        if(key)
        {
            suptr->uindex  = atoi((gchar*)key);
            xmlFree(key);
        }
        GList *slist;

        if(sss->type) g_free(sss->type);
        sss->type = g_strdup((gchar*)fst->SType);
        slist =  g_hash_table_lookup(fclass->EnumsAndStructs->unionTable,(gchar*)sss->type);
        /* 下面下拉框当面名字　*/
        FactoryStructItem *nextobj =  g_list_nth_data(slist,suptr->uindex);
//        suptr->structlist = slist;
        g_return_if_fail(nextobj); /* 没有找到控件原型,可能出错了. */

        if(suptr->curkey) g_free(suptr->curkey);
        suptr->curkey =g_strdup(nextobj->Name);

        SaveStruct *nsitm = factory_get_savestruct(nextobj);/* 紧跟它下面的控件名 */
        g_return_if_fail(nsitm);


        nsitm->sclass = sss->sclass;
        SaveUbtn *sbtn = &nsitm->value.ssubtn;
        factory_strjoin(&nsitm->name,suptr->curkey,".");
        /* 把当前选择的成员初始化保存到哈希表 */
        sbtn->structlist = g_list_copy(nextobj->datalist);
        sbtn->savelist = NULL;
        /* 读它下面的子节点 */
        factory_read_union_button_from_file(sss->sclass,
                                            attr_node->xmlChildrenNode,
                                            sbtn);
        g_tree_insert(suptr->ubtreeVal,g_strdup(suptr->curkey),nsitm);
//        g_hash_table_insert(suptr->saveVal,suptr->curkey,nsitm);
        /* 这里不知道为什么，插入一个节点后，这个suptr->curkey 就被free, 　下面又重新设置它的值*/
//       g_free(suptr->curkey);
//       suptr->curkey =g_strdup( nextobj->Name);
    }
    else if(!g_ascii_strncasecmp((gchar*)key,"BBTN",4))
    {
        SaveEntry *sey  = &sss->value.sentry;
        sss->celltype = BBTN;
        sss->isPointer = TRUE;
        sss->newdlg_func = factory_create_basebutton_dialog;
        sss->close_func = factory_save_basebutton_dialog;

        factory_handle_entry_item(sey,fst);
        key = xmlGetProp(attr_node,(xmlChar *)"value");
        g_list_free1(sey->data);
        sey->data = NULL;
        if(key)
        {
            gchar **split = g_strsplit((gchar*)key,",",-1);
            int len = g_strv_length(split);
            int n = 0;
            for(; n < len; n++)
            {
                sey->data = g_list_append(sey->data,g_strdup(split[n]));
            }
            g_strfreev(split);
        }
        xmlFree(key);
    }
    else if(!g_ascii_strncasecmp((gchar*)key,"EBTN",4))
    {
        sss->celltype = EBTN;
//        factory_inital_ebtn(sss,fst);
//        if(factory_is_io_port(fst->SType))
//            factory_inital_io_port_ebtn(sss,fst);
//        else
        factory_inital_ebtn(sss,fst);
        SaveEbtn *sebtn = &sss->value.ssebtn;
        key = xmlGetProp(attr_node,(xmlChar *)"value");
        if(key)
        {
            gchar **split = g_strsplit((gchar*)key,",",-1);
            int len = g_strv_length(split);
            int n = 0;
            for(; n < len; n++)
            {
                SaveEnumArr *sea = g_list_nth_data(sebtn->ebtnwlist,n);
                SaveEnum *se = sea->senum;
                se->evalue = g_strdup(split[n]);
                GQuark equark = g_quark_from_string(se->evalue);
                GList *tlist  = sebtn->ebtnslist;
                for(; tlist; tlist = tlist->next)
                {
                    FactoryStructEnum *fse = tlist->data;
                    GQuark fquark = g_quark_from_string(fse->value);
                    if(fquark == equark)
                    {
                        se->index = g_list_index(sebtn->ebtnslist,fse);
                        break;
                    }
                }
            }
            g_strfreev(split);
        }
        xmlFree(key);
    }
    else
    {
        key = xmlGetProp(attr_node,(xmlChar *)"value");
        if(key)
            sss->value.vnumber = g_strdup((gchar*)key);
        xmlFree(key);
        sss->celltype = SPINBOX;
    }
}

void factory_get_union_child_node( AttributeNode obtn_node )
{
    while (obtn_node != NULL)
    {
        if (xmlIsBlankNode(obtn_node))
        {
            obtn_node = obtn_node->next;
            continue;
        }
        if ( obtn_node  && (strcmp((char *) obtn_node->name,JL_NODE)==0) )
        {
            break;
        }
        obtn_node = obtn_node->next;
    }
}

GList *factory_get_list_from_hashtable(STRUCTClass *fclass)
{
    FactoryStructItemList *fsil = g_hash_table_lookup(fclass->EnumsAndStructs->structTable,
                                  fclass->element.object.name);
    GList *tlist = NULL;
    if(fsil)
        tlist =  fsil->list;
    return tlist;
}

void factory_read_specific_object_from_file(STRUCTClass *fclass,
        ObjectNode obj_node,
        const gchar *filename)
{

    gchar *objname = fclass->element.object.name;
    g_return_if_fail(objname);
    if(!g_ascii_strcasecmp(objname,TYPE_FILELST))
    {

        factory_read_mfile_filelist_from_xml(obj_node,filename);
    }
    else /*IDLST*/
    {
        factory_idlist_read_xml(obj_node);
//        factory_read_idlist_items(obj_node); /* 2014-6-19 改用这个函数读取*/
    }

}





void factory_read_value_from_xml(STRUCTClass *fclass,
                                 ObjectNode obj_node)
{

    GList *tlist = factory_get_list_from_hashtable(fclass);
    g_return_if_fail(tlist);
    GList *tttt = tlist;
    AttributeNode attr_node = obj_node;
    for(; tttt != NULL ; tttt = tttt->next)
    {
        FactoryStructItem *fst = tttt->data;
        fst->orgclass = fclass;
        SaveStruct *sss = factory_get_savestruct(fst);
        sss->widget1 = NULL;
        sss->widget2 = NULL;
        sss->org = fst;
        sss->sclass = fclass;

        xmlChar *key = NULL;
        attr_node = data_next(attr_node);
        if(!attr_node)
            continue;
        key = xmlGetProp(attr_node,(xmlChar *)"name");
        if(!key) continue;

        if(g_ascii_strcasecmp((gchar*)key,fst->Name))
            continue;

        xmlFree(key);
        key =  xmlGetProp(attr_node,(xmlChar *)"type");
        if(!key) continue;
//        if(g_ascii_strcasecmp((gchar*)key,fst->FType) && !factory_is_special_object(fst->FType) )
//            continue;
        /* 添加完整类型名比较 */
        sss->name = g_strdup(fst->Name);
        sss->type = g_strdup(fst->FType);
//        if(factory_is_special_object(fst->FType) && g_ascii_strcasecmp(key,fst->FType) &&
//           !factory_music_fm_get_type(fst->Name))
//        {
//            factory_critical_error_exit(factory_utf8(g_strdup_printf("结构体　%s.\n成员%s.\n和读取的数据不一致．行数:%d",
//                                             fclass->name,fst->Name,attr_node->line)));
//        }
        xmlFree(key);

//        gchar *hkey =  g_strjoin("##",fst->FType,fst->Name,NULL);
        factory_read_object_value_from_file(sss,fst,attr_node);
//        SaveStruct *firstval =  g_hash_table_lookup(fclass->widgetmap,hkey);
        SaveStruct *firstval = g_list_nth_data(fclass->widgetSave,g_list_index(fclass->widgetSave,fst));
        if(firstval)
            *firstval = *sss;
        else
        {
//            g_hash_table_insert(fclass->widgetmap,g_strdup(hkey),sss);
            fclass->widgetSave = g_list_append(fclass->widgetSave,sss);
        }

//        g_free(hkey);
    }

}

void factory_read_object_comobox_value_from_file(AttributeNode attr_node,
        ActionID *aid)
{
    aid->pre_quark = empty_quark;
    aid->value = g_strdup("-1");
    aid->line = NULL;
    aid->conn_ptr = NULL;

    xmlChar *key  =  xmlGetProp(attr_node,(xmlChar *)"idname");
    if(key)
    {
        aid->pre_quark = g_quark_from_string((gchar*)key);
        xmlFree(key);
    }

    key  =  xmlGetProp(attr_node,(xmlChar *)"name");
    if(key)
    {
        g_free(aid->title_name);
        aid->title_name = g_strdup((gchar*)key);
        xmlFree(key);
    }
    key  =  xmlGetProp(attr_node,(xmlChar *)"value");
    if(key)
    {
        aid->value = g_strdup((gchar*)key);
        xmlFree(key);
    }

}



/* 删除一条线,只要找到一个就退出 */
static void factory_actionid_line_removed(STRUCTClass *tclass,
        DiaObject *line)
{
    GList *applist = NULL;

    GList *dlist  = tclass->widgetSave;
    for(; dlist; dlist = dlist->next)
    {
        SaveStruct *sst = (SaveStruct*)dlist->data;

        ActionID *aid = factory_find_ocombox_item_otp(sst,line);
        if(aid)
        {
            aid->line = NULL;
            aid->conn_ptr = NULL;
            aid->pre_quark = empty_quark;
            break;
        }

    }
}


void factory_update_view_names(STRUCTClass *fclass)
{
    g_hash_table_remove(curLayer->defnames,fclass->name);
    GList *connlist = fclass->connections[8].connected; /* 本对像连接多少条线 */
    for(; connlist; connlist = connlist->next)
    {
        Connection *connection  = (Connection *)connlist->data;
        g_return_if_fail(connection);
        ConnectionPoint *start_cp, *end_cp;
        start_cp = connection->endpoint_handles[0].connected_to;
        end_cp = connection->endpoint_handles[1].connected_to;
        STRUCTClass *tclass = NULL;
        if(start_cp)
        {
            /* 把这条线从对像里删掉*/
            tclass = start_cp->object;
            if(tclass != fclass)
            {
                tclass->connections[8].connected =
                    g_list_remove(tclass->connections[8].connected,connection);
                factory_actionid_line_removed(tclass,connection);
            }

        }
        if(end_cp)
        {
            tclass = end_cp->object;
            if(tclass != fclass)
            {
                tclass->connections[8].connected =
                    g_list_remove(tclass->connections[8].connected,connection);
            }

        }

//        diagram_select(ddisplay_active()->diagram,connection);
//        edit_delete_callback(NULL); /*调用一个现有的函数删除线条*/
        layer_remove_object(curLayer,connection);
        diagram_flush(ddisplay_active()->diagram);
    }

    if(ddisplay_active_diagram()->isTemplate)
    {
        factory_template_update_item(fclass->name); /* 如果是模版模式,要更新一下 */
    }
}

void factory_inital_ebtn(SaveStruct *sss,const FactoryStructItem *fst)
{
    SaveEbtn *sebtn = &sss->value.ssebtn;
    g_return_if_fail(sebtn);
    sebtn->ebtnslist = g_hash_table_lookup(factoryContainer->enumTable,fst->SType);
    sebtn->ebtnwlist = NULL;
    sebtn->width = g_strdup(fst->Max);
    sss->close_func = factory_save_enumbutton_dialog;
    sss->newdlg_func = factory_create_enumbutton_dialog;
    if(factory_is_io_port(fst->SType))
        sss->newdlg_func = factory_create_io_port_dialog;
    GList *tmplist = sebtn->ebtnslist;
    int index = 0;
    gchar *value = NULL;
    GList *fill_list = NULL;
    for(; tmplist; tmplist = tmplist->next)
    {
        FactoryStructEnum *kvmap = tmplist->data;
        if(!g_ascii_strcasecmp(fst->Value,kvmap->key))
        {
            index = g_list_index(sebtn->ebtnslist,kvmap);
            value = kvmap->value;
        }
        fill_list = g_list_append(fill_list,kvmap->key);
    }
    SaveEntry tmp;
    factory_handle_entry_item(&tmp,fst);
    sebtn->arr_base  = tmp.arr_base;

    GList *tlist  = sebtn->ebtnslist; /* 源数据用填充下拉框,这里来自枚举*/
    GList *nixlist = NULL;
    int n = 0;
    gchar **title = g_strsplit(fst->Name,"[",-1);
    gchar *name =  g_strconcat(title[0],"(%d)",NULL);
    g_strfreev(title);
    n = 0;
    for(; n < sebtn->arr_base.reallen; n++)
    {
        SaveEnum *see = g_new0(SaveEnum,1);
        see->enumList = fill_list;
        see->index = index;
        see->evalue = value;
        see->width = fst->Max;
        SaveEnumArr *sea = g_new0(SaveEnumArr,1);
        sea->widget1 = NULL;
        sea->widget2 = NULL;
        sea->senum = see;
        sebtn->ebtnwlist = g_list_append(sebtn->ebtnwlist,sea);
    }
}


SaveStruct *factory_get_action_savestruct(SaveStruct *sss,FactoryStructItem *fst)
{

    if(g_str_has_suffix(sss->name,"]"))
    {
        ActIDArr *nid = &sss->value.nextid;
//        nid->itemlist = NULL;
        nid->actlist = NULL;
        nid->wlist = NULL;
        sss->celltype = OBTN; /* 这里是数组了,需要按键创建新窗口来设置 */
        sss->newdlg_func = factory_create_objectbutton_dialog;
        sss->close_func = factory_save_objectbutton_dialog;
        SaveEntry tmp;
        factory_handle_entry_item(&tmp,fst);
        nid->arr_base = tmp.arr_base;

        gchar **title = g_strsplit(sss->name,"[",-1);
        gchar *name =  g_strconcat(title[0],"(%d)",NULL);
        g_strfreev(title);

        int n = 0;
        for(; n < nid->arr_base.reallen; n++ )
        {
            ActionID *aid = g_new0(ActionID,1);
            aid->pre_quark = empty_quark;
            aid->value = g_strdup("-1");
            aid->title_name = g_strdup_printf(name,n);
            aid->conn_ptr = NULL;
            aid->line = NULL;
            nid->actlist = g_list_append(nid->actlist,aid);
        }
        g_free(name);
    }
    else
    {
        sss->celltype = OCOMBO;
        ActionID *aid = &sss->value.actid;
        aid->pre_quark = empty_quark;
        aid->value = g_strdup("-1");
        aid->title_name = g_strdup(sss->name);
    }
    sss->org = fst;
    return sss;
}

SaveStruct * factory_get_savestruct(FactoryStructItem *fst)
{
    g_return_if_fail(fst);
//    g_return_if_fail(fst->Name);
    SaveStruct *sss = g_new0(SaveStruct,1);
    sss->widget1= NULL;
    sss->widget2 = NULL;

    sss->type = g_strdup(fst->FType);
    sss->name = g_strdup(fst->Name);
    sss->sclass = fst->orgclass;
    sss->close_func = NULL;
    sss->newdlg_func = NULL;
    sss->templ_pos = 0;
    sss->templ_quark =0;

    if(!sss->name)
    {
        factory_critical_error_exit(factory_utf8(g_strdup_printf("%s类型,没有名字．",sss->type)));
    }

    if(!g_ascii_strncasecmp(sss->name,ACTION_ID,6))
    {
        return factory_get_action_savestruct(sss,fst);
    }

    /* 2014-3-26 lcy 通过名字去哈希表里找链表*/

    switch(fst->Itype)
    {
    case BT:
    {
        if( factory_find_array_flag(fst->Name))
        {
            /* 2014-3-25 lcy 这里是字符串，需用文本框显示了*/
            sss->celltype = ENTRY;
            SaveEntry *sey = &sss->value.sentry;
            factory_handle_entry_item(sey,fst);
            if(!sey->isString)
            {
                sss->celltype = BBTN;
                sss->newdlg_func = factory_create_basebutton_dialog;
                sss->close_func = factory_save_basebutton_dialog;
                if( 2 == strlen(fst->SType) && !g_ascii_strcasecmp(fst->SType,"u1"))
                {
                    int n = 0 ;
                    int maxi = sey->arr_base.reallen;
                    for( ; n < maxi; n++)
                    {
                        /* 初始值 */
                        sey->data =  g_list_append(sey->data,g_strdup_printf("%d",0));
                    }
                }
                else
                {
                    gchar *fmt = NULL;
                    int len = 2;
                    switch(sey->width)
                    {
                    case 1:
                        fmt = g_strdup("0x%02x");
                        len = 4;
                        break;
                    case 2:
                        fmt = g_strdup("0x%04x");
                        len = 6;
                        break;
                    case 4:
                        fmt = g_strdup("0x%08x");
                        len = 6;
                        break;
                    default:
                        fmt = g_strdup("0x%02x");
                    }
                    int n = 0 ;
                    int maxi = sey->arr_base.reallen;
                    int first = g_strtod(fst->Value,NULL);
                    for( ; n < maxi; n++)
                    {
                        /* 初始值 */
                        gchar *tmp = g_strdup_printf(fmt,first);
                        tmp[len]='\0';
                        sey->data =  g_list_append(sey->data,g_strdup(tmp));
                        g_free(tmp);
                    }
                    g_free(fmt);
                }

            }
        }
        else
        {
            sss->celltype = SPINBOX;
            sss->value.vnumber = g_strdup(fst->Value);
        }
    }
    break;
    case ET:
    {
        if( factory_find_array_flag(fst->Name))
        {
            sss->celltype = EBTN;
//                if(factory_is_io_port(fst->SType))
//                    factory_inital_io_port_ebtn(sss,fst);
//                else
            factory_inital_ebtn(sss,fst);
        }
        else
        {
            SaveEnum *senum = &sss->value.senum;
            sss->celltype = ECOMBO;
            senum->enumList = fst->datalist;
//                sss->value.senum.width = g_strdup(fst->Max);
            GList *t = senum->enumList;
            GQuark vquark = g_quark_from_string(fst->Value);
            for(; t != NULL ; t = t->next)
            {
                FactoryStructEnum *kvmap = t->data;
                GQuark kquark = g_quark_from_string(kvmap->key);
                if(kquark == vquark)
                {
                    senum->index = g_list_index(fst->datalist,kvmap);
                    senum->width = g_strdup(fst->Max);
                    senum->evalue = g_strdup(kvmap->value);
                    break;
                }

            }
            if(!t && !senum->width ) /*源文件有错误，这里用默认值*/
            {
                gchar *msg =
                    g_strdup_printf(factory_utf8("对像名:%s\n成员名:%s\n找不到枚举对像的默认值,%s\n请检查源文件."),
                                    sss->sclass->name,sss->name,fst->Value);
                factory_critical_error_exit(msg);
//                sss->value.senum.index = 0;
//                sss->value.senum.width = g_strdup(fst->Max);
//                FactoryStructEnum *kvmap = sss->value.senum.enumList->data;
//                sss->value.senum.evalue = g_strdup(kvmap->value);
            }

        }
    }
    break;
    case ST:
    {
        sss->celltype = UBTN;
        sss->isPointer = TRUE; /* 这里是一个按键*/

        if(!g_strcasecmp(fst->SType,TYPE_FILELST)) /*　 特殊的控件,音乐文件管理　*/
        {
            SaveMusicDialog *smd = NULL;
            if(!curLayer->smd)
            {
                curLayer->smd = g_new0(SaveMusicDialog,1);
                smd =curLayer->smd;
//                smd->smfm = NULL;
//                smd->mfmos = &mfmo_opts;
                smd->title = factory_utf8("文件管理");
//                smd->lastDir = g_strdup("");
//                smd->smfm = g_new0(SaveMusicFileMan,1);
                /* 根据文件个数算偏移数 */
                gchar **split = g_strsplit(factoryContainer->system_files,",",-1);
                smd->offset = g_strv_length(split);
                g_strfreev(split);
                /* 这里用数字哈希表来保存,文件名的hash值 */
//                smd->mtable = g_hash_table_new(g_direct_hash,g_direct_equal);
                smd->mbtree = g_tree_new(g_ascii_strcasecmp);
//                smd->midtable = g_hash_table_new(g_direct_hash,g_direct_equal);
            }

            smd = curLayer->smd;



            if(g_str_has_suffix(fst->Name,"]")) /* 这里是一个数组*/
            {
                sss->celltype = LBTN;
                ListBtn *ltb = &sss->value.slbtn;
                SaveEntry tmp;
                factory_handle_entry_item(&tmp,fst);
                ltb->arr_base = tmp.arr_base;
                int r = 0;
                for(; r < ltb->arr_base.reallen ; r++ ) /* 初始化一下 */
                {
                    ListBtnArr *lba = g_new0(ListBtnArr,1);
                    lba->widget1 = NULL;
                    lba->vnumber  =  g_new0(gchar**,1);
                    *lba->vnumber = g_strdup(fst->Value);
                    ltb->vlist = g_list_append(ltb->vlist,lba);
                }

                sss->newdlg_func = factory_create_list_array_manager_dialog;
//                sss->close_func = factory_save_list_array_manager_dialog;
            }
            else
            {
                if(factory_music_fm_item_is_index(fst->Name))
                {
//                    if(factory_music_fm_get_position_type(item->Name) == OFFSET_SEL )
//                    {
                    gint val = g_strtod(sss->value.vnumber,NULL);
                    g_free(sss->value.vnumber);
                    sss->value.vnumber = g_new0(SaveSel,1);
                    SaveSel *ssel = sss->value.vnumber;
                    ssel->ntable = NULL;
                    ssel->offset_val = val;
//                    }
//                    else
//                    {
//
//                    }
                }
                else
                {
                    /* sss->value.vnumber  原定义为一个gchar 指针，在这里当做gpointer 用了*/
                    sss->value.vnumber = g_strdup(fst->Value);
//                smd->vnumber = sss->value.vnumber;
                    smd->btnname = g_strdup(fst->Cname);

                    // sss->close_func = factory_music_file_manager_apply;
                }
                sss->newdlg_func = factory_create_file_manager_dialog;

            }

        }
        else if(!g_strcasecmp(fst->SType,TYPE_IDLST))
        {
            SaveIdDialog   *sid = NULL;
            if(!curLayer->sid)
                curLayer->sid = g_new0(SaveIdDialog,1);

            sid =(SaveIdDialog*)(curLayer->sid);

            sss->value.vnumber = g_new0(SaveSel,1);
            SaveSel *ssel = sss->value.vnumber;
            ssel->ntable = NULL;
            ssel->offset_val = 0; /* 定在开头 */

//            sss->value.vnumber = g_strdup(fst->Value);
//            sss->newdlg_func = factory_new_idlist_dialog;
            sss->newdlg_func = factory_idlist_create_dialog;
//            sss->close_func = factory_save_idlist_dialog;
        }
        else
        {
            SaveUbtn *sbtn = &sss->value.ssubtn;
            sbtn->structlist = fst->datalist;
            sbtn->savelist = NULL;
            sss->newdlg_func = factory_create_unionbutton_dialog;
            sss->close_func = factory_save_unionbutton_dialog;
        }
    }
    break;
    case UT:
    {
        /*  这种成员最少有两个 gtk_widget */
        SaveUnion *suptr = &sss->value.sunion;
        sss->celltype = UCOMBO;
        suptr->vbox = NULL;
        suptr->pre_quark = empty_quark;
        suptr->ubtreeVal = g_tree_new(factory_str_compare);
        suptr->curkey = g_strdup("");
        suptr->comobox = NULL;
        suptr->structlist = fst->datalist;
        /* nextobj 就是当前下拉框所显示的 */
        FactoryStructItem *nextobj =
            factory_get_factorystructitem(suptr->structlist,fst->Value);
        if(!nextobj)
        {
            gchar *msg =
                g_strdup_printf(factory_utf8("对像名:%s\n成员名:%s\n找不到联合体的默认值,%s\n请检查源文件.\n"),
                                sss->sclass->name,sss->name,fst->Value);
            factory_critical_error_exit(msg);
            break;
        }

        suptr->uindex = g_list_index(suptr->structlist,nextobj);
        GList *p = suptr->structlist;
//        suptr->curtext = g_strdup(nextobj->Name);
        suptr->curkey = g_strdup(nextobj->Name);
        SaveStruct *tsst = factory_get_savestruct(nextobj);
        if(!tsst)
            break;
        tsst->sclass = sss->sclass;
        factory_strjoin(&tsst->name,suptr->curkey,".");
        /* 把当前选择的成员初始化保存到哈希表 */
        g_tree_insert(suptr->ubtreeVal,g_strdup(suptr->curkey),tsst);
//        g_hash_table_insert(suptr->saveVal,suptr->curkey,tsst);
        if(tsst->isPointer )
        {
            SaveUbtn *sbtn = &tsst->value.ssubtn;
            if(!g_list_length(sbtn->savelist) && sbtn->structlist)
            {

//                sbtn->htoflist = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
                GList *slist = sbtn->structlist;
                for(; slist; slist = slist->next)
                {
                    FactoryStructItem *o = slist->data;
                    SaveStruct *s  = factory_get_savestruct(o);
                    s->sclass = sss->sclass;
                    factory_strjoin(&s->name,suptr->curkey,".");
                    sbtn->savelist = g_list_append(sbtn->savelist,s);
                }
            }
        }
    }
    break;

    }
    sss->org = fst;
    return sss;
}

void factory_set_original_class(STRUCTClass *fclass)
{
    GList *tlist = factory_get_list_from_hashtable(fclass);
    for(; tlist; tlist = tlist->next)
    {
        FactoryStructItem *fst = tlist->data;
        fst->orgclass = fclass;
    }
}

void factory_set_structsave_class(STRUCTClass *fclass)
{
    GList *tlist = fclass->widgetSave;
    for(; tlist; tlist = tlist->next)
    {
        SaveStruct *sst = tlist->data;
        sst->sclass = fclass;
    }
}


void factory_read_initial_to_struct(STRUCTClass *fclass) /*2014-3-26 lcy 拖入控件时取得它的值*/
{
    /* 这里从原哈希表复制一份出来 */
//    gchar **tmp =  g_strsplit(fclass->name,"(",-1);
    factory_debug_to_log(g_strdup_printf(factory_utf8("初始化对像,名字:%s.\n"),fclass->name));
    factory_set_original_class(fclass);
    GList *tttt = factory_get_list_from_hashtable(fclass);
    for(; tttt != NULL ; tttt = tttt->next)
    {
        FactoryStructItem *fst = tttt->data;
        SaveStruct *sst= factory_get_savestruct(fst);
        factory_debug_to_log(g_strdup_printf(factory_utf8("初始化成员,名字:%s.\n"),sst->name));
//        g_hash_table_insert(fclass->widgetmap,g_strjoin("##",fst->FType,fst->Name,NULL),sst);
        fclass->widgetSave = g_list_append(fclass->widgetSave,sst);
    }

}



void
factory_handle_entry_item(SaveEntry* sey,FactoryStructItem *fst)
{
    /* 2014-3-25 lcy 这里是字符串，需用文本框显示了*/
    /* lcy  array[3][2]   ==>  ooo[0]="array", ooo[1]="3", ooo[2]="", ooo[3]="2" ,ooo[4]="" */

    ArrayBaseProp *abp  = &sey->arr_base;
    abp->reallen = 0; /* 总长度 r * c */
    gchar **ooo = g_strsplit_set (fst->Name,"[]",-1);
    int num = g_strv_length(ooo);

    sey->width = 0;
    if(g_ascii_isalnum(fst->SType[1]))
        sey->width = g_strtod(&fst->SType[1],NULL) / 8; /* u32,u8, 这里取里面的数字*/


//    if(2== strlen(fst->SType) && !g_ascii_strncasecmp(fst->SType,"u1",2))
    if(sey->width == 0 )
        sey->width = 1;
    abp->row = 1;
    if(num >= 5)
    {
        abp->row = g_strtod(ooo[1],NULL); /* 如果是二维数组,这里是行 */
        abp->col = g_strtod(ooo[3],NULL);
        abp->reallen = abp->row * abp->col;
    }
    else
    {
        abp->col = g_strtod(ooo[1],NULL);
        abp->reallen = abp->col;
    }
    g_strfreev(ooo);


    sey->isString =  !g_ascii_strncasecmp(fst->SType,"s8",2) ? TRUE : FALSE;
    int strlength = sey->width * abp->row * abp->col; /**   u32[6][2] ==  (32/8) * 6 * 2    **/
    if(sey->isString)
    {
        sey->data = g_new0(gchar,strlength );
        sey->data =  g_locale_to_utf8(fst->Value,strlength,NULL,NULL,NULL);
    }
    else
    {
//                    if(!g_ascii_strncasecmp(fst->SType,"u1",2))
        if( (abp->row == 1) && (abp->col > 8))
        {
            /* 2014-3-31 lcy 一维数组化成二维数组用来显示*/
            int sqrtv = sqrt(abp->col);
            int modv = abp->col % 8;
            if((sqrtv * sqrtv  == abp->col) && (sqrtv > 8)) /* 可开方成整数 */
                abp->row = abp->col = sqrtv;
            else if(modv) /* 看一下能不能用 8x8 的矩阵*/
            {
                abp->row = abp->col / 8;
                abp->row++;
                abp->col = 8;
            }
            else
            {
                abp->row = sey->width * 2;
                abp->col = abp->col / abp->row;
            }

        }
//                    sey->data.arrstr = g_new0(unsigned char,strlength);
//                    memset(sey->data.arrstr,0xff,strlength);
    }
}

static void
structclass_destroy(STRUCTClass *structclass)
{
    structclass->destroyed = TRUE;

    dia_font_unref(structclass->normal_font);
//  dia_font_unref(structclass->abstract_font);
//  dia_font_unref(structclass->polymorphic_font);
    dia_font_unref(structclass->classname_font);
//  dia_font_unref(structclass->abstract_classname_font);
//  dia_font_unref(structclass->comment_font);

    element_destroy(&structclass->element);

    g_free(structclass->name);
//  g_free(structclass->stereotype);
//  g_free(structclass->comment);
//
//  list = structclass->attributes;
//  while (list != NULL) {
//    attr = (STRUCTAttribute *)list->data;
//    g_free(attr->left_connection);
//    g_free(attr->right_connection);
//    struct_attribute_destroy(attr);
//    list = g_list_next(list);
//  }
//  g_list_free(structclass->attributes);
//
//  list = structclass->operations;
//  while (list != NULL) {
//    op = (STRUCTOperation *)list->data;
//    g_free(op->left_connection);
//    g_free(op->right_connection);
//    struct_operation_destroy(op);
//    list = g_list_next(list);
//  }
//  g_list_free(structclass->operations);
//
//  list = structclass->formal_params;
//  while (list != NULL) {
//    param = (STRUCTFormalParameter *)list->data;
//    struct_formalparameter_destroy(param);
//    list = g_list_next(list);
//  }
//  g_list_free(structclass->formal_params);
//
//  if (structclass->stereotype_string != NULL) {
//    g_free(structclass->stereotype_string);
//  }
//    if(structclass->widgetmap && g_hash_table_size(structclass->widgetmap))
//    {
//        g_hash_table_destroy(structclass->widgetmap);
//    }

    if (structclass->properties_dialog != NULL)
    {
        structclass_dialog_free (structclass->properties_dialog);
    }
//    g_list_foreach(structclass->widgetSave,(GFunc)g_free,NULL);
    g_list_free(structclass->widgetSave);
    g_free(structclass);
}

static void factory_hashtable_copy(gpointer key,gpointer value,gpointer user_data)
{
    GHashTable *t = (GHashTable *)user_data;
    g_hash_table_insert(t,key,value);
}

static DiaObject *
structclass_copy(STRUCTClass *structclass)
{
    int i;
    STRUCTClass *newstructclass;
    Element *elem, *newelem;
    DiaObject *newobj;
//  GList *list;
//  STRUCTFormalParameter *param;

    elem = &structclass->element;

    newstructclass = g_malloc0(sizeof(STRUCTClass));
    newelem = &newstructclass->element;
    newobj = &newelem->object;
    newobj->name = g_strdup(elem->object.name);

    if(factory_is_system_data(newobj->name))
    {
        return NULL; /* 系统信息不能copy*/
    }

    newobj->oindex = g_list_length(curLayer->objects);
    newobj->isTemplate = structclass->element.object.isTemplate;

    element_copy(elem, newelem);

    newstructclass->font_height = structclass->font_height;
    newstructclass->abstract_font_height = structclass->abstract_font_height;
    newstructclass->polymorphic_font_height = structclass->polymorphic_font_height;
    newstructclass->classname_font_height = structclass->classname_font_height;
    newstructclass->abstract_classname_font_height =
        structclass->abstract_classname_font_height;
    newstructclass->comment_font_height =
        structclass->comment_font_height;

    newstructclass->normal_font =
        dia_font_copy(structclass->normal_font);
//  newstructclass->abstract_font =
//          dia_font_copy(structclass->abstract_font);
//  newstructclass->polymorphic_font =
//          dia_font_copy(structclass->polymorphic_font);
    newstructclass->classname_font =
        dia_font_copy(structclass->classname_font);
//  newstructclass->abstract_classname_font =
//          dia_font_copy(structclass->abstract_classname_font);
//  newstructclass->comment_font =
//          dia_font_copy(structclass->comment_font);
    newstructclass->name = g_strdup(structclass->name);
    newstructclass->isInitial = TRUE;
    newstructclass->hasIdnumber = FALSE;

    //  factory_rename_structclass(newstructclass);
//    newstructclass->name = g_strdup(structclass->name);
//  if (structclass->stereotype != NULL && structclass->stereotype[0] != '\0')
//    newstructclass->stereotype = g_strdup(structclass->stereotype);
//  else
//    newstructclass->stereotype = NULL;
//
//  if (structclass->comment != NULL)
//    newstructclass->comment = g_strdup(structclass->comment);
//  else
//    newstructclass->comment = NULL;
//
//  newstructclass->abstract = structclass->abstract;
//  newstructclass->suppress_attributes = structclass->suppress_attributes;
//  newstructclass->suppress_operations = structclass->suppress_operations;
//  newstructclass->visible_attributes = structclass->visible_attributes;
//  newstructclass->visible_operations = structclass->visible_operations;
//  newstructclass->visible_comments = structclass->visible_comments;
//  newstructclass->wrap_operations = structclass->wrap_operations;
//  newstructclass->wrap_after_char = structclass->wrap_after_char;
//  newstructclass->comment_line_length = structclass->comment_line_length;
//  newstructclass->comment_tagging = structclass->comment_tagging;
    newstructclass->line_width = structclass->line_width;
    newstructclass->text_color = structclass->text_color;
    newstructclass->line_color = structclass->line_color;
    newstructclass->fill_color = structclass->fill_color;
    newstructclass->vcolor = structclass->vcolor;

//  newstructclass->attributes = NULL;
//  list = structclass->attributes;
//  while (list != NULL) {
//    STRUCTAttribute *attr = (STRUCTAttribute *)list->data;
//    /* not copying the connection, if there was one */
//    STRUCTAttribute *newattr = struct_attribute_copy(attr);
//    struct_attribute_ensure_connection_points (newattr, newobj);
//
//    newstructclass->attributes = g_list_append(newstructclass->attributes,
//					    newattr);
//    list = g_list_next(list);
//  }
//
//  newstructclass->operations = NULL;
//  list = structclass->operations;
//  while (list != NULL) {
//    STRUCTOperation *op = (STRUCTOperation *)list->data;
//    STRUCTOperation *newop = struct_operation_copy(op);
//    struct_operation_ensure_connection_points (newop, newobj);
//
//    newstructclass->operations = g_list_append(newstructclass->operations,
//					     newop);
//    list = g_list_next(list);
//  }
//
//  newstructclass->template = structclass->template;
//
//  newstructclass->formal_params = NULL;
//  list = structclass->formal_params;
//  while (list != NULL) {
//    param = (STRUCTFormalParameter *)list->data;
//    newstructclass->formal_params =
//      g_list_append(newstructclass->formal_params,
//		     struct_formalparameter_copy(param));
//    list = g_list_next(list);
//  }
//  newstructclass->stereotype_string = NULL;

    for (i=0; i<STRUCTCLASS_CONNECTIONPOINTS; i++)
    {
        newobj->connections[i] = &newstructclass->connections[i];
        newstructclass->connections[i].object = newobj;
        newstructclass->connections[i].connected = NULL;
        newstructclass->connections[i].pos = structclass->connections[i].pos;
        newstructclass->connections[i].last_pos = structclass->connections[i].last_pos;
    }

    structclass_calculate_data(newstructclass);

    i = STRUCTCLASS_CONNECTIONPOINTS;
//  if ( (newstructclass->visible_attributes) &&
//       (!newstructclass->suppress_attributes)) {
//    list = newstructclass->attributes;
//    while (list != NULL) {
//      STRUCTAttribute *attr = (STRUCTAttribute *)list->data;
//      newobj->connections[i++] = attr->left_connection;
//      newobj->connections[i++] = attr->right_connection;
//
//      list = g_list_next(list);
//    }
//  }
//
//  if ( (newstructclass->visible_operations) &&
//       (!newstructclass->suppress_operations)) {
//    list = newstructclass->operations;
//    while (list != NULL) {
//      STRUCTOperation *op = (STRUCTOperation *)list->data;
//      newobj->connections[i++] = op->left_connection;
//      newobj->connections[i++] = op->right_connection;
//
//      list = g_list_next(list);
//    }
//  }

#ifdef STRUCT_MAINPOINT
    newobj->connections[i] = &newstructclass->connections[STRUCTCLASS_CONNECTIONPOINTS];
    newstructclass->connections[STRUCTCLASS_CONNECTIONPOINTS].object = newobj;
    newstructclass->connections[STRUCTCLASS_CONNECTIONPOINTS].connected = NULL;
    newstructclass->connections[STRUCTCLASS_CONNECTIONPOINTS].pos =
        structclass->connections[STRUCTCLASS_CONNECTIONPOINTS].pos;
    newstructclass->connections[STRUCTCLASS_CONNECTIONPOINTS].last_pos =
        structclass->connections[STRUCTCLASS_CONNECTIONPOINTS].last_pos;
    newstructclass->connections[STRUCTCLASS_CONNECTIONPOINTS].flags =
        structclass->connections[STRUCTCLASS_CONNECTIONPOINTS].flags;
    i++;
#endif

    structclass_update_data(newstructclass);

    newstructclass->EnumsAndStructs = factoryContainer;
//    newstructclass->widgetmap = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
    newstructclass->widgetSave = NULL;
    if(structclass->pps)
    {
        newstructclass->pps = g_new0(PublicSection,1);
        newstructclass->pps->hasfinished = structclass->pps->hasfinished;
    }


    GList *plist = structclass->widgetSave;
    for(; plist; plist = plist->next)
    {
        SaveStruct *sst = factory_savestruct_copy(plist->data);
        sst->sclass = newstructclass;
        newstructclass->widgetSave =
            g_list_append(newstructclass->widgetSave,sst);
    }
//    g_hash_table_foreach(structclass->widgetmap,factory_hashtable_copy,newstructclass->widgetmap);
    newstructclass->properties_dialog = NULL;

//    factory_set_original_class(newstructclass);
    return &newstructclass->element.object;
}


static void factory_base_item_save(SaveStruct *sss,ObjectNode ccc)
{
    g_return_if_fail(sss);
    xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)sss->name);
    xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)sss->type);

    /* 这里会递归调用 */
    switch(sss->celltype)
    {
    case ECOMBO:
        xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"ECOMBO");
        xmlSetProp(ccc, (const xmlChar *)"width", (xmlChar *)sss->value.senum.width);
        xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)sss->value.senum.evalue);
        break;
    case ENTRY:
    {
        xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"ENTRY");
        SaveEntry *sey = &sss->value.sentry;
        xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)sey->data);
    }
    break;

    case LBTN:
    {
        xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
        xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"LBTN");
        ListBtn *lbtn = &sss->value.slbtn;
        GList *tlist = lbtn->vlist;
        gchar *ret =g_strdup("");

        for(; tlist; tlist = tlist->next)
        {
            /* 2014-3-31 lcy 把链表里的数据用逗号连接 */
            ListBtnArr *lba = tlist->data;
            gchar *p = g_strconcat(g_strdup(ret),*lba->vnumber,g_strdup(","),NULL);
            g_free(ret);
            ret = g_strdup(p);
            g_free(p);
        }
        int len = strlen(ret);
        if(ret[len-1]==',')
            ret[len-1] = '\0';
        xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)ret);
    }
    break;
    case BBTN:
    {
        xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"BBTN");
        SaveEntry *sey = &sss->value.sentry;
        GList *tlist = sey->data;
        gchar *ret =g_strdup("");

        for(; tlist; tlist = tlist->next)
        {
            /* 2014-3-31 lcy 把链表里的数据用逗号连接 */
            gchar *p = g_strconcat(g_strdup(ret),tlist->data,g_strdup(","),NULL);
            g_free(ret);
            ret = g_strdup(p);
            g_free(p);
        }
        int len = strlen(ret);
        if(ret[len-1]==',')
            ret[len-1] = '\0';
        xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)ret);
    }
    break;
    case EBTN:
    {
        xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"EBTN");
        SaveEbtn *sebtn = &sss->value.ssebtn;
        xmlSetProp(ccc, (const xmlChar *)"width", (xmlChar *)sebtn->width);
        GList *tlist = sebtn->ebtnwlist;
        gchar *ret =g_strdup("");

        for(; tlist; tlist = tlist->next)
        {
            /* 2014-3-31 lcy 把链表里的数据用逗号连接 */
            SaveEnumArr *sea = tlist->data;
            SaveEnum *se = sea->senum;
            gchar *p = g_strconcat(g_strdup(ret),se->evalue,g_strdup(","),NULL);
            g_free(ret);
            ret = g_strdup(p);
            g_free(p);
        }
        int len = strlen(ret);
        if(ret[len-1]==',')
            ret[len-1] = '\0';
        xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)ret);
    }
    break;
//    case SBTN:
    case SPINBOX:
        if(!g_ascii_strncasecmp(sss->org->Name,WACDID,6)) /*这里一个关键字判断是否是ＩＤ*/
        {
            STRUCTClass *sclass = sss->sclass;
            sss->value.vnumber = g_strdup_printf("%d",sclass->element.object.oindex);
        }
        xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"SPINBOX");
        xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)sss->value.vnumber);
        break;
    }
    /* 这里是不是要加一个OCOMBO 控件识别呢? */
}

static void factory_write_object_comobox(ActionID *aid,ObjectNode ccc ,const gchar *type)
{
    xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)aid->title_name);

    if(!aid->conn_ptr && (aid->pre_quark == empty_quark))
    {
        g_free(aid->value);
        aid->value = g_strdup("-1");
    }

    xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)type);
    xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"OCOMBO");
    if(aid->conn_ptr)
        aid->value =
            g_strdup_printf("%d",((DiaObject*)aid->conn_ptr)->oindex);
    xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)aid->value );

//    xmlSetProp(ccc, (const xmlChar *)"index", (xmlChar *)g_strdup_printf(_("%d"),aid->index));
//    gchar *pre_name = g_quark_to_string(aid->pre_quark);

    gchar *pre_name = aid->conn_ptr ? ((STRUCTClass*)aid->conn_ptr)->name
                      : g_quark_to_string(empty_quark);
    xmlSetProp(ccc, (const xmlChar *)"idname",(xmlChar *)pre_name );

}

static void factory_base_struct_save_to_file(SaveStruct *sss,ObjectNode obj_node)
{
    g_return_if_fail(sss);
    factory_debug_to_log(g_strdup_printf(factory_utf8("保存对像成员,名字:%s.\n"),sss->name));
    switch(sss->celltype)
    {
    case ECOMBO:
    case ENTRY:
    case BBTN:
    case EBTN:
    case SPINBOX:
    case LBTN:
//    case SBTN:
    {
        ObjectNode ccc = xmlNewChild(obj_node, NULL,
                                     (const xmlChar *)JL_NODE, NULL);
        factory_base_item_save(sss,ccc);
    }
    break;
    case OCOMBO:
    {
        ObjectNode ccc = xmlNewChild(obj_node, NULL,
                                     (const xmlChar *)JL_NODE, NULL);
        ActionID *aid = &sss->value.actid;
        factory_write_object_comobox(aid,ccc,sss->type);
    }
    break;
    case OBTN:
    {
        ActIDArr *nid  = &sss->value.nextid;
        g_return_if_fail(nid);
        GList *tlist = nid->actlist;
        ObjectNode obtn = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_obtn", NULL);
        xmlSetProp(obtn, (const xmlChar *)"name", (xmlChar *)sss->name);
        xmlSetProp(obtn, (const xmlChar *)"wtype", (xmlChar *)"OBTN");
        for(; tlist; tlist = tlist->next)
        {
            ActionID *aid = tlist->data;
            ObjectNode ccc = xmlNewChild(obtn, NULL, (const xmlChar *)JL_NODE, NULL);
            factory_write_object_comobox(tlist->data,ccc,sss->type);
        }
    }
    break;
    case UCOMBO:
    {
        SaveUnion *suptr = &sss->value.sunion;
        g_return_if_fail(suptr);
//        SaveStruct *tsst  =  g_hash_table_lookup(suptr->saveVal,suptr->curkey);
        SaveStruct *tsst = g_tree_lookup(suptr->ubtreeVal,
                                         suptr->curkey);
        if(tsst)
        {
            ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_union", NULL);
            xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)sss->name);
            xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)sss->type);
            xmlSetProp(ccc,(const xmlChar *)"idname",(xmlChar*)suptr->curkey);
            if(tsst->isPointer)
            {
                xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"UBTN");
                xmlSetProp(ccc, (const xmlChar *)"index", (xmlChar *)g_strdup_printf("%d",suptr->uindex));
                factory_base_struct_save_to_file(tsst,ccc); /* 递归调用本函数 */
            }
            else
            {
                ObjectNode aaa = xmlNewChild(ccc, NULL, (const xmlChar *)"JL_union", NULL);
                xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)tsst->type);
                factory_base_item_save(tsst,aaa);
            }

        }
    }
    break;
    case UBTN:
    {
        gchar *laststr = factory_get_last_section(sss->type,".");
//        if(factory_is_special_object(stype)) /* 特殊控件 */
        if(!g_strcasecmp(laststr,TYPE_FILELST))
        {
//            factory_mfile_idlist_save_xml(sss,obj_node);
            factory_mfile_save_item_to_xml1(sss,obj_node);
        }
        else if(!g_strcasecmp(laststr,TYPE_IDLST))
        {
//            factory_save_idlist_to_xml(sss,obj_node);
            factory_idlist_item_save_to_xml(sss,obj_node);
        }
        else
        {
            SaveUbtn *sbtn = &sss->value.ssubtn;
            g_return_if_fail(sbtn);
//        GList *slist = g_hash_table_get_values(sbtn->htoflist);
            GList *slist = sbtn->savelist;

            for(; slist; slist = slist->next)
            {
                SaveStruct *s = slist->data;
                factory_base_struct_save_to_file(s,obj_node);
            }

        }
        g_free(laststr);
    }
    break;
    default:
        break;
    }
}

static void factory_struct_save_to_xml(gpointer key,gpointer value,gpointer user_data)
{
    /* 2014-3-27 lcy 这里每行与XML的行对应用 ,采用多个属性值存储*/
    SaveStruct *sss = (SaveStruct*)value;
    factory_base_struct_save_to_file(sss,user_data);
}



static DiaObject *
factory_struct_items_load(ObjectNode obj_node,int version,
                          const char *filename)
{

    STRUCTClass *structclass;
    Element *elem;
    DiaObject *obj;
    AttributeNode attr_node;
    int i;
//  GList *list;
    structclass = g_malloc0(sizeof(STRUCTClass));
    elem = &structclass->element;
    obj = &elem->object;
    obj->type = &structclass_type;
    obj->ops = &structclass_ops;

    xmlChar*   key = xmlGetProp(obj_node,(xmlChar *)"templ");
    if(key)
    {
        obj->isTemplate  = g_strtod(key,NULL);
        xmlFree (key);
    }
//    obj->type->version = g_strdup(factoryContainer->file_version);
    obj->type->version = g_strdup_printf("%s@%s.%s",
                                         factoryContainer->project_number,
                                         factoryContainer->major_version,
                                         factoryContainer->minor_version);
    element_load(elem, obj_node);
    if(curLayer != factoryContainer->curLayer)
        curLayer = factoryContainer->curLayer;
//    if(!factoryContainer->fgdn_func)
//        factoryContainer->fgdn_func =(FactoryGetDownloadNameList)factory_get_download_name_list;
#ifdef STRUCT_MAINPOINT
    element_init(elem, 8, STRUCTCLASS_CONNECTIONPOINTS + 1);
#else
    element_init(elem, 8, STRUCTCLASS_CONNECTIONPOINTS);
#endif

    structclass->properties_dialog =  NULL;

    for (i=0; i<STRUCTCLASS_CONNECTIONPOINTS; i++)
    {
        obj->connections[i] = &structclass->connections[i];
        structclass->connections[i].object = obj;
        structclass->connections[i].connected = NULL;
    }

//    structclass->widgetmap = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
    structclass->widgetSave = NULL;
    fill_in_fontdata(structclass);

    /* kind of dirty, object_load_props() may leave us in an inconsistent state --hb */
    object_load_props(obj,obj_node);

    /* parameters loaded via StdProp dont belong here anymore. In case of strings they
     * will produce leaks. Otherwise the are just wasteing time (at runtime and while
     * reading the code). Except maybe for some compatibility stuff.
     * Although that *could* probably done via StdProp too.                      --hb
     */

    /* new since 0.94, don't wrap by default to keep old diagrams intact */
//  structclass->wrap_operations = FALSE;
    structclass->fill_color = color_white;
    structclass->vcolor = N_COLOR;

    attr_node  =   factory_find_custom_node(obj_node,STRUCT_NODE);
    if(!attr_node)
    {
        factory_critical_error_exit(factory_utf8(g_strdup_printf("找不到XML节点名:%s,文件内容错误.\n文件名:%s\t行数:%d",
                                    STRUCT_NODE,filename,obj_node->line)));
    }
    if(attr_node)
    {
        key = xmlGetProp(attr_node,(xmlChar *)"name");
        if (key)
        {
            structclass->element.object.name =  g_strdup((gchar*)key);
            xmlFree (key);
        }
        key = xmlGetProp(attr_node,(xmlChar *)"vname");
        if(key)
        {
            structclass->name = g_strdup((gchar*)key);
            xmlFree (key);
        }

        key = xmlGetProp(attr_node,(xmlChar *)"flag");
        if(key)
        {
            int n = g_strtod(key,NULL);
            if(!structclass->pps)
            {
                structclass->pps = g_new0(PublicSection,1);
                structclass->pps->name = g_strdup(structclass->name);
            }
            structclass->pps->hasfinished = n == 0 ? FALSE : TRUE;
            structclass->fill_color = structclass->pps->hasfinished ? color_edited : color_white;
            xmlFree (key);
        }


    }

    fill_in_fontdata(structclass);
//  structclass->stereotype_string = NULL;
    structclass_calculate_data(structclass);

    elem->extra_spacing.border_trans = structclass->line_width/2.0;
    structclass_update_data(structclass);

    for (i=0; i<8; i++)
    {
        obj->handles[i]->type = HANDLE_NON_MOVABLE;
    }

#ifdef DEBUG
    //structclass_sanity_check(structclass, "Loaded class");
#endif
    structclass->EnumsAndStructs = NULL;
    structclass->EnumsAndStructs = factoryContainer;
    structclass->isInitial = TRUE;
    /* 读取文件里面的值 */
//    factory_set_all_factoryclass(structclass);


    factory_debug_to_log(g_strdup_printf(factory_utf8("加载对像,名字:%s.\n"),structclass->name));

    if(factory_is_special_object(obj->name))
    {
        if(!curLayer->smd)
        {
            curLayer->smd = g_new0(SaveMusicDialog,1);
            SaveMusicDialog *smd = curLayer->smd;
            smd->title = factory_utf8("文件管理");
            /* 根据文件个数算偏移数 */
            gchar **split = g_strsplit(factoryContainer->system_files,",",-1);
            smd->offset = g_strv_length(split);
            g_strfreev(split);
            /* 这里用数字哈希表来保存,文件名的hash值 */
            smd->mbtree = g_tree_new(g_ascii_strcasecmp);
//            smd->mtable = g_hash_table_new(g_direct_hash,g_direct_equal);
//            smd->midtable = g_hash_table_new(g_direct_hash,g_direct_equal);
        }

        if(!curLayer->sid)
        {
            curLayer->sid = g_new0(SaveIdDialog,1);
        }

        factory_read_specific_object_from_file(structclass,
                                               attr_node,
                                               filename);
        structclass_destroy(structclass) ;
        return NULL;
    }
    else if(structclass->element.object.isTemplate)
    {
        /*模版读取*/
        factory_template_read_from_xml(structclass,attr_node,filename);
        return &structclass->element.object;
    }
    else
        factory_read_value_from_xml(structclass,attr_node->xmlChildrenNode);

    if(factory_is_system_data(obj->name))
        factoryContainer->sys_info->system_info = g_list_copy(structclass->widgetSave);
    else
    {
        gpointer ptr = g_hash_table_lookup(curLayer->defnames,structclass->name);
        if(ptr !=structclass )
            factory_rename_structclass(structclass);
        else
            g_hash_table_insert(curLayer->defnames,structclass->name,structclass);

        SaveStruct *sst = (SaveStruct *)structclass->widgetSave->data;
        obj->oindex = g_strtod(sst->value.vnumber,NULL);
        structclass->hasIdnumber = TRUE;
    }
    // curLayer->defnames = g_list_append(curLayer->defnames,structclass->name);
    return &structclass->element.object;
}




static void
factory_struct_items_save(STRUCTClass *structclass, ObjectNode obj_node,
                          const char *filename)
{
    g_return_if_fail(curLayer);
    g_return_if_fail(structclass);
    SaveMusicDialog *smd = curLayer->smd;
    g_return_if_fail(smd);
    element_save(&structclass->element, obj_node);
    /*  2014-3-22 lcy 这里保存自定义控件的数据 */
    /* 2014-3-25 lcy 这里添加一个自定义节点名来安置这个结构体的成员*/
    gchar *objname = structclass->element.object.name;

    ObjectNode item_node = xmlNewChild(obj_node, NULL,
                                       (const xmlChar *)STRUCT_NODE, NULL);
    xmlSetProp(item_node, (const xmlChar *)"name", (xmlChar *)objname);
    xmlSetProp(item_node, (const xmlChar *)"vname", (xmlChar *)structclass->name);

    FactoryStructItemList *fsi = g_hash_table_lookup(structclass->EnumsAndStructs->structTable,
                                 objname);
    if(structclass->pps)
        xmlSetProp(item_node, (const xmlChar *)"flag",
                   structclass->pps->hasfinished ? (xmlChar *)"1" : (xmlChar *)"0");
    else
        xmlSetProp(item_node, (const xmlChar *)"flag", (xmlChar *)"0");


    g_return_if_fail(fsi);
    if(fsi) /* 写入到指定文件名 */
        xmlSetProp(item_node, (const xmlChar *)"file",
                   (xmlChar *)fsi->sfile);
    //g_hash_table_foreach(structclass->widgetmap,factory_struct_save_to_xml,(gpointer)obj_node);
    if(factory_is_system_data(fsi->sname))
        structclass->widgetSave = g_list_copy(factoryContainer->sys_info->system_info);
    factory_debug_to_log(g_strdup_printf(factory_utf8("保存对像,名字:%s.\n"),structclass->name));
    if(!g_strcasecmp(fsi->sname,TYPE_FILELST))
    {
        /* 这里写音乐管理界面上的数据 */
        factory_mfile_save_to_xml(item_node,filename);
    }
    else if(!g_strcasecmp(fsi->sname,TYPE_IDLST))
    {
        SaveIdDialog *sid = (SaveIdDialog *)curLayer->sid;
        if(!sid || !sid->idlists)
            return;
        factory_idlist_save_to_xml(item_node,sid->idlists);
        /* 这里添加一个兼容以前的版本 */
    }
    else if(structclass->element.object.isTemplate &&
            ddisplay_active_diagram()->isTemplate)
    {
        /*保存模版*/
        factory_template_write_to_xml(structclass->widgetSave,item_node);
    }
    else
    {
        GList *saveList = structclass->widgetSave;

        for(; saveList; saveList = saveList->next)
        {
            factory_base_struct_save_to_file(saveList->data,item_node);
        }
    }


}


/** Returns the number of connection points used by the attributes and
 * connections in the current state of the object.
 */
static int
structclass_num_dynamic_connectionpoints(STRUCTClass *structclass)
{
    int num = 0;
//  if ( (structclass->visible_attributes) &&
//       (!structclass->suppress_attributes)) {
//    GList *list = structclass->attributes;
//    num += 2 * g_list_length(list);
//  }
//
//  if ( (structclass->visible_operations) &&
//       (!structclass->suppress_operations)) {
//    GList *list = structclass->operations;
//    num += 2 * g_list_length(list);
//  }
    return num;
}

void
structclass_sanity_check(STRUCTClass *c, gchar *msg)
{
#ifdef STRUCT_MAINPOINT
    int num_fixed_connections = STRUCTCLASS_CONNECTIONPOINTS + 1;
#else
    int num_fixed_connections = STRUCTCLASS_CONNECTIONPOINTS;
#endif
    DiaObject *obj = (DiaObject*)c;
//  GList *attrs;
    int i;

//  dia_object_sanity_check((DiaObject *)c, msg);

    /* Check that num_connections is correct */
    dia_assert_true(num_fixed_connections + structclass_num_dynamic_connectionpoints(c)
                    == obj->num_connections,
                    "%s: Class %p has %d connections, but %d fixed and %d dynamic\n",
                    msg, c, obj->num_connections, num_fixed_connections,
                    structclass_num_dynamic_connectionpoints(c));

    for (i = 0; i < STRUCTCLASS_CONNECTIONPOINTS; i++)
    {
        dia_assert_true(&c->connections[i] == obj->connections[i],
                        "%s: Class %p connection mismatch at %d: %p != %p\n",
                        msg, c, i, &c->connections[i], obj->connections[i]);
    }

#ifdef STRUCT_MAINPOINT
    dia_assert_true(&c->connections[i] ==
                    obj->connections[i + structclass_num_dynamic_connectionpoints(c)],
                    "%s: Class %p mainpoint mismatch: %p != %p (at %d)\n",
                    msg, c, i, &c->connections[i],
                    obj->connections[i + structclass_num_dynamic_connectionpoints(c)],
                    i + structclass_num_dynamic_connectionpoints(c));
#endif

    /* Check that attributes are set up right. */
    i = 0;
//  for (attrs = c->attributes; attrs != NULL; attrs = g_list_next(attrs)) {
//    STRUCTAttribute *attr = (STRUCTAttribute *)attrs->data;
//
//    dia_assert_true(attr->name != NULL,
//		    "%s: %p attr %d has null name\n",
//		    msg, c, i);
//    dia_assert_true(attr->type != NULL,
//		    "%s: %p attr %d has null type\n",
//		    msg, c, i);
//#if 0 /* attr->comment == NULL is fine everywhere else */
//    dia_assert_true(attr->comment != NULL,
//		    "%s: %p attr %d has null comment\n",
//		    msg, c, i);
//#endif
//
//    /* the following checks are only right with visible attributes */
//    if (c->visible_attributes && !c->suppress_attributes) {
//      int conn_offset = STRUCTCLASS_CONNECTIONPOINTS + 2 * i;
//
//      dia_assert_true(attr->left_connection != NULL,
//		      "%s: %p attr %d has null left connection\n",
//		      msg, c, i);
//      dia_assert_true(attr->right_connection != NULL,
//		      "%s: %p attr %d has null right connection\n",
//		      msg, c, i);
//
//      dia_assert_true(attr->left_connection == obj->connections[conn_offset],
//		      "%s: %p attr %d left conn %p doesn't match obj conn %d: %p\n",
//		      msg, c, i, attr->left_connection,
//		      conn_offset, obj->connections[conn_offset]);
//      dia_assert_true(attr->right_connection == obj->connections[conn_offset + 1],
//		      "%s: %p attr %d right conn %p doesn't match obj conn %d: %p\n",
//		      msg, c, i, attr->right_connection,
//		      conn_offset + 1, obj->connections[conn_offset + 1]);
//      i++;
//    }
//  }
    /* Check that operations are set up right. */
}


void factory_actionid_copy(const ActionID *onid,
                           ActionID *nnid)
{
    if(onid)
    {
        nnid->value = g_strdup(onid->value);
        nnid->title_name = g_strdup(onid->title_name);
        nnid->pre_quark = onid->pre_quark;
        nnid->conn_ptr = onid->conn_ptr;
    }
    else
    {
        nnid->value = g_strdup("-1");
        nnid->pre_quark = empty_quark;
        nnid->title_name = g_strdup("");
        nnid->conn_ptr = NULL;
    }

    nnid->line = NULL;

}

static gboolean
factory_tree_foreach_copy (gpointer key,gpointer value,
                           gpointer data)
{
    GTree *tree = data;
    g_tree_insert(tree,g_strdup(key),value);
    return FALSE;
}


/* 这里就是一个很复杂的完全copy */
SaveStruct *factory_savestruct_copy(const SaveStruct *old)
{
    g_return_if_fail(old);
    SaveStruct *newsst = g_new0(SaveStruct,1);
    newsst->name = g_strdup(old->name);
    newsst->widget1 = NULL;
    newsst->widget2 = NULL;
    newsst->type = g_strdup(old->type);
    newsst->celltype = old->celltype;
    newsst->isPointer = old->isPointer;
    newsst->templ_pos = old->templ_pos;
    newsst->templ_quark = old->templ_quark;

//    int t = offsetof(SaveStruct,value);
//    memcpy(((char*)newsst)+t,((char*)old)+t,
//           sizeof(_value));

    switch(old->celltype)
    {
    case ECOMBO:
    {
        SaveEnum *osen = &old->value.senum;
        SaveEnum *nsen = &newsst->value.senum;
        nsen->evalue = g_strdup(osen->evalue);
        nsen->enumList = g_list_copy(osen->enumList);
        nsen->width = g_strdup(osen->width);
        nsen->index = osen->index;
    }
    break;
    case UCOMBO:
    {
        SaveUnion *osuptr = &old->value.sunion;
        SaveUnion *nsuptr = &newsst->value.sunion;
        nsuptr->pre_quark = empty_quark;
        nsuptr->ubtreeVal = g_tree_new(factory_str_compare);
//        g_tree_foreach(osuptr->ubtreeVal,factory_tree_foreach_copy,
//                       nsuptr->ubtreeVal);

        nsuptr->comobox = NULL;
        nsuptr->vbox = NULL;
        nsuptr->curkey = g_strdup(osuptr->curkey);

        nsuptr->structlist = g_list_copy(osuptr->structlist);
        nsuptr->uindex = osuptr->uindex;
        if(g_tree_nnodes(osuptr->ubtreeVal))
        {
            SaveStruct *usst = g_tree_lookup(osuptr->ubtreeVal,
                                             osuptr->curkey);
            /*这里只复制当前一个值*/
            if(!usst) break;
            g_tree_insert(nsuptr->ubtreeVal,g_strdup(nsuptr->curkey),
                          factory_savestruct_copy(usst));
        }

    }
    break; /* union comobox */
    case UBTN:
    {
        /* 这个类型有可能是文件列表,或者ID列表 */
        if(factory_is_special_object(old->type))
        {
            if(factory_music_fm_item_is_index(old->name) ||
                    !g_ascii_strcasecmp(old->org->SType,TYPE_IDLST))
            {
                SaveSel *ossl = old->value.vnumber;
                SaveSel *nssl = g_new0(SaveSel,1);
                newsst->value.vnumber = nssl;
                nssl->ntable = ossl->ntable;
                nssl->offset_val = ossl->offset_val;
            }
            else
                newsst->value.vnumber = g_strdup(old->value.vnumber);
            break;
        }

        SaveUbtn *sbtn = &old->value.ssubtn;
        GList *sslist = sbtn->savelist;
        GList *nslist = NULL;
        for(; sslist; sslist = sslist->next)
        {
            SaveStruct *osst = sslist->data;
            if(!osst) continue;
            nslist = g_list_append(nslist,
                                   factory_savestruct_copy(osst));
        }
        SaveUbtn *nsbtn = &newsst->value.ssubtn;
        nsbtn->savelist = nslist;
    }
    break;/* 这里是按键按钮 */
    case OBTN:
    {
        ActIDArr *nnid = &newsst->value.nextid;
        ActIDArr *onid = &old->value.nextid;
        nnid->arr_base = onid->arr_base;
//        nnid->actlist = g_list_copy(onid->actlist);
        nnid->wlist = NULL;
        GList *olist = onid->actlist;
        g_list_free1(nnid->actlist);
        nnid->actlist = NULL;
        for(; olist; olist = olist->next)
        {
            ActionID *aid = g_new0(ActionID,1);
            factory_actionid_copy(olist->data,aid);
            nnid->actlist = g_list_append(nnid->actlist,aid);
        }
    }
    break;
    case OCOMBO:
    {
        ActionID *nnid = &newsst->value.actid;
        ActionID *onid = &old->value.actid;
        factory_actionid_copy(onid,nnid);
    }
    break;/* object combox*/
    case BBTN:
    case ENTRY:
    {
        SaveEntry *osey = &old->value.sentry;
        SaveEntry *nsey = &newsst->value.sentry;
        nsey->arr_base = osey->arr_base;
        nsey->isString = osey->isString;
        if(osey->isString)
        {
            nsey->data = g_strdup(osey->data);
        }
        else
        {
            nsey->data = g_list_copy(osey->data);
        }
        nsey->wlist = g_list_copy(osey->wlist);
    }
    break; /* 文本 */
    case SPINBOX:
    {
        newsst->value.vnumber = g_strdup(old->value.vnumber);
    }
    break;
//    case BBTN:
//    {
//        SaveEntry *osey = &old->value.sentry;
//        SaveEntry *nsey = &newsst->value.sentry;
//        nsey->arr_base = osey->arr_base;
//        nsey->isString = osey->isString
//        nsey->data = g_list_copy(osey->data);
//
//        nsey->wlist = g_list_copy(osey->wlist);
//    }
//    break;
    case EBTN:
    {
        SaveEbtn *osebtn = &old->value.ssebtn;
        SaveEbtn *nsebtn = &newsst->value.ssebtn;
        nsebtn->arr_base = osebtn->arr_base;
        nsebtn->width = g_strdup(osebtn->width);
        nsebtn->ebtnwlist = NULL;
        nsebtn->ebtnslist = g_list_copy(osebtn->ebtnslist);
    }
    break;/* 枚举也有数组 */
    case LBTN:
    {
        ListBtn *oltb = &old->value.slbtn;
        ListBtn *nltb = &newsst->value.slbtn;
        nltb->arr_base = oltb->arr_base;
        nltb->vlist = g_list_copy(oltb->vlist);
    }
    break; /* 文件管理与ID管理 数组形式的按键 */
    default:
        break;
    }
    newsst->org = old->org;
    newsst->close_func = old->close_func;
    newsst->newdlg_func = old->newdlg_func;
    return newsst;
}


GtkTreeModel *factory_create_combox_model(GList *itemlist)
{
    GtkListStore *model = gtk_list_store_new (1, G_TYPE_STRING);
    GList *tlist = itemlist;
    for(; tlist; tlist = tlist->next)
    {
        factory_append_iten_to_cbmodal(model,tlist->data);
    }
    return GTK_TREE_MODEL(model);
}


gboolean factory_comobox_compre_foreach(GtkTreeModel *model,
                                        GtkTreePath *path,
                                        GtkTreeIter *iter,
                                        gpointer data)
{
    ComboxCmp *cc = (ComboxCmp*)data;
    gchar *curstr;
    gtk_tree_model_get(model,iter,0,&curstr,-1);
    GQuark quark = g_quark_from_string(curstr);
    g_free(curstr);
    if(cc->qindex == quark)
    {
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(cc->combox),iter);
        return TRUE;
    }
    return FALSE;
}

void factory_accelerator_to_response(gpointer user_data,gpointer otherdata)
{
    gtk_dialog_response(GTK_DIALOG(user_data),GTK_RESPONSE_OK);
}
