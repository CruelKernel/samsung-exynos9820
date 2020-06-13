/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/memblock.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <soc/samsung/tmu.h>
#include <linux/isp_cooling.h>
#include <linux/cpuidle.h>
#include <linux/soc/samsung/exynos-soc.h>
#include <soc/samsung/bts.h>
#ifdef CONFIG_CMU_EWF
#include <soc/samsung/cmu_ewf.h>
#endif
#if defined(CONFIG_SECURE_CAMERA_USE)
#include <linux/smc.h>
#endif

#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>

#ifdef CONFIG_EXYNOS_BUSMONITOR
#include <linux/exynos-busmon.h>
#endif

#if IS_ENABLED(CONFIG_EXYNOS_SNAPSHOT)
#include <linux/exynos-ss.h>
#elif IS_ENABLED(CONFIG_DEBUG_SNAPSHOT)
#include <linux/debug-snapshot.h>
#endif
#include <soc/samsung/exynos-bcm_dbg.h>
#include <linux/sec_debug.h>

#include "fimc-is-resourcemgr.h"
#include "fimc-is-hw.h"
#include "fimc-is-debug.h"
#include "fimc-is-core.h"
#include "fimc-is-dvfs.h"
#include "fimc-is-clk-gate.h"
#if !defined(ENABLE_IS_CORE)
#include "fimc-is-interface-library.h"
#include "hardware/fimc-is-hw-control.h"
#endif

#if defined(SECURE_CAMERA_FACE)
#include <linux/ion_exynos.h>
#endif

#if defined(CONFIG_SCHED_EHMP)
#include <linux/ehmp.h>
struct gb_qos_request gb_req = {
	.name = "camera_ehmp_boost",
};
#elif defined(CONFIG_SCHED_EMS)
#include <linux/ems.h>
struct gb_qos_request gb_req = {
	.name = "camera_ems_boost",
};
#endif

#define DEBUG_LEVEL_LOW 0

#define CLUSTER_MIN_MASK			0x0000FFFF
#define CLUSTER_MIN_SHIFT			0
#define CLUSTER_MAX_MASK			0xFFFF0000
#define CLUSTER_MAX_SHIFT			16

#if defined(QOS_INTCAM)
struct pm_qos_request exynos_isp_qos_int_cam;
#endif
struct pm_qos_request exynos_isp_qos_int;
struct pm_qos_request exynos_isp_qos_mem;
struct pm_qos_request exynos_isp_qos_cam;
struct pm_qos_request exynos_isp_qos_disp;
struct pm_qos_request exynos_isp_qos_hpg;
struct pm_qos_request exynos_isp_qos_cluster0_min;
struct pm_qos_request exynos_isp_qos_cluster0_max;
struct pm_qos_request exynos_isp_qos_cluster1_min;
struct pm_qos_request exynos_isp_qos_cluster1_max;
struct pm_qos_request exynos_isp_qos_cluster2_min;
struct pm_qos_request exynos_isp_qos_cluster2_max;
struct pm_qos_request exynos_isp_qos_cpu_online_min;
#ifdef CONFIG_CMU_EWF
unsigned int idx_ewf;
#endif

#define C0MIN_QOS_ADD(freq) pm_qos_add_request(&exynos_isp_qos_cluster0_min, PM_QOS_CLUSTER0_FREQ_MIN, freq * 1000)
#define C0MIN_QOS_DEL() pm_qos_remove_request(&exynos_isp_qos_cluster0_min)
#define C0MIN_QOS_UPDATE(freq) pm_qos_update_request(&exynos_isp_qos_cluster0_min, freq * 1000)
#define C0MAX_QOS_ADD(freq) pm_qos_add_request(&exynos_isp_qos_cluster0_max, PM_QOS_CLUSTER0_FREQ_MAX, freq * 1000)
#define C0MAX_QOS_DEL() pm_qos_remove_request(&exynos_isp_qos_cluster0_max)
#define C0MAX_QOS_UPDATE(freq) pm_qos_update_request(&exynos_isp_qos_cluster0_max, freq * 1000)
#define C1MIN_QOS_ADD(freq) pm_qos_add_request(&exynos_isp_qos_cluster1_min, PM_QOS_CLUSTER1_FREQ_MIN, freq * 1000)
#define C1MIN_QOS_DEL() pm_qos_remove_request(&exynos_isp_qos_cluster1_min)
#define C1MIN_QOS_UPDATE(freq) pm_qos_update_request(&exynos_isp_qos_cluster1_min, freq * 1000)
#define C1MAX_QOS_ADD(freq) pm_qos_add_request(&exynos_isp_qos_cluster1_max, PM_QOS_CLUSTER1_FREQ_MAX, freq * 1000)
#define C1MAX_QOS_DEL() pm_qos_remove_request(&exynos_isp_qos_cluster1_max)
#define C1MAX_QOS_UPDATE(freq) pm_qos_update_request(&exynos_isp_qos_cluster1_max, freq * 1000)
#define C2MIN_QOS_ADD(freq) pm_qos_add_request(&exynos_isp_qos_cluster2_min, PM_QOS_CLUSTER2_FREQ_MIN, freq * 1000)
#define C2MIN_QOS_DEL() pm_qos_remove_request(&exynos_isp_qos_cluster2_min)
#define C2MIN_QOS_UPDATE(freq) pm_qos_update_request(&exynos_isp_qos_cluster2_min, freq * 1000)
#define C2MAX_QOS_ADD(freq) pm_qos_add_request(&exynos_isp_qos_cluster2_max, PM_QOS_CLUSTER2_FREQ_MAX, freq * 1000)
#define C2MAX_QOS_DEL() pm_qos_remove_request(&exynos_isp_qos_cluster2_max)
#define C2MAX_QOS_UPDATE(freq) pm_qos_update_request(&exynos_isp_qos_cluster2_max, freq * 1000)

extern struct fimc_is_sysfs_debug sysfs_debug;
extern int fimc_is_sensor_runtime_suspend(struct device *dev);
extern int fimc_is_sensor_runtime_resume(struct device *dev);

#ifdef ENABLE_IS_CORE
static int fimc_is_resourcemgr_allocmem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_mem *mem = &resourcemgr->mem;
	struct fimc_is_minfo *minfo = &resourcemgr->minfo;
	int ret = 0;

	minfo->total_size = FW_MEM_SIZE;
#if defined (FW_SUSPEND_RESUME)
	minfo->total_size += FW_BACKUP_SIZE;
#endif
#if defined (ENABLE_ODC)
	minfo->total_size += SIZE_ODC_INTERNAL_BUF * NUM_ODC_INTERNAL_BUF;
#endif
#if defined (ENABLE_VDIS)
	minfo->total_size += SIZE_DIS_INTERNAL_BUF * NUM_DIS_INTERNAL_BUF;
#endif
#if defined (ENABLE_DNR_IN_TPU)
	minfo->total_size += SIZE_DNR_INTERNAL_BUF * NUM_DNR_INTERNAL_BUF;
#endif
#if defined (ENABLE_FD_DMA_INPUT)
	minfo->total_size += SIZE_LHFD_SHOT_BUF * MAX_LHFD_SHOT_BUF;
#endif

	minfo->pb_fw = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, minfo->total_size, NULL, 0);
	if (IS_ERR(minfo->pb_fw)) {
		err("failed to allocate buffer for FW");
		return -ENOMEM;
	}

#if defined (ENABLE_FD_SW)
	minfo->pb_lhfd = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, LHFD_MAP_SIZE, NULL, 0);
	if (IS_ERR(minfo->pb_lhfd)) {
		err("failed to allocate buffer for LHFD_MAP");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_lhfd->size;
#endif

	probe_info("[RSC] Internal memory size (aligned) : %08lx\n", minfo->total_size);

	return ret;
}

static int fimc_is_resourcemgr_initmem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_minfo *minfo = NULL;
	int ret = 0;
	u32 offset;

	ret = fimc_is_resourcemgr_allocmem(resourcemgr);
	if (ret) {
		err("Couldn't alloc for FIMC-IS firmware\n");
		ret = -ENOMEM;
		goto p_err;
	}

	minfo = &resourcemgr->minfo;
	/* set information */
	resourcemgr->minfo.dvaddr = CALL_BUFOP(minfo->pb_fw, dvaddr, minfo->pb_fw);
	resourcemgr->minfo.kvaddr = CALL_BUFOP(minfo->pb_fw, kvaddr, minfo->pb_fw);

	offset = SHARED_OFFSET;
	resourcemgr->minfo.dvaddr_fshared = resourcemgr->minfo.dvaddr + offset;
	resourcemgr->minfo.kvaddr_fshared = resourcemgr->minfo.kvaddr + offset;

	offset = FW_MEM_SIZE - PARAM_REGION_SIZE;
	resourcemgr->minfo.dvaddr_region = resourcemgr->minfo.dvaddr + offset;
	resourcemgr->minfo.kvaddr_region = resourcemgr->minfo.kvaddr + offset;

	offset = FW_MEM_SIZE;
#if defined (FW_SUSPEND_RESUME)
	offset += FW_BACKUP_SIZE;
#endif

#if defined(ENABLE_ODC) || defined(ENABLE_VDIS) || defined(ENABLE_DNR_IN_TPU)
	resourcemgr->minfo.dvaddr_tpu = resourcemgr->minfo.dvaddr + offset;
	resourcemgr->minfo.kvaddr_tpu = resourcemgr->minfo.kvaddr + offset;
#if defined (ENABLE_ODC)
	offset += (SIZE_ODC_INTERNAL_BUF * NUM_ODC_INTERNAL_BUF);
#endif
#if defined (ENABLE_VDIS)
	offset += (SIZE_DIS_INTERNAL_BUF * NUM_DIS_INTERNAL_BUF);
