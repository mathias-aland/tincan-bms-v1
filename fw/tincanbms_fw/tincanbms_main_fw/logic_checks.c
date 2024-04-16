/*
 * logic_checks.c
 *
 * Created: 04-04-2024 14:43:23
 *  Author: Mathias
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
#include "usart0.h"
#include "bmsmodule.h"
#include "systick.h"
#include "globals.h"




/* return >0 if contactor should be locked out */

uint16_t check_contactor_lockout(bool charge_mode)
{
	static uint32_t peak_current_timer = 0;
	static uint32_t counter_timeout = 0;
	static uint32_t lastCnt = 0;
	
	uint16_t retVal = 0;
	
	if (batt_stats.numModulesPresent != bms_limits.packsExpected)
	{
		// Number of packs mismatch
		retVal |= CONT_LOCKOUT_PACK_CNT;
	}
	
	/* Check if cell voltages are within limits */
	if (batt_stats.cellVolt_max > bms_limits.cellVoltHi)
	{
		// Cell voltage too high
		retVal |= CONT_LOCKOUT_OVP;
	}
	
	if (charge_mode == true)
	{
		// Charge
		if (batt_stats.cellVolt_min < bms_limits.chgInhibitVolt)
		{
			// Cell voltage too low
			retVal |= CONT_LOCKOUT_UVP_CHG;
		}
	}
	else
	{
		// Discharge
		if (batt_stats.cellVolt_min < bms_limits.cellVoltLo)
		{
			// Cell voltage too low
			retVal |= CONT_LOCKOUT_UVP;
		}		
	}
	
	/* Check if cell temperatures are within limits */
	if (batt_stats.temp_max > bms_limits.tempHi)
	{
		// Cell temperature too high
		retVal |= CONT_LOCKOUT_OTP;
	}
	
	if (batt_stats.temp_min < bms_limits.tempLo)
	{
		// Cell temperature too low
		retVal |= CONT_LOCKOUT_UTP;
	}
	
	
	/* Check if battery current is within limit */
	if ((batt_stats.batt_current > bms_limits.maxCurrentPeak) || (batt_stats.batt_current < bms_limits.minCurrentPeak))
	{
		// Current is too high
		retVal |= CONT_LOCKOUT_OCP_PEAK;
	}
	else if ((batt_stats.batt_current > bms_limits.maxCurrent) || (batt_stats.batt_current < bms_limits.minCurrent))
	{
		// Current high but not over peak limit, check if timer has expired
		if (SysTick_CheckElapsed(peak_current_timer, (uint32_t)bms_limits.peakCurrentDelay * 1000))
		{
			// Timer expired
			retVal |= CONT_LOCKOUT_OCP;
		}
	}
	else
	{
		// Within limits, reset timer
		peak_current_timer = SysTick_GetTicks();
	}
	
	/* Check for fault from secondary MCU */
	
	switch (comp_get_cont_state())
	{
		case CONT_STATE_FAULT_ESTOP:
		{
			retVal |= CONT_LOCKOUT_ESTOP;
			break;
		}
		case CONT_STATE_FAULT_HW:
		{
			retVal |= CONT_LOCKOUT_HW;
			break;
		}
		case CONT_STATE_FAULT_TIMEOUT:
		{
			retVal |= CONT_LOCKOUT_TIMEOUT;
			break;
		}
	}
	
	
	/* Check for active faults / alerts in the battery packs */
	
	if (batt_stats.moduleFaults & BATT_FAULT_NOCOMM)
	{
		// Comm fault
		retVal |= CONT_LOCKOUT_COMM;
	}
	
	
	if (((batt_stats.moduleFaults & ~BATT_FAULT_NOCOMM) & (bms_limits.alertFaultMask & 0xFF)) || (batt_stats.moduleAlerts & (bms_limits.alertFaultMask >> 8)))
	{
		// Fault active
		retVal |= CONT_LOCKOUT_PACK_FAULT;
	}
	
	// Check counter
	if (batt_stats.readOutsCnt != lastCnt)
	{
		// Counter increased since last time, reset timer
		counter_timeout = SysTick_GetTicks();
		lastCnt = batt_stats.readOutsCnt;
	}
	else
	{
		// No update, check timer
		if (SysTick_CheckElapsed(counter_timeout, 1000))	// 1 second should allow for some updates
		{
			// Timer expired
			retVal |= CONT_LOCKOUT_COUNTER;
		}
	}
	
	return retVal;
	
}

