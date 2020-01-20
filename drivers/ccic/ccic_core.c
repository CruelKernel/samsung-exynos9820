/*
 * driver/../ccic_core.c - CCIC Core implementation
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * Author:Wookwang Lee. <wookwang.lee@samsung.com>,
 * Author:Guneet Singh Khurana  <gs.khurana@samsung.com>,
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/sec_sysfs.h>
#include <linux/ccic/ccic_core.h>
#include <linux/ccic/ccic_sysfs.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#ifndef CONFIG_SWITCH
#error "ERROR: CONFIG_SWITCH is not set."
#endif /* CONFIG_SWITCH */

#include <linux/switch.h>

struct device *ccic_device;
static struct switch_dev switch_dock = {
	.name = "ccic_dock",
};

int ccic_register_switch_device(int mode)
{
	int ret = 0;

	if (mode) {
		ret = switch_dev_register(&switch_dock);
		if (ret < 0) {
			pr_err("%s: Failed to register dock switch(%d)\n",
			       __func__, ret);
			return -ENODEV;
		}
	} else {
		switch_dev_unregister(&switch_dock);
	}
	return 0;
}

void ccic_send_dock_intent(int type)
{
	pr_info("%s: CCIC dock type(%d)", __func__, type);
	switch_set_state(&switch_dock, type);
}

void ccic_send_dock_uevent(u32 vid, u32 pid, int state)
{
	char switch_string[32];
	char pd_ids_string[32];
	char *envp[3] = { switch_string, pd_ids_string, NULL };

	pr_info("%s: CCIC dock : USBPD_IPS=%04x:%04x SWITCH_STATE=%d\n",
			__func__,
			le16_to_cpu(vid),
			le16_to_cpu(pid),
			state);

	if (IS_ERR(ccic_device)) {
		pr_err("%s CCIC ERROR: Failed to send a dock uevent!\n",
				__func__);
		return;
	}

	snprintf(switch_string, 32, "SWITCH_STATE=%d", state);
	snprintf(pd_ids_string, 32, "USBPD_IDS=%04x:%04x",
			le16_to_cpu(vid),
			le16_to_cpu(pid));
	kobject_uevent_env(&ccic_device->kobj, KOBJ_CHANGE, envp);
}

int ccic_core_register_chip(pccic_data_t pccic_data)
{
	int ret = 0;

	pr_info("%s\n", __func__);
	if (IS_ERR(ccic_device)) {
		pr_err("%s ccic_device is not present try again\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	dev_set_drvdata(ccic_device, pccic_data);

	/* create sysfs group */
	ret = sysfs_create_group(&ccic_device->kobj, &ccic_sysfs_group);
	if (ret)
		pr_err("%s: ccic sysfs fail, ret %d", __func__, ret);
out:
	return ret;
}

void ccic_core_unregister_chip(void)
{
	pr_info("%s\n", __func__);
	if (IS_ERR(ccic_device)) {
		pr_err("%s ccic_device is not present try again\n", __func__);
		return;
	}
	sysfs_remove_group(&ccic_device->kobj, &ccic_sysfs_group);
	dev_set_drvdata(ccic_device, NULL);
}

int ccic_core_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	ccic_device = sec_device_create(NULL, "ccic");
	if (IS_ERR(ccic_device)) {
		pr_err("%s Failed to create device(switch)!\n", __func__);
		ret = -ENODEV;
		goto out;
	}
	dev_set_drvdata(ccic_device, NULL);
	ccic_sysfs_init_attrs();
out:
	return ret;
}
void *ccic_core_get_drvdata(void)
{
	pccic_data_t pccic_data = NULL;

	pr_info("%s\n", __func__);
	if (!ccic_device) {
		pr_err("%s ccic_device is not present try again\n", __func__);
		return NULL;
	}
	pccic_data = dev_get_drvdata(ccic_device);
	if (!pccic_data) {
		pr_err("%s drv data is null in ccic device\n", __func__);
		return NULL;
	}
	return (pccic_data->drv_data);
}

