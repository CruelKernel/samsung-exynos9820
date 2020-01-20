/*
 * Platform data for the NXP PCA9468 battery charger driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _PCA9468_CHARGER_H_
#define _PCA9468_CHARGER_H_

#define BITS(_end, _start) ((BIT(_end) - BIT(_start)) + BIT(_end))
#define MASK2SHIFT(_mask)	__ffs(_mask)
#define MIN(a, b)	   ((a < b) ? (a):(b))

//
// Register Map
//
#define PCA9468_REG_DEVICE_INFO 		0x00	// Device ID, revision
#define PCA9468_BIT_DEV_REV				BITS(7,4)
#define PCA9468_BIT_DEV_ID				BITS(3,0)
#define PCA9468_DEVICE_ID				0x18	// Default ID

#define PCA9468_REG_INT1				0x01	// Interrupt register
#define PCA9468_BIT_V_OK_INT			BIT(7)
#define PCA9468_BIT_NTC_TEMP_INT		BIT(6)
#define PCA9468_BIT_CHG_PHASE_INT		BIT(5)
#define PCA9468_BIT_CTRL_LIMIT_INT		BIT(3)
#define PCA9468_BIT_TEMP_REG_INT		BIT(2)
#define PCA9468_BIT_ADC_DONE_INT		BIT(1)
#define PCA9468_BIT_TIMER_INT			BIT(0)

#define PCA9468_REG_INT1_MSK			0x02	// INT mask for INT1 register
#define PCA9468_BIT_V_OK_M				BIT(7)
#define PCA9468_BIT_NTC_TEMP_M			BIT(6)
#define PCA9468_BIT_CHG_PHASE_M			BIT(5)
#define PCA9468_BIT_RESERVED_M			BIT(4)
#define PCA9468_BIT_CTRL_LIMIT_M		BIT(3)
#define PCA9468_BIT_TEMP_REG_M			BIT(2)
#define PCA9468_BIT_ADC_DONE_M			BIT(1)
#define PCA9468_BIT_TIMER_M				BIT(0)

#define PCA9468_REG_INT1_STS			0x03	// INT1 status regsiter
#define PCA9468_BIT_V_OK_STS			BIT(7)
#define PCA9468_BIT_NTC_TEMP_STS		BIT(6)
#define PCA9468_BIT_CHG_PHASE_STS		BIT(5)
#define PCA9468_BIT_CTRL_LIMIT_STS		BIT(3)
#define PCA9468_BIT_TEMP_REG_STS		BIT(2)
#define PCA9468_BIT_ADC_DONE_STS		BIT(1)
#define PCA9468_BIT_TIMER_STS			BIT(0)

#define PCA9468_REG_STS_A				0x04	// INT1 status register
#define PCA9468_BIT_IIN_LOOP_STS		BIT(7)
#define PCA9468_BIT_CHG_LOOP_STS		BIT(6)
#define PCA9468_BIT_VFLT_LOOP_STS		BIT(5)
#define PCA9468_BIT_CFLY_SHORT_STS		BIT(4)
#define PCA9468_BIT_VOUT_UV_STS			BIT(3)
#define PCA9468_BIT_VBAT_OV_STS			BIT(2)
#define PCA9468_BIT_VIN_OV_STS			BIT(1)
#define PCA9468_BIT_VIN_UV_STS			BIT(0)

#define PCA9468_REG_STS_B				0x05	// INT1 status register
#define PCA9468_BIT_BATT_MISS_STS		BIT(7)
#define PCA9468_BIT_OCP_FAST_STS		BIT(6)
#define PCA9468_BIT_OCP_AVG_STS			BIT(5)
#define PCA9468_BIT_ACTIVE_STATE_STS	BIT(4)
#define PCA9468_BIT_SHUTDOWN_STATE_STS	BIT(3)
#define PCA9468_BIT_STANDBY_STATE_STS	BIT(2)
#define PCA9468_BIT_CHARGE_TIMER_STS	BIT(1)
#define PCA9468_BIT_WATCHDOG_TIMER_STS	BIT(0)

#define PCA9468_REG_STS_C				0x06	// IIN status
#define PCA9468_BIT_IIN_STS				BITS(7,2)

#define PCA9468_REG_STS_D				0x07	// ICHG status
#define PCA9468_BIT_ICHG_STS			BITS(7,1)

#define PCA9468_REG_STS_ADC_1			0x08	// ADC register
#define PCA9468_BIT_ADC_IIN7_0			BITS(7,0)

#define PCA9468_REG_STS_ADC_2			0x09	// ADC register
#define PCA9468_BIT_ADC_IOUT5_0			BITS(7,2)
#define PCA9468_BIT_ADC_IIN9_8			BITS(1,0)

#define PCA9468_REG_STS_ADC_3			0x0A	// ADC register
#define PCA9468_BIT_ADC_VIN3_0			BITS(7,4)
#define PCA9468_BIT_ADC_IOUT9_6			BITS(3,0)

#define PCA9468_REG_STS_ADC_4			0x0B	// ADC register
#define PCA9468_BIT_ADC_VOUT1_0			BITS(7,6)
#define PCA9468_BIT_ADC_VIN9_4			BITS(5,0)

#define PCA9468_REG_STS_ADC_5			0x0C	// ADC register
#define PCA9468_BIT_ADC_VOUT9_2			BITS(7,0)

#define PCA9468_REG_STS_ADC_6			0x0D	// ADC register
#define PCA9468_BIT_ADC_VBAT7_0			BITS(7,0)

#define PCA9468_REG_STS_ADC_7			0x0E	// ADC register
#define PCA9468_BIT_ADC_DIETEMP5_0		BITS(7,2)
#define PCA9468_BIT_ADC_VBAT9_8			BITS(1,0)

#define PCA9468_REG_STS_ADC_8			0x0F	// ADC register
#define PCA9468_BIT_ADC_NTCV3_0			BITS(7,4)
#define PCA9468_BIT_ADC_DIETEMP9_6		BITS(3,0)

#define PCA9468_REG_STS_ADC_9			0x10	// ADC register
#define PCA9468_BIT_ADC_NTCV9_4			BITS(5,0)

#define PCA9468_REG_ICHG_CTRL			0x20	// Change current configuration
#define PCA9468_BIT_ICHG_SS				BIT(7)
#define PCA9468_BIT_ICHG_CFG			BITS(6,0)

#define PCA9468_REG_IIN_CTRL			0x21	// Input current configuration
#define PCA9468_BIT_LIMIT_INCREMENT_EN	BIT(7)
#define PCA9468_BIT_IIN_SS				BIT(6)
#define PCA9468_BIT_IIN_CFG				BITS(5,0)

#define PCA9468_REG_START_CTRL			0x22	// Device initialization configuration
#define PCA9468_BIT_SNSRES				BIT(7)
#define PCA9468_BIT_EN_CFG				BIT(6)
#define PCA9468_BIT_STANDBY_EN			BIT(5)
#define PCA9468_BIT_REV_IIN_DET			BIT(4)
#define PCA9468_BIT_FSW_CFG				BITS(3,0)

#define PCA9468_REG_ADC_CTRL			0x23	// ADC configuration
#define PCA9468_BIT_FORCE_ADC_MODE		BITS(7,6)
#define PCA9468_BIT_ADC_SHUTDOWN_CFG	BIT(5)
#define PCA9468_BIT_HIBERNATE_DELAY		BITS(4,3)

#define PCA9468_REG_ADC_CFG				0x24	// ADC channel configuration
#define PCA9468_BIT_CH7_EN				BIT(7)
#define PCA9468_BIT_CH6_EN				BIT(6)
#define PCA9468_BIT_CH5_EN				BIT(5)
#define PCA9468_BIT_CH4_EN				BIT(4)
#define PCA9468_BIT_CH3_EN				BIT(3)
#define PCA9468_BIT_CH2_EN				BIT(2)
#define PCA9468_BIT_CH1_EN				BIT(1)

#define PCA9468_REG_TEMP_CTRL			0x25	// Temperature configuration
#define PCA9468_BIT_TEMP_REG			BITS(7,6)
#define PCA9468_BIT_TEMP_DELTA			BITS(5,4)
#define PCA9468_BIT_TEMP_REG_EN			BIT(3)
#define PCA9468_BIT_NTC_PROTECTION_EN	BIT(2)
#define PCA9468_BIT_TEMP_MAX_EN			BIT(1)

#define PCA9468_REG_PWR_COLLAPSE		0x26	// Power collapse configuration
#define PCA9468_BIT_UV_DELTA			BITS(7,6)
#define PCA9468_BIT_IIN_FORCE_COUNT		BIT(4)
#define PCA9468_BIT_BAT_MISS_DET_EN		BIT(3)

#define PCA9468_REG_V_FLOAT				0x27	// Voltage regulation configuration
#define PCA9468_BIT_V_FLOAT				BITS(7,0)

#define PCA9468_REG_SAFETY_CTRL			0x28	// Safety configuration
#define PCA9468_BIT_WATCHDOG_EN			BIT(7)
#define PCA9468_BIT_WATCHDOG_CFG		BITS(6,5)
#define PCA9468_BIT_CHG_TIMER_EN		BIT(4)
#define PCA9468_BIT_CHG_TIMER_CFG		BITS(3,2)
#define PCA9468_BIT_OV_DELTA			BITS(1,0)

#define PCA9468_REG_NTC_TH_1			0x29	// Thermistor threshold configuration
#define PCA9468_BIT_NTC_THRESHOLD7_0	BITS(7,0)

#define PCA9468_REG_NTC_TH_2			0x2A	// Thermistor threshold configuration
#define PCA9468_BIT_NTC_THRESHOLD9_8	BITS(1,0)

#define PCA9468_REG_ADC_ACCESS			0x30

#define PCA9468_REG_ADC_ADJUST			0x31
#define PCA9468_BIT_ADC_GAIN			BITS(7,4)

#define PCA9468_REG_ADC_IMPROVE			0x3D
#define PCA9468_BIT_ADC_IIN_IMP			BIT(3)

#define PCA9468_REG_ADC_MODE			0x3F
#define PCA9468_BIT_ADC_MODE			BIT(4)

#define PCA9468_MAX_REGISTER			PCA9468_REG_ADC_MODE


#define PCA9468_IIN_CFG_STEP			100000	// input current step, unit - uA
#define PCA9468_IIN_CFG(_input_current)	(_input_current/PCA9468_IIN_CFG_STEP)	// input current, unit - uA
#define PCA9468_ICHG_CFG(_chg_current)	(_chg_current/100000)					// charging current, uint - uA
#define PCA9468_V_FLOAT(_v_float)		((_v_float/1000 - 3725)/5)				// v_float voltage, unit - uV

#define PCA9468_SNSRES_5mOhm			0x00
#define PCA9468_SNSRES_10mOhm			PCA9468_BIT_SNSRES

#define PCA9468_NTC_TH_STEP				2346	// 2.346mV, unit - uV

/* VIN Overvoltage setting from 2*VOUT */
enum {
	OV_DELTA_10P,
	OV_DELTA_20P,
	OV_DELTA_30P,
	OV_DELTA_40P,
};

