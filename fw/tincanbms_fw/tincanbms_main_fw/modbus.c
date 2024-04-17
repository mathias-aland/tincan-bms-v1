/*
 * modbus.c
 *
 * Created: 10-09-2023 22:50:40
 *  Author: Mathias
 */ 

#include <avr/io.h>
#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <limits.h>
#include <ctype.h>
#include <stdbool.h>
#include <util/atomic.h>

#include "modbus.h"
#include "bmsmodule.h"
#include "globals.h"
#include "i2c.h"
#include "systick.h"

enum {
	MODBUS_STATE_WAITSTART = 0,			// waiting for start character (':')
	MODBUS_STATE_WAITDATA1,				// waiting for data (first hex digit in byte) or end of packet (CR)
	MODBUS_STATE_WAITDATA2,				// waiting for data (last hex digit in byte)
	MODBUS_STATE_WAITEND,				// waiting for end of packet (LF)
	MODBUS_STATE_WAITPROC,				// waiting for processing
	MODBUS_STATE_SENDSTART,				// send start character (':')
	MODBUS_STATE_SENDDATA1,				// send data byte (first hex digit in byte)
	MODBUS_STATE_SENDDATA2,				// send data byte (last hex digit in byte)
	MODBUS_STATE_SENDLRC1,				// send LRC (first hex digit in byte)
	MODBUS_STATE_SENDLRC2,				// send LRC (last hex digit in byte)
	MODBUS_STATE_SENDCR,				// send CR
	MODBUS_STATE_SENDLF					// send LF
};

volatile uint8_t modbusState;


uint8_t modbus_addr = 0;	// slave address

uint8_t modbus_recvSum;
uint8_t modbus_buf[255];	// Max PDU is 253 bytes + 1 byte LRC + 1 byte station address = 255 bytes buffer size
uint8_t modbus_buflen = 0;

extern COMP_MCU_state_t comp_mcu_state;

//volatile bool modbus_pend = false;

// the following function assumes the character is a valid HEX digit, eg. 0-9, A-F or a-f
uint8_t hexToNibble(char digit)
{
	if (digit >= 'a')
	{
		return digit - 'a' + 10;
	}
	else if (digit >= 'A')
	{
		return digit - 'A' + 10;
	}
	else
	{
		return digit - '0';
	}
}

char nibbleToHex(uint8_t nibble)
{
	char c;
	
	if (nibble < 10)
	{
		c = nibble + '0';
	}
	else
	{
		c = nibble - 10 + 'A';
	}
	
	return c;
}



ISR(USART0_DRE_vect) {
	// Data Register Empty
	static uint8_t lrc;
	static uint8_t bufPtr;
	
	// Red LED on
	PORTD.OUTSET = 0x80;
	
	switch (modbusState)
	{
		case MODBUS_STATE_SENDSTART:
			USART0.TXDATAL = ':';						// send start character
			USART0.STATUS |= USART_TXCIF_bm;			// Clear TX complete flag
			bufPtr = 0;
			lrc = 0;
			modbusState = MODBUS_STATE_SENDDATA1;
			break;
		case MODBUS_STATE_SENDDATA1:
			if (bufPtr == modbus_buflen)
			{
				// all data sent, proceed with LRC
				lrc = -lrc;
				USART0.TXDATAL = nibbleToHex(lrc >> 4);		// send LRC byte (first hex digit in byte)
				USART0.STATUS |= USART_TXCIF_bm;			// Clear TX complete flag
				modbusState = MODBUS_STATE_SENDLRC2;
			}
			else
			{
				USART0.TXDATAL = nibbleToHex(modbus_buf[bufPtr] >> 4);	// send data byte (first hex digit in byte)
				USART0.STATUS |= USART_TXCIF_bm;						// Clear TX complete flag
				modbusState = MODBUS_STATE_SENDDATA2;				
			}
			break;
		case MODBUS_STATE_SENDDATA2:
			USART0.TXDATAL = nibbleToHex(modbus_buf[bufPtr] & 0x0f);	// send data byte (last hex digit in byte)
			USART0.STATUS |= USART_TXCIF_bm;							// Clear TX complete flag
			lrc += modbus_buf[bufPtr++];								// update LRC
			modbusState = MODBUS_STATE_SENDDATA1;
			break;
		case MODBUS_STATE_SENDLRC2:
			USART0.TXDATAL = nibbleToHex(lrc & 0x0f);		// send LRC byte (last hex digit in byte)
			USART0.STATUS |= USART_TXCIF_bm;				// Clear TX complete flag
			modbusState = MODBUS_STATE_SENDCR;
			break;
		case MODBUS_STATE_SENDCR:
			USART0.TXDATAL = '\r';					// send CR
			USART0.STATUS |= USART_TXCIF_bm;		// Clear TX complete flag
			modbusState = MODBUS_STATE_SENDLF;
			break;	
		case MODBUS_STATE_SENDLF:
			USART0.TXDATAL = '\n';					// send LF
			USART0.STATUS |= USART_TXCIF_bm;		// Clear TX complete flag
			USART0.CTRLA &= ~USART_DREIE_bm;	// disable Data Register Empty interrupt
			modbusState = MODBUS_STATE_WAITSTART;
			break;
		default:
			USART0.CTRLA &= ~USART_DREIE_bm;	// disable Data Register Empty interrupt
			modbusState = MODBUS_STATE_WAITSTART;
	}
	
	
	// Red LED off
	PORTD.OUTCLR = 0x80;
	
}


