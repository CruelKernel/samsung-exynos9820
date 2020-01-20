/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Author: Sung-Hyun Na <sunghyun.na@samsung.com>
 * Author: Minho Lee <minho55.lee@samsung.com>
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

#ifdef __KERNEL__

#ifndef __EXCITE__
#include <linux/delay.h>
#include <linux/io.h>
#endif

#include <linux/platform_device.h>

#else

#include "types.h"
#include "customfunctions.h"
#include "mct.h"

#endif

#include "phy-samsung-usb-cal.h"
#include "phy-samsung-usb3-cal.h"

#define USB_SS_TX_TUNE_PCS

//#define USB_MUX_UTMI_ENABLE

enum exynos_usbcon_cr {
	USBCON_CR_ADDR = 0,
	USBCON_CR_DATA = 1,
	USBCON_CR_READ = 18,
	USBCON_CR_WRITE = 19,
};

static u16 samsung_exynos_cal_cr_access(struct exynos_usbphy_info *usbphy_info,
		enum exynos_usbcon_cr cr_bit, u16 data)
{
	void __iomem *base;
	u32 phyreg0;
	u32 phyreg1;
	u32 loop;
	u32 loop_cnt;

	if (usbphy_info->used_phy_port != -1) {
		if (usbphy_info->used_phy_port == 0)
			base = usbphy_info->regs_base;
		else
			base = usbphy_info->regs_base_2nd;

	} else
		base = usbphy_info->regs_base;

	/* Clear CR port register */
	phyreg0 = readl(base + EXYNOS_USBCON_PHYREG0);
	phyreg0 &= ~(0xfffff);
	writel(phyreg0, base + EXYNOS_USBCON_PHYREG0);

	/* Set Data for cr port */
	phyreg0 &= ~PHYREG0_CR_DATA_MASK;
	phyreg0 |= PHYREG0_CR_DATA_IN(data);
	writel(phyreg0, base + EXYNOS_USBCON_PHYREG0);

	if (cr_bit == USBCON_CR_ADDR)
		loop = 1;
	else
		loop = 2;

	for (loop_cnt = 0; loop_cnt < loop; loop_cnt++) {
		u32 trigger_bit = 0;
		u32 handshake_cnt = 2;
		/* Trigger cr port */
		if (cr_bit == USBCON_CR_ADDR)
			trigger_bit = PHYREG0_CR_CR_CAP_ADDR;
		else {
			if (loop_cnt == 0)
				trigger_bit = PHYREG0_CR_CR_CAP_DATA;
			else {
				if (cr_bit == USBCON_CR_READ)
					trigger_bit = PHYREG0_CR_READ;
				else
					trigger_bit = PHYREG0_CR_WRITE;
			}
		}
		/* Handshake Procedure */
		do {
			u32 usec = 100;
			if (handshake_cnt == 2)
				phyreg0 |= trigger_bit;
			else
				phyreg0 &= ~trigger_bit;
			writel(phyreg0, base + EXYNOS_USBCON_PHYREG0);

			/* Handshake */
			do {
				phyreg1 = readl(base + EXYNOS_USBCON_PHYREG1);
				if ((handshake_cnt == 2)
						&& (phyreg1 & PHYREG1_CR_ACK))
					break;
				else if ((handshake_cnt == 1)
						&& !(phyreg1 & PHYREG1_CR_ACK))
					break;

				udelay(1);
			} while (usec-- > 0);

			if (!usec)
				pr_err("CRPORT handshake timeout1 (0x%08x)\n",
						phyreg0);

			udelay(5);
			handshake_cnt--;
		} while (handshake_cnt != 0);
		udelay(50);
	}
	return (u16) ((phyreg1 & PHYREG1_CR_DATA_OUT_MASK) >> 1);
}

void samsung_exynos_cal_cr_write(struct exynos_usbphy_info *usbphy_info,
		u16 addr, u16 data)
{
	samsung_exynos_cal_cr_access(usbphy_info, USBCON_CR_ADDR, addr);
	samsung_exynos_cal_cr_access(usbphy_info, USBCON_CR_WRITE, data);
}

u16 samsung_exynos_cal_cr_read(struct exynos_usbphy_info *usbphy_info, u16 addr)
{
	samsung_exynos_cal_cr_access(usbphy_info, USBCON_CR_ADDR, addr);
	return samsung_exynos_cal_cr_access(usbphy_info, USBCON_CR_READ, 0);
}

void samsung_exynos_cal_usb3phy_tune_fix_rxeq(
	struct exynos_usbphy_info *usbphy_info)
{
	u16 reg;
	struct exynos_usbphy_ss_tune *tune = usbphy_info->ss_tune;

	if (!tune)
		return;

	reg = samsung_exynos_cal_cr_read(usbphy_info, 0x1006);
	reg &= ~(1 << 6);
	samsung_exynos_cal_cr_write(usbphy_info, 0x1006, reg);

	udelay(10);

	reg |= (1 << 7);
	samsung_exynos_cal_cr_write(usbphy_info, 0x1006, reg);

	udelay(10);

	reg &= ~(0x7 << 0x8);
	reg |= (tune->fix_rxeq_value << 8);
	samsung_exynos_cal_cr_write(usbphy_info, 0x1006, reg);

	udelay(10);

	reg |= (1 << 11);
	samsung_exynos_cal_cr_write(usbphy_info, 0x1006, reg);

	printk("Reg RX_OVRD_IN_HI : 0x%x\n",
		samsung_exynos_cal_cr_read(usbphy_info, 0x1006));

	printk("Reg RX_CDR_CDR_FSM_DEBUG : 0x%x\n",
		samsung_exynos_cal_cr_read(usbphy_info, 0x101c));
}

