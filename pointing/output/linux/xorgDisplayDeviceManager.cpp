/* -*- mode: c++ -*-
 *
 * pointing/output/linux/xorgDisplayDeviceManager.h --
 *
 * Initial software
 * Authors: Izzat Mukhanov
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/output/linux/xorgDisplayDeviceManager.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdexcept>
#include <stdlib.h>
#include <X11/extensions/Xrandr.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#include <iostream>

#define BUFFER_SIZE 128
#define OCNE(X) ((XRROutputChangeNotifyEvent*)X)

namespace pointing {

const char *con_actions[] = { "connected", "disconnected", "unknown" };

static void
xerror(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

static int
error_handler(void) {
    exit(EXIT_FAILURE);
}

void *xorgDisplayDeviceManager::eventloop(void * /* context */)
{
  //xorgDisplayDeviceManager *self = (xorgDisplayDeviceManager *)context;
  while (1)
  {
    XEvent ev;
    Display *dpy;
    char buf[BUFFER_SIZE];
    uid_t uid;

    if (((uid = getuid()) == 0) || uid != geteuid())
      xerror("May not run as root\n");

    if ((dpy = XOpenDisplay(NULL)) == NULL)
      xerror("Cannot open display\n");

    XRRSelectInput(dpy, DefaultRootWindow(dpy), RROutputChangeNotifyMask);
    XSync(dpy, False);
    XSetIOErrorHandler((XIOErrorHandler) error_handler);
    while(1) {
      if (!XNextEvent(dpy, &ev)) {
        std::cout << "haha" << std::endl;
        XRRScreenResources *resources = XRRGetScreenResources(OCNE(&ev)->display,
                                                              OCNE(&ev)->window);
        if (resources == NULL) {
          fprintf(stderr, "Could not get screen resources\n");
          continue;
        }

        XRROutputInfo *info = XRRGetOutputInfo(OCNE(&ev)->display, resources,
                                               OCNE(&ev)->output);
        if (info == NULL) {
          XRRFreeScreenResources(resources);
          fprintf(stderr, "Could not get output info\n");
          continue;
        }

        std::cout << "String: " << DisplayString(dpy) << std::endl;

        snprintf(buf, BUFFER_SIZE, "%s %s", info->name,
                 con_actions[info->connection]);
        //if (verbose) {
          printf("Event: %s %s\n", info->name,
                 con_actions[info->connection]);
          printf("Time: %lu\n", info->timestamp);
          if (info->crtc == 0) {
            printf("Size: %lumm x %lumm\n", info->mm_width, info->mm_height);
          }
          else {
            printf("CRTC: %lu\n", info->crtc);
            XRRCrtcInfo *crtc = XRRGetCrtcInfo(dpy, resources, info->crtc);
            if (crtc != NULL) {
              printf("Size: %dx%d\n", crtc->width, crtc->height);
              XRRFreeCrtcInfo(crtc);
            }
          }
        //}
        XRRFreeScreenResources(resources);
        XRRFreeOutputInfo(info);
      }
    }
    // Each second
    usleep(1000000);
  }
  return 0;
}

xorgDisplayDeviceManager::xorgDisplayDeviceManager() {
  int ret = pthread_create(&thread, NULL, eventloop, (void *)this);
  if (ret < 0)
  {
    perror("xorgDisplayDeviceManager::xorgDisplayDeviceManager");
    throw std::runtime_error("xorgDisplayDeviceManager: pthread_create failed");
  }
}
}
