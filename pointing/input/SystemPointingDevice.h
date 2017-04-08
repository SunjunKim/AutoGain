/* -*- mode: c++ -*-
 *
 * pointing/input/SystemPointingDevice.h --
 *
 * Initial software
 * Authors: Izzat Mukhanov
 * Copyright � Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#ifndef SystemPointingDevice_h
#define SystemPointingDevice_h

#include <pointing/input/PointingDevice.h>

namespace pointing {

  /**
   * @brief The SystemPointingDevice class is used to represent Pointing Devices connected to the computer.
   *
   * It defines general behavior for all three platforms.
   */
  class SystemPointingDevice : public PointingDevice
  {
  protected:
    friend class PointingDeviceManager;

    URI uri, anyURI;

    double forced_cpi = -1., forced_hz = -1.;

    int vendorID = 0, productID = 0;

    int debugLevel = 0;

    PointingCallback callback = NULL;
    void *callback_context = NULL;

    std::string vendor, product;
    bool active = false;

  public:

    SystemPointingDevice(URI uri) ;

    bool isActive(void) const;

    int getVendorID(void) const;
    std::string getVendor(void) const;
    int getProductID(void) const;
    std::string getProduct(void) const;

    double getResolution(double *defval=0) const;
    double getUpdateFrequency(double *defval=0) const;

    URI getURI(bool expanded=false, bool crossplatform=false) const;

    void setPointingCallback(PointingCallback callback, void *context=0);

    void setDebugLevel(int level);

    virtual ~SystemPointingDevice();
  } ;

}

#endif