static void set_ss_tx_impedance(struct exynos_usbphy_info *usbphy_info)
{
	u16 rtune_debug, tx_ovrd_in_hi;
	u8 tx_imp;

	/* obtain calibration code for 45Ohm impedance */
	rtune_debug = samsung_exynos_cal_cr_read(usbphy_info, 0x3);
	/* set SUP.DIG.RTUNE_DEBUG.TYPE = 2 */
	rtune_debug &= ~(0x3 << 3);
	rtune_debug |= (0x2 << 3);
	samsung_exynos_cal_cr_write(usbphy_info, 0x3, rtune_debug);

	/* read SUP.DIG.RTUNE_STAT (0x0004[9:0]) */
	tx_imp = samsung_exynos_cal_cr_read(usbphy_info, 0x4);
	/* current_tx_cal_code[9:0] = SUP.DIG.RTUNE_STAT (0x0004[9:0]) */
	tx_imp += 8;
	/* tx_cal_code[9:0] = current_tx_cal_code[9:0] + 8(decimal)
	 * NOTE, max value is 63;
	 * i.e. if tx_cal_code[9:0] > 63, tx_cal_code[9:0]==63; */
	if (tx_imp > 63)
		tx_imp = 63;

	/* set LANEX_DIG_TX_OVRD_IN_HI.TX_RESET_OVRD = 1 */
	tx_ovrd_in_hi = samsung_exynos_cal_cr_read(usbphy_info, 0x1001);
	tx_ovrd_in_hi |= (1 << 7);
	samsung_exynos_cal_cr_write(usbphy_info, 0x1001, tx_ovrd_in_hi);

	/* SUP.DIG.RTUNE_DEBUG.MAN_TUNE = 0 */
	rtune_debug &= ~(1 << 1);
	samsung_exynos_cal_cr_write(usbphy_info, 0x3, rtune_debug);

	/* set SUP.DIG.RTUNE_DEBUG.VALUE = tx_cal_code[9:0] */
	rtune_debug &= ~(0x1ff << 5);
	rtune_debug |= (tx_imp << 5);
	samsung_exynos_cal_cr_write(usbphy_info, 0x3, rtune_debug);

	/* set SUP.DIG.RTUNE_DEBUG.SET_VAL = 1 */
	rtune_debug |= (1 << 2);
	samsung_exynos_cal_cr_write(usbphy_info, 0x3, rtune_debug);

	/* set SUP.DIG.RTUNE_DEBUG.SET_VAL = 0 */
	rtune_debug &= ~(1 << 2);
	samsung_exynos_cal_cr_write(usbphy_info, 0x3, rtune_debug);

	/* set LANEX_DIG_TX_OVRD_IN_HI.TX_RESET = 1 */
	tx_ovrd_in_hi |= (1 << 6);
	samsung_exynos_cal_cr_write(usbphy_info, 0x1001, tx_ovrd_in_hi);

	/* set LANEX_DIG_TX_OVRD_IN_HI.TX_RESET = 0 */
	tx_ovrd_in_hi &= ~(1 << 6);
	samsung_exynos_cal_cr_write(usbphy_info, 0x1001, tx_ovrd_in_hi);
}

void samsung_exynos_cal_usb3phy_tune_adaptive_eq(
	struct exynos_usbphy_info *usbphy_info, u8 eq_fix)
{
	u16 reg;

	reg = samsung_exynos_cal_cr_read(usbphy_info, 0x1006);
	if (eq_fix) {
		reg |= (1 << 6);
		reg &= ~(1 << 7);
	} else {
		reg &= ~(1 << 6);
		reg |= (1 << 7);
	}
	samsung_exynos_cal_cr_write(usbphy_info, 0x1006, reg);
}

void samsung_exynos_cal_usb3phy_tune_chg_rxeq(
	struct exynos_usbphy_info *usbphy_info, u8 eq_val)
{
	u16 reg;

	reg = samsung_exynos_cal_cr_read(usbphy_info, 0x1006);
	reg &= ~(0x7 << 0x8);
	reg |= ((eq_val & 0x7) << 8);
	reg |= (1 << 11);
	samsung_exynos_cal_cr_write(usbphy_info, 0x1006, reg);
}

static void exynos_cal_ss_enable(struct exynos_usbphy_info *info,
	u32 *arg_phyclkrst, u8 port_num)
{
	u32 phyclkrst = *arg_phyclkrst;
	u32 phypipe;
	u32 phyparam0;
	u32 phyreg0;
	void *reg_base;

	if (port_num == 0)
		reg_base = info->regs_base;
	else
		reg_base = info->regs_base_2nd;

	if (info->version < EXYNOS_USBCON_VER_01_0_1)
		phyclkrst |= PHYCLKRST_COMMONONN;

	/* Disable Common block control by link */
	phyclkrst &= ~PHYCLKRST_EN_UTMISUSPEND;

	/* Digital Supply Mode : normal operating mode */
	phyclkrst |= PHYCLKRST_RETENABLEN;

	/* ref. clock enable for ss function */
	phyclkrst |= PHYCLKRST_REF_SSP_EN;

	phyparam0 = readl(info->regs_base + EXYNOS_USBCON_PHYPARAM0);
	if (info->refsel == USBPHY_REFSEL_DIFF_PAD)
		phyparam0 |= PHYPARAM0_REF_USE_PAD;
	else
		phyparam0 &= ~PHYPARAM0_REF_USE_PAD;
	writel(phyparam0, reg_base + EXYNOS_USBCON_PHYPARAM0);

	if (info->version >= EXYNOS_USBCON_VER_01_0_1)
		phyreg0 = readl(reg_base + EXYNOS_USBCON_PHYREG0);

	switch (info->refclk) {
	case USBPHY_REFCLK_DIFF_100MHZ:
		phyclkrst &= ~PHYCLKRST_MPLL_MULTIPLIER_MASK;
		phyclkrst |= PHYCLKRST_MPLL_MULTIPLIER(0x00);
		phyclkrst &= ~PHYCLKRST_REF_CLKDIV2;
		if (info->version == EXYNOS_USBCON_VER_01_0_1) {
			phyreg0 &= ~PHYREG0_SSC_REFCLKSEL_MASK;
			phyreg0 |= PHYREG0_SSC_REFCLKSEL(0x00);
		} else {
			phyclkrst &= ~PHYCLKRST_SSC_REFCLKSEL_MASK;
			phyclkrst |= PHYCLKRST_SSC_REFCLKSEL(0x00);
		}
		break;
	case USBPHY_REFCLK_DIFF_26MHZ:
	case USBPHY_REFCLK_DIFF_52MHZ:
		phyclkrst &= ~PHYCLKRST_MPLL_MULTIPLIER_MASK;
		phyclkrst |= PHYCLKRST_MPLL_MULTIPLIER(0x60);
		if (info->refclk & 0x40)
			phyclkrst |= PHYCLKRST_REF_CLKDIV2;
		else
			phyclkrst &= ~PHYCLKRST_REF_CLKDIV2;
		if (info->version == EXYNOS_USBCON_VER_01_0_1) {
			phyreg0 &= ~PHYREG0_SSC_REFCLKSEL_MASK;
			phyreg0 |= PHYREG0_SSC_REFCLKSEL(0x108);
		} else {
			phyclkrst &= ~PHYCLKRST_SSC_REFCLKSEL_MASK;
			phyclkrst |= PHYCLKRST_SSC_REFCLKSEL(0x108);
		}
		break;
	case USBPHY_REFCLK_DIFF_24MHZ:
	case USBPHY_REFCLK_EXT_24MHZ:
		phyclkrst &= ~PHYCLKRST_MPLL_MULTIPLIER_MASK;
		phyclkrst |= PHYCLKRST_MPLL_MULTIPLIER(0x00);
		phyclkrst &= ~PHYCLKRST_REF_CLKDIV2;
		if (info->version != EXYNOS_USBCON_VER_01_0_1) {
			phyclkrst &= ~PHYCLKRST_SSC_REFCLKSEL_MASK;
			phyclkrst |= PHYCLKRST_SSC_REFCLKSEL(0x88);
		} else {
			phyreg0 &= ~PHYREG0_SSC_REFCLKSEL_MASK;
			phyreg0 |= PHYREG0_SSC_REFCLKSEL(0x88);
		}
		break;
	case USBPHY_REFCLK_DIFF_20MHZ:
	case USBPHY_REFCLK_DIFF_19_2MHZ:
		phyclkrst &= ~PHYCLKRST_MPLL_MULTIPLIER_MASK;
		phyclkrst &= ~PHYCLKRST_REF_CLKDIV2;
		if (info->version == EXYNOS_USBCON_VER_01_0_1)
			phyreg0 &= ~PHYREG0_SSC_REFCLKSEL_MASK;
		else
			phyclkrst &= ~PHYCLKRST_SSC_REFCLKSEL_MASK;
		break;
	default:
		break;
	}
	/* SSC Enable */
	if (info->ss_tune) {
		u32 range = info->ss_tune->ssc_range;

		if (info->ss_tune->enable_ssc)
			phyclkrst |= PHYCLKRST_SSC_EN;
		else
			phyclkrst &= ~PHYCLKRST_SSC_EN;

		if (info->version != EXYNOS_USBCON_VER_01_0_1) {
			phyclkrst &= ~PHYCLKRST_SSC_RANGE_MASK;
			phyclkrst |= PHYCLKRST_SSC_RANGE(range);
		} else {
			phyreg0 &= ~PHYREG0_SSC_RANGE_MASK;
			phyreg0 |= PHYREG0_SSC_RAMGE(range);
		}
	} else {
		/* Default Value : SSC Enable */
		phyclkrst |= PHYCLKRST_SSC_EN;
		if (info->version != EXYNOS_USBCON_VER_01_0_1) {
			phyclkrst &= ~PHYCLKRST_SSC_RANGE_MASK;
			phyclkrst |= PHYCLKRST_SSC_RANGE(0x0);
		} else {
			phyreg0 &= ~PHYREG0_SSC_RANGE_MASK;
			phyreg0 |= PHYREG0_SSC_RAMGE(0x0);
		}
	}

	/* Select UTMI CLOCK 0 : PHY CLOCK, 1 : FREE CLOCK */
	phypipe = readl(reg_base + EXYNOS_USBCON_PHYPIPE);
	phypipe |= PHY_CLOCK_SEL;
	writel(phypipe, reg_base + EXYNOS_USBCON_PHYPIPE);

	if (info->version >= EXYNOS_USBCON_VER_01_0_1)
		writel(phyreg0, reg_base + EXYNOS_USBCON_PHYREG0);

	*arg_phyclkrst = phyclkrst;
}

