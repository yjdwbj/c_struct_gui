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
#ifndef INTERFACE_H
#define INTERFACE_H

#include "display.h"

#include "menus.h"
#include "disp_callbacks.h"
#include "tool.h"
#include "sheet.h"
#include "app_procs.h"
#include "arrows.h"
#include "color_area.h"
#include "linewidth_area.h"
#include "attributes.h"

/* Integrated UI Constants */
#define  DIA_MAIN_WINDOW   "dia-main-window"
#define  DIA_MAIN_NOTEBOOK "dia-main-notebook"

/* Distributed UI Constants */
#define  DIA_TOOLBOX       "dia-toolbox"

#define TEMPL_SWIN "templ_swin"
#define ACT_SWIN "act_swin"


void create_integrated_ui (void);
void factory_template_add_item(SheetObject *obj,const gchar *sname);
DIAVAR Sheet *get_sheet_by_name(const gchar *name);
DIAVAR void factory_template_open_template_filename(const gchar *name);

gboolean integrated_ui_main_toolbar_is_showing (void);
void     integrated_ui_main_toolbar_show (void);
void     integrated_ui_main_toolbar_hide (void);

gboolean integrated_ui_main_statusbar_is_showing (void);
void     integrated_ui_main_statusbar_show (void);
void     integrated_ui_main_statusbar_hide (void);

gboolean integrated_ui_layer_view_is_showing (void);
void     integrated_ui_layer_view_show (void);
void     integrated_ui_layer_view_hide (void);

int is_integrated_ui (void);

void create_display_shell(DDisplay *ddisp,
			  int width, int height,
			  char *title, int use_mbar, int top_level_window);

void create_toolbox (void);
extern __declspec(dllexport) void toolbox_show(void);
void toolbox_hide(void);

DIAVAR GtkWidget *interface_get_toolbox_shell(void);

void tool_select_callback(GtkWidget *widget, gpointer data);
void create_integrated_ui (void);
void create_tree_window(void);

void create_sheets(GtkWidget *parent);
void create_template_sheets(GtkWidget *parent);
extern GtkWidget *modify_tool_button;

void view_zoom_set (float zoom_factor); /* zoom_factor is 10 * percentage */

typedef struct _ToolButton ToolButton;

typedef struct _ToolButtonData ToolButtonData;

struct _ToolButtonData
{
  ToolType type;
  gpointer extra_data;
  gpointer user_data; /* Used by create_object_tool */
  GtkWidget *widget;
};

struct _ToolButton
{
  gchar **icon_data;
  char  *tool_desc;
  char	*tool_accelerator;
  char  *action_name;
  ToolButtonData callback_data;
};

extern const int num_tools;
extern ToolButton tool_data[];
extern gchar *interface_current_sheet_name;

void tool_select_update (GtkWidget *w, gpointer   data);
void fill_sheet_menu(void);

#ifndef GTK_DISABLE_DEPRECATED
extern GtkTooltips *tool_tips;
#endif

void close_notebook_page_callback (GtkButton *button, gpointer user_data);

#endif /* INTERFACE_H */
