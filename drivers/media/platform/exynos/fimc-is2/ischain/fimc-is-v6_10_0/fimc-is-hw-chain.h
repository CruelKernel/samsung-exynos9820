/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_CHAIN_V6_10_H
#define FIMC_IS_HW_CHAIN_V6_10_H

#include "fimc-is-hw-api-common.h"
#include "fimc-is-groupmgr.h"
#include "fimc-is-config.h"

#define GROUP_HW_MAX	(GROUP_SLOT_MAX)

#define IORESOURCE_CSIS_DMA	0
#define IORESOURCE_3AA0		1
#define IORESOURCE_3AA1		2
#define IORESOURCE_ISP		3
#define IORESOURCE_MCSC		4
#define IORESOURCE_VRA_CH0	5
#define IORESOURCE_VRA_CH1	6
#define IORESOURCE_PAF_CORE	7
#define IORESOURCE_PAF_RDMA	8

#define FIMC_IS_RESERVE_LIB_SIZE	(0x01100000)	/* 17MB */
#define FIMC_IS_TAAISP_SIZE		(0x00500000)	/* 5MB */
#define TAAISP_MEDRC_SIZE		(0x00000000)	/* zero */
#define FIMC_IS_VRA_SIZE		(0x00800000)	/* 8MB */

#define FIMC_IS_HEAP_SIZE		(0x02800000)	/* 40MB */

#define SYSREG_CAM_BASE_ADDR		0x14510000
#define SYSREG_ISP_BASE_ADDR		0x14710000
#define SYSREG_ISP_LHM_ATB_GLUE_ADDR		0x14710900

#define HWFC_INDEX_RESET_ADDR		0x14641050

#define TZPC_CAM				(SYSREG_CAM_BASE_ADDR + 0x0200)
#define TZPC_DECPROT1STAT_OFFSET				(0x0010)
#define TZPC_DECPROT1SET_OFFSET				(0x0014)
#define TZPC_DECPROT1CLR_OFFSET				(0x0018)

#define LIC_3AA1_OFFSET_ADDR		(0x00004000)

#define PAF_CONTEXT0_OFFSET			(0x00000000)
#define PAF_CONTEXT1_OFFSET			(0x00004000)
#define PAF_RDMA0_OFFSET			(0x00000000)
#define PAF_RDMA1_OFFSET			(0x00004000)

enum taaisp_chain_id {
	ID_3AA_0 = 0,
	ID_3AA_1 = 1,
	ID_ISP_0 = 2,
	ID_ISP_1 = 3,
	ID_TPU_0 = 4,
	ID_TPU_1 = 5,
	ID_DCP	 = 6,
	ID_3AAISP_MAX
};

/* the number of interrupt source at each IP */
enum hwip_interrupt_map {
	INTR_HWIP1 = 0,
	INTR_HWIP2 = 1,
	INTR_HWIP_MAX
};

/* Specific interrupt map belonged to each IP */

/* MC-Scaler */
#define MCSC_INTR_MASK		(0x0030007C)
#define USE_DMA_BUFFER_INDEX	(0) /* 0 ~ 7 */
#define MCSC_PRECISION		(20)
#define MCSC_POLY_RATIO_UP	(10)
#define MCSC_POLY_RATIO_DOWN	(16)
#define MCSC_POST_RATIO_DOWN	(16)
/* #define MCSC_POST_WA */
#define MCSC_POST_WA_SHIFT	(8)	/* 256 = 2^8 */

#define MCSC_OUTPUT_SSB		(0xF)	/* This number has no special meaning. */

#define MAX_MCSC_DNR_WIDTH		(5376)
#define MAX_MCSC_DNR_HEIGHT		(4320)
#define MAX_MCSC_DNR_NUM_BUFFER		(2)

