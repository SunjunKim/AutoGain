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
        // structure marshaling
        struct PairedExtrema
        {
            public PairedExtrema(int min, int max, float pers)
            {
                MinIndex = min;
                MaxIndex = max;
                Persistence = pers;
            }

	        int MinIndex;
            int MaxIndex;
	        float Persistence;	
        };

        persistence1d.p1d p;

        public AutoGain()
        {
            p = new persistence1d.p1d();

            // get 1000 random data
            List<float> list = new List<float>();
            Random rnd = new Random();
            for(int i=0;i<100;i++)
            {
                list.Add(rnd.Next(100));
            }

            p.RunPersistence(list);

            List<PairedExtrema> pairs = new List<PairedExtrema>();
            List<int> mins = new List<int>();
            List<int> maxs = new List<int>();
            List<float> pers = new List<float>();

            p.GetPairedExtrema(mins, maxs, pers, 50f);
            for (int i = 0; i < mins.Count;i++ )
            {
                PairedExtrema np = new PairedExtrema(mins[i], maxs[i], pers[i]);
                pairs.Add(np);
            }
        }   
    }
}