#endif
#if defined (ENABLE_DNR_IN_TPU)
	offset += (SIZE_DNR_INTERNAL_BUF * NUM_DNR_INTERNAL_BUF);
#endif
#else
	resourcemgr->minfo.dvaddr_tpu = 0;
	resourcemgr->minfo.kvaddr_tpu = 0;
#endif

#if defined(ENABLE_FD_SW)
	resourcemgr->minfo.dvaddr_lhfd = CALL_BUFOP(minfo->pb_lhfd, dvaddr, minfo->pb_lhfd);
	resourcemgr->minfo.kvaddr_lhfd = CALL_BUFOP(minfo->pb_lhfd, kvaddr, minfo->pb_lhfd);
#else
	resourcemgr->minfo.dvaddr_lhfd = 0;
	resourcemgr->minfo.kvaddr_lhfd = 0;
#endif
	resourcemgr->minfo.kvaddr_debug_cnt =  resourcemgr->minfo.kvaddr +
					DEBUG_REGION_OFFSET + DEBUG_REGION_SIZE;

	probe_info("[RSC] Kernel virtual for internal: 0x%lx\n", resourcemgr->minfo.kvaddr);
	probe_info("[RSC] Device virtual for internal: 0x%llx\n", resourcemgr->minfo.dvaddr);
	probe_info("[RSC] fimc_is_init_mem done\n");

p_err:
	return ret;
}

#ifndef ENABLE_RESERVED_MEM
static int fimc_is_resourcemgr_deinitmem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_minfo *minfo = &resourcemgr->minfo;
	int ret = 0;

	CALL_VOID_BUFOP(minfo->pb_fw, free, minfo->pb_setfile);
#if defined (ENABLE_FD_SW)
	CALL_VOID_BUFOP(minfo->pb_lhfd, free, minfo->pb_lhfd);
#endif
	return ret;
}
#endif

#else /* #ifdef ENABLE_IS_CORE */
static int fimc_is_resourcemgr_allocmem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_mem *mem = &resourcemgr->mem;
	struct fimc_is_minfo *minfo = &resourcemgr->minfo;
	size_t tpu_size = 0;
#if !defined(ENABLE_DYNAMIC_MEM) && defined(ENABLE_TNR)
	size_t tnr_size = ((SIZE_TNR_IMAGE_BUF + SIZE_TNR_WEIGHT_BUF) * NUM_OF_TNR_BUF);
#endif
	int i;

	minfo->total_size = 0;
	/* setfile */
	minfo->pb_setfile = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, SETFILE_SIZE, NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_setfile)) {
		err("failed to allocate buffer for SETFILE");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_setfile->size;

	/* calibration data for each sensor postion */
	for (i = 0; i < SENSOR_POSITION_MAX; i++) {
		minfo->pb_cal[i] = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, CALDATA_SIZE, NULL, 0);
		if (IS_ERR_OR_NULL(minfo->pb_cal[i])) {
			err("failed to allocate buffer for REAR_CALDATA");
			return -ENOMEM;
		}
		minfo->total_size += minfo->pb_cal[i]->size;
	}

	/* library logging */
	minfo->pb_debug = mem->kmalloc(DEBUG_REGION_SIZE + 0x10, 16);
	if (IS_ERR_OR_NULL(minfo->pb_debug)) {
		/* retry by ION */
		minfo->pb_debug = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, DEBUG_REGION_SIZE + 0x10, NULL, 0);
		if (IS_ERR_OR_NULL(minfo->pb_debug)) {
			err("failed to allocate buffer for DEBUG_REGION");
			return -ENOMEM;
		}
	}
	minfo->total_size += minfo->pb_debug->size;

	/* library event logging */
	minfo->pb_event = mem->kmalloc(EVENT_REGION_SIZE + 0x10, 16);
	if (IS_ERR_OR_NULL(minfo->pb_event)) {
		/* retry by ION */
		minfo->pb_event = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, EVENT_REGION_SIZE + 0x10, NULL, 0);
		if (IS_ERR_OR_NULL(minfo->pb_event)) {
			err("failed to allocate buffer for EVENT_REGION");
			return -ENOMEM;
		}
	}
	minfo->total_size += minfo->pb_event->size;

	/* data region */
	minfo->pb_dregion = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, DATA_REGION_SIZE, NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_dregion)) {
		err("failed to allocate buffer for DATA_REGION");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_dregion->size;

	/* parameter region */
	minfo->pb_pregion = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx,
						(FIMC_IS_STREAM_COUNT * PARAM_REGION_SIZE), NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_pregion)) {
		err("failed to allocate buffer for PARAM_REGION");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_pregion->size;

	/* fshared data region */
	minfo->pb_fshared = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, FSHARED_REGION_SIZE, NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_fshared)) {
		err("failed to allocate buffer for FSHARED_REGION");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_fshared->size;

#if !defined(ENABLE_DYNAMIC_MEM)
	/* 3aa/isp internal DMA buffer */
	minfo->pb_taaisp = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx,
				TAAISP_DMA_SIZE, NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_taaisp)) {
		err("failed to allocate buffer for TAAISP_DMA");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_taaisp->size;

	info("[RSC] TAAISP_DMA memory size (aligned) : %08lx\n", TAAISP_DMA_SIZE);

	/* ME/DRC buffer */
#if (MEDRC_DMA_SIZE > 0)
	minfo->pb_medrc = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx,
				MEDRC_DMA_SIZE, NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_medrc)) {
		err("failed to allocate buffer for ME_DRC");
		return -ENOMEM;
	}

	info("[RSC] ME_DRC memory size (aligned) : %08lx\n", MEDRC_DMA_SIZE);

	minfo->total_size += minfo->pb_medrc->size;
#endif

#if defined(ENABLE_TNR)
	/* TNR internal DMA buffer */
	minfo->pb_tnr = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, tnr_size, NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_tnr)) {
		err("failed to allocate buffer for TNR DMA");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_tnr->size;

	info("[RSC] TNR_DMA memory size: %08lx\n", tnr_size);
#endif
#endif

#if defined (ENABLE_FD_SW)
	minfo->pb_lhfd = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, LHFD_MAP_SIZE, NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_lhfd)) {
		err("failed to allocate buffer for LHFD_MAP");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_lhfd->size;
#endif

#if defined (ENABLE_VRA)
	minfo->pb_vra = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, VRA_DMA_SIZE, NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_vra)) {
		err("failed to allocate buffer for VRA");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_vra->size;
#endif

#if defined (ENABLE_DNR_IN_MCSC)
	minfo->pb_mcsc_dnr = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, MCSC_DNR_DMA_SIZE, NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_mcsc_dnr)) {
		err("failed to allocate buffer for MCSC DNR");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_mcsc_dnr->size;
#endif

#if defined (ENABLE_ODC)
	tpu_size += (SIZE_ODC_INTERNAL_BUF * NUM_ODC_INTERNAL_BUF);
#endif
#if defined (ENABLE_VDIS)
	tpu_size += (SIZE_DIS_INTERNAL_BUF * NUM_DIS_INTERNAL_BUF);
#endif
#if defined (ENABLE_DNR_IN_TPU)
	tpu_size += (SIZE_DNR_INTERNAL_BUF * NUM_DNR_INTERNAL_BUF);
#endif
	if (tpu_size > 0) {
		minfo->pb_tpu = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, tpu_size, NULL, 0);
		if (IS_ERR_OR_NULL(minfo->pb_tpu)) {
			err("failed to allocate buffer for TPU");
			return -ENOMEM;
		}
		minfo->total_size += minfo->pb_tpu->size;
	}

	probe_info("[RSC] Internal memory size (aligned) : %08lx\n", minfo->total_size);

	return 0;
}

static int fimc_is_resourcemgr_initmem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_minfo *minfo = NULL;
	int ret = 0;
	int i;

	probe_info("fimc_is_init_mem - ION\n");

	ret = fimc_is_resourcemgr_allocmem(resourcemgr);
	if (ret) {
		err("Couldn't alloc for FIMC-IS\n");
		ret = -ENOMEM;
		goto p_err;
	}

	minfo = &resourcemgr->minfo;
	/* set information */
	resourcemgr->minfo.dvaddr = 0;
	resourcemgr->minfo.kvaddr = 0;

	resourcemgr->minfo.dvaddr_debug = CALL_BUFOP(minfo->pb_debug, dvaddr, minfo->pb_debug);
	resourcemgr->minfo.kvaddr_debug = CALL_BUFOP(minfo->pb_debug, kvaddr, minfo->pb_debug);
	resourcemgr->minfo.phaddr_debug = CALL_BUFOP(minfo->pb_debug, phaddr, minfo->pb_debug);

	resourcemgr->minfo.dvaddr_event = CALL_BUFOP(minfo->pb_event, dvaddr, minfo->pb_event);
	resourcemgr->minfo.kvaddr_event = CALL_BUFOP(minfo->pb_event, kvaddr, minfo->pb_event);
	resourcemgr->minfo.phaddr_event = CALL_BUFOP(minfo->pb_event, phaddr, minfo->pb_event);

	resourcemgr->minfo.dvaddr_fshared = CALL_BUFOP(minfo->pb_fshared, dvaddr, minfo->pb_fshared);
	resourcemgr->minfo.kvaddr_fshared = CALL_BUFOP(minfo->pb_fshared, kvaddr, minfo->pb_fshared);

	resourcemgr->minfo.dvaddr_region = CALL_BUFOP(minfo->pb_pregion, dvaddr, minfo->pb_pregion);
	resourcemgr->minfo.kvaddr_region = CALL_BUFOP(minfo->pb_pregion, kvaddr, minfo->pb_pregion);

#if defined(ENABLE_FD_SW)
	resourcemgr->minfo.dvaddr_lhfd = CALL_BUFOP(minfo->pb_lhfd, dvaddr, minfo->pb_lhfd);
	resourcemgr->minfo.kvaddr_lhfd = CALL_BUFOP(minfo->pb_lhfd, kvaddr, minfo->pb_lhfd);
