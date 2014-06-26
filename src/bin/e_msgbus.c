#include "e.h"

/* local subsystem functions */
static void _e_msgbus_request_name_cb(void        *data,
                                      DBusMessage *msg,
                                      DBusError   *err);

static DBusMessage *_e_msgbus_core_restart_cb(E_DBus_Object *obj,
                                              DBusMessage   *msg);
static DBusMessage *_e_msgbus_core_shutdown_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_core_hibernate_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_core_suspend_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_core_e_restart_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_core_e_exit_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_core_logout_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_core_lock_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_core_exec_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_core_save_setting_cb(E_DBus_Object *obj,
                                                   DBusMessage *msg);

static DBusMessage *_e_msgbus_module_load_cb(E_DBus_Object *obj,
                                             DBusMessage   *msg);
static DBusMessage *_e_msgbus_module_unload_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_module_enable_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_module_disable_cb(E_DBus_Object *obj,
                                                DBusMessage   *msg);
static DBusMessage *_e_msgbus_module_list_cb(E_DBus_Object *obj,
                                             DBusMessage   *msg);

static DBusMessage *_e_msgbus_profile_set_cb(E_DBus_Object *obj,
                                             DBusMessage   *msg);
static DBusMessage *_e_msgbus_profile_get_cb(E_DBus_Object *obj,
                                             DBusMessage   *msg);
static DBusMessage *_e_msgbus_profile_list_cb(E_DBus_Object *obj,
                                              DBusMessage   *msg);
static DBusMessage *_e_msgbus_profile_add_cb(E_DBus_Object *obj,
                                             DBusMessage   *msg);
static DBusMessage *_e_msgbus_profile_delete_cb(E_DBus_Object *obj,
                                                DBusMessage   *msg);

static DBusMessage *_e_msgbus_resolution_get_cb(E_DBus_Object *obj,
                                                DBusMessage *msg);
static DBusMessage *_e_msgbus_resolution_set_cb(E_DBus_Object *obj,
                                                DBusMessage *msg);
static DBusMessage *_e_msgbus_resolution_list_cb(E_DBus_Object *obj,
                                                 DBusMessage *msg);

static DBusMessage *_e_msgbus_edje_cacheget_cb(E_DBus_Object *obj,
                                               DBusMessage *msg);
static DBusMessage *_e_msgbus_edje_cacheset_cb(E_DBus_Object *obj,
                                               DBusMessage *msg);
static DBusMessage *_e_msgbus_edge_collection_cacheget_cb(E_DBus_Object *obj,
                                                          DBusMessage *msg);
static DBusMessage *_e_msgbus_edge_collection_cacheset_cb(E_DBus_Object *obj,
                                                          DBusMessage *msg);

static DBusMessage *_e_msgbus_image_get_cb(E_DBus_Object *obj,
                                           DBusMessage *msg);
static DBusMessage *_e_msgbus_image_set_cb(E_DBus_Object *obj,
                                           DBusMessage *msg);

static DBusMessage *_e_msgbus_font_get_cb(E_DBus_Object *obj,
                                          DBusMessage *msg);
static DBusMessage *_e_msgbus_font_set_cb(E_DBus_Object *obj,
                                          DBusMessage *msg);

static DBusMessage *_e_msgbus_focus_get_cb(E_DBus_Object *obj,
                                           DBusMessage *msg);
static DBusMessage *_e_msgbus_focus_set_cb(E_DBus_Object *obj,
                                           DBusMessage *msg);

static DBusMessage *_e_msgbus_theme_get_cb(E_DBus_Object *obj,
                                           DBusMessage   *msg);
static DBusMessage *_e_msgbus_theme_set_cb(E_DBus_Object *obj,
                                           DBusMessage   *msg);
static DBusMessage *_e_msgbus_theme_list_cb(E_DBus_Object *obj,
                                            DBusMessage   *msg);
static DBusMessage *_e_msgbus_theme_add_cb(E_DBus_Object *obj,
                                           DBusMessage   *msg);
static DBusMessage *_e_msgbus_theme_del_cb(E_DBus_Object *obj,
                                              DBusMessage   *msg);
static DBusMessage *_e_msgbus_theme_lut_cb(E_DBus_Object *obj,
                                              DBusMessage   *msg);

static DBusMessage *_e_msgbus_intl_get_cb(E_DBus_Object *obj,
                                          DBusMessage   *msg);
static DBusMessage *_e_msgbus_intl_set_cb(E_DBus_Object *obj,
                                          DBusMessage   *msg);
static DBusMessage *_e_msgbus_intl_list_cb(E_DBus_Object *obj,
                                          DBusMessage   *msg);

