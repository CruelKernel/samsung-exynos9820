/*
 * Copyright (C) 2012-2020, Samsung Electronics Co., Ltd.
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

#include <linux/atomic.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mmzone.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include "tz_cdev.h"
#include "tz_common.h"
#include "tz_mem.h"
#include "tzdev.h"
#include "tzlog.h"

struct tzdev_mem_priv {
	struct mutex mutex;
	unsigned int id;
};

static atomic_t tzdev_mem_device_ready = ATOMIC_INIT(0);

static int tzdev_mem_op_open(struct inode *inode, struct file *filp)
{
	struct tzdev_mem_priv *priv;

	(void)inode;

	priv = kmalloc(sizeof(struct tzdev_mem_priv), GFP_KERNEL);
	if (!priv) {
		tzdev_teec_error("Failed to allocate iwshmem private data.\n");
		return -ENOMEM;
	}

	mutex_init(&priv->mutex);
	priv->id = 0;

	filp->private_data = priv;

	return 0;
}

static int tzdev_mem_op_release(struct inode *inode, struct file *filp)
{
	struct tzdev_mem_priv *priv;

	(void)inode;

	priv = filp->private_data;
	if (priv->id)
		tzdev_mem_release_user(priv->id);

	mutex_destroy(&priv->mutex);
	kfree(priv);

	return 0;
}

static long tzdev_mem_op_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct tzdev_mem_priv *priv = filp->private_data;
	struct tzio_mem_register __user *argp = (struct tzio_mem_register __user *)arg;
	struct tzio_mem_register memreg;
	int ret;

	if (cmd != TZIO_MEM_REGISTER)
		return -ENOTTY;

	if (copy_from_user(&memreg, argp, sizeof(struct tzio_mem_register)))
		return -EFAULT;

	mutex_lock(&priv->mutex);

	if (priv->id) {
		ret = -EEXIST;
		goto out;
	}

	ret = tzdev_mem_register_user(memreg.size, memreg.write);
	if (ret < 0)
		goto out;

	priv->id = ret;

out:
	mutex_unlock(&priv->mutex);

	return ret;
}

static int tzdev_mem_op_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct tzdev_mem_priv *priv = filp->private_data;
	struct tzdev_mem_reg *mem;
	unsigned long i;
	int ret;

	if (vma->vm_pgoff)
		return -EINVAL;

	mutex_lock(&priv->mutex);

	if (!priv->id) {
		ret = -ENXIO;
		goto out;
	}
	if (!(vma->vm_flags & VM_WRITE)) {
		ret = -EPERM;
		goto out;
	}
	if (vma->vm_flags & VM_EXEC) {
		ret = -EPERM;
		goto out;
	}

	vma->vm_flags |= VM_DONTCOPY | VM_DONTEXPAND;
	vma->vm_flags &= ~VM_MAYEXEC;

	BUG_ON(tzdev_mem_find(priv->id, &mem));

	if (vma_pages(vma) != mem->nr_pages) {
		ret = -EIO;
		goto out;
	}

	for (i = 0; i < mem->nr_pages; i++) {
		ret = vm_insert_page(vma, vma->vm_start + i * PAGE_SIZE, mem->pages[i]);
		if (ret)
			goto out;
	}

out:
	mutex_unlock(&priv->mutex);

	return ret;
}

static const struct file_operations tzdev_mem_fops = {
	.owner = THIS_MODULE,
	.open = tzdev_mem_op_open,
	.release = tzdev_mem_op_release,
	.unlocked_ioctl = tzdev_mem_op_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tzdev_mem_op_ioctl,
#endif /* CONFIG_COMPAT */
	.mmap = tzdev_mem_op_mmap,
};

static struct tz_cdev tzdev_mem_cdev = {
	.name = "tziwshmem",
	.fops = &tzdev_mem_fops,
	.owner = THIS_MODULE,
};

int tzdev_umem_register(void)
{
	int ret;

	ret = tz_cdev_register(&tzdev_mem_cdev);
	if (ret) {
		tzdev_teec_error("Failed to create iwshmem device, error=%d\n", ret);
		return ret;
	}

	atomic_set(&tzdev_mem_device_ready, 1);

	tzdev_teec_info("Iwshmem user interface initialization done.\n");

	return 0;
}

int tzdev_umem_unregister(void)
{
	if (!atomic_cmpxchg(&tzdev_mem_device_ready, 1, 0)) {
		tzdev_teec_info("Iwshmem user interface was not initialized.\n");
		return -EPERM;
	}

	tz_cdev_unregister(&tzdev_mem_cdev);

	tzdev_teec_info("Iwshmem user interface finalization done.\n");

	return 0;
}
