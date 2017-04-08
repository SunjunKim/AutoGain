/* -*- mode: c++ -*-
 *
 * pointing/input/windows/winPointingDevice.h --
 *
 * Initial software
 * Authors: Damien Marchal, Izzat Mukhanov
 * Copyright © INRIA
 *
 */

#ifndef winPointingDevice_h
#define winPointingDevice_h

#include <pointing/input/SystemPointingDevice.h>

namespace pointing
{
    class winPointingDevice : public SystemPointingDevice
    {
        friend class winPointingDeviceManager;

        int buttons = 0;

        // For absolute coordinates
        int lastX = -1, lastY = -1;

    public:
        winPointingDevice(URI uri);

        void getAbsolutePosition(double *x, double *y) const override;
    } ;
}

#endif
