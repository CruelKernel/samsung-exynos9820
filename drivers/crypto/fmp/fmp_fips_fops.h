/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _FMP_FIPS_FOPS_H_
#define _FMP_FIPS_FOPS_H_
int fmp_fips_open(struct inode *inode, struct file *file);
int fmp_fips_release(struct inode *inode, struct file *file);
long fmp_fips_ioctl(struct file *file, unsigned int cmd, unsigned long arg_);
#ifdef CONFIG_COMPAT
long fmp_fips_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg_);
#endif /* CONFIG_COMPAT */
#endif /* _FMP_FIPS_FOPS_H_ */
