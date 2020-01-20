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

#if defined(CONFIG_SOC_EMULATOR9820) || defined(CONFIG_SOC_EXYNOS9820)
#include <linux/io.h>
#include <linux/delay.h>
#endif

#include "npu-log.h"
#include "npu-device.h"
#include "npu-system.h"

#ifdef CONFIG_NPU_HARDWARE
#include "mailbox_ipc.h"
#endif

#ifdef CONFIG_FIRMWARE_SRAM_DUMP_DEBUGFS
#include "npu-util-memdump.h"
#endif

#if 0
#define CAM_L0 690000
#define CAM_L1 680000
#define CAM_L2 670000
#define CAM_L3 660000
#define CAM_L4 650000
#define CAM_L5 640000

#define MIF_L0 2093000
#define MIF_L1 2002000
#define MIF_L2 1794000
#define MIF_L3 1539000
#define MIF_L4 1352000
#define MIF_L5 1014000
#define MIF_L6 845000
#define MIF_L7 676000
#endif

struct pm_qos_request exynos_npu_qos_cam;
struct pm_qos_request exynos_npu_qos_mem;

struct system_pwr sysPwr;

#if defined(CONFIG_SOC_EMULATOR9820) || defined(CONFIG_SOC_EXYNOS9820)
#define OFFSET_END	0xFFFFFFFF
struct reg_set_map {
	u32		offset;
	u32		val;
	u32		mask;
};

struct reg_set_map_2 {
	struct npu_iomem_area	*iomem_area;
	u32			offset;
	u32			val;
	u32			mask;
};

/* Initialzation steps for system_resume */
enum npu_system_resume_steps {
	NPU_SYS_RESUME_SETUP_WAKELOCK,
	NPU_SYS_RESUME_INIT_FWBUF,
	NPU_SYS_RESUME_FW_LOAD,
	NPU_SYS_RESUME_CLK_PREPARE,
	NPU_SYS_RESUME_CPU_ON,
	NPU_SYS_RESUME_CLKGATE23,
	NPU_SYS_RESUME_CLKGATE1,
	NPU_SYS_RESUME_CLKGATE4,
	NPU_SYS_RESUME_CLKGATE5,
	NPU_SYS_RESUME_OPEN_INTERFACE,
	NPU_SYS_RESUME_COMPLETED
};

static int npu_firmware_load(struct npu_system *system);
int npu_awwl_checker_en(struct npu_system *system);
int npu_pwr_on(struct npu_system *system);
int npu_clk_init(struct npu_system *system);
static int init_baaw_p_npu(struct npu_system *system);
int npu_cpu_on(struct npu_system *system);
int npu_cpu_off(struct npu_system *system);
static int init_iomem_area(struct npu_system *system);
static int set_hw_reg(const struct npu_iomem_area *base, const struct reg_set_map *set_map,
		      size_t map_len, int regset_mdelay);
static int set_hw_reg_2(const struct reg_set_map_2 *set_map, size_t map_len, int regset_mdelay);
static int set_sfr(const u32 sfr_addr, const u32 value, const u32 mask);
static int get_sfr(const u32 sfr_addr);
#endif
#ifdef CONFIG_EXYNOS_NPU_DRAM_FW_LOG_BUF
#define DRAM_FW_LOG_BUF_SIZE	(2048*1024)
#define DRAM_FW_REPORT_BUF_SIZE	(1024*1024)
static struct npu_memory_buffer dram_fw_log_buf = {
	.size = DRAM_FW_LOG_BUF_SIZE,
};
static struct npu_memory_buffer fw_report_buf = {
	.size = DRAM_FW_REPORT_BUF_SIZE,
};

#ifdef CLKGate23_SOC_HWACG
int CLKGate23_SOC_HWACG_read(struct npu_system *system)
{
	int ret = 0;

	void __iomem *reg_addr;
	volatile u32 v;
	struct npu_iomem_area *base;
	u32 offset;
	size_t i = 0;

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
		npu_info("v, reg_addr(0x%p)\n", reg_addr);

		v = readl(reg_addr) & 0xffffffff;
		npu_info("read cpu_on_regs, v(0x%x)\n", v);
	}

	return ret;
}

int CLKGate23_SOC_HWACG_qch_disable(struct npu_system *system)
{
	int ret = 0;

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
		npu_dbg("qch_on, v, reg_addr(0x%p)\n", reg_addr);

		v = readl(reg_addr) & 0xffffffff;
		npu_dbg("qch_on,  read cpu_on_regs, v(0x%x)\n", v);

		v = (v & (~mask)) | (val & mask);
		writel(v, (void *)(reg_addr));
		npu_dbg("written on, (0x%08x) at (%p)\n", v, reg_addr);
	}

	return ret;
}

int CLKGate23_SOC_HWACG_qch_enable(struct npu_system *system)
{
	int ret = 0;

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
		npu_dbg("qch clk off, reg_addr(0x%p)\n", reg_addr);

		v = readl(reg_addr) & 0xffffffff;
		npu_dbg("qch clk off, read cpu_on_regs, val(0x%x)\n", v);

		v = (v & (~mask)) | (val & mask);
		writel(v, (void *)(reg_addr));
		npu_dbg("written off (0x%08x) at (%p)\n", v, reg_addr);
	}

	return ret;
}
#endif

#ifdef CLKGate1_DRCG_EN
int CLKGate1_DRCG_EN_read(void)
{
	int ret = 0;

	npu_info("start CLKGate1_DRCG_EN_read\n");
	get_sfr(0x17910104);
	get_sfr(0x17910404);
	get_sfr(0x17A10104);
	return ret;
}

int CLKGate1_DRCG_EN_write_disable(void)
{
	int ret = 0;

	npu_info("start CLKGate1_DRCG_EN_write_disable\n");
	set_sfr(0x17910104, 0x00, 0xffffffff);
	set_sfr(0x17910404, 0x00, 0xffffffff);
	set_sfr(0x17A10104, 0x00, 0xffffffff);
	return ret;
}
#endif

