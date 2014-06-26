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
#include <math.h>

#include "modify_tool.h"
#include "handle_ops.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "message.h"
#include "properties-dialog.h"
#include "select.h"
#include "preferences.h"
#include "cursor.h"
#include "highlight.h"
#include "textedit.h"
#include "textline.h"

#include "parent.h"
#include "diacanvas.h"
#include "prop_text.h"
#include "gtk/gtk.h"


static DiaObject *click_select_object(DDisplay *ddisp, Point *clickedpoint,
                                      GdkEventButton *event);
static int do_if_clicked_handle(DDisplay *ddisp, ModifyTool *tool,
                                Point *clickedpoint,
                                GdkEventButton *event);
static void modify_button_press(ModifyTool *tool, GdkEventButton *event,
                                DDisplay *ddisp);
static void modify_button_hold(ModifyTool *tool, GdkEventButton *event,
                               DDisplay *ddisp);
static void modify_button_release(ModifyTool *tool, GdkEventButton *event,
                                  DDisplay *ddisp);
static void modify_motion(ModifyTool *tool, GdkEventMotion *event,
                          DDisplay *ddisp);
static void modify_double_click(ModifyTool *tool, GdkEventButton *event,
                                DDisplay *ddisp);
static void modify_make_text_edit(DDisplay *ddisp, DiaObject *obj,
                                  Point *clickedpoint);
static void modify_start_text_edit(DDisplay *ddisp, Text *text, DiaObject *obj,
                                   Point *clickedpoint);


Tool *
create_modify_tool(void)
{
    ModifyTool *tool;

    tool = g_new0(ModifyTool, 1);
    tool->tool.type = MODIFY_TOOL;
    tool->tool.button_press_func = (ButtonPressFunc) &modify_button_press;
    tool->tool.button_hold_func = (ButtonHoldFunc) &modify_button_hold;
    tool->tool.button_release_func = (ButtonReleaseFunc) &modify_button_release;
    tool->tool.motion_func = (MotionFunc) &modify_motion;
    tool->tool.double_click_func = (DoubleClickFunc) &modify_double_click;
    tool->gc = NULL;
    tool->state = STATE_NONE;
    tool->break_connections = 0;
    tool->auto_scrolled = FALSE;

    tool->orig_pos = NULL;

    return (Tool *)tool;
}

static ModifierKeys
gdk_event_to_dia_ModifierKeys(guint event_state)
{
    ModifierKeys mod = MODIFIER_NONE;
    if (event_state & GDK_SHIFT_MASK)
        mod |= MODIFIER_SHIFT;
    /*     Used intenally do not propagate
     *     if (event_state & GDK_CONTROL_MASK)
                 mod | MODIFIER_CONTROL;
                 */
    return mod;
}


void
free_modify_tool(Tool *tool)
{
    ModifyTool *mtool = (ModifyTool *)tool;
    if (mtool->gc)
        gdk_gc_unref(mtool->gc);
    g_free(mtool);
}

/*
  This function is buggy. Fix it later!
static void
transitive_select(DDisplay *ddisp, Point *clickedpoint, DiaObject *obj)
{
  guint i;
  GList *j;
  DiaObject *obj1;

  for(i = 0; i < obj->num_connections; i++) {
    printf("%d\n", i);
    j = obj->connections[i]->connected;
    while(j != NULL && (obj1 = (DiaObject *)j->data) != NULL) {
      diagram_select(ddisp->diagram, obj1);
      obj1->ops->select(obj1, clickedpoint,
			(Renderer *)ddisp->renderer);
      transitive_select(ddisp, clickedpoint, obj1);
      j = g_list_next(j);
    }
  }
}
*/

