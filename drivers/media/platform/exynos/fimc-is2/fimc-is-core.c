/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <video/videonode.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/pm_qos.h>
#include <linux/bug.h>
#include <linux/v4l2-mediabus.h>
#include <linux/gpio.h>
#
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif
#if defined(CONFIG_SECURE_CAMERA_USE)
#include <linux/smc.h>
#endif

#include "fimc-is-core.h"
#include "fimc-is-cmd.h"
#include "fimc-is-regs.h"
#include "fimc-is-debug.h"
#include "fimc-is-hw.h"
#include "fimc-is-err.h"
#include "fimc-is-framemgr.h"
#include "fimc-is-dt.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-clk-gate.h"
#include "fimc-is-dvfs.h"
#include "include/fimc-is-module.h"
#include "fimc-is-device-sensor.h"
#include "sensor/module_framework/fimc-is-device-sensor-peri.h"

#ifdef ENABLE_FAULT_HANDLER
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
#include <linux/exynos_iovmm.h>
#else
#include <plat/sysmmu.h>
#endif
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
#define PM_QOS_CAM_THROUGHPUT	PM_QOS_RESERVED
#endif

struct device *fimc_is_dev = NULL;

extern struct pm_qos_request exynos_isp_qos_int;
extern struct pm_qos_request exynos_isp_qos_mem;
extern struct pm_qos_request exynos_isp_qos_cam;

/* sysfs global variable for debug */
struct fimc_is_sysfs_debug sysfs_debug;

#ifndef ENABLE_IS_CORE
/* sysfs global variable for set position to actuator */
struct fimc_is_sysfs_actuator sysfs_actuator;
#ifdef FIXED_SENSOR_DEBUG
struct fimc_is_sysfs_sensor sysfs_sensor;
#endif
#endif

int debug_clk;
module_param(debug_clk, int, 0644);
int debug_stream;
module_param(debug_stream, int, 0644);
int debug_video;
module_param(debug_video, int, 0644);
int debug_hw;
module_param(debug_hw, int, 0644);
int debug_device;
module_param(debug_device, int, 0644);
int debug_irq;
module_param(debug_irq, int, 0644);
int debug_csi;
module_param(debug_csi, int, 0644);
int debug_sensor;
module_param(debug_sensor, int, 0644);
int debug_time_launch;
module_param(debug_time_launch, int, 0644);
int debug_time_queue;
module_param(debug_time_queue, int, 0644);
int debug_time_shot;
module_param(debug_time_shot, int, 0644);
int debug_pdp;
module_param(debug_pdp, int, 0644);

struct fimc_is_device_sensor *fimc_is_get_sensor_device(struct fimc_is_core *core)
{
	mutex_lock(&core->sensor_lock);

	return &core->sensor[0];
}

int fimc_is_put_sensor_device(struct fimc_is_core *core)
{
	mutex_unlock(&core->sensor_lock);

	return 0;
}

#ifdef CONFIG_CPU_THERMAL_IPA
static int fimc_is_mif_throttling_notifier(struct notifier_block *nb,
		unsigned long val, void *v)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_device_sensor *device = NULL;
	int i;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core is null");
		goto exit;
	}

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		if (test_bit(FIMC_IS_SENSOR_OPEN, &core->sensor[i].state)) {
			device = &core->sensor[i];
			break;
		}
	}

	if (device && !test_bit(FIMC_IS_SENSOR_FRONT_DTP_STOP, &device->state))
		/* Set DTP */
		set_bit(FIMC_IS_MIF_THROTTLING_STOP, &device->force_stop);
	else
		err("any sensor is not opened");

exit:
	err("MIF: cause of mif_throttling, mif_qos is [%lu]!!!\n", val);

	return NOTIFY_OK;
}

static struct notifier_block exynos_fimc_is_mif_throttling_nb = {
	.notifier_call = fimc_is_mif_throttling_notifier,
};
#endif

static int fimc_is_suspend(struct device *dev)
{
	pr_debug("FIMC_IS Suspend\n");
	return 0;
}

static int fimc_is_resume(struct device *dev)
{
	pr_debug("FIMC_IS Resume\n");
	return 0;
}

#if defined(SECURE_CAMERA_IRIS)
static int fimc_is_secure_iris(struct fimc_is_device_sensor *device,
	u32 type, u32 scenario, ulong smc_cmd)
{
	int ret = 0;

	if (scenario != SENSOR_SCENARIO_SECURE)
		return ret;

	switch (smc_cmd) {
	case SMC_SECCAM_PREPARE:
		ret = exynos_smc(SMC_SECCAM_PREPARE, 0, 0, 0);
		if (ret) {
			merr("[SMC] SMC_SECURE_CAMERA_PREPARE fail(%d)\n", device, ret);
		} else {
			minfo("[SMC] Call SMC_SECURE_CAMERA_PREPARE ret(%d) / smc_state(%d->%d)\n",
				device, ret, device->smc_state, FIMC_IS_SENSOR_SMC_PREPARE);
			device->smc_state = FIMC_IS_SENSOR_SMC_PREPARE;
		}
		break;
	case SMC_SECCAM_UNPREPARE:
		if (device->smc_state != FIMC_IS_SENSOR_SMC_PREPARE)
			break;

		ret = exynos_smc(SMC_SECCAM_UNPREPARE, 0, 0, 0);
		if (ret != 0) {
			merr("[SMC] SMC_SECURE_CAMERA_UNPREPARE fail(%d)\n", device, ret);
		} else {
			minfo("[SMC] Call SMC_SECURE_CAMERA_UNPREPARE ret(%d) / smc_state(%d->%d)\n",
				device, ret, device->smc_state, FIMC_IS_SENSOR_SMC_UNPREPARE);
			device->smc_state = FIMC_IS_SENSOR_SMC_UNPREPARE;
		}
		break;
	default:
		break;
	}

	return ret;
}
#endif

