/*
 * p9220_charger.h
 * Samsung p9220 Charger Header
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __p9220_CHARGER_H
#define __p9220_CHARGER_H __FILE__

#include <linux/mfd/core.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include "../sec_charging_common.h"

/* REGISTER MAPS */
#define P9220_CHIP_ID_L_REG					0x00
#define P9220_CHIP_ID_H_REG					0x01
#define P9220_CHIP_REVISION_REG				0x02
#define P9220_CUSTOMER_ID_REG				0x03
#define P9220_OTP_FW_MAJOR_REV_L_REG		0x04
#define P9220_OTP_FW_MAJOR_REV_H_REG		0x05
#define P9220_OTP_FW_MINOR_REV_L_REG		0x06
#define P9220_OTP_FW_MINOR_REV_H_REG		0x07
#define P9220_SRAM_FW_MAJOR_REV_L_REG		0x1C
#define P9220_SRAM_FW_MAJOR_REV_H_REG		0x1D
#define P9220_SRAM_FW_MINOR_REV_L_REG		0x1E
#define P9220_SRAM_FW_MINOR_REV_H_REG		0x1F
#define P9220_EPT_REG						0x06
#define P9220_ADC_VRECT_REG					0x07
#define P9220_ADC_IOUT_REG					0x08
#define P9220_ADC_VOUT_REG					0x09
#define P9220_ADC_DIE_TEMP_REG				0x0A
#define P9220_ADC_ALLGN_X_REG				0x0B
#define P9220_ADC_ALIGN_Y_REG				0x0C
#define P9220_INT_STATUS_L_REG				0x34
#define P9220_INT_STATUS_H_REG				0x35
#define P9220_INT_L_REG						0x36
#define P9220_INT_H_REG						0x37
#define P9220_INT_ENABLE_L_REG				0x38
#define P9220_INT_ENABLE_H_REG				0x39
#define P9220_CHG_STATUS_REG				0x3A
#define P9220_END_POWER_TRANSFER_REG		0x3B
#define P9220_ADC_VOUT_L_REG				0x3C
#define P9220_ADC_VOUT_H_REG				0x3D
#define P9220_VOUT_SET_REG					0x3E
#define P9220_VRECT_SET_REG					0x3F
#define P9220_ADC_VRECT_L_REG				0x40
#define P9220_ADC_VRECT_H_REG				0x41
#define P9220_ADC_TX_ISENSE_L_REG			0x42
#define P9220_ADC_TX_ISENSE_H_REG			0x43
#define P9220_ADC_RX_IOUT_L_REG				0x44
#define P9220_ADC_RX_IOUT_H_REG				0x45
#define P9220_ADC_DIE_TEMP_L_REG			0x46
#define P9220_ADC_DIE_TEMP_H_REG			0x47
#define P9220_OP_FREQ_L_REG					0x48
#define P9220_OP_FREQ_H_REG					0x49
#define P9220_ILIM_SET_REG					0x4A
#define P9220_ADC_ALLIGN_X_REG				0x4B
#define P9220_ADC_ALLIGN_Y_REG				0x4C
#define P9220_SYS_OP_MODE_REG				0x4D
#define P9220_COMMAND_REG					0x4E
#define P9220_PACKET_HEADER					0x50
#define P9220_RX_DATA_COMMAND				0x51
#define P9220_RX_DATA_VALUE0				0x52
#define P9220_RX_DATA_VALUE1				0x53
#define P9220_RX_DATA_VALUE2				0x54
#define P9220_RX_DATA_VALUE3				0x55
#define P9220_INT_CLEAR_L_REG				0x56
#define P9220_INT_CLEAR_H_REG				0x57
#define P9220_TX_DATA_COMMAND				0x58
#define P9220_TX_DATA_VALUE0				0x59
#define P9220_TX_DATA_VALUE1				0x5a
#define P9220_RXID_0_REG					0x5C
#define P9220_RXID_1_REG					0x5D
#define P9220_RXID_2_REG					0x5E
#define P9220_RXID_3_REG					0x5F
#define P9220_RXID_4_REG					0x60
#define P9220_RXID_5_REG					0x61
#define P9220_MOD_DEPTH_REG                 0x63
#define P9220_WPC_FOD_0A_REG				0x68
#define P9220_TX_PING_FREQ_REG				0xFC

