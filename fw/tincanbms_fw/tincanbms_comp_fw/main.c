/*
 * tincanbms_comp_fw.c
 *
 * Created: 26-10-2022 17:06:08
 * Author : Mathias
 */ 

#define F_CPU 20000000UL

#include <avr/io.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <util/crc16.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>


#include "cal.h"

FUSES = {
	.WDTCFG = PERIOD_OFF_gc | WINDOW_OFF_gc,
	.BODCFG = LVL_BODLEVEL3_gc | BOD_SLEEP_SAMPLED_gc | BOD_SAMPFREQ_128HZ_gc | BOD_ACTIVE_ENWAKE_gc,
	.OSCCFG = CLKSEL_OSCHF_gc,
	.SYSCFG0 = RSTPINCFG_RST_gc | UPDIPINCFG_UPDI_gc | CRCSEL_CRC16_gc | CRCSRC_NOCRC_gc,
	.SYSCFG1 = SUT_0MS_gc | MVSYSCFG_SINGLE_gc,
	.CODESIZE = 0x00,
	.BOOTSIZE = 0x00
};

LOCKBITS = LOCK_KEY_NOLOCK_gc;


#define COMM_TIMEOUT	(1000 / 5)	// 1000 ms
#define FAULT_TIMEOUT	(100 / 5)	// 100 ms

#define SYS_STATUS_SLEEP_BIT	7

#define OUT_STATE_OFF		0
#define OUT_STATE_ON		1
#define OUT_STATE_PWM		2
#define OUT_STATE_PWM_INV	3

#define CAL_ENABLE_KEY	0xC5
#define CAL_SAVE_KEY	0x6D

typedef enum 
{
	I2C_STATE_IDLE = 0,
	I2C_STATE_REGADDR,
	I2C_STATE_REGWRITE
	
} I2C_STATE_t;


typedef enum
{
	CONT_STATE_OFF = 0,
	CONT_STATE_ON,
	CONT_STATE_FAULT_HW,
	CONT_STATE_FAULT_ESTOP,
	CONT_STATE_FAULT_TIMEOUT,
	CONT_STATE_FAULT_PINLVL
} CONT_STATE_t;


//
//typedef enum
//{
	//ADC_IDX_IN1 = 0,
	//ADC_IDX_IN2,
	//ADC_IDX_IN3,
	//ADC_IDX_IN4,
	//ADC_IDX_IN5,
	//ADC_IDX_IN6,
	//ADC_IDX_IN7,
	//ADC_IDX_IN8,
	//ADC_IDX_V_SUPPLY,
	//ADC_IDX_CONT_MON,
	//ADC_IDX_ESTOP_MON,
	//ADC_IDX_CURR_REF,
	//ADC_IDX_CURR_DIAG,
	//ADC_IDX_CURR_SENSE,
	//ADC_IDX_VDD
//} ADC_IDX_t;


typedef struct 
{
	uint8_t adc_map_neg;	
	uint8_t adc_map_pos;
	uint8_t adc_map_ref;
	uint8_t adc_map_samplen;
} adc_map_t;


const __flash adc_map_t ADC_CH_MAPPING[] =
{
	{0, ADC_MUXPOS_VDDDIV10_gc, VREF_REFSEL_2V048_gc, 8},	// ADC_IDX_VDD, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN30_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_IN1, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN29_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_IN2, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN28_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_IN3, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN27_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_IN4, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN26_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_IN5, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN25_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_IN6, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN24_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_IN7, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN23_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_IN8, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN17_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_V_SUPPLY, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN18_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_CONT_MON, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN20_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_ESTOP_MON, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN16_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_CURR_REF, 2+8=10 us sample time
	{0, ADC_MUXPOS_AIN7_gc, VREF_REFSEL_4V096_gc, 8},		// ADC_IDX_CURR_DIAG, 2+8=10 us sample time
	{ADC_MUXNEG_AIN16_gc, ADC_MUXPOS_AIN7_gc, VREF_REFSEL_2V048_gc, 8}		// ADC_IDX_CURR_SENSE, 2+8=10 us sample time
};

//const uint8_t ADC_CH_MAPPING[] = {
	//ADC_MUXPOS_VDDDIV10_gc,
	//ADC_MUXPOS_AIN30_gc,
	//ADC_MUXPOS_AIN29_gc,
	//ADC_MUXPOS_AIN28_gc,
	//ADC_MUXPOS_AIN27_gc,
	//ADC_MUXPOS_AIN26_gc,
	//ADC_MUXPOS_AIN25_gc,
	//ADC_MUXPOS_AIN24_gc,
	//ADC_MUXPOS_AIN23_gc,
	//ADC_MUXPOS_AIN17_gc,
	//ADC_MUXPOS_AIN18_gc,
	//ADC_MUXPOS_AIN20_gc,
	//ADC_MUXPOS_AIN16_gc,
	//ADC_MUXPOS_AIN7_gc,
	//ADC_MUXPOS_AIN7_gc,
//};



typedef struct
{
	uint8_t protocol_ver;
	uint8_t sys_status;
	uint8_t sys_alarms;		
	uint16_t dac_val;
	int8_t mcu_temp;
	uint16_t v_supply;
	uint16_t vdd;
	int16_t i_sense;
	uint16_t cont_mon;
	uint16_t i_ref;
	uint16_t i_diag;
	uint16_t estop_mon;
	uint8_t inp_state;
	uint16_t inp_v[8];
	uint8_t inp_lo_threshold[8];
	uint8_t inp_hi_threshold[8];	
	uint8_t outp_state;
	uint8_t outp_pwm_freq;		// 0-255 -> 1-256 Hz
	uint8_t outp_pwm_duty[2];	// 0-255 -> 0-100%
	uint8_t cont_state;
	uint8_t cont_enable;
	uint8_t watchdog_cnt;
	uint8_t cal_enable;	
	cal_entry_t cal_data[15];
} params_t;


volatile params_t params = 
{
	.protocol_ver = 1
};

const __flash params_t params_write_mask = 
{
	.protocol_ver = 0x00,
	.sys_status = 0x80,
	.sys_alarms = 0x01,
	.dac_val = 0x03ff,
	.mcu_temp = 0,
	.v_supply = 0,
	.vdd = 0,
	.i_sense = 0,
	.cont_mon = 0,
	.i_ref = 0,
	.i_diag = 0,
	.estop_mon = 0,
	.inp_state = 0,
	.inp_v = {[0 ... 7] = 0},
	.inp_lo_threshold = {[0 ... 7] = 0xff},
	.inp_hi_threshold = {[0 ... 7] = 0xff},
	.outp_state = 0xff,
	.outp_pwm_freq = 0xff,
	.outp_pwm_duty = {[0 ... 1] = 0xff},
	.cont_state = 0,
	.cont_enable = 0xff,
	.watchdog_cnt = 0xff,
	.cal_enable = 0xff,
	.cal_data = {[0 ... 14] = {.offset = 0xffff, .fullscale = 0xffff}}
};


//const EEMEM cal_eepage_t eeprom_data;



volatile uint8_t fault_timeout = 0;

volatile uint8_t watchdog_timeout = 0;
volatile uint8_t watchdog_cnt_last = 0;
volatile uint8_t cont_enable_last = 0;
//volatile uint8_t cont_state = CONT_STATE_OFF;

//uint16_t adc_res[13];

//int16_t curr_res;
//uint16_t vdd_res;
//int16_t temperature_in_C;

volatile bool i2c_active = false;

//volatile bool eewrite_pend = false;

uint8_t i2c_val_buf[sizeof(params)];
uint8_t i2c_upd_buf[sizeof(params)];

//volatile bool reg_upd_pend = false;
volatile uint8_t reg_upd_startIdx = 0;
volatile uint8_t reg_upd_idx = 0;
volatile uint8_t reg_upd_numBytes= 0;

#define I2C_REG_SYS_STATUS	0x01

