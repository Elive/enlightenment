#include "e.h"

/* E_Desk is a child object of E_Zone. A desk is essentially a background
 * and an associated set of client windows. Each zone can have an arbitrary
 * number of desktops.
 */

static void      _e_desk_free(E_Desk *desk);
static void      _e_desk_event_desk_show_free(void *data, void *ev);
static void      _e_desk_event_desk_before_show_free(void *data, void *ev);
static void      _e_desk_event_desk_after_show_free(void *data, void *ev);
static void      _e_desk_event_desk_deskshow_free(void *data, void *ev);
static void      _e_desk_event_desk_name_change_free(void *data, void *ev);
static void      _e_desk_show_begin(E_Desk *desk, int mode, int x, int dy);
static void      _e_desk_show_end(E_Desk *desk);
static Eina_Bool _e_desk_show_animator(void *data);
static void      _e_desk_hide_begin(E_Desk *desk, int mode, int dx, int dy);
static void      _e_desk_hide_end(E_Desk *desk);
static Eina_Bool _e_desk_hide_animator(void *data);
#if (ECORE_VERSION_MAJOR > 1) || (ECORE_VERSION_MINOR >= 8)
static void      _e_desk_event_desk_window_profile_change_free(void *data, void *ev);
static void      _e_desk_window_profile_change_protocol_set(void);
#endif

EAPI int E_EVENT_DESK_SHOW = 0;
EAPI int E_EVENT_DESK_BEFORE_SHOW = 0;
EAPI int E_EVENT_DESK_AFTER_SHOW = 0;
EAPI int E_EVENT_DESK_DESKSHOW = 0;
EAPI int E_EVENT_DESK_NAME_CHANGE = 0;
EAPI int E_EVENT_DESK_WINDOW_PROFILE_CHANGE = 0;

EINTERN int
e_desk_init(void)
{
   E_EVENT_DESK_SHOW = ecore_event_type_new();
   E_EVENT_DESK_BEFORE_SHOW = ecore_event_type_new();
   E_EVENT_DESK_AFTER_SHOW = ecore_event_type_new();
   E_EVENT_DESK_DESKSHOW = ecore_event_type_new();
   E_EVENT_DESK_NAME_CHANGE = ecore_event_type_new();
   E_EVENT_DESK_WINDOW_PROFILE_CHANGE = ecore_event_type_new();
   return 1;
}

EINTERN int
e_desk_shutdown(void)
{
   return 1;
}

EAPI E_Desk *
e_desk_new(E_Zone *zone, int x, int y)
{
   E_Desk *desk;
   Eina_List *l;
   E_Config_Desktop_Name *cfname;
#if (ECORE_VERSION_MAJOR > 1) || (ECORE_VERSION_MINOR >= 8)
   E_Config_Desktop_Window_Profile *cfprof;
#endif
   char name[40];
   int ok = 0;

   E_OBJECT_CHECK_RETURN(zone, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, NULL);

   desk = E_OBJECT_ALLOC(E_Desk, E_DESK_TYPE, _e_desk_free);
   if (!desk) return NULL;

   desk->zone = zone;
   desk->x = x;
   desk->y = y;

   /* Get current desktop's name */
   EINA_LIST_FOREACH(e_config->desktop_names, l, cfname)
     {
        if ((cfname->container >= 0) &&
            ((int)zone->container->num != cfname->container)) continue;
        if ((cfname->zone >= 0) &&
            ((int)zone->num != cfname->zone)) continue;
        if ((cfname->desk_x != desk->x) || (cfname->desk_y != desk->y))
          continue;
        desk->name = eina_stringshare_add(cfname->name);
        ok = 1;
        break;
     }

   if (!ok)
     {
        snprintf(name, sizeof(name), _(e_config->desktop_default_name), x, y);
        desk->name = eina_stringshare_add(name);
     }
#if (ECORE_VERSION_MAJOR > 1) || (ECORE_VERSION_MINOR >= 8)
   /* Get window profile name for current desktop */
   ok = 0;
   EINA_LIST_FOREACH(e_config->desktop_window_profiles, l, cfprof)
     {
        if ((cfprof->container >= 0) &&
            ((int)zone->container->num != cfprof->container)) continue;
        if ((cfprof->zone >= 0) &&
            ((int)zone->num != cfprof->zone)) continue;
        if ((cfprof->desk_x != desk->x) || (cfprof->desk_y != desk->y))
          continue;
        desk->window_profile = eina_stringshare_add(cfprof->profile);
        ok = 1;
        break;
     }

   if (!ok)
     {
        desk->window_profile = eina_stringshare_add
          (e_config->desktop_default_window_profile);
     }
#endif
   return desk;
}

