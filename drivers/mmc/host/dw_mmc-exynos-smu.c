/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/mmc/host.h>
#include <linux/mmc/dw_mmc.h>
#include <crypto/smu.h>

#include "dw_mmc.h"
#include "dw_mmc-exynos.h"

#define EXYNOS_MMC_SMU_LABEL	"mmc-exynos-smu"

static void exynos_mmc_smu_entry0_init(struct dw_mci *host)
{
	mci_writel(host, MPSBEGIN0, 0);
	mci_writel(host, MPSEND0, 0xffffffff);
	mci_writel(host, MPSLUN0, 0xff);
	mci_writel(host, MPSCTRL0, DWMCI_MPSCTRL_BYPASS);
}

static struct exynos_smu_variant_ops *exynos_mmc_smu_get_vops(struct device *dev)
{
	struct exynos_smu_variant_ops *smu_vops = NULL;
	struct device_node *node;

	node = of_parse_phandle(dev->of_node, EXYNOS_MMC_SMU_LABEL, 0);
	if (!node) {
		dev_err(dev, "%s: exynos-mmc-smu property not specified\n",
			__func__);
		goto err;
	}

	smu_vops = exynos_smu_get_variant_ops(node);
	if (!smu_vops)
		dev_err(dev, "%s: Fail to get smu_vops\n", __func__);

	of_node_put(node);
err:
	return smu_vops;
}

static struct platform_device *exynos_mmc_smu_get_pdevice(struct device *dev)
{
	struct device_node *node;
	struct platform_device *smu_pdev = NULL;

	node = of_parse_phandle(dev->of_node, EXYNOS_MMC_SMU_LABEL, 0);
	if (!node) {
		dev_err(dev, "%s: exynos-mmc-smu property not specified\n",
			__func__);
		goto err;
	}

	smu_pdev = exynos_smu_get_pdevice(node);
err:
	return smu_pdev;
}

int exynos_mmc_smu_get_dev(struct dw_mci *host)
{
	int ret;
	struct device *dev;
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (!host || !host->dev) {
		pr_err("%s: invalid mmc host, host->dev\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	dev = host->dev;
	priv->smu.vops = exynos_mmc_smu_get_vops(dev);
	priv->smu.pdev = exynos_mmc_smu_get_pdevice(dev);

	if (priv->smu.pdev == ERR_PTR(-EPROBE_DEFER)) {
		dev_err(dev, "%s: SMU device not probed yet\n", __func__);
		ret = -EPROBE_DEFER;
		goto err;
	}

	if (!priv->smu.pdev || !priv->smu.vops) {
		dev_err(dev, "%s: Host device doesn't have SMU or fail to get SMU",
				__func__);
		ret = -ENODEV;
		goto err;
	}

	return 0;
err:
	priv->smu.pdev = NULL;
	priv->smu.vops = NULL;

	return ret;
}

int exynos_mmc_smu_init(struct dw_mci *host)
{
	struct smu_data_setting smu_set;
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (!priv->smu.vops || !priv->smu.pdev) {
		exynos_mmc_smu_entry0_init(host);
		return 0;
	}

	smu_set.id = SMU_EMBEDDED;
	smu_set.command = SMU_INIT;
	return priv->smu.vops->init(priv->smu.pdev, &smu_set);
}

int exynos_mmc_smu_sec_cfg(struct dw_mci *host)
{
	struct smu_data_setting smu_set;
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (!priv->smu.vops || !priv->smu.pdev)
		return 0;

	smu_set.id = SMU_EMBEDDED;
	smu_set.desc_type = CFG_DESCTYPE;
	return priv->smu.vops->sec_config(priv->smu.pdev, &smu_set);
}

int exynos_mmc_smu_resume(struct dw_mci *host)
{
	struct smu_data_setting smu_set;
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (!priv->smu.vops || !priv->smu.pdev) {
		exynos_mmc_smu_entry0_init(host);
		return 0;
	}

	smu_set.id = SMU_EMBEDDED;
	return priv->smu.vops->resume(priv->smu.pdev, &smu_set);
}

int exynos_mmc_smu_abort(struct dw_mci *host)
{
	struct smu_data_setting smu_set;
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (!priv->smu.vops || !priv->smu.pdev)
		return 0;

	smu_set.id = SMU_EMBEDDED;
	smu_set.command = SMU_ABORT;
	return priv->smu.vops->abort(priv->smu.pdev, &smu_set);
}
