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
#include <linux/platform_device.h>
#include "phy-samsung-usb-cal.h"
#include "phy-exynos-usbdp-gen2-reg.h"
#include <linux/soc/samsung/exynos-soc.h>

#define CDR_FBB_FINE_VAL		0x2

static inline u32 usbdp_cal_reg_rd(void *addr)
{
	u32 reg;

	reg = readl(addr);
#if defined(USBDP_GEN2_DBG)
	print_log("[USB/DP]Rd Reg:0x%x\t\tVal:0x%x", addr, reg);
#endif
	return reg;
}

static inline void usbdp_cal_reg_wr(u32 val, void *addr)
{
#if defined(USBDP_GEN2_DBG)
	print_log("[USB/DP]Wr Reg:0x%x\t\tVal:0x%x", addr, val);
#endif
	writel(val, addr);
}

int phy_exynos_usbdp_g2_rx_cdr_pll_lock(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	u8 reg0b84;
	u8 reg1b84;

	int cnt;

	if (info->used_phy_port == 0) {
		for (cnt = 1000; cnt != 0; cnt--) {
			reg0b84 = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_A0B84);
			if ((reg0b84 & USBDP_TRSV_A0B84_LN0_MON_RX_CDR_LOCK_DONE))
				break;
			udelay(1);
		}
	} else {
		for (cnt = 1000; cnt != 0; cnt--) {
			reg1b84 = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_A1B84);
			if ((reg1b84 & USBDP_TRSV_A1B84_LN2_MON_RX_CDR_LOCK_DONE))
				break;
			udelay(1);
		}
	}
	if (!cnt)
		return -1;
	dev_info(info->dev, "CDR Lock done\n");

	return 0;
}

void phy_exynos_usbdp_g2_tune_each(struct exynos_usbphy_info *info, char *name,
	u32 val)
{
	void __iomem *regs_base = info->regs_base;
	u32 reg = 0;

	if (!name)
		return;

	if (val == -1)
		return;

	if (!strcmp(name, "ssrx_sqhs_th")) {
		/* RX squelch detect threshold level */
		/* LN0 TX */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_26E_09B8);
		reg &= USBDP_TRSV_26E_LN0_RX_SQHS_TH_CLR;
		reg |= USBDP_TRSV_26E_LN0_RX_SQHS_TH(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_26E_09B8);
		/* LN2 TX */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_066E);
		reg &= USBDP_TRSV_066E_LN2_RX_SQHS_TH_CLR;
		reg |= USBDP_TRSV_066E_LN2_RX_SQHS_TH(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_066E);

	} else if (!strcmp(name, "ssrx_lfps_th")) {
		/* lfps detect threshold control */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0270);
		reg &= USBDP_TRSV_0270_LN0_RX_LFPS_TH_CTRL_CLR;
		reg |= USBDP_TRSV_0270_LN0_RX_LFPS_TH_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0270);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0670);
		reg &= USBDP_TRSV_0670_LN2_RX_LFPS_TH_CTRL_CLR;
		reg |= USBDP_TRSV_0670_LN2_RX_LFPS_TH_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0670);

	} else if (!strcmp(name, "ssrx_mf_eq_en")) {
		/* RX MF EQ Enable */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0241);
		reg |= USBDP_TRSV_0241_LN0_RX_CTLE_RL_HF_HBR3(3); // 04.05
		reg |= USBDP_TRSV_0241_LN0_RX_CTLE_MF_BWD_EN;
		reg &= USBDP_TRSV_0241_LN0_RX_CTLE_OC_DAC_PU_CLR; // 04.05
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0241);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0641);
		reg |= USBDP_TRSV_0641_LN2_RX_CTLE_RL_HF_HBR3(3);
		reg |= USBDP_TRSV_0641_LN2_RX_CTLE_MF_BWD_EN;
		reg &= USBDP_TRSV_0641_LN2_RX_CTLE_OC_DAC_PU_CLR;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0641);

	} else if (!strcmp(name, "ssrx_mf_eq_ctrl_ss")) {
		/* RX MF EQ CONTROL */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0311);
		reg &= USBDP_TRSV_0311_LN0_RX_SSLMS_MF_INIT_RATE_SP_CLR;
		reg |= USBDP_TRSV_0311_LN0_RX_SSLMS_MF_INIT_RATE_SP(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0311);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0711);
		reg &= USBDP_TRSV_0711_LN2_RX_SSLMS_MF_INIT_RATE_SP_CLR;
		reg |= USBDP_TRSV_0711_LN2_RX_SSLMS_MF_INIT_RATE_SP(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0711);
	} else if (!strcmp(name, "ssrx_hf_eq_ctrl_ss")) {
		/* RX HF EQ CONTROL */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_030B);
		reg &= USBDP_TRSV_030B_LN0_RX_SSLMS_HF_INIT_RATE_SP_CLR;
		reg |= USBDP_TRSV_030B_LN0_RX_SSLMS_HF_INIT_RATE_SP(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_030B);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_070B);
		reg &= USBDP_TRSV_070B_LN2_RX_SSLMS_HF_INIT_RATE_SP_CLR;
		reg |= USBDP_TRSV_070B_LN2_RX_SSLMS_HF_INIT_RATE_SP(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_070B);
	} else if (!strcmp(name, "ssrx_mf_eq_ctrl_ssp")) {
		/* RX MF EQ CONTROL */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0312);
		reg &= USBDP_TRSV_0312_LN0_RX_SSLMS_MF_INIT_RATE_SSP_CLR;
		reg |= USBDP_TRSV_0312_LN0_RX_SSLMS_MF_INIT_RATE_SSP(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0312);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0712);
		reg &= USBDP_TRSV_0712_LN2_RX_SSLMS_MF_INIT_RATE_SSP_CLR;
		reg |= USBDP_TRSV_0712_LN2_RX_SSLMS_MF_INIT_RATE_SSP(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0712);
	} else if (!strcmp(name, "ssrx_hf_eq_ctrl_ssp")) {
		/* RX HF EQ CONTROL */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_030C);
		reg &= USBDP_TRSV_030C_LN0_RX_SSLMS_HF_INIT_RATE_SSP_CLR;
		reg |= USBDP_TRSV_030C_LN0_RX_SSLMS_HF_INIT_RATE_SSP(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_030C);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_070C);
		reg &= USBDP_TRSV_70C_LN2_RX_SSLMS_HF_INIT_RATE_SSP_CLR;
		reg |= USBDP_TRSV_70C_LN2_RX_SSLMS_HF_INIT_RATE_SSP(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_070C);
	} else if (!strcmp(name, "ssrx_dfe1_tap_ctrl")) {
		/* RX DFE1 TAp CONTROL */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0279);
		reg &= USBDP_TRSV_0279_LN0_RX_SSLMS_C1_INIT_CLR;
		reg |= USBDP_TRSV_0279_LN0_RX_SSLMS_C1_INIT(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0279);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0679);
		reg &= USBDP_TRSV_0679_LN2_RX_SSLMS_C1_INIT_CLR;
		reg |= USBDP_TRSV_0679_LN2_RX_SSLMS_C1_INIT(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0679);

	} else if (!strcmp(name, "ssrx_dfe2_tap_ctrl")) {
		/* RX DFE2 TAp CONTROL */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_027A);
		reg &= USBDP_TRSV_027A_LN0_RX_SSLMS_C2_INIT_CLR;
		reg |= USBDP_TRSV_027A_LN0_RX_SSLMS_C2_INIT(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_027A);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_067A);
		reg &= USBDP_TRSV_067A_LN2_RX_SSLMS_C2_INIT_CLR;
		reg |= USBDP_TRSV_067A_LN2_RX_SSLMS_C2_INIT(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_067A);

	} else if (!strcmp(name, "ssrx_dfe3_tap_ctrl")) {
		/* RX DFE3 TAp CONTROL */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_027B);
		reg &= USBDP_TRSV_027B_LN0_RX_SSLMS_C3_INIT_CLR;
		reg |= USBDP_TRSV_027B_LN0_RX_SSLMS_C3_INIT(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_027B);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_067B);
		reg &= USBDP_TRSV_067B_LN2_RX_SSLMS_C3_INIT_CLR;
		reg |= USBDP_TRSV_067B_LN2_RX_SSLMS_C3_INIT(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_067B);

	} else if (!strcmp(name, "ssrx_dfe4_tap_ctrl")) {
		/* RX DFE4 TAp CONTROL */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_027C);
		reg &= USBDP_TRSV_027C_LN0_RX_SSLMS_C4_INIT_CLR;
		reg |= USBDP_TRSV_027C_LN0_RX_SSLMS_C4_INIT(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_027C);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_067C);
		reg &= USBDP_TRSV_067C_LN2_RX_SSLMS_C4_INIT_CLR;
		reg |= USBDP_TRSV_067C_LN2_RX_SSLMS_C4_INIT(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_067C);

	} else if (!strcmp(name, "ssrx_dfe5_tap_ctrl")) {
		/* RX DFE5 TAp CONTROL */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_027D);
		reg &= USBDP_TRSV_027D_LN0_RX_SSLMS_C5_INIT_CLR;
		reg |= USBDP_TRSV_027D_LN0_RX_SSLMS_C5_INIT(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_027D);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_067D);
		reg &= USBDP_TRSV_067D_LN2_RX_SSLMS_C5_INIT_CLR;
		reg |= USBDP_TRSV_067D_LN2_RX_SSLMS_C5_INIT(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_067D);

	} else if (!strcmp(name, "ssrx_term_cal")) {
		/* RX termination calibration */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_02BD);
		reg &= USBDP_TRSV_02BD_LN0_RX_RTERM_CTRL_CLR;
		reg |= USBDP_TRSV_02BD_LN0_RX_RTERM_CTRL(val);
		reg |= USBDP_TRSV_02BD_LN0_RX_RCAL_OPT_CODE(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_02BD);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_06BD);
		reg &= USBDP_TRSV_06BD_LN2_RX_RTERM_CTRL_CLR;
		reg |= USBDP_TRSV_06BD_LN2_RX_RTERM_CTRL(val);
		reg |= USBDP_TRSV_06BD_LN2_RX_RCAL_OPT_CODE(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_06BD);

	} else if (!strcmp(name, "sstx_amp")) {
		/* TX Amplitude */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0404);
		reg |= USBDP_TRSV_0404_OVRD_LN1_TX_DRV_LVL_CTRL;
		reg &= USBDP_TRSV_0404_LN1_TX_DRV_LVL_CTRL_CLR;
		reg |= USBDP_TRSV_0404_LN1_TX_DRV_LVL_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0404);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0804);
		reg |= USBDP_TRSV_0804_OVRD_LN3_TX_DRV_LVL_CTRL;
		reg &= USBDP_TRSV_0804_LN3_TX_DRV_LVL_CTRL_CLR;
		reg |= USBDP_TRSV_0804_LN3_TX_DRV_LVL_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0804);

	} else if (!strcmp(name, "sstx_deemp")) {
		/* TX de-emphasise */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0405);
		reg |= USBDP_TRSV_0405_OVRD_LN1_TX_DRV_POST_LVL_CTRL;
		reg &= USBDP_TRSV_0405_LN1_TX_DRV_POST_LVL_CTRL_CLR;
		reg |= USBDP_TRSV_0405_LN1_TX_DRV_POST_LVL_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0405);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0805);
		reg |= USBDP_TRSV_0805_OVRD_LN3_TX_DRV_POST_LVL_CTRL;
		reg &= USBDP_TRSV_0805_LN3_TX_DRV_LVL_POST_CTRL_CLR;
		reg |= USBDP_TRSV_0805_LN3_TX_DRV_LVL_POST_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0805);

	} else if (!strcmp(name, "sstx_pre_shoot")) {
		/* TX pre shoot */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0406);
		reg |= USBDP_TRSV_0406_OVRD_LN1_TX_DRV_PRE_LVL_CTRL;
		reg &= USBDP_TRSV_0406_LN1_TX_DRV_PRE_LVL_CTRL_CLR;
		reg |= USBDP_TRSV_0406_LN1_TX_DRV_PRE_LVL_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0406);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0806);
		reg |= USBDP_TRSV_0806_OVRD_LN3_TX_DRV_PRE_LVL_CTRL;
		reg &= USBDP_TRSV_0806_LN3_TX_DRV_PRE_LVL_CTRL_CLR;
		reg |= USBDP_TRSV_0806_LN3_TX_DRV_PRE_LVL_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0806);

	} else if (!strcmp(name, "sstx_idrv_up")) {
		/* TX idrv up */
#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0407);
		reg &= USBDP_TRSV_0407_LN1_TX_DRV_IDRV_IUP_CTRL_CLR;
		reg |= USBDP_TRSV_0407_LN1_TX_DRV_IDRV_IUP_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0407);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0807);
		reg &= USBDP_TRSV_0807_LN3_TX_DRV_IDRV_IUP_CTRL_CLR;
		reg |= USBDP_TRSV_0807_LN3_TX_DRV_IDRV_IUP_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0807);
