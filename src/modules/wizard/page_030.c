/* Menu setup */
#include "e_wizard.h"
/*
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
*/
EAPI int
wizard_page_show(E_Wizard_Page *pg __UNUSED__)
{
   char buf[PATH_MAX];

   TS(__FILE__);
   snprintf(buf, sizeof(buf), "%s/etc/xdg/menus/enlightenment.menu",
            e_prefix_get());
   e_config->default_system_menu = eina_stringshare_add(buf);
   return 0; /* 1 == show ui, and wait for user, 0 == just continue */
}
/*
EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}
*/
