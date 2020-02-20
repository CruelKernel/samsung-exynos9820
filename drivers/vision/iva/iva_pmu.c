/* iva_pmu.c
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Authors:
 *	Ilkwan Kim <ilkwan.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/time.h>

#include "regs/iva_base_addr.h"
#include "regs/iva_pmu_reg.h"
#include "iva_pmu.h"
#if defined(CONFIG_SOC_EXYNOS9820)
#include "iva_mcu.h"
#endif

#undef PMU_CLK_ENABLE_WITH_BIT_SET		/* clock polarity */

static const uint32_t iva_pmu_clk_addr[pmu_clk_type_max] = {
	[pmu_clk_type_core]	= IVA_PMU_CLOCK_CONTROL_ADDR,
#if defined(CONFIG_SOC_EXYNOS9810)
	[pmu_clk_type_sfr]	= IVA_PMU_SFR_CLOCK_CONTROL_ADDR
#endif
};

static uint32_t __iva_pmu_read_mask(void __iomem *pmu_reg, uint32_t mask)
{
	return readl(pmu_reg) & mask;
}

/* return masked old values */
static uint32_t __iva_pmu_write_mask(void __iomem *pmu_reg, uint32_t mask, bool set)
{
	uint32_t old_val, val;

	old_val = readl(pmu_reg);
	if (set)
		val = old_val | mask;
	else
		val = old_val & ~mask;
	writel(val, pmu_reg);

	return old_val & mask;
}

uint32_t iva_pmu_ctrl(struct iva_dev_data *iva,
			enum iva_pmu_ctrl ctrl, bool set)
{
	return __iva_pmu_write_mask(iva->pmu_va + IVA_PMU_CTRL_ADDR, ctrl, set);
}

uint32_t iva_pmu_reset_hwa(struct iva_dev_data *iva,
			enum iva_pmu_ip_id ip, bool release)
{
	return __iva_pmu_write_mask(iva->pmu_va + IVA_PMU_SOFT_RESET_ADDR,
			ip, release);
}

/* return bool per mask bits provided by @ip */
uint32_t iva_pmu_is_clock_enabled(struct iva_dev_data *iva,
		enum iva_pmu_clk_type type, enum iva_pmu_ip_id ip)
{
	uint32_t mask = (uint32_t) ip;
	uint32_t masked_val = __iva_pmu_read_mask(
			iva->pmu_va + iva_pmu_clk_addr[type], mask);

#ifndef PMU_CLK_ENABLE_WITH_BIT_SET
	masked_val = (~masked_val) & mask;
#endif
	return masked_val;
}

/* return bool per mask bits provided by @ip */
uint32_t iva_pmu_enable_clock(struct iva_dev_data *iva, enum iva_pmu_clk_type type,
		enum iva_pmu_ip_id ip, bool enable)
{
	uint32_t old_masked_val;
	uint32_t mask = (uint32_t) ip;

	/* skip input type check */
	if (type <= pmu_clk_type_none || type >= pmu_clk_type_max)
		return 0;

#ifndef PMU_CLK_ENABLE_WITH_BIT_SET
	enable = !enable;
#endif
	old_masked_val = __iva_pmu_write_mask(
			iva->pmu_va + iva_pmu_clk_addr[type], mask, enable);

#ifndef PMU_CLK_ENABLE_WITH_BIT_SET
	old_masked_val = (~old_masked_val) & mask;
#endif
	return old_masked_val;
}

#if defined(CONFIG_SOC_EXYNOS9810)
uint32_t iva_pmu_enable_auto_clock_gating(struct iva_dev_data *iva,
		enum iva_pmu_ip_id ip, bool enable)
{
	return __iva_pmu_write_mask(iva->pmu_va + IVA_PMU_HWACG_EN_ADDR,
			ip, enable);
}
#endif

