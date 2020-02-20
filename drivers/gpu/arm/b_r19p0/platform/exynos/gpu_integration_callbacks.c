/* drivers/gpu/arm/.../platform/gpu_integration_callbacks.c
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
 * @file gpu_integration_callbacks.c
 * DDK porting layer.
 */

#include <mali_kbase.h>
#include <mali_midg_regmap.h>
#include <mali_kbase_sync.h>

#include <linux/pm_qos.h>
#include <linux/sched.h>

#include <mali_kbase_gpu_memory_debugfs.h>
#include <backend/gpu/mali_kbase_device_internal.h>

#if MALI_SEC_PROBE_TEST != 1
#include <platform/exynos/gpu_integration_defs.h>
#endif

#if defined(CONFIG_SCHED_EMS)
#include <linux/ems.h>
static struct gb_qos_request gb_req = {
	.name = "ems_boost",
};
#elif defined(CONFIG_SCHED_EHMP)
#include <linux/ehmp.h>
static struct gb_qos_request gb_req = {
		.name = "ehmp_boost",
};
#elif defined(CONFIG_SCHED_HMP)
extern int set_hmp_boost(int enable);
#endif

/* MALI_SEC_INTEGRATION */
#include <mali_uk.h>
#define KBASE_REG_CUSTOM_TMEM       (1ul << 19)
#define KBASE_REG_CUSTOM_PMEM       (1ul << 20)

#define ENTRY_TYPE_MASK     3ULL
#define ENTRY_IS_ATE        1ULL
#define ENTRY_IS_INVAL      2ULL
#define ENTRY_IS_PTE        3ULL

#define ENTRY_ATTR_BITS (7ULL << 2)	/* bits 4:2 */
#define ENTRY_RD_BIT (1ULL << 6)
#define ENTRY_WR_BIT (1ULL << 7)
#define ENTRY_SHARE_BITS (3ULL << 8)	/* bits 9:8 */
#define ENTRY_ACCESS_BIT (1ULL << 10)
#define ENTRY_NX_BIT (1ULL << 54)

#define ENTRY_FLAGS_MASK (ENTRY_ATTR_BITS | ENTRY_RD_BIT | ENTRY_WR_BIT | \
		ENTRY_SHARE_BITS | ENTRY_ACCESS_BIT | ENTRY_NX_BIT)

/*
* peak_flops: 100/85
* sobel: 100/50
*/
#define COMPUTE_JOB_WEIGHT (10000/50)

#ifdef CONFIG_SENSORS_SEC_THERMISTOR
extern int sec_therm_get_ap_temperature(void);
#endif

#ifdef CONFIG_MALI_SEC_VK_BOOST
#include <linux/pm_qos.h>
extern struct pm_qos_request exynos5_g3d_mif_min_qos;
#endif

extern int gpu_register_dump(void);

void gpu_create_context(void *ctx)
{
#if MALI_SEC_PROBE_TEST != 1
	struct kbase_context *kctx;
	char current_name[sizeof(current->comm)];

	kctx = (struct kbase_context *)ctx;
	KBASE_DEBUG_ASSERT(kctx != NULL);

	kctx->ctx_status = CTX_UNINITIALIZED;

	get_task_comm(current_name, current);
	strncpy((char *)(&kctx->name), current_name, CTX_NAME_SIZE);

	kctx->ctx_status = CTX_INITIALIZED;

	kctx->destroying_context = false;

	kctx->need_to_force_schedule_out = false;
#endif
}

