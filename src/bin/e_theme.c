#include "e.h"

/* local subsystem functions */
typedef struct _E_Theme_Result E_Theme_Result;

struct _E_Theme_Result
{
   const char *file;
   const char *cache;
   Eina_Hash  *quickfind;
};

static Eina_Bool  _e_theme_mappings_free_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata);
static Eina_Bool  _e_theme_mappings_quickfind_free_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata);
static void       _e_theme_category_register(const char *category);
static Eina_List *_e_theme_collection_item_register(Eina_List *list, const char *name);
static Eina_List *_e_theme_collection_items_find(const char *base, const char *collname);
static void e_theme_handler_set(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *path);
static int e_theme_handler_test(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *path);

/* local subsystem globals */
static Eina_Hash *mappings = NULL;
static Eina_Hash *group_cache = NULL;

static Eina_List *categories = NULL;
static E_Fm2_Mime_Handler *theme_hdl = NULL;

/* externally accessible functions */

EINTERN int
e_theme_init(void)
{
   E_Config_Theme *et;
   Eina_List *l = NULL;

   /* Register mime handler */
   theme_hdl = e_fm2_mime_handler_new(_("Set As Theme"), "preferences-desktop-theme",
                                      e_theme_handler_set, NULL,
                                      e_theme_handler_test, NULL);
   if (theme_hdl) e_fm2_mime_handler_glob_add(theme_hdl, "*.edj");

   /* this is a fallback that is ALWAYS there - if all fails things will */
   /* always fall back to the default theme. the rest after this are config */
   /* values users can set */
   if (ecore_file_exists("/usr/share/enlightenment/data/themes/Elive Light.edj"))
     e_theme_file_set("base", "Elive Light.edj");
   else
     e_theme_file_set("base", "default.edj");

   EINA_LIST_FOREACH(e_config->themes, l, et)
     {
        char buf[256];

        snprintf(buf, sizeof(buf), "base/%s", et->category);
        e_theme_file_set(buf, et->file);
     }

   if (!mappings) mappings = eina_hash_string_superfast_new(NULL);
   group_cache = eina_hash_string_superfast_new(NULL);

   return 1;
}

EINTERN int
e_theme_shutdown(void)
{
   const char *str;

   if (theme_hdl)
     {
        e_fm2_mime_handler_glob_del(theme_hdl, "*.edj");
        e_fm2_mime_handler_free(theme_hdl);
     }
   if (mappings)
     {
        eina_hash_foreach(mappings, _e_theme_mappings_free_cb, NULL);
        eina_hash_free(mappings);
        mappings = NULL;
     }
   if (group_cache)
     {
        eina_hash_free(group_cache);
        group_cache = NULL;
     }
   EINA_LIST_FREE(categories, str)
     eina_stringshare_del(str);

   return 1;
}

/**  
 * Assigns a edje group from the current theme to 
 * a recently created edje object 
 *
 * @param o edje object to assign group to
 * @param category that hold the required edj file
 * @param group the edje group to use
 * @return returns true on success, false if unable to assign group
 */
EAPI int
e_theme_edje_object_set(Evas_Object *o, const char *category, const char *group)
{
   E_Theme_Result *res;
   char buf[256];
   char *p;

   /* find category -> edje mapping */
   _e_theme_category_register(category);
   res = eina_hash_find(mappings, category);
   if (res)
     {
        const char *str;

        /* if found check cached path */
        str = res->cache;
        if (!str)
          {
             /* no cached path */
             str = res->file;
             /* if its not an absolute path find it */
             if (str[0] != '/')
               str = e_path_find(path_themes, str);
             /* save cached value */
             if (str) res->cache = str;
          }
        if (str)
          {
             void *tres;
             int ok;

             snprintf(buf, sizeof(buf), "%s/::/%s", str, group);
             tres = eina_hash_find(group_cache, buf);
             if (!tres)
               {
                  ok = edje_object_file_set(o, str, group);
                  /* save in the group cache hash */
                  if (ok)
                    eina_hash_add(group_cache, buf, res);
                  else
                    eina_hash_add(group_cache, buf, (void *)1);
               }
             else if (tres == (void *)1)
               ok = 0;
             else
               ok = 1;
             if (ok)
               {
                  if (tres)
                    edje_object_file_set(o, str, group);
                  return 1;
               }
          }
     }
   /* no mapping or set failed - fall back */
   eina_strlcpy(buf, category, sizeof(buf));
   /* shorten string up to and not including last / char */
   p = strrchr(buf, '/');
   if (p) *p = 0;
   /* no / anymore - we are already as far back as we can go */
   else return 0;
   /* try this category */
   return e_theme_edje_object_set(o, buf, group);
}