bool check_turtle_mode_enable()
{
	static uint32_t turtle_mode_timer = 0;
	
	if (batt_stats.cellVolt_min < bms_limits.turtlemodeVolt)
	{
		// Cell voltage is low, activate turtle mode when timer expires
		if (SysTick_CheckElapsed(turtle_mode_timer, (uint32_t)bms_limits.turtlemodeDelay * 1000))
		{
			// Timer expired
			return true;
		}
	}
	else
	{
		// Within limits, reset timer
		turtle_mode_timer = SysTick_GetTicks();
	}
	
	return false;
}


bool check_charger_enable(bool ac_present)
{
	static uint32_t charger_timer = 0;
	static bool charger_laststate = false;
	static bool charger_lockout = false;
	
	bool charger_state;
	
	charger_state = charger_laststate;
	
	if (ac_present)
	{
		// AC power is connected
				
		/* Check if cell temperatures are within limits */
		if (batt_stats.temp_max > bms_limits.chgMaxTemp)
		{
			// Cell temperature too high
			charger_state = false;
		}
		else if (batt_stats.temp_min < bms_limits.chgMinTemp)
		{
			// Cell temperature too low
			charger_state = false;
		}
		else
		{
			// Cell temps OK, check voltages
			if (batt_stats.cellVolt_max < bms_limits.chgEnVolt)
			{
				// Start charger
				charger_state = true;
			}
			else if (batt_stats.cellVolt_max > bms_limits.chgDisVolt)
			{
				// Stop charger
				charger_state = false;
			}
		}
		
		
	}
	else
	{
		// No AC power connected, stop charger
		charger_state = false;
	}


	if (charger_state != charger_laststate)
	{
		charger_laststate = charger_state;
		
		if (charger_state == false)
		{
			charger_lockout = true;	// Lockout for some time to avoid rapid on/off cycles
			charger_timer = SysTick_GetTicks();
			return false;
		}
		
		
		//if (SysTick_CheckElapsed(charger_timer, (uint32_t)bms_limits.chgDelay * 1000))
		//{
			//// Timer expired
			//charger_laststate = charger_state;
		//}		
		
	}
	//else
	//{
	//	charger_timer = SysTick_GetTicks();
	//}
	
	
	if (charger_lockout)
	{
		if (SysTick_CheckElapsed(charger_timer, (uint32_t)bms_limits.chgDelay * 1000))
		{
			// Timer expired
			charger_lockout = false;
		}
		else
		{
			return false;	// lockout in effect, don't allow charging
		}
	}
	
	return charger_laststate;
	
	
}


bool check_regen_lockout()
{
	static uint32_t regen_timer = 0;
	
	static bool regen_lockout = false;
	
	/* Check if cell temperatures are within limits */
	if ((batt_stats.temp_max > bms_limits.regenMaxTemp) || (batt_stats.temp_min < bms_limits.regenMinTemp))
	{
		// Cell temperature outside allowed range
		regen_lockout = true;
		regen_timer = SysTick_GetTicks();
	}
	else if (batt_stats.cellVolt_max > bms_limits.regenMaxVolt)
	{
		// Cell voltage is too high
		regen_lockout = true;
		regen_timer = SysTick_GetTicks();
	}
	else
	{
		// OK, check timer
		if (SysTick_CheckElapsed(regen_timer, (uint32_t)bms_limits.regenResetDelay * 1000))
		{
			// Timer expired
			regen_lockout = false;
		}		
	}
	
	
	
	return regen_lockout;
	
}

