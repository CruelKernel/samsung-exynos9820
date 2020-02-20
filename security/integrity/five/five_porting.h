/*
 * This is needed backporting of source code from Kernel version 4.x
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 *
 * Egor Uleyskiy, <e.uleyskiy@samsung.com>
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

#ifndef __LINUX_FIVE_PORTING_H
#define __LINUX_FIVE_PORTING_H

#include <linux/version.h>

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
#define inode_lock_nested(inode, subclass) \
				mutex_lock_nested(&(inode)->i_mutex, subclass)
#define inode_unlock(inode)	mutex_unlock(&(inode)->i_mutex)
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 14, 20)
/* kmemcheck is gone.
 * Since Kernel 4.14.21 SLAB_NOTRACK isn't present in Kernel.
 * But for backward compatibility with old Kernels
 * we have to define it here.
 */
#define SLAB_NOTRACK 0
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 17)
/* Some linux headers are moved.
 * Since Kernel 4.11 get_task_struct moved to sched/ folder.
 */
#include <linux/sched/task.h>
#else
#include <linux/sched.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
#include <linux/fs.h>

static inline int __vfs_removexattr(struct dentry *dentry, const char *name)
{
	struct inode *inode = d_inode(dentry);

	if (!inode->i_op->removexattr)
		return -EOPNOTSUPP;

	return inode->i_op->removexattr(dentry, name);
}

static inline ssize_t __vfs_getxattr(struct dentry *dentry, struct inode *inode,
				     const char *name, void *value, size_t size)
{
	if (!inode->i_op->getxattr)
		return -EOPNOTSUPP;

	return inode->i_op->getxattr(dentry, name, value, size);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
/* __GFP_WAIT was changed to __GFP_RECLAIM in
 * https://lore.kernel.org/patchwork/patch/592262/
 */
#define __GFP_RECLAIM __GFP_WAIT
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
#include <linux/uaccess.h>

static inline ssize_t __vfs_read(struct file *file, char __user *buf,
				 size_t count, loff_t *pos)
{
	ssize_t ret;

	if (file->f_op->read)
		ret = file->f_op->read(file, buf, count, pos);
	else if (file->f_op->aio_read)
		ret = do_sync_read(file, buf, count, pos);
	else if (file->f_op->read_iter)
		ret = new_sync_read(file, buf, count, pos);
	else
		ret = -EINVAL;

	return ret;
}

static inline int integrity_kernel_read(struct file *file, loff_t offset,
					char *addr, unsigned long count)
{
	mm_segment_t old_fs;
	char __user *buf = (char __user *)addr;
	ssize_t ret;

	if (!(file->f_mode & FMODE_READ))
		return -EBADF;

	old_fs = get_fs();
	set_fs(get_ds());
	ret = __vfs_read(file, buf, count, &offset);
	set_fs(old_fs);

	return ret;
}
#endif

#endif /* __LINUX_FIVE_PORTING_H */