#define P9220_NUM_FOD_REG					12

#define P9220_CHIP_ID_MAJOR_1_REG			0x0070
#define P9220_CHIP_ID_MAJOR_0_REG			0x0074
#define P9220_CHIP_ID_MINOR_REG				0x0078
#define P9220_LDO_EN_REG					0x301c

/* Chip Revision and Font Register, Chip_Rev (0x02) */
#define P9220_CHIP_GRADE_MASK				0x0f
#define P9220_CHIP_REVISION_MASK			0xf0

/* Status Registers, Status_L (0x34), Status_H (0x35) */
#define P9220_STAT_VOUT_SHIFT				7
#define P9220_STAT_STAT_VRECT_SHIFT			6
#define P9220_STAT_MODE_CHANGE_SHIFT		5
#define P9220_STAT_TX_DATA_RECEIVED_SHIFT	4
#define P9220_STAT_OVER_VOL_SHIFT			1
#define P9220_STAT_OVER_CURR_SHIFT			0
#define P9220_STAT_VOUT_MASK				(1 << P9220_STAT_VOUT_SHIFT)
#define P9220_STAT_STAT_VRECT_MASK			(1 << P9220_STAT_STAT_VRECT_SHIFT)
#define P9220_STAT_MODE_CHANGE_MASK			(1 << P9220_STAT_MODE_CHANGE_SHIFT)
#define P9220_STAT_TX_DATA_RECEIVED_MASK	(1 << P9220_STAT_TX_DATA_RECEIVED_SHIFT)
#define P9220_STAT_OVER_VOL_MASK			(1 << P9220_STAT_OVER_VOL_SHIFT)
#define P9220_STAT_OVER_CURR_MASK			(1 << P9220_STAT_OVER_CURR_SHIFT)

#define P9220_STAT_OVER_TEMP_SHIFT			7
#define P9220_STAT_TX_OVER_CURR_SHIFT		6
#define P9220_STAT_TX_OVER_TEMP_SHIFT		5
#define P9220_STAT_TX_FOD_SHIFT				4
#define P9220_STAT_TX_CONNECT_SHIFT			3
#define P9220_STAT_OVER_TEMP_MASK			(1 << P9220_STAT_OVER_TEMP_SHIFT)
#define P9220_STAT_TX_OVER_CURR_MASK		(1 << P9220_STAT_TX_OVER_CURR_SHIFT)
#define P9220_STAT_TX_OVER_TEMP_MASK		(1 << P9220_STAT_TX_OVER_TEMP_SHIFT)
#define P9220_STAT_TX_FOD_MASK				(1 << P9220_STAT_TX_FOD_SHIFT)
#define P9220_STAT_TX_CONNECT_MASK			(1 << P9220_STAT_TX_CONNECT_SHIFT)

/* Interrupt Registers, INT_L (0x36), INT_H (0x37) */
#define P9220_INT_STAT_VOUT					P9220_STAT_VOUT_MASK
#define P9220_INT_STAT_VRECT				P9220_STAT_STAT_VRECT_MASK
#define P9220_INT_MODE_CHANGE				P9220_STAT_MODE_CHANGE_MASK
#define P9220_INT_TX_DATA_RECEIVED			P9220_STAT_TX_DATA_RECEIVED_MASK
#define P9220_INT_OVER_VOLT					P9220_STAT_OVER_VOL_MASK
#define P9220_INT_OVER_CURR					P9220_STAT_OVER_CURR_MASK

#define P9220_INT_OVER_TEMP					P9220_STAT_OVER_TEMP_MASK
#define P9220_INT_TX_OVER_CURR				P9220_STAT_TX_OVER_CURR_MASK
#define P9220_INT_TX_OVER_TEMP				P9220_STAT_TX_OVER_TEMP_MASK
#define P9220_INT_TX_FOD					P9220_STAT_TX_FOD_MASK
#define P9220_INT_TX_CONNECT				P9220_STAT_TX_CONNECT_MASK