static void exynos_cal_ss_power_down(struct exynos_usbphy_info *info, bool en)
{
	u32 phytest;

	phytest = readl(info->regs_base + EXYNOS_USBCON_PHYTEST);
	if (en == 1) {
		phytest &= ~PHYTEST_POWERDOWN_HSP;
		phytest &= ~PHYTEST_POWERDOWN_SSP;
		writel(phytest, info->regs_base + EXYNOS_USBCON_PHYTEST);
		if (info->used_phy_port == 1) {
			void *reg_base = info->regs_base_2nd;

			phytest |= PHYTEST_POWERDOWN_SSP;
			writel(phytest, info->regs_base + EXYNOS_USBCON_PHYTEST);

			phytest = readl(reg_base + EXYNOS_USBCON_PHYTEST);
			phytest &= ~PHYTEST_POWERDOWN_HSP;
			phytest &= ~PHYTEST_POWERDOWN_SSP;
			writel(phytest, reg_base + EXYNOS_USBCON_PHYTEST);
		}
	} else {
		phytest |= PHYTEST_POWERDOWN_HSP;
		phytest |= PHYTEST_POWERDOWN_SSP;
		writel(phytest, info->regs_base + EXYNOS_USBCON_PHYTEST);
		if (info->used_phy_port == 1) {
			void *reg_base = info->regs_base_2nd;

			phytest = readl(reg_base + EXYNOS_USBCON_PHYTEST);
			phytest |= PHYTEST_POWERDOWN_HSP;
			phytest |= PHYTEST_POWERDOWN_SSP;
			writel(phytest, reg_base + EXYNOS_USBCON_PHYTEST);
		}
	}
}

static void exynos_cal_hs_enable(struct exynos_usbphy_info *info,
	u32 *arg_phyclkrst)
{
	u32 phyclkrst = *arg_phyclkrst;

	u32 hsphyplltune = readl(info->regs_base + EXYNOS_USBCON_HSPHYPLLTUNE);

	if ((info->version & 0xf0) >= 0x10) {
		/* Disable Common block control by link */
		phyclkrst |= PHYCLKRST_EN_UTMISUSPEND;
		phyclkrst |= PHYCLKRST_COMMONONN;
	} else {
		phyclkrst |= PHYCLKRST_EN_UTMISUSPEND;
		phyclkrst |= PHYCLKRST_COMMONONN;
	}

	/* Change PHY PLL Tune value */
	if (info->refclk == USBPHY_REFCLK_EXT_24MHZ)
		hsphyplltune |= HSPHYPLLTUNE_PLL_B_TUNE;
	else
		hsphyplltune &= ~HSPHYPLLTUNE_PLL_B_TUNE;
	hsphyplltune |= HSPHYPLLTUNE_PLL_P_TUNE(0xe);
	writel(hsphyplltune, info->regs_base + EXYNOS_USBCON_HSPHYPLLTUNE);

	*arg_phyclkrst = phyclkrst;
}

static void exynos_cal_hs_power_down(struct exynos_usbphy_info *info, bool en)
{
	u32 hsphyctrl;

	hsphyctrl = readl(info->regs_base + EXYNOS_USBCON_HSPHYCTRL);
	if (en) {
		hsphyctrl &= ~HSPHYCTRL_SIDDQ;
		hsphyctrl &= ~HSPHYCTRL_PHYSWRST;
		hsphyctrl &= ~HSPHYCTRL_PHYSWRSTALL;
	} else {
		hsphyctrl |= HSPHYCTRL_SIDDQ;
	}
	writel(hsphyctrl, info->regs_base + EXYNOS_USBCON_HSPHYCTRL);
}

static void exynos_cal_usbphy_q_ch(void *regs_base, u8 enable)
{
	u32 phy_resume;

	if (enable) {
		/* WA for Q-channel: disable all q-act from usb */
		phy_resume = readl(regs_base + EXYNOS_USBCON_PHYRESUME);
		phy_resume |= PHYRESUME_DIS_ID0_QACT;
		phy_resume |= PHYRESUME_DIS_VBUSVALID_QACT;
		phy_resume |= PHYRESUME_DIS_BVALID_QACT;
		phy_resume |= PHYRESUME_DIS_LINKGATE_QACT;
		//phy_resume |= PHYRESUME_DIS_BUSPEND_QACT;
		phy_resume &= ~PHYRESUME_FORCE_QACT;
		udelay(500);
		writel(phy_resume, regs_base + EXYNOS_USBCON_PHYRESUME);
		udelay(500);
		phy_resume = readl(regs_base + EXYNOS_USBCON_PHYRESUME);
		phy_resume |= PHYRESUME_FORCE_QACT;
		udelay(500);
		writel(phy_resume, regs_base + EXYNOS_USBCON_PHYRESUME);
	} else {
		phy_resume = readl(regs_base + EXYNOS_USBCON_PHYRESUME);
		phy_resume &= ~PHYRESUME_FORCE_QACT;
		phy_resume |= PHYRESUME_DIS_ID0_QACT;
		phy_resume |= PHYRESUME_DIS_VBUSVALID_QACT;
		phy_resume |= PHYRESUME_DIS_BVALID_QACT;
		phy_resume |= PHYRESUME_DIS_LINKGATE_QACT;
		//phy_resume |= PHYRESUME_DIS_BUSPEND_QACT;
		writel(phy_resume, regs_base + EXYNOS_USBCON_PHYRESUME);
	}
}

