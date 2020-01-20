/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef FIMC_IS_PMIC_H
#define FIMC_IS_PMIC_H

#include <linux/i2c.h>
#include <linux/slab.h>
#include "fimc-is-core.h"
#include "fimc-is-companion.h"

struct comp_pmic_vout {
	int sel;	/* selector, unique value for vout entry and indepedant to dcdc vendor */
	int val;	/* dcdc-specific value for vout register */
	char vout[7];	/* voltage level string */
};

void comp_pmic_init(struct dcdc_power *dcdc, struct i2c_client *client);
#endif