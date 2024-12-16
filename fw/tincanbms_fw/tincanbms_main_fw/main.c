/*
 * tincanbms_main_fw.c
 *
 * Created: 26-10-2022 17:05:42
 * Author : Mathias
 */ 

#include "clkctrl.h"

#include <avr/io.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdint.h>

#include <stdbool.h>
#include "i2c.h"
#include "spi.h"
#include "usart0.h"
#include "bmsmodule.h"
#include "systick.h"
#include "logic_checks.h"
#include "eeprom.h"
#include "modbus.h"


FUSES = {
	.WDTCFG = PERIOD_OFF_gc | WINDOW_OFF_gc,
	.BODCFG = LVL_BODLEVEL3_gc | BOD_SLEEP_SAMPLED_gc | BOD_SAMPFREQ_128HZ_gc | BOD_ACTIVE_ENWAKE_gc,
	.OSCCFG = CLKSEL_OSCHF_gc,
	.SYSCFG0 = RSTPINCFG_RST_gc | UPDIPINCFG_UPDI_gc | CRCSEL_CRC16_gc | CRCSRC_NOCRC_gc | FUSE_EESAVE_bm,
	.SYSCFG1 = SUT_0MS_gc | MVSYSCFG_SINGLE_gc,
	.CODESIZE = 0x00,
	.BOOTSIZE = 0x00
};

LOCKBITS = LOCK_KEY_NOLOCK_gc;


#define PERIOD          999  /* f TCD cycle = 20MHz / (1 + 999) = 20kHz */
#define DUTY_CYCLE      750





#define ADC_SHIFT_DIV64    (6)







#define I2C_REG_SYS_STATUS	0x01

#define I2C_REG_SUPPLY_VOLTAGE	0x10
#define I2C_REG_VDD				0x11
#define I2C_REG_CURRENT			0x12
#define I2C_REG_CONT_MON		0x13
#define I2C_REG_MCU_TEMP		0x14
#define I2C_REG_CURR_REF		0x15
#define I2C_REG_CURR_DIAG		0x16
#define I2C_REG_ESTOP_MON		0x17
#define I2C_REG_IN1_V			0x20
#define I2C_REG_IN2_V			0x21
#define I2C_REG_IN3_V			0x22
#define I2C_REG_IN4_V			0x23
#define I2C_REG_IN5_V			0x24
#define I2C_REG_IN6_V			0x25
#define I2C_REG_IN7_V			0x26
#define I2C_REG_IN8_V			0x27


#define USART_BAUD_RATE(BAUD_RATE) (uint16_t)(((float)(F_CPU) * 64 / (16 * (float)(BAUD_RATE))) + 0.5)


bool heaterEnabled = false;
bool pumpEnabled = false;

uint8_t outputState = 0;

uint32_t pump_lastRun = 0;




ISR(NMI_vect)
{
	// Clock failure
	
	PORTD.OUTCLR = PIN2_bm;		// disallow contactor ON (main)
			
	// Disable TCD
	while(!(TCD0.STATUS & TCD_ENRDY_bm));	// ensure ENRDY bit is set
	TCD0.CTRLA = 0;							// Disable TCD
	while(!(TCD0.STATUS & TCD_ENRDY_bm));	// ensure ENRDY bit is set
			
	// Disable TCD pin control
	ccp_write_io((void*)&(TCD0.FAULTCTRL), 0);
	
	PORTD.OUTCLR = PIN3_bm | PIN4_bm;	// Disable contactor control FETs
	PORTD.OUTCLR = PIN6_bm;				// 5V ext disable	
	PORTD.OUTSET = PIN7_bm;				// Red LED ON
	PORTF.OUTCLR = PIN3_bm;				// Green LED off
	
	
	// Disable watchdog
	while (WDT.STATUS & WDT_SYNCBUSY_bm);
	ccp_write_io((void*)&(WDT.CTRLA), WDT_PERIOD_OFF_gc);	// WDT OFF	
	
	
	// Switch to 32K internal oscillator
	ccp_write_io((void*)&(CLKCTRL.MCLKCTRLA),CLKCTRL_CLKSEL_OSC32K_gc);

	// wait for 32K oscillator to start
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSC32KS_bm));
	
	// endless loop
	for(;;)
	{
		_delay_loop_2(2048);
		PORTD.OUTTGL = PIN7_bm;				// Red LED toggle
	}
	
		
}





