#include "e.h"
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

E_Glxinfo *eglx;

EAPI int
e_glxinfo_init(void)
{
   Display *dpy;
   int scrnum = 0;
   Eina_Bool allowDirect = EINA_TRUE;
   Window win;
   char* displayName = NULL;

   int attribSingle[] = {
        GLX_RGBA,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        None
   };
   int attribDouble[] = {
        GLX_RGBA,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_DOUBLEBUFFER,
        None
   };
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   GLXContext ctx = NULL;
   XVisualInfo *visinfo;
   int width = 100, height = 100;

   eglx = E_NEW(E_Glxinfo, 1);

   dpy = XOpenDisplay(displayName);
   if (!dpy)
     {
        eglx->gl = EINA_FALSE;
        ERR("unable to open display %s", XDisplayName(displayName));
        return 0;
     }

   root = RootWindow(dpy, scrnum);
   visinfo = glXChooseVisual(dpy, scrnum, attribSingle);
   if (!visinfo)
     visinfo = glXChooseVisual(dpy, scrnum, attribDouble);

   if (visinfo)
     ctx = glXCreateContext(dpy, visinfo, NULL, allowDirect);

#ifdef GLX_VERSION_1_3
   if (!visinfo) {
        int fbAttribSingle[] = {
             GLX_RENDER_TYPE, GLX_RGBA_BIT,
             GLX_RED_SIZE, 1,
             GLX_GREEN_SIZE, 1,
             GLX_BLUE_SIZE, 1,
             GLX_DOUBLEBUFFER, GL_FALSE,
             None
        };
        int fbAttribDouble[] = {
             GLX_RENDER_TYPE, GLX_RGBA_BIT,
             GLX_RED_SIZE, 1,
             GLX_GREEN_SIZE, 1,
             GLX_BLUE_SIZE, 1,
             GLX_DOUBLEBUFFER, GL_TRUE,
             None
        };
        GLXFBConfig *configs = NULL;
        int nConfigs;

        if(visinfo == VisualNoMask)
          {
             eglx->gl = EINA_FALSE;
             ERR("No Masks");
             XFree(visinfo);
             return 0;
          }

        configs = glXChooseFBConfig(dpy, scrnum, fbAttribSingle, &nConfigs);
        if (!configs)
          configs = glXChooseFBConfig(dpy, scrnum, fbAttribDouble, &nConfigs);

        if (configs) {
             visinfo = glXGetVisualFromFBConfig(dpy, configs[0]);
             ctx = glXCreateNewContext(dpy, configs[0], GLX_RGBA_TYPE, NULL, allowDirect);
             XFree(configs);
        }
   }
#endif
   if (!visinfo) {
        eglx->gl = EINA_FALSE;
        ERR("Error: couldn't find RGB GLX visual or fbconfig");
        return 0;
   }

   if (!ctx) {
        ERR("Error: glXCreateContext failed");
        eglx->gl = EINA_FALSE;
        XFree(visinfo);
        return 0;
   }

   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
   win = XCreateWindow(dpy, root, 0, 0, width, height,
                       0, visinfo->depth, InputOutput, visinfo->visual, mask, &attr);

   if (glXMakeCurrent(dpy, win, ctx))
     {
        eglx->gl = EINA_TRUE;

        eglx->server.vendor = eina_stringshare_add(glXQueryServerString(dpy, scrnum, GLX_VENDOR));
        eglx->server.version = eina_stringshare_add(glXQueryServerString(dpy, scrnum, GLX_VERSION));
        eglx->server.extensions = eina_stringshare_add(glXQueryServerString(dpy, scrnum, GLX_EXTENSIONS));

        eglx->client.vendor = eina_stringshare_add(glXGetClientString(dpy, GLX_VENDOR));
        eglx->client.version = eina_stringshare_add(glXGetClientString(dpy, GLX_VERSION));
        eglx->client.extensions = eina_stringshare_add(glXGetClientString(dpy, GLX_EXTENSIONS));

        eglx->gl_vendor = eina_stringshare_add((const char *) glGetString(GL_VENDOR));
        eglx->gl_renderer = eina_stringshare_add((const char *)glGetString(GL_RENDERER));
        eglx->gl_version = eina_stringshare_add((const char *)glGetString(GL_VERSION));
        eglx->gl_extensions = eina_stringshare_add((const char *)glGetString(GL_EXTENSIONS));
     }
   else
     {
        eglx->gl = EINA_FALSE;
        ERR("glXMakeCurrent failed!!!");
     }

   return 1;
}

EAPI int
e_glxinfo_shutdown(void)
{
   if (eglx->gl_vendor)          eina_stringshare_del(eglx->gl_vendor);
   if (eglx->gl_renderer)        eina_stringshare_del(eglx->gl_renderer);
   if (eglx->gl_version)         eina_stringshare_del(eglx->gl_version);
   if (eglx->gl_extensions)      eina_stringshare_del(eglx->gl_extensions);

   if (eglx->server.vendor)      eina_stringshare_del(eglx->server.vendor);
   if (eglx->server.version)     eina_stringshare_del(eglx->server.version);
   if (eglx->server.extensions)  eina_stringshare_del(eglx->server.extensions);

   if (eglx->client.vendor)      eina_stringshare_del(eglx->client.vendor);
   if (eglx->client.version)     eina_stringshare_del(eglx->client.version);
   if (eglx->client.extensions)  eina_stringshare_del(eglx->client.extensions);

   return 1;
}

EAPI E_Glxinfo *
e_glxinfo_get(void)
{
   if (!eglx) return NULL;
   return eglx;
}

EAPI const char*
e_glxinfo_renderer_get(void)
{
   if(!eglx->gl) return NULL;
   return eglx->gl_renderer;
}

EAPI const char*
e_glxinfo_gl_extensions_get(void)
{
   if(!eglx->gl) return NULL;
   return eglx->gl_extensions;
}

EAPI const char*
e_glxinfo_gl_extensions_server_get(void)
{
   if(!eglx->gl) return NULL;
   return eglx->server.extensions;
}

EAPI const char*
e_glxinfo_gl_extensions_client_get(void)
{
   if(!eglx->gl) return NULL;
   return eglx->client.extensions;
}




