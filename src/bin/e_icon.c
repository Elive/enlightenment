#include "e.h"

//#define USE_ICON_CACHE

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _Cache_Item   Cache_Item;
typedef struct _Cache        Cache;

struct _E_Smart_Data
{
   Evas_Coord    x, y, w, h;
   Evas_Object  *obj;
   Evas_Object  *eventarea;
   const char   *fdo;
   Ecore_Timer  *guessing_animation;
   Ecore_Timer  *timer, *fdo_reload_timer;
#ifdef USE_ICON_CACHE
   const char   *file;
   Cache_Item   *ci;
#endif
   double        last_resize;
   int           size;
   int           frame, frame_count;
   unsigned char fill_inside : 1;
   unsigned char scale_up : 1;
   unsigned char preload : 1;
   unsigned char loading : 1;
   unsigned char animated : 1;
   Eina_Bool edje : 1;
};

struct _Cache_Item
{
   unsigned int timestamp;

   Evas_Object *icon, *obj;
   const char  *id;
   Eina_List   *objs;
};

struct _Cache
{
   Eina_Hash   *hash;

   char        *file;
   Eet_File    *ef;
   Ecore_Timer *timer;
   Eina_List   *load_queue;
};

/* local subsystem functions */
static void      _e_icon_smart_reconfigure(E_Smart_Data *sd);
static void      _e_icon_smart_init(void);
static void      _e_icon_smart_add(Evas_Object *obj);
static void      _e_icon_smart_del(Evas_Object *obj);
static void      _e_icon_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void      _e_icon_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void      _e_icon_smart_show(Evas_Object *obj);
static void      _e_icon_smart_hide(Evas_Object *obj);
static void      _e_icon_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void      _e_icon_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void      _e_icon_smart_clip_unset(Evas_Object *obj);
static void      _e_icon_obj_prepare(Evas_Object *obj, E_Smart_Data *sd);
static void      _e_icon_preloaded(void *data, Evas *e, Evas_Object *obj, void *event_info);

#ifdef USE_ICON_CACHE
static Eina_Bool _e_icon_cache_find(Evas_Object *o, const char *file);
static void      _e_icon_cache_icon_loaded(Cache_Item *ci);
static void      _e_icon_cache_icon_try_next(Cache_Item *ci);
static void      _e_icon_cache_item_free(void *data);
static void      _e_icon_obj_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
#endif

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

#ifdef USE_ICON_CACHE
static Cache *_cache = NULL;
static E_Config_DD *cache_edd = NULL;
static E_Config_DD *cache_item_edd = NULL;
#define DBG(...)
#endif

EINTERN int
e_icon_init(void)
{
#ifdef USE_ICON_CACHE

   Eet_File *ef;
   char buf[PATH_MAX];

#undef T
#undef D
#define T Cache_Item
#define D cache_item_edd
   D = E_CONFIG_DD_NEW("Cache_Item", T);
   E_CONFIG_VAL(D, T, timestamp, UINT);
#undef T
#undef D
#define T Cache
#define D cache_edd
   D = E_CONFIG_DD_NEW("Cache", T);
   E_CONFIG_HASH(D, T, hash, cache_item_edd);
#undef T
#undef D

   e_user_dir_concat_static(buf, "icon_cache.eet");

   ef = eet_open(buf, EET_FILE_MODE_READ_WRITE);
   if (!ef) return 1;  /* not critical */

   _cache = eet_data_read(ef, cache_edd, "idx");
   if (!_cache)
     _cache = E_NEW(Cache, 1);

   if (!_cache->hash)
     _cache->hash = eina_hash_string_superfast_new(_e_icon_cache_item_free);

   eet_close(ef);

   _cache->file = strdup(buf);

   _cache->ef = NULL;
#endif
   return 1;
}

EINTERN int
e_icon_shutdown(void)
{
#ifdef USE_ICON_CACHE
   if (_cache)
     {
        E_FREE(_cache->file);

        if (_cache->ef)
          eet_close(_cache->ef);

        if (_cache->load_queue)
          eina_list_free(_cache->load_queue);

        eina_hash_free(_cache->hash);
        E_FREE(_cache);
     }

   E_CONFIG_DD_FREE(cache_item_edd);
   E_CONFIG_DD_FREE(cache_edd);
#endif

   return 1;
}

