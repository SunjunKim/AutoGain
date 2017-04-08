/* -*- mode: c++ -*-
 *
 * pointing-xorg/transferfunctions/OSXFunction.h --
 *
 * Initial software
 * Authors: Nicolas Roussel
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing-xorg/transferfunctions/XorgFunction.h>

#include <iostream>
#include <sstream>

#include "XorgFunctionPrivate.cpp"

namespace pointing {

  // Default values from xorg-server-1.9.3/include/site.h
#define XORG_DEFAULT_CTRL_NUM 2
#define XORG_DEFAULT_CTRL_DEN 1
#define XORG_DEFAULT_CTRL_THR 4

  // Default values from xorg-server-1.9.3/dix/devices.c and ptrveloc.c
#define XORG_DEFAULT_SCHEME   PtrAccelPredictable
#define XORG_DEFAULT_PROFILE  AccelProfileClassic
#define XORG_DEFAULT_PRED_CM  10.0
#define XORG_DEFAULT_PRED_CA  1.0
#define XORG_DEFAULT_PRED_MA  1.0

  static DeviceIntPtr
  createDevice(int ctrl_num, int ctrl_den, int ctrl_threshold,
	       int scheme, 
	       int predictableProfile, float corr_mul, float const_acceleration, float min_acceleration) {
    DeviceIntPtr dev = new DeviceIntRec ;
    dev->valuator = new ValuatorClassRec ;
    dev->valuator->accelScheme.number = PtrAccelNoOp ;
    dev->valuator->accelScheme.AccelSchemeProc = NULL ;
    dev->valuator->accelScheme.accelData = NULL ;
    dev->valuator->accelScheme.AccelCleanupProc = NULL ;
    dev->ptrfeed = new PtrFeedbackClassRec ;
    dev->ptrfeed->ctrl.num = ctrl_num ;
    dev->ptrfeed->ctrl.den = ctrl_den ;
    dev->ptrfeed->ctrl.threshold = ctrl_threshold ;
    for (unsigned int i=0; i<MAX_VALUATORS; ++i)
      dev->last.remainder[i] = 0 ;
    if (InitPointerAccelerationScheme(dev, scheme)) {
      DeviceVelocityPtr vel = GetDevicePredictableAccelData(dev) ;
      if (vel) {
	SetAccelerationProfile(vel, predictableProfile) ;
	vel->corr_mul = corr_mul ;
	vel->const_acceleration = const_acceleration ;
	vel->min_acceleration = min_acceleration ;
      }
    } else
      std::cerr << "XorgFunction: InitPointerAccelerationScheme failed" << std::endl ;
    return dev ;
  }

  static void
  deleteDevice(DeviceIntPtr dev) {
    if (!dev) return ;
    if (dev->valuator->accelScheme.AccelCleanupProc)
      dev->valuator->accelScheme.AccelCleanupProc(dev) ;
    delete dev->ptrfeed ;
    delete dev->valuator ;
    delete dev ;
  }

  XorgFunction::XorgFunction(URI &uri, PointingDevice* input, DisplayDevice* output) {
    epoch = TimeStamp::undef ;

    this->input = input ;
    this->output = output ;
    baseuri = uri ;
    baseuri.generalize() ;

    ctrl_num = XORG_DEFAULT_CTRL_NUM ;
    ctrl_den = XORG_DEFAULT_CTRL_DEN ;
    ctrl_threshold = XORG_DEFAULT_CTRL_THR ;
    scheme = XORG_DEFAULT_SCHEME ;
    predictableProfile = XORG_DEFAULT_PROFILE ;
    corr_mul = XORG_DEFAULT_PRED_CM ;
    const_acceleration = XORG_DEFAULT_PRED_CA ;
    min_acceleration = XORG_DEFAULT_PRED_MA ;

    URI::getQueryArg(uri.query, "num", &ctrl_num) ;
    URI::getQueryArg(uri.query, "den", &ctrl_den) ;
    URI::getQueryArg(uri.query, "thr", &ctrl_threshold) ;

    normalize = false;
    URI::getQueryArg(uri.query, "normalize", &normalize);

    if (uri.opaque=="noop")
      scheme = PtrAccelNoOp ;
    else if (uri.opaque=="lightweight")
      scheme = PtrAccelLightweight ;
    else if (uri.opaque=="none")
      predictableProfile = AccelProfileNone ;
    else if (uri.opaque=="classic")
      predictableProfile = AccelProfileClassic ;
    else if (uri.opaque=="devicespecific")
      predictableProfile = AccelProfileDeviceSpecific ;
    else if (uri.opaque=="polynomial")
      predictableProfile = AccelProfilePolynomial ;
    else if (uri.opaque=="smoothlinear")
      predictableProfile = AccelProfileSmoothLinear ;
    else if (uri.opaque=="simple")
      predictableProfile = AccelProfileSimple ;
    else if (uri.opaque=="power")
      predictableProfile = AccelProfilePower ;
    else if (uri.opaque=="linear")
      predictableProfile = AccelProfileLinear ;
    else if (uri.opaque=="smoothlimited")
      predictableProfile = AccelProfileSmoothLimited ;

    URI::getQueryArg(uri.query, "cm", &corr_mul) ;
    URI::getQueryArg(uri.query, "ca", &const_acceleration) ;
    URI::getQueryArg(uri.query, "ma", &min_acceleration) ;

    dev = createDevice(ctrl_num, ctrl_den, ctrl_threshold, scheme, 
		       predictableProfile, corr_mul, const_acceleration, min_acceleration) ;
  }

  void
  XorgFunction::clearState(void) {
    deleteDevice(dev) ;
    dev = createDevice(ctrl_num, ctrl_den, ctrl_threshold, scheme, 
		       predictableProfile, corr_mul, const_acceleration, min_acceleration) ;
  }

  void
  XorgFunction::applyi(int dxMickey, int dyMickey, int *dxPixel, int *dyPixel,
		      TimeStamp::inttime timestamp) {

    if (normalize)
      normalizeInput(&dxMickey, &dyMickey, input);
    // std::cerr << "XorgFunction::apply:" << timestamp << std::endl ;
    if (timestamp==TimeStamp::undef) 
      timestamp = TimeStamp::createAsInt() ;
    if (epoch==TimeStamp::undef)
      epoch = timestamp ;
    int first = 0, num = 2 ;
    int valuators[2] = {dxMickey, dyMickey} ;
    int ms = (timestamp-epoch)/TimeStamp::one_millisecond ;
    // std::cerr << "XorgFunction::apply: " << timestamp << "/" << ms << " " << dxMickey << " " << dyMickey << std::endl ;
    if (dev->valuator->accelScheme.AccelSchemeProc)
      dev->valuator->accelScheme.AccelSchemeProc(dev, first, num, valuators, ms) ;
    if (normalize)
      normalizeOutput(valuators, valuators + 1, output);
    *dxPixel = valuators[0] ;
    *dyPixel = valuators[1] ;
  }

  void
  XorgFunction::applyd(int dxMickey, int dyMickey, double *dxPixel, double *dyPixel,
              TimeStamp::inttime timestamp) {
    int dxInt, dyInt;
    applyi(dxMickey, dyMickey, &dxInt, &dyInt, timestamp);
    *dxPixel = dxInt;
    *dyPixel = dyInt;
  }

  URI
  XorgFunction::getURI(bool expanded) const {
    URI uri(baseuri) ;
    int i = 0 ;
    std::stringstream q ;
    if (expanded || ctrl_num!=XORG_DEFAULT_CTRL_NUM) 
      q << (i++?"&":"") << "num=" << ctrl_num ;
    if (expanded || ctrl_den!=XORG_DEFAULT_CTRL_DEN) 
      q << (i++?"&":"") << "den=" << ctrl_den ;
    if (expanded || ctrl_threshold!=XORG_DEFAULT_CTRL_THR) 
      q << (i++?"&":"") << "thr=" << ctrl_threshold ;
    if (scheme==PtrAccelPredictable) {
      if (expanded || corr_mul!=XORG_DEFAULT_PRED_CM) 
	q << (i++?"&":"") << "cm=" << corr_mul ;
      if (expanded || const_acceleration!=XORG_DEFAULT_PRED_CA) 
	q << (i++?"&":"") << "ca=" << const_acceleration ;
      if (expanded || min_acceleration!=XORG_DEFAULT_PRED_MA) 
	q << (i++?"&":"") << "ma=" << min_acceleration ;
    }
    if (expanded || normalize)
      q << (i++?"&":"") << "normalize=" << normalize ;
    uri.query = q.str() ;
    return uri ;
  }

  XorgFunction::~XorgFunction(void) {
    deleteDevice(dev) ;
  }

}