#ifdef CLKGate4_IP_HWACG
int CLKGate4_IP_HWACG_read(void)
{
	int ret = 0;

	npu_info("start CLKGate4_IP_HWACG_read\n");
	get_sfr(0x179f45ec);
	get_sfr(0x179f45f0);
	get_sfr(0x17af45ec);
	get_sfr(0x17af45f0);
	get_sfr(0x179f4dec);
	get_sfr(0x179f4df0);
	get_sfr(0x17af4dec);
	get_sfr(0x17af4df0);
	get_sfr(0x179f40ec);
	get_sfr(0x179f40f0);
	get_sfr(0x17af40ec);
	get_sfr(0x17af40f0);
	get_sfr(0x179f48ec);
	get_sfr(0x179f48f0);
	get_sfr(0x17af48ec);
	get_sfr(0x17af48f0);
	get_sfr(0x179f58ec);
	get_sfr(0x179f58f0);
	get_sfr(0x17af58ec);
	get_sfr(0x17af58f0);
	get_sfr(0x179f5cec);
	get_sfr(0x179f5cf0);
	get_sfr(0x17af5cec);
	get_sfr(0x17af5cf0);
	get_sfr(0x179fc0ec);
	get_sfr(0x179fc0f0);
	get_sfr(0x17afc0ec);
	get_sfr(0x17afc0f0);
	get_sfr(0x179f50ec);
	get_sfr(0x179f50f0);
	get_sfr(0x17af50ec);
	get_sfr(0x17af50f0);
	get_sfr(0x179f54ec);
	get_sfr(0x179f54f0);
	get_sfr(0x17af54ec);
	get_sfr(0x17af54f0);
	return ret;
}

int CLKGate4_IP_HWACG_write_disable(void)
{
	int ret = 0;

	npu_info("start CLKGate4_IP_HWACG_write_disable\n");
	set_sfr(0x179f45ec, 0x00, 0xffffffff);
	set_sfr(0x179f45f0, 0x00, 0xffffffff);
	set_sfr(0x17af45ec, 0x00, 0xffffffff);
	set_sfr(0x17af45f0, 0x00, 0xffffffff);
	set_sfr(0x179f4dec, 0x00, 0xffffffff);
	set_sfr(0x179f4df0, 0x00, 0xffffffff);
	set_sfr(0x17af4dec, 0x00, 0xffffffff);
	set_sfr(0x17af4df0, 0x00, 0xffffffff);
	set_sfr(0x179f40ec, 0x00, 0xffffffff);
	set_sfr(0x179f40f0, 0x00, 0xffffffff);
	set_sfr(0x17af40ec, 0x00, 0xffffffff);
	set_sfr(0x17af40f0, 0x00, 0xffffffff);
	set_sfr(0x179f48ec, 0x00, 0xffffffff);
	set_sfr(0x179f48f0, 0x00, 0xffffffff);
	set_sfr(0x17af48ec, 0x00, 0xffffffff);
	set_sfr(0x17af48f0, 0x00, 0xffffffff);
	set_sfr(0x179f58ec, 0x00, 0xffffffff);
	set_sfr(0x179f58f0, 0x00, 0xffffffff);
	set_sfr(0x17af58ec, 0x00, 0xffffffff);
	set_sfr(0x17af58f0, 0x00, 0xffffffff);
	set_sfr(0x179f5cec, 0x00, 0xffffffff);
	set_sfr(0x179f5cf0, 0x00, 0xffffffff);
	set_sfr(0x17af5cec, 0x00, 0xffffffff);
	set_sfr(0x17af5cf0, 0x00, 0xffffffff);
	set_sfr(0x179fc0ec, 0x00, 0xffffffff);
	set_sfr(0x179fc0f0, 0x00, 0xffffffff);
	set_sfr(0x17afc0ec, 0x00, 0xffffffff);
	set_sfr(0x17afc0f0, 0x00, 0xffffffff);
	set_sfr(0x179f50ec, 0x00, 0xffffffff);
	set_sfr(0x179f50f0, 0x00, 0xffffffff);
	set_sfr(0x17af50ec, 0x00, 0xffffffff);
	set_sfr(0x17af50f0, 0x00, 0xffffffff);
	set_sfr(0x179f54ec, 0x00, 0xffffffff);
	set_sfr(0x179f54f0, 0x00, 0xffffffff);
	set_sfr(0x17af54ec, 0x00, 0xffffffff);
	set_sfr(0x17af54f0, 0x00, 0xffffffff);
	return ret;
}

int CLKGate4_IP_HWACG_qch_disable(void)
{
	int ret = 0;

	npu_info("start CLKGate4_IP_HWACG_qch_disable\n");
	set_sfr(0x179f45ec, 0x3f0002, 0xffffffff);
	set_sfr(0x179f45f0, 0x200000, 0xffffffff);
	set_sfr(0x17af45ec, 0x3f0002, 0xffffffff);
	set_sfr(0x17af45f0, 0x200000, 0xffffffff);
	set_sfr(0x179f4dec, 0x3f0002, 0xffffffff);
	set_sfr(0x179f4df0, 0x200000, 0xffffffff);
	set_sfr(0x17af4dec, 0x3f0002, 0xffffffff);
	set_sfr(0x17af4df0, 0x200000, 0xffffffff);
	set_sfr(0x179f40ec, 0x3f0002, 0xffffffff);
	set_sfr(0x179f40f0, 0x200000, 0xffffffff);
	set_sfr(0x17af40ec, 0x3f0002, 0xffffffff);
	set_sfr(0x17af40f0, 0x200000, 0xffffffff);
	set_sfr(0x179f48ec, 0x3f0002, 0xffffffff);
	set_sfr(0x179f48f0, 0x200000, 0xffffffff);
	set_sfr(0x17af48ec, 0x3f0002, 0xffffffff);
	set_sfr(0x17af48f0, 0x200000, 0xffffffff);
	set_sfr(0x179f58ec, 0x3f0002, 0xffffffff);
	set_sfr(0x179f58f0, 0x200000, 0xffffffff);
	set_sfr(0x17af58ec, 0x3f0002, 0xffffffff);
	set_sfr(0x17af58f0, 0x200000, 0xffffffff);
	set_sfr(0x179f5cec, 0x3f0002, 0xffffffff);
	set_sfr(0x179f5cf0, 0x200000, 0xffffffff);
	set_sfr(0x17af5cec, 0x3f0002, 0xffffffff);
	set_sfr(0x17af5cf0, 0x200000, 0xffffffff);
	set_sfr(0x179fc0ec, 0x3f0002, 0xffffffff);
	set_sfr(0x179fc0f0, 0x200000, 0xffffffff);
	set_sfr(0x17afc0ec, 0x3f0002, 0xffffffff);
	set_sfr(0x17afc0f0, 0x200000, 0xffffffff);
	set_sfr(0x179f50ec, 0x3f0002, 0xffffffff);
	set_sfr(0x179f50f0, 0x200000, 0xffffffff);
	set_sfr(0x17af50ec, 0x3f0002, 0xffffffff);
	set_sfr(0x17af50f0, 0x200000, 0xffffffff);
	set_sfr(0x179f54ec, 0x3f0002, 0xffffffff);
	set_sfr(0x179f54f0, 0x200000, 0xffffffff);
	set_sfr(0x17af54ec, 0x3f0002, 0xffffffff);
	set_sfr(0x17af54f0, 0x200000, 0xffffffff);
	return ret;
}

