/* regs-dsim.h
 *
 * Register definition file for Samsung MIPI-DSIM driver
 *
 * Copyright (c) 2015 Samsung Electronics
 * Seungbeom Park <sb1.park@samsung.com>
 * Jiun Yu <jiun.yu@samsung.com>
 * Seuni Park <seuni.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _REGS_DSIM_H
#define _REGS_DSIM_H

#define DSIM_SWRST					(0x4)
#define DSIM_DPHY_RST					(1 << 16)
#define DSIM_SWRST_FUNCRST				(1 << 8)
#define DSIM_SWRST_RESET				(1 << 0)

#define DSIM_LINK_STATUS0				(0x8)
#define DSIM_LINK_STATUS0_VIDEO_MODE_STATUS_GET(x)	((x >> 24) & 0x1)
#define DSIM_LINK_STATUS0_VM_LINE_CNT_GET(x)		((x >> 0) & 0x1fff)

#define DSIM_LINK_STATUS1				(0xc)
#define DSIM_LINK_STATUS1_CMD_MODE_STATUS_GET(x)	(x >> 26 & 0x1)

#define DSIM_LINK_STATUS3				(0x14)
#define DSIM_LINK_STATUS3_PLL_STABLE			(1 << 0)

#define DSIM_MIPI_STATUS				(0x18)
#define DSIM_MIPI_STATUS_SHADOW_REG_UP_EN		(1 << 25)
#define DSIM_MIPI_STATUS_SHADOW_REG_UP_DONE		(1 << 24)
#define DSIM_MIPI_STATUS_BUF_READ_ACK			(1 << 3)
#define DSIM_MIPI_STATUS_BUF_READ_REQ			(1 << 2)
#define DSIM_MIPI_STATUS_TE				(1 << 0)

#define DSIM_DPHY_STATUS				(0x1c)
#define DSIM_DPHY_STATUS_TX_READY_HS_CLK		(1 << 10)
#define DSIM_DPHY_STATUS_ULPS_CLK			(1 << 9)
#define DSIM_DPHY_STATUS_STOP_STATE_CLK			(1 << 8)
#define DSIM_DPHY_STATUS_ULPS_DATA_LANE_GET(x)		(((x) >> 4) & 0xf)
#define DSIM_DPHY_STATUS_ULPS_DAT(_x)			(((_x) & 0xf) << 4)
#define DSIM_DPHY_STATUS_STOP_STATE_DAT(_x)		(((_x) & 0xf) << 0)

#define DSIM_CLK_CTRL					(0x20)
#define DSIM_CLK_CTRL_CLOCK_SEL			(1 << 26)
#define DSIM_CLK_CTRL_NONCONT_CLOCK_LANE		(1 << 25)
#define DSIM_CLK_CTRL_CLKLANE_ONOFF			(1 << 24)
#define DSIM_CLK_CTRL_TX_REQUEST_HSCLK			(1 << 20)
#define DSIM_CLK_CTRL_WORDCLK_EN			(1 << 17)
#define DSIM_CLK_CTRL_ESCCLK_EN				(1 << 16)
#define DSIM_CLK_CTRL_LANE_ESCCLK_EN(_x)		((_x) << 8)
#define DSIM_CLK_CTRL_LANE_ESCCLK_EN_MASK		(0x1f << 8)
#define DSIM_CLK_CTRL_ESC_PRESCALER(_x)			((_x) << 0)
#define DSIM_CLK_CTRL_ESC_PRESCALER_MASK		(0xff << 0)

#define DSIM_DESKEW_CTRL				(0x24)
#define DSIM_DESKEW_CTRL_HW_EN				(1 << 15)
#define DSIM_DESKEW_CTRL_HW_POSITION			(1 << 14)
#define DSIM_DESKEW_CTRL_HW_INTERVAL(_x)		((_x) << 2)
#define DSIM_DESKEW_CTRL_HW_INTERVAL_MASK		(0xfff << 2)
#define DSIM_DESKEW_CTRL_HW_INIT			(1 << 1)
#define DSIM_DESKEW_CTRL_SW_SEND			(1 << 0)

/* Time out register */
#define DSIM_TIMEOUT					(0x28)
#define DSIM_TIMEOUT_BTA_TOUT(_x)			((_x) << 16)
#define DSIM_TIMEOUT_BTA_TOUT_MASK			(0xffff << 16)
#define DSIM_TIMEOUT_LPDR_TOUT(_x)			((_x) << 0)
#define DSIM_TIMEOUT_LPDR_TOUT_MASK			(0xffff << 0)

