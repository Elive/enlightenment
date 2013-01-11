# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

#include "e.h"

static EDBus_Connection *conn = NULL;
static int retval = EXIT_SUCCESS;
static int pending = 0;

static void
fm_open_reply(void *data __UNUSED__, const EDBus_Message *msg,
              EDBus_Pending *dbus_pending __UNUSED__)
{
   const char *name, *txt;
   if (edbus_message_error_get(msg, &name, &txt))
     {
        retval = EXIT_FAILURE;
        ERR("%s: %s", name, txt);
     }

   pending--;
   if (!pending) ecore_main_loop_quit();
}

static Eina_Bool
fm_error_quit_last(void *data __UNUSED__)
{
   if (!pending) ecore_main_loop_quit();
   return EINA_FALSE;
}

static void
fm_open(const char *path)
{
   EDBus_Message *msg;
   const char *method;
   char *p;

   if (path[0] == '/')
     p = strdup(path);
   else
     {
        char buf[PATH_MAX];
        if (!getcwd(buf, sizeof(buf)))
          {
             ERR("Could not get current working directory: %s", strerror(errno));
             ecore_idler_add(fm_error_quit_last, NULL);
             return;
          }
        if (strcmp(path, ".") == 0)
          p = strdup(buf);
        else
          {
             char tmp[PATH_MAX];
             snprintf(tmp, sizeof(tmp), "%s/%s", buf, path);
             p = strdup(tmp);
          }
     }

   DBG("'%s' -> '%s'", path, p);
   if ((!p) || (p[0] == '\0'))
     {
        ERR("Could not get path '%s'", path);
        ecore_idler_add(fm_error_quit_last, NULL);
        free(p);
        return;
     }

   if (ecore_file_is_dir(p))
     method = "OpenDirectory";
   else
     method = "OpenFile";

   msg = edbus_message_method_call_new("org.enlightenment.FileManager",
                                       "/org/enlightenment/FileManager",
                                       "org.enlightenment.FileManager",
                                       method);
   if (!msg)
     {
        ERR("Could not create DBus Message");
        ecore_idler_add(fm_error_quit_last, NULL);
        free(p);
        return;
     }
   edbus_message_arguments_append(msg, "s", p);
   free(p);

   if (!edbus_connection_send(conn, msg, fm_open_reply, NULL, -1))
     {
        ERR("Could not send DBus Message");
        ecore_idler_add(fm_error_quit_last, NULL);
     }
   else
     pending++;
   edbus_message_unref(msg);
}

static const Ecore_Getopt options = {
   "enlightenment_filemanager",
   "%prog [options] [file-or-folder1] ... [file-or-folderN]",
   PACKAGE_VERSION,
   "(C) 2012 Gustavo Sverzut Barbieri and others",
   "BSD 2-Clause",
   "Opens the Enlightenment File Manager at a given folders.",
   EINA_FALSE,
   {
      ECORE_GETOPT_VERSION('V', "version"),
      ECORE_GETOPT_COPYRIGHT('C', "copyright"),
      ECORE_GETOPT_LICENSE('L', "license"),
      ECORE_GETOPT_HELP('h', "help"),
      ECORE_GETOPT_SENTINEL
   }
};

EAPI int
main(int argc, char *argv[])
{
   Eina_Bool quit_option = EINA_FALSE;
   Ecore_Getopt_Value values[] = {
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_NONE
   };
   int args;

   args = ecore_getopt_parse(&options, values, argc, argv);
   if (args < 0)
     {
        ERR("Could not parse command line options.");
        return EXIT_FAILURE;
     }

   if (quit_option) return EXIT_SUCCESS;

   ecore_init();
   ecore_file_init();
   edbus_init();

   conn = edbus_connection_get(EDBUS_CONNECTION_TYPE_SESSION);
   if (!conn)
     {
        ERR("Could not DBus SESSION bus.");
        retval = EXIT_FAILURE;
        goto end;
     }

   retval = EXIT_SUCCESS;

   if (args == argc) fm_open(".");
   else
     {
        for (; args < argc; args++)
          fm_open(argv[args]);
     }

   ecore_main_loop_begin();
   edbus_connection_unref(conn);
end:
   edbus_shutdown();
   ecore_file_shutdown();
   ecore_shutdown();
   return retval;
}

