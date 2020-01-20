/*
 * Samsung EXYNOS SoC series MIPI CSI/DSI D/C-PHY driver
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <soc/samsung/exynos-pmu.h>

/* the maximum number of PHY for each module */
#define EXYNOS_MIPI_PHYS_NUM	4

#define EXYNOS_MIPI_PHY_ISO_BYPASS  (1 << 0)

#define MIPI_PHY_MxSx_UNIQUE	(0 << 1)
#define MIPI_PHY_MxSx_SHARED	(1 << 1)
#define MIPI_PHY_MxSx_INIT_DONE	(2 << 1)

enum exynos_mipi_phy_owner {
	EXYNOS_MIPI_PHY_OWNER_DSIM = 0,
	EXYNOS_MIPI_PHY_OWNER_CSIS = 1,
};

/* per MIPI-PHY module */
struct exynos_mipi_phy_data {
	u8 flags;
	int active_count;
	spinlock_t slock;
};

#define MKVER(ma, mi)	(((ma) << 16) | (mi))
enum phy_infos {
	VERSION,
	TYPE,
	LANES,
	SPEED,
	SETTLE,
};

struct exynos_mipi_phy_cfg {
	u16 major;
	u16 minor;
	u16 mode;
	/* u32 max_speed */
	int (*set)(void __iomem *regs, int option, u32 *info);
};

/* per DT MIPI-PHY node, can have multiple elements */
struct exynos_mipi_phy {
	struct device *dev;
	spinlock_t slock;
	struct regmap *reg_pmu;
	struct regmap *reg_reset;
	enum exynos_mipi_phy_owner owner;
	struct mipi_phy_desc {
		struct phy *phy;
		struct exynos_mipi_phy_data *data;
		unsigned int index;
		unsigned int iso_offset;
		unsigned int rst_bit;
		void __iomem *regs;
	} phys[EXYNOS_MIPI_PHYS_NUM];
};

/* 1: Isolation bypass, 0: Isolation enable */
static int __set_phy_isolation(struct regmap *reg_pmu,
		unsigned int offset, unsigned int on)
{
	unsigned int val;
	int ret;

	val = on ? EXYNOS_MIPI_PHY_ISO_BYPASS : 0;

	if (reg_pmu)
		ret = regmap_update_bits(reg_pmu, offset,
			EXYNOS_MIPI_PHY_ISO_BYPASS, val);
	else
		ret = exynos_pmu_update(offset,
			EXYNOS_MIPI_PHY_ISO_BYPASS, val);

	if (ret)
		pr_err("%s failed to %s PHY isolation 0x%x\n",
				__func__, on ? "set" : "clear", offset);

	pr_debug("%s off=0x%x, val=0x%x\n", __func__, offset, val);

	return ret;
}

/* 1: Enable reset -> release reset, 0: Enable reset */
static int __set_phy_reset(struct regmap *reg_reset,
		unsigned int bit, unsigned int on)
{
	unsigned int cfg;
	int ret = 0;

	if (!reg_reset)
		return 0;

	ret = regmap_update_bits(reg_reset, 0, BIT(bit), 0);
	if (ret)
		pr_err("%s failed to reset PHY(%d)\n", __func__, bit);

	if (on) {
		ret = regmap_update_bits(reg_reset, 0, BIT(bit), BIT(bit));
		if (ret)
			pr_err("%s failed to release reset PHY(%d)\n",
					__func__, bit);
	}

	regmap_read(reg_reset, 0, &cfg);
	pr_debug("%s bit=%d, cfg=0x%x\n", __func__, bit, cfg);

	return ret;
}

static int __set_phy_init(struct exynos_mipi_phy *state,
		struct mipi_phy_desc *phy_desc, unsigned int on)
{
	unsigned int cfg;
	int ret = 0;

	if (state->reg_pmu)
		ret = regmap_read(state->reg_pmu,
			phy_desc->iso_offset, &cfg);
	else
		ret = exynos_pmu_read(phy_desc->iso_offset, &cfg);

	if (ret) {
		dev_err(state->dev, "%s Can't read 0x%x\n",
				__func__, phy_desc->iso_offset);
		ret = -EINVAL;
		goto phy_exit;
	}

