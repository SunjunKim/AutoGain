/* -*- mode: c++ -*-
 *
 * pointing/input/windows/USB.cpp --
 *
 * Initial software
 * Authors: Damien Marchal, Izzat Mukhanov
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#ifndef USB_H
#define USB_H

#include <string>
#include <windows.h>
#include <iostream>
#include <stdexcept>

// Dirty HACK. These defines are missing on my QT- installation... is this the case
// for other guy ? Maybe FIXME.
#ifndef WM_INPUT_DEVICE_CHANGE
#define WM_INPUT_DEVICE_CHANGE 0x00FE
#define GIDC_ARRIVAL 1
#define GIDC_REMOVAL 2
#define RIDEV_DEVNOTIFY 0x00002000
#define RIDEV_EXINPUTSINK 0x00001000
#endif //

namespace pointing {

  /**
   * @brief usbIDToString
   * @param vid Input vendorID
   * @param pid Input productID
   * @param vendor Output vendor name
   * @param product Output product name
   */
  void FindHIDDevice(unsigned int vid, unsigned int pid, std::string &vendor, std::string &product);

  /**
   * @brief getMouseNameFromDevice
   * @param h HANDLE unique pointer to the device
   * @param vendor Output parameter vendor name
   * @param product Output parameter product name
   * @param vendorID Output parameter vendor identifier
   * @param productID Output parameter product identifier
   * @return true if the name was found successfully
   */
  bool getMouseNameFromDevice(HANDLE h, std::string &vendor, std::string &product, int *vendorID=0, int *productID=0);

}

#endif