/* externally accessible functions */
EAPI Evas_Object *
e_icon_add(Evas *evas)
{
   _e_icon_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

static void
_e_icon_obj_prepare(Evas_Object *obj, E_Smart_Data *sd)
{
   if (!sd->obj) return;

   if (sd->edje)
     {
        Evas_Object *pclip;

        pclip = evas_object_clip_get(sd->obj);
        evas_object_del(sd->obj);
#ifdef USE_ICON_CACHE
        sd->ci = NULL;
        eina_stringshare_replace(&sd->file, NULL);
#endif
        sd->obj = evas_object_image_add(evas_object_evas_get(obj));
        if (!sd->animated)
          evas_object_image_scale_hint_set(sd->obj,
                                           EVAS_IMAGE_SCALE_HINT_STATIC);
        evas_object_smart_member_add(sd->obj, obj);
        evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_IMAGE_PRELOADED,
                                       _e_icon_preloaded, obj);
        evas_object_clip_set(sd->obj, pclip);
     }
}

static Eina_Bool
_frame_anim(void *data)
{
   E_Smart_Data *sd = data;
   double t;
   int fr;

   sd->frame++;
   fr = (sd->frame % (sd->frame_count)) + 1;
   evas_object_image_animated_frame_set(sd->obj, fr);
   t = evas_object_image_animated_frame_duration_get(sd->obj, fr, 0);
   sd->timer = ecore_timer_add(t, _frame_anim, sd);
   return EINA_FALSE;
}

static int
_handle_anim(E_Smart_Data *sd)
{
   double t;

   if (sd->timer) ecore_timer_del(sd->timer);
   sd->timer = NULL;
   sd->frame = 0;
   sd->frame_count = 0;
   if (!evas_object_image_animated_get(sd->obj)) return 0;
   // FIXME: hack around jiyouns BUG!!!!!!!!
   {
      const char *file;
      char buf[256];
      snprintf(buf, sizeof(buf), "%ld", (long)sd);
      evas_object_image_file_get(sd->obj, &file, NULL);
      evas_object_image_file_set(sd->obj, file, buf);
   }
   sd->frame_count = evas_object_image_animated_frame_count_get(sd->obj);
   if (sd->frame_count < 2) return 0;
   evas_object_show(sd->obj);
   t = evas_object_image_animated_frame_duration_get(sd->obj, sd->frame, 0);
   sd->timer = ecore_timer_add(t, _frame_anim, sd);
   return 1;
}

EAPI Eina_Bool
e_icon_file_set(Evas_Object *obj, const char *file)
{
   E_Smart_Data *sd;
   int len;

   if (!file) return EINA_FALSE;
   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(0);
   if (!(sd = evas_object_smart_data_get(obj)))
     return EINA_FALSE;

   len = strlen(file);
   if ((len > 4) && (!strcasecmp(file + len - 4, ".edj")))
     return e_icon_file_edje_set(obj, file, "icon");

   /* smart code here */
   _e_icon_obj_prepare(obj, sd);
   /* FIXME: 64x64 - unhappy about this. use icon size */
   sd->loading = 0;
   if (sd->fdo)
     {
        eina_stringshare_del(sd->fdo);
        sd->fdo = NULL;
     }

   if (sd->timer) ecore_timer_del(sd->timer);
   sd->timer = NULL;
   if (sd->guessing_animation) ecore_timer_del(sd->guessing_animation);
   sd->guessing_animation = NULL;
   sd->frame = 0;
   sd->frame_count = 0;
   sd->edje = EINA_FALSE;

   if (sd->size != 0)
     evas_object_image_load_size_set(sd->obj, sd->size, sd->size);
   if (sd->preload) evas_object_hide(sd->obj);

#ifdef USE_ICON_CACHE
   if (_e_icon_cache_find(obj, file))
     {
        _e_icon_smart_reconfigure(sd);
        return EINA_TRUE;
     }
#endif

   evas_object_image_file_set(sd->obj, file, NULL);
   if (evas_object_image_load_error_get(sd->obj) != EVAS_LOAD_ERROR_NONE)
     return EINA_FALSE;

   if (!_handle_anim(sd))
     {
        if (sd->preload)
          {
             sd->loading = 1;
             evas_object_image_preload(sd->obj, EINA_FALSE);
          }
        else if (evas_object_visible_get(obj))
          {
             evas_object_show(sd->obj);
#ifdef USE_ICON_CACHE
             _e_icon_cache_icon_loaded(sd->ci);
#endif
          }
     }
#ifdef USE_ICON_CACHE
   else
     {
        evas_object_event_callback_del_full(sd->obj, EVAS_CALLBACK_DEL,
                                            _e_icon_obj_del, obj);
        _cache->load_queue = eina_list_remove(_cache->load_queue, sd->ci);
        eina_stringshare_del(sd->ci->id);
        E_FREE(sd->ci);
        sd->ci = NULL;
     }
#endif

   _e_icon_smart_reconfigure(sd);
   return EINA_TRUE;
}

