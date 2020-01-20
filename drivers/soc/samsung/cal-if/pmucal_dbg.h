/*
 * Exynos PMUCAL debug interface support.
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __PMUCAL_DBG_H
#define __PMUCAL_DBG_H __FILE__

enum PMUCAL_DBG_BLOCK {
	BLK_CPU,
	BLK_CLUSTER,
	BLK_LOCAL,
	BLK_SYSTEM,
	NUM_BLKS,
};

struct pmucal_dbg_info {
	u32 block_id;
	void *pmucal_data;
	u32 emul_offset;
	u32 emul_bit;
	u32 emul_en;
	u32 emul_enabled;
	u32 latency_offset;
	u64 on_latency_min;
	u64 on_latency_avg;
	u64 on_latency_max;
	u64 on_cnt;
	u64 off_latency_min;
	u64 off_latency_avg;
	u64 off_latency_max;
	u64 off_cnt;
};

#ifdef CONFIG_PMUCAL_DBG
void pmucal_dbg_set_emulation(struct pmucal_dbg_info *dbg);
void pmucal_dbg_req_emulation(struct pmucal_dbg_info *dbg, bool en);
void pmucal_dbg_do_profile(struct pmucal_dbg_info *dbg, bool is_on);
int pmucal_dbg_init(void);
#else
static inline void pmucal_dbg_set_emulation(struct pmucal_dbg_info *dbg)
{
	return;
}
static inline void pmucal_dbg_req_emulation(struct pmucal_dbg_info *dbg, bool en)
{
	return;
}
static inline void pmucal_dbg_do_profile(struct pmucal_dbg_info *dbg, bool is_on)
{
	return;
}
static inline int pmucal_dbg_init(void)
{
	return 0;
}
#endif

#endif /* __PMUCAL_DBG_H */
