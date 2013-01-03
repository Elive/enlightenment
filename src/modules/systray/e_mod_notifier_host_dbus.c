#include "e_mod_notifier_host_private.h"

#define WATCHER_BUS "org.kde.StatusNotifierWatcher"
#define WATCHER_PATH "/StatusNotifierWatcher"
#define WATCHER_IFACE "org.kde.StatusNotifierWatcher"

#define ITEM_IFACE "org.kde.StatusNotifierItem"

#define HOST_REGISTRER "/bla" //TODO check what watcher expect we send to him

extern const char *Category_Name[];
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
notifier_item_find(const char *path, const char *bus_id, Instance_Notifier_Host *host_inst)
{
   Notifier_Item *item;
   EINA_INLIST_FOREACH(host_inst->items_list, item)
     {
        if (item->bus_id == bus_id && item->path == path)
          return item;
     }
   return NULL;
}

static int
id_find(const char *text, const char *array_of_names[], unsigned max)
{
   unsigned i;
   for (i = 0; i < max; i++)
     {
        if (strcmp(text, array_of_names[i]))
          continue;
        return i;
      }
   return 0;
}

static void
item_prop_get(void *data, const void *key, EDBus_Message_Iter *var)
{
   Notifier_Item *item = data;

   if (!strcmp(key, "Category"))
     {
        const char *category;
        edbus_message_iter_arguments_get(var, "s", &category);
        item->category = id_find(category, Category_Name, CATEGORY_LAST);
     }
   else if (!strcmp(key, "IconName"))
     {
        const char *name;
        edbus_message_iter_arguments_get(var, "s", &name);
        eina_stringshare_replace(&item->icon_name, name);
     }
   else if (!strcmp(key, "AttentionIconName"))
     {
        const char *name;
        edbus_message_iter_arguments_get(var, "s", &name);
        eina_stringshare_replace(&item->attention_icon_name, name);
     }
   else if (!strcmp(key, "IconThemePath"))
     {
        const char *path;
        edbus_message_iter_arguments_get(var, "s", &path);
        eina_stringshare_replace(&item->icon_path, path);
     }
   else if (!strcmp(key, "Menu"))
     {
        const char *path;
        edbus_message_iter_arguments_get(var, "o", &path);
        eina_stringshare_replace(&item->menu_path, path);
     }
   else if (!strcmp(key, "Status"))
     {
        const char *status;
        edbus_message_iter_arguments_get(var, "s", &status);
        item->status = id_find(status, Status_Names, STATUS_LAST);
     }
   else if (!strcmp(key, "Id"))
     {
        const char *id;
        edbus_message_iter_arguments_get(var, "s", &id);
        eina_stringshare_replace(&item->id, id);
     }
   else if (!strcmp(key, "Title"))
     {
        const char *title;
        edbus_message_iter_arguments_get(var, "s", &title);
        eina_stringshare_replace(&item->title, title);
     }
}

//debug
static void
render_menu_itens(void *data, E_DBusMenu_Item *new_root_item)
{
   Notifier_Item *item = data;
   E_DBusMenu_Item *child;
   //TODO create evas_object of menu

   printf("icon = %s\n", item->id);
   EINA_SAFETY_ON_FALSE_RETURN(new_root_item->is_submenu);

   EINA_INLIST_FOREACH(new_root_item->sub_items, child)
     {
        if (child->type == E_DBUSMENU_ITEM_TYPE_SEPARATOR)
          printf("\tseparator\n");
        else
          {
             printf("\tLabel= %s | Icon name=%s | Toggle type=%d | Toggle state=%d | Visible = %d | Enabled = %d\n",
                    child->label, child->icon_name, child->toggle_type,
                    child->toggle_state, child->visible, child->enabled);
          }
     }
}

static void
props_changed(void *data, const EDBus_Message *msg)
{
   Notifier_Item *item = data;
   const char *interface, *menu = item->menu_path;
   EDBus_Message_Iter *changed, *invalidate;

   if (!edbus_message_arguments_get(msg, "sa{sv}as", &interface, &changed, &invalidate))
     {
        ERR("Error reading message");
        return;
     }

   edbus_message_iter_dict_iterate(changed, "sv", item_prop_get, item);

   if (menu != item->menu_path)
     {
        EDBus_Connection *conn = edbus_object_connection_get(edbus_proxy_object_get(item->proxy));
        e_dbusmenu_unload(item->menu_data);
        item->menu_data = e_dbusmenu_load(conn, item->bus_id, item->menu_path, item);
     }
}

