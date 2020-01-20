/*
 * linux/drivers/video/fbdev/exynos/dpu20/cal_9110/regs-decon.h
 *
 * Register definition file for Samsung DECON driver
 *
 * Copyright (c) 2018 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _REGS_DISP_SS_H
#define _REGS_DISP_SS_H

#define DISP_DPU_MIPI_PHY_CON				0x0008
/* _v : [0,1] */
#define SEL_RESET_DPHY_MASK(_v)			(0x1 << (4 + (_v)))
#define M_RESETN_M4S4_MODULE_MASK		(0x1 << 1)
#define M_RESETN_M4S4_TOP_MASK			(0x1 << 0)

#define DISP_DPU_TE_QACTIVE_PLL_EN			0x0010
#define TE_QACTIVE_PLL_EN				(0x1 << 0)

#endif /* _REGS_DISP_SS_H */

#ifndef _REGS_DECON_H
#define _REGS_DECON_H

/*
 * [ BLK_DPU BASE ADDRESS ]
 *
 * - CMU_DISPAUD		0x1498_0000
 * - SYSREG_DPU			0x1484_0000
 * - DPP				0x1489_0000
 * - DECON0				0x148B_0000
 * - DPU_DMA			0x1488_0000
 * - MIPI_DSIM0			0x148E_0000
 * - DCPHY_TOP_M4S4		0x1447_0000
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
 *	SHD_BLENDER	0x8000		0x8FFF
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

#define GLOBAL_CONTROL					0x0000
#define GLOBAL_CONTROL_SRESET				(1 << 28)
#define GLOBAL_CONTROL_PSLVERR_EN				(1 << 24)
#define GLOBAL_CONTROL_OPERATION_MODE_F		(1 << 8)
#define GLOBAL_CONTROL_OPERATION_MODE_VIDEO_F	(0 << 8)
#define GLOBAL_CONTROL_OPERATION_MODE_CMD_F	(1 << 8)
#define GLOBAL_CONTROL_IDLE_STATUS			(1 << 5)
#define GLOBAL_CONTROL_RUN_STATUS			(1 << 4)
#define GLOBAL_CONTROL_DECON_EN				(1 << 1)
#define GLOBAL_CONTROL_DECON_EN_F			(1 << 0)

#define SRAM_SHARE_ENABLE				0x0030
#define SRAM1_SHARE_ENABLE_F		(1 << 16)
#define ALL_SRAM_SHARE_ENABLE			(0x1 << 16)
#define ALL_SRAM_SHARE_DISABLE			(0x0)

#define INTERRUPT_ENABLE				0x0040
#define DPU_FRAME_DONE_INT_EN		(1 << 13)
#define DPU_FRAME_START_INT_EN		(1 << 12)
#define DPU_EXTRA_INT_EN			(1 << 4)
#define DPU_INT_EN					(1 << 0)
#define INTERRUPT_ENABLE_MASK			0x3011

#define EXTRA_INTERRUPT_ENABLE			0x0044
#define DPU_TIME_OUT_INT_EN				(1 << 4)

#define TIME_OUT_VALUE					0x0048

#define INTERRUPT_PENDING				0x004C
#define DPU_FRAME_DONE_INT_PEND		(1 << 13)
#define DPU_FRAME_START_INT_PEND	(1 << 12)
#define DPU_EXTRA_INT_PEND			(1 << 4)

#define EXTRA_INTERRUPT_PENDING			0x0050
#define DPU_TIME_OUT_INT_PEND			(1 << 4)

#define SHADOW_REG_UPDATE_REQ				0x0060
#define SHADOW_REG_UPDATE_REQ_GLOBAL		(1 << 31)
#define SHADOW_REG_UPDATE_REQ_WIN(_win)		(1 << (_win))
#define SHADOW_REG_UPDATE_REQ_FOR_DECON		(0x7)

#define SECURE_CONTROL				0x0064
#define TZPC_FLAG_WIN(_win)				(1 << (_win))

#define HW_SW_TRIG_CONTROL				0x0070
#define HW_TRIG_SEL(_v)				((_v) << 24)
#define HW_TRIG_SEL_MASK			(0x3 << 24)
#define HW_TRIG_SEL_FROM_DDI1			(1 << 24)
#define HW_TRIG_SEL_FROM_DDI0			(0 << 24)
#define HW_TRIG_SKIP(_v)			((_v) << 16)
#define HW_TRIG_SKIP_MASK			(0xff << 16)
#define HW_TRIG_ACTIVE_VALUE			(1 << 13)
#define HW_TRIG_EDGE_POLARITY			(1 << 12)
#define SW_TRIG_EN					(1 << 8)
#define HW_TRIG_MASK_DECON			(1 << 4)
#define HW_SW_TRIG_TIMER_CLEAR		(1 << 3)
#define HW_SW_TRIG_TIMER_EN			(1 << 2)
#define HW_TRIG_EN				(1 << 0)

#define HW_SW_TRIG_TIMER				0x0074

#define HW_TE_CNT					0x0078
#define HW_TRIG_CNT_B(_v)				((_v) << 16)
#define HW_TRIG_CNT_B_MASK				(0xffff << 16)
#define HW_TRIG_CNT_A(_v)				((_v) << 0)
#define HW_TRIG_CNT_A_MASK				(0xffff << 0)

#define HW_SW_TRIG_CONTROL_SECURE		0x007C
#define HW_TRIG_MASK_SECURE			(1 << 4)

#define CLOCK_CONTROL_0				0x00F0
/*
 * [28] QACTIVE_PLL_VALUE = 0
 * [24] QACTIVE_VALUE = 0
 * 0: QACTIVE is dynamically changed by DECON h/w,
 * 1: QACTIVE is stuck to 1'b1
 * [16][12][8][0] AUTO_CG_EN_xxx
 */

