/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Author: Sung-Hyun Na <sunghyun.na@samsung.com>
 *
 * Chip Abstraction Layer for USB PHY
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

#include <linux/delay.h>
#include <linux/io.h>
#include "phy-samsung-usb-cal.h"
#include "phy-exynos-usbdp-reg.h"

int phy_exynos_usbdp_check_pll_lock(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	u32 reg2f, reg4b;
	int cnt;

	for (cnt = 1000; cnt != 0; cnt--) {
		reg2f = readl(regs_base + EXYNOS_USBDP_COM_CMN_R2F);
		reg4b = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R4B);
		if ((reg2f & USBDP_CMN2F_PLL_LOCK_DONE) &&
				(reg4b & USBDP_TRSV4B_CDR_FLD_PLL_MODE_DONE))
			break;
		udelay(1);
	}
	if (!cnt)
		return -1;
	pr_info("PLL Lock done\n");
	return 0;
}

void phy_exynos_usbdp_tune_each(struct exynos_usbphy_info *info, char *name,
	int val)
{
	void __iomem *regs_base = info->regs_base;
	u32 reg;

	if (!name)
		return;

	if (val == -1)
		return;

	if (!strcmp(name, "sstx_deemph")) {
		/* TX De-emphasis */
		reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R0C);
		reg &= ~USBDP_TRSV0C_MAN_TX_DE_EMP_LVL_MASK;
		reg |= USBDP_TRSV0C_MAN_TX_DE_EMP_LVL_SET(val);
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R0C);
	} else if (!strcmp(name, "sstx_amp")) {
		/* TX Amplitude */
		reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R0C);
		reg &= ~USBDP_TRSV0C_MAN_TX_DRVR_LVL_MASK;
		reg |= USBDP_TRSV0C_MAN_TX_DRVR_LVL_SET(val);
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R0C);
	} else if (!strcmp(name, "sstx_boost")) {
		u32 reg56, drv_value;

		reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R59);
		reg56 = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R56);
		reg &= ~USBDP_TRSV59_TX_SFR_DRV_IDRV_IUP_CTRL_MASK;
		reg56 &= ~USBDP_TRSV56_LFPS_DRV_IDRV_IUP_CTRL_MASK;
		if (val == 1)
			drv_value = 0x2;
		else if (val == 2)
			drv_value = 0x4;
		else if (val == 3)
			drv_value = 0x1;
		else if (val == 4)
			drv_value = 0x6;
		else if (val == 5)
			drv_value = 0x3;
		else if (val == 6)
			drv_value = 0x7;
		else
			drv_value = 0x0;
		reg |= USBDP_TRSV59_TX_SFR_DRV_IDRV_IUP_CTRL_SET(drv_value);
		reg56 |= USBDP_TRSV56_LFPS_DRV_IDRV_IUP_CTRL_SET(drv_value);
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R59);
		writel(reg56, regs_base + EXYNOS_USBDP_COM_TRSV_R56);
	} else if (!strcmp(name, "ssrx_los")) {
		/* Rx Squelch Detect Threshold */
		reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R0A);
		reg &= ~USBDP_TRSV0A_APB_CAL_OFFSET_DIFP_MASK;
		reg |= USBDP_TRSV0A_APB_CAL_OFFSET_DIFP_SET(val);
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R0A);

		reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R0B);
		reg &= ~USBDP_TRSV0B_APB_CAL_OFFSET_DIFN_MASK;
		reg |= USBDP_TRSV0B_APB_CAL_OFFSET_DIFN_SET(val);
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R0B);
	} else if (!strcmp(name, "ssrx_ctle_peak")) {
		/* Rx EQ CTLE peak frequency */
		reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R02);
		reg &= ~USBDP_TRSV02_RXAFE_LEQ_CSEL_GEN2_MASK;
		reg |= USBDP_TRSV02_RXAFE_LEQ_CSEL_GEN2_SET(val);
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R02);
	} else if (!strcmp(name, "ssrx_eq_code")) {
		/* Rx EQ code */
		reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R03);
		reg &= ~USBDP_TRSV03_RXAFE_LEQ_RSEL_GEN1_MASK;
		reg &= ~USBDP_TRSV03_RXAFE_LEQ_RSEL_GEN2_MASK;
		reg |= USBDP_TRSV03_RXAFE_LEQ_RSEL_GEN2_SET(val);
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R03);
	} else if (!strcmp(name, "ssrx_cur_ctrl")) {
		/* Rx EQ Current control */
		reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R01);
		reg &= ~USBDP_TRSV01_RXAFE_LEQ_ISEL_GEN2_MASK;
		reg |= USBDP_TRSV01_RXAFE_LEQ_ISEL_GEN2_SET(val);
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R01);
	} else if (!strcmp(name, "ssrx_eqen")) {
		/* Rx EQ Stage Enable control */
		reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R01);
		reg &= ~USBDP_TRSV01_RXAFE_CTLE_SEL_MASK;
		reg |= USBDP_TRSV01_RXAFE_CTLE_SEL_SET(val);
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R01);
	} else if (!strcmp(name, "lfps_rx_th")) {
		reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R3D);
		reg &= ~USBDP_TRSV3D_SFR_LFPS_DET_TH_MASK;
		reg |= USBDP_TRSV3D_SFR_LFPS_DET_TH_SET(val);
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R3D);
	} else if (!strcmp(name, "add_tune_13b")) {
		reg = val & 0xff;
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R13B);
	} else if (!strcmp(name, "add_tune_163")) {
		reg = val & 0xff;
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R163);
	} else if (!strcmp(name, "add_tune_172")) {
		reg = val & 0xff;
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R172);
	} else if (!strcmp(name, "add_tune_173")) {
		reg = val & 0xff;
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R173);
	} else if (!strcmp(name, "add_tune_174")) {
		reg = val & 0xff;
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R174);
	} else if (!strcmp(name, "add_tune_175")) {
		reg = val & 0xff;
		writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R175);
	}
}

