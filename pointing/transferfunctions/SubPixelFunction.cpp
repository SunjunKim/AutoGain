/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/SubPixelFunction.cpp --
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

#include <pointing/transferfunctions/SubPixelFunction.h>
#include <math.h>

#define         DEFAULT_RESOLUTION_HUMAN       400
#define         MIN_RESOLUTION_HUMAN           200

#define         MIN(X, Y)                      (((X) < (Y)) ? (X) : (Y))
#define         MAX(X, Y)                      (((X) > (Y)) ? (X) : (Y))

// Implementation follows the original paper:
// http://interaction.lille.inria.fr/~roussel/publications/2012-UIST-subpixel.pdf
// However, the difference is that variables are expressed in inches, rather than mms
namespace pointing
{
  URI SubPixelFunction::decodeURI(URI &uri)
  {
    std::string encodedUri;
    URI::getQueryArg(uri.query, "transFunc", &encodedUri);
    return URI::decode(encodedUri);
  }

  SubPixelFunction::SubPixelFunction(const char *uriString, PointingDevice *input, DisplayDevice *output)
  {
    URI uri(uriString);
    URI decodedURI(uri);
    initialize(uri, decodedURI, input, output);
  }

  SubPixelFunction::SubPixelFunction(std::string uriString, PointingDevice *input, DisplayDevice *output)
  {
    URI uri(uriString);
    URI decodedURI(uri);
    initialize(uri, decodedURI, input, output);
  }

  SubPixelFunction::SubPixelFunction(URI &uri, PointingDevice *input, DisplayDevice *output)
  {
    std::string encodedUri;
    URI::getQueryArg(uri.query, "transFunc", &encodedUri);

    URI funcUri(URI::decode(encodedUri));
    initialize(uri, funcUri, input, output);
  }

  void SubPixelFunction::initialize(URI &uri, URI &funcUri, PointingDevice *input, DisplayDevice *output)
  {
    debugLevel = 0 ;
    URI::getQueryArg(uri.query, "debugLevel", &debugLevel) ;

    func = TransferFunction::create(funcUri, input, output);

    isOn = true;
    cardinality = widgetSize = lastTime = 0;
    URI::getQueryArg(uri.query, "isOn", &isOn);
    URI::getQueryArg(uri.query, "cardinality", &cardinality);
    if (!URI::getQueryArg(uri.query, "widgetsize", &widgetSize))
      URI::getQueryArg(uri.query, "widgetSize", &widgetSize);

    this->input = input;
    this->output = output;

    minGainAndVelocity();
    int resHuman = DEFAULT_RESOLUTION_HUMAN;
    URI::getQueryArg(uri.query, "resHuman", &resHuman);
    setHumanResolution(resHuman);
    computeParameters();
  }

  SubPixelFunction::SubPixelFunction(URI uri, URI funcUri, PointingDevice *input, DisplayDevice *output)
  {
    initialize(uri, funcUri, input, output);
  }

  void SubPixelFunction::minGainAndVelocity()
  {
    int x = 0;
    double dxP = 0, dxY;
    while (dxP < 1 && ++x < 128)
    {
      func->applyd(x, 0, &dxP, &dxY);
    }

    if (debugLevel)
      std::cerr << "One pixel gain starts when input displacement is " << x << " or more" << std::endl;

    gPix = dxP / x * input->getResolution() / output->getResolution();
    vPix = input->getUpdateFrequency() / output->getResolution() / gPix;

    if (debugLevel)
      std::cerr << "One pixel gain Gpix: " << gPix << std::endl << "One pixel velocity Vpix: " << vPix << std::endl;
  }

  void SubPixelFunction::computeParameters()
  {
    if (cardinality > 0 && widgetSize > 0)
    {
      vUse = input->getUpdateFrequency() / 1000 / resUseful;
      gOpt = widgetSize * resUseful / cardinality / output->getResolution();
      if (gOpt > gPix)
      {
        std::cerr << "Warning: Gopt > Gpix. No subpixeling" << std::endl;
        cardinality = widgetSize = 0;
      }
      if (debugLevel)
      {
        std::cerr << "Optimal gain Gopt: " << gOpt << std::endl << "Useful velocity Vuse: " << vUse << std::endl;
      }
    }
    else
    {
      // If one of them is incorrect, change both to 0
      cardinality = widgetSize = 0;
    }
  }

