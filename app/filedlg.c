/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * filedlg.c: some dialogs for saving/loading/exporting files.
 * Copyright (C) 1999 James Henstridge
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

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <glib/gstdio.h>

#undef GTK_DISABLE_DEPRECATED /* gtk_file_chooser_dialog_new_with_backend */
#include <gtk/gtk.h>
#include "intl.h"
#include "filter.h"
#include "dia_dirs.h"
#include "persistence.h"
#include "display.h"
#include "message.h"
#include "layer_dialog.h"
#include "load_save.h"
#include "preferences.h"
#include "interface.h"
#include "recent_files.h"
#include "confirm.h"

#include "filedlg.h"
#include "object.h"

static GtkWidget *opendlg = NULL;
static GtkWidget *savedlg = NULL;
static GtkWidget *exportdlg = NULL;

static void
toggle_compress_callback(GtkWidget *widget)
{
    /* Changes prefs exactly when the user toggles the setting, i.e.
     * the setting really remembers what the user chose last time, but
     * lets diagrams of the opposite kind stay that way unless the user
     * intervenes.
     */
    prefs.new_diagram.compress_save =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

/**
 * Given an import filter index and optionally a filename for fallback
 * return the import filter to use
 */
static DiaImportFilter *
ifilter_by_index (int index, const char* filename)
{
    DiaImportFilter *ifilter = NULL;

    if (index >= 0)
        ifilter = g_list_nth_data (filter_get_import_filters(), index);
    else if (filename) /* fallback, should not happen */
        ifilter = filter_guess_import_filter(filename);

    return ifilter;
}

typedef void* (* FilterGuessFunc) (const gchar* filename);

/**
 * Respond to the file chooser filter facility, that is match
 * the extension of a given filename to the selected filter
 */
static gboolean
matching_extensions_filter (const GtkFileFilterInfo* fi,
                            gpointer               data)
{
    FilterGuessFunc guess_func = (FilterGuessFunc)data;

    g_assert (guess_func);

    if (!fi->filename)
        return 0; /* filter it, IMO should not happen --hb */

    if (guess_func (fi->filename))
        return 1;

    return 0;
}

/**
 * React on the diagram::removed signal by destroying the dialog
 *
 * This function isn't used cause it conflicts with the pattern introduced:
 * instead of destroying the dialog with the diagram, the dialog is keeping
 * a refernce to it. As a result we could access the diagram even when the
 * display of it is gone ...
 */
#if 0
static void
diagram_removed (Diagram* dia, GtkWidget* dialog)
{
    g_return_if_fail (DIA_IS_DIAGRAM (dia));
    g_return_if_fail (GTK_IS_WIDGET (dialog));

    gtk_widget_destroy (dialog);
}
#endif

static GtkFileFilter *
build_gtk_file_filter_from_index (int index)
{
    DiaImportFilter *ifilter = NULL;
    GtkFileFilter *filter = NULL;

    ifilter = g_list_nth_data (filter_get_import_filters(), index-1);
    if (ifilter)
    {
        GString *pattern = g_string_new ("*.");
        int i = 0;

        filter = gtk_file_filter_new ();

        while (ifilter->extensions[i] != NULL)
        {
            if (i != 0)
                g_string_append (pattern, "|*.");
            g_string_append (pattern, ifilter->extensions[i]);
            ++i;
        }
        gtk_file_filter_set_name (filter, _("Supported Formats"));
        gtk_file_filter_add_pattern (filter, pattern->str);

        g_string_free (pattern, TRUE);

    }
    else
    {
        /* match the other selections extension */
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, _("Supported Formats"));
        gtk_file_filter_add_custom (filter, GTK_FILE_FILTER_FILENAME,
                                    matching_extensions_filter, filter_guess_import_filter, NULL);
    }
    return filter;
}

