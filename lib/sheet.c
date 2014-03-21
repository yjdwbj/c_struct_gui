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

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <glib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include "dia_xml_libxml.h"
#include <string.h>

#include "intl.h"

#include "sheet.h"
#include "message.h"
#include "object.h"
#include "dia_dirs.h"

FactoryStructItemAll structList = {NULL,NULL};
static GSList *sheets = NULL;

Sheet *
new_sheet(char *name, gchar *description, char *filename, SheetScope scope,
          Sheet *shadowing)
{
  Sheet *sheet;

  sheet = g_new(Sheet, 1);

  sheet->name = g_strdup(name);
  sheet->description = g_strdup(description);

  sheet->filename = filename;
  sheet->scope = scope;
  sheet->shadowing = shadowing;
  sheet->objects = NULL;
  return sheet;
}

void
sheet_prepend_sheet_obj(Sheet *sheet, SheetObject *obj)
{
  DiaObjectType *type;

  type = object_get_type(obj->object_type);
  if (type == NULL) {
    message_warning(_("DiaObject '%s' needed in sheet '%s' was not found.\n"
		      "It will not be available for use."),
		    obj->object_type, sheet->name);
  } else {
    sheet->objects = g_slist_prepend( sheet->objects, (gpointer) obj);
  }
}

void
sheet_append_sheet_obj(Sheet *sheet, SheetObject *obj)
{
  DiaObjectType *type;

  type = object_get_type(obj->object_type);
  if (type == NULL) {
    message_warning(_("DiaObject '%s' needed in sheet '%s' was not found.\n"
		      "It will not be available for use."),
		      obj->object_type, sheet->name);
  } else {
    sheet->objects = g_slist_append( sheet->objects, (gpointer) obj);
  }
}

void
register_sheet(Sheet *sheet)
{
  sheets = g_slist_append(sheets, (gpointer) sheet);
}

GSList *
get_sheets_list(void)
{
  return sheets;
}

/* Sheet file management */

static void load_sheets_from_dir(const gchar *directory, SheetScope scope);
static void load_register_sheet(const gchar *directory,const gchar *filename,
                                SheetScope scope);

/** Sort the list of sheets by *locale*.
 */
static gint
dia_sheet_sort_callback(gconstpointer a, gconstpointer b)
{
  return g_utf8_collate(gettext( ((Sheet *)(a))->name ),
			gettext( ((Sheet *)(b))->name ));
}

void
dia_sort_sheets(void)
{
  sheets = g_slist_sort(sheets, dia_sheet_sort_callback);
}

void
load_all_sheets(void)
{
  char *sheet_path;
  char *home_dir;
    factoryReadDataFromFile(&structList);
//  home_dir = dia_config_filename("sheets");
//  if (home_dir) {
//    dia_log_message ("sheets from '%s'", home_dir);
//    load_sheets_from_dir(home_dir, SHEET_SCOPE_USER);
//    g_free(home_dir);
//  }
//
//  sheet_path = getenv("DIA_SHEET_PATH");
//  if (sheet_path) {
//    char **dirs = g_strsplit(sheet_path,G_SEARCHPATH_SEPARATOR_S,0);
//    int i;
//
//    for (i=0; dirs[i] != NULL; i++) {
//      dia_log_message ("sheets from '%s'", dirs[i]);
//      load_sheets_from_dir(dirs[i], SHEET_SCOPE_SYSTEM);
//    }
//    g_strfreev(dirs);
//  }
//  else
  {
    char *thedir = dia_get_data_directory("sheets");
    dia_log_message ("sheets from '%s'", thedir);
    load_sheets_from_dir(thedir, SHEET_SCOPE_SYSTEM);
    g_free(thedir);
  }

  /* Sorting their sheets alphabetically makes user merging easier */

  dia_sort_sheets();
}

static void
load_sheets_from_dir(const gchar *directory, SheetScope scope)
{
  GDir *dp;
  const char *dentry;
  gchar *p;

  dp = g_dir_open(directory, 0, NULL);
  if (!dp) return;

  while ( (dentry = g_dir_read_name(dp)) ) {
    gchar *filename = g_strconcat(directory,G_DIR_SEPARATOR_S,
				  dentry,NULL);

    if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
      g_free(filename);
      continue;
    }

    /* take only .sheet files */
    p = filename + strlen(filename) - 6 /* strlen(".sheet") */;
    if (0!=strncmp(p,".sheet",6)) {
      g_free(filename);
      continue;
    }

    load_register_sheet(directory, filename, scope);
    g_free(filename);

  }

  g_dir_close(dp);
}


