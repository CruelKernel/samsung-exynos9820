/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _MMC_EXYNOS_FMP_H_
#define _MMC_EXYNOS_FMP_H_

#ifdef CONFIG_MMC_DW_EXYNOS_FMP
int exynos_mmc_fmp_cfg(struct dw_mci *host,
				void *desc,
				struct mmc_data *mmc_data,
				struct page *page,
				int sector_offfset,
				bool cmdq_enabled);
int exynos_mmc_fmp_clear(struct dw_mci *host, void *desc,
				bool cmdq_enabled);
#else
inline int exynos_mmc_fmp_cfg(struct dw_mci *host,
				void *desc,
				struct mmc_data *mmc_data,
				struct page *page,
				int sector_offset,
				bool cmdq_enabled)
{
	struct dw_mci_exynos_priv_data *priv;

	if (host) {
		priv = host->priv;
		if (priv) {
			priv->fmp.pdev = NULL;
			priv->fmp.vops = NULL;
		}
	}
	return 0;
}

inline int exynos_mmc_fmp_clear(struct dw_mci *host, void *desc,
				bool cmdq_enabled)
{
	return 0;
}
#endif /* CONFIG_MMC_DW_EXYNOS_FMP */
#endif /* _MMC_EXYNOS_FMP_H_ */