EAPI void
e_desk_name_set(E_Desk *desk, const char *name)
{
   E_Event_Desk_Name_Change *ev;

   E_OBJECT_CHECK(desk);
   E_OBJECT_TYPE_CHECK(desk, E_DESK_TYPE);

   eina_stringshare_replace(&desk->name, name);

   ev = E_NEW(E_Event_Desk_Name_Change, 1);
   ev->desk = desk;
   e_object_ref(E_OBJECT(desk));
   ecore_event_add(E_EVENT_DESK_NAME_CHANGE, ev,
                   _e_desk_event_desk_name_change_free, NULL);
}

EAPI void
e_desk_name_add(int container, int zone, int desk_x, int desk_y, const char *name)
{
   E_Config_Desktop_Name *cfname;

   e_desk_name_del(container, zone, desk_x, desk_y);

   cfname = E_NEW(E_Config_Desktop_Name, 1);
   cfname->container = container;
   cfname->zone = zone;
   cfname->desk_x = desk_x;
   cfname->desk_y = desk_y;
   if (name) cfname->name = eina_stringshare_add(name);
   else cfname->name = NULL;
   e_config->desktop_names = eina_list_append(e_config->desktop_names, cfname);
}

EAPI void
e_desk_name_del(int container, int zone, int desk_x, int desk_y)
{
   Eina_List *l = NULL;
   E_Config_Desktop_Name *cfname = NULL;

   EINA_LIST_FOREACH(e_config->desktop_names, l, cfname)
     {
        if ((cfname->container == container) && (cfname->zone == zone) &&
            (cfname->desk_x == desk_x) && (cfname->desk_y == desk_y))
          {
             e_config->desktop_names =
               eina_list_remove_list(e_config->desktop_names, l);
             if (cfname->name) eina_stringshare_del(cfname->name);
             E_FREE(cfname);
             break;
          }
     }
}

EAPI void
e_desk_name_update(void)
{
   Eina_List *m, *c, *z, *l;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   E_Desk *desk;
   E_Config_Desktop_Name *cfname;
   int d_x, d_y, ok;
   char name[40];

   EINA_LIST_FOREACH(e_manager_list(), m, man)
     {
        EINA_LIST_FOREACH(man->containers, c, con)
          {
             EINA_LIST_FOREACH(con->zones, z, zone)
               {
                  for (d_x = 0; d_x < zone->desk_x_count; d_x++)
                    {
                       for (d_y = 0; d_y < zone->desk_y_count; d_y++)
                         {
                            desk = zone->desks[d_x + zone->desk_x_count * d_y];
                            ok = 0;

                            EINA_LIST_FOREACH(e_config->desktop_names, l, cfname)
                              {
                                 if ((cfname->container >= 0) &&
                                     ((int)con->num != cfname->container)) continue;
                                 if ((cfname->zone >= 0) &&
                                     ((int)zone->num != cfname->zone)) continue;
                                 if ((cfname->desk_x != d_x) ||
                                     (cfname->desk_y != d_y)) continue;
                                 e_desk_name_set(desk, cfname->name);
                                 ok = 1;
                                 break;
                              }

                            if (!ok)
                              {
                                 snprintf(name, sizeof(name),
                                          _(e_config->desktop_default_name),
                                          d_x, d_y);
                                 e_desk_name_set(desk, name);
                              }
                         }
                    }
               }
          }
     }
}

