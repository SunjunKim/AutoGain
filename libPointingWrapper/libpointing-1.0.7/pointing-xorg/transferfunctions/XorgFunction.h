/* -*- mode: c++ -*-
 *
 * pointing-xorg/transferfunctions/XorgFunction.h --
 *
 * Initial software
 * Authors: Nicolas Roussel
 * Copyright ? Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#ifndef XorgFunction_h
#define XorgFunction_h

#include <pointing/transferfunctions/TransferFunction.h>
#include <pointing/utils/URI.h>

#include <string>

#include <stdint.h>

struct _DeviceIntRec ;

namespace pointing {

  /**
   * @brief The XorgFunction class implements the Xorg cursor
   * configuration as closely as possible.
   *
   * This class wraps up the publicly available source code
   * for pointer acceleration mechanisms. It allows a user to work with
   * schemes like "predicatable" or "lightweight" and
   * different acceleration profiles.
   *
   * It does not replicate the Xorg transfer functions but uses them
   * directly from the API.
   */
  class XorgFunction : public TransferFunction {

    PointingDevice *input ;
    DisplayDevice *output ;

    /**
     * @brief The reference time from which the elapsed time is computed
     * to maintain better estimation of the real pointing device velocity.
     */
    TimeStamp::inttime epoch ;

    /**
     * @brief The structure which is used to define the parameters
     * to work with Xorg API.
     */
    struct ::_DeviceIntRec *dev ;

    int ctrl_num ;
    int ctrl_den ;
    int ctrl_threshold ;
    int scheme ;
    int predictableProfile ;
    float corr_mul ;
    float const_acceleration ;
    float min_acceleration ;
    bool normalize;

    URI baseuri ;

  public:

    XorgFunction(URI &uri, PointingDevice* input, DisplayDevice* output) ;

    void clearState(void) ;

    void applyi(int dxMickey, int dyMickey, int *dxPixel, int *dyPixel,
           TimeStamp::inttime timestamp=TimeStamp::undef) ;
    void applyd(int dxMickey, int dyMickey, double *dxPixel, double *dyPixel,
           TimeStamp::inttime timestamp=TimeStamp::undef) ;

    URI getURI(bool expanded=false) const ;

    ~XorgFunction(void) ;

  } ;

}

#endif
