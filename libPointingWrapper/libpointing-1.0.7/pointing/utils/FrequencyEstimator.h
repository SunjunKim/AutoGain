/* -*- mode: c++ -*-
 *
 * pointing/utils/FrequencyEstimator.h --
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

#ifndef FREQUENCY_ESTIMATOR_H
#define FREQUENCY_ESTIMATOR_H

#include <pointing/utils/TimeStamp.h>

namespace pointing {

  /**
   * @brief The FrequencyEstimator class can be used to estimate the frequency of an input device
   *
   * Frequency is estimated using a sliding window and computing the variance of timestamp deltas
   * Whenever the variance becomes less than minVariance, minVariance is updated.
   * Whenever the estimated frequency changes significantly the stableVariance is updated.
   * Therefore the stableVariance >= minVariance.
   * stableVariance is used to detect the change of input frequency.
   */
  class FrequencyEstimator
  {
    TimeStamp::inttime lastTime = 0;

    static const int N = 10; // Memorize last N dx values
    int dxInd = 0; // Circular array
    double dxs[N];
    double sumDx = 0.;
    double minVariance = 10e9;
    double stableVariance = 250.;
    double estimate = -1.;

  public:
    FrequencyEstimator();

    void registerTimeStamp(pointing::TimeStamp::inttime timestamp);

    double estimatedFrequency() const;

    void reset();
  };

}

#endif // FREQUENCY_ESTIMATOR_H
