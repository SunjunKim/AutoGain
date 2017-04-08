/* -*- mode: c++ -*-
 *
 * pointing/transferfunctions/windows/winSystemPointerAcceleration.cpp --
 *
 * Initial software
 * Authors: Géry Casiez
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/transferfunctions/windows/winSystemPointerAcceleration.h>
#include <pointing/transferfunctions/TransferFunction.h>
#if _MSC_VER > 1700
#include <VersionHelpers.h>
#endif

namespace pointing {

winSystemPointerAcceleration::winSystemPointerAcceleration() {
}

// Transfer function counts for which we will
// record the corresponding pixel translations
// and then convert into velocities and write them into
// registry to have a custom transfer function
int bestInd[] = {0, 1, 3, 11, 127};

// Fixed16 Format
unsigned convertToFixedWin16(double number)
{
    unsigned first = unsigned(number);
    unsigned second = unsigned((number - first)*65536.);
    return (first << 16) | second;
}

// Look up all values points -> pixels
void fillDisplacements(TransferFunction *func, double *vPointing, double *vDisplay)
{
    for (int i = 1; i < 128; i++)
    {
        double dpx = 0., dpy = 0.;
        func->applyd(i, 0, &dpx, &dpy);
        func->clearState();
        // 3.5, 96, 150 - Constants by default
        vPointing[i] = i / 3.5;
        vDisplay[i] = dpx * 96. / 150;
    }
}

void findBestPoints(double *vPointing, double *vDisplay)
{
    int maxIndex[2] = {0};
    double gains[128] = {0};
    for (int i = 1; i < 128; i++)
        gains[i] =  vDisplay[i] / vPointing[i];

    for (int n = 0; n < 2; n++)
    {
        double maxDiff = 0.;
        for (int i = 2; i < 127; i++) // 0, 1 and 127 are taken by default
        {
            // Second derivative
            double diff = abs(gains[i + 1] - 2 * gains[i] + gains[i - 1]);
            if (diff > maxDiff && i != maxIndex[(n - 1) % 2])
            {
                maxDiff = diff;
                maxIndex[n] = i;
            }
        }
    }
    // Should be increasing order
    if (maxIndex[0] > maxIndex[1])
    {
        int temp = maxIndex[0];
        maxIndex[0] = maxIndex[1];
        maxIndex[1] = temp;
    }
#if 0
    std::cerr << "Found indices: " << maxIndex[0] << " " << maxIndex[1] << std::endl;
#endif
    bestInd[2] = maxIndex[0];
    bestInd[3] = maxIndex[1];
}

void winSystemPointerAcceleration::setTransferFunction(URI &funcURI)
{
    PointingDevice *input = PointingDevice::create("dummy:?cpi=400&hz=125");
    DisplayDevice *output = DisplayDevice::create("dummy:?ppi=96");
    TransferFunction *func = TransferFunction::create(funcURI, input, output);

    double vPointing[128] = {0.};
    double vDisplay[128] = {0.};

    fillDisplacements(func, vPointing, vDisplay);
    findBestPoints(vPointing, vDisplay);

    // 5 integers for the values others for the padding
    // in the registry
    unsigned int vRegIn[10] = {0};
    unsigned int vRegOut[10] = {0};
    for (int i = 0; i < 5; i++)
    {
        vRegIn[2*i] = convertToFixedWin16(vPointing[bestInd[i]]);
        vRegOut[2*i] = convertToFixedWin16(vDisplay[bestInd[i]]);
    }

    HKEY tableKey;
    DWORD size = 40;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Mouse"), 0, KEY_SET_VALUE,
                     &tableKey) == ERROR_SUCCESS)
    {
        RegSetValueEx(tableKey, TEXT("SmoothMouseXCurve"), 0, REG_BINARY, (const BYTE *)vRegIn, size);
        RegSetValueEx(tableKey, TEXT("SmoothMouseYCurve"), 0, REG_BINARY, (const BYTE *)vRegOut, size);
        RegCloseKey(tableKey);
    }

    for (int i = 0; i < 128; i++)
    {
        double dpx = 0., dpy = 0.;
        func->applyd(i, 0, &dpx, &dpy);
        //std::cout << i << " " << i / 3.5 << " " << dpx / 96. * 150 << std::endl;
    }

    delete func;
    delete output;
    delete input;
    set(0, true);
}

  void
  winSystemPointerAcceleration::get(std::string *winVersion, int *sliderPosition, bool *enhancePointerPrecision) {
    HKEY mouseKey;
    int mouseSensitivity = 10; // From registry HKEY_CURRENT_USER\Control Panel\Mouse\MouseSensitivity

    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Mouse"), 0, KEY_READ,
		     &mouseKey) == ERROR_SUCCESS) {
      TCHAR mouseSensitivityREG_SZ[3];
      DWORD sizeofMouseREG_SZ = sizeof(mouseSensitivityREG_SZ);
      if (RegQueryValueEx(mouseKey, TEXT("MouseSensitivity"), NULL, NULL,
              (LPBYTE)&mouseSensitivityREG_SZ, &sizeofMouseREG_SZ) == ERROR_SUCCESS) {
	mouseSensitivityREG_SZ[2] = TEXT('\0'); // (Safety string termination)
	char mouseSensitivityChar[2];
	mouseSensitivityChar[0] = (char)mouseSensitivityREG_SZ[0];
	mouseSensitivityChar[1] = (char)mouseSensitivityREG_SZ[1];
	mouseSensitivity = atoi(mouseSensitivityChar);
      }
      TCHAR mouseSpeedREG_SZ[2];
      sizeofMouseREG_SZ = sizeof(mouseSpeedREG_SZ);
      if (RegQueryValueEx(mouseKey, TEXT("MouseSpeed"), NULL, NULL,
			  (LPBYTE)&mouseSpeedREG_SZ, &sizeofMouseREG_SZ) == ERROR_SUCCESS) {
	mouseSpeedREG_SZ[1] = TEXT('\0'); // (Safety string termination)
	char mouseSpeedChar[2];
	mouseSpeedChar[0] = (char)mouseSpeedREG_SZ[0];
	mouseSpeedChar[1] = '\0';
	int mouseSpeed = atoi(mouseSpeedChar);
	if (mouseSpeed == 1) *enhancePointerPrecision = true; else *enhancePointerPrecision = false;
      }
      RegCloseKey(mouseKey);
    }
    if (mouseSensitivity == 1)
      *sliderPosition = -5; 
    else
      *sliderPosition = (mouseSensitivity-10)/2;

#if _MSC_VER < 1800

	OSVERSIONINFO osVersion;
	osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osVersion);
	if (osVersion.dwMajorVersion == 5 && osVersion.dwMinorVersion == 1)
		*winVersion = "xp";
	if (osVersion.dwMajorVersion == 6 && osVersion.dwMinorVersion == 0)
		*winVersion = "vista";
	if (osVersion.dwMajorVersion == 6 && osVersion.dwMinorVersion == 1)
		*winVersion = "7";
	if (osVersion.dwMajorVersion == 6 && osVersion.dwMinorVersion == 2)
		*winVersion = "8";
	if (osVersion.dwMajorVersion == 6 && osVersion.dwMinorVersion == 3)
		*winVersion = "8.1";
	if (osVersion.dwMajorVersion == 10 && osVersion.dwMinorVersion == 0)
		*winVersion = "10";

#else
#ifndef IsWindows10OrGreater
    // TODO Temporary fix
#define IsWindows10OrGreater() IsWindowsVersionOrGreater(10, 0, 0)
#endif
	if (IsWindows10OrGreater())
		*winVersion = "10";
    else if (IsWindows8Point1OrGreater())
		*winVersion = "8.1";
	else if (IsWindows8OrGreater())
		*winVersion = "8.0";
	else if (IsWindows7SP1OrGreater())
		*winVersion = "7 SP1";
	else if (IsWindows7OrGreater())
		*winVersion = "7";
	else if (IsWindowsXPSP3OrGreater())
		*winVersion = "XP SP3";
	else if (IsWindowsXPSP2OrGreater())
		*winVersion = "XP SP2";
	else if (IsWindowsXPSP1OrGreater())
		*winVersion = "XP SP1";
	else if (IsWindowsXPOrGreater())
		*winVersion = "XP";
#endif


  }

  void
  winSystemPointerAcceleration::set(int sliderPosition, bool enhancePointerPrecision) {
    // Note that the "Pointer Options" tab in "Mouse Properties" won't upate the slider position and
    // check box state until rebooting. The changes are still correctly applied to the mouse pointer

    if ((sliderPosition < -5) || (sliderPosition > 5)) {
      std::cerr << "winSystemPointerAcceleration: slider position has to be >= -5 and <= +5" << std::endl;
      return;
    }
    int accValue = sliderPosition*2+10;
    if (accValue == 0) accValue = 1;
    if (!SystemParametersInfo(SPI_SETMOUSESPEED, 0, (void*)accValue, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE))
      std::cerr << "winSystemPointerAcceleration could not set the parameters to the system" << std::endl;

    int param[3] = {6,10, 1};
    if (!enhancePointerPrecision) param[2] = 0;
    SystemParametersInfo(SPI_SETMOUSE, 0, param, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
  }

  winSystemPointerAcceleration::~winSystemPointerAcceleration() {
  }

}
 