#define I2C_SUPPLY_VOLTAGE	0x10
#define I2C_VDD				0x11
#define I2C_CURRENT			0x12
#define I2C_CONT_MON		0x13
#define I2C_MCU_TEMP		0x14
#define I2C_CURR_REF		0x15
#define I2C_CURR_DIAG		0x16
#define I2C_ESTOP_MON		0x17

#define I2C_IN1_V			0x20
#define I2C_IN2_V			0x21
#define I2C_IN3_V			0x22
#define I2C_IN4_V			0x23
#define I2C_IN5_V			0x24
#define I2C_IN6_V			0x25
#define I2C_IN7_V			0x26
#define I2C_IN8_V			0x27







void initCal()
{

	
	
	// read EEPROM
	uint8_t ee_ver = eeprom_read_byte((void*)0x00);
	uint8_t ee_cnt = eeprom_read_byte((void*)0x01);
		
	uint16_t crc16 = 0xffff;
				
	crc16 = _crc_xmodem_update(crc16, ee_ver);
	crc16 = _crc_xmodem_update(crc16, ee_cnt);
				
	uint8_t len = sizeof(params.cal_data);
		
	eeprom_read_block(params.cal_data, (void*)0x02, len);
				
	for (uint8_t i = 0; i < len; i++)
	{
		crc16 = _crc_xmodem_update(crc16, ((uint8_t*)params.cal_data)[i]);
	}
				 
	uint16_t ee_crc16 = eeprom_read_word((void*)(len + 2));

	if ((ee_ver != 0x01) || (ee_crc16 != crc16))
	{
		// failure, load defaults
		
		for (uint8_t i=0; i < 8; i++)
		{
			params.cal_data[ADC_IDX_IN1+i].fullscale = 20480;
			params.cal_data[ADC_IDX_IN1+i].offset = 0;
		}
	
		params.cal_data[ADC_IDX_V_SUPPLY].fullscale = 20480;
		params.cal_data[ADC_IDX_V_SUPPLY].offset = 0;
	
		params.cal_data[ADC_IDX_CONT_MON].fullscale = 20480;
		params.cal_data[ADC_IDX_CONT_MON].offset = 0;
	
		params.cal_data[ADC_IDX_ESTOP_MON].fullscale = 20480;
		params.cal_data[ADC_IDX_ESTOP_MON].offset = 0;
	
		params.cal_data[ADC_IDX_CURR_REF].fullscale = 4096;
		params.cal_data[ADC_IDX_CURR_REF].offset = 0;
	
		params.cal_data[ADC_IDX_CURR_DIAG].fullscale = 4096;
		params.cal_data[ADC_IDX_CURR_DIAG].offset = 0;
	
		params.cal_data[ADC_IDX_CURR_SENSE].fullscale = 9830;	// A/10
		params.cal_data[ADC_IDX_CURR_SENSE].offset = 14;
	
		params.cal_data[ADC_IDX_VDD].fullscale = 20480;
		params.cal_data[ADC_IDX_VDD].offset = 0;		
		
	}
	
	
}