/* Escape mode register */
#define DSIM_ESCMODE					(0x2c)
#define DSIM_ESCMODE_STOP_STATE_CNT(_x)			((_x) << 21)
#define DSIM_ESCMODE_STOP_STATE_CNT_MASK		(0x7ff << 21)
#define DSIM_ESCMODE_FORCE_STOP_STATE			(1 << 20)
#define DSIM_ESCMODE_FORCE_BTA				(1 << 16)
#define DSIM_ESCMODE_CMD_LPDT				(1 << 7)
#define DSIM_ESCMODE_TRIGGER_RST			(1 << 4)
#define DSIM_ESCMODE_TX_ULPS_DATA			(1 << 3)
#define DSIM_ESCMODE_TX_ULPS_DATA_EXIT			(1 << 2)
#define DSIM_ESCMODE_TX_ULPS_CLK			(1 << 1)
#define DSIM_ESCMODE_TX_ULPS_CLK_EXIT			(1 << 0)

#define DSIM_NUM_OF_TRANSFER				(0x30)
#define DSIM_NUM_OF_TRANSFER_PER_FRAME(_x)		((_x) << 0)
#define DSIM_NUM_OF_TRANSFER_PER_FRAME_MASK		(0xfffff << 0)
#define DSIM_NUM_OF_TRANSFER_PER_FRAME_GET(x)		(((x) >> 0) & 0xfffff)

#define DSIM_UNDERRUN_CTRL				(0x34)
#define DSIM_UNDERRUN_CTRL_CM_UNDERRUN_LP_REF(_x)	((_x) << 0)
#define DSIM_UNDERRUN_CTRL_CM_UNDERRUN_LP_REF_MASK	(0xffff << 0)

#define DSIM_THRESHOLD					(0x38)
#define DSIM_THRESHOLD_LEVEL(_x)			((_x) << 0)
#define DSIM_THRESHOLD_LEVEL_MASK			(0xffff << 0)

/* Display image resolution register */
#define DSIM_RESOL					(0x3c)
#define DSIM_RESOL_VRESOL(x)				(((x) & 0x1fff) << 16)
#define DSIM_RESOL_VRESOL_MASK				(0x1fff << 16)
#define DSIM_RESOL_HRESOL(x)				(((x) & 0x1fff) << 0)
#define DSIM_RESOL_HRESOL_MASK				(0x1fff << 0)
#define DSIM_RESOL_LINEVAL_GET(_v)			(((_v) >> 16) & 0x1fff)
#define DSIM_RESOL_HOZVAL_GET(_v)			(((_v) >> 0) & 0x1fff)

/* Main display Vporch register */
#define DSIM_VPORCH					(0x40)
#define DSIM_VPORCH_VFP_CMD_ALLOW(_x)			((_x) << 24)
#define DSIM_VPORCH_VFP_CMD_ALLOW_MASK			(0xff << 24)
#define DSIM_VPORCH_STABLE_VFP(_x)			((_x) << 16)
#define DSIM_VPORCH_STABLE_VFP_MASK			(0xff << 16)
#define DSIM_VPORCH_VFP_TOTAL(_x)			((_x) << 8)
#define DSIM_VPORCH_VFP_TOTAL_MASK			(0xff << 8)
#define DSIM_VPORCH_VBP(_x)				((_x) << 0)
#define DSIM_VPORCH_VBP_MASK				(0xff << 0)

/* Main display Hporch register */
#define DSIM_HPORCH					(0x44)
#define DSIM_HPORCH_HFP(_x)				((_x) << 16)
#define DSIM_HPORCH_HFP_MASK				(0xffff << 16)
#define DSIM_HPORCH_HBP(_x)				((_x) << 0)
#define DSIM_HPORCH_HBP_MASK				(0xffff << 0)

/* Main display sync area register */
#define DSIM_SYNC					(0x48)
#define DSIM_SYNC_VSA(_x)				((_x) << 16)
#define DSIM_SYNC_VSA_MASK				(0xff << 16)
#define DSIM_SYNC_HSA(_x)				((_x) << 0)
#define DSIM_SYNC_HSA_MASK				(0xffff << 0)

