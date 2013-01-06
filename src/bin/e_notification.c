#include "e.h"

typedef struct _Notification_Data
{
   EDBus_Connection *conn;
   EDBus_Service_Interface *iface;
   E_Notification_Notify_Cb notify_cb;
   E_Notification_Close_Cb close_cb;
   void *data;
   const E_Notification_Server_Info *server_info;
} Notification_Data;

static Notification_Data *n_data = NULL;

static void
hints_dict_iter(void *data, const void *key, EDBus_Message_Iter *var)
{
   E_Notification_Notify *n = data;
   if (!strcmp(key, "image-data") || !strcmp(key, "image_data"))
     {
        EDBus_Message_Iter *st, *data_iter;
        int w, h, r, bits, channels;
        Eina_Bool alpha;
        unsigned char *raw_data;
        if (!edbus_message_iter_arguments_get(var, "(iiibiiay)", &st))
          return;
        if (!edbus_message_iter_arguments_get(st, "iiibiiay", &w, &h, &r,
                                              &alpha, &bits, &channels,
                                              &data_iter))
          return;
        edbus_message_iter_fixed_array_get(data_iter, 'y', &raw_data,
                                           &n->icon.raw.data_size);
        n->icon.raw.width = w;
        n->icon.raw.height = h;
        n->icon.raw.has_alpha = alpha;
        n->icon.raw.rowstride = r;
        n->icon.raw.bits_per_sample = bits;
        n->icon.raw.channels = channels;
        n->icon.raw.data = malloc(sizeof(char) * n->icon.raw.data_size);
        EINA_SAFETY_ON_NULL_RETURN(n->icon.raw.data);
        memcpy(n->icon.raw.data, raw_data, sizeof(char)*n->icon.raw.data_size);
     }
   else if (!strcmp(key, "urgency"))
     {
        unsigned char urgency;
        edbus_message_iter_arguments_get(var, "y", &urgency);
        if (urgency < 3)
          n->urgency = urgency;
     }
   else if (!strcmp(key, "image-path") || !strcmp(key, "image_path"))
     {
        edbus_message_iter_arguments_get(var, "s", &n->icon.icon_path);
        n->icon.icon_path = eina_stringshare_add(n->icon.icon_path);
     }
}

static EDBus_Message *
notify_cb(const EDBus_Service_Interface *iface EINA_UNUSED, const EDBus_Message *msg)
{
   E_Notification_Notify *n;
   EDBus_Message_Iter *actions_iter, *hints_iter;
   EDBus_Message *reply;

   if (!n_data->notify_cb)
     return NULL;

   n = calloc(1, sizeof(E_Notification_Notify));
   EINA_SAFETY_ON_NULL_RETURN_VAL(n, NULL);

   if (!edbus_message_arguments_get(msg, "susssasa{sv}i", &n->app_name,
                                    &n->replaces_id, &n->icon.icon, &n->sumary,
                                    &n->body, &actions_iter, &hints_iter,
                                    &n->timeout))
     {
        ERR("Reading message.");
        free(n);
        return NULL;
     }
   edbus_message_iter_dict_iterate(hints_iter, "sv", hints_dict_iter, n);
   n->app_name = eina_stringshare_add(n->app_name);
   n->icon.icon = eina_stringshare_add(n->icon.icon);
   n->sumary = eina_stringshare_add(n->sumary);
   n->body = eina_stringshare_add(n->body);

   n->id = n_data->notify_cb(n_data->data, n);
   reply = edbus_message_method_return_new(msg);
   edbus_message_arguments_append(reply, "u", n->id);
   return reply;
}

static EDBus_Message *
close_notification_cb(const EDBus_Service_Interface *iface EINA_UNUSED, const EDBus_Message *msg)
{
   unsigned id;
   if (!edbus_message_arguments_get(msg, "u", &id))
     return NULL;
   if (n_data->close_cb)
     n_data->close_cb(n_data->data, id);
   return edbus_message_method_return_new(msg);
}