ISR(USART0_RXC_vect) {
	uint8_t rxData;
	uint8_t rxFlags;
	
	// Red LED on
	PORTD.OUTSET = 0x80;
	
	// read bytes from FIFO
	rxFlags = USART0.RXDATAH;
	rxData = USART0.RXDATAL;
	
	
	if (rxFlags & USART_FERR_bm)
	{
		// framing error
		// Red LED off
	PORTD.OUTCLR = 0x80;
		return;	// ignore data
	}
	else if (rxFlags & USART_BUFOVF_bm)
	{
		// buffer overflow
		// Red LED off
	PORTD.OUTCLR = 0x80;
		return;	// ignore data
	}
	else
	{
		// RX OK
		
		if (USART0.CTRLA & USART_DREIE_bm)
		{
			// Data Register Empty interrupt enabled -> TX busy (ignore RX while transmitting)
			// Red LED off
	PORTD.OUTCLR = 0x80;
			return;
		}
		
		if (modbusState > MODBUS_STATE_WAITEND)
		{
			// do nothing
			// Red LED off
	PORTD.OUTCLR = 0x80;
			return;
		}
		
		
		if (rxData == ':')
		{
			// start of frame, reset buffer and start waiting for address
			modbus_buflen = 0;
			modbus_recvSum = 0;	// clear LRC
			modbusState = MODBUS_STATE_WAITDATA1;
			// Red LED off
	PORTD.OUTCLR = 0x80;
			return;
			
		}
		else if (rxData == '\r')
		{
			// end of packet 1
			
			if ((modbusState != MODBUS_STATE_WAITDATA1) || (modbus_buflen < 3))
			{
				// unexpected end of packet char received, start over
				modbusState = MODBUS_STATE_WAITSTART;
			}
			else
			{
				modbusState = MODBUS_STATE_WAITEND;
			}
		}
		else if (rxData == '\n')
		{
			// end of packet 2
			if (modbusState != MODBUS_STATE_WAITEND)
			{
				// unexpected end of packet char received, start over
				modbusState = MODBUS_STATE_WAITSTART;
			}
			else
			{
				// entire packet received, do LRC check
				// in case of correct reception the sum of all data bytes will be zero
				if (modbus_recvSum != 0)
				{
					// LRC check failed, discard data and start over
					modbusState = MODBUS_STATE_WAITSTART;
				}
				else
				{
					// data OK, strip LRC
					modbus_buflen--;
					
					modbusState = MODBUS_STATE_WAITPROC;
					//modbus_pend = true;
				}
			}			
		}
		else if (isxdigit(rxData))
		//else if (((rxData >= '0') && (rxData <= '9')) || ((rxData >= 'A') && (rxData <= 'F')) || ((rxData >= 'a') && (rxData <= 'f')))
		{
			// valid hex char
			if (modbusState == MODBUS_STATE_WAITDATA1)
			{	
				// high nibble received
				if (modbus_buflen >= sizeof(modbus_buf))
				{
					// buffer overflow, discard and start over
					modbusState = MODBUS_STATE_WAITSTART;
				}
				else
				{
					modbus_buf[modbus_buflen] = hexToNibble(rxData) << 4;	// high nibble
					modbusState = MODBUS_STATE_WAITDATA2;
				}
			}
			else if (modbusState == MODBUS_STATE_WAITDATA2)
			{
				// low nibble received
				modbus_buf[modbus_buflen] |= hexToNibble(rxData);	// low nibble
				
				if ((modbus_buflen == 0) && (modbus_buf[0] != modbus_addr))
				{
					// not our address, start over
					modbusState = MODBUS_STATE_WAITSTART;
				}
				else
				{
					modbus_recvSum += modbus_buf[modbus_buflen++];	// update LRC
					modbusState = MODBUS_STATE_WAITDATA1;
				}
			}
		}
	}
	
	
	// Red LED off
	PORTD.OUTCLR = 0x80;
}

