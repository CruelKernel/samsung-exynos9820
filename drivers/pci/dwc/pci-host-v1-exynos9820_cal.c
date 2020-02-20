/*
 * PCIe phy driver for Samsung EXYNOS8890
 *
 * Copyright (C) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Kyoungil Kim <ki0351.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/of_gpio.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/exynos-pci-noti.h>
#include "pcie-designware.h"
#include "pcie-exynos-host-v1.h"

#if IS_ENABLED(CONFIG_EXYNOS_OTP)
#include <linux/exynos_otp.h>
#endif
extern struct exynos_pcie g_pcie_host_v1[MAX_RC_NUM];

/* avoid checking rx elecidle when access DBI */
void exynos_host_v1_phy_check_rx_elecidle(void *phy_pcs_base_regs, int val, int ch_num)
{
	//Todo need guide
}

/* Need to apply it at the initial setting sequence */
void exynos_host_v1_100Mhz_from_socpll_off(void)	// // TBD: Need to confirm to the Designer
{
	/* It is already setted in bootloader */
#ifdef SOCPLL_GATING	//TBD
	/* FSYS1 CMU Clock Gating Enable */
	writel(0x10000000, 0x11400800);

	/* PCIe QCH SOCPLL gating */
	writel(0x00000000, 0x11403000);
#endif
}

/* PHY all power down */
void exynos_host_v1_phy_all_pwrdn(void *phy_base_regs, void *phy_pcs_base_regs,
		void *sysreg_base_regs, int ch_num)
{
#if 0 /* it should be enabled after setting elecidle */
	void __iomem *cmu_fsys0_base;

	cmu_fsys0_base = ioremap(0x13000000, 0x4000);

	writel(0x3, phy_base_regs + 0x400);

	writel(0x3F0003, cmu_fsys0_base + 0x304C);
	writel(0x3F0003, cmu_fsys0_base + 0x3050);
	writel(0x3F0003, cmu_fsys0_base + 0x3054);

	/* After setting for PHY power-down, it needs to set PMU isolation setting with delay*/
	mdelay(1);

	exynos_host_v1_100Mhz_from_socpll_off();
	iounmap(cmu_fsys0_base);
#endif

	writel(0x23, phy_base_regs + 0x400);
}

/* PHY all power down clear */
void exynos_host_v1_phy_all_pwrdn_clear(void *phy_base_regs, void *phy_pcs_base_regs,
		void *sysreg_base_regs, int ch_num)
{
        writel(0x0, phy_base_regs + 0x400);
}

void exynos_host_v1_pcie_phy_otp_config(void *phy_base_regs, int ch_num)
{
#if IS_ENABLED(CONFIG_EXYNOS_OTP)
#else
	return ;
#endif
}

