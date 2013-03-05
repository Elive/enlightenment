#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

/* Increment for Major Changes */
#define MOD_CONFIG_FILE_EPOCH      1
/* Increment for Minor Changes (ie: user doesn't need a new config) */
#define MOD_CONFIG_FILE_GENERATION 0
#define MOD_CONFIG_FILE_VERSION    ((MOD_CONFIG_FILE_EPOCH * 1000000) + MOD_CONFIG_FILE_GENERATION)

typedef struct _Config Config;
struct _Config 
{
   E_Module *module;
   E_Config_Dialog *cfd;
   E_Int_Menu_Augmentation *aug;
   Ecore_Timer *countdown_timer;
   Ecore_Timer *clockmode_timer;
   int version;
   int menu_augmentation;

   int sleep_type;
   Eina_Bool alert_message;
   struct
     {
        int active;
        int always;
        double hour;
        double min;
        double time;
     }countdown, clockmode;
};

void e_configure_show(E_Container *con, const char *params);
void e_configure_del(void);

E_Config_Dialog *e_int_config_conf_module(E_Container *con, const char *params);
E_Config_Dialog *e_int_config_sleep_module(E_Container *con, const char *params);
E_Config_Dialog *e_int_config_sleep_alert(E_Container *con, const char *params);
void e_mod_config_menu_add(void *data, E_Menu *m);
void e_mod_mode_sleep_countdown_timer_reset(void);
void e_mod_mode_sleep_clockmode_timer_reset(void);
double e_mod_mode_sleep_clockmode_epoch_time_get(void);
double e_mod_mode_sleep_time_left_calc(void);

extern Config *conf;

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf Main Configuration Dialog
 *
 * Show the main configuration dialog used to access other
 * configuration.
 *
 * @}
 */

#endif
