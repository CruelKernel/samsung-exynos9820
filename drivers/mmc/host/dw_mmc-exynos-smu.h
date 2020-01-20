/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _MMC_EXYNOS_SMU_H_
#define _MMC_EXYNOS_SMU_H_

#ifdef CONFIG_MMC_DW_EXYNOS_SMU
int exynos_mmc_smu_get_dev(struct dw_mci *host);
int exynos_mmc_smu_init(struct dw_mci *host);
int exynos_mmc_smu_sec_cfg(struct dw_mci *host);
int exynos_mmc_smu_resume(struct dw_mci *host);
int exynos_mmc_smu_abort(struct dw_mci *host);
#else
inline int exynos_mmc_smu_get_dev(struct dw_mci *host)
{
	struct dw_mci_exynos_priv_data *priv;

	if (!host || !host->priv)
		goto out;

	priv = host->priv;
	priv->smu.pdev = NULL;
	priv->smu.vops = NULL;
out:
	return 0;
}

inline int exynos_mmc_smu_init(struct dw_mci *host)
{
#if 0
	mci_writel(host, MPSBEGIN0, 0);
	mci_writel(host, MPSEND0, 0xffffffff);
	mci_writel(host, MPSLUN0, 0xff);
	mci_writel(host, MPSCTRL0, DWMCI_MPSCTRL_BYPASS);
#endif
	return 0;
}

inline int exynos_mmc_smu_sec_cfg(struct dw_mci *host)
{
	return 0;
}

inline int exynos_mmc_smu_resume(struct dw_mci *host)
{
#if 0
	mci_writel(host, MPSBEGIN0, 0);
	mci_writel(host, MPSEND0, 0xffffffff);
	mci_writel(host, MPSLUN0, 0xff);
	mci_writel(host, MPSCTRL0, DWMCI_MPSCTRL_BYPASS);
#endif
	return 0;
}

inline int exynos_mmc_smu_abort(struct dw_mci *host)
{
	return 0;
}
#endif /* CONFIG_MMC_DW_EXYNOS_SMU */
#endif /* _MMC_EXYNOS_SMU_H_ */
