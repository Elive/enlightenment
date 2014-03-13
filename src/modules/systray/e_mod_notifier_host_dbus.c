#include "e_mod_notifier_host_private.h"

#define WATCHER_BUS "org.kde.StatusNotifierWatcher"
#define WATCHER_PATH "/StatusNotifierWatcher"
#define WATCHER_IFACE "org.kde.StatusNotifierWatcher"

#define ITEM_IFACE "org.kde.StatusNotifierItem"

#define HOST_REGISTRER "/bla" //TODO check what watcher expect we send to him

#undef ERR
#define ERR(...) fprintf(stderr, __VA_ARGS__)

extern const char *Category_Names[];
extern const char *Status_Names[];

typedef struct _Notifier_Host_Data {
   Instance_Notifier_Host *host_inst;
   void *data;
} Notifier_Host_Data;

static Eina_Bool
service_string_parse(const char *item, const char **path, const char **bus_id)
{
   unsigned i;
   for (i = 0; i < strlen(item); i++)
     {
        if (item[i] != '/')
          continue;
        *path = eina_stringshare_add(item+i);
        *bus_id = eina_stringshare_nprintf(i+1, "%s", item);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

static Notifier_Item *
notifier_item_find(const char *path, const char *bus_id, Context_Notifier_Host *ctx)
{
   Notifier_Item *item;
   EINA_INLIST_FOREACH(ctx->item_list, item)
     {
        if (item->bus_id == bus_id && item->path == path)
          return item;
     }
   return NULL;
}

static int
id_find(const char *s, const char *names[])
{
   unsigned i;

   for (i = 0; names[i]; i++)
     {
        if (!strcmp(s, names[i]))
          return i;
     }
   return 0;
}

static void
item_prop_get(void *data, const void *key, Eldbus_Message_Iter *var)
{
   Notifier_Item *item = data;

   if (!strcmp(key, "Category"))
     {
        const char *category;
        eldbus_message_iter_arguments_get(var, "s", &category);
        item->category = id_find(category, Category_Names);
     }
   else if (!strcmp(key, "IconName"))
     {
        const char *name;
        eldbus_message_iter_arguments_get(var, "s", &name);
        eina_stringshare_replace(&item->icon_name, name);
     }
   else if (!strcmp(key, "AttentionIconName"))
     {
        const char *name;
        eldbus_message_iter_arguments_get(var, "s", &name);
        eina_stringshare_replace(&item->attention_icon_name, name);
     }
   else if (!strcmp(key, "IconThemePath"))
     {
        const char *path;
        eldbus_message_iter_arguments_get(var, "s", &path);
        eina_stringshare_replace(&item->icon_path, path);
     }
   else if (!strcmp(key, "Menu"))
     {
        const char *path;
        eldbus_message_iter_arguments_get(var, "o", &path);
        eina_stringshare_replace(&item->menu_path, path);
     }
   else if (!strcmp(key, "Status"))
     {
        const char *status;
        eldbus_message_iter_arguments_get(var, "s", &status);
        item->status = id_find(status, Status_Names);
     }
   else if (!strcmp(key, "Id"))
     {
        const char *id;
        eldbus_message_iter_arguments_get(var, "s", &id);
        eina_stringshare_replace(&item->id, id);
     }
   else if (!strcmp(key, "Title"))
     {
        const char *title;
        eldbus_message_iter_arguments_get(var, "s", &title);
        eina_stringshare_replace(&item->title, title);
     }
}

static void
props_changed(void *data, const Eldbus_Message *msg)
{
   Notifier_Item *item = data;
   const char *interface, *menu = item->menu_path;
   Eldbus_Message_Iter *changed, *invalidate;

   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &interface, &changed, &invalidate))
     {
        ERR("Error reading message");
        return;
     }

   eldbus_message_iter_dict_iterate(changed, "sv", item_prop_get, item);

   if (menu != item->menu_path)
     {
        Eldbus_Connection *conn = eldbus_object_connection_get(eldbus_proxy_object_get(item->proxy));
        item->dbus_item = NULL;
        e_dbusmenu_unload(item->menu_data);
        item->menu_data = e_dbusmenu_load(conn, item->bus_id, item->menu_path,
                                          item);
        e_dbusmenu_update_cb_set(item->menu_data, systray_notifier_update_menu);
     }
}

static void
props_get_all_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   const char *error, *error_name;
   Eldbus_Message_Iter *dict;
   Notifier_Item *item = data;
   Eldbus_Connection *conn;

   if (eldbus_message_error_get(msg, &error, &error_name))
     {
        ERR("%s %s", error, error_name);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "a{sv}", &dict))
     {
        ERR("Error getting arguments.");
        return;
     }

   eldbus_message_iter_dict_iterate(dict, "sv", item_prop_get, item);

   if (!item->menu_path)
     ERR("Notifier item %s dont have menu path.", item->menu_path);

   conn = eldbus_object_connection_get(eldbus_proxy_object_get(item->proxy));
   item->menu_data = e_dbusmenu_load(conn, item->bus_id, item->menu_path, item);
   e_dbusmenu_update_cb_set(item->menu_data, systray_notifier_update_menu);

   systray_notifier_item_update(item);
}