/* Configuration register */
#define DSIM_CONFIG					(0x4c)
#define DSIM_CONFIG_SYNC_INFORM				(1 << 27)
#define DSIM_CONFIG_BURST_MODE				(1 << 26)
#define DSIM_CONFIG_LP_FORCE_EN				(1 << 24)
#define DSIM_CONFIG_HSE_DISABLE				(1 << 23)
#define DSIM_CONFIG_HFP_DISABLE				(1 << 22)
#define DSIM_CONFIG_HBP_DISABLE				(1 << 21)
#define DSIM_CONFIG_HSA_DISABLE				(1 << 20)
#define DSIM_CONFIG_CPRS_EN				(1 << 19)
#define DSIM_CONFIG_VIDEO_MODE				(1 << 18)
#define DSIM_CONFIG_VC_ID(_x)				((_x) << 15)
#define DSIM_CONFIG_VC_ID_MASK				(0x3 << 15)
#define DSIM_CONFIG_PIXEL_FORMAT(_x)			((_x) << 9)
#define DSIM_CONFIG_PIXEL_FORMAT_MASK			(0x3f << 9)
#define DSIM_CONFIG_PER_FRAME_READ_EN			(1 << 8)
#define DSIM_CONFIG_EOTP_EN				(1 << 7)
#define DSIM_CONFIG_NUM_OF_DATA_LANE(_x)		((_x) << 5)
#define DSIM_CONFIG_NUM_OF_DATA_LANE_MASK		(0x3 << 5)
#define DSIM_CONFIG_LANES_EN(_x)			(((_x) & 0x1f) << 0)

/* Interrupt source register */
#define DSIM_INTSRC					(0x50)
#define DSIM_INTSRC_PLL_STABLE				(1 << 31)
#define DSIM_INTSRC_SW_RST_RELEASE			(1 << 30)
#define DSIM_INTSRC_SFR_PL_FIFO_EMPTY			(1 << 29)
#define DSIM_INTSRC_SFR_PH_FIFO_EMPTY			(1 << 28)
#define DSIM_INTSRC_SFR_PH_FIFO_OVERFLOW		(1 << 27)
#define DSIM_INTSRC_SW_DESKEW_DONE			(1 << 26)
#define DSIM_INTSRC_BUS_TURN_OVER			(1 << 25)
#define DSIM_INTSRC_FRAME_DONE				(1 << 24)
#define DSIM_INTSRC_INVALID_SFR_VALUE			(1 << 23)
#define DSIM_INTSRC_ABNRMAL_CMD_ST			(1 << 22)
#define DSIM_INTSRC_LPRX_TOUT				(1 << 21)
#define DSIM_INTSRC_BTA_TOUT				(1 << 20)
#define DSIM_INTSRC_UNDER_RUN				(1 << 19)
#define DSIM_INTSRC_RX_DATA_DONE			(1 << 18)
#define DSIM_INTSRC_ERR_RX_ECC				(1 << 15)
#define DSIM_INTSRC_VT_STATUS				(1 << 13)

/* Interrupt mask register */
#define DSIM_INTMSK					(0x54)
#define DSIM_INTMSK_PLL_STABLE				(1 << 31)
#define DSIM_INTMSK_SW_RST_RELEASE			(1 << 30)
#define DSIM_INTMSK_SFR_PL_FIFO_EMPTY			(1 << 29)
#define DSIM_INTMSK_SFR_PH_FIFO_EMPTY			(1 << 28)
#define DSIM_INTMSK_SFR_PH_FIFO_OVERFLOW		(1 << 27)
#define DSIM_INTMSK_SW_DESKEW_DONE			(1 << 26)
#define DSIM_INTMSK_BUS_TURN_OVER			(1 << 25)
#define DSIM_INTMSK_FRAME_DONE				(1 << 24)
#define DSIM_INTMSK_INVALID_SFR_VALUE			(1 << 23)
#define DSIM_INTMSK_ABNRMAL_CMD_ST			(1 << 22)
#define DSIM_INTMSK_LPRX_TOUT				(1 << 21)
#define DSIM_INTMSK_BTA_TOUT				(1 << 20)
#define DSIM_INTMSK_UNDER_RUN				(1 << 19)
#define DSIM_INTMSK_RX_DATA_DONE			(1 << 18)
#define DSIM_INTMSK_ERR_RX_ECC				(1 << 15)
#define DSIM_INTMSK_VT_STATUS				(1 << 13)

