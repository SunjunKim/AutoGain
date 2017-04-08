/* -*- mode: c++ -*-
 *
 * pointing/input/osx/osxHIDInputDevice.h --
 *
 * Initial software
 * Authors: Nicolas Roussel
 * Copyright © INRIA
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 * External dependencies: CoreFoundation
 *
 */

#ifndef osxHIDInputDevice_h
#define osxHIDInputDevice_h

#include <pointing/utils/URI.h>

#include <Availability.h>
#include <IOKit/hid/IOHIDManager.h>
#include <pointing/utils/HIDReportParser.h>

#include <map>
#include <string>

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
#define IOHIDReportCallbackSignature IOHIDReportWithTimeStampCallback
#else
#define IOHIDReportCallbackSignature IOHIDReportCallback
#endif

namespace pointing {

  class osxHIDInputDevice {

  protected:

    friend class osxHIDPointingDevice ;

    IOHIDManagerRef manager ;
    CFMutableDictionaryRef device_match ;
    CFArrayRef elements_match ;

    struct __device {
      IOHIDDeviceRef device ;
      uint8_t report[64] ;
      IOHIDQueueRef queue ;
      __device(IOHIDDeviceRef device) ;
      ~__device() ;
    } ;

    __device *theDevice ;
    URI uri ;

    IOHIDReportCallbackSignature inputreport_callback ;
    void *inputreport_context ;
    IOHIDValueCallback value_callback ;
    void *value_context ;
    IOHIDCallback queue_callback ;
    void *queue_context ;

    int debugLevel ;
    bool seizeDevice ;

    static int queueSize ;
    HIDReportParser *parser;

    static void AddDevice(void *context, IOReturn result, void *sender, IOHIDDeviceRef device) ;
    static void RemoveDevice(void *context, IOReturn result, void *sender, IOHIDDeviceRef device) ;

  public:

    osxHIDInputDevice(URI device_uri, 
		      const char *device_description=0, const char *elements_description=0) ;

    bool isActive(void) const ;

    IOHIDDeviceRef getDevice(void) const ;

    void setInputReportCallback(IOHIDReportCallbackSignature callback, void *context=0) ;
    void setValueCallback(IOHIDValueCallback callback, void *context=0) ;
    void setQueueCallback(IOHIDCallback callback, void *context=0) ;

    URI getURI(bool expanded=false) const ;

    void setDebugLevel(int level) { debugLevel = level ; }

    ~osxHIDInputDevice(void) ;

  } ;

}

#endif
