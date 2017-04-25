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

        //logging properties
        int ballistic_submovement_count;
        int total_submovement_count;
        int net_submovement_count;
        int non_ballistic_submovement_count;
        int unaimed_submovement_count;
        int clutching_submovement_count;
        int interrupted_submovement_count;
        double trajectory_length;
        double net_gain_change;
        double force_inefficiency_x;
        double force_inefficiency_y;
        double acc_duration_ratio;
        double click_location_x;
        double click_location_y;
        double average_motor_speed;
        double average_display_speed;
        double maximum_motor_speed;
        double maximum_display_speed;
        double total_duration;
        String overshoot_or_undershoot;
        double sub_aim_point = 0.95;



        // public properties
        public string DeviceID { get { return this.deviceID; } }
        public double HZ { get { return this.rate; } }
        public double CPI { get { return this.cpi; } set { this.cpi = value; } }
        public double PPI { get { return this.ppi; } }
        public double CDGain { get { return ppi / cpi; } }
        public Queue<MouseEventLog> Events { get { return this.events; } }
        public List<double> curve { get { return gainCurves; } }
        public bool doLearning = false;
        
        // constants
        const int binCount = 128; // how many bins are there
        const double binSize = 0.005; // size of a bin unit: m/s 0.01 => 1cm/s motor movement
        double time_buffer_threshold = 1 / 125.0 * 3.0 * 1000.0;
        
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
        double gain_change_rate = 1;

        double lastSpeed = 0;

        StreamWriter logger;


        //Aim point estimation
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

                loadWindowCurve();
                saveAutoGain();
            }

            // logger initialize
            int capacity = (int)(timewindow * 1000);
            events = new Queue<MouseEventLog>(capacity);

            logger = openLog();
        }



        // TODO: 지금은 constant지만, 나중에 여러 default curve를 로드할 수 있도록 수정.
        public void loadWindowCurve()
        {  // gain curve initialize
            int mouseSpeed;
            bool isEpp;

            getMouseParameters(out mouseSpeed, out isEpp);

            int slot = (int)Math.Min(mouseSpeed, 20) / 2;

            gainCurves.Clear();
            if(!isEpp)
            {
                gainCurves.Add(0);
                for (int i = 0; i < binCount - 1;i++ )
                {
                    gainCurves.Add(winMultipliers[slot]/CDGain);
                }
            }
            else
            {
                for(int i=0;i<binCount;i++)
                {
                    gainCurves.Add(epp[i]*(mouseSpeed/10.0)/CDGain);
                }
            }
            saveAutoGain();
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

            // CDgain = ppi / cpi;
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
            resetLoggers();

            // get history from the queue
            List<MouseEventLog> history = events.ToList<MouseEventLog>();

            // calculate speeds.
            List<double> position_x = new List<double>();
            List<double> position_y = new List<double>();
            List<double> acc_x = new List<double>();
            List<double> acc_y = new List<double>();
            List<double> vel_x = new List<double>();
            List<double> vel_y = new List<double>();
            List<double> filtered_acc_x = new List<double>();
            List<double> filtered_acc_y = new List<double>();
            List<double> filtered_vel_x = new List<double>();
            List<double> filtered_vel_y = new List<double>();
            List<double> time_sum = new List<double>();

            List<double> output_speeds = new List<double>();
            List<double> filtered_speeds = new List<double>();

            List<double> input_speeds = new List<double>();
            List<double> timespans = new List<double>();
            double sx_temp = 0;
            double sy_temp = 0;
            double input_dx_temp = 0;
            double input_dy_temp = 0;
            double output_dx_temp = 0;
            double output_dy_temp = 0;
            double tx;
            double ty;


            // Obtaining position, veloctity, speed, acceleration
            #region kinematic measures


            double time_temp = -history[0].timespan;
            double time_buffer = -history[0].timespan;

            foreach (var displacement in history)
            {
                time_temp += displacement.timespan;
                sx_temp += displacement.systemDX / ppm;
                sy_temp += displacement.systemDY / ppm;
                time_buffer += displacement.timespan;
                input_dx_temp += displacement.deviceDX;
                input_dy_temp += displacement.deviceDY;
                output_dx_temp += displacement.systemDX;
                output_dy_temp += displacement.systemDY;

                if (time_buffer >= time_buffer_threshold)
                {

                    output_speeds.Add(Math.Sqrt(output_dx_temp * output_dx_temp + output_dy_temp * output_dy_temp) / ppm / (time_buffer / 1000));
                    input_speeds.Add(Math.Sqrt(input_dx_temp * input_dx_temp + input_dy_temp * input_dy_temp) / cpm / (time_buffer / 1000));

                    input_dx_temp = 0;
                    input_dy_temp = 0;
                    output_dx_temp = 0;
                    output_dy_temp = 0;

                    timespans.Add(time_buffer);
                    time_buffer = 0;

                    time_sum.Add(time_temp / 1000.0);
                    position_x.Add(sx_temp);
                    position_y.Add(sy_temp);
                }
            }
            if (position_x.Count == 0)
            {
                return;
            }

            tx = position_x.Last();
            ty = position_y.Last();
            click_location_x = tx;
            click_location_y = ty;

            vel_x.Add(0);
            vel_y.Add(0);
            for (int i = 1; i < position_x.Count - 1; i++)
            {
                vel_x.Add((position_x[i + 1] - position_x[i - 1]) / (time_sum[i + 1] - time_sum[i - 1]));
                vel_y.Add((position_y[i + 1] - position_y[i - 1]) / (time_sum[i + 1] - time_sum[i - 1]));
            }
            vel_x.Add(0);
            vel_y.Add(0);

            double[] kernel = { 0.0, 0, 0.27901, 0.44198, 0.27901, 0, 0.0 };

            for (int j = 0; j < vel_x.Count; j++)
            {
                double value_vx = 0;
                double value_vy = 0;

                double kernel_sum = 0;
                for (int k = -3; k < 3; k++)
                {
                    if ((j + k) >= 0 && (j + k) < vel_x.Count)
                    {
                        value_vx += vel_x[j + k] * kernel[k + 3];
                        value_vy += vel_y[j + k] * kernel[k + 3];

                        kernel_sum += kernel[k + 3];
                    }
                }
                filtered_vel_x.Add(value_vx / kernel_sum);
                filtered_vel_y.Add(value_vy / kernel_sum);
            }

            acc_x.Add(0);
            acc_y.Add(0);
            for (int i = 1; i < vel_x.Count - 1; i++)
            {
                acc_x.Add((filtered_vel_x[i + 1] - filtered_vel_x[i - 1]) / (time_sum[i + 1] - time_sum[i - 1]));
                acc_y.Add((filtered_vel_y[i + 1] - filtered_vel_y[i - 1]) / (time_sum[i + 1] - time_sum[i - 1]));
            }
            acc_x.Add(0);
            acc_y.Add(0);


            for (int j = 0; j < output_speeds.Count; j++)
            {
                double value = 0;
                double value_ax = 0;
                double value_ay = 0;

                double kernel_sum = 0;
                for (int k = -3; k < 3; k++)
                {
                    if ((j + k) >= 0 && (j + k) < output_speeds.Count)
                    {
                        value += output_speeds[j + k] * kernel[k + 3];
                        value_ax += acc_x[j + k] * kernel[k + 3];
                        value_ay += acc_y[j + k] * kernel[k + 3];

                        kernel_sum += kernel[k + 3];
                    }
                }
                filtered_speeds.Add(value / kernel_sum);
                filtered_acc_x.Add(value_ax / kernel_sum);
                filtered_acc_y.Add(value_ay / kernel_sum);
            }

            #endregion

            // TODO: speed array filtering => adaptive thresholding (relative to filtered_speeds.Max)?
            double persistence1d_threshold = 0.03;

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

            // if there's too little submovements, exit.
            if (mins.Count <= 1 || maxs.Count <= 1)
            {
                return;
            }

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
                double sx_start = position_x[mins[i]];
                double sy_start = position_y[mins[i]];
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
                    double sx_end = position_x[j];
                    double sy_end = position_y[j];
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

            int first_non_clutching_submovement = is_clutching_min_aligned.Count - 1;
            for (int i = first_min_index; i < is_clutching_min_aligned.Count - 1; i++)
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

            //Counting each number of submovement types for later logging stagef
            #region Count each number of submovement types
            for (int i = first_min_index; i < is_clutching_min_aligned.Count - 1; i++)
            {
                if (is_clutching_min_aligned[i] == 1)
                {
                    clutching_submovement_count++;
                }

                if (is_unaimed[i] == 1)
                {
                    unaimed_submovement_count++;
                }

                if (is_interrupted[i] == 1)
                {
                    interrupted_submovement_count++;
                }
            }
            total_submovement_count = mins.Count - 1;
            net_submovement_count = mins.Count - 1 - first_min_index;

            if (index_last_ballistic < (mins.Count - 2))
            {
                non_ballistic_submovement_count = mins.Count - 1 - index_last_ballistic - 1;
                ballistic_submovement_count = index_last_ballistic - first_non_clutching_submovement + 1;
            }
            else
            {
                non_ballistic_submovement_count = 0;
                ballistic_submovement_count = mins.Count - 1 - first_non_clutching_submovement;
            }

            #endregion


            //Logging data only for submovements of interest
            List<double> part_display_speed = filtered_speeds.GetRange(mins[first_min_index], filtered_speeds.Count - mins[first_min_index]);
            List<double> part_motor_speed = input_speeds.GetRange(mins[first_min_index], input_speeds.Count - mins[first_min_index]);
            List<double> part_postion_x = position_x.GetRange(mins[first_min_index], position_x.Count - mins[first_min_index]);
            List<double> part_postion_y = position_y.GetRange(mins[first_min_index], position_y.Count - mins[first_min_index]);
            List<double> part_time_sum = time_sum.GetRange(mins[first_min_index], time_sum.Count - mins[first_min_index]);
            List<double> part_acc_x = filtered_acc_x.GetRange(mins[first_min_index], filtered_acc_x.Count - mins[first_min_index]);
            List<double> part_acc_y = filtered_acc_y.GetRange(mins[first_min_index], filtered_acc_y.Count - mins[first_min_index]);

            maximum_motor_speed = part_motor_speed.Max();
            maximum_display_speed = part_display_speed.Max();
            average_motor_speed = part_motor_speed.Average();
            average_display_speed = part_display_speed.Average();

            for (int i = 0; i < part_postion_x.Count - 1; i++)
            {
                double temp_dx = part_postion_x[i + 1] - part_postion_x[i];
                double temp_dy = part_postion_y[i + 1] - part_postion_y[i];

                trajectory_length += Math.Sqrt(temp_dx * temp_dx + temp_dy * temp_dy);
            }

            int speed_peak_index = 0;
            double temp_speed = 0;

            for (int i = 0; i < part_display_speed.Count; i++)
            {
                if (part_display_speed[i] > temp_speed)
                {
                    speed_peak_index = i;
                    temp_speed = part_display_speed[i];
                }
            }

            acc_duration_ratio = (part_time_sum[speed_peak_index] - part_time_sum.First()) / (part_time_sum.Last() - part_time_sum.First());
            total_duration = (part_time_sum.Last() - part_time_sum.First());

            double current_sign = 0;
            for (int i = 0; i < part_acc_x.Count; i++)
            {
                if (part_acc_x[i] != 0)
                {
                    current_sign = Math.Sign(part_acc_x[i]);
                    break;
                }
            }

            for (int i = 0; i < part_acc_x.Count; i++)
            {
                if (Math.Sign(part_acc_x[i]) != current_sign && Math.Sign(part_acc_x[i]) != 0)
                {
                    current_sign = Math.Sign(part_acc_x[i]);
                    force_inefficiency_x++;
                }
            }

            current_sign = 0;
            for (int i = 0; i < part_acc_y.Count; i++)
            {
                if (part_acc_y[i] != 0)
                {
                    current_sign = Math.Sign(part_acc_y[i]);
                    break;
                }
            }

            for (int i = 0; i < part_acc_y.Count; i++)
            {
                if (Math.Sign(part_acc_y[i]) != current_sign && Math.Sign(part_acc_y[i]) != 0)
                {
                    current_sign = Math.Sign(part_acc_y[i]);
                    force_inefficiency_y++;
                }
            }

            // Start updating gain function per each submovemen
            #region updating gain function

            int number_aimed = 0;
            for (int i = index_right - 1; i >= first_min_index; i--)
            {
                if (is_unaimed[i] != 1)
                {
                    number_aimed++;
                }
            }

            if (number_aimed <= 1)
            {
                return;
            }

            int overshootCount = 0;

            for (int i = index_right - 1; i >= first_min_index; i--)
            {

                double sx_end = position_x[mins[i + 1]];
                double sy_end = position_y[mins[i + 1]];
                double sx_start = position_x[mins[i]];
                double sy_start = position_y[mins[i]];
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

                overshoot_or_undershoot = i.ToString() + "," + is_clutching_min_aligned[i].ToString() + "," + is_interrupted[i].ToString() + "," + is_unaimed[i].ToString() + "," + longitudinal_error.ToString();
                writeLog(logger, "overshoot", overshoot_or_undershoot);
                overshootCount++;

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

            //writeLog(logger, "Overshoot", ""+overshootCount);

            double gainChangeSum = 0;
            for (int i = 0; i < binCount; i++)
            {
                gainChangeSum += Math.Abs(gainChanges[i]);
                gainChanges[i] = 0.0;
            }

            /* Smoothing
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
            */

            net_gain_change = gainChangeSum;

            //            writeLog("GainChange", gainChangeSum.ToString());
            #endregion

            if (is_updated)
            {
                Console.WriteLine("Updated");
                saveAutoGain();
            }

            writeLog(logger, "logBundle", makeLogBundle());
        }

        /// <summary>
        /// Reset all logging variables
        /// </summary>
        /// <param name="instant_aim_point"></param>
        public void resetLoggers()
        {
            //sub_aim_point variable should not be reset;
            unaimed_submovement_count = 0;
            clutching_submovement_count = 0;
            interrupted_submovement_count = 0;
            total_submovement_count = 0;
            net_submovement_count = 0;
            ballistic_submovement_count = 0;
            non_ballistic_submovement_count = 0;
            maximum_motor_speed = 0;
            maximum_display_speed = 0;
            average_motor_speed = 0;
            average_display_speed = 0;
            net_gain_change = 0;
            trajectory_length = 0;
            acc_duration_ratio = 0;
            click_location_x = 0;
            click_location_y = 0;
            overshoot_or_undershoot = "";

            force_inefficiency_x = 0;
            force_inefficiency_y = 0;
            total_duration = 0;
        }

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

        public StreamWriter openLog()
        {
            String pathString = getLogPath();
            String filename = "0_log.csv";
            String fileString = Path.Combine(pathString, filename);

            Directory.CreateDirectory(pathString);
            return new StreamWriter(fileString, true, Encoding.UTF8);
        }

        public void writeLog(StreamWriter sw, String logType, String value)
        {
            sw.WriteLine("{0},{1},{2}", DateTime.Now.ToString("yyyyMMdd_HHmmss"), logType, value);
            sw.Flush();
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

        private String makeLogBundle()
        {
            String logStr = "" + ballistic_submovement_count;
            logStr += "," + total_submovement_count;
            logStr += "," + net_submovement_count;
            logStr += "," + non_ballistic_submovement_count;
            logStr += "," + unaimed_submovement_count;
            logStr += "," + clutching_submovement_count;
            logStr += "," + interrupted_submovement_count;
            logStr += "," + trajectory_length;
            logStr += "," + net_gain_change;
            logStr += "," + force_inefficiency_x;
            logStr += "," + force_inefficiency_y;
            logStr += "," + acc_duration_ratio;
            logStr += "," + click_location_x;
            logStr += "," + click_location_y;
            logStr += "," + average_motor_speed;
            logStr += "," + average_display_speed;
            logStr += "," + maximum_motor_speed;
            logStr += "," + maximum_display_speed;
            logStr += "," + total_duration;
            logStr += "," + sub_aim_point;
            
            return logStr;
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

        #region windows mouse curve related functions


        [DllImport("user32.dll", SetLastError = true)]
        static extern bool SystemParametersInfo(int uiAction, int uiParam, IntPtr pvParam, int fWinIni);

        // get system mouse setting :: speed tick (1~20) / Enhanced Pointer Precision option val)
        void getMouseParameters(out int speed, out bool enhancedPointerPrecision)
        {
            speed = SystemInformation.MouseSpeed;
            enhancedPointerPrecision = false;

            const int SPI_GETMOUSE = 0x03;

            Int32[] mouseParams = new Int32[3];
            IntPtr unmanagedPointer = Marshal.AllocHGlobal(mouseParams.Length * sizeof(Int32));

            // Get the current values.
            bool result = SystemParametersInfo(SPI_GETMOUSE, 0, unmanagedPointer, 0);
            Marshal.Copy(unmanagedPointer, mouseParams, 0, mouseParams.Length);
            Marshal.FreeHGlobal(unmanagedPointer);

            if (result)
            {
                if (mouseParams[2] != 0)
                    enhancedPointerPrecision = true;
            }
        }

        double[] winMultipliers =
        {
            0.03125,
            0.0625,
            0.25,
            0.5,
            0.75,
            1,
            1.5,
            2,
            2.5,
            3,
            3.5
        };

        double[] epp = 
        {
            0	,
            0.58	,
            0.655	,
            0.726666667	,
            0.7675	,
            0.844	,
            0.926666667	,
            0.982857143	,
            1.0275	,
            1.061111111	,
            1.088	,
            1.11	,
            1.128333333	,
            1.143846154	,
            1.207142857	,
            1.31	,
            1.4	,
            1.48	,
            1.55	,
            1.613157895	,
            1.6705	,
            1.721904762	,
            1.768181818	,
            1.811304348	,
            1.850416667	,
            1.8864	,
            1.919615385	,
            1.950740741	,
            1.978928571	,
            2.005517241	,
            2.030666667	,
            2.053548387	,
            2.075625	,
            2.096060606	,
            2.115294118	,
            2.133428571	,
            2.150555556	,
            2.166756757	,
            2.182105263	,
            2.196923077	,
            2.2105	,
            2.223902439	,
            2.236428571	,
            2.248139535	,
            2.259772727	,
            2.270666667	,
            2.281086957	,
            2.29106383	,
            2.300833333	,
            2.309795918	,
            2.3186	,
            2.327254902	,
            2.335192308	,
            2.343207547	,
            2.350740741	,
            2.358	,
            2.365	,
            2.371754386	,
            2.378275862	,
            2.384576271	,
            2.390833333	,
            2.396557377	,
            2.402419355	,
            2.407936508	,
            2.413125	,
            2.418461538	,
            2.423484848	,
            2.428358209	,
            2.433088235	,
            2.437681159	,
            2.442285714	,
            2.446478873	,
            2.450833333	,
            2.454794521	,
            2.458918919	,
            2.4628	,
            2.466578947	,
            2.47025974	,
            2.473846154	,
            2.477341772	,
            2.48075	,
            2.484197531	,
            2.487317073	,
            2.49060241	,
            2.493571429	,
            2.496705882	,
            2.499651163	,
            2.502528736	,
            2.505340909	,
            2.508089888	,
            2.510777778	,
            2.513516484	,
            2.515978261	,
            2.518602151	,
            2.520957447	,
            2.523473684	,
            2.525833333	,
            2.52814433	,
            2.530408163	,
            2.532626263	,
            2.5348	,
            2.536930693	,
            2.539117647	,
            2.541067961	,
            2.543173077	,
            2.545047619	,
            2.547075472	,
            2.548971963	,
            2.550833333	,
            2.55266055	,
            2.554454545	,
            2.556216216	,
            2.558035714	,
            2.559646018	,
            2.561403509	,
            2.562956522	,
            2.564655172	,
            2.566239316	,
            2.56779661	,
            2.569327731	,
            2.570833333	,
            2.57231405	,
            2.573770492	,
            2.575284553	,
            2.576612903	,
            2.57808	,
            2.579365079	,
            2.580787402	,
        };

        #endregion
    }
}