#else
	resourcemgr->minfo.dvaddr_lhfd = 0;
	resourcemgr->minfo.kvaddr_lhfd = 0;
#endif

#if defined(ENABLE_VRA)
	resourcemgr->minfo.dvaddr_vra = CALL_BUFOP(minfo->pb_vra, dvaddr, minfo->pb_vra);
	resourcemgr->minfo.kvaddr_vra = CALL_BUFOP(minfo->pb_vra, kvaddr, minfo->pb_vra);
#else
	resourcemgr->minfo.dvaddr_vra = 0;
	resourcemgr->minfo.kvaddr_vra = 0;
#endif

#if defined(ENABLE_DNR_IN_MCSC)
	resourcemgr->minfo.dvaddr_mcsc_dnr = CALL_BUFOP(minfo->pb_mcsc_dnr, dvaddr, minfo->pb_mcsc_dnr);
	resourcemgr->minfo.kvaddr_mcsc_dnr = CALL_BUFOP(minfo->pb_mcsc_dnr, kvaddr, minfo->pb_mcsc_dnr);
#else
	resourcemgr->minfo.dvaddr_mcsc_dnr = 0;
	resourcemgr->minfo.kvaddr_mcsc_dnr = 0;
#endif

#if defined(ENABLE_ODC) || defined(ENABLE_VDIS) || defined(ENABLE_DNR_IN_TPU)
	resourcemgr->minfo.dvaddr_tpu = CALL_BUFOP(minfo->pb_tpu, dvaddr, minfo->pb_tpu);
	resourcemgr->minfo.kvaddr_tpu = CALL_BUFOP(minfo->pb_tpu, kvaddr, minfo->pb_tpu);
#else
	resourcemgr->minfo.dvaddr_tpu = 0;
	resourcemgr->minfo.kvaddr_tpu = 0;
#endif
	resourcemgr->minfo.kvaddr_debug_cnt =  resourcemgr->minfo.kvaddr_debug
						+ DEBUG_REGION_SIZE;
	resourcemgr->minfo.kvaddr_event_cnt =  resourcemgr->minfo.kvaddr_event
						+ EVENT_REGION_SIZE;
	resourcemgr->minfo.kvaddr_setfile = CALL_BUFOP(minfo->pb_setfile, kvaddr, minfo->pb_setfile);

	for (i = 0; i < SENSOR_POSITION_MAX; i++)
		resourcemgr->minfo.kvaddr_cal[i] =
			CALL_BUFOP(minfo->pb_cal[i], kvaddr, minfo->pb_cal[i]);

	probe_info("[RSC] Kernel virtual for library: %08lx\n", resourcemgr->minfo.kvaddr);
	probe_info("[RSC] Kernel virtual for debug: %08lx\n", resourcemgr->minfo.kvaddr_debug);
	probe_info("[RSC] fimc_is_init_mem done\n");
p_err:
	return ret;
}

#ifdef ENABLE_DYNAMIC_MEM
static int fimc_is_resourcemgr_alloc_dynamic_mem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_mem *mem = &resourcemgr->mem;
	struct fimc_is_minfo *minfo = &resourcemgr->minfo;
#if defined (ENABLE_TNR)
	size_t tnr_size = ((SIZE_TNR_IMAGE_BUF + SIZE_TNR_WEIGHT_BUF) * NUM_OF_TNR_BUF);
#endif

	/* 3aa/isp internal DMA buffer */
	minfo->pb_taaisp = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx,
				TAAISP_DMA_SIZE, NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_taaisp)) {
		err("failed to allocate buffer for TAAISP_DMA memory");
		return -ENOMEM;
	}

	info("[RSC] TAAISP_DMA memory size (aligned) : %08lx\n", TAAISP_DMA_SIZE);

	/* ME/DRC buffer */
#if (MEDRC_DMA_SIZE > 0)
	minfo->pb_medrc = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx,
				MEDRC_DMA_SIZE, NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_medrc)) {
		CALL_VOID_BUFOP(minfo->pb_taaisp, free, minfo->pb_taaisp);
		err("failed to allocate buffer for ME_DRC");
		return -ENOMEM;
	}

	info("[RSC] ME_DRC memory size (aligned) : %08lx\n", MEDRC_DMA_SIZE);
#endif

#if defined(ENABLE_TNR)
	/* TNR internal DMA buffer */
	minfo->pb_tnr = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, tnr_size, NULL, 0);
	if (IS_ERR_OR_NULL(minfo->pb_tnr)) {
		CALL_VOID_BUFOP(minfo->pb_taaisp, free, minfo->pb_taaisp);
#if (MEDRC_DMA_SIZE > 0)
			CALL_VOID_BUFOP(minfo->pb_medrc, free, minfo->pb_medrc);
#endif
		err("failed to allocate buffer for TNR DMA");
		return -ENOMEM;
	}

	info("[RSC] TNR_DMA memory size: %08lx\n", tnr_size);
#endif

	return 0;
}

static int fimc_is_resourcemgr_init_dynamic_mem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_minfo *minfo = &resourcemgr->minfo;
	int ret = 0;
	unsigned long kva;
	dma_addr_t dva;

	probe_info("fimc_is_init_mem - ION\n");

	ret = fimc_is_resourcemgr_alloc_dynamic_mem(resourcemgr);
	if (ret) {
		err("Couldn't alloc for FIMC-IS\n");
		ret = -ENOMEM;
		goto p_err;
	}

	kva = CALL_BUFOP(minfo->pb_taaisp, kvaddr, minfo->pb_taaisp);
	dva = CALL_BUFOP(minfo->pb_taaisp, dvaddr, minfo->pb_taaisp);
	info("[RSC] TAAISP_DMA memory kva:0x%1x, dva: %pad\n", kva, &dva);

#if (MEDRC_DMA_SIZE > 0)
	kva = CALL_BUFOP(minfo->pb_medrc, kvaddr, minfo->pb_medrc);
	dva = CALL_BUFOP(minfo->pb_medrc, dvaddr, minfo->pb_medrc);
	info("[RSC] ME_DRC memory kva:0x%1x, dva: %pad\n", kva, &dva);
#endif

#if defined(ENABLE_TNR)
	kva = CALL_BUFOP(minfo->pb_tnr, kvaddr, minfo->pb_tnr);
	dva = CALL_BUFOP(minfo->pb_tnr, dvaddr, minfo->pb_tnr);
	info("[RSC] TNR_DMA memory kva:0x%1x, dva: %pad\n", kva, &dva);
#endif

	info("[RSC] %s done\n", __func__);

p_err:
	return ret;
}

static int fimc_is_resourcemgr_deinit_dynamic_mem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_minfo *minfo = &resourcemgr->minfo;

#if defined(ENABLE_TNR)
	CALL_VOID_BUFOP(minfo->pb_tnr, free, minfo->pb_tnr);
#endif
#if (MEDRC_DMA_SIZE > 0)
		CALL_VOID_BUFOP(minfo->pb_medrc, free, minfo->pb_medrc);
#endif
	CALL_VOID_BUFOP(minfo->pb_taaisp, free, minfo->pb_taaisp);

	return 0;
}
#endif /* #ifdef ENABLE_DYNAMIC_MEM */

#if defined(SECURE_CAMERA_FACE)
static int fimc_is_resourcemgr_alloc_secure_mem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_mem *mem = &resourcemgr->mem;
	struct fimc_is_minfo *minfo = &resourcemgr->minfo;

	/* 3aa/isp internal DMA buffer */
	minfo->pb_taaisp_s = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx,
				TAAISP_DMA_SIZE, "camera_heap",
				ION_FLAG_CACHED | ION_FLAG_PROTECTED);
	if (IS_ERR_OR_NULL(minfo->pb_taaisp_s)) {
		err("failed to allocate buffer for TAAISP_DMA_S");
		return -ENOMEM;
	}

	info("[RSC] TAAISP_DMA_S memory size (aligned) : %08lx\n", TAAISP_DMA_SIZE);

	/* ME/DRC buffer */
#if (MEDRC_DMA_SIZE > 0)
	minfo->pb_medrc_s = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx,
				MEDRC_DMA_SIZE, "secure_camera_heap",
				ION_FLAG_CACHED | ION_FLAG_PROTECTED);
	if (IS_ERR_OR_NULL(minfo->pb_medrc_s)) {
		CALL_VOID_BUFOP(minfo->pb_taaisp_s, free, minfo->pb_taaisp_s);
		err("failed to allocate buffer for ME_DRC_S");
		return -ENOMEM;
	}

	info("[RSC] ME_DRC_S memory size (aligned) : %08lx\n", MEDRC_DMA_SIZE);
#endif

	return 0;
}

static int fimc_is_resourcemgr_init_secure_mem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_core *core;
	int ret = 0;

	core = container_of(resourcemgr, struct fimc_is_core, resourcemgr);
	FIMC_BUG(!core);

	if (core->scenario != FIMC_IS_SCENARIO_SECURE)
		return ret;

	info("fimc_is_init_secure_mem - ION\n");

	ret = fimc_is_resourcemgr_alloc_secure_mem(resourcemgr);
	if (ret) {
		err("Couldn't alloc for FIMC-IS\n");
		ret = -ENOMEM;
		goto p_err;
	}

	info("[RSC] %s done\n", __func__);
p_err:
	return ret;
}

