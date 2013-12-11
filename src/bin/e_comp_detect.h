#ifdef E_TYPEDEFS
#else
#ifndef E_COMP_DETECT
#define E_COMP_DETECT

extern EAPI int E_EVENT_COMPOSITE_ACTIVE;
extern EAPI int E_EVENT_COMPOSITE_INACTIVE;
extern EAPI int E_EVENT_COMPOSITE_CHANGE;

EAPI int e_comp_detect_init(void);
EAPI int e_comp_detect_shutdown(void);

#endif
#endif