	/* Add INIT_DONE flag when ISO is already bypass(LCD_ON_UBOOT) */
	if (cfg && EXYNOS_MIPI_PHY_ISO_BYPASS)
		phy_desc->data->flags |= MIPI_PHY_MxSx_INIT_DONE;

phy_exit:
	return ret;
}

static int __set_phy_alone(struct exynos_mipi_phy *state,
		struct mipi_phy_desc *phy_desc, unsigned int on)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&state->slock, flags);

	if (on) {
		ret = __set_phy_isolation(state->reg_pmu,
				phy_desc->iso_offset, on);
		if (ret)
			goto phy_exit;

		ret = __set_phy_reset(state->reg_reset,
				phy_desc->rst_bit, on);
	} else {
		ret = __set_phy_reset(state->reg_reset,
				phy_desc->rst_bit, on);
		if (ret)
			goto phy_exit;

		ret = __set_phy_isolation(state->reg_pmu,
				phy_desc->iso_offset, on);
	}

phy_exit:
	pr_debug("%s: isolation 0x%x, reset 0x%x\n", __func__,
			phy_desc->iso_offset, phy_desc->rst_bit);

	spin_unlock_irqrestore(&state->slock, flags);

	return ret;
}

static int __set_phy_share(struct exynos_mipi_phy *state,
		struct mipi_phy_desc *phy_desc, unsigned int on)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&phy_desc->data->slock, flags);

	on ? ++(phy_desc->data->active_count) : --(phy_desc->data->active_count);

	/* If phy is already initialization(power_on) */
	if (state->owner == EXYNOS_MIPI_PHY_OWNER_DSIM &&
			phy_desc->data->flags & MIPI_PHY_MxSx_INIT_DONE) {
		phy_desc->data->flags &= (~MIPI_PHY_MxSx_INIT_DONE);
		spin_unlock_irqrestore(&phy_desc->data->slock, flags);
		return ret;
	}

	if (on) {
		/* Isolation bypass when reference count is 1 */
		if (phy_desc->data->active_count) {
			ret = __set_phy_isolation(state->reg_pmu,
					phy_desc->iso_offset, on);
			if (ret)
				goto phy_exit;
		}

		ret = __set_phy_reset(state->reg_reset,
				phy_desc->rst_bit, on);
	} else {
		ret = __set_phy_reset(state->reg_reset,
				phy_desc->rst_bit, on);
		if (ret)
			goto phy_exit;

		/* Isolation enabled when reference count is zero */
		if (phy_desc->data->active_count == 0)
			ret = __set_phy_isolation(state->reg_pmu,
					phy_desc->iso_offset, on);
	}

phy_exit:
	pr_debug("%s: isolation 0x%x, reset 0x%x\n", __func__,
			phy_desc->iso_offset, phy_desc->rst_bit);

	spin_unlock_irqrestore(&phy_desc->data->slock, flags);

	return ret;
}

static int __set_phy_state(struct exynos_mipi_phy *state,
		struct mipi_phy_desc *phy_desc, unsigned int on)
{
	int ret = 0;

	if (phy_desc->data->flags & MIPI_PHY_MxSx_SHARED)
		ret = __set_phy_share(state, phy_desc, on);
	else
		ret = __set_phy_alone(state, phy_desc, on);

	return ret;
}

static void update_bits(void __iomem *addr, unsigned int start,
			unsigned int width, unsigned int val)
{
	unsigned int cfg;
	unsigned int mask = (width >= 32) ? 0xffffffff : ((1U << width) - 1);

	cfg = readl(addr);
	cfg &= ~(mask << start);
	cfg |= ((val & mask) << start);
	writel(cfg, addr);
}

#define PHY_REF_SPEED	(1500)
static int __set_phy_cfg_0501_0000_dphy(void __iomem *regs, int option, u32 *cfg)
{

	int i;
	u32 skew_cal_en = 0;
	u32 skew_delay_sel = 0;
	u32 hs_mode_sel = 1;

	if (cfg[SPEED] >= PHY_REF_SPEED) {
		skew_cal_en = 1;

		if (cfg[SPEED] >= 3000)
			skew_delay_sel = 1;
		else if (cfg[SPEED] >= 2000)
			skew_delay_sel = 2;
		else
			skew_delay_sel = 3;

		hs_mode_sel = 0;
	}

	writel(0x2, regs + 0x0018);
	update_bits(regs + 0x0084, 0, 8, 0x1);
	for (i = 0; i <= cfg[LANES]; i++) {
		update_bits(regs + 0x04e0 + (i * 0x400), 0, 1, skew_cal_en);
		update_bits(regs + 0x043c + (i * 0x400), 5, 2, skew_delay_sel);
		update_bits(regs + 0x04ac + (i * 0x400), 2, 1, hs_mode_sel);
		update_bits(regs + 0x04b0 + (i * 0x400), 0, 8, cfg[SETTLE]);
	}

	return 0;
}

