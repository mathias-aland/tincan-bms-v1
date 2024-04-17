/*
 * batt.c
 *
 * Created: 09-10-2023 01:12:27
 *  Author: Mathias
 */ 

#include "clkctrl.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/crc16.h>
#include "bmsmodule.h"
#include "globals.h"


#define GUARD_MSEC		10000
#define GUARD_USEC		10
#define GUARD_TIME_MIN	(500 * GUARD_USEC) // minimum guard time after RX done


#define RXBUF_SIZE	32
#define TXBUF_SIZE  32

enum
{
	RX_STATUS_IDLE = 0,
	RX_STATUS_BUSY,
	RX_STATUS_OK,
	RX_STATUS_NOREPLY,
	RX_STATUS_CRC_ERR,
	RX_STATUS_TIMEOUT,
	RX_STATUS_OVF,
	RX_STATUS_FERR
};

enum
{
	COMM_STATUS_IDLE = 0,
	COMM_STATUS_BUSY,
	COMM_STATUS_GUARD,
	COMM_STATUS_GUARD_RETRY,
};


volatile uint8_t retries = 0;
volatile static uint16_t wait_ms = 0;
volatile uint16_t guard_time = 0;


volatile uint8_t rxBuf[RXBUF_SIZE];
volatile uint8_t rxBuf_cnt = 0;	// number of bytes received
volatile uint8_t rxBuf_len = 0;	// number of bytes to receive
volatile uint8_t rxStatus = RX_STATUS_IDLE;

volatile uint8_t txBuf[TXBUF_SIZE];
volatile uint8_t txBuf_cnt = 0;	// number of bytes sent
volatile uint8_t txBuf_len = 0;	// number of bytes to send
volatile uint8_t txStatus = COMM_STATUS_IDLE;

volatile uint8_t batt_state = BATT_STATE_IDLE;

uint8_t numFoundModules;

volatile BMSModule modules[MAX_MODULES]; // store data for as many modules as we've configured for.

bool batt_init_pend = false;
bool batt_init_ok = false;

bool batt_clear_alarms_pend = false;
bool batt_clear_alarms_ok = false;
uint8_t batt_clear_alarms_alertbits = 0;
uint8_t batt_clear_alarms_faultbits = 0;


bool batt_balance_set_pend = false;
bool batt_balance_set_ok = false;
uint8_t batt_balance_pack = 0;
uint8_t batt_balance_cells = 0;
uint8_t batt_balance_timeout = 0;

volatile uint8_t bat_datacollection_timer;


uint32_t data_readouts_done = 0;


ISR(TCB1_INT_vect) {
	
	// Red LED on
	PORTD.OUTSET = 0x80;
	
	if (TCB1.INTFLAGS & TCB_CAPT_bm)
	{
		// capture
		// calculate number of modules from delay
		// ~21us per module (13 bit-times)
		
		
		//uint16_t capt = TCB1.CCMP;
		//capt = (capt + (210/2)) / 210;
		
		TCB1.CTRLA &= ~TCB_ENABLE_bm;				// stop TCB1
		TCB1.CNT = 0;								// reset counter
		TCB1.INTFLAGS = TCB_CAPT_bm;				// clear flag		
		
		if (txStatus == COMM_STATUS_GUARD)
		{
			// guard time expired, go to IDLE state
			txStatus = COMM_STATUS_IDLE;
		}
		else if (txStatus == COMM_STATUS_GUARD_RETRY)
		{
			// retry TX/RX
			
			txBuf_cnt = 0;			// reset TX buffer counter
			rxBuf_cnt = 0; 			// reset RX buffer counter
				
			// Enable receiver
			USART1.CTRLB |= USART_RXEN_bm;
				
			// start timeout counter
			TCB1.CCMP = 20000;	// 2 ms
			TCB1.CTRLA |= TCB_ENABLE_bm;				// start TCB1
			
			// start TX
			USART1.CTRLA |= USART_DREIE_bm;	// Enable Data Register Empty interrupt, this will trigger the TX sequence
			
			txStatus = COMM_STATUS_BUSY;
			
		}
		else
		{
			// timeout
			USART1.CTRLB &= ~USART_RXEN_bm;		// disable receiver
			USART1.CTRLA &= ~USART_DREIE_bm;	// disable Data Register Empty interrupt
			//USART1.CTRLA &= ~USART_TXCIE_bm;	// disable Transmit Complete interrupt

			rxStatus = RX_STATUS_TIMEOUT;
			batt_comm_stats.uart_timeout_cnt++;	
			
			if (retries)
			{
				txStatus = COMM_STATUS_GUARD_RETRY;
				retries--;
			}
			else
			{

				txStatus = COMM_STATUS_GUARD;				
			}

			TCB1.CCMP = 50000;	// 5 ms	
			TCB1.CTRLA |= TCB_ENABLE_bm;				// start TCB1
			
		
		}
		
	}
	
	
	if (TCB1.INTFLAGS & TCB_OVF_bm)
	{
		// overflow
		
		TCB1.INTFLAGS = TCB_OVF_bm;				// clear flag
	}
	
	
	// Red LED off
	PORTD.OUTCLR = 0x80;
	
}

