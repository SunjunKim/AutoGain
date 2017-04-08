/* -*- mode: c++ -*-
 *
 * pointing/output/windows/winDisplayDeviceManager.cpp --
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

#include <pointing/output/windows/winDisplayDeviceManager.h>
#include <pointing/output/windows/winDisplayDevice.h>
#include <pointing/output/windows/winDisplayDeviceHelper.h>
#include <windows.h>
#include <iterator>
#include <algorithm>
#include <iostream>
#include <set>

using namespace std;

namespace pointing
{
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

  winDisplayDeviceManager::winDisplayDeviceManager():msgWndw(0)
  {
    scanDisplays();
    loopThread = CreateThread(NULL, 0, Loop, LPVOID(this), 0, &dwThreadId);
  }

  DWORD WINAPI winDisplayDeviceManager::Loop(LPVOID lpvThreadParam)
  {
    winDisplayDeviceManager* self = (winDisplayDeviceManager*) lpvThreadParam;
    self->msgWndw = 0;
    WNDCLASSEX w;
    memset(&w, 0, sizeof(w));
    w.cbSize = sizeof(w);
    w.lpfnWndProc = (WNDPROC)winDisplayDeviceManager::wndwProc;
    w.lpszClassName = L"ClassPointingDeviceManager";
    ATOM atom = ::RegisterClassEx(&w);
    if (!atom){ throw std::runtime_error("winDisplayDeviceManager::Unable to register a new windows class for message processing"); }
    self->msgWndw=CreateWindow( w.lpszClassName,
                                L"",
                                WS_BORDER | WS_CAPTION,
                                0, 0, 0, 0,
                                HWND_DESKTOP ,
                                NULL,
                                NULL,
                                NULL);
    if(!self->msgWndw){ throw std::runtime_error("winDisplayDeviceManager::Unable to create a message window"); }
    SetWindowLongPtr(self->msgWndw, GWLP_USERDATA, (LONG_PTR)self);

    MSG messages;
    while(GetMessage(&messages, NULL, 0, 0))
    {
      TranslateMessage(&messages);
      DispatchMessage(&messages);
    }
    return messages.wParam;
  }

  URI uriForDisplayId(wstring displayId)
  {
    URI uri;
    std::string simpleString;
    simpleString.assign(displayId.begin(), displayId.end());
    uri.scheme = "windisplay" ;
    std::stringstream path ;
    path << "/" << simpleString ;
    uri.path = path.str() ;
    return uri ;
  }

  bool winDisplayDeviceManager::ConvertDevice(HMONITOR hMon, DisplayDeviceDescriptor &desc)
  {
    MONITORINFOEX mInfo;
    mInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMon,&mInfo);
    desc.width = mInfo.rcMonitor.right - mInfo.rcMonitor.left;
    desc.height = mInfo.rcMonitor.bottom - mInfo.rcMonitor.top;

    DISPLAY_DEVICE ddMon;
    ZeroMemory(&ddMon, sizeof(ddMon));
    ddMon.cb = sizeof(ddMon);
    DWORD devMon = 0;

    while (EnumDisplayDevices(mInfo.szDevice, devMon, &ddMon, 0))
    {
      if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE)
        break;
      devMon++;
    }

    // DeviceID = 0x0042ebd4 L"MONITOR\\HWP2847\\{4d36e96e-e325-11ce-bfc1-08002be10318}\\0004"
    wstring displayId = ddMon.DeviceID;
    /* Remove MONITOR\\ */
    displayId = displayId.substr(displayId.find(L"\\") + 1);
    /* Remove \\{4d36e96e-e325-11ce-bfc1-08002be10318}\\0004" */
    displayId = displayId.substr(0, displayId.find(L"\\"));
    // Now displayId = HWP2847
    winDisplayInfo dInfo;
    if (isdisplayIDvalid(displayId))
    {
      getDisplayInfo(displayId, &dInfo);
      desc.devURI = uriForDisplayId(displayId).asString();
      desc.name = dInfo.monitorName;
      return true;
    }
    return false;
  }

  BOOL CALLBACK winDisplayDeviceManager::EnumDispProc(HMONITOR hMon, HDC, RECT*, LPARAM lParam)
  {
    winDisplayDeviceManager *self = (winDisplayDeviceManager *)lParam;
    DisplayDeviceDescriptor desc;
    if (self->ConvertDevice(hMon, desc))
      self->descMap[hMon] = desc;
    return true;
  }

  void winDisplayDeviceManager::scanDisplays()
  {
    // TODO mutex for this variable ???
    descMap.clear();
    EnumDisplayMonitors(0,0,winDisplayDeviceManager::EnumDispProc,(LPARAM)this);

    // Add new descriptors
    set<DisplayDeviceDescriptor> tempSet, resultSet;
    for (descMap_t::iterator it = descMap.begin(); it != descMap.end(); it++)
    {
      addDevice(it->second);
      tempSet.insert(it->second);
    }

    // We could just copy the new descriptors, however, we need to call callback functions
    // for old displays, that's why we do this:
    std::set_difference(descriptors.begin(), descriptors.end(), tempSet.begin(), tempSet.end(),
                        std::inserter(resultSet, resultSet.end()));

    set<DisplayDeviceDescriptor>::iterator it = resultSet.begin();
    for (; it != resultSet.end(); it++)
    {
      DisplayDeviceDescriptor desc = *it;
      removeDevice(desc);
    }
  }

  LONG APIENTRY winDisplayDeviceManager::wndwProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
  {
    switch(message)
    {
    case WM_DISPLAYCHANGE:
    {
      winDisplayDeviceManager* self=(winDisplayDeviceManager*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
      self->scanDisplays();
      break;
    }
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
  }

  DisplayDescriptorSet winDisplayDeviceManager::readAllFromRegistry()
  {
    DisplayDescriptorSet result;
    list<wstring> allKeys = getAllKeys();
    for (list<wstring>::iterator it = allKeys.begin(); it != allKeys.end(); it++)
    {
      wstring achKey = *it;
      winDisplayInfo dinfo;
      getDisplayInfo(achKey,&dinfo);
      if ((dinfo.w > 0) && (dinfo.h > 0))
      {
        DisplayDeviceDescriptor desc;
        desc.devURI = uriForDisplayId(achKey).asString();
        desc.name = dinfo.monitorName;
        result.insert(desc);
      }
    }
    return result;
  }

  // FIXME: There is no guarantee that descMap contains the info required
  // Because of threading issue. Must implement mutex
  DisplayDeviceDescriptor winDisplayDeviceManager::uriFromPoint(int x, int y)
  {
    POINT p;
    p.x = x;
    p.y = y;
    HMONITOR hMon = MonitorFromPoint(p, MONITOR_DEFAULTTOPRIMARY);
    return descMap[hMon];
  }
}

