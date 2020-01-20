/*
 * Samsung EXYNOS SoC series USB DRD PHY DebugFS file
 *
 * Phy provider for USB 3.0 DRD controller on Exynos SoC series
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 * Author: Kyounghye Yun <k-hye.yun@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ptrace.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/regmap.h>

#include <linux/usb/ch9.h>
#include "phy-exynos-usbdrd.h"
#include "phy-exynos-usbdp-reg.h"
#include "phy-exynos-usbdp-gen2-reg.h"
#include "phy-exynos-debug.h"

static struct exynos_debugfs_prvdata *prvdata_dp;

/* Makalu */
#if defined(CONFIG_PHY_SAMSUNG_USB_GEN2)
/* PHY Combo DP register set */
static const struct debugfs_reg32 exynos_usb3drd_dp_regs[] = {
	dump_register_dp(TRSV_0241), /* ssrx_mf_eq_en */
	dump_register_dp(TRSV_0641), /* ssrx_mf_eq_en */
	dump_register_dp(TRSV_0311), /* ssrx_mf_eq_ctrl_ss */
	dump_register_dp(TRSV_0711), /* ssrx_mf_eq_ctrl_ss */
	dump_register_dp(TRSV_030B), /* ssrx_hf_eq_ctrl_ss */
	dump_register_dp(TRSV_070B), /* ssrx_hf_eq_ctrl_ss */
	dump_register_dp(TRSV_0279), /* ssrx_dfe1_tap_ctrl */
	dump_register_dp(TRSV_0679), /* ssrx_dfe1_tap_ctrl */
	dump_register_dp(TRSV_02BD), /* ssrx_term_cal */
	dump_register_dp(TRSV_06BD), /* ssrx_term_cal */
};

/* PHY Combo DP register set */
static const struct debugfs_regmap32 exynos_usb3drd_dp_regmap[] = {
	dump_regmap_gen2_mask(TRSV_0241, USBDP_TRSV_0241,
		LN0_RX_CTLE_RL_HF_HBR3, 5),
	dump_regmap_dp(TRSV_0241, USBDP_TRSV_0241,
		LN0_RX_CTLE_MF_BWD_EN, 4),
	dump_regmap_dp(TRSV_0241, USBDP_TRSV_0241,
		LN0_RX_CTLE_OC_DAC_PU, 3),
	dump_regmap_gen2_mask(TRSV_0241, USBDP_TRSV_0241,
		LN0_RX_CTLE_I_MF_FWD_CTRL, 0),
	dump_regmap_gen2_mask(TRSV_0641, USBDP_TRSV_0641,
		LN2_RX_CTLE_RL_HF_HBR3, 5),
	dump_regmap_dp(TRSV_0641, USBDP_TRSV_0641,
		LN2_RX_CTLE_MF_BWD_EN, 4),
	dump_regmap_dp(TRSV_0641, USBDP_TRSV_0641,
		LN2_RX_CTLE_OC_DAC_PU, 3),
	dump_regmap_gen2_mask(TRSV_0641, USBDP_TRSV_0641,
		LN2_RX_CTLE_I_MF_FWD_CTRL, 0),
	dump_regmap_gen2_mask(TRSV_0311, USBDP_TRSV_0311,
		LN0_RX_SSLMS_MF_INIT_RATE_SP, 0),
	dump_regmap_gen2_mask(TRSV_0711, USBDP_TRSV_0711,
		LN2_RX_SSLMS_MF_INIT_RATE_SP, 0),
	dump_regmap_gen2_mask(TRSV_030B, USBDP_TRSV_030B,
		LN0_RX_SSLMS_HF_INIT_RATE_SP, 0),
	dump_regmap_gen2_mask(TRSV_070B, USBDP_TRSV_070B,
		LN2_RX_SSLMS_HF_INIT_RATE_SP, 0),
	dump_regmap_gen2_mask(TRSV_0279, USBDP_TRSV_0279,
		LN0_RX_SSLMS_C1_INIT, 0),
	dump_regmap_gen2_mask(TRSV_0679, USBDP_TRSV_0679,
		LN2_RX_SSLMS_C1_INIT, 0),
	dump_regmap_dp(TRSV_02BD, USBDP_TRSV_02BD,
		LN0_RX_OVRD_CAL_RSTN, 7),
	dump_regmap_dp(TRSV_02BD, USBDP_TRSV_02BD,
		LN0_RX_RCAL_RSTN, 6),
	dump_regmap_gen2_mask(TRSV_02BD, USBDP_TRSV_02BD,
		LN0_RX_RCAL_OPT_CODE, 4),
	dump_regmap_gen2_mask(TRSV_02BD, USBDP_TRSV_02BD,
		LN0_RX_RTERM_CTRL, 0),
	dump_regmap_dp(TRSV_06BD, USBDP_TRSV_06BD,
		LN2_RX_OVRD_CAL_RSTN, 7),
	dump_regmap_dp(TRSV_06BD, USBDP_TRSV_06BD,
		LN2_RX_RCAL_RSTN, 6),
	dump_regmap_gen2_mask(TRSV_06BD, USBDP_TRSV_06BD,
		LN2_RX_RCAL_OPT_CODE, 4),
	dump_regmap_gen2_mask(TRSV_06BD, USBDP_TRSV_06BD,
		LN2_RX_RTERM_CTRL, 0),
};
#else /* SOCs without Gen2 - Lhotse */
/* PHY Combo DP register set */
static const struct debugfs_reg32 exynos_usb3drd_dp_regs[] = {
	dump_register_dp(TRSV_R01),
	dump_register_dp(TRSV_R02),
	dump_register_dp(TRSV_R03),
	dump_register_dp(TRSV_R04),
	dump_register_dp(TRSV_R0C),
};