static DBusMessage *_e_msgbus_wallpaper_add_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_wallpaper_set_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);

static DBusMessage *_e_msgbus_exec_cmd_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
#define E_MSGBUS_WIN_ACTION_CB_PROTO(NAME) \
static DBusMessage *_e_msgbus_window_##NAME##_cb(E_DBus_Object *obj __UNUSED__, DBusMessage   *msg)

E_MSGBUS_WIN_ACTION_CB_PROTO(list);
E_MSGBUS_WIN_ACTION_CB_PROTO(close);
E_MSGBUS_WIN_ACTION_CB_PROTO(kill);
E_MSGBUS_WIN_ACTION_CB_PROTO(focus);
E_MSGBUS_WIN_ACTION_CB_PROTO(iconify);
E_MSGBUS_WIN_ACTION_CB_PROTO(uniconify);
E_MSGBUS_WIN_ACTION_CB_PROTO(maximize);
E_MSGBUS_WIN_ACTION_CB_PROTO(unmaximize);

/* local subsystem globals */
static E_Msgbus_Data *_e_msgbus_data = NULL;

/* externally accessible functions */
EINTERN int
e_msgbus_init(void)
{
   E_DBus_Interface *iface;

   _e_msgbus_data = E_NEW(E_Msgbus_Data, 1);

   e_dbus_init();
#ifdef HAVE_HAL
   e_hal_init();
#endif

   _e_msgbus_data->conn = e_dbus_bus_get(DBUS_BUS_SESSION);
   if (!_e_msgbus_data->conn)
     {
        WRN("Cannot get DBUS_BUS_SESSION");
        return 0;
     }
   e_dbus_request_name(_e_msgbus_data->conn,
           "org.enlightenment.wm.service", 0, _e_msgbus_request_name_cb, NULL);
   _e_msgbus_data->obj = e_dbus_object_add(_e_msgbus_data->conn,
           "/org/enlightenment/wm/RemoteObject", NULL);

   iface = e_dbus_interface_new("org.enlightenment.wm.Core");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Core interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Hardcore methods */
   e_dbus_interface_method_add(iface, "Restart", "", "", _e_msgbus_core_restart_cb);
   e_dbus_interface_method_add(iface, "Shutdown", "", "", _e_msgbus_core_shutdown_cb);
   e_dbus_interface_method_add(iface, "Hibernate", "", "", _e_msgbus_core_hibernate_cb);
   e_dbus_interface_method_add(iface, "Suspend", "", "", _e_msgbus_core_suspend_cb);

   e_dbus_interface_method_add(iface, "E_Restart", "", "", _e_msgbus_core_e_restart_cb);
   e_dbus_interface_method_add(iface, "E_Exit", "", "", _e_msgbus_core_e_exit_cb);

   e_dbus_interface_method_add(iface, "Logout", "", "", _e_msgbus_core_logout_cb);
   e_dbus_interface_method_add(iface, "Lock", "", "", _e_msgbus_core_lock_cb);
   e_dbus_interface_method_add(iface, "Execute", "s", "", _e_msgbus_core_exec_cb);
   e_dbus_interface_method_add(iface, "Save_Setting", "", "", _e_msgbus_core_save_setting_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Module");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Module interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Module methods */
   e_dbus_interface_method_add(iface, "Load", "s", "", _e_msgbus_module_load_cb);
   e_dbus_interface_method_add(iface, "Unload", "s", "", _e_msgbus_module_unload_cb);
   e_dbus_interface_method_add(iface, "Enable", "s", "", _e_msgbus_module_enable_cb);
   e_dbus_interface_method_add(iface, "Disable", "s", "", _e_msgbus_module_disable_cb);
   e_dbus_interface_method_add(iface, "List", "", "a(si)", _e_msgbus_module_list_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Profile");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Profile interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Profile methods */
   e_dbus_interface_method_add(iface, "Set", "s", "", _e_msgbus_profile_set_cb);
   e_dbus_interface_method_add(iface, "Get", "", "s", _e_msgbus_profile_get_cb);
   e_dbus_interface_method_add(iface, "List", "", "as", _e_msgbus_profile_list_cb);
   e_dbus_interface_method_add(iface, "Add", "s", "", _e_msgbus_profile_add_cb);
   e_dbus_interface_method_add(iface, "Delete", "s", "", _e_msgbus_profile_delete_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Window");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Window interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Window methods */
   e_dbus_interface_method_add(iface, "List", "", "a(si)", _e_msgbus_window_list_cb);
   e_dbus_interface_method_add(iface, "Close", "i", "", _e_msgbus_window_close_cb);
   e_dbus_interface_method_add(iface, "Kill", "i", "", _e_msgbus_window_kill_cb);
   e_dbus_interface_method_add(iface, "Focus", "i", "", _e_msgbus_window_focus_cb);
   e_dbus_interface_method_add(iface, "Iconify", "i", "", _e_msgbus_window_iconify_cb);
   e_dbus_interface_method_add(iface, "Uniconify", "i", "", _e_msgbus_window_uniconify_cb);
   e_dbus_interface_method_add(iface, "Maximize", "i", "", _e_msgbus_window_maximize_cb);
   e_dbus_interface_method_add(iface, "Unmaximize", "i", "", _e_msgbus_window_unmaximize_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Wallpaper");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Wallpaper interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Wallpaper methods */
   e_dbus_interface_method_add(iface, "Add", "s", "", _e_msgbus_wallpaper_add_cb);
   e_dbus_interface_method_add(iface, "Set", "s", "", _e_msgbus_wallpaper_set_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Intl");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Intl interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Intl methods */
   e_dbus_interface_method_add(iface, "Get", "", "s", _e_msgbus_intl_get_cb);
   e_dbus_interface_method_add(iface, "Set", "s", "", _e_msgbus_intl_set_cb);
   e_dbus_interface_method_add(iface, "List", "", "s", _e_msgbus_intl_list_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Theme");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Theme interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Theme methods */
   e_dbus_interface_method_add(iface, "Get", "s", "", _e_msgbus_theme_get_cb);
   e_dbus_interface_method_add(iface, "Set", "ss", "", _e_msgbus_theme_set_cb);
   e_dbus_interface_method_add(iface, "List", "", "s", _e_msgbus_theme_list_cb);
   e_dbus_interface_method_add(iface, "Add", "s", "", _e_msgbus_theme_add_cb);
   e_dbus_interface_method_add(iface, "Del", "s", "", _e_msgbus_theme_del_cb);
   e_dbus_interface_method_add(iface, "List_User_Themes", "", "s", _e_msgbus_theme_lut_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Focus");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Focus interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Focus methods */
   e_dbus_interface_method_add(iface, "Get", "", "s", _e_msgbus_focus_get_cb);
   e_dbus_interface_method_add(iface, "Set", "s", "", _e_msgbus_focus_set_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Font");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Font interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Font methods */
   e_dbus_interface_method_add(iface, "Get", "", "d", _e_msgbus_font_get_cb);
   e_dbus_interface_method_add(iface, "Set", "d", "", _e_msgbus_font_set_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Image");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Image interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Image methods */
   e_dbus_interface_method_add(iface, "Get", "", "d", _e_msgbus_image_get_cb);
   e_dbus_interface_method_add(iface, "Set", "d", "", _e_msgbus_image_set_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Edje");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Edje interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Edje methods */
   e_dbus_interface_method_add(iface, "Cache_Get", "", "i", _e_msgbus_edje_cacheget_cb);
   e_dbus_interface_method_add(iface, "Cache_Set", "i", "", _e_msgbus_edje_cacheset_cb);
   e_dbus_interface_method_add(iface, "Collection_Cache_Get", "", "i",
                               _e_msgbus_edge_collection_cacheget_cb);
   e_dbus_interface_method_add(iface, "Collection_Cache_Set", "i", "",
                               _e_msgbus_edge_collection_cacheset_cb);


   iface = e_dbus_interface_new("org.enlightenment.wm.Resolution");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Resolution interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Resolution methods */
   e_dbus_interface_method_add(iface, "Get", "", "s", _e_msgbus_resolution_get_cb);
   e_dbus_interface_method_add(iface, "Set", "i", "", _e_msgbus_resolution_set_cb);
   e_dbus_interface_method_add(iface, "List", "", "s", _e_msgbus_resolution_list_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Exec");
   if (!iface)
     {
        WRN("Cannot add org.enlightenment.wm.Exec interface");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Exec methods */
   e_dbus_interface_method_add(iface, "Cmd", "s", "", _e_msgbus_exec_cmd_cb);
   return 1;
}

EINTERN int
e_msgbus_shutdown(void)
{
   if (_e_msgbus_data->obj)
     {
        e_dbus_object_free(_e_msgbus_data->obj);
     }
   if (_e_msgbus_data->conn)
     {
        e_dbus_connection_close(_e_msgbus_data->conn);
     }
#ifdef HAVE_HAL
   e_hal_shutdown();
#endif
   e_dbus_shutdown();

   E_FREE(_e_msgbus_data);
   _e_msgbus_data = NULL;
   return 1;
}

EAPI void
e_msgbus_interface_attach(E_DBus_Interface *iface)
{
   if (!_e_msgbus_data->obj) return;
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
}

EAPI void
e_msgbus_interface_detach(E_DBus_Interface *iface)
{
   if (!_e_msgbus_data->obj) return;
   e_dbus_object_interface_detach(_e_msgbus_data->obj, iface);
}

static void
_e_msgbus_request_name_cb(void        *data __UNUSED__,
                          DBusMessage *msg __UNUSED__,
                          DBusError   *err __UNUSED__)
{
//TODO Handle Errors
}

/* Core Handlers */
static DBusMessage *
_e_msgbus_core_restart_cb(E_DBus_Object *obj __UNUSED__,
                          DBusMessage   *msg)
{
   e_sys_action_do(E_SYS_REBOOT, NULL);
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_core_shutdown_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage   *msg)
{
   e_sys_action_do(E_SYS_HALT_NOW, NULL);
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_core_hibernate_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage   *msg)
{
   e_sys_action_do(E_SYS_HIBERNATE, NULL);
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_core_suspend_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage   *msg)
{
   e_sys_action_do(E_SYS_SUSPEND, NULL);
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_core_e_restart_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage   *msg)
{
   E_Action *a;

   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_core_e_exit_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage   *msg)
{
   E_Action *a;

   a = e_action_find("exit");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_core_logout_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage   *msg)
{
   e_sys_action_do(E_SYS_LOGOUT, NULL);
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_core_lock_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage   *msg)
{
   e_desklock_show(EINA_TRUE);
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_core_exec_cb(E_DBus_Object *odj __UNUSED__,
                       DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *exec;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &exec);

   if(exec)
     {
        ecore_exe_run(exec, NULL);
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_core_save_setting_cb(E_DBus_Object *obj __UNUSED__,
                        DBusMessage   *msg)
{
   e_config_save();
   return dbus_message_new_method_return(msg);
}
/* Modules Handlers */
static DBusMessage *
_e_msgbus_module_load_cb(E_DBus_Object *obj __UNUSED__,
                         DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *module;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &module);

   if (!e_module_find(module))
     {
        e_module_new(module);
        e_config_save_queue();
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_module_unload_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *module;
   E_Module *m;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &module);

   if ((m = e_module_find(module)))
     {
        e_module_disable(m);
        e_object_del(E_OBJECT(m));
        e_config_save_queue();
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_module_enable_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *module;
   E_Module *m;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &module);

   if ((m = e_module_find(module)))
     {
        e_module_enable(m);
        e_config_save_queue();
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_module_disable_cb(E_DBus_Object *obj __UNUSED__,
                            DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *module;
   E_Module *m;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &module);

   if ((m = e_module_find(module)))
     {
        e_module_disable(m);
        e_config_save_queue();
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_module_list_cb(E_DBus_Object *obj __UNUSED__,
                         DBusMessage   *msg)
{
   Eina_List *l;
   E_Module *mod;
   DBusMessage *reply;
   DBusMessageIter iter;
   DBusMessageIter arr;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(si)", &arr);

   EINA_LIST_FOREACH(e_module_list(), l, mod)
     {
        DBusMessageIter sub;
        const char *name;
        int enabled;

        name = mod->name;
        enabled = mod->enabled;
        dbus_message_iter_open_container(&arr, DBUS_TYPE_STRUCT, NULL, &sub);
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &(name));
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &(enabled));
        dbus_message_iter_close_container(&arr, &sub);
     }
   dbus_message_iter_close_container(&iter, &arr);

   return reply;
}

/* Profile Handlers */
static DBusMessage *
_e_msgbus_profile_set_cb(E_DBus_Object *obj __UNUSED__,
                         DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *profile;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &profile);

   e_config_save_flush();
   e_config_profile_set(profile);
   e_config_profile_save();
   e_config_save_block_set(1);
   e_sys_action_do(E_SYS_RESTART, NULL);

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_profile_get_cb(E_DBus_Object *obj __UNUSED__,
                         DBusMessage   *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   const char *profile;

   profile = e_config_profile_get();

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &profile);

   return reply;
}

static DBusMessage *
_e_msgbus_profile_list_cb(E_DBus_Object *obj __UNUSED__,
                          DBusMessage   *msg)
{
   Eina_List *l;
   char *name;
   DBusMessage *reply;
   DBusMessageIter iter;
   DBusMessageIter arr;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);

   l = e_config_profile_list();
   EINA_LIST_FREE(l, name)
     {
        dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &name);
        free(name);
     }
   dbus_message_iter_close_container(&iter, &arr);

   return reply;
}

static DBusMessage *
_e_msgbus_profile_add_cb(E_DBus_Object *obj __UNUSED__,
                         DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *profile, buf[PATH_MAX];

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &profile);

   snprintf(buf, sizeof(buf), "%s", e_config_profile_get());

   if (profile && profile[0])
     {   e_config_profile_add(profile);
        e_config_profile_set(profile);
        e_config_save();
        e_config_profile_set(buf);
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_profile_delete_cb(E_DBus_Object *obj __UNUSED__,
                            DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *profile;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &profile);
   if (!strcmp(e_config_profile_get(), profile))
     {
        DBusMessage *ret;

        ret = dbus_message_new_error(msg, "org.enlightenment.DBus.InvalidArgument",
                                     "Can't delete active profile");
        return ret;
     }
   e_config_profile_del(profile);

   return dbus_message_new_method_return(msg);
}

/* Window handlers */
static DBusMessage *
_e_msgbus_window_list_cb(E_DBus_Object *obj __UNUSED__,
                         DBusMessage *msg)
{
   Eina_List *l;
   E_Border *bd;
   DBusMessage *reply;
   DBusMessageIter iter;
   DBusMessageIter arr;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(si)", &arr);

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        DBusMessageIter sub;
        dbus_message_iter_open_container(&arr, DBUS_TYPE_STRUCT, NULL, &sub);
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &bd->client.icccm.name);
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &bd->client.win);
        dbus_message_iter_close_container(&arr, &sub);
     }
   dbus_message_iter_close_container(&iter, &arr);

   return reply;
}

/* Resolution handlers */
static DBusMessage *
_e_msgbus_resolution_get_cb(E_DBus_Object *obj __UNUSED__,
                            DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;
   E_Manager *man;
   Ecore_X_Randr_Screen_Size size;
   char buf[PATH_MAX], *res;

   man = e_manager_current_get();
   ecore_x_randr_screen_primary_output_current_size_get(man->root,
                           &size.width,&size.height,NULL,NULL,0);
   if((size.width > 1) && (size.height > 1))
     {
	snprintf(buf, sizeof(buf), "%dx%d",size.width,size.height);
        res = strdup(buf);
     }

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &res);

   return  reply;
}

static DBusMessage *
_e_msgbus_resolution_set_cb(E_DBus_Object *obj __UNUSED__,
                            DBusMessage *msg)
{
   DBusMessageIter iter;
   E_Manager *man;
   int index;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &index);

   man = e_manager_current_get();
   ecore_x_randr_screen_primary_output_size_set(man->root,index);

  return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_resolution_list_cb(E_DBus_Object *obj __UNUSED__,
                             DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   DBusMessageIter arr;
   E_Manager *man;
   Ecore_X_Randr_Screen_Size_MM *size;
   char buf[128],*value;
   int i,s;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);

   man = e_manager_current_get();
   size = ecore_x_randr_screen_primary_output_sizes_get(man->root, &s);

   for (i = 0; i < s; i++)
     {
        snprintf(buf, sizeof(buf), "%dx%d %d",size[i].width,size[i].height,i);
        value = strdup(buf);
        dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &value);
        E_FREE(value);
     }
   dbus_message_iter_close_container(&iter, &arr);
   E_FREE(size);
   return reply;
}

static DBusMessage *
_e_msgbus_edje_cacheget_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32,
                                  &e_config->edje_cache);

   return reply;
}

static DBusMessage *
_e_msgbus_edje_cacheset_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage *msg)
{
   DBusMessageIter iter;
   int cache;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &cache);

   e_config->edje_cache = cache;
   E_CONFIG_LIMIT(e_config->edje_cache, 0, 256);
   e_canvas_recache();
   e_config_save_queue();
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_edge_collection_cacheget_cb(E_DBus_Object *obj __UNUSED__,
                                      DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32,
                                  &e_config->edje_collection_cache);

   return reply;
}

static DBusMessage *
_e_msgbus_edge_collection_cacheset_cb(E_DBus_Object *obj __UNUSED__,
                                      DBusMessage *msg)
{
   DBusMessageIter iter;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &e_config->edje_collection_cache);

   E_CONFIG_LIMIT(e_config->edje_collection_cache, 0, 512);
   e_canvas_recache();
   e_config_save_queue();

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_image_get_cb(E_DBus_Object *obj __UNUSED__,
                       DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   double cache;

   cache = ((double)e_config->image_cache / 1024);

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_DOUBLE,
                                  &cache);

   return reply;
}
static DBusMessage *
_e_msgbus_image_set_cb(E_DBus_Object *obj __UNUSED__,
                       DBusMessage *msg)
{
   DBusMessageIter iter;
   double cache;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &cache);

   e_config->image_cache = (cache * 1024);
   E_CONFIG_LIMIT(e_config->image_cache, 0, 32 *1024);
   e_canvas_recache();
   e_config_save_queue();

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_font_get_cb(E_DBus_Object *obj __UNUSED__,
                      DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   double cache;

   cache = ((double)e_config->font_cache / 1024);

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_DOUBLE, &cache);

   return reply;
}