static void
import_adapt_extension_callback(GtkWidget *widget)
{
    int index = gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
    GtkFileFilter *former = NULL;
    GSList *list, *elem;

    list = gtk_file_chooser_list_filters (GTK_FILE_CHOOSER (opendlg));
    for (elem = list; elem != NULL; elem = g_slist_next (elem))
        if (strcmp (_("Supported Formats"), gtk_file_filter_get_name (GTK_FILE_FILTER(elem->data))) == 0)
            former = GTK_FILE_FILTER(elem->data);
    g_slist_free (list);

    if (former)
    {
        /* replace the previous filter */
        GtkFileFilter *filter = build_gtk_file_filter_from_index (index);
        gtk_file_chooser_remove_filter (GTK_FILE_CHOOSER (opendlg), former);
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (opendlg), filter);
        gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (opendlg), filter);
    }
}

/**
 * Create the combobox menu to select Import Filter options
 */
GtkWidget *
create_open_menu(void)
{
    GtkWidget *menu;
    GList *tmp;


    menu = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(menu), _("By extension"));

    for (tmp = filter_get_import_filters(); tmp != NULL; tmp = tmp->next)
    {
        DiaImportFilter *ifilter = tmp->data;
        gchar *filter_label;

        if (!ifilter)
            continue;
        filter_label = filter_get_import_filter_label(ifilter);
        gtk_combo_box_append_text (GTK_COMBO_BOX(menu), filter_label);
        g_free(filter_label);
    }
    g_signal_connect(GTK_OBJECT(menu), "changed",
                     G_CALLBACK(import_adapt_extension_callback), NULL);
    return menu;
}

/**
    Respond to the user finishing the Template Open Dialog either accept or cancel/destory
*/



static void factory_template_open_response_callback(GtkWidget *fs,
        gint       response,
        gpointer   user_data)
{
    char *filename;
    if (response == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));
        factory_template_open_template_filename(filename);
        g_free (filename);
    }
    gtk_widget_destroy(opendlg);
}


/**
 * Respond to the user finishing the Open Dialog either accept or cancel/destroy
 */
static void
file_open_response_callback(GtkWidget *fs,
                            gint       response,
                            gpointer   user_data)
{
    char *filename;
    Diagram *diagram = NULL;
//    GtkWidget *parent_window =  g_object_get_data(G_OBJECT(opendlg),"parent_window");
    if (response == GTK_RESPONSE_ACCEPT)
    {
        gint index = gtk_combo_box_get_active (GTK_COMBO_BOX(user_data));

        if (index >= 0) /* remember it */
            persistence_set_integer ("import-filter", index);
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));
        diagram = diagram_load(filename, ifilter_by_index (index - 1, filename));
        g_free (filename);

        if (diagram != NULL)
        {
            diagram_update_extents(diagram);
            layer_dialog_set_diagram(diagram);

            if (diagram->displays == NULL)
            {
                /*	GSList *displays = diagram->displays;
                	GSList *displays_head = displays;
                	diagram->displays = NULL;
                	for (; displays != NULL; displays = g_slist_next(displays)) {
                	  DDisplay *loaded_display = (DDisplay *)displays->data;
                	  copy_display(loaded_display);
                	  g_free(loaded_display);
                	}
                	g_slist_free(displays_head);
                      } else {
                */
                new_display(diagram);

            }
        }
        if(diagram && diagram->displays)
        {

            GList *list = g_hash_table_get_values(diagram->data->active_layer->defnames);
            if(list)
            {
                DiaObject *obj = list->data;
                obj->ops->reset_objectsfillcolor(obj);
                diagram_redraw_all();
            }
            /* 把连线的信息读取回来 */
            GList *tlist = list;
            for(; tlist ; tlist = tlist->next)
            {
                DiaObject *obj = tlist->data;
                obj->ops->rec_find_ocbox(obj,NULL,FIND_PTR_LOAD);
            }
        }

    }
    gtk_widget_destroy(fs);
}

