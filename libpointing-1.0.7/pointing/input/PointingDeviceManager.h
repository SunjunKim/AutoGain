/* -*- mode: c++ -*-
 *
 * pointing/input/PointingDeviceManager.h --
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
#include <list>
#include <map>
#include <pointing/utils/URI.h>

#ifdef __APPLE__
#include <IOKit/hid/IOHIDManager.h>
#define identifier IOHIDDeviceRef
#endif

#ifdef __linux__
#define identifier std::string
#endif

#ifdef _WIN32
#include <windows.h>
#define identifier HANDLE
#endif

#ifndef POINTINGDEVICEMANAGER_H
#define POINTINGDEVICEMANAGER_H

namespace pointing
{
    class SystemPointingDevice;

    struct PointingDeviceDescriptor
    {
        URI devURI;

        int vendorID = 0;
        int productID = 0;

        std::string vendor = "???";
        std::string product = "???";

        // To use set of PointingDeviceDescriptors
        bool operator < (const PointingDeviceDescriptor& rhs) const;
    };

    typedef void (*DeviceUpdateCallback)(void *context, const PointingDeviceDescriptor &descriptor, bool wasAdded);
    typedef std::set<PointingDeviceDescriptor> PointingDescriptorSet;

    /**
     * @brief PointingDeviceIterator iterates over the list of pointers to the PointingDevices
     */
    //@{
    typedef PointingDescriptorSet::iterator PointingDescriptorIterator;
    typedef PointingDescriptorSet::const_iterator PointingDescriptorConstIterator;
    //@}

    struct CallbackInfo
    {
        DeviceUpdateCallback callbackFunc;
        void *context;
        CallbackInfo(DeviceUpdateCallback callbackFunc, void *context)
            :callbackFunc(callbackFunc),context(context) { }

        // To use the set of the CallbackInfos
        bool operator < (const CallbackInfo& rhs) const;
    };

    /**
     * @brief The PointingDeviceManager class is a helper class which enumerates
     * the list of existing pointing devices.
     * This class is a singleton which calls its platform-specific subclass
     * constructor.
     *
     * Provides functionality to handle newly added or removed devices.
     */
    class PointingDeviceManager
    {
      friend class SystemPointingDevice;

    private:
        PointingDescriptorSet descriptors;

        void addDescriptor(PointingDeviceDescriptor &descriptor);
        void removeDescriptor(PointingDeviceDescriptor &descriptor);

        std::set<CallbackInfo> callbackInfos;

        void callCallbackFunctions(PointingDeviceDescriptor &descriptor, bool wasAdded);

        static PointingDeviceManager *singleManager;

        void convertAnyCandidates();

        void matchCandidates();

    protected:
        typedef std::list<SystemPointingDevice *> PointingList;

        // This struct can be extended in subclasses to add
        // platform-specific data
        struct PointingDeviceData
        {
          PointingDeviceDescriptor desc;
          PointingList pointingList;
          virtual ~PointingDeviceData() {}
        };

        std::map<identifier, PointingDeviceData *> devMap;

        PointingList candidates;
        int debugLevel = 0;

        // Should be implemented by a subclass
        virtual void processMatching(PointingDeviceData *pdd, SystemPointingDevice *device)=0;

        void activateDevice(SystemPointingDevice *device, PointingDeviceData *pdd);

        /**
         * @brief Called from subclasses
         * @param key platform-specific unique identifier
         * @param pdd Pointer to the platform-specific data associated with the device
         */
        //@{
        void registerDevice(identifier key, PointingDeviceData *pdd);
        bool unregisterDevice(identifier);
        //@}

        void printDeviceInfo(PointingDeviceData *pdd, bool add);

        PointingDeviceData *findDataForDevice(SystemPointingDevice *device);

        /**
         * @brief Whenever there is a PointingDevice is created or deleted
         * those methods are called internally from a SystemPointingDevice
         */
        //@{
        virtual void addPointingDevice(SystemPointingDevice *device);
        virtual void removePointingDevice(SystemPointingDevice *device);
        //@}

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
        static PointingDeviceManager *get();

        /**
         * @brief anyToSpecific Converts a given URI into platform-specific unique URI
         * @param anyURI URI with any scheme
         * @return platform-specific URI
         */
        URI anyToSpecific(const URI &anyURI) const;

        /**
         * @brief generalizeAny Remove all arguments from the given any: URI
         * except for vendor vendor and product arguments
         * @param anyURI URI with any scheme
         * @return URI with only vendor and product query arguments
         */
        URI generalizeAny(const URI &anyURI) const;

        //static void destroy();

        /**
         * @brief size
         * @return The number of Pointing Devices
         */
        size_t size() const { return descriptors.size(); }

        /*
         * Delegate the iteration to the inner set of the descriptors
         */
        //@{
        PointingDescriptorIterator begin() { return descriptors.begin(); }
        PointingDescriptorIterator end() { return descriptors.end(); }
        //@}

        virtual ~PointingDeviceManager(void) {}
    };
}

#endif // POINTINGDEVICEMANAGER_H