static DBusMessage *
_e_msgbus_font_set_cb(E_DBus_Object *obj __UNUSED__,
                      DBusMessage *msg)
{
   DBusMessageIter iter;
   double cache;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &cache);

   e_config->font_cache = (cache * 1024);
   E_CONFIG_LIMIT(e_config->font_cache, 0, 4 * 1024);
   e_canvas_recache();
   e_config_save_queue();
   return dbus_message_new_method_return(msg);
}



static DBusMessage*
_e_msgbus_focus_get_cb(E_DBus_Object *obj __UNUSED__,
                       DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   char *focus;
   int value = 0;

   value = e_config->focus_policy;
   if (value == E_FOCUS_CLICK)
     focus = "CLICK";
   else if (value == E_FOCUS_MOUSE)
     focus = "MOUSE";
   else if (value == E_FOCUS_SLOPPY)
     focus = "SLOPPY";

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &focus);

   return reply;
}

static DBusMessage*
_e_msgbus_focus_set_cb(E_DBus_Object *obj __UNUSED__,
                       DBusMessage *msg)
{
   DBusMessageIter iter;
   const char *focus;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &focus);

    if ((strncmp(focus, "MOUSE", sizeof(focus)) == 0) ||
        (strncmp(focus, "mouse", sizeof(focus)) == 0 ))
      {
         e_config->focus_policy = E_FOCUS_MOUSE;
         e_border_button_bindings_ungrab_all();
         e_border_button_bindings_grab_all();
         e_config_save_queue();
      }
   else if ((strncmp(focus, "CLICK", sizeof(focus)) == 0) ||
            (strncmp(focus, "click", sizeof(focus)) == 0))
     {
        e_config->focus_policy = E_FOCUS_CLICK;
        e_border_button_bindings_ungrab_all();
        e_border_button_bindings_grab_all();
        e_config_save_queue();
     }
   else if ((strncmp(focus, "SLOPPY", sizeof(focus)) == 0) ||
            (strncmp(focus, "sloppy", sizeof(focus)) == 0))
     {
        e_config->focus_policy = E_FOCUS_SLOPPY;
        e_border_button_bindings_ungrab_all();
        e_border_button_bindings_grab_all();
        e_config_save_queue();
     }
   else
	 ERR("org.enlightenment.wm.Focus.Set must be MOUSE, CLICK, SLOPPY");

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_theme_get_cb(E_DBus_Object *obj __UNUSED__,
                       DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   E_Config_Theme *ect = NULL;
   const char *theme, *err;
   err = strdup("Invalid Theme Category");

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &theme);

   if (theme)
     ect = e_theme_config_get(theme);

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);

   if((ect) && (strlen(ect->file) > 0))
     dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &ect->file);
   else
     dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &err);

   return reply;
}