void gpu_destroy_context(void *ctx)
{
#if MALI_SEC_PROBE_TEST != 1
	struct kbase_context *kctx;
	struct kbase_device *kbdev;
#if (defined(CONFIG_SCHED_EMS) || defined(CONFIG_SCHED_EHMP) || defined(CONFIG_SCHED_HMP) || defined(CONFIG_MALI_SEC_VK_BOOST))
	struct exynos_context *platform;
#endif

	kctx = (struct kbase_context *)ctx;
	KBASE_DEBUG_ASSERT(kctx != NULL);

	kbdev = kctx->kbdev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	kctx->destroying_context = true;

	kctx->ctx_status = CTX_DESTROYED;

#ifdef CONFIG_MALI_DVFS
	gpu_dvfs_boost_lock(GPU_DVFS_BOOST_UNSET);
#endif
#if defined(CONFIG_SCHED_EMS) || defined(CONFIG_SCHED_EHMP)
	platform = (struct exynos_context *) kbdev->platform_context;
	mutex_lock(&platform->gpu_sched_hmp_lock);
	if (platform->ctx_need_qos)
	{
		platform->ctx_need_qos = false;
		gb_qos_update_request(&gb_req, 0);
	}

	mutex_unlock(&platform->gpu_sched_hmp_lock);
#elif defined(CONFIG_SCHED_HMP)
    platform = (struct exynos_context *) kbdev->platform_context;
    mutex_lock(&platform->gpu_sched_hmp_lock);
    if (platform->ctx_need_qos) {
        platform->ctx_need_qos = false;
        set_hmp_boost(0);
        set_hmp_aggressive_up_migration(false);
        set_hmp_aggressive_yield(false);
    }
    mutex_unlock(&platform->gpu_sched_hmp_lock);
#endif
#ifdef CONFIG_MALI_SEC_VK_BOOST
	platform = (struct exynos_context *) kbdev->platform_context;
	mutex_lock(&platform->gpu_vk_boost_lock);

	if (kctx->ctx_vk_need_qos) {
		pm_qos_update_request(&exynos5_g3d_mif_min_qos, platform->table[platform->step].mem_freq);
		kctx->ctx_vk_need_qos = false;
		platform->ctx_vk_need_qos = false;
	}

	mutex_unlock(&platform->gpu_vk_boost_lock);
#endif
#ifdef CONFIG_MALI_SEC_CL_BOOST
    platform->cl_boost_disable = false;
#endif
#endif /* MALI_SEC_PROBE_TEST */
}

int gpu_vendor_dispatch(struct kbase_context *kctx, u32 flags)
{
	struct kbase_device *kbdev;

	kbdev = kctx->kbdev;

	switch (flags)
	{
#if MALI_SEC_PROBE_TEST != 1
	case KBASE_FUNC_STEP_UP_MAX_GPU_LIMIT:
		{
#ifdef CONFIG_MALI_DVFS
			struct exynos_context *platform = (struct exynos_context *)kbdev->platform_context;

			if (!platform->using_max_limit_clock) {
				platform->using_max_limit_clock = true;
			}
#endif
			break;
		}
	case KBASE_FUNC_RESTORE_MAX_GPU_LIMIT:
		{
#ifdef CONFIG_MALI_DVFS
			struct exynos_context *platform = (struct exynos_context *)kbdev->platform_context;

			if (platform->using_max_limit_clock) {
				platform->using_max_limit_clock = false;
			}
#endif
			break;
		}
	case KBASE_FUNC_SET_MIN_LOCK:
		{
#if defined(CONFIG_MALI_PM_QOS)
			struct exynos_context *platform;
			platform = (struct exynos_context *) kbdev->platform_context;
#if (defined(CONFIG_SCHED_EMS) || defined(CONFIG_SCHED_EHMP))
			mutex_lock(&platform->gpu_sched_hmp_lock);
			if (!platform->ctx_need_qos) {
				platform->ctx_need_qos = true;
				/* set hmp boost */
				gb_qos_update_request(&gb_req, 100);
			}
			mutex_unlock(&platform->gpu_sched_hmp_lock);
			gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_EGL_SET);
#elif defined(CONFIG_SCHED_HMP)
			mutex_lock(&platform->gpu_sched_hmp_lock);
			if (!platform->ctx_need_qos) {
				platform->ctx_need_qos = true;
				/* set hmp boost */
				set_hmp_boost(1);
				set_hmp_aggressive_up_migration(true);
				set_hmp_aggressive_yield(true);
			}
			mutex_unlock(&platform->gpu_sched_hmp_lock);
			gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_EGL_SET);
#endif
#endif /* CONFIG_MALI_PM_QOS */
			break;
		}

	case KBASE_FUNC_UNSET_MIN_LOCK:
		{
#if defined(CONFIG_MALI_PM_QOS)
			struct exynos_context *platform;
			platform = (struct exynos_context *) kbdev->platform_context;
#if (defined(CONFIG_SCHED_EMS) || defined(CONFIG_SCHED_EHMP))
			mutex_lock(&platform->gpu_sched_hmp_lock);
			if (platform->ctx_need_qos) {
				platform->ctx_need_qos = false;
				/* unset hmp boost */
				gb_qos_update_request(&gb_req, 0);
			}
			mutex_unlock(&platform->gpu_sched_hmp_lock);
			gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_EGL_RESET);