void update_params()
{
					// update data
				
				uint8_t startIdx_tmp = reg_upd_startIdx;
				uint8_t idx_tmp = reg_upd_idx;
							
				if ((startIdx_tmp < idx_tmp) && (idx_tmp <= sizeof(i2c_upd_buf)))
				{
					// OK
					
					uint8_t *pParams = (uint8_t*)&params;
					pParams += startIdx_tmp;
					
					const __flash uint8_t *pParams_mask = (const __flash uint8_t*)&params_write_mask;
					pParams_mask += startIdx_tmp;
					
					uint8_t *pI2C_buf = i2c_upd_buf;
					pI2C_buf += startIdx_tmp;
					
					bool calsave_pend = false;
					
					while (startIdx_tmp < idx_tmp)
					{
						
						
						if (pI2C_buf == i2c_upd_buf + offsetof(params_t, cal_enable))
						{
							// process cal enable
							if (*pI2C_buf == CAL_ENABLE_KEY)
							{
								// cal enable requested
								if (params.cal_enable != CAL_SAVE_KEY)
								{
									// save not pending, enable cal
									params.cal_enable = CAL_ENABLE_KEY;
								}
							}
							else if (*pI2C_buf == CAL_SAVE_KEY)
							{
								// cal save requested, check if cal was enabled
								if (params.cal_enable == CAL_ENABLE_KEY)
								{
									// cal was enabled, proceed with EE save
									// defer trigger until later to allow for cal data update
									calsave_pend = true;
								}
							}
							else
							{
								// disable cal
								params.cal_enable = 0;
							}
						}
						// check if writing to cal region
						else if ((pI2C_buf < i2c_upd_buf + offsetof(params_t, cal_data)) || (params.cal_enable == CAL_ENABLE_KEY))
						{
							// not updating cal values or cal mode is enabled -> proceed with update
							*pParams = (*pParams & ~*pParams_mask) | (*pI2C_buf & *pParams_mask);	// apply write mask and update parameter byte	
						}
						
						pParams++;
						pParams_mask++;
						pI2C_buf++;
						startIdx_tmp++;
					}	
					
					
					// check if cal data should be saved to EEPROM
					if (calsave_pend)
					{
						// yes
						params.cal_enable = CAL_SAVE_KEY;
					}
					
					
					// update outputs
					
					
					
					uint8_t outState_tmp = params.outp_state;					
					uint16_t newPER = params.outp_pwm_freq;
					newPER++;
					
					uint32_t tca_clk;
					uint8_t tca_ctrla;
					
					// set PWM period
					if (params.outp_pwm_freq < 4)
					{
						// 1-4 Hz, set prescaler to 1024
						// frequency error will be < 0.02 % for values of 1-4 Hz (plus oscillator tolerance)
						tca_ctrla = TCA_SINGLE_CLKSEL_DIV1024_gc | TCA_SINGLE_ENABLE_bm;
						tca_clk = (F_CPU/1024);
					}
					else
					{
						// >4 Hz, set prescaler to 64
						// frequency error will be < 0.1 % for values of 4-256 Hz (plus oscillator tolerance)
						tca_ctrla = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
						tca_clk = (F_CPU/64);
					}
					
					
					newPER = (tca_clk / newPER) - 1;
					
					uint16_t tca_cmp2 = ((uint32_t)params.outp_pwm_duty[0] * newPER) / 255;;
					uint16_t tca_cmp1 = ((uint32_t)params.outp_pwm_duty[1] * newPER) / 255;;
					

					
					// set frequency and duty cycle
					TCA0.SINGLE.CTRLESET = TCA_SINGLE_LUPD_bm;						// Disable update
					TCA0.SINGLE.PERBUF = newPER;									// set new PER
					TCA0.SINGLE.CMP2BUF = tca_cmp2;									// OUT2 duty
					TCA0.SINGLE.CMP1BUF = tca_cmp1;									// OUT3 duty
					TCA0.SINGLE.CTRLA = tca_ctrla;									// set prescaler					
					TCA0.SINGLE.CTRLECLR = TCA_SINGLE_LUPD_bm;						// Enable update
					
					
					if (outState_tmp & 0x01)
					{
						// OUT1 = ON
						PORTD.OUTSET = PIN3_bm;	// PD3 = 1
					}
					else
					{
						// OUT1 = OFF
						PORTD.OUTCLR = PIN3_bm;	// PD3 = 0
					}
					
					uint8_t ctrlb_tmp = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
					
					
					switch (outState_tmp & 0x06)
					{
						case (OUT_STATE_OFF << 1):
							// OUT2 GPIO mode, off
							PORTD.OUTCLR = PIN2_bm;						// PD2 = 0
							break;
						case (OUT_STATE_ON << 1):
							// OUT2 GPIO mode, on
							PORTD.OUTSET = PIN2_bm;						// PD2 = 1
							break;
						case (OUT_STATE_PWM << 1):
							// OUT2 PWM mode
							PORTD.PIN2CTRL &= ~PORT_INVEN_bm;			// PD2 not inverted
							ctrlb_tmp |= TCA_SINGLE_CMP2EN_bm;			// enable PWM output
							break;
						case (OUT_STATE_PWM_INV << 1):
							// OUT2 inverted PWM mode
							PORTD.PIN2CTRL |= PORT_INVEN_bm;			// PD2 inverted
							ctrlb_tmp |= TCA_SINGLE_CMP2EN_bm;			// enable PWM output
							break;
					}
					
					switch (outState_tmp & 0x18)
					{
						case (OUT_STATE_OFF << 3):
							// OUT3 GPIO mode, off
							PORTD.OUTCLR = PIN1_bm;						// PD1 = 0
							break;
						case (OUT_STATE_ON << 3):
							// OUT3 GPIO mode, on
							PORTD.OUTSET = PIN1_bm;						// PD1 = 1
							break;
						case (OUT_STATE_PWM << 3):
							// OUT3 PWM mode
							PORTD.PIN1CTRL &= ~PORT_INVEN_bm;			// PD1 not inverted
							ctrlb_tmp |= TCA_SINGLE_CMP1EN_bm;			// enable PWM output
							break;
						case (OUT_STATE_PWM_INV << 3):
							// OUT3 inverted PWM mode
							PORTD.PIN1CTRL |= PORT_INVEN_bm;			// PD1 inverted
							ctrlb_tmp |= TCA_SINGLE_CMP1EN_bm;			// enable PWM output
							break;
					}


					TCA0.SINGLE.CTRLB = ctrlb_tmp;
			
					
					if (outState_tmp & 0x20)
					{
						// OUT4 = ON
						PORTC.OUTSET = 0x08;	// PC3 = 1
					}
					else
					{
						// OUT4 = OFF
						PORTC.OUTCLR = 0x08;	// PC3 = 0
					}
					
					if (outState_tmp & 0x40)
					{
						// RLY1 = ON
						PORTD.OUTSET = 0x10;	// PD4 = 1
					}
					else
					{
						// RLY1 = OFF
						PORTD.OUTCLR = 0x10;	// PD4 = 0
					}
					
					if (outState_tmp & 0x80)
					{
						// RLY2 = ON
						PORTD.OUTSET = 0x20;	// PD5 = 1
					}
					else
					{
						// RLY2 = OFF
						PORTD.OUTCLR = 0x20;	// PD5 = 0
					}		
					
					
					// set DAC
					
					
					VREF.DAC0REF = params.dac_val >> 14;	// set reference
					DAC0.DATA = params.dac_val << 6;		// DAC data register is left-justified

					// check watchdog counter
					
					if (params.watchdog_cnt != watchdog_cnt_last)
					{
						// reset timer
						watchdog_timeout = COMM_TIMEOUT;
						watchdog_cnt_last = params.watchdog_cnt;
					}


					// check if contactor should be enabled
					if (cont_enable_last != params.cont_enable)
					{
						cont_enable_last = params.cont_enable;
						
						if (0xA3 == params.cont_enable)
						{
							// try to enable contactor
							
							
							if (params.estop_mon < 6000)	// check estop
							{
								// voltage <6V, estop active or supply low
								PORTF.OUTCLR = PIN3_bm;
								//EVSYS.SWEVENTA = EVSYS_SWEVENTA_CH3_gc;
								params.cont_state = CONT_STATE_FAULT_ESTOP;
							}
							else if (PORTA.IN & PIN2_bm)	// check PA2 (FAULT)
							{
								// pin high = no fault
								// enable contactor
								PORTF.OUTSET = PIN3_bm;
								watchdog_timeout = COMM_TIMEOUT;	// reset watchdog timer
								fault_timeout = FAULT_TIMEOUT;		// reset fault timer
								params.cont_state = CONT_STATE_ON;
								
								
								//EVSYS.SWEVENTA = EVSYS_SWEVENTA_CH2_gc;
								//_delay_us(10);
								
								//if (PORTF.IN & PIN3_bm)
								//{
									//// detected high level on CONT_RLY pin
									//watchdog_timeout = COMM_TIMEOUT;	// reset watchdog timer
									//fault_timeout = FAULT_TIMEOUT;		// reset fault timer
									//cont_state = CONT_STATE_ON;
								//}
								//else
								//{
									//// failed to detect high level on CONT_RLY pin
									//PORTF.OUTCLR = PIN3_bm;
									////EVSYS.SWEVENTA = EVSYS_SWEVENTA_CH3_gc;	// CONT_RLY off
									//cont_state = CONT_STATE_FAULT_PINLVL;
								//}
								
							}
							else
							{
								// Fault pin active, don't enable
								PORTF.OUTCLR = PIN3_bm;
								//EVSYS.SWEVENTA = EVSYS_SWEVENTA_CH3_gc;	// CONT_RLY off
								params.cont_state = CONT_STATE_FAULT_HW;
							}
						}
						else
						{
							// switch off contactor
							PORTF.OUTCLR = PIN3_bm;
							//EVSYS.SWEVENTA = EVSYS_SWEVENTA_CH3_gc;	// CONT_RLY off
							params.cont_state = CONT_STATE_OFF;
						}
					}	
				}
				
				//reg_upd_pend = false;
				
				//uint8_t *pParams = (uint8_t*)&params;
				//uint8_t *pParams_mask = (uint8_t*)&params_write_mask;
				//uint8_t *pI2C_buf = i2c_upd_buf;
			//
				//for (uint8_t i = 0; i < sizeof(i2c_upd_buf); i++)
				//{		
					//// apply write mask
					//*pParams = (*pParams & ~*pParams_mask) | (*pI2C_buf & *pParams_mask);
					//pParams++;
					//pParams_mask++;
					//pI2C_buf++;
				//}
			
			
				//if ((reg_upd_startIdx < reg_upd_idx) && (reg_upd_idx < sizeof(i2c_upd_buf)))
				//{
					//// OK
					//
					//
					//
					////offsetof(params_t,params_t.sys_status)
					//
					//uint8_t i = reg_upd_startIdx;
					//while (i < reg_upd_idx)
					//{
						//uint8_t offs = offsetof(params_t,sys_status);
						////uint8_t len = sizeof(params.sys_status);
						//
						//if (i == offs)
						//{
							//// sys_status written
							////params.sys_status &= ~(i2c_upd_buf[i] & );
						//}
						//
						//
					//}
					//
					//
				//}
				//else
				//{
					//// out of range
					//
				//}
}