static DiaObject *
click_select_object(DDisplay *ddisp, Point *clickedpoint,
                    GdkEventButton *event)
{
    Diagram *diagram;
    real click_distance;
    DiaObject *obj;

    diagram = ddisp->diagram;

    /* Find the closest object to select it: */

    click_distance = ddisplay_untransform_length(ddisp, 3.0);

    obj = diagram_find_clicked_object(diagram, clickedpoint,
                                      click_distance);


    if (obj==NULL)
    {
        GList *plist = g_hash_table_get_values(diagram->data->active_layer->defnames);
        if(plist)  /* 清空所有高亮 */
        {
            DiaObject *dia = plist->data;
            dia->ops->reset_objectsfillcolor(obj);
        }
        diagram_redraw_all();
    }
    else
    {
        /* Selected an object. */
        GList *already;
        /*printf("Selected object!\n");*/

        already = g_list_find(diagram->data->selected, obj);
        if (already == NULL)   /* Not already selected */
        {
            /*printf("Not already selected\n");*/

            if (!(event->state & GDK_SHIFT_MASK))
            {
                /* Not Multi-select => remove current selection */
                diagram_remove_all_selected(diagram, TRUE);
            }

            diagram_select(diagram, obj);
            if(obj->type == object_get_type(CLASS_STRUCT))
            {
                obj->ops->reset_objectsfillcolor(obj); /* 清空所有高亮 */
                diagram_redraw_all();
                obj->ops->SearchConnLink(obj,3); /* 2014-6-23 lcy 回调查找最近三层的连线 */
//                obj->ops->update_fillcolor();
                diagram_redraw_all(); /* 这个必须要重写画布上有的所有对像 */
            }

            /* To be removed once text edit mode is stable.  By then,
             * we don't want to automatically edit selected objects.
            textedit_activate_object(ddisp, obj, clickedpoint);
                */

            /*
            This stuff is buggy, fix it later.
                 if (event->state & GDK_CONTROL_MASK) {
            transitive_select(ddisp, clickedpoint, obj);
                 }
                 */

            ddisplay_do_update_menu_sensitivity(ddisp);
            object_add_updates_list(diagram->data->selected, diagram);
            diagram_flush(diagram);

            return obj;
        }
        else     /* Clicked on already selected. */
        {
            /*printf("Already selected\n");*/
            /* To be removed once text edit mode is stable.  By then,
             * we don't want to automatically edit selected objects.
            textedit_activate_object(ddisp, obj, clickedpoint);
            */
            object_add_updates_list(diagram->data->selected, diagram);
            diagram_flush(diagram);

            if (event->state & GDK_SHIFT_MASK)   /* Multi-select */
            {
                /* Remove the selected selected */
                ddisplay_do_update_menu_sensitivity(ddisp);
                diagram_unselect_object(diagram, (DiaObject *)already->data);
                diagram_flush(ddisp->diagram);
            }
            else
            {
                /* Maybe start editing text */
#ifdef NEW_TEXT_EDIT
                modify_make_text_edit(ddisp, obj, clickedpoint);
#endif
                return obj;
            }
        }
    } /* Else part moved to allow union/intersection select */

    return NULL;
}

static glong
time_micro()
{
    GTimeVal tv;

    g_get_current_time(&tv);
    return tv.tv_sec*G_USEC_PER_SEC+tv.tv_usec;
}

static int do_if_clicked_handle(DDisplay *ddisp, ModifyTool *tool,
                                Point *clickedpoint, GdkEventButton *event)
{
    DiaObject *obj;
    Handle *handle;
    real dist;

    handle = NULL;
    dist = diagram_find_closest_handle(ddisp->diagram, &handle,
                                       &obj, clickedpoint);
    if  (handle_is_clicked(ddisp, handle, clickedpoint))
    {
        tool->state = STATE_MOVE_HANDLE;
        tool->break_connections = TRUE;
        tool->last_to = handle->pos;
        tool->handle = handle;
        tool->object = obj;
        gdk_pointer_grab (ddisp->canvas->window, FALSE,
                          GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                          NULL, NULL, event->time);
        tool->start_at = handle->pos;
        tool->start_time = time_micro();
        ddisplay_set_all_cursor(get_cursor(CURSOR_SCROLL));
        return TRUE;
    }
    return FALSE;
}

static void
modify_button_press(ModifyTool *tool, GdkEventButton *event,
                    DDisplay *ddisp)
{

    Point clickedpoint;
    DiaObject *clicked_obj;
    gboolean some_selected;

    ddisplay_untransform_coords(ddisp,
                                (int)event->x, (int)event->y,
                                &clickedpoint.x, &clickedpoint.y);

    /* don't got to single handle movement if there is more than one object selected */
    some_selected = g_list_length (ddisp->diagram->data->selected) > 1;
    if (!some_selected && do_if_clicked_handle(ddisp, tool, &clickedpoint, event))
        return;

    clicked_obj = click_select_object(ddisp, &clickedpoint, event);
    if (!some_selected && do_if_clicked_handle(ddisp, tool, &clickedpoint, event))
        return;

    if ( clicked_obj != NULL )
    {
        tool->state = STATE_MOVE_OBJECT;
        tool->object = clicked_obj;
        tool->move_compensate = clicked_obj->position;
        point_sub(&tool->move_compensate, &clickedpoint);
        tool->break_connections = TRUE;
        gdk_pointer_grab (ddisp->canvas->window, FALSE,
                          GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                          NULL, NULL, event->time);
        tool->start_at = clickedpoint;
        tool->start_time = time_micro();
        ddisplay_set_all_cursor(get_cursor(CURSOR_SCROLL));
    }
    else
    {
        tool->state = STATE_BOX_SELECT;
        tool->start_box = clickedpoint;
        tool->end_box = clickedpoint;
        tool->x1 = tool->x2 = (int) event->x;
        tool->y1 = tool->y2 = (int) event->y;

        if (tool->gc == NULL)
        {
            tool->gc = gdk_gc_new(ddisp->canvas->window);
            gdk_gc_set_line_attributes(tool->gc, 1, GDK_LINE_ON_OFF_DASH,
                                       GDK_CAP_BUTT, GDK_JOIN_MITER);
            gdk_gc_set_foreground(tool->gc, &color_gdk_white);
            gdk_gc_set_function(tool->gc, GDK_XOR);
        }

        gdk_draw_rectangle (ddisp->canvas->window, tool->gc, FALSE,
                            tool->x1, tool->y1,
                            tool->x2 - tool->x1, tool->y2 - tool->y1);

        gdk_pointer_grab (ddisp->canvas->window, FALSE,
                          GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                          NULL, NULL, event->time);
    }
}


