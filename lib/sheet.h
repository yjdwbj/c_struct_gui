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
  char *description;    /*��������*/
  char **pixmap; /* in xpm format */

  void *user_data;
  enum { USER_DATA_IS_INTDATA, USER_DATA_IS_OTHER } user_data_type;

  gboolean line_break;

  char *pixmap_file; /* fallback if pixmap is NULL */
  gboolean has_icon_on_sheet; /* �Ƿ���ʾ��*/
  FactoryTemplateItem ftitm; /* �����Ϊ��,�����һ��ģ�� */
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

struct _FactoryStructItem{  // ���ﶨ��һ���ṹ��ÿһ����Ҫ��ʾ�Ķ���, ����൱��һ��ģ��,ͨ��ȡ�ļ��ϵ���Ŀ�����ɶ��ٸ����ֱ���.
    gchar  *FType;  // ��ȫ������ Struct.u32
    gchar  *SType;  // ������ u32
    gchar  *Name;   // Ӣ����
    gchar  *Cname; // ����ע����
    gchar  *Value;  // Ĭ��ֵ
    gchar  *Min;
    gchar  *Max;    // ��ֵ
    gchar  *Comment;   // ����ע��
    ItemType Itype;   // ��������
    GList *datalist;  /* ����ó�Ա���˽ṹ�����������,��ָ��Ͳ�ΪNULL*/
    gpointer orgclass;
    gboolean isVisible; /* �ɼ��� */
    gboolean isSensitive; /* �ɱ༭�� */
};



/* 2014-3-26 lcy �����ù�ϣ��ı����������ٽ�Ͻṹ�崫�θ� callback ��������ӿؼ���ԭ�������������洢���ڸ�Ϊ��ϣ��*/
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