void factory_template_open_callback(gpointer data,guint action,GtkWidget *widget)
{
    if (!opendlg)
    {
        DDisplay *ddisp;
        Diagram *dia = NULL;
        GtkWindow *parent_window;
        gchar *filename = NULL;

        /* FIXME: we should not use ddisp_active but instead get the current diagram
         * from caller. Thus we could offer the option to "load into" if invoked by
         * <Display/File/Open. It wouldn't make any sense if invoked by
         * <Toolbox>/File/Open ...
         */
        ddisp = ddisplay_active();
        if (ddisp)
        {
            dia = ddisp->diagram;
            parent_window = GTK_WINDOW(ddisp->shell);
        }
        else
        {
            parent_window = GTK_WINDOW(interface_get_toolbox_shell());
        }
        persistence_register_integer ("import-filter", 0);
        opendlg = gtk_file_chooser_dialog_new_with_backend(_("Open Template"), parent_window,
                  GTK_FILE_CHOOSER_ACTION_OPEN,
                  "default", /* default, not gnome-vfs */
                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                  NULL);
        g_object_set_data(G_OBJECT(opendlg),"parent_window",parent_window);
        gtk_dialog_set_default_response(GTK_DIALOG(opendlg), GTK_RESPONSE_ACCEPT);
        gtk_window_set_role(GTK_WINDOW(opendlg), "open_diagram");
        if (dia && dia->filename)
            filename = g_filename_from_utf8(dia->filename, -1, NULL, NULL, NULL);
        if (filename != NULL)
        {
            char* fnabs = dia_get_absolute_filename (filename);
            if (fnabs)
                gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(opendlg), fnabs);
            g_free(fnabs);
            g_free(filename);
        }
        g_signal_connect(GTK_OBJECT(opendlg), "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &opendlg);
        g_signal_connect(GTK_OBJECT(opendlg),"response",
                         G_CALLBACK(factory_template_open_response_callback),NULL);
    }
    else
    {
        gtk_widget_set_sensitive(opendlg, TRUE);
        if (GTK_WIDGET_VISIBLE(opendlg))
            return;
    }
    if (!gtk_file_chooser_get_extra_widget(GTK_FILE_CHOOSER(opendlg)))
    {

        /* set up the gtk file (name) filters */
        /* 0 = by extension */
        GtkFileFilter* filter;
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, factory_utf8("模版文件"));
        gtk_file_filter_add_pattern (filter, "*.lcy");
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (opendlg), filter);
    }

    gtk_widget_show(opendlg);
}


/**
 * Handle menu click File/Open
 *
 * This is either with or without diagram
 */