EAPI void
e_desk_show(E_Desk *desk)
{
   E_Border_List *bl;
   E_Border *bd;
   E_Event_Desk_Show *ev;
   E_Event_Desk_Before_Show *eev;
   E_Event_Desk_After_Show *eeev;
   Edje_Message_Float_Set *msg;
   Eina_List *l;
   E_Shelf *es;
   int was_zone = 0, x, y, dx = 0, dy = 0;

   E_OBJECT_CHECK(desk);
   E_OBJECT_TYPE_CHECK(desk, E_DESK_TYPE);
   if (desk->visible) return;

   eev = E_NEW(E_Event_Desk_Before_Show, 1);
   eev->desk = e_desk_current_get(desk->zone);
   e_object_ref(E_OBJECT(eev->desk));
   ecore_event_add(E_EVENT_DESK_BEFORE_SHOW, eev,
                   _e_desk_event_desk_before_show_free, NULL);

   ecore_x_window_shadow_tree_flush();
   for (x = 0; x < desk->zone->desk_x_count; x++)
     {
        for (y = 0; y < desk->zone->desk_y_count; y++)
          {
             E_Desk *desk2;

             desk2 = e_desk_at_xy_get(desk->zone, x, y);
             if (desk2->visible)
               {
                  desk2->visible = 0;
                  if (e_config->desk_flip_wrap)
                    {
                       /* current desk (desk2) is last desk, switching to first desk (desk) */
                       if ((!desk->x) && (!desk->y) && (desk2->x + 1 == desk->zone->desk_x_count) && (desk2->y + 1 == desk->zone->desk_y_count))
                         {
                            dx = (desk->x != desk2->x) ? 1 : 0;
                            dy = (desk->y != desk2->y) ? 1 : 0;
                         }
                       /* current desk (desk2) is first desk, switching to last desk (desk) */
                       else if ((!desk2->x) && (!desk2->y) && (desk->x + 1 == desk->zone->desk_x_count) && (desk->y + 1 == desk->zone->desk_y_count))
                         {
                            dx = (desk->x != desk2->x) ? -1 : 0;
                            dy = (desk->y != desk2->y) ? -1 : 0;
                         }
                    }
                  if ((!dx) && (!dy))
                    {
                       dx = desk->x - desk2->x;
                       dy = desk->y - desk2->y;
                    }
                  if (e_config->desk_flip_animate_mode > 0)
                    _e_desk_hide_begin(desk2, e_config->desk_flip_animate_mode,
                                       dx, dy);
                  break;
               }
          }
     }

   desk->zone->desk_x_prev = desk->zone->desk_x_current;
   desk->zone->desk_y_prev = desk->zone->desk_y_current;
   desk->zone->desk_x_current = desk->x;
   desk->zone->desk_y_current = desk->y;
   desk->visible = 1;

   msg = alloca(sizeof(Edje_Message_Float_Set) + (4 * sizeof(double)));
   msg->count = 5;
   msg->val[0] = e_config->desk_flip_animate_time;
   msg->val[1] = (double)desk->x;
   msg->val[2] = (double)desk->zone->desk_x_count;
   msg->val[3] = (double)desk->y;
   msg->val[4] = (double)desk->zone->desk_y_count;
   edje_object_message_send(desk->zone->bg_object, EDJE_MESSAGE_FLOAT_SET, 0, msg);

   if (desk->zone->bg_object) was_zone = 1;
   if (e_config->desk_flip_animate_mode == 0)
     {
        bl = e_container_border_list_first(desk->zone->container);
        while ((bd = e_container_border_list_next(bl)))
          {
             if ((!bd->hidden) && (bd->desk->zone == desk->zone) && (!bd->iconic))
               {
                  if ((bd->desk == desk) || (bd->sticky))
                    e_border_show(bd);
                  else if (bd->moving)
                    e_border_desk_set(bd, desk);
                  else
                    e_border_hide(bd, 2);
               }
          }
        e_container_border_list_free(bl);
     }

   if (e_config->desk_flip_animate_mode > 0)
     _e_desk_show_begin(desk, e_config->desk_flip_animate_mode, dx, dy);
   else
     {
        if (e_config->focus_last_focused_per_desktop)
          e_desk_last_focused_focus(desk);
     }

   if (was_zone)
     e_bg_zone_update(desk->zone, E_BG_TRANSITION_DESK);
   else
     e_bg_zone_update(desk->zone, E_BG_TRANSITION_START);

   ev = E_NEW(E_Event_Desk_Show, 1);
   ev->desk = desk;
   e_object_ref(E_OBJECT(desk));
   ecore_event_add(E_EVENT_DESK_SHOW, ev, _e_desk_event_desk_show_free, NULL);

   EINA_LIST_FOREACH(e_shelf_list(), l, es)
     {
        if (e_shelf_desk_visible(es, desk))
          e_shelf_show(es);
        else
          e_shelf_hide(es);
     }

   if (e_config->desk_flip_animate_mode == 0)
     {
        eeev = E_NEW(E_Event_Desk_After_Show, 1);
        eeev->desk = e_desk_current_get(desk->zone);
        e_object_ref(E_OBJECT(eeev->desk));
        ecore_event_add(E_EVENT_DESK_AFTER_SHOW, eeev,
                        _e_desk_event_desk_after_show_free, NULL);
     }
   e_zone_edge_flip_eval(desk->zone);
}