/* Packet Header FIFO register */
#define DSIM_PKTHDR					(0x58)
#define DSIM_PKTHDR_BTA_TYPE(_x)			((_x) << 24)
#define DSIM_PKTHDR_DATA1(_x)				((_x) << 16)
#define DSIM_PKTHDR_DATA0(_x)				((_x) << 8)
#define DSIM_PKTHDR_ID(_x)				((_x) << 0)
#define DSIM_PKTHDR_DATA				(0x1ffffff << 0)

/* Payload FIFO register */
#define DSIM_PAYLOAD					(0x5c)

/* Read FIFO register */
#define DSIM_RXFIFO					(0x60)

/* SFR control Register for Stanby & Shadow*/
#define DSIM_SFR_CTRL					(0x64)
#define DSIM_SFR_CTRL_SHADOW_REG_READ_EN		(1 << 1)
#define DSIM_SFR_CTRL_SHADOW_EN				(1 << 0)

/* FIFO status and control register */
#define DSIM_FIFOCTRL					(0x68)
#define DSIM_FIFOCTRL_NUMBER_OF_PH_SFR(_x)		(((_x) & 0x3f) << 16)
#define DSIM_FIFOCTRL_NUMBER_OF_PH_SFR_GET(x)		(((x) >> 16) & 0x3f)
#define DSIM_FIFOCTRL_EMPTY_RX				(1 << 12)
#define DSIM_FIFOCTRL_FULL_PH_SFR			(1 << 11)
#define DSIM_FIFOCTRL_EMPTY_PH_SFR			(1 << 10)
#define DSIM_FIFOCTRL_FULL_PL_SFR			(1 << 9)
#define DSIM_FIFOCTRL_EMPTY_PL_SFR			(1 << 8)
#define DSIM_FIFOCTRL_INIT_RX				(1 << 2)
#define DSIM_FIFOCTRL_INIT_PL_SFR			(1 << 1)
#define DSIM_FIFOCTRL_INIT_PH_SFR			(1 << 0)

#define DSIM_LP_SCATTER					(0x6c)
#define DSIM_LP_SCATTER_PATTERN(_x)			((_x) << 16)
#define DSIM_LP_SCATTER_PATTERN_MASK			(0xffff << 16)
#define DSIM_LP_SCATTER_EN				(1 << 0)

/* Muli slice setting register*/
#define DSIM_CPRS_CTRL					(0x74)
#define DSIM_CPRS_CTRL_MULI_SLICE_PACKET		(1 << 3)
#define DSIM_CPRS_CTRL_NUM_OF_SLICE(_x)			((_x) << 0)
#define DSIM_CPRS_CTRL_NUM_OF_SLICE_MASK		(0x7 << 0)
#define DSIM_CPRS_CTRL_NUM_OF_SLICE_GET(x)		(((x) >> 0) & 0x7)

/*Slice01 size register*/
#define DSIM_SLICE01					(0x78)
#define DSIM_SLICE01_SIZE_OF_SLICE1(_x)			((_x) << 16)
#define DSIM_SLICE01_SIZE_OF_SLICE1_MASK		(0x1fff << 16)
#define DSIM_SLICE01_SIZE_OF_SLICE1_GET(x)		(((x) >> 16) & 0x1fff)
#define DSIM_SLICE01_SIZE_OF_SLICE0(_x)			((_x) << 0)
#define DSIM_SLICE01_SIZE_OF_SLICE0_MASK		(0x1fff << 0)
#define DSIM_SLICE01_SIZE_OF_SLICE0_GET(x)		(((x) >> 0) & 0x1fff)

/*Slice23 size register*/
#define DSIM_SLICE23					(0x7c)
#define DSIM_SLICE23_SIZE_OF_SLICE3(_x)			((_x) << 16)
#define DSIM_SLICE23_SIZE_OF_SLICE3_MASK		(0x1fff << 16)
#define DSIM_SLICE23_SIZE_OF_SLICE3_GET(x)		(((x) >> 16) & 0x1fff)
#define DSIM_SLICE23_SIZE_OF_SLICE2(_x)			((_x) << 0)
#define DSIM_SLICE23_SIZE_OF_SLICE2_MASK		(0x1fff << 0)
#define DSIM_SLICE23_SIZE_OF_SLICE2_GET(x)		(((x) >> 0) & 0x1fff)