ISR(USART1_DRE_vect) {
	// Data Register Empty
	
	// Red LED on
	PORTD.OUTSET = 0x80;
	
	if (txBuf_cnt < txBuf_len)
	{
		USART1.TXDATAL = txBuf[txBuf_cnt++];		// send byte
		USART1.STATUS |= USART_TXCIF_bm;			// Clear TX complete flag
	}
	
	if (txBuf_cnt == txBuf_len)
	{
		// All data pushed to TX buffer
		USART1.CTRLA &= ~USART_DREIE_bm;	// disable Data Register Empty interrupt
		//USART1.CTRLA |= USART_TXCIE_bm;		// enable Transmit Complete interrupt
	}
	
	// Red LED off
	PORTD.OUTCLR = 0x80;
	
}

//ISR(USART1_TXC_vect) {
	//// TX complete
	//USART1.CTRLA &= ~USART_TXCIE_bm;	// disable Transmit Complete interrupt
//}


ISR(USART1_RXC_vect) {
	static uint8_t rxCrc;
	static bool rxBlockBit;
	uint8_t rxData;
	uint8_t rxFlags;
	bool notifyDone = false;
	
	// Red LED on
	PORTD.OUTSET = 0x80;
	
	// read bytes from FIFO
	rxFlags = USART1.RXDATAH;
	rxData = USART1.RXDATAL;
	
	
	if (rxFlags & USART_FERR_bm)
	{
		// framing error
		rxStatus = RX_STATUS_FERR;
		
		batt_comm_stats.uart_ferr_cnt++;
		
		// Notify on RX complete
		notifyDone = true;
	}
	else if (rxFlags & USART_BUFOVF_bm)
	{
		// buffer overflow
		rxStatus = RX_STATUS_OVF;
		
		batt_comm_stats.uart_ovf_cnt++;
		
		// Notify on RX complete
		notifyDone = true;
	}
	else
	{
		// RX OK
		if (rxBuf_cnt == 0)
		{
			rxCrc = 0;	// clear CRC
			
			// check if blocking bit is set
			if (rxData & 0x80)
			{
				rxBlockBit = true;
				rxData &= 0x7f;
			}
			else
			{
				rxBlockBit = false;
			}
		}
		
	
		if (rxBuf_cnt < RXBUF_SIZE)
		{
			
			if ((rxBuf_cnt < 3) && (txBuf[rxBuf_cnt] != rxData))	// first 3 bytes should be identical in TX/RX buffers
			{
				// mismatch
				rxStatus = RX_STATUS_CRC_ERR;
				
				batt_comm_stats.uart_dataerr_cnt++;
			
				// Notify on RX complete
				notifyDone = true;
			}				
			else
			{			
				rxBuf[rxBuf_cnt++] = rxData;
			
				if (txBuf[0] & 0x01)  // check if bit 0 is set (write)
				{
					// write
					if (rxBuf_cnt == 4)
					{
						// writes are always 4 bytes
						// All expected data received
					
						if (txBuf[3] != rxData)	// check last byte
						{
							rxStatus = RX_STATUS_CRC_ERR;
							
							batt_comm_stats.uart_dataerr_cnt++;
						}
						else if ((txBuf[0] == 1) && (!rxBlockBit)) // if packet was sent to address 0, check if blocking bit is set
						{
							// yes, but blocking bit not set
							rxStatus = RX_STATUS_NOREPLY;
							
							batt_comm_stats.uart_noreply_cnt++;
						}
						else
						{
							rxStatus = RX_STATUS_OK;
							
							batt_comm_stats.uart_ok_cnt++;
						}
					
						// Notify on RX complete
						notifyDone = true;
					}
				}
				else
				{
					// read
				
					if ((rxBuf_cnt == 3) && (txBuf[0] == 0) && (!rxBlockBit))
					{
						// if packet was sent to address 0, check if blocking bit is set, if not only 3 bytes are expected (no answer)
						rxStatus = RX_STATUS_NOREPLY;
						
						batt_comm_stats.uart_noreply_cnt++;
											
						// Notify on RX complete
						notifyDone = true;
					}
					else if (rxBuf_cnt == (txBuf[2] + 4))
					{
						// All expected data received
						// last byte will be CRC
						
						if (rxCrc != rxData)
						{
							// CRC error
							rxStatus = RX_STATUS_CRC_ERR;
							batt_comm_stats.uart_crcerr_cnt++;
						}
						else
						{
							rxStatus = RX_STATUS_OK;
							
							batt_comm_stats.uart_ok_cnt++;
						}
					
						// Notify on RX complete
						notifyDone = true;
					}
					else
					{
						rxCrc = _crc8_ccitt_update(rxCrc, rxBuf_cnt == 1 ? rxData & 0x7f : rxData);	// update CRC
					}
				}
			}			
		}
		else
		{
			// buffer overflow
			rxStatus = RX_STATUS_OVF;
			
			batt_comm_stats.uart_buf_ovf_cnt++;
			
			// Notify on RX complete
			notifyDone = true;
		}
	}
	
	if (notifyDone)
	{
		
		USART1.CTRLB &= ~USART_RXEN_bm;		// disable receiver
		USART1.CTRLA &= ~USART_DREIE_bm;	// disable Data Register Empty interrupt
		//USART1.CTRLA &= ~USART_TXCIE_bm;	// disable Transmit Complete interrupt
		
		
		TCB1.CTRLA &= ~TCB_ENABLE_bm;				// stop TCB1
		TCB1.CNT = 0;								// reset counter
		
		if ((rxStatus == RX_STATUS_OK) || (rxStatus == RX_STATUS_NOREPLY))
		{
			// if data RX is OK we only need ~10 UART frames (158 us) of idle time.
			TCB1.CCMP = guard_time;
			txStatus = COMM_STATUS_GUARD;
		}
		else
		{
			// if data is not OK, wait 5 ms before resuming comms to allow for everything to go into idle state
			TCB1.CCMP = 50000;	// 5 ms
			
			if (retries)
			{
				txStatus = COMM_STATUS_GUARD_RETRY;
				retries--;			
			}

		}
		

		TCB1.INTFLAGS = TCB_OVF_bm | TCB_CAPT_bm;	// clear flags
		TCB1.CTRLA |= TCB_ENABLE_bm;				// start TCB1
	}
	
	
	// Red LED off
	PORTD.OUTCLR = 0x80;
	
}