EAPI void
e_desk_deskshow(E_Zone *zone)
{
   E_Border *bd;
   E_Border_List *bl;
   E_Desk *desk;
   E_Event_Desk_Show *ev;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   desk = e_desk_current_get(zone);
   bl = e_container_border_list_first(zone->container);
   ecore_x_window_shadow_tree_flush();
   while ((bd = e_container_border_list_next(bl)))
     {
        if (bd->desk == desk)
          {
             if (desk->deskshow_toggle)
               {
                  if (bd->deskshow)
                    {
                        bd->deskshow = 0;
                        e_border_uniconify(bd);
                    }
               }
             else
               {
                  if (bd->iconic) continue;
                  if (bd->client.netwm.state.skip_taskbar) continue;
                  if (bd->user_skip_winlist) continue;
                  bd->deskshow = 1;
                  e_border_iconify(bd);
               }
          }
     }
   desk->deskshow_toggle = desk->deskshow_toggle ? 0 : 1;
   e_container_border_list_free(bl);
   ev = E_NEW(E_Event_Desk_Show, 1);
   ev->desk = desk;
   e_object_ref(E_OBJECT(desk));
   ecore_event_add(E_EVENT_DESK_DESKSHOW, ev,
                   _e_desk_event_desk_deskshow_free, NULL);
}

EAPI void
e_desk_last_focused_focus(E_Desk *desk)
{
   E_Border *bd;

   bd = e_desk_last_focused_border_get(desk);
   if (bd)
     {
        bd->desk_set_focus = 1;
        e_border_focus_set_with_pointer(bd);
     }
}

/*
 * This functions gets the last focus border from a specific destkop,
 * if it found the border, it will return E_Border, else return NULL
 */
EAPI E_Border *
e_desk_last_focused_border_get(E_Desk *desk)
{
   Eina_List *l = NULL;
   E_Border *bd;

   EINA_LIST_FOREACH(e_border_focus_stack_get(), l, bd)
     {
        if ((!bd->iconic) && (bd->visible) &&
            ((bd->desk == desk) || ((bd->zone == desk->zone) && bd->sticky)) &&
            (bd->client.icccm.accepts_focus || bd->client.icccm.take_focus) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_TOOLBAR) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_MENU) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_SPLASH) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DESKTOP))
          {
             /* this was the window last focused in this desktop */
             if (!bd->lock_focus_out)
               return bd;
          }
     }
   return NULL;
}

EAPI void
e_desk_row_add(E_Zone *zone)
{
   e_zone_desk_count_set(zone, zone->desk_x_count, zone->desk_y_count + 1);
}

EAPI void
e_desk_row_remove(E_Zone *zone)
{
   e_zone_desk_count_set(zone, zone->desk_x_count, zone->desk_y_count - 1);
}

EAPI void
e_desk_col_add(E_Zone *zone)
{
   e_zone_desk_count_set(zone, zone->desk_x_count + 1, zone->desk_y_count);
}

EAPI void
e_desk_col_remove(E_Zone *zone)
{
   e_zone_desk_count_set(zone, zone->desk_x_count - 1, zone->desk_y_count);
}

EAPI E_Desk *
e_desk_current_get(E_Zone *zone)
{
   E_OBJECT_CHECK_RETURN(zone, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, NULL);
   
   return e_desk_at_xy_get(zone, zone->desk_x_current, zone->desk_y_current);
}

EAPI E_Desk *
e_desk_at_xy_get(E_Zone *zone, int x, int y)
{
   E_OBJECT_CHECK_RETURN(zone, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, NULL);

   if ((x >= zone->desk_x_count) || (y >= zone->desk_y_count))
     return NULL;
   else if ((x < 0) || (y < 0))
     return NULL;

   if (!zone->desks) return NULL;
   return zone->desks[x + (y * zone->desk_x_count)];
}

EAPI E_Desk *
e_desk_at_pos_get(E_Zone *zone, int pos)
{
   int x, y;

   E_OBJECT_CHECK_RETURN(zone, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, NULL);

   y = pos / zone->desk_x_count;
   x = pos - (y * zone->desk_x_count);

   if ((x >= zone->desk_x_count) || (y >= zone->desk_y_count))
     return NULL;

   return zone->desks[x + (y * zone->desk_x_count)];
}

EAPI void
e_desk_xy_get(E_Desk *desk, int *x, int *y)
{
   E_OBJECT_CHECK(desk);
   E_OBJECT_TYPE_CHECK(desk, E_DESK_TYPE);

   if (x) *x = desk->x;
   if (y) *y = desk->y;
}

