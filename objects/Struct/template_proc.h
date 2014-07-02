#ifndef TEMPLATE_PROC_H_INCLUDED
#define TEMPLATE_PROC_H_INCLUDED
#include "object.h"
#include "struct_class.h"
#include "plug-ins.h"
/* ��ģ����صĺ����������Ŀ¼��,����Ϊ,ֻ�ܶ������ض��Ľṹ������ģ�� */


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
    gint pos; /* ��ԭ�ṹ�ڵ�λ�� */
    gboolean ischecked; /* �Ƿ�ѡ���� */
    GQuark name_quark;
};
extern TemplateOps *templ_ops ;




void factory_template_save_dialog(GtkWidget *widget,gint       response_id,gpointer   user_data);

static void factory_template_save(FactoryStructItemList *fssl);

void factory_template_load();

static void factory_template_edit_callback(GtkAction *action);



static void factory_setback_values(GtkWidget *widget);
static void factory_setback_model_values(GSList *mlist);
static gboolean factory_find_item_pos(GSList *list,const gchar *str);
static NewItem *factory_template_find_old_item(GSList *slist,const gchar *str);

#endif // TEMPLATE_PROC_H_INCLUDED
