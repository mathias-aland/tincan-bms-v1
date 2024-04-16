/*
 * cal.c
 *
 * Created: 02-05-2023 20:19:33
 *  Author: Mathias
 */ 

/* --- CAL DATA structure v2 ---
 * 0x000: CH1 OFFSET MSB
 * 0x001: CH1 OFFSET LSB
 * 0x002: CH2 OFFSET MSB
 * 0x003: CH2 OFFSET LSB
 * 0x004: CH3 OFFSET MSB
 * 0x005: CH3 OFFSET LSB
 * 0x006: CH4 OFFSET MSB
 * 0x007: CH4 OFFSET LSB
 * 0x008: CH5 OFFSET MSB
 * 0x009: CH5 OFFSET LSB
 * 0x00A: CH6 OFFSET MSB
 * 0x00B: CH6 OFFSET LSB
 * 0x00C: VDD OFFSET MSB
 * 0x00D: VDD OFFSET LSB
 * 0x00E: TEMP OFFSET MSB
 * 0x00F: TEMP OFFSET LSB
 * 0x010: CH1 FULLSCALE MSB
 * 0x011: CH1 FULLSCALE LSB
 * 0x012: CH2 FULLSCALE MSB
 * 0x013: CH2 FULLSCALE LSB
 * 0x014: CH3 FULLSCALE MSB
 * 0x015: CH3 FULLSCALE LSB
 * 0x016: CH4 FULLSCALE MSB
 * 0x017: CH4 FULLSCALE LSB
 * 0x018: CH5 FULLSCALE MSB
 * 0x019: CH5 FULLSCALE LSB
 * 0x01A: CH6 FULLSCALE MSB
 * 0x01B: CH6 FULLSCALE LSB
 * 0x01C: VDD FULLSCALE MSB
 * 0x01D: VDD FULLSCALE LSB
 * 0x01E: TEMP FULLSCALE MSB
 * 0x01F: TEMP FULLSCALE LSB
 */

#include <avr/io.h>
#include <avr/cpufunc.h>
#include <avr/eeprom.h>

#include "cal.h"




//cal_page_t calData = {
	//.version = 1,
	//.counter = 0,
	//.cal_data = {0},
	//.CRC = 0
//};


//cal_page_t calData = {
	//.version = 1,
	//.counter = 0,
	//.vdd.fullscale = 20480,
	//.vdd.offset = 0,
	//.curr_ref.fullscale = 4096,
	//.curr_ref.offset = 0,
	//.curr_sense.fullscale = 2048,
	//.curr_sense.offset = 0,
	//.v_supply.fullscale = 20480,
	//.v_supply.offset = 0,
	//.cont_mon.fullscale = 20480,
	//.cont_mon.offset = 0,
	//.estop_mon.fullscale = 20480,
	//.estop_mon.offset = 0,
	//.inp[0].fullscale = 20480,
	//.inp[0].offset = 0,
	//.inp[1].fullscale = 20480,
	//.inp[1].offset = 0,
	//.inp[2].fullscale = 20480,
	//.inp[2].offset = 0,
	//.inp[3].fullscale = 20480,
	//.inp[3].offset = 0,
	//.inp[4].fullscale = 20480,
	//.inp[4].offset = 0,
	//.inp[5].fullscale = 20480,
	//.inp[5].offset = 0,
	//.inp[6].fullscale = 20480,
	//.inp[6].offset = 0,
	//.inp[7].fullscale = 20480,
	//.inp[7].offset = 0,
	//.CRC = 0
	//};
	
//void initCal()
//{
	//for (uint8_t i=0; i < 8; i++)
	//{
		//calData.cal_data[ADC_IDX_IN1+i].fullscale = 20480;
		//calData.cal_data[ADC_IDX_IN1+i].offset = 0;
	//}
	//
	//calData.cal_data[ADC_IDX_V_SUPPLY].fullscale = 20480;
	//calData.cal_data[ADC_IDX_V_SUPPLY].offset = 0;
	//
	//calData.cal_data[ADC_IDX_CONT_MON].fullscale = 20480;
	//calData.cal_data[ADC_IDX_CONT_MON].offset = 0;
	//
	//calData.cal_data[ADC_IDX_ESTOP_MON].fullscale = 20480;
	//calData.cal_data[ADC_IDX_ESTOP_MON].offset = 0;
	//
	//calData.cal_data[ADC_IDX_CURR_REF].fullscale = 4096;
	//calData.cal_data[ADC_IDX_CURR_REF].offset = 0;
	//
	//calData.cal_data[ADC_IDX_CURR_DIAG].fullscale = 4096;
	//calData.cal_data[ADC_IDX_CURR_DIAG].offset = 0;
	//
	//calData.cal_data[ADC_IDX_CURR_SENSE].fullscale = 9830;	// A/10
	//calData.cal_data[ADC_IDX_CURR_SENSE].offset = 14;
	//
	//calData.cal_data[ADC_IDX_VDD].fullscale = 20480;
	//calData.cal_data[ADC_IDX_VDD].offset = 0;
	//
//}
	//
//cal_page_t* getCalData()
//{
	//return &calData;
//}