/* End of Power Transfer Register, EPT (0x3B) (RX only) */
#define P9220_EPT_UNKNOWN					0
#define P9220_EPT_END_OF_CHG				1
#define P9220_EPT_INT_FAULT					2
#define P9220_EPT_OVER_TEMP					3
#define P9220_EPT_OVER_VOL					4
#define P9220_EPT_OVER_CURR					5
#define P9220_EPT_BATT_FAIL					6
#define P9220_EPT_RECONFIG					7

/* System Operating Mode Register,Sys_Op_Mode (0x4D) */
#define P9220_SYS_MODE_INIT					0
#define P9220_SYS_MODE_WPC					1
#define P9220_SYS_MODE_PMA					2
#define P9220_SYS_MODE_MISSING_BACK			3
#define P9220_SYS_MODE_TX					4
#define P9220_SYS_MODE_WPC_RX				5
#define P9220_SYS_MODE_PMA_RX				6
#define P9220_SYS_MODE_MISSING				7


/* Command Register, COM(0x4E) */
#define P9220_CMD_CLEAR_INT_SHIFT			5
#define P9220_CMD_SEND_CHG_STS_SHIFT		4
#define P9220_CMD_SEND_EOP_SHIFT			3
#define P9220_CMD_SET_TX_MODE_SHIFT			2
#define P9220_CMD_TOGGLE_LDO_SHIFT			1
#define P9220_CMD_SEND_RX_DATA_SHIFT		0
#define P9220_CMD_CLEAR_INT_MASK			(1 << P9220_CMD_CLEAR_INT_SHIFT)
#define P9220_CMD_SEND_CHG_STS_MASK			(1 << P9220_CMD_SEND_CHG_STS_SHIFT)
#define P9220_CMD_SEND_EOP_MASK				(1 << P9220_CMD_SEND_EOP_SHIFT)
#define P9220_CMD_SET_TX_MODE_MASK			(1 << P9220_CMD_SET_TX_MODE_SHIFT)
#define P9220_CMD_TOGGLE_LDO_MASK			(1 << P9220_CMD_TOGGLE_LDO_SHIFT)
#define P9220_CMD_SEND_RX_DATA_MASK			(1 << P9220_CMD_SEND_RX_DATA_SHIFT)

/* Pro-prietary Packet Header Register, PPP_Header(0x50) */
#define P9220_HEADER_END_SIG_STRENGTH		0x01
#define P9220_HEADER_END_POWER_TRANSFER		0x02
#define P9220_HEADER_END_CTR_ERROR			0x03
#define P9220_HEADER_END_RECEIVED_POWER		0x04
#define P9220_HEADER_END_CHARGE_STATUS		0x05
#define P9220_HEADER_POWER_CTR_HOLD_OFF		0x06
#define P9220_HEADER_AFC_CONF				0x28
#define P9220_HEADER_CONFIGURATION			0x51
#define P9220_HEADER_IDENTIFICATION			0x71
#define P9220_HEADER_EXTENDED_IDENT			0x81

/* RX Data Command Register, RX Data_COM (0x51) */
#define P9220_RX_DATA_COM_UNKNOWN			0x00
#define P9220_RX_DATA_COM_TX_ID				0x01
#define P9220_RX_DATA_COM_CHG_STATUS		0x05
#define P9220_RX_DATA_COM_AFC_SET			0x06
#define P9220_RX_DATA_COM_AFC_DEBOUCE		0x07
#define P9220_RX_DATA_COM_SID_TAG			0x08
#define P9220_RX_DATA_COM_SID_TOKEN			0x09
#define P9220_RX_DATA_COM_TX_STANBY			0x0a
#define P9220_RX_DATA_COM_LED_CTRL			0x0b
#define P9220_RX_DATA_COM_REQ_AFC			0x0c
#define P9220_RX_DATA_COM_FAN_CTRL			0x0d