uint8_t writeCoil(uint16_t addr, bool data)
{
	
	switch (addr)
	{
		case MODBUS_COIL_FINDBATT:
			if (data)
			{
				// find battery packs
				batt_req_init();
			}
			break;
		case MODBUS_COIL_EESAVE:
			if (data)
			{
				// EE save
				EE_requestWrite();
			}
			break;
		case MODBUS_COIL_SIM_STATUS_WREN:
			sim_status_wr_en = data;
			break;			
		default:
			return MODBUS_ERR_ILLEGAL_DATA_ADDR;	// address not found
	}
	return 0;
	
}


uint8_t readCoil(uint16_t addr, bool *data)
{
	
	switch (addr)
	{
		case MODBUS_COIL_FINDBATT:
			*data = batt_status_init();
			break;
		case MODBUS_COIL_EESAVE:
			*data = EE_writePending();
			break;
		case MODBUS_COIL_SIM_STATUS_WREN:
			*data = sim_status_wr_en;
			break;
		default:
			return MODBUS_ERR_ILLEGAL_DATA_ADDR;	// address not found
	}
	return 0;
	
}



uint8_t readCoils(uint16_t addr, uint16_t numCoils)
{
	if ((numCoils > 1000))
	{
		return MODBUS_ERR_ILLEGAL_DATA_ADDR;	// too many coils requested
	}
	
	uint8_t bytes = (numCoils + 7) / 8;	// Round upwards
	
	modbus_buf[2] = bytes;	// number of data bytes to follow
	modbus_buflen = 3 + (bytes);
	
	uint8_t modbus_buf_idx = 3;	// first data byte at pos 3
	
	bool coilValue;
	uint8_t retVal;
	
	while (bytes)
	{
		
		modbus_buf[modbus_buf_idx] = 0;
		
		for (uint8_t bitpos = 0; bitpos < 8; bitpos++)
		{
			if (numCoils == 0)
			{
				break;
			}
			
			retVal = readCoil(addr, &coilValue);
			
			if (retVal != 0)
			{
				return retVal;
			}
			
			modbus_buf[modbus_buf_idx] |= coilValue << bitpos;
			
			addr++;
			numCoils--;
		}
		
		bytes--;
		modbus_buf_idx++;
		
	}

	return 0;
	
}



uint8_t readHoldingRegs(uint16_t addr, uint16_t numRegs)
{
	if (numRegs > 125)
	{
		return MODBUS_ERR_ILLEGAL_DATA_ADDR;	// too many regs requested
	}
	
	
	modbus_buf[2] = numRegs * 2;	// number of data bytes to follow
	modbus_buflen = 3 + (numRegs * 2);
	
	uint8_t modbus_buf_idx = 3;	// first data byte at pos 3
	
	uint16_t *pParams = (uint16_t*)&bms_limits;
	uint16_t *pSimData = (uint16_t*)&sim_modules;
	
	while (numRegs)
	{		
		if ((addr >= MODBUS_HOLDING_PARAMS) && ((addr - MODBUS_HOLDING_PARAMS)  < sizeof(bms_limits) / 2))
		{
			// Read parameter
			modbus_buf[modbus_buf_idx++] = pParams[addr - MODBUS_HOLDING_PARAMS] >> 8;		// MSB
			modbus_buf[modbus_buf_idx++] = pParams[addr - MODBUS_HOLDING_PARAMS] & 0xFF;	// LSB
			
		}
		else if (addr == MODBUS_HOLDING_SIM_MODE_KEY)
		{
			// Read sim key
			modbus_buf[modbus_buf_idx++] = simKey >> 8;		// MSB
			modbus_buf[modbus_buf_idx++] = simKey & 0xFF;	// LSB	
		}
		else if (addr == MODBUS_HOLDING_SIM_MODULE_COUNT)
		{
			// Read sim modules count
			modbus_buf[modbus_buf_idx++] = 0;					// MSB
			modbus_buf[modbus_buf_idx++] = sim_numFoundModules;	// LSB
		}
		else if (addr == MODBUS_HOLDING_SIM_BATT_CURRENT)
		{
			// Read sim batt current
			modbus_buf[modbus_buf_idx++] = sim_batt_current >> 8;		// MSB
			modbus_buf[modbus_buf_idx++] = sim_batt_current & 0xFF;		// LSB	
		}
		else if ((addr >= MODBUS_HOLDING_SIM_MODULE_DATA) && ((addr - MODBUS_HOLDING_SIM_MODULE_DATA)  < sizeof(sim_modules) / 2))
		{
			// Read sim data
			modbus_buf[modbus_buf_idx++] = pSimData[addr - MODBUS_HOLDING_SIM_MODULE_DATA] >> 8;		// MSB
			modbus_buf[modbus_buf_idx++] = pSimData[addr - MODBUS_HOLDING_SIM_MODULE_DATA] & 0xFF;		// LSB
		}
		else
		{
			return MODBUS_ERR_ILLEGAL_DATA_ADDR;	// address not found
		}
		
		numRegs--;
		addr++;
	}

	return 0;
	
}


