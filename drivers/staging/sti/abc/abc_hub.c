/* abc_hub.c
 *
 * Abnormal Behavior Catcher Hub Driver
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * Sangsu Ha <sangsu.ha@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/sti/abc_hub.h>

static struct device *abc_hub_dev;
static int abc_hub_probed;

#ifdef CONFIG_OF
static int abc_hub_parse_dt(struct device *dev)
{
	struct abc_hub_platform_data *pdata = dev->platform_data;
	struct device_node *np;

	np = dev->of_node;
	pdata->nSub = of_get_child_count(np);
	if (!pdata->nSub) {
		dev_err(dev, "There is no dt of sub module\n");
		return -ENODEV;
	}

#ifdef CONFIG_SEC_ABC_HUB_COND
	if (parse_cond_data(dev, pdata, np) < 0)
		dev_info(dev, "sub module(cond) is not supported\n");
	else
		pdata->cond_pdata.init = 1;
#endif
#ifdef CONFIG_SEC_ABC_HUB_BOOTC
	if (parse_bootc_data(dev, pdata, np) < 0)
		dev_info(dev, "sub module(bootc) is not supported\n");
	else
		pdata->bootc_pdata.init = 1;
#endif
	return 0;
}
#endif

#ifdef CONFIG_OF
static const struct of_device_id abc_hub_dt_match[] = {
	{ .compatible = "samsung,abc_hub" },
	{ }
};
#endif

static int abc_hub_suspend(struct device *dev)
{
	int ret = 0;
#ifdef CONFIG_SEC_ABC_HUB_COND
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);

	if (pinfo->pdata->cond_pdata.init)
		ret = abc_hub_cond_suspend(dev);
#endif
	return ret;
}

static int abc_hub_resume(struct device *dev)
{
	int ret = 0;
#ifdef CONFIG_SEC_ABC_HUB_COND
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);

	if (pinfo->pdata->cond_pdata.init)
		ret = abc_hub_cond_resume(dev);
#endif
	return ret;
}

static int abc_hub_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct dev_pm_ops abc_hub_pm = {
	.suspend = abc_hub_suspend,
	.resume = abc_hub_resume,
};

static ssize_t store_abc_hub_enable(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);

	/* The enabel will be managed for each sub module when sub module becomes over two. */
	if (!strncmp(buf, "1", 1)) {
		dev_info(dev, "abc_hub driver enabled.\n");

		if (sec_abc_wait_enabled() < 0)
			dev_err(dev, "abc driver is not enabled\n");

		pinfo->enabled = ABC_HUB_ENABLED;
#ifdef CONFIG_SEC_ABC_HUB_COND
		if (pinfo->pdata->cond_pdata.init)
			abc_hub_cond_enable(dev, pinfo->enabled);
#endif
#ifdef CONFIG_SEC_ABC_HUB_BOOTC
		if (pinfo->pdata->bootc_pdata.init)
			abc_hub_bootc_enable(dev, pinfo->enabled);
#endif
	} else if (!strncmp(buf, "0", 1)) {
		dev_info(dev, "abc_hub driver disabled.\n");
		pinfo->enabled = ABC_HUB_DISABLED;
#ifdef CONFIG_SEC_ABC_HUB_COND
		if (pinfo->pdata->cond_pdata.init)
			abc_hub_cond_enable(dev, pinfo->enabled);
#endif
#ifdef CONFIG_SEC_ABC_HUB_BOOTC
		if (pinfo->pdata->bootc_pdata.init)
			abc_hub_bootc_enable(dev, pinfo->enabled);
#endif
	}
	return count;
}

static ssize_t show_abc_hub_enable(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", pinfo->enabled);
}
static DEVICE_ATTR(enable, 0644, show_abc_hub_enable, store_abc_hub_enable);

int abc_hub_get_enabled(void)
{
	struct abc_hub_info *pinfo;

	if (!abc_hub_probed)
		return 0;

	pinfo = dev_get_drvdata(abc_hub_dev);

	return pinfo->enabled;
}
EXPORT_SYMBOL(abc_hub_get_enabled);

