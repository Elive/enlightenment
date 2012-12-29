/* Language chooser */
#include "e_wizard.h"

#define FREE_SPLIT(x) if (x[0]) { free(x[0]); free(x); }
typedef struct _Layout Layout;
typedef struct _XKB_Data XKB_Data;

struct _Layout
{
   const char *name;
   const char *label;
};

struct _XKB_Data
{
   Eina_List *layout;
   Eina_List *options;
   const char *variant;
   const char *model;
};

static const char *rules_file = NULL;
static const char *layout = NULL;
static Eina_List *layouts = NULL;

static void
find_rules(void)
{
   int i = 0;
   const char *lstfiles[] = {
#if defined __NetBSD__
      "/usr/X11R7/lib/X11/xkb/rules/xorg.lst",
#elif defined __OpenBSD__
      "/usr/X11R6/share/X11/xkb/rules/base.lst",
#endif
      "/usr/share/X11/xkb/rules/xorg.lst",
      "/usr/share/X11/xkb/rules/xfree86.lst",
      "/usr/local/share/X11/xkb/rules/xorg.lst",
      "/usr/local/share/X11/xkb/rules/xfree86.lst",
      "/usr/X11R6/lib/X11/xkb/rules/xorg.lst",
      "/usr/X11R6/lib/X11/xkb/rules/xfree86.lst",
      "/usr/local/X11R6/lib/X11/xkb/rules/xorg.lst",
      "/usr/local/X11R6/lib/X11/xkb/rules/xfree86.lst",
      NULL
   };

   for (; lstfiles[i]; i++)
     {
        FILE *f = fopen(lstfiles[i], "r");
        if (f)
          {
             fclose(f);
             rules_file = lstfiles[i];
             break;
          }
     }
}

static char*
_xkb_clean_line(char* s)
{
    char* space;
#define STR_CLEAN(x)             \
 while ((space = strchr(s, x)))  \
    {                            \
        *space = '\0';           \
        strcat(s, space + 1);    \
    }

    if (strchr(s, '#')) return NULL;
    if (!strchr(s, '=')) return NULL;

    STR_CLEAN(' ');
    STR_CLEAN('\"');
    STR_CLEAN('\n');
#undef STR_CLEAN

    space = strchr(s, '=');
    if ((space-s+1) == (int) strlen(s)) return NULL;
    if (strncmp(s, "XKB", 3)) return NULL;
    return s;
}

static Eina_List *
_xkb_list_get(const char *data)
{
   Eina_List *l = NULL;

   if (strchr(data, ','))
     {
        int i;
        char **split;

        split = eina_str_split(data, ",", 0);

        for (i = 0; split[i] != NULL; i++)
          {
             if(strlen(split[i]) == 0)
               {
                  l = eina_list_append(l, NULL);
                  continue;
               }
             l = eina_list_append(l, eina_stringshare_add(split[i]));
          }
        FREE_SPLIT(split);
     }
   return l;
}

static XKB_Data *
_xkb_data_get(void)
{
   XKB_Data *xkb_data = NULL;
   FILE *fp;
   fp = fopen("/etc/default/keyboard", "r");

   if (fp)
     {
        xkb_data = E_NEW(XKB_Data, 1);

        char line [1024];
        while (fgets(line, sizeof line, fp))
          {
             if (_xkb_clean_line(line))
               {
                  char **split;
                  split = eina_str_split(line, "=", 2);

                  if (!strcasecmp(split[0], "XKBMODEL"))
                    xkb_data->model = eina_stringshare_add(split[1]);

                  if (!strcasecmp(split[0], "XKBLAYOUT"))
                    xkb_data->layout = _xkb_list_get(split[1]);

                  if (!strcasecmp(split[0], "XKBVARIANT"))
                    xkb_data->variant = eina_stringshare_add(split[1]);

                  if (!strcasecmp(split[0], "XKBOPTIONS"))
                    xkb_data->options = _xkb_list_get(split[1]);

                  FREE_SPLIT(split);
               }
          }
        fclose(fp);
     }
   return xkb_data;
}

static void
_xkb_data_add_to_e_config(XKB_Data *data)
{
   int i = 0;
   const char *lay, *var, *opt, *mod;
   E_Config_XKB_Layout *nl;
   E_Config_XKB_Option *no;

   if (!data) return;

   EINA_LIST_FREE(data->layout, lay)
     {
        if (!lay) continue;

        var = eina_stringshare_add(eina_list_nth(_xkb_list_get(data->variant), i));
        if (!var) var = eina_stringshare_add("basic");

        mod = data->model ? data->model : eina_stringshare_add("default");

        printf("Setting keyboard layout: %s|%s|%s \n", lay, data->model, var);

        if (!i && e_config->xkb.used_layouts)
          {
             E_Config_XKB_Layout *def;

             def = eina_list_nth(e_config->xkb.used_layouts, 0);
             if (!strcmp(def->name, lay))
               {
                  def->variant = eina_stringshare_add(var);
                  def->model = eina_stringshare_add(mod);
                  e_xkb_update(1);
                  e_xkb_layout_set(def);
                  i++;
                  continue;
               }
          }

        nl = E_NEW(E_Config_XKB_Layout, 1);
        nl->name = eina_stringshare_add(lay);
        nl->variant = eina_stringshare_add(var);
        nl->model = eina_stringshare_add(mod);

        e_config->xkb.used_layouts = eina_list_append(e_config->xkb.used_layouts, nl);
        if (!i)
          e_xkb_layout_set(nl);
        i++;
     }

   e_config->xkb.used_options = eina_list_free(e_config->xkb.used_options);
   EINA_LIST_FREE(data->options, opt)
     {
        no = E_NEW(E_Config_XKB_Option, 1);
        no->name = eina_stringshare_add(opt);
        e_config->xkb.used_options = eina_list_append(e_config->xkb.used_options, no);
     }
    eina_stringshare_del(data->model);
    eina_stringshare_del(data->variant);
    free(data);
}

