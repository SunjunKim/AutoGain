/* -*- mode: c++ -*-
 *
 * pointing/output/DummyDisplayDevice.h --
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

#ifndef DummyDisplayDevice_h
#define DummyDisplayDevice_h

#include <pointing/output/DisplayDevice.h>

namespace pointing {
  /**
   * @brief The DummyDisplayDevice class is a pseudo-device class.
   * It stores configuration values passed on the query string and simulates
   * the display behavior.
   */

  class DummyDisplayDevice : public DisplayDevice {

    DisplayDevice::Bounds bounds ;
    DisplayDevice::Size size ;
    int refreshrate ;
    double resolution ;

  public:

    DummyDisplayDevice(void) ;
    DummyDisplayDevice(URI uri) ;
  
    /**
     * @brief setBounds
     * @param b is the bounds of the display expressed in pixels.
     */
    void setBounds(Bounds b) { bounds = b ; }

    /**
     * @brief setSize
     * @param s is the size of the display expressed in mms.
     */
    void setSize(Size s) { size = s ; }

    /**
     * @brief setRefreshRate
     * @param r represents the refresh rate of the display (Hz).
     */
    void setRefreshRate(int r) { refreshrate = r ; }

    /**
     * @brief setResolution
     * @param r is the pixel density in PPI.
     */
    void setResolution(double r) { resolution = r ; }

    Bounds getBounds(Bounds * /*defval*/=0) { return bounds ; }
    Size getSize(Size * /*defval*/=0) { return size ; }
    double getRefreshRate(double * /*defval*/=0) { return refreshrate ; }
    double getResolution(double *hdpi=0, double *vdpi=0, double *defval=0) ;

    URI getURI(bool expanded=false) const ;

  } ;

}

#endif