void BATT_systick_cb()
{
	// this will be called from systick ISR every 1 ms. Check timeouts etc:
	
	
	// no need for atomic block as this is run in ISR context
	// only count when comms are idle
	if ((txStatus == COMM_STATUS_IDLE) && (wait_ms > 0))
	{
		// wait active
		wait_ms--;
	}
	
	if (bat_datacollection_timer > 0)
		bat_datacollection_timer--;
	
}


uint8_t calcCRC(uint8_t *input, uint8_t lenInput)
{
	uint8_t crc = 0;
	
	while (lenInput--)
	{
		crc = _crc8_ccitt_update(crc, *input++);	// update CRC
	}
	
	return crc;
}





void start_tx()
{
	txBuf_cnt = 0;			// reset TX buffer counter
	rxBuf_cnt = 0; 			// reset RX buffer counter
	
	txStatus = COMM_STATUS_BUSY;
	
	// Enable receiver & interrupt
	USART1.CTRLB |= USART_RXEN_bm;
	USART1.CTRLA |= USART_RXCIE_bm;
	
	// setup timeout counter
	TCB1.CCMP = 20000;	// 2 ms
	
	cli();
	USART1.CTRLA |= USART_DREIE_bm;		// Enable Data Register Empty interrupt, this will trigger the TX sequence
	TCB1.CTRLA |= TCB_ENABLE_bm;		// start TCB1
	sei();
	
}



