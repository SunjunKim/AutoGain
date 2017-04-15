using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using MouseTester;
using Gma.System.MouseKeyHook;
using System.IO;
using System.Windows.Forms.DataVisualization.Charting;
//using libPointingWrapper;

/*
 * MouseEvent and RawMouse codes are adopted and modified from MouseTester project (https://github.com/microe1/MouseTester) (MIT)
 * and https://www.codeproject.com/articles/17123/using-raw-input-from-c-to-handle-multiple-keyboard (LGPLv3)
 * mousehook library adopted from https://github.com/gmamaladze/globalmousekeyhook (BSD-3-Clause)
 * icon from http://www.iconseeker.com/search-icon/hydropro-hardware/mouse-1.html (free license)
 */

namespace CustomMouseCurve
{
    public partial class Form1 : Form
    {
        private RawMouse mouse = new RawMouse();        // RAWINPUT
        private IKeyboardMouseEvents m_GlobalHook;      // Mouse hook (to prevent mouse movement and detect clicks)

        Point p;                        // internally managed mouse pointer lication
        long lastEventTimestamp = 0;    // to calculate timespan between events
        long polling = 0;               // polling counter
        bool doTranslate = true;        // true if it translate the mouse events
        int unusedCounter = 0;

        double currentDPI = 96.0; // majority of screen's default dpi

        Dictionary<String, AutoGain> AGfunctions = new Dictionary<string, AutoGain>();
        AutoGain currentAG = null;

        //AutoGain ag = new AutoGain("test", 4000, 95);

        private void mouseEventCallback(object RawMouse, MouseEvent meventinfo)
        {
            if (RawMouse == null)
            {
                pauseTranslate();
                textBoxInfo.Text = "WARNING: Absolute device is not supported!";
                return;
            }
            else
                resumeTranslate();

            if (doTranslate)
            {
                if(!AGfunctions.ContainsKey(meventinfo.source))
                {
                    AGfunctions.Add(meventinfo.source, new AutoGain(meventinfo.source, currentDPI));
                    loadLogs();
                }
                translateFunction(meventinfo.buttonflags, meventinfo.lastx, meventinfo.lasty, meventinfo.usTimestamp, meventinfo.source);
            }
            unusedCounter = 0;
            labelDevice.Text = "Current: " + meventinfo.source;
            textBoxInfo.Text = AGfunctions[meventinfo.source].ToString();
            currentAG = AGfunctions[meventinfo.source];
        }

        /// <summary>
        /// define a translate function for mouse movement
        /// </summary>
        /// <param name="usButtonFlags">button flags</param>
        /// <param name="x">dx (relative mouse movement, negative: left / positive: right)</param>
        /// <param name="y">dy (relative mouse movement, negative: up / positive: down</param>
        /// <param name="usTimestamp">timestamp in microsecond, can be reset by calling mouse.StopWatchReset()</param>
        /// <param name="source">source device name. format: VID_PID (VID and PID are 4-digit hex numbers)</param>
        private void translateFunction(ushort usButtonFlags, int x, int y, long usTimestamp, string source)
        {
            // checking button press (e.g., LDn): (usFlags & MouseTester.RawMouse.MOUSE_LEFT_BUTTON_DOWN) != 0)
            // p.x, p.y <= internally managed pointer location (double)            

            // to record polling rate. 
            polling++;

            AutoGain ag = AGfunctions[source];
            Win32.POINT pt = Win32.GetCursorPosition();

            // timekeeping functions
            double timespan = double.MaxValue;

            if (lastEventTimestamp == 0)
                lastEventTimestamp = usTimestamp;
            else
            {
                timespan = (usTimestamp - lastEventTimestamp) / 1000.0;
                lastEventTimestamp = usTimestamp;
            }

            double tx, ty;

            ag.getTranslatedValue(x, y, timespan, out tx, out ty);
            
            // if preserved pointing and internal pointer is too far, reset the internal pointer
            if(Math.Sqrt(Math.Pow(p.x - pt.X,2)+Math.Pow(p.x - pt.X,2))>5)
            {
                p.x = pt.X;
                p.y = pt.Y;
            }
            
            // set new mouse pointer coordinate
            p.x += tx;
            p.y += ty;

            ag.feedMouseEvent(new AutoGain.MouseEventLog(usButtonFlags, x, y, tx, ty, usTimestamp, usTimestamp/1000.0, timespan, source));
            
            // move the mouse pointer
            Win32.setCursorAbsolute((int)p.x, (int)p.y);
            
            // limit mouse cursor inside the screens (can handle multiple screens)
            bool isInsideScreen = false;
            foreach (Screen screen in Screen.AllScreens)
            {
                var bounds = screen.Bounds;
                if (bounds.Contains((int)p.x, (int)p.y))
                {
                    isInsideScreen = true;
                }
            }
            if (!isInsideScreen) // if p.x or p.y goes beyond the screen bounds...
            {
                // reset the p.x and p.y by newly reading the mouse positions                
                p.x = pt.X;
                p.y = pt.Y;
            }
        }

