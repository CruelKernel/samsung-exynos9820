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


#include <linux/pm_qos.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/clk-provider.h>

#include <linux/io.h>
#include <linux/delay.h>

#include "npu-log.h"
#include "npu-config.h"
#include "npu-common.h"
#include "npu-device.h"
#include "npu-system.h"
#include "npu-util-regs.h"
#include "npu-system-soc.h"

#define OFFSET_END	0xFFFFFFFF

/* Initialzation steps for system_resume */
enum npu_system_resume_soc_steps {
	NPU_SYS_RESUME_SOC_CPU_ON,
	NPU_SYS_RESUME_SOC_CLKGATE23,
	NPU_SYS_RESUME_SOC_CLKGATE1,
	NPU_SYS_RESUME_SOC_CLKGATE4,
	NPU_SYS_RESUME_SOC_CLKGATE5,
	NPU_SYS_RESUME_SOC_COMPLETED
};

static int init_baaw_p_npu(struct npu_system *system);
static int npu_cpu_on(struct npu_system *system);
static int npu_cpu_off(struct npu_system *system);
static int init_iomem_area(struct npu_system *system);

#ifdef CLKGate23_SOC_HWACG
static __attribute__((unused)) int CLKGate23_SOC_HWACG_read(struct npu_system *system)
{
	size_t i = 0;

	void __iomem *reg_addr;
	volatile u32 v;
	struct npu_iomem_area *base;
	u32 offset;

	const struct reg_set_map_2 CLKGate23_HWACG_regs[] = {
		{&system->sfr_npu0,	0x800,	0x00,	0x20000000},	/* NPU0_CMU_NPU0_CONTROLLER_OPTION */
		{&system->sfr_npu1,	0x800,	0x00,	0x20000000},	/* NPU1_CMU_NPU1_CONTROLLER_OPTION */
		{&system->sfr_npu0,	0x30c0,	0x06,	0x07},	/* NPU0_CPU_CONFIGURATION */
		{&system->sfr_npu1,	0x307c,	0x06,	0x07},	/* NPU0_CPU_OUT */
	};

	for (i = 0; i < ARRAY_SIZE(CLKGate23_HWACG_regs); i++) {
		offset = CLKGate23_HWACG_regs[i].offset;
		base = CLKGate23_HWACG_regs[i].iomem_area;
		reg_addr = base->vaddr + offset;
		npu_info("v, reg_addr(0x%pK)\n", reg_addr);

		v = readl(reg_addr) & 0xffffffff;
		npu_info("read cpu_on_regs, v(0x%x)\n", v);
	}

	return 0;
}

static void CLKGate23_SOC_HWACG_qch_disable(struct npu_system *system)
{
	void __iomem *reg_addr;
	volatile u32 v;
	struct npu_iomem_area *base;
	u32 offset;
	u32 mask;
	u32 val;
	size_t i = 0;

	const struct reg_set_map_2 CLKGate23_HWACG_regs[] = {
		{&system->sfr_npu0,	0x30c0,	0x06,	0x07},	/* NPU0_CPU_CONFIGURATION */
		{&system->sfr_npu1,	0x307c,	0x06,	0x07},	/* NPU0_CPU_OUT */
	};

	for (i = 0; i < ARRAY_SIZE(CLKGate23_HWACG_regs); i++) {
		base = CLKGate23_HWACG_regs[i].iomem_area;
		offset = CLKGate23_HWACG_regs[i].offset;
		val = CLKGate23_HWACG_regs[i].val;
		mask = CLKGate23_HWACG_regs[i].mask;

		reg_addr = base->vaddr + offset;
		npu_dbg("qch_on, v, reg_addr(0x%pK)\n", reg_addr);

		v = readl(reg_addr) & 0xffffffff;
		npu_dbg("qch_on,  read cpu_on_regs, v(0x%x)\n", v);

		v = (v & (~mask)) | (val & mask);
		writel(v, (void *)(reg_addr));
		npu_dbg("written on, (0x%08x) at (%pK)\n", v, reg_addr);
	}
}

