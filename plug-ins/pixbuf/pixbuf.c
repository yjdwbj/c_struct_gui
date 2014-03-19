/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * pixbuf.c -- GdkPixbuf Exporter writing to various pixbuf supported
 *             bitmap formats.
 *
 * Copyright (C) 2002, 2004 Hans Breuer, <Hans@Breuer.Org>
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "diagdkrenderer.h"
#include "filter.h"
#include "plug-ins.h"
#include "prop_text.h"
#include "prop_geomtypes.h"
#include "object.h"

static Rectangle rect;
static real zoom = 1.0;

static void
export_data(DiagramData *data, const gchar *filename, 
            const gchar *diafilename, void* user_data)
{
  DiaGdkRenderer *renderer;
  GdkColor color;
  int width, height;
  GdkPixbuf* pixbuf = NULL;
  GError* error = NULL;
  const char* format = (const char*)user_data;

  rect.left = data->extents.left;
  rect.top = data->extents.top;
  rect.right = data->extents.right;
  rect.bottom = data->extents.bottom;

  /* quite arbitrary */
  zoom = 20.0 * data->paper.scaling; 
  /* Adding a bit of padding to account for rounding errors.  Better to
   * pad than to clip.  See bug #413275 */
  width = ceil((rect.right - rect.left) * zoom) + 1;
  height = ceil((rect.bottom - rect.top) * zoom) + 1;

  renderer = g_object_new (DIA_TYPE_GDK_RENDERER, NULL);
  renderer->transform = dia_transform_new (&rect, &zoom);
  renderer->pixmap = gdk_pixmap_new(NULL, width, height, gdk_visual_get_system()->depth);
  renderer->gc = gdk_gc_new(renderer->pixmap);

  /* draw background */
  color_convert (&data->bg_color, &color);
  gdk_gc_set_foreground(renderer->gc, &color);
  gdk_draw_rectangle(renderer->pixmap, renderer->gc, 1,
		 0, 0, width, height);

  data_render(data, DIA_RENDERER (renderer), NULL, NULL, NULL);

  pixbuf = gdk_pixbuf_get_from_drawable (NULL, renderer->pixmap, 
                                         gdk_colormap_get_system (),
                                         0, 0, 0, 0,
                                         width, height);
  if (pixbuf)
    {
      gdk_pixbuf_save (pixbuf, filename, format, &error, NULL);
      g_object_unref (pixbuf);
    }
  else
    {
      message_error ("Failed to create pixbuf from drawable.");
    }

  if (error)
    {
      message_warning(_("Could not save file:\n%s\n%s"),
		      dia_message_filename(filename),
                      error->message);
      g_error_free (error);
    }

  g_object_unref (renderer);
}

static gboolean
import_data (const gchar *filename, DiagramData *data, void* user_data)
{
  DiaObjectType *otype = object_get_type("Standard - Image");
  gint width, height;

  if (!otype) /* this would be really broken */
    return FALSE;

  g_assert (user_data);

  if (gdk_pixbuf_get_file_info (filename, &width, &height))
    {
      DiaObject *obj;
      Handle *h1, *h2;
      Point point;
      point.x = point.y = 0.0;

      obj = otype->ops->create(&point, otype->default_user_data, &h1, &h2);
      if (obj)
        {
          PropDescription prop_descs [] = {
            { "image_file", PROP_TYPE_FILE },
            { "elem_width", PROP_TYPE_REAL },
            { "elem_height", PROP_TYPE_REAL },
            PROP_DESC_END};
          GPtrArray *plist = prop_list_from_descs (prop_descs, pdtpp_true);
          StringProperty *strprop = g_ptr_array_index(plist, 0);
          RealProperty *realprop_w  = g_ptr_array_index(plist, 1);
          RealProperty *realprop_h  = g_ptr_array_index(plist, 2);

          strprop->string_data = g_strdup (filename);
          realprop_w->real_data = width / 20.0;
          realprop_h->real_data = height / 20.0;
          obj->ops->set_props(obj, plist);
          prop_list_free (plist);

          layer_add_object(data->active_layer, obj);
          return TRUE;
        }
    }
  else
    {
      message_warning ("Pixbuf[%s] can't load:\n%s", (gchar*)user_data, filename);
    }

  return FALSE;
}

static GList *_import_filters = NULL;
static GList *_export_filters = NULL;

/* --- dia plug-in interface --- */
static gboolean
_plugin_can_unload (PluginInfo *info)
{
  return TRUE;
}

