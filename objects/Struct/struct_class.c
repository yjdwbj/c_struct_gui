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

static MusicFileManagerOpts  mfmo_opts =
{
    (OpenDialog) factory_file_manager_dialog,
    (ApplyDialog) factory_music_file_manager_apply,
    (Item_Added) factory_music_file_manager_new_item_changed,
    (Clear_All) factory_music_file_manager_remove_all
};



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
gpointer *factory_read_object_comobox_value_from_file(AttributeNode attr_node);
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
void factory_delete_line_between_two_objects(STRUCTClass *startc,const gchar *endc);


//static ObjectChange *_structclass_apply_props_from_dialog(STRUCTClass *structclass, GtkWidget *widget);
ObjectChange *factory_apply_props_from_dialog(STRUCTClass *structclass, GtkWidget *widget);
void factory_delete_line_between_two_objects(STRUCTClass *startc,const gchar *endc);

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
    (UpdateObjectIndex)   factory_update_index
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

    DiaObject *obj = &fclass->element.object;
    Layer *curlay = obj->parent_layer;
    GList *list = curlay->objects;
    obj->oindex  = g_list_index(list,obj);


//    gchar **tmp =  g_strsplit(fclass->name,"(",-1);
//    gchar *newname = g_strconcat(tmp[0],g_strdup_printf(_("(%03d)"), n),NULL);
//    g_strfreev(tmp);
//    g_free(fclass->name);
//    fclass->name =  g_strdup(newname);
//    g_free(newname);
//    obj->name = fclass->name;
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

static DiaObject *  // 2014-3-19 lcy 这里初始化结构
factory_struct_items_create(Point *startpoint,
                            void *user_data,
                            Handle **handle1,
                            Handle **handle2)
{
    DDisplay *ddisp = ddisplay_active();
    if(ddisp)
        curLayer = ddisp->diagram->data->active_layer;
    STRUCTClass *structclass;
//  STRUCTClassDialog *properties_dialog;
    Element *elem;
    DiaObject *obj;
    int i;

    structclass = g_malloc0(sizeof(STRUCTClass));
    structclass->isInitial = FALSE;

    elem = &structclass->element;
    obj = &elem->object;

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
    for(; sstruct !=NULL; sstruct = sstruct->next)
    {
        FactoryStructItemList *i = sstruct->data;
        if(i->number == index)
        {
            structclass->name = g_strdup(i->vname) ;
            obj->name = g_strdup(_(i->sname));
            break;
        }
    }

    if(curLayer && !factory_is_special_object(obj->name))
    {
        GList *list = curLayer->defnames;
        int num = 0;
        for(; list; list = list->next)
        {
            gchar *str = list->data; /* 同名的加序号 */
            if(!g_ascii_strncasecmp(str,structclass->name,strlen(structclass->name)))
                num++;
        }


        if(num>0)
        {
            gchar *tstr = g_strdup(structclass->name);
            g_free(structclass->name);
            structclass->name =g_strconcat(tstr,g_strdup_printf("(%d)",num),NULL);
            g_free(tstr);
        }

        if(!factory_is_system_data(obj->name))
            curLayer->defnames = g_list_append(curLayer->defnames,structclass->name);


    }





    /* 2014-3-26 lcy  这里初始哈希表用存widget与它的值*/
// structclass->widgetmap = g_hash_table_new(g_direct_hash,g_direct_equal);
    structclass->widgetmap = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);

    obj->type = &structclass_type;
    obj->type->version  = g_strdup(factoryContainer->file_version);
    obj->ops = &structclass_ops;

    structclass->line_width = attributes_get_default_linewidth();
    structclass->text_color = color_black;
    structclass->line_color = attributes_get_foreground();
    structclass->fill_color = attributes_get_background();

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
    structclass_update_data(structclass);

    for (i=0; i<8; i++)
    {
        obj->handles[i]->type = HANDLE_NON_MOVABLE;
    }

    *handle1 = NULL;
    *handle2 = NULL;
    structclass->EnumsAndStructs = NULL;
    structclass->EnumsAndStructs = factoryContainer;

    return &structclass->element.object;
}

//AttributeNode *factory_get_xml_next_node(AttributeNode  obj_node,const gchar *comprename)
//{
//     AttributeNode attr_node = obj_node;
//     while(attr_node = data_next(attr_node))
//        {
//            xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"name");
//            if(key)
//            {
//                if(0 == g_ascii_strncasecmp((gchar*)key,comprename,strlen((gchar*)key)))
//                {
//                    sss->name = g_strdup(comprename);
//                    break;
//                }
//            }
//            xmlFree(key);
//
//        }
//        return attr_node;
//}
gchar *factory_get_last_section(const gchar *src,const gchar *delimiter)
{
    gchar **split = g_strsplit(src,".",-1);
    int section = g_strv_length(split);
    gchar *ret = g_strdup(split[section-1]);
    g_strfreev(split);
    return ret;
}
FactoryStructItem *factory_get_factorystructitem(GList *inlist,const gchar *name)
{
    FactoryStructItem *fst = NULL;
    GList *p =inlist;
    for(; p; p=p->next)
    {
        FactoryStructItem *t = p->data;
        if(!g_ascii_strcasecmp(t->Name,name))
        {
            fst = p->data;
            break;
        }
    }
    return fst;
}

