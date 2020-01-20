/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/io.h>

#include "score_log.h"
#include "score_regs.h"
#include "score_dpmu.h"

static void __iomem *dpmu_sfr;
static int debug_en = 1;
static int performance_en;
static int stall_en;

void score_dpmu_print_dbg_intr(void)
{
	writel(0x1, dpmu_sfr + CPUID_OF_CCH);
	score_info("DPMU:%32s: %8x\n",
			"DBG_INTR_ENABLE",
			readl(dpmu_sfr + DBG_INTR_ENABLE));
	score_info("DPMU:%32s: %8x %8x\n",
			"DBG_INTR_STATUS(raw/masked)",
			readl(dpmu_sfr + DBG_RAW_INTR_STATUS),
			readl(dpmu_sfr + DBG_MASKED_INTR_STATUS));
	writel(0x0, dpmu_sfr + CPUID_OF_CCH);
}

void score_dpmu_print_monitor_status(void)
{
	score_info("DPMU:%32s: %8x %8x\n",
			"MONITOR_ENABLE(dbg/perf)",
			readl(dpmu_sfr + DBG_MONITOR_ENABLE),
			readl(dpmu_sfr + PERF_MONITOR_ENABLE));
}

void score_dpmu_print_mc_cache(void)
{
	if (!performance_en)
		return;
	score_info("DPMU:%32s: %8x %8x\n",
			"MC_ICACHE_ACC_COUNT(H/L)",
			readl(dpmu_sfr + MC_ICACHE_ACC_COUNT_HIGH),
			readl(dpmu_sfr + MC_ICACHE_ACC_COUNT_LOW));
	score_info("DPMU:%32s: %8x\n",
			"MC_ICACHE_MISS_COUNT",
			readl(dpmu_sfr + MC_ICACHE_MISS_COUNT));
	score_info("DPMU:%32s: %8x %8x\n",
			"MC_DCACHE_COUNT(acc/miss)",
			readl(dpmu_sfr + MC_DCACHE_ACC_COUNT),
			readl(dpmu_sfr + MC_DCACHE_MISS_COUNT));
}

void score_dpmu_print_kc1_cache(void)
{
	if (!performance_en)
		return;
	score_info("DPMU:%32s: %8x %8x\n",
			"KC1_ICACHE_ACC_COUNT(H/L)",
			readl(dpmu_sfr + KC1_ICACHE_ACC_COUNT_HIGH),
			readl(dpmu_sfr + KC1_ICACHE_ACC_COUNT_LOW));
	score_info("DPMU:%32s: %8x\n",
			"KC1_ICACHE_MISS_COUNT",
			readl(dpmu_sfr + KC1_ICACHE_MISS_COUNT));
	score_info("DPMU:%32s: %8x %8x\n",
			"KC1_DCACHE_COUNT(acc/miss)",
			readl(dpmu_sfr + KC1_DCACHE_ACC_COUNT),
			readl(dpmu_sfr + KC1_DCACHE_MISS_COUNT));
}

void score_dpmu_print_kc2_cache(void)
{
	if (!performance_en)
		return;
	score_info("DPMU:%32s: %8x %8x\n",
			"KC2_ICACHE_ACC_COUNT(H/L)",
			readl(dpmu_sfr + KC2_ICACHE_ACC_COUNT_HIGH),
			readl(dpmu_sfr + KC2_ICACHE_ACC_COUNT_LOW));
	score_info("DPMU:%32s: %8x\n",
			"KC2_ICACHE_MISS_COUNT",
			readl(dpmu_sfr + KC2_ICACHE_MISS_COUNT));
	score_info("DPMU:%32s: %8x %8x\n",
			"KC2_DCACHE_COUNT(acc/miss)",
			readl(dpmu_sfr + KC2_DCACHE_ACC_COUNT),
			readl(dpmu_sfr + KC2_DCACHE_MISS_COUNT));
}

