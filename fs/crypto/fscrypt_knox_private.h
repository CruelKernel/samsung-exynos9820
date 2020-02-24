/*
 * fscrypto_knox_private.h
 *
 *  Created on: Oct 11, 2018
 *      Author: olic.moon
 */

#ifndef FS_CRYPTO_FSCRYPT_KNOX_PRIVATE_H_
#define FS_CRYPTO_FSCRYPT_KNOX_PRIVATE_H_

#ifdef CONFIG_DDAR
#include <sdp/dd.h>
#endif

#define FSCRYPT_KNOX_FLG_DDAR_SHIFT					8
#define FSCRYPT_KNOX_FLG_DDAR_MASK					0x0000FF00
#define FSCRYPT_KNOX_FLG_DDAR_ENABLED				(DD_POLICY_ENABLED << FSCRYPT_KNOX_FLG_DDAR_SHIFT)
#define FSCRYPT_KNOX_FLG_DDAR_USER_SPACE_CRYPTO		(DD_POLICY_USER_SPACE_CRYPTO << FSCRYPT_KNOX_FLG_DDAR_SHIFT)
#define FSCRYPT_KNOX_FLG_DDAR_KERNEL_CRYPTO			(DD_POLICY_KERNEL_CRYPTO << FSCRYPT_KNOX_FLG_DDAR_SHIFT)
#define FSCRYPT_KNOX_FLG_DDAR_GID_RESTRICTION		(DD_POLICY_GID_RESTRICTION << FSCRYPT_KNOX_FLG_DDAR_SHIFT)

#define FSCRYPT_KNOX_FLG_SDP_MASK					0xFFFF0000
#define FSCRYPT_KNOX_FLG_SDP_ENABLED             	0x00100000
#define FSCRYPT_KNOX_FLG_SDP_IS_SENSITIVE           0x00200000
#define FSCRYPT_KNOX_FLG_SDP_MULTI_ENGINE           0x00400000// eCryptfs header contains engine id.
#define FSCRYPT_KNOX_FLG_SDP_TO_SET_SENSITIVE       0x00800000
#define FSCRYPT_KNOX_FLG_SDP_TO_SET_PROTECTED       0x01000000
#define FSCRYPT_KNOX_FLG_SDP_DECRYPTED_FEK_SET      0x02000000// Be careful, this flag must be avoided to be set to xattr.
#define FSCRYPT_KNOX_FLG_SDP_IS_EMPTY_CTFM_SET      0x04000000
#define FSCRYPT_KNOX_FLG_SDP_TO_CLEAR_NONCE         0x08000000
//#define FSCRYPT_KNOX_FLG_SDP_TO_CLEAR_CACHE          0x10000000
#define FSCRYPT_KNOX_FLG_SDP_IS_CHAMBER_DIR         0x20000000
#define FSCRYPT_KNOX_FLG_SDP_IS_DIRECTORY           0x40000000
#define FSCRYPT_KNOX_FLG_SDP_IS_PROTECTED           0x80000000

static inline int fscrypt_dd_flg_enabled(int flags) {
	return (flags & FSCRYPT_KNOX_FLG_DDAR_ENABLED) ? 1:0;
}

static inline int fscrypt_dd_flg_userspace_crypto(int flags) {
	return (flags & FSCRYPT_KNOX_FLG_DDAR_USER_SPACE_CRYPTO) ? 1:0;
}

static inline int fscrypt_dd_flg_kernel_crypto(int flags) {
	return (flags & FSCRYPT_KNOX_FLG_DDAR_KERNEL_CRYPTO) ? 1:0;
}

static inline int fscrypt_dd_flg_gid_restricted(int flags, int gid) {
	return (flags & FSCRYPT_KNOX_FLG_DDAR_GID_RESTRICTION) ? 1:0;
}

struct fscrypt_context;

int dd_test_and_inherit_context(
		struct fscrypt_context *ctx,
		struct inode *parent, struct inode *child,
		struct fscrypt_info *ci, void *fs_data);

int update_encryption_context_with_dd_policy(
		struct inode *inode,
		const struct dd_policy *policy);

void *dd_get_info(const struct inode *inode);

#endif /* FS_CRYPTO_FSCRYPT_KNOX_PRIVATE_H_ */