void factory_read_union_button_from_file(STRUCTClass *fclass,ObjectNode obtn_node,SaveUbtn *sbtn)
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
        if(nnode)
        {
            nnode->widget1 = NULL;
            nnode->widget2 = NULL;
            nnode->org = sitem;
            nnode->sclass = sitem->orgclass;
            nnode->name = g_strdup((gchar*)key);
            xmlFree(key);
            key = xmlGetProp(obtn_node,(xmlChar *)"type");
            if(key);
            nnode->type = g_strdup((gchar*)key);
            xmlFree(key);
            factory_read_object_value_from_file(nnode,sitem,obtn_node);
            sbtn->savelist = g_list_append(sbtn->savelist,nnode);
        }
    }

}
void  factory_read_object_value_from_file(SaveStruct *sss,FactoryStructItem *fst,ObjectNode attr_node)
{

    STRUCTClass *fclass = fst->orgclass;
    xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"type");
    if(factory_music_fm_get_type(sss->name))
    {
        sss->celltype = UBTN;
        key = xmlGetProp(attr_node,(xmlChar *)"wtype");
        sss->type = g_strdup((gchar*)key);
        xmlFree(key);
        key = xmlGetProp(attr_node,(xmlChar *)"value");
        SaveKV *skv = sss->value.vnumber;

//        sss->value.vnumber = g_strdup((gchar*)key);
        skv->value = g_strdup((gchar*)key);
        key = xmlGetProp(attr_node,(xmlChar*)"index");
        xmlFree(key);
        skv->radindex = g_strtod((gchar*)key,NULL);
        xmlFree(key);
        return;
    }


    if (key)
    {
        sss->type  =  g_locale_to_utf8((gchar*)key,-1,NULL,NULL,NULL);/* 找到数据类型 */
        xmlFree (key);
    }
    key = xmlGetProp(attr_node,(xmlChar *)"wtype"); /* 显示控件类型 */
    if(!key) return NULL;

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
            if((gchar*)key)
                for(; t != NULL ; t = t->next)
                {
                    FactoryStructEnum *fse = t->data;
                    if(0 == g_ascii_strcasecmp(fse->value,(gchar*)key))
                    {
                        sen->evalue = g_locale_to_utf8((gchar*)key,-1,NULL,NULL,NULL);
                        sen->index = g_list_index(targettable,fse);
                        break;
                    }
                }

            xmlFree(key);
            key = xmlGetProp(attr_node,(xmlChar *)"width");
            if(key)
                sen->width = g_locale_to_utf8((gchar*)key,-1,NULL,NULL,NULL);
            xmlFree(key);
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
        NextID *nid = &sss->value.nextid;

        ActionID *aid = factory_read_object_comobox_value_from_file(attr_node);
        if(g_list_length(nid->actlist) == 1)
        {
            ActionID *exists = nid->actlist->data;
            *exists = *aid;
        }
        else
            nid->actlist = g_list_append(nid->actlist,aid);
    }
    else if(!g_ascii_strncasecmp((gchar*)key,"OBTN",4))
    {
        sss->celltype = OBTN;
        sss->isPointer = TRUE;
        sss->newdlg_func = factory_create_objectbutton_dialog;
        sss->close_func = factory_save_objectbutton_dialog;
        NextID *nid = &sss->value.nextid;
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
            if ( obtn_node  && (strcmp((char *) obtn_node->name, "JL_item")==0) )
            {
                key = xmlGetProp(obtn_node,(xmlChar *)"name");
                if(key)
                {
                    ActionID *aid = factory_read_object_comobox_value_from_file(obtn_node);
                    nid->actlist = g_list_append(nid->actlist,aid);
                }
                xmlFree(key);
            }
            obtn_node = obtn_node->next;
        }

    }
    else if(!g_ascii_strncasecmp((gchar*)key,"UBTN",6))
    {
        sss->celltype = UCOMBO;
        SaveUnion *suptr = &sss->value.sunion;
        key = xmlGetProp(attr_node,(xmlChar *)"index");
        if(key)
            suptr->index  = atoi((gchar*)key);
        xmlFree(key);
        key = xmlGetProp(attr_node,(xmlChar *)"type");
        if(key)
            sss->type = g_strdup((gchar*)key);
        xmlFree(key);
        GList *slist;
        if(key)
        {
            /* 通过类型名 找到对应的联合体 */
            slist =  g_hash_table_lookup(fclass->EnumsAndStructs->unionTable,(gchar*)sss->type);
        }

        g_hash_table_insert(suptr->saveVal,suptr->curkey,sss);

        /* 读它下面的子节点 */
        AttributeNode obtn_node  =  attr_node->xmlChildrenNode;
        while (obtn_node != NULL)
        {
            if (xmlIsBlankNode(obtn_node))
            {
                obtn_node = obtn_node->next;
                continue;
            }
            if ( obtn_node  && (strcmp((char *) obtn_node->name, "JL_item")==0) )
            {
                break;
            }
            obtn_node = obtn_node->next;
        }
        FactoryStructItem *nextobj =  g_list_nth_data(slist,suptr->index);
        suptr->structlist = slist;

        if(!nextobj)
            return ; /* 没有找到控件原型,可能出错了. */
        suptr->curkey = g_strjoin("##",nextobj->FType,nextobj->Name,NULL);
        suptr->curtext = g_strdup(nextobj->Name);
        SaveStruct *nsitm = factory_get_savestruct(nextobj);/* 紧跟它下面的控件名 */
        if(nsitm)
        {
            nsitm->sclass = sss->sclass;
            SaveUbtn *sbtn = &nsitm->value.ssubtn;
            factory_strjoin(&nsitm->name,sss->name,".");
            /* 把当前选择的成员初始化保存到哈希表 */
//                sbtn->htoflist = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
            sbtn->structlist = nextobj->datalist;
            sbtn->savelist = NULL;
            factory_read_union_button_from_file(sss->sclass,obtn_node->prev,sbtn);
            g_hash_table_insert(suptr->saveVal,suptr->curkey,nsitm);
        }

    }
