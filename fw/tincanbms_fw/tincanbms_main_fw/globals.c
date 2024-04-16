/*
 * globals.c
 *
 * Created: 04-04-2024 15:01:20
 *  Author: Mathias
 */ 
#include "clkctrl.h"
#include "globals.h"

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

batt_stats_t batt_stats;



BMSLimits bms_limits;

uint16_t cont_lockout = 0;

uint8_t sim_numFoundModules = 0;
int16_t sim_batt_current = 0;

BMSModule sim_modules[SIM_MAX_MODULES]; // store data for as many modules as we've configured for.
uint32_t sim_data_readouts_done = 0;
uint16_t simKey = 0;
bool sim_status_wr_en = false;

