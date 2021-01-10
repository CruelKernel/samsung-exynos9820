/* drivers/input/touchscreen/sec_ts.h
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 * http://www.samsungsemi.com/
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SEC_TS_H__
#define __SEC_TS_H__

#include <asm/unaligned.h>
#include <linux/completion.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/input/sec_cmd.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include <linux/proc_fs.h>

#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
#include <linux/input/tui_hal_ts.h>
#endif
#ifdef CONFIG_SAMSUNG_TUI
#include "stui_inf.h"
#endif

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
#include <linux/input/sec_secure_touch.h>
#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <soc/qcom/scm.h>

#define SECURE_TOUCH_ENABLE	1
#define SECURE_TOUCH_DISABLE	0
#endif

#define SEC_TS_I2C_NAME		"sec_ts"
#define SEC_TS_DEVICE_NAME	"SEC_TS"

#define USE_OPEN_CLOSE
#undef USE_RESET_DURING_POWER_ON
#undef USE_RESET_EXIT_LPM
#define USE_POR_AFTER_I2C_RETRY
#undef USER_OPEN_DWORK
#define MINORITY_REPORT

#include <linux/input/sec_tclm_v2.h>
#ifdef CONFIG_INPUT_TOUCHSCREEN_TCLMV2
#define TCLM_CONCEPT
#endif

#if defined(USE_RESET_DURING_POWER_ON) || defined(USE_POR_AFTER_I2C_RETRY) || defined(USE_RESET_EXIT_LPM)
#define USE_POWER_RESET_WORK
#endif

#define TOUCH_PRINT_INFO_DWORK_TIME	30000	/* 30s */
#define TOUCH_RESET_DWORK_TIME		10
#define BRUSH_Z_DATA		63	/* for ArtCanvas */

#define TYPE_STATUS_EVENT_CMD_DRIVEN	0
#define TYPE_STATUS_EVENT_ERR		1
#define TYPE_STATUS_EVENT_INFO		2
#define TYPE_STATUS_EVENT_USER_INPUT	3
#define TYPE_STATUS_EVENT_SPONGE_INFO	6
#define TYPE_STATUS_EVENT_VENDOR_INFO	7
#define TYPE_STATUS_CODE_SAR	0x28

#define BIT_STATUS_EVENT_CMD_DRIVEN(a)	(a << TYPE_STATUS_EVENT_CMD_DRIVEN)
#define BIT_STATUS_EVENT_ERR(a)		(a << TYPE_STATUS_EVENT_ERR)
#define BIT_STATUS_EVENT_INFO(a)	(a << TYPE_STATUS_EVENT_INFO)
#define BIT_STATUS_EVENT_USER_INPUT(a)	(a << TYPE_STATUS_EVENT_USER_INPUT)
#define BIT_STATUS_EVENT_VENDOR_INFO(a)	(a << TYPE_STATUS_EVENT_VENDOR_INFO)

#define DO_FW_CHECKSUM			(1 << 0)
#define DO_PARA_CHECKSUM		(1 << 1)
#define MAX_SUPPORT_TOUCH_COUNT		10
#define MAX_SUPPORT_HOVER_COUNT		1

#define SEC_TS_EVENTID_HOVER		10

#define TSP_PATH_EXTERNAL_FW		"/sdcard/Firmware/TSP/tsp.bin"
#define TSP_PATH_EXTERNAL_FW_SIGNED	"/sdcard/Firmware/TSP/tsp_signed.bin"
#define TSP_PATH_SPU_FW_SIGNED		"/spu/TSP/ffu_tsp.bin"

#define SEC_TS_MAX_FW_PATH		64
#define SEC_TS_FW_BLK_SIZE_MAX		(512)
#define SEC_TS_FW_BLK_SIZE_DEFAULT	(512)	// y761 & y771 ~
#define SEC_TS_SELFTEST_REPORT_SIZE	80

#define SEC_TS_FW_HEADER_SIGN		0x53494654
#define SEC_TS_FW_CHUNK_SIGN		0x53434654

#define SEC_TS_LOCATION_DETECT_SIZE	6

#define AMBIENT_CAL			0
#define OFFSET_CAL_SDC			1
#define OFFSET_CAL_SEC			2

#define SEC_TS_NVM_OFFSET_FAC_RESULT			0
#define SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT		1

/* TCLM_CONCEPT */
#define SEC_TS_NVM_OFFSET_CAL_COUNT			2
#define SEC_TS_NVM_OFFSET_TUNE_VERSION			3
#define SEC_TS_NVM_OFFSET_TUNE_VERSION_LENGTH		2

#define SEC_TS_NVM_OFFSET_CAL_POSITION			5
#define SEC_TS_NVM_OFFSET_HISTORY_QUEUE_COUNT		6
#define SEC_TS_NVM_OFFSET_HISTORY_QUEUE_LASTP		7
#define SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO		8
#define SEC_TS_NVM_OFFSET_HISTORY_QUEUE_SIZE		20

#define SEC_TS_NVM_OFFSET_CAL_FAIL_FLAG			(SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO + SEC_TS_NVM_OFFSET_HISTORY_QUEUE_SIZE + 1)
#define SEC_TS_NVM_OFFSET_CAL_FAIL_CNT			(SEC_TS_NVM_OFFSET_CAL_FAIL_FLAG + 1)
#define SEC_TS_NVM_OFFSET_LENGTH			(SEC_TS_NVM_OFFSET_CAL_FAIL_CNT + 1)

#define SEC_TS_NVM_LAST_BLOCK_OFFSET			SEC_TS_NVM_OFFSET_LENGTH
#define SEC_TS_NVM_TOTAL_OFFSET_LENGTH		(SEC_TS_NVM_LAST_BLOCK_OFFSET + 1)