static Eina_Bool
basic_prop_get(const char *propname, void *data, const Eldbus_Message *msg)
{
   Eldbus_Message_Iter *var;
   const char *error, *error_msg;

   if (eldbus_message_error_get(msg, &error, &error_msg))
     {
        ERR("%s %s", error, error_msg);
        return EINA_FALSE;
     }

   if (!eldbus_message_arguments_get(msg, "v", &var))
     {
        ERR("Error reading message.");
        return EINA_FALSE;
     }
   item_prop_get(data, propname, var);
   return EINA_TRUE;
}

static void
attention_icon_get_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   Notifier_Item *item = data;
   const char *propname = "AttentionIconName";
   basic_prop_get(propname, item, msg);
   systray_notifier_item_update(item);
}

static void
new_attention_icon_cb(void *data, const Eldbus_Message *msg EINA_UNUSED)
{
   Notifier_Item *item = data;
   eldbus_proxy_property_get(item->proxy, "AttentionIconName", attention_icon_get_cb, item);
}

static void
icon_get_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   Notifier_Item *item = data;
   const char *propname = "IconName";
   basic_prop_get(propname, item, msg);
   systray_notifier_item_update(item);
}

static void
new_icon_cb(void *data, const Eldbus_Message *msg EINA_UNUSED)
{
   Notifier_Item *item = data;
   eldbus_proxy_property_get(item->proxy, "IconName", icon_get_cb, item);
}

static void
title_get_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   Notifier_Item *item = data;
   const char *propname = "Title";
   basic_prop_get(propname, item, msg);
   systray_notifier_item_update(item);
}

static void
new_title_cb(void *data, const Eldbus_Message *msg EINA_UNUSED)
{
   Notifier_Item *item = data;
   eldbus_proxy_property_get(item->proxy, "Title", title_get_cb, item);
}

static void
new_icon_theme_path_cb(void *data, const Eldbus_Message *msg)
{
   Notifier_Item *item = data;
   const char *path;
   if (!eldbus_message_arguments_get(msg, "s", &path))
     {
        ERR("Error reading message.");
        return;
     }
   eina_stringshare_replace(&item->icon_path, path);
   systray_notifier_item_update(item);
}

static void
new_status_cb(void *data, const Eldbus_Message *msg)
{
   Notifier_Item *item = data;
   const char *status;
   if (!eldbus_message_arguments_get(msg, "s", &status))
     {
        ERR("Error reading message.");
        return;
     }
   item->status = id_find(status, Status_Names);
   systray_notifier_item_update(item);
}

static void
notifier_item_add(const char *path, const char *bus_id, Context_Notifier_Host *ctx)
{
   Eldbus_Proxy *proxy;
   Notifier_Item *item = calloc(1, sizeof(Notifier_Item));
   Eldbus_Signal_Handler *s;
   EINA_SAFETY_ON_NULL_RETURN(item);

   item->path = path;
   item->bus_id = bus_id;
   ctx->item_list = eina_inlist_append(ctx->item_list,
                                        EINA_INLIST_GET(item));

   proxy = eldbus_proxy_get(eldbus_object_get(ctx->conn, bus_id, path),
                           ITEM_IFACE);
   item->proxy = proxy;
   eldbus_proxy_property_get_all(proxy, props_get_all_cb, item);
   s = eldbus_proxy_properties_changed_callback_add(proxy, props_changed, item);
   item->signals = eina_list_append(item->signals, s);
   s = eldbus_proxy_signal_handler_add(proxy, "NewAttentionIcon",
                                      new_attention_icon_cb, item);
   item->signals = eina_list_append(item->signals, s);
   s = eldbus_proxy_signal_handler_add(proxy, "NewIcon",
                                      new_icon_cb, item);
   item->signals = eina_list_append(item->signals, s);
   s = eldbus_proxy_signal_handler_add(proxy, "NewIconThemePath",
                                      new_icon_theme_path_cb, item);
   item->signals = eina_list_append(item->signals, s);
   s = eldbus_proxy_signal_handler_add(proxy, "NewStatus", new_status_cb, item);
   item->signals = eina_list_append(item->signals, s);
   s = eldbus_proxy_signal_handler_add(proxy, "NewTitle", new_title_cb, item);
   item->signals = eina_list_append(item->signals, s);
}

static void
notifier_item_add_cb(void *data, const Eldbus_Message *msg)
{
   const char *item, *bus, *path;
   Context_Notifier_Host *ctx = data;

   if (!eldbus_message_arguments_get(msg, "s", &item))
     {
        ERR("Error getting arguments from msg.");
        return;
     }
   DBG("add %s", item);
   if (service_string_parse(item, &path, &bus))
     notifier_item_add(path, bus, ctx);
}