int CLKGate4_IP_HWACG_qch_enable(void)
{
	int ret = 0;

	npu_info("start CLKGate4_IP_HWACG_qch_enable\n");
	set_sfr(0x179f45ec, 0x3f0003, 0xffffffff);
	set_sfr(0x179f45f0, 0x10000000, 0xffffffff);
	set_sfr(0x17af45ec, 0x3f0003, 0xffffffff);
	set_sfr(0x17af45f0, 0x10000000, 0xffffffff);
	set_sfr(0x179f4dec, 0x3f0003, 0xffffffff);
	set_sfr(0x179f4df0, 0x10000000, 0xffffffff);
	set_sfr(0x17af4dec, 0x3f0003, 0xffffffff);
	set_sfr(0x17af4df0, 0x10000000, 0xffffffff);
	set_sfr(0x179f40ec, 0x3f0003, 0xffffffff);
	set_sfr(0x179f40f0, 0x10000000, 0xffffffff);
	set_sfr(0x17af40ec, 0x3f0003, 0xffffffff);
	set_sfr(0x17af40f0, 0x10000000, 0xffffffff);
	set_sfr(0x179f48ec, 0x3f0003, 0xffffffff);
	set_sfr(0x179f48f0, 0x10000000, 0xffffffff);
	set_sfr(0x17af48ec, 0x3f0003, 0xffffffff);
	set_sfr(0x17af48f0, 0x10000000, 0xffffffff);
	set_sfr(0x179f58ec, 0x3f0003, 0xffffffff);
	set_sfr(0x179f58f0, 0x10000000, 0xffffffff);
	set_sfr(0x17af58ec, 0x3f0003, 0xffffffff);
	set_sfr(0x17af58f0, 0x10000000, 0xffffffff);
	set_sfr(0x179f5cec, 0x3f0003, 0xffffffff);
	set_sfr(0x179f5cf0, 0x10000000, 0xffffffff);
	set_sfr(0x17af5cec, 0x3f0003, 0xffffffff);
	set_sfr(0x17af5cf0, 0x10000000, 0xffffffff);
	set_sfr(0x179fc0ec, 0x3f0003, 0xffffffff);
	set_sfr(0x179fc0f0, 0x10000000, 0xffffffff);
	set_sfr(0x17afc0ec, 0x3f0003, 0xffffffff);
	set_sfr(0x17afc0f0, 0x10000000, 0xffffffff);
	set_sfr(0x179f50ec, 0x3f0003, 0xffffffff);
	set_sfr(0x179f50f0, 0x10000000, 0xffffffff);
	set_sfr(0x17af50ec, 0x3f0003, 0xffffffff);
	set_sfr(0x17af50f0, 0x10000000, 0xffffffff);
	set_sfr(0x179f54ec, 0x3f0003, 0xffffffff);
	set_sfr(0x179f54f0, 0x10000000, 0xffffffff);
	set_sfr(0x17af54ec, 0x3f0003, 0xffffffff);
	set_sfr(0x17af54f0, 0x10000000, 0xffffffff);
	return ret;
}
#endif

#ifdef CLKGate5_IP_DRCG_EN
int CLKGate5_IP_DRCG_EN_read(void)
{
	int ret = 0;

	npu_info("start CLKGate5_IP_DRCG_EN_read\n");
	get_sfr(0x179f45fc);
	get_sfr(0x17af45fc);
	get_sfr(0x179f4dfc);
	get_sfr(0x17af4dfc);
	get_sfr(0x179f40fc);
	get_sfr(0x17af40fc);
	get_sfr(0x179f48fc);
	get_sfr(0x17af48fc);
	get_sfr(0x179f58fc);
	get_sfr(0x17af58fc);
	get_sfr(0x179f5cfc);
	get_sfr(0x17af5cfc);
	get_sfr(0x179fc0fc);
	get_sfr(0x17afc0fc);
	get_sfr(0x179f50fc);
	get_sfr(0x17af50fc);
	get_sfr(0x179f54fc);
	get_sfr(0x17af54fc);
	get_sfr(0x179f80fc);
	get_sfr(0x17af80fc);
	get_sfr(0x179f84fc);
	get_sfr(0x17af84fc);
	get_sfr(0x179f88fc);
	get_sfr(0x17af88fc);
	get_sfr(0x179f8cfc);
	get_sfr(0x17af8cfc);
	get_sfr(0x179f90fc);
	get_sfr(0x17af90fc);
	get_sfr(0x179f94fc);
	get_sfr(0x17af94fc);
	get_sfr(0x179f98fc);
	get_sfr(0x17af98fc);
	get_sfr(0x179f9cfc);
	get_sfr(0x17af9cfc);
	return ret;
}

