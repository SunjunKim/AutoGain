/* -*- mode: c++ -*-
 *
 * pointing/output/DisplayDeviceManager.cpp --
 *
 * Initial software
 * Authors: Izzat Mukhanov
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#ifdef __APPLE__
#include <pointing/output/osx/osxDisplayDeviceManager.h>
#endif

#ifdef _WIN32
#include <pointing/output/windows/winDisplayDeviceManager.h>
#endif

#ifdef __linux__
#include <pointing/output/linux/xorgDisplayDeviceManager.h>
#endif

#include <stdexcept>
#include <iostream>
#include <pointing/output/DisplayDeviceManager.h>

namespace pointing {

    bool DisplayDeviceDescriptor::operator < (const DisplayDeviceDescriptor& rhs) const
    {
      if (devURI < rhs.devURI) return true;
      if (devURI > rhs.devURI) return false;
      if (width < rhs.width) return true;
      if (width > rhs.width) return false;
      return height < rhs.height;
    }

    DisplayDeviceManager *DisplayDeviceManager::singleManager = 0;

    DisplayDeviceManager *DisplayDeviceManager::get()
    {
        if (singleManager == NULL)
        {
        #ifdef __APPLE__
            singleManager = new osxDisplayDeviceManager();
            //singleManager = new PointingDeviceManager();
        #endif

        #ifdef _WIN32
            singleManager = new winDisplayDeviceManager();
        #endif

        #ifdef __linux__
            singleManager = new DisplayDeviceManager();
        #endif
        }
        return singleManager;
    }

    void DisplayDeviceManager::callCallbackFunctions(DisplayDeviceDescriptor &descriptor, bool wasAdded)
    {
        for (CallbackInfoIterator it = callbackInfos.begin(); it != callbackInfos.end(); it++)
        {
            CallbackInfo callbackInfo = *it;
            callbackInfo.callbackFunc(callbackInfo.context, descriptor, wasAdded);
        }
    }

    void DisplayDeviceManager::addDevice(DisplayDeviceDescriptor &descriptor)
    {
        std::pair<std::set<DisplayDeviceDescriptor>::iterator,bool> ret;
        ret = descriptors.insert(descriptor);
        if (ret.second) // If this device was not already there
            callCallbackFunctions(descriptor, true);
    }

    void DisplayDeviceManager::removeDevice(DisplayDeviceDescriptor &descriptor)
    {
        DisplayDescriptorIterator it = descriptors.find(descriptor);
        if (it != descriptors.end())
        {
            DisplayDeviceDescriptor foundDesc = *it;
            descriptors.erase(it);
            callCallbackFunctions(foundDesc, false);
        }
    }

    void DisplayDeviceManager::addDeviceUpdateCallback(DeviceUpdateCallback callback, void *context)
    {
        CallbackInfo info(callback, context);
        callbackInfos.insert(info);
    }

    void DisplayDeviceManager::removeDeviceUpdateCallback(DeviceUpdateCallback callback, void *context)
    {
        CallbackInfo info(callback, context);
        callbackInfos.erase(info);
    }
}