void
file_open_callback(gpointer data, guint action, GtkWidget *widget)
{
//    if (!opendlg)
//    {
        GtkWidget *odlg = NULL;
        DDisplay *ddisp;
        Diagram *dia = NULL;
        GtkWindow *parent_window;
        gchar *filename = NULL;

        /* FIXME: we should not use ddisp_active but instead get the current diagram
         * from caller. Thus we could offer the option to "load into" if invoked by
         * <Display/File/Open. It wouldn't make any sense if invoked by
         * <Toolbox>/File/Open ...
         */
        ddisp = ddisplay_active();
        if (ddisp)
        {
            dia = ddisp->diagram;
            parent_window = GTK_WINDOW(ddisp->shell);
        }
        else
        {
            parent_window = GTK_WINDOW(interface_get_toolbox_shell());
        }
        persistence_register_integer ("import-filter", 0);
        odlg = gtk_file_chooser_dialog_new_with_backend(_("Open Diagram"), parent_window,
                  GTK_FILE_CHOOSER_ACTION_OPEN,
                  "default", /* default, not gnome-vfs */
                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                  NULL);
        g_object_set_data(G_OBJECT(odlg),"parent_window",parent_window);
        gtk_dialog_set_default_response(GTK_DIALOG(odlg), GTK_RESPONSE_ACCEPT);
        gtk_window_set_role(GTK_WINDOW(odlg), "open_diagram");
        if (dia && dia->filename)
            filename = g_filename_from_utf8(dia->filename, -1, NULL, NULL, NULL);
        if (filename != NULL)
        {
            char* fnabs = dia_get_absolute_filename (filename);
            if (fnabs)
                gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(odlg), fnabs);
            g_free(fnabs);
            g_free(filename);
        }
        g_signal_connect(GTK_OBJECT(odlg), "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &odlg);
//    }
//    else
//    {
//        gtk_widget_set_sensitive(opendlg, TRUE);
//        if (GTK_WIDGET_VISIBLE(opendlg))
//            return;
//    }
    if (!gtk_file_chooser_get_extra_widget(GTK_FILE_CHOOSER(odlg)))
    {
        GtkWidget *hbox, *label, *omenu, *options;
        GtkFileFilter* filter;

        options = gtk_frame_new(_("Open Options"));
        gtk_frame_set_shadow_type(GTK_FRAME(options), GTK_SHADOW_ETCHED_IN);

        hbox = gtk_hbox_new(FALSE, 1);
        gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
        gtk_container_add(GTK_CONTAINER(options), hbox);
        gtk_widget_show(hbox);

        label = gtk_label_new (_("Determine file type:"));
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
        gtk_widget_show (label);

        omenu = create_open_menu();
        gtk_box_pack_start(GTK_BOX(hbox), omenu, TRUE, TRUE, 0);
        gtk_widget_show(omenu);

        gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(odlg),
                                          options);

        gtk_widget_show(options);
        g_signal_connect(GTK_OBJECT(odlg), "response",
                         G_CALLBACK(file_open_response_callback), omenu);

        /* set up the gtk file (name) filters */
        /* 0 = by extension */
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (odlg),
                                     build_gtk_file_filter_from_index (0));
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, _("All Files"));
        gtk_file_filter_add_pattern (filter, "*");
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (odlg), filter);

//        filter = gtk_file_filter_new ();
//        gtk_file_filter_set_name (filter, factory_utf8("模版文件"));
//        gtk_file_filter_add_pattern (filter, "*.lcy");
//        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (odlg), filter);

        gtk_combo_box_set_active (GTK_COMBO_BOX (omenu), persistence_get_integer ("import-filter"));
    }

    gtk_widget_show(odlg);
}

/**
 * Respond to a button press (also destroy) in the save as dialog.
 */
static void
file_save_as_response_callback(GtkWidget *fs,
                               gint       response,
                               gpointer   user_data)
{
    char *filename;
    Diagram *dia;
    struct stat stat_struct;


    if (response == GTK_RESPONSE_ACCEPT)
    {
        dia = g_object_get_data (G_OBJECT(fs), "user_data");

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));
        factory_debug_to_log(g_strdup_printf(factory_utf8("另存为文件,名字:%s.\n"),filename));

        if (g_stat(filename, &stat_struct) == 0)
        {
            GtkWidget *dialog = NULL;
            char *utf8filename = NULL;
            if (!g_utf8_validate(filename, -1, NULL))
            {
                utf8filename = g_filename_to_utf8(filename, -1, NULL, NULL, NULL);
                if (utf8filename == NULL)
                {
                    message_warning(_("Some characters in the filename are neither UTF-8\n"
                                      "nor your local encoding.\nSome things will break."));
                }
            }
            if (utf8filename == NULL) utf8filename = g_strdup(filename);


            dialog = gtk_message_dialog_new (GTK_WINDOW(fs),
                                             GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
                                             GTK_BUTTONS_YES_NO,
                                             _("File already exists"));
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                    _("The file '%s' already exists.\n"
                      "Do you want to overwrite it?"), utf8filename);
            g_free(utf8filename);
            gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

            if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_YES)
            {
                /* don't hide/destroy the dialog, but simply go back to it */
                gtk_window_present (GTK_WINDOW (fs));
                gtk_widget_destroy(dialog);
                g_free (filename);
                return;
            }
            gtk_widget_destroy(dialog);
        }