static DBusMessage *
_e_msgbus_theme_set_cb(E_DBus_Object *obj __UNUSED__,
                       DBusMessage *msg)
{
   DBusMessageIter iter;
   char *category;
   char *edje_file;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &category);
   dbus_message_iter_next(&iter);
   dbus_message_iter_get_basic(&iter, &edje_file);

   e_theme_config_set(category,edje_file);
   e_config_save_queue();
   e_sys_action_do(E_SYS_RESTART, NULL);
   return dbus_message_new_method_return(msg);
}

static DBusMessage*
_e_msgbus_theme_list_cb(E_DBus_Object *obj __UNUSED__,
                        DBusMessage *msg)
{
   Eina_List *l;
   E_Config_Theme *ect = NULL;
   DBusMessage *reply;
   DBusMessageIter iter;
   DBusMessageIter arr;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);
   ect = e_theme_config_get("theme");

   EINA_LIST_FOREACH(e_config->themes, l, ect)
     {
	dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &ect->file);
     }
   dbus_message_iter_close_container(&iter, &arr);

   return reply;
}

static DBusMessage *
_e_msgbus_theme_add_cb(E_DBus_Object *obj __UNUSED__,
                       DBusMessage *msg)
{
   DBusMessageIter iter;
   char *file,buf[PATH_MAX];

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &file);

   snprintf(buf, sizeof(buf), "%s/themes/%s", e_user_dir_get(),
            ecore_file_file_get(file));
   ecore_file_cp(file,buf);

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_theme_del_cb(E_DBus_Object *obj __UNUSED__,
                       DBusMessage *msg)
{
   DBusMessageIter iter;
   char *file,buf[PATH_MAX];

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &file);

   snprintf(buf, sizeof(buf), "%s/themes/%s", e_user_dir_get(),
            ecore_file_file_get(file));
   ecore_file_unlink(buf);

   return dbus_message_new_method_return(msg);
}

