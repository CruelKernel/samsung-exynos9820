/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fc8080_tun.c
 *
 *	Description : fc8080 tuner driver source file
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
 *	20130409 v0p4
 *	20130412 v0p5
 *	20130415 v0p6
 *	20130509 v1p0
 *	20130520 v1p1
 *	20130524 v1p2
 *	20130524 v1p3
 *	20131125 v1p4
 */

#include "fci_types.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fci_hal.h"
#include "fc8080_regs.h"

static s32 fc8080_write(HANDLE handle, u8 addr, u8 data)
{
	s32 res;
	u8 tmp;

	tmp = data;
	res = tuner_i2c_write(handle, addr, 1, &tmp, 1);

	return res;
}

static s32 fc8080_read(HANDLE handle, u8 addr, u8 *data)
{
	s32 res;

	res = tuner_i2c_read(handle, addr, 1, data, 1);

	return res;
}

/*
 *static s32 fc8080_bb_write(HANDLE handle, u16 addr, u8 data)
 *{
 *	s32 res;
 *
 *	res = bbm_write(handle, addr, data);
 *
 *	return res;
 *}
 */

static s32 fc8080_bb_read(HANDLE handle, u16 addr, u8 *data)
{
	s32 res;

	res = bbm_read(handle, addr, data);

	return res;
}

static s32 fc8080_set_filter(HANDLE handle)
{
	u8 cal = 0;
#ifdef CONFIG_TDMB_XTAL_FREQ
	u32 tcxo = main_xtal_freq * 1000;
#else
	u32 tcxo = FC8080_FREQ_XTAL * 1000;
#endif
	u32 csfbw = 780000;

	cal = (unsigned int) (tcxo * 78 * 2 / csfbw) / 100;

	fc8080_write(handle, 0x3c, 0x01);
	fc8080_write(handle, 0x3d, cal);
	fc8080_write(handle, 0x3c, 0x00);

	fc8080_write(handle, 0x32, 0x07);

	return BBM_OK;
}

