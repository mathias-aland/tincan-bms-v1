using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace tincanbms_cfg
{
    public partial class frmOptions : Form
    {
        public frmOptions()
        {
            InitializeComponent();
        }

        private void frmOptions_Load(object sender, EventArgs e)
        {
            cbComPort.Items.AddRange(System.IO.Ports.SerialPort.GetPortNames());
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            
        }
    }
}
