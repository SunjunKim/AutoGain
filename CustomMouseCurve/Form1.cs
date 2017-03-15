﻿using System;
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

/*
 * MouseEvent and RawMouse parts are adopted from MouseTester project (https://github.com/microe1/MouseTester)
 * mousehook library adopted from https://github.com/gmamaladze/globalmousekeyhook
 * icon from http://www.iconseeker.com/search-icon/hydropro-hardware/mouse-1.html
 */

namespace CustomMouseCurve
{
    public partial class Form1 : Form
    {
        private RawMouse mouse = new RawMouse();
        private IKeyboardMouseEvents m_GlobalHook;

        // MOUSE MOVEMENT translate function!
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

        Point p;
        double lastEventTimestamp = 0;
        double accel = 1.0;
        double speed = 0.5;
        long polling = 0;
        bool doTranslate = true;

        private void translateMouseMove(int x, int y, double timestamp)
        {
            // preserving double precision
            // p.x, p.y <= new point
            // x, y <= input from mouse
            /*
            p.x += (double)x * 1.0;
            p.y += (double)y * 1.0;
            */
            // Example acceleration function http://stackoverflow.com/questions/8773037/how-to-simulate-mouse-acceleration

            polling++;

            double timespan = double.MaxValue;

            if (lastEventTimestamp == 0)
                lastEventTimestamp = timestamp;
            else
            {
                timespan = timestamp - lastEventTimestamp;
                lastEventTimestamp = timestamp;
            }

            double a = speed;
            double b = accel;

            double dr = Math.Sqrt(x * x + y * y);

            if(dr == 0 || timespan == 0)
            {
                // Reset counters;
                lastEventTimestamp = 0;
                mouse.StopWatchReset();
                return;
            }
            
            double v = dr / timespan / 10;

            double vNew = a * v + b * v * v;
            double drNew = vNew * timespan * 10;           

            double xNew = x * drNew / dr;
            double yNew = y * drNew / dr;

            if (double.IsNaN(xNew) || double.IsNaN(yNew))
                return;

            if (double.IsNaN(p.x))
                p.x = 0;
            if (double.IsNaN(p.y))
                p.y = 0;

            // set new mouse pointer coordinate
            p.x += xNew;
            p.y += yNew;

            

            Win32.setCursorAbsolute((int)p.x, (int)p.y);
            
            // limit mouse cursor within a screen size
            bool isInBound = false;
            foreach (Screen screen in Screen.AllScreens)
            {
                var bounds = screen.Bounds;
                if (bounds.Contains((int)p.x, (int)p.y))
                {
                    isInBound = true;
                }
            }
            if (!isInBound)
            {
                Win32.POINT pt = Win32.GetCursorPosition();
                p.x = pt.X;
                p.y = pt.Y;
            }
        }

        public Form1()
        {
            InitializeComponent();

            // Code adopted from https://github.com/microe1/MouseTester/blob/master/MouseTester/MouseTester/Form1.cs
            #region Set process priority to the highest
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

            this.mouse.RegisterRawInputMouse(Handle);
            this.mouse.mevent += new RawMouse.MouseEventHandler(this.readMouseEvent);
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

            int minx, miny, maxx, maxy;
            minx = miny = int.MaxValue;
            maxx = maxy = int.MinValue;

            foreach (Screen screen in Screen.AllScreens)
            {
                var bounds = screen.Bounds;
                minx = Math.Min(minx, bounds.X);
                miny = Math.Min(miny, bounds.Y);
                maxx = Math.Max(maxx, bounds.Right);
                maxy = Math.Max(maxy, bounds.Bottom);

                Console.WriteLine("({0}, {1}, {2}, {3})", minx, maxx, miny, maxy);
            }

            Console.WriteLine("(width, height) = ({0}, {1})", maxx - minx, maxy - miny);
        }

        
        private void readMouseEvent(object RawMouse, MouseEvent meventinfo)
        {
            if (doTranslate)
                translateMouseMove(meventinfo.lastx, meventinfo.lasty * -1, meventinfo.ts);
        }
        
        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            notifyIcon1.Dispose();
        }

        // prevent mouse move event
        private void m_GlobalHook_MouseMoveExt(object sender, MouseEventExtArgs e)
        {
            if (doTranslate)
                e.Handled = true;
            else
                e.Handled = false;
        }

        const int WM_NCLBUTTONDOWN  = 0x00A1;
        const int WM_NCHITTEST      = 0x0084;
        const int WM_NCMOUSELEAVE   = 0x02A2;

        // Code adopted from https://github.com/microe1/MouseTester/blob/master/MouseTester/MouseTester/Form1.cs
        protected override void WndProc(ref Message m)
        {
            this.mouse.ProcessRawInput(m);
            base.WndProc(ref m);
            /*
            if (m.Msg == WM_NCHITTEST)
            {
                if(ClientRectangle.Contains(PointToClient(Control.MousePosition)))
                {
                    if (doTranslate == true)
                    {
                        toolStripSplitButton1.ForeColor = Color.Red;
                        toolStripSplitButton1.Text = "OFF";                
                    }
                    doTranslate = false;
                }
                else
                {
                    if (doTranslate == false)
                    {
                        toolStripSplitButton1.ForeColor = Color.Blue;
                        toolStripSplitButton1.Text = "ON";                
                    }
                    doTranslate = true;

                }
            }*/
        }

        private void 종료ToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void trackBar1_Scroll(object sender, EventArgs e)
        {
            accel = (double)trackBar1.Value / 10;
            label1.Text = "Acceleration = " + accel;
        }

        private void trackBar2_Scroll(object sender, EventArgs e)
        {
            speed = (double)trackBar2.Value / 10;
            label2.Text = "Speed = " + speed;
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            toolStripStatusLabel1.Text = "Polling rate = " + polling + " Hz";
            polling = 0;
        }

        private void Form1_MouseDown(object sender, MouseEventArgs e)
        {
            //Console.WriteLine("Disable the translation");
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
    }


    #region Mouse move related functions
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
