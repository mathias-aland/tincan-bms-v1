#include "clkctrl.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "i2c.h"

//#define I2C_BAUD(F_SCL, T_RISE)   ((((((float)(F_CPU) / (float)(F_SCL))) - 10 - ((float)(F_CPU) * (T_RISE) / 1000000))) / 2)
#define I2C_BAUD(freq, t_rise) ((F_CPU / freq) / 2) - (5 + (((F_CPU / 1000000) * t_rise) / 2000))

#define I2C_SEND_TX_ADDR(data)    TWI0.MADDR=(data)
#define I2C_SEND_TX_DATA(data)    TWI0.MDATA=(data)
#define I2C_GET_RX_DATA()         (TWI0.MDATA)
#define I2C_SEND_NACK()           TWI0.MCTRLB|=TWI_ACKACT_bm
#define I2C_SEND_ACK()            TWI0.MCTRLB&=~TWI_ACKACT_bm
#define I2C_SEND_STOP()           TWI0.MCTRLB|=TWI_MCMD_STOP_gc
#define I2C_CLEAR_INT_FLAGS()     TWI0.MSTATUS|=(TWI_RIF_bm|TWI_WIF_bm)
#define I2C_RESET_BUS()           do{TWI0.MCTRLA&=~TWI_ENABLE_bm;TWI0.MCTRLA|=TWI_ENABLE_bm;TWI0.MSTATUS|=TWI_BUSSTATE_IDLE_gc;}while(0)
#define I2C_IS_NACK()             (TWI0.MSTATUS&TWI_RXACK_bm)?true:false
#define I2C_IS_BUS_ERROR()        (TWI0.MSTATUS&TWI_BUSERR_bm)?true:false  
#define I2C_IS_DATA()             (TWI0.MDATA)?true:false
#define I2C_IS_ADDR()             (TWI0.MADDR)?true:false
#define I2C_IS_ARB_LOST()         (TWI0.MSTATUS&TWI_ARBLOST_bm)?true:false
#define I2C_IS_BUSY()             (internal_status.busy || !(TWI0.MSTATUS & TWI_BUSSTATE_IDLE_gc))?true:false

typedef enum
{
    I2C_STATE_IDLE = 0,
    I2C_STATE_TX,
    I2C_STATE_RX
} i2c_states_t;

typedef struct
{
    bool busy;
    uint8_t address;
    uint8_t *writePtr;
    size_t writeLength;
    uint8_t *readPtr;
    size_t readLength;
    bool switchToRead;
    i2c_error_t errorState; 
    i2c_states_t state;
} i2c_internal_t;


typedef struct 
{
	bool set_outputs_pend;
	bool contactor_enable_pend;
	
	uint8_t outputs_newstate;
	uint8_t outputs_newpwmfreq;
	uint8_t outputs_newduty1;
	uint8_t outputs_newduty2;
	
	uint8_t contactor_enable_newstate;
} i2c_pend_t;


i2c_pend_t i2c_pend = {.set_outputs_pend = false, .outputs_newstate = 0};


static volatile i2c_internal_t internal_status;




COMP_MCU_state_t comp_mcu_state;

uint8_t i2c_txBuf[sizeof(comp_mcu_state)+1];
uint8_t i2c_rxBuf[sizeof(comp_mcu_state)];

volatile static uint16_t wait_ms = 0;

void u0_printch(char ch)
{
	while (!(USART0.STATUS & USART_DREIF_bm));
	USART0.TXDATAL = ch;
}