void score_dpmu_print_axi(void)
{
	if (!performance_en)
		return;
	score_info("DPMU:%32s: %8x %8x %8x %8x %8x\n",
			"RDBEAT_COUNT(m0/m1/m2/m3/s2)",
			readl(dpmu_sfr + AXIM0_RDBEAT_COUNT),
			readl(dpmu_sfr + AXIM1_RDBEAT_COUNT),
			readl(dpmu_sfr + AXIM2_RDBEAT_COUNT),
			readl(dpmu_sfr + AXIM3_RDBEAT_COUNT),
			readl(dpmu_sfr + AXIS2_RDBEAT_COUNT));
	score_info("DPMU:%32s: %8x %8x %8x %8x %8x\n",
			"WRBEAT_COUNT(m0/m1/m2/m3/s2)",
			readl(dpmu_sfr + AXIM0_WRBEAT_COUNT),
			readl(dpmu_sfr + AXIM1_WRBEAT_COUNT),
			readl(dpmu_sfr + AXIM2_WRBEAT_COUNT),
			readl(dpmu_sfr + AXIM3_WRBEAT_COUNT),
			readl(dpmu_sfr + AXIS2_WRBEAT_COUNT));
	score_info("DPMU:%32s: %8x %8x %8x %8x %8x\n",
			"RD_MAX_MO_COUNT(m0/m1/m2/m3/s2)",
			readl(dpmu_sfr + AXIM0_RD_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIM1_RD_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIM2_RD_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIM3_RD_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIS2_RD_MAX_MO_COUNT));
	score_info("DPMU:%32s: %8x %8x %8x %8x %8x\n",
			"WR_MAX_MO_COUNT(m0/m1/m2/m3/s2)",
			readl(dpmu_sfr + AXIM0_WR_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIM1_WR_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIM2_WR_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIM3_WR_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIS2_WR_MAX_MO_COUNT));
	score_info("DPMU:%32s: %8x %8x %8x %8x %8x\n",
			"RD_MAX_REQ_LAT(m0/m1/m2/m3/s2)",
			readl(dpmu_sfr + AXIM0_RD_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIM1_RD_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIM2_RD_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIM3_RD_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIS2_RD_MAX_REQ_LAT));
	score_info("DPMU:%32s: %8x %8x %8x %8x %8x\n",
			"WR_MAX_REQ_LAT(m0/m1/m2/m3/s2)",
			readl(dpmu_sfr + AXIM0_WR_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIM1_WR_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIM2_WR_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIM3_WR_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIS2_WR_MAX_REQ_LAT));
	score_info("DPMU:%32s: %8x %8x %8x %8x %8x\n",
			"RD_MAX_AR2R_LAT(m0/m1/m2/m3/s2)",
			readl(dpmu_sfr + AXIM0_RD_MAX_AR2R_LAT),
			readl(dpmu_sfr + AXIM1_RD_MAX_AR2R_LAT),
			readl(dpmu_sfr + AXIM2_RD_MAX_AR2R_LAT),
			readl(dpmu_sfr + AXIM3_RD_MAX_AR2R_LAT),
			readl(dpmu_sfr + AXIS2_RD_MAX_AR2R_LAT));
	score_info("DPMU:%32s: %8x %8x %8x %8x %8x\n",
			"WR_MAX_R2R_LAT(m0/m1/m2/m3/s2)",
			readl(dpmu_sfr + AXIM0_WR_MAX_R2R_LAT),
			readl(dpmu_sfr + AXIM1_WR_MAX_R2R_LAT),
			readl(dpmu_sfr + AXIM2_WR_MAX_R2R_LAT),
			readl(dpmu_sfr + AXIM3_WR_MAX_R2R_LAT),
			readl(dpmu_sfr + AXIS2_WR_MAX_R2R_LAT));
}