static uint16_t ADC0_read(void)
{
    /* Start ADC conversion */
    ADC0.COMMAND = ADC_STCONV_bm;
    
    /* Wait until ADC conversion done */
    while ( !(ADC0.INTFLAGS & ADC_RESRDY_bm) )
    {
        ;
    }
    
    /* Clear the interrupt flag by writing 1: */
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    
    return ADC0.RES;
}


bool i2c_readval(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
	i2c_error_t i2c_ret;
	i2c_ret = I2C_Write(addr, &reg, 1);
	
	if(i2c_ret != I2C_ERROR_NONE)
	{
		return false;
	}
	
	i2c_ret = I2C_Read(addr,buf,len);
	
	if(i2c_ret != I2C_ERROR_NONE)
	{
		return false;
	}
	
	return true;
}








void sim_update_stats()
{
	
	
	if (sim_numFoundModules == 0)
	{
		return;
	}
	
	batt_stats_t batt_stats_tmp;
	
	batt_stats_tmp.temp_max = INT16_MIN;
	batt_stats_tmp.temp_min = INT16_MAX;
	batt_stats_tmp.cellVolt_max = 0;
	batt_stats_tmp.cellVolt_min = UINT16_MAX;
	batt_stats_tmp.moduleFaults = 0;
	batt_stats_tmp.moduleAlerts = 0;
	

	for (uint8_t mod=0; mod < sim_numFoundModules; mod++)
	{
		//uint8_t alerts, faults, cuv_flt, cov_flt;
		
		// check cell voltages
		for (int cell=0; cell<6; cell++)
		{
			uint16_t cellV = sim_modules[mod].cellVolt[cell];
			
			// Update cell min/max voltages
			if (cellV < batt_stats_tmp.cellVolt_min)
			{
				batt_stats_tmp.cellVolt_min = cellV;
			}
			
			if (cellV > batt_stats_tmp.cellVolt_max)
			{
				batt_stats_tmp.cellVolt_max = cellV;
			}
		}
		
		// check temperatures
		for (uint8_t sensor=0; sensor<2; sensor++)
		{
			int16_t tempVal = sim_modules[mod].temperatures[sensor];
			
			if (tempVal < batt_stats_tmp.temp_min)
			{
				batt_stats_tmp.temp_min = tempVal;	// update min temp value
			}
			
			if (tempVal > batt_stats_tmp.temp_max)
			{
				batt_stats_tmp.temp_max = tempVal;	// update max temp value
			}
		}
		
		/* Alerts */
		batt_stats_tmp.moduleAlerts |= sim_modules[mod].alerts;
		
		/* Faults */
		batt_stats_tmp.moduleFaults |= sim_modules[mod].faults;
	}
	
	// Current
	batt_stats_tmp.batt_current = sim_batt_current;
	
	// Readouts
	batt_stats_tmp.readOutsCnt = sim_data_readouts_done;
	
	// Number of modules
	batt_stats_tmp.numModulesPresent = sim_numFoundModules;
	
	batt_stats = batt_stats_tmp;
}


enum
{
	MAIN_STATE_INIT = 0,	// Startup state
	MAIN_STATE_IDLE,		// Idle
	MAIN_STATE_IGN_ON,		// Ignition switched on
	MAIN_STATE_DRIVE,		// Drive mode
	
	
	MAIN_STATE_CHARGE,		// Charger connected
	MAIN_STATE_FAULT,		// Fault
	MAIN_STATE_SLEEP		// Sleep mode
};

uint8_t main_state = MAIN_STATE_INIT;



uint8_t mainLogic_init()
{
	return MAIN_STATE_IDLE;
}

void driveMode_init()
{
	
}




