/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/Interpolation.h --
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

#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <pointing/transferfunctions/TransferFunction.h>
#include <map>
#include <string>
#include <vector>
#include <pointing/utils/ConfigDict.h>

namespace pointing
{
  enum class InterpolationSpace { VelocityGain, VelocityVelocity };

  /**
   * @brief The Interpolation class is subclass of TransferFunction which can interpolate
   * between given values in the table.
   *
   */
  class Interpolation : public TransferFunction
  {
    void constructAccMap();
    void loadTableFromConfig(ConfigDict &accCfg);
    void loadFromDirectory();

  protected:

    PointingDevice *pointingDevice;
    DisplayDevice *displayDevice;
    PointingDevice *originalInput = nullptr;
    DisplayDevice *originalOutput = nullptr;

    InterpolationSpace space = InterpolationSpace::VelocityGain;

    bool normalize = false;
    std::string directory;
    std::string curAcc;

    int previousMouseRawX;
    int previousMouseRawY;
    float previousMouseXRemainder;
    float previousMouseYRemainder;

    /**
     * @brief Interpolate Values that do not exist in the main table are interpolated
     * using 2 other tables which refer to indices of the closest values.
     * @param lowInd
     * @param highInd
     */
    void Interpolate(std::vector<int> &lowInd, std::vector<int> &highInd);

    // Divide in order to have a coefficient, not the mapping
    // It means not dpx = tableAcc[dp]
    // But dpx = tableAcc[dp] * dp
    void TableToCoefficients();

    ConfigDict cfg;

    std::string system; // System name and version
    std::map<std::string, std::string> mapAcc;
    std::string replaceAlias(std::string &curAcc);

    std::vector<float> tableAcc;
    float valueFromTable(unsigned index);

    /**
     * @brief Interpolation::valueFromTable Makes subpoint interpolation
     * @param index Number of points
     * @return Corresponding Px
     */
    double valueFromTable(double index);

  public:

    Interpolation(URI &uri, PointingDevice *input, DisplayDevice *output);
    ~Interpolation();

    void loadTableStr(std::string tableData);

    void clearState(void);

    void applyi(int dxMickey, int dyMickey, int *dxPixel, int *dyPixel,
                TimeStamp::inttime timestamp=TimeStamp::undef);

    void applyd(int dxMickey, int dyMickey, double *dxPixel, double *dyPixel,
                TimeStamp::inttime timestamp=TimeStamp::undef);

    URI getURI(bool expanded=false) const;
  };
}

#endif // INTERPOLATION_H
