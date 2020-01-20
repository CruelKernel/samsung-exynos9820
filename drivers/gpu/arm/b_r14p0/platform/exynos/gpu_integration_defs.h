/* drivers/gpu/arm/.../platform/gpu_integration_defs.h
 *
 * Copyright 2011 by S.LSI. Samsung Electronics Inc.
 * San#24, Nongseo-Dong, Giheung-Gu, Yongin, Korea
 *
 * Samsung SoC Mali-T Series DDK porting layer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software FoundatIon.
 */

/**
 * @file gpu_integration_defs.h
 * DDK porting layer.
 */

#ifndef _SEC_INTEGRATION_H_
#define _SEC_INTEGRATION_H_

#include <mali_kbase.h>
#include <mali_kbase_mem_linux.h>
#include "mali_kbase_platform.h"
#include "gpu_dvfs_handler.h"

/* kctx initialized with zero from vzalloc, so initialized value required only */
#define CTX_UNINITIALIZED 0x0
#define CTX_INITIALIZED 0x1
#define CTX_DESTROYED 0x2
#define CTX_NAME_SIZE 32

/* MALI_SEC_SECURE_RENDERING */
#define SMC_GPU_CRC_REGION_NUM		8

/* MALI_SEC_INTEGRATION */
#define KBASE_PM_TIME_SHIFT			8

/* MALI_SEC_INTEGRATION */
#define MEM_FREE_LIMITS 16384
#define MEM_FREE_DEFAULT 16384

uintptr_t gpu_get_callbacks(void);
int gpu_vendor_dispatch(struct kbase_context *kctx, u32 flags);
void gpu_cacheclean(struct kbase_device *kbdev);
void kbase_mem_free_list_cleanup(struct kbase_context *kctx);
void kbase_mem_set_max_size(struct kbase_context *kctx);
int gpu_memory_seq_show(struct seq_file *sfile, void *data);

struct kbase_vendor_callbacks {
	void (*create_context)(void *ctx);
	void (*destroy_context)(void *ctx);
	void (*pm_metrics_init)(void *dev);
	void (*pm_metrics_term)(void *dev);
	void (*cl_boost_init)(void *dev);
	void (*cl_boost_update_utilization)(void *dev, void *atom, u64 microseconds_spent);
	int (*get_core_mask)(void *dev);
	int (*init_hw)(void *dev);
	void (*debug_pagetable_info)(void *ctx, u64 vaddr);
	void (*jd_done_worker)(void *dev);
	void (*update_status)(void *dev, char *str, u32 val);
	bool (*mem_profile_check_kctx)(void *ctx);
	int (*register_dump)(void);
};

#endif /* _SEC_INTEGRATION_H_ */
