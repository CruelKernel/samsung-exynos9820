/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_CHAIN_V6_0_H
#define FIMC_IS_HW_CHAIN_V6_0_H

#include "fimc-is-hw-api-common.h"
#include "fimc-is-groupmgr.h"
#include "fimc-is-config.h"

enum sysreg_isppre_reg_name {
	SYSREG_ISPPRE_R_USER_CON,
	SYSREG_ISPPRE_R_SC_CON0,
	SYSREG_ISPPRE_R_SC_CON1,
	SYSREG_ISPPRE_R_SC_CON2,
	SYSREG_ISPPRE_R_SC_CON4,
	SYSREG_ISPPRE_R_SC_CON6,
	SYSREG_ISPPRE_R_SC_CON8,
	SYSREG_ISPPRE_R_SC_PDP_IN_EN,
	SYSREG_ISPPRE_R_MIPI_PHY_CON,
	SYSREG_ISPPRE_R_MIPI_PHY_MUX,
	SYSREG_ISPPRE_REG_CNT
};

enum sysreg_isplp_reg_name {
	SYSREG_ISPLP_R_USER_CON,
	SYSREG_ISPLP_REG_CNT
};

enum sysreg_isphq_reg_name {
	SYSREG_ISPHQ_R_USER_CON,
	SYSREG_ISPHQ_REG_CNT
};

enum sysreg_dcf_reg_name {
	SYSREG_DCF_R_DCF_CON,
	SYSREG_DCF_REG_CNT
};

enum qe_3aaw_reg_name {
	QE_3AAW_R_QOS_CON,
	QE_3AAW_R_READ_CH_CON,
	QE_3AAW_R_WRITE_CH_CON,
	QE_3AAW_REG_CNT
};

enum qe_isplp_reg_name {
	QE_ISPLP_R_QOS_CON,
	QE_ISPLP_R_READ_CH_CON,
	QE_ISPLP_R_WRITE_CH_CON,
	QE_ISPLP_REG_CNT
};

enum qe_3aa_reg_name {
	QE_3AA_R_QOS_CON,
	QE_3AA_R_READ_CH_CON,
	QE_3AA_R_WRITE_CH_CON,
	QE_3AA_REG_CNT
};

enum qe_isphq_reg_name {
	QE_ISPHQ_R_QOS_CON,
	QE_ISPHQ_R_READ_CH_CON,
	QE_ISPHQ_R_WRITE_CH_CON,
	QE_ISPHQ_REG_CNT
};

enum sysreg_isppre_reg_field {
	SYSREG_ISPPRE_F_GLUEMUX_ISPHQ_VAL,
	SYSREG_ISPPRE_F_GLUEMUX_ISPLP_VAL,
	SYSREG_ISPPRE_F_AWCACHE_PORT_ISPPRE,
	SYSREG_ISPPRE_F_ARCACHE_PORT_ISPPRE,
	SYSREG_ISPPRE_F_GLUEMUX_3AA0_VAL,
	SYSREG_ISPPRE_F_GLUEMUX_3AA1_VAL,
	SYSREG_ISPPRE_F_GLUEMUX_PDP_DMA0_OTF_SEL,
	SYSREG_ISPPRE_F_GLUEMUX_PDP_DMA1_OTF_SEL,
	SYSREG_ISPPRE_F_GLUEMUX_PDP_DMA2_OTF_SEL,
	SYSREG_ISPPRE_F_GLUEMUX_PDP_DMA3_OTF_SEL,
	SYSREG_ISPPRE_F_PDP_CORE1_IN_CSIS3_EN,
	SYSREG_ISPPRE_F_PDP_CORE1_IN_CSIS2_EN,
	SYSREG_ISPPRE_F_PDP_CORE1_IN_CSIS1_EN,
	SYSREG_ISPPRE_F_PDP_CORE1_IN_CSIS0_EN,
	SYSREG_ISPPRE_F_PDP_CORE0_IN_CSIS3_EN,
	SYSREG_ISPPRE_F_PDP_CORE0_IN_CSIS2_EN,
	SYSREG_ISPPRE_F_PDP_CORE0_IN_CSIS1_EN,
	SYSREG_ISPPRE_F_PDP_CORE0_IN_CSIS0_EN,
	SYSREG_ISPPRE_F_MIPI_RESETN_M0S4S4S2_S1,
	SYSREG_ISPPRE_F_MIPI_RESETN_M0S4S2_S1,
	SYSREG_ISPPRE_F_MIPI_RESETN_M0S4S2_S,
	SYSREG_ISPPRE_F_MIPI_RESETN_M4S4_MODULE,
	SYSREG_ISPPRE_F_MIPI_RESETN_M4S4_TOP,
	SYSREG_ISPPRE_F_CSIS2_DCPHY_S1_MUX_SEL,
	SYSREG_ISPPRE_F_CSIS1_DPHY_S_MUX_SEL,
	SYSREG_ISPPRE_F_CSIS0_DCPHY_S_MUX_SEL,
	SYSREG_ISPPRE_REG_FIELD_CNT
};

