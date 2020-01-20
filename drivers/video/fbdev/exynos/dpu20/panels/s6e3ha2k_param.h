/* linux/drivers/video/decon_display/s6e3fa0_param.h
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * Jiun Yu <jiun.yu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HA0K_PARAM_H__
#define __S6E3HA0K_PARAM_H__

#define S6E3HA2_ID_REG			0x04
#define S6E3HA2_RD_LEN			3
#define S6E3HA2_RDDPM_ADDR		0x0A
#define S6E3HA2_RDDSM_ADDR		0x0E

/* MIPI commands list */
static const unsigned char SEQ_TEST_KEY_ON_F0[] = {
	0xF0,
	0x5A, 0x5A
};

static const unsigned char SEQ_TEST_KEY_ON_FC[] = {
	0xFC,
	0x5A, 0x5A
};

static const unsigned char SEQ_TEST_KEY_OFF_FC[] = {
	0xFC,
	0xA5, 0xA5
};

static const unsigned char SEQ_SLEEP_OUT[] = {
	0x11,
};

static const unsigned char SEQ_REG_F2[] = {
	0xF2,
	0x67, 0x41, 0xC3, 0x06, 0x0A
};

static const unsigned char SEQ_TE_START_SETTING[] = {
	0xB9,
	0x10, 0x09, 0xFF, 0x00, 0x09
};

static const unsigned char SEQ_REG_F9[] = {
	0xF9,
	0x29
};

static const unsigned char SEQ_TOUCH_HSYNC[] = {
	0xBD,
	0x30, 0x22, 0x02, 0x16, 0x02, 0x16
};

static const unsigned char SEQ_PENTILE_CONTROL[] = {
	0xC0,
	0x30, 0x00, 0xD8, 0xD8
};

static const unsigned char SEQ_COLUMN_ADDRESS[] = {
	0x2A,
	0x00, 0x00, 0x05, 0x9F
};

static const unsigned char SEQ_GAMMA_CONDITION_SET[] = {
	0xCA,
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x00, 0x00
};

static const unsigned char SEQ_AID_SET[] = {
	0xB2,
	0x03, 0x10
};

static const unsigned char SEQ_ELVSS_SET[] = {
	0xB6,
	0x9C, 0x0A
};

static const unsigned char SEQ_GAMMA_UPDATE[] = {
	0xF7,
	0x03
};

static const unsigned char SEQ_ACL_OFF[] = {
	0x55,
	0x00
};

static const unsigned char SEQ_ACL_OPR[] = {
	0xB5,
	0x40
};

static const unsigned char SEQ_HBM_OFF[] = {
	0xB4,
	0x04
};

static const unsigned char SEQ_TSET_GLOBAL[] = {
	0xB0,
	0x07
};

static const unsigned char SEQ_TSET[] = {
	0xB8,
	0x19
};

static const unsigned char SEQ_TEST_KEY_OFF_F0[] = {
	0xF0,
	0xA5, 0xA5
};

static const unsigned char SEQ_TEST_KEY_OFF_F1[] = {
	0xF1, 0xA5, 0xA5
};

static const unsigned char SEQ_DISPLAY_ON[] = {
	0x29,
};

static const unsigned char SEQ_TE_ON[] = {
	0x35,
	0x00
};

static const unsigned char SEQ_ESD_FG[] = {
	0xED,
	0x01, 0x04
};

static const unsigned char SEQ_ALLPOFF[] = {
	0x22
};

static const unsigned char SEQ_ALLPON[] = {
	0x23
};

static const unsigned char SEQ_NOP[] = {
	0x00,
};

static const unsigned char SEQ_DISPLAY_OFF[] = {
	0x28
};

#endif /* __S6E3HA0K_PARAM_H__ */
