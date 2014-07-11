#ifndef MUSICFILEMANAGER_H_INCLUDED
#define MUSICFILEMANAGER_H_INCLUDED
#include "object.h"
#include "struct_class.h"


enum
{
    COLUMN_SEQUENCE,
    COLUMN_PHY,
    COLUMN_FNAME,
    COLUMN_DNAME,
    NUM_OF_MCOLUMN
};



static void factory_mfile_manager_changed_dname(GtkCellRendererText *cell,
        const gchar         *path_string,
        const gchar         *new_text,
        gpointer             data);



static void factory_cleanall_mfile_modal(GtkWidget *widget,gpointer user_data);
static void factory_delete_mfile_item_modal(GtkWidget *widget,gpointer user_data);
static void factory_file_manager_refresh_all_iter(SaveMusicDialog *smd);


static void factory_choose_musicfile_callback(GtkWidget *dlg,gint       response,gpointer   user_data);
void factory_open_file_dialog(GtkWidget *widget,gpointer user_data);
void factory_delete_file_manager_item(GtkWidget *widget,gpointer user_data);
void factory_cleanall_file_manager_item(GtkWidget *widget,gpointer user_data);
void factory_insert_file_manager_item(GtkWidget *widget,gpointer user_data);

void factory_music_file_manager_new_item_changed(SaveMusicDialog *smd);
void factory_music_file_manager_remove_all(SaveMusicDialog *smd);
void factory_music_file_manager_apply(GtkWidget *widget,
                                        gint       response_id,
                                        SaveMusicDialog *smd);

void factory_mfile_manager_update_idmodal(SaveMusicDialog *smd);
void factory_mfile_manager_clean_modal(SaveMusicDialog *smd);
void factory_music_file_manager_select_callback(GtkWidget *clist,
        gint row,gint column,
        GdkEventButton *event,
        gint  *ret);

MusicFileManagerOpts  mfmo_opts =
{
    (OpenDialog) factory_file_manager_dialog,
    (ApplyDialog) 0,
    (Item_Added) factory_mfile_manager_update_idmodal,
    (Clear_All) factory_mfile_manager_clean_modal
};




#endif // MUSICFILEMANAGER_H_INCLUDED