static int __set_phy_cfg_0502_0000_dphy(void __iomem *regs, int option, u32 *cfg)
{

	int i;
	u32 settle_clk_sel = 1;
	u32 skew_delay_sel = 0;
	u32 type = cfg[TYPE] & 0xffff;
	u32 skew_cal_en = 0;

	if (cfg[SPEED] >= PHY_REF_SPEED) {
		settle_clk_sel = 0;
		skew_cal_en = 1;
	}

	if (cfg[SPEED] >= PHY_REF_SPEED && cfg[SPEED] < 4000) {
		if (cfg[SPEED] >= 3000)
			skew_delay_sel = 1;
		else if (cfg[SPEED] >= 2000)
			skew_delay_sel = 2;
		else
			skew_delay_sel = 3;
	}

	writel(0x00000001, regs + 0x0000); /* SC_GNR_CON0 */
	writel(0x00001450, regs + 0x0004); /* SC_GNR_CON1 */
	writel(0x00000004, regs + 0x0008); /* SC_ANA_CON0 */
	writel(0x00009000, regs + 0x000c); /* SC_ANA_CON1 */
	writel(0x00000005, regs + 0x0010); /* SC_ANA_CON2 */
	writel(0x00000600, regs + 0x0014); /* SC_ANA_CON3 */
	writel(0x00000301, regs + 0x0030); /* SC_TIME_CON0 */

	for (i = 0; i <= cfg[LANES]; i++) {
		writel(0x00000001, regs + 0x0100 + (i * 0x100)); /* SD_GNR_CON0 */
		writel(0x00001450, regs + 0x0104 + (i * 0x100)); /* SD_GNR_CON1 */
		writel(0x00000004, regs + 0x0108 + (i * 0x100)); /* SD_ANA_CON0 */
		writel(0x00009000, regs + 0x010c + (i * 0x100)); /* SD_ANA_CON1 */
		writel(0x00000005, regs + 0x0110 + (i * 0x100)); /* SD_ANA_CON2 */
		update_bits(regs + 0x0110 + (i * 0x100), 8, 2, skew_delay_sel); /* SD_ANA_CON2 */
		writel(0x00000600, regs + 0x0114 + (i * 0x100)); /* SD_ANA_CON3 */
		/* DC Combo lane has below SFR (0/1/2) */
		if ((type == 0xDC) && (i < 3))
			writel(0x00000040, regs + 0x0124 + (i * 0x100)); /* SD_ANA_CON7 */
		update_bits(regs + 0x0130 + (i * 0x100), 0, 8, cfg[SETTLE]); /* SD_TIME_CON0 */
		update_bits(regs + 0x0130 + (i * 0x100), 8, 1, settle_clk_sel); /* SD_TIME_CON0 */
		writel(0x00000003, regs + 0x0134 + (i * 0x100)); /* SD_TIME_CON1 */
		update_bits(regs + 0x0140 + (i * 0x100), 0, 1, skew_cal_en); /* SD_DESKEW_CON0 */
		writel(0x0000081a, regs + 0x0150 + (i * 0x100)); /* SD_DESKEW_CON4 */
	}

	return 0;
}