enum {
	WDT_DISABLE,
	WDT_ENABLE,
};

enum {
	WDT_1SEC,
	WDT_2SEC,
	WDT_4SEC,
	WDT_8SEC,
};

/* Switching frequency */
enum {
	FSW_CFG_833KHZ,
	FSW_CFG_893KHZ,
	FSW_CFG_935KHZ,
	FSW_CFG_980KHZ,
	FSW_CFG_1020KHZ,
	FSW_CFG_1080KHZ,
	FSW_CFG_1120KHZ,
	FSW_CFG_1160KHZ,
	FSW_CFG_440KHZ,
	FSW_CFG_490KHZ,
	FSW_CFG_540KHZ,
	FSW_CFG_590KHZ,
	FSW_CFG_630KHZ,
	FSW_CFG_680KHZ,
	FSW_CFG_730KHZ,
	FSW_CFG_780KHZ
};

/* Enable pin polarity selection */
#define PCA9468_EN_ACTIVE_H		0x00
#define PCA9468_EN_ACTIVE_L		PCA9468_BIT_EN_CFG
#define PCA9468_STANDBY_FORCED	PCA9468_BIT_STANDBY_EN
#define PCA9468_STANDBY_DONOT	0

/* ADC Channel */
enum {
	ADCCH_VOUT = 1,
	ADCCH_VIN,
	ADCCH_VBAT,
	ADCCH_ICHG,
	ADCCH_IIN,
	ADCCH_DIETEMP,
	ADCCH_NTC,
	ADCCH_MAX
};

