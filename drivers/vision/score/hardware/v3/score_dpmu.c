/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
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
static int debug_on = 1;
static int performance_on;

void score_dpmu_print_system(void)
{
	score_info("%24s:%8x %24s:%8x\n",
			"RELEASE_DATE",
			readl(dpmu_sfr + RELEASE_DATE),
			"SW_RESET",
			readl(dpmu_sfr + SW_RESET));
	score_info("%24s:%8x %24s:%8x\n",
			"DSPM_MCGEN",
			readl(dpmu_sfr + DSPM_MCGEN),
			"DSPM_ALWAYS_ON",
			readl(dpmu_sfr + DSPM_ALWAYS_ON));
	score_info("%24s:%8x %24s:%8x\n",
			"DSPS_MCGEN",
			readl(dpmu_sfr + DSPS_MCGEN),
			"DSPS_ALWAYS_ON",
			readl(dpmu_sfr + DSPS_ALWAYS_ON));
	score_info("%24s:%8x %24s:%8x\n",
			"DSPM_QCH_INFO",
			readl(dpmu_sfr + DSPM_QCH_INFO),
			"DSPS_QCH_INFO",
			readl(dpmu_sfr + DSPS_QCH_INFO));
	score_info("%24s:%8x %24s:%8x\n",
			"AXIM0_SIGNAL",
			readl(dpmu_sfr + AXIM0_SIGNAL),
			"AXIM2_SIGNAL",
			readl(dpmu_sfr + AXIM2_SIGNAL));

	score_info("%24s:%8x %24s:%8x\n",
			"APCPU_INTR_ENABLE",
			readl(dpmu_sfr + APCPU_INTR_ENABLE),
			"TS_INTR_ENABLE",
			readl(dpmu_sfr + TS_INTR_ENABLE));
	score_info("%24s:%8x %24s:%8x\n",
			"IVA_INTR_ENABLE",
			readl(dpmu_sfr + IVA_INTR_ENABLE),
			"NPU_INTR_ENABLE",
			readl(dpmu_sfr + NPU_INTR_ENABLE));
	score_info("%24s:%8x %24s:%8x\n",
			"BR1_INTR_ENABLE",
			readl(dpmu_sfr + BR1_INTR_ENABLE),
			"BR2_INTR_ENABLE",
			readl(dpmu_sfr + BR2_INTR_ENABLE));
	score_info("%24s:%8x %24s:%8x\n",
			"TS_CACHE_INTR_ENABLE",
			readl(dpmu_sfr + TS_CACHE_INTR_ENABLE),
			"BR_CACHE_INTR_ENABLE",
			readl(dpmu_sfr + BR_CACHE_INTR_ENABLE));
	score_info("%24s:%8x\n",
			"SCQ_INTR_ENABLE",
			readl(dpmu_sfr + SCQ_INTR_ENABLE));
	score_info("%24s:%8x %24s:%8x\n",
			"DSPM_DBG_INTR_ENABLE",
			readl(dpmu_sfr + DSPM_DBG_INTR_ENABLE),
			"DSPS_DBG_INTR_ENABLE",
			readl(dpmu_sfr + DSPS_DBG_INTR_ENABLE));
	score_info("%24s:%8x %24s:%8x\n",
			"DSPM_DBG_MONITOR_ENABLE",
			readl(dpmu_sfr + DSPM_DBG_MONITOR_ENABLE),
			"DSPS_DBG_MONITOR_ENABLE",
			readl(dpmu_sfr + DSPS_DBG_MONITOR_ENABLE));
	score_info("%24s:%8x %24s:%8x\n",
			"DSPM_PERF_MONITOR_ENABLE",
			readl(dpmu_sfr + DSPM_PERF_MONITOR_ENABLE),
			"DSPS_PERF_MONITOR_ENABLE",
			readl(dpmu_sfr + DSPS_PERF_MONITOR_ENABLE));

	score_info("%24s:%8x %24s:%8x\n",
			"APCPU_INTR_STATUS",
			readl(dpmu_sfr + APCPU_RAW_INTR_STATUS),
			"SCQ_INTR_STATUS",
			readl(dpmu_sfr + SCQ_INTR_STATUS));
	score_info("%24s:%8x %24s:%8x\n",
			"ISP_INTR_STATUS",
			readl(dpmu_sfr + ISP_INTR_STATUS),
			"SHUB_INTR_STATUS",
			readl(dpmu_sfr + SHUB_INTR_STATUS));
	score_info("%24s:%8x %24s:%8x\n",
			"APCPU_SWI_STATUS",
			readl(dpmu_sfr + APCPU_SWI_STATUS),
			"TS_SWI_STATUS",
			readl(dpmu_sfr + TS_SWI_STATUS));
	score_info("%24s:%8x %24s:%8x\n",
			"BR1_SWI_STATUS",
			readl(dpmu_sfr + BR1_SWI_STATUS),
			"BR2_SWI_STATUS",
			readl(dpmu_sfr + BR2_SWI_STATUS));
	score_info("%24s:%8x\n",
			"NPU_SWI_STATUS",
			readl(dpmu_sfr + NPU_SWI_STATUS));
	score_info("%24s:%8x %24s:%8x\n",
			"TS_CACHE_INTR_STATUS",
			readl(dpmu_sfr + TS_CACHE_RAW_INTR),
			"BR_CACHE_INTR_STATUS",
			readl(dpmu_sfr + BR_CACHE_RAW_INTR));
	score_info("%24s:%8x %24s:%8x\n",
			"DSPM_DBG_INTR_STATUS",
			readl(dpmu_sfr + DSPM_DBG_RAW_INTR_STATUS),
			"DSPS_DBG_INTR_STATUS",
			readl(dpmu_sfr + DSPS_DBG_RAW_INTR_STATUS));

	score_info("%24s:%8x %24s:%8x\n",
			"BR0_STA_MONITOR",
			readl(dpmu_sfr + BR0_STA_MONITOR),
			"BR1_STA_MONITOR",
			readl(dpmu_sfr + BR1_STA_MONITOR));
	score_info("%24s:%8x %8x %8x %8x\n",
			"MPU_MONITOR(0-3)",
			readl(dpmu_sfr + MPU_MONITOR0),
			readl(dpmu_sfr + MPU_MONITOR1),
			readl(dpmu_sfr + MPU_MONITOR2),
			readl(dpmu_sfr + MPU_MONITOR3));
	score_info("%24s:%8x %8x %8x %8x\n",
			"MPU_MONITOR(4-7)",
			readl(dpmu_sfr + MPU_MONITOR4),
			readl(dpmu_sfr + MPU_MONITOR5),
			readl(dpmu_sfr + MPU_MONITOR6),
			readl(dpmu_sfr + MPU_MONITOR7));
	score_info("%24s:%8x\n",
			"TS_CACHE_STATUS",
			readl(dpmu_sfr + TS_CACHE_STATUS));
	score_info("%24s:%8x %24s:%8x\n",
			"TS_CACHE_IC_BASE_ADDR",
			readl(dpmu_sfr + TS_CACHE_IC_BASE_ADDR),
			"TS_CACHE_IC_CODE_SIZE",
			readl(dpmu_sfr + TS_CACHE_IC_CODE_SIZE));
	score_info("%24s:%8x %24s:%8x\n",
			"TS_CACHE_DC_BASE_ADDR",
			readl(dpmu_sfr + TS_CACHE_DC_BASE_ADDR),
			"TS_CACHE_DC_DATA_SIZE",
			readl(dpmu_sfr + TS_CACHE_DC_DATA_SIZE));
	score_info("%24s:%8x %24s:%8x\n",
			"TS_CACHE_STACK_START",
			readl(dpmu_sfr + TS_CACHE_DC_STACK_START_ADDR),
			"TS_CACHE_STACK_END",
			readl(dpmu_sfr + TS_CACHE_DC_STACK_END_ADDR));
	score_info("%24s:%8x %24s:%8x\n",
			"TS_CACHE_IC_CTRL_STATUS",
			readl(dpmu_sfr + TS_CACHE_IC_CTRL_STATUS),
			"TS_CACHE_DC_CTRL_STATUS",
			readl(dpmu_sfr + TS_CACHE_DC_CTRL_STATUS));

	score_info("%24s:%8x %24s:%8x\n",
			"BR_CACHE_MODE",
			readl(dpmu_sfr + BR_CACHE_MODE),
			"BR_CACHE_STATUS",
			readl(dpmu_sfr + BR_CACHE_STATUS));
	score_info("%24s:%8x %24s:%8x\n",
			"BR_CACHE_IC_BASE_ADDR",
			readl(dpmu_sfr + BR_CACHE_IC_BASE_ADDR),
			"BR_CACHE_IC_CODE_SIZE",
			readl(dpmu_sfr + BR_CACHE_IC_CODE_SIZE));
	score_info("%24s:%8x %24s:%8x\n",
			"BR1_CACHE_DC_BASE_ADDR",
			readl(dpmu_sfr + BR1_CACHE_DC_BASE_ADDR),
			"BR1_CACHE_DC_DATA_SIZE",
			readl(dpmu_sfr + BR1_CACHE_DC_DATA_SIZE));
	score_info("%24s:%8x %24s:%8x\n",
			"BR2_CACHE_DC_BASE_ADDR",
			readl(dpmu_sfr + BR2_CACHE_DC_BASE_ADDR),
			"BR2_CACHE_DC_DATA_SIZE",
			readl(dpmu_sfr + BR2_CACHE_DC_DATA_SIZE));
	score_info("%24s:%8x %24s:%8x\n",
			"BR1_CACHE_STACK_START",
			readl(dpmu_sfr + BR1_CACHE_DC_STACK_START_ADDR),
			"BR1_CACHE_STACK_END",
			readl(dpmu_sfr + BR1_CACHE_DC_STACK_END_ADDR));
	score_info("%24s:%8x %24s:%8x\n",
			"BR2_CACHE_STACK_START",
			readl(dpmu_sfr + BR2_CACHE_DC_STACK_START_ADDR),
			"BR2_CACHE_STACK_END",
			readl(dpmu_sfr + BR2_CACHE_DC_STACK_END_ADDR));
	score_info("%24s:%8x\n",
			"BR_CACHE_IC_CTRL_STATUS",
			readl(dpmu_sfr + BR_CACHE_IC_CTRL_STATUS));
	score_info("%24s:%8x %24s:%8x\n",
			"BR1_CACHE_DC_CTRL_STATUS",
			readl(dpmu_sfr + BR1_CACHE_DC_CTRL_STATUS),
			"BR2_CACHE_DC_CTRL_STATUS",
			readl(dpmu_sfr + BR2_CACHE_DC_CTRL_STATUS));

	score_info("%24s:%8x %24s:%8x\n",
			"SCQ_CMD_NUMBER",
			readl(dpmu_sfr + SCQ_QUEUE_CMD_NUMBER),
			"SCQ_CMD_VALID",
			readl(dpmu_sfr + SCQ_QUEUE_CMD_VALID));
	score_info("%24s:%8x %24s:%8x\n",
			"SCQ_CMD_NUMBER1",
			readl(dpmu_sfr + SCQ_QUEUE_CMD_NUMBER1),
			"SCQ_CMD_NUMBER2",
			readl(dpmu_sfr + SCQ_QUEUE_CMD_NUMBER2));
	score_info("%24s:%8x %24s:%8x\n",
			"SCQ_READ_LEFT_COUNT",
			readl(dpmu_sfr + SCQ_READ_SRAM_LEFT_COUNT),
			"SCQ_WRITE_LEFT_COUNT",
			readl(dpmu_sfr + SCQ_WRITE_SRAM_LEFT_COUNT));

	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA_QUEUE_STATUS(0-3)",
			readl(dpmu_sfr + DMA0_QUEUE_STATUS),
			readl(dpmu_sfr + DMA1_QUEUE_STATUS),
			readl(dpmu_sfr + DMA2_QUEUE_STATUS),
			readl(dpmu_sfr + DMA3_QUEUE_STATUS));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA_PUSH_COUNT(0-3)",
			readl(dpmu_sfr + DMA0_PUSH_COUNT),
			readl(dpmu_sfr + DMA1_PUSH_COUNT),
			readl(dpmu_sfr + DMA2_PUSH_COUNT),
			readl(dpmu_sfr + DMA3_PUSH_COUNT));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA_POP_COUNT(0-3)",
			readl(dpmu_sfr + DMA0_POP_COUNT),
			readl(dpmu_sfr + DMA1_POP_COUNT),
			readl(dpmu_sfr + DMA2_POP_COUNT),
			readl(dpmu_sfr + DMA3_POP_COUNT));

	score_info("%24s:%8x %8x %8x\n",
			"INIT_DONE(ts/b1/b2)",
			readl(dpmu_sfr + TS_INIT_DONE_CHECK),
			readl(dpmu_sfr + BR1_INIT_DONE_CHECK),
			readl(dpmu_sfr + BR2_INIT_DONE_CHECK));
	score_info("%24s:%8x %8x\n",
			"TS_MALLOC(addr/size)",
			readl(dpmu_sfr + TS_MALLOC_BASE_ADDR),
			readl(dpmu_sfr + TS_MALLOC_SIZE));
	score_info("%24s:%8x %8x %8x\n",
			"BR_MALLOC(b1/b2/size)",
			readl(dpmu_sfr + BR1_MALLOC_BASE_ADDR),
			readl(dpmu_sfr + BR2_MALLOC_BASE_ADDR),
			readl(dpmu_sfr + BR_MALLOC_SIZE));
	score_info("%24s:%8x %8x %8x %8x\n",
			"PRINT(addr/size/f/r)",
			readl(dpmu_sfr + PRINT_BASE_ADDR),
			readl(dpmu_sfr + PRINT_SIZE),
			readl(dpmu_sfr + PRINT_QUEUE_FRONT_ADDR),
			readl(dpmu_sfr + PRINT_QUEUE_REAR_ADDR));
	score_info("%24s:%8x %8x %8x\n",
			"PROFILER(ts/b1/b2)",
			readl(dpmu_sfr + PROFILER_TS_BASE_ADDR),
			readl(dpmu_sfr + PROFILER_BR1_BASE_ADDR),
			readl(dpmu_sfr + PROFILER_BR2_BASE_ADDR));
	score_info("%24s:%8x %8x %8x\n",
			"PROFILER_HEAD(ts/b1/b2)",
			readl(dpmu_sfr + PROFILER_TS_HEAD),
			readl(dpmu_sfr + PROFILER_BR1_HEAD),
			readl(dpmu_sfr + PROFILER_BR2_HEAD));
}