uint8_t check_heater_enable(bool chargerEn, bool ignOn)
{
	static uint32_t pump_lastRun = 0;
	static uint8_t heater_state = HEATER_ENABLE_OFF;
	
	if ((heater_state != HEATER_ENABLE_PUMP_HEAT) && (batt_stats.temp_min <= bms_limits.heaterOnTemp))
	{
		// switch heater and pump on
		heater_state = HEATER_ENABLE_PUMP_HEAT;
	}
	else if ((heater_state == HEATER_ENABLE_PUMP_HEAT) && (batt_stats.temp_min >= bms_limits.heaterOffTemp))
	{
		// all cells heated up, switch off heater
		heater_state = HEATER_ENABLE_PUMP;
		pump_lastRun = SysTick_GetTicks();	// let pump run for some time to allow for cool-down
	}
	
	if (chargerEn || ignOn)
	{
		heater_state = HEATER_ENABLE_PUMP;
		//pump_lastRun = SysTick_GetTicks() - ((uint32_t)bms_limits.pumpOnSeconds * 1000);
	}
	else if (heater_state == HEATER_ENABLE_PUMP)
	{
		// heater off but pump still on
		if (SysTick_CheckElapsed(pump_lastRun, (uint32_t)bms_limits.pumpOnSeconds * 1000))
		{
			// time elapsed, switch off pump
			heater_state = HEATER_ENABLE_OFF;
			pump_lastRun = SysTick_GetTicks();
		}
		
	}
	else if (heater_state == HEATER_ENABLE_OFF)
	{
		// pump off		
		if (batt_stats.temp_min <= bms_limits.pumpOnTemp)
		{
			if (SysTick_CheckElapsed(pump_lastRun, (uint32_t)bms_limits.pumpOffSeconds * 1000))
			{
				// time elapsed, switch on pump
				heater_state = HEATER_ENABLE_PUMP;
				pump_lastRun = SysTick_GetTicks();
			}
		}
		else
		{
			// temps are higher than start threshold, reset timer
			pump_lastRun = SysTick_GetTicks();
		}
	}
	
	return heater_state;
	
}




bool check_ignition_active()
{
	static bool ignStatus = false;
	
	if ((comp_get_gpi(bms_limits.ignGPI) > bms_limits.ignThresholdOn) && (!ignStatus))
	{
		// ignition ON
		ignStatus = true;
	}
	else if ((comp_get_gpi(bms_limits.ignGPI) < bms_limits.ignThresholdOff) && (ignStatus))
	{
		// ignition OFF
		ignStatus = false;
	}
	
	return ignStatus;
}

bool check_start_active()
{
	static bool startStatus = false;
	
	if ((comp_get_gpi(bms_limits.startGPI) > bms_limits.startThresholdOn) && (!startStatus))
	{
		// start ON
		startStatus = true;
	}
	else if ((comp_get_gpi(bms_limits.startGPI) < bms_limits.startThresholdOff) && (startStatus))
	{
		// start OFF
		startStatus = false;
	}
	
	return startStatus;
}

bool check_acdet_active()
{
	static uint32_t acdet_timer = 0;
	static bool acPres_laststate = false;
	
	bool acPres_state = false;
	
	if ((comp_get_gpi(bms_limits.acPresGPI) > bms_limits.acPresThresholdOn))
	{
		// ac_pres ON
		acPres_state = true;
	}
	else if ((comp_get_gpi(bms_limits.acPresGPI) < bms_limits.acPresThresholdOff))
	{
		// ac_pres OFF
		acPres_state = false;
	}
	
	
	
	if (acPres_state != acPres_laststate)
	{
			
		if (SysTick_CheckElapsed(acdet_timer, (uint32_t)bms_limits.acPresDelay * 1000))
		{
			// Timer expired
			acPres_laststate = acPres_state;
		}
			
	}
	else
	{
		acdet_timer = SysTick_GetTicks();
	}
	
	return acPres_laststate;
}