static void
_plugin_unload (PluginInfo *info)
{
  GList *p;

  for (p = _import_filters; p != NULL; p = g_list_next(p)) {
    DiaImportFilter *ifilter = (DiaImportFilter *)p->data;
    filter_unregister_import (ifilter);
    g_free ((gchar*)ifilter->description);
    g_strfreev ((gchar**)ifilter->extensions);
    g_free ((gpointer)ifilter->user_data);
    g_free ((gchar*)ifilter->unique_name);
  }
  g_list_free (_import_filters);
  for (p = _export_filters; p != NULL; p = g_list_next(p)) {
    DiaExportFilter *efilter = p->data;
    filter_unregister_export (efilter);
    g_free ((gchar*)efilter->description);
    g_strfreev ((gchar**)efilter->extensions);
    g_free ((gpointer)efilter->user_data);
    g_free ((gchar*)efilter->unique_name);
  }
  g_list_free (_export_filters);
}

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  /*
   * If GTK is not initialized yet don't register this plug-in. This is
   * almost the same as app_is_interactive() but avoids to make plug-ins
   * depend on Dia's app core function. Also what we really need is a 
   * display, not an interactive app ;)
   */
  if (gdk_display_get_default ()) {
    if (!dia_plugin_info_init(info, "Pixbuf",
			      _("gdk-pixbuf based bitmap export/import"),
			      _plugin_can_unload,
                              _plugin_unload))
      return DIA_PLUGIN_INIT_ERROR;
    else {
      GSList* formats = gdk_pixbuf_get_formats ();
      GSList* sl = formats;

      /* if we get this far we still may be running non-interactive. To avoid complains
       * from color_convert() we are initializing ourselves ;)
       */
     color_init ();

     /*
      * Instead of hard-coding capabilities, ask GdkPixbuf what's installed
      */
     for (sl = formats; sl != NULL; sl = g_slist_next (sl)) {
        GdkPixbufFormat* format = (GdkPixbufFormat*)sl->data;

        if (gdk_pixbuf_format_is_writable (format)) {
          DiaExportFilter* efilter = g_new0 (DiaExportFilter, 1);
          gchar* name;

          name = gdk_pixbuf_format_get_name (format);
          /* the pixbuf desriptions are too generic for Dia's usage, make our own */
          efilter->description = g_strdup_printf ("Pixbuf[%s]", name);
          /* NOT: gdk_pixbuf_format_get_description (format); */
          efilter->extensions = (const gchar**)gdk_pixbuf_format_get_extensions (format);
          efilter->export_func = export_data;
          efilter->user_data = g_strdup (name);
          efilter->unique_name = g_strdup_printf ("pixbuf-%s", (gchar*)efilter->user_data);
          g_free (name);
          _export_filters = g_list_append (_export_filters, efilter);
          filter_register_export(efilter);
        }
        /* there is no write only filter */
        {
          DiaImportFilter* ifilter;
          gchar* name;

          name = gdk_pixbuf_format_get_name (format);
          /* filter out the less useful ones to keep the total list reasonable short 
           * (If anyone complains make it configurable via persistence and this default ;-)
           */
          if (   strcmp (name, "ani") == 0
              || strcmp (name, "ico") == 0
              || strcmp (name, "pcx") == 0
              || strcmp (name, "pnm") == 0
              || strcmp (name, "ras") == 0
              || strcmp (name, "tiff") == 0
              || strcmp (name, "wbmp") == 0
              || strcmp (name, "xbm") == 0)
            {
              g_free (name);
              continue;
            }
	  ifilter = g_new0 (DiaImportFilter, 1);
          /* the pixbuf desriptions are too generic for Dia's usage, make our own */
          ifilter->description = g_strdup_printf ("Pixbuf[%s]", name);
          ifilter->extensions = (const gchar**)gdk_pixbuf_format_get_extensions (format);
          ifilter->import_func = import_data;
          ifilter->user_data = gdk_pixbuf_format_get_name (format);
          /* they are in differnt namespaces aren't they? */
          ifilter->unique_name = g_strdup_printf ("pixbuf-%s", name);
	  /* don't use pixbuf loader for vector formats */
	  if (   strcmp (name, "svg") == 0
	      || strcmp (name, "svgz") == 0
	      || strcmp (name, "wmf") == 0
	      || strcmp (name, "emf") == 0)
	    ifilter->hints = FILTER_DONT_GUESS;
          g_free (name);
          _import_filters = g_list_append (_import_filters, ifilter);
          filter_register_import(ifilter);
        }
      }
      g_slist_free (formats);
    }
  }

  return DIA_PLUGIN_INIT_OK;
}
