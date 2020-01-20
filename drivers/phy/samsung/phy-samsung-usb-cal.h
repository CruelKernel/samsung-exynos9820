/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Author: Sung-Hyun Na <sunghyun.na@samsung.com>
 *
 * USBPHY configuration definitions for Samsung USB PHY CAL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __PHY_SAMSUNG_USB_FW_CAL_H__
#define __PHY_SAMSUNG_USB_FW_CAL_H__

#define EXYNOS_USBCON_VER_01_0_0	0x0100	/* Istor	*/
#define EXYNOS_USBCON_VER_01_0_1	0x0101	/* JF 3.0	*/
#define EXYNOS_USBCON_VER_01_1_1	0x0111	/* KC		*/
#define EXYNOS_USBCON_VER_01_MAX	0x01FF

#define EXYNOS_USBCON_VER_02_0_0	0x0200	/* Insel-D, Island	*/
#define EXYNOS_USBCON_VER_02_0_1	0x0201	/* JF EVT0 2.0 Host	*/
#define EXYNOS_USBCON_VER_02_1_0	0x0210
#define EXYNOS_USBCON_VER_02_1_1	0x0211	/* JF EVT1 2.0 Host	*/
#define EXYNOS_USBCON_VER_02_1_2	0x0212  /* Katmai EVT0 */
#define EXYNOS_USBCON_VER_02_MAX	0x02FF

#define EXYNOS_USBCON_VER_03_0_0	0x0300	/* Lhotse, Lassen HS */
#define EXYNOS_USBCON_VER_03_0_1	0x0301	/* MK */
#define EXYNOS_USBCON_VER_03_MAX	0x03FF

#define EXYNOS_USBCON_VER_04_0_0	0x0400	/* Lhotse - USB/DP  */
#define EXYNOS_USBCON_VER_04_MAX	0x04FF

/* Sub phy control - not include System/Link control */
#define EXYNOS_USBCON_VER_05_0_0	0x0500	/* High Speed Only	*/
#define EXYNOS_USBCON_VER_05_1_0	0x0510	/* Super Speed		*/
#define EXYNOS_USBCON_VER_05_3_0	0x0530	/* Super Speed Dual PHY	*/
#define EXYNOS_USBCON_VER_05_MAX	0x05FF

#define EXYNOS_USBCON_VER_F2_0_0	0xF200
#define EXYNOS_USBCON_VER_F2_MAX	0xF2FF

#define EXYNOS_USBCON_VER_MAJOR_VER_MASK	0xFF00
#define EXYNOS_USBCON_VER_SS_CAP			0x0010

#define EXYNOS_USBCON_VER_MINOR(_x)	((_x) & 0xf)
#define EXYNOS_USBCON_VER_MID(_x)	((_x) & 0xf0)
#define EXYNOS_USBCON_VER_MAJOR(_x)	((_x) & 0xff00)

enum exynos_usbphy_mode {
	USBPHY_MODE_DEV = 0,
	USBPHY_MODE_HOST = 1,

	/* usb phy for uart bypass mode */
	USBPHY_MODE_BYPASS = 0x10,
};

enum exynos_usbphy_refclk {
	USBPHY_REFCLK_DIFF_100MHZ = 0x80 | 0x27,
	USBPHY_REFCLK_DIFF_52MHZ = 0x80 | 0x02 | 0x40,
	USBPHY_REFCLK_DIFF_26MHZ = 0x80 | 0x02,
	USBPHY_REFCLK_DIFF_24MHZ = 0x80 | 0x2a,
	USBPHY_REFCLK_DIFF_20MHZ = 0x80 | 0x31,
	USBPHY_REFCLK_DIFF_19_2MHZ = 0x80 | 0x38,

	USBPHY_REFCLK_EXT_50MHZ = 0x07,
	USBPHY_REFCLK_EXT_26MHZ = 0x06,
	USBPHY_REFCLK_EXT_24MHZ = 0x05,
	USBPHY_REFCLK_EXT_20MHZ = 0x04,
	USBPHY_REFCLK_EXT_12MHZ = 0x02,
};

enum exynos_usbphy_refsel {
	USBPHY_REFSEL_CLKCORE = 0x2,
	USBPHY_REFSEL_EXT_OSC = 0x1,
	USBPHY_REFSEL_EXT_XTAL = 0x0,

	USBPHY_REFSEL_DIFF_PAD = 0x6,
	USBPHY_REFSEL_DIFF_INTERNAL = 0x4,
	USBPHY_REFSEL_DIFF_SINGLE = 0x3,
};

enum exynos_usbphy_utmi {
	USBPHY_UTMI_FREECLOCK, USBPHY_UTMI_PHYCLOCK,
};

enum exynos_usbphy_tune_para {
	USBPHY_TUNE_HS_COMPDIS = 0x0,
	USBPHY_TUNE_HS_OTG = 0x1,
	USBPHY_TUNE_HS_SQRX = 0x2,
	USBPHY_TUNE_HS_TXFSLS = 0x3,
	USBPHY_TUNE_HS_TXHSXV = 0x4,
	USBPHY_TUNE_HS_TXPREEMP = 0x5,
	USBPHY_TUNE_HS_TXPREEMP_PLUS = 0x6,
	USBPHY_TUNE_HS_TXRES = 0x7,
	USBPHY_TUNE_HS_TXRISE = 0x8,
	USBPHY_TUNE_HS_TXVREF = 0x9,