#else
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0407);
		reg &= USBDP_TRSV_0407_LN1_TX_DRV_IDRV_IUP_CTRL_CLR;
		reg |= USBDP_TRSV_0407_LN1_TX_DRV_IDRV_IUP_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0407);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0807);
		reg &= USBDP_TRSV_0807_LN3_TX_DRV_IDRV_IUP_CTRL_CLR;
		reg |= USBDP_TRSV_0807_LN3_TX_DRV_IDRV_IUP_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0807);
#endif
	} else if (!strcmp(name, "sstx_idrv_dn")) {
		/* TX idrv down */
#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_CMN_041C);
		reg &= USBDP_CMN_041C_LN1_TX_DRV_IDRV_IDN_CTRL_CLR;
		reg |= USBDP_CMN_041C_LN1_TX_DRV_IDRV_IDN_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_041C);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_CMN_0420);
		reg &= USBDP_CMN_0420_LN3_TX_DRV_IDRV_IDN_CTRL_CLR;
		reg |= USBDP_CMN_0420_LN3_TX_DRV_IDRV_IDN_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_0420);
#else
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0407);
		reg &= USBDP_TRSV_0407_LN1_TX_DRV_IDRV_IDN_CTRL_CLR;
		reg |= USBDP_TRSV_0407_LN1_TX_DRV_IDRV_IDN_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0407);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0807);
		reg &= USBDP_TRSV_0807_LN3_TX_DRV_IDRV_IDN_CTRL_CLR;
		reg |= USBDP_TRSV_0807_LN3_TX_DRV_IDRV_IDN_CTRL(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0807);
#endif
	} else if (!strcmp(name, "sstx_up_term")) {
		/* TX up termination */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0420);
		reg &= USBDP_TRSV_0420_LN1_TX_RCAL_UP_CODE_CLR;
		reg |= USBDP_TRSV_0420_LN1_TX_RCAL_UP_CODE(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0420);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0820);
		reg &= USBDP_TRSV_0820_LN3_TX_RCAL_UP_CODE_CLR;
		reg |= USBDP_TRSV_0820_LN3_TX_RCAL_UP_CODE(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0820);

	} else if (!strcmp(name, "sstx_dn_term")) {
		/* TX dn termination */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0420);
		reg &= USBDP_TRSV_0420_LN1_TX_RCAL_DN_CODE_CLR;
		reg |= USBDP_TRSV_0420_LN1_TX_RCAL_DN_CODE(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0420);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0820);
		reg &= USBDP_TRSV_0820_LN3_TX_RCAL_DN_CODE_CLR;
		reg |= USBDP_TRSV_0820_LN3_TX_RCAL_DN_CODE(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0820);
	} else if (!strcmp(name, "fbb_fine")) {
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R2A6_0A98);
		reg |= USBDP_TRSV_R2A6_LN0_RX_CDR_FBB_FINE_CTRL_SET(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R2A6_0A98);

	} else if (!strcmp(name, "fbb_hi")) {
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_A0a9c);
		reg |= HI_FBB(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_A0a9c);
	} else if (!strcmp(name, "fbb_lo")) {
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_A0a9c);
		reg |= LO_FBB(val);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_A0a9c);

	}

	/* 2018.05.18 CDR MF/HF/DFE post setting */
	if ((!strcmp(name, "ssrx_mf_eq_ctrl_ss")) ||
		(!strcmp(name, "ssrx_hf_eq_ctrl_ss")) ||
			(!strcmp(name, "ssrx_mf_eq_ctrl_ssp")) ||
			(!strcmp(name, "ssrx_hf_eq_ctrl_ssp")) ||
			(!strcmp(name, "ssrx_dfe1_tap_ctrl"))) {
				usbdp_cal_reg_wr(0x08, regs_base + 0x0a34);
				usbdp_cal_reg_wr(0x08, regs_base + 0x1a34);
				usbdp_cal_reg_wr(0x18, regs_base + 0x0a34);
				usbdp_cal_reg_wr(0x18, regs_base + 0x1a34);
				usbdp_cal_reg_wr(0x1e, regs_base + 0x0a34);
				usbdp_cal_reg_wr(0x1e, regs_base + 0x1a34);
	}
}

