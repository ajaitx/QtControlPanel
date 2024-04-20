#ifndef RELAY8_H_
#define RELAY8_H_

#include <stdint.h>

#define ADC_RAW_VAL_SIZE	2
#define DAC_MV_VAL_SIZE		2
#define VOLT_TO_MILIVOLT	1000
#define OPTO_CH_NO 8
#define GPIO_CH_NO 4
#define COUNTER_SIZE 4
#define DRY_CONTACT_COUNT	8
#define DRY_CONTACT_CONTOR_SIZE 4

#define RETRY_TIMES	10
#define CALIBRATION_KEY 0xaa
#define RESET_CALIBRATION_KEY	0x55 
#define WDT_RESET_SIGNATURE 	0xCA
#define WDT_MAX_OFF_INTERVAL_S 4147200 //48 days
#define UI_VAL_SIZE	2
#define CAN_FIFO_SIZE 10
#define OWB_SENS_CNT 16
#define OWB_TEMP_SIZE_B 2

enum
{
	I2C_MEM_RELAY_VAL = 0,//reserved 4 bits for open-drain and 4 bits for leds
	I2C_MEM_RELAY_SET,
	I2C_MEM_RELAY_CLR,
	I2C_MEM_OPTO_IN_VAL,

	I2C_MEM_U0_10_OUT_VAL1,
	I2C_MEM_U0_10_OUT_VAL2 = I2C_MEM_U0_10_OUT_VAL1 + UI_VAL_SIZE,
	I2C_MEM_U0_10_OUT_VAL3 = I2C_MEM_U0_10_OUT_VAL2 + UI_VAL_SIZE,
	I2C_MEM_U0_10_OUT_VAL4 = I2C_MEM_U0_10_OUT_VAL3 + UI_VAL_SIZE,
	I2C_MEM_I4_20_OUT_VAL1 = I2C_MEM_U0_10_OUT_VAL4 + UI_VAL_SIZE,
	I2C_MEM_I4_20_OUT_VAL2 = I2C_MEM_I4_20_OUT_VAL1 + UI_VAL_SIZE,
	I2C_MEM_I4_20_OUT_VAL3 = I2C_MEM_I4_20_OUT_VAL2 + UI_VAL_SIZE,
	I2C_MEM_I4_20_OUT_VAL4 = I2C_MEM_I4_20_OUT_VAL3 + UI_VAL_SIZE,
	I2C_MEM_OD_PWM1 = I2C_MEM_I4_20_OUT_VAL4 + UI_VAL_SIZE,
	I2C_MEM_OD_PWM2 = I2C_MEM_OD_PWM1 + UI_VAL_SIZE,
	I2C_MEM_OD_PWM3 = I2C_MEM_OD_PWM2 + UI_VAL_SIZE,
	I2C_MEM_OD_PWM4 = I2C_MEM_OD_PWM3 + UI_VAL_SIZE,

	I2C_MEM_U0_10_IN_VAL1 = I2C_MEM_OD_PWM4 + UI_VAL_SIZE,
	I2C_MEM_U0_10_IN_VAL2 = I2C_MEM_U0_10_IN_VAL1 + UI_VAL_SIZE,
	I2C_MEM_U0_10_IN_VAL3 = I2C_MEM_U0_10_IN_VAL2 + UI_VAL_SIZE,
	I2C_MEM_U0_10_IN_VAL4 = I2C_MEM_U0_10_IN_VAL3 + UI_VAL_SIZE,
	I2C_MEM_U_PM_10_IN_VAL1 = I2C_MEM_U0_10_IN_VAL4 + UI_VAL_SIZE,
	I2C_MEM_U_PM_10_IN_VAL2 = I2C_MEM_U_PM_10_IN_VAL1 + UI_VAL_SIZE,
	I2C_MEM_U_PM_10_IN_VAL3 = I2C_MEM_U_PM_10_IN_VAL2 + UI_VAL_SIZE,
	I2C_MEM_U_PM_10_IN_VAL4 = I2C_MEM_U_PM_10_IN_VAL3 + UI_VAL_SIZE,
	I2C_MEM_I4_20_IN_VAL1 = I2C_MEM_U_PM_10_IN_VAL4 + UI_VAL_SIZE,
	I2C_MEM_I4_20_IN_VAL2 = I2C_MEM_I4_20_IN_VAL1 + UI_VAL_SIZE,
	I2C_MEM_I4_20_IN_VAL3 = I2C_MEM_I4_20_IN_VAL2 + UI_VAL_SIZE,
	I2C_MEM_I4_20_IN_VAL4 = I2C_MEM_I4_20_IN_VAL3 + UI_VAL_SIZE,