void score_dpmu_print_dma0(void)
{
	if (!performance_en)
		return;
	score_info("DPMU:%32s: %8x %8x %8x %8x\n",
			"DMA0_EXEC_CYCLE 00~03",
			readl(dpmu_sfr + DMA0_EXEC_CYCLE0),
			readl(dpmu_sfr + DMA0_EXEC_CYCLE1),
			readl(dpmu_sfr + DMA0_EXEC_CYCLE2),
			readl(dpmu_sfr + DMA0_EXEC_CYCLE3));
	score_info("DPMU:%32s: %8x %8x %8x %8x\n",
			"DMA0_EXEC_CYCLE 04~07",
			readl(dpmu_sfr + DMA0_EXEC_CYCLE4),
			readl(dpmu_sfr + DMA0_EXEC_CYCLE5),
			readl(dpmu_sfr + DMA0_EXEC_CYCLE6),
			readl(dpmu_sfr + DMA0_EXEC_CYCLE7));
	score_info("DPMU:%32s: %8x %8x %8x %8x\n",
			"DMA0_EXEC_CYCLE 08~11",
			readl(dpmu_sfr + DMA0_EXEC_CYCLE8),
			readl(dpmu_sfr + DMA0_EXEC_CYCLE9),
			readl(dpmu_sfr + DMA0_EXEC_CYCLE10),
			readl(dpmu_sfr + DMA0_EXEC_CYCLE11));
	score_info("DPMU:%32s: %8x %8x %8x %8x\n",
			"DMA0_EXEC_CYCLE 12~15",
			readl(dpmu_sfr + DMA0_EXEC_CYCLE12),
			readl(dpmu_sfr + DMA0_EXEC_CYCLE13),
			readl(dpmu_sfr + DMA0_EXEC_CYCLE14),
			readl(dpmu_sfr + DMA0_EXEC_CYCLE15));
}

void score_dpmu_print_dma1(void)
{
	if (!performance_en)
		return;
	score_info("DPMU:%32s: %8x %8x %8x %8x\n",
			"DMA1_EXEC_CYCLE 00~03",
			readl(dpmu_sfr + DMA1_EXEC_CYCLE0),
			readl(dpmu_sfr + DMA1_EXEC_CYCLE1),
			readl(dpmu_sfr + DMA1_EXEC_CYCLE2),
			readl(dpmu_sfr + DMA1_EXEC_CYCLE3));
	score_info("DPMU:%32s: %8x %8x %8x %8x\n",
			"DMA1_EXEC_CYCLE 04~07",
			readl(dpmu_sfr + DMA1_EXEC_CYCLE4),
			readl(dpmu_sfr + DMA1_EXEC_CYCLE5),
			readl(dpmu_sfr + DMA1_EXEC_CYCLE6),
			readl(dpmu_sfr + DMA1_EXEC_CYCLE7));
	score_info("DPMU:%32s: %8x %8x %8x %8x\n",
			"DMA1_EXEC_CYCLE 08~11",
			readl(dpmu_sfr + DMA1_EXEC_CYCLE8),
			readl(dpmu_sfr + DMA1_EXEC_CYCLE9),
			readl(dpmu_sfr + DMA1_EXEC_CYCLE10),
			readl(dpmu_sfr + DMA1_EXEC_CYCLE11));
	score_info("DPMU:%32s: %8x %8x %8x %8x\n",
			"DMA1_EXEC_CYCLE 12~15",
			readl(dpmu_sfr + DMA1_EXEC_CYCLE12),
			readl(dpmu_sfr + DMA1_EXEC_CYCLE13),
			readl(dpmu_sfr + DMA1_EXEC_CYCLE14),
			readl(dpmu_sfr + DMA1_EXEC_CYCLE15));
}

void score_dpmu_print_instruction_count(void)
{
	if (!performance_en)
		return;
	score_info("DPMU:%32s: %8x %8x\n",
			"MC_INST_COUNT(H/L)",
			readl(dpmu_sfr + MC_INST_COUNT_HIGH),
			readl(dpmu_sfr + MC_INST_COUNT_LOW));
	score_info("DPMU:%32s: %8x %8x\n",
			"KC1_INST_COUNT(H/L)",
			readl(dpmu_sfr + KC1_INST_COUNT_HIGH),
			readl(dpmu_sfr + KC1_INST_COUNT_LOW));
	score_info("DPMU:%32s: %8x %8x\n",
			"KC2_INST_COUNT(H/L)",
			readl(dpmu_sfr + KC2_INST_COUNT_HIGH),
			readl(dpmu_sfr + KC2_INST_COUNT_LOW));
}

