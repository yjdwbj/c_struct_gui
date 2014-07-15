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

#define _BSD_SOURCE 1 /* to get the prototype for fchmod() from sys/stat.h */
#define _POSIX_C_SOURCE 2 /* so we get fdopen declared even when compiling with -ansi */
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h> /* g_access() and friends */
#include <errno.h>

#ifndef W_OK
#define W_OK 2
#endif

#include "intl.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include "dia_xml_libxml.h"
#include "dia_xml.h"

#include "load_save.h"
#include "group.h"
#include "diagramdata.h"
#include "object.h"
#include "message.h"
#include "preferences.h"
#include "diapagelayout.h"
#include "autosave.h"
#include "newgroup.h"

#ifdef G_OS_WIN32
#include <io.h>
#endif
extern FactoryStructItemAll *factoryContainer;
static void read_connections(GList *objects, xmlNodePtr layer_node,
                             GHashTable *objects_hash);
static void GHFuncUnknownObjects(gpointer key,
                                 gpointer value,
                                 gpointer user_data);
static GList *read_objects(xmlNodePtr objects,
                           GHashTable *objects_hash,
                           const char *filename, DiaObject *parent,
                           GHashTable *unknown_objects_hash);
static void hash_free_string(gpointer       key,
                             gpointer       value,
                             gpointer       user_data);
static xmlNodePtr find_node_named (xmlNodePtr p, const char *name);
static gboolean diagram_data_load(const char *filename, DiagramData *data,
                                  void* user_data);
static gboolean write_objects(GList *objects, xmlNodePtr objects_node,
                              GHashTable *objects_hash, int *obj_nr,
                              const char *filename);
static gboolean write_connections(GList *objects, xmlNodePtr layer_node,
                                  GHashTable *objects_hash);
static xmlDocPtr diagram_data_write_doc(DiagramData *data, const char *filename);
//static int diagram_data_raw_save(DiagramData *data, const char *filename);
static int diagram_data_save(DiagramData *data, const char *filename);


static void
GHFuncUnknownObjects(gpointer key,
                     gpointer value,
                     gpointer user_data)
{
    GString* s = (GString*)user_data;
    g_string_append(s, "\n");
    g_string_append(s, (gchar*)key);
    g_free(key);
}

//static GList *factory_read_template_objects()

/**
 * Recursive function to read objects from a specific level in the xml.
 *
 * Nowadays there are quite a few of them :
 * - Layers : a diagram may have different layers, but this function does *not*
 *   add the created objects to them as it does not know on which nesting level it
 *   is called. So the topmost caller must add the returned objects to the layer.
 * - Groups : groups in itself can have an arbitrary nesting level including other
 *   groups or objects or both of them. A group not containing any objects is by
 *   definition useless. So it is not created. This is to avoid trouble with some older
 *   diagrams which happen to be saved with empty groups.
 * - Parents : if the parent relations would have been there from the beginning of
 *   Dias file format they probably would have been added as nesting level
 *   themselves. But to maintain forward compatibility (allow to let older versions
 *   of Dia to see as much as possible) they were added all on the same level and
 *   the parent child relation is reconstructed from additional attributes.
 */
static GList *
read_objects(xmlNodePtr objects,
             GHashTable *objects_hash,const char *filename, DiaObject *parent,
             GHashTable *unknown_objects_hash)
{
    GList *list;
    DiaObjectType *type;
    DiaObject *obj;
    ObjectNode obj_node;
    char *typestr;
    char *versionstr;
    char *idd;
    char *objname;
    int version;
    xmlNodePtr child_node;

    list = NULL;

    obj_node = objects->xmlChildrenNode;

    while ( obj_node != NULL)
    {
        if (xmlIsBlankNode(obj_node))
        {
            obj_node = obj_node->next;
            continue;
        }

        if (!obj_node) break;

        if (xmlStrcmp(obj_node->name, (const xmlChar *)"object")==0)
        {
            typestr = (char *) xmlGetProp(obj_node, (const xmlChar *)"type");
            versionstr = (char *) xmlGetProp(obj_node, (const xmlChar *)"version");
            idd = (char *) xmlGetProp(obj_node, (const xmlChar *)"id");
//            objname = (gchar*)xmlGetProp(obj_node,(const xmlChar*)"name");
//            if(objname)
//            {
//                if(factory_is_special_object(objname)) /*特殊控件不需要加载的*/
//                {
//                    obj_node = obj_node->next;
//                    continue;
//                }
//            }
            version = 0;
            gchar **split  = g_strsplit(versionstr,"@",-1);
            if(g_strv_length(split)!=2)
            {
                factory_critical_error_exit(factory_utf8(_("版本格式不正信息,样列: 工程号@主版本号．次版本号　!\n")));
            }

            gchar **mvers = g_strsplit(split[1],".",-1);
            if(g_strv_length(mvers) != 2  )
            {
                factory_critical_error_exit(factory_utf8(_("结构体文件的版本错误!找不到有效版本号.")));
            }

            if (g_ascii_strcasecmp(mvers[0],factoryContainer->major_version))
            {
                //	version = atoi(versionstr);
                factory_critical_error_exit(factory_utf8(_("主版本号不一致!无法打开.")));
            }

            if(g_ascii_strcasecmp(mvers[1],factoryContainer->minor_version))
            {
                int sminor = g_strtod(factoryContainer->minor_version,NULL);
                int rminor = g_strtod(mvers[1],NULL);
                if(rminor > sminor)
                {
                    factory_critical_error_exit(factory_utf8(_("当前打开的文件版本，高于当前控件的版本号.")));
                }
            }
            g_strfreev(mvers);
            g_strfreev(split);
            xmlFree(versionstr);

            type = object_get_type((char *)typestr);

            if (!type)
            {
                if (g_utf8_validate (typestr, -1, NULL) &&
                        NULL == g_hash_table_lookup(unknown_objects_hash, typestr))
                    g_hash_table_insert(unknown_objects_hash, g_strdup(typestr), 0);
            }
            else
            {
                obj = type->ops->load(obj_node, version, filename);
                objname = (gchar*)xmlGetProp(obj_node,(const xmlChar*)"name");
                if(objname && (strlen(objname) > 1)) /*不会加载它,只是用它读取数据*/
                {
                    if(factory_is_special_object(objname) ||
                            factory_is_system_data(objname)) /*特殊控件不需要加载的*/
                    {
                        obj_node = obj_node->next;
                        continue;
                    }
                }

                if(obj->isTemplate)
                {
                    obj_node = obj_node->next;
                    continue;
                }

                list = g_list_append(list, obj);
                if (parent)
                {
                    obj->parent = parent;
                    parent->children = g_list_append(parent->children, obj);
                }

                g_hash_table_insert(objects_hash, g_strdup((char *)idd), obj);

                child_node = obj_node->children;

                while(child_node)
                {
                    if (xmlStrcmp(child_node->name, (const xmlChar *)"children") == 0)
                    {
                        GList *children_read = read_objects(child_node, objects_hash, filename, obj, unknown_objects_hash);
                        list = g_list_concat(list, children_read);
                        break;
                    }
                    child_node = child_node->next;
                }
            }
            if (typestr) xmlFree(typestr);
            if (idd) xmlFree (idd);
        }
        else if (xmlStrcmp(obj_node->name, (const xmlChar *)"group")==0
                 && obj_node->children)
        {
            /* don't create empty groups */
            GList *inner_objects = read_objects(obj_node, objects_hash, filename, NULL, unknown_objects_hash);

            if (inner_objects)
            {
                obj = group_create(inner_objects);

#ifdef USE_NEWGROUP
                /* Old group objects had objects recursively inside them.  Since
                 * objects are now done with parenting, we need to extract those objects,
                 * make a newgroup object, set parent-child relationships, and add
                 * all of them to the diagram.
                 */
                {
                    DiaObject *newgroup;
                    GList *objects;
                    Point lower_right;
                    type = object_get_type("Misc - NewGroup");
                    lower_right = obj->position;
                    newgroup = type->ops->create(&lower_right,
                                                 NULL,
                                                 NULL,
                                                 NULL);
                    list = g_list_append(list, newgroup);

                    for (objects = group_objects(obj); objects != NULL; objects = g_list_next(objects))
                    {
                        DiaObject *subobj = (DiaObject *) objects->data;
                        list = g_list_append(list, subobj);
                        subobj->parent = newgroup;
                        newgroup->children = g_list_append(newgroup->children, subobj);
                        if (subobj->bounding_box.right > lower_right.x)
                        {
                            lower_right.x = subobj->bounding_box.right;
                        }
                        if (subobj->bounding_box.bottom > lower_right.y)
                        {
                            lower_right.y = subobj->bounding_box.bottom;
                        }
                    }
                    newgroup->ops->move_handle(newgroup, newgroup->handles[7],
                                               &lower_right, NULL,
                                               HANDLE_MOVE_CREATE_FINAL, 0);
                    /* We've used the info from the old group, destroy it */
                    group_destroy_shallow(obj);
                }
#else
                list = g_list_append(list, obj);
#endif
            }
        }
        else
        {
            /* silently ignore other nodes */
        }

        obj_node = obj_node->next;
    }
    return list;
}