const char *
_e_theme_edje_file_get(const char *category, const char *group, Eina_Bool fallback_icon)
{
   E_Theme_Result *res;
   char buf[4096];
   const char *q;
   char *p;

   /* find category -> edje mapping */
   _e_theme_category_register(category);
   res = eina_hash_find(mappings, category);

   if (e_config->icon_theme &&
       (!fallback_icon) &&
       (!strcmp(category, "base")) &&
       (!strncmp(group, "e/icons", 7)))
     return "";

   if (res)
     {
        const char *str;

        /* if found check cached path */
        str = res->cache;
        if (!str)
          {
             /* no cached path */
             str = res->file;
             /* if its not an absolute path find it */
             if (str[0] != '/')
               str = e_path_find(path_themes, str);
             /* save cached value */
             if (str) res->cache = str;
          }
        if (str)
          {
             void *tres;
             Eina_List *coll, *l;
             int ok;

             snprintf(buf, sizeof(buf), "%s/::/%s", str, group);
             tres = eina_hash_find(group_cache, buf);
             if (!tres)
               {
                  /* if the group exists - return */
                  if (!res->quickfind)
                    {
                       const char *col;

                       res->quickfind = eina_hash_string_superfast_new(NULL);
                       /* create a quick find hash of all group entries */
                       coll = edje_file_collection_list(str);

                       EINA_LIST_FOREACH(coll, l, col)
                         {
                            q = eina_stringshare_add(col);
                            eina_hash_direct_add(res->quickfind, q, q);
                         }
                       if (coll) edje_file_collection_list_free(coll);
                    }
                  /* save in the group cache hash */
                  if (eina_hash_find(res->quickfind, group))
                    {
                       eina_hash_add(group_cache, buf, res);
                       ok = 1;
                    }
                  else
                    {
                       eina_hash_add(group_cache, buf, (void *)1);
                       ok = 0;
                    }
               }
             else if (tres == (void *)1) /* special pointer "1" == not there */
               ok = 0;
             else
               ok = 1;
             if (ok) return str;
          }
     }
   /* no mapping or set failed - fall back */
   eina_strlcpy(buf, category, sizeof(buf));
   /* shorten string up to and not including last / char */
   p = strrchr(buf, '/');
   if (p) *p = 0;
   /* no / anymore - we are already as far back as we can go */
   else return "";
   /* try this category */
   return e_theme_edje_file_get(buf, group);
}

EAPI const char *
e_theme_edje_file_get(const char *category, const char *group)
{
   return _e_theme_edje_file_get(category, group, EINA_FALSE);
}

EAPI const char *
e_theme_edje_icon_fallback_file_get(const char *group)
{
   return _e_theme_edje_file_get("base", group, EINA_TRUE);
}

/*
 * this is used to set the theme for a CATEGORY of E17. "base" is always set
 * to the default theme - because if a selected theme wants "base/theme", but
 * does not provide theme elements, it can fall back to the default theme.
 *
 * the idea is you can actually set a different theme for different parts of
 * the desktop... :)
 *
 * other possible categories...
 *  e_theme_file_set("base/theme/about", "default.edj");
 *  e_theme_file_set("base/theme/borders", "default.edj");
 *  e_theme_file_set("base/theme/background", "default.edj");
 *  e_theme_file_set("base/theme/configure", "default.edj");
 *  e_theme_file_set("base/theme/dialog", "default.edj");
 *  e_theme_file_set("base/theme/menus", "default.edj");
 *  e_theme_file_set("base/theme/error", "default.edj");
 *  e_theme_file_set("base/theme/gadman", "default.edj");
 *  e_theme_file_set("base/theme/dnd", "default.edj");
 *  e_theme_file_set("base/theme/icons", "default.edj");
 *  e_theme_file_set("base/theme/pointer", "default.edj");
 *  e_theme_file_set("base/theme/transitions", "default.edj");
 *  e_theme_file_set("base/theme/widgets", "default.edj");
 *  e_theme_file_set("base/theme/winlist", "default.edj");
 *  e_theme_file_set("base/theme/modules", "default.edj");
 *  e_theme_file_set("base/theme/modules/pager", "default.edj");
 *  e_theme_file_set("base/theme/modules/ibar", "default.edj");
 *  e_theme_file_set("base/theme/modules/ibox", "default.edj");
 *  e_theme_file_set("base/theme/modules/clock", "default.edj");
 *  e_theme_file_set("base/theme/modules/battery", "default.edj");
 *  e_theme_file_set("base/theme/modules/cpufreq", "default.edj");
 *  e_theme_file_set("base/theme/modules/start", "default.edj");
 *  e_theme_file_set("base/theme/modules/temperature", "default.edj");
 */