/*
 * Lists User Themes
 */
static DBusMessage *
_e_msgbus_theme_lut_cb(E_DBus_Object *obj __UNUSED__,
                       DBusMessage *msg)
{
   Eina_List *l,*list;
   DBusMessage *reply;
   DBusMessageIter iter;
   DBusMessageIter arr;
   char buf[PATH_MAX], *theme;

   snprintf(buf, sizeof(buf), "%s/themes/", e_user_dir_get());
   list = ecore_file_ls(buf);

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);

   EINA_LIST_FOREACH(list, l, theme)
     {
        snprintf(buf, sizeof(buf), "%s/themes/%s",e_user_dir_get(),theme);
        E_FREE(theme);
        theme = strdup(buf);
	dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &theme);
        E_FREE(theme);
     }
   dbus_message_iter_close_container(&iter, &arr);

   return reply;
}

static DBusMessage *
_e_msgbus_intl_get_cb(E_DBus_Object *obj __UNUSED__,
                      DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   const char *intl;

   intl = (char *) e_intl_language_get();

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &intl);

   return reply;
}

static DBusMessage *
_e_msgbus_intl_set_cb(E_DBus_Object *obj __UNUSED__,
                      DBusMessage *msg)
{
   DBusMessageIter iter;
   const char *intl;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &intl);

   if (e_config->language) eina_stringshare_del(e_config->language);
   e_config->language = NULL;

   if (intl) e_config->language = eina_stringshare_add(intl);
   if ((e_config->language) && (e_config->language[0] != 0))
     e_intl_language_set(intl);

   e_config_save_queue();
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_intl_list_cb(E_DBus_Object *obj __UNUSED__,
                       DBusMessage *msg)
{
   FILE *output;
   char *name = NULL;
   DBusMessage *reply;
   DBusMessageIter iter;
   DBusMessageIter arr;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);

   output = popen("locale -a", "r");
   if (output)
     {
        char line[32];
        while (fscanf(output, "%[^\n]\n", line) == 1)
          {
             E_Locale_Parts *locale_parts;

             locale_parts = e_intl_locale_parts_get(line);
             if (locale_parts)
               {
                  name = strdup(line);
                  dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &name);
               }
             E_FREE(name);
          }
        pclose(output);
     }
   dbus_message_iter_close_container(&iter, &arr);

   return reply;
}