void phy_exynos_usbdp_tune(struct exynos_usbphy_info *info)
{
	u32 cnt = 0;

	if (!info)
		return;

	if (!info->tune_param)
		return;

	for (; info->tune_param[cnt].value != EXYNOS_USB_TUNE_LAST; cnt++) {
		char *para_name;
		int val;

		val = info->tune_param[cnt].value;
		if (val == -1)
			continue;
		para_name = info->tune_param[cnt].name;
		if (!para_name)
			break;
		phy_exynos_usbdp_tune_each(info, para_name, val);
	}
}

void phy_exynos_usbdp_ilbk(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	u32 reg;

	/* PCS Block */
	reg = readl(info->regs_base_2nd + USBDP_PCSREG_MASK_PMA_IF);
	reg |= (1 << 3) | (1 << 4);
	reg &= ~((1 << 11) | (1 << 12));
	writel(reg, info->regs_base_2nd + USBDP_PCSREG_MASK_PMA_IF);

	/* Turn on analog block for internal loopback */
	reg = readl(info->regs_base_2nd + USBDP_PCSREG_OUT_VEC_3);
	/* Power down */
	reg &= ~USBDP_PCSREG_DYN_CON_PWR_DOWN;
	reg |= USBDP_PCSREG_SEL_OUT_PWR_DOWN;
	writel(reg, info->regs_base_2nd + USBDP_PCSREG_OUT_VEC_3);

	/* Check PLL lock done */
	phy_exynos_usbdp_check_pll_lock(info);

	/* Stpe 2.1 */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R57);
	reg |= 0x1;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R57);
	/* Stpe 2.2 */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R26);
	reg |= USBDP_TRSV26_MAN_TX_SER_EN;
	reg |= USBDP_TRSV26_MAN_CDR_DES_EN;
	reg |= USBDP_TRSV26_MAN_CDR_EN;
	reg |= USBDP_TRSV26_MAN_CDR_CKLPB_MUX_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R26);
	/* Stpe 2.3 Enable Manual mode control */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R27);
	reg |= USBDP_TRSV27_MAN_EN_CTRL;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R27);

	/* Step 3: Start TX BIST */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R28);
	reg |= USBDP_TRSV28_AUTO_BIST_EN;
	reg |= USBDP_TRSV28_BIST_DATA_EN;
	reg |= USBDP_TRSV28_TX_BIST_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R28);

	/* Step 4.1: */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R23);
	reg &= ~USBDP_TRSV23_DATA_CLEAR_BY_SIGVAL;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R23);

	/* Step 4.2: */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R39);
	reg |= USBDP_TRSV39_CDR_TXDATA_SEL;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R39);

	/* Step 4.3: */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R0F);
	reg |= USBDP_TRSV0F_TX_SER_LB_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R0F);

	/* Step 4.4: */
	if (info->used_phy_port == 0) {
		reg = readl(regs_base + EXYNOS_USBDP_COM_DP_R10);
		reg |= USBDP_DP10_LN0_TX_MAN_EN_RST_CTRL_0;
		writel(reg, regs_base + EXYNOS_USBDP_COM_DP_R10);

		reg = readl(regs_base + EXYNOS_USBDP_COM_DP_R11);
		reg |= USBDP_DP11_LN0_TX_SLB_EN;
		writel(reg, regs_base + EXYNOS_USBDP_COM_DP_R11);
	} else {
		reg = readl(regs_base + EXYNOS_USBDP_COM_DP_R50);
		reg |= USBDP_DP50_LN2_TX_MAN_EN_RST_CTRL_0;
		writel(reg, regs_base + EXYNOS_USBDP_COM_DP_R50);

		reg = readl(regs_base + EXYNOS_USBDP_COM_DP_R51);
		reg |= USBDP_DP51_LN2_TX_SLB_EN;
		writel(reg, regs_base + EXYNOS_USBDP_COM_DP_R51);
	}

	/* Step 5: */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R27);
	reg |= USBDP_TRSV27_MAN_TX_SER;
	reg |= USBDP_TRSV27_MAN_DESKEW_RSTN;
	reg |= USBDP_TRSV27_MAN_CDR_AFC_RSTN;
	reg |= USBDP_TRSV27_MAN_CDR_AFC_INIT_RSTN;
	reg |= USBDP_TRSV27_MAN_VALID_EN;
	reg |= USBDP_TRSV27_MAN_CDR_DES_RSTN;
	reg |= USBDP_TRSV27_MAN_RSTN_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R27);

	/* Step 6: wait 4time comma pattern  */
	udelay(2);

	/* Step 7.1: */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R57);
	reg &= ~0x1;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R57);

	/* Step 7.2: */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R27);
	reg &= ~USBDP_TRSV27_MAN_EN_CTRL;
	reg &= ~USBDP_TRSV27_MAN_RSTN_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R27);

	/* Step 8: */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R28);
	reg &= ~USBDP_TRSV28_AUTO_BIST_EN;
	reg &= ~USBDP_TRSV28_BIST_DATA_EN;
	reg &= ~USBDP_TRSV28_TX_BIST_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R28);

	/* Step 9.1: RX CDR data clear */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R23);
	reg |= USBDP_TRSV23_DATA_CLEAR_BY_SIGVAL;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R23);

	/* 9.2 disable TX to RX serial loopback path */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R0F);
	reg &= ~USBDP_TRSV0F_TX_SER_LB_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R0F);

	/* Step 9.3: disable loopback path on the each laen */
	if (info->used_phy_port == 0) {
		reg = readl(regs_base + EXYNOS_USBDP_COM_DP_R10);
		reg &= ~USBDP_DP10_LN0_TX_MAN_EN_RST_CTRL_0;
		writel(reg, regs_base + EXYNOS_USBDP_COM_DP_R10);

		reg = readl(regs_base + EXYNOS_USBDP_COM_DP_R11);
		reg &= ~USBDP_DP11_LN0_TX_SLB_EN;
		writel(reg, regs_base + EXYNOS_USBDP_COM_DP_R11);
	} else {
		reg = readl(regs_base + EXYNOS_USBDP_COM_DP_R50);
		reg &= ~USBDP_DP50_LN2_TX_MAN_EN_RST_CTRL_0;
		writel(reg, regs_base + EXYNOS_USBDP_COM_DP_R50);

		reg = readl(regs_base + EXYNOS_USBDP_COM_DP_R51);
		reg &= ~USBDP_DP51_LN2_TX_SLB_EN;
		writel(reg, regs_base + EXYNOS_USBDP_COM_DP_R51);
	}

	reg = readl(info->regs_base_2nd + USBDP_PCSREG_OUT_VEC_3);
	/* Power down */
	reg &= ~(USBDP_PCSREG_DYN_CON_PWR_DOWN | USBDP_PCSREG_SEL_OUT_PWR_DOWN);
	writel(reg, info->regs_base_2nd + USBDP_PCSREG_OUT_VEC_3);

	udelay(1);

	reg = 0x7f00;
	writel(reg, info->regs_base_2nd + USBDP_PCSREG_MASK_PMA_IF);
}

