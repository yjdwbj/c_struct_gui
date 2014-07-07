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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* wants to be included early to avoid complains about setjmp.h */
#ifdef HAVE_LIBPNG
#include <png.h> /* just for the version stuff */
#endif

#ifdef GNOME
#undef GTK_DISABLE_DEPRECATED
/* /usr/include/libgnomeui-2.0/libgnomeui/gnome-entry.h:58: error: expected specifier-qualifier-list before 'GtkCombo' */
#include <gnome.h>
#endif

#include <gtk/gtk.h>
#include <gmodule.h>

#ifdef HAVE_FREETYPE
#include <pango/pangoft2.h>
#endif

#include <libxml/parser.h>
#include <libxml/xmlerror.h>

#include <glib/gstdio.h>

#include "intl.h"
#include "app_procs.h"
#include "object.h"
#include "commands.h"
#include "tool.h"
#include "interface.h"
#include "modify_tool.h"
#include "group.h"
#include "message.h"
#include "display.h"
#include "layer_dialog.h"
#include "load_save.h"
#include "preferences.h"
#include "dia_dirs.h"
#include "sheet.h"
#include "plug-ins.h"
#include "recent_files.h"
#include "authors.h"
#include "autosave.h"
#include "dynamic_refresh.h"
#include "persistence.h"
#include "sheets.h"
#include "exit_dialog.h"
#include "newgroup.h"
#include "dialib.h"
#include "diaerror.h"

static gboolean
handle_initial_diagram(const char *input_file_name,
                       const char *export_file_name,
                       const char *export_file_format,
                       const char *size,
                       char *show_layers,
                       const char *outdir);

static void create_user_dirs(void);
static PluginInitResult internal_plugin_init(PluginInfo *info);
static gboolean handle_all_diagrams(GSList *files, char *export_file_name,
                                    char *export_file_format, char *size, char *show_layers,
                                    const gchar *input_directory, const gchar *output_directory);
static void print_credits(gboolean credits);

static gboolean dia_is_interactive = FALSE;

#ifdef GNOME

static void
session_die (gpointer client_data)
{
    gtk_main_quit ();
}

static int
save_state (GnomeClient        *client,
            gint                phase,
            GnomeRestartStyle   save_style,
            gint                shutdown,
            GnomeInteractStyle  interact_style,
            gint                fast,
            gpointer            client_data)
{
    gchar *argv[20];
    gint i = 0;
    GList *l;
    Diagram *dia;

    argv[i++] = "dia";

    for(l = dia_open_diagrams(); l != NULL; l = g_list_next(l))
    {
        dia = (Diagram *)l->data;
        if(!dia->unsaved)
        {
            argv[i++] = dia->filename;
        }
    }

    gnome_client_set_restart_command (client, i, argv);
    gnome_client_set_clone_command (client, i, argv);

    return TRUE;
}
#endif

static char *
build_output_file_name(const char *infname, const char *format, const char *outdir)
{
    const char *pe = strrchr(infname,'.');
    const char *pp = strrchr(infname,G_DIR_SEPARATOR);
    char *tmp;
    if (!pp)
        pp = infname;
    else
        pp += 1;
    if (!pe)
        return g_strconcat(outdir ? outdir : "", pp,".",format,NULL);

    tmp = g_malloc0(strlen(pp)+1+strlen(format)+1);
    memcpy(tmp,pp,pe-pp);
    strcat(tmp,".");
    strcat(tmp,format);
    if (outdir)
    {
        char *ret = g_strconcat(outdir, G_DIR_SEPARATOR_S, tmp, NULL);
        g_free(tmp);
        return ret;
    }
    return tmp;
}

/* Handle the string between commas. We have either of:
 *
 * 1. XX, the number XX
 * 2. -XX, every number until XX
 * 3. XX-, every number from XX until n_layers
 * 4. XX-YY, every number between XX-YY
 */
static void
show_layers_parse_numbers(DiagramData *diagdata, gboolean *visible_layers, gint n_layers, const char *str)
{
    char *p;
    unsigned long int low = 0;
    unsigned long int high = n_layers;
    unsigned long int i;

    if (str == NULL)
        return;

    /* Case 2, starts with '-' */
    if (*str == '-')
    {
        str++;
        low = 0;
        high = strtoul(str, &p, 10)+1;
        /* This must be a number (otherwise we would have called parse_name) */
        g_assert(p != str);
    }
    else
    {
        /* Case 1, 3 or 4 */
        low = strtoul(str, &p, 10);
        high = low+1; /* Assume case 1 */
        g_assert(p != str);
        if (*p == '-')
        {
            /* Case 3 or 4 */
            str = p + 1;
            if (*str == '\0') /* Case 3 */
                high = n_layers;
            else
            {
                high = strtoul(str, &p, 10)+1;
                g_assert(p != str);
            }
        }
    }

    if ( high <= low )
    {
        /* This is not an errror */
        g_print(_("Warning: invalid layer range %lu - %lu\n"), low, high-1 );
        return;
    }
    if (high > n_layers)
        high = n_layers;

    /* Set the visible layers */
    for ( i = low; i < high; i++ )
    {
        Layer *lay = (Layer *)g_ptr_array_index(diagdata->layers, i);

        if (visible_layers[i] == TRUE)
            g_print(_("Warning: Layer %lu (%s) selected more than once.\n"), i, lay->name);
        visible_layers[i] = TRUE;
    }
}