static void
modify_button_hold(ModifyTool *tool, GdkEventButton *event,
                   DDisplay *ddisp)
{
    Point clickedpoint;

    switch (tool->state)
    {
    case STATE_MOVE_OBJECT:
        /* A button hold is as if user was moving object - if it is
         * a text object and can be edited, then the move is cancelled */
        ddisplay_untransform_coords(ddisp,
                                    (int)event->x, (int)event->y,
                                    &clickedpoint.x, &clickedpoint.y);

        if (tool->object != NULL &&
                diagram_is_selected(ddisp->diagram, tool->object))
        {
            if (textedit_activate_object(ddisp, tool->object, &clickedpoint))
            {
                /* Return tool to normal state - object is text and is in edit */
                gdk_pointer_ungrab (event->time);
                tool->orig_pos = NULL;
                tool->state = STATE_NONE;
                /* Activate Text Edit */
                gtk_action_activate (menus_get_action ("ToolsTextedit"));
            }
        }
        break;
    case STATE_MOVE_HANDLE:
        break;
    case STATE_BOX_SELECT:
        break;
    case STATE_NONE:
        break;
    default:
        message_error("Internal error: Strange state in modify_tool (button_hold)\n");
    }
}

static void
modify_double_click(ModifyTool *tool, GdkEventButton *event,
                    DDisplay *ddisp)
{
    if(factory_get_toplevel_dialog()) /* 如果已经打开一个窗口,就无视这个操作*/
        return;
    Point clickedpoint;
    DiaObject *clicked_obj;

    ddisplay_untransform_coords(ddisp,
                                (int)event->x, (int)event->y,
                                &clickedpoint.x, &clickedpoint.y);

    clicked_obj = click_select_object(ddisp, &clickedpoint, event);

    if ( clicked_obj != NULL )
    {
//        gchar *n1 = g_strdup("Standard - Line");
//        if(!g_ascii_strncasecmp(clicked_obj->type->name,n1,strlen(n1))) /* 线条不显示属性窗口*/
        if(clicked_obj->type == object_get_type(CLASS_LINE))
        {
//            g_free(n1);
            return;
        }
//        g_free(n1);
        object_list_properties_show(ddisp->diagram, ddisp->diagram->data->selected);
    }
    else     /* No object selected */
    {
        /*printf("didn't select object\n");*/
        if (!(event->state & GDK_SHIFT_MASK))
        {
            /* Not Multi-select => Remove all selected */
            ddisplay_do_update_menu_sensitivity(ddisp);
            diagram_remove_all_selected(ddisp->diagram, TRUE);
            diagram_flush(ddisp->diagram);
        }
    }
}

#define MIN_PIXELS 10

/** Makes sure that objects aren't accidentally moved when double-clicking
 * for properties.  Objects do not move unless double click time has passed
 * or the move is 'significant'.  Allowing the 'significant' move makes a
 * regular grab-and-move less jerky.
 *
 * There's a slight chance that the user could move outside the
 * minimum movement limit and back in within the double click time
 * (normally .25 seconds), but that's very, very unlikely.  If
 * it happens, the cursor wouldn't reset just right.
 */

static gboolean
modify_move_already(ModifyTool *tool, DDisplay *ddisp, Point *to)
{
    static gboolean settings_taken = FALSE;
    static int double_click_time = 250;
    real dist;

    if (!settings_taken)
    {
        /* One could argue that if the settings were updated while running,
           we should re-read them.  But I don't see any way to get notified,
           and I don't want to do this whole thing for each bit of the
           move --Lars */
        GtkSettings *settings = gtk_settings_get_default();
        if (settings == NULL)
        {
            g_message(_("Couldn't get GTK settings"));
        }
        else
        {
            g_object_get(G_OBJECT(settings),
                         "gtk-double-click-time", &double_click_time, NULL);
        }
        settings_taken = TRUE;
    }
    if (tool->start_time < time_micro()-double_click_time*1000)
    {
        return TRUE;
    }
    dist = distance_point_point_manhattan(&tool->start_at, to);
    if (ddisp->grid.snap)
    {
        real grid_x = ddisp->diagram->grid.width_x;
        real grid_y = ddisp->diagram->grid.width_y;
        if (dist > grid_x || dist > grid_y)
        {
            return TRUE;
        }
    }
    if (ddisplay_transform_length(ddisp, dist) > MIN_PIXELS)
    {
        return (ddisplay_transform_length(ddisp, dist) > MIN_PIXELS);
    }
    else
    {
        return FALSE;
    }
}

