#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_comp.h"
#include "e_comp_cfdata.h"

static E_Int_Menu_Augmentation *maug = NULL;

/* module private routines */
EINTERN Mod *_comp_mod = NULL;

/* public module routines. all modules must have these */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Composite Settings"
};

static void
_e_mod_comp_conf_cb(void *data __UNUSED__, E_Menu *m EINA_UNUSED, E_Menu_Item *mi __UNUSED__)
{
   e_int_config_comp_module(NULL, NULL);
}

static void
_e_mod_config_menu_create(void *d EINA_UNUSED, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Composite"));
   e_util_menu_item_theme_icon_set(mi, "preferences-composite");
   e_menu_item_callback_set(mi, _e_mod_comp_conf_cb, NULL);
}

EAPI void *
e_modapi_init(E_Module *m)
{
   Mod *mod;
   E_Comp_Config *conf;

   conf = e_comp_config_get();
   if (!conf)
     {
        EINA_LOG_CRIT("COMP CONFIG MISSING!!!! ARGH!");
        return NULL;
     }

   mod = calloc(1, sizeof(Mod));
   m->data = mod;

   mod->module = m;
   e_configure_registry_category_add("appearance", 10, _("Look"), NULL,
                                     "preferences-look");
   e_configure_registry_item_add("appearance/comp", 120, _("Composite"), NULL,
                                 "preferences-composite", e_int_config_comp_module);

   mod->conf = conf;
   maug = e_int_menus_menu_augmentation_add_sorted("config/1", _("Composite"), _e_mod_config_menu_create, NULL, NULL, NULL);
   mod->conf->max_unmapped_pixels = 32 * 1024;
   mod->conf->keep_unmapped = 1;

   /* force some config vals off */
   mod->conf->lock_fps = 0;
   mod->conf->indirect = 0;

   /* XXX: update old configs. add config versioning */
   if (mod->conf->first_draw_delay == 0)
     mod->conf->first_draw_delay = 0.20;

   _comp_mod = mod;
   {
      E_Configure_Option *co;

      e_configure_option_domain_current_set("conf_comp");
      E_CONFIGURE_OPTION_ADD_CUSTOM(co, _("settings"), _("Composite settings panel"), _("composite"), _("border"));
      co->info = eina_stringshare_add("appearance/comp");
      E_CONFIGURE_OPTION_ICON(co, "preferences-composite");
   }

   e_module_delayed_set(m, 0);
   e_module_priority_set(m, -1000);

   return mod;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   Mod *mod = m->data;

   e_configure_registry_item_del("appearance/comp");
   e_configure_registry_category_del("appearance");

   if (mod->config_dialog)
     {
        e_object_del(E_OBJECT(mod->config_dialog));
        mod->config_dialog = NULL;
     }

   free(mod);
   e_configure_option_domain_clear("conf_comp");

   if (maug)
     {
        e_int_menus_menu_augmentation_del("config/1", maug);
        maug = NULL;
     }

   if (mod == _comp_mod) _comp_mod = NULL;

   return 1;
}