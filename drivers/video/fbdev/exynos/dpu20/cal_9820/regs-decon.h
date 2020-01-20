/*
 * linux/drivers/video/fbdev/exynos/dpu_9810/regs_decon.h
 *
 * Register definition file for Samsung DECON driver
 *
 * Copyright (c) 2014 Samsung Electronics
 * SeungBeom park<sb1.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _REGS_DISP_SS_H
#define _REGS_DISP_SS_H

#define DISP_DPU_MIPI_PHY_CON			0x0008
/* _v : [0,1] */
#define SEL_RESET_DPHY_MASK(_v)			(0x1 << (4 + (_v)))
#define M_RESETN_M4S4_MODULE_MASK		(0x1 << 1)
#define M_RESETN_M4S4_TOP_MASK			(0x1 << 0)

#define DISP_DPU_TE_QACTIVE_PLL_EN		0x0010
#define TE_QACTIVE_PLL_EN			(0x1 << 0)

#endif /* _REGS_DISP_SS_H */

#ifndef _REGS_DECON_H
#define _REGS_DECON_H

/*
 * [ BLK_DPU BASE ADDRESS ]
 *
 * - CMU_DPU			0x1900_0000
 * - SYSREG_DPU			0x1901_0000
 * - DPP			0x1902_0000
 * - DECON0			0x1903_0000
 * - DECON1			0x1904_0000
 * - DECON2			0x1905_0000
 * - DPU_WB_MUX			0x1906_0000
 * - DPU_DMA			0x1907_0000
 * - MIPI_DSIM0			0x1908_0000
 * - MIPI_DSIM1			0x1909_0000
 * - SYSMMU_DPUD0		0x190A_0000
 * - SYSMMU_DPUD1		0x190B_0000
 * - SYSMMU_DPUD2		0x190C_0000
 * - SYSMMU_DPUD0_S		0x190D_0000
 * - SYSMMU_DPUD1_S		0x190E_0000
 * - SYSMMU_DPUD2_S		0x190F_0000
 * - PPMU_DPUD0			0x1910_0000
 * - PPMU_DPUD1			0x1911_0000
 * - PPMU_DPUD2			0x1912_0000
 * - BTM_DPUD0			0x1913_0000
 * - BTM_DPUD1			0x1914_0000
 * - BTM_DPUD2			0x1915_0000
 * - MIPI_DCPHY			0x1916_0000
 * - DPU_PGEN			0x191A_0000
 * - HDR			0x191C_0000
 */

/*
 *	IP			start_offset	end_offset
 *=================================================
 *	DECON			0x0000		0x0fff
 *	BLENDER			0x1000		0x1fff
 *-------------------------------------------------
 *	DSC0			0x4000		0x4fff
 *	DSC1			0x5000		0x5fff
 *	DSC2			0x6000		0x6fff
 *-------------------------------------------------
 *	SHD_DECON		0x7000		0x7FFF
 *-------------------------------------------------
 *	SHD_BLENDER		0x8000		0x8FFF
 *-------------------------------------------------
 *	SHD_DSC0		0xB000		0xBFFF
 *	SHD_DSC1		0xC000		0xCFFF
 *	SHD_DSC2		0xD000		0xCFFF
 *-------------------------------------------------
 */


/*
 * DECON registers
 * ->
 * updated by SHADOW_REG_UPDATE_REQ[31] : SHADOW_REG_UPDATE_REQ
 *	(0x0000~0x011C, 0x0230~0x209C )
 */

#define GLOBAL_CONTROL				0x0000
#define GLOBAL_CONTROL_SRESET			(1 << 28)
#define GLOBAL_CONTROL_PSLVERR_EN		(1 << 24)
#define GLOBAL_CONTROL_TEN_BPC_MODE_F		(1 << 20)
#define GLOBAL_CONTROL_TEN_BPC_MODE_MASK	(1 << 20)
#define GLOBAL_CONTROL_MASTER_MODE_F(_v)	((_v) << 12)
#define GLOBAL_CONTROL_MASTER_MODE_MASK		(0xF << 12)
#define GLOBAL_CONTROL_OPERATION_MODE_F		(1 << 8)
#define GLOBAL_CONTROL_OPERATION_MODE_VIDEO_F	(0 << 8)
#define GLOBAL_CONTROL_OPERATION_MODE_CMD_F	(1 << 8)
#define GLOBAL_CONTROL_IDLE_STATUS		(1 << 5)
#define GLOBAL_CONTROL_RUN_STATUS		(1 << 4)
#define GLOBAL_CONTROL_DECON_EN			(1 << 1)
#define GLOBAL_CONTROL_DECON_EN_F		(1 << 0)

#define RESOURCE_OCCUPANCY_INFO_0		0x0010
#define RESOURCE_OCCUPANCY_INFO_1		0x0014
#define RESOURCE_OCCUPANCY_INFO_2		0x0018

