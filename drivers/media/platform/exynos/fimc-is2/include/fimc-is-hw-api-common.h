/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_API_COMMON_H
#define FIMC_IS_HW_API_COMMON_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>

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
	RWS			= 8,
	/* write only*/
	RIW			= 9,
	/* read only latched implementation register */
	ROL			= 10,
};

struct fimc_is_reg {
	unsigned int	 sfr_offset;
	char		 *reg_name;
};

struct fimc_is_field {
	char			*field_name;
	unsigned int		bit_start;
	unsigned int		bit_width;
	enum regdata_type	type;
	int			reset;
};

u32 fimc_is_hw_get_reg(void __iomem *base_addr, const struct fimc_is_reg *reg);
void fimc_is_hw_set_reg(void __iomem *base_addr, const struct fimc_is_reg *reg, u32 val);
u32 fimc_is_hw_get_field(void __iomem *base_addr, const struct fimc_is_reg *reg, const struct fimc_is_field *field);
void fimc_is_hw_set_field(void __iomem *base_addr, const struct fimc_is_reg *reg, const struct fimc_is_field *field, u32 val);
u32 fimc_is_hw_get_field_value(u32 reg_value, const struct fimc_is_field *field);
u32 fimc_is_hw_set_field_value(u32 reg_value, const struct fimc_is_field *field, u32 val);
void fimc_is_hw_dump_regs(void __iomem *base_addr, const struct fimc_is_reg *regs, u32 total_cnt);
#endif