/* ADC step */
#define VIN_STEP		16000	// 16mV(16000uV) LSB, Range(0V ~ 16.368V)
#define VBAT_STEP		5000	// 5mV(5000uV) LSB, Range(0V ~ 5.115V)
#define IIN_STEP		4890 	// 4.89mA(4890uA) LSB, Range(0A ~ 5A)
#define ICHG_STEP		9780 	// 9.78mA(9780uA) LSB, Range(0A ~ 10A)
#define DIETEMP_STEP  	435		// 0.435C LSB, Range(-25C ~ 160C)
#define DIETEMP_DENOM	1000	// 1000, denominator
#define DIETEMP_MIN 	-25  	// -25C
#define DIETEMP_MAX		160		// 160C
#define VOUT_STEP		5000 	// 5mV(5000uV) LSB, Range(0V ~ 5.115V)
#define NTCV_STEP		2346 	// 2.346mV(2346uV) LSB, Range(0V ~ 2.4V)
#define ADC_IIN_OFFSET	900000	// 900mA

/* adc_gain bit[7:4] of reg 0x31 - 2's complement */
static int adc_gain[16] = { 0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1 };

/* Timer defination */
#define PCA9468_VBATMIN_CHECK_T	1000	// 1000ms
#define PCA9468_CCMODE_CHECK1_T	10000	// 10000ms
#define PCA9468_CCMODE_CHECK2_T	30000	// 30000ms
#define PCA9468_CCMODE_CHECK3_T	10000	// 10000ms
#define PCA9468_CCMODE_CHECK4_T	5000	// 5000ms
#define PCA9468_CVMODE_CHECK_T 	10000	// 10000ms
#define PCA9468_PDMSG_WAIT_T	200		// 200ms
#define PCA4968_ENABLE_DELAY_T	150		// 150ms
#if defined(CONFIG_BATTERY_SAMSUNG)
#define PCA9468_PPS_PERIODIC_T	7500	// 7500ms
#else
#define PCA9468_PPS_PERIODIC_T	10000	// 10000ms
#endif

