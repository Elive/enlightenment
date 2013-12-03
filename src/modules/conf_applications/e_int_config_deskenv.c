#include "e.h"

/* PROTOTYPES - same all the time */

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _E_Config_Dialog_Data
{
   int load_xrdb;
   int load_xrdb_always;
   int load_xmodmap;
   int load_xmodmap_always;
   int load_gnome;
   int load_kde;
   int exe_always_single_instance;
};

/* a nice easy setup function that does the dirty work */
E_Config_Dialog *
e_int_config_deskenv(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "windows/desktop_environments"))
     return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   /* methods */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;

   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con, _("Desktop Environments"), "E",
                             "windows/desktop_environments",
                             "preferences-desktop-environments", 0, v, NULL);
   return cfd;
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->load_xrdb = e_config->deskenv.load_xrdb;
   cfdata->load_xrdb_always = e_config->deskenv.load_xrdb_always;
   cfdata->load_xmodmap = e_config->deskenv.load_xmodmap;
   cfdata->load_xmodmap_always = e_config->deskenv.load_xmodmap_always;
   cfdata->load_gnome = e_config->deskenv.load_gnome;
   cfdata->load_kde = e_config->deskenv.load_kde;
   cfdata->exe_always_single_instance = e_config->exe_always_single_instance;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

/**--APPLY--**/
static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->deskenv.load_xrdb = cfdata->load_xrdb;
   e_config->deskenv.load_xrdb_always = cfdata->load_xrdb_always;
   e_config->deskenv.load_xmodmap = cfdata->load_xmodmap;
   e_config->deskenv.load_xmodmap_always = cfdata->load_xmodmap_always;
   e_config->deskenv.load_gnome = cfdata->load_gnome;
   e_config->deskenv.load_kde = cfdata->load_kde;
   e_config->exe_always_single_instance = cfdata->exe_always_single_instance;
   e_config_save_queue();
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *fr, *ob;

   o = e_widget_list_add(evas, 0, 0);
   
   fr = e_widget_framelist_add(evas, _("Execution"), 0);
   ob = e_widget_check_add(evas, _("Only launch single instances"),
                           &(cfdata->exe_always_single_instance));
   e_widget_framelist_object_append(fr, ob);
   e_widget_list_object_append(o, fr, 1, 0, 0.0);
   
   fr = e_widget_framelist_add(evas, _("X11 Basics"), 0);
   ob = e_widget_check_add(evas, _("Load X Resources"),
                           &(cfdata->load_xrdb));
   e_widget_framelist_object_append(fr, ob);
   ob = e_widget_check_add(evas, _("Load X Resources everytime Enlightenment Restart"),
                           &(cfdata->load_xrdb_always));
   e_widget_framelist_object_append(fr, ob);
   ob = e_widget_check_add(evas, _("Load X Modifier Map"),
                           &(cfdata->load_xmodmap));
   e_widget_framelist_object_append(fr, ob);
   ob = e_widget_check_add(evas, _("Load X Modifier Map everytime Enlightenment Restart"),
                           &(cfdata->load_xmodmap_always));
   e_widget_framelist_object_append(fr, ob);

   e_widget_list_object_append(o, fr, 1, 0, 0.0);
   
   fr = e_widget_framelist_add(evas, _("Major Desktops"), 0);
   ob = e_widget_check_add(evas, _("Start GNOME services on login"),
                           &(cfdata->load_gnome));
   e_widget_framelist_object_append(fr, ob);
   ob = e_widget_check_add(evas, _("Start KDE services on login"),
                           &(cfdata->load_kde));
   e_widget_framelist_object_append(fr, ob);
   e_widget_list_object_append(o, fr, 1, 0, 0.0);
   
   return o;
}

