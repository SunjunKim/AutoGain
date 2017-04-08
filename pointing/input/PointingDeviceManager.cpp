/* -*- mode: c++ -*-
 *
 * pointing/input/PointingDeviceManager.cpp --
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
#include <pointing/input/osx/osxPointingDeviceManager.h>
#endif

#ifdef _WIN32
#include <pointing/input/windows/winPointingDeviceManager.h>
#endif

#ifdef __linux__
#include <pointing/input/linux/linuxPointingDeviceManager.h>
#endif

#include <stdexcept>
#include <iostream>
#include <pointing/input/PointingDeviceManager.h>
#include <pointing/input/SystemPointingDevice.h>

namespace pointing
{
#define MAX(X, Y)           (((X) > (Y)) ? (X) : (Y))

    PointingDeviceManager *PointingDeviceManager::singleManager = 0;

    PointingDeviceManager *PointingDeviceManager::get()
    {
        if (singleManager == NULL)
        {
#ifdef __APPLE__
            singleManager = new osxPointingDeviceManager();
#endif

#ifdef _WIN32
            singleManager = new winPointingDeviceManager();
#endif

#ifdef __linux__
            singleManager = new linuxPointingDeviceManager();
#endif
        }
        return singleManager;
    }

    bool PointingDeviceDescriptor::operator < (const PointingDeviceDescriptor& rhs) const
    {
        return devURI.asString() < rhs.devURI.asString();
    }

    // To use the set of the CallbackInfos
    bool CallbackInfo::operator < (const CallbackInfo& rhs) const
    {
        if (context < rhs.context) return true;
        if (context > rhs.context) return false;

        return callbackFunc < rhs.callbackFunc;
    }

    void PointingDeviceManager::callCallbackFunctions(PointingDeviceDescriptor &descriptor, bool wasAdded)
    {
        for (const CallbackInfo &callbackInfo : callbackInfos)
        {
            callbackInfo.callbackFunc(callbackInfo.context, descriptor, wasAdded);
        }
    }

    PointingDeviceManager::PointingDeviceData *PointingDeviceManager::findDataForDevice(SystemPointingDevice *device)
    {
      URI uri = device->uri;
      for(auto &kv : devMap)
      {
          PointingDeviceData *pdd = kv.second;
          if (pdd->desc.devURI == uri)
            return pdd;
      }
      return NULL;
    }

    void PointingDeviceManager::addDescriptor(PointingDeviceDescriptor &descriptor)
    {
        descriptors.insert(descriptor);
        callCallbackFunctions(descriptor, true);
    }

    void PointingDeviceManager::removeDescriptor(PointingDeviceDescriptor &descriptor)
    {
        PointingDescriptorIterator it = descriptors.find(descriptor);
        if (it != descriptors.end())
        {
            PointingDeviceDescriptor foundDesc = *it;
            descriptors.erase(it);
            callCallbackFunctions(foundDesc, false);
        }
    }

    URI PointingDeviceManager::anyToSpecific(const URI &anyURI) const
    {
        if (anyURI.scheme != "any")
        {
            std::cerr << "PointingDeviceManager::anyToSpecific: URI scheme must be \"any\"" << std::endl;
            return anyURI;
        }
        int vendorID = -1;
        int productID = -1;
        URI::getQueryArg(anyURI.query, "vendor", &vendorID);
        URI::getQueryArg(anyURI.query, "product", &productID);

        for (const PointingDeviceDescriptor &pdd : descriptors)
        {
            if ((vendorID == -1 || pdd.vendorID == vendorID)
            && (productID == -1 || pdd.productID == productID))
            {
                return pdd.devURI;
            }
        }
        //std::cerr << "Warning: could not find a device with a given URI" << std::endl ;
        return anyURI;
    }

    URI PointingDeviceManager::generalizeAny(const URI &anyURI) const
    {
        URI result = anyURI;
        int vendorID = -1, productID = -1;
        URI::getQueryArg(anyURI.query, "vendor", &vendorID);
        URI::getQueryArg(anyURI.query, "product", &productID);
        result.generalize();
        if (vendorID != -1)
            URI::addQueryArg(result.query, "vendor", vendorID);
        if (productID != -1)
            URI::addQueryArg(result.query, "product", productID);
        return result;
    }

    /*void PointingDeviceManager::destroy()
  {
      delete singleManager;
      singleManager = NULL;
  }
  */

    void PointingDeviceManager::addDeviceUpdateCallback(DeviceUpdateCallback callback, void *context)
    {
        CallbackInfo info(callback, context);
        callbackInfos.insert(info);
    }

    void PointingDeviceManager::removeDeviceUpdateCallback(DeviceUpdateCallback callback, void *context)
    {
        CallbackInfo info(callback, context);
        callbackInfos.erase(info);
    }

    void PointingDeviceManager::convertAnyCandidates()
    {
        for (SystemPointingDevice *device : candidates)
        {
            if (!device->anyURI.asString().empty())
                device->uri = anyToSpecific(device->anyURI);
        }
    }

    void PointingDeviceManager::matchCandidates()
    {
        convertAnyCandidates();
        for(auto &kv : devMap)
        {
            PointingDeviceData *pdd = kv.second;

            PointingList::iterator i = candidates.begin();
            while (i != candidates.end())
            {
                SystemPointingDevice *device = *i;

                // Found matching device
                // Move it from candidates to devMap
                if (pdd->desc.devURI == device->uri)
                {
                    candidates.erase(i++);
                    activateDevice(device, pdd);
                    processMatching(pdd, device);
                }
                else
                    i++;
            }
        }
    }

    void PointingDeviceManager::activateDevice(SystemPointingDevice *device, PointingDeviceData *pdd)
    {
        pdd->pointingList.push_back(device);
        device->active = true;
        device->productID = pdd->desc.productID;
        device->vendorID = pdd->desc.vendorID;
        device->vendor = pdd->desc.vendor;
        device->product = pdd->desc.product;
    }

    void PointingDeviceManager::printDeviceInfo(PointingDeviceData *pdd, bool add)
    {
        bool match = pdd->pointingList.size();
        std::cerr << (add ? (match ? "+ " : "  ") : "- ") << pdd->desc.devURI
                  << " [" << std::hex << "vend:0x" << pdd->desc.vendorID
                  << ", prod:0x" << pdd->desc.productID
                  << std::dec << " - " << pdd->desc.vendor
                  << " " << pdd->desc.product << "]" << std::endl;
        /*
    if (match)
      std::cerr << ", " << pdd->pointingList.front()->getResolution() << " CPI"
                << ", " << pdd->pointingList.front()->getUpdateFrequency() << " Hz";
    */
    }

    void PointingDeviceManager::registerDevice(identifier key, PointingDeviceData *pdd)
    {
        devMap[key] = pdd;
        addDescriptor(pdd->desc);
        matchCandidates();

        if (debugLevel > 0) printDeviceInfo(pdd, true);
    }

    bool PointingDeviceManager::unregisterDevice(identifier key)
    {
        auto it = devMap.find(key);
        if (it != devMap.end())
        {
            PointingDeviceData *pdd = it->second;
            removeDescriptor(pdd->desc);
            for (SystemPointingDevice *device : pdd->pointingList)
            {
                device->active = false;
                candidates.push_back(device);
            }
            if (debugLevel > 0) printDeviceInfo(pdd, false);
            delete pdd;
            devMap.erase(it);
            matchCandidates();
            return true;
        }
        return false;
    }

    void PointingDeviceManager::addPointingDevice(SystemPointingDevice *device)
    {
        candidates.push_back(device);
        matchCandidates();
        if (!debugLevel && device->debugLevel)
        {
            // This happens once at the beginning
            // To print devices which are already registered
            for (auto &kv : devMap)
                printDeviceInfo(kv.second, true);
        }
        debugLevel = MAX(debugLevel, device->debugLevel);
    }

    void PointingDeviceManager::removePointingDevice(SystemPointingDevice *device)
    {
      PointingDeviceData *pdd = findDataForDevice(device);
      if (pdd)
        pdd->pointingList.remove(device);
      candidates.remove(device);
    }
}