int CLKGate5_IP_DRCG_EN_write_disable(void)
{
	int ret = 0;

	npu_info("start CLKGate5_IP_DRCG_EN_write_disable\n");
	set_sfr(0x179f45fc, 0x00, 0xffffffff);
	set_sfr(0x17af45fc, 0x00, 0xffffffff);
	set_sfr(0x179f4dfc, 0x00, 0xffffffff);
	set_sfr(0x17af4dfc, 0x00, 0xffffffff);
	set_sfr(0x179f40fc, 0x00, 0xffffffff);
	set_sfr(0x17af40fc, 0x00, 0xffffffff);
	set_sfr(0x179f48fc, 0x00, 0xffffffff);
	set_sfr(0x17af48fc, 0x00, 0xffffffff);
	set_sfr(0x179f58fc, 0x00, 0xffffffff);
	set_sfr(0x17af58fc, 0x00, 0xffffffff);
	set_sfr(0x179f5cfc, 0x00, 0xffffffff);
	set_sfr(0x17af5cfc, 0x00, 0xffffffff);
	set_sfr(0x179fc0fc, 0x00, 0xffffffff);
	set_sfr(0x17afc0fc, 0x00, 0xffffffff);
	set_sfr(0x179f50fc, 0x00, 0xffffffff);
	set_sfr(0x17af50fc, 0x00, 0xffffffff);
	set_sfr(0x179f54fc, 0x00, 0xffffffff);
	set_sfr(0x17af54fc, 0x00, 0xffffffff);
	set_sfr(0x179f80fc, 0x00, 0xffffffff);
	set_sfr(0x17af80fc, 0x00, 0xffffffff);
	set_sfr(0x179f84fc, 0x00, 0xffffffff);
	set_sfr(0x17af84fc, 0x00, 0xffffffff);
	set_sfr(0x179f88fc, 0x00, 0xffffffff);
	set_sfr(0x17af88fc, 0x00, 0xffffffff);
	set_sfr(0x179f8cfc, 0x00, 0xffffffff);
	set_sfr(0x17af8cfc, 0x00, 0xffffffff);
	set_sfr(0x179f90fc, 0x00, 0xffffffff);
	set_sfr(0x17af90fc, 0x00, 0xffffffff);
	set_sfr(0x179f94fc, 0x00, 0xffffffff);
	set_sfr(0x17af94fc, 0x00, 0xffffffff);
	set_sfr(0x179f98fc, 0x00, 0xffffffff);
	set_sfr(0x17af98fc, 0x00, 0xffffffff);
	set_sfr(0x179f9cfc, 0x00, 0xffffffff);
	set_sfr(0x17af9cfc, 0x00, 0xffffffff);
	return ret;
}
#endif

int npu_system_alloc_fw_dram_log_buf(struct npu_system *system)
{
	int ret;
	BUG_ON(!system);

	npu_info("start: initialization.\n");

	/* Request log buf allocation */
	ret = npu_memory_alloc(&system->memory, &dram_fw_log_buf);
	if (ret) {
		npu_err("fail(%d) in Log buffer memory allocation\n", ret);
		return ret;
	}

	npu_info("DRAM log buffer for firmware: size(%d) / dv(%pad) / kv(%p)",
		 DRAM_FW_LOG_BUF_SIZE, &dram_fw_log_buf.daddr, dram_fw_log_buf.vaddr);

	/* Initialize memory logger dram log buf */
	npu_store_log_init(dram_fw_log_buf.vaddr, dram_fw_log_buf.size);

	if (!fw_report_buf.vaddr) {
		ret = npu_memory_alloc(&system->memory, &fw_report_buf);
		if (ret) {
			npu_err("fail(%d) in Log buffer memory allocation\n", ret);
			return ret;
		}
		npu_fw_report_init(fw_report_buf.vaddr, fw_report_buf.size);
	} else {//Case of fw_report is already allocated by ion memory
		npu_dbg("fw_report is already initialized - %p.\n", fw_report_buf.vaddr);
	}

	/* Initialize firmware utc handler with dram log buf */
	ret = npu_fw_test_initialize(system, &dram_fw_log_buf);
	if (ret) {
		npu_err("npu_fw_test_probe() failed : ret = %d\n", ret);
		return ret;
	}
	npu_info("complete : initialization.\n");
	return 0;
}

static int npu_system_free_fw_dram_log_buf(struct npu_system *system)
{
	int ret;

	BUG_ON(!system);

	/* De-initialize memory logger dram log buf */
	npu_store_log_deinit();

	ret = npu_memory_free(&system->memory, &dram_fw_log_buf);
	if (ret) {
		npu_err("fail(%d) in Log buffer memory free\n", ret);
		goto err_exit;
	}

	npu_info("DRAM log buffer for firmware freed.\n");
	ret = 0;

err_exit:
	return ret;
}

#else
#define npu_system_alloc_fw_dram_log_buf(t)	(0)
#define npu_system_free_fw_dram_log_buf(t)	(0)
#endif	/* CONFIG_EXYNOS_NPU_DRAM_FW_LOG_BUF */

int npu_system_probe(struct npu_system *system, struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev;
#if defined(CONFIG_SOC_EMULATOR9820) || defined(CONFIG_SOC_EXYNOS9820)
	void *addr;
	int irq;
#endif
	BUG_ON(!system);
	BUG_ON(!pdev);

	dev = &pdev->dev;
#if defined(CONFIG_SOC_EMULATOR9820) || defined(CONFIG_SOC_EXYNOS9820)
	system->pdev = pdev;	/* TODO: Reference counting ? */
	system->cam_qos = 0;
	system->mif_qos = 0;

	npu_dbg("system probe: ioremap areas\n");
	ret = init_iomem_area(system);
	if (ret) {
		probe_err("fail(%d) in init_iomem_area", ret);
		goto p_err;
	}

	npu_dbg("system probe: Set BAAW for SRAM area\n");
	ret = init_baaw_p_npu(system);
	ret = 0;
	if (ret) {
		probe_err("fail(%d)in init_baaw_p_npu", ret);
		goto p_err;
	}

#endif

#if defined(CONFIG_SOC_EMULATOR9820) || defined(CONFIG_SOC_EXYNOS9820)
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		probe_err("fail(%d) in platform_get_irq(0)", irq);
		ret = -EINVAL;
		goto p_err;
	}
	system->irq0 = irq;

	irq = platform_get_irq(pdev, 1);
	if (irq < 0) {
		probe_err("fail(%d) in platform_get_irq(1)", irq);
		ret = -EINVAL;
		goto p_err;
	}
	system->irq1 = irq;
#endif

	ret = npu_memory_probe(&system->memory, dev);
	if (ret) {
		npu_err("fail(%d) in npu_memory_probe\n", ret);
		goto p_err;
	}
