/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/SubPixelFunction.h --
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

#ifndef SUBPIXELFUNCTION_H
#define SUBPIXELFUNCTION_H

#include <pointing/transferfunctions/TransferFunction.h>

namespace pointing
{
  /**
   * @brief The SubPixelFunction class is the subclass of TransferFunction
   * which can be used to extend any TransferFunction to have subpixel functionality.
   *
   * The implementation is based on the paper of N. Roussel, G. Casiez, J. Aceituno and D. Vogel.
   * "Giving a hand to the eyes: leveraging input accuracy for subpixel interaction."
   */
  class SubPixelFunction : public TransferFunction
  {
    PointingDevice *input;
    DisplayDevice *output;
    // Decorator pattern is used to extend any TransferFunction
    TransferFunction *func;

    bool isOn;
    int cardinality;
    int widgetSize;
    float resUseful;

    TimeStamp::inttime lastTime;
    double vPix, vUse, gOpt, gPix;

    void minGainAndVelocity();
    void computeParameters();

    int debugLevel;

    URI decodeURI(URI &uri);
    void initialize(URI &uri, URI &funcUri, PointingDevice *input, DisplayDevice *output);

  public:

    SubPixelFunction(const char* uri, PointingDevice *input, DisplayDevice *output);
    SubPixelFunction(std::string uri, PointingDevice *input, DisplayDevice *output);
    SubPixelFunction(URI &uri, PointingDevice *input, DisplayDevice *output);

    SubPixelFunction(URI uri, URI funcUri, PointingDevice *input, DisplayDevice *output);

    /**
     * @brief setSubPixeling Turn on or off subpixeling
     * @param subpixeling Subpixeling is turned on if True otherwise defined transfer function is applied
     */
    void setSubPixeling(bool subpixeling);

    /**
     * @brief Returns true if subpixeling is activated
     */
    bool getSubPixeling() const;

    /**
     * @brief setHumanResolution Sets the resolution of the pointing device
     * at which humans have maximum pointing task efficiency.
     * @param humanResolution Device's Human Resolution in CPI
     */
    void setHumanResolution(int humanResolution);

    /**
     * @brief getHumanResolution Gets the resolution of the pointing device
     * at which humans have maximum pointing task efficiency.
     */
    int getHumanResolution() const;

    /**
     * @brief setCardinalitySize
     * @param cardinality Parameter defining the number of elements which users expect to select from.
     * @param size The size of the model (element or widget) in pixels
     */
    void setCardinalitySize(int cardinality, int size);

    /**
     * @brief getCardinalitySize
     * @param cardinality Parameter defining the number of elements which users expect to select from.
     * @param size The size of the model (element or widget) in pixels
     */
    void getCardinalitySize(int *cardinality, int *size) const;

    void clearState(void);

    void applyi(int dxMickey, int dyMickey, int *dxPixel, int *dyPixel,
                TimeStamp::inttime timestamp=TimeStamp::undef);

    void applyd(int dxMickey, int dyMickey, double *dxPixel, double *dyPixel,
                TimeStamp::inttime timestamp=TimeStamp::undef);

    URI getURI(bool expanded=false) const;

    /**
     * @brief getInnerURI The original transfer function URI without subpixeling
     * @param expanded Indicates whether all parameters must be included in URI.
     * @return URI result
     */
    URI getInnerURI(bool expanded=false) const;

    ~SubPixelFunction();
  };
}

#endif // SUBPIXELFUNCTION_H
