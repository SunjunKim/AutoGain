/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/SigmoidFunction.h --
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

#ifndef SigmoidFunction_h
#define SigmoidFunction_h

#include <pointing/transferfunctions/TransferFunction.h>
#include <pointing/utils/URI.h>

/*
  The sigmoid function is designed in the pointing
  device speed, gain space

Gain   /\
       |
       |
  Gmax +       -----------------------
       |      /
       |     /
  Gmin +----/
       |
       +----+--+-----------------------
           V1  V2      MouseSpeed (m/s)
 */

namespace pointing {

  /**
   * @brief The SigmoidFunction class implements the transfer function which is
   * an discrete approximation of the sigmoid function.
   *
   * In this case 2 mouse speeds are used as boundaries which are mapped onto
   * 2 gain boundaries such that if the mouse speed is in-between 2 speed boundaries,
   * then the corresponding gain value is computed by interpolating min and max gain values.
   * Otherwise, minimum or maximum gain value is chosen.
   */
  class SigmoidFunction : public TransferFunction {

  private:

    float Gmin;
    float Gmax;
    float V1; // m/s
    float V2; // m/s

    bool disableSubPixelation;

    int previousMouseRawX;
    int previousMouseRawY;
    float previousMouseXRemainder;
    float previousMouseYRemainder;

    PointingDevice* pointingDevice;
    DisplayDevice* displayDevice;

  public:

    SigmoidFunction(URI &uri, PointingDevice* input, DisplayDevice* output);

    void clearState(void) ;
    void applyi(int dxMickey, int dyMickey, int *dxPixel, int *dyPixel,
	       TimeStamp::inttime timestamp=TimeStamp::undef) ;
    void applyd(int dxMickey, int dyMickey, double *dxPixel, double *dyPixel,
           TimeStamp::inttime timestamp=TimeStamp::undef) ;

    URI getURI(bool expanded=false) const ;

    ~SigmoidFunction() {}

  }; 

}

#endif