void phy_exynos_g2_usbdp_tune(struct exynos_usbphy_info *info)
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
		phy_exynos_usbdp_g2_tune_each(info, para_name, val);
	}
}

int phy_exynos_usbdp_g2_check_pll_lock(struct exynos_usbphy_info *info)
{

	void __iomem *regs_base = info->regs_base;
	u8 reg0350;
	//u8 reg0354;
	int cnt;

	for (cnt = 1000; cnt != 0; cnt--) {
		reg0350 = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_CMN_A0350);
		if ((reg0350 & USBDP_CMN_A0350_ANA_LCPLL_LOCK_DONE))
			break;
		udelay(1);
	}

	if (!cnt)
		return -1;
	dev_info(info->dev, "LC PLL Lock done\n");

	return 0;
}

void phy_exynos_usbdp_g2_run_lane(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	u8 reg;

#if defined(CONFIG_SOC_EXYNOS9820_EVT0)
	/* minimize Long chaneel RX data loss : 2018.03.20 */
	reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_08DC);
	reg &= ~USBDP_TRSV_0237_OVRD_LN0_RX_CTLE_EN;
	reg &= ~USBDP_TRSV_0237_LN0_RX_CTLE_EN;
	reg &= ~USBDP_TRSV_0237_OVRD_LN0_RX_CTLE_OC_EN;
	reg &= ~USBDP_TRSV_0237_LN0_RX_CTLE_OC_EN;
	reg &= ~USBDP_TRSV_0237_OVRD_LN0_RX_CTLE_MF_FWD_EN;
	reg |= USBDP_TRSV_0237_LN0_RX_CTLE_MF_FWD_EN;
	reg |= USBDP_TRSV_0237_LN0_ANA_RX_CTLE_HF_EN;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_08DC);

	reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_18DC);
	reg &= ~USBDP_TRSV_0637_OVRD_LN2_RX_CTLE_EN;
	reg &= ~USBDP_TRSV_0637_LN2_RX_CTLE_EN;
	reg &= ~USBDP_TRSV_0637_OVRD_LN2_RX_CTLE_OC_EN;
	reg &= ~USBDP_TRSV_0637_LN2_RX_CTLE_OC_EN;
	reg &= ~USBDP_TRSV_0637_OVRD_LN2_RX_CTLE_MF_FWD_EN;
	reg |= USBDP_TRSV_0637_LN2_RX_CTLE_MF_FWD_EN;
	reg |= USBDP_TRSV_0637_LN2_ANA_RX_CTLE_HF_EN;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_18DC);

	reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_09AC);
	//reg &= USBDP_TRSV_09AC_LN0_ANA_RX_SQ_VREF_820M_SEL_CLR;
	reg &= ~USBDP_TRSV_09AC_OVRD_LN0_RX_SQHS_EN;
	reg &= ~USBDP_TRSV_09AC_LN0_RX_SQHS_EN;
	reg &= ~USBDP_TRSV_09AC_OVRD_LN0_RX_SQHS_DIFN_OC_EN;
	reg &= ~USBDP_TRSV_09AC_LN0_RX_SQHS_DIFN_OC_EN;
	reg &= ~USBDP_TRSV_09AC_LN0_ANA_RX_SQHS_DIFN_OC_CODE_SIGN;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_09AC);

	reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_19AC);
	reg &= USBDP_TRSV_066B_LN2_ANA_RX_SQ_VREF_820M_SEL_CLR;
	reg &= ~USBDP_TRSV_066B_OVRD_LN2_RX_SQHS_EN;
	reg &= ~USBDP_TRSV_066B_LN2_RX_SQHS_EN;
	reg &= ~USBDP_TRSV_066B_OVRD_LN2_RX_SQHS_DIFN_OC_EN;
	reg &= ~USBDP_TRSV_066B_LN2_RX_SQHS_DIFN_OC_EN;
	reg &= ~USBDP_TRSV_066B_LN2_ANA_RX_SQHS_DIFN_OC_CODE_SIGN;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_19AC);
	/* 2018.03.20 */
#endif

	reg = 0;
	reg &= ~USBDP_CMN_A031C_PCS_TX_DRV_EN;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A031C);

	reg = 0;
	reg &= ~USBDP_CMN_A0328_PCS_TX_ELECIDLE;
	reg &= ~USBDP_CMN_A0328_PCS_TX_DRV_PRE_LVL_CTRL_MSK;
	reg &= ~USBDP_CMN_A0328_OVRD_PCS_TX_DRV_PRE_LVL_CTRL;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0328);

	reg = 0;
	reg &= ~USBDP_CMN_A032C_PCS_TX_SER_EN;
	reg &= ~USBDP_CMN_A032C_PCS_RX_LPFS_EN;
	reg &= ~USBDP_CMN_A032C_PCS_TX_RCV_DET_EN;
	reg &= ~USBDP_CMN_A032C_PCS_TX_LPFS_EN;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A032C);
}

static void phy_exynos_usbdp_g2_set_pcs(struct exynos_usbphy_info *info)
{
	u32 reg;
	void __iomem *regs_base = info->regs_base_2nd;

	/* Change Elastic buffer threshold */
	reg = usbdp_cal_reg_rd(regs_base + USBDP_GEN2_PCSREG_EBUF_PARAM);
	reg &= ~(USBDPG2_PCS_NUM_INIT_BUFFERING_MSK |
			USBDPG2_PCS_SKP_REMOVE_TH_MSK);
	reg |= USBDPG2_PCS_SKP_REMOVE_TH_SET(0x1);
	usbdp_cal_reg_wr(reg, regs_base + USBDP_GEN2_PCSREG_EBUF_PARAM);

	/* Change Elastic buffer Drain param
	 * -> enable TSEQ Mask
	 */
	reg = usbdp_cal_reg_rd(regs_base + USBDP_GEN2_PCSREG_EBUF_DRAINER_PARAM);
	reg |= USBDP_GEN2_PCSREG_EN_MASK_TSEQ;
	usbdp_cal_reg_wr(reg, regs_base + USBDP_GEN2_PCSREG_EBUF_DRAINER_PARAM);

	/* abnormal comman pattern mask */
	reg = usbdp_cal_reg_rd(regs_base + USBDP_GEN2_PCSREG_BACK_END_MODE_VEC);
	reg &= ~USBDPG2_PCS_DISABLE_DATA_MASK_MSK;
	usbdp_cal_reg_wr(reg, regs_base + USBDP_GEN2_PCSREG_BACK_END_MODE_VEC);

	/* De-serializer enabled when U2 */
	reg = usbdp_cal_reg_rd(regs_base + USBDP_GEN2_PCSREG_OUT_VEC_2);
	reg &= ~USBDP_GEN2_PCSREG_DYN_CON_DESERIAL;
	reg |= USBDP_GEN2_PCSREG_SEL_OUT_DESERIAL;
	usbdp_cal_reg_wr(reg, regs_base + USBDP_GEN2_PCSREG_OUT_VEC_2);
	//usbdp_cal_reg_wr(0x51255, 0x10af014c);

	/* TX Keeper Disable when U3 */
	reg = usbdp_cal_reg_rd(regs_base + USBDP_GEN2_PCSREG_OUT_VEC_3);
	reg &= ~USBDP_GEN2_PCSREG_DYN_CON_PMA_TX_KEEPER;
	reg |= USBDP_GEN2_PCSREG_SEL_OUT_PMA_TX_KEEPER;
	usbdp_cal_reg_wr(reg, regs_base + USBDP_GEN2_PCSREG_OUT_VEC_3);
}