uint8_t writeHoldingRegs(uint16_t addr, uint16_t numRegs)
{
	if ((numRegs > 125) || (numRegs * 2 != modbus_buf[6]))
	{
		return MODBUS_ERR_ILLEGAL_DATA_ADDR;	// too many registers or invalid byte count
	}
	
	modbus_buflen = 6;
	
	uint8_t modbus_buf_idx = 7;	// first data byte at pos 7
		
	uint16_t *pParams = (uint16_t*)&bms_limits;
	uint16_t *pSimData = (uint16_t*)&sim_modules;
	
	bool increaseReadOutCnt = false;
	
	while (numRegs)
	{		
		if ((addr >= MODBUS_HOLDING_PARAMS) && ((addr - MODBUS_HOLDING_PARAMS)  < sizeof(bms_limits) / 2))
		{
			// Write parameter
			uint16_t tempParam = modbus_buf[modbus_buf_idx++];	// MSB
			tempParam <<= 8;									// Shift MSB
			tempParam |= modbus_buf[modbus_buf_idx++];			// LSB
			pParams[addr - MODBUS_HOLDING_PARAMS] = tempParam;	// Set parameter
			
		}
		else if (addr == MODBUS_HOLDING_SIM_MODE_KEY)
		{
			// Write sim mode key
			uint16_t tempParam = modbus_buf[modbus_buf_idx++];	// MSB
			tempParam <<= 8;									// Shift MSB
			tempParam |= modbus_buf[modbus_buf_idx++];			// LSB
			simKey = tempParam;
					
		}
		else if (addr == MODBUS_HOLDING_SIM_MODULE_COUNT)
		{
			// Write sim modules count
			uint16_t tempParam = modbus_buf[modbus_buf_idx++];	// MSB
			tempParam <<= 8;									// Shift MSB
			tempParam |= modbus_buf[modbus_buf_idx++];			// LSB
			
			
			if (tempParam > SIM_MAX_MODULES)
			{
				sim_numFoundModules = SIM_MAX_MODULES;
			}
			else
			{
				sim_numFoundModules = tempParam;
			}
			
		}
		else if (addr == MODBUS_HOLDING_SIM_BATT_CURRENT)
		{
			// Write sim batt current
			uint16_t tempParam = modbus_buf[modbus_buf_idx++];	// MSB
			tempParam <<= 8;									// Shift MSB
			tempParam |= modbus_buf[modbus_buf_idx++];			// LSB
			sim_batt_current = tempParam;
			
		}
		else if ((addr >= MODBUS_HOLDING_SIM_MODULE_DATA) && ((addr - MODBUS_HOLDING_SIM_MODULE_DATA)  < sizeof(sim_modules) / 2))
		{
			// Write sim data
			uint16_t tempParam = modbus_buf[modbus_buf_idx++];				// MSB
			tempParam <<= 8;												// Shift MSB
			tempParam |= modbus_buf[modbus_buf_idx++];						// LSB			
			
			if (((addr - MODBUS_HOLDING_SIM_MODULE_DATA) % 13) > 9)
			{
				// status bits
				
				if (sim_status_wr_en)
				{
					pSimData[addr - MODBUS_HOLDING_SIM_MODULE_DATA] = tempParam;	// Set data				
				}
			}
			else
			{
				// normal data
				pSimData[addr - MODBUS_HOLDING_SIM_MODULE_DATA] = tempParam;	// Set data
				increaseReadOutCnt = true;	
			}
			
			

			
			
		}
		else
		{
			return MODBUS_ERR_ILLEGAL_DATA_ADDR;	// address not found
		}
		
		numRegs--;
		addr++;
	}
	
	if (increaseReadOutCnt)
		sim_data_readouts_done++;

	return 0;
	
}





