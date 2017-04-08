/* -*- mode: c++ -*-
 *
 * pointing/input/linux/linuxPointingDeviceManager.h --
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

#ifndef linuxPointingDeviceManager_h
#define linuxPointingDeviceManager_h

#include <pointing/input/PointingDeviceManager.h>
#include <pointing/input/linux/linuxPointingDevice.h>
#include <pthread.h>
#include <libudev.h>

namespace pointing {

  class HIDReportParser;

  /**
   * @brief The linuxPointingDeviceManager class is the platform-specific
   * subclass of the PointingDeviceManager class
   */
  class linuxPointingDeviceManager : public PointingDeviceManager
  {
    friend class PointingDeviceManager;
    friend class linuxPointingDevice;

    // Add linux-specific data
    struct linuxPointingDeviceData : PointingDeviceData
    {
      int fd = -1;
      pthread_t thread;
      udev_device *evDev = NULL;
      // If there are several PointingDevice objects with seize
      // corresponding to the same physical device
      // Seize the device until all of them are deleted
      int seizeCount = 0;
      int buttons = 0;
      std::string devnode;
    };

    struct udev *udev;
    struct udev_monitor *monitor;

    pthread_t thread;

    static void cleanup_handler(void *arg);

    /**
     * @brief This static function works in another thread.
     * It queries for added or removed devices.
     */
    static void *eventloop(void *self);
    static void *checkReports(void *self);

    void enableDevice(bool value, std::string fullName);

    void monitor_readable();
    void readable(linuxPointingDeviceData *pdd);

    bool outputsRelative(udev_device *dev);

    int readHIDDescriptor(int devID, HIDReportParser *parser);
    void fillDevInfo(udev_device *hiddev, linuxPointingDeviceData *pdd);

    void processMatching(PointingDeviceData *pdd, SystemPointingDevice *device);

    void checkFoundDevice(udev_device *device);
    void checkLostDevice(udev_device *device);

    void unSeizeDevice(linuxPointingDeviceData *data);
    virtual void removePointingDevice(SystemPointingDevice *device) override;

    linuxPointingDeviceManager();
    ~linuxPointingDeviceManager();
  };
}

#endif