ISR(TWI0_TWIS_vect)
{
	volatile uint8_t sstatus = TWI0.SSTATUS;
	static bool ackMatters = false;
	static bool dataByte = false;
	static bool isDataWrite = false;
	static uint8_t regAddrInit = 0;
	static uint8_t regAddr = 0;
	//static uint8_t byteCnt = 0;
	//static uint8_t shadowBuf_len = 0;
	//static uint8_t shadowBuf_ptr;
	//static uint8_t shadowBuf[8];
	
	//static I2C_STATE_t i2c_state = I2C_STATE_IDLE;
	
	volatile uint8_t payload;
	
	uint8_t i2c_cmd;
	
	if (sstatus & TWI_APIF_bm)
	{
		// address/stop interrupt
		if (sstatus & TWI_AP_bm)
		{
			// address interrupt
			payload = TWI0.SDATA;	// read address
			
			i2c_active = true;
			dataByte = false;	// in case of write, the first byte after Start will be the register address
			ackMatters = false;	// in case of read, ignore ACK flag on first data interrupt (invalid as no ACK/NACK is sent from host yet)
			isDataWrite = false;
			
			regAddr = regAddrInit;	// always reset reg ptr on start condition
			
			TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;  // send ACK
			
			if (sstatus & TWI_DIR_bm)
			{
				// host wants to read data, update i2c buffer
				memcpy(i2c_val_buf, &params, sizeof(params));
			}
			
			
			//if (sstatus & TWI_DIR_bm)
			//{
				// host wants to read data
								
				//shadowBuf_ptr = 0;	// reset pointer
				//shadowBuf_len = 0;	// set length to zero
				
				
				
				
				
				
				
				//i2c_cmd = TWI_SCMD_RESPONSE_gc;	// "Execute Acknowledge Action succeeded by reception of next byte"
				
				//// fill shadow buffer
				//switch (regAddr)
				//{
					//case I2C_REG_SYS_STATUS:
						//shadowBuf[0] = 0x55;
						//shadowBuf_len = 1;
						//break;
					//case I2C_SUPPLY_VOLTAGE:
						//shadowBuf[0] = params.v_supply & 0xFF;		// low byte
						//shadowBuf[1] = (params.v_supply >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
					//case I2C_VDD:
						//shadowBuf[0] = params.vdd & 0xFF;			// low byte
						//shadowBuf[1] = (params.vdd >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
					//case I2C_CURRENT:
						//shadowBuf[0] = params.i_sense & 0xFF;			// low byte
						//shadowBuf[1] = (params.i_sense >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
					//case I2C_CONT_MON:
						//shadowBuf[0] = params.cont_mon & 0xFF;			// low byte
						//shadowBuf[1] = (params.cont_mon >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
					//case I2C_MCU_TEMP:
						//shadowBuf[0] = params.mcu_temp & 0xFF;			// low byte
						//shadowBuf[1] = (params.mcu_temp >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
					//case I2C_CURR_REF:
						//shadowBuf[0] = params.i_ref & 0xFF;			// low byte
						//shadowBuf[1] = (params.i_ref >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;	
						//break;
					//case I2C_CURR_DIAG:
						//shadowBuf[0] = params.i_diag & 0xFF;			// low byte
						//shadowBuf[1] = (params.i_diag >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;	
						//break;	
					//case I2C_ESTOP_MON:
						//shadowBuf[0] = params.estop_mon & 0xFF;			// low byte
						//shadowBuf[1] = (params.estop_mon >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;	
						//break;
							//
					//case I2C_IN1_V:
						//shadowBuf[0] = params.inp_v[0] & 0xFF;			// low byte
						//shadowBuf[1] = (params.inp_v[0] >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
					//case I2C_IN2_V:
						//shadowBuf[0] = params.inp_v[1] & 0xFF;			// low byte
						//shadowBuf[1] = (params.inp_v[1] >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
					//case I2C_IN3_V:
						//shadowBuf[0] = params.inp_v[2] & 0xFF;			// low byte
						//shadowBuf[1] = (params.inp_v[2] >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
					//case I2C_IN4_V:
						//shadowBuf[0] = params.inp_v[3] & 0xFF;			// low byte
						//shadowBuf[1] = (params.inp_v[3] >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
					//case I2C_IN5_V:
						//shadowBuf[0] = params.inp_v[4] & 0xFF;			// low byte
						//shadowBuf[1] = (params.inp_v[4] >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
					//case I2C_IN6_V:
						//shadowBuf[0] = params.inp_v[5] & 0xFF;			// low byte
						//shadowBuf[1] = (params.inp_v[5] >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
					//case I2C_IN7_V:
						//shadowBuf[0] = params.inp_v[6] & 0xFF;			// low byte
						//shadowBuf[1] = (params.inp_v[6] >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
					//case I2C_IN8_V:
						//shadowBuf[0] = params.inp_v[7] & 0xFF;			// low byte
						//shadowBuf[1] = (params.inp_v[7] >> 8) & 0xFF;	// high byte;
						//shadowBuf_len = 2;
						//break;
						//
						//
					//default:
						//// unknown CMD, send NACK
						//i2c_cmd = TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc;  // "Execute ACK Action succeeded by waiting for any Start (S/Sr) condition"
					//
				//}
								
				//TWI0.SCTRLB = i2c_cmd;
				
			//}
			//else
			//{
				// host wants to write data
			//	dataByte = false;
			//	TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;  // "Execute Acknowledge Action succeeded by reception of next byte"
				
			//}
			
			
		}
		else
		{
			// stop interrupt
			TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;  // "Wait for any Start (S/Sr) condition"
			
			//if ((!reg_upd_pend) && (isDataWrite))
			if (isDataWrite)
			{
				// registers were updated (written)
				isDataWrite = false;
				reg_upd_startIdx = regAddrInit;
				reg_upd_idx = regAddr;
				//reg_upd_pend = true;
				
				update_params();
				
			}
			
			
			i2c_active = false;
			
		}
	}
	else if (sstatus & TWI_DIF_bm)
	{
		if (sstatus & TWI_DIR_bm)
		{
			// host wants to read data
			if ((sstatus & (TWI_COLL_bm | TWI_RXACK_bm)) && (true == ackMatters))
			{
				// collision detected or RXACK bit set
				//ackMatters = false;
				i2c_cmd = TWI_SCMD_COMPTRANS_gc;         // "Wait for any Start (S/Sr) condition"
			}
			else
			{
				ackMatters = true;
				
				
				if (regAddr >= sizeof(i2c_val_buf))
				{
					// wrap around to address 0
					regAddr = 0;
				}
				
				TWI0.SDATA = i2c_val_buf[regAddr++];
				i2c_cmd = TWI_SCMD_RESPONSE_gc;          // "Execute a byte read operation followed by Acknowledge Action"
				
				//if (shadowBuf_ptr < shadowBuf_len)
				//{
					//// still data left in buffer
					//TWI0.SDATA = shadowBuf[shadowBuf_ptr++];
					//TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;          // "Execute a byte read operation followed by Acknowledge Action"
				//}
				//else
				//{
					//// no data left
					//TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;         // "Wait for any Start (S/Sr) condition"
				//}
				
			}
		}
		else
		{
			// host wants to write data
			payload = TWI0.SDATA;
			
			if (dataByte)
			{
				// receive data
				
				//if (reg_upd_pend)
				//{
					//// waiting for previous write to be processed, send NACK
					//i2c_cmd = TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc;  // "Execute ACK Action succeeded by waiting for any Start (S/Sr) condition"
				//}
				//else if (regAddr >= sizeof(i2c_upd_buf))
				if (regAddr >= sizeof(i2c_upd_buf))
				{
					// overflow, send NACK (no wrap-around on write)
					i2c_cmd = TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc;  // "Execute ACK Action succeeded by waiting for any Start (S/Sr) condition"
				}
				else
				{
					// init vars in case this is the first byte
					if (!isDataWrite)
					{
						reg_upd_startIdx = 0;
						reg_upd_idx = 0;
						isDataWrite = true;
					}
					
					
					// store byte and send ACK
					i2c_upd_buf[regAddr++] = payload;
					
					i2c_cmd = TWI_SCMD_RESPONSE_gc;	// "Execute Acknowledge Action succeeded by reception of next byte"
				}
				
				
				
			}
			else
			{
				// set reg address
				
				if (payload < sizeof(i2c_val_buf))
				{
					// OK, set address and send ACK
					regAddrInit = payload;
					regAddr = payload;
					dataByte = true;	
					isDataWrite = false;				
					i2c_cmd = TWI_SCMD_RESPONSE_gc;	// "Execute Acknowledge Action succeeded by reception of next byte"
				}
				else
				{
					// invalid address, send NACK
					i2c_cmd = TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc;  // "Execute ACK Action succeeded by waiting for any Start (S/Sr) condition"
				}
				
				//switch (regAddr)
				//{
					//case I2C_REG_SYS_STATUS:
					//case I2C_SUPPLY_VOLTAGE:
					//case I2C_VDD:
					//case I2C_CURRENT:
					//case I2C_CONT_MON:
					//case I2C_MCU_TEMP:
					//case I2C_CURR_REF:
					//case I2C_CURR_DIAG:
					//case I2C_ESTOP_MON:
					//case I2C_IN1_V:
					//case I2C_IN2_V:
					//case I2C_IN3_V:
					//case I2C_IN4_V:
					//case I2C_IN5_V:
					//case I2C_IN6_V:
					//case I2C_IN7_V:
					//case I2C_IN8_V:
						//i2c_cmd = TWI_SCMD_RESPONSE_gc;	// "Execute Acknowledge Action succeeded by reception of next byte"
						//break;
					//default:
						//// unknown CMD, send NACK
						//regAddr = 0;
						//i2c_cmd = TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc;  // "Execute ACK Action succeeded by waiting for any Start (S/Sr) condition"
						//
				//}
				
				//TWI0.SCTRLB = i2c_cmd;	// Execute CMD
			}
			
			

		}
		
		TWI0.SCTRLB = i2c_cmd;	// Execute CMD
		
	}
}



