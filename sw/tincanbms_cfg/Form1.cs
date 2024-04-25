using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Serialization;

namespace tincanbms_cfg
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }


        BindingList<batteryData> lBattData = new BindingList<batteryData>();

        public class batteryData : INotifyPropertyChanged
        {

            private int u_addr;
            private double u_module;
            private double u_cell1;
            private double u_cell2;
            private double u_cell3;
            private double u_cell4;
            private double u_cell5;
            private double u_cell6;
            private double temp_1;
            private double temp_2;

            private byte mod_faults;
            private byte mod_alerts;
            private byte mod_status;
            private byte mod_balState;

            public int addr { get { return u_addr; } set { if (u_addr != value) { u_addr = value; NotifyPropertyChanged("addr"); } } }
            public double modVolt { get { return u_module; } set { if (u_module != value) { u_module = value; NotifyPropertyChanged("modVolt"); } } } 
            public double cell1Volt { get { return u_cell1; } set { if (u_cell1 != value) { u_cell1 = value; NotifyPropertyChanged("cell1Volt"); } } }
            public double cell2Volt { get { return u_cell2; } set { if (u_cell2 != value) { u_cell2 = value; NotifyPropertyChanged("cell2Volt"); } } }
            public double cell3Volt { get { return u_cell3; } set { if (u_cell3 != value) { u_cell3 = value; NotifyPropertyChanged("cell3Volt"); } } }
            public double cell4Volt { get { return u_cell4; } set { if (u_cell4 != value) { u_cell4 = value; NotifyPropertyChanged("cell4Volt"); } } }
            public double cell5Volt { get { return u_cell5; } set { if (u_cell5 != value) { u_cell5 = value; NotifyPropertyChanged("cell5Volt"); } } }
            public double cell6Volt { get { return u_cell6; } set { if (u_cell6 != value) { u_cell6 = value; NotifyPropertyChanged("cell6Volt"); } } }
            public double temp1 { get { return temp_1; } set { if (temp_1 != value) { temp_1 = value; NotifyPropertyChanged("temp1"); } } }
            public double temp2 { get { return temp_2; } set { if (temp_2 != value) { temp_2 = value; NotifyPropertyChanged("temp2"); } } }

            public byte modFaults { get { return mod_faults; } set { if (mod_faults != value) { mod_faults = value; NotifyPropertyChanged("modFaults"); } } }
            public byte modAlerts { get { return mod_alerts; } set { if (mod_alerts != value) { mod_alerts = value; NotifyPropertyChanged("modAlerts"); } } }
            public byte modStatus { get { return mod_status; } set { if (mod_status != value) { mod_status = value; NotifyPropertyChanged("modStatus"); } } }
            public byte modBalState { get { return mod_balState; } set { if (mod_balState != value) { mod_balState = value; NotifyPropertyChanged("modBalState"); } } }




            public event PropertyChangedEventHandler PropertyChanged;

            private void NotifyPropertyChanged(string p)
            {
                if (PropertyChanged != null)
                    PropertyChanged(this, new PropertyChangedEventArgs(p));
            }

            public void NotifyAll ()
            {
                NotifyPropertyChanged("addr");
                NotifyPropertyChanged("modVolt");
                NotifyPropertyChanged("cell1Volt");
                NotifyPropertyChanged("cell2Volt");
                NotifyPropertyChanged("cell3Volt");
                NotifyPropertyChanged("cell4Volt");
                NotifyPropertyChanged("cell5Volt");
                NotifyPropertyChanged("cell6Volt");
                NotifyPropertyChanged("temp1");
                NotifyPropertyChanged("temp2");
                NotifyPropertyChanged("modFaults");
                NotifyPropertyChanged("modAlerts");
                NotifyPropertyChanged("modStatus");
                NotifyPropertyChanged("modBalState");

            }

        }


        public class simData
        {
            public ushort u_cell1;
            public ushort u_cell2;
            public ushort u_cell3;
            public ushort u_cell4;
            public ushort u_cell5;
            public ushort u_cell6;
            public short temp_1;
            public short temp_2;
            public byte status;
            public byte alerts;
            public byte faults;
            public byte bal;
            public byte cov;
            public byte cuv;
        }

        public class paramDataClass
        {
            public uint moduleCount = 0;
            public uint cellVoltHi = 0;
            public uint cellVoltLo = 0;
            public int tempHi = 0;
            public int tempLo = 0;
            public int minCurrent = 0;
            public int minCurrentPeak = 0;
            public int maxCurrent = 0;
            public int maxCurrentPeak = 0;
            public uint contactorRelayTime = 0;
            public uint contactorPrechargeTime = 0;
            public uint contactorOnPulseTime = 0;
            public uint turtlemodeVolt = 0;
            public uint chgDisVolt = 0;
            public uint chgEnVolt = 0;
            public uint chgInhibitVolt = 0;
            public int chgMinTemp = 0;
            public int chgMaxTemp = 0;
            public int regenMinTemp = 0;
            public int regenMaxTemp = 0;
            public uint regenMaxVolt = 0;
            public uint pumpOffSeconds = 0;
            public uint pumpOnSeconds = 0;
            public int pumpOnTemp = 0;
            public int heaterOffTemp = 0;
            public int heaterOnTemp = 0;
            public uint balMinVoltage = 0;
            public uint ignThresholdOn = 40960;
            public uint ignThresholdOff = 0;
            public uint startThresholdOn = 40960;
            public uint startThresholdOff = 0;
            public uint acPresThresholdOn = 40960;
            public uint acPresThresholdOff = 0;
            public uint peakCurrentDelay = 0;
            public uint contactorOffDelay = 0;
            public uint packsExpected = 0;
            public uint contactorPwmFrequency = 1;
            public uint contactorPwmDuty = 0;
            public uint contactorIdleDelay = 0;
            public uint turtlemodeDelay = 0;
            public uint chgDelay = 0;
            public uint acPresDelay = 0;
            public uint regenResetDelay = 0;
            public uint pumpFreqPWM = 1;
            public uint pumpDutyPWM = 0;
            public uint balMaxDelta = 0;
            public uint balFinishDelta = 0;
            public uint balInterval = 1;
            public uint balIdleTime = 0;
            public uint ignGPI = 1;
            public uint startGPI = 1;
            public uint acPresGPI = 1;

            public int out1Func = 0;
            public int out2Func = 0;
            public int out3Func = 0;
            public int out4Func = 0;
            public int rly1Func = 0;
            public int rly2Func = 0;



            public uint secCellOVP = 2000;
            public uint secCellOVPdelay = 100;
            public uint secCellUVP = 700;
            public uint secCellUVPdelay = 100;
            public uint secMaxTemp = 40;
            public uint secMaxTempDelay = 10;
        }

        paramDataClass paramData = new paramDataClass();

        bool battInitPend = false;

        bool exitPend = false;
        private bool dumpCfgPend = false;

        bool simModeActive = false;
        bool simModePend = false;

        bool simWritePend = false;
        bool simReadPend = false;

        bool writeCfgPend = false;
        bool saveCfgPend = false;

        bool enableBootPend = false;
        bool exitBootPend = false;


        bool bootEnabled = false;

        List<simData> simDataList = new List<simData>();


        byte modbus_LRC(byte[] buf, int startPos, int len)
        {
            byte lrc = 0;
            while (len-- > 0)
            {
                lrc += buf[startPos++];
            }

            return (byte)-lrc;
        }

        public static byte[] StringToByteArray(String hex)
        {
            int NumberChars = hex.Length;
            byte[] bytes = new byte[NumberChars / 2];
            for (int i = 0; i < NumberChars; i += 2)
                bytes[i / 2] = Convert.ToByte(hex.Substring(i, 2), 16);
            return bytes;
        }

        byte[] modbusASCII_readReg(Byte slaveAddr, UInt16 regAddr, UInt16 regCnt)
        {
            if (regCnt > 125)
                throw new ArgumentOutOfRangeException(nameof(regCnt), "Too many registers requested");

            byte[] modbus_buf = new byte[7];

            modbus_buf[0] = slaveAddr;  // slave address
            modbus_buf[1] = 4;          // function code (4)
            modbus_buf[2] = (byte)(regAddr >> 8);   // regAddr MSB
            modbus_buf[3] = (byte)(regAddr & 0xff); // regAddr LSB
            modbus_buf[4] = (byte)(regCnt >> 8);   // regCnt MSB
            modbus_buf[5] = (byte)(regCnt & 0xff); // regCnt LSB
            modbus_buf[6] = modbus_LRC(modbus_buf, 1, 5);   // LRC

            string modbusStr = ":" + BitConverter.ToString(modbus_buf,0,7).Replace("-", "");



            serialPort1.WriteLine(modbusStr);

            serialPort1.ReadTimeout = 1000;

            modbusStr = serialPort1.ReadLine();
            return StringToByteArray(modbusStr.Substring(modbusStr.IndexOf(':')+1));

        }
        byte[] modbusASCII_readHoldReg(Byte slaveAddr, UInt16 regAddr, UInt16 regCnt)
        {
            if (regCnt > 125)
                throw new ArgumentOutOfRangeException(nameof(regCnt), "Too many registers requested");

            byte[] modbus_buf = new byte[7];

            modbus_buf[0] = slaveAddr;  // slave address
            modbus_buf[1] = 3;          // function code (3)
            modbus_buf[2] = (byte)(regAddr >> 8);   // regAddr MSB
            modbus_buf[3] = (byte)(regAddr & 0xff); // regAddr LSB
            modbus_buf[4] = (byte)(regCnt >> 8);   // regCnt MSB
            modbus_buf[5] = (byte)(regCnt & 0xff); // regCnt LSB
            modbus_buf[6] = modbus_LRC(modbus_buf, 1, 5);   // LRC

            string modbusStr = ":" + BitConverter.ToString(modbus_buf, 0, 7).Replace("-", "");



            serialPort1.WriteLine(modbusStr);

            serialPort1.ReadTimeout = 1000;

            modbusStr = serialPort1.ReadLine();
            return StringToByteArray(modbusStr.Substring(modbusStr.IndexOf(':') + 1));

        }



        bool modbusASCII_writeHoldRegs(Byte slaveAddr, UInt16 regAddr, UInt16 regCnt, byte[] regData)
        {
            if (regCnt > 125)
                throw new ArgumentOutOfRangeException(nameof(regCnt), "Too many registers requested");

            if (regCnt * 2 != regData.Length)
                throw new ArgumentOutOfRangeException(nameof(regData.Length), "Length mismatch");



            // Reverse bytes (LE -> BE)
            if (BitConverter.IsLittleEndian)
            {
                for (int i = 0; i < regCnt; i++)
                {
                    Array.Reverse(regData, i * 2, 2);
                }
            }




            byte[] modbus_buf = new byte[8 + regData.Length];

            modbus_buf[0] = slaveAddr;  // slave address
            modbus_buf[1] = 16;          // function code (16)
            modbus_buf[2] = (byte)(regAddr >> 8);   // regAddr MSB
            modbus_buf[3] = (byte)(regAddr & 0xff); // regAddr LSB
            modbus_buf[4] = (byte)(regCnt >> 8);   // regCnt MSB
            modbus_buf[5] = (byte)(regCnt & 0xff); // regCnt LSB
            modbus_buf[6] = (byte)(regData.Length); // byte count

            int modbus_buf_idx = 7;


            regData.CopyTo(modbus_buf, modbus_buf_idx);

            modbus_buf_idx += regData.Length;

            modbus_buf[modbus_buf_idx] = modbus_LRC(modbus_buf, 1, modbus_buf_idx-1);   // LRC

            string modbusStr = ":" + BitConverter.ToString(modbus_buf, 0, modbus_buf.Length).Replace("-", "");



            serialPort1.WriteLine(modbusStr);

            serialPort1.ReadTimeout = 1000;

            modbusStr = serialPort1.ReadLine();




            byte[] modbus_resp = StringToByteArray(modbusStr.Substring(modbusStr.IndexOf(':') + 1));

            if (modbus_resp.Length != 7)
            {
                // Length wrong
                return false;
            }


            byte LRC = modbus_LRC(modbus_resp, 1, 5);


            if (LRC != modbus_resp[6])
            {
                // LRC wrong
                return false;
            }

            for (int i=0; i<6; i++)
            {
                if (modbus_resp[i] != modbus_buf[i])
                {
                    // wrong reply
                    return false;
                }

            }



            return true;
        }




        void GetBatteryData()
        {


            //DataTable dt = new DataTable();


            //DataColumn dc = dt.Columns.Add("#", typeof(Int32));
            //dc.Unique = true;


            //dt.Columns.Add("Umod", typeof(double));
            //dt.Columns.Add("U1", typeof(double));
            //dt.Columns.Add("U2", typeof(double));
            //dt.Columns.Add("U3", typeof(double));
            //dt.Columns.Add("U4", typeof(double));
            //dt.Columns.Add("U5", typeof(double));
            //dt.Columns.Add("U6", typeof(double));
            //dt.Columns.Add("T1", typeof(double));
            //dt.Columns.Add("T2", typeof(double));

            //dt.PrimaryKey = new DataColumn[] { dt.Columns["#"] };

            //Random rnd = new Random();

            //for (int i = 1; i < 17; i++)
            //{
            //    DataRow dr = dt.NewRow();

            //    dr["#"] = i;
            //    dr["Umod"] = 20.0 + (rnd.NextDouble() * 2);
            //    dr["U1"] = 3.2 + rnd.NextDouble();
            //    dr["U2"] = 3.2 + rnd.NextDouble();
            //    dr["U3"] = 3.2 + rnd.NextDouble();
            //    dr["U4"] = 3.2 + rnd.NextDouble();
            //    dr["U5"] = 3.2 + rnd.NextDouble();
            //    dr["U6"] = 3.2 + rnd.NextDouble();
            //    dr["T1"] = 10.0 + (rnd.NextDouble() * 20);
            //    dr["T2"] = 10.0 + (rnd.NextDouble() * 20);
            //    dt.Rows.Add(dr);
            //}



            //return dt;

            //BindingList<batteryData> lBattData = new BindingList<batteryData>();


                

            if (lBattData.Count == 0)
            {
                for (int i = 1; i < 17; i++)
                {
                    batteryData bData = new batteryData();
                    bData.addr = i;

                    this.Invoke((MethodInvoker)delegate
                    {
                        // Run on UI thread:
                        lBattData.Add(bData);
                        bData.NotifyAll();
                    });

                    

                }
            }

            //Random rnd = new Random();

            for (int i = 1; i < 17; i++)
            {
                batteryData bData = lBattData[i - 1];


                if (serialPort1.IsOpen)
                {
                    byte[] modb = modbusASCII_readReg(0, (ushort)(100*i), 11);

                    this.Invoke((MethodInvoker)delegate
                    {
                        // Run on UI thread:
                        bData.modVolt = (((UInt16)modb[3] << 8) | modb[4]) / 1000.0;
                        bData.cell1Volt = (((UInt16)modb[5] << 8) | modb[6]) / 1000.0;
                        bData.cell2Volt = (((UInt16)modb[7] << 8) | modb[8]) / 1000.0;
                        bData.cell3Volt = (((UInt16)modb[9] << 8) | modb[10]) / 1000.0;
                        bData.cell4Volt = (((UInt16)modb[11] << 8) | modb[12]) / 1000.0;
                        bData.cell5Volt = (((UInt16)modb[13] << 8) | modb[14]) / 1000.0;
                        bData.cell6Volt = (((UInt16)modb[15] << 8) | modb[16]) / 1000.0;
                        bData.temp1 = System.Net.IPAddress.NetworkToHostOrder(BitConverter.ToInt16(modb, 17)) / 100.0;
                        bData.temp2 = System.Net.IPAddress.NetworkToHostOrder(BitConverter.ToInt16(modb, 19)) / 100.0;

                        bData.modAlerts = modb[21];
                        bData.modFaults = modb[22];
                        bData.modStatus = modb[23];
                        bData.modBalState = modb[24];


                    });


                    //bData.temp1 = (((Int16)modb[17] << 8) | modb[18]) / 10.0;
                    //bData.temp2 = (((Int16)modb[19] << 8) | modb[20]) / 10.0;

                }

                //bData.cell1Volt = 4.0 + rnd.NextDouble()/2;
                //bData.cell2Volt = 4.0 + rnd.NextDouble() / 2;
                //bData.cell3Volt = 4.0 + rnd.NextDouble() / 2;
                //bData.cell4Volt = 4.0 + rnd.NextDouble() / 2;
                //bData.cell5Volt = 4.0 + rnd.NextDouble() / 2;
                //bData.cell6Volt = 4.0 + rnd.NextDouble() / 2;
                //bData.modVolt = bData.cell1Volt + bData.cell2Volt + bData.cell3Volt + bData.cell4Volt + bData.cell5Volt + bData.cell6Volt;
                //bData.temp1 = 20.0 + (rnd.NextDouble()/4);
                //bData.temp2 = 20.0 + (rnd.NextDouble()/4);
            }


            //batteryData bData = new batteryData();

            //Random rnd = new Random();



            //bData.modVolt = 20.2;


            //bData.temp1 = 23.3;





            //bData.temperature.Add(24.5);
            //bData.cellVolt = new List<double>();
            //bData.cellVolt.Add(4.1);
            //bData.cellVolt.Add(4.0);
            //bData.cellVolt.Add(3.9);
            //bData.cellVolt.Add(3.85);
            //bData.cellVolt.Add(4.15);
            //bData.cellVolt.Add(4.12);
            //lBattData.Add(bData);

            //bData = new batteryData();
            //bData.modVolt = 20.2;
            //bData.temperature = new List<double>();
            //bData.temperature.Add(23.3);
            //bData.temperature.Add(24.5);
            //bData.cellVolt = new List<double>();
            //bData.cellVolt.Add(4.1);
            //bData.cellVolt.Add(4.0);
            //bData.cellVolt.Add(3.9);
            //bData.cellVolt.Add(3.85);
            //bData.cellVolt.Add(4.15);
            //bData.cellVolt.Add(4.12);
            //lBattData.Add(bData);

            //return lBattData;

        }

        private void optionsToolStripMenuItem1_Click(object sender, EventArgs e)
        {
            frmOptions optDlg = new frmOptions();

            optDlg.cbComPort.Text = Properties.Settings.Default.com_port;

            DialogResult dr= optDlg.ShowDialog();

            if (dr == DialogResult.OK)
            {
                Properties.Settings.Default.com_port = optDlg.cbComPort.Text;
                Properties.Settings.Default.Save();
            }
        }


        public static void SetDoubleBuffered(System.Windows.Forms.Control c)
        {
            //Taxes: Remote Desktop Connection and painting
            //http://blogs.msdn.com/oldnewthing/archive/2006/01/03/508694.aspx
            if (System.Windows.Forms.SystemInformation.TerminalServerSession)
                return;

            System.Reflection.PropertyInfo aProp =
                  typeof(System.Windows.Forms.Control).GetProperty(
                        "DoubleBuffered",
                        System.Reflection.BindingFlags.NonPublic |
                        System.Reflection.BindingFlags.Instance);

            aProp.SetValue(c, true, null);
        }


        private void Form1_Load(object sender, EventArgs e)
        {

            SetDoubleBuffered(dbgData);
            SetDoubleBuffered(dbgData2);
            SetDoubleBuffered(lvLockoutFlags);
            SetDoubleBuffered(dataGridView1);

            dataGridView1.AutoGenerateColumns = false;
            dataGridView1.DataSource = lBattData;

            updateParamsGUI();
            

            for (int i=0; i<16; i++)
            {
                simData sd = new simData();

                sd.alerts = 0;
                sd.bal = 0;
                sd.cov = 0;
                sd.cuv = 0;
                sd.faults = 0;
                sd.status = 0;
                sd.temp_1 = 10*100;
                sd.temp_2 = 10*100;
                sd.u_cell1 = 4000;
                sd.u_cell2 = 4000;
                sd.u_cell3 = 4000;
                sd.u_cell4 = 4000;
                sd.u_cell5 = 4000;
                sd.u_cell6 = 4000;

                simDataList.Add(sd);


            }

            foreach (ListViewItem itm in dbgData.Items)
            {
                itm.SubItems.Add("N/A");
                itm.SubItems.Add("N/A");
            }

            foreach (ListViewItem itm in dbgData2.Items)
            {
                itm.SubItems.Add("N/A");
            }

            foreach (ListViewItem itm in lvLockoutFlags.Items)
            {
                itm.Font = new System.Drawing.Font("Consolas", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point);
                itm.SubItems.Add("N/A");
            }



            //GetBatteryData();




            //dataGridView1.Columns[1].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[2].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[3].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[4].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[5].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[6].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[7].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[8].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[9].DefaultCellStyle.Format = "N2";

            //dataGridView1.AutoResizeColumns(DataGridViewAutoSizeColumnsMode.DisplayedCells);

            //timer1.Start();
            //backgroundWorker1.RunWorkerAsync();


        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            //dataGridView1.SuspendLayout();
           // dataGridView1.DataSource = null;
           // GetBatteryData();


            //dataGridView1.Columns[1].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[2].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[3].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[4].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[5].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[6].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[7].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[8].DefaultCellStyle.Format = "N2";
            //dataGridView1.Columns[9].DefaultCellStyle.Format = "N2";

            //dataGridView1.AutoResizeColumns(DataGridViewAutoSizeColumnsMode.DisplayedCells);



            //dataGridView1.ResumeLayout();
        }

        private void dataGridView1_CellFormatting(object sender, DataGridViewCellFormattingEventArgs e)
        {
            if ((e.ColumnIndex == dataGridView1.Columns["cell1"].Index) ||
                (e.ColumnIndex == dataGridView1.Columns["cell2"].Index) ||
                (e.ColumnIndex == dataGridView1.Columns["cell3"].Index) ||
                (e.ColumnIndex == dataGridView1.Columns["cell4"].Index) ||
                (e.ColumnIndex == dataGridView1.Columns["cell5"].Index) ||
                (e.ColumnIndex == dataGridView1.Columns["cell6"].Index))
            {
                // Cell voltage
                if ((double)e.Value > 4.3)
                {
                   // e.CellStyle.BackColor = Color.Red;
                    //e.CellStyle.SelectionBackColor = Color.DarkRed;
                }
                else if ((double)e.Value > 4.2)
                {
                   // e.CellStyle.BackColor = Color.Yellow;
                   // e.CellStyle.SelectionBackColor = Color.Olive;
                }
                else
                {
                   // e.CellStyle.BackColor = Color.Green;
                   // e.CellStyle.SelectionBackColor = Color.DarkGreen;
                }
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            battInitPend = true;
            button1.Enabled = false;

        }

        private void backgroundWorker1_DoWork(object sender, DoWorkEventArgs e)
        {
            bool firstFetch = true;

            try
            {
                serialPort1.NewLine = "\r\n";
                serialPort1.PortName = Properties.Settings.Default.com_port;

                serialPort1.Open();

                serialPort1.WriteTimeout = 1000;
            }
            catch (Exception ex)
            {
                this.Invoke((MethodInvoker)delegate
                {
                    // Run on UI thread:
                    MessageBox.Show("Error: " + ex.Message, "Init COM port", MessageBoxButtons.OK, MessageBoxIcon.Error);
                });
                return;
            }


            while (exitPend == false)
            {
                if ((enableBootPend) && (!bootEnabled))
                {

                    try
                    {
                        serialPort1.Write(":00050007FF00F5\r\n");
                        bootEnabled = true;

                    }
                    catch (Exception ex)
                    {
                        this.Invoke((MethodInvoker)delegate
                        {
                            // Run on UI thread:
                            MessageBox.Show("Error: " + ex.Message, "Enable BOOT mode", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        });
                    }

                    enableBootPend = false;

                    this.Invoke((MethodInvoker)delegate
                    {
                        // Run on UI thread:
                        btnEnBoot.Enabled = true;
                    });




                    Thread.Sleep(100);
                    continue;
                }

                if (bootEnabled)
                {
                    if (exitBootPend)
                    {

                        if (serialPort1.IsOpen)
                            serialPort1.Close();

                        Thread.Sleep(100);

                        updi updi1 = new updi(Properties.Settings.Default.com_port, true);

                        try
                        {

                            updi1.open();

                            updi1.updi_link_init();
                            updi1.updi_cpu_reset();

                            updi1.close();

                            Thread.Sleep(5000);

                            bootEnabled = false;

                        }
                        catch (Exception ex)
                        {
                            updi1.close();

                            this.Invoke((MethodInvoker)delegate
                            {
                                // Run on UI thread:
                                MessageBox.Show("Error: " + ex.Message, "Exit BOOT mode", MessageBoxButtons.OK, MessageBoxIcon.Error);
                            });
                        }


                        try
                        {

                            serialPort1.NewLine = "\r\n";
                            serialPort1.PortName = Properties.Settings.Default.com_port;

                            serialPort1.Open();

                            serialPort1.WriteTimeout = 1000;


                        }
                        catch (Exception ex)
                        {
                            this.Invoke((MethodInvoker)delegate
                            {
                                // Run on UI thread:
                                MessageBox.Show("Error: " + ex.Message, "Init COM port", MessageBoxButtons.OK, MessageBoxIcon.Error);
                            });
                        }


                        exitBootPend = false;

                        this.Invoke((MethodInvoker)delegate
                        {
                            // Run on UI thread:
                            btnExitBoot.Enabled = true;
                        });

                    }

                    Thread.Sleep(100);
                    continue;
                }


                if (battInitPend)
                {


                    try
                    {
                        serialPort1.Write(":00050003FF00F9\r\n");
                        serialPort1.ReadTimeout = 1000;
                        serialPort1.ReadLine();
                    }
                    catch (Exception ex)
                    {
                        this.Invoke((MethodInvoker)delegate
                        {
                            // Run on UI thread:
                            MessageBox.Show("Error: " + ex.Message, "Battery init", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        });
                    }
                    battInitPend = false;

                    this.Invoke((MethodInvoker)delegate
                    {
                        // Run on UI thread:
                        button1.Enabled = true;
                    });
                }

                if (dumpCfgPend)
                {

                    try
                    {

                        if (serialPort1.IsOpen)
                        {
                            byte[] modb = modbusASCII_readHoldReg(0, 0, 61);

                            this.Invoke((MethodInvoker)delegate
                            {

                                // Reverse bytes (BE -> LE)
                                if (BitConverter.IsLittleEndian)
                                {
                                    for (int i = 0; i<61; i++)
                                    {
                                        Array.Reverse(modb, 3+(i*2), 2);
                                    }

                                    
                                }
                                MemoryStream stream = new MemoryStream(modb);
                                BinaryReader br = new BinaryReader(stream);

                                br.ReadByte();  // Slave address
                                br.ReadByte();  // Function code
                                br.ReadByte();  // Byte count
















                                paramData.cellVoltHi = br.ReadUInt16();
                                paramData.cellVoltLo = br.ReadUInt16();
                                paramData.tempHi = br.ReadInt16();
                                paramData.tempLo = br.ReadInt16();
                                paramData.minCurrent = br.ReadInt16();
                                paramData.minCurrentPeak = br.ReadInt16();
                                paramData.maxCurrent = br.ReadInt16();
                                paramData.maxCurrentPeak = br.ReadInt16();

                                paramData.contactorRelayTime = br.ReadUInt16();
                                paramData.contactorPrechargeTime = br.ReadUInt16();
                                paramData.contactorOnPulseTime = br.ReadUInt16();

                                paramData.turtlemodeVolt = br.ReadUInt16();

                                paramData.chgDisVolt = br.ReadUInt16();
                                paramData.chgEnVolt = br.ReadUInt16();
                                paramData.chgInhibitVolt = br.ReadUInt16();
                                paramData.chgMinTemp = br.ReadInt16();
                                paramData.chgMaxTemp = br.ReadInt16();


                                paramData.regenMinTemp = br.ReadInt16();
                                paramData.regenMaxTemp = br.ReadInt16();
                                paramData.regenMaxVolt = br.ReadUInt16();


                                paramData.pumpOffSeconds = br.ReadUInt16();
                                paramData.pumpOnSeconds = br.ReadUInt16();
                                paramData.pumpOnTemp = br.ReadInt16();
                                paramData.heaterOffTemp = br.ReadInt16();
                                paramData.heaterOnTemp = br.ReadInt16();

                                paramData.balMinVoltage = br.ReadUInt16();

                                paramData.ignThresholdOn = br.ReadUInt16();
                                paramData.ignThresholdOff = br.ReadUInt16();
                                paramData.startThresholdOn = br.ReadUInt16();
                                paramData.startThresholdOff = br.ReadUInt16();
                                paramData.acPresThresholdOn = br.ReadUInt16();
                                paramData.acPresThresholdOff = br.ReadUInt16();

                                paramData.peakCurrentDelay = br.ReadUInt16();
                                paramData.contactorOffDelay = br.ReadUInt16();
                                paramData.moduleCount = br.ReadUInt16();

                                paramData.contactorPwmFrequency = (uint)br.ReadUInt16() + 1;
                                paramData.contactorPwmDuty = br.ReadUInt16();
                                paramData.contactorIdleDelay = br.ReadUInt16();


                                paramData.turtlemodeDelay = br.ReadUInt16();

                                paramData.chgDelay = br.ReadUInt16();
                                paramData.acPresDelay = br.ReadUInt16();

                                paramData.regenResetDelay = br.ReadUInt16();

                                paramData.pumpFreqPWM = (uint)br.ReadUInt16() + 1;
                                paramData.pumpDutyPWM = br.ReadUInt16();

                                paramData.balMaxDelta = br.ReadUInt16();
                                paramData.balFinishDelta = br.ReadUInt16();
                                paramData.balInterval = (uint)(br.ReadUInt16() & 0x3f);
                                paramData.balIdleTime = br.ReadUInt16();

                                br.ReadUInt16();        // alertFaultMask

                                paramData.ignGPI = br.ReadUInt16();
                                paramData.startGPI = br.ReadUInt16();
                                paramData.acPresGPI = br.ReadUInt16();

                                paramData.out1Func = br.ReadUInt16();
                                paramData.out2Func = br.ReadUInt16();
                                paramData.out3Func = br.ReadUInt16();
                                paramData.out4Func = br.ReadUInt16();
                                paramData.rly1Func = br.ReadUInt16();
                                paramData.rly2Func = br.ReadUInt16();


                                // LSB = CONFIG_COV (0x42), MSB = CONFIG_COVT (0x43)
                                //.secCOVreg = 0x80                   // Enable over-voltage protection (CONFIG_COV bit 7)
                                //               | ((4250 - 2000) / 50)      // Set over-voltage protection to 4250 mV	
                                //               | 0x8000                // Timeout unit = ms (CONFIG_COVT bit 7)
                                //               | ((3100 / 100) << 8),		// Timeout = 3100 ms

                                UInt16 regCOV = br.ReadUInt16();

                                paramData.secCellOVP = (uint)((regCOV & 0x7f) * 50 + 2000);
                                paramData.secCellOVPdelay = (uint)(((regCOV >> 8) & 0x7f) * 100);


                                // LSB = CONFIG_CUV (0x44), MSB = CONFIG_CUVT (0x45)
                                //.secCOVreg = 0x80                   // Enable under-voltage protection (CONFIG_CUV bit 7)
                                //           | ((3100 - 700) / 100)      // Set under-voltage protection to 3100 mV
                                //           | 0x8000                    // Timeout unit = ms (CONFIG_CUVT bit 7)
                                //           | ((3100 / 100) << 8),      // Timeout = 3100 ms	

                                UInt16 regCUV = br.ReadUInt16();

                                paramData.secCellUVP = (uint)((regCUV & 0x7f) * 100 + 700);
                                paramData.secCellUVPdelay = (uint)(((regCUV >> 8) & 0x7f) * 100);


                                // LSB = CONFIG_OT (0x46), MSB = CONFIG_OTT (0x47)
                                //.secOTreg = ((65 - 35) / 5)             // Overtemp 1 = 65 degC
                                //        | (((65 - 35) / 5) << 4)        // Overtemp 2 = 65 degC
                                //        | ((2550 / 10) << 8),		      // Timeout = 2550 ms

                                UInt16 regOT = br.ReadUInt16();

                                paramData.secMaxTemp = (uint)((regOT & 0x0f) * 5 + 35);
                                paramData.secMaxTempDelay = (uint)((regOT >> 8) * 10);

                                // rly2Func.SelectedIndex = BitConverter.ToUInt16(modb, 131);

                                //tbParams.Clear();

                                //for (int i=0; i<64; i++)
                                //{
                                //tbParams.Text += (i+1).ToString() + ": " + (((UInt16)modb[(i*2)+3] << 8) | modb[(i*2)+4]).ToString("X4") + "\r\n";
                                //}















                                //conLockoutOVP.Value = br.ReadUInt16();
                                //conLockoutUVP.Value = br.ReadUInt16();
                                //conLockoutMaxTemp.Value = br.ReadInt16() / 100.0m;
                                //conLockoutMinTemp.Value = br.ReadInt16() / 100.0m;
                                //conLockoutMinCurr.Value = br.ReadInt16();
                                //conLockoutMinCurrPeak.Value = br.ReadInt16();
                                //conLockoutMaxCurr.Value = br.ReadInt16();
                                //conLockoutMaxCurrPeak.Value = br.ReadInt16();

                                //contRlyTime.Value = br.ReadUInt16();
                                //contPrechgTime.Value = br.ReadUInt16();
                                //contOnPulseTime.Value = br.ReadUInt16();

                                //turtleThresV.Value = br.ReadUInt16();

                                //chgStopV.Value = br.ReadUInt16();
                                //chgStartV.Value = br.ReadUInt16();
                                //chgInhibitV.Value = br.ReadUInt16();
                                //chgMinTemp.Value = br.ReadInt16() / 100.0m;
                                //chgMaxTemp.Value = br.ReadInt16() / 100.0m;


                                //regenMinTemp.Value = br.ReadInt16() / 100.0m;
                                //regenMaxTemp.Value = br.ReadInt16() / 100.0m;
                                //regenThresholdV.Value = br.ReadUInt16();


                                //heaterPumpOffTime.Value = br.ReadUInt16();
                                //heaterPumpOnTime.Value = br.ReadUInt16();
                                //heaterPumpOnTemp.Value = br.ReadInt16() / 100.0m;
                                //heaterOffTemp.Value = br.ReadInt16() / 100.0m;
                                //heaterOnTemp.Value = br.ReadInt16() / 100.0m;

                                //cellBalMinV.Value = br.ReadUInt16();

                                //inpIgnOnThres.Value = br.ReadUInt16();
                                //inpIgnOffThres.Value = br.ReadUInt16();
                                //inpStartOnThres.Value = br.ReadUInt16();
                                //inpStartOffThres.Value = br.ReadUInt16();
                                //inpAcDetOnThres.Value = br.ReadUInt16();
                                //inpAcPresOffThres.Value = br.ReadUInt16();

                                //conLockoutPeakDelay.Value = br.ReadUInt16();
                                //conLockoutDelay.Value = br.ReadUInt16();
                                //modCnt.Value = br.ReadUInt16();

                                //contPWMfreq.Value = br.ReadUInt16();
                                //contPWMduty.Value = br.ReadUInt16();
                                //contIdleTime.Value = br.ReadUInt16();


                                //turtleOnDelay.Value = br.ReadUInt16();

                                //chgRestartDly.Value = br.ReadUInt16();
                                //chgAcDetDly.Value = br.ReadUInt16();

                                //regenResetDelay.Value = br.ReadUInt16();

                                //heaterPumpFreq.Value = br.ReadUInt16() + 1.0m;
                                //heaterPumpDuty.Value = br.ReadUInt16() / 2.55m;

                                //cellBalStartV.Value = br.ReadUInt16();
                                //cellBalEndV.Value = br.ReadUInt16();
                                //cellBalTimeout.Value = br.ReadUInt16() & 0x3f;
                                //cellBalIdleTime.Value = br.ReadUInt16();

                                //br.ReadUInt16();    // alertFaultMask

                                //inpIgnInNum.Value = br.ReadUInt16();
                                //inpStartInNum.Value = br.ReadUInt16();
                                //inpAcDetInNum.Value = br.ReadUInt16();

                                //out1Func.SelectedIndex = br.ReadUInt16();
                                //out2Func.SelectedIndex = br.ReadUInt16();
                                //out3Func.SelectedIndex = br.ReadUInt16();
                                //out4Func.SelectedIndex = br.ReadUInt16();
                                //rly1Func.SelectedIndex = br.ReadUInt16();
                                //rly2Func.SelectedIndex = br.ReadUInt16();





                                //// LSB = CONFIG_COV (0x42), MSB = CONFIG_COVT (0x43)
                                ////.secCOVreg = 0x80                   // Enable over-voltage protection (CONFIG_COV bit 7)
                                ////               | ((4250 - 2000) / 50)      // Set over-voltage protection to 4250 mV	
                                ////               | 0x8000                // Timeout unit = ms (CONFIG_COVT bit 7)
                                ////               | ((3100 / 100) << 8),		// Timeout = 3100 ms

                                //UInt16 regCOV = br.ReadUInt16();

                                //secCellOVP.Value = (regCOV & 0xff) * 50 + 2000;
                                //secOVPdelay.Value = (regCOV >> 8) * 100;


                                //// LSB = CONFIG_CUV (0x44), MSB = CONFIG_CUVT (0x45)
                                ////.secCOVreg = 0x80                   // Enable under-voltage protection (CONFIG_CUV bit 7)
                                ////           | ((3100 - 700) / 100)      // Set under-voltage protection to 3100 mV
                                ////           | 0x8000                    // Timeout unit = ms (CONFIG_CUVT bit 7)
                                ////           | ((3100 / 100) << 8),      // Timeout = 3100 ms	

                                //UInt16 regCUV = br.ReadUInt16();

                                //secCellUVP.Value = (regCUV & 0xff) * 100 + 700;
                                //secUVPdelay.Value = (regCUV >> 8) * 100;


                                //// LSB = CONFIG_OT (0x46), MSB = CONFIG_OTT (0x47)
                                ////.secOTreg = ((65 - 35) / 5)             // Overtemp 1 = 65 degC
                                ////        | (((65 - 35) / 5) << 4)        // Overtemp 2 = 65 degC
                                ////        | ((2550 / 10) << 8),		      // Timeout = 2550 ms

                                //UInt16 regOT = br.ReadUInt16();

                                //secMaxTemp.Value = (regOT & 0x0f) * 5 + 35;
                                //secOvertempDelay.Value = (regOT >> 8) * 10;

                                // rly2Func.SelectedIndex = BitConverter.ToUInt16(modb, 131);

                                //tbParams.Clear();

                                //for (int i=0; i<64; i++)
                                //{
                                //tbParams.Text += (i+1).ToString() + ": " + (((UInt16)modb[(i*2)+3] << 8) | modb[(i*2)+4]).ToString("X4") + "\r\n";
                                //}

                                updateParamsGUI();

                                br.Close();
                                stream.Close();

                            });


                            //bData.temp1 = (((Int16)modb[17] << 8) | modb[18]) / 10.0;
                            //bData.temp2 = (((Int16)modb[19] << 8) | modb[20]) / 10.0;

                        }

                    }
                    catch (Exception ex)
                    {
                        this.Invoke((MethodInvoker)delegate
                        {
                            // Run on UI thread:
                            MessageBox.Show("Error: " + ex.Message, "Dump parameters", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        });
                    }
                    dumpCfgPend = false;

                    this.Invoke((MethodInvoker)delegate
                    {
                        // Run on UI thread:
                        btnCfgDump.Enabled = true;
                    });
                }

                if (writeCfgPend)
                {
                    try
                    {


                        byte[] modb = new byte[61*2];

                        using (BinaryWriter bw = new BinaryWriter(new MemoryStream(modb)))
                        {

                            this.Invoke((MethodInvoker)delegate
                            {


                                bw.Write((UInt16)conLockoutOVP.Value);
                                bw.Write((UInt16)conLockoutUVP.Value);
                                bw.Write((Int16)(conLockoutMaxTemp.Value * 100));
                                bw.Write((Int16)(conLockoutMinTemp.Value * 100));
                                bw.Write((Int16)conLockoutMinCurr.Value);
                                bw.Write((Int16)conLockoutMinCurrPeak.Value);
                                bw.Write((Int16)conLockoutMaxCurr.Value);
                                bw.Write((Int16)conLockoutMaxCurrPeak.Value);


                                bw.Write((UInt16)contRlyTime.Value);
                                bw.Write((UInt16)contPrechgTime.Value);
                                bw.Write((UInt16)contOnPulseTime.Value);

                                bw.Write((UInt16)turtleThresV.Value);

                                bw.Write((UInt16)chgStopV.Value);
                                bw.Write((UInt16)chgStartV.Value);
                                bw.Write((UInt16)chgInhibitV.Value);
                                bw.Write((Int16)(chgMinTemp.Value * 100));
                                bw.Write((Int16)(chgMaxTemp.Value * 100));

                                bw.Write((Int16)(regenMinTemp.Value * 100));
                                bw.Write((Int16)(regenMaxTemp.Value * 100));

                                bw.Write((UInt16)regenThresholdV.Value);


                                bw.Write((UInt16)heaterPumpOffTime.Value);
                                bw.Write((UInt16)heaterPumpOnTime.Value);
                                bw.Write((Int16)(heaterPumpOnTemp.Value * 100));
                                bw.Write((Int16)(heaterOffTemp.Value * 100));
                                bw.Write((Int16)(heaterOnTemp.Value * 100));

                                bw.Write((UInt16)cellBalMinV.Value);

                                bw.Write((UInt16)inpIgnOnThres.Value);
                                bw.Write((UInt16)inpIgnOffThres.Value);
                                bw.Write((UInt16)inpStartOnThres.Value);
                                bw.Write((UInt16)inpStartOffThres.Value);
                                bw.Write((UInt16)inpAcDetOnThres.Value);
                                bw.Write((UInt16)inpAcPresOffThres.Value);

                                bw.Write((UInt16)conLockoutPeakDelay.Value);
                                bw.Write((UInt16)conLockoutDelay.Value);
                                bw.Write((UInt16)modCnt.Value);

                                bw.Write((UInt16)(contPWMfreq.Value - 1));
                                bw.Write((UInt16)contPWMduty.Value);
                                bw.Write((UInt16)contIdleTime.Value);


                                bw.Write((UInt16)turtleOnDelay.Value);
                                bw.Write((UInt16)chgRestartDly.Value);
                                bw.Write((UInt16)chgAcDetDly.Value);


                                bw.Write((UInt16)regenResetDelay.Value);
                                bw.Write((UInt16)(heaterPumpFreq.Value - 1));
                                bw.Write((UInt16)heaterPumpDuty.Value);

                                bw.Write((UInt16)cellBalStartV.Value);
                                bw.Write((UInt16)cellBalEndV.Value);
                                bw.Write((UInt16)(((UInt16)cellBalTimeout.Value) | 0x80));
                                bw.Write((UInt16)cellBalIdleTime.Value);

                                bw.Write((UInt16)0xff3b); //alertFaultMask


                                bw.Write((UInt16)inpIgnInNum.Value);
                                bw.Write((UInt16)inpStartInNum.Value);
                                bw.Write((UInt16)inpAcDetInNum.Value);


                                if (out1Func.SelectedIndex < 0)
                                {
                                    out1Func.SelectedIndex = 0;
                                }
                                
                                bw.Write((UInt16)out1Func.SelectedIndex);


                                if (out2Func.SelectedIndex < 0)
                                {
                                    out2Func.SelectedIndex = 0;
                                }

                                bw.Write((UInt16)out2Func.SelectedIndex);

                                if (out3Func.SelectedIndex < 0)
                                {
                                    out3Func.SelectedIndex = 0;
                                }

                                bw.Write((UInt16)out3Func.SelectedIndex);

                                if (out4Func.SelectedIndex < 0)
                                {
                                    out4Func.SelectedIndex = 0;
                                }

                                bw.Write((UInt16)out4Func.SelectedIndex);

                                if (rly1Func.SelectedIndex < 0)
                                {
                                    rly1Func.SelectedIndex = 0;
                                }

                                bw.Write((UInt16)rly1Func.SelectedIndex);

                                if (rly2Func.SelectedIndex < 0)
                                {
                                    rly2Func.SelectedIndex = 0;
                                }

                                bw.Write((UInt16)rly2Func.SelectedIndex);

                                // LSB = CONFIG_COV (0x42), MSB = CONFIG_COVT (0x43)
                                //.secCOVreg = 0x80                   // Enable over-voltage protection (CONFIG_COV bit 7)
                                //               | ((4250 - 2000) / 50)      // Set over-voltage protection to 4250 mV	
                                //               | 0x8000                // Timeout unit = ms (CONFIG_COVT bit 7)
                                //               | ((3100 / 100) << 8),		// Timeout = 3100 ms

                                UInt16 regCOV = 0x8080;
                                regCOV |= (UInt16)((secCellOVP.Value - 2000) / 50);
                                regCOV |= (UInt16)((UInt16)(secOVPdelay.Value / 100) << 8);


                                 // LSB = CONFIG_CUV (0x44), MSB = CONFIG_CUVT (0x45)
	                                //.secCOVreg = 0x80                   // Enable under-voltage protection (CONFIG_CUV bit 7)
                                 //           | ((3100 - 700) / 100)      // Set under-voltage protection to 3100 mV
                                 //           | 0x8000                    // Timeout unit = ms (CONFIG_CUVT bit 7)
                                 //           | ((3100 / 100) << 8),      // Timeout = 3100 ms	

                                UInt16 regCUV = 0x8080;
                                regCUV |= (UInt16)((secCellUVP.Value - 700) / 100);
                                regCUV |= (UInt16)((UInt16)(secUVPdelay.Value / 100) << 8);


                                // LSB = CONFIG_OT (0x46), MSB = CONFIG_OTT (0x47)
                                //.secOTreg = ((65 - 35) / 5)             // Overtemp 1 = 65 degC
                                //        | (((65 - 35) / 5) << 4)        // Overtemp 2 = 65 degC
                                //        | ((2550 / 10) << 8),		      // Timeout = 2550 ms

                                UInt16 regOT = (UInt16)((secMaxTemp.Value - 35) / 5);
                                regOT |= (UInt16)((UInt16)((secMaxTemp.Value - 35) / 5) << 4);
                                regOT |= (UInt16)((UInt16)(secOvertempDelay.Value / 10) << 8);



                                bw.Write(regCOV); // secCOVreg
                                bw.Write(regCUV); // secCUVreg
                                bw.Write(regOT); // secOTreg



                            });

                            bw.Flush();

                            if (modbusASCII_writeHoldRegs(0, 0, (ushort)(modb.Length / 2), modb) == false)
                            {
                                MessageBox.Show("Parameter write failure" , "Write parameters", MessageBoxButtons.OK, MessageBoxIcon.Error);
                            }
                        }



                    }
                    catch (Exception ex)
                    {
                        this.Invoke((MethodInvoker)delegate
                        {
                            // Run on UI thread:
                            MessageBox.Show("Error: " + ex.Message, "Write parameters", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        });
                    }

                    writeCfgPend = false;

                    this.Invoke((MethodInvoker)delegate
                    {
                        // Run on UI thread:
                        btnCfgWrite.Enabled = true;
                    });
                }


                if (saveCfgPend)
                {

                    try
                    {
                        serialPort1.Write(":00050004FF00F8\r\n");
                        serialPort1.ReadTimeout = 1000;
                        serialPort1.ReadLine();
                    }
                    catch (Exception ex)
                    {
                        this.Invoke((MethodInvoker)delegate
                        {
                            // Run on UI thread:
                            MessageBox.Show("Error: " + ex.Message, "Save parameters", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        });
                    }

                    saveCfgPend = false;

                    this.Invoke((MethodInvoker)delegate
                    {
                        // Run on UI thread:
                        btnCfgSave.Enabled = true;
                    });

                }

                if (simModePend)
                {

                    try
                    {

                        byte[] modb = new byte[2];

                        using (BinaryWriter bw = new BinaryWriter(new MemoryStream(modb)))
                        {
                            this.Invoke((MethodInvoker)delegate
                            {
                                if (simModeActive)
                                    bw.Write(UInt16.Parse(simKey.Text, System.Globalization.NumberStyles.HexNumber));
                                else
                                    bw.Write((UInt16)0);
                            });

                            if (modbusASCII_writeHoldRegs(0, 1000, 1, modb))
                            {
                                simModePend = false;
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        simModePend = false;
                        this.Invoke((MethodInvoker)delegate
                        {
                            // Run on UI thread:
                            MessageBox.Show("Error: " + ex.Message, "Set sim mode", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        });
                    }


                }

                if (simReadPend)
                {
                    try
                    {

                        byte[] modb = modbusASCII_readHoldReg(0, 1001, 2);

                        // Reverse bytes (BE -> LE)
                        if (BitConverter.IsLittleEndian)
                        {
                            for (int i = 0; i < 2; i++)
                            {
                                Array.Reverse(modb, 3 + (i * 2), 2);
                            }


                        }

                        int modCnt = 0;

                        using (BinaryReader br = new BinaryReader(new MemoryStream(modb)))
                        {
                            br.ReadByte();  // Slave address
                            br.ReadByte();  // Function code
                            br.ReadByte();  // Byte count

                            this.Invoke((MethodInvoker)delegate
                            {
                                modCnt = br.ReadUInt16();
                                simModCnt.Value = modCnt;
                                simBattCurr.Value = br.ReadInt16();
                            });
                        }


                        modb = modbusASCII_readHoldReg(0, 1003, (ushort)(13 * modCnt));

                        // Reverse bytes (BE -> LE)
                        if (BitConverter.IsLittleEndian)
                        {
                            for (int i = 0; i < (ushort)(13 * modCnt); i++)
                            {
                                Array.Reverse(modb, 3 + (i * 2), 2);
                            }


                        }

                        using (BinaryReader br = new BinaryReader(new MemoryStream(modb)))
                        {
                            br.ReadByte();  // Slave address
                            br.ReadByte();  // Function code
                            br.ReadByte();  // Byte count


                            this.Invoke((MethodInvoker)delegate
                            {
                                for (int i = 0; i < modCnt; i++)
                                {
                                    simDataList[i].u_cell1 = br.ReadUInt16();
                                    simDataList[i].u_cell2 = br.ReadUInt16();
                                    simDataList[i].u_cell3 = br.ReadUInt16();
                                    simDataList[i].u_cell4 = br.ReadUInt16();
                                    simDataList[i].u_cell5 = br.ReadUInt16();
                                    simDataList[i].u_cell6 = br.ReadUInt16();

                                    br.ReadUInt16(); // Voltage sum
                                br.ReadUInt16(); // Module voltage


                                simDataList[i].temp_1 = br.ReadInt16();
                                    simDataList[i].temp_2 = br.ReadInt16();


                                    simDataList[i].status = br.ReadByte();
                                    simDataList[i].alerts = br.ReadByte();
                                    simDataList[i].faults = br.ReadByte();
                                    simDataList[i].cov = br.ReadByte();
                                    simDataList[i].cuv = br.ReadByte();
                                    simDataList[i].bal = br.ReadByte();
                                }

                                updateSim();

                            });
                        }
                    }
                    catch (Exception ex)
                    {
                        this.Invoke((MethodInvoker)delegate
                        {
                            // Run on UI thread:
                            MessageBox.Show("Error: " + ex.Message, "Read sim parameters", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        });
                    }


                    simReadPend = false;
                }



                bool wrEn = false;

                this.Invoke((MethodInvoker)delegate
                {
                    wrEn = simWrEn.Checked;
                });

                if (simModeActive && wrEn)
                {


                    // update status bits WrEn
                    bool wrEnBits = false;

                    this.Invoke((MethodInvoker)delegate
                    {
                        // Run on UI thread:
                        wrEnBits = simBitsWrEn.Checked;
                    });


                    try
                    {

                        if (wrEnBits)
                        {
                            serialPort1.Write(":00050006FF00F6\r\n");
                        }
                        else
                        {
                            serialPort1.Write(":000500060000F5\r\n");
                        }

                        serialPort1.ReadTimeout = 1000;

                        serialPort1.ReadLine();

                        int modCnt = 0;


                        this.Invoke((MethodInvoker)delegate
                        {
                            modCnt = (int)simModCnt.Value;
                        });

                        byte[] modb = new byte[4 + (26 * modCnt)];

                        using (BinaryWriter bw = new BinaryWriter(new MemoryStream(modb)))
                        {

                            this.Invoke((MethodInvoker)delegate
                            {

                                bw.Write((ushort)simModCnt.Value);      // MODBUS_HOLDING_SIM_MODULE_COUNT (1001)
                                bw.Write((short)simBattCurr.Value);    // MODBUS_HOLDING_SIM_BATT_CURRENT (1002)


                                for (int i = 0; i < simModCnt.Value; i++)
                                {
                                    bw.Write(simDataList[i].u_cell1);    // Cell 1 voltage
                                    bw.Write(simDataList[i].u_cell2);    // Cell 2 voltage
                                    bw.Write(simDataList[i].u_cell3);    // Cell 3 voltage
                                    bw.Write(simDataList[i].u_cell4);    // Cell 4 voltage
                                    bw.Write(simDataList[i].u_cell5);    // Cell 5 voltage
                                    bw.Write(simDataList[i].u_cell6);    // Cell 6 voltage

                                    UInt16 modVolt = simDataList[i].u_cell1;
                                    modVolt += simDataList[i].u_cell2;
                                    modVolt += simDataList[i].u_cell3;
                                    modVolt += simDataList[i].u_cell4;
                                    modVolt += simDataList[i].u_cell5;
                                    modVolt += simDataList[i].u_cell6;

                                    bw.Write(modVolt);    // Voltage sum
                                    bw.Write(modVolt);    // Module voltage

                                    bw.Write(simDataList[i].temp_1);    // Temp 1
                                    bw.Write(simDataList[i].temp_2);    // Temp 2


                                    bw.Write(simDataList[i].status);
                                    bw.Write(simDataList[i].alerts);
                                    bw.Write(simDataList[i].faults);
                                    bw.Write(simDataList[i].cov);
                                    bw.Write(simDataList[i].cuv);
                                    bw.Write(simDataList[i].bal);


                                }

                            });

                            bw.Flush();

                            if (modbusASCII_writeHoldRegs(0, 1001, (ushort)(modb.Length / 2), modb))
                            {
                                // simWritePend = false;
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        this.Invoke((MethodInvoker)delegate
                        {
                            // Run on UI thread:
                            simWrEn.Checked = false;
                            MessageBox.Show("Error: " + ex.Message, "Write sim parameters", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        });
                    }

                }





                try
                {
                    GetBatteryData();
                    if (firstFetch)
                    {
                        firstFetch = false;
                        this.Invoke((MethodInvoker)delegate
                        {
                            // Run on UI thread:
                            dataGridView1.AutoResizeColumns(DataGridViewAutoSizeColumnsMode.AllCells);
                        });
                    
                    }

                    // get debug data



                        if (serialPort1.IsOpen)
                        {
                            byte[] modb = modbusASCII_readReg(0, 10000, 19);

                            this.Invoke((MethodInvoker)delegate
                            {

                                // Reverse bytes (BE -> LE)
                                if (BitConverter.IsLittleEndian)
                                {
                                    for (int i = 0; i < 19; i++)
                                    {
                                        Array.Reverse(modb, 3 + (i * 2), 2);
                                    }


                                }
                                MemoryStream stream = new MemoryStream(modb);
                                BinaryReader br = new BinaryReader(stream);

                                br.ReadByte();  // Slave address
                                br.ReadByte();  // Function code
                                br.ReadByte();  // Byte count

                                dbgData.BeginUpdate();
                                lvLockoutFlags.BeginUpdate();

                                foreach (ListViewItem itm in dbgData.Items)
                                {
                                    UInt16 val = br.ReadUInt16();
                                    string newText = "0x" + val.ToString("X4");

                                    if (newText != itm.SubItems[1].Text)
                                        itm.SubItems[1].Text = newText;

                                    if (itm.Index == 0)
                                    {
                                        sbyte bVal = (sbyte)(val & 0xff);

                                        newText = bVal.ToString();

                                        if (newText != itm.SubItems[2].Text)
                                            itm.SubItems[2].Text = newText;

                                    }
                                    else if (itm.Index == 3)
                                    {
                                        short sVal = (short)val;

                                        newText = (sVal/10.0).ToString("N1");

                                        if (newText != itm.SubItems[2].Text)
                                            itm.SubItems[2].Text = newText;
                                    }
                                    else if (itm.Index == 3)
                                    {
                                        short sVal = (short)val;

                                        newText = sVal.ToString();

                                        if (newText != itm.SubItems[2].Text)
                                            itm.SubItems[2].Text = newText;
                                    }
                                    else if (itm.Index == 16)
                                    {
                                        byte bVal = (byte)(val & 0xff);

                                        newText = bVal.ToString();

                                        if (newText != itm.SubItems[2].Text)
                                            itm.SubItems[2].Text = newText;
                                    }
                                    else if (itm.Index == 17)
                                    {
                                        byte bVal = (byte)(val & 0xff);

                                        newText = bVal.ToString();

                                        if (newText != itm.SubItems[2].Text)
                                            itm.SubItems[2].Text = newText;
                                    }
                                    else
                                    {
                                        newText = val.ToString();

                                        if (newText != itm.SubItems[2].Text)
                                            itm.SubItems[2].Text = newText;
                                    }

                                    if (itm.Index == 18)
                                    {

                                        foreach (ListViewItem lockoutFlag in lvLockoutFlags.Items)
                                        {
                                            if ((val >> lockoutFlag.Index & 0x01) == 0x01)
                                            {
                                                // Flag active
                                                if (lockoutFlag.SubItems[1].Text != "Yes")
                                                {
                                                    lockoutFlag.SubItems[1].Text = "Yes";
                                                    lockoutFlag.ForeColor = Color.Red;
                                                }
                                                
                                            }
                                            else
                                            {
                                                // Flag inactive
                                                if (lockoutFlag.SubItems[1].Text != "No")
                                                {
                                                    lockoutFlag.SubItems[1].Text = "No";
                                                    lockoutFlag.ForeColor = Color.Green;
                                                }
                                            }
                                        }

                                    }

                                }

                                dbgData.EndUpdate();
                                lvLockoutFlags.EndUpdate();

                                br.Close();
                                stream.Close();

                            });



                        modb = modbusASCII_readReg(0, 11000, 34);

                        this.Invoke((MethodInvoker)delegate
                        {

                            // Reverse bytes (BE -> LE)
                            if (BitConverter.IsLittleEndian)
                            {
                                for (int i = 0; i < 17; i++)
                                {
                                    Array.Reverse(modb, 3 + (i * 4), 4);
                                }


                            }
                            MemoryStream stream = new MemoryStream(modb);
                            BinaryReader br = new BinaryReader(stream);

                            br.ReadByte();  // Slave address
                            br.ReadByte();  // Function code
                            br.ReadByte();  // Byte count

                            dbgData2.BeginUpdate();

                            foreach (ListViewItem itm in dbgData2.Items)
                            {
                                UInt32 val = br.ReadUInt32();
                                string newText = val.ToString();

                                if (newText != itm.SubItems[1].Text)
                                    itm.SubItems[1].Text = newText;

                            }

                            dbgData2.EndUpdate();


                            br.Close();
                            stream.Close();

                        });

                    }



























                }
                catch (Exception ex)
                {

                }






                Thread.Sleep(10);

            }
        }

        private void startToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (!backgroundWorker1.IsBusy)
                backgroundWorker1.RunWorkerAsync();
        }

        private void btnCfgDump_Click(object sender, EventArgs e)
        {
            btnCfgDump.Enabled = false;
            dumpCfgPend = true;
        }

        private void groupBox1_Enter(object sender, EventArgs e)
        {

        }

        private void cbEnableSimMode_CheckedChanged(object sender, EventArgs e)
        {
            if (cbEnableSimMode.Checked)
            {
                DialogResult dr = MessageBox.Show("CAUTION: Enabling simulated data mode will disable the normal BMS protection systems. THIS IS VERY DANGEROUS.\r\n\r\nDo you want to proceed?", "Caution", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);

                if (dr == DialogResult.Yes)
                {
                    // enable sim mode
                    simModeActive = true;
                    simModePend = true;
                }
                else
                {
                    simModeActive = false;
                    cbEnableSimMode.Checked = false;
                }
            }
            else
            {
                // disable sim mode

                if (simModeActive)
                {
                    simModeActive = false;
                    simModePend = true;
                    MessageBox.Show("Simulated data mode disabled. BMS power cycle recommeded!", "Caution", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }

                
            }
        }

        private void simModCnt_ValueChanged(object sender, EventArgs e)
        {
            if (simModCnt.Value > 0)
            {
                simSelMod.Minimum = 1;
                simSelMod.Maximum = simModCnt.Value;
                groupBox11.Enabled = true;
            }
            else
            {
                groupBox11.Enabled = false;
                simSelMod.Minimum = 0;
                simSelMod.Maximum = 0;
            }
        }


        void updateParamsFromGUI()
        {
            paramData.cellVoltHi = (uint)conLockoutOVP.Value;
            paramData.cellVoltLo = (uint)conLockoutUVP.Value;


            paramData.tempHi = (int)(conLockoutMaxTemp.Value * 100.0m);
            paramData.tempLo = (int)(conLockoutMinTemp.Value * 100.0m);
            paramData.minCurrent = (int)conLockoutMinCurr.Value;
            paramData.minCurrentPeak = (int)conLockoutMinCurrPeak.Value;
            paramData.maxCurrent = (int)conLockoutMaxCurr.Value;
            paramData.maxCurrentPeak = (int)conLockoutMaxCurrPeak.Value;

            paramData.contactorRelayTime = (uint)contRlyTime.Value;
            paramData.contactorPrechargeTime = (uint)contPrechgTime.Value;
            paramData.contactorOnPulseTime = (uint)contOnPulseTime.Value;

            paramData.turtlemodeVolt = (uint)turtleThresV.Value;

            paramData.chgDisVolt = (uint)chgStopV.Value;
            paramData.chgEnVolt = (uint)chgStartV.Value;
            paramData.chgInhibitVolt = (uint)chgInhibitV.Value;
            paramData.chgMinTemp = (int)(chgMinTemp.Value * 100.0m);
            paramData.chgMaxTemp = (int)(chgMaxTemp.Value * 100.0m);


            paramData.regenMinTemp = (int)(regenMinTemp.Value * 100.0m);
            paramData.regenMaxTemp = (int)(regenMaxTemp.Value * 100.0m);
            paramData.regenMaxVolt = (uint)regenThresholdV.Value;


            paramData.pumpOffSeconds = (uint)heaterPumpOffTime.Value;
            paramData.pumpOnSeconds = (uint)heaterPumpOnTime.Value;
            paramData.pumpOnTemp = (int)(heaterPumpOnTemp.Value * 100.0m);
            paramData.heaterOffTemp = (int)(heaterOffTemp.Value * 100.0m);
            paramData.heaterOnTemp = (int)(heaterOnTemp.Value * 100.0m);

            paramData.balMinVoltage = (uint)cellBalMinV.Value;

            paramData.ignThresholdOn = (uint)inpIgnOnThres.Value;
            paramData.ignThresholdOff = (uint)inpIgnOffThres.Value;
            paramData.startThresholdOn = (uint)inpStartOnThres.Value;
            paramData.startThresholdOff = (uint)inpStartOffThres.Value;
            paramData.acPresThresholdOn = (uint)inpAcDetOnThres.Value;
            paramData.acPresThresholdOff = (uint)inpAcPresOffThres.Value;

            paramData.peakCurrentDelay = (uint)conLockoutPeakDelay.Value;
            paramData.contactorOffDelay = (uint)conLockoutDelay.Value;
            paramData.moduleCount = (uint)modCnt.Value;

            paramData.contactorPwmFrequency = (uint)contPWMfreq.Value;
            paramData.contactorPwmDuty = (uint)contPWMduty.Value;
            paramData.contactorIdleDelay = (uint)contIdleTime.Value;


            paramData.turtlemodeDelay = (uint)turtleOnDelay.Value;

            paramData.chgDelay = (uint)chgRestartDly.Value;
            paramData.acPresDelay = (uint)chgAcDetDly.Value;

            paramData.regenResetDelay = (uint)regenResetDelay.Value;

            paramData.pumpFreqPWM = (uint)heaterPumpFreq.Value;
            paramData.pumpDutyPWM = (uint)heaterPumpDuty.Value;

            paramData.balMaxDelta = (uint)cellBalStartV.Value;
            paramData.balFinishDelta = (uint)cellBalEndV.Value;
            paramData.balInterval = (uint)cellBalTimeout.Value;
            paramData.balIdleTime = (uint)cellBalIdleTime.Value;

            paramData.ignGPI = (uint)inpIgnInNum.Value;
            paramData.startGPI = (uint)inpStartInNum.Value;
            paramData.acPresGPI = (uint)inpAcDetInNum.Value;

            paramData.out1Func = out1Func.SelectedIndex;
            paramData.out2Func = out2Func.SelectedIndex;
            paramData.out3Func = out3Func.SelectedIndex;
            paramData.out4Func = out4Func.SelectedIndex;
            paramData.rly1Func = rly1Func.SelectedIndex;
            paramData.rly2Func = rly2Func.SelectedIndex;

            paramData.secCellOVP = (uint)secCellOVP.Value;
            paramData.secCellUVP = (uint)secCellUVP.Value;
            paramData.secCellOVPdelay = (uint)secOVPdelay.Value;
            paramData.secCellUVPdelay = (uint)secUVPdelay.Value;
            paramData.secMaxTemp = (uint)secMaxTemp.Value;
            paramData.secMaxTempDelay = (uint)secOvertempDelay.Value;

        }

        void updateParamsGUI()
        {

            conLockoutOVP.Value = paramData.cellVoltHi;
            conLockoutUVP.Value = paramData.cellVoltLo;


            conLockoutMaxTemp.Value = paramData.tempHi / 100.0m;
            conLockoutMinTemp.Value = paramData.tempLo / 100.0m;
            conLockoutMinCurr.Value = paramData.minCurrent;
            conLockoutMinCurrPeak.Value = paramData.minCurrentPeak;
            conLockoutMaxCurr.Value = paramData.maxCurrent;
            conLockoutMaxCurrPeak.Value = paramData.maxCurrentPeak;

            contRlyTime.Value = paramData.contactorRelayTime;
            contPrechgTime.Value = paramData.contactorPrechargeTime;
            contOnPulseTime.Value = paramData.contactorOnPulseTime;

            turtleThresV.Value = paramData.turtlemodeVolt;

            chgStopV.Value = paramData.chgDisVolt;
            chgStartV.Value = paramData.chgEnVolt;
            chgInhibitV.Value = paramData.chgInhibitVolt;
            chgMinTemp.Value = paramData.chgMinTemp / 100.0m;
            chgMaxTemp.Value = paramData.chgMaxTemp / 100.0m;


            regenMinTemp.Value = paramData.regenMinTemp / 100.0m;
            regenMaxTemp.Value = paramData.regenMaxTemp / 100.0m;
            regenThresholdV.Value = paramData.regenMaxVolt;


            heaterPumpOffTime.Value = paramData.pumpOffSeconds;
            heaterPumpOnTime.Value = paramData.pumpOnSeconds;
            heaterPumpOnTemp.Value = paramData.pumpOnTemp / 100.0m;
            heaterOffTemp.Value = paramData.heaterOffTemp / 100.0m;
            heaterOnTemp.Value = paramData.heaterOnTemp / 100.0m;

            cellBalMinV.Value = paramData.balMinVoltage;

            inpIgnOnThres.Value = paramData.ignThresholdOn;
            inpIgnOffThres.Value = paramData.ignThresholdOff;
            inpStartOnThres.Value = paramData.startThresholdOn;
            inpStartOffThres.Value = paramData.startThresholdOff;
            inpAcDetOnThres.Value = paramData.acPresThresholdOn;
            inpAcPresOffThres.Value = paramData.acPresThresholdOff;

            conLockoutPeakDelay.Value = paramData.peakCurrentDelay;
            conLockoutDelay.Value = paramData.contactorOffDelay;
            modCnt.Value = paramData.moduleCount;

            contPWMfreq.Value = paramData.contactorPwmFrequency;
            contPWMduty.Value = paramData.contactorPwmDuty;
            contIdleTime.Value = paramData.contactorIdleDelay;


            turtleOnDelay.Value = paramData.turtlemodeDelay;

            chgRestartDly.Value = paramData.chgDelay;
            chgAcDetDly.Value = paramData.acPresDelay;

            regenResetDelay.Value = paramData.regenResetDelay;

            heaterPumpFreq.Value = paramData.pumpFreqPWM;
            heaterPumpDuty.Value = paramData.pumpDutyPWM;

            cellBalStartV.Value = paramData.balMaxDelta;
            cellBalEndV.Value = paramData.balFinishDelta;
            cellBalTimeout.Value = paramData.balInterval;
            cellBalIdleTime.Value = paramData.balIdleTime;

            inpIgnInNum.Value = paramData.ignGPI;
            inpStartInNum.Value = paramData.startGPI;
            inpAcDetInNum.Value = paramData.acPresGPI;

            out1Func.SelectedIndex = paramData.out1Func;
            out2Func.SelectedIndex = paramData.out2Func;
            out3Func.SelectedIndex = paramData.out3Func;
            out4Func.SelectedIndex = paramData.out4Func;
            rly1Func.SelectedIndex = paramData.rly1Func;
            rly2Func.SelectedIndex = paramData.rly2Func;

            secCellOVP.Value = paramData.secCellOVP;
            secCellUVP.Value = paramData.secCellUVP;
            secOVPdelay.Value = paramData.secCellOVPdelay;
            secUVPdelay.Value = paramData.secCellUVPdelay;
            secMaxTemp.Value = paramData.secMaxTemp;
            secOvertempDelay.Value = paramData.secMaxTempDelay;
        }

        void updateSim()
        {
            if (simSelMod.Value > 0)
            {
                simCell1V.Value = simDataList[(int)simSelMod.Value - 1].u_cell1;
                simCell2V.Value = simDataList[(int)simSelMod.Value - 1].u_cell2;
                simCell3V.Value = simDataList[(int)simSelMod.Value - 1].u_cell3;
                simCell4V.Value = simDataList[(int)simSelMod.Value - 1].u_cell4;
                simCell5V.Value = simDataList[(int)simSelMod.Value - 1].u_cell5;
                simCell6V.Value = simDataList[(int)simSelMod.Value - 1].u_cell6;

                simTemp1.Value = simDataList[(int)simSelMod.Value - 1].temp_1 / 100.0m;
                simTemp2.Value = simDataList[(int)simSelMod.Value - 1].temp_2 / 100.0m;


                for (int i=0; i<8; i++)
                {
                    if ((simDataList[(int)simSelMod.Value - 1].status & (1 << i)) == (1 << i))
                    {
                        simStatusBits.SetItemChecked(i, true);
                    }
                    else
                    {
                        simStatusBits.SetItemChecked(i, false);
                    }
                }

                for (int i = 0; i < 8; i++)
                {
                    if ((simDataList[(int)simSelMod.Value - 1].alerts & (1 << i)) == (1 << i))
                    {
                        simAlertBits.SetItemChecked(i, true);
                    }
                    else
                    {
                        simAlertBits.SetItemChecked(i, false);
                    }
                }

                for (int i = 0; i < 8; i++)
                {
                    if ((simDataList[(int)simSelMod.Value - 1].faults & (1 << i)) == (1 << i))
                    {
                        simFaultBits.SetItemChecked(i, true);
                    }
                    else
                    {
                        simFaultBits.SetItemChecked(i, false);
                    }
                }

                for (int i = 0; i < 8; i++)
                {
                    if ((simDataList[(int)simSelMod.Value - 1].bal & (1 << i)) == (1 << i))
                    {
                        simBalBits.SetItemChecked(i, true);
                    }
                    else
                    {
                        simBalBits.SetItemChecked(i, false);
                    }
                }

                for (int i = 0; i < 8; i++)
                {
                    if ((simDataList[(int)simSelMod.Value - 1].cov & (1 << i)) == (1 << i))
                    {
                        simCOVBits.SetItemChecked(i, true);
                    }
                    else
                    {
                        simCOVBits.SetItemChecked(i, false);
                    }
                }

                for (int i = 0; i < 8; i++)
                {
                    if ((simDataList[(int)simSelMod.Value - 1].cuv & (1 << i)) == (1 << i))
                    {
                        simCUVBits.SetItemChecked(i, true);
                    }
                    else
                    {
                        simCUVBits.SetItemChecked(i, false);
                    }
                }

            }
        }
        private void simSelMod_ValueChanged(object sender, EventArgs e)
        {
            updateSim();
                
        }

        private void simStatusBits_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            if (simSelMod.Value > 0)
            {

                byte newByte = simDataList[(int)simSelMod.Value - 1].status;
                newByte &= (byte)~(1 << e.Index);

                if (e.NewValue == CheckState.Checked)
                {
                    newByte |= (byte)(1 << e.Index);
                }

                simDataList[(int)simSelMod.Value - 1].status = newByte;
            }
        }

        private void simAlertBits_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            if (simSelMod.Value > 0)
            {

                byte newByte = simDataList[(int)simSelMod.Value - 1].alerts;
                newByte &= (byte)~(1 << e.Index);

                if (e.NewValue == CheckState.Checked)
                {
                    newByte |= (byte)(1 << e.Index);
                }

                simDataList[(int)simSelMod.Value - 1].alerts = newByte;
            }
        }

        private void simFaultBits_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            if (simSelMod.Value > 0)
            {

                byte newByte = simDataList[(int)simSelMod.Value - 1].faults;
                newByte &= (byte)~(1 << e.Index);

                if (e.NewValue == CheckState.Checked)
                {
                    newByte |= (byte)(1 << e.Index);
                }

                simDataList[(int)simSelMod.Value - 1].faults = newByte;
            }
        }

        private void simBalBits_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            if (simSelMod.Value > 0)
            {

                byte newByte = simDataList[(int)simSelMod.Value - 1].bal;
                newByte &= (byte)~(1 << e.Index);

                if (e.NewValue == CheckState.Checked)
                {
                    newByte |= (byte)(1 << e.Index);
                }

                simDataList[(int)simSelMod.Value - 1].bal = newByte;
            }
        }

        private void simCOVBits_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            if (simSelMod.Value > 0)
            {

                byte newByte = simDataList[(int)simSelMod.Value - 1].cov;
                newByte &= (byte)~(1 << e.Index);

                if (e.NewValue == CheckState.Checked)
                {
                    newByte |= (byte)(1 << e.Index);
                }

                simDataList[(int)simSelMod.Value - 1].cov = newByte;
            }
        }

        private void simCUVBits_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            if (simSelMod.Value > 0)
            {

                byte newByte = simDataList[(int)simSelMod.Value - 1].cuv;
                newByte &= (byte)~(1 << e.Index);

                if (e.NewValue == CheckState.Checked)
                {
                    newByte |= (byte)(1 << e.Index);
                }

                simDataList[(int)simSelMod.Value - 1].cuv = newByte;
            }
        }

        private void simCell1V_ValueChanged(object sender, EventArgs e)
        {
            if (simSelMod.Value > 0)
                simDataList[(int)simSelMod.Value - 1].u_cell1 = (ushort)simCell1V.Value;
        }

        private void simCell2V_ValueChanged(object sender, EventArgs e)
        {
            if (simSelMod.Value > 0)
                simDataList[(int)simSelMod.Value - 1].u_cell2 = (ushort)simCell2V.Value;
        }

        private void simCell3V_ValueChanged(object sender, EventArgs e)
        {
            if (simSelMod.Value > 0)
                simDataList[(int)simSelMod.Value - 1].u_cell3 = (ushort)simCell3V.Value;
        }

        private void simCell4V_ValueChanged(object sender, EventArgs e)
        {
            if (simSelMod.Value > 0)
                simDataList[(int)simSelMod.Value - 1].u_cell4 = (ushort)simCell4V.Value;
        }

        private void simCell5V_ValueChanged(object sender, EventArgs e)
        {
            if (simSelMod.Value > 0)
                simDataList[(int)simSelMod.Value - 1].u_cell5 = (ushort)simCell5V.Value;
        }

        private void simCell6V_ValueChanged(object sender, EventArgs e)
        {
            if (simSelMod.Value > 0)
                simDataList[(int)simSelMod.Value - 1].u_cell6 = (ushort)simCell6V.Value;
        }

        private void simTemp1_ValueChanged(object sender, EventArgs e)
        {
            if (simSelMod.Value > 0)
                simDataList[(int)simSelMod.Value - 1].temp_1 = (short)(simTemp1.Value*100);
        }

        private void simTemp2_ValueChanged(object sender, EventArgs e)
        {
            if (simSelMod.Value > 0)
                simDataList[(int)simSelMod.Value - 1].temp_2 = (short)(simTemp2.Value*100);
        }

        private void btnSimRead_Click(object sender, EventArgs e)
        {
            simReadPend = true;
        }

        private void btnCfgWrite_Click(object sender, EventArgs e)
        {
            updateParamsFromGUI();
            btnCfgWrite.Enabled = false;
            writeCfgPend = true;
        }

        private void btnCfgSave_Click(object sender, EventArgs e)
        {
            btnCfgSave.Enabled = false;
            saveCfgPend = true;
        }

        private void exportConfigToolStripMenuItem_Click(object sender, EventArgs e)
        {

            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Filter = "XML files|*.xml";
            DialogResult dr = sfd.ShowDialog();

            if (dr == DialogResult.OK)
            {
                try
                {
                    System.Xml.Serialization.XmlSerializer xmsSer = new System.Xml.Serialization.XmlSerializer(paramData.GetType());

                    using (StreamWriter writer = new StreamWriter(sfd.FileName))
                    {
                        xmsSer.Serialize(writer, paramData);
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error: " + ex.Message, "Export parameters", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }


        }

        private void importConfigToolStripMenuItem_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "XML files|*.xml";
            DialogResult dr = ofd.ShowDialog();

            if (dr == DialogResult.OK)
            {
                try
                {

                    XmlSerializer ser = new XmlSerializer(typeof(paramDataClass));
                    using (StreamReader reader = new StreamReader(ofd.FileName))
                    {
                        paramData = (paramDataClass)ser.Deserialize(reader);
                    }

                    updateParamsGUI();
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error: " + ex.Message, "Export parameters", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private void btnEnBoot_Click(object sender, EventArgs e)
        {
            btnEnBoot.Enabled = false;
            enableBootPend = true;
        }

        private void btnExitBoot_Click(object sender, EventArgs e)
        {
            btnExitBoot.Enabled = false;
            exitBootPend = true;
        }
    }
}
