/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_MODULE_PREPROC_H
#define FIMC_IS_MODULE_PREPROC_H

#include "fimc-is-preprocessor-c3.h"
#include "fimc-is-core.h"

#define EXT_CLK_Mhz (26)

#define CHECK_INIT_N_RET(s)				\
	if (!(s)) {					\
		err("Does not initialized!");	\
		return -EPERM;				\
	}

struct fimc_is_module_preprocessor {
	sencmd_func			cmd_func[COMP_CMD_END + 1];
	cis_shared_data			*cis_data;
	bool				initialized;
	u8 *pdaf_buf[PREPROC_PDAF_BUF_NUM];
	int pdaf_count;
};

#endif