static EDBus_Message *
capabilities_cb(const EDBus_Service_Interface *iface EINA_UNUSED, const EDBus_Message *msg)
{
   EDBus_Message *reply = edbus_message_method_return_new(msg);
   int i;
   EDBus_Message_Iter *main_iter, *array;

   main_iter = edbus_message_iter_get(reply);
   edbus_message_iter_arguments_append(main_iter, "as", &array);

   for (i = 0; n_data->server_info->capabilities[i]; i++)
     edbus_message_iter_arguments_append(array, "s",
                                      n_data->server_info->capabilities[i]);
   edbus_message_iter_container_close(main_iter, array);
   return reply;
}

static EDBus_Message *
server_info_cb(const EDBus_Service_Interface *iface EINA_UNUSED, const EDBus_Message *msg)
{
   EDBus_Message *reply = edbus_message_method_return_new(msg);
   edbus_message_arguments_append(reply, "ssss", n_data->server_info->name,
                                  n_data->server_info->vendor,
                                  n_data->server_info->version,
                                  n_data->server_info->spec_version);
   return reply;
}

static const EDBus_Method methods[] = {
   { "Notify",
     EDBUS_ARGS({"s", "app_name"}, {"u", "replaces_id"}, {"s", "app_icon"}, {"s", "summary"}, {"s", "body"}, {"as", "actions"}, {"a{sv}", "hints"}, {"i", "expire_timeout"}),
     EDBUS_ARGS({"u", "id"}), notify_cb },
   { "CloseNotification", EDBUS_ARGS({"u", "id"}), NULL, close_notification_cb },
   { "GetCapabilities", NULL, EDBUS_ARGS({"as", "capabilities"}),
      capabilities_cb },
   { "GetServerInformation", NULL,
      EDBUS_ARGS({"s", "name"}, {"s", "vendor"}, {"s", "version"}, {"s", "spec_version"}),
      server_info_cb },
   { }
};

enum
{
   SIGNAL_NOTIFICATION_CLOSED = 0,
   SIGNAL_ACTION_INVOKED,
};

static const EDBus_Signal signals[] = {
   [SIGNAL_NOTIFICATION_CLOSED] =
     { "NotificationClosed", EDBUS_ARGS({"u", "id"}, {"u", "reason"}) },
   [SIGNAL_ACTION_INVOKED] =
     { "ActionInvoked", EDBUS_ARGS({"u", "id"}, {"s", "action_key"}) },
   { }
};

#define PATH      "/org/freedesktop/Notifications"
#define BUS       "org.freedesktop.Notifications"
#define INTERFACE "org.freedesktop.Notifications"

static const EDBus_Service_Interface_Desc desc = {
   INTERFACE, methods, signals, NULL, NULL, NULL
};

EAPI Eina_Bool
e_notification_server_register(const E_Notification_Server_Info *server_info, E_Notification_Notify_Cb n_cb, E_Notification_Close_Cb close_cb, const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(server_info, EINA_FALSE);
   if (n_data)
     return EINA_FALSE;
   n_data = calloc(1, sizeof(Notification_Data));
   EINA_SAFETY_ON_NULL_RETURN_VAL(n_data, EINA_FALSE);

   edbus_init();
   n_data->conn = edbus_connection_get(EDBUS_CONNECTION_TYPE_SESSION);
   n_data->iface = edbus_service_interface_register(n_data->conn, PATH, &desc);
   n_data->notify_cb = n_cb;
   n_data->close_cb = close_cb;
   n_data->data = (void *) data;
   n_data->server_info = server_info;
   edbus_name_request(n_data->conn, BUS,
                      EDBUS_NAME_REQUEST_FLAG_REPLACE_EXISTING, NULL, NULL);

   return EINA_TRUE;
}

EAPI void
e_notification_server_unregister(void)
{
   EINA_SAFETY_ON_NULL_RETURN(n_data);
   edbus_service_interface_unregister(n_data->iface);
   edbus_connection_unref(n_data->conn);
   edbus_shutdown();
   free(n_data);
   n_data = NULL;
}

EAPI void
e_notification_notify_free(E_Notification_Notify *notify)
{
   EINA_SAFETY_ON_NULL_RETURN(notify);
   eina_stringshare_del(notify->app_name);
   eina_stringshare_del(notify->body);
   eina_stringshare_del(notify->icon.icon);
   if (notify->icon.icon_path)
     eina_stringshare_del(notify->icon.icon_path);
   eina_stringshare_del(notify->sumary);
   if (notify->icon.raw.data)
     free(notify->icon.raw.data);
   free(notify);
}

