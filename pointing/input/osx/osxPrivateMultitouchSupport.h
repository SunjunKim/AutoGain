/* -*- mode: c++ -*-
 *
 * pointing/input/osx/osxPrivateMultitouchSupport.h --
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

#ifndef pointing_utils_osxPrivateMultitouchSupport_h
#define pointing_utils_osxPrivateMultitouchSupport_h

#include <CoreFoundation/CoreFoundation.h>

#if !TARGET_OS_IPHONE
#include <IOKit/IOTypes.h> 
#endif

namespace pointing {

  extern "C" {

    typedef CFTypeRef MTDeviceRef;
  
    typedef struct {
      float x ;
      float y ;
    } MTPoint ;
  
    typedef struct {
      MTPoint pos ;
      MTPoint vel ;
    } MTVector ;

    typedef enum {
      VERBOSE=0,
      LESS_VERBOSE=0x10000000
    } MTRunMode ;

    typedef enum {
      OFF=0,
      ON=1
    } MTEasyCallbackState ;

    typedef enum {
      NotTracking,
      StartInRange,
      HoverInRange,
      MakeTouch,
      Touching,
      BreakTouch,
      LingerInRange,
      OutOfRange
    } MTPathStage ;
  
    typedef struct {
      int32_t frame ;
      double timestamp ;
      int32_t pathIndex ;
      MTPathStage pathStage ;
      int32_t fingerId ;
      int32_t handId ;
      MTVector normVec ;
      float size ;
      int32_t zero1 ;
      float angle ;
      float majorAxis ;
      float minorAxis ; 
      MTVector absVec ; // in mm
      int32_t zero2 ;
      int32_t zero3 ;
      float zDensity ;
    } MTTouch ;

    double MTAbsoluteTimeGetCurrent(void) ;

    // ------------------------------------------------------------------------

    bool MTDeviceIsAvailable(void) ;

    CFArrayRef MTDeviceCreateList(void) ;

    // ------------------------------------------------------------------------

    MTDeviceRef MTDeviceCreateDefault(void) ;
    MTDeviceRef MTDeviceCreateFromDeviceID(int64_t dev) ;
    MTDeviceRef MTDeviceCreateFromGUID(uuid_t *guid) ; // FIXME: doesn't work with MTDeviceGetGUID...
#if !TARGET_OS_IPHONE
    MTDeviceRef MTDeviceCreateFromService(io_service_t service) ;
#endif

    OSStatus MTDeviceStart(MTDeviceRef dev, MTRunMode mode=LESS_VERBOSE) ;
    bool MTDeviceIsRunning(MTDeviceRef dev) ;
    void MTDeviceStop(MTDeviceRef dev) ;

    // Those two are actually called by MTDeviceStart
    CFRunLoopSourceRef MTDeviceCreateMultitouchRunLoopSource(MTDeviceRef dev) ;
    OSStatus MTDeviceScheduleOnRunLoop(MTDeviceRef dev, CFRunLoopRef runloop, CFStringRef mode) ;

    void MTDeviceRelease(MTDeviceRef dev) ;

    // ------------------------------------------------------------------------

    OSStatus MTDeviceGetFamilyID(MTDeviceRef dev, int32_t *familyId) ;
    OSStatus MTDeviceGetDeviceID(MTDeviceRef dev, uint64_t* id) ;
    OSStatus MTDeviceGetDriverType(MTDeviceRef dev, int32_t *dType) ;
    OSStatus MTDeviceGetActualType(MTDeviceRef dev, int32_t *aType) ;
    OSStatus MTDeviceGetGUID(MTDeviceRef dev, uuid_t *guid) ;
    OSStatus MTDeviceGetSerialNumber(MTDeviceRef dev, uint64_t* id) ; // Not sure...
#if !TARGET_OS_IPHONE
    io_service_t MTDeviceGetService(MTDeviceRef dev) ;
#endif

    // width and height are returned in hundreds of mm
    OSStatus MTDeviceGetSensorSurfaceDimensions(MTDeviceRef dev, int32_t *width, int32_t *height) ;

    OSStatus MTDeviceGetSensorDimensions(MTDeviceRef dev, int32_t *rows, int32_t *cols) ;

    char *MTGetPathStageName(MTPathStage pathstage) ;

    bool MTDeviceIsAlive(MTDeviceRef dev) ;
    bool MTDeviceIsValid(MTDeviceRef dev) ;
    bool MTDeviceIsMTHIDDevice(MTDeviceRef dev);
    bool MTDeviceIsBuiltIn(MTDeviceRef dev) ;
    bool MTDeviceIsOpaqueSurface(MTDeviceRef dev) ; // works only when device is running

    bool MTDevicePowerControlSupported(MTDeviceRef dev) ;
    void MTDeviceSetUILocked(MTDeviceRef dev, bool state) ;

    // ------------------------------------------------------------------------

    void MTEasyInstallPrintCallbacks(MTDeviceRef dev, MTEasyCallbackState state=ON) ;

    typedef void (*MTContactFrameCallback)(MTDeviceRef dev, MTTouch *contacts, int32_t numContacts, double timestamp, int32_t frameNum) ;
    void MTRegisterContactFrameCallback(MTDeviceRef dev, MTContactFrameCallback callback) ;
    typedef void (*MTContactFrameCallbackWithRefCon)(MTDeviceRef dev, MTTouch *contacts, int32_t numContacts, double timestamp, int32_t frameNum, void *refCon) ;
    void MTRegisterContactFrameCallbackWithRefcon(MTDeviceRef dev, MTContactFrameCallbackWithRefCon callback, void *refCon) ;
    void MTUnregisterContactFrameCallback(MTDeviceRef dev, MTContactFrameCallback callback) ;

    typedef int (*MTDevicePathCallbackFunction)(MTDeviceRef dev, int32_t pathIndex, MTPathStage pathStage, MTTouch *contact) ;
    void MTRegisterPathCallback(MTDeviceRef, MTDevicePathCallbackFunction, void *refCon) ;
    void MTUnregisterPathCallback(MTDeviceRef, MTDevicePathCallbackFunction, void *refCon) ;

    typedef void (*MTButtonStateCallback)(MTDeviceRef dev, int32_t pressed, int32_t released, void *refCon) ;
    void MTRegisterButtonStateCallback(MTDeviceRef dev, MTButtonStateCallback callback, void *refCon) ;
    void MTUnregisterButtonStateCallback(MTDeviceRef dev, MTButtonStateCallback callback, void *refCon) ;

    // There's also
    //   void MTRegisterImageCallback(MTDeviceRef dev, ...) ;
    //   void MTUnregisterImageCallback(MTDeviceRef dev, ...) ;
    //   void MTRegisterFullFrameCallback(MTDeviceRef dev, ...) ;
    //   void MTUnregisterFullFrameCallback(MTDeviceRef dev, ...) ;
    // and a few others...

    // ------------------------------------------------------------------------

    inline const char*
    MTPathStageName(MTPathStage ps) {
      switch (ps) {
      case NotTracking: return "NotTracking" ;
      case StartInRange: return "StartInRange" ;
      case HoverInRange: return "HoverInRange" ;
      case MakeTouch: return "MakeTouch" ;
      case Touching: return "Touching" ;
      case BreakTouch: return "BreakTouch" ;
      case LingerInRange: return "LingerInRange" ;
      case OutOfRange: return "OutOfRange" ;
      }
      return "???" ;
    }
   
  }

}

#endif