/** Used for highlighting mainpoint connections. */
static Color mainpoint_color = { 1.0, 0.8, 0.0 };
/** Used for highlighting normal connections. */
static Color cp_color = { 1.0, 0.0, 0.0 };

static void
modify_motion(ModifyTool *tool, GdkEventMotion *event,
              DDisplay *ddisp)
{
    Point to;
    Point now, delta, full_delta;
    gboolean auto_scroll, vertical = FALSE;
    ConnectionPoint *connectionpoint = NULL;
    ObjectChange *objchange;

    if (tool->state==STATE_NONE)
        return; /* Fast path... */

    auto_scroll = ddisplay_autoscroll(ddisp, event->x, event->y);

    ddisplay_untransform_coords(ddisp, event->x, event->y, &to.x, &to.y);

    if (!modify_move_already(tool, ddisp, &to)) return;

    switch (tool->state)
    {
    case STATE_MOVE_OBJECT:

        if (tool->orig_pos == NULL)
        {
            GList *list;
            int i;
            DiaObject *obj;

            /* consider non-selected children affected */
            list = parent_list_affected(ddisp->diagram->data->selected);
            tool->orig_pos = g_new(Point, g_list_length(list));
            i=0;
            while (list != NULL)
            {
                obj = (DiaObject *)  list->data;
                tool->orig_pos[i] = obj->position;
                list = g_list_next(list);
                i++;
            }
        }

        if (tool->break_connections)
            diagram_unconnect_selected(ddisp->diagram); /* Pushes UNDO info */

        if (event->state & GDK_CONTROL_MASK)
        {
            full_delta = to;
            point_sub(&full_delta, &tool->start_at);
            vertical = (fabs(full_delta.x) < fabs(full_delta.y));
        }

        point_add(&to, &tool->move_compensate);
        snap_to_grid(ddisp, &to.x, &to.y);

        now = tool->object->position;

        delta = to;
        point_sub(&delta, &now);

        if (event->state & GDK_CONTROL_MASK)
        {
            /* Up-down or left-right */
            if (vertical)
            {
                delta.x = tool->start_at.x + tool->move_compensate.x - now.x;
            }
            else
            {
                delta.y = tool->start_at.y + tool->move_compensate.y - now.y;
            }
        }

        object_add_updates_list(ddisp->diagram->data->selected, ddisp->diagram);
        objchange = object_list_move_delta(ddisp->diagram->data->selected, &delta);
        if (objchange != NULL)
        {
            undo_object_change(ddisp->diagram, tool->object, objchange);
        }
        object_add_updates_list(ddisp->diagram->data->selected, ddisp->diagram);

        object_add_updates(tool->object, ddisp->diagram);

        /* Put current mouse position in status bar */
        {
            gchar *postext;
            GtkStatusbar *statusbar = GTK_STATUSBAR (ddisp->modified_status);
            guint context_id = gtk_statusbar_get_context_id (statusbar, "ObjectPos");
            gtk_statusbar_pop (statusbar, context_id);
            postext = g_strdup_printf("%.3f, %.3f - %.3f, %.3f",
                                      tool->object->bounding_box.left,
                                      tool->object->bounding_box.top,
                                      tool->object->bounding_box.right,
                                      tool->object->bounding_box.bottom);

            gtk_statusbar_pop (statusbar, context_id);
            gtk_statusbar_push (statusbar, context_id, postext);

            g_free(postext);
        }

        diagram_update_connections_selection(ddisp->diagram);
        diagram_flush(ddisp->diagram);
        break;
    case STATE_MOVE_HANDLE:
        full_delta = to;
        point_sub(&full_delta, &tool->start_at);

        /* make sure resizing is restricted to its parent */


        /* if resize was blocked by parent, that means the resizing was
          outward, thus it won't bother the children so we don't have to
          check the children */
        if (!parent_handle_move_out_check(tool->object, &to))
            parent_handle_move_in_check(tool->object, &to, &tool->start_at);

        if (event->state & GDK_CONTROL_MASK)
            vertical = (fabs(full_delta.x) < fabs(full_delta.y));

        highlight_reset_all(ddisp->diagram);
        if ((tool->handle->connect_type != HANDLE_NONCONNECTABLE))
        {
            /* Move to ConnectionPoint if near: */
            connectionpoint =
                object_find_connectpoint_display(ddisp, &to, tool->object, TRUE);
            if (connectionpoint != NULL)
            {
                Color *hi_color;
                to = connectionpoint->pos;
                if (connectionpoint->flags & CP_FLAGS_MAIN)
                {
                    hi_color = &mainpoint_color;
                }
                else
                {
                    hi_color = &cp_color;
                }
                highlight_object(connectionpoint->object, hi_color, ddisp->diagram);
                ddisplay_set_all_cursor(get_cursor(CURSOR_CONNECT));
            }
        }
        if (connectionpoint == NULL)
        {
            /* No connectionopoint near, then snap to grid (if enabled) */
            snap_to_grid(ddisp, &to.x, &to.y);
            ddisplay_set_all_cursor(get_cursor(CURSOR_SCROLL));
        }

        if (tool->break_connections)
        {
            /* break connections to the handle currently selected. */
            if (tool->handle->connected_to!=NULL)
            {
                Change *change = undo_unconnect(ddisp->diagram, tool->object,
                                                tool->handle);

                (change->apply)(change, ddisp->diagram);
            }
        }

        if (event->state & GDK_CONTROL_MASK)
        {
            /* Up-down or left-right */
            if (vertical)
            {
                to.x = tool->start_at.x;
            }
            else
            {
                to.y = tool->start_at.y;
            }
        }

        if (tool->orig_pos == NULL)
        {
            tool->orig_pos = g_new(Point, 1);
            *tool->orig_pos = tool->handle->pos;
        }

        /* Put current mouse position in status bar */
        {
            gchar *postext;
            GtkStatusbar *statusbar = GTK_STATUSBAR (ddisp->modified_status);
            guint context_id = gtk_statusbar_get_context_id (statusbar, "ObjectPos");

            if (tool->object)   /* play safe */
            {
                real w = tool->object->bounding_box.right - tool->object->bounding_box.left;
                real h = tool->object->bounding_box.bottom - tool->object->bounding_box.top;
                postext = g_strdup_printf("%.3f, %.3f (%.3fx%.3f)", to.x, to.y, w, h);
            }
            else
            {
                postext = g_strdup_printf("%.3f, %.3f", to.x, to.y);
            }

            gtk_statusbar_pop (statusbar, context_id);
            gtk_statusbar_push (statusbar, context_id, postext);

            g_free(postext);
        }

        object_add_updates(tool->object, ddisp->diagram);

        /* Handle undo */
        objchange = tool->object->ops->move_handle(tool->object, tool->handle,
                    &to, connectionpoint,
                    HANDLE_MOVE_USER, gdk_event_to_dia_ModifierKeys(event->state));
        if (objchange != NULL)
        {
            undo_object_change(ddisp->diagram, tool->object, objchange);
        }
        object_add_updates(tool->object, ddisp->diagram);

        diagram_update_connections_selection(ddisp->diagram);
        diagram_flush(ddisp->diagram);
        break;
    case STATE_BOX_SELECT:

        if (!auto_scroll && !tool->auto_scrolled)
        {
            gdk_draw_rectangle (ddisp->canvas->window, tool->gc, FALSE,
                                tool->x1, tool->y1,
                                tool->x2 - tool->x1, tool->y2 - tool->y1);
        }

        tool->end_box = to;

        ddisplay_transform_coords(ddisp,
                                  MIN(tool->start_box.x, tool->end_box.x),
                                  MIN(tool->start_box.y, tool->end_box.y),
                                  &tool->x1, &tool->y1);
        ddisplay_transform_coords(ddisp,
                                  MAX(tool->start_box.x, tool->end_box.x),
                                  MAX(tool->start_box.y, tool->end_box.y),
                                  &tool->x2, &tool->y2);

        gdk_draw_rectangle (ddisp->canvas->window, tool->gc, FALSE,
                            tool->x1, tool->y1,
                            tool->x2 - tool->x1, tool->y2 - tool->y1);
        break;
    case STATE_NONE:

        break;
    default:
        message_error("Internal error: Strange state in modify_tool\n");
    }

    tool->last_to = to;
    tool->auto_scrolled = auto_scroll;
}