void score_dpmu_print_interrupt_count(void)
{
	if (!performance_en)
		return;
	score_info("DPMU:%32s: %8x %8x\n",
			"MC_INTR_COUNT(H/L)",
			readl(dpmu_sfr + MC_INTR_COUNT_HIGH),
			readl(dpmu_sfr + MC_INTR_COUNT_LOW));
	score_info("DPMU:%32s: %8x %8x\n",
			"KC1_INTR_COUNT(H/L)",
			readl(dpmu_sfr + KC1_INTR_COUNT_HIGH),
			readl(dpmu_sfr + KC1_INTR_COUNT_LOW));
	score_info("DPMU:%32s: %8x %8x\n",
			"KC2_INTR_COUNT(H/L)",
			readl(dpmu_sfr + KC2_INTR_COUNT_HIGH),
			readl(dpmu_sfr + KC2_INTR_COUNT_LOW));
}

void score_dpmu_print_slave_err(void)
{
	if (!debug_en)
		return;
	score_info("DPMU:%32s: %8x\n",
			"SCQ_SLVERR_INFO",
			readl(dpmu_sfr + SCQ_SLVERR_INFO));
	score_info("DPMU:%32s: %8x\n",
			"TCMBUS_SLVERR_INFO",
			readl(dpmu_sfr + TCMBUS_SLVERR_INFO));
	score_info("DPMU:%32s: %8x\n",
			"CACHE_SLVERR_INFO",
			readl(dpmu_sfr + CACHE_SLVERR_INFO));
	score_info("DPMU:%32s: %8x\n",
			"IBC_SLVERR_INFO",
			readl(dpmu_sfr + IBC_SLVERR_INFO));
	score_info("DPMU:%32s: %8x\n",
			"AXI_SLVERR_INFO",
			readl(dpmu_sfr + AXI_SLVERR_INFO));
	score_info("DPMU:%32s: %8x\n",
			"DSP_SLVERR_INFO",
			readl(dpmu_sfr + DSP_SLVERR_INFO));
}

void score_dpmu_print_decode_err(void)
{
	if (!debug_en)
		return;
	score_info("DPMU:%32s: %8x %8x %8x\n",
			"DECERR_INFO(MC/KC1/KC2)",
			readl(dpmu_sfr + MC_DECERR_INFO),
			readl(dpmu_sfr + KC1_DECERR_INFO),
			readl(dpmu_sfr + KC2_DECERR_INFO));
}

void score_dpmu_print_pc(int count)
{
	int idx;

	if (!debug_en)
		return;
	for (idx = 0; idx < count; ++idx)
		score_info("DPMU:%32s: %8x %8x %8x\n",
				"PROG_COUNTER(MC/KC1/KC2)",
				readl(dpmu_sfr + MC_PROG_COUNTER),
				readl(dpmu_sfr + KC1_PROG_COUNTER),
				readl(dpmu_sfr + KC2_PROG_COUNTER));
}