static void CLKGate23_SOC_HWACG_qch_enable(struct npu_system *system)
{
	void __iomem *reg_addr;
	volatile u32 v;
	struct npu_iomem_area *base;
	u32 offset;
	u32 mask;
	u32 val;
	size_t i = 0;

	const struct reg_set_map_2 CLKGate23_HWACG_regs[] = {
		{&system->sfr_npu0,	0x30c0,	0x02,	0x07},	/* NPU0_CPU_CONFIGURATION */
		{&system->sfr_npu1,	0x307c,	0x02,	0x07},	/* NPU0_CPU_OUT */
	};

	for (i = 0; i < ARRAY_SIZE(CLKGate23_HWACG_regs); i++) {
		base = CLKGate23_HWACG_regs[i].iomem_area;
		offset = CLKGate23_HWACG_regs[i].offset;
		val = CLKGate23_HWACG_regs[i].val;
		mask = CLKGate23_HWACG_regs[i].mask;

		reg_addr = base->vaddr + offset;
		npu_dbg("qch clk off, reg_addr(0x%pK)\n", reg_addr);

		v = readl(reg_addr) & 0xffffffff;
		npu_dbg("qch clk off, read cpu_on_regs, val(0x%x)\n", v);

		v = (v & (~mask)) | (val & mask);
		writel(v, (void *)(reg_addr));
		npu_dbg("written off (0x%08x) at (%pK)\n", v, reg_addr);
	}
}
#endif

#ifdef CLKGate1_DRCG_EN
static int CLKGate1_DRCG_EN_read(void)
{
	int ret = 0;

	npu_info("start CLKGate1_DRCG_EN_read\n");
	npu_get_sfr(0x17910104);
	npu_get_sfr(0x17910404);
	npu_get_sfr(0x17A10104);
	return ret;
}

static int CLKGate1_DRCG_EN_write_disable(void)
{
	int ret = 0;

	npu_info("start CLKGate1_DRCG_EN_write_disable\n");
	npu_set_sfr(0x17910104, 0x00, 0xffffffff);
	npu_set_sfr(0x17910404, 0x00, 0xffffffff);
	npu_set_sfr(0x17A10104, 0x00, 0xffffffff);
	return ret;
}
#endif

#ifdef CLKGate4_IP_HWACG
static __attribute__((unused)) int CLKGate4_IP_HWACG_read(void)
{
	int ret = 0;

	npu_info("start CLKGate4_IP_HWACG_read\n");
	npu_get_sfr(0x179f45ec);
	npu_get_sfr(0x179f45f0);
	npu_get_sfr(0x17af45ec);
	npu_get_sfr(0x17af45f0);
	npu_get_sfr(0x179f4dec);
	npu_get_sfr(0x179f4df0);
	npu_get_sfr(0x17af4dec);
	npu_get_sfr(0x17af4df0);
	npu_get_sfr(0x179f40ec);
	npu_get_sfr(0x179f40f0);
	npu_get_sfr(0x17af40ec);
	npu_get_sfr(0x17af40f0);
	npu_get_sfr(0x179f48ec);
	npu_get_sfr(0x179f48f0);
	npu_get_sfr(0x17af48ec);
	npu_get_sfr(0x17af48f0);
	npu_get_sfr(0x179f58ec);
	npu_get_sfr(0x179f58f0);
	npu_get_sfr(0x17af58ec);
	npu_get_sfr(0x17af58f0);
	npu_get_sfr(0x179f5cec);
	npu_get_sfr(0x179f5cf0);
	npu_get_sfr(0x17af5cec);
	npu_get_sfr(0x17af5cf0);
	npu_get_sfr(0x179fc0ec);
	npu_get_sfr(0x179fc0f0);
	npu_get_sfr(0x17afc0ec);
	npu_get_sfr(0x17afc0f0);
	npu_get_sfr(0x179f50ec);
	npu_get_sfr(0x179f50f0);
	npu_get_sfr(0x17af50ec);
	npu_get_sfr(0x17af50f0);
	npu_get_sfr(0x179f54ec);
	npu_get_sfr(0x179f54f0);
	npu_get_sfr(0x17af54ec);
	npu_get_sfr(0x17af54f0);
	return ret;
}

