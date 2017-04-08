/* -*- mode: c++ -*-
 *
 * pointing/output/windows/winDisplayDeviceHelper.h --
 *
 * Initial software
 * Authors: Géry Casiez, Izzat Mukhanov
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/output/windows/winDisplayDeviceHelper.h>

#include <iostream>
#include <sstream>
#include <algorithm>

namespace pointing {

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

  bool isdisplayIDvalid(std::wstring displayID) {
    bool res = false;
    if (displayID.length() == 0) return false;

    HKEY hSubKey;
    TCHAR keyName[MAX_KEY_LENGTH];
    keyName[0] = '\0';

    wcscat_s((wchar_t*)keyName, MAX_KEY_LENGTH, (const wchar_t*)TEXT("SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\"));
    wcscat_s((wchar_t*)keyName, MAX_KEY_LENGTH, (const wchar_t*)(displayID.c_str()));
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName, 0, KEY_ENUMERATE_SUB_KEYS, &hSubKey) == ERROR_SUCCESS) {
      res = true;
      RegCloseKey(hSubKey);
    }

    return res;
  }

  std::wstring getControlPanelDisplayName(int* numberDisplays) {
    DISPLAY_DEVICE dd;
    dd.cb = sizeof(dd);
    DWORD dev = 0;
    // monitor number, as used by Display Properties > Settings

    *numberDisplays = 0;
    std::wstring res;

    while (EnumDisplayDevices(0, dev, &dd, 0))
      {
	DISPLAY_DEVICE ddMon;
	ZeroMemory(&ddMon, sizeof(ddMon));
	ddMon.cb = sizeof(ddMon);
	DWORD devMon = 0;

	while (EnumDisplayDevices(dd.DeviceName, devMon, &ddMon, 0))
	  {
	    if (//ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE &&
		!(ddMon.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
	      {
		//std::wcout << ddMon.DeviceName << " " << ddMon.DeviceString << std::endl;
		res = ddMon.DeviceString;
		(*numberDisplays)++;
	      }
	    devMon++;

	    ZeroMemory(&ddMon, sizeof(ddMon));
	    ddMon.cb = sizeof(ddMon);
	  }

	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	dev++;
      }
    return res;
  }

  void decodeEDID(unsigned char edid[], winDisplayInfo* dinfo) {
    dinfo->ppi = 0;
    dinfo->refreshRate = 0;
    dinfo->resx = 0;
    dinfo->resy = 0;
    dinfo->w = 0;
    dinfo->h = 0;

    int pixelClock = (edid[55] << 8) + edid[54];

    // Screen size in mm
    if (pixelClock > 0) {
      dinfo->w = (int)(((edid[68] & 0xF0) << 4) +  edid[66]);
      dinfo->h = (int)(((edid[68] & 0xF) << 8) +edid[67]);
    } else {
      dinfo->w = edid[21] * 10;
      dinfo->h = edid[22] * 10;
    }

    // screen max resolution in pixel
    if (pixelClock > 0) {
      dinfo->resx = (int)(((edid[58] & 0xF0) << 4) +  edid[56]);
      dinfo->resy = (int)(((edid[61] & 0xF0) << 4) +edid[59]);
    } else {
      dinfo->resx = edid[38] * 8 + 248;

      int AspectRatio = (edid[39] >> 6);
      switch (AspectRatio) {
      case 0:
	dinfo->resy = (dinfo->resx / 16 * 10);
	break;
      case 1:
	dinfo->resy = (dinfo->resx / 4 * 3);
	break;
      case 2:
	dinfo->resy = (dinfo->resx / 5 * 4);
	break;
      case 3:
	dinfo->resy = (dinfo->resx / 16 * 9);
	break;
      default:
	dinfo->resy = (dinfo->resx / 4 * 3);
	break;
      }
    }

    dinfo->refreshRate = ((edid[39] & 0x1f) + 60);
    if (dinfo->w > 0) dinfo->ppi = dinfo->resx / ((float) dinfo->w) * 25.4; else dinfo->ppi = 0;

    // Monitor name
    int s = 0;
    if (edid[75] == 0xFC) s = 77; // Block 1
    if (edid[93] == 0xFC) s = 95; // Block 2
    if (edid[111] == 0xFC) s = 113; // Block 3

    for (int i=0; i<13; i++)
      dinfo->monitorName[i] = ' ';
    dinfo->monitorName[13] = '\0';
    int i = 0;
    while ((i<13) && (edid[s+i] != 0x0A)) {
      dinfo->monitorName[i] = edid[s+i];
      i++;
    }

  }

  void getDisplayInfo(std::wstring displayKey, winDisplayInfo* dinfo) {
    DWORD    cbName2;                   // size of name string
    TCHAR    achKey2[MAX_KEY_LENGTH];   // buffer for subkey name
    DWORD retCode;
    HKEY hSubKey;
    TCHAR keyName[MAX_KEY_LENGTH];
    FILETIME ftLastWriteTime;      // last write time
    keyName[0] = '\0';
    dinfo->w = 0;
    dinfo->h = 0;
	wcscat_s((wchar_t*)keyName, MAX_KEY_LENGTH, (const wchar_t*)TEXT("SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\"));
	wcscat_s((wchar_t*)keyName, MAX_KEY_LENGTH, (const wchar_t*)(displayKey.c_str()));
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName, 0, KEY_ENUMERATE_SUB_KEYS, &hSubKey) == ERROR_SUCCESS) {
      cbName2 = MAX_KEY_LENGTH;
      retCode = RegEnumKeyEx(hSubKey, 0,
			     achKey2,
			     &cbName2,
			     NULL,
			     NULL,
			     NULL,
			     &ftLastWriteTime);
	  SYSTEMTIME systemTime;
  FileTimeToSystemTime(&ftLastWriteTime, &systemTime);

      if (retCode == ERROR_SUCCESS) {
    wcscat_s((wchar_t*)keyName, MAX_KEY_LENGTH, (const wchar_t*)TEXT("\\"));
	wcscat_s((wchar_t*)keyName, MAX_KEY_LENGTH, (const wchar_t*)achKey2);
	wcscat_s((wchar_t*)keyName, MAX_KEY_LENGTH, (const wchar_t*)TEXT("\\Device Parameters"));

	HKEY hSubKey2;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName, 0, KEY_READ, &hSubKey2) == ERROR_SUCCESS) {
	  unsigned char hwREG_SZ[300];
	  DWORD sizeofhwREG_SZ = sizeof(hwREG_SZ);
	  if (RegQueryValueEx(hSubKey2, TEXT("EDID"), NULL, NULL,
			      (LPBYTE)&hwREG_SZ, &sizeofhwREG_SZ) == ERROR_SUCCESS) {
	    decodeEDID(hwREG_SZ, dinfo);
	  }
	}
	RegCloseKey(hSubKey2);
      }
      RegCloseKey(hSubKey);
    }
    else std::wcerr << "windisplay:/" << displayKey << " does not exist" << std::endl;
  }

  // Number of displays known in the registry
  int numberDisplays() {
    HKEY hKey;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Enum\\DISPLAY"), 0, KEY_READ, &hKey);

    TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name
    DWORD    cchClassName = MAX_PATH;  // size of class string
    DWORD    cSubKeys=0;               // number of subkeys
    DWORD    cbMaxSubKey;              // longest subkey size
    DWORD    cchMaxClass;              // longest class string
    DWORD    cValues;              // number of values for key
    DWORD    cchMaxValue;          // longest value name
    DWORD    cbMaxValueData;       // longest value data
    DWORD    cbSecurityDescriptor; // size of security descriptor
    FILETIME ftLastWriteTime;      // last write time

    // Get the class name and the value count.
    RegQueryInfoKey(
			      hKey,                    // key handle
			      achClass,                // buffer for class name
			      &cchClassName,           // size of class string
			      NULL,                    // reserved
			      &cSubKeys,               // number of subkeys
			      &cbMaxSubKey,            // longest subkey size
			      &cchMaxClass,            // longest class string
			      &cValues,                // number of values for this key
			      &cchMaxValue,            // longest value name
			      &cbMaxValueData,         // longest value data
			      &cbSecurityDescriptor,   // security descriptor
			      &ftLastWriteTime);       // last write time

    RegCloseKey(hKey);

    return cSubKeys - 1;
  }

  // First display found in the registry different from the default one
  std::wstring getFirstDisplay() {
    HKEY hKey;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Enum\\DISPLAY"), 0, KEY_READ, &hKey);

    TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
    DWORD    cbName;                   // size of name string
    TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name
    DWORD    cchClassName = MAX_PATH;  // size of class string
    DWORD    cSubKeys=0;               // number of subkeys
    DWORD    cbMaxSubKey;              // longest subkey size
    DWORD    cchMaxClass;              // longest class string
    DWORD    cValues;              // number of values for key
    DWORD    cchMaxValue;          // longest value name
    DWORD    cbMaxValueData;       // longest value data
    DWORD    cbSecurityDescriptor; // size of security descriptor
    FILETIME ftLastWriteTime;      // last write time

    DWORD i, retCode;

    std::wstring res = L"";

    // Get the class name and the value count.
    retCode = RegQueryInfoKey(
			      hKey,                    // key handle
			      achClass,                // buffer for class name
			      &cchClassName,           // size of class string
			      NULL,                    // reserved
			      &cSubKeys,               // number of subkeys
			      &cbMaxSubKey,            // longest subkey size
			      &cchMaxClass,            // longest class string
			      &cValues,                // number of values for this key
			      &cchMaxValue,            // longest value name
			      &cbMaxValueData,         // longest value data
			      &cbSecurityDescriptor,   // security descriptor
			      &ftLastWriteTime);       // last write time

    // Enumerate the subkeys, until RegEnumKeyEx fails.

    if (cSubKeys)
      {
	//std::cout << "Number of subkeys: " << cSubKeys <<std::endl;
	bool found = false;
	i = 0;
	while ((!found) && (i<cSubKeys))
	  {
	    cbName = MAX_KEY_LENGTH;
	    retCode = RegEnumKeyEx(hKey, i,
				   achKey,
				   &cbName,
				   NULL,
				   NULL,
				   NULL,
				   &ftLastWriteTime);
	    if (retCode == ERROR_SUCCESS)
	      {
    winDisplayInfo dinfo;
		getDisplayInfo(achKey,&dinfo);
		if ((dinfo.w > 0) && (dinfo.h > 0)) {
		  found = true;
		  res =  achKey;
		}
	      }
	    i++;
	  }
      }
    RegCloseKey(hKey);
    return res;
  }

  std::list<std::wstring> getAllKeys()
  {
    std::list<std::wstring> result;
    HKEY hKey;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Enum\\DISPLAY"), 0, KEY_READ, &hKey);

    TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
    DWORD    cbName;                   // size of name string
    TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name
    DWORD    cchClassName = MAX_PATH;  // size of class string
    DWORD    cSubKeys=0;               // number of subkeys
    DWORD    cbMaxSubKey;              // longest subkey size
    DWORD    cchMaxClass;              // longest class string
    DWORD    cValues;              // number of values for key
    DWORD    cchMaxValue;          // longest value name
    DWORD    cbMaxValueData;       // longest value data
    DWORD    cbSecurityDescriptor; // size of security descriptor
    FILETIME ftLastWriteTime;      // last write time

    DWORD i, retCode;

    // Get the class name and the value count.
    retCode = RegQueryInfoKey(
          hKey,                    // key handle
          achClass,                // buffer for class name
          &cchClassName,           // size of class string
          NULL,                    // reserved
          &cSubKeys,               // number of subkeys
          &cbMaxSubKey,            // longest subkey size
          &cchMaxClass,            // longest class string
          &cValues,                // number of values for this key
          &cchMaxValue,            // longest value name
          &cbMaxValueData,         // longest value data
          &cbSecurityDescriptor,   // security descriptor
          &ftLastWriteTime);       // last write time

    // Enumerate the subkeys, until RegEnumKeyEx fails.

    if (cSubKeys)
    {
      //std::cout << "Number of subkeys: " << cSubKeys <<std::endl;

      for (i=0; i<cSubKeys; i++)
      {
        cbName = MAX_KEY_LENGTH;
        retCode = RegEnumKeyEx(hKey, i,
                               achKey,
                               &cbName,
                               NULL,
                               NULL,
                               NULL,
                               &ftLastWriteTime);
        if (retCode == ERROR_SUCCESS)
          result.push_back(achKey);
      }
    }
    RegCloseKey(hKey);
    return result;
  }

  void listDisplays(std::wostream& out) {
    out << "List of all displays connected one day to your computer:" << std::endl;
    std::list<std::wstring> allKeys = getAllKeys();
    for (std::list<std::wstring>::iterator it = allKeys.begin(); it != allKeys.end(); it++)
    {
      std::wstring achKey = *it;
      winDisplayInfo dinfo;
      getDisplayInfo(achKey,&dinfo);
      if ((dinfo.w > 0) && (dinfo.h > 0))
        out << "windisplay:/" << achKey << " [" << dinfo.monitorName
            << ", " << dinfo.w << "x" << dinfo.h << " mm"
            << ", " << dinfo.resx << "x" << dinfo.resy << " pixels"
            << ", " << dinfo.ppi << " PPI"
            << ", " << dinfo.refreshRate << " Hz]" << std::endl;
    }
  }

  std::wstring getMatchingRegistryKey(std::wstring displayName) {
    std::wstring res = L"";
    HKEY hKey;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Enum\\DISPLAY"), 0, KEY_READ, &hKey);

    TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
    DWORD    cbName;                   // size of name string
    TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name
    DWORD    cchClassName = MAX_PATH;  // size of class string
    DWORD    cSubKeys=0;               // number of subkeys
    DWORD    cbMaxSubKey;              // longest subkey size
    DWORD    cchMaxClass;              // longest class string
    DWORD    cValues;              // number of values for key
    DWORD    cchMaxValue;          // longest value name
    DWORD    cbMaxValueData;       // longest value data
    DWORD    cbSecurityDescriptor; // size of security descriptor
    FILETIME ftLastWriteTime;      // last write time

    DWORD i, retCode;

    // Get the class name and the value count.
    retCode = RegQueryInfoKey(
			      hKey,                    // key handle
			      achClass,                // buffer for class name
			      &cchClassName,           // size of class string
			      NULL,                    // reserved
			      &cSubKeys,               // number of subkeys
			      &cbMaxSubKey,            // longest subkey size
			      &cchMaxClass,            // longest class string
			      &cValues,                // number of values for this key
			      &cchMaxValue,            // longest value name
			      &cbMaxValueData,         // longest value data
			      &cbSecurityDescriptor,   // security descriptor
			      &ftLastWriteTime);       // last write time

    // Enumerate the subkeys, until RegEnumKeyEx fails.
    if (cSubKeys)
      {
	//std::cout << "Number of subkeys: " << cSubKeys <<std::endl;

	for (i=0; i<cSubKeys; i++)
	  {
	    cbName = MAX_KEY_LENGTH;
	    retCode = RegEnumKeyEx(hKey, i,
				   achKey,
				   &cbName,
				   NULL,
				   NULL,
				   NULL,
				   &ftLastWriteTime);
	    if (retCode == ERROR_SUCCESS)
	      {
    winDisplayInfo dinfo;
		getDisplayInfo(achKey,&dinfo);
		if ((dinfo.w > 0) && (dinfo.h > 0))
		  if (matchStrings(displayName, std::string(dinfo.monitorName))) res = achKey;
	      }
	  }
      }
    RegCloseKey(hKey);
    return res;
  }

  bool matchStrings(std::wstring nameControlPanel, std::string nameEDID) {
    std::string nameCP;
    nameCP.assign(nameControlPanel.begin(), nameControlPanel.end());
    std::transform(nameCP.begin(), nameCP.end(), nameCP.begin(), toupper);

    // Remove spaces at the end of nameEDID
	if (nameEDID.length() > 0)
	{
		while (nameEDID[ nameEDID.length()-1] == ' ')
		  nameEDID.erase(nameEDID.length()-1,1);
	}

    std::transform(nameEDID.begin(), nameEDID.end(), nameEDID.begin(), toupper);

    size_t found;
    found=nameCP.find(nameEDID);
    if (found!=std::string::npos)
      return true;
    else
      return false;
  }
}
