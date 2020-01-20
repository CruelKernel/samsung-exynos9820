/*
 * drivers/soc/samsung/secmem.c
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/pm_runtime.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/export.h>
#include <linux/pm_qos.h>
#include <linux/dma-contiguous.h>
#include <linux/ion_exynos.h>
#include <linux/smc.h>
#include <linux/dma-buf.h>

#include <asm/memory.h>
#include <asm/cacheflush.h>

#include <soc/samsung/secmem.h>

#define SECMEM_DEV_NAME	"s5p-smem"


#define DRM_PROT_VER_CHUNK_BASED_PROT	0
#define DRM_PROT_VER_BUFFER_BASED_PROT	1

struct miscdevice secmem;
struct secmem_crypto_driver_ftn *crypto_driver;

uint32_t instance_count;

#if defined(CONFIG_SOC_EXYNOS5433)
static uint32_t secmem_regions[] = {
	ION_EXYNOS_ID_G2D_WFD,
	ION_EXYNOS_ID_VIDEO,
	ION_EXYNOS_ID_SECTBL,
	ION_EXYNOS_ID_MFC_FW,
	ION_EXYNOS_ID_MFC_NFW,
};

static char *secmem_regions_name[] = {
	"g2d_wfd",	/* 0 */
	"video",	/* 1 */
	"sectbl",       /* 2 */
	"mfc_fw",       /* 3 */
	"mfc_nfw",      /* 4 */
	NULL
};
#elif defined(CONFIG_SOC_EXYNOS7420)
static uint32_t secmem_regions[] = {
	ION_EXYNOS_ID_G2D_WFD,
	ION_EXYNOS_ID_VIDEO,
	ION_EXYNOS_ID_VIDEO_EXT,
	ION_EXYNOS_ID_MFC_FW,
	ION_EXYNOS_ID_MFC_NFW,
};

static char *secmem_regions_name[] = {
	"g2d_wfd",	/* 0 */
	"video",	/* 1 */
	"video_ext",	/* 2 */
	"mfc_fw",	/* 3 */
	"mfc_nfw",	/* 4 */
	NULL
};
#endif

static bool drm_onoff;
static DEFINE_MUTEX(drm_lock);
static DEFINE_MUTEX(smc_lock);

struct secmem_info {
	struct device	*dev;
	bool		drm_enabled;
};

struct protect_info {
	uint32_t dev;
	uint32_t enable;
};

#define SECMEM_IS_PAGE_ALIGNED(addr) (!((addr) & (~PAGE_MASK)))

int drm_enable_locked(struct secmem_info *info, bool enable)
{
	if (drm_onoff == enable) {
		pr_err("%s: DRM is already %s\n", __func__, drm_onoff ? "on" : "off");
		return -EINVAL;
	}

	drm_onoff = enable;
	/*
	 * this will only allow this instance to turn drm_off either by
	 * calling the ioctl or by closing the fd
	 */
	info->drm_enabled = enable;

	return 0;
}