EAPI void
e_notification_notify_close(E_Notification_Notify *notify, E_Notification_Notify_Closed_Reason reason)
{
   EINA_SAFETY_ON_NULL_RETURN(n_data);
   EINA_SAFETY_ON_NULL_RETURN(notify);
   EINA_SAFETY_ON_FALSE_RETURN(reason <= E_NOTIFICATION_NOTIFY_CLOSED_REASON_UNDEFINED);
   edbus_service_signal_emit(n_data->iface, SIGNAL_NOTIFICATION_CLOSED,
                             notify->id, reason);
}

EAPI Evas_Object *
e_notification_notify_raw_image_get(E_Notification_Notify *notify, Evas *evas)
{
   Evas_Object *o;
   unsigned char *imgdata;

   EINA_SAFETY_ON_NULL_RETURN_VAL(notify, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(evas, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(notify->icon.raw.data, NULL);

   o = evas_object_image_filled_add(evas);
   evas_object_resize(o, notify->icon.raw.width, notify->icon.raw.height);
   evas_object_image_colorspace_set(o, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(o, notify->icon.raw.has_alpha);
   evas_object_image_size_set(o, notify->icon.raw.width, notify->icon.raw.height);

   imgdata = evas_object_image_data_get(o, EINA_TRUE);
   EINA_SAFETY_ON_NULL_GOTO(imgdata, error);

   if (notify->icon.raw.bits_per_sample == 8)
     {
        /* Although not specified.
         * The data are very likely to come from a GdkPixbuf
         * which align each row on a 4-bytes boundary when using RGB.
         * And is RGBA otherwise. */
        int x, y, *d;
        unsigned char *s;
        int rowstride = evas_object_image_stride_get(o);

        for (y = 0; y < notify->icon.raw.height; y++)
          {
             s = notify->icon.raw.data + (y * notify->icon.raw.rowstride);
             d = (int *)(imgdata + (y * rowstride));

             for (x = 0; x < notify->icon.raw.width;
                  x++, s += notify->icon.raw.channels, d++)
               {
                  if (notify->icon.raw.has_alpha)
                    *d = (s[3] << 24) | (s[0] << 16) | (s[1] << 8) | (s[2]);
                  else
                    *d = (0xff << 24) | (s[0] << 16) | (s[1] << 8) | (s[2]);
               }
          }
     }
   evas_object_image_data_update_add(o, 0, 0, notify->icon.raw.width,
                                     notify->icon.raw.height);
   evas_object_image_data_set(o, imgdata);

   return o;
error:
   evas_object_del(o);
   return NULL;
}

/* client API */

static void
client_notify_cb(void *data, const EDBus_Message *msg, EDBus_Pending *pending)
{
   unsigned id = 0;
   E_Notification_Client_Send_Cb cb = edbus_pending_data_del(pending, "cb");
   EDBus_Connection *conn = edbus_pending_data_del(pending, "conn");
   if (edbus_message_error_get(msg, NULL, NULL))
     goto end;
   if (!edbus_message_arguments_get(msg, "u", &id))
     goto end;
end:
   cb(data, id);
   edbus_connection_unref(conn);
   edbus_shutdown();
}

static Eina_Bool
notification_client_dbus_send(E_Notification_Notify *notify, E_Notification_Client_Send_Cb cb, const void *data)
{
   EDBus_Connection *conn;
   EDBus_Message *msg;
   EDBus_Message_Iter *main_iter, *actions, *hints;
   EDBus_Message_Iter *entry, *var;
   EDBus_Pending *p;
   EINA_SAFETY_ON_NULL_RETURN_VAL(notify, EINA_FALSE);
   edbus_init();
   conn = edbus_connection_get(EDBUS_CONNECTION_TYPE_SESSION);
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, EINA_FALSE);

   msg = edbus_message_method_call_new(BUS, PATH, INTERFACE, "Notify");
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, EINA_FALSE);

   //build message
   main_iter = edbus_message_iter_get(msg);
   if (!edbus_message_iter_arguments_append(main_iter, "susssas",
                                            notify->app_name ?: "",
                                            notify->replaces_id,
                                            notify->icon.icon ?: "",
                                            notify->sumary ?: "",
                                            notify->body ?: "",
                                            &actions))
     goto error;
   edbus_message_iter_container_close(main_iter, actions);
   if (!edbus_message_iter_arguments_append(main_iter, "a{sv}", &hints))
     goto error;

   if (notify->icon.raw.data)
     {
        EDBus_Message_Iter *st, *data_iter;
        int i;
        edbus_message_iter_arguments_append(hints, "{sv}", &entry);
        edbus_message_iter_arguments_append(entry, "s", "image-data");
        var = edbus_message_iter_container_new(entry, 'v', "(iiibiiay)");
        edbus_message_iter_arguments_append(var, "(iiibiiay)", &st);
        edbus_message_iter_arguments_append(st, "iiibiiay",
                                            notify->icon.raw.width,
                                            notify->icon.raw.height,
                                            notify->icon.raw.rowstride,
                                            notify->icon.raw.has_alpha,
                                            notify->icon.raw.bits_per_sample,
                                            notify->icon.raw.channels,
                                            &data_iter);
        for (i = 0; i < notify->icon.raw.data_size; i++)
          edbus_message_iter_basic_append(data_iter, 'y', notify->icon.raw.data[i]);
        edbus_message_iter_container_close(st, data_iter);
        edbus_message_iter_container_close(var, st);
        edbus_message_iter_container_close(entry, var);
        edbus_message_iter_container_close(hints, entry);
     }
   if (notify->icon.icon_path)
     {
        edbus_message_iter_arguments_append(hints, "{sv}", &entry);
        edbus_message_iter_arguments_append(entry, "s", "image-path");
        var = edbus_message_iter_container_new(entry, 'v', "s");
        edbus_message_iter_arguments_append(var, "s", notify->icon.icon_path);
        edbus_message_iter_container_close(entry, var);
        edbus_message_iter_container_close(hints, entry);
     }

   edbus_message_iter_arguments_append(hints, "{sv}", &entry);
   edbus_message_iter_arguments_append(entry, "s", "urgency");
   var = edbus_message_iter_container_new(entry, 'v', "y");
   edbus_message_iter_arguments_append(var, "y", notify->urgency);
   edbus_message_iter_container_close(entry, var);
   edbus_message_iter_container_close(hints, entry);

   edbus_message_iter_container_close(main_iter, hints);

   edbus_message_iter_arguments_append(main_iter, "i", notify->timeout);

   p = edbus_connection_send(conn, msg, client_notify_cb, data, 5000);
   EINA_SAFETY_ON_NULL_GOTO(p, error);
   edbus_pending_data_set(p, "cb", cb);
   edbus_pending_data_set(p, "conn", conn);
   edbus_message_unref(msg);
   return EINA_TRUE;
error:
   edbus_message_unref(msg);
   return EINA_FALSE;
}

static void
normalize_notify(E_Notification_Notify *notify)
{
   if (!notify->timeout)
     notify->timeout = -1;
}

EAPI Eina_Bool
e_notification_client_send(E_Notification_Notify *notify, E_Notification_Client_Send_Cb cb, const void *data)
{
   unsigned id;
   E_Notification_Notify *copy;

   EINA_SAFETY_ON_NULL_RETURN_VAL(notify, EINA_FALSE);
   normalize_notify(notify);

   if (!n_data)
     return notification_client_dbus_send(notify, cb, data);

   //local
   copy = malloc(sizeof(E_Notification_Notify));
   EINA_SAFETY_ON_NULL_RETURN_VAL(copy, EINA_FALSE);
   memcpy(copy, notify, sizeof(E_Notification_Notify));

   copy->app_name = eina_stringshare_add(notify->app_name);
   copy->body = eina_stringshare_add(notify->body);
   copy->sumary = eina_stringshare_add(notify->sumary);
   copy->icon.icon = eina_stringshare_add(notify->icon.icon);
   if (notify->icon.icon_path)
     copy->icon.icon_path = eina_stringshare_add(notify->icon.icon_path);

   id = n_data->notify_cb(n_data->data, copy);
   if (cb)
     cb((void *) data, id);
   return EINA_TRUE;
}