/** Find the list of objects selected by current rubberbanding.
 * The list should be freed after use. */
static GList *
find_selected_objects(DDisplay *ddisp, ModifyTool *tool)
{
    Rectangle r;
    r.left = MIN(tool->start_box.x, tool->end_box.x);
    r.right = MAX(tool->start_box.x, tool->end_box.x);
    r.top = MIN(tool->start_box.y, tool->end_box.y);
    r.bottom = MAX(tool->start_box.y, tool->end_box.y);

    if (prefs.reverse_rubberbanding_intersects &&
            tool->start_box.x > tool->end_box.x)
    {
        return
            layer_find_objects_intersecting_rectangle(ddisp->diagram->data->active_layer, &r);
    }
    else
    {
        return
            layer_find_objects_in_rectangle(ddisp->diagram->data->active_layer, &r);
    }
}

static void
modify_button_release(ModifyTool *tool, GdkEventButton *event,
                      DDisplay *ddisp)
{
    Point *dest_pos, to;
    GList *list;
    int i;
    DiaObject *active_obj = NULL;
    ObjectChange *objchange;
    Focus *active_focus;

    tool->break_connections = FALSE;
    ddisplay_set_all_cursor(default_cursor);

    /* remove position from status bar */
    {
        GtkStatusbar *statusbar = GTK_STATUSBAR (ddisp->modified_status);
        guint context_id = gtk_statusbar_get_context_id (statusbar, "ObjectPos");
        gtk_statusbar_pop (statusbar, context_id);
    }
    switch (tool->state)
    {
    case STATE_MOVE_OBJECT:
        /* Return to normal state */
        gdk_pointer_ungrab (event->time);

        ddisplay_untransform_coords(ddisp, event->x, event->y, &to.x, &to.y);
        if (!modify_move_already(tool, ddisp, &to))
        {
            tool->orig_pos = NULL;
            tool->state = STATE_NONE;
            return;
        }

        diagram_update_connections_selection(ddisp->diagram);

        if (tool->orig_pos != NULL)
        {
            /* consider the non-selected children affected */
            list = parent_list_affected(ddisp->diagram->data->selected);
            dest_pos = g_new(Point, g_list_length(list));
            i=0;
            while (list != NULL)
            {
                DiaObject *obj = (DiaObject *)  list->data;
                dest_pos[i] = obj->position;
                list = g_list_next(list);
                i++;
            }

            undo_move_objects(ddisp->diagram, tool->orig_pos, dest_pos,
                              parent_list_affected(ddisp->diagram->data->selected));
        }

        ddisplay_connect_selected(ddisp); /* pushes UNDO info */
        diagram_update_extents(ddisp->diagram);
        diagram_modified(ddisp->diagram);
        diagram_flush(ddisp->diagram);

        undo_set_transactionpoint(ddisp->diagram->undo);

        tool->orig_pos = NULL;
        tool->state = STATE_NONE;
        break;
    case STATE_MOVE_HANDLE:
        gdk_pointer_ungrab (event->time);
        tool->state = STATE_NONE;

        if (tool->orig_pos != NULL)
        {
            undo_move_handle(ddisp->diagram, tool->handle, tool->object,
                             *tool->orig_pos, tool->last_to);
        }

        /* Final move: */
        object_add_updates(tool->object, ddisp->diagram);
        objchange = tool->object->ops->move_handle(tool->object, tool->handle,
                    &tool->last_to, NULL,
                    HANDLE_MOVE_USER_FINAL,gdk_event_to_dia_ModifierKeys(event->state));
        if (objchange != NULL)
        {
            undo_object_change(ddisp->diagram, tool->object, objchange);
        }

        object_add_updates(tool->object, ddisp->diagram);

        /* Connect if possible: */
        if (tool->handle->connect_type != HANDLE_NONCONNECTABLE)
        {
            object_connect_display(ddisp, tool->object, tool->handle, TRUE); /* pushes UNDO info */
            diagram_update_connections_selection(ddisp->diagram);
        }

        highlight_reset_all(ddisp->diagram);
        diagram_flush(ddisp->diagram);

        diagram_modified(ddisp->diagram);
        diagram_update_extents(ddisp->diagram);

        undo_set_transactionpoint(ddisp->diagram->undo);

        if (tool->orig_pos != NULL)
        {
            g_free(tool->orig_pos);
            tool->orig_pos = NULL;
        }

        break;
    case STATE_BOX_SELECT:

        gdk_pointer_ungrab (event->time);
        /* Remember the currently active object for reactivating. */
        active_focus = get_active_focus((DiagramData *) ddisp->diagram);
        active_obj = active_focus!=NULL?focus_get_object(active_focus):NULL;
        /* Remove last box: */
        if (!tool->auto_scrolled)
        {
            gdk_draw_rectangle (ddisp->canvas->window, tool->gc, FALSE,
                                tool->x1, tool->y1,
                                tool->x2 - tool->x1, tool->y2 - tool->y1);
        }

        {
            GList *list, *list_to_free;

            list = list_to_free = find_selected_objects(ddisp, tool);

            if (selection_style == SELECT_REPLACE &&
                    !(event->state & GDK_SHIFT_MASK))
            {
                /* Not Multi-select => Remove all selected */
                diagram_remove_all_selected(ddisp->diagram, TRUE);
            }

            if (selection_style == SELECT_INTERSECTION)
            {
                GList *intersection = NULL;

                while (list != NULL)
                {
                    DiaObject *obj = (DiaObject *)list->data;

                    if (diagram_is_selected(ddisp->diagram, obj))
                    {
                        intersection = g_list_append(intersection, obj);
                    }

                    list = g_list_next(list);
                }
                list = intersection;
                diagram_remove_all_selected(ddisp->diagram, TRUE);
                while (list != NULL)
                {
                    DiaObject *obj = (DiaObject *)list->data;

                    diagram_select(ddisp->diagram, obj);

                    list = g_list_next(list);
                }
                g_list_free(intersection);
            }
            else
            {
                while (list != NULL)
                {
                    DiaObject *obj = (DiaObject *)list->data;

                    if (selection_style == SELECT_REMOVE)
                    {
                        if (diagram_is_selected(ddisp->diagram, obj))
                            diagram_unselect_object(ddisp->diagram, obj);
                    }
                    else if (selection_style == SELECT_INVERT)
                    {
                        if (diagram_is_selected(ddisp->diagram, obj))
                            diagram_unselect_object(ddisp->diagram, obj);
                        else
                            diagram_select(ddisp->diagram, obj);
                    }
                    else
                    {
                        if (!diagram_is_selected(ddisp->diagram, obj))
                            diagram_select(ddisp->diagram, obj);
                    }

                    list = g_list_next(list);
                }
            }

            g_list_free(list_to_free);

        }

        /* To be removed once text edit mode is stable.  By then,
         * we don't want to automatically edit selected objects.
        if (active_obj != NULL &&
        diagram_is_selected(ddisp->diagram, active_obj)) {
          textedit_activate_object(ddisp, active_obj, NULL);
        } else {
          textedit_activate_first(ddisp);
        }
        */
        ddisplay_do_update_menu_sensitivity(ddisp);
        ddisplay_flush(ddisp);

        tool->state = STATE_NONE;
        break;
    case STATE_NONE:
        break;
    default:
        message_error("Internal error: Strange state in modify_tool\n");

    }
}

