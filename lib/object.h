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

/** @file object.h -- definitions for Dia objects, in particular the 'virtual'
 * functions and the object and type structures.
 */
#ifndef OBJECT_H
#define OBJECT_H

#include "diatypes.h"
#include <gtk/gtk.h>

#include "geometry.h"
#include "connectionpoint.h"
#include "handle.h"
#include "diamenu.h"
#include "objchange.h"
#include "dia_xml.h"
#include "text.h"



GHashTable *global_Table;
G_BEGIN_DECLS

/** Flags for DiaObject */
/** Set this if the DiaObject can 'contain' other objects that move with
 *  it, a.k.a. be a parent.  A parent moves its children along and
 *  constricts its children to live inside its borders.
 */
#define DIA_OBJECT_CAN_PARENT 1
/** Set this if the DiaObject grabs all input destined for its children.
 * This is typically used for group-like objects.
 */
#define DIA_OBJECT_GRABS_CHILD_INPUT 2
/** Set this if the DiaObject expands/contracts when children are
 * moved/modified/added/deleted */
#define DIA_OBJECT_EXPAND_WITH_CHILDREN 4

/** This enumeration gives a bitset of modifier keys currently held down.
 */
typedef enum {
  MODIFIER_NONE,
  MODIFIER_LEFT_SHIFT,
  MODIFIER_RIGHT_SHIFT,
  MODIFIER_SHIFT,
  MODIFIER_LEFT_ALT = 4,
  MODIFIER_RIGHT_ALT = 8,
  MODIFIER_ALT = 12,
  MODIFIER_LEFT_CONTROL = 16,
  MODIFIER_RIGHT_CONTROL = 32,
  MODIFIER_CONTROL = 48
} ModifierKeys;

/************************************
 ** Some general function prototypes
 **
 ** These are the prototypes for the
 ** functions that must be defined for
 ** every object we can insert in a
 ** diagram.
 **
 ************************************/

/*** Object type (class) operations ***/

/** Function called to create an object.
 *  This function is responsible for allocation memory for the object.
 *  This is one of the object class functions.
 * @param startpoint : initial position for the object
 * @param user_data  : Can be used to pass extra params to the creation
 *                of the object. Can be used to have two icons of
 *                the same object that are instansiated a bit different.
 * @param handle1    : (return) Handle connected to startpoint
 * @param handle2    : (return) Handle dragged on creation
 *  both handle1 and handle2 can be NULL
 * @return A newly created object.
*/
typedef DiaObject* (*CreateFunc) (Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);

/** This function load the object's data from file fd. No header has to be
 *  skipped. The data should be read using the functions in lib/files.h
 *  The memory for the object has to be allocated (see CreateFunc)
 *  The version number is the version number of the DiaObjectType that was saved.
 *  This must be used to maintain backwards compatible if you change some
 *  in the save format. All objects must be capable of reading all earlier
 *  version.
 *  This is one of the object class functions.
 * @param obj_node A node in an XML object to load from.
 * @param version The version of the object found in the XML file.
 * @param filename The name of the file we're loading from, for use in
 *                 error messages.
 */
typedef DiaObject* (*LoadFunc) (ObjectNode obj_node, int version,
				const char *filename);

/** This function save the object's data to file fd. No header is required.
 *  The data should be written using the functions in lib/files.h
 *  This is one of the object class functions.
 * @param obj An object to save.
 * @param obj_node An XML node to save it in.
 * @param filename The name of the file we're saving to, for use in error
 *                 messages.
 */
typedef void (*SaveFunc) (DiaObject* obj, ObjectNode obj_node,
			  const char *filename);

/** Function called when the user has double clicked on an Tool.
 *  When this function is called and the dialog already is created,
 *  make sure to update the values in the widgets so that it
 *  accurately describes the current state of the tool.
 *  This is one of the object class functions.
 * @return a dialog that the user can use to edit the defaults for new
 * objects of this type.
*/
typedef GtkWidget *(*GetDefaultsFunc) ();

/** Function called when the user clicks Apply on an edit defaults dialog.
 * This is currently not used by any object.
 * This is one of the object class functions.
 */
typedef void *(*ApplyDefaultsFunc) ();

/*** Object operations ***/

