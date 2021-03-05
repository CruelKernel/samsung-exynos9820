/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha9/s6e3ha9_aod.c
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
#include "s6e3ha9_aod.h"

void s6e3ha9_copy_self_mask_ctrl(struct maptbl *tbl, u8 *dst)
{
	pr_info("%s was called\n", __func__);
	pr_info("%x %x %x\n", dst[0], dst[1], dst[2]);
}

int s6e3ha9_init_self_mask_ctrl(struct maptbl *tbl)
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
	pr_info("%s was called\n", __func__);
	return 0;
}

void s6e3ha9_copy_digital_pos(struct maptbl *tbl, u8 *dst)
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


/*beyond does not support blink - remove */
#if 0
void s6e3ha9_copy_digital_blink(struct maptbl *tbl, u8 *dst)
{
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
}
#endif

void s6e3ha9_copy_time(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if (aod == NULL) {
		panel_err("AOD:ERR:%s:aod is null\n", __func__);
		return;
	}

	dst[TIME_HH_REG] = props->cur_time.cur_h;
	dst[TIME_MM_REG] = props->cur_time.cur_m;
	dst[TIME_SS_REG] = props->cur_time.cur_s;
	dst[TIME_MSS_REG] = props->cur_time.cur_ms;

	panel_info("AOD:INFO:%s: %x %x %x\n", __func__, dst[0], dst[1], dst[2]);
}


void s6e3ha9_copy_timer_rate(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if ((aod == NULL) || (props == NULL)) {
		panel_err("AOD:ERR:%s:aod/props is null\n", __func__);
		return;
	}

	/* in case of analog set comp_en */
	if ((props->analog.en) && (props->first_clk_update == 0))
		dst[2] |= 0x10;
	else
		dst[2] &= ~0x10;

	switch (props->cur_time.interval) {
	case ALG_INTERVAL_100m:
		dst[1] = 0x3;
		dst[2] = (dst[2] & ~0x03);
		break;
	case ALG_INTERVAL_200m:
		dst[1] = 0x06;
		dst[2] = (dst[2] & ~0x03) | 0x01;
		break;
	case ALG_INTERVAL_500m:
		dst[1] = 0x0f;
		dst[2] = (dst[2] & ~0x03) | 0x02;
		break;
	case ALG_INTERVAL_1000:
		dst[1] = 0x1e;
		dst[2] = (dst[2] & ~0x03) | 0x03;
		break;
	case INTERVAL_DEBUG:
		dst[1] = 0x01;
		dst[2] = (dst[2] & ~0x03) | 0x03;
		break;
	default:
		panel_info("AOD:INFO:%s:invalid interval:%d\n",
			__func__, props->cur_time.interval);
		break;
	}
	panel_info("PANEL:INFO:%s:dst[1]:%x, dst[2]:%x\n",
		__func__, dst[1], dst[2]);
}


