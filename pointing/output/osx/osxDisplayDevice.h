/* -*- mode: c++ -*-
 *
 * pointing/output/osx/osxDisplayDevice.h --
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

#ifndef osxDisplayDevice_h
#define osxDisplayDevice_h

#include <pointing/output/DisplayDevice.h>

#include <ApplicationServices/ApplicationServices.h>

namespace pointing {

  /**
   * @brief The osxDisplayDevice class is a platform specific subclass of DisplayDevice
   * which is implemented to work with Display devices on the osx platforms.
   */
  class osxDisplayDevice : public DisplayDevice {

    CGDirectDisplayID displayID ;

    typedef enum {NOTHING=0, BOUNDS=1, SIZE=2, REFRESHRATE=4} cachedinfo ;
    int cached ;

    /**
     * @brief These parameters are cached whenever they are requested
     * the first time for a new display device.
     */
    //@{
    DisplayDevice::Bounds cached_bounds ;
    DisplayDevice::Size cached_size ;
    double cached_refreshrate ;
    //@}

    /**
     * @brief Caches Bounds, Size and RefreshRate of the Display
     */
    void cacheAll(URI &uri);

    /**
     * @brief Lists all the avilable displays.
     */
    void listDisplays(std::ostream& out) ;

  public:
  
    osxDisplayDevice(void) ;
    osxDisplayDevice(URI uri) ;
    osxDisplayDevice(CGDirectDisplayID did) ;

    Bounds getBounds(Bounds *defval=0) ;
    Size getSize(Size *defval=0) ;
    double getRefreshRate(double *defval=0) ;

    URI getURI(bool expanded=false) const ;

  } ;

}

#endif
