/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _NPU_SYSTEM_H_
#define _NPU_SYSTEM_H_

#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include "npu-profile.h"
#include "npu-qos.h"

#ifdef CONFIG_NPU_HARDWARE
#include "npu-interface.h"
#include "mailbox.h"
#endif

#include "npu-exynos.h"
#if 0
#include "npu-interface.h"
#endif

#include "npu-memory.h"

#include "npu-binary.h"
#if 0
#include "npu-tv.h"
#endif

#include "npu-fw-test-handler.h"

struct npu_system {
#if defined(CONFIG_SOC_EMULATOR9820) || defined(CONFIG_SOC_EXYNOS9820)
	struct platform_device	*pdev;

	struct npu_iomem_area	tcu_sram;
	struct npu_iomem_area	idp_sram;
	struct npu_iomem_area	sfr_npu0;
	struct npu_iomem_area	sfr_npu1;
	struct npu_iomem_area	pmu_npu;
	struct npu_iomem_area	pmu_npu_cpu;
#ifdef REINIT_NPU_BAAW
	struct npu_iomem_area	baaw_npu;
#endif
	struct npu_iomem_area	mbox_sfr;
	struct npu_iomem_area	pwm_npu;
	struct npu_fw_test_handler	npu_fw_test_handler;

	int			irq0;
	int			irq1;

	struct clk 		*npu_clk;
	u32			cam_qos;
	u32			mif_qos;
#endif

#ifdef CONFIG_PM_SLEEP
	/* maintain to be awake */
	struct wake_lock 	npu_wake_lock;
#endif

	struct npu_exynos exynos;
	struct npu_memory memory;
	struct npu_binary binary;
#ifdef CONFIG_NPU_HARDWARE
	volatile struct mailbox_hdr	*mbox_hdr;
	volatile struct npu_interface	*interface;
#endif
	struct npu_profile_control	profile_ctl;
	struct npu_qos_setting		qos_setting;

	/* Open status (Bitfield of npu_system_resume_steps) */
	unsigned long			resume_steps;
	unsigned long			resume_soc_steps;
};

int npu_system_probe(struct npu_system *system, struct platform_device *pdev);
int npu_system_release(struct npu_system *system, struct platform_device *pdev);
int npu_system_open(struct npu_system *system);
int npu_system_close(struct npu_system *system);
int npu_system_resume(struct npu_system *system, u32 mode);
int npu_system_suspend(struct npu_system *system);
int npu_system_start(struct npu_system *system);
int npu_system_stop(struct npu_system *system);
#endif
