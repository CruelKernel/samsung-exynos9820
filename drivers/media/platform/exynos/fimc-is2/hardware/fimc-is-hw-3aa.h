/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_3AA_H
#define FIMC_IS_HW_3AA_H

#include "fimc-is-hw-control.h"
#include "fimc-is-interface-ddk.h"

struct fimc_is_hw_3aa {
	struct fimc_is_lib_isp		lib[FIMC_IS_STREAM_COUNT];
	struct fimc_is_lib_support	*lib_support;
	struct lib_interface_func	*lib_func;
	struct taa_param_set		param_set[FIMC_IS_STREAM_COUNT];
};

int fimc_is_hw_3aa_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
	struct fimc_is_interface_ischain *itfc, int id, const char *name);
int fimc_is_hw_3aa_mode_change(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map);
void fimc_is_hw_3aa_update_param(struct fimc_is_hw_ip *hw_ip, struct is_param_region *region,
	struct taa_param_set *param_set, u32 lindex, u32 hindex, u32 instance);
void fimc_is_hw_3aa_dump(void);
#endif