static void
props_get_all_cb(void *data, const EDBus_Message *msg, EDBus_Pending *pending EINA_UNUSED)
{
   const char *error, *error_name;
   EDBus_Message_Iter *dict;
   Notifier_Item *item = data;
   EDBus_Connection *conn;

   if (edbus_message_error_get(msg, &error, &error_name))
     {
        ERR("%s %s", error, error_name);
        return;
     }

   if (!edbus_message_arguments_get(msg, "a{sv}", &dict))
     {
        ERR("Error getting arguments.");
        return;
     }

   edbus_message_iter_dict_iterate(dict, "sv", item_prop_get, item);

   if (!item->menu_path)
     ERR("Notifier item %s dont have menu path.", item->menu_path);

   conn = edbus_object_connection_get(edbus_proxy_object_get(item->proxy));
   item->menu_data = e_dbusmenu_load(conn, item->bus_id, item->menu_path, item);
   e_dbusmenu_update_cb_set(item->menu_data, render_menu_itens);

   systray_notifier_item_update(item);
}

static Eina_Bool
basic_prop_get(const char *propname, void *data, const EDBus_Message *msg)
{
   EDBus_Message_Iter *var;
   const char *error, *error_msg;

   if (edbus_message_error_get(msg, &error, &error_msg))
     {
        ERR("%s %s", error, error_msg);
        return EINA_FALSE;
     }

   if (!edbus_message_arguments_get(msg, "v", &var))
     {
        ERR("Error reading message.");
        return EINA_FALSE;
     }
   item_prop_get(data, propname, var);
   return EINA_TRUE;
}

static void
attention_icon_get_cb(void *data, const EDBus_Message *msg, EDBus_Pending *pending EINA_UNUSED)
{
   Notifier_Item *item = data;
   const char *propname = "AttentionIconName";
   basic_prop_get(propname, item, msg);
   systray_notifier_item_update(item);
}

static void
new_attention_icon_cb(void *data, const EDBus_Message *msg EINA_UNUSED)
{
   Notifier_Item *item = data;
   edbus_proxy_property_get(item->proxy, "AttentionIconName", attention_icon_get_cb, item);
}

static void
icon_get_cb(void *data, const EDBus_Message *msg, EDBus_Pending *pending EINA_UNUSED)
{
   Notifier_Item *item = data;
   const char *propname = "IconName";
   basic_prop_get(propname, item, msg);
   systray_notifier_item_update(item);
}

static void
new_icon_cb(void *data, const EDBus_Message *msg EINA_UNUSED)
{
   Notifier_Item *item = data;
   edbus_proxy_property_get(item->proxy, "IconName", icon_get_cb, item);
}

static void
title_get_cb(void *data, const EDBus_Message *msg, EDBus_Pending *pending EINA_UNUSED)
{
   Notifier_Item *item = data;
   const char *propname = "Title";
   basic_prop_get(propname, item, msg);
   systray_notifier_item_update(item);
}

static void
new_title_cb(void *data, const EDBus_Message *msg EINA_UNUSED)
{
   Notifier_Item *item = data;
   edbus_proxy_property_get(item->proxy, "Title", title_get_cb, item);
}

static void
new_icon_theme_path_cb(void *data, const EDBus_Message *msg)
{
   Notifier_Item *item = data;
   const char *path;
   if (!edbus_message_arguments_get(msg, "s", &path))
     {
        ERR("Error reading message.");
        return;
     }
   eina_stringshare_replace(&item->icon_path, path);
   systray_notifier_item_update(item);
}

static void
new_status_cb(void *data, const EDBus_Message *msg)
{
   Notifier_Item *item = data;
   const char *status;
   if (!edbus_message_arguments_get(msg, "s", &status))
     {
        ERR("Error reading message.");
        return;
     }
   item->status = id_find(status, Status_Names, STATUS_LAST);
   systray_notifier_item_update(item);
}

