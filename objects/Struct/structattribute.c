/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * structattribute.c : refactored from struct.c, class.c to final use StdProps
 *                  PROP_TYPE_DARRAY, a list where each element is a set
 *                  of properies described by the same StdPropDesc
 * Copyright (C) 2005 Hans Breuer
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

#include <string.h>

#include "struct.h"
#include "properties.h"

extern PropEnumData _struct_visibilities[];

static PropDescription structattribute_props[] = {
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Name"), NULL, NULL },
//  { "type", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Type"), NULL, NULL },
//  { "value", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Value"), NULL, NULL },
//  { "comment", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Comment"), NULL, NULL },
//  { "visibility", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Visibility"), NULL, _struct_visibilities },
//  { "abstract", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Abstract (?)"), NULL, NULL },
//  { "class_scope", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Class scope (static)"), NULL, NULL },

  PROP_DESC_END
};

static PropOffset structattribute_offsets[] = {
//  { "name", PROP_TYPE_STRING, offsetof(STRUCTAttribute, name) },
//  { "type", PROP_TYPE_STRING, offsetof(STRUCTAttribute, type) },
//  { "value", PROP_TYPE_STRING, offsetof(STRUCTAttribute, value) },
//  { "comment", PROP_TYPE_STRING, offsetof(STRUCTAttribute, comment) },
//  { "visibility", PROP_TYPE_ENUM, offsetof(STRUCTAttribute, visibility) },
//  { "abstract", PROP_TYPE_BOOL, offsetof(STRUCTAttribute, abstract) },
//  { "class_scope", PROP_TYPE_BOOL, offsetof(STRUCTAttribute, class_scope) },
  { NULL, 0, 0 },
};


PropDescDArrayExtra structattribute_extra = {
  { structattribute_props, structattribute_offsets, "structattribute" }
//  (NewRecordFunc)struct_attribute_new,
//  (FreeRecordFunc)struct_attribute_destroy
};


//STRUCTAttribute *
//struct_attribute_new(void)
//{
//  STRUCTAttribute *attr;
//  static gint next_id = 1;
//
//  attr = g_new0(STRUCTAttribute, 1);
//  attr->internal_id = next_id++;
//  attr->name = g_strdup("");
//  attr->type = g_strdup("");
//  attr->value = NULL;
//  attr->comment = g_strdup("");
//  attr->visibility = STRUCT_PUBLIC;
//  attr->abstract = FALSE;
//  attr->class_scope = FALSE;
//#if 0 /* setup elsewhere */
//  attr->left_connection = g_new0(ConnectionPoint, 1);
//  attr->right_connection = g_new0(ConnectionPoint, 1);
//#endif
//  return attr;
//}

/** Copy the data of an attribute into another, but not the connections.
 * Frees up any strings in the attribute being copied into. */
//void
//struct_attribute_copy_into(STRUCTAttribute *attr, STRUCTAttribute *newattr)
//{
//  newattr->internal_id = attr->internal_id;
//  if (newattr->name != NULL) {
//    g_free(newattr->name);
//  }
//  newattr->name = g_strdup(attr->name);
//  if (newattr->type != NULL) {
//    g_free(newattr->type);
//  }
//  newattr->type = g_strdup(attr->type);
//
//  if (newattr->value != NULL) {
//    g_free(newattr->value);
//  }
//  if (attr->value != NULL) {
//    newattr->value = g_strdup(attr->value);
//  } else {
//    newattr->value = NULL;
//  }
//
//  if (newattr->comment != NULL) {
//    g_free(newattr->comment);
//  }
//  if (attr->comment != NULL)
//    newattr->comment = g_strdup (attr->comment);
//  else
//    newattr->comment = NULL;
//
//  newattr->visibility = attr->visibility;
//  newattr->abstract = attr->abstract;
//  newattr->class_scope = attr->class_scope;
//}

/** Copy an attribute's content.
 */
