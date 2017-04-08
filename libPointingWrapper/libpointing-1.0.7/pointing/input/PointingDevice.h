/* -*- mode: c++ -*-
 *
 * pointing/input/PointingDevice.h --
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

#ifndef PointingDevice_h
#define PointingDevice_h

#include <pointing/utils/URI.h>
#include <pointing/utils/TimeStamp.h>
#include <pointing/utils/FrequencyEstimator.h>

#include <string>
#include <iostream>

#include <stdint.h>
#include <cstring>

namespace pointing {

  /**
   * @brief The PointingDevice class is used to represent Pointing Devices connected to the computer or pseudo-devices.
   *
   * It is an abstract class which creates an instance using create factory method.
   */
  class PointingDevice {

  protected:

    PointingDevice(void) ;

    FrequencyEstimator freqEstim;

    /**
     * @brief registerTimestamp Registers the current timestamp to calculate frequency of the device
     */
    void registerTimestamp(TimeStamp::inttime timestamp, int dx, int dy);

    /**
     * @brief estimatedUpdateFrequency Estimates the frequency depending on the input timestamps
     * @return 125, 250, 500, 1000 or -1
     */
    double estimatedUpdateFrequency() const;

  public:

    //double estimatedUpdateFrequency() const;

    /**
     * The enumeration of mouse buttons
     */
    enum {BUTTON_1=1, /**< Left mouse button    */
          BUTTON_2=2, /**< Middle mouse button  */
          BUTTON_3=4  /**< Right mouse button   */ } ;

    /**
     * This signature is used when a callback function is created
     * for the Pointing Device. The callback function is called
     * whenever the device is moved or its buttons are pressed.
     * @param dx Horizontal translation of the device
     * @param dy Vertical tranlation of the device
     * @param buttons The state of the buttons (from 0 to 7)
     */
    typedef void (*PointingCallback)(void *context, 
				     TimeStamp::inttime timestamp, 
				     int dx, int dy, 
				     int buttons) ;

    /**
     * @brief This static function is used to instantiate a platform-specific object
     * of the class.
     * @param device_uri is used to initialize the Device with given parameters.
     * If not given, any supported device is chosen.
     * @return PointingDevice instance with specified parameters.
     */
    //@{
    static PointingDevice *create(const char *device_uri=0) ;
    static PointingDevice *create(std::string device_uri) ;
    //@}

    /**
     * @brief The function is used to sleep the current thread
     * @param milliseconds Sleep time of the main thread in milliseconds
     */
    static void idle(int milliseconds) ;

    /**
     * @brief Indicates whether the current device is active
     * @return true if active
     */
    virtual bool isActive(void) const { return true ; }

    /**
     * @brief Vendor identifier number
     * @return integer number associated with vendor of the device
     */
    virtual int getVendorID(void) const { return 0 ; }

    /**
     * @brief Readable vendor name
     * @return Vendor name
     */
    virtual std::string getVendor(void) const { return "???" ; }

    /**
     * @brief Product identifier number
     * @return Product number associated with the device
     */
    virtual int getProductID(void) const { return 0 ; }

    /**
     * @brief Readable product name
     * @return Product name
     */
    virtual std::string getProduct(void) const { return "???" ; }

    /**
     * @brief Resolution of the mouse in CPI (counts per inch).
     * @param defval is input variable to define explicitly the resolution.
     * @return Resolution in CPI (if not available 400 cpi).
     */
    virtual double getResolution(double *defval=0) const = 0 ;

    /**
     * @brief Update frequency of the mouse in Hz.
     * @param defval is input variable to define explicitly the update frequency.
     * @return Update frequency in Hz (if not available 125 Hz).
     */
    virtual double getUpdateFrequency(double *defval=0) const = 0 ;

    /**
     * @brief Constructs the URI according to the parameters of the device
     * @param expanded specifies if additional parameters must be displayed.
     * @param crossplatform defines whether the URI should be crossplatform.
     * if True vendorId and productId are included in the URI
     * @return The corresponding URI of the device.
     */
    virtual URI getURI(bool expanded=false, bool crossplatform=false) const = 0 ;

    /**
     * Not implemented in the current version
     */
    //@{
    int mm2counts(double millimeters) const ;
    double counts2mm(int counts) const ;
    int in2counts(double inches) const ;
    double counts2in(int counts) const ;
    //@}

    /**
     * @brief Sets the callback function which is called when device events occur.
     * @param callback The pointer to the callback function.
     * @param context The pointer to anything that can be used in the callback function.
     */
    virtual void setPointingCallback(PointingCallback callback, void *context=0) = 0 ;

    /**
     * @brief Sets the level of information for debugging purposes (default = 0).
     */
    virtual void setDebugLevel(int /*level*/) {}

    /**
     * @brief Outputs the debug information to the given output stream.
     */
    virtual void debug(std::ostream& /*out*/) const {}

    /**
     * @brief getAbsolutePosition Returns absolute position of the device if available otherwise -1, -1
     * @param x
     * @param y
     */
    virtual void getAbsolutePosition(double *x, double *y) const;

    virtual ~PointingDevice(void) {}

  } ;

}

#endif