	I2C_MEM_I4_20_OUT_VALID1 = I2C_MEM_I4_20_IN_VAL4 + UI_VAL_SIZE,
	I2C_MEM_I4_20_OUT_VALID2 = I2C_MEM_I4_20_OUT_VALID1 + UI_VAL_SIZE,
	I2C_MEM_I4_20_OUT_VALID3 = I2C_MEM_I4_20_OUT_VALID2 + UI_VAL_SIZE,
	I2C_MEM_I4_20_OUT_VALID4 = I2C_MEM_I4_20_OUT_VALID3 + UI_VAL_SIZE,
	I2C_MEM_CALIB_VALUE = 60,
	I2C_MEM_CALIB_CHANNEL = I2C_MEM_CALIB_VALUE + 2, //0-10V out [1,4]; 0-10V in [5, 12]; R 1K in [13, 20]; R 10K in [21, 28]
	I2C_MEM_CALIB_KEY, //set calib point 0xaa; reset calibration on the channel 0x55
	I2C_MEM_CALIB_STATUS,
	I2C_MODBUS_SETINGS_ADD = 65,
	I2C_NBS1,
	I2C_MBS2,
	I2C_MBS3,
	I2C_MODBUS_ID_OFFSET_ADD,
	I2C_RTC_YEAR_ADD,
	I2C_RTC_MONTH_ADD,
	I2C_RTC_DAY_ADD,
	I2C_RTC_HOUR_ADD,
	I2C_RTC_MINUTE_ADD,
	I2C_RTC_SECOND_ADD,
	I2C_RTC_SET_YEAR_ADD,
	I2C_RTC_SET_MONTH_ADD,
	I2C_RTC_SET_DAY_ADD,
	I2C_RTC_SET_HOUR_ADD,
	I2C_RTC_SET_MINUTE_ADD,
	I2C_RTC_SET_SECOND_ADD,
	I2C_RTC_CMD_ADD,
	I2C_MEM_WDT_RESET_ADD,
	I2C_MEM_WDT_INTERVAL_SET_ADD,
	I2C_MEM_WDT_INTERVAL_GET_ADD = I2C_MEM_WDT_INTERVAL_SET_ADD + 2,
	I2C_MEM_WDT_INIT_INTERVAL_SET_ADD = I2C_MEM_WDT_INTERVAL_GET_ADD + 2,
	I2C_MEM_WDT_INIT_INTERVAL_GET_ADD = I2C_MEM_WDT_INIT_INTERVAL_SET_ADD + 2,
	I2C_MEM_WDT_RESET_COUNT_ADD = I2C_MEM_WDT_INIT_INTERVAL_GET_ADD + 2,
	I2C_MEM_WDT_CLEAR_RESET_COUNT_ADD = I2C_MEM_WDT_RESET_COUNT_ADD + 2,
	I2C_MEM_WDT_POWER_OFF_INTERVAL_SET_ADD,
	I2C_MEM_WDT_POWER_OFF_INTERVAL_GET_ADD = I2C_MEM_WDT_POWER_OFF_INTERVAL_SET_ADD
		+ 4,
	I2C_MEM_OPTO_RISING_ENABLE = I2C_MEM_WDT_POWER_OFF_INTERVAL_GET_ADD + 4,
	I2C_MEM_OPTO_FALLING_ENABLE,
	I2C_MEM_OPTO_CH_CONT_RESET,
	I2C_MEM_OPTO_COUNT1,
	I2C_MEM_OPTO_COUNT2 = I2C_MEM_OPTO_COUNT1 + 2,
	I2C_MEM_OPTO_COUNT3 = I2C_MEM_OPTO_COUNT2 + 2,
	I2C_MEM_OPTO_COUNT4 = I2C_MEM_OPTO_COUNT3 + 2,
	I2C_MEM_DIAG_TEMPERATURE = 0x72,
	I2C_MEM_DIAG_24V,
	I2C_MEM_DIAG_24V_1,
	I2C_MEM_DIAG_5V,
	I2C_MEM_DIAG_5V_1,
	CAN_REC_MPS_MEM,