//   dia->data->is_compressed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(user_data));
        dia->data->is_compressed = 0; // 2014-3-22 lcy 关闭压缩保存功能。

        diagram_update_extents(dia);

        diagram_set_filename(dia, filename);
        if (diagram_save(dia, g_strdup(filename)))
            recent_file_history_add(filename);

        g_free (filename);
    }
    /* if we have our own reference, drop it before destroy */
    if ((dia = g_object_get_data (G_OBJECT(fs), "user_data")) != NULL)
    {
        g_object_set_data (G_OBJECT(fs), "user_data", NULL);
        g_object_unref (dia);
    }
    gtk_widget_destroy(GTK_WIDGET(fs));
}



void factory_template_save_as_callback(gpointer data,guint action,GtkWidget *widget)
{

}

void factory_template_save_callback(gpointer data,guint action, GtkWidget *widget)
{

}


/**
 * Respond to the File/Save As.. menu
 *
 * We have only one file save dialog at a time. So if the dialog alread exists
 * and the user tries to Save as once more only the diagram refernced will
 * change. Maybe we should also indicate the refernced diagram in the dialog.
 */
void
file_save_as_callback(gpointer data, guint action, GtkWidget *widget)
{
    DDisplay *ddisp;
    Diagram *dia;
    gchar *filename = NULL;

    ddisp = ddisplay_active();
    if (!ddisp) return;
    dia = ddisp->diagram;

    if (!savedlg)
    {
        GtkWidget *compressbutton;
        savedlg =  gtk_file_chooser_dialog_new (_("Save Diagram"), GTK_WINDOW(ddisp->shell),
                                                GTK_FILE_CHOOSER_ACTION_SAVE,
                                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                                NULL);

//    savedlg = gtk_file_chooser_dialog_new_with_backend(_("Save Diagram"),
//					  GTK_WINDOW(ddisp->shell),
//					  GTK_FILE_CHOOSER_ACTION_SAVE,
//					  "gtk+", /* default, not gnome-vfs */
//					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
//					  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
//					  NULL);
        gtk_window_set_modal(GTK_WINDOW(savedlg),TRUE);
        gtk_dialog_set_default_response(GTK_DIALOG(savedlg), GTK_RESPONSE_ACCEPT);
//    gtk_window_set_role(GTK_WINDOW(savedlg), "save_diagram");
        /* Need better way to make it a reasonable size.  Isn't there some*/
        /* standard look for them (or is that just Gnome?)*/
        compressbutton = gtk_check_button_new_with_label(_("Compress diagram files"));
        gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(savedlg),
                                          compressbutton);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compressbutton),
                                     dia->data->is_compressed);
        gtk_widget_set_sensitive(compressbutton,FALSE); // 2014-3-22 lcy 让压缩选项变灰。
        g_signal_connect(G_OBJECT(compressbutton), "toggled",
                         G_CALLBACK(toggle_compress_callback), NULL);
        gtk_widget_show(compressbutton);
#if GTK_CHECK_VERSION (2,12,0)
        gtk_widget_set_tooltip_text (compressbutton,
                                     _("Compression reduces file size to less than 1/10th "
                                       "size and speeds up loading and saving.  Some text "
                                       "programs cannot manipulate compressed files."));
#else
        gtk_tooltips_set_tip(tool_tips, compressbutton,
                             _("Compression reduces file size to less than 1/10th "
                               "size and speeds up loading and saving.  Some text "
                               "programs cannot manipulate compressed files."), NULL);