/** Function called before an object is deleted.
 *  This function must call the parent class's DestroyFunc, and then free
 *  the memory associated with the object, but not the object itself
 *  Must also unconnect itself from all other objects.
 *  (This is by calling object_destroy, or letting the super-class call it)
 *  This is one of the object_ops functions.
 * @param obj An object to destroy.
 */
typedef void (*DestroyFunc) (DiaObject* obj);


/** Function responsible for drawing the object.
 *  Every drawing must be done through the use of the Renderer, so that we
 *  can render the picture on screen, in an eps file, ...
 *  This is one of the object_ops functions.
 * @param The object to draw.
 * @param The renderer object to draw with.
 */
typedef void (*DrawFunc) (DiaObject* obj, DiaRenderer* ddisp);


/** This function must return the distance between the DiaObject and the Point.
 *  Several functions are provided in geometry.h to facilitate this calculus.
 *  This is one of the object_ops functions.
 * @param obj The object.
 * @param point A point to give the distance to.
 * @return The distance from the point to the nearest part of the object.
 *         If the point is inside a closed object, return 0.0.
 */
typedef real (*DistanceFunc) (DiaObject* obj, Point* point);


/** Function called once the object has been selected.
 *  Basically, this function should update the object (position of the
 *  handles,...)  This is one of the object_ops functions.
 *  This function should not redraw the object.
 * @param obj An object that is being selected.
 * @param clicked_point is the point on the screen where the user has clicked
 * @param interactive_renderer is a renderer that has some extra functions
 *                        most notably the possibility to get EXACT
 *			 measures of strings. Used to place cursors
 *			 and other interactive stuff.
 *			 (Don't draw to the renderer)
 */
typedef void (*SelectFunc) (DiaObject*   obj,
			    Point*    clicked_point,
			    DiaRenderer* interactive_renderer);

/** Returns a copy of DiaObject.
 *  This must be an depth-copy (pointers must be duplicated and so on)
 *  as the initial object can be deleted any time.
 *  This is one of the object_ops functions.
 * @param obj An object to make a copy of.
 * @return A newly allocated object copied from `obj', but without any
 *         connections to other objects.
 */
typedef DiaObject* (*CopyFunc) (DiaObject* obj);

/** Function called to move the entire object.
 *  This is one of the object_ops functions.
 * @param obj The object being moved.
 * @param pos Where the object is being moved to.
 *  Its exact definition depends on the object. It is the point on the
 *  object that 'snaps' to the grid if that is enabled. (generally it
 *  is the upper left corner)
 * @return An ObjectChange* with additional undo information, or
 * (in most cases) NULL.  Undo for moving the object itself is handled
 * elsewhere.
 */
typedef ObjectChange* (*MoveFunc) (DiaObject* obj, Point * pos);

/** Function called to move one of the handles associated with the
 *  object.  This is one of the object_ops functions.
 * @param obj The object whose handle is being moved.
 * @param handle The handle being moved.
 * @param pos The position it has been moved to (corrected for
 *   vertical/horizontal only movement).
 * @param cp If non-NULL, the connectionpoint found at this position.
 *   If @a cp is NULL, there may or may not be a connectionpoint.
 * @param The reason the handle was moved.
 *     - HANDLE_MOVE_USER means the user is dragging the point.
 *     - HANDLE_MOVE_USER_FINAL means the user let go of the point.
 *     - HANDLE_MOVE_CONNECTED means it was moved because something
 *	    it was connected to moved.
 * @param modifiers gives a bitset of modifier keys currently held down
 *     - MODIFIER_SHIFT is either shift key
 *     - MODIFIER_ALT is either alt key
 *     - MODIFIER_CONTROL is either control key
 *	    Each has MODIFIER_LEFT_* and MODIFIER_RIGHT_* variants
 * @return An @a ObjectChange* with additional undo information, or
 *  (in most cases) NULL.  Undo for moving the handle itself is handled
 *  elsewhere.
 */
typedef ObjectChange* (*MoveHandleFunc) (DiaObject*          obj,
					 Handle*          handle,
					 Point*           pos,
					 ConnectionPoint* cp,
					 HandleMoveReason reason,
					 ModifierKeys     modifiers);

