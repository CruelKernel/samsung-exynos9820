/*
 * linux/drivers/video/fbdev/exynos/panel/aod_drv.h
 *
 * Header file for AOD Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __AOD_DRV_H__
#define __AOD_DRV_H__

#include <linux/kernel.h>
#include <linux/miscdevice.h>

#include "aod_drv_ioctl.h"

#define SUPPORT_NORMAL_SELF_MOVE

#ifdef SUPPORT_NORMAL_SELF_MOVE
enum {
	NORMAL_SELF_MOVE_PATTERN_OFF,
	NORMAL_SELF_MOVE_PATTERN_1,
	NORMAL_SELF_MOVE_PATTERN_2,
	NORMAL_SELF_MOVE_PATTERN_3,
	NORMAL_SELF_MOVE_PATTERN_4,
	MAX_NORMAL_SELF_MOVE_PATTERN,
};
#endif

enum AOD_SEQ {
	SELF_MASK_IMG_SEQ = 0,
	SELF_MASK_ENA_SEQ,
	SELF_MASK_DIS_SEQ,
	ANALOG_IMG_SEQ,
	ANALOG_CTRL_SEQ,
	ANALOG_DISABLE_SEQ,
	DIGITAL_IMG_SEQ,
	DIGITAL_CTRL_SEQ,
	SELF_MOVE_ON_SEQ,
	SELF_MOVE_OFF_SEQ,
	SELF_MOVE_RESET_SEQ,
	SELF_ICON_IMG_SEQ,
	ICON_GRID_ON_SEQ,
	ICON_GRID_OFF_SEQ,
	ENTER_AOD_SEQ,
	EXIT_AOD_SEQ,
	SET_TIME_SEQ,
	//todo ha9 need additional config for ha9
	CTRL_ICON_SEQ,
	DISABLE_ICON_SEQ,
	ENABLE_PARTIAL_SCAN,
	DISABLE_PARTIAL_SCAN,
#ifdef SUPPORT_NORMAL_SELF_MOVE
	ENABLE_SELF_MOVE_SEQ,
	DISABLE_SELF_MOVE_SEQ,
#endif
	SELF_MASK_CHECKSUM_SEQ,
	MAX_AOD_SEQ,
};

enum AOD_MAPTBL {
	SELF_MASK_CTRL_MAPTBL = 0,
	ANALOG_POS_MAPTBL,
	ANALOG_CLK_CTRL_MAPTBL,
	DIGITAL_CLK_CTRL_MAPTBL,
	DIGITAL_POS_MAPTBL,
	DIGITAL_BLK_MAPTBL,
	SET_TIME_MAPTBL,
	ICON_GRID_ON_MAPTBL,
	SELF_MOVE_MAPTBL,
	SELF_MOVE_POS_MAPTBL,
	SELF_MOVE_RESET_MAPTBL,
//todo ha9 need additional config for ha9
	SET_TIME_RATE,
	CTRL_ICON,
	SET_DIGITAL_COLOR,
	SET_DIGITAL_UN_WIDTH,
	SET_PARTIAL_MODE,
	SET_PARTIAL_AREA,
	SET_PARTIAL_HLPM,
#ifdef SUPPORT_NORMAL_SELF_MOVE
	SELF_MOVE_PATTERN_MAPTBL,
#endif
	MAX_AOD_MAPTBL,
};

struct aod_tune {
	char *name;
	struct seqinfo *seqtbl;
	u32 nr_seqtbl;
	struct maptbl *maptbl;
	u32 nr_maptbl;
	int self_mask_en;
};

struct clk_pos {
	int pos_x;
	int pos_y;
};

struct img_info {
	u8 *buf;
	u32 size;
	unsigned int up_flag;
};

struct aod_ioctl_props {
	int self_mask_en;
	int self_move_en;
	int self_reset_cnt;
	/* Reg: 0x77, Offset : 0x03 */
	struct aod_cur_time cur_time;
	/* Reg: 0x77, Offset: 0x09 */
	struct clk_pos pos;
	/* Reg: 0x77, Offset: 0x07 */

	struct self_icon_info icon;
	struct self_grid_info self_grid;
	struct analog_clk_info analog;
	struct digital_clk_info digital;
	struct partial_scan_info partial;

	int clk_rate;
	int debug_interval;
	int debug_rotate;
	int debug_force_update;

	int first_clk_update;
#ifdef SUPPORT_NORMAL_SELF_MOVE
	int self_move_pattern;
#endif
	int prev_rotate;
	u8* self_mask_checksum;
		/*
		because len is different in each ddi
		0 : unsupport
	*/
	int self_mask_checksum_len;
};

struct aod_dev_info {
	int reset_flag;

	struct mutex lock;
	struct aod_ioctl_props props;
	struct notifier_block fb_notif;
	char *temp;
	struct seqinfo *seqtbl;
	u32 nr_seqtbl;
	struct maptbl *maptbl;
	u32 nr_maptbl;
	struct aod_ops *ops;
	struct miscdevice dev;

	struct img_info icon_img;
	struct img_info dc_img;
	struct img_info ac_img;
};

struct aod_ops {
	int (*init_panel)(struct aod_dev_info *aod_dev);
	int (*enter_to_lpm)(struct aod_dev_info *aod_dev);
	int (*exit_from_lpm)(struct aod_dev_info *aod_dev);
	int (*doze_suspend)(struct aod_dev_info *aod_dev);
	int (*power_off)(struct aod_dev_info *aod_dev);
#ifdef SUPPORT_NORMAL_SELF_MOVE
	int (*self_move_pattern_update)(struct aod_dev_info *aod_dev);
#endif
};

int aod_drv_probe(struct panel_device *panel, struct aod_tune *aod_tune);
int panel_do_aod_seqtbl_by_index(struct aod_dev_info *aod, int index);

#endif //__AOD_DRV_H__
