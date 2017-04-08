/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/Composition.cpp --
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

#include <pointing/transferfunctions/Composition.h>

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace pointing {

  Composition::Composition(URI &uri, PointingDevice* input, DisplayDevice* output) {
    debugLevel = 0 ;
    URI::getQueryArg(uri.query, "debugLevel", &debugLevel) ;

    this->input = input ;
    this->output = output ;

    std::string filename = uri.opaque!="" ? uri.opaque : uri.path ;
    if (filename.empty()) return ;

    std::string func_uri ;
    std::ifstream comp_file(filename.c_str()) ;
    if (comp_file.is_open()) {
      while (comp_file.good()) {
	getline (comp_file, func_uri) ;
	if (!func_uri.empty()) {
	  if (debugLevel)
	    std::cerr << "Composition::Composition: creating " << func_uri << std::endl;
	  appendFunction(func_uri) ;
	}
      }
      comp_file.close() ;
    } else {
      std::stringstream msg ;
      msg << "Composition: unable to open file " << filename ;
      throw std::runtime_error(msg.str()) ;
    }
  }

  void
  Composition::prependFunction(std::string s) {
    URI uri(s) ;
    prependFunction(uri) ;
  }

  void
  Composition::prependFunction(URI &uri) {
    prependFunction(TransferFunction::create(uri, input, output)) ;
  }

  void
  Composition::prependFunction(TransferFunction *function) {
    if (function) functions.push_front(function) ;
  }

  void
  Composition::appendFunction(std::string s) {
    URI uri(s) ;
    appendFunction(uri) ;
  }

  void
  Composition::appendFunction(URI &uri) {
    appendFunction(TransferFunction::create(uri, input, output)) ;
  }

  void
  Composition::appendFunction(TransferFunction *function) {
    if (function) functions.push_back(function) ;
  }

  void
  Composition::applyi(int dxMickey, int dyMickey, int *dxPixel, int *dyPixel,
		     TimeStamp::inttime timestamp) {
    for (std::list<TransferFunction*>::iterator f=functions.begin();
	 f!=functions.end(); ++f) {
      TransferFunction *function = (*f) ;
      function->applyi(dxMickey, dyMickey, dxPixel, dyPixel, timestamp) ;
      if (debugLevel) {
	std::cerr << "Composition: " << function->getURI().asString() << std::endl ;
	std::cerr << "   " << dxMickey << " " << dyMickey << std::endl ;
	std::cerr << "   " << *dxPixel << " " << *dyPixel << std::endl ;
      }
      dxMickey = *dxPixel ;
      dyMickey = *dyPixel ;
    }
    if (debugLevel) std::cerr << std::endl ;
  }

  void
  Composition::applyd(int dxMickey, int dyMickey, double *dxPixel, double *dyPixel,
             TimeStamp::inttime timestamp) {
    for (std::list<TransferFunction*>::iterator f=functions.begin();
     f!=functions.end(); ++f) {
      TransferFunction *function = (*f) ;
      function->applyd(dxMickey, dyMickey, dxPixel, dyPixel, timestamp) ;
      if (debugLevel) {
    std::cerr << "Composition: " << function->getURI().asString() << std::endl ;
    std::cerr << "   " << dxMickey << " " << dyMickey << std::endl ;
    std::cerr << "   " << *dxPixel << " " << *dyPixel << std::endl ;
      }
      dxMickey = *dxPixel ;
      dyMickey = *dyPixel ;
    }
    if (debugLevel) std::cerr << std::endl ;
  }

  URI
  Composition::getURI(bool /*expanded*/) const {
    URI uri ;
    uri.scheme = "composition" ;

    std::stringstream path ;
    for (std::list<TransferFunction*>::const_iterator f=functions.begin();
	 f!=functions.end(); ++f) {
      URI f_uri = (*f)->getURI() ;
      path << "/" << f_uri.scheme ;
    }
    uri.path = path.str() ;
  
    return uri ;
  }

  Composition::~Composition(void) {
    while (!functions.empty()) {
      TransferFunction *function = functions.front() ;
      functions.pop_front() ;
      delete function ;
    }
  }

}
