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
#include "pcie-exynos-host-v0.h"

#if IS_ENABLED(CONFIG_EXYNOS_OTP)
#include <linux/exynos_otp.h>
#endif
extern struct exynos_pcie g_pcie[MAX_RC_NUM];
extern struct exynos_pcie g_pcie_host_v1[MAX_RC_NUM];

/* avoid checking rx elecidle when access DBI */
void exynos_phy_check_rx_elecidle(void *phy_pcs_base_regs, int val, int ch_num)
{
	u32 reg_val;

	if (ch_num == 0) {
		reg_val = readl(phy_pcs_base_regs + 0xEC);
		reg_val &= ~(0x1 << 3);
		reg_val |= (val << 3);
		writel(reg_val, phy_pcs_base_regs + 0xEC);
	} else if (ch_num == 1) {
		reg_val = readl(phy_pcs_base_regs + 0x110);
		reg_val &= ~(0x1 << 2);
		reg_val |= (val << 2);
		writel(reg_val, phy_pcs_base_regs + 0x110);
	}
}

/* Need to apply it at the initial setting sequence */
void exynos_100Mhz_from_socpll_off(void)
{
	/* It is already setted in bootloader */
#ifdef SOCPLL_GATING
	/* FSYS1 CMU Clock Gating Enable */
	writel(0x10000000, 0x11400800);

	/* PCIe QCH SOCPLL gating */
	writel(0x00000000, 0x11403000);
#endif
}

/* PHY all power down */
void exynos_phy_all_pwrdn(void *phy_base_regs, void *phy_pcs_base_regs,
			void *sysreg_base_regs, int ch_num)
{
	/* if you want to use channel 1, you need to set below register */
	u32 __maybe_unused val;

	if (ch_num == 1)
		return ;

	/* Trsv */
	writel(0xFE, phy_base_regs + (0x57 * 4));

	/* Common */
	writel(0xC1, phy_base_regs + (0x20 * 4));

	/* Set PCS values */
	val = readl(phy_pcs_base_regs + 0x100);
	val |= (0x1 << 6);
	writel(val, phy_pcs_base_regs + 0x100);

	val = readl(phy_pcs_base_regs + 0x104);
	val |= (0x1 << 7);
	writel(val, phy_pcs_base_regs + 0x104);

	/* if you want to use channel 1, you need to set below register */
#ifdef SUPPORT_CHANNEL_1
	/* Ch1 SOC Block and PCS Lane Disable */
	val = readl(sysreg_base_regs + 0xC);
	val &= ~(0xA << 12);
	writel(val, sysreg_base_regs + 0xC);
#endif
	exynos_100Mhz_from_socpll_off();
}

/* PHY all power down clear */
void exynos_phy_all_pwrdn_clear(void *phy_base_regs, void *phy_pcs_base_regs,
				void *sysreg_base_regs, int ch_num)
{
	/* if you want to use channel 1, you need to set below register */
	u32 __maybe_unused val;
#ifdef SUPPORT_CHANNEL_1
	u32 val;

	/* Ch0 & Ch1 SOC Block and PCS Lane Enable */
	val = readl(sysreg_base_regs + 0xC);
	val |= 0xf << 12;
	writel(val, sysreg_base_regs + 0xC);
#endif
	if (ch_num == 1)
		return ;

	/* Set PCS values */
	val = readl(phy_pcs_base_regs + 0x100);
	val &= ~(0x1 << 6);
	writel(val, phy_pcs_base_regs + 0x100);

	val = readl(phy_pcs_base_regs + 0x104);
	val &= ~(0x1 << 7);
	writel(val, phy_pcs_base_regs + 0x104);

	/* Common */
	writel(0xC0, phy_base_regs + (0x20 * 4));

	/* Trsv */
	writel(0x7E, phy_base_regs + (0x57 * 4));
}

void exynos_pcie_phy_otp_config(void *phy_base_regs, int ch_num)
{
#if IS_ENABLED(CONFIG_EXYNOS_OTP)
	u8 utype;
	u8 uindex_count;
	struct tune_bits *data;
	u32 index;
	u32 value;
	u16 magic_code;
	u32 i;

	struct pcie_port *pp = &g_pcie[ch_num].pp;

	if (ch_num == 0)
		magic_code = 0x5030;
	else if (ch_num == 1)
		magic_code = 0x5031;
	else
		return ;

	if (!otp_tune_bits_parsed(magic_code, &utype, &uindex_count, &data)) {
		dev_err(pp->dev, "%s: [OTP] uindex_count %d", __func__,
						uindex_count);
		for (i = 0; i < uindex_count; i++) {
			index = data[i].index;
			value = data[i].value;

			dev_err(pp->dev, "%s: [OTP][Return Value] index = "
					"0x%x, value = 0x%x\n",
					__func__, index, value);
			dev_err(pp->dev, "%s: [OTP][Before Reg Value] offset "
					"0x%x = 0x%x\n", __func__, index * 4,
					readl(phy_base_regs + (index * 4)));
			if (readl(phy_base_regs + (index * 4)) != value)
				writel(value, phy_base_regs + (index * 4));
			else
				return;

			dev_err(pp->dev, "%s: [OTP][After Reg Value] offset "
					"0x%x = 0x%x\n", __func__, index * 4,
					readl(phy_base_regs + (index * 4)));
		}
	}
#else
	return ;
#endif
}

