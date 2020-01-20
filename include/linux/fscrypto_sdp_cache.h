/*
 *  Copyright (C) 2017 Samsung Electronics Co., Ltd.
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

#ifndef _FSCRYPTO_SDP_CACHE_H
#define _FSCRYPTO_SDP_CACHE_H

#ifdef CONFIG_FSCRYPT_SDP

#include <linux/fs.h>

int fscrypt_sdp_cache_init(void);
void fscrypt_sdp_cache_add_inode_num(struct inode *inode);
void fscrypt_sdp_cache_remove_inode_num(struct inode *inode);
void fscrypt_sdp_cache_drop_inode_mappings(int engine_id);
int fscrypt_sdp_file_not_readable(struct file *file);
int fscrypt_sdp_file_not_writable(struct file *file);
void fscrypt_sdp_unset_file_io_ongoing(struct file *file);
void fscrypt_sdp_unset_clearing_ongoing(struct inode *inode);
bool fscrypt_sdp_is_cache_releasable(struct inode *inode);
bool fscrypt_sdp_is_locked_sensitive_inode(struct inode *inode);
int fscrypt_sdp_d_delete_wrapper(const struct dentry *dentry);
void fscrypt_sdp_drop_inode(struct inode *inode);
#endif

#endif	/* _FSCRYPTO_SDP_CACHE_H */