/* Battery Threshold */
#if defined(CONFIG_BATTERY_SAMSUNG)
#define PCA9468_DC_VBAT_MIN		3100000	// 3100000uV
#else
#define PCA9468_DC_VBAT_MIN		3500000	// 3500000uV
#endif
/* Input Current Limit default value */
#define PCA9468_IIN_CFG_DFT		2000000	// 2000000uA
#define PCA9468_IIN_CFG_MIN		500000	// uA
#define PCA9468_IIN_CFG_MAX		5000000	// uA

/* Charging Current default value */
#define PCA9468_ICHG_CFG_DFT	6000000	// 6000000uA
#define PCA9468_ICHG_CFG_MIN	0	//uA
#define PCA9468_ICHG_CFG_MAX	8000000	//uA

/* Charging Float Voltage default value */
#define PCA9468_VFLOAT_DFT		4350000	// 4350000uV
#define PCA9468_VFLOAT_MIN		3725000	//uA

/* Sense Resistance default value */
#if defined(CONFIG_BATTERY_SAMSUNG)
#define PCA9468_SENSE_R_DFT		0		// 5mOhm
#else
#define PCA9468_SENSE_R_DFT		1		// 10mOhm
#endif
/* Switching Frequency default value */
#define PCA9468_FSW_CFG_DFT		3		// 980KHz
/* NTC threshold voltage default value */
#define PCA9468_NTC_TH_DFT		0		// 0uV

/* Charging Done Condition */
#define PCA9468_ICHG_DONE_DFT	600000	// 600mA
#define PCA9468_IIN_DONE_DFT	500000	// 500mA

/* CC mode 1,2 battery threshold */
#define PCA9468_CC2_VBAT_DFT	4090000 // 4090000uV
#define PCA9468_CC3_VBAT_DFT	4190000 // 4190000uV
/* CC2 mode TA current */
#define PCA9468_CC2_TA_CUR_DFT	1500000 // 1500000uA
/* CC3 mode TA current */
#define PCA9468_CC3_TA_CUR_DFT	1250000 // 1250000uA

/* CV mode threshold */
#define PCA9468_CV1_VBAT_DFT	4090000 // 4090000uV
#define PCA9468_CV2_VBAT_DFT	4190000	// 4190000uV
#define PCA9468_CV3_VBAT_DFT	4350000	// 4300000uV

/* Maximum TA voltage threshold */
#define PCA9468_TA_MAX_VOL		9800000 // 9800000uV
/* Maximum TA current threshold */
#define PCA9468_TA_MAX_CUR		2450000	// 2450000uA
/* Mimimum TA current threshold */
#define PCA9468_TA_MIN_CUR		1000000	// 1000000uA - PPS minimum current

/* Minimum TA voltage threshold in Preset mode */
#define PCA9468_TA_MIN_VOL_PRESET	7500000	// 7500000uV
/* TA voltage threshold starting Adjust CC mode */
#define PCA9468_TA_MIN_VOL_CCADJ	8500000	// 8000000uV-->8500000uV