#define SRAM_SHARE_ENABLE			0x0030
#define SRAM0_SHARE_ENABLE_F			(1 << 12)
#define SRAM1_SHARE_ENABLE_F			(1 << 16)
#define SRAM2_SHARE_ENABLE_F			(1 << 20)
#define SRAM3_SHARE_ENABLE_F			(1 << 24)
#define ALL_SRAM_SHARE_ENABLE			(0x1111 << 12)
#define ALL_SRAM_SHARE_DISABLE			(0x0)

#define INTERRUPT_ENABLE			0x0040
#define DPU_DQE_DIMMING_END_INT_EN		(1 << 21)
#define DPU_DQE_DIMMING_START_INT_EN		(1 << 20)
#define DPU_FRAME_DONE_INT_EN			(1 << 13)
#define DPU_FRAME_START_INT_EN			(1 << 12)
#define DPU_EXTRA_INT_EN			(1 << 4)
#define DPU_INT_EN				(1 << 0)
#define INTERRUPT_ENABLE_MASK			0x3011

#define EXTRA_INTERRUPT_ENABLE			0x0044
#define DPU_RESOURCE_CONFLICT_INT_EN		(1 << 8)
#define DPU_TIME_OUT_INT_EN			(1 << 4)

#define TIME_OUT_VALUE				0x0048

#define INTERRUPT_PENDING			0x004C
#define DPU_DQE_DIMMING_END_INT_PEND		(1 << 21)
#define DPU_DQE_DIMMING_START_INT_PEND		(1 << 20)
#define DPU_FRAME_DONE_INT_PEND			(1 << 13)
#define DPU_FRAME_START_INT_PEND		(1 << 12)
#define DPU_EXTRA_INT_PEND			(1 << 4)

#define EXTRA_INTERRUPT_PENDING			0x0050
#define DPU_RESOURCE_CONFLICT_INT_PEND		(1 << 8)
#define DPU_TIME_OUT_INT_PEND			(1 << 4)

#define SHADOW_REG_UPDATE_REQ			0x0060
#define SHADOW_REG_UPDATE_REQ_GLOBAL		(1 << 31)
#define SHADOW_REG_UPDATE_REQ_DQE		(1 << 28)
#define SHADOW_REG_UPDATE_REQ_WIN(_win)		(1 << (_win))
#define SHADOW_REG_UPDATE_REQ_FOR_DECON		(0x3f)

#define SECURE_CONTROL				0x0064
#define TZPC_FLAG_WIN(_win)			(1 << (_win))

#define HW_SW_TRIG_CONTROL			0x0070
#define HW_SW_TRIG_HS_STATUS			(1 << 28)
#define HW_TRIG_SEL(_v)				((_v) << 24)
#define HW_TRIG_SEL_MASK			(0x3 << 24)
#define HW_TRIG_SEL_FROM_DDI1			(1 << 24)
#define HW_TRIG_SEL_FROM_DDI0			(0 << 24)
#define HW_TRIG_SKIP(_v)			((_v) << 16)
#define HW_TRIG_SKIP_MASK			(0xff << 16)
#define HW_TRIG_ACTIVE_VALUE			(1 << 13)
#define HW_TRIG_EDGE_POLARITY			(1 << 12)
#define SW_TRIG_EN				(1 << 8)
#define HW_TRIG_MASK_SLAVE0			(1 << 5)
#define HW_TRIG_MASK_DECON			(1 << 4)
#define HW_SW_TRIG_TIMER_CLEAR			(1 << 3)
#define HW_SW_TRIG_TIMER_EN			(1 << 2)
#define HW_TRIG_EN				(1 << 0)

#define HW_SW_TRIG_TIMER			0x0074

#define HW_TE_CNT				0x0078
#define HW_TRIG_CNT_B_GET(_v)			(((_v) >> 16) & 0xffff)
#define HW_TRIG_CNT_A_GET(_v)			(((_v) >> 0) & 0xffff)

#define HW_SW_TRIG_CONTROL_SECURE		0x007C
#define HW_TRIG_MASK_SECURE_SLAVE1		(1 << 6)
#define HW_TRIG_MASK_SECURE_SLAVE0		(1 << 5)
#define HW_TRIG_MASK_SECURE			(1 << 4)

#define PLL_SLEEP_CONTROL			0x0090
#define PLL_SLEEP_MASK_OUTIF1			(1 << 5)
#define PLL_SLEEP_MASK_OUTIF0			(1 << 4)
#define PLL_SLEEP_EN_OUTIF1_F			(1 << 1)
#define PLL_SLEEP_EN_OUTIF0_F			(1 << 0)

