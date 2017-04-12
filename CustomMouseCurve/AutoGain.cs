using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using persistence1d;
using System.Runtime.InteropServices;

namespace CustomMouseCurve
{
    class AutoGain
    {
        const int binCount = 128; // how many bins are there
        const double binSize = 0.01; // size of a bin unit: m/s
        string deviceID { get; set; }
        public string DeviceID { get { return this.deviceID; } }
        double rate { get; set; } // mouse max. polling rate
        double cpi { get; set; } // mouse counts per inch
        double cpm { get { return cpi / 0.0254; } } // counts per meter, 0.0254 m = 1 inch
        double ppi { get; set; } // screen resolution, pixels per inch
        double ppm { get { return ppi / 0.0254; } } // pixels per meter, 0.0254 m = 1 inch
        double minTimespan { get { return 1000 / rate; } } // minimum timespan for given polling rate

        private Queue<MouseEventLog> events;
        public Queue<MouseEventLog> Events { get { return this.events; } }
        private const double timewindow = 30; // unit: second
        List<double> gainCurves = new List<double>(binCount);

        /// <summary>
        /// AutoGain initializer
        /// </summary>
        /// <param name="deviceID">pointing device id</param>
        /// <param name="Hz">pointing device refresh rate</param>
        /// <param name="cpi">pointing device counts per inch</param>
        /// <param name="dpi">display device dots per inch</param>
        public AutoGain(string deviceID, double Hz = 125, double cpi = 800, double dpi = 72)
        {
            this.deviceID = deviceID;
            this.rate = Hz;
            this.cpi = cpi;
            this.ppi = dpi;
            
            // gain curve initialize
            loadDefaultCurve();

            // logger initialize
            int capacity = (int)(timewindow * 1000);
            events = new Queue<MouseEventLog>(capacity);
        }

        // TODO: 지금은 constant지만, 나중에 여러 default curve를 로드할 수 있도록 수정.
        public void loadDefaultCurve()
        {
            loadDefaultCurve("constant", 1.0);
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

            // clear old datapoints over the timewindow.
            while (events.Peek().timestamp < datapoint.timestamp - timewindow * 1000)
            {
                events.Dequeue();
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
