/*
 * SMU (Secure Management Unit) driver for Exynos
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/printk.h>
#include <linux/smc.h>
#include <crypto/smu.h>

int exynos_smu_init(int id, int smu_cmd)
{
	int ret = exynos_smc(SMC_CMD_SMU, smu_cmd, id, 0);

	if (ret)
		pr_err("%s: Fail smc call for SMU init. ret(%d)\n",
				__func__, ret);
	return ret;
}

int exynos_smu_resume(int id)
{
	int ret = exynos_smc(SMC_CMD_FMP_SMU_RESUME, 0, id, 0);

	if (ret)
		pr_err("%s: Fail smc call for SMU resume. ret(%d)\n",
				__func__, ret);
	return ret;
}

int exynos_smu_abort(int id, int smu_cmd)
{
	int ret = exynos_smc(SMC_CMD_SMU, smu_cmd, id, 0);

	if (ret)
		pr_err("%s: Fail smc call for SMU abort. ret(%d)\n",
			__func__, ret);
	return ret;
}
