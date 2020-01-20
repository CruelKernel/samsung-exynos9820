/*
 *  Copyright (C) 2017, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include "fingerprint_sysfs.h"

struct class *fingerprint_class;
EXPORT_SYMBOL_GPL(fingerprint_class);

/*
 * Create sysfs interface
 */
void set_fingerprint_attr(struct device *dev,
	struct device_attribute *attributes[])
{
	int i;

	for (i = 0; attributes[i] != NULL; i++)
		if ((device_create_file(dev, attributes[i])) < 0)
			pr_err("%s: fail device_create_file! %d\n", __func__, i);
}

int fingerprint_register(struct device *dev, void *drvdata,
	struct device_attribute *attributes[], char *name)
{
	int ret = 0;

	if (!fingerprint_class) {
		fingerprint_class = class_create(THIS_MODULE, "fingerprint");
		if (IS_ERR(fingerprint_class))
			return PTR_ERR(fingerprint_class);
	}

	dev = device_create(fingerprint_class, NULL, 0, drvdata, "%s", name);

	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("%s: device_create failed! %d\n", __func__, ret);
		return ret;
	}

	set_fingerprint_attr(dev, attributes);

	return 0;
}

void fingerprint_unregister(struct device *dev,
	struct device_attribute *attributes[])
{
	int i;

	for (i = 0; attributes[i] != NULL; i++)
		device_remove_file(dev, attributes[i]);
}

void destroy_fingerprint_class(void)
{
	if (fingerprint_class) {
		class_destroy(fingerprint_class);
		fingerprint_class = NULL;
	}
}

static int __init fingerprint_class_init(void)
{
	pr_info("%s\n", __func__);
	fingerprint_class = class_create(THIS_MODULE, "fingerprint");

	if (IS_ERR(fingerprint_class)) {
		pr_err("%s, create fingerprint_class is failed.(err=%d)\n",
			__func__, IS_ERR(fingerprint_class));
		return PTR_ERR(fingerprint_class);
	}

	fingerprint_class->dev_uevent = NULL;

	return 0;
}

static void __exit fingerprint_class_exit(void)
{
	if (fingerprint_class) {
		class_destroy(fingerprint_class);
		fingerprint_class = NULL;
	}
}

subsys_initcall(fingerprint_class_init);
module_exit(fingerprint_class_exit);

MODULE_DESCRIPTION("fingerprint sysfs class");
MODULE_LICENSE("GPL");
