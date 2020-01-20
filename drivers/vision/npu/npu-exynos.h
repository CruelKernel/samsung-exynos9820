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

#ifndef NPU_EXYNOS_H_
#define NPU_EXYNOS_H_

#include <linux/clk.h>
#include <linux/device.h>

struct npu_exynos;

struct npu_regblock {
	unsigned int		offset;
	unsigned int		blocks;
	char				*name;
};

enum  regdata_type {
	/* read write */
	RW			= 0,
	/* read only */
	RO			= 1,
	/* write only */
	WO			= 2,
	/* write input */
	WI			= 2,
	/* clear after read */
	RAC			= 3,
	/* write 1 -> clear */
	W1C			= 4,
	/* write read input */
	WRI			= 5,
	/* write input */
	RWI			= 5,
	/* only scaler */
	R_W			= 6,
	/* read & write for clear */
	RWC			= 7,
	/* read & write as dual setting */
	RWS
};

struct npu_reg {
	unsigned int		offset;
	char			*name;
};

struct npu_field {
	char			*name;
	unsigned int		bit_start;
	unsigned int		bit_width;
	enum regdata_type	type;
};

struct npu_clk {
	const char	*name;
	struct clk	*clk;
};

struct npu_clk_ops {
	int (*clk_cfg)(struct npu_exynos *exynos);
	int (*clk_on)(struct npu_exynos *exynos);
	int (*clk_off)(struct npu_exynos *exynos);
	int (*clk_dump)(struct npu_exynos *exynos);
};

struct npu_ctl_ops {
	int (*ctl_cfg)(struct npu_exynos *exynos);
	int (*ctl_on)(struct npu_exynos *exynos);
	int (*ctl_off)(struct npu_exynos *exynos);
	int (*ctl_dump)(struct npu_exynos *exynos);
};

struct npu_exynos {
	struct device				*dev;
	void						*regbase;
	void						*ram0base;
	void						*ram1base;
	struct pinctrl				*pinctrl;
	const struct npu_clk_ops	*clk_ops;
	const struct npu_ctl_ops	*ctl_ops;
};

int npu_exynos_probe(struct npu_exynos *exynos, struct device *dev,
	void *regs, void *ram0, void *ram1);

void npu_readl(void __iomem *base_addr, struct npu_reg *reg, u32 *val);
void npu_writel(void __iomem *base_addr, struct npu_reg *reg, u32 val);

#define CLK_OP(exynos, op) (exynos->clk_ops ? exynos->clk_ops->op(exynos) : 0)
#define CTL_OP(exynos, op) (exynos->ctl_ops ? exynos->ctl_ops->op(exynos) : 0)

#endif
