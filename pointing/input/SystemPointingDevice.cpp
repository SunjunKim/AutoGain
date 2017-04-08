/* -*- mode: c++ -*-
 *
 * pointing/input/linux/SystemPointingDevice.cpp --
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

#include <pointing/input/SystemPointingDevice.h>
#include <pointing/input/PointingDeviceManager.h>

namespace pointing {

  #define DEFAULT_CPI       400.001
  #define DEFAULT_HZ        125.001

  SystemPointingDevice::SystemPointingDevice(URI uri):uri(uri)
  {
    URI::getQueryArg(uri.query, "cpi", &forced_cpi);
    URI::getQueryArg(uri.query, "hz", &forced_hz);
    URI::getQueryArg(uri.query, "debugLevel", &debugLevel);

    PointingDeviceManager *man = PointingDeviceManager::get();

    if (uri.scheme == "any")
    {
      anyURI = man->generalizeAny(uri);
      URI::getQueryArg(uri.query, "vendor", &vendorID);
      URI::getQueryArg(uri.query, "product", &productID);
    }
    else
      this->uri.generalize();
  }

  bool SystemPointingDevice::isActive(void) const
  {
    return active;
  }

  int SystemPointingDevice::getVendorID() const
  {
      return vendorID;
  }

  std::string SystemPointingDevice::getVendor() const
  {
      return vendor;
  }

  int SystemPointingDevice::getProductID() const
  {
      return productID;
  }

  std::string SystemPointingDevice::getProduct() const
  {
      return product;
  }

  double SystemPointingDevice::getResolution(double *defval) const
  {
    if (forced_cpi > 0) return forced_cpi;
    return defval ? *defval : DEFAULT_CPI;
  }

  double SystemPointingDevice::getUpdateFrequency(double *defval) const
  {
    if (forced_hz > 0) return forced_hz;
    double estimated = estimatedUpdateFrequency();
    if (estimated > 0.)
      return estimated;
    return defval ? *defval : DEFAULT_HZ;
  }

  URI SystemPointingDevice::getURI(bool expanded, bool crossplatform) const
  {
    URI result = uri;
    if (crossplatform)
    {
      if (anyURI.scheme.size())
        result = anyURI;
      else
      {
        int vendorID = getVendorID();
        if (vendorID)
          URI::addQueryArg(result.query, "vendor", vendorID) ;

        int productID = getProductID();
        if (productID)
          URI::addQueryArg(result.query, "product", productID) ;
      }
    }

    if (expanded || debugLevel)
        URI::addQueryArg(result.query, "debugLevel", debugLevel);
    if (expanded || forced_cpi > 0)
        URI::addQueryArg(result.query, "cpi", getResolution());
    if (expanded || forced_hz > 0)
        URI::addQueryArg(result.query, "hz", getUpdateFrequency());

    return result;
  }

  void SystemPointingDevice::setPointingCallback(PointingCallback cbck, void *ctx)
  {
    callback_context = ctx;
    callback = cbck;
  }

  void SystemPointingDevice::setDebugLevel(int level)
  {
    debugLevel = level;
  }

  SystemPointingDevice::~SystemPointingDevice()
  {
    PointingDeviceManager::get()->removePointingDevice(this);
  }
}