/* PHY Combo DP register set */
static const struct debugfs_regmap32 exynos_usb3drd_dp_regmap[] = {
	dump_regmap_dp_mask(TRSV_R01, USBDP_TRSV01, RXAFE_LEQ_ISEL_GEN2, 6),
	dump_regmap_dp_mask(TRSV_R01, USBDP_TRSV01, RXAFE_LEQ_ISEL_GEN1, 4),
	dump_regmap_dp_mask(TRSV_R01, USBDP_TRSV01, RXAFE_CTLE_SEL, 2),
	dump_regmap_dp_mask(TRSV_R01, USBDP_TRSV01, RXAFE_SCLBUF_EN, 0),
	dump_regmap_dp_mask(TRSV_R02, USBDP_TRSV02, RXAFE_LEQ_CSEL_GEN2, 4),
	dump_regmap_dp_mask(TRSV_R02, USBDP_TRSV02, RXAFE_LEQ_CSEL_GEN1, 0),
	dump_regmap_dp_mask(TRSV_R03, USBDP_TRSV03, RXAFE_LEQ_RSEL_GEN2, 3),
	dump_regmap_dp_mask(TRSV_R03, USBDP_TRSV03, RXAFE_LEQ_RSEL_GEN1, 0),
	dump_regmap_dp_mask(TRSV_R04, USBDP_TRSV04, RXAFE_SQ_VFFSET_CTRL, 0),
	dump_regmap_dp_mask(TRSV_R0C, USBDP_TRSV0C, MAN_TX_DE_EMP_LVL, 4),
	dump_regmap_dp_mask(TRSV_R0C, USBDP_TRSV0C, MAN_TX_DRVR_LVL, 0),
};
#endif

static int debugfs_phy_power_state(struct exynos_usbdrd_phy *phy_drd, int phy_index)
{
	struct regmap *reg_pmu;
	u32 pmu_offset;
	int phy_on;
	int ret;

	reg_pmu = phy_drd->phys[phy_index].reg_pmu;
	pmu_offset = phy_drd->phys[phy_index].pmu_offset;
	ret = regmap_read(reg_pmu, pmu_offset, &phy_on);
	if (ret) {
		dev_err(phy_drd->dev, "Can't read 0x%x\n", pmu_offset);
		return ret;
	}
	phy_on &= phy_drd->phys[phy_index].pmu_mask;

	return phy_on;
}

