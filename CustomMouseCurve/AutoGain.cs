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
using System.Drawing;
using System.Windows.Forms;

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
        public List<double> curve { get { return gainCurves; } }
        public bool doLearning = false;
        
        // constants
        const int binCount = 128; // how many bins are there
        const double binSize = 0.005; // size of a bin unit: m/s 0.01 => 1cm/s motor movement
        
        // internal variables
        string deviceID;
        double rate; // mouse max. polling rate
        double cpi;  // mouse counts per inch
        double cpm { get { return cpi / 0.0254; } } // counts per meter, 0.0254 m = 1 inch
        double ppi; // screen resolution, pixels per inch
        double ppm { get { return ppi / 0.0254; } } // pixels per meter, 0.0254 m = 1 inch
        double minTimespan { get { return 1000 / rate; } } // minimum timespan for given polling rate

        Queue<MouseEventLog> events;
        const double timewindow = 5; // unit: second
        List<double> gainCurves = new List<double>(binCount);
        int max_number_submovement = 3;        
        double gain_change_rate = 0.005;

        double lastSpeed = 0;

        //Aim point estimation
        double sub_aim_point = 0.95;
        double process_noise = 0.2;
        double sensor_noise = 40.0;
        double estimated_error = 1.0;
        double kalman_gain = 1.0;
        double filtered_aim_point = 0.95;

        // for Hz calculation
        int reportCounter = 0;
        Timer hzCalculateTimer;


        /// <summary>
        /// AutoGain initializer
        /// </summary>
        /// <param name="deviceID">pointing device id</param>
        /// <param name="dpi">display device dots per inch</param>
        /// <param name="cpi">pointing device counts per inch</param>
        public AutoGain(string deviceID, double dpi = 96, double cpi = 800)
        {
            this.deviceID = deviceID;

            Rectangle resolution = Screen.PrimaryScreen.Bounds;

            hzCalculateTimer = new Timer();
            hzCalculateTimer.Interval = 1000;
            hzCalculateTimer.Tick += hzCalculateTimer_Tick;
            hzCalculateTimer.Start();

            String logPath = getLogPath();
            // get the latest log if exist
            if(Directory.Exists(logPath))
            {
                // load the latest log.
                loadAutoGain();
                this.ppi = dpi;
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



        // TODO: 지금은 constant지만, 나중에 여러 default curve를 로드할 수 있도록 수정.
        public void loadDefaultCurve()
        {
            loadDefaultCurve("constant", 1.0);
        }

        public void loadDefaultCurve(string type, double multiplier)
        {
            switch (type)
            {
                case "constant":
                    gainCurves.Add(0);
                    for (int i = 0; i < binCount-1; i++)
                    {
                        gainCurves.Add(multiplier);
                    }
                    return;
            }
        }

        public void feedMouseEvent(MouseEventLog datapoint)
        {
            events.Enqueue(datapoint);
            reportCounter++;

            // button clicked!!
            //if((datapoint.buttonflags & (RawMouse.RI_MOUSE_LEFT_BUTTON_DOWN | RawMouse.RI_MOUSE_RIGHT_BUTTON_DOWN)) != 0)
            if ((datapoint.buttonflags & RawMouse.RI_MOUSE_LEFT_BUTTON_DOWN) != 0)
            {
                lastSpeed = 0;
                // update the gain curve
                if (doLearning)
                    updateCurve();
                // clear the queue
                events.Clear();
            }

            // clear old datapoints over the timewindow.
            while (events.Count > 0 && (events.Peek().usTimestamp < datapoint.usTimestamp - timewindow * 1e6 || events.Peek().timestamp > datapoint.timestamp))
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

            if (lastSpeed < speed)
                lastSpeed = speed;

            double CDGain = ppi / cpi;

            double gain = getInterpolatedValue(speed / binSize, gainCurves) * CDGain;

            tx = dx * gain;
            ty = dy * gain;

            return true;
        }



        /// <summary>
        /// Update the gain curve using AutoGain algorithm
        /// </summary>
        private void updateCurve()
        {
            // TODO: Byungjoo ==> make this function.

            // get history from the queue
            List<MouseEventLog> history = events.ToList<MouseEventLog>();

            // calculate speeds.
            List<double> output_speeds = new List<double>();
            List<double> filtered_speeds = new List<double>();

            List<double> input_speeds = new List<double>();
            List<double> timespans = new List<double>();
            List<double> sx_sum = new List<double>();
            List<double> sy_sum = new List<double>();
            double sx_temp = 0;
            double sy_temp = 0;
            double tx;
            double ty;

            foreach (var displacement in history)
            {
                sx_temp += displacement.systemDX;
                sy_temp += displacement.systemDY;
                output_speeds.Add(Math.Sqrt(displacement.systemDX * displacement.systemDX + displacement.systemDY * displacement.systemDY) / ppm / (displacement.timespan / 1000));
                input_speeds.Add(Math.Sqrt(displacement.deviceDX * displacement.deviceDX + displacement.deviceDY * displacement.deviceDY) / cpm / (displacement.timespan / 1000));
                timespans.Add(displacement.timespan);
                sx_sum.Add(sx_temp);
                sy_sum.Add(sy_temp);
            }
            tx = sx_sum[sx_sum.Count - 1];
            ty = sy_sum[sy_sum.Count - 1];

            double[] kernel = { 0.0, 0, 0.27901, 0.44198, 0.27901, 0, 0.0 };

            for (int j = 0; j < output_speeds.Count; j++)
            {
                double value = 0;
                double kernel_sum = 0;
                for (int k = -3; k < 3; k++)
                {
                    if ((j + k) >= 0 && (j + k) < output_speeds.Count)
                    {
                        value += output_speeds[j + k] * kernel[k + 3];
                        kernel_sum += kernel[k + 3];
                    }
                }
                filtered_speeds.Add(value / kernel_sum);
            }

            // TODO: speed array filtering => adaptive thresholding (relative to filtered_speeds.Max)?
            double persistence1d_threshold = 0.11;

            List<int> mins = new List<int>();
            List<int> maxs = new List<int>();
            #region persistence1d, finding peaks and save them to mins/maxs
            persistence1d.p1d p;
            p = new persistence1d.p1d();

            p.RunPersistence(filtered_speeds);

            p.GetExtremaIndices(mins, maxs, persistence1d_threshold);
            p.Dispose();
            #endregion

            mins.Sort();
            maxs.Sort();

            bool is_updated = false;

            if (mins.Count > 1 && maxs.Count > 1)
            {
                // if the Max peak appeared first, remove this.
                if (mins[0] > maxs[0])
                {
                    maxs.RemoveAt(0);
                }
                mins.Add(output_speeds.Count - 1);

                // find the first biggest sub movement.

                int first_max_index = 0;        // index of maxs ==> first max index in speed array = maxs[first_max_index] / min[first_min_index]
                int first_min_index = 0;
                #region Finding the biggest sub movement
                double buffer_speed = 0;
                for (int i = 0; i < maxs.Count; i++)
                {
                    if (output_speeds[maxs[i]] > buffer_speed)
                    {
                        first_max_index = i;
                        buffer_speed = output_speeds[maxs[i]];
                    }
                }

                int gMax = maxs[first_max_index];
                for (int i = 0; i < mins.Count; i++)
                {
                    if (mins[i] < gMax)
                    {
                        first_min_index = i;
                    }
                }
                #endregion


                // find clutching submovements (aligned to maxs)
                List<int> is_clutching_max_aligned = new List<int>();
                List<int> is_clutching_min_aligned = new List<int>();

                double clutch_timespan_threshold = 130; // ms

                // TODO: definition of clutching should be updated in terms of distance for a given time widnow
                #region Marking clutching submovements

                for (int i = 0; i < maxs.Count - 1; i++)
                {
                    int clutching_mark = 0;
                    for (int j = maxs[i]; j < maxs[i + 1]; j++)
                    {
                        if (j < timespans.Count)
                        {
                            double timespan = timespans[j];
                            if (timespan > clutch_timespan_threshold)
                            {
                                clutching_mark = 1;
                            }
                        }
                    }
                    is_clutching_max_aligned.Add(clutching_mark);
                }
                is_clutching_max_aligned.Add(0);


                List<int> max_checked = new List<int>();

                for (int i = 0; i < maxs.Count; i++)
                {
                    max_checked.Add(0);
                }

                for (int i = 0; i < mins.Count - 1; i++)
                {
                    Boolean never_assigned = true;
                    for (int j = 0; j < maxs.Count; j++)
                    {

                        if (mins[i] < maxs[j] && mins[i + 1] > maxs[j] && max_checked[j] == 0)
                        {
                            is_clutching_min_aligned.Add(is_clutching_max_aligned[j]);
                            max_checked[j] = 1;
                            never_assigned = false;
                        }

                    }

                    if (never_assigned)
                    {
                        is_clutching_min_aligned.Add(0);
                    }
                }


                #endregion

                // find unamied and interrupted submovements (aligned to mins)
                List<int> is_unaimed = new List<int>();
                List<int> is_interrupted = new List<int>();

                double unaimed_angle_threshold = Math.PI / 4;
                double overshoot_threshold = 1.5;
                double interrupted_threshold = 0.5;

                
                //TODO: Check the threshold of interrupted, overhsoot, ballistic - check movement speed
                #region Marking unaimed and interrupted submovements
                for (int i = 0; i < mins.Count - 1; i++)
                {
                    double sx_start = sx_sum[mins[i]];
                    double sy_start = sy_sum[mins[i]];
                    double max_angle = 0;
                    double angle = 0;
                    double d1 = 0;
                    double d2 = 0;
                    double d3 = 0;
                    double a = 0;
                    double b = 0;

                    int unaimed_mark = 1;
                    int interrupted_mark = 0;


                    for (int j = mins[i]; j <= mins[i + 1]; j++)
                    {
                        double sx_end = sx_sum[j];
                        double sy_end = sy_sum[j];
                        a = sy_end - sy_start;
                        b = sx_start - sx_end;
                        d1 = (Math.Sqrt((sx_start - sx_end) * (sx_start - sx_end) + (sy_start - sy_end) * (sy_start - sy_end)));
                        d2 = (Math.Sqrt((sx_start - tx) * (sx_start - tx) + (sy_start - ty) * (sy_start - ty)));
                        d3 = (Math.Sqrt((sx_end - tx) * (sx_end - tx) + (sy_end - ty) * (sy_end - ty)));
                        angle = 0;
                        if (d1 != 0 && d2 != 0)
                        {
                            angle = Math.Acos((d1 * d1 + d2 * d2 - d3 * d3) / 2 / d1 / d2);
                            if (angle > max_angle)
                            {
                                max_angle = angle;
                            }
                        }
                    }

                    if (a != 0 && b != 0 && (d2 * Math.Cos(angle)) != 0 && max_angle <= unaimed_angle_threshold && d1 / (d2 * Math.Cos(angle)) <= overshoot_threshold)
                    {
                        unaimed_mark = 0;
                    }
                    is_unaimed.Add(unaimed_mark);

                    if (a != 0 && b != 0 && (d2 * Math.Cos(angle)) != 0 && d1 / (d2 * Math.Cos(angle)) < interrupted_threshold)
                    {
                        interrupted_mark = 1;
                    }
                    is_interrupted.Add(interrupted_mark);

                }
                #endregion

                int first_non_clutching_submovement = mins.Count - 1;
                for (int i = first_min_index; i < is_clutching_min_aligned.Count; i++)
                {
                    if (is_clutching_min_aligned[i] != 1)
                    {
                        first_non_clutching_submovement = i;
                        break;
                    }
                }

                int index_right = mins.Count - 1;
                int index_last_ballistic = first_non_clutching_submovement + max_number_submovement - 1;

                List<int> speedAppearingCounts = new List<int>();
                List<int> is_speed_appeared = new List<int>();
                List<double> gainChanges = new List<double>();

                for (int i = 0; i < binCount; i++)
                {
                    speedAppearingCounts.Add(0);
                    gainChanges.Add(0);
                    is_speed_appeared.Add(0);
                }

     
                
                // Start updating gain function per each submovemen
                #region updating gain function
                for (int i = index_right - 1; i >= first_min_index; i--)
                {
                    
                    double sx_end = sx_sum[mins[i + 1]];
                    double sy_end = sy_sum[mins[i + 1]];
                    double sx_start = sx_sum[mins[i]];
                    double sy_start = sy_sum[mins[i]];
                    double a = sy_end - sy_start;
                    double b = sx_start - sx_end;
                    double d1 = (Math.Sqrt((sx_start - sx_end) * (sx_start - sx_end) + (sy_start - sy_end) * (sy_start - sy_end)));
                    double d2 = (Math.Sqrt((sx_start - tx) * (sx_start - tx) + (sy_start - ty) * (sy_start - ty)));
                    double d3 = (Math.Sqrt((sx_end - tx) * (sx_end - tx) + (sy_end - ty) * (sy_end - ty)));
                    double angle = 0;

                    if (d1 != 0 && d2 != 0)
                        angle = Math.Acos((d1 * d1 + d2 * d2 - d3 * d3) / 2 / d1 / d2);

                    // update aim point only for non-interrupted, non-clutching, non-unaimed, ballistic submovements
                    if (is_clutching_min_aligned[i] != 1 && is_interrupted[i] != 1 && is_unaimed[i] != 1 && i <= index_last_ballistic)
                    {
                        updateAimPoint(d1 / (d2 * Math.Cos(angle)));
                    }

                    double longitudinal_error = (sub_aim_point) * (d2 * Math.Cos(angle)) - d1;
                    if (i > index_last_ballistic)
                    {
                        longitudinal_error = d2 * Math.Cos(angle) - d1;
                    }


                    if (is_unaimed[i] != 1)
                    {
                        for (int j = mins[i]; j < mins[i + 1]; j++)
                        {
                            double current_speed = input_speeds[j];

                            if (current_speed != 0)
                            {

                                int middle = (int)Math.Ceiling(current_speed / binSize);
                                if (middle < speedAppearingCounts.Count && is_speed_appeared[middle] == 0)
                                {
                                    speedAppearingCounts[middle] = speedAppearingCounts[middle] + 1;
                                }
                            }
                        }

                        for (int j = 0; j < binCount; j++)
                        {
                            if (speedAppearingCounts[j] != 0.0)
                            {
                                is_speed_appeared[j] = 1;
                            }
                        }

                        for (int j = 0; j < binCount; j++)
                        {
                            if (speedAppearingCounts[j] > 0.0)
                            {

                                gainChanges[j] = gainChanges[j] + gain_change_rate * longitudinal_error;
                                is_updated = true;
                            }
                            speedAppearingCounts[j] = 0;
                        }

                        for (int j = 1; j < gainCurves.Count; j++)
                        {
                            double value = 0;
                            double kernel_sum = 0;
                            for (int k = -3; k < 3; k++)
                            {
                                if ((j + k) >= 0 && (j + k) < binCount)
                                {
                                    value += gainChanges[j + k] * kernel[k + 3];
                                    kernel_sum += kernel[k + 3];
                                }
                            }
                            gainCurves[j] = gainCurves[j] + value / kernel_sum;
                            if (gainCurves[j] < 0.0)
                            {
                                gainCurves[j] = 0.0;
                            }
                        }
                    }
                }

                double gainChangeSum = 0;
                for (int i = 0; i < binCount; i++)
                {
                    gainChangeSum += Math.Abs(gainChanges[i]);
                    gainChanges[i] = 0.0;
                }

                List<double> tempGains = new List<double>();
                for (int j = 1; j < gainCurves.Count; j++)
                {
                    double value = 0;
                    double kernel_sum = 0;
                    for (int k = -3; k < 3; k++)
                    {
                        if ((j + k) >= 0 && (j + k) < binCount)
                        {
                            value += gainCurves[j + k] * kernel[k + 3];
                            kernel_sum += kernel[k + 3];
                        }
                    }
                    tempGains.Add(value / kernel_sum);   
                }

                gainCurves.Clear();
                gainCurves.Add(0.0);
                gainCurves.AddRange(tempGains.ToArray());

                writeLog("GainChange", gainChangeSum.ToString());

                #endregion

            }

            if (is_updated)
                saveAutoGain();
        }

        #region prev updateCurve
        /*
        /// <summary>
        /// Update the gain curve using AutoGain algorithm
        /// </summary>
        private void updateCurve()
        {
            // get history from the queue
            List<MouseEventLog> history = events.ToList<MouseEventLog>();

            // calculate speeds.
            List<double> speeds = new List<double>();
            List<double> timespans = new List<double>();
            List<double> sx_sum = new List<double>();
            List<double> sy_sum = new List<double>();
            double sx_temp = 0;
            double sy_temp = 0;
            double tx;
            double ty;

            foreach (var displacement in history)
            {
                sx_temp += displacement.systemDX;
                sy_temp += displacement.systemDY;
                // TODO: system DX/DY 는 ppi (스크린 pixel per meter) 에 영향을 받는 움직임인데, 이걸 왜 cpm (마우스의 counts per meter) 으로 나누죠?
                speeds.Add(Math.Sqrt(displacement.systemDX * displacement.systemDX + displacement.systemDY * displacement.systemDY) / cpm / (displacement.timespan / 1000));
                timespans.Add(displacement.timespan);
                sx_sum.Add(sx_temp);
                sy_sum.Add(sy_temp);
            }
            tx = sx_sum[sx_sum.Count - 1];
            ty = sy_sum[sy_sum.Count - 1];

            // TODO: speed array filtering
            double persistence1d_threshold = 0.01;

            List<int> mins = new List<int>();
            List<int> maxs = new List<int>();
            #region persistence1d, finding peaks and save them to mins/maxs
            persistence1d.p1d p;
            p = new persistence1d.p1d();

            p.RunPersistence(speeds);

            p.GetExtremaIndices(mins, maxs, persistence1d_threshold);
            p.Dispose();
            #endregion

            mins.Sort();
            maxs.Sort();

            if (mins.Count > 1 && maxs.Count > 1)
            {
                // if the Max peak appeared first, remove this.
                if (mins[0] > maxs[0])
                {
                    maxs.RemoveAt(0);
                }
                mins.Add(speeds.Count - 1);

                // find the first biggest sub movement.

                int first_max_index = 0;        // index of maxs ==> first max index in speed array = maxs[first_max_index] / min[first_min_index]
                int first_min_index = 0;
                #region Finding the biggest sub movement
                double buffer_speed = 0;
                for (int i = 0; i < maxs.Count; i++)
                {
                    if (speeds[maxs[i]] > buffer_speed)
                    {
                        first_max_index = i;
                        buffer_speed = speeds[maxs[i]];
                    }
                }

                int gMax = maxs[first_max_index];
                for (int i = 0; i < mins.Count; i++)
                {
                    if (mins[i] < gMax)
                    {
                        first_min_index = i;
                    }
                }
                #endregion


                // find clutching submovements (aligned to maxs)
                List<int> is_clutching_max_aligned = new List<int>();
                List<int> is_clutching_min_aligned = new List<int>();

                double clutch_timespan_threshold = 130; // ms
                #region Marking clutching submovements

                for (int i = 0; i < maxs.Count - 1; i++)
                {
                    int clutching_mark = 0;
                    for (int j = maxs[i]; j < maxs[i + 1]; j++)
                    {
                        if (j < timespans.Count)
                        {
                            double timespan = timespans[j];
                            if (timespan > clutch_timespan_threshold)
                            {
                                clutching_mark = 1;
                            }
                        }
                    }
                    is_clutching_max_aligned.Add(clutching_mark);
                }
                is_clutching_max_aligned.Add(0);


                List<int> max_checked = new List<int>();

                for (int i = 0; i < maxs.Count; i++)
                {
                    max_checked.Add(0);
                }

                for (int i = 0; i < mins.Count - 1; i++)
                {
                    Boolean never_assigned = true;
                    for (int j = 0; j < maxs.Count; j++)
                    {

                        if (mins[i] < maxs[j] && mins[i + 1] > maxs[j] && max_checked[j] == 0)
                        {
                            is_clutching_min_aligned.Add(is_clutching_max_aligned[j]);
                            max_checked[j] = 1;
                            never_assigned = false;
                        }

                    }

                    if (never_assigned)
                    {
                        is_clutching_min_aligned.Add(0);
                    }
                }


                #endregion

                // find unamied and interrupted submovements (aligned to mins)
                List<int> is_unaimed = new List<int>();
                List<int> is_interrupted = new List<int>();

                double unaimed_angle_threshold = Math.PI / 4;
                double overshoot_threshold = 1.5;
                double interrupted_threshold = 0.5;

                #region Marking unaimed and interrupted submovements
                for (int i = 0; i < mins.Count - 1; i++)
                {
                    double sx_start = sx_sum[mins[i]];
                    double sy_start = sy_sum[mins[i]];
                    double max_angle = 0;
                    double angle = 0;
                    double d1 = 0;
                    double d2 = 0;
                    double d3 = 0;
                    double a = 0;
                    double b = 0;

                    int unaimed_mark = 1;
                    int interrupted_mark = 0;


                    for (int j = mins[i]; j <= mins[i + 1]; j++)
                    {
                        double sx_end = sx_sum[j];
                        double sy_end = sy_sum[j];
                        a = sy_end - sy_start;
                        b = sx_start - sx_end;
                        d1 = (Math.Sqrt((sx_start - sx_end) * (sx_start - sx_end) + (sy_start - sy_end) * (sy_start - sy_end)));
                        d2 = (Math.Sqrt((sx_start - tx) * (sx_start - tx) + (sy_start - ty) * (sy_start - ty)));
                        d3 = (Math.Sqrt((sx_end - tx) * (sx_end - tx) + (sy_end - ty) * (sy_end - ty)));
                        angle = 0;
                        if (d1 != 0 && d2 != 0)
                        {
                            angle = Math.Acos((d1 * d1 + d2 * d2 - d3 * d3) / 2 / d1 / d2);
                            if (angle > max_angle)
                            {
                                max_angle = angle;
                            }
                        }
                    }

                    if (a != 0 && b != 0 && (d2 * Math.Cos(angle)) != 0 && max_angle <= unaimed_angle_threshold && d1 / (d2 * Math.Cos(angle)) <= overshoot_threshold)
                    {
                        unaimed_mark = 0;
                    }
                    is_unaimed.Add(unaimed_mark);

                    if (a != 0 && b != 0 && (d2 * Math.Cos(angle)) != 0 && d1 / (d2 * Math.Cos(angle)) < interrupted_threshold)
                    {
                        interrupted_mark = 1;
                    }
                    is_interrupted.Add(interrupted_mark);

                }
                #endregion


                int first_non_clutching_submovement = mins.Count - 1;
                for (int i = 0; i < is_clutching_min_aligned.Count; i++)
                {
                    if (is_clutching_min_aligned[i] != 1)
                    {
                        first_non_clutching_submovement = i;
                        break;
                    }
                }

                int index_right = mins.Count - 1;
                int index_last_ballistic = first_non_clutching_submovement + max_number_submovement - 1;

                List<int> speedAppearingCounts = new List<int>();
                List<double> gainChanges = new List<double>();

                for (int i = 0; i < binCount; i++)
                {
                    speedAppearingCounts.Add(0);
                    gainChanges.Add(0);
                }

                // Start updating gain function per each submovemen
                #region updating gain function
                for (int i = index_right - 1; i >= 0; i--)
                {
                    double sx_end = sx_sum[mins[i + 1]];
                    double sy_end = sy_sum[mins[i + 1]];
                    double sx_start = sx_sum[mins[i]];
                    double sy_start = sy_sum[mins[i]];
                    double a = sy_end - sy_start;
                    double b = sx_start - sx_end;
                    double d1 = (Math.Sqrt((sx_start - sx_end) * (sx_start - sx_end) + (sy_start - sy_end) * (sy_start - sy_end)));
                    double d2 = (Math.Sqrt((sx_start - tx) * (sx_start - tx) + (sy_start - ty) * (sy_start - ty)));
                    double d3 = (Math.Sqrt((sx_end - tx) * (sx_end - tx) + (sy_end - ty) * (sy_end - ty)));
                    double angle = 0;

                    if (d1 != 0 && d2 != 0)
                        angle = Math.Acos((d1 * d1 + d2 * d2 - d3 * d3) / 2 / d1 / d2);

                    // update aim point only for non-interrupted, non-clutching, non-unaimed, ballistic submovements
                    if (is_clutching_min_aligned[i] != 1 && is_interrupted[i] != 1 && is_unaimed[i] != 1 && i <= index_last_ballistic)
                    {
                        updateAimPoint(d1 / (d2 * Math.Cos(angle)));
                    }

                    double longitudinal_error = (sub_aim_point) * (d2 * Math.Cos(angle)) - d1;
                    if (i > index_last_ballistic)
                    {
                        longitudinal_error = d2 * Math.Cos(angle) - d1;
                    }

                    if (is_unaimed[i] != 1)
                    {
                        for (int j = mins[i]; j < mins[i + 1]; j++)
                        {
                            double current_speed = speeds[j];

                            if (current_speed != 0)
                            {

                                int middle = (int)Math.Round(current_speed / binSize);
                                //Console.WriteLine(middle);
                                middle = Math.Min(middle, speedAppearingCounts.Count - 1);

                                speedAppearingCounts[middle] = speedAppearingCounts[middle] + 1;
                            }
                        }

                        for (int j = 0; j < binCount; j++)
                        {
                            if (speedAppearingCounts[j] > 0.0)
                            {
                                gainChanges[j] = gainChanges[j] + gain_change_rate * longitudinal_error;
                            }
                            speedAppearingCounts[j] = 0;
                        }

                        double[] kernel = { 0.0, 0, 0.27901, 0.44198, 0.27901, 0, 0.0 };

                        for (int j = 0; j < gainCurves.Count; j++)
                        {
                            double value = 0;
                            for (int k = -3; k < 3; k++)
                            {
                                if ((j + k) >= 0 && (j + k) < binCount)
                                {
                                    value += gainChanges[j + k] * kernel[k + 3];
                                }
                            }
                            gainCurves[j] = gainCurves[j] + value;
                            if (gainCurves[j] < 0.0)
                            {
                                gainCurves[j] = 0.0;
                            }

                        }

                    }
                }

                for (int i = 0; i < binCount; i++)
                {
                    gainChanges[i] = 0.0;
                }

                #endregion

            }

            //TODO: definition of gain function, range of input speed, bin count, averaging speed list, deciding appropriate gain change rate, modifying to only work with last 2-3 submovements (now it is updating gain functions for all submovements) 

            // TODO: gain recalculation.
            // check the [MouseEventLog] struct for detailed descriptions for [history] logs.
            // for persistence1d, use: List<PairedExtrema> getPairedExtrema(List<double> inputData, double threshold)
            //for (int i = 0; i < gainCurves.Count; i++)
            //{
            //    // update here gainCurves[i] = ??;
            //}
            saveAutoGain();
        }
        */
        #endregion 

        /// <summary>
        /// low pass filtering aim point measurements
        /// </summary>
        /// <param name="instant_aim_point"></param>
        public void updateAimPoint(double instant_aim_point)
        {
            estimated_error = estimated_error + sensor_noise;
            kalman_gain = process_noise / (process_noise + sensor_noise);
            filtered_aim_point = filtered_aim_point + kalman_gain * (instant_aim_point - filtered_aim_point);
            estimated_error = (1 - kalman_gain) * estimated_error;
            sub_aim_point = filtered_aim_point;
        }



        void hzCalculateTimer_Tick(object sender, EventArgs e)
        {
            // every 1 sec, calculate Hz.
            if (reportCounter > this.rate)
            {
                this.rate = reportCounter;
                Debug.WriteLine("Update report rate: {0}", this.rate);
            }
            reportCounter = 0;
        }

        public override string ToString()
        {
            String ret = "Information";
            ret += "\r\nID   = " + this.deviceID;
            ret += "\r\nHz   = " + this.HZ;
            ret += "\r\nCPI  = " + this.CPI;
            ret += "\r\nPPI  = " + this.PPI;
            //ret += "\r\n#cnt = " + events.Count;
            ret += "\r\nspd  = " + Math.Round(lastSpeed, 5) + " m/s";
            ret += "\r\nbin  = " + Math.Round(lastSpeed / binSize, 2) + "th";
            return ret;
        }

        public void writeLog(String logType, String value)
        {
            String pathString = getLogPath();
            String filename = "0_log.csv";
            String fileString = Path.Combine(pathString, filename);

            Directory.CreateDirectory(pathString);
            StreamWriter sw = new StreamWriter(fileString, true, Encoding.UTF8);


            sw.WriteLine("{0},{1},{2}", DateTime.Now.ToString("yyyyMMdd_HHmmss"), logType, value);

            sw.Flush();
            sw.Close();
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

            Debug.WriteLine("Successfully loaded {0}", path);
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


        // interpolation
        public static double getInterpolatedValue(double index, List<double> list)
        {
            int lowerIndex = (int)Math.Floor(index);
            int upperIndex = (int)Math.Ceiling(index);

            // minimum value (for out of index)
            if (lowerIndex < 0)
                return list[0];
            // maximum value (for out of index)
            if (upperIndex >= list.Count)
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
            public long usTimestamp;
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
            public MouseEventLog(ushort buttonflags, int devDX, int devDY, double sysDX, double sysDY, long usTimestamp, double timestamp, double timespan, string source)
            {
                this.buttonflags = buttonflags;
                this.deviceDX = devDX;
                this.deviceDY = devDY;
                this.systemDX = sysDX;
                this.systemDY = sysDY;
                this.usTimestamp = usTimestamp;
                this.timestamp = timestamp;
                this.timespan = timespan;
                this.source = source;
            }
        }

    }
}
