using System;
using System.Collections.Generic;
using System.Linq;
using System.Diagnostics;
using System.Text;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Collections;
using Microsoft.Win32;
using System.Text.RegularExpressions;

namespace MouseTester
{
    class RawMouse
    {
        #region const definitions

        // The following constants are defined in Windows.h

        private const int RIDEV_INPUTSINK = 0x00000100;
        private const int RIDEV_CAPTUREMOUSE = 0x00000200;
        private const int RIDEV_NOLEGACY = 0x00000030;
        
        private const int RID_INPUT = 0x10000003;

        private const int FAPPCOMMAND_MASK = 0xF000;
        private const int FAPPCOMMAND_MOUSE = 0x8000;
        private const int FAPPCOMMAND_OEM = 0x1000;

        private const int RIM_TYPEMOUSE = 0;
        private const int RIM_TYPEKEYBOARD = 1;
        private const int RIM_TYPEHID = 2;

        private const int RIDI_DEVICENAME = 0x20000007;
        private const uint RIDI_DEVICEINFO = 0x2000000b;

        private const int WM_KEYDOWN = 0x0100;
        private const int WM_SYSKEYDOWN = 0x0104;
        private const int WM_INPUT = 0x00FF;
        private const int VK_OEM_CLEAR = 0xFE;
        private const int VK_LAST_KEY = VK_OEM_CLEAR; // this is a made up value used as a sentinel
        
        private const int RI_MOUSE_LEFT_BUTTON_DOWN = 0x0001;
        private const ushort RI_MOUSE_LEFT_BUTTON_UP = 0x0002;
        private const int RI_MOUSE_RIGHT_BUTTON_DOWN = 0x0004;
        private const ushort RI_MOUSE_RIGHT_BUTTON_UP = 0x0008;

        #endregion const definitions 

        /// <summary>
        /// Class encapsulating the information about a
        /// keyboard event, including the device it
        /// originated with and what key was pressed
        /// </summary>
        public class DeviceInfo
        {
            public string deviceName;
            public string deviceType;
            public IntPtr deviceHandle;
            public string Name;
            public string source;
            public ushort key;
            public string vKey;
        }

        #region Windows.h structure declarations

        // The following structures are defined in Windows.h

        [StructLayout(LayoutKind.Sequential)]
        internal struct RAWINPUTDEVICELIST
        {
            public IntPtr hDevice;
            [MarshalAs(UnmanagedType.U4)]
            public int dwType;
        }