uint8_t readRegs(uint16_t addr, uint16_t numRegs)
{
	if (numRegs > 125)
	{
		return MODBUS_ERR_ILLEGAL_DATA_ADDR;	// too many regs requested
	}
	
	
	
	modbus_buf[2] = numRegs * 2;	// number of data bytes to follow
	modbus_buflen = 3 + (numRegs * 2);
	
	//uint8_t modbus_buf_idx = 3;	// first data byte at pos 3
	
	uint8_t *pModbus_buf = &modbus_buf[3];
	
	while (numRegs)
	{
		
		if (addr == 99)
		{
			// number of modules			
			*pModbus_buf++ = 0;	// MSB
			*pModbus_buf++ = batt_get_module_cnt();	// LSB
			
		}
		else if ((addr >= 100) && (addr <= 1699))
		{
			// read battery pack data
			uint8_t battPack = addr / 100 - 1;	// which battery pack?
			uint8_t valAddr = addr % 100;		// which value?
		
			if (valAddr == 0)
			{
				// get module voltage
				uint16_t modVolt = batt_get_module_volt(battPack);
				*pModbus_buf++ = modVolt >> 8;	// MSB
				*pModbus_buf++ = modVolt & 0xFF;	// LSB
			}
			else if ((valAddr >= 1) && (valAddr <= 6))
			{
				// get cell voltage
				uint16_t cellVolt = batt_get_cell_volt(battPack, valAddr-1);
				*pModbus_buf++ = cellVolt >> 8;	// MSB
				*pModbus_buf++ = cellVolt & 0xFF;	// LSB
			}
			else if ((valAddr >= 7) && (valAddr <= 8))
			{
				// get temperature
				int16_t temp = batt_get_temperature(battPack, valAddr-7);
				*pModbus_buf++ = temp >> 8;	// MSB
				*pModbus_buf++ = temp & 0xFF;	// LSB
			}
			else if (valAddr == 9)
			{
				// alerts, faults
				*pModbus_buf++ = batt_get_module_alerts(battPack);	// MSB
				*pModbus_buf++ = batt_get_module_faults(battPack);	// LSB
			}
			else if (valAddr == 10)
			{
				// status, balance state
				*pModbus_buf++ = batt_get_module_status(battPack);	// MSB
				*pModbus_buf++ = batt_get_module_bal(battPack);	// LSB
			}
			else
			{
				return MODBUS_ERR_ILLEGAL_DATA_ADDR;	// address not found
			}
		
		}
		else if (addr == MODBUS_INPUT_COMP_MCU_TEMP)
		{
			// Read comp mcu state data
			*pModbus_buf++ = 0;		// MSB
			*pModbus_buf++ = comp_mcu_state.mcu_temp;		// LSB			
		}
		else if (addr == MODBUS_INPUT_COMP_V_SUPPLY)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.v_supply >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.v_supply & 0xFF;		// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_VDD)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.vdd >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.vdd & 0xFF;		// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_I_SENSE)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.i_sense >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.i_sense & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_CONT_MON)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.cont_mon >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.cont_mon & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_I_REF)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.i_ref >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.i_ref & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_I_DIAG)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.i_diag >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.i_diag & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_ESTOP_MON)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.estop_mon >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.estop_mon & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_IN1_V)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.inp_v[0] >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.inp_v[0] & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_IN2_V)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.inp_v[1] >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.inp_v[1] & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_IN3_V)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.inp_v[2] >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.inp_v[2] & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_IN4_V)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.inp_v[3] >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.inp_v[3] & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_IN5_V)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.inp_v[4] >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.inp_v[4] & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_IN6_V)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.inp_v[5] >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.inp_v[5] & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_IN7_V)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.inp_v[6] >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.inp_v[6] & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_IN8_V)
		{
			// Read comp mcu state data
			*pModbus_buf++ = comp_mcu_state.inp_v[7] >> 8;		// MSB
			*pModbus_buf++ = comp_mcu_state.inp_v[7] & 0xFF;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_OUT)
		{
			// Read comp mcu state data
			*pModbus_buf++ = 0;		// MSB
			*pModbus_buf++ = comp_mcu_state.outp_state;	// LSB
		}
		else if (addr == MODBUS_INPUT_COMP_CONT_STATE)
		{
			// Read comp mcu state data
			*pModbus_buf++ = 0;		// MSB
			*pModbus_buf++ = comp_mcu_state.cont_state;	// LSB
		}
		else if (addr == MODBUS_INPUT_CONT_LOCKOUT)
		{
			*pModbus_buf++ = cont_lockout >> 8;		// MSB
			*pModbus_buf++ = cont_lockout & 0xFF;		// LSB
		}
		else if (addr == MODBUS_INPUT_BATT_COMM_OK_CNT)
		{
			uint32_t tmp32;
			
			cli();
			tmp32 = batt_comm_stats.uart_ok_cnt;
			sei();
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB	
				numRegs--;		
				addr++;	
			}
		}
		
		else if (addr == MODBUS_INPUT_BATT_COMM_BUF_OVF_CNT)
		{
			uint32_t tmp32;
			
			cli();
			tmp32 = batt_comm_stats.uart_buf_ovf_cnt;
			sei();
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_BATT_COMM_CRCERR_CNT)
		{
			uint32_t tmp32;
			
			cli();
			tmp32 = batt_comm_stats.uart_crcerr_cnt;
			sei();
			
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_BATT_COMM_DATAERR_CNT)
		{
			uint32_t tmp32;
			
			cli();
			tmp32 = batt_comm_stats.uart_dataerr_cnt;
			sei();
					
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
					
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_BATT_COMM_FERR_CNT)
		{
			uint32_t tmp32;
			
			cli();
			tmp32 = batt_comm_stats.uart_ferr_cnt;
			sei();
			
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_BATT_COMM_NOREPLY_CNT)
		{
			uint32_t tmp32;
			
			cli();
			tmp32 = batt_comm_stats.uart_noreply_cnt;
			sei();
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_BATT_COMM_OVF_CNT)
		{
			uint32_t tmp32;

			cli();
			tmp32 = batt_comm_stats.uart_ovf_cnt;
			sei();
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}	
		else if (addr == MODBUS_INPUT_BATT_COMM_TIMEOUT_CNT)
		{
			uint32_t tmp32;
			
			cli();
			tmp32 = batt_comm_stats.uart_timeout_cnt;
			sei();
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}	
		
		else if (addr == MODBUS_INPUT_I2C_TRANSACTIONS)
		{
			uint32_t tmp32;
			
			cli();
			tmp32 = i2c_comm_stats.i2c_transactions;
			sei();
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_I2C_BUS_ERR_CNT)
		{
			uint32_t tmp32;
			
			cli();
			tmp32 = i2c_comm_stats.i2c_bus_err_cnt;
			sei();
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_I2C_NACK_ADDR_CNT)
		{
			uint32_t tmp32;
			
			cli();
			tmp32 = i2c_comm_stats.i2c_nack_addr;
			sei();
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_I2C_NACK_DATA_CNT)
		{
			uint32_t tmp32;
			
			cli();
			tmp32 = i2c_comm_stats.i2c_nack_data;
			sei();
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_I2C_UNKNOWN_IRQ_CNT)
		{
			uint32_t tmp32;
			
			cli();
			tmp32 = i2c_comm_stats.i2c_unknown_irq;
			sei();
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_I2C_STUCK_SDA_CYCLES)
		{
			uint32_t tmp32;
			
			tmp32 = i2c_comm_stats.i2c_stuck_sda_cycles;
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_I2C_STUCK_SCL_CYCLES)
		{
			uint32_t tmp32;
			
			tmp32 = i2c_comm_stats.i2c_stuck_scl_cycles;
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_MAINLOOP_CYCLES)
		{
			uint32_t tmp32;
			
			tmp32 = mainloop_cycles;
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}
		else if (addr == MODBUS_INPUT_SYSTICK)
		{
			uint32_t tmp32;
			
			tmp32 = SysTick_GetTicks();
			
			*pModbus_buf++ = tmp32 >> 24;		// MSB
			*pModbus_buf++ = tmp32 >> 16;
			
			if (numRegs > 1)
			{
				*pModbus_buf++ = tmp32 >> 8;
				*pModbus_buf++ = tmp32 & 0xFF;	// LSB
				numRegs--;
				addr++;	
			}
		}		
		else
		{
			return MODBUS_ERR_ILLEGAL_DATA_ADDR;	// address not found
		}
		
		numRegs--;
		addr++;
	}

	return 0;
	
}

