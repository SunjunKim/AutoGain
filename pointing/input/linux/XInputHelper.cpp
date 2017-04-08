/* -*- mode: c++ -*-
 *
 * pointing/input/linux/XinputHelper.cpp --
 *
 * Initial software
 * Authors: Izzatbek Mukhanov
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

// This file can be used to find a device via XInput interface, enable/disable it
// Or to pass other commands

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <pointing/input/linux/XInputHelper.h>
#include <X11/Xatom.h>

namespace pointing
{
XDeviceInfo* find_device_info(Display	*display,
                   const char		*name,
                   Bool		only_extended)
  {
    XDeviceInfo	*devices;
    XDeviceInfo *found = NULL;
    int		loop;
    int		num_devices;
    int		len = strlen(name);
    Bool	is_id = True;
    XID		id = (XID)-1;

    for(loop=0; loop<len; loop++) {
      if (!isdigit(name[loop])) {
        is_id = False;
        break;
      }
    }

    if (is_id) {
      id = atoi(name);
    }

    devices = XListInputDevices(display, &num_devices);

    for(loop=0; loop<num_devices; loop++) {
      if ((!only_extended || (devices[loop].use >= IsXExtensionDevice)) &&
          ((!is_id && strcmp(devices[loop].name, name) == 0) ||
           (is_id && devices[loop].id == id))) {
        if (found) {
          fprintf(stderr,
                  "Warning: There are multiple devices named '%s'.\n"
                  "To ensure the correct one is selected, please use "
                  "the device ID instead.\n\n", name);
          return NULL;
        } else {
          found = &devices[loop];
        }
      }
    }
    return found;
  }

  void executeCommand(Display	*dpy, XDeviceInfo *info, const char *command, unsigned char data)
  {
    if (!info)
    {
      fprintf(stderr, "unable to find the device\n");
      return;
    }

    XDevice *dev = XOpenDevice(dpy, info->id);
    if (!dev)
    {
      fprintf(stderr, "unable to open the device\n");
      return;
    }

    Atom prop = XInternAtom(dpy, command, False);

    XChangeDeviceProperty(dpy, dev, prop, XA_INTEGER, 8, PropModeReplace,
                          &data, 1);

    XCloseDevice(dpy, dev);
  }

  void enableSynapticsTouchpad()
  {
    Display	*dpy = XOpenDisplay(0);
    if (dpy)
    {
      XDeviceInfo *info = find_device_info(dpy, "SynPS/2 Synaptics TouchPad", False);
      if (info)
      {
        executeCommand(dpy, info, "Synaptics Grab Event Device", 0);
        // Need to restart the device to apply changes
        executeCommand(dpy, info, "Device Enabled", 0);
        executeCommand(dpy, info, "Device Enabled", 1);
      }
      XCloseDisplay(dpy);
    }
  }
}
