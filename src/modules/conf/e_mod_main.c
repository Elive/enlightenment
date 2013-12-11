#include "e.h"
#include "e_mod_main.h"

typedef struct _Instance Instance;
struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_toggle;
};

/* actual module specifics */

static void             _e_mod_action_conf_cb(E_Object *obj, const char *params);
static void             _e_mod_conf_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void             _e_mod_menu_add(void *data, E_Menu *m);
static void             _e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void             _config_pre_activate_cb(void *data, E_Menu *m);

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char      *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object     *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char      *_gc_id_new(const E_Gadcon_Client_Class *client_class);
static void             _cb_action_conf(void *data, Evas_Object *obj, const char *emission, const char *source);

static void             _conf_new(void);
static void             _conf_free(void);
static Eina_Bool        _e_mod_mode_sleep_countdown_timer(void *data);
static Eina_Bool        _e_mod_mode_sleep_clockmode_timer(void *data);

static E_Module *conf_module = NULL;
static E_Action *act = NULL;
static E_Int_Menu_Augmentation *maug = NULL;
static E_Config_DD *conf_edd = NULL;
Config *conf = NULL;

static Eina_List *instances = NULL;

/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION, "configuration",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
      e_gadcon_site_is_not_toolbar
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Instance *inst;

   inst = E_NEW(Instance, 1);
   inst->o_toggle = edje_object_add(gc->evas);
   e_theme_edje_object_set(inst->o_toggle,
                           "base/theme/modules/conf",
                           "e/modules/conf/main");

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_toggle);
   inst->gcc->data = inst;

   edje_object_signal_callback_add(inst->o_toggle, "e,action,conf", "",
                                   _cb_action_conf, inst);

   instances = eina_list_append(instances, inst);
   e_gadcon_client_util_menu_attach(inst->gcc);

   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);
   if (inst->o_toggle) evas_object_del(inst->o_toggle);
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   Evas_Coord mw, mh;

   edje_object_size_min_get(gcc->o_base, &mw, &mh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(gcc->o_base, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Settings");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[PATH_MAX];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-conf.edj",
            e_module_dir_get(conf_module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _gadcon_class.name;
}

/*
   static void
   _cb_button_click(void *data __UNUSED__, void *data2 __UNUSED__)
   {
   E_Action *a;

   a = e_action_find("configuration");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
   }
 */

static void
_cb_action_conf(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Instance *inst;
   E_Action *a;

   if (!(inst = data)) return;
   a = e_action_find("configuration");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

static void
_e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   Eina_List *l;
   E_Configure_Cat *ecat;

   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     {
        if ((ecat->pri >= 0) && (ecat->items))
          {
             E_Configure_It *eci;
             Eina_List *ll;

             EINA_LIST_FOREACH(ecat->items, ll, eci)
               {
                  char buf[1024];

                  if ((eci->pri >= 0) && (eci == data))
                    {
                       snprintf(buf, sizeof(buf), "%s/%s", ecat->cat, eci->item);
                       e_configure_registry_call(buf, m->zone->container, NULL);
                    }
               }
          }
     }
}

static void
_config_pre_activate_cb(void *data, E_Menu *m)
{
   E_Configure_Cat *ecat = data;
   E_Configure_It *eci;
   Eina_List *l;
   E_Menu_Item *mi;

   e_menu_pre_activate_callback_set(m, NULL, NULL);

   EINA_LIST_FOREACH(ecat->items, l, eci)
     {
        if (eci->pri >= 0)
          {
             mi = e_menu_item_new(m);
             e_menu_item_label_set(mi, eci->label);
             if (eci->icon)
               {
                  if (eci->icon_file)
                    e_menu_item_icon_edje_set(mi, eci->icon_file, eci->icon);
                  else
                    e_util_menu_item_theme_icon_set(mi, eci->icon);
               }
             e_menu_item_callback_set(mi, _e_mod_run_cb, eci);
          }
     }
}

static void
_config_item_activate_cb(void *data, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   E_Configure_Cat *ecat = data;
   e_configure_show(m->zone->container, ecat->cat);
}

static void
_config_all_pre_activate_cb(void *data __UNUSED__, E_Menu *m)
{
   const Eina_List *l;
   E_Configure_Cat *ecat;

   e_menu_pre_activate_callback_set(m, NULL, NULL);

   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     {
        E_Menu_Item *mi;
        E_Menu *sub;

        if ((ecat->pri < 0) || (!ecat->items)) continue;

        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, ecat->label);
        if (ecat->icon)
          {
             if (ecat->icon_file)
               e_menu_item_icon_edje_set(mi, ecat->icon_file, ecat->icon);
             else
               e_util_menu_item_theme_icon_set(mi, ecat->icon);
          }
        e_menu_item_callback_set(mi, _config_item_activate_cb, ecat);
        sub = e_menu_new();
        e_menu_item_submenu_set(mi, sub);
        e_object_unref(E_OBJECT(sub));
        e_menu_pre_activate_callback_set(sub, _config_pre_activate_cb, ecat);
     }
}

/* menu item add hook */
void
e_mod_config_menu_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;
   E_Menu *sub;

   e_menu_pre_activate_callback_set(m, NULL, NULL);

   sub = e_menu_new();
   e_menu_pre_activate_callback_set(sub, _config_all_pre_activate_cb, NULL);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("All"));
   e_menu_item_submenu_set(mi, sub);
   e_object_unref(E_OBJECT(sub));
}

