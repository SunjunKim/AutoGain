/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/linux/xorgSystemPointerAcceleration.h --
 *
 * Initial software
 * Authors: Nicolas Roussel
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#ifndef xorgSystemPointerAcceleration_h
#define xorgSystemPointerAcceleration_h

#include <X11/Xlib.h>

namespace pointing {

  /**
   * @brief The xorgSystemPointerAcceleration class is used to set or read the cursor parameters
   * of the current system.
   *
   * There are three main values which can be set or read.
   * Acceleration, given as a fraction with numerator and denominator parts,
   * represents the value for high CD gain, whereas the low gain is taken as 1, by default.
   *
   * Threshold, which represents minimum velocity at which the low gain is
   * switched to the high gain.
   */
  class xorgSystemPointerAcceleration {

  private:

    Display *dpy ;

  public:

    xorgSystemPointerAcceleration(const char *display=0) ;

    /**
     * @brief Reads the acceleration value (expressed as a fraction) and the threshold form the system
     * @param accel_numerator Pointer to the numerator part of the acceleration
     * @param accel_denominator Pointer to the denominator part of the acceleration
     * @param threshold The threshold at which low gain is switched to the high.
     */
    void get(int *accel_numerator, int *accel_denominator, int *threshold) ;

    /**
     * @brief Sets the acceleration value (expressed as a fraction) and the threshold form the system
     * @param accel_numerator Pointer to the numerator part of the acceleration
     * @param accel_denominator Pointer to the denominator part of the acceleration
     * @param threshold The threshold at which low gain is switched to the high.
     */
    void set(int accel_numerator, int accel_denominator, int threshold) ;

    ~xorgSystemPointerAcceleration() ;

  } ;

}

#endif