//    else if(!g_ascii_strncasecmp((gchar*)key,"UBTN",6))
//    {
//        sss->celltype = UCOMBO; /* 这里的UBTN -> UCOMBO　没有搞错，上面还有对应相反的 */
//        sss->isPointer = TRUE;
//        SaveUbtn *sbtn = &sss->value.ssubtn;
//        sbtn->structlist = fst->datalist;
//        sbtn->savelist = NULL;
//        sss->newdlg_func = factory_create_unionbutton_dialog;
//        sss->close_func = factory_save_unionbutton_dialog;
////            sbtn->htoflist = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
//    }
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
                GList *tlist  = sebtn->ebtnslist;
                for(; tlist; tlist = tlist->next)
                {
                    FactoryStructEnum *fse = tlist->data;
                    if(!g_ascii_strcasecmp(fse->value,se->evalue))
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

GList *factory_get_list_from_hashtable(STRUCTClass *fclass)
{
    FactoryStructItemList *fsil = g_hash_table_lookup(fclass->EnumsAndStructs->structTable,
                                  fclass->element.object.name);
    GList *tlist = NULL;
    if(fsil)
        tlist =  fsil->list;
    return tlist;
}

void factory_read_specific_object_from_file(STRUCTClass *fclass,ObjectNode obj_node)
{

    gchar *objname = fclass->element.object.name;
    if(factory_music_fm_get_type(objname))
    {
        SaveMusicDialog *smd = MusicManagerDialog;
        AttributeNode attr_node = obj_node;
        while(attr_node = data_next(attr_node))
        {
            xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"name");
            if(key)
            {
                if(0 == g_ascii_strcasecmp((gchar*)key,"name"))
                {
                    /* 这里读取左边布局的数据 */
                    SaveMusicItem *smi = g_new0(SaveMusicItem,1);
                    smi->id_index = g_strdup((gchar*)key);

                    key = xmlGetProp(attr_node,(xmlChar *)"value");
                    smi->active = g_strdup((gchar*)key);

                    key = xmlGetProp(attr_node,(xmlChar *)"addr");
                    smi->id_addr = g_strdup((gchar*)key);
//                    sss->name = g_strdup(fst->Name);
                    smd->itemlist = g_list_append(smd->itemlist,smi);


                }
                else if(!g_ascii_strcasecmp((gchar*)key,"Music_File"))
                {
                    factory_read_file_list_from_xml(attr_node->xmlChildrenNode);
                }
            }
            xmlFree(key);

        }
        if(!attr_node)
            return;

    }
    else
    {
        SaveIdDialog *sid = IdDialog;
        AttributeNode attr_node = obj_node;
        while(attr_node = data_next(attr_node))
        {
            xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"name");
            if(!key) continue;

            if(!g_ascii_strcasecmp((gchar*)key,"name"))
            {
                SaveMusicItem *smi = g_new0(SaveMusicItem,1);
                smi->id_index = g_strdup((gchar*)key);
                key = xmlGetProp(attr_node,(xmlChar *)"addr");
                smi->id_addr = g_strdup((gchar*)key);
                key = xmlGetProp(attr_node,(xmlChar *)"dname");
                smi->dname = g_strdup((gchar*)key);
                sid->idlists = g_list_append(sid->idlists,smi);
            }
        }

    }
}


void factory_read_file_list_from_xml(ObjectNode obj_node)
{
    /*　这里读文件列表　*/
    SaveMusicFileMan *smfm = MusicManagerDialog->smfm;
    AttributeNode attr_node = obj_node;
    while(attr_node = data_next(attr_node))
    {
        xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"name");
        if(!key) continue;

        if(0 == g_ascii_strcasecmp((gchar*)key,"name"))
        {
            SaveMusicFile *smf = g_new0(SaveMusicFile,1);
            smf->index = g_strdup((gchar*)key);

            key = xmlGetProp(attr_node,(xmlChar *)"fname");
            smf->full_name = g_strdup((gchar*)key);

            key = xmlGetProp(attr_node,(xmlChar *)"addr");
            smf->file_addr = g_strdup((gchar*)key);

            key = xmlGetProp(attr_node,(xmlChar *)"dname");
            smf->down_name = g_strdup((gchar*)key);
//                    sss->name = g_strdup(fst->Name);

            gchar **split = g_strsplit(smf->full_name,"\\",-1);
            int len = g_strv_length(split);
            smf->base_name = g_strdup(split[len-1]);
            g_strfreev(split);
            smfm->filelist = g_list_append(smfm->filelist,smf);
        }


        xmlFree(key);

    }
    if(!attr_node)
        return;
}


void factory_read_value_from_xml(STRUCTClass *fclass,ObjectNode obj_node)
{
//    gchar **tmp =  g_strsplit(fclass->name,"(",-1);
//    GList *tlist = g_hash_table_lookup(fclass->EnumsAndStructs->structTable,tmp[0]);
//    GList *tttt = tlist;
//    g_strfreev(tmp);

    GList *tlist = factory_get_list_from_hashtable(fclass);
    GList *tttt = tlist;

    for(; tttt != NULL ; tttt = tttt->next)
    {
        FactoryStructItem *fst = tttt->data;
        fst->orgclass = fclass;
        SaveStruct *sss = factory_get_savestruct(fst);
        sss->widget1 = NULL;
        sss->widget2 = NULL;
        sss->org = fst;
        sss->sclass = fclass;
        AttributeNode attr_node = obj_node;
        while(attr_node = data_next(attr_node))
        {
            xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"name");
            if(key)
            {
                if(0 == g_ascii_strcasecmp((gchar*)key,fst->Name))
                {
                    sss->name = g_strdup(fst->Name);
                    break;
                }
            }
            xmlFree(key);

        }
        if(!attr_node)
            continue;

        gchar *hkey =  g_strjoin("##",fst->FType,fst->Name,NULL);
        factory_read_object_value_from_file(sss,fst,attr_node);
        SaveStruct *firstval =  g_hash_table_lookup(fclass->widgetmap,hkey);
        if(firstval)
            *firstval = *sss;
        else
        {
            g_hash_table_insert(fclass->widgetmap,g_strdup(hkey),sss);
            fclass->widgetSave = g_list_append(fclass->widgetSave,sss);
        }

        g_free(hkey);
    }

}

gpointer *factory_read_object_comobox_value_from_file(AttributeNode attr_node)
{
    xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"index");
    ActionID *aid = g_new0(ActionID,1);
    aid->index = atoi((gchar*)key);
    xmlFree(key);
    key  =  xmlGetProp(attr_node,(xmlChar *)"nvalue");
    aid->pre_name = g_strdup((gchar*)key);
    xmlFree(key);
    key  =  xmlGetProp(attr_node,(xmlChar *)"name");
    aid->title_name = g_strdup((gchar*)key);
    key  =  xmlGetProp(attr_node,(xmlChar *)"value");
    aid->value = g_strdup((gchar*)key);
    xmlFree(key);
    return aid;
}