/* TX Data Command Register, TX Data_COM (0x58) */
#define P9220_TX_DATA_COM_UNKNOWN			0x00
#define P9220_TX_DATA_COM_TX_ID				0x01
#define P9220_TX_DATA_COM_AFC_TX			0x02
#define P9220_TX_DATA_COM_ACK				0x03
#define P9220_TX_DATA_COM_NAK				0x04

/* END POWER TRANSFER CODES IN WPC */
#define P9220_EPT_CODE_UNKOWN				0x00
#define P9220_EPT_CODE_CHARGE_COMPLETE		0x01
#define P9220_EPT_CODE_INTERNAL_FAULT		0x02
#define P9220_EPT_CODE_OVER_TEMPERATURE		0x03
#define P9220_EPT_CODE_OVER_VOLTAGE			0x04
#define P9220_EPT_CODE_OVER_CURRENT			0x05
#define P9220_EPT_CODE_BATTERY_FAILURE		0x06
#define P9220_EPT_CODE_RECONFIGURE			0x07
#define P9220_EPT_CODE_NO_RESPONSE			0x08

#define P9220_POWER_MODE_MASK				(0x1 << 0)
#define P9220_SEND_USER_PKT_DONE_MASK		(0x1 << 7)
#define P9220_SEND_USER_PKT_ERR_MASK		(0x3 << 5)
#define P9220_SEND_ALIGN_MASK				(0x1 << 3)
#define P9220_SEND_EPT_CC_MASK				(0x1 << 0)
#define P9220_SEND_EOC_MASK					(0x1 << 0)

#define P9220_PTK_ERR_NO_ERR				0x00
#define P9220_PTK_ERR_ERR					0x01
#define P9220_PTK_ERR_ILLEGAL_HD			0x02
#define P9220_PTK_ERR_NO_DEF				0x03

#define P9220_VOUT_5V_VAL					0x0f
#define P9220_VOUT_6V_VAL					0x19
#define P9220_VOUT_7V_VAL					0x23
#define P9220_VOUT_8V_VAL					0x2d
/* We set VOUT to 10V actually for HERO for RE/CE standard authentication */
#define P9220_VOUT_9V_VAL					0x37
#define P9220_VOUT_10V_VAL					0x41

#define P9220_FW_RESULT_DOWNLOADING			2
#define P9220_FW_RESULT_PASS				1
#define P9220_FW_RESULT_FAIL				0

enum {
	P9220_EVENT_IRQ = 0,
	P9220_IRQS_NR,
};

enum {
	P9220_PAD_MODE_NONE = 0,
	P9220_PAD_MODE_WPC,
	P9220_PAD_MODE_WPC_AFC,
	P9220_PAD_MODE_WPC_PACK,
	P9220_PAD_MODE_WPC_PACK_TA,
	P9220_PAD_MODE_WPC_STAND,
	P9220_PAD_MODE_WPC_STAND_HV,
	P9220_PAD_MODE_PMA,
	P9220_PAD_MODE_TX,
	P9220_PAD_MODE_WPC_VEHICLE,
};

/* PAD Vout */
enum {
	PAD_VOUT_5V = 0,
	PAD_VOUT_9V,
	PAD_VOUT_10V,
	PAD_VOUT_12V,
	PAD_VOUT_18V,
	PAD_VOUT_19V,
	PAD_VOUT_20V,
	PAD_VOUT_24V,
};

/* vout settings */
enum {
	P9220_VOUT_0V = 0,
	P9220_VOUT_5V,
	P9220_VOUT_6V,
	P9220_VOUT_9V,
	P9220_VOUT_CC_CV,
	P9220_VOUT_CV_CALL,
	P9220_VOUT_CC_CALL,
	P9220_VOUT_9V_STEP,
};

enum {
    P9220_ADC_VOUT = 0,
    P9220_ADC_VRECT,
    P9220_ADC_TX_ISENSE,
    P9220_ADC_RX_IOUT,
    P9220_ADC_DIE_TEMP,
    P9220_ADC_ALLIGN_X,
    P9220_ADC_ALLIGN_Y,
    P9220_ADC_OP_FRQ,
    P9220_ADC_TX_PING,
};

