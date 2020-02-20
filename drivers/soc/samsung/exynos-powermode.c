/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS Power mode
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of.h>
#include <linux/slab.h>

#include <soc/samsung/exynos-pm.h>
#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/exynos-powermode.h>

struct exynos_powermode_info {
	/*
	 * While system boot, wakeup_mask and idle_ip_mask is intialized with
	 * device tree. These are used by system power mode.
	 */
	unsigned int	num_wakeup_mask;
	unsigned int	*wakeup_mask_offset;
	unsigned int	*wakeup_stat_offset;
	unsigned int	*wakeup_mask[NUM_SYS_POWERDOWN];
};

static struct exynos_powermode_info *pm_info;

/******************************************************************************
 *                              System power mode                             *
 ******************************************************************************/
int exynos_system_idle_enter(void)
{
	int ret;

	ret = exynos_prepare_sys_powerdown(SYS_SICD);
	if (ret)
		return ret;

	exynos_pm_notify(SICD_ENTER);

	return 0;
}

void exynos_system_idle_exit(int cancel)
{
	exynos_pm_notify(SICD_EXIT);

	exynos_wakeup_sys_powerdown(SYS_SICD, cancel);
}

#define PMU_EINT_WAKEUP_MASK	0x60C
#define PMU_EINT_WAKEUP_MASK2	0x61C
static void exynos_set_wakeupmask(enum sys_powerdown mode)
{
	int i;
	u64 eintmask = exynos_get_eint_wake_mask();

	/* Set external interrupt mask */
	exynos_pmu_write(PMU_EINT_WAKEUP_MASK, (u32)eintmask);
	exynos_pmu_write(PMU_EINT_WAKEUP_MASK2, (u32)(eintmask >> 32));

	for (i = 0; i < pm_info->num_wakeup_mask; i++) {
		exynos_pmu_write(pm_info->wakeup_stat_offset[i], 0);

		if ((mode == SYS_SICD || ((mode == SYS_SLEEP_USBL2) && (otg_is_connect() != 1))) && (i == 1))
			exynos_pmu_write(pm_info->wakeup_mask_offset[1], 0);
		else
			exynos_pmu_write(pm_info->wakeup_mask_offset[i],
					pm_info->wakeup_mask[mode][i]);
	}
}

int exynos_prepare_sys_powerdown(enum sys_powerdown mode)
{
	int ret;

	exynos_set_wakeupmask(mode);

	ret = cal_pm_enter(mode);
	if (ret)
		pr_err("CAL Fail to set powermode\n");

	return ret;
}

void exynos_wakeup_sys_powerdown(enum sys_powerdown mode, bool early_wakeup)
{
	if (early_wakeup)
		cal_pm_earlywakeup(mode);
	else
		cal_pm_exit(mode);
}

/******************************************************************************
 *                            Driver initialization                           *
 ******************************************************************************/

#define for_each_syspwr_mode(mode)                              \
       for ((mode) = 0; (mode) < NUM_SYS_POWERDOWN; (mode)++)

static int alloc_wakeup_mask(int num_wakeup_mask)
{
	unsigned int mode;

	pm_info->wakeup_mask_offset = kzalloc(sizeof(unsigned int)
				* num_wakeup_mask, GFP_KERNEL);
	if (!pm_info->wakeup_mask_offset)
		return -ENOMEM;

	pm_info->wakeup_stat_offset = kzalloc(sizeof(unsigned int)
				* num_wakeup_mask, GFP_KERNEL);
	if (!pm_info->wakeup_stat_offset)
		return -ENOMEM;

	for_each_syspwr_mode(mode) {
		pm_info->wakeup_mask[mode] = kzalloc(sizeof(unsigned int)
				* num_wakeup_mask, GFP_KERNEL);

		if (!pm_info->wakeup_mask[mode])
			goto free_reg_offset;
	}

	return 0;

free_reg_offset:
	for_each_syspwr_mode(mode)
		if (pm_info->wakeup_mask[mode])
			kfree(pm_info->wakeup_mask[mode]);

	kfree(pm_info->wakeup_mask_offset);
	kfree(pm_info->wakeup_stat_offset);

	return -ENOMEM;
}

static int parsing_dt_wakeup_mask(struct device_node *np)
{
	int ret;
	struct device_node *root, *child;
	unsigned int mode, mask_index = 0;

	root = of_find_node_by_name(np, "wakeup-masks");
	pm_info->num_wakeup_mask = of_get_child_count(root);

	ret = alloc_wakeup_mask(pm_info->num_wakeup_mask);
	if (ret)
		return ret;

	for_each_child_of_node(root, child) {
		for_each_syspwr_mode(mode) {
			ret = of_property_read_u32_index(child, "mask",
				mode, &pm_info->wakeup_mask[mode][mask_index]);
			if (ret)
				return ret;
		}

		ret = of_property_read_u32(child, "mask-offset",
				&pm_info->wakeup_mask_offset[mask_index]);
		if (ret)
			return ret;

		ret = of_property_read_u32(child, "stat-offset",
				&pm_info->wakeup_stat_offset[mask_index]);
		if (ret)
			return ret;

		mask_index++;
	}

	return 0;
}

static int __init exynos_powermode_init(void)
{
	struct device_node *np;
	int ret;

	pm_info = kzalloc(sizeof(struct exynos_powermode_info), GFP_KERNEL);
	if (pm_info == NULL) {
		pr_err("%s: failed to allocate exynos_powermode_info\n", __func__);
		return -ENOMEM;
	}

	np = of_find_node_by_name(NULL, "exynos-powermode");
	ret = parsing_dt_wakeup_mask(np);
	if (ret)
		pr_warn("Fail to initialize the wakeup mask with err = %d\n", ret);

	return 0;
}
arch_initcall(exynos_powermode_init);