void samsung_exynos_cal_usb3phy_enable(struct exynos_usbphy_info *usbphy_info)
{
	void __iomem *regs_base = usbphy_info->regs_base;
	u32 version = usbphy_info->version;
	enum exynos_usbphy_refclk refclkfreq = usbphy_info->refclk;
	u32 phyutmi;
	u32 phyclkrst;
	u32 linkport;

	/* check phycon version */
	if (!usbphy_info->hw_version) {
		u32 phyversion = readl(regs_base);

		if (!phyversion) {
			usbphy_info->hw_version = -1;
			usbphy_info->used_phy_port = -1;
		} else {
			usbphy_info->hw_version = phyversion;
			usbphy_info->regs_base += 4;
			regs_base = usbphy_info->regs_base;

			if (usbphy_info->regs_base_2nd)
				usbphy_info->regs_base_2nd += 4;
		}
	}

	/* Set force q-channel */
	if ((version & 0xf) >= 0x01)
		exynos_cal_usbphy_q_ch(regs_base, 1);

	/* Select PHY MUX */
	if (usbphy_info->used_phy_port != -1) {
		u32 physel;

		physel = readl(regs_base + EXYNOS_USBCON_PHYSELECTION);
		if (usbphy_info->used_phy_port == 0) {
			physel &= ~PHYSEL_PIPE;
			physel &= ~PHYSEL_PIPE_CLK;
#if defined(USB_MUX_UTMI_ENABLE)
			/* UE_TASK: for utmi 2nd port test : will be removed */
			physel &= ~PHYSEL_UTMI_CLK;
			physel &= ~PHYSEL_UTMI;
			physel &= ~PHYSEL_SIDEBAND;
#endif
		} else {
			physel |= PHYSEL_PIPE;
			physel |= PHYSEL_PIPE_CLK;
#if defined(USB_MUX_UTMI_ENABLE)
			/* UE_TASK: for utmi 2nd port test : will be removed */
			physel |= PHYSEL_UTMI_CLK;
			physel |= PHYSEL_UTMI;
			physel |= PHYSEL_SIDEBAND;
#endif
		}
		writel(physel, regs_base + EXYNOS_USBCON_PHYSELECTION);
	}

	/* set phy clock & control HS phy */
	phyclkrst = readl(regs_base + EXYNOS_USBCON_PHYCLKRST);

	/* assert port_reset */
	phyclkrst |= PHYCLKRST_PORTRESET;

	/* Select Reference clock source path */
	phyclkrst &= ~PHYCLKRST_REFCLKSEL_MASK;
	phyclkrst |= PHYCLKRST_REFCLKSEL(usbphy_info->refsel);

	/* Select ref clk */
	phyclkrst &= ~PHYCLKRST_FSEL_MASK;
	phyclkrst |= PHYCLKRST_FSEL(refclkfreq & 0x3f);

	/* Enable Common Block in suspend/sleep mode */
	phyclkrst &= ~PHYCLKRST_COMMONONN;

	/* Additional control for 3.0 PHY */
	if ((EXYNOS_USBCON_VER_01_0_0 <= version)
			&& (version <= EXYNOS_USBCON_VER_01_MAX)) {
		exynos_cal_ss_enable(usbphy_info, &phyclkrst, 0);
		/* select used phy port */
		if (usbphy_info->used_phy_port == 1) {
			void *reg_2nd = usbphy_info->regs_base_2nd;

			exynos_cal_ss_enable(usbphy_info, &phyclkrst, 1);

			writel(phyclkrst,
			       reg_2nd + EXYNOS_USBCON_PHYCLKRST);
			udelay(10);

			phyclkrst &= ~PHYCLKRST_PORTRESET;
			writel(phyclkrst,
			       reg_2nd + EXYNOS_USBCON_PHYCLKRST);

			phyclkrst |= PHYCLKRST_PORTRESET;
		}
	} else if ((EXYNOS_USBCON_VER_02_0_0 <= version)
			&& (version <= EXYNOS_USBCON_VER_02_MAX))
		exynos_cal_hs_enable(usbphy_info, &phyclkrst);

	writel(phyclkrst, regs_base + EXYNOS_USBCON_PHYCLKRST);
	udelay(10);

	phyclkrst &= ~PHYCLKRST_PORTRESET;
	writel(phyclkrst, regs_base + EXYNOS_USBCON_PHYCLKRST);

	if ((EXYNOS_USBCON_VER_01_0_0 <= version)
			&& (version <= EXYNOS_USBCON_VER_01_MAX)) {
		exynos_cal_ss_power_down(usbphy_info, 1);
	} else if (version >= EXYNOS_USBCON_VER_02_0_0
			&& version <= EXYNOS_USBCON_VER_02_MAX) {
		exynos_cal_hs_power_down(usbphy_info, 1);
	}
	udelay(500);

	/* release force_sleep & force_suspend */
	phyutmi = readl(regs_base + EXYNOS_USBCON_PHYUTMI);
	phyutmi &= ~PHYUTMI_FORCESLEEP;
	phyutmi &= ~PHYUTMI_FORCESUSPEND;

	/* DP/DM Pull Down Control */
	phyutmi &= ~PHYUTMI_DMPULLDOWN;
	phyutmi &= ~PHYUTMI_DPPULLDOWN;

	/* Set VBUSVALID signal if VBUS pad is not used */
	if (usbphy_info->not_used_vbus_pad) {
		u32 linksystem;

		linksystem = readl(regs_base + EXYNOS_USBCON_LINKSYSTEM);
		linksystem |= LINKSYSTEM_FORCE_BVALID;
		linksystem |= LINKSYSTEM_FORCE_VBUSVALID;
		writel(linksystem, regs_base + EXYNOS_USBCON_LINKSYSTEM);
#if defined(USB_MUX_UTMI_ENABLE)
		/* UE_TASK: for utmi 2nd port test : will be removed */
		if (usbphy_info->regs_base_2nd && usbphy_info->used_phy_port == 1) {
			linksystem = readl(usbphy_info->regs_base_2nd + EXYNOS_USBCON_LINKSYSTEM);
			linksystem |= LINKSYSTEM_FORCE_BVALID;
			linksystem |= LINKSYSTEM_FORCE_VBUSVALID;
			writel(linksystem, usbphy_info->regs_base_2nd + EXYNOS_USBCON_LINKSYSTEM);
		}
#endif
		phyutmi |= PHYUTMI_VBUSVLDEXTSEL;
		phyutmi |= PHYUTMI_VBUSVLDEXT;
	}

	/* disable OTG block and VBUS valid comparator */
	phyutmi &= ~PHYUTMI_DRVVBUS;
	phyutmi |= PHYUTMI_OTGDISABLE;
	writel(phyutmi, regs_base + EXYNOS_USBCON_PHYUTMI);

#if defined(USB_MUX_UTMI_ENABLE)
	/* UE_TASK: for utmi 2nd port test : will be removed */
	if (usbphy_info->regs_base_2nd && usbphy_info->used_phy_port == 1)
		writel(phyutmi, usbphy_info->regs_base_2nd + EXYNOS_USBCON_PHYUTMI);
#endif

	/* OVC io usage */
	linkport = readl(regs_base + EXYNOS_USBCON_LINKPORT);
	if (usbphy_info->use_io_for_ovc) {
		linkport &= ~LINKPORT_HOST_PORT_OVCR_U3_SEL;
		linkport &= ~LINKPORT_HOST_PORT_OVCR_U2_SEL;
	} else {
		linkport |= LINKPORT_HOST_PORT_OVCR_U3_SEL;
		linkport |= LINKPORT_HOST_PORT_OVCR_U2_SEL;
	}
	writel(linkport, regs_base + EXYNOS_USBCON_LINKPORT);

	if ((EXYNOS_USBCON_VER_02_0_0 <= version)
				&& (version <= EXYNOS_USBCON_VER_02_MAX)) {

		u32 hsphyctrl;

		hsphyctrl = readl(regs_base + EXYNOS_USBCON_HSPHYCTRL);
		hsphyctrl |= HSPHYCTRL_PHYSWRST;
		writel(hsphyctrl, regs_base + EXYNOS_USBCON_HSPHYCTRL);
		udelay(20);
		hsphyctrl = readl(regs_base + EXYNOS_USBCON_HSPHYCTRL);
		hsphyctrl &= ~HSPHYCTRL_PHYSWRST;
		writel(hsphyctrl, regs_base + EXYNOS_USBCON_HSPHYCTRL);
	}
}