void score_dpmu_print_system(void)
{
	score_info("DPMU:%32s: %8x\n",
			"SW_RESET",
			readl(dpmu_sfr + SW_RESET));
	score_info("DPMU:%32s: %8x\n",
			"RELEASE_DATE",
			readl(dpmu_sfr + RELEASE_DATE));
	score_info("DPMU:%32s: %8x %8x\n",
			"AXIM0_CACHE(r/w)",
			readl(dpmu_sfr + AXIM0_ARCACHE),
			readl(dpmu_sfr + AXIM0_AWCACHE));
	score_info("DPMU:%32s: %8x %8x\n",
			"AXIM2_CACHE(r/w)",
			readl(dpmu_sfr + AXIM2_ARCACHE),
			readl(dpmu_sfr + AXIM2_AWCACHE));
	score_info("DPMU:%32s: %8x %8x %8x\n",
			"WAKEUP(MC/KC1/KC2)",
			readl(dpmu_sfr + MC_WAKEUP),
			readl(dpmu_sfr + KC1_WAKEUP),
			readl(dpmu_sfr + KC2_WAKEUP));
	score_info("DPMU:%32s: %8x %8x\n",
			"SM_MCGEN/CLK_REQ",
			readl(dpmu_sfr + SM_MCGEN),
			readl(dpmu_sfr + CLK_REQ));
	score_info("DPMU:%32s: %8x\n",
			"APCPU_INTR_ENABLE",
			readl(dpmu_sfr + APCPU_INTR_ENABLE));
	score_info("DPMU:%32s: %8x %8x %8x\n",
			"INTR_ENABLE(MC/KC1/KC2)",
			readl(dpmu_sfr + MC_INTR_ENABLE),
			readl(dpmu_sfr + KC1_INTR_ENABLE),
			readl(dpmu_sfr + KC2_INTR_ENABLE));
	score_info("DPMU:%32s: %8x\n",
			"IVA_INTR_ENABLE",
			readl(dpmu_sfr + IVA_INTR_ENABLE));
	score_info("DPMU:%32s: %8x\n",
			"APCPU_SWI_STATUS",
			readl(dpmu_sfr + APCPU_SWI_STATUS));
	score_info("DPMU:%32s: %8x %8x %8x\n",
			"SWI_STATUS(MC/KC1/KC2)",
			readl(dpmu_sfr + MC_SWI_STATUS),
			readl(dpmu_sfr + KC1_SWI_STATUS),
			readl(dpmu_sfr + KC2_SWI_STATUS));
	score_info("DPMU:%32s: %8x %8x\n",
			"APCPU_INTR_STATUS(raw/masked)",
			readl(dpmu_sfr + APCPU_RAW_INTR_STATUS),
			readl(dpmu_sfr + APCPU_MASKED_INTR_STATUS));
	score_info("DPMU:%32s: %8x %8x\n",
			"MC_CACHE_BASEADDR(IC/DC)",
			readl(dpmu_sfr + MC_CACHE_IC_BASEADDR),
			readl(dpmu_sfr + MC_CACHE_DC_BASEADDR));
	score_info("DPMU:%32s: %8x %8x\n",
			"KC1_CACHE_BASEADDR(IC/DC)",
			readl(dpmu_sfr + KC1_CACHE_IC_BASEADDR),
			readl(dpmu_sfr + KC1_CACHE_DC_BASEADDR));
	score_info("DPMU:%32s: %8x %8x\n",
			"KC2_CACHE_BASEADDR(IC/DC)",
			readl(dpmu_sfr + KC2_CACHE_IC_BASEADDR),
			readl(dpmu_sfr + KC2_CACHE_DC_BASEADDR));
	score_info("DPMU:%32s: %8x %8x %8x\n",
			"DC_CCTRL_STATUS(MC/KC1/KC2)",
			readl(dpmu_sfr + MC_CACHE_DC_CCTRL_STATUS),
			readl(dpmu_sfr + KC1_CACHE_DC_CCTRL_STATUS),
			readl(dpmu_sfr + KC2_CACHE_DC_CCTRL_STATUS));
	score_info("DPMU:%32s: %8x\n",
			"SCQ_INTR_ENABLE",
			readl(dpmu_sfr + SCQ_INTR_ENABLE));
	score_info("DPMU:%32s: %8x %8x\n",
			"SCQ_INTR_STATUS(raw/masked)",
			readl(dpmu_sfr + SCQ_RAW_INTR_STATUS),
			readl(dpmu_sfr + SCQ_MASKED_INTR_STATUS));
#ifdef SCORE_EVT1
	score_info("DPMU:%32s: %8x %8x %8x %8x %8x\n",
			"PACKET_NUM(AP/MC/KC1/KC2/IVA)",
			readl(dpmu_sfr + SCQ_APCPU_PACKET_NUM),
			readl(dpmu_sfr + SCQ_MC_PACKET_NUM),
			readl(dpmu_sfr + SCQ_KC1_PACKET_NUM),
			readl(dpmu_sfr + SCQ_KC2_PACKET_NUM),
			readl(dpmu_sfr + SCQ_IVA_PACKET_NUM));
#endif
	score_info("DPMU:%32s: %8x %8x %8x\n",
			"PRINT(front/rear/count)",
			readl(dpmu_sfr + PRINT_QUEUE_FRONT_ADDR),
			readl(dpmu_sfr + PRINT_QUEUE_REAR_ADDR),
			readl(dpmu_sfr + PRINT_COUNT));
	score_info("DPMU:%32s: %8x %8x %8x\n",
			"DONE_CHECK(MC/KC1/KC2)",
			readl(dpmu_sfr + MC_INIT_DONE_CHECK),
			readl(dpmu_sfr + KC1_INIT_DONE_CHECK),
			readl(dpmu_sfr + KC2_INIT_DONE_CHECK));
	score_info("DPMU:%32s: %8x %8x\n",
			"MC_MALLOC(addr/size)",
			readl(dpmu_sfr + MC_MALLOC_BASE_ADDR),
			readl(dpmu_sfr + MC_MALLOC_SIZE));
	score_info("DPMU:%32s: %8x %8x %8x\n",
			"KC_MALLOC(kc1/kc2/size)",
			readl(dpmu_sfr + KC1_MALLOC_BASE_ADDR),
			readl(dpmu_sfr + KC2_MALLOC_BASE_ADDR),
			readl(dpmu_sfr + KC_MALLOC_SIZE));
	score_info("DPMU:%32s: %8x %8x\n",
			"PRINT(addr/size)",
			readl(dpmu_sfr + PRINT_BASE_ADDR),
			readl(dpmu_sfr + PRINT_SIZE));
	score_info("DPMU:%32s: %8x %8x\n",
			"PROFILER(mc/mc_size)",
			readl(dpmu_sfr + PROFILER_MC_BASE_ADDR),
			readl(dpmu_sfr + PROFILER_MC_SIZE));
	score_info("DPMU:%32s: %8x %8x %8x\n",
			"PROFILER(kc1/kc2/kc_size)",
			readl(dpmu_sfr + PROFILER_KC1_BASE_ADDR),
			readl(dpmu_sfr + PROFILER_KC2_BASE_ADDR),
			readl(dpmu_sfr + PROFILER_KC_SIZE));
	score_info("DPMU:%32s: %8x %8x\n",
			"PROFILER MC(head/tail)",
			readl(dpmu_sfr + PROFILER_MC_HEAD),
			readl(dpmu_sfr + PROFILER_MC_TAIL));
	score_info("DPMU:%32s: %8x %8x\n",
			"PROFILER KC1(head/tail)",
			readl(dpmu_sfr + PROFILER_KC1_HEAD),
			readl(dpmu_sfr + PROFILER_KC1_TAIL));
	score_info("DPMU:%32s: %8x %8x\n",
			"PROFILER KC2(head/tail)",
			readl(dpmu_sfr + PROFILER_KC2_HEAD),
			readl(dpmu_sfr + PROFILER_KC2_TAIL));
}