static int fimc_is_resourcemgr_deinit_secure_mem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_minfo *minfo = &resourcemgr->minfo;
	struct fimc_is_core *core;
	int ret = 0;

	core = container_of(resourcemgr, struct fimc_is_core, resourcemgr);
	FIMC_BUG(!core);

	if (minfo->pb_taaisp_s)
		CALL_VOID_BUFOP(minfo->pb_taaisp_s, free, minfo->pb_taaisp_s);
	if (minfo->pb_medrc_s)
		CALL_VOID_BUFOP(minfo->pb_medrc_s, free, minfo->pb_medrc_s);

	minfo->pb_taaisp_s = NULL;
	minfo->pb_medrc_s = NULL;

	info("[RSC] %s done\n", __func__);

	return ret;
}
#endif

static struct vm_struct fimc_is_lib_vm;
static struct vm_struct fimc_is_heap_vm;
static struct vm_struct fimc_is_heap_rta_vm;
#ifdef CONFIG_UH_RKP
static struct vm_struct fimc_lib_vm_for_rkp;
#endif
#if defined(RESERVED_MEM_IN_DT)
static int fimc_is_rmem_device_init(struct reserved_mem *rmem,
				    struct device *dev)
{
	WARN(1, "%s() should never be called!", __func__);
	return 0;
}

static void fimc_is_rmem_device_release(struct reserved_mem *rmem,
					struct device *dev)
{
}

static const struct reserved_mem_ops fimc_is_rmem_ops = {
	.device_init	= fimc_is_rmem_device_init,
	.device_release	= fimc_is_rmem_device_release,
};

static int __init fimc_is_reserved_mem_setup(struct reserved_mem *rmem)
{
	const __be32 *prop;
	u64 kbase;
	int len;
#ifdef CONFIG_UH_RKP
	u64 roundup_end_addr;
	rkp_dynamic_load_t rkp_dyn;
	int ret;
#endif

	prop = of_get_flat_dt_prop(rmem->fdt_node, "kernel_virt", &len);
	if (!prop) {
		pr_err("kernel_virt is not found in '%s' node\n", rmem->name);
		return -EINVAL;
	}

	if (len != dt_root_addr_cells * sizeof(__be32)) {
		pr_err("invalid kernel_virt property in '%s' node.\n",
			rmem->name);
		return -EINVAL;
	}

	kbase = dt_mem_next_cell(dt_root_addr_cells, &prop);

	fimc_is_lib_vm.phys_addr = rmem->base;
	fimc_is_lib_vm.addr = (void *)kbase;
	fimc_is_lib_vm.size = LIB_SIZE + PAGE_SIZE;

	BUG_ON(rmem->size < LIB_SIZE);

#ifdef CONFIG_UH_RKP
	memcpy(&fimc_lib_vm_for_rkp, &fimc_is_lib_vm, sizeof(struct vm_struct));

	roundup_end_addr = (u64)roundup((u64)fimc_is_lib_vm.addr + (u64)fimc_is_lib_vm.size, SECTION_SIZE);
	fimc_lib_vm_for_rkp.addr = (void *)rounddown((u64)fimc_lib_vm_for_rkp.addr, SECTION_SIZE);
	fimc_lib_vm_for_rkp.size = roundup_end_addr - (u64)fimc_lib_vm_for_rkp.addr;

	rkp_dyn.binary_base = fimc_is_lib_vm.phys_addr;
	rkp_dyn.binary_size = LIB_SIZE;
	rkp_dyn.type = RKP_DYN_COMMAND_BREAKDOWN_BEFORE_INIT;
	ret = uh_call(UH_APP_RKP, RKP_DYNAMIC_LOAD, RKP_DYN_COMMAND_BREAKDOWN_BEFORE_INIT, (u64)&rkp_dyn, 0, 0);
	if (ret) {
		err_lib("fail to break-before-init FIMC in EL2");
	}
	vm_area_add_early(&fimc_lib_vm_for_rkp);
#else
	vm_area_add_early(&fimc_is_lib_vm);
#endif

	probe_info("fimc-is library memory: 0x%llx\n", kbase);

	fimc_is_heap_vm.addr = (void *)HEAP_START;
	fimc_is_heap_vm.size = HEAP_SIZE + PAGE_SIZE;

	vm_area_add_early(&fimc_is_heap_vm);

	probe_info("fimc-is heap memory: 0x%lx\n", HEAP_START);

	rmem->ops = &fimc_is_rmem_ops;

	return 0;
}
RESERVEDMEM_OF_DECLARE(fimc_is_lib, "exynos,fimc_is_lib", fimc_is_reserved_mem_setup);
#else
static int __init fimc_is_lib_mem_alloc(char *str)
{
	ulong addr = 0;
#ifdef CONFIG_UH_RKP
	u64 roundup_end_addr;
	rkp_dynamic_load_t rkp_dyn;
	int ret;
#endif

	if (kstrtoul(str, 0, (ulong *)&addr) || !addr) {
		probe_warn("invalid fimc-is library memory address, use default");
		addr = __LIB_START;
	}

	if (addr != __LIB_START)
		probe_warn("use different address [reserve-fimc=0x%lx default:0x%lx]",
				addr, __LIB_START);

	fimc_is_lib_vm.phys_addr = memblock_alloc(LIB_SIZE, SZ_2M);
	fimc_is_lib_vm.addr = (void *)addr;
	fimc_is_lib_vm.size = LIB_SIZE + PAGE_SIZE;
#ifdef CONFIG_UH_RKP
	memcpy(&fimc_lib_vm_for_rkp, &fimc_is_lib_vm, sizeof(struct vm_struct));

	roundup_end_addr = (u64)roundup((u64)fimc_is_lib_vm.addr + (u64)fimc_is_lib_vm.size, SECTION_SIZE);
	fimc_lib_vm_for_rkp.addr = (void *)rounddown((u64)fimc_lib_vm_for_rkp.addr, SECTION_SIZE);
	fimc_lib_vm_for_rkp.size = roundup_end_addr - (u64)fimc_lib_vm_for_rkp.addr;

	rkp_dyn.binary_base = fimc_is_lib_vm.phys_addr;
	rkp_dyn.binary_size = LIB_SIZE;
	rkp_dyn.type = RKP_DYN_COMMAND_BREAKDOWN_BEFORE_INIT;
	ret = uh_call(UH_APP_RKP, RKP_DYNAMIC_LOAD, RKP_DYN_COMMAND_BREAKDOWN_BEFORE_INIT, (u64)&rkp_dyn, 0, 0);
	if (ret) {
		err_lib("fail to break-before-init FIMC in EL2");
	}
	vm_area_add_early(&fimc_lib_vm_for_rkp);
#else
	vm_area_add_early(&fimc_is_lib_vm);
#endif

	probe_info("fimc-is library memory: 0x%lx\n", addr);

	fimc_is_heap_vm.addr = (void *)HEAP_START;
	fimc_is_heap_vm.size = HEAP_SIZE + PAGE_SIZE;

	vm_area_add_early(&fimc_is_heap_vm);

	probe_info("fimc-is heap DDK memory: 0x%lx\n", HEAP_START);

	fimc_is_heap_rta_vm.addr = (void *)HEAP_RTA_START;
	fimc_is_heap_rta_vm.size = HEAP_RTA_SIZE + PAGE_SIZE;

	vm_area_add_early(&fimc_is_heap_rta_vm);

	probe_info("fimc-is heap RTA memory: 0x%lx\n", HEAP_RTA_START);

	return 0;
}
__setup("reserve-fimc=", fimc_is_lib_mem_alloc);
#endif

static int __init fimc_is_lib_mem_map(void)
{
	int page_size, i;
	struct page *page;
	struct page **pages;
	pgprot_t prot;

#ifdef CONFIG_UH_RKP
	prot = PAGE_KERNEL_RKP_RO;
#else
	prot = PAGE_KERNEL;
#endif

	if (!fimc_is_lib_vm.phys_addr) {
		probe_err("There is no reserve-fimc= at bootargs.");
		return -ENOMEM;
	}

	page_size = fimc_is_lib_vm.size / PAGE_SIZE;
	pages = kzalloc(sizeof(struct page*) * page_size, GFP_KERNEL);
	page = phys_to_page(fimc_is_lib_vm.phys_addr);

	for (i = 0; i < page_size; i++)
		pages[i] = page++;

	if (map_vm_area(&fimc_is_lib_vm, prot, pages)) {
		probe_err("failed to mapping between virt and phys for binary");
		vunmap(fimc_is_lib_vm.addr);
		kfree(pages);
		return -ENOMEM;
	}

	kfree(pages);

	return 0;
}

static int __init fimc_is_heap_mem_map(struct fimc_is_resourcemgr *resourcemgr,
	struct vm_struct *vm, int heap_size)
{
	struct fimc_is_mem *mem = &resourcemgr->mem;
	struct fimc_is_priv_buf *pb;
	struct scatterlist *sg;
	struct sg_table *table;
	int i, j;
	int npages = vm->size / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;

	pb = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, heap_size, NULL, 0);
	if (IS_ERR_OR_NULL(pb)) {
		err("failed to allocate buffer for HEAP");
		vfree(pages);
		return -ENOMEM;
	}

	table = pb->sgt;

	for_each_sg(table->sgl, sg, table->nents, i) {
		int npages_this_entry = PAGE_ALIGN(sg->length) / PAGE_SIZE;
		struct page *page = sg_page(sg);

		BUG_ON(i >= npages);

		for (j = 0; j < npages_this_entry; j++)
			*(tmp++) = page++;
	}

	if (map_vm_area(vm, PAGE_KERNEL, pages)) {
		probe_err("failed to mapping between virt and phys for binary");
		vunmap(vm->addr);
		vfree(pages);
		CALL_VOID_BUFOP(pb, free, pb);
		return -ENOMEM;
	}

	vfree(pages);

	return 0;
}
#endif /* #ifdef ENABLE_IS_CORE */