s32 fc8080_tuner_init(HANDLE handle, u32 band)
{
	s32 i;
	s32 n_rfagc_pd1_avg, n_rfagc_pd2_avg;

	u8 rfagc_pd2[6], rfagc_pd2_avg, rfagc_pd2_max, rfagc_pd2_min;
	u8 rfagc_pd1[6], rfagc_pd1_avg, rfagc_pd1_max, rfagc_pd1_min;

	fc8080_write(handle, 0x00, 0x00);
	fc8080_write(handle, 0x02, 0x11);

	fc8080_set_filter(handle);

	fc8080_write(handle, 0xb3, 0x78);

	fc8080_write(handle, 0x9c, 0x00);
	fc8080_write(handle, 0x9d, 0x01);
	fc8080_write(handle, 0x9e, 0x02);
	fc8080_write(handle, 0x9f, 0x0a);
	fc8080_write(handle, 0xa0, 0x2a);
	fc8080_write(handle, 0xa1, 0x32);
	fc8080_write(handle, 0xa2, 0x52);
	fc8080_write(handle, 0xa3, 0x56);
	fc8080_write(handle, 0xa4, 0x76);
	fc8080_write(handle, 0xa5, 0x7e);
	fc8080_write(handle, 0xa6, 0x7f);
	fc8080_write(handle, 0xa7, 0x9f);

	fc8080_write(handle, 0xa8, 0xc0);
	fc8080_write(handle, 0xd0, 0x00);

	rfagc_pd1[0] = 0;
	rfagc_pd1[1] = 0;
	rfagc_pd1[2] = 0;
	rfagc_pd1[3] = 0;
	rfagc_pd1[4] = 0;
	rfagc_pd1[5] = 0;
	rfagc_pd1_max = 0;
	rfagc_pd1_min = 255;

	for (i = 0; i < 6; i++) {
		fc8080_read(handle, 0xd8, &rfagc_pd1[i]);

		if (rfagc_pd1[i] >= rfagc_pd1_max)
			rfagc_pd1_max = rfagc_pd1[i];

		if (rfagc_pd1[i] <= rfagc_pd1_min)
			rfagc_pd1_min = rfagc_pd1[i];
	}

	n_rfagc_pd1_avg = (rfagc_pd1[0] + rfagc_pd1[1] + rfagc_pd1[2] +
				rfagc_pd1[3] + rfagc_pd1[4] + rfagc_pd1[5] -
				rfagc_pd1_max - rfagc_pd1_min) / 4;

	rfagc_pd1_avg = (u8) n_rfagc_pd1_avg;

	fc8080_write(handle, 0x7f, rfagc_pd1_avg);

	rfagc_pd2[0] = 0;
	rfagc_pd2[1] = 0;
	rfagc_pd2[2] = 0;
	rfagc_pd2[3] = 0;
	rfagc_pd2[4] = 0;
	rfagc_pd2[5] = 0;

	rfagc_pd2_max = 0;
	rfagc_pd2_min = 255;

	for (i = 0; i < 6; i++) {
		fc8080_read(handle, 0xd6, &rfagc_pd2[i]);

		if (rfagc_pd2[i] >= rfagc_pd2_max)
			rfagc_pd2_max = rfagc_pd2[i];

		if (rfagc_pd2[i] <= rfagc_pd2_min)
			rfagc_pd2_min = rfagc_pd2[i];
	}

	n_rfagc_pd2_avg = (rfagc_pd2[0] + rfagc_pd2[1] + rfagc_pd2[2] +
				rfagc_pd2[3] + rfagc_pd2[4] + rfagc_pd2[5] -
				rfagc_pd2_max - rfagc_pd2_min) / 4;

	rfagc_pd2_avg = (u8) n_rfagc_pd2_avg;

	fc8080_write(handle, 0x7e, rfagc_pd2_avg);

	fc8080_write(handle, 0x79, 0x26);
	fc8080_write(handle, 0x7a, 0x21);
	fc8080_write(handle, 0x7b, 0xff);
	fc8080_write(handle, 0x7c, 0x1b);
	fc8080_write(handle, 0x7d, 0x18);
	fc8080_write(handle, 0x84, 0x00);
	fc8080_write(handle, 0x85, 0x0d);
	fc8080_write(handle, 0x86, 0x00);
	fc8080_write(handle, 0x87, 0x10);
	fc8080_write(handle, 0x88, 0x00);
	fc8080_write(handle, 0x89, 0x0e);
	fc8080_write(handle, 0x8a, 0x00);
	fc8080_write(handle, 0x8b, 0x0e);
	fc8080_write(handle, 0x8c, 0x03);
	fc8080_write(handle, 0x8d, 0x10);
	fc8080_write(handle, 0x8e, 0x00);
	fc8080_write(handle, 0x8f, 0x10);
	fc8080_write(handle, 0x90, 0x04);
	fc8080_write(handle, 0x91, 0x10);
	fc8080_write(handle, 0x92, 0x00);
	fc8080_write(handle, 0x93, 0x00);
	fc8080_write(handle, 0x94, 0x00);
	fc8080_write(handle, 0x95, 0x0e);
	fc8080_write(handle, 0x96, 0x00);
	fc8080_write(handle, 0x97, 0x0e);
	fc8080_write(handle, 0x98, 0x00);
	fc8080_write(handle, 0x99, 0x0e);
	fc8080_write(handle, 0x9a, 0x00);
	fc8080_write(handle, 0x9b, 0x0e);
	fc8080_write(handle, 0x80, 0x3d);
	fc8080_write(handle, 0x81, 0x20);
	fc8080_write(handle, 0x82, 0x25);
	fc8080_write(handle, 0x83, 0x0a);

	fc8080_write(handle, 0xd0, 0x00);

#ifdef CONFIG_TDMB_XTAL_FREQ
	if (main_xtal_freq == 16384) {
		fc8080_write(handle, 0xd2, 0x18);
		fc8080_write(handle, 0xd4, 0x2a);
	} else if (main_xtal_freq == 19200) {
		fc8080_write(handle, 0xd2, 0x1d);
		fc8080_write(handle, 0xd4, 0x33);
	} else if (main_xtal_freq == 24000) {
		fc8080_write(handle, 0xd2, 0x28);
		fc8080_write(handle, 0xd4, 0x44);
	} else if (main_xtal_freq == 24576) {
		fc8080_write(handle, 0xd2, 0x28);
		fc8080_write(handle, 0xd4, 0x44);
	} else if (main_xtal_freq == 27120) {
		fc8080_write(handle, 0xd2, 0x2d);
		fc8080_write(handle, 0xd4, 0x4c);
	} else if (main_xtal_freq == 38400) {
		fc8080_write(handle, 0xd2, 0x44);
		fc8080_write(handle, 0xd4, 0x6f);
	}
#else
#if (FC8080_FREQ_XTAL == 16384)
	fc8080_write(handle, 0xd2, 0x18);
	fc8080_write(handle, 0xd4, 0x2a);
#elif (FC8080_FREQ_XTAL == 19200)
	fc8080_write(handle, 0xd2, 0x1d);
	fc8080_write(handle, 0xd4, 0x33);
#elif (FC8080_FREQ_XTAL == 24000)
	fc8080_write(handle, 0xd2, 0x28);
	fc8080_write(handle, 0xd4, 0x44);
#elif (FC8080_FREQ_XTAL == 24576)
	fc8080_write(handle, 0xd2, 0x28);
	fc8080_write(handle, 0xd4, 0x44);
#elif (FC8080_FREQ_XTAL == 27120)
	fc8080_write(handle, 0xd2, 0x2d);
	fc8080_write(handle, 0xd4, 0x4c);
#elif (FC8080_FREQ_XTAL == 38400)
	fc8080_write(handle, 0xd2, 0x44);
	fc8080_write(handle, 0xd4, 0x6f);
#endif
#endif
	fc8080_write(handle, 0xae, 0x36);
	fc8080_write(handle, 0xad, 0x8b);
	fc8080_write(handle, 0x14, 0x63);
	fc8080_write(handle, 0x15, 0x02);
	fc8080_write(handle, 0x18, 0x43);
	fc8080_write(handle, 0x19, 0x21);
	fc8080_write(handle, 0x27, 0x82);
	fc8080_write(handle, 0x28, 0x33);
	fc8080_write(handle, 0x33, 0x43);
	fc8080_write(handle, 0x34, 0x41);
	fc8080_write(handle, 0x53, 0x41);
	fc8080_write(handle, 0x54, 0x01);
	fc8080_write(handle, 0x66, 0xf6);

	fc8080_write(handle, 0xaf, 0x14);
	fc8080_write(handle, 0xb6, 0x00);
	fc8080_write(handle, 0xb8, 0xb0);
	fc8080_write(handle, 0xb9, 0xc4);

	return BBM_OK;
}

