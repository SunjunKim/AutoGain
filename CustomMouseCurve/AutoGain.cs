using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using persistence1d;
using System.Runtime.InteropServices;
using System.Diagnostics;
using MouseTester;
using System.IO;

namespace CustomMouseCurve
{
    class AutoGain
    {
        // public properties
        public string DeviceID { get { return this.deviceID; } }
        public double HZ { get { return this.rate; } }
        public double CPI { get { return this.cpi; } }
        public double PPI { get { return this.ppi; } }
        public Queue<MouseEventLog> Events { get { return this.events; } }
        
        // constants
        const int binCount = 128; // how many bins are there
        const double binSize = 0.01; // size of a bin unit: m/s
        
        // internal variables
        string deviceID;
        double rate; // mouse max. polling rate
        double cpi;  // mouse counts per inch
        double cpm { get { return cpi / 0.0254; } } // counts per meter, 0.0254 m = 1 inch
        double ppi; // screen resolution, pixels per inch
        double ppm { get { return ppi / 0.0254; } } // pixels per meter, 0.0254 m = 1 inch
        double minTimespan { get { return 1000 / rate; } } // minimum timespan for given polling rate

        Queue<MouseEventLog> events;
        const double timewindow = 30; // unit: second
        List<double> gainCurves = new List<double>(binCount);

        // for Hz calculation
        int reportCount = 0;
        double lastTimeRecord = 0;

        /// <summary>
        /// AutoGain initializer
        /// </summary>
        /// <param name="deviceID">pointing device id</param>
        /// <param name="cpi">pointing device counts per inch</param>
        /// <param name="dpi">display device dots per inch</param>
        public AutoGain(string deviceID, double cpi = 800, double dpi = 72)
        {
            this.deviceID = deviceID;

            String logPath = getLogPath();
            // get the latest log if exist
            if(Directory.Exists(logPath))
            {
                // load the latest log.
                loadAutoGain();
            }
            else
            {
                // assign a default values
                this.rate = 125; // for a regular mouse device
                this.cpi = cpi;
                this.ppi = dpi;

                // gain curve initialize
                loadDefaultCurve();

                saveAutoGain();
            }

            // logger initialize
            int capacity = (int)(timewindow * 1000);
            events = new Queue<MouseEventLog>(capacity);
        }

        public override string ToString()
        {
            String ret = "Information";
            ret += "\r\nID   = " + this.deviceID;
            ret += "\r\nHz   = " + this.HZ;
            ret += "\r\nCPI  = " + this.CPI;
            ret += "\r\nPPI  = " + this.PPI;
            ret += "\r\n#cnt = " + events.Count;
            return ret;
        }

        public String getLogPath()
        {
            String basePath = AppDomain.CurrentDomain.BaseDirectory;
            String pathString = Path.Combine(basePath, this.deviceID);

            return pathString;
        }

        public void saveAutoGain()
        {
            String pathString = getLogPath();
            String filename = DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".csv";
            String fileString = Path.Combine(pathString, filename);
            
            StreamWriter sw;
            Directory.CreateDirectory(pathString);
            sw = new StreamWriter(fileString, false, Encoding.UTF8);

            // top five lines: deviceID / rate / cpi / ppi / # function values
            sw.WriteLine(this.DeviceID);
            sw.WriteLine(this.rate);
            sw.WriteLine(this.cpi);
            sw.WriteLine(this.ppi);
            sw.WriteLine(gainCurves.Count);

            // flush all values
            for (int i = 0; i < gainCurves.Count; i++)
            {
                sw.WriteLine(gainCurves[i]);
            }

            sw.Flush();
            sw.Close();
        }

        public bool loadAutoGain(string path)
        {
            if (!File.Exists(path))
                return false;

            string[] lines = File.ReadAllLines(path, Encoding.UTF8);

            if (lines.Length <= 5)
                return false;

            // top five lines: deviceID / rate / cpi / ppi / # function values
            bool success = true;
            success &= double.TryParse(lines[1], out rate);
            success &= double.TryParse(lines[2], out cpi);
            success &= double.TryParse(lines[3], out ppi);

            int lineCount = 0;
            success &= int.TryParse(lines[4], out lineCount);

            if (!success || lines.Length < 5 + lineCount)
                return false;

            for (int i = 5; i < lineCount + 5; i++)
            {
                double val = 0;
                success &= double.TryParse(lines[i], out val);
                if(success)
                {
                    gainCurves.Add(val);
                }
            }

            if (!success)
                return false;

            Console.WriteLine("Successfully loaded {0}", path);
            return true;
        }

        // if not filename specified, load the latest.
        public bool loadAutoGain()
        {
            String path = getLogPath();
            List<string> files = new List<string>(Directory.GetFiles(path, "*.csv"));
            // get the latest one! (order by filename, the lastest one should have the top item in dictionary order).
            return loadAutoGain(files.Max());
        }

        // TODO: 지금은 constant지만, 나중에 여러 default curve를 로드할 수 있도록 수정.
        public void loadDefaultCurve()
        {
            loadDefaultCurve("constant", 10.0);
        }

        public void loadDefaultCurve(string type, double multiplier)
        {
            switch(type)
            {
                case "constant":
                    for (int i = 0; i < binCount; i++)
                    {
                        gainCurves.Add(multiplier);
                    }
                    return;
            }
        }