#define MCSC_DNR_WIDTH			(MAX_MCSC_DNR_WIDTH)
#define MCSC_DNR_HEIGHT			(MAX_MCSC_DNR_HEIGHT)
#define FIMC_IS_MCSC_DNR_SIZE		ALIGN(MCSC_DNR_WIDTH * MCSC_DNR_HEIGHT * 2 \
						* MAX_MCSC_DNR_NUM_BUFFER, 16)

#define MCSC_DNR_USE_FIRST		(false)
#define MCSC_DNR_USE_INTERNAL_BUF	(true)
#define MCSC_DNR_OUTPUT_INDEX		(0)
#define MCSC_DNR_USE_TUNING		(true)
#define MCSC_SETFILE_VERSION		(0x14027431)
#define MCSC_DJAG_IN_VIDEO_MODE		(DEV_HW_MCSC0)
#define MCSC_DJAG_IN_CAPTURE_MODE	(DEV_HW_MCSC1)
#define MCSC_CAC_IN_VIDEO_MODE		(DEV_HW_MCSC0)
#define MCSC_CAC_IN_CAPTURE_MODE	(DEV_HW_MCSC1)

enum mc_scaler_interrupt_map {
	INTR_MC_SCALER_FRAME_END		= 0,
	INTR_MC_SCALER_FRAME_START		= 1,
	INTR_MC_SCALER_WDMA_FINISH		= 2,
	INTR_MC_SCALER_CORE_FINISH		= 3,
	INTR_MC_SCALER_INPUT_HORIZONTAL_OVF	= 7,
	INTR_MC_SCALER_INPUT_HORIZONTAL_UNF	= 8,
	INTR_MC_SCALER_INPUT_VERTICAL_OVF	= 9,
	INTR_MC_SCALER_INPUT_VERTICAL_UNF	= 10,
	INTR_MC_SCALER_OVERFLOW			= 11,
	INTR_MC_SCALER_INPUT_FRAME_CURSH	= 12,
	INTR_MC_SCALER_OUTSTALL			= 13,	/* for backward compatibility : no valid bit */
	INTR_MC_SCALER_SHADOW_COPY_FINISH	= 16,
	INTR_MC_SCALER_SHADOW_COPY_FINISH_OVF	= 17,
	INTR_MC_SCALER_FM_SUB_FRAME_FINISH	= 20,
	INTR_MC_SCALER_FM_SUB_FRAME_START	= 21,
};

/* VRA */
#define VRA_CH1_INTR_CNT_PER_FRAME	(5)		/* for YUV422_2P DMA input */

