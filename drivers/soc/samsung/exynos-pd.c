/*
 * Exynos PM domain support for PMUCAL 3.0 interface.
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Implementation of Exynos specific power domain control which is used in
 * conjunction with runtime-pm.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <soc/samsung/exynos-pd.h>
#include <soc/samsung/bts.h>
#include <soc/samsung/cal-if.h>
#include <linux/apm-exynos.h>
#include <linux/sec_debug.h>
struct exynos_pm_domain *exynos_pd_lookup_name(const char *domain_name)
{
	struct exynos_pm_domain *exypd = NULL;
	struct device_node *np;

	if (IS_ERR_OR_NULL(domain_name))
		return NULL;

	for_each_compatible_node(np, NULL, "samsung,exynos-pd") {
		struct platform_device *pdev;
		struct exynos_pm_domain *pd;

		if (of_device_is_available(np)) {
			pdev = of_find_device_by_node(np);
			if (!pdev)
				continue;
			pd = platform_get_drvdata(pdev);
			if (!strcmp(pd->name, domain_name)) {
				exypd = pd;
				break;
			}
		}
	}
	return exypd;
}
EXPORT_SYMBOL(exynos_pd_lookup_name);

int exynos_pd_status(struct exynos_pm_domain *pd)
{
	int status;

	if (unlikely(!pd))
		return -EINVAL;

	mutex_lock(&pd->access_lock);
	status = cal_pd_status(pd->cal_pdid);
	mutex_unlock(&pd->access_lock);

	return status;
}
EXPORT_SYMBOL(exynos_pd_status);
/* Power domain on sequence.
 * on_pre, on_post functions are registered as notification handler at CAL code.
 */
static void exynos_pd_power_on_pre(struct exynos_pm_domain *pd)
{
	exynos_update_ip_idle_status(pd->idle_ip_index, 0);

	if (pd->devfreq_index >= 0) {
		exynos_bts_scitoken_setting(true);
	}
}

static void exynos_pd_power_on_post(struct exynos_pm_domain *pd)
{
}

static void exynos_pd_power_off_pre(struct exynos_pm_domain *pd)
{
#ifdef CONFIG_EXYNOS_CL_DVFS_G3D
	if (!strcmp(pd->name, "pd-g3d")) {
		exynos_g3d_power_down_noti_apm();
	}
#endif /* CONFIG_EXYNOS_CL_DVFS_G3D */
}

static void exynos_pd_power_off_post(struct exynos_pm_domain *pd)
{
	exynos_update_ip_idle_status(pd->idle_ip_index, 1);

	if (pd->devfreq_index >= 0) {
		exynos_bts_scitoken_setting(false);
	}
}

static void exynos_pd_prepare_forced_off(struct exynos_pm_domain *pd)
{
}

static int exynos_pd_power_on(struct generic_pm_domain *genpd)
{
	struct exynos_pm_domain *pd = container_of(genpd, struct exynos_pm_domain, genpd);
	int ret = 0;

	mutex_lock(&pd->access_lock);

	DEBUG_PRINT_INFO("%s(%s)+\n", __func__, pd->name);

	if (unlikely(!pd->pd_control)) {
		pr_debug(EXYNOS_PD_PREFIX "%s is Logical sub power domain, dose not have to power on control\n", pd->name);
		goto acc_unlock;
	}

	if (pd->power_down_skipped) {
		pr_info(EXYNOS_PD_PREFIX "%s power-on is skipped.\n", pd->name);
		goto acc_unlock;
	}

	exynos_pd_power_on_pre(pd);

	ret = pd->pd_control(pd->cal_pdid, 1);
	if (ret) {
		pr_err(EXYNOS_PD_PREFIX "%s cannot be powered on\n", pd->name);
		exynos_pd_power_off_post(pd);
		ret = -EAGAIN;
		goto acc_unlock;
	}

	exynos_pd_power_on_post(pd);

acc_unlock:
	DEBUG_PRINT_INFO("%s(%s)-, ret = %d\n", __func__, pd->name, ret);
	mutex_unlock(&pd->access_lock);

	return ret;
}

