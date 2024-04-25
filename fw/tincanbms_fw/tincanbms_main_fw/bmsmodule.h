/*
 * bmsmodule.h
 *
 *  Created on: 28 juni 2022
 *      Author: Mathias
 */

#ifndef INC_BMSMODULE_H_
#define INC_BMSMODULE_H_


//#define MAX_MODULE_ADDR     62

//#define MAX_MODULE_ADDR     20

#define MAX_MODULES     16


#define ALERT_AR        0x80
#define ALERT_PARITY    0x40
#define ALERT_ECC_ERR   0x20
#define ALERT_FORCE     0x10
#define ALERT_TSD       0x08
#define ALERT_SLEEP     0x04
#define ALERT_OT2       0x02
#define ALERT_OT1       0x01

#define FAULT_SELFTEST  0x20
#define FAULT_FORCE     0x10
#define FAULT_POR       0x08
#define FAULT_CRC       0x04
#define FAULT_CUV       0x02
#define FAULT_COV       0x01




#define BATT_ADDR_BROADCAST	0x3f

#define BATT_ALERT_AR		0x80
#define BATT_ALERT_PARITY	0x40
#define BATT_ALERT_ECC_ERR	0x20
#define BATT_ALERT_FORCE	0x10
#define BATT_ALERT_TSD		0x08
#define BATT_ALERT_SLEEP	0x04
#define BATT_ALERT_OT2		0x02
#define BATT_ALERT_OT1		0x01

#define BATT_FAULT_NOCOMM	0x40
#define BATT_FAULT_SELFTEST	0x20
#define BATT_FAULT_FORCE	0x10
#define BATT_FAULT_POR		0x08
#define BATT_FAULT_CRC		0x04
#define BATT_FAULT_CUV		0x02
#define BATT_FAULT_COV		0x01

#define BATT_STATUS_ADDR_RQST	0x80
#define BATT_STATUS_FAULT		0x40
#define BATT_STATUS_ALERT		0x20
#define BATT_STATUS_ECC_COR		0x08
#define BATT_STATUS_UVLO		0x04
#define BATT_STATUS_CBT			0x02
#define BATT_STATUS_DRDY		0x01


#define BQ_REG_DEV_STATUS      0x00
#define BQ_REG_GPAI            0x01
#define BQ_REG_VCELL1          0x03
#define BQ_REG_VCELL2          0x05
#define BQ_REG_VCELL3          0x07
#define BQ_REG_VCELL4          0x09
#define BQ_REG_VCELL5          0x0B
#define BQ_REG_VCELL6          0x0D
#define BQ_REG_TEMPERATURE1    0x0F
#define BQ_REG_TEMPERATURE2    0x11
#define BQ_REG_ALERT_STATUS    0x20
#define BQ_REG_FAULT_STATUS    0x21
#define BQ_REG_COV_FAULT       0x22
#define BQ_REG_CUV_FAULT       0x23
#define BQ_REG_ADC_CTRL        0x30
#define BQ_REG_IO_CTRL         0x31
#define BQ_REG_BAL_CTRL        0x32
#define BQ_REG_BAL_TIME        0x33
#define BQ_REG_ADC_CONV        0x34
#define BQ_REG_ADDR_CTRL       0x3B
#define BQ_REG_RESET	       0x3C

#define BQ_IO_CTRL_SLEEP_BIT	0x04




enum
{
	BATT_OP_NONE = 0,
	BATT_OP_RESET,
	BATT_OP_CHECKADDR,
	BATT_OP_SETADDR,
	BATT_OP_IDLE
};

enum
{
	BATT_OP_STATE_NONE = 0,
	BATT_OP_STATE_BUSY,
	BATT_OP_STATE_OK,
	BATT_OP_STATE_ERR
};



enum
{
	BATT_STATE_IDLE = 0,
	BATT_STATE_GETVALUES,
	BATT_STATE_GETVALUES_ENABLE_TEMP,
	BATT_STATE_GETVALUES_ENABLE_ADC,
	BATT_STATE_GETVALUES_START_ADC,
	BATT_STATE_GETVALUES_DISABLE_TEMP,
	BATT_STATE_GETVALUES_READ_BALSTATE,
	BATT_STATE_GETVALUES_READ1,
	BATT_STATE_GETVALUES_READ2,
	BATT_STATE_INIT,
	BATT_STATE_INIT_RESET,
	BATT_STATE_INIT_CHECK_UNASSIGNED,
	BATT_STATE_INIT_ASSIGN_ADDR,
	BATT_STATE_INIT_TEST,
	BATT_STATE_ENABLE_SLEEP,
	BATT_STATE_DISABLE_SLEEP,
	BATT_STATE_RESET_ALARMS,
	BATT_STATE_RESET_ALARMS_2,
	BATT_STATE_RESET_ALARMS_3,
	BATT_STATE_RESET_ALARMS_4,
	BATT_STATE_RESET_ALARMS_5,
	BATT_STATE_SET_THRESHOLDS,
	BATT_STATE_SET_BALANCE,
	BATT_STATE_SET_BALANCE_2,
	BATT_STATE_SET_BALANCE_3,
	BATT_STATE_SET_BALANCE_4
};