EAPI Eina_Bool
e_icon_file_key_set(Evas_Object *obj, const char *file, const char *key)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(0);
   if (!(sd = evas_object_smart_data_get(obj)))
     return EINA_FALSE;

   /* smart code here */
   sd->loading = 0;
   if (sd->fdo)
     {
        eina_stringshare_del(sd->fdo);
        sd->fdo = NULL;
     }

   if (sd->timer) ecore_timer_del(sd->timer);
   sd->timer = NULL;
   if (sd->guessing_animation) ecore_timer_del(sd->guessing_animation);
   sd->guessing_animation = NULL;
   sd->frame = 0;
   sd->frame_count = 0;
   sd->edje = EINA_FALSE;

   _e_icon_obj_prepare(obj, sd);
   if (sd->size != 0)
     evas_object_image_load_size_set(sd->obj, sd->size, sd->size);
   if (sd->preload) evas_object_hide(sd->obj);
   evas_object_image_file_set(sd->obj, file, key);
   if (evas_object_image_load_error_get(sd->obj) != EVAS_LOAD_ERROR_NONE)
     return EINA_FALSE;
   if (!_handle_anim(sd))
     {
        if (sd->preload)
          {
             sd->loading = 1;
             evas_object_image_preload(sd->obj, 0);
          }
        else if (evas_object_visible_get(obj))
          evas_object_show(sd->obj);
     }
   _e_icon_smart_reconfigure(sd);
   return EINA_TRUE;
}

EAPI void
e_icon_edje_object_set(Evas_Object *obj, Evas_Object *edje)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR();
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* smart code here */
   if (sd->obj) evas_object_del(sd->obj);
   sd->loading = 0;
   if (sd->fdo)
     {
        eina_stringshare_del(sd->fdo);
        sd->fdo = NULL;
     }

   if (sd->timer) ecore_timer_del(sd->timer);
   sd->timer = NULL;
   if (sd->guessing_animation) ecore_timer_del(sd->guessing_animation);
   sd->guessing_animation = NULL;
   sd->frame = 0;
   sd->frame_count = 0;
   sd->edje = EINA_TRUE;
   sd->obj = edje;

   if (evas_object_visible_get(obj)) evas_object_show(sd->obj);
   evas_object_smart_member_add(sd->obj, obj);
   _e_icon_smart_reconfigure(sd);
}

EAPI Eina_Bool
e_icon_file_edje_set(Evas_Object *obj, const char *file, const char *part)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(0);
   if (!(sd = evas_object_smart_data_get(obj)))
     return EINA_FALSE;

   /* smart code here */
   if (sd->obj) evas_object_del(sd->obj);
   sd->loading = 0;
   if (sd->fdo)
     {
        eina_stringshare_del(sd->fdo);
        sd->fdo = NULL;
     }

   if (sd->timer) ecore_timer_del(sd->timer);
   sd->timer = NULL;
   if (sd->guessing_animation) ecore_timer_del(sd->guessing_animation);
   sd->guessing_animation = NULL;
   sd->frame = 0;
   sd->frame_count = 0;
   sd->edje = EINA_TRUE;

   sd->obj = edje_object_add(evas_object_evas_get(obj));
   edje_object_file_set(sd->obj, file, part);
   if (edje_object_load_error_get(sd->obj) != EDJE_LOAD_ERROR_NONE)
     return EINA_FALSE;
   if (evas_object_visible_get(obj)) evas_object_show(sd->obj);
   evas_object_smart_member_add(sd->obj, obj);
   _e_icon_smart_reconfigure(sd);
   return EINA_TRUE;
}

