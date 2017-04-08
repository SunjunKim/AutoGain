/* -*- mode: c++ -*-
 *
 * pointing/output/linux/xorgDisplayDevice.h --
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

#include <pointing/output/linux/xorgDisplayDevice.h>

#include <X11/extensions/Xrandr.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace pointing {

  xorgDisplayDevice::xorgDisplayDevice(void) {
    dpy = XOpenDisplay(0) ;
    if (dpy == NULL)
      throw std::runtime_error("xorgDisplayDevice: can't open display") ;
    cached = NOTHING ;
  }

  xorgDisplayDevice::xorgDisplayDevice(URI uri) {
    // uri.debug(std::cerr) ;

    if (uri.opaque.empty())
      dpy = XOpenDisplay(0) ;
    else
      // hostname:displaynumber.screennumber
      dpy = XOpenDisplay(uri.opaque.c_str()) ;
    if (dpy == NULL)
      throw std::runtime_error("xorgDisplayDevice: can't open display") ;
    cached = NOTHING ;
  }

  DisplayDevice::Bounds
  xorgDisplayDevice::getBounds(Bounds */*defval*/) {
    if (cached & BOUNDS) return cached_bounds ;

    int scr = DefaultScreen(dpy) ;
    cached_bounds = DisplayDevice::Bounds(0, 0,
					  DisplayWidth(dpy, scr),
					  DisplayHeight(dpy, scr)) ;
    cached = cached | BOUNDS ;

    return cached_bounds ;
  }

  DisplayDevice::Size
  xorgDisplayDevice::getSize(Size */*defval*/) {
    if (cached & SIZE) return cached_size ;

    int scr = DefaultScreen(dpy) ;
    cached_size = DisplayDevice::Size(DisplayWidthMM(dpy, scr),
				      DisplayHeightMM(dpy, scr)) ;
    cached = cached | SIZE ;

    return cached_size ;
  }

  double
  xorgDisplayDevice::getRefreshRate(double */*defval*/) {
    if (cached & REFRESHRATE) return cached_refreshrate ;

    Window root = RootWindow(dpy, DefaultScreen(dpy)) ;
    XRRScreenConfiguration *sc = XRRGetScreenInfo(dpy, root) ;
    cached_refreshrate = XRRConfigCurrentRate(sc) ;
    cached = cached | REFRESHRATE ;
  
    return cached_refreshrate ;
  }

  URI
  xorgDisplayDevice::getURI(bool /*expanded*/) const {
    URI uri ;
    uri.scheme = "xorgdisplay" ;
    uri.opaque = DisplayString(dpy) ;
    return uri ;
  }

  xorgDisplayDevice::~xorgDisplayDevice(void) {
    XCloseDisplay(dpy) ;
  }

}