#endif
        g_signal_connect (GTK_FILE_CHOOSER(savedlg),
                          "response", G_CALLBACK(file_save_as_response_callback), compressbutton);
        g_signal_connect(GTK_OBJECT(savedlg), "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &savedlg);
    }
    else
    {
        GtkWidget *compressbutton = gtk_file_chooser_get_extra_widget(GTK_FILE_CHOOSER(savedlg));
        gtk_widget_set_sensitive(savedlg, TRUE);
        g_signal_handlers_block_by_func(G_OBJECT(compressbutton), toggle_compress_callback, NULL);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compressbutton),
                                     dia->data->is_compressed);
        g_signal_handlers_unblock_by_func(G_OBJECT(compressbutton), toggle_compress_callback, NULL);
        if (g_object_get_data (G_OBJECT (savedlg), "user_data") != NULL)
            g_object_unref (g_object_get_data (G_OBJECT (savedlg), "user_data"));
        if (GTK_WIDGET_VISIBLE(savedlg))
        {
            /* keep a refernce to the diagram */
            g_object_ref(dia);
            g_object_set_data (G_OBJECT (savedlg), "user_data", dia);
            gtk_window_present (GTK_WINDOW(savedlg));
            return;
        }
    }
    if (dia && dia->filename)
        filename = g_filename_from_utf8(dia->filename, -1, NULL, NULL, NULL);
    if (filename != NULL)
    {
        char* fnabs = dia_get_absolute_filename (filename);
        if (fnabs)
        {
            gchar *base = g_path_get_basename(dia->filename);
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(savedlg), fnabs);
            /* FileChooser api insist on exiting files for set_filename  */
            /* ... and does not use filename encoding on this one. */
            gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(savedlg), base);
            g_free(base);
        }
        g_free(fnabs);
        g_free(filename);
    }
    g_object_ref(dia);
    g_object_set_data (G_OBJECT (savedlg), "user_data", dia);

    gtk_widget_show(savedlg);
}

/**
 * Respond to the File/Save menu entry.
 *
 * Delegates to Save As if there is no filename set yet.
 */
void
file_save_callback(gpointer data, guint action, GtkWidget *widget)
{
    Diagram *diagram;

    diagram = ddisplay_active_diagram();
    if (!diagram) return;

    if(diagram->isTemplate)
    {
        FactoryTemplateItem *titem = diagram->templ_item;
        if(!titem->entrypoint ||
                !titem->modellist ||
                !titem->fsil->sname)
        {
            gchar *msg = factory_utf8("你没有编辑任何东西,保存之前请先运行过编辑对话框!");
            GtkWidget * msg_dialog = gtk_message_dialog_new (GTK_WINDOW (ddisplay_active()->shell),
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_WARNING,
                                     GTK_BUTTONS_CLOSE,
                                     msg);

            gint yes_or_no = gtk_dialog_run (GTK_DIALOG (msg_dialog));
            gtk_widget_destroy (msg_dialog);
            titem->templ_ops->templ_edit(titem->fsil);
            g_free(msg);
            return;
        }

    }

    if (diagram->unsaved)
    {
        file_save_as_callback(data, action, widget);
    }
    else
    {
        gchar *filename = g_filename_from_utf8(diagram->filename, -1, NULL, NULL, NULL);
        diagram_update_extents(diagram);
        if (diagram_save(diagram, g_strdup(filename)))
            recent_file_history_add(filename);
        g_free (filename);
    }
}

/**
 * Given an export filter index return the export filter to use
 */
static DiaExportFilter *
efilter_by_index (int index, const gchar** ext)
{
    DiaExportFilter *efilter = NULL;

    /* the index in the selection list *is* the index of the filter,
     * filters supporing multiple formats are multiple times in the list */
    if (index >= 0)
    {
        efilter = g_list_nth_data (filter_get_export_filters(), index);
        if (efilter)
        {
            if (ext)
                *ext = efilter->extensions[0];
            return efilter;
        }
        else /* getting here means invalid index */
            g_warning ("efilter_by_index() index=%d out of range", index);
    }

    return efilter;
}