EAPI Eina_Bool
e_icon_fdo_icon_set(Evas_Object *obj, const char *icon)
{
   E_Smart_Data *sd;
   const char *path;
   int len;

   if (!icon) return EINA_FALSE;
   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(0);
   if (icon[0] == '/') return e_icon_file_set(obj, icon);

   if (!(sd = evas_object_smart_data_get(obj)))
     return EINA_FALSE;

   if (sd->timer) ecore_timer_del(sd->timer);
   sd->timer = NULL;
   if (sd->guessing_animation) ecore_timer_del(sd->guessing_animation);
   sd->guessing_animation = NULL;
   sd->frame = 0;
   sd->frame_count = 0;
   sd->edje = EINA_FALSE;

   eina_stringshare_replace(&sd->fdo, icon);
   if (!sd->fdo) return EINA_FALSE;

   path = efreet_icon_path_find(e_config->icon_theme, sd->fdo, sd->size);
   if (!path)
     {
        if (e_util_strcmp(e_config->icon_theme, "hicolor"))
          path = efreet_icon_path_find("hicolor", sd->fdo, sd->size);
        if (!path) return EINA_FALSE;
     }

   len = strlen(icon);
   if ((len > 4) && (!strcasecmp(icon + len - 4, ".edj")))
     return e_icon_file_edje_set(obj, path, "icon");

   /* smart code here */
   _e_icon_obj_prepare(obj, sd);
   sd->loading = 0;
   if (sd->size != 0)
     evas_object_image_load_size_set(sd->obj, sd->size, sd->size);
   if (sd->preload) evas_object_hide(sd->obj);
   evas_object_image_file_set(sd->obj, path, NULL);
   if (evas_object_image_load_error_get(sd->obj) != EVAS_LOAD_ERROR_NONE)
     return EINA_FALSE;
   if (sd->preload)
     {
        sd->loading = 1;
        evas_object_image_preload(sd->obj, 0);
     }
   else if (evas_object_visible_get(obj))
     evas_object_show(sd->obj);
   _e_icon_smart_reconfigure(sd);
   return EINA_TRUE;
}

EAPI void
e_icon_object_set(Evas_Object *obj, Evas_Object *o)
{
   E_Smart_Data *sd;
   const char *str;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR();
   if (!(sd = evas_object_smart_data_get(obj))) return;
   str = evas_object_type_get(o);
   if ((!str) || strcmp(str, "image"))
     CRI(EINA_COLOR_RED"******************\ntrying to set an image object of type '%s'! this is not what you want!\n******************\n"EINA_COLOR_RESET, str);

   if (sd->timer) ecore_timer_del(sd->timer);
   sd->timer = NULL;
   if (sd->guessing_animation) ecore_timer_del(sd->guessing_animation);
   sd->guessing_animation = NULL;
   sd->frame = 0;
   sd->frame_count = 0;
   sd->edje = EINA_FALSE;

   /* smart code here */
   if (sd->obj) evas_object_del(sd->obj);
   sd->loading = 0;
   sd->obj = o;
   evas_object_smart_member_add(sd->obj, obj);
   if (evas_object_visible_get(obj)) evas_object_show(sd->obj);
   _e_icon_smart_reconfigure(sd);
}

EAPI Eina_Bool
e_icon_file_get(const Evas_Object *obj, const char **file, const char **group)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(EINA_FALSE);
   if ((!file) && (!group)) return EINA_FALSE;
   if (file) *file = NULL;
   if (group) *group = NULL;
   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;
#ifdef USE_ICON_CACHE
   if (sd->file)
     {
         if (file) *file = sd->file;
         return EINA_TRUE;
     }
#endif
   if (sd->edje)
     {
        edje_object_file_get(sd->obj, file, group);
        return file || group;
     }
   evas_object_image_file_get(sd->obj, file, group);
   return file || group;
}

EAPI void
e_icon_smooth_scale_set(Evas_Object *obj, Eina_Bool smooth)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   if (!(sd = evas_object_smart_data_get(obj))) return;
   if (sd->edje) return;
   evas_object_image_smooth_scale_set(sd->obj, smooth);
}

EAPI Eina_Bool
e_icon_smooth_scale_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(0);
   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;
   if (sd->edje)
     return EINA_FALSE;
   return evas_object_image_smooth_scale_get(sd->obj);
}

EAPI void
e_icon_alpha_set(Evas_Object *obj, Eina_Bool alpha)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   if (!(sd = evas_object_smart_data_get(obj))) return;
   if (sd->edje) return;
   evas_object_image_alpha_set(sd->obj, alpha);
}

EAPI Eina_Bool
e_icon_alpha_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(0);
   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;
   if (sd->edje) return EINA_FALSE;
   return evas_object_image_alpha_get(sd->obj);
}