static __attribute__((unused)) int CLKGate4_IP_HWACG_write_disable(void)
{
	int ret = 0;

	npu_info("start CLKGate4_IP_HWACG_write_disable\n");
	npu_set_sfr(0x179f45ec, 0x00, 0xffffffff);
	npu_set_sfr(0x179f45f0, 0x00, 0xffffffff);
	npu_set_sfr(0x17af45ec, 0x00, 0xffffffff);
	npu_set_sfr(0x17af45f0, 0x00, 0xffffffff);
	npu_set_sfr(0x179f4dec, 0x00, 0xffffffff);
	npu_set_sfr(0x179f4df0, 0x00, 0xffffffff);
	npu_set_sfr(0x17af4dec, 0x00, 0xffffffff);
	npu_set_sfr(0x17af4df0, 0x00, 0xffffffff);
	npu_set_sfr(0x179f40ec, 0x00, 0xffffffff);
	npu_set_sfr(0x179f40f0, 0x00, 0xffffffff);
	npu_set_sfr(0x17af40ec, 0x00, 0xffffffff);
	npu_set_sfr(0x17af40f0, 0x00, 0xffffffff);
	npu_set_sfr(0x179f48ec, 0x00, 0xffffffff);
	npu_set_sfr(0x179f48f0, 0x00, 0xffffffff);
	npu_set_sfr(0x17af48ec, 0x00, 0xffffffff);
	npu_set_sfr(0x17af48f0, 0x00, 0xffffffff);
	npu_set_sfr(0x179f58ec, 0x00, 0xffffffff);
	npu_set_sfr(0x179f58f0, 0x00, 0xffffffff);
	npu_set_sfr(0x17af58ec, 0x00, 0xffffffff);
	npu_set_sfr(0x17af58f0, 0x00, 0xffffffff);
	npu_set_sfr(0x179f5cec, 0x00, 0xffffffff);
	npu_set_sfr(0x179f5cf0, 0x00, 0xffffffff);
	npu_set_sfr(0x17af5cec, 0x00, 0xffffffff);
	npu_set_sfr(0x17af5cf0, 0x00, 0xffffffff);
	npu_set_sfr(0x179fc0ec, 0x00, 0xffffffff);
	npu_set_sfr(0x179fc0f0, 0x00, 0xffffffff);
	npu_set_sfr(0x17afc0ec, 0x00, 0xffffffff);
	npu_set_sfr(0x17afc0f0, 0x00, 0xffffffff);
	npu_set_sfr(0x179f50ec, 0x00, 0xffffffff);
	npu_set_sfr(0x179f50f0, 0x00, 0xffffffff);
	npu_set_sfr(0x17af50ec, 0x00, 0xffffffff);
	npu_set_sfr(0x17af50f0, 0x00, 0xffffffff);
	npu_set_sfr(0x179f54ec, 0x00, 0xffffffff);
	npu_set_sfr(0x179f54f0, 0x00, 0xffffffff);
	npu_set_sfr(0x17af54ec, 0x00, 0xffffffff);
	npu_set_sfr(0x17af54f0, 0x00, 0xffffffff);
	return ret;
}

static int CLKGate4_IP_HWACG_qch_disable(void)
{
	int ret = 0;

	npu_info("start CLKGate4_IP_HWACG_qch_disable\n");
	npu_set_sfr(0x179f45ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x179f45f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x17af45ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x17af45f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x179f4dec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x179f4df0, 0x200000, 0xffffffff);
	npu_set_sfr(0x17af4dec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x17af4df0, 0x200000, 0xffffffff);
	npu_set_sfr(0x179f40ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x179f40f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x17af40ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x17af40f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x179f48ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x179f48f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x17af48ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x17af48f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x179f58ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x179f58f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x17af58ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x17af58f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x179f5cec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x179f5cf0, 0x200000, 0xffffffff);
	npu_set_sfr(0x17af5cec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x17af5cf0, 0x200000, 0xffffffff);
	npu_set_sfr(0x179fc0ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x179fc0f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x17afc0ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x17afc0f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x179f50ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x179f50f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x17af50ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x17af50f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x179f54ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x179f54f0, 0x200000, 0xffffffff);
	npu_set_sfr(0x17af54ec, 0x3f0002, 0xffffffff);
	npu_set_sfr(0x17af54f0, 0x200000, 0xffffffff);
	return ret;
}

static int CLKGate4_IP_HWACG_qch_enable(void)
{
	int ret = 0;

	npu_info("start CLKGate4_IP_HWACG_qch_enable\n");
	npu_set_sfr(0x179f45ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x179f45f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x17af45ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x17af45f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x179f4dec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x179f4df0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x17af4dec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x17af4df0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x179f40ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x179f40f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x17af40ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x17af40f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x179f48ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x179f48f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x17af48ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x17af48f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x179f58ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x179f58f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x17af58ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x17af58f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x179f5cec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x179f5cf0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x17af5cec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x17af5cf0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x179fc0ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x179fc0f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x17afc0ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x17afc0f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x179f50ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x179f50f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x17af50ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x17af50f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x179f54ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x179f54f0, 0x10000000, 0xffffffff);
	npu_set_sfr(0x17af54ec, 0x3f0003, 0xffffffff);
	npu_set_sfr(0x17af54f0, 0x10000000, 0xffffffff);
	return ret;
}
#endif