void score_dpmu_print_pc(int count)
{
	int idx;

	if (!debug_on)
		return;

	for (idx = 0; idx < count; ++idx)
		score_info("%21s[%d]:%8x %8x\n",
				"PC(ts/b1)", idx,
				readl(dpmu_sfr + TS_PROGRAM_COUNTER),
				readl(dpmu_sfr + BR1_PROGRAM_COUNTER));
}

void score_dpmu_print_debug(void)
{
	if (!debug_on)
		return;

	score_dpmu_print_pc(10);
	score_info("%24s:%8x\n",
			"TS_THREAD_INFO",
			readl(dpmu_sfr + TS_INFO));
	score_info("%24s:%8x\n",
			"SCQ_SLVERR_INFO",
			readl(dpmu_sfr + SCQ_SLVERR_INFO));
	score_info("%24s:%8x\n",
			"TS_CACHE_SLVERR_INFO",
			readl(dpmu_sfr + TSCACHE_SLVERR_INFO0));
	score_info("%24s:%8x %8x %8x\n",
			"TS_CACHE_ERR(pm/dm/sp)",
			readl(dpmu_sfr + TSCACHE_SLVERR_INFO1),
			readl(dpmu_sfr + TSCACHE_SLVERR_INFO2),
			readl(dpmu_sfr + TSCACHE_SLVERR_INFO3));
#if defined(SCORE_EVT1)
	score_info("%24s:%8x %8x\n",
			"TS_ACTIVE_COUNT",
			readl(dpmu_sfr + TS_ACTIVE_COUNT_HIGH),
			readl(dpmu_sfr + TS_ACTIVE_COUNT_LOW));
#endif
	score_info("%24s:%8x\n",
			"BR_CACHE_SLVERR_INFO",
			readl(dpmu_sfr + BRCACHE_SLVERR_INFO0));
	score_info("%24s:%8x %8x %8x\n",
			"BR1_CACHE_ERR(pm/dm/sp)",
			readl(dpmu_sfr + BRCACHE_SLVERR_INFO1),
			readl(dpmu_sfr + BRCACHE_SLVERR_INFO2),
			readl(dpmu_sfr + BRCACHE_SLVERR_INFO3));
	score_info("%24s:%8x %8x %8x\n",
			"BR2_CACHE_ERR(pm/dm/sp)",
			readl(dpmu_sfr + BRCACHE_SLVERR_INFO4),
			readl(dpmu_sfr + BRCACHE_SLVERR_INFO5),
			readl(dpmu_sfr + BRCACHE_SLVERR_INFO6));
	score_info("%24s:%8x\n",
			"TCM_BOUNDARY_ERRINFO",
			readl(dpmu_sfr + TCMBOUNDARY_ERRINFO));
#if defined(SCORE_EVT1)
	score_info("%24s:%8x %8x\n",
			"BR1_ACTIVE_COUNT",
			readl(dpmu_sfr + BR1_ACTIVE_COUNT_HIGH),
			readl(dpmu_sfr + BR1_ACTIVE_COUNT_LOW));
	score_info("%24s:%8x %8x\n",
			"BR2_ACTIVE_COUNT",
			readl(dpmu_sfr + BR2_ACTIVE_COUNT_HIGH),
			readl(dpmu_sfr + BR2_ACTIVE_COUNT_LOW));
#endif
}