#define SIGVAL_FILTER_VAL	0x0

void phy_exynos_usbdp_g2_enable(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	u32 reg;


#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
		/* { 2018.07.05 Power Optimization Code */
		reg = USBDP_CMN_A0830_LN0_TX_JEQ_EVEN_CTRL_SP_SET(0x7);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0830);
		reg = USBDP_CMN_A1830_LN2_TX_JEQ_EVEN_CTRL_SP_SET(0x7);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A1830);

		reg = USBDP_TRSV_A085C_LN0_TX_LANE_LC_RO_CLK_SEL_SP;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_A085C);
		reg = USBDP_TRSV_A185C_OVRD_LN2_TX_SER_VREG_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_A185C);

		reg = USBDP_TRSV_R040C_LN1_TX_JEQ_EVEN_CTRL_SP_SET(0x7);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R040C_1030);
		reg = USBDP_CMN_A1830_LN2_TX_JEQ_EVEN_CTRL_SP_SET(0x7);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A2030);

		reg = USBDP_TRSV_A105C_OVRD_LN1_TX_SER_VREG_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_A105C);
		reg = USBDP_TRSV_A205C_OVRD_LN3_TX_SER_VREG_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_A205C);

		reg = 0;
		reg |= USBDP_CMN_A0228_ANA_ROPLL_CD_HSCLK_WEST_EN;
		reg |= USBDP_CMN_A0228_ANA_ROPLL_CD_HSCLK_EAST_EN;
		reg |= USBDP_CMN_A0228_OVRD_ROPLL_CD_VREG_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0228);

		reg = 0;
		reg |= USBDP_CMN_R041_PLL_CD_RSTN_SEL_EAST;
		reg |= USBDP_CMN_R041_PLL_CD_RSTN_SEL_WAST;
		reg |= USBDP_CMN_R041_PLL_CD_HSCLK_WEST_CTRL_SET(0x0);
		reg |= USBDP_CMN_R041_PLL_CD_HSCLK_EAST_CTRL_SET(0x0);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R041_0104);

		reg = 0;
		reg |= USBDP_CMN_R093_PLL_CD_RSTN_SEL_EAST;
		reg |= USBDP_CMN_R093_PLL_CD_RSTN_SEL_WAST;
		reg |= USBDP_CMN_R093_PLL_CD_HSCLK_WEST_CTRL_SET(0x0);
		reg |= USBDP_CMN_R093_PLL_CD_HSCLK_EAST_CTRL_SET(0x0);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R093_248);
		/* } 2018.07.05 Power Optimization Code */

		/* UE_TASK: Check DP Reset and CDR Watchdog en
		 * usbdp_cal_reg_wr(0x02, regs_base + 0x038c);
		 */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_CMN_A038C);
		reg &= ~USBDP_CMN_A038C_DP_INIT_RSTN;
		reg &= ~USBDP_CMN_A038C_DP_CMN_RSTN;
		reg |= USBDP_CMN_A038C_CDR_WATCHDOG_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A038C);

		/* UE_TASK: ln0 / ln2 resered port
		 * usbdp_cal_reg_wr(0x04, regs_base + 0x0878);
		 * usbdp_cal_reg_wr(0x04, regs_base + 0x1878);
		 */
		reg = 0;
		usbdp_cal_reg_wr(0x04, regs_base + EXYNOS_USBDP_TRSV_0878);
		usbdp_cal_reg_wr(0x04, regs_base + EXYNOS_USBDP_TRSV_1878);

		/* UE_TASK: ln0 / ln2 rx rtem incm ctrl
		 * usbdp_cal_reg_wr(0x07, regs_base + 0x09a4);
		 * usbdp_cal_reg_wr(0x07, regs_base + 0x19a4);
		 */
		reg = 0;
		reg |= USBDP_TRSV_A09A4_LN0_ANA_RX_RTERM_INCM_SW_CTRL_SET(0x1);
		reg |= USBDP_TRSV_A09A4_LN0_ANA_RX_RTERM_INCM_VCM_CTRL_SET(0x3);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_A09A4);
		reg = 0;
		reg |= USBDP_TRSV_A19A4_LN2_ANA_RX_RTERM_INCM_SW_CTRL_SET(0x1);
		reg |= USBDP_TRSV_A19A4_LN2_ANA_RX_RTERM_INCM_VCM_CTRL_SET(0x3);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_A19A4);

		/* UE_TASK: ln0 / ln2 rx rterm vcm
		 * usbdp_cal_reg_wr(0x22, regs_base + 0x09a8);
		 * usbdp_cal_reg_wr(0x22, regs_base + 0x19a8);
		 */
		reg = 0;
		reg |= USBDP_TRSV_R026A_LN0_ANA_RX_RTERM_INCM_VCM_CTRL_SET(0x1);
		reg |= USBDP_TRSV_R026A_LN0_RX_RTERM_VCM_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R026A_09A8);
		reg = 0;
		reg |= USBDP_TRSV_R66A_LN2_ANA_RX_RTERM_INCM_VCM_CTRL_SET(0x1);
		reg |= USBDP_TRSV_R66A_LN2_RX_RTERM_VCM_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R66A_19A8);


		/* UE_TASK: ln0 / ln2 rx dfe vga rl sp /ssp ctrl
		 * usbdp_cal_reg_wr(0x12, regs_base + 0x0994);
		 * usbdp_cal_reg_wr(0x12, regs_base + 0x1994);
		 */
		reg = 0;
		reg |= USBDP_TRSV_0265_LN0_RX_DFE_VGA_RL_CTRL_SP_SET(0x2);
		reg |= USBDP_TRSV_0265_LN0_RX_DFE_VGA_RL_CTRL_SSP_SET(0x2);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_0994);
		reg = 0;
		reg |= USBDP_TRSV_0665_LN2_RX_DFE_VGA_RL_CTRL_SP_SET(0x2);
		reg |= USBDP_TRSV_0665_LN2_RX_DFE_VGA_RL_CTRL_SSP_SET(0x2);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_1994);

		/* UE_TASK: ln0 / ln2 rx dfe vga rl ctrl rbr,hbr
		 * usbdp_cal_reg_wr(0x12, regs_base + 0x0998);
		 * usbdp_cal_reg_wr(0x12, regs_base + 0x1998);
		 */
		reg = 0;
		reg |= USBDP_TRSV_R0266_LN0_RX_DFE_VGA_RL_CTRL_RBR_SET(0x2);
		reg |= USBDP_TRSV_R0266_LN0_RX_DFE_VGA_RL_CTRL_HBR_SET(0x2);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R0266_0998);
		reg = 0;
		reg |= USBDP_TRSV_R666_LN2_RX_DFE_VGA_RL_CTRL_RBR_SET(0x2);
		reg |= USBDP_TRSV_R666_LN2_RX_DFE_VGA_RL_CTRL_HRB_SET(0x2);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R666_1998);

		/* UE_TASK: ln0 / ln2 rx dfe vga rl ctrl hbr2, hbr3
		 * usbdp_cal_reg_wr(0x48, regs_base + 0x099c);
		 * usbdp_cal_reg_wr(0x48, regs_base + 0x199c);
		 */
		reg = 0;
		reg |= USBDP_TRSV_R0267_LN0_RX_DFE_VGA_RL_CTRL_HBR2_SET(0x2);
		reg |= USBDP_TRSV_R0267_LN0_RX_DFE_VGA_RL_CTRL_HBR3_SET(0x2);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R0267_099C);
		reg = 0;
		reg |= USBDP_TRSV_R667_LN2_RX_DFE_VGA_RL_CTRL_HRB2_SET(0x2);
		reg |= USBDP_TRSV_R667_LN2_RX_DFE_VGA_RL_CTRL_HRB3_SET(0x2);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R667_199C);

		/* UE_TASK: ln0 / ln2 rx cdr_mdiv_sel_pll sp, ssp
		 * usbdp_cal_reg_wr(0x77, regs_base + 0x0898);
		 * usbdp_cal_reg_wr(0x77, regs_base + 0x1898);
		 */
		reg = 0;
		reg |= USBDP_TRSV_0226_LN0_RX_CDR_MDIV_SEL_PLL_SP_SET(0x7);
		reg |= USBDP_TRSV_0226_LN0_RX_CDR_MDIV_SEL_PLL_SSP_SET(0x7);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_0898);

		reg = 0;
		reg |= USBDP_TRSV_0626_LN2_RX_CDR_MDIV_SEL_PLL_SP_SET(0x7);
		reg |= USBDP_TRSV_0626_LN2_RX_CDR_MDIV_SEL_PLL_SSP_SET(0x7);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_1898);

		/* UE_TASK: lcpll avc force enable
		 * usbdp_cal_reg_wr(0x01, regs_base + 0x0054);
		 */
		reg = 0;
		reg |= USBDP_CMN_015_ANA_LCPLL_AVC_FORCE_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_015_0054);

		/* UE_TASK: lcpll cd vreg en, hsclk east/west en
		 * usbdp_cal_reg_wr(0x38, regs_base + 0x00e0);
		 */
		reg = 0;
		reg |= USBDP_CMN_038_ANA_LCPLL_CD_HSCLK_WEST_EN;
		reg |= USBDP_CMN_038_ANA_LCPLL_CD_HSCLK_EAST_EN;
		reg |= USBDP_CMN_038_OVRD_LCPLL_CD_VREG_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R038_00E0);

		/* 2018.06.28 enhanced pll stability */
		reg = 0;
		reg &= ~USBDP_CMN_0018_LCPLL_ANA_CPI_CTRL_COARSE_MSK;
		reg |= USBDP_CMN_0018_LCPLL_ANA_CPI_CTRL_COARSE_SET(0x4);
		reg &= ~USBDP_CMN_0018_LCPLL_ANA_CPI_CTRL_FINE_MSK;
		reg |= USBDP_CMN_0018_LCPLL_ANA_CPI_CTRL_FINE_SET(0x4);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_0060);

		reg = 0;
		reg &= ~USBDP_CMN_0019_LCPLL_ANA_CPP_CTRL_COARSE_MSK;
		reg |= USBDP_CMN_0019_LCPLL_ANA_CPP_CTRL_COARSE_SET(0x7);
		reg &= ~USBDP_CMN_0019_LCPLL_ANA_CPP_CTRL_FINE_MSK;
		reg |= USBDP_CMN_0019_LCPLL_ANA_CPP_CTRL_FINE_SET(0x7);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_0064);

		reg = 0;
		reg &= ~USBDP_CMN_001C_LCPLL_ANA_LPF_C_SEL_FINE_MSK;
		reg |= USBDP_CMN_001C_LCPLL_ANA_LPF_C_SEL_FINE_SET(0x7);
		reg &= ~USBDP_CMN_001C_LCPLL_ANA_LPF_R_SEL_COARSE_MSK;
		reg |= USBDP_CMN_001C_LCPLL_ANA_LPF_R_SEL_COARSE_SET(0x6);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_0070);
		/* ***************************************************** */