/** Function called when the user has double clicked on an DiaObject.
 *  This is one of the object_ops functions.
 * @param obj An obj that this dialog is being made for.
 * @param is_default If true, this dialog is for object defaults, and
 * the toolbox options should not be shown.
 * @return A dialog to edit the properties of the object.
 * When this function is called and the dialog already is created,
 * make sure to update the values in the widgets so that it
 * accurately describes the current state of the object.
 * Remember to destroy this dialog when the object is destroyed!

 * Note that if you want to use the same dialog multiple times,
 * you should ref it first.  Just run the following on the widget
 * when you create it:
 *   gtk_object_ref(GTK_OBJECT(widget));
 *   gtk_object_sink(GTK_OBJECT(widget)); / * optional, but recommended * /
 * If you don't do this, the widget will be destroyed when the
 * properties dialog is closed.
 */
typedef GtkWidget *(*GetPropertiesFunc) (DiaObject* obj, gboolean is_default);

/** This function is called when the user clicks on
 *  the "Apply" button.  The widget parameter is the one created by
 *  the get_properties function.
 *  This is one of the object_ops functions.
 * @param obj The object whose dialog has had its Apply button clicked.
 * @param widget The properties dialog being applied.
 * @return a Change that can be used for undo/redo.
 * The returned change is already applied.
 */
typedef ObjectChange *(*ApplyPropertiesDialogFunc) (DiaObject* obj, GtkWidget *widget);

/** This function is used to apply a list of properties to the current
 *  object. It is typically called by ApplyPropertiesDialogFunc. This
 *  is different from SetPropsFunc since this is used to implement
 *  undo/redo.
 *  This is one of the object_ops functions.
 * @param obj The object to which properties are to be applied
 * @param props The list of properties that are to be applied
 * @return a Change for undo/redo
 */
 typedef ObjectChange *(*ApplyPropertiesListFunc) (DiaObject* obj, GPtrArray* props);

/** Desribe the properties that this object supports.
 *  This is one of the object_ops functions.
 * @param obj The object whose properties we want described.
 * @return a NULL-terminated array of property descriptions.
 * As the const return implies the returned data is not owned by  the
 * caller. If this function returns a dynamically created description,
 * then DestroyFunc must free the description.
 */
typedef const PropDescription *(* DescribePropsFunc) (DiaObject *obj);

/** Get the actual values of the properties given.
 * Note that the props array need not contain all the properties
 * defined for the object, nor do all the properties in the array need be
 * defined for the object.  All properties in the props array that are
 * actually set will be set.
 * This is one of the object_ops functions.
 * @param obj An object that delivers the values.
 * @param props A list of Property objects whose values are to be set based
 *              on the objects internal data.  The types for the objects are
 *              also being set as a side-effect.
 */
typedef void (* GetPropsFunc) (DiaObject *obj, GPtrArray *props);

/** Set the object to have the values defined in the properties list.
 * Note that the props array may contain more or fewer properties than the
 * object defines, but only and all the ones defined for the object will
 * be applied to the object.
 * This is one of the object_ops functions.
 * @param obj An object to update values on.
 * @param props An array of Property objects whose values are to be set on
 *              the object.
 */
typedef void (* SetPropsFunc) (DiaObject *obj, GPtrArray *props);

/** Return an object-specific menu with toggles etc. properly set.
 * This is one of the object_ops functions.
 * @param obj The object that is selected when the object menu is asked for.
 * @param position Where the user clicked.  This can be used to place whatever
 * the menu point may create, such as new segment corners.
 * @return A menu description with values set appropriately for this object.
 * The description object must not be freed by the caller.
 */
typedef DiaMenu *(*ObjectMenuFunc) (DiaObject* obj, Point *position);

/** Function for updates on a text part of an object.  This function, if
 * not null, will be called every time the text is changed or editing
 * starts or stops.
 * This is one of the object_ops functions.
 * @param obj The self object
 * @param text The text entry being edited
 * @param state The state of the editing, either TEXT_EDIT_START,
 * TEXT_EDIT_INSERT, TEXT_EDIT_DELETE, or TEXT_EDIT_END.
 * @param textchange For TEXT_EDIT_INSERT, the text about to be inserted.
 * For TEXT_EDIT_DELETE, the text about to be deleted.
 * @return For TEXT_EDIT_INSERT and TEXT_EDIT_DELETE, TRUE this change
 * will be allowed, FALSE otherwise.  For TEXT_EDIT_START and TEXT_EDIT_END,
 * the return value is ignored.
 */
typedef gboolean (*TextEditFunc) (DiaObject *obj, Text *text, TextEditState state, gchar *textchange);

