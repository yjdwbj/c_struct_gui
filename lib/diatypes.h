/* Dia -- an diagram creation/manipulation program -*- c -*-
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

/** @file diatypes.h -- All externally visible structures should be defined here */

#ifndef TYPES_H
#define TYPES_H
#include "diavar.h"
/* THIS HEADER MUST NOT INCLUDE ANY OTHER HEADER! */
/*#include "units.h" */

/* In diagramdata.h: */
typedef struct _DiagramData DiagramData;
typedef struct _Layer Layer;
typedef struct _NewDiagramData NewDiagramData;

/* In arrows.h: */
typedef struct _Arrow Arrow;

/* In bezier_conn.h: */
typedef struct _BezierConn BezierConn;

/* In beziershape.h: */
typedef struct _BezierShape BezierShape;

/* In boundingbox.h: */
typedef struct _PolyBBExtras PolyBBExtras;
typedef struct _LineBBExtras LineBBExtras;
typedef struct _ElementBBExtras ElementBBExtras;

/* In color.h: */
typedef struct _Color Color;

/* In connection.h: */
typedef struct _Connection Connection;

/* In connectionpoint.h: */
typedef struct _ConnectionPoint ConnectionPoint;

/* In create.h: */
typedef struct _MultipointCreateData MultipointCreateData;
typedef struct _BezierCreateData BezierCreateData;

/* In dia_image.h: */
typedef struct _DiaImage DiaImage;

/* In diagdkrenderer.h: */
typedef struct _DiaGdkRenderer DiaGdkRenderer;
typedef struct _DiaGdkRendererClass DiaGdkRendererClass;
typedef struct _DDisp DDisp;

/* In dialibartrenderer.h: */
typedef struct _DiaLibartRenderer DiaLibartRenderer;
typedef struct _DiaLibartRendererClass DiaLibartRendererClass;

/* In diamenu.h: */
typedef struct _DiaMenuItem DiaMenuItem;
typedef struct _DiaMenu DiaMenu;

/* In diarenderer.h: */
typedef struct _BezierApprox BezierApprox;
typedef struct _DiaRenderer DiaRenderer;
typedef struct _DiaRendererClass DiaRendererClass;
typedef struct _DiaInteractiveRendererInterface DiaInteractiveRendererInterface;

/* In diacellrendererproperty.h: */
typedef struct _DiaCellRendererProperty DiaCellRendererProperty;

/* In diasvgrenderer.h: */
typedef struct _DiaSvgRenderer DiaSvgRenderer;
typedef struct _DiaSvgRendererClass DiaSvgRendererClass;

/* In diatransform.h: */
typedef struct _DiaTransform DiaTransform;

/* In element.h: */
typedef struct _Element Element;

/* In filter.h: */
typedef struct _DiaExportFilter DiaExportFilter;
typedef struct _DiaImportFilter DiaImportFilter;
typedef struct _DiaCallbackFilter DiaCallbackFilter;

/* In focus.h: */
typedef struct _Focus Focus;

/* In font.h: */
typedef struct _DiaFont DiaFont;
typedef struct _DiaFontClass DiaFontClass;

/* In geometry.h: */
typedef struct _Point Point;
typedef struct _Rectangle Rectangle;
typedef struct _IntRectangle IntRectangle;
typedef struct _BezPoint BezPoint;

/* In group.h: */
typedef struct _Group Group;

/* In handle.h: */
typedef struct _Handle Handle;

/* In neworth_conn.h: */
typedef struct _NewOrthConn NewOrthConn;

/* In objchange.h: */
typedef struct _ObjectState ObjectState;
typedef struct _ObjectChange ObjectChange;

/* In object.h: */
typedef struct _DiaObject DiaObject;
typedef struct _ObjectOps ObjectOps;
typedef struct _DiaObjectType DiaObjectType;
typedef struct _FactorySystemType FactorySystemType;

typedef struct _ObjectTypeOps ObjectTypeOps;

/* In orth_conn.h: */
typedef struct _OrthConn OrthConn;

/* In paper.h: */
typedef struct _PaperInfo PaperInfo;

/* In plug-ins.h: */
typedef struct _PluginInfo PluginInfo;

/* In poly_conn.h: */
typedef struct _PolyConn PolyConn;

/* In polyshape.h: */
typedef struct _PolyShape PolyShape;

/* In properties.h: */
typedef struct _PropDescription PropDescription;
typedef struct _Property Property;
typedef struct _PropEventData PropEventData;
typedef struct _PropDialog PropDialog;
typedef struct _PropEventHandlerChain PropEventHandlerChain;
typedef struct _PropWidgetAssoc PropWidgetAssoc;
typedef struct _PropertyOps PropertyOps;
typedef struct _PropNumData PropNumData;
typedef struct _PropEnumData PropEnumData;
typedef struct _PropDescCommonArrayExtra PropDescCommonArrayExtra;
typedef struct _PropDescDArrayExtra PropDescDArrayExtra;
typedef struct _PropDescSArrayExtra PropDescSArrayExtra;
typedef struct _PropOffset PropOffset;