#ifdef CLKGate5_IP_DRCG_EN
static int CLKGate5_IP_DRCG_EN_read(void)
{
	int ret = 0;

	npu_info("start CLKGate5_IP_DRCG_EN_read\n");
	npu_get_sfr(0x179f45fc);
	npu_get_sfr(0x17af45fc);
	npu_get_sfr(0x179f4dfc);
	npu_get_sfr(0x17af4dfc);
	npu_get_sfr(0x179f40fc);
	npu_get_sfr(0x17af40fc);
	npu_get_sfr(0x179f48fc);
	npu_get_sfr(0x17af48fc);
	npu_get_sfr(0x179f58fc);
	npu_get_sfr(0x17af58fc);
	npu_get_sfr(0x179f5cfc);
	npu_get_sfr(0x17af5cfc);
	npu_get_sfr(0x179fc0fc);
	npu_get_sfr(0x17afc0fc);
	npu_get_sfr(0x179f50fc);
	npu_get_sfr(0x17af50fc);
	npu_get_sfr(0x179f54fc);
	npu_get_sfr(0x17af54fc);
	npu_get_sfr(0x179f80fc);
	npu_get_sfr(0x17af80fc);
	npu_get_sfr(0x179f84fc);
	npu_get_sfr(0x17af84fc);
	npu_get_sfr(0x179f88fc);
	npu_get_sfr(0x17af88fc);
	npu_get_sfr(0x179f8cfc);
	npu_get_sfr(0x17af8cfc);
	npu_get_sfr(0x179f90fc);
	npu_get_sfr(0x17af90fc);
	npu_get_sfr(0x179f94fc);
	npu_get_sfr(0x17af94fc);
	npu_get_sfr(0x179f98fc);
	npu_get_sfr(0x17af98fc);
	npu_get_sfr(0x179f9cfc);
	npu_get_sfr(0x17af9cfc);
	return ret;
}

static int CLKGate5_IP_DRCG_EN_write_disable(void)
{
	int ret = 0;

	npu_info("start CLKGate5_IP_DRCG_EN_write_disable\n");
	npu_set_sfr(0x179f45fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af45fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f4dfc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af4dfc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f40fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af40fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f48fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af48fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f58fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af58fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f5cfc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af5cfc, 0x00, 0xffffffff);
	npu_set_sfr(0x179fc0fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17afc0fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f50fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af50fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f54fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af54fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f80fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af80fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f84fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af84fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f88fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af88fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f8cfc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af8cfc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f90fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af90fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f94fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af94fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f98fc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af98fc, 0x00, 0xffffffff);
	npu_set_sfr(0x179f9cfc, 0x00, 0xffffffff);
	npu_set_sfr(0x17af9cfc, 0x00, 0xffffffff);
	return ret;
}
#endif

int npu_system_soc_probe(struct npu_system *system, struct platform_device *pdev)
{
	int ret = 0;

	BUG_ON(!system);

	npu_dbg("system soc probe: ioremap areas\n");
	ret = init_iomem_area(system);
	if (ret) {
		probe_err("fail(%d) in init_iomem_area\n", ret);
		goto p_err;
	}

	npu_dbg("system probe: Set BAAW for SRAM area\n");
	ret = init_baaw_p_npu(system);
	if (ret) {
		probe_err("fail(%d)in init_baaw_p_npu\n", ret);
		goto p_err;
	}

	return 0;
p_err:
	return ret;
}

int npu_system_soc_release(struct npu_system *system, struct platform_device *pdev)
{
	return 0;
}

static inline void print_iomem_area(const char *pr_name, const struct npu_iomem_area *mem_area)
{
	npu_info(KERN_CONT "(%8s) Phy(0x%08x)-(0x%08llx) Virt(%pK) Size(%llu)\n",
		pr_name, mem_area->paddr, mem_area->paddr + mem_area->size,
		mem_area->vaddr, mem_area->size);
}