#define TOUCH_TX_CHANNEL_NUM			50
#define TOUCH_RX_CHANNEL_NUM			50

/* SEC_TS READ REGISTER ADDRESS */
#define SEC_TS_CMD_SENSE_ON			0x10
#define SEC_TS_CMD_SENSE_OFF			0x11
#define SEC_TS_CMD_SW_RESET			0x12
#define SEC_TS_CMD_CALIBRATION_SEC		0x13	// send it to touch ic, but toucu ic works nothing.
#define SEC_TS_CMD_FACTORY_PANELCALIBRATION	0x14

#define SEC_TS_READ_GPIO_STATUS			0x20	// not support
#define SEC_TS_READ_FIRMWARE_INTEGRITY		0x21
#define SEC_TS_READ_DEVICE_ID			0x22
#define SEC_TS_READ_PANEL_INFO			0x23
#define SEC_TS_READ_CORE_CONFIG_VERSION		0x24

#define SEC_TS_CMD_SET_TOUCHFUNCTION		0x30
#define SEC_TS_CMD_SET_TSC_MODE			0x31
#define SET_TS_CMD_SET_CHARGER_MODE		0x32
#define SET_TS_CMD_SET_NOISE_MODE		0x33
#define SET_TS_CMD_SET_REPORT_RATE		0x34
#define SEC_TS_CMD_TOUCH_MODE_FOR_THRESHOLD	0x35
#define SEC_TS_CMD_TOUCH_THRESHOLD		0x36
#define SET_TS_CMD_KEY_THRESHOLD		0x37
#define SEC_TS_CMD_SET_COVERTYPE		0x38
#define SEC_TS_CMD_WAKEUP_GESTURE_MODE		0x39
#define SEC_TS_WRITE_POSITION_FILTER		0x3A
#define SEC_TS_CMD_WET_MODE			0x3B
#define SEC_TS_CMD_JIG_MODE			0x3C
#define SEC_TS_CMD_SET_LOW_POWER_SENSITIVITY	0x40
#define SEC_TS_CMD_ERASE_FLASH			0x45
#define SEC_TS_READ_ID				0x52
#define SEC_TS_READ_BOOT_STATUS			0x55
#define SEC_TS_CMD_ENTER_FW_MODE		0x57
#define SEC_TS_READ_ONE_EVENT			0x60
#define SEC_TS_READ_ALL_EVENT			0x61
#define SEC_TS_CMD_CLEAR_EVENT_STACK		0x62
#define SEC_TS_CMD_MUTU_RAW_TYPE		0x70
#define SEC_TS_CMD_SELF_RAW_TYPE		0x71
#define SEC_TS_READ_TOUCH_RAWDATA		0x72
#define SEC_TS_READ_TOUCH_SELF_RAWDATA		0x73
#define SEC_TS_CMD_FACTORY_LEVEL		0x74
#define SEC_TS_GET_FACTORY_DATA			0x75
#define SEC_TS_CMD_SENSITIVITY_MODE		0x77
#define SEC_TS_READ_SENSITIVITY_VALUE		0x78
#define SEC_TS_SET_FACTORY_DATA_TYPE		0x7D
#define SEC_TS_READ_PROX_INTENSITY		0x7E
#define SEC_TS_READ_SELFTEST_RESULT		0x80
#define SEC_TS_CMD_CALIBRATION_AMBIENT		0x81
#define SEC_TS_CMD_P2P_TEST			0x82
#define SEC_TS_CMD_P2P_MODE			0x83
#define SEC_TS_CMD_NVM				0x85
#define SEC_TS_CMD_STATEMANAGE_ON		0x8E
#define SEC_TS_CMD_CALIBRATION_OFFSET_SDC	0x8F
//#define SEC_TS_CMD_START_LOWPOWER_TEST	0x9B
#define SEC_TS_CMD_LPM_AOD_OFF_ON		0x9B
#define SEC_TS_CMD_SIP_MODE			0xB5
#define SEC_TS_CMD_GAME_MODE			0x47
#define SET_TS_CMD_SET_LOWTEMPERATURE_MODE	0xBE

#define SEC_TS_CMD_LPM_AOD_OFF	0x01
#define SEC_TS_CMD_LPM_AOD_ON	0x02

/* SEC_TS SPONGE OPCODE COMMAND */
#define SEC_TS_CMD_SPONGE_OFFSET_UTC			0x10
#define SEC_TS_CMD_SPONGE_PRESS_PROPERTY		0x14
#define SEC_TS_CMD_SPONGE_FOD_INFO			0x15
#define SEC_TS_CMD_SPONGE_FOD_POSITION			0x19
#define SEC_TS_CMD_SPONGE_GET_INFO			0x90
#define SEC_TS_CMD_SPONGE_WRITE_PARAM			0x91
#define SEC_TS_CMD_SPONGE_READ_PARAM			0x92
#define SEC_TS_CMD_SPONGE_NOTIFY_PACKET			0x93
#define SEC_TS_CMD_SPONGE_LP_DUMP			0xF0

#define SEC_TS_CMD_STATUS_EVENT_TYPE	0xA0
#define SEC_TS_READ_FW_INFO		0xA2
#define SEC_TS_READ_FW_VERSION		0xA3
#define SEC_TS_READ_PARA_VERSION	0xA4
#define SEC_TS_READ_IMG_VERSION		0xA5
#define SEC_TS_CMD_GET_CHECKSUM		0xA6
#define SEC_TS_CMD_DEADZONE_RANGE	0xAA
#define SEC_TS_CMD_LONGPRESSZONE_RANGE	0xAB
#define SEC_TS_CMD_LONGPRESS_DROP_AREA	0xAC
#define SEC_TS_CMD_LONGPRESS_DROP_DIFF	0xAD
#define SEC_TS_READ_TS_STATUS		0xAF
#define SEC_TS_CMD_SELFTEST		0xAE
#define SEC_TS_CMD_SET_GET_FACTORY_MODE	0xB2
#define SEC_TS_CMD_INPUT_GPIO_CONTROL	0xBF