ISR(TCB0_INT_vect)
{
	// systick (5 ms)
	TCB0.INTFLAGS = TCB_CAPT_bm;	// Clear flag
	
	if (params.cont_state == CONT_STATE_ON)
	{
		
		// check FAULT input
		
		if ((PORTA.IN & PIN2_bm) == 0)
		{
			// FAULT active
			if (fault_timeout > 0)
			{
				fault_timeout--;
			}
			else
			{
				// timeout, disable contactor
				PORTF.OUTCLR = PIN3_bm;
				//EVSYS.SWEVENTA = EVSYS_SWEVENTA_CH3_gc;
				params.cont_state = CONT_STATE_FAULT_HW;
			}
		}
		else
		{
			// FAULT not active
			fault_timeout = FAULT_TIMEOUT;	// reset timer
		}
	}
		
	if (watchdog_timeout > 0)
	{
		watchdog_timeout--;
	}
	else
	{
		// timeout, disable contactor and release all general purpose outputs
		PORTF.OUTCLR = PIN3_bm;
			
		TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
		PORTD.OUTCLR = PIN3_bm | PIN2_bm | PIN1_bm | PIN4_bm | PIN5_bm;		// PD3 = 0, PD2 = 0, PD1 = 0, PD4 = 0, PD5 = 0
		PORTC.OUTCLR = PIN3_bm;		// PC3 = 0
		DAC0.DATA = 0;
			
			
		//EVSYS.SWEVENTA = EVSYS_SWEVENTA_CH3_gc;
		
		if (params.cont_state != CONT_STATE_OFF)
		{
			params.cont_state = CONT_STATE_FAULT_TIMEOUT;
		}
	}
	
}


//ISR(TCA0_CMP0_vect)
//{
	//// FAULT pre-warning (50 ms)
	//TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP0_bm;	// clear flag
//}
//
//ISR(TCA0_CMP1_vect)
//{
	//// FAULT active (100 ms)
	//
	//
	//if (cont_state == CONT_STATE_ON)
	//{
		//// HW fault input active, disable contactor (should already be done by HW but do it here also just to be safe)
		//EVSYS.SWEVENTA = EVSYS_SWEVENTA_CH3_gc;
		//cont_state = CONT_STATE_FAULT_HW;
	//}
	//
	//TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm;	// clear flag
//}


//cal_page_t* calData;

//uint16_t adc_meas(uint8_t adc_ch)
//{
	//uint16_t retVal;
	//
	//ADC0.MUXPOS = ADC_CH_MAPPING[adc_ch];
	//ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc;   /* 12-bit mode */
	///* Start ADC conversion */
	//ADC0.COMMAND = ADC_STCONV_bm;
	//while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));
	//ADC0.INTFLAGS = ADC_RESRDY_bm;
				//
	//retVal = (((uint32_t)ADC0.RES * calData->cal_data[adc_ch].fullscale) / 65536) + calData->cal_data[adc_ch].offset;	// apply calibration
				//
	//ADC0.CTRLA = 0;	// disable ADC
	//
	//return retVal;		
//}

void adc_start()
{
	// measure temperature
	ADC0.CTRLA = 0;	// disable ADC
	VREF.ADC0REF = VREF_REFSEL_2V048_gc;
	ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
	ADC0.SAMPCTRL = 30;	// Sample time 2 + 30 = 32 us
	ADC0.CTRLB = ADC_SAMPNUM_NONE_gc;	// 1 sample
	ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc;   /* 12-bit mode */
	
	/* Start ADC conversion */
	ADC0.COMMAND = ADC_STCONV_bm;
}

void adc_setup(uint8_t adc_ch, uint8_t adc_samp_cnt)
{
	ADC0.CTRLA = 0;											// disable ADC
	ADC0.CTRLB = adc_samp_cnt & ADC_SAMPNUM_gm;				// Number of samples
	ADC0.SAMPCTRL = ADC_CH_MAPPING[adc_ch].adc_map_samplen;	// Sample time
	ADC0.MUXPOS = ADC_CH_MAPPING[adc_ch].adc_map_pos;		// Positive MUX
	VREF.ADC0REF = ADC_CH_MAPPING[adc_ch].adc_map_ref;		// ADC reference
	
	if (ADC_CH_MAPPING[adc_ch].adc_map_neg == 0)
	{
		// single ended mode
		ADC0.MUXNEG = ADC_MUXNEG_GND_gc;						// Negative MUX
		ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc;		// 12-bit single ended mode
	}
	else
	{
		// differential mode
		ADC0.MUXNEG = ADC_CH_MAPPING[adc_ch].adc_map_neg;							// Negative MUX
		ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc | ADC_CONVMODE_DIFF_gc;	// 12-bit diff mode 
	}
		

}

uint16_t adc_get_U16(uint8_t adc_ch)
{
	int16_t cal_offset = params.cal_data[adc_ch].offset;		// Offset
	uint16_t cal_scale = params.cal_data[adc_ch].fullscale;	// Scale
	uint16_t adc_reading = ADC0.RES;							// ADC conversion result
	
	uint32_t temp = adc_reading;
	temp *= cal_scale;
	temp /= 65536;
	
	int32_t res = temp;
	res += cal_offset;
	
	if (res < 0)
	{
		return 0;
	}
	else if (res > 65535)
	{
		return 65535;
	}
	else
	{
		return res;
	}
	
	
	//if ((cal_offset < 0) && ((-cal_offset) > temp))
	//{
		//// result will be negative
		//return 0;
	//}
	//else
	//{
		//// result will be positive
		//return temp + cal_offset;	// apply offset
	//}
	
	
	
	
	//return (((uint32_t)ADC0.RES * calData->cal_data[adc_ch].fullscale) / 65536) + calData->cal_data[adc_ch].offset;	// apply calibration
}

int16_t adc_get_S16(uint8_t adc_ch)
{
	int16_t cal_offset = params.cal_data[adc_ch].offset;		// Offset
	uint16_t cal_scale = params.cal_data[adc_ch].fullscale;	// Scale
	int16_t adc_reading = ADC0.RES;							// ADC conversion result
	
	int32_t temp = adc_reading;
	temp *= cal_scale;
	temp /= 32768;
	temp += cal_offset;
	
	if (temp < -32768)
	{
		return -32768;
	}
	else if (temp > 32767)
	{
		return 32767;
	}
	else
	{
		return temp;
	}
	
	
	//return (((int32_t)ADC0.RES * calData->cal_data[adc_ch].fullscale) / 32768) + calData->cal_data[adc_ch].offset;	// apply calibration
}

int8_t adc_get_MCU_Temp()
{
	uint16_t sigrow_offset = SIGROW.TEMPSENSE1;		// Read unsigned offset from signature row
	uint16_t sigrow_slope = SIGROW.TEMPSENSE0;		// Read unsigned gain/slope from signature row
	uint16_t adc_reading = ADC0.RES;				// Read ADC conversion result

	if (adc_reading > sigrow_offset)
	{
		// out of range
		return -128;
	}
	else
	{
		uint32_t temp = sigrow_offset - adc_reading;	// Subtract ADC reading from offset
	
		temp *= sigrow_slope;							// Result can overflow 16-bit variable
		temp += 4096 / 2;								// Ensures correct rounding on division below
		temp /= 4096;									// Round off to nearest degree in Kelvin
		
		int16_t tempC = temp - 273;						// Convert to Celsius
		
		if (tempC < -128)
		{
			// out of range
			return -128;
		}
		else if (tempC > 127)
		{
			// out of range
			return 127;
		}
		else
		{
			return tempC;
		}
		
		//return temp - 273;								// return temperature in Celsius
	}
}