/* Command configuration register */
#define DSIM_CMD_CONFIG					(0x80)
#define DSIM_CMD_CONFIG_PKT_GO_RDY			(1 << 17)
#define DSIM_CMD_CONFIG_PKT_GO_EN			(1 << 16)
#define DSIM_CMD_CONFIG_MULTI_CMD_PKT_EN		(1 << 8)
#define DSIM_CMD_CONFIG_MULTI_PKT_CNT(_x)		((_x) << 0)
#define DSIM_CMD_CONFIG_MULTI_PKT_CNT_MASK		(0x3f << 0)

/* TE based command register*/
#define DSIM_CMD_TE_CTRL0				(0x84)
#define DSIM_CMD_TE_CTRL0_TIME_STABLE_VFP(_x)		((_x) << 0)
#define DSIM_CMD_TE_CTRL0_TIME_STABLE_VFP_MASK		(0xffff << 0)
//#define DSIM_CMD_TE_CTRL0_TIME_VSYNC_TOUT(_x)	((_x) << 0)
//#define DSIM_CMD_TE_CTRL0_TIME_VSYNC_TOUT_MASK	(0xffff << 0)

/* TE based command register*/
#define DSIM_CMD_TE_CTRL1				(0x88)
#define DSIM_CMD_TE_CTRL1_TIME_TE_PROTECT_ON(_x)	((_x) << 16)
#define DSIM_CMD_TE_CTRL1_TIME_TE_PROTECT_ON_MASK	(0xffff << 16)
#define DSIM_CMD_TE_CTRL1_TIME_TE_TOUT(_x)		((_x) << 0)
#define DSIM_CMD_TE_CTRL1_TIME_TE_TOUT_MASK		(0xffff << 0)

/*Command Mode Status register*/
#define DSIM_CMD_STATUS					(0x8c)
#define	DSIM_CMD_STATUS_ABNORMAL_CAUSE_ST_GET(x)	(((x) >> 0) & 0xff)

#define DSIM_VIDEO_TIMER				(0x90)
#define DSIM_VIDEO_TIMER_COMPENSATE(_x)			((_x) << 8)
#define DSIM_VIDEO_TIMER_COMPENSATE_MASK		(0xffffff << 8)
#define DSIM_VIDEO_TIMER_VSTATUS_INTR_SEL(_x)		((_x) << 1)
#define DSIM_VIDEO_TIMER_VSTATUS_INTR_SEL_MASK		(0x3 << 1)
#define DSIM_VIDEO_TIMER_SYNC_MODE			(1 << 0)

/*BIST generation register*/
#define	DSIM_BIST_CTRL0					(0x94)
#define	DSIM_BIST_CTRL0_BIST_PTRN_MOVE_EN		(1 << 4)
#define	DSIM_BIST_CTRL0_BIST_PTRN_MODE(_x)		((_x) << 1)
#define	DSIM_BIST_CTRL0_BIST_PTRN_MODE_MASK		(0x7 << 1)
#define	DSIM_BIST_CTRL0_BIST_EN				(1 << 0)

/*BIST generation register*/
#define	DSIM_BIST_CTRL1					(0x98)
#define	DSIM_BIST_CTRL1_BIST_PTRN_PRBS7_SEED(_x)	((_x) << 24)
#define	DSIM_BIST_CTRL1_BIST_PTRN_PRBS7_SEED_MASK	(0x7f << 24)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_R(_x)		((_x) << 16)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_R_MASK		(0XFF << 16)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_G(_x)		((_x) << 8)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_G_MASK		(0xFF << 8)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_B(_x)		((_x) << 0)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_B_MASK		(0xFF << 0)

/*DSIM to CSI loopback register*/
#define	DSIM_CSIS_LB					(0x9C)
#define DSIM_CSIS_LB_1BYTEPPI_MODE			(1 << 9)
#define	DSIM_CSIS_LB_CSIS_LB_EN				(1 << 8)
#define DSIM_CSIS_LB_CSIS_PH(_x)			((_x) << 0)
#define DSIM_CSIS_LB_CSIS_PH_MASK			(0xff << 0)

