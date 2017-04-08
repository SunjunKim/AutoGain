/* -*- mode: c++ -*-
 *
 * pointing/input/linux/linuxPointingDevice.cpp --
 *
 * Initial software
 * Authors: Nicolas Roussel, Izzat Mukhanov
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/input/linux/linuxPointingDevice.h>
#include <pointing/input/linux/linuxPointingDeviceManager.h>

namespace pointing {

  linuxPointingDevice::linuxPointingDevice(URI device_uri) : SystemPointingDevice(device_uri)
  {
    URI::getQueryArg(device_uri.query, "seize", &seize);
    linuxPointingDeviceManager *man = static_cast<linuxPointingDeviceManager *>(PointingDeviceManager::get());
    man->addPointingDevice(this);
  }

  double linuxPointingDevice::getUpdateFrequency(double *defval) const
  {
    if (forced_hz > 0)
      return forced_hz;

    if (hz > 0)
      return hz;

    return SystemPointingDevice::getUpdateFrequency(defval);
  }

  URI linuxPointingDevice::getURI(bool expanded, bool crossplatform) const
  {
    URI result = SystemPointingDevice::getURI(expanded, crossplatform);

    if (expanded || seize)
        URI::addQueryArg(result.query, "seize", seize);

    return result;
  }
}