void samsung_exynos_cal_usb3phy_late_enable(
	struct exynos_usbphy_info *usbphy_info)
{
	u32 version = usbphy_info->version;
	struct exynos_usbphy_ss_tune *tune = usbphy_info->ss_tune;

	if (EXYNOS_USBCON_VER_01_0_0 <= version
			&& version <= EXYNOS_USBCON_VER_01_MAX) {
		/* Set RXDET_MEAS_TIME[11:4] each reference clock */
		samsung_exynos_cal_cr_write(usbphy_info, 0x1010, 0x80);
		printk("Check CR Port write 0x1010: 0x%x\n",
			samsung_exynos_cal_cr_read(usbphy_info, 0x1010));
		if (tune) {
#if defined(USB_SS_TX_TUNE_PCS)
			samsung_exynos_cal_cr_write(usbphy_info,
					0x1002, 0x4000 |
					(tune->tx_deemphasis_3p5db << 7) |
					tune->tx_swing_full);
#else
			samsung_exynos_cal_cr_write(usbphy_info, 0x1002, 0x0);
#endif
			/* Set RX_SCOPE_LFPS_EN */
			if (tune->rx_decode_mode) {
				samsung_exynos_cal_cr_write(usbphy_info,
							    0x1026, 0x1);
			}
			if (tune->set_crport_level_en) {
				/* Enable override los_bias, los_level and
				 * tx_vboost_lvl, Set los_bias to 0x5 and
				 * los_level to 0x9 */
				samsung_exynos_cal_cr_write(usbphy_info,
							    0x15, 0xA409);
				/* Set TX_VBOOST_LEVLE to default Value (0x4) */
				samsung_exynos_cal_cr_write(usbphy_info,
					0x12, 0x8000);
			}
			/* to set the charge pump proportional current */
			if (tune->set_crport_mpll_charge_pump) {
				samsung_exynos_cal_cr_write(usbphy_info,
							    0x30, 0xC0);
			}
			if (tune->enable_fixed_rxeq_mode) {
				samsung_exynos_cal_usb3phy_tune_fix_rxeq(
						usbphy_info);
			}
			set_ss_tx_impedance(usbphy_info);
		}
	}
}

void samsung_exynos_cal_usb3phy_disable(struct exynos_usbphy_info *usbphy_info)
{
	void __iomem *regs_base = usbphy_info->regs_base;
	u32 version = usbphy_info->version;
	u32 phyutmi;
	u32 phyclkrst;

	phyclkrst = readl(regs_base + EXYNOS_USBCON_PHYCLKRST);
	phyclkrst |= PHYCLKRST_EN_UTMISUSPEND;
	phyclkrst |= PHYCLKRST_COMMONONN;
	phyclkrst |= PHYCLKRST_RETENABLEN;
	phyclkrst &= ~PHYCLKRST_REF_SSP_EN;
	phyclkrst &= ~PHYCLKRST_SSC_EN;
	/* Select Reference clock source path */
	phyclkrst &= ~PHYCLKRST_REFCLKSEL_MASK;
	phyclkrst |= PHYCLKRST_REFCLKSEL(usbphy_info->refsel);

	/* Select ref clk */
	phyclkrst &= ~PHYCLKRST_FSEL_MASK;
	phyclkrst |= PHYCLKRST_FSEL(usbphy_info->refclk & 0x3f);
	writel(phyclkrst, regs_base + EXYNOS_USBCON_PHYCLKRST);

	phyutmi = readl(regs_base + EXYNOS_USBCON_PHYUTMI);
	phyutmi &= ~PHYUTMI_IDPULLUP;
	phyutmi &= ~PHYUTMI_DRVVBUS;
	phyutmi |= PHYUTMI_FORCESUSPEND;
	phyutmi |= PHYUTMI_FORCESLEEP;
	if (usbphy_info->not_used_vbus_pad) {
		phyutmi &= ~PHYUTMI_VBUSVLDEXTSEL;
		phyutmi &= ~PHYUTMI_VBUSVLDEXT;
	}
	writel(phyutmi, regs_base + EXYNOS_USBCON_PHYUTMI);

	if ((EXYNOS_USBCON_VER_01_0_0 <= version)
			&& (version <= EXYNOS_USBCON_VER_01_MAX)) {
		exynos_cal_ss_power_down(usbphy_info, 0);
	} else if (version >= EXYNOS_USBCON_VER_02_0_0
			&& version <= EXYNOS_USBCON_VER_02_MAX) {
		exynos_cal_hs_power_down(usbphy_info, 0);
	}

	/* Clear VBUSVALID signal if VBUS pad is not used */
	if (usbphy_info->not_used_vbus_pad) {
		u32 linksystem;

		linksystem = readl(regs_base + EXYNOS_USBCON_LINKSYSTEM);
		linksystem &= ~LINKSYSTEM_FORCE_BVALID;
		linksystem &= ~LINKSYSTEM_FORCE_VBUSVALID;
		writel(linksystem, regs_base + EXYNOS_USBCON_LINKSYSTEM);
	}

	/* Set force q-channel */
	if ((version & 0xf) >= 0x01)
		exynos_cal_usbphy_q_ch(regs_base, 0);
}

