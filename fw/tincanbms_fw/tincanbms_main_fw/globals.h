/*
 * globals.h
 *
 * Created: 04-04-2024 15:02:25
 *  Author: Mathias
 */ 


#ifndef GLOBALS_H_
#define GLOBALS_H_

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

#define SIM_MODE_KEY 0xDEAD

#define HEATER_ENABLE_OFF			0
#define HEATER_ENABLE_PUMP			1
#define HEATER_ENABLE_PUMP_HEAT		2


#define TEMP_SCALE 100

#define SIM_MAX_MODULES	2

#define CONT_LOCKOUT_UVP_CHG	0x0001
#define CONT_LOCKOUT_UVP		0x0002
#define CONT_LOCKOUT_OVP		0x0004
#define CONT_LOCKOUT_UTP		0x0008
#define CONT_LOCKOUT_OTP		0x0010
#define CONT_LOCKOUT_OCP		0x0020
#define CONT_LOCKOUT_OCP_PEAK	0x0040
#define CONT_LOCKOUT_PACK_FAULT	0x0080
#define CONT_LOCKOUT_COMM		0x0100
#define CONT_LOCKOUT_HW			0x0200
#define CONT_LOCKOUT_ESTOP		0x0400
#define CONT_LOCKOUT_TIMEOUT	0x0800
#define CONT_LOCKOUT_COUNTER	0x1000
#define CONT_LOCKOUT_PACK_CNT	0x2000
#define CONT_LOCKOUT_NO_MODULES	0x4000
#define CONT_LOCKOUT_OTHER		0x8000



#define OUT_STATE_RELAY1_ON		0x40
#define OUT_STATE_RELAY2_ON		0x80
#define OUT_STATE_FET1_ON		0x01
#define OUT_STATE_FET2_OFF		0x00
#define OUT_STATE_FET2_ON		0x02
#define OUT_STATE_FET2_PWM		0x04
#define OUT_STATE_FET2_INVPWM	0x06
#define OUT_STATE_FET3_OFF		0x00
#define OUT_STATE_FET3_ON		0x08
#define OUT_STATE_FET3_PWM		0x10
#define OUT_STATE_FET3_INVPWM	0x18
#define OUT_STATE_FET4_ON		0x20



typedef struct
{
	uint32_t readOutsCnt;
	uint16_t cellVolt_max;
	uint16_t cellVolt_min;
	int16_t temp_max;		// unit = 0.1 degrees C
	int16_t temp_min;		// unit = 0.1 degrees C
	uint16_t batt_last_flt;
	int16_t batt_current;
	uint8_t moduleFaults;
	uint8_t moduleAlerts;
	uint8_t numModulesPresent;
	bool mains_present;
} batt_stats_t;


typedef struct
{
	uint32_t uart_ferr_cnt;
	uint32_t uart_ovf_cnt;
	uint32_t uart_timeout_cnt;
	uint32_t uart_buf_ovf_cnt;
	uint32_t uart_crcerr_cnt;
	uint32_t uart_dataerr_cnt;
	uint32_t uart_noreply_cnt;
	uint32_t uart_ok_cnt;
} batt_comm_stats_t;

typedef struct
{
	uint32_t i2c_transactions;
	uint32_t i2c_bus_err_cnt;
	uint32_t i2c_nack_addr;
	uint32_t i2c_nack_data;
	uint32_t i2c_unknown_irq;
	uint32_t i2c_stuck_sda_cycles;
	uint32_t i2c_stuck_scl_cycles;
} i2c_comm_stats_t;


extern batt_stats_t batt_stats;


extern BMSLimits bms_limits;

extern uint8_t sim_numFoundModules;
extern BMSModule sim_modules[SIM_MAX_MODULES];
extern uint32_t sim_data_readouts_done;
//extern bool simulatedValues;
extern uint16_t simKey;
extern int16_t sim_batt_current;


extern bool sim_status_wr_en;

extern uint16_t cont_lockout;

extern volatile batt_comm_stats_t batt_comm_stats;
extern volatile i2c_comm_stats_t i2c_comm_stats;
extern volatile uint32_t mainloop_cycles;

#endif /* GLOBALS_H_ */