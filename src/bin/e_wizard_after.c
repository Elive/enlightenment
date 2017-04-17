#include "e.h"

#define LIVE_SYSTEM "/usr/lib/deliver/hooks.d/e17-wizard"
#define USER_SYSTEM "/usr/lib/user-manager/hooks.d/e17-wizard"
#define E_WIZARD_TIMEOUT 360

# define TIME_SCRIPT(x)                                        \
{                                                          \
   p1 = ecore_time_unix_get();                             \
   printf("E_Wizard_After: %1.5f [%1.5f] - %s\n", p1 - p0, p1 - p2, x); \
   p2 = p1;                                                \
}


typedef struct _E_Wizard_After E_Wizard_After;
struct _E_Wizard_After
{
   Eina_List *script_list;
   Ecore_Timer *timer;

   const char *script_dir;
   const char *btn_label;
   Eina_Bool quiet;
   int total;
};

static const char    *_e_wizard_after_script_dir_get(const char *place);
static Eina_List     *_e_wizard_after_script_list_get(const char *dir);

static void          _e_wizard_after_run(const char *place);
Eina_Bool            _e_wizard_after_done(void);
static void          _e_wizard_after_script_load(void);

static Eina_Bool     _e_wizard_after_script_quiet_get(void);
static Eina_Bool     _e_wizard_after_cb_handler(void *data, int type, void *event);
static Eina_Bool     _e_wizard_after_cb_timeout(void *data);

static int           _e_wizard_after_script_number = 1;
static int           _e_wizard_after_timeout = E_WIZARD_TIMEOUT;
static int           _e_wizard_after_idle = 0;

static pid_t pid;
static double p0, p1, p2;

E_Wizard_After *ewp;

EAPI void
e_wizard_after_run(void)
{
   p0 = p1 = p2 = ecore_time_unix_get();
   TIME_SCRIPT("Start E_Wizard_After");
   _e_wizard_after_run("after");
}

EAPI Eina_Bool
e_wizard_after_done(void)
{
   return _e_wizard_after_done();
}

static void
_e_wizard_after_run(const char *place)
{

   ewp = E_NEW(E_Wizard_After, 1);

   //Initialize!
   ewp->script_dir = NULL;
   ewp->script_list = NULL;
   ewp->total = 0;
   ewp->quiet = EINA_FALSE;

   _e_wizard_after_script_number = 1;
   _e_wizard_after_idle = 0;
   _e_wizard_after_timeout = E_WIZARD_TIMEOUT;

   ewp->quiet = _e_wizard_after_script_quiet_get();
   ewp->script_dir = eina_stringshare_add(_e_wizard_after_script_dir_get(place));

   if (!ewp->script_dir)
     {
        fprintf(stderr, "Unable to get script directory\n");
        return;
     }

   ewp->script_list = _e_wizard_after_script_list_get(ewp->script_dir);

   if (!ewp->script_list)
     {
        fprintf(stderr, "Unable to generate script list\n");
        return;
     }

   ewp->total = eina_list_count(ewp->script_list);

   _e_wizard_after_script_load();
   ewp->timer = ecore_timer_add(1.0, _e_wizard_after_cb_timeout, NULL);
}

Eina_Bool
_e_wizard_after_done(void)
{
   if (!_e_wizard_after_timeout) return EINA_TRUE;
   if (_e_wizard_after_script_number <= ewp->total)
     return EINA_FALSE;

   if (ewp->btn_label) eina_stringshare_del(ewp->btn_label);
   ewp->btn_label = NULL;

   if ((_e_wizard_after_idle <= 3) && (ewp->total > 0))
     {
        _e_wizard_after_idle++;
        return EINA_FALSE;
     }

   if (ewp->script_dir) eina_stringshare_del(ewp->script_dir);
   if (ewp->timer) ecore_timer_del(ewp->timer);

   ewp->timer = NULL;
   ewp->script_dir = NULL;

   eina_list_free(ewp->script_list);
   E_FREE(ewp);

   TIME_SCRIPT("Finish E_Wizard Pre/Post");
   return EINA_TRUE;
}


/* This function looks in "/proc/cmdline" for "boot=live" if it is
 * found it will return LIVE_SYSTEM{first/last} else USER_SYSTEM{first/last}
 */
