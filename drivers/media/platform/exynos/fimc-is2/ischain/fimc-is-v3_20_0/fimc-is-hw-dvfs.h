/*
 * Samsung Exynos SoC series FIMC-IS driver
 *
 * exynos fimc-is2 core functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_DVFS_H
#define FIMC_IS_HW_DVFS_H

/* dvfs table idx ex.different dvfa table  pure bayer or dynamic bayer */
#define FIMC_IS_DVFS_TABLE_IDX_MAX 2

/* FIMC-IS DVFS SCENARIO enum */
enum FIMC_IS_SCENARIO_ID {
	FIMC_IS_SN_DEFAULT,
	FIMC_IS_SN_FRONT_PREVIEW,
	FIMC_IS_SN_FRONT_CAPTURE,
	FIMC_IS_SN_FRONT_CAMCORDING,
	FIMC_IS_SN_FRONT_CAMCORDING_CAPTURE,
	FIMC_IS_SN_FRONT_VT1,
	FIMC_IS_SN_FRONT_VT2,
	FIMC_IS_SN_REAR_PREVIEW,
	FIMC_IS_SN_REAR_CAPTURE,
	FIMC_IS_SN_REAR_CAMCORDING,
	FIMC_IS_SN_REAR_CAMCORDING_CAPTURE,
	FIMC_IS_SN_PREVIEW_HIGH_SPEED_FPS,
	FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS,
	FIMC_IS_SN_VIDEO_HIGH_SPEED_120FPS,
	FIMC_IS_SN_VRA,
	FIMC_IS_SN_MAX,
	FIMC_IS_SN_END,
};

/* for assign staic / dynamic scenario check logic data */
int fimc_is_hw_dvfs_init(void *dvfs_data);
#endif /* FIMC_IS_HW_DVFS_H */
