/*
 * Samsung Exynos SoC series SCore driver
 *
 * Definition of SCore SFR registers
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_REGS_H__
#define __SCORE_REGS_H__

#define SW_RESET				(0x0030)
#define RELEASE_DATE				(0x0074)
#define AXIM0_ARCACHE				(0x0080)
#define AXIM0_AWCACHE				(0x0084)
#define AXIM2_ARCACHE				(0x0088)
#define AXIM2_AWCACHE				(0x008c)

#define MC_WAKEUP				(0x1000)
#define KC1_WAKEUP				(0x1004)
#define KC2_WAKEUP				(0x1008)
#define SM_MCGEN				(0x100c)
#define CLK_REQ					(0x101c)

#define APCPU_INTR_ENABLE			(0x2000)
#define MC_INTR_ENABLE				(0x2004)
#define KC1_INTR_ENABLE				(0x2008)
#define KC2_INTR_ENABLE				(0x200c)
#define IVA_INTR_ENABLE				(0x2010)
#define APCPU_SWI_STATUS			(0x2014)
#define APCPU_SWI_CLEAR				(0x2018)
#define MC_SWI_STATUS				(0x201c)
#define KC1_SWI_STATUS				(0x2024)
#define KC2_SWI_STATUS				(0x202c)
#define APCPU_RAW_INTR_STATUS			(0x2034)
#define APCPU_MASKED_INTR_STATUS		(0x2038)

#define MC_CACHE_STATUS				(0x3004)
#define MC_CACHE_IC_BASEADDR			(0x3040)
#define MC_CACHE_DC_BASEADDR			(0x30c0)
#define MC_CACHE_DC_CCTRL_REQ			(0x30d0)
#define MC_CACHE_DC_CCTRL_STATUS		(0x30d4)
#define MC_CACHE_DC_RCTRL_REQ			(0x30e0)
#define MC_CACHE_DC_RCTRL_STATUS		(0x30e4)
#define MC_CACHE_DC_RCTRL_ADDR			(0x30e8)
#define MC_CACHE_DC_RCTRL_SIZE			(0x30ec)

#define KC1_CACHE_STATUS			(0x4004)
#define KC1_CACHE_IC_BASEADDR			(0x4040)
#define KC1_CACHE_DC_BASEADDR			(0x40c0)
#define KC1_CACHE_DC_CCTRL_REQ			(0x40d0)
#define KC1_CACHE_DC_CCTRL_STATUS		(0x40d4)
#define KC1_CACHE_DC_RCTRL_REQ			(0x40e0)
#define KC1_CACHE_DC_RCTRL_STATUS		(0x40e4)
#define KC1_CACHE_DC_RCTRL_ADDR			(0x40e8)
#define KC1_CACHE_DC_RCTRL_SIZE			(0x40ec)

#define KC2_CACHE_STATUS			(0x5004)
#define KC2_CACHE_IC_BASEADDR			(0x5040)
#define KC2_CACHE_DC_BASEADDR			(0x50c0)
#define KC2_CACHE_DC_CCTRL_REQ			(0x50d0)
#define KC2_CACHE_DC_CCTRL_STATUS		(0x50d4)
#define KC2_CACHE_DC_RCTRL_REQ			(0x50e0)
#define KC2_CACHE_DC_RCTRL_STATUS		(0x50e4)
#define KC2_CACHE_DC_RCTRL_ADDR			(0x50e8)
#define KC2_CACHE_DC_RCTRL_SIZE			(0x50ec)

#ifdef SCORE_EVT1
#define MC_STALL_COUNT_ENABLE			(0x3120)
#define MC_PROC_STALL_COUNT			(0x3124)
#define MC_IC_STALL_COUNT			(0x3128)
#define MC_DC_STALL_COUNT			(0x312c)
#define MC_TCM_STALL_COUNT			(0x3130)
#define MC_SFR_STALL_COUNT			(0x3134)

#define KC1_SFR_STALL_COUNT_ENABLE		(0x4118)
#define KC1_STALL_COUNT_ENABLE			(0x4120)
#define KC1_PROC_STALL_COUNT			(0x4124)
#define KC1_IC_STALL_COUNT			(0x4128)
#define KC1_DC_STALL_COUNT			(0x412c)
#define KC1_TCM_STALL_COUNT			(0x4130)
#define KC1_SFR_STALL_COUNT			(0x4134)

#define KC2_SFR_STALL_COUNT_ENABLE		(0x5118)
#define KC2_STALL_COUNT_ENABLE			(0x5120)
#define KC2_PROC_STALL_COUNT			(0x5124)
#define KC2_IC_STALL_COUNT			(0x5128)
#define KC2_DC_STALL_COUNT			(0x512c)
#define KC2_TCM_STALL_COUNT			(0x5130)
#define KC2_SFR_STALL_COUNT			(0x5134)
#endif

/* SCQ */
#if defined(SCORE_EVT0)
#define SCQ_MINFO				(0x6000)
#define SCQ_MSET				(0x6004)
#define SCQ_SINFO				(0x6008)
#define SCQ_STAKEN				(0x600c)
#define SCQ_SRAM				(0x6010)
#define SCQ_RAW_INTR_STATUS			(0x6014)
#define SCQ_INTR_ENABLE				(0x6018)
#define SCQ_INTR_CLEAR				(0x601c)
#define SCQ_MASKED_INTR_STATUS			(0x6020)
#else /* SCORE_EVT1 */
#define SCQ_MINFO				(0x6000)
#define SCQ_MSET				(0x6004)
#define SCQ_SINFO				(0x6008)
#define SCQ_STAKEN				(0x600c)
#define SCQ_SRAM				(0x6010)
#define SCQ_SRAM1				(0x6014)
#define SCQ_RAW_INTR_STATUS			(0x6018)
#define SCQ_INTR_ENABLE				(0x601c)
#define SCQ_INTR_CLEAR				(0x6020)
#define SCQ_MASKED_INTR_STATUS			(0x6024)
#define SCQ_APCPU_PACKET_NUM			(0x602c)
#define SCQ_MC_PACKET_NUM			(0x6030)
#define SCQ_KC1_PACKET_NUM			(0x6034)
#define SCQ_KC2_PACKET_NUM			(0x6038)
#define SCQ_IVA_PACKET_NUM			(0x603c)
#endif