static void
read_connections(GList *objects, xmlNodePtr layer_node,
                 GHashTable *objects_hash)
{
    ObjectNode obj_node;
    GList *list;
    xmlNodePtr connections;
    xmlNodePtr connection;
    char *handlestr;
    char *tostr;
    char *connstr;
    int handle, conn;
    DiaObject *to;

    list = objects;
    obj_node = layer_node->xmlChildrenNode;
    while ((list != NULL) && (obj_node != NULL))
    {
        DiaObject *obj = (DiaObject *) list->data;

        /* the obj and there node must stay in sync to properly setup connections */
        while (obj_node && (xmlIsBlankNode(obj_node) || XML_COMMENT_NODE == obj_node->type))
            obj_node = obj_node->next;
        if (!obj_node) break;

        if IS_GROUP(obj)
        {
            read_connections(group_objects(obj), obj_node, objects_hash);
        }
        else
        {
            gboolean broken = FALSE;
            /* an invalid bounding box is a good sign for some need of corrections */
            gboolean wants_update = obj->bounding_box.right >= obj->bounding_box.left
                                    || obj->bounding_box.top >= obj->bounding_box.bottom;
            connections = obj_node->xmlChildrenNode;
            while ((connections!=NULL) &&
                    (xmlStrcmp(connections->name, (const xmlChar *)"connections")!=0))
                connections = connections->next;
            if (connections != NULL)
            {
                connection = connections->xmlChildrenNode;
                while (connection != NULL)
                {
                    char *donestr;
                    if (xmlIsBlankNode(connection))
                    {
                        connection = connection->next;
                        continue;
                    }
                    handlestr = (char * )xmlGetProp(connection, (const xmlChar *)"handle");
                    tostr = (char *) xmlGetProp(connection, (const xmlChar *)"to");
                    connstr = (char *) xmlGetProp(connection, (const xmlChar *)"connection");

                    handle = atoi(handlestr);
                    conn = strtol(connstr, &donestr, 10);
                    if (*donestr != '\0')   /* Didn't convert it all -- use string */
                    {
                        conn = -1;
                    }

                    to = g_hash_table_lookup(objects_hash, tostr);

                    if (to == NULL)
                    {
                        message_error(_("Error loading diagram.\n"
                                        "Linked object not found in document."));
                        broken = TRUE;
                    }
                    else if (handle < 0 || handle >= obj->num_handles)
                    {
                        message_error(_("Error loading diagram.\n"
                                        "connection handle %d does not exist on '%s'."),
                                      handle, to->type->name);
                        broken = TRUE;
                    }
                    else
                    {
                        if (conn == -1)   /* Find named connpoint */
                        {
                            int i;
                            for (i = 0; i < to->num_connections; i++)
                            {
                                if (to->connections[i]->name != NULL &&
                                        strcmp(to->connections[i]->name, connstr) == 0)
                                {
                                    conn = i;
                                    break;
                                }
                            }
                        }
                        if (conn >= 0 && conn < to->num_connections)
                        {
                            object_connect(obj, obj->handles[handle],
                                           to->connections[conn]);
                            /* force an update on the connection, helpful with (incomplete) generated files */
                            if (wants_update)
                            {
                                obj->handles[handle]->pos =
                                    to->connections[conn]->last_pos = to->connections[conn]->pos;
#if 0
                                obj->ops->move_handle(obj, obj->handles[handle], &to->connections[conn]->pos,
                                                      to->connections[conn], HANDLE_MOVE_CONNECTED,0);
#endif
                            }
                        }
                        else
                        {
                            message_error(_("Error loading diagram.\n"
                                            "connection point %d does not exist on '%s'."),
                                          conn, to->type->name);
                            broken = TRUE;
                        }
                    }

                    if (handlestr) xmlFree(handlestr);
                    if (tostr) xmlFree(tostr);
                    if (connstr) xmlFree(connstr);

                    connection = connection->next;
                }
                /* Fix positions of the connection object for (de)generated files.
                 * Only done for the last point connected otherwise the intermediate posisitions
                 * may screw the auto-routing algorithm.
                 */
                if (!broken && obj && obj->ops->set_props && wants_update)
                {
                    /* called for it's side-effect of update_data */
                    obj->ops->move(obj,&obj->position);

                    for (handle = 0; handle < obj->num_handles; ++handle)
                    {
                        if (obj->handles[handle]->connected_to)
                            obj->ops->move_handle(obj, obj->handles[handle], &obj->handles[handle]->pos,
                                                  obj->handles[handle]->connected_to, HANDLE_MOVE_CONNECTED,0);
                    }
                }
            }
        }

        /* Now set up parent relationships. */
        connections = obj_node->xmlChildrenNode;
        while ((connections!=NULL) &&
                (xmlStrcmp(connections->name, (const xmlChar *)"childnode")!=0))
            connections = connections->next;
        if (connections != NULL)
        {
            tostr = (char *)xmlGetProp(connections, (const xmlChar *)"parent");
            if (tostr)
            {
                obj->parent = g_hash_table_lookup(objects_hash, tostr);
                if (obj->parent == NULL)
                {
                    message_error(_("Can't find parent %s of %s object\n"),
                                  tostr, obj->type->name);
                }
                else
                {
                    obj->parent->children = g_list_prepend(obj->parent->children, obj);
                }
            }
        }

        list = g_list_next(list);
        obj_node = obj_node->next;
    }
}

