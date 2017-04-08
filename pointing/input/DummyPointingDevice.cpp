/* -*- mode: c++ -*-
 *
 * pointing/input/DummyPointingDevice.cpp --
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

#include <pointing/input/DummyPointingDevice.h>

#include <sstream>

#define DUMMY_DEFAULT_CPI 400
#define DUMMY_DEFAULT_HZ 125

namespace pointing {

  DummyPointingDevice::DummyPointingDevice(URI uri) {
    cpi = DUMMY_DEFAULT_CPI ;
    hz = DUMMY_DEFAULT_HZ ;
    URI::getQueryArg(uri.query, "hz", &hz) ;
    if (!URI::getQueryArg(uri.query, "cpi", &cpi))
      URI::getQueryArg(uri.query, "dpi", &cpi) ;
  }

  URI
  DummyPointingDevice::getURI(bool expanded, bool /*crossplatform*/) const {
    URI uri ;
    uri.scheme = "dummy" ;
    int i = 0 ;
    std::stringstream q ;
    if (expanded || hz!=DUMMY_DEFAULT_HZ) 
      q << (i++?"&":"") << "hz=" << hz ;
    if (expanded || cpi!=DUMMY_DEFAULT_CPI) 
      q << (i++?"&":"") << "cpi=" << cpi ;
    uri.query = q.str() ;
    return uri ;
  }

}
