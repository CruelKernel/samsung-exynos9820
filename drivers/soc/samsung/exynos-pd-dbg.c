/*
 * Exynos PM domain debugfs support.
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include <soc/samsung/exynos-pd.h>

#ifdef CONFIG_DEBUG_FS
static struct dentry *exynos_pd_dbg_root;

static int exynos_pd_dbg_long_test(struct device *dev)
{
	int ret, i;

	pr_info("%s %s: test start.\n", EXYNOS_PD_DBG_PREFIX, __func__);

	if (pm_runtime_enabled(dev) && pm_runtime_active(dev)) {
		ret = pm_runtime_put_sync(dev);
		if (ret) {
			pr_err("%s %s: put sync failed.\n",
					EXYNOS_PD_DBG_PREFIX, __func__);
			return ret;
		}
	}

	for (i = 0; i < 100; i++) {
		ret = pm_runtime_get_sync(dev);
		if (ret) {
			pr_err("%s %s: get sync failed.\n",
					EXYNOS_PD_DBG_PREFIX, __func__);
			return ret;
		}
		mdelay(50);
		ret = pm_runtime_put_sync(dev);
		if (ret) {
			pr_err("%s %s: put sync failed.\n",
					EXYNOS_PD_DBG_PREFIX, __func__);
			return ret;
		}
		mdelay(50);
	}

	pr_info("%s %s: test done.\n", EXYNOS_PD_DBG_PREFIX, __func__);

	return ret;
}

static struct generic_pm_domain *exynos_pd_dbg_dev_to_genpd(struct device *dev)
{
	if (IS_ERR_OR_NULL(dev->pm_domain))
		return ERR_PTR(-EINVAL);

	return pd_to_genpd(dev->pm_domain);
}

static void exynos_pd_dbg_genpd_lock_spin(struct generic_pm_domain *genpd)
	__acquires(&genpd->slock)
{
	unsigned long flags;

	spin_lock_irqsave(&genpd->slock, flags);
	genpd->lock_flags = flags;
}

static void exynos_pd_dbg_genpd_unlock_spin(struct generic_pm_domain *genpd)
	__releases(&genpd->slock)
{
	spin_unlock_irqrestore(&genpd->slock, genpd->lock_flags);
}

static void exynos_pd_dbg_genpd_lock(struct generic_pm_domain *genpd)
{
	if (genpd->flags & GENPD_FLAG_IRQ_SAFE)
		exynos_pd_dbg_genpd_lock_spin(genpd);
	else
		mutex_lock(&genpd->mlock);
}

static void exynos_pd_dbg_genpd_unlock(struct generic_pm_domain *genpd)
{
	if (genpd->flags & GENPD_FLAG_IRQ_SAFE)
		exynos_pd_dbg_genpd_unlock_spin(genpd);
	else
		mutex_unlock(&genpd->mlock);
}

static void exynos_pd_dbg_summary_show(struct generic_pm_domain *genpd)
{
	static const char * const gpd_status_lookup[] = {
		[GPD_STATE_ACTIVE] = "on",
		[GPD_STATE_POWER_OFF] = "off"
	};
	static const char * const rpm_status_lookup[] = {
		[RPM_ACTIVE] = "active",
		[RPM_RESUMING] = "resuming",
		[RPM_SUSPENDED] = "suspended",
		[RPM_SUSPENDING] = "suspending"
	};
	const char *p = "";
	struct pm_domain_data *pm_data;
	struct gpd_link *link;

	exynos_pd_dbg_genpd_lock(genpd);

	if (genpd->status >= ARRAY_SIZE(gpd_status_lookup)) {
		pr_err("%s invalid GPD_STATUS\n", EXYNOS_PD_DBG_PREFIX);
		exynos_pd_dbg_genpd_unlock(genpd);
		return ;
	}

	pr_info("[GENPD] : %-30s [GPD_STATUS] : %-15s\n",
			genpd->name, gpd_status_lookup[genpd->status]);

	list_for_each_entry(pm_data, &genpd->dev_list, list_node) {
		if (pm_data->dev->power.runtime_error)
			p = "error";
		else if (pm_data->dev->power.disable_depth)
			p = "unsupported";
		else if (pm_data->dev->power.runtime_status < ARRAY_SIZE(rpm_status_lookup))
			p = rpm_status_lookup[pm_data->dev->power.runtime_status];
		else
			WARN_ON(1);

		pr_info("\t[DEV] : %-30s [RPM_STATUS] : %-15s\n",
					dev_name(pm_data->dev), p);
	}

	list_for_each_entry(link, &genpd->master_links, master_node)
		exynos_pd_dbg_summary_show(link->slave);

	exynos_pd_dbg_genpd_unlock(genpd);
}

static ssize_t exynos_pd_dbg_read(struct file *file, char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct device *dev = file->private_data;
	struct generic_pm_domain *genpd = exynos_pd_dbg_dev_to_genpd(dev);

	exynos_pd_dbg_summary_show(genpd);

	return 0;
}

static ssize_t exynos_pd_dbg_write(struct file *file, const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct device *dev = file->private_data;
	char buf[32];
	size_t buf_size;

	buf_size = min(count, (sizeof(buf)-1));
	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;

	switch (buf[0]) {
	case '0':
		if (pm_runtime_put_sync(dev))
			pr_err("%s %s: put sync failed.\n",
					EXYNOS_PD_DBG_PREFIX, __func__);
		break;
	case '1':
		if (pm_runtime_get_sync(dev))
			pr_err("%s %s: get sync failed.\n",
					EXYNOS_PD_DBG_PREFIX, __func__);
		break;
	case 'c':
		exynos_pd_dbg_long_test(dev);
		break;
	default:
		pr_err("%s %s: Invalid input ['0'|'1'|'c']\n",
				EXYNOS_PD_DBG_PREFIX, __func__);
		break;
	}

	return count;
}

static const struct file_operations exynos_pd_dbg_fops = {
	.open = simple_open,
	.read = exynos_pd_dbg_read,
	.write = exynos_pd_dbg_write,
	.llseek = default_llseek,
};
#endif

static int exynos_pd_dbg_probe(struct platform_device *pdev)
{
	int ret;
	struct exynos_pd_dbg_info *dbg_info;

	dbg_info = kzalloc(sizeof(struct exynos_pd_dbg_info), GFP_KERNEL);
	if (!dbg_info) {
		pr_err("%s %s: could not allocate mem for dbg_info\n",
				EXYNOS_PD_DBG_PREFIX, __func__);
		ret = -ENOMEM;
		goto err_dbg_info;
	}
	dbg_info->dev = &pdev->dev;
#ifdef CONFIG_DEBUG_FS
	if (!exynos_pd_dbg_root) {
		exynos_pd_dbg_root = debugfs_create_dir("exynos-pd", NULL);
		if (!exynos_pd_dbg_root) {
			pr_err("%s %s: could not create debugfs dir\n",
					EXYNOS_PD_DBG_PREFIX, __func__);
			ret = -ENOMEM;
			goto err_dbgfs_root;
		}
	}

	dbg_info->fops = exynos_pd_dbg_fops;
	dbg_info->d = debugfs_create_file(dev_name(&pdev->dev), 0644,
			exynos_pd_dbg_root, dbg_info->dev, &dbg_info->fops);
	if (!dbg_info->d) {
		pr_err("%s %s: could not creatd debugfs file\n",
				EXYNOS_PD_DBG_PREFIX, __func__);
		ret = -ENOMEM;
		goto err_dbgfs_pd;
	}
#endif
	platform_set_drvdata(pdev, dbg_info);

	pm_runtime_enable(&pdev->dev);

	ret = pm_runtime_get_sync(&pdev->dev);
	if (ret) {
		pr_err("%s %s: get_sync of %s failed.\n",
			EXYNOS_PD_DBG_PREFIX, __func__, dev_name(&pdev->dev));
		goto err_get_sync;
	}

	ret = pm_runtime_put_sync(&pdev->dev);
	if (ret) {
		pr_err("%s %s: put sync of %s failed.\n",
			EXYNOS_PD_DBG_PREFIX, __func__, dev_name(&pdev->dev));
		goto err_put_sync;
	}

	return 0;

err_get_sync:
err_put_sync:
#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(dbg_info->d);
err_dbgfs_pd:
	debugfs_remove_recursive(exynos_pd_dbg_root);
err_dbgfs_root:
#endif
	kfree(dbg_info);
err_dbg_info:
	return ret;
}

static int exynos_pd_dbg_remove(struct platform_device *pdev)
{
	struct exynos_pd_dbg_info *dbg_info = platform_get_drvdata(pdev);
	struct device *dev = dbg_info->dev;

	if (pm_runtime_enabled(dev) && pm_runtime_active(dev))
		pm_runtime_put_sync(dev);

	pm_runtime_disable(dev);

#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(dbg_info->d);
	debugfs_remove_recursive(exynos_pd_dbg_root);
#endif
	kfree(dbg_info);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static int exynos_pd_dbg_runtime_suspend(struct device *dev)
{
	pr_info("%s %s's Runtime_Suspend\n",
			EXYNOS_PD_DBG_PREFIX, dev_name(dev));
	return 0;
}

static int exynos_pd_dbg_runtime_resume(struct device *dev)
{
	pr_info("%s %s's Runtime_Resume\n",
			EXYNOS_PD_DBG_PREFIX, dev_name(dev));
	return 0;
}

static struct dev_pm_ops exynos_pd_dbg_pm_ops = {
	SET_RUNTIME_PM_OPS(exynos_pd_dbg_runtime_suspend,
			exynos_pd_dbg_runtime_resume,
			NULL)
};

#ifdef CONFIG_OF
static const struct of_device_id exynos_pd_dbg_match[] = {
	{
		.compatible = "samsung,exynos-pd-dbg",
	},
	{},
};
#endif

static struct platform_driver exynos_pd_dbg_drv = {
	.probe		= exynos_pd_dbg_probe,
	.remove		= exynos_pd_dbg_remove,
	.driver		= {
		.name	= "exynos_pd_dbg",
		.owner	= THIS_MODULE,
		.pm	= &exynos_pd_dbg_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = exynos_pd_dbg_match,
#endif
	},
};

static int __init exynos_pd_dbg_init(void)
{
	return platform_driver_register(&exynos_pd_dbg_drv);
}
late_initcall(exynos_pd_dbg_init);