/* event string format
 *
 * ex) MODULE=tsp@ERROR=power_status_mismatch
 *     MODULE=tsp@ERROR=power_status_mismatch@EXT_LOG=fw_ver(0108)
 *
 */
void abc_hub_send_event(char *str)
{
	struct abc_hub_info *pinfo;

	if (!abc_hub_probed) {
		dev_err(abc_hub_dev, "ABC Hub driver is not initialized!\n");
		return;
	}

	pinfo = dev_get_drvdata(abc_hub_dev);

	if (pinfo->enabled == ABC_HUB_DISABLED) {
		dev_info(abc_hub_dev, "ABC Hub is disabled!\n");
		return;
	}

	/* It just sends event to abc driver. The function will be added for gathering hw param big data. */
	sec_abc_send_event(str);
}
EXPORT_SYMBOL(abc_hub_send_event);

static int abc_hub_probe(struct platform_device *pdev)
{
	struct abc_hub_platform_data *pdata;
	struct abc_hub_info *pinfo;
	int ret = 0;

	dev_info(&pdev->dev, "%s\n", __func__);

	abc_hub_probed = false;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				     sizeof(struct abc_hub_platform_data), GFP_KERNEL);

		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate platform data\n");
			return -ENOMEM;
		}

		pdev->dev.platform_data = pdata;
		ret = abc_hub_parse_dt(&pdev->dev);
		if (ret) {
			dev_err(&pdev->dev, "Failed to parse dt data\n");
			return ret;
		}

		pr_info("%s: parse dt done\n", __func__);
	} else {
		pdata = pdev->dev.platform_data;
	}

	if (!pdata) {
		dev_err(&pdev->dev, "There are no platform data\n");
		return -EINVAL;
	}

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);

	if (!pinfo)
		return -ENOMEM;

	pinfo->dev = sec_device_create(pinfo, "sec_abc_hub");
	if (IS_ERR(pinfo->dev)) {
		pr_err("%s Failed to create device(sec_abc_hub)!\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	ret = device_create_file(pinfo->dev, &dev_attr_enable);
	if (ret) {
		pr_err("%s: Failed to create device enabled file\n", __func__);
		ret = -ENODEV;
		goto err_create_abc_hub_enable_sysfs;
	}

	abc_hub_dev = pinfo->dev;
	pinfo->pdata = pdata;

	platform_set_drvdata(pdev, pinfo);

#ifdef CONFIG_SEC_ABC_HUB_COND
	if (pdata->cond_pdata.init) {
		if (abc_hub_cond_init(abc_hub_dev) < 0) {
			dev_err(&pdev->dev, "abc_hub_cond_init fail\n");
			pdata->cond_pdata.init = 0;
		}
	}
#endif

#ifdef CONFIG_SEC_ABC_HUB_BOOTC
	if (pdata->bootc_pdata.init) {
		if (abc_hub_bootc_init(abc_hub_dev) < 0) {
			dev_err(&pdev->dev, "abc_hub_booc_init fail\n");
			pdata->bootc_pdata.init = 0;
		}
	}
#endif

	abc_hub_probed = true;
	dev_info(&pdev->dev, "%s success!\n", __func__);

	return ret;

err_create_abc_hub_enable_sysfs:
	sec_device_destroy(abc_hub_dev->devt);
out:
	kfree(pinfo);
	devm_kfree(&pdev->dev, pdata);

	return ret;
}

static struct platform_driver abc_hub_driver = {
	.probe = abc_hub_probe,
	.remove = abc_hub_remove,
	.driver = {
		.name = "abc_hub",
		.owner = THIS_MODULE,
#if defined(CONFIG_PM)
		.pm	= &abc_hub_pm,
#endif
#if CONFIG_OF
		.of_match_table = of_match_ptr(abc_hub_dt_match),
#endif
	},
};

static int __init abc_hub_init(void)
{
	pr_info("%s\n", __func__);

	return platform_driver_register(&abc_hub_driver);
}

static void __exit abc_hub_exit(void)
{
	return platform_driver_unregister(&abc_hub_driver);
}

module_init(abc_hub_init);
module_exit(abc_hub_exit);

MODULE_DESCRIPTION("Samsung ABC Hub Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
