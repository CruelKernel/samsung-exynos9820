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

#ifndef FIMC_IS_CLK_GATE_H
#define FIMC_IS_CLK_GATE_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include "fimc-is-core.h"

int fimc_is_clk_gate_init(struct fimc_is_core *core);
int fimc_is_clk_gate_lock_set(struct fimc_is_core *core, u32 instance, u32 is_start);
/* For several groups */
int fimc_is_wrap_clk_gate_set(struct fimc_is_core *core,
			int msk_group_id, bool is_on);
/* For only single group */
int fimc_is_clk_gate_set(struct fimc_is_core *core,
			int group_id, bool is_on, bool skip_set_state, bool user_scenario);

int fimc_is_set_user_clk_gate(u32 group_id,
		struct fimc_is_core *core,
		bool is_on,
		unsigned long msk_state,
		struct exynos_fimc_is_clk_gate_info *gate_info);

#endif
