/* -*- mode: c++ -*-
 *
 * pointing/output/osx/osxDisplayDeviceManager.h --
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

#ifndef osxDisplayDeviceManager_h
#define osxDisplayDeviceManager_h

#include <pointing/output/DisplayDevice.h>
#include <pointing/output/DisplayDeviceManager.h>

#include <ApplicationServices/ApplicationServices.h>
//#include <map>

namespace pointing
{
  /**
   * @brief The osxDisplayDeviceManager class is the helper class to work with
   * all display devices.
   */
  class osxDisplayDeviceManager : public DisplayDeviceManager
  {
    static void MyDisplayReconfigurationCallBack(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags, void *userInfo);

    void addDisplay(CGDirectDisplayID did);
    void removeDisplay(CGDirectDisplayID did);

    DisplayDeviceDescriptor convertDevice(CGDirectDisplayID did);

    osxDisplayDeviceManager() ;

    friend class DisplayDeviceManager;
  };
}

#endif