/* SEC_TS FLASH COMMAND */
#define SEC_TS_CMD_FLASH_READ_ADDR	0xD0
#define SEC_TS_CMD_FLASH_READ_SIZE	0xD1
#define SEC_TS_CMD_FLASH_READ_DATA	0xD2
#define SEC_TS_CMD_CHG_SYSMODE		0xD7
#define SEC_TS_CMD_FLASH_ERASE		0xD8
#define SEC_TS_CMD_FLASH_WRITE		0xD9
#define SEC_TS_CMD_FLASH_PADDING	0xDA

#define SEC_TS_READ_BL_UPDATE_STATUS	0xDB
#define SEC_TS_CMD_SET_POWER_MODE	0xE4
#define SEC_TS_CMD_EDGE_DEADZONE	0xE5
#define SEC_TS_CMD_SET_MONITOR_NOISE_MODE	0xE7
#define SEC_TS_CMD_SET_USER_PRESSURE		0xEB
#define SEC_TS_CMD_SET_TEMPERATURE_COMP_MODE	0xEC
#define SEC_TS_CMD_SET_TOUCHABLE_AREA		0xED
#define SEC_TS_CMD_SET_BRUSH_MODE		0xEF

#define SEC_TS_READ_CALIBRATION_REPORT		0xF1
#define SEC_TS_CMD_SET_VENDOR_EVENT_LEVEL	0xF2
#define SEC_TS_CMD_SET_SCAN_MODE		0xF3

#define SEC_TS_CMD_SET_MISCAL_THD		0xA9
#define SEC_TS_CMD_RUN_MISCAL			0xA7
#define SEC_TS_CMD_GET_MISCAL_RESULT		0xA8

#define SEC_TS_FLASH_SIZE_64		64
#define SEC_TS_FLASH_SIZE_128		128
#define SEC_TS_FLASH_SIZE_256		256

#define SEC_TS_FLASH_SIZE_CMD		1
#define SEC_TS_FLASH_SIZE_ADDR		2
#define SEC_TS_FLASH_SIZE_CHECKSUM	1

#define SEC_TS_STATUS_BOOT_MODE		0x10
#define SEC_TS_STATUS_APP_MODE		0x20

#define SEC_TS_FIRMWARE_PAGE_SIZE_256	256
#define SEC_TS_FIRMWARE_PAGE_SIZE_128	128

/* SEC status event id */
#define SEC_TS_COORDINATE_EVENT		0
#define SEC_TS_STATUS_EVENT		1
#define SEC_TS_GESTURE_EVENT		2
#define SEC_TS_EMPTY_EVENT		3

#define SEC_TS_EVENT_BUFF_SIZE		16

#define SEC_TS_COORDINATE_ACTION_NONE		0
#define SEC_TS_COORDINATE_ACTION_PRESS		1
#define SEC_TS_COORDINATE_ACTION_MOVE		2
#define SEC_TS_COORDINATE_ACTION_RELEASE	3

#define SEC_TS_TOUCHTYPE_NORMAL		0
#define SEC_TS_TOUCHTYPE_HOVER		1
#define SEC_TS_TOUCHTYPE_FLIPCOVER	2
#define SEC_TS_TOUCHTYPE_GLOVE		3
#define SEC_TS_TOUCHTYPE_STYLUS		4
#define SEC_TS_TOUCHTYPE_PALM		5
#define SEC_TS_TOUCHTYPE_WET		6
#define SEC_TS_TOUCHTYPE_PROXIMITY	7
#define SEC_TS_TOUCHTYPE_JIG		8

/* SEC_TS_GESTURE_TYPE */
#define SEC_TS_GESTURE_CODE_SWIPE		0x00
#define SEC_TS_GESTURE_CODE_DOUBLE_TAP		0x01
#define SEC_TS_GESTURE_CODE_PRESS		0x03
#define SEC_TS_GESTURE_CODE_SINGLE_TAP		0x04

/* SEC_TS_GESTURE_ID */
#define SEC_TS_GESTURE_ID_AOD			0x00
#define SEC_TS_GESTURE_ID_DOUBLETAP_TO_WAKEUP	0x01

/* SEC_TS_INFO : Info acknowledge event */
#define SEC_TS_ACK_BOOT_COMPLETE	0x00
#define SEC_TS_ACK_WET_MODE	0x1

/* SEC_TS_VENDOR_INFO : Vendor acknowledge event */
#define SEC_TS_VENDOR_ACK_OFFSET_CAL_DONE		0x40
#define SEC_TS_VENDOR_ACK_SELF_TEST_DONE		0x41
#define SEC_TS_VENDOR_ACK_CMR_TEST_DONE			0x42	/* mutual */
#define SEC_TS_VENDOR_ACK_CSR_TX_TEST_DONE		0x43	/* self_tx */
#define SEC_TS_VENDOR_ACK_CSR_RX_TEST_DONE		0x44	/* self_rx */
#define SEC_TS_VENDOR_ACK_CMR_KEY_TEST_DONE		0x46	/* mutual_key */
#define SEC_TS_VENDOR_ACK_RX_NODE_GAP_TEST_DONE		0x47	/* mutual_key */

#define SEC_TS_VENDOR_ACK_LOWPOWER_SELF_TEST_DONE	0x58
#define SEC_TS_VENDOR_STATE_CHANGED			0x61
#define SEC_TS_VENDOR_ACK_NOISE_STATUS_NOTI		0x64
#define SEC_TS_VENDOR_ACK_PRE_NOISE_STATUS_NOTI		0x6D