/* clock gating is disabled on bringup */
#define CLOCK_CONTROL_0_CG_MASK			((0x1 << 0) | \
						(0x1 << 12) | (0x1 << 16))
#define CLOCK_CONTROL_0_QACTIVE_MASK	((0x1 << 24) | (0x1 << 28))
#define CLOCK_CONTROL_0_TE_QACTIVE_PLL_ON (0x1 << 28)

#define OUTFIFO_SIZE_CONTROL_0			0x0120
#define OUTFIFO_HEIGHT_F(_v)			((_v) << 16)
#define OUTFIFO_HEIGHT_MASK				(0x3fff << 16)
#define OUTFIFO_HEIGHT_GET(_v)			(((_v) >> 16) & 0x3fff)
#define OUTFIFO_WIDTH_F(_v)				((_v) << 0)
#define OUTFIFO_WIDTH_MASK				(0x3fff << 0)
#define OUTFIFO_WIDTH_GET(_v)			(((_v) >> 0) & 0x3fff)

#define OUTFIFO_TH_CONTROL_0			0x012C
#define OUTFIFO_TH_1H_F					(0x5 << 0)
#define OUTFIFO_TH_2H_F					(0x6 << 0)
#define OUTFIFO_TH_F(_v)				((_v) << 0)
#define OUTFIFO_TH_MASK					(0x7 << 0)
#define OUTFIFO_TH_GET(_v)				((_v) >> 0 & 0x7)

#define OUTFIFO_DATA_ORDER_CONTROL		0x0130
#define OUTFIFO_PIXEL_ORDER_SWAP_F(_v)		((_v) << 4)
#define OUTFIFO_PIXEL_ORDER_SWAP_MASK	(0x7 << 4)
#define OUTFIFO_PIXEL_ORDER_SWAP_GET(_v)	(((_v) >> 4) & 0x7)

#define BLENDER_BG_IMAGE_SIZE_0			0x0200
#define BLENDER_BG_HEIGHT_F(_v)			((_v) << 16)
#define BLENDER_BG_HEIGHT_MASK			(0x3fff << 16)
#define BLENDER_BG_HEIGHT_GET(_v)		(((_v) >> 16) & 0x3fff)
#define BLENDER_BG_WIDTH_F(_v)			((_v) << 0)
#define BLENDER_BG_WIDTH_MASK			(0x3fff << 0)
#define BLENDER_BG_WIDTH_GET(_v)		(((_v) >> 0) & 0x3fff)

#define BLENDER_BG_IMAGE_COLOR_0			0x0208
#define BLENDER_BG_A_F(_v)			((_v) << 16)
#define BLENDER_BG_A_MASK			(0xff << 16)
#define BLENDER_BG_A_GET(_v)			(((_v) >> 16) & 0xff)
#define BLENDER_BG_R_F(_v)			((_v) << 0)
#define BLENDER_BG_R_MASK			(0x3ff << 0)
#define BLENDER_BG_R_GET(_v)			(((_v) >> 0) & 0x3ff)

#define BLENDER_BG_IMAGE_COLOR_1			0x020C
#define BLENDER_BG_G_F(_v)			((_v) << 16)
#define BLENDER_BG_G_MASK			(0x3ff << 16)
#define BLENDER_BG_G_GET(_v)			(((_v) >> 16) & 0x3ff)
#define BLENDER_BG_B_F(_v)			((_v) << 0)
#define BLENDER_BG_B_MASK			(0x3ff << 0)
#define BLENDER_BG_B_GET(_v)			(((_v) >> 0) & 0x3ff)

#define LRMERGER_MODE_CONTROL				0x0210
#define LRM23_MODE_F(_v)			((_v) << 16)
#define LRM23_MODE_MASK			(0x7 << 16)
#define LRM01_MODE_F(_v)			((_v) << 0)
#define LRM01_MODE_MASK			(0x7 << 0)

