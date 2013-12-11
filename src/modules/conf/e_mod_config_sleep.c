#include "e.h"
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   int sleep_type;
   Evas_Object *or[3];
   Ecore_Timer *message;
   E_Dialog *dia;

   struct
     {
        Evas_Object *oh;
        Evas_Object *om;
        Evas_Object *od;

        double hour;
        double min;
        int active;
        int always;
     }countdown, clockmode;
};

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
e_int_config_sleep_module(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;
   char buf[PATH_MAX];

   /* is this config dialog already visible ? */
   if (e_config_dialog_find("Sleep Timeout", "advanced/sleep")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_check_changed;

   snprintf(buf, sizeof(buf), "%s/e-module-conf.edj", conf->module->dir);
   cfd = e_config_dialog_new(con, _("Sleep Timeout Configuration"), "Sleep",
                             "advanced/sleep", buf, 0, v, NULL);

   conf->cfd = cfd;
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;

   cfdata->countdown.hour = conf->countdown.hour;
   cfdata->countdown.min = conf->countdown.min;
   cfdata->countdown.active = conf->countdown.active;

   cfdata->clockmode.hour = conf->clockmode.hour;
   cfdata->clockmode.min = conf->clockmode.min;
   cfdata->clockmode.active = conf->clockmode.active;
   cfdata->clockmode.always = conf->clockmode.always;

   cfdata->sleep_type = conf->sleep_type;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   conf->cfd = NULL;
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ol, *ow, *oo, *os, *oc;
   E_Radio_Group *rg;

   o = e_widget_table_add(evas, 0);
   of = e_widget_framelist_add(evas, _("Countdown"), 0);

   oc = e_widget_check_add(evas, _("Enable"), &(cfdata->countdown.active));
   e_widget_framelist_object_append(of, oc);

   ol = e_widget_label_add(evas, _("Time to count before closing computer"));
   e_widget_framelist_object_append(of, ol);

   os = e_widget_slider_add(evas, 1, 0, _("%1.0f hour"), 0.0, 23.0, 1.00, 0, &(cfdata->countdown.hour), NULL, 100);
   e_widget_framelist_object_append(of, os);
   if (!cfdata->countdown.active) e_widget_disabled_set(os, 1);
   cfdata->countdown.oh = os;

   os = e_widget_slider_add(evas, 1, 0, _("%1.0f minute"), 0.0, 59.0, 1.00, 0, &(cfdata->countdown.min), NULL, 100);
   e_widget_framelist_object_append(of, os);
   if (!cfdata->countdown.active) e_widget_disabled_set(os, 1);
   cfdata->countdown.om = os;

   e_widget_table_object_append(o, of, 0, 0, 1, 1, 1, 1, 1, 0);

   of = e_widget_framelist_add(evas, _("Clock Mode"), 0);

   oc = e_widget_check_add(evas, _("Enable"), &(cfdata->clockmode.active));
   e_widget_framelist_object_append(of, oc);

   ol = e_widget_label_add(evas, _("Select which hour of the day (24h) to close Computer"));
   e_widget_framelist_object_append(of, ol);

   os = e_widget_slider_add(evas, 1, 0, _("%1.0f hour"), 0.0, 23.0, 1.00, 0, &(cfdata->clockmode.hour), NULL, 100);
   e_widget_framelist_object_append(of, os);
   if (!cfdata->clockmode.active) e_widget_disabled_set(os, 1);
   cfdata->clockmode.oh = os;

   os = e_widget_slider_add(evas, 1, 0, _("%1.0f minute"), 0.0, 59.0, 1.00, 0, &(cfdata->clockmode.min), NULL, 100);
   e_widget_framelist_object_append(of, os);
   if (!cfdata->clockmode.active) e_widget_disabled_set(os, 1);
   cfdata->clockmode.om = os;

   oc = e_widget_check_add(evas, _("Close computer with this setting each day"), &(cfdata->clockmode.always));
   e_widget_framelist_object_append(of, oc);
   if (!cfdata->clockmode.active) e_widget_disabled_set(oc, 1);
   cfdata->clockmode.od = oc;

   e_widget_table_object_append(o, of, 1, 0, 1, 1, 1, 1, 1, 0);

   of = e_widget_framelist_add(evas, _("Halt Type"), 0);

   oo = e_widget_table_add(evas, 0);
   rg = e_widget_radio_group_new(&(cfdata->sleep_type));

   ow = e_widget_radio_add(evas, _("Shutdown"), 0, rg);
   e_widget_table_object_append(oo, ow, 0, 0, 1, 1, 1, 1, 1, 0);
   e_widget_disabled_set(ow, 1);
   if (cfdata->clockmode.active || cfdata->countdown.active) e_widget_disabled_set(ow, 0);
   cfdata->or[0] = ow;

   ow = e_widget_radio_add(evas, _("Hibernate"), 1, rg);
   e_widget_table_object_append(oo, ow, 1, 0, 1, 1, 1, 1, 1, 0);
   e_widget_disabled_set(ow, 1);
   if (cfdata->clockmode.active || cfdata->countdown.active) e_widget_disabled_set(ow, 0);
   cfdata->or[1] = ow;

   ow = e_widget_radio_add(evas, _("Suspend"), 2, rg);
   e_widget_table_object_append(oo, ow, 2, 0, 1, 1, 1, 1, 1, 0);
   e_widget_disabled_set(ow, 1);
   if (cfdata->clockmode.active || cfdata->countdown.active) e_widget_disabled_set(ow, 0);
   cfdata->or[2] = ow;


   e_widget_framelist_object_append(of, oo);
   e_widget_table_object_append(o, of, 0, 1, 2, 1, 1, 1, 1, 0);
   return o;
}


static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   conf->sleep_type = cfdata->sleep_type;

   if ((conf->countdown.hour != cfdata->countdown.hour) ||
       (conf->countdown.min  != cfdata->countdown.min) ||
       (conf->countdown.active != cfdata->countdown.active))
     {
        conf->countdown.hour = cfdata->countdown.hour;
        conf->countdown.min = cfdata->countdown.min;
        conf->countdown.time = ecore_time_unix_get();
        conf->countdown.active = cfdata->countdown.active;

        e_mod_mode_sleep_countdown_timer_reset();
     }

   if ((conf->clockmode.hour != cfdata->clockmode.hour) ||
       (conf->clockmode.min  != cfdata->clockmode.min) ||
       (conf->clockmode.active != cfdata->clockmode.active) ||
       (conf->clockmode.always != cfdata->clockmode.always))
     {
        conf->clockmode.hour = cfdata->clockmode.hour;
        conf->clockmode.min = cfdata->clockmode.min;
        conf->clockmode.active = cfdata->clockmode.active;
        conf->clockmode.always = cfdata->clockmode.always;
        conf->clockmode.time = e_mod_mode_sleep_clockmode_epoch_time_get();

        e_mod_mode_sleep_clockmode_timer_reset();
     }

   e_config_mode_changed();
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->countdown.active)
     {
        e_widget_disabled_set(cfdata->countdown.oh, 0);
        e_widget_disabled_set(cfdata->countdown.om, 0);
     }
   else
     {
        e_widget_disabled_set(cfdata->countdown.oh, 1);
        e_widget_disabled_set(cfdata->countdown.om, 1);
     }

   if (cfdata->clockmode.active)
     {
        e_widget_disabled_set(cfdata->clockmode.oh, 0);
        e_widget_disabled_set(cfdata->clockmode.om, 0);
        e_widget_disabled_set(cfdata->clockmode.od, 0);
     }
   else
     {
        e_widget_disabled_set(cfdata->clockmode.oh, 1);
        e_widget_disabled_set(cfdata->clockmode.om, 1);
        e_widget_disabled_set(cfdata->clockmode.od, 1);
     }

   if (cfdata->clockmode.active || cfdata->countdown.active)
     {
        e_widget_disabled_set(cfdata->or[0], 0);
        e_widget_disabled_set(cfdata->or[1], 0);
        e_widget_disabled_set(cfdata->or[2], 0);
     }
   else
     {
        e_widget_disabled_set(cfdata->or[0], 1);
        e_widget_disabled_set(cfdata->or[1], 1);
        e_widget_disabled_set(cfdata->or[2], 1);
     }

   return 1;
}