/* SEC_TS_ERROR : Error event */
#define SEC_TS_ERR_EVNET_CORE_ERR	0x0
#define SEC_TS_ERR_EVENT_QUEUE_FULL	0x01
#define SEC_TS_ERR_EVENT_ESD		0x2

/* SEC_TS_DEBUG : Print event contents */
#define SEC_TS_DEBUG_PRINT_ALLEVENT		0x01
#define SEC_TS_DEBUG_PRINT_ONEEVENT		0x02
#define SEC_TS_DEBUG_PRINT_I2C_READ_CMD		0x04
#define SEC_TS_DEBUG_PRINT_I2C_WRITE_CMD	0x08
#define SEC_TS_DEBUG_SEND_UEVENT		0x80

/* SEC_OFFSET_SIGNUTRE */
#define SEC_CM_HIST_DATA_SIZE			3 * 2 * 48	/* (CM1/2/3)*2 */
#define SEC_OFFSET_SIGNATURE			0x59525446
#define SEC_CM2_SIGNATURE			0x324D5446
#define SEC_CM3_SIGNATURE			0x334D5446
#define SEC_FAIL_HIST_SIGNATURE			0x53484646

#define SEC_TS_BIT_SETFUNC_TOUCH		(1 << 0)
#define SEC_TS_BIT_SETFUNC_MUTUAL		(1 << 0)
#define SEC_TS_BIT_SETFUNC_HOVER		(1 << 1)
#define SEC_TS_BIT_SETFUNC_COVER		(1 << 2)
#define SEC_TS_BIT_SETFUNC_GLOVE		(1 << 3)
#define SEC_TS_BIT_SETFUNC_STYLUS		(1 << 4)
#define SEC_TS_BIT_SETFUNC_PALM			(1 << 5)
#define SEC_TS_BIT_SETFUNC_WET			(1 << 6)
#define SEC_TS_BIT_SETFUNC_PROXIMITY		(1 << 7)

#define SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC	(SEC_TS_BIT_SETFUNC_TOUCH | SEC_TS_BIT_SETFUNC_PALM | SEC_TS_BIT_SETFUNC_WET)

#define SEC_TS_BIT_CHARGER_MODE_NO			(0x1 << 0)
#define SEC_TS_BIT_CHARGER_MODE_WIRE_CHARGER		(0x1 << 1)
#define SEC_TS_BIT_CHARGER_MODE_WIRELESS_CHARGER	(0x1 << 2)
#define SEC_TS_BIT_CHARGER_MODE_WIRELESS_BATTERY_PACK	(0x1 << 3)

#define STATE_MANAGE_ON			1
#define STATE_MANAGE_OFF		0

#define SEC_TS_STATUS_NOT_CALIBRATION	0x50
#define SEC_TS_STATUS_CALIBRATION_SDC	0xA1
#define SEC_TS_STATUS_CALIBRATION_SEC	0xA2

#define SEC_TS_CMD_EDGE_HANDLER		0xAA
#define SEC_TS_CMD_EDGE_AREA		0xAB
#define SEC_TS_CMD_DEAD_ZONE		0xAC
#define SEC_TS_CMD_LANDSCAPE_MODE	0xAD

#define SEC_TS_SET_EAR_DETECT_MODE	0xEA
#define STATUS_EVENT_VENDOR_PROXIMITY	0x6A

#define SEC_TS_CMD_PROX_POWER_OFF	0xBD

/* SPEN mode */
#define SPEN_DISABLE_MODE	0x00
#define SPEN_ENABLE_MODE	0x01
#define SPEN_NOISE_MODE		0x02

enum grip_write_mode {
	G_NONE				= 0,
	G_SET_EDGE_HANDLER		= 1,
	G_SET_EDGE_ZONE			= 2,
	G_SET_NORMAL_MODE		= 4,
	G_SET_LANDSCAPE_MODE	= 8,
	G_CLR_LANDSCAPE_MODE	= 16,
};
enum grip_set_data {
	ONLY_EDGE_HANDLER		= 0,
	GRIP_ALL_DATA			= 1,
};

typedef enum {
	SEC_TS_STATE_POWER_OFF = 0,
	SEC_TS_STATE_LPM,
	SEC_TS_STATE_POWER_ON
} TOUCH_POWER_MODE;

typedef enum {
	TOUCH_SYSTEM_MODE_BOOT		= 0,
	TOUCH_SYSTEM_MODE_CALIBRATION	= 1,
	TOUCH_SYSTEM_MODE_TOUCH		= 2,
	TOUCH_SYSTEM_MODE_SELFTEST	= 3,
	TOUCH_SYSTEM_MODE_FLASH		= 4,
	TOUCH_SYSTEM_MODE_LOWPOWER	= 5,
	TOUCH_SYSTEM_MODE_LISTEN
} TOUCH_SYSTEM_MODE;

typedef enum {
	TOUCH_MODE_STATE_IDLE		= 0,
	TOUCH_MODE_STATE_HOVER		= 1,
	TOUCH_MODE_STATE_TOUCH		= 2,
	TOUCH_MODE_STATE_NOISY		= 3,
	TOUCH_MODE_STATE_CAL		= 4,
	TOUCH_MODE_STATE_CAL2		= 5,
	TOUCH_MODE_STATE_WAKEUP		= 10
} TOUCH_MODE_STATE;

enum switch_system_mode {
	TO_TOUCH_MODE			= 0,
	TO_LOWPOWER_MODE		= 1,
	TO_SELFTEST_MODE		= 2,
	TO_FLASH_MODE			= 3,
};

enum external_noise_mode {
	EXT_NOISE_MODE_NONE		= 0,
	EXT_NOISE_MODE_MONITOR		= 1,	/* for dex mode */
	EXT_NOISE_MODE_MAX,			/* add new mode above this line */
};