static int debugfs_print_regmap(struct seq_file *s, const struct debugfs_regmap32 *regs,
				int nregs, void __iomem *base,
				const struct debugfs_reg32 *parent)
{
	int i, j = 0;
	int bit = 0;
	unsigned int bitmask;
	int max_string = 24;
	int calc_tab;
	u32 bit_value, reg_value;

	reg_value = readl(base + parent->offset);
	seq_printf(s, "%s (0x%04lx) : 0x%08x\n", parent->name,
							parent->offset, reg_value);
	for (i = 0; i < nregs; i++, regs++) {
		if (!strcmp(regs->name, parent->name)) {
			bit_value = (reg_value & regs->bitmask) >> regs->bitoffset;

			seq_printf(s, "\t%s", regs->bitname);
			calc_tab = max_string/8 - strlen(regs->bitname)/8;
			for (j = 0 ; j < calc_tab; j++)
				seq_puts(s, "\t");

			if (regs->mask) {
				bitmask = regs->bitmask;
				bitmask = bitmask >> regs->bitoffset;
				while (bitmask) {
					bitmask = bitmask >> 1;
					bit++;
				}
				seq_printf(s, "[%d:%d]\t: 0x%x\n", (int)regs->bitoffset,
						((int)regs->bitoffset + bit - 1), bit_value);
				bit = 0;
			} else {
				seq_printf(s, "[%d]\t: 0x%x\n", (int)regs->bitoffset,
										bit_value);
			}
		}
	}
	return 0;

}

static int debugfs_show_regmap(struct seq_file *s, void *data)
{
	struct exynos_debugfs_prvdata *prvdata = s->private;
	struct debugfs_regset_map *regmap = prvdata->regmap;
	struct debugfs_regset32 *regset = prvdata->regset;
	const struct debugfs_reg32 *regs = regset->regs;
	int phy_on, i = 0;

	phy_on = debugfs_phy_power_state(prvdata->phy_drd, 0);
	if (phy_on < 0) {
		seq_printf(s, "can't read PHY register, error : %d\n", phy_on);
		return -EIO;
	}
	if (!phy_on) {
		seq_puts(s, "can't get PHY register, PHY: Power OFF\n");
		return 0;
	}
	for (i = 0; i < regset->nregs; i++, regs++) {
		debugfs_print_regmap(s, regmap->regs, regmap->nregs,
						regset->base, regs);
	}

	return 0;
}

static int debugfs_open_regmap(struct inode *inode, struct file *file)
{
	return single_open(file, debugfs_show_regmap, inode->i_private);
}

static const struct file_operations fops_regmap = {
	.open =		debugfs_open_regmap,
	.read =		seq_read,
	.llseek =	seq_lseek,
	.release =	single_release,
};

static int debugfs_print_regdump(struct seq_file *s, struct exynos_usbdrd_phy *phy_drd,
				const struct debugfs_reg32 *regs, int nregs,
				void __iomem *base)
{
	int phy_on;
	int i;

	for (i = 0; i < EXYNOS_DRDPHYS_NUM; i++) {
		phy_on = debugfs_phy_power_state(phy_drd, i);
		if (phy_on < 0) {
			seq_printf(s, "can't read PHY register, error : %d\n", phy_on);
			return phy_on;
		}
		if (!phy_on) {
			seq_printf(s, "can't get PHY register, PHY%d : Power OFF\n", i);
			continue;
		}

		for (i = 0; i < nregs; i++, regs++) {
			seq_printf(s, "%s", regs->name);
			if (strlen(regs->name) < 8)
				seq_puts(s, "\t\t");
			else
				seq_puts(s, "\t");

			seq_printf(s, "= 0x%08x\n", readl(base + regs->offset));
		}
	}

	return 0;
}
static int debugfs_show_regdump(struct seq_file *s, void *data)
{
	struct exynos_debugfs_prvdata *prvdata = s->private;
	struct debugfs_regset32 *regset = prvdata->regset;
	int ret;

	ret = debugfs_print_regdump(s, prvdata->phy_drd, regset->regs,
					regset->nregs, regset->base);
	if (ret < 0)
		return ret;

	return 0;
}

static int debugfs_open_regdump(struct inode *inode, struct file *file)
{
	return single_open(file, debugfs_show_regdump, inode->i_private);
}

