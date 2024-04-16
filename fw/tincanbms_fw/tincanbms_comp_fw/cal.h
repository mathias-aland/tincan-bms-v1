/*
 * cal.h
 *
 * Created: 02-05-2023 23:51:15
 *  Author: Mathias
 */ 


#ifndef CAL_H_
#define CAL_H_


typedef struct
{
	int16_t offset;
	uint16_t fullscale;
} cal_entry_t;



typedef struct
{
	uint8_t version;
	uint8_t counter;
	cal_entry_t cal_data[15];
	uint16_t crc;		
} cal_eepage_t;


//typedef struct
//{
	//uint8_t version;
	//uint8_t counter;
	//cal_entry_t vdd;
	//cal_entry_t curr_ref;
	//cal_entry_t curr_sense;
	//cal_entry_t v_supply;
	//cal_entry_t cont_mon;
	//cal_entry_t estop_mon;
	//cal_entry_t inp[8];
	//uint16_t CRC;
//} cal_page_t;



typedef enum
{
	ADC_IDX_VDD = 0,
	ADC_IDX_IN1,
	ADC_IDX_IN2,
	ADC_IDX_IN3,
	ADC_IDX_IN4,
	ADC_IDX_IN5,
	ADC_IDX_IN6,
	ADC_IDX_IN7,
	ADC_IDX_IN8,
	ADC_IDX_V_SUPPLY,
	ADC_IDX_CONT_MON,
	ADC_IDX_ESTOP_MON,
	ADC_IDX_CURR_REF,	
	ADC_IDX_CURR_DIAG,
	ADC_IDX_CURR_SENSE
} ADC_IDX_t;






//void initCal();
//cal_page_t* getCalData();

#endif /* CAL_H_ */