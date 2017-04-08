/* -*- mode: c++ -*-
 *
 * pointing/input/osx/osxPointingDevice.cpp --
 *
 * Initial software
 * Authors: Nicolas Roussel, Izzatbek Mukhanov
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/input/osx/osxPointingDevice.h>
#include <pointing/input/osx/osxPointingDeviceManager.h>

namespace pointing {

  osxPointingDevice::osxPointingDevice(URI device_uri) : SystemPointingDevice(device_uri)
  {
    URI::getQueryArg(device_uri.query, "seize", &seize);
    osxPointingDeviceManager *man = static_cast<osxPointingDeviceManager *>(PointingDeviceManager::get());
    man->addPointingDevice(this);
  }

  bool osxPointingDevice::isUSB(void)
  {
    return uri.path.find("/USB") == 0;
  }

  bool osxPointingDevice::isBluetooth(void)
  {
    return uri.path.find("/Bluetooth") == 0;
  }

  double osxPointingDevice::getResolution(double *defval) const
  {
    if (forced_cpi > 0)
      return forced_cpi;

    if (cpi > 0)
      return cpi;

    return SystemPointingDevice::getResolution(defval);
  }

  double osxPointingDevice::getUpdateFrequency(double *defval) const
  {
    if (forced_hz > 0)
      return forced_hz;

    if (hz > 0)
      return hz;

    return SystemPointingDevice::getUpdateFrequency(defval);
  }

  URI osxPointingDevice::getURI(bool expanded, bool crossplatform) const
  {
    URI result = SystemPointingDevice::getURI(expanded, crossplatform);

    if (expanded || seize)
        URI::addQueryArg(result.query, "seize", seize);

    return result;
  }
}