void factoryReadDataFromFile(FactoryStructItemAll *allstructlist)
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
    GList *datalist = NULL;
    GList  *structlist = NULL;
    GList *enumlist = NULL;

//     gchar *sbuf[MAX_SECTION];   // 2014-3-19 lcy 这里分几个段
    gboolean isEmnu = FALSE;
    gboolean isStruct = FALSE;

    FactoryStructEnumList *fsel = NULL;
    FactoryStructItemList *fssl = NULL;
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
                fsel = g_new0(FactoryStructEnumList,1);
                fsel->name = g_locale_to_utf8(sbuf[2],-1,NULL,NULL,NULL);
                fsel->list = NULL;

            }
            else if( 0 == strncmp("Struct",sbuf[1],6)) // 2014-3-20 lcy 这里匹配到结构体名字.
            {
                isStruct = TRUE;
                fssl = g_new0(FactoryStructItemList,1);
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
                allstructlist->structList = g_list_append(allstructlist->structList,fssl);
            }
            else{
                isEmnu = FALSE;
                allstructlist->enumList = g_list_append(allstructlist->enumList,fsel);
            }
        }
        else if(isEmnu) // 2014-3-19 lcy 读取一个枚举.
        {
               if(aline[0] == '/' || aline[0] == '#' || !strlen(aline))
                continue;
               FactoryStructEnum *kvmap  = g_new0(FactoryStructEnum,1);
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
        fssl->list = g_list_append(fssl->list,item);
        g_strfreev(sbuf);
        }

    }
    fclose(fd);
}

