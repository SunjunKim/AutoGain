/* -*- mode: c++ -*-
 *
 * pointing/input/linux/linuxPointingDeviceManager.cpp --
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

#include <pointing/input/linux/linuxPointingDeviceManager.h>

#include <pointing/input/linux/XInputHelper.h>
#include <pointing/utils/HIDReportParser.h>
#include <pointing/utils/URI.h>

#include <unistd.h>

#include <linux/input.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <iostream>
#include <stdexcept>

using namespace std;

namespace pointing {

#define EVENT_DEV "/dev/input/event"

  URI uriFromDevice(udev_device *hiddev)
  {
    const char *devnode = udev_device_get_devnode(hiddev);
    URI devUri;
    devUri.scheme = "input";
    devUri.path = devnode;
    return devUri;
  }

  static inline string sysattr2string(udev_device *dev, const char *key, const char *defval=0)
  {
    const char *value = udev_device_get_sysattr_value(dev, key);
    return value ? value : (defval ? defval : string());
  }

  static inline int sysattr2int(udev_device *dev, const char *key, int defval=0)
  {
    const char *value = udev_device_get_sysattr_value(dev, key);
    return value ? strtol(value, NULL, 16) : defval;
  }

  static inline void udevDebugDevice(udev_device *dev, std::ostream& out)
  {
     out << std::endl ;
     const char *s = udev_device_get_devpath(dev) ;
     out << "devpath: " << (s?s:"NULL") << std::endl ;
     s = udev_device_get_subsystem(dev) ;
     out << "  subsystem: " << (s?s:"NULL") << std::endl ;
     s = udev_device_get_devtype(dev) ;
     out << "  devtype: " << (s?s:"NULL") << std::endl ;
     s = udev_device_get_syspath(dev) ;
     out << "  syspath: " << (s?s:"NULL") << std::endl ;
     s = udev_device_get_sysname(dev) ;
     out << "  sysname: " << (s?s:"NULL") << std::endl ;
     s = udev_device_get_sysnum(dev) ;
     out << "  sysnum: " << (s?s:"NULL") << std::endl ;
     s = udev_device_get_devnode(dev) ;
     out << "  devnode: " << (s?s:"NULL") << std::endl ;
     s = udev_device_get_driver(dev) ;
     out << "  driver: " << (s?s:"NULL") << std::endl ;
     out << std::endl ;
  }

  bool checkDev(int devID)
  {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(devID, &rfds);

    int nbready = select(devID + 1, &rfds, NULL, NULL, 0);;
    pthread_testcancel();
    if (nbready == -1)
      perror("linuxPointingDevice::eventloop");
    return FD_ISSET(devID, &rfds);
  }

  udev_device* findEvDev(udev* udev, udev_device* dev)
  {
    // This function finds the associated event devnode
    // For a given "mouse" devnode
    dev = udev_device_get_parent(dev);
    if (!dev)
      return NULL;

    udev_enumerate *enumerate = udev_enumerate_new(udev);

    udev_enumerate_add_match_parent(enumerate, dev);
    udev_enumerate_scan_devices(enumerate);

    udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry *entry;

    udev_list_entry_foreach(entry, devices)
    {
      const char *path = udev_list_entry_get_name(entry);
      udev_device* child = udev_device_new_from_syspath(udev, path);
      const char *devnode = udev_device_get_devnode(child);
      if (devnode && strncmp(devnode, EVENT_DEV, strlen(EVENT_DEV)) == 0)
      {
        // Found corresponding event devnode
        // Need to unref it at the end
        return child;
      }
      udev_device_unref(child);
    }
    udev_enumerate_unref(enumerate);
    return NULL;
  }

  void *linuxPointingDeviceManager::eventloop(void *context)
  {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) ;
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) ;

    linuxPointingDeviceManager *self = (linuxPointingDeviceManager*)context ;

    udev_enumerate *enumerate = udev_enumerate_new(self->udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);

    udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry *dev_list_entry;
    udev_list_entry_foreach(dev_list_entry, devices) {
      const char *path = udev_list_entry_get_name(dev_list_entry);
      udev_device *dev = udev_device_new_from_syspath(self->udev, path);
      self->checkFoundDevice(dev);
      udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);

    udev_monitor_enable_receiving(self->monitor) ;
    int monfd = udev_monitor_get_fd(self->monitor) ;
    while (true)
    {
      if (checkDev(monfd))
        self->monitor_readable();
    }
    return 0 ;
  }

  void linuxPointingDeviceManager::cleanup_handler(void *arg)
  {
      printf("Called clean-up handler\n");
      linuxPointingDeviceData *pdd = static_cast<linuxPointingDeviceData *>(arg);
      linuxPointingDeviceManager *self = (linuxPointingDeviceManager *)PointingDeviceManager::get();
      if (pdd->fd > -1)
        close(pdd->fd);
      if (pdd->evDev)
        udev_device_unref(pdd->evDev) ;
      self->unregisterDevice(pdd->devnode);
  }

  void *linuxPointingDeviceManager::checkReports(void *context)
  {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) ;
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) ;

    linuxPointingDeviceData *pdd = (linuxPointingDeviceData *)context;
    linuxPointingDeviceManager *self = (linuxPointingDeviceManager *)PointingDeviceManager::get();
    pthread_cleanup_push(self->cleanup_handler, pdd);

    while (true)
    {
      if (checkDev(pdd->fd))
        self->readable(pdd);
    }
    pthread_cleanup_pop(0);
    return 0 ;
  }

  linuxPointingDeviceManager::linuxPointingDeviceManager()
  {
    // Unblock Synaptics Touchpad if there is one
    // Since it is usually grabbed by XServer
    enableSynapticsTouchpad();
    udev = udev_new();
    if (!udev)
      throw runtime_error("linuxPointingDeviceManager: udev_new failed");

    monitor = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(monitor, "input", NULL);

    int ret = pthread_create(&thread, NULL, eventloop, (void*)this);
    if (ret < 0)
    {
      perror("linuxPointingDeviceManager::linuxPointingDeviceManager") ;
      throw runtime_error("linuxPointingDeviceManager: pthread_create failed") ;
    }
  }

  void linuxPointingDeviceManager::monitor_readable(void)
  {
      udev_device *dev = udev_monitor_receive_device(monitor) ;
      if (dev)
      {
        const char *action = udev_device_get_action(dev) ;
        if (!strcmp(action,"add")) {
          checkFoundDevice(dev) ;
        } else if (!strcmp(action,"remove")) {
          checkLostDevice(dev) ;
        }
        udev_device_unref(dev) ;
      }
  }

  bool linuxPointingDeviceManager::outputsRelative(udev_device *dev)
  {
    udev_device *parent = udev_device_get_parent(dev);
    if (!parent)
      return false;
    if (sysattr2int(parent, "capabilities/abs"))
      return false;
    if (!sysattr2int(parent, "capabilities/rel"))
      return false;
    return true;
  }

  void linuxPointingDeviceManager::fillDevInfo(udev_device *hiddev, linuxPointingDeviceData *pdd)
  {
    udev_device *usbdev = udev_device_get_parent_with_subsystem_devtype(hiddev, "usb", "usb_device");
    // If there is no usbdev, most probably the embedded touchpad was found
    if (usbdev)
    {
      pdd->desc.vendor = sysattr2string(usbdev, "manufacturer", "???");
      pdd->desc.product = sysattr2string(usbdev, "product", "???");
      pdd->desc.vendorID = sysattr2int(usbdev, "idVendor");
      pdd->desc.productID = sysattr2int(usbdev, "idProduct");
    }
    else
    {
      udev_device *dev = udev_device_get_parent(hiddev);
      if (!dev)
        return;
      pdd->desc.vendorID = sysattr2int(dev, "id/vendor");
      pdd->desc.productID = sysattr2int(dev, "id/product");
      pdd->desc.product = sysattr2string(dev, "name", "???");
    }
    udev_device *evDev = findEvDev(udev, hiddev);
    if ((usbdev || outputsRelative(hiddev)) && evDev)
    {
      pdd->evDev = evDev;
      pdd->desc.devURI = uriFromDevice(pdd->evDev);
    }
    else
      pdd->desc.devURI = uriFromDevice(hiddev);
  }

  void linuxPointingDeviceManager::processMatching(PointingDeviceData *data, SystemPointingDevice *device)
  {
    linuxPointingDevice *dev = static_cast<linuxPointingDevice *>(device);
    linuxPointingDeviceData *pdd = static_cast<linuxPointingDeviceData *>(data);

    if (dev->seize && !pdd->seizeCount++)
    {
      if (ioctl(pdd->fd, EVIOCGRAB, 1) != 0)
        std::cerr << "linuxPointingDeviceManager::processMatching: could not seize the device" << std::endl;
    }
    udev_device *par = udev_device_get_parent_with_subsystem_devtype(pdd->evDev, "usb", "usb_interface");
    if (par)
    {
      int ms = sysattr2int(par, "ep_81/bInterval");
      if (ms)
        dev->hz = 1000.0 / ms;
    }
  }

  void linuxPointingDeviceManager::checkFoundDevice(udev_device *hiddev)
  {
    const char *name = udev_device_get_sysname(hiddev);
    if (!name)
      return;
    bool matchMouse = strncmp(name, "mouse", 5) == 0;
    bool matchMice = strcmp(name, "mice") == 0;
    if (!matchMouse && !matchMice)
      return;
    if (debugLevel > 1)
      udevDebugDevice(hiddev, std::cerr);
    linuxPointingDeviceData *pdd = new linuxPointingDeviceData;
    if (matchMouse)
      fillDevInfo(hiddev, pdd);
    else // matchMice
    {
      pdd->desc.devURI = uriFromDevice(hiddev);
      pdd->desc.product = "Mice";
      pdd->desc.vendor = "Virtual";
    }
    pdd->devnode = udev_device_get_devnode(pdd->evDev ? pdd->evDev : hiddev);

    pdd->fd = open(pdd->devnode.c_str(), O_RDONLY);
    // Non blocking
    int flags = fcntl(pdd->fd, F_GETFL, 0);
    fcntl(pdd->fd, F_SETFL, flags | O_NONBLOCK);
    if (pdd->fd == -1) {
      std::cerr << "linuxPointingDeviceManager::checkFoundDevice: unable to open " << pdd->devnode << std::endl;
      if (pdd->evDev)
          udev_device_unref(pdd->evDev);
      delete pdd;
      return;
    }

    registerDevice(pdd->devnode, pdd);

    int ret = pthread_create(&pdd->thread, NULL, checkReports, pdd);
    if (ret < 0)
    {
      perror("linuxPointingDeviceManager::checkFoundDevice");
      throw runtime_error("linuxPointingDeviceManager: pthread_create failed");
    }
  }

  void linuxPointingDeviceManager::checkLostDevice(udev_device *hiddev)
  {
    const char *devnode = udev_device_get_devnode(hiddev);
    if (!devnode)
      return;
    auto it = devMap.find(devnode);
    if (it != devMap.end())
    {
      linuxPointingDeviceData *pdd = static_cast<linuxPointingDeviceData *>(it->second);
      if (pthread_cancel(pdd->thread) != 0)
        perror("linuxPointingDeviceManager::checkLostDevice");
      unSeizeDevice(pdd);
    }
  }

  void linuxPointingDeviceManager::removePointingDevice(SystemPointingDevice *device)
  {
    linuxPointingDeviceData *pdd = static_cast<linuxPointingDeviceData *>(findDataForDevice(device));
    linuxPointingDevice *dev = static_cast<linuxPointingDevice *>(device);
    if (dev->seize)
      pdd->seizeCount--;
    if (pdd && !pdd->seizeCount)
    {
      unSeizeDevice(pdd);
    }
    PointingDeviceManager::removePointingDevice(device);
  }

  void linuxPointingDeviceManager::unSeizeDevice(linuxPointingDeviceData *pdd)
  {
    if (pdd->fd > 0)
    {
      ioctl(pdd->fd, EVIOCGRAB, 0);
      pdd->seizeCount = 0;
    }
  }

  void linuxPointingDeviceManager::readable(linuxPointingDeviceData *pdd)
  {
    TimeStamp::inttime now = TimeStamp::createAsInt();
    input_event ie;

    int dx = 0, dy = 0;

    int hasRead = 0;
    while (read(pdd->fd, &ie, sizeof(input_event)) > 0)
    {
      // Read event file descriptor
      if (pdd->evDev)
      {
        if (ie.type == EV_REL)
        {
          if (ie.code == REL_X)
            dx = ie.value;
          else if (ie.code == REL_Y)
            dy = ie.value;
        }
        else if (ie.type == EV_KEY)
        {
          if (ie.code == BTN_LEFT)
            pdd->buttons = ie.value ? (pdd->buttons | (1 << 0)) : (pdd->buttons & ~(1 << 0));
          else if (ie.code == BTN_RIGHT)
            pdd->buttons = ie.value ? (pdd->buttons | (1 << 1)) : (pdd->buttons & ~(1 << 1));
          else if (ie.code == BTN_MIDDLE)
            pdd->buttons = ie.value ? (pdd->buttons | (1 << 2)) : (pdd->buttons & ~(1 << 2));
        }
      }
      // Read mouse file descriptor
      else
      {
        unsigned char *ptr = (unsigned char*)&ie;
        pdd->buttons = ptr[0] & 7; // 3 bits only
        dx = (char)ptr[1];
        dy = (char)ptr[2];
      }
      hasRead++;
    }

    if (hasRead)
    {
      for (SystemPointingDevice *device : pdd->pointingList)
      {
        linuxPointingDevice *dev = static_cast<linuxPointingDevice *>(device);
        dev->registerTimestamp(now, dx, dy);
        if (dev->callback)
          dev->callback(dev->callback_context, now, dx, dy, pdd->buttons);
      }
    }
  }

  linuxPointingDeviceManager::~linuxPointingDeviceManager()
  {
    if (pthread_cancel(thread) < 0)
      perror("linuxPointingDeviceManager::~linuxPointingDeviceManager");
    // TODO cancel all threads
    udev_monitor_unref(monitor);
    udev_unref(udev);
  }
}
