/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/ConstantFunction.cpp --
 *
 * Initial software
 * Authors: G?y Casiez, Nicolas Roussel
 * Copyright ?Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/transferfunctions/ConstantFunction.h>

#include <iostream>
#include <sstream>
#include <cmath>

namespace pointing {

#define Sign(X) ((X>0)?(1):(-1))

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define abs fabs

#define CONSTANT_DEFAULT_CDGAIN 4.0f
#define CONSTANT_DEFAULT_NOSUBPIX false

  ConstantFunction::ConstantFunction(URI &uri, PointingDevice* input, DisplayDevice* output) {
    pointingDevice = input;
    displayDevice = output;

    previousMouseRawX = 0;
    previousMouseRawY = 0;
    previousMouseXRemainder = 0.0;
    previousMouseYRemainder = 0.0;

    CDGain = CONSTANT_DEFAULT_CDGAIN ;
    disableSubPixelation = CONSTANT_DEFAULT_NOSUBPIX ;
    URI::getQueryArg(uri.query, "gain", &CDGain) ;
    URI::getQueryArg(uri.query, "cdgain", &CDGain) ;
    URI::getQueryArg(uri.query, "nosubpix", &disableSubPixelation) ;
  }

  void
  ConstantFunction::clearState(void) {
    previousMouseRawX = 0;
    previousMouseRawY = 0;
    previousMouseXRemainder = 0.0;
    previousMouseYRemainder = 0.0;
  }

  void
  ConstantFunction::applyi(int mouseRawX, int mouseRawY, int *mouseX, int *mouseY,
			       TimeStamp::inttime /*timestamp*/) {

    float pixelGain = CDGain * (float) displayDevice->getResolution() /
      (float) pointingDevice->getResolution() ;
 
    // Calculate accelerated mouse deltas
    float mouseXplusRemainder = mouseRawX * pixelGain + previousMouseXRemainder;
    float mouseYplusRemainder = mouseRawY * pixelGain + previousMouseYRemainder;
  
    // Split mouse delta into integer part (applied now) and remainder part (saved for next time)
    // (NOTE: Only when disableSubPixelation==true does this have any significant or cumulative effect)

    // If the direction of the translation is changed then remainder is set to 0.
    if (disableSubPixelation && fabs(mouseXplusRemainder) <= abs((float)mouseRawX)) {
      *mouseX = mouseRawX;
      previousMouseXRemainder = 0.0;
    } else {
      *mouseX = (int)floor(mouseXplusRemainder);
      previousMouseXRemainder = mouseXplusRemainder - *mouseX;
    }
  
    if (disableSubPixelation && fabs(mouseYplusRemainder) <= abs((float)mouseRawY)) {
      *mouseY = mouseRawY;
      previousMouseYRemainder = 0.0;
    } else {
      *mouseY = (int)floor(mouseYplusRemainder);
      previousMouseYRemainder = mouseYplusRemainder - *mouseY;
    }

#if 0
    std::cerr << "ConstantFunction " << this << ": "
	      << mouseRawX << "," << mouseRawY << " "
	      << "gain=" << pixelGain << " "
	      << *mouseX << "," << *mouseY
	      << " " << disableSubPixelation
	      << std::endl ;
#endif
  }

  void
  ConstantFunction::applyd(int mouseRawX, int mouseRawY, double *mouseX, double *mouseY,
                           TimeStamp::inttime /*timestamp*/) {
    double pixelGain = CDGain * (double) displayDevice->getResolution() /
      (double) pointingDevice->getResolution() ;

    *mouseX = mouseRawX * pixelGain;
    *mouseY = mouseRawY * pixelGain;
  }

  URI
  ConstantFunction::getURI(bool expanded) const {
    URI uri ;
    uri.scheme = "constant" ;
    int i = 0 ;
    std::stringstream q ;
    if (expanded || CDGain!=CONSTANT_DEFAULT_CDGAIN) 
      q << (i++?"&":"") << "cdgain=" << CDGain ;
    if (expanded || disableSubPixelation!=CONSTANT_DEFAULT_NOSUBPIX) 
      q << (i++?"&":"") << "nosubpix=" << (disableSubPixelation?"true":"false") ;
    uri.query = q.str() ;
    return uri ;
  }

}
