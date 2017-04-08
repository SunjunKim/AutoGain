/* -*- mode: c++ -*-
 *
 * pointing/utils/FrequencyEstimator.cpp --
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

#include <pointing/utils/FrequencyEstimator.h>
#include <math.h>

namespace pointing
{

#define errThreshold .6

  FrequencyEstimator::FrequencyEstimator()
  {
    for (int i = 0; i < N; i++)
      dxs[i] = 0;
  }

  void FrequencyEstimator::registerTimeStamp(TimeStamp::inttime timestamp)
  {
    double delta = double(timestamp - lastTime) / TimeStamp::one_millisecond;
    lastTime = timestamp;
    // Delta cannot be less than 0.4 ms
    // So filter it out
    if (delta < 0.4)
      delta = dxInd;

    sumDx += delta - dxs[dxInd];
    dxs[dxInd] = delta;
    dxInd = (dxInd + 1) % N;

    double average = sumDx / N;
    double variance = 0.;
    for (int i = 0; i < N; i++)
    {
        double dif = dxs[i] - average;
        variance += dif * dif;
    }

    double curEstimate = sumDx / N;
    // Polling rate was changed
    if (variance < stableVariance && curEstimate - estimate > errThreshold)
    {
      // Reset minimum variance
      minVariance = 10e9;
    }
    // Improve estimate
    if (variance < minVariance)
    {
      minVariance = variance;
      if (curEstimate - estimate > errThreshold && minVariance < stableVariance)
      {
        stableVariance = minVariance;
      }
      estimate = curEstimate;
    }
  }

  double FrequencyEstimator::estimatedFrequency() const
  {
    if (minVariance >= stableVariance)
      return -1.;

    static const int stdFreqN = 4;
    static const double stdFreqs[stdFreqN] = { 1., 2., 4., 8. };

    for (int i = 0; i < stdFreqN; i++)
    {
      double err = fabs(estimate - stdFreqs[i]);
      if (err < errThreshold)
        return 1000. / stdFreqs[i];
    }
    return 1000. / estimate;
  }

  void FrequencyEstimator::reset()
  {
    lastTime = 0;
    minVariance = 10e9;
    stableVariance = 250.;
    estimate = -1.;
  }
}