ISR(TWI0_TWIM_vect)
{
	
	// Red LED on
	//PORTD.OUTSET = 0x80;
	

	//u0_printch('!');
	
	uint8_t m_status = TWI0.MSTATUS;
	
	
	if ((m_status & TWI_BUSERR_bm) || (m_status & TWI_ARBLOST_bm))
	{
		// bus error
		// should never happen as we only have one master on the bus
		// try to recover by forcing IDLE state followed by sending STOP
		// TODO: move to mainloop to avoid IRQ storm is case of stuck bus
		
		//printf("I2C BUS ERR!\n\r");
		//printf("TWI0.MSTATUS = 0x%02x\n\r", m_status);
		//printf("BUSY = 0x%02x\n\r", internal_status.busy);
		//printf("STATE = 0x%02x\n\r", internal_status.state);
		//printf("ERRSTATE = 0x%02x\n\r", internal_status.errorState);
		//printf("writeLength = %u\n\r", internal_status.writeLength);
		//printf("readLength = %u\n\r", internal_status.readLength);
		
		internal_status.errorState = I2C_ERROR_BUS_COLLISION;	
		TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc | TWI_BUSERR_bm | TWI_ARBLOST_bm;
		TWI0.MCTRLB = TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc;
		internal_status.busy = false;
		internal_status.state = I2C_STATE_IDLE;
		goto TWI_ISR_END;
	}
	else if (m_status & TWI_WIF_bm)
	{
		// RXACK is only valid if WIF is set but not BUSERR/ARBLOST
		
		if (m_status & TWI_RXACK_bm)
		{
			
			//printf("I2C NAK!\n\r");
			//printf("TWI0.MSTATUS = 0x%02x\n\r", m_status);
			//printf("BUSY = 0x%02x\n\r", internal_status.busy);
			//printf("STATE = 0x%02x\n\r", internal_status.state);
			//printf("ERRSTATE = 0x%02x\n\r", internal_status.errorState);
			//printf("writeLength = %u\n\r", internal_status.writeLength);
			//printf("readLength = %u\n\r", internal_status.readLength);
						
			
			if (TWI0.MADDR)
			{
				// address NACK
				internal_status.errorState = I2C_ERROR_ADDR_NACK;
			}
			else
			{
				// data NACK
				internal_status.errorState = I2C_ERROR_DATA_NACK;
			}
			

			//u0_printch('N');
			
			// Client sent NACK, abort
			TWI0.MCTRLB = TWI_MCMD_STOP_gc;	// Send STOP
			internal_status.busy = false;
			internal_status.state = I2C_STATE_IDLE;
			goto TWI_ISR_END;
		}
		else
		{
			// client sent ACK, continue sending data
			
			if (internal_status.writeLength)
			{
				//u0_printch('T');
				internal_status.writeLength--;
				TWI0.MDATA=*internal_status.writePtr++;
				internal_status.state = I2C_STATE_TX;
			}
			else
			{
				if (internal_status.switchToRead)
				{
					//u0_printch('r');
					internal_status.switchToRead = false;
					TWI0.MADDR = (internal_status.address << 1 | 1);	// Send address (R)
					internal_status.state = I2C_STATE_RX;
				}
				else
				{
					//u0_printch('s');
					TWI0.MCTRLB = TWI_MCMD_STOP_gc;	// Send STOP
					internal_status.busy = false;
					internal_status.state = I2C_STATE_IDLE;
				}
			}
			
			
			//u0_printch('A');
		}
				
	}
	else if (m_status & TWI_RIF_bm)
	{
		//u0_printch('R');
		if (internal_status.readLength > 0)
		{
			*internal_status.readPtr++ = TWI0.MDATA;
			internal_status.readLength--;
		}
			
		if (internal_status.readLength == 0)
		{
			/* Last byte, send NACK followed by STOP condition */
			//u0_printch('S');
			TWI0.MCTRLB = TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc;
			internal_status.busy = false;
			internal_status.state = I2C_STATE_IDLE;
		}
		else
		{
			//u0_printch('n');
			/* More bytes to receive, send ACK and read next byte */
			TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;
			internal_status.state = I2C_STATE_RX;
		}
	}
	else
	{
		//printf("I2C UNKNOWN IRQ!\n\r");
		//printf("TWI0.MSTATUS = 0x%02x\n\r", m_status);
		//printf("BUSY = 0x%02x\n\r", internal_status.busy);
		//printf("STATE = 0x%02x\n\r", internal_status.state);
		//printf("ERRSTATE = 0x%02x\n\r", internal_status.errorState);
		//printf("writeLength = %u\n\r", internal_status.writeLength);
		//printf("readLength = %u\n\r", internal_status.readLength);
		
		// IRQ fired but no flags set ???		
		goto TWI_ISR_END;
	}
	
	// Red LED off
TWI_ISR_END:
	PORTD.OUTCLR = 0x80;
	
}