#elif defined(CONFIG_SCHED_HMP)
			mutex_lock(&platform->gpu_sched_hmp_lock);
			if (platform->ctx_need_qos) {
				platform->ctx_need_qos = false;
				/* unset hmp boost */
				set_hmp_boost(0);
				set_hmp_aggressive_up_migration(false);
				set_hmp_aggressive_yield(false);
			}
			mutex_unlock(&platform->gpu_sched_hmp_lock);
			gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_EGL_RESET);
#endif
#endif /* CONFIG_MALI_PM_QOS */
			break;
		}
	case KBASE_FUNC_SET_VK_BOOST_LOCK:
		{
#if defined(CONFIG_MALI_PM_QOS) && defined(CONFIG_MALI_SEC_VK_BOOST)
			struct exynos_context *platform;
			platform = (struct exynos_context *) kbdev->platform_context;

			mutex_lock(&platform->gpu_vk_boost_lock);

			if (!kctx->ctx_vk_need_qos) {
				kctx->ctx_vk_need_qos = true;
				platform->ctx_vk_need_qos = true;
			}

			if (platform->ctx_vk_need_qos == true && platform->max_lock == platform->gpu_vk_boost_max_clk_lock) {
				pm_qos_update_request(&exynos5_g3d_mif_min_qos, platform->gpu_vk_boost_mif_min_clk_lock);
			}

			mutex_unlock(&platform->gpu_vk_boost_lock);
#endif
			break;
		}
	case KBASE_FUNC_UNSET_VK_BOOST_LOCK:
		{
#if defined(CONFIG_MALI_PM_QOS) && defined(CONFIG_MALI_SEC_VK_BOOST)
			struct exynos_context *platform;
			platform = (struct exynos_context *) kbdev->platform_context;

			mutex_lock(&platform->gpu_vk_boost_lock);

			if (kctx->ctx_vk_need_qos) {
				kctx->ctx_vk_need_qos = false;
				platform->ctx_vk_need_qos = false;
				pm_qos_update_request(&exynos5_g3d_mif_min_qos, platform->table[platform->step].mem_freq);
			}

			mutex_unlock(&platform->gpu_vk_boost_lock);
#endif
			break;
		}
#endif /* MALI_SEC_PROBE_TEST */
	default:
		break;
	}
	return 0;
}

int gpu_memory_seq_show(struct seq_file *sfile, void *data)
{
	return 0;
}