#define SCALED_SIZE_CONTROL_0			0x00A0
#define SCALED_SIZE_HEIGHT_F(_v)		((_v) << 16)
#define SCALED_SIZE_HEIGHT_MASK			(0x3fff << 16)
#define SCALED_SIZE_HEIGHT_GET(_v)		(((_v) >> 16) & 0x3fff)
#define SCALED_SIZE_WIDTH_F(_v)			((_v) << 0)
#define SCALED_SIZE_WIDTH_MASK			(0x3fff << 0)
#define SCALED_SIZE_WIDTH_GET(_v)		(((_v) >> 0) & 0x3fff)

#define CLOCK_CONTROL_0				0x00F0
/*
 * [28] QACTIVE_PLL_VALUE = 0
 * [24] QACTIVE_VALUE = 0
 *   0: QACTIVE is dynamically changed by DECON h/w,
 *   1: QACTIVE is stuck to 1'b1
 * [16][12][8][4][0] AUTO_CG_EN_xxx
 */
/* clock gating is disabled on bringup */
#define CLOCK_CONTROL_0_CG_MASK			(0x11111 << 0)
#define CLOCK_CONTROL_0_QACTIVE_MASK		((0x1 << 24) | (0x1 << 28))
#define CLOCK_CONTROL_0_TE_QACTIVE_PLL_ON	(0x1 << 28)

#define SPLITTER_SIZE_CONTROL_0			0x0100
#define SPLITTER_HEIGHT_F(_v)			((_v) << 16)
#define SPLITTER_HEIGHT_MASK			(0x3fff << 16)
#define SPLITTER_HEIGHT_GET(_v)			(((_v) >> 16) & 0x3fff)
#define SPLITTER_WIDTH_F(_v)			((_v) << 0)
#define SPLITTER_WIDTH_MASK			(0x3fff << 0)
#define SPLITTER_WIDTH_GET(_v)			(((_v) >> 0) & 0x3fff)

#define SPLITTER_SPLIT_IDX_CONTROL		0x0104
#define SPLITTER_SPLIT_IDX_F(_v)		((_v) << 0)
#define SPLITTER_SPLIT_IDX_MASK			(0x3fff << 0)
#define SPLITTER_OVERLAP_F(_v)			((_v) << 16)
#define SPLITTER_OVERLAP_MASK			(0x7f << 16)

#define OUTFIFO_SIZE_CONTROL_0			0x0120
#define OUTFIFO_HEIGHT_F(_v)			((_v) << 16)
#define OUTFIFO_HEIGHT_MASK			(0x3fff << 16)
#define OUTFIFO_HEIGHT_GET(_v)			(((_v) >> 16) & 0x3fff)
#define OUTFIFO_WIDTH_F(_v)			((_v) << 0)
#define OUTFIFO_WIDTH_MASK			(0x3fff << 0)
#define OUTFIFO_WIDTH_GET(_v)			(((_v) >> 0) & 0x3fff)

#define OUTFIFO_SIZE_CONTROL_1			0x0124
#define OUTFIFO_1_WIDTH_F(_v)			((_v) << 0)
#define OUTFIFO_1_WIDTH_MASK			(0x3fff << 0)
#define OUTFIFO_1_WIDTH_GET(_v)			(((_v) >> 0) & 0x3fff)

#define OUTFIFO_SIZE_CONTROL_2			0x0128
#define OUTFIFO_COMPRESSED_SLICE_HEIGHT_F(_v)	((_v) << 16)
#define OUTFIFO_COMPRESSED_SLICE_HEIGHT_MASK	(0x3fff << 16)
#define OUTFIFO_COMPRESSED_SLICE_HEIGHT_GET(_v)	(((_v) >> 16) & 0x3fff)
#define OUTFIFO_COMPRESSED_SLICE_WIDTH_F(_v)	((_v) << 0)
#define OUTFIFO_COMPRESSED_SLICE_WIDTH_MASK	(0x3fff << 0)
#define OUTFIFO_COMPRESSED_SLICE_WIDTH_GET(_v)	(((_v) >> 0) & 0x3fff)

#define OUTFIFO_TH_CONTROL_0			0x012C
#define OUTFIFO_TH_1H_F				(0x5 << 0)
#define OUTFIFO_TH_2H_F				(0x6 << 0)
#define OUTFIFO_TH_F(_v)			((_v) << 0)
#define OUTFIFO_TH_MASK				(0x7 << 0)
#define OUTFIFO_TH_GET(_v)			((_v) >> 0 & 0x7)

#define OUTFIFO_DATA_ORDER_CONTROL		0x0130
#define OUTFIFO_PIXEL_ORDER_SWAP_F(_v)		((_v) << 4)
#define OUTFIFO_PIXEL_ORDER_SWAP_MASK		(0x7 << 4)
#define OUTFIFO_PIXEL_ORDER_SWAP_GET(_v)	(((_v) >> 4) & 0x7)

#define READ_URGENT_CONTROL_0			0x0140
#define READ_URGETN_GENERATION_EN_F		(0x1 << 0)