/* module setup */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Conf" };

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[PATH_MAX];

   conf_module = m;

   /* add module supplied action */
   act = e_action_add("configuration");
   if (act)
     {
        act->func.go = _e_mod_action_conf_cb;
        e_action_predef_name_set(N_("Launch"), N_("Settings Panel"),
                                 "configuration", NULL, NULL, 0);
     }
   maug =
     e_int_menus_menu_augmentation_add_sorted("config/0", _("Settings Panel"),
                                              _e_mod_menu_add, NULL, NULL, NULL);
   e_module_delayed_set(m, 1);

   snprintf(buf, sizeof(buf), "%s/e-module-conf.edj",
            e_module_dir_get(conf_module));

   e_configure_registry_category_add("advanced", 80, _("Advanced"),
                                     NULL, "preferences-advanced");
   e_configure_registry_item_add("advanced/conf", 110, _("Configuration Panel"),
                                 NULL, buf, e_int_config_conf_module);
   e_configure_registry_item_add("advanced/sleep", 120, _("Sleep Timeout Configuration"),
                                 NULL, buf, e_int_config_sleep_module);

   conf_edd = E_CONFIG_DD_NEW("Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, menu_augmentation, INT);
   E_CONFIG_VAL(D, T, countdown.hour, DOUBLE);
   E_CONFIG_VAL(D, T, countdown.min, DOUBLE);
   E_CONFIG_VAL(D, T, countdown.time, DOUBLE);
   E_CONFIG_VAL(D, T, countdown.active, INT);
   E_CONFIG_VAL(D, T, clockmode.hour, DOUBLE);
   E_CONFIG_VAL(D, T, clockmode.min, DOUBLE);
   E_CONFIG_VAL(D, T, clockmode.time, DOUBLE);
   E_CONFIG_VAL(D, T, clockmode.active, INT);
   E_CONFIG_VAL(D, T, clockmode.always, INT);
   E_CONFIG_VAL(D, T, sleep_type, INT);

   conf = e_config_domain_load("module.conf", conf_edd);
   if (conf)
     {
        if (!e_util_module_config_check(_("Configuration Panel"), conf->version, MOD_CONFIG_FILE_VERSION))
          _conf_free();
     }

   if (!conf) _conf_new();
   conf->module = m;

   if (conf->menu_augmentation)
     {
        conf->aug =
          e_int_menus_menu_augmentation_add
            ("config/2", e_mod_config_menu_add, NULL, NULL, NULL);
     }
   conf->countdown_timer = NULL;
   conf->clockmode_timer = NULL;

   e_gadcon_provider_register(&_gadcon_class);

   e_mod_mode_sleep_countdown_timer_reset();
   e_mod_mode_sleep_clockmode_timer_reset();
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   e_configure_del();

   e_configure_registry_item_del("advanced/conf");
   e_configure_registry_category_del("advanced");

   if (conf->cfd) e_object_del(E_OBJECT(conf->cfd));
   conf->cfd = NULL;

   e_gadcon_provider_unregister(&_gadcon_class);

   /* remove module-supplied menu additions */
   if (maug)
     {
        e_int_menus_menu_augmentation_del("config/0", maug);
        maug = NULL;
     }
   if (conf->aug)
     {
        e_int_menus_menu_augmentation_del("config/2", conf->aug);
        conf->aug = NULL;
     }

   /* remove module-supplied action */
   if (act)
     {
        e_action_predef_name_del("Launch", "Settings Panel");
        e_action_del("configuration");
        act = NULL;
     }
   conf_module = NULL;

   E_FREE(conf);
   E_CONFIG_DD_FREE(conf_edd);

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.conf", conf_edd, conf);
   return 1;
}

