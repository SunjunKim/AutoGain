/* -*- mode: c++ -*-
 *
 * pointing/utils/PointingCursor.h --
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

#ifndef PointingCursor_h
#define PointingCursor_h

namespace pointing {

  /**
   * @brief The PointingCursor class is used to set or get the position of the mouse cursor
   */
  class PointingCursor {

  protected:

    PointingCursor(void) {}

  public:
    /**
     * @brief These static functions can be used to set or get the position of mouse cursor
     * in global display coordinates.
     * @x The X-coordinate starting from the left
     * @y The Y-coordinate starting from the top
     */
    //@{
    static void setPosition(double x, double y);
    static void getPosition(double *x, double *y);
    //@}

  };
}

#endif