        [StructLayout(LayoutKind.Explicit)]
        internal struct RAWINPUT
        {
            [FieldOffset(0)]
            public RAWINPUTHEADER header;
            [FieldOffset(16)]
            public RAWMOUSE mouse;
            [FieldOffset(16)]
            public RAWKEYBOARD keyboard;
            [FieldOffset(16)]
            public RAWHID hid;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct RAWINPUTHEADER
        {
            [MarshalAs(UnmanagedType.U4)]
            public int dwType;
            [MarshalAs(UnmanagedType.U4)]
            public int dwSize;
            public IntPtr hDevice;
            [MarshalAs(UnmanagedType.U4)]
            public int wParam;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct RAWHID
        {
            [MarshalAs(UnmanagedType.U4)]
            public int dwSizHid;
            [MarshalAs(UnmanagedType.U4)]
            public int dwCount;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct BUTTONSSTR
        {
            [MarshalAs(UnmanagedType.U2)]
            public ushort usButtonFlags;
            [MarshalAs(UnmanagedType.U2)]
            public ushort usButtonData;
        }

        [StructLayout(LayoutKind.Explicit)]
        internal struct RAWMOUSE
        {
            [MarshalAs(UnmanagedType.U2)]
            [FieldOffset(0)]
            public ushort usFlags;
            [MarshalAs(UnmanagedType.U4)]
            [FieldOffset(4)]
            public uint ulButtons;
            [FieldOffset(4)]
            public BUTTONSSTR buttonsStr;
            [MarshalAs(UnmanagedType.U4)]
            [FieldOffset(8)]
            public uint ulRawButtons;
            [FieldOffset(12)]
            public int lLastX;
            [FieldOffset(16)]
            public int lLastY;
            [MarshalAs(UnmanagedType.U4)]
            [FieldOffset(20)]
            public uint ulExtraInformation;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct RAWKEYBOARD
        {
            [MarshalAs(UnmanagedType.U2)]
            public ushort MakeCode;
            [MarshalAs(UnmanagedType.U2)]
            public ushort Flags;
            [MarshalAs(UnmanagedType.U2)]
            public ushort Reserved;
            [MarshalAs(UnmanagedType.U2)]
            public ushort VKey;
            [MarshalAs(UnmanagedType.U4)]
            public uint Message;
            [MarshalAs(UnmanagedType.U4)]
            public uint ExtraInformation;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct RAWINPUTDEVICE
        {
            [MarshalAs(UnmanagedType.U2)]
            public ushort usUsagePage;
            [MarshalAs(UnmanagedType.U2)]
            public ushort usUsage;
            [MarshalAs(UnmanagedType.U4)]
            public int dwFlags;
            public IntPtr hwndTarget;
        }

        public enum RawInputDeviceInformationCommand : int
        {
            /// <summary>
            /// pData points to a string that contains the device name. For this uiCommand only, the value in pcbSize is the character count (not the byte count).
            /// </summary>
            RIDI_DEVICENAME = 0x20000007,
            /// <summary>
            /// pData points to an RID_DEVICE_INFO structure.
            /// </summary>
            RIDI_DEVICEINFO = 0x2000000b,
            /// <summary>
            /// pData points to the previously parsed data.
            /// </summary>
            RIDI_PREPARSEDDATA = 0x20000005
        }

        [StructLayout(LayoutKind.Explicit)]
        public struct RID_DEVICE_INFO
        {
            /// <summary>
            /// The size, in bytes, of the RID_DEVICE_INFO structure. 
            /// </summary>
            [FieldOffset(0)]
            public int cbSize;
            /// <summary>
            /// The type of raw input data.
            /// </summary>
            [FieldOffset(4)]
            public RawInputDeviceType dwType;
            /// <summary>
            /// If dwType is RIM_TYPEMOUSE, this is the RID_DEVICE_INFO_MOUSE structure that defines the mouse. 
            /// </summary>
            [FieldOffset(8)]
            public RID_DEVICE_INFO_MOUSE mouse;
            /// <summary>
            /// If dwType is RIM_TYPEKEYBOARD, this is the RID_DEVICE_INFO_KEYBOARD structure that defines the keyboard. 
            /// </summary>
            [FieldOffset(8)]
            public RID_DEVICE_INFO_KEYBOARD keyboard;
            /// <summary>
            /// If dwType is RIM_TYPEHID, this is the RID_DEVICE_INFO_HID structure that defines the HID device. 
            /// </summary>
            [FieldOffset(8)]
            public RID_DEVICE_INFO_HID hid;
        }
        /// <summary>
        /// Defines the raw input data coming from the specified mouse.
        /// </summary>
        /// <remarks>http://msdn.microsoft.com/en-us/library/windows/desktop/ms645589%28v=vs.85%29.aspx</remarks>
        [StructLayout(LayoutKind.Sequential)]
        public struct RID_DEVICE_INFO_MOUSE
        {
            /// <summary>
            /// The identifier of the mouse device.
            /// </summary>
            public int dwId;
            /// <summary>
            /// The number of buttons for the mouse.
            /// </summary>
            public int dwNumberOfButtons;
            /// <summary>
            /// The number of data points per second. This information may not be applicable for every mouse device.
            /// </summary>
            public int dwSampleRate;
            /// <summary>
            /// TRUE if the mouse has a wheel for horizontal scrolling; otherwise, FALSE.
            /// Windows XP:  This member is only supported starting with Windows Vista.
            /// </summary>
            public bool fHasHorizontalWheel;
        }
        /// <summary>
        /// Defines the raw input data coming from the specified keyboard. 
        /// </summary>
        /// <remarks>http://msdn.microsoft.com/en-us/library/windows/desktop/ms645587%28v=vs.85%29.aspx</remarks>
        [StructLayout(LayoutKind.Sequential)]
        public struct RID_DEVICE_INFO_KEYBOARD
        {
            /// <summary>
            /// The type of the keyboard. 
            /// </summary>
            public int dwType;
            /// <summary>
            /// The subtype of the keyboard. 
            /// </summary>
            public int dwSubType;
            /// <summary>
            /// The scan code mode. 
            /// </summary>
            public int dwKeyboardMode;
            /// <summary>
            /// The number of function keys on the keyboard.
            /// </summary>
            public int dwNumberOfFunctionKeys;
            /// <summary>
            /// The number of LED indicators on the keyboard.
            /// </summary>
            public int dwNumberOfIndicators;
            /// <summary>
            /// The total number of keys on the keyboard. 
            /// </summary>
            public int dwNumberOfKeysTotal;
        }
        /// <summary>
        /// Defines the raw input data coming from the specified Human Interface Device (HID). 
        /// </summary>
        /// <remarks>http://msdn.microsoft.com/en-us/library/windows/desktop/ms645584%28v=vs.85%29.aspx</remarks>
        [StructLayout(LayoutKind.Sequential)]
        public struct RID_DEVICE_INFO_HID
        {
            /// <summary>
            /// The vendor identifier for the HID. 
            /// </summary>
            public int dwVendorId;
            /// <summary>
            /// The product identifier for the HID. 
            /// </summary>
            public int dwProductId;
            /// <summary>
            /// The version number for the HID. 
            /// </summary>
            public int dwVersionNumber;
            /// <summary>
            /// The top-level collection Usage Page for the device. 
            /// </summary>
            public ushort usUsagePage;
            /// <summary>
            /// The top-level collection Usage for the device. 
            /// </summary>
            public ushort usUsage;
        }
        /// <summary>
        /// The type of raw input data.
        /// </summary>
        public enum RawInputDeviceType : int
        {
            /// <summary>
            /// Data comes from a mouse.
            /// </summary>
            RIM_TYPEMOUSE = 0,
            /// <summary>
            /// Data comes from a keyboard.
            /// </summary>
            RIM_TYPEKEYBOARD = 1,
            /// <summary>
            /// Data comes from an HID that is not a keyboard or a mouse.
            /// </summary>
            RIM_TYPEHID = 2,
        }
        #endregion Windows.h structure declarations

        #region User32 dll imports
        [DllImport("user32.dll")]
        extern static bool RegisterRawInputDevices(RAWINPUTDEVICE[] pRawInputDevices,
                                                   uint uiNumDevices,
                                                   uint cbSize);

        [DllImport("User32.dll")]
        extern static uint GetRawInputData(IntPtr hRawInput,
                                           uint uiCommand,
                                           IntPtr pData,
                                           ref uint pcbSize,
                                           uint cbSizeHeader);

        [DllImport("User32.dll")]
        extern static uint GetRawInputDeviceInfo(IntPtr hDevice, uint uiCommand, IntPtr pData, ref uint pcbSize);

        [DllImport("User32.dll")]
        extern static uint GetRawInputDeviceList(IntPtr pRawInputDeviceList, ref uint uiNumDevices, uint cbSize);
        #endregion

        /// <summary>
        /// List of mouse devices. 
        /// Key: the device handle
        /// Value: the device info class
        /// </summary>
        private Hashtable deviceList = new Hashtable();

        private Stopwatch stopWatch = new Stopwatch();
        public double stopwatch_freq = 0.0;

        public void RegisterRawInputMouse(IntPtr hwnd)
        {
            RAWINPUTDEVICE[] rid = new RAWINPUTDEVICE[1];
            rid[0].usUsagePage = 1;
            rid[0].usUsage = 2;
            rid[0].dwFlags = RIDEV_INPUTSINK;
            rid[0].hwndTarget = hwnd;

            if (!RegisterRawInputDevices(rid, (uint)rid.Length, (uint)Marshal.SizeOf(rid[0])))
            {
                Debug.WriteLine("RegisterRawInputDevices() Failed");
            }

            Debug.WriteLine("High Resolution Stopwatch: " + Stopwatch.IsHighResolution + "\n" +
                            "Stopwatch TS: " + (1e6 / Stopwatch.Frequency).ToString() + " us\n" +
                            "Stopwatch Hz: " + (Stopwatch.Frequency / 1e6).ToString() + " MHz\n");

            this.stopwatch_freq = 1e3 / Stopwatch.Frequency;
        }

        #region int EnumerateDevices()

        /// <summary>
        /// Iterates through the list provided by GetRawInputDeviceList,
        /// counting keyboard devices and adding them to deviceList.
        /// </summary>
        /// <returns>The number of keyboard devices found.</returns>
        public int EnumerateDevices()
        {
            deviceList.Clear();
            int NumberOfDevices = 0;
            uint deviceCount = 0;
            int dwSize = (Marshal.SizeOf(typeof(RAWINPUTDEVICELIST)));

            // Get the number of raw input devices in the list,
            // then allocate sufficient memory and get the entire list
            if (GetRawInputDeviceList(IntPtr.Zero, ref deviceCount, (uint)dwSize) == 0)
            {
                IntPtr pRawInputDeviceList = Marshal.AllocHGlobal((int)(dwSize * deviceCount));
                GetRawInputDeviceList(pRawInputDeviceList, ref deviceCount, (uint)dwSize);

                // Iterate through the list, discarding undesired items
                // and retrieving further information on keyboard devices
                for (int i = 0; i < deviceCount; i++)
                {
                    DeviceInfo dInfo;
                    string deviceName;
                    uint pcbSize = 0;

                    RAWINPUTDEVICELIST rid = (RAWINPUTDEVICELIST)Marshal.PtrToStructure(
                                               new IntPtr((pRawInputDeviceList.ToInt32() + (dwSize * i))),
                                               typeof(RAWINPUTDEVICELIST));

                    GetRawInputDeviceInfo(rid.hDevice, RIDI_DEVICENAME, IntPtr.Zero, ref pcbSize);

                    if (pcbSize > 0)
                    {
                        IntPtr pData = Marshal.AllocHGlobal((int)pcbSize);
                        GetRawInputDeviceInfo(rid.hDevice, RIDI_DEVICENAME, pData, ref pcbSize);
                        deviceName = (string)Marshal.PtrToStringAnsi(pData);

                        // Drop the "root" keyboard and mouse devices used for Terminal 
                        // Services and the Remote Desktop
                        if (deviceName.ToUpper().Contains("ROOT"))
                        {
                            continue;
                        }

                        // add mouse device and retrive VID/PID for future reference
                        if(rid.dwType == RIM_TYPEMOUSE)
                        {
                            dInfo = new DeviceInfo();

                            dInfo.deviceName = (string)Marshal.PtrToStringAnsi(pData);
                            dInfo.deviceHandle = rid.hDevice;
                            dInfo.deviceType = GetDeviceType(rid.dwType);
                            string DeviceDesc = ReadReg(deviceName);
                            dInfo.Name = DeviceDesc;
                            
                            var exp = new Regex(@"VID_(?<Vid>[0-9A-F]+)&PID_(?<Pid>[0-9A-F]+)");
                            var match = exp.Match(dInfo.deviceName);
                            String id = match.Groups["Vid"].ToString() + "_" + match.Groups["Pid"].ToString();
                            dInfo.source = id;

                            Console.WriteLine("{0},{1},{2}", dInfo.deviceName, id, dInfo.deviceHandle);

                            if (!deviceList.Contains(rid.hDevice))
                            {
                                NumberOfDevices++;
                                deviceList.Add(rid.hDevice, dInfo);
                            }
                        }

                        Marshal.FreeHGlobal(pData);
                    }
                }
                Marshal.FreeHGlobal(pRawInputDeviceList);

                return NumberOfDevices;

            }
            else
            {
                throw new ApplicationException("An error occurred while retrieving the list of devices.");
            }

        }

        #endregion EnumerateDevices()

        #region GetDeviceType( int device )

        /// <summary>
        /// Converts a RAWINPUTDEVICELIST dwType value to a string
        /// describing the device type.
        /// </summary>
        /// <param name="device">A dwType value (RIM_TYPEMOUSE, 
        /// RIM_TYPEKEYBOARD or RIM_TYPEHID).</param>
        /// <returns>A string representation of the input value.</returns>
        private string GetDeviceType(int device)
        {
            string deviceType;
            switch (device)
            {
                case RIM_TYPEMOUSE: deviceType = "MOUSE"; break;
                case RIM_TYPEKEYBOARD: deviceType = "KEYBOARD"; break;
                case RIM_TYPEHID: deviceType = "HID"; break;
                default: deviceType = "UNKNOWN"; break;
            }
            return deviceType;
        }

        #endregion GetDeviceType( int device )

        #region ReadReg( string item, ref bool isKeyboard )

        /// <summary>
        /// Reads the Registry to retrieve a friendly description
        /// of the device.
        /// </summary>
        /// <param name="item">The device name to search for, as provided by GetRawInputDeviceInfo.</param>
        /// <returns>The device description stored in the Registry entry's DeviceDesc value.</returns>
        private string ReadReg(string item)
        {
            // Example Device Identification string
            // @"\??\ACPI#PNP0303#3&13c0b0c5&0#{884b96c3-56ef-11d1-bc8c-00a0c91405dd}";

            // remove the \??\
            item = item.Substring(4);

            string[] split = item.Split('#');

            string id_01 = split[0];    // ACPI (Class code)
            string id_02 = split[1];    // PNP0303 (SubClass code)
            string id_03 = split[2];    // 3&13c0b0c5&0 (Protocol code)
            //The final part is the class GUID and is not needed here

            //Open the appropriate key as read-only so no permissions
            //are needed.
            RegistryKey OurKey = Registry.LocalMachine;

            string findme = string.Format(@"System\CurrentControlSet\Enum\{0}\{1}\{2}", id_01, id_02, id_03);

            OurKey = OurKey.OpenSubKey(findme, false);

            //Retrieve the desired information
            string deviceDesc = (string)OurKey.GetValue("DeviceDesc");
            return deviceDesc;
        }

        #endregion ReadReg( string item, ref bool isKeyboard )


        public void StopWatchReset()
        {
            this.stopWatch.Reset();
            this.stopWatch.Start();
        }

        public delegate void MouseEventHandler(object RawMouse, MouseEvent meventinfo);
        public MouseEventHandler mevent;

        public void ProcessRawInput(Message m)
        {
            if (m.Msg == WM_INPUT)
            {
                uint dwSize = 0;

                GetRawInputData(m.LParam,
                                RID_INPUT, IntPtr.Zero,
                                ref dwSize,
                                (uint)Marshal.SizeOf(typeof(RAWINPUTHEADER)));

                IntPtr buffer = Marshal.AllocHGlobal((int)dwSize);
                try
                {
                    if (buffer != IntPtr.Zero &&
                        GetRawInputData(m.LParam,
                                        RID_INPUT,
                                        buffer,
                                        ref dwSize,
                                        (uint)Marshal.SizeOf(typeof(RAWINPUTHEADER))) == dwSize)
                    {
                        RAWINPUT raw = (RAWINPUT)Marshal.PtrToStructure(buffer, typeof(RAWINPUT));                        

                        if (raw.header.dwType == RIM_TYPEMOUSE)
                        {                            
                            if (mevent != null)
                            {
                                DeviceInfo dInfo = (DeviceInfo)deviceList[raw.header.hDevice];
                                // if the mouse device is not registered yet, reEnumerate devices
                                if (dInfo == null)
                                { 
                                    EnumerateDevices();
                                    dInfo = (DeviceInfo)deviceList[raw.header.hDevice];
                                }
                                  
                                MouseEvent meventinfo = new MouseEvent(raw.mouse.buttonsStr.usButtonFlags, raw.mouse.lLastX, -raw.mouse.lLastY,
                                                                       stopWatch.ElapsedTicks * 1e3 / Stopwatch.Frequency, dInfo.source);
                                mevent(this, meventinfo);
                            }
                            //Debug.WriteLine((stopWatch.ElapsedTicks * 1e3 / Stopwatch.Frequency).ToString() + ", " +
                            //                raw.mouse.lLastX.ToString() + ", " +
                            //                raw.mouse.lLastY.ToString());
                        }
                    }
                }
                finally
                {
                    Marshal.FreeHGlobal(buffer);
                }
            }
        }
    }
}