void samsung_exynos_cal_usb3phy_config_host_mode(
		struct exynos_usbphy_info *usbphy_info)
{
	void __iomem *regs_base = usbphy_info->regs_base;
	u32 phyutmi;

	phyutmi = readl(regs_base + EXYNOS_USBCON_PHYUTMI);
	phyutmi |= PHYUTMI_DMPULLDOWN;
	phyutmi |= PHYUTMI_DPPULLDOWN;
	phyutmi &= ~PHYUTMI_VBUSVLDEXTSEL;
	phyutmi &= ~PHYUTMI_VBUSVLDEXT;
	writel(phyutmi, regs_base + EXYNOS_USBCON_PHYUTMI);
}

void samsung_exynos_cal_usb3phy_enable_dp_pullup(
		struct exynos_usbphy_info *usbphy_info)
{
	void __iomem *regs_base = usbphy_info->regs_base;
	u32 phyutmi;

	phyutmi = readl(regs_base + EXYNOS_USBCON_PHYUTMI);
	phyutmi |= PHYUTMI_VBUSVLDEXT;
	writel(phyutmi, regs_base + EXYNOS_USBCON_PHYUTMI);
}

void samsung_exynos_cal_usb3phy_disable_dp_pullup(
		struct exynos_usbphy_info *usbphy_info)
{
	void __iomem *regs_base = usbphy_info->regs_base;
	u32 phyutmi;

	phyutmi = readl(regs_base + EXYNOS_USBCON_PHYUTMI);
	phyutmi &= ~PHYUTMI_VBUSVLDEXT;
	writel(phyutmi, regs_base + EXYNOS_USBCON_PHYUTMI);
}

void samsung_exynos_cal_usb3phy_tune_each(
	struct exynos_usbphy_info *usbphy_info,
	enum exynos_usbphy_tune_para para,
	int val)
{
	void __iomem *regs_base = usbphy_info->regs_base;
	u32 reg;

#if defined(USB_MUX_UTMI_ENABLE)
	/* UE_TASK: for utmi 2nd port test : will be removed */
	if (usbphy_info->regs_base_2nd && usbphy_info->used_phy_port == 1)
		regs_base = usbphy_info->regs_base_2nd;
#endif

	if (para < 0x10000) {
		struct exynos_usbphy_hs_tune *tune = usbphy_info->hs_tune;

		reg = readl(regs_base + EXYNOS_USBCON_PHYPARAM0);
		switch ((int) para) {
		case USBPHY_TUNE_HS_COMPDIS:
			reg &= ~PHYPARAM0_COMPDISTUNE_MASK;
			reg |= PHYPARAM0_COMPDISTUNE(val);
			if (tune)
				tune->compdis = val;
			break;
		case USBPHY_TUNE_HS_OTG:
			reg &= ~PHYPARAM0_OTGTUNE_MASK;
			reg |= PHYPARAM0_OTGTUNE(val);
			if (tune)
				tune->otg = val;
			break;
		case USBPHY_TUNE_HS_SQRX:
			reg &= ~PHYPARAM0_SQRXTUNE_MASK;
			reg |= PHYPARAM0_SQRXTUNE(val);
			if (tune)
				tune->rx_sqrx = val;
			break;
		case USBPHY_TUNE_HS_TXFSLS:
			reg &= ~PHYPARAM0_TXFSLSTUNE_MASK;
			reg |= PHYPARAM0_TXFSLSTUNE(val);
			if (tune)
				tune->tx_fsls = val;
			break;
		case USBPHY_TUNE_HS_TXHSXV:
			reg &= ~PHYPARAM0_TXHSXVTUNE_MASK;
			reg |= PHYPARAM0_TXHSXVTUNE(val);
			if (tune)
				tune->tx_hsxv = val;
			break;
		case USBPHY_TUNE_HS_TXPREEMP:
			reg &= ~PHYPARAM0_TXPREEMPAMPTUNE_MASK;
			reg |= PHYPARAM0_TXPREEMPAMPTUNE(val);
			if (tune)
				tune->tx_pre_emp = val;
			break;
		case USBPHY_TUNE_HS_TXPREEMP_PLUS:
			if (val)
				reg |= PHYPARAM0_TXPREEMPPULSETUNE;
			else
				reg &= ~PHYPARAM0_TXPREEMPPULSETUNE;
			if (tune)
				tune->tx_pre_emp_plus = val & 0x1;
			break;
		case USBPHY_TUNE_HS_TXRES:
			reg &= ~PHYPARAM0_TXRESTUNE_MASK;
			reg |= PHYPARAM0_TXRESTUNE(val);
			if (tune)
				tune->tx_res = val;
			break;
		case USBPHY_TUNE_HS_TXRISE:
			reg &= ~PHYPARAM0_TXRISETUNE_MASK;
			reg |= PHYPARAM0_TXRISETUNE(val);
			if (tune)
				tune->tx_rise = val;
			break;
		case USBPHY_TUNE_HS_TXVREF:
			reg &= ~PHYPARAM0_TXVREFTUNE_MASK;
			reg |= PHYPARAM0_TXVREFTUNE(val);
			if (tune)
				tune->tx_vref = val;
			break;
		}
		writel(reg, regs_base + EXYNOS_USBCON_PHYPARAM0);
	} else {
		struct exynos_usbphy_ss_tune *tune = usbphy_info->ss_tune;

		if (usbphy_info->used_phy_port != -1) {
			if (usbphy_info->used_phy_port == 0)
				regs_base = usbphy_info->regs_base;
			else
				regs_base = usbphy_info->regs_base_2nd;
		}

		switch ((int) para) {
		case USBPHY_TUNE_SS_FIX_EQ:
			samsung_exynos_cal_usb3phy_tune_adaptive_eq(usbphy_info,
								    val);
			if (tune)
				tune->enable_fixed_rxeq_mode = val & 0x1;
			break;
		case USBPHY_TUNE_SS_RX_EQ:
			samsung_exynos_cal_usb3phy_tune_chg_rxeq(usbphy_info,
								 val);
			if (tune)
				tune->fix_rxeq_value = val & 0xf;
			break;
		case USBPHY_TUNE_SS_TX_BOOST:
			reg = readl(regs_base + EXYNOS_USBCON_PHYPARAM2);
			reg &= ~PHYPARAM2_TX_VBOOST_LVL_MASK;
			reg |= PHYPARAM2_TX_VBOOST_LVL(val);
			writel(reg, regs_base + EXYNOS_USBCON_PHYPARAM2);
			if (tune)
				tune->tx_boost_level = val;
			break;
		case USBPHY_TUNE_SS_TX_SWING:
#if !defined(USB_SS_TX_TUNE_PCS)
			reg = readl(regs_base + EXYNOS_USBCON_PHYPARAM1);
			reg &= ~PHYPARAM1_PCS_TXSWING_FULL_MASK;
			reg |= PHYPARAM1_PCS_TXSWING_FULL(val);
			writel(reg, regs_base + EXYNOS_USBCON_PHYPARAM1);
#else
			reg = samsung_exynos_cal_cr_read(usbphy_info, 0x1002);
			reg &= ~0x7f;
			reg |= val;
			samsung_exynos_cal_cr_write(usbphy_info,
							 0x1002, 0x4000 | reg);
#endif
			if (tune)
				tune->tx_swing_full = val;
			break;
		case USBPHY_TUNE_SS_TX_DEEMPHASIS:
#if !defined(USB_SS_TX_TUNE_PCS)
			reg = readl(regs_base + EXYNOS_USBCON_PHYPARAM1);
			reg &= ~PHYPARAM1_PCS_TXDEEMPH_3P5DB_MASK;
			reg |= PHYPARAM1_PCS_TXDEEMPH_3P5DB(val);
			writel(reg, regs_base + EXYNOS_USBCON_PHYPARAM1);
#else
			reg = samsung_exynos_cal_cr_read(usbphy_info, 0x1002);
			reg &= ~(0x3f << 7);
			reg |= (val & 0x3f) << 7;
			samsung_exynos_cal_cr_write(usbphy_info,
							 0x1002, 0x4000 | reg);
#endif
			if (tune)
				tune->tx_deemphasis_3p5db = val;
			break;
		}

	}
}