#if defined(SECURE_CAMERA_FACE)
static int fimc_is_secure_face(struct fimc_is_core *core,
	u32 type, u32 scenario, ulong smc_cmd)
{
	int ret = 0;

	if (scenario != FIMC_IS_SCENARIO_SECURE)
		return ret;

	mutex_lock(&core->secure_state_lock);
	switch (smc_cmd) {
	case SMC_SECCAM_PREPARE:
		if (core->secure_state == FIMC_IS_STATE_UNSECURE) {
#if defined(SECURE_CAMERA_FACE_SEQ_CHK)
			ret = 0;
#else
			ret = exynos_smc(SMC_SECCAM_PREPARE, 0, 0, 0);
#endif
			if (ret != 0) {
				err("[SMC] SMC_SECCAM_PREPARE fail(%d)", ret);
			} else {
				info("[SMC] Call SMC_SECCAM_PREPARE ret(%d) / state(%d->%d)\n",
					ret, core->secure_state, FIMC_IS_STATE_SECURED);
				core->secure_state = FIMC_IS_STATE_SECURED;
			}
		}
		break;
	case SMC_SECCAM_UNPREPARE:
		if (core->secure_state == FIMC_IS_STATE_SECURED) {
#if defined(SECURE_CAMERA_FACE_SEQ_CHK)
			ret = 0;
#else
			ret = exynos_smc(SMC_SECCAM_UNPREPARE, 0, 0, 0);
#endif
			if (ret != 0) {
				err("[SMC] SMC_SECCAM_UNPREPARE fail(%d)\n", ret);
			} else {
				info("[SMC] Call SMC_SECCAM_UNPREPARE ret(%d) / smc_state(%d->%d)\n",
					ret, core->secure_state, FIMC_IS_STATE_UNSECURE);
				core->secure_state = FIMC_IS_STATE_UNSECURE;
			}
		}
		break;
	default:
		break;
	}
	mutex_unlock(&core->secure_state_lock);

	return ret;
}
#endif

int fimc_is_secure_func(struct fimc_is_core *core,
	struct fimc_is_device_sensor *device, u32 type, u32 scenario, ulong smc_cmd)
{
	int ret = 0;

	switch (type) {
	case FIMC_IS_SECURE_CAMERA_IRIS:
#if defined(SECURE_CAMERA_IRIS)
		ret = fimc_is_secure_iris(device, type, scenario, smc_cmd);
#endif
		break;
	case FIMC_IS_SECURE_CAMERA_FACE:
#if defined(SECURE_CAMERA_FACE)
		ret = fimc_is_secure_face(core, type, scenario, smc_cmd);
#endif
		break;
	default:
		break;
	}

	return ret;
}