static void
show_layers_parse_word(DiagramData *diagdata, gboolean *visible_layers, gint n_layers, const char *str)
{
    GPtrArray *layers = diagdata->layers;
    gboolean found = FALSE;

    /* Apply --show-layers=LAYER,LAYER,... switch. 13.3.2004 sampo@iki.fi */
    if (layers)
    {
        int len,k = 0;
        Layer *lay;
        char *p;
        for (k = 0; k < layers->len; k++)
        {
            lay = (Layer *)g_ptr_array_index(layers, k);

            if (lay->name)
            {
                len =  strlen(lay->name);
                if ((p = strstr(str, lay->name)) != NULL)
                {
                    if (((p == str) || (p[-1] == ','))    /* zap false positives */
                            && ((p[len] == 0) || (p[len] == ',')))
                    {
                        found = TRUE;
                        if (visible_layers[k] == TRUE)
                            g_print(_("Warning: Layer %d (%s) selected more than once.\n"), k, lay->name);
                        visible_layers[k] = TRUE;
                    }
                }
            }
        }
    }

    if (found == FALSE)
        g_print(_("Warning: There is no layer named %s\n"), str);
}

static void
show_layers_parse_string(DiagramData *diagdata, gboolean *visible_layers, gint n_layers, const char *str)
{
    gchar **pp;
    int i;

    pp = g_strsplit(str, ",", 100);

    for (i = 0; pp[i]; i++)
    {
        gchar *p = pp[i];

        /* Skip the empty string */
        if (strlen(p) == 0)
            continue;

        /* If the string is only numbers and '-' chars, it is parsed as a
         * number range. Otherwise it is parsed as a layer name.
         */
        if (strlen(p) != strspn(p, "0123456789-") )
            show_layers_parse_word(diagdata, visible_layers, n_layers, p);
        else
            show_layers_parse_numbers(diagdata, visible_layers, n_layers, p);
    }

    g_strfreev(pp);
}


static void
handle_show_layers(DiagramData *diagdata, const char *show_layers)
{
    gboolean *visible_layers;
    Layer *layer;
    int i;

    visible_layers = g_malloc(diagdata->layers->len * sizeof(gboolean));
    /* Assume all layers are non-visible */
    for (i=0; i<diagdata->layers->len; i++)
        visible_layers[i] = FALSE;

    /* Split the layer-range by commas */
    show_layers_parse_string(diagdata, visible_layers, diagdata->layers->len,
                             show_layers);

    /* Set the visibility of the layers */
    for (i=0; i<diagdata->layers->len; i++)
    {
        layer = g_ptr_array_index(diagdata->layers, i);

        if (visible_layers[i] == TRUE)
            layer->visible = TRUE;
        else
            layer->visible = FALSE;
    }
    g_free(visible_layers);
}


const char *argv0 = NULL;

/** Convert infname to outfname, using input filter inf and export filter
 * ef.  If either is null, try to guess them.
 * size might be NULL.
 */
static gboolean
do_convert(const char *infname,
           const char *outfname, DiaExportFilter *ef,
           const char *size,
           char *show_layers)
{
    DiaImportFilter *inf;
    DiagramData *diagdata = NULL;

    inf = filter_guess_import_filter(infname);
    if (!inf)
        inf = &dia_import_filter;

    if (ef == NULL)
    {
        ef = filter_guess_export_filter(outfname);
        if (!ef)
        {
            g_critical(_("%s error: don't know how to export into %s\n"),
                       argv0,outfname);
            exit(1);
        }
    }

    dia_is_interactive = FALSE;

    if (0==strcmp(infname,outfname))
    {
        g_critical(_("%s error: input and output file name is identical: %s"),
                   argv0, infname);
        exit(1);
    }

    diagdata = g_object_new (DIA_TYPE_DIAGRAM_DATA, NULL);

    if (!inf->import_func(infname,diagdata,inf->user_data))
    {
        g_critical(_("%s error: need valid input file %s\n"),
                   argv0, infname);
        exit(1);
    }

    /* Apply --show-layers */
    if (show_layers)
        handle_show_layers(diagdata, show_layers);

    /* recalculate before export */
    data_update_extents(diagdata);

    /* Do our best in providing the size to the filter, but don't abuse user_data
     * too much for it. It _must not_ be changed after initialization and there
     * are quite some filter selecting their output format by it. --hb
     */
    if (size)
    {
        if (ef == filter_get_by_name ("png-libart")) /* the warning we get is appropriate, don't cast */
            ef->export_func(diagdata, outfname, infname, size);
        else
        {
            g_warning ("--size parameter unsupported for %s filter",
                       ef->unique_name ? ef->unique_name : "selected");
            ef->export_func(diagdata, outfname, infname, ef->user_data);
        }
    }
    else
        ef->export_func(diagdata, outfname, infname, ef->user_data);
    /* if (!quiet) */ fprintf(stdout,
                              _("%s --> %s\n"),
                              infname,outfname);
    g_object_unref(diagdata);
    return TRUE;
}

void debug_break(void); /* shut gcc up */
int debug_break_dont_optimize = 1;
void
debug_break(void)
{
    if (debug_break_dont_optimize > 0)
        debug_break_dont_optimize -= 1;
}