enum vra_chain0_interrupt_map {
	CH0INT_CIN_FR_ST			= 0,	/* VSYNC rise - verified when CIN is active */
	CH0INT_CIN_FR_END			= 1,	/* VSYNC fall - verified when CIN is active */
	CH0INT_CIN_LINE_ST			= 2,	/* Not used - per line */
	CH0INT_CIN_LINE_END			= 3,	/* Not used - per line */
	CH0INT_CIN_SP_LINE			= 4,	/* On start of line defined by reg_cin2isp_int_row
							- Currently not used */
	CH0INT_CIN_ERR_SIZES			= 5,	/* Interrupt occurs as the error occurs
							- not used - CH0INT_FRAME_SIZE_ERROR is used instead */
	CH0INT_CIN_ERR_YUV_FORMAT		= 6,	/* For YUV444 - not all channels are valid,
							for 422 - Even & Odd data not match (e.g. YU match YV) */
	CH0INT_CIN_FR_ST_NO_ACTIVE		= 7,	/* VSYNC received when CH0 not enabled */
	CH0INT_DMA_IN_ERROR			= 8,	/* An error that is "read" by DMA controller.
							Not a control error. Should be reported to HOST. */
	CH0INT_DMA_IN_FLUSH_DONE		= 9,	/* When TRANS_STOP_REQ is set this interrupt should be set */
	CH0INT_DMA_IN_FR_END			= 10,	/* Finished operation */
	CH0INT_DMA_IN_INFO			= 11,	/* Stall / Frame Start while active / Track
							- see RW_reg_dma_info_int_vector_ofs. Not used. */
	CH0INT_OUT_DMA_ERROR			= 12,	/* An error that is "read" by DMA controller.
							Not a control error. Should be reported to HOST. */
	CH0INT_OUT_DMA_FLUSH_DONE		= 13,	/* When TRANS_STOP_REQ is set this interrupt should be set */
	CH0INT_OUT_DMA_FR_END			= 14,	/* Finished operation */
	CH0INT_OUT_DMA_INFO			= 15,	/* Stall / Frame Start while active / Track
							- see RW_reg_dma_info_int_vector_ofs. Not used. */
	CH0INT_RWS_TRIGGER			= 16,	/* Not used (In our code Trigger == Frame Start) */
	CH0INT_END_FRAME			= 17,	/* CIN + DMAs + ISP chain were finished */
	CH0INT_END_ISP_DMA_OUT			= 18,	/* ISP chain + DMA out were finished (doesn't include CIN) */
	CH0INT_END_ISP_INPUT			= 19,	/* ISP chain + CIN / Input DMAs were finished
							(doesn't include output) */
	CH0INT_FRAME_SIZE_ERROR			= 20,	/* Like CH0INT_CIN_ERR_SIZES
							but interrupt occurs when VSYNC falls */
	CH0INT_ERR_FR_ST_BEF_FR_END		= 21,	/* VSYNC received while previous frame is being processed.
							Might be a trigger for "no end isp" */
	CH0INT_ERR_FR_ST_WHILE_FLUSH		= 22,	/* Should be tested on end of TRANS_STOP */
	CH0INT_ERR_VRHR_INTERVAL_VIOLATION	= 23,	/* Violation of minimal delta Frame start to first data */
	CH0INT_ERR_HFHR_INTERVAL_VIOLATION	= 24,	/* Violation of minimal delta End line -> Next line */
	CH0INT_ERR_PYRAMID_OVERFLOW		= 25
};

enum vra_chain1_interrupt_map {
	CH1INT_IN_CONT_SP_LINE			= 0,	/* Reach RW_reg_ch1_interrupted_insruction +
							RW_reg_ch1_interrupted_line */
	CH1INT_IN_STOP_IMMED_DONE		= 1,	/* Stop immediate command is done. */
	CH1INT_IN_END_OF_CONTEXT		= 2,	/* The reason might be found in RW_reg_ch1_stop_cause */
	CH1INT_IN_START_OF_CONTEXT		= 3,	/* Not used. */
	CH1INT_END_LOAD_FEATURES		= 4,	/* Not used. */
	CH1INT_SHADOW_TRIGGER			= 5,	/* Not used. */
	CH1INT_OUT_DMA_OVERFLOW			= 6,	/* Some output results not written to memory */
	CH1INT_MAX_NUM_RESULTS			= 7,	/* RCCs found more than maximal number of results
							- some results not sent to DMA */
	CH1INT_IN_DMA_ERROR			= 8,
	CH1INT_IN_DMA_FLUSH_DONE		= 9,	/* Not used. HW responsibility to check */
	CH1INT_IN_DMA_FR_END			= 10,	/* Finished operation */
	CH1INT_IN_DMA_INFO			= 11,	/* Stall / Frame Start while active / Track
							- see RW_reg_dma_info_int_vector_ofs */
	CH1INT_RES_DMA_ERROR			= 12,
	CH1INT_RES_DMA_FLUSH_DONE		= 13,	/* Not used. HW responsibility to check */
	CH1INT_RES_DMA_FR_END			= 14,	/* Finished operation */
	CH1INT_RES_DMA_INFO			= 15,	/* Stall / Frame Start while active / Track
							- see RW_reg_dma_info_int_vector_ofs */
	CH1INT_WATCHDOG				= 16	/* Watchdog timer expired => RCC is assumed to be stack */
};
#endif