void score_dpmu_print_stall_count(void)
{
#ifdef SCORE_EVT1
	if (!stall_en)
		return;
	score_info("DPMU:%32s: %8x %8x %8x\n",
			"STALL ENABLE(MC/KC1/KC2)",
			readl(dpmu_sfr + MC_STALL_COUNT_ENABLE),
			readl(dpmu_sfr + KC1_STALL_COUNT_ENABLE),
			readl(dpmu_sfr + KC2_STALL_COUNT_ENABLE));
	score_info("DPMU:%32s: %8x %8x\n",
			"SFR STALL ENABLE(KC1/KC2)",
			readl(dpmu_sfr + KC1_SFR_STALL_COUNT_ENABLE),
			readl(dpmu_sfr + KC2_SFR_STALL_COUNT_ENABLE));
	score_info("DPMU:%32s: %8x %8x %8x %8x %8x\n",
			"MC_STALL(PROC/IC/DC/TCM/SFR)",
			readl(dpmu_sfr + MC_PROC_STALL_COUNT),
			readl(dpmu_sfr + MC_IC_STALL_COUNT),
			readl(dpmu_sfr + MC_DC_STALL_COUNT),
			readl(dpmu_sfr + MC_TCM_STALL_COUNT),
			readl(dpmu_sfr + MC_SFR_STALL_COUNT));
	score_info("DPMU:%32s: %8x %8x %8x %8x %8x\n",
			"KC1_STALL(PROC/IC/DC/TCM/SFR)",
			readl(dpmu_sfr + KC1_PROC_STALL_COUNT),
			readl(dpmu_sfr + KC1_IC_STALL_COUNT),
			readl(dpmu_sfr + KC1_DC_STALL_COUNT),
			readl(dpmu_sfr + KC1_TCM_STALL_COUNT),
			readl(dpmu_sfr + KC1_SFR_STALL_COUNT));
	score_info("DPMU:%32s: %8x %8x %8x %8x %8s\n",
			"KC2_STALL(PROC/IC/DC/TCM/SFR)",
			readl(dpmu_sfr + KC2_PROC_STALL_COUNT),
			readl(dpmu_sfr + KC2_IC_STALL_COUNT),
			readl(dpmu_sfr + KC2_DC_STALL_COUNT),
			readl(dpmu_sfr + KC2_TCM_STALL_COUNT),
			"NOT USE");
#endif
}