static int __set_phy_cfg_0502_0001_dphy(void __iomem *regs, int option, u32 *cfg)
{

	int i;
	u32 settle_clk_sel = 1;
	u32 skew_delay_sel = 0;
	u32 skew_cal_en = 0;

	if (cfg[SPEED] >= PHY_REF_SPEED) {
		settle_clk_sel = 0;
		skew_cal_en = 1;
	}

	if (cfg[SPEED] >= PHY_REF_SPEED && cfg[SPEED] < 4000) {
		if (cfg[SPEED] >= 3000)
			skew_delay_sel = 1;
		else if (cfg[SPEED] >= 2000)
			skew_delay_sel = 2;
		else
			skew_delay_sel = 3;
	}

	writel(0x00000001, regs + 0x0500); /* SC_GNR_CON0 */
	writel(0x00001450, regs + 0x0504); /* SC_GNR_CON1 */
	writel(0x00000004, regs + 0x0508); /* SC_ANA_CON0 */
	writel(0x00009000, regs + 0x050c); /* SC_ANA_CON1 */
	writel(0x00000005, regs + 0x0510); /* SC_ANA_CON2 */
	writel(0x00000600, regs + 0x0514); /* SC_ANA_CON3 */
	writel(0x00000301, regs + 0x0530); /* SC_TIME_CON0 */

	for (i = 0; i <= cfg[LANES]; i++) {
		writel(0x00000001, regs + 0x0000 + (i * 0x100)); /* SD_GNR_CON0 */
		writel(0x00001450, regs + 0x0004 + (i * 0x100)); /* SD_GNR_CON1 */
		writel(0x00000004, regs + 0x0008 + (i * 0x100)); /* SD_ANA_CON0 */
		writel(0x00009000, regs + 0x000c + (i * 0x100)); /* SD_ANA_CON1 */
		writel(0x00000005, regs + 0x0010 + (i * 0x100)); /* SD_ANA_CON2 */
		update_bits(regs + 0x0010 + (i * 0x100), 8, 2, skew_delay_sel); /* SD_ANA_CON2 */
		update_bits(regs + 0x0010 + (i * 0x100), 15, 1, 1); /* RESETN_CFG_SEL */
		update_bits(regs + 0x0010 + (i * 0x100), 7, 1, 1); /* RXDDRCLKHS_SEL */
		writel(0x00000600, regs + 0x0014 + (i * 0x100)); /* SD_ANA_CON3 */
		update_bits(regs + 0x0030 + (i * 0x100), 0, 8, cfg[SETTLE]); /* SD_TIME_CON0 */
		update_bits(regs + 0x0030 + (i * 0x100), 8, 1, settle_clk_sel); /* SD_TIME_CON0 */
		writel(0x00000003, regs + 0x0034 + (i * 0x100)); /* SD_TIME_CON1 */
		update_bits(regs + 0x0040 + (i * 0x100), 0, 1, skew_cal_en); /* SD_DESKEW_CON0 */
		writel(0x0000081a, regs + 0x0050 + (i * 0x100)); /* SD_DESKEW_CON4 */
	}

	return 0;
}

