using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO.Ports;
using System.Threading;

namespace tincanbms_cfg
{
    class updi
    {
        const byte UPDI_BREAK = 0x00;
        const byte UPDI_PHY_SYNC = 0x55;
        const byte UPDI_KEY = 0xE0;
        const byte UPDI_KEY_SIB = 0x04;
        const byte UPDI_SIB_32BYTES = 0x02;
        const byte UPDI_STCS = 0xC0;
        const byte UPDI_LDCS = 0x80;


        //# CS and ASI Register Address map
        const byte UPDI_CS_STATUSA = 0x00;
        const byte UPDI_CS_STATUSB = 0x01;
        const byte UPDI_CS_CTRLA = 0x02;
        const byte UPDI_CS_CTRLB = 0x03;
        const byte UPDI_ASI_KEY_STATUS = 0x07;
        const byte UPDI_ASI_RESET_REQ = 0x08;
        const byte UPDI_ASI_CTRLA = 0x09;
        const byte UPDI_ASI_SYS_CTRLA = 0x0A;
        const byte UPDI_ASI_SYS_STATUS = 0x0B;
        const byte UPDI_ASI_CRC_STATUS = 0x0C;

        const byte UPDI_CTRLA_IBDLY_BIT = 7;
        const byte UPDI_CTRLB_CCDETDIS_BIT = 3;
        const byte UPDI_CTRLB_UPDIDIS_BIT = 2;

        const byte UPDI_RESET_REQ_VALUE = 0x59;


        SerialPort sp = new SerialPort();

        public updi(string ComPort, bool rts)
        {
            sp.PortName = ComPort;
            sp.BaudRate = 115200;
            sp.StopBits = StopBits.Two;
            sp.Parity = Parity.Even;
            sp.RtsEnable = rts;
            sp.ReadTimeout = 500;
            
        }

        public void open()
        {
            if (!sp.IsOpen)
                sp.Open();
        }
        public void close()
        {
            if (sp.IsOpen)
                sp.Close();
        }
        public void updi_physical_send_double_break()
        {
            byte[] buffer = new byte[1];
            sp.BaudRate = 300;
            sp.StopBits = StopBits.One;

            buffer[0] = UPDI_BREAK;

            sp.DiscardInBuffer();
            sp.Write(buffer, 0, 1);
            sp.ReadByte();

            Thread.Sleep(100);

            sp.Write(buffer, 0, 1);
            sp.ReadByte();

            sp.BaudRate = 115200;
            sp.StopBits = StopBits.Two;

            sp.DiscardInBuffer();
        }


        public byte[] updi_physical_sib(byte size)
        {
            byte[] buffer = new byte[2];
            byte[] recvBuf = new byte[size];

            buffer[0] = UPDI_PHY_SYNC;
            buffer[1] = UPDI_KEY | UPDI_KEY_SIB | UPDI_SIB_32BYTES;

            sp.DiscardInBuffer();
            sp.Write(buffer, 0, 2);
            sp.Read(buffer, 0, 2);
            sp.Read(recvBuf, 0, size);

            return recvBuf;

        }

        public void updi_link_stcs(byte address, byte value)
        {
            byte[] buffer = new byte[3];

            buffer[0] = UPDI_PHY_SYNC;
            buffer[1] = (byte)(UPDI_STCS | (address & 0x0F));
            buffer[2] = value;

            sp.DiscardInBuffer();
            sp.Write(buffer, 0, 3);
            sp.Read(buffer, 0, 3);

        }



        public int updi_link_ldcs(byte address)
        {
            byte[] buffer = new byte[2];
            byte[] recvBuf = new byte[1];

            buffer[0] = UPDI_PHY_SYNC;
            buffer[1] = (byte)(UPDI_LDCS | (address & 0x0F));

            sp.DiscardInBuffer();
            sp.Write(buffer, 0, 2);
            sp.Read(buffer, 0, 2);
            return sp.ReadByte();
        }


        public void updi_link_init_session_parameters()
        {
            // Set the inter-byte delay bit and disable collision detection
            updi_link_stcs(UPDI_CS_CTRLB, 1 << UPDI_CTRLB_CCDETDIS_BIT);
            updi_link_stcs(UPDI_CS_CTRLA, 1 << UPDI_CTRLA_IBDLY_BIT);
        }

        public int updi_link_check()
        {
            int value;
            value = updi_link_ldcs(UPDI_CS_STATUSA);
            if (value > 0)
            {
                // UDPI init OK
                return 0;
            }
            else
            {
                // UDPI not OK - reinitialisation required
                return -1;
            }
        }

        public int updi_link_init()
        {
            updi_physical_send_double_break();
            updi_link_init_session_parameters();

            return updi_link_check();

        }

        public void updi_cpu_reset()
        {
            updi_link_stcs(UPDI_ASI_RESET_REQ, UPDI_RESET_REQ_VALUE);
            updi_link_stcs(UPDI_ASI_RESET_REQ, 0x00);

        }

    }
}
