/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __EXYNOS_BCM_DBG_DUMP_H_
#define __EXYNOS_BCM_DBG_DUMP_H_

/* BCM DUMP format definition */
#define BCM_DUMP_PRE_DEFINE_SHIFT		(16)
#define BCM_DUMP_MAX_STR			(4 * 1024)

struct exynos_bcm_out_data {
	u32				ccnt;
	u32				pmcnt[BCM_EVT_EVENT_MAX];
};

struct exynos_bcm_dump_info {
	/*
	 * dump_header:
	 * [31] = dump validation
	 * [18:16] = pre-defined event index
	 * [5:0] = BCM IP index
	 */
	u32				dump_header;
	u32				dump_seq_no;
	u32				dump_time;
	struct exynos_bcm_out_data	out_data;
} __attribute__((packed));

#ifdef CONFIG_EXYNOS_BCM_DBG_DUMP
int exynos_bcm_dbg_buffer_dump(struct exynos_bcm_dbg_data *data, bool klog);
#else
#define exynos_bcm_dbg_buffer_dump(a, b) do {} while (0)
#endif

#endif	/* __EXYNOS_BCM_DBG_DUMP_H_ */