static void print_all_iomem_area(const struct npu_system *system)
{
	npu_dbg("start in IOMEM mapping\n");
	print_iomem_area("TCU_SRAM", &system->tcu_sram);
	print_iomem_area("IDP_SRAM", &system->idp_sram);
	print_iomem_area("SFR_NPU0", &system->sfr_npu0);
	print_iomem_area("SFR_NPU1", &system->sfr_npu1);
	print_iomem_area("PMU_NPU", &system->pmu_npu);
	print_iomem_area("PMU_NCPU", &system->pmu_npu_cpu);
#ifdef REINIT_NPU_BAAW
	print_iomem_area("NPU_BAAW", &system->baaw_npu);
#endif
	print_iomem_area("MBOX_SFR", &system->mbox_sfr);
	print_iomem_area("PWM_NPU", &system->pwm_npu);
	npu_dbg("end in IOMEM mapping\n");
}

int npu_system_soc_resume(struct npu_system *system, u32 mode)
{
	int ret = 0;

	BUG_ON(!system);

	/* Clear resume steps */
	system->resume_soc_steps = 0;

	print_all_iomem_area(system);

	ret = npu_cpu_on(system);
	if (ret) {
		npu_err("fail(%d) in npu_cpu_on\n", ret);
		goto p_err;
	}
	set_bit(NPU_SYS_RESUME_SOC_CPU_ON, &system->resume_soc_steps);

#ifdef CLKGate23_SOC_HWACG
	CLKGate23_SOC_HWACG_qch_disable(system);
	set_bit(NPU_SYS_RESUME_SOC_CLKGATE23, &system->resume_soc_steps);
#endif

#ifdef CLKGate1_DRCG_EN
	CLKGate1_DRCG_EN_write_disable();
	set_bit(NPU_SYS_RESUME_SOC_CLKGATE1, &system->resume_soc_steps);
#else
	npu_info("CLKGate1_DRCG_EN_write_enable\n");
#endif

#ifdef CLKGate4_IP_HWACG
	CLKGate4_IP_HWACG_qch_disable();
	set_bit(NPU_SYS_RESUME_SOC_CLKGATE4, &system->resume_soc_steps);
#endif

#ifdef CLKGate5_IP_DRCG_EN
	CLKGate5_IP_DRCG_EN_write_disable();
	set_bit(NPU_SYS_RESUME_SOC_CLKGATE5, &system->resume_soc_steps);
#else
	npu_info("CLKGate5_IP_DRCG_EN_write_enable\n");
#endif
	set_bit(NPU_SYS_RESUME_SOC_COMPLETED, &system->resume_soc_steps);

	return ret;
p_err:
	npu_err("Failure detected[%d].\n", ret);
	return ret;
}

int npu_system_soc_suspend(struct npu_system *system)
{
	int ret = 0;

	BUG_ON(!system);

	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_SOC_COMPLETED, &system->resume_soc_steps, NULL, ;);

#ifdef CLKGate23_SOC_HWACG
	BIT_CHECK_AND_EXECUTE(
		NPU_SYS_RESUME_SOC_CLKGATE23,
		&system->resume_soc_steps,
		"CLKGate23_SOC_HWACG_qch_enable", {
		CLKGate23_SOC_HWACG_qch_enable(system);
	});
#endif

#ifdef CLKGate1_DRCG_EN
	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_SOC_CLKGATE1, &system->resume_soc_steps, NULL, ;);
#endif

#ifdef CLKGate4_IP_HWACG
	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_SOC_CLKGATE4, &system->resume_soc_steps, "CLKGate4_IP_HWACG_qch_enable", {
		CLKGate4_IP_HWACG_qch_enable();
	});
#endif

#ifdef CLKGate5_IP_DRCG_EN
	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_SOC_CLKGATE5, &system->resume_soc_steps, NULL, ;);
#endif

	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_SOC_CPU_ON, &system->resume_soc_steps, "Turn NPU cpu off", {
		ret = npu_cpu_off(system);
		if (ret)
			npu_err("fail(%d) in npu_cpu_off\n", ret);
	});

	if (system->resume_soc_steps != 0)
		npu_warn("Missing clean-up steps [%lu] found.\n", system->resume_soc_steps);

	/* Function itself never be failed, even thought there was some error */
	return ret;
}

#ifdef REINIT_NPU_BAAW

/* Register initialization for BAAW_P_NPU */
const u32 BAAW_P_NPU_BASE = 0x1A350000;
const u32 BAAW_P_NPU_LEN = 0x60;

