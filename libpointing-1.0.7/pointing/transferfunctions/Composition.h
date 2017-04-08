/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/Composition.h --
 *
 * Initial software
 * Authors: Nicolas Roussel
 * Copyright � Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#ifndef Composition_h
#define Composition_h

#include <pointing/transferfunctions/TransferFunction.h>

#include <list>

namespace pointing {

  /**
   * @brief The Composition class can be used to apply several transfer functions to the input.
   *
   * It is called by specifying URI as composition:<filename> where filename is read line by line,
   * where each line must represent a transfer function.
   * function are called in appearing order, i.e.
   * pixels = fn(...f2(f1(mickeys)))
   *
   * A function can be added to the start or to the end of the function list.
  */
  class Composition : public TransferFunction {

    PointingDevice *input ;
    DisplayDevice *output ;
    std::list<TransferFunction*> functions ;

    int debugLevel ;

  public:
  
    Composition(URI &uri, PointingDevice* input, DisplayDevice* output) ;

    /**
     * @brief prependFunction is used to add a function to the beginning of the list
     * so that it is called at the beginning.
     * URI, URI string specifying a transfer function
     * or transfer function itself may be passed as an argument.
     */
    //@{
    void prependFunction(std::string uri) ;
    void prependFunction(URI &uri) ;
    void prependFunction(TransferFunction *function) ;
    //@}

    /**
     * @brief appendFunction is used to add a function to the end of the list
     * so that it is called at the after all the functions in the list.
     * URI, URI string specifying a transfer function
     * or transfer function itself may be passed as an argument.
     */
    //@{
    void appendFunction(std::string uri) ;
    void appendFunction(URI &uri) ;
    void appendFunction(TransferFunction *function) ;
    //@}

    /**
     * @return The number of transfer functions.
     */
    unsigned long size(void) const { return functions.size() ; }

    void clearState(void) {}

    void applyi(int dxMickey, int dyMickey, int *dxPixel, int *dyPixel,
	       TimeStamp::inttime timestamp=TimeStamp::undef) ;
    void applyd(int dxMickey, int dyMickey, double *dxPixel, double *dyPixel,
           TimeStamp::inttime timestamp=TimeStamp::undef) ;

    URI getURI(bool expanded=false) const ;

    ~Composition(void) ;

  } ;

}

#endif