static int exynos_pd_power_off(struct generic_pm_domain *genpd)
{
	struct exynos_pm_domain *pd = container_of(genpd, struct exynos_pm_domain, genpd);
	int ret = 0;

	mutex_lock(&pd->access_lock);

	DEBUG_PRINT_INFO("%s(%s)+\n", __func__, pd->name);

	if (unlikely(!pd->pd_control)) {
		pr_debug(EXYNOS_PD_PREFIX "%s is Logical sub power domain, dose not have to power off control\n", genpd->name);
		goto acc_unlock;
	}

	if (pd->power_down_ok && !pd->power_down_ok()) {
		pr_info(EXYNOS_PD_PREFIX "%s power-off is skipped.\n", pd->name);
		pd->power_down_skipped = true;
		goto acc_unlock;
	}

	exynos_pd_power_off_pre(pd);

	ret = pd->pd_control(pd->cal_pdid, 0);
	if (unlikely(ret)) {
		if (ret == -4) {
			pr_err(EXYNOS_PD_PREFIX "Timed out during %s  power off! -> forced power off\n", genpd->name);
			exynos_pd_prepare_forced_off(pd);
			ret = pd->pd_control(pd->cal_pdid, 0);
			if (unlikely(ret)) {
				pr_auto(ASL1, EXYNOS_PD_PREFIX "%s occur error at power off!\n", genpd->name);
				sec_debug_set_extra_info_epd((char *)(genpd->name));
				goto acc_unlock;
			}
		} else {
			pr_auto(ASL1, EXYNOS_PD_PREFIX "%s occur error at power off!!\n", genpd->name);
			sec_debug_set_extra_info_epd((char *)(genpd->name));
			goto acc_unlock;
		}
	}

	exynos_pd_power_off_post(pd);
	pd->power_down_skipped = false;

acc_unlock:
	DEBUG_PRINT_INFO("%s(%s)-, ret = %d\n", __func__, pd->name, ret);
	mutex_unlock(&pd->access_lock);

	return ret;
}

#ifdef CONFIG_OF
/**
 *  of_get_devfreq_sync_volt_idx - check devfreq sync voltage idx
 *
 *  Returns the index if the "devfreq-sync-voltage" is described in DT pd node,
 *  -ENOENT otherwise.
 */
static int of_get_devfreq_sync_volt_idx(const struct device_node *device)
{
	int ret;
	u32 val;

	ret = of_property_read_u32(device, "devfreq-sync-voltage", &val);
	if (ret)
		return -ENOENT;

	return val;
}

static bool exynos_pd_power_down_ok_aud(void)
{
#ifdef CONFIG_SND_SOC_SAMSUNG_ABOX
#ifdef CONFIG_SOC_EMULATOR8895
	return false;
#else
	return !abox_is_on();
#endif
#else
	return true;
#endif
}

static bool exynos_pd_power_down_ok_vts(void)
{
#ifdef CONFIG_SND_SOC_SAMSUNG_VTS
	return !vts_is_on();
#else
	return true;
#endif
}

static bool exynos_pd_power_down_ok_usb(void)
{
#ifdef CONFIG_USB_DWC3_EXYNOS
	return !otg_is_connect();
#else
	return true;
#endif
}

static void of_get_power_down_ok(struct exynos_pm_domain *pd)
{
	int ret;
	u32 val;
	struct device_node *device = pd->of_node;

	ret = of_property_read_u32(device, "power-down-ok", &val);
	if (ret)
		return ;
	else {
		switch (val) {
		case PD_OK_AUD:
			pd->power_down_ok = exynos_pd_power_down_ok_aud;
			break;
		case PD_OK_VTS:
			pd->power_down_ok = exynos_pd_power_down_ok_vts;
			break;
		case PD_OK_USB:
			pd->power_down_ok = exynos_pd_power_down_ok_usb;
			break;
		default:
			break;
		}
	}
}

