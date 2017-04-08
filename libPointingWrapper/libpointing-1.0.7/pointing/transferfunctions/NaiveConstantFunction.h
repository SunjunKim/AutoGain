/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/NaiveConstantFunction.h --
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

#ifndef NaiveConstantFunction_h
#define NaiveConstantFunction_h

#include <pointing/transferfunctions/TransferFunction.h>

namespace pointing {

  /**
   * @brief The NaiveConstantFunction class implements a transfer function
   * which simply multilies the input translation by the constant gain
   * without taking into account device characteristics.
   */
  class NaiveConstantFunction : public TransferFunction {

  private:

    /**
     * @brief gain A gain by which input is mutiplied.
     */
    float gain ;

  public:

    NaiveConstantFunction(URI &uri, PointingDevice* input, DisplayDevice* output);

    void clearState(void) {}

    void applyi(int dxMickey, int dyMickey, int *dxPixel, int *dyPixel,
	       TimeStamp::inttime timestamp=TimeStamp::undef) ;

    void applyd(int dxMickey, int dyMickey, double *dxPixel, double *dyPixel,
           TimeStamp::inttime timestamp=TimeStamp::undef) ;

    URI getURI(bool expanded=false) const ;

    ~NaiveConstantFunction() {}

  } ; 

}

#endif