static void
hash_free_string(gpointer       key,
                 gpointer       value,
                 gpointer       user_data)
{
    g_free(key);
}

static xmlNodePtr
find_node_named (xmlNodePtr p, const char *name)
{
    while (p && 0 != xmlStrcmp(p->name, (xmlChar *)name))
        p = p->next;
    return p;
}


GList* factory_template_load_only(const char *filename)
{
    GHashTable *objects_hash;
    int fd;
    GList *list;
    xmlDocPtr doc;
    xmlNodePtr diagramdata;
    xmlNodePtr paperinfo, gridinfo, guideinfo;
    xmlNodePtr layer_node;
    AttributeNode attr;
    xmlNsPtr namespace;
    gchar firstchar;
    GHashTable* unknown_objects_hash = g_hash_table_new(g_str_hash, g_str_equal);

    if (g_file_test (filename, G_FILE_TEST_IS_DIR))
    {
        message_error(_("You must specify a file, not a directory.\n"));
        return FALSE;
    }
    fd = g_open(filename, O_RDONLY, 0);
    if (fd==-1)
    {
        message_error(_("Couldn't open: '%s' for reading.\n"),
                      dia_message_filename(filename));
        return FALSE;
    }
    /* Note that this closing and opening means we can't read from a pipe */
    close(fd);
    doc = xmlDiaParseFile(filename);
    if (doc == NULL)
    {
        message_error(_("Error loading diagram %s.\nUnknown file type."),
                      dia_message_filename(filename));
        return FALSE;
    }

    if (doc->xmlRootNode == NULL)
    {
        message_error(_("Error loading diagram %s.\nUnknown file type."),
                      dia_message_filename(filename));
        xmlFreeDoc (doc);
        return FALSE;
    }
    namespace = xmlSearchNs(doc, doc->xmlRootNode, (const xmlChar *)"dia");
    if (xmlStrcmp (doc->xmlRootNode->name, (const xmlChar *)"diagram") || (namespace == NULL))
    {
        message_error(_("Error loading diagram %s.\nNot a Dia file."),
                      dia_message_filename(filename));
        xmlFreeDoc (doc);
        return FALSE;
    }
    layer_node =
        find_node_named (doc->xmlRootNode->xmlChildrenNode, "layer");
    objects_hash = g_hash_table_new(g_str_hash, g_str_equal);
    while (layer_node != NULL)
    {
        gchar *name;
        char *visible;
        char *active;

        if (xmlIsBlankNode(layer_node))
        {
            layer_node = layer_node->next;
            continue;
        }

        if (!layer_node) break;


        /* Read in all objects: */

        list = read_objects(layer_node, objects_hash,
                            filename, NULL, unknown_objects_hash);

        layer_node = layer_node->next;
    }

    return list;
}


