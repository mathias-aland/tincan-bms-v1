/*
 * eeprom.c
 *
 * Created: 06-04-2024 02:01:08
 *  Author: Mathias
 */ 


#include "clkctrl.h"

#include <avr/io.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/crc16.h>

#include <stdio.h>
#include <stdint.h>

#include <stdbool.h>
#include "globals.h"

const __flash BMSLimits bms_limits_defaults = {
	/* set default values */
	
	/* Contactor lockout */
	.cellVoltHi = 4220,
	.cellVoltLo = 3200,
	.tempHi = 60 * TEMP_SCALE,			// 60 degC
	.tempLo = -30 * TEMP_SCALE,			// -30 degC
	.minCurrent = -800,					// -800 A
	.minCurrentPeak = -1000,			// -1000 A
	.maxCurrent = 800,					// 800 A
	.maxCurrentPeak = 1000,				// 1000 A
	.peakCurrentDelay = 4,				// Allow current between maxCurrent and maxCurrentPeak for this amount of seconds
	
	.contactorOffDelay = 3000,				// Delay in ms before contactor shuts off in case a value is outside normal range
	.packsExpected = 2,					// Number of battery packs expected
	
	/* Secondary protection */
	// LSB = CONFIG_COV (0x42), MSB = CONFIG_COVT (0x43)
	.secCOVreg = 0x80					// Enable over-voltage protection (CONFIG_COV bit 7)
				| ((4250-2000)/50)		// Set over-voltage protection to 4250 mV	
				| 0x8000				// Timeout unit = ms (CONFIG_COVT bit 7)
				| ((3100/100)<<8),		// Timeout = 3100 ms
				 
	// LSB = CONFIG_CUV (0x44), MSB = CONFIG_CUVT (0x45)
	.secCUVreg = 0x80					// Enable under-voltage protection (CONFIG_CUV bit 7)
				| ((3100-700)/100)		// Set under-voltage protection to 3100 mV
				| 0x8000				// Timeout unit = ms (CONFIG_CUVT bit 7)
				| ((3100/100)<<8),		// Timeout = 3100 ms	 
	
	// LSB = CONFIG_OT (0x46), MSB = CONFIG_OTT (0x47)
	.secOTreg = ((65-35)/5)				// Overtemp 1 = 65 degC
				| (((65-35)/5)<<4)		// Overtemp 2 = 65 degC
				| ((2550/10)<<8),		// Timeout = 2550 ms
	
	/* Turtle mode */
	.turtlemodeVolt = 3500,				// Limit power below this voltage, reset when charger is connected
	.turtlemodeDelay = 20,				// 20 seconds
	
	/* Charger control */
	.chgDisVolt = 4050,					// Shut down charger at this voltage level
	.chgEnVolt = 4020,					// Restart charger at this voltage level (only when previously shutdown)
	.chgInhibitVolt = 2400,				// Don't allow charging below this level
	.chgMinTemp = 0 * TEMP_SCALE,		// 0 degC
	.chgMaxTemp = 45 * TEMP_SCALE,		// 45 degC
	.chgDelay = 10,						// Delay before charger can be switched on again after switching off
	.acPresDelay = 3,					// AC detect delay
	
	/* Regen lockout */
	.regenMinTemp = 10 * TEMP_SCALE,	// 10 degC
	.regenMaxTemp = 45 * TEMP_SCALE,	// 45 degC
	.regenMaxVolt = 4180,
	.regenResetDelay = 10,
	
	/* Heater & pump control */
	.pumpOffSeconds = 30*60,			// 30 minutes
	.pumpOnSeconds = 5*60,				// 5 minutes
	.pumpOnTemp = 1 * TEMP_SCALE,
	.pumpFreqPWM = 2 - 1,					// 2 Hz
	.pumpDutyPWM = 25,			// 25 %
	.heaterOffTemp = 1.5 * TEMP_SCALE,
	.heaterOnTemp = 0.5 * TEMP_SCALE,
	
	/* Cell balancing */
	// Cell with lowest voltage is used as reference
	.balMaxDelta = 20,			// 20 mV
	.balFinishDelta = 0,		// 0 mV
	.balMinVoltage = 4000,		// 4000 mV
	.balInterval = 0x80 | 2,	// 2 minutes
	.balIdleTime = 2,			// 2 seconds idle time before restarting balancing
	
	/* GPIO */
	.ignThresholdOn = 8000,
	.ignThresholdOff = 4000,
	.startThresholdOn = 8000,
	.startThresholdOff = 4000,
	.acPresThresholdOn = 6000,
	.acPresThresholdOff = 2000,
	.ignGPI = 5,
	.startGPI = 2,
	.acPresGPI = 6,
	
	/* Contactor control */
	.contactorRelayTime = 100,			// Delay from contactor relay ON to contactor FET ON
	.contactorPrechargeTime = 500,		// Precharge time
	.contactorOnPulseTime = 100,		// ON pulse
	.contactorPwmFrequency = 20 - 1,	// 20 kHz
	.contactorPwmDuty = 25,				// 25 %
	.contactorIdleDelay = 5,			// Time to wait before switching off HV contactor when HV is not needed anymore
	
	/* Misc */
	.alertFaultMask = ((ALERT_FORCE | ALERT_TSD) << 8) | FAULT_SELFTEST | FAULT_FORCE,
	//.alertMask = ALERT_FORCE | ALERT_TSD,
	//.faultMask = FAULT_SELFTEST | FAULT_FORCE,
	.gpoFunc = {[0] = GPO_FUNC_BATT_PUMP, [1] = GPO_FUNC_BATT_PUMP_PWM, [2] = GPO_FUNC_BATT_HEATER, [3] = GPO_FUNC_CHG_ENABLE, [4] = GPO_FUNC_REGEN_LOCKOUT, [5] = GPO_FUNC_DRIVE_ENABLE}
};