EAPI void
e_icon_preload_set(Evas_Object *obj, Eina_Bool preload)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   if (!(sd = evas_object_smart_data_get(obj))) return;
   sd->preload = preload;
}

EAPI Eina_Bool
e_icon_preload_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(0);
   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;
   return sd->preload;
}

EAPI void
e_icon_size_get(const Evas_Object *obj, int *w, int *h)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   if (!(sd = evas_object_smart_data_get(obj)))
     {
        if (w) *w = 0;
        if (h) *h = 0;
        return;
     }
   if (sd->edje)
     edje_object_size_min_calc(sd->obj, w, h);
   else
     evas_object_image_size_get(sd->obj, w, h);
}

EAPI Eina_Bool
e_icon_fill_inside_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(0);
   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;
   return sd->fill_inside;
}

EAPI void
e_icon_fill_inside_set(Evas_Object *obj, Eina_Bool fill_inside)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   if (!(sd = evas_object_smart_data_get(obj))) return;
   fill_inside = !!fill_inside;
   if (sd->fill_inside == fill_inside) return;
   sd->fill_inside = fill_inside;
   _e_icon_smart_reconfigure(sd);
}

EAPI Eina_Bool
e_icon_scale_up_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(0);
   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;
   return sd->scale_up;
}

EAPI void
e_icon_scale_up_set(Evas_Object *obj, Eina_Bool scale_up)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   if (!(sd = evas_object_smart_data_get(obj))) return;
   scale_up = !!scale_up;
   if (sd->scale_up == scale_up) return;
   sd->scale_up = scale_up;
   _e_icon_smart_reconfigure(sd);
}

EAPI void
e_icon_data_set(Evas_Object *obj, void *data, int w, int h)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   if (!(sd = evas_object_smart_data_get(obj))) return;
   if (sd->edje) return;
   evas_object_image_size_set(sd->obj, w, h);
   evas_object_image_data_copy_set(sd->obj, data);
}

EAPI void *
e_icon_data_get(const Evas_Object *obj, int *w, int *h)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(NULL);
   if (!(sd = evas_object_smart_data_get(obj))) return NULL;
   if (sd->edje) return NULL;
   evas_object_image_size_get(sd->obj, w, h);
   return evas_object_image_data_get(sd->obj, 0);
}

EAPI void
e_icon_scale_size_set(Evas_Object *obj, int size)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   if (!(sd = evas_object_smart_data_get(obj))) return;
   sd->size = size;
   if (sd->edje) return;
   evas_object_image_load_size_set(sd->obj, sd->size, sd->size);
}

EAPI int
e_icon_scale_size_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(0);
   if (!(sd = evas_object_smart_data_get(obj))) return 0;
   return sd->size;
}

EAPI void
e_icon_selected_set(const Evas_Object *obj, Eina_Bool selected)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   if (!(sd = evas_object_smart_data_get(obj))) return;
   if (!sd->edje) return;
   if (selected)
     edje_object_signal_emit(sd->obj, "e,state,selected", "e");
   else
     edje_object_signal_emit(sd->obj, "e,state,unselected", "e");
}

/* local subsystem globals */
static void
_e_icon_smart_reconfigure(E_Smart_Data *sd)
{
   int iw, ih;
   Evas_Coord x, y, w, h;

   if (!sd->obj) return;
   if (sd->edje)
     {
        w = sd->w;
        h = sd->h;
        x = sd->x;
        y = sd->y;
        evas_object_move(sd->obj, x, y);
        evas_object_resize(sd->obj, w, h);
        evas_object_move(sd->eventarea, x, y);
        evas_object_resize(sd->eventarea, w, h);
     }
   else
     {
        ih = 0;
        ih = 0;
        evas_object_image_size_get(sd->obj, &iw, &ih);
        if (iw < 1) iw = 1;
        if (ih < 1) ih = 1;

        if (sd->fill_inside)
          {
             w = sd->w;
             h = ((double)ih * w) / (double)iw;
             if (h > sd->h)
               {
                  h = sd->h;
                  w = ((double)iw * h) / (double)ih;
               }
          }
        else
          {
             w = sd->w;
             h = ((double)ih * w) / (double)iw;
             if (h < sd->h)
               {
                  h = sd->h;
                  w = ((double)iw * h) / (double)ih;
               }
          }
        if (!sd->scale_up)
          {
             if ((w > iw) || (h > ih))
               {
                  w = iw;
                  h = ih;
               }
          }
        x = sd->x + ((sd->w - w) / 2);
        y = sd->y + ((sd->h - h) / 2);
        evas_object_move(sd->obj, x, y);
        evas_object_image_fill_set(sd->obj, 0, 0, w, h);
        evas_object_resize(sd->obj, w, h);
        evas_object_move(sd->eventarea, x, y);
        evas_object_resize(sd->eventarea, w, h);
     }
}