static int exynos_pd_genpd_init(struct exynos_pm_domain *pd, int state)
{
	pd->genpd.name = pd->name;
	pd->genpd.power_off = exynos_pd_power_off;
	pd->genpd.power_on = exynos_pd_power_on;

	/* pd power on/off latency is less than 1ms */
	pm_genpd_init(&pd->genpd, NULL, state ? false : true);

	pd->genpd.states = kzalloc(sizeof(struct genpd_power_state), GFP_KERNEL);

	if (!pd->genpd.states)
		return -ENOMEM;

	pd->genpd.states[0].power_on_latency_ns = 1000000;
	pd->genpd.states[0].power_off_latency_ns = 1000000;

	return 0;
}

/* exynos_pd_show_power_domain - show current power domain status.
 *
 * read the status of power domain and show it.
 */
static void exynos_pd_show_power_domain(void)
{
	struct device_node *np;
	for_each_compatible_node(np, NULL, "samsung,exynos-pd") {
		struct platform_device *pdev;
		struct exynos_pm_domain *pd;

		if (of_device_is_available(np)) {
			pdev = of_find_device_by_node(np);
			if (!pdev)
				continue;
			pd = platform_get_drvdata(pdev);
			pr_info("   %-9s - %-3s\n", pd->genpd.name,
					cal_pd_status(pd->cal_pdid) ? "on" : "off");
		} else
			pr_info("   %-9s - %s\n", np->name, "on,  always");
	}

	return;
}

