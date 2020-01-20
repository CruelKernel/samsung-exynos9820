/*
 * linux/drivers/video/fbdev/exynos/aod/aod_drv_ioctl.h
 *
 * Source file for AOD Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>


#ifndef __AOD_DRV_IOCTL_H__
#define __AOD_DRV_IOCTL_H__


#define SUPPORT_INTERVAL

struct aod_cur_time {
	int cur_h;
	int cur_m;
	int cur_s;
	int cur_ms;
	int disp_24h;
#ifdef SUPPORT_INTERVAL
	int interval;
#endif
};

struct self_icon_info {
	u32 en;
	u32 pos_x;
	u32 pos_y;
	u32 width;
	u32 height;
	u32 color;
};

struct self_grid_info {
	int en;
	int s_pos_x;
	int s_pos_y;
	int e_pos_x;
	int e_pos_y;
};

#define ALG_INTERVAL_100m	0
#define ALG_INTERVAL_200m	1
#define ALG_INTERVAL_500m	2
#define ALG_INTERVAL_1000	3
#define INTERVAL_DEBUG		999


#define ALG_ROTATE_0		0
#define ALG_ROTATE_90		1
#define ALG_ROTATE_180		2
#define ALG_ROTATE_270		3

struct analog_clk_info {
	int en;
	int pos_x;
	int pos_y;
	int rotate;
	int mask_en;
	int mem_reuse_en;
};

struct digital_clk_info {
	int en;
	int en_hh;
	int en_mm;
	int pos1_x;
	int pos1_y;
	int pos2_x;
	int pos2_y;
	int pos3_x;
	int pos3_y;
	int pos4_x;
	int pos4_y;
	int img_width;
	int img_height;
	int color;
	int unicode_attr;
	int unicode_width;
};

struct partial_scan_info {
	u32 hlpm_scan_en;
	u32 hlpm_mode_sel;
	u32 hlpm_area_1;
	u32 hlpm_area_2;
	u32 hlpm_area_3;
	u32 hlpm_area_4;
	u32 scan_en;
	u32 scan_sl;
	u32 scan_el;
};

#define AOD_IOCTL_MAGIC	'S'

#define IOCTL_SELF_MOVE_EN			_IOW(AOD_IOCTL_MAGIC, 1, int)
#define IOCTL_SELF_MOVE_OFF			_IOW(AOD_IOCTL_MAGIC, 2, int)
#define IOCTL_SELF_MOVE_RESET		_IOW(AOD_IOCTL_MAGIC, 3, int)

#define IOCTL_SET_TIME				_IOW(AOD_IOCTL_MAGIC, 11, struct aod_cur_time)

#define IOCTL_SET_ICON				_IOW(AOD_IOCTL_MAGIC, 21, struct self_icon_info)

#define IOCTL_SET_GRID				_IOW(AOD_IOCTL_MAGIC, 31, struct self_grid_info)

#define IOCTL_SET_ANALOG_CLK		_IOW(AOD_IOCTL_MAGIC, 41, struct analog_clk_info)

#define IOCTL_SET_DIGITAL_CLK		_IOW(AOD_IOCTL_MAGIC, 51, struct digital_clk_info)

#define IOCTL_SET_PARTIAL_HLPM_SCAN _IOW(AOD_IOCTL_MAGIC, 61, struct partial_scan_info)
#endif //__AOD_DRV_IOCTL_H__