uint8_t mainLogic_idle()
{
	static bool sw5V_enabled = false;
	static bool contactorEnabled = false;
	static bool driveEnabled = false;
	static bool chargerEnabled_last = false;
	
	static bool startActive_last = false;
	
	static uint32_t contactor_timer = 0;
	static uint32_t contactor_lockout_timer = 0;
	
	static uint32_t contactor_idle_timer = 0;	
	
	static uint8_t cont_step = 0;
	
	static bool init = false;	// flag to signal startup done
	
	uint8_t newOutputState = 0;
	
	uint16_t cont_lockout_new = 0;

	bool acPresActive = false;
	bool startActive = false;
	bool ignActive = false;
	bool turtleActive = false;
	bool chargerEnabled = false;
	bool regenLockout = false;
	
	uint8_t heaterState = 0;

	// wait for data to become available
	if (!init)
	{
		if (batt_stats.readOutsCnt >= 3)
		{
			init = true;
		} 
	}


	acPresActive = check_acdet_active();
	ignActive = check_ignition_active();
	startActive = check_start_active();

	if ((batt_stats.numModulesPresent != 0) && (init == true))
	{
		chargerEnabled = check_charger_enable(acPresActive);
		heaterState = check_heater_enable(cont_lockout != 0, chargerEnabled, ignActive);
		turtleActive = check_turtle_mode_enable();
		regenLockout = check_regen_lockout();
	}
	
	if ((!startActive_last) && (ignActive) && (startActive))
	{
		// start mode active
		startActive_last = true;
		
		// reset lockout
		cont_lockout = 0;		
	}
	else if ((startActive_last) && (!startActive))
	{
		// start mode released
		startActive_last = false;
	}
	
	if (chargerEnabled)
	{
		cont_lockout &= ~CONT_LOCKOUT_UVP;	// Clear UVP flag when charger is connected
	}
	
	if ((batt_stats.numModulesPresent != 0) && (init == true))
	{
		cont_lockout_new |= check_contactor_lockout(chargerEnabled);
	}
	
	
	
	if ((cont_lockout_new != 0) && ((cont_lockout_new | cont_lockout) != cont_lockout))
	{
		if (SysTick_CheckElapsed(contactor_lockout_timer, bms_limits.contactorOffDelay))
		{
			// Timer expired
			cont_lockout |= cont_lockout_new;
		}	
	}
	else
	{
		contactor_lockout_timer = SysTick_GetTicks();
	}
	
	//cont_lockout = 0;	// DEBUG
	
	if ((batt_stats.numModulesPresent == 0) || (init == false))
	{
		
		// Make sure heater is switched off in case no modules are found or data not available yet
		heaterState = HEATER_ENABLE_OFF;
	}
	
	if ((cont_lockout) || (batt_stats.numModulesPresent == 0) || (init == false))
	{
		// Contactor locked out, or no modules found, or data not available yet
		contactorEnabled = false;
		driveEnabled = false;
		chargerEnabled = false;		
	}
	else
	{
		// OK to switch on contactor
		
		if ((!acPresActive) && (ignActive) && (startActive) && (!driveEnabled))
		{
			// Transition to drive mode
			chargerEnabled = false;
			driveEnabled = true;
		}
		
		if (((acPresActive) || (!ignActive)) && (driveEnabled))
		{
			// Disable drive mode
			driveEnabled = false;
		}
		
		if (driveEnabled || chargerEnabled || (heaterState == HEATER_ENABLE_PUMP_HEAT))
		{
			// HV required
			if (!contactorEnabled)
			{
				// Extra check to avoid switching on if fault is present even if fault timer has not expired yet
				if (cont_lockout_new != 0)
				{
					driveEnabled = false;
					chargerEnabled = false;
				}
				else
				{
					contactorEnabled = true;
				}
			}
			
			contactor_idle_timer = SysTick_GetTicks();	// reset idle timer
			
		}
		else if (contactorEnabled)
		{
			// HV not needed anymore
			if (SysTick_CheckElapsed(contactor_idle_timer, (uint32_t)bms_limits.contactorIdleDelay * 1000))
			{
				// Timer expired
				contactorEnabled = false;
			}
		}
		
	}
	
	// WDT reset
	__builtin_avr_wdr();

	// contactor control
	
	if (contactorEnabled)
	{
		
		if (heaterState == HEATER_ENABLE_OFF)
		{
			// Force pump on when contactor is on
			heaterState = HEATER_ENABLE_PUMP;
		}
		
		if ((cont_step == 0) || (cont_step == 255))
		{
			// Step 1: Enable watchdog
			if (!(WDT.STATUS & WDT_SYNCBUSY_bm))
			{
				// WDT sync done
				ccp_write_io((void*)&(WDT.CTRLA), WDT_PERIOD_128CLK_gc);	// WDT ON, 125 ms
				cont_step = 1;
			}
		}
		
		else if (cont_step == 1)
		{
			// Step 2: Enable relay					
			if (!(WDT.STATUS & WDT_SYNCBUSY_bm))
			{
				// WDT sync done
				// allow contactor ON (comp)
				if (contactor_enable(0xa3))
				{
					// OK
					PORTD.OUTSET = 0x04;	// allow contactor ON (main)
		
					contactor_timer = SysTick_GetTicks();
					cont_step = 2;						
				}
				

			}
		}
		
		
		
		else if (cont_step == 2)
		{
			if (SysTick_CheckElapsed(contactor_timer, bms_limits.contactorRelayTime))
			{
				// Timer expired
				// Step 3: Enable precharge FET
				PORTD.OUTSET = PIN3_bm;
				contactor_timer = SysTick_GetTicks();
				cont_step = 3;
			}
		}
		
		
		
		else if (cont_step == 3)
		{
			if (SysTick_CheckElapsed(contactor_timer, bms_limits.contactorPrechargeTime))
			{
				// Timer expired
				// Step 4: Enable contactor control FET
				PORTD.OUTSET = PIN4_bm;
				contactor_timer = SysTick_GetTicks();
				cont_step = 4;
			}
		}
		else if (cont_step == 4)
		{
			if (SysTick_CheckElapsed(contactor_timer, bms_limits.contactorOnPulseTime))
			{
				// Timer expired
				// Step 5: Set appropriate PWM duty for contactor holding, switch off precharge
				PORTD.OUTCLR = PIN3_bm;
								
				/* write protected register */
				ccp_write_io((void*)&(TCD0.FAULTCTRL), TCD_CMPCEN_bm); /* enable channel C */
				    
				//WGMODE One ramp mode;
				TCD0.CTRLB = TCD_WGMODE_ONERAMP_gc;
				    
				// Waveform B to output C
				TCD0.CTRLC = TCD_CMPCSEL_PWMB_gc;
				    
				/* set the signal period and duty cycle */
				uint16_t tmpPwmPeriod = bms_limits.contactorPwmFrequency;
				tmpPwmPeriod = 20000 / (tmpPwmPeriod + 1);
				
				TCD0.CMPBCLR = tmpPwmPeriod - 1;
				//TCD0.CMPBCLR = (20000 / ((uint16_t)bms_limits.contactorPwmFrequency + 1)) - 1;
				
				TCD0.CMPBSET = tmpPwmPeriod - (uint16_t)(((uint32_t)tmpPwmPeriod * bms_limits.contactorPwmDuty) / 100);

				/* ensure ENRDY bit is set */
				while(!(TCD0.STATUS & TCD_ENRDY_bm));
				
				/* TCD input clock is 20MHz (20MHz divided by 1) */
				TCD0.CTRLA =  TCD_CLKSEL_OSCHF_gc        /* choose the timer's clock */
				            |  TCD_CNTPRES_DIV1_gc        /* choose the prescaler */
				            |  TCD_ENABLE_bm;             /* enable the timer */
				
				
				
				contactor_timer = SysTick_GetTicks();
				cont_step = 128;
				
			}
		}

		
		
	}
	else
	{
		PORTD.OUTCLR = PIN2_bm;		// disallow contactor ON (main)
		
		// Disable TCD
		while(!(TCD0.STATUS & TCD_ENRDY_bm));	// ensure ENRDY bit is set
		TCD0.CTRLA = 0;							// Disable TCD
		while(!(TCD0.STATUS & TCD_ENRDY_bm));	// ensure ENRDY bit is set
		
		// Disable TCD pin control
		ccp_write_io((void*)&(TCD0.FAULTCTRL), 0);
		
		// Disable contactor control FETs
		PORTD.OUTCLR = PIN3_bm | PIN4_bm;		
			
		if (cont_step == 255)
		{
			// Step 255: Disable watchdog
			if (!(WDT.STATUS & WDT_SYNCBUSY_bm))
			{
				// WDT sync done
				ccp_write_io((void*)&(WDT.CTRLA), WDT_PERIOD_OFF_gc);	// WDT OFF
				cont_step = 0;
			}
		}
		else if (cont_step != 0)
		{
			// disallow contactor ON (comp)
			if (contactor_enable(0x00))
			{
				// OK
				cont_step = 255;
			}
		}
	}
	
	
	// Cell balancing
	if ((acPresActive) && (cont_step == 0) && (init == true))
	{
		// balancing allowed
		batt_set_balAllowed(true);
	}
	else
	{
		// balancing forbidden
		batt_set_balAllowed(false);
	}
	
	// Pump control output
	if (heaterState != HEATER_ENABLE_OFF)
	{
		// Pump should be started
		newOutputState |= setPumpOutput();
	}	
	
	if (cont_step == 128)
	{
		// Contactor ON
		newOutputState |= setContStatusOutput();
		
		// Charger control output
		if (chargerEnabled)
		{
			newOutputState |= setChargerOutput();
		}
	
		// Turtle mode control output
		if ((turtleActive) && (driveEnabled))
		{
			newOutputState |= setTurtleOutput();
		}
	
		// Regen lockout control output
		if ((regenLockout) && (driveEnabled))
		{
			newOutputState |= setRegenLockoutOutput();
		}

		// Drive enable output
		if (driveEnabled)
		{
			newOutputState |= setDriveEnableOutput();
		}	
		
		// Heater control output
		if (heaterState == HEATER_ENABLE_PUMP_HEAT)
		{
			newOutputState |= setHeaterOutput();
		}			
		
	}
	
	// Charger interlock output
	if (acPresActive)
	{
		newOutputState |= setChargerInterlockOutput();
	}
	
	// Check engine output
	if  ((((cont_lockout) || (batt_stats.numModulesPresent == 0)) && ignActive) || (!init))
	{
		newOutputState |= setCheckEngineOutput();
	}	
	
	if (outputState != newOutputState)
	{
		if (set_outputs(newOutputState, bms_limits.pumpFreqPWM, (bms_limits.pumpDutyPWM * 255)/100, (bms_limits.pumpDutyPWM * 255)/100))
		{
			// successful
			outputState = newOutputState;
		}
	}
	
	return MAIN_STATE_IDLE;
}


