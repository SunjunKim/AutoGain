/* -*- mode: c++ -*-
 *
 * pointing/output/windows/winDisplayDevice.h --
 *
 * Initial software
 * Authors: Géry Casiez
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/output/windows/winDisplayDevice.h>

#include <iostream>
#include <sstream>
#include <string.h>

namespace pointing {

  winDisplayDevice::winDisplayDevice(void) {
    displayID = L"";
  }

  winDisplayDevice::winDisplayDevice(URI uri) {
    displayID = L"";
    dinfo.monitorName[0] = '\0';
    dinfo.ppi = 0;
    dinfo.refreshRate = 0;
    dinfo.resx = 0;
    dinfo.resy = 0;
    dinfo.w = 0;
    dinfo.h = 0;

    URI::getQueryArg(uri.query, "bw", &dinfo.resx);
    URI::getQueryArg(uri.query, "bh", &dinfo.resy);
    URI::getQueryArg(uri.query, "w", &dinfo.w);
    URI::getQueryArg(uri.query, "h", &dinfo.h);
    URI::getQueryArg(uri.query, "hz", &dinfo.refreshRate);

    if (uri.path.empty()) {
      if (numberDisplays() == 1)
	displayID = getFirstDisplay();
      else {
  int numberDisplays;
  std::wstring monitorName = getControlPanelDisplayName(&numberDisplays);
  if (numberDisplays >= 1)
    displayID = getMatchingRegistryKey(monitorName);
      }
    }
    else {
      std::string displayIDtmp ;
      std::istringstream to_string ;
      to_string.str(uri.path.erase(0,1)) ; // Skip the leading '/'
      to_string >> displayIDtmp ;
      std::wstring str2(displayIDtmp.length(), L' '); // Make room for characters
      std::copy(displayIDtmp.begin(), displayIDtmp.end(), str2.begin());
      displayID = str2;
    }
    int debugLevel = 0 ;
    if (URI::getQueryArg(uri.query, "debugLevel", &debugLevel)) {
      setDebugLevel(debugLevel) ;
      listDisplays(std::wcerr) ;
    }
    if (isdisplayIDvalid(displayID))
      getDisplayInfo(displayID, &dinfo);
    else
      std::wcerr << "[WARNING] Unable to set a display from your URI " << uri.asString().c_str() << std::endl;
  }

  DisplayDevice::Size
  winDisplayDevice::getSize(Size * /*defval*/) {
    /*
    // Does not work properly:
    int widthMM=0, heightMM=0;
    HWND hwnd = GetDesktopWindow();
    HDC hdc = GetDC(hwnd);
  
    widthMM = GetDeviceCaps(hdc, HORZSIZE);
    heightMM = GetDeviceCaps(hdc, VERTSIZE);
    ReleaseDC(hwnd, hdc);*/

    return DisplayDevice::Size(dinfo.w, dinfo.h) ;
  }

  double
  winDisplayDevice::getRefreshRate(double *defval) {
    /*HWND hwnd = GetDesktopWindow();
      HDC hdc = GetDC(hwnd);
  
      result = GetDeviceCaps(hdc, VREFRESH);

      ReleaseDC(hwnd, hdc);*/

    double result = dinfo.refreshRate ;
    if (!result) 
      result = defval ? *defval : 60 ; // 60 = default refresh rate for LCDs
    return result ;
  }

  DisplayDevice::Bounds
  winDisplayDevice::getBounds(Bounds * /*defval*/) {
    /*int width, height;
      HWND hwnd = GetDesktopWindow();
      HDC hdc = GetDC(hwnd);

      width = GetDeviceCaps(hdc, HORZRES);
      height = GetDeviceCaps(hdc, VERTRES);

      ReleaseDC(hwnd, hdc);

      result = DisplayDevice::Bounds(0, 0, // 0,0 because it's the main display
      width,
      height) ;*/

    return DisplayDevice::Bounds(0, 0, // 0,0 because it's the main display
				 dinfo.resx,
				 dinfo.resy) ;
  }

  URI
  winDisplayDevice::getURI(bool expanded) const {
    URI uri ;
    std::string simpleString;
    simpleString.assign(displayID.begin(), displayID.end());
    uri.scheme = "windisplay" ;
    std::stringstream path ;
    path << "/" << simpleString ;
    uri.path = path.str() ;
    if (expanded)
    {
      URI::addQueryArg(uri.query, "bw", dinfo.resx);
      URI::addQueryArg(uri.query, "bh", dinfo.resy);
      URI::addQueryArg(uri.query, "w", dinfo.w);
      URI::addQueryArg(uri.query, "h", dinfo.h);
      URI::addQueryArg(uri.query, "hz", dinfo.refreshRate);
    }

    return uri ;
  }
}