uint8_t adc_cur_idx = ADC_IDX_VDD;

ISR(ADC0_RESRDY_vect)
{
	// ADC conversion ready
	uint8_t muxPos = ADC0.MUXPOS;
	//uint8_t muxNeg = ADC0.MUXNEG;
	
	
	ADC0.INTFLAGS = ADC_RESRDY_bm;
	
	
	if (muxPos == ADC_MUXPOS_TEMPSENSE_gc)
	{
		// Temperature measurement complete, this is handled differently
		params.mcu_temp = adc_get_MCU_Temp();
		
		// start measuring the other channels
		adc_cur_idx = ADC_IDX_VDD;
		adc_setup(ADC_IDX_VDD, ADC_SAMPNUM_ACC128_gc);
		ADC0.COMMAND = ADC_STCONV_bm;
		
	}
	else
	{
		// measurement complete
		
		switch (adc_cur_idx)
		{
			case ADC_IDX_VDD:
				params.vdd = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_IN1:
				params.inp_v[0] = adc_get_U16(adc_cur_idx);
				break;	
			case ADC_IDX_IN2:
				params.inp_v[1] = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_IN3:
				params.inp_v[2] = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_IN4:
				params.inp_v[3] = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_IN5:
				params.inp_v[4] = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_IN6:
				params.inp_v[5] = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_IN7:
				params.inp_v[6] = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_IN8:
				params.inp_v[7] = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_V_SUPPLY:
				params.v_supply = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_CONT_MON:
				params.cont_mon = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_ESTOP_MON:
				params.estop_mon = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_CURR_REF:
				params.i_ref = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_CURR_DIAG:
				params.i_diag = adc_get_U16(adc_cur_idx);
				break;
			case ADC_IDX_CURR_SENSE:
				params.i_sense = adc_get_S16(adc_cur_idx);
				break;	
		}
		
		if (adc_cur_idx < ADC_IDX_CURR_SENSE)
		{
			// more channels to measure
			adc_cur_idx++;
			adc_setup(adc_cur_idx, ADC_SAMPNUM_ACC128_gc);
			ADC0.COMMAND = ADC_STCONV_bm;
		}
		else
		{
			// done, check if any actions need to be taken
			
			if (params.estop_mon < 6000)
			{
				// voltage <6V, estop active or supply low
				if (params.cont_state == CONT_STATE_ON)
				{
					// disable contactor
					PORTF.OUTCLR = PIN3_bm;
					params.cont_state = CONT_STATE_FAULT_ESTOP;
				}
						
			}
			
			// restart
			adc_start();
			
		}
		
	}
	
	
	//
	//
	//
	//switch (muxPos)
	//{
		//case ADC_MUXPOS_TEMPSENSE_gc:
			//// Temperature measurement complete
			//uint16_t sigrow_offset = SIGROW.TEMPSENSE1; // Read unsigned offset from signature row
			//uint16_t sigrow_slope = SIGROW.TEMPSENSE0; // Read unsigned gain/slope from signature row
			//uint16_t adc_reading = ADC0.RES; // ADCn conversion result
			//uint32_t temp = sigrow_offset - adc_reading;
			//temp *= sigrow_slope; // Result can overflow 16-bit variable
			//temp += 4096 / 2; // Ensures correct rounding on division below
			//temp /= 4096; // Round off to nearest degree in Kelvin
			//params.mcu_temp = temp - 273;
			//
			//// measure vdd
			//// Accumulate 128 samples, sample time 2 + 8 = 10 us
			//adc_setup(ADC_IDX_VDD, ADC_SAMPNUM_ACC128_gc);
//
			///* Start ADC conversion */
			//ADC0.COMMAND = ADC_STCONV_bm;
			//
			//break;
			//
		//case ADC_MUXPOS_VDDDIV10_gc:
			//params.vdd = adc_get_U32(ADC_IDX_VDD);
						//
			//// measure current
			//// Accumulate 128 samples, sample time 2 + 8 = 10 us
			//adc_setup(ADC_IDX_CURR_SENSE, ADC_SAMPNUM_ACC128_gc);			
			////ADC0.CTRLB = ADC_SAMPNUM_ACC16_gc;	// Accumulate 16 samples
			//
			///* Start ADC conversion */
			//ADC0.COMMAND = ADC_STCONV_bm;
			//
			//break;
			//
		//case ADC_MUXPOS_AIN7_gc:
		//
			//if (muxNeg == ADC_MUXNEG_AIN16_gc)
			//{
				//// Current measurement complete (differential)
				//params.i_sense = adc_get_S32(ADC_IDX_CURR_SENSE);
				//
				//// measure current diag input
				//// Accumulate 128 samples, sample time 2 + 8 = 10 us, ref = 4.096 V, single ended
				//adc_setup(ADC_IDX_CURR_DIAG, ADC_SAMPNUM_ACC128_gc);
								//
				///* Start ADC conversion */
				//ADC0.COMMAND = ADC_STCONV_bm;
			//}
			//else
			//{
				//// Current diag measurement complete (single ended)
				//params.i_diag = adc_get_U32(ADC_IDX_CURR_DIAG);
				//
			//}
			//
			//break;
		//
//
	//}
	
}