/* DPMU SFR (Debug and Performance Monitoring Unit */
#define CPUID_OF_CCH				(0x0034)
#define DBG_RAW_INTR_STATUS			(0xc000)
#define DBG_INTR_ENABLE				(0xc004)
#define DBG_INTR_CLEAR				(0xc008)
#define DBG_MASKED_INTR_STATUS			(0xc00c)
#define DBG_MONITOR_ENABLE			(0xc010)
#define PERF_MONITOR_ENABLE			(0xc014)
#define PERF_MONITOR_CLEAR			(0xc018)
#define PERF_MONITOR_DMA_INIT			(0xc01c)
#define MC_ICACHE_ACC_COUNT_LOW			(0xc020)
#define MC_ICACHE_ACC_COUNT_HIGH		(0xc024)
#define MC_ICACHE_MISS_COUNT			(0xc028)
#define MC_DCACHE_ACC_COUNT			(0xc030)
#define MC_DCACHE_MISS_COUNT			(0xc034)
#define KC1_ICACHE_ACC_COUNT_LOW		(0xc040)
#define KC1_ICACHE_ACC_COUNT_HIGH		(0xc044)
#define KC1_ICACHE_MISS_COUNT			(0xc048)
#define KC1_DCACHE_ACC_COUNT			(0xc050)
#define KC1_DCACHE_MISS_COUNT			(0xc054)
#define KC2_ICACHE_ACC_COUNT_LOW		(0xc060)
#define KC2_ICACHE_ACC_COUNT_HIGH		(0xc064)
#define KC2_ICACHE_MISS_COUNT			(0xc068)
#define KC2_DCACHE_ACC_COUNT			(0xc070)
#define KC2_DCACHE_MISS_COUNT			(0xc074)
#define AXIM0_RDBEAT_COUNT			(0xc080)
#define AXIM0_WRBEAT_COUNT			(0xc084)
#define AXIM0_RD_MAX_MO_COUNT			(0xc088)
#define AXIM0_WR_MAX_MO_COUNT			(0xc08c)
#define AXIM0_RD_MAX_REQ_LAT			(0xc090)
#define AXIM0_WR_MAX_REQ_LAT			(0xc094)
#define AXIM0_RD_MAX_AR2R_LAT			(0xc098)
#define AXIM0_WR_MAX_R2R_LAT			(0xc09c)
#define AXIM1_RDBEAT_COUNT			(0xc0a0)
#define AXIM1_WRBEAT_COUNT			(0xc0a4)
#define AXIM1_RD_MAX_MO_COUNT			(0xc0a8)
#define AXIM1_WR_MAX_MO_COUNT			(0xc0ac)
#define AXIM1_RD_MAX_REQ_LAT			(0xc0b0)
#define AXIM1_WR_MAX_REQ_LAT			(0xc0b4)
#define AXIM1_RD_MAX_AR2R_LAT			(0xc0b8)
#define AXIM1_WR_MAX_R2R_LAT			(0xc0bc)
#define AXIM2_RDBEAT_COUNT			(0xc0c0)
#define AXIM2_WRBEAT_COUNT			(0xc0c4)
#define AXIM2_RD_MAX_MO_COUNT			(0xc0c8)
#define AXIM2_WR_MAX_MO_COUNT			(0xc0cc)
#define AXIM2_RD_MAX_REQ_LAT			(0xc0d0)
#define AXIM2_WR_MAX_REQ_LAT			(0xc0d4)
#define AXIM2_RD_MAX_AR2R_LAT			(0xc0d8)
#define AXIM2_WR_MAX_R2R_LAT			(0xc0dc)
#define AXIM3_RDBEAT_COUNT			(0xc0e0)
#define AXIM3_WRBEAT_COUNT			(0xc0e4)
#define AXIM3_RD_MAX_MO_COUNT			(0xc0e8)
#define AXIM3_WR_MAX_MO_COUNT			(0xc0ec)
#define AXIM3_RD_MAX_REQ_LAT			(0xc0f0)
#define AXIM3_WR_MAX_REQ_LAT			(0xc0f4)
#define AXIM3_RD_MAX_AR2R_LAT			(0xc0f8)
#define AXIM3_WR_MAX_R2R_LAT			(0xc0fc)
#define AXIS2_RDBEAT_COUNT			(0xc100)
#define AXIS2_WRBEAT_COUNT			(0xc104)
#define AXIS2_RD_MAX_MO_COUNT			(0xc108)
#define AXIS2_WR_MAX_MO_COUNT			(0xc10c)
#define AXIS2_RD_MAX_REQ_LAT			(0xc110)
#define AXIS2_WR_MAX_REQ_LAT			(0xc114)
#define AXIS2_RD_MAX_AR2R_LAT			(0xc118)
#define AXIS2_WR_MAX_R2R_LAT			(0xc11c)
#define DMA0_EXEC_CYCLE0			(0xc120)
#define DMA0_EXEC_CYCLE1			(0xc124)
#define DMA0_EXEC_CYCLE2			(0xc128)
#define DMA0_EXEC_CYCLE3			(0xc12c)
#define DMA0_EXEC_CYCLE4			(0xc130)
#define DMA0_EXEC_CYCLE5			(0xc134)
#define DMA0_EXEC_CYCLE6			(0xc138)
#define DMA0_EXEC_CYCLE7			(0xc13c)
#define DMA0_EXEC_CYCLE8			(0xc140)
#define DMA0_EXEC_CYCLE9			(0xc144)
#define DMA0_EXEC_CYCLE10			(0xc148)
#define DMA0_EXEC_CYCLE11			(0xc14c)
#define DMA0_EXEC_CYCLE12			(0xc150)
#define DMA0_EXEC_CYCLE13			(0xc154)
#define DMA0_EXEC_CYCLE14			(0xc158)
#define DMA0_EXEC_CYCLE15			(0xc15c)
#define DMA1_EXEC_CYCLE0			(0xc160)
#define DMA1_EXEC_CYCLE1			(0xc164)
#define DMA1_EXEC_CYCLE2			(0xc168)
#define DMA1_EXEC_CYCLE3			(0xc16c)
#define DMA1_EXEC_CYCLE4			(0xc170)
#define DMA1_EXEC_CYCLE5			(0xc174)
#define DMA1_EXEC_CYCLE6			(0xc178)
#define DMA1_EXEC_CYCLE7			(0xc17c)
#define DMA1_EXEC_CYCLE8			(0xc180)
#define DMA1_EXEC_CYCLE9			(0xc184)
#define DMA1_EXEC_CYCLE10			(0xc188)
#define DMA1_EXEC_CYCLE11			(0xc18c)
#define DMA1_EXEC_CYCLE12			(0xc190)
#define DMA1_EXEC_CYCLE13			(0xc194)
#define DMA1_EXEC_CYCLE14			(0xc198)
#define DMA1_EXEC_CYCLE15			(0xc19c)
#define MC_INST_COUNT_LOW			(0xc1a0)
#define MC_INST_COUNT_HIGH			(0xc1a4)
#define KC1_INST_COUNT_LOW			(0xc1a8)
#define KC1_INST_COUNT_HIGH			(0xc1ac)
#define KC2_INST_COUNT_LOW			(0xc1b0)
#define KC2_INST_COUNT_HIGH			(0xc1b4)
#define MC_INTR_COUNT_LOW			(0xc1b8)
#define MC_INTR_COUNT_HIGH			(0xc1bc)
#define KC1_INTR_COUNT_LOW			(0xc1c0)
#define KC1_INTR_COUNT_HIGH			(0xc1c4)
#define KC2_INTR_COUNT_LOW			(0xc1c8)
#define KC2_INTR_COUNT_HIGH			(0xc1cc)
#define SCQ_SLVERR_INFO				(0xc1d0)
#define TCMBUS_SLVERR_INFO			(0xc1d4)
#define CACHE_SLVERR_INFO			(0xc1d8)
#define IBC_SLVERR_INFO				(0xc1dc)
#define AXI_SLVERR_INFO				(0xc1e0)
#define DSP_SLVERR_INFO				(0xc1e4)
#define MC_DECERR_INFO				(0xc1f0)
#define KC1_DECERR_INFO				(0xc1f4)
#define KC2_DECERR_INFO				(0xc1f8)
#define MC_PROG_COUNTER				(0xc200)
#define KC1_PROG_COUNTER			(0xc204)
#define KC2_PROG_COUNTER			(0xc208)