#else
		/* Change Term */
		reg = USBDP_CMN_008_ANA_AUX_TX_TERM_SET(0x4);
		reg |= USBDP_CMN_008_ANA_AUX_RX_TERM_SET(0x8);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_008_0020);

		/* UE_TASK: Wrtie Value same as Reset Value */
		reg = USBDP_CMN_014_ANA_LCPLL_AVC_CNT_RUN_NUM_SET(0x4);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_014_0050);

		/* for tSSC, not use cal data but forcing 2018.04.06 */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_CMN_015_0054);
		reg |= USBDP_CMN_015_ANA_LCPLL_AVC_FORCE_EN;
		reg &= ~USBDP_CMN_015_ANA_LCPLL_AVC_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_015_0054);

		//usbdp_cal_reg_wr(0x02, regs_base + 0x0054); // 04.06 02 -->01

		/* UE_TASK: Wrtie Value same as Reset Value */
		reg = USBDP_CMN_016_ANA_LCPLL_AVC_VCI_MAX_SEL_SET(0x6);
		reg |= USBDP_CMN_016_ANA_LCPLL_AVC_MAN_CAP_BIAS_CODE_SET(0x4);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_016_0058);

		/* UE_TASK: Wrtie Value same as Reset Value */
		reg = USBDP_CMN_017_ANA_LCPLL_AVC_VCI_MIN_SEL_SET(0x2);
		reg |= USBDP_CMN_017_ANA_LCPLL_AVC_VCI_MID_SEL_SET(0x4);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_017_005C);

		/* Change LCPLL */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_CMN_R01D_0074);
		reg &= ~USBDP_CMN_01D_LCPLL_ANA_LPF_R_SEL_FINE_MASK;
		reg |= USBDP_CMN_01D_LCPLL_ANA_LPF_R_SEL_FINE_SET(0x3);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R01D_0074);

		/* UE_TASK: Wrtie Value same as Reset Value */
		reg = USBDP_CMN_024_ANA_LCPLL_PMS_MDIV_SET(0x60);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R024_0090);

		/* UE_TASK: Wrtie Value same as Reset Value */
		reg = USBDP_CMN_025_ANA_LCPLL_PMS_MDIV_AFC_SET(0x60);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R025_0094);

		/* UE_TASK: Wrtie Value same as Reset Value */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_CMN_R031_00C4);
		reg &= ~USBDP_CMN_031_ANA_LCPLL_SDM_PH_NUM_SEL;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R031_00C4);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_CMN_A012C);
		reg &= ~USBDP_CMN_A012C_ROPLL_ANA_CPP_CTRL_FINE_MASK;
		reg |= USBDP_CMN_A012C_ROPLL_ANA_CPP_CTRL_FINE_SET(0x4);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A012C);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_CMN_A0134);
		reg &= ~USBDP_CMN_A0134_ROPLL_ANA_LPF_R_SEL_FINE_MASK;
		reg |= USBDP_CMN_A0134_ROPLL_ANA_LPF_R_SEL_FINE_SET(0x4);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0134);

		reg = USBDP_CMN_A015C_ROPLL_PMS_MDIV_RBR_SET(0x4e);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A015C);

		reg = USBDP_CMN_A0174_ROPLL_PMS_MDIV_AFC_RBR_SET(0x4e);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0174);

		reg = USBDP_CMN_A018C_ROPLL_PMS_SDIV_RBR_SET(0x4);
		reg |= USBDP_CMN_A018C_ROPLL_PMS_SDIV_HBR_SET(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A018C);

		reg = USBDP_CMN_A01AC_ROPLL_SDM_DENOM_RBR_SET(0x2c);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A01AC);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_CMN_A01BC);
		reg |= USBDP_CMN_A01BC_ROPLL_SDM_NUMERATOR_SIGN_RBR;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A01BC);

		reg = USBDP_CMN_A01C8_ROPLL_SDM_NUMERATOR_RBR_SET(0x8);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A01C8);

		reg = USBDP_CMN_A01F0_ROPLL_SDC_NUMERATOR_RBR_SET(0x7);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A01F0);

		reg = USBDP_CMN_A0208_ROPLL_SDC_DENOMINATOR_RBR_SET(0x16);
		usbdp_cal_reg_wr(0x16, regs_base + EXYNOS_USBDP_COM_CMN_A0208);

		reg = USBDP_CMN_A021C_ANA_ROPLL_SSC_FM_DEVIATION_SET(0x19);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A021C);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_CMN_A0220);
		reg &= ~USBDP_CMN_A0220_ANA_ROPLL_SSC_FM_FREQ_MASK;
		reg |= USBDP_CMN_A0220_ANA_ROPLL_SSC_FM_FREQ_SET(0xf);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0220);

		/* SSC enable */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_CMN_R00B4_02D0);
		reg &= ~USBDP_CMN_R00B4_CDR_LOCK_DELAY_CODE_MASK;
		reg &= ~USBDP_CMN_R00B4_RX_OC_DONE_DELAY_CODE_MASK;
		reg |= USBDP_CMN_R00B4_CDR_LOCK_DELAY_CODE_SET(0x2);
		reg |= USBDP_CMN_R00B4_RX_OC_DONE_DELAY_CODE_SET(0x2);

		reg &= ~USBDP_CMN_R00B4_SSC_EN; //20180630 ows
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R00B4_02D0);

		reg = USBDP_TRSV_R0235_LN0_RX_CDR_VCO_FREQ_BOOST_RBR_SET(0x1);
		reg |= USBDP_TRSV_R0235_LN0_RX_CDR_VCO_FREQ_BOOST_HBR_SET(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R0235_08CC);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R0244_0910);
		reg &= ~USBDP_TRSV_R0244_LN0_RX_CTLE_I_MF_FWD_CTRL_HBR3_MSK;
		reg &= ~USBDP_TRSV_R0244_LN0_RX_CTLE_I_HF_CTRL_SP_MSK;
		reg |= USBDP_TRSV_R0244_LN0_RX_CTLE_I_HF_CTRL_SP_SET(0x1);
		reg |= USBDP_TRSV_R0244_LN0_RX_CTLE_I_MF_FWD_CTRL_HBR3_SET(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R0244_0910);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R0644_1910);
		reg &= ~USBDP_TRSV_R0644_LN2_RX_CTLE_I_MF_FWD_CTRL_HBR3_MSK;
		reg &= ~USBDP_TRSV_R0644_LN2_RX_CTLE_I_HF_CTRL_SP_MSK;
		reg |= USBDP_TRSV_R0644_LN2_RX_CTLE_I_HF_CTRL_SP_SET(0x1);
		reg |= USBDP_TRSV_R0644_LN2_RX_CTLE_I_MF_FWD_CTRL_HBR3_SET(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R0644_1910);


		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_0994);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_0994);

		reg = USBDP_TRSV_R0266_LN0_RX_DFE_VGA_RL_CTRL_HBR_SET(0x7);
		reg |= USBDP_TRSV_R0266_LN0_RX_DFE_VGA_RL_CTRL_RBR_SET(0x7);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R0266_0998);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R0267_099C);
		reg &= ~USBDP_TRSV_R0267_LN0_ANA_RX_DFE_VGA_PBIAS_CTRL_MSK;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R0267_099C);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R026A_09A8);
		reg |= USBDP_TRSV_R026A_OVRD_LN0_RX_RTERM_CM_PULLDN;
		reg |= USBDP_TRSV_R026A_LN0_RX_RTERM_CM_PULLDN;
		reg |= USBDP_TRSV_R026A_OVRD_LN0_RX_RTERM_VCM_EN;
		reg &= ~USBDP_TRSV_R026A_LN0_RX_RTERM_VCM_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R026A_09A8);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R027E_09F8);
		reg |= ~USBDP_TRSV_R027E_LN0_RX_SSLMS_HF_INIT_MSK;
		reg |= USBDP_TRSV_R027E_LN0_RX_SSLMS_HF_INIT_SET(0xc);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R027E_09F8);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R27F_09FC);
		reg &= ~USBDP_TRSV_R27F_LN0_RX_SSLMS_MF_INIT_MSK;
		reg |= USBDP_TRSV_R27F_LN0_RX_SSLMS_MF_INIT_SET(0xc);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R27F_09FC);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R028B_0A2C);
		reg &= ~USBDP_TRSV_R028B_LN0_RX_SSLMS_ADPA_COEF_SEL8;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R028B_0A2C);

		reg = USBDP_TRSV_R20BE_LN0_TX_RCAL_UP_OPT_CODE_SET(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R02BE_0AF8);

		reg = USBDP_TRSV_R20BF_LN0_TX_RCAL_UP_CODE_SET(0x3);
		reg |= USBDP_TRSV_R20BF_LN0_TX_RCAL_DN_OPT_CODE_SET(0x1);
		usbdp_cal_reg_wr(0x13, regs_base + EXYNOS_USBDP_TRSV_R02BF_0AFC);

		reg = USBDP_TRSV_R02C0_LN0_TX_RCAL_DN_CODE_SET(0x3);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R02C0_0B00);

		reg = USBDP_TRSV_R41E_LN0_RX_DFE_ADD_DIS;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R41E_1078);

		reg = USBDP_TRSV_R607_LN2_ANA_TX_DRV_ACCDRV_EN;
		reg |= USBDP_TRSV_R607_LN2_ANA_TX_DRV_IDRV_VREF_SEL;
		reg |= USBDP_TRSV_R607_LN2_ANA_TX_DRV_IDRV_IUP_CTRL_SET(0x7);
		reg |= USBDP_TRSV_R607_LN2_ANA_TX_DRV_IDRV_IDN_CTRL_SET(0x0);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R607_181C);

		reg = USBDP_TRSV_R633_LN2_RX_CDR_VCO_FREQ_BOOST_RBR_SET(0x1);
		reg |= USBDP_TRSV_R633_LN2_RX_CDR_VCO_FREQ_BOOST_HBR_SET(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R633_18CC);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R66A_19A8);
		reg |= USBDP_TRSV_R66A_OVRD_LN2_ANA_RX_RTERM_CM_PULLDN;
		reg |= USBDP_TRSV_R66A_LN2_ANA_RX_RTERM_CM_PULLDN;
		reg |= USBDP_TRSV_R66A_OVRD_LN2_RX_RTERM_VCM_EN;
		reg &= ~USBDP_TRSV_R66A_LN2_RX_RTERM_VCM_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R66A_19A8);

		reg = USBDP_TRSV_R67E_LN2_RX_SSLMS_HF_INIT_SET(0xc);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R67E_19F8);

		reg = USBDP_TRSV_R67F_LN2_RX_SSLMS_MF_INIT_SET(0xc);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R67F_19FC);

		usbdp_cal_reg_wr(0x00, regs_base + EXYNOS_USBDP_TRSV_R68B_1A2C);

		reg = USBDP_TRSV_R6BE_LN2_TX_RCAL_UP_OPT_CODE_SET(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R6BE_1AF8);

		reg = USBDP_TRSV_R6BF_LN2_TX_RCAL_DN_OPT_CODE_SET(0x1);
		reg |= USBDP_TRSV_R6BF_LN2_TX_RCAL_UP_CODE_SET(0x3);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R6BF_1AFC);

		reg = USBDP_TRSV_R6C0_LN2_TX_RCAL_DN_CODE_SET(0x3);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R6C0_1B00);

		reg = USBDP_TRSV_R41E_LN2_RX_DFE_ADD_DIS;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R81E_2078);

		/* update ATE vector test fixed : 20180309 */

		/* UE_TASK: Check LCPLL value
		 * 1. usbdp_cal_reg_wr(0x77, regs_base + 0x0104);
		 * 2. for power saving
		 *    reg = USBDP_CMN_A0104_ANA_LCPLL_RSVD_SET(0x44);
		 *    usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R041_0104);
		 */
		reg = USBDP_CMN_R041_PLL_CD_RSTN_SEL_EAST;
		reg |= USBDP_CMN_R041_PLL_CD_RSTN_SEL_WAST;

		reg |= USBDP_CMN_R041_PLL_CD_HSCLK_WEST_CTRL_SET(0x0);
		reg |= USBDP_CMN_R041_PLL_CD_HSCLK_EAST_CTRL_SET(0x0);

		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R041_0104);

		/* UE_TASK: Check ROPLL value
		 * 1. usbdp_cal_reg_wr(0x77, regs_base + 0x0248);
		 * 2. for power saving
		 *    reg = USBDP_CMN_A0248_ANA_ROPLL_RSVD_SET(0x44);
		 *    usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0248);
		 */
		reg = USBDP_CMN_R093_PLL_CD_RSTN_SEL_EAST;
		reg |= USBDP_CMN_R093_PLL_CD_RSTN_SEL_WAST;

		reg |= USBDP_CMN_R093_PLL_CD_HSCLK_WEST_CTRL_SET(0x0);
		reg |= USBDP_CMN_R093_PLL_CD_HSCLK_EAST_CTRL_SET(0x0);

		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R093_248);

		/* UE_TASK: Check DP Reset and CDR Watchdog en
		 * usbdp_cal_reg_wr(0x02, regs_base + 0x038c);
		 */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_CMN_A038C);
		reg &= ~USBDP_CMN_A038C_DP_INIT_RSTN;
		reg &= ~USBDP_CMN_A038C_DP_CMN_RSTN;
		reg |= USBDP_CMN_A038C_CDR_WATCHDOG_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A038C);

		/* RX LFPS 50MHz support */
		/* UE_TASK: Check Reset Value is 0x60
		 * usbdp_cal_reg_wr(0x60, regs_base + 0x09d0);
		 * usbdp_cal_reg_wr(0x60, regs_base + 0x19d0);*/


		/* tx_drv_lfps_mode_idrv_iup_ctrl = 0x7
		 * tx_drv_lfps_mode_idrv_idn_ctrl = 0x0
		 */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_CMN_R0EA_03A8);
		reg &= ~USBDP_CMN_R0EA_TX_DRV_LFPS_MODE_IDRV_IUP_CTRL_MSK;
		reg |= USBDP_CMN_R0EA_TX_DRV_LFPS_MODE_IDRV_IUP_CTRL_SET(0x7);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R0EA_03A8);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_CMN_R0EB_03AC);
		reg &= ~USBDP_CMN_R0EV_TX_DRV_LFPS_MODE_IDRV_IDN_CTRL_MSK;
		reg |= USBDP_CMN_R0EV_TX_DRV_LFPS_MODE_IDRV_IDN_CTRL_SET(0x0);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R0EB_03AC);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_CMN_R207_081C);
		reg &= ~USBDP_CMN_R207_LN0_ANA_TX_DRV_IDRV_IDN_CTRL_MSK;
		reg |= USBDP_CMN_R207_LN0_ANA_TX_DRV_IDRV_IDN_CTRL_SET(0x0);
		reg &= ~USBDP_CMN_R207_LN0_ANA_TX_DRV_IDRV_IUP_CTRL_MSK;
		reg |= USBDP_CMN_R207_LN0_ANA_TX_DRV_IDRV_IUP_CTRL_SET(0x7);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_R207_081C);

		/* default TUNE: selection tuning parameter basis */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_028C);
		reg &= USBDP_TRSV_028C_LN0_RX_SSLMS_ADAP_COEF_SEL_CLR;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_028C);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_068C);
		reg &= USBDP_TRSV_068C_LN2_RX_SSLMS_ADAP_COEF_SEL_CLR;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_068C);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_02BD);
		reg |= USBDP_TRSV_02BD_LN0_RX_RCAL_OPT_CODE(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_02BD);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_06BD);
		reg |= USBDP_TRSV_06BD_LN2_RX_RCAL_OPT_CODE(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_06BD);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0404);

		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0404);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0804);

		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0804);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0405);

		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0405);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0805);

		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0805);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0406);

		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0406);
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_0806);

		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0806);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_041F);
		reg &= USBDP_TRSV_041F_LN1_TX_RCAL_UP_OPT_CODE_CLR;
		reg |= USBDP_TRSV_041F_LN1_TX_RCAL_UP_OPT_CODE(0x1);
		reg &= USBDP_TRSV_041F_LN1_TX_RCAL_DN_OPT_CODE_CLR;
		reg |= USBDP_TRSV_041F_LN1_TX_RCAL_DN_OPT_CODE(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_041F);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_081F);
		reg &= USBDP_TRSV_081F_LN3_TX_RCAL_UP_OPT_CODE_CLR;
		reg |= USBDP_TRSV_081F_LN3_TX_RCAL_UP_OPT_CODE(0x1);
		reg &= USBDP_TRSV_081F_LN3_TX_RCAL_DN_OPT_CODE_CLR;
		reg |= USBDP_TRSV_081F_LN3_TX_RCAL_DN_OPT_CODE(0x1);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_081F);

		/* 2018.06.28 enhanced pll stability */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_CMN_0060);
		reg &= ~USBDP_CMN_0018_LCPLL_ANA_CPI_CTRL_COARSE_MSK;
		reg |= USBDP_CMN_0018_LCPLL_ANA_CPI_CTRL_COARSE_SET(0x4);
		reg &= ~USBDP_CMN_0018_LCPLL_ANA_CPI_CTRL_FINE_MSK;
		reg |= USBDP_CMN_0018_LCPLL_ANA_CPI_CTRL_FINE_SET(0x4);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_0060);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_CMN_0064);
		reg &= ~USBDP_CMN_0019_LCPLL_ANA_CPP_CTRL_COARSE_MSK;
		reg |= USBDP_CMN_0019_LCPLL_ANA_CPP_CTRL_COARSE_SET(0x7);
		reg &= ~USBDP_CMN_0019_LCPLL_ANA_CPP_CTRL_FINE_MSK;
		reg |= USBDP_CMN_0019_LCPLL_ANA_CPP_CTRL_FINE_SET(0x7);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_0064);

		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_CMN_0070);
		reg &= ~USBDP_CMN_001C_LCPLL_ANA_LPF_C_SEL_FINE_MSK;
		reg |= USBDP_CMN_001C_LCPLL_ANA_LPF_C_SEL_FINE_SET(0x7);
		reg &= ~USBDP_CMN_001C_LCPLL_ANA_LPF_R_SEL_COARSE_MSK;
		reg |= USBDP_CMN_001C_LCPLL_ANA_LPF_R_SEL_COARSE_SET(0x6);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_CMN_0070);

 /* else EVT0 */