void    I2C_Init(void)
{
	
    /* reset internal status structure */
    memset((void*)&internal_status, 0, sizeof(internal_status));
	
    /* Enable Pull-ups */
    PORTC.PIN2CTRL |= PORT_PULLUPEN_bm;
    PORTC.PIN3CTRL |= PORT_PULLUPEN_bm;
	
	
	
	// check for stuck SDA

    /* FMPEN OFF; INPUTLVL I2C; SDAHOLD OFF; SDASETUP 4CYC */
    TWI0.CTRLA = 0x0;
    
    /* Debug Run */
    TWI0.DBGCTRL = 0x0;
    
    /* Host Baud Rate Control */
    //TWI0.MBAUD = (uint8_t)I2C_BAUD(400000, 100);	// 400 kHz (385 kHz measured)
	TWI0.MBAUD = (uint8_t)I2C_BAUD(100000, 100);	// 100 kHz (99 kHz measured)
    
    /* ENABLE enabled; QCEN disabled; RIEN false; SMEN disabled; TIMEOUT DISABLED; WIEN false */
    TWI0.MCTRLA = TWI_ENABLE_bm;
    	
	// clear all flags and enter IDLE state
	TWI0.MSTATUS = TWI_RIF_bm | TWI_WIF_bm | TWI_CLKHOLD_bm | TWI_ARBLOST_bm | TWI_BUSERR_bm | TWI_BUSSTATE_IDLE_gc;

	// Enable RIF/WIF
	TWI0.MCTRLA = TWI_ENABLE_bm | TWI_RIEN_bm | TWI_WIEN_bm;
	
	
}

i2c_error_t I2C_WriteRead(uint8_t address, uint8_t *pData_w, size_t dataLength_w, uint8_t *pData_r, size_t dataLength_r)
{
	//i2c_error_t retErrorState;
	if (!I2C_IS_BUSY())
	{
		internal_status.busy = true;
		internal_status.address = address;
		internal_status.switchToRead = true;
		internal_status.writePtr = pData_w;
		internal_status.writeLength = dataLength_w;
		internal_status.readPtr = pData_r;
		internal_status.readLength = dataLength_r;
		internal_status.errorState = I2C_ERROR_NONE;
		internal_status.state = I2C_STATE_TX;
		TWI0.MADDR = (internal_status.address << 1);	// Send address (W)
		
		
		
		// retErrorState = internal_status.errorState;
		// internal_status.errorState = I2C_ERROR_NONE;
		return I2C_ERROR_NONE;
	}
	else
	return I2C_ERROR_BUSY;
}

i2c_error_t I2C_Write(uint8_t address, uint8_t *pData, size_t dataLength)
{
    //i2c_error_t retErrorState;
    if (!I2C_IS_BUSY())
    {
        internal_status.busy = true;
        internal_status.address = address;
        internal_status.switchToRead = false;
        internal_status.writePtr = pData;
        internal_status.writeLength = dataLength;
        internal_status.readPtr = NULL;
        internal_status.readLength = 0;
        internal_status.errorState = I2C_ERROR_NONE;
        internal_status.state = I2C_STATE_TX;
		TWI0.MADDR = (internal_status.address << 1);	// Send address (W)
		
		
		
       // retErrorState = internal_status.errorState;
       // internal_status.errorState = I2C_ERROR_NONE;
        return I2C_ERROR_NONE;
    }
    else
        return I2C_ERROR_BUSY;
}

i2c_error_t I2C_Read(uint8_t address, uint8_t *pData, size_t dataLength)
{
    //i2c_error_t retErrorState;
    if (!I2C_IS_BUSY())
    {
        internal_status.busy = true;
        internal_status.address = address;
        internal_status.switchToRead = false;
        internal_status.readPtr = pData;
        internal_status.readLength = dataLength;
        internal_status.writePtr = NULL;
        internal_status.writeLength = 0;
        internal_status.errorState = I2C_ERROR_NONE;
        internal_status.state = I2C_STATE_RX;		
		TWI0.MADDR = (internal_status.address << 1 | 1);	// Send address (R)		

        //retErrorState = internal_status.errorState;
        //internal_status.errorState = I2C_ERROR_NONE;
        return I2C_ERROR_NONE;
    }
    else
        return I2C_ERROR_BUSY;
}

void I2C_systick_cb()
{
	// this will be called from systick ISR every 1 ms. Check timeouts etc:
	
		// no need for atomic block as this is run in ISR context
		// only count when comms are idle
		//if ((internal_status.state == I2C_STATE_IDLE) && (wait_ms > 0))
		//{
			//// wait active
			//wait_ms--;
		//}
		
		// no need for atomic block as this is run in ISR context
		// only count when comms are idle
		if (wait_ms > 0)
		{
			// wait active
			wait_ms--;
		}
	
}