EAPI void
e_desk_next(E_Zone *zone)
{
   int x, y;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if ((zone->desk_x_count < 2) && (zone->desk_y_count < 2))
     return;

   x = zone->desk_x_current;
   y = zone->desk_y_current;

   x++;
   if (x >= zone->desk_x_count)
     {
        x = 0;
        y++;
        if (y >= zone->desk_y_count) y = 0;
     }

   e_desk_show(e_desk_at_xy_get(zone, x, y));
}

EAPI void
e_desk_prev(E_Zone *zone)
{
   int x, y;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if ((zone->desk_x_count < 2) && (zone->desk_y_count < 2))
     return;

   x = zone->desk_x_current;
   y = zone->desk_y_current;

   x--;
   if (x < 0)
     {
        x = zone->desk_x_count - 1;
        y--;
        if (y < 0) y = zone->desk_y_count - 1;
     }
   e_desk_show(e_desk_at_xy_get(zone, x, y));
}

#if (ECORE_VERSION_MAJOR > 1) || (ECORE_VERSION_MINOR >= 8)
EAPI void
e_desk_window_profile_set(E_Desk     *desk,
                          const char *profile)
{
   E_Event_Desk_Window_Profile_Change *ev;

   E_OBJECT_CHECK(desk);
   E_OBJECT_TYPE_CHECK(desk, E_DESK_TYPE);

   eina_stringshare_replace(&desk->window_profile, profile);

   ev = E_NEW(E_Event_Desk_Window_Profile_Change, 1);
   ev->desk = desk;
   e_object_ref(E_OBJECT(desk));
   ecore_event_add(E_EVENT_DESK_WINDOW_PROFILE_CHANGE, ev,
                   _e_desk_event_desk_window_profile_change_free, NULL);
}

EAPI void
e_desk_window_profile_add(int         container,
                          int         zone,
                          int         desk_x,
                          int         desk_y,
                          const char *profile)
{
   E_Config_Desktop_Window_Profile *cfprof;

   e_desk_window_profile_del(container, zone, desk_x, desk_y);

   cfprof = E_NEW(E_Config_Desktop_Window_Profile, 1);
   cfprof->container = container;
   cfprof->zone = zone;
   cfprof->desk_x = desk_x;
   cfprof->desk_y = desk_y;
   if (profile) cfprof->profile = eina_stringshare_add(profile);
   else cfprof->profile = NULL;
   e_config->desktop_window_profiles = eina_list_append(e_config->desktop_window_profiles, cfprof);
}

EAPI void
e_desk_window_profile_del(int container,
                          int zone,
                          int desk_x,
                          int desk_y)
{
   Eina_List *l = NULL;
   E_Config_Desktop_Window_Profile *cfprof = NULL;

   EINA_LIST_FOREACH(e_config->desktop_window_profiles, l, cfprof)
     {
        if (!((cfprof->container == container) &&
              (cfprof->zone == zone) &&
              (cfprof->desk_x == desk_x) &&
              (cfprof->desk_y == desk_y)))
          continue;

        e_config->desktop_window_profiles =
          eina_list_remove_list(e_config->desktop_window_profiles, l);
        if (cfprof->profile) eina_stringshare_del(cfprof->profile);
        E_FREE(cfprof);
        break;
     }
}

EAPI void
e_desk_window_profile_update(void)
{
   Eina_List *m, *c, *z, *l;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   E_Desk *desk;
   E_Config_Desktop_Window_Profile *cfprof;
   int d_x, d_y, ok;

   _e_desk_window_profile_change_protocol_set();

   if (!(e_config->use_desktop_window_profile))
     return;

   EINA_LIST_FOREACH(e_manager_list(), m, man)
     {
        EINA_LIST_FOREACH(man->containers, c, con)
          {
             EINA_LIST_FOREACH(con->zones, z, zone)
               {
                  for (d_x = 0; d_x < zone->desk_x_count; d_x++)
                    {
                       for (d_y = 0; d_y < zone->desk_y_count; d_y++)
                         {
                            desk = zone->desks[d_x + zone->desk_x_count * d_y];
                            ok = 0;

                            EINA_LIST_FOREACH(e_config->desktop_window_profiles, l, cfprof)
                              {
                                 if ((cfprof->container >= 0) &&
                                     ((int)con->num != cfprof->container)) continue;
                                 if ((cfprof->zone >= 0) &&
                                     ((int)zone->num != cfprof->zone)) continue;
                                 if ((cfprof->desk_x != d_x) ||
                                     (cfprof->desk_y != d_y)) continue;
                                 e_desk_window_profile_set(desk, cfprof->profile);
                                 ok = 1;
                                 break;
                              }

                            if (!ok)
                              {
                                 e_desk_window_profile_set
                                   (desk, e_config->desktop_default_window_profile);
                              }
                         }
                    }
               }
          }
     }
}
#endif

