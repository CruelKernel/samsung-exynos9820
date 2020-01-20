/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sec_hqm_device.h>


static struct device *dev;


#define TAG	"[HQM] "


void send_uevent_via_hqm_device(hqm_device_info hqm_info)
{
	char *event[2];
	int i;
	int size;

	if ((hqm_info.feature[0] < 'A') || (hqm_info.feature[0] > 'Z'))
		return;

	event[0] = kasprintf(GFP_KERNEL,
			"FEATURE=%s",hqm_info.feature);

	event[1] = NULL;

	kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, event);
	printk(TAG "send uevent :  %s\n" ,event[0]);

	size = ARRAY_SIZE(event);
	for (i = 0; i < size - 1; i++)
		kfree(event[i]);

}


static const struct file_operations hqm_fops = {
	.owner          =       THIS_MODULE,
};

static int __init sec_hqm_device_init(void)
{
	int rc = 0;
	int device_major;
	struct class *device_class;

	printk(TAG "%s\n", __func__);

	device_major = register_chrdev(0, "hqm_event", &hqm_fops);
	if (rc) {
		pr_err(TAG "Cant' register chrdev for hqm.\n");
		goto failed;
	}
		
	device_class = class_create(THIS_MODULE, "hqm");
	if (IS_ERR(device_class)) {
		rc = PTR_ERR(device_class);
		pr_err(TAG "Can't create device class (%d)\n", rc);
		goto failed;
	}

	dev = device_create(device_class, NULL, MKDEV(device_major, 0),
				NULL, "hqm_event");

	if (IS_ERR(dev)) {
		rc = PTR_ERR(dev);
		pr_err(TAG "Can't create hqm device (%d)\n", rc);
		goto failed;
	}

failed :
	return rc;
}
arch_initcall(sec_hqm_device_init);

