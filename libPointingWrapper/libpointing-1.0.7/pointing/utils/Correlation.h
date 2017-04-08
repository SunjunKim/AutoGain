/* -*- mode: c++ -*-
 *
 * pointing/utils/Correlation.h --
 *
 * Initial software
 * Authors: Damien Marchal
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#ifndef CORRELATION_H
#define CORRELATION_H

#include <vector>
#include <iostream>
#include <math.h>

namespace pointing {

  namespace Correlation {

    typedef std::vector<double> VectorCoeficient;

    template<class VectorInterface, typename T>
    double icorr(int i, VectorInterface& data, VectorInterface& pattern, T& pcenter, bool relative=true){
      double sum=0;

      // Compute the center
      typename VectorInterface::value_type dcenter;
      if(relative){
	for(int j=pattern.size()-1, ij=i;j>=0;j--, ij--){
	  dcenter+=data[ij];
	}
	dcenter/=(1.0*pattern.size());
      }


      for(int j=pattern.size()-1, ij=i;j>=0;j--, ij--){
	typename VectorInterface::value_type d=(pattern[j]-pcenter)-(data[ij]-dcenter);
	sum+= (d.x()*d.x()+d.y()*d.y());
      }
      return sum;
    }
    template<class VectorInterface>
    int compute(const VectorInterface& data, const VectorInterface& pattern, VectorCoeficient& corrcoef, bool relative=true){
      // Resize the output vector in case it is not large enought to store the computation.
      corrcoef.resize(data.size());

      // This is needed to align sequence in a relative manner
      typename VectorInterface::value_type pcenter;
      if(relative){
	for(unsigned int i=0;i<pattern.size();i++){
	  pcenter+=pattern[i];
	}
	pcenter/=((1.0)*pattern.size());
      }

      // For each possible alignment calculate the match
      for(unsigned int i=data.size()-1;i>=pattern.size()+1;i--){
	corrcoef[i]=icorr(i, data, pattern, pcenter, relative);
      }
      return 0;
    }


  }

}

#endif // CORRELATION_H