static gboolean
diagram_data_load(const char *filename, DiagramData *data, void* user_data)
{
    GHashTable *objects_hash;
    int fd;
    GList *list;
    xmlDocPtr doc;
    xmlNodePtr diagramdata;
    xmlNodePtr paperinfo, gridinfo, guideinfo;
    xmlNodePtr layer_node;
    AttributeNode attr;
    Layer *layer;
    xmlNsPtr namespace;
    gchar firstchar;
    Diagram *diagram = DIA_IS_DIAGRAM (data) ? DIA_DIAGRAM (data) : NULL;
    Layer *active_layer = NULL;
    GHashTable* unknown_objects_hash = g_hash_table_new(g_str_hash, g_str_equal);

    if (g_file_test (filename, G_FILE_TEST_IS_DIR))
    {
        message_error(_("You must specify a file, not a directory.\n"));
        return FALSE;
    }

    fd = g_open(filename, O_RDONLY, 0);

    if (fd==-1)
    {
        message_error(_("Couldn't open: '%s' for reading.\n"),
                      dia_message_filename(filename));
        return FALSE;
    }

    if (read(fd, &firstchar, 1))
    {
        data->is_compressed = (firstchar != '<'); // 2014-3-22 lcy 这里读取第一个字符，看它是不是尖括号来检测是不是一个压缩过的文件。
    }
    else
    {
        /* Couldn't read a single char?  Set to default. */
        data->is_compressed = 0 /* prefs.new_diagram.compress_save*/;
    }

    /* Note that this closing and opening means we can't read from a pipe */
    close(fd);

    doc = xmlDiaParseFile(filename);

    if (doc == NULL)
    {
        message_error(_("Error loading diagram %s.\nUnknown file type."),
                      dia_message_filename(filename));
        return FALSE;
    }

    if (doc->xmlRootNode == NULL)
    {
        message_error(_("Error loading diagram %s.\nUnknown file type."),
                      dia_message_filename(filename));
        xmlFreeDoc (doc);
        return FALSE;
    }

    namespace = xmlSearchNs(doc, doc->xmlRootNode, (const xmlChar *)"dia");
    if (xmlStrcmp (doc->xmlRootNode->name, (const xmlChar *)"diagram") || (namespace == NULL))
    {
        message_error(_("Error loading diagram %s.\nNot a Dia file."),
                      dia_message_filename(filename));
        xmlFreeDoc (doc);
        return FALSE;
    }

    /* Destroy the default layer: */
    g_ptr_array_remove(data->layers, data->active_layer);
    layer_destroy(data->active_layer);

    diagramdata =
        find_node_named (doc->xmlRootNode->xmlChildrenNode, "diagramdata");

    /* Read in diagram data: */
    data->bg_color = prefs.new_diagram.bg_color;
    attr = composite_find_attribute(diagramdata, "background");
    if (attr != NULL)
        data_color(attribute_first_data(attr), &data->bg_color);


    /* 读回之前设置的四种颜色 */
    FactoryColors *color = factoryContainer->color;
    attr = composite_find_attribute(diagramdata, "color_edited");
    if(attr != NULL)
        data_color(attribute_first_data(attr), &color->color_edited);
    else
        color->color_edited = color_edited;


    attr = composite_find_attribute(diagramdata, "color_highlight");
    if(attr != NULL)
        data_color(attribute_first_data(attr), &color->color_highlight);
    else
        color->color_highlight = color_highlight;


    attr = composite_find_attribute(diagramdata, "color_foreground");
    if(attr != NULL)
        data_color(attribute_first_data(attr), &color->color_foreground);
    else
        color->color_foreground = color_black;

    attr = composite_find_attribute(diagramdata, "color_background");
    if(attr != NULL)
        data_color(attribute_first_data(attr), &color->color_background);
    else
        color->color_background = color_white;


    if (diagram)
    {
        diagram->pagebreak_color = prefs.new_diagram.pagebreak_color;
        attr = composite_find_attribute(diagramdata, "pagebreak");
        if (attr != NULL)
            data_color(attribute_first_data(attr), &diagram->pagebreak_color);
    }
    /* load paper information from diagramdata section */
    attr = composite_find_attribute(diagramdata, "paper");
    if (attr != NULL)
    {
        paperinfo = attribute_first_data(attr);

        attr = composite_find_attribute(paperinfo, "name");
        if (attr != NULL)
        {
            g_free(data->paper.name);
            data->paper.name = data_string(attribute_first_data(attr));
        }
        if (data->paper.name == NULL || data->paper.name[0] == '\0')
        {
            data->paper.name = g_strdup(prefs.new_diagram.papertype);
        }
        /* set default margins for paper size ... */
        dia_page_layout_get_default_margins(data->paper.name,
                                            &data->paper.tmargin,
                                            &data->paper.bmargin,
                                            &data->paper.lmargin,
                                            &data->paper.rmargin);

        attr = composite_find_attribute(paperinfo, "tmargin");
        if (attr != NULL)
            data->paper.tmargin = data_real(attribute_first_data(attr));
        attr = composite_find_attribute(paperinfo, "bmargin");
        if (attr != NULL)
            data->paper.bmargin = data_real(attribute_first_data(attr));
        attr = composite_find_attribute(paperinfo, "lmargin");
        if (attr != NULL)
            data->paper.lmargin = data_real(attribute_first_data(attr));
        attr = composite_find_attribute(paperinfo, "rmargin");
        if (attr != NULL)
            data->paper.rmargin = data_real(attribute_first_data(attr));

        attr = composite_find_attribute(paperinfo, "is_portrait");
        data->paper.is_portrait = TRUE;
        if (attr != NULL)
            data->paper.is_portrait = data_boolean(attribute_first_data(attr));

        attr = composite_find_attribute(paperinfo, "scaling");
        data->paper.scaling = 1.0;
        if (attr != NULL)
            data->paper.scaling = data_real(attribute_first_data(attr));

        attr = composite_find_attribute(paperinfo, "fitto");
        data->paper.fitto = FALSE;
        if (attr != NULL)
            data->paper.fitto = data_boolean(attribute_first_data(attr));

        attr = composite_find_attribute(paperinfo, "fitwidth");
        data->paper.fitwidth = 1;
        if (attr != NULL)
            data->paper.fitwidth = data_int(attribute_first_data(attr));

        attr = composite_find_attribute(paperinfo, "fitheight");
        data->paper.fitheight = 1;
        if (attr != NULL)
            data->paper.fitheight = data_int(attribute_first_data(attr));

        /* calculate effective width/height */
        dia_page_layout_get_paper_size(data->paper.name,
                                       &data->paper.width,
                                       &data->paper.height);
        if (!data->paper.is_portrait)
        {
            gfloat tmp = data->paper.width;

            data->paper.width = data->paper.height;
            data->paper.height = tmp;
        }
        data->paper.width -= data->paper.lmargin;
        data->paper.width -= data->paper.rmargin;
        data->paper.height -= data->paper.tmargin;
        data->paper.height -= data->paper.bmargin;
        data->paper.width /= data->paper.scaling;
        data->paper.height /= data->paper.scaling;
    }

    if (diagram)
    {
        attr = composite_find_attribute(diagramdata, "grid");
        if (attr != NULL)
        {
            gridinfo = attribute_first_data(attr);

            attr = composite_find_attribute(gridinfo, "width_x");
            if (attr != NULL)
                diagram->grid.width_x = data_real(attribute_first_data(attr));
            attr = composite_find_attribute(gridinfo, "width_y");
            if (attr != NULL)
                diagram->grid.width_y = data_real(attribute_first_data(attr));
            attr = composite_find_attribute(gridinfo, "visible_x");
            if (attr != NULL)
                diagram->grid.visible_x = data_int(attribute_first_data(attr));
            attr = composite_find_attribute(gridinfo, "visible_y");
            if (attr != NULL)
                diagram->grid.visible_y = data_int(attribute_first_data(attr));

            diagram->grid.colour = prefs.new_diagram.grid_color;
            attr = composite_find_attribute(diagramdata, "color");
            if (attr != NULL)
                data_color(attribute_first_data(attr), &diagram->grid.colour);
        }
    }
    if (diagram)
    {
        attr = composite_find_attribute(diagramdata, "guides");
        if (attr != NULL)
        {
            guint i;
            DataNode guide;

            guideinfo = attribute_first_data(attr);

            attr = composite_find_attribute(guideinfo, "hguides");
            if (attr != NULL)
            {
                diagram->guides.nhguides = attribute_num_data(attr);
                g_free(diagram->guides.hguides);
                diagram->guides.hguides = g_new(real, diagram->guides.nhguides);

                guide = attribute_first_data(attr);
                for (i = 0; i < diagram->guides.nhguides; i++, guide = data_next(guide))
                    diagram->guides.hguides[i] = data_real(guide);
            }
            attr = composite_find_attribute(guideinfo, "vguides");
            if (attr != NULL)
            {
                diagram->guides.nvguides = attribute_num_data(attr);
                g_free(diagram->guides.vguides);
                diagram->guides.vguides = g_new(real, diagram->guides.nvguides);

                guide = attribute_first_data(attr);
                for (i = 0; i < diagram->guides.nvguides; i++, guide = data_next(guide))
                    diagram->guides.vguides[i] = data_real(guide);
            }
        }
    }

    /* Read in all layers: */
    layer_node =
        find_node_named (doc->xmlRootNode->xmlChildrenNode, "layer");

    objects_hash = g_hash_table_new(g_str_hash, g_str_equal);

    while (layer_node != NULL)
    {
        gchar *name;
        char *visible;
        char *active;

        if (xmlIsBlankNode(layer_node))
        {
            layer_node = layer_node->next;
            continue;
        }

        if (!layer_node) break;

        name = (char *)xmlGetProp(layer_node, (const xmlChar *)"name");
        if (!name) break; /* name is mandatory */

        layer = new_layer(g_strdup(name), data);
        if (name) xmlFree(name);

        layer->visible = FALSE;
        visible = (char *)xmlGetProp(layer_node, (const xmlChar *)"visible");
        if (visible)
        {
            if (strcmp(visible, "true")==0)
                layer->visible = TRUE;
            xmlFree(visible);
            factoryContainer->curLayer = layer;
        }
        /* Read in all objects: */

        list = read_objects(layer_node, objects_hash, filename, NULL, unknown_objects_hash);
        layer_add_objects (layer, list);
        read_connections( list, layer_node, objects_hash);

        data_add_layer(data, layer);

        active = (char *)xmlGetProp(layer_node, (const xmlChar *)"active");
        if (active)
        {
            if (strcmp(active, "true")==0)
                active_layer = layer;
            xmlFree(active);
        }

        layer_node = layer_node->next;
    }

    if (!active_layer)
        data->active_layer = (Layer *) g_ptr_array_index(data->layers, 0);
    else
        data_set_active_layer(data, active_layer);

    xmlFreeDoc(doc);

    g_hash_table_foreach(objects_hash, hash_free_string, NULL);

    g_hash_table_destroy(objects_hash);

    if (data->layers->len < 1)
    {
        message_error (_("Error loading diagram:\n%s.\n"
                         "A valid Dia file defines at least one layer."),
                       dia_message_filename(filename));
        return FALSE;
    }
    else if (0 < g_hash_table_size(unknown_objects_hash))
    {
        GString*    unknown_str = g_string_new("Unknown types while reading diagram file");

        /* show all the unknown types in one message */
        g_hash_table_foreach(unknown_objects_hash,
                             GHFuncUnknownObjects,
                             unknown_str);
        message_warning("%s", unknown_str->str);
        g_string_free(unknown_str, TRUE);
    }
    g_hash_table_destroy(unknown_objects_hash);

    return TRUE;
}

