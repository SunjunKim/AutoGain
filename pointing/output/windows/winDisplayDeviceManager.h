/* -*- mode: c++ -*-
 *
 * pointing/output/windows/winDisplayDeviceManager.h --
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

#ifndef WINDISPLAYDEVICEMANAGER_H
#define WINDISPLAYDEVICEMANAGER_H

#include <windows.h>
#include <pointing/output/DisplayDeviceManager.h>
#include <map>

namespace pointing
{
  class winDisplayDeviceManager : public DisplayDeviceManager
  {
    // Called only from DisplayDeviceManager
    winDisplayDeviceManager();

    friend class DisplayDeviceManager;

    typedef std::map<HMONITOR, DisplayDeviceDescriptor> descMap_t;

    descMap_t descMap;

    HANDLE loopThread;
    DWORD dwThreadId;
    HWND msgWndw;
    static DWORD WINAPI Loop(LPVOID lpvThreadParam);
    static LONG APIENTRY wndwProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    static BOOL CALLBACK EnumDispProc(HMONITOR hMon, HDC dcMon, RECT* pRcMon, LPARAM lParam);
    void scanDisplays();

    bool ConvertDevice(HMONITOR hMon, DisplayDeviceDescriptor &desc);

  public:
    /**
     * @brief readAllFromRegistry
     * @return All displays which were once registered
     */
    DisplayDescriptorSet readAllFromRegistry();

    /**
     * @brief Finds the display containing a given point.
     * If the point is not contained within any display monitor, returns the primary monitor descriptor
     * @param x The X-coordinate of the point
     * @param y The Y-coordinate of the point
     * @return DisplayDeviceDescriptor
     */
    DisplayDeviceDescriptor uriFromPoint(int x, int y);
  };
}

#endif // WINDISPLAYDEVICEMANAGER_H
