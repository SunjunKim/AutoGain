using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MouseTester
{
    public struct MouseEvent
    {
        public ushort buttonflags;
        public int lastx;
        public int lasty;
        public long usTimestamp;
        public string source;

        public MouseEvent(ushort buttonflags, int lastx, int lasty, long usTimestamp, string source)
        {
            this.buttonflags = buttonflags;
            this.lastx = lastx;
            this.lasty = lasty;
            this.usTimestamp = usTimestamp;
            this.source = source;
        }
    }

    
}