void factory_inital_ebtn(SaveStruct *sss,const FactoryStructItem *fst)
{
    SaveEbtn *sebtn = &sss->value.ssebtn;
    sebtn->ebtnslist = g_hash_table_lookup(factoryContainer->enumTable,fst->SType);
    sebtn->ebtnwlist = NULL;
    sebtn->width = g_strdup(fst->Max);
    sss->close_func = factory_save_enumbutton_dialog;
    sss->newdlg_func = factory_create_enumbutton_dialog;
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
    for(; n < sebtn->arr_base->reallen; n++)
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

SaveStruct * factory_get_savestruct(FactoryStructItem *fst)
{
    SaveStruct *sss = g_new0(SaveStruct,1);
    sss->widget1= NULL;
    sss->widget2 = NULL;

    sss->type = g_strdup(fst->FType);
    sss->name = g_strdup(fst->Name);
    sss->sclass = fst->orgclass;
    sss->close_func = NULL;
    sss->newdlg_func = NULL;

    if(!g_ascii_strcasecmp(fst->Min,factory_utf8(_("固定值"))))
    {
        sss->isSensitive = FALSE;
    }
    else
        sss->isSensitive = TRUE;

    /* 这里添加两个特别的判断 */



    /* 2014-3-26 lcy 通过名字去哈希表里找链表*/
    if(!g_ascii_strncasecmp(sss->name,ACTION_ID,ACT_SIZE))
    {
        sss->celltype = OCOMBO;
        NextID *nid = &sss->value.nextid;
        nid->itemlist = NULL;
        nid->actlist = NULL;
        nid->wlist = NULL;
        if(g_str_has_suffix(sss->name,"]"))
        {
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
            for(; n < nid->arr_base->reallen; n++ )
            {
                ActionID *aid = g_new0(ActionID,1);
                aid->index = 0;
                aid->pre_name = g_strdup("");
                aid->value = g_strdup("-1");
                aid->title_name = g_strdup_printf(name,n);
                nid->actlist = g_list_append(nid->actlist,aid);
            }
            g_free(name);
        }
        else
        {
            ActionID *aid = g_new0(ActionID,1);
            aid->index = 0;
            aid->pre_name = g_strdup("");
            aid->value = g_strdup("-1");
            aid->title_name = g_strdup(sss->name);
            nid->actlist = g_list_append(nid->actlist,aid);
        }

    }
    else
        switch(fst->Itype)
        {
        case BT:
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
                        int maxi = sey->arr_base->reallen;
                        for( ; n < maxi; n++)
                        {
                            /* 初如值 */
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
                        int maxi = sey->arr_base->reallen;
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
                if(!g_ascii_strcasecmp(fst->Name,"wActID")) /*这里一个关键字判断是否是ＩＤ*/
                {
                    STRUCTClass *sclass = fst->orgclass;

                    sss->value.vnumber = g_strdup_printf("%d",sclass->element.object.oindex);
                    sss->isSensitive = FALSE;
                }

                else
                    sss->value.vnumber = g_strdup(fst->Value);
            }
            break;
        case ET:
        {
            if( factory_find_array_flag(fst->Name))
            {
                sss->celltype = EBTN;
                factory_inital_ebtn(sss,fst);
            }
            else
            {
                sss->celltype = ECOMBO;
                sss->value.senum.enumList = fst->datalist;
//                sss->value.senum.width = g_strdup(fst->Max);
                GList *t = sss->value.senum.enumList;
                for(; t != NULL ; t = t->next)
                {
                    FactoryStructEnum *kvmap = t->data;
                    if(!g_ascii_strcasecmp(fst->Value,kvmap->key))
                    {
                        sss->value.senum.index = g_list_index(fst->datalist,kvmap);
                        sss->value.senum.width = g_strdup(fst->Max);
                        sss->value.senum.evalue = g_strdup(kvmap->value);
                        break;
                    }

                }
                if(!t && !sss->value.senum.width ) /*源文件有错误，这里用默认值*/
                {
                    sss->value.senum.index = 0;
                    sss->value.senum.width = g_strdup(fst->Max);
                    FactoryStructEnum *kvmap = sss->value.senum.enumList->data;
                    sss->value.senum.evalue = g_strdup(kvmap->value);
                }

            }


        }
        break;
        case ST:
        {
            sss->celltype = UBTN;
            sss->isPointer = TRUE; /* 这里是一个按键*/

            if(!g_strcasecmp(fst->SType,"FILELST")) /*　 特殊的控件,音乐文件管理　*/
            {

                if(!MusicManagerDialog)
                {
                    MusicManagerDialog = g_new0(SaveMusicDialog,1);
                    MusicManagerDialog->title = factory_utf8("文件管理");
                    MusicManagerDialog->smfm = NULL;
                    MusicManagerDialog->mfmos = &mfmo_opts;
//                    MusicManagerDialog->radindex = -1;
                }
                /* sss->value.vnumber  原定义为一个gchar 指针，在这里当做gpointer 用了*/
                SaveKV *skv = g_new0(SaveKV,1);
                MusicManagerDialog->skv = skv;
                skv->value = g_strdup(fst->Value);
                skv->radindex = -1;
                sss->value.vnumber = skv;
                MusicManagerDialog->btnname = g_strdup(fst->Cname);
                sss->newdlg_func = factory_file_manager_dialog;
                sss->close_func = factory_music_file_manager_apply;
            }
            else if(!g_strcasecmp(fst->SType,"IDLST"))
            {

                if(!IdDialog)
                {
                    IdDialog = g_new0(SaveIdDialog,1);
                }
                SaveKV *skv = g_new0(SaveKV,1);
                IdDialog->skv = skv;
                skv->value = g_strdup(fst->Value);
                skv->radindex = -1;
                sss->value.vnumber = skv;
                sss->newdlg_func = factory_new_idlist_dialog;
                sss->close_func = factory_save_idlist_dialog;
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
            suptr->saveVal = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
            suptr->curkey = g_strdup("");
            suptr->index = 0;
            suptr->comobox = NULL;
            suptr->structlist = fst->datalist;
            GList *p = suptr->structlist;
            /* nextobj 就是当前下拉框所显示的 */
            FactoryStructItem *nextobj =  g_list_nth_data(suptr->structlist,suptr->index);
            if(!nextobj)
                break;
            suptr->curkey = g_strjoin("##",nextobj->FType,nextobj->Name,NULL);
            SaveStruct *tsst = factory_get_savestruct(nextobj);
            if(!tsst)
                break;
            tsst->sclass = sss->sclass;
            factory_strjoin(&tsst->name,sss->name,".");
            /* 把当前选择的成员初始化保存到哈希表 */
            g_hash_table_insert(suptr->saveVal,suptr->curkey,tsst);
            if(tsst->isPointer )
            {
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
        }
        break;
        default:
            break;
        }
    sss->org = fst;
    return sss;
}

//void factory_initial_origin_struct(GHashTable *savetable,GList *srclist,const gchar *key)
//{
//     /* srclist is FactoryStructItem GList */
//    GHashTable *structtable = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
//    if(srclist)
//    for(;srclist != NULL ; srclist = srclist->next)
//    {
//        FactoryStructItem *fst = srclist->data;
//        SaveStruct *sst= factory_get_savestruct(fst);
//        g_hash_table_insert(structtable,g_strjoin("##",fst->FType,fst->Name,NULL),sst);
//    }
//    g_hash_table_insert(savetable,key,structtable);
//}


void factory_set_all_factoryclass(STRUCTClass *fclass,gchar *name)
{
//    gchar **tmp =  g_strsplit(fclass->name,"(",-1);
    GList *tlist = factory_get_list_from_hashtable(fclass);

//    g_strfreev(tmp);
    for(; tlist; tlist = tlist->next)
    {
        FactoryStructItem *fst = tlist->data;
        fst->orgclass = fclass;
    }
}

void factory_read_initial_to_struct(STRUCTClass *fclass) /*2014-3-26 lcy 拖入控件时取得它的值*/
{
    /* 这里从原哈希表复制一份出来 */
//    gchar **tmp =  g_strsplit(fclass->name,"(",-1);
    GList *tttt = factory_get_list_from_hashtable(fclass);
    GList * tlist = tttt ;
    for(; tlist; tlist = tlist->next)
    {
        FactoryStructItem *fst = tlist->data;
        fst->orgclass = fclass;
    }

    for(; tttt != NULL ; tttt = tttt->next)
    {
        FactoryStructItem *fst = tttt->data;
        SaveStruct *sst= factory_get_savestruct(fst);
        g_hash_table_insert(fclass->widgetmap,g_strjoin("##",fst->FType,fst->Name,NULL),sst);
        fclass->widgetSave = g_list_append(fclass->widgetSave,sst);
    }
//    factory_initial_origin_struct(fclass->widgetmap,tttt,fclass->name);
//    int s = g_list_length(tlist);
//    if(tlist)
//    for(;tlist != NULL ; tlist = tlist->next)
//    {
//        FactoryStructItem *fst = tlist->data;
//        SaveStruct *sst= factory_get_savestruct(fst);
//        g_hash_table_insert(fclass->widgetmap,g_strjoin("##",fst->FType,fst->Name,NULL),sst);
//    }
}



void
factory_handle_entry_item(SaveEntry* sey,FactoryStructItem *fst)
{
    /* 2014-3-25 lcy 这里是字符串，需用文本框显示了*/
    /* lcy  array[3][2]   ==>  ooo[0]="array", ooo[1]="3", ooo[2]="", ooo[3]="2" ,ooo[4]="" */

    ArrayBaseProp *abp  = g_new0(ArrayBaseProp,1);
    sey->arr_base = abp;
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
    if(structclass->widgetmap && g_hash_table_size(structclass->widgetmap))
    {
        g_hash_table_destroy(structclass->widgetmap);
    }

    if (structclass->properties_dialog != NULL)
    {
        structclass_dialog_free (structclass->properties_dialog);
    }

    g_list_free1(structclass->widgetSave);


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

#ifdef DEBUG
    structclass_sanity_check(newstructclass, "Copied");
#endif
    newstructclass->EnumsAndStructs = factoryContainer;
    newstructclass->widgetmap = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
    g_hash_table_foreach(structclass->widgetmap,factory_hashtable_copy,newstructclass->widgetmap);
    newstructclass->properties_dialog = NULL;

    return &newstructclass->element.object;
}



static void factory_base_item_save(SaveStruct *sss,ObjectNode ccc)
{
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
        xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"SPINBOX");
        xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)sss->value.vnumber);
        break;
    }
    /* 这里要不是加一个OCOMBO 控件识别呢? */
}

