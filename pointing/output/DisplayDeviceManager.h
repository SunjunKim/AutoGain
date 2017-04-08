/* -*- mode: c++ -*-
 *
 * pointing/output/DisplayDeviceManager.h --
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

#include <set>
#include <string>

#ifndef DISPLAYDEVICEMANAGER_H
#define DISPLAYDEVICEMANAGER_H

namespace pointing
{
    struct DisplayDeviceDescriptor
    {
        std::string devURI;
        std::string name;

        int width; // In pixels
        int height;

        DisplayDeviceDescriptor(std::string devURI = "", std::string name = "")
            :devURI(devURI),name(name),width(-1),height(-1)
        { }

        bool operator < (const DisplayDeviceDescriptor& rhs) const;
    };

    typedef std::set<DisplayDeviceDescriptor> DisplayDescriptorSet;
    typedef DisplayDescriptorSet::iterator DisplayDescriptorIterator;
    typedef DisplayDescriptorSet::const_iterator DisplayDescriptorConstIterator;

    class DisplayDeviceManager
    {
        typedef void (*DeviceUpdateCallback)(void *context, const DisplayDeviceDescriptor &descriptor, bool wasAdded);

        struct CallbackInfo
        {
            DeviceUpdateCallback callbackFunc;
            void *context;
            CallbackInfo(DeviceUpdateCallback callbackFunc, void *context)
                :callbackFunc(callbackFunc),context(context) { }

            bool operator < (const CallbackInfo& rhs) const
            {
                if (context < rhs.context) return true;
                if (context > rhs.context) return false;

                return callbackFunc < rhs.callbackFunc;
            }
        };

    protected:
        DisplayDeviceManager():callback(NULL) {}
        DeviceUpdateCallback callback;

        virtual ~DisplayDeviceManager(void) {}
        static DisplayDeviceManager *singleManager;

        DisplayDescriptorSet descriptors;

        std::set<CallbackInfo> callbackInfos;
        typedef std::set<CallbackInfo>::iterator CallbackInfoIterator;

        void callCallbackFunctions(DisplayDeviceDescriptor &descriptor, bool wasAdded);

    public:

        /**
         * @brief Adds the callback function which is called when
         * a device was added or removed
         */
        void addDeviceUpdateCallback(DeviceUpdateCallback callback, void *context);

        /**
         * @brief Removes the callback function which is called when
         * a device was added or removed
         */
        void removeDeviceUpdateCallback(DeviceUpdateCallback callback, void *context);

        /**
         * @brief This static function is used to instantiate a platform-specific object
         * of the class or return the already existing one.
         */
        static DisplayDeviceManager *get();

        /**
         * @brief size
         * @return The number of Display Devices
         */
        size_t size() const { return descriptors.size(); }

        //static void destroy();

        /*
         * Delegate the iteration to the inner set of the descriptors
         */
        //@{
        DisplayDescriptorIterator begin() { return descriptors.begin(); }
        DisplayDescriptorIterator end() { return descriptors.end(); }
        //@}

    protected:
        void addDevice(DisplayDeviceDescriptor &descriptor);
        void removeDevice(DisplayDeviceDescriptor &descriptor);
    };
}

#endif // DISPLAYDEVICEMANAGER_H
