/* -*- mode: c++ -*-
 *
 * pointing/input/osx/osxPrivateMultitouchDevice.h --
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

#ifndef osxPrivateMultitouchDevice_h
#define osxPrivateMultitouchDevice_h

#include <pointing/input/PointingDevice.h>
#include <pointing/input/osx/osxPrivateMultitouchSupport.h>

#include <iostream>

namespace pointing {

  class osxPrivateMultitouchDevice {

    MTDeviceRef device ;

    static void listDevices(void) ;
    
  public:

    int debugLevel ;

    static void debug(MTTouch &contact, std::ostream& out) ;

    static void debug(MTDeviceRef device,
		      MTTouch *contacts, int32_t numContacts, double timestamp, int32_t frameNum,
		      std::ostream& out) ;

    osxPrivateMultitouchDevice(int debugLevel=0) ;
    osxPrivateMultitouchDevice(uint64_t devid, int debugLevel=0) ;
    osxPrivateMultitouchDevice(const char *id, int debugLevel=0) ; // will try device ID, then GUID

    uint64_t getID(void) ;
    std::string getGUID(void) ;

    int getSensorDimensions(int *cols, int *rows) ;
    int getSensorSurfaceDimensions(int *width, int *height) ;

    MTVector norm2abs(MTVector &norm) ;
    
    void easyInstallPrintCallbacks(void) ;

    void registerContactFrameCallback(MTContactFrameCallback callback) ;
    void registerContactFrameCallbackWithRefCon(MTContactFrameCallbackWithRefCon callback, void *refCon) ;
    void unregisterContactFrameCallback(MTContactFrameCallback callback) ;

    void registerButtonStateCallback(MTButtonStateCallback callback, void *refCon=0) ;
    void unregisterButtonStateCallback(MTButtonStateCallback callback, void *refCon=0) ;

    void start(MTRunMode mode=LESS_VERBOSE) ;
    void stop(void) ;

    ~osxPrivateMultitouchDevice(void) ;

  } ;

}

#endif
