/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3fa7/s6e3fa7_aod.c
 *
 * Source file for AOD Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../panel_drv.h"
#include "s6e3fa7_aod.h"

#ifdef CONFIG_EXTEND_LIVE_CLOCK
void s6e3fa7_copy_self_mask_ctrl(struct maptbl *tbl, u8 *dst)
{
	pr_info("%s was called\n", __func__);
	pr_info("%x %x %x\n", dst[0], dst[1], dst[2]);
}

int s6e3fa7_init_self_mask_ctrl(struct maptbl *tbl)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;
	props->self_mask_checksum_len = SELFMASK_CHECKSUM_LEN;
	props->self_mask_checksum = kmalloc(sizeof(u8) * props->self_mask_checksum_len, GFP_KERNEL);
	if (!props->self_mask_checksum) {
		panel_err("PANEL:ERR:%s:failed to mem alloc\n", __func__);
		return -ENOMEM;
	}
	props->self_mask_checksum[0] = SELFMASK_CHECKSUM_VALID1;
	props->self_mask_checksum[1] = SELFMASK_CHECKSUM_VALID2;
	props->self_mask_checksum[2] = SELFMASK_CHECKSUM_VALID3;
	props->self_mask_checksum[3] = SELFMASK_CHECKSUM_VALID4;
	props->self_mask_checksum[4] = SELFMASK_CHECKSUM_VALID5;
	props->self_mask_checksum[5] = SELFMASK_CHECKSUM_VALID6;
	props->self_mask_checksum[6] = SELFMASK_CHECKSUM_VALID7;
	props->self_mask_checksum[7] = SELFMASK_CHECKSUM_VALID8;
	pr_info("%s was called\n", __func__);
	return 0;
}

void s6e3fa7_copy_digital_pos(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if (props->digital.en == 0) {
		panel_info("AOD:WARN:%s:digital clk was disabled\n", __func__);
		return;
	}

	dst[DIG_CLK_POS1_X1_REG] = (u8)(props->digital.pos1_x >> 8);
	dst[DIG_CLK_POS1_X2_REG] = (u8)(props->digital.pos1_x & 0xff);
	dst[DIG_CLK_POS1_Y1_REG] = (u8)(props->digital.pos1_y >> 8);
	dst[DIG_CLK_POS1_Y2_REG] = (u8)(props->digital.pos1_y & 0xff);

	dst[DIG_CLK_POS2_X1_REG] = (u8)(props->digital.pos2_x >> 8);
	dst[DIG_CLK_POS2_X2_REG] =  (u8)(props->digital.pos2_x & 0xff);
	dst[DIG_CLK_POS2_Y1_REG] = (u8)(props->digital.pos2_y >> 8);
	dst[DIG_CLK_POS2_Y2_REG] = (u8)(props->digital.pos2_y & 0xff);

	dst[DIG_CLK_POS3_X1_REG] = (u8)(props->digital.pos3_x >> 8);
	dst[DIG_CLK_POS3_X2_REG] =  (u8)(props->digital.pos3_x & 0xff);
	dst[DIG_CLK_POS3_Y1_REG] = (u8)(props->digital.pos3_y >> 8);
	dst[DIG_CLK_POS3_Y2_REG] = (u8)(props->digital.pos3_y & 0xff);

	dst[DIG_CLK_POS4_X1_REG] = (u8)(props->digital.pos4_x >> 8);
	dst[DIG_CLK_POS4_X2_REG] =  (u8)(props->digital.pos4_x & 0xff);
	dst[DIG_CLK_POS4_Y1_REG] = (u8)(props->digital.pos4_y >> 8);
	dst[DIG_CLK_POS4_Y2_REG] = (u8)(props->digital.pos4_y & 0xff);

	dst[DIG_CLK_WIDTH1] = (u8)(props->digital.img_width >> 8);
	dst[DIG_CLK_WIDTH2] = (u8)(props->digital.img_width & 0xff);
	dst[DIG_CLK_HEIGHT1] = (u8)(props->digital.img_height >> 8);
	dst[DIG_CLK_HEIGHT2] = (u8)(props->digital.img_height & 0xff);

}