//STRUCTAttribute *
//struct_attribute_copy(STRUCTAttribute *attr)
//{
//  STRUCTAttribute *newattr;
//
//  newattr = g_new0(STRUCTAttribute, 1);
//
//  struct_attribute_copy_into(attr, newattr);
//
//  return newattr;
//}
//
//void
//struct_attribute_destroy(STRUCTAttribute *attr)
//{
//  g_free(attr->name);
//  g_free(attr->type);
//  if (attr->value != NULL)
//    g_free(attr->value);
//  if (attr->comment != NULL)
//    g_free(attr->comment);
//#if 0 /* free'd elsewhere */
//  g_free(attr->left_connection);
//  g_free(attr->right_connection);
//#endif
//  g_free(attr);
//}


//void
//struct_attribute_write(AttributeNode attr_node, STRUCTAttribute *attr)
//{
//  DataNode composite;
//
//  composite = data_add_composite(attr_node, "structattribute");
//
//  data_add_string(composite_add_attribute(composite, "name"),
//		  attr->name);
//  data_add_string(composite_add_attribute(composite, "type"),
//		  attr->type);
//  data_add_string(composite_add_attribute(composite, "value"),
//		  attr->value);
//  data_add_string(composite_add_attribute(composite, "comment"),
//		  attr->comment);
//  data_add_enum(composite_add_attribute(composite, "visibility"),
//		attr->visibility);
//  data_add_boolean(composite_add_attribute(composite, "abstract"),
//		  attr->abstract);
//  data_add_boolean(composite_add_attribute(composite, "class_scope"),
//		  attr->class_scope);
//}

/* Warning, the following *must* be strictly ASCII characters (or fix the
   following code for UTF-8 cleanliness */

//char visible_char[] = { '+', '-', '#', ' ' };
//
//char *
//struct_get_attribute_string (STRUCTAttribute *attribute)
//{
//  int len;
//  char *str;
//
//  len = 1 + (attribute->name ? strlen (attribute->name) : 0)
//          + (attribute->type ? strlen (attribute->type) : 0);
//  if (attribute->name && attribute->name[0] && attribute->type && attribute->type[0]) {
//    len += 2;
//  }
//  if (attribute->value != NULL && attribute->value[0] != '\0') {
//    len += 3 + strlen (attribute->value);
//  }
//
//  str = g_malloc (sizeof (char) * (len + 1));
//
//  str[0] = visible_char[(int) attribute->visibility];
//  str[1] = 0;
//
//  strcat (str, attribute->name ? attribute->name : "");
//  if (attribute->name && attribute->name[0] && attribute->type && attribute->type[0]) {
//    strcat (str, ": ");
//  }
//  strcat (str, attribute->type ? attribute->type : "");
//  if (attribute->value != NULL && attribute->value[0] != '\0') {
//    strcat (str, " = ");
//    strcat (str, attribute->value);
//  }
//
//  g_assert (strlen (str) == len);
//
//  return str;
//}

/*!
 * The ownership of these connection points is quite complicated. Instead of being part of the STRUCTAttribute as one may expect
  * at first, they are somewhat in between the DiaObject (see: DiaObject::connections and the concrete user, here STRUCTClass)
  * and the STRUCTAttribute.
  * But with taking undo state mangement into account it gets even worse. Deleted (to be restored connection points) live inside
  * the STRUCTClassChange until they get reverted back to the object *or* get free'd by structclass_change_free()
  * Since the implementation of attributes/operations being settable via StdProps there are more places to keep this stuff
  * consitent. So here comes a tolerant helper.
  *
  * NOTE: Same function as struct_operation_ensure_connection_points(), with C++ it would be a template function ;)
 */
//void
//struct_attribute_ensure_connection_points (STRUCTAttribute* attr, DiaObject* obj)
//{
//  if (!attr->left_connection)
//    attr->left_connection = g_new0(ConnectionPoint,1);
//  attr->left_connection->object = obj;
//  if (!attr->right_connection)
//    attr->right_connection = g_new0(ConnectionPoint,1);
//  attr->right_connection->object = obj;
//}