void samsung_exynos_cal_usb3phy_hs_tune_extract(
	struct exynos_usbphy_info *usbphy_info)
{
	struct exynos_usbphy_hs_tune *hs_tune;
	u32 reg;

	hs_tune = usbphy_info->hs_tune;

	if (!hs_tune)
		return;

	reg = readl(usbphy_info->regs_base + EXYNOS_USBCON_PHYPARAM0);

	hs_tune->tx_vref = PHYPARAM0_TXVREFTUNE_EXT(reg);
	hs_tune->tx_rise = PHYPARAM0_TXRISETUNE_EXT(reg);
	hs_tune->tx_res = PHYPARAM0_TXRESTUNE_EXT(reg);
	hs_tune->tx_pre_emp_plus = PHYPARAM0_TXPREEMPPULSETUNE_EXT(reg);
	hs_tune->tx_pre_emp = PHYPARAM0_TXPREEMPAMPTUNE_EXT(reg);
	hs_tune->tx_hsxv = PHYPARAM0_TXHSXVTUNE_EXT(reg);
	hs_tune->tx_fsls = PHYPARAM0_TXFSLSTUNE_EXT(reg);
	hs_tune->rx_sqrx = PHYPARAM0_SQRXTUNE_EXT(reg);
	hs_tune->otg = PHYPARAM0_OTGTUNE_EXT(reg);
	hs_tune->compdis = PHYPARAM0_COMPDISTUNE_EXT(reg);
}

void samsung_exynos_cal_usb3phy_tune_dev(struct exynos_usbphy_info *usbphy_info)
{
	void __iomem *regs_base = usbphy_info->regs_base;
	u32 reg;

#if defined(USB_MUX_UTMI_ENABLE)
	/* UE_TASK: for utmi 2nd port test : will be removed */
	if (usbphy_info->regs_base_2nd && usbphy_info->used_phy_port == 1)
		regs_base = usbphy_info->regs_base_2nd;
#endif

	/* Set the LINK Version Control and Frame Adjust Value */
	reg = readl(regs_base + EXYNOS_USBCON_LINKSYSTEM);
	reg &= ~LINKSYSTEM_FLADJ_MASK;
	reg |= LINKSYSTEM_FLADJ(0x20);
	reg |= LINKSYSTEM_XHCI_VERSION_CONTROL;
	writel(reg, regs_base + EXYNOS_USBCON_LINKSYSTEM);

	/* Tuning the HS Block of phy */
	if (usbphy_info->hs_tune) {
		struct exynos_usbphy_hs_tune *tune = usbphy_info->hs_tune;

		/* Set tune value for 2.0(HS/FS) function */
		reg = readl(regs_base + EXYNOS_USBCON_PHYPARAM0);
		/* TX VREF TUNE */
		reg &= ~PHYPARAM0_TXVREFTUNE_MASK;
		reg |= PHYPARAM0_TXVREFTUNE(tune->tx_vref);
		/* TX RISE TUNE */
		reg &= ~PHYPARAM0_TXRISETUNE_MASK;
		reg |= PHYPARAM0_TXRISETUNE(tune->tx_rise);
		/* TX RES TUNE */
		reg &= ~PHYPARAM0_TXRESTUNE_MASK;
		reg |= PHYPARAM0_TXRESTUNE(tune->tx_res);
		/* TX PRE EMPHASIS PLUS */
		if (tune->tx_pre_emp_plus)
			reg |= PHYPARAM0_TXPREEMPPULSETUNE;
		else
			reg &= ~PHYPARAM0_TXPREEMPPULSETUNE;
		/* TX PRE EMPHASIS */
		reg &= ~PHYPARAM0_TXPREEMPAMPTUNE_MASK;
		reg |= PHYPARAM0_TXPREEMPAMPTUNE(tune->tx_pre_emp);
		/* TX HS XV TUNE */
		reg &= ~PHYPARAM0_TXHSXVTUNE_MASK;
		reg |= PHYPARAM0_TXHSXVTUNE(tune->tx_hsxv);
		/* TX FSLS TUNE */
		reg &= ~PHYPARAM0_TXFSLSTUNE_MASK;
		reg |= PHYPARAM0_TXFSLSTUNE(tune->tx_fsls);
		/* RX SQ TUNE */
		reg &= ~PHYPARAM0_SQRXTUNE_MASK;
		reg |= PHYPARAM0_SQRXTUNE(tune->rx_sqrx);
		/* OTG TUNE */
		reg &= ~PHYPARAM0_OTGTUNE_MASK;
		reg |= PHYPARAM0_OTGTUNE(tune->otg);
		/* COM DIS TUNE */
		reg &= ~PHYPARAM0_COMPDISTUNE_MASK;
		reg |= PHYPARAM0_COMPDISTUNE(tune->compdis);

		writel(reg, regs_base + EXYNOS_USBCON_PHYPARAM0);
	}

	/* Tuning the SS Block of phy */
	if (usbphy_info->ss_tune) {
		struct exynos_usbphy_ss_tune *tune = usbphy_info->ss_tune;

		if (usbphy_info->used_phy_port != -1) {
			if (usbphy_info->used_phy_port == 0)
				regs_base = usbphy_info->regs_base;
			else
				regs_base = usbphy_info->regs_base_2nd;
		}
#if !defined(USB_SS_TX_TUNE_PCS)
		/* Set the PHY Signal Quality Tuning Value */
		reg = readl(regs_base + EXYNOS_USBCON_PHYPARAM1);
		/* TX SWING FULL */
		reg &= ~PHYPARAM1_PCS_TXSWING_FULL_MASK;
		reg |= PHYPARAM1_PCS_TXSWING_FULL(tune->tx_swing_full);
		/* TX DE EMPHASIS 3.5 dB */
		reg &= ~PHYPARAM1_PCS_TXDEEMPH_3P5DB_MASK;
		reg |= PHYPARAM1_PCS_TXDEEMPH_3P5DB(
				tune->tx_deemphasis_3p5db);
		writel(reg, regs_base + EXYNOS_USBCON_PHYPARAM1);
#endif

		/* Set vboost value for eye diagram */
		reg = readl(regs_base + EXYNOS_USBCON_PHYPARAM2);
		/* TX VBOOST Value */
		reg &= ~PHYPARAM2_TX_VBOOST_LVL_MASK;
		reg |= PHYPARAM2_TX_VBOOST_LVL(tune->tx_boost_level);
		/* LOS BIAS */
		reg &= ~PHYPARAM2_LOS_BIAS_MASK;
		reg |= PHYPARAM2_LOS_BIAS(tune->los_bias);
		writel(reg, regs_base + EXYNOS_USBCON_PHYPARAM2);

		/*
		 * Set pcs_rx_los_mask_val for 14nm PHY to mask the abnormal
		 * LFPS and glitches
		 */
		reg = readl(regs_base + EXYNOS_USBCON_PHYPCSVAL);
		reg &= ~PHYPCSVAL_PCS_RX_LOS_MASK_VAL_MASK;
		reg |= PHYPCSVAL_PCS_RX_LOS_MASK_VAL(tune->los_mask_val);
		writel(reg, regs_base + EXYNOS_USBCON_PHYPCSVAL);
	}
}

