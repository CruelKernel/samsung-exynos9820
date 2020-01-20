/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * EXYNOS - Security Dump Manager support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/smc.h>
#include <linux/printk.h>

int exynos_sdm_dump_secure_region(void)
{
	int ret;

	ret = exynos_smc(SMC_CMD_DUMP_SECURE_REGION, 0, 0, 0);
	pr_info("%s: 0x%x\n", __func__, ret);

	return ret;
}

int exynos_sdm_flush_secdram(void)
{
	int ret;

	ret = exynos_smc(SMC_CMD_FLUSH_SECDRAM, 0, 0, 0);

	return ret;
}