#define READ_URGENT_CONTROL_1			0x0144
#define READ_URGENT_HIGH_THRESHOLD_F(_v)	((_v) << 16)
#define READ_URGENT_HIGH_THRESHOLD_MASK		(0xffff << 16)
#define READ_URGENT_HIGH_THRESHOLD_GET(_v)	(((_v) >> 16) & 0xffff)
#define READ_URGENT_LOW_THRESHOLD_F(_v)		((_v) << 0)
#define READ_URGENT_LOW_THRESHOLD_MASK		(0xffff << 0)
#define READ_URGENT_LOW_THRESHOLD_GET(_v)	(((_v) >> 0) & 0xffff)

#define READ_URGENT_CONTROL_2			0x0148
#define READ_URGENT_WAIT_CYCLE_F(_v)		((_v) << 0)
#define READ_URGENT_WAIT_CYCLE_GET(_v)		((_v) >> 0)

#define DTA_CONTROL				0x0180
#define DTA_EN_F				(1 << 0)

#define DTA_THRESHOLD				0x0184
#define DTA_HIGH_TH_F(_v)			((_v) << 16)
#define DTA_HIGH_TH_MASK			(0xffff << 16)
#define DTA_HIGH_TH_GET(_v)			(((_v) >> 16) & 0xffff)
#define DTA_LOW_TH_F(_v)			((_v) << 0)
#define DTA_LOW_TH_MASK				(0xffff << 0)
#define DTA_LOW_TH_GET(_v)			(((_v) >> 0) & 0xffff)

#define BLENDER_BG_IMAGE_SIZE_0			0x0200
#define BLENDER_BG_HEIGHT_F(_v)			((_v) << 16)
#define BLENDER_BG_HEIGHT_MASK			(0x3fff << 16)
#define BLENDER_BG_HEIGHT_GET(_v)		(((_v) >> 16) & 0x3fff)
#define BLENDER_BG_WIDTH_F(_v)			((_v) << 0)
#define BLENDER_BG_WIDTH_MASK			(0x3fff << 0)
#define BLENDER_BG_WIDTH_GET(_v)		(((_v) >> 0) & 0x3fff)

#define BLENDER_BG_IMAGE_COLOR_0		0x0208
#define BLENDER_BG_A_F(_v)			((_v) << 16)
#define BLENDER_BG_A_MASK			(0xff << 16)
#define BLENDER_BG_A_GET(_v)			(((_v) >> 16) & 0xff)
#define BLENDER_BG_R_F(_v)			((_v) << 0)
#define BLENDER_BG_R_MASK			(0x3ff << 0)
#define BLENDER_BG_R_GET(_v)			(((_v) >> 0) & 0x3ff)

#define BLENDER_BG_IMAGE_COLOR_1		0x020C
#define BLENDER_BG_G_F(_v)			((_v) << 16)
#define BLENDER_BG_G_MASK			(0x3ff << 16)
#define BLENDER_BG_G_GET(_v)			(((_v) >> 16) & 0x3ff)
#define BLENDER_BG_B_F(_v)			((_v) << 0)
#define BLENDER_BG_B_MASK			(0x3ff << 0)
#define BLENDER_BG_B_GET(_v)			(((_v) >> 0) & 0x3ff)

#define LRMERGER_MODE_CONTROL			0x0210
#define LRM23_MODE_F(_v)			((_v) << 16)
#define LRM23_MODE_MASK				(0x7 << 16)
#define LRM01_MODE_F(_v)			((_v) << 0)
#define LRM01_MODE_MASK				(0x7 << 0)

#define DATA_PATH_CONTROL_0			0x0214
#define WIN_MAPCOLOR_EN_F(_win)			(1 << (4*_win + 1))
#define WIN_EN_F(_win)				(1 << (4*_win + 0))

#define DATA_PATH_CONTROL_1			0x0218
#define WIN_CHMAP_F(_win, _ch)			(((_ch) & 0x7) << (4*_win))
#define WIN_CHMAP_MASK(_win)			(0x7 << (4*_win))

#define DATA_PATH_CONTROL_2			0x0230
#define SCALER_PATH_F(_v)			((_v) << 24)
#define SCALER_PATH_MASK			(0x3 << 24)
#define SCALER_PATH_GET(_v)			(((_v) >> 24) & 0x3)
#define EHNANCE_PATH_F(_v)			((_v) << 12)
#define EHNANCE_PATH_MASK			(0x7 << 12)
#define EHNANCE_PATH_GET(_v)			(((_v) >> 12) & 0x7)
#define COMP_OUTIF_PATH_F(_v)			((_v) << 0)
#define COMP_OUTIF_PATH_MASK			(0xff << 0)
#define COMP_OUTIF_PATH_GET(_v)			(((_v) >> 0) & 0xff)

