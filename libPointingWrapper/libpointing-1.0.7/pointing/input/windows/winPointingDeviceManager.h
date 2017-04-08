/* -*- mode: c++ -*-
 *
 * pointing/input/windows/winPointingDeviceManager.h --
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

#ifndef WINPOINTINGDEVICEMANAGER_H
#define WINPOINTINGDEVICEMANAGER_H

#include <pointing/input/PointingDeviceManager.h>
#include <windows.h>
#include <pointing/input/windows/winPointingDevice.h>
#include <vector>

namespace pointing
{
    class winPointingDeviceManager : public PointingDeviceManager
    {
        friend class PointingDeviceManager;
        friend class winPointingDevice;

        typedef enum
        {
            THREAD_UNDEFINED=0,
            THREAD_RUNNING,
            THREAD_TERMINATING,
            THREAD_HALTED
        } ThreadState;

        ThreadState run = THREAD_UNDEFINED; // for the Loop thread
        HANDLE hThreads[1];
        DWORD dwThreadId;
        static DWORD WINAPI Loop(LPVOID lpvThreadParam);
        void processMessage(MSG *msg);
        HWND msghwnd_;

        static LONG APIENTRY rawInputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        HWND rawInputInit();

        bool fillDescriptorInfo(HANDLE h, PointingDeviceDescriptor &desc);

        // True if the mouse has moved
        bool relativeDisplacement(const PRAWMOUSE pmouse, winPointingDevice *dev, int *dx, int *dy);

        void processMatching(PointingDeviceData *, SystemPointingDevice *);

        winPointingDeviceManager();
        ~winPointingDeviceManager();
    };
}

#endif // WINPOINTINGDEVICEMANAGER_H
