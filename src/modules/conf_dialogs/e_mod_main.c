#include "e.h"
#include "e_mod_main.h"

/* actual module specifics */
static E_Module *conf_module = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Settings - Dialogs"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("settings", 80, _("Settings"), NULL, "preferences-system");
   e_configure_registry_item_add("settings/dialogs", 10, _("Dialogs"), NULL, "preferences-system", e_int_config_dialogs);
   e_configure_registry_item_add("settings/profiles", 50, _("Profiles"), NULL, "preferences-profiles", e_int_config_profiles);
   e_configure_registry_item_add("settings/erestartcfg", 50, _("E Restart Config"), NULL, "preferences-profiles", e_int_config_erestart);
   conf_module = m;
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;
   while ((cfd = e_config_dialog_get("E", "settings/erestartcfg"))) e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "settings/profiles"))) e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "settings/dialogs"))) e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("settings/erestartcfg");
   e_configure_registry_item_del("settings/profiles");
   e_configure_registry_item_del("settings/dialogs");
   e_configure_registry_category_del("settings");
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
