#include "e.h"
/* for X11 VidMode stuff */
# include <X11/Xutil.h>
# include <X11/extensions/xf86vmode.h>

static Ecore_Event_Handler *_e_nightmode_handler_change_mode = NULL;

static void
xcalib_main(Eina_Bool set)
{
   u_int16_t *r_ramp = NULL, *g_ramp = NULL, *b_ramp = NULL;
   int i;
   u_int16_t tmpRampVal = 0;
   int screen = -1;
   int ramp_size = 256;

   /* X11 */
   XF86VidModeGamma gamma;
   Ecore_X_Display *dpy = NULL;

   dpy = ecore_x_display_get();
   screen = DefaultScreen (dpy);

   /* clean gamma table if option set */
   gamma.red = 1.0;
   gamma.green = 1.0;
   gamma.blue = 1.0;
   if (!set) {
        if (!XF86VidModeSetGamma (dpy, screen, &gamma)) {
             ERR ("Unable to reset display gamma");
        }
        return;
   }

   /* get number of entries for gamma ramps */
   if (!XF86VidModeGetGammaRampSize (dpy, screen, &ramp_size)) {
        ERR ("Unable to query gamma ramp size");
   }

   r_ramp = (unsigned short *) malloc (ramp_size * sizeof (unsigned short));
   g_ramp = (unsigned short *) malloc (ramp_size * sizeof (unsigned short));
   b_ramp = (unsigned short *) malloc (ramp_size * sizeof (unsigned short));

   if (!XF86VidModeGetGammaRamp (dpy, screen, ramp_size, r_ramp, g_ramp, b_ramp))
     WRN ("Unable to get display calibration");

     {
        float redBrightness = 0.0;
        float redContrast = 100.0;
        float redMin = 0.0;
        float redMax = 1.0;

        redMin = (double)r_ramp[0] / 65535.0;
        redMax = (double)r_ramp[ramp_size - 1] / 65535.0;
        redBrightness = redMin * 100.0;
        redContrast = (redMax - redMin) / (1.0 - redMin) * 100.0;
        DBG("Red Brightness: %f   Contrast: %f  Max: %f  Min: %f\n", redBrightness, redContrast, redMax, redMin);
     }

     {
        float greenBrightness = 0.0;
        float greenContrast = 100.0;
        float greenMin = 0.0;
        float greenMax = 1.0;

        greenMin = (double)g_ramp[0] / 65535.0;
        greenMax = (double)g_ramp[ramp_size - 1] / 65535.0;
        greenBrightness = greenMin * 100.0;
        greenContrast = (greenMax - greenMin) / (1.0 - greenMin) * 100.0;
        DBG("Green Brightness: %f   Contrast: %f  Max: %f  Min: %f\n", greenBrightness, greenContrast, greenMax, greenMin);
     }

     {
        float blueBrightness = 0.0;
        float blueContrast = 100.0;
        float blueMin = 0.0;
        float blueMax = 1.0;

        blueMin = (double)b_ramp[0] / 65535.0;
        blueMax = (double)b_ramp[ramp_size - 1] / 65535.0;
        blueBrightness = blueMin * 100.0;
        blueContrast = (blueMax - blueMin) / (1.0 - blueMin) * 100.0;
        DBG("Blue Brightness: %f   Contrast: %f  Max: %f  Min: %f\n", blueBrightness, blueContrast, blueMax, blueMin);
     }

   if (set)
     {
        for (i = 0; i < ramp_size; i++) {
             if(i >= ramp_size / 2)
               break;
             tmpRampVal = r_ramp[i];
             r_ramp[i] = r_ramp[ramp_size - i - 1];
             r_ramp[ramp_size - i - 1] = tmpRampVal;
             tmpRampVal = g_ramp[i];
             g_ramp[i] = g_ramp[ramp_size - i - 1];
             g_ramp[ramp_size - i - 1] = tmpRampVal;
             tmpRampVal = b_ramp[i];
             b_ramp[i] = b_ramp[ramp_size - i - 1];
             b_ramp[ramp_size - i - 1] = tmpRampVal;
        }
     }
/* write gamma ramp to X-server */
if (!XF86VidModeSetGammaRamp (dpy, screen, ramp_size, r_ramp, g_ramp, b_ramp))
   WRN ("Unable to calibrate display");

   DBG("X-LUT size:      \t%d\n", ramp_size);

   free(r_ramp);
   free(g_ramp);
   free(b_ramp);

}

EAPI int e_nightmode_init(void)
{
   if (e_config->mode.night)
     xcalib_main(EINA_TRUE);
   else
     xcalib_main(EINA_FALSE);

   return 1;
}

int e_nightmode_shutdown(void)
{
   if (e_config->mode.night)
     xcalib_main(EINA_FALSE);

   if (_e_nightmode_handler_change_mode)
     ecore_event_handler_del(_e_nightmode_handler_change_mode);

   return 1;
}