void gpu_update_status(void *dev, char *str, u32 val)
{
	struct kbase_device *kbdev;
	struct exynos_context *platform;
	int i;

	kbdev = (struct kbase_device *)dev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	platform = (struct exynos_context *) kbdev->platform_context;
	if (strcmp(str, "completion_code") == 0) {
		if (val == 0x40)
			platform->gpu_exception_count[GPU_JOB_CONFIG_FAULT]++;
		else if (val == 0x41)
			platform->gpu_exception_count[GPU_JOB_POWER_FAULT]++;
		else if (val == 0x42)
			platform->gpu_exception_count[GPU_JOB_READ_FAULT]++;
		else if (val == 0x43)
			platform->gpu_exception_count[GPU_JOB_WRITE_FAULT]++;
		else if (val == 0x44)
			platform->gpu_exception_count[GPU_JOB_AFFINITY_FAULT]++;
		else if (val == 0x48)
			platform->gpu_exception_count[GPU_JOB_BUS_FAULT]++;
		else if (val == 0x58)
			platform->gpu_exception_count[GPU_DATA_INVALIDATE_FAULT]++;
		else if (val == 0x59)
			platform->gpu_exception_count[GPU_TILE_RANGE_FAULT]++;
		else if (val == 0x60)
			platform->gpu_exception_count[GPU_OUT_OF_MEMORY_FAULT]++;
		/* GPU FAULT */
		else if (val == 0x80)
			platform->gpu_exception_count[GPU_DELAYED_BUS_FAULT]++;
		else if (val == 0x88)
			platform->gpu_exception_count[GPU_SHAREABILITY_FAULT]++;
		/* MMU FAULT */
		else if (val >= 0xC0 && val <= 0xC7)
			platform->gpu_exception_count[GPU_MMU_TRANSLATION_FAULT]++;
		else if (val >= 0xC8 && val <= 0xCF)
			platform->gpu_exception_count[GPU_MMU_PERMISSION_FAULT]++;
		else if (val >= 0xD0 && val <= 0xD7)
			platform->gpu_exception_count[GPU_MMU_TRANSTAB_BUS_FAULT]++;
		else if (val >= 0xD8 && val <= 0xDF)
			platform->gpu_exception_count[GPU_MMU_ACCESS_FLAG_FAULT]++;
		else if (val >= 0xE0 && val <= 0xE7)
			platform->gpu_exception_count[GPU_MMU_ADDRESS_SIZE_FAULT]++;
		else if (val >= 0xE8 && val <= 0xEF)
			platform->gpu_exception_count[GPU_MMU_MEMORY_ATTRIBUTES_FAULT]++;
		else
			platform->gpu_exception_count[GPU_UNKNOWN]++;
	} else if (strcmp(str, "soft_stop") == 0)
		platform->gpu_exception_count[GPU_SOFT_STOP]++;
	else if (strcmp(str, "hard_stop") == 0)
		platform->gpu_exception_count[GPU_HARD_STOP]++;
	else if (strcmp(str, "reset_count") == 0)
		platform->gpu_exception_count[GPU_RESET]++;

	for (i = GPU_JOB_CONFIG_FAULT; i < GPU_EXCEPTION_LIST_END; i++)
		platform->fault_count += platform->gpu_exception_count[i];
}

#define KBASE_MMU_PAGE_ENTRIES	512

static phys_addr_t mmu_pte_to_phy_addr(u64 entry)
{
	if (!(entry & 1))
		return 0;

	return entry & ~0xFFF;
}

/* MALI_SEC_INTEGRATION */
static void gpu_page_table_info_dp_level(struct kbase_context *kctx, u64 vaddr, phys_addr_t pgd, int level)
{
	u64 *pgd_page;
	int i;
	int index = (vaddr >> (12 + ((3 - level) * 9))) & 0x1FF;
	int min_index = index - 3;
	int max_index = index + 3;

	if (min_index < 0)
		min_index = 0;
	if (max_index >= KBASE_MMU_PAGE_ENTRIES)
		max_index = KBASE_MMU_PAGE_ENTRIES - 1;

	/* Map and dump entire page */

	pgd_page = kmap(pfn_to_page(PFN_DOWN(pgd)));

	dev_err(kctx->kbdev->dev, "Dumping level %d @ physical address 0x%016llX (matching index %d):\n", level, pgd, index);

	if (!pgd_page) {
		dev_err(kctx->kbdev->dev, "kmap failure\n");
		return;
	}

	for (i = min_index; i <= max_index; i++) {
		if (i == index) {
			dev_err(kctx->kbdev->dev, "[%03d]: 0x%016llX *\n", i, pgd_page[i]);
		} else {
			dev_err(kctx->kbdev->dev, "[%03d]: 0x%016llX\n", i, pgd_page[i]);
		}
	}

	/* parse next level (if any) */

	if ((pgd_page[index] & 3) == ENTRY_IS_PTE) {
		phys_addr_t target_pgd = mmu_pte_to_phy_addr(pgd_page[index]);
		gpu_page_table_info_dp_level(kctx, vaddr, target_pgd, level + 1);
	} else if ((pgd_page[index] & 3) == ENTRY_IS_ATE) {
		dev_err(kctx->kbdev->dev, "Final physical address: 0x%016llX\n", pgd_page[index] & ~(0xFFF | ENTRY_FLAGS_MASK));
	} else {
		dev_err(kctx->kbdev->dev, "Final physical address: INVALID!\n");
	}

	kunmap(pfn_to_page(PFN_DOWN(pgd)));
}

