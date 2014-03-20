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
#ifndef SHEET_H
#define SHEET_H

#include <glib.h>

#include "diatypes.h"

struct _SheetObject {
  char *object_type;
  char *description;
  char **pixmap; /* in xpm format */

  void *user_data;
  enum { USER_DATA_IS_INTDATA, USER_DATA_IS_OTHER } user_data_type;

  gboolean line_break;

  char *pixmap_file; /* fallback if pixmap is NULL */
  gboolean has_icon_on_sheet;
};

typedef enum
{
  SHEET_SCOPE_SYSTEM,
  SHEET_SCOPE_USER
}
SheetScope;

struct _Sheet {
  char *name;
  char *description;
  char *filename;

  SheetScope scope;
  Sheet *shadowing;           /* If (scope == USER), the system sheet that this
                                 user sheet is shadowing.
                                 If (scope == SYSTEM), there has been a name
                                 collision between a system sheet and a user
                                 sheet and the system sheet appears to be more
                                 recent than the user sheet.
                                 Can be NULL. */

  GSList *objects; /* list of SheetObject */
};


struct _FactorStructEnum
{
    gchar *key;
    gchar *value;
};

struct _FactoryStructItem{  // 这里定义一个结构的每一项需要显示的东西, 这就相当于一个模子,通读取文件上的条目来生成多少个这种表项.
    gchar  *itemType;  // 类型
    gchar  *itemName;   // 英文名
    gchar  *itemCname; // 中文注释名
    gchar  *itemValue;  // 默认值
    gchar  *itemMin;
    gchar  *itemMax;
    gchar *itemComment;   // 浮动注释
};
struct _FactorStructEnumList{
    gchar *name;
    GList *list;
};

struct _FactorStructItemList{
    gchar *name;
    GList *list;
    int number;
};

struct _FactorStructItemAll{
    GList *enumList;
    GList *structList;
};

Sheet *new_sheet(char *name, char *description, char *filename,
                 SheetScope scope, Sheet *shadowing);
void sheet_prepend_sheet_obj(Sheet *sheet, SheetObject *type);
DIAVAR void sheet_append_sheet_obj(Sheet *sheet, SheetObject *type);
DIAVAR void register_sheet(Sheet *sheet);
DIAVAR GSList *get_sheets_list(void);

DIAVAR void load_all_sheets(void);
DIAVAR void dia_sort_sheets(void);

#endif /* SHEET_H */