	I2C_MEM_REVISION_MAJOR = 0x78,
	I2C_MEM_REVISION_MINOR,
	I2C_MEM_CAN_TX_FIFO_LEVEL,
	I2C_MEM_CAN_TX_FIFO,
	I2C_MEM_CAN_RX_FIFO_LEVEL = I2C_MEM_CAN_TX_FIFO + CAN_FIFO_SIZE,
	I2C_MEM_CAN_RX_FIFO,
	I2C_MEM_CAN_RX_FIFO_MARK = I2C_MEM_CAN_RX_FIFO + CAN_FIFO_SIZE,
	I2C_MEM_DIAG_3V,
	I2C_MEM_DIAG_3V_1,
	I2C_MEM_1WB_DEV,
	I2C_MEM_1WB_TEMP_ALL,
	I2C_MEM_1WB_ROM_CODE_IDX = I2C_MEM_1WB_TEMP_ALL + OWB_TEMP_SIZE_B,
	I2C_MEM_1WB_ROM_CODE,//rom code 64 bits
	I2C_MEM_1WB_ROM_CODE_END = I2C_MEM_1WB_ROM_CODE + 7,



	I2C_MEM_CPU_RESET = 0xaa,
	I2C_MEM_HSI_LO,
	I2C_MEM_HSI_HI,
	I2C_MEM_1WB_START_SEARCH,
	I2C_MEM_1WB_T1,
	I2C_MEM_1WB_T16 = I2C_MEM_1WB_T1 + OWB_SENS_CNT * OWB_TEMP_SIZE_B,
	I2C_MEM_SIZE = 255
	//SLAVE_BUFF_SIZE = 255
};

enum CAL_CH_START_ID{
	CAL_0_10V_OUT_START_ID = 1,
	CAL_0_10V_OUT_STOP_ID = 4,
	CAL_4_20mA_OUT_START_ID,
	CAL_4_20mA_OUT_STOP_ID = 8,
	CAL_0_10V_IN_START_ID,
	CAL_0_10V_IN_STOP_ID = 12,
	CAL_PM_10V_IN_START_ID,
	CAL_PM_10V_IN_STOP_ID = 16,
	CAL_4_20mA_IN_START_ID,
	CAL_4_20mA_IN_STOP_ID = 24,
};

#define CHANNEL_NR_MIN		1
#define RELAY_CH_NR_MAX		4

#define OPTO_CH_NR_MAX		4
#define OD_CH_NR_MAX			4
#define I_OUT_CH_NR_MAX		4
#define U_OUT_CH_NR_MAX		4
#define U_IN_CH_NR_MAX		4
#define I_IN_CH_NR_MAX		4
#define LED_CH_NR_MAX		4

#define OD_PWM_VAL_MAX	10000

#define ERROR	-1
#define OK		0
#define FAIL	-1
#define ARG_CNT_ERR -3
#define COMM_ERR -4

#define SLAVE_OWN_ADDRESS_BASE 0x50

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;

typedef struct
	__attribute__((packed))
	{
		unsigned int mbBaud :24;
		unsigned int mbType :4;
		unsigned int mbParity :2;
		unsigned int mbStopB :2;
		unsigned int add:8;
	} ModbusSetingsType;
	
	
int doBoardInit(int stack);
		
#endif //RELAY8_H_