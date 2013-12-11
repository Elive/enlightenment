#include "e.h"

int E_EVENT_COMPOSITE_ACTIVE = 0;
int E_EVENT_COMPOSITE_INACTIVE = 0;
int E_EVENT_COMPOSITE_CHANGE = 0;
static int _e_comp_old_val = 0;
static int _e_comp_new_val = 0;
static Ecore_Timer *_e_comp_timer = NULL;

static Eina_Bool
_e_composite_detect_timer(EINA_UNUSED void *data)
{
   if (ecore_x_screen_is_composited(e_manager_current_get()->num))
     {
        if (!_e_comp_old_val && !_e_comp_new_val)
          {
             _e_comp_new_val = 1;
             ecore_event_add(E_EVENT_COMPOSITE_ACTIVE, NULL, NULL, NULL);
          }
        else if (_e_comp_old_val && !_e_comp_new_val)
          {
             _e_comp_old_val = 0;
             _e_comp_new_val = 1;
             ecore_event_add(E_EVENT_COMPOSITE_ACTIVE, NULL, NULL, NULL);
          }
     }
   else
     {
        if (!_e_comp_old_val && !_e_comp_new_val)
          {
             _e_comp_old_val = 1;
             ecore_event_add(E_EVENT_COMPOSITE_INACTIVE, NULL, NULL, NULL);
          }
        else if (!_e_comp_old_val && _e_comp_new_val)
          {
             _e_comp_new_val = 0;
             _e_comp_old_val = 1;
             ecore_event_add(E_EVENT_COMPOSITE_INACTIVE, NULL, NULL, NULL);
          }
     }

   return ECORE_CALLBACK_RENEW;
}

EAPI int
e_comp_detect_init(void)
{

   E_EVENT_COMPOSITE_ACTIVE = ecore_event_type_new();
   E_EVENT_COMPOSITE_INACTIVE = ecore_event_type_new();
   E_EVENT_COMPOSITE_CHANGE = ecore_event_type_new();

   if (!_e_comp_timer)
     _e_comp_timer = ecore_timer_add(0.25, _e_composite_detect_timer, NULL);
   return 1;
}

EAPI int
e_comp_detect_shutdown(void)
{
   if (_e_comp_timer) ecore_timer_del(_e_comp_timer);
   _e_comp_timer = NULL;
   return 1;
}
