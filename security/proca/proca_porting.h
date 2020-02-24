/*
 * This is needed backporting of source code from Kernel version 4.x
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 *
 * Hryhorii Tur, <hryhorii.tur@partner.samsung.com>
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

#ifndef __LINUX_PROCA_PORTING_H
#define __LINUX_PROCA_PORTING_H

#include <linux/version.h>
#include <linux/memory.h>
#include <linux/fs.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)

static inline struct inode *locks_inode(const struct file *f)
{
	return f->f_path.dentry->d_inode;
}

static inline ssize_t
__vfs_getxattr(struct dentry *dentry, struct inode *inode, const char *name,
	       void *value, size_t size)
{
	if (inode->i_op->getxattr)
		return inode->i_op->getxattr(dentry, name, value, size);
	else
		return -EOPNOTSUPP;
}

#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 17)
/* Some linux headers are moved.
 * Since Kernel 4.11 get_task_struct moved to sched/ folder.
 */
#include <linux/sched/task.h>
#else
#include <linux/sched.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 21)
/* d_backing_inode is absent on some Linux Kernel 3.x. but it back porting for
 * few Samsung kernels:
 * Exynos7570 (3.18.14): CL 13680422
 * Exynos7870 (3.18.14): CL 14632149
 * SDM450 (3.18.71): initially
 */
#if !defined(CONFIG_SOC_EXYNOS7570) && !defined(CONFIG_ARCH_SDM450) && \
	!defined(CONFIG_SOC_EXYNOS7870)
#define d_backing_inode(dentry)	((dentry)->d_inode)
#endif
#define inode_lock(inode)	mutex_lock(&(inode)->i_mutex)
#define inode_unlock(inode)	mutex_unlock(&(inode)->i_mutex)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 2, 0)
#define security_add_hooks(hooks, count, name)
#else
#define LINUX_LSM_SUPPORTED
#include <linux/lsm_hooks.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
#define security_add_hooks(hooks, count, name) security_add_hooks(hooks, count)
#endif
#endif

/*
 * VA_BITS is present only on 64 bit kernels
 */
#if defined(CONFIG_ARM)
#define VA_BITS 30
#endif

/*
 * VA_START macro is backported to SDM450 kernel (3.18.120)
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0) && \
	!(defined(CONFIG_ARCH_SDM450) && defined(CONFIG_ARM64))
#define VA_START		(UL(0xffffffffffffffff) - \
	(UL(1) << VA_BITS) + 1)
#endif

/*
 * KASLR is backported to 4.4 kernels
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)

static inline uintptr_t get_kimage_vaddr(void)
{
	return PAGE_OFFSET;
}

static inline uintptr_t get_kimage_voffset(void)
{
	return get_kimage_vaddr() - virt_to_phys((void *)get_kimage_vaddr());
}

#else

static inline u64 get_kimage_vaddr(void)
{
	return kimage_vaddr;
}

static inline u64 get_kimage_voffset(void)
{
	return kimage_voffset;
}
#endif

#endif /* __LINUX_PROCA_PORTING_H */
