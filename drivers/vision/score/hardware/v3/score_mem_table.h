/*
 * Samsung Exynos SoC series SCore driver
 *
 * Definition of memory in kernel space that SCore device uses
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_MEM_TABLE_H__
#define __SCORE_MEM_TABLE_H__

#include "score_firmware.h"

/* Memory size that SCore driver uses in kernel space */
#define SCORE_MEMORY_INTERNAL_SIZE	(SZ_16M)

#define SCORE_FW_SIZE			(SZ_1M * 3)

#define SCORE_TS_MALLOC_SIZE		(SZ_2M)
#define SCORE_BR_MALLOC_SIZE		(SZ_1M)
#define SCORE_MALLOC_SIZE		\
	(SCORE_TS_MALLOC_SIZE + SCORE_BR_MALLOC_SIZE * 2)
#define SCORE_PRINT_SIZE		(SZ_1M)
#define SCORE_TS_PROFILER_SIZE		(SZ_1M)
#define SCORE_BR_PROFILER_SIZE		(SZ_1M)
#define SCORE_PROFILER_SIZE		\
	(SCORE_TS_PROFILER_SIZE + SCORE_BR_PROFILER_SIZE * 2)
#endif

