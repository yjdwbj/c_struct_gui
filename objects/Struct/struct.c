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

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "intl.h"
#include "object.h"
#include "plug-ins.h"
#include "sheets.h"


extern DiaObjectType structclass_type;
extern DiaObjectType template_type;
extern DiaObjectType factory_systeminfo_type;
extern GList* factory_get_download_name_list(const gchar *path);
extern TemplateOps templops;
extern empty_quark;


DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "STRUCT",
			    _("Unified Modelling Language diagram objects STRUCT 1.3"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

   object_register_type(&structclass_type);
   object_register_type(&template_type);
   object_register_type(&factory_systeminfo_type); /* 这里为系统信息注册一个类型，这里可以单独做一个动态链接库的 */
   factoryContainer = g_new0(FactoryStructItemAll,1);
   factoryContainer->fgdn_func =(FactoryGetDownloadNameList)factory_get_download_name_list;
   FactoryColors *color = g_new0(FactoryColors,1);
   color->color_foreground = color_black;
   color->color_background = color_white;
   color->color_highlight = color_highlight;
   color->color_edited = color_edited;
   factoryContainer->color = color;
   templ_ops = g_new0(TemplateOps,1);
   templ_ops = &templops;

   empty_quark = g_quark_from_static_string("");
  return DIA_PLUGIN_INIT_OK;
}


//PropEnumData _struct_visibilities[] = {
////  { N_("Public"), STRUCT_PUBLIC },
////  { N_("Private"), STRUCT_PRIVATE },
////  { N_("Protected"), STRUCT_PROTECTED },
////  { N_("Implementation"), STRUCT_IMPLEMENTATION },
//  { NULL, 0 }
//};
//
//PropEnumData _struct_inheritances[] = {
////  { N_("Abstract"), STRUCT_ABSTRACT },
////  { N_("Polymorphic (virtual)"), STRUCT_POLYMORPHIC },
////  { N_("Leaf (final)"), STRUCT_LEAF },
//  { NULL, 0 }
//};

