/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/windows/winSystemPointerAcceleration.h --
 *
 * Initial software
 * Authors: Géry Casiez
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#ifndef winSystemPointerAcceleration_h
#define winSystemPointerAcceleration_h

#include <windows.h>
#include <iostream>
#include <pointing/utils/URI.h>

namespace pointing {

  /**
   * @brief The winSystemPointerAcceleration class is used to set or read the cursor parameters
   * of the current system.
   */
  class winSystemPointerAcceleration {

  public:

    winSystemPointerAcceleration() ;

    /**
     * @brief setTransferFunction
     * @param funcURI The transfer function URI
     */
    void setTransferFunction(URI &funcURI);

    /**
     * @brief Reads the current system parameters
     * @param winVersion The version of the running windows.
     * @param sliderPosition The cursor speed slider position
     * @param enhancePointerPrecision The "Enhance Pointer Precision" checkbox value
     */
    void get(std::string *winVersion, int *sliderPosition, bool *enhancePointerPrecision) ;

    /**
     * @brief Sets the system values with a given ones
     * @param sliderPosition The cursor speed slider position
     * @param enhancePointerPrecision The "Enhance Pointer Precision" checkbox value
     */
    void set(int sliderPosition, bool enhancePointerPrecision) ;

    ~winSystemPointerAcceleration() ;

  } ;

}

#endif