/* PLL control register */
#define DSIM_PLLCTRL					(0xa0)
#define DSIM_PLLCTRL_PLL_EN				(1 << 23)

/* PLL timer register */
#define DSIM_PLLTMR					(0xac)

/*
 * DPHY  registers
 */

/* DPHY BIAS setting */
#define DSIM_PHY_BIAS_CTRL(_id)		(0x0400 + (4 * (_id)))

/* DPHY DTB setting */
#define DSIM_PHY_DTB_CTRL(_id)		(0x0800 + (4 * (_id)))

/* PMSK setting */
#define DSIM_PHY_PLL_CTRL_01		(0x0C04)
#define DSIM_PHY_PLL_CTRL_02		(0x0C08)
#define DSIM_PHY_PLL_CTRL_03		(0x0C0C)
#define DSIM_PHY_PLL_CTRL_04		(0x0C10)
#define DSIM_PHY_PLL_CTRL_05		(0x0C14)
#define DSIM_PHY_PLL_CTRL_06		(0x0C18)
#define DSIM_PHY_PLL_CTRL_07		(0x0C1C)
#define DSIM_PHY_PLL_CTRL_08		(0x0C20)
#define DSIM_PHY_PLL_CTRL_09		(0x0C24)
#define DSIM_PHY_PLL_CTRL_0A		(0x0C28)
#define DSIM_PHY_PLL_CTRL_0B		(0x0C2C)
#define DSIM_PHY_PLL_CTRL_0C		(0x0C30)
#define DSIM_PHY_PLL_CTRL_0D		(0x0C34)

#define DSIM_PHY_PMS_K_15_8(_x)		(((_x) & 0xff00) >> 8)
#define DSIM_PHY_PMS_K_15_8_MASK	(0xff << 0)

#define DSIM_PHY_PMS_K_7_0(_x)		((_x) & 0xff)
#define DSIM_PHY_PMS_K_7_0_MASK		(0xff << 0)

#define DSIM_PHY_PMS_P_5_0(_x)		(((_x) & 0x3f) << 2)
#define DSIM_PHY_PMS_P_5_0_MASK		(0x3f << 2)

#define DSIM_PHY_PMS_M_9_8(_x)		(((_x) & 0x300) >> 8)
#define DSIM_PHY_PMS_M_9_8_MASK		(0x3 << 0)

#define DSIM_PHY_PMS_M_7_0(_x)		((_x) & 0xff)
#define DSIM_PHY_PMS_M_7_0_MASK		(0xff << 0)

#define DSIM_PHY_PMS_S_2_0(_x)		(((_x) & 0x7) << 5)
#define DSIM_PHY_PMS_S_2_0_MASK		(0x7 << 5)

#if defined(CONFIG_EXYNOS_DSIM_DITHER)
#define DSIM_PHY_DITHER_EN			(0x1 << 1)

#define DSIM_PHY_DITHER_MFR(_x)		(((_x) & 0xff))
#define DSIM_PHY_DITHER_MFR_MASK	(0xff << 0)

#define DSIM_PHY_DITHER_MRR(_x)		(((_x) & 0x3f) << 2)
#define DSIM_PHY_DITHER_MRR_MASK	(0x3f << 2)

#define DSIM_PHY_DITHER_SEL_PF(_x)	(((_x) & 0x3) << 0)
#define DSIM_PHY_DITHER_SEL_PF_MASK	(0x3 << 0)

#define DSIM_PHY_DITHER_ICP(_x)		(((_x) & 0x3) << 6)
#define DSIM_PHY_DITHER_ICP_MASK	(0x3 << 6)

#define DSIM_PHY_DITHER_AFC_ENB		(0x1 << 3)

#define DSIM_PHY_DITHER_EXTAFC(_x)	(((_x) & 0x1f) << 0)
#define DSIM_PHY_DITHER_EXTAFC_MASK	(0x1f << 0)

#define DSIM_PHY_DITHER_FEED_EN		(0x1 << 4)

#define DSIM_PHY_DITHER_FSEL		(0x1 << 4)

#define DSIM_PHY_DITHER_FOUT_MASK	(0x1 << 5)

