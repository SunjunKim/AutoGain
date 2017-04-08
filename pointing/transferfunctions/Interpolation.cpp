/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/Interpolation.cpp --
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

#include <pointing/transferfunctions/Interpolation.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <assert.h>

using namespace std;

namespace pointing
{
#define Sign(X) ((X>0)?(1):(-1))
#define Abs(X) (Sign(X)*X)

  double round(double d)
  {
    return floor(d + 0.5);
  }

  std::string iToStr(int i)
  {
    stringstream ss;
    ss << i;
    return ss.str();
  }

  void Interpolation::TableToCoefficients()
  {
    for (size_t i = 1; i < tableAcc.size(); i++)
    {
      tableAcc[i] /= i;
    }
  }

  void Interpolation::Interpolate(vector<int> &lowInd, vector<int> &highInd)
  {
    for (size_t i = 0; i < tableAcc.size(); i++)
    {
      int prevI = lowInd[i];
      int nextI = highInd[i];
      if (prevI != nextI)
      {
        float coef = float(i - prevI) / (nextI - prevI);
        tableAcc[i] = (tableAcc[nextI] - tableAcc[prevI]) * coef + tableAcc[prevI];
      }
    }
  }

  void Interpolation::loadTableFromConfig(ConfigDict &accCfg)
  {
    int nCounts = accCfg.get<int>("max-counts") + 1;
    tableAcc = vector<float>(nCounts, 0.0);
    tableAcc[nCounts - 1] = accCfg.get<float>(iToStr(nCounts));

    vector<int> lowInd(nCounts, 0);
    vector<int> highInd(nCounts, nCounts - 1);

    // Get the values, at the same time fill the lowInd table
    // in order to find the closest previous value for the
    // points without corresponding values
    for (int i = 1; i < nCounts; i++)
    {
      float px = accCfg.get<float>(iToStr(i));
      if (px > 0)
      {
        tableAcc[i] = px;
        lowInd[i] = i;
      }
      else
        lowInd[i] = lowInd[i - 1];
    }

    // Fill the closest next values
    for (int i = nCounts - 2; i >= 0; i--)
    {
      if (lowInd[i] == i) // There is a value
        highInd[i] = i;
      else
        highInd[i] = highInd[i + 1];
    }
    if (space == InterpolationSpace::VelocityGain)
      TableToCoefficients();
    Interpolate(lowInd, highInd);
    if (space == InterpolationSpace::VelocityVelocity)
      TableToCoefficients();
  }

  void Interpolation::loadFromDirectory()
  {
    string path = directory + "/" + replaceAlias(curAcc) + ".dat";
    ConfigDict accCfg;
    if (!accCfg.loadFrom(path))
    {
      cerr << "Unable to open the interpolation data from " << path << endl;
      path = directory + "/" + cfg.get<string>("default-function") + ".dat";
      if (accCfg.loadFrom(path))
        cerr << "Loaded the default function from" << path << endl;
    }
    loadTableFromConfig(accCfg);
  }

  void Interpolation::loadTableStr(std::string tableData)
  {
    ConfigDict accCfg;
    accCfg.load(tableData);
    loadTableFromConfig(accCfg);
  }

  vector<string> split(const string &s, const string &separ)
  {
    vector<string> elems;
    int i = 0, last = 0;
    while ((i = s.find(separ, last)) != -1)
    {
      string subs = s.substr(last, i - last);
      elems.push_back(subs);
      last = i + separ.length();
    }
    if (last < int(s.length())) // The last element
      elems.push_back(s.substr(last));
    return elems;
  }

  void Interpolation::constructAccMap()
  {
    vector<string> vecFunctions = split(cfg.get<string>("functions"), ",");
    vector<string> vecFunctionAls = split(cfg.get<string>("function-aliases"), ",");
    int len = min(vecFunctions.size(), vecFunctionAls.size());
    for (int i = 0; i < len; i++)
    {
      mapAcc[vecFunctionAls[i]] = vecFunctions[i];
    }
  }

  std::string Interpolation::replaceAlias(string &curAcc)
  {
    map<string, string>::iterator it = mapAcc.find(curAcc);
    if (it != mapAcc.end())
      return mapAcc[curAcc];
    return curAcc;
  }

