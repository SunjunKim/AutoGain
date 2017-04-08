/* -*- mode: c++ -*-
 *
 * pointing/input/osx/osxPrivateMultitouchDevice.cpp --
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

#include <pointing/input/osx/osxPrivateMultitouchDevice.h>

#include <iomanip>
#include <stdexcept>
#include <cmath>

namespace pointing {

  static std::string
  uuid2str(uuid_t bytes) {
    CFUUIDRef uuidref = CFUUIDCreateWithBytes(kCFAllocatorDefault, bytes[0],bytes[1],bytes[2],bytes[3],bytes[4],bytes[5],bytes[6],bytes[7],bytes[8],bytes[9],bytes[10],bytes[11],bytes[12],bytes[13],bytes[14],bytes[15]) ;
    CFStringRef str = CFUUIDCreateString(kCFAllocatorDefault, uuidref) ;
    CFIndex slen = CFStringGetLength(str) ;
    CFStringEncoding encoding = kCFStringEncodingUTF8 ;
    char *strbytes = (char*) CFStringGetCStringPtr(str, encoding) ;
    bool ok = true, freebytes = false ;
    if (strbytes == NULL) {
      freebytes = true ;
      strbytes = new char [4*slen +1] ;
      ok  = CFStringGetCString(str, strbytes, slen, encoding) ;
    }
    std::string result = std::string(strbytes) ;
    if (freebytes) delete [] strbytes ;
    CFRelease(str) ;
    CFRelease(uuidref) ;
    return result ;
  }

  static void
  str2uuid(const char *str, uuid_t uuid) {
    CFStringRef cfstr = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
							str, kCFStringEncodingUTF8,
							kCFAllocatorNull) ;
    CFUUIDRef cfuuid = CFUUIDCreateFromString(kCFAllocatorDefault, cfstr) ;
    CFUUIDBytes cfuuidbytes = CFUUIDGetUUIDBytes(cfuuid) ;
    uuid[0] = cfuuidbytes.byte0 ;
    uuid[1] = cfuuidbytes.byte1 ;
    uuid[2] = cfuuidbytes.byte2 ;
    uuid[3] = cfuuidbytes.byte3 ;
    uuid[4] = cfuuidbytes.byte4 ;
    uuid[5] = cfuuidbytes.byte5 ;
    uuid[6] = cfuuidbytes.byte6 ;
    uuid[7] = cfuuidbytes.byte7 ;
    uuid[8] = cfuuidbytes.byte8 ;
    uuid[9] = cfuuidbytes.byte9 ;
    uuid[10] = cfuuidbytes.byte10 ;
    uuid[11] = cfuuidbytes.byte11 ;
    uuid[12] = cfuuidbytes.byte12 ;
    uuid[13] = cfuuidbytes.byte13 ;
    uuid[14] = cfuuidbytes.byte14 ;
    uuid[15] = cfuuidbytes.byte15 ;
    CFRelease(cfuuid) ;
    CFRelease(cfstr) ;
  }

  osxPrivateMultitouchDevice::osxPrivateMultitouchDevice(int dbglevel) {
    debugLevel = dbglevel ;
    device = MTDeviceCreateDefault() ;
    if (!device) throw std::runtime_error("Unable to get MTDeviceRef") ;
  }

  void
  osxPrivateMultitouchDevice::listDevices(void) {
    CFArrayRef devices = MTDeviceCreateList() ;
    for (int i=0; i<CFArrayGetCount(devices); i++) {
      MTDeviceRef dev = (MTDeviceRef)CFArrayGetValueAtIndex(devices, i) ;
      uuid_t tmp_uuid ;
      MTDeviceGetGUID(dev, &tmp_uuid) ;
      uint64_t devid ;
      MTDeviceGetDeviceID(dev, &devid) ;
      std::cerr << "  #" << (i+1) << ": " << dev
		<< " ID=" << devid
		<< " GUID=" << uuid2str(tmp_uuid)
		<< std::endl ;
    }
  }
  
  osxPrivateMultitouchDevice::osxPrivateMultitouchDevice(const char *id, int dbglevel) {
    debugLevel = dbglevel ;

    uint64_t devid = strtoull(id, NULL, 10) ;
    device = MTDeviceCreateFromDeviceID(devid) ;

    if (!device) {
      uuid_t uuid ;
      str2uuid(id, uuid) ;
      device = MTDeviceCreateFromGUID(&uuid) ;
    }
    
    if (!device) {
      std::cerr << "osxPrivateMultitouchDevice: bad device ID or GUID" << std::endl ;
      listDevices() ;
      throw std::runtime_error("Unable to get MTDeviceRef") ;
    }
  }

  osxPrivateMultitouchDevice::osxPrivateMultitouchDevice(uint64_t devid, int dbglevel) {
    debugLevel = dbglevel ;
    device = MTDeviceCreateFromDeviceID(devid) ;
    if (!device) {
      std::cerr << "osxPrivateMultitouchDevice: bad device ID" << std::endl ;
      listDevices() ;
      throw std::runtime_error("Unable to get MTDeviceRef") ;
    }
  }

  uint64_t
  osxPrivateMultitouchDevice::getID(void) {
    uint64_t devid ;
    MTDeviceGetDeviceID(device, &devid) ;
    return devid ;
  }

  std::string
  osxPrivateMultitouchDevice::getGUID(void) {
    uuid_t uuid ;
    MTDeviceGetGUID(device, &uuid) ;
    return uuid2str(uuid) ;
  }

  int
  osxPrivateMultitouchDevice::getSensorDimensions(int *cols, int *rows) {
    int result = MTDeviceGetSensorDimensions(device, rows, cols) ;
    if (result!=0)
      *cols = *rows = 0 ;
    return result ;
  }

  int
  osxPrivateMultitouchDevice::getSensorSurfaceDimensions(int *width, int *height) {
    int result = MTDeviceGetSensorSurfaceDimensions(device, width, height) ;
    if (result==0) {
      *width /= 100 ;
      *height /= 100 ;
    } else {
      *width = *height = 0 ;
    }
    return result ;
  }

  MTVector
  osxPrivateMultitouchDevice::norm2abs(MTVector &norm) {
    int width=0, height=0 ;
    getSensorSurfaceDimensions(&width, &height) ;
    MTVector result ;
    result.pos.x = norm.pos.x * width ;
    result.pos.y = norm.pos.y * height ;
    result.vel.x = norm.vel.x * height ;
    result.vel.y = norm.vel.y * height ;
    return result ;
  }
  
  void
  osxPrivateMultitouchDevice::easyInstallPrintCallbacks(void) {
    MTEasyInstallPrintCallbacks(device, ON) ;
  }

  void
  osxPrivateMultitouchDevice::registerContactFrameCallback(MTContactFrameCallback callback) {
    MTRegisterContactFrameCallback(device, callback) ;
  }

  void
  osxPrivateMultitouchDevice::registerContactFrameCallbackWithRefCon(MTContactFrameCallbackWithRefCon callback, void *refCon) {
    MTRegisterContactFrameCallbackWithRefcon(device, callback, refCon) ;
  }

  void
  osxPrivateMultitouchDevice::unregisterContactFrameCallback(MTContactFrameCallback callback) {
    MTUnregisterContactFrameCallback(device, callback) ;
  }

  void
  osxPrivateMultitouchDevice::registerButtonStateCallback(MTButtonStateCallback callback, void *refCon) {
    MTRegisterButtonStateCallback(device, callback, refCon) ;
  }

  void
  osxPrivateMultitouchDevice::unregisterButtonStateCallback(MTButtonStateCallback callback, void *refCon) {
    MTUnregisterButtonStateCallback(device, callback, refCon) ;
  }

  void
  osxPrivateMultitouchDevice::start(MTRunMode mode) {
    MTDeviceStart(device, mode) ;
  }

  void
  osxPrivateMultitouchDevice::stop(void) {
    MTDeviceStop(device) ;
  }

  osxPrivateMultitouchDevice::~osxPrivateMultitouchDevice(void) {
    stop() ;
    MTDeviceRelease(device) ;
  }

  void
  osxPrivateMultitouchDevice::debug(MTTouch &c, std::ostream& out) {
    double ratio = (c.minorAxis>0) ? (c.majorAxis/c.minorAxis) : 1.0 ;
    double speed = sqrt(pow(c.absVec.vel.x,2)+pow(c.absVec.vel.y,2)) ;
    out << "  "
#if 0
	<< "F" << c.frame << " @" << c.timestamp << " "
#endif
	<< "P" << std::setfill('0') << std::setw(2) << c.pathIndex
	<< " " << MTGetPathStageName(c.pathStage)
	<< " H" << c.handId << " F" << c.fingerId
	<< " (" << c.absVec.pos.x << "," << c.absVec.pos.y << ")mm"
	<< " (" << c.normVec.pos.x << "," << c.normVec.pos.y << ")"
	<< " " << c.size << "ZTot"
	<< " " << c.zDensity << "ZDen"
	<< " " << c.majorAxis << "MR/" << c.minorAxis << "mR=" << ratio
	<< " " << c.angle*180/M_PI
	<< " (" << c.absVec.vel.x << "," << c.absVec.vel.y << ")->" << speed << "mm/s"
#if 0
	<< " [" << c.zero1 << ", " << c.zero2 << " " << c.zero3 << "]"
#endif
	<< std::endl ;
  }

  void
  osxPrivateMultitouchDevice::debug(MTDeviceRef device,
				    MTTouch *contacts, int32_t numContacts, double timestamp, int32_t frameNum,
				    std::ostream& out) {
    out << "D" << device << " F" << frameNum << " @" << timestamp << " " << numContacts << std::endl ;
    for (int i=0; i<numContacts; ++i) {
      MTTouch &c = contacts[i] ;
      out << "  " ;
      debug(c, out) ;
    }
  }

}
