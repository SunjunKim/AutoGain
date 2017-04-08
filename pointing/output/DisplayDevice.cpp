/* -*- mode: c++ -*-
 *
 * pointing/output/DisplayDevice.cpp --
 *
 * Initial software
 * Authors: Géry Casiez, Nicolas Roussel
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/utils/URI.h>
#include <pointing/output/DisplayDevice.h>
#include <pointing/output/DummyDisplayDevice.h>

#ifdef __APPLE__
#include <pointing/output/osx/osxDisplayDevice.h>
#endif

#ifdef __linux__
#include <pointing/output/linux/xorgDisplayDevice.h>
#endif

#ifdef _WIN32
#include <pointing/output/windows/winDisplayDevice.h>
#endif

#include <stdexcept>
#include <sstream>
#include <cmath>
#include <cstdlib>

namespace pointing {

  DisplayDevice *
  DisplayDevice::create(const char *device_uri) {
    std::string uri ;
    if (device_uri) uri = device_uri ;
    return create(uri) ;
  }

  DisplayDevice *
  DisplayDevice::create(std::string device_uri) {
    if (device_uri.empty() || device_uri.find("default:")!=std::string::npos) {
      const char *default_device = getenv("DISPLAY_DEVICE") ;
      device_uri = default_device ? default_device : "" ;
    }

    if (device_uri.empty()) 
      device_uri = (char *)"any:?debugLevel=1" ;

    URI uri(device_uri) ;
    // uri.debug(std::cerr) ;

    bool anywilldo = uri.scheme=="any" ;

#ifdef __APPLE__
    if (anywilldo || uri.scheme=="cgdisplay" || uri.scheme=="osxdisplay")
      return new osxDisplayDevice(uri) ;
#endif

#ifdef __linux__
    if (anywilldo || uri.scheme=="xorgdisplay")
      return new xorgDisplayDevice(uri) ;
#endif

#ifdef _WIN32
    if (anywilldo || uri.scheme=="windisplay")
      return new winDisplayDevice(uri) ;
#endif

    if (anywilldo || uri.scheme=="dummy")
      return new DummyDisplayDevice(uri) ;

    std::stringstream msg ;
    msg << "Unsupported display device: \"" << device_uri << "\"" ;
    throw std::runtime_error(msg.str()) ;
  }

  double
  DisplayDevice::getResolution(double *hppiptr, double *vppiptr, double * /*defval*/) {
    const double mmPerInch = 25.4 ;
    Bounds bounds = getBounds() ; 
    Size size = getSize() ;
    double wInInches = size.width/mmPerInch ;
    double hInInches = size.height/mmPerInch ;
    double hPPI = bounds.size.width / wInInches ;
    double vPPI = bounds.size.height / hInInches ;
    if (hppiptr) *hppiptr = hPPI ;
    if (vppiptr) *vppiptr = vPPI ;
#if 0
    double PPI = (hPPI+vPPI)/2.0 ;
#else
    double dInInches = sqrt(wInInches*wInInches + hInInches*hInInches) ;
    double PPI = sqrt(bounds.size.width*bounds.size.width + bounds.size.height*bounds.size.height) / dInInches ;
#endif
    return PPI ;
  }

}