#define DSIM_CONNECTION_CONTROL			0x0250
#define DSIM_CONNECTION_DSIM1_F(_v)		((_v) << 4)
#define DSIM_CONNECTION_DSIM1_MASK		(0x7 << 4)
#define DSIM_CONNECTION_DSIM1_GET(_v)		(((_v) >> 4) & 0x7)
#define DSIM_CONNECTION_DSIM0_F(_v)		((_v) << 0)
#define DSIM_CONNECTION_DSIM0_MASK		(0x7 << 0)
#define DSIM_CONNECTION_DSIM0_GET(_v)		(((_v) >> 0) & 0x7)

#define DSIM0_TE_TIMEOUT_CONTROL		0x0254
#define DSIM0_TE_TIMEOUT_CNT(_v)		((_v) << 0)
#define DSIM0_TE_TIMEOUT_CNT_MASK		(0xffff << 0)
#define DSIM0_TE_TIMEOUT_CNT_GET(_v)		(((_v) >> 0) & 0xffff)

#define DSIM1_TE_TIMEOUT_CONTROL		0x0258
#define DSIM1_TE_TIMEOUT_CNT(_v)		((_v) << 0)
#define DSIM1_TE_TIMEOUT_CNT_MASK		(0xffff << 0)
#define DSIM1_TE_TIMEOUT_CNT_GET(_v)		(((_v) >> 0) & 0xffff)

#define DSIM0_START_TIME_CONTROL		0x0260
#define DSIM0_START_TIME(_v)			((_v) << 0)

#define DSIM1_START_TIME_CONTROL		0x0264
#define DSIM1_START_TIME(_v)			((_v) << 0)

#define DP_CONNECTION_CONTROL			0x0270
#define DP_CONNECTION_SEL_DP1(_v)		((_v) << 4)
#define DP_CONNECTION_SEL_DP1_MASK		(0x7 << 4)
#define DP_CONNECTION_SEL_DP1_GET(_v)		(((_v) >> 4) & 0x7)
#define DP_CONNECTION_SEL_DP0(_v)		((_v) << 0)
#define DP_CONNECTION_SEL_DP0_MASK		(0x7 << 0)
#define DP_CONNECTION_SEL_DP0_GET(_v)		(((_v) >> 0) & 0x7)

/* DECON CRC for ASB */
#define CRC_DATA_0				0x0280
#define CRC_DATA_DSIMIF1_GET(_v)		(((_v) >> 16) & 0xffff)
#define CRC_DATA_DSIMIF0_GET(_v)		(((_v) >> 0) & 0xffff)

#define CRC_DATA_2				0x0288
#define CRC_DATA_DP1_GET(_v)			(((_v) >> 16) & 0xffff)
#define CRC_DATA_DP0_GET(_v)			(((_v) >> 0) & 0xffff)

#define CRC_CONTROL				0x028C
#define CRC_COLOR_SEL(_v)			((_v) << 16)
#define CRC_COLOR_SEL_MASK			(0x3 << 16)
#define CRC_START				(1 << 0)

#define FRAME_COUNT				0x02A0

/* BLENDER */
#define WIN_CONTROL_0(_win)			(0x1000 + ((_win) * 0x30))
#define WIN_ALPHA1_F(_v)			(((_v) & 0xFF) << 24)
#define WIN_ALPHA1_MASK				(0xFF << 24)
#define WIN_ALPHA0_F(_v)			(((_v) & 0xFF) << 16)
#define WIN_ALPHA0_MASK				(0xFF << 16)
#define WIN_ALPHA_GET(_v, _n)			(((_v) >> (16 + 8 * (_n))) & 0xFF)
#define WIN_FUNC_F(_v)				(((_v) & 0xF) << 8)
#define WIN_FUNC_MASK				(0xF << 8)
#define WIN_FUNC_GET(_v)			(((_v) >> 8) & 0xf)
#define WIN_SRESET				(1 << 4)
#define WIN_ALPHA_MULT_SRC_SEL_F(_v)		(((_v) & 0x3) << 0)
#define WIN_ALPHA_MULT_SRC_SEL_MASK		(0x3 << 0)

#define WIN_CONTROL_1(_win)			(0x1004 + ((_win) * 0x30))
#define WIN_FG_ALPHA_D_SEL_F(_v)		(((_v) & 0xF) << 24)
#define WIN_FG_ALPHA_D_SEL_MASK			(0xF << 24)
#define WIN_BG_ALPHA_D_SEL_F(_v)		(((_v) & 0xF) << 16)
#define WIN_BG_ALPHA_D_SEL_MASK			(0xF << 16)
#define WIN_FG_ALPHA_A_SEL_F(_v)		(((_v) & 0xF) << 8)
#define WIN_FG_ALPHA_A_SEL_MASK			(0xF << 8)
#define WIN_BG_ALPHA_A_SEL_F(_v)		(((_v) & 0xF) << 0)
#define WIN_BG_ALPHA_A_SEL_MASK			(0xF << 0)

