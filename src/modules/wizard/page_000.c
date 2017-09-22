/* Splash Screen */

#include "e_wizard.h"

static Ecore_Timer *_next_timer = NULL;
EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__, Eina_Bool *need_xdg_desktops __UNUSED__, Eina_Bool *need_xdg_icons __UNUSED__)
{
   e_wizard_pre_run();
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
   if (e_wizard_pre_label_get())
     e_wizard_button_label_set(e_wizard_pre_label_get());
   else
     e_wizard_button_wait();

   if(!e_wizard_pre_done()) return ECORE_CALLBACK_RENEW;
   e_wizard_button_next_enable_set(1);
   e_wizard_next();
   return ECORE_CALLBACK_CANCEL;
}

EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o;

   TS(__FILE__);

   // Scale configuration from here
   int dpi;
   double scale = 1.0;

   dpi = ecore_x_dpi_get();
   scale = ((float)dpi / 96);

   // defaults
   if (scale < 0.8) scale = 0.8;
   if (scale > 3.0) scale = 3.0;

   /*// TODO FIXME: temporal elive limitation to max 1.5 scaling: e17 is not so good to work with more than 1.5*/
   if (scale > 1.5) scale = 1.5;

   // set a good scale since the first page (ignored later in page 050)
   e_config->scale.use_dpi = 0;
   e_config->scale.use_custom = 1;
   e_config->scale.factor = scale;
   e_scale_update();


   e_wizard_title_set(_("Enlightenment"));
   o = edje_object_add(pg->evas);
   e_theme_edje_object_set(o, "base/theme/wizard", "e/wizard/firstpage");
   e_wizard_page_show(o);
   e_wizard_button_wait();

   /* advance in 0.5 sec */
   if (!_next_timer)
     _next_timer = ecore_timer_add(0.5, _next_page, NULL);
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