#define EDIT_BORDER_WIDTH 5

static gboolean
modify_edit_end(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
    GtkTextView *view = GTK_TEXT_VIEW(widget);
    DiaObject *obj = (DiaObject*)data;
    GQuark quark = g_quark_from_string(PROP_TYPE_TEXT);
    const PropDescription *props = obj->ops->describe_props(obj);
    int i;

    printf("Ending focus\n");

    for (i = 0; props[i].name != NULL; i++)
    {
        printf("Testing to remove: %s\n", props[i].name);
        if (props[i].type_quark == quark)
        {
            GPtrArray *textprops = g_ptr_array_sized_new(1);
            TextProperty *textprop;
            Property *prop = props[i].ops->new_prop(&props[i], pdtpp_true);
            GtkTextBuffer *buf;
            GtkTextIter start, end;

            printf("Going to stop %d\n", i);
            buf = gtk_text_view_get_buffer(view);
            g_ptr_array_add(textprops, prop);
            obj->ops->get_props(obj, textprops);
            textprop = (TextProperty*)prop;
            if (textprop->text_data != NULL) g_free(textprop->text_data);
            gtk_text_buffer_get_bounds(buf, &start, &end);
            textprop->text_data = gtk_text_buffer_get_text(buf, &start, &end, TRUE);
            printf("Setting text %s\n", textprop->text_data);
            obj->ops->set_props(obj, textprops);
            gtk_widget_destroy(widget);
        }
    }
    return FALSE;
}

