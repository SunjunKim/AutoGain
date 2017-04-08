/* -*- mode: c++ -*-
 *
 * pointing/input/linux/XinputHelper.h --
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

#include <X11/extensions/XInput.h>

namespace pointing
{
  XDeviceInfo* find_device_info(Display	*display,
                   const char		*name,
                   Bool		only_extended);

  void executeCommand(Display	*dpy, XDeviceInfo *info, const char *command, unsigned char data);

  void enableSynapticsTouchpad();
}