s32 fc8080_set_freq(HANDLE handle, u32 band, u32 freq)
{
	u32 f_diff, f_diff_shifted, n_val, k_val;
	u32 f_vco, f_comp;
	u8 div_ratio, lo_mode, r_val, data_0x55;
	u8 pre_shift_bits = 4;

	div_ratio = 16;
	lo_mode = 0;
	f_vco = freq * div_ratio;

	if (f_vco > 2800000) {
		div_ratio = 12;
		lo_mode = 1;
		f_vco = freq * div_ratio;
	}
#ifdef CONFIG_TDMB_XTAL_FREQ
	if (f_vco < main_xtal_freq * 35)
		r_val = 2;
	else
		r_val = 1;

	f_comp = main_xtal_freq / r_val;
#else
	if (f_vco < FC8080_FREQ_XTAL * 35)
		r_val = 2;
	else
		r_val = 1;

	f_comp = FC8080_FREQ_XTAL / r_val;
#endif

	n_val = f_vco / f_comp;
	f_diff = f_vco - f_comp * n_val;

	f_diff_shifted = f_diff << (20 - pre_shift_bits);

	k_val = f_diff_shifted / (f_comp >> pre_shift_bits);
	k_val = k_val | 1;

	data_0x55 = ((r_val == 1) ? 0x00 : 0x10) + (u8) (k_val >> 16);

	fc8080_write(handle, 0x51, 7 + 8 * lo_mode);
	fc8080_write(handle, 0x55, data_0x55);
	fc8080_write(handle, 0x56, (u8) ((k_val >> 8) & 0xff));
	fc8080_write(handle, 0x57, (u8) (((k_val) & 0xff)));
	fc8080_write(handle, 0x58, (n_val) & 0xff);

	if (freq < 207000)
		fc8080_write(handle, 0x6a, 0x0b);
	else
		fc8080_write(handle, 0x6a, 0x03);

	fc8080_write(handle, 0x04, 0x04);
	fc8080_write(handle, 0x50, 0xf3);
	fc8080_write(handle, 0x50, 0xff);
	fc8080_write(handle, 0x04, 0x00);

	return BBM_OK;
}

s32 fc8080_get_rssi(HANDLE handle, s32 *rssi)
{
	s32 res = BBM_OK;
	s32 pga = 0;

	u8 lna, rfvga, filter, preamp_pga, a1, ext_lna, a2, a3, k, crntmode1,
		crntmode0;
	u8 temp = 0;

	res = fc8080_read(handle, 0xab, &temp);
	lna = temp & 0x0f;
	ext_lna = temp >> 7;

	res = fc8080_read(handle, 0xac, &rfvga);
	rfvga = rfvga & 0x1f;

	res = fc8080_bb_read(handle, 0x014d, &filter);
	filter = filter & 0x1f;

	res = fc8080_bb_read(handle, 0x014e, &preamp_pga);

	if (preamp_pga > 127)
		pga = -1 * ((256 - preamp_pga) + 1);
	else if (preamp_pga <= 127)
		pga = preamp_pga;

	res = fc8080_read(handle, 0xaf, &a1);

	res = fc8080_read(handle, 0xae, &temp);
	a2 = temp & 0x0f;
	a3 = temp >> 4;

	res = fc8080_read(handle, 0xad, &k);

	if (filter < 7) {
		crntmode1 = 0;
		crntmode0 = 0;
	} else if (filter == 7) {
		crntmode1 = 0;
		crntmode0 = 1;
	} else {
		crntmode1 = 1;
		crntmode0 = 0;
	}

	*rssi = (5 * lna + (rfvga / 2) + (6 * filter) - (pga / 4) +
		(a1 * ext_lna) + (a2 * crntmode1) + (a3 * crntmode0) - k);

	return res;
}

s32 fc8080_tuner_deinit(HANDLE handle)
{
	return BBM_OK;
}