bool batt_write_reg(uint8_t devAddr, uint8_t regAddr, uint8_t value, uint8_t retry_cnt, uint16_t guardTime)
{
	if (txStatus != COMM_STATUS_IDLE)
	{
		// transfer already in progress
		return false;
	}
	
	devAddr &= 0x3f;
	devAddr <<= 1;
	
	txBuf[0] = devAddr | 1;
	txBuf[1] = regAddr;
	txBuf[2] = value;
	txBuf[3] = calcCRC(txBuf, 3);
	txBuf_len = 4;			// send 4 bytes
	retries = retry_cnt;
	guard_time = guardTime;
	
	start_tx();
	
	return true;
}


bool batt_read_reg(uint8_t devAddr, uint8_t regAddr, uint8_t count, uint8_t retry_cnt, uint16_t guardTime)
{
	if (txStatus != COMM_STATUS_IDLE)
	{
		// transfer already in progress
		return false;
	}
	
	if (count + 4 > RXBUF_SIZE)
	{
		// requested too many bytes
		return false;
	}
	
	devAddr &= 0x3f;
	devAddr <<= 1;
	
	txBuf[0] = devAddr;
	txBuf[1] = regAddr;
	txBuf[2] = count;
	txBuf_len = 3;			// send 3 bytes
	retries = retry_cnt;
	guard_time = guardTime;
	
	start_tx();
	
	return true;
	
}




static void set_wait_ms(uint16_t ms)
{
	cli();
	wait_ms = ms;
	sei();
}


uint32_t batt_get_total_readouts()
{
	return data_readouts_done;
}

void batt_req_init()
{
	batt_init_pend = true;
}

bool batt_status_init()
{
	return batt_init_pend;
}

bool batt_result_init()
{
	return batt_init_ok;
}


void batt_req_clear(uint8_t alertMask, uint8_t faultMask)
{
	batt_clear_alarms_alertbits = alertMask;
	batt_clear_alarms_faultbits = faultMask;
	batt_clear_alarms_pend = true;
	batt_clear_alarms_ok = false;
}

bool batt_status_clear()
{
	return batt_clear_alarms_pend;
}

bool batt_result_clear()
{
	return batt_clear_alarms_ok;
}




void batt_req_setbal(uint8_t pack, uint8_t cells, uint8_t timeout)
{
	batt_balance_pack = pack;
	batt_balance_cells = cells;
	batt_balance_timeout = timeout;
	batt_balance_set_ok = false;
	batt_balance_set_pend = true;	
}

bool batt_bal_pend()
{
	return batt_balance_set_pend;
}

bool batt_bal_ok()
{
	return batt_balance_set_ok;
}

uint8_t batt_get_module_alerts(uint8_t modNum)
{
	// get module voltage
	return modules[modNum].alerts;
}

uint8_t batt_get_module_faults(uint8_t modNum)
{
	// get module voltage
	return modules[modNum].faults;
}

uint8_t batt_get_module_status(uint8_t modNum)
{
	// get module voltage
	return modules[modNum].status;
}

uint8_t batt_get_module_bal(uint8_t modNum)
{
	// get module voltage
	return modules[modNum].balState;
}

uint16_t batt_get_module_volt(uint8_t modNum)
{
	// get module voltage
	return modules[modNum].moduleVolt;
}

uint16_t batt_get_cell_volt(uint8_t modNum, uint8_t cellNum)
{
	// get cell voltage
	return modules[modNum].cellVolt[cellNum];
}

int16_t batt_get_temperature(uint8_t modNum, uint8_t tempNum)
{
	// get cell voltage
	return modules[modNum].temperatures[tempNum];
}



uint8_t batt_get_module_cnt()
{
	// get cell voltage
	return numFoundModules;
}


void set_comm_flt_all()
{
	// set COMM fault for all modules
	for (uint8_t mod = 0; mod < numFoundModules; mod++)
	{
		modules[mod].faults |= BATT_FAULT_NOCOMM;		// set comm fault flag
	}
}