enum {
	P9220_END_SIG_STRENGTH = 0,
	P9220_END_POWER_TRANSFER,
	P9220_END_CTR_ERROR,
	P9220_END_RECEIVED_POWER,
	P9220_END_CHARGE_STATUS,
	P9220_POWER_CTR_HOLD_OFF,
	P9220_AFC_CONF_5V,
	P9220_AFC_CONF_9V,
	P9220_CONFIGURATION,
	P9220_IDENTIFICATION,
	P9220_EXTENDED_IDENT,
	P9220_LED_CONTROL_ON,
	P9220_LED_CONTROL_OFF,
	P9220_FAN_CONTROL_ON,
	P9220_FAN_CONTROL_OFF,
	P9220_REQUEST_AFC_TX,
	P9220_REQUEST_TX_ID,
};

enum p9220_irq_source {
	TOP_INT = 0,
};

enum p9220_chip_rev {
	P9220_A_GRADE_IC = 0,
	P9220_B_GRADE_IC,
	P9220_C_GRADE_IC,
	P9220_D_GRADE_IC,
};

enum p9220_irq {
	P9220_IRQ_STAT_VOUT = 0,
	P9220_IRQ_STAT_VRECT,
	P9220_IRQ_MODE_CHANGE,
	P9220_IRQ_TX_DATA_RECEIVED,
	P9220_IRQ_OVER_VOLT,
	P9220_IRQ_OVER_CURR,
	P9220_IRQ_OVER_TEMP,
	P9220_IRQ_TX_OVER_CURR,
	P9220_IRQ_TX_OVER_TEMP,
	P9220_IRQ_TX_FOD,
	P9220_IRQ_TX_CONNECT,
	P9220_IRQ_NR,
};

struct p9220_irq_data {
	int mask;
	enum p9220_irq_source group;
};

enum p9220_firmware_mode {
	P9220_RX_FIRMWARE = 0,
	P9220_TX_FIRMWARE,
};

enum p9220_read_mode {
	P9220_IC_GRADE = 0,
	P9220_IC_VERSION,
	P9220_IC_VENDOR,
};

enum p9220_headroom {
	P9220_HEADROOM_0 = 0,
	P9220_HEADROOM_1, /* 0.277V */
	P9220_HEADROOM_2, /* 0.497V */
	P9220_HEADROOM_3, /* 0.650V */
	P9220_HEADROOM_4, /* 0.030V */
	P9220_HEADROOM_5, /* 0.082V */
};

struct p9220_ppp_info {
	u8 header;
	u8 rx_data_com;
	u8 data_val[4];
	int data_size;
};

#define DECLARE_IRQ(idx, _group, _mask)		\
	[(idx)] = { .group = (_group), .mask = (_mask) }
static const struct p9220_irq_data p9220_irqs[] = {
	DECLARE_IRQ(P9220_IRQ_STAT_VOUT,	TOP_INT, 1 << 0),
};