static gboolean
write_objects(GList *objects, xmlNodePtr objects_node,
              GHashTable *objects_hash, int *obj_nr, const char *filename)
{
    char buffer[31];
    ObjectNode obj_node;
    xmlNodePtr group_node;
    GList *list;
//    gboolean istemplate = g_str_has_suffix(filename,".lcy"); /* 用保存文件后缀名来判断是否要保存为模版 */
    list = objects;
    while (list != NULL)
    {
        DiaObject *obj = (DiaObject *) list->data;

        if (g_hash_table_lookup(objects_hash, obj))
        {
            list = g_list_next(list);
            continue;
        }

        if (IS_GROUP(obj) && group_objects(obj) != NULL)
        {
            group_node = xmlNewChild(objects_node, NULL, (const xmlChar *)"group", NULL);
            write_objects(group_objects(obj), group_node,
                          objects_hash, obj_nr, filename);
        }
        else
        {
            obj_node = xmlNewChild(objects_node, NULL, (const xmlChar *)"object", NULL);
            xmlSetProp(obj_node, (const xmlChar *)"type", (xmlChar *)obj->type->name);

//      g_snprintf(buffer, 30, "%d", obj->type->version);
            /* 写入自定义的唯一长串版本号 */
            gchar *fversion = g_strdup_printf("%s@%s.%s",factoryContainer->project_number,factoryContainer->major_version,
                                              factoryContainer->minor_version);
            xmlSetProp(obj_node, (const xmlChar *)"version", (xmlChar *)fversion);
            g_free(fversion);



            g_snprintf(buffer, 30, "O%d", *obj_nr);
            xmlSetProp(obj_node, (const xmlChar *)"id", (xmlChar *)buffer);
            xmlSetProp(obj_node, (const xmlChar *)"name", (xmlChar *)obj->name); /* 添加名字来确认那些个节点用不用加载　*/

            if(obj->isTemplate)
            {
                xmlSetProp(obj_node,(const xmlChar *)"templ",(xmlChar*)"1");
            }
            else
                xmlSetProp(obj_node,(const xmlChar *)"templ",(xmlChar*)"0");

            (*obj->type->ops->save)(obj, obj_node, filename);

            /* Add object -> obj_nr to hash table */
            g_hash_table_insert(objects_hash, obj, GINT_TO_POINTER(*obj_nr));
            (*obj_nr)++;

            /*
            if (object_flags_set(obj, DIA_OBJECT_CAN_PARENT) && obj->children)
            {
            int res;
            xmlNodePtr parent_node;
                   parent_node = xmlNewChild(obj_node, NULL, "children", NULL);
                   res = write_objects(obj->children, parent_node, objects_hash, obj_nr, filename);
            if (!res) return FALSE;
                 }
                 */
        }

        list = g_list_next(list);
    }
//    if(!istemplate)
//    {
//        objects = g_list_remove(objects,fileobj);
//        fileobj->ops->destroy(fileobj);
//        objects = g_list_remove(objects,idobj);
//        idobj->ops->destroy(idobj);
//        objects = g_list_remove(objects,sysinfoobj);
//        sysinfoobj->ops->destroy(sysinfoobj);
//    }
    return TRUE;
}

/* Parents broke assumption that both objects and xml nodes are laid out
 * linearly.
 */

static gboolean
write_connections(GList *objects, xmlNodePtr layer_node,
                  GHashTable *objects_hash)
{
    ObjectNode obj_node;
    GList *list;
    xmlNodePtr connections;
    xmlNodePtr connection;
    char buffer[31];
    int i;

    list = objects;
    obj_node = layer_node->xmlChildrenNode;
    while ((list != NULL) && (obj_node != NULL))
    {
        DiaObject *obj = (DiaObject *) list->data;

        while (obj_node && xmlIsBlankNode(obj_node)) obj_node = obj_node->next;
        if (!obj_node) break;

        if IS_GROUP(obj)
        {
            write_connections(group_objects(obj), obj_node, objects_hash);
        }
        else
        {
            connections = NULL;

            for (i=0; i<obj->num_handles; i++)
            {
                ConnectionPoint *con_point;
                Handle *handle;

                handle = obj->handles[i];
                con_point = handle->connected_to;

                if ( con_point != NULL )
                {
                    DiaObject *other_obj;
                    int con_point_nr;

                    other_obj = con_point->object;

                    con_point_nr=0;
                    while (other_obj->connections[con_point_nr] != con_point)
                    {
                        con_point_nr++;
                        if (con_point_nr>=other_obj->num_connections)
                        {
                            message_error("Internal error saving diagram\n con_point_nr >= other_obj->num_connections\n");
                            return FALSE;
                        }
                    }

                    if (connections == NULL)
                        connections = xmlNewChild(obj_node, NULL, (const xmlChar *)"connections", NULL);

                    connection = xmlNewChild(connections, NULL, (const xmlChar *)"connection", NULL);
                    /* from what handle on this object*/
                    g_snprintf(buffer, 30, "%d", i);
                    xmlSetProp(connection, (const xmlChar *)"handle", (xmlChar *)buffer);
                    /* to what object */
                    g_snprintf(buffer, 30, "O%d",
                               GPOINTER_TO_INT(g_hash_table_lookup(objects_hash,
                                               other_obj)));

                    xmlSetProp(connection, (const xmlChar *)"to", (xmlChar *) buffer);
                    /* to what connection_point on that object */
                    if (other_obj->connections[con_point_nr]->name != NULL)
                    {
                        g_snprintf(buffer, 30, "%s", other_obj->connections[con_point_nr]->name);
                    }
                    else
                    {
                        g_snprintf(buffer, 30, "%d", con_point_nr);
                    }
                    xmlSetProp(connection, (const xmlChar *)"connection", (xmlChar *) buffer);
                }
            }
        }

        if (obj->parent)
        {
            DiaObject *other_obj = obj->parent;
            xmlNodePtr parent;
            g_snprintf(buffer, 30, "O%d",
                       GPOINTER_TO_INT(g_hash_table_lookup(objects_hash, other_obj)));
            parent = xmlNewChild(obj_node, NULL, (const xmlChar *)"childnode", NULL);
            xmlSetProp(parent, (const xmlChar *)"parent", (xmlChar *)buffer);
        }

        list = g_list_next(list);
        obj_node = obj_node->next;
    }
    return TRUE;
}