enum sysreg_isplp_reg_field {
	SYSREG_ISPLP_F_C2COM_SW_RESET,
	SYSREG_ISPLP_F_GLUEMUX_MCSC_SC1_VAL,
	SYSREG_ISPLP_F_GLUEMUX_MCSC_SC0_VAL,
	SYSREG_ISPLP_F_AWCACHE_ISPLP1,
	SYSREG_ISPLP_F_ARCACHE_ISPLP1,
	SYSREG_ISPLP_F_AWCACHE_ISPLP0,
	SYSREG_ISPLP_F_ARCACHE_ISPLP0,
	SYSREG_ISPLP_REG_FIELD_CNT
};

enum sysreg_isphq_reg_field {
	SYSREG_ISPHQ_F_ISPHQ_SW_RESETn_C2COM,
	SYSREG_ISPHQ_F_AWCACHE_ISPHQ,
	SYSREG_ISPHQ_F_ARCACHE_ISPHQ,
	SYSREG_ISPHQ_REG_FIELD_CNT
};

enum sysreg_dcf_reg_field {
	SYSREG_DCF_F_CIP_SEL,
	SYSREG_DCF_REG_FIELD_CNT
};

enum c2sync_type {
	OOF = 0,
	HBZ = 1,
};

#define GROUP_HW_MAX	(GROUP_SLOT_MAX)

#define IORESOURCE_CSIS_DMA	0
#define IORESOURCE_3AAW		1
#define IORESOURCE_3AA		2
#define IORESOURCE_ISPLP	3
#define IORESOURCE_ISPHQ	4
#define IORESOURCE_MCSC		5
#define IORESOURCE_VRA_CH0	6
#define IORESOURCE_VRA_CH1	7
#define IORESOURCE_DCP		8
#define IORESOURCE_STAT_DMA	9

#define FIMC_IS_RESERVE_LIB_SIZE	(0x00600000)	/* 6MB */
#define FIMC_IS_TAAISP_SIZE		(0x00500000)	/* 5MB */
#define TAAISP_MEDRC_SIZE		(0x00000000)	/* zero */
#define FIMC_IS_VRA_SIZE		(0x00800000)	/* 8MB */

#define FIMC_IS_HEAP_SIZE		(0x02800000)	/* 40MB */

#define SYSREG_ISPPRE_BASE_ADDR		0x16210000
#define SYSREG_ISPLP_BASE_ADDR		0x16410000
#define SYSREG_ISPHQ_BASE_ADDR		0x16610000
#define SYSREG_DCF_BASE_ADDR		0x16A10000
#define SYSREG_DCRD_BASE_ADDR		0x16B10000
#define SYSREG_DCPOST_BASE_ADDR		0x16810000

#define HWFC_INDEX_RESET_ADDR		0x16441050
#define QE_3AA0_BASE_ADDR		0x16330000
#define QE_ISPLP_BASE_ADDR		0x16490000
#define QE_3AA1_BASE_ADDR		0x16340000
#define QE_PDP_BASE_ADDR		0x16320000

#define CSIS0_QCH_EN_ADDR		0x16230004
#define CSIS1_QCH_EN_ADDR		0x16240004
#define CSIS2_QCH_EN_ADDR		0x16250004
#define CSIS3_QCH_EN_ADDR		0x16260004

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
#define MCSC_INTR_MASK		(0x00000034)
#define USE_DMA_BUFFER_INDEX	(0) /* 0 ~ 7 */
#define MCSC_PRECISION		(20)
#define MCSC_POLY_RATIO_UP	(14)
#define MCSC_POLY_RATIO_DOWN	(16)
#define MCSC_POST_RATIO_DOWN	(16)
/* #define MCSC_POST_WA */
#define MCSC_POST_WA_SHIFT	(8)	/* 256 = 2^8 */

#define MCSC_OUTPUT_SSB		(0xF)	/* This number has no special meaning. */

#define MCSC_USE_DEJAG_TUNING_PARAM		(true)
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
	INTR_MC_SCALER_OUTSTALL			= 12,
	INTR_MC_SCALER_SHADOW_COPY_FINISH	= 16,
	INTR_MC_SCALER_SHADOW_COPY_FINISH_OVF	= 17,
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
