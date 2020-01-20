/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : tuner.c
 *
 *	Description : tuner control driver source file
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *	History :
 *	----------------------------------------------------------------------
 */

#include <linux/kernel.h>
#include "fci_types.h"
#include "fci_tun.h"
#include "fci_hpi.h"
#include "fci_hal.h"
#include "fc8080_bb.h"
#include "fc8080_tun.h"
#include "fc8080_regs.h"

#ifndef CONFIG_TDMB_XTAL_FREQ
#if (FC8080_FREQ_XTAL == 16000)
#define CLOCK_RATIO 34359738 /* 33554432 * ((float) 1.024) */
#elif (FC8080_FREQ_XTAL == 16384)
#define CLOCK_RATIO 33554432 /* 33554432 * ((float) 1.) */
#elif (FC8080_FREQ_XTAL == 19200)
#define CLOCK_RATIO 28631996 /* 33554432 * ((float) 0.8533) */
#elif (FC8080_FREQ_XTAL == 24000)
#define CLOCK_RATIO 22347251 /* 33554432 * ((float) 0.666) */
#elif (FC8080_FREQ_XTAL == 24576)
#define CLOCK_RATIO 33554432 /* 33554432 * ((float) 1.) */
#elif (FC8080_FREQ_XTAL == 26000)
#define CLOCK_RATIO 31715649 /* 33554432 * ((float) 0.9452) */
#elif (FC8080_FREQ_XTAL == 27000)
#define CLOCK_RATIO 30541244 /* 33554432 * ((float) 0.9102) */
#elif (FC8080_FREQ_XTAL == 27120)
#define CLOCK_RATIO 30403670 /* 33554432 * ((float) 0.9061) */
#elif (FC8080_FREQ_XTAL == 32000)
#define CLOCK_RATIO 22347251 /* 33554432 * ((float) 0.666) */
#elif (FC8080_FREQ_XTAL == 38400)
#define CLOCK_RATIO 28631996 /* 33554432 * ((float) 0.8533) */
#endif
#endif

struct tuner_i2c_driver {
	s32 (*init)(HANDLE handle, s32 speed, s32 slaveaddr);
	s32 (*read)(HANDLE handle, u8 chip, u8 addr, u8 addr_len, u8 *data,
			u8 len);
	s32 (*write)(HANDLE handle, u8 chip, u8 addr, u8 addr_len, u8 *data,
			u8 len);
	s32 (*deinit)(HANDLE handle);
};

struct tuner_driver {
	s32 (*init)(HANDLE handle, enum band_type band);
	s32 (*set_freq)(HANDLE handle, enum band_type band, u32 freq);
	s32 (*get_rssi)(HANDLE handle, s32 *rssi);
	s32 (*deinit)(HANDLE handle);
};

static struct tuner_i2c_driver fcihpi = {
	&fci_hpi_init,
	&fci_hpi_read,
	&fci_hpi_write,
	&fci_hpi_deinit
};

static struct tuner_driver fc8080_tuner = {
	&fc8080_tuner_init,
	&fc8080_set_freq,
	&fc8080_get_rssi,
	&fc8080_tuner_deinit
};

static struct tuner_i2c_driver *tuner_ctrl = &fcihpi;
static struct tuner_driver *tuner = &fc8080_tuner;
static enum band_type tuner_band = BAND3_TYPE;
u8 tuner_addr;

s32 tuner_ctrl_select(HANDLE handle, enum i2c_type type)
{
	switch (type) {
	case FCI_HPI_TYPE:
		tuner_ctrl = &fcihpi;
		break;
	default:
		return BBM_E_TN_CTRL_SELECT;
	}

	if (tuner_ctrl->init(handle, 400, 0))
		return BBM_E_TN_CTRL_INIT;

	return BBM_OK;
}

s32 tuner_i2c_read(HANDLE handle, u8 addr, u8 addr_len, u8 *data, u8 len)
{
	if (tuner_ctrl->read(handle, tuner_addr, addr, addr_len, data, len))
		return BBM_E_TN_READ;

	return BBM_OK;
}