enum wireless_charger_param {
	TYPE_WIRELESS_CHARGER_NONE	= 0,
	TYPE_WIRELESS_CHARGER		= 1,
};

enum {
	TYPE_RAW_DATA			= 0,	/* Total - Offset : delta data */
	TYPE_SIGNAL_DATA		= 1,	/* Signal - Filtering & Normalization */
	TYPE_AMBIENT_BASELINE		= 2,	/* Cap Baseline */
	TYPE_AMBIENT_DATA		= 3,	/* Cap Ambient */
	TYPE_REMV_BASELINE_DATA		= 4,
	TYPE_DECODED_DATA		= 5,	/* Raw */
	TYPE_REMV_AMB_DATA		= 6,	/*  TYPE_RAW_DATA - TYPE_AMBIENT_DATA */
	TYPE_OFFSET_DATA_SEC		= 19,	/* Cap Offset in SEC Manufacturing Line */
	TYPE_OFFSET_DATA_SDC		= 29,	/* Cap Offset in SDC Manufacturing Line */
	TYPE_RAW_DATA_P2P_AVG		= 30,	/* Raw p2p avg data for 50 frame */
	TYPE_RAW_DATA_P2P_DIFF		= 31,	/* Raw p2p/diff data for 50 frame */
	TYPE_RAW_DATA_NODE_GAP_Y	= 32,	/* Raw p2p gap y data for 50 frame */
	TYPE_RAWDATA_MAX,
	TYPE_INVALID_DATA		= 0xFF,	/* Invalid data type for release factory mode */
};

typedef enum {
	SPONGE_EVENT_TYPE_SPAY			= 0x04,
	SPONGE_EVENT_TYPE_SINGLE_TAP		= 0x08,
	SPONGE_EVENT_TYPE_AOD_PRESS		= 0x09,
	SPONGE_EVENT_TYPE_AOD_LONGPRESS		= 0x0A,
	SPONGE_EVENT_TYPE_AOD_DOUBLETAB		= 0x0B,
	SPONGE_EVENT_TYPE_AOD_HOMEKEY_PRESS	= 0x0C,
	SPONGE_EVENT_TYPE_AOD_HOMEKEY_RELEASE	= 0x0D,
	SPONGE_EVENT_TYPE_AOD_HOMEKEY_RELEASE_NO_HAPTIC	= 0x0E
} SPONGE_EVENT_TYPE;

#define EVENT_TYPE_TSP_SCAN_UNBLOCK	0xE1;
#define EVENT_TYPE_TSP_SCAN_BLOCK	0xE2;

#define CMD_RESULT_WORD_LEN		10

#define SEC_TS_I2C_RETRY_CNT		3
#define SEC_TS_WAIT_RETRY_CNT		100

#define SEC_TS_MODE_SPONGE_SWIPE		(1 << 1)
#define SEC_TS_MODE_SPONGE_AOD			(1 << 2)
#define SEC_TS_MODE_SPONGE_SINGLE_TAP		(1 << 3)
#define SEC_TS_MODE_SPONGE_PRESS		(1 << 4)
#define SEC_TS_MODE_SPONGE_DOUBLETAP_TO_WAKEUP	(1 << 5)

enum sec_ts_cover_id {
	SEC_TS_FLIP_WALLET = 0,
	SEC_TS_VIEW_COVER,
	SEC_TS_COVER_NOTHING1,
	SEC_TS_VIEW_WIRELESS,
	SEC_TS_COVER_NOTHING2,
	SEC_TS_CHARGER_COVER,
	SEC_TS_VIEW_WALLET,
	SEC_TS_LED_COVER,
	SEC_TS_CLEAR_FLIP_COVER,
	SEC_TS_QWERTY_KEYBOARD_KOR,
	SEC_TS_QWERTY_KEYBOARD_US,
	SEC_TS_NEON_COVER,
	SEC_TS_ALCANTARA_COVER,
	SEC_TS_GAMEPACK_COVER,
	SEC_TS_LED_BACK_COVER,
	SEC_TS_CLEAR_SIDE_VIEW_COVER,
	SEC_TS_MONTBLANC_COVER = 100,
};

enum sec_fw_update_status {
	SEC_NOT_UPDATE = 0,
	SEC_NEED_FW_UPDATE,
	SEC_NEED_CALIBRATION_ONLY,
	SEC_NEED_FW_UPDATE_N_CALIBRATION,
};

enum tsp_hw_parameter {
	TSP_ITO_CHECK		= 1,
	TSP_RAW_CHECK		= 2,
	TSP_MULTI_COUNT		= 3,
	TSP_WET_MODE		= 4,
	TSP_COMM_ERR_COUNT	= 5,
	TSP_MODULE_ID		= 6,
};

#define TEST_MODE_MIN_MAX		false
#define TEST_MODE_ALL_NODE		true
#define TEST_MODE_READ_FRAME		false
#define TEST_MODE_READ_CHANNEL		true

enum offset_fac_position {
	OFFSET_FAC_NOSAVE		= 0,	// FW index 0
	OFFSET_FAC_SUB			= 1,	// FW Index 2
	OFFSET_FAC_MAIN			= 2,	// FW Index 3
	OFFSET_FAC_SVC			= 3,	// FW Index 4
};

enum offset_fw_position {
	OFFSET_FW_NOSAVE		= 0,
	OFFSET_FW_SDC			= 1,
	OFFSET_FW_SUB			= 2,
	OFFSET_FW_MAIN			= 3,
	OFFSET_FW_SVC			= 4,
};

