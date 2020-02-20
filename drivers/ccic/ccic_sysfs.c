/*
 * driver/../ccic_core.c - S2MM005 USBPD device driver
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

#include <linux/types.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ccic/ccic_sysfs.h>
#include <linux/ccic/ccic_core.h>

static ssize_t ccic_sysfs_show_property(struct device *dev,
				       struct device_attribute *attr, char *buf);

static ssize_t ccic_sysfs_store_property(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count);

#define CCIC_SYSFS_ATTR(_name)				\
{							\
	.attr = { .name = #_name },			\
	.show = ccic_sysfs_show_property,		\
	.store = ccic_sysfs_store_property,		\
}

static struct device_attribute ccic_attributes[] = {
	CCIC_SYSFS_ATTR(chip_name),
	CCIC_SYSFS_ATTR(cur_version),
	CCIC_SYSFS_ATTR(src_version),
	CCIC_SYSFS_ATTR(lpm_mode),
	CCIC_SYSFS_ATTR(state),
	CCIC_SYSFS_ATTR(rid),
	CCIC_SYSFS_ATTR(ccic_control_option),
	CCIC_SYSFS_ATTR(booting_dry),
	CCIC_SYSFS_ATTR(fw_update),
	CCIC_SYSFS_ATTR(fw_update_status),
	CCIC_SYSFS_ATTR(water),
	CCIC_SYSFS_ATTR(dex_fan_uvdm),
	CCIC_SYSFS_ATTR(acc_device_version),
	CCIC_SYSFS_ATTR(debug_opcode),
	CCIC_SYSFS_ATTR(control_gpio),
	CCIC_SYSFS_ATTR(usbpd_ids),
	CCIC_SYSFS_ATTR(usbpd_type),
	CCIC_SYSFS_ATTR(cc_pin_status),
	CCIC_SYSFS_ATTR(ram_test),
	CCIC_SYSFS_ATTR(sbu_adc),
	CCIC_SYSFS_ATTR(vsafe0v_status),
};

static ssize_t ccic_sysfs_show_property(struct device *dev,
				       struct device_attribute *attr, char *buf)
{

	ssize_t ret = 0;
	pccic_data_t pccic_data = dev_get_drvdata(dev);
	pccic_sysfs_property_t pccic_sysfs = (pccic_sysfs_property_t)pccic_data->ccic_syfs_prop;
	const ptrdiff_t off = attr - ccic_attributes;

	if (off == CCIC_SYSFS_PROP_CHIP_NAME) {
		return snprintf(buf, PAGE_SIZE, "%s\n",
					pccic_data->name);
	} else {
		ret = pccic_sysfs->get_property(pccic_data, off, buf);
		if (ret < 0) {
			if (ret == -ENODATA)
				dev_info(dev,
					"driver has no data for `%s' property\n",
					attr->attr.name);
			else if (ret != -ENODEV)
				dev_err(dev,
					"driver failed to report `%s' property: %zd\n",
					attr->attr.name, ret);
			return ret;
		}
		return ret;
	}
}

static ssize_t ccic_sysfs_store_property(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	ssize_t ret = 0;
	pccic_data_t pccic_data = dev_get_drvdata(dev);
	pccic_sysfs_property_t pccic_sysfs = (pccic_sysfs_property_t)pccic_data->ccic_syfs_prop;
	const ptrdiff_t off = attr - ccic_attributes;

	ret = pccic_sysfs->set_property(pccic_data, off, buf, count);
	if (ret < 0) {
			if (ret == -ENODATA)
				dev_info(dev,
					"driver cannot set  data for `%s' property\n",
					attr->attr.name);
			else if (ret != -ENODEV)
				dev_err(dev,
					"driver failed to set `%s' property: %zd\n",
					attr->attr.name, ret);
			return ret;
		}
		return ret;
}

static umode_t ccic_sysfs_attr_is_visible(struct kobject *kobj,
					 struct attribute *attr, int attrno)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	pccic_data_t pccic_data = dev_get_drvdata(dev);
	pccic_sysfs_property_t pccic_sysfs =
			(pccic_sysfs_property_t)pccic_data->ccic_syfs_prop;
	umode_t mode = 0444;
	int i;

	for (i = 0; i < pccic_sysfs->num_properties; i++) {
		int property = pccic_sysfs->properties[i];

		if (property == attrno) {
			if (pccic_sysfs->property_is_writeable &&
					pccic_sysfs->property_is_writeable(
					pccic_data, property) > 0)
				mode |= 0200;
			if (pccic_sysfs->property_is_writeonly &&
			    pccic_sysfs->property_is_writeonly(pccic_data, property)
			    > 0)
				mode = 0200;
			return mode;
		}
	}

	return 0;
}

static struct attribute *__ccic_sysfs_attrs[ARRAY_SIZE(ccic_attributes) + 1];

const struct attribute_group ccic_sysfs_group = {
	.attrs = __ccic_sysfs_attrs,
	.is_visible = ccic_sysfs_attr_is_visible,
};

void ccic_sysfs_init_attrs(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ccic_attributes); i++)
		__ccic_sysfs_attrs[i] = &ccic_attributes[i].attr;
}