#if defined(CONFIG_SOC_EMULATOR9820) || defined(CONFIG_SOC_EXYNOS9820)
	addr = (void *)(system->mbox_sfr.vaddr);
	//(void *)ioremap_nocache(0x178b0000,0x100000);
	ret = npu_interface_probe(dev, addr, system->irq0, system->irq1);
	if (ret) {
		npu_err("fail(%d) in npu_interface_probe\n", ret);
		goto p_err;
	}

	ret = npu_binary_init(&system->binary,
		dev,
		NPU_FW_PATH1,
		NPU_FW_PATH2,
		NPU_FW_NAME);
	if (ret) {
		npu_err("fail(%d) in npu_binary_init\n", ret);
		goto p_err;
	}
	ret = npu_util_memdump_probe(system);
	if (ret) {
		npu_err("fail(%d) in npu_util_memdump_probe\n", ret);
		goto p_err;
	}

	/* get npu clock */
	system->npu_clk = devm_clk_get(dev, "clk_npu");
	if (IS_ERR(system->npu_clk)) {
		npu_err("%s() fail to get clk_npu\n", __func__);
		ret = PTR_ERR(system->npu_clk);
		goto p_err;
	}

	/* enable runtime pm */
	pm_runtime_enable(dev);

	ret = npu_qos_probe(system);
	if (ret) {
		npu_err("npu_qos_probe is fail(%d)\n", ret);
		goto p_qos_err;
	}

#ifdef CONFIG_PM_SLEEP
	/* initialize the npu wake lock */
	wake_lock_init(&system->npu_wake_lock, WAKE_LOCK_SUSPEND, "npu_run_wlock");
#endif
	init_waitqueue_head(&sysPwr.wq);
	sysPwr.system_result.result_code = NPU_SYSTEM_JUST_STARTED;
	goto p_exit;

#endif
p_qos_err:
	pm_runtime_disable(dev);
	if (system->npu_clk)
		devm_clk_put(dev, system->npu_clk);
p_err:
p_exit:
	return ret;
}

/* TODO: Implement throughly */
int npu_system_release(struct npu_system *system, struct platform_device *pdev)
{
	int ret;
	struct device *dev;

	BUG_ON(!system);
	BUG_ON(!pdev);

	dev = &pdev->dev;

#ifdef CONFIG_PM_SLEEP
	wake_lock_destroy(&system->npu_wake_lock);
#endif

	pm_runtime_disable(dev);

	if (system->npu_clk) {
		devm_clk_put(dev, system->npu_clk);
		platform_set_drvdata(pdev, NULL);
	}

	ret = npu_qos_release(system);
	if (ret)
		npu_err("fail(%d) in npu_qos_release\n", ret);

	return 0;
}

static inline void print_iomem_area(const char *pr_name, const struct npu_iomem_area *mem_area)
{
	npu_info(KERN_CONT "(%8s) Phy(0x%08x)-(0x%08llx) Virt(%p) Size(%llu)\n",
		pr_name, mem_area->paddr, mem_area->paddr + mem_area->size,
		mem_area->vaddr, mem_area->size);
}

static void print_all_iomem_area(const struct npu_system *system)
{
	npu_dbg("start in IOMEM mapping\n");
#if defined(CONFIG_SOC_EMULATOR9820) || defined(CONFIG_SOC_EXYNOS9820)
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
#endif	/* defined(CONFIG_SOC_EMULATOR9820) || defined(CONFIG_SOC_EXYNOS9820) */
	npu_dbg("end in IOMEM mapping\n");
}

int npu_system_open(struct npu_system *system)
{
	int ret = 0;
	struct device *dev;

	BUG_ON(!system);
	BUG_ON(!system->pdev);
	dev = &system->pdev->dev;

	ret = npu_memory_open(&system->memory);
	if (ret) {
		npu_err("fail(%d) in npu_memory_open\n", ret);
		goto p_err;
	}

	ret = npu_util_memdump_open(system);
	if (ret) {
		npu_err("fail(%d) in npu_util_memdump_open\n", ret);
		goto p_err;
	}

	/* Clear resume steps */
	system->resume_steps = 0;
p_err:
	return ret;
}

int npu_system_close(struct npu_system *system)
{
	int ret = 0;

#ifdef CONFIG_FIRMWARE_SRAM_DUMP_DEBUGFS
	ret = npu_util_memdump_close(system);
	if (ret)
		npu_err("fail(%d) in npu_util_memdump_close\n", ret);
#endif
	ret = npu_memory_close(&system->memory);
	if (ret)
		npu_err("fail(%d) in npu_memory_close\n", ret);
	return ret;
}

