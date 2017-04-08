/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/linux/xorgSystemPointerAcceleration.cpp --
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

#include <pointing/transferfunctions/linux/xorgSystemPointerAcceleration.h>

#include <iostream>
#include <stdexcept>

namespace pointing {

  xorgSystemPointerAcceleration::xorgSystemPointerAcceleration(const char *display) {
#if 0
    if (display)
      std::cerr << "xorgSystemPointerAcceleration: opening " << display << std::endl ;
#endif
    dpy = XOpenDisplay(display) ;
    if (dpy == NULL)
      throw std::runtime_error("xorgSystemPointerAcceleration: can't open display") ;
  }

  void
  xorgSystemPointerAcceleration::get(int *accel_numerator, int *accel_denominator,
				     int *threshold) {
    XGetPointerControl(dpy, accel_numerator, accel_denominator, threshold) ;
  }

  void
  xorgSystemPointerAcceleration::set(int accel_numerator, int accel_denominator, 
				     int threshold) {
    XChangePointerControl(dpy, 1, 1, accel_numerator, accel_denominator, threshold) ;
  }

  xorgSystemPointerAcceleration::~xorgSystemPointerAcceleration() {
    XCloseDisplay (dpy) ;
  }

}