  Interpolation::Interpolation(URI &uri, PointingDevice *input, DisplayDevice *output)
    :tableAcc(1, 0.)
  {
    if (uri.scheme!="interp")
      cerr << "Interpolation warning: scheme is '" << uri.scheme << "', should be 'interp'" << endl ;

    pointingDevice = input;
    displayDevice = output;

    URI::getQueryArg(uri.query, "f", &curAcc);
    URI::getQueryArg(uri.query, "normalize", &normalize);
    string spaceStr;
    URI::getQueryArg(uri.query, "space", &spaceStr);
    if (spaceStr == "vv")
      space = InterpolationSpace::VelocityVelocity;

    directory = uri.opaque!="" ? uri.opaque : uri.path ;
    string configDictPath = directory + "/config.dict";
    if (cfg.loadFrom(configDictPath))
    {
      string origInputURI = cfg.get<string>("libpointing-input");
      string origOutputURI = cfg.get<string>("libpointing-output");
      if (origInputURI.length() && origOutputURI.length())
      {
        originalInput = PointingDevice::create(origInputURI);
        originalOutput = DisplayDevice::create(origOutputURI);
      }
      else
      {
        // Cannot normalize if original devices are not defined
        normalize = false;
      }

      system = cfg.get<string>("system");
      constructAccMap();
      if (curAcc.empty())
        curAcc = cfg.get<string>("default-function");
      loadFromDirectory();
    }
    else
      cerr << "Unable to open the interpolation file " << configDictPath << endl;
    clearState();
  }

  void Interpolation::clearState(void)
  {
    previousMouseRawX = 0;
    previousMouseRawY = 0;
    previousMouseXRemainder = 0.0;
    previousMouseYRemainder = 0.0;
  }

  // Floors -0.2 to 0 not to -1
  float AbsFloor(float num)
  {
    return floor(Abs(num))*Sign(num);
  }

  float Interpolation::valueFromTable(unsigned index)
  {
    if (index < (unsigned)tableAcc.size())
      return tableAcc[index];
    return tableAcc.back();
  }

  double Interpolation::valueFromTable(double index)
  {
    unsigned prevI = floor(index);
    unsigned nextI = ceil(index);
    double prevVal = valueFromTable(prevI);
    if (prevI >= nextI)
      return prevVal;
    double nextVal = valueFromTable(nextI);
    double coef = double(index - prevI) / (nextI - prevI);
    return coef * (nextVal - prevVal) + prevVal;
  }

  void Interpolation::applyi(int mouseRawX, int mouseRawY, int *dxPixel, int *dyPixel,
                             TimeStamp::inttime /*timestamp*/)
  {
    if (mouseRawX != 0) {
      if (Sign(mouseRawX) != Sign(previousMouseRawX)) previousMouseXRemainder = 0.0 ;
      previousMouseRawX = mouseRawX ;
    }
    if (mouseRawY != 0) {
      if (Sign(mouseRawY) != Sign(previousMouseRawY)) previousMouseYRemainder = 0.0 ;
      previousMouseRawY = mouseRawY;
    }

    float index = floor(sqrt(float(mouseRawX) * mouseRawX + float(mouseRawY) * mouseRawY));
	
    if (normalize)
      index *= round(originalInput->getResolution()) / round(pointingDevice->getResolution());

    float valTable = valueFromTable(index);
    float curValueX = valTable * Abs(mouseRawX) * Sign(mouseRawX);
    float curValueY = valTable * Abs(mouseRawY) * Sign(mouseRawY);

    if (normalize)
    {
      float coef = displayDevice->getResolution() / originalOutput->getResolution();
      curValueX = coef * curValueX;
      curValueY = coef * curValueY;
    }

    float mouseXplusRemainder = curValueX + previousMouseXRemainder;
    float mouseYplusRemainder = curValueY + previousMouseYRemainder;

    *dxPixel = (int)AbsFloor(mouseXplusRemainder) ;
    *dyPixel = (int)AbsFloor(mouseYplusRemainder) ;
    previousMouseXRemainder = mouseXplusRemainder - *dxPixel ;
    previousMouseYRemainder = mouseYplusRemainder - *dyPixel ;
  }

  void Interpolation::applyd(int mouseRawX, int mouseRawY, double *dxPixel, double *dyPixel,
                             TimeStamp::inttime /*timestamp*/)
  {
    double index = floor(sqrt(double(mouseRawX * mouseRawX + mouseRawY * mouseRawY)));

    if (normalize)
      index *= round(originalInput->getResolution()) / round(pointingDevice->getResolution());

    double valTable = valueFromTable(index);
    double curValueX = valTable * Abs(mouseRawX) * Sign(mouseRawX);
    double curValueY = valTable * Abs(mouseRawY) * Sign(mouseRawY);

    if (normalize)
    {
      double coef = displayDevice->getResolution() / originalOutput->getResolution();
      curValueX = coef * curValueX;
      curValueY = coef * curValueY;
    }

    *dxPixel = curValueX;
    *dyPixel = curValueY;
  }

  URI Interpolation::getURI(bool expanded) const
  {
    URI uri;
    uri.scheme = "interp";
    uri.path = directory;
    URI::addQueryArg(uri.query, "f", curAcc);
    if (expanded || normalize)
      URI::addQueryArg(uri.query, "normalize", normalize);
    if (expanded)
    {
      string spaceStr = "vg";
      if (space == InterpolationSpace::VelocityVelocity)
        spaceStr = "vv";
      URI::addQueryArg(uri.query, "space", spaceStr);
    }
    return uri;
  }

  Interpolation::~Interpolation()
  {
    delete originalInput;
    delete originalOutput;
  }
}
