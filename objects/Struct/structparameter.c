/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * structparameter.c : refactored from struct.c, class.c to final use StdProps
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

static PropEnumData _struct_parameter_kinds[] = {
//  { N_("Undefined"), STRUCT_UNDEF_KIND} ,
//  { N_("In"), STRUCT_IN },
//  { N_("Out"), STRUCT_OUT },
//  { N_("In & Out"), STRUCT_INOUT },
  { NULL, 0 }
};

static PropDescription structparameter_props[] = {
//  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Name"), NULL, NULL },
//  { "type", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Type"), NULL, NULL },
//  { "value", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Value"), NULL, NULL },
//  { "comment", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Comment"), NULL, NULL },
//  { "kind", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
//  N_("Kind"), NULL, _struct_parameter_kinds },

  PROP_DESC_END
};

static PropOffset structparameter_offsets[] = {
//  { "name", PROP_TYPE_STRING, offsetof(STRUCTParameter, name) },
//  { "type", PROP_TYPE_STRING, offsetof(STRUCTParameter, type) },
//  { "value", PROP_TYPE_STRING, offsetof(STRUCTParameter, value) },
//  { "comment", PROP_TYPE_STRING, offsetof(STRUCTParameter, comment) },
//  { "kind", PROP_TYPE_ENUM, offsetof(STRUCTParameter, kind) },
  { NULL, 0, 0 },
};

PropDescDArrayExtra structparameter_extra = {
  { structparameter_props, structparameter_offsets, "structparameter" },
//  (NewRecordFunc)struct_parameter_new,
//  (FreeRecordFunc)struct_parameter_destroy
};

//STRUCTParameter *
//struct_parameter_new(void)
//{
//  STRUCTParameter *param;
//
//  param = g_new0(STRUCTParameter, 1);
//  param->name = g_strdup("");
//  param->type = g_strdup("");
//  param->comment = g_strdup("");
//  param->value = NULL;
//  param->kind = STRUCT_UNDEF_KIND;
//
//  return param;
//}
//
//void
//struct_parameter_destroy(STRUCTParameter *param)
//{
//  g_free(param->name);
//  g_free(param->type);
//  if (param->value != NULL)
//    g_free(param->value);
//  g_free(param->comment);
//
//  g_free(param);
//}
//
//char *
//struct_get_parameter_string (STRUCTParameter *param)
//{
//  int len;
//  char *str;
//
//  /* Calculate length: */
//  len = strlen (param->name) + 1 + strlen (param->type);
//
//  if (param->value != NULL) {
//    len += 1 + strlen (param->value) ;
//  }
//
//  switch(param->kind)
//    {
//    case STRUCT_UNDEF_KIND:
//      break;
//    case STRUCT_IN:
//      len += 3;
//      break;
//    case STRUCT_OUT:
//      len += 4;
//      break;
//    case STRUCT_INOUT:
//      len += 6;
//      break;
//    }
//
//  /* Generate string: */
//  str = g_malloc (sizeof (char) * (len + 1));
//
//  strcpy(str, "");
//
//  switch(param->kind)
//    {
//    case STRUCT_UNDEF_KIND:
//      break;
//    case STRUCT_IN:
//      strcat (str, "in ");
//      break;
//    case STRUCT_OUT:
//      strcat (str, "out ");
//      break;
//    case STRUCT_INOUT:
//      strcat (str, "inout ");
//      break;
//    }
//
//
//  strcat (str, param->name);
//  strcat (str, ":");
//  strcat (str, param->type);
//  if (param->value != NULL) {
//    strcat (str, "=");
//    strcat (str, param->value);
//  }
//
//  g_assert (strlen (str) == len);
//
//  return str;
//}
//