static void
load_register_sheet(const gchar *dirname, const gchar *filename,
                    SheetScope scope)
{
  xmlDocPtr doc;
  xmlNsPtr ns;
  xmlNodePtr node, contents,subnode,root;
  xmlChar *tmp;
  gchar *name = NULL, *description = NULL;
  int name_score = -1;
  int descr_score = -1;
  Sheet *sheet = NULL;
  GSList *sheetp;
  gboolean set_line_break = FALSE;
  gboolean name_is_gmalloced = FALSE;
  Sheet *shadowing = NULL;
  Sheet *shadowing_sheet = NULL;

  /* the XML fun begins here. */

  doc = xmlDoParseFile(filename);
  if (!doc) return;
  root = doc->xmlRootNode;
  while (root && (root->type != XML_ELEMENT_NODE)) root=root->next;
  if (!root) return;
  if (xmlIsBlankNode(root)) return;

  if (!(ns = xmlSearchNsByHref(doc,root, (const xmlChar *)
	   DIA_XML_NAME_SPACE_BASE "dia-sheet-ns"))) {
    g_warning("could not find sheet namespace");
    xmlFreeDoc(doc);
    return;
  }
    if ((root->ns != ns) || (xmlStrcmp(root->name, (const xmlChar *)"sheet"))) {
    g_warning("root element was %s -- expecting sheet",
              doc->xmlRootNode->name);
    xmlFreeDoc(doc);
    return;
  }

  contents = NULL;
  for (node = root->xmlChildrenNode; node != NULL; node = node->next) {
    if (xmlIsBlankNode(node)) continue;
    if (node->type != XML_ELEMENT_NODE)
      continue;

    if (node->ns == ns && !xmlStrcmp(node->name, (const xmlChar *)"name")) {
      gint score;

      /* compare the xml:lang property on this element to see if we get a
       * better language match.  LibXML seems to throw away attribute
       * namespaces, so we use "lang" instead of "xml:lang" */
      /* Now using the C locale for internal sheet names, instead gettexting
       * when the name is used in the menus.  Not going to figure out the
       * XML lang system more than absolutely necessary now.   --LC
       */
      /*
      tmp = xmlGetProp(node, "xml:lang");
      if (!tmp) tmp = xmlGetProp(node, "lang");
      */
      score = intl_score_locale("C");
      /*
      if (tmp) xmlFree(tmp);
      */

      if (name_score < 0 || score < name_score) {
        name_score = score;
        if (name) xmlFree(name);
        name = (char *) xmlNodeGetContent(node);
      }
    } else if (node->ns == ns && !xmlStrcmp(node->name, (const xmlChar *)"description")) {
      gint score;

      /* compare the xml:lang property on this element to see if we get a
       * better language match.  LibXML seems to throw away attribute
       * namespaces, so we use "lang" instead of "xml:lang" */
      tmp = xmlGetProp(node, (const xmlChar *)"xml:lang");
      if (!tmp) tmp = xmlGetProp(node, (const xmlChar *)"lang");
      score = intl_score_locale((char *) tmp);
      if (tmp) xmlFree(tmp);

      if (descr_score < 0 || score < descr_score) {
        descr_score = score;
        if (description) xmlFree(description);
        description = (char *) xmlNodeGetContent(node);
      }

    } else if (node->ns == ns && !xmlStrcmp(node->name, (const xmlChar *)"contents")) {
      contents = node;
    }
  }

  if (!name || !contents) {
    g_warning("No <name> and/or <contents> in sheet %s--skipping", filename);
    xmlFreeDoc(doc);
    if (name) xmlFree(name);
    if (description) xmlFree(description);
    return;
  }

  /* Notify the user when we load a sheet that appears to be an updated
     version of a sheet loaded previously (i.e. from ~/.dia/sheets). */
#ifdef DEBUG
    int n = g_list_length(sheets);
#endif

  sheetp = get_sheets_list();
  while (sheetp)
  {
    if (sheetp->data && !strcmp(((Sheet *)(sheetp->data))->name, name))
    {
      struct stat first_file, this_file;
      int stat_ret;

      stat_ret = stat(((Sheet *)(sheetp->data))->filename, &first_file);
      g_assert(!stat_ret);

      stat_ret = stat(filename, &this_file);
      g_assert(!stat_ret);

      if (this_file.st_mtime > first_file.st_mtime)
      {
        gchar *tmp = g_strdup_printf("%s [Copy of system]", name);
        message_notice(_("The system sheet '%s' appears to be more recent"
                         " than your custom\n"
                         "version and has been loaded as '%s' for this session."
                         "\n\n"
                         "Move new objects (if any) from '%s' into your custom"
                         " sheet\n"
                         "or remove '%s', using the 'Sheets and Objects' dialog."),
                         name, tmp, tmp, tmp);
        xmlFree(name);
        name = tmp;
        name_is_gmalloced = TRUE;
        shadowing = sheetp->data;  /* This copy-of-system sheet shadows
                                      a user sheet */
      }
      else
      {
        /* The already-created user sheet shadows this sheet (which will be
           invisible), but we don't know this sheet's address yet */
        shadowing_sheet = sheetp->data;
      }
    }
    sheetp = g_slist_next(sheetp);
  }

  sheet = new_sheet(name, description, g_strdup(filename), scope, shadowing);

  if (shadowing_sheet)
    shadowing_sheet->shadowing = sheet;                   /* Hilarious :-) */

  if (name_is_gmalloced == TRUE)
    g_free(name);
  else
    xmlFree(name);
  xmlFree(description);

  for (node = contents->xmlChildrenNode ; node != NULL; node = node->next) {  // 2014-3-20 lcy 超长的for循环.
    SheetObject *sheet_obj;
    DiaObjectType *otype;
    gchar *iconname = NULL;

    int subdesc_score = -1;
    gchar *objdesc = NULL;

    gint intdata = 0;
    gchar *chardata = NULL;

    gboolean has_intdata = FALSE;
    gboolean has_icon_on_sheet = FALSE;

    if (xmlIsBlankNode(node)) continue;

    if (node->type != XML_ELEMENT_NODE)
      continue;
    if (node->ns != ns) continue;
    if (!xmlStrcmp(node->name, (const xmlChar *)"object")) {
      /* nothing */
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"shape")) {
      g_message(_("%s: you should use object tags rather than shape tags now"),
                filename);
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"br")) {
      /* Line break tag. */
      set_line_break = TRUE;
      continue;
    } else
      continue; /* unknown tag */

    tmp = xmlGetProp(node, (const xmlChar *)"intdata");
    if (tmp) {
      char *p;
      intdata = (gint)strtol((char *) tmp,&p,0);
      if (*p != 0) intdata = 0;
      xmlFree(tmp);
      has_intdata = TRUE;
    }
    chardata = (gchar *) xmlGetProp(node, (const xmlChar *)"chardata");
    /* TODO.... */
    if (chardata) xmlFree(chardata);

    for (subnode = node->xmlChildrenNode;
         subnode != NULL ;
         subnode = subnode->next) {
      if (xmlIsBlankNode(subnode)) continue;

      if (subnode->ns == ns && !xmlStrcmp(subnode->name, (const xmlChar *)"description")) {
	gint score;

	/* compare the xml:lang property on this element to see if we get a
	 * better language match.  LibXML seems to throw away attribute
	 * namespaces, so we use "lang" instead of "xml:lang" */

	tmp = xmlGetProp(subnode, (xmlChar *)"xml:lang");
	if (!tmp) tmp = xmlGetProp(subnode, (xmlChar *)"lang");
	score = intl_score_locale((char *) tmp);
	if (tmp) xmlFree(tmp);

	if (subdesc_score < 0 || score < subdesc_score) {
	  subdesc_score = score;
	  if (objdesc) free(objdesc);
	  objdesc = (gchar *) xmlNodeGetContent(subnode);
	}

      } else if (subnode->ns == ns && !xmlStrcmp(subnode->name, (const xmlChar *)"icon")) {
          tmp = xmlNodeGetContent(subnode);
          iconname = g_strconcat(dirname,G_DIR_SEPARATOR_S, (char *) tmp,NULL);
	  if(!shadowing_sheet && !g_file_test (iconname, G_FILE_TEST_EXISTS))
          {
	   /* Fall back to system directory if there is no user icon */
            gchar *sheetdir = dia_get_data_directory("sheets");
            iconname = g_strconcat(sheetdir,G_DIR_SEPARATOR_S, (char *) tmp,NULL);
	    g_free(sheetdir);
          }
          has_icon_on_sheet = TRUE;
          if (tmp) xmlFree(tmp);
      }
    }

    tmp = xmlGetProp(node, (xmlChar *)"name");

    GList *slist = structList.structList;

     FactoryStructItemList *fssl = NULL;
    for(;slist != NULL; slist = slist->next) // 2014-3-21 lcy 这里根据结构体个数创那图标.
    {
       fssl = slist->data;
    sheet_obj = g_new(SheetObject,1);
    sheet_obj->object_type = g_strdup((char *) tmp);
    sheet_obj->description = g_strdup(fssl->name);
//    xmlFree(objdesc);     objdesc = NULL;

    sheet_obj->pixmap = NULL;
   // sheet_obj->user_data = GINT_TO_POINTER(intdata); /* XXX modify user_data type ? */
    sheet_obj->user_data = GINT_TO_POINTER(fssl->number);
    sheet_obj->user_data_type = has_intdata ? USER_DATA_IS_INTDATA /* sure,   */
                                            : USER_DATA_IS_OTHER;  /* why not */
    sheet_obj->pixmap_file = iconname;
    sheet_obj->has_icon_on_sheet = has_icon_on_sheet;
    sheet_obj->line_break = set_line_break;
    set_line_break = FALSE;

    if ((otype = object_get_type((char *) tmp)) == NULL) {
      /* Don't complain. This does happen when disabling plug-ins too.
      g_warning("object_get_type(%s) returned NULL", tmp); */
      if (sheet_obj->description) g_free(sheet_obj->description);
      g_free(sheet_obj->pixmap_file);
      g_free(sheet_obj->object_type);
      g_free(sheet_obj);
      if (tmp)
        xmlFree(tmp);
      continue;
    }

    /* set defaults */
    if (sheet_obj->pixmap_file == NULL) {
      g_assert(otype->pixmap || otype->pixmap_file);
      sheet_obj->pixmap = otype->pixmap; // 2014-3-20 lcy 这里是加xpm 的图片.
      sheet_obj->pixmap_file = otype->pixmap_file;
      sheet_obj->has_icon_on_sheet = has_icon_on_sheet;
    }
    if (sheet_obj->user_data == NULL
        && sheet_obj->user_data_type != USER_DATA_IS_INTDATA)
      sheet_obj->user_data = otype->default_user_data;
    else
      sheet_obj->user_data_type = USER_DATA_IS_INTDATA;

   // if (tmp) xmlFree(tmp);

    /* we don't need to fix up the icon and descriptions for simple objects,
       since they don't have their own description, and their icon is
       already automatically handled. */
    sheet_append_sheet_obj(sheet,sheet_obj);

    }
    if (tmp) xmlFree(tmp);
    xmlFree(objdesc);     objdesc = NULL;
  }    // 2014-3-20 lcy 超长的for循环.

  if (!shadowing_sheet)
    register_sheet(sheet);

  xmlFreeDoc(doc);
}

