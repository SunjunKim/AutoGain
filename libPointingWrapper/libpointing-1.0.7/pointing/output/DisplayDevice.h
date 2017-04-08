/* -*- mode: c++ -*-
 *
 * pointing/output/DisplayDevice.h --
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

#ifndef DisplayDevice_h
#define DisplayDevice_h

#include <pointing/utils/URI.h>

#include <iostream>

namespace pointing {

  /**
   * @brief DisplayDevice class is used to represent the displays connected to the computer.
   *
   * It is an abstract class which creates an instance of its platform-specific
   * subclass.
   */
  class DisplayDevice {

  protected:

    DisplayDevice(void) {}

  public:
  
	/**
     * @brief A structure to maintain coordinates of a pixel.
	 */
    struct Point { 
      float x, y ;
      Point(void) : x(0), y(0) {}
      Point(float x, float y) : x(x), y(y) {}
    } ;
	
	/**
     * @brief Display size in mms.
	 */
    struct Size {
      float width, height ;
      Size(void) : width(0), height(0) {}
      Size(float w, float h) : width(w), height(h) {}
    } ;

	/**
     * @brief Display bounds (origin and size) in pixels
	 */
    struct Bounds {
      Point origin ; 
      Size size ; 
      Bounds(void) {}
      Bounds(float x, float y, float w, float h) : origin(x,y), size(w,h) {}
    } ;

	/**
     * @brief Static create method is used to instantiate an object of the class.
	 */
    static DisplayDevice *create(const char *device_uri=0) ;
    static DisplayDevice *create(std::string device_uri) ;
  
    /**
	 * @return Bounds of the device expressed in pixels.
	 */
    virtual Bounds getBounds(Bounds *defval=0) = 0 ;      // pixels

	/**
	 * @return The size of the device (width and height) in mm.
	 */
    virtual Size getSize(Size *defval=0) = 0 ;            // mm

    /**
     * @brief Computes the pixel density (resolution) of the display device.
     *
     * It is computed by calculating the diagonal size of the display
     * in pixels and inches and then calculating PPI (Pixels per inch).
     * @param hppi Horizontal resolution (OUT)
     * @param vppi Vertical resolution (OUT)
     * @param defval Default value
     * @return PPI resolution
     */
    virtual double getResolution(double *hppi, double *vppi, double *defval=0) ; // ppi
    double getResolution(double *defval=0) {
      return getResolution(0, 0, defval) ;
    }

	/**
	 * @return The refresh rate of the device in Hz.
	 */
    virtual double getRefreshRate(double *defval=0) = 0 ; // Hz

	/**
	 * @return The corresponding URI of the device.
	 * @param expanded specifies if additional parameters must be displayed
	 */
    virtual URI getURI(bool expanded=false) const = 0 ;

	/**
     * @brief Sets the level of information for debugging purposes (default = 0).
	 */
    virtual void setDebugLevel(int /*level*/) {}
    virtual void debug(std::ostream& /*out*/) const {}

    virtual ~DisplayDevice(void) {}

  } ;

}

#endif