/* Filename seems to be junk, but is passed on to objects */
static xmlDocPtr
diagram_data_write_doc(DiagramData *data, const char *filename)
{
    xmlDocPtr doc;
    xmlNodePtr tree;
    xmlNodePtr pageinfo, gridinfo, guideinfo;
    xmlNodePtr layer_node;
    GHashTable *objects_hash;
    gboolean res;
    int obj_nr;
    guint i;
    Layer *layer;
    AttributeNode attr;
    xmlNs *name_space;
    Diagram *diagram = DIA_IS_DIAGRAM (data) ? DIA_DIAGRAM (data) : NULL;

    doc = xmlNewDoc((const xmlChar *)"1.0");
    doc->encoding = xmlStrdup((const xmlChar *)"UTF-8");

    doc->xmlRootNode = xmlNewDocNode(doc, NULL, (const xmlChar *)"diagram", NULL);

    name_space = xmlNewNs(doc->xmlRootNode,
                          (const xmlChar *)DIA_XML_NAME_SPACE_BASE,
                          (const xmlChar *)"dia");
    xmlSetNs(doc->xmlRootNode, name_space);

    tree = xmlNewChild(doc->xmlRootNode, name_space, (const xmlChar *)"diagramdata", NULL);

    attr = new_attribute((ObjectNode)tree, "background");
    data_add_color(attr, &data->bg_color);
    /* 2014-6-24 lcy 保存选择的颜色*/
    FactoryColors *color = factoryContainer->color;
    attr = new_attribute((ObjectNode)tree, "color_edited");
    data_add_color(attr,&color->color_edited);

    attr = new_attribute((ObjectNode)tree, "color_highlight");
    data_add_color(attr,&color->color_highlight);

    attr = new_attribute((ObjectNode)tree, "color_foreground");
    data_add_color(attr,&color->color_foreground);

    attr = new_attribute((ObjectNode)tree, "color_background");
    data_add_color(attr,&color->color_background);


    /**/


    if (diagram)
    {
        attr = new_attribute((ObjectNode)tree, "pagebreak");
        data_add_color(attr, &diagram->pagebreak_color);
    }
    attr = new_attribute((ObjectNode)tree, "paper");
    pageinfo = data_add_composite(attr, "paper");
    data_add_string(composite_add_attribute(pageinfo, "name"),
                    data->paper.name);
    data_add_real(composite_add_attribute(pageinfo, "tmargin"),
                  data->paper.tmargin);
    data_add_real(composite_add_attribute(pageinfo, "bmargin"),
                  data->paper.bmargin);
    data_add_real(composite_add_attribute(pageinfo, "lmargin"),
                  data->paper.lmargin);
    data_add_real(composite_add_attribute(pageinfo, "rmargin"),
                  data->paper.rmargin);
    data_add_boolean(composite_add_attribute(pageinfo, "is_portrait"),
                     data->paper.is_portrait);
    data_add_real(composite_add_attribute(pageinfo, "scaling"),
                  data->paper.scaling);
    data_add_boolean(composite_add_attribute(pageinfo, "fitto"),
                     data->paper.fitto);
    if (data->paper.fitto)
    {
        data_add_int(composite_add_attribute(pageinfo, "fitwidth"),
                     data->paper.fitwidth);
        data_add_int(composite_add_attribute(pageinfo, "fitheight"),
                     data->paper.fitheight);
    }

    if (diagram)
    {
        attr = new_attribute((ObjectNode)tree, "grid");
        gridinfo = data_add_composite(attr, "grid");
        data_add_real(composite_add_attribute(gridinfo, "width_x"),
                      diagram->grid.width_x);
        data_add_real(composite_add_attribute(gridinfo, "width_y"),
                      diagram->grid.width_y);
        data_add_int(composite_add_attribute(gridinfo, "visible_x"),
                     diagram->grid.visible_x);
        data_add_int(composite_add_attribute(gridinfo, "visible_y"),
                     diagram->grid.visible_y);
        attr = new_attribute((ObjectNode)tree, "color");
        data_add_composite(gridinfo, "color");
        data_add_color(attr, &diagram->grid.colour);

        attr = new_attribute((ObjectNode)tree, "guides");
        guideinfo = data_add_composite(attr, "guides");
        attr = composite_add_attribute(guideinfo, "hguides");
        for (i = 0; i < diagram->guides.nhguides; i++)
            data_add_real(attr, diagram->guides.hguides[i]);
        attr = composite_add_attribute(guideinfo, "vguides");
        for (i = 0; i < diagram->guides.nvguides; i++)
            data_add_real(attr, diagram->guides.vguides[i]);
    }

    objects_hash = g_hash_table_new(g_direct_hash, g_direct_equal);

    obj_nr = 0;

    for (i = 0; i < data->layers->len; i++)
    {
        layer_node = xmlNewChild(doc->xmlRootNode, name_space, (const xmlChar *)"layer", NULL);
        layer = (Layer *) g_ptr_array_index(data->layers, i);
        xmlSetProp(layer_node, (const xmlChar *)"name", (xmlChar *)layer->name);

        if (layer->visible)
            xmlSetProp(layer_node, (const xmlChar *)"visible", (const xmlChar *)"true");
        else
            xmlSetProp(layer_node, (const xmlChar *)"visible", (const xmlChar *)"false");

        if (layer == data->active_layer)
            xmlSetProp(layer_node, (const xmlChar *)"active", (const xmlChar *)"true");

        write_objects(layer->objects, layer_node,
                      objects_hash, &obj_nr, filename);

        res = write_connections(layer->objects, layer_node, objects_hash);
        /* Why do we bail out like this?  It leaks! */
        if (!res)
            return NULL;
    }
    g_hash_table_destroy(objects_hash);

    if (data->is_compressed)
        xmlSetDocCompressMode(doc, 9);
    else
        xmlSetDocCompressMode(doc, 0);

    return doc;
}

/** This tries to save the diagram into a file, without any backup
 * Returns >= 0 on success.
 * Only for internal use. */
int
diagram_data_raw_save(DiagramData *data, const char *filename)
{
    xmlDocPtr doc;
    int ret;

    doc = diagram_data_write_doc(data, filename);

    ret = xmlDiaSaveFile (filename, doc);
    xmlFreeDoc(doc);

    return ret;
}