static int __set_phy_cfg_0502_0002_dphy(void __iomem *regs, int option, u32 *cfg)
{
	int i;
	u32 settle_clk_sel = 1;
	u32 skew_delay_sel = 0;
	u32 type = cfg[TYPE] & 0xffff;
	u32 t_clk_miss = 3;
	u32 freq_s_xi_c = 26; /* MHz */
	u32 clk_div1234_mc;
	u32 skew_cal_en = 0;

	if (cfg[SPEED] >= PHY_REF_SPEED) {
		settle_clk_sel = 0;
		skew_cal_en = 1;
	}

	if (cfg[SPEED] >= PHY_REF_SPEED && cfg[SPEED] < 4000) {
		if (cfg[SPEED] >= 3000)
			skew_delay_sel = 1;
		else if (cfg[SPEED] >= 2000)
			skew_delay_sel = 2;
		else
			skew_delay_sel = 3;
	}

	writel(0x00000001, regs + 0x0000); /* SC_GNR_CON0 */
	writel(0x00001450, regs + 0x0004); /* SC_GNR_CON1 */
	if (cfg[SPEED] > 4500)
		writel(0x00000000, regs + 0x0008); /* SC_ANA_CON0 */
	else
		writel(0x00000004, regs + 0x0008); /* SC_ANA_CON0 */
	if (cfg[SPEED] > 4500)
		writel(0x00008000, regs + 0x000c); /* SC_ANA_CON1 */
	else
		writel(0x00009000, regs + 0x000c); /* SC_ANA_CON1 */
	writel(0x00000005, regs + 0x0010); /* SC_ANA_CON2 */
	clk_div1234_mc = max(0, (int)(5 - DIV_ROUND_UP(((t_clk_miss - 1) * cfg[SPEED]) >> 2, freq_s_xi_c)));
	update_bits(regs + 0x0010, 8, 2, clk_div1234_mc); /* SC_ANA_CON2 */
	writel(0x00000600, regs + 0x0014); /* SC_ANA_CON3 */
	writel(0x00000301, regs + 0x0030); /* SC_TIME_CON0 */

	for (i = 0; i <= cfg[LANES]; i++) {
		writel(0x00000001, regs + 0x0100 + (i * 0x100)); /* SD_GNR_CON0 */
		writel(0x00001450, regs + 0x0104 + (i * 0x100)); /* SD_GNR_CON1 */
		if (cfg[SPEED] > 4500)
			writel(0x00000000, regs + 0x0108 + (i * 0x100)); /* SD_ANA_CON0 */
		else
			writel(0x00000004, regs + 0x0108 + (i * 0x100)); /* SD_ANA_CON0 */
		if (cfg[SPEED] > 4500)
			writel(0x00008260, regs + 0x010c + (i * 0x100)); /* SD_ANA_CON1 */
		else if (cfg[SPEED] == 4500)
			writel(0x00009260, regs + 0x010c + (i * 0x100)); /* SD_ANA_CON1 */
		else
			writel(0x00009000, regs + 0x010c + (i * 0x100)); /* SD_ANA_CON1 */
		writel(0x00000005, regs + 0x0110 + (i * 0x100)); /* SD_ANA_CON2 */
		update_bits(regs + 0x0110 + (i * 0x100), 8, 2, skew_delay_sel); /* SD_ANA_CON2 */
		writel(0x00000600, regs + 0x0114 + (i * 0x100)); /* SD_ANA_CON3 */
		/* DC Combo lane has below SFR (0/1/2) */
		if ((type == 0xDC) && (i < 3))
			writel(0x00000040, regs + 0x0124 + (i * 0x100)); /* SD_ANA_CON7 */
		update_bits(regs + 0x0130 + (i * 0x100), 0, 8, cfg[SETTLE]); /* SD_TIME_CON0 */
		update_bits(regs + 0x0130 + (i * 0x100), 8, 1, settle_clk_sel); /* SD_TIME_CON0 */
		writel(0x00000003, regs + 0x0134 + (i * 0x100)); /* SD_TIME_CON1 */
		update_bits(regs + 0x0140 + (i * 0x100), 0, 1, skew_cal_en); /* SD_DESKEW_CON0 */
		writel(0x0000081a, regs + 0x0150 + (i * 0x100)); /* SD_DESKEW_CON4 */
	}

	return 0;
}

