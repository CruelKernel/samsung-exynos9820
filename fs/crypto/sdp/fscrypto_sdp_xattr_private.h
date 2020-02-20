/*
 *  Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FSCRYPTO_SDP_XATTR_H
#define _FSCRYPTO_SDP_XATTR_H

#ifdef CONFIG_FSCRYPT_SDP

#include <linux/fs.h>

int fscrypt_knox_set_context(struct inode *inode, void *ctx, size_t len);
int fscrypt_sdp_get_context(struct inode *inode, void *ctx, size_t len);
int fscrypt_sdp_set_context(struct inode *inode, void *ctx, size_t len, void *fs_data);
int fscrypt_sdp_set_context_nolock(struct inode *inode, void *ctx, size_t len, void *fs_data);

#endif

#endif	/* _FSCRYPTO_SDP_XATTR_H */
