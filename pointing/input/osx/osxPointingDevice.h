/* -*- mode: c++ -*-
 *
 * pointing/input/osx/osxPointingDevice.h --
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

#ifndef osxPointingDevice_h
#define osxPointingDevice_h

#include <pointing/input/SystemPointingDevice.h>

namespace pointing {

  class osxPointingDevice : public SystemPointingDevice
  {
    friend class osxPointingDeviceManager;
    double cpi = -1., hz = -1.;
    bool seize = false;

  public:
    osxPointingDevice(URI device_uri);

    bool isUSB(void);
    bool isBluetooth(void);

    double getResolution(double *defval=0) const override;
    double getUpdateFrequency(double *defval=0) const override;

    URI getURI(bool expanded, bool crossplatform) const override;
  };
}

#endif