#define WIN_START_POSITION(_win)		(0x1008 + ((_win) * 0x30))
#define WIN_STRPTR_Y_F(_v)			(((_v) & 0x3FFF) << 16)
#define WIN_STRPTR_X_F(_v)			(((_v) & 0x3FFF) << 0)

#define WIN_END_POSITION(_win)			(0x100C + ((_win) * 0x30))
#define WIN_ENDPTR_Y_F(_v)			(((_v) & 0x3FFF) << 16)
#define WIN_ENDPTR_X_F(_v)			(((_v) & 0x3FFF) << 0)

#define WIN_COLORMAP_0(_win)			(0x1010 + ((_win) * 0x30))
#define WIN_MAPCOLOR_A_F(_v)			((_v) << 16)
#define WIN_MAPCOLOR_A_MASK			(0xff << 16)
#define WIN_MAPCOLOR_R_F(_v)			((_v) << 0)
#define WIN_MAPCOLOR_R_MASK			(0x3ff << 0)

#define WIN_COLORMAP_1(_win)			(0x1014 + ((_win) * 0x30))
#define WIN_MAPCOLOR_G_F(_v)			((_v) << 16)
#define WIN_MAPCOLOR_G_MASK			(0x3ff << 16)
#define WIN_MAPCOLOR_B_F(_v)			((_v) << 0)
#define WIN_MAPCOLOR_B_MASK			(0x3ff << 0)

#define WIN_START_TIME_CONTROL(_win)		(0x1018 + ((_win) * 0x30))
#define WIN_START_TIME_CONTROL_F(_v)		((_v) << 0)
#define WIN_START_TIME_CONTROL_MASK		(0x3fff << 0)

/*
 * DSC registers
 * ->
 * 0x4000 ~
 * DSC 0 : 0x4000
 * DSC 1 : 0x5000
 * DSC 2 : 0x6000
 *
 * <-
 * DSC registers
 */

#define DSC0_OFFSET				0x4000
#define DSC1_OFFSET				0x5000
#define DSC2_OFFSET				0x6000

#define DSC_CONTROL0				0x0000
#define DSC_SW_RESET				(0x1 << 28)
#define DSC_DCG_EN_REF(_v)			((_v) << 19)
#define DSC_DCG_EN_SSM(_v)			((_v) << 18)
#define DSC_DCG_EN_ICH(_v)			((_v) << 17)
#define DSC_DCG_EN_ALL_OFF			(0x0 << 17)
#define DSC_DCG_EN_ALL_MASK			(0x7 << 17)
#define DSC_BIT_SWAP(_v)			((_v) << 10)
#define DSC_BYTE_SWAP(_v)			((_v) << 9)
#define DSC_WORD_SWAP(_v)			((_v) << 8)
#define DSC_SWAP(_b, _c, _w)			((_b << 10) |\
							(_c << 9) | (_w << 8))
#define DSC_SWAP_MASK				((1 << 10) |\
							(1 << 9) | (1 << 8))
#define DSC_FLATNESS_DET_TH_MASK		(0xf << 4)
#define DSC_FLATNESS_DET_TH_F(_v)		((_v) << 4)
#define DSC_SLICE_MODE_CH_MASK			(0x1 << 3)
#define DSC_SLICE_MODE_CH_F(_v)			((_v) << 3)
#define DSC_CG_EN_MASK				(0x1 << 1)
#define DSC_CG_EN_F(_v)				((_v) << 1)
#define DSC_DUAL_SLICE_EN_MASK			(0x1 << 0)
#define DSC_DUAL_SLICE_EN_F(_v)			((_v) << 0)

#define DSC_CONTROL3				0x000C
#define DSC_REMAINDER_F(_v)			((_v) << 12)
#define DSC_REMAINDER_MASK			(0x3 << 12)
#define DSC_REMAINDER_GET(_v)			(((_v) >> 12) & 0x3)
#define DSC_GRPCNTLINE_F(_v)			((_v) << 0)
#define DSC_GRPCNTLINE_MASK			(0x7ff << 0)
#define DSC_GRPCNTLINE_GET(_v)			(((_v) >> 0) & 0x7ff)

#define DSC_CRC_0				0x0010
#define DSC_CRC_EN_MASK				(0x1 << 16)
#define DSC_CRC_EN_F(_v)			((_v) << 16)
#define DSC_CRC_CODE_MASK			(0xffff << 0)
#define DSC_CRC_CODE_F(_v)			((_v) << 0)

#define DSC_CRC_1				0x0014
#define DSC_CRC_Y_S0_MASK			(0xffff << 16)
#define DSC_CRC_Y_S0_F(_v)			((_v) << 16)
#define DSC_CRC_CO_S0_MASK			(0xffff << 0)
#define DSC_CRC_CO_S0_F(_v)			((_v) << 0)