static void
_e_desk_free(E_Desk *desk)
{
   if (desk->name) eina_stringshare_del(desk->name);
   desk->name = NULL;
   if (desk->animator) ecore_animator_del(desk->animator);
   desk->animator = NULL;
   free(desk);
}

static void
_e_desk_event_desk_show_free(void *data __UNUSED__, void *event)
{
   E_Event_Desk_Show *ev;

   ev = event;
   e_object_unref(E_OBJECT(ev->desk));
   free(ev);
}

static void
_e_desk_event_desk_before_show_free(void *data __UNUSED__, void *event)
{
   E_Event_Desk_Before_Show *ev;

   ev = event;
   e_object_unref(E_OBJECT(ev->desk));
   free(ev);
}

static void
_e_desk_event_desk_after_show_free(void *data __UNUSED__, void *event)
{
   E_Event_Desk_After_Show *ev;

   ev = event;
   e_object_unref(E_OBJECT(ev->desk));
   free(ev);
}

static void
_e_desk_event_desk_deskshow_free(void *data __UNUSED__, void *event)
{
   E_Event_Desk_Show *ev;

   ev = event;
   e_object_unref(E_OBJECT(ev->desk));
   free(ev);
}

static void
_e_desk_event_desk_name_change_free(void *data __UNUSED__, void *event)
{
   E_Event_Desk_Name_Change *ev;

   ev = event;
   e_object_unref(E_OBJECT(ev->desk));
   free(ev);
}
#if (ECORE_VERSION_MAJOR > 1) || (ECORE_VERSION_MINOR >= 8)
static void
_e_desk_event_desk_window_profile_change_free(void *data __UNUSED__, void *event)
{
   E_Event_Desk_Window_Profile_Change *ev;
   ev = event;
   e_object_unref(E_OBJECT(ev->desk));
   E_FREE(ev);
}
#endif
static void
_e_desk_show_begin(E_Desk *desk, int mode, int dx, int dy)
{
   E_Border_List *bl;
   E_Border *bd;
   double t;

   if (dx < 0) dx = -1;
   if (dx > 0) dx = 1;
   if (dy < 0) dy = -1;
   if (dy > 0) dy = 1;

   t = ecore_loop_time_get();
   bl = e_container_border_list_first(desk->zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
        if ((bd->desk->zone == desk->zone) && (!bd->iconic))
          {
             if (bd->moving)
               {
                  bd->fx.start.t = t;
                  bd->fx.start.x = 0;
                  bd->fx.start.y = 0;
                  e_border_desk_set(bd, desk);
                  e_border_show(bd);
               }
             else if ((bd->desk == desk) && (!bd->sticky))
               {
                  e_border_tmp_input_hidden_push(bd);
                  bd->fx.start.t = t;
                  if (mode == 1)
                    {
                       bd->fx.start.x = bd->zone->w * (dx * 1.5);
                       bd->fx.start.y = bd->zone->h * (dy * 1.5);
                    }
                  else if (mode == 2)
                    {
                       int mx, my, bx, by;
                       double fx, fy, ang, rad, len, lmax;

                       mx = bd->zone->x + (bd->zone->w / 2);
                       my = bd->zone->y + (bd->zone->h / 2);

                       bx = bd->x + (bd->w / 2) - mx;
                       by = bd->y + (bd->h / 2) - my;
                       if (bx == 0) bx = 1;
                       if (by == 0) by = 1;
                       fx = (double)bx / (double)(bd->zone->w / 2);
                       fy = (double)by / (double)(bd->zone->h / 2);
                       ang = atan(fy / fx);
                       if (fx < 0.0)
                         ang = M_PI + ang;
                       len = sqrt((bx * bx) + (by * by));
                       lmax = sqrt(((bd->zone->w / 2) * (bd->zone->w / 2)) +
                                   ((bd->zone->h / 2) * (bd->zone->h / 2)));
                       rad = sqrt((bd->w * bd->w) + (bd->h * bd->h)) / 2.0;
                       bx = cos(ang) * (lmax - len + rad);
                       by = sin(ang) * (lmax - len + rad);
                       bd->fx.start.x = bx;
                       bd->fx.start.y = by;
                    }
                  if (bd->fx.start.x < 0)
                    bd->fx.start.x -= bd->zone->x;
                  else
                    bd->fx.start.x += bd->zone->container->w - (bd->zone->x + bd->zone->w);
                  if (bd->fx.start.y < 0)
                    bd->fx.start.y -= bd->zone->y;
                  else
                    bd->fx.start.y += bd->zone->container->h - (bd->zone->y + bd->zone->h);
                  e_border_fx_offset(bd, bd->fx.start.x, bd->fx.start.y);
                  e_border_comp_hidden_set(bd, EINA_TRUE);
               }
          }
     }
   e_container_border_list_free(bl);
   if (desk->animator) ecore_animator_del(desk->animator);
   desk->animator = ecore_animator_add(_e_desk_show_animator, desk);
   desk->animating = EINA_TRUE;
}

