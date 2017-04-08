/* -*- mode: c++ -*-
 *
 * pointing/output/osx/osxDisplayDeviceManager.cpp --
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

#include <pointing/output/osx/osxDisplayDeviceManager.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <stdexcept>

extern "C" bool NSApplicationLoad(void) ;

// https://developer.apple.com/library/mac/documentation/GraphicsImaging/Reference/Quartz_Services_Ref/index.html#//apple_ref/doc/c_ref/CGDisplayChangeSummaryFlags

namespace pointing
{
  void osxDisplayDeviceManager::MyDisplayReconfigurationCallBack(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags, void *userInfo) {
    osxDisplayDeviceManager *self = (osxDisplayDeviceManager *)userInfo;
    if (flags & kCGDisplayAddFlag)
      self->addDisplay(display);
    if (flags & kCGDisplayRemoveFlag)
      self->removeDisplay(display);
  }

  std::string CFStringRefToStdString(CFStringRef aString) {
    if (aString == NULL) {
      return "Unknown";
    }

    CFIndex length = CFStringGetLength(aString);
    CFIndex maxSize =
    CFStringGetMaximumSizeForEncoding(length,
                                      kCFStringEncodingUTF8);
    char *buffer = (char *)malloc(maxSize);
    if (CFStringGetCString(aString, buffer, maxSize,
                           kCFStringEncodingUTF8)) {
      std::string result(buffer);
      free(buffer);
      return result;
    }
    return "Unknown";
  }

  // Returns the io_service_t corresponding to a CG display ID, or 0 on failure.
  // The io_service_t should be released with IOObjectRelease when not needed.
  //
  static io_service_t IOServicePortFromCGDisplayID(CGDirectDisplayID displayID)
  {
      io_iterator_t iter;
      io_service_t serv, servicePort = 0;

      CFMutableDictionaryRef matching = IOServiceMatching("IODisplayConnect");

      // releases matching for us
      kern_return_t err = IOServiceGetMatchingServices(kIOMasterPortDefault,
                                                       matching,
                                                       &iter);
      if (err)
          return 0;

      while ((serv = IOIteratorNext(iter)) != 0)
      {
          CFDictionaryRef info;
          CFIndex vendorID, productID, serialNumber;
          CFNumberRef vendorIDRef, productIDRef, serialNumberRef;
          Boolean success;

          info = IODisplayCreateInfoDictionary(serv,
                                               kIODisplayOnlyPreferredName);

          vendorIDRef = static_cast<CFNumberRef>(CFDictionaryGetValue(info,
                                             CFSTR(kDisplayVendorID)));
          productIDRef = static_cast<CFNumberRef>(CFDictionaryGetValue(info,
                                              CFSTR(kDisplayProductID)));
          serialNumberRef = static_cast<CFNumberRef>(CFDictionaryGetValue(info,
                                                 CFSTR(kDisplaySerialNumber)));

          success = CFNumberGetValue(vendorIDRef, kCFNumberCFIndexType,
                                     &vendorID);
          success &= CFNumberGetValue(productIDRef, kCFNumberCFIndexType,
                                      &productID);
          if (serialNumberRef != 0)
            success &= CFNumberGetValue(serialNumberRef, kCFNumberCFIndexType,
                                      &serialNumber);

          if (!success)
          {
              CFRelease(info);
              continue;
          }

          // If the vendor and product id along with the serial don't match
          // then we are not looking at the correct monitor.
          // NOTE: The serial number is important in cases where two monitors
          //       are the exact same.
          if (CGDisplayVendorNumber(displayID) != vendorID  ||
              CGDisplayModelNumber(displayID) != productID  ||
              CGDisplaySerialNumber(displayID) != serialNumber)
          {
              CFRelease(info);
              continue;
          }

          // The VendorID, Product ID, and the Serial Number all Match Up!
          // Therefore we have found the appropriate display io_service
          servicePort = serv;
          CFRelease(info);
          break;
      }

      IOObjectRelease(iter);
      return servicePort;
  }

  std::string getDisplayName(CGDirectDisplayID did)
  {
    std::string result;
    CFDictionaryRef deviceInfo = IODisplayCreateInfoDictionary(IOServicePortFromCGDisplayID(did), kIODisplayOnlyPreferredName);
    CFTypeRef value = NULL;
    value = CFDictionaryGetValue(deviceInfo, CFSTR(kDisplayProductName));
    if (value)
    {
      CFDictionaryRef valueDict = (CFDictionaryRef)value;
      CFIndex number = CFDictionaryGetCount(valueDict);
      if (number > 0)
      {
        CFStringRef *key = (CFStringRef *)malloc(sizeof(CFStringRef)*number);
        CFDictionaryGetKeysAndValues(valueDict, NULL, (const void **)key);
        result = CFStringRefToStdString(key[0]);
        free(key);
      }
    }
    else
      result = "Unknown Display Name";
    CFRelease(deviceInfo);
    return result;
  }

  DisplayDeviceDescriptor osxDisplayDeviceManager::convertDevice(CGDirectDisplayID did)
  {
    std::stringstream uri;
    uri << "osxdisplay:/" << did;
    DisplayDeviceDescriptor desc;
    desc.devURI = uri.str();
    desc.name = getDisplayName(did);

    CGDisplayModeRef dispMode = CGDisplayCopyDisplayMode(did);
    CGRect rect = CGDisplayBounds(did);
    desc.width = rect.size.width;
    desc.height = rect.size.height;
    CGDisplayModeRelease(dispMode);

    return desc;
  }

  void osxDisplayDeviceManager::addDisplay(CGDirectDisplayID did)
  {
    DisplayDeviceDescriptor desc = convertDevice(did);
    addDevice(desc);
  }

  void osxDisplayDeviceManager::removeDisplay(CGDirectDisplayID did)
  {
    DisplayDeviceDescriptor desc = convertDevice(did);
    removeDevice(desc);
  }

  osxDisplayDeviceManager::osxDisplayDeviceManager()
  {
    CGDisplayCount dspyCnt = 0 ;
    CGDisplayErr err = CGGetActiveDisplayList(0, 0, &dspyCnt) ;
    CGDirectDisplayID *activeDspys = new CGDirectDisplayID [dspyCnt] ;
    err = CGGetActiveDisplayList(dspyCnt, activeDspys, &dspyCnt) ;
    for (unsigned int i = 0; i < dspyCnt; ++i) {
      DisplayDeviceDescriptor desc = convertDevice(activeDspys[i]) ;
      addDevice(desc);
    }
    delete [] activeDspys;
    NSApplicationLoad() ;
    CGDisplayRegisterReconfigurationCallback(MyDisplayReconfigurationCallBack, this);
  }
}