#define DSC_CRC_2				0x0018
#define DSC_CRC_CG_S0_MASK			(0xffff << 16)
#define DSC_CRC_CG_S0_F(_v)			((_v) << 16)
#define DSC_CRC_Y_S1_MASK			(0xffff << 0)
#define DSC_CRC_Y_S1_F(_v)			((_v) << 0)

#define DSC_CRC_3				0x001C
#define DSC_CRC_CO_S1_MASK			(0xffff << 16)
#define DSC_CRC_CO_S1_F(_v)			((_v) << 16)
#define DSC_CRC_CG_S1_MASK			(0xffff << 0)
#define DSC_CRC_CG_S1_F(_v)			((_v) << 0)

#define DSC_PPS00_03				0x0020
#define PPS00_VER(_v)				((_v) << 24)
#define PPS00_VER_MASK				(0xff << 24)
#define PPS01_ID(_v)				(_v << 16)
#define PPS01_ID_MASK				(0xff << 16)
#define PPS03_BPC_LBD_MASK			(0xffff << 0)
#define PPS03_BPC_LBD(_v)			(_v << 0)

#define DSC_PPS04_07				0x0024
#define PPS04_COMP_CFG(_v)			((_v) << 24)
#define PPS04_COMP_CFG_MASK			(0x3f << 24)
#define PPS05_BPP(_v)				(_v << 16)
#define PPS05_BPP_MASK				(0xff << 16)
#define PPS06_07_PIC_HEIGHT_MASK		(0xffff << 0)
#define PPS06_07_PIC_HEIGHT(_v)			(_v << 0)

#define DSC_PPS08_11				0x0028
#define PPS08_09_PIC_WIDHT_MASK			(0xffff << 16)
#define PPS08_09_PIC_WIDHT(_v)			((_v) << 16)
#define PPS10_11_SLICE_HEIGHT_MASK		(0xffff << 0)
#define PPS10_11_SLICE_HEIGHT(_v)		(_v << 0)

#define DSC_PPS12_15				0x002C
#define PPS12_13_SLICE_WIDTH_MASK		(0xffff << 16)
#define PPS12_13_SLICE_WIDTH(_v)		((_v) << 16)
#define PPS14_15_CHUNK_SIZE_MASK		(0xffff << 0)
#define PPS14_15_CHUNK_SIZE(_v)			(_v << 0)

#define DSC_PPS16_19				0x0030
#define PPS16_17_INIT_XMIT_DELAY_MASK		(0x3ff << 16)
#define PPS16_17_INIT_XMIT_DELAY(_v)		((_v) << 16)
#define PPS18_19_INIT_DEC_DELAY_MASK		(0xffff << 0)
#define PPS18_19_INIT_DEC_DELAY(_v)		((_v) << 0)

#define DSC_PPS20_23				0x0034
#define PPS21_INIT_SCALE_VALUE_MASK		(0x3f << 16)
#define PPS21_INIT_SCALE_VALUE(_v)		((_v) << 16)
#define PPS22_23_SCALE_INC_INTERVAL_MASK	(0xffff << 0)
#define PPS22_23_SCALE_INC_INTERVAL(_v)		(_v << 0)

#define DSC_PPS24_27				0x0038
#define PPS24_25_SCALE_DEC_INTERVAL_MASK	(0xfff << 16)
#define PPS24_25_SCALE_DEC_INTERVAL(_v)		((_v) << 16)
/* FL : First Line */
#define PPS27_FL_BPG_OFFSET_MASK		(0x1f << 0)
#define PPS27_FL_BPG_OFFSET(_v)			(_v << 0)

#define DSC_PPS28_31				0x003C
/* NFL : Not First Line */
#define PPS28_29_NFL_BPG_OFFSET_MASK		(0xffff << 16)
#define PPS28_29_NFL_BPG_OFFSET(_v)		((_v) << 16)
#define PPS30_31_SLICE_BPG_OFFSET_MASK		(0xffff << 0)
#define PPS30_31_SLICE_BPG_OFFSET(_v)		(_v << 0)

#define DSC_PPS32_35				0x0040
#define PPS32_33_INIT_OFFSET_MASK		(0xffff << 16)
#define PPS32_33_INIT_OFFSET(_v)		((_v) << 16)
#define PPS34_35_FINAL_OFFSET_MASK		(0xffff << 0)
#define PPS34_35_FINAL_OFFSET(_v)		(_v << 0)

#define DSC_PPS36_39				0x0044
#define PPS36_FLATNESS_MIN_QP_MASK		(0xff << 24)
#define PPS36_FLATNESS_MIN_QP(_v)		((_v) << 24)
#define PPS37_FLATNESS_MAX_QP_MASK		(0xff << 16)
#define PPS37_FLATNESS_MAX_QP(_v)		((_v) << 16)
#define PPS38_39_RC_MODEL_SIZE_MASK		(0xffff << 0)
#define PPS38_39_RC_MODEL_SIZE(_v)		(_v << 0)

