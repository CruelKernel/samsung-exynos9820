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

#ifndef _FSCRYPTO_SDP_H
#define _FSCRYPTO_SDP_H

#ifdef CONFIG_FSCRYPT_SDP
#include <sdp/dek_common.h>
#ifndef CONFIG_DDAR
#define FSCRYPT_KNOX_FLG_SDP_MASK       0xFFFF0000
#else
#include "../fscrypt_knox_private.h"
#endif

#include <linux/fscrypto_sdp_cache.h>
#include <linux/fscrypto_sdp_ioctl.h>
#include <linux/fscrypto_sdp_name.h>
#include "fscrypto_sdp_dek_private.h"
#include "fscrypto_sdp_xattr_private.h"
#include "sdp_crypto.h"
/**
 * SDP Encryption context for inode
 */
#define PKG_NAME_SIZE 16
#define SDP_DEK_SDP_ENABLED             0x00100000
#define SDP_DEK_IS_SENSITIVE            0x00200000
#define SDP_DEK_IS_UNINITIALIZED        0x00400000
//#define SDP_DEK_MULTI_ENGINE            0x00400000
#define SDP_DEK_TO_SET_SENSITIVE        0x00800000
#define SDP_DEK_TO_CONVERT_KEY_TYPE     0x01000000
//#define SDP_DEK_DECRYPTED_FEK_SET       0x02000000
//#define SDP_DEK_IS_EMPTY_CTFM_SET       0x04000000
//#define SDP_DEK_TO_CLEAR_NONCE          0x08000000
//#define SDP_DEK_TO_CLEAR_CACHE          0x10000000
#define SDP_IS_CHAMBER_DIR              0x20000000
#define SDP_IS_DIRECTORY                0x40000000
#define SDP_IS_INO_CACHED	            0x80000000
#define SDP_IS_CLEARING_ONGOING         0x00010000
#define SDP_IS_FILE_IO_ONGOING          0x00020000

#define RV_PAGE_CACHE_CLEANED           1
#define RV_PAGE_CACHE_NOT_CLEANED       2

struct fscrypt_sdp_context {
	//Store knox_flags to fscrypt_context, not in this context.
	__u32 engine_id;
	__u32 sdp_dek_type;
	__u32 sdp_dek_len;
	__u8 sdp_dek_buf[DEK_MAXLEN];
	__u8 sdp_en_buf[MAX_EN_BUF_LEN];
} __attribute__((__packed__));

extern int dek_is_locked(int engine_id);
extern int dek_encrypt_dek_efs(int engine_id, dek_t *plainDek, dek_t *encDek);
extern int dek_decrypt_dek_efs(int engine_id, dek_t *encDek, dek_t *plainDek);
extern int dek_encrypt_fek(unsigned char *master_key, unsigned int master_key_len,
					unsigned char *fek, unsigned int fek_len,
					unsigned char *efek, unsigned int *efek_len);
extern int dek_decrypt_fek(unsigned char *master_key, unsigned int master_key_len,
					unsigned char *efek, unsigned int efek_len,
					unsigned char *fek, unsigned int *fek_len);
extern int fscrypt_sdp_get_engine_id(struct inode *inode);

#ifndef IS_ENCRYPTED //Implemented from 4.14(Beyond)
#define IS_ENCRYPTED(inode)	(1)
#endif

//Exclusively masking the shared flags
#define FSCRYPT_SDP_PARSE_FLAG_SDP_ONLY(flag) (flag & FSCRYPT_KNOX_FLG_SDP_MASK)
#define FSCRYPT_SDP_PARSE_FLAG_OUT_OF_SDP(flag) (flag & ~FSCRYPT_KNOX_FLG_SDP_MASK)

#endif

#endif	/* _FSCRYPTO_SDP_H */