static void
dump_dependencies(void)
{
#ifdef __GNUC__
    g_print ("Compiler: GCC " __VERSION__ "\n");
#elif defined _MSC_VER
    g_print ("Compiler: MSC %d\n", _MSC_VER);
#else
    g_print ("Compiler: unknown\n");
#endif
    /* some flags/defines which may be interesting */
    g_print ("  with : "
#ifdef G_THREADS_ENABLED
             "threads "
#endif
#ifdef HAVE_CAIRO
             "cairo "
#endif
#ifdef GNOME
             "gnome "
#endif
#ifdef HAVE_GNOMEPRINT
             "gnomeprint "
#endif
#ifdef HAVE_LIBART
             "libart "
#endif
#ifdef HAVE_PANGOCAIRO
             "pangocairo "
#endif
             "\n");

    /* print out all those dependies, both compile and runtime if possible
     * Note: this is not meant to be complete but does only include libaries
     * which may or have cause(d) us trouble in some versions
     */
    g_print ("Library versions (at compile time)\n");
#ifdef HAVE_LIBPNG
    g_print ("libpng  : %s (%s)\n", png_get_header_ver(NULL), PNG_LIBPNG_VER_STRING);
#endif
#ifdef HAVE_FREETYPE
    {
        FT_Library ft_library;
        FT_Int     ft_major_version;
        FT_Int     ft_minor_version;
        FT_Int     ft_micro_version;

        if (FT_Init_FreeType (&ft_library) == 0)
        {
            FT_Library_Version (ft_library,
                                &ft_major_version,
                                &ft_minor_version,
                                &ft_micro_version);

            g_print ("freetype: %d.%d.%d\n", ft_major_version, ft_minor_version, ft_micro_version);
            FT_Done_FreeType (ft_library);
        }
        else
            g_print ("freetype: ?.?.?\n");
    }
#endif
    {
        const gchar* libxml_rt_version = "?";
#if 0
        /* this is stupid, does not compile on Linux:
         * app_procs.c:504: error: expected identifier before '(' token
         *
         * In fact libxml2 has different ABI for LIBXML_THREAD_ENABLED, this code only compiles without
         * threads enabled, but apparently it does only work when theay are.
         */
        xmlInitParser();
        if (xmlGetGlobalState())
            libxml_rt_version = xmlGetGlobalState()->xmlParserVersion;
#endif
        libxml_rt_version = xmlParserVersion;
        if (atoi(libxml_rt_version))
            g_print ("libxml  : %d.%d.%d (%s)\n",
                     atoi(libxml_rt_version) / 10000, atoi(libxml_rt_version) / 100 % 100, atoi(libxml_rt_version) % 100,
                     LIBXML_DOTTED_VERSION);
        else /* may include "extra" */
            g_print ("libxml  : %s (%s)\n", libxml_rt_version ? libxml_rt_version : "??", LIBXML_DOTTED_VERSION);
    }
    g_print ("glib    : %d.%d.%d (%d.%d.%d)\n",
             glib_major_version, glib_minor_version, glib_micro_version,
             GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
#ifdef PANGO_VERSION_ENCODE
    g_print ("pango   : %s (%d.%d.%d)\n", pango_version_string(), PANGO_VERSION_MAJOR, PANGO_VERSION_MINOR, PANGO_VERSION_MICRO);
#else
    g_print ("pango   : version not available (>= 1.14.x)\n"); /* Pango did not provide such */
#endif
#if HAVE_CAIRO
#  ifdef CAIRO_VERSION_STRING
    g_print ("cairo   : %s (%s)\n", cairo_version_string(), CAIRO_VERSION_STRING);
#  else
    g_print ("cairo   : %s (%d.%d.%d)\n", cairo_version_string(), CAIRO_VERSION_MAJOR, CAIRO_VERSION_MINOR, CAIRO_VERSION_MICRO);
#  endif
#endif
#if 0
    {
        gchar  linkedname[1024];
        gint   len = 0;

        /* relying on PREFIX is wrong */
        if ((len = readlink (PREFIX "/lib/libpango-1.0.so", linkedname, 1023)) > 0)
        {
            /* man 2 readlink : does not append a  NUL  character */
            linkedname[len] = '\0';
            g_print ("%s/%s\n", PREFIX, linkedname);
        }
    }
#endif
    g_print ("gtk+    : %d.%d.%d (%d.%d.%d)\n",
             gtk_major_version, gtk_minor_version, gtk_micro_version,
             GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);

#if 0
    /* we could read $PREFIX/share/gnome-about/gnome-version.xml
     * but is it worth the effort ? */
    g_print ("gnome   : %d.%d.%d (%d.%d.%d)\n"
             gnome_major_version, gnome_minor_version, gnome_micro_version,
             GNOME_MAJOR_VERSION, GNOME_MINOR_VERSION, GNOME_MICRO_VERSION);
#endif
}

gboolean
app_is_interactive(void)
{
    return dia_is_interactive;
}

/** Handle loading of diagrams given on command line, including conversions.
 * Returns TRUE if any automatic conversions were performed.
 * Note to future hackers:  'size' is currently the only argument that can be
 * sent to exporters.  If more arguments are desired, please don't just add
 * even more arguments, but create a more general system.
 */
static gboolean
handle_initial_diagram(const char *in_file_name,
                       const char *out_file_name,
                       const char *export_file_format,
                       const char *size,
                       char* show_layers,
                       const char *outdir)
{
    DDisplay *ddisp = NULL;
    Diagram *diagram = NULL;
    gboolean made_conversions = FALSE;

    if (export_file_format)
    {
        char *export_file_name = NULL;
        DiaExportFilter *ef = NULL;

        /* First try guessing based on extension */
        export_file_name = build_output_file_name(in_file_name, export_file_format, outdir);

        /* to make the --size hack even uglier but work again for the only filter supporting it */
        if (   size && strcmp(export_file_format, "png") == 0)
            ef = filter_get_by_name ("png-libart");
        if (!ef)
            ef = filter_guess_export_filter(export_file_name);
        if (ef == NULL)
        {
            ef = filter_get_by_name(export_file_format);
            if (ef == NULL)
            {
                g_critical(_("Can't find output format/filter %s\n"), export_file_format);
                return FALSE;
            }
            g_free (export_file_name);
            export_file_name = build_output_file_name(in_file_name, ef->extensions[0], outdir);
        }
        made_conversions |= do_convert(in_file_name,
                                       (out_file_name != NULL?out_file_name:export_file_name),
                                       ef, size, show_layers);
        g_free(export_file_name);
    }
    else if (out_file_name)
    {
        DiaExportFilter *ef = NULL;

        /* if this looks like an ugly hack to you, agreed ;)  */
        if (size && strstr(out_file_name, ".png"))
            ef = filter_get_by_name ("png-libart");

        made_conversions |= do_convert(in_file_name, out_file_name, ef,
                                       size, show_layers);
    }
    else
    {
        if (g_file_test(in_file_name, G_FILE_TEST_EXISTS))
        {
            diagram = diagram_load (in_file_name, NULL);
        }
        else
        {
            diagram = new_diagram (in_file_name);
        }

        if (diagram != NULL)
        {
            diagram_update_extents(diagram);
            if (app_is_interactive())
            {
                layer_dialog_set_diagram(diagram);
                /* the display initial diagram holds two references */
                ddisp = new_display(diagram);
            }
            else
            {
                g_object_unref(diagram);
            }
        }
    }
    return made_conversions;
}

#ifdef HAVE_FREETYPE
/* Translators:  This is an option, not to be translated */
#define EPS_PANGO "eps-pango, "
#else
#define EPS_PANGO ""
#endif

#ifdef G_OS_WIN32
/* Translators:  This is an option, not to be translated */
#define WMF "wmf, "
#else
#define WMF ""
#endif

static const gchar *input_directory = NULL;
static const gchar *output_directory = NULL;

static gboolean
_check_option_input_directory (const gchar    *option_name,
                               const gchar    *value,
                               gpointer        data,
                               GError        **error)
{
    gchar *directory = g_filename_from_utf8 (value, -1, NULL, NULL, NULL);

    if (g_file_test (directory, G_FILE_TEST_IS_DIR))
    {
        input_directory = directory;
        return TRUE;
    }
    g_set_error (error, DIA_ERROR, DIA_ERROR_DIRECTORY,
                 _("Input-directory '%s' must exist!\n"), directory);
    g_free (directory);
    return FALSE;
}
static gboolean
_check_option_output_directory (const gchar    *option_name,
                                const gchar    *value,
                                gpointer        data,
                                GError        **error)
{
    gchar *directory = g_filename_from_utf8 (value, -1, NULL, NULL, NULL);

    if (g_file_test (directory, G_FILE_TEST_IS_DIR))
    {
        output_directory = directory;
        return TRUE;
    }
    g_set_error (error, DIA_ERROR, DIA_ERROR_DIRECTORY,
                 _("Output-directory '%s' must exist!\n"), directory);
    g_free (directory);
    return FALSE;
}

static void
_setup_textdomains (void)
{
#ifdef G_OS_WIN32
    /* calculate runtime directory */
    {
        gchar* localedir = dia_get_lib_directory ("locale");

        bindtextdomain(GETTEXT_PACKAGE, localedir);
        g_free (localedir);
    }
#else
    const gchar *localedir;

    localedir = g_getenv("DIA_LOCALE_PATH");
    if(localedir != NULL)
    {
        bindtextdomain(GETTEXT_PACKAGE, localedir);
    }
    else
    {
        bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    }
#endif

#if defined ENABLE_NLS && defined HAVE_BIND_TEXTDOMAIN_CODESET
    bind_textdomain_codeset(GETTEXT_PACKAGE,"UTF-8");
#endif
    textdomain(GETTEXT_PACKAGE);
}




static gchar* factory_get_format_date_and_time()
{
    GDate *cdate =  g_date_new();
    g_date_set_time(cdate,time(NULL));
    GDateTime *gdt =  g_date_time_new_now_utc();
    gchar *datatime = NULL;
    datatime = g_strconcat(g_strdup_printf("%d-",cdate->year),g_strdup_printf("%d-",cdate->month),
                                           g_strdup_printf("%d-",cdate->day),
                                           g_strdup_printf("%d-",g_date_time_get_hour(gdt)+8), /* 加8是中国GMT+8区*/
                                           g_strdup_printf("%d-",g_date_time_get_minute(gdt)),
                                           g_strdup_printf("%d",g_date_time_get_second(gdt)),NULL );
    return datatime;
}
void  app_init (int argc, char **argv)
{
    static gboolean nosplash = FALSE;
    static gboolean nonew = FALSE;
    static gboolean use_integrated_ui = TRUE;  // 2014-3-20 lcy 这里是一个开关,是否把工具栏与编辑区域集成在一起.
    static gboolean credits = FALSE;
    static gboolean version = FALSE;
    static gboolean verbose = FALSE;
    static gboolean log_to_stderr = FALSE;
#ifdef GNOME
    GnomeClient *client;
#endif
    static char *export_file_name = NULL;
    static char *export_file_format = NULL;
    static char *size = NULL;
    static char *show_layers = NULL;
    gboolean made_conversions = FALSE;
    GSList *files = NULL;
    static const gchar **filenames = NULL;
    int i = 0;

    gchar *export_format_string =
        /* Translators:  The argument is a list of options, not to be translated */
        g_strdup_printf(_("Select the filter/format out of: %s"),
                        "cgm, dia, dxf, eps, eps-builtin, " EPS_PANGO
                        "fig, mp, plt, hpgl, png ("
#  if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)
                        "png-libart, "
#  endif
#  ifdef HAVE_CAIRO
                        "cairo-png, cairo-alpha-png, "
#  endif
                        /* we always have pixbuf but don't know exactly all it's *few* save formats */
                        "pixbuf-png), jpg, "
                        "shape, svg, tex (pgf-tex, pstricks-tex), " WMF
                        "wpg");

    GOptionContext *context = NULL;
    static GOptionEntry options[] =
    {
        {
            "export", 'e', 0, G_OPTION_ARG_FILENAME, NULL /* &export_file_name */,
            N_("Export loaded file and exit"), N_("OUTPUT")
        },
        {
            "filter",'t', 0, G_OPTION_ARG_STRING, NULL /* &export_file_format */,
            NULL /* &export_format_string */, N_("TYPE")
        },
        {
            "size", 's', 0, G_OPTION_ARG_STRING, NULL,
            N_("Export graphics size"), N_("WxH")
        },
        {
            "show-layers", 'L', 0, G_OPTION_ARG_STRING, NULL,
            N_("Show only specified layers (e.g. when exporting). Can be either the layer name or a range of layer numbers (X-Y)"),
            N_("LAYER,LAYER,...")
        },
        {
            "nosplash", 'n', 0, G_OPTION_ARG_NONE, &nosplash,
            N_("Don't show the splash screen"), NULL
        },
        {
            "nonew", 'n', 0, G_OPTION_ARG_NONE, &nonew,
            N_("Don't create empty diagram"), NULL
        },
        {
            "integrated", '\0', 0, G_OPTION_ARG_NONE, &use_integrated_ui,
            N_("Start integrated user interface (diagrams in tabs)"), NULL
        },
        {
            "log-to-stderr", 'l', 0, G_OPTION_ARG_NONE, &log_to_stderr,
            N_("Send error messages to stderr instead of showing dialogs."), NULL
        },
        {
            "input-directory", 'I', 0, G_OPTION_ARG_CALLBACK, _check_option_input_directory,
            N_("Directory containing input files"), N_("DIRECTORY")
        },
        {
            "output-directory", 'O', 0, G_OPTION_ARG_CALLBACK, _check_option_output_directory,
            N_("Directory containing output files"), N_("DIRECTORY")
        },
        {
            "credits", 'c', 0, G_OPTION_ARG_NONE, &credits,
            N_("Display credits list and exit"), NULL
        },
        {
            "verbose", 0, 0, G_OPTION_ARG_NONE, &verbose,
            N_("Generate verbose output"), NULL
        },
        {
            "version", 'v', 0, G_OPTION_ARG_NONE, &version,
            N_("Display version and exit"), NULL
        },
        {
            G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, NULL /* &filenames */,
            NULL, NULL
        },
        { NULL }
    };

    /* for users of app_init() the default is interactive */
    dia_is_interactive = TRUE;

    options[0].arg_data = &export_file_name;
    options[1].arg_data = &export_file_format;
    options[1].description = export_format_string;
    options[2].arg_data = &size;
    options[3].arg_data = &show_layers;
    g_assert (strcmp (options[13].long_name, G_OPTION_REMAINING) == 0);
    options[13].arg_data = (void*)&filenames;

    argv0 = (argc > 0) ? argv[0] : "(none)";

    gtk_set_locale();
    setlocale(LC_NUMERIC, "C");
    _setup_textdomains ();

    context = g_option_context_new(_("[FILE...]"));
    g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
    g_option_context_add_group (context, gtk_get_option_group (FALSE));

    if (argv)
    {
        GError *error = NULL;

        if (!g_option_context_parse (context, &argc, &argv, &error))
        {
            if (error)   /* IMO !error here is a bug upstream, triggered e.g. with --gdk-debug=updates */
            {
                g_print ("%s", error->message);
                g_error_free (error);
            }
            else
            {
                g_print (_("Invalid option?"));
            }

            g_option_context_free(context);
            exit(1);
        }
        /* second level check of command line options, existance of input files etc. */
        if (filenames)
        {
            while (filenames[i] != NULL)
            {
                gchar *filename;
                gchar *testpath;

                if (g_str_has_prefix (filenames[i], "file://"))
                {
                    filename = g_filename_from_uri (filenames[i], NULL, NULL);
                    if (!g_utf8_validate(filename, -1, NULL))
                    {
                        gchar *tfn = filename;
                        filename = g_filename_to_utf8(filename, -1, NULL, NULL, NULL);
                        g_free(tfn);
                    }
                }
                else
                    filename = g_filename_to_utf8 (filenames[i], -1, NULL, NULL, NULL);

                if (!filename)
                {
                    g_print (_("Filename conversion failed: %s\n"), filenames[i]);
                    continue;
                }

                if (g_path_is_absolute(filename))
                    testpath = filename;
                else
                    testpath = g_build_filename(input_directory ? input_directory : ".", filename, NULL);

                /* we still have a problem here, if GLib's file name encoding would not be utf-8 */
                if (g_file_test (testpath, G_FILE_TEST_IS_REGULAR))
                    files = g_slist_append(files, filename);
                else
                {
                    g_print (_("Missing input: %s\n"), filename);
                    g_free (filename);
                }
                if (filename != testpath)
                    g_free (testpath);
                ++i;
            }
        }
        /* given some files to output, we are not starting up the UI */
        if (export_file_name || export_file_format || size)
            dia_is_interactive = FALSE;

    }

    gchar *lfile = g_strdup_printf(LOGNAME,factory_get_format_date_and_time());
    gchar *logfpath = g_build_filename(dia_get_lib_directory("log"), lfile, NULL);

    logfd = fopen(logfpath,"w"); /*打开日志句柄*/

    /* Now write the data in the temporary file name. */

    if (logfd==NULL)
    {
        message_error(_("Can't open output file %s: %s\n"),
                      dia_message_filename(logfpath), strerror(errno));
        return;
    }
    /* 检查一下依赖程序是否存在 */
    gchar *isdownload_gui = g_strconcat(dia_get_lib_directory("bin"),G_DIR_SEPARATOR_S "isdownload_gui.exe",NULL);
    gchar *isdownload = g_strconcat(dia_get_lib_directory("bin"),G_DIR_SEPARATOR_S "isd_download.exe",NULL);
    gchar *makebin = g_strconcat(dia_get_lib_directory("bin"),G_DIR_SEPARATOR_S "makebin.exe",NULL);
    gchar *music_convert = g_strconcat(dia_get_lib_directory("bin"),G_DIR_SEPARATOR_S "music_convert.exe",NULL);
    factory_test_file_exist(makebin);
    factory_test_file_exist(isdownload_gui);
    factory_test_file_exist(isdownload);
    factory_test_file_exist(music_convert);


    if (argv && dia_is_interactive && !version)
    {
#ifdef GNOME
        GnomeProgram *program =
            gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,
                                argc, argv,
                                /* haven't found a quick way to pass GOption here */
                                GNOME_PARAM_GOPTION_CONTEXT, context,
                                GNOME_PROGRAM_STANDARD_PROPERTIES,
                                GNOME_PARAM_NONE);
        client = gnome_master_client();
        if(client == NULL)
        {
            g_warning(_("Can't connect to session manager!\n"));
        }
        else
        {
            g_signal_connect(GTK_OBJECT (client), "save_yourself",
                             G_CALLBACK (save_state), NULL);
            g_signal_connect(GTK_OBJECT (client), "die",
                             G_CALLBACK (session_die), NULL);
        }

        /* This smaller icon is 48x48, standard Gnome size */
        /* gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/dia_gnome_icon.png");*/

#else
#  ifdef G_THREADS_ENABLED
        g_thread_init (NULL);
#  endif
        gtk_init(&argc, &argv);
#endif
    }
    else
    {
#ifdef G_THREADS_ENABLED
        g_thread_init (NULL);
#endif
        g_type_init();
#ifdef GDK_WINDOWING_WIN32
        /*
         * On windoze there is no command line without display so this call is harmless.
         * But it is needed to avoid failing in gdk functions just because there is a
         * display check. Still a little hack ...
         */
        gtk_init(&argc, &argv);
#endif
    }

    /* done with option parsing, don't leak */
    g_free(export_format_string);

    if (version)
    {
#if (defined __TIME__) && (defined __DATE__)
        /* TRANSLATOR: 2nd and 3rd %s are time and date respectively. */
        printf(g_locale_from_utf8(_("Dia version %s, compiled %s %s\n"), -1, NULL, NULL, NULL), VERSION, __TIME__, __DATE__);
#else
        printf(g_locale_from_utf8(_("Dia version %s\n"), -1, NULL, NULL, NULL), VERSION);
#endif
        if (verbose)
            dump_dependencies();
        exit(0);
    }

    if (!dia_is_interactive)
        log_to_stderr = TRUE;

    libdia_init (   (dia_is_interactive ? DIA_INTERACTIVE : 0)
                    | (log_to_stderr ? DIA_MESSAGE_STDERR : 0)
                    | (verbose ? DIA_VERBOSE : 0) );


    print_credits(credits);


    if (dia_is_interactive)
    {
        create_user_dirs();

        if (!nosplash)
            app_splash_init("");

        /* Init cursors: */
        default_cursor = gdk_cursor_new(GDK_LEFT_PTR);
        ddisplay_set_all_cursor(default_cursor);
    }

    dia_register_plugins();
    dia_register_builtin_plugin(internal_plugin_init);

    load_all_sheets();     /* new mechanism */


    dia_log_message ("object defaults");
    dia_object_defaults_load (NULL, TRUE /* prefs.object_defaults_create_lazy */);

// debug_break();
    /* 2014-3-28 lcy  这里默认是检测standard - Box */
    if (object_get_type(CLASS_LINE) == NULL)
    {
        message_error(_("Couldn't find standard objects when looking for "
                        "object-libs; exiting...\n"));
        g_critical( _("Couldn't find standard objects when looking for "
                      "object-libs in '%s'; exiting...\n"), dia_get_lib_directory("dia"));
        factory_critical_error_exit(_("Couldn't find standard objects when looking for "
                        "object-libs; exiting...\n"));
    }

    persistence_load();

    /** Must load prefs after persistence */
    prefs_init();

    if (dia_is_interactive)
    {

        /* further initialization *before* reading files */
        active_tool = create_modify_tool();

        dia_log_message ("ui creation");
        if (use_integrated_ui)
        {
            create_integrated_ui();
        }
        else
        {
            create_toolbox();
            /* for the integrated ui case it is integrated */
            persistence_register_window_create("layer_window",
                                               (NullaryFunc*)&create_layer_dialog);
        }

        /*fill recent file menu */
        recent_file_history_init();

        /* Set up autosave to check every 5 minutes */
#if GLIB_CHECK_VERSION(2,14,0)
//        g_timeout_add_seconds(5*60, autosave_check_autosave, NULL);
#else
//        g_timeout_add(5*60*1000, autosave_check_autosave, NULL);
#endif

        /* Create Diagram Tree Window */
        create_tree_window();

        persistence_register_window_create("sheets_main_dialog",
                                           (NullaryFunc*)&sheets_dialog_create);

        /* In current setup, we can't find the autosaved files. */
        /*autosave_restore_documents();*/

    }
    factory_load_all_templates(); /*加载目录下的所有模版名*/
    dia_log_message ("diagrams");
    made_conversions = handle_all_diagrams(files, export_file_name,
                                           export_file_format, size, show_layers,
                                           input_directory, output_directory);

    if (dia_is_interactive && files == NULL && !nonew)
    {
        if (use_integrated_ui)
        {
            /* 这里是否打开软件就新建一个工程 */
//            GList * list;
//
//            file_new_callback(NULL);
//            list = dia_open_diagrams();
//            if (list)
//            {
//                Diagram * diagram = list->data;
//                diagram_update_extents(diagram);
//                diagram->is_default = TRUE;
//            }
        }
        else
        {
            gchar *filename = g_filename_from_utf8(_("Diagram1.dia"), -1, NULL, NULL, NULL);
            Diagram *diagram = new_diagram (filename);

            g_free(filename);

            if (diagram != NULL)
            {
                diagram_update_extents(diagram);
                diagram->is_default = TRUE;
                /* I think this is done in diagram_init() with a call to
                 * layer_dialog_update_diagram_list() */
                layer_dialog_set_diagram(diagram);
                new_display(diagram);
            }
        }
    }
    g_slist_free(files);
    if (made_conversions) exit(0);

//  dynobj_refresh_init();
    dia_log_message ("initialized");
}