static int init_baaw_p_npu(struct npu_system *system)
{
	int ret;

	const static struct reg_set_map baaw_init[] = {
		{0x00, 0x017B00, 0xFFFFFF},
		{0x04, 0x017B40, 0xFFFFFF},
		{0x08, 0x010000, 0xFFFFFF},
		{0x0C, 0x80000003, 0x80000003},
		{0x10, 0x017B40, 0xFFFFFF},
		{0x14, 0x017B80, 0xFFFFFF},
		{0x18, 0x010100, 0xFFFFFF},
		{0x1C, 0x80000003, 0x80000003},
		{0x20, 0x017B80, 0xFFFFFF},
		{0x24, 0x017BC0, 0xFFFFFF},
		{0x28, 0x010200, 0xFFFFFF},
		{0x2C, 0x80000003, 0x80000003},
		{0x30, 0x017BC0, 0xFFFFFF},
		{0x34, 0x017C00, 0xFFFFFF},
		{0x38, 0x010300, 0xFFFFFF},
		{0x3C, 0x80000003, 0x80000003},
		{0x40, 0x017800, 0xFFFFFF},
		{0x44, 0x017A00, 0xFFFFFF},
		{0x48, 0x000400, 0xFFFFFF},
		{0x4C, 0x80000003, 0x80000003},
		{0x50, 0x017A00, 0xFFFFFF},
		{0x54, 0x017A80, 0xFFFFFF},
		{0x58, 0x000000, 0xFFFFFF},
		{0x5C, 0x80000003, 0x80000003},
	};

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	npu_info("start in BAAW initialization\n");

	npu_dbg("update in BAAW initialization\n");
	ret = npu_set_hw_reg(&system->baaw_npu, baaw_init, ARRAY_SIZE(baaw_init), 0);
	if (ret) {
		npu_err("fail(%) in npu_set_hw_reg(baaw_init)\n", ret);
		goto err_exit;
	}
	npu_info("complete in BAAW initialization\n");
	return 0;

err_exit:
	npu_err("error(%d) in BAAW initialization\n", ret);
	return ret;
}
#else
/* Do nothing */
static int init_baaw_p_npu(struct npu_system *system) {return 0; }

#endif

static __attribute__((unused)) int npu_awwl_checker_en(struct npu_system *system)
{
	int ret;
	const struct reg_set_map awwl_checker_en[] = {
		/* NPU0 */
		{0x10424, 0x02, 0x02},	/* CM7_ETC */
	};

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	npu_info("start in npu_awwl_checker_en\n");

	npu_dbg("set SFR in npu_awwl_checker_en\n");
	ret = npu_set_hw_reg(&system->sfr_npu0, awwl_checker_en, ARRAY_SIZE(awwl_checker_en), 0);
	if (ret) {
		npu_err("fail(%d) in npu_set_hw_reg(awwl_checker_en)\n", ret);
		goto err_exit;
	}
	npu_info("complete in npu_awwl_checker_en\n");
	return 0;

err_exit:
	npu_err("error(%d) in npu_awwl_checker_en\n", ret);
	return ret;
}

static __attribute__((unused)) int npu_clk_init(struct npu_system *system)
{
	int ret;
	const struct reg_set_map clk_on_npu0[] = {
		/* NPU0 */
		{0x1804, 0x00, 0xFFFFFFFF},	/* CLK_CON_DIV_DIV_CLK_NPU0_CPU */
		{0x1800, 0x03, 0xFFFFFFFF},	/* CLK_CON_DIV_DIV_CLK_NPU0_BUSP */
		{0x0120, 0x10, 0xFFFFFFFF},	/* PLL_CON0_MUX_CLKCMU_NPU0_CPU_USER */
		{0x0100, 0x10, 0xFFFFFFFF},	/* PLL_CON0_MUX_CLKCMU_NPU0_BUS_USER */
	};
	const struct reg_set_map clk_on_npu1[] = {
		{0x1800, 0x01, 0xFFFFFFFF},	/* CLK_CON_DIV_DIV_CLK_NPU1_BUSP */
		{0x0100, 0x10, 0xFFFFFFFF},	/* PLL_CON0_MUX_CLKCMU_NPU1_BUS_USER */
	};

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	npu_info("start in npu_clk_init\n");

	npu_dbg("initialize clock NPU0 in npu_clk_init\n");
	ret = npu_set_hw_reg(&system->sfr_npu0, clk_on_npu0, ARRAY_SIZE(clk_on_npu0), 0);
	if (ret) {
		npu_err("fail(%d) in npu_set_hw_reg(clk_on_npu0)\n", ret);
		goto err_exit;
	}

	npu_dbg("Initialize clock NPU1 in npu_clk_init\n");
	ret = npu_set_hw_reg(&system->sfr_npu1, clk_on_npu1, ARRAY_SIZE(clk_on_npu1), 0);
	if (ret) {
		npu_err("fail(%d) in npu_set_hw_reg(clk_on_npu1)\n", ret);
		goto err_exit;
	}
	npu_info("complete in npu_clk_init\n");
	return 0;

err_exit:
	npu_err("error(%d) in npu_clk_init\n", ret);
	return ret;
}