enum offset_fac_data_type {
	OFFSET_FAC_DATA_NO			= 0,
	OFFSET_FAC_DATA_CM			= 1,
	OFFSET_FAC_DATA_CM2			= 2,
	OFFSET_FAC_DATA_CM3			= 3,
	OFFSET_FAC_DATA_SELF_FAIL	= 7,
};

/* factory test mode */
struct sec_ts_test_mode {
	u8 type;
	short min;
	short max;
	bool allnode;
	bool frame_channel;
};

struct sec_ts_fw_file {
	u8 *data;
	u32 pos;
	size_t size;
};

/*
 * write 0xE4 [ 11 | 10 | 01 | 00 ]
 * MSB <-------------------> LSB
 * read 0xE4
 * mapping sequnce : LSB -> MSB
 * struct sec_ts_test_result {
 * * assy : front + OCTA assay
 * * module : only OCTA
 *	 union {
 *		 struct {
 *			 u8 assy_count:2;		-> 00
 *			 u8 assy_result:2;		-> 01
 *			 u8 module_count:2;	-> 10
 *			 u8 module_result:2;	-> 11
 *		 } __attribute__ ((packed));
 *		 unsigned char data[1];
 *	 };
 *};
 */
struct sec_ts_test_result {
	union {
		struct {
			u8 assy_count:2;
			u8 assy_result:2;
			u8 module_count:2;
			u8 module_result:2;
		} __attribute__ ((packed));
		unsigned char data[1];
	};
};

/* 48 byte */
struct sec_ts_selftest_fail_hist {
	u32 tsp_signature;
	u32 tsp_fw_version;
	u8 fail_cnt1;
	u8 fail_cnt2;
	u16 selftest_exec_parm;
	u32 test_result;
	u8 fail_data[8];
	u32 fail_type:8;
	u32 reserved:24;
	u32 defective_data[5];
} __attribute__ ((packed));

/* 16 byte */
struct sec_ts_gesture_status {
	u8 eid:2;
	u8 stype:4;
	u8 sf:2;
	u8 gesture_id;
	u8 gesture_data_1;
	u8 gesture_data_2;
	u8 gesture_data_3;
	u8 gesture_data_4;
	u8 reserved_1;
	u8 left_event_5_0:5;
	u8 reserved_2:3;
	u8 noise_level;
	u8 max_strength;
	u8 hover_id_num:4;
	u8 reserved10:4;
	u8 reserved11;
	u8 reserved12;
	u8 reserved13;
	u8 reserved14;
	u8 reserved15;
} __attribute__ ((packed));

/* 16 byte */
struct sec_ts_event_status {
	u8 eid:2;
	u8 stype:4;
	u8 sf:2;
	u8 status_id;
	u8 status_data_1;
	u8 status_data_2;
	u8 status_data_3;
	u8 status_data_4;
	u8 status_data_5;
	u8 left_event_5_0:5;
	u8 reserved_2:3;
	u8 noise_level;
	u8 max_strength;
	u8 hover_id_num:4;
	u8 reserved10:4;
	u8 reserved11;
	u8 reserved12;
	u8 reserved13;
	u8 reserved14;
	u8 reserved15;
} __attribute__ ((packed));

/* 16 byte */
struct sec_ts_event_coordinate {
	u8 eid:2;
	u8 tid:4;
	u8 tchsta:2;
	u8 x_11_4;
	u8 y_11_4;
	u8 y_3_0:4;
	u8 x_3_0:4;
	u8 major;
	u8 minor;
	u8 z:6;
	u8 ttype_3_2:2;
//	u8 left_event:6;
	u8 left_event:5;
	u8 max_energy_flag:1;
	u8 ttype_1_0:2;
	u8 noise_level;
	u8 max_strength;
	u8 hover_id_num:4;
	u8 noise_status:2;
	u8 reserved10:2;
	u8 reserved11;
	u8 reserved12;
	u8 reserved13;
	u8 reserved14;
	u8 reserved15;
} __attribute__ ((packed));

/* not fixed */
struct sec_ts_coordinate {
	u8 id;
	u8 ttype;
	u8 action;
	u16 x;
	u16 y;
	u16 p_x;
	u16 p_y;
	u8 z;
	u8 hover_flag;
	u8 glove_flag;
	u8 touch_height;
	u16 mcount;
	u8 major;
	u8 minor;
	bool palm;
	int palm_count;
	u8 left_event;
	u16 max_energy_x;
	u16 max_energy_y;
	u8 noise_level;
	u8 max_strength;
	u8 hover_id_num;
	u8 noise_status;
};


struct sec_ts_data {
	u32 crc_addr;
	u32 fw_addr;
	u32 para_addr;
	u32 flash_page_size;
	u8 boot_ver[3];

	struct device *dev;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct input_dev *input_dev_pad;
	struct input_dev *input_dev_touch;
	struct input_dev *input_dev_proximity;
	struct sec_ts_plat_data *plat_data;
	struct sec_ts_coordinate coord[MAX_SUPPORT_TOUCH_COUNT + MAX_SUPPORT_HOVER_COUNT];


	u8 lowpower_mode;
	u8 brush_mode;
	u8 touchable_area;
	u8 external_noise_mode;
	volatile u8 touch_noise_status;
	volatile u8 touch_pre_noise_status;
	volatile bool input_closed;
	long prox_power_off;

	int touch_count;
	int tx_count;
	int rx_count;
	int i2c_burstmax;
	int ta_status;
	volatile int power_status;
	int raw_status;
	u16 touch_functions;
	u16 ic_status;
	u8 charger_mode;
	bool force_charger_mode;
	struct sec_ts_event_coordinate touchtype;
	u8 gesture_status[6];
	u8 cal_status;
	struct mutex device_mutex;
	struct mutex i2c_mutex;
	struct mutex eventlock;
	struct mutex modechange;
	struct mutex sponge_mutex;

