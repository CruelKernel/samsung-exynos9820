/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DVFS_H
#define FIMC_IS_DVFS_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include "fimc-is-time.h"
#include "fimc-is-cmd.h"
#include "fimc-is-regs.h"
#include "fimc-is-err.h"
#include "fimc-is-video.h"
#include "fimc-is-groupmgr.h"
#include "fimc-is-device-ischain.h"

#define	DVFS_NOT_MATCHED	0 /* not matched in this scenario */
#define	DVFS_MATCHED		1 /* matched in this scenario */
#define	DVFS_SKIP		2 /* matched, but do not anything. skip changing dvfs */

#define KEEP_FRAME_TICK_DEFAULT (5)
#define FIMC_IS_DVFS_DUAL_TICK (4)
#define DVFS_SN_STR(__SCENARIO) #__SCENARIO
#define GET_DVFS_CHK_FUNC(__SCENARIO) check_ ## __SCENARIO
#define DECLARE_DVFS_CHK_FUNC(__SCENARIO) \
	int check_ ## __SCENARIO \
		(struct fimc_is_device_ischain *device, struct fimc_is_group *group, int position, int resol, \
			int fps, int stream_cnt, int streaming_cnt, unsigned long sensor_map, struct fimc_is_dual_info *dual_info, ...)
#define DECLARE_EXT_DVFS_CHK_FUNC(__SCENARIO) \
	int check_ ## __SCENARIO \
		(struct fimc_is_device_sensor *device, int position, int resol, int fps, \
			int stream_cnt, int streaming_cnt, unsigned long sensor_map, struct fimc_is_dual_info *dual_info, ...)
#define GET_KEY_FOR_DVFS_TBL_IDX(__HAL_VER) \
	(#__HAL_VER "_TBL_IDX")

#define DECLARE_DVFS_DT(SIZE, ...) \
	struct fimc_is_dvfs_dt_t fimc_is_dvfs_dt_arr[SIZE] = {__VA_ARGS__};
#define DECLARE_EXTERN_DVFS_DT(SIZE) \
	extern struct fimc_is_dvfs_dt_t fimc_is_dvfs_dt_arr[SIZE];

#define SIZE_HD (720 * 480)
#define SIZE_FHD (1920 * 1080)
#define SIZE_WHD (2560 * 1440)
#define SIZE_UHD (3840 * 2160)
#define SIZE_16MP_FHD_BDS (3072 * 2304) /* based a 4:3 */
#define SIZE_12MP_FHD_BDS (2688 * 2016) /* based a 4:3 */
#define SIZE_12MP_QHD_BDS (3072 * 2304)
#define SIZE_12MP_UHD_BDS (4032 * 3024)
#define SIZE_8MP_FHD_BDS (2176 * 1632)
#define SIZE_8MP_QHD_BDS (3264 * 1836)

struct fimc_is_dvfs_dt_t {
	const char *parse_scenario_nm;	/* string for parsing from DTS */
	u32 scenario_id;	/* scenario_id */
};

struct fimc_is_dvfs_scenario {
	u32 scenario_id;	/* scenario_id */
	char *scenario_nm;	/* string of scenario_id */
	int priority;		/* priority for dynamic scenario */
	int keep_frame_tick;	/* keep qos lock during specific frames when dynamic scenario */

	/* function pointer to check a scenario */
	int (*check_func)(struct fimc_is_device_ischain *device, struct fimc_is_group *group, int position, int resol,
			int fps, int stream_cnt, int streaming_cnt, unsigned long sensor_map, struct fimc_is_dual_info *dual_info, ...);
	int (*ext_check_func)(struct fimc_is_device_sensor *device, int position, int resol,
			int fps, int stream_cnt, int streaming_cnt, unsigned long sensor_map, struct fimc_is_dual_info *dual_info, ...);
};

struct fimc_is_dvfs_scenario_ctrl {
	int cur_scenario_id;	/* selected scenario idx */
	int cur_frame_tick;	/* remained frame tick to keep qos lock in dynamic scenario */
	int scenario_cnt;	/* total scenario count */
	int cur_scenario_idx;	/* selected scenario idx for scenarios */
	struct fimc_is_dvfs_scenario *scenarios;
};

int fimc_is_dvfs_init(struct fimc_is_resourcemgr *resourcemgr);
int fimc_is_dvfs_sel_table(struct fimc_is_resourcemgr *resourcemgr);
int fimc_is_dvfs_sel_static(struct fimc_is_device_ischain *device);
int fimc_is_dvfs_sel_dynamic(struct fimc_is_device_ischain *device, struct fimc_is_group *group);
int fimc_is_dvfs_sel_external(struct fimc_is_device_sensor *device);
int fimc_is_get_qos(struct fimc_is_core *core, u32 type, u32 scenario_id);
int fimc_is_set_dvfs(struct fimc_is_core *core, struct fimc_is_device_ischain *device, u32 scenario_id);
void fimc_is_dual_mode_update(struct fimc_is_device_ischain *device,
	struct fimc_is_group *group,
	struct fimc_is_frame *frame);
void fimc_is_dual_dvfs_update(struct fimc_is_device_ischain *device,
	struct fimc_is_group *group,
	struct fimc_is_frame *frame);

unsigned int fimc_is_get_bit_count(unsigned long bits);
#endif
