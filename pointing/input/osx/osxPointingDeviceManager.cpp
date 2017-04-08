/* -*- mode: c++ -*-
 *
 * pointing/input/osx/osxPointingDeviceManager.cpp --
 *
 * Initial software
 * Authors: Izzatbek Mukhanov
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/input/osx/osxPointingDeviceManager.h>

#include <pointing/input/osx/osxHIDUtils.h>
#include <pointing/input/osx/osxPointingDevice.h>
#include <pointing/utils/osx/osxPlistUtils.h>

#include <iomanip>
#include <stdexcept>

namespace pointing {
  
  void fillDescriptorInfo(IOHIDDeviceRef devRef, PointingDeviceDescriptor &desc)
  {
    desc.devURI = hidDeviceURI(devRef);
    desc.vendor = hidDeviceGetStringProperty(devRef, CFSTR(kIOHIDManufacturerKey));
    desc.product = hidDeviceGetStringProperty(devRef, CFSTR(kIOHIDProductKey));
    desc.vendorID = hidDeviceGetIntProperty(devRef, CFSTR(kIOHIDVendorIDKey));
    desc.productID = hidDeviceGetIntProperty(devRef, CFSTR(kIOHIDProductIDKey));
  }

  osxPointingDeviceManager::osxPointingDeviceData::~osxPointingDeviceData()
  {
    if (devRef)
    {
      IOHIDDeviceClose(devRef, kIOHIDOptionsTypeNone);
      CFRelease(devRef);
    }
  }

  void osxPointingDeviceManager::processMatching(PointingDeviceData *data, SystemPointingDevice *device)
  {
    osxPointingDevice *dev = static_cast<osxPointingDevice *>(device);
    osxPointingDeviceData *pdd = static_cast<osxPointingDeviceData *>(data);
    IOOptionBits inOptions = dev->seize ? kIOHIDOptionsTypeSeizeDevice : kIOHIDOptionsTypeNone;
    dev->cpi = hidGetPointingResolution(pdd->devRef);
    dev->hz = 1.0 / hidGetReportInterval(pdd->devRef);
    if (IOHIDDeviceOpen(pdd->devRef, inOptions) != kIOReturnSuccess)
    {
      std::cerr << "IOHIDDeviceOpen failed" << std::endl;
      if (inOptions == kIOHIDOptionsTypeSeizeDevice)
        std::cerr << "Could not seize " << device->getURI() << std::endl;
    }
  }

  void osxPointingDeviceManager::AddDevice(void *sender, IOReturn, void *, IOHIDDeviceRef devRef)
  {
    CFDataRef descriptor = (CFDataRef)IOHIDDeviceGetProperty(devRef, CFSTR(kIOHIDReportDescriptorKey));
    if (!descriptor)
      return;

    osxPointingDeviceManager *self = (osxPointingDeviceManager *)sender;

    const UInt8 *bytes = CFDataGetBytePtr(descriptor);
    CFIndex length = CFDataGetLength(descriptor);

    if (self->debugLevel > 1)
    {
      std::cerr << "HID descriptors: [ " << std::flush ;
      for (int i=0; i<length; ++i)
        std::cerr << std::hex << std::setfill('0') << std::setw(2) << (int)bytes[i] << " " ;
      std::cerr << "]" << std::endl ;
    }

    HIDReportParser parser;
    // Try to parse with the parser
    // Return if it fails, this is probably not a pointing device 
    if (!parser.setDescriptor(bytes, length))
    {
      if (self->debugLevel > 1)
        std::cerr << "    osxPointingDeviceManager::AddDevice: unable to parse the HID report descriptor" << std::endl;
      return;
    }
    
    osxPointingDeviceData *pdd = new osxPointingDeviceData;

    // We should not open or close IOHIDManager's IOHIDDeviceRefs.
    // So, create our own IOHIDDeviceRef from pdd->devRef.
    io_service_t devService = IOHIDDeviceGetService(devRef);
    pdd->devRef = IOHIDDeviceCreate(kCFAllocatorDefault, devService);
    fillDescriptorInfo(pdd->devRef, pdd->desc);
    // Note that, devRef and pdd->devRef are not the same.
    // devRef is the IOHIDManager's IOHIDDeviceRef.
    self->registerDevice(devRef, pdd);
    pdd->parser = parser;
    IOHIDDeviceScheduleWithRunLoop(pdd->devRef, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    IOHIDDeviceRegisterInputReportCallback(pdd->devRef, pdd->report, sizeof(pdd->report), hidReportCallback, pdd);
  }

  void osxPointingDeviceManager::RemoveDevice(void *sender, IOReturn, void *, IOHIDDeviceRef devRef)
  {
    osxPointingDeviceManager *self = (osxPointingDeviceManager *)sender;
    self->unregisterDevice(devRef);
  }

  osxPointingDeviceManager::osxPointingDeviceManager()
  {
    manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager)
      throw std::runtime_error("IOHIDManagerCreate failed");

    std::string xml = hidDeviceFromVendorProductUsagePageUsage(0, 0, kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse);
    const char *plist = xml.c_str();
    CFMutableDictionaryRef device_match = (CFMutableDictionaryRef)getPropertyListFromXML(plist);
    IOHIDManagerSetDeviceMatching(manager, device_match);

    IOHIDManagerRegisterDeviceMatchingCallback(manager, AddDevice, (void*)this);
    IOHIDManagerRegisterDeviceRemovalCallback(manager, RemoveDevice, (void*)this);
    IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetMain(), kCFRunLoopDefaultMode);

    if (IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone)!=kIOReturnSuccess)
      throw std::runtime_error("IOHIDManagerOpen failed");
  }

  osxPointingDeviceManager::~osxPointingDeviceManager()
  {
    if (manager)
    {
      IOHIDManagerClose(manager, kIOHIDOptionsTypeNone) ;
      CFRelease(manager) ;
    }
  }

  void osxPointingDeviceManager::hidReportCallback(void *context, IOReturn, void *, IOHIDReportType, uint32_t, uint8_t *report, CFIndex)
  {
    // std::cerr << "hidReportCallback" << std::endl ;
    TimeStamp::inttime timestamp = TimeStamp::createAsInt();

    osxPointingDeviceData *pdd = static_cast<osxPointingDeviceData *>(context);

    if (pdd->parser.setReport(report))
    {
      int dx = 0, dy = 0, buttons = 0;
      pdd->parser.getReportData(&dx, &dy, &buttons) ;
      for (SystemPointingDevice *device : pdd->pointingList)
      {
        osxPointingDevice *dev = static_cast<osxPointingDevice *>(device);
        dev->registerTimestamp(timestamp, dx, dy);
        if (dev->callback)
          dev->callback(dev->callback_context, timestamp, dx, dy, buttons);
      }
    }
  }
}
