/* -*- mode: c++ -*-
 *
 * pointing/output/windows/winDisplayDevice.h --
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

#ifndef winDisplayDevice_h
#define winDisplayDevice_h

#include <pointing/output/DisplayDevice.h>
#include <pointing/output/windows/winDisplayDeviceManager.h>
#include <pointing/output/windows/winDisplayDeviceHelper.h>

namespace pointing {

  /**
   * @brief A Windows-specific subclass of DisplayDevice.
   */
  class winDisplayDevice : public DisplayDevice {

    std::wstring displayID ;
    winDisplayInfo dinfo ;

    friend class winDisplayDeviceManager;

  public:
  
    winDisplayDevice(void) ;
    winDisplayDevice(URI uri) ;

    Bounds getBounds(Bounds *defval=0) ;
    Size getSize(Size *defval=0) ;
    double getRefreshRate(double *defval=0) ;

    URI getURI(bool expanded=false) const ;

  } ;

}

#endif
