/*
 * Samsung Exynos5 SoC series Actuator driver
 *
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DEVICE_ZC533_H
#define FIMC_IS_DEVICE_ZC533_H

#define ZC533_POS_SIZE_BIT		ACTUATOR_POS_SIZE_10BIT
#define ZC533_POS_MAX_SIZE		((1 << ZC533_POS_SIZE_BIT) - 1)
#define ZC533_POS_DIRECTION		ACTUATOR_RANGE_INF_TO_MAC

struct fimc_is_caldata_list_zc533 {
	u32 af_position_type;
	u32 af_position_worstt;
	u32 af_macro_position_type;
	u32 af_macro_position_worst;
	u32 af_default_position;
	u8 reserved0[12];
	u32 equipment_info;
	u8 reserved1[8];
	u32 cal_map_ver;
	u8 operating_mode;
	u8 reserved2;
	u8 gain;
	u8 reserved3[33];
};

#endif