void
modify_start_text_edit(DDisplay *ddisp, Text *text, DiaObject *obj, Point *clickedpoint)
{
    GtkWidget *view = gtk_text_view_new();
    int x, y, i;
    GtkTextBuffer *buf;
    GtkTextTag *fonttag;
    GtkTextIter start, end;
    Rectangle text_bbox;

    printf("modify_start_text_edit\n");
    /* This might need to account for zoom factor. */
    text_calc_boundingbox(text, &text_bbox);
    ddisplay_transform_coords(ddisp,
                              text_bbox.left,
                              text_bbox.top,
                              &x, &y);
    dia_canvas_put(DIA_CANVAS(ddisp->canvas), view,
                   x-EDIT_BORDER_WIDTH, y-EDIT_BORDER_WIDTH);
    buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    for (i = 0; i < text->numlines; i++)
    {
        gtk_text_buffer_insert_at_cursor(buf, text_get_line(text, i), -1);
    }
    fonttag =
        gtk_text_buffer_create_tag(buf,
                                   NULL,
                                   "font-desc",
                                   dia_font_get_description(text->font),
                                   NULL);
    gtk_text_buffer_get_bounds(buf, &start, &end);
    gtk_text_buffer_apply_tag(buf, fonttag, &start, &end);

    printf("Above lines %d below %d\n",
           gtk_text_view_get_pixels_above_lines(GTK_TEXT_VIEW(view)),
           gtk_text_view_get_pixels_below_lines(GTK_TEXT_VIEW(view)));

    gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(view),
                                         GTK_TEXT_WINDOW_LEFT,
                                         EDIT_BORDER_WIDTH);
    gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(view),
                                         GTK_TEXT_WINDOW_RIGHT,
                                         EDIT_BORDER_WIDTH);
    gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(view),
                                         GTK_TEXT_WINDOW_TOP,
                                         EDIT_BORDER_WIDTH);
    gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(view),
                                         GTK_TEXT_WINDOW_BOTTOM,
                                         EDIT_BORDER_WIDTH);
    /* Using deprecated function because the fucking gobject documentation
     * fucking sucks. */