/* return true if successful */

void reset_contactor_lockout()
{
	
}


uint8_t setChargerOutput()
{
	uint8_t newOutputState = 0;
	
	if (bms_limits.gpoFunc[0] == GPO_FUNC_CHG_ENABLE)
	{
		newOutputState |= OUT_STATE_FET1_ON;
	}
	
	if (bms_limits.gpoFunc[1] == GPO_FUNC_CHG_ENABLE)
	{
		newOutputState |= OUT_STATE_FET2_ON;
	}
	
	if (bms_limits.gpoFunc[2] == GPO_FUNC_CHG_ENABLE)
	{
		newOutputState |= OUT_STATE_FET3_ON;
	}
	
	if (bms_limits.gpoFunc[3] == GPO_FUNC_CHG_ENABLE)
	{
		newOutputState |= OUT_STATE_FET4_ON;
	}
	
	if (bms_limits.gpoFunc[4] == GPO_FUNC_CHG_ENABLE)
	{
		newOutputState |= OUT_STATE_RELAY1_ON;
	}
	
	if (bms_limits.gpoFunc[5] == GPO_FUNC_CHG_ENABLE)
	{
		newOutputState |= OUT_STATE_RELAY2_ON;
	}
	
	return newOutputState;
}


uint8_t setContStatusOutput()
{
	uint8_t newOutputState = 0;
	
	if (bms_limits.gpoFunc[0] == GPO_FUNC_CONT_STATUS)
	{
		newOutputState |= OUT_STATE_FET1_ON;
	}
	
	if (bms_limits.gpoFunc[1] == GPO_FUNC_CONT_STATUS)
	{
		newOutputState |= OUT_STATE_FET2_ON;
	}
	
	if (bms_limits.gpoFunc[2] == GPO_FUNC_CONT_STATUS)
	{
		newOutputState |= OUT_STATE_FET3_ON;
	}
	
	if (bms_limits.gpoFunc[3] == GPO_FUNC_CONT_STATUS)
	{
		newOutputState |= OUT_STATE_FET4_ON;
	}
	
	if (bms_limits.gpoFunc[4] == GPO_FUNC_CONT_STATUS)
	{
		newOutputState |= OUT_STATE_RELAY1_ON;
	}
	
	if (bms_limits.gpoFunc[5] == GPO_FUNC_CONT_STATUS)
	{
		newOutputState |= OUT_STATE_RELAY2_ON;
	}
	
	return newOutputState;
}



uint8_t setHeaterOutput()
{
	uint8_t newOutputState = 0;
	
	if (bms_limits.gpoFunc[0] == GPO_FUNC_BATT_HEATER)
	{
		newOutputState |= OUT_STATE_FET1_ON;
	}
	
	if (bms_limits.gpoFunc[1] == GPO_FUNC_BATT_HEATER)
	{
		newOutputState |= OUT_STATE_FET2_ON;
	}
	
	if (bms_limits.gpoFunc[2] == GPO_FUNC_BATT_HEATER)
	{
		newOutputState |= OUT_STATE_FET3_ON;
	}
	
	if (bms_limits.gpoFunc[3] == GPO_FUNC_BATT_HEATER)
	{
		newOutputState |= OUT_STATE_FET4_ON;
	}
	
	if (bms_limits.gpoFunc[4] == GPO_FUNC_BATT_HEATER)
	{
		newOutputState |= OUT_STATE_RELAY1_ON;
	}
	
	if (bms_limits.gpoFunc[5] == GPO_FUNC_BATT_HEATER)
	{
		newOutputState |= OUT_STATE_RELAY2_ON;
	}
	
	return newOutputState;
}