static int npu_cpu_on(struct npu_system *system)
{
	int ret;
	const struct reg_set_map_2 cpu_on_regs[] = {
		/* NPU0_CM7_CFG1, SysTick Callibration, use external OSC, 26MHz */
		{&system->sfr_npu0,	0x1040c,	0x3f79f,	0xffffffff},
#ifdef FORCE_HWACG_DISABLE
		{&system->sfr_npu0,	0x800,	0x00,	0x30000000},	/* NPU0_CMU_NPU0_CONTROLLER_OPTION */
		{&system->sfr_npu1,	0x800,	0x00,	0x30000000},	/* NPU1_CMU_NPU1_CONTROLLER_OPTION */
#endif
		{&system->pmu_npu_cpu,	0x00,	0x01,	0x01},	/* NPU0_CPU_CONFIGURATION */
#ifdef NPU_CM7_RELEASE_HACK
		{&system->pmu_npu_cpu,	0x20,	0x01,	0x01},	/* NPU0_CPU_OUT */
#endif
#ifdef FORCE_WDT_DISABLE
		{&system->pmu_npu_cpu,	0x20,	0x00,	0x02},	/* NPU0_CPU_OUT */
#else
		{&system->pmu_npu_cpu,  0x20,   0x02,   0x02},  /* NPU0_CPU_OUT */
#endif
	};

	npu_info("start in npu_cpu_on\n");
	ret = npu_set_hw_reg_2(cpu_on_regs, ARRAY_SIZE(cpu_on_regs), 0);
	if (ret) {
		npu_err("fail(%d) in npu_set_hw_reg_2(cpu_on_regs)\n", ret);
		goto err_exit;
	}

	npu_info("complete in npu_cpu_on\n");
	return 0;
err_exit:
	npu_err("error(%d) in npu_cpu_on\n", ret);
	return ret;
}

static int npu_cpu_off(struct npu_system *system)
{
	int ret;

	const struct reg_set_map_2 cpu_off_regs[] = {
#ifdef FORCE_HWACG_DISABLE
		{&system->sfr_npu0,	0x800,	0x30000000,	0x30000000},	/* NPU0_CMU_NPU0_CONTROLLER_OPTION */
		{&system->sfr_npu1,	0x800,	0x30000000,	0x30000000},	/* NPU1_CMU_NPU1_CONTROLLER_OPTION */
#endif
#ifdef NPU_CM7_RELEASE_HACK
		{&system->pmu_npu_cpu,  0x20,	0x00,	0x01},	/* NPU0_CPU_OUT */
#endif
		{&system->pmu_npu_cpu,  0x00,	0x00,	0x01},	/* NPU0_CPU_CONFIGURATION */
	};

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	npu_info("start in npu_cpu_off\n");
	ret = npu_set_hw_reg_2(cpu_off_regs, ARRAY_SIZE(cpu_off_regs), 0);
	if (ret) {
		npu_err("fail(%d) in npu_set_hw_reg2(cpu_off_regs)\n", ret);
		goto err_exit;
	}
	npu_info("complete in npu_cpu_off\n");
	return 0;
err_exit:
	npu_err("error(%d) in npu_cpu_off\n", ret);
	return ret;
}