  void SubPixelFunction::setSubPixeling(bool subpixeling)
  {
    this->isOn = subpixeling;
  }

  bool SubPixelFunction::getSubPixeling() const
  {
    return this->isOn;
  }

  void SubPixelFunction::setHumanResolution(int resHuman)
  {
    // Should not be too small
    resUseful = MIN(MAX(resHuman, MIN_RESOLUTION_HUMAN), input->getResolution());
  }

  int SubPixelFunction::getHumanResolution() const
  {
    return this->resUseful;
  }

  void SubPixelFunction::setCardinalitySize(int cardinality, int size)
  {
    this->cardinality = cardinality;
    this->widgetSize = size;
    computeParameters();
  }

  void SubPixelFunction::getCardinalitySize(int *cardinality, int *size) const
  {
    *cardinality = this->cardinality;
    *size = this->widgetSize;
  }

  void SubPixelFunction::clearState()
  {
    lastTime = 0;
    func->clearState();
  }

  void SubPixelFunction::applyi(int dxMickey, int dyMickey, int *dxPixel, int *dyPixel, TimeStamp::inttime timestamp)
  {
    func->applyi(dxMickey, dyMickey, dxPixel, dyPixel, timestamp);
  }

  void SubPixelFunction::applyd(int dxMickey, int dyMickey, double *dxPixel, double *dyPixel, TimeStamp::inttime timestamp)
  {
    if (isOn && cardinality > 0)
    {
      double dt = double(timestamp - lastTime) / TimeStamp::one_second;
      lastTime = (!timestamp || timestamp == TimeStamp::undef) ? TimeStamp::createAsInt() : timestamp;

      // inches
      double dd = sqrt(dxMickey * dxMickey + dyMickey * dyMickey) / input->getResolution();
      if (!dd)
      {
        *dxPixel = *dyPixel = 0.;
        return;
      }

      // inches per sec
      double speed = dd / dt;

      double outDx = 0, outDy = 0;
      func->applyd(dxMickey, dyMickey, &outDx, &outDy, timestamp);
      double gain = sqrt(outDx * outDx + outDy * outDy) / dd / output->getResolution();

      if (debugLevel > 1)
        std::cerr << "Original gain: " << gain << " for speed: " << speed << std::endl;

      double q = 1.0;
      if (vPix > vUse)
      {
        // q should be between 0 and 1
        q = MIN(MAX((speed - vUse) / (vPix - vUse), 0.), 1.);
      }
      else
      {
        q = MIN(speed / vPix, 1.0);
      }
      gain = (1 - q) * gOpt + q * gain;

      double pixelGain = gain * output->getResolution() / input->getResolution();
      *dxPixel = dxMickey * pixelGain;
      *dyPixel = dyMickey * pixelGain;

      if (debugLevel > 1) std::cerr << "Computed gain: " << gain << std::endl;
    }
    else func->applyd(dxMickey, dyMickey, dxPixel, dyPixel, timestamp);
  }

  URI SubPixelFunction::getURI(bool expanded) const
  {
    std::string encodedTF = URI::encode(func->getURI(expanded).asString());
    URI uri;
    uri.scheme = "subpixel";
    URI::addQueryArg(uri.query, "cardinality", cardinality);
    URI::addQueryArg(uri.query, "widgetSize", widgetSize);
    URI::addQueryArg(uri.query, "transFunc", encodedTF);
    if (expanded || resUseful != DEFAULT_RESOLUTION_HUMAN)
      URI::addQueryArg(uri.query, "resHuman", resUseful);
    return uri ;
  }

  URI SubPixelFunction::getInnerURI(bool expanded) const
  {
    return func->getURI(expanded);
  }

  SubPixelFunction::~SubPixelFunction()
  {
    delete func;
  }
}