void gpu_debug_pagetable_info(void *ctx, u64 vaddr)
{
	struct kbase_context *kctx;

	kctx = (struct kbase_context *)ctx;
	KBASE_DEBUG_ASSERT(kctx != NULL);

	dev_err(kctx->kbdev->dev, "Looking up virtual GPU address: 0x%016llX\n", vaddr);
	gpu_page_table_info_dp_level(kctx, vaddr, kctx->mmu.pgd, 0);
}

#ifdef CONFIG_MALI_SEC_CL_BOOST
void gpu_cl_boost_init(void *dev)
{
	struct kbase_device *kbdev;

	kbdev = (struct kbase_device *)dev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	atomic_set(&kbdev->pm.backend.metrics.time_compute_jobs, 0);
	atomic_set(&kbdev->pm.backend.metrics.time_vertex_jobs, 0);
	atomic_set(&kbdev->pm.backend.metrics.time_fragment_jobs, 0);
}

void gpu_cl_boost_update_utilization(void *dev, void *atom, u64 microseconds_spent)
{
	struct kbase_jd_atom *katom;
	struct kbase_device *kbdev;

	kbdev = (struct kbase_device *)dev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	katom = (struct kbase_jd_atom *)atom;
	KBASE_DEBUG_ASSERT(katom != NULL);

	if (katom->core_req & BASE_JD_REQ_ONLY_COMPUTE)
		atomic_add((microseconds_spent >> KBASE_PM_TIME_SHIFT), &kbdev->pm.backend.metrics.time_compute_jobs);
	else if (katom->core_req & BASE_JD_REQ_FS)
		atomic_add((microseconds_spent >> KBASE_PM_TIME_SHIFT), &kbdev->pm.backend.metrics.time_fragment_jobs);
	else if (katom->core_req & BASE_JD_REQ_CS)
		atomic_add((microseconds_spent >> KBASE_PM_TIME_SHIFT), &kbdev->pm.backend.metrics.time_vertex_jobs);
}
#endif

#ifdef CONFIG_MALI_DVFS
static void dvfs_callback(struct work_struct *data)
{
	unsigned long flags;
	struct kbasep_pm_metrics_state *metrics;
	struct kbase_device *kbdev;
	struct exynos_context *platform;

	KBASE_DEBUG_ASSERT(data != NULL);

	metrics = container_of(data, struct kbasep_pm_metrics_state, work.work);

	kbdev = metrics->kbdev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	platform = (struct exynos_context *)kbdev->platform_context;
	KBASE_DEBUG_ASSERT(platform != NULL);

	kbase_platform_dvfs_event(metrics->kbdev, 0);

	spin_lock_irqsave(&metrics->lock, flags);

#ifdef CONFIG_MALI_RT_PM
	if (metrics->timer_active)
#endif
		queue_delayed_work_on(0, platform->dvfs_wq,
				platform->delayed_work, msecs_to_jiffies(platform->polling_speed));

	spin_unlock_irqrestore(&metrics->lock, flags);
}