/**
 * Adapt the filename to the export filter index
 */
static void
export_adapt_extension (const gchar* name, int index)
{
    const gchar* ext = NULL;
    DiaExportFilter *efilter = efilter_by_index (index, &ext);
    gchar *basename = g_path_get_basename (name);
    gchar *utf8_name = NULL;

    if (!efilter || !ext)
        utf8_name = g_filename_to_utf8 (basename, -1, NULL, NULL, NULL);
    else
    {
        const gchar *last_dot = strrchr(basename, '.');
        GString *s = g_string_new(basename);
        if (last_dot)
            g_string_truncate(s, last_dot-basename);
        g_string_append(s, ".");
        g_string_append(s, ext);
        utf8_name = g_filename_to_utf8 (s->str, -1, NULL, NULL, NULL);
        g_string_free (s, TRUE);
    }
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(exportdlg), utf8_name);
    g_free (utf8_name);
    g_free (basename);
}
static void
export_adapt_extension_callback(GtkWidget *widget)
{
    int index = gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
    gchar *name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(exportdlg));

    if (name && index > 0) /* Ignore "By Extension" */
        export_adapt_extension (name, index - 1);
    g_free (name);
}

/**
 * Create a new "option menu" for the export options
 */
static GtkWidget *
create_export_menu(void)
{
    GtkWidget *menu;
    GList *tmp;

    menu = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(menu), _("By extension"));

    for (tmp = filter_get_export_filters(); tmp != NULL; tmp = tmp->next)
    {
        DiaExportFilter *ef = tmp->data;
        gchar *filter_label;

        if (!ef)
            continue;
        filter_label = filter_get_export_filter_label(ef);
        gtk_combo_box_append_text (GTK_COMBO_BOX(menu), filter_label);
        g_free(filter_label);
    }
    g_signal_connect(GTK_OBJECT(menu), "changed",
                     G_CALLBACK(export_adapt_extension_callback), NULL);
    return menu;
}

/**
 * A button hit in the Export Dialog
 */
static void
file_export_response_callback(GtkWidget *fs,
                              gint       response,
                              gpointer   user_data)
{
    char *filename;
    Diagram *dia;
    DiaExportFilter *ef;
    struct stat statbuf;

    dia = g_object_get_data (G_OBJECT (fs), "user_data");
    g_assert (dia);

    if (response == GTK_RESPONSE_ACCEPT)
    {
        gint index;

        diagram_update_extents(dia);

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));

        if (g_stat(filename, &statbuf) == 0)
        {
            GtkWidget *dialog = NULL;

            dialog = gtk_message_dialog_new (GTK_WINDOW(fs),
                                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_QUESTION,
                                             GTK_BUTTONS_YES_NO,
                                             _("File already exists"));
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                    _("The file '%s' already exists.\n"
                      "Do you want to overwrite it?"), dia_message_filename(filename));
            gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

            if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_YES)
            {
                /* if not overwrite allow to select another filename */
                gtk_widget_destroy(dialog);
                g_free (filename);
                return;
            }
            gtk_widget_destroy(dialog);
        }

        index = gtk_combo_box_get_active (GTK_COMBO_BOX(user_data));
        if (index >= 0)
            persistence_set_integer ("export-filter", index);
        ef = efilter_by_index (index - 1, NULL);
        if (!ef)
            ef = filter_guess_export_filter(filename);
        if (ef)
        {
            g_object_ref(dia->data);
            ef->export_func(dia->data, filename, dia->filename, ef->user_data);
            g_object_unref(dia->data);
        }
        else
            message_error(_("Could not determine which export filter\n"
                            "to use to save '%s'"), dia_message_filename(filename));
        g_free (filename);
    }
    g_object_unref (dia); /* drop our diagram reference */
    gtk_widget_destroy(exportdlg);
}

/**
 * React to <Display>/File/Export
 */
