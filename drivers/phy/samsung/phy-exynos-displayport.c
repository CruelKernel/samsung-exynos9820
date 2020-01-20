/*
 * Samsung EXYNOS SoC series MIPI DISPLAYPORT PHY driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 * Author: Kwangje Kim <kj1.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/spinlock.h>

#define EXYNOS_DISPLAYPORT_PHY_ISO_BYPASS  (1 << 0)

struct exynos_displayport_phy {
	struct device *dev;
	spinlock_t slock;
	void __iomem *regs;
	struct regmap *reg_pmu;
	struct displayport_phy_desc {
		struct phy *phy;
		unsigned int iso_offset;
	} phys;
};

/* 1: Isolation bypass, 0: Isolation enable */
static int __set_phy_isolation(struct regmap *reg_pmu,
		unsigned int offset, unsigned int on)
{
	unsigned int val;
	int ret;

	val = on ? EXYNOS_DISPLAYPORT_PHY_ISO_BYPASS : 0;

	ret = regmap_update_bits(reg_pmu, offset,
			EXYNOS_DISPLAYPORT_PHY_ISO_BYPASS, val);

	pr_debug("%s off=0x%x, val=0x%x\n", __func__, offset, val);
	return ret;
}

static const struct of_device_id exynos_displayport_phy_of_table[] = {
	{ .compatible = "samsung,displayport-phy" },
	{ },
};
MODULE_DEVICE_TABLE(of, exynos_displayport_phy_of_table);

#define to_displayport_phy(desc) \
	container_of((desc), struct exynos_displayport_phy, phys)

static int exynos_displayport_phy_power_on(struct phy *phy)
{
	struct displayport_phy_desc *phy_desc = phy_get_drvdata(phy);
	struct exynos_displayport_phy *state = to_displayport_phy(phy_desc);

	return __set_phy_isolation(state->reg_pmu, phy_desc->iso_offset, 1);
}

static int exynos_displayport_phy_power_off(struct phy *phy)
{
	struct displayport_phy_desc *phy_desc = phy_get_drvdata(phy);
	struct exynos_displayport_phy *state = to_displayport_phy(phy_desc);

	return __set_phy_isolation(state->reg_pmu, phy_desc->iso_offset, 0);
}

static struct phy *exynos_displayport_phy_of_xlate(struct device *dev,
					struct of_phandle_args *args)
{
	struct exynos_displayport_phy *state = dev_get_drvdata(dev);

	return state->phys.phy;
}

static struct phy_ops exynos_displayport_phy_ops = {
	.power_on	= exynos_displayport_phy_power_on,
	.power_off	= exynos_displayport_phy_power_off,
	.owner		= THIS_MODULE,
};

static int exynos_displayport_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct exynos_displayport_phy *state;
	struct phy_provider *phy_provider;
	const struct of_device_id *of_id;
	struct phy *generic_phy;
	unsigned int iso[1];
	unsigned int elements;
	int ret = 0;

	state = devm_kzalloc(dev, sizeof(*state), GFP_KERNEL);
	if (!state)
		return -ENOMEM;

	state->dev = &pdev->dev;

	of_id = of_match_device(of_match_ptr(exynos_displayport_phy_of_table), dev);
	if (!of_id)
		return -EINVAL;

	dev_set_drvdata(dev, state);
	spin_lock_init(&state->slock);

	state->reg_pmu = syscon_regmap_lookup_by_phandle(node,
						"samsung,pmu-syscon");
	if (IS_ERR(state->reg_pmu)) {
		dev_err(dev, "Failed to lookup PMU regmap\n");
		return PTR_ERR(state->reg_pmu);
	}

	elements = of_property_count_u32_elems(node, "isolation");
	ret = of_property_read_u32_array(node, "isolation", iso, elements);
	if (ret) {
		dev_err(dev, "cannot get displayport-phy isolation!!!\n");
		return ret;
	}

	state->phys.iso_offset = iso[0];
	dev_dbg(dev, "%s: iso 0x%x\n", __func__, state->phys.iso_offset);

	generic_phy = devm_phy_create(dev, NULL,
				&exynos_displayport_phy_ops);
	if (IS_ERR(generic_phy)) {
		dev_err(dev, "failed to create PHY\n");
		return PTR_ERR(generic_phy);
	}

	state->phys.phy = generic_phy;

	phy_set_drvdata(generic_phy, &state->phys);

	phy_provider = devm_of_phy_provider_register(dev,
			exynos_displayport_phy_of_xlate);

	if (IS_ERR(phy_provider))
		dev_err(dev, "failed to create exynos displayport-phy\n");
	else
		dev_err(dev, "Creating exynos-displayport-phy\n");

	return PTR_ERR_OR_ZERO(phy_provider);
}

static int exynos_displayport_phy_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_info(dev, "%s, successfully removed\n", __func__);
	return 0;
}

static struct platform_driver exynos_displayport_phy_driver = {
	.probe	= exynos_displayport_phy_probe,
	.remove = exynos_displayport_phy_remove,
	.driver = {
		.name  = "exynos-displayport-phy",
		.of_match_table = of_match_ptr(exynos_displayport_phy_of_table),
		.suppress_bind_attrs = true,
	}
};
module_platform_driver(exynos_displayport_phy_driver);

MODULE_DESCRIPTION("Samsung EXYNOS SoC DISPLAYPORT PHY driver");
MODULE_LICENSE("GPL v2");