int factory_project_raw_save(DiagramData *data)
{
     xmlDocPtr doc;
    gchar *datadir = dia_get_data_directory("data");
    int ret;
    /* 这里最终要保存生成bin文件的 */
    gchar *newfname = g_strconcat(datadir,G_DIR_SEPARATOR_S,"project.dia",NULL);
    int i =0;
    Layer *layer;
    for (i = 0; i < data->layers->len; i++)
    {

        layer = (Layer *) g_ptr_array_index(data->layers, i);
        GList *olist = layer->objects;
        for(;olist ; olist = olist->next)
        {
            DiaObject *diaobj = olist->data;
            if(diaobj->isTemplate)
            {

            }
        }

    }


    doc = diagram_data_write_doc(data, newfname);

    ret = xmlDiaSaveFile (newfname, doc);
    xmlFreeDoc(doc);
    g_free(newfname);
    g_free(datadir);

    return ret;
}

/** This saves the diagram, using a backup in case of failure.
 * @param data
 * @param filename
 * @returns TRUE on successfull save, FALSE otherwise.  If a failure is
 * indicated, an error message will already have been given to the user.
 */
static int
diagram_data_save(DiagramData *data, const char *user_filename)
{
    FILE *file;
    char *bakname=NULL,*tmpname=NULL,*dirname=NULL,*pth;
    char *filename = (char *)user_filename;
    int mode,_umask;
    int fildes;
    int ret = 0;

    /* Once we depend on GTK 2.8+, we can use these tests. */
#if GLIB_CHECK_VERSION(2,8,0) && !defined G_OS_WIN32
    /* Check that we're allowed to write to the target file at all. */
    /* not going to work with 'My Docments' - read-only but still useable, see bug #504469 */
    if (   g_file_test(filename, G_FILE_TEST_EXISTS)
            && g_access(filename, W_OK) != 0)
    {
        message_error(_("Not allowed to write to output file %s\n"),
                      dia_message_filename(filename));
        goto CLEANUP;
    }
#endif

    if (g_file_test(user_filename, G_FILE_TEST_IS_SYMLINK))
    {
        GError *error = NULL;
        filename = g_file_read_link(user_filename, &error);
        if (!filename)
        {
            message_error("%s", error->message);
            g_error_free(error);
            goto CLEANUP;
        }
    }

    /* build the temporary and backup file names */
    dirname = g_strdup(filename);
    pth = strrchr((char *)dirname,G_DIR_SEPARATOR);
    if (pth)
    {
        *(pth+1) = 0;
    }
    else
    {
        g_free(dirname);
        dirname = g_strdup("." G_DIR_SEPARATOR_S);
    }
    tmpname = g_strconcat(dirname,"__diaXXXXXX",NULL);
    bakname = g_strconcat(filename,"~",NULL);

#if GLIB_CHECK_VERSION(2,8,0) && !defined G_OS_WIN32
    /* Check that we can create the other files */
    if (   g_file_test(dirname, G_FILE_TEST_EXISTS)
            && g_access(dirname, W_OK) != 0)
    {
        message_error(_("Not allowed to write temporary files in %s\n"),
                      dia_message_filename(dirname));
        goto CLEANUP;
    }
#endif

    /* open a temporary name, and fix the modes to match what fopen() would have
       done (mkstemp() is (rightly so) a bit paranoid for what we do).  */
    fildes = g_mkstemp(tmpname);
    /* should not be necessary anymore on *NIXas well, because we are using g_mkstemp ? */
    _umask = umask(0);
    umask(_umask);
    mode = 0666 & ~_umask;
#ifndef G_OS_WIN32
    ret = fchmod(fildes,mode);
#else
    ret = 0; /* less paranoia on windoze */
#endif
    file = fdopen(fildes,"wb");

    /* Now write the data in the temporary file name. */

    if (file==NULL)
    {
        message_error(_("Can't open output file %s: %s\n"),
                      dia_message_filename(tmpname), strerror(errno));
        goto CLEANUP;
    }
    fclose(file);
//    if(data->isProject)
//    {
    GList *curlist = data->active_layer->objects;
    DiaObject *fileobj,*idobj,*sysinfoobj;

    /* 这里要创建一个用来　File.lst 的文件的结构体,保存完了就要删掉的*/
    DiaObjectType *otype = object_get_type(CLASS_STRUCT);
    FactoryStructItemList *fsil= g_hash_table_lookup(factoryContainer->structTable,TYPE_FILELST);
    Point startpoint = {0.0,0.0};
    Handle *h1,*h2;
    fileobj = otype->ops->create(&startpoint,(void*)fsil->number,&h1,&h2);

    FactoryStructItemList *fsil2= g_hash_table_lookup(factoryContainer->structTable,TYPE_IDLST);
    idobj = otype->ops->create(&startpoint,(void*)fsil2->number,&h1,&h2);

    FactoryStructItemList *fsil3= g_hash_table_lookup(factoryContainer->structTable,TYPE_SYSDATA);
    sysinfoobj = otype->ops->create(&startpoint,(void*)fsil3->number,&h1,&h2);

    fileobj->name = g_strdup(fsil->sname);
    idobj->name = g_strdup(fsil2->sname);
    sysinfoobj->name = g_strdup(fsil3->sname);

    curlist = g_list_append(curlist,fileobj);
    curlist = g_list_append(curlist,idobj);
    curlist = g_list_append(curlist,sysinfoobj);

    ret = diagram_data_raw_save(data, filename);

//    ret = factory_project_raw_save(data);

    curlist = g_list_remove(curlist,fileobj);
    curlist = g_list_remove(curlist,idobj);
    curlist = g_list_remove(curlist,sysinfoobj);

    /* 上面是删掉一些不需要要界面上以控件形式显示的控件.这就是特殊控件 */

    if (ret < 0)
    {
        /* Save failed; we clean our stuff up, without touching the file named
           "filename" if it existed. */
        message_error(_("Internal error %d writing file %s\n"),
                      ret, dia_message_filename(tmpname));
        g_unlink(tmpname);
        goto CLEANUP;
    }
    /* save succeeded. We kill the old backup file, move the old file into
       backup, and the temp file into the new saved file. */
//    g_unlink(bakname);
//    g_rename(filename,bakname);
//    ret = g_rename(tmpname,filename);
//    if (ret < 0)
//    {
//        message_error(_("Can't rename %s to final output file %s: %s\n"),
//                      dia_message_filename(filename),
//                      dia_message_filename(filename), strerror(errno));
//    }
CLEANUP:
    if (filename != user_filename)
        g_free(filename);
    g_free(tmpname);
    g_free(dirname);
    g_free(bakname);
    /*这里添加生成BIN文件*/
    gchar *exefile = dia_get_lib_directory("bin");
    gchar *fullpath = g_strconcat(exefile,G_DIR_SEPARATOR_S"makebin.exe",NULL);

    g_return_if_fail(factory_test_file_exist(fullpath));

    gchar *input = g_strdup_printf("-i=%s",user_filename);
    int n = strlen(user_filename);
    gchar *newfile = g_strdup(user_filename);
    newfile[n-1] = 'n';
    newfile[n-2] = 'i';
    newfile[n-3] = 'b';
    gchar *outfile = g_strdup_printf("-o=%s",newfile);
    /* 转换本地码,不然会有乱码的 */
    gchar *argv[] = {fullpath,input,outfile,NULL};

    g_spawn_sync(NULL,
                 argv,NULL,
                 G_SPAWN_LEAVE_DESCRIPTORS_OPEN,
                 NULL,NULL,
                 NULL,NULL,NULL,NULL);

//    g_spawn_async_with_pipes(dia_get_lib_directory("bin"),
//                             argv,NULL,G_SPAWN_SEARCH_PATH,NULL,NULL,&pid,NULL,NULL,NULL,NULL);

    g_free(outfile);
    g_free(newfile);
    g_free(input);
    g_free(exefile);

    return (ret?FALSE:TRUE);
}

