/*
 * modbus.h
 *
 * Created: 10-09-2023 22:50:50
 *  Author: Mathias
 */ 


#ifndef MODBUS_H_
#define MODBUS_H_

#define MODBUS_FLAG_RXDONE	0x01

// coils and discrete inputs (bit access)
#define MODBUS_FUNC_READ_COILS			1
#define MODBUS_FUNC_READ_DINPUTS		2
#define MODBUS_FUNC_WRITE_COIL			5
#define MODBUS_FUNC_WRITE_COILS			15

// input and holding registers (16-bit access)
#define MODBUS_FUNC_READ_HOLDING_REGS	3
#define MODBUS_FUNC_READ_INPUT_REGS		4
#define MODBUS_FUNC_WRITE_HOLDING_REG	6
#define MODBUS_FUNC_WRITE_HOLDING_REGS	16

// file record access
#define MODBUS_FUNC_READFILE	20
#define MODBUS_FUNC_WRITEFILE	21


// exceptions
#define MODBUS_ERR_ILLEGAL_FUNC			1
#define MODBUS_ERR_ILLEGAL_DATA_ADDR	2
#define MODBUS_ERR_ILLEGAL_DATA_VAL		3


// modbus address map

#define MODBUS_COIL_FINDBATT		3
#define MODBUS_COIL_EESAVE			4
#define MODBUS_COIL_SIM_MODE		5
#define MODBUS_COIL_SIM_STATUS_WREN	6
#define MODBUS_COIL_ENABLE_BOOT		7

#define MODBUS_HOLDING_PARAMS			0
#define MODBUS_HOLDING_SIM_MODE_KEY		1000
#define MODBUS_HOLDING_SIM_MODULE_COUNT	1001
#define MODBUS_HOLDING_SIM_BATT_CURRENT	1002
#define MODBUS_HOLDING_SIM_MODULE_DATA	1003


#define MODBUS_INPUT_COMP_MCU_TEMP		10000
#define MODBUS_INPUT_COMP_V_SUPPLY		10001
#define MODBUS_INPUT_COMP_VDD			10002
#define MODBUS_INPUT_COMP_I_SENSE		10003
#define MODBUS_INPUT_COMP_CONT_MON		10004
#define MODBUS_INPUT_COMP_I_REF			10005
#define MODBUS_INPUT_COMP_I_DIAG		10006
#define MODBUS_INPUT_COMP_ESTOP_MON		10007
#define MODBUS_INPUT_COMP_IN1_V			10008
#define MODBUS_INPUT_COMP_IN2_V			10009
#define MODBUS_INPUT_COMP_IN3_V			10010
#define MODBUS_INPUT_COMP_IN4_V			10011
#define MODBUS_INPUT_COMP_IN5_V			10012
#define MODBUS_INPUT_COMP_IN6_V			10013
#define MODBUS_INPUT_COMP_IN7_V			10014
#define MODBUS_INPUT_COMP_IN8_V			10015
#define MODBUS_INPUT_COMP_OUT			10016
#define MODBUS_INPUT_COMP_CONT_STATE	10017


#define MODBUS_INPUT_CONT_LOCKOUT		10018


//#define  MODBUS_INPUT_BATT_COMM_STATS	11000

#define  MODBUS_INPUT_BATT_COMM_OK_CNT		11000
#define  MODBUS_INPUT_BATT_COMM_CRCERR_CNT	11002
#define  MODBUS_INPUT_BATT_COMM_DATAERR_CNT	11004
#define  MODBUS_INPUT_BATT_COMM_FERR_CNT	11006
#define  MODBUS_INPUT_BATT_COMM_OVF_CNT		11008
#define  MODBUS_INPUT_BATT_COMM_BUF_OVF_CNT	11010
#define  MODBUS_INPUT_BATT_COMM_TIMEOUT_CNT	11012
#define  MODBUS_INPUT_BATT_COMM_NOREPLY_CNT	11014


#define  MODBUS_INPUT_I2C_TRANSACTIONS		11016
#define  MODBUS_INPUT_I2C_BUS_ERR_CNT		11018
#define  MODBUS_INPUT_I2C_NACK_ADDR_CNT		11020
#define  MODBUS_INPUT_I2C_NACK_DATA_CNT		11022
#define  MODBUS_INPUT_I2C_UNKNOWN_IRQ_CNT	11024
#define  MODBUS_INPUT_I2C_STUCK_SDA_CYCLES	11026
#define  MODBUS_INPUT_I2C_STUCK_SCL_CYCLES	11028

#define  MODBUS_INPUT_MAINLOOP_CYCLES		11030
#define  MODBUS_INPUT_SYSTICK				11032

void startModbus();

void modbus_state_machine();

#endif /* MODBUS_H_ */