#ifdef E_TYPEDEFS

typedef enum _E_Screenshot_Quality
{
   E_SCREENSHOT_QUALITY_LOW = 25,
   E_SCREENSHOT_QUALITY_MEDIUM = 50,
   E_SCREENSHOT_QUALITY_HIGH = 85,
   E_SCREENSHOT_QUALITY_PERFECT = 100
}E_Screenshot_Quality;

typedef struct _E_Screenshot E_Screenshot;

#else
#ifndef E_SCREENSHOT_H
#define E_SCREENSHOT_H

struct _E_Screenshot
{
   const char *filename;
   const char *ret_url;
   Evas_Object *screenshot;
   Ecore_X_Visual *visual;
   Ecore_X_Window xwin;
   Ecore_Con_Url *url;
   Eina_List *handlers;
   Ecore_Timer *timer;
   Evas *evas;
   Eina_Bool finish;
   int quality;
   const char *options;
   int x, y, w, h;
   struct tm *tm;
   void *data;
};

EAPI E_Screenshot *e_screenshot_new(void);
EAPI int e_screenshot_upload(E_Screenshot *e_ss);
EAPI void e_screenshot_quality_set(E_Screenshot *e_ss, int quality);
EAPI void e_screenshot_desktop_now(E_Screenshot *e_ss);
EAPI void e_screenshot_free(E_Screenshot *e_ss);

#endif
#endif