static void
_e_desk_show_end(E_Desk *desk)
{
   E_Event_Desk_After_Show *ev;
   E_Border_List *bl;
   E_Border *bd;

   bl = e_container_border_list_first(desk->zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
        if ((bd->desk->zone == desk->zone) && (!bd->iconic))
          {
             if (bd->moving)
               e_border_fx_offset(bd, 0, 0);
             else if ((bd->desk == desk) && (!bd->sticky))
               {
                  e_border_fx_offset(bd, 0, 0);
                  e_border_comp_hidden_set(bd, EINA_FALSE);

                  if (!bd->visible)
                    e_border_show(bd);
               }
             e_border_tmp_input_hidden_pop(bd);
          }
     }

   if ((e_config->focus_policy == E_FOCUS_MOUSE) ||
       (e_config->focus_policy == E_FOCUS_SLOPPY))
     {
        if (e_config->focus_last_focused_per_desktop)
          e_desk_last_focused_focus(desk);
     }
   else
     {
        if (e_config->focus_last_focused_per_desktop)
          e_desk_last_focused_focus(desk);
     }

   e_container_border_list_free(bl);
   ecore_x_window_shadow_tree_flush();
   ev = E_NEW(E_Event_Desk_After_Show, 1);
   ev->desk = e_desk_current_get(desk->zone);
   e_object_ref(E_OBJECT(ev->desk));
   ecore_event_add(E_EVENT_DESK_AFTER_SHOW, ev,
                   _e_desk_event_desk_after_show_free, NULL);
}

