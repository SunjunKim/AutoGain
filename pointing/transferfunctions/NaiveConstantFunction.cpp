/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/NaiveConstantFunction.cpp --
 *
 * Initial software
 * Authors: Nicolas Roussel
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/transferfunctions/NaiveConstantFunction.h>

#include <sstream>

namespace pointing {

  NaiveConstantFunction::NaiveConstantFunction(URI &uri, 
					       PointingDevice* /*input*/,
					       DisplayDevice* /*output*/) {
    gain = 1.0 ;
    URI::getQueryArg(uri.query, "gain", &gain) ;
    URI::getQueryArg(uri.query, "cdgain", &gain) ;
  }

  void NaiveConstantFunction::applyi(int dxMickey, int dyMickey, int *dxPixel, int *dyPixel,
				    TimeStamp::inttime /*timestamp*/) {
    *dxPixel = dxMickey * gain ;
    *dyPixel = dyMickey * gain ;

#if 0
    std::cerr << "NaiveConstantFunction " << this << ": "
	      << dxMickey << "," << dyMickey << " "
	      << "gain=" << gain << " "
	      << *dxPixel << "," << *dyPixel
	      << std::endl ;
#endif
  }

  void NaiveConstantFunction::applyd(int dxMickey, int dyMickey, double *dxPixel, double *dyPixel,
                    TimeStamp::inttime /*timestamp*/) {
    *dxPixel = dxMickey * gain ;
    *dyPixel = dyMickey * gain ;
  }

  URI
  NaiveConstantFunction::getURI(bool /*expanded*/) const {
    URI uri ;
    uri.scheme = "naive" ;
    std::stringstream q ;
    q << "cdgain=" << gain ;
    uri.query = q.str() ;
    return uri ;
  }

}
