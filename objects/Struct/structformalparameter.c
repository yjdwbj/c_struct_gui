/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * structformalparameter.c : refactored from struct.c, class.c to final use StdProps
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

static PropDescription structformalparameter_props[] = {
//  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Name"), NULL, NULL },
//  { "type", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Type"), NULL, NULL },

  PROP_DESC_END
};

//static PropOffset structformalparameter_offsets[] = {
////  { "name", PROP_TYPE_STRING, offsetof(STRUCTFormalParameter, name) },
////  { "type", PROP_TYPE_STRING, offsetof(STRUCTFormalParameter, type) },
//  { NULL, 0, 0 },
//};

//PropDescDArrayExtra structformalparameter_extra = {
//  { structformalparameter_props, structformalparameter_offsets, "structformalparameter" },
////  (NewRecordFunc)struct_formalparameter_new,
////  (FreeRecordFunc)struct_formalparameter_destroy
//};

//STRUCTFormalParameter *
//struct_formalparameter_new(void)
//{
//  STRUCTFormalParameter *param;
//
//  param = g_new0(STRUCTFormalParameter, 1);
//  param->name = g_strdup("");
//  param->type = NULL;
//
//  return param;
//}
//
//STRUCTFormalParameter *
//struct_formalparameter_copy(STRUCTFormalParameter *param)
//{
//  STRUCTFormalParameter *newparam;
//
//  newparam = g_new0(STRUCTFormalParameter, 1);
//
//  newparam->name = g_strdup(param->name);
//  if (param->type != NULL) {
//    newparam->type = g_strdup(param->type);
//  } else {
//    newparam->type = NULL;
//  }
//
//  return newparam;
//}
//
//void
//struct_formalparameter_destroy(STRUCTFormalParameter *param)
//{
//  g_free(param->name);
//  if (param->type != NULL)
//    g_free(param->type);
//  g_free(param);
//}
//
//void
//struct_formalparameter_write(AttributeNode attr_node, STRUCTFormalParameter *param)
//{
//  DataNode composite;
//
//  composite = data_add_composite(attr_node, "structformalparameter");
//
//  data_add_string(composite_add_attribute(composite, "name"),
//		  param->name);
//  data_add_string(composite_add_attribute(composite, "type"),
//		  param->type);
//}
//
//char *
//struct_get_formalparameter_string (STRUCTFormalParameter *parameter)
//{
//  int len;
//  char *str;
//
//  /* Calculate length: */
//  len = parameter->name ? strlen (parameter->name) : 0;
//
//  if (parameter->type != NULL) {
//    len += 1 + strlen (parameter->type);
//  }
//
//  /* Generate string: */
//  str = g_malloc (sizeof (char) * (len + 1));
//  strcpy (str, parameter->name ? parameter->name : "");
//  if (parameter->type != NULL) {
//    strcat (str, ":");
//    strcat (str, parameter->type);
//  }
//
//  g_assert (strlen (str) == len);
//
//  return str;
//}