typedef void  (*UpdateData)(DiaObject *obj);
typedef void  (*ConnectionTwoObject)(DiaObject *obj,gpointer data,int num);

typedef void (*UpdateObjectIndex)(DiaObject *obj);
typedef void (*ObjectRename)(DiaObject *obj);
typedef void (*UpdateObjectVName)(DiaObject *obj);
/*************************************
 **  The functions provided in object.c
 *************************************/


DIAVAR void object_init(DiaObject *obj, int num_handles, int num_connections);
DIAVAR void object_destroy(DiaObject *obj); /* Unconnects handles, so don't
					    free handles before calling. */
DIAVAR void object_copy(DiaObject *from, DiaObject *to);

DIAVAR void object_save(DiaObject *obj, ObjectNode obj_node);
DIAVAR void object_load(DiaObject *obj, ObjectNode obj_node);

DIAVAR GList *object_copy_list(GList *list);
ObjectChange* object_list_move_delta_r(GList *objects, Point *delta, gboolean affected);
DIAVAR ObjectChange* object_list_move_delta(GList *objects, Point *delta);
/** Rotate an object around a point.  If center is NULL, the position of
 * the object is used. */
DIAVAR ObjectChange* object_list_rotate(GList *objects, Point *center, real angle);
/** Scale an object around a point.  If center is NULL, the position of
 * the object is used. */
ObjectChange* object_list_scale(GList *objects, Point *center, real factor);
DIAVAR void destroy_object_list(GList *list);
DIAVAR void object_add_handle(DiaObject *obj, Handle *handle);
void object_add_handle_at(DiaObject *obj, Handle *handle, int pos);
DIAVAR void object_remove_handle(DiaObject *obj, Handle *handle);
DIAVAR void object_add_connectionpoint(DiaObject *obj, ConnectionPoint *conpoint);
DIAVAR void object_remove_connectionpoint(DiaObject *obj,
				   ConnectionPoint *conpoint);
void object_add_connectionpoint_at(DiaObject *obj,
				   ConnectionPoint *conpoint,
				   int pos);
DIAVAR void object_connect(DiaObject *obj, Handle *handle,
		    ConnectionPoint *conpoint);
DIAVAR void object_unconnect(DiaObject *connected_obj, Handle *handle);
DIAVAR void object_remove_connections_to(ConnectionPoint *conpoint);
void object_unconnect_all(DiaObject *connected_obj);
void object_registry_init(void);
DIAVAR void object_register_type(DiaObjectType *type);
DIAVAR void object_registry_foreach(GHFunc func, gpointer  user_data);
DiaObjectType *object_get_type(char *name);
DIAVAR gchar *object_get_displayname (DiaObject* obj);
DIAVAR gboolean object_flags_set(DiaObject* obj, gint flags);

DIAVAR GtkWidget* factory_create_new_dialog_with_buttons(gchar *title,GtkWidget *parent);

/* These functions can be used as a default implementation for an object which
   can be completely described, loaded and saved through standard properties.
*/
DIAVAR DiaObject *object_load_using_properties(const DiaObjectType *type,
                                     ObjectNode obj_node, int version,
                                     const char *filename);
DIAVAR void object_save_using_properties(DiaObject *obj, ObjectNode obj_node,
                                  int version, const char *filename);
DIAVAR DiaObject *object_copy_using_properties(DiaObject *obj);

/*****************************************
 **  The structures used to define an object
 *****************************************/

/**
 * \private This is currently unused
 *
 * This structure defines an affine transformation that has been applied
 * to this object.  Affine transformations done on a group are performed
 * on all objects in the group.
 */
typedef struct _Affine {
  real rotation;
  real scale;
  real translation;
} Affine;


/*!
  \brief _DiaObject vtable

  This structure gives access to the functions used to manipulate an object
  See information above on the use of the functions
*/
struct _ObjectOps {
  DestroyFunc         destroy;
  DrawFunc            draw;
  DistanceFunc        distance_from;
  SelectFunc          selectf;
  CopyFunc            copy;
  MoveFunc            move;
  MoveHandleFunc      move_handle;
  GetPropertiesFunc   get_properties;
  ApplyPropertiesDialogFunc  apply_properties_from_dialog;
  ObjectMenuFunc      get_object_menu;