int npu_system_resume(struct npu_system *system, u32 mode)
{
	int ret = 0;
	void *addr;
	struct device *dev;
	struct clk *npu_clk;
	struct npu_device *device;

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	dev = &system->pdev->dev;
	npu_clk = system->npu_clk;
	device = container_of(system, struct npu_device, system);

	/* Clear resume steps */
	system->resume_steps = 0;

	print_all_iomem_area(system);
#ifdef CONFIG_PM_SLEEP
	/* prevent the system to suspend */
	if (!wake_lock_active(&system->npu_wake_lock)) {
		wake_lock(&system->npu_wake_lock);
		npu_info("wake_lock, now(%d)\n", wake_lock_active(&system->npu_wake_lock));
	}
	set_bit(NPU_SYS_RESUME_SETUP_WAKELOCK, &system->resume_steps);
#endif

	ret = npu_system_alloc_fw_dram_log_buf(system);
	if (ret) {
		npu_err("fail(%d) in npu_system_alloc_fw_dram_log_buf\n", ret);
		goto p_err;
	}
	set_bit(NPU_SYS_RESUME_INIT_FWBUF, &system->resume_steps);

	ret = npu_firmware_load(system);
	if (ret) {
		npu_err("fail(%d) in npu_firmware_load\n", ret);
		goto p_err;
	}
	set_bit(NPU_SYS_RESUME_FW_LOAD, &system->resume_steps);

	if (npu_clk) {
		ret = clk_prepare_enable(npu_clk);
		if (ret) {
			npu_err("fail to enable npu_clk(%d)\n", ret);
			goto p_err;
		}
	}
	set_bit(NPU_SYS_RESUME_CLK_PREPARE, &system->resume_steps);

	ret = npu_cpu_on(system);
	if (ret) {
		npu_err("fail(%d) in npu_cpu_on\n", ret);
		goto p_err;
	}
	set_bit(NPU_SYS_RESUME_CPU_ON, &system->resume_steps);

#ifdef CLKGate23_SOC_HWACG
	CLKGate23_SOC_HWACG_qch_disable(system);
	set_bit(NPU_SYS_RESUME_CLKGATE23, &system->resume_steps);
#endif

#ifdef CLKGate1_DRCG_EN
	CLKGate1_DRCG_EN_write_disable();
	set_bit(NPU_SYS_RESUME_CLKGATE1, &system->resume_steps);
#else
	npu_info("CLKGate1_DRCG_EN_write_enable\n");
#endif

#ifdef CLKGate4_IP_HWACG
	CLKGate4_IP_HWACG_qch_disable();
	set_bit(NPU_SYS_RESUME_CLKGATE4, &system->resume_steps);
#endif

#ifdef CLKGate5_IP_DRCG_EN
	CLKGate5_IP_DRCG_EN_write_disable();
	set_bit(NPU_SYS_RESUME_CLKGATE5, &system->resume_steps);
#else
	npu_info("CLKGate5_IP_DRCG_EN_write_enable\n");
#endif
	addr = (void *)(system->tcu_sram.vaddr);
	system->mbox_hdr = (volatile struct mailbox_hdr *)(addr + NPU_MAILBOX_BASE - sizeof(struct mailbox_hdr));
	ret = npu_interface_open(system);
	if (ret) {
		npu_err("fail(%d) in npu_interface_open\n", ret);
		goto p_err;
	}
	set_bit(NPU_SYS_RESUME_OPEN_INTERFACE, &system->resume_steps);

	set_bit(NPU_SYS_RESUME_COMPLETED, &system->resume_steps);

	return ret;
p_err:
	npu_err("Failure detected[%d]. Set emergency recovery flag.\n", ret);
	set_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state);
	ret = 0;//emergency case will be cared by suspend func
	return ret;
}

int npu_system_suspend(struct npu_system *system)
{
	int ret = 0;
	struct device *dev;
	struct clk *npu_clk;
	struct npu_device *device;

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	dev = &system->pdev->dev;
	npu_clk = system->npu_clk;
	device = container_of(system, struct npu_device, system);

	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_COMPLETED, &system->resume_steps, NULL, ;);

	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_OPEN_INTERFACE, &system->resume_steps, "Close interface", {
		ret = npu_interface_close();
		if (ret)
			npu_err("fail(%d) in npu_interface_close\n", ret);
	});

#ifdef CLKGate23_SOC_HWACG
	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_CLKGATE23, &system->resume_steps, "CLKGate23_SOC_HWACG_qch_enable", {
		CLKGate23_SOC_HWACG_qch_enable(system);
	});
#endif

#ifdef CLKGate1_DRCG_EN
	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_CLKGATE1, &system->resume_steps, NULL, ;);
#endif

#ifdef CLKGate4_IP_HWACG
	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_CLKGATE4, &system->resume_steps, "CLKGate4_IP_HWACG_qch_enable", {
		CLKGate4_IP_HWACG_qch_enable();
	});
#endif

#ifdef CLKGate5_IP_DRCG_EN
	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_CLKGATE5, &system->resume_steps, NULL, ;);
#endif

	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_CPU_ON, &system->resume_steps, "Turn NPU cpu off", {
		ret = npu_cpu_off(system);
		if (ret)
			npu_err("fail(%d) in npu_cpu_off\n", ret);
	});

#ifdef CONFIG_PM_SLEEP
	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_SETUP_WAKELOCK, &system->resume_steps, "Unlock wake lock", {
		if (wake_lock_active(&system->npu_wake_lock)) {
			wake_unlock(&system->npu_wake_lock);
			npu_dbg("wake_unlock, now(%d)\n", wake_lock_active(&system->npu_wake_lock));
		}
	});
#endif

	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_CLK_PREPARE, &system->resume_steps, "Unprepare clk", {
		if (npu_clk) {
			clk_disable_unprepare(npu_clk);
			/* check */
			if (__clk_is_enabled(npu_clk))
				npu_err("%s() req npu_clk off but on\n", __func__);
		}
	});

#if 0 // 0521_CLEAN_CODE
	ret = npu_memory_close(&system->memory);
	if (ret) {
		npu_err("fail(%d) in vpu_memory_close\n", ret);
		goto p_err;
	}
#endif

	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_FW_LOAD, &system->resume_steps, NULL, ;);
	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_INIT_FWBUF, &system->resume_steps, "Free DRAM fw log buf", {
		ret = npu_system_free_fw_dram_log_buf(system);
		if (ret)
			npu_err("fail(%d) in npu_cpu_off\n", ret);
	});

	if (system->resume_steps != 0)
		npu_warn("Missing clean-up steps [%lu] found.\n", system->resume_steps);

	/* Function itself never be failed, even thought there was some error */
	return 0;
}

int npu_system_start(struct npu_system *system)
{
	int ret = 0;
	struct device *dev;

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	dev = &system->pdev->dev;

#ifdef CONFIG_FIRMWARE_SRAM_DUMP_DEBUGFS
	ret = npu_util_memdump_start(system);
	if (ret) {
		npu_err("fail(%d) in npu_util_memdump_start\n", ret);
		goto p_err;
	}
#endif

	ret = npu_qos_start(system);
	if (ret) {
		npu_err("fail(%d) in npu_qos_start\n", ret);
		goto p_err;
	}

p_err:

	return ret;
}

int npu_system_stop(struct npu_system *system)
{
	int ret = 0;
	struct device *dev;

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	dev = &system->pdev->dev;
#ifdef CONFIG_FIRMWARE_SRAM_DUMP_DEBUGFS
	ret = npu_util_memdump_stop(system);
	if (ret) {
		npu_err("fail(%d) in npu_util_memdump_stop\n", ret);
		goto p_err;
	}
#endif

	ret = npu_qos_stop(system);
	if (ret) {
		npu_err("fail(%d) in npu_qos_stop\n", ret);
		goto p_err;
	}
p_err:

	return 0;
}

int npu_system_save_result(struct npu_session *session, struct nw_result nw_result)
{
	int ret = 0;
	sysPwr.system_result.result_code = nw_result.result_code;
	wake_up(&sysPwr.wq);
	return ret;
}