uint8_t setPumpOutput()
{
	uint8_t newOutputState = 0;
	
	if (bms_limits.gpoFunc[0] == GPO_FUNC_BATT_PUMP)
	{
		newOutputState |= OUT_STATE_FET1_ON;
	}
	
	if (bms_limits.gpoFunc[1] == GPO_FUNC_BATT_PUMP)
	{
		newOutputState |= OUT_STATE_FET2_ON;
	}
	else if (bms_limits.gpoFunc[1] == GPO_FUNC_BATT_PUMP_PWM)
	{
		newOutputState |= OUT_STATE_FET2_PWM;
	}
	
	if (bms_limits.gpoFunc[2] == GPO_FUNC_BATT_PUMP)
	{
		newOutputState |= OUT_STATE_FET3_ON;
	}
	else if (bms_limits.gpoFunc[2] == GPO_FUNC_BATT_PUMP_PWM)
	{
		newOutputState |= OUT_STATE_FET3_PWM;
	}
	
	if (bms_limits.gpoFunc[3] == GPO_FUNC_BATT_PUMP)
	{
		newOutputState |= OUT_STATE_FET4_ON;
	}
	
	if (bms_limits.gpoFunc[4] == GPO_FUNC_BATT_PUMP)
	{
		newOutputState |= OUT_STATE_RELAY1_ON;
	}
	
	if (bms_limits.gpoFunc[5] == GPO_FUNC_BATT_PUMP)
	{
		newOutputState |= OUT_STATE_RELAY2_ON;
	}
	
	return newOutputState;
}

uint8_t setTurtleOutput()
{
	uint8_t newOutputState = 0;
	
	if (bms_limits.gpoFunc[0] == GPO_FUNC_TURTLE_MODE)
	{
		newOutputState |= OUT_STATE_FET1_ON;
	}
	
	if (bms_limits.gpoFunc[1] == GPO_FUNC_TURTLE_MODE)
	{
		newOutputState |= OUT_STATE_FET2_ON;
	}
	
	if (bms_limits.gpoFunc[2] == GPO_FUNC_TURTLE_MODE)
	{
		newOutputState |= OUT_STATE_FET3_ON;
	}
	
	if (bms_limits.gpoFunc[3] == GPO_FUNC_TURTLE_MODE)
	{
		newOutputState |= OUT_STATE_FET4_ON;
	}
	
	if (bms_limits.gpoFunc[4] == GPO_FUNC_TURTLE_MODE)
	{
		newOutputState |= OUT_STATE_RELAY1_ON;
	}
	
	if (bms_limits.gpoFunc[5] == GPO_FUNC_TURTLE_MODE)
	{
		newOutputState |= OUT_STATE_RELAY2_ON;
	}
	
	return newOutputState;
}

uint8_t setRegenLockoutOutput()
{
	uint8_t newOutputState = 0;
	
	if (bms_limits.gpoFunc[0] == GPO_FUNC_REGEN_LOCKOUT)
	{
		newOutputState |= OUT_STATE_FET1_ON;
	}
	
	if (bms_limits.gpoFunc[1] == GPO_FUNC_REGEN_LOCKOUT)
	{
		newOutputState |= OUT_STATE_FET2_ON;
	}
	
	if (bms_limits.gpoFunc[2] == GPO_FUNC_REGEN_LOCKOUT)
	{
		newOutputState |= OUT_STATE_FET3_ON;
	}
	
	if (bms_limits.gpoFunc[3] == GPO_FUNC_REGEN_LOCKOUT)
	{
		newOutputState |= OUT_STATE_FET4_ON;
	}
	
	if (bms_limits.gpoFunc[4] == GPO_FUNC_REGEN_LOCKOUT)
	{
		newOutputState |= OUT_STATE_RELAY1_ON;
	}
	
	if (bms_limits.gpoFunc[5] == GPO_FUNC_REGEN_LOCKOUT)
	{
		newOutputState |= OUT_STATE_RELAY2_ON;
	}
	
	return newOutputState;
}

