/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/TransferFunction.h --
 *
 * Initial software
 * Authors: Géry Casiez, Nicolas Roussel
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#ifndef TransferFunction_h
#define TransferFunction_h

#include <pointing/utils/URI.h>
#include <pointing/utils/TimeStamp.h>
#include <pointing/input/PointingDevice.h>
#include <pointing/output/DisplayDevice.h>

#include <string>
#include <iostream>
#include <list>

namespace pointing {

  /**
   * @brief The TransferFunction class is an abstract class that creates an object
   * of its concrete subclasses.
   *
   * Subclasses may caracterize either platform specific
   * transfer functions or transfer functions conforming to a certain law.
   * It uses URIs to specify the object type.
   */
  class TransferFunction {

  protected:

    TransferFunction(void) {}

    /**
     * @brief normalizeInput Normalizes input to a transfer function with respect to the given PointingDevice.
     * For example, to prevent fast movements of the cursor for high resolution mice.
     * dx and dy are changed according to the input->getResolution()
     * @param dx Input x-displacement
     * @param dy Input y-displacement
     * @param input Given PointingDevice
     */
    void normalizeInput(int *dx, int *dy, PointingDevice *input) const;

    /**
     * @brief normalizeOutput Normalizes output of a transfer function with respect to the given DisplayDevice.
     * For example, to prevent slow movements of the cursor for high resolution display.
     * dx and dy are changed according to the output->getResolution()
     * @param dx x-pixel-displacement
     * @param dy y-pixel-displacement
     * @param output Given DisplayDevice
     */
    void normalizeOutput(int *dx, int *dy, DisplayDevice *output) const;

  public:

    static std::list<std::string> schemes(void) ;
    
    /**
     * @brief Static method to instantiate an object of a sub-class.
     * @param function_uri URI defining the type and details of an object
     * @param input Input device associated with this transfer function.
     * @param output Output device associated with this transfer function
     * @return Pointer to the initialized transfer function.
     */
    //@{
    static TransferFunction* create(const char* function_uri,
				    PointingDevice* input, DisplayDevice* output) ;

    static TransferFunction* create(std::string function_uri,
				    PointingDevice* input, DisplayDevice* output) ;

    static TransferFunction* create(URI &function_uri,
				    PointingDevice* input, DisplayDevice* output) ;
    //@}

    /**
     * @brief Method which clears the current state of the device
     * to be the default one (without any remainders or previous data).
     */
    virtual void clearState(void) = 0 ;

    /**
     * @brief apply The main method of the class which applies the transfer function.
     * @param dxMickey Translation in dots of the input device along x direction.
     * @param dyMickey Translation in dots of the input device along y direction.
     * @param dxPixel (Integer) Computed translation in pixels of the output device along x direction.
     * @param dyPixel (Integer) Computed translation in pixels of the output device along y direction.
     * @param timestamp
     */
    virtual void applyi(int dxMickey, int dyMickey, int *dxPixel, int *dyPixel,
               TimeStamp::inttime timestamp=TimeStamp::undef) = 0 ;

    /**
     * @brief apply The main method of the class which applies the transfer function.
     * @param dxMickey Translation in dots of the input device along x direction.
     * @param dyMickey Translation in dots of the input device along y direction.
     * @param dxPixel (Double) Computed translation in pixels of the output device along x direction.
     * @param dyPixel (Double) Computed translation in pixels of the output device along y direction.
     * @param timestamp
     */
    virtual void applyd(int dxMickey, int dyMickey, double *dxPixel, double *dyPixel,
               TimeStamp::inttime timestamp=TimeStamp::undef) = 0;

    /**
     * @brief getURI The method constructs URI corresponding to the
     * type and parameters of the transfer function.
     * @param expanded Indicates whether all parameters must be included in URI.
     * @return URI result.
     */
    virtual URI getURI(bool expanded=false) const = 0 ;

    /**
     * @brief Sets the level of information for debugging purposes (default = 0).
     */
    virtual void setDebugLevel(int /*level*/) {}
    virtual void debug(std::ostream& /*out*/) const {}
   
    virtual ~TransferFunction() {}

  } ;

}

#endif
