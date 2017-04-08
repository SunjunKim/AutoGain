/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/osx/osxSystemPointerAcceleration.cpp --
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

#include <pointing/transferfunctions/osx/osxSystemPointerAcceleration.h>

#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hidsystem/IOHIDShared.h>

#include <stdexcept>
#include <string>

namespace pointing {

  osxSystemPointerAcceleration::osxSystemPointerAcceleration(void) {
    mach_port_t port ;
    if (IOMasterPort(MACH_PORT_NULL, &port)!=KERN_SUCCESS)
      throw std::runtime_error("osxSystemPointerAcceleration: IOMasterPort failed") ;

    CFMutableDictionaryRef classes = IOServiceMatching("IOHIDSystem") ;
    if (!classes)
      throw std::runtime_error("osxSystemPointerAcceleration: IOServiceMatching failed") ;

    io_iterator_t services ;
    if (IOServiceGetMatchingServices(port, classes, &services)!=KERN_SUCCESS)
      throw std::runtime_error("osxSystemPointerAcceleration: IOServiceGetMatchingServices failed") ;

    io_object_t service = IOIteratorNext(services) ;
    if (!service)
      throw std::runtime_error("osxSystemPointerAcceleration: no IOHIDSystem") ;
  
    if (IOServiceOpen(service, mach_task_self(), kIOHIDParamConnectType, &connect)!=KERN_SUCCESS)
      throw std::runtime_error("osxSystemPointerAcceleration: IOServiceOpen failed") ;

    IOObjectRelease(services) ;
  }

  double
  osxSystemPointerAcceleration::get(const char *t) const {
    std::string target = t?t:"" ;
    kern_return_t result ;
    double acceleration = 0.0 ;
    if (target=="mouse") {
      result = IOHIDGetAccelerationWithKey(connect, CFSTR(kIOHIDMouseAccelerationType), &acceleration) ;
    } else if (target=="trackpad" || target=="touchpad") {
      result = IOHIDGetAccelerationWithKey(connect, CFSTR(kIOHIDTrackpadAccelerationType), &acceleration) ;
    } else {
      result = IOHIDGetMouseAcceleration(connect, &acceleration) ;
    } 
    if (result!=KERN_SUCCESS)
      throw std::runtime_error("osxSystemPointerAcceleration::get: failed") ;
    return acceleration ;
  }

  void
  osxSystemPointerAcceleration::set(double acceleration, const char *t) {
    std::string target = t?t:"" ;
    kern_return_t result ;
    if (target=="mouse") {
      result = IOHIDSetAccelerationWithKey(connect, CFSTR(kIOHIDMouseAccelerationType), acceleration) ;
    } else if (target=="trackpad" || target=="touchpad") {
      result = IOHIDSetAccelerationWithKey(connect, CFSTR(kIOHIDTrackpadAccelerationType), acceleration) ;
    } else {
      result = IOHIDSetMouseAcceleration(connect, acceleration) ;
    } 
    if (result!=KERN_SUCCESS)
      throw std::runtime_error("osxSystemPointerAcceleration::set: failed") ;
  }

  osxSystemPointerAcceleration::~osxSystemPointerAcceleration() {
    IOServiceClose(connect) ;
    IOObjectRelease(connect) ;
  }

}