/* In ps-utf8.h: */
typedef struct _PSFontDescriptor PSFontDescriptor;
typedef struct _PSEncodingPage PSEncodingPage;
typedef struct _PSUnicoder PSUnicoder;
typedef struct _PSUnicoderCallbacks PSUnicoderCallbacks;

/* In sheet.h: */
typedef struct _Sheet Sheet;
typedef struct _SheetObject SheetObject;

/* In text.h: */
typedef struct _Text Text;

/* In textline.h: */
typedef struct _TextLine TextLine;

/* In textattr.h: */
typedef struct _TextAttributes TextAttributes;

/* In widgets.h: */
typedef struct _DiaSizeSelector       DiaSizeSelector;
typedef struct _DiaSizeSelectorClass  DiaSizeSelectorClass;
typedef struct _DiaFontSelector       DiaFontSelector;
typedef struct _DiaFontSelectorClass  DiaFontSelectorClass;
typedef struct _DiaAlignmentSelector       DiaAlignmentSelector;
typedef struct _DiaAlignmentSelectorClass  DiaAlignmentSelectorClass;
typedef struct _DiaLineStyleSelector       DiaLineStyleSelector;
typedef struct _DiaLineStyleSelectorClass  DiaLineStyleSelectorClass;
typedef struct _DiaColorSelector       DiaColorSelector;
typedef struct _DiaColorSelectorClass  DiaColorSelectorClass;
typedef struct _DiaArrowSelector       DiaArrowSelector;
typedef struct _DiaArrowSelectorClass  DiaArrowSelectorClass;
typedef struct _DiaFileSelector       DiaFileSelector;
typedef struct _DiaFileSelectorClass  DiaFileSelectorClass;

/* Template */
typedef struct _TemplateOps TemplateOps;

typedef struct _SaveIdDialog SaveIdDialog; /*ID列表界面*/
typedef struct _SaveMusicDialog SaveMusicDialog; /* 主界面　*/

typedef struct _FactoryStructItem FactoryStructItem;
typedef struct _FactoryStructEnum FactoryStructEnum;
//typedef struct _FactoryStructEnumList FactoryStructEnumList;

typedef struct _FactoryItemInOriginalMap  FactoryItemInOriginalMap;
struct _FactoryItemInOriginalMap
{
    gchar *struct_name; /* 结构体名 */
    gchar *act_name; /*行为名字*/
    GSList *itemslist ; /*选择结构体内的几个成员组成新的链表*/
};


typedef struct _FactoryStructItemList  FactoryStructItemList;
struct _FactoryStructItemList{
    gchar *sname; /* 结构体名 */
    gchar *vname; /* 可见的名称 */
    GList *list;
    int number;
    gchar *sfile; /* 指定保存文件名 */
    gboolean isvisible; /* 是否显示到时面板上 */
//    gboolean isTemplate; /* 跟结构体放在一起,用这个标识 */
//    GList *templlist; /*存储FactoryItemInOriginalMap*/
};




typedef GList* (*FactoryGetDownloadNameList)(const gchar* path);
typedef void (*TemplateEdit)(gpointer action);
typedef gboolean (*TemplateSaveToFile)(FactoryStructItemList *);

struct _TemplateOps
{
    TemplateEdit templ_edit;
    TemplateSaveToFile templ_save;
    void      (*(unused[4]))(DiaObject *obj,...);
};
//typedef int (*DiagramDataRawSave)(DiagramData *data, const char *filename);
typedef struct _FactoryTemplateItem FactoryTemplateItem;
struct _FactoryTemplateItem
{
    FactoryStructItemList fsil;
    gchar *entrypoint; /* 入口行为的名字 */
//    GList *templlist; /*存储FactoryItemInOriginalMap*/
    GSList *modellist; /* model top list */
    TemplateOps *templ_ops; /* 关于模版相关的函数 */
//    TemplateSaveToFile templ_save;
};

typedef struct _FactorySystemInfo FactorySystemInfo;
struct _FactorySystemInfo
{
    FactorySystemType *fstype;
    gpointer *system_info; /*系统信息*/
//    GList *IO_List; /* IO 列表*/
//    GList *IO_selected; /*已经选择的列表 */
//    int io_mindex; /*最大的ＩＯ枚举值*/
//    gchar *null_io; /* 空ＩＯ */
};
typedef struct _FactoryColors FactoryColors;


typedef struct _FactoryStructItemAll  FactoryStructItemAll; // 2014-3-20 lcy 存放整个文件的结果,第一个为枚举,第二个存结构体
struct _FactoryStructItemAll{
    GHashTable *enumTable;
    GHashTable *structTable; // 2014-3-25 lcy 这里存放 FactoryStructItemList 的哈希表。
    GHashTable *unionTable;
    GList* structList; // 2014-3-25 lcy 这里存放 FactoryStructItemList 的链表。
    gchar *project_number; /*工程号*/
    gchar *major_version;
    gchar *minor_version;
    gchar *system_files;
    Layer *curLayer;
    FactorySystemInfo *sys_info; /*系统信息*/
    FactoryGetDownloadNameList fgdn_func;
    FactoryColors *color;
    TemplateEdit templ_edit;
    TemplateSaveToFile templ_save;
//    DiagramDataRawSave diagram_data_raw_save;

};




#endif