gboolean
app_exit(void)
{
    factory_debug_to_log(factory_utf8("程序退出.\n"));
    GList *list;
    GSList *slist;
    if(logfd)
    {
        fclose(logfd);/* 关闭日志　*/
    }

    /*
     * The following "solves" a crash related to a second call of app_exit,
     * after gtk_main_quit was called. It may be a win32 gtk-1.3.x bug only
     * but the check shouldn't hurt on *ix either.          --hb
     */
    static gboolean app_exit_once = FALSE;

    if (app_exit_once)
    {
        g_error(_("This shouldn't happen.  Please file a bug report at bugzilla.gnome.org\n"
                  "describing how you can cause this message to appear.\n"));
        return FALSE;
    }

    if (diagram_modified_exists())
    {
        if (is_integrated_ui ())
        {
            GtkWidget                *dialog;
            int                       result;
            exit_dialog_item_array_t *items  = NULL;
            GList *                   list;
            Diagram *                 diagram;

            dialog = exit_dialog_make (GTK_WINDOW (interface_get_toolbox_shell ()),
                                       _("Exiting Dia"));

            list = dia_open_diagrams();
            while (list)
            {
                diagram = list->data;

                if (diagram_is_modified (diagram))
                {
                    const gchar * name = diagram_get_name (diagram);
                    const gchar * path = diagram->filename;
                    exit_dialog_add_item (dialog, name, path, diagram);
                }

                list = g_list_next (list);
            }

            result = exit_dialog_run (dialog, &items);
            if(dialog)
                gtk_widget_destroy (dialog);

            if (result == EXIT_DIALOG_EXIT_CANCEL)
            {
                return FALSE;
            }
            else if (result == EXIT_DIALOG_EXIT_SAVE_SELECTED)
            {
                int i;
                for (i = 0 ; i < items->array_size ; i++)
                {
                    gchar *filename;

                    diagram  = items->array[i].data;
                    filename = g_filename_from_utf8 (diagram->filename, -1, NULL, NULL, NULL);
                    diagram_update_extents (diagram);
                    if (!diagram_save (diagram, filename))
                    {
                        exit_dialog_free_items (items);
                        return FALSE;
                    }
                    g_free (filename);
                }
                exit_dialog_free_items (items);
            }
            else if (result == EXIT_DIALOG_EXIT_NO_SAVE)
            {
                list = dia_open_diagrams();
                while (list)
                {
                    diagram = list->data;

                    /* slight hack: don't ask again */
                    diagram_set_modified (diagram, FALSE);
                    undo_clear(diagram->undo);
                    list = g_list_next (list);
                }
            }
        }
        else
        {
            GtkWidget *dialog;
            GtkWidget *button;
            dialog = gtk_message_dialog_new(
                         NULL, GTK_DIALOG_MODAL,
                         GTK_MESSAGE_QUESTION,
                         GTK_BUTTONS_NONE, /* no standard buttons */
                         _("Quitting without saving modified diagrams"));
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                    _("Modified diagrams exist. "
                      "Are you sure you want to quit Dia "
                      "without saving them?"));

            gtk_window_set_title (GTK_WINDOW(dialog), _("Quit Dia"));

            button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
            gtk_dialog_add_action_widget (GTK_DIALOG(dialog), button, GTK_RESPONSE_CANCEL);
            GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
            gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);

            button = gtk_button_new_from_stock (GTK_STOCK_QUIT);
            gtk_dialog_add_action_widget (GTK_DIALOG(dialog), button, GTK_RESPONSE_OK);

            gtk_widget_show_all (dialog);

            if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
            {
                gtk_widget_destroy(dialog);
                return FALSE;
            }
            gtk_widget_destroy(dialog);
        }
    }
    prefs_save();

    persistence_save();

    dynobj_refresh_finish();

    dia_object_defaults_save (NULL);

    /* Free loads of stuff (toolbox) */

    list = dia_open_diagrams();
    while (list!=NULL)
    {
        Diagram *dia = (Diagram *)list->data;
        list = g_list_next(list);

        slist = dia->displays;
        while (slist!=NULL)
        {
            DDisplay *ddisp = (DDisplay *)slist->data;


            if(ddisp && ddisp->shell)
                gtk_widget_destroy(ddisp->shell);

            slist = g_slist_next(slist);
        }
        /* The diagram is freed when the last display is destroyed */
    }

    /* save pluginrc */
    if (dia_is_interactive)
        dia_pluginrc_write();

    gtk_main_quit();

    /* This printf seems to prevent a race condition with unrefs. */
    /* Yuck.  -Lars */
    /* Trying to live without it. -Lars 10/8/07*/
    /* g_print(_("Thank you for using Dia.\n")); */
    app_exit_once = TRUE;

    return TRUE;
}