/* action callback */
static void
_e_mod_action_conf_cb(E_Object *obj, const char *params)
{
   E_Zone *zone = NULL;

   if (obj)
     {
        if (obj->type == E_MANAGER_TYPE)
          zone = e_util_zone_current_get((E_Manager *)obj);
        else if (obj->type == E_CONTAINER_TYPE)
          zone = e_util_zone_current_get(((E_Container *)obj)->manager);
        else if (obj->type == E_ZONE_TYPE)
          zone = ((E_Zone *)obj);
        else
          zone = e_util_zone_current_get(e_manager_current_get());
     }
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   if ((zone) && (params))
     e_configure_registry_call(params, zone->container, params);
   else if (zone)
     e_configure_show(zone->container, params);
}

/* menu item callback(s) */
static void
_e_mod_conf_cb(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   e_configure_show(m->zone->container, NULL);
}

static void
_e_mod_mode_sleep_toggle(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Container *con = e_container_current_get(e_manager_current_get());
   e_int_config_sleep_module(con, NULL);
}

static void
_e_mod_mode_presentation_toggle(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi)
{
   e_config->mode.presentation = !e_config->mode.presentation;
   e_menu_item_toggle_set(mi, e_config->mode.presentation);
   e_config_mode_changed();
   e_config_save_queue();
}

static void
_e_mod_mode_offline_toggle(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi)
{
   e_config->mode.offline = !e_config->mode.offline;
   e_menu_item_toggle_set(mi, e_config->mode.offline);
   e_config_mode_changed();
   e_config_save_queue();
}

static void
_e_mod_submenu_modes_fill(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_check_set(mi, 1);
   if (conf->countdown.active || conf->clockmode.active)
     e_menu_item_toggle_set(mi, 1);
   else
     e_menu_item_toggle_set(mi, 0);
   e_menu_item_label_set(mi, _("Sleep Timeout"));
   e_util_menu_item_theme_icon_set(mi, "preferences-modes-sleep");
   e_menu_item_callback_set(mi, _e_mod_mode_sleep_toggle, NULL);

   mi = e_menu_item_new(m);
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, e_config->mode.presentation);
   e_menu_item_label_set(mi, _("Presentation"));
   e_util_menu_item_theme_icon_set(mi, "preferences-modes-presentation");
   e_menu_item_callback_set(mi, _e_mod_mode_presentation_toggle, NULL);

   mi = e_menu_item_new(m);
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, e_config->mode.offline);
   e_menu_item_label_set(mi, _("Offline"));
   e_util_menu_item_theme_icon_set(mi, "preferences-modes-offline");
   e_menu_item_callback_set(mi, _e_mod_mode_offline_toggle, NULL);

   e_menu_pre_activate_callback_set(m, NULL, NULL);
}

static E_Menu *
_e_mod_submenu_modes_get(void)
{
   E_Menu *m;

   if (!(m = e_menu_new())) return NULL;
   e_menu_pre_activate_callback_set(m, _e_mod_submenu_modes_fill, NULL);
   return m;
}

