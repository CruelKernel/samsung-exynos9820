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

#include "npu-log.h"

#include "npu-config.h"
#include "npu-exynos.h"

extern struct npu_clk npu_clk_array[];
extern const u32 npu_clk_array_size;
extern const struct npu_clk_ops npu_clk_ops;
extern const struct npu_ctl_ops npu_ctl_ops;

int npu_clk_set_rate(struct device *dev, u32 index, ulong frequency)
{
	int ret = 0;
	struct clk *clk;

	if (index >= npu_clk_array_size) {
		npu_err("invalid(%d >= %d) index\n", index, npu_clk_array_size);
		ret = -EINVAL;
		goto p_err;
	}

	clk = npu_clk_array[index].clk;
	if (IS_ERR_OR_NULL(clk)) {
		npu_err("null(%d) in clk\n", index);
		ret = -EINVAL;
		goto p_err;
	}

	ret = clk_set_rate(clk, frequency);
	if (ret) {
		npu_err("fail(%d) in clk_set_rate\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

ulong npu_clk_get_rate(struct device *dev, u32 index)
{
	ulong frequency;
	struct clk *clk;

	if (index >= npu_clk_array_size) {
		npu_err("invalid(%d >= %d) index\n", index, npu_clk_array_size);
		frequency = -EINVAL;
		goto p_err;
	}

	clk = npu_clk_array[index].clk;
	if (IS_ERR_OR_NULL(clk)) {
		npu_err("null(%d) in clk\n", index);
		frequency = -EINVAL;
		goto p_err;
	}

	frequency = clk_get_rate(clk);

p_err:
	npu_info("%s : %ld(Mhz)\n", npu_clk_array[index].name, frequency/1000000);
	return frequency;
}

int  npu_clk_enable(struct device *dev, u32 index)
{
	int ret = 0;
	struct clk *clk;

	if (index >= npu_clk_array_size) {
		npu_err("invalid(%d >= %d) index\n", index, npu_clk_array_size);
		ret = -EINVAL;
		goto p_err;
	}

	clk = npu_clk_array[index].clk;
	if (IS_ERR_OR_NULL(clk)) {
		npu_err("null(%d) in clk\n", index);
		ret = -EINVAL;
		goto p_err;
	}

	ret = clk_prepare_enable(clk);
	if (ret) {
		npu_err("fail(%s) in clk_prepare_enable\n", npu_clk_array[index].name);
		goto p_err;
	}

p_err:
	return ret;
}

int npu_clk_disable(struct device *dev, u32 index)
{
	int ret = 0;
	struct clk *clk;

	if (index >= npu_clk_array_size) {
		npu_err("invalid(%d >= %d) index\n", index, npu_clk_array_size);
		ret = -EINVAL;
		goto p_err;
	}

	clk = npu_clk_array[index].clk;
	if (IS_ERR_OR_NULL(clk)) {
		npu_err("null(%d) in clk\n", index);
		ret = -EINVAL;
		goto p_err;
	}

	clk_disable_unprepare(clk);

p_err:
	return ret;
}

static int npu_exynos_clk_init(struct device *dev)
{
	int ret = 0;
	const char *name;
	struct clk *clk;
	u32 index;

	for (index = 0; index < npu_clk_array_size; ++index) {
		name = npu_clk_array[index].name;
		if (!name) {
			probe_err("name is NULL\n");
			ret = -EINVAL;
			break;
		}

		clk = clk_get(dev, name);
		if (IS_ERR_OR_NULL(clk)) {
			probe_err("%s clk is not found\n", name);
			ret = -EINVAL;
			break;
		}

		npu_clk_array[index].clk = clk;
	}

	return ret;
}

int npu_exynos_probe(struct npu_exynos *exynos, struct device *dev,
	void *regs, void *ram0, void *ram1)
{
	int ret = 0;

	BUG_ON(!exynos);
	BUG_ON(!dev);
	exynos->regbase = regs;
	exynos->ram0base = ram0;
	exynos->ram1base = ram1;
	exynos->clk_ops = &npu_clk_ops;
	//exynos->ctl_ops = &npu_ctl_ops;
	exynos->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(exynos->pinctrl)) {
		probe_err("devm_pinctrl_get is fail");
		ret = PTR_ERR(exynos->pinctrl);
		goto p_err;
	}

	ret = npu_exynos_clk_init(dev);
	if (ret) {
		probe_err("npu_exynos_clk_init is fail(%d)", ret);
		goto p_err;
	}
	BUG_ON(!exynos);
	BUG_ON(!dev);
p_err:
	return ret;
}

void npu_readl(void __iomem *base_addr, struct npu_reg *reg, u32 *val)
{
	*val = readl(base_addr + reg->offset);

	npu_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, *val);
}

void npu_writel(void __iomem *base_addr, struct npu_reg *reg, u32 val)
{
	npu_info("[REG][%s][0x%04X], val(W):[0x%08X]\n", reg->name, reg->offset, val);
	writel(val, base_addr + reg->offset);
}

void npu_readf(void __iomem *base_addr, struct npu_reg *reg, struct npu_field *field, u32 *val)
{
	*val = (readl(base_addr + reg->offset) >> (field->bit_start)) & ((1 << (field->bit_width)) - 1);
	npu_info("[REG][%s][%s][0x%04X], val(R):[0x%08X]\n", reg->name, field->name, reg->offset, *val);
}

void npu_writef(void __iomem *base_addr, struct npu_reg *reg, struct npu_field *field, u32 val)
{
	u32 mask, temp;

	mask = ((1 << field->bit_width) - 1);
	temp = readl(base_addr + reg->offset) & ~(mask << field->bit_start);
	temp |= (val & mask) << (field->bit_start);

	npu_info("[REG][%s][%s][0x%04X], val(W):[0x%08X]\n", reg->name, field->name, reg->offset, val);
	writel(temp, base_addr + reg->offset);
}
