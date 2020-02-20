/**
 * dwc3-exynos.c - Samsung EXYNOS DWC3 Specific Glue layer
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Anton Tikhomirov <av.tikhomirov@samsung.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/usb/otg.h>
#include <linux/usb/usb_phy_generic.h>
#include <linux/usb/samsung_usb.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>


#include <linux/io.h>
#include <linux/usb/otg-fsm.h>

#include <soc/samsung/exynos-cpupm.h>

/* -------------------------------------------------------------------------- */
struct dwc3_exynos_rsw {
	struct otg_fsm		*fsm;
	struct work_struct	work;
};



struct dwc3_exynos {
	struct platform_device	*usb2_phy;
	struct platform_device	*usb3_phy;
	struct device		*dev;

	struct clk		**clocks;

	struct regulator	*vdd33;
	struct regulator	*vdd10;

	int			idle_ip_index;

	struct dwc3_exynos_rsw	rsw;
};

void dwc3_otg_run_sm(struct otg_fsm *fsm);
int dwc3_otg_phy_enable(struct otg_fsm *fsm, int owner, bool on);

static const struct of_device_id exynos_dwc3_match[] = {
	{
		.compatible = "samsung,exynos-dwusb",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_dwc3_match);

/* -------------------------------------------------------------------------- */

static int dwc3_exynos_clk_get(struct dwc3_exynos *exynos)
{
	struct device *dev = exynos->dev;
	const char **clk_ids;
	struct clk *clk;
	int clk_count;
	int ret, i;

	clk_count = of_property_count_strings(dev->of_node, "clock-names");
	if (IS_ERR_VALUE((unsigned long)clk_count)) {
		dev_err(dev, "invalid clk list in %s node", dev->of_node->name);
		return -EINVAL;
	}

	clk_ids = (const char **)devm_kmalloc(dev,
				(clk_count + 1) * sizeof(const char *),
				GFP_KERNEL);
	if (!clk_ids) {
		dev_err(dev, "failed to alloc for clock ids");
		return -ENOMEM;
	}

	for (i = 0; i < clk_count; i++) {
		ret = of_property_read_string_index(dev->of_node, "clock-names",
								i, &clk_ids[i]);
		if (ret) {
			dev_err(dev, "failed to read clocks name %d from %s node\n",
					i, dev->of_node->name);
			return ret;
		}
	}
	clk_ids[clk_count] = NULL;

	exynos->clocks = (struct clk **) devm_kmalloc(exynos->dev,
			clk_count * sizeof(struct clk *), GFP_KERNEL);
	if (!exynos->clocks) {
		dev_err(exynos->dev, "%s: couldn't alloc\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; clk_ids[i] != NULL; i++) {
		clk = devm_clk_get(exynos->dev, clk_ids[i]);
		if (IS_ERR_OR_NULL(clk))
			goto err;

		exynos->clocks[i] = clk;
	}
	exynos->clocks[i] = NULL;

	return 0;

err:
	dev_err(exynos->dev, "couldn't get %s clock\n", clk_ids[i]);
	return -EINVAL;
}

static int dwc3_exynos_clk_prepare(struct dwc3_exynos *exynos)
{
	int i;
	int ret;

	for (i = 0; exynos->clocks[i] != NULL; i++) {
		ret = clk_prepare(exynos->clocks[i]);
		if (ret)
			goto err;
	}

	return 0;

err:
	dev_err(exynos->dev, "couldn't prepare clock[%d]\n", i);

	/* roll back */
	for (i = i - 1; i >= 0; i--)
		clk_unprepare(exynos->clocks[i]);

	return ret;
}

static int dwc3_exynos_clk_enable(struct dwc3_exynos *exynos)
{
	int i;
	int ret;

	for (i = 0; exynos->clocks[i] != NULL; i++) {
		ret = clk_enable(exynos->clocks[i]);
		if (ret)
			goto err;
	}

	return 0;

err:
	dev_err(exynos->dev, "couldn't enable clock[%d]\n", i);

	/* roll back */
	for (i = i - 1; i >= 0; i--)
		clk_disable(exynos->clocks[i]);

	return ret;
}

static void dwc3_exynos_clk_unprepare(struct dwc3_exynos *exynos)
{
	int i;

	for (i = 0; exynos->clocks[i] != NULL; i++)
		clk_unprepare(exynos->clocks[i]);
}

static void dwc3_exynos_clk_disable(struct dwc3_exynos *exynos)
{
	int i;

	for (i = 0; exynos->clocks[i] != NULL; i++)
		clk_disable(exynos->clocks[i]);
}

/* -------------------------------------------------------------------------- */

static struct dwc3_exynos *dwc3_exynos_match(struct device *dev)
{
	const struct of_device_id *matches = NULL;
	struct dwc3_exynos *exynos = NULL;

	if (!dev)
		return NULL;

	matches = exynos_dwc3_match;

	if (of_match_device(matches, dev))
		exynos = dev_get_drvdata(dev);

	return exynos;
}

bool dwc3_exynos_rsw_available(struct device *dev)
{
	struct dwc3_exynos *exynos;

	exynos = dwc3_exynos_match(dev);
	if (!exynos)
		return false;

	return true;
}

int dwc3_exynos_rsw_start(struct device *dev)
{
	struct dwc3_exynos	*exynos = dev_get_drvdata(dev);
	struct dwc3_exynos_rsw	*rsw = &exynos->rsw;

	dev_info(dev, "%s\n", __func__);

	/* B-device by default */
	rsw->fsm->id = 1;
	rsw->fsm->b_sess_vld = 0;

	return 0;
}

void dwc3_exynos_rsw_stop(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
}

static void dwc3_exynos_rsw_work(struct work_struct *w)
{
	struct dwc3_exynos_rsw	*rsw = container_of(w,
					struct dwc3_exynos_rsw, work);
	struct dwc3_exynos	*exynos = container_of(rsw,
					struct dwc3_exynos, rsw);

	dev_info(exynos->dev, "%s\n", __func__);

	dwc3_otg_run_sm(rsw->fsm);
}

int dwc3_exynos_rsw_setup(struct device *dev, struct otg_fsm *fsm)
{
	struct dwc3_exynos	*exynos = dev_get_drvdata(dev);
	struct dwc3_exynos_rsw	*rsw = &exynos->rsw;

	dev_dbg(dev, "%s\n", __func__);

	INIT_WORK(&rsw->work, dwc3_exynos_rsw_work);

	rsw->fsm = fsm;

	return 0;
}

void dwc3_exynos_rsw_exit(struct device *dev)
{
	struct dwc3_exynos	*exynos = dev_get_drvdata(dev);
	struct dwc3_exynos_rsw	*rsw = &exynos->rsw;

	dev_dbg(dev, "%s\n", __func__);

	cancel_work_sync(&rsw->work);

	rsw->fsm = NULL;
}

/**
 * dwc3_exynos_id_event - receive ID pin state change event.
 * @state : New ID pin state.
 */
int dwc3_exynos_id_event(struct device *dev, int state)
{
	struct dwc3_exynos	*exynos;
	struct dwc3_exynos_rsw	*rsw;
	struct otg_fsm		*fsm;

	dev_dbg(dev, "EVENT: ID: %d\n", state);

	exynos = dev_get_drvdata(dev);
	if (!exynos)
		return -ENOENT;

	rsw = &exynos->rsw;

	fsm = rsw->fsm;
	if (!fsm)
		return -ENOENT;

	if (fsm->id != state) {
		fsm->id = state;
		schedule_work(&rsw->work);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(dwc3_exynos_id_event);

/**
 * dwc3_exynos_vbus_event - receive VBus change event.
 * vbus_active : New VBus state, true if active, false otherwise.
 */
int dwc3_exynos_vbus_event(struct device *dev, bool vbus_active)
{
	struct dwc3_exynos	*exynos;
	struct dwc3_exynos_rsw	*rsw;
	struct otg_fsm		*fsm;

	dev_dbg(dev, "EVENT: VBUS: %sactive\n", vbus_active ? "" : "in");

	exynos = dev_get_drvdata(dev);
	if (!exynos)
		return -ENOENT;

	rsw = &exynos->rsw;

	fsm = rsw->fsm;
	if (!fsm)
		return -ENOENT;

	if (fsm->b_sess_vld != vbus_active) {
		fsm->b_sess_vld = vbus_active;
		schedule_work(&rsw->work);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(dwc3_exynos_vbus_event);

/**
 * dwc3_exynos_start_ldo - received ldo control event.
 */
int dwc3_exynos_start_ldo(struct device *dev, bool on)
{
#if 0 // temp
	struct dwc3_exynos	*exynos;
	struct dwc3_exynos_rsw	*rsw;
	struct otg_fsm		*fsm;

	dev_dbg(dev, "%s, %s\n", __func__, on ? "on" : "off");

	exynos = dev_get_drvdata(dev);
	if (!exynos)
		return -ENOENT;

	rsw = &exynos->rsw;

	fsm = rsw->fsm;
	if (!fsm)
		return -ENOENT;

	dwc3_otg_ldo_control(fsm, on);
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(dwc3_exynos_start_ldo);

/**
 * dwc3_exynos_phy_enable - received combo phy control.
 */
int dwc3_exynos_phy_enable(int owner, bool on)
{
	struct dwc3_exynos	*exynos;
	struct dwc3_exynos_rsw	*rsw;
	struct otg_fsm		*fsm;
	struct device_node *np = NULL;
	struct platform_device *pdev = NULL;
	int ret = 0;

	pr_info("%s owner=%d (usb:0 dp:1) on=%d +\n", __func__, owner, on);

	np = of_find_compatible_node(NULL, NULL, "samsung,exynos-dwusb");
	if (np) {
		pdev = of_find_device_by_node(np);
		if (!pdev) {
			pr_err("%s we can't get platform device\n", __func__);
			ret = -ENODEV;
			goto err;
		}
		of_node_put(np);
	} else {
		pr_err("%s we can't get np\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	exynos = platform_get_drvdata(pdev);
	if (!exynos) {
		pr_err("%s we can't get drvdata\n", __func__);
		ret = -ENOENT;
		goto err;
	}

	rsw = &exynos->rsw;

	fsm = rsw->fsm;
	if (!fsm) {
		pr_err("%s we can't get fsm\n", __func__);
		ret = -ENOENT;
		goto err;
	}

	dwc3_otg_phy_enable(fsm, owner, on);
err:
	pr_info("%s -\n", __func__);
	return ret;
}
EXPORT_SYMBOL_GPL(dwc3_exynos_phy_enable);

static int dwc3_exynos_register_phys(struct dwc3_exynos *exynos)
{
	struct usb_phy_generic_platform_data pdata;
	struct platform_device	*pdev;
	int			ret;

	memset(&pdata, 0x00, sizeof(pdata));

	pdev = platform_device_alloc("usb_phy_generic", PLATFORM_DEVID_AUTO);
	if (!pdev)
		return -ENOMEM;

	exynos->usb2_phy = pdev;
	pdata.type = USB_PHY_TYPE_USB2;
	pdata.gpio_reset = -1;

	ret = platform_device_add_data(exynos->usb2_phy, &pdata, sizeof(pdata));
	if (ret)
		goto err1;

	pdev = platform_device_alloc("usb_phy_generic", PLATFORM_DEVID_AUTO);
	if (!pdev) {
		ret = -ENOMEM;
		goto err1;
	}

	exynos->usb3_phy = pdev;
	pdata.type = USB_PHY_TYPE_USB3;

	ret = platform_device_add_data(exynos->usb3_phy, &pdata, sizeof(pdata));
	if (ret)
		goto err2;

	ret = platform_device_add(exynos->usb2_phy);
	if (ret)
		goto err2;

	ret = platform_device_add(exynos->usb3_phy);
	if (ret)
		goto err3;

	return 0;

err3:
	platform_device_del(exynos->usb2_phy);

err2:
	platform_device_put(exynos->usb3_phy);

err1:
	platform_device_put(exynos->usb2_phy);

	return ret;
}

static int dwc3_exynos_remove_child(struct device *dev, void *unused)
{
	struct platform_device *pdev = to_platform_device(dev);

	platform_device_unregister(pdev);

	return 0;
}

static int dwc3_exynos_probe(struct platform_device *pdev)
{
	struct dwc3_exynos	*exynos;
	struct device		*dev = &pdev->dev;
	struct device_node	*node = dev->of_node;
	int			ret;

	pr_info("%s: +++\n", __func__);
	exynos = devm_kzalloc(dev, sizeof(*exynos), GFP_KERNEL);
	if (!exynos)
		return -ENOMEM;

	ret = dma_set_mask(dev, DMA_BIT_MASK(36));
	if (ret) {
		pr_err("dma set mask ret = %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, exynos);

	exynos->dev = dev;

	exynos->idle_ip_index = exynos_get_idle_ip_index(dev_name(dev));
	pr_info("%s, usb idle ip = %d\n", __func__,
			exynos->idle_ip_index);
	exynos_update_ip_idle_status(exynos->idle_ip_index, 0);

	ret = dwc3_exynos_clk_get(exynos);
	if (ret)
		return ret;

	ret = dwc3_exynos_clk_prepare(exynos);
	if (ret)
		return ret;

	ret = dwc3_exynos_clk_enable(exynos);
	if (ret) {
		dwc3_exynos_clk_unprepare(exynos);
		return ret;
	}

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	exynos->vdd33 = devm_regulator_get(dev, "vdd33");
	if (IS_ERR(exynos->vdd33)) {
	        dev_dbg(dev, "couldn't get regulator vdd33\n");
		exynos->vdd33 = NULL;
	}
	if (exynos->vdd33) {
		ret = regulator_enable(exynos->vdd33);
		if (ret) {
			dev_err(dev, "Failed to enable VDD33 supply\n");
			goto vdd33_err;
		}
	}

	exynos->vdd10 = devm_regulator_get(dev, "vdd10");
	if (IS_ERR(exynos->vdd10)) {
		dev_dbg(dev, "couldn't get regulator vdd10\n");
		exynos->vdd10 = NULL;
	}
	if (exynos->vdd10) {
		ret = regulator_enable(exynos->vdd10);
		if (ret) {
			dev_err(dev, "Failed to enable VDD10 supply\n");
			goto vdd10_err;
		}
        }

	ret = dwc3_exynos_register_phys(exynos);
	if (ret) {
		dev_err(dev, "couldn't register PHYs\n");
		goto phys_err;
	}

	if (node) {
		ret = of_platform_populate(node, NULL, NULL, dev);
		if (ret) {
			dev_err(dev, "failed to add dwc3 core\n");
			goto populate_err;
		}
	} else {
		dev_err(dev, "no device node, failed to add dwc3 core\n");
		ret = -ENODEV;
		goto populate_err;
	}

	pr_info("%s: ---\n", __func__);
	return 0;

populate_err:
	platform_device_unregister(exynos->usb2_phy);
	platform_device_unregister(exynos->usb3_phy);
phys_err:
	if (exynos->vdd10)
		regulator_disable(exynos->vdd10);
vdd10_err:
	if (exynos->vdd33)
		regulator_disable(exynos->vdd33);
vdd33_err:
	pm_runtime_disable(&pdev->dev);
	dwc3_exynos_clk_disable(exynos);
	dwc3_exynos_clk_unprepare(exynos);
	pm_runtime_set_suspended(&pdev->dev);
	return ret;
}

static int dwc3_exynos_remove(struct platform_device *pdev)
{
	struct dwc3_exynos	*exynos = platform_get_drvdata(pdev);

	device_for_each_child(&pdev->dev, NULL, dwc3_exynos_remove_child);
	platform_device_unregister(exynos->usb2_phy);
	platform_device_unregister(exynos->usb3_phy);

	pm_runtime_disable(&pdev->dev);
	if (!pm_runtime_status_suspended(&pdev->dev)) {
		dwc3_exynos_clk_disable(exynos);
		pm_runtime_set_suspended(&pdev->dev);
	}

	dwc3_exynos_clk_unprepare(exynos);

	if (exynos->vdd33)
		regulator_disable(exynos->vdd33);
	if (exynos->vdd10)
		regulator_disable(exynos->vdd10);

	return 0;
}

#ifdef CONFIG_PM
static int dwc3_exynos_runtime_suspend(struct device *dev)
{
	struct dwc3_exynos *exynos = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);

	dwc3_exynos_clk_disable(exynos);

	/* inform what USB state is idle to IDLE_IP */
	exynos_update_ip_idle_status(exynos->idle_ip_index, 1);

	return 0;
}

static int dwc3_exynos_runtime_resume(struct device *dev)
{
	struct dwc3_exynos *exynos = dev_get_drvdata(dev);
	int ret = 0;

	dev_info(dev, "%s\n", __func__);

	/* inform what USB state is not idle to IDLE_IP */
	exynos_update_ip_idle_status(exynos->idle_ip_index, 0);

	ret = dwc3_exynos_clk_enable(exynos);
	if (ret) {
		dev_err(dev, "%s: clk_enable failed\n", __func__);
		return ret;
	}

	return 0;
}
#endif

#ifdef CONFIG_PM_SLEEP
static int dwc3_exynos_suspend(struct device *dev)
{
	struct dwc3_exynos *exynos = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	if (pm_runtime_suspended(dev))
		return 0;

	dwc3_exynos_clk_disable(exynos);

	if (exynos->vdd33)
		regulator_disable(exynos->vdd33);
	if (exynos->vdd10)
		regulator_disable(exynos->vdd10);

	/* inform what USB state is idle to IDLE_IP */
	//exynos_update_ip_idle_status(exynos->idle_ip_index, 1);

	return 0;
}

static int dwc3_exynos_resume(struct device *dev)
{
	struct dwc3_exynos *exynos = dev_get_drvdata(dev);
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	/* inform what USB state is not idle to IDLE_IP */
	//exynos_update_ip_idle_status(exynos->idle_ip_index, 0);

	if (exynos->vdd33) {
		ret = regulator_enable(exynos->vdd33);
		if (ret) {
			dev_err(dev, "Failed to enable VDD33 supply\n");
			return ret;
		}
	}
	if (exynos->vdd10) {
		ret = regulator_enable(exynos->vdd10);
		if (ret) {
			if (exynos->vdd33)
				regulator_disable(exynos->vdd33);
			dev_err(dev, "Failed to enable VDD10 supply\n");
			return ret;
		}
	}

	ret = dwc3_exynos_clk_enable(exynos);
	if (ret) {
		dev_err(dev, "%s: clk_enable failed\n", __func__);
		return ret;
	}

	/* runtime set active to reflect active state. */
	pm_runtime_disable(dev);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	return 0;
}

static const struct dev_pm_ops dwc3_exynos_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dwc3_exynos_suspend, dwc3_exynos_resume)
	SET_RUNTIME_PM_OPS(dwc3_exynos_runtime_suspend,
			dwc3_exynos_runtime_resume, NULL)
};

#define DEV_PM_OPS	(&dwc3_exynos_dev_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver dwc3_exynos_driver = {
	.probe		= dwc3_exynos_probe,
	.remove		= dwc3_exynos_remove,
	.driver		= {
		.name	= "exynos-dwc3",
		.of_match_table = exynos_dwc3_match,
		.pm	= DEV_PM_OPS,
	},
};

module_platform_driver(dwc3_exynos_driver);

MODULE_AUTHOR("Anton Tikhomirov <av.tikhomirov@samsung.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DesignWare USB3 EXYNOS Glue Layer");