#define DSIM_PHY_DITHER_RSEL(_x)	(((_x) & 0xf) << 0)
#define DSIM_PHY_DITHER_RSEL_MASK	(0xf << 0)
#endif

/* master clock lane  setting */
#define DSIM_PHY_ACTRL_MC(_id)		(0x1000 + (4 * (_id)))

#define DSIM_PHY_DCTRL_MC_01		(0x1080)
#define DSIM_PHY_DCTRL_MC_02		(0x1084)
#define DSIM_PHY_DCTRL_MC_03		(0x1088)
#define DSIM_PHY_DCTRL_MC_04		(0x108C)
#define DSIM_PHY_DCTRL_MC_05		(0x1090)
#define DSIM_PHY_DCTRL_MC_06		(0x1094)
#define DSIM_PHY_DCTRL_MC_07		(0x1098)
#define DSIM_PHY_DCTRL_MC_08		(0x109C)
#define DSIM_PHY_DCTRL_MC_09		(0x10A0)
#define DSIM_PHY_DCTRL_MC_0A		(0x10A4)
#define DSIM_PHY_DCTRL_MC_0B		(0x10A8)
#define DSIM_PHY_DCTRL_MC_0C		(0x10AC)
#define DSIM_PHY_DCTRL_MC_0D		(0x10B0)
#define DSIM_PHY_DCTRL_MC_0E		(0x10B4)

/* master data lane  setting : D0 */
#define DSIM_PHY_ACTRL_MD0(_id)		(0x1400 + (4 * (_id)))

#define DSIM_PHY_DCTRL_MD0_01		(0x1480)
#define DSIM_PHY_DCTRL_MD0_02		(0x1484)
#define DSIM_PHY_DCTRL_MD0_03		(0x1488)
#define DSIM_PHY_DCTRL_MD0_04		(0x148C)
#define DSIM_PHY_DCTRL_MD0_05		(0x1490)
#define DSIM_PHY_DCTRL_MD0_06		(0x1494)
#define DSIM_PHY_DCTRL_MD0_07		(0x1498)
#define DSIM_PHY_DCTRL_MD0_08		(0x149C)
#define DSIM_PHY_DCTRL_MD0_09		(0x14A0)
#define DSIM_PHY_DCTRL_MD0_0A		(0x14A4)
#define DSIM_PHY_DCTRL_MD0_0B		(0x14A8)
#define DSIM_PHY_DCTRL_MD0_0C		(0x14AC)

/* master data lane  setting : D1 */
#define DSIM_PHY_ACTRL_MD1(_id)		(0x1800 + (4 * (_id)))

#define DSIM_PHY_DCTRL_MD1_01		(0x1880)
#define DSIM_PHY_DCTRL_MD1_02		(0x1884)
#define DSIM_PHY_DCTRL_MD1_03		(0x1888)
#define DSIM_PHY_DCTRL_MD1_04		(0x188C)
#define DSIM_PHY_DCTRL_MD1_05		(0x1890)
#define DSIM_PHY_DCTRL_MD1_06		(0x1894)
#define DSIM_PHY_DCTRL_MD1_07		(0x1898)
#define DSIM_PHY_DCTRL_MD1_08		(0x189C)
#define DSIM_PHY_DCTRL_MD1_09		(0x18A0)
#define DSIM_PHY_DCTRL_MD1_0A		(0x18A4)
#define DSIM_PHY_DCTRL_MD1_0B		(0x18A8)
#define DSIM_PHY_DCTRL_MD1_0C		(0x18AC)

/* master data lane  setting : D2 */
#define DSIM_PHY_ACTRL_MD2(_id)		(0x1C00 + (4 * (_id)))

#define DSIM_PHY_DCTRL_MD2_01		(0x1C80)
#define DSIM_PHY_DCTRL_MD2_02		(0x1C84)
#define DSIM_PHY_DCTRL_MD2_03		(0x1C88)
#define DSIM_PHY_DCTRL_MD2_04		(0x1C8C)
#define DSIM_PHY_DCTRL_MD2_05		(0x1C90)
#define DSIM_PHY_DCTRL_MD2_06		(0x1C94)
#define DSIM_PHY_DCTRL_MD2_07		(0x1C98)
#define DSIM_PHY_DCTRL_MD2_08		(0x1C9C)
#define DSIM_PHY_DCTRL_MD2_09		(0x1CA0)
#define DSIM_PHY_DCTRL_MD2_0A		(0x1CA4)
#define DSIM_PHY_DCTRL_MD2_0B		(0x1CA8)
#define DSIM_PHY_DCTRL_MD2_0C		(0x1CAC)

