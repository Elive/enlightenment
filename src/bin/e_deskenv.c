#include "e.h"

static Ecore_Timer *xmodmap_timer = NULL;
static Eina_Bool xmodmap_ran = EINA_FALSE;

EINTERN int
e_deskenv_init(void)
{
   char buf[PATH_MAX], buf2[PATH_MAX + sizeof("xrdb -load ")];

   // run xdrb -load .Xdefaults & .Xresources
   // NOTE: one day we should replace this with an e based config + service
   if ((e_config->deskenv.load_xrdb && (e_config->enlightenment_restart_count == 1)) ||
        e_config->deskenv.load_xrdb_always)
     {
        e_user_homedir_concat(buf, sizeof(buf), ".Xdefaults");
        if (ecore_file_exists(buf))
          {
             snprintf(buf2, sizeof(buf2), "xrdb -load %s", buf);
             INF("Running: %s", buf2);
             ecore_exe_run(buf2, NULL);
          }
        e_user_homedir_concat(buf, sizeof(buf), ".Xresources");
        if (ecore_file_exists(buf))
          {
             snprintf(buf2, sizeof(buf2), "xrdb -load %s", buf);
             INF("Running: %s", buf2);
             ecore_exe_run(buf2, NULL);
          }
     }

   e_deskenv_xmodmap_run();

   // make gnome apps happy
   // NOTE: one day we should replace this with an e based config + service
   if (e_config->deskenv.load_gnome)
     {
        ecore_exe_run("gnome-settings-daemon", NULL);
     }

   // make kde apps happy
   // NOTE: one day we should replace this with an e based config + service ??
   if (e_config->deskenv.load_kde)
     {
        ecore_exe_run("kdeinit", NULL);
     }
   return 1;
}

EINTERN int
e_deskenv_shutdown(void)
{
   return 1;
}

static Eina_Bool
_e_deskenv_xmodmap_run(EINA_UNUSED void *data)
{
   if (!e_xkb_setxkbmap_ran_get()) return ECORE_CALLBACK_RENEW;

   if ((e_config->deskenv.load_xmodmap && (e_config->enlightenment_restart_count == 1)) ||
        e_config->deskenv.load_xmodmap_always)
     {
        char buf[PATH_MAX + sizeof("xmodmap ")];

        snprintf(buf, sizeof(buf), "xmodmap %s",(char*) data);
        free(data);
        INF("SET XMODMAP RUN: %s", buf);
        ecore_exe_run(buf, NULL);
        xmodmap_ran = EINA_TRUE;

        if (xmodmap_timer) ecore_timer_del(xmodmap_timer);
        xmodmap_timer = NULL;
        return ECORE_CALLBACK_DONE;
     }

   xmodmap_timer = NULL;
   return ECORE_CALLBACK_DONE;
}


EAPI void
e_deskenv_xmodmap_run(void)
{
   char buf[PATH_MAX];
   // load ~/.Xmodmap
   // NOTE: one day we should replace this with an e based config + service
   e_user_homedir_concat(buf, sizeof(buf), ".Xmodmap");
   if (!ecore_file_exists(buf)) return;
   if (xmodmap_ran) return;

   if (xmodmap_timer) ecore_timer_del(xmodmap_timer);
   xmodmap_timer = ecore_timer_add(0.150, _e_deskenv_xmodmap_run, strdup(buf));
}