        public void feedMouseEvent(MouseEventLog datapoint)
        {
            events.Enqueue(datapoint);
            reportCount++;

            // button clicked!!
            if((datapoint.buttonflags & (RawMouse.RI_MOUSE_LEFT_BUTTON_DOWN | RawMouse.RI_MOUSE_RIGHT_BUTTON_DOWN)) != 0)
            {
                // update the gain curve
                updateCurve();
                // clear the queue
                events.Clear();
            }

            // every 1 sec, calculate Hz.
            if((datapoint.timestamp - lastTimeRecord) >= 1000)
            {
                if (reportCount > this.rate)
                {
                    this.rate = reportCount;
                    Debug.WriteLine("Update report rate: {0}", this.rate);
                }

                lastTimeRecord = datapoint.timestamp;
                reportCount = 0;
            }
            // clear old datapoints over the timewindow.
            while (events.Count > 0 && events.Peek().timestamp < datapoint.timestamp - timewindow * 1000)
            {
                events.Dequeue();
            }
        }

        /// <summary>
        /// Update the gain curve using AutoGain algorithm
        /// </summary>
        private void updateCurve()
        {
            // TODO: Byungjoo ==> make this function.
            
            // get history from the queue
            List<MouseEventLog> history = events.ToList<MouseEventLog>();
            
            // TODO: gain recalculation.
            // check the [MouseEventLog] struct for detailed descriptions for [history] logs.
            // for persistence1d, use: List<PairedExtrema> getPairedExtrema(List<double> inputData, double threshold)
            saveAutoGain();
            for(int i=0;i<gainCurves.Count;i++)
            {
                // update here gainCurves[i] = ??;
            }

        }

        /// <summary>
        /// translate mouse movement
        /// </summary>
        /// <param name="dx"></param>
        /// <param name="dy"></param>
        /// <param name="timespan"></param>
        /// <param name="tx"></param>
        /// <param name="ty"></param>
        /// <returns></returns>
        public bool getTranslatedValue(double dx, double dy, double timespan, out double tx, out double ty)
        {
            timespan = Math.Max(minTimespan, timespan); // preventing too fast polling because of a delayed event call
                
            double magnitude = Math.Sqrt(dx * dx + dy * dy) / cpm; // calculated unit: meter
            double speed = magnitude / (timespan / 1000);  // unit: m/s

            double CDGain = cpi / ppi;

            double gain = getInterpolatedValue(speed / binSize, gainCurves) / CDGain;

            tx = dx * gain;
            ty = dy * gain;

            return true;
        }

        // interpolation
        public static double getInterpolatedValue(double index, List<double> list)
        {
            int lowerIndex = (int)Math.Floor(index);
            int upperIndex = (int)Math.Ceiling(index);

            // minimum value (for out of index)
            if (lowerIndex < 0)
                return list[0];
            // maximum value (for out of index)
            if (upperIndex > list.Count)
                return list[list.Count - 1];
            
            return linearMap(index, lowerIndex, upperIndex, list[lowerIndex], list[upperIndex]);
        }
        public static double linearMap(double x, double x0, double x1, double y0, double y1)
        {
            if ((x1 - x0) == 0)
                return (y0 + y1) / 2;

            double ratio = (x - x0) / (x1 - x0);
            return (y1 - y0) * ratio + y0;
        }

        #region persistence1d related codes
        struct PairedExtrema
        {
            public PairedExtrema(int min, int max, double pers)
            {
                MinIndex = min;
                MaxIndex = max;
                Persistence = pers;
            }

            int MinIndex;
            int MaxIndex;
            double Persistence;
        };

        /// <summary>
        /// Get paired extrema using persistence1d library,
        /// check (https://github.com/yeara/Persistence1D)
        /// and persistence1dWrapper class for more detail.
        /// </summary>
        /// <param name="inputData">list of 'float' type data</param>
        /// <param name="threshold">persistence threshold value</param>
        /// <returns>List of paired extrema (PairedExtrema struct)</returns>
        private List<PairedExtrema> getPairedExtrema(List<double> inputData, double threshold)
        {
            persistence1d.p1d p;
            p = new persistence1d.p1d();

            p.RunPersistence(inputData);

            List<PairedExtrema> pairs = new List<PairedExtrema>();
            List<int> mins = new List<int>();
            List<int> maxs = new List<int>();
            List<double> pers = new List<double>();

            p.GetPairedExtrema(mins, maxs, pers, threshold);
            for (int i = 0; i < mins.Count; i++)
            {
                PairedExtrema np = new PairedExtrema(mins[i], maxs[i], pers[i]);
                pairs.Add(np);
            }

            p.Dispose();

            return pairs;
        }
        #endregion

        public struct MouseEventLog
        {
            public ushort buttonflags;
            public int deviceDX;
            public int deviceDY;
            public double systemDX;
            public double systemDY;
            public double timestamp;
            public double timespan;
            public string source;

            /// <summary>
            /// Mouse log structure for autogain
            /// </summary>
            /// <param name="buttonflags">buttunFlags (check RAWMOUSE structure)</param>
            /// <param name="devDX">dx (device level, before transfer function)</param>
            /// <param name="devDY">dy (device level, before transfer function)</param>
            /// <param name="sysDX">dx (system level, after transfer function)</param>
            /// <param name="sysDY">dy (system level, after transfer function)</param>
            /// <param name="timestamp">timestamp (in ms, absoulute)</param>
            /// <param name="timespan">timespan (in ms, time elapsed from the last event)</param>
            /// <param name="source">vid and pid descriptor of the device</param>
            public MouseEventLog(ushort buttonflags, int devDX, int devDY, double sysDX, double sysDY, double timestamp, double timespan, string source)
            {
                this.buttonflags = buttonflags;
                this.deviceDX = devDX;
                this.deviceDY = devDY;
                this.systemDX = sysDX;
                this.systemDY = sysDY;
                this.timestamp = timestamp;
                this.timespan = timespan;
                this.source = source;
            }
        }

    }
}