#if defined(CONFIG_SOC_EMULATOR9820) || defined(CONFIG_SOC_EXYNOS9820)
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

	npu_info("start in BAAW initialization");

	npu_dbg("update in BAAW initialization\n");
	ret = set_hw_reg(&system->baaw_npu, baaw_init, ARRAY_SIZE(baaw_init), 0);
	if (ret) {
		npu_err("fail(%) in set_hw_reg(baaw_init)\n", ret);
		goto err_exit;
	}
	npu_info("complete in BAAW initialization : \n");
	return 0;

err_exit:
	npu_info("error(%d) in BAAW initialization\n", ret);
	return ret;
}
#else
/* Do nothing */
static int init_baaw_p_npu(struct npu_system *system) {return 0; }

#endif

static int npu_firmware_load(struct npu_system *system)
{
	int ret = 0;
	u32 v;

	BUG_ON(!system);
	npu_info("Firmware load : Start\n");
#ifdef CLEAR_SRAM_ON_FIRMWARE_LOADING
#ifdef CLEAR_ON_SECOND_LOAD_ONLY
	v = readl(system->tcu_sram.vaddr + system->tcu_sram.size - sizeof(u32));
	npu_dbg("firmware load: Check current signature value : 0x%08x (%s)\n",
		v, (v == 0)?"First load":"Second load");
#else
	v = 1;
#endif
	if (v != 0) {
		npu_dbg("firmware load : clear TCU SRAM at %p, Len(%llu)\n",
			system->tcu_sram.vaddr, system->tcu_sram.size);
		/* Using memset here causes unaligned access fault.
		Refer: https://patchwork.kernel.org/patch/6362401/ */
		memset_io(system->tcu_sram.vaddr, 0, system->tcu_sram.size);
		npu_dbg("firmware load: clear IDP SRAM at %p, Len(%llu)\n",
			system->idp_sram.vaddr, system->idp_sram.size);
		memset_io(system->idp_sram.vaddr, 0, system->idp_sram.size);
	}
#else
	npu_dbg("firmware load: clear firmware signature at %p(u64)\n",
		system->tcu_sram.vaddr + system->tcu_sram.size - sizeof(u64));
	writel(0, system->tcu_sram.vaddr + system->tcu_sram.size - sizeof(u64));
#endif
	npu_dbg("firmware load: read and locate firmware to %p\n", system->tcu_sram.vaddr);
	ret = npu_firmware_file_read(&system->binary, system->tcu_sram.vaddr, system->tcu_sram.size);
	if (ret) {
		npu_err("error(%d) in npu_binary_read\n", ret);
		goto err_exit;
	}
	npu_dbg("checking firmware head MAGIC(0x%08x)\n", *(u32 *)system->tcu_sram.vaddr);

	npu_info("complete in npu_firmware_load\n");
	return 0;
err_exit:

	npu_info("error(%d) in npu_firmware_load\n", ret);
	return ret;
}

int npu_awwl_checker_en(struct npu_system *system)
{
	int ret;
	const struct reg_set_map awwl_checker_en[] = {
		/* NPU0 */
		{0x10424, 0x02, 0x02},	/* CM7_ETC */
	};

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	npu_info("start in npu_awwl_checker_en");

	npu_dbg("set SFR in npu_awwl_checker_en\n");
	ret = set_hw_reg(&system->sfr_npu0, awwl_checker_en, ARRAY_SIZE(awwl_checker_en), 0);
	if (ret) {
		npu_err("fail(%d) in set_hw_reg(awwl_checker_en)\n", ret);
		goto err_exit;
	}
	npu_info("complete in npu_awwl_checker_en");
	return 0;

err_exit:
	npu_info("error(%d) in npu_awwl_checker_en", ret);
	return ret;
}

int npu_clk_init(struct npu_system *system)
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

	npu_info("start in npu_clk_init");

	npu_dbg("initialize clock NPU0 in npu_clk_init\n");
	ret = set_hw_reg(&system->sfr_npu0, clk_on_npu0, ARRAY_SIZE(clk_on_npu0), 0);
	if (ret) {
		npu_err("fail(%d) in set_hw_reg(clk_on_npu0)\n", ret);
		goto err_exit;
	}

	npu_dbg("Initialize clock NPU1 in npu_clk_init\n");
	ret = set_hw_reg(&system->sfr_npu1, clk_on_npu1, ARRAY_SIZE(clk_on_npu1), 0);
	if (ret) {
		npu_err("fail(%d) in set_hw_reg(clk_on_npu1)\n", ret);
		goto err_exit;
	}
	npu_info("complete in npu_clk_init");
	return 0;

err_exit:
	npu_info("error(%d) in npu_clk_init", ret);
	return ret;
}