EAPI void
e_theme_file_set(const char *category, const char *file)
{
   E_Theme_Result *res;

   if (group_cache)
     {
        eina_hash_free(group_cache);
        group_cache = NULL;
     }
   _e_theme_category_register(category);
   res = eina_hash_find(mappings, category);
   if (res)
     {
        eina_hash_del(mappings, category, res);
        if (res->file)
          {
             e_filereg_deregister(res->file);
             eina_stringshare_del(res->file);
          }
        if (res->cache) eina_stringshare_del(res->cache);
        E_FREE(res);
     }
   res = E_NEW(E_Theme_Result, 1);
   res->file = eina_stringshare_add(file);
   e_filereg_register(res->file);
   if (!mappings)
     mappings = eina_hash_string_superfast_new(NULL);
   eina_hash_add(mappings, category, res);
}

EAPI int
e_theme_config_set(const char *category, const char *file)
{
   E_Config_Theme *ect;
   Eina_List *next;

   /* Don't accept unused categories */
#if 0
   if (!e_theme_category_find(category)) return 0;
#endif

   /* search for the category */
   EINA_LIST_FOREACH(e_config->themes, next, ect)
     {
        if (!strcmp(ect->category, category))
          {
             if (ect->file) eina_stringshare_del(ect->file);
             ect->file = eina_stringshare_add(file);
             return 1;
          }
     }

   /* the text class doesnt exist */
   ect = E_NEW(E_Config_Theme, 1);
   ect->category = eina_stringshare_add(category);
   ect->file = eina_stringshare_add(file);

   e_config->themes = eina_list_append(e_config->themes, ect);
   return 1;
}

/*
 * returns a pointer to the data, return null if nothing if found.
 */
EAPI E_Config_Theme *
e_theme_config_get(const char *category)
{
   E_Config_Theme *ect = NULL;
   Eina_List *next;

   /* search for the category */
   EINA_LIST_FOREACH(e_config->themes, next, ect)
     {
        if (!strcmp(ect->category, category))
          return ect;
     }
   return NULL;
}

EAPI int
e_theme_config_remove(const char *category)
{
   E_Config_Theme *ect;
   Eina_List *next;

   /* search for the category */
   EINA_LIST_FOREACH(e_config->themes, next, ect)
     {
        if (!e_util_strcmp(ect->category, category))
          {
             e_config->themes = eina_list_remove_list(e_config->themes, next);
             if (ect->category) eina_stringshare_del(ect->category);
             if (ect->file) eina_stringshare_del(ect->file);
             free(ect);
             return 1;
          }
     }
   return 1;
}

EAPI Eina_List *
e_theme_config_list(void)
{
   return e_config->themes;
}

EAPI int
e_theme_category_find(const char *category)
{
   if (eina_list_search_sorted(categories, EINA_COMPARE_CB(strcmp), category))
     return 1;
   return 0;
}

EAPI Eina_List *
e_theme_category_list(void)
{
   return categories;
}

EAPI int
e_theme_transition_find(const char *transition)
{
   Eina_List *trans = NULL;
   int found = 0;
   const char *str;

   trans =
     _e_theme_collection_items_find("base/theme/transitions", "e/transitions");

   if (eina_list_search_sorted(trans, EINA_COMPARE_CB(strcmp), transition))
     found = 1;

   EINA_LIST_FREE(trans, str)
     eina_stringshare_del(str);

   return found;
}

EAPI Eina_List *
e_theme_transition_list(void)
{
   return _e_theme_collection_items_find("base/theme/transitions",
                                         "e/transitions");
}

EAPI int
e_theme_border_find(const char *border)
{
   Eina_List *bds = NULL;
   int found = 0;
   const char *str;

   bds =
     _e_theme_collection_items_find("base/theme/borders", "e/widgets/border");

   if (eina_list_search_sorted(bds, EINA_COMPARE_CB(strcmp), border))
     found = 1;

   EINA_LIST_FREE(bds, str)
     eina_stringshare_del(str);

   return found;
}

EAPI Eina_List *
e_theme_border_list(void)
{
   return _e_theme_collection_items_find("base/theme/borders",
                                         "e/widgets/border");
}

EAPI int
e_theme_shelf_find(const char *shelf)
{
   Eina_List *shelfs = NULL;
   int found = 0;
   const char *str;

   shelfs =
     _e_theme_collection_items_find("base/theme/shelf", "e/shelf");

   if (eina_list_search_sorted(shelfs, EINA_COMPARE_CB(strcmp), shelf))
     found = 1;

   EINA_LIST_FREE(shelfs, str)
     eina_stringshare_del(str);

   return found;
}

EAPI Eina_List *
e_theme_shelf_list(void)
{
   return _e_theme_collection_items_find("base/theme/shelf", "e/shelf");
}

EAPI int
e_theme_comp_find(const char *comp)
{
   Eina_List *comps = NULL;
   int found = 0;
   const char *str;

   comps = _e_theme_collection_items_find("base/theme/borders", "e/comp");

   if (eina_list_search_sorted(comps, EINA_COMPARE_CB(strcmp), comp))
     found = 1;

   EINA_LIST_FREE(comps, str)
     eina_stringshare_del(str);

   return found;
}