static int __set_phy_cfg_0502_0003_dphy(void __iomem *regs, int option, u32 *cfg)
{
	int i;
	u32 settle_clk_sel = 1;
	u32 skew_delay_sel = 0;
	u32 t_clk_miss = 3;
	u32 freq_s_xi_c = 26; /* MHz */
	u32 clk_div1234_mc;
	u32 skew_cal_en = 0;

	if (cfg[SPEED] >= PHY_REF_SPEED) {
		settle_clk_sel = 0;
		skew_cal_en = 1;
	}

	if (cfg[SPEED] >= PHY_REF_SPEED && cfg[SPEED] < 4000) {
		if (cfg[SPEED] >= 3000)
			skew_delay_sel = 1;
		else if (cfg[SPEED] >= 2000)
			skew_delay_sel = 2;
		else
			skew_delay_sel = 3;
	}

	writel(0x00000001, regs + 0x0500); /* SC_GNR_CON0 */
	writel(0x00001450, regs + 0x0504); /* SC_GNR_CON1 */
	if (cfg[SPEED] > 4500)
		writel(0x00000000, regs + 0x0508); /* SC_ANA_CON0 */
	else
		writel(0x00000004, regs + 0x0508); /* SC_ANA_CON0 */
	if (cfg[SPEED] > 4500)
		writel(0x00008000, regs + 0x050c); /* SC_ANA_CON1 */
	else
		writel(0x00009000, regs + 0x050c); /* SC_ANA_CON1 */
	writel(0x00000005, regs + 0x0510); /* SC_ANA_CON2 */
	clk_div1234_mc = max(0, (int)(5 - DIV_ROUND_UP(((t_clk_miss - 1) * cfg[SPEED]) >> 2, freq_s_xi_c)));
	update_bits(regs + 0x0510, 8, 2, clk_div1234_mc); /* SC_ANA_CON2 */
	writel(0x00000600, regs + 0x0514); /* SC_ANA_CON3 */
	writel(0x00000301, regs + 0x0530); /* SC_TIME_CON0 */

	for (i = 0; i <= cfg[LANES]; i++) {
		writel(0x00000001, regs + 0x0000 + (i * 0x100)); /* SD_GNR_CON0 */
		writel(0x00001450, regs + 0x0004 + (i * 0x100)); /* SD_GNR_CON1 */
		if (cfg[SPEED] > 4500)
			writel(0x00000000, regs + 0x0008 + (i * 0x100)); /* SD_ANA_CON0 */
		else
			writel(0x00000004, regs + 0x0008 + (i * 0x100)); /* SD_ANA_CON0 */
		if (cfg[SPEED] > 4500)
			writel(0x00008260, regs + 0x000c + (i * 0x100)); /* SD_ANA_CON1 */
		else if (cfg[SPEED] == 4500)
			writel(0x00009260, regs + 0x000c + (i * 0x100)); /* SD_ANA_CON1 */
		else
			writel(0x00009000, regs + 0x000c + (i * 0x100)); /* SD_ANA_CON1 */
		writel(0x00000005, regs + 0x0010 + (i * 0x100)); /* SD_ANA_CON2 */
		update_bits(regs + 0x0010 + (i * 0x100), 8, 2, skew_delay_sel); /* SD_ANA_CON2 */
		update_bits(regs + 0x0010 + (i * 0x100), 15, 1, 1); /* RESETN_CFG_SEL */
		update_bits(regs + 0x0010 + (i * 0x100), 7, 1, 1); /* RXDDRCLKHS_SEL */
		writel(0x00000600, regs + 0x0014 + (i * 0x100)); /* SD_ANA_CON3 */
		update_bits(regs + 0x0030 + (i * 0x100), 0, 8, cfg[SETTLE]); /* SD_TIME_CON0 */
		update_bits(regs + 0x0030 + (i * 0x100), 8, 1, settle_clk_sel); /* SD_TIME_CON0 */
		writel(0x00000003, regs + 0x0034 + (i * 0x100)); /* SD_TIME_CON1 */
		update_bits(regs + 0x0040 + (i * 0x100), 0, 1, skew_cal_en); /* SD_DESKEW_CON0 */
		writel(0x0000081a, regs + 0x0050 + (i * 0x100)); /* SD_DESKEW_CON4 */
	}

	return 0;
}


static const struct exynos_mipi_phy_cfg phy_cfg_table[] = {
	{
		.major = 0x0501,
		.minor = 0x0000,
		.mode = 0xD,
		.set = __set_phy_cfg_0501_0000_dphy,
	},
	{
		.major = 0x0502,
		.minor = 0x0000,
		.mode = 0xD,
		.set = __set_phy_cfg_0502_0000_dphy,
	},
	{
		.major = 0x0502,
		.minor = 0x0001,
		.mode = 0xD,
		.set = __set_phy_cfg_0502_0001_dphy,
	},
	{
		.major = 0x0502,
		.minor = 0x0002,
		.mode = 0xD,
		.set = __set_phy_cfg_0502_0002_dphy,
	},
	{
		.major = 0x0502,
		.minor = 0x0003,
		.mode = 0xD,
		.set = __set_phy_cfg_0502_0003_dphy,
	},
	{ },
};

static int __set_phy_cfg(struct exynos_mipi_phy *state,
		struct mipi_phy_desc *phy_desc, int option, void *info)
{
	u32 *cfg = (u32 *)info;
	unsigned long i;
	int ret = -EINVAL;

	for (i = 0; i < ARRAY_SIZE(phy_cfg_table); i++) {
		if ((cfg[VERSION] == MKVER(phy_cfg_table[i].major,
					phy_cfg_table[i].minor))
		    && ((cfg[TYPE] >> 16) == phy_cfg_table[i].mode)) {
			ret = phy_cfg_table[i].set(phy_desc->regs,
					option, cfg);
			break;
		}
	}

	return ret;
}

static struct exynos_mipi_phy_data mipi_phy_m4s4_top = {
	.flags = MIPI_PHY_MxSx_SHARED,
	.active_count = 0,
	.slock = __SPIN_LOCK_UNLOCKED(mipi_phy_m4s4_top.slock),
};

static struct exynos_mipi_phy_data mipi_phy_m4s4_mod = {
	.flags = MIPI_PHY_MxSx_SHARED,
	.active_count = 0,
	.slock = __SPIN_LOCK_UNLOCKED(mipi_phy_m4s4_mod.slock),
};