void samsung_exynos_cal_usb3phy_tune_host(
		struct exynos_usbphy_info *usbphy_info)
{
	void __iomem *regs_base = usbphy_info->regs_base;
	u32 reg;

#if defined(USB_MUX_UTMI_ENABLE)
	/* UE_TASK: for utmi 2nd port test : will be removed */
	if (usbphy_info->regs_base_2nd && usbphy_info->used_phy_port == 1)
		regs_base = usbphy_info->regs_base_2nd;
#endif

	/* Set the LINK Version Control and Frame Adjust Value */
	reg = readl(regs_base + EXYNOS_USBCON_LINKSYSTEM);
	reg &= ~LINKSYSTEM_FLADJ_MASK;
	reg |= LINKSYSTEM_FLADJ(0x20);
	reg |= LINKSYSTEM_XHCI_VERSION_CONTROL;
	writel(reg, regs_base + EXYNOS_USBCON_LINKSYSTEM);

	/* Tuning the HS Block of phy */
	if (usbphy_info->hs_tune) {
		struct exynos_usbphy_hs_tune *tune = usbphy_info->hs_tune;

		/* Set tune value for 2.0(HS/FS) function */
		reg = readl(regs_base + EXYNOS_USBCON_PHYPARAM0);
		/* TX VREF TUNE */
		reg &= ~PHYPARAM0_TXVREFTUNE_MASK;
		reg |= PHYPARAM0_TXVREFTUNE(tune->tx_vref);
		/* TX RISE TUNE */
		reg &= ~PHYPARAM0_TXRISETUNE_MASK;
		reg |= PHYPARAM0_TXRISETUNE(tune->tx_rise);
		/* TX RES TUNE */
		reg &= ~PHYPARAM0_TXRESTUNE_MASK;
		reg |= PHYPARAM0_TXRESTUNE(tune->tx_res);
		/* TX PRE EMPHASIS PLUS */
		if (tune->tx_pre_emp_plus)
			reg |= PHYPARAM0_TXPREEMPPULSETUNE;
		else
			reg &= ~PHYPARAM0_TXPREEMPPULSETUNE;
		/* TX PRE EMPHASIS */
		reg &= ~PHYPARAM0_TXPREEMPAMPTUNE_MASK;
		reg |= PHYPARAM0_TXPREEMPAMPTUNE(tune->tx_pre_emp);
		/* TX HS XV TUNE */
		reg &= ~PHYPARAM0_TXHSXVTUNE_MASK;
		reg |= PHYPARAM0_TXHSXVTUNE(tune->tx_hsxv);
		/* TX FSLS TUNE */
		reg &= ~PHYPARAM0_TXFSLSTUNE_MASK;
		reg |= PHYPARAM0_TXFSLSTUNE(tune->tx_fsls);
		/* RX SQ TUNE */
		reg &= ~PHYPARAM0_SQRXTUNE_MASK;
		reg |= PHYPARAM0_SQRXTUNE(tune->rx_sqrx);
		/* OTG TUNE */
		reg &= ~PHYPARAM0_OTGTUNE_MASK;
		reg |= PHYPARAM0_OTGTUNE(tune->otg);
		/* COM DIS TUNE */
		reg &= ~PHYPARAM0_COMPDISTUNE_MASK;
		reg |= PHYPARAM0_COMPDISTUNE(tune->compdis);

		writel(reg, regs_base + EXYNOS_USBCON_PHYPARAM0);
	}

	/* Tuning the SS Block of phy */
	if (usbphy_info->ss_tune) {
		struct exynos_usbphy_ss_tune *tune = usbphy_info->ss_tune;

		if (usbphy_info->used_phy_port != -1) {
			if (usbphy_info->used_phy_port == 0)
				regs_base = usbphy_info->regs_base;
			else
				regs_base = usbphy_info->regs_base_2nd;
		}
#if !defined(USB_SS_TX_TUNE_PCS)
		/* Set the PHY Signal Quality Tuning Value */
		reg = readl(regs_base + EXYNOS_USBCON_PHYPARAM1);
		/* TX SWING FULL */
		reg &= ~PHYPARAM1_PCS_TXSWING_FULL_MASK;
		reg |= PHYPARAM1_PCS_TXSWING_FULL(tune->tx_swing_full);
		/* TX DE EMPHASIS 3.5 dB */
		reg &= ~PHYPARAM1_PCS_TXDEEMPH_3P5DB_MASK;
		reg |= PHYPARAM1_PCS_TXDEEMPH_3P5DB(
				tune->tx_deemphasis_3p5db);
		writel(reg, regs_base + EXYNOS_USBCON_PHYPARAM1);
#endif

		/* Set vboost value for eye diagram */
		reg = readl(regs_base + EXYNOS_USBCON_PHYPARAM2);
		/* TX VBOOST Value */
		reg &= ~PHYPARAM2_TX_VBOOST_LVL_MASK;
		reg |= PHYPARAM2_TX_VBOOST_LVL(tune->tx_boost_level);
		/* LOS BIAS */
		reg &= ~PHYPARAM2_LOS_BIAS_MASK;
		reg |= PHYPARAM2_LOS_BIAS(tune->los_bias);
		writel(reg, regs_base + EXYNOS_USBCON_PHYPARAM2);

		/*
		 * Set pcs_rx_los_mask_val for 14nm PHY to mask the abnormal
		 * LFPS and glitches
		 */
		reg = readl(regs_base + EXYNOS_USBCON_PHYPCSVAL);
		reg &= ~PHYPCSVAL_PCS_RX_LOS_MASK_VAL_MASK;
		reg |= PHYPCSVAL_PCS_RX_LOS_MASK_VAL(tune->los_mask_val);
		writel(reg, regs_base + EXYNOS_USBCON_PHYPCSVAL);
	}
}
