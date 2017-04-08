/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/ConstantFunction.h --
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

#ifndef ConstantFunction_h
#define ConstantFunction_h

#include <pointing/transferfunctions/TransferFunction.h>

namespace pointing {

  /**
   * @brief The ConstantFunction class implements the transfer function with a constant
   * gain factor computed taking into account device and display resolutions
   * in order to achieve precise mapping.
   */
  class ConstantFunction : public TransferFunction {

  private:

    float CDGain;
    bool disableSubPixelation;

    int previousMouseRawX;
    int previousMouseRawY;
    float previousMouseXRemainder;
    float previousMouseYRemainder;

    PointingDevice* pointingDevice;
    DisplayDevice* displayDevice;

  public:

    ConstantFunction(URI &uri, PointingDevice* input, DisplayDevice* output);

    void clearState(void) ;
    void applyi(int dxMickey, int dyMickey, int *dxPixel, int *dyPixel,
	       TimeStamp::inttime timestamp=TimeStamp::undef) ;
    void applyd(int dxMickey, int dyMickey, double *dxPixel, double *dyPixel,
           TimeStamp::inttime =TimeStamp::undef) ;

    URI getURI(bool expanded=false) const ;

    ~ConstantFunction() {}

  } ; 

}

#endif