void gpu_pm_metrics_init(void *dev)
{
	struct kbase_device *kbdev;
	struct exynos_context *platform;

	kbdev = (struct kbase_device *)dev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	platform = (struct exynos_context *)kbdev->platform_context;
	KBASE_DEBUG_ASSERT(platform != NULL);

	INIT_DELAYED_WORK(&kbdev->pm.backend.metrics.work, dvfs_callback);
	platform->dvfs_wq = create_workqueue("g3d_dvfs");
	platform->delayed_work = &kbdev->pm.backend.metrics.work;

	queue_delayed_work_on(0, platform->dvfs_wq,
		platform->delayed_work, msecs_to_jiffies(platform->polling_speed));
}

void gpu_pm_metrics_term(void *dev)
{
	struct kbase_device *kbdev;
	struct exynos_context *platform;

	kbdev = (struct kbase_device *)dev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	platform = (struct exynos_context *)kbdev->platform_context;
	KBASE_DEBUG_ASSERT(platform != NULL);

	cancel_delayed_work(platform->delayed_work);
	flush_workqueue(platform->dvfs_wq);
	destroy_workqueue(platform->dvfs_wq);
}
#endif

/* caller needs to hold kbdev->pm.backend.metrics.lock before calling this function */
#ifdef CONFIG_MALI_DVFS
int gpu_pm_get_dvfs_utilisation(struct kbase_device *kbdev, int *util_gl_share, int util_cl_share[2])
{
	unsigned long flags;
	int utilisation = 0;
#if !defined(CONFIG_MALI_SEC_CL_BOOST)
	int busy;
#else
	int compute_time = 0, vertex_time = 0, fragment_time = 0, total_time = 0, compute_time_rate = 0;
#endif

	ktime_t now = ktime_get();
	ktime_t diff;

	KBASE_DEBUG_ASSERT(kbdev != NULL);
	spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
	diff = ktime_sub(now, kbdev->pm.backend.metrics.time_period_start);

	if (kbdev->pm.backend.metrics.gpu_active) {
		u32 ns_time = (u32) (ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);
		kbdev->pm.backend.metrics.values.time_busy += ns_time;
		kbdev->pm.backend.metrics.values.busy_cl[0] += ns_time * kbdev->pm.backend.metrics.active_cl_ctx[0];
		kbdev->pm.backend.metrics.values.busy_cl[1] += ns_time * kbdev->pm.backend.metrics.active_cl_ctx[1];
		kbdev->pm.backend.metrics.time_period_start = now;
	} else {
		kbdev->pm.backend.metrics.values.time_idle += (u32) (ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);
		kbdev->pm.backend.metrics.time_period_start = now;
	}
	spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);
	if (kbdev->pm.backend.metrics.values.time_idle + kbdev->pm.backend.metrics.values.time_busy == 0) {
		/* No data - so we return NOP */
		utilisation = -1;
#if !defined(CONFIG_MALI_SEC_CL_BOOST)
		if (util_gl_share)
			*util_gl_share = -1;
		if (util_cl_share) {
			util_cl_share[0] = -1;
			util_cl_share[1] = -1;
		}
#endif
		goto out;
	}

	utilisation = (100 * kbdev->pm.backend.metrics.values.time_busy) /
			(kbdev->pm.backend.metrics.values.time_idle +
			 kbdev->pm.backend.metrics.values.time_busy);

#if !defined(CONFIG_MALI_SEC_CL_BOOST)
	busy = kbdev->pm.backend.metrics.values.busy_gl +
		kbdev->pm.backend.metrics.values.busy_cl[0] +
		kbdev->pm.backend.metrics.values.busy_cl[1];

	if (busy != 0) {
		if (util_gl_share)
			*util_gl_share =
				(100 * kbdev->pm.backend.metrics.values.busy_gl) / busy;
		if (util_cl_share) {
			util_cl_share[0] =
				(100 * kbdev->pm.backend.metrics.values.busy_cl[0]) / busy;
			util_cl_share[1] =
				(100 * kbdev->pm.backend.metrics.values.busy_cl[1]) / busy;
		}
	} else {
		if (util_gl_share)
			*util_gl_share = -1;
		if (util_cl_share) {
			util_cl_share[0] = -1;
			util_cl_share[1] = -1;
		}
	}
