/* -*- mode: c++ -*-
 *
 * pointing/input/windows/USB.cpp --
 *
 * Initial software
 * Authors: Damien Marchal
 * Copyright © INRIA
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/input/windows/USB.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>

extern "C"
{
#include <SetupAPI.h>
#include <hidsdi.h>
}

namespace pointing {

  struct DeviceId
  {
    unsigned int vid;
    unsigned int pid;

    DeviceId(unsigned int vid, unsigned int pid):vid(vid),pid(pid){}

    bool operator < (const DeviceId& rhs) const
    {
        if (vid < rhs.vid) return true;
        if (vid > rhs.vid) return false;

        return pid < rhs.pid;
    }
  };

  struct DeviceName
  {
    std::string vendor;
    std::string product;
  };

  std::map<DeviceId, DeviceName> deviceMap;

  void FindHIDDevice(unsigned int vid, unsigned int pid, std::string &vendor, std::string &product)
  {
    // We will save all the found ids into the map
    // So that we can return the result immediately
    // If the required device ids were not found we enumerate all the HID devices
    DeviceId did(vid, pid);
    std::map<DeviceId, DeviceName>::iterator it = deviceMap.find(did);
    if (it != deviceMap.end())
    {
      vendor = it->second.vendor;
      product = it->second.product;
      return;
    }

    GUID hidGuid;
    SP_INTERFACE_DEVICE_DATA deviceInterfaceData;
    HIDD_ATTRIBUTES thisHidDevice;

    unsigned long bytesReturned;

    char vendorName[256];
    char productName[256];

    char temp[2]; //for conversion UNICODE to String
    unsigned char buffer[253]; //don't use more than 253 bytes

    // First, get my class identifier
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO PnPHandle = SetupDiGetClassDevs(&hidGuid, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if(PnPHandle != INVALID_HANDLE_VALUE)
    {
      bool Result;
      DWORD memberIndex = 0;
      while (1)
      {
        ZeroMemory(&deviceInterfaceData,sizeof(deviceInterfaceData));
        deviceInterfaceData.cbSize = sizeof(deviceInterfaceData); // Length of data structure in bytes
        // Call this each time, strange behavior
        // Each time SetupDiGetDeviceInterfaceDetail gets called hidGuid changes
        HidD_GetHidGuid(&hidGuid);
        Result = SetupDiEnumDeviceInterfaces(PnPHandle, 0, &hidGuid, memberIndex, &deviceInterfaceData);
        if (!Result)
        {
          DWORD last_error = GetLastError();
          if (last_error == ERROR_NO_MORE_ITEMS)
            break;
          else
            std::cerr << "Error in FindHIDDevice with the code " << last_error << std::endl;
        }
        memberIndex++;

        // First called is needed to obtain the size of the structure for allocation
        Result = SetupDiGetDeviceInterfaceDetail(PnPHandle, &deviceInterfaceData, NULL, 0, &bytesReturned, 0);
        PSP_INTERFACE_DEVICE_DETAIL_DATA piddd = (PSP_INTERFACE_DEVICE_DETAIL_DATA)calloc(bytesReturned, 1);
        piddd->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
        Result = SetupDiGetDeviceInterfaceDetail(PnPHandle, &deviceInterfaceData, piddd, bytesReturned, 0, 0);
        if (!Result)
        {
          std::cerr << "Could not find the system name for this HID device" << std::endl;
          free(piddd);
          continue;
        }

        // Can now open this HID device
        HANDLE hidHandle = CreateFile(piddd->DevicePath,
          GENERIC_WRITE,
          FILE_SHARE_READ|FILE_SHARE_WRITE,
          NULL, OPEN_EXISTING,
          0, NULL);

        free(piddd);

        if(hidHandle == INVALID_HANDLE_VALUE)
        {
          std::cerr << "Invalid handle value for hid device" << std::endl;
          CloseHandle(hidHandle);
          continue;
        }

        // Get VID and PID
        Result = HidD_GetAttributes(hidHandle, &thisHidDevice);

        // Get Manufacturers name if present
        if(HidD_GetManufacturerString(hidHandle, buffer, sizeof(buffer)))
        {
          int i = 0;
          temp[1]=0;
          strcpy_s(vendorName,"\0");
          while (buffer[i] != 0) // Convert from UNICODE to String
          {
            temp[0]=buffer[i];
            strcat_s(vendorName,temp);
            i = i + 2;
          }
        } else strcpy_s(vendorName,"- unknown -");

        // Get Product name if present
        if(HidD_GetProductString(hidHandle, buffer, sizeof(buffer)))
        {
          int i = 0;
          temp[1]=0;
          strcpy_s(productName,"\0");
          while (buffer[i] != 0) // Convert from UNICODE to String
          {
            temp[0]=buffer[i];
            strcat_s(productName,temp);
            i = i + 2;
          }
        } else strcpy_s(productName,"- unknown -");

        //show all existing HID devices
        //delete or comment it if you don't like this
        /*fprintf(stdout,"VID: 0x%04X PID: 0x%04X\n"
          "VendorName: %s  ProductName: %s\n\n"
          ,thisHidDevice.VendorID
          ,thisHidDevice.ProductID
          ,vendorName
          ,productName
          );
        */

        // Save the results into the map in order not to search each time
        DeviceName dname;
        dname.product = productName;
        dname.vendor = vendorName;
        DeviceId newDid(thisHidDevice.VendorID, thisHidDevice.ProductID);
        deviceMap[newDid] = dname;

        // is this my HID device ?
        if(thisHidDevice.VendorID==vid && thisHidDevice.ProductID==pid)
        {
          vendor = vendorName;
          product = productName;
        }
        if(hidHandle) { CloseHandle(hidHandle); hidHandle=NULL; }
      }
    }
    SetupDiDestroyDeviceInfoList(PnPHandle);
  }

  bool getMouseNameFromDevice(HANDLE h, std::string &vendor, std::string &product, int *outVendorID, int *outProductID)
  {
      RID_DEVICE_INFO deviceinfo;

      // First get the device name ...
      UINT pcbSize = 0;
      if( GetRawInputDeviceInfoA(h, RIDI_DEVICENAME, NULL, &pcbSize) != 0 ){
        throw std::runtime_error("Unable to retreive the size of the device name ");
      }
      std::auto_ptr<CHAR> buffer(new CHAR[pcbSize+1]);
      if(GetRawInputDeviceInfoA(h, RIDI_DEVICENAME, buffer.get(), &pcbSize) <= 0 ){
        throw std::runtime_error("Unable to retreive the name of the device ");
      }
      std::string devicename=buffer.get();

      // FIXME...RDP_MOUSE is a virtual device used by the the Remote Desktop Protocole
      // Uncomment the following if you want to see RDP mouse as device.
      if(devicename.find("RDP_MOUSE")!=std::string::npos) return false;

      // FIXME
      // My Trackpad seems to be visible as an ACPI device, maybe it is the way by which a PS/2
      // device is exposed through windows.
      if(devicename.find("ACPI")!=std::string::npos){
        product = "ACPI/PS2 device (Trackpad ?)";
        vendor = "unknown";
        return true;
      }

      if(devicename.find("HID")==std::string::npos) return false;

      deviceinfo.dwType=12345;
      deviceinfo.cbSize=sizeof(RID_DEVICE_INFO);
      if(GetRawInputDeviceInfo(h, RIDI_DEVICEINFO, NULL, &pcbSize)){
        throw std::runtime_error("Unable to retreive the number of bytes to stores the device informations.");
      }

      if(GetRawInputDeviceInfo(h, RIDI_DEVICEINFO, &deviceinfo, &pcbSize)<=0){
        throw std::runtime_error("Unable to retreive the device information for this device.");
      }

      if(deviceinfo.dwType!=RIM_TYPEMOUSE)return false;

      int productID, vendorID;
      int vid=devicename.find("VID_", 0)+4;
      int pid=devicename.find("PID_", 0)+4;

      std::stringstream svid(devicename.substr(vid, vid+4));
      svid>>std::hex>>vendorID;
      if (outVendorID) *outVendorID = vendorID;

      std::stringstream spid(devicename.substr(pid, pid+4));
      spid>>std::hex>>productID;
      if (outProductID) *outProductID = productID;

      FindHIDDevice(*outVendorID, *outProductID, vendor, product);
      return true;
  }
}