static const struct file_operations fops_regdump = {
	.open =		debugfs_open_regdump,
	.read =		seq_read,
	.llseek =	seq_lseek,
	.release =	single_release,
};
static int debugfs_show_bitset(struct seq_file *s, void *data)
{
	char *b_name = s->private;
	struct debugfs_regset_map *regmap = prvdata_dp->regmap;
	const struct debugfs_regmap32 *cmp = regmap->regs;
	const struct debugfs_regmap32 *regs;
	unsigned int bitmask;
	int i, bit = 0;
	u32 reg_value, bit_value;
	u32 detect_regs = 0;

	for (i = 0; i < regmap->nregs; i++, cmp++) {
		if (!strcmp(cmp->bitname, b_name)) {
			regs = cmp;
			detect_regs = 1;
			break;
		}
	}

	if (!detect_regs)
		return -EINVAL;

	reg_value = readl(prvdata_dp->regset->base + regs->offset);
	bit_value = (reg_value & regs->bitmask) >> regs->bitoffset;
	if (regs->mask) {
		bitmask = regs->bitmask;
		bitmask = bitmask >> regs->bitoffset;
		while (bitmask) {
			bitmask = bitmask >> 1;
			bit++;
		}
		seq_printf(s, "%s [%d:%d] = 0x%x\n", regs->name,
				(int)regs->bitoffset,
				((int)regs->bitoffset + bit - 1), bit_value);
	} else {
		seq_printf(s, "%s [%d] = 0x%x\n", regs->name,
				(int)regs->bitoffset, bit_value);
	}
	return 0;
}
static ssize_t debugfs_write_regset(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	char *reg_name = s->private;
	struct debugfs_regset32 *regset = prvdata_dp->regset;
	const struct debugfs_reg32 *regs = regset->regs;
	unsigned long value;
	char buf[8];
	int i;

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	value = simple_strtol(buf, NULL, 16);

	for (i = 0; i < regset->nregs; i++, regs++) {
		if (!strcmp(regs->name, reg_name))
			break;
	}

	writel(value, regset->base + regs->offset);

	return count;
}
static ssize_t debugfs_write_bitset(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	char *b_name = s->private;
	struct debugfs_regset_map *regmap = prvdata_dp->regmap;
	const struct debugfs_regmap32 *regs = regmap->regs;
	unsigned long value;
	char buf[32];
	int i;
	u32 reg_value;

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count))) {
		seq_printf(s, "%s, write error\n", __func__);
		return -EFAULT;
	}
	value = simple_strtol(buf, NULL, 2);

	for (i = 0; i < regmap->nregs; i++, regs++) {
		if (!strcmp(regs->bitname, b_name))
			break;
	}

	value = value << regs->bitoffset;
	reg_value = readl(prvdata_dp->regset->base + regs->offset);
	reg_value &= ~(regs->bitmask);
	reg_value |= (u32)value;
	writel(reg_value, prvdata_dp->regset->base + regs->offset);

	return count;
}

static int debugfs_open_bitset(struct inode *inode, struct file *file)
{
	return single_open(file, debugfs_show_bitset, inode->i_private);
}

static int debugfs_show_regset(struct seq_file *s, void *data)
{
	char *p_name = s->private;
	struct debugfs_regset32 *regset = prvdata_dp->regset;
	struct debugfs_regset_map *regmap = prvdata_dp->regmap;
	const struct debugfs_reg32 *regs = regset->regs;
	const struct debugfs_reg32 *parents;
	u32 detect_regs = 0;


	int i;

	for (i = 0; i < regset->nregs; i++, regs++) {
		if (!strcmp(regs->name, p_name)) {
			parents = regs;
			detect_regs = 1;
			break;
		}
	}
	if (!detect_regs)
		return -EINVAL;

	debugfs_print_regmap(s, prvdata_dp->regmap->regs, regmap->nregs,
						regset->base, parents);

	return 0;
}

static int debugfs_open_regset(struct inode *inode, struct file *file)
{
	return single_open(file, debugfs_show_regset, inode->i_private);
}

static const struct file_operations fops_regset = {
	.open =		debugfs_open_regset,
	.write =	debugfs_write_regset,
	.read =		seq_read,
	.llseek =	seq_lseek,
	.release =	single_release,
};

static const struct file_operations fops_bitset = {
	.open =		debugfs_open_bitset,
	.write =	debugfs_write_bitset,
	.read =		seq_read,
	.llseek =	seq_lseek,
	.release =	single_release,
};

static int debugfs_create_regfile(struct exynos_debugfs_prvdata *prvdata,
					const struct debugfs_reg32 *parents,
					struct dentry *root)
{
	struct debugfs_regset_map *regmap = prvdata->regmap;
	const struct debugfs_regmap32 *regs = regmap->regs;
	struct dentry *file;
	int i, ret;

	file = debugfs_create_file(parents->name, 0644, root,
							parents->name, &fops_regset);
	if (!file) {
		ret = -ENOMEM;
		return ret;
	}
	for (i = 0; i < regmap->nregs; i++, regs++) {
		if (!strcmp(regs->name, parents->name)) {
			file = debugfs_create_file(regs->bitname, 0644,
					root, regs->bitname, &fops_bitset);
			if (!file) {
				ret = -ENOMEM;
				return ret;
			}
		}
	}