#define BATT_FLT_NO_MODULES	0x0001
#define BATT_FLT_UVP		0x0002
#define BATT_FLT_OVP		0x0004
#define BATT_FLT_UTP		0x0008
#define BATT_FLT_OTP		0x0010
#define BATT_FLT_FAULT		0x0020
#define BATT_FLT_ALERT		0x0040
#define BATT_FLT_COMM		0x0080
#define BATT_FLT_HW			0x0100
#define BATT_FLT_ESTOP		0x0200
#define BATT_FLT_TIMEOUT	0x0400
#define BATT_FLT_OTHER		0x8000


//typedef enum {
//
	//BATT_FLT_NONE = 0,
	//BATT_FLT_NO_MODULES,
	//BATT_FLT_UVP,
	//BATT_FLT_OVP,
	//BATT_FLT_UTP,
	//BATT_FLT_OTP,
	//BATT_FLT_FAULT,
	//BATT_FLT_ALERT,
	//BATT_FLT_COMM,
	//BATT_FLT_OTHER,
//
//} batt_flt_t;


typedef enum {

	GPO_FUNC_NONE = 0,
	GPO_FUNC_ALARM_MINOR,
	GPO_FUNC_ALARM_MAJOR,
	GPO_FUNC_CHECK_ENGINE,
	GPO_FUNC_BATT_HEATER,
	GPO_FUNC_BATT_PUMP,
	GPO_FUNC_BATT_PUMP_PWM,
	GPO_FUNC_CHG_ENABLE,
	GPO_FUNC_CHG_INTERLOCK,
	GPO_FUNC_DRIVE_ENABLE,
	GPO_FUNC_CHG_STATUS,
	GPO_FUNC_CONT_STATUS,
	GPO_FUNC_TURTLE_MODE,
	GPO_FUNC_REGEN_LOCKOUT
} gpo_func_t;

#define GPO_FLAG_INVERT	0x80

//typedef struct
//{
	//uint8_t op;
	//uint8_t op_request;
	//uint8_t op_state;
	//uint8_t op_step;
	//uint8_t curr_addr;
	//uint8_t retry_cnt;
//} batt_state_t;





typedef struct
{
	uint16_t cellVolt[6];			// voltage in millivolts, calculated as 16 bit value * 6.250 / 16383 = volts
	uint16_t moduleVolt;			// summed cell voltages in millivolts
	uint16_t retmoduleVolt;			// voltage in millivolts, calculated as 16 bit value * 33.333 / 16383 = volts
	int16_t temperatures[2];		// unit = 0.1 degrees C
	uint8_t status;
	uint8_t alerts;
	uint8_t faults;
	uint8_t COVFaults;
	uint8_t CUVFaults;
	uint8_t balState;
} BMSModule;

typedef struct
{
	uint16_t version;
	uint16_t params_start;
	uint16_t params_length;
	uint16_t params_CRC16;
	uint16_t reserved_1;
	uint16_t reserved_2;
	uint16_t reserved_3;
	uint16_t header_CRC16;
} EE_header;

