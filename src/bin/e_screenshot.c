#include "e.h"

static void _e_screenshot_filename_set(E_Screenshot *e_ss);
static Eina_Bool _e_screenshot_upload_data_cb(void *data, int ev_type __UNUSED__, void *event);
static Eina_Bool _e_screenshot_upload_complete_cb(void *data, int ev_type __UNUSED__, void *event);

EAPI E_Screenshot *
e_screenshot_new(void)
{
   time_t tt;
   E_Zone *zone;
   E_Manager *man;
   E_Container *con;
   E_Screenshot *e_ss;
   Ecore_X_Window_Attributes watt;

   e_ss = E_NEW(E_Screenshot, 1);
   e_ss->filename = NULL;

   watt.visual = 0;
   man = e_manager_current_get();
   con = e_container_current_get(man);
   zone = e_zone_current_get(con);
   e_ss->xwin = man->root;
   e_ss->evas = zone->black_evas;
   e_ss->w = zone->w;
   e_ss->h = zone->h;
   e_ss->x = 0;
   e_ss->y = 0;
   e_ss->quality = E_SCREENSHOT_QUALITY_HIGH;
   e_ss->url = ecore_con_url_new("http://www.enlightenment.org/shot.php");
   ecore_con_url_http_version_set(e_ss->url, ECORE_CON_URL_HTTP_VERSION_1_0);
   e_ss->handlers = NULL;

   if (!ecore_x_window_attributes_get(e_ss->xwin, &watt))
     return NULL;

   e_ss->visual = watt.visual;
   time(&tt);
   e_ss->tm = localtime(&tt);

   _e_screenshot_filename_set(e_ss);
   return e_ss;
}