#ifdef NEW_TEXT_EDIT
    gtk_signal_connect(GTK_OBJECT(view), "focus-out-event",
                       modify_edit_end, obj);
#endif
    gtk_widget_grab_focus(view);
    gtk_widget_show(view);
}

void
modify_make_text_edit(DDisplay *ddisp, DiaObject *obj, Point *clickedpoint)
{
    const PropDescription *props = obj->ops->describe_props(obj);
    int i;
    for (i = 0; props[i].name != NULL; i++)
    {
        GQuark type = g_quark_from_string(PROP_TYPE_TEXT);
        printf("Testing %s\n", props[i].type);
        if (props[i].type_quark == type)
        {
            GtkWidget *view = gtk_text_view_new();
            GPtrArray *textprops = g_ptr_array_sized_new(1);
            TextProperty *textprop;
            Property *prop = props[i].ops->new_prop(&props[i], pdtpp_true);
            int x, y;
            GtkTextBuffer *buf;
            GtkTextTag *fonttag;
            GtkTextIter start, end;
            real ascent;
            int ascent_pixels;
            TextLine *temp_line;

            g_ptr_array_add(textprops, prop);

            printf("Found text prop %d\n", i);
            obj->ops->get_props(obj, textprops);
            textprop = (TextProperty*)prop;
            ddisplay_transform_coords(ddisp,
                                      textprop->attr.position.x,
                                      textprop->attr.position.y,
                                      &x, &y);
            temp_line = text_line_new(textprop->text_data,
                                      textprop->attr.font,
                                      textprop->attr.height);
            /* This might need to account for zoom factor. */
            ascent = ddisplay_transform_length(ddisp,
                                               text_line_get_ascent(temp_line));
            text_line_destroy(temp_line);
            printf("Text prop string %s pos %d, %d ascent %f\n",
                   textprop->text_data, x, y, ascent);
            ascent_pixels = ddisplay_transform_length(ddisp, ascent);
            y -= ascent_pixels;
            dia_canvas_put(DIA_CANVAS(ddisp->canvas), view,
                           x-EDIT_BORDER_WIDTH, y-EDIT_BORDER_WIDTH);
            buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
            gtk_text_buffer_insert_at_cursor(buf, textprop->text_data, -1);
            fonttag =
                gtk_text_buffer_create_tag(buf,
                                           NULL,
                                           "font-desc",
                                           dia_font_get_description(textprop->attr.font),
                                           NULL);
            gtk_text_buffer_get_bounds(buf, &start, &end);
            gtk_text_buffer_apply_tag(buf, fonttag, &start, &end);

            printf("Above lines %d below %d\n",
                   gtk_text_view_get_pixels_above_lines(GTK_TEXT_VIEW(view)),
                   gtk_text_view_get_pixels_below_lines(GTK_TEXT_VIEW(view)));

            gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(view),
                                                 GTK_TEXT_WINDOW_LEFT,
                                                 EDIT_BORDER_WIDTH);
            gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(view),
                                                 GTK_TEXT_WINDOW_RIGHT,
                                                 EDIT_BORDER_WIDTH);
            gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(view),
                                                 GTK_TEXT_WINDOW_TOP,
                                                 EDIT_BORDER_WIDTH);
            gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(view),
                                                 GTK_TEXT_WINDOW_BOTTOM,
                                                 EDIT_BORDER_WIDTH);
            /* Using deprecated function because the fucking gobject documentation
             * fucking sucks. */
#ifdef NEW_TEXT_EDIT
            gtk_signal_connect(GTK_OBJECT(view), "focus-out-event",
                               modify_edit_end, obj);
#endif
            gtk_widget_grab_focus(view);
            gtk_widget_show(view);
        }
    }
}
