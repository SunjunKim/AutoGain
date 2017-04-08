/* -*- mode: c++ -*-
 *
 * pointing/input/windows/winPointingDevice.cpp --
 *
 * Initial software
 * Authors: Damien Marchal, Izzat Mukhanov
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */
/* See file LICENSE in the top-directory of the project. */


#include <pointing/input/windows/winPointingDevice.h>
#include <pointing/input/windows/winPointingDeviceManager.h>

namespace pointing {
  
  winPointingDevice::winPointingDevice(URI uri) : SystemPointingDevice(uri) {
    winPointingDeviceManager *man = static_cast<winPointingDeviceManager *>(PointingDeviceManager::get());
    man->addPointingDevice(this);
  }

  void
  winPointingDevice::getAbsolutePosition(double *x, double *y) const {
    *x = lastX;
    *y = lastY;
  }

}
