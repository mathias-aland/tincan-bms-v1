/*
 * logic_checks.h
 *
 * Created: 04-04-2024 16:49:39
 *  Author: Mathias
 */ 


#ifndef LOGIC_CHECKS_H_
#define LOGIC_CHECKS_H_











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

/* return true if contactor should be locked out */

uint16_t check_contactor_lockout(bool charge_mode);



bool check_turtle_mode_enable();
bool check_charger_enable(bool ac_present);
bool check_regen_lockout();
uint8_t check_heater_enable(bool chargerEn, bool ignOn);
bool reset_contactor_lockout();


bool check_ignition_active();
bool check_start_active();
bool check_acdet_active();


uint8_t setChargerOutput();
uint8_t setHeaterOutput();
uint8_t setPumpOutput();
uint8_t setTurtleOutput();
uint8_t setCheckEngineOutput();
uint8_t setChargerInterlockOutput();
uint8_t setDriveEnableOutput();
uint8_t setRegenLockoutOutput();
uint8_t setContStatusOutput();

#endif /* LOGIC_CHECKS_H_ */