static struct exynos_mipi_phy_data mipi_phy_m4s4s4 = {
	.flags = MIPI_PHY_MxSx_SHARED,
	.active_count = 0,
	.slock = __SPIN_LOCK_UNLOCKED(mipi_phy_m4s4s4.slock),
};

static struct exynos_mipi_phy_data mipi_phy_m4s0 = {
	.flags = MIPI_PHY_MxSx_UNIQUE,
	.active_count = 0,
	.slock = __SPIN_LOCK_UNLOCKED(mipi_phy_m4s0.slock),
};

static struct exynos_mipi_phy_data mipi_phy_m2s4s4s2 = {
	.flags = MIPI_PHY_MxSx_SHARED,
	.active_count = 0,
	.slock = __SPIN_LOCK_UNLOCKED(mipi_phy_m2s4s4s2.slock),
};

static struct exynos_mipi_phy_data mipi_phy_m1s2s2 = {
	.flags = MIPI_PHY_MxSx_SHARED,
	.active_count = 0,
	.slock = __SPIN_LOCK_UNLOCKED(mipi_phy_m1s2s2.slock),
};

static struct exynos_mipi_phy_data mipi_phy_m0s4s4s4_mod = {
	.flags = MIPI_PHY_MxSx_SHARED,
	.active_count = 0,
	.slock = __SPIN_LOCK_UNLOCKED(mipi_phy_m0s4s4s4.slock),
};