	int nv;
	int disassemble_count;

	struct delayed_work work_read_info;
	struct delayed_work work_print_info;
	u32	print_info_cnt_open;
	u32	print_info_cnt_release;
	u16	print_info_currnet_mode;
#ifdef USE_POWER_RESET_WORK
	struct delayed_work reset_work;
	volatile bool reset_is_on_going;
#endif
	struct delayed_work work_read_functions;
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	atomic_t secure_enabled;
	atomic_t secure_pending_irqs;
	struct completion secure_powerdown;
	struct completion secure_interrupt;
#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
	struct completion st_irq_received;
#endif
#endif
	struct completion resume_done;
	struct wake_lock wakelock;
	struct sec_cmd_data sec;
	short *pFrame;

	bool probe_done;
	bool info_work_done;
	volatile bool shutdown_is_on_going;
	int cover_type;
	u8 cover_cmd;
	u16 rect_data[4];
	u16 fod_rect_data[4];
	bool fod_set_val;

	int tspid_val;
	int tspicid_val;
	bool use_sponge;
	unsigned int scrub_id;
	unsigned int scrub_x;
	unsigned int scrub_y;

	unsigned int spen_mode_val;
	u8 tsp_temp_data;
	bool tsp_temp_data_skip;

	u8 grip_edgehandler_direction;
	int grip_edgehandler_start_y;
	int grip_edgehandler_end_y;
	u16 grip_edge_range;
	u8 grip_deadzone_up_x;
	u8 grip_deadzone_dn_x;
	int grip_deadzone_y;
	u8 grip_landscape_mode;
	int grip_landscape_edge;
	u16 grip_landscape_deadzone;
	u16 grip_landscape_top_deadzone;
	u16 grip_landscape_bottom_deadzone;
	u16 grip_landscape_top_gripzone;
	u16 grip_landscape_bottom_gripzone;

	struct delayed_work ghost_check;
	u8 tsp_dump_lock;

	struct sec_tclm_data *tdata;
	bool is_cal_done;

	volatile int wet_mode;
	bool factory_level;
	u8 factory_position;

	u8 ed_enable;
	u16 proximity_thd;
	bool proximity_jig_mode; 
	u8 sip_mode;

	unsigned char ito_test[4];		/* ito panel tx/rx chanel */
	unsigned char check_multi;
	unsigned int multi_count;		/* multi touch count */
	unsigned int wet_count;			/* wet mode count */
	unsigned int noise_count;		/* noise mode count */
	unsigned int dive_count;		/* dive mode count */
	unsigned int comm_err_count;	/* i2c comm error count */
	unsigned int checksum_result;	/* checksum result */
	unsigned char module_id[4];
	unsigned int all_finger_count;
	unsigned int all_aod_tap_count;
	unsigned int all_spay_count;
	int max_ambient;
	int max_ambient_channel_tx;
	int max_ambient_channel_rx;
	int min_ambient;
	int min_ambient_channel_tx;
	int min_ambient_channel_rx;
	int mode_change_failed_count;
	int ic_reset_count;

	/* average value for each channel */
	short ambient_tx[TOUCH_TX_CHANNEL_NUM];
	short ambient_rx[TOUCH_RX_CHANNEL_NUM];

	/* max - min value for each channel */
	short ambient_tx_delta[TOUCH_TX_CHANNEL_NUM];
	short ambient_rx_delta[TOUCH_RX_CHANNEL_NUM];

	/* for factory - factory_cmd_result_all() */
	short cm_raw_set_avg_min;
	short cm_raw_set_avg_max;
	short cm_raw_set_p2p;
	
	short self_raw_set_avg_tx_min;
	short self_raw_set_avg_tx_max;
	short self_raw_set_avg_rx_min;
	short self_raw_set_avg_rx_max;
	short self_raw_set_p2p_tx_diff;
	short self_raw_set_p2p_rx_diff;

	short cm_raw_key_p2p_min;
	short cm_raw_key_p2p_max;
	short cm_raw_key_p2p_diff;
	short cm_raw_key_p2p_diff_data[2][3];	/* key : max support key is 3 */
	short cm_raw_set_p2p_gap_y;

	u32	defect_probability;
#ifdef MINORITY_REPORT
	u8	item_ito;
	u8	item_rawdata;
	u8	item_crc;
	u8	item_i2c_err;
	u8	item_wet;
#endif

	int debug_flag;
	int fix_active_mode;

	u8 lp_sensitivity;

	u8 fod_vi_size;
	u8 press_prop;
/* thermistor */
	union power_supply_propval psy_value;
	struct power_supply *psy;

	int proc_cmoffset_size;
	int proc_cmoffset_all_size;
	char *cmoffset_sdc_proc;
	char *cmoffset_sub_proc;
	char *cmoffset_main_proc;
	char *cmoffset_all_proc;

	int proc_fail_hist_size;
	int proc_fail_hist_all_size;
	char *fail_hist_sdc_proc;
	char *fail_hist_sub_proc;
	char *fail_hist_main_proc;
	char *fail_hist_all_proc;

	int (*sec_ts_i2c_write)(struct sec_ts_data *ts, u8 reg, u8 *data, int len);
	int (*sec_ts_i2c_read)(struct sec_ts_data *ts, u8 reg, u8 *data, int len);
	int (*sec_ts_i2c_write_burst)(struct sec_ts_data *ts, u8 *data, int len);
	int (*sec_ts_i2c_read_bulk)(struct sec_ts_data *ts, u8 *data, int len);
	int (*sec_ts_read_sponge)(struct sec_ts_data *ts, u8 *data, int len);
	int (*sec_ts_write_sponge)(struct sec_ts_data *ts, u8 *data, int len);
};

