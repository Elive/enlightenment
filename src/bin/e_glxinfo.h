#ifdef E_TYPEDEFS

typedef struct _E_Glxinfo E_Glxinfo;
struct _E_Glxinfo
{
   Eina_Bool gl;

   const char *gl_vendor;
   const char *gl_renderer;
   const char *gl_version;
   const char *gl_extensions;

   const char *glx_extensions;

   struct
     {
        const char *vendor;
        const char *version;
        const char *extensions;
     }server, client;
};

#else
#ifndef E_GLXINFO_H
#define E_GLXINFO_H

EAPI int e_glxinfo_init(void);
EAPI int e_glxinfo_shutdown(void);
EAPI const char *e_glxinfo_renderer_get(void);
EAPI const char *e_glxinfo_gl_extensions_get(void);
EAPI const char *e_glxinfo_gl_extensions_server_get(void);
EAPI const char *e_glxinfo_gl_extensions_client_get(void);
EAPI E_Glxinfo  *e_glxinfo_get(void);
#endif
#endif