/* menu item add hook */
static void
_e_mod_menu_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Settings Panel"));
   e_util_menu_item_theme_icon_set(mi, "preferences-system");
   e_menu_item_callback_set(mi, _e_mod_conf_cb, NULL);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Modes"));
   e_util_menu_item_theme_icon_set(mi, "preferences-modes");
   e_menu_item_submenu_set(mi, _e_mod_submenu_modes_get());
   e_object_unref(E_OBJECT(mi->submenu));
}

static void
_conf_new(void)
{
   conf = E_NEW(Config, 1);
   conf->menu_augmentation = 1;
   conf->sleep_type = 2;

   conf->version = MOD_CONFIG_FILE_VERSION;
   e_config_save_queue();
}

static void
_conf_free(void)
{
   E_FREE(conf);
}

static void
_e_mod_mode_sleep_interval_adjust(Ecore_Timer *timer, int seconds_left)
{
   int interval = 15;

   if (seconds_left > 3600) interval = 1800;
   else if ((seconds_left > 1800) && (seconds_left < 3600)) interval = 900;
   else if ((seconds_left > 900) && (seconds_left < 1800))  interval = 450;
   else if ((seconds_left > 450) && (seconds_left < 900))
     {
        interval = 225;

        if (!conf->alert_message)
          {
             E_Container *con = e_container_current_get(e_manager_current_get());
             e_int_config_sleep_alert(con, NULL);
             conf->alert_message = EINA_TRUE;
          }
     }
   else if ((seconds_left > 225) && (seconds_left < 450))   interval = 115;
   else if ((seconds_left > 115) && (seconds_left < 225))   interval = 60;
   else if ((seconds_left >55 ) && (seconds_left < 115))   interval = 30;
   else if (seconds_left < 55) interval = 5;

 fprintf(stdout, "Changing Ecore_Timer Interval to: %d \n", interval);
 ecore_timer_interval_set(timer, interval);
}

double
e_mod_mode_sleep_time_left_calc(void)
{
   double hour_to_sec = 0 , min_to_sec = 0;

   if (conf->countdown.hour)
     hour_to_sec = (conf->countdown.hour * 3600);

   if (conf->countdown.min)
     min_to_sec = (conf->countdown.min * 60);

   if (hour_to_sec)
     return ((hour_to_sec + min_to_sec) + conf->countdown.time) - ecore_time_unix_get();
   else
     return (min_to_sec + conf->countdown.time) - ecore_time_unix_get();
}

void
e_mod_mode_sleep_countdown_timer_reset(void)
{
   if (conf->countdown_timer)
     ecore_timer_del(conf->countdown_timer);
   conf->countdown_timer = NULL;

   if (conf->countdown.active)
     conf->countdown_timer = ecore_timer_loop_add(5.0, _e_mod_mode_sleep_countdown_timer, NULL);
}

static Eina_Bool
_e_mod_mode_sleep_countdown_timer(void *data __UNUSED__)
{
   double seconds_left;

   if ((!conf->countdown.hour && !conf->countdown.min) || !conf->countdown.active)
     {
        if (conf->countdown_timer) ecore_timer_del(conf->countdown_timer);
        conf->countdown_timer = NULL;

     return ECORE_CALLBACK_CANCEL;
     }

   seconds_left = e_mod_mode_sleep_time_left_calc();

   fprintf(stdout, "Countdown: Time left in seconds:%0.0f \n", seconds_left);
   _e_mod_mode_sleep_interval_adjust(conf->countdown_timer, seconds_left);

   if (seconds_left <= 0 )
     {
        fprintf(stdout, "Action: %d \n", conf->sleep_type);

        conf->countdown.active = 0;
        e_config_save_queue();


        if ((int) seconds_left < (-5))
          {
             fprintf(stdout, "Seems you just turned on the pc... ignoring old sleep time\n");

             if (conf->countdown_timer) ecore_timer_del(conf->countdown_timer);
             conf->countdown_timer = NULL;

             return ECORE_CALLBACK_DONE;
          }

        if (conf->sleep_type == 2)
          e_sys_action_do(E_SYS_SUSPEND, NULL);
        else if (conf->sleep_type == 1)
          e_sys_action_do(E_SYS_HIBERNATE, NULL);
        else if (conf->sleep_type == 0)
          e_sys_action_do(E_SYS_HALT_NOW, NULL);

        if (conf->countdown_timer) ecore_timer_del(conf->countdown_timer);
        conf->countdown_timer = NULL;

        return ECORE_CALLBACK_DONE;
     }

   return ECORE_CALLBACK_RENEW;
}