static void factory_write_object_comobox(ActionID *aid,ObjectNode ccc )
{
    xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)aid->title_name);
//                    xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"OCOMBO");
    xmlSetProp(ccc, (const xmlChar *)"index", (xmlChar *)g_strdup_printf(_("%d"),aid->index));


    xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)aid->value);


    xmlSetProp(ccc, (const xmlChar *)"nvalue", (xmlChar *)aid->pre_name);
}

static void factory_base_struct_save_to_file(SaveStruct *sss,ObjectNode obj_node)
{
    switch(sss->celltype)
    {
    case ECOMBO:
    case ENTRY:
    case BBTN:
    case EBTN:
    case SPINBOX:
//    case SBTN:
    {
        ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_item", NULL);
        factory_base_item_save(sss,ccc);
    }
    break;
    case OCOMBO:
    {
        /*每一项用JL_item 做节点名，存放多个属性*/
        ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_item", NULL);
        xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)sss->type);
        xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"OCOMBO");
        NextID *nid  = &sss->value.nextid;
        GList *tlist = nid->actlist;
        ActionID *aid = g_list_first(tlist)->data;
        if(strlen(aid->pre_name )> 0)
        {
            STRUCTClass *fclass = factory_find_diaobject_by_name(curLayer,aid->pre_name);
            if(fclass ) /* 防止自身ＩＤ更新 */
            {
                aid->value = g_strdup_printf("%d",fclass->element.object.oindex);
            }
        }
        factory_write_object_comobox(aid,ccc);
    }
    break;
    case OBTN:
    {
        NextID *nid  = &sss->value.nextid;
        GList *tlist = nid->actlist;
        ObjectNode obtn = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_obtn", NULL);
        xmlSetProp(obtn, (const xmlChar *)"name", (xmlChar *)sss->name);
        xmlSetProp(obtn, (const xmlChar *)"wtype", (xmlChar *)"OBTN");
        for(; tlist; tlist = tlist->next)
        {
            ActionID *aid = tlist->data;
            ObjectNode ccc = xmlNewChild(obtn, NULL, (const xmlChar *)"JL_item", NULL);
            xmlSetProp(obtn, (const xmlChar *)"name", (xmlChar *)sss->name);
            xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)sss->type);
            xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"OCOMBO");
            STRUCTClass *fclass = factory_find_diaobject_by_name(curLayer,aid->pre_name);
            if(fclass && fclass->widgetSave->data) /* 防止自身ＩＤ更新 */
            {
                SaveStruct *tmp = fclass->widgetSave->data;
                aid->value = g_strdup(tmp->value.vnumber);
            }
            factory_write_object_comobox(tlist->data,ccc);
        }
    }
    break;
    case UCOMBO:
    {
        SaveUnion *suptr = &sss->value.sunion;
        SaveStruct *tsst  =  g_hash_table_lookup(suptr->saveVal,suptr->curkey);
        if(tsst)
        {
            ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_union", NULL);
            xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)sss->name);
            xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)sss->type);
            if(tsst->isPointer)
            {
                xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)"UBTN");
                xmlSetProp(ccc, (const xmlChar *)"index", (xmlChar *)g_strdup_printf("%d",suptr->index));
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
        gchar **split = g_strsplit(sss->type,".",-1);
        gchar *stype = split[g_strv_length(split)-1];
        if(factory_is_special_object(stype)) /* 特殊控件 */
        {
            ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_item", NULL);
            xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)sss->name);
            xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
            xmlSetProp(ccc, (const xmlChar *)"wtype", (xmlChar *)sss->type);

            SaveKV *skv = sss->value.vnumber;
            xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)skv->value);
            xmlSetProp(ccc, (const xmlChar *)"index", (xmlChar *)g_strdup_printf("%d",skv->radindex));

        }
        else
        {
            SaveUbtn *sbtn = &sss->value.ssubtn;
//        GList *slist = g_hash_table_get_values(sbtn->htoflist);
            GList *slist = sbtn->savelist;
            if(slist)
            {
                for(; slist; slist = slist->next)
                {
                    SaveStruct *s = slist->data;
                    factory_base_struct_save_to_file(s,obj_node);
                }
            }
        }
        g_strfreev(split);


    }
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
factory_struct_items_load(ObjectNode obj_node,int version, const char *filename)
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
    obj->type->version = g_strdup(factoryContainer->file_version);
    element_load(elem, obj_node);

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

    structclass->widgetmap = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
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
    attr_node  =   factory_find_custom_node(obj_node,"JL_struct");
    if(attr_node)
    {

        xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"name");
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
    int s = g_hash_table_size(structclass->widgetmap);
    if(factory_is_special_object(obj->name))
    {
        if(!MusicManagerDialog)
        {
            MusicManagerDialog = g_new0(SaveMusicDialog,1);
            MusicManagerDialog->title = factory_utf8("文件管理");
            MusicManagerDialog->smfm = NULL;
            MusicManagerDialog->mfmos = &mfmo_opts;
//                    MusicManagerDialog->radindex = -1;
        }

        if(!IdDialog)
        {
            IdDialog = g_new0(SaveIdDialog,1);
        }
        factory_read_specific_object_from_file(structclass,attr_node->xmlChildrenNode);
        structclass_destroy(structclass) ;
        return NULL;
    }
    else
        factory_read_value_from_xml(structclass,attr_node->xmlChildrenNode);
    return &structclass->element.object;
}