static void
notifier_item_del_cb(void *data, const Eldbus_Message *msg)
{
   const char *service, *bus, *path;
   Notifier_Item *item;
   Context_Notifier_Host *ctx = data;

   if (!eldbus_message_arguments_get(msg, "s", &service))
     {
        ERR("Error getting arguments from msg.");
        return;
     }
   DBG("service %s", service);
   if (!service_string_parse(service, &path, &bus))
     return;
   item = notifier_item_find(path, bus, ctx);
   if (item)
     systray_notifier_item_free(item);
   eina_stringshare_del(path);
   eina_stringshare_del(bus);
}

static void
notifier_items_get_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   const char *item;
   const char *error, *error_msg;
   Eldbus_Message_Iter *array, *variant;
   Context_Notifier_Host *ctx = data;

   if (eldbus_message_error_get(msg, &error, &error_msg))
     {
        ERR("%s %s", error, error_msg);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "v", &variant))
     {
        ERR("Error getting arguments from msg.");
        return;
     }

   if (!eldbus_message_iter_arguments_get(variant, "as", &array))
     {
        ERR("Error getting arguments from msg.");
        return;
     }

   while (eldbus_message_iter_get_and_next(array, 's', &item))
     {
        const char *bus, *path;
        if (service_string_parse(item, &path, &bus))
          notifier_item_add(path, bus, ctx);
     }
}

static void
item_registered_local_cb(void *data, const char *service)
{
   const char *bus, *path;
   Context_Notifier_Host *ctx = data;
   if (service_string_parse(service, &path, &bus))
     notifier_item_add(path, bus, ctx);
}

static void
item_unregistered_local_cb(void *data, const char *service)
{
   const char *bus, *path;
   Context_Notifier_Host *ctx = data;
   Notifier_Item *item;

   if (!service_string_parse(service, &path, &bus))
     return;
   item = notifier_item_find(path, bus, ctx);
   if (item)
     systray_notifier_item_free(item);
   eina_stringshare_del(path);
   eina_stringshare_del(bus);
}

static void
name_request_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   const char *error, *error_msg;
   unsigned flag;
   Eldbus_Object *obj;
   Context_Notifier_Host *ctx = data;

   ctx->pending = eina_list_remove(ctx->pending, pending);
   if (eldbus_message_error_get(msg, &error, &error_msg))
     {
        ERR("%s %s", error, error_msg);
        goto end;
     }

   if (!eldbus_message_arguments_get(msg, "u", &flag))
     {
        ERR("Error reading message.");
        goto end;
     }

   if (flag == ELDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER)
     {
        systray_notifier_dbus_watcher_start(ctx->conn,
                                            item_registered_local_cb,
                                            item_unregistered_local_cb, ctx);
        return;
     }
end:
   WRN("Bus name: %s already in use, getting data via dbus.\n", WATCHER_BUS);
   obj = eldbus_object_get(ctx->conn, WATCHER_BUS, WATCHER_PATH);
   ctx->watcher = eldbus_proxy_get(obj, WATCHER_IFACE);
   eldbus_proxy_call(ctx->watcher, "RegisterStatusNotifierHost", NULL, NULL, -1, "s",
                    HOST_REGISTRER);
   eldbus_proxy_property_get(ctx->watcher, "RegisteredStatusNotifierItems",
                            notifier_items_get_cb, ctx);
   eldbus_proxy_signal_handler_add(ctx->watcher, "StatusNotifierItemRegistered",
                                  notifier_item_add_cb, ctx);
   eldbus_proxy_signal_handler_add(ctx->watcher, "StatusNotifierItemUnregistered",
                                  notifier_item_del_cb, ctx);
}

void
systray_notifier_dbus_init(Context_Notifier_Host *ctx)
{
   Eldbus_Pending *p;
   
   eldbus_init();
   ctx->conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   if (!ctx->conn) return;
   p = eldbus_name_request(ctx->conn,
                          WATCHER_BUS, ELDBUS_NAME_REQUEST_FLAG_REPLACE_EXISTING,
                          name_request_cb, ctx);
   if (p) ctx->pending = eina_list_append(ctx->pending, p);
}

void systray_notifier_dbus_shutdown(Context_Notifier_Host *ctx)
{
   Eina_Inlist *safe_list;
   Notifier_Item *item;

   ERR("systray_notifier_dbus_shutdown");

   EINA_INLIST_FOREACH_SAFE(ctx->item_list, safe_list, item)
     systray_notifier_item_free(item);

   if (!ctx->watcher)
     systray_notifier_dbus_watcher_stop();
   else
     {
        Eldbus_Object *obj;
        obj = eldbus_proxy_object_get(ctx->watcher);
        eldbus_proxy_unref(ctx->watcher);
        eldbus_object_unref(obj);
        ctx->watcher = NULL;
     }
   eldbus_connection_unref(ctx->conn);
   eldbus_shutdown();
}