#define PCA9468_TA_VOL_PRE_OFFSET	500000	// 200000uV --> 500000uV
/* Adjust CC mode TA voltage step */
#define PCA9468_TA_VOL_STEP_ADJ_CC	40000	// 40000uV
/* Pre CV mode TA voltage step */
#define PCA9468_TA_VOL_STEP_PRE_CV	20000	// 20000uV
/* IIN_CC adc offset for accuracy */
#define PCA9468_IIN_ADC_OFFSET		20000	// 20000uA

/* PD Message Voltage and Current Step */
#define PD_MSG_TA_VOL_STEP			20000	// 20mV
#define PD_MSG_TA_CUR_STEP			50000	// 50mA

#if defined(CONFIG_BATTERY_SAMSUNG)
#define PCA9468_SEC_DENOM_U_M		1000 // 1000, denominator
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
#define PCA9468_BATT_WDT_CONTROL_T		30000	// 30s
#endif
#endif

/* INT1 Register Buffer */
enum {
	REG_INT1,
	REG_INT1_MSK,
	REG_INT1_STS,
	REG_INT1_MAX
};

/* STS Register Buffer */
enum {
	REG_STS_A,
	REG_STS_B,
	REG_STS_C,
	REG_STS_D,
	REG_STS_MAX
};

/* Direct Charging State */
enum {
	DC_STATE_NO_CHARGING,	/* No charigng */

	DC_STATE_CHECK_VBAT,	/* Check min battery level */
	DC_STATE_PRESET_DC, 	/* Preset TA voltage/current for the direct charging */
	DC_STATE_CHECK_ACTIVE,	/* Check active status before entering Adjust CC mode */
	DC_STATE_ADJUST_CC,		/* Adjust CC mode */
	DC_STATE_START_CC,		/* Start CC mode */
	DC_STATE_CC_MODE,		/* Check CC mode status */
	DC_STATE_START_CV,		/* Start CV mode */
	DC_STATE_CV_MODE,		/* Check CV mode status */
	DC_STATE_CHARGING_DONE,	/* Charging Done */
	DC_STATE_ADJUST_TAVOL,	/* Adjust TA voltage to set new TA current under 1000mA input */

	DC_STATE_ADJUST_TACUR,	/* Adjust TA current to set new TA current over 1000mA input */
	DC_STATE_MAX,
};

/* CC Mode Status */
enum {
	CCMODE_CHG_LOOP,
	CCMODE_VFLT_LOOP,
	CCMODE_IIN_LOOP,
	CCMODE_LOOP_INACTIVE,
	CCMODE_VIN_UVLO,
};

/* CV Mode Status */
enum {
	CVMODE_CHG_LOOP,
	CVMODE_VFLT_LOOP,
	CVMODE_IIN_LOOP,
	CVMODE_LOOP_INACTIVE,
	CVMODE_CHG_DONE,
	CVMODE_VIN_UVLO,
};

/* Timer ID */
enum {
	TIMER_ID_NONE,

	TIMER_VBATMIN_CHECK,
	TIMER_PRESET_DC,
	TIMER_PRESET_CONFIG,
	TIMER_CHECK_ACTIVE,
	TIMER_ADJUST_CCMODE,
	TIMER_ENTER_CCMODE,
	TIMER_CHECK_CCMODE,
	TIMER_ENTER_CVMODE,
	TIMER_CHECK_CVMODE,
	TIMER_PDMSG_SEND,

	TIMER_ADJUST_TAVOL,
	TIMER_ADJUST_TACUR,
};

/* PD Message Type */
enum {
	PD_MSG_REQUEST_APDO,
	PD_MSG_REQUEST_FIXED_PDO,
};

/* TA increment Type */
enum {
	INC_NONE,	/* No increment */
	INC_TA_VOL, /* TA voltage increment */
	INC_TA_CUR, /* TA current increment */
};