        public Form1()
        {
            InitializeComponent();

            
            // get system dpi setup
            PointF dpi = PointF.Empty;
            using (Graphics g = this.CreateGraphics())
            {
                dpi.X = g.DpiX;
                dpi.Y = g.DpiY;
            }
            currentDPI = Math.Sqrt(dpi.X * dpi.X + dpi.Y * dpi.Y) / Math.Sqrt(2);
            Debug.WriteLine("current monitor DPI : {0}", currentDPI);
            Debug.WriteLine(double.MaxValue);

            // Code adopted from https://github.com/microe1/MouseTester/blob/master/MouseTester/MouseTester/Form1.cs
            #region Set process priority to the highest and RAWINPUT mouse
            try
            {
                Process.GetCurrentProcess().ProcessorAffinity = new IntPtr(2); // Use only the second core 
                Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.RealTime; // Set highest process priority
                Thread.CurrentThread.Priority = ThreadPriority.Highest; // Set highest thread priority
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.ToString());
            }

            this.mouse.EnumerateDevices();
            this.mouse.RegisterRawInputMouse(Handle);
            this.mouse.mevent += new RawMouse.MouseEventHandler(this.mouseEventCallback);
            this.mouse.StopWatchReset();
            #endregion

            // Codes adopted from https://github.com/gmamaladze/globalmousekeyhook
            #region Set mouse hook
            m_GlobalHook = Hook.GlobalEvents();
            m_GlobalHook.MouseMoveExt += m_GlobalHook_MouseMoveExt;
            #endregion

            // Program goes to tray
            //this.WindowState = FormWindowState.Minimized;
            //this.ShowInTaskbar = false;
            this.notifyIcon1.Visible = true;
            notifyIcon1.ContextMenuStrip = contextMenuStrip1;

            Win32.POINT currentPoint = Win32.GetCursorPosition();
            p = new Point(currentPoint.X, currentPoint.Y);

            timer1.Start();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            loadLogs();
        }