static void
factory_struct_items_save(STRUCTClass *structclass, ObjectNode obj_node,
                          const char *filename)
{
    element_save(&structclass->element, obj_node);
    /*  2014-3-22 lcy 这里保存自定义控件的数据 */
    /* 2014-3-25 lcy 这里添加一个自定义节点名来安置这个结构体的成员*/
    gchar *objname = structclass->element.object.name;
    obj_node = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_struct", NULL);
    xmlSetProp(obj_node, (const xmlChar *)"name", (xmlChar *)objname);
    xmlSetProp(obj_node, (const xmlChar *)"vname", (xmlChar *)structclass->name);
    FactoryStructItemList *fsi = g_hash_table_lookup(structclass->EnumsAndStructs->structTable,
                                 objname);
    if(fsi) /* 写入到指定文件名 */
        xmlSetProp(obj_node, (const xmlChar *)"file", (xmlChar *)fsi->sfile);
    //g_hash_table_foreach(structclass->widgetmap,factory_struct_save_to_xml,(gpointer)obj_node);
    if(!g_strcasecmp(fsi->sname,"FILELST"))
    {
        if(!MusicManagerDialog || !MusicManagerDialog->itemlist)
            return;
        gchar *rows = g_strdup_printf("%d",g_list_length(MusicManagerDialog->itemlist));
        xmlSetProp(obj_node, (const xmlChar *)"rows", (xmlChar *)rows);
        g_free(rows);
        GList *flist = MusicManagerDialog->itemlist;
        for(; flist; flist = flist->next)
        {
            SaveMusicItem *smi = flist->data;
            ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_item", NULL);
            xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)g_strdup_printf("%d",smi->id_index));
            xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
            xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)g_strdup_printf("%d",smi->active ? smi->active : -1));
            xmlSetProp(ccc, (const xmlChar *)"addr", (xmlChar *)g_strdup_printf("%d",smi->id_addr));
        }

        ObjectNode newobj = xmlNewChild(obj_node, NULL, (const xmlChar *)"Music_File", NULL);

        xmlSetProp(newobj, (const xmlChar *)"offset",
                   (xmlChar *)g_strdup_printf("%d",MusicManagerDialog->smfm->offset));
        flist = MusicManagerDialog->smfm->filelist;
        for(; flist; flist = flist->next) /* 这里保存文件列表 */
        {
            SaveMusicFile *smf = flist->data;
            ObjectNode tnode = xmlNewChild(newobj, NULL, (const xmlChar *)"file", NULL);
            xmlSetProp(tnode, (const xmlChar *)"name", (xmlChar *)g_strdup_printf("%d",smf->index));
            xmlSetProp(tnode, (const xmlChar *)"addr", (xmlChar *)g_strdup_printf("%d",smf->file_addr));
            xmlSetProp(tnode, (const xmlChar *)"fname", (xmlChar *)smf->full_name);
            xmlSetProp(tnode, (const xmlChar *)"dname", (xmlChar *)smf->down_name);
        }
    }
    else if(!g_strcasecmp(fsi->sname,"IDLST"))
    {
        if(!IdDialog || !IdDialog->idlists)
            return;
        gchar *rows = g_strdup_printf("%d",g_list_length(IdDialog->idlists));
        xmlSetProp(obj_node, (const xmlChar *)"rows", (xmlChar *)rows);
        g_free(rows);
        GList *idlist = IdDialog->idlists;
        for(; idlist; idlist = idlist->next)
        {
            SaveIdItem *sit =idlist->data;
            ObjectNode ccc = xmlNewChild(obj_node, NULL, (const xmlChar *)"JL_item", NULL);
            xmlSetProp(ccc, (const xmlChar *)"name", (xmlChar *)g_strdup_printf("%d",sit->id_index));
            xmlSetProp(ccc, (const xmlChar *)"type", (xmlChar *)"u16");
            gchar *val = "-1";
            if(sit->active>0)
            {
                STRUCTClass *tcalss = factory_get_object_from_layer(curLayer,sit->dname);
                if(tcalss)
                {
                    SaveStruct *sst = tcalss->widgetSave->data;
                    val = g_strdup(sst->value.vnumber);
                }
            }
            xmlSetProp(ccc, (const xmlChar *)"value", (xmlChar *)val);
            xmlSetProp(ccc, (const xmlChar *)"addr", (xmlChar *)g_strdup_printf("%d",sit->id_addr));
        }
    }
    else
    {
        GList *saveList = structclass->widgetSave;

        for(; saveList; saveList = saveList->next)
        {
            factory_base_struct_save_to_file(saveList->data,obj_node);
        }
    }


}