void factory_call_isd_download()
{

    gchar *filename = g_strdup(ddisplay_active_diagram()->filename);
//    factory_debug_to_log(g_strdup_printf(factory_utf8("下载文件到小机,文件名:%s.\n"),filename));
    g_return_if_fail(filename);
    gchar *path = strrchr((char *)filename,G_DIR_SEPARATOR);
    if(path)
        *(path+1) = 0;
//    gchar *filename = g_get_current_dir();
    g_return_if_fail(factoryContainer);

    GList *dlist = factoryContainer->fgdn_func(filename); /* 取得最新的download 文件列表*/
//    g_return_if_fail(dlist);
    int len = 0;
    if(dlist)
        len = g_list_length(dlist);
    gchar *isdownload_gui = g_strconcat(dia_get_lib_directory("bin"),G_DIR_SEPARATOR_S, "isdownload_gui.exe",NULL);
    gchar *isdownload = g_strconcat(dia_get_lib_directory("bin"),G_DIR_SEPARATOR_S ,"isd_download.exe",NULL);
    gchar *datapath = dia_get_lib_directory("data"); /* 系统文件所在目录 */
    g_return_if_fail(factory_test_file_exist(isdownload));
    g_return_if_fail(factory_test_file_exist(isdownload_gui));
    gchar **system_files = g_strsplit(factoryContainer->system_files,",",-1);
    int slen = g_strv_length(system_files);
    len +=slen;
    gchar **argv = g_new(gchar*, len+1);

    int n = 0;
    argv[n] =g_strdup(factory_locale(isdownload_gui));
    for(; n < slen; n++)
    {
        argv[n+1] = g_strconcat(datapath,G_DIR_SEPARATOR_S,system_files[n],NULL);
    }
    g_strfreev(system_files);
    GList *tlist = dlist;
    for(; tlist; tlist=tlist->next,n++)
    {
        argv[n+1] = g_strdup(tlist->data);
    }
    argv[n+1] = 0;

    g_spawn_sync(filename,
                 argv,NULL,
                 G_SPAWN_SEARCH_PATH,
                 NULL,NULL,
                 NULL,NULL,NULL,NULL);
//    GPid pid;
//    g_spawn_async_with_pipes(filename,
//                             argv,NULL,G_SPAWN_SEARCH_PATH,NULL,NULL,&pid,NULL,NULL,NULL,NULL);
//    for(; dlist; dlist = dlist->next)
//    {
//        GFile *dst = g_file_new_for_path(g_strconcat(filename,dlist->data,NULL));
//        g_file_delete(dst,NULL,NULL);
//    }
    g_free(isdownload_gui);
    g_free(isdownload);
    g_free(filename);
    g_free(datapath);
//    g_strfreev(argv);
}


void factory_load_all_templates(void)
{
    gchar*   thedir = dia_get_data_directory("template");
    GDir *dp;
    const char *dentry;
    gchar *p;

    dp = g_dir_open(thedir, 0, NULL);
    if (!dp) return;

    while ( (dentry = g_dir_read_name(dp)) )
    {
        gchar *filename = g_strconcat(thedir,G_DIR_SEPARATOR_S,
                                      dentry,NULL);

        if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
        {
            g_free(filename);
            continue;
        }

        /* take only .sheet files */
        if(g_str_has_suffix(filename,LCY))
        {
             factory_template_open_template_filename(filename);
        }

        g_free(filename);

    }

    g_dir_close(dp);
}


int
diagram_save(Diagram *dia, const char *filename)
{
    gboolean res;
    if(dia->isTemplate)
    {
        FactoryTemplateItem *fti = dia->templ_item;
        res = fti->templ_ops->templ_save(fti->fsil);
    }
    else
    {
        res = diagram_data_save(dia->data, filename);
        if(dia->data->readytodownload) /* 这里是下载到小机器的标志 */
        {
            dia->data->readytodownload = FALSE;
            factory_call_isd_download();
        }
    }

    if (!res)
    {
        return res;
    }

    dia->unsaved = FALSE;
    undo_mark_save(dia->undo);
    diagram_set_modified (dia, FALSE);

    diagram_cleanup_autosave(dia);
    return TRUE;
}

/* Autosave stuff.  Needs to use low-level save to avoid setting and resetting flags */
void
diagram_cleanup_autosave(Diagram *dia)
{
    gchar *savefile;
    struct stat statbuf;

    savefile = dia->autosavefilename;
    if (savefile == NULL) return;
#ifdef TRACES
    g_print("Cleaning up autosave %s for %s\n",
            savefile, dia->filename ? dia->filename : "<no name>");
#endif
    if (g_stat(savefile, &statbuf) == 0)   /* Success */
    {
        g_unlink(savefile);
    }
    g_free(savefile);
    dia->autosavefilename = NULL;
    dia->autosaved = FALSE;
}

/** Absolutely autosave a diagram.
 * Called after a periodic check at the first idleness.
 */
void
diagram_autosave(Diagram *dia)
{
    gchar *save_filename;

    /* Must check if the diagram is still valid, or Death Ensues! */
    GList *diagrams = dia_open_diagrams();
    Diagram *diagram;
    while (diagrams != NULL)
    {
        diagram = (Diagram *)diagrams->data;
        if (diagram == dia &&
                diagram_is_modified(diagram) &&
                !diagram->autosaved)
        {
            save_filename = g_strdup_printf("%s.autosave", dia->filename);

            if (dia->autosavefilename != NULL)
                g_free(dia->autosavefilename);
            dia->autosavefilename = save_filename;
            diagram_data_raw_save(dia->data, save_filename);
            dia->autosaved = TRUE;
            return;
        }
        diagrams = g_list_next(diagrams);
    }
}

/* --- filter interfaces --- */
static void
export_native(DiagramData *data, const gchar *filename,
              const gchar *diafilename, void* user_data)
{
    diagram_data_save(data, filename);
}

static const gchar *extensions[] = { "dia", NULL };
static const gchar *lcy_extensions[] = { "lcy", NULL };
DiaExportFilter dia_export_filter =
{
    N_("Dia Diagram File"),
    extensions,
    export_native
};
DIAVAR  DiaImportFilter dia_import_filter =
{
    N_("Dia Diagram File"),
    extensions,
    diagram_data_load
};

DIAVAR  DiaImportFilter lcy_import_filter =
{
    N_("lcy define Template File"),
    lcy_extensions,
    diagram_data_load
};