void EE_readParams()
{
	// read EEPROM
	uint8_t ee_ver = eeprom_read_byte((void*)0x00);
	uint8_t ee_reserved = eeprom_read_byte((void*)0x01);
	
	uint16_t crc16 = 0xffff;
	
	crc16 = _crc_xmodem_update(crc16, ee_ver);
	crc16 = _crc_xmodem_update(crc16, ee_reserved);
	
	uint8_t len = sizeof(bms_limits);
	
	eeprom_read_block(&bms_limits, (void*)0x02, len);
	
	for (uint8_t i = 0; i < len; i++)
	{
		crc16 = _crc_xmodem_update(crc16, ((uint8_t*)&bms_limits)[i]);
	}
	
	uint16_t ee_crc16 = eeprom_read_word((void*)(len + 2));

	if ((ee_ver != 0x01) || (ee_reserved != 0x00) || (ee_crc16 != crc16))
	{
		// failure, load defaults
		
		memcpy_P(&bms_limits,&bms_limits_defaults,sizeof(BMSLimits));
		
	}	
}

bool ee_writepend = false;

void EE_requestWrite()
{
	ee_writepend = true;		
}

bool EE_writePending()
{
	return ee_writepend;
}

void EE_writeParamsLoop()
{
	static uint16_t ee_curraddr = 0;
	static uint16_t ee_crc16;
	static uint8_t *pData;
	
	if (!eeprom_is_ready())
	{
		// EE busy
		return;
	}
	
	if (ee_writepend)
	{
		if (ee_curraddr == 0)
		{
			// first byte, reset CRC and data pointer
			ee_crc16 = 0xffff;
			pData = (uint8_t*)&bms_limits;
			
			// Version
			ee_crc16 = _crc_xmodem_update(ee_crc16, 0x01);
			eeprom_update_byte((void*)0x00,0x01);
			
		}
		else if (ee_curraddr == 1)
		{
			// Reserved
			ee_crc16 = _crc_xmodem_update(ee_crc16, 0x00);
			eeprom_update_byte((void*)0x01,0x00);	// reserved for future use
		}
		else if (ee_curraddr == sizeof(bms_limits) + 2)
		{
			// CRC first byte
			eeprom_update_byte((void*)ee_curraddr, (ee_crc16 & 0xFF));
		}
		else if (ee_curraddr == sizeof(bms_limits) + 3)
		{
			// CRC last byte
			eeprom_update_byte((void*)ee_curraddr, (ee_crc16 >> 8));
			
			// All data written
			ee_writepend = false;
			ee_curraddr = 0xFFFF;
		}
		else if (ee_curraddr > sizeof(bms_limits) + 3)
		{
			// Error
			ee_writepend = false;
			ee_curraddr = 0xFFFF;
		}
		else
		{
			// Data
			ee_crc16 = _crc_xmodem_update(ee_crc16, *pData);
			eeprom_update_byte((void*)ee_curraddr,*pData++);
		}
		
		ee_curraddr++;
		
	}
	
}