#ifndef TEMPLATE_PROC_H_INCLUDED
#define TEMPLATE_PROC_H_INCLUDED
#include "object.h"
#include "struct_class.h"
/* ��ģ����صĺ����������Ŀ¼��,����Ϊ,ֻ�ܶ������ض��Ľṹ������ģ�� */

void factory_template_save_dialog(GtkWidget *widget,gint       response_id,gpointer   user_data);

void factory_template_save(FactoryStructItemList *fssl);

void factory_template_load();
static void factory_setback_values(GtkWidget *widget);
static void factory_setback_model_values(GSList *mlist);
static gboolean factory_find_item_pos(GList *list,const gchar *str);
#endif // TEMPLATE_PROC_H_INCLUDED