typedef enum
{
	I2C_OP_INITBUS = 0,
	I2C_OP_IDLE,
	I2C_OP_READ_COMP_MCU_1,
	I2C_OP_READ_COMP_MCU_2,
	I2C_OP_REFRESH_WATCHDOG,
	I2C_OP_REFRESH_WATCHDOG_DONE,
	I2C_OP_SET_OUTPUTS,
	I2C_OP_SET_OUTPUTS_DONE,
	I2C_OP_SET_CONTACTOR_ENABLE,
	I2C_OP_SET_CONTACTOR_ENABLE_DONE,
} I2C_OP_t;


I2C_OP_t i2c__op = I2C_OP_INITBUS;


static void set_wait_ms(uint16_t ms)
{
	cli();
	wait_ms = ms;
	sei();
}


static uint16_t get_wait_ms()
{
	uint16_t retval;
	cli();
	retval = wait_ms;
	sei();
	
	return retval;
}


bool set_outputs(uint8_t newState, uint8_t pwmFreq, uint8_t pwmDuty1, uint8_t pwmDuty2)
{
	if (i2c_pend.set_outputs_pend)
		return false;	// already pending
	
	i2c_pend.outputs_newstate = newState;
	i2c_pend.outputs_newpwmfreq = pwmFreq;
	i2c_pend.outputs_newduty1 = pwmDuty1;
	i2c_pend.outputs_newduty2 = pwmDuty2;
	
	i2c_pend.set_outputs_pend = true;
	return true;
}

bool contactor_enable(uint8_t val)
{
	if (i2c_pend.contactor_enable_pend)
		return false;	// already pending
		
	i2c_pend.contactor_enable_newstate = val;
	i2c_pend.contactor_enable_pend = true;
	return true;
}


uint8_t comp_get_cont_state()
{
	return comp_mcu_state.cont_state;
}


// GPIO number between 1 and 8
uint16_t comp_get_gpi(uint8_t gpio_num)
{
	if ((gpio_num < 1) || (gpio_num > 8))
	{
		return 0;
	}
	
	
	return comp_mcu_state.inp_v[gpio_num-1];
	
}


