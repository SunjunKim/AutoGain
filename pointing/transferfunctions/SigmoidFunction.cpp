/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/SigmoidFunction.cpp --
 *
 * Initial software
 * Authors: Géry Casiez
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/transferfunctions/SigmoidFunction.h>

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

#define SIGMOID_DEFAULT_GMIN 1.0f
#define SIGMOID_DEFAULT_GMAX 6.0f
#define SIGMOID_DEFAULT_V1 0.05f
#define SIGMOID_DEFAULT_V2 0.2f
#define SIGMOID_DEFAULT_NOSUBPIX false

  SigmoidFunction::SigmoidFunction(URI &uri, PointingDevice* input, DisplayDevice* output) {
    pointingDevice = input;
    displayDevice = output;

    if (uri.scheme!="sigmoid")
      std::cerr << "SigmoidFunction warning: scheme is '" << uri.scheme << "', should be 'sigmoid'" << std::endl ;

    disableSubPixelation = SIGMOID_DEFAULT_NOSUBPIX ;
    URI::getQueryArg(uri.query, "nosubpix", &disableSubPixelation) ;

    Gmin = SIGMOID_DEFAULT_GMIN ;
    URI::getQueryArg(uri.query, "gmin", &Gmin) ;
    Gmax = SIGMOID_DEFAULT_GMAX ;
    URI::getQueryArg(uri.query, "gmax", &Gmax) ;
    if (Gmin > Gmax)
      std::cerr << "Error Gmin > Gmax !!!!" << std::endl;

    V1 = SIGMOID_DEFAULT_V1 ;
    URI::getQueryArg(uri.query, "v1", &V1) ;
    V2 = SIGMOID_DEFAULT_V2 ;
    URI::getQueryArg(uri.query, "v2", &V2) ;
    if (V1 > V2)
      std::cerr << "Error V1 > V2 !!!!" << std::endl;

    clearState() ;
  }

  void 
  SigmoidFunction::clearState(void) {
    previousMouseRawX = 0;
    previousMouseRawY = 0;
    previousMouseXRemainder = 0.0;
    previousMouseYRemainder = 0.0;
  }

  void
  SigmoidFunction::applyi(int mouseRawX, int mouseRawY, int *mouseX, int *mouseY,
			 TimeStamp::inttime /*timestamp*/) {
    if (mouseRawX != 0) {
      if (Sign(mouseRawX) != Sign(previousMouseRawX)) previousMouseXRemainder = 0.0 ;
      previousMouseRawX = mouseRawX ;
    }
    if (mouseRawY != 0) {
      if (Sign(mouseRawY) != Sign(previousMouseRawY)) previousMouseYRemainder = 0.0 ;
      previousMouseRawY = mouseRawY;
    }
  
    float counts = sqrt((float)(mouseRawX*mouseRawX + mouseRawY*mouseRawY));
    float meters = (counts/pointingDevice->getResolution()) * 0.0254 ;
    float mouseSpeed = meters * pointingDevice->getUpdateFrequency();

    float CDGain ;
    if (mouseSpeed <= V1)
      CDGain = Gmin;
    else if (mouseSpeed >= V2) 
      CDGain = Gmax;
    else
      CDGain = Gmin + (Gmax-Gmin)/(V2-V1) * (mouseSpeed - V1);

    float pixelGain = CDGain * displayDevice->getResolution() / pointingDevice->getResolution() ;
 
    float mouseXplusRemainder = mouseRawX * pixelGain + previousMouseXRemainder;
    float mouseYplusRemainder = mouseRawY * pixelGain + previousMouseYRemainder;
  
    *mouseX = (int)floor(mouseXplusRemainder) ;
    *mouseY = (int)floor(mouseYplusRemainder) ;
    if (!disableSubPixelation) {
      previousMouseXRemainder = mouseXplusRemainder - *mouseX ;
      previousMouseYRemainder = mouseYplusRemainder - *mouseY ;
    }
  }

  void
  SigmoidFunction::applyd(int mouseRawX, int mouseRawY, double *mouseX, double *mouseY,
             TimeStamp::inttime /*timestamp*/) {

    double counts = sqrt((double)(mouseRawX*mouseRawX + mouseRawY*mouseRawY));
    double meters = (counts/pointingDevice->getResolution()) * 0.0254 ;
    double mouseSpeed = meters * pointingDevice->getUpdateFrequency();

    double CDGain ;
    if (mouseSpeed <= V1)
      CDGain = Gmin;
    else if (mouseSpeed >= V2)
      CDGain = Gmax;
    else
      CDGain = Gmin + (Gmax-Gmin)/(V2-V1) * (mouseSpeed - V1);

    double pixelGain = CDGain * displayDevice->getResolution() / pointingDevice->getResolution() ;

    *mouseX = mouseRawX * pixelGain ;
    *mouseY = mouseRawY * pixelGain ;
  }

  URI
  SigmoidFunction::getURI(bool expanded) const {
    URI uri ;
    uri.scheme = "sigmoid" ;
    int i = 0 ;
    std::stringstream q ;
    if (expanded || Gmin!=SIGMOID_DEFAULT_GMIN) 
      q << (i++?"&":"") << "gmin=" << Gmin ;
    if (expanded || Gmax!=SIGMOID_DEFAULT_GMAX) 
      q << (i++?"&":"") << "gmax=" << Gmax ;
    if (expanded || V1!=SIGMOID_DEFAULT_V1) 
      q << (i++?"&":"") << "v1=" << V1 ;
    if (expanded || V2!=SIGMOID_DEFAULT_V2) 
      q << (i++?"&":"") << "v2=" << V2 ;
    if (expanded || disableSubPixelation!=SIGMOID_DEFAULT_NOSUBPIX) 
      q << (i++?"&":"") << "nosubpix=" << (disableSubPixelation?"true":"false") ;
    uri.query = q.str() ;
    return uri ;
  }

}