//void factory_get_enum_values(GList* src,GList *dst)
//{
//    /* 2014-3-26 lcy 这里处理控件上的值，用来做保存文件时的数据源。*/
//    GList *thisstruct = src;
//    for(; thisstruct != NULL; thisstruct = thisstruct->next)
//    {
//        SaveStruct *ss = g_new0(SaveStruct,1);
//        FactoryStructItem *item = thisstruct->data;
//        gchar **split = g_strsplit(item->FType,".",-1);
//        int section = g_strv_length(split);
//        /* 查询枚举哈希表的值*/
//        GList *enumitem =  g_hash_table_lookup(factoryContainer->enumTable,(gpointer)split[section-1]);
//        if(enumitem)
//        {
////                             GList *tmp  = enumitem;
////                             ss->celltype = ENUM;
////                             FactoryStructEnum *cur =  factory_find_list_node(tmp,item->Value);
////                             ss->value.index = g_list_index(tmp,cur);
//        }
//        else if(factory_find_array_flag(item->Name))
//        {
//            ss->celltype = ENTRY;
//        }
//        else
//        {
//            ss->celltype = SPINBOX;
//        }
//        ss->name = item->Name;
//        ss->type = item->FType;
//        g_strfreev(split);
//        dst = g_list_append(dst,ss);
//    }
//}

//static void
//structclass_save(STRUCTClass *structclass, ObjectNode obj_node,
//	      const char *filename)
//{
//  STRUCTAttribute *attr;
//  STRUCTOperation *op;
//  STRUCTFormalParameter *formal_param;
//  GList *list;
//  AttributeNode attr_node;
//
//#ifdef DEBUG
//  structclass_sanity_check(structclass, "Saving");
//#endif
//
//  element_save(&structclass->element, obj_node);
//fclass->name
//  /* Class info: */
//  data_add_string(new_attribute(obj_node, "name"),
//		  structclass->name);
//  data_add_string(new_attribute(obj_node, "stereotype"),
//		  structclass->stereotype);
//  data_add_string(new_attribute(obj_node, "comment"),
//                  structclass->comment);
//  data_add_boolean(new_attribute(obj_node, "abstract"),
//		   structclass->abstract);
//  data_add_boolean(new_attribute(obj_node, "suppress_attributes"),
//		   structclass->suppress_attributes);
//  data_add_boolean(new_attribute(obj_node, "suppress_operations"),
//		   structclass->suppress_operations);
//  data_add_boolean(new_attribute(obj_node, "visible_attributes"),
//		   structclass->visible_attributes);
//  data_add_boolean(new_attribute(obj_node, "visible_operations"),
//		   structclass->visible_operations);
//  data_add_boolean(new_attribute(obj_node, "visible_comments"),
//		   structclass->visible_comments);
//  data_add_boolean(new_attribute(obj_node, "wrap_operations"),
//		   structclass->wrap_operations);
//  data_add_int(new_attribute(obj_node, "wrap_after_char"),
//		   structclass->wrap_after_char);
//  data_add_int(new_attribute(obj_node, "comment_line_length"),
//                   structclass->comment_line_length);
//  data_add_boolean(new_attribute(obj_node, "comment_tagging"),
//                   structclass->comment_tagging);
//  data_add_real(new_attribute(obj_node, PROP_STDNAME_LINE_WIDTH),
//		   structclass->line_width);
//  data_add_color(new_attribute(obj_node, "line_color"),
//		   &structclass->line_color);
//  data_add_color(new_attribute(obj_node, "fill_color"),
//		   &structclass->fill_color);
//  data_add_color(new_attribute(obj_node, "text_color"),
//		   &structclass->text_color);
//  data_add_font (new_attribute (obj_node, "normal_font"),
//                 structclass->normal_font);
//  data_add_font (new_attribute (obj_node, "abstract_font"),
//                 structclass->abstract_font);
//  data_add_font (new_attribute (obj_node, "polymorphic_font"),
//                 structclass->polymorphic_font);
//  data_add_font (new_attribute (obj_node, "classname_font"),
//                 structclass->classname_font);
//  data_add_font (new_attribute (obj_node, "abstract_classname_font"),
//                 structclass->abstract_classname_font);
//  data_add_font (new_attribute (obj_node, "comment_font"),
//                 structclass->comment_font);
//  data_add_real (new_attribute (obj_node, "normal_font_height"),
//                 structclass->font_height);
//  data_add_real (new_attribute (obj_node, "polymorphic_font_height"),
//                 structclass->polymorphic_font_height);
//  data_add_real (new_attribute (obj_node, "abstract_font_height"),
//                 structclass->abstract_font_height);
//  data_add_real (new_attribute (obj_node, "classname_font_height"),
//                 structclass->classname_font_height);
//  data_add_real (new_attribute (obj_node, "abstract_classname_font_height"),
//                 structclass->abstract_classname_font_height);
//  data_add_real (new_attribute (obj_node, "comment_font_height"),
//                 structclass->comment_font_height);
//
//  /* Attribute info: */
//  attr_node = new_attribute(obj_node, "attributes");
//  list = structclass->attributes;
//  while (list != NULL) {
//    attr = (STRUCTAttribute *) list->data;
//    struct_attribute_write(attr_node, attr);
//    list = g_list_next(list);
//  }
//
//  /* Operations info: */
//  attr_node = new_attribute(obj_node, "operations");
//  list = structclass->operations;
//  while (list != NULL) {
//    op = (STRUCTOperation *) list->data;
//    struct_operation_write(attr_node, op);
//    list = g_list_next(list);
//  }
//
//  /* Template info: */
//  data_add_boolean(new_attribute(obj_node, "template"),
//		   structclass->template);
//
//  attr_node = new_attribute(obj_node, "templates");
//  list = structclass->formal_params;
//  while (list != NULL) {
//    formal_param = (STRUCTFormalParameter *) list->data;
//    struct_formalparameter_write(attr_node, formal_param);
//    list = g_list_next(list);
//  }
//}

