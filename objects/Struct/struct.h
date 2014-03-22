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

/** \file objects/STRUCT/uml.h  Objects contained  in 'STRUCT - Class' type also and helper functions */

#ifndef STRUCT_H
#define STRUCT_H

#include <glib.h>
#include "intl.h"
#include "connectionpoint.h"
#include "dia_xml.h"

typedef struct _FactoryAttribute  FactoryAttribute;

typedef struct _STRUCTAttribute STRUCTAttribute;
typedef struct _STRUCTOperation STRUCTOperation;
typedef struct _STRUCTParameter STRUCTParameter;
typedef struct _STRUCTFormalParameter STRUCTFormalParameter;

/** the visibility (allowed acces) of (to) various STRUCT sub elements */
typedef enum _STRUCTVisibility {
  STRUCT_PUBLIC, /**< everyone can use it */
  STRUCT_PRIVATE, /**< only accessible inside the class itself */
  STRUCT_PROTECTED, /**< the class and its inheritants ca use this */
  STRUCT_IMPLEMENTATION /**< ?What's this? Means implementation decision */
} STRUCTVisibility;

/** In some languages there are different kinds of class inheritances */
typedef enum _STRUCTInheritanceType {
  STRUCT_ABSTRACT, /**< Pure virtual method: an object of this class cannot be instanciated */
  STRUCT_POLYMORPHIC, /**< Virtual method : could be reimplemented in derivated classes */
  STRUCT_LEAF /**< Final method: can't be redefined in subclasses */
} STRUCTInheritanceType;

/** describes the data flow between caller and callee */
typedef enum _STRUCTParameterKind {
  STRUCT_UNDEF_KIND, /**< not defined */
  STRUCT_IN, /**< by value */
  STRUCT_OUT, /**< by ref, can be passed in uninitialized */
  STRUCT_INOUT /**< by ref */
} STRUCTParameterKind;

typedef gchar * STRUCTStereotype;

/** \brief A list of STRUCTAttribute is contained in STRUCTClass
 * Some would call them member variables ;)
 */
struct _FactoryAttribute
{
    gchar *name;
    gchar *type;
    gchar *value;
};

struct _STRUCTAttribute {
  gint internal_id; /**< Arbitrary integer to recognize attributes after
		     * the user has shuffled them in the dialog. */
  gchar *name; /**< the member variables name */
  gchar *type; /**< the return value */
  gchar *value; /**< default parameter : Can be NULL => No default value */
  gchar *comment; /**< comment */
  STRUCTVisibility visibility; /**< attributes visibility */
  int abstract; /**< not sure if this applicable */
  int class_scope; /**< in C++ : static member */

  ConnectionPoint* left_connection; /**< left */
  ConnectionPoint* right_connection; /**< right */
};

/** \brief A list of STRUCTOperation is contained in STRUCTClass
 * Some would call them member functions ;)
 */
struct _STRUCTOperation {
  gint internal_id; /**< Arbitrary integer to recognize operations after
		     * the user has shuffled them in the dialog. */
  gchar *name; /**< the function name */
  gchar *type; /**< Return type, NULL => No return type */
  gchar *comment; /**< comment */
  STRUCTStereotype stereotype; /**< just some string */
  STRUCTVisibility visibility; /**< allowed access */
  STRUCTInheritanceType inheritance_type;
  int query; /**< Do not modify the object, in C++ this is a const function */
  int class_scope;
  GList *parameters; /**< List of STRUCTParameter */

  ConnectionPoint* left_connection; /**< left */
  ConnectionPoint* right_connection; /**< right */

  gboolean needs_wrapping; /** Whether this operation needs wrapping */
  gint wrap_indent; /** The amount of indentation in chars */
  GList *wrappos; /** Absolute wrapping positions */
  real ascent; /** The ascent amount used for line distance in wrapping */
};


/** \brief A list of STRUCTParameter is contained in STRUCTOperation
 * Some would call them functions parameters ;)
 */
struct _STRUCTParameter {
  gchar *name; /**<  name*/
  gchar *type; /**< return value */
  gchar *value; /**< Initialization,  can be NULL => No default value */
  gchar *comment; /**< comment */
  STRUCTParameterKind kind; /**< Not currently used */
};

/** \brief A list of STRUCTFormalParameter is contained in STRUCTOperation
 * Some would call them template parameters ;)
 */
struct _STRUCTFormalParameter {
  gchar *name; /**< name */
  gchar *type; /**< Can be NULL => Type parameter */
};

/* Characters used to start/end stereotypes: */
/** start stereotype symbol(like \xab) for local locale */
#define STRUCT_STEREOTYPE_START _("<<")
/** end stereotype symbol(like \xbb) for local locale */
#define STRUCT_STEREOTYPE_END _(">>")

/** calculated the 'formated' representation */
extern gchar *struct_get_attribute_string (STRUCTAttribute *attribute);
/** calculated the 'formated' representation */
extern gchar *struct_get_operation_string(STRUCTOperation *operation);
/** calculated the 'formated' representation */
extern gchar *struct_get_parameter_string(STRUCTParameter *param);
/** calculated the 'formated' representation */
extern gchar *struct_get_formalparameter_string(STRUCTFormalParameter *parameter);
extern void struct_attribute_copy_into(STRUCTAttribute *srcattr, STRUCTAttribute *destattr);
extern STRUCTAttribute *struct_attribute_copy(STRUCTAttribute *attr);
extern void struct_operation_copy_into(STRUCTOperation *srcop, STRUCTOperation *destop);
extern STRUCTOperation *struct_operation_copy(STRUCTOperation *op);
extern STRUCTFormalParameter *struct_formalparameter_copy(STRUCTFormalParameter *param);
extern void struct_attribute_destroy(STRUCTAttribute *attribute);
extern void struct_operation_destroy(STRUCTOperation *op);
extern void struct_parameter_destroy(STRUCTParameter *param);
extern void struct_formalparameter_destroy(STRUCTFormalParameter *param);
extern STRUCTAttribute *struct_attribute_new(void);
extern STRUCTOperation *struct_operation_new(void);
extern STRUCTParameter *struct_parameter_new(void);
extern STRUCTFormalParameter *struct_formalparameter_new(void);

extern void struct_attribute_ensure_connection_points (STRUCTAttribute *attr, DiaObject* obj);
extern void struct_operation_ensure_connection_points (STRUCTOperation *oper, DiaObject* obj);
extern void factory_widget_value_write(AttributeNode attr_node, FactoryAttribute *attr);

extern void struct_attribute_write(AttributeNode attr_node, STRUCTAttribute *attr);
extern void struct_operation_write(AttributeNode attr_node, STRUCTOperation *op);
extern void struct_formalparameter_write(AttributeNode attr_node, STRUCTFormalParameter *param);

#endif /* STRUCT_H */