  DescribePropsFunc   describe_props;
  GetPropsFunc        get_props;
  SetPropsFunc        set_props;

  TextEditFunc        edit_text;

  ApplyPropertiesListFunc apply_properties_list;

  /*!
    Unused places (for extension).
    These should be NULL for now. In the future they might be used.
    Then an older object will be binary compatible, because all new code
    checks if new ops are supported (!= NULL)
  */
  UpdateData update_data;
  ConnectionTwoObject connection_two_obj;
  UpdateObjectIndex   update_index;
  ObjectRename  obj_rename;
  UpdateObjectVName  update_vname;
  void      (*(unused[4]))(DiaObject *obj,...);
};

/*!
  \class _DiaObject

  \brief Base class for all of Dia's objects, i.e. diagram building blocks

  The base class in the DiaObject hierarchy.
  All information in this structure read-only
  from the application point of view except
  when connection objects. (Then handles and
  connections are changed).

  position is not necessarly the corner of the object, but rather
  some 'good' spot on it which will be natural to snap to.
*/
struct _DiaObject {
  DiaObjectType    *type; /*!< pointer to the registered type */
  Point             position; /*!<  often but not necessarily the upper left corner of the object */
  /** The area that contains all parts of the 'real' object, i.e. the parts
   *  that would be printed, exported to pixmaps etc.  This is also used to
   *  determine the size of autofit scaling, so it should be as large as
   *  the objects without interactive bits and preferably no larger.
   *  The bounding_box will always contain this box.
   *  Do not access this field directly, but use dia_object_get_enclosing_box().
   */
  Rectangle         bounding_box;
  Affine            affine; /*!< unused */

  int               num_handles;
  Handle          **handles;

  int               num_connections;
  ConnectionPoint **connections;

  ObjectOps *ops; /*!< pointer to the vtable */

  Layer *parent_layer; /*< Back-pointer to the owning layer.
			   This may only be set by functions internal to
			   the layer, and accessed via
			   dia_object_get_parent_layer() */
  DiaObject *parent; /*!< Back-pointer to DiaObject which is parenting this object. Can be NULL */
  GList *children; /*!< In case this object is a parent of other object the children are listed here */
  gint flags; /*!< Various flags that can be set for this object, see defines above */

  Color *highlight_color; /*!< The color that this object is currently
			       highlighted with, or NULL if it is not
			       highlighted. */
  /** The area that contains all parts rendered interactively, so includes
   *  handles, bezier controllers etc.  Despite historical difference, this
   *  should not be accessed directly, but through dia_object_get_bounding_box().
   *  Note that handles and connection points are not included by this, but
   *  added by that which needs it.
   *  Internal:  If this is set to a 0x0 box, returns bounding_box.  That is for
   *  those objects that don't actually calculate it, but can just use the BB.
   *  Since handles and CPs are not in the BB, that will be the case for most
   *  objects.
   */
  Rectangle         enclosing_box;
  /** Metainfo of the object, should not be manipulated directy */
  GHashTable       *meta;
  gchar *name; /* 2014-4-2 lcy 加一个名字做调试 */
  guint32 oindex; /*2014-5-4 lcy 这里是控件唯一编号*/
  gboolean selectable; /* 2014-5-28 添加一个可选与不可选的变量，比如，像线条是不可选的，可选的时候就是要删除的时候 */
};

/*!
 * \brief Vtable for _DiaObjectType
 */
struct _ObjectTypeOps {
  CreateFunc        create;
  LoadFunc          load;
  SaveFunc          save;
  GetDefaultsFunc   get_defaults;
  ApplyDefaultsFunc apply_defaults;
  /*
    Unused places (for extension).
    These should be NULL for now. In the future they might be used.
    Then an older object will be binary compatible, because all new code
    checks if new ops are supported (!= NULL)
  */
  void      (*(unused[10]))(DiaObject *obj,...);
};

/*!
   \brief _DiaObject factory

   Structure so that the ObjectFactory can create objects
   of unknown type. (Read in from a shared lib.)
 */
struct _DiaObjectType {

  char *name; /*!< The type name should follow a pattern of '<module> - <class>' like "UML - Class" */
//  int version; /*!< DiaObjects must be backward compatible, i.e. support possibly older versions formats */
  int version;


  char **pixmap; /* Also put a pixmap in the sheet_object.
		    This one is used if not in sheet but in toolbar.
		    Stored in xpm format */