struct pca9468_platform_data {
	unsigned int	irq_gpio;	/* GPIO pin that's connected to INT# */
	unsigned int	iin_cfg;	/* Input Current Limit - uA unit */
	unsigned int 	ichg_cfg;	/* Charging Current - uA unit */
	unsigned int	v_float;	/* V_Float Voltage - uV unit */
	unsigned int 	iin_cc2;	/* CC2 mode Input current - uA unit */
	unsigned int	vbat_cc2;	/* CC2 mode vbat threshold - uV unit */
	unsigned int	iin_cc3;	/* CC3 mode Input current - uA unit */
	unsigned int	vbat_cc3;	/* CC3 mode vbat threshold - uV unit */
	unsigned int 	iin_topoff;	/* Input Topoff current -uV unit */
	unsigned int 	snsres;		/* Current sense resister, 0 - 5mOhm, 1 - 10mOhm */
	unsigned int 	fsw_cfg; 	/* Switching frequency, refer to the datasheet, 0 - 833kHz, ... , 3 - 980kHz */
	unsigned int	ntc_th;		/* NTC voltage threshold : 0~2.4V - uV unit */
#if defined(CONFIG_BATTERY_SAMSUNG)
	int chgen_gpio;
	char *sec_dc_name;
#endif
};

/**
 * struct pca9468_charger - pca9468 charger instance
 * @monitor_wake_lock: lock to enter the suspend mode
 * @lock: protects concurrent access to online variables
 * @dev: pointer to device
 * @regmap: pointer to driver regmap
 * @mains: power_supply instance for AC/DC power
 * @timer_work: timer work for charging
 * @timer_id: timer id for timer_work
 * @timer_period: timer period for timer_work
 * @last_update_time: last update time after sleep
 * @pps_work: pps work for PPS periodic time
 * @pd: phandle for qualcomm PMI usbpd-phy
 * @mains_online: is AC/DC input connected
 * @charging_state: direct charging state
 * @ret_state: return direct charging state after DC_STATE_ADJUST_TAVOL is done
 * @iin_cc: input current for the direct charging in cc mode, uA
 * @ta_cur: AC/DC(TA) current, uA
 * @ta_vol: AC/DC(TA) voltage, uV
 * @ta_objpos: AC/DC(TA) PDO object position
 * @ta_target_vol: TA target voltage before any compensation
 * @ta_max_cur: TA maximum current of APDO, uA
 * @ta_max_vol: TA maximum voltage for the direct charging, uV
 * @prev_iin: Previous IIN ADC of PCA9468, uA
 * @prev_inc: Previous TA voltage or current increment factor
 * @req_new_iin: Request for new input current limit, true or false
 * @req_new_vfloat: Request for new vfloat, true or false
 * @new_iin: New request input current limit, uA
 * @new_vfloat: New request vfloat, uV
 * @adc_comp_gain: adc gain for compensation
 * @pdata: pointer to platform data
 * @debug_root: debug entry
 * @debug_address: debug register address
 */
struct pca9468_charger {
	struct wakeup_source	monitor_wake_lock;
	struct mutex		lock;
	struct device		*dev;
	struct regmap		*regmap;
#if defined(CONFIG_BATTERY_SAMSUNG)
	struct power_supply	*psy_chg;
#else
	struct power_supply	*mains;
#endif
	struct delayed_work timer_work;
	unsigned int		timer_id;
	unsigned long      	timer_period;
	unsigned long		last_update_time;

	struct delayed_work	pps_work;
#ifdef CONFIG_USBPD_PHY_QCOM
	struct usbpd 		*pd;
#endif

	bool				mains_online;
	unsigned int 		charging_state;
	unsigned int		ret_state;

	unsigned int		iin_cc;
	
	unsigned int		ta_cur;
	unsigned int		ta_vol;
	unsigned int		ta_objpos;

	unsigned int		ta_target_vol;

	unsigned int		ta_max_cur;
	unsigned int		ta_max_vol;

	unsigned int		prev_iin;
	unsigned int		prev_inc;

	bool				req_new_iin;
	bool				req_new_vfloat;
	unsigned int		new_iin;
	unsigned int		new_vfloat;
	
	int					adc_comp_gain;

	struct pca9468_platform_data *pdata;

	/* debug */
	struct dentry		*debug_root;
	u32					debug_address;

#if defined(CONFIG_BATTERY_SAMSUNG)
	int input_current;
	int charging_current;
	int float_voltage;
	int chg_status;
	int health_status;
	bool wdt_kick;

	int adc_val[ADCCH_MAX];

	unsigned int pdo_index;
	unsigned int pdo_max_voltage;
	unsigned int pdo_max_current;

#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	struct delayed_work wdt_control_work;
#endif
#endif
};

#if defined(CONFIG_BATTERY_SAMSUNG)
extern int sec_pd_select_pps(int num, int ppsVol, int ppsCur);
extern int sec_pd_get_apdo_max_current(unsigned int *pdo_pos, unsigned int taMaxVol, unsigned int *taMaxCur);
#endif //_CONFIG_PDIC_PD30

#endif