static void create_user_dirs(void)
{
    gchar *dir, *subdir;

#ifdef G_OS_WIN32
    /* not necessary to quit the program with g_error, everywhere else
     * dia_config_filename appears to be used. Spit out a warning ...
     */
    if (!g_get_home_dir())
    {
        g_warning(_("Could not create per-user Dia config directory"));
        return; /* ... and return. Probably removes my one and only FAQ. --HB */
    }
#endif
    dir = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S ".dia", NULL);
    if (g_mkdir(dir, 0755) && errno != EEXIST)
    {
#ifndef G_OS_WIN32
        g_critical(_("Could not create per-user Dia config directory"));
        exit(1);
#else /* HB: it this really a reason to exit the program on *nix ? */
        g_warning(_("Could not create per-user Dia config directory. Please make "
                    "sure that the environment variable HOME points to an existing directory."));
#endif
    }

    /* it is no big deal if these directories can't be created */
    subdir = g_strconcat(dir, G_DIR_SEPARATOR_S "objects", NULL);
    g_mkdir(subdir, 0755);
    g_free(subdir);
    subdir = g_strconcat(dir, G_DIR_SEPARATOR_S "shapes", NULL);
    g_mkdir(subdir, 0755);
    g_free(subdir);
    subdir = g_strconcat(dir, G_DIR_SEPARATOR_S "sheets", NULL);
    g_mkdir(subdir, 0755);
    g_free(subdir);
    g_free(dir);
}

