#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   int cfg_xkb;
   int cfg_deskenv;
};

E_Config_Dialog *
e_int_config_erestart(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "settings/erestartcfg")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.check_changed = _basic_check_changed;
   v->override_auto_apply = 1;

   cfd = e_config_dialog_new(con, _("E Restart Config"),
                             "E", "settings/erestartcfg",
                             "preferences-system", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->cfg_xkb = e_config->reload_xkb_on_erestart;
   cfdata->cfg_deskenv = e_config->reload_deskenv_on_erestart;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->reload_xkb_on_erestart = cfdata->cfg_xkb;
   e_config->reload_deskenv_on_erestart = cfdata->cfg_deskenv;

   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return ((e_config->reload_xkb_on_erestart != cfdata->cfg_xkb) ||
           (e_config->reload_deskenv_on_erestart != cfdata->cfg_deskenv));
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General Settings"), 0);

   ob = e_widget_check_add(evas, _("Update .Xmodmap everytime E17 restarts"), &(cfdata->cfg_xkb));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Update .Xdefaults & .Xresources everytime E17 restarts"), &(cfdata->cfg_deskenv));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 0, 0.5);

   return o;
}

