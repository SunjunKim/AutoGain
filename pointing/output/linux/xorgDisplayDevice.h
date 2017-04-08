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

#ifndef xorgDisplayDevice_h
#define xorgDisplayDevice_h

#include <pointing/output/DisplayDevice.h>

#include <X11/Xlib.h>

namespace pointing {

  class xorgDisplayDevice : public DisplayDevice {

    Display *dpy ;

    typedef enum {NOTHING=0, BOUNDS=1, SIZE=2, REFRESHRATE=4} cachedinfo ;
    int cached ;
    DisplayDevice::Bounds cached_bounds ;
    DisplayDevice::Size cached_size ;
    double cached_refreshrate ;

  public:
  
    xorgDisplayDevice(void) ;
    xorgDisplayDevice(URI uri) ;

    Bounds getBounds(Bounds *defval=0) ;
    Size getSize(Size *defval=0) ;
    double getRefreshRate(double *defval=0) ;

    URI getURI(bool expanded=false) const ;

    ~xorgDisplayDevice(void) ;

  } ;

}

#endif
