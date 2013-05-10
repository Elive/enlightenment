#include "e.h"

#define BUS "org.enlightenment.Screenshot"
#define PATH "/org/enlightenment/Screenshot"
#define INTERFACE "org.enlightenment.Screenshot"

static EDBus_Message *_e_screenshot_url_get(const EDBus_Service_Interface *iface, const EDBus_Message *msg);
static void on_name_request(void *data, const EDBus_Message *msg, EDBus_Pending *pending);
static Eina_Bool _ecore_timer_cb(void *data);

EDBus_Connection *_e_ss_conn = NULL;

static const EDBus_Signal signals[] = {
       {NULL}
};

static const EDBus_Method methods[] = {
       { "Screenshot_URL_Get", NULL, EDBUS_ARGS({"s", "string"}),
          _e_screenshot_url_get, 0
       },
       {NULL}
};

static const EDBus_Service_Interface_Desc iface_desc = {
     INTERFACE, methods, signals, NULL, NULL, NULL
};

EINTERN int
e_screenshot_edbus_init(void)
{
   EDBus_Service_Interface *iface;

   edbus_init();

   _e_ss_conn = edbus_connection_get(EDBUS_CONNECTION_TYPE_SESSION);

   iface = edbus_service_interface_register(_e_ss_conn, PATH, &iface_desc);
   edbus_name_request(_e_ss_conn, BUS, EDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE,
                      on_name_request, iface);

   return 1;
}

EINTERN int
e_screenshot_edbus_shutdown(void)
{
   if(_e_ss_conn)
     {
        edbus_connection_unref(_e_ss_conn);
     }

   edbus_shutdown();
   _e_ss_conn = NULL;
   return 1;
}


static EDBus_Message *
_e_screenshot_url_get(const EDBus_Service_Interface *iface EINA_UNUSED,
                      const EDBus_Message *msg)
{
   E_Screenshot *_e_ss = NULL;

   _e_ss = e_screenshot_new();
   e_screenshot_desktop_now(_e_ss);
   e_screenshot_quality_set(_e_ss, E_SCREENSHOT_QUALITY_MEDIUM);
   e_screenshot_upload(_e_ss);
   _e_ss->data = edbus_message_method_return_new(msg);

   _e_ss->timer = ecore_timer_loop_add(0.5, _ecore_timer_cb, _e_ss);

   return NULL;
}

static void
on_name_request(void *data EINA_UNUSED, const EDBus_Message *msg,
                EDBus_Pending *pending EINA_UNUSED)
{
   unsigned int reply;

   if (edbus_message_error_get(msg, NULL, NULL))
     {
        ERR("error on on_name_request\n");
        return;
     }

   if (!edbus_message_arguments_get(msg, "u", &reply))
     {
        ERR("error geting arguments on on_name_request\n");
        return;
     }

   if (reply != EDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER)
     {
        ERR("error name already in use\n");
        return;
     }
}

static Eina_Bool
_ecore_timer_cb(void *data)
{
   E_Screenshot *_e_ss = data;
   EDBus_Message *msg = _e_ss->data;

   char buf[PATH_MAX];
   if (!_e_ss->finish) return ECORE_CALLBACK_RENEW;

   snprintf(buf, sizeof(buf), "%s\n", _e_ss->ret_url);

   edbus_message_arguments_append(msg, "s", strdup(buf));
   edbus_connection_send(_e_ss_conn, msg, NULL, NULL, -1);

   ecore_timer_del(_e_ss->timer);
   _e_ss->timer = NULL;

   e_screenshot_free(_e_ss);
   _e_ss = NULL;

   return ECORE_CALLBACK_DONE;
}