/*
 * This function apply the wallpaper to the background after import
 */
static void
_wallpaper_import_cb(const char *path __UNUSED__, void *data)
{
   E_Import_Config_Dialog *import;
   E_Zone *zone;
   E_Desk *desk;
   E_Container *con;

   import = data;
   con = e_container_current_get(e_manager_current_get());
   zone = e_util_zone_current_get(con->manager);
   desk = e_desk_current_get(zone);

   if (import->place == IMPORT_WALLPAPER_ALL)
     {
        while (e_config->desktop_backgrounds)
          {
             E_Config_Desktop_Background *cfbg;
             cfbg = e_config->desktop_backgrounds->data;
             e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
          }
        e_bg_default_set(import->fdest);
     }
   else if (import->place == IMPORT_WALLPAPER_DESK)
     {
        e_bg_del(con->num, zone->id, desk->x, desk->y);
        e_bg_del(con->num, -1, desk->x, desk->y);
        e_bg_del(-1, zone->num, desk->x, desk->y);
        e_bg_del(-1, -1, desk->x, desk->y);
        e_bg_add(con->num, zone->id, desk->x, desk->y, import->fdest);
     }
   else if (import->place == IMPORT_WALLPAPER_SCREEN)
     {
        Eina_List *l, *fl = NULL;
        for (l = e_config->desktop_backgrounds; l; l = l->next)
          {
             E_Config_Desktop_Background *cfbg;
             cfbg = l->data;
             if ((cfbg->container == (int)zone->container->num) &&
                (cfbg->zone == (int)zone->id))
               fl = eina_list_append(fl, cfbg);
          }
        while (fl)
          {
             E_Config_Desktop_Background *cfbg;
             cfbg = fl->data;
             e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
             fl = eina_list_remove_list(fl, fl);
          }
        e_bg_add(zone->container->num, zone->id, -1, -1, import->fdest);
     }

   e_bg_update();
   e_config_save_queue();
}