void score_dpmu_print_target(void)
{
	score_info("DPMU:%32s: %8x %8x\n",
			"PROLOGUE_DONE(KC1/KC2)",
			readl(dpmu_sfr + KC1_PROLOGUE_DONE),
			readl(dpmu_sfr + KC2_PROLOGUE_DONE));
	score_info("DPMU:%32s: %8x\n",
			"TOTAL_TILE_COUNT",
			readl(dpmu_sfr + TOTAL_TILE_COUNT));
	score_info("DPMU:%32s: %8x\n",
			"TOTAL_TCM_SIZE",
			readl(dpmu_sfr + TOTAL_TCM_SIZE));
	score_info("DPMU:%32s: %8x\n",
			"ENABLE_TCM_HALF",
			readl(dpmu_sfr + ENABLE_TCM_HALF));
	score_info("DPMU:%32s: %8x %8x %8x %8x %8x\n",
			"MC_ABORT_STATE(0-4)",
			readl(dpmu_sfr + MC_ABORT_STATE0),
			readl(dpmu_sfr + MC_ABORT_STATE1),
			readl(dpmu_sfr + MC_ABORT_STATE2),
			readl(dpmu_sfr + MC_ABORT_STATE3),
			readl(dpmu_sfr + MC_ABORT_STATE4));
	score_info("DPMU:%32s: %8x\n",
			"SCHEDULE_STATE",
			readl(dpmu_sfr + SCHEDULE_STATE));
	score_info("DPMU:%32s: %8x\n",
			"CORE_STATE",
			readl(dpmu_sfr + CORE_STATE));
	score_info("DPMU:%32s: %8x\n",
			"PRIORITY_STATE",
			readl(dpmu_sfr + PRIORITY_STATE));
	score_info("DPMU:%32s: %8x\n",
			"MC_KERNEL_ID",
			readl(dpmu_sfr + MC_KERNEL_ID));
	score_info("DPMU:%32s: %8x\n",
			"MC_ENTRY_FLAG",
			readl(dpmu_sfr + MC_ENTRY_FLAG));
	score_info("DPMU:%32s: %8x %8x\n",
			"INDIRECT(FUNC/ADDR)",
			readl(dpmu_sfr + INDIRECT_FUNC),
			readl(dpmu_sfr + INDIRECT_ADDR));
	score_info("DPMU:%32s: %8x\n",
			"MC_REQUEST_TYPE",
			readl(dpmu_sfr + MC_REQUEST_TYPE));
	score_info("DPMU:%32s: %8x\n",
			"MC_TCM_MALLOC_STATE",
			readl(dpmu_sfr + MC_TCM_MALLOC_STATE));
	score_info("DPMU:%32s: %8x %8x\n",
			"CURRENT_KERNEL(KC1/KC2)",
			readl(dpmu_sfr + KC1_CURRENT_KERNEL),
			readl(dpmu_sfr + KC2_CURRENT_KERNEL));
}