int npu_cpu_on(struct npu_system *system)
{
	int ret;
	const struct reg_set_map_2 cpu_on_regs[] = {
		/* NPU0_CM7_CFG1, SysTick Callibration, use external OSC, 26MHz */
		{&system->sfr_npu0,	0x1040c,	0x3f79f,	0xffffffff},
#ifdef FORCE_HWACG_DISABLE
		{&system->sfr_npu0,	0x800,	0x00,	0x10000000},	/* NPU0_CMU_NPU0_CONTROLLER_OPTION */
		{&system->sfr_npu1,	0x800,	0x00,	0x10000000},	/* NPU1_CMU_NPU1_CONTROLLER_OPTION */
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

	npu_info("start in npu_cpu_on");
	ret = set_hw_reg_2(cpu_on_regs, ARRAY_SIZE(cpu_on_regs), 0);
	if (ret) {
		npu_err("fail(%d) in set_hw_reg_2(cpu_on_regs)\n", ret);
		goto err_exit;
	}

	npu_info("complete in npu_cpu_on\n");
	return 0;
err_exit:
	npu_info("error(%d) in npu_cpu_on\n", ret);
	return ret;
}

int npu_cpu_off(struct npu_system *system)
{
	int ret;

	const static struct reg_set_map cpu_on_regs[] = {
#ifdef NPU_CM7_RELEASE_HACK
		{0x20,	0x00,	0x01},	/* NPU0_CPU_OUT */
#endif
		{0x00,	0x00,	0x01},	/* NPU0_CPU_CONFIGURATION */
	};

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	npu_info("start in npu_cpu_off\n");
	ret = set_hw_reg(&system->pmu_npu_cpu, cpu_on_regs, ARRAY_SIZE(cpu_on_regs), 0);
	if (ret) {
		npu_err("fail(%d) in set_hw_reg(cpu_on_regs)\n", ret);
		goto err_exit;
	}
	npu_info("complete in npu_cpu_off\n");
	return 0;
err_exit:
	npu_info("error(%d) in npu_cpu_off\n", ret);
	return ret;
}

#define TIMEOUT_ITERATION	100
int npu_pwr_on(struct npu_system *system)
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
	ret = set_hw_reg(&system->pmu_npu, pwr_on_regs, ARRAY_SIZE(pwr_on_regs), 0);
	if (ret) {
		npu_err("fail(%d) in set_hw_reg(pwr_on_regs)\n", ret);
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
			if ((v & pwr_status_regs[i].mask) != pwr_status_regs[i].val)	{
				check_pass = 0;
			}
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
	npu_info("error(%d) in npu_pwr_on\n", ret);
	return ret;

}

struct npu_iomem_init_data {
	u32	start;
	u32	end;
	struct npu_iomem_area *area_info;	/* Save iomem result */
};

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

	npu_info("check1\n");

	probe_trace("start in iomem area.\n");

	for (i = 0; init_data[i].start != OFFSET_END; ++i) {
		size = init_data[i].end - init_data[i].start;
		iomem = devm_ioremap_nocache(&(system->pdev->dev), init_data[i].start, size);
		if (IS_ERR_OR_NULL(iomem)) {
			probe_err("fail(%p) in devm_ioremap_nocache(0x%08x, %u)\n",
				  iomem, init_data[i].start, size);
			ret = -EFAULT;
			goto err_exit;
		}
		init_data[i].area_info->vaddr = iomem;
		init_data[i].area_info->paddr = init_data[i].start;
		init_data[i].area_info->size = size;
		probe_trace("Paddr[%08x]-[%08x] => Mapped @[%p], Length = %llu\n",
			   init_data[i].start, init_data[i].end,
			   init_data[i].area_info->vaddr, init_data[i].area_info->size);
	}
	probe_trace("complete in init_iomem_area\n");
	return 0;
err_exit:
	probe_err("error(%d) in init_iomem_area\n", ret);
	return ret;
}

static inline void __write_hw_reg(const struct npu_iomem_area *base, u32 offset, u32 val, u32 mask)
{
	void __iomem *reg_addr;
	volatile u32 v;

	BUG_ON(!base);
	BUG_ON(!base->vaddr);

	if (offset > base->size) {
		npu_err("offset(%u) exceeds iomem region size(%llu), starting at (%u)\n",
				offset, base->size, base->paddr);
		BUG_ON(1);
	}
	reg_addr = base->vaddr + offset;
	v = readl(reg_addr);
	npu_dbg("setting register pa(0x%08x) va(%p) cur(0x%08x) val(0x%08x) mask(0x%08x)\n",
		base->paddr + offset, reg_addr,	v, val, mask);

	v = (v & (~mask)) | (val & mask);
	writel(v, (void *)(reg_addr));
	npu_dbg("written (0x%08x) at (%p)\n", v, reg_addr);
}

/*
 * Set the set of register value specified in set_map,
 * with the base specified at base.
 * if regset_mdelay != 0, it will be the delay between register set.
 */
static int set_hw_reg(const struct npu_iomem_area *base, const struct reg_set_map *set_map,
		      size_t map_len, int regset_mdelay)
{
	size_t i;

	BUG_ON(!base);
	BUG_ON(!set_map);
	BUG_ON(!base->vaddr);

	for (i = 0; i < map_len; ++i) {
		__write_hw_reg(base, set_map[i].offset, set_map[i].val, set_map[i].mask);
		/* Insert delay between register setting */
		if (regset_mdelay > 0)	{
			mdelay(regset_mdelay);
		}
	}
	return 0;
}

static int set_hw_reg_2(const struct reg_set_map_2 *set_map, size_t map_len, int regset_mdelay)
{
	size_t i;

	BUG_ON(!set_map);

	for (i = 0; i < map_len; ++i) {
		__write_hw_reg(set_map[i].iomem_area, set_map[i].offset, set_map[i].val, set_map[i].mask);

		/* Insert delay between register setting */
		if (regset_mdelay > 0)	{
			mdelay(regset_mdelay);
		}
	}
	return 0;
}

__attribute__((unused)) static int set_sfr(const u32 sfr_addr, const u32 value, const u32 mask)
{
	int			ret;
	void __iomem		*iomem = NULL;
	struct npu_iomem_area	area_info;	/* Save iomem result */

	iomem = ioremap_nocache(sfr_addr, sizeof(u32));
	if (IS_ERR_OR_NULL(iomem)) {
		probe_err("fail(%p) in ioremap_nocache(0x%08x)\n",
			  iomem, sfr_addr);
		ret = -EFAULT;
		goto err_exit;
	}
	area_info.vaddr = iomem;
	area_info.paddr = sfr_addr;
	area_info.size = sizeof(u32);

	__write_hw_reg(&area_info, 0, value, mask);

	ret = 0;
err_exit:
	if (iomem)
		iounmap(iomem);

	return ret;
}

__attribute__((unused)) static int get_sfr(const u32 sfr_addr)
{
	int			ret;
	void __iomem		*iomem = NULL;
	struct npu_iomem_area	area_info;	/* Save iomem result */
	volatile u32 v;
	void __iomem *reg_addr;

	iomem = ioremap_nocache(sfr_addr, sizeof(u32));
	if (IS_ERR_OR_NULL(iomem)) {
		probe_err("fail(%p) in ioremap_nocache(0x%08x)\n",
			  iomem, sfr_addr);
		ret = -EFAULT;
		goto err_exit;
	}
	area_info.vaddr = iomem;
	area_info.paddr = sfr_addr;
	area_info.size = sizeof(u32);

	reg_addr = area_info.vaddr;

	v = readl(reg_addr);
	npu_trace("get_sfr, vaddr(0x%p), paddr(0x%08x), val(0x%x)\n",
		area_info.vaddr, area_info.paddr, v);

	ret = 0;
err_exit:
	if (iomem)
		iounmap(iomem);

	return ret;
}

#endif