void i2c_loop()
{
	i2c_error_t i2c_ret;
	//static uint8_t comp_mcu_data_idx = 0;
	//printf("TWI0.MSTATUS = 0x%02x\n\r", TWI0.MSTATUS);
	//printf("BUSY = 0x%02x\n\r", internal_status.busy);
	//printf("STATE = 0x%02x\n\r", internal_status.state);
	//printf("ERRSTATE = 0x%02x\n\r", internal_status.errorState);
	//printf("WAIT_MS = %u\n\r", get_wait_ms());

	if (i2c__op == I2C_OP_INITBUS)
	{
		// init I2C bus
		
		TWI0.MCTRLA = 0;	// disable TWI host
		PORTC.OUTCLR = PIN2_bm | PIN3_bm;	// OUT (SDA/SCL) = 0		
		
		// check for SCL high
		if ((PORTC.IN & PIN3_bm) == 0)
		{
			// SCL still low, wait
			return;
		}
		
		_delay_us(10);	// enforce minimum SCL high time
		
		// check for stuck SDA
		if ((PORTC.IN & PIN2_bm) == 0)
		{
			// SDA still low, send clock pulse
			PORTC.DIRSET = PIN3_bm;	// pull SCL low
			_delay_us(10);
			PORTC.DIRCLR = PIN3_bm;	// release SCL
			return;	// don't take up too much CPU time, let other processes run and continue next cycle
		}
		
		// SDA high, send START followed by STOP (NOTE: technically not allowed by I2C spec)
		PORTC.DIRSET = PIN2_bm;	// pull SDA low (START condition)
		_delay_us(10);
		PORTC.DIRCLR = PIN2_bm;	// release SDA (STOP condition)
		_delay_us(10);
		
		// bus should now be in idle state
		
		/* reset internal status structure */
		memset((void*)&internal_status, 0, sizeof(internal_status));

		/* FMPEN OFF; INPUTLVL I2C; SDAHOLD OFF; SDASETUP 4CYC */
		TWI0.CTRLA = 0x0;
    
		/* Debug Run */
		TWI0.DBGCTRL = 0x0;
    
		/* Host Baud Rate Control */
		//TWI0.MBAUD = (uint8_t)I2C_BAUD(400000, 100);	// 400 kHz (385 kHz measured)
		TWI0.MBAUD = (uint8_t)I2C_BAUD(100000, 100);	// 100 kHz (99 kHz measured)
    
		/* ENABLE enabled; QCEN disabled; RIEN false; SMEN disabled; TIMEOUT DISABLED; WIEN false */
		TWI0.MCTRLA = TWI_ENABLE_bm;
    
		// clear all flags and enter IDLE state
		TWI0.MSTATUS = TWI_RIF_bm | TWI_WIF_bm | TWI_CLKHOLD_bm | TWI_ARBLOST_bm | TWI_BUSERR_bm | TWI_BUSSTATE_IDLE_gc;

		// Enable RIF/WIF
		TWI0.MCTRLA = TWI_ENABLE_bm | TWI_RIEN_bm | TWI_WIEN_bm;
		
		i2c__op = I2C_OP_IDLE;
	}
	
	if (internal_status.state != I2C_STATE_IDLE)
	{
		// i2c busy
		return;
	}	
	
	if (i2c__op == I2C_OP_IDLE)
	{
		
		// check for any pending operations
		if (i2c_pend.set_outputs_pend)
		{
			// set outputs pending
			i2c__op = I2C_OP_SET_OUTPUTS;
		}
		else if (i2c_pend.contactor_enable_pend)
		{
			// set contactor enable pending
			i2c__op = I2C_OP_SET_CONTACTOR_ENABLE;
		}
		else
		{
			// no operations pending, resume data reading
			
			if (get_wait_ms() == 0)
			{
				i2c__op = I2C_OP_READ_COMP_MCU_1;	
			}	
		}
	}
	else if (i2c__op == I2C_OP_SET_OUTPUTS)
	{
		i2c_txBuf[0] = offsetof(COMP_MCU_state_t, outp_state);	// write to outp_state
		i2c_txBuf[1] = i2c_pend.outputs_newstate;	// outputs state
		i2c_txBuf[2] = i2c_pend.outputs_newpwmfreq;	// PWM frequency
		i2c_txBuf[3] = i2c_pend.outputs_newduty1;	// PWM1 duty
		i2c_txBuf[4] = i2c_pend.outputs_newduty2;	// PWM2 duty
		
		i2c_ret = I2C_Write(0x10, i2c_txBuf, 5);
	
		//i2c_txBuf[0] = i2c_comp_mcu_data_table[comp_mcu_data_idx].comp_mcu_reg;
		//i2c_ret = I2C_WriteRead(0x10, i2c_txBuf, 1, i2c_rxBuf, i2c_comp_mcu_data_table[comp_mcu_data_idx].var_len);
	
		if(i2c_ret == I2C_ERROR_NONE)
		{
			// OK
			i2c__op = I2C_OP_SET_OUTPUTS_DONE;
		}
		else
		{
			// error
			i2c__op = I2C_OP_INITBUS;	// reset bus
			//set_wait_ms(500);	// DEBUG
		}
	
	}
	else if (i2c__op == I2C_OP_SET_OUTPUTS_DONE)
	{
		// check if successful
		
		if (internal_status.errorState != I2C_ERROR_NONE)
		{
			// error
			i2c__op = I2C_OP_INITBUS;	// reset bus
			//set_wait_ms(500);	// DEBUG
		}
		else
		{
			// OK
			i2c__op = I2C_OP_IDLE;
			i2c_pend.set_outputs_pend = false;
		}
		
	}
	else if (i2c__op == I2C_OP_SET_CONTACTOR_ENABLE)
	{
		i2c_txBuf[0] = offsetof(COMP_MCU_state_t, cont_enable);	// write to cont_enable
		i2c_txBuf[1] = i2c_pend.contactor_enable_newstate;	// cont_enable
		
		
		i2c_ret = I2C_Write(0x10, i2c_txBuf, 2);
	
		//i2c_txBuf[0] = i2c_comp_mcu_data_table[comp_mcu_data_idx].comp_mcu_reg;
		//i2c_ret = I2C_WriteRead(0x10, i2c_txBuf, 1, i2c_rxBuf, i2c_comp_mcu_data_table[comp_mcu_data_idx].var_len);
	
		if(i2c_ret == I2C_ERROR_NONE)
		{
			// OK
			i2c__op = I2C_OP_SET_CONTACTOR_ENABLE_DONE;
		}
		else
		{
			// error
			i2c__op = I2C_OP_INITBUS;	// reset bus
			//set_wait_ms(500);	// DEBUG
			//set_wait_ms(100);
		}
	
	}
	else if (i2c__op == I2C_OP_SET_CONTACTOR_ENABLE_DONE)
	{
		// check if successful
		
		if (internal_status.errorState != I2C_ERROR_NONE)
		{
			// error
			i2c__op = I2C_OP_INITBUS;	// reset bus
			//set_wait_ms(500);	// DEBUG
			//set_wait_ms(100);
		}
		else
		{
			// OK
			i2c__op = I2C_OP_IDLE;
			i2c_pend.contactor_enable_pend = false;
		}
		
	}	
	else if (i2c__op == I2C_OP_READ_COMP_MCU_1)
	{
		i2c_txBuf[0] = 0;	// start reading from address 0
		i2c_ret = I2C_WriteRead(0x10, i2c_txBuf, 1, i2c_rxBuf, sizeof(comp_mcu_state));
		
		//i2c_txBuf[0] = i2c_comp_mcu_data_table[comp_mcu_data_idx].comp_mcu_reg;
		//i2c_ret = I2C_WriteRead(0x10, i2c_txBuf, 1, i2c_rxBuf, i2c_comp_mcu_data_table[comp_mcu_data_idx].var_len);
		
		if(i2c_ret == I2C_ERROR_NONE)
		{
			// OK
			i2c__op = I2C_OP_READ_COMP_MCU_2;
		}
		else
		{
			// error
			i2c__op = I2C_OP_INITBUS;	// reset bus
			//set_wait_ms(10);
			//set_wait_ms(500);	// DEBUG
		}
		
	}
	else if (i2c__op == I2C_OP_READ_COMP_MCU_2)
	{
		// check if successful
			
		if (internal_status.errorState != I2C_ERROR_NONE)
		{
			// error
			i2c__op = I2C_OP_INITBUS;	// reset bus
			//set_wait_ms(10);
			//set_wait_ms(500);	// DEBUG
		}
		else
		{
			// OK
			memcpy(&comp_mcu_state, i2c_rxBuf, sizeof(comp_mcu_state));
			
			//memcpy(i2c_comp_mcu_data_table[comp_mcu_data_idx].pVar, i2c_rxBuf, i2c_comp_mcu_data_table[comp_mcu_data_idx].var_len);
			//comp_mcu_data_idx++;
			
			//if (comp_mcu_data_idx < (sizeof(i2c_comp_mcu_data_table) / sizeof(i2c_comp_mcu_data_table[0])))
			//{
				// still values to read
			//	i2c__op = I2C_OP_READ_COMP_MCU_1;
			//}
			//else
			//{
				// done
			//	comp_mcu_data_idx = 0;
				i2c__op = I2C_OP_REFRESH_WATCHDOG;
			//}
		}
	}
	
	else if (i2c__op == I2C_OP_REFRESH_WATCHDOG)
	{
		i2c_txBuf[0] = offsetof(COMP_MCU_state_t, watchdog_cnt);	// write to watchdog_cnt
		i2c_txBuf[1] = comp_mcu_state.watchdog_cnt + 1;	// watchdog_cnt
		
		
		i2c_ret = I2C_Write(0x10, i2c_txBuf, 2);
		
		if(i2c_ret == I2C_ERROR_NONE)
		{
			// OK
			i2c__op = I2C_OP_REFRESH_WATCHDOG_DONE;
		}
		else
		{
			// error
			i2c__op = I2C_OP_INITBUS;	// reset bus
			//set_wait_ms(10);
			//set_wait_ms(500);	// DEBUG
		}
	}
	else if (i2c__op == I2C_OP_REFRESH_WATCHDOG_DONE)
	{
		// check if successful
		
		if (internal_status.errorState != I2C_ERROR_NONE)
		{
			// error
			i2c__op = I2C_OP_INITBUS;	// reset bus
			//set_wait_ms(10);
			//set_wait_ms(500);	// DEBUG
		}
		else
		{
			// OK
			i2c__op = I2C_OP_IDLE;
			set_wait_ms(100);	// wait 100ms before next readout
			//set_wait_ms(500);	// DEBUG
		}
	}
}