int main(void)
{
	/* OUT Registers Initialization */
	PORTA.OUT = 0;
	PORTC.OUT = 0;
	PORTD.OUT = 0;
	PORTF.OUT = 0x20;
	  
	/* DIR Registers Initialization */
	PORTA.DIR = 0;
	PORTC.DIR = 0x08;
	PORTD.DIR = 0x3E;
	PORTF.DIR = 0x28;
	
	
	/* PINxCTRL registers Initialization */
	PORTA.PINCONFIG	 = PORT_ISC_INPUT_DISABLE_gc;
	PORTA.PINCTRLUPD = 0x08;
	PORTC.PINCTRLUPD = 0x07;
	PORTD.PINCTRLUPD = 0xC0;
	PORTF.PINCTRLUPD = 0x17;	
	
	PORTA.PINCONFIG  = 0x00;
	PORTA.PINCTRLUPD = 0xF7;
	PORTC.PINCTRLUPD = 0x08;
	PORTD.PINCTRLUPD = 0x3E;
	PORTF.PINCTRLUPD = 0xE8;
	
	PORTA.PORTCTRL = PORT_SRL_bm;   // Port A slew rate limit on
	PORTC.PORTCTRL = PORT_SRL_bm;   // Port C slew rate limit on
	PORTD.PORTCTRL = PORT_SRL_bm;   // Port D slew rate limit on
	PORTF.PORTCTRL = PORT_SRL_bm;   // Port F slew rate limit on
	
	//PORTF.PIN0CTRL |= PORT_PULLUPEN_bm;	// PF0 pullup enable
	//PORTD.PIN7CTRL |= PORT_PULLUPEN_bm;	// PD7 pullup enable
	
	PORTA.PIN0CTRL |= PORT_PULLUPEN_bm;	// PA0 pullup enable (SDA)
	PORTA.PIN1CTRL |= PORT_PULLUPEN_bm;	// PA1 pullup enable (SCL)
	
	/* PORTMUX Initialization */
	PORTMUX.TWIROUTEA = PORTMUX_TWI0_ALT3_gc;       // TWI0 on PA0/1
	PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTD_gc;		// TCA0 on port D
	
	/* Clock init */
	//AUTOTUNE disabled; FRQSEL 20 MHz system clock; RUNSTDBY disabled;
	ccp_write_io((void*)&(CLKCTRL.OSCHFCTRLA),0x20);

	// System clock stability check by polling the status register.
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSCHFS_bm));
	
	//_delay_ms(1000);
	
	/* --- Systick init (TCB0) --- */
	TCB0.CCMP = 50000;	// 5 ms
	TCB0.INTCTRL = TCB_CAPT_bm;	// Enable CAPT interrupt
	TCB0.CTRLA = TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm;	// div by 2 (10 MHz)	
	
	
	/* PWM init (TCA0) */
	params.outp_pwm_duty[0] = 0;	// 0 %
	params.outp_pwm_duty[1] = 0;	// 0 %
	params.outp_pwm_freq = 99;		// 100 Hz
	
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;	// single slope PWM, outputs off
	TCA0.SINGLE.PER = 3124;	// 100 Hz
	TCA0.SINGLE.CMP2 = 0;	// 0 %
	TCA0.SINGLE.CMP1 = 0;	// 0 %
	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bp;	// Enable counter, prescaler = 64
	
	// initial i2c buffer fill
	//memcpy(i2c_val_buf, &params, sizeof(params));
	//memcpy(i2c_upd_buf, &params, sizeof(params));
	
	/* --- DAC init --- */
	VREF.DAC0REF = VREF_REFSEL_1V024_gc;		// enable DAC reference
	DAC0.DATA = 0;
	DAC0.CTRLA = DAC_OUTEN_bm | DAC_ENABLE_bm;	// enable DAC
	
	
	/* --- ADC init --- */
	
	/* Select ADC voltage reference */
	VREF.ADC0REF = VREF_REFSEL_4V096_gc;
	ADC0.CTRLB = ADC_SAMPNUM_ACC16_gc;	// Accumulate 16 samples
	ADC0.CTRLC = ADC_PRESC_DIV20_gc;      /* CLK_PER divided by 20 = 1 MHz */	
	ADC0.CTRLD = ADC_INITDLY_DLY32_gc;	// Init delay 32 us
	ADC0.SAMPCTRL = 8;	// Sample time 2 + 8 = 10 us
	ADC0.MUXNEG = ADC_MUXNEG_GND_gc;
	ADC0.MUXPOS = ADC_MUXPOS_GND_gc;
	ADC0.INTFLAGS = ADC_RESRDY_bm | ADC_WCMP_bm;	// clear interrupt flags
	ADC0.INTCTRL = ADC_RESRDY_bm;					// enable interrupt
	
	//// configure FAULT input (PA2) to trigger TCA0 via EVSYS CH0
	//EVSYS.CHANNEL0 = EVSYS_CHANNEL0_PORTA_PIN2_gc;
	//EVSYS.USERTCA0CNTB = EVSYS_USER_CHANNEL0_gc;
	//
	//
	//
	//EVSYS.CHANNEL1 = EVSYS_CHANNEL0_TCA0_CMP1_LCMP1_gc;
//
	//EVSYS.USERCCLLUT0B = EVSYS_USER_CHANNEL0_gc;	
	//EVSYS.USERCCLLUT1A = EVSYS_USER_CHANNEL1_gc;
	//EVSYS.USERCCLLUT0A = EVSYS_USER_CHANNEL2_gc;
	//EVSYS.USERCCLLUT1B = EVSYS_USER_CHANNEL3_gc;
	//
	//// EVSYS_CH0 -> LUT0B
	//// TCA0_CMP1 -> EVSYS_CH1 -> LUT1A
	//// SW_EVT -> EVSYS_CH2 -> LUT0A (set contactor ON)
	//// SW_EVT -> EVSYS_CH3 -> LUT1B (set contactor OFF)
	//// LUT0 -> S
	//// LUT1 -> R
	//// LUT1 -> LUT0 IN2 (C)
		//
	///* LUT0 (set)
	//A	B	OUT
	//X	0	0	block set while fault is still active
	//0	1	0
	//1	1	1
	//*/
	//
	///* LUT0 (set)
	//A	B	C	OUT
	//X	0	X	0	block set while fault is still active
	//X	X	1	0	make sure the R/S inputs are not both 1 (forbidden state)
	//0	1	0	0	idle state, ready for set cmd
	//1	1	0	1	set RS flip flop
	//*/
	//
	///* LUT1 (reset)
	//A	B	OUT
	//0	0	0
	//0	1	1
	//1	0	1
	//1	1	1
	//*/
	//
	//
	//CCL.LUT0CTRLB = CCL_INSEL0_EVENTA_gc | CCL_INSEL1_EVENTB_gc | CCL_INSEL2_LINK_gc;
	//CCL.TRUTH0 = 0x08;	// OUT=1 only when inputs are 110
	//
	//CCL.LUT1CTRLB = CCL_INSEL0_EVENTA_gc | CCL_INSEL1_EVENTB_gc;
	//CCL.TRUTH1 = 0x0E;	// OR
	//
	//CCL.SEQCTRL0 = CCL_SEQSEL_RS_gc;	// LUT0-OUT is out from SEQ0
	//
