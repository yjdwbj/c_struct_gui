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
#include "struct.h"
#include "plug-ins.h"
#include "sheets.h"

extern DiaObjectType structclass_type;
extern FactorStructItemAll structList;


static void factoryReadDataFromFile(FactorStructItemAll *allstructlist)
{
#define MAX_LINE 1024
#define MAX_SECTION 7

    gchar *datafilepath;
    const gchar* cfname = "test.data";
    struct stat statbuf;
    datafilepath = dia_get_lib_directory("config"); /// append /test.data
    if ( stat(datafilepath, &statbuf) < 0)
   {
       message_error(_("Couldn't find config path "
		  "object-libs; exiting...\n"));
    }

    char* filename = g_strconcat(datafilepath, G_DIR_SEPARATOR_S ,
		     cfname, NULL);
    FILE *fd;
    if((fd =  fopen(filename,"r")) == NULL)
    {
         message_error(_("Couldn't open filename "
		  "object-libs; exiting...\n"));
    }

    if(stat(filename,&statbuf) <0 )
    {
        message_error(_("Couldn't read  filename stats"
		  "object-libs; exiting...\n"));
    }



    char filetxt[MAX_LINE]={'\0'};
    gchar* aline = NULL;
    GSList *datalist = NULL;
    GList  *structlist = NULL;
    GList *enumlist = NULL;

//     gchar *sbuf[MAX_SECTION];   // 2014-3-19 lcy 这里分几个段
    gboolean isEmnu = FALSE;
    gboolean isStruct = FALSE;

    FactorStructEnumList *fsel = NULL;
    FactorStructItemList *fssl = NULL;
    int n = 0;
    while(fgets(filetxt,MAX_LINE,fd)!=NULL)
    {
        aline = g_strstrip(filetxt);
        if(aline[0]==':')
        {
            gchar ** sbuf=NULL;
            sbuf=  g_strsplit_set (filetxt,":",-1);

            if(g_strv_length(sbuf) < 3)
            {
                    message_error(_("This Header format is error."
                    "object-libs; exiting...\n"));
            }
            if(0 == strncmp("Enum",sbuf[1],4)) // 2014-3-20 lcy 这里匹配到枚举名字.
            {
                isEmnu = TRUE;
                fsel = g_new0(FactorStructEnumList,1);
                fsel->name = g_locale_to_utf8(sbuf[2],-1,NULL,NULL,NULL);
                fsel->list = NULL;

            }
            else if( 0 == strncmp("Struct",sbuf[1],6)) // 2014-3-20 lcy 这里匹配到结构体名字.
            {
                isStruct = TRUE;
                fssl = g_new0(FactorStructItemList,1);
                fssl->name = g_locale_to_utf8(sbuf[2],-1,NULL,NULL,NULL);
                fssl->list = NULL;
                fssl->number = n++;
            }
            g_strfreev(sbuf);
        }
        else if(aline[0] == '{' )
        {
            continue;
        }
        else if(aline[0] == '}')
        {
            if(isStruct)
            {
                isStruct = FALSE;
                structlist = g_list_append(structlist,fssl);
            }
            else{
                isEmnu = FALSE;
                enumlist = g_list_append(enumlist,fsel);
            }
        }
        else if(isEmnu) // 2014-3-19 lcy 读取一个枚举.
        {
               if(aline[0] == '/' || aline[0] == '#' || !strlen(aline))
                continue;
               FactorStructEnum *kvmap  = g_new0(FactorStructEnum,1);
               gchar ** sbuf=NULL;
               sbuf=  g_strsplit_set (aline,":",-1);
                if( g_strv_length(sbuf) <2)
                {
                    kvmap->key = g_locale_to_utf8(sbuf[0],-1,NULL,NULL,NULL);
                    kvmap->value = g_locale_to_utf8("0",-1,NULL,NULL,NULL);
                }
                else
                {
                    kvmap->key = g_locale_to_utf8(sbuf[0],-1,NULL,NULL,NULL);
                    kvmap->value = g_locale_to_utf8(sbuf[1],-1,NULL,NULL,NULL);
                }
              fsel->list  =  g_list_append(fsel->list ,kvmap);
              g_strfreev(sbuf);
        }
        else{   // 2013-3-20 lcy  这里把每一项结构体数据放进链表.

         gchar ** sbuf=NULL;
        if(aline[0] == '/' || aline[0] == '#' || !strlen(aline))
           continue;
         FactoryStructItem *item = g_new0(FactoryStructItem,1);
      //  sscanf(&aline,"%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]",sbuf[0],sbuf[1],sbuf[2],sbuf[3],sbuf[4],sbuf[5]);
       sbuf=  g_strsplit_set (filetxt,":",-1);

       if( g_strv_length(sbuf) <MAX_SECTION)
        continue;

        item->itemType = g_locale_to_utf8(sbuf[0],-1,NULL,NULL,NULL);
        item->itemName = g_locale_to_utf8(sbuf[1],-1,NULL,NULL,NULL);
        item->itemCname = g_locale_to_utf8(sbuf[2],-1,NULL,NULL,NULL);
        item->itemValue = g_locale_to_utf8(sbuf[3],-1,NULL,NULL,NULL);
        item->itemMin = g_locale_to_utf8(sbuf[4],-1,NULL,NULL,NULL);
        item->itemMax = g_locale_to_utf8(sbuf[5],-1,NULL,NULL,NULL);
        item->itemComment = g_locale_to_utf8(sbuf[6],-1,NULL,NULL,NULL);
        datalist = g_list_append(datalist,item);
        g_strfreev(sbuf);
        }

    }
    fclose(fd);

    allstructlist->enumList = enumlist;
    allstructlist->structList = structlist;

}


DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "STRUCT",
			    _("Unified Modelling Language diagram objects STRUCT 1.3"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;
    factoryReadDataFromFile(&structList);
   object_register_type(&structclass_type);

  return DIA_PLUGIN_INIT_OK;
}


PropEnumData _struct_visibilities[] = {
  { N_("Public"), STRUCT_PUBLIC },
  { N_("Private"), STRUCT_PRIVATE },
  { N_("Protected"), STRUCT_PROTECTED },
  { N_("Implementation"), STRUCT_IMPLEMENTATION },
  { NULL, 0 }
};

PropEnumData _struct_inheritances[] = {
  { N_("Abstract"), STRUCT_ABSTRACT },
  { N_("Polymorphic (virtual)"), STRUCT_POLYMORPHIC },
  { N_("Leaf (final)"), STRUCT_LEAF },
  { NULL, 0 }
};