#endif

#ifdef CONFIG_MALI_SEC_CL_BOOST
	compute_time = atomic_read(&kbdev->pm.backend.metrics.time_compute_jobs);
	vertex_time = atomic_read(&kbdev->pm.backend.metrics.time_vertex_jobs);
	fragment_time = atomic_read(&kbdev->pm.backend.metrics.time_fragment_jobs);
	total_time = compute_time + vertex_time + fragment_time;

	if (compute_time > 0 && total_time > 0) {
		compute_time_rate = (100 * compute_time) / total_time;
		if (compute_time_rate == 100)
			kbdev->pm.backend.metrics.is_full_compute_util = true;
		else
			kbdev->pm.backend.metrics.is_full_compute_util = false;
	} else
		kbdev->pm.backend.metrics.is_full_compute_util = false;
#endif
 out:

	spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
	kbdev->pm.backend.metrics.values.time_idle = 0;
	kbdev->pm.backend.metrics.values.time_busy = 0;
#ifdef CONFIG_MALI_SEC_CL_BOOST
	atomic_set(&kbdev->pm.backend.metrics.time_compute_jobs, 0);
	atomic_set(&kbdev->pm.backend.metrics.time_vertex_jobs, 0);
	atomic_set(&kbdev->pm.backend.metrics.time_fragment_jobs, 0);
#else
	kbdev->pm.backend.metrics.values.busy_cl[0] = 0;
	kbdev->pm.backend.metrics.values.busy_cl[1] = 0;
	kbdev->pm.backend.metrics.values.busy_gl = 0;
#endif
	spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);
	return utilisation;
}
#endif /* CONFIG_MALI_DVFS */

/* MALI_SEC_INTEGRATION */
static bool gpu_mem_profile_check_kctx(void *ctx)
{
	bool found_element = false;
#if MALI_SEC_PROBE_TEST != 1
	struct kbase_device *kbdev;
	struct kbase_context *kctx, *tmp;

	kbdev = gpu_get_device_structure();

	list_for_each_entry_safe(kctx, tmp, &kbdev->kctx_list, kctx_list_link)	{
		if (kctx == (struct kbase_context *)ctx)
			if (kctx->destroying_context == false)	{
				found_element = true;
				break;
			}
	}
#endif
	return found_element;
}

struct kbase_vendor_callbacks exynos_callbacks = {
	.create_context = gpu_create_context,
	.destroy_context = gpu_destroy_context,
#ifdef CONFIG_MALI_SEC_CL_BOOST
	.cl_boost_init = gpu_cl_boost_init,
	.cl_boost_update_utilization = gpu_cl_boost_update_utilization,
#else
	.cl_boost_init = NULL,
	.cl_boost_update_utilization = NULL,
#endif
#if defined(CONFIG_SOC_EXYNOS7420) || defined(CONFIG_SOC_EXYNOS7890)
	.init_hw = exynos_gpu_init_hw,
#else
	.init_hw = NULL,
#endif
#ifdef CONFIG_MALI_DVFS
	.pm_metrics_init = gpu_pm_metrics_init,
	.pm_metrics_term = gpu_pm_metrics_term,
#else
	.pm_metrics_init = NULL,
	.pm_metrics_term = NULL,
#endif
	.debug_pagetable_info = gpu_debug_pagetable_info,
	.mem_profile_check_kctx = gpu_mem_profile_check_kctx,
#if MALI_SEC_PROBE_TEST != 1
	.register_dump = gpu_register_dump,
	.update_status = gpu_update_status,
#else
	.register_dump = NULL,
	.update_status = NULL,
#endif
};

uintptr_t gpu_get_callbacks(void)
{
	return (uintptr_t)&exynos_callbacks;
}