//
	//
	//
	//
	//CCL.LUT3CTRLB = CCL_INSEL0_LINK_gc;	// from LUT0/SEQ0
	//CCL.TRUTH3 = 0x02;
	//CCL.LUT0CTRLA = CCL_ENABLE_bm;
	//CCL.LUT1CTRLA = CCL_ENABLE_bm;
	//CCL.LUT3CTRLA = CCL_ENABLE_bm | CCL_OUTEN_bm;
	//
	//CCL.CTRLA = CCL_ENABLE_bm;
	//
	//// clear output (should already be clear, but do it just in case)
	//EVSYS.SWEVENTA = EVSYS_SWEVENTA_CH3_gc;	
	//
	//
	//TCA0.SINGLE.CMP0 = 976;		// 50 ms
	//TCA0.SINGLE.CMP1 = 1952;	// 100 ms
	//TCA0.SINGLE.EVCTRL = TCA_SINGLE_EVACTB_RESTART_HIGHLVL_gc | TCA_SINGLE_CNTBEI_bm;
	//TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm | TCA_SINGLE_CMP0_bm;	// Enable CMP0, CMP1 interrupt
	//TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	//TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc | TCA_SINGLE_ENABLE_bm;	// DIV by 1024, enable counter
	
	

	
	// I2C init
	
	/* FMPEN OFF; INPUTLVL I2C; SDAHOLD OFF; SDASETUP 4CYC */
	TWI0.CTRLA = 0x0;
	    
	TWI0.DBGCTRL = 0x0;
	TWI0.SADDR = 0x10 << 1;	// slave address = 0x10
	TWI0.SCTRLA = TWI_ENABLE_bm | TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm;
		
		
	initCal();
	
	//calData = getCalData();
	
	//int16_t adcRes;
	
	
	// enable WDT
	ccp_write_io((void*)&(WDT.CTRLA), WDT_PERIOD_512CLK_gc);	// 512 ms
	//wdt_enable(WDTO_500MS);
	
	sei();	
	
	adc_start();
	
    /* Replace with your application code */
    while (1) 
    {
		
		
		
		
		//// measure temperature
		//ADC0.CTRLA = 0;	// disable ADC
		//VREF.ADC0REF = VREF_REFSEL_2V048_gc;
		//ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
		//ADC0.SAMPCTRL = 30;	// Sample time 2 + 30 = 32 us
		//ADC0.CTRLB = ADC_SAMPNUM_NONE_gc;	// 1 sample
		//ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc;   /* 12-bit mode */
		//
		///* Start ADC conversion */
		//ADC0.COMMAND = ADC_STCONV_bm;
		//while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));
		//ADC0.INTFLAGS = ADC_RESRDY_bm;
		//
		//uint16_t sigrow_offset = SIGROW.TEMPSENSE1; // Read unsigned offset from signature row
		//uint16_t sigrow_slope = SIGROW.TEMPSENSE0; // Read unsigned gain/slope from signature row
		//uint16_t adc_reading = ADC0.RES; // ADCn conversion result
		//uint32_t temp = sigrow_offset - adc_reading;
		//temp *= sigrow_slope; // Result can overflow 16-bit variable
		//temp += 4096 / 2; // Ensures correct rounding on division below
		//temp /= 4096; // Round off to nearest degree in Kelvin
		//params.mcu_temp = temp - 273;
		
		//// measure vdd
		//ADC0.CTRLA = 0;	// disable ADC
		//ADC0.CTRLB = ADC_SAMPNUM_ACC128_gc;	// Accumulate 128 samples
		//ADC0.SAMPCTRL = 0;	// Sample time 2 + 8 = 10 us
		//ADC0.MUXPOS = ADC_MUXPOS_VDDDIV10_gc;
		//ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc;   /* 12-bit mode */
		///* Start ADC conversion */
		//ADC0.COMMAND = ADC_STCONV_bm;
		//while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));
		//ADC0.INTFLAGS = ADC_RESRDY_bm;
		//params.vdd = (((uint32_t)ADC0.RES * calData->cal_data[ADC_IDX_VDD].fullscale) / 65536) + calData->cal_data[ADC_IDX_VDD].offset;	// apply calibration
		//
		
		//// measure current
		//ADC0.CTRLA = 0;	// disable ADC
		//
		////ADC0.CTRLB = ADC_SAMPNUM_ACC16_gc;	// Accumulate 16 samples
		//ADC0.SAMPCTRL = 8;	// Sample time 2 + 8 = 10 us
		//ADC0.MUXPOS = ADC_MUXPOS_AIN7_gc;
		//ADC0.MUXNEG = ADC_MUXNEG_AIN16_gc;
		//ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc | ADC_CONVMODE_DIFF_gc;   /* 12-bit diff mode */
		///* Start ADC conversion */
		//ADC0.COMMAND = ADC_STCONV_bm;
		//while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));
		//ADC0.INTFLAGS = ADC_RESRDY_bm;
		////curr_res = ADC0.RES;
		//params.i_sense = (((int32_t)ADC0.RES * calData->cal_data[ADC_IDX_CURR_SENSE].fullscale) / 32768) + calData->cal_data[ADC_IDX_CURR_SENSE].offset;	// apply calibration
		
		//// measure other channels
		//ADC0.CTRLA = 0;	// disable ADC
		//VREF.ADC0REF = VREF_REFSEL_4V096_gc;
		//ADC0.MUXNEG = ADC_MUXNEG_GND_gc;
		
		//params.inp_v[0] = adc_meas(ADC_IDX_IN1);
		//params.inp_v[1] = adc_meas(ADC_IDX_IN2);
		//params.inp_v[2] = adc_meas(ADC_IDX_IN3);
		//params.inp_v[3] = adc_meas(ADC_IDX_IN4);
		//params.inp_v[4] = adc_meas(ADC_IDX_IN5);
		//params.inp_v[5] = adc_meas(ADC_IDX_IN6);
		//params.inp_v[6] = adc_meas(ADC_IDX_IN7);
		//params.inp_v[7] = adc_meas(ADC_IDX_IN8);
		//params.v_supply = adc_meas(ADC_IDX_V_SUPPLY);
		//params.cont_mon = adc_meas(ADC_IDX_CONT_MON);
		//params.estop_mon = adc_meas(ADC_IDX_ESTOP_MON);
		//params.i_ref = adc_meas(ADC_IDX_CURR_REF);
		//params.i_diag = adc_meas(ADC_IDX_CURR_DIAG);
		
		//for (uint8_t adc_ch=0; adc_ch < (sizeof(ADC_CH_MAPPING)/sizeof(ADC_CH_MAPPING[0])); adc_ch++)
		//{
			//ADC0.MUXPOS = ADC_CH_MAPPING[adc_ch];
			//ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc;   /* 12-bit mode */
			///* Start ADC conversion */
			//ADC0.COMMAND = ADC_STCONV_bm;
			//while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));
			//ADC0.INTFLAGS = ADC_RESRDY_bm;	
			//
			//adc_res[adc_ch] = (((uint32_t)ADC0.RES * calData->cal_data[adc_ch].fullscale) / 65536) + calData->cal_data[adc_ch].offset;	// apply calibration
			//
			//ADC0.CTRLA = 0;	// disable ADC
			//
		//}
		
		


		// check if estop was activated
		
		//if (params.estop_mon < 6000)
		//{
			//// voltage <6V, estop active or supply low
			//if (cont_state == CONT_STATE_ON)
			//{
				//// disable contactor
				//PORTF.OUTCLR = PIN3_bm;
				////EVSYS.SWEVENTA = EVSYS_SWEVENTA_CH3_gc;
				//cont_state = CONT_STATE_FAULT_ESTOP;
			//}	
			//
		//}
		
		
		// check if any data was written to buffer
		//if (reg_upd_pend)
		//{
			//update_params();
//
				//reg_upd_pend = false;
		//}	
		
		
		
		//params.cont_state = cont_state;
		
		

		
				// copy data to i2c buffer
				
				//cli();	// need to disable interrupts
				
				
		//// disable i2c data interrupt
		//TWI0.SCTRLA = TWI_ENABLE_bm | TWI_APIEN_bm | TWI_PIEN_bm;
		//
		//
		//if (!i2c_active)
		//{
//
//
//
			//
			//
			//
			//// update i2c buffers, but only if i2c is idle
			//memcpy(i2c_val_buf, &params, sizeof(params));
			////memcpy(i2c_upd_buf, &params, sizeof(params));
			//
			//
		//}
		//
		//// enable i2c data interrupt
		//TWI0.SCTRLA = TWI_ENABLE_bm | TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm;
		
		
		
		
		
		//sei();
		
		
		
		if (params.cal_enable == CAL_SAVE_KEY)
		{
			// update EEPROM
			uint16_t crc16 = 0xffff;
			
			crc16 = _crc_xmodem_update(crc16, 0x01);
			crc16 = _crc_xmodem_update(crc16, 0xff);
			
			__builtin_avr_wdr();	// Reset WDT
			eeprom_update_byte((void*)0x00,0x01);	// version
			__builtin_avr_wdr();	// Reset WDT
			eeprom_update_byte((void*)0x01,0xff);	// counter (not used, reserved for future use)			
			__builtin_avr_wdr();	// Reset WDT
			
			uint8_t len = sizeof(params.cal_data);
			
			uint8_t i;
			
			uint8_t *pCalData = (uint8_t*)params.cal_data;
			
			for (i = 2; i < (len + 2); i++)
			{				
				crc16 = _crc_xmodem_update(crc16, *pCalData);
				
				eeprom_update_byte((void*)i,*pCalData++);
				__builtin_avr_wdr();	// Reset WDT
			}
			
			eeprom_update_word((void*)i, crc16);

			
			//eeprom_update_block(params.cal_data, (void*)0x02, len);
			
			
			
			
			params.cal_enable = CAL_ENABLE_KEY;
		}
		
		
		__builtin_avr_wdr();	// Reset WDT
		
		
		
		set_sleep_mode(SLEEP_MODE_IDLE);
		cli();
		if (params.cal_enable != CAL_SAVE_KEY)
		{
			// EEPROM write not pending, go to sleep
			sleep_enable();
			sei();
			sleep_cpu();
			sleep_disable();
		}
		sei();
		
		
		
		
		
		
		//sleep_mode();
		
    }
}

//#define I2C_REG_SYS_STATUS	0x01
//
//#define I2C_SUPPLY_VOLTAGE	0x10
//#define I2C_VDD				0x11
//#define I2C_CURRENT			0x12
//#define I2C_CONT_MON		0x13
//#define I2C_MCU_TEMP		0x14
//#define I2C_CURR_REF		0x15
//#define I2C_CURR_DIAG		0x16
//#define I2C_ESTOP_MON		0x17
//
//#define I2C_IN1_V			0x20
//#define I2C_IN2_V			0x21
//#define I2C_IN3_V			0x22
//#define I2C_IN4_V			0x23
//#define I2C_IN5_V			0x24
//#define I2C_IN6_V			0x25
//#define I2C_IN7_V			0x26
//#define I2C_IN8_V			0x27