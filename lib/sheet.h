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
DIAVAR FactoryStructItemAll *factoryContainer;
DIAVAR TemplateOps *templ_ops;
struct _SheetObject {
  char *object_type;
  char *description;    /*中文名字*/
  char **pixmap; /* in xpm format */

  void *user_data;
  enum { USER_DATA_IS_INTDATA, USER_DATA_IS_OTHER } user_data_type;

  gboolean line_break;

  char *pixmap_file; /* fallback if pixmap is NULL */
  gboolean has_icon_on_sheet; /* 是否显示　*/
  FactoryTemplateItem ftitm; /* 如果不为空,这就是一个模版 */
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


struct _FactoryStructEnum
{
    gchar *key;
    gchar *value;
};

typedef enum{
    BT, /* base type */
    ET, /* enum type */
    ST, /* struct type */
    UT, /* union type */
    NT  /* defult type unkown*/
}ItemType;

struct _FactoryStructItem{  // 这里定义一个结构的每一项需要显示的东西, 这就相当于一个模子,通读取文件上的条目来生成多少个这种表项.
    gchar  *FType;  // 完全类型名 Struct.u32
    gchar  *SType;  // 短类型 u32
    gchar  *Name;   // 英文名
    gchar  *Cname; // 中文注释名
    gchar  *Value;  // 默认值
    gchar  *Min;
    gchar  *Max;    // 极值
    gchar  *Comment;   // 浮动注释
    ItemType Itype;   // 基本类形
    GList *datalist;  /* 如果该成员是人结构体或者是联合,该指针就不为NULL*/
    gpointer orgclass;
    gboolean isVisible; /* 可见项 */
    gboolean isSensitive; /* 可编辑项 */
};



/* 2014-3-26 lcy 这里用哈希表的遍历函数，再结合结构体传参给 callback 函数来添加控件，原来是用链表来存储现在改为哈希表*/
//static void factory_create_obj_from_hashtable(gpointer key,
//                gpointer value,
//                gpointer user_data);
//static void factory_add_sheet_obj(Sheet *sheet,SheetObject *sheet_obj,DiaObjectType *otype,gchar *tmp,gchar *sheet_name);
typedef void (*Factor_callback_fun)(Sheet *sheet,SheetObject *sheet_obj,DiaObjectType *otype,gchar *tmp,gchar *sheet_name);
typedef struct _FactoryCreateSheets  FactoryCreateSheets;
struct _FactoryCreateSheets{
    Factor_callback_fun callback_func;
    Sheet* sheet;
    SheetObject *sheet_obj;
    DiaObjectType *otype;
    gchar *tmp;
};

typedef enum {
    TYPEDEF,
    STRUCT,
    WRONG
}DEFTYPE;


Sheet *new_sheet(char *name, char *description, char *filename,
                 SheetScope scope, Sheet *shadowing);
void sheet_prepend_sheet_obj(Sheet *sheet, SheetObject *type);
DIAVAR void sheet_append_sheet_obj(Sheet *sheet, SheetObject *type);
DIAVAR void register_sheet(Sheet *sheet);
DIAVAR GSList *get_sheets_list(void);
DIAVAR void factoryReadDataFromFile(const gchar* name);
static DEFTYPE factory_check_define(gchar *data);
DIAVAR void factory_read_native_c_file(const gchar* name);


DIAVAR void load_all_sheets(void);
DIAVAR void dia_sort_sheets(void);

#endif /* SHEET_H */