void phy_exynos_usbdp_pcs_reset(struct exynos_usbphy_info *info)
{
	u32 reg;

	/* Set Proper Value for PCS to avoid abnormal RX Detect */
	reg = readl(info->regs_base_2nd + USBDP_PCSREG_FRONT_END_MODE_VEC);
	reg |= USBDP_PCSREG_EN_REALIGN;
	writel(reg, info->regs_base_2nd + USBDP_PCSREG_FRONT_END_MODE_VEC);

	/* optimize elastic buffer threshold */
	reg = USBDP_PCSREG_SKP_REMOVE_TH_SET(0x13);
	reg |= USBDP_PCSREG_SKP_INSERT_TH_SET(0xa);
	reg |= USBDP_PCSREG_NUM_INIT_BUFFERING_SET(0xb);
	writel(reg, info->regs_base_2nd + USBDP_PCSREG_EBUF_PARAM);

	reg = readl(info->regs_base_2nd + USBDP_PCSREG_BACK_END_MODE_VEC);
	reg &= ~USBDP_PCSREG_DISABLE_DATA_MASK;
	writel(reg, info->regs_base_2nd + USBDP_PCSREG_BACK_END_MODE_VEC);

	reg = USBDP_PCSREG_COMP_EN_ASSERT_SET(0x3f);
	writel(reg, info->regs_base_2nd + USBDP_PCSREG_DET_COMP_EN_SET);

	/* BGR is always on in the P3 mode */
	reg = readl(info->regs_base_2nd + USBDP_PCSREG_OUT_VEC_3);
	reg &= ~USBDP_PCSREG_DYN_CON_BGR_BIAS;
	reg |= USBDP_PCSREG_SEL_OUTBGR_BIAS;
	writel(reg, info->regs_base_2nd + USBDP_PCSREG_OUT_VEC_3);
}