/* master data lane  setting : D3 */
#define DSIM_PHY_ACTRL_MD3(_id)		(0x2000 + (4 * (_id)))

#define DSIM_PHY_DCTRL_MD3_01		(0x2080)
#define DSIM_PHY_DCTRL_MD3_02		(0x2084)
#define DSIM_PHY_DCTRL_MD3_03		(0x2088)
#define DSIM_PHY_DCTRL_MD3_04		(0x208C)
#define DSIM_PHY_DCTRL_MD3_05		(0x2090)
#define DSIM_PHY_DCTRL_MD3_06		(0x2094)
#define DSIM_PHY_DCTRL_MD3_07		(0x2098)
#define DSIM_PHY_DCTRL_MD3_08		(0x209C)
#define DSIM_PHY_DCTRL_MD3_09		(0x20A0)
#define DSIM_PHY_DCTRL_MD3_0A		(0x20A4)
#define DSIM_PHY_DCTRL_MD3_0B		(0x20A8)
#define DSIM_PHY_DCTRL_MD3_0C		(0x20AC)

/* macros for DPHY timing controls */
#define DSIM_PHY_ULPS_EXIT_CNT_7_0(_x)	((_x) << 0)
#define DSIM_PHY_ULPS_EXIT_CNT_7_0_MASK	(0xff << 0)

#define DSIM_PHY_ULPS_EXIT_CNT_9_8(_x)	(((_x) & 0x300) >> 5)
#define DSIM_PHY_ULPS_EXIT_CNT_9_8_MASK	(0x3 << 3)

#define DSIM_PHY_HS_MODE_SEL			(0x1 << 2)

#define DSIM_PHY_SKEWCAL_EN				(0x1 << 4)

#define DSIM_PHY_SWKECAL_INIT_WAIT_TIME(_x)		((_x) << 0)
#define DSIM_PHY_SWKECAL_INIT_WAIT_TIME_MASK	(0xf << 0)

#define DSIM_PHY_SWKECAL_INIT_RUN_TIME(_x)		(((_x) & 0xf) << 4)
#define DSIM_PHY_SWKECAL_INIT_RUN_TIME_MASK		(0xf << 4)

#define DSIM_PHY_SWKECAL_RUN_TIME(_x)		((_x) << 0)
#define DSIM_PHY_SWKECAL_RUN_TIME_MASK		(0xf << 0)

#define DSIM_PHY_TLPXCTRL(_x)			((_x) << 0)
#define DSIM_PHY_TLPXCTRL_MASK			(0xff << 0)

#define DSIM_PHY_THSEXITCTL(_x)			((_x) << 0)
#define DSIM_PHY_THSEXITCTL_MASK		(0xff << 0)

#define DSIM_PHY_TCLKPRPRCTL(_x)		((_x) << 0)
#define DSIM_PHY_TCLKPRPRCTL_MASK		(0xff << 0)

#define DSIM_PHY_TCLKZEROCTL(_x)		((_x) << 0)
#define DSIM_PHY_TCLKZEROCTL_MASK		(0xff << 0)

#define DSIM_PHY_TCLKPOSTCTL(_x)		((_x) << 0)
#define DSIM_PHY_TCLKPOSTCTL_MASK		(0xff << 0)

#define DSIM_PHY_TCLKTRAILCTL(_x)		((_x) << 0)
#define DSIM_PHY_TCLKTRAILCTL_MASK		(0xff << 0)

#define DSIM_PHY_THSPRPRCTL(_x)			((_x) << 0)
#define DSIM_PHY_THSPRPRCTL_MASK		(0xff << 0)

#define DSIM_PHY_THSZEROCTL(_x)			((_x) << 0)
#define DSIM_PHY_THSZEROCTL_MASK		(0xff << 0)

#define DSIM_PHY_TTRAILCTL(_x)			((_x) << 0)
#define DSIM_PHY_TTRAILCTL_MASK			(0xff << 0)

#endif /* _REGS_DSIM_H */