void score_dpmu_print_debug(void)
{
	int idx;

	for (idx = 0; idx < 32; idx += 4)
		score_info("DPMU:%26s %02d-%02d: %8x %8x %8x %8x\n",
				"USERDEFINED", idx, idx + 3,
				readl(dpmu_sfr + USERDEFINED(idx)),
				readl(dpmu_sfr + USERDEFINED(idx + 1)),
				readl(dpmu_sfr + USERDEFINED(idx + 2)),
				readl(dpmu_sfr + USERDEFINED(idx + 3)));
}

void score_dpmu_print_all(void)
{
	score_dpmu_print_dbg_intr();
	score_dpmu_print_monitor_status();
	score_dpmu_print_mc_cache();
	score_dpmu_print_kc1_cache();
	score_dpmu_print_kc2_cache();
	score_dpmu_print_axi();
	score_dpmu_print_dma0();
	score_dpmu_print_dma1();
	score_dpmu_print_instruction_count();
	score_dpmu_print_interrupt_count();
	score_dpmu_print_slave_err();
	score_dpmu_print_decode_err();
	score_dpmu_print_pc(10);
	score_dpmu_print_system();
	score_dpmu_print_stall_count();
	score_dpmu_print_target();
	score_dpmu_print_debug();
}

void score_dpmu_debug_enable_on(int on)
{
	score_enter();
	debug_en = on;
	score_leave();
}

int score_dpmu_debug_enable_check(void)
{
	score_check();
	return debug_en;
}

void score_dpmu_performance_enable_on(int on)
{
	score_enter();
	performance_en = on;
	score_leave();
}

int score_dpmu_performance_enable_check(void)
{
	score_check();
	return performance_en;
}

void score_dpmu_stall_enable_on(int on)
{
	score_enter();
#ifdef SCORE_EVT1
	stall_en = on;
#else
	score_info("stall count can not be used on EVT0\n");
#endif
	score_leave();
}

int score_dpmu_stall_enable_check(void)
{
	score_check();
	return stall_en;
}

void score_dpmu_enable(void)
{
	score_enter();
	if (debug_en)
		writel(0x1, dpmu_sfr + DBG_MONITOR_ENABLE);
	if (performance_en)
		writel(0x7ffff, dpmu_sfr + PERF_MONITOR_ENABLE);

#ifdef SCORE_EVT1
	if (stall_en) {
		writel(0x1, dpmu_sfr + MC_STALL_COUNT_ENABLE);
		writel(0x1, dpmu_sfr + KC1_SFR_STALL_COUNT_ENABLE);
		writel(0x1, dpmu_sfr + KC1_STALL_COUNT_ENABLE);
		/* writel(0x1, dpmu_sfr + KC2_SFR_STALL_COUNT_ENABLE); */
		writel(0x1, dpmu_sfr + KC2_STALL_COUNT_ENABLE);
	}
#endif
	score_leave();
}

void score_dpmu_disable(void)
{
	score_enter();
	if (debug_en)
		writel(0x0, dpmu_sfr + DBG_MONITOR_ENABLE);
	if (performance_en)
		writel(0x0, dpmu_sfr + PERF_MONITOR_ENABLE);

#ifdef SCORE_EVT1
	if (stall_en) {
		writel(0x0, dpmu_sfr + MC_STALL_COUNT_ENABLE);
		writel(0x0, dpmu_sfr + KC1_SFR_STALL_COUNT_ENABLE);
		writel(0x0, dpmu_sfr + KC1_STALL_COUNT_ENABLE);
		/* writel(0x0, dpmu_sfr + KC2_SFR_STALL_COUNT_ENABLE); */
		writel(0x0, dpmu_sfr + KC2_STALL_COUNT_ENABLE);
	}
#endif
	score_leave();
}

void score_dpmu_init(void __iomem *sfr)
{
	dpmu_sfr = sfr;
}