void phy_exynos_usbdp_enable(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	u32 reg, reg_2c, reg_2d, reg_dp_b3;

	/* Recevier Detection Off */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R24);
	reg |= USBDP_TRSV24_MAN_TX_RCV_DET_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R24);

	/* Set PCS Value */
	phy_exynos_usbdp_pcs_reset(info);

	reg = 0;
	reg |= USBDP_CMN0E_PLL_AFC_VCO_CNT_RUN_NO_SET(0x4);
	reg |= USBDP_CMN0E_PLL_AFC_ANA_CPI_CTRL_SET(0x2);
	writel(reg, regs_base + EXYNOS_USBDP_COM_CMN_R0E);

	reg = 0;
	reg |= USBDP_CMN0F_PLL_ANA_EN_PI;
	reg |= USBDP_CMN0F_PLL_ANA_DCC_EN_SET(0xf);
	reg |= USBDP_CMN0F_PLL_ANA_CPP_CTRL_SET(0x7);
	writel(reg, regs_base + EXYNOS_USBDP_COM_CMN_R0F);

	reg = 0;
	reg |= USBDP_CMN10_PLL_ANA_VCI_SEL_SET(0x6);
	reg |= USBDP_CMN10_PLL_ANA_LPF_RSEL_SET(0x8);
	writel(reg, regs_base + EXYNOS_USBDP_COM_CMN_R10);

	reg = 0;
	reg |= USBDP_CMN25_PLL_AGMC_TG_CODE_SET(0x30);
	writel(reg, regs_base + EXYNOS_USBDP_COM_CMN_R25);

	reg = 0;
	reg |= USBDP_CMN26_PLL_AGMC_COMP_EN;
	reg |= USBDP_CMN26_PLL_AGMC_FROM_MAX_GM;
	reg |= USBDP_CMN26_PLL_AFC_FROM_PRE_CODE;
	reg |= USBDP_CMN26_PLL_AFC_MAN_BSEL_L_SET(0x3);
	writel(reg, regs_base + EXYNOS_USBDP_COM_CMN_R26);

	reg = 0;
	reg |= USBDP_CMN27_PLL_ANA_LC_VREF_BYPASS_EN;
	reg |= USBDP_CMN27_PLL_ANA_LC_VCDO_CAP_OFFSET_SEL_SET(0x5);
	reg |= USBDP_CMN27_PLL_ANA_LC_VCO_BUFF_EN;
	reg |= USBDP_CMN27_PLL_ANA_LC_GM_COMP_VCI_SEL_SET(0x4);
	writel(reg, regs_base + EXYNOS_USBDP_COM_CMN_R27);

	reg = 0;
	reg |= USBDP_TRSV23_DATA_CLEAR_BY_SIGVAL;
	reg |= USBDP_TRSV23_FBB_H_BW_DIFF_SET(0x5);
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R23);

	/* SSC Setting */
	reg = readl(regs_base + EXYNOS_USBDP_COM_CMN_R24);
	reg |= USBDP_CMN24_SSC_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_CMN_R24);

	reg_2c = readl(regs_base + EXYNOS_USBDP_COM_CMN_R2C);
	reg_2d = readl(regs_base + EXYNOS_USBDP_COM_CMN_R2D);
	reg_dp_b3 = readl(regs_base + EXYNOS_USBDP_COM_DP_RB3);
	if (info->used_phy_port == 0) {
		reg_2c |= USBDP_CMN2C_MAN_USBDP_MODE_SET(0x3);
		reg_2d |= USBDP_CMN2D_USB_TX1_SEL;
		reg_2d &= ~USBDP_CMN2D_USB_TX3_SEL;
		reg_dp_b3 &= ~USBDP_DPB3_CMN_DUMMY_CTRL_7;
		reg_dp_b3 |= USBDP_DPB3_CMN_DUMMY_CTRL_6;
		reg_dp_b3 |= USBDP_DPB3_CMN_DUMMY_CTRL_1;
		reg_dp_b3 |= USBDP_DPB3_CMN_DUMMY_CTRL_0;

		/* Add at 17.08.21 - decrease DP TX level on DP lane */
		writel(0x00,  regs_base + EXYNOS_USBDP_COM_DP_R16);
		writel(0x00,  regs_base + EXYNOS_USBDP_COM_DP_R1A);
	} else {
		reg_2c &= ~USBDP_CMN2C_MAN_USBDP_MODE_MASK;
		reg_2d &= ~USBDP_CMN2D_USB_TX1_SEL;
		reg_2d |= USBDP_CMN2D_USB_TX3_SEL;
		reg_dp_b3 |= USBDP_DPB3_CMN_DUMMY_CTRL_7;
		reg_dp_b3 &= ~USBDP_DPB3_CMN_DUMMY_CTRL_6;
		reg_dp_b3 &= ~USBDP_DPB3_CMN_DUMMY_CTRL_1;
		reg_dp_b3 &= ~USBDP_DPB3_CMN_DUMMY_CTRL_0;

		/* Add at 17.08.21 - decrease DP TX level on DP lane */
		writel(0x00,  regs_base + EXYNOS_USBDP_COM_DP_R56);
		writel(0x00,  regs_base + EXYNOS_USBDP_COM_DP_R5A);
	}
	reg_2c |= USBDP_CMN2C_MAN_USBDP_MODE_EN;
	reg_2d &= ~USBDP_CMN2D_LCPLL_SSCDIV_MASK;
	reg_2d |= USBDP_CMN2D_LCPLL_SSCDIV_SET(0x1);
	writel(reg_2c, regs_base + EXYNOS_USBDP_COM_CMN_R2C);
	writel(reg_2d, regs_base + EXYNOS_USBDP_COM_CMN_R2D);
	writel(reg_dp_b3,  regs_base + EXYNOS_USBDP_COM_DP_RB3);

	/* LFPS RX Threshold Control */
	reg = 0;
	reg |= USBDP_TRSV38_SFR_RX_LFPS_LPF_CTRL_SET(0x2);
	reg |= USBDP_TRSV38_SFR_RX_LFPS_TH_CTRL_SET(0x2);
	reg |= USBDP_TRSV38_RX_LFPS_DET_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R38);

	/* FBB manual control enable */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R1F);
	reg |= USBDP_TRSV1F_FBB_MAN_SEL;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R1F);

	/* Set FBB proper value */
	reg = USBDP_TRSV21_FBB_MAN_LB_SET(0x1);
	reg |= USBDP_TRSV21_FBB_MAN_HB_SET(0x2);
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R21);

	/* Enable Manual de-emphasis control */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R0D);
	reg |= USBDP_TRSV0D_MAN_DRVR_DE_EMP_LVL_MAN_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R0D);

	/* Enable TX Current booster */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R56);
	reg |= USBDP_TRSV56_LFPS_DRV_IDRV_IUP_CTRL_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R56);

	/* Set Tune Value */
	phy_exynos_usbdp_tune(info);

	/* Sys Valid Debouncd Digital Filter */
	reg = 0;
	reg |= USBDP_TRSV34_INT_SIGVAL_FILT_SEL;
	reg |= USBDP_TRSV34_OUT_SIGVAL_FILT_SEL;
	reg |= USBDP_TRSV34_SIGVAL_FILT_DLY_CODE_SET(0x3);
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R34);

	/* Recevier Detection On */
	reg = readl(regs_base + EXYNOS_USBDP_COM_TRSV_R24);
	reg &= ~USBDP_TRSV24_MAN_TX_RCV_DET_EN;
	writel(reg, regs_base + EXYNOS_USBDP_COM_TRSV_R24);

	writel(0x78, regs_base + EXYNOS_USBDP_COM_TRSV_R3D);

}

void phy_exynos_usbdp_disable(struct exynos_usbphy_info *info)
{

}