  ObjectTypeOps *ops; /*!< pointer to the vtable */

  char *pixmap_file; /* fallback if pixmap is NULL */
  void *default_user_data; /* use this if no user data is specified in
                the .sheet file */
};

typedef void (*system_info_callback)(GtkWidget *parent); /*!< pointer to the vtable */

struct _FactorySystemType{
    char *name; /*!< The type name should follow a pattern of '<module> - <class>' like "UML - Class" */
//  int version; /*!< DiaObjects must be backward compatible, i.e. support possibly older versions formats */
    int version;


    char **pixmap; /* Also put a pixmap in the sheet_object.
		    This one is used if not in sheet but in toolbar.
		    Stored in xpm format */

    system_info_callback  sysinfo_callback;

    char *pixmap_file; /* fallback if pixmap is NULL */
    void *default_user_data; /* use this if no user data is specified in
                the .sheet file */
};

/* base property stuff ... */
#define OBJECT_COMMON_PROPERTIES \
  { "obj_pos", PROP_TYPE_POINT, PROP_FLAG_OPTIONAL, \
    "Object position", "Where the object is located"}, \
  { "obj_bb", PROP_TYPE_RECT, PROP_FLAG_OPTIONAL, \
    "Object bounding box", "The bounding box of the object"}, \
  { "meta", PROP_TYPE_DICT, PROP_FLAG_OPTIONAL, \
    "Object meta info", "Some key/value pairs"}

#define OBJECT_COMMON_PROPERTIES_OFFSETS \
  { "obj_pos", PROP_TYPE_POINT, offsetof(DiaObject, position) }, \
  { "obj_bb", PROP_TYPE_RECT, offsetof(DiaObject, bounding_box) }, \
  { "meta", PROP_TYPE_DICT, offsetof(DiaObject, meta) }


DIAVAR gboolean       dia_object_defaults_load (const gchar *filename,
                                         gboolean create_lazy);
DIAVAR DiaObject  *dia_object_default_get  (const DiaObjectType *type, gpointer user_data);
DIAVAR DiaObject  *dia_object_default_create (const DiaObjectType *type,
                                    Point *startpoint,
                                    void *user_data,
                                    Handle **handle1,
                                    Handle **handle2);
DIAVAR gboolean         dia_object_defaults_save (const gchar *filename);
DIAVAR Layer           *dia_object_get_parent_layer(DiaObject *obj);
DIAVAR gboolean         dia_object_is_selected (const DiaObject *obj);
DIAVAR const Rectangle *dia_object_get_bounding_box(const DiaObject *obj);
DIAVAR const Rectangle *dia_object_get_enclosing_box(const DiaObject *obj);
DiaObject       *dia_object_get_parent_with_flags(DiaObject *obj, guint flags);
DIAVAR gboolean         dia_object_is_selectable(DiaObject *obj);
/* The below are for debugging purposes only. */
DIAVAR gboolean   dia_object_sanity_check(const DiaObject *obj, const gchar *msg);

/** A standard definition of a function that takes a DiaObject */
typedef void (*DiaObjectFunc) (const DiaObject *obj);

/** convenience functions for meta info */
DIAVAR void   dia_object_set_meta (DiaObject *obj, const gchar *key, const gchar *value);
DIAVAR gchar *dia_object_get_meta (DiaObject *obj, const gchar *key);

DIAVAR gchar *factory_utf8(gchar *str);
DIAVAR gboolean factory_test_file_exist(const gchar *fname);
DIAVAR gchar *factory_locale(gchar *str);
DIAVAR gboolean factory_is_special_object(const gchar *name);
DIAVAR gchar *factory_get_last_section(const gchar *src,const gchar *delimiter);
DIAVAR gboolean factory_is_valid_type(gpointer data);
DIAVAR GtkWidget* factory_create_new_dialog_with_buttons(gchar *title,GtkWidget *parent);
DIAVAR void factory_critical_error_exit(const gchar *msg_err);
DIAVAR void factory_init_list(GtkWidget *list);
DIAVAR void factory_add_to_list(GtkWidget *list,const gchar *str);

DIAVAR void factory_callback_system_data();
DIAVAR gboolean factory_is_system_data(const gchar *name);
DIAVAR gboolean factory_is_io_port(const gchar *name);




G_END_DECLS

#endif /* OBJECT_H */


