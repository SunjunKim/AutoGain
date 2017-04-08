/* -*- mode: c++ -*-
 *
 * pointing/input/windows/winPointingDeviceManager.cpp --
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

#include <pointing/input/windows/winPointingDeviceManager.h>
#include <pointing/input/windows/winPointingDevice.h>
#include <pointing/input/windows/USB.h>
#include <math.h>

using namespace std;

namespace pointing
{
    float roundf(float x)
    {
         return x >= 0.0f ? floorf(x + 0.5f) : ceilf(x - 0.5f);
    }

    URI uriForHandle(HANDLE h)
    {
      URI result;
      if (h)
      {
        result.scheme = "winhid";
        result.path = "/" + std::to_string(PtrToUint(h));
      }
      else
        result.scheme = "any";
      return result;
    }

    bool winPointingDeviceManager::fillDescriptorInfo(HANDLE h, PointingDeviceDescriptor &desc)
    {
        int vendorID = -1, productID = -1;
        string vendor, product;
        bool result = getMouseNameFromDevice(h, vendor, product, &vendorID, &productID);
        if (vendorID != -1)
            desc.vendorID = vendorID;
        if (productID != -1)
            desc.productID = productID;
        desc.vendor = vendor;
        desc.product = product;
        desc.devURI = uriForHandle(h);
        return result;
    }

    winPointingDeviceManager::winPointingDeviceManager()
    {
        hThreads[0]=CreateThread(NULL, 0, Loop, LPVOID(this), 0, &dwThreadId);
        while(run==THREAD_UNDEFINED){Sleep(10);}
    }

    winPointingDeviceManager::~winPointingDeviceManager()
    {
        run = THREAD_TERMINATING;
        while(run != THREAD_HALTED){Sleep(100);}
        //DestroyWindow(msghwnd_);
    }

  void winPointingDeviceManager::processMessage(MSG *msg)
  {
    TranslateMessage(msg);
    DispatchMessage(msg);
  }

  DWORD WINAPI winPointingDeviceManager::Loop(LPVOID lpvThreadParam)
  {
    winPointingDeviceManager* self = static_cast<winPointingDeviceManager *>(lpvThreadParam);
    self->msghwnd_=self->rawInputInit();
    MSG msg;

    // This while loop makes sure that all connected devices
    // are registered, only then we allow running the main loop
    // It is necessary to avoid the case when a specified device is not
    // found in the dispatcher at the beginning of the program.
    while (PeekMessage(&msg, self->msghwnd_, 0, 0, PM_REMOVE))
    {
      self->processMessage(&msg);
      if (msg.message != WM_INPUT_DEVICE_CHANGE)
        break;
    }
    self->run = THREAD_RUNNING;

    while (1)
      if ( GetMessage(&msg, self->msghwnd_, 0, 0) )
        self->processMessage(&msg);

    self->run=THREAD_HALTED;
    return 0;
  }

  HWND winPointingDeviceManager::rawInputInit()
  {
    HWND tempHwnd = 0;
    WNDCLASSEX w;
    memset(&w, 0, sizeof(w));
    w.cbSize = sizeof(w);
    w.lpfnWndProc = (WNDPROC)winPointingDeviceManager::rawInputProc;
    w.lpszClassName = L"MyServiceWindowClass";
    ATOM atom = ::RegisterClassEx(&w);
    if (!atom){ throw std::runtime_error("Unable to register a new windows class for message processing"); }
    tempHwnd=CreateWindow( w.lpszClassName,
                           L"",
                           WS_BORDER | WS_CAPTION,
                           0, 0, 0, 0,
                           HWND_MESSAGE ,
                           NULL,
                           NULL,
                           NULL);
    if(!tempHwnd){ throw std::runtime_error("Unable to create a message window"); }
    SetWindowLongPtr(tempHwnd, GWLP_USERDATA, (LONG_PTR)this);

    RAWINPUTDEVICE Rid[1];
    Rid[0].usUsagePage = 0x01;
    Rid[0].usUsage = 0x02;
    Rid[0].dwFlags = RIDEV_INPUTSINK |  RIDEV_DEVNOTIFY ;   // adds HID mouse
    Rid[0].hwndTarget = tempHwnd;

    //tempHwnd = GetConsoleWindow();

    if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE) {
      throw std::runtime_error("Unable to register this window to rawinput.");
    }

    return tempHwnd;
  }

  bool winPointingDeviceManager::relativeDisplacement(const PRAWMOUSE pmouse, winPointingDevice *dev, int *dx, int *dy)
  {
    if (pmouse->usFlags & MOUSE_MOVE_ABSOLUTE)
    {
      const bool virtualDesktop = pmouse->usFlags & MOUSEEVENTF_VIRTUALDESK;
      const int width = GetSystemMetrics(virtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
      const int height = GetSystemMetrics(virtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
      const float dpx = USHRT_MAX / width; // One pixel displacement
      const float dpy = USHRT_MAX / height;

      if (dev->lastX > 0 || dev->lastY > 0)
      {
        *dx = roundf((pmouse->lLastX - dev->lastX) / dpx);
        *dy = roundf((pmouse->lLastY - dev->lastY) / dpy);
      }
      // Save last values for the next callback
      dev->lastX = pmouse->lLastX;
      dev->lastY = pmouse->lLastY;
      // Always return true for the virtual mouse
      // Since in this case movement of a mouse for small displacements
      // may produce 0, 0 because display is taken into account
      // So at least we know that mouse has moved
      return true;
    }
    *dx = pmouse->lLastX;
    *dy = pmouse->lLastY;
    return *dx || *dy;
  }

  void winPointingDeviceManager::processMatching(PointingDeviceManager::PointingDeviceData *, SystemPointingDevice *)
  {
  }

  // Static function that process the rawinput events and let the others processed by the default processor.
  // hwnd field stores the window handler, uMsg is the event type, wParam and lParam are additional
  // parameters.
  LONG APIENTRY winPointingDeviceManager::rawInputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    // Check if we have received a rawinput event
    // This can be either insertion/removal of a device or a message from one of them.
    if(uMsg==WM_INPUT_DEVICE_CHANGE)
    {
      // We have stored in the GWL_USERDATA a pointer to the winPointingDevice. This object
      // is needed to route the event to the user provided callback and context.
      winPointingDeviceManager* self=(winPointingDeviceManager*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

      switch(wParam)
      {
      case GIDC_ARRIVAL:
      {
          PointingDeviceDescriptor desc;
          if (self->fillDescriptorInfo((HANDLE)lParam, desc))
          {
              PointingDeviceData *pdd = new PointingDeviceData;
              pdd->desc = desc;
              self->registerDevice((HANDLE)lParam, pdd);
          }
          break;
      }
      case GIDC_REMOVAL:
          self->unregisterDevice((HANDLE)lParam);
          break;
      }

      return 0;
    }
    else if(uMsg==WM_INPUT)
    {
      UINT dwSize = 0;

      // We have stored in the GWL_USERDATA a pointer to the winPointingDevice. This object
      // is needed to route the event to the user provided callback and context.
      winPointingDeviceManager* self=(winPointingDeviceManager*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

      // Retreive the raw input data...
      GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize,
                      sizeof(RAWINPUTHEADER));
      LPBYTE lpb = new BYTE[dwSize];
      if (lpb == NULL)
        return 0;

      if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize,
                          sizeof(RAWINPUTHEADER)) != dwSize)
        throw std::runtime_error("GetRawInputData does not return correct size !\n");
      //ON_ERROR("GetRawInputData does not return correct size !\n");

      RAWINPUT* raw = (RAWINPUT*)lpb;
      if (raw->header.dwType == RIM_TYPEMOUSE)
      {
        //std::cout << "Input frame  from: " << std::hex << raw->header.hDevice << std::endl;
        auto it = self->devMap.find(raw->header.hDevice);
        if(it != self->devMap.end())
        {
          TimeStamp::inttime now = TimeStamp::createAsInt();
          PointingDeviceData *pdd = static_cast<PointingDeviceData *>(it->second);
          for (SystemPointingDevice *device : pdd->pointingList)
          {
            winPointingDevice *dev = static_cast<winPointingDevice *>(device);
            // To prevent calling the callback function for simple touchs
            // we verify that there is a button clicked or a displacement
            // Otherwise for some touchpads the callback is called even if you
            // hold your finger on the touchpad.
            int dx = 0, dy = 0;
            if (raw->data.mouse.usButtonFlags || self->relativeDisplacement(&raw->data.mouse, dev, &dx, &dy))
            {
              if(raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
                dev->buttons |= 1 << 0;
              if(raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
                dev->buttons &= ~(1 << 0);

              if(raw->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
                dev->buttons |= 1 << 1;
              if(raw->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
                dev->buttons &= ~(1 << 1);

              if(raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
                dev->buttons |= 1 << 2;
              if(raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
                dev->buttons &= ~(1 << 2);

              dev->registerTimestamp(now, dx, dy);

              if (dev->callback != NULL)
                dev->callback(dev->callback_context, now, dx, dy, dev->buttons);
            }
          }
        }
      }
      delete[] lpb;
      return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }
}
