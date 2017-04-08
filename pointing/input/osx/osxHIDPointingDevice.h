/* -*- mode: c++ -*-
 *
 * pointing/input/osx/osxHIDPointingDevice.h --
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

#ifndef osxHIDPointingDevice_h
#define osxHIDPointingDevice_h

#include <pointing/input/osx/osxHIDInputDevice.h>
#include <pointing/input/PointingDevice.h>
#include <pointing/utils/URI.h>
#include <pointing/utils/TimeStamp.h>

#include <mach/mach_time.h>

namespace pointing {

  class osxHIDPointingDevice : public PointingDevice {

    struct PointingReport {
      TimeStamp::inttime t ;
      int32_t dx, dy ;
      uint32_t btns ;

      PointingReport(void) ;
      PointingReport& operator = (PointingReport& src) ;
      void clear(void) ; // preserves btns
      bool isOlderThan(TimeStamp::inttime t) const ;
      bool setButton(uint32_t index, uint32_t value) ;
      std::string toString(void) const ;
    } ;

    TimeStamp::inttime epoch ;
    TimeStamp::inttime epoch_mach ;
    mach_timebase_info_data_t mach_timebaseinfo ;
    
    int vendorID, productID ; 
    int primaryUsagePage, primaryUsage ;

    osxHIDInputDevice *hiddev ;
    PointingCallback callback ;
    void *callback_context ;
    bool use_report_callback ;
    bool use_queue_callback ;
    PointingReport qreport ;

    double forced_cpi, forced_hz ;

    bool isUSB(void) ;
    bool isBluetooth(void) ;

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
    static void hidReportCallback(void *context, IOReturn result, void *sender,
				  IOHIDReportType type, uint32_t reportID,
				  uint8_t *report, CFIndex reportLength,
				  uint64_t timeStamp) ;
#else
    static void hidReportCallback(void *context, IOReturn result, void *sender,
				  IOHIDReportType type, uint32_t reportID, 
				  uint8_t *report, CFIndex reportLength) ;
#endif
    
    static void hidQueueCallback(void *context, IOReturn result, void *sender) ;

    void report(osxHIDPointingDevice::PointingReport &r) ;

  public:

    osxHIDPointingDevice(URI uri) ;

    bool isActive(void) const ;

    int getVendorID(void) const ;
    std::string getVendor(void) const ;
    int getProductID(void) const ;
    std::string getProduct(void) const ;
    double getResolution(double *defval=0) const ;
    double getUpdateFrequency(double *defval=0) const ;

    URI getURI(bool expanded=false, bool crossplatform=false) const ;

    void setPointingCallback(PointingCallback callback, void *context=0) ;

    void setDebugLevel(int level) ;

    ~osxHIDPointingDevice(void) ;

  } ;

}

#endif