void exynos_pcie_phy_config(void *phy_base_regs, void *phy_pcs_base_regs,
		void *sysreg_base_regs, void *elbi_base_regs, int ch_num)
{
	/* 26MHz gen2 */
	u32 cmn_config_val[48] = {
				0x01, 0xE1, 0x05, 0x00, 0x88, 0x88, 0x88, 0x0C,
				0x61, 0x45, 0x65, 0x24, 0x33, 0x18, 0xE3, 0xFC,
				0xD8, 0x05, 0xE6, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x60, 0x11, 0x00, 0xA0, 0x05, 0x04, 0x18, 0x88,
				0xC0, 0xFF, 0x9B, 0x52, 0x22, 0x30, 0x4F, 0xDC,
				0x40, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x80,
				}; /* Ch0 HS1 Gen2*/
	u32 trsv_config_val[80] = {
				0x31, 0x40, 0x37, 0x99, 0x85, 0x00, 0xC0, 0xFF,
				0xFF, 0x3F, 0x8C, 0xC8, 0x02, 0x01, 0x88, 0x80,
				0x06, 0x90, 0x6C, 0x66, 0x09, 0x61, 0x42, 0x44,
				0xC6, 0x50, 0x0A, 0x33, 0x58, 0xE7, 0x20, 0x22,
				0x80, 0x38, 0x05, 0x85, 0x00, 0x00, 0x00, 0x7E,
				0x00, 0x00, 0x55, 0x15, 0xAC, 0xAA, 0x3E, 0x00,
				0x00, 0x00, 0x20, 0x3F, 0x00, 0x03, 0x01, 0x00,
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
				0x05, 0x85, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
				}; /* Ch0 Gen2 */

	/* 26MHz Gen3 */
	u32 cmn_config_val_ch1[160] = {
				0x01, 0x83, 0x00, 0x00, 0x01, 0x42, 0x20, 0x33,
				0x04, 0x15, 0x80, 0x46, 0x46, 0x46, 0x3F, 0x3C,
				0x24, 0x3F, 0x3C, 0x24, 0x24, 0x24, 0x00, 0x07,
				0x3F, 0xDD, 0xA1, 0x11, 0x78, 0xA8, 0x00, 0x03,
				0x91, 0x38, 0xAA, 0xA0, 0x60, 0x60, 0x4D, 0x11,
				0x11, 0x01, 0x17, 0x00, 0x00, 0x27, 0x27, 0x8F,
				0x01, 0x10, 0x10, 0x20, 0x00, 0x2D, 0x27, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x10, 0x01, 0x00, 0x42, 0x20,
				0x33, 0x04, 0x15, 0x80, 0x46, 0x46, 0x46, 0x3F,
				0x3C, 0x24, 0x3F, 0x3C, 0x24, 0x24, 0x24, 0x00,
				0x07, 0x3F, 0xDD, 0xA1, 0x11, 0x78, 0xA8, 0x00,
				0x03, 0x91, 0x38, 0xAA, 0xA0, 0x60, 0x60, 0x4D,
				0x11, 0x11, 0x01, 0x17, 0x00, 0x00, 0x27, 0x27,
				0x8F, 0x01, 0x10, 0x10, 0x20, 0x00, 0x2D, 0x27,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00, 0x60,
				0x8B, 0x20, 0x01, 0x03, 0x00, 0x00, 0x04, 0x00,
				0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				};
	u32 trsv_config_val_ch1[176] = {
				0x40, 0x6B, 0x0B, 0x13, 0x00, 0x00, 0x03, 0x00,
				0x01, 0xA4, 0x03, 0x3C, 0x00, 0x00, 0x17, 0x77,
				0x77, 0x73, 0x30, 0x04, 0x04, 0x00, 0x02, 0x00,
				0x01, 0x47, 0xD4, 0x7C, 0x05, 0x1F, 0x00, 0x24,
				0x00, 0x11, 0x11, 0x11, 0x11, 0x4C, 0x0C, 0x03,
				0x94, 0x00, 0x03, 0x03, 0x30, 0x02, 0x70, 0x02,
				0x30, 0x02, 0x11, 0x07, 0x07, 0x00, 0x0A, 0x40,
				0x13, 0x80, 0x50, 0x50, 0x00, 0x01, 0x28, 0x00,
				0x00, 0x00, 0x08, 0x44, 0x00, 0x00, 0x00, 0x00,
				0x08, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x17,
				0xFD, 0x30, 0x00, 0x60, 0x00, 0x60, 0x00, 0x9A,
				0x03, 0x03, 0x03, 0x05, 0x0A, 0x10, 0x10, 0x04,
				0x00, 0x1A, 0x99, 0x99, 0x99, 0x44, 0x4F, 0xFF,
				0x99, 0x94, 0x44, 0x03, 0x02, 0x0D, 0x03, 0x13,
				0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x04,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				};
	int i;
	u32 reg_val;

	if (ch_num == 0) {
		/* Set PCS_HIGH_SPEED */
		writel(readl(sysreg_base_regs) | (0x1 << 1), sysreg_base_regs);
		writel((((readl(sysreg_base_regs + 0xC)
				& ~(0xf << 4)) & ~(0xf << 2))
				| (0x3 << 2)) & ~(0x1 << 1),
				sysreg_base_regs + 0xC);
		/* Lane0_enable */
		writel(readl(sysreg_base_regs + 0xC) | (0x1 << 12),
				sysreg_base_regs + 0xC);
	} else if (ch_num == 1) {
		/* 0x1141_1060h[0] = "1" */
		writel(readl(sysreg_base_regs + 0xc) | (0x1 << 0),
				sysreg_base_regs + 0xc);
		/* 0x1141_1054h[1] = "0" */
		writel(readl(sysreg_base_regs) & ~(0x1 << 1), sysreg_base_regs);
	}

	/* pcs_g_rst */
	writel(0x1, elbi_base_regs + 0x288);
	udelay(10);
	writel(0x0, elbi_base_regs + 0x288);
	udelay(10);
	writel(0x1, elbi_base_regs + 0x288);
	udelay(10);

	/* PHY Common block Setting */
	if (ch_num == 0) {
		/* PHY Common block Setting */
		for (i = 0; i < sizeof(cmn_config_val) / 4; i++)
			writel(cmn_config_val[i], phy_base_regs + (i * 4));

		/* PHY Tranceiver/Receiver block Setting */
		for (i = 0; i < sizeof(trsv_config_val) / 4; i++)
			writel(trsv_config_val[i],
					phy_base_regs + ((0x30 + i) * 4));
	} else if (ch_num == 1) {
		/* PHY Common block Setting */
		for (i = 0; i < sizeof(cmn_config_val_ch1) / 4; i++)
			writel(cmn_config_val_ch1[i], phy_base_regs + (i * 4));

		/* PHY Tranceiver/Receiver block Setting */
		for (i = 0; i < sizeof(trsv_config_val_ch1) / 4; i++)
			writel(trsv_config_val_ch1[i],
					phy_base_regs + ((0x100 + i) * 4));
	}

#if IS_ENABLED(CONFIG_EXYNOS_OTP)
	/* PHY OTP Tuning bit configuration Setting */
	exynos_pcie_phy_otp_config(phy_base_regs, ch_num);
#endif

	if (ch_num == 1) {
		/* Additional PHY Setting */
		writel(0x38, phy_base_regs + 0x84);
		writel(0x07, phy_base_regs + 0xA4);
		writel(0x38, phy_base_regs + 0x188);
		writel(0x07, phy_base_regs + 0x1A8);
		writel(0x01, phy_base_regs + 0x228);

		/* Additional PCS Setting */
		writel(0x1E4, phy_pcs_base_regs + 0x168);
		writel(0xB0B, phy_pcs_base_regs + 0x134);
		writel(0xA24, phy_pcs_base_regs + 0x148);
		writel(0x003, phy_pcs_base_regs + 0x14C);
		writel(0x12011, phy_pcs_base_regs + 0x150);
		writel(0x33022, phy_pcs_base_regs + 0x154);
		writel(0x55044, phy_pcs_base_regs + 0x158);
		writel(0x00066, phy_pcs_base_regs + 0x15C);
		writel(0x003, phy_pcs_base_regs + 0x19C);
		writel(0x96C0, phy_pcs_base_regs + 0x1A0);
		writel(0x6780, phy_pcs_base_regs + 0x1A4);
		writel(0x8700, phy_pcs_base_regs + 0x1A8);
		writel(0x57C0, phy_pcs_base_regs + 0x1AC);
		writel(0x0900, phy_pcs_base_regs + 0x1B0);
		writel(0x0804, phy_pcs_base_regs + 0x1B4);
		writel(0x07C5, phy_pcs_base_regs + 0x1B8);
		writel(0x7644, phy_pcs_base_regs + 0x1BC);
		writel(0x56C4, phy_pcs_base_regs + 0x1C0);
		writel(0x0747, phy_pcs_base_regs + 0x1C4);
		writel(0xD5C0, phy_pcs_base_regs + 0x1C8);
		writel(0x028, phy_pcs_base_regs + 0x1CC);
		writel(0x029, phy_pcs_base_regs + 0x1D0);
		writel(0x02A, phy_pcs_base_regs + 0x1D4);
		writel(0x02B, phy_pcs_base_regs + 0x1D8);
		writel(0x02C, phy_pcs_base_regs + 0x1DC);
		writel(0x02D, phy_pcs_base_regs + 0x1E0);
		writel(0x02E, phy_pcs_base_regs + 0x1E4);
		writel(0x000, phy_pcs_base_regs + 0x1E8);
	}

	/* tx amplitude control */
	/* writel(0x14, phy_base_regs + (0x5C * 4)); */

	if (ch_num == 0) {
		u32 reg_val;
		/* tx latency */
		writel(0x70, phy_pcs_base_regs + 0xF8);
		/* pcs refclk out control */
		writel(0x87, phy_pcs_base_regs + 0x100);
		writel(0x50, phy_pcs_base_regs + 0x104);
		/* PRGM_TIMEOUT_L1SS_VAL Setting */
		reg_val = readl(phy_pcs_base_regs + 0xC);
		reg_val &= ~(0x1 << 1);
		reg_val |= (0x1 << 4);
		writel(reg_val, phy_pcs_base_regs + 0xC);
	} else if (ch_num == 1) {
		/* tx latency */
		writel(0x1E4, phy_pcs_base_regs + 0x168);
		/* pcs refclk out control */
		writel(readl(phy_pcs_base_regs + 0x174) & ~(0x1 << 8),
				phy_pcs_base_regs + 0x174);
	}

	/* PCIE_MAC RST */
	writel(0x1, elbi_base_regs + 0x290);
	udelay(10);
	writel(0x0, elbi_base_regs + 0x290);
	udelay(10);
	writel(0x1, elbi_base_regs + 0x290);
	udelay(10);

	/* PCIE_PHY PCS&PMA(CMN)_RST */
	writel(0x1, elbi_base_regs + 0x28C);
	udelay(10);
	writel(0x0, elbi_base_regs + 0x28C);
	udelay(10);
	writel(0x1, elbi_base_regs + 0x28C);
	udelay(100);

	/* CDR Reset */
	reg_val = readl(phy_pcs_base_regs + 0xD0);
	reg_val |= (0x3 << 6);
	writel(reg_val, phy_pcs_base_regs + 0xD0);
	udelay(20);
	reg_val = readl(phy_pcs_base_regs + 0xD0);
	reg_val &= ~(0x3 << 6);
	reg_val |= (0x2 << 6);
	writel(reg_val, phy_pcs_base_regs + 0xD0);
	reg_val &= ~(0x3 << 6);
	writel(reg_val, phy_pcs_base_regs + 0xD0);

}

