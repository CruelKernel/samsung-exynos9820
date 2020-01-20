/* s6e3aa2_param.h
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * SeungBeom, Park <sb1.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3AA2_PARAM_H__
#define __S6E3AA2_PARAM_H__

static const unsigned char TEST_KEY_ON_0[] = {
	0xF0,
	0x5A, 0x5A,
};

static const unsigned char TEST_KEY_OFF_0[] = {
	0xF0,
	0xA5, 0xA5,
};

static const unsigned char TEST_KEY_ON_1[] = {
	0xF1,
	0x5A, 0x5A,
};

static const unsigned char TEST_KEY_OFF_1[] = {
	0xF1,
	0xA5, 0xA5,
};

static const unsigned char HIDDEN_KEY_ON[] = {
	0xFC,
	0x5A, 0x5A,
};

static const unsigned char HIDDEN_KEY_OFF[] = {
	0xFC,
	0xA5, 0xA5,
};


static const unsigned char LANE_2[] = {
	0xC4,
	0x02,
};

static const unsigned char LANE_4[] = {
	0xC4,
	0x04,
};

static const unsigned char SLEEP_OUT[] = {
	0x11,
};

static const unsigned char SLEEP_IN[] = {
	0x10,
};

static const unsigned char DISPLAY_ON[] = {
	0x29,
};

static const unsigned char DISPLAY_OFF[] = {
	0x28,
};

static const unsigned char TEST_KEY_ON_F0[] = {
	0xF0,
	0x5A, 0x5A
};

static const unsigned char TEST_KEY_OFF_F0[] = {
	0xF0,
	0xA5, 0xA5
};

static const unsigned char TEST_KEY_ON_F1[] = {
	0xF1,
	0x5A, 0x5A
};

static const unsigned char TEST_KEY_OFF_F1[] = {
	0xF1,
	0xA5, 0xA5
};

static const unsigned char TEST_KEY_ON_FC[] = {
	0xFC,
	0x5A, 0x5A
};

static const unsigned char TEST_KEY_OFF_FC[] = {
	0xFC,
	0xA5, 0xA5
};

static const unsigned char TE_ON[] = {
	0x35,
	0x00, 0x00,
};

static const unsigned char PCD_SET_DET_LOW[] = {
	0xCC,
	0x5C
};

static const unsigned char ERR_FG_SETTING[] = {
	0xED,
	0x44
};

static const unsigned char GAMMA_CONDITION_SET[] = {
	0xCA,
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x00, 0x00
};

static const unsigned char AID_SETTING[] = {
	0xB1,
	0xFF, 0x20, 0x1A, 0x33, 0x5E, 0x8C, 0xB3, 0xD9, 0xFF, 0x60,
	0x0D
};

static const unsigned char ELVSS_SET[] = {
	0xB5,
	0xA0,
	0x1C,	/* B5h 2nd Para: MPS_CON */
	0x44,	/* B5h 3rd Para: ELVSS_Dim_offset */
};

static const unsigned char GAMMA_UPDATE[] = {
	0xF7,
	0x03
};

static const unsigned char HBM_OFF[] = {
	0x53,
	0x00
};

static const unsigned char OPR_ACL_OFF[] = {
	0xB4,
	0x40	/* 16 Frame Avg. at ACL Off */
};

static const unsigned char ACL_OFF[] = {
	0x55,
	0x00
};

static const unsigned char TSET_SETTING_1[] = {
	0xB0,
	0x1D
};

static const unsigned char TSET_SETTING_2[] = {
	0xB5,
	0x19
};

static const unsigned char VIDEO_MODE_F2[] = {
	0xF2,
	0x01, 0x0E, 0x39, 0x80, 0x5A, 0xA0, 0x0A, 0x0E, 0x00, 0x91, 0x20
};

static const unsigned char MIPI_ILVL_E8[] = {
	0xE8,
	0xA4, 0x08, 0x00	/* ILVL = 8 */
};

#endif /* __S6E3AA2_PARAM_H__ */

