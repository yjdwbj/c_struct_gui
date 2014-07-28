#ifndef IDLIST_H_INCLUDED
#define IDLIST_H_INCLUDED
#include "struct.h"
#include "object.h"
#include "struct_class.h"



//typedef struct _IDListStore IDListStore;
//struct _IDListStore
//{
//    gint sequence;
//    gint id_addr;
//    gchar *id_text;
//};




void factory_save_idlist_dialog(GtkWidget *widget,gint       response_id,gpointer user_data);


#endif // IDLIST_H_INCLUDED