EAPI void
e_screenshot_desktop_now(E_Screenshot *e_ss)
{
   Ecore_X_Screen *scr;
   Ecore_X_Display *dpy;
   Ecore_X_Image *img;
   Ecore_X_Colormap colormap;
   unsigned char *src;
   unsigned int *dst;
   int bpl = 0, rows = 0, bpp = 0;

   scr = ecore_x_default_screen_get();
   dpy = ecore_x_display_get();
   dst = malloc(e_ss->w * e_ss->h * sizeof(int));

   img = ecore_x_image_new(e_ss->w, e_ss->h, e_ss->visual,
                                       ecore_x_window_depth_get(e_ss->xwin));
   ecore_x_image_get(img, e_ss->xwin, e_ss->x, e_ss->y, 0, 0,
                     e_ss->w, e_ss->h);
   src = ecore_x_image_data_get(img, &bpl, &rows, &bpp);
   colormap = ecore_x_default_colormap_get(dpy, scr);
   ecore_x_image_to_argb_convert(src, bpp, bpl, colormap, e_ss->visual,
                                 0, 0, e_ss->w, e_ss->h, dst,
                                 (e_ss->w * sizeof(int)), 0, 0);

   e_ss->screenshot = evas_object_image_filled_add(e_ss->evas);
   evas_object_image_colorspace_set(e_ss->screenshot, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(e_ss->screenshot, EINA_FALSE);
   evas_object_image_size_set(e_ss->screenshot, e_ss->w, e_ss->h);
   evas_object_image_data_copy_set(e_ss->screenshot, dst);
   evas_object_image_data_update_add(e_ss->screenshot, 0, 0, e_ss->w, e_ss->h);
   free(dst);
   ecore_x_image_free(img);
}

EAPI void
e_screenshot_quality_set(E_Screenshot *e_ss, int quality)
{
   if (!e_ss) return;
   e_ss->quality = quality;
   _e_screenshot_filename_set(e_ss);
}

EAPI int
e_screenshot_upload(E_Screenshot *e_ss)
{
   FILE *f;
   int i, fd = -1, fsize = 0;
   char buf[PATH_MAX];
   unsigned char *fdata = NULL;

   snprintf(buf, sizeof(buf), "/tmp/%s", e_ss->filename);

   for (i = 0; i < 10240; i++)
     {
        fd = open(buf, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if (fd >= 0) break;
     }

   if (fd < 0)
     {
        ERR("unable to create file");
        return -1;
     }

   if(!evas_object_image_save(e_ss->screenshot, buf, NULL, NULL))
     {
        ERR("Unable to save screenshot");
     }

   f = fdopen(fd, "rb");

   if (!f)
     {
        ERR("Unable to open file");
        return -1;
     }

   fseek(f, 0, SEEK_END);
   fsize = ftell(f);
   if (fsize < 1)
     {
        ERR("Bad filesize");
        return -1;
     }
   rewind(f);
   fdata = malloc(fsize);
   if (!fdata)
     {
        ERR("Cant allocate memory");
        fclose(f);
        return -1;
     }
   if (fread(fdata, fsize, 1, f) != 1)
     {
        ERR("Cant read picture");
        E_FREE(fdata);
        fclose(f);
        return -1;
     }
   fclose(f);
   //TODO: unlink file!
   E_LIST_HANDLER_APPEND(e_ss->handlers, ECORE_CON_EVENT_URL_DATA,
                         _e_screenshot_upload_data_cb, e_ss);
   E_LIST_HANDLER_APPEND(e_ss->handlers, ECORE_CON_EVENT_URL_COMPLETE,
                         _e_screenshot_upload_complete_cb, e_ss);
   ecore_con_url_post(e_ss->url, fdata, fsize, "application/x-e-shot");

   return 0;
}

static Eina_Bool
_e_screenshot_upload_data_cb(void *data, int ev_type __UNUSED__, void *event)
{
   Ecore_Con_Event_Url_Data *ev = event;
   E_Screenshot *e_ss = data;

   if (ev->url_con != e_ss->url) return EINA_TRUE;
   if (ev->size < 1024)
     {
        char *txt = alloca(ev->size + 1);

        memcpy(txt, ev->data, ev->size);
        txt[ev->size] = 0;

        if (!e_ss->ret_url)
          e_ss->ret_url = eina_stringshare_add(txt);
        else
          eina_stringshare_replace(&e_ss->ret_url, txt);
     }

   return EINA_FALSE;
}

static Eina_Bool
_e_screenshot_upload_complete_cb(void *data, int ev_type __UNUSED__, void *event)
{
   Ecore_Con_Event_Url_Complete *ev = event;
   E_Screenshot *e_ss = data;

   if (ev->url_con != e_ss->url) return EINA_TRUE;
   if (ev->status != 200)
     {
        if (!e_ss->ret_url)
          e_ss->ret_url = eina_stringshare_add("");
        else
          eina_stringshare_replace(&e_ss->ret_url, "");
     }
   e_ss->finish = EINA_TRUE;
   return EINA_FALSE;
}

static void
_e_screenshot_filename_set(E_Screenshot *e_ss)
{
   char buf[PATH_MAX], opts[PATH_MAX];

   if (e_ss->quality == E_SCREENSHOT_QUALITY_PERFECT)
     {
        strftime(buf, sizeof(buf), "shot-%Y-%m-%d_%H-%M-%S.png", e_ss->tm);
        snprintf(opts, sizeof(opts), "compress=%i", 9);
     }
   else
     {
        strftime(buf, sizeof(buf), "shot-%Y-%m-%d_%H-%M-%S.jpg", e_ss->tm);
        snprintf(opts, sizeof(opts), "quality=%i", e_ss->quality);
     }

   if (!e_ss->filename)
     e_ss->filename = eina_stringshare_add(buf);
   else
     eina_stringshare_replace(&e_ss->filename, buf);

   if (!e_ss->options)
     e_ss->options = eina_stringshare_add(opts);
   else
     eina_stringshare_replace(&e_ss->options, opts);
}

EAPI void
e_screenshot_free(E_Screenshot *e_ss)
{
   if (e_ss->filename) eina_stringshare_del(e_ss->filename);
   if (e_ss->options) eina_stringshare_del(e_ss->options);
   if (e_ss->ret_url) eina_stringshare_del(e_ss->ret_url);
   if (e_ss->screenshot) evas_object_del(e_ss->screenshot);
   if (e_ss->url) ecore_con_url_free(e_ss->url);
   if (e_ss->handlers) E_FREE_LIST(e_ss->handlers, ecore_event_handler_del);
   E_FREE(e_ss);
}