static const char*
_e_wizard_after_script_dir_get(const char *place)
{
   const char filename[] = "/proc/cmdline";
   FILE *file;
   char buf[PATH_MAX];

   file = fopen(filename, "r");

   if (file)
     {
        char line[1024];

        fgets(line, sizeof(line), file);
        if (strstr(line, "boot=live"))
          snprintf(buf, sizeof(buf), "%s/%s", LIVE_SYSTEM, place);
        else
          snprintf(buf, sizeof(buf), "%s/%s", USER_SYSTEM, place);
        fclose(file);
     }

   return strdup(buf);
}

static Eina_Bool
_e_wizard_after_script_quiet_get(void)
{
   const char filename[] = "/proc/cmdline";
   FILE *file;
   Eina_Bool quiet = EINA_FALSE;

   // default value
   quiet = EINA_TRUE;

   file = fopen(filename, "r");

   if (file)
     {
        char line[1024];

        fgets(line, sizeof(line), file);
        if (strstr(line, "debug"))
          quiet = EINA_FALSE;
        fclose(file);
     }

   return quiet;
}

static void
_e_wizard_after_script_load(void)
{
   Eina_List *l;
   Eina_Strbuf *buf;
   Eina_Stringshare *filename;
   size_t base_len;

   if (!ewp) return;
   if (!ewp->script_dir) return;
   if (!ewp->script_list) return;

   base_len = strlen(ewp->script_dir) + 1;

   buf = eina_strbuf_new();
   eina_strbuf_append(buf, ewp->script_dir);
   eina_strbuf_append_char(buf, '/');

   EINA_LIST_FOREACH(ewp->script_list, l, filename)
     {
        Ecore_Exe *child;

        eina_strbuf_append(buf, filename);
        eina_strbuf_prepend(buf, " \"");
        eina_strbuf_append(buf, "\"");

        if (!ewp->quiet)
          eina_strbuf_prepend(buf, "urxvt -e ");

        child = ecore_exe_pipe_run(eina_strbuf_string_get(buf),
                                   ECORE_EXE_PIPE_WRITE |
                                   ECORE_EXE_PIPE_READ_LINE_BUFFERED |
                                   ECORE_EXE_PIPE_ERROR_LINE_BUFFERED,
                                   NULL);

        pid = ecore_exe_pid_get(child);

        if (pid == -1)
          ERR("Unable to retreive PID");

        ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _e_wizard_after_cb_handler, NULL);
        eina_strbuf_remove(buf, base_len, strlen(eina_strbuf_string_get(buf)));

        ewp->script_list = eina_list_remove(ewp->script_list, filename);
        break;
     }
   eina_strbuf_string_free(buf);
}

static Eina_List*
_e_wizard_after_script_list_get(const char *dir)
{
   Eina_List *list, *script_list = NULL;
   char *file;

   if (!ecore_file_exists(dir) ||
       !ecore_file_is_dir(dir)) return NULL;

   list = ecore_file_ls(dir);

   EINA_LIST_FREE(list, file)
     {
        char buf[PATH_MAX];
        snprintf(buf, sizeof(buf), "%s/%s", dir, file);

        if (!ecore_file_exists(buf) || !ecore_file_can_exec(buf) ||
            ecore_file_is_dir(buf))
          continue;

        if (ecore_file_can_exec(buf))
          script_list = eina_list_append(script_list, file);
     }
   return script_list;
}

static Eina_Bool
_e_wizard_after_cb_handler(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Exe_Event_Del *eed;
   const char *cmd;

   if (!(eed = event)) return ECORE_CALLBACK_CANCEL;
   if (!eed->exe) return ECORE_CALLBACK_CANCEL;

   cmd = ecore_exe_cmd_get(eed->exe);
   if (!cmd) return ECORE_CALLBACK_CANCEL;

   if (eed->pid == pid)
     {
        TIME_SCRIPT(cmd);

        _e_wizard_after_script_number++;
        _e_wizard_after_script_load();
     }
   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_e_wizard_after_cb_timeout(void *data EINA_UNUSED)
{
   if (_e_wizard_after_timeout)
     {
        _e_wizard_after_timeout--;
        return ECORE_CALLBACK_RENEW;
     }
   else
     TIME_SCRIPT("Timed Out");
   _e_wizard_after_timeout = E_WIZARD_TIMEOUT;
   return ECORE_CALLBACK_DONE;
}