#define TIMEOUT_ITERATION	100
static __attribute__((unused)) int npu_pwr_on(struct npu_system *system)
{
	int ret = 0;
	int check_pass;
	u32 v;
	size_t i;
	int j;
	void __iomem *reg_addr;

	const static struct reg_set_map pwr_on_regs[] = {
		{0x00,	0x01,	0x01},		/* NPU0_CONFIGURATION */
		{0x80,	0x01,	0x01},		/* NPU1_CONFIGURATION */
	};
	const static struct reg_set_map pwr_status_regs[] = {
		{0x04,	0x01,	0x01},		/* NPU0_STATUS */
		{0x84,	0x01,	0x01},		/* NPU1_STATUS */
	};

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	npu_info("start in npu_pwr_on\n");

	npu_dbg("set power-on NPU in npu_pwr_on\n");
	ret = npu_set_hw_reg(&system->pmu_npu, pwr_on_regs, ARRAY_SIZE(pwr_on_regs), 0);
	if (ret) {
		npu_err("fail(%d) in npu_set_hw_reg(pwr_on_regs)\n", ret);
		goto err_exit;
	}

	/* Check status */
	npu_dbg("check status in npu_pwr_on\n");
	for (j = 0, check_pass = 0; j < TIMEOUT_ITERATION && !check_pass; j++) {
		check_pass = 1;
		for (i = 0; i < ARRAY_SIZE(pwr_status_regs); ++i) {
			// Check status flag
			reg_addr = system->pmu_npu.vaddr + pwr_status_regs[i].offset;
			v = readl(reg_addr);
			if ((v & pwr_status_regs[i].mask) != pwr_status_regs[i].val)
				check_pass = 0;

			npu_dbg("get status NPU[%zu]=0x%08x\n", i, v);
		}
		mdelay(10);
	}

	/* Timeout check */
	if (j >= TIMEOUT_ITERATION) {
		ret = -ETIMEDOUT;
		goto err_exit;
	}
	npu_info("complete in npu_pwr_on\n");
	return 0;

err_exit:
	npu_err("error(%d) in npu_pwr_on\n", ret);
	return ret;

}

static int init_iomem_area(struct npu_system *system)
{
	const struct npu_iomem_init_data init_data[] = {
		{NPU_IOMEM_TCUSRAM_START,	NPU_IOMEM_TCUSRAM_END,		&system->tcu_sram},
		{NPU_IOMEM_IDPSRAM_START,	NPU_IOMEM_IDPSRAM_END,		&system->idp_sram},
		{NPU_IOMEM_SFR_NPU0_START,	NPU_IOMEM_SFR_NPU0_END,		&system->sfr_npu0},
		{NPU_IOMEM_SFR_NPU1_START,	NPU_IOMEM_SFR_NPU1_END,		&system->sfr_npu1},
		{NPU_IOMEM_PMU_NPU_START,	NPU_IOMEM_PMU_NPU_END,		&system->pmu_npu},
		{NPU_IOMEM_PMU_NPU_CPU_START,	NPU_IOMEM_PMU_NPU_CPU_END,	&system->pmu_npu_cpu},
#ifdef REINIT_NPU_BAAW
		{NPU_IOMEM_BAAW_NPU_START,	NPU_IOMEM_BAAW_NPU_END,		&system->baaw_npu},
#endif
		{NPU_IOMEM_MBOX_SFR_START,	NPU_IOMEM_MBOX_SFR_END,		&system->mbox_sfr},
		{NPU_IOMEM_PWM_START,		NPU_IOMEM_PWM_END,		&system->pwm_npu},
		{OFFSET_END, OFFSET_END, NULL}
	};
	int i;
	int ret;
	void __iomem *iomem;
	u32 size;

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	probe_trace("start in iomem area.\n");

	for (i = 0; init_data[i].start != OFFSET_END; ++i) {
		size = init_data[i].end - init_data[i].start;
		iomem = devm_ioremap_nocache(&(system->pdev->dev), init_data[i].start, size);
		if (IS_ERR_OR_NULL(iomem)) {
			probe_err("fail(%pK) in devm_ioremap_nocache(0x%08x, %u)\n",
				  iomem, init_data[i].start, size);
			ret = -EFAULT;
			goto err_exit;
		}
		init_data[i].area_info->vaddr = iomem;
		init_data[i].area_info->paddr = init_data[i].start;
		init_data[i].area_info->size = size;
		probe_trace("Paddr[%08x]-[%08x] => Mapped @[%pK], Length = %llu\n",
			   init_data[i].start, init_data[i].end,
			   init_data[i].area_info->vaddr, init_data[i].area_info->size);
	}
	probe_trace("complete in init_iomem_area\n");
	return 0;
err_exit:
	probe_err("error(%d) in init_iomem_area\n", ret);
	return ret;
}