void
e_mod_mode_sleep_clockmode_timer_reset(void)
{
   if (conf->clockmode_timer)
     ecore_timer_del(conf->clockmode_timer);
   conf->clockmode_timer = NULL;

   if (conf->clockmode.active)
     conf->clockmode_timer = ecore_timer_loop_add(5.0, _e_mod_mode_sleep_clockmode_timer, NULL);
}


double
e_mod_mode_sleep_clockmode_epoch_time_get(void)
{
   struct tm ts;
   struct tm *ts_today;
   time_t t;
   double epoch;

   t = time(NULL);
   ts_today = localtime(&t);

   localtime_r(&t, &ts);
   ts.tm_sec = 0;
   ts.tm_min = (int) conf->clockmode.min;
   ts.tm_hour = (int) conf->clockmode.hour;
   ts.tm_mday = ts_today->tm_mday;
   ts.tm_mon = ts_today->tm_mon;
   ts.tm_year = ts_today->tm_year;
   epoch = (double)mktime(&ts);

   if (epoch < ecore_time_unix_get())
     epoch += 3600*24;

   return epoch;
}

static Eina_Bool
_e_mod_mode_sleep_clockmode_timer(void *data __UNUSED__)
{
   double seconds_left;

   if (!conf->clockmode.active)
     {
        if (conf->clockmode_timer) ecore_timer_del(conf->clockmode_timer);
        conf->clockmode_timer = NULL;

     return ECORE_CALLBACK_CANCEL;
     }

   seconds_left = conf->clockmode.time - ecore_time_unix_get();
   fprintf(stdout, "ClockMode: Time left in seconds:%0.0f \n", seconds_left);
   _e_mod_mode_sleep_interval_adjust(conf->clockmode_timer, seconds_left);

   if ((int) seconds_left <= 0 )
     {
        fprintf(stdout, "Action: %d \n", conf->sleep_type);

        if (conf->clockmode.always)
          conf->clockmode.time = e_mod_mode_sleep_clockmode_epoch_time_get();
        else
          conf->clockmode.active = 0;

        e_config_save_queue();
        if ((int) seconds_left < (-5))
          {
             fprintf(stdout, "Seems you just turned on the pc... ignoring old sleep time\n");

             if (conf->clockmode.always)
               {
                  conf->clockmode.time = e_mod_mode_sleep_clockmode_epoch_time_get();
                  e_config_save_queue();

                  return ECORE_CALLBACK_RENEW;
               }
             else
               {
                  conf->clockmode.active = 0;
                  e_config_save_queue();

                  if (conf->clockmode_timer) ecore_timer_del(conf->clockmode_timer);
                  conf->clockmode_timer = NULL;
                  return ECORE_CALLBACK_DONE;
               }
          }


        if (conf->sleep_type == 2)
          e_sys_action_do(E_SYS_SUSPEND, NULL);
        else if (conf->sleep_type == 1)
          e_sys_action_do(E_SYS_HIBERNATE, NULL);
        else if (conf->sleep_type == 0)
          e_sys_action_do(E_SYS_HALT_NOW, NULL);

        if (conf->clockmode.active)
          return ECORE_CALLBACK_RENEW;

        if (conf->clockmode_timer) ecore_timer_del(conf->clockmode_timer);
        conf->clockmode_timer = NULL;

        return ECORE_CALLBACK_DONE;
     }

   return ECORE_CALLBACK_RENEW;
}