#endif
	phy_exynos_g2_usbdp_tune(info);

	reg = 0;
	reg &= ~USBDP_CMN_A0310_PCS_PLL_EN;
	reg &= ~USBDP_CMN_A0310_PCS_LANE_RSTN;
	reg &= ~USBDP_CMN_A0310_PCS_INIT_RSTN;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0310);

	reg = 0;
	reg &= ~USBDP_CMN_A0308_PCS_BIAS_EN;
	reg &= ~USBDP_CMN_A0308_PCS_BGR_EN;
	reg &= ~USBDP_CMN_A0308_PCS_PM_STATE_MASK;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0308);

	phy_exynos_usbdp_g2_set_pcs(info);

	/* hot temp fail test : 0412 */
	/* CDR FBB Control */
	//reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R2A2_0A88);
	//reg |= USBDP_TRSV_R2A2_LN0_RX_CDR_FBB_MAN_SEL;
	//usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R2A2_0A88);

	reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R2A6_0A98);
	reg &= ~USBDP_TRSV_R2A6_LN0_RX_CDR_FBB_FINE_CTRL_MSK;
	reg |= USBDP_TRSV_R2A6_LN0_RX_CDR_FBB_FINE_CTRL_SET(CDR_FBB_FINE_VAL);
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R2A6_0A98);

	//reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R6A2_1A88);
	//reg |= USBDP_TRSV_R6A2_LN2_RX_CDR_FBB_MAN_SEL;
	//usbdp_cal_reg_wr(0x05, regs_base + EXYNOS_USBDP_TRSV_R6A2_1A88);

	reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_R6A6_1A98);
	reg &= ~USBDP_TRSV_R6A6_LN2_RX_CDR_FBB_FINE_CTRL_MSK;
	reg |= USBDP_TRSV_R6A6_LN2_RX_CDR_FBB_FINE_CTRL_SET(CDR_FBB_FINE_VAL);
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_R6A6_1A98);


	if (info->used_phy_port == 0) {
		reg = 0;
		reg &= ~USBDP_CMN_A0258_OVRD_CMN_RATE;
		reg &= ~USBDP_CMN_A0258_CMN_LCPLL_ALONE_MODE; // usb rx0, tx1, dp tx2, tx3
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0258);

