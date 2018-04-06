#include "e.h"
#include "Ecore.h"
#include "Ecore_X.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/extensions/randrproto.h>
#include <X11/extensions/Xrandr.h>
#include "e_backlight_fake.h"

static double
dmin(double x, double y)
{
    if (x < y) {
  return (x);
    } else {
  return (y);
    }
}

static int
find_last_non_clamped(CARD16 array[], int size) {
    int i;
    for (i = size - 1; i > 0; i--) {
        if (array[i] < 0xffff)
        return i;
    }
    return 0;
}

EAPI double
e_backlight_fake_update(E_Zone *zone)
{
  Ecore_X_Window root;
  Display *dpy;
  XRRScreenResources *rr;
  XRRCrtcGamma *crtgamma;
  double       brightness;
  double i1, v1, i2, v2;
  int size, middle, last_best, last_red, last_green, last_blue;
  CARD16 *best_array;

  root = zone->container->manager->root;
  dpy = ecore_x_display_get();

  rr = XRRGetScreenResources(dpy,root);

  size = XRRGetCrtcGammaSize(dpy, rr->crtcs[0]);
  if (!size) {
    XRRFreeScreenResources(rr);
    printf("Failed to get size of gamma for output\n");
    return 1.0;
  }

  crtgamma = XRRGetCrtcGamma(dpy, rr->crtcs[0]);
  if (!crtgamma) {
    XRRFreeScreenResources(rr);
    printf("Failed to get gamma for output \n");
    return 1.0;
  }

  last_red = find_last_non_clamped(crtgamma->red, size);
  last_green = find_last_non_clamped(crtgamma->green, size);
  last_blue = find_last_non_clamped(crtgamma->blue, size);
  best_array = crtgamma->red;
  last_best = last_red;
  if (last_green > last_best) {
    last_best = last_green;
    best_array = crtgamma->green;
  }
  if (last_blue > last_best) {
    last_best = last_blue;
    best_array = crtgamma->blue;
  }
  if (last_best == 0)
    last_best = 1;

  middle = last_best / 2;
  i1 = (double)(middle + 1) / size;
  v1 = (double)(best_array[middle]) / 65535;
  i2 = (double)(last_best + 1) / size;
  v2 = (double)(best_array[last_best]) / 65535;
  if (v2 < 0.0001) { /* The screen is black */
    brightness = 0;
  } else {
    if ((last_best + 1) == size)
      brightness = v2;
    else
      brightness = exp((log(v2)*log(i1) - log(v1)*log(i2))/log(i1/i2));
  }
  XRRFreeGamma(crtgamma);
  XRRFreeScreenResources(rr);
  return brightness;
}

EAPI void
e_backlight_fake_set(E_Zone *zone, double val)
{
  int i = 0;
  Ecore_X_Window root;
  Display* dpy;
  XRRScreenResources *rr;
  XRRCrtcGamma *crtgamma;
  size_t size;

  root = zone->container->manager->root;
  dpy = ecore_x_display_get();
  //dpy = XOpenDisplay(NULL);// ecore_x_display_get();
  //root = RootWindow(dpy, screen); // DefaultRootWindow(dpy);

  rr = XRRGetScreenResources(dpy,root);
  size = XRRGetCrtcGammaSize(dpy, rr->crtcs[0]);

  if (size == 0)
  {
    XRRFreeScreenResources(rr);
    printf("Failed to get size of gamma for output\n");
    return;
  }

  crtgamma = XRRAllocGamma (size);

  double  red = 1.0, green = 1.0, blue = 1.0;

  for (i = 0; i < size; i++) {
    if (red == 1.0 && val == 1.0)
      crtgamma->red[i] = (i << 8) + i;
    else
      crtgamma->red[i] = dmin(pow((double)i/(double)(size - 1),
                                  red) * val,
                              1.0) * 65535.0;

    if (green == 1.0 && val == 1.0)
      crtgamma->green[i] = (i << 8) + i;
    else
      crtgamma->green[i] = dmin(pow((double)i/(double)(size - 1),
                                    green) * val,
                                1.0) * 65535.0;

    if (blue == 1.0 && val == 1.0)
      crtgamma->blue[i] = (i << 8) + i;
    else
      crtgamma->blue[i] = dmin(pow((double)i/(double)(size - 1),
                                   blue) * val,
                               1.0) * 65535.0;
  }

  XRRSetCrtcGamma (dpy, rr->crtcs[0], crtgamma);
  XSync(dpy, False);
  XRRFreeGamma(crtgamma);
  XRRFreeScreenResources(rr);
  return 0;
}
