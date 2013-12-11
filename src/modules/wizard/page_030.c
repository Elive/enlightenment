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

static void
_wizard_update_menu_cache(Efreet_Menu *menu)
{
   Eina_List *l;

   if (menu->entries)
     {
        Efreet_Menu *entry;

        EINA_LIST_FOREACH(menu->entries, l, entry)
          {
             if (entry->type == EFREET_MENU_ENTRY_DESKTOP)
               e_int_menus_cache_update(entry->desktop);
             else if (entry->type == EFREET_MENU_ENTRY_MENU)
               _wizard_update_menu_cache(entry);
          }
     }
}

EAPI int
wizard_page_show(E_Wizard_Page *pg __UNUSED__)
{
   Efreet_Menu *menu = NULL;
   char buf[PATH_MAX];

   TS(__FILE__);
   snprintf(buf, sizeof(buf), "%s/etc/xdg/menus/enlightenment.menu",
            e_prefix_get());
   e_config->default_system_menu = eina_stringshare_add(buf);

   menu = efreet_menu_get();

   if (menu) _wizard_update_menu_cache(menu);
   e_config_save_queue();
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