void exynos_host_v1_pcie_phy_config(void *phy_base_regs, void *phy_pcs_base_regs,
		void *sysreg_base_regs, void *elbi_base_regs, void *dbi_base_regs, int ch_num)
{

	/* pcs_g_rst */
	writel(0x1, elbi_base_regs + 0x1404);
	udelay(10);
	writel(0x0, elbi_base_regs + 0x1404);
	udelay(10);
	writel(0x1, elbi_base_regs + 0x1404);
	udelay(10);

	writel(0xB9, phy_base_regs + 0x30);
	writel(0x03, phy_base_regs + 0x44);
	writel(0x2F, phy_base_regs + 0x78);
	writel(0x2F, phy_base_regs + 0x78);
	writel(0xF8, phy_base_regs + 0x7C);
	writel(0x18, phy_base_regs + 0x110);
	writel(0x18, phy_base_regs + 0x208);
	writel(0x1C, phy_base_regs + 0x214);
	writel(0x56, phy_base_regs + 0x228);
	//writel(0x2F, phy_base_regs + 0x80C);
	writel(0xFC, phy_base_regs + 0x830);
	writel(0x12, phy_base_regs + 0x838);
	writel(0xB8, phy_base_regs + 0x83C);
	writel(0x80, phy_base_regs + 0x874);
	writel(0x3A, phy_base_regs + 0x8EC);
	writel(0x1E, phy_base_regs + 0x8F8);
	writel(0xF0, phy_base_regs + 0x8FC);
	writel(0xE6, phy_base_regs + 0x990);
	writel(0x44, phy_base_regs + 0x99C);
	writel(0x23, phy_base_regs + 0x9A0);
	writel(0x5E, phy_base_regs + 0x9A4);
	writel(0x3E, phy_base_regs + 0xA18);

	writel(0x29, phy_base_regs + 0xB4C); /* updated by HW engineer */
	writel(0x28, phy_base_regs + 0xB58);

	writel(0x05, phy_base_regs + 0xBA0);
	writel(0x55, phy_base_regs + 0xBA4);
	writel(0x6A, phy_base_regs + 0xBA8);
	writel(0xFC, phy_base_regs + 0x1030);
	writel(0x80, phy_base_regs + 0x1074);
	writel(0x44, phy_base_regs + 0x119C);
	writel(0x23, phy_base_regs + 0x11A0);
	writel(0x5E, phy_base_regs + 0x11A4);
	writel(0x25, phy_base_regs + 0x134C);
	writel(0x24, phy_base_regs + 0x1358);
	writel(0x05, phy_base_regs + 0x13A0);
	writel(0x55, phy_base_regs + 0x13A4);
	writel(0x6A, phy_base_regs + 0x13A8);

	writel(0x20, phy_base_regs + 0x44C);
	writel(0x20, phy_base_regs + 0x488);
	writel(0x1, phy_base_regs + 0x438);	/* enable PHY power down delay */
	writel(0x8A, phy_base_regs + 0x450);	/* mask_refclk_out_en*/


#if IS_ENABLED(CONFIG_EXYNOS_OTP)
	/* PHY OTP Tuning bit configuration Setting */
	exynos_pcie_phy_otp_config(phy_base_regs, ch_num);
#endif

	/* tx amplitude control */
	/* writel(0x14, phy_base_regs + (0x5C * 4)); */

	/* PCIE_MAC RST */
	writel(0x1, elbi_base_regs + 0x1400);
	udelay(10);
	writel(0x0, elbi_base_regs + 0x1400);
	udelay(10);
	writel(0x1, elbi_base_regs + 0x1400);
	udelay(10);
	/* PCIE_PHY PCS&PMA(CMN)_RST */
	writel(0x1, elbi_base_regs + 0x1408);
	udelay(10);
	writel(0x0, elbi_base_regs + 0x1408);
	udelay(10);
	writel(0x1, elbi_base_regs + 0x1408);
	udelay(100);
	/* CDR Reset */	//TBD

	/* Additional PHY Setting */
	/* Additional PCS Setting */
	writel(0x700D5, phy_pcs_base_regs + 0x154);
	writel(0x700D5, phy_pcs_base_regs + 0x954);

	//writel(0x12001, dbi_base_regs + 0x890);
	/* EQ Off --> DBI_Base + 0x890h //Need to insert at the Setup_RC code */
}

void exynos_host_v1_pcie_phy_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct exynos_pcie *exynos_pcie = to_exynos_pcie(pci);

	dev_info(pci->dev, "Initialize PHY functions.\n");

	exynos_pcie->phy_ops.phy_check_rx_elecidle =
		exynos_host_v1_phy_check_rx_elecidle;
	exynos_pcie->phy_ops.phy_all_pwrdn = exynos_host_v1_phy_all_pwrdn;
	exynos_pcie->phy_ops.phy_all_pwrdn_clear = exynos_host_v1_phy_all_pwrdn_clear;
	exynos_pcie->phy_ops.phy_config = exynos_host_v1_pcie_phy_config;
}
