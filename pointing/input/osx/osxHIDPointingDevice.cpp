/* -*- mode: c++ -*-
 *
 * pointing/input/osx/osxHIDPointingDevice.cpp --
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

#include <pointing/input/osx/osxHIDPointingDevice.h>
#include <pointing/input/osx/osxHIDUtils.h>

#include <sstream>

namespace pointing {

  // --- default values ---------------------------------------------------
  
#define OSX_DEFAULT_VENDOR    0
#define OSX_DEFAULT_PRODUCT   0
#define OSX_DEFAULT_USAGEPAGE kHIDPage_GenericDesktop
#define OSX_DEFAULT_USAGE     kHIDUsage_GD_Mouse
#define OSX_DEFAULT_CPI       400.001
#define OSX_DEFAULT_HZ        125.001

  // --- for testing/debugging purposes -----------------------------------
  
#define DEBUG_MODE                 0

#define FORCE_QUEUE_MODE           0

#define USE_IOHIDValueGetTimeStamp 1
  
  // ----------------------------------------------------------------------

  osxHIDPointingDevice::osxHIDPointingDevice(URI uri) {
    epoch = TimeStamp::createAsInt() ;
    epoch_mach = mach_absolute_time() ;
    mach_timebase_info(&mach_timebaseinfo) ;
      
    vendorID = OSX_DEFAULT_VENDOR ;
    productID = OSX_DEFAULT_PRODUCT ;
    primaryUsagePage = OSX_DEFAULT_USAGEPAGE ;
    primaryUsage = OSX_DEFAULT_USAGE ;

    URI::getQueryArg(uri.query, "vendor", &vendorID) ;
    URI::getQueryArg(uri.query, "product", &productID) ;
    URI::getQueryArg(uri.query, "usagePage", &primaryUsagePage) ;
    URI::getQueryArg(uri.query, "usage", &primaryUsage) ;
    if (vendorID || productID)
      uri.scheme = "any" ; // FIXME: otherwise, won't match in osxHIDInputDevice (might want to fix this)

    std::string plist = hidDeviceFromVendorProductUsagePageUsage(vendorID, productID,
								 primaryUsagePage, primaryUsage) ;
    hiddev = new osxHIDInputDevice(uri, plist.c_str()) ;    

    callback = 0 ;
    callback_context = 0 ;
#if FORCE_QUEUE_MODE
    use_queue_callback = true ;
    use_report_callback = !use_queue_callback ;
#else
    if (isBluetooth()) {
      use_queue_callback = false ;
      use_report_callback = !use_queue_callback ;      
    } else { // isUSB() or default, used for example if no path is specified
      // defaults to our custom-made report parser which seems to be faster      
      use_queue_callback = URI::getQueryArg(uri.query, "queue") ;
      use_report_callback = !use_queue_callback ;            
    }
#endif
    if (use_report_callback)
      hiddev->setInputReportCallback(hidReportCallback, this) ;
    if (use_queue_callback)
      hiddev->setQueueCallback(hidQueueCallback, this) ;

    forced_cpi = forced_hz = -1.0 ;
    URI::getQueryArg(uri.query, "cpi", &forced_cpi) ;
    URI::getQueryArg(uri.query, "hz", &forced_hz) ;
  }

  bool
  osxHIDPointingDevice::isActive(void) const {
    return hiddev->isActive() ;
  }

  URI
  osxHIDPointingDevice::getURI(bool expanded, bool crossplatform) const {
    URI uri;

    if (crossplatform) {
      uri.scheme = "any";
      int vendorId, productId ;
      if ((vendorId = getVendorID()))
	URI::addQueryArg(uri.query, "vendor", vendorId) ;
      if ((productId = getProductID()))
	URI::addQueryArg(uri.query, "product", productId) ;
    } else {
      uri = hiddev->getURI(expanded) ;
      // FIXME: query args for modes (report and queue) are useless as
      // long as they are not both fully supported
      if (primaryUsagePage!=OSX_DEFAULT_USAGEPAGE)
	URI::addQueryArg(uri.query, "usagePage", primaryUsagePage) ;
      if (primaryUsage!=OSX_DEFAULT_USAGE)
	URI::addQueryArg(uri.query, "usage", primaryUsage) ;
    }

    if (expanded || hiddev->debugLevel)
      URI::addQueryArg(uri.query, "debugLevel", hiddev->debugLevel) ;
    if (expanded || forced_cpi>0)
      URI::addQueryArg(uri.query, "cpi", getResolution()) ;
    if (expanded || forced_hz>0)
      URI::addQueryArg(uri.query, "hz", getUpdateFrequency()) ;

    return uri ;
  }

  // ---------------------------------------------------------------------------

  bool
  osxHIDPointingDevice::isUSB(void) {
    return (hiddev && hiddev->uri.path.find("/USB")==0) ;
  }

  bool osxHIDPointingDevice::isBluetooth(void) {
    return (hiddev && hiddev->uri.path.find("/Bluetooth")==0) ;
  }

  // ---------------------------------------------------------------------------

  int
  osxHIDPointingDevice::getVendorID(void) const {
    if (hiddev->theDevice)
      return hidDeviceGetIntProperty(hiddev->theDevice->device, CFSTR(kIOHIDVendorIDKey)) ;
    return 0 ;
  }

  std::string 
  osxHIDPointingDevice::getVendor(void) const {
    if (hiddev->theDevice)
      return hidDeviceGetStringProperty(hiddev->theDevice->device, CFSTR(kIOHIDManufacturerKey)) ;
    return "???" ;
  }

  int
  osxHIDPointingDevice::getProductID(void) const {
    if (hiddev->theDevice)
      return hidDeviceGetIntProperty(hiddev->theDevice->device, CFSTR(kIOHIDProductIDKey)) ;
    return 0 ;
  }

  std::string
  osxHIDPointingDevice::getProduct(void) const {
    if (hiddev->theDevice)
      return hidDeviceGetStringProperty(hiddev->theDevice->device, CFSTR(kIOHIDProductKey)) ;
    return "???" ;
  }

  // ---------------------------------------------------------------------------

  double
  osxHIDPointingDevice::getResolution(double *defval) const {
    if (forced_cpi>0) return forced_cpi ;

    double result = -1.0 ;
    if (hiddev->theDevice) result = hidGetPointingResolution(hiddev->theDevice->device) ;
    if (result>0) return result ;
    if (defval) return *defval ;
    return OSX_DEFAULT_CPI ;
  }

  // ---------------------------------------------------------------------------

  double
  osxHIDPointingDevice::getUpdateFrequency(double *defval) const {
    if (forced_hz>0) return forced_hz ;

    double result = -1 ;
    if (hiddev->theDevice) result = 1.0 / hidGetReportInterval(hiddev->theDevice->device) ;

    double estimated = estimatedUpdateFrequency();
    if (result == 125. && estimated > 0.)
      return estimated;
    if (result>0) return result ;
    if (estimated > 0.)
      return estimated;
    if (defval) return *defval ;
    return OSX_DEFAULT_HZ ;
  }

  // ---------------------------------------------------------------------------

  void
  osxHIDPointingDevice::setDebugLevel(int level) {
    hiddev->setDebugLevel(level) ;
  }

  void
  osxHIDPointingDevice::setPointingCallback(PointingCallback cbck, void *ctx) {
    callback = cbck ;
    callback_context = ctx ;
    if (callback) {
      if (use_report_callback)
	hiddev->setInputReportCallback(hidReportCallback, this) ;
      if (use_queue_callback)
	hiddev->setQueueCallback(hidQueueCallback, this) ;
    } else {
      if (use_report_callback)
	hiddev->setInputReportCallback(0, 0) ;
      if (use_queue_callback)
	hiddev->setQueueCallback(0, 0) ;
    }
  }

  osxHIDPointingDevice::~osxHIDPointingDevice(void) {
    delete hiddev ;
  }

  // ---------------------------------------------------------------------------

  osxHIDPointingDevice::PointingReport::PointingReport(void) {
    t = TimeStamp::undef ;
    dx = dy = 0 ;
    btns = 0 ;
  }

  osxHIDPointingDevice::PointingReport&
  osxHIDPointingDevice::PointingReport::operator = (osxHIDPointingDevice::PointingReport& src) {
    if (&src!=this) {
      t = src.t ;
      dx = src.dx ;
      dy = src.dy ;
      btns = src.btns ;
    }
    return *this ;
  }

  void
  osxHIDPointingDevice::PointingReport::clear(void) {
    dx = dy = 0 ;
    t = TimeStamp::undef ;
  }

  bool
  osxHIDPointingDevice::PointingReport::setButton(uint32_t index, uint32_t value) {
    int tmp = 1<<index ;
    btns = value ? (btns | tmp) : (btns & ~tmp) ;
    return true ;
  }

  bool
  osxHIDPointingDevice::PointingReport::isOlderThan(TimeStamp::inttime timestamp) const {
    return (t!=TimeStamp::undef /*&& timestamp!=TimeStamp::undef*/ && t<timestamp) ;
  }

  std::string
  osxHIDPointingDevice::PointingReport::toString(void) const {
    std::stringstream result ;
    result << "PointingReport [t=" << t 
	   << ", dx=" << dx
	   << ", dy=" << dy
	   << ", btns=" << btns
	   << "]" ;
    return result.str() ;
  }

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
  void
  osxHIDPointingDevice::hidReportCallback(void *context,
					  IOReturn /*result*/,
					  void */*sender*/,
					  IOHIDReportType /*type*/,
					  uint32_t /*reportID*/,
					  uint8_t *report,
					  CFIndex /*reportLength*/,
					  uint64_t abstime) {
    osxHIDPointingDevice *self = (osxHIDPointingDevice*)context ;    
    uint64_t nanoseconds = (abstime - self->epoch_mach) * self->mach_timebaseinfo.numer / self->mach_timebaseinfo.denom ;
    TimeStamp::inttime timestamp = self->epoch + nanoseconds*TimeStamp::one_nanosecond ;
#else
    void
      osxHIDPointingDevice::hidReportCallback(void *context,
					      IOReturn /*result*/,
					      void */*sender*/,
					      IOHIDReportType /*type*/,
					      uint32_t /*reportID*/,
					      uint8_t *report,
					      CFIndex /*reportLength*/) {
      TimeStamp::inttime timestamp = TimeStamp::createAsInt() ;
      osxHIDPointingDevice *self = (osxHIDPointingDevice*)context ;    
#endif    

#if DEBUG_MODE
      std::cerr << timestamp << " osxHIDPointingDevice::hidReportCallback" << std::endl ;
#endif
      
      if (!self->hiddev->parser->setReport(report)) return ;

      int dx = 0, dy = 0, buttons = 0;
      self->hiddev->parser->getReportData(&dx, &dy, &buttons) ;
	
      PointingReport r ;
      r.t = timestamp ;
      r.dx = dx ;
      r.dy = dy ;
      r.btns = buttons ;
      self->report(r) ;      
    }

    void
    osxHIDPointingDevice::hidQueueCallback(void *context, IOReturn /*result*/, void *sender) {
#if !USE_IOHIDValueGetTimeStamp
      TimeStamp::inttime timestamp = TimeStamp::createAsInt() ;
#endif
    
      osxHIDPointingDevice *self = (osxHIDPointingDevice*)context ;
      IOHIDQueueRef queue = (IOHIDQueueRef)sender ;

      while (true) {

	IOHIDValueRef hidvalue = IOHIDQueueCopyNextValueWithTimeout(queue, 0.) ;
	if (!hidvalue) break ;

#if USE_IOHIDValueGetTimeStamp
	TimeStamp::inttime abstime = IOHIDValueGetTimeStamp(hidvalue) ;
	uint64_t nanoseconds = (abstime - self->epoch_mach) * self->mach_timebaseinfo.numer / self->mach_timebaseinfo.denom ;
	TimeStamp::inttime timestamp = self->epoch + nanoseconds*TimeStamp::one_nanosecond ;
#endif

#if DEBUG_MODE
	std::cerr << timestamp << " osxHIDPointingDevice::hidQueueCallback" << std::endl ;
#endif
	
	if (self->qreport.isOlderThan(timestamp)) {
	  self->report(self->qreport) ; // Flush the old qreport before creating a new one
	  self->qreport.clear() ;
	}

	IOHIDElementRef element = IOHIDValueGetElement(hidvalue) ;
	uint32_t usagepage = IOHIDElementGetUsagePage(element) ;
	uint32_t usage = IOHIDElementGetUsage(element) ;
	if (usagepage==kHIDPage_GenericDesktop) {
	  if (usage==kHIDUsage_GD_X || usage==kHIDUsage_GD_Y) {
	    // Could use IOHIDValueGetScaledValue(hidvalue, kIOHIDValueScaleTypePhysical)?
	    CFIndex d = IOHIDValueGetIntegerValue(hidvalue) ;
	    if (d) {
	      if (usage==kHIDUsage_GD_X) self->qreport.dx = (int32_t)d ; else self->qreport.dy = (int32_t)d ;
	      self->qreport.t = timestamp ;
	    }
	  }
	  // FIXME: GD_Z, GD_Wheel, etc.
	} else if (usagepage==kHIDPage_Button) {
	  self->qreport.setButton(usage-1, // kHIDUsage_Button_1 is 1
				  (uint32_t)IOHIDValueGetIntegerValue(hidvalue)) ;
	  self->qreport.t = timestamp ;
	}

	CFRelease(hidvalue) ;
      }

      // Flush the qreport we were constructing, if any
      if (self->qreport.t!=TimeStamp::undef)
	self->report(self->qreport) ;
      self->qreport.clear() ;
    }

    void
      osxHIDPointingDevice::report(osxHIDPointingDevice::PointingReport &r) {
      if (r.t) {
	registerTimestamp(r.t, r.dx, r.dy);
	if (callback) {
	  if (hiddev->debugLevel>1)
	    std::cerr << "osxHIDPointingDevice::report: " << r.toString() << std::endl ;
	  if (r.t==TimeStamp::undef) std::cerr << "TimeStamp::undef!" << std::endl ;
	  callback(callback_context, r.t, r.dx, r.dy, r.btns) ;
	} else if (hiddev->debugLevel>2) {
	  std::cerr << "osxHIDPointingDevice::report: skipping " << r.toString() << std::endl ;
	}
      }
    }

    // -----------------------------------------------------------------------

  }
