/* -*- mode: c++ -*-
 *
 * pointing/output/linux/xorgDisplayDeviceManager.h --
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

#ifndef xorgDisplayDeviceManager_h
#define xorgDisplayDeviceManager_h

#include <pointing/output/DisplayDeviceManager.h>

namespace pointing {

  class xorgDisplayDeviceManager : public DisplayDeviceManager {
    xorgDisplayDeviceManager();

    pthread_t thread;
    static void *eventloop(void *self);

    friend class DisplayDeviceManager;
  };
}

#endif // xorgDisplayDeviceManager