static int fimc_is_tmu_notifier(struct notifier_block *nb,
	unsigned long state, void *data)
{
#ifdef CONFIG_EXYNOS_THERMAL
	int ret = 0, fps = 0;
	struct fimc_is_resourcemgr *resourcemgr;
#if IS_ENABLED(CONFIG_EXYNOS_SNAPSHOT_THERMAL) || IS_ENABLED(CONFIG_DEBUG_SNAPSHOT_THERMAL)
	char *cooling_device_name = "ISP";
#endif
	resourcemgr = container_of(nb, struct fimc_is_resourcemgr, tmu_notifier);

	switch (state) {
	case ISP_NORMAL:
		resourcemgr->tmu_state = ISP_NORMAL;
		resourcemgr->limited_fps = 0;
		break;
	case ISP_COLD:
		resourcemgr->tmu_state = ISP_COLD;
		resourcemgr->limited_fps = 0;
		break;
	case ISP_THROTTLING:
		resourcemgr->tmu_state = ISP_THROTTLING;
		fps = isp_cooling_get_fps(0, *(unsigned long *)data);

		/* The FPS can be defined to any specific value. */
		if (fps >= 60) {
			resourcemgr->limited_fps = 0;
			warn("[RSC] THROTTLING : Unlimited FPS");
		} else {
			resourcemgr->limited_fps = fps;
			warn("[RSC] THROTTLING : Limited %dFPS", fps);
		}
		break;
	case ISP_TRIPPING:
		resourcemgr->tmu_state = ISP_TRIPPING;
		resourcemgr->limited_fps = 5;
		warn("[RSC] TRIPPING : Limited 5FPS");
		break;
	default:
		err("[RSC] invalid tmu state(%ld)", state);
		break;
	}

#if IS_ENABLED(CONFIG_EXYNOS_SNAPSHOT_THERMAL)
	exynos_ss_thermal(NULL, 0, cooling_device_name, resourcemgr->limited_fps);
#elif IS_ENABLED(CONFIG_DEBUG_SNAPSHOT_THERMAL)
	dbg_snapshot_thermal(NULL, 0, cooling_device_name, resourcemgr->limited_fps);
#endif

	return ret;
#else

	return 0;
#endif
}

#ifdef CONFIG_EXYNOS_BUSMONITOR
static int due_to_fimc_is(const char *desc)
{
	if (desc && (strstr((char *)desc, "CAM")
				|| strstr((char *)desc, "ISP")))
			return 1;

	return 0;
}

static int fimc_is_bm_notifier(struct notifier_block *nb,
	unsigned long state, void *data)
{
	int i;
	struct fimc_is_core *core;
	struct fimc_is_resourcemgr *resourcemgr;
	struct busmon_notifier *busmon;

	resourcemgr = container_of(nb, struct fimc_is_resourcemgr, bm_notifier);
	core = container_of(resourcemgr, struct fimc_is_core, resourcemgr);
	busmon = (struct busmon_notifier *)data;

	if (!busmon)
		return 0;

	if (due_to_fimc_is(busmon->init_desc)
			|| due_to_fimc_is(busmon->target_desc)
			|| due_to_fimc_is(busmon->masterip_desc)) {
		info("1. NOC info.\n");
		info("%s: init description : %s\n", __func__, busmon->init_desc);
		info("%s: target descrition: %s\n", __func__, busmon->target_desc);
		info("%s: user description : %s\n", __func__, busmon->masterip_desc);
		info("%s: user id          : %u\n", __func__, busmon->masterip_idx);
		info("%s: target address   : %lx\n",__func__, busmon->target_addr);

		for (i = 0; i < FIMC_IS_STREAM_COUNT; ++i) {
			if (!test_bit(FIMC_IS_ISCHAIN_POWER_ON, &core->ischain[i].state))
				continue;

			info("2. FW log dump\n");
			fimc_is_hw_logdump(&core->interface);

			info("3. Clock info.\n");
			schedule_work(&core->wq_data_print_clk);
		}
	}

	return 0;
}
#endif /* CONFIG_EXYNOS_BUSMONITOR */

#ifdef ENABLE_FW_SHARE_DUMP
static int fimc_is_fw_share_dump(void)
{
	int ret = 0;
	u8 *buf;
	struct fimc_is_core *core = NULL;
	struct fimc_is_resourcemgr *resourcemgr;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core)
		goto p_err;

	resourcemgr = &core->resourcemgr;
	buf = (u8 *)resourcemgr->fw_share_dump_buf;

	/* dump share region in fw area */
	if (IS_ERR_OR_NULL(buf)) {
		err("%s: fail to alloc", __func__);
		ret = -ENOMEM;
		goto p_err;
	}

	/* sync with fw for memory */
	vb2_ion_sync_for_cpu(resourcemgr->minfo.fw_cookie, 0,
			SHARED_OFFSET, DMA_BIDIRECTIONAL);

	memcpy(buf, (u8 *)resourcemgr->minfo.kvaddr_fshared, SHARED_SIZE);

	info("%s: dumped ramdump addr(virt/phys/size): (%p/%p/0x%X)", __func__, buf,
			(void *)virt_to_phys(buf), SHARED_SIZE);
p_err:
	return ret;
}
#endif

#ifdef ENABLE_KERNEL_LOG_DUMP
int fimc_is_kernel_log_dump(bool overwrite)
{
	static int dumped = 0;
	struct fimc_is_core *core;
	struct fimc_is_resourcemgr *resourcemgr;
	void *log_kernel = NULL;
	unsigned long long when;
	unsigned long usec;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core)
		return -EINVAL;

	resourcemgr = &core->resourcemgr;

	if (dumped && !overwrite) {
		when = resourcemgr->kernel_log_time;
		usec = do_div(when, NSEC_PER_SEC) / NSEC_PER_USEC;
		info("kernel log was saved already at [%5lu.%06lu]\n",
				(unsigned long)when, usec);

		return -ENOSPC;
	}

#if IS_ENABLED(CONFIG_EXYNOS_SNAPSHOT)
	log_kernel = (void *)exynos_ss_get_item_vaddr("log_kernel");
#elif IS_ENABLED(CONFIG_DEBUG_SNAPSHOT)
	log_kernel = (void *)dbg_snapshot_get_item_vaddr("log_kernel");
#endif
	if (!log_kernel)
		return -EINVAL;

	if (resourcemgr->kernel_log_buf) {
		resourcemgr->kernel_log_time = local_clock();

		info("kernel log saved to %p(%p) from %p\n",
				resourcemgr->kernel_log_buf,
				(void *)virt_to_phys(resourcemgr->kernel_log_buf),
				log_kernel);
#if IS_ENABLED(CONFIG_EXYNOS_SNAPSHOT)
		memcpy(resourcemgr->kernel_log_buf, log_kernel,
				exynos_ss_get_item_size("log_kernel"));
#elif IS_ENABLED(CONFIG_DEBUG_SNAPSHOT)
		memcpy(resourcemgr->kernel_log_buf, log_kernel,
				dbg_snapshot_get_item_size("log_kernel"));
#endif

		dumped = 1;
	}

	return 0;
}
#endif

int fimc_is_resource_dump(void)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_group *group;
	struct fimc_is_subdev *subdev;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_device_ischain *device = NULL;
	struct fimc_is_device_csi *csi;
	int i, j, vc;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core)
		goto exit;

	info("### %s dump start ###\n", __func__);

	groupmgr = &core->groupmgr;

	/* dump per core */
	for (i = 0; i < FIMC_IS_STREAM_COUNT; ++i) {
		device = &core->ischain[i];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN_STREAM, &device->state))
			continue;

		if (test_bit(FIMC_IS_ISCHAIN_CLOSING, &device->state))
			continue;

		/* clock & gpio dump */
		schedule_work(&core->wq_data_print_clk);
#ifdef ENABLE_IS_CORE
		/* fw log, mcuctl dump */
		fimc_is_hw_logdump(&core->interface);
		fimc_is_hw_regdump(&core->interface);
#else
		/* ddk log dump */
		fimc_is_lib_logdump();
		fimc_is_hardware_clk_gate_dump(&core->hardware);
		fimc_is_hardware_sfr_dump(&core->hardware, HW_SLOT_MAX, false);
#endif
		break;
	}

	/* dump per ischain */
	for (i = 0; i < FIMC_IS_STREAM_COUNT; ++i) {
		device = &core->ischain[i];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN_STREAM, &device->state))
			continue;

		if (test_bit(FIMC_IS_ISCHAIN_CLOSING, &device->state))
			continue;

		if (device->sensor && !test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) {
			csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(device->sensor->subdev_csi);
			if (csi) {
				csi_hw_dump(csi->base_reg);
				csi_hw_phy_dump(csi->phy_reg, csi->instance);
				for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++) {
					csi_hw_vcdma_dump(csi->vc_reg[csi->scm][vc]);
					csi_hw_vcdma_cmn_dump(csi->cmn_reg[csi->scm][vc]);
				}
				csi_hw_common_dma_dump(csi->csi_dma->base_reg);
#if defined(ENABLE_PDP_STAT_DMA)
				csi_hw_common_dma_dump(csi->csi_dma->base_reg_stat);
#endif
			}
		}

		/* dump all framemgr */
		group = groupmgr->leader[i];
		while (group) {
			if (!test_bit(FIMC_IS_GROUP_OPEN, &group->state))
				break;

			for (j = 0; j < ENTRY_END; j++) {
				subdev = group->subdev[j];
				if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
					framemgr = GET_SUBDEV_FRAMEMGR(subdev);
					if (framemgr) {
						unsigned long flags;

						mserr(" dump framemgr..", subdev, subdev);
						framemgr_e_barrier_irqs(framemgr, 0, flags);
						frame_manager_print_queues(framemgr);
						framemgr_x_barrier_irqr(framemgr, 0, flags);
					}
				}
			}

			group = group->next;
		}
	}

	info("### %s dump end ###\n", __func__);

exit:
	return 0;
}