#if defined(CONFIG_SOC_EXYNOS9820)
uint32_t iva_pmu_enable_sfr_auto_clock_gating(struct iva_dev_data *iva,
		enum iva_pmu_ip_id ip, bool enable)
{
	return __iva_pmu_write_mask(iva->pmu_va + IVA_PMU_SFR_HWACG_EN_ADDR,
			ip, enable);
}
uint32_t iva_pmu_enable_core_auto_clock_gating(struct iva_dev_data *iva,
		enum iva_pmu_ip_id ip, bool enable)
{
	return __iva_pmu_write_mask(iva->pmu_va + IVA_PMU_CORE_HWACG_EN_ADDR,
			ip, enable);
}
#endif

uint32_t iva_pmu_ctrl_mcu(struct iva_dev_data *iva,
		enum iva_pmu_mcu_ctrl mcu_ctrl, bool set)
{
	return __iva_pmu_write_mask(iva->pmu_va + IVA_PMU_VMCU_CTRL_ADDR,
			mcu_ctrl, set);
}

void iva_pmu_prepare_mcu_sram(struct iva_dev_data *iva)
{
#if defined(CONFIG_SOC_EXYNOS8895)
	/*
	 * Mark PMU_VMCU as active to prevent any QChannel deadlocks
	 * that can happen when resetting the MCU
	 */
	iva_pmu_ctrl(iva, pmu_q_ch_vmcu_active, true);
	/* Put MCU into reset and hold boot*/
	iva_pmu_ctrl_mcu(iva, pmu_mcu_reset_n | pmu_en_sleep, false);
#elif defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
	iva_pmu_ctrl(iva, pmu_ctrl_qactive, true);
	iva_pmu_ctrl_mcu(iva, pmu_mcu_reset_n, false);
#endif

#if defined(CONFIG_SOC_EXYNOS8895) || defined(CONFIG_SOC_EXYNOS9810)
	iva_pmu_ctrl_mcu(iva, pmu_boothold, true);
#elif defined(CONFIG_SOC_EXYNOS9820)
	iva_mcu_ctrl(iva, mcu_boothold, true);
#endif

	/* Release MCU reset */
	iva_pmu_ctrl_mcu(iva, pmu_mcu_reset_n, true);
}

void iva_pmu_reset_mcu(struct iva_dev_data *iva)
{
#if defined(CONFIG_SOC_EXYNOS9820)
    iva_mcu_ctrl(iva, mcu_boothold, false);
#else
	iva_pmu_ctrl_mcu(iva, pmu_boothold, false);
#endif

#if defined(CONFIG_SOC_EXYNOS8895)
	/*
	 * Remove VMCU as active
	 * so that CMU can put us in low-power if and when appropriate
	 */
	iva_pmu_ctrl(iva, pmu_q_ch_vmcu_active, false);
#elif defined(CONFIG_SOC_EXYNOS9810)
	iva_pmu_ctrl(iva, pmu_ctrl_qactive, false);
#endif
}

void iva_pmu_prepare_sfr_dump(struct iva_dev_data *iva)
{
	/* ensure all hwas are released and all clocks are on */
#if defined(CONFIG_SOC_EXYNOS8895)
	iva_pmu_enable_all_clks(iva);
#elif defined(CONFIG_SOC_EXYNOS9810)
	iva_pmu_ctrl(iva, pmu_ctrl_qactive, false);

	/*
	 * to minimize the effect of hwa status change during dump,
	 * enable only sfr clocks.
	 */
	iva_pmu_enable_sfr_clks(iva);
#ifdef ENABLE_CORE_CLK_DURING_DUMP_WORKAROUND
	iva_pmu_enable_all_clks(iva);
#endif
#elif defined(CONFIG_SOC_EXYNOS9820)
	iva_pmu_ctrl(iva, pmu_ctrl_qactive, false);

	/*
	 * to minimize the effect of hwa status change during dump,
	 * enable only sfr clocks.
	 */
	iva_pmu_enable_core_clks(iva);
#ifdef ENABLE_CORE_CLK_DURING_DUMP_WORKAROUND
	iva_pmu_enable_all_clks(iva);
#endif
#endif
	iva_pmu_release_all(iva);
}

#define IVA_PMU_PRINT(dev, pmu_base, reg, reg_name)	\
	dev_info(dev, "%12s: %08X\n", reg_name, readl(pmu_base + reg))