int main(void) {
    
    
  /* OUT Registers Initialization */
    PORTA.OUT = 0x84;
    PORTC.OUT = 0x1;
    PORTD.OUT = 0x0;
    PORTF.OUT = 0x0;    
    
  /* DIR Registers Initialization */
    PORTA.DIR = 0xD4;
    PORTC.DIR = 0x1;
    PORTD.DIR = 0xDC;
    PORTF.DIR = 0xA;



  /* PINxCTRL registers Initialization */
    PORTA.PIN0CTRL = 0x0;
    PORTA.PIN1CTRL = 0x0;
    PORTA.PIN2CTRL = 0x0;
    PORTA.PIN3CTRL = 0x0;
    PORTA.PIN4CTRL = 0x0;
    PORTA.PIN5CTRL = 0x0;
    PORTA.PIN6CTRL = 0x0;
    PORTA.PIN7CTRL = 0x0;
    PORTC.PIN0CTRL = 0x0;
    PORTC.PIN1CTRL = 0x0;
    PORTC.PIN2CTRL = PORT_PULLUPEN_bm;	// SDA pull-up active
    PORTC.PIN3CTRL = PORT_PULLUPEN_bm;	// SCL pull-up active
    PORTC.PIN4CTRL = 0x0;
    PORTC.PIN5CTRL = 0x0;
    PORTC.PIN6CTRL = 0x0;
    PORTC.PIN7CTRL = 0x0;
    PORTD.PIN0CTRL = 0x0;
    PORTD.PIN1CTRL = 0x0;
    PORTD.PIN2CTRL = 0x0;
    PORTD.PIN3CTRL = 0x0;
    PORTD.PIN4CTRL = 0x0;
    PORTD.PIN5CTRL = 0x0;
    PORTD.PIN6CTRL = 0x0;
    PORTD.PIN7CTRL = 0x0;
    PORTF.PIN0CTRL = 0x0;
    PORTF.PIN1CTRL = 0x0;
    PORTF.PIN2CTRL = 0x0;
    PORTF.PIN3CTRL = 0x0;
    PORTF.PIN4CTRL = 0x0;
    PORTF.PIN5CTRL = 0x0;
    PORTF.PIN6CTRL = 0x0;
    PORTF.PIN7CTRL = 0x0;
    
    PORTA.PORTCTRL = PORT_SRL_bm;   // Port A slew rate limit on
    PORTC.PORTCTRL = PORT_SRL_bm;   // Port C slew rate limit on
    PORTD.PORTCTRL = PORT_SRL_bm;   // Port D slew rate limit on
    PORTF.PORTCTRL = PORT_SRL_bm;   // Port F slew rate limit on

  /* PORTMUX Initialization */
    PORTMUX.SPIROUTEA = PORTMUX_SPI0_DEFAULT_gc;    // SPI0 on PA4/5/6/7
    PORTMUX.TCDROUTEA = PORTMUX_TCD0_ALT4_gc;       // TCD0 on PA4/PA5/PD4/PD5
    PORTMUX.TWIROUTEA = PORTMUX_TWI0_ALT2_gc;       // TWI0 on PC2/3
    PORTMUX.USARTROUTEA = PORTMUX_USART0_ALT2_gc | PORTMUX_USART1_DEFAULT_gc;   // USART0 on PA2/3, USART1 on PC0/1
	
	
	
	/* Clock init */
	//AUTOTUNE disabled; FRQSEL 20 MHz system clock; RUNSTDBY disabled;
	ccp_write_io((void*)&(CLKCTRL.OSCHFCTRLA),CLKCTRL_FRQSEL_20M_gc);
	
	// System clock stability check by polling the status register.
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSCHFS_bm));	
	
	// enable CFD interrupt
	ccp_write_io((void*)&(CLKCTRL.MCLKINTCTRL),CLKCTRL_INTTYPE_NMI_gc | CLKCTRL_CFD_bm);
	
	// enable CFD
	ccp_write_io((void*)&(CLKCTRL.MCLKCTRLC),CLKCTRL_CFDEN_bm);
	
	// enable crystal osc
	ccp_write_io((void*)&(CLKCTRL.XOSCHFCTRLA),CLKCTRL_FRQRANGE_24M_gc | CLKCTRL_CSUTHF_4K_gc  | CLKCTRL_ENABLE_bm);	// 20 MHz crystal, 4k cycles startup time
	
	// select crystal as clock source
	ccp_write_io((void*)&(CLKCTRL.MCLKCTRLA),CLKCTRL_CLKSEL_EXTCLK_gc);

	// wait for crystal to start
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
	
	
	// load parameters
	EE_readParams();
	
	
	
	
	// systick init (TCB0)
	TCB0.CCMP = 0x4E20;	// 1 ms
	TCB0.INTCTRL = TCB_CAPT_bm;	// Enable CAPT interrupt
	TCB0.CTRLA = TCB_ENABLE_bm;
	
    // batt comm timer init (TCB1)
	TCB1.CTRLB = TCB_CNTMODE_INT_gc;
	TCB1.CCMP = 65535;
	TCB1.CNT = 0;
	TCB1.INTCTRL = TCB_CAPT_bm;						// enable capture interrupt
	
	
	
	    
	    if (PORTF.IN & 0x20)
	    {
		    // normal
		    PORTF.OUT |= 0x02;
		    PORTD.OUT &= ~0x80;
	    }
	    else
	    {
		    // boot mode
		    PORTF.OUT &= ~0x02;
		    PORTD.OUT |= 0x80;
	    }
	
	
	
	
	
	
    /* --- USART0 INIT (PC COMM) --- */
    USART0_Init();     
     
     _delay_ms(100);
    
     //printf("PWR enable.\n\r");
     
     PORTD.OUTSET = 0x40;
     
     _delay_ms(500);
     
     
     /* --- ADC init --- */
     
     /* Select ADC voltage reference */
    VREF.ADC0REF = VREF_REFSEL_VDD_gc;
    
    /* Disable digital input buffers */
    PORTF.PIN2CTRL &= ~PORT_ISC_gm;
    PORTF.PIN2CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    PORTF.PIN4CTRL &= ~PORT_ISC_gm;
    PORTF.PIN4CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    
    /* Disable pull-up resistor */
    PORTF.PIN2CTRL &= ~PORT_PULLUPEN_bm;
    PORTF.PIN4CTRL &= ~PORT_PULLUPEN_bm;

    ADC0.CTRLC = ADC_PRESC_DIV4_gc;      /* CLK_PER divided by 4 */
    
    /* Set the accumulator mode to accumulate 64 samples */
    ADC0.CTRLB = ADC_SAMPNUM_ACC64_gc;
    
    ADC0.CTRLA = ADC_ENABLE_bm          /* ADC Enable: enabled */
               | ADC_RESSEL_10BIT_gc;   /* 10-bit mode */
    
    
    /* --- READ DIP SWITCHES --- */
    PORTF.OUTSET = 0x08;    // enable reading
    ADC0.MUXPOS = ADC_MUXPOS_AIN18_gc;  // DIP1
    _delay_ms(10);  // wait for voltages to settle
    
    uint16_t adcVal;
    
    adcVal = ADC0_read();
    adcVal = adcVal >> ADC_SHIFT_DIV64;
    //printf("DIP1: %u\n\r", adcVal);
    
    
    
    ADC0.MUXPOS = ADC_MUXPOS_AIN20_gc;  // DIP2
    _delay_ms(10);  // wait for voltages to settle
        
    adcVal = ADC0_read();
    adcVal = adcVal >> ADC_SHIFT_DIV64;
    //printf("DIP2: %u\n\r", adcVal);
    
    PORTF.OUTCLR = 0x08;    // disable reading
     
    
    /* --- I2C INIT --- */
    //printf("I2C init.\n\r");
    
    I2C_Init();
    
	
	
    
    
    /* --- SPI init --- */
    //printf("SPI init.\n\r");
    
    /* SPI Clock speed is set to 2 x 4MHz / 16 = 500 kHz */
    SPI0.CTRLA = SPI_CLK2X_bm           /* Enable double-speed */
               | SPI_ENABLE_bm          /* Enable module */
               | SPI_MASTER_bm          /* SPI module in Host mode */
               | SPI_PRESC_DIV16_gc;    /* System Clock divided by 16 */
    
    
    for (uint16_t i = 0; i < 256; i++)
    {

    }
    
	
	// Reset CAN controller
    mcp2515_reset();
	_delay_ms(1);	// wait for reset
	
    //printf("CANSTAT = 0x%02x\n\r", mcp2515_readbyte(0x0E));	// CANSTAT
   // printf("CANCTRL = 0x%02x\n\r", mcp2515_readbyte(0x0F));	// CANCTRL

	// CAN controller sleep
	mcp2515_writebyte(0x0F, 0x23);	// CANCTRL = 0x23 (set sleep mode, disable CLKOUT)

	
	/* --- USART1 INIT (BATT COMM) --- */
	
	/* Configure Baud Rate */
	USART1.BAUD = USART_BAUD_RATE(612500);
	
	
	
	/* Enable TX/RX */
	USART1.CTRLB =  USART_TXEN_bm | USART_RXEN_bm;
	
	/* Enable RX interrupt */
	USART1.CTRLA |= USART_RXCIE_bm;
	
	
	USART0.CTRLA |= USART_RXCIE_bm;	// enable UART interrupts
	
	sei();	// global interrupt enable

		
		//reg = TCB0.CTRLA;
		//printf("TCB0.CTRLA = 0x%02x\n\r", reg);
		//reg = TCB0.INTCTRL;
		//printf("TCB0.INTCTRL = 0x%02x\n\r", reg);
		
	
		
		
		USART0.CTRLB |= USART_RXEN_bm;
	/* Enable USART0 RX interrupt */
	USART0.CTRLA |= USART_RXCIE_bm;
		
		
		PORTF.OUT &= ~0x02;	// Green LED off
		
		
		//set_outputs(0xf5, 1, 64, 191);	// all outputs ON, PWM1-2 active, 2 Hz, 25/75% duty 
		
		//contactor_enable(0xa3); // allow contactor ON
		
		
		
		set_outputs(0, bms_limits.pumpFreqPWM, (bms_limits.pumpDutyPWM * 255)/100, (bms_limits.pumpDutyPWM * 255)/100);
		contactor_enable(0x00);		// disallow contactor ON (comp)
		
		//_delay_ms(100);
		
		batt_exit_boot();
		
		_delay_ms(100);
		
		batt_req_init();
		batt_req_clear(0xEF, 0x2F);	// Init has higher priority and will run to completion before starting alarm clear


    while (1) {
	    

		bms_state_machine();
		i2c_loop();
		modbus_state_machine();
	   // cli_process();
		
		
		if (simKey == SIM_MODE_KEY)
		{
			sim_update_stats();	// update battery stats (min/max temp, faults etc)
		}
		else
		{
			simKey = 0;
		}
		
		
		// control logic
		mainLogic_idle();
		
		EE_writeParamsLoop();
		
		
		mainloop_cycles++;
		
	}
}