void sendReply(uint8_t exception)
{
	if (exception != 0)
	{
		// exception
		modbus_buf[1] |= 0x80;	// set highest bit in FC to indicate exception status
		modbus_buf[2] = exception;
		modbus_buflen = 3;
	}
	
	// start TX
	modbusState = MODBUS_STATE_SENDSTART;
	USART0.CTRLA |= USART_DREIE_bm;	// Enable Data Register Empty interrupt, this will trigger the TX sequence	
	
}





void modbus_start()
{
	USART0.CTRLA |= USART_RXCIE_bm;	// enable UART interrupts
}



void modbus_state_machine()
{
	
	if (modbusState == MODBUS_STATE_WAITPROC)
	{
		// message waiting
		
		// packet received
		uint8_t exception = 0;
						
		// check function code
		switch (modbus_buf[1])
		{
			case MODBUS_FUNC_WRITE_COIL:
							
				if (modbus_buflen == 6)
				{
					if ((modbus_buf[4] == 0xFF) && (modbus_buf[5] == 0x00))
					{
						// coil value = 1
						exception = writeCoil(((uint16_t)modbus_buf[2] << 8) | modbus_buf[3], true);
					}
					else if ((modbus_buf[4] == 0x00) && (modbus_buf[5] == 0x00))
					{
						// coil value = 0
						exception = writeCoil(((uint16_t)modbus_buf[2] << 8) | modbus_buf[3], false);
					}
					else
					{
						// illegal value
						exception = MODBUS_ERR_ILLEGAL_DATA_VAL;
					}
				}
				else
				{
					// Send "Illegal Data Value" exception
					exception = MODBUS_ERR_ILLEGAL_DATA_VAL;
				}
				break;
			case MODBUS_FUNC_READ_COILS:

				if (modbus_buflen == 6)
				{
					exception = readCoils(((uint16_t)modbus_buf[2] << 8) | modbus_buf[3], ((uint16_t)modbus_buf[4] << 8) | modbus_buf[5]);
				}
				else
				{
					// Send "Illegal Data Value" exception
					exception = MODBUS_ERR_ILLEGAL_DATA_VAL;
				}
				break;
			case MODBUS_FUNC_READ_INPUT_REGS:

				if (modbus_buflen == 6)
				{
					exception = readRegs(((uint16_t)modbus_buf[2] << 8) | modbus_buf[3], ((uint16_t)modbus_buf[4] << 8) | modbus_buf[5]);
				}
				else
				{
					// Send "Illegal Data Value" exception
					exception = MODBUS_ERR_ILLEGAL_DATA_VAL;
				}
				break;
						
						
			case MODBUS_FUNC_READ_HOLDING_REGS:
				if (modbus_buflen == 6)
				{
					exception = readHoldingRegs(((uint16_t)modbus_buf[2] << 8) | modbus_buf[3], ((uint16_t)modbus_buf[4] << 8) | modbus_buf[5]);
				}
				else
				{
					// Send "Illegal Data Value" exception
					exception = MODBUS_ERR_ILLEGAL_DATA_VAL;
				}
				break;
			
			case MODBUS_FUNC_WRITE_HOLDING_REGS:
				if (modbus_buflen >= 9)
				{
					exception = writeHoldingRegs(((uint16_t)modbus_buf[2] << 8) | modbus_buf[3], ((uint16_t)modbus_buf[4] << 8) | modbus_buf[5]);
				}
				else
				{
					// Send "Illegal Data Value" exception
					exception = MODBUS_ERR_ILLEGAL_DATA_VAL;
				}
				break;
			
			
			default:
				exception = MODBUS_ERR_ILLEGAL_FUNC;
		}
						
		sendReply(exception);
		
	}
	
}