#define DSC_PPS40_43				0x0048
#define PPS40_RC_EDGE_FACTOR_MASK		(0xff << 24)
#define PPS40_RC_EDGE_FACTOR(_v)		((_v) << 24)
#define PPS41_RC_QUANT_INCR_LIMIT0_MASK		(0xff << 16)
#define PPS41_RC_QUANT_INCR_LIMIT0(_v)		((_v) << 16)
#define PPS42_RC_QUANT_INCR_LIMIT1_MASK		(0xff << 8)
#define PPS42_RC_QUANT_INCR_LIMIT1(_v)		((_v) << 8)
#define PPS44_RC_TGT_OFFSET_HI_MASK		(0xf << 4)
#define PPS44_RC_TGT_OFFSET_HI(_v)		((_v) << 4)
#define PPS44_RC_TGT_OFFSET_LO_MASK		(0xf << 0)
#define PPS44_RC_TGT_OFFSET_LO(_v)		((_v) << 0)

#define DSC_PPS44_47				0x004C
#define PPS44_RC_BUF_THRESH_0_MASK		(0xff << 24)
#define PPS44_RC_BUF_THRESH_0(_v)		((_v) << 24)
#define PPS45_RC_BUF_THRESH_1_MASK		(0xff << 16)
#define PPS45_RC_BUF_THRESH_1(_v)		((_v) << 16)
#define PPS46_RC_BUF_THRESH_2_MASK		(0xff << 8)
#define PPS46_RC_BUF_THRESH_3(_v)		((_v) << 8)
#define PPS47_RC_BUF_THRESH_3_MASK		(0xff << 0)
#define PPS47_RC_BUF_THRESH_3(_v)		((_v) << 0)

#define DSC_PPS48_51				0x0050
#define PPS48_RC_BUF_THRESH_4_MASK		(0xff << 24)
#define PPS48_RC_BUF_THRESH_4(_v)		((_v) << 24)
#define PPS49_RC_BUF_THRESH_5_MASK		(0xff << 16)
#define PPS49_RC_BUF_THRESH_5(_v)		((_v) << 16)
#define PPS50_RC_BUF_THRESH_6_MASK		(0xff << 8)
#define PPS50_RC_BUF_THRESH_6(_v)		((_v) << 8)
#define PPS51_RC_BUF_THRESH_7_MASK		(0xff << 0)
#define PPS51_RC_BUF_THRESH_7(_v)		((_v) << 0)

#define DSC_PPS52_55				0x0054
#define PPS52_RC_BUF_THRESH_8_MASK		(0xff << 24)
#define PPS52_RC_BUF_THRESH_8(_v)		((_v) << 24)
#define PPS53_RC_BUF_THRESH_9_MASK		(0xff << 16)
#define PPS53_RC_BUF_THRESH_9(_v)		((_v) << 16)
#define PPS54_RC_BUF_THRESH_A_MASK		(0xff << 8)
#define PPS54_RC_BUF_THRESH_A(_v)		((_v) << 8)
#define PPS55_RC_BUF_THRESH_B_MASK		(0xff << 0)
#define PPS55_RC_BUF_THRESH_B(_v)		((_v) << 0)

#define DSC_PPS56_59				0x0058
#define PPS56_RC_BUF_THRESH_C_MASK		(0xff << 24)
#define PPS56_RC_BUF_THRESH_C(_v)		((_v) << 24)
#define PPS57_RC_BUF_THRESH_D_MASK		(0xff << 16)
#define PPS57_RC_BUF_THRESH_D(_v)		((_v) << 16)
#define PPS58_RC_RANGE_PARAM_MASK		(0xff << 8)
#define PPS58_RC_RANGE_PARAM(_v)		(_v << 8)
#define PPS59_RC_RANGE_PARAM_MASK		(0xff << 0)
#define PPS59_RC_RANGE_PARAM(_v)		(_v << 0)
#define PPS58_59_RC_RANGE_PARAM_MASK		(0xFFFF << 0)
#define PPS58_59_RC_RANGE_PARAM(_v)		(_v << 0)


#define DSC_DEBUG_EN				0x0078
#define DSC_DBG_EN_MASK				(1 << 31)
#define DSC_DBG_EN(_v)				((_v) << 31)
#define DSC_DBG_SEL_MASK			(0xffff << 0)
#define DSC_DBG_SEL(_v)				((_v) << 0)

#define DSC_DEBUG_DATA				0x007C

#define DSCC_DEBUG_EN				0x0080
#define DSCC_DBG_EN_MASK			(1 << 31)
#define DSCC_DBG_EN(_v)				((_v) << 31)
#define DSCC_DBG_SEL_MASK			(0xffff << 0)
#define DSCC_DBG_SEL(_v)			((_v) << 0)

#define DSCC_DEBUG_DATA				0x0084



#define SHADOW_OFFSET				0x7000

#endif /* _REGS_DECON_H */
