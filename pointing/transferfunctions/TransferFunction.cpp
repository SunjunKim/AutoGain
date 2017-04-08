/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/TransferFunction.cpp --
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

#include <pointing/transferfunctions/TransferFunction.h>
#include <pointing/transferfunctions/SubPixelFunction.h>

#ifdef POINTING_WIN
#include <pointing-win/transferfunctions/WindowsFunction.h>
#endif

#ifdef POINTING_OSX
#include <pointing-osx/transferfunctions/OSXFunction.h>
#endif

#ifdef POINTING_XORG
#include <pointing-xorg/transferfunctions/XorgFunction.h>
#endif

#include <pointing/transferfunctions/SubPixelFunction.h>
#include <pointing/transferfunctions/NaiveConstantFunction.h>
#include <pointing/transferfunctions/ConstantFunction.h>
#include <pointing/transferfunctions/SigmoidFunction.h>
#include <pointing/transferfunctions/Interpolation.h>

#include <pointing/transferfunctions/Composition.h>
#include <pointing/utils/FileUtils.h>

#ifdef __APPLE__
#include <pointing/transferfunctions/osx/osxSystemPointerAcceleration.h>
#endif

#ifdef __linux__
#include <pointing/transferfunctions/linux/xorgSystemPointerAcceleration.h>
#endif

#ifdef _WIN32
#include <pointing/transferfunctions/windows/winSystemPointerAcceleration.h>
#endif

#define   DEFAULT_INPUT_CPI     400.
#define   DEFAULT_OUTPUT_PPI    96.

#define   DEFAULT_OSX_SETTING       0.6875
#define   DEFAULT_WINDOWS_SLIDER    0
#define   DEFAULT_WINDOWS_EPP       true

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstdlib>

namespace pointing {

  std::list<std::string>
  TransferFunction::schemes(void) {
    std::list<std::string> result ;
    result.push_back("system") ;
#ifdef POINTING_WIN
    result.push_back("windows") ;
#endif
#ifdef POINTING_OSX
    result.push_back("osx") ;
#endif
#ifdef POINTING_XORG
    result.push_back("xorg") ;
#endif
    result.push_back("subpixel");
    result.push_back("constant") ;
    result.push_back("interpolation") ;
    result.push_back("naive") ;
    result.push_back("sigmoid") ;
    result.push_back("composition") ;
    return result ;
  }
  
  TransferFunction* 
  TransferFunction::create(const char* function_uri,
			   PointingDevice* input, DisplayDevice* output) {
    std::string uri ;
    if (function_uri) uri = function_uri ;
    return create(uri, input, output) ;
  }

  void TransferFunction::normalizeInput(int *dx, int *dy, PointingDevice *input) const {
    double resolution = DEFAULT_INPUT_CPI;
    double coef = DEFAULT_INPUT_CPI / input->getResolution(&resolution);
    if (coef > 0. && coef != 1.)
    {
      *dx = coef * *dx;
      *dy = coef * *dy;
    }
  }

  void TransferFunction::normalizeOutput(int *dx, int *dy, DisplayDevice *output) const {
    double resolution = DEFAULT_OUTPUT_PPI;
    double coef = output->getResolution(&resolution) / DEFAULT_OUTPUT_PPI;
    if (coef > 1.)
    {
      *dx = coef * *dx;
      *dy = coef * *dy;
    }
  }

  TransferFunction* 
  TransferFunction::create(std::string function_uri,
			   PointingDevice* input, DisplayDevice* output) {
    if (function_uri.empty()) {
      const char *default_function = getenv("POINTING_FUNCTION") ;
      if (default_function) function_uri = default_function ;
    }
    if (function_uri.empty()) 
      throw std::runtime_error("Can't create a TransferFunction from an empty URI...") ;

    URI uri(function_uri) ;
    return create(uri, input, output) ;
  }