void bms_state_machine()
{
	static uint8_t curr_module;
	uint16_t wait_ms_tmp;
	
	if (txStatus != COMM_STATUS_IDLE)
	{
		// Waiting for comms, nothing to do here
		return;
	}
	
	cli();
	wait_ms_tmp = wait_ms;
	sei();
	
	if (wait_ms_tmp > 0)
	{
		// wait active
		return;
	}
	
	
	
	switch (batt_state)
	{
		
		
			case BATT_STATE_IDLE:
				// check if any op is pending
				
				if (batt_init_pend)
				{
					batt_state = BATT_STATE_INIT;
				}
				else if (batt_clear_alarms_pend)
				{
					batt_state = BATT_STATE_RESET_ALARMS;	
				}
				else if (batt_balance_set_pend)
				{
					batt_state = BATT_STATE_SET_BALANCE;
				}
				else if (bat_datacollection_timer == 0)
				{
					bat_datacollection_timer = 100;
					batt_state = BATT_STATE_GETVALUES;					
				}
				

				
				break;
			case BATT_STATE_GETVALUES:
				if (numFoundModules == 0)
				{
					batt_state = BATT_STATE_IDLE;
				}
				else
				{
					curr_module = 1;
					// enable temperature inputs
					batt_state = BATT_STATE_GETVALUES_ENABLE_TEMP;
					batt_write_reg(BATT_ADDR_BROADCAST, BQ_REG_IO_CTRL, 0x03, 3, GUARD_TIME_MIN);					
				}
				break;
			case BATT_STATE_GETVALUES_ENABLE_TEMP:
				// check if operation was successful
				if (rxStatus == RX_STATUS_OK)
				{
					// OK
					// enable all ADC channels
					batt_state = BATT_STATE_GETVALUES_ENABLE_ADC;
					batt_write_reg(BATT_ADDR_BROADCAST, BQ_REG_ADC_CTRL, 0x3D, 3, 2 * GUARD_MSEC); // wait 2 ms for ADC startup
				}
				else
				{
					// failed
					// TODO: notify
					
					// set COMM fault for all modules
					set_comm_flt_all();
					
					batt_state = BATT_STATE_IDLE;
				}
				
				break;
			case BATT_STATE_GETVALUES_ENABLE_ADC:
				// check if operation was successful
				if (rxStatus == RX_STATUS_OK)
				{
					// OK
					// start ADC conversions (~600 us to finish)					
					batt_state = BATT_STATE_GETVALUES_START_ADC;
					batt_write_reg(BATT_ADDR_BROADCAST, BQ_REG_ADC_CONV, 0x01, 3, 1 * GUARD_MSEC);	// wait 1 ms for ADC
				}
				else
				{
					// failed
					// TODO: notify
					
					// set COMM fault for all modules
					set_comm_flt_all();
					
					batt_state = BATT_STATE_IDLE;
				}
				
				break;
			
			case BATT_STATE_GETVALUES_START_ADC:
				// check if operation was successful
				if (rxStatus == RX_STATUS_OK)
				{
					// OK
					// disable temp sensors
					batt_state = BATT_STATE_GETVALUES_DISABLE_TEMP;
					batt_write_reg(BATT_ADDR_BROADCAST, BQ_REG_IO_CTRL, 0x00, 3, GUARD_TIME_MIN);
				}
				else
				{
					// failed
					// TODO: notify
					
					// set COMM fault for all modules
					set_comm_flt_all();
					
					batt_state = BATT_STATE_IDLE;
				}
				
				break;
			case BATT_STATE_GETVALUES_DISABLE_TEMP:
				// check if operation was successful
				if (rxStatus == RX_STATUS_OK)
				{
					// OK
					// read data from first module (block 1)
					batt_state = BATT_STATE_GETVALUES_READ1;
					batt_read_reg(1, BQ_REG_DEV_STATUS, 19, 3, GUARD_TIME_MIN);
				}
				else
				{
					// failed
					// TODO: notify
					
					// set COMM fault for all modules
					set_comm_flt_all();
					
					batt_state = BATT_STATE_IDLE;
				}
				
				break;
			case BATT_STATE_GETVALUES_READ1:
				// check if operation was successful
				if (rxStatus == RX_STATUS_OK)
				{
					// OK, block 1 read successful
					// process data
					
					modules[curr_module-1].faults &= ~BATT_FAULT_NOCOMM;	// clear comm fault flag
								
					// check status
					modules[curr_module-1].status = rxBuf[3];
								
								
					modules[curr_module-1].retmoduleVolt = ((uint32_t)((uint16_t)rxBuf[4] * 256 + rxBuf[5]) * 33333) / 16383;	// Vbat in mV
								
					uint32_t totVolt = 0;
								
					for (uint8_t cell = 0; cell < 6; cell++)
					{
						totVolt += ((uint16_t)rxBuf[6 + (cell * 2)] * 256 + rxBuf[7 + (cell * 2)]);
									
						modules[curr_module-1].cellVolt[cell] = ((uint32_t)((uint16_t)rxBuf[6 + (cell * 2)] * 256 + rxBuf[7 + (cell * 2)]) * 6250) / 16383;	// Vcell in mV
					}
								
								
					//totVolt *= 6250;
					//totVolt /= 16383;
					//modules[i].moduleVolt = totVolt;
								
					modules[curr_module-1].moduleVolt = ((uint32_t)totVolt * 6250) / 16383;	// added voltages
								
								
					// temperature TS1
					// R_TS1 = ((TEMPERATURE1_H * 256 + TEMPERATURE1_L) + 2) / 33046
					// Rth(temp)  = (RB / Ratio) - (RB + Rt)
								
					//float res_ts1 = (1.78f / (((rxBuf[17] * 256 + rxBuf[18]) + 2) / 33046.0f) - 3.57f);	// calculate NTC resistance
					
					// temperature conversion code from https://github.com/collin80/TeslaBMS/blob/master/BMSModule.cpp used as starting point
					
					float ts1 = (1780.0f*33046.0f) / ((rxBuf[18] * 256 + rxBuf[19]) + 2) - (1780.0f * 2);	// calculate NTC resistance in ohms
					ts1 = 1.0f / (0.0007610373573f + (0.0002728524832f * logf(ts1)) + (powf(logf(ts1), 3) * 0.0000001022822735f));	// calculate temperature in Kelvin using the the Steinhart-Hart Thermistor Equation
					ts1 -= 273.15f;	// convert to Celsius
					ts1 *= 100.0f;	// scale to degC/100
					
					long resInt1 = lrintf(ts1);	// round to integer
					
					if (resInt1 < INT16_MIN)
					{
						resInt1 = INT16_MIN;
					}
					else if (resInt1 > INT16_MAX)
					{
						resInt1 = INT16_MAX;
					}
					
					modules[curr_module-1].temperatures[0] = resInt1;
					
					//modules[curr_module-1].temperatures[0] = ts1 * 10;	// convert to Celsius, resolution 0.1 deg
								
								
								
					// tempCalc =  1.0f / (0.0007610373573f + (0.0002728524832f * logf(tempTemp)) + (powf(logf(tempTemp), 3) * 0.0000001022822735f));
								
					//uint32_t res_ts1 = ((uint32_t)17800*33046) / ((rxBuf[17] * 256 + rxBuf[18]) + 2) - (17800 * 2);	// return resistance in 0.1 ohm resolution
								
								
								
								
					//temp_raw =
								
					//ratio_ts1 = ((rxBuf[18] * 256 + rxBuf[19]) + 2) / 33046;
								
								
								
								
					//res_ts1 = (1780/ratio_ts1)-(1780+1780);
								
								
					float ts2 = (1780.0f*33068.0f) / ((rxBuf[20] * 256 + rxBuf[21]) + 9) - (1780.0f * 2);	// calculate NTC resistance in ohms
					ts2 = 1.0f / (0.0007610373573f + (0.0002728524832f * logf(ts2)) + (powf(logf(ts2), 3) * 0.0000001022822735f));	// calculate temperature in Kelvin using the the Steinhart-Hart Thermistor Equation	
					ts2 -= 273.15f;	// convert to Celsius
					ts2 *= 100.0f;	// scale to degC/100
					
					long resInt2 = lrintf(ts2);	// round to integer
					
					if (resInt2 < INT16_MIN)
					{
						resInt2 = INT16_MIN;
					}
					else if (resInt2 > INT16_MAX)
					{
						resInt2 = INT16_MAX;
					}
					
					modules[curr_module-1].temperatures[1] = resInt2;		
							
							
							
							
								
					//modules[curr_module-1].temperatures[1] = (ts2 - 273.15f) * 10;	// convert to Celsius, resolution 0.1 deg			
					
				}
				else
				{
					// failed
					modules[curr_module-1].faults |= BATT_FAULT_NOCOMM;		// set comm fault flag
				}
				
				// read data (device alerts and faults)
				batt_state = BATT_STATE_GETVALUES_READ2;
				batt_read_reg(curr_module, BQ_REG_ALERT_STATUS, 2, 3, GUARD_TIME_MIN);				
				break;
			
			case BATT_STATE_GETVALUES_READ2:
				// check if operation was successful
				if (rxStatus == RX_STATUS_OK)
				{
					// OK
					modules[curr_module-1].alerts = rxBuf[3];
					modules[curr_module-1].faults = rxBuf[4];
				}
				else
				{
					// failed
					modules[curr_module-1].faults |= BATT_FAULT_NOCOMM;		// set comm fault flag
				}
				
				curr_module++;	// next module
					
				if (curr_module > numFoundModules)
				{
					// finished, all modules read
					// TODO: notify
						
					//set_wait_ms(500);
					data_readouts_done++;
						
					batt_state = BATT_STATE_IDLE;
				}
				else
				{
					// still data left to read
					// read data (block 1)
					batt_state = BATT_STATE_GETVALUES_READ1;
					batt_read_reg(curr_module, BQ_REG_DEV_STATUS, 19, 3, GUARD_TIME_MIN);
				}
				
				break;			
			case BATT_STATE_INIT:
				numFoundModules = 0;
				// send reset to all modules
				batt_state = BATT_STATE_INIT_RESET;
				
				// un-assign all addresses by broadcasting "set addr to 0" command
				batt_write_reg(BATT_ADDR_BROADCAST, BQ_REG_ADDR_CTRL, 0, 3, GUARD_TIME_MIN);	// set address to 0 on all modules
				//batt_write_reg(BATT_ADDR_BROADCAST, BQ_REG_RESET, 0xA5, 3, 1 * GUARD_MSEC);	// wait 1 ms for reset
				break;
			
			case BATT_STATE_INIT_RESET:
				// check if reset was successful
				if (rxStatus == RX_STATUS_OK)
				{
					// OK
					batt_state = BATT_STATE_INIT_CHECK_UNASSIGNED;
					batt_read_reg(0, BQ_REG_DEV_STATUS, 1, 3, GUARD_TIME_MIN);	// read device status				
				}
				else
				{
					// failed
					// TODO: notify
					batt_init_ok = false;
					batt_init_pend = false;
					batt_state = BATT_STATE_IDLE;
				}
				break;
			case BATT_STATE_INIT_CHECK_UNASSIGNED:
				if (rxStatus == RX_STATUS_OK)
				{
					// OK, unassigned module(s) present
					
					if (numFoundModules < MAX_MODULES)
					{
						// assign address
						batt_state = BATT_STATE_INIT_ASSIGN_ADDR;
						batt_write_reg(0, BQ_REG_ADDR_CTRL, (numFoundModules + 1) | 0x80, 0, GUARD_TIME_MIN);	// set address
					}
					else
					{
						// too many modules
						// TODO: notify
						if (numFoundModules == 0)
						{
							// error, no modules found
							batt_init_ok = false;
						}
						else
						{
							batt_init_ok = true;
						}
						
						batt_init_pend = false;
						batt_state = BATT_STATE_IDLE;
					}
				}
				else if (rxStatus == RX_STATUS_NOREPLY)
				{
					// OK, no unassigned module(s) present, we are done
					// TODO: notify
					batt_init_ok = true;
					batt_init_pend = false;
					batt_state = BATT_STATE_IDLE;
				}
				else
				{
					// failed
					// TODO: notify
					batt_init_ok = false;
					batt_init_pend = false;
					batt_state = BATT_STATE_IDLE;
				}
			
				break;
			case BATT_STATE_INIT_ASSIGN_ADDR:
				if (rxStatus == RX_STATUS_OK)
				{
					// OK, assignment successful
					// continue checking for unassigned devices
					numFoundModules++;
					batt_state = BATT_STATE_INIT_CHECK_UNASSIGNED;
					batt_read_reg(0, BQ_REG_DEV_STATUS, 1, 3, GUARD_TIME_MIN);	// read device status
				}
				else
				{
					// failed
					// TODO: notify
					batt_init_ok = false;
					batt_init_pend = false;
					batt_state = BATT_STATE_IDLE;
				}
				break;
			
			case BATT_STATE_INIT_TEST:
			case BATT_STATE_ENABLE_SLEEP:
			case BATT_STATE_DISABLE_SLEEP:
				break;
			case BATT_STATE_RESET_ALARMS:
				if (numFoundModules == 0)
				{
					batt_clear_alarms_ok = false;
					batt_clear_alarms_pend = false;
					batt_state = BATT_STATE_IDLE;
				}
				else
				{
					// reset alerts in all modules
					batt_write_reg(BATT_ADDR_BROADCAST, BQ_REG_ALERT_STATUS, batt_clear_alarms_alertbits, 3, GUARD_TIME_MIN);
					batt_state = BATT_STATE_RESET_ALARMS_2;
				}
				break;
			case BATT_STATE_RESET_ALARMS_2:
				if (rxStatus == RX_STATUS_OK)
				{
					// OK, reset bits to 0
					batt_write_reg(BATT_ADDR_BROADCAST, BQ_REG_ALERT_STATUS, 0, 3, GUARD_TIME_MIN);
					batt_state = BATT_STATE_RESET_ALARMS_3;					
				}
				else
				{
					// failed
					batt_clear_alarms_ok = false;
					batt_clear_alarms_pend = false;
					batt_state = BATT_STATE_IDLE;
				}
				break;
			case BATT_STATE_RESET_ALARMS_3:
				if (rxStatus == RX_STATUS_OK)
				{
					// OK, reset faults in all modules
					batt_write_reg(BATT_ADDR_BROADCAST, BQ_REG_FAULT_STATUS, batt_clear_alarms_faultbits, 3, GUARD_TIME_MIN);
					batt_state = BATT_STATE_RESET_ALARMS_4;
				}
				else
				{
					// failed
					batt_clear_alarms_ok = false;
					batt_clear_alarms_pend = false;
					batt_state = BATT_STATE_IDLE;
				}
				break;
			case BATT_STATE_RESET_ALARMS_4:
				if (rxStatus == RX_STATUS_OK)
				{
					// OK, reset bits to 0
					batt_write_reg(BATT_ADDR_BROADCAST, BQ_REG_FAULT_STATUS, 0, 3, GUARD_TIME_MIN);
					batt_state = BATT_STATE_RESET_ALARMS_5;
				}
				else
				{
					// failed
					batt_clear_alarms_ok = false;
					batt_clear_alarms_pend = false;
					batt_state = BATT_STATE_IDLE;
				}
				break;
			case BATT_STATE_RESET_ALARMS_5:
				if (rxStatus == RX_STATUS_OK)
				{
					// OK
					batt_clear_alarms_ok = true;
				}
				else
				{
					// failed
					batt_clear_alarms_ok = false;
				}
				batt_clear_alarms_pend = false;
				batt_state = BATT_STATE_IDLE;
				break;
			case BATT_STATE_SET_THRESHOLDS:
				break;
			case BATT_STATE_SET_BALANCE:
				
				
				if ((numFoundModules == 0) || (batt_balance_pack == 0) || (batt_balance_pack > numFoundModules))
				{
					batt_balance_set_ok = false;
					batt_balance_set_pend = false;
					batt_state = BATT_STATE_IDLE;
				}
				else
				{
					// set balance timer in selected module
					batt_write_reg(batt_balance_pack, BQ_REG_BAL_TIME, batt_balance_timeout, 3, GUARD_TIME_MIN);
					batt_state = BATT_STATE_SET_BALANCE_2;
				}
				break;
				
				
				
			case BATT_STATE_SET_BALANCE_2:
			
			
				if (rxStatus == RX_STATUS_OK)
				{
					// OK, reset balance outputs to 0
					batt_write_reg(batt_balance_pack, BQ_REG_BAL_CTRL, 0, 3, GUARD_TIME_MIN);
					batt_state = BATT_STATE_SET_BALANCE_3;
				}
				else
				{
					// failed
					batt_balance_set_ok = false;
					batt_balance_set_pend = false;
					batt_state = BATT_STATE_IDLE;
				}
				break;
			
					
				
			case BATT_STATE_SET_BALANCE_3:
				
				
				if (rxStatus == RX_STATUS_OK)
				{
					// OK, set balance outputs
					
					
					if (batt_balance_cells == 0)
					{
						// No cells to balance, we are done here
						modules[batt_balance_pack-1].balState = 0;
						batt_balance_set_ok = true;
						batt_balance_set_pend = false;
						batt_state = BATT_STATE_IDLE;
					}
					else
					{
						batt_write_reg(batt_balance_pack, BQ_REG_BAL_CTRL, batt_balance_cells, 3, GUARD_TIME_MIN);
						batt_state = BATT_STATE_SET_BALANCE_4;
					}
					
					
				}
				else
				{
					// failed
					batt_balance_set_ok = false;
					batt_balance_set_pend = false;
					batt_state = BATT_STATE_IDLE;
				}
				break;
				
			case BATT_STATE_SET_BALANCE_4:
			
				if (rxStatus == RX_STATUS_OK)
				{
					// OK
					modules[batt_balance_pack-1].balState = batt_balance_cells;
					batt_balance_set_ok = true;
				}
				else
				{
					// failed
					batt_balance_set_ok = false;
				}
				
				batt_balance_set_pend = false;
				batt_state = BATT_STATE_IDLE;
				break;
		
		
	}
	
}