/**
 * 0x8c00 ~ 0x8ffc : Reserved SFR of shared memory
 * This is used from the top (RESERVED 0 - 255)
 * RESERVED 0 - 31 : USERDEFINED(N)
 * RESERVED 32 : front of queue for ScPrintf
 * RESERVED 33 : rear of queue for ScPrintf
 * RESERVED 34 : counter for ScPrintf
 * RESERVED 35 : flag of MC_INIT_DONE_CHECK
 * RESERVED 36 : flag of KC1_INIT_DONE_CHECK
 * RESERVED 37 : flag of KC2_INIT_DONE_CHECK
 * RESERVED 38 : base address of malloc at master
 * RESERVED 39 : size of malloc at master
 * RESERVED 40 : base address of malloc at knight1
 * RESERVED 41 : base address of malloc at knight2
 * RESERVED 42 : size of malloc at knight
 * RESERVED 43 : base address of print
 * RESERVED 44 : size of print
 * RESERVED 45 : MC size of profiler
 * RESERVED 46 : MC base address of profiler
 * RESERVED 47 : MC profiler ring buffer head
 * RESERVED 48 : MC profiler ring buffer tail
 * RESERVED 49 : KC1 and KC2 size of profiler
 * RESERVED 50 : KC1 base address of profiler
 * RESERVED 51 : KC1 profiler ring buffer head
 * RESERVED 52 : KC1 profiler ring buffer tail
 * RESERVED 53 : KC2 base address of profiler
 * RESERVED 54 : KC2 profiler ring buffer head
 * RESERVED 55 : KC2 profiler ring buffer tail
 * RESERVED 56 : profiler tag select filter
 */