static const char*
locale_region_keyboard_get(void)
{
   const char *kb = eina_stringshare_add("us");
   E_Locale_Parts *locale;

   locale = e_intl_locale_parts_get(e_intl_language_get());
   fprintf(stdout, "Lang:[%s], Region:[%s] \n", locale->lang, locale->region);
   if (locale)
     {
        Eina_List *l;
        Layout *lay;

        if (!strcasecmp(locale->lang, "eo"))
          {
             eina_stringshare_del(kb);
             return eina_stringshare_add("epo");;
          }

        EINA_LIST_FOREACH(layouts, l, lay)
          {
             fprintf(stdout, "Lay[%s] || Lang:[%s], Region:[%s] \n",lay->name, locale->lang, locale->region);

             if (locale->region && (strcasecmp(lay->name, locale->region) == 0))
               {
                  eina_stringshare_del(kb);
                  kb = eina_stringshare_add(lay->name);
                  break;
               }

             if (locale->lang && (strcasecmp(lay->name, locale->lang) == 0))
               {
                  eina_stringshare_del(kb);
                  kb = eina_stringshare_add(lay->name);
                  break;
               }
          }
     }
   return kb;
}

static int
_layout_sort_cb(const void *data1, const void *data2)
{
   const Layout *l1 = data1;
   const Layout *l2 = data2;
   return e_util_strcasecmp(l1->label ?: l1->name, l2->label ?: l2->name);
}

int
parse_rules(void)
{
   char buf[4096];
   FILE *f = fopen(rules_file, "r");
   if (!f) return 0;

   for (;; )
     {
        if (!fgets(buf, sizeof(buf), f)) goto err;
        if (!strncmp(buf, "! layout", 8))
          {
             for (;; )
               {
                  Layout *lay;
                  char name[4096], label[4096];

                  if (!fgets(buf, sizeof(buf), f)) goto err;
                  if (sscanf(buf, "%s %[^\n]", name, label) != 2) break;
                  lay = calloc(1, sizeof(Layout));
                  lay->name = eina_stringshare_add(name);
                  lay->label = eina_stringshare_add(label);
                  layouts = eina_list_append(layouts, lay);
               }
             break;
          }
     }
err:
   fclose(f);
   layouts = eina_list_sort(layouts, 0, _layout_sort_cb);
   return 1;
}

static void
implement_layout(void)
{
   Eina_List *l;
   E_Config_XKB_Layout *nl;
   Eina_Bool found = EINA_FALSE;
   XKB_Data *data;

   data = _xkb_data_get();
   _xkb_data_add_to_e_config(data);

   if (!layout) return;

   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, nl)
     {
        if ((nl->name) && (!strcmp(layout, nl->name)))
          {
             found = EINA_TRUE;
             break;
          }
     }
   if (!found)
     {
        nl = E_NEW(E_Config_XKB_Layout, 1);
        nl->name = eina_stringshare_ref(layout);
        nl->variant = eina_stringshare_add("basic");
        nl->model = eina_stringshare_add("default");
        e_config->xkb.used_layouts = eina_list_prepend(e_config->xkb.used_layouts, nl);
        e_xkb_update(-1);
     }
   e_xkb_layout_set(nl);
}

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__, Eina_Bool *need_xdg_desktops __UNUSED__, Eina_Bool *need_xdg_icons __UNUSED__)
{
   // parse kbd rules here
   find_rules();
   parse_rules();
   return 1;
}
/*
EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}
*/
EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o, *of, *ob, *ic;
   Eina_List *l;
   const char *kb;
   int i, sel = -1;

   kb = locale_region_keyboard_get();

   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Keyboard"));
   of = e_widget_framelist_add(pg->evas, _("Select one"), 0);
   ob = e_widget_ilist_add(pg->evas, 32 * e_scale, 32 * e_scale, &layout);
   e_widget_size_min_set(ob, 140 * e_scale, 140 * e_scale);

   e_widget_ilist_freeze(ob);
   for (i = 0, l = layouts; l; l = l->next, i++)
     {
        Layout *lay;
        const char *label;

        lay = l->data;
        ic = e_icon_add(pg->evas);
        e_xkb_e_icon_flag_setup(ic, lay->name);
        label = lay->label;
        if (!label) label = "Unknown";
        e_widget_ilist_append(ob, ic, _(label), NULL, NULL, lay->name);
        if (lay->name)
          {
             fprintf(stdout, "Lay:[%s] Reg[%s]\n", lay->name, kb);
             if (!strcasecmp(lay->name, kb)) sel = i;
          }
     }
   eina_stringshare_del(kb);

   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   if (sel >= 0)
     {
        e_widget_ilist_selected_set(ob, sel);
        e_widget_ilist_nth_show(ob, sel, 0);
     }

   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   evas_object_show(ob);
   evas_object_show(of);
   e_wizard_page_show(o);
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   /* special - key layout inits its stuff the moment it goes away */
   implement_layout();
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   // do this again as we want it to apply to the new profile
   implement_layout();
   return 1;
}
#undef FREE_SPLIT

