#include "e.h"
static int pointer_position_x = 0;
static int pointer_position_y = 0;
static Ecore_Timer *_focus_timer = NULL;

/* local subsystem functions */
static Eina_Bool _e_focus_raise_timer(void *data);
static void      _e_focus_event_mouse_in_set_focus(E_Border *bd, int desk_set_focus);
static Eina_Bool _e_focus_event_mouse_in_update_mouse_focus(void *data);
static Eina_Bool _e_focus_pager_interconnect(E_Border *bd);
/* local subsystem globals */

typedef struct _E_Focus_Pager_Geometry E_Focus_Pager_Geometry;
struct _E_Focus_Pager_Geometry
{
   int x, y, w, h;
   Eina_Bool set;
};

E_Focus_Pager_Geometry _focus_pager_geometry;

/* externally accessible functions */
EINTERN int
e_focus_init(void)
{
   return 1;
}

EINTERN int
e_focus_shutdown(void)
{
   return 1;
}

EAPI void
e_focus_idler_before(void)
{
   return;
}

EAPI void
e_focus_pager_geometry_set(int x, int y, int w, int h, Eina_Bool set)
{
   _focus_pager_geometry.w = _focus_pager_geometry.h = _focus_pager_geometry.x = _focus_pager_geometry.y = 0;
   _focus_pager_geometry.w = w;
   _focus_pager_geometry.h = h;
   _focus_pager_geometry.x = x;
   _focus_pager_geometry.y = y;
   _focus_pager_geometry.set = set;
}

EAPI void
e_focus_event_mouse_in(E_Border *bd)
{
   int desk_set_focus = 0;

   if ((e_config->focus_policy == E_FOCUS_MOUSE) ||
       (e_config->focus_policy == E_FOCUS_SLOPPY))
     {
        E_Border *_last_focused = NULL;

        _last_focused = e_desk_last_focused_border_get(bd->desk);
        if (_last_focused && (_last_focused->desk_set_focus))
          {
             _last_focused->desk_set_focus = 0;
             pointer_position_x = 0;
             pointer_position_y = 0;

             if (bd->client.win != _last_focused->client.win)
               desk_set_focus = 1;
          }

        _e_focus_event_mouse_in_set_focus(bd, desk_set_focus);
     }
   if (bd->raise_timer) ecore_timer_del(bd->raise_timer);
   bd->raise_timer = NULL;
   if (e_config->use_auto_raise && (!desk_set_focus))
     {
        if (e_config->auto_raise_delay == 0.0)
          {
             if (!bd->lock_user_stacking)
               {
                  if (e_config->border_raise_on_focus)
                    e_border_raise(bd);
               }
          }
        else
          bd->raise_timer = ecore_timer_add(e_config->auto_raise_delay, _e_focus_raise_timer, bd);
     }
}

EAPI void
e_focus_event_mouse_out(E_Border *bd)
{
   if (e_config->focus_policy == E_FOCUS_MOUSE)
     {
        /* FIXME: this is such a hack. its a big hack around x's async events
         * as we dont know always exactly what action causes what event
         * so by waiting more than 0.2 secs before reverting focus to nothing
         * since we entered root, we are ignoring mouse in's on the root
         * container for a bit after the mosue may have entered it
         */
        if ((ecore_loop_time_get() - e_grabinput_last_focus_time_get()) > 0.2)
          {
             if (!bd->lock_focus_in)
               {
                  if (bd->focused)
                    e_border_focus_set(bd, 0, 1);
               }
          }
     }
   if (bd->raise_timer)
     {
        ecore_timer_del(bd->raise_timer);
        bd->raise_timer = NULL;
     }
}

EAPI void
e_focus_event_mouse_down(E_Border *bd)
{
   if (e_config->focus_policy == E_FOCUS_CLICK)
     {
        e_border_focus_set(bd, 1, 1);

        if (!bd->lock_user_stacking)
          {
             if (e_config->border_raise_on_focus)
               e_border_raise(bd);
          }
     }
   else if (e_config->always_click_to_raise)
     {
        if (!bd->lock_user_stacking)
          {
             if (e_config->border_raise_on_focus)
               e_border_raise(bd);
          }
     }
   else if (e_config->always_click_to_focus)
     {
        e_border_focus_set(bd, 1, 1);
     }
}

EAPI void
e_focus_event_mouse_up(E_Border *bd __UNUSED__)
{
}

EAPI void
e_focus_event_focus_in(E_Border *bd)
{
   if ((e_config->focus_policy == E_FOCUS_CLICK) &&
       (!e_config->always_click_to_raise) &&
       (!e_config->always_click_to_focus))
     {
        if (!bd->button_grabbed) return;
        e_bindings_mouse_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
        e_bindings_wheel_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
        ecore_x_window_button_ungrab(bd->win, 1, 0, 1);
        ecore_x_window_button_ungrab(bd->win, 2, 0, 1);
        ecore_x_window_button_ungrab(bd->win, 3, 0, 1);
        e_bindings_mouse_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
        e_bindings_wheel_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
        bd->button_grabbed = 0;
     }
}