//static DiaObject *structclass_load(ObjectNode obj_node, int version,
//			     const char *filename)
//{
//  STRUCTClass *structclass;
//  Element *elem;
//  DiaObject *obj;
//  AttributeNode attr_node;
//  int i;
//  GList *list;
//
//
//  structclass = g_malloc0(sizeof(STRUCTClass));
//
//  elem = &structclass->element;
//  obj = &elem->object;
//
//  obj->type = &structclass_type;
//  obj->ops = &structclass_ops;
//
//  element_load(elem, obj_node);
//
//#ifdef STRUCT_MAINPOINT
//  element_init(elem, 8, STRUCTCLASS_CONNECTIONPOINTS + 1);
//#else
//  element_init(elem, 8, STRUCTCLASS_CONNECTIONPOINTS);
//#endif
//
//  structclass->properties_dialog =  NULL;
//
//  for (i=0;i<STRUCTCLASS_CONNECTIONPOINTS;i++) {
//    obj->connections[i] = &structclass->connections[i];
//    structclass->connections[i].object = obj;
//    structclass->connections[i].connected = NULL;
//  }
//
//  fill_in_fontdata(structclass);
//
//  /* kind of dirty, object_load_props() may leave us in an inconsistent state --hb */
//  object_load_props(obj,obj_node);
//
//  /* parameters loaded via StdProp dont belong here anymore. In case of strings they
//   * will produce leaks. Otherwise the are just wasteing time (at runtime and while
//   * reading the code). Except maybe for some compatibility stuff.
//   * Although that *could* probably done via StdProp too.                      --hb
//   */
//
//  /* new since 0.94, don't wrap by default to keep old diagrams intact */
//    attr_node  =   factory_find_custom_node(obj_node,"JL_struct");
//  if(attr_node)
//  {
//      xmlChar *key = xmlGetProp(attr_node,(xmlChar *)"name");
//      if (key) {
//           structclass->name =  g_locale_to_utf8(key,-1,NULL,NULL,NULL);
//            xmlFree (key);
//        }
//  }
//
//  structclass->wrap_operations = FALSE;
//  attr_node = object_find_attribute(obj_node, "wrap_operations");
//  if (attr_node != NULL)
//    structclass->wrap_operations = data_boolean(attribute_first_data(attr_node));
//
//  structclass->wrap_after_char = STRUCTCLASS_WRAP_AFTER_CHAR;
//  attr_node = object_find_attribute(obj_node, "wrap_after_char");
//  if (attr_node != NULL)
//    structclass->wrap_after_char = data_int(attribute_first_data(attr_node));
//
//  /* if it uses the new name the value is already set by object_load_props() above */
//  structclass->comment_line_length = STRUCTCLASS_COMMENT_LINE_LENGTH;
//  attr_node = object_find_attribute(obj_node,"comment_line_length");
//  /* support the unusal cased name, although it only existed in cvs version */
//  if (attr_node == NULL)
//    attr_node = object_find_attribute(obj_node,"Comment_line_length");
//  if (attr_node != NULL)
//    structclass->comment_line_length = data_int(attribute_first_data(attr_node));
//
//  /* compatibility with 0.94 and before as well as the temporary state with only 'comment_line_length' */
//  structclass->comment_tagging = (attr_node != NULL);
//  attr_node = object_find_attribute(obj_node, "comment_tagging");
//  if (attr_node != NULL)
//    structclass->comment_tagging = data_boolean(attribute_first_data(attr_node));
//
//  /* Loads the line width */
//  structclass->line_width = STRUCTCLASS_BORDER;
//  attr_node = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
//  if(attr_node != NULL)
//    structclass->line_width = data_real(attribute_first_data(attr_node));
//
//  structclass->line_color = color_black;
//  /* support the old name ... */
//  attr_node = object_find_attribute(obj_node, "foreground_color");
//  if(attr_node != NULL)
//    data_color(attribute_first_data(attr_node), &structclass->line_color);
//  structclass->text_color = structclass->line_color;
//  /* ... but prefer the new one */
//  attr_node = object_find_attribute(obj_node, "line_color");
//  if(attr_node != NULL)
//    data_color(attribute_first_data(attr_node), &structclass->line_color);
//  attr_node = object_find_attribute(obj_node, "text_color");
//  if(attr_node != NULL)
//    data_color(attribute_first_data(attr_node), &structclass->text_color);
//
//  structclass->fill_color = color_white;
//  /* support the old name ... */
//  attr_node = object_find_attribute(obj_node, "background_color");
//  if(attr_node != NULL)
//    data_color(attribute_first_data(attr_node), &structclass->fill_color);
//  /* ... but prefer the new one */
//  attr_node = object_find_attribute(obj_node, "fill_color");
//  if(attr_node != NULL)
//    data_color(attribute_first_data(attr_node), &structclass->fill_color);
//
//  /* Attribute info: */
//  list = structclass->attributes;
//  while (list) {
//    STRUCTAttribute *attr = list->data;
//    g_assert(attr);
//
//    struct_attribute_ensure_connection_points (attr, obj);
//    list = g_list_next(list);
//  }
//
//  /* Operations info: */
//  list = structclass->operations;
//  while (list) {
//    STRUCTOperation *op = (STRUCTOperation *)list->data;
//    g_assert(op);
//
//    struct_operation_ensure_connection_points (op, obj);
//    list = g_list_next(list);
//  }
//
//  /* Template info: */
//  structclass->template = FALSE;
//  attr_node = object_find_attribute(obj_node, "template");
//  if (attr_node != NULL)
//    structclass->template = data_boolean(attribute_first_data(attr_node));
//
//  fill_in_fontdata(structclass);
//
//  structclass->stereotype_string = NULL;
//
//  structclass_calculate_data(structclass);
//
//  elem->extra_spacing.border_trans = structclass->line_width/2.0;
//  structclass_update_data(structclass);
//
//  for (i=0;i<8;i++) {
//    obj->handles[i]->type = HANDLE_NON_MOVABLE;
//  }
//
//#ifdef DEBUG
//  structclass_sanity_check(structclass, "Loaded class");
//#endif
//
//  return &structclass->element.object;
//}

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


