/*
 * Copyright (C) 2018 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "IONFD2PHYS: " fmt
/* #define DEBUG */

#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "tz_cdev.h"
#include "tzdev.h"

#define TZDEV_MAX_PFNS_COUNT	(SIZE_MAX / sizeof(sk_pfn_t))

struct ionfd2phys {
	int fd;
	size_t nr_pfns;
	sk_pfn_t *pfns;
};

#ifdef CONFIG_COMPAT
struct ionfd2phys32 {
	int fd;
	unsigned int nr_pfns;
	u32 pfns;
};
#endif

static long __ionfd2phys_ioctl(int fd, size_t nr_pfns, sk_pfn_t *pfns)
{
	int ret = 0;
	struct dma_buf *d_buf;
	void *addr, *vaddr;
	size_t pfn;

	d_buf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(d_buf)) {
		ret = PTR_ERR(d_buf);
		pr_err("Failed to get dma buf %d\n", ret);
		return ret;
	}

	vaddr = dma_buf_vmap(d_buf);
	if (IS_ERR_OR_NULL(vaddr)) {
		ret = PTR_ERR(vaddr);
		pr_err("Failed to vmap dma buf %d\n", ret);
		goto put_dma_buf;
	}

	addr = vaddr;
	/* There is no kernel public API for getting imported buffer
	*  size. So just use what user has supplied */
	for (pfn = 0; pfn < nr_pfns; ++pfn) {
		pfns[pfn] = vmalloc_to_pfn(addr);
		addr += PAGE_SIZE;
	}
	dma_buf_vunmap(d_buf, vaddr);

put_dma_buf:
	dma_buf_put(d_buf);

	return ret;
}

static long ionfd2phys_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int ret;
	struct ionfd2phys data;
	sk_pfn_t *pfns;

	if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
		pr_err("Failed ION FD: copy_from_user()\n");
		ret = -EFAULT;
		goto out;
	}

	if (data.nr_pfns > TZDEV_MAX_PFNS_COUNT) {
		ret = -EINVAL;
		goto out;
	}

	pfns = kzalloc(data.nr_pfns * sizeof(sk_pfn_t), GFP_KERNEL);
	if (!pfns) {
		pr_err("Failed ION FD: kzalloc(%zu)\n", data.nr_pfns);
		ret = -ENOMEM;
		goto out;
	}

	ret = __ionfd2phys_ioctl(data.fd, data.nr_pfns, pfns);
	if (ret) {
		pr_err("Failed ION FD: __ionfd2phys_ioctl()\n");
		goto pfns_free;
	}

	if (copy_to_user((void __user *)data.pfns, pfns,
				data.nr_pfns * sizeof(sk_pfn_t))) {
		pr_err("Failed ION FD: copy_to_user()\n");
		ret = -EFAULT;
		goto pfns_free;
	}

pfns_free:
	kfree(pfns);
out:
	return ret;
}

#ifdef CONFIG_COMPAT
static long compat_ionfd2phys_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int ret;
	struct ionfd2phys32 data;
	sk_pfn_t *pfns;

	if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
		pr_err("Failed ION FD: copy_from_user()\n");
		ret = -EFAULT;
		goto out;
	}

	if (data.nr_pfns > TZDEV_MAX_PFNS_COUNT) {
		ret = -EINVAL;
		goto out;
	}

	pfns = kzalloc(data.nr_pfns * sizeof(sk_pfn_t), GFP_KERNEL);
	if (!pfns) {
		pr_err("Failed ION FD: kzalloc(%d)\n", data.nr_pfns);
		ret = -ENOMEM;
		goto out;
	}

	ret = __ionfd2phys_ioctl(data.fd, (size_t)data.nr_pfns, pfns);
	if (ret) {
		pr_err("Failed ION FD: __ionfd2phys_ioctl()\n");
		goto pfns_free;
	}

	if (copy_to_user(compat_ptr(data.pfns), pfns,
				data.nr_pfns * sizeof(sk_pfn_t))) {
		pr_err("Failed ION FD: copy_to_user()\n");
		ret = -EFAULT;
		goto pfns_free;
	}

pfns_free:
	kfree(pfns);
out:
	return ret;
}
#endif

static struct file_operations ionfd2phys_fops = {
	.unlocked_ioctl = ionfd2phys_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_ionfd2phys_ioctl,
#endif
};

static struct tz_cdev ionfd2phys_cdev = {
	.name = "ionfd2phys",
	.fops = &ionfd2phys_fops,
	.owner = THIS_MODULE,
};

static ssize_t system_heap_id_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
#if defined(CONFIG_SOC_EXYNOS9820)
	/* This is dirty hack for ex9820 platform. This platform uses
	 * DTS/device tree to get information about preallocated ION heaps and
	 * in DTS/device tree system heap has ID 1, but kernel headers defines
	 * system heap ID as ION_HEAP_TYPE_SYSTEM and system heap ID
	 * is zero */
#define ION_HEAP_TYPE_SYSTEM_EX9820 (1)
	return sprintf(buf, "%d\n", ION_HEAP_TYPE_SYSTEM_EX9820);
#endif
	return -ENOSYS;
}

static DEVICE_ATTR(system_heap_id, S_IRUGO, system_heap_id_show, NULL);

static ssize_t ionfd2phys_id_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%d\n", 1);
}

static DEVICE_ATTR(ionfd2phys_id, S_IRUGO, ionfd2phys_id_show, NULL);

static int __init ionfd2phys_init(void)
{
	int ret;

	ret = tz_cdev_register(&ionfd2phys_cdev);
	if (ret) {
		pr_err("Unable to register ionfd2phys char device %d\n", ret);
		return ret;
	}

	ret = device_create_file(ionfd2phys_cdev.device, &dev_attr_system_heap_id);
	if (ret) {
		pr_err("system_heap_id sysfs file creation failed\n");
		goto out_unregister;
	}

	ret = device_create_file(ionfd2phys_cdev.device, &dev_attr_ionfd2phys_id);
	if (ret) {
		pr_err("ionfd2phys_id sysfs file creation failed\n");
		goto out_remove_file;
	}
	return 0;

out_remove_file:
	device_remove_file(ionfd2phys_cdev.device, &dev_attr_system_heap_id);
out_unregister:
	tz_cdev_unregister(&ionfd2phys_cdev);

	return ret;
}

static void __exit ionfd2phys_exit(void)
{
	tz_cdev_unregister(&ionfd2phys_cdev);
}

module_init(ionfd2phys_init);
module_exit(ionfd2phys_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Kuzin <a.kuzin@samsung.com>");