EAPI void
e_focus_event_focus_out(E_Border *bd)
{
   if ((e_config->focus_policy == E_FOCUS_CLICK) &&
       (!e_config->always_click_to_raise) &&
       (!e_config->always_click_to_focus))
     {
        if (bd->button_grabbed) return;
        ecore_x_window_button_grab(bd->win, 1,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        ecore_x_window_button_grab(bd->win, 2,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        ecore_x_window_button_grab(bd->win, 3,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        bd->button_grabbed = 1;
     }
}

EAPI void
e_focus_setup(E_Border *bd)
{
   if ((e_config->focus_policy == E_FOCUS_CLICK) ||
       (e_config->always_click_to_raise) ||
       (e_config->always_click_to_focus))
     {
        if (bd->button_grabbed) return;
        ecore_x_window_button_grab(bd->win, 1,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        ecore_x_window_button_grab(bd->win, 2,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        ecore_x_window_button_grab(bd->win, 3,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        bd->button_grabbed = 1;
     }
}

EAPI void
e_focus_setdown(E_Border *bd)
{
   if (!bd->button_grabbed) return;
   e_bindings_mouse_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
   e_bindings_wheel_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
   ecore_x_window_button_ungrab(bd->win, 1, 0, 1);
   ecore_x_window_button_ungrab(bd->win, 2, 0, 1);
   ecore_x_window_button_ungrab(bd->win, 3, 0, 1);
   e_bindings_mouse_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
   e_bindings_wheel_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
   bd->button_grabbed = 0;
}

/* local subsystem functions */
static Eina_Bool
_e_focus_raise_timer(void *data)
{
   E_Border *bd;

   bd = data;
   if (!bd->lock_user_stacking) e_border_raise(bd);
   bd->raise_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}
/*
 * This function checks if pager sets it's popup geometry
 * it then checks if the mouse is within the geometry of pager
 * if it is and the mouse is also over a border ((AND)) focus
 * was already set by the desktop, do not update focus unless
 * the user moves the mouse.
 */
static Eina_Bool
_e_focus_pager_interconnect(E_Border *bd)
{
   E_Border *bdu = NULL;
   int pointer_x, pointer_y, zw, zh;
   int pager_x, pager_y, pager_w, pager_h;

   if (!_focus_pager_geometry.set) return EINA_FALSE;

   bdu = e_border_under_pointer_get(bd->desk, NULL);
   if (!bdu) return EINA_FALSE;

   ecore_x_pointer_root_xy_get(&pointer_x, &pointer_y);
   e_zone_useful_geometry_get(bd->zone, NULL, NULL, &zw, &zh);

   pager_x = _focus_pager_geometry.x;
   pager_y = _focus_pager_geometry.y;
   pager_w = _focus_pager_geometry.w;
   pager_h = _focus_pager_geometry.h;

   if (((pointer_x > pager_x) && (pointer_x < (pager_x + pager_w))) &&
       ((pointer_y > pager_y) && (pointer_y < (pager_y + pager_h))))
     {
        E_Border *bdf = NULL;

        bdf = e_border_focused_get();
        if ((bdf) && (bdu->client.win == bdf->client.win)) return EINA_FALSE;

        e_focus_pager_geometry_set(0, 0, 0, 0, EINA_FALSE);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}
   
/*
 * If "Refocus last window on desktop switch" is Enabled and the user
 * switched desktop and the mouse pointer is above another window that
 * is not the last focused border, WE DO NOT UPDATE FOCUS, instead we
 * wait for mouse movement, if the mouse moves 10 pixels in any direction
 * in the window we will then update focus.
*/
static void
_e_focus_event_mouse_in_set_focus(E_Border *bd, int desk_set_focus)
{
   if (!desk_set_focus && _e_focus_pager_interconnect(bd))
     desk_set_focus = 1;

   if (desk_set_focus)
    {
        if (_focus_timer) ecore_timer_del(_focus_timer);
        _focus_timer = ecore_timer_loop_add(0.05,_e_focus_event_mouse_in_update_mouse_focus, bd);
        return;
     }
   else if (!desk_set_focus)
     {
        if (_focus_timer) ecore_timer_del(_focus_timer);
        _focus_timer = NULL;
     }

   e_border_focus_set(bd, 1, 1);
}

/*
 * This functions checks if the mouse has moved when you
 * switched desktop, if the mouse moves more than 10px in
 * any direction it will update focus.
 */
static Eina_Bool
_e_focus_event_mouse_in_update_mouse_focus(void *data)
{
   E_Border *bd;
   int pointer_curr_x, pointer_curr_y;

   if (!(bd = data)) return ECORE_CALLBACK_CANCEL;

   ecore_x_pointer_root_xy_get(&pointer_curr_x, &pointer_curr_y);

   if (!e_border_under_pointer_get(bd->desk, NULL))
     {
        if (_focus_timer) ecore_timer_del(_focus_timer);
        _focus_timer = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   if (pointer_position_x || pointer_position_y)
     {
        int offset = 10;
        int x, y;

        x = pointer_position_x - pointer_curr_x;
        y = pointer_position_y - pointer_curr_y;

        if (((x > 0) && (x >=  offset))   || ((y > 0) && (y >=  offset)) ||
            ((x < 0) && (x <= ~offset+1)) || ((y < 0) && (y <= ~offset+1)))
          {
             e_border_focus_set(bd, 1, 1);
             pointer_position_x = 0;
             pointer_position_y = 0;

             if (_focus_timer) ecore_timer_del(_focus_timer);
             _focus_timer = NULL;

             return ECORE_CALLBACK_DONE;
          }
     }

   if (!pointer_position_x && !pointer_position_y)
     {
        pointer_position_x = pointer_curr_x;
        pointer_position_y = pointer_curr_y;
     }
   return ECORE_CALLBACK_RENEW;
}