static int secmem_open(struct inode *inode, struct file *file)
{
	struct miscdevice *miscdev = file->private_data;
	struct device *dev = miscdev->this_device;
	struct secmem_info *info;

	info = kzalloc(sizeof(struct secmem_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->dev = dev;
	file->private_data = info;

	mutex_lock(&drm_lock);
	instance_count++;
	mutex_unlock(&drm_lock);

	return 0;
}

static int secmem_release(struct inode *inode, struct file *file)
{
	struct secmem_info *info = file->private_data;

	/* disable drm if we were the one to turn it on */
	mutex_lock(&drm_lock);
	instance_count--;
	if (instance_count == 0) {
		if (info->drm_enabled) {
			int ret;
			ret = drm_enable_locked(info, false);
			if (ret < 0)
				pr_err("fail to lock/unlock drm status. lock = %d\n", false);
		}
	}

	mutex_unlock(&drm_lock);

	kfree(info);
	return 0;
}

static long secmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct secmem_info *info = filp->private_data;

	switch (cmd) {
#if defined(CONFIG_ION) || defined(CONFIG_ION_EXYNOS)
	case (uint32_t)SECMEM_IOC_GET_FD_PHYS_ADDR:
	{
		struct secfd_info fd_info;
		struct dma_buf *dmabuf;
		struct dma_buf_attachment *attachment;
		struct sg_table *sgt;

		if (copy_from_user(&fd_info, (int __user *)arg,
					sizeof(fd_info)))
			return -EFAULT;

		dmabuf = dma_buf_get(fd_info.fd);
		if (IS_ERR_OR_NULL(dmabuf)) {
			pr_err("smem ioctl error(%d)\n", __LINE__);
			return -ENOMEM;
		}

		attachment = dma_buf_attach(dmabuf, info->dev);
		if (!attachment) {
			pr_err("smem ioctl error(%d)\n", __LINE__);
			dma_buf_put(dmabuf);
			return -ENOMEM;
		}

		sgt = dma_buf_map_attachment(attachment, DMA_TO_DEVICE);
		if (!sgt) {
			pr_err("smem ioctl error(%d)\n", __LINE__);
			dma_buf_detach(dmabuf, attachment);
			dma_buf_put(dmabuf);
			return -ENOMEM;
		}

		fd_info.phys = sg_phys(sgt->sgl);
		pr_debug("%s: physical addr from kernel space = 0x%08x\n",
				__func__, (unsigned int)fd_info.phys);

		dma_buf_unmap_attachment(attachment, sgt, DMA_TO_DEVICE);
		dma_buf_detach(dmabuf, attachment);
		dma_buf_put(dmabuf);

		if (copy_to_user((void __user *)arg, &fd_info, sizeof(fd_info)))
			return -EFAULT;
		break;
	}
#endif
	case (uint32_t)SECMEM_IOC_GET_DRM_ONOFF:
		smp_rmb();
		if (copy_to_user((void __user *)arg, &drm_onoff, sizeof(bool)))
			return -EFAULT;
		break;
	case (uint32_t)SECMEM_IOC_SET_DRM_ONOFF:
	{
		int ret, val = 0;

		if (copy_from_user(&val, (int __user *)arg, sizeof(int)))
			return -EFAULT;

		mutex_lock(&drm_lock);
		if ((info->drm_enabled && !val) ||
		    (!info->drm_enabled && val)) {
			/*
			 * 1. if we enabled drm, then disable it
			 * 2. if we don't already hdrm enabled,
			 *    try to enable it.
			 */
			ret = drm_enable_locked(info, val);
			if (ret < 0)
				pr_err("fail to lock/unlock drm status. lock = %d\n", val);
		}
		mutex_unlock(&drm_lock);
		break;
	}
	case (uint32_t)SECMEM_IOC_GET_CRYPTO_LOCK:
	{
		break;
	}
	case (uint32_t)SECMEM_IOC_RELEASE_CRYPTO_LOCK:
	{
		break;
	}
	case (uint32_t)SECMEM_IOC_SET_TZPC:
	{
		break;
	}
	case (uint32_t)SECMEM_IOC_SET_VIDEO_EXT_PROC:
	{
		int val, ret;

		if (copy_from_user(&val, (int __user *)arg, sizeof(int)))
			return -EFAULT;
		mutex_lock(&smc_lock);
		ret = exynos_smc((uint32_t)(SMC_DRM_VIDEO_PROC), val, 0, 0);
		if (ret) {
			pr_err("Failed to control VIDEO EXT region protection. prot = %d\n", val);
			mutex_unlock(&smc_lock);
			return -ENOMEM;
		}

		mutex_unlock(&smc_lock);
		break;
	}
	case (uint32_t)SECMEM_IOC_GET_DRM_PROT_VER:
	{
		int val;

		val = DRM_PROT_VER_BUFFER_BASED_PROT;
		if (copy_to_user((void __user *)arg, &val, sizeof(int)))
			return -EFAULT;

		break;
	}
	default:
		return -ENOTTY;
	}

	return 0;
}

#if 0
void secmem_crypto_register(struct secmem_crypto_driver_ftn *ftn)
{
	crypto_driver = ftn;
}
EXPORT_SYMBOL(secmem_crypto_register);

void secmem_crypto_deregister(void)
{
	crypto_driver = NULL;
}
EXPORT_SYMBOL(secmem_crypto_deregister);
#endif

static const struct file_operations secmem_fops = {
	.owner		= THIS_MODULE,
	.open		= secmem_open,
	.release	= secmem_release,
	.compat_ioctl = secmem_ioctl,
	.unlocked_ioctl = secmem_ioctl,
};

struct miscdevice secmem = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= SECMEM_DEV_NAME,
	.fops	= &secmem_fops,
};

static int __init secmem_init(void)
{
	int ret;

	ret = misc_register(&secmem);
	if (ret) {
		pr_err("%s: SECMEM can't register misc on minor=%d\n",
			__func__, MISC_DYNAMIC_MINOR);
		return ret;
	}

	crypto_driver = NULL;

	return 0;
}

static void __exit secmem_exit(void)
{
	misc_deregister(&secmem);
}

module_init(secmem_init);
module_exit(secmem_exit);
