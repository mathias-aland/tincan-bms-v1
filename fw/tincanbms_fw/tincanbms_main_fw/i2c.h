#ifndef I2C_H_INCLUDED
#define	I2C_H_INCLUDED

#include <avr/io.h>
#include <stdint.h>
#include <stddef.h>




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


typedef enum
{
    I2C_ERROR_NONE,             /* No Error */
    I2C_ERROR_ADDR_NACK,        /* Address Not Acknowledged */
    I2C_ERROR_DATA_NACK,        /* Data Not Acknowledged */
    I2C_ERROR_BUS_COLLISION,    /* Bus Collision Error */
    I2C_ERROR_BUSY              /* I2C Host is busy */
} i2c_error_t;

typedef struct
{
	int16_t offset;
	uint16_t fullscale;
} cal_entry_t;

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
} COMP_MCU_state_t;


typedef enum
{
	CONT_STATE_OFF = 0,
	CONT_STATE_ON,
	CONT_STATE_FAULT_HW,
	CONT_STATE_FAULT_ESTOP,
	CONT_STATE_FAULT_TIMEOUT,
	CONT_STATE_FAULT_PINLVL
} COMP_MCU_CONT_STATE_t;



extern COMP_MCU_state_t comp_mcu_state;

void        I2C_Init(void);
i2c_error_t I2C_Write(uint8_t address, uint8_t *pData, size_t dataLength);
i2c_error_t I2C_Read(uint8_t address, uint8_t *pData, size_t dataLength);
void i2c_loop();


bool set_outputs(uint8_t newState, uint8_t pwmFreq, uint8_t pwmDuty1, uint8_t pwmDuty2);
//bool clear_outputs(uint8_t bitmask);
bool contactor_enable(uint8_t val);


uint8_t comp_get_cont_state();

uint16_t comp_get_gpi(uint8_t gpio_num);

void I2C_systick_cb();

#endif	/* I2C_H_INCLUDED */