void score_dpmu_print_target(void)
{
	score_info("%24s:%8x %8x\n",
			"PROLOGUE_DONE(b1/b2)",
			readl(dpmu_sfr + BR1_PROLOGUE_DONE),
			readl(dpmu_sfr + BR2_PROLOGUE_DONE));
	score_info("%24s:%8x %24s:%8x\n",
			"TOTAL_TILE_COUNT",
			readl(dpmu_sfr + TOTAL_TILE_COUNT),
			"TOTAL_TCM_SIZE",
			readl(dpmu_sfr + TOTAL_TCM_SIZE));
	score_info("%24s:%8x %24s:%8x\n",
			"ENABLE_TCM_HALF",
			readl(dpmu_sfr + ENABLE_TCM_HALF),
			"TS_VSIZE_TYPE",
			readl(dpmu_sfr + TS_VSIZE_TYPE));
	score_info("%24s:%8x %8x\n",
			"SCHEDULE_COUNT(n/h)",
			readl(dpmu_sfr + SCHEDULE_NORMAL_COUNT),
			readl(dpmu_sfr + SCHEDULE_HIGH_COUNT));
	score_info("%24s:%8x\n",
			"SCHEDULE_BITMAP",
			readl(dpmu_sfr + SCHEDULE_BITMAP));
	score_info("%24s:%8x %8x %8x %8x\n",
			"SCHEDULE_WRPOS(n/h)",
			readl(dpmu_sfr + SCHEDULE_NORMAL_WPOS),
			readl(dpmu_sfr + SCHEDULE_NORMAL_RPOS),
			readl(dpmu_sfr + SCHEDULE_HIGH_WPOS),
			readl(dpmu_sfr + SCHEDULE_HIGH_RPOS));
	score_info("%24s:%8x %8x %8x %8x %8x\n",
			"TS_ABORT_STATE(0-4)",
			readl(dpmu_sfr + TS_ABORT_STATE0),
			readl(dpmu_sfr + TS_ABORT_STATE1),
			readl(dpmu_sfr + TS_ABORT_STATE2),
			readl(dpmu_sfr + TS_ABORT_STATE3),
			readl(dpmu_sfr + TS_ABORT_STATE4));
	score_info("%24s:%8x\n",
			"SCHEDULE_STATE",
			readl(dpmu_sfr + SCHEDULE_STATE));
	score_info("%24s:%8x %24s:%8x\n",
			"CORE_STATE",
			readl(dpmu_sfr + CORE_STATE),
			"PRIORITY_STATE",
			readl(dpmu_sfr + PRIORITY_STATE));
	score_info("%24s:%8x %24s:%8x\n",
			"TS_KERNEL_ID",
			readl(dpmu_sfr + TS_KERNEL_ID),
			"TS_ENTRY_FLAG",
			readl(dpmu_sfr + TS_ENTRY_FLAG));
	score_info("%24s:%8x %24s:%8x\n",
			"INDIRECT_FUNC",
			readl(dpmu_sfr + INDIRECT_FUNC),
			"INDIRECT_ADDR",
			readl(dpmu_sfr + INDIRECT_ADDR));
	score_info("%24s:%8x %24s:%8x\n",
			"TS_REQUEST_TYPE",
			readl(dpmu_sfr + TS_REQUEST_TYPE),
			"TS_TCM_MALLOC_STATE",
			readl(dpmu_sfr + TS_TCM_MALLOC_STATE));
	score_info("%24s:%8x %8x\n",
			"CURRENT_KERNEL(b1/b2)",
			readl(dpmu_sfr + BR1_CURRENT_KERNEL),
			readl(dpmu_sfr + BR2_CURRENT_KERNEL));
}

