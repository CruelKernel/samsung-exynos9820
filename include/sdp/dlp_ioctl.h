/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * Sensitive Data Protection
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef DLP_IOCTL_H_
#define DLP_IOCTL_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#define DLP_DEV_PATH	"/dev/sdp_dlp"

#define __DLPIOC		0x78
#define MAX_EXT_LENGTH 1000 

typedef struct _dlp_lock_set {
	int user_id;
} dlp_lock_set;

typedef struct _dlp_extension_set {
	int user_id;
	char extensions[MAX_EXT_LENGTH+1];
} dlp_extension_set;

#define DLP_LOCK_ENABLE		_IOW(__DLPIOC, 1, dlp_lock_set)
#define DLP_LOCK_DISABLE	_IOW(__DLPIOC, 2, dlp_lock_set)
#define DLP_EXTENSION_SET	_IOW(__DLPIOC, 3, dlp_extension_set)

#endif /* DLP_IOCTL_H_ */
