using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using MouseTester;
using System.IO;
using System.Diagnostics;
using System.Globalization;

namespace CustomMouseCurve
{
    /// <summary>
    /// Logging mouse events.
    /// </summary>
    class MouseLogger
    {
        private Queue<MouseEvent> events;
        private double timewindow;

        public Queue<MouseEvent> Events
        {
            get
            {
                return this.events;
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="loggingTime">Size of time window for mouse event logging (unit: second)</param>
        public MouseLogger(double loggingTime)
        {
            // Assume that 1000 fps mouse is the maximum
            int capacity = (int)(loggingTime * 1000);
            events = new Queue<MouseEvent>(capacity);
            this.timewindow = loggingTime;
        }

        public void Add(MouseEvent e)
        {
            events.Enqueue(e);

            while(events.Peek().ts < e.ts - timewindow*1000)
            {
                events.Dequeue();
            }
        }

        public void Clear()
        {
            events.Clear();
        }


        public void Save(string fname)
        {
            try
            {
                using (StreamWriter sw = File.CreateText(fname))
                {
                    sw.WriteLine("xCount,yCount,Time (ms)");
                    foreach (MouseEvent e in this.events)
                    {
                        sw.WriteLine(e.lastx.ToString() + "," + e.lasty.ToString() + "," + e.ts.ToString(CultureInfo.InvariantCulture) + "," + e.source);
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.ToString());
            }
        }

    }
}