void s6e3fa7_copy_digital_blink(struct maptbl *tbl, u8 *dst)
{
	/* beyond does not support blink - remove */
#if 0
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if (props->digital.b_en == 0) {
		panel_info("AOD:WARN:%s:digital blink was disabled\n", __func__);
		dst[DIG_BLK_EN_REG] = 0x00;
		return;
	}

	dst[DIG_BLK_EN_REG] = DIG_BLK_EN;
	dst[DIG_BLK1_RADIUS_REG] = props->digital.b_radius;
	dst[DIG_BLK2_RADIUS_REG] = props->digital.b_radius;

	dst[DIG_BLK1_COLOR1_REG] = (u8)(props->digital.b_color >> 16);
	dst[DIG_BLK1_COLOR2_REG] = (u8)(props->digital.b_color >> 8);
	dst[DIG_BLK1_COLOR3_REG] = (u8)(props->digital.b_color & 0xff);

	dst[DIG_BLK2_COLOR1_REG] = (u8)(props->digital.b_color >> 16);
	dst[DIG_BLK2_COLOR2_REG] = (u8)(props->digital.b_color >> 8);
	dst[DIG_BLK2_COLOR3_REG] = (u8)(props->digital.b_color & 0xff);

	dst[DIG_BLK1_POS_X1_REG] = (u8)(props->digital.b1_pos_x >> 8);
	dst[DIG_BLK1_POS_X2_REG] = (u8)(props->digital.b1_pos_x & 0xff);
	dst[DIG_BLK1_POS_Y1_REG] = (u8)(props->digital.b1_pos_y >> 8);
	dst[DIG_BLK1_POS_Y2_REG] = (u8)(props->digital.b1_pos_y & 0xff);

	dst[DIG_BLK2_POS_X1_REG] = (u8)(props->digital.b2_pos_x >> 8);
	dst[DIG_BLK2_POS_X2_REG] = (u8)(props->digital.b2_pos_x & 0xff);
	dst[DIG_BLK2_POS_Y1_REG] = (u8)(props->digital.b2_pos_y >> 8);
	dst[DIG_BLK2_POS_Y2_REG] = (u8)(props->digital.b2_pos_y & 0xff);
#endif

}


void s6e3fa7_copy_set_time_ctrl(struct maptbl *tbl, u8 *dst)
{
	char dig_en = 0;
	char en_reg = 0;
	char sc_inc_step = 0;
	char sc_update_rate = 0;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if (aod == NULL) {
		panel_err("AOD:ERR:%s:aod is null\n", __func__);
		return;
	}

	en_reg |= (SC_TIME_EN | SC_A_CLK_EN);

	dst[TIME_HH_REG] = props->cur_time.cur_h;
	dst[TIME_MM_REG] = props->cur_time.cur_m;
	dst[TIME_SS_REG] = props->cur_time.cur_s;
	dst[TIME_MSS_REG] = props->cur_time.cur_ms;

	if (props->analog.en)
		en_reg |= SC_DISP_ON;

	if (props->digital.en) {
		en_reg &= ~(SC_A_CLK_EN);
		en_reg |= (SC_D_CLK_EN | SC_DISP_ON);

		if (props->digital.en_hh)
			dig_en |= (props->digital.en_hh & 0x03) << 2;

		if (props->digital.en_mm)
			dig_en |= (props->digital.en_mm & 0x03);

		dst[DIGITAL_EN_REG] = dig_en;
	}

	switch (props->cur_time.interval) {
	case ALG_INTERVAL_100m:
		sc_update_rate = 0x03;
		sc_inc_step = 0x10;
		break;
	case ALG_INTERVAL_200m:
		sc_update_rate = 0x06;
		sc_inc_step = 0x11;
		break;
	case ALG_INTERVAL_500m:
		sc_update_rate = 0x0f;
		sc_inc_step = 0x12;
		break;
	case ALG_INTERVAL_1000:
		sc_update_rate = 0x1e;
		sc_inc_step = 0x13;
		break;
	case INTERVAL_DEBUG:
		sc_update_rate = 0x01;
		sc_inc_step = 0x13;
		break;
	default:
		sc_update_rate = 0x01;
		sc_inc_step = 0x13;
		panel_err("AOD:ERR:%s:undefined interval mode : %d\n",
			__func__, props->cur_time.interval);
		break;
	}

	if (props->first_clk_update) {
		sc_inc_step &= ~(0x10);
		props->first_clk_update = 0;
	}

	if (props->digital.en)
		sc_inc_step &= ~(0x10);

	if (props->cur_time.disp_24h)
		en_reg |= SC_24H_EN;

	dst[TIME_RATE_REG] = sc_update_rate;
	dst[TIME_COMP_REG] = sc_inc_step;
	dst[TIMER_EN_REG] = en_reg;

	panel_info("AOD:INFO:%s: %x %x %x\n", __func__, dst[0], dst[1], dst[2]);
}

