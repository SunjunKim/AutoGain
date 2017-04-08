/* -*- mode: c++ -*-
 *
 * pointing/output/osx/osxDisplayDevice.cpp --
 *
 * Initial software
 * Authors: Géry Casiez, Nicolas Roussel
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/output/osx/osxDisplayDevice.h>

#include <IOKit/graphics/IOGraphicsLib.h>

#include <iostream>
#include <sstream>

namespace pointing {

  void
  osxDisplayDevice::listDisplays(std::ostream& out) {
    CGDisplayCount dspyCnt = 0 ;
    CGDisplayErr err = CGGetActiveDisplayList(0, 0, &dspyCnt) ;
    out << "osxDisplayDevice: found " << dspyCnt << " display(s)" << std::endl ;
    CGDirectDisplayID *activeDspys = new CGDirectDisplayID [dspyCnt] ;
    err = CGGetActiveDisplayList(dspyCnt, activeDspys, &dspyCnt) ;
    for (unsigned int i=0; i<dspyCnt; ++i) {
      CGDirectDisplayID did = activeDspys[i] ;
      osxDisplayDevice device(did) ;
      DisplayDevice::Size size = device.getSize() ;
      DisplayDevice::Bounds bounds = device.getBounds() ;
      out << "  cgdisplay:/" << did
	  << " [" << size.width << "x" << size.height << " mm"
	  << ", " << bounds.size.width << "x" << bounds.size.height << " pixels"
	  << ", " << device.getResolution() << " PPI"
	  << ", " << device.getRefreshRate() << " Hz]" ;
      if (CGDisplayIsMain(did))
	out << " main" ;
      if (CGDisplayIsInHWMirrorSet(did))
	out << " hw_mirror" ;
      else if (CGDisplayIsInMirrorSet(did)) 
	out << " sw_mirror" ;
      out << std::endl ;
    }
    delete [] activeDspys ;
  }

  osxDisplayDevice::osxDisplayDevice(void) {
    displayID = CGMainDisplayID() ;
    cached = NOTHING ;
  }

  void osxDisplayDevice::cacheAll(URI &uri)
  {
      cached = NOTHING;
      if (URI::getQueryArg(uri.query, "hz", &cached_refreshrate))
          cached |= REFRESHRATE;
      if (URI::getQueryArg(uri.query, "bx", &cached_bounds.origin.x)
        | URI::getQueryArg(uri.query, "by", &cached_bounds.origin.y)
        | URI::getQueryArg(uri.query, "bw", &cached_bounds.size.width)
        | URI::getQueryArg(uri.query, "bh", &cached_bounds.size.height))
          cached |= BOUNDS;
      if (URI::getQueryArg(uri.query, "w", &cached_size.width)
        | URI::getQueryArg(uri.query, "h", &cached_size.height))
          cached |= SIZE;
      getRefreshRate();
      getSize();
      getBounds();
  }

  osxDisplayDevice::osxDisplayDevice(URI uri) {
    if (uri.path.empty()) 
      displayID = CGMainDisplayID() ;
    else {
      std::istringstream to_int ;
      to_int.str(uri.path.erase(0,1)) ; // Skip the leading '/'
      to_int >> displayID ;
    }
    int debugLevel = 0 ;
    if (URI::getQueryArg(uri.query, "debugLevel", &debugLevel)) {
      setDebugLevel(debugLevel) ;
      if (debugLevel>0) listDisplays(std::cerr) ;
    }
    cacheAll(uri);
  }

  osxDisplayDevice::osxDisplayDevice(CGDirectDisplayID did) {
    displayID = did ;
    cached = NOTHING ;
  }

  DisplayDevice::Bounds
  osxDisplayDevice::getBounds(Bounds */*defval*/) {
    if (cached&BOUNDS) return cached_bounds ;

    CGDisplayModeRef dispMode = CGDisplayCopyDisplayMode(displayID) ;
    //cached_bounds = DisplayDevice::Bounds(0, 0, // 0,0 because it's the main display
    //				  CGDisplayModeGetWidth(dispMode),
    //				  CGDisplayModeGetHeight(dispMode)) ;
    CGRect rect = CGDisplayBounds(displayID);
    cached_bounds.origin.x = rect.origin.x;
    cached_bounds.origin.y = rect.origin.y;
    cached_bounds.size.width = rect.size.width;
    cached_bounds.size.height = rect.size.height;

    CGDisplayModeRelease(dispMode) ;
    cached = cached | BOUNDS ;

    return cached_bounds ;
  }

  DisplayDevice::Size
  osxDisplayDevice::getSize(Size */*defval*/) {
    if (cached&SIZE) return cached_size ;

    CGSize size = CGDisplayScreenSize(displayID) ;
    cached_size = DisplayDevice::Size(size.width, size.height) ;
    cached = cached | SIZE ;

    return cached_size ;
  }

  double
  osxDisplayDevice::getRefreshRate(double *defval) {
    if (cached&REFRESHRATE) return cached_refreshrate ;

    CGDisplayModeRef dispMode = CGDisplayCopyDisplayMode(displayID) ;
    cached_refreshrate = CGDisplayModeGetRefreshRate(dispMode) ;
    if (!cached_refreshrate) 
      cached_refreshrate = defval ? *defval : 60 ; // 60 = default refresh rate for LCDs
    CGDisplayModeRelease(dispMode) ;
    cached = cached | REFRESHRATE ;
  
    return cached_refreshrate ;
  }

  URI
  osxDisplayDevice::getURI(bool expanded) const {
    URI uri ;
    uri.scheme = "osxdisplay" ;
    std::stringstream path ;
    path << "/" << displayID ;
    uri.path = path.str() ;
    if (expanded)
    {
        if (cached & BOUNDS)
        {
            URI::addQueryArg(uri.query, "bx", cached_bounds.origin.x) ;
            URI::addQueryArg(uri.query, "by", cached_bounds.origin.y) ;
            URI::addQueryArg(uri.query, "bw", cached_bounds.size.width) ;
            URI::addQueryArg(uri.query, "bh", cached_bounds.size.height) ;
        }
        if (cached & SIZE)
        {
            URI::addQueryArg(uri.query, "w", cached_size.width) ;
            URI::addQueryArg(uri.query, "h", cached_size.height) ;
        }
        if (cached & REFRESHRATE)
        {
            URI::addQueryArg(uri.query, "hz", cached_refreshrate) ;
        }
    }
    return uri ;
  }

