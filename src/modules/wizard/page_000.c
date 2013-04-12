/* Splash Screen */

#include "e_wizard.h"

static Ecore_Timer *_next_timer = NULL;

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__, Eina_Bool *need_xdg_desktops __UNUSED__, Eina_Bool *need_xdg_icons __UNUSED__)
{
   e_pre_wizard_init();
   return 1;
}

/*
EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}
*/
static Eina_Bool
_next_page(void *data __UNUSED__)
{
   if(!e_pre_wizard_done()) return ECORE_CALLBACK_RENEW;
   e_wizard_button_next_enable_set(1);
   e_wizard_next();
   return ECORE_CALLBACK_CANCEL;
}

EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o;

   TS(__FILE__);
   e_wizard_title_set(_("Enlightenment"));
   o = edje_object_add(pg->evas);
   e_theme_edje_object_set(o, "base/theme/wizard", "e/wizard/firstpage");
   e_wizard_page_show(o);
   e_wizard_button_wait();

   /* advance in 1 sec */
   if (!_next_timer)
     _next_timer = ecore_timer_add(1.0, _next_page, NULL);
   return 1;
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   if (_next_timer) ecore_timer_del(_next_timer);
   _next_timer = NULL;
   return 1;
}
/*
EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}
*/