static void
notifier_item_add(const char *path, const char *bus_id, Instance_Notifier_Host *host_inst)
{
   EDBus_Proxy *proxy;
   Notifier_Item *item = calloc(1, sizeof(Notifier_Item));
   EDBus_Signal_Handler *s;
   EINA_SAFETY_ON_NULL_RETURN(item);

   item->path = path;
   item->bus_id = bus_id;
   host_inst->items_list = eina_inlist_append(host_inst->items_list,
                                              EINA_INLIST_GET(item));
   item->host_inst = host_inst;

   proxy = edbus_proxy_get(edbus_object_get(host_inst->conn, bus_id, path),
                           ITEM_IFACE);
   item->proxy = proxy;
   edbus_proxy_property_get_all(proxy, props_get_all_cb, item);
   s = edbus_proxy_properties_changed_callback_add(proxy, props_changed, item);
   item->signals = eina_list_append(item->signals, s);
   s = edbus_proxy_signal_handler_add(proxy, "NewAttentionIcon",
                                      new_attention_icon_cb, item);
   item->signals = eina_list_append(item->signals, s);
   s = edbus_proxy_signal_handler_add(proxy, "NewIcon",
                                      new_icon_cb, item);
   item->signals = eina_list_append(item->signals, s);
   s = edbus_proxy_signal_handler_add(proxy, "NewIconThemePath",
                                      new_icon_theme_path_cb, item);
   item->signals = eina_list_append(item->signals, s);
   s = edbus_proxy_signal_handler_add(proxy, "NewStatus", new_status_cb, item);
   item->signals = eina_list_append(item->signals, s);
   s = edbus_proxy_signal_handler_add(proxy, "NewTitle", new_title_cb, item);
   item->signals = eina_list_append(item->signals, s);
}

static void
notifier_item_add_cb(void *data, const EDBus_Message *msg)
{
   const char *item, *bus, *path;
   Instance_Notifier_Host *host_inst = data;

   if (!edbus_message_arguments_get(msg, "s", &item))
     {
        ERR("Error getting arguments from msg.");
        return;
     }
   DBG("add %s", item);
   if (service_string_parse(item, &path, &bus))
     notifier_item_add(path, bus, host_inst);
}

static void
notifier_item_del_cb(void *data, const EDBus_Message *msg)
{
   const char *service, *bus, *path;
   Notifier_Item *item;
   Instance_Notifier_Host *host_inst = data;

   if (!edbus_message_arguments_get(msg, "s", &service))
     {
        ERR("Error getting arguments from msg.");
        return;
     }
   DBG("service %s", service);
   if (!service_string_parse(service, &path, &bus))
     return;
   item = notifier_item_find(path, bus, host_inst);
   if (item)
     systray_notifier_item_free(item);
   eina_stringshare_del(path);
   eina_stringshare_del(bus);
}

static void
notifier_items_get_cb(void *data, const EDBus_Message *msg, EDBus_Pending *pending EINA_UNUSED)
{
   const char *item;
   const char *error, *error_msg;
   EDBus_Message_Iter *array, *variant;
   Instance_Notifier_Host *host_inst = data;

   if (edbus_message_error_get(msg, &error, &error_msg))
     {
        ERR("%s %s", error, error_msg);
        return;
     }

   if (!edbus_message_arguments_get(msg, "v", &variant))
     {
        ERR("Error getting arguments from msg.");
        return;
     }

   if (!edbus_message_iter_arguments_get(variant, "as", &array))
     {
        ERR("Error getting arguments from msg.");
        return;
     }

   while (edbus_message_iter_get_and_next(array, 's', &item))
     {
        const char *bus, *path;
        if (service_string_parse(item, &path, &bus))
          notifier_item_add(path, bus, host_inst);
     }
}

void systray_notifier_dbus_init(Instance_Notifier_Host *host_inst)
{
   EDBus_Object *obj;
   edbus_init();

   host_inst->conn = edbus_connection_get(EDBUS_CONNECTION_TYPE_SESSION);
   obj = edbus_object_get(host_inst->conn, WATCHER_BUS, WATCHER_PATH);
   host_inst->watcher = edbus_proxy_get(obj, WATCHER_IFACE);
   edbus_proxy_call(host_inst->watcher, "RegisterStatusNotifierHost", NULL, NULL, -1, "s",
                    HOST_REGISTRER);
   edbus_proxy_property_get(host_inst->watcher, "RegisteredStatusNotifierItems",
                            notifier_items_get_cb, host_inst);
   edbus_proxy_signal_handler_add(host_inst->watcher, "StatusNotifierItemRegistered",
                                  notifier_item_add_cb, host_inst);
   edbus_proxy_signal_handler_add(host_inst->watcher, "StatusNotifierItemUnregistered",
                                  notifier_item_del_cb, host_inst);
}

void systray_notifier_dbus_shutdown(Instance_Notifier_Host *host_inst)
{
   Eina_Inlist *safe_list;
   Notifier_Item *item;
   EDBus_Object *obj;

   EINA_INLIST_FOREACH_SAFE(host_inst->items_list, safe_list, item)
     systray_notifier_item_free(item);

   obj = edbus_proxy_object_get(host_inst->watcher);
   edbus_proxy_unref(host_inst->watcher);
   edbus_object_unref(obj);
   edbus_connection_unref(host_inst->conn);
   edbus_shutdown();
}