#if 0
  int
  osxDisplayDevice::getDepth(void) const {
    CGDisplayModeRef dispMode = CGDisplayCopyDisplayMode(displayID) ;
    CFStringRef s = CGDisplayModeCopyPixelEncoding(dispMode) ;
    // CFShow(s) ; // See also IOKit.framework/Headers/graphics/IOGraphicsTypes.h
    int bpp = 0 ;
    if (s) {
      if (!CFStringCompare(s, CFSTR(IO1BitIndexedPixels), kCFCompareCaseInsensitive))
	bpp = 1 ;
      else if (!CFStringCompare(s, CFSTR(IO2BitIndexedPixels), kCFCompareCaseInsensitive))
	bpp = 2 ;
      else if (!CFStringCompare(s, CFSTR(IO4BitIndexedPixels), kCFCompareCaseInsensitive))
	bpp = 4 ;
      else if (!CFStringCompare(s, CFSTR(IO8BitIndexedPixels), kCFCompareCaseInsensitive))
	bpp = 8 ;
      else if (!CFStringCompare(s, CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive))
	bpp = 16 ;
      else if (!CFStringCompare(s, CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive))
	bpp = 32 ;
      else if (!CFStringCompare(s, CFSTR(kIO30BitDirectPixels), kCFCompareCaseInsensitive))
	bpp = 30 ;
      else if (!CFStringCompare(s, CFSTR(kIO64BitDirectPixels), kCFCompareCaseInsensitive))
	bpp = 64 ;
      else if (!CFStringCompare(s, CFSTR(kIO16BitFloatPixels), kCFCompareCaseInsensitive))
	bpp = 16 ;
      else if (!CFStringCompare(s, CFSTR(kIO32BitFloatPixels), kCFCompareCaseInsensitive))
	bpp = 32 ;
      CFRelease(s) ;
    }
    CGDisplayModeRelease(dispMode) ;
    return bpp ;
  }
#endif

}