	USBPHY_TUNE_SS_TX_BOOST = 0x0 | 0x10000,
	USBPHY_TUNE_SS_TX_SWING = 0x1 | 0x10000,
	USBPHY_TUNE_SS_TX_DEEMPHASIS = 0x2 | 0x10000,
	USBPHY_TUNE_SS_LOS_BIAS = 0x3 | 0x10000,
	USBPHY_TUNE_SS_LOS_MASK_VAL = 0x4 | 0x10000,
	USBPHY_TUNE_SS_FIX_EQ = 0x5 | 0x10000,
	USBPHY_TUNE_SS_RX_EQ = 0x6 | 0x10000,

	USBPHY_TUNE_COMBO = 0x20000,
	USBPHY_TUNE_COMBO_TX_AMP	= USBPHY_TUNE_COMBO | 0x0,
	USBPHY_TUNE_COMBO_TX_EMPHASIS	= USBPHY_TUNE_COMBO | 0x1,
	USBPHY_TUNE_COMBO_TX_IDRV	= USBPHY_TUNE_COMBO | 0x2,
	USBPHY_TUNE_COMBO_TX_ACCDRV	= USBPHY_TUNE_COMBO | 0x3,
};

enum exynos_usb_bc {
	BC_NO_CHARGER,
	BC_SDP,
	BC_DCP,
	BC_CDP,
	BC_ACA_DOCK,
	BC_ACA_A,
	BC_ACA_B,
	BC_ACA_C,
};

struct exynos_usb_tune_param {
	char name[30];
	int value;
};

#define EXYNOS_USB_TUNE_LAST	0x4C415354

/* HS PHY tune parameter */
struct exynos_usbphy_hs_tune {
	u8 tx_vref;
	u8 tx_pre_emp;
	u8 tx_pre_emp_puls;
	u8 tx_res;
	u8 tx_rise;
	u8 tx_hsxv;
	u8 tx_fsls;
	u8 rx_sqrx;
	u8 compdis;
	u8 otg;
	bool enable_user_imp;
	u8 user_imp_value;
	enum exynos_usbphy_utmi utmi_clk;
};

/* SS PHY tune parameter */
struct exynos_usbphy_ss_tune {
	/* TX Swing Level*/
	u8 tx_boost_level;
	u8 tx_swing_level;
	u8 tx_swing_full;
	u8 tx_swing_low;
	/* TX De-Emphasis */
	u8 tx_deemphasis_mode;
	u8 tx_deemphasis_3p5db;
	u8 tx_deemphasis_6db;
	/* SSC Operation*/
	u8 enable_ssc;
	u8 ssc_range;
	/* Loss-of-Signal detector threshold level */
	u8 los_bias;
	/* Loss-of-Signal mask width */
	u16 los_mask_val;
	/* RX equalizer mode */
	u8 enable_fixed_rxeq_mode;
	u8 fix_rxeq_value;
	/* Decrease TX Impedance */
	u8 decrease_ss_tx_imp;

	u8 set_crport_level_en;
	u8 set_crport_mpll_charge_pump;
	/* RX LFPS(decode) mode */
	u8 rx_decode_mode;
};

/**
 * struct exynos_usbphy_info : USBPHY information to share USBPHY CAL code
 * @version: PHY controller version
 *	     0x0100 - for EXYNOS_USB3 : EXYNOS7420, EXYNOS7890
 *	     0x0101 -			EXYNOS8890
 *	     0x0111 -			EXYNOS8895
 *	     0x0200 - for EXYNOS_USB2 : EXYNOS7580, EXYNOS3475
 *	     0x0210 -			EXYNOS8890_EVT1
 *	     0xF200 - for EXT	      : EXYNOS7420_HSIC
 * @refclk: reference clock frequency for USBPHY
 * @refsrc: reference clock source path for USBPHY
 * @use_io_for_ovc: use over-current notification io for USBLINK
 * @regs_base: base address of PHY control register *
 */

struct exynos_usbphy_info {
	/* Device Information */
	struct device *dev;

	u32	version;
	enum exynos_usbphy_refclk refclk;
	enum exynos_usbphy_refsel refsel;

	bool use_io_for_ovc;
	bool common_block_disable;
	bool not_used_vbus_pad;

	void __iomem *regs_base;

	/* HS PHY tune parameter */
	struct exynos_usbphy_hs_tune *hs_tune;

	/* SS PHY tune parameter */
	struct exynos_usbphy_ss_tune *ss_tune;

	/* Tune Parma list */
	struct exynos_usb_tune_param *tune_param;

	/* multiple phy */
	int	hw_version;
	void __iomem *regs_base_2nd;
	int	used_phy_port;

	/* Alternative PHY REF_CLK source */
	bool alt_ref_clk;

	/* Remote Wake-up Advisor */
	unsigned hs_rewa :1;

	/* Dual PHY */
	bool dual_phy;
};


#endif	/* __PHY_SAMSUNG_USB_FW_CAL_H__ */
