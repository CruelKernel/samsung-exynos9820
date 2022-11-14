/*
 *  Copyright (C) 2011, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifndef __SSP_PRJ_H__
#define __SSP_PRJ_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/wakelock.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/rtc.h>
#include <linux/time.h>
#include <linux/regulator/consumer.h>
#include <linux/ssp_platformdata.h>
#include <linux/spi/spi.h>
#include "bbdpl/bbd.h"
#include <linux/sec_class.h>
#include "sensor_list.h"

#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
#include <linux/vmalloc.h> /* memory scraps issue. */
#endif

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
#include "ssp_sensorhub.h"
#endif

#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#endif

#ifdef CONFIG_PANEL_NOTIFY
#include <linux/panel_notify.h>
#endif
//#if defined (CONFIG_SENSORS_SSP_VLTE)
//#include <linux/hall.h>
//#endif

#include "ssp_dump.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#undef CONFIG_HAS_EARLYSUSPEND
#endif

#define SSP_DBG		1

#define SUCCESS		1
#define FAIL		0
#define ERROR		-1

#define FACTORY_DATA_MAX	99

#if SSP_DBG
#define SSP_FUNC_DBG 1
#define SSP_DATA_DBG 0

/* ssp mcu device ID */
#define DEVICE_ID			0x55

