/*
 * usbpd-core.c  --  usbpd control IC framework.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/suspend.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/ccic/core.h>
#include <linux/module.h>

static struct class usbpd_class;

static struct dentry *debugfs_root;

#if defined(CONFIG_IFCONN_NOTIFIER)
static int usbpd_handle_ifconn_notification(struct notifier_block *nb,
				unsigned long action, void *data)
{
	struct ifconn_notifier_template *template = data;
	struct usbpd_dev *udev =
			container_of(nb, struct usbpd_dev, nb);
	struct pdic_notifier_data *pd_noti = &udev->pd_noti;
	ifconn_notifier_id_t notifier_id = template->id;
	int select_pdo_num = 0, ret = 0;

	pr_info("%s, event : %d\n", __func__, template->event);
	if (notifier_id == IFCONN_NOTIFY_ID_SELECT_PDO) {
		select_pdo_num = template->event;
		if (pd_noti->sink_status.selected_pdo_num == select_pdo_num || select_pdo_num == 0)
			return 0;
		else if (select_pdo_num > pd_noti->sink_status.available_pdo_num)
			pd_noti->sink_status.selected_pdo_num = pd_noti->sink_status.available_pdo_num;
		else if (select_pdo_num < 1)
			pd_noti->sink_status.selected_pdo_num = 1;
		else
			pd_noti->sink_status.selected_pdo_num = select_pdo_num;

		ret = udev->desc->ops->usbpd_select_pdo(udev, pd_noti->sink_status.selected_pdo_num);
	}

	return ret;
}
#endif

static void usbpd_init_pd_info(struct usbpd_desc *usbpd_desc)
{
	struct usbpd_info *pd_info = &usbpd_desc->pd_info;

	pd_info->is_host = 0;
	pd_info->is_client = 0;
	pd_info->is_pr_swap = 0;
	pd_info->power_role = 0;
	pd_info->data_role = 0;
	pd_info->request_power_number = 0;
	pd_info->pd_charger_attached = false;
	pd_info->plug_attached = false;
	pd_info->dp_is_connect = false;
	pd_info->dp_hs_connect = false;
	pd_info->is_sent_pin_configuration = false;
	pd_info->is_src = false;
	pd_info->is_samsung_accessory_enter_mode = false;
}

static struct usbpd_init_data *usbpd_get_init_data(const char *name)
{
	struct device_node *np;
	struct usbpd_init_data *init_data = NULL;
	int ret = 0;

	np = of_find_node_by_name(NULL, "usbpd-manager");
	if (np == NULL) {
		pr_err("%s np NULL\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	init_data = kzalloc(sizeof(struct usbpd_init_data), GFP_KERNEL);
	if (!init_data)
		return ERR_PTR(-ENOMEM); /* Out of memory? */

	ret = of_property_read_u32(np, "pdic,max_voltage",
			&init_data->max_charging_volt);
	if (ret < 0)
		pr_err("%s error reading max_charging_volt %d\n",
				__func__, init_data->max_charging_volt);

	ret = of_property_read_u32(np, "pdic,max_power",
			&init_data->max_power);
	if (ret < 0)
		pr_err("%s error reading max_power %d\n",
				__func__, init_data->max_power);

	ret = of_property_read_u32(np, "pdic,op_power",
			&init_data->op_power);
	if (ret < 0)
		pr_err("%s error reading op_power %d\n",
				__func__, init_data->op_power);

	ret = of_property_read_u32(np, "pdic,max_current",
			&init_data->max_current);
	if (ret < 0)
		pr_err("%s error reading max_current %d\n",
				__func__, init_data->max_current);

	ret = of_property_read_u32(np, "pdic,min_current",
			&init_data->min_current);
	if (ret < 0)
		pr_err("%s error reading min_current %d\n",
				__func__, init_data->min_current);

	init_data->giveback = of_property_read_bool(np,
					     "pdic,giveback");
	init_data->usb_com_capable = of_property_read_bool(np,
					     "pdic,usb_com_capable");
	init_data->no_usb_suspend = of_property_read_bool(np,
					     "pdic,no_usb_suspend");

	/* source capability */
	ret = of_property_read_u32(np, "source,max_voltage",
			&init_data->source_max_volt);
	ret = of_property_read_u32(np, "source,min_voltage",
			&init_data->source_min_volt);
	ret = of_property_read_u32(np, "source,max_power",
			&init_data->source_max_power);

	/* sink capability */
	ret = of_property_read_u32(np, "sink,capable_max_voltage",
			&init_data->sink_cap_max_volt);
	if (ret < 0) {
		init_data->sink_cap_max_volt = 5000;
		pr_err("%s error reading sink_cap_max_volt %d\n",
				__func__, init_data->sink_cap_max_volt);
	}

	return init_data;
}