static __init int exynos_pd_dt_parse(void)
{
	struct platform_device *pdev = NULL;
	struct device_node *np = NULL;
	int ret = 0;

	for_each_compatible_node(np, NULL, "samsung,exynos-pd") {
		struct exynos_pm_domain *pd;
		struct device_node *children;
		int initial_state;

		/* skip unmanaged power domain */
		if (!of_device_is_available(np))
			continue;

		pdev = of_find_device_by_node(np);

		pd = kzalloc(sizeof(*pd), GFP_KERNEL);
		if (!pd) {
			pr_err(EXYNOS_PD_PREFIX "%s: failed to allocate memory for domain\n",
					__func__);
			return -ENOMEM;
		}

		/* init exynos_pm_domain's members  */
		pd->name = kstrdup(np->name, GFP_KERNEL);
		ret = of_property_read_u32(np, "cal_id", (u32 *)&pd->cal_pdid);
		if (ret) {
			pr_err(EXYNOS_PD_PREFIX "%s: failed to get cal_pdid  from of %s\n",
					__func__, pd->name);
			return -ENODEV;
		}
		pd->of_node = np;
		pd->pd_control = cal_pd_control;
		pd->check_status = exynos_pd_status;
		pd->devfreq_index = of_get_devfreq_sync_volt_idx(pd->of_node);
		of_get_power_down_ok(pd);
		pd->power_down_skipped = false;

		ret = of_property_read_u32(np, "need_smc", (u32 *)&pd->need_smc);
		if (ret) {
			pd->need_smc = 0x0;
		} else {
			cal_pd_set_smc_id(pd->cal_pdid, pd->need_smc);
			pr_info(EXYNOS_PD_PREFIX "%s: %s read need_smc 0x%x successfully.!\n",
					__func__, pd->name, pd->need_smc);
		}
		initial_state = cal_pd_status(pd->cal_pdid);
		if (initial_state == -1) {
			pr_err(EXYNOS_PD_PREFIX "%s: %s is in unknown state\n",
					__func__, pd->name);
			return -EINVAL;
		}

		pd->idle_ip_index = exynos_get_idle_ip_index(pd->name);

		mutex_init(&pd->access_lock);
		platform_set_drvdata(pdev, pd);

		ret = exynos_pd_genpd_init(pd, initial_state);
		if (ret) {
			pr_err(EXYNOS_PD_PREFIX "%s: exynos_pd_genpd_init fail: %s, ret:%d\n",
					__func__, pd->name, ret);
			return ret;
		}

		of_genpd_add_provider_simple(np, &pd->genpd);

		/* add LOGICAL sub-domain
		 * It is not assumed that there is REAL sub-domain.
		 * Power on/off functions are not defined here.
		 */
		for_each_child_of_node(np, children) {
			struct exynos_pm_domain *sub_pd;
			struct platform_device *sub_pdev;

			sub_pd = kzalloc(sizeof(*sub_pd), GFP_KERNEL);
			if (!sub_pd) {
				pr_err("%s %s: failed to allocate memory for power domain\n",
						EXYNOS_PD_PREFIX, __func__);
				return -ENOMEM;
			}

			sub_pd->name = kstrdup(children->name, GFP_KERNEL);
			sub_pd->of_node = children;

			/* Logical sub-domain does not have to power on/off control*/
			sub_pd->pd_control = NULL;

			sub_pd->devfreq_index = of_get_devfreq_sync_volt_idx(sub_pd->of_node);

			/* kernel does not create sub-domain pdev. */
			sub_pdev = of_find_device_by_node(children);
			if (!sub_pdev)
				sub_pdev = of_platform_device_create(children, NULL, &pdev->dev);
			if (!sub_pdev) {
				pr_err(EXYNOS_PD_PREFIX "sub domain allocation failed: %s\n", children->name);
				continue;
			}

			mutex_init(&sub_pd->access_lock);
			platform_set_drvdata(sub_pdev, sub_pd);

			ret = exynos_pd_genpd_init(sub_pd, initial_state);
			if (ret) {
				pr_err(EXYNOS_PD_PREFIX "%s: exynos_pd_genpd_init fail: %s, ret:%d\n",
						__func__, pd->name, ret);
				return ret;
			}

			of_genpd_add_provider_simple(children, &sub_pd->genpd);

			if (pm_genpd_add_subdomain(&pd->genpd, &sub_pd->genpd))
				pr_err("%s %s can't add subdomain %s\n",
					EXYNOS_PD_PREFIX, pd->genpd.name, sub_pd->genpd.name);
			else
				pr_info(EXYNOS_PD_PREFIX "%s has a new logical child %s.\n",
						pd->genpd.name, sub_pd->genpd.name);
		}
	}

	/* EXCEPTION: add physical sub-pd to master pd using device tree */
	for_each_compatible_node(np, NULL, "samsung,exynos-pd") {
		struct exynos_pm_domain *parent_pd, *child_pd;
		struct device_node *parent;
		struct platform_device *parent_pd_pdev, *child_pd_pdev;
		int i;

		/* skip unmanaged power domain */
		if (!of_device_is_available(np))
			continue;

		/* child_pd_pdev should have value. */
		child_pd_pdev = of_find_device_by_node(np);
		child_pd = platform_get_drvdata(child_pd_pdev);

		/* search parents in device tree */
		for (i = 0; i < MAX_PARENT_POWER_DOMAIN; i++) {
			/* find parent node if available */
			parent = of_parse_phandle(np, "parent", i);
			if (!parent)
				break;

			/* display error when parent is unmanaged. */
			if (!of_device_is_available(parent)) {
				pr_err(EXYNOS_PD_PREFIX "%s is not managed by runtime pm.\n", parent->name);
				continue;
			}

			/* parent_pd_pdev should have value. */
			parent_pd_pdev = of_find_device_by_node(parent);
			parent_pd = platform_get_drvdata(parent_pd_pdev);

			if (pm_genpd_add_subdomain(&parent_pd->genpd, &child_pd->genpd))
				pr_err(EXYNOS_PD_PREFIX "%s cannot add subdomain %s\n",
						parent_pd->name, child_pd->name);
			else
				pr_info(EXYNOS_PD_PREFIX "%s has a new child %s.\n",
						parent_pd->name, child_pd->name);
		}
	}

	return 0;
}
#endif /* CONFIG_OF */

static int __init exynos_pd_init(void)
{
	int ret;
#ifdef CONFIG_OF
	if (of_have_populated_dt()) {
		ret = exynos_pd_dt_parse();
		if (ret) {
			pr_err(EXYNOS_PD_PREFIX "dt parse failed.\n");
			return ret;
		}

		pr_info("%s PM Domain Initialize\n", EXYNOS_PD_PREFIX);
		/* show information of power domain registration */
		exynos_pd_show_power_domain();

		return 0;
	}
#endif
	pr_err(EXYNOS_PD_PREFIX "PM Domain works along with Device Tree\n");
	return -EPERM;
}
subsys_initcall(exynos_pd_init);