void exynos_pcie_phy_init(struct pcie_port *pp)
{
	/*1234
	struct exynos_pcie *exynos_pcie = to_exynos_pcie(pp);
	*/

	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct exynos_pcie *exynos_pcie = to_exynos_pcie(pci);

	dev_info(pci->dev, "Initialize PHY functions.\n");

	exynos_pcie->phy_ops.phy_check_rx_elecidle =
		exynos_phy_check_rx_elecidle;
	exynos_pcie->phy_ops.phy_all_pwrdn = exynos_phy_all_pwrdn;
	exynos_pcie->phy_ops.phy_all_pwrdn_clear = exynos_phy_all_pwrdn_clear;
	exynos_pcie->phy_ops.phy_config = exynos_pcie_phy_config;
}

/* Exynos9820 specific quirks are defined here. */
static void exynos9820_pcie_quirks(struct pci_dev *dev)
{
	device_disable_async_suspend(&dev->dev);
	pr_info("[%s] async suspend disabled\n", __func__);

#ifdef CONFIG_EXYNOS_PCIE_IOMMU
	set_dma_ops(&dev->dev, &exynos_pcie_dma_ops);
	pr_info("DMA operations are changed to support SysMMU.\n");
#endif
}
DECLARE_PCI_FIXUP_FINAL(PCI_ANY_ID, PCI_ANY_ID, exynos9820_pcie_quirks);