void s6e3fa7_copy_icon_grid_on_ctrl(struct maptbl *tbl, u8 *dst)
{
	char enable = 0;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if (aod == NULL) {
		panel_err("AOD:ERR:%s:aod is null\n", __func__);
		return;
	}

	if (props->self_grid.en) {

		dst[GI_SX_POS0_REG] = (char)(props->self_grid.s_pos_x >> 8);
		dst[GI_SX_POS1_REG] = (char)(props->self_grid.s_pos_x & 0x00ff);
		dst[GI_SY_POS0_REG] = (char)(props->self_grid.s_pos_y >> 8);
		dst[GI_SY_POS1_REG] = (char)(props->self_grid.s_pos_y & 0x00ff);

		dst[GI_EX_POS0_REG] = (char)(props->self_grid.e_pos_x >> 8);
		dst[GI_EX_POS1_REG] = (char)(props->self_grid.e_pos_x & 0x00ff);
		dst[GI_EY_POS0_REG] = (char)(props->self_grid.e_pos_y >> 8);
		dst[GI_EY_POS1_REG] = (char)(props->self_grid.e_pos_y & 0x00ff);

		enable |= SG_GRID_ENABLE;
	}

	if (props->icon.en) {
		dst[SI_POS_X_POS0_REG] = (char)(props->icon.pos_x >> 8);
		dst[SI_POS_X_POS1_REG] = (char)(props->icon.pos_x & 0x00ff);
		dst[SI_POS_Y_POS0_REG] = (char)(props->icon.pos_y >> 8);
		dst[SI_POS_Y_POS1_REG] = (char)(props->icon.pos_y & 0x00ff);

		dst[SI_IMG_WIDTH0_REG] = (char)(props->icon.width >> 8);
		dst[SI_IMG_WIDTH1_REG] = (char)(props->icon.width & 0xff);
		dst[SI_IMG_HEIGHT0_REG] = (char)(props->icon.height >> 8);
		dst[SI_IMG_HEIGHT1_REG] = (char)(props->icon.height & 0x00ff);

		enable |= SI_ICON_ENABLE;
	}

	dst[SI_ENABLE_REG] = enable;

	panel_info("AOD:INFO:%s: %x %x %x %x\n", __func__,
		dst[SI_POS_X_POS0_REG], dst[SI_POS_X_POS1_REG],
		dst[SI_POS_Y_POS0_REG], dst[SI_POS_Y_POS1_REG]);
}


void s6e3fa7_copy_self_move_on_ctrl(struct maptbl *tbl, u8 *dst)
{
	char enable = 0x3;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if (props->icon.en)
		enable &= ~(ICON_SELF_MOVE_EN);

	if (props->self_move_en)
		enable &= ~(FB_SELF_MOVE_EN);

	dst[SM_ENABLE_REG] = enable;

	panel_info("AOD:INFO:%s: %x\n", __func__, dst[SM_ENABLE_REG]);
}


void s6e3fa7_copy_analog_pos_ctrl(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if (props->analog.en) {
		dst[ANALOG_POS_X1_REG] = (char)(props->analog.pos_x >> 8);
		dst[ANALOG_POS_X2_REG] = (char)(props->analog.pos_x & 0xff);
		dst[ANALOG_POS_Y1_REG] = (char)(props->analog.pos_y >> 8);
		dst[ANALOG_POS_Y2_REG] = (char)(props->analog.pos_y & 0xff);
	}

	switch (props->analog.rotate) {
	case ALG_ROTATE_0:
		dst[ANALOG_ROT_REG] = ALG_ROTATE_0;
		break;
	case ALG_ROTATE_90:
		dst[ANALOG_ROT_REG] = ALG_ROTATE_90;
		break;
	case ALG_ROTATE_180:
		dst[ANALOG_ROT_REG] = ALG_ROTATE_180;
		break;
	case ALG_ROTATE_270:
		dst[ANALOG_ROT_REG] = ALG_ROTATE_270;
		break;
	default:
		dst[ANALOG_ROT_REG] = ALG_ROTATE_0;
		panel_err("AOD:ERR:%s:undefined rotation mode : %d\n",
			__func__, props->analog.rotate);
		break;
	}

	panel_info("AOD:INFO:%s: %x\n", __func__, dst[ANALOG_POS_X1_REG]);
}

void s6e3fa7_copy_analog_clock_ctrl(struct maptbl *tbl, u8 *dst)
{
	char en_reg = 0;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if (aod == NULL) {
		panel_err("AOD:ERR:%s:aod is null\n", __func__);
		return;
	}

	en_reg |= (SC_TIME_EN | SC_A_CLK_EN);

	if (props->analog.en) {
		if (props->prev_rotate != props->analog.rotate) {
			panel_info("AOD:INFO:%s:analog rotate mismatch: %d->%d\n",
				__func__,props->prev_rotate, props->analog.rotate);
			msleep(1000);
		}
		en_reg |= SC_DISP_ON;
	}
	if (props->cur_time.disp_24h)
		en_reg |= SC_24H_EN;

	dst[TIMER_EN_REG] = en_reg;

	panel_info("AOD:INFO:%s: %x %x %x\n", __func__, dst[0], dst[1], dst[2]);
}