  TransferFunction* 
  TransferFunction::create(URI &uri,
               PointingDevice* input, DisplayDevice* output) {

    if (uri.scheme=="system") {
#ifdef __APPLE__
      osxSystemPointerAcceleration sysAcc ;
      const char *target = uri.opaque.c_str();
      double setting = sysAcc.get(target) ;
      // setting arg has to be defined
      if (URI::getQueryArg(uri.query, "setting")) {
        URI::getQueryArg(uri.query, "setting", &setting) ;
        sysAcc.set(setting, target) ;
        setting = sysAcc.get(target) ;
      }
      uri.scheme = "osx" ;
      std::stringstream q ;
      q << "setting=" << setting ;
      uri.query = q.str() ;
#endif

#ifdef __linux__
      const char *xorg_display = 0 ;
      URI out_uri = output->getURI() ;
      if (out_uri.scheme=="xorgdisplay" && !out_uri.opaque.empty())
	xorg_display = out_uri.opaque.c_str() ;
      xorgSystemPointerAcceleration sysAcc(xorg_display) ;
      int num=0, den=0, thr=0 ;
      sysAcc.get(&num, &den, &thr) ;
      // num, den and thr args have to be defined
      if (URI::getQueryArg(uri.query, "num") && URI::getQueryArg(uri.query, "den") && URI::getQueryArg(uri.query, "thr")) {
	URI::getQueryArg(uri.query, "num", &num) ;
	URI::getQueryArg(uri.query, "den", &den) ;
	URI::getQueryArg(uri.query, "thr", &thr) ;
	sysAcc.set(num, den, thr) ;
      }
      sysAcc.get(&num, &den, &thr) ;
      uri.scheme = "xorg" ;
      std::stringstream q ;
      q << "num=" << num << "&den=" << den << "&thr=" << thr ;
      uri.query = q.str() ;
      std::cerr << uri.asString() << std::endl ;
#endif

#ifdef _WIN32

      winSystemPointerAcceleration sysAcc ;
      std::string winVersion;
      int sliderPosition;
      bool enhancePointerPrecision;
      // slider and epp args have to be defined
      if (URI::getQueryArg(uri.query, "slider") && URI::getQueryArg(uri.query, "epp")) {
        URI::getQueryArg(uri.query, "slider", &sliderPosition) ;
        //URI::getQueryArg(uri.query, "enhancePointerPrecision", &enhancePointerPrecision) ;
        URI::getQueryArg(uri.query, "epp", &enhancePointerPrecision) ;
        sysAcc.set(sliderPosition, enhancePointerPrecision);
      }
      sysAcc.get(&winVersion,&sliderPosition,&enhancePointerPrecision);
      uri.scheme = "windows";
      uri.opaque = winVersion;
      std::stringstream q ;
      q << "slider=" << sliderPosition << "&epp=" << enhancePointerPrecision;
      uri.query = q.str() ;
      std::cerr << uri.asString() << std::endl ; 
#endif
    }

#ifdef POINTING_WIN
    if (uri.scheme=="windows")
      return new WindowsFunction(uri, input, output) ;
#endif

#ifdef POINTING_OSX
    if (uri.scheme=="osx")
      return new OSXFunction(uri, input, output) ;
#endif

#ifdef POINTING_XORG
    if (uri.scheme=="xorg")
      return new XorgFunction(uri, input, output) ;
#endif

    // Replace with the corresponding interpolated tfs
    if (uri.scheme == "osx" || uri.scheme == "windows")
    {
      std::string modulePath = moduleHeadersPath();
      std::string subDir;
      if (uri.scheme == "osx")
      {
        double setting = DEFAULT_OSX_SETTING;
        URI::getQueryArg(uri.query, "setting", &setting);
        URI::addQueryArg(uri.query, "f", setting);
        subDir = "/pointing-echomouse/darwin-16";
      }
      else
      {
        int slider = DEFAULT_WINDOWS_SLIDER;
        bool epp = DEFAULT_WINDOWS_EPP;
        URI::getQueryArg(uri.query, "slider", &slider);
        URI::addQueryArg(uri.query, "f", slider);
        URI::getQueryArg(uri.query, "epp", &epp);
        subDir = "/pointing-echomouse/windows" + std::string(epp ? "/epp" : "/no-epp");
      }
      uri.opaque = "";
      uri.path = modulePath + subDir;
      uri.scheme = "interp";
    }

    if (uri.scheme=="interp")
      return new Interpolation(uri, input, output) ;

    if (uri.scheme=="subpixel")
      return new SubPixelFunction(uri, input, output);

    if (uri.scheme=="constant")
      return new ConstantFunction(uri, input, output) ;

    if (uri.scheme=="naive")
      return new NaiveConstantFunction(uri, input, output) ;

    if (uri.scheme=="sigmoid")
      return new SigmoidFunction(uri, input, output) ;

    if (uri.scheme=="composition")
      return new Composition(uri, input, output) ;

    throw std::runtime_error(std::string("Unsupported function: ")+uri.asString()) ;
  }

}