#define DATA_PATH_CONTROL_0				0x0214
#define WIN_MAPCOLOR_EN_F(_win)		(1 << (4*_win + 1))
#define WIN_EN_F(_win)				(1 << (4*_win + 0))

#define DATA_PATH_CONTROL_1				0x0218
#define WIN_CHMAP_F(_win, _ch)		(((_ch) & 0x7) << (4*_win))
#define WIN_CHMAP_MASK(_win)			(0x7 << (4*_win))

#define DATA_PATH_CONTROL_2				0x0230
#define COMP_OUTIF_PATH_F(_v)			((_v) << 0)
#define COMP_OUTIF_PATH_MASK			(0xff << 0)
#define COMP_OUTIF_PATH_GET(_v)			(((_v) >> 0) & 0xff)

#define DSIM0_TE_TIMEOUT_CONTROL		0x0254
#define DSIM0_TE_TIMEOUT_CNT(_v)		((_v) << 0)
#define DSIM0_TE_TIMEOUT_CNT_MASK		(0xffff << 0)
#define DSIM0_TE_TIMEOUT_CNT_GET(_v)	(((_v) >> 0) & 0xffff)

#define DSIM1_TE_TIMEOUT_CONTROL		0x0258
#define DSIM1_TE_TIMEOUT_CNT(_v)		((_v) << 0)
#define DSIM1_TE_TIMEOUT_CNT_MASK		(0xffff << 0)
#define DSIM1_TE_TIMEOUT_CNT_GET(_v)	(((_v) >> 0) & 0xffff)

#define DSIM0_START_TIME_CONTROL		0x0260
#define DSIM0_START_TIME(_v)		((_v) << 0)

/* DECON CRC for ASB */
#define CRC_DATA_0				0x0280
#define CRC_DATA_DSIMIF0_GET(_v)		(((_v) >> 0) & 0xffff)

#define CRC_CONTROL				0x028C
#define CRC_COLOR_SEL(_v)			((_v) << 16)
#define CRC_COLOR_SEL_MASK			(0x3 << 16)
#define CRC_START				(1 << 0)

#define FRAME_COUNT				0x02A0

/* BLENDER */
#define WIN_CONTROL_0(_win)			(0x1000 + ((_win) * 0x30))
#define WIN_ALPHA1_F(_v)			(((_v) & 0xFF) << 24)
#define WIN_ALPHA1_MASK			(0xFF << 24)
#define WIN_ALPHA0_F(_v)			(((_v) & 0xFF) << 16)
#define WIN_ALPHA0_MASK			(0xFF << 16)
#define WIN_ALPHA_GET(_v, _n)		(((_v) >> (16 + 8 * (_n))) & 0xFF)
#define WIN_FUNC_F(_v)				(((_v) & 0xF) << 8)
#define WIN_FUNC_MASK				(0xF << 8)
#define WIN_FUNC_GET(_v)			(((_v) >> 8) & 0xf)
#define WIN_SRESET				(1 << 4)
#define WIN_ALPHA_MULT_SRC_SEL_F(_v)		(((_v) & 0x3) << 0)
#define WIN_ALPHA_MULT_SRC_SEL_MASK		(0x3 << 0)

#define WIN_CONTROL_1(_win)			(0x1004 + ((_win) * 0x30))
#define WIN_FG_ALPHA_D_SEL_F(_v)		(((_v) & 0xF) << 24)
#define WIN_FG_ALPHA_D_SEL_MASK		(0xF << 24)
#define WIN_BG_ALPHA_D_SEL_F(_v)		(((_v) & 0xF) << 16)
#define WIN_BG_ALPHA_D_SEL_MASK		(0xF << 16)
#define WIN_FG_ALPHA_A_SEL_F(_v)		(((_v) & 0xF) << 8)
#define WIN_FG_ALPHA_A_SEL_MASK		(0xF << 8)
#define WIN_BG_ALPHA_A_SEL_F(_v)		(((_v) & 0xF) << 0)
#define WIN_BG_ALPHA_A_SEL_MASK		(0xF << 0)

#define WIN_START_POSITION(_win)		(0x1008 + ((_win) * 0x30))
#define WIN_STRPTR_Y_F(_v)			(((_v) & 0x3FFF) << 16)
#define WIN_STRPTR_X_F(_v)			(((_v) & 0x3FFF) << 0)

#define WIN_END_POSITION(_win)		(0x100C + ((_win) * 0x30))
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

#define WIN_START_TIME_CONTROL(_win)	(0x1018 + ((_win) * 0x30))
#define WIN_START_TIME_CONTROL_F(_v)	((_v) << 0)
#define WIN_START_TIME_CONTROL_MASK		(0x3fff << 0)

#define SHADOW_OFFSET					0x7000

#endif /* _REGS_DECON_H */