	return 0;
}

static int debugfs_create_regdir(struct exynos_debugfs_prvdata *prvdata,
						struct dentry *root)
{
	struct exynos_usbdrd_phy *phy_drd = prvdata->phy_drd;
	struct debugfs_regset32 *regset = prvdata->regset;
	const struct debugfs_reg32 *regs = regset->regs;
	struct dentry	*dir;
	int ret, i;

	for (i = 0; i < regset->nregs; i++, regs++) {
		dir = debugfs_create_dir(regs->name, root);
		if (!dir) {
			dev_err(phy_drd->dev, "failed to create '%s' reg dir",
								regs->name);
			return -ENOMEM;
		}
		ret = debugfs_create_regfile(prvdata, regs, dir);
		if (ret < 0) {
			dev_err(phy_drd->dev, "failed to create bitfile for %s, error : %d\n",
						regs->name, ret);
			return ret;
		}
	}

	return 0;
}
int exynos_usbdrd_dp_debugfs_init(struct exynos_usbdrd_phy *phy_drd)
{
	struct device	*dev = phy_drd->dev;
	struct dentry	*root;
	struct dentry	*dir;
	struct dentry	*file;
	u32 version = phy_drd->usbphy_sub_info.version;
	int		ret;

#if defined(CONFIG_PHY_SAMSUNG_USB_GEN2)
	/* Makalu */
	root = debugfs_create_dir("10ae0000.usbdp", NULL);
#else
	/* Lhotse */
	root = debugfs_create_dir("110a0000.usbdp", NULL);
#endif

	if (!root) {
		dev_err(dev, "failed to create root directory for USBPHY debugfs");
		ret = -ENOMEM;
		goto err0;
	}

	prvdata_dp = devm_kmalloc(dev, sizeof(struct exynos_debugfs_prvdata), GFP_KERNEL);
	if (!prvdata_dp) {
		dev_err(dev, "failed to alloc private data for debugfs");
		ret = -ENOMEM;
		goto err1;
	}
	prvdata_dp->root = root;
	prvdata_dp->phy_drd = phy_drd;

	prvdata_dp->regset = devm_kmalloc(dev, sizeof(*prvdata_dp->regset), GFP_KERNEL);
	if (!prvdata_dp->regset) {
		dev_err(dev, "failed to alloc regmap");
		ret = -ENOMEM;
		goto err1;
	}

	if (phy_drd->usbphy_sub_info.version == EXYNOS_USBCON_VER_04_0_0) {
		/* for USB3PHY Lhotse */
		prvdata_dp->regset->regs = exynos_usb3drd_dp_regs;
		prvdata_dp->regset->nregs = ARRAY_SIZE(exynos_usb3drd_dp_regs);
	}

	prvdata_dp->regset->base = phy_drd->reg_phy2;

	prvdata_dp->regmap = devm_kmalloc(dev, sizeof(*prvdata_dp->regmap), GFP_KERNEL);
	if (!prvdata_dp->regmap) {
		dev_err(dev, "failed to alloc regmap");
		ret = -ENOMEM;
		goto err1;
	}

	if (version == EXYNOS_USBCON_VER_04_0_0) {
		/* for USB3PHY Lhotse */
		prvdata_dp->regmap->regs = exynos_usb3drd_dp_regmap;
		prvdata_dp->regmap->nregs = ARRAY_SIZE(exynos_usb3drd_dp_regmap);
	}

	file = debugfs_create_file("regdump", 0444, root, prvdata_dp, &fops_regdump);
	if (!file) {
		dev_err(dev, "failed to create file for register dump");
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("regmap", 0444, root, prvdata_dp, &fops_regmap);
	if (!file) {
		dev_err(dev, "failed to create file for register dump");
		ret = -ENOMEM;
		goto err1;
	}

	dir = debugfs_create_dir("regset", root);
	if (!dir) {
		ret = -ENOMEM;
		goto err1;
	}

	ret = debugfs_create_regdir(prvdata_dp, dir);
	if (ret < 0) {
		dev_err(dev, "failed to create regfile, error = %d\n", ret);
		goto err1;
	}


	return 0;

err1:
	debugfs_remove_recursive(root);
err0:
	return ret;
}