#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
		reg = 0;
		reg |= USBDP_CMN_A0288_DP_LANE_EN_SET(0x0);
		reg |= USBDP_CMN_A0288_LANE_MUX_SEL_DP_SET(0xc);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0288);
#else
		reg = 0;
		reg |= USBDP_CMN_A0288_DP_LANE_EN_SET(0xc);
		reg |= USBDP_CMN_A0288_LANE_MUX_SEL_DP_SET(0xc);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0288);
#endif

#if defined(CONFIG_SOC_EXYNOS9820_EVT0)
		reg = 0;
		reg |= USBDP_CMN_A0310_OVRD_PCS_PLL_EN;
		reg |= USBDP_CMN_A0310_PCS_PLL_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0310);
#endif

#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
		reg = 0;
		reg |= USBDP_CMN_A028C_OVRD_RX_CDR_DATA_MODE_EXIT;
		reg |= USBDP_CMN_A028C_RX_CDR_DATA_MODE_EXIT;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A028C);
#else
		reg = 0;
		reg &= ~USBDP_CMN_A028C_OVRD_RX_CDR_DATA_MODE_EXIT;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A028C);
#endif

		reg = 0;
		reg |= USBDP_TRSV_0400_LN1_ANA_TX_DRV_BEACON_LFPS_SYNC_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0400);

		reg = 0;
		reg |= USBDP_TRSV_0800_LN3_ANA_TX_DRV_BEACON_LFPS_SYNC_EN;
		reg |= USBDP_TRSV_0800_OVRD_LN3_TX_DRV_BEACON_LFPS_OUT_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0800);

		/* SIGVAL Filter for lane 0 */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_02BC);
		reg &= USBDP_TRSV_02BC_LN0_TG_RX_SIGVAL_LPF_DELAY_TIME_CLR;
		reg |= USBDP_TRSV_02BC_LN0_TG_RX_SIGVAL_LPF_DELAY_TIME_SET(SIGVAL_FILTER_VAL);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_02BC);
	} else {
		reg = 0;
		reg &= ~USBDP_CMN_A0258_OVRD_CMN_RATE;
		reg &= ~USBDP_CMN_A0258_CMN_LCPLL_ALONE_MODE; // dp rx0, tx1, usb tx2, tx3
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0258);

		reg = 0;