static Eina_Bool
_e_desk_show_animator(void *data)
{
   E_Desk *desk;
   E_Border_List *bl;
   E_Border *bd;
   double t, dt, spd;

   desk = data;

   if (!desk->animating)
     {
        _e_desk_show_end(desk);
        desk->animator = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   t = ecore_loop_time_get();
   dt = -1.0;
   spd = e_config->desk_flip_animate_time;
   bl = e_container_border_list_first(desk->zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
        if ((bd->desk->zone == desk->zone) && (!bd->iconic))
          {
             if (bd->moving)
               {
               }
             else if ((bd->desk == desk) && (!bd->sticky))
               {
                  if (!bd->visible)
                    e_border_show(bd);

                  dt = (t - bd->fx.start.t) / spd;
                  if (dt > 1.0) dt = 1.0;
                  dt = 1.0 - dt;
                  dt *= dt; /* decelerate - could be a better hack */
                  e_border_fx_offset(bd,
                                     ((double)bd->fx.start.x * dt),
                                     ((double)bd->fx.start.y * dt));
               }
          }
     }
   e_container_border_list_free(bl);
   if (dt <= 0.0)
     desk->animating = EINA_FALSE;

   return ECORE_CALLBACK_RENEW;
}

static void
_e_desk_hide_begin(E_Desk *desk, int mode, int dx, int dy)
{
   E_Border_List *bl;
   E_Border *bd;
   double t;

   if (dx < 0) dx = -1;
   if (dx > 0) dx = 1;
   if (dy < 0) dy = -1;
   if (dy > 0) dy = 1;

   t = ecore_loop_time_get();
   bl = e_container_border_list_first(desk->zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
        if ((bd->desk->zone == desk->zone) && (!bd->iconic))
          {
             if (bd->moving)
               {
                  bd->fx.start.t = t;
                  bd->fx.start.x = 0;
                  bd->fx.start.y = 0;
               }
             else if ((bd->desk == desk) && (!bd->sticky))
               {
                  bd->fx.start.t = t;
                  if (mode == 1)
                    {
                       bd->fx.start.x = bd->zone->w * (-dx * 1.5);
                       bd->fx.start.y = bd->zone->h * (-dy * 1.5);
                    }
                  else if (mode == 2)
                    {
                       int mx, my, bx, by;
                       double fx, fy, ang, rad, len, lmax;

                       mx = bd->zone->x + (bd->zone->w / 2);
                       my = bd->zone->y + (bd->zone->h / 2);

                       bx = bd->x + (bd->w / 2) - mx;
                       by = bd->y + (bd->h / 2) - my;
                       if (bx == 0) bx = 1;
                       if (by == 0) by = 1;
                       fx = (double)bx / (double)(bd->zone->w / 2);
                       fy = (double)by / (double)(bd->zone->h / 2);
                       ang = atan(fy / fx);
                       if (fx < 0.0)
                         ang = M_PI + ang;
                       len = sqrt((bx * bx) + (by * by));
                       lmax = sqrt(((bd->zone->w / 2) * (bd->zone->w / 2)) +
                                   ((bd->zone->h / 2) * (bd->zone->h / 2)));
                       rad = sqrt((bd->w * bd->w) + (bd->h * bd->h)) / 2.0;
                       bx = cos(ang) * (lmax - len + rad);
                       by = sin(ang) * (lmax - len + rad);
                       bd->fx.start.x = bx;
                       bd->fx.start.y = by;
                    }
                  if (bd->fx.start.x < 0)
                    bd->fx.start.x -= bd->zone->x;
                  else
                    bd->fx.start.x += bd->zone->container->w - (bd->zone->x + bd->zone->w);
                  if (bd->fx.start.y < 0)
                    bd->fx.start.y -= bd->zone->y;
                  else
                    bd->fx.start.y += bd->zone->container->h - (bd->zone->y + bd->zone->h);
                  e_border_fx_offset(bd, 0, 0);
                  e_border_comp_hidden_set(bd, EINA_TRUE);
               }
          }
     }
   e_container_border_list_free(bl);
   if (desk->animator) ecore_animator_del(desk->animator);
   desk->animator = ecore_animator_add(_e_desk_hide_animator, desk);
   desk->animating = EINA_TRUE;
}

static void
_e_desk_hide_end(E_Desk *desk)
{
   E_Border_List *bl;
   E_Border *bd;

   bl = e_container_border_list_first(desk->zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
        if ((bd->desk->zone == desk->zone) && (!bd->iconic))
          {
             if (bd->moving)
               e_border_fx_offset(bd, 0, 0);
             else if ((bd->desk == desk) && (!bd->sticky))
               {
                  e_border_fx_offset(bd, 0, 0);
                  e_border_comp_hidden_set(bd, EINA_FALSE);
                  e_border_hide(bd, 2);
               }
          }
     }
   e_container_border_list_free(bl);
   ecore_x_window_shadow_tree_flush();
}

static Eina_Bool
_e_desk_hide_animator(void *data)
{
   E_Desk *desk;
   E_Border_List *bl;
   E_Border *bd;
   double t, dt, spd;

   desk = data;

   if (!desk->animating)
     {
        _e_desk_hide_end(desk);
        desk->animator = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   t = ecore_loop_time_get();
   dt = -1.0;
   spd = e_config->desk_flip_animate_time;
   bl = e_container_border_list_first(desk->zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
        if ((bd->desk->zone == desk->zone) && (!bd->iconic))
          {
             if (bd->moving)
               {
               }
             else if ((bd->desk == desk) && (!bd->sticky))
               {
                  dt = (t - bd->fx.start.t) / spd;
                  if (dt > 1.0) dt = 1.0;
                  dt *= dt; /* decelerate - could be a better hack */
                  e_border_fx_offset(bd,
                                     ((double)bd->fx.start.x * dt),
                                     ((double)bd->fx.start.y * dt));
               }
          }
     }
   e_container_border_list_free(bl);

   if ((dt < 0.0) || (dt >= 1.0))
     desk->animating = EINA_FALSE;

   return ECORE_CALLBACK_RENEW;
}

#if (ECORE_VERSION_MAJOR > 1) || (ECORE_VERSION_MINOR >= 8)
static void
_e_desk_window_profile_change_protocol_set(void)
{
   Eina_List *l = NULL;
   E_Manager *man;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        ecore_x_e_window_profile_supported_set
          (man->root, e_config->use_desktop_window_profile);
     }
}
#endif