static PluginInitResult
internal_plugin_init(PluginInfo *info)
{
    if (!dia_plugin_info_init(info, "Internal",
                              _("Objects and filters internal to dia"),
                              NULL, NULL))
        return DIA_PLUGIN_INIT_ERROR;

    /* register the group object type */
    object_register_type(&group_type);
#ifdef USE_NEWGROUP
    object_register_type(&newgroup_type);
#endif

    /* register import filters */
    filter_register_import(&dia_import_filter);
    filter_register_import(&lcy_import_filter); /* 注册模版类型格式 */

    /* register export filters */
    /* Standard Dia format */
    filter_register_export(&dia_export_filter);

    return DIA_PLUGIN_INIT_OK;
}

static gboolean
handle_all_diagrams(GSList *files, char *export_file_name,
                    char *export_file_format, char *size, char *show_layers,
                    const gchar *input_directory, const gchar *output_directory)
{
    GSList *node = NULL;
    gboolean made_conversions = FALSE;

    for (node = files; node; node = node->next)
    {
        gchar *inpath = input_directory ? g_build_filename(input_directory, node->data, NULL) : node->data;
        made_conversions |=
            handle_initial_diagram(inpath, export_file_name,
                                   export_file_format, size, show_layers, output_directory);
        if (inpath != node->data)
            g_free(inpath);
    }
    return made_conversions;
}