uint8_t setCheckEngineOutput()
{
	uint8_t newOutputState = 0;
	
	if (bms_limits.gpoFunc[0] == GPO_FUNC_CHECK_ENGINE)
	{
		newOutputState |= OUT_STATE_FET1_ON;
	}
	
	if (bms_limits.gpoFunc[1] == GPO_FUNC_CHECK_ENGINE)
	{
		newOutputState |= OUT_STATE_FET2_ON;
	}
	
	if (bms_limits.gpoFunc[2] == GPO_FUNC_CHECK_ENGINE)
	{
		newOutputState |= OUT_STATE_FET3_ON;
	}
	
	if (bms_limits.gpoFunc[3] == GPO_FUNC_CHECK_ENGINE)
	{
		newOutputState |= OUT_STATE_FET4_ON;
	}
	
	if (bms_limits.gpoFunc[4] == GPO_FUNC_CHECK_ENGINE)
	{
		newOutputState |= OUT_STATE_RELAY1_ON;
	}
	
	if (bms_limits.gpoFunc[5] == GPO_FUNC_CHECK_ENGINE)
	{
		newOutputState |= OUT_STATE_RELAY2_ON;
	}
	
	return newOutputState;
}

uint8_t setChargerInterlockOutput()
{
	uint8_t newOutputState = 0;
	
	if (bms_limits.gpoFunc[0] == GPO_FUNC_CHG_INTERLOCK)
	{
		newOutputState |= OUT_STATE_FET1_ON;
	}
	
	if (bms_limits.gpoFunc[1] == GPO_FUNC_CHG_INTERLOCK)
	{
		newOutputState |= OUT_STATE_FET2_ON;
	}
	
	if (bms_limits.gpoFunc[2] == GPO_FUNC_CHG_INTERLOCK)
	{
		newOutputState |= OUT_STATE_FET3_ON;
	}
	
	if (bms_limits.gpoFunc[3] == GPO_FUNC_CHG_INTERLOCK)
	{
		newOutputState |= OUT_STATE_FET4_ON;
	}
	
	if (bms_limits.gpoFunc[4] == GPO_FUNC_CHG_INTERLOCK)
	{
		newOutputState |= OUT_STATE_RELAY1_ON;
	}
	
	if (bms_limits.gpoFunc[5] == GPO_FUNC_CHG_INTERLOCK)
	{
		newOutputState |= OUT_STATE_RELAY2_ON;
	}
	
	return newOutputState;
}

uint8_t setDriveEnableOutput()
{
	uint8_t newOutputState = 0;
	
	if (bms_limits.gpoFunc[0] == GPO_FUNC_DRIVE_ENABLE)
	{
		newOutputState |= OUT_STATE_FET1_ON;
	}
	
	if (bms_limits.gpoFunc[1] == GPO_FUNC_DRIVE_ENABLE)
	{
		newOutputState |= OUT_STATE_FET2_ON;
	}
	
	if (bms_limits.gpoFunc[2] == GPO_FUNC_DRIVE_ENABLE)
	{
		newOutputState |= OUT_STATE_FET3_ON;
	}
	
	if (bms_limits.gpoFunc[3] == GPO_FUNC_DRIVE_ENABLE)
	{
		newOutputState |= OUT_STATE_FET4_ON;
	}
	
	if (bms_limits.gpoFunc[4] == GPO_FUNC_DRIVE_ENABLE)
	{
		newOutputState |= OUT_STATE_RELAY1_ON;
	}
	
	if (bms_limits.gpoFunc[5] == GPO_FUNC_DRIVE_ENABLE)
	{
		newOutputState |= OUT_STATE_RELAY2_ON;
	}
	
	return newOutputState;
}