#ifdef ENABLE_PANIC_HANDLER
static int fimc_is_panic_handler(struct notifier_block *nb, ulong l,
	void *buf)
{
#if !defined(ENABLE_IS_CORE)
	fimc_is_resource_dump();
#endif
#ifdef ENABLE_FW_SHARE_DUMP
	/* dump share area in fw region */
	fimc_is_fw_share_dump();
#endif
	return 0;
}

static struct notifier_block notify_panic_block = {
	.notifier_call = fimc_is_panic_handler,
};
#endif

#if defined(ENABLE_REBOOT_HANDLER) && !defined(ENABLE_IS_CORE)
static int fimc_is_reboot_handler(struct notifier_block *nb, ulong l,
	void *buf)
{
	struct fimc_is_core *core = NULL;

	info("%s:++\n", __func__);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core)
		goto exit;

	fimc_is_cleanup(core);
exit:
	info("%s:--\n", __func__);
	return 0;
}

static struct notifier_block notify_reboot_block = {
	.notifier_call = fimc_is_reboot_handler,
};
#endif

int fimc_is_resourcemgr_probe(struct fimc_is_resourcemgr *resourcemgr,
	void *private_data, struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np;

	FIMC_BUG(!resourcemgr);
	FIMC_BUG(!private_data);

	np = pdev->dev.of_node;
	resourcemgr->private_data = private_data;

	clear_bit(FIMC_IS_RM_COM_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_SS0_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_SS1_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_SS2_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_SS3_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_SS4_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_SS5_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_ISC_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_POWER_ON, &resourcemgr->state);
	atomic_set(&resourcemgr->rsccount, 0);
	atomic_set(&resourcemgr->qos_refcount, 0);
	atomic_set(&resourcemgr->resource_sensor0.rsccount, 0);
	atomic_set(&resourcemgr->resource_sensor1.rsccount, 0);
	atomic_set(&resourcemgr->resource_sensor2.rsccount, 0);
	atomic_set(&resourcemgr->resource_sensor3.rsccount, 0);
	atomic_set(&resourcemgr->resource_ischain.rsccount, 0);
	atomic_set(&resourcemgr->resource_preproc.rsccount, 0);

	resourcemgr->cluster0 = 0;
	resourcemgr->cluster1 = 0;
	resourcemgr->cluster2 = 0;
	resourcemgr->hal_version = IS_HAL_VER_1_0;

	/* rsc mutex init */
	mutex_init(&resourcemgr->rsc_lock);
	mutex_init(&resourcemgr->sysreg_lock);

	/* temperature monitor unit */
	resourcemgr->tmu_notifier.notifier_call = fimc_is_tmu_notifier;
	resourcemgr->tmu_notifier.priority = 0;
	resourcemgr->tmu_state = ISP_NORMAL;
	resourcemgr->limited_fps = 0;

	resourcemgr->streaming_cnt = 0;

	/* bus monitor unit */

#ifdef CONFIG_EXYNOS_BUSMONITOR
	resourcemgr->bm_notifier.notifier_call = fimc_is_bm_notifier;
	resourcemgr->bm_notifier.priority = 0;

	busmon_notifier_chain_register(&resourcemgr->bm_notifier);
#endif

	ret = exynos_tmu_isp_add_notifier(&resourcemgr->tmu_notifier);
	if (ret) {
		probe_err("exynos_tmu_isp_add_notifier is fail(%d)", ret);
		goto p_err;
	}

#ifdef ENABLE_RESERVED_MEM
	ret = fimc_is_resourcemgr_initmem(resourcemgr);
	if (ret) {
		probe_err("fimc_is_resourcemgr_initmem is fail(%d)", ret);
		goto p_err;
	}
#endif

	ret = fimc_is_lib_mem_map();
	if (ret) {
		probe_err("fimc_is_lib_mem_map is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_heap_mem_map(resourcemgr, &fimc_is_heap_vm, HEAP_SIZE);
	if (ret) {
		probe_err("fimc_is_heap_mem_map for HEAP_DDK is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_heap_mem_map(resourcemgr, &fimc_is_heap_rta_vm, HEAP_RTA_SIZE);
	if (ret) {
		probe_err("fimc_is_heap_mem_map for HEAP_RTA is fail(%d)", ret);
		goto p_err;
	}

#ifdef ENABLE_DVFS
	/* dvfs controller init */
	ret = fimc_is_dvfs_init(resourcemgr);
	if (ret) {
		probe_err("%s: fimc_is_dvfs_init failed!\n", __func__);
		goto p_err;
	}
#endif

#ifdef ENABLE_PANIC_HANDLER
	atomic_notifier_chain_register(&panic_notifier_list, &notify_panic_block);
#endif
#if defined(ENABLE_REBOOT_HANDLER) && !defined(ENABLE_IS_CORE)
	register_reboot_notifier(&notify_reboot_block);
#endif
#ifdef ENABLE_SHARED_METADATA
	spin_lock_init(&resourcemgr->shared_meta_lock);
#endif
#ifdef ENABLE_FW_SHARE_DUMP
	/* to dump share region in fw area */
	resourcemgr->fw_share_dump_buf = (ulong)kzalloc(SHARED_SIZE, GFP_KERNEL);
#endif
#ifdef CONFIG_CMU_EWF
	get_cmuewf_index(np, &idx_ewf);
#endif

#ifdef ENABLE_KERNEL_LOG_DUMP
#if IS_ENABLED(CONFIG_EXYNOS_SNAPSHOT)
	resourcemgr->kernel_log_buf = kzalloc(exynos_ss_get_item_size("log_kernel"),
						GFP_KERNEL);
#elif IS_ENABLED(CONFIG_DEBUG_SNAPSHOT)
	resourcemgr->kernel_log_buf = kzalloc(dbg_snapshot_get_item_size("log_kernel"),
						GFP_KERNEL);
#endif
#endif

	mutex_init(&resourcemgr->global_param.lock);
	resourcemgr->global_param.video_mode = 0;
	resourcemgr->global_param.state = 0;

p_err:
	probe_info("[RSC] %s(%d)\n", __func__, ret);
	return ret;
}

int fimc_is_resource_open(struct fimc_is_resourcemgr *resourcemgr, u32 rsc_type, void **device)
{
	int ret = 0;
	u32 stream;
	void *result;
	struct fimc_is_resource *resource;
	struct fimc_is_core *core;
	struct fimc_is_device_ischain *ischain;

	FIMC_BUG(!resourcemgr);
	FIMC_BUG(!resourcemgr->private_data);
	FIMC_BUG(rsc_type >= RESOURCE_TYPE_MAX);

	result = NULL;
	core = (struct fimc_is_core *)resourcemgr->private_data;
	resource = GET_RESOURCE(resourcemgr, rsc_type);
	if (!resource) {
		err("[RSC] resource is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	switch (rsc_type) {
	case RESOURCE_TYPE_PREPROC:
		result = &core->preproc;
		resource->pdev = core->preproc.pdev;
		break;
	case RESOURCE_TYPE_SENSOR0:
		result = &core->sensor[RESOURCE_TYPE_SENSOR0];
		resource->pdev = core->sensor[RESOURCE_TYPE_SENSOR0].pdev;
		break;
	case RESOURCE_TYPE_SENSOR1:
		result = &core->sensor[RESOURCE_TYPE_SENSOR1];
		resource->pdev = core->sensor[RESOURCE_TYPE_SENSOR1].pdev;
		break;
	case RESOURCE_TYPE_SENSOR2:
		result = &core->sensor[RESOURCE_TYPE_SENSOR2];
		resource->pdev = core->sensor[RESOURCE_TYPE_SENSOR2].pdev;
		break;
	case RESOURCE_TYPE_SENSOR3:
		result = &core->sensor[RESOURCE_TYPE_SENSOR3];
		resource->pdev = core->sensor[RESOURCE_TYPE_SENSOR3].pdev;
		break;
#if (FIMC_IS_SENSOR_COUNT > 4)
	case RESOURCE_TYPE_SENSOR4:
		result = &core->sensor[RESOURCE_TYPE_SENSOR4];
		resource->pdev = core->sensor[RESOURCE_TYPE_SENSOR4].pdev;
		break;
#if (FIMC_IS_SENSOR_COUNT > 5)
	case RESOURCE_TYPE_SENSOR5:
		result = &core->sensor[RESOURCE_TYPE_SENSOR5];
		resource->pdev = core->sensor[RESOURCE_TYPE_SENSOR5].pdev;
		break;
#endif
#endif
	case RESOURCE_TYPE_ISCHAIN:
		for (stream = 0; stream < FIMC_IS_STREAM_COUNT; ++stream) {
			ischain = &core->ischain[stream];
			if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &ischain->state)) {
				result = ischain;
				resource->pdev = ischain->pdev;
				break;
			}
		}
		break;
	}

	if (device)
		*device = result;

p_err:
	dbgd_resource("%s\n", __func__);
	return ret;
}

int fimc_is_resource_get(struct fimc_is_resourcemgr *resourcemgr, u32 rsc_type)
{
	int ret = 0;
	u32 rsccount;
	struct fimc_is_resource *resource;
	struct fimc_is_core *core;
	int i;

	FIMC_BUG(!resourcemgr);
	FIMC_BUG(!resourcemgr->private_data);
	FIMC_BUG(rsc_type >= RESOURCE_TYPE_MAX);

	core = (struct fimc_is_core *)resourcemgr->private_data;

	mutex_lock(&resourcemgr->rsc_lock);

	rsccount = atomic_read(&core->rsccount);
	resource = GET_RESOURCE(resourcemgr, rsc_type);
	if (!resource) {
		err("[RSC] resource is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (!core->pdev) {
		err("[RSC] pdev is NULL");
		ret = -EMFILE;
		goto p_err;
	}

	if (rsccount >= (FIMC_IS_STREAM_COUNT + FIMC_IS_VIDEO_SS5_NUM)) {
		err("[RSC] Invalid rsccount(%d)", rsccount);
		ret = -EMFILE;
		goto p_err;
	}

#ifdef ENABLE_KERNEL_LOG_DUMP
	/* to secure kernel log when there was an instance that remain open */
	{
		struct fimc_is_resource *resource_ischain;
		resource_ischain = GET_RESOURCE(resourcemgr, RESOURCE_TYPE_ISCHAIN);
		if ((rsc_type != RESOURCE_TYPE_ISCHAIN)	&& rsccount == 1) {
			if (atomic_read(&resource_ischain->rsccount) == 1)
				fimc_is_kernel_log_dump(false);
		}
	}
#endif

	if (rsccount == 0) {
		TIME_LAUNCH_STR(LAUNCH_TOTAL);
		pm_stay_awake(&core->pdev->dev);

		resourcemgr->cluster0 = 0;
		resourcemgr->cluster1 = 0;
		resourcemgr->cluster2 = 0;

#ifdef CONFIG_CMU_EWF
		set_cmuewf(idx_ewf, 1);
#endif
#ifdef ENABLE_DVFS
		/* dvfs controller init */
		ret = fimc_is_dvfs_init(resourcemgr);
		if (ret) {
			err("%s: fimc_is_dvfs_init failed!\n", __func__);
			goto p_err;
		}
#endif
		/* CSIS common DMA rcount set */
		atomic_set(&core->csi_dma.rcount, 0);
#if defined(SECURE_CAMERA_FACE)
		mutex_init(&core->secure_state_lock);
		core->secure_state = FIMC_IS_STATE_UNSECURE;
		core->scenario = 0;

		info("%s: fimc-is secure state has reset\n", __func__);
#endif
		core->dual_info.mode = FIMC_IS_DUAL_MODE_NOTHING;
		core->dual_info.pre_mode = FIMC_IS_DUAL_MODE_NOTHING;
		core->dual_info.tick_count = 0;
#ifdef CONFIG_VENDER_MCD
		core->vender.opening_hint = IS_OPENING_HINT_NONE;
#endif

		for (i = 0; i < MAX_SENSOR_SHARED_RSC; i++) {
			spin_lock_init(&core->shared_rsc_slock[i]);
			atomic_set(&core->shared_rsc_count[i], 0);
		}

		for (i = 0; i < SENSOR_CONTROL_I2C_MAX; i++)
			atomic_set(&core->i2c_rsccount[i], 0);

		resourcemgr->global_param.state = 0;
		resourcemgr->shot_timeout = FIMC_IS_SHOT_TIMEOUT;
		resourcemgr->shot_timeout_tick = 0;

#ifdef ENABLE_DYNAMIC_MEM
		ret = fimc_is_resourcemgr_init_dynamic_mem(resourcemgr);
		if (ret) {
			err("fimc_is_resourcemgr_initmem is fail(%d)\n", ret);
			goto p_err;
		}
#endif

#ifdef CONFIG_EXYNOS_BCM_DBG_AUTO
		if (sec_debug_get_debug_level() != DEBUG_LEVEL_LOW) {
			exynos_bcm_dbg_start();
		}
#endif
	}

	if (atomic_read(&resource->rsccount) == 0) {
		switch (rsc_type) {
		case RESOURCE_TYPE_PREPROC:
#if defined(CONFIG_PM)
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_preproc_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_COM_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR0:
#ifdef CONFIG_PM
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_SS0_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR1:
#ifdef CONFIG_PM
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_SS1_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR2:
#ifdef CONFIG_PM
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_SS2_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR3:
#ifdef CONFIG_PM
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_SS3_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR4:
#ifdef CONFIG_PM
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_SS4_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR5:
#ifdef CONFIG_PM
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_SS5_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_ISCHAIN:
			if (test_bit(FIMC_IS_RM_POWER_ON, &resourcemgr->state)) {
				err("all resource is not power off(%lX)", resourcemgr->state);
				ret = -EINVAL;
				goto p_err;
			}

			ret = fimc_is_debug_open(&resourcemgr->minfo);
			if (ret) {
				err("fimc_is_debug_open is fail(%d)", ret);
				goto p_err;
			}

			ret = fimc_is_interface_open(&core->interface);
			if (ret) {
				err("fimc_is_interface_open is fail(%d)", ret);
				goto p_err;
			}

#if defined(SECURE_CAMERA_FACE)
			ret = fimc_is_resourcemgr_init_secure_mem(resourcemgr);
			if (ret) {
				err("fimc_is_resourcemgr_init_secure_mem is fail(%d)\n", ret);
				goto p_err;
			}
#endif

			ret = fimc_is_ischain_power(&core->ischain[0], 1);
			if (ret) {
				err("fimc_is_ischain_power is fail(%d)", ret);
				fimc_is_ischain_power(&core->ischain[0], 0);
				goto p_err;
			}

			/* W/A for a lower version MCUCTL */
			fimc_is_interface_reset(&core->interface);

#if !defined(ENABLE_IS_CORE) && defined(USE_MCUCTL)
			fimc_is_hw_s_ctrl(&core->interface, 0, HW_S_CTRL_CHAIN_IRQ, 0);
#endif

#ifdef ENABLE_CLOCK_GATE
			if (sysfs_debug.en_clk_gate &&
					sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST)
				fimc_is_clk_gate_init(core);
#endif

			set_bit(FIMC_IS_RM_ISC_POWER_ON, &resourcemgr->state);
			set_bit(FIMC_IS_RM_POWER_ON, &resourcemgr->state);

#if defined(CONFIG_SOC_EXYNOS8895)
			/* HACK for 8895, cpuidle on/off */
			info("%s: call cpuidle_pause()\n", __func__);
			cpuidle_pause();
#endif

#ifdef CONFIG_EXYNOS_BTS
			info("%s: call bts_update_scen(1)\n", __func__);
			bts_update_scen(BS_CAMERA_DEFAULT, 1);
#endif
			fimc_is_hw_ischain_qe_cfg();

			break;
		default:
			err("[RSC] resource type(%d) is invalid", rsc_type);
			BUG();
			break;
		}

#if !defined(ENABLE_IS_CORE) && !defined(DISABLE_LIB)
		if ((rsc_type == RESOURCE_TYPE_ISCHAIN)
			&& (!test_and_set_bit(FIMC_IS_BINARY_LOADED, &resourcemgr->binary_state))) {
			TIME_LAUNCH_STR(LAUNCH_DDK_LOAD);
			ret = fimc_is_load_bin();
			if (ret < 0) {
				err("fimc_is_load_bin() is fail(%d)", ret);
				clear_bit(FIMC_IS_BINARY_LOADED, &resourcemgr->binary_state);
				goto p_err;
			}
			TIME_LAUNCH_END(LAUNCH_DDK_LOAD);
		}
#endif
		fimc_is_vender_resource_get(&core->vender);
	}

	if (rsccount == 0) {
#ifdef ENABLE_HWACG_CONTROL
		/* for CSIS HWACG */
		fimc_is_hw_csi_qchannel_enable_all(true);
#endif
		/* when the first sensor device be opened */
		if (rsc_type < RESOURCE_TYPE_ISCHAIN)
			fimc_is_hw_camif_init();
	}

	atomic_inc(&resource->rsccount);
	atomic_inc(&core->rsccount);

p_err:
	mutex_unlock(&resourcemgr->rsc_lock);

	info("[RSC] rsctype: %d, rsccount: device[%d], core[%d]\n", rsc_type,
			atomic_read(&resource->rsccount), rsccount + 1);
	return ret;
}

int fimc_is_resource_put(struct fimc_is_resourcemgr *resourcemgr, u32 rsc_type)
{
	int ret = 0;
	u32 rsccount;
	struct fimc_is_resource *resource;
	struct fimc_is_core *core;

	FIMC_BUG(!resourcemgr);
	FIMC_BUG(!resourcemgr->private_data);
	FIMC_BUG(rsc_type >= RESOURCE_TYPE_MAX);

	core = (struct fimc_is_core *)resourcemgr->private_data;

	mutex_lock(&resourcemgr->rsc_lock);

	rsccount = atomic_read(&core->rsccount);
	resource = GET_RESOURCE(resourcemgr, rsc_type);
	if (!resource) {
		err("[RSC] resource is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (!core->pdev) {
		err("[RSC] pdev is NULL");
		ret = -EMFILE;
		goto p_err;
	}

	if (rsccount == 0) {
		err("[RSC] Invalid rsccount(%d)\n", rsccount);
		ret = -EMFILE;
		goto p_err;
	}

	/* local update */
	if (atomic_read(&resource->rsccount) == 1) {
#if !defined(ENABLE_IS_CORE) && !defined(DISABLE_LIB)
		if ((rsc_type == RESOURCE_TYPE_ISCHAIN)
			&& (test_and_clear_bit(FIMC_IS_BINARY_LOADED, &resourcemgr->binary_state))) {
			fimc_is_load_clear();

			info("fimc_is_load_clear() done\n");
		}
#endif
		switch (rsc_type) {
		case RESOURCE_TYPE_PREPROC:
#if defined(CONFIG_PM)
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_preproc_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_COM_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR0:
#if defined(CONFIG_PM)
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_SS0_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR1:
#if defined(CONFIG_PM)
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_SS1_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR2:
#ifdef CONFIG_PM
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_SS2_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR3:
#ifdef CONFIG_PM
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_SS3_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR4:
#ifdef CONFIG_PM
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_SS4_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR5:
#ifdef CONFIG_PM
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_SS5_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_ISCHAIN:
#if defined(SECURE_CAMERA_FACE)
			ret = fimc_is_secure_func(core, NULL, FIMC_IS_SECURE_CAMERA_FACE,
				core->scenario, SMC_SECCAM_UNPREPARE);
#endif
			ret = fimc_is_itf_power_down(&core->interface);
			if (ret)
				err("power down cmd is fail(%d)", ret);

			ret = fimc_is_ischain_power(&core->ischain[0], 0);
			if (ret)
				err("fimc_is_ischain_power is fail(%d)", ret);

			ret = fimc_is_interface_close(&core->interface);
			if (ret)
				err("fimc_is_interface_close is fail(%d)", ret);

#if defined(SECURE_CAMERA_FACE)
			ret = fimc_is_resourcemgr_deinit_secure_mem(resourcemgr);
			if (ret)
				err("fimc_is_resourcemgr_deinit_secure_mem is fail(%d)", ret);
#endif

			ret = fimc_is_debug_close();
			if (ret)
				err("fimc_is_debug_close is fail(%d)", ret);

			clear_bit(FIMC_IS_RM_ISC_POWER_ON, &resourcemgr->state);

#if defined(CONFIG_SOC_EXYNOS8895)
			/* HACK for 8895, cpuidle on/off */
			info("%s: call cpuidle_resume()\n", __func__);
			cpuidle_resume();
#endif

#ifdef CONFIG_EXYNOS_BTS
			info("%s: call bts_update_scen(0)\n", __func__);
			bts_update_scen(BS_CAMERA_DEFAULT, 0);
#endif
			break;
		}

		fimc_is_vender_resource_put(&core->vender);
	}

	/* global update */
	if (atomic_read(&core->rsccount) == 1) {
		u32 current_min, current_max;

#ifdef CONFIG_EXYNOS_BCM_DBG_AUTO
		if (sec_debug_get_debug_level() != DEBUG_LEVEL_LOW) {
			exynos_bcm_dbg_stop(CAMERA_DRIVER);
		}
#endif

#ifdef ENABLE_DYNAMIC_MEM
		ret = fimc_is_resourcemgr_deinit_dynamic_mem(resourcemgr);
		if (ret)
			err("fimc_is_resourcemgr_deinit_dynamic_mem is fail(%d)", ret);
#endif

		current_min = (resourcemgr->cluster0 & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
		current_max = (resourcemgr->cluster0 & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;
		if (current_min) {
			C0MIN_QOS_DEL();
			warn("[RSC] cluster0 minfreq is not removed(%dMhz)\n", current_min);
		}

		if (current_max) {
			C0MAX_QOS_DEL();
			warn("[RSC] cluster0 maxfreq is not removed(%dMhz)\n", current_max);
		}

		current_min = (resourcemgr->cluster1 & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
		current_max = (resourcemgr->cluster1 & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;
		if (current_min) {
			C1MIN_QOS_DEL();
			warn("[RSC] cluster1 minfreq is not removed(%dMhz)\n", current_min);
		}

		if (current_max) {
			C1MAX_QOS_DEL();
			warn("[RSC] cluster1 maxfreq is not removed(%dMhz)\n", current_max);
		}

		current_min = (resourcemgr->cluster2 & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
		current_max = (resourcemgr->cluster2 & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;
		if (current_min) {
			C2MIN_QOS_DEL();
			warn("[RSC] cluster2 minfreq is not removed(%dMhz)\n", current_min);
		}

		if (current_max) {
			C2MAX_QOS_DEL();
			warn("[RSC] cluster2 maxfreq is not removed(%dMhz)\n", current_max);
		}

		resourcemgr->cluster0 = 0;
		resourcemgr->cluster1 = 0;
		resourcemgr->cluster2 = 0;
#ifdef CONFIG_CMU_EWF
		set_cmuewf(idx_ewf, 0);
#endif
#ifdef CONFIG_VENDER_MCD
		core->vender.closing_hint = IS_CLOSING_HINT_NONE;
#endif

		/* clear hal version, default 1.0 */
		resourcemgr->hal_version = IS_HAL_VER_1_0;

		ret = fimc_is_runtime_suspend_post(&resource->pdev->dev);
		if (ret)
			err("fimc_is_runtime_suspend_post is fail(%d)", ret);

		pm_relax(&core->pdev->dev);

		clear_bit(FIMC_IS_RM_POWER_ON, &resourcemgr->state);
	}

	atomic_dec(&resource->rsccount);
	atomic_dec(&core->rsccount);

p_err:
	mutex_unlock(&resourcemgr->rsc_lock);

	info("[RSC] rsctype: %d, rsccount: device[%d], core[%d]\n", rsc_type,
			atomic_read(&resource->rsccount), rsccount - 1);
	return ret;
}

int fimc_is_resource_ioctl(struct fimc_is_resourcemgr *resourcemgr, struct v4l2_control *ctrl)
{
	int ret = 0;

	FIMC_BUG(!resourcemgr);
	FIMC_BUG(!ctrl);

	switch (ctrl->id) {
	/* APOLLO CPU0~3 */
	case V4L2_CID_IS_DVFS_CLUSTER0:
		{
			u32 current_min, current_max;
			u32 request_min, request_max;

			current_min = (resourcemgr->cluster0 & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
			current_max = (resourcemgr->cluster0 & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;
			request_min = (ctrl->value & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
			request_max = (ctrl->value & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;

			if (current_min) {
				if (request_min)
					C0MIN_QOS_UPDATE(request_min);
				else
					C0MIN_QOS_DEL();
			} else {
				if (request_min)
					C0MIN_QOS_ADD(request_min);
			}

			if (current_max) {
				if (request_max)
					C0MAX_QOS_UPDATE(request_max);
				else
					C0MAX_QOS_DEL();
			} else {
				if (request_max)
					C0MAX_QOS_ADD(request_max);
			}

			info("[RSC] cluster0 minfreq : %dMhz\n", request_min);
			info("[RSC] cluster0 maxfreq : %dMhz\n", request_max);
			resourcemgr->cluster0 = (request_max << CLUSTER_MAX_SHIFT) | request_min;
		}
		break;
	/* ATLAS CPU4~5 */
	case V4L2_CID_IS_DVFS_CLUSTER1:
		{
			u32 current_min, current_max;
			u32 request_min, request_max;

			current_min = (resourcemgr->cluster1 & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
			current_max = (resourcemgr->cluster1 & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;
			request_min = (ctrl->value & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
			request_max = (ctrl->value & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;

			if (current_min) {
				if (request_min)
					C1MIN_QOS_UPDATE(request_min);
				else
					C1MIN_QOS_DEL();
			} else {
				if (request_min)
					C1MIN_QOS_ADD(request_min);
			}

			if (current_max) {
				if (request_max)
					C1MAX_QOS_UPDATE(request_max);
				else
					C1MAX_QOS_DEL();
			} else {
				if (request_max)
					C1MAX_QOS_ADD(request_max);
			}

			info("[RSC] cluster1 minfreq : %dMhz\n", request_min);
			info("[RSC] cluster1 maxfreq : %dMhz\n", request_max);
			resourcemgr->cluster1 = (request_max << CLUSTER_MAX_SHIFT) | request_min;
		}
		break;
	/* ATLAS CPU6~7 */
	case V4L2_CID_IS_DVFS_CLUSTER2:
		{
			u32 current_min, current_max;
			u32 request_min, request_max;

			current_min = (resourcemgr->cluster2 & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
			current_max = (resourcemgr->cluster2 & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;
			request_min = (ctrl->value & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
			request_max = (ctrl->value & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;

			if (current_min) {
				if (request_min)
					C2MIN_QOS_UPDATE(request_min);
				else
					C2MIN_QOS_DEL();
			} else {
				if (request_min)
					C2MIN_QOS_ADD(request_min);
			}

			if (current_max) {
				if (request_max)
					C2MAX_QOS_UPDATE(request_max);
				else
					C2MAX_QOS_DEL();
			} else {
				if (request_max)
					C2MAX_QOS_ADD(request_max);
			}

			info("[RSC] cluster2 minfreq : %dMhz\n", request_min);
			info("[RSC] cluster2 maxfreq : %dMhz\n", request_max);
			resourcemgr->cluster2 = (request_max << CLUSTER_MAX_SHIFT) | request_min;
		}
		break;
	}

	return ret;
}

void fimc_is_resource_set_global_param(struct fimc_is_resourcemgr *resourcemgr, void *device)
{
	bool video_mode;
	struct fimc_is_device_ischain *ischain = device;
	struct fimc_is_global_param *global_param = &resourcemgr->global_param;

	mutex_lock(&global_param->lock);

	if (!global_param->state) {
		video_mode = IS_VIDEO_SCENARIO(ischain->setfile & FIMC_IS_SETFILE_MASK);
		global_param->video_mode = video_mode;
		ischain->hardware->video_mode = video_mode;
		minfo("video mode %d\n", ischain, video_mode);
	}

	set_bit(ischain->instance, &global_param->state);

	mutex_unlock(&global_param->lock);
}

void fimc_is_resource_clear_global_param(struct fimc_is_resourcemgr *resourcemgr, void *device)
{
	struct fimc_is_device_ischain *ischain = device;
	struct fimc_is_global_param *global_param = &resourcemgr->global_param;

	mutex_lock(&global_param->lock);

	clear_bit(ischain->instance, &global_param->state);

	if (!global_param->state) {
		global_param->video_mode = false;
		ischain->hardware->video_mode = false;
	}

	mutex_unlock(&global_param->lock);
}

int fimc_is_logsync(struct fimc_is_interface *itf, u32 sync_id, u32 msg_test_id)
{
	int ret = 0;

	/* print kernel sync log */
	log_sync(sync_id);

#ifdef ENABLE_FW_SYNC_LOG
	ret = fimc_is_hw_msg_test(itf, sync_id, msg_test_id);
	if (ret)
	err("fimc_is_hw_msg_test(%d)", ret);
#endif
	return ret;
}