static void
_e_icon_smart_init(void)
{
   if (_e_smart) return;
   {
      static Evas_Smart_Class sc = EVAS_SMART_CLASS_INIT_NAME_VERSION("e_icon");
      if (!sc.add)
        {
           sc.add = _e_icon_smart_add;
           sc.del = _e_icon_smart_del;
           sc.move = _e_icon_smart_move;
           sc.resize = _e_icon_smart_resize;
           sc.show = _e_icon_smart_show;
           sc.hide = _e_icon_smart_hide;
           sc.color_set = _e_icon_smart_color_set;
           sc.clip_set = _e_icon_smart_clip_set;
           sc.clip_unset = _e_icon_smart_clip_unset;
        }
      _e_smart = evas_smart_class_new(&sc);
   }
}

static void
_e_icon_preloaded(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(data))) return;

   evas_object_smart_callback_call(data, "preloaded", NULL);
   evas_object_show(sd->obj);
   sd->loading = 0;

#ifdef USE_ICON_CACHE
   _e_icon_cache_icon_loaded(sd->ci);
#endif
}

static void
_e_icon_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = calloc(1, sizeof(E_Smart_Data)))) return;
   sd->eventarea = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_color_set(sd->eventarea, 0, 0, 0, 0);
   evas_object_smart_member_add(sd->eventarea, obj);

   sd->obj = evas_object_image_add(evas_object_evas_get(obj));
   evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_IMAGE_PRELOADED,
                                  _e_icon_preloaded, obj);
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   sd->fill_inside = 1;
   sd->scale_up = 1;
   sd->size = 64;
   evas_object_smart_member_add(sd->obj, obj);
   evas_object_smart_data_set(obj, sd);
}

static void
_e_icon_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   evas_object_del(sd->obj);
   evas_object_del(sd->eventarea);
   if (sd->fdo) eina_stringshare_del(sd->fdo);
#ifdef USE_ICON_CACHE
   if (sd->file) eina_stringshare_del(sd->file);
#endif
   if (sd->fdo_reload_timer) ecore_timer_del(sd->fdo_reload_timer);
   if (sd->timer) ecore_timer_del(sd->timer);
   if (sd->guessing_animation) ecore_timer_del(sd->guessing_animation);
   evas_object_smart_data_set(obj, NULL);
   memset(sd, 0, sizeof(*sd));
   free(sd);
}

static void
_e_icon_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   if ((sd->x == x) && (sd->y == y)) return;
   sd->x = x;
   sd->y = y;
   _e_icon_smart_reconfigure(sd);
}

static Eina_Bool
_e_icon_guess_anim(void *data)
{
   E_Smart_Data *sd = data;
   double t = ecore_loop_time_get();

   if (t - sd->last_resize < 0.2)
     {
        evas_object_image_scale_hint_set(sd->obj,
                                         EVAS_IMAGE_SCALE_HINT_DYNAMIC);
        sd->animated = EINA_TRUE;
     }
   else
     {
        evas_object_image_scale_hint_set(sd->obj,
                                         EVAS_IMAGE_SCALE_HINT_STATIC);
     }

   sd->guessing_animation = NULL;
   return EINA_FALSE;
}

static Eina_Bool
_e_icon_fdo_reload(void *data)
{
   E_Smart_Data *sd = data;
   const char *path;

   sd->fdo_reload_timer = NULL;
   sd->size = MAX(sd->w, sd->h);
   path = efreet_icon_path_find(e_config->icon_theme, sd->fdo, sd->size);
   if (!path)
     {
        if (e_util_strcmp(e_config->icon_theme, "hicolor"))
          path = efreet_icon_path_find("hicolor", sd->fdo, sd->size);
        if (!path) return EINA_FALSE;
     }

   /* smart code here */
   evas_object_image_load_size_set(sd->obj, sd->size, sd->size);
   evas_object_image_file_set(sd->obj, path, NULL);
   if (sd->preload)
     {
        sd->loading = 1;
        evas_object_image_preload(sd->obj, 0);
     }
   return EINA_FALSE;
}

