#ifndef TEMPLATE_PROC_H_INCLUDED
#define TEMPLATE_PROC_H_INCLUDED
#include "object.h"
#include "struct_class.h"
#include "plug-ins.h"
/* 把模版相关的函数放在这个目录下,是因为,只能对这种特定的结构体做成模版 */


enum
{
    ITEM_NAME,
    ITEM_BOOL,
    ITEM_VISIBLE,
    ITEM_COUNT
} ;


typedef struct _NewItem NewItem;
struct _NewItem
{
    const gchar    *label;
//    gboolean        fixed;
    gboolean        expand;
    GList *children;
    gchar *tooltips;
    gint pos; /* 在原结构内的位置 */
    gboolean ischecked; /* 是否钩选上了 */
    GQuark name_quark;
};
extern TemplateOps *templ_ops ;




void factory_template_save_dialog(GtkWidget *widget,gint       response_id,gpointer   user_data);

static gboolean factory_template_save(FactoryStructItemList *fssl);



static void factory_template_edit_callback(GtkAction *action);
void factory_template_actionid_verifyed(Diagram *diagram);

static void factory_template_create(DiaObject *dia);
static void factory_setback_values(GtkWidget *widget);
static void factory_setback_model_values(GSList *mlist);
static gboolean factory_find_item_pos(GSList *list,const gchar *str);
static NewItem *factory_template_find_old_item(GSList *slist,const gchar *str);

#endif // TEMPLATE_PROC_H_INCLUDED
