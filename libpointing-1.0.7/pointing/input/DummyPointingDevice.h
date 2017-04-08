/* -*- mode: c++ -*-
 *
 * pointing/input/DummyPointingDevice.h --
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

#ifndef DummyPointingDevice_h
#define DummyPointingDevice_h

#include <pointing/input/PointingDevice.h>

namespace pointing {

  /**
   * @brief The DummyPointingDevice class is a pseudo-device class.
   * It is used for testing and debugging purposes.
   *
   * It can be instantiated with PointingDevice::create function with the following URI:
   * dummy:?cpi=800&hz=125
   */
  class DummyPointingDevice : public PointingDevice {

    int cpi, hz ;

  public:
  
    /**
     * @brief The constructor
     * @param uri Instantiates the device according to the given URI.
     */
    DummyPointingDevice(URI uri) ;

    double getResolution(double * /*defval*/=0) const { return cpi ; }

    double getUpdateFrequency(double * /*defval*/=0) const { return hz ; }

    URI getURI(bool expanded=false, bool crossplatform=true) const ;

    /**
     * @brief The callback function will never be executed.
     * The purpose of this class is to return specified resolution and update frequency
     * when queried.
     */
    void setPointingCallback(PointingCallback /*callback*/, void * /*context*/) {}

    ~DummyPointingDevice() {}

  } ;

}

#endif