static void
_e_icon_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   if ((sd->w == w) && (sd->h == h)) return;
   sd->w = w;
   sd->h = h;
   if (sd->fdo)
     {
        if (sd->fdo_reload_timer) ecore_timer_del(sd->fdo_reload_timer);
        sd->fdo_reload_timer = ecore_timer_add(0.1, _e_icon_fdo_reload, sd);
     }

   if ((!sd->edje) && ((sd->loading && sd->preload) ||
        (!sd->loading && !sd->preload))
       && !sd->animated)
     {
        evas_object_image_scale_hint_set(sd->obj,
                                         EVAS_IMAGE_SCALE_HINT_DYNAMIC);
        if (!sd->guessing_animation)
          sd->guessing_animation = ecore_timer_add(0.3,
                                                   _e_icon_guess_anim,
                                                   sd);
     }

   sd->last_resize = ecore_loop_time_get();
   _e_icon_smart_reconfigure(sd);
}

static void
_e_icon_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   if (!((sd->preload) && (sd->loading)))
     {
        evas_object_show(sd->obj);
#ifdef USE_ICON_CACHE
        if (!sd->preload)
          _e_icon_cache_icon_loaded(sd->ci);
#endif
     }

   evas_object_show(sd->eventarea);
}

static void
_e_icon_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   evas_object_hide(sd->obj);
   evas_object_hide(sd->eventarea);
}

static void
_e_icon_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   evas_object_color_set(sd->obj, r, g, b, a);
}

static void
_e_icon_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   evas_object_clip_set(sd->obj, clip);
   evas_object_clip_set(sd->eventarea, clip);
}

static void
_e_icon_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   evas_object_clip_unset(sd->obj);
   evas_object_clip_unset(sd->eventarea);
}

#ifdef USE_ICON_CACHE

static void
_e_icon_cache_item_free(void *data)
{
   Cache_Item *ci = data;
   eina_stringshare_del(ci->id);
   E_FREE(ci);
}

static Eina_Bool
_e_icon_cache_save(void *data)
{
   if (_cache->load_queue)
     {
        Cache_Item *ci;
        Eina_List *l;

        /* EINA_LIST_FOREACH(_cache->load_queue, l, ci)
         *   printf("  : %s\n", ci->id); */

        return ECORE_CALLBACK_RENEW;
     }

   eet_sync(_cache->ef);
   eet_close(_cache->ef);

   _cache->ef = NULL;
   _cache->timer = NULL;

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_icon_cache_find(Evas_Object *obj, const char *file)
{
   E_Smart_Data *sd;
   Cache_Item *ci;
   char buf[4096];
   const char *id;
   Eina_List *l;

   if (!_cache) return EINA_FALSE;

   if (!(sd = evas_object_smart_data_get(obj)))
     return EINA_FALSE;

   snprintf(buf, sizeof(buf), "%d:%s", sd->size, file);

   if ((ci = eina_hash_find(_cache->hash, buf)))
     {
        unsigned int w, h, alpha;
        void *data;
        int found = 0;

        if (!_cache->ef)
          _cache->ef = eet_open(_cache->file, EET_FILE_MODE_READ_WRITE);

        if (_cache->ef && (data = eet_data_image_read(_cache->ef, buf,
                                                      &w, &h, &alpha,
                                                      NULL, NULL, NULL)))
          {
             evas_object_image_size_set(sd->obj, w, h);
             evas_object_image_alpha_set(sd->obj, alpha);
             evas_object_image_data_copy_set(sd->obj, data);
             evas_object_smart_callback_call(obj, "preloaded", NULL);
             evas_object_show(sd->obj);
             free(data);
             found = 1;
          }

        if ((_cache->ef) && !(_cache->timer))
          _cache->timer = ecore_timer_add(3.0, _e_icon_cache_save, NULL);

        if (found)
          return EINA_TRUE;

        eina_hash_del_by_key(_cache->hash, ci->id);
        ci = NULL;
     }

   id = eina_stringshare_add(buf);

   /* not found in cache, check load queue */
   EINA_LIST_FOREACH(_cache->load_queue, l, ci)
     {
        if (ci->id != id) continue;
        ci->objs = eina_list_append(ci->objs, obj);
        sd->ci = ci;
        evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_DEL,
                                       _e_icon_obj_del, obj);
        eina_stringshare_del(id);
        return EINA_TRUE;
     }

   ci = E_NEW(Cache_Item, 1);
   ci->id = id;
   ci->icon = sd->obj;
   ci->obj = obj;
   sd->ci = ci;
   sd->file = eina_stringshare_add(file);

   evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_DEL, _e_icon_obj_del, obj);

   _cache->load_queue = eina_list_append(_cache->load_queue, ci);

   return EINA_FALSE;
}