#ifdef ENABLE_FAULT_HANDLER
static void fimc_is_print_target_dva(struct fimc_is_frame *leader_frame)
{
	u32 plane_index;

	for (plane_index = 0; plane_index < FIMC_IS_MAX_PLANES; plane_index++) {
		if (leader_frame->sourceAddress[plane_index])
			pr_err("sourceAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->sourceAddress[plane_index]);
		if (leader_frame->txcTargetAddress[plane_index])
			pr_err("txcTargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->txcTargetAddress[plane_index]);
		if (leader_frame->txpTargetAddress[plane_index])
			pr_err("txpTargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->txpTargetAddress[plane_index]);
		if (leader_frame->ixcTargetAddress[plane_index])
			pr_err("ixcTargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->ixcTargetAddress[plane_index]);
		if (leader_frame->ixpTargetAddress[plane_index])
			pr_err("ixpTargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->ixpTargetAddress[plane_index]);
		if (leader_frame->mexcTargetAddress[plane_index])
			pr_err("mexcTargetAddress[%d] = 0x%16LX\n",
				plane_index, leader_frame->mexcTargetAddress[plane_index]);
		if (leader_frame->sccTargetAddress[plane_index])
			pr_err("sccTargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->sccTargetAddress[plane_index]);
		if (leader_frame->scpTargetAddress[plane_index])
			pr_err("scpTargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->scpTargetAddress[plane_index]);
		if (leader_frame->sc0TargetAddress[plane_index])
			pr_err("sc0TargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->sc0TargetAddress[plane_index]);
		if (leader_frame->sc1TargetAddress[plane_index])
			pr_err("sc1TargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->sc1TargetAddress[plane_index]);
		if (leader_frame->sc2TargetAddress[plane_index])
			pr_err("sc2TargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->sc2TargetAddress[plane_index]);
		if (leader_frame->sc3TargetAddress[plane_index])
			pr_err("sc3TargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->sc3TargetAddress[plane_index]);
		if (leader_frame->sc4TargetAddress[plane_index])
			pr_err("sc4TargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->sc4TargetAddress[plane_index]);
		if (leader_frame->sc5TargetAddress[plane_index])
			pr_err("sc5TargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->sc5TargetAddress[plane_index]);
		if (leader_frame->dxcTargetAddress[plane_index])
			pr_err("dxcTargetAddress[%d] = 0x%08X\n",
				plane_index, leader_frame->dxcTargetAddress[plane_index]);
	}
}

void fimc_is_print_frame_dva(struct fimc_is_subdev *subdev)
{
	u32 j, k;
	struct fimc_is_framemgr *framemgr;
	struct camera2_shot *shot;

	framemgr = GET_SUBDEV_FRAMEMGR(subdev);
	if (test_bit(FIMC_IS_SUBDEV_START, &subdev->state) && framemgr) {
		for (j = 0; j < framemgr->num_frames; ++j) {
			for (k = 0; k < framemgr->frames[j].planes; k++) {
				msinfo(" BUF[%d][%d] %pad = (0x%lX)\n",
					subdev, subdev, j, k,
					&framemgr->frames[j].dvaddr_buffer[k],
					framemgr->frames[j].mem_state);

				shot = framemgr->frames[j].shot;
				if (shot)
					fimc_is_print_target_dva(&framemgr->frames[j]);
			}
		}
	}
}

static void __fimc_is_fault_handler(struct device *dev)
{
	u32 i, j, k, sd_index;
	int vc;
	struct fimc_is_core *core;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_ischain *ischain;
	struct fimc_is_subdev *subdev;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_resourcemgr *resourcemgr;

	core = dev_get_drvdata(dev);
	if (core) {
		resourcemgr = &core->resourcemgr;

		fimc_is_hw_fault(&core->interface);
		/* dump FW page table 1nd(~16KB), 2nd(16KB~32KB) */
		fimc_is_hw_memdump(&core->interface,
			resourcemgr->minfo.kvaddr + TTB_OFFSET, /* TTB_BASE ~ 32KB */
			resourcemgr->minfo.kvaddr + TTB_OFFSET + TTB_SIZE);
		fimc_is_hw_logdump(&core->interface);

		/* SENSOR */
		for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
			sensor = &core->sensor[i];
			framemgr = GET_FRAMEMGR(sensor->vctx);
			if (test_bit(FIMC_IS_SENSOR_OPEN, &sensor->state) && framemgr) {
				struct fimc_is_device_flite *flite;
				struct fimc_is_device_csi *csi;

				for (j = 0; j < framemgr->num_frames; ++j) {
					for (k = 0; k < framemgr->frames[j].planes; k++) {
						pr_err("[SS%d] BUF[%d][%d] = %pad(0x%lX)\n", i, j, k,
							&framemgr->frames[j].dvaddr_buffer[k],
							framemgr->frames[j].mem_state);
					}
				}

				/* vc0 */
				subdev = &sensor->ssvc0;
				for (sd_index = 0; sd_index < CSI_VIRTUAL_CH_MAX; sd_index++) {
					fimc_is_print_frame_dva(subdev);
					subdev++;
				}

				/* csis, bns sfr dump */
				if (sensor->subdev_flite) {
					flite = (struct fimc_is_device_flite *)v4l2_get_subdevdata(sensor->subdev_flite);
					if (flite)
						flite_hw_dump(flite->base_reg);
				}

				csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
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
		}

		/* ISCHAIN */
		for (i = 0; i < FIMC_IS_STREAM_COUNT; i++) {
			ischain = &core->ischain[i];
			if (test_bit(FIMC_IS_ISCHAIN_OPEN, &ischain->state)) {
				/* 3AA */
				subdev = &ischain->group_3aa.leader;
				fimc_is_print_frame_dva(subdev);

				/* 3AAC ~ 3AAP */
				subdev = &ischain->txc;
				for (sd_index = 0; sd_index < NUM_OF_3AA_SUBDEV; sd_index++) {
					fimc_is_print_frame_dva(subdev);
					subdev++;
				}

				/* ISP */
				subdev = &ischain->group_isp.leader;
				fimc_is_print_frame_dva(subdev);

				/* ISPC */
				subdev = &ischain->ixc;
				for (sd_index = 0; sd_index < NUM_OF_ISP_SUBDEV; sd_index++) {
					fimc_is_print_frame_dva(subdev);
					subdev++;
				}

				/* DCP */
				subdev = &ischain->group_dcp.leader;
				fimc_is_print_frame_dva(subdev);

				/* DC1S ~ DC4C*/
				subdev = &ischain->dc1s;
				for (sd_index = 0; sd_index < NUM_OF_DCP_SUBDEV; sd_index++) {
					fimc_is_print_frame_dva(subdev);
					subdev++;
				}

				/* for ME */
				subdev = &ischain->mexc;
				fimc_is_print_frame_dva(subdev);

				/* DIS */
				subdev = &ischain->group_dis.leader;
				fimc_is_print_frame_dva(subdev);

				/* SCC */
				subdev = &ischain->scc;
				fimc_is_print_frame_dva(subdev);

				/* SCP */
				subdev = &ischain->scp;
				fimc_is_print_frame_dva(subdev);

				/* MCS */
				subdev = &ischain->group_mcs.leader;
				fimc_is_print_frame_dva(subdev);

				/* M0P ~ M5P */
				subdev = &ischain->m0p;
				for (sd_index = 0; sd_index < NUM_OF_MCS_SUBDEV; sd_index++) {
					fimc_is_print_frame_dva(subdev);
					subdev++;
				}

				/* VRA */
				subdev = &ischain->group_vra.leader;
				fimc_is_print_frame_dva(subdev);
			}
		}
	} else {
		pr_err("failed to get core\n");
	}
}

static void wq_func_print_clk(struct work_struct *data)
{
	struct fimc_is_core *core;

	core = container_of(data, struct fimc_is_core, wq_data_print_clk);

	CALL_POPS(core, print_clk);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0))
#define SECT_ORDER 20
#define LPAGE_ORDER 16
#define SPAGE_ORDER 12

#define lv1ent_page(sent) ((*(sent) & 3) == 1)

#define lv1ent_offset(iova) ((iova) >> SECT_ORDER)
#define lv2ent_offset(iova) (((iova) & 0xFF000) >> SPAGE_ORDER)
#define lv2table_base(sent) (*(sent) & 0xFFFFFC00)

static unsigned long *section_entry(unsigned long *pgtable, unsigned long iova)
{
	return pgtable + lv1ent_offset(iova);
}

static unsigned long *page_entry(unsigned long *sent, unsigned long iova)
{
	return (unsigned long *)__va(lv2table_base(sent)) + lv2ent_offset(iova);
}

static char *sysmmu_fault_name[SYSMMU_FAULTS_NUM] = {
	"PAGE FAULT",
	"AR MULTI-HIT FAULT",
	"AW MULTI-HIT FAULT",
	"BUS ERROR",
	"AR SECURITY PROTECTION FAULT",
	"AR ACCESS PROTECTION FAULT",
	"AW SECURITY PROTECTION FAULT",
	"AW ACCESS PROTECTION FAULT",
	"UNKNOWN FAULT"
};

static int fimc_is_fault_handler(struct device *dev, const char *mmuname,
					enum exynos_sysmmu_inttype itype,
					unsigned long pgtable_base,
					unsigned long fault_addr)
{
	unsigned long *ent;

	if ((itype >= SYSMMU_FAULTS_NUM) || (itype < SYSMMU_PAGEFAULT))
		itype = SYSMMU_FAULT_UNKNOWN;

	pr_err("%s occured at 0x%lx by '%s'(Page table base: 0x%lx)\n",
		sysmmu_fault_name[itype], fault_addr, mmuname, pgtable_base);

	ent = section_entry(__va(pgtable_base), fault_addr);
	pr_err("\tLv1 entry: 0x%lx\n", *ent);

	if (lv1ent_page(ent)) {
		ent = page_entry(ent, fault_addr);
		pr_err("\t Lv2 entry: 0x%lx\n", *ent);
	}

	__fimc_is_fault_handler(dev);

	pr_err("Generating Kernel OOPS... because it is unrecoverable.\n");

	BUG();

	return 0;
}
#else
static int __attribute__((unused)) fimc_is_fault_handler(struct iommu_domain *domain,
	struct device *dev,
	unsigned long fault_addr,
	int fault_flag,
	void *token)
{
	pr_err("<FIMC-IS FAULT HANDLER>\n");
	pr_err("Device virtual(0x%X) is invalid access\n", (u32)fault_addr);

	__fimc_is_fault_handler(dev);

	return -EINVAL;
}
#endif
#endif /* ENABLE_FAULT_HANDLER */

static ssize_t show_clk_gate_mode(struct device *dev, struct device_attribute *attr,
				  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", sysfs_debug.clk_gate_mode);
}

static ssize_t store_clk_gate_mode(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
#ifdef HAS_FW_CLOCK_GATE
	switch (buf[0]) {
	case '0':
		sysfs_debug.clk_gate_mode = CLOCK_GATE_MODE_HOST;
		break;
	case '1':
		sysfs_debug.clk_gate_mode = CLOCK_GATE_MODE_FW;
		break;
	default:
		pr_debug("%s: %c\n", __func__, buf[0]);
		break;
	}
#endif
	return count;
}

static ssize_t show_en_clk_gate(struct device *dev, struct device_attribute *attr,
				  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", sysfs_debug.en_clk_gate);
}

static ssize_t store_en_clk_gate(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
#ifdef ENABLE_CLOCK_GATE
	switch (buf[0]) {
	case '0':
		sysfs_debug.en_clk_gate = false;
		sysfs_debug.clk_gate_mode = CLOCK_GATE_MODE_HOST;
		break;
	case '1':
		sysfs_debug.en_clk_gate = true;
		sysfs_debug.clk_gate_mode = CLOCK_GATE_MODE_HOST;
		break;
	default:
		pr_debug("%s: %c\n", __func__, buf[0]);
		break;
	}
#endif

#ifdef ENABLE_DIRECT_CLOCK_GATE
	switch (buf[0]) {
	case '0':
		sysfs_debug.en_clk_gate = false;
		break;
	case '1':
		sysfs_debug.en_clk_gate = true;
		break;
	default:
		pr_debug("%s: %c\n", __func__, buf[0]);
		break;
	}
#endif
	return count;
}

static ssize_t show_en_dvfs(struct device *dev, struct device_attribute *attr,
				  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", sysfs_debug.en_dvfs);
}

static ssize_t store_en_dvfs(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
#ifdef ENABLE_DVFS
	struct fimc_is_core *core =
		(struct fimc_is_core *)dev_get_drvdata(dev);
	struct fimc_is_resourcemgr *resourcemgr;
	int i;

	FIMC_BUG(!core);

	resourcemgr = &core->resourcemgr;

	switch (buf[0]) {
	case '0':
		sysfs_debug.en_dvfs = false;
		/* update dvfs lever to max */
		mutex_lock(&resourcemgr->dvfs_ctrl.lock);
		for (i = 0; i < FIMC_IS_STREAM_COUNT; i++) {
			if (test_bit(FIMC_IS_ISCHAIN_OPEN, &((core->ischain[i]).state)))
				fimc_is_set_dvfs(core, &(core->ischain[i]), FIMC_IS_SN_MAX);
		}
		fimc_is_dvfs_init(resourcemgr);
		resourcemgr->dvfs_ctrl.static_ctrl->cur_scenario_id = FIMC_IS_SN_MAX;
		mutex_unlock(&resourcemgr->dvfs_ctrl.lock);
		break;
	case '1':
		/* It can not re-define static scenario */
		sysfs_debug.en_dvfs = true;
		break;
	default:
		pr_debug("%s: %c\n", __func__, buf[0]);
		break;
	}
#endif
	return count;
}

static ssize_t show_pattern_en(struct device *dev, struct device_attribute *attr,
				  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", sysfs_debug.pattern_en);
}

static ssize_t store_pattern_en(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int ret = 0;
	unsigned int cmd;
	struct fimc_is_core *core =
		(struct fimc_is_core *)platform_get_drvdata(to_platform_device(dev));

	ret = kstrtouint(buf, 0, &cmd);
	if (ret)
		return ret;

	switch (cmd) {
	case 0:
	case 1:
		if (atomic_read(&core->rsccount))
			pr_warn("%s: patter generator cannot be enabled while camera is running.\n", __func__);
		else
			sysfs_debug.pattern_en = cmd;
		break;
	default:
		pr_warn("%s: invalid parameter (%d)\n", __func__, cmd);
		break;
	}

	return count;
}

static ssize_t show_pattern_fps(struct device *dev, struct device_attribute *attr,
				  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", sysfs_debug.pattern_fps);
}

static ssize_t store_pattern_fps(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int ret = 0;
	unsigned int cmd;

	ret = kstrtouint(buf, 0, &cmd);
	if (ret)
		return ret;

	sysfs_debug.pattern_fps = cmd;

	return count;
}

static ssize_t show_hal_debug_mode(struct device *dev, struct device_attribute *attr,
				  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%lx\n", sysfs_debug.hal_debug_mode);
}

static ssize_t store_hal_debug_mode(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int ret;
	unsigned long long debug_mode = 0;

	ret = kstrtoull(buf, 16 /* hexa */, &debug_mode);
	if (ret < 0) {
		pr_err("%s, %s, failed for debug_mode:%llu, ret:%d", __func__, buf, debug_mode, ret);
		return 0;
	}

	sysfs_debug.hal_debug_mode = (unsigned long)debug_mode;

	return count;
}

static ssize_t show_hal_debug_delay(struct device *dev, struct device_attribute *attr,
				  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u ms\n", sysfs_debug.hal_debug_delay);
}

static ssize_t store_hal_debug_delay(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int ret;

	ret = kstrtouint(buf, 10, &sysfs_debug.hal_debug_delay);
	if (ret < 0) {
		pr_err("%s, %s, failed for debug_delay:%u, ret:%d", __func__, buf, sysfs_debug.hal_debug_delay, ret);
		return 0;
	}

	return count;
}

#ifdef ENABLE_DBG_STATE
static ssize_t show_debug_state(struct device *dev, struct device_attribute *attr,
				  char *buf)
{
	struct fimc_is_core *core =
		(struct fimc_is_core *)dev_get_drvdata(dev);
	struct fimc_is_resourcemgr *resourcemgr;

	FIMC_BUG(!core);

	resourcemgr = &core->resourcemgr;

	return snprintf(buf, PAGE_SIZE, "%d\n", resourcemgr->hal_version);
}

static ssize_t store_debug_state(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct fimc_is_core *core =
		(struct fimc_is_core *)dev_get_drvdata(dev);
	struct fimc_is_resourcemgr *resourcemgr;

	FIMC_BUG(!core);

	resourcemgr = &core->resourcemgr;

	switch (buf[0]) {
	case '0':
		break;
	case '1':
		break;
	case '7':
		break;
	default:
		pr_debug("%s: %c\n", __func__, buf[0]);
		break;
	}

	return count;
}
#endif

#ifndef ENABLE_IS_CORE
static ssize_t store_actuator_init_step(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret_count;
	unsigned int init_step;

	ret_count = sscanf(buf, "%u", &init_step);
	if (ret_count != 1)
		return -EINVAL;

	switch (init_step) {
		/* case number is step of set position */
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			sysfs_actuator.init_step = init_step;
			break;
		/* default actuator setting (2step default) */
		default:
			sysfs_actuator.init_step = 0;
			break;
	}

	return count;
}

static ssize_t store_actuator_init_positions(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int i;
	int ret_count;
	int init_positions[INIT_MAX_SETTING];

	ret_count = sscanf(buf, "%d %d %d %d %d", &init_positions[0], &init_positions[1],
						&init_positions[2], &init_positions[3], &init_positions[4]);
	if (ret_count > INIT_MAX_SETTING)
		return -EINVAL;

	for (i = 0; i < ret_count; i++) {
		if (init_positions[i] >= 0 && init_positions[i] < 1024)
			sysfs_actuator.init_positions[i] = init_positions[i];
		else
			sysfs_actuator.init_positions[i] = 0;
	}

	return count;
}

static ssize_t store_actuator_init_delays(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret_count;
	int i;
	int init_delays[INIT_MAX_SETTING];

	ret_count = sscanf(buf, "%d %d %d %d %d", &init_delays[0], &init_delays[1],
							&init_delays[2], &init_delays[3], &init_delays[4]);
	if (ret_count > INIT_MAX_SETTING)
		return -EINVAL;

	for (i = 0; i < ret_count; i++) {
		if (init_delays[i] >= 0)
			sysfs_actuator.init_delays[i] = init_delays[i];
		else
			sysfs_actuator.init_delays[i] = 0;
	}

	return count;
}

static ssize_t store_actuator_enable_fixed(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	if (buf[0] == '1')
		sysfs_actuator.enable_fixed = true;
	else
		sysfs_actuator.enable_fixed = false;

	return count;
}

static ssize_t store_actuator_fixed_position(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret_count;
	int fixed_position;

	ret_count = sscanf(buf, "%d", &fixed_position);

	if (ret_count != 1)
		return -EINVAL;

	if (fixed_position < 0 || fixed_position > 1024)
		return -EINVAL;

	sysfs_actuator.fixed_position = fixed_position;

	return count;
}

static ssize_t show_actuator_init_step(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", sysfs_actuator.init_step);
}

static ssize_t show_actuator_init_positions(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d %d %d %d %d\n", sysfs_actuator.init_positions[0],
						sysfs_actuator.init_positions[1], sysfs_actuator.init_positions[2],
						sysfs_actuator.init_positions[3], sysfs_actuator.init_positions[4]);
}

static ssize_t show_actuator_init_delays(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d %d %d %d %d\n", sysfs_actuator.init_delays[0],
							sysfs_actuator.init_delays[1], sysfs_actuator.init_delays[2],
							sysfs_actuator.init_delays[3], sysfs_actuator.init_delays[4]);
}

static ssize_t show_actuator_enable_fixed(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", sysfs_actuator.enable_fixed);
}

static ssize_t show_actuator_fixed_position(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", sysfs_actuator.fixed_position);
}

#ifdef FIXED_SENSOR_DEBUG
static ssize_t show_fixed_sensor_val(struct device *dev, struct device_attribute *attr,
				  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "fps(%d) ex(%d %d) a_gain(%d %d) d_gain(%d %d)\n",
			sysfs_sensor.frame_duration,
			sysfs_sensor.long_exposure_time,
			sysfs_sensor.short_exposure_time,
			sysfs_sensor.long_analog_gain,
			sysfs_sensor.short_analog_gain,
			sysfs_sensor.long_digital_gain,
			sysfs_sensor.short_digital_gain);
}

static ssize_t store_fixed_sensor_val(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int ret_count;
	int input_val[7];

	ret_count = sscanf(buf, "%d %d %d %d %d %d %d", &input_val[0], &input_val[1],
							&input_val[2], &input_val[3],
							&input_val[4], &input_val[5], &input_val[6]);
	if (ret_count != 7) {
		probe_err("%s: count should be 7 but %d \n", __func__, ret_count);
		return -EINVAL;
	}

	sysfs_sensor.frame_duration = input_val[0];
	sysfs_sensor.long_exposure_time = input_val[1];
	sysfs_sensor.short_exposure_time = input_val[2];
	sysfs_sensor.long_analog_gain = input_val[3];
	sysfs_sensor.short_analog_gain = input_val[4];
	sysfs_sensor.long_digital_gain = input_val[5];
	sysfs_sensor.short_digital_gain = input_val[6];

	return count;



}

static ssize_t show_en_fixed_sensor(struct device *dev, struct device_attribute *attr,
				  char *buf)
{
	if (sysfs_sensor.is_en)
		return snprintf(buf, PAGE_SIZE, "%s\n", "enabled");
	else
		return snprintf(buf, PAGE_SIZE, "%s\n", "disabled");
}

static ssize_t store_en_fixed_sensor(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	if (buf[0] == '1')
		sysfs_sensor.is_en = true;
	else
		sysfs_sensor.is_en = false;

	return count;
}
#endif
#endif

static DEVICE_ATTR(en_clk_gate, 0644, show_en_clk_gate, store_en_clk_gate);
static DEVICE_ATTR(clk_gate_mode, 0644, show_clk_gate_mode, store_clk_gate_mode);
static DEVICE_ATTR(en_dvfs, 0644, show_en_dvfs, store_en_dvfs);
static DEVICE_ATTR(pattern_en, 0644, show_pattern_en, store_pattern_en);
static DEVICE_ATTR(pattern_fps, 0644, show_pattern_fps, store_pattern_fps);
static DEVICE_ATTR(hal_debug_mode, 0644, show_hal_debug_mode, store_hal_debug_mode);
static DEVICE_ATTR(hal_debug_delay, 0644, show_hal_debug_delay, store_hal_debug_delay);

#ifdef ENABLE_DBG_STATE
static DEVICE_ATTR(en_debug_state, 0644, show_debug_state, store_debug_state);
#endif

#ifndef ENABLE_IS_CORE
static DEVICE_ATTR(init_step, 0644, show_actuator_init_step, store_actuator_init_step);
static DEVICE_ATTR(init_positions, 0644, show_actuator_init_positions, store_actuator_init_positions);
static DEVICE_ATTR(init_delays, 0644, show_actuator_init_delays, store_actuator_init_delays);
static DEVICE_ATTR(enable_fixed, 0644, show_actuator_enable_fixed, store_actuator_enable_fixed);
static DEVICE_ATTR(fixed_position, 0644, show_actuator_fixed_position, store_actuator_fixed_position);

#ifdef FIXED_SENSOR_DEBUG
static DEVICE_ATTR(fixed_sensor_val, 0644, show_fixed_sensor_val, store_fixed_sensor_val);
static DEVICE_ATTR(en_fixed_sensor, 0644, show_en_fixed_sensor, store_en_fixed_sensor);
#endif
#endif

static struct attribute *fimc_is_debug_entries[] = {
	&dev_attr_en_clk_gate.attr,
	&dev_attr_clk_gate_mode.attr,
	&dev_attr_en_dvfs.attr,
	&dev_attr_pattern_en.attr,
	&dev_attr_pattern_fps.attr,
	&dev_attr_hal_debug_mode.attr,
	&dev_attr_hal_debug_delay.attr,
#ifdef ENABLE_DBG_STATE
	&dev_attr_en_debug_state.attr,
#endif
#ifndef ENABLE_IS_CORE
	&dev_attr_init_step.attr,
	&dev_attr_init_positions.attr,
	&dev_attr_init_delays.attr,
	&dev_attr_enable_fixed.attr,
	&dev_attr_fixed_position.attr,
#ifdef FIXED_SENSOR_DEBUG
	&dev_attr_fixed_sensor_val.attr,
	&dev_attr_en_fixed_sensor.attr,
#endif
#endif
	NULL,
};
static struct attribute_group fimc_is_debug_attr_group = {
	.name	= "debug",
	.attrs	= fimc_is_debug_entries,
};

static int __init fimc_is_probe(struct platform_device *pdev)
{
	struct exynos_platform_fimc_is *pdata;
#if defined (ENABLE_IS_CORE) || defined (USE_MCUCTL)
	struct resource *mem_res;
	struct resource *regs_res;
#endif
	struct fimc_is_core *core;
	int ret = -ENODEV;
#ifndef ENABLE_IS_CORE
	int i;
#endif
	u32 stream;
#if defined(USE_I2C_LOCK)
	u32 channel;
#endif
	struct pinctrl_state *s;
#if defined(SECURE_CAMERA_IRIS) || defined(SECURE_CAMERA_FACE)
	ulong mem_info_addr, mem_info_size;
#endif


	probe_info("%s:start(%ld, %ld)\n", __func__,
		sizeof(struct fimc_is_core), sizeof(struct fimc_is_video_ctx));

	core = kzalloc(sizeof(struct fimc_is_core), GFP_KERNEL);
	if (!core) {
		probe_err("core is NULL");
		return -ENOMEM;
	}

	fimc_is_dev = &pdev->dev;
	dev_set_drvdata(fimc_is_dev, core);

	pdata = dev_get_platdata(&pdev->dev);
	if (!pdata) {
#ifdef CONFIG_OF
		ret = fimc_is_parse_dt(pdev);
		if (ret) {
			err("fimc_is_parse_dt is fail(%d)", ret);
			return ret;
		}

		pdata = dev_get_platdata(&pdev->dev);
#else
		BUG();
#endif
	}

#ifdef USE_ION_ALLOC
	core->fimc_ion_client = exynos_ion_client_create("fimc-is");
#endif
	core->pdev = pdev;
	core->pdata = pdata;
	core->current_position = SENSOR_POSITION_REAR;
	device_init_wakeup(&pdev->dev, true);

	INIT_WORK(&core->wq_data_print_clk, wq_func_print_clk);

	/* for mideaserver force down */
	atomic_set(&core->rsccount, 0);

#if defined (ENABLE_IS_CORE) || defined (USE_MCUCTL)
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_res) {
		probe_err("Failed to get io memory region(%p)", mem_res);
		goto p_err1;
	}

	regs_res = request_mem_region(mem_res->start, resource_size(mem_res), pdev->name);
	if (!regs_res) {
		probe_err("Failed to request io memory region(%p)", regs_res);
		goto p_err1;
	}

	core->regs_res = regs_res;
	core->regs =  ioremap_nocache(mem_res->start, resource_size(mem_res));
	if (!core->regs) {
		probe_err("Failed to remap io region(%p)", core->regs);
		goto p_err2;
	}
#else
	core->regs_res = NULL;
	core->regs = NULL;
#endif

#ifdef ENABLE_IS_CORE
	core->irq = platform_get_irq(pdev, 0);
	if (core->irq < 0) {
		probe_err("Failed to get irq(%d)", core->irq);
		goto p_err3;
	}
#endif

	if (pdata) {
		ret = pdata->clk_get(&pdev->dev);
		if (ret) {
			probe_err("clk_get is fail(%d)", ret);
			goto p_err3;
		}
	}

	dma_set_mask(&pdev->dev, DMA_BIT_MASK(36));

	ret = fimc_is_mem_init(&core->resourcemgr.mem, core->pdev);
	if (ret) {
		probe_err("fimc_is_mem_probe is fail(%d)", ret);
		goto p_err3;
	}

	ret = fimc_is_resourcemgr_probe(&core->resourcemgr, core, core->pdev);
	if (ret) {
		probe_err("fimc_is_resourcemgr_probe is fail(%d)", ret);
		goto p_err3;
	}

	ret = fimc_is_interface_probe(&core->interface,
		&core->resourcemgr.minfo,
		(ulong)core->regs,
		core->irq,
		core);
	if (ret) {
		probe_err("fimc_is_interface_probe is fail(%d)", ret);
		goto p_err3;
	}

	ret = fimc_is_debug_probe();
	if (ret) {
		probe_err("fimc_is_deubg_probe is fail(%d)", ret);
		goto p_err3;
	}

	ret = fimc_is_vender_probe(&core->vender);
	if (ret) {
		probe_err("fimc_is_vender_probe is fail(%d)", ret);
		goto p_err3;
	}

	/* group initialization */
	ret = fimc_is_groupmgr_probe(pdev, &core->groupmgr);
	if (ret) {
		probe_err("fimc_is_groupmgr_probe is fail(%d)", ret);
		goto p_err3;
	}

	ret = fimc_is_devicemgr_probe(&core->devicemgr);
	if (ret) {
		probe_err("fimc_is_devicemgr_probe is fail(%d)", ret);
		goto p_err3;
	}

	for (stream = 0; stream < FIMC_IS_STREAM_COUNT; ++stream) {
		ret = fimc_is_ischain_probe(&core->ischain[stream],
			&core->interface,
			&core->resourcemgr,
			&core->groupmgr,
			&core->devicemgr,
			&core->resourcemgr.mem,
			core->pdev,
			stream);
		if (ret) {
			probe_err("fimc_is_ischain_probe(%d) is fail(%d)", stream, ret);
			goto p_err3;
		}

#ifndef ENABLE_IS_CORE
		core->ischain[stream].hardware = &core->hardware;
#endif
	}

	ret = v4l2_device_register(&pdev->dev, &core->v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev, "failed to register fimc-is v4l2 device\n");
		goto p_err3;
	}

#ifdef SOC_30S
	/* video entity - 3a0 */
	fimc_is_30s_video_probe(core);
#endif

#ifdef SOC_30C
	/* video entity - 3a0 capture */
	fimc_is_30c_video_probe(core);
#endif

#ifdef SOC_30P
	/* video entity - 3a0 preview */
	fimc_is_30p_video_probe(core);
#endif

#ifdef SOC_30F
	/* video entity - 3a0 early-FD */
	fimc_is_30f_video_probe(core);
#endif

#ifdef SOC_30G
	/* video entity - 3a0 MRG_OUT */
	fimc_is_30g_video_probe(core);
#endif

#ifdef SOC_31S
	/* video entity - 3a1 */
	fimc_is_31s_video_probe(core);
#endif

#ifdef SOC_31C
	/* video entity - 3a1 capture */
	fimc_is_31c_video_probe(core);
#endif

#ifdef SOC_31P
	/* video entity - 3a1 preview */
	fimc_is_31p_video_probe(core);
#endif

#ifdef SOC_31F
	/* video entity - 3a1 early-FD */
	fimc_is_31f_video_probe(core);
#endif

#ifdef SOC_31G
	/* video entity - 3a1 MRG_OUT */
	fimc_is_31g_video_probe(core);
#endif

#ifdef SOC_32S
	/* video entity - 3a2 */
	fimc_is_32s_video_probe(core);
#endif

#ifdef SOC_32P
	/* video entity - 3a2 preview */
	fimc_is_32p_video_probe(core);
#endif

#ifdef SOC_I0S
	/* video entity - isp0 */
	fimc_is_i0s_video_probe(core);
#endif

#ifdef SOC_I0C
	/* video entity - isp0 capture */
	fimc_is_i0c_video_probe(core);
#endif

#ifdef SOC_I0P
	/* video entity - isp0 preview */
	fimc_is_i0p_video_probe(core);
#endif

#ifdef SOC_I1S
	/* video entity - isp1 */
	fimc_is_i1s_video_probe(core);
#endif

#ifdef SOC_I1C
	/* video entity - isp1 capture */
	fimc_is_i1c_video_probe(core);
#endif

#ifdef SOC_I1P
	/* video entity - isp1 preview */
	fimc_is_i1p_video_probe(core);
#endif

#ifdef SOC_ME0C
	/* video entity - me out */
	fimc_is_me0c_video_probe(core);
#endif

#ifdef SOC_ME1C
	/* video entity - me out */
	fimc_is_me1c_video_probe(core);
#endif

#if defined(SOC_DIS) || defined(SOC_D0S)
	/* video entity - tpu0 */
	fimc_is_d0s_video_probe(core);
#endif

#ifdef SOC_D0C
	/* video entity - tpu0 capture */
	fimc_is_d0c_video_probe(core);
#endif

#if defined(SOC_D1S)
	/* video entity - tpu1 */
	fimc_is_d1s_video_probe(core);
#endif

#ifdef SOC_D1C
	/* video entity - tpu1 capture */
	fimc_is_d1c_video_probe(core);
#endif

#ifdef SOC_DCP
	fimc_is_dcp0s_video_probe(core); /* dcp master */
	fimc_is_dcp1s_video_probe(core); /* dcp slave */
	fimc_is_dcp0c_video_probe(core); /* dcp master main capture */
	fimc_is_dcp1c_video_probe(core); /* dcp slave main capture */
	fimc_is_dcp2c_video_probe(core); /* dcp diparity capture */
	fimc_is_dcp3c_video_probe(core); /* dcp master sub capture */
	fimc_is_dcp4c_video_probe(core); /* dcp slave sub capture */
#endif

#ifdef SOC_SCC
	/* video entity - scc */
	fimc_is_scc_video_probe(core);
#endif

#ifdef SOC_SCP
	/* video entity - scp */
	fimc_is_scp_video_probe(core);
#endif

#ifdef SOC_MCS
	/* video entity - scp */
	fimc_is_m0s_video_probe(core);
	fimc_is_m1s_video_probe(core);
	fimc_is_m0p_video_probe(core);
	fimc_is_m1p_video_probe(core);
	fimc_is_m2p_video_probe(core);
	fimc_is_m3p_video_probe(core);
	fimc_is_m4p_video_probe(core);
	fimc_is_m5p_video_probe(core);
#endif

#ifdef SOC_VRA
	/* video entity - vra */
	fimc_is_vra_video_probe(core);
#endif

#ifdef SOC_PAF0
	/* video entity - paf_rdma0 */
	fimc_is_paf0s_video_probe(core);
#endif

#ifdef SOC_PAF1
	/* video entity - paf_rdma1 */
	fimc_is_paf1s_video_probe(core);
#endif

/* TODO: video probe is needed for DCP */

	platform_set_drvdata(pdev, core);

	mutex_init(&core->sensor_lock);

	/* CSIS common dma probe */
	ret = fimc_is_csi_dma_probe(&core->csi_dma, core->pdev);
	if (ret) {
		dev_err(&pdev->dev, "fimc_is_csi_dma_probe fail\n");
		goto p_err1;
	}

#ifndef ENABLE_IS_CORE
	ret = fimc_is_interface_ischain_probe(&core->interface_ischain,
		&core->hardware,
		&core->resourcemgr,
		core->pdev,
		(ulong)core->regs);
	if (ret) {
		dev_err(&pdev->dev, "interface_ischain_probe fail\n");
		goto p_err1;
	}

	ret = fimc_is_hardware_probe(&core->hardware, &core->interface, &core->interface_ischain);
	if (ret) {
		dev_err(&pdev->dev, "hardware_probe fail\n");
		goto p_err1;
	}

	/* set sysfs for set position to actuator */
	sysfs_actuator.init_step = 0;
	for (i = 0; i < INIT_MAX_SETTING; i++) {
		sysfs_actuator.init_positions[i] = -1;
		sysfs_actuator.init_delays[i] = -1;
	}
#ifdef FIXED_SENSOR_DEBUG
	sysfs_sensor.is_en = false;
	sysfs_sensor.frame_duration = FIXED_FPS_VALUE;
	sysfs_sensor.long_exposure_time = FIXED_EXPOSURE_VALUE;
	sysfs_sensor.short_exposure_time = FIXED_EXPOSURE_VALUE;
	sysfs_sensor.long_analog_gain = FIXED_AGAIN_VALUE;
	sysfs_sensor.short_analog_gain = FIXED_AGAIN_VALUE;
	sysfs_sensor.long_digital_gain = FIXED_DGAIN_VALUE;
	sysfs_sensor.short_digital_gain = FIXED_DGAIN_VALUE;
#endif
#endif

	EXYNOS_MIF_ADD_NOTIFIER(&exynos_fimc_is_mif_throttling_nb);

#if defined(SECURE_CAMERA_IRIS) || defined(SECURE_CAMERA_FACE)
	probe_info("%s: call SMC_SECCAM_SETENV, SECURE_CAMERA_CH(%#x), SECURE_CAMERA_HEAP_ID(%d)\n",
		__func__, SECURE_CAMERA_CH, SECURE_CAMERA_HEAP_ID);
#if defined(SECURE_CAMERA_FACE_SEQ_CHK)
	ret = 0;
#else	
	ret = exynos_smc(SMC_SECCAM_SETENV, SECURE_CAMERA_CH, SECURE_CAMERA_HEAP_ID, 0);
#endif
	if (ret) {
		dev_err(fimc_is_dev, "[SMC] SMC_SECCAM_SETENV fail(%d)\n", ret);
		goto p_err3;
	}
	
	mem_info_addr = core->secure_mem_info[0] ? core->secure_mem_info[0] : SECURE_CAMERA_MEM_ADDR;
	mem_info_size = core->secure_mem_info[1] ? core->secure_mem_info[1] : SECURE_CAMERA_MEM_SIZE;

	probe_info("%s: call SMC_SECCAM_INIT, mem_info(%#08lx, %#08lx)\n",
		__func__, mem_info_addr, mem_info_size);

#if defined(SECURE_CAMERA_FACE_SEQ_CHK)
	ret = 0;
#else	
	ret = exynos_smc(SMC_SECCAM_INIT, mem_info_addr, mem_info_size, 0);
#endif
	if (ret) {
		dev_err(fimc_is_dev, "[SMC] SMC_SECCAM_INIT fail(%d)\n", ret);
		goto p_err3;
	}
	mem_info_addr = core->non_secure_mem_info[0] ? core->non_secure_mem_info[0] : NON_SECURE_CAMERA_MEM_ADDR;
	mem_info_size = core->non_secure_mem_info[1] ? core->non_secure_mem_info[1] : NON_SECURE_CAMERA_MEM_SIZE;

	probe_info("%s: call SMC_SECCAM_INIT_NSBUF, mem_info(%#08lx, %#08lx)\n",
		__func__, mem_info_addr, mem_info_size);
#if defined(SECURE_CAMERA_FACE_SEQ_CHK)
	ret = 0;
#else
	ret = exynos_smc(SMC_SECCAM_INIT_NSBUF, mem_info_addr, mem_info_size, 0);
#endif
	if (ret) {
		dev_err(fimc_is_dev, "[SMC] SMC_SECCAM_INIT_NSBUF fail(%d)\n", ret);
		goto p_err3;
	}
#endif

#if defined(CONFIG_PM)
	pm_runtime_enable(&pdev->dev);
#endif

#ifdef ENABLE_FAULT_HANDLER
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0))
	exynos_sysmmu_set_fault_handler(fimc_is_dev, fimc_is_fault_handler);
#else
	iovmm_set_fault_handler(fimc_is_dev, fimc_is_fault_handler, NULL);
#endif
#endif

#if defined(USE_I2C_LOCK)
	for (channel = 0; channel < SENSOR_CONTROL_I2C_MAX; channel++) {
		mutex_init(&core->i2c_lock[channel]);
	}
#endif

	mutex_init(&core->ois_mode_lock);

	/* set sysfs for debuging */
	sysfs_debug.en_clk_gate = 0;
	sysfs_debug.en_dvfs = 1;
	sysfs_debug.hal_debug_mode = 0;
	sysfs_debug.hal_debug_delay = DBG_HAL_DEAD_PANIC_DELAY;
#ifdef ENABLE_CLOCK_GATE
	sysfs_debug.en_clk_gate = 1;
#ifdef HAS_FW_CLOCK_GATE
	sysfs_debug.clk_gate_mode = CLOCK_GATE_MODE_FW;
#else
	sysfs_debug.clk_gate_mode = CLOCK_GATE_MODE_HOST;
#endif
#endif

#ifdef ENABLE_DIRECT_CLOCK_GATE
	sysfs_debug.en_clk_gate = 1;
#endif
	sysfs_debug.pattern_en = 0;
	sysfs_debug.pattern_fps = 0;

	ret = sysfs_create_group(&core->pdev->dev.kobj, &fimc_is_debug_attr_group);

	s = pinctrl_lookup_state(pdata->pinctrl, "release");

	if (!IS_ERR(s)) {
		if (pinctrl_select_state(pdata->pinctrl, s) < 0) {
			probe_err("pinctrl_select_state is fail\n");
			goto p_err3;
		}
	} else {
		probe_err("pinctrl_lookup_state failed!!!\n");
	}

	core->shutdown = false;
	core->reboot = false;

	probe_info("%s:end\n", __func__);
	return 0;

p_err3:
	iounmap(core->regs);
#if defined (ENABLE_IS_CORE) || defined (USE_MCUCTL)
p_err2:
	release_mem_region(regs_res->start, resource_size(regs_res));
#endif
p_err1:
	kfree(core);
	return ret;
}

void fimc_is_cleanup(struct fimc_is_core *core)
{
	struct fimc_is_device_sensor *device;
	int ret = 0;
	u32 i;

	if (!core) {
		err("%s: core(NULL)", __func__);
		return;
	}

	core->reboot = true;
	info("%s++: shutdown(%d), reboot(%d)\n", __func__, core->shutdown, core->reboot);
	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		device = &core->sensor[i];
		if (!device) {
			err("%s: device(NULL)", __func__);
			continue;
		}

		if (test_bit(FIMC_IS_SENSOR_FRONT_START, &device->state)) {
			minfo("call sensor_front_stop()\n", device);

			ret = fimc_is_sensor_front_stop(device);
			if (ret)
				mwarn("fimc_is_sensor_front_stop() is fail(%d)", device, ret);
		}
	}
	info("%s:--\n", __func__);
}

static void fimc_is_shutdown(struct platform_device *pdev)
{
	struct fimc_is_core *core = (struct fimc_is_core *)platform_get_drvdata(pdev);
	struct fimc_is_device_sensor *device;
	struct v4l2_subdev *subdev;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri;
	u32 i;

	if (!core) {
		err("%s: core(NULL)", __func__);
		return;
	}

	core->shutdown = true;
	info("%s++: shutdown(%d), reboot(%d)\n", __func__, core->shutdown, core->reboot);
#if !defined(ENABLE_IS_CORE)
	fimc_is_cleanup(core);
#endif
	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		device = &core->sensor[i];
		if (!device) {
			warn("%s: device(NULL)", __func__);
			continue;
		}

		if (test_bit(FIMC_IS_SENSOR_OPEN, &device->state)) {
			subdev = device->subdev_module;
			if (!subdev) {
				warn("%s: subdev(NULL)", __func__);
				continue;
			}

			module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
			if (!module) {
				warn("%s: module(NULL)", __func__);
				continue;
			}

			sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;
			if (!sensor_peri) {
				warn("%s: sensor_peri(NULL)", __func__);
				continue;
			}

			fimc_is_sensor_deinit_sensor_thread(sensor_peri);
			fimc_is_sensor_deinit_mode_change_thread(sensor_peri);
		}
	}
	info("%s:--\n", __func__);
}

static const struct dev_pm_ops fimc_is_pm_ops = {
	.suspend		= fimc_is_suspend,
	.resume			= fimc_is_resume,
	.runtime_suspend	= fimc_is_ischain_runtime_suspend,
	.runtime_resume		= fimc_is_ischain_runtime_resume,
};

#ifdef CONFIG_OF
static const struct of_device_id exynos_fimc_is_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_match);

static struct platform_driver fimc_is_driver = {
	.shutdown	= fimc_is_shutdown,
	.driver = {
		.name	= FIMC_IS_DRV_NAME,
		.owner	= THIS_MODULE,
		.pm	= &fimc_is_pm_ops,
		.of_match_table = exynos_fimc_is_match,
	}
};

#else
static struct platform_driver fimc_is_driver = {
	.driver = {
		.name	= FIMC_IS_DRV_NAME,
		.owner	= THIS_MODULE,
		.pm	= &fimc_is_pm_ops,
	}
};
#endif
builtin_platform_driver_probe(fimc_is_driver, fimc_is_probe);

MODULE_AUTHOR("Gilyeon im<kilyeon.im@samsung.com>");
MODULE_DESCRIPTION("Exynos FIMC_IS2 driver");
MODULE_LICENSE("GPL");