void s6e3fa7_copy_digital_clock_ctrl(struct maptbl *tbl, u8 *dst)
{
	char en_reg = 0;
	char dig_en = 0;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if (aod == NULL) {
		panel_err("AOD:ERR:%s:aod is null\n", __func__);
		return;
	}

	en_reg |= SC_TIME_EN;

	if (props->digital.en) {
		en_reg |= (SC_D_CLK_EN | SC_DISP_ON);

		if (props->digital.en_hh)
			dig_en |= (props->digital.en_hh & 0x03) << 2;

		if (props->digital.en_mm)
			dig_en |= (props->digital.en_mm & 0x03);

		dst[DIGITAL_EN_REG] = dig_en;
	}

	if (props->cur_time.disp_24h)
		en_reg |= SC_24H_EN;

	dst[TIMER_EN_REG] = en_reg;

	panel_info("AOD:INFO:%s: %x %x %x\n", __func__, dst[0], dst[1], dst[2]);
}


int s6e3fa7_getidx_self_mode_pos(struct maptbl *tbl)
{
	int row = 0;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	switch (props->cur_time.interval) {
	case ALG_INTERVAL_100m:
		panel_info("AOD:INFO:%s:interval : 100msec\n", __func__);
		row = 0;
		break;
	case ALG_INTERVAL_200m:
		panel_info("AOD:INFO:%s:interval : 200msec\n", __func__);
		row = 1;
		break;
	case ALG_INTERVAL_500m:
		panel_info("AOD:INFO:%s:interval : 500msec\n", __func__);
		row = 2;
		break;
	case ALG_INTERVAL_1000:
		panel_info("AOD:INFO:%s:interval : 1sec\n", __func__);
		row = 3;
		break;
	case INTERVAL_DEBUG:
		panel_info("AOD:INFO:%s:interval : debug\n", __func__);
		row = 4;
		break;
	default:
		panel_info("AOD:INFO:%s:invalid interval:%d\n",
			__func__, props->cur_time.interval);
		row = 0;
		break;
	}
	return maptbl_index(tbl, 0, row, 0);
}

#define REG_MOVE_DSP_X	4

void s6e3fa7_copy_self_move_reset(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	props->self_reset_cnt++;

	dst[REG_MOVE_DSP_X] = (char)props->self_reset_cnt;

	panel_info("AOD:INFO:%s: %x:%x:%x:%x:%x\n",
		__func__, dst[0], dst[1], dst[2], dst[3], dst[4]);
}

#ifdef SUPPORT_NORMAL_SELF_MOVE
int s6e3fa7_getidx_self_pattern(struct maptbl *tbl)
{
	int row = 0;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	switch (props->self_move_pattern) {
	case 1:
	case 3:
		panel_info("AOD:INFO:%s:pattern : %d\n",
				__func__, props->self_move_pattern);
		row = 0;
		break;
	case 2:
	case 4:
		panel_info("AOD:INFO:%s:pattern : %d\n",
				__func__, props->self_move_pattern);
		row = 1;
		break;
	default:
		panel_info("AOD:INFO:%s:invalid pattern:%d\n",
			__func__, props->self_move_pattern);
		row = 0;
		break;
	}

	return maptbl_index(tbl, 0, row, 0);
}

#if 0
void s6e3fa7_copy_self_move_enable(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	dst[8] = (char)props->self_move_interval;

	panel_info("AOD:INFO:%s: %x:%x:%x:%x:%x:%x:%x:%x:(%x)\n",
		__func__, dst[0], dst[1], dst[2], dst[3], dst[4],
		dst[5], dst[6], dst[7], dst[8]);
}
#endif

void s6e3fa7_copy_self_move_pattern(struct maptbl *tbl, u8 *dst)
{
	int idx;

	if (!tbl || !dst) {
		pr_err("%s, invalid parameter (tbl %p, dst %p\n",
				__func__, tbl, dst);
		return;
	}

	idx = maptbl_getidx(tbl);
	if (idx < 0) {
		pr_err("%s, failed to getidx %d\n", __func__, idx);
		return;
	}
	memcpy(dst, &(tbl->arr)[idx], sizeof(u8) * tbl->sz_copy);

#ifdef FAST_TIMER
	dst[7] = 0x22;
#endif

	panel_info("AOD:INFO:%s: %x:%x:%x:%x:%x:%x:%x:(%x):%x\n",
		__func__, dst[0], dst[1], dst[2], dst[3],
		dst[4], dst[5], dst[6], dst[7], dst[8]);
}
#endif
#endif