EAPI Eina_List *
e_theme_comp_list(void)
{
   return _e_theme_collection_items_find("base/theme/borders", "e/comp");
}

/* local subsystem functions */
static void
e_theme_handler_set(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *path)
{
   E_Action *a;
   char buf[PATH_MAX];
   int copy = 1;

   if (!path) return;

   /* if not in system dir or user dir, copy to user dir */
   e_prefix_data_concat_static(buf, "data/themes");
   if (!strncmp(buf, path, strlen(buf)))
     copy = 0;
   if (copy)
     {
        e_user_dir_concat_static(buf, "themes");
        if (!strncmp(buf, path, strlen(buf)))
          copy = 0;
     }
   if (copy)
     {
        const char *file;
        char *name;

        file = ecore_file_file_get(path);
        name = ecore_file_strip_ext(file);

        e_user_dir_snprintf(buf, sizeof(buf), "themes/%s-%f.edj", name, ecore_time_unix_get());
        free(name);

        if (!ecore_file_exists(buf))
          {
             ecore_file_cp(path, buf);
             e_theme_config_set("theme", buf);
          }
        else
          e_theme_config_set("theme", path);
     }
   else
     e_theme_config_set("theme", path);

   e_config_save_queue();
   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

static int
e_theme_handler_test(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *path)
{
   if (!path) return 0;
   if (!edje_file_group_exists(path, "e/widgets/border/default/border"))
     return 0;
   return 1;
}

static Eina_Bool
_e_theme_mappings_free_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__)
{
   E_Theme_Result *res;

   res = data;
   if (res->file) eina_stringshare_del(res->file);
   if (res->cache) eina_stringshare_del(res->cache);
   if (res->quickfind)
     {
        eina_hash_foreach(res->quickfind, _e_theme_mappings_quickfind_free_cb, NULL);
        eina_hash_free(res->quickfind);
     }
   free(res);
   return EINA_TRUE;
}

static Eina_Bool
_e_theme_mappings_quickfind_free_cb(const Eina_Hash *hash __UNUSED__, const void *key, void *data __UNUSED__, void *fdata __UNUSED__)
{
   eina_stringshare_del(key);
   return EINA_TRUE;
}

static void
_e_theme_category_register(const char *category)
{
   Eina_List *l;
   int ret;

   if (!categories)
     categories = eina_list_append(categories, eina_stringshare_add(category));

   l = eina_list_search_sorted_near_list(categories, EINA_COMPARE_CB(strcmp),
                                         category, &ret);

   if (!ret) return;

   if (ret < 0)
     categories = eina_list_append_relative_list(categories, eina_stringshare_add(category), l);
   else
     categories = eina_list_prepend_relative_list(categories, eina_stringshare_add(category), l);
}

static Eina_List *
_e_theme_collection_item_register(Eina_List *list, const char *name)
{
   const char *item;
   Eina_List *l;

   EINA_LIST_FOREACH(list, l, item)
     {
        if (!strcmp(name, item)) return list;
     }
   list = eina_list_append(list, eina_stringshare_add(name));
   return list;
}

static Eina_List *
_e_theme_collection_items_find(const char *base, const char *collname)
{
   Eina_List *list = NULL;
   E_Theme_Result *res;
   char *category, *p;

   category = alloca(strlen(base) + 1);
   strcpy(category, base);
   do
     {
        res = eina_hash_find(mappings, category);
        if (res)
          {
             const char *str;

             /* if found check cached path */
             str = res->cache;
             if (!str)
               {
                  /* no cached path */
                  str = res->file;
                  /* if its not an absolute path find it */
                  if (str[0] != '/') str = e_path_find(path_themes, str);
                  /* save cached value */
                  if (str) res->cache = str;
               }
             if (str)
               {
                  Eina_List *coll, *l;

                  coll = edje_file_collection_list(str);
                  if (coll)
                    {
                       const char *c;
                       int collname_len;

                       collname_len = strlen(collname);
                       EINA_LIST_FOREACH(coll, l, c)
                         {
                            if (!strncmp(c, collname, collname_len))
                              {
                                 char *trans, *p2;

                                 trans = strdup(c);
                                 p = trans + collname_len + 1;
                                 if (*p)
                                   {
                                      p2 = strchr(p, '/');
                                      if (p2) *p2 = 0;
                                      list = _e_theme_collection_item_register(list, p);
                                   }
                                 free(trans);
                              }
                         }
                       edje_file_collection_list_free(coll);
                    }
               }
          }
        p = strrchr(category, '/');
        if (p) *p = 0;
     }
   while (p);

   list = eina_list_sort(list, 0, EINA_COMPARE_CB(strcmp));
   return list;
}

