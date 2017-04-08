/* -*- mode: c++ -*-
 *
 * pointing/output/DummyDisplayDevice.cpp --
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

#include <pointing/output/DummyDisplayDevice.h>

#include <sstream>

namespace pointing {

#define DUMMY_DEFAULT_HZ  0
#define DUMMY_DEFAULT_PPI 0
#define DUMMY_DEFAULT_BX  0
#define DUMMY_DEFAULT_BY  0
#define DUMMY_DEFAULT_BW  0
#define DUMMY_DEFAULT_BH  0
#define DUMMY_DEFAULT_W   0
#define DUMMY_DEFAULT_H   0

  DummyDisplayDevice::DummyDisplayDevice(void) {
    size.width = DUMMY_DEFAULT_W ;
    size.height = DUMMY_DEFAULT_H ;
    bounds.origin.x = DUMMY_DEFAULT_BX ;
    bounds.origin.y = DUMMY_DEFAULT_BY ;
    bounds.size.width = DUMMY_DEFAULT_BW ;
    bounds.size.height = DUMMY_DEFAULT_BH ;
    refreshrate = DUMMY_DEFAULT_HZ ;
    resolution = DUMMY_DEFAULT_PPI ;
  }

  DummyDisplayDevice::DummyDisplayDevice(URI uri) {
    size.width = DUMMY_DEFAULT_W ;
    size.height = DUMMY_DEFAULT_H ;
    bounds.origin.x = DUMMY_DEFAULT_BX ;
    bounds.origin.y = DUMMY_DEFAULT_BY ;
    bounds.size.width = DUMMY_DEFAULT_BW ;
    bounds.size.height = DUMMY_DEFAULT_BH ;
    refreshrate = DUMMY_DEFAULT_HZ ;
    resolution = DUMMY_DEFAULT_PPI ;
    URI::getQueryArg(uri.query, "hz", &refreshrate) ;
    URI::getQueryArg(uri.query, "bx", &bounds.origin.x) ;
    URI::getQueryArg(uri.query, "by", &bounds.origin.y) ;
    URI::getQueryArg(uri.query, "bw", &bounds.size.width) ;
    URI::getQueryArg(uri.query, "bh", &bounds.size.height) ;
    URI::getQueryArg(uri.query, "w", &size.width) ;
    URI::getQueryArg(uri.query, "h", &size.height) ;
    if (!URI::getQueryArg(uri.query, "ppi", &resolution))
      URI::getQueryArg(uri.query, "dpi", &resolution) ;
  }

  double
  DummyDisplayDevice::getResolution(double *hdpi, double *vdpi, double * /*defval*/) {
    if (resolution>0) {
      if (hdpi) *hdpi = resolution ;
      if (vdpi) *vdpi = resolution ;
      return resolution ;
    }
    return DisplayDevice::getResolution(hdpi, vdpi) ;
  }

  URI
  DummyDisplayDevice::getURI(bool expanded) const {
    URI uri ;
    uri.scheme = "dummy" ;
    std::stringstream q ;
    q << "hz=" << refreshrate ;
    if (resolution>0)
      q << "&ppi=" << resolution ;
    if (expanded || bounds.origin.x!=DUMMY_DEFAULT_BX || bounds.origin.y!=DUMMY_DEFAULT_BY)
      q << "&bx=" << bounds.origin.x << "&by=" << bounds.origin.y ;
    if (expanded || bounds.size.width!=DUMMY_DEFAULT_BW || bounds.size.height!=DUMMY_DEFAULT_BH)
      q << "&bw=" << bounds.size.width << "&bh=" << bounds.size.height ;
    if (expanded || size.width!=DUMMY_DEFAULT_W || size.height!=DUMMY_DEFAULT_H)
      q << "&w=" << size.width << "&h=" << size.height ;
    uri.query = q.str() ;
    return uri ;
  }

}
