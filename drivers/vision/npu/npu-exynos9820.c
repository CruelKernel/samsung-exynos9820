/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/delay.h>

#include "npu-config.h"
#include "npu-exynos.h"
#include "npu-exynos9820.h"
#include "npu-debug.h"

#include "npu-log.h"

#define CLK_INDEX(name) NPU_##name
#define REGISTER_CLK(name)[CLK_INDEX(name)] = {#name, NULL}

extern int npu_clk_set_rate(struct device *dev, u32 index, ulong frequency);
extern ulong npu_clk_get_rate(struct device *dev, u32 index);
extern int  npu_clk_enable(struct device *dev, u32 index);
extern int npu_clk_disable(struct device *dev, u32 index);

enum npu_clk_index {
	CLK_INDEX(npu)
};

struct npu_clk npu_clk_array[] = {
	REGISTER_CLK(npu)
};

const u32 npu_clk_array_size = ARRAY_SIZE(npu_clk_array);

int npu_exynos_clk_cfg(struct npu_exynos *exynos)
{
	return 0;
}

int npu_exynos_clk_on(struct npu_exynos *exynos)
{
	npu_clk_enable(exynos->dev, CLK_INDEX(npu));
	npu_clk_get_rate(exynos->dev, CLK_INDEX(npu));
	return 0;
}

int npu_exynos_clk_off(struct npu_exynos *exynos)
{
	npu_clk_disable(exynos->dev, CLK_INDEX(npu));
	return 0;
}

int npu_exynos_clk_dump(struct npu_exynos *exynos)
{
	int ret = 0;
	struct npu_reg *reg;
	void *npu0cmu_base;
	void *npu1cmu_base;
	u32 val;

	npu0cmu_base = ioremap_nocache(0x15A80000, 0x2000);
	if (!npu0cmu_base) {
		npu_err("fail(npu0cmu_base) in ioremap\n");
		ret = -EINVAL;
		goto p_err;
	}

	npu1cmu_base = ioremap_nocache(0x15A80000, 0x2000);
	if (!npu1cmu_base) {
		npu_err("fail(npu1cmu_base) in ioremap\n");
		ret = -EINVAL;
		goto p_err;
	}

	reg = &npu0_cmuclk_regs[MUX_CLKCMU_NPU0_BUS];
	npu_readl(npu0cmu_base, reg, &val);
	npu_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, val);

	reg = &npu0_cmuclk_regs[MUX_CLKCMU_NPU0_CPU];
	npu_readl(npu0cmu_base, reg, &val);
	npu_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, val);

	reg = &npu0_cmuclk_regs[DIV_CLKCMU_NPU0_BUS];
	npu_readl(npu0cmu_base, reg, &val);
	npu_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, val);

	reg = &npu0_cmuclk_regs[DIV_CLKCMU_NPU0_CPU];
	npu_readl(npu0cmu_base, reg, &val);
	npu_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, val);

	reg = &npu1_cmuclk_regs[MUX_CLKCMU_NPU1_BUS];
	npu_readl(npu1cmu_base, reg, &val);
	npu_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, val);

	reg = &npu1_cmuclk_regs[DIV_CLKCMU_NPU1_BUS];
	npu_readl(npu1cmu_base, reg, &val);
	npu_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, val);

p_err:
	if (npu0cmu_base)
		iounmap(npu0cmu_base);

	if (npu1cmu_base)
		iounmap(npu1cmu_base);

	return ret;
}

int dummy_npu_exynos_clk_cfg(struct npu_exynos *exynos) { return 0; }
int dummy_npu_exynos_clk_on(struct npu_exynos *exynos) { return 0; }
int dummy_npu_exynos_clk_off(struct npu_exynos *exynos) { return 0; }
int dummy_npu_exynos_clk_dump(struct npu_exynos *exynos) { return 0; }

const struct npu_clk_ops npu_clk_ops = {
#ifdef CONFIG_EXYNOS_NPU_HARDWARE
	.clk_cfg		= npu_exynos_clk_cfg,
	.clk_on		= npu_exynos_clk_on,
	.clk_off		= npu_exynos_clk_off,
	.clk_dump	= npu_exynos_clk_dump
#else
	.clk_cfg		= dummy_npu_exynos_clk_cfg,
	.clk_on		= dummy_npu_exynos_clk_on,
	.clk_off		= dummy_npu_exynos_clk_off,
	.clk_dump	= dummy_npu_exynos_clk_dump
#endif
};

int npu_exynos_ctl_cfg(struct npu_exynos *exynos)
{
	return 0;
}

int npu_exynos_ctl_on(struct npu_exynos *exynos)
{

	return 0;
}

int npu_exynos_ctl_off(struct npu_exynos *exynos)
{

	return 0;
}

int npu_exynos_ctl_dump(struct npu_exynos *exynos)
{
	return 0;
}

int dummy_npu_exynos_ctl_cfg(struct npu_exynos *exynos) { return 0; }
int dummy_npu_exynos_ctl_on(struct npu_exynos *exynos) { return 0; }
int dummy_npu_exynos_ctl_off(struct npu_exynos *exynos) { return 0; }
int dummy_npu_exynos_ctl_dump(struct npu_exynos *exynos) { return 0; }

const struct npu_ctl_ops npu_ctl_ops = {
#ifdef CONFIG_EXYNOS_NPU_HARDWARE
	.ctl_cfg		= npu_exynos_ctl_cfg,
	.ctl_on		= npu_exynos_ctl_on,
	.ctl_off		= npu_exynos_ctl_off,
	.ctl_dump	= npu_exynos_ctl_dump
#else
	.ctl_cfg		= dummy_npu_exynos_ctl_cfg,
	.ctl_on		= dummy_npu_exynos_ctl_on,
	.ctl_off		= dummy_npu_exynos_ctl_off,
	.ctl_dump	= dummy_npu_exynos_ctl_dump
#endif
};

