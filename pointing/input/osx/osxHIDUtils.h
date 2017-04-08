/* -*- mode: c++ -*-
 *
 * pointing/input/osx/osxHIDUtils.h --
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

#ifndef osxHIDUtils_h
#define osxHIDUtils_h

#include <pointing/utils/URI.h>

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/usb/IOUSBLib.h>

#include <iostream>
#include <string>

namespace pointing {
  
  int32_t
  hidDeviceGetIntProperty(IOHIDDeviceRef device,
			  CFStringRef prop, int32_t defval=0) ;

  std::string
  hidDeviceGetStringProperty(IOHIDDeviceRef device,
			     CFStringRef prop, std::string defval="") ;

  URI
  hidDeviceURI(IOHIDDeviceRef device) ;
  
  SInt32 // -1 if resolution is unknown
  hidGetPointingResolution(IOHIDDeviceRef device) ;

  
  double // in seconds (-1 if interval is unknown)
  hidGetReportInterval(IOHIDDeviceRef device) ;

  void
  hidDebugDevice(IOHIDDeviceRef device, std::ostream& out) ;
  
  // -----------------------------------------------------------------------
    
  std::string // can be used to create an osxHIDInputDevice
  hidDeviceFromVendorProductUsagePageUsage(int vendorID, int productID,
					   int primaryUsagePage, int primaryUsage) ;

  // -----------------------------------------------------------------------
  
  std::string
  hidAnyPointingDevice(void) ;
  
  std::string
  hidXYElements(void) ;

}

#endif