#define SM_RESERVED_BASE		(0x8ffc)
#define SM_RESERVED(n)			(SM_RESERVED_BASE - 0x4 * (n))

#define USERDEFINED(n)			(SM_RESERVED(n))
#define PRINT_QUEUE_FRONT_ADDR		(SM_RESERVED(32))
#define PRINT_QUEUE_REAR_ADDR		(SM_RESERVED(33))
#define PRINT_COUNT			(SM_RESERVED(34))
#define MC_INIT_DONE_CHECK		(SM_RESERVED(35))
#define KC1_INIT_DONE_CHECK		(SM_RESERVED(36))
#define KC2_INIT_DONE_CHECK		(SM_RESERVED(37))
#define MC_MALLOC_BASE_ADDR		(SM_RESERVED(38))
#define MC_MALLOC_SIZE			(SM_RESERVED(39))
#define KC1_MALLOC_BASE_ADDR		(SM_RESERVED(40))
#define KC2_MALLOC_BASE_ADDR		(SM_RESERVED(41))
#define KC_MALLOC_SIZE			(SM_RESERVED(42))
#define PRINT_BASE_ADDR			(SM_RESERVED(43))
#define PRINT_SIZE			(SM_RESERVED(44))
#define PROFILER_MC_SIZE		(SM_RESERVED(45))
#define PROFILER_MC_BASE_ADDR		(SM_RESERVED(46))
#define PROFILER_MC_HEAD		(SM_RESERVED(47))
#define PROFILER_MC_TAIL		(SM_RESERVED(48))
#define PROFILER_KC_SIZE		(SM_RESERVED(49))
#define PROFILER_KC1_BASE_ADDR		(SM_RESERVED(50))
#define PROFILER_KC1_HEAD		(SM_RESERVED(51))
#define PROFILER_KC1_TAIL		(SM_RESERVED(52))
#define PROFILER_KC2_BASE_ADDR		(SM_RESERVED(53))
#define PROFILER_KC2_HEAD		(SM_RESERVED(54))
#define PROFILER_KC2_TAIL		(SM_RESERVED(55))
#define PROFILER_TAG_MASK		(SM_RESERVED(56))
#define KC1_PROLOGUE_DONE		(SM_RESERVED(57))
#define KC2_PROLOGUE_DONE		(SM_RESERVED(58))
#define TOTAL_TILE_COUNT		(SM_RESERVED(59))
#define TOTAL_TCM_SIZE			(SM_RESERVED(60))
#define ENABLE_TCM_HALF			(SM_RESERVED(61))

/* target debug */
#define MC_ABORT_STATE0			(SM_RESERVED(62))
#define MC_ABORT_STATE1			(SM_RESERVED(63))
#define MC_ABORT_STATE2			(SM_RESERVED(64))
#define MC_ABORT_STATE3			(SM_RESERVED(65))
#define MC_ABORT_STATE4			(SM_RESERVED(66))
#define SCHEDULE_STATE			(SM_RESERVED(67))
#define CORE_STATE			(SM_RESERVED(68))
#define PRIORITY_STATE			(SM_RESERVED(69))
#define MC_KERNEL_ID			(SM_RESERVED(70))
#define MC_ENTRY_FLAG			(SM_RESERVED(71))
#define INDIRECT_FUNC			(SM_RESERVED(72))
#define INDIRECT_ADDR			(SM_RESERVED(73))
#define MC_REQUEST_TYPE			(SM_RESERVED(74))
#define MC_TCM_MALLOC_STATE		(SM_RESERVED(75))
#define KC1_CURRENT_KERNEL		(SM_RESERVED(76))
#define KC2_CURRENT_KERNEL		(SM_RESERVED(77))

#endif