void score_dpmu_print_userdefined(void)
{
	int idx;

	for (idx = 0; idx < USERDEFINED_COUNT; idx += 4)
		score_info("%19s %02d-%02d:%8x %8x %8x %8x\n",
				"USERDEFINED", idx, idx + 3,
				readl(dpmu_sfr + USERDEFINED(idx)),
				readl(dpmu_sfr + USERDEFINED(idx + 1)),
				readl(dpmu_sfr + USERDEFINED(idx + 2)),
				readl(dpmu_sfr + USERDEFINED(idx + 3)));
}

void score_dpmu_print_performance(void)
{
	if (!performance_on)
		return;

	score_info("%24s:%8x %8x %8x %8x\n",
			"RDBEAT_COUNT(m0-3)",
			readl(dpmu_sfr + AXIM0_RDBEAT_COUNT),
			readl(dpmu_sfr + AXIM1_RDBEAT_COUNT),
			readl(dpmu_sfr + AXIM2_RDBEAT_COUNT),
			readl(dpmu_sfr + AXIM3_RDBEAT_COUNT));
	score_info("%24s:%8x %8x\n",
			"RDBEAT_COUNT(s0-1)",
			readl(dpmu_sfr + AXIS1_RDBEAT_COUNT),
			readl(dpmu_sfr + AXIS2_RDBEAT_COUNT));
	score_info("%24s:%8x %8x %8x %8x\n",
			"WRBEAT_COUNT(m0-3)",
			readl(dpmu_sfr + AXIM0_WRBEAT_COUNT),
			readl(dpmu_sfr + AXIM1_WRBEAT_COUNT),
			readl(dpmu_sfr + AXIM2_WRBEAT_COUNT),
			readl(dpmu_sfr + AXIM3_WRBEAT_COUNT));
	score_info("%24s:%8x %8x\n",
			"WRBEAT_COUNT(s0-1)",
			readl(dpmu_sfr + AXIS1_WRBEAT_COUNT),
			readl(dpmu_sfr + AXIS2_WRBEAT_COUNT));
	score_info("%24s:%8x %8x %8x %8x\n",
			"RD_MAX_MO_COUNT(m0-3)",
			readl(dpmu_sfr + AXIM0_RD_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIM1_RD_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIM2_RD_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIM3_RD_MAX_MO_COUNT));
	score_info("%24s:%8x %8x\n",
			"RD_MAX_MO_COUNT(s0-1)",
			readl(dpmu_sfr + AXIS1_RD_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIS2_RD_MAX_MO_COUNT));
	score_info("%24s:%8x %8x %8x %8x\n",
			"WR_MAX_MO_COUNT(m0-3)",
			readl(dpmu_sfr + AXIM0_WR_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIM1_WR_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIM2_WR_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIM3_WR_MAX_MO_COUNT));
	score_info("%24s:%8x %8x\n",
			"WR_MAX_MO_COUNT(s0-1)",
			readl(dpmu_sfr + AXIS1_WR_MAX_MO_COUNT),
			readl(dpmu_sfr + AXIS2_WR_MAX_MO_COUNT));
	score_info("%24s:%8x %8x %8x %8x\n",
			"RD_MAX_REQ_LAT(m0-3)",
			readl(dpmu_sfr + AXIM0_RD_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIM1_RD_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIM2_RD_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIM3_RD_MAX_REQ_LAT));
	score_info("%24s:%8x %8x\n",
			"RD_MAX_REQ_LAT(s0-1)",
			readl(dpmu_sfr + AXIS1_RD_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIS2_RD_MAX_REQ_LAT));
	score_info("%24s:%8x %8x %8x %8x\n",
			"WR_MAX_REQ_LAT(m0-3)",
			readl(dpmu_sfr + AXIM0_WR_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIM1_WR_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIM2_WR_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIM3_WR_MAX_REQ_LAT));
	score_info("%24s:%8x %8x\n",
			"WR_MAX_REQ_LAT(s0-1)",
			readl(dpmu_sfr + AXIS1_WR_MAX_REQ_LAT),
			readl(dpmu_sfr + AXIS2_WR_MAX_REQ_LAT));
	score_info("%24s:%8x %8x %8x %8x\n",
			"RD_MAX_AR2R_LAT(m0-3)",
			readl(dpmu_sfr + AXIM0_RD_MAX_AR2R_LAT),
			readl(dpmu_sfr + AXIM1_RD_MAX_AR2R_LAT),
			readl(dpmu_sfr + AXIM2_RD_MAX_AR2R_LAT),
			readl(dpmu_sfr + AXIM3_RD_MAX_AR2R_LAT));
	score_info("%24s:%8x %8x\n",
			"RD_MAX_AR2R_LAT(s0-1)",
			readl(dpmu_sfr + AXIS1_RD_MAX_AR2R_LAT),
			readl(dpmu_sfr + AXIS2_RD_MAX_AR2R_LAT));
	score_info("%24s:%8x %8x %8x %8x\n",
			"RD_MAX_R2R_LAT(m0-3)",
			readl(dpmu_sfr + AXIM0_RD_MAX_R2R_LAT),
			readl(dpmu_sfr + AXIM1_RD_MAX_R2R_LAT),
			readl(dpmu_sfr + AXIM2_RD_MAX_R2R_LAT),
			readl(dpmu_sfr + AXIM3_RD_MAX_R2R_LAT));
	score_info("%24s:%8x %8x\n",
			"RD_MAX_R2R_LAT(s0-1)",
			readl(dpmu_sfr + AXIS1_RD_MAX_R2R_LAT),
			readl(dpmu_sfr + AXIS2_RD_MAX_R2R_LAT));
#if defined(SCORE_EVT1)
	score_info("%24s:%8x\n",
			"BRCACHE_RDBEAT_COUNT",
			readl(dpmu_sfr + AXIM0_BRCACHE_RDBEAT_COUNT));
	score_info("%24s:%8x\n",
			"BRCACHE_WRBEAT_COUNT",
			readl(dpmu_sfr + AXIM0_BRCACHE_WRBEAT_COUNT));
	score_info("%24s:%8x\n",
			"BRCACHE_RD_MAX_MO_COUNT",
			readl(dpmu_sfr + AXIM0_BRCACHE_RD_MAX_MO_COUNT));
	score_info("%24s:%8x\n",
			"BRCACHE_WR_MAX_MO_COUNT",
			readl(dpmu_sfr + AXIM0_BRCACHE_WR_MAX_MO_COUNT));
	score_info("%24s:%8x\n",
			"BRCACHE_RD_MAX_REQ_LAT",
			readl(dpmu_sfr + AXIM0_BRCACHE_RD_MAX_REQ_LAT));
	score_info("%24s:%8x\n",
			"BRCACHE_WR_MAX_REQ_LAT",
			readl(dpmu_sfr + AXIM0_BRCACHE_WR_MAX_REQ_LAT));
	score_info("%24s:%8x\n",
			"BRCACHE_RD_MAX_AR2R_LAT",
			readl(dpmu_sfr + AXIM0_BRCACHE_RD_MAX_AR2R_LAT));
	score_info("%24s:%8x\n",
			"BRCACHE_RD_MAX_R2R_LAT",
			readl(dpmu_sfr + AXIM0_BRCACHE_RD_MAX_R2R_LAT));
#endif
	score_info("%24s:%8x %8x\n",
			"TS_INST_COUNT",
			readl(dpmu_sfr + TS_INST_COUNT_HIGH),
			readl(dpmu_sfr + TS_INST_COUNT_LOW));
	score_info("%24s:%8x %8x\n",
			"TS_INTR_COUNT",
			readl(dpmu_sfr + TS_INTR_COUNT_HIGH),
			readl(dpmu_sfr + TS_INTR_COUNT_LOW));
	score_info("%24s:%8x %8x\n",
			"BR1_INST_COUNT",
			readl(dpmu_sfr + BR1_INST_COUNT_HIGH),
			readl(dpmu_sfr + BR1_INST_COUNT_LOW));
	score_info("%24s:%8x %8x\n",
			"BR2_INST_COUNT",
			readl(dpmu_sfr + BR2_INST_COUNT_HIGH),
			readl(dpmu_sfr + BR2_INST_COUNT_LOW));
	score_info("%24s:%8x %8x\n",
			"BR1_INTR_COUNT",
			readl(dpmu_sfr + BR1_INTR_COUNT_HIGH),
			readl(dpmu_sfr + BR1_INTR_COUNT_LOW));
	score_info("%24s:%8x %8x\n",
			"BR2_INTR_COUNT",
			readl(dpmu_sfr + BR2_INTR_COUNT_HIGH),
			readl(dpmu_sfr + BR2_INTR_COUNT_LOW));
	score_info("%24s:%8x %8x %8x\n",
			"TS_ICACHE(access/miss)",
			readl(dpmu_sfr + TS_ICACHE_ACCESS_COUNT_HIGH),
			readl(dpmu_sfr + TS_ICACHE_ACCESS_COUNT_LOW),
			readl(dpmu_sfr + TS_ICACHE_MISS_COUNT));
	score_info("%24s:%8x %8x\n",
			"TS_DCACHE(access/miss)",
			readl(dpmu_sfr + TS_DCACHE_ACCESS_COUNT),
			readl(dpmu_sfr + TS_DCACHE_MISS_COUNT));
	score_info("%24s:%8x %8x %8x\n",
			"BR1_ICACHE(access/miss)",
			readl(dpmu_sfr + BR1_ICACHE_ACCESS_COUNT_HIGH),
			readl(dpmu_sfr + BR1_ICACHE_ACCESS_COUNT_LOW),
			readl(dpmu_sfr + BR1_ICACHE_MISS_COUNT));
	score_info("%24s:%8x %8x %8x\n",
			"BR2_ICACHE(access/miss)",
			readl(dpmu_sfr + BR2_ICACHE_ACCESS_COUNT_HIGH),
			readl(dpmu_sfr + BR2_ICACHE_ACCESS_COUNT_LOW),
			readl(dpmu_sfr + BR2_ICACHE_MISS_COUNT));
	score_info("%24s:%8x %8x\n",
			"BRL2_ICACHE(access/miss)",
			readl(dpmu_sfr + BRL2_ICACHE_ACCESS_COUNT),
			readl(dpmu_sfr + BRL2_ICACHE_MISS_COUNT));
	score_info("%24s:%8x %8x\n",
			"BR1_DCACHE(access/miss)",
			readl(dpmu_sfr + BR1_DCACHE_ACCESS_COUNT),
			readl(dpmu_sfr + BR1_DCACHE_MISS_COUNT));
	score_info("%24s:%8x %8x\n",
			"BR2_DCACHE(access/miss)",
			readl(dpmu_sfr + BR2_DCACHE_ACCESS_COUNT),
			readl(dpmu_sfr + BR2_DCACHE_MISS_COUNT));
	score_info("%24s:%8x %8x %8x %8x\n",
			"TS_STALL(core/pm/dm/sp)",
			readl(dpmu_sfr + TS_CORE_STALL_COUNT),
			readl(dpmu_sfr + TS_PM_STALL_COUNT),
			readl(dpmu_sfr + TS_DM_STALL_COUNT),
			readl(dpmu_sfr + TS_SP_STALL_COUNT));
	score_info("%24s:%8x %8x %8x %8x\n",
			"BR1_STALL(core/pm/dm/sp)",
			readl(dpmu_sfr + BR1_CORE_STALL_COUNT),
			readl(dpmu_sfr + BR1_PM_STALL_COUNT),
			readl(dpmu_sfr + BR1_DM_STALL_COUNT),
			readl(dpmu_sfr + BR1_SP_STALL_COUNT));
	score_info("%24s:%8x %8x %8x %8x\n",
			"BR2_STALL(core/pm/dm/sp)",
			readl(dpmu_sfr + BR2_CORE_STALL_COUNT),
			readl(dpmu_sfr + BR2_PM_STALL_COUNT),
			readl(dpmu_sfr + BR2_DM_STALL_COUNT),
			readl(dpmu_sfr + BR2_SP_STALL_COUNT));
	score_info("%24s:%8x %8x %8x\n",
			"SFR_STALL(ts/b1/b2)",
			readl(dpmu_sfr + TS_SFR_STALL_COUNT),
			readl(dpmu_sfr + BR1_SFR_STALL_COUNT),
			readl(dpmu_sfr + BR2_SFR_STALL_COUNT));
	score_info("%24s:%8x %8x\n",
			"BR1_TCM(stall/access)",
			readl(dpmu_sfr + BR1_TCM_STALL_COUNT),
			readl(dpmu_sfr + BR1_TCM_ACCESS_COUNT));
	score_info("%24s:%8x %8x\n",
			"BR2_TCM(stall/access)",
			readl(dpmu_sfr + BR2_TCM_STALL_COUNT),
			readl(dpmu_sfr + BR2_TCM_ACCESS_COUNT));
	score_info("%24s:%8x %8x\n",
			"DMARD_TCM(stall/access)",
			readl(dpmu_sfr + DMARD_TCM_STALL_COUNT),
			readl(dpmu_sfr + DMARD_TCM_ACCESS_COUNT));
	score_info("%24s:%8x %8x\n",
			"DMAWR_TCM(stall/access)",
			readl(dpmu_sfr + DMAWR_TCM_STALL_COUNT),
			readl(dpmu_sfr + DMAWR_TCM_ACCESS_COUNT));
	score_info("%24s:%8x %8x\n",
			"IVA_TCM(stall/access)",
			readl(dpmu_sfr + IVA_TCM_STALL_COUNT),
			readl(dpmu_sfr + IVA_TCM_ACCESS_COUNT));
	score_info("%24s:%8x\n",
			"DMA_EXEC_INDEX_INFO",
			readl(dpmu_sfr + DMA_EXEC_INDEX_INFO));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA0_EXEC_COUNT(00-03)",
			readl(dpmu_sfr + DMA0_EXEC_COUNT0),
			readl(dpmu_sfr + DMA0_EXEC_COUNT1),
			readl(dpmu_sfr + DMA0_EXEC_COUNT2),
			readl(dpmu_sfr + DMA0_EXEC_COUNT3));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA0_EXEC_COUNT(04-07)",
			readl(dpmu_sfr + DMA0_EXEC_COUNT4),
			readl(dpmu_sfr + DMA0_EXEC_COUNT5),
			readl(dpmu_sfr + DMA0_EXEC_COUNT6),
			readl(dpmu_sfr + DMA0_EXEC_COUNT7));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA0_EXEC_COUNT(08-11)",
			readl(dpmu_sfr + DMA0_EXEC_COUNT8),
			readl(dpmu_sfr + DMA0_EXEC_COUNT9),
			readl(dpmu_sfr + DMA0_EXEC_COUNT10),
			readl(dpmu_sfr + DMA0_EXEC_COUNT11));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA0_EXEC_COUNT(12-15)",
			readl(dpmu_sfr + DMA0_EXEC_COUNT12),
			readl(dpmu_sfr + DMA0_EXEC_COUNT13),
			readl(dpmu_sfr + DMA0_EXEC_COUNT14),
			readl(dpmu_sfr + DMA0_EXEC_COUNT15));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA1_EXEC_COUNT(00-03)",
			readl(dpmu_sfr + DMA1_EXEC_COUNT0),
			readl(dpmu_sfr + DMA1_EXEC_COUNT1),
			readl(dpmu_sfr + DMA1_EXEC_COUNT2),
			readl(dpmu_sfr + DMA1_EXEC_COUNT3));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA1_EXEC_COUNT(04-07)",
			readl(dpmu_sfr + DMA1_EXEC_COUNT4),
			readl(dpmu_sfr + DMA1_EXEC_COUNT5),
			readl(dpmu_sfr + DMA1_EXEC_COUNT6),
			readl(dpmu_sfr + DMA1_EXEC_COUNT7));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA1_EXEC_COUNT(08-11)",
			readl(dpmu_sfr + DMA1_EXEC_COUNT8),
			readl(dpmu_sfr + DMA1_EXEC_COUNT9),
			readl(dpmu_sfr + DMA1_EXEC_COUNT10),
			readl(dpmu_sfr + DMA1_EXEC_COUNT11));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA1_EXEC_COUNT(12-15)",
			readl(dpmu_sfr + DMA1_EXEC_COUNT12),
			readl(dpmu_sfr + DMA1_EXEC_COUNT13),
			readl(dpmu_sfr + DMA1_EXEC_COUNT14),
			readl(dpmu_sfr + DMA1_EXEC_COUNT15));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA2_EXEC_COUNT(00-03)",
			readl(dpmu_sfr + DMA2_EXEC_COUNT0),
			readl(dpmu_sfr + DMA2_EXEC_COUNT1),
			readl(dpmu_sfr + DMA2_EXEC_COUNT2),
			readl(dpmu_sfr + DMA2_EXEC_COUNT3));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA2_EXEC_COUNT(04-07)",
			readl(dpmu_sfr + DMA2_EXEC_COUNT4),
			readl(dpmu_sfr + DMA2_EXEC_COUNT5),
			readl(dpmu_sfr + DMA2_EXEC_COUNT6),
			readl(dpmu_sfr + DMA2_EXEC_COUNT7));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA2_EXEC_COUNT(08-11)",
			readl(dpmu_sfr + DMA2_EXEC_COUNT8),
			readl(dpmu_sfr + DMA2_EXEC_COUNT9),
			readl(dpmu_sfr + DMA2_EXEC_COUNT10),
			readl(dpmu_sfr + DMA2_EXEC_COUNT11));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA2_EXEC_COUNT(12-15)",
			readl(dpmu_sfr + DMA2_EXEC_COUNT12),
			readl(dpmu_sfr + DMA2_EXEC_COUNT13),
			readl(dpmu_sfr + DMA2_EXEC_COUNT14),
			readl(dpmu_sfr + DMA2_EXEC_COUNT15));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA3_EXEC_COUNT(00-03)",
			readl(dpmu_sfr + DMA3_EXEC_COUNT0),
			readl(dpmu_sfr + DMA3_EXEC_COUNT1),
			readl(dpmu_sfr + DMA3_EXEC_COUNT2),
			readl(dpmu_sfr + DMA3_EXEC_COUNT3));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA3_EXEC_COUNT(04-07)",
			readl(dpmu_sfr + DMA3_EXEC_COUNT4),
			readl(dpmu_sfr + DMA3_EXEC_COUNT5),
			readl(dpmu_sfr + DMA3_EXEC_COUNT6),
			readl(dpmu_sfr + DMA3_EXEC_COUNT7));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA3_EXEC_COUNT(08-11)",
			readl(dpmu_sfr + DMA3_EXEC_COUNT8),
			readl(dpmu_sfr + DMA3_EXEC_COUNT9),
			readl(dpmu_sfr + DMA3_EXEC_COUNT10),
			readl(dpmu_sfr + DMA3_EXEC_COUNT11));
	score_info("%24s:%8x %8x %8x %8x\n",
			"DMA3_EXEC_COUNT(12-15)",
			readl(dpmu_sfr + DMA3_EXEC_COUNT12),
			readl(dpmu_sfr + DMA3_EXEC_COUNT13),
			readl(dpmu_sfr + DMA3_EXEC_COUNT14),
			readl(dpmu_sfr + DMA3_EXEC_COUNT15));
}