void iva_pmu_show_status(struct iva_dev_data *iva)
{
	struct iva_pmu_dump_reg {
		uint32_t	reg_offset;
		const char	*reg_name;
	};

	struct device	*dev = iva->dev;
	void		*pmu_base = iva->pmu_va;
	uint32_t	i;

	const struct iva_pmu_dump_reg iva_pmu_dump_regs[] = {
		{ IVA_PMU_CTRL_ADDR,		"PMU_CTRL"	},
		{ IVA_PMU_STATUS_ADDR,		"PMU_STATUS"	},
		{ IVA_PMU_IRQ_EN_ADDR,		"IRQ_EN"	},
		{ IVA_PMU_IRQ_RAW_STATUS_ADDR,	"RAW_STATUS"	},
		{ IVA_PMU_IRQ_STATUS_ADDR,	"IRQ_STATUS"	},
		{ IVA_PMU_CLOCK_CONTROL_ADDR,	"CLOCK_CTROL"	},
		{ IVA_PMU_SOFT_RESET_ADDR,	"SOFT_RESET"	},
		{ IVA_PMU_VMCU_CTRL_ADDR,	"VMCU_CTRL"	},
#if defined(CONFIG_SOC_EXYNOS9810)
		{ IVA_PMU_HWACG_EN_ADDR,	"HWACG_EN"	},
#elif defined(CONFIG_SOC_EXYNOS9820)
		{ IVA_PMU_SFR_HWACG_EN_ADDR, 	"SFR_HWACG"	},
		{ IVA_PMU_CORE_HWACG_EN_ADDR, 	"CORE_HWACG"	},
		{ IVA_PMU_QACTIVE_MON_ADDR, 	"QACT_MON"	},
#endif
		{ IVA_PMU_HWA_ACTIVITY_ADDR,	"HWA_ACTIVITY"	},
	};

	dev_info(dev, "--------------  IVA PMU  -------------\n");
	for (i = 0; i < (uint32_t) ARRAY_SIZE(iva_pmu_dump_regs); i++) {
		IVA_PMU_PRINT(dev, pmu_base,
				iva_pmu_dump_regs[i].reg_offset,
				iva_pmu_dump_regs[i].reg_name);
	}
}

void iva_pmu_show_qactive_status(struct iva_dev_data *iva)
{
	struct device	*dev = iva->dev;
	void		*pmu_base = iva->pmu_va;

	IVA_PMU_PRINT(dev, pmu_base, IVA_PMU_CTRL_ADDR, "PMU_CTRL");
	IVA_PMU_PRINT(dev, pmu_base, IVA_PMU_STATUS_ADDR, "PMU_STATUS");
}

int iva_pmu_init(struct iva_dev_data *iva, bool en_hwa)
{
	if (en_hwa) {		/* unreset and enable clocks for all blocks */
		iva_pmu_enable_all_clks(iva);
		iva_pmu_reset_all(iva);
		iva_pmu_release_all(iva);
	} else {		/* explicitly disable all ips */
		iva_pmu_reset_all(iva);
		iva_pmu_disable_all_clks(iva);
	}

	return 0;
}

void iva_pmu_deinit(struct iva_dev_data *iva)
{
	iva_pmu_ctrl(iva, pmu_ctrl_qactive, false);
	iva_pmu_show_qactive_status(iva);
	iva_pmu_reset_all(iva);
	iva_pmu_disable_all_clks(iva);
}

int iva_pmu_probe(struct iva_dev_data *iva)
{
	struct device *dev = iva->dev;

	if (iva->pmu_va) {
		dev_warn(dev, "%s() already mapped into pmu_va(%p)\n", __func__,
				iva->pmu_va);
		return 0;
	}

	if (!iva->iva_va) {
		dev_err(dev, "%s() null iva_va\n", __func__);
		return -EINVAL;
	}

	iva->pmu_va = iva->iva_va + IVA_PMU_BASE_ADDR;

	dev_dbg(iva->dev, "%s() succeed to get pmu va\n", __func__);
	return 0;
}

void iva_pmu_remove(struct iva_dev_data *iva)
{
	if (iva->pmu_va)
		iva->pmu_va = NULL;

	dev_dbg(iva->dev, "%s() succeed to release pmu va\n", __func__);
}
