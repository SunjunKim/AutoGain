using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace CustomMouseCurve
{
    public partial class SetupForm : Form
    {
        Form1 parent;
        public event EventHandler UpdateData;
        public int newCPI { get {
            int output = 0;
            if(int.TryParse(textBoxNewCPI.Text, out output))
                return output;
            else
                return 0;
        } }

        public SetupForm(Form1 parent, double CPI, double PPI)
        {
            InitializeComponent();
            this.parent = parent;
            textBoxInfo.Text += "\r\n# CPI = " + CPI;
            textBoxInfo.Text += "\r\nCPI means Counts per Inch for a pointing device." ;
            textBoxInfo.Text += "\r\n\r\n# PPI = " + PPI;
            textBoxInfo.Text += "\r\nPPI means Dots per Inch for a display";
        }

        public virtual void OnUpdateData(EventArgs e)
        {
            EventHandler eh = UpdateData;
            if (eh != null)
            {
                eh(this, e);
            }
        }
        
        protected override void WndProc(ref Message m)
        {
            const int WM_NCLBUTTONDOWN = 0x00A1;
            const int WM_NCLBUTTONUP = 0x00A2;

            // detect form title bar click, and disable translation.
            switch (m.Msg)
            {
                case WM_NCLBUTTONDOWN:
                    parent.pauseTranslate();
                    m.Result = IntPtr.Zero;
                    break;

                case WM_NCLBUTTONUP:
                    parent.resumeTranslate();
                    m.Result = IntPtr.Zero;
                    break;
            }

            base.WndProc(ref m);
        }

        private void buttonUpdate_Click(object sender, EventArgs e)
        {
            OnUpdateData(e);
        }
    }
}
