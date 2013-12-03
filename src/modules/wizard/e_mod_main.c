#include "e_wizard.h"

/* actual module specifics */
E_Module *wiz_module = NULL;

/*
 * These are the currently planned wizard pages:
 *
 * o == interactive
 * . == automatic (no gui - none implemented currently)
 *
 * * = done
 * - = code here, but disabled in build
 *
 * --- THE LIST
 * o *ask for language (default selection is current locale).
 * o *ask for initial profile
 * o *find XDG app menus/repositories and list them let user choose which
 *    one(s) are to be used.
 * o -ask for ibar initial app set
 * o -ask if user wants desktop icons or not (enable fwin module but seed it
 *    with default config icons on desktop and favorites).
 * o -ask click to focus or sloppy
 * . *take some of current config (language, fileman, profile) and load
 *    load profile, apply language to it and save, restart e.
 *
 * why are some disabled? profiels take care of this and do a better job
 * at collecting all the things together. for example illume makes no sense
 * with pointer focus and ibar icons/desktop makes no sense.
 */

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Wizard"
};

double t0, t1, t2;

static int
_cb_sort_files(char *f1, char *f2)
{
   return strcmp(f1, f2);
}

EAPI void *
e_modapi_init(E_Module *m)
{
   Eina_List *files;
   char buf[PATH_MAX];
   char *file;

   t0 = ecore_time_unix_get();
   t1 = t0;
   t2 = t0;

   wiz_module = m;
   e_wizard_init();

   e_config->scale.use_dpi = 0;
   e_config->scale.use_custom = 1;
   e_config->scale.factor = 1.2;
   e_scale_update();
   snprintf(buf, sizeof(buf), "%s/%s", e_module_dir_get(m), MODULE_ARCH);
   files = ecore_file_ls(buf);
   files = eina_list_sort(files, 0, (Eina_Compare_Cb)_cb_sort_files);
   EINA_LIST_FREE(files, file)
     {
        if ((!strncmp(file, "page_", 5)) && (eina_str_has_extension(file, ".so")))
          {
             void *handle;

             snprintf(buf, sizeof(buf), "%s/%s/%s",
                      e_module_dir_get(m), MODULE_ARCH, file);
             handle = dlopen(buf, RTLD_NOW | RTLD_GLOBAL);
             TS(buf);
             if (handle)
               e_wizard_page_add(handle,
                                 dlsym(handle, "wizard_page_init"),
                                 dlsym(handle, "wizard_page_shutdown"),
                                 dlsym(handle, "wizard_page_show"),
                                 dlsym(handle, "wizard_page_hide"),
                                 dlsym(handle, "wizard_page_apply"));
             else
               {
                  // if its an executable...
                  // XXX
                  // else...
                  printf("%s\n", dlerror());
               }
          }
        free(file);
     }
   e_wizard_go();

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   e_wizard_shutdown();
   wiz_module = NULL;
// FIXME: wrong place
//   e_module_disable(m); /* disable - on restart this won't be loaded now */
//   e_sys_action_do(E_SYS_RESTART, NULL); /* restart e - cleanly try settings */
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}

