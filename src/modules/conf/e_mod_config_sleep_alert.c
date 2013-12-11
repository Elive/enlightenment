#include "e.h"
#include "e_mod_main.h"

typedef enum _Sleep_Modes
{
   E_SLEEP_MODE_COUNT = 1,
   E_SLEEP_MODE_CLOCK = 2,
} Sleep_Modes;

struct _E_Config_Dialog_Data
{
   int sleep_mode;
   Evas_Object *om;
   Ecore_Timer *message;
};

static void         *_create_data(E_Config_Dialog *cfd);
static void          _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int           _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int           _basic_close(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object  *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Eina_Bool     _timer_cb_label(void *data);

E_Config_Dialog *
e_int_config_sleep_alert(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;
   E_Border *bd;
   char buf[PATH_MAX];

   /* is this config dialog already visible ? */
   if (e_config_dialog_find("Sleep Alert", "advanced/sleep_alert")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->close_cfdata = _basic_close;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->override_auto_apply = EINA_TRUE;
   v->hide_apply = EINA_TRUE;
   v->close_label = eina_stringshare_add("Cancel");

   snprintf(buf, sizeof(buf), "%s/e-module-conf.edj", conf->module->dir);
   cfd = e_config_dialog_new(con, _("Sleep Message"), "Sleep Alert",
                             "advanced/sleep_alert", buf, 0, v, NULL);

   e_config_dialog_changed_set(cfd, 1);

   bd = e_border_find_by_client_window(cfd->dia->win->evas_win);
   e_border_stick(bd);
   e_border_center(bd);
   e_border_raise(bd);

   conf->cfd = cfd;
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;

   return cfdata;
}

static int
_basic_close(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->sleep_mode == E_SLEEP_MODE_COUNT)
     conf->countdown.active = 0;

   if (cfdata->sleep_mode == E_SLEEP_MODE_CLOCK)
     conf->clockmode.active = 0;

   conf->alert_message = EINA_FALSE;
   e_config_save_queue();

   return 1;
}


static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   return 1;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   ecore_timer_del(cfdata->message);
   cfdata->message = NULL;
   conf->cfd = NULL;
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *om;

   o = e_widget_table_add(evas, 0);
   om = e_widget_label_add(evas, NULL);
   cfdata->om = om;
   _timer_cb_label(cfdata);
   e_widget_table_object_append(o, om, 0, 0, 2, 1, 1, 1, 1, 0);

   cfdata->message = ecore_timer_add(0.5, _timer_cb_label, cfdata);
   return o;
}

static Eina_Bool
_timer_cb_label(void *data)
{
   E_Config_Dialog_Data *cfdata;
   char buf[256];
   const char *sleep_type;
   double countdown, clockmode, unix_time;
   int seconds_left = 0,  hours, minutes, seconds, remainder;

   cfdata = data;

   unix_time = ecore_time_unix_get();
   countdown = e_mod_mode_sleep_time_left_calc();

   clockmode = (conf->clockmode.time - unix_time);

   if (conf->sleep_type == 0)
     sleep_type = "Shutdown";
   else if (conf->sleep_type == 1)
     sleep_type = "Hibernate";
   else if (conf->sleep_type == 2)
     sleep_type = "Suspend";

   if (conf->countdown.active && conf->clockmode.active)
     {
        if (countdown < clockmode)
          {
             cfdata->sleep_mode = E_SLEEP_MODE_COUNT;
             seconds_left = (int) countdown;
          }

        if (clockmode < countdown)
          {
             cfdata->sleep_mode = E_SLEEP_MODE_CLOCK;
             seconds_left = (int) clockmode;
          }
     }
   else if (conf->countdown.active && !conf->clockmode.active)
     {
        cfdata->sleep_mode = E_SLEEP_MODE_COUNT;
        seconds_left = (int) countdown;
     }
   else if (!conf->countdown.active && conf->clockmode.active)
     {
        cfdata->sleep_mode = E_SLEEP_MODE_CLOCK;
        seconds_left = (int) clockmode;
     }

   hours = seconds_left / 3600;
   remainder = seconds_left % 3600;
   minutes = remainder / 60;
   seconds = remainder % 60;


   snprintf(buf, sizeof(buf), _("The computer will %s in %02d:%02d:%02d"), sleep_type,
            hours, minutes, seconds);

   if (!seconds_left)
     snprintf(buf, sizeof(buf), _("Sleep Disabled!"));

   e_widget_label_text_set(cfdata->om, buf);
   return ECORE_CALLBACK_RENEW;
}