static const struct of_device_id exynos_mipi_phy_of_table[] = {
	{
		.compatible = "samsung,mipi-phy-m4s4-top",
		.data = &mipi_phy_m4s4_top,
	},
	{
		.compatible = "samsung,mipi-phy-m4s4-mod",
		.data = &mipi_phy_m4s4_mod,
	},
	{
		.compatible = "samsung,mipi-phy-m4s4s4",
		.data = &mipi_phy_m4s4s4,
	},
	{
		.compatible = "samsung,mipi-phy-m4s0",
		.data = &mipi_phy_m4s0,
	},
	{
		.compatible = "samsung,mipi-phy-m2s4s4s2",
		.data = &mipi_phy_m2s4s4s2,
	},
	{
		.compatible = "samsung,mipi-phy-m1s2s2",
		.data = &mipi_phy_m1s2s2,
	},
	{
		.compatible = "samsung,mipi-phy-m0s4s4s4-mod",
		.data = &mipi_phy_m0s4s4s4_mod,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, exynos_mipi_phy_of_table);

#define to_mipi_video_phy(desc) \
	container_of((desc), struct exynos_mipi_phy, phys[(desc)->index])

static int exynos_mipi_phy_init(struct phy *phy)
{
	struct mipi_phy_desc *phy_desc = phy_get_drvdata(phy);
	struct exynos_mipi_phy *state = to_mipi_video_phy(phy_desc);

	return __set_phy_init(state, phy_desc, 1);
}

static int exynos_mipi_phy_power_on(struct phy *phy)
{
	struct mipi_phy_desc *phy_desc = phy_get_drvdata(phy);
	struct exynos_mipi_phy *state = to_mipi_video_phy(phy_desc);

	return __set_phy_state(state, phy_desc, 1);
}

static int exynos_mipi_phy_power_off(struct phy *phy)
{
	struct mipi_phy_desc *phy_desc = phy_get_drvdata(phy);
	struct exynos_mipi_phy *state = to_mipi_video_phy(phy_desc);

	return __set_phy_state(state, phy_desc, 0);
}

static int exynos_mipi_phy_set(struct phy *phy, int option, void *info)
{
	struct mipi_phy_desc *phy_desc = phy_get_drvdata(phy);
	struct exynos_mipi_phy *state = to_mipi_video_phy(phy_desc);

	return __set_phy_cfg(state, phy_desc, option, info);
}

static struct phy *exynos_mipi_phy_of_xlate(struct device *dev,
					struct of_phandle_args *args)
{
	struct exynos_mipi_phy *state = dev_get_drvdata(dev);

	if (WARN_ON(args->args[0] >= EXYNOS_MIPI_PHYS_NUM))
		return ERR_PTR(-ENODEV);

	return state->phys[args->args[0]].phy;
}

static struct phy_ops exynos_mipi_phy_ops = {
	.init		= exynos_mipi_phy_init,
	.power_on	= exynos_mipi_phy_power_on,
	.power_off	= exynos_mipi_phy_power_off,
	.set		= exynos_mipi_phy_set,
	.owner		= THIS_MODULE,
};

static int exynos_mipi_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct exynos_mipi_phy *state;
	struct phy_provider *phy_provider;
	struct exynos_mipi_phy_data *phy_data;
	const struct of_device_id *of_id;
	unsigned int iso[EXYNOS_MIPI_PHYS_NUM];
	unsigned int rst[EXYNOS_MIPI_PHYS_NUM];
	struct resource *res;
	unsigned int i;
	int ret = 0, elements = 0;

	state = devm_kzalloc(dev, sizeof(*state), GFP_KERNEL);
	if (!state)
		return -ENOMEM;

	state->dev  = &pdev->dev;

	of_id = of_match_device(of_match_ptr(exynos_mipi_phy_of_table), dev);
	if (!of_id)
		return -EINVAL;

	phy_data = (struct exynos_mipi_phy_data *)of_id->data;

	dev_set_drvdata(dev, state);
	spin_lock_init(&state->slock);

	/* PMU isolation (optional) */
	state->reg_pmu = syscon_regmap_lookup_by_phandle(node,
						   "samsung,pmu-syscon");
	if (IS_ERR(state->reg_pmu)) {
		dev_err(dev, "failed to lookup PMU regmap, use PMU interface\n");
		state->reg_pmu = NULL;
	}

	elements = of_property_count_u32_elems(node, "isolation");
	if ((elements < 0) || (elements > EXYNOS_MIPI_PHYS_NUM))
		return -EINVAL;

	ret = of_property_read_u32_array(node, "isolation", iso,
					elements);
	if (ret) {
		dev_err(dev, "cannot get PHY isolation offset\n");
		return ret;
	}

	/* SYSREG reset (optional) */
	state->reg_reset = syscon_regmap_lookup_by_phandle(node,
						"samsung,reset-sysreg");
	if (IS_ERR(state->reg_reset)) {
		state->reg_reset = NULL;
	} else {
		ret = of_property_read_u32_array(node, "reset", rst, elements);
		if (ret) {
			dev_err(dev, "cannot get PHY reset bit\n");
			return ret;
		}
	}

	of_property_read_u32(node, "owner", &state->owner);

	for (i = 0; i < elements; i++) {
		state->phys[i].iso_offset = iso[i];
		state->phys[i].rst_bit	  = rst[i];
		dev_info(dev, "%s: isolation 0x%x\n", __func__,
				state->phys[i].iso_offset);
		if (state->reg_reset)
			dev_info(dev, "%s: reset %d\n", __func__,
				state->phys[i].rst_bit);

		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (res) {
			state->phys[i].regs = devm_ioremap_resource(dev, res);
			if (IS_ERR(state->phys[i].regs))
				return PTR_ERR(state->phys[i].regs);
		}
	}

	for (i = 0; i < elements; i++) {
		struct phy *generic_phy = devm_phy_create(dev, NULL,
				&exynos_mipi_phy_ops);
		if (IS_ERR(generic_phy)) {
			dev_err(dev, "failed to create PHY\n");
			return PTR_ERR(generic_phy);
		}

		state->phys[i].index	= i;
		state->phys[i].phy	= generic_phy;
		state->phys[i].data	= phy_data;
		phy_set_drvdata(generic_phy, &state->phys[i]);
	}

	phy_provider = devm_of_phy_provider_register(dev,
			exynos_mipi_phy_of_xlate);

	if (IS_ERR(phy_provider))
		dev_err(dev, "failed to create exynos mipi-phy\n");
	else
		dev_err(dev, "creating exynos-mipi-phy\n");

	return PTR_ERR_OR_ZERO(phy_provider);
}

static struct platform_driver exynos_mipi_phy_driver = {
	.probe	= exynos_mipi_phy_probe,
	.driver = {
		.name  = "exynos-mipi-phy",
		.of_match_table = of_match_ptr(exynos_mipi_phy_of_table),
		.suppress_bind_attrs = true,
	}
};
module_platform_driver(exynos_mipi_phy_driver);

MODULE_DESCRIPTION("Samsung EXYNOS SoC MIPI CSI/DSI D/C-PHY driver");
MODULE_LICENSE("GPL v2");
