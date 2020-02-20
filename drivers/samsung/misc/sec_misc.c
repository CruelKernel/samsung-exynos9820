/*
 * driver/misc/sec_misc.c
 *
 * driver supporting miscellaneous functions for Samsung P-series device
 *
 * COPYRIGHT(C) Samsung Electronics Co., Ltd. 2006-2011 All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/wakelock.h>
#include <linux/blkdev.h>
#include <mach/gpio.h>
#include <linux/sec_class.h>

static struct wake_lock sec_misc_wake_lock;
static const struct file_operations sec_misc_fops = {
	.owner = THIS_MODULE,
	/*	.read = sec_misc_read,
		.ioctl = sec_misc_ioctl,
		.open = sec_misc_open,
		.release = sec_misc_release, */
};

static struct miscdevice sec_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sec_misc",
	.fops = &sec_misc_fops,
};

/*
 * For Drop Caches
 */
#include <linux/fs.h>
#include <linux/vmstat.h>
#include <linux/swap.h>

#define K(x) ((x) << (PAGE_SHIFT - 10))

extern void drop_pagecache_sb(struct super_block *sb, void *unused);
extern void drop_slab(void);

static ssize_t drop_caches_show
	(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	return snprintf(buf, sizeof(buf), "%d\n", ret);
}

static ssize_t drop_caches_store
	(struct device *dev, struct device_attribute *attr,\
		const char *buf, size_t size)
{
	struct sysinfo i;

	if (strlen(buf) > 2)
		goto out;

	if (buf[0] == '3') {
		si_meminfo(&i);
		printk("[Before]\nMemFree : %8lu kB\n", K(i.freeram));

		iterate_supers(drop_pagecache_sb, NULL);
		drop_slab();

		si_meminfo(&i);
		printk("[After]\nMemFree : %8lu kB\n", K(i.freeram));
		printk("Cached Drop done!\n");
	}
out:
	return size;
}

static DEVICE_ATTR(drop_caches, S_IRUGO | S_IWUSR | S_IWGRP,\
			drop_caches_show, drop_caches_store);
/*
 * End Drop Caches
 */


struct device *sec_misc_dev;

static struct device_attribute *sec_misc_attrs[] = {
	&dev_attr_drop_caches,
};

static int __init sec_misc_init(void)
{
	int ret = 0;
	int i;

	ret = misc_register(&sec_misc_device);
	if (ret < 0) {
		printk(KERN_ERR "misc_register failed!\n");
		goto failed_register_misc;
	}
	sec_misc_dev = sec_device_create(NULL, "sec_misc");
	if (IS_ERR(sec_misc_dev)) {
		printk(KERN_ERR "failed to create device!\n");
		ret = -ENODEV;
		goto failed_create_device;
	}

	for (i = 0; i < ARRAY_SIZE(sec_misc_attrs) ; i++) {
		ret = device_create_file(sec_misc_dev, sec_misc_attrs[i]);
		if (ret < 0) {
			pr_err("failed to create sec misc device file \n");
			goto failed_create_device_file;
		}
	}

	wake_lock_init(&sec_misc_wake_lock, WAKE_LOCK_SUSPEND, "sec_misc");

	return 0;

failed_create_device_file:
	if (i) {
		for (--i; i >= 0; i--)
			device_remove_file(sec_misc_dev, sec_misc_attrs[i]);
	}
failed_create_device:
	misc_deregister(&sec_misc_device);
failed_register_misc:
	return ret;
}

static void __exit sec_misc_exit(void)
{
	wake_lock_destroy(&sec_misc_wake_lock);
}

module_init(sec_misc_init);
module_exit(sec_misc_exit);

/* Module information */
MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Samsung PX misc. driver");
MODULE_LICENSE("GPL");
