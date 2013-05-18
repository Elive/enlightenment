/* Delete previous copy of config profile and save new one */
#include "e_wizard.h"

static Ecore_Timer *_next_timer = NULL;
#if 0
EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__, Eina_Bool *need_xdg_desktops __UNUSED__, Eina_Bool *need_xdg_icons __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_show(E_Wizard_Page *pg __UNUSED__)
{
   return 0; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}
#endif

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__, Eina_Bool *need_xdg_desktops __UNUSED__, Eina_Bool *need_xdg_icons __UNUSED__)
{
   e_wizard_post_run();
   return 1;
}

static Eina_Bool
_end_page(void *data __UNUSED__)
{
   if (e_wizard_pre_label_get())
     e_wizard_button_label_set(e_wizard_pre_label_get());
   else
     e_wizard_button_wait();

   if(!e_wizard_pre_done()) return ECORE_CALLBACK_RENEW;
   e_util_env_set("E_RESTART", NULL);
   e_env_unset("E_RESTART_COUNT"); 
   // restart e
   e_sys_action_do(E_SYS_RESTART, NULL);
   TS("DONE");
   return ECORE_CALLBACK_CANCEL;
}

EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o;
   TS(__FILE__);

   e_config->wizard_after = 1;
   // save the config now everyone has modified it
   e_config_save();

   e_wizard_title_set(_("Enlightenment Completing Configuration"));
   o = edje_object_add(pg->evas);
   e_theme_edje_object_set(o, "base/theme/wizard", "e/wizard/firstpage");
   e_wizard_page_show(o);
   e_wizard_button_wait();

   // diusable restart env so we actually start a whole new session properly 
   if (!_next_timer)
     _next_timer = ecore_timer_add(0.5, _end_page, NULL);

   return 1;
}