s32 tuner_i2c_write(HANDLE handle, u8 addr, u8 addr_len, u8 *data, u8 len)
{
	if (tuner_ctrl->write(handle, tuner_addr, addr, addr_len, data, len))
		return BBM_E_TN_WRITE;

	return BBM_OK;
}

s32 tuner_set_freq(HANDLE handle, u32 freq)
{
	s32 res = BBM_NOK;
	u8 tmp;
#ifdef FC8080_I2C
	u16 buf_en = 0;
#endif
	if (tuner == NULL) {
		pr_info("TDMB : TUNER NULL ERROR\n");
		return BBM_NOK;
	}

#ifdef FC8080_I2C
	bbm_word_read(handle, BBM_BUF_ENABLE, &buf_en);
	bbm_word_write(handle, BBM_BUF_ENABLE, buf_en & 0x0ff);
#endif

	res = tuner->set_freq(handle, tuner_band, freq);
	if (res != BBM_OK) {
		pr_info("TDMB : TUNER res ERROR\n");
		return BBM_NOK;
	}

#ifdef CONFIG_TDMB_XTAL_FREQ
	if (main_xtal_freq == 16000)
		tmp = (u8) (34359738 / freq); /* 33554432 * ((float) 1.024) */
	else if (main_xtal_freq == 16384)
		tmp = (u8) (33554432 / freq); /* 33554432 * ((float) 1.) */
	else if (main_xtal_freq == 19200)
		tmp = (u8) (28631996 / freq); /* 33554432 * ((float) 0.8533) */
	else if (main_xtal_freq == 24000)
		tmp = (u8) (22347251 / freq); /* 33554432 * ((float) 0.666) */
	else if (main_xtal_freq == 24576)
		tmp = (u8) (33554432 / freq); /* 33554432 * ((float) 1.) */
	else if (main_xtal_freq == 26000)
		tmp = (u8) (31715649 / freq); /* 33554432 * ((float) 0.9452) */
	else if (main_xtal_freq == 27000)
		tmp = (u8) (30541244 / freq); /* 33554432 * ((float) 0.9102) */
	else if (main_xtal_freq == 27120)
		tmp = (u8) (30403670 / freq); /* 33554432 * ((float) 0.9061) */
	else if (main_xtal_freq == 32000)
		tmp = (u8) (22347251 / freq); /* 33554432 * ((float) 0.666) */
	else if (main_xtal_freq == 38400)
		tmp = (u8) (28631996 / freq); /* 33554432 * ((float) 0.8533) */
	else
		tmp = (u8) (33554432 / freq);
#else
	tmp = (u8) (CLOCK_RATIO / freq);
#endif
	bbm_write(handle, BBM_INV_CARRIER_FREQ, tmp);

	fc8080_reset(handle);

#ifdef FC8080_I2C
	bbm_word_write(handle, BBM_BUF_ENABLE, buf_en);
#endif

	return res;
}

s32 tuner_select(HANDLE handle, enum product_type product, enum band_type band)
{
	switch (product) {
	case FC8080_TUNER:
		tuner = &fc8080_tuner;
		tuner_band = (enum band_type) band;
		tuner_addr = 0x00;

		tuner_ctrl_select(handle, FCI_HPI_TYPE);

		bbm_write(handle, BBM_QDD_COMMAND, 0xc4);
		break;
	default:
		return BBM_E_TN_SELECT;
	}

	if (tuner->init == NULL)
		return BBM_E_TN_SELECT;

	if (tuner->init(handle, tuner_band))
		return BBM_E_TN_INIT;

	return BBM_OK;
}

s32 tuner_get_rssi(HANDLE handle, s32 *rssi)
{
	if (tuner->get_rssi(handle, rssi))
		return BBM_E_TN_RSSI;

	return BBM_OK;
}