static const u8 OTPBootloader[] = {
0x00, 0x04, 0x00, 0x20, 0x57, 0x01, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xFE, 0xE7, 0x00, 0x00, 0x80, 0x00, 0x00, 0xE0, 0x00, 0xBF, 0x40, 0x1E, 0xFC, 0xD2, 0x70, 0x47,
0x00, 0xB5, 0x6F, 0x4A, 0x6F, 0x4B, 0x01, 0x70, 0x01, 0x20, 0xFF, 0xF7, 0xF3, 0xFF, 0x52, 0x1E,
0x02, 0xD0, 0x18, 0x8B, 0x00, 0x06, 0xF7, 0xD4, 0x00, 0xBD, 0xF7, 0xB5, 0x05, 0x46, 0x6A, 0x48,
0x81, 0xB0, 0x00, 0x21, 0x94, 0x46, 0x81, 0x81, 0x66, 0x48, 0x31, 0x21, 0x01, 0x80, 0x04, 0x21,
0x81, 0x80, 0x06, 0x21, 0x01, 0x82, 0x28, 0x20, 0xFF, 0xF7, 0xDC, 0xFF, 0x00, 0x24, 0x15, 0xE0,
0x02, 0x99, 0x28, 0x5D, 0x09, 0x5D, 0x02, 0x46, 0x8A, 0x43, 0x01, 0xD0, 0x10, 0x20, 0x50, 0xE0,
0x81, 0x43, 0x0A, 0xD0, 0x5D, 0x4E, 0xB0, 0x89, 0x08, 0x27, 0x38, 0x43, 0xB0, 0x81, 0x28, 0x19,
0xFF, 0xF7, 0xCE, 0xFF, 0xB0, 0x89, 0xB8, 0x43, 0xB0, 0x81, 0x64, 0x1C, 0x64, 0x45, 0xE7, 0xD3,
0x54, 0x48, 0x36, 0x21, 0x01, 0x82, 0x00, 0x24, 0x38, 0xE0, 0x02, 0x98, 0x00, 0x27, 0x06, 0x5D,
0x52, 0x48, 0x82, 0x89, 0x08, 0x21, 0x0A, 0x43, 0x82, 0x81, 0x28, 0x19, 0x00, 0x90, 0x4D, 0x4A,
0x08, 0x20, 0x90, 0x80, 0x02, 0x20, 0xFF, 0xF7, 0xAD, 0xFF, 0x28, 0x5D, 0x33, 0x46, 0x83, 0x43,
0x15, 0xD0, 0x48, 0x49, 0x04, 0x20, 0x88, 0x80, 0x02, 0x20, 0xFF, 0xF7, 0xA3, 0xFF, 0x19, 0x46,
0x00, 0x98, 0xFF, 0xF7, 0xA5, 0xFF, 0x43, 0x49, 0x0F, 0x20, 0x88, 0x80, 0x02, 0x20, 0xFF, 0xF7,
0x99, 0xFF, 0x28, 0x5D, 0xB0, 0x42, 0x02, 0xD0, 0x7F, 0x1C, 0x0A, 0x2F, 0xDF, 0xD3, 0x3F, 0x48,
0x82, 0x89, 0x08, 0x21, 0x8A, 0x43, 0x82, 0x81, 0x0A, 0x2F, 0x06, 0xD3, 0x3C, 0x48, 0x29, 0x19,
0x41, 0x80, 0x29, 0x5D, 0xC1, 0x80, 0x04, 0x20, 0x03, 0xE0, 0x64, 0x1C, 0x64, 0x45, 0xC4, 0xD3,
0x02, 0x20, 0x34, 0x49, 0x11, 0x22, 0x0A, 0x80, 0x04, 0x22, 0x8A, 0x80, 0x32, 0x49, 0xFF, 0x22,
0x8A, 0x81, 0x04, 0xB0, 0xF0, 0xBD, 0x34, 0x49, 0x32, 0x48, 0x08, 0x60, 0x2F, 0x4D, 0x00, 0x22,
0xAA, 0x81, 0x2E, 0x4E, 0x20, 0x3E, 0xB2, 0x83, 0x2A, 0x80, 0x2B, 0x48, 0x5A, 0x21, 0x40, 0x38,
0x01, 0x80, 0x81, 0x15, 0x81, 0x80, 0x0B, 0x21, 0x01, 0x81, 0x2C, 0x49, 0x81, 0x81, 0x14, 0x20,
0xFF, 0xF7, 0x60, 0xFF, 0x2A, 0x4B, 0x01, 0x20, 0x18, 0x80, 0x02, 0x20, 0xFF, 0xF7, 0x5A, 0xFF,
0x8D, 0x20, 0x18, 0x80, 0x9A, 0x80, 0xFF, 0x20, 0x98, 0x82, 0x03, 0x20, 0x00, 0x02, 0x18, 0x82,
0xFC, 0x20, 0x98, 0x83, 0x22, 0x49, 0x95, 0x20, 0x20, 0x31, 0x08, 0x80, 0x1C, 0x4C, 0x0C, 0x20,
0x22, 0x80, 0xA8, 0x81, 0x20, 0x20, 0xB0, 0x83, 0x28, 0x80, 0xAA, 0x81, 0x04, 0x26, 0xA8, 0x89,
0x30, 0x43, 0xA8, 0x81, 0x20, 0x88, 0x01, 0x28, 0x1B, 0xD1, 0x61, 0x88, 0x80, 0x03, 0xA2, 0x88,
0x08, 0x18, 0x51, 0x18, 0x8B, 0xB2, 0x00, 0x21, 0x04, 0xE0, 0x0F, 0x19, 0x3F, 0x7A, 0xFB, 0x18,
0x9B, 0xB2, 0x49, 0x1C, 0x8A, 0x42, 0xF8, 0xD8, 0xE1, 0x88, 0x27, 0x46, 0x99, 0x42, 0x01, 0xD0,
0x08, 0x20, 0x0B, 0xE0, 0x00, 0x2A, 0x08, 0xD0, 0x09, 0x49, 0x08, 0x31, 0xFF, 0xF7, 0x35, 0xFF,
0x38, 0x80, 0xA8, 0x89, 0xB0, 0x43, 0xA8, 0x81, 0xD9, 0xE7, 0x02, 0x20, 0x20, 0x80, 0xD6, 0xE7,
0x10, 0x27, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x40, 0x40, 0x30, 0x00, 0x40, 0x20, 0x6C, 0x00, 0x40,
0x00, 0x04, 0x00, 0x20, 0xFF, 0x0F, 0x00, 0x00, 0x80, 0xE1, 0x00, 0xE0, 0x04, 0x1D, 0x00, 0x00,
0x00, 0x64, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

#define P9220_OTP_FIRM_VERSION						0x4012
#define P9220_SRAM_FIRM_VERSION						0x04 /* 2015-6-9 */

#define FREQ_OFFSET	384000 /* 64*6000 */

struct p9220_charger_platform_data {
	int wpc_det;
	int irq_wpc_det;
	int wpc_int;
	int irq_wpc_int;
	int cs100_status;
	int vout_status;
	int wireless_cc_cv;
	int siop_level;
	int cable_type;
	bool default_voreg;
	int is_charging;
	u32 *fod_data_cv;
	u32 *fod_data;
	int fod_data_check;
	bool ic_on_mode;
	int hw_rev_changed; /* this is only for noble/zero2 */
	int otp_firmware_result;
	int tx_firmware_result;
	int wc_ic_grade;
	int wc_ic_rev;
	int otp_firmware_ver;
	int tx_firmware_ver;
	int vout;
	int vrect;
	char *wireless_charger_name;
	char *wired_charger_name;
	int tx_status;
	int wpc_cc_cv_vout;
	int wpc_cv_call_vout;
	int wpc_cc_call_vout;
	int opfq_cnt;
	u8 tx_data_cmd;
	u8 tx_data_val;
	u32 hv_vout_wa; /* this is only for Hero/Poseidon */
};

#define p9220_charger_platform_data_t \
	struct p9220_charger_platform_data

struct p9220_charger_data {
	struct i2c_client				*client;
	struct device					*dev;
	p9220_charger_platform_data_t 	*pdata;
	struct mutex io_lock;
	const struct firmware *firm_data_bin;

	int wc_w_state;

	struct power_supply psy_chg;
	struct wake_lock wpc_wake_lock;
	struct wake_lock wpc_update_lock;
	struct wake_lock wpc_opfq_lock;
	struct workqueue_struct *wqueue;
	struct work_struct	wcin_work;
	struct delayed_work	wpc_det_work;
	struct delayed_work	wpc_opfq_work;
	struct delayed_work	wpc_isr_work;
	struct delayed_work	wpc_tx_id_work;

	u16 addr;
	int size;
	int is_afc;
	int pad_vout;
};

#endif /* __p9220_CHARGER_H */