void score_dpmu_print_all(void)
{
	score_dpmu_print_system();
	score_dpmu_print_debug();
	score_dpmu_print_target();
	score_dpmu_print_userdefined();
	score_dpmu_print_performance();
}

void score_dpmu_debug_control(int on)
{
	score_check();
	debug_on = on;
}

int score_dpmu_debug_check(void)
{
	score_check();
	return debug_on;
}

void score_dpmu_performance_control(int on)
{
	score_check();
	performance_on = on;
}

int score_dpmu_performance_check(void)
{
	score_check();
	return performance_on;
}

void score_dpmu_enable(void)
{
	score_enter();
	if (debug_on) {
		writel(0x1, dpmu_sfr + DSPM_DBG_MONITOR_ENABLE);
		writel(0x1, dpmu_sfr + DSPS_DBG_MONITOR_ENABLE);
	}
	if (performance_on) {
		writel(0xffff, dpmu_sfr + DSPM_PERF_MONITOR_ENABLE);
		writel(0xffff, dpmu_sfr + DSPS_PERF_MONITOR_ENABLE);
	}
	score_leave();
}

void score_dpmu_disable(void)
{
	score_enter();
	if (performance_on) {
		writel(0x0, dpmu_sfr + DSPS_PERF_MONITOR_ENABLE);
		writel(0x0, dpmu_sfr + DSPM_PERF_MONITOR_ENABLE);
	}
	if (debug_on) {
		writel(0x0, dpmu_sfr + DSPS_DBG_MONITOR_ENABLE);
		writel(0x0, dpmu_sfr + DSPM_DBG_MONITOR_ENABLE);
	}
	score_leave();
}

void score_dpmu_init(void __iomem *sfr)
{
	dpmu_sfr = sfr;
}