void s6e3ha9_copy_icon_grid_on_ctrl(struct maptbl *tbl, u8 *dst)
{
	char enable = 0;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if ((aod == NULL) || (props == NULL)) {
			panel_err("AOD:ERR:%s:aod/props is null\n", __func__);
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

#if 0
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
#endif
	dst[SI_ENABLE_REG] = enable;

	panel_info("AOD:INFO:%s: %x %x %x %x\n", __func__,
		dst[SI_POS_X_POS0_REG], dst[SI_POS_X_POS1_REG],
		dst[SI_POS_Y_POS0_REG], dst[SI_POS_Y_POS1_REG]);
}


void s6e3ha9_copy_self_move_on_ctrl(struct maptbl *tbl, u8 *dst)
{
	char enable = 0;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if ((aod == NULL) || (props == NULL)) {
		panel_err("AOD:ERR:%s:aod/props is null\n", __func__);
		return;
	}
#if 0
	if (props->icon.en)
		enable &= ~(ICON_SELF_MOVE_EN);
#endif
	if (props->self_move_en)
		enable |= FB_SELF_MOVE_EN;

	dst[SM_ENABLE_REG] = enable;

	panel_info("AOD:INFO:%s: %x\n", __func__, dst[SM_ENABLE_REG]);
}

void s6e3ha9_copy_analog_pos(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if ((aod == NULL) || (props == NULL)) {
		panel_err("AOD:ERR:%s:aod/props is null\n", __func__);
		return;
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

	dst[ANALOG_POS_X1_REG] = (char)(props->analog.pos_x >> 8);
	dst[ANALOG_POS_X2_REG] = (char)(props->analog.pos_x & 0xff);
	dst[ANALOG_POS_Y1_REG] = (char)(props->analog.pos_y >> 8);
	dst[ANALOG_POS_Y2_REG] = (char)(props->analog.pos_y & 0xff);

	panel_dbg("AOD:INFO:%s: %x\n", __func__, dst[ANALOG_POS_X1_REG]);
}

void s6e3ha9_copy_analog_en(struct maptbl *tbl, u8 *dst)
{
	char en_reg = 0;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if ((aod == NULL) || (props == NULL)) {
		panel_err("AOD:ERR:%s:aod/props is null\n", __func__);
		return;
	}

	if (props->analog.en)
		en_reg = SC_A_CLK_EN | SC_DISP_ON;

	dst[TIMER_EN_REG] = en_reg;

	panel_dbg("AOD:INFO:%s: %x %x %x\n", __func__, dst[0], dst[1], dst[2]);
}

#define DIGITAL_EN_REG 	0x02
#define DIGITAL_UN_REG	0x03
#define DIGITAL_FMT_REG 0x04

void s6e3ha9_copy_digital_en(struct maptbl *tbl, u8 *dst)
{
	char en_reg = 0;
	char disp_format = 0;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if (aod == NULL) {
		panel_err("AOD:ERR:%s:aod is null\n", __func__);
		return;
	}

	if (props->digital.en) {

		if (props->digital.en_hh)
			disp_format |= (props->digital.en_hh & 0x03) << 2;

		if (props->digital.en_mm)
			disp_format |= (props->digital.en_mm & 0x03);

		if (props->cur_time.disp_24h)
			disp_format |= (props->cur_time.disp_24h & 0x03) << 4;

		en_reg = SC_D_CLK_EN | SC_DISP_ON;

		dst[DIGITAL_UN_REG] = props->digital.unicode_attr;
		dst[DIGITAL_FMT_REG] = disp_format;
	}

	dst[DIGITAL_EN_REG] = en_reg;

	panel_info("AOD:INFO:%s: %x %x %x %x\n", __func__, dst[0], dst[1], dst[2], dst[3]);
}

int s6e3ha9_getidx_self_mode_pos(struct maptbl *tbl)
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

void s6e3ha9_copy_self_move_reset(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	props->self_reset_cnt++;

	dst[REG_MOVE_DSP_X] = (char)props->self_reset_cnt;

	panel_info("AOD:INFO:%s: %x:%x:%x:%x:%x\n",
		__func__, dst[0], dst[1], dst[2], dst[3], dst[4]);
}

#define ICON_REG_EN 2
#define ICON_ENABLE 0x11

#define ICON_REG_XPOS0	3
#define ICON_REG_XPOS1	4
#define ICON_REG_YPOS0	5
#define ICON_REG_YPOS1	6

#define ICON_REG_WIDTH0	7
#define ICON_REG_WIDTH1	8

#define ICON_REG_HEIGHT0	9
#define ICON_REG_HEIGHT1	10

#define ICON_REG_COLOR0		11
#define ICON_REG_COLOR1		12
#define ICON_REG_COLOR2		13
#define ICON_REG_COLOR3		14


void s6e3ha9_copy_icon_ctrl(struct maptbl *tbl, u8 *dst)
{
	u8 enable = 0;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if ((aod == NULL) || (props == NULL)) {
		panel_err("AOD:ERR:%s:aod/props is null\n", __func__);
		return;
	}

	panel_info("%s : %d\n", __func__, props->icon.en);

	if (props->icon.en) {

		dst[ICON_REG_XPOS0] = (char)(props->icon.pos_x >> 8);
		dst[ICON_REG_XPOS1] = (char)(props->icon.pos_x & 0x00ff);
		dst[ICON_REG_YPOS0] = (char)(props->icon.pos_y >> 8);
		dst[ICON_REG_YPOS1] = (char)(props->icon.pos_y & 0x00ff);

		dst[ICON_REG_WIDTH0] = (char)(props->icon.width >> 8);
		dst[ICON_REG_WIDTH1] = (char)(props->icon.width & 0xff);
		dst[ICON_REG_HEIGHT0] = (char)(props->icon.height >> 8);
		dst[ICON_REG_HEIGHT1] = (char)(props->icon.height & 0x00ff);

		dst[ICON_REG_COLOR0] = (char)(props->icon.color >> 24);
		dst[ICON_REG_COLOR1] = (char)(props->icon.color >> 16);
		dst[ICON_REG_COLOR2] = (char)(props->icon.color >> 8);
		dst[ICON_REG_COLOR3] = (char)(props->icon.color & 0x00ff);

		enable = ICON_ENABLE;
	}

	dst[ICON_REG_EN] = enable;

	panel_info("AOD:INFO:%s: %x:%x:%x:%x:%x\n",
		__func__, dst[0], dst[1], dst[2], dst[3], dst[4]);
}

#define DIG_COLOR_ALPHA_REG 0x1
#define DIG_COLOR_RED_REG	0x2
#define DIG_COLOR_GREEN_REG	0x3
#define DIG_COLOR_BLUE_REG	0x4

void s6e3ha9_copy_digital_color(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if ((aod == NULL) || (props == NULL)) {
		panel_err("AOD:ERR:%s:aod/props is null\n", __func__);
		return;
	}

	if (!props->digital.en)
		return;

	dst[DIG_COLOR_ALPHA_REG] = (char)((props->digital.color >> 24) & 0xff);
	dst[DIG_COLOR_RED_REG] = (char)((props->digital.color >> 16) & 0xff);
	dst[DIG_COLOR_GREEN_REG] = (char)((props->digital.color >> 8) & 0xff);
	dst[DIG_COLOR_BLUE_REG] = (char)(props->digital.color & 0xff);

}

#define DIG_UN_WIDTH0 	0x1
#define DIG_UN_WIDTH1 	0x2

void s6e3ha9_copy_digital_un_width(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if ((aod == NULL) || (props == NULL)) {
		panel_err("AOD:ERR:%s:aod/props is null\n", __func__);
		return;
	}

	if (!props->digital.en)
		return;

	dst[DIG_UN_WIDTH0] = (char)((props->digital.unicode_width >> 8) & 0xff);
	dst[DIG_UN_WIDTH1] = (char)(props->digital.unicode_width & 0xff);

}

#define SCAN_ENABLE_REG 0x1
#define SCAN_MODE_REG 0x2

#define ENABLE_PARTIAL_HLPM_VAL 0x01
#define ENABLE_PARTIAL_SCAN_VAL 0x10


void s6e3ha9_copy_partial_mode(struct maptbl *tbl, u8 *dst)
{
	u8 enable = 0;
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if ((aod == NULL) || (props == NULL)) {
		panel_err("AOD:ERR:%s:aod/props is null\n", __func__);
		return;
	}

	if (props->partial.hlpm_scan_en) {
		enable = dst[SCAN_ENABLE_REG] | ENABLE_PARTIAL_HLPM_VAL;
		dst[SCAN_MODE_REG] = props->partial.hlpm_mode_sel;
	}

	if (props->partial.scan_en)
		enable |= dst[SCAN_ENABLE_REG] | ENABLE_PARTIAL_SCAN_VAL;

	dst[SCAN_ENABLE_REG] = enable;

	panel_info("%s : enable : 0x%x\n", __func__, enable);
}

/*SP_PLT_SCAN_.. */
#define PARTIAL_AREA_SL0_REG 0x01
#define PARTIAL_AREA_SL1_REG 0x02
#define PARTIAL_AREA_EL0_REG 0x03
#define PARTIAL_AREA_EL1_REG 0x04

void s6e3ha9_copy_partial_area(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if ((aod == NULL) || (props == NULL)) {
		panel_err("AOD:ERR:%s:aod/props is null\n", __func__);
		return;
	}

	if (props->partial.scan_en) {
		dst[PARTIAL_AREA_SL0_REG] = (char)(props->partial.scan_sl >> 8);
		dst[PARTIAL_AREA_SL1_REG] = (char)(props->partial.scan_sl & 0xff);
		dst[PARTIAL_AREA_EL0_REG] = (char)(props->partial.scan_el >> 8);
		dst[PARTIAL_AREA_EL1_REG] = (char)(props->partial.scan_el & 0xff);
	}

}

#define PARTIAL_HLPM1_L0_REG 0x01
#define PARTIAL_HLPM1_L1_REG 0x02

#define PARTIAL_HLPM2_L0_REG 0x03
#define PARTIAL_HLPM2_L1_REG 0x04

#define PARTIAL_HLPM3_L0_REG 0x05
#define PARTIAL_HLPM3_L1_REG 0x06

#define PARTIAL_HLPM4_L0_REG 0x07
#define PARTIAL_HLPM4_L1_REG 0x08


void s6e3ha9_copy_partial_hlpm(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	if ((aod == NULL) || (props == NULL)) {
		panel_err("AOD:ERR:%s:aod/props is null\n", __func__);
		return;
	}

	if (props->partial.hlpm_scan_en) {
		dst[PARTIAL_HLPM1_L0_REG] = (char)(props->partial.hlpm_area_1 >> 8);
		dst[PARTIAL_HLPM1_L1_REG] = (char)(props->partial.hlpm_area_1 & 0xff);

		dst[PARTIAL_HLPM2_L0_REG] = (char)(props->partial.hlpm_area_2 >> 8);
		dst[PARTIAL_HLPM2_L1_REG] = (char)(props->partial.hlpm_area_2 & 0xff);

		dst[PARTIAL_HLPM3_L0_REG] = (char)(props->partial.hlpm_area_3 >> 8);
		dst[PARTIAL_HLPM3_L1_REG] = (char)(props->partial.hlpm_area_3 & 0xff);

		dst[PARTIAL_HLPM4_L0_REG] = (char)(props->partial.hlpm_area_4 >> 8);
		dst[PARTIAL_HLPM4_L1_REG] = (char)(props->partial.hlpm_area_4 & 0xff);
#if 0
		if ((props->partial.hlpm_mode_sel & 0x01) == 0) {
			dst[PARTIAL_HLPM1_L0_REG] = 0;
			dst[PARTIAL_HLPM1_L1_REG] = 0;
		}

		if ((props->partial.hlpm_mode_sel & 0x10) == 0) {
			dst[PARTIAL_HLPM4_L0_REG] = (3040 - 1) >> 8;
			dst[PARTIAL_HLPM4_L1_REG] = (3040 - 1 ) & 0xff;
		}
#endif
	}

}

#ifdef SUPPORT_NORMAL_SELF_MOVE
int s6e3ha9_getidx_self_pattern(struct maptbl *tbl)
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
void s6e3ha9_copy_self_move_enable(struct maptbl *tbl, u8 *dst)
{
	struct aod_dev_info *aod = tbl->pdata;
	struct aod_ioctl_props *props = &aod->props;

	dst[8] = (char)props->self_move_interval;

	panel_info("AOD:INFO:%s: %x:%x:%x:%x:%x:%x:%x:%x:(%x)\n",
		__func__, dst[0], dst[1], dst[2], dst[3], dst[4],
		dst[5], dst[6], dst[7], dst[8]);
}
#endif

void s6e3ha9_copy_self_move_pattern(struct maptbl *tbl, u8 *dst)
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
