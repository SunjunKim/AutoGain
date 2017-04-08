/*
 *
 * pointing/input/osx/osxHIDUtils.cpp --
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

#include <pointing/input/osx/osxHIDUtils.h>

#include <IOKit/hid/IOHIDUsageTables.h>

#include <stdexcept>
#include <sstream>

namespace pointing {

  // --- for testing/debugging purposes -----------------------------------

#define DEBUG_MODE                     0
  
#define DEBUG_SHOWS_ACCELERATION_TABLE 0
  
#define SHOW_IOPATH                    0

  // -----------------------------------------------------------------------

  static io_service_t
  hidGetParentService(io_service_t service, io_name_t classname) {
    for (io_service_t result=service; result!=MACH_PORT_NULL;) {
      if (IOObjectConformsTo(result, classname)) {
	if (result==service) IOObjectRetain(result) ;
	return result ;
      }
      io_service_t parent = MACH_PORT_NULL ;
      // IORegistryEntryGetParentEntry retains the returned object
      if (IORegistryEntryGetParentEntry(result, kIOServicePlane, &parent)!=KERN_SUCCESS)
	parent = MACH_PORT_NULL ;
      if (result!=service) IOObjectRelease(result) ;
      result = parent ;
    }
    return MACH_PORT_NULL ;
  }

#if 0
  static io_service_t
  hidGetParentService(IOHIDDeviceRef device, io_name_t classname) {
    return hidGetParentService(IOHIDDeviceGetService(device), classname) ;
  }
#endif
  
  // -----------------------------------------------------------------------

  static std::string
  CFStringToSTLString(const CFStringRef cfstr) {
    if (!cfstr) return "" ;

    CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfstr), 
						     kCFStringEncodingUTF8) ;
    char *buffer = new char[size] ;
    if (!CFStringGetCString(cfstr, buffer, size, kCFStringEncodingUTF8))
      throw std::runtime_error("CFStringGetCString failed") ;

    std::string result(buffer) ;
    delete [] buffer ;
    return result ;
  }

  int32_t
  hidDeviceGetIntProperty(IOHIDDeviceRef device, CFStringRef prop, int32_t defval) {
    int32_t result = defval ;
    CFNumberRef n = (CFNumberRef)IOHIDDeviceGetProperty(device, prop) ;
    if (n) CFNumberGetValue(n, kCFNumberSInt32Type, &result) ;
    return result ;
  }

  std::string
  hidDeviceGetStringProperty(IOHIDDeviceRef device, CFStringRef prop, std::string defval) {
    std::string result = defval ;
    CFStringRef s = (CFStringRef)IOHIDDeviceGetProperty(device, prop) ;
    if (s) result = CFStringToSTLString(s) ;
    return result ;
  }

  // -----------------------------------------------------------------------

  URI
  hidDeviceURI(IOHIDDeviceRef device) {
    io_name_t className ;
    io_service_t ioserv = IOHIDDeviceGetService(device) ;
    IOObjectGetClass(ioserv, className) ;
    std::stringstream uriAsString ;
    uriAsString << "osxhid:/"
		<< hidDeviceGetStringProperty(device, CFSTR(kIOHIDTransportKey))
		<< "/"
		<< std::hex << hidDeviceGetIntProperty(device, CFSTR(kIOHIDLocationIDKey)) << std::dec
		<< "/" << className ;

    std::string uriAsString_s = uriAsString.str() ;
#if SHOW_IOPATH
    io_string_t path ;
    IORegistryEntryGetPath(ioserv, kIOServicePlane, path);
    std::cerr << std::endl << path << std::endl ;
#endif
    return URI(uriAsString_s) ;
  }

  // -----------------------------------------------------------------------

  void
  hidDebugDevice(IOHIDDeviceRef device, std::ostream& out) {
    int32_t vendorID = hidDeviceGetIntProperty(device, CFSTR(kIOHIDVendorIDKey)) ;
    int32_t productID = hidDeviceGetIntProperty(device, CFSTR(kIOHIDProductIDKey)) ;
    std::string manufacturer = hidDeviceGetStringProperty(device, CFSTR(kIOHIDManufacturerKey)) ;
    std::string product = hidDeviceGetStringProperty(device, CFSTR(kIOHIDProductKey)) ;
    std::string serial = hidDeviceGetStringProperty(device, CFSTR(kIOHIDSerialNumberKey)) ;
      
    out << hidDeviceURI(device).asString()
	<< " ["
	<< std::hex
	<< "vend:0x" << vendorID << ", prod:0x" << productID
	<< std::dec ;
    if (!serial.empty())
      out << ", serial:'" << serial << "'" ;
    SInt32 resolution = hidGetPointingResolution(device) ;
    if (resolution!=-1) out << ", " << resolution << " CPI" ;
    double reportinterval = hidGetReportInterval(device) ;
    if (reportinterval!=-1) out << ", " << 1.0/reportinterval << " Hz" ;
    out << " - "
	<< product
	<< " (" << manufacturer << ")" 
	<< "]" ;
#if DEBUG_MODE && DEBUG_SHOWS_ACCELERATION_TABLE
    out << " ACC:[" << hidDeviceGetStringProperty(device, CFSTR(kIOHIDPointerAccelerationTableKey)) << "]" ;
#endif
  }

  // -----------------------------------------------------------------------

  static SInt32
  hidGetPointingResolution(io_service_t service) {
    if (service==MACH_PORT_NULL) return -1 ;

    SInt32 resolution = -1 ;

    CFTypeRef res = IORegistryEntryCreateCFProperty(service, CFSTR(kIOHIDPointerResolutionKey), kCFAllocatorDefault, kNilOptions) ;
    if (res) {
      if (CFGetTypeID(res)==CFNumberGetTypeID()
	  && CFNumberGetValue((CFNumberRef)res, kCFNumberSInt32Type, &resolution))
	resolution = resolution>>16 ;
      CFRelease(res) ;
#if 0
      io_name_t className ;
      IOObjectGetClass(service, className) ;
      std::cerr << className << " --> " << resolution << std::endl ;
#endif
      return resolution ;      
    }

    io_iterator_t children ;
    IORegistryEntryGetChildIterator(service, kIOServicePlane, &children) ;
    io_object_t child ;
    while (resolution==-1 && (child = IOIteratorNext(children))) {
      resolution = hidGetPointingResolution(child) ;
      IOObjectRelease(child) ;
    }
    IOObjectRelease(children) ;
    
    return resolution ;
  }

  SInt32
  hidGetPointingResolution(IOHIDDeviceRef device) {
    if (!device) return -1 ;
    return hidGetPointingResolution(IOHIDDeviceGetService(device)) ;
  }

  // -----------------------------------------------------------------------

  double
  hidGetReportInterval(IOHIDDeviceRef device) {
    int32_t microseconds = hidDeviceGetIntProperty(device, CFSTR(kIOHIDReportIntervalKey), -1) ;
    return microseconds<0 ? -1.0 : microseconds/1000000.0 ;
  }

  // -----------------------------------------------------------------------

  std::string
  hidDeviceFromVendorProductUsagePageUsage(int vendorID, int productID, 
					   int primaryUsagePage, int primaryUsage) {
    std::stringstream result ;
    result << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	   << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
	   << "<plist version=\"1.0\">"
	   << "<dict>" ;
    if (vendorID)
      result << "<key>VendorID</key><integer>" << vendorID << "</integer>" ;
    if (productID)
      result << "<key>ProductID</key><integer>" << productID << "</integer>" ;
    if (primaryUsagePage)
      result << "<key>PrimaryUsagePage</key><integer>" << primaryUsagePage << "</integer>" ;
    if (primaryUsage)
      result << "<key>PrimaryUsage</key><integer>" << primaryUsage << "</integer>" ;
    result << "</dict>"
	   << "</plist>" ;
    return result.str() ;
  }

  // -----------------------------------------------------------------------
  
  std::string
  hidAnyPointingDevice(void) {
    return hidDeviceFromVendorProductUsagePageUsage(0,0,kHIDPage_GenericDesktop,kHIDUsage_GD_Mouse) ;
  }

  std::string
  hidXYElements(void) { 
    static const char *xyelements = "<?xml version='1.0' encoding='UTF-8'?><!DOCTYPE plist PUBLIC '-//Apple//DTD PLIST 1.0//EN' 'http://www.apple.com/DTDs/PropertyList-1.0.dtd'><plist version='1.0'><array><dict><key>UsagePage</key><integer>1</integer><key>Usage</key><integer>48</integer></dict><dict><key>UsagePage</key><integer>1</integer><key>Usage</key><integer>49</integer></dict></array></plist>" ;
    return xyelements ;
  }

  // -----------------------------------------------------------------------

}