        private void loadLogs()
        {
            String basePath = AppDomain.CurrentDomain.BaseDirectory;
            String[] logs = Directory.GetDirectories(basePath);

            listBoxAGFunctions.Items.Clear();

            foreach(String s in logs)
            {
                listBoxAGFunctions.Items.Add(Path.GetFileName(s));
            }
        }
        
        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            notifyIcon1.Dispose();
        }

        // prevent mouse move event when doTranslate enabled
        private void m_GlobalHook_MouseMoveExt(object sender, MouseEventExtArgs e)
        {
            if (doTranslate)
                e.Handled = true;
            else
                e.Handled = false;
        }

        // Code adopted from https://github.com/microe1/MouseTester/blob/master/MouseTester/MouseTester/Form1.cs
        protected override void WndProc(ref Message m)
        {
            const int WM_NCLBUTTONDOWN = 0x00A1;
            const int WM_NCLBUTTONUP = 0x00A2;

            // detect form title bar click, and disable translation.
            switch(m.Msg)
            {
                case WM_NCLBUTTONDOWN:
                    pauseTranslate();
                    m.Result = IntPtr.Zero;
                    break;

                case WM_NCLBUTTONUP:
                    resumeTranslate();
                    m.Result = IntPtr.Zero;
                    break;
            }
            this.mouse.ProcessRawInput(m);
            base.WndProc(ref m);
        }

        #region Interface related functions

        private void 종료ToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if(polling == 0)
                unusedCounter++;
            toolStripStatusLabel1.Text = "Rate: " + polling + " Hz";

            // if a mouse is unused until 60s, reset the counter;
            if(unusedCounter > 60)
            {
                unusedCounter = 0;
                mouse.StopWatchReset();
            }

            polling = 0;

            // update gain curve
            {
                Series curveChart = chartGainCurve.Series.ElementAt(0);
                curveChart.Points.Clear();

                if (currentAG != null)
                {
                    for (int i = 0; i < currentAG.curve.Count; i++)
                    {
                        curveChart.Points.AddXY(i, currentAG.curve[i]);
                    }
                }
            }
        }

        private void oFFToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (doTranslate == true)
            {
                toolStripSplitButton1.ForeColor = Color.Red;
                toolStripSplitButton1.Text = "OFF";
            }
            doTranslate = false;
        }

        private void oNToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (doTranslate == false)
            {
                toolStripSplitButton1.ForeColor = Color.Blue;
                toolStripSplitButton1.Text = "ON";

                p.x = Win32.GetCursorPosition().X;
                p.y = Win32.GetCursorPosition().Y;
            }
            doTranslate = true;
        }

        private void toolStripSplitButton1_ButtonClick(object sender, EventArgs e)
        {
            if (doTranslate)
            {
                oFFToolStripMenuItem_Click(sender, e);
            }
            else
            {
                oNToolStripMenuItem_Click(sender, e);
            }
        }
        #endregion
        
        private void buttonSetup_Click(object sender, EventArgs e)
        {
            if(currentAG != null)
            {
                pauseTranslate();
                SetupForm setWindow = new SetupForm(this, currentAG.CPI, currentAG.PPI);
                setWindow.Size = new Size(300, 500);
                setWindow.Show();
            }
        }

        public void pauseTranslate()
        {
            doTranslate = false;
        }

        public void resumeTranslate()
        {
            if (toolStripSplitButton1.ForeColor == Color.Blue)
                doTranslate = true;
            else
                doTranslate = false;
        }

    }

    public class Point
    {
        public double x = 0;
        public double y = 0;

        public Point(double x, double y)
        {
            this.x = x;
            this.y = y;
        }
    };


    #region System Mouse related functions (get/set cursor pos)
    public class Win32
    {
        [DllImport("User32.Dll")]
        public static extern long SetCursorPos(int x, int y);

        [DllImport("User32.Dll")]
        public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);

        /// <summary>
        /// Struct representing a point.
        /// </summary>
        [StructLayout(LayoutKind.Sequential)]
        public struct POINT
        {
            public int X;
            public int Y;

            public static implicit operator Point(POINT point)
            {
                return new Point(point.X, point.Y);
            }
        }

        /// <summary>
        /// Retrieves the cursor's position, in screen coordinates.
        /// </summary>
        /// <see>See MSDN documentation for further information.</see>
        [DllImport("user32.dll")]
        public static extern bool GetCursorPos(out POINT lpPoint);

        public static POINT GetCursorPosition()
        {
            POINT lpPoint;
            GetCursorPos(out lpPoint);
            //bool success = User32.GetCursorPos(out lpPoint);
            // if (!success)

            return lpPoint;
        }

        public static void moveCursorRelative(int x, int y)
        {
            POINT p = GetCursorPosition();
            p.X += x;
            p.Y += y;

            SetCursorPos(p.X, p.Y);
        }

        public static void setCursorAbsolute(int x, int y)
        {
            POINT p;
            p.X = x;
            p.Y = y;
            SetCursorPos(p.X, p.Y);
        }
    }
    #endregion

}