#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
		reg |= USBDP_CMN_A0288_DP_LANE_EN_SET(0x0);
#else
		reg |= USBDP_CMN_A0288_DP_LANE_EN_SET(0x3);
#endif
		reg |= USBDP_CMN_A0288_LANE_MUX_SEL_DP_SET(0x3);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0288);

#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
		if (reg == 0x33) { /* usbdp alt mode selectin case */
			if (exynos_soc_info.sub_rev == 0x0) {
				/* Apply WA in only EVT1.0
				 * UE_TASK: usbdp mux signal map error - W/A
				 * usbdp_cal_reg_wr(0x03, regs_base + 0x0310);
				 * usbdp_cal_reg_wr(0x04, regs_base + 0x0b04);
				 */
				reg = 0;
				reg |= USBDP_CMN_A0310_OVRD_PCS_PLL_EN;
				reg |= USBDP_CMN_A0310_PCS_PLL_EN;
				usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A0310);
				reg = 0;
				reg |= USBDP_TRSV_02C1_LN0_MISC_TX_CLK_SRC;
				usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_0B04);
			}
		}
#endif

#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
		reg = 0;
		reg |= USBDP_CMN_A028C_OVRD_RX_CDR_DATA_MODE_EXIT;
		reg |= USBDP_CMN_A028C_RX_CDR_DATA_MODE_EXIT;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A028C);
#else
		reg = 0;
		reg &= ~USBDP_CMN_A028C_OVRD_RX_CDR_DATA_MODE_EXIT;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_CMN_A028C);
#endif
		reg = 0;
		reg |= USBDP_TRSV_0400_LN1_ANA_TX_DRV_BEACON_LFPS_SYNC_EN;
		reg |= USBDP_TRSV_0400_OVRD_LN1_TX_DRV_BEACON_LFPS_OUT_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0400);

		reg = 0;
		reg |= USBDP_TRSV_0800_LN3_ANA_TX_DRV_BEACON_LFPS_SYNC_EN;
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_0800);

		/* SIGVAL Filter for lane 2 */
		reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_COM_TRSV_06BC);
		reg &= USBDP_TRSV_06BC_LN2_TG_RX_SIGVAL_LPF_DELAY_TIME_CLR;
		reg |= USBDP_TRSV_06BC_LN2_TG_RX_SIGVAL_LPF_DELAY_TIME_SET(SIGVAL_FILTER_VAL);
		usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_COM_TRSV_06BC);
	}

#if defined(CONFIG_SOC_EXYNOS9820_EVT0)
	/* minimize Long chaneel RX data loss : 2018.03.20 */
	reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_08DC);
	reg |= USBDP_TRSV_0237_OVRD_LN0_RX_CTLE_EN;
	reg |= USBDP_TRSV_0237_LN0_RX_CTLE_EN;
	reg &= ~USBDP_TRSV_0237_OVRD_LN0_RX_CTLE_OC_EN;
	reg &= ~USBDP_TRSV_0237_LN0_RX_CTLE_OC_EN;
	reg |= USBDP_TRSV_0237_OVRD_LN0_RX_CTLE_MF_FWD_EN;
	reg |= USBDP_TRSV_0237_LN0_RX_CTLE_MF_FWD_EN;
	reg |= USBDP_TRSV_0237_LN0_ANA_RX_CTLE_HF_EN;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_08DC);

	reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_18DC);
	reg |= USBDP_TRSV_0637_OVRD_LN2_RX_CTLE_EN;
	reg |= USBDP_TRSV_0637_LN2_RX_CTLE_EN;
	reg &= ~USBDP_TRSV_0637_OVRD_LN2_RX_CTLE_OC_EN;
	reg &= ~USBDP_TRSV_0637_LN2_RX_CTLE_OC_EN;
	reg |= USBDP_TRSV_0637_OVRD_LN2_RX_CTLE_MF_FWD_EN;
	reg |= USBDP_TRSV_0637_LN2_RX_CTLE_MF_FWD_EN;
	reg |= USBDP_TRSV_0637_LN2_ANA_RX_CTLE_HF_EN;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_18DC);

	reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_09AC);
	reg &= USBDP_TRSV_09AC_LN0_ANA_RX_SQ_VREF_820M_SEL_CLR;
	reg |= USBDP_TRSV_09AC_OVRD_LN0_RX_SQHS_EN;
	reg |= USBDP_TRSV_09AC_LN0_RX_SQHS_EN;
	reg &= ~USBDP_TRSV_09AC_OVRD_LN0_RX_SQHS_DIFN_OC_EN;
	reg &= ~USBDP_TRSV_09AC_LN0_RX_SQHS_DIFN_OC_EN;
	reg &= ~USBDP_TRSV_09AC_LN0_ANA_RX_SQHS_DIFN_OC_CODE_SIGN;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_09AC);

	reg = usbdp_cal_reg_rd(regs_base + EXYNOS_USBDP_TRSV_19AC);
	reg &= USBDP_TRSV_066B_LN2_ANA_RX_SQ_VREF_820M_SEL_CLR;
	reg |= USBDP_TRSV_066B_OVRD_LN2_RX_SQHS_EN;
	reg |= USBDP_TRSV_066B_LN2_RX_SQHS_EN;
	reg &= ~USBDP_TRSV_066B_OVRD_LN2_RX_SQHS_DIFN_OC_EN;
	reg &= ~USBDP_TRSV_066B_LN2_RX_SQHS_DIFN_OC_EN;
	reg &= ~USBDP_TRSV_066B_LN2_ANA_RX_SQHS_DIFN_OC_CODE_SIGN;
	usbdp_cal_reg_wr(reg, regs_base + EXYNOS_USBDP_TRSV_19AC);
#endif

}

void phy_exynos_usbdp_g2_disable(struct exynos_usbphy_info *info)
{

}

void phy_exynos_usbdp_g2_set_dp_lane(struct exynos_usbphy_info *info)
{

}

void phy_exynos_usbdp_g2_set_dtb_mux(struct exynos_usbphy_info *info,
	int mux_val)
{
	void __iomem *regs_base = info->regs_base;

	//writel(mux_val & 0xff, regs_base + 0x0284);
	writel(0x2D, regs_base + 0x0B74);
	writel(0x02, regs_base + 0x02C4);
}