/* --credits option. Added by Andrew Ferrier.

   Hopefully we're not ignoring anything too crucial by
   quitting directly after the credits.

   The phrasing of the English here might need changing
   if we switch from plural to non-plural (say, only
   one maintainer).
*/
static void
print_credits(gboolean credits)
{
    if (credits)
    {
        int i;
        const gint nauthors = (sizeof(authors) / sizeof(authors[0])) - 1;
        const gint ndocumentors = (sizeof(documentors) / sizeof(documentors[0])) - 1;

        g_print(_("The original author of Dia was:\n\n"));
        for (i = 0; i < NUMBER_OF_ORIG_AUTHORS; i++)
        {
            g_print("%s\n", authors[i]);
        }

        g_print(_("\nThe current maintainers of Dia are:\n\n"));
        for (i = NUMBER_OF_ORIG_AUTHORS; i < NUMBER_OF_ORIG_AUTHORS + NUMBER_OF_MAINTAINERS; i++)
        {
            g_print("%s\n", authors[i]);
        }

        g_print(_("\nOther authors are:\n\n"));
        for (i = NUMBER_OF_ORIG_AUTHORS + NUMBER_OF_MAINTAINERS; i < nauthors; i++)
        {
            g_print("%s\n", authors[i]);
        }

        g_print(_("\nDia is documented by:\n\n"));
        for (i = 0; i < ndocumentors; i++)
        {
            g_print("%s\n", documentors[i]);
        }

        exit(0);
    }
}

int app_is_embedded(void)
{
    return 0;
}
