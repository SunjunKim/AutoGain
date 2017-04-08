/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/osx/osxSystemPointerAcceleration.h --
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

#ifndef osxSystemPointerAcceleration_h
#define osxSystemPointerAcceleration_h

#include <IOKit/IOKitLib.h>

namespace pointing {

  /**
   * @brief The osxSystemPointerAcceleration class is used to set or read the cursor parameters
   * of the current system.
   */
  class osxSystemPointerAcceleration {

  private:

    io_object_t connect ;

  public:

    osxSystemPointerAcceleration(void) ;

    /**
     * @brief get
     * @param target parameter can be specified as "mouse",
     * "trackpad" or "touchpad" to get the corresponding parameter
     * @return
     */
    double get(const char *target=0) const ;
    void set(double acceleration, const char *target=0) ;

    ~osxSystemPointerAcceleration() ;

  } ;

}

#endif
