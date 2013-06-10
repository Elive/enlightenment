#include "e.h"

Eina_Bool xmodmap_run_already = EINA_FALSE;

EINTERN int
e_deskenv_init(void)
{
   Eina_Bool run_deskenv = EINA_FALSE;
   Eina_Bool run_xkb = EINA_FALSE;
   char buf[PATH_MAX], buf2[PATH_MAX + sizeof("xrdb -load ")];

   if (e_config->reload_deskenv_on_erestart)
     run_deskenv = EINA_TRUE;
   else if (!e_config->reload_deskenv_on_erestart)
     {
        if (e_config->enlightenment_restart_count == 1)
          run_deskenv = EINA_TRUE;
        else
          run_deskenv = EINA_FALSE;
     }

   // run xdrb -load .Xdefaults & .Xresources
   // NOTE: one day we should replace this with an e based config + service
   if (e_config->deskenv.load_xrdb && run_deskenv)
     {
        INF("Running DeskEnv!!!!");
        e_user_homedir_concat(buf, sizeof(buf), ".Xdefaults");
        if (ecore_file_exists(buf))
          {
             snprintf(buf2, sizeof(buf2), "xrdb -load %s", buf);
             ecore_exe_run(buf2, NULL);
          }
        e_user_homedir_concat(buf, sizeof(buf), ".Xresources");
        if (ecore_file_exists(buf))
          {
             snprintf(buf2, sizeof(buf2), "xrdb -load %s", buf);
             ecore_exe_run(buf2, NULL);
          }
     }

   if (e_config->reload_xkb_on_erestart)
     {
        run_xkb = EINA_TRUE;
        if (xmodmap_run_already)
          xmodmap_run_already = EINA_FALSE;
     }
   else if (!e_config->reload_xkb_on_erestart)
     {
        if (e_config->enlightenment_restart_count == 1)
          run_xkb = EINA_TRUE;
        else
          run_xkb = EINA_FALSE;
     }

   //FIXME: e_xkb_update runs e_deskenv_xmodmap_run, so why do we run it here?
   if(run_xkb)
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

EAPI void
e_deskenv_xmodmap_run(void)
{
   char buf[PATH_MAX], buf2[PATH_MAX + sizeof("xmodmap ")];
   // load ~/.Xmodmap
   // NOTE: one day we should replace this with an e based config + service
   if (!e_config->deskenv.load_xmodmap) return;
   if (xmodmap_run_already) return;
   e_user_homedir_concat(buf, sizeof(buf), ".Xmodmap");
   if (!ecore_file_exists(buf)) return;
   snprintf(buf2, sizeof(buf2), "xmodmap %s", buf);
   INF("SET XMODMAP RUN: %s", buf2);
   ecore_exe_run(buf2, NULL);
   xmodmap_run_already = EINA_TRUE;
}