/*Wallpaper Handlers*/
static DBusMessage *
_e_msgbus_wallpaper_add_cb(E_DBus_Object *obj __UNUSED__,
                              DBusMessage *msg)
{
   DBusMessageIter iter;
   const char *background;
   E_Container *con;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &background);

   if(ecore_file_exists(background) == EINA_FALSE)
     {
        e_util_dialog_show(_("Picture _e_msgbus_wallpaper_add_cb Error"),
                           _("Enlightenment was unable to import the picture<br>"
                             "File not found."));
        return dbus_message_new_method_return(msg);
     }

   con = e_container_current_get(e_manager_current_get());
   e_import_place_show = IMPORT_PLACE_SHOW;
   e_import_config_dialog_show(con,background,
                              (Ecore_End_Cb)_wallpaper_import_cb, NULL);

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_wallpaper_set_cb(E_DBus_Object *obj __UNUSED__,
                              DBusMessage *msg)
{
   DBusMessageIter iter;
   const char *background;
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &background);
   while (e_config->desktop_backgrounds)
     {
        E_Config_Desktop_Background *cfbg;
        cfbg = e_config->desktop_backgrounds->data;
        e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
     }
   e_bg_default_set(background);
   e_bg_update();
   e_config_save_queue();

   return dbus_message_new_method_return(msg);
}