static void
_e_icon_cache_icon_try_next(Cache_Item *ci)
{
   Evas_Object *obj;
   E_Smart_Data *sd;

   if (!ci->objs)
     {
        /* no more e_icon wait for this object to bet loaded */
        _cache->load_queue = eina_list_remove(_cache->load_queue, ci);
        _e_icon_cache_item_free(ci);
        return;
     }

   obj = eina_list_data_get(ci->objs);
   ci->objs = eina_list_remove_list(ci->objs, ci->objs);

   if (!obj)
     goto __try_next;

   if (!(sd = evas_object_smart_data_get(obj)))
     goto __try_next;

   evas_object_image_file_set(sd->obj, sd->file, NULL);
   if (evas_object_image_load_error_get(sd->obj) != EVAS_LOAD_ERROR_NONE)
     goto __try_next;

   sd->ci->icon = sd->obj;
   sd->ci->obj = obj;
   evas_object_image_preload(sd->obj, EINA_FALSE);
   return;

__try_next:
   evas_object_event_callback_del_full(sd->obj, EVAS_CALLBACK_DEL,
                                       _e_icon_obj_del, obj);
   _e_icon_cache_icon_try_next(ci);
}

static void
_e_icon_obj_del(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(data))) return;
   if (!sd->ci) return;

   /* check if the deleted object is the one that is used for
      preloading.  when other objs wait for this image data start
      preloading again with the next. */

   if (sd->ci->icon == obj)
     {
        _e_icon_cache_icon_try_next(sd->ci);
        sd->ci = NULL;
     }
   else
     {
        sd->ci->objs = eina_list_remove(sd->ci->objs, data);
        sd->ci = NULL;
     }
}

static void
_e_icon_cache_icon_loaded(Cache_Item *ci)
{
   int w, h, alpha;
   E_Smart_Data *sd;
   Evas_Object *obj;
   void *data;

   if (!_cache) return;

   if (!ci || !ci->id) return;
   _cache->load_queue = eina_list_remove(_cache->load_queue, ci);

   data = evas_object_image_data_get(ci->icon, EINA_FALSE);
   evas_object_image_size_get(ci->icon, &w, &h);
   alpha = evas_object_image_alpha_get(ci->icon);

   evas_object_event_callback_del_full(ci->icon, EVAS_CALLBACK_DEL,
                                       _e_icon_obj_del, ci->obj);
   evas_object_smart_callback_call(ci->obj, "preloaded", NULL);

   DBG("icon loaded %p, %s\n", data, ci->id);

   sd = evas_object_smart_data_get(ci->obj);
   sd->ci = NULL;

   /* pass loaded data to other e_icon wating for this */
   EINA_LIST_FREE(ci->objs, obj)
     {
        sd = evas_object_smart_data_get(obj);
        sd->ci = NULL;
        evas_object_event_callback_del_full(sd->obj, EVAS_CALLBACK_DEL,
                                            _e_icon_obj_del, obj);
        if (!data) continue;

        evas_object_image_size_set(sd->obj, w, h);
        evas_object_image_alpha_set(sd->obj, alpha);
        evas_object_image_data_copy_set(sd->obj, data);
        evas_object_show(sd->obj);
        evas_object_smart_callback_call(obj, "preloaded", NULL);
     }

   if (data)
     {
        if (!_cache->ef)
          _cache->ef = eet_open(_cache->file, EET_FILE_MODE_READ_WRITE);
        if (_cache->ef && eet_data_image_write(_cache->ef, ci->id, data,
                                               w, h, alpha, 1, 100, 0))
          {
             eina_hash_add(_cache->hash, ci->id, ci);
             eet_data_write(_cache->ef, cache_edd, "idx", _cache, 1);

             if (!_cache->timer)
               _cache->timer = ecore_timer_add(3.0, _e_icon_cache_save, NULL);

             eina_stringshare_replace(&ci->id, NULL);
             return;
          }
     }

   DBG("couldnt write cache %p !!!\n", _cache->ef);
   _e_icon_cache_item_free(ci);
}

#endif