#define ssp_dbg(format, ...) \
	pr_info(format, ##__VA_ARGS__)
#else
#define ssp_dbg(format, ...)
#endif

#if SSP_FUNC_DBG
#define func_dbg() \
	pr_info("[SSP]: %s\n", __func__)
#else
#define func_dbg()
#endif

#if SSP_DATA_DBG
#define data_dbg(format, ...) \
	pr_info(format, ##__VA_ARGS__)
#else
#define data_dbg(format, ...)
#endif

#define SSP_SW_RESET_TIME	3000
#define DEFAULT_POLLING_DELAY	(200 * NSEC_PER_MSEC)
#define PROX_AVG_READ_NUM	80
#define DEFAULT_RETRIES		3
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
#define DATA_PACKET_SIZE	2000
#else
#define DATA_PACKET_SIZE	960
#endif
 // this packet size related when AP send ssp packet to MCU.
#define MAX_SSP_PACKET_SIZE	1000
#define SSP_INSTRUCTION_PACKET  9

#define SSP_DEBUG_TIME_FLAG_ON		"SSP:DEBUG_TIME=1"
#define SSP_DEBUG_TIME_FLAG_OFF		"SSP:DEBUG_TIME=0"

#define SSP_HALL_IC_ON			"SSP:HALL_IC=1"
#define SSP_HALL_IC_OFF			"SSP:HALL_IC=0"
#define SSP_AUTO_ROTATION_ORIENTATION "SSP:AUTO_ROTATION_ORIENTATION="
#define SSP_SAR_BACKOFF_MOTION_NOTI "SSP:SAR_BACKOFF_MOTION_NOTI="

#if 0 //def	CONFIG_SENSORS_SSP_PROX_AUTOCAL_AMS 
#define CONFIG_SENSORS_SSP_PROX_ADC_CAL
#endif

extern bool ssp_debug_time_flag;

#define ssp_debug_time(format, ...) \
	do { \
		if (unlikely(ssp_debug_time_flag)) \
			pr_info(format, ##__VA_ARGS__); \
	} while (0)

/* SSP Binary Type */
enum {
	KERNEL_BINARY = 0,
	KERNEL_CRASHED_BINARY,
	UMS_BINARY,
};

/*
 * SENSOR_DELAY_SET_STATE
 * Check delay set to avoid sending ADD instruction twice
 */
enum {
	INITIALIZATION_STATE = 0,
	NO_SENSOR_STATE,
	ADD_SENSOR_STATE,
	RUNNING_SENSOR_STATE,
};

/* for MSG2SSP_AP_GET_THERM */
enum {
	ADC_BATT = 0,
	ADC_CHG,
};

enum {
	SENSORS_BATCH_DRY_RUN			   = 0x00000001,
	SENSORS_BATCH_WAKE_UPON_FIFO_FULL   = 0x00000002
};

enum {
	META_DATA_FLUSH_COMPLETE = 1,
};

#define SSP_INVALID_REVISION		99999
#define SSP_INVALID_REVISION2		0xFFFFFF

/* Gyroscope DPS */
#define GYROSCOPE_DPS250		250
#define GYROSCOPE_DPS500		500
#define GYROSCOPE_DPS1000		1000
#define GYROSCOPE_DPS2000		2000

/* Gesture Sensor Current */
#define DEFAULT_IR_CURRENT		100

/* kernel -> ssp manager cmd*/
#define SSP_LIBRARY_SLEEP_CMD	(1 << 5)
#define SSP_LIBRARY_LARGE_DATA_CMD	(1 << 6)
#define SSP_LIBRARY_WAKEUP_CMD	(1 << 7)

/* AP -> SSP Instruction */
#define MSG2SSP_INST_BYPASS_SENSOR_ADD	0xA1
#define MSG2SSP_INST_BYPASS_SENSOR_REMOVE	0xA2
#define MSG2SSP_INST_REMOVE_ALL		0xA3
#define MSG2SSP_INST_CHANGE_DELAY		0xA4
#define MSG2SSP_INST_LIBRARY_ADD		0xB1
#define MSG2SSP_INST_LIBRARY_REMOVE		0xB2
#define MSG2SSP_AP_SET_THERMISTOR_TABLE	0xB3
#define MSG2SSP_INST_LIB_NOTI		0xB4
#define MSG2SSP_INST_VDIS_FLAG		0xB5
#define MSG2SSP_INST_LIB_DATA		0xC1
#define MSG2SSP_INST_CURRENT_TIMESTAMP	0xa7

#define MSG2SSP_AP_MCU_SET_GYRO_CAL		0xCD
#define MSG2SSP_AP_MCU_SET_ACCEL_CAL		0xCE
#define MSG2SSP_AP_MCU_SET_PROX_CAL		0xCF
#define MSG2SSP_AP_STATUS_SHUTDOWN		0xD0
#define MSG2SSP_AP_STATUS_WAKEUP		0xD1
#define MSG2SSP_AP_STATUS_SLEEP		0xD2
#define MSG2SSP_AP_STATUS_RESUME		0xD3
#define MSG2SSP_AP_STATUS_SUSPEND		0xD4
#define MSG2SSP_AP_STATUS_RESET		0xD5
#define MSG2SSP_AP_STATUS_POW_CONNECTED	0xD6
#define MSG2SSP_AP_STATUS_POW_DISCONNECTED	0xD7
#define MSG2SSP_AP_STATUS_SCONTEXT_WAKEUP	0x97
#define MSG2SSP_AP_STATUS_SCONTEXT_SLEEP	0x98
#define MSG2SSP_AP_SAR_BACKOFF_MOTION_NOTI	0x9B
#define MSG2SSP_AP_TEMPHUMIDITY_CAL_DONE	0xDA
#define MSG2SSP_AP_MCU_SET_DUMPMODE		0xDB
#define MSG2SSP_AP_MCU_DUMP_CHECK		0xDC
#define MSG2SSP_AP_MCU_BATCH_FLUSH		0xDD
#define MSG2SSP_AP_MCU_BATCH_COUNT		0xDF
#define MSG2SSP_AP_MCU_GET_ACCEL_RANGE	0xE1

#ifdef CONFIG_SENSORS_MULTIPLE_GLASS_TYPE
#define MSG2SSP_AP_GLASS_TYPE			 0xEC
#endif

#if defined(CONFIG_SSP_MOTOR_CALLBACK)
#define MSG2SSP_AP_MCU_SET_MOTOR_STATUS		0xC3
#endif

#define MSG2SSP_AP_WHOAMI			0x0F
#define MSG2SSP_AP_FIRMWARE_REV		0xF0
#define MSG2SSP_AP_SENSOR_FORMATION		0xF1
#define MSG2SSP_AP_SENSOR_PROXTHRESHOLD	0xF2
#define MSG2SSP_AP_SENSOR_BARCODE_EMUL	0xF3
#define MSG2SSP_AP_SENSOR_SCANNING		0xF4
#define MSG2SSP_AP_SET_MAGNETIC_HWOFFSET	0xF5
#define MSG2SSP_AP_GET_MAGNETIC_HWOFFSET	0xF6
#define MSG2SSP_AP_SENSOR_GESTURE_CURRENT	0xF7
#define MSG2SSP_AP_GET_THERM		0xF8
#define MSG2SSP_AP_GET_BIG_DATA		0xF9
#define MSG2SSP_AP_SET_BIG_DATA		0xFA
#define MSG2SSP_AP_START_BIG_DATA		0xFB
#define MSG2SSP_AP_SET_MAGNETIC_STATIC_MATRIX	0xFD
#define MSG2SSP_AP_SET_HALL_THRESHOLD		   0xE9
#define MSG2SSP_AP_SENSOR_TILT			0xEA
#define MSG2SSP_AP_MCU_SET_TIME			0xFE
#define MSG2SSP_AP_MCU_GET_TIME			0xFF
#define MSG2SSP_AP_MOBEAM_DATA_SET		0x31
#define MSG2SSP_AP_MOBEAM_REGISTER_SET		0x32
#define MSG2SSP_AP_MOBEAM_COUNT_SET		0x33
#define MSG2SSP_AP_MOBEAM_START			0x34
#define MSG2SSP_AP_MOBEAM_STOP			0x35
#define MSG2SSP_AP_GEOMAG_LOGGING		0x36
#define MSG2SSP_AP_SENSOR_LPF			0x37
#define MSG2AP_INST_TIMESTAMP_OFFSET		0xA8
#define MSG2SSP_AP_THERMISTOR_CHANNEL_0 	0xA9
#define MSG2SSP_AP_THERMISTOR_CHANNEL_1 	0xAA
#define MSG2SSP_AP_INFORMATION			0xB6

#define MSG2SSP_AP_IRDATA_SEND		0x38
#define MSG2SSP_AP_IRDATA_SEND_RESULT 0x39
#define MSG2SSP_AP_PROX_GET_TRIM	0x40
#define MSG2SSP_AP_PROX_GET_THRESHOLD	0x47

#define SH_MSG2AP_GYRO_CALIBRATION_START   0x43
#define SH_MSG2AP_GYRO_CALIBRATION_STOP	0x44
#define SH_MSG2AP_GYRO_CALIBRATION_EVENT_OCCUR  0x45

// to set mag cal data to ssp
#define MSG2SSP_AP_MAG_CAL_PARAM	 (0x46)

// for data injection
#define MSG2SSP_AP_DATA_INJECTION_MODE_ON_OFF 0x41
#define MSG2SSP_AP_DATA_INJECTION_SEND 0x42

#define MSG2SSP_AP_SET_PROX_SETTING		0x48
#define MSG2SSP_AP_SET_LIGHT_COEF		0x49
#define MSG2SSP_AP_GET_LIGHT_COEF		0x50
#define MSG2SSP_AP_SENSOR_PROX_ALERT_THRESHOLD 0x51

#define MSG2SSP_AP_REGISTER_DUMP		0x4A
#define MSG2SSP_AP_REGISTER_SETTING		  0x4B
#define MSG2SSP_AP_GET_PROXIMITY_LIGHT_DHR_SENSOR_INFO	0x4C

#define MSG2SSP_AP_FUSEROM			0X01

/* PANEL data */
#define MSG2SSP_GET_DDI_COPR		0x90
#define MSG2SSP_PANEL_INFORMATION	0x91
#define MSG2SSP_GET_TEST_COPR		0x92
#define MSG2SSP_GET_READ_COPR		0x93
#define MSG2SSP_READ_COPR_ON_OFF	0x94
#define MSG2SSP_GET_COPR_ROIX		0x95
#define MSG2SSP_HALL_IC_ON_OFF		0x96
#define MSG2SSP_AUTO_ROTATION_ORIENTATION 0x9A

/* voice data */
#define TYPE_WAKE_UP_VOICE_SERVICE			0x01
#define TYPE_WAKE_UP_VOICE_SOUND_SOURCE_AM		0x01
#define TYPE_WAKE_UP_VOICE_SOUND_SOURCE_GRAMMER	0x02

/* Factory Test */
#define ACCELEROMETER_FACTORY	0x80
#define GYROSCOPE_FACTORY		0x81
#define GEOMAGNETIC_FACTORY		0x82
#define PRESSURE_FACTORY		0x85
#define GESTURE_FACTORY		0x86
#define TEMPHUMIDITY_CRC_FACTORY	0x88
#define GYROSCOPE_TEMP_FACTORY	0x8A
#define GYROSCOPE_DPS_FACTORY	0x8B
#define MCU_FACTORY		0x8C
#define MCU_SLEEP_FACTORY		0x8D

/* Factory data length */
#define ACCEL_FACTORY_DATA_LENGTH		1
#define GYRO_FACTORY_DATA_LENGTH		36
#define MAGNETIC_FACTORY_DATA_LENGTH		26
#define PRESSURE_FACTORY_DATA_LENGTH		1
#define MCU_FACTORY_DATA_LENGTH		5
#define	GYRO_TEMP_FACTORY_DATA_LENGTH	2
#define	GYRO_DPS_FACTORY_DATA_LENGTH	1
#define TEMPHUMIDITY_FACTORY_DATA_LENGTH	1
#define MCU_SLEEP_FACTORY_DATA_LENGTH	FACTORY_DATA_MAX
#define GESTURE_FACTORY_DATA_LENGTH		4

#if defined(CONFIG_SENSORS_SSP_TMG399x)
#define DEFAULT_HIGH_THRESHOLD			130
#define DEFAULT_LOW_THRESHOLD			90
#define DEFAULT_CAL_HIGH_THRESHOLD		130
#define DEFAULT_CAL_LOW_THRESHOLD		54
#elif defined(CONFIG_SENSORS_SSP_TMD4905)/*CONFIG_SENSORS_SSP_PROX_AUTOCAL_AMS*/
#define DEFAULT_HIGH_THRESHOLD				600
#define DEFAULT_LOW_THRESHOLD				400
#define DEFAULT_DETECT_HIGH_THRESHOLD		8192
#define DEFAULT_DETECT_LOW_THRESHOLD		5000
#elif defined(CONFIG_SENSORS_SSP_TMD4904)/*CONFIG_SENSORS_SSP_PROX_AUTOCAL_AMS*/
#define DEFAULT_HIGH_THRESHOLD				420
#define DEFAULT_LOW_THRESHOLD				290
#define DEFAULT_DETECT_HIGH_THRESHOLD		16383
#define DEFAULT_DETECT_LOW_THRESHOLD		5000
#else
#ifdef CONFIG_SENSORS_SSP_PROX_AUTOCAL_AMS
#define DEFAULT_HIGH_THRESHOLD				2400
#define DEFAULT_LOW_THRESHOLD				1600
#ifdef CONFIG_SENSORS_SSP_CROWN
#define DEFAULT_DETECT_HIGH_THRESHOLD		16360
#define DEFAULT_DETECT_LOW_THRESHOLD		1500
#elif defined(CONFIG_SENSORS_SSP_BEYOND)
#define DEFAULT_DETECT_HIGH_THRESHOLD		16368
#define DEFAULT_DETECT_LOW_THRESHOLD		250
#define DEFAULT_DETECT_LOW_THRESHOLD_FOR_LCUT	2200
#else
#define DEFAULT_DETECT_HIGH_THRESHOLD		16368
#define DEFAULT_DETECT_LOW_THRESHOLD		1000
#endif //CONFIG_SENSORS_SSP_CROWN
#else
#define DEFAULT_HIGH_THRESHOLD			2000
#define DEFAULT_LOW_THRESHOLD			1400
#define DEFAULT_CAL_HIGH_THRESHOLD		2000
#define DEFAULT_CAL_LOW_THRESHOLD		840
#endif //CONFIG_SENSORS_SSP_PROX_AUTOCAL_AMS
#endif

#define DEFAULT_PROX_ALERT_HIGH_THRESHOLD			1000

/* SSP -> AP ACK about write CMD */
#define MSG_ACK		0x80	/* ACK from SSP to AP */
#define MSG_NAK		0x70	/* NAK from SSP to AP */

#define MAX_GYRO		32767
#define MIN_GYRO		-32768

#define MAX_COMP_BUFF 60

#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
/* Sensors's reporting mode */
#define REPORT_MODE_CONTINUOUS	0
#define REPORT_MODE_ON_CHANGE	1
#define REPORT_MODE_SPECIAL	2
#define REPORT_MODE_UNKNOWN	3

/* Key value for Camera - Gyroscope sync */
#define CAMERA_GYROSCOPE_SYNC 7700000ULL /*7.7ms*/
#define CAMERA_GYROSCOPE_VDIS_SYNC 6600000ULL /*6.6ms*/
#define CAMERA_GYROSCOPE_SUPER_VDIS_SYNC 5500000ULL /*5.5ms*/
#define CAMERA_GYROSCOPE_ULTRA_VDIS_SYNC 4400000ULL /*4.4ms*/
#define CAMERA_GYROSCOPE_SYNC_DELAY 10000000ULL
#define CAMERA_GYROSCOPE_VDIS_SYNC_DELAY 5000000ULL
#define CAMERA_GYROSCOPE_SUPER_VDIS_SYNC_DELAY 2000000ULL
#define CAMERA_GYROSCOPE_ULTRA_VDIS_SYNC_DELAY 1000000ULL

/** HIFI Sensor **/
#define SIZE_TIMESTAMP_BUFFER	1000
#define SIZE_MOVING_AVG_BUFFER	20
#define SKIP_CNT_MOVING_AVG_ADD	2
#define SKIP_CNT_MOVING_AVG_CHANGE	4
#endif

/* Mag type definition */
#define MAG_TYPE_YAS  0
#define MAG_TYPE_AKM  1

/* Magnetic cal parameter size */
#define MAC_CAL_PARAM_SIZE_AKM  13
#define MAC_CAL_PARAM_SIZE_YAS  7

/* Magnetic Read Size */
#ifdef CONFIG_SSP_SUPPORT_MAGNETIC_OVERFLOW
	#define UNCAL_MAGNETIC_SIZE	 13
	#define MAGNETIC_SIZE		   8
#else
	#define UNCAL_MAGNETIC_SIZE	 12
	#define MAGNETIC_SIZE		   7
#endif
/* ak0911 magnetic pdc matrix size */
#define PDC_SIZE					   27

/* gyro calibration state*/
#define GYRO_CALIBRATION_STATE_NOT_YET		0
#define GYRO_CALIBRATION_STATE_REGISTERED	1
#define GYRO_CALIBRATION_STATE_EVENT_OCCUR  2
#define GYRO_CALIBRATION_STATE_DONE		3

/* bbd patch version define only for beyond*/
#define bbd_old     0
#define bbd_new_old     1
#define bbd_current     2

#define REGISTER_RW_DDI			89
#define REGISTER_RW_BUFFER_MAX		256

/* temphumidity sensor*/
struct shtc1_buffer {
	u16 batt[MAX_COMP_BUFF];
	u16 chg[MAX_COMP_BUFF];
	s16 temp[MAX_COMP_BUFF];
	u16 humidity[MAX_COMP_BUFF];
	u16 baro[MAX_COMP_BUFF];
	u16 gyro[MAX_COMP_BUFF];
	char len;
};

/* SSP_INSTRUCTION_CMD */
enum {
	REMOVE_SENSOR = 0,
	ADD_SENSOR,
	CHANGE_DELAY,
	GO_SLEEP,
	REMOVE_LIBRARY,
	ADD_LIBRARY,
};

struct meta_data_event {
	s32 what;
	s32 sensor;
} __attribute__((__packed__));

struct sensor_value {
	union {
		struct {
			s16 x;
			s16 y;
			s16 z;
		};
		struct {
			s32 x;
			s32 y;
			s32 z;
		} gyro;
		struct {
			s32 x;
			s32 y;
			s32 z;
			s32 offset_x;
			s32 offset_y;
			s32 offset_z;
		} uncal_gyro;
		struct {		/*calibrated mag, gyro*/
			s16 cal_x;
			s16 cal_y;
			s16 cal_z;
			u8 accuracy;
#ifdef CONFIG_SSP_SUPPORT_MAGNETIC_OVERFLOW
						u8 overflow;
#endif
		};
		struct {		/*uncalibrated mag, gyro*/
			s16 uncal_x;
			s16 uncal_y;
			s16 uncal_z;
			s16 offset_x;
			s16 offset_y;
			s16 offset_z;
#ifdef CONFIG_SSP_SUPPORT_MAGNETIC_OVERFLOW
						u8 uncaloverflow;
#endif
		};
		struct {		/* rotation vector */
			s32 quat_a;
			s32 quat_b;
			s32 quat_c;
			s32 quat_d;
			u8 acc_rot;
		};
		struct {
#ifdef CONFIG_SENSORS_SSP_LIGHT_REPORT_LUX
			u32 lux;
			s32 cct;
#endif
			u16 r;
			u16 g;
			u16 b;
			u16 w;
#ifdef CONFIG_SENSORS_SSP_LIGHT_MAX_GAIN_2BYTE
#ifdef CONFIG_SENSORS_SSP_LIGHT_MAX_ATIME_2BYTE
			u16 a_gain;
			u16 a_time;
#ifdef CONFIG_SENSORS_SSP_LIGHT_ADDING_LUMINANCE
			u8 brightness;
#endif
#ifdef CONFIG_SENSORS_SSP_LIGHT_LUX_RAW
			u32 lux_raw;
#endif
#else
			u16 a_gain;
			u8 a_time;
#endif
#else
			u8 a_time;
			u8 a_gain;
#endif
		} __attribute__((__packed__));

#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
		struct {
			u16 irdata;
			u16 ir_r;
			u16 ir_g;
			u16 ir_b;
			u16 ir_w;
#ifdef CONFIG_SENSORS_SSP_LIGHT_MAX_GAIN_2BYTE
			u16 ir_a_gain;
			u8 ir_a_time; // ir_brightness;
#else
			u8 ir_a_time;
			u8 ir_a_gain;
#endif
		};
#endif
		struct {
			s32 cap_main;
			s16 useful;
			s16 offset;
			u8 irq_stat;
		};
		struct {
			u8 prox_detect;
#if defined(CONFIG_SENSORS_SSP_TMG399x)
			u8 prox_adc;
#else
/* CONFIG_SENSORS_SSP_TMD4903, CONFIG_SENSORS_SSP_TMD3782, CONFIG_SENSORS_SSP_TMD4904 */
			u16 prox_adc;
			u32 light;
#endif
#if defined(CONFIG_SENSORS_SSP_PROX_LIGHT_DIFF)
			u32 light_diff;
#endif
		} __attribute__((__packed__));
#ifdef CONFIG_SENSORS_SSP_PROX_ADC_CAL
		struct {
			s16 proxcal_offset;
			u16 proxcal_thresh_hi;
			u16 proxcal_thresh_lo;
		};
#endif
		u8 step_det;
		u8 sig_motion;
#if defined(CONFIG_SENSORS_SSP_TMG399x)
		u8 prox_raw[4];
#else
/* CONFIG_SENSORS_SSP_TMD4903, CONFIG_SENSORS_SSP_TMD3782, CONFIG_SENSORS_SSP_TMD4904 */
		s16 prox_raw[4];
#endif
		 struct {
			u8 prox_alert_detect;
			u16 prox_alert_adc;
		} __attribute__((__packed__));
		struct {
			s32 pressure;
			s16 temperature;
		};
		 struct {
			u8 thermistor_type;
			s16 thermistor_raw;
		} __attribute__((__packed__));
		s16 light_flicker;
		u8 data[20];
		u32 step_diff;
		u8 tilt_detector;
		u8 pickup_gesture;
		u8 call_gesture;
		u8 wakeup_move_event[2]; // wakeup motion[0] & move[1] event come sametime
		struct {
			u8 pocket_mode;
			u8 pocket_reason;
			u32 pocket_base_proxy;
			u32 pocket_current_proxy;
			u32 pocket_release_diff;
			u32 pocket_min_release;
			u32 pocket_light_data;
			u32 pocket_temp;
		} __attribute__((__packed__));
		u8 led_cover_event;
		u8 auto_rotation_event;
		u8 sar_backoff_motion_event;
		u8 scontext_buf[SCONTEXT_DATA_SIZE];
		struct {
			u8 proximity_pocket_detect;
			u16 proximity_pocket_adc;
		} __attribute__((__packed__));
		struct meta_data_event meta_data;
	};
	u64 timestamp;
} __attribute__((__packed__));

extern struct class *sensors_event_class;

struct calibraion_data {
	s32 x;
	s32 y;
	s32 z;
};

struct grip_calibration_data {
	s32 cap_main;
	s32 ref_cap_main;
	s16 useful;
	s16 offset;
	int init_threshold;
	int diff_threshold;
	int threshold;
	bool calibrated;
	bool mode_set;
	int temp;
	int temp_cal;
	char slope;
};

struct hw_offset_data {
	char x;
	char y;
	char z;
};

/* ssp_msg options bit*/
#define SSP_SPI		0	/* read write mask */
#define SSP_RETURN	2	/* write and read option */
#define SSP_GYRO_DPS	3	/* gyro dps mask */
#define SSP_INDEX	3	/* data index mask */

#define SSP_SPI_MASK		(3 << SSP_SPI)	/* read write mask */
#define SSP_GYRO_DPS_MASK	(3 << SSP_GYRO_DPS)
/* dump index mask. Index is up to 8191 */
#define SSP_INDEX_MASK	(8191 << SSP_INDEX)

struct ssp_msg {
	u8 cmd;
	u16 length;
	u16 options;
	u32 data;

	struct list_head list;
	struct completion *done;
	char *buffer;
	u8 free_buffer;
	bool *dead_hook;
	bool dead;
} __attribute__((__packed__));

enum {
	AP2HUB_READ = 0,
	AP2HUB_WRITE,
	HUB2AP_WRITE,
	AP2HUB_READY,
	AP2HUB_RETURN
};

enum {
	BIG_TYPE_DUMP = 0,
	BIG_TYPE_READ_LIB,
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
	BIG_TYPE_READ_HIFI_BATCH,
#endif
	/*+snamy.jeong 0706 for voice model download & pcm dump*/
	BIG_TYPE_VOICE_NET,
	BIG_TYPE_VOICE_GRAM,
	BIG_TYPE_VOICE_PCM,
	/*-snamy.jeong 0706 for voice model download & pcm dump*/
	BIG_TYPE_TEMP,
	BIG_TYPE_MAX,
};

enum {
	BATCH_MODE_NONE = 0,
	BATCH_MODE_RUN,
};

extern struct mutex shutdown_lock;

#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
struct ssp_batch_event {
	char *batch_data;
	int batch_length;
};
#else
struct ssp_time_diff {
	u16 batch_count;
	u16 batch_mode;
	u64 time_diff;
	u64 irq_diff;
	u16 batch_count_fixed;
};
#endif

struct ois_sensor_interface{
	void *core;
	void (*ois_func)(void *);
};

struct ssp_data {
	struct iio_dev *indio_dev[SENSOR_MAX];
	struct iio_dev *indio_scontext_dev;
	struct iio_dev *meta_indio_dev;

	struct spi_device *spi;
	struct workqueue_struct *bbd_on_packet_wq;
	struct work_struct work_bbd_on_packet;
	struct workqueue_struct *bbd_mcu_ready_wq;
	struct work_struct work_bbd_mcu_ready;

#if defined(CONFIG_SSP_MOTOR_CALLBACK)
	struct workqueue_struct *ssp_motor_wq;
	struct work_struct work_ssp_motor;
#endif
	struct delayed_work work_ssp_reset;

#ifdef SSP_BBD_USE_SEND_WORK
	struct workqueue_struct *bbd_send_packet_wq;
	struct work_struct work_bbd_send_packet;
	struct ssp_msg *bbd_send_msg;
	unsigned short bbd_msg_options;
#endif	/* SSP_BBD_USE_SEND_WORK  */
	struct i2c_client *client;
	struct wake_lock ssp_wake_lock;
	struct wake_lock ssp_batch_wake_lock;
	struct wake_lock ssp_comm_wake_lock;
	struct timer_list debug_timer;
	struct workqueue_struct *debug_wq;
	struct work_struct work_debug;
	struct calibraion_data accelcal;
	struct calibraion_data gyrocal;
	struct grip_calibration_data gripcal;
	struct hw_offset_data magoffset;
	struct sensor_value buf[SENSOR_MAX];
	struct device *sen_dev;
	struct device *mcu_device;
	struct device *acc_device;
	struct device *gyro_device;
	struct device *mag_device;
	struct device *prs_device;
	struct device *prox_device;
	struct device *grip_device;
	struct device *light_device;
	struct device *ges_device;
#ifdef SENSORS_SSP_IRLED
	struct device *irled_device;
#endif
#ifdef CONFIG_SENSORS_SSP_LIGHT_COLORID
	struct device *hiddenhole_device;
	int light_efs_file_status;
	struct delayed_work work_ssp_light_efs_file_init;
#endif
	struct device *temphumidity_device;
#ifdef CONFIG_SENSORS_SSP_MOBEAM
	struct device *mobeam_device;
#endif
	struct miscdevice batch_io_device;
/*snamy.jeong@samsung.com temporary code for voice data sending to mcu*/
	struct device *voice_device;
#if defined(CONFIG_MUIC_NOTIFIER)
	struct notifier_block cpuidle_muic_nb;
#endif
	struct device *thermistor_device;
	
	struct ois_sensor_interface *ois_control;
	struct ois_sensor_interface *ois_reset;

	bool bFirstRef;
	bool bSspShutdown;
	bool bAccelAlert;
	bool bProximityRawEnabled;
	bool bGeomagneticRawEnabled;
	bool bGeomagneticLogged;
	bool bBarcodeEnabled;
	bool bMcuDumpMode;
	bool bBinaryChashed;
	bool bProbeIsDone;
	bool bDumping;
	bool bTimeSyncing;
	bool bHandlingIrq;
	bool grip_off;
#if defined(CONFIG_MUIC_NOTIFIER)
	bool jig_is_attached;
#endif

	int light_coef[7];
#if defined(CONFIG_SENSORS_SSP_PROX_AUTOCAL_AMS)
#ifdef CONFIG_SENSORS_SSP_PROX_ADC_CAL
	int ProxOffset;
#endif
	unsigned int uProxHiThresh;
	unsigned int uProxLoThresh;
	unsigned int uProxHiThresh_detect;
	unsigned int uProxLoThresh_detect;
#else
	unsigned int uProxCanc;
	unsigned int uCrosstalk;
	unsigned int uProxCalResult;
	unsigned int uProxHiThresh;
	unsigned int uProxLoThresh;
	unsigned int uProxHiThresh_default;
	unsigned int uProxLoThresh_default;
	unsigned int uProxHiThresh_cal;
	unsigned int uProxLoThresh_cal;
#endif

	unsigned int uProxAlertHiThresh;
	unsigned int uIr_Current;
	unsigned char uFuseRomData[3];
	unsigned char uMagCntlRegData;
	char *pchLibraryBuf;
	char chLcdLdi[2];
	int iLibraryLength;
	int aiCheckStatus[SENSOR_MAX];

	unsigned int uComFailCnt;
	unsigned int uResetCnt;
	unsigned int uTimeOutCnt;
	unsigned int uIrqCnt;
	unsigned int uDumpCnt;

	int sealevelpressure;
	unsigned int uGyroDps;
	u64 uSensorState;
	unsigned int uCurFirmRev;
	unsigned int uFactoryProxAvg[4];
	char uLastResumeState;
	char uLastAPState;
	s32 iPressureCal;
	u64 step_count_total;

	atomic64_t aSensorEnable;
	int64_t adDelayBuf[SENSOR_MAX];
	u64 lastTimestamp[SENSOR_MAX];
	u64 LastSensorTimeforReset[SENSOR_MAX]; //only use accel and light
	u32 IsBypassMode[SENSOR_MAX]; //only use accel and light
	s32 batchLatencyBuf[SENSOR_MAX];
	s8 batchOptBuf[SENSOR_MAX];
	bool reportedData[SENSOR_MAX];
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
	bool skipEventReport;
	bool cameraGyroSyncMode;

	/** HIFI Sensor **/
	u64 ts_index_buffer[SIZE_TIMESTAMP_BUFFER];
	unsigned int ts_stacked_cnt;
	unsigned int ts_stacked_offset;
	unsigned int ts_prev_index[SENSOR_MAX];
	u64 ts_irq_last;
	u64 ts_last_enable_cmd_time;

	u64 lastDeltaTimeNs[SENSOR_MAX];

	u64 ts_avg_buffer[SENSOR_MAX][SIZE_MOVING_AVG_BUFFER];
	u8 ts_avg_buffer_cnt[SENSOR_MAX];
	u8 ts_avg_buffer_idx[SENSOR_MAX];
	u64 ts_avg_buffer_sum[SENSOR_MAX];
	int ts_avg_skip_cnt[SENSOR_MAX];

	struct workqueue_struct *batch_wq;
	struct mutex batch_events_lock;
	struct ssp_batch_event batch_event;

	/* HiFi batching suspend-wakeup issue */
	u64 resumeTimestamp;
	bool bIsResumed;
	int mcu_host_wake_int;
	int mcu_host_wake_irq;
	bool bIsReset;
#endif
	int (*wakeup_mcu)(void);
	int (*set_mcu_reset)(int);

	void (*get_sensor_data[SENSOR_MAX])(char *, int *,
		struct sensor_value *);
	void (*report_sensor_data[SENSOR_MAX])(struct ssp_data *, int sensor_type,
		struct sensor_value *);

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	struct ssp_sensorhub_data *hub_data;
#endif
	int ap_type;
	int ap_rev;
	int accel_position;
	int mag_position;
	u8 mag_matrix_size;
	u8 *mag_matrix;
	unsigned char pdc_matrix[PDC_SIZE];
	short hall_threshold[5];
#ifdef CONFIG_SENSORS_SSP_SHTC1
	struct miscdevice shtc1_device;
	char *comp_engine_ver;
	struct platform_device *pdev_pam_temp;
	struct s3c_adc_client *adc_client;
	u8 cp_thm_adc_channel;
	u8 cp_thm_adc_arr_size;
	u8 batt_thm_adc_arr_size;
	u8 chg_thm_adc_arr_size;
	struct thm_adc_table *cp_thm_adc_table;
	struct thm_adc_table *batt_thm_adc_table;
	struct thm_adc_table *chg_thm_adc_table;
	struct mutex cp_temp_adc_lock;
	struct mutex bulk_temp_read_lock;
	struct shtc1_buffer *bulk_buffer;
#endif
	struct mutex comm_mutex;
	struct mutex pending_mutex;
	struct mutex enable_mutex;
	struct mutex ssp_enable_mutex;

	s16 *static_matrix;
	struct list_head pending_list;

	void (*ssp_big_task[BIG_TYPE_MAX])(struct work_struct *);
	u64 timestamp;
	int light_log_cnt;
	int light_cct_log_cnt;
#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
	int light_ir_log_cnt;
#endif
#ifdef CONFIG_SENSORS_MULTIPLE_GLASS_TYPE
	u32 glass_type;
#endif
	int acc_type;
	int gyro_lib_state;
	int mag_type;

	/* variable for sensor register dump */
	char *sensor_dump[SENSOR_MAX];

	/* data for injection */
	u8 data_injection_enable;
	struct miscdevice ssp_data_injection_device;

//#if defined (CONFIG_SENSORS_SSP_VLTE)
//	struct notifier_block hall_ic_nb;
//	int change_axis;
//#endif

#if defined(CONFIG_SSP_MOTOR_CALLBACK)
	int motor_state;
#endif
#if defined(CONFIG_PANEL_NOTIFY)
	struct panel_bl_event_data panel_event_data;
#endif
	char sensor_state[SENSOR_MAX + 1 + ((SENSOR_MAX-1) / 10)]; // \0 + blank
	unsigned int uNoRespSensorCnt;
	unsigned int errorCount;
	unsigned int pktErrCnt;
	unsigned int mcuCrashedCnt;
	bool mcuAbnormal;
	bool IsMcuCrashed;

/* variables for conditional leveling timestamp */
	bool first_sensor_data[SENSOR_MAX];
	u64 timestamp_factor;

	struct regulator *regulator_vdd_mcu_1p8;
	const char *vdd_mcu_1p8_name;

	int shub_en;
	int pin_ap_sleep;
	int pin_shub_int;
	int irq_shub_int;
	bool intendedMcuReset;
	char registerValue[REGISTER_RW_BUFFER_MAX + 3];
	u8 dhrAccelScaleRange;
	unsigned int skipSensorData;
	bool resetting;

/* variables for timestamp sync */
	struct delayed_work work_ssp_tiemstamp_sync;
	u64 timestamp_offset;

/* information of sensorhub for big data */
	bool IsGpsWorking;
	char resetInfo[2048];
        char resetInfoDebug[512];
        u64 resetInfoDebugTime;
	u32 resetCntGPSisOn;
/* thermistor (up & sub) table information*/
	short tempTable_up[23];
	short tempTable_sub[23];
        short threshold_up;
        short threshold_sub;
/* when gyro selftest, check sensor should be skip*/
	bool IsGyroselftest;
/* DVIS check flag*/
	bool IsVDIS_Enabled;
/* AP suspend check flag*/
        bool IsAPsuspend;
/* no ack about mcu_resp pin*/
        bool IsNoRespCnt;
/* hall ic */
	bool hall_ic_status; // 0: open 1: close
};

//#if defined (CONFIG_SENSORS_SSP_VLTE)
//extern int folder_state;
//#endif

struct ssp_big {
	struct ssp_data *data;
	struct work_struct work;
	u32 length;
	u32 addr;
};

#ifdef CONFIG_SENSORS_SSP_MOBEAM
struct reg_index_table {
	unsigned char reg;
	unsigned char index;
};
#endif

int ssp_iio_configure_ring(struct iio_dev *indio_dev);
void ssp_iio_unconfigure_ring(struct iio_dev *indio_dev);
void ssp_enable(struct ssp_data *data, bool enable);
int ssp_spi_async(struct ssp_data *data, struct ssp_msg *msg);
int ssp_spi_sync(struct ssp_data *data, struct ssp_msg *msg, int timeout);
extern void clean_msg(struct ssp_msg *msg);
extern void bcm4773_debug_info(void);
void clean_pending_list(struct ssp_data *data);
void toggle_mcu_reset(struct ssp_data *data);
int initialize_mcu(struct ssp_data *data);
int initialize_input_dev(struct ssp_data *data);
int initialize_sysfs(struct ssp_data *data);
void initialize_function_pointer(struct ssp_data *data);
void initialize_accel_factorytest(struct ssp_data *data);
#ifndef CONFIG_SENSORS_SSP_CM3323
void initialize_prox_factorytest(struct ssp_data *data);
#endif
void initialize_light_factorytest(struct ssp_data *data);
void initialize_gyro_factorytest(struct ssp_data *data);
void initialize_pressure_factorytest(struct ssp_data *data);
void initialize_thermistor_factorytest(struct ssp_data *data);
void initialize_magnetic_factorytest(struct ssp_data *data);
void initialize_gesture_factorytest(struct ssp_data *data);
#ifdef CONFIG_SENSORS_SSP_IRLED
void initialize_irled_factorytest(struct ssp_data *data);
#endif
void initialize_temphumidity_factorytest(struct ssp_data *data);
void remove_accel_factorytest(struct ssp_data *data);
void remove_gyro_factorytest(struct ssp_data *data);
#ifndef CONFIG_SENSORS_SSP_CM3323
void remove_prox_factorytest(struct ssp_data *data);
#endif
void remove_light_factorytest(struct ssp_data *data);
void remove_pressure_factorytest(struct ssp_data *data);
void remove_thremistor_factorytest(struct ssp_data *data);
void remove_magnetic_factorytest(struct ssp_data *data);
void remove_gesture_factorytest(struct ssp_data *data);
#ifdef CONFIG_SENSORS_SSP_IRLED
void remove_irled_factorytest(struct ssp_data *data);
#endif
void remove_temphumidity_factorytest(struct ssp_data *data);
#ifdef CONFIG_SENSORS_SSP_MOBEAM
void initialize_mobeam(struct ssp_data *data);
void remove_mobeam(struct ssp_data *data);
#endif
#ifdef CONFIG_SENSORS_SSP_SX9306
void initialize_grip_factorytest(struct ssp_data *data);
void remove_grip_factorytest(struct ssp_data *data);
void open_grip_caldata(struct ssp_data *data);
int set_grip_calibration(struct ssp_data *data, bool set);
#endif
void sensors_remove_symlink(struct input_dev *inputdev);
void destroy_sensor_class(void);
int initialize_event_symlink(struct ssp_data *data);
int sensors_create_symlink(struct input_dev *inputdev);
int accel_open_calibration(struct ssp_data *data);
int gyro_open_calibration(struct ssp_data *data);
int pressure_open_calibration(struct ssp_data *data);
int proximity_open_calibration(struct ssp_data *data);
int load_magnetic_cal_param_from_nvm(u8 *data, u8 length);
int set_magnetic_cal_param_to_ssp(struct ssp_data *data);
int save_magnetic_cal_param_to_nvm(struct ssp_data *data, char *pchRcvDataFrame, int *iDataIdx);
void remove_input_dev(struct ssp_data *data);
void remove_sysfs(struct ssp_data *data);
void remove_event_symlink(struct ssp_data *data);
int ssp_send_cmd(struct ssp_data *data, char command, int arg);
int send_instruction(struct ssp_data *data, u8 uInst, u8 uSensorType, u8 *uSendBuf, u16 uLength);
int send_instruction_sync(struct ssp_data *data, u8 uInst, u8 uSensorType, u8 *uSendBuf, u16 uLength);
int flush(struct ssp_data *data, u8 uSensorType);
int get_batch_count(struct ssp_data *data, u8 uSensorType);
int select_irq_msg(struct ssp_data *data);
int get_chipid(struct ssp_data *data);
int set_big_data_start(struct ssp_data *data, u8 type, u32 length);
int mag_open_hwoffset(struct ssp_data *data);
int mag_store_hwoffset(struct ssp_data *data);
int set_hw_offset(struct ssp_data *data);
int get_hw_offset(struct ssp_data *data);
int set_gyro_cal(struct ssp_data *data);
int save_gyro_caldata(struct ssp_data *data, s32 *iCalData);
int set_accel_cal(struct ssp_data *data);
#ifdef CONFIG_SENSORS_SSP_PROX_ADC_CAL
void set_prox_cal(struct ssp_data *data);
int proximity_save_calibration(struct ssp_data *data);
#endif
int initialize_magnetic_sensor(struct ssp_data *data);
int initialize_thermistor_table(struct ssp_data *data);
int set_ap_information(struct ssp_data *data);
int set_sensor_position(struct ssp_data *data);
#ifdef CONFIG_SENSORS_MULTIPLE_GLASS_TYPE
int set_glass_type(struct ssp_data *data);
#endif
int set_magnetic_static_matrix(struct ssp_data *data);
void sync_sensor_state(struct ssp_data *data);
void set_proximity_threshold(struct ssp_data *data);
void set_proximity_alert_threshold(struct ssp_data *data);
void set_proximity_barcode_enable(struct ssp_data *data, bool bEnable);
void set_gesture_current(struct ssp_data *data, unsigned char uData1);
int set_hall_threshold(struct ssp_data *data);
void set_gyro_cal_lib_enable(struct ssp_data *data, bool bEnable);
void set_light_coef(struct ssp_data *data);
#ifdef CONFIG_SENSORS_SSP_LIGHT_COLORID
int initialize_light_colorid(struct ssp_data *data);
void initialize_hiddenhole_factorytest(struct ssp_data *data);
void remove_hiddenhole_factorytest(struct ssp_data *data);
void initialize_light_colorid_do_task(struct work_struct *work);
#endif
int get_msdelay(int64_t dDelayRate);
#if defined(CONFIG_SSP_MOTOR_CALLBACK)
int send_motor_state(struct ssp_data *data);
#endif
u64 get_sensor_scanning_info(struct ssp_data *data);
unsigned int get_firmware_rev(struct ssp_data *data);
u8 get_accel_range(struct ssp_data *data);
int parse_dataframe(struct ssp_data *data, char *pchRcvDataFrame, int iLength);
void enable_debug_timer(struct ssp_data *data);
void disable_debug_timer(struct ssp_data *data);
int initialize_debug_timer(struct ssp_data *data);
void get_proximity_threshold(struct ssp_data *data);
void report_meta_data(struct ssp_data *data, int sensor_type, struct sensor_value *s);
void report_acc_data(struct ssp_data *data, int sensor_type, struct sensor_value *accdata);
void report_gyro_data(struct ssp_data *data, int sensor_type, struct sensor_value *gyrodata);
void report_mag_data(struct ssp_data *data, int sensor_type, struct sensor_value *magdata);
void report_mag_uncaldata(struct ssp_data *data, int sensor_type, struct sensor_value *magdata);
void report_rot_data(struct ssp_data *data, int sensor_type, struct sensor_value *rotdata);
void report_game_rot_data(struct ssp_data *data, int sensor_type, struct sensor_value *grvec_data);
void report_step_det_data(struct ssp_data *data, int sensor_type, struct sensor_value *stepdet_data);
void report_gesture_data(struct ssp_data *data, int sensor_type, struct sensor_value *gesdata);
void report_pressure_data(struct ssp_data *data, int sensor_type, struct sensor_value *predata);
void report_light_data(struct ssp_data *data, int sensor_type, struct sensor_value *lightdata);
#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
void report_light_ir_data(struct ssp_data *data, int sensor_type, struct sensor_value *lightirdata);
#endif
void report_light_flicker_data(struct ssp_data *data, int sensor_type, struct sensor_value *lightFlickerData);
void report_light_cct_data(struct ssp_data *data, int sensor_type, struct sensor_value *lightdata);
void report_prox_data(struct ssp_data *data, int sensor_type, struct sensor_value *proxdata);
void report_prox_raw_data(struct ssp_data *data, int sensor_type, struct sensor_value *proxrawdata);
void report_prox_alert_data(struct ssp_data *data, int sensor_type, struct sensor_value *proxdata);
void report_proximity_pocket_data(struct ssp_data *data, int sensor_type, struct sensor_value *proximity_pocket);
#ifdef CONFIG_SENSORS_SSP_PROX_ADC_CAL
void report_prox_adc_calib_data(struct ssp_data *data, int sensor_type, struct sensor_value *proxcalibdata);
#endif
void report_grip_data(struct ssp_data *data, int sensor_type, struct sensor_value *gripdata);
void report_geomagnetic_raw_data(struct ssp_data *data, int sensor_type, struct sensor_value *magrawdata);
void report_sig_motion_data(struct ssp_data *data, int sensor_type, struct sensor_value *sig_motion_data);
void report_uncalib_gyro_data(struct ssp_data *data, int sensor_type, struct sensor_value *gyrodata);
void report_step_cnt_data(struct ssp_data *data, int sensor_type, struct sensor_value *sig_motion_data);
#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
void report_interrupt_gyro_data(struct ssp_data *data, int sensor_type, struct sensor_value *gyrodata);
#endif
int print_mcu_debug(char *pchRcvDataFrame, int *pDataIdx, int iRcvDataFrameLength);
void report_temp_humidity_data(struct ssp_data *data, int sensor_type, struct sensor_value *temp_humi_data);
void report_shake_cam_data(struct ssp_data *data, int sensor_type, struct sensor_value *shake_cam_data);
void report_bulk_comp_data(struct ssp_data *data);
void report_tilt_data(struct ssp_data *data, int sensor_type, struct sensor_value *tilt_data);
void report_pickup_data(struct ssp_data *data, int sensor_type, struct sensor_value *pickup_data);
void report_scontext_data(struct ssp_data *data, int sensor_type, struct sensor_value *scontextbuf);
void report_thermistor_data(struct ssp_data *data, int sensor_type, struct sensor_value *thermistor_data);
void report_uncalib_accel_data(struct ssp_data *data, int sensor_type, struct sensor_value *acceldata);
void report_wakeup_motion_data(struct ssp_data *data, int sensor_type, struct sensor_value *wakeup_motion_data);
void report_call_gesture_data(struct ssp_data *data, int sensor_type, struct sensor_value *call_gesture_data);
void report_move_detector_data(struct ssp_data *data, int sensor_type, struct sensor_value *move_detector_data);
void report_pocket_mode_data(struct ssp_data *data, int sensor_type, struct sensor_value *pocket_data);
void report_led_cover_event_data(struct ssp_data *data, int sensor_type, struct sensor_value *led_cover_event_data);
void report_auto_rotation_data(struct ssp_data *data, int sensor_type, struct sensor_value *auto_rotation_data);
void report_sar_backoff_motion_data(struct ssp_data *data, int sensor_type, struct sensor_value *sar_backoff_motion_data);
unsigned int get_module_rev(struct ssp_data *data);
void reset_mcu(struct ssp_data *data);
int sensors_register(struct device *dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
void sensors_unregister(struct device *dev,
	struct device_attribute *attributes[]);
ssize_t mcu_reset_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t mcu_revision_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t mcu_factorytest_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size);
ssize_t mcu_factorytest_show(struct device *dev,
	struct device_attribute *attr, char *buf);
ssize_t mcu_model_name_show(struct device *dev,
	struct device_attribute *attr, char *buf);
ssize_t mcu_sleep_factorytest_show(struct device *dev,
	struct device_attribute *attr, char *buf);
ssize_t mcu_sleep_factorytest_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size);
unsigned int ssp_check_sec_dump_mode(void);

void ssp_dump_task(struct work_struct *work);
void ssp_read_big_library_task(struct work_struct *work);
void ssp_send_big_library_task(struct work_struct *work);
void ssp_pcm_dump_task(struct work_struct *work);
void ssp_temp_task(struct work_struct *work);

int callback_bbd_on_control(void *ssh_data, const char *str_ctrl);
int callback_bbd_on_mcu_ready(void *ssh_data, bool ready);
int callback_bbd_on_packet(void *ssh_data, const char *buf, size_t size);
int callback_bbd_on_packet_alarm(void *ssh_data);
int callback_bbd_on_mcu_reset(void *ssh_data, bool IsNoResp);
void bbd_on_packet_work_func(struct work_struct *work);
void bbd_mcu_ready_work_func(struct work_struct *work);
int bbd_do_transfer(struct ssp_data *data, struct ssp_msg *msg,
		struct completion *done, int timeout);
#ifdef SSP_BBD_USE_SEND_WORK
void bbd_send_packet_work_func(struct work_struct *work);
#endif	/* SSP_BBD_USE_SEND_WORK  */
#ifdef CONFIG_SSP_MOTOR_CALLBACK
void ssp_motor_work_func(struct work_struct *work);
int get_current_motor_state(void);
#endif
int set_time(struct ssp_data *data);
int get_time(struct ssp_data *data);
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
u64 get_current_timestamp(void);
void ssp_reset_batching_resources(struct ssp_data *data);
irqreturn_t ssp_shub_int_handler(int irq, void *device);
#endif

#ifdef CONFIG_PANEL_NOTIFY 
int send_panel_information(struct panel_bl_event_data *evdata);
#endif
int get_patch_version(int ap_type, int hw_rev);

int send_hall_ic_status(bool enable);

//#if defined (CONFIG_SENSORS_SSP_VLTE)
//int ssp_ckeck_lcd(int);
//#endif

int send_sensor_dump_command(struct ssp_data *data, u8 sensor_type);
int send_all_sensor_dump_command(struct ssp_data *data);

void ssp_timestamp_sync_work_func(struct work_struct *work);
void ssp_reset_work_func(struct work_struct *work);
void set_AccelCalibrationInfoData(char *pchRcvDataFrame, int *iDataIdx);
void set_GyroCalibrationInfoData(char *pchRcvDataFrame, int *iDataIdx);
int send_vdis_flag(struct ssp_data *data, bool bFlag);
void initialize_super_vdis_setting(void);

#endif