struct sec_ts_plat_data {
	int max_x;
	int max_y;
	int display_x;
	int display_y;
	unsigned irq_gpio;
	int irq_type;
	int i2c_burstmax;
	int bringup;
	u32	area_indicator;
	u32	area_navigation;
	u32	area_edge;

	const char *firmware_name;
	const char *model_name;
	const char *project_name;
	const char *regulator_dvdd;
	const char *regulator_avdd;

	u8 core_version_of_ic[4];
	u8 core_version_of_bin[4];
	u8 config_version_of_ic[4];
	u8 config_version_of_bin[4];
	u8 img_version_of_ic[4];
	u8 img_version_of_bin[4];

	struct pinctrl *pinctrl;

	int (*power)(void *data, bool on);
	int tsp_icid;
	int tsp_id;

	bool regulator_boot_on;
	bool support_mt_pressure;
	bool support_dex;
	bool support_ear_detect;
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	int ss_touch_num;
#endif
	bool support_fod;
	bool enable_settings_aot;
	bool sync_reportrate_120;
	bool support_open_short_test;
	bool support_mis_calibration_test;
};

typedef struct {
	u32 signature;			/* signature */
	u32 version;			/* version */
	u32 totalsize;			/* total size */
	u32 checksum;			/* checksum */
	u32 img_ver;			/* image file version */
	u32 img_date;			/* image file date */
	u32 img_description;		/* image file description */
	u32 fw_ver;			/* firmware version */
	u32 fw_date;			/* firmware date */
	u32 fw_description;		/* firmware description */
	u32 para_ver;			/* parameter version */
	u32 para_date;			/* parameter date */
	u32 para_description;		/* parameter description */
	u32 num_chunk;			/* number of chunk */
	u32 reserved1;
	u32 reserved2;
} fw_header;

typedef struct {
	u32 signature;
	u32 addr;
	u32 size;
	u32 reserved;
} fw_chunk;

int sec_ts_power(void *data, bool on);
int sec_ts_stop_device(struct sec_ts_data *ts);
int sec_ts_start_device(struct sec_ts_data *ts);
int sec_ts_set_lowpowermode(struct sec_ts_data *ts, u8 mode);
int sec_ts_set_external_noise_mode(struct sec_ts_data *ts, u8 mode);
int sec_ts_firmware_update_on_probe(struct sec_ts_data *ts, bool force_update);
int sec_ts_firmware_update_on_hidden_menu(struct sec_ts_data *ts, int update_type);
int sec_ts_glove_mode_enables(struct sec_ts_data *ts, int mode);
int sec_ts_set_cover_type(struct sec_ts_data *ts, bool enable);
int sec_ts_wait_for_ready(struct sec_ts_data *ts, unsigned int ack);
int sec_ts_fn_init(struct sec_ts_data *ts);
int sec_ts_read_calibration_report(struct sec_ts_data *ts);
int sec_ts_fix_tmode(struct sec_ts_data *ts, u8 mode, u8 state);
int sec_ts_release_tmode(struct sec_ts_data *ts);
int sec_ts_set_custom_library(struct sec_ts_data *ts);
int sec_ts_set_aod_rect(struct sec_ts_data *ts);
int sec_ts_set_fod_rect(struct sec_ts_data *ts);
int sec_ts_set_temp(struct sec_ts_data *ts, bool bforced);

int sec_ts_check_custom_library(struct sec_ts_data *ts);
int sec_ts_set_touch_function(struct sec_ts_data *ts);

void sec_ts_locked_release_all_finger(struct sec_ts_data *ts);
void sec_ts_unlocked_release_all_finger(struct sec_ts_data *ts);

void sec_ts_fn_remove(struct sec_ts_data *ts);
void sec_ts_delay(unsigned int ms);
int sec_ts_read_information(struct sec_ts_data *ts);

#ifdef MINORITY_REPORT
void minority_report_sync_latest_value(struct sec_ts_data *ts);
#endif

#ifdef TCLM_CONCEPT
int sec_tclm_data_read(struct i2c_client *client, int address);
int sec_tclm_data_write(struct i2c_client *client, int address);
int sec_tclm_execute_force_calibration(struct i2c_client *client, int cal_mode);
#endif
int get_tsp_nvm_data(struct sec_ts_data *ts, u8 offset);

void sec_ts_run_rawdata_all(struct sec_ts_data *ts, bool full_read);

void sec_ts_reinit(struct sec_ts_data *ts);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
int sec_ts_raw_device_init(struct sec_ts_data *ts);
#endif

#define UEVENT_OPEN_SHORT_PASS		1
#define UEVENT_OPEN_SHORT_FAIL		2
#define UEVENT_TSP_I2C_ERROR		3
#define UEVENT_TSP_I2C_RESET		4
void send_event_to_user(struct sec_ts_data *ts, int number, int val);

#if defined(CONFIG_DISPLAY_SAMSUNG)
int get_lcd_attached(char *mode);
#endif

#if defined(CONFIG_EXYNOS_DPU20)
int get_lcd_info(char *arg);
#endif

extern struct sec_ts_data *ts_dup;

#ifdef CONFIG_BATTERY_SAMSUNG
extern unsigned int lpcharge;
#endif

void set_grip_data_to_ic(struct sec_ts_data *ts, u8 flag);
void sec_ts_set_grip_type(struct sec_ts_data *ts, u8 set_type);


ssize_t get_cmoffset_dump_all(struct sec_ts_data *ts, char *buf, u8 position);
ssize_t get_selftest_fail_hist_dump_all(struct sec_ts_data *ts, char *buf, u8 position);
void sec_ts_ioctl_init(struct sec_ts_data *ts);
void sec_ts_ioctl_remove(struct sec_ts_data *ts);

int sec_ts_set_press_property(struct sec_ts_data *ts);

#endif