void
file_export_callback(gpointer data, guint action, GtkWidget *widget)
{
    DDisplay *ddisp;
    Diagram *dia;
    gchar *filename = NULL;

    ddisp = ddisplay_active();
    if (!ddisp) return;
    dia = ddisp->diagram;

    if (!confirm_export_size (dia, GTK_WINDOW(ddisp->shell), CONFIRM_MEMORY|CONFIRM_PAGES))
        return;

    if (!exportdlg)
    {
        persistence_register_integer ("export-filter", 0);
        exportdlg = gtk_file_chooser_dialog_new_with_backend(_("Export Diagram"),
                    GTK_WINDOW(ddisp->shell),
                    GTK_FILE_CHOOSER_ACTION_SAVE,
                    "gtk+", /* default, not gnome-vfs */
                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                    NULL);
        gtk_dialog_set_default_response(GTK_DIALOG(exportdlg), GTK_RESPONSE_ACCEPT);
        gtk_window_set_role(GTK_WINDOW(exportdlg), "export_diagram");
        g_signal_connect(GTK_OBJECT(exportdlg), "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &exportdlg);
    }
    if (!gtk_file_chooser_get_extra_widget(GTK_FILE_CHOOSER(exportdlg)))
    {
        GtkWidget *hbox, *label, *omenu, *options;
        GtkFileFilter* filter;

        options = gtk_frame_new(_("Export Options"));
        gtk_frame_set_shadow_type(GTK_FRAME(options), GTK_SHADOW_ETCHED_IN);

        hbox = gtk_hbox_new(FALSE, 1);
        gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
        gtk_container_add(GTK_CONTAINER(options), hbox);
        gtk_widget_show(hbox);

        label = gtk_label_new (_("Determine file type:"));
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
        gtk_widget_show (label);

        omenu = create_export_menu();
        gtk_box_pack_start(GTK_BOX(hbox), omenu, TRUE, TRUE, 0);
        gtk_widget_show(omenu);
        g_object_set_data(G_OBJECT(exportdlg), "export-menu", omenu);

        gtk_widget_show(options);
        gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(exportdlg), options);
        /* set up file filters */
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, _("All Files"));
        gtk_file_filter_add_pattern (filter, "*");
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (exportdlg), filter);
        /* match the other selections extension */
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, _("Supported Formats"));
        gtk_file_filter_add_custom (filter, GTK_FILE_FILTER_FILENAME,
                                    matching_extensions_filter, filter_guess_export_filter, NULL);
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (exportdlg), filter);

        gtk_combo_box_set_active (GTK_COMBO_BOX (omenu), persistence_get_integer ("export-filter"));

        g_signal_connect(GTK_FILE_CHOOSER(exportdlg),
                         "response", G_CALLBACK(file_export_response_callback), omenu);
    }
    if (g_object_get_data (G_OBJECT(exportdlg), "user_data"))
        g_object_unref (g_object_get_data (G_OBJECT(exportdlg), "user_data"));
    g_object_ref(dia);
    g_object_set_data (G_OBJECT (exportdlg), "user_data", dia);
    gtk_widget_set_sensitive(exportdlg, TRUE);

    if (dia && dia->filename)
        filename = g_filename_from_utf8(dia->filename, -1, NULL, NULL, NULL);
    if (filename != NULL)
    {
        char* fnabs = dia_get_absolute_filename (filename);
        if (fnabs)
        {
            char *folder = g_path_get_dirname (fnabs);
            char *basename = g_path_get_basename (fnabs);
            /* can't use gtk_file_chooser_set_filename for various reasons, see e.g. bug #305850 */
            gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(exportdlg), folder);
            export_adapt_extension (basename, persistence_get_integer ("export-filter") - 1);
            g_free (folder);
            g_free (basename);
        }
        g_free(fnabs);
        g_free(filename);
    }

    gtk_widget_show(exportdlg);
}
