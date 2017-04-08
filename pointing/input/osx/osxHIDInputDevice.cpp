/* -*- mode: c++ -*-
 *
 * pointing/input/osx/osxHIDInputDevice.cpp --
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

#include <pointing/utils/osx/osxPlistUtils.h>
#include <pointing/input/osx/osxHIDInputDevice.h>
#include <pointing/input/osx/osxHIDUtils.h>

#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace pointing {

  // --- default values ---------------------------------------------------

  int osxHIDInputDevice::queueSize = 4096 ;
  
#define OSX_DEFAULT_DEBUGLEVEL 0
#define OSX_DEFAULT_SEIZEDEVICE false

  // --- for testing/debugging purposes -----------------------------------

#define DEBUG_MODE              0

#define DEBUG_MATCHING_ELEMENTS 1
  
  // -----------------------------------------------------------------------

  osxHIDInputDevice::__device::__device(IOHIDDeviceRef dev) {
    device = dev ;
    CFRetain(device) ;
    queue = 0 ;
  }

  osxHIDInputDevice::__device::~__device() {
    if (queue) {
      IOHIDQueueStop(queue) ;
      CFRelease(queue) ;
    }
    CFRelease(device) ;
  }

  // -----------------------------------------------------------------------

  void
  osxHIDInputDevice::AddDevice(void *context,
			       IOReturn /*result*/, void */*sender*/, IOHIDDeviceRef device) {
    osxHIDInputDevice *self = (osxHIDInputDevice*)context ;

    URI devUri = hidDeviceURI(device) ;

    bool match = self->theDevice==0 && (self->uri.isEmpty() || self->uri.scheme=="any" || self->uri.resemble(devUri)) ;
    if (self->debugLevel>0) {
      std::cerr << (match?"+ ":"  ") ;
      hidDebugDevice(device, std::cerr) ;
      std::cerr << std::endl ;
    }
    if (!match) return ;

    self->theDevice = new __device(device) ;
    self->uri = devUri ;

    CFDataRef descriptor = (CFDataRef)IOHIDDeviceGetProperty(self->theDevice->device, CFSTR(kIOHIDReportDescriptorKey)) ;
    if (descriptor) {
      const UInt8 *bytes = CFDataGetBytePtr(descriptor) ;
      CFIndex length = CFDataGetLength(descriptor) ;
      if (self->inputreport_callback && !self->parser->setDescriptor(bytes, length))
        std::cerr << "osxHIDInputDevice::AddDevice: unable to parse the HID report descriptor" << std::endl;
      if (self->debugLevel > 1) {
        std::cerr << "    HID report descriptor: [ " << std::flush ;
        for (int i=0; i<length; ++i)
          std::cerr << std::hex << std::setfill('0') << std::setw(2) << (int)bytes[i] << " " ;
        std::cerr << "]" << std::endl ;
      }
    }

#if DEBUG_MODE
      std::cerr << "Setting up callbacks" << std::endl ;
#endif

    // ----------------------------------------------------------------
    
    if (self->inputreport_callback) {
#if DEBUG_MODE
      std::cerr << "Setting up report callback" << std::endl ;
#endif
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
      IOHIDDeviceRegisterInputReportWithTimeStampCallback(device,
					     self->theDevice->report, sizeof(self->theDevice->report),
					     self->inputreport_callback, self->inputreport_context) ;
#else
      IOHIDDeviceRegisterInputReportCallback(device,
					     self->theDevice->report, sizeof(self->theDevice->report),
					     self->inputreport_callback, self->inputreport_context) ;
#endif
    }

    // ----------------------------------------------------------------
    
    if (self->value_callback) {
#if DEBUG_MODE
      std::cerr << "Setting up value callback" << std::endl ;
#endif     
      IOHIDDeviceSetInputValueMatchingMultiple(device, self->elements_match) ; 
      IOHIDDeviceRegisterInputValueCallback(device, self->value_callback, self->value_context) ;
    }

    // ----------------------------------------------------------------
    
    if (self->queue_callback) {
#if DEBUG_MODE
      std::cerr << "Setting up queue callback" << std::endl ;
#endif
      self->theDevice->queue = IOHIDQueueCreate(kCFAllocatorDefault, device, queueSize, kIOHIDOptionsTypeNone) ;
      if (self->elements_match) {
#if DEBUG && DEBUG_MATCHING_ELEMENTS	
	std::cerr << "Queue, elements_match" << std::endl ;
#endif
	CFIndex mcount = CFArrayGetCount(self->elements_match) ;
	for (CFIndex mindex=0; mindex<mcount; ++mindex) {
	  CFDictionaryRef matching = (CFDictionaryRef)CFArrayGetValueAtIndex(self->elements_match, mindex) ;
	  CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, matching, kIOHIDOptionsTypeNone) ;
	  if (!elements) continue ;
	  CFIndex ecount = CFArrayGetCount(elements) ;
	  for (CFIndex eindex=0; eindex<ecount; ++eindex) {
	    IOHIDElementRef e = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, eindex) ;
	    IOHIDQueueAddElement(self->theDevice->queue, e) ;
#if DEBUG && DEBUG_MATCHING_ELEMENTS
	    std::cerr << "elements_match EINDEX: " << eindex
		      << ", usagepage: " << IOHIDElementGetUsagePage(e)
		      << ", usage: " << IOHIDElementGetUsage(e)
		      << std::endl ;
#endif
	  }
	}
      } else {
#if DEBUG && DEBUG_MATCHING_ELEMENTS	
	std::cerr << "Queue, no elements_match" << std::endl ;
#endif
	CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, 0, kIOHIDOptionsTypeNone) ;
	if (elements) {
	  CFIndex ecount = CFArrayGetCount(elements) ;
	  for (CFIndex eindex=0; eindex<ecount; ++eindex) {
	    IOHIDElementRef e = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, eindex) ;
	    IOHIDQueueAddElement(self->theDevice->queue, e) ;
#if DEBUG && DEBUG_MATCHING_ELEMENTS
	    std::cerr << "!elements_match EINDEX: " << eindex
		      << ", usagepage: " << IOHIDElementGetUsagePage(e)
		      << ", usage: " << IOHIDElementGetUsage(e)
		      << std::endl ;
#endif
	  }
	}
      }
      IOHIDQueueRegisterValueAvailableCallback(self->theDevice->queue, self->queue_callback, self->queue_context) ;
      IOHIDQueueScheduleWithRunLoop(self->theDevice->queue, CFRunLoopGetMain(), kCFRunLoopDefaultMode) ;
      IOHIDQueueStart(self->theDevice->queue) ;
    }

    // ----------------------------------------------------------------
  }

  osxHIDInputDevice::osxHIDInputDevice(URI uri,
				       const char *device_description,
				       const char *elements_description):parser(0) {
    theDevice = 0 ;
    inputreport_callback = 0 ;
    inputreport_context = 0 ;
    value_callback = 0 ;
    value_context = 0 ;    
    queue_callback = 0 ;
    queue_context = 0 ;
    debugLevel = OSX_DEFAULT_DEBUGLEVEL ;
    seizeDevice = OSX_DEFAULT_SEIZEDEVICE ;

    this->uri = uri ;
    this->uri.generalize() ;
    URI::getQueryArg(uri.query, "debugLevel", &debugLevel) ;
    URI::getQueryArg(uri.query, "seize", &seizeDevice) ;
    parser = new HIDReportParser(NULL, 0, debugLevel);

    manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone) ;
    if (!manager) 
      throw std::runtime_error("IOHIDManagerCreate failed") ;

    device_match = 0 ;
    if (device_description) {
      if (!strncmp(device_description, "<?xml", 5)) {
	device_match = (CFMutableDictionaryRef)getPropertyListFromXML(device_description) ;
	if (debugLevel>1) 
	  std::cerr << "Filtering devices based on XML description: " << device_description << std::endl ;
      } else {
	device_match = (CFMutableDictionaryRef)getPropertyListFromFile(device_description) ;
	if (debugLevel>1) 
	  std::cerr << "Filtering devices based on file " << device_description << std::endl ;
      }
    }
    IOHIDManagerSetDeviceMatching(manager, device_match) ;

    elements_match = 0 ;
    if (elements_description) {
      if (!strncmp(elements_description, "<?xml", 5)) {
	elements_match = (CFArrayRef)getPropertyListFromXML(elements_description) ;
	if (debugLevel>1) 
	  std::cerr << "Filtering elements based on XML description: " << elements_description << std::endl ;
      } else {
	elements_match = (CFArrayRef)getPropertyListFromFile(elements_description) ;
	if (debugLevel>1) 
	  std::cerr << "Filtering elements based on file " << elements_description << std::endl ;
      }
    }
  
    IOHIDManagerRegisterDeviceMatchingCallback(manager, AddDevice, (void*)this) ;
    IOHIDManagerRegisterDeviceRemovalCallback(manager, RemoveDevice, (void*)this) ;
    IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetMain(), kCFRunLoopDefaultMode) ;

    IOOptionBits inOptions = seizeDevice ? kIOHIDOptionsTypeSeizeDevice : kIOHIDOptionsTypeNone ;
    if (IOHIDManagerOpen(manager, inOptions)!=kIOReturnSuccess) 
      throw std::runtime_error("IOHIDManagerOpen failed") ;
  }

  void
  osxHIDInputDevice::setInputReportCallback(IOHIDReportCallbackSignature callback, void *context) {
    inputreport_callback = callback ;
    inputreport_context = context ;
  }

  void
  osxHIDInputDevice::setValueCallback(IOHIDValueCallback callback, void *context) {
    value_callback = callback ;
    value_context = context ;
  }

  void
  osxHIDInputDevice::setQueueCallback(IOHIDCallback callback, void *context) {
    queue_callback = callback ;
    queue_context = context ;
  }

  URI
  osxHIDInputDevice::getURI(bool expanded) const {
    URI result = uri ;
    if (expanded || seizeDevice!=OSX_DEFAULT_SEIZEDEVICE)
      URI::addQueryArg(result.query, "seize", seizeDevice) ;
    return result ;
  }

  void
  osxHIDInputDevice::RemoveDevice(void *context, IOReturn /*result*/, void */*sender*/, IOHIDDeviceRef device) {
    osxHIDInputDevice *self = (osxHIDInputDevice*)context ;

    if (self->debugLevel>0) std::cerr << "- " << device << std::endl ;

    if (device==self->getDevice()) {
      delete self->theDevice ;
      self->theDevice = 0 ;
    }
  }

  bool
  osxHIDInputDevice::isActive(void) const {
    return theDevice!=0 ;
  }

  IOHIDDeviceRef
  osxHIDInputDevice::getDevice(void) const {
    if (theDevice) return theDevice->device ;
    return 0 ;
  }

  osxHIDInputDevice::~osxHIDInputDevice(void) {
    delete theDevice ;
    delete parser ;
    if (device_match) CFRelease(device_match) ;
    if (elements_match) CFRelease(elements_match) ;
    if (manager) {
      IOHIDManagerClose(manager, kIOHIDOptionsTypeNone) ;
      CFRelease(manager) ;
    }
  }

  // -----------------------------------------------------------------------

}