/**
 * usbpd_register - register usbpd
 * @usbpd_desc: usbpd to register
 * @cfg: runtime configuration for usbpd
 *
 * Called by usbpd drivers to register a usbpd.
 * Returns a valid pointer to struct usbpd_dev on success
 * or an ERR_PTR() on error.
 */
struct usbpd_dev *
usbpd_register(struct device *dev, struct usbpd_desc *usbpd_desc, void *_data)
{
	struct usbpd_init_data *init_data;
	struct usbpd_dev *udev;
	int ret = 0;

	if (usbpd_desc->name == NULL || usbpd_desc->ops == NULL)
		return ERR_PTR(-EINVAL);

	udev = kzalloc(sizeof(struct usbpd_dev), GFP_KERNEL);
	if (udev == NULL)
		return ERR_PTR(-ENOMEM);

#if defined(CONFIG_IFCONN_NOTIFIER)
	ret = ifconn_notifier_register(&udev->nb,
				usbpd_handle_ifconn_notification,
				IFCONN_NOTIFY_CCIC,
				IFCONN_NOTIFY_MANAGER);
	if (ret < 0) {
		pr_err("%s notifier register failed.\n", __func__);
	}
#endif

	init_data = usbpd_get_init_data(usbpd_desc->name);
	if (IS_ERR(init_data)) {
		pr_err("%s, init_data init error!\n", __func__);
		init_data = NULL;
	}
	/* Todo abnormal state
	if (!init_data || ret < 0) {
		init_data = config->init_data;
		udev->dev.of_node = of_node_get(config->of_node);
	}
	*/
	mutex_init(&udev->mutex);
	udev->desc = usbpd_desc;
	udev->drv_data = _data;

	udev->dev.parent = dev;
	dev_set_name(&udev->dev, usbpd_desc->name);

	usbpd_init_pd_info(udev->desc);


	/* set usbpd constraints */
	if (init_data)
		udev->init_data = init_data;

	/* Todo for sysfs */
	ret = device_register(&udev->dev);

	if (ret != 0)
		goto clean;

	mdelay(10);

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	dual_role_init(udev);
#endif

	return udev;

clean:
	kfree(udev);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(usbpd_register);

/**
 * usbpd_unregister - unregister usbpd
 * @udev: usbpd to unregister
 *
 * Called by usbpd drivers to unregister a usbpd.
 */
void usbpd_unregister(struct usbpd_dev *udev)
{
	if (udev == NULL)
		return;

	device_unregister(&udev->dev);
}
EXPORT_SYMBOL_GPL(usbpd_unregister);

/**
 * udev_get_drvdata - get udev usbpd driver data
 * @udev: usbpd
 *
 * Get udev usbpd driver private data. This call can be used in the
 * usbpd driver context.
 */
void *udev_get_drvdata(struct usbpd_dev *udev)
{
	return udev->drv_data;
}
EXPORT_SYMBOL_GPL(udev_get_drvdata);

static int __init usbpd_init(void)
{
	int ret;

	ret = class_register(&usbpd_class);

	debugfs_root = debugfs_create_dir("usbpd", NULL);
	if (!debugfs_root)
		pr_warn("usbpd: Failed to create debugfs directory\n");

	return ret;
}

/* init early to allow our consumers to complete system booting */
core_initcall(usbpd_init);