typedef struct
{
		/* --- 16-bit --- */
		
		/* Contactor lockout */
		uint16_t cellVoltHi;
        uint16_t cellVoltLo;
        int16_t tempHi;		
        int16_t tempLo;
		int16_t minCurrent;
		int16_t minCurrentPeak;
		int16_t maxCurrent;
		int16_t maxCurrentPeak;
		
		/* Contactor control */
		uint16_t contactorRelayTime;		
		uint16_t contactorPrechargeTime;
		uint16_t contactorOnPulseTime;
		

		/* Turtle mode */
		uint16_t turtlemodeVolt;
		
		/* Charger control */
        uint16_t chgDisVolt;			
        uint16_t chgEnVolt;
		uint16_t chgInhibitVolt;
		int16_t chgMinTemp;
		int16_t chgMaxTemp;
		
		/* Regen lockout */
		int16_t regenMinTemp;
		int16_t regenMaxTemp;
		uint16_t regenMaxVolt;
		
		/* Heater & pump control */
        uint16_t pumpOffSeconds;			
        uint16_t pumpOnSeconds;		
        int16_t pumpOnTemp;		
			
        int16_t heaterOffTemp;	
		int16_t heaterOnTemp;			
		
		/* Cell balancing */
		uint16_t balMinVoltage;
		
		/* GPIO */
		uint16_t ignThresholdOn;
		uint16_t ignThresholdOff;
		uint16_t startThresholdOn;
		uint16_t startThresholdOff;
		uint16_t acPresThresholdOn;
		uint16_t acPresThresholdOff;
		
		/* --- 8-bit, 16-bit for now to make modbus mapping easier --- */
		
		/* Contactor lockout */
		uint16_t peakCurrentDelay;		
        uint16_t contactorOffDelay;		
		uint16_t packsExpected;
		
		/* Contactor control */
		uint16_t contactorPwmFrequency;
		uint16_t contactorPwmDuty;
		uint16_t contactorIdleDelay;		
		
		/* Turtle mode */
		uint16_t turtlemodeDelay;
		
		/* Charger control */
		uint16_t chgDelay;
		uint16_t acPresDelay;
		
		/* Regen lockout */
		uint16_t regenResetDelay;
		
		/* Heater & pump control */
		uint16_t pumpFreqPWM;
		uint16_t pumpDutyPWM;		
		
		/* Cell balancing */
		uint16_t balMaxDelta;
		uint16_t balFinishDelta;		
		uint16_t balInterval;
		uint16_t balIdleTime;
		
		

		/* Misc */

		
        uint16_t alertFaultMask;
        //uint16_t faultMask;
		
		uint16_t ignGPI;
		uint16_t startGPI;
		uint16_t acPresGPI;
		
		uint16_t gpoFunc[6];
		//uint16_t gpoFlags[6];
		
		/* Secondary protection */
		uint16_t secCOVreg;
		uint16_t secCUVreg;
		uint16_t secOTreg;
		
} BMSLimits;


typedef struct
{
	uint32_t ok_cnt;
	uint32_t crcerr_cnt;
	uint32_t ferr_cnt;
	uint32_t ovf_cnt;
	uint32_t timeout_cnt;
} comm_stats_t;


enum
{
	BATT_CMD_NONE = 0,
	BATT_CMD_CLEAR_FAULTS,
	BATT_CMD_CLEAR_ALERTS
};

enum
{
	BATT_CMD_RESULT_OK = 0,
	BATT_CMD_RESULT_PENDING,
	BATT_CMD_RESULT_FAIL
};

typedef struct
{
	uint8_t cmd;
	uint8_t addr;	
	uint8_t data;
	uint8_t result;
} batt_cmd_t;


void BATT_systick_cb();

void bms_state_machine();

bool sendData2();
bool startRX(uint8_t len, uint16_t timeout);

void BMSModuleManager_renumberBoardIDs();
void BMSModuleManager_findBoards();
void BMSModuleManager_setupBoards();
void BMSModule_readStatus(uint8_t moduleAddress);
bool BMSModule_readModuleValues(uint8_t moduleAddress);
void BMSModule_clearFaults(uint8_t moduleAddress);


void batt_req_init();
bool batt_status_init();
bool batt_result_init();

void batt_req_clear(uint8_t alertMask, uint8_t faultMask);
bool batt_status_clear();
bool batt_result_clear();

//void batt_req_setbal(uint8_t pack, uint8_t cells, uint8_t timeout);
//bool batt_bal_pend();
//bool batt_bal_ok();


void batt_set_balAllowed(bool balAllowed);

uint16_t batt_get_module_volt(uint8_t modNum);
uint16_t batt_get_cell_volt(uint8_t modNum, uint8_t cellNum);
int16_t batt_get_temperature(uint8_t modNum, uint8_t tempNum);
uint8_t batt_get_module_cnt();



uint8_t batt_get_module_alerts(uint8_t modNum);
uint8_t batt_get_module_faults(uint8_t modNum);
uint8_t batt_get_module_status(uint8_t modNum);
uint8_t batt_get_module_bal(uint8_t modNum);


uint32_t batt_get_total_readouts();






extern uint8_t numFoundModules;
extern volatile BMSModule modules[MAX_MODULES];

extern volatile comm_stats_t comm_stats;
//extern volatile batt_state_t batt_state;
//extern volatile uint8_t batt_state;
extern volatile batt_cmd_t batt_cmd;

#endif /* INC_BMSMODULE_H_ */