/*Exec Handlers*/
static DBusMessage *
_e_msgbus_exec_cmd_cb(E_DBus_Object *obj __UNUSED__,
                              DBusMessage *msg)
{
   DBusMessageIter iter;
   const char *cmd;
   E_Exec_Instance *ei;
   E_Zone *zone;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &cmd);

   if (!cmd)
     return dbus_message_new_method_return(msg);

   zone = e_util_zone_current_get(e_container_current_get(e_manager_current_get())->manager);
   ei = e_exec(zone, NULL, cmd, NULL, NULL);
   ei->desk_x = zone->desk_x_current;
   ei->desk_y = zone->desk_y_current;

   if ((!ei) || (!ei->exe))
     {
        ERR("could not execute '%s'", cmd);
     }

   return dbus_message_new_method_return(msg);
}
#define E_MSGBUS_WIN_ACTION_CB_BEGIN(NAME) \
static DBusMessage * \
_e_msgbus_window_##NAME##_cb(E_DBus_Object *obj __UNUSED__, DBusMessage *msg) \
{ \
   E_Border *bd; \
   int xwin;\
   DBusMessageIter iter;\
\
   dbus_message_iter_init(msg, &iter);\
   dbus_message_iter_get_basic(&iter, &xwin);\
   bd = e_border_find_by_client_window(xwin);\
   if (bd)\
     {

#define E_MSGBUS_WIN_ACTION_CB_END \
     }\
\
   return dbus_message_new_method_return(msg);\
}

E_MSGBUS_WIN_ACTION_CB_BEGIN(close)
e_border_act_close_begin(bd);
E_MSGBUS_WIN_ACTION_CB_END

E_MSGBUS_WIN_ACTION_CB_BEGIN(kill)
e_border_act_kill_begin(bd);
E_MSGBUS_WIN_ACTION_CB_END

E_MSGBUS_WIN_ACTION_CB_BEGIN(focus)
e_border_focus_set(bd, 1, 1);
if (!bd->lock_user_stacking)
  {
     if (e_config->border_raise_on_focus)
       e_border_raise(bd);
  }
E_MSGBUS_WIN_ACTION_CB_END

E_MSGBUS_WIN_ACTION_CB_BEGIN(iconify)
e_border_iconify(bd);
E_MSGBUS_WIN_ACTION_CB_END

E_MSGBUS_WIN_ACTION_CB_BEGIN(uniconify)
e_border_uniconify(bd);
E_MSGBUS_WIN_ACTION_CB_END

E_MSGBUS_WIN_ACTION_CB_BEGIN(maximize)
e_border_maximize(bd, e_config->maximize_policy);
E_MSGBUS_WIN_ACTION_CB_END

E_MSGBUS_WIN_ACTION_CB_BEGIN(unmaximize)
e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
E_MSGBUS_WIN_ACTION_CB_END
