/*
 * sdp_dek.c
 *
 */
#include <linux/kthread.h>
#include <linux/pagemap.h>
#include <linux/string.h>
#include <asm/unaligned.h>

#include <sdp/fs_handler.h>
#include "sdp_crypto.h"
#include "../fscrypt_private.h"

#ifdef CONFIG_FS_CRYPTO_SEC_EXTENSION
# include "../crypto_sec.h"
# define SDP_DERIVED_KEY_OUTPUT_SIZE SEC_FS_DERIVED_KEY_OUTPUT_SIZE
#else
# define SDP_DERIVED_KEY_OUTPUT_SIZE SDP_CRYPTO_SHA512_OUTPUT_SIZE
#endif
#ifdef CONFIG_CRYPTO_KBKDF_CTR_HMAC_SHA512
# include <crypto/kbkdf.h>
#endif

static int __fscrypt_get_sdp_context(struct inode *inode, struct fscrypt_info *crypt_info);
inline int __fscrypt_get_sdp_dek(struct fscrypt_info *crypt_info, unsigned char *dek, unsigned int dek_len);
inline int __fscrypt_get_sdp_uninitialized_dek(struct fscrypt_info *crypt_info,
						unsigned char *dek, unsigned int dek_len);
inline int __fscrypt_set_sdp_fek(struct fscrypt_info *crypt_info,
									struct fscrypt_key *master_key,
									unsigned char *fek, unsigned int fek_len);
inline int __fscrypt_get_sdp_fek(struct fscrypt_info *crypt_info,
									struct fscrypt_key *master_key,
									unsigned char *fek, unsigned int *fek_len);
inline int __fscrypt_set_sdp_dek(struct fscrypt_info *crypt_info, unsigned char *fe_key, int fe_key_len);
inline int __fscrypt_sdp_finish_set_sensitive(struct inode *inode,
				struct fscrypt_context *ctx, struct fscrypt_info *crypt_info,
				struct fscrypt_key *key);
inline void __fscrypt_sdp_finalize_tasks(struct inode *inode,
						struct fscrypt_info *ci, u8 *raw_key, int key_len);
static inline int __fscrypt_derive_nek_iv(u8 *drv_key, u32 drv_key_len, u8 *out, u32 out_len);
static inline int __fscrypt_get_nonce(u8 *key, u32 key_len,
						u8 *enonce, u32 nonce_len,
						u8 *out, u32 out_len);
static inline int __fscrypt_set_nonce(u8 *key, u32 key_len,
						u8 *enonce, u32 nonce_len,
						u8 *out, u32 out_len);
static struct kmem_cache *sdp_info_cachep;

inline struct sdp_info *fscrypt_sdp_alloc_sdp_info(void)
{
	struct sdp_info *ci_sdp_info;

	ci_sdp_info = kmem_cache_alloc(sdp_info_cachep, GFP_NOFS);
	if (!ci_sdp_info) {
		DEK_LOGE("Failed to alloc sdp info!!\n");
		return NULL;
	}
	ci_sdp_info->sdp_flags = 0;
	spin_lock_init(&ci_sdp_info->sdp_flag_lock);

	return ci_sdp_info;
}

void dump_file_key_hex(const char* tag, uint8_t *data, size_t data_len)
{
	static const char *hex = "0123456789ABCDEF";
	static const char delimiter = ' ';
	int i;
	char *buf;
	size_t buf_len;

	if (tag == NULL || data == NULL || data_len <= 0) {
		return;
	}

	buf_len = data_len * 3;
	buf = (char *)kmalloc(buf_len, GFP_ATOMIC);
	if (buf == NULL) {
		return;
	}
	for (i= 0 ; i < data_len ; i++) {
		buf[i*3 + 0] = hex[(data[i] >> 4) & 0x0F];
		buf[i*3 + 1] = hex[(data[i]) & 0x0F];
		buf[i*3 + 2] = delimiter;
	}
	buf[buf_len - 1] = '\0';
	printk(KERN_ERR
		"[%s] %s(len=%d) : %s\n", "DEK_DBG", tag, data_len, buf);
	kfree(buf);
}

#ifdef CONFIG_SDP_KEY_DUMP
int fscrypt_sdp_dump_file_key(struct inode *inode)
{
	struct fscrypt_info *ci = inode->i_crypt_info;
	struct fscrypt_key file_encryption_key;
	int res;

	inode_lock(inode);

	DEK_LOGE("dump file key for ino (%ld)\n", inode->i_ino);

	memset(&file_encryption_key, 0, sizeof(file_encryption_key));
	if (!ci->ci_sdp_info) {
		DEK_LOGE("ci_sdp_info is null\n");
		res = fscrypt_get_encryption_key(inode, &file_encryption_key);
	} else {
		res = fscrypt_get_encryption_key_classified(inode, &file_encryption_key);
	}
	if (res) {
		DEK_LOGE("failed to retrieve file encryption key, rc:%d\n", res);
		goto out;
	}
	dump_file_key_hex("FILE ENCRYPTION KEY", file_encryption_key.raw, file_encryption_key.size);
	memzero_explicit(file_encryption_key.raw, file_encryption_key.size);

out:
	inode_unlock(inode);
	return res;
}
#endif

int fscrypt_sdp_set_sdp_policy(struct inode *inode, int engine_id)
{
	struct fscrypt_info *ci = inode->i_crypt_info;
	struct fscrypt_sdp_context sdp_ctx;
	struct fscrypt_context ctx;
	int res;

	inode_lock(inode);

	DEK_LOGD("set sdp policy to %ld\n",  inode->i_ino);

	if (!S_ISDIR(inode->i_mode)) {
		DEK_LOGE("set_policy: operation permitted only to directory\n");
		res = -ENOTDIR;
		goto unlock_finsh;
	}

	if (!inode->i_sb->s_cop->empty_dir(inode)) {
		DEK_LOGE("set_policy: target directory is not empty\n");
		res = -ENOTEMPTY;
		goto unlock_finsh;
	}

	res = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx));
	if (res != sizeof(ctx)) {
		if (res >= 0)
		DEK_LOGE("set_policy: failed to get fscrypt ctx (err:%d)\n", res);
		    res = -EEXIST;
		goto unlock_finsh;
	}

	if (FSCRYPT_SDP_PARSE_FLAG_OUT_OF_SDP(ctx.knox_flags)) {
		DEK_LOGE("set_policy: flags already occupied : 0x%08x\n", ctx.knox_flags);
		res = -EFAULT;
		goto unlock_finsh;
	}

	sdp_ctx.engine_id = engine_id;
	sdp_ctx.sdp_dek_type = DEK_TYPE_PLAIN;
	sdp_ctx.sdp_dek_len = 0;
	memset(sdp_ctx.sdp_dek_buf, 0, DEK_MAXLEN);
	memset(sdp_ctx.sdp_en_buf, 0, MAX_EN_BUF_LEN);

	res = fscrypt_sdp_set_context_nolock(inode, &sdp_ctx, sizeof(sdp_ctx));
	if (res) {
		DEK_LOGE("set_policy: failed to set sdp context\n");
		goto unlock_finsh;
	}

	ctx.knox_flags |= SDP_DEK_SDP_ENABLED;
	ctx.knox_flags |= SDP_IS_DIRECTORY;

	res = inode->i_sb->s_cop->set_context(inode, &ctx, sizeof(ctx), NULL);
	if (res) {
		DEK_LOGE("set_policy: failed to set fscrypt ctx (err:%d)\n", res);
		goto unlock_finsh;
	}

	if (!ci->ci_sdp_info) {
		struct sdp_info *ci_sdp_info = fscrypt_sdp_alloc_sdp_info();
		if (!ci_sdp_info) {
			res = -ENOMEM;
			goto unlock_finsh;
		}

		ci_sdp_info->sdp_flags = ctx.knox_flags;
		ci_sdp_info->engine_id = sdp_ctx.engine_id;
		ci_sdp_info->sdp_dek.type = sdp_ctx.sdp_dek_type;
		ci_sdp_info->sdp_dek.len = sdp_ctx.sdp_dek_len;

		if (cmpxchg(&ci->ci_sdp_info, NULL, ci_sdp_info) != NULL) {
			fscrypt_sdp_put_sdp_info(ci_sdp_info);
			res = -EFAULT;
			goto unlock_finsh;
		}
	}

	DEK_LOGD("set_policy: updated as 0x%08x\n", ctx.knox_flags);

unlock_finsh:
	inode_unlock(inode);
	return res;
}

int fscrypt_sdp_set_sensitive(struct inode *inode, int engine_id, struct fscrypt_key *key)
{
	struct fscrypt_info *ci = inode->i_crypt_info;
	struct fscrypt_sdp_context sdp_ctx;
	struct fscrypt_context ctx;
	int rc = 0;
	int is_dir = 0;
	int is_native = 0;

	if (!ci->ci_sdp_info) {
		struct sdp_info *ci_sdp_info = fscrypt_sdp_alloc_sdp_info();
		if (!ci_sdp_info) {
			return -ENOMEM;
		}

		if (cmpxchg(&ci->ci_sdp_info, NULL, ci_sdp_info) != NULL) {
			fscrypt_sdp_put_sdp_info(ci_sdp_info);
			return -EPERM;
		}
	}

	is_native = fscrypt_sdp_is_native(ci);

	ci->ci_sdp_info->engine_id = engine_id;
	if (S_ISDIR(inode->i_mode)) {
		ci->ci_sdp_info->sdp_flags |= SDP_DEK_IS_SENSITIVE;
		is_dir = 1;
	} else if (S_ISREG(inode->i_mode)) {
		ci->ci_sdp_info->sdp_flags |= SDP_DEK_TO_SET_SENSITIVE;
	}

	sdp_ctx.engine_id = engine_id;
	sdp_ctx.sdp_dek_type = DEK_TYPE_PLAIN;
	sdp_ctx.sdp_dek_len = DEK_MAXLEN;
	memset(sdp_ctx.sdp_dek_buf, 0, DEK_MAXLEN);
	memset(sdp_ctx.sdp_en_buf, 0, MAX_EN_BUF_LEN);

	rc = fscrypt_sdp_set_context(inode, &sdp_ctx, sizeof(sdp_ctx));

	if (rc) {
		DEK_LOGE("%s: Failed to set sensitive flag (err:%d)\n", __func__, rc);
		return rc;
	}

	rc = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx));
	if (rc < 0) {
		DEK_LOGE("%s: Failed to get fscrypt ctx (err:%d)\n", __func__, rc);
		return rc;
	}

	if (!is_dir) {
		//run setsensitive with nonce from ctx
		rc = __fscrypt_sdp_finish_set_sensitive(inode, &ctx, ci, key);
	} else {
		ctx.knox_flags = FSCRYPT_SDP_PARSE_FLAG_OUT_OF_SDP(ctx.knox_flags) | SDP_DEK_IS_SENSITIVE;
		if (is_native) {
			ctx.knox_flags |= SDP_DEK_SDP_ENABLED;
		}
		inode_lock(inode);
		rc = inode->i_sb->s_cop->set_context(inode, &ctx, sizeof(ctx), NULL);
		inode_unlock(inode);
	}

	return rc;
}

int fscrypt_sdp_set_protected(struct inode *inode, int engine_id)
{
	struct fscrypt_info *ci = inode->i_crypt_info;
	struct fscrypt_sdp_context sdp_ctx;
	struct fscrypt_context ctx;
	struct fscrypt_key fek;
	int is_native = 0;
	int rc = 0;

	if (!ci || !ci->ci_sdp_info)
		return -EPERM;

	if (!(ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE)) {
		DEK_LOGD("set_protected: already in protected\n");
		return 0;
	}

	if (dek_is_locked(ci->ci_sdp_info->engine_id)) {
		DEK_LOGE("set_protected: failed due to sdp in locked state\n");
		return -EPERM;
	}

	rc = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx));
	if (rc != sizeof(ctx)) {
		DEK_LOGE("set_protected: failed to get fscrypt ctx (err:%d)\n", rc);
		return -EINVAL;
	}

	if (!S_ISDIR(inode->i_mode)) {
		rc = fscrypt_sdp_get_context(inode, &sdp_ctx, sizeof(sdp_ctx));
		if (rc != sizeof(sdp_ctx)) {
			DEK_LOGE("set_protected: failed to get sdp context (err:%d)\n", rc);
			return -EINVAL;
		}
#if DEK_DEBUG
		hex_key_dump("set_protected: enonce", sdp_ctx.sdp_en_buf, MAX_EN_BUF_LEN);
#endif
		fek.size = FS_MAX_KEY_SIZE;
		rc = __fscrypt_get_sdp_dek(ci, fek.raw, fek.size);
		if (rc) {
			DEK_LOGE("set_protected: failed to find fek (err:%d)\n", rc);
			return rc;
		}
#if DEK_DEBUG
		hex_key_dump("set_protected: fek", fek.raw, fek.size);
#endif
		rc = __fscrypt_get_nonce(fek.raw, fek.size,
								sdp_ctx.sdp_en_buf, FS_KEY_DERIVATION_NONCE_SIZE,
								ctx.nonce, FS_KEY_DERIVATION_NONCE_SIZE);
		if (rc) {
			DEK_LOGE("set_protected: failed to get nonce (err:%d)\n", rc);
			goto out;
		}
#if DEK_DEBUG
		hex_key_dump("set_protected: nonce", ctx.nonce, FS_KEY_DERIVATION_NONCE_SIZE);
#endif
	}

	is_native = fscrypt_sdp_is_native(ci);
	if (is_native) {
		if (S_ISREG(inode->i_mode)) {
			rc = fscrypt_sdp_store_fek(inode, ci, fek.raw, fek.size);
			if (rc) {
				DEK_LOGE("set_protected: failed to store fek (err:%d)\n", rc);
				goto out;
			}
		}
		ci->ci_sdp_info->engine_id = engine_id;
		ci->ci_sdp_info->sdp_flags &= ~SDP_DEK_IS_SENSITIVE;

		ctx.knox_flags &= ~SDP_DEK_IS_SENSITIVE;
		sdp_ctx.engine_id = engine_id;
		if (S_ISREG(inode->i_mode)) {
			sdp_ctx.sdp_dek_type = ci->ci_sdp_info->sdp_dek.type;
			sdp_ctx.sdp_dek_len = ci->ci_sdp_info->sdp_dek.len;
			memcpy(sdp_ctx.sdp_dek_buf, ci->ci_sdp_info->sdp_dek.buf, DEK_MAXLEN); //Full copy without memset
		} else {
			sdp_ctx.sdp_dek_type = DEK_TYPE_PLAIN;
			sdp_ctx.sdp_dek_len = 0;
			memset(sdp_ctx.sdp_dek_buf, 0, DEK_MAXLEN);
		}
		memset(sdp_ctx.sdp_en_buf, 0, MAX_EN_BUF_LEN);
	} else {
		// Clear all in non-native case
		ctx.knox_flags = FSCRYPT_SDP_PARSE_FLAG_OUT_OF_SDP(ci->ci_sdp_info->sdp_flags);
		sdp_ctx.sdp_dek_type = DEK_TYPE_PLAIN;
		sdp_ctx.sdp_dek_len = 0;
		memset(sdp_ctx.sdp_dek_buf, 0, DEK_MAXLEN);
		memset(sdp_ctx.sdp_en_buf, 0, MAX_EN_BUF_LEN);
		// Does it work for non-native case???
		// rc = fscrypt_sdp_set_context(inode, NULL, 0);
	}

	rc = fscrypt_sdp_set_context(inode, &sdp_ctx, sizeof(sdp_ctx));
	if (rc) {
		DEK_LOGE("set_protected: set sdp context (err:%d)\n", rc);
		goto out;
	}

	inode_lock(inode);
	rc = inode->i_sb->s_cop->set_context(inode, &ctx, sizeof(ctx), NULL);
	inode_unlock(inode);
	if (rc) {
		DEK_LOGE("set_protected: failed to set fscrypt ctx (err:%d)\n", rc);
		goto out;
	}

	fscrypt_sdp_cache_remove_inode_num(inode);
	mapping_clear_sensitive(inode->i_mapping);

out:
	memzero_explicit(&fek, sizeof(fek));
	return rc;
}

int fscrypt_sdp_initialize(struct inode *inode, int engine_id, struct fscrypt_key *key)
{
	struct fscrypt_info *ci = inode->i_crypt_info;
	struct fscrypt_sdp_context sdp_ctx;
	struct fscrypt_context ctx;
	int res = 0;

	if (!ci || !ci->ci_sdp_info)
		return -EPERM;

	if (!S_ISREG(inode->i_mode))
		return -EPERM;

	if (!(ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_UNINITIALIZED))
		return res;

	res = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx));
	if (res != sizeof(ctx)) {
		if (res >= 0)
			res = -EEXIST;
		DEK_LOGE("sdp_initialize: failed to get fscrypt ctx (err:%d)\n", res);
		goto out;
	}

	if (ci->ci_sdp_info->sdp_flags & SDP_DEK_TO_SET_SENSITIVE)
	{
		// The key is fek, and the objective is { fek }_SDPK
		res = __fscrypt_sdp_finish_set_sensitive(inode, &ctx, ci, key);
		if (res) {
			DEK_LOGE("sdp_initialize: failed to set sensitive (err:%d)\n", res);
		}
	}
	else
	{
		// The key is cekey, and the objective is { fek }_cekey
		res = __fscrypt_set_sdp_fek(ci, key,
									ci->ci_sdp_info->sdp_dek.buf,
									ci->ci_sdp_info->sdp_dek.len);
		if (res) {
			DEK_LOGE("sdp_initialize: failed to set fek (err:%d)\n", res);
			goto out;
		}
		ci->ci_sdp_info->sdp_flags &= ~SDP_DEK_IS_UNINITIALIZED;

		sdp_ctx.engine_id = engine_id;
		sdp_ctx.sdp_dek_type = ci->ci_sdp_info->sdp_dek.type;
		sdp_ctx.sdp_dek_len = ci->ci_sdp_info->sdp_dek.len;
		memcpy(sdp_ctx.sdp_dek_buf, ci->ci_sdp_info->sdp_dek.buf, DEK_MAXLEN); //Full copy without memset
//		memset(sdp_ctx.sdp_en_buf, 0, MAX_EN_BUF_LEN); // Keep it as dummy

		/* Update SDP Context */
		res = fscrypt_sdp_set_context(inode, &sdp_ctx, sizeof(sdp_ctx));
		if (res) {
			DEK_LOGE("sdp_initialize: failed to set sdp context (err:%d)\n", res);
			goto out;
		}

		/* Update FS Context */
		ctx.knox_flags &= ~SDP_DEK_IS_UNINITIALIZED;
		inode_lock(inode);
		res = inode->i_sb->s_cop->set_context(inode, &ctx, sizeof(ctx), NULL);
		inode_unlock(inode);
		if (res) {
			DEK_LOGE("sdp_initialize: failed to set fscrypt ctx (err:%d)\n", res);
			goto out;
		}
		ci->ci_sdp_info->sdp_flags = ctx.knox_flags;

		DEK_LOGD("sdp_initialize: Success\n");
	}

out:
	return res;
}

int fscrypt_sdp_add_chamber_directory(int engine_id, struct inode *inode)
{
	struct fscrypt_info *ci = inode->i_crypt_info;
	struct fscrypt_sdp_context sdp_ctx;
	struct fscrypt_context ctx;
	int rc = 0;

	rc = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx));
	if (rc < 0) {
		DEK_LOGE(KERN_ERR
			   "%s: Failed to get fscrypt ctx (err:%d)\n", __func__, rc);
		return rc;
	}

	if (!ci->ci_sdp_info) {
		struct sdp_info *ci_sdp_info = fscrypt_sdp_alloc_sdp_info();
		if (!ci_sdp_info) {
			return -ENOMEM;
		}

		if (cmpxchg(&ci->ci_sdp_info, NULL, ci_sdp_info) != NULL) {
			DEK_LOGD("Need to put info\n");
			fscrypt_sdp_put_sdp_info(ci_sdp_info);
			return -EPERM;
		}
	}

	ci->ci_sdp_info->sdp_flags |= SDP_IS_CHAMBER_DIR;

	if (!(ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE)) {
		ci->ci_sdp_info->sdp_flags |= SDP_DEK_IS_SENSITIVE;
		ci->ci_sdp_info->engine_id = engine_id;
		sdp_ctx.sdp_dek_type = DEK_TYPE_PLAIN;
		sdp_ctx.sdp_dek_len = DEK_MAXLEN;
		memset(sdp_ctx.sdp_dek_buf, 0, DEK_MAXLEN);
	} else {
		sdp_ctx.sdp_dek_type = ci->ci_sdp_info->sdp_dek.type;
		sdp_ctx.sdp_dek_len = ci->ci_sdp_info->sdp_dek.len;
		memcpy(sdp_ctx.sdp_dek_buf, ci->ci_sdp_info->sdp_dek.buf, ci->ci_sdp_info->sdp_dek.len);
	}
	sdp_ctx.engine_id = ci->ci_sdp_info->engine_id;

	rc = fscrypt_sdp_set_context(inode, &sdp_ctx, sizeof(sdp_ctx));

	if (rc) {
		DEK_LOGE("%s: Failed to add chamber dir.. (err:%d)\n", __func__, rc);
		return rc;
	}

	ctx.knox_flags = ci->ci_sdp_info->sdp_flags | FSCRYPT_SDP_PARSE_FLAG_OUT_OF_SDP(ctx.knox_flags);
	inode_lock(inode);
	rc = inode->i_sb->s_cop->set_context(inode, &ctx, sizeof(ctx), NULL);
	inode_unlock(inode);
	if (rc) {
		DEK_LOGE("%s: Failed to set ext4 context for sdp (err:%d)\n", __func__, rc);
	}

	return rc;
}

int fscrypt_sdp_remove_chamber_directory(struct inode *inode)
{
	struct fscrypt_info *ci = inode->i_crypt_info;
	struct fscrypt_sdp_context sdp_ctx;
	struct fscrypt_context ctx;
	int rc = 0;

	rc = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx));
	if (rc < 0) {
		DEK_LOGE(KERN_ERR
			   "%s: Failed to get fscrypt ctx (err:%d)\n", __func__, rc);
		return rc;
	}

	if (!ci->ci_sdp_info)
		return -EINVAL;

	ci->ci_sdp_info->sdp_flags = 0;

	sdp_ctx.engine_id = ci->ci_sdp_info->engine_id;
	sdp_ctx.sdp_dek_type = ci->ci_sdp_info->sdp_dek.type;
	sdp_ctx.sdp_dek_len = ci->ci_sdp_info->sdp_dek.len;
	memset(sdp_ctx.sdp_dek_buf, 0, DEK_MAXLEN);

	rc = fscrypt_sdp_set_context(inode, &sdp_ctx, sizeof(sdp_ctx));

	if (rc) {
		DEK_LOGE("%s: Failed to remove chamber dir.. (err:%d)\n", __func__, rc);
		return rc;
	}

	ctx.knox_flags = FSCRYPT_SDP_PARSE_FLAG_OUT_OF_SDP(ctx.knox_flags);
	inode_lock(inode);
	rc = inode->i_sb->s_cop->set_context(inode, &ctx, sizeof(ctx), NULL);
	inode_unlock(inode);
	if (rc) {
		DEK_LOGE("%s: Failed to set ext4 context for sdp (err:%d)\n", __func__, rc);
	}

	return rc;
}

static inline int __fscrypt_derive_nek_iv(u8 *drv_key, u32 drv_key_len, u8 *out, u32 out_len)
{
	int err;
#ifdef CONFIG_CRYPTO_KBKDF_CTR_HMAC_SHA512
	u32 L = SDP_DERIVED_KEY_OUTPUT_SIZE;

	if (drv_key == NULL
			|| out == NULL || out_len < SDP_DERIVED_KEY_OUTPUT_SIZE)
		return -EINVAL;

	err = crypto_calc_kdf_hmac_sha512_ctr(KDF_DEFAULT, KDF_RLEN_08BIT,
					drv_key, drv_key_len,
					out, &L,
					SDP_CRYPTO_NEK_DRV_LABEL, strlen(SDP_CRYPTO_NEK_DRV_LABEL),
					SDP_CRYPTO_NEK_DRV_CONTEXT, strlen(SDP_CRYPTO_NEK_DRV_CONTEXT));
	if (err) {
		printk(KERN_ERR
			"derive_nek_iv: failed in crypto_calc_kdf_hmac_sha512_ctr (err:%d)\n", err);
	}
#else
	if (drv_key == NULL
			|| out == NULL || out_len < SDP_DERIVED_KEY_OUTPUT_SIZE)
		return -EINVAL;

	err = sdp_crypto_hash_sha512(drv_key, drv_key_len, out);
	if (err) {
		printk(KERN_ERR
			"derive_nek_iv: failed in sdp_crypto_hash_sha512 (err:%d)\n", err);
	}
#endif
	return err;
}

inline int __fscrypt_get_sdp_dek(struct fscrypt_info *crypt_info,
						unsigned char *dek, unsigned int dek_len)
{
	int res = 0;
	dek_t *__dek;

	__dek = kmalloc(sizeof(dek_t), GFP_NOFS);
	if (!__dek)
		return -ENOMEM;

	res = dek_decrypt_dek_efs(crypt_info->ci_sdp_info->engine_id, &(crypt_info->ci_sdp_info->sdp_dek), __dek);
	if (res < 0) {
		res = -ENOKEY;
		goto out;
	}

	if (__dek->len > dek_len) {
		res = -EINVAL;
		goto out;
	}
	memcpy(dek, __dek->buf, __dek->len);

out:
	memset(__dek->buf, 0, DEK_MAXLEN);
	kzfree(__dek);
	return res;
}

inline int __fscrypt_get_sdp_uninitialized_dek(struct fscrypt_info *crypt_info,
						unsigned char *dek, unsigned int dek_len)
{
	dek_t *__dek;

	__dek = &crypt_info->ci_sdp_info->sdp_dek;
	if (!__dek)
		return -ENOKEY;

	if (__dek->len > dek_len)
		return -EINVAL;

	memcpy(dek, __dek->buf, dek_len);

	return 0;
}

int fscrypt_sdp_store_fek(struct inode *inode,
						struct fscrypt_info *crypt_info,
						unsigned char *fek, unsigned int fek_len)
{
	int res;
	struct fscrypt_key master_key;
	res = fscrypt_get_encryption_kek(inode, crypt_info, &master_key);
	if (unlikely(res))
		goto out;

	res = __fscrypt_set_sdp_fek(crypt_info, &master_key, fek, fek_len);
	if (unlikely(res))
		DEK_LOGE("store_fek: failed. (err:%d)\n", res);

	memzero_explicit(master_key.raw, master_key.size);
out:
	return res;
}

inline int __fscrypt_set_sdp_fek(struct fscrypt_info *crypt_info,
									struct fscrypt_key *master_key,
									unsigned char *fek, unsigned int fek_len)
{
	int res;
	dek_t *__fek;
	dek_t *__efek;

	if (unlikely(fek_len > DEK_MAXLEN))
		return -EINVAL;

	if (master_key->size < SDP_CRYPTO_GCM_DEFAULT_KEY_LEN)
		return -EINVAL;

	__fek = kmalloc(sizeof(dek_t), GFP_NOFS);
	if (!__fek)
		return -ENOMEM;

	__efek = &crypt_info->ci_sdp_info->sdp_dek;

	__fek->type = DEK_TYPE_PLAIN;
	__fek->len = fek_len;
	memset(__fek->buf, 0, DEK_MAXLEN);
	memcpy(__fek->buf, fek, fek_len);

	res = dek_encrypt_fek(master_key->raw, SDP_CRYPTO_GCM_DEFAULT_KEY_LEN,
						__fek->buf, __fek->len,
						__efek->buf, &__efek->len);
	if (likely(!res))
		__efek->type = DEK_TYPE_AES_ENC;

	memzero_explicit(__fek->buf, __fek->len);
	kzfree(__fek);
	return res;
}

inline int __fscrypt_get_sdp_fek(struct fscrypt_info *crypt_info,
									struct fscrypt_key *master_key,
									unsigned char *fek, unsigned int *fek_len)
{
	int res;
	dek_t *__efek;

	if (master_key->size < SDP_CRYPTO_GCM_DEFAULT_KEY_LEN)
		return -EINVAL;

	__efek = &crypt_info->ci_sdp_info->sdp_dek;
	if (!__efek)
		return -EFAULT;

	res = dek_decrypt_fek(master_key->raw, SDP_CRYPTO_GCM_DEFAULT_KEY_LEN,
						__efek->buf, __efek->len,
						fek, fek_len);
	return res;
}

inline int __fscrypt_set_sdp_dek(struct fscrypt_info *crypt_info,
		unsigned char *dek, int dek_len)
{
	int res;
	dek_t *__dek;

	if (unlikely(dek_len > DEK_MAXLEN))
		return -EINVAL;

	__dek = kmalloc(sizeof(dek_t), GFP_NOFS);
	if (!__dek) {
		return -ENOMEM;
	}

	__dek->type = DEK_TYPE_PLAIN;
	__dek->len = dek_len;
	memset(__dek->buf, 0, DEK_MAXLEN);
	memcpy(__dek->buf, dek, dek_len);

	res = dek_encrypt_dek_efs(crypt_info->ci_sdp_info->engine_id, __dek, &crypt_info->ci_sdp_info->sdp_dek);

	memset(__dek->buf, 0, DEK_MAXLEN);
	kzfree(__dek);
	return res;
}

inline int __fscrypt_get_nonce(u8 *key, u32 key_len,
						u8 *enonce, u32 nonce_len,
						u8 *out, u32 out_len)
{
	u8 *nek;
	u8 drv_buf[SDP_DERIVED_KEY_OUTPUT_SIZE];
	u32 drv_buf_len = SDP_DERIVED_KEY_OUTPUT_SIZE;
	u32 nek_len = SDP_CRYPTO_NEK_LEN;
	int rc;
	struct crypto_aead *tfm;
	gcm_pack32 pack;
	gcm_pack __pack;
	size_t pack_siz = sizeof(pack);

	if (out == NULL || out_len < nonce_len)
		return -EINVAL;

	rc = __fscrypt_derive_nek_iv(key, key_len, drv_buf, drv_buf_len);
	if (rc)
		goto out;

	memcpy(&pack, enonce, pack_siz);
	nek = (u8 *)drv_buf;
#if DEK_DEBUG
	hex_key_dump("get_nonce: gcm pack", (uint8_t *)&pack, pack_siz);
	hex_key_dump("get_nonce: nek_iv", nek, SDP_DERIVED_KEY_OUTPUT_SIZE);
#endif
	__pack.type = SDP_CRYPTO_GCM_PACK32;
	__pack.iv = pack.iv;
	__pack.data = pack.data;
	__pack.auth = pack.auth;

	tfm = sdp_crypto_aes_gcm_key_setup(nek, nek_len);
	if (IS_ERR(tfm)) {
		rc = PTR_ERR(tfm);
		goto out;
	}

	rc = sdp_crypto_aes_gcm_decrypt_pack(tfm, &__pack);
	if (!rc) {
		memcpy(out, pack.data, nonce_len);
#if DEK_DEBUG
		hex_key_dump("get_nonce: pack", (u8 *)&pack, pack_siz);
#endif
	}

	crypto_free_aead(tfm);

out:
	memzero_explicit(&pack, pack_siz);
	memzero_explicit(drv_buf, SDP_DERIVED_KEY_OUTPUT_SIZE);
	return rc;
}

static inline int __fscrypt_set_nonce(u8 *key, u32 key_len,
						u8 *nonce, u32 nonce_len,
						u8 *out, u32 out_len)
{
	u8 *iv;
	u8 *nek;
	u8 drv_buf[SDP_DERIVED_KEY_OUTPUT_SIZE];
	u32 drv_buf_len = SDP_DERIVED_KEY_OUTPUT_SIZE;
	u32 nek_len = SDP_CRYPTO_NEK_LEN;
	int rc;
	struct crypto_aead *tfm;
	gcm_pack32 pack;
	gcm_pack __pack;
	size_t pack_siz = sizeof(pack);

	if (out == NULL || out_len < pack_siz)
		return -EINVAL;

	rc = __fscrypt_derive_nek_iv(key, key_len, drv_buf, drv_buf_len);
	if (rc)
		goto out;

	memset(&pack, 0, pack_siz);

	nek = (u8 *)drv_buf;
	iv = (u8 *)nek + nek_len;
#if DEK_DEBUG
	hex_key_dump("set_nonce: nonce", nonce, nonce_len);
	hex_key_dump("set_nonce: nek_iv", nek, SDP_DERIVED_KEY_OUTPUT_SIZE);
#endif
	memcpy(pack.iv, iv, SDP_CRYPTO_GCM_IV_LEN);
	memcpy(pack.data, nonce, nonce_len);
	__pack.type = SDP_CRYPTO_GCM_PACK32;
	__pack.iv = pack.iv;
	__pack.data = pack.data;
	__pack.auth = pack.auth;


	tfm = sdp_crypto_aes_gcm_key_setup(nek, nek_len);
	if (IS_ERR(tfm)) {
		rc = PTR_ERR(tfm);
		goto out;
	}

	rc = sdp_crypto_aes_gcm_encrypt_pack(tfm, &__pack);
	if (!rc) {
		memcpy(out, &pack, pack_siz);
#if DEK_DEBUG
		hex_key_dump("set_nonce: pack", (u8 *)&pack, pack_siz);
#endif
	}

	crypto_free_aead(tfm);

out:
	memzero_explicit(&pack, pack_siz);
	memzero_explicit(drv_buf, SDP_DERIVED_KEY_OUTPUT_SIZE);
	return rc;
}

inline int __fscrypt_get_sdp_context(struct inode *inode, struct fscrypt_info *crypt_info)
{
	int res = 0;
	struct fscrypt_sdp_context sdp_ctx;

	res = fscrypt_sdp_get_context(inode, &sdp_ctx, sizeof(sdp_ctx));

	if (res == sizeof(sdp_ctx)) {
		crypt_info->ci_sdp_info->engine_id = sdp_ctx.engine_id;
		crypt_info->ci_sdp_info->sdp_dek.type = sdp_ctx.sdp_dek_type;
		crypt_info->ci_sdp_info->sdp_dek.len = sdp_ctx.sdp_dek_len;
		DEK_LOGD("sdp_flags = 0x%08x, engine_id = %d\n", crypt_info->ci_sdp_info->sdp_flags, sdp_ctx.engine_id);
		memcpy(crypt_info->ci_sdp_info->sdp_dek.buf, sdp_ctx.sdp_dek_buf,
				sizeof(crypt_info->ci_sdp_info->sdp_dek.buf));

		if (S_ISDIR(inode->i_mode))
			crypt_info->ci_sdp_info->sdp_flags |= SDP_IS_DIRECTORY;

		res = 0;
	} else {
		res = -EINVAL;
	}

	return res;
}

static inline void __fscrypt_sdp_set_inode_sensitive(struct inode *inode)
{
	fscrypt_sdp_cache_add_inode_num(inode);
	mapping_set_sensitive(inode->i_mapping);
}

inline int __fscrypt_sdp_finish_set_sensitive(struct inode *inode,
				struct fscrypt_context *ctx, struct fscrypt_info *crypt_info,
				struct fscrypt_key *key) {
	int res = 0;
	int is_native = 0;
	struct fscrypt_sdp_context sdp_ctx;
	struct fscrypt_key fek;
	u8 enonce[MAX_EN_BUF_LEN];

	if ((crypt_info->ci_sdp_info->sdp_flags & SDP_DEK_TO_SET_SENSITIVE)
			|| (crypt_info->ci_sdp_info->sdp_flags & SDP_DEK_TO_CONVERT_KEY_TYPE)) {
		DEK_LOGD("sensitive SDP_DEK_TO_SET_SENSITIVE\n");
		//It's a new sensitive file, let's make sdp dek!

		is_native = fscrypt_sdp_is_native(crypt_info);
		if (key) {
			DEK_LOGD("set_sensitive: fek is already given!\n");
			memcpy(&fek, key, sizeof(fek));
		} else {
			memset(&fek, 0, sizeof(fek));
			if (is_native) {
				res = fscrypt_sdp_derive_fekey(inode, crypt_info, &fek);
			} else {
				res = fscrypt_get_encryption_key(inode, &fek);
			}
			if (res) {
				DEK_LOGE("set_sensitive: failed to find fek (err:%d)\n", res);
				return res;
			}
		}
#if DEK_DEBUG
		hex_key_dump("set_sensitive: fek", fek.raw, fek.size);
#endif
		res = __fscrypt_set_nonce(fek.raw, fek.size,
								ctx->nonce, FS_KEY_DERIVATION_NONCE_SIZE,
								enonce, MAX_EN_BUF_LEN);
		if (res) {
			DEK_LOGE("set_sensitive: failed to encrypt nonce (err:%d)\n", res);
			goto out;
		}
#if DEK_DEBUG
		hex_key_dump("set_sensitive: enonce", enonce, MAX_EN_BUF_LEN);
#endif
		res = __fscrypt_set_sdp_dek(crypt_info, fek.raw, fek.size);
		if (res) {
				DEK_LOGE("set_sensitive: failed to encrypt dek (err:%d)\n", res);
			goto out;
		}

		crypt_info->ci_sdp_info->sdp_flags &= ~(SDP_DEK_TO_SET_SENSITIVE);
		crypt_info->ci_sdp_info->sdp_flags &= ~(SDP_DEK_TO_CONVERT_KEY_TYPE);
		crypt_info->ci_sdp_info->sdp_flags |= SDP_DEK_IS_SENSITIVE;
		sdp_ctx.engine_id = crypt_info->ci_sdp_info->engine_id;
		sdp_ctx.sdp_dek_type = crypt_info->ci_sdp_info->sdp_dek.type;
		sdp_ctx.sdp_dek_len = crypt_info->ci_sdp_info->sdp_dek.len;

		/* Update EFEK */
		memcpy(sdp_ctx.sdp_dek_buf, crypt_info->ci_sdp_info->sdp_dek.buf, DEK_MAXLEN);

		/* Update EN */
		memcpy(sdp_ctx.sdp_en_buf, enonce, MAX_EN_BUF_LEN);

		/* Update SDP Context */
		res = fscrypt_sdp_set_context(inode, &sdp_ctx, sizeof(sdp_ctx));
		if (res) {
			DEK_LOGE("set_sensitive: failed to set sdp context (err:%d)\n", res);
			goto out;
		}

		/* Update FS Context */
		ctx->knox_flags = (FSCRYPT_SDP_PARSE_FLAG_OUT_OF_SDP(ctx->knox_flags) | SDP_DEK_IS_SENSITIVE);
		if (is_native) {
			ctx->knox_flags |= SDP_DEK_SDP_ENABLED;
		}
		memzero_explicit(ctx->nonce, FS_KEY_DERIVATION_NONCE_SIZE);
		inode_lock(inode);
		res = inode->i_sb->s_cop->set_context(inode, ctx, sizeof(*ctx), NULL);
		inode_unlock(inode);
		if (res) {
			DEK_LOGE("set_sensitive: failed to set fscrypt context(err:%d)\n", res);
			goto out;
		}
		DEK_LOGD("sensitive SDP_DEK_TO_SET_SENSITIVE finished!!\n");
	}

	if ((crypt_info->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE) &&
			!(crypt_info->ci_sdp_info->sdp_flags & SDP_IS_DIRECTORY)) {
		__fscrypt_sdp_set_inode_sensitive(inode);
	}

out:
	memzero_explicit(&fek, sizeof(fek));
	return res;
}

int fscrypt_sdp_get_engine_id(struct inode *inode)
{
	struct fscrypt_info *crypt_info;

	crypt_info = inode->i_crypt_info;
	if (!crypt_info || !crypt_info->ci_sdp_info)
		return -1;

	return crypt_info->ci_sdp_info->engine_id;
}

typedef enum sdp_thread_type {
	SDP_THREAD_SET_SENSITIVE,
	SDP_THREAD_KEY_CONVERT,
	SDP_THREAD_INITIALIZE
} sdp_thread_type_t;

// Should be called by only fscrypt_sdp_run_thread()
inline int __fscrypt_sdp_thread_initialize(void *arg)
{
	sdp_ess_material *sem = (sdp_ess_material *)arg;
	struct inode *inode;
	struct fscrypt_info *ci;
	struct fscrypt_key *key;

	if (unlikely(!sem))
		return -EINVAL;
	if (sem->inode) {
		inode = sem->inode;
		key = &sem->key;
		ci = inode->i_crypt_info;

		if (ci && ci->ci_sdp_info) {
			fscrypt_sdp_initialize(inode, ci->ci_sdp_info->engine_id, key);
		}

		if (key)
			memzero_explicit(key, sizeof(*key));
		iput(inode);
	}
	kzfree(sem);
	return 0;
}

// Should be called by only fscrypt_sdp_run_thread()
inline int __fscrypt_sdp_thread_set_sensitive(void *arg)
{
	sdp_ess_material *sem = (sdp_ess_material *)arg;
	struct inode *inode;
	struct fscrypt_info *ci;
	struct fscrypt_key *key;

	if (unlikely(!sem))
		return -EINVAL;
	if (sem->inode) {
		inode = sem->inode;
		key = &sem->key;
		ci = inode->i_crypt_info;

		if (ci && ci->ci_sdp_info) {
			fscrypt_sdp_set_sensitive(inode, ci->ci_sdp_info->engine_id, key);
		}

		if (key)
			memzero_explicit(key, sizeof(*key));
		iput(inode);
	}
	kzfree(sem);
	return 0;
}

// Should be called by only fscrypt_sdp_run_thread()
inline int __fscrypt_sdp_thread_convert_sdp_key(void *arg)
{
	sdp_ess_material *sem = (sdp_ess_material *)arg;
	struct inode *inode;
	struct fscrypt_info *ci;
	struct fscrypt_context ctx;
	struct fscrypt_sdp_context sdp_ctx;
	struct fscrypt_key *fek = NULL;
	int rc = 0;

	if (unlikely(!sem))
		return -EINVAL;
	if (sem->inode) {
		inode = sem->inode;
		fek = &sem->key;
		ci = inode->i_crypt_info;

		if (ci && ci->ci_sdp_info) {
			rc = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx));
			if (rc != sizeof(ctx)) {
				if (rc > 0 )
					rc = -EINVAL;
				DEK_LOGE("convert_key: failed to get fscrypt ctx (err:%d)\n", rc);
				goto out;
			}

			if (!fek) {
				rc = -ENOKEY;
				DEK_LOGE("convert_key: failed to find fek (err:%d)\n", rc);
				goto out;
			}

			rc = fscrypt_sdp_get_context(inode, &sdp_ctx, sizeof(sdp_ctx));
			if (rc != sizeof(sdp_ctx)) {
				if (rc >= 0 )
					rc = -EINVAL;
				DEK_LOGE("convert_key: failed to get sdp context (err:%d)\n", rc);
				goto out;
			}
#if DEK_DEBUG
			hex_key_dump("convert_key: fek", fek->raw, fek->size);
#endif
			rc = __fscrypt_get_nonce(fek->raw, fek->size,
									sdp_ctx.sdp_en_buf, FS_KEY_DERIVATION_NONCE_SIZE,
									ctx.nonce, FS_KEY_DERIVATION_NONCE_SIZE);
			if (rc) {
				DEK_LOGE("convert_key: failed to get nonce (err:%d)\n", rc);
				goto out;
			}

			__fscrypt_sdp_finish_set_sensitive(inode, &ctx, ci, fek);
		}
out:
		if (fek)
			memzero_explicit(fek, sizeof(*fek));
		iput(inode);
	}
	kzfree(sem);
	return 0;
}

inline static const char *__thread_type_to_name(sdp_thread_type_t thread_type)
{
	switch (thread_type) {
		case SDP_THREAD_INITIALIZE:
			return "__fscrypt_sdp_thread_initialize";
		case SDP_THREAD_SET_SENSITIVE:
			return "__fscrypt_sdp_thread_set_sensitive";
		case SDP_THREAD_KEY_CONVERT:
			return "__fscrypt_sdp_thread_convert_sdp_key";
		default:
			return "__fscrypt_sdp_thread_unknown";
	}
}

inline static int (*__thread_type_to_func(sdp_thread_type_t thread_type))(void*)
{
	switch (thread_type) {
		case SDP_THREAD_INITIALIZE:
			return __fscrypt_sdp_thread_initialize;
		case SDP_THREAD_SET_SENSITIVE:
			return __fscrypt_sdp_thread_set_sensitive;
		case SDP_THREAD_KEY_CONVERT:
			return __fscrypt_sdp_thread_convert_sdp_key;
		default:
			return NULL;
	}
}

/*
 * This function is to build up the sdp essential material to perform
 * sdp operations, and ship the material into the sem pointer taken over.
 *
 * The data shall be raw fek, and the data length shall be the key size.
 */
inline int fscrypt_sdp_build_sem(struct inode *inode,
									void *data, unsigned int data_len,
									sdp_thread_type_t thread_type,
									sdp_ess_material **sem)
{
	int res = 0;
	struct fscrypt_info *ci = inode->i_crypt_info; // Will never be null.
	sdp_ess_material *__sem;

	if (unlikely(
			!data || data_len > FS_MAX_KEY_SIZE))
		return -EINVAL;

	__sem = *sem = kmalloc(sizeof(sdp_ess_material), GFP_ATOMIC);
	if (unlikely(!__sem))
		return -ENOMEM;

	if (thread_type == SDP_THREAD_INITIALIZE
			&& !fscrypt_sdp_is_to_sensitive(ci)) {
		// To initialize non-sensitive file, cekey is required.
		__sem->inode = inode;
		res = fscrypt_get_encryption_kek(__sem->inode, ci, &__sem->key);
		if (unlikely(res)) {
			DEK_LOGD("sdp_build_sem: failed to get kek (err:%d)\n", res);
		}
	}
	else {
		__sem->inode = inode;
		__sem->key.mode = 0;
		__sem->key.size = data_len;
		memcpy(__sem->key.raw, data, data_len);
	}
	return res;
}

/*
 * The function is supposed to manage the sem taken over.
 */
inline int fscrypt_sdp_run_thread_if_required(struct inode *inode,
								sdp_thread_type_t thread_type,
								sdp_ess_material *sem)
{
	int res = -1;
	int is_required = 1;
	int (*task_func)(void *data);
	struct task_struct *task = NULL;
	sdp_ess_material *__sem = sem;

	if (unlikely(!__sem))
		return -EINVAL;

	/*
	 * If the inode is on ext4 and the current has a journal_info,
	 * then it is deemed as being in write context.
	 */
	if (inode->i_sb->s_magic == EXT4_SUPER_MAGIC &&
			current->journal_info) {
		is_required = 0;
	}

	task_func = __thread_type_to_func(thread_type);
	if (unlikely(!task_func)) {
		DEK_LOGD("sdp_run_thread: failed due to invalid task function - %s\n",
				__thread_type_to_name(thread_type));
		goto out;
	}

	if (is_required) {
		if (igrab(inode)) {
			task = kthread_run(task_func, (void *)__sem, __thread_type_to_name(thread_type));
		}
		if(IS_ERR_OR_NULL(task)) {
			if (IS_ERR(task)) {
				res = PTR_ERR(task);
				memzero_explicit(__sem, sizeof(sdp_ess_material));
				iput(inode);
			}
		} else {
			DEK_LOGD("sdp_run_thread: left behind - %s\n",
					__thread_type_to_name(thread_type));
			res = 0;
		}
	} else {
		if (igrab(inode)) {
			res = task_func((void *)__sem);
		}
		DEK_LOGD("sdp_run_thread: finished task - %s = %d\n",
				__thread_type_to_name(thread_type), res);
	}

out:
	if (res)
		kzfree(__sem);
	return res;
}

inline int fscrypt_sdp_run_thread(struct inode *inode,
								void *data, unsigned int data_len,
								sdp_thread_type_t thread_type)
{
	int res = -1;
	struct task_struct *task = NULL;
	struct fscrypt_info *ci = NULL;
	sdp_ess_material *__sem = NULL;

	if (unlikely(
			!data || data_len > FS_MAX_KEY_SIZE))
		return -EINVAL;

	__sem = kmalloc(sizeof(sdp_ess_material), GFP_ATOMIC);
	if (unlikely(!__sem))
		return -ENOMEM;

	if (thread_type == SDP_THREAD_INITIALIZE) {
		if (igrab(inode)) {
			 // i_crypt_info must be set at fscrypt_get_encryption_info */
			ci = inode->i_crypt_info;

			if (fscrypt_sdp_is_to_sensitive(ci)) {
				__sem->inode = inode;
				__sem->key.mode = 0;
				__sem->key.size = data_len;
				memcpy(__sem->key.raw, data, data_len);
			} else {
				__sem->inode = inode;
				res = fscrypt_get_encryption_kek(__sem->inode, ci, &__sem->key);
				if (unlikely(res)) {
					DEK_LOGD("sdp_run_thread: failed to get kek (err:%d)\n", res);
					task = ERR_PTR(res);
					goto out;
				}
			}
			task = kthread_run(__fscrypt_sdp_thread_initialize, (void *)__sem, __thread_type_to_name(thread_type));
		}
	} else if (thread_type == SDP_THREAD_SET_SENSITIVE) {
		if (igrab(inode)) {
			__sem->inode = inode;
			__sem->key.mode = 0;
			__sem->key.size = data_len;
			memcpy(__sem->key.raw, data, data_len);
			task = kthread_run(__fscrypt_sdp_thread_set_sensitive, (void *)__sem, __thread_type_to_name(thread_type));
		}
	}
	else if (thread_type == SDP_THREAD_KEY_CONVERT) {
		if (igrab(inode)) {
			__sem->inode = inode;
			__sem->key.mode = 0;
			__sem->key.size = data_len;
			memcpy(__sem->key.raw, data, data_len);
			task = kthread_run(__fscrypt_sdp_thread_convert_sdp_key, (void *)__sem, __thread_type_to_name(thread_type));
		}
	}

out:
	if(IS_ERR_OR_NULL(task)) {
		if (IS_ERR(task)) {
			res = PTR_ERR(task);
			memzero_explicit(__sem, sizeof(sdp_ess_material));
			iput(inode);
		}
		// The reason shall be printed at outer function.
		// DEK_LOGE("sdp_run_thread: failed to create kernel thread (err:%d)\n", res);
		kzfree(__sem);
	} else {
		res = 0;
	}
	return res;
}

int fscrypt_sdp_update_sdp_info(struct inode *inode,
						const struct fscrypt_context *ctx,
						struct fscrypt_info *crypt_info)
{
	crypt_info->ci_sdp_info->sdp_flags =
						FSCRYPT_SDP_PARSE_FLAG_SDP_ONLY(ctx->knox_flags);
	return __fscrypt_get_sdp_context(inode, crypt_info);
}

int fscrypt_sdp_is_classified(struct fscrypt_info *crypt_info)
{
	return !(crypt_info->ci_sdp_info->sdp_flags & SDP_IS_DIRECTORY)
			&& ((crypt_info->ci_sdp_info->sdp_flags & SDP_DEK_SDP_ENABLED)
				|| (crypt_info->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE));
}

int fscrypt_sdp_is_uninitialized(struct fscrypt_info *crypt_info)
{
	return (crypt_info->ci_sdp_info->sdp_flags & SDP_DEK_IS_UNINITIALIZED);
}

int fscrypt_sdp_is_native(struct fscrypt_info *crypt_info)
{
	return (crypt_info->ci_sdp_info->sdp_flags & SDP_DEK_SDP_ENABLED);
}

int fscrypt_sdp_is_sensitive(struct fscrypt_info *crypt_info)
{
	return (crypt_info->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE);
}

int fscrypt_sdp_is_to_sensitive(struct fscrypt_info *crypt_info)
{
	return (crypt_info->ci_sdp_info->sdp_flags & SDP_DEK_TO_SET_SENSITIVE);
}

void fscrypt_sdp_update_conv_status(struct fscrypt_info *crypt_info)
{
	if ((crypt_info->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE)
			&& (crypt_info->ci_sdp_info->sdp_dek.type != DEK_TYPE_AES_ENC)) {
		crypt_info->ci_sdp_info->sdp_flags |= SDP_DEK_TO_CONVERT_KEY_TYPE;
		DEK_LOGD("Need conversion!\n");
	}
	return;
}

int fscrypt_sdp_derive_dek(struct fscrypt_info *crypt_info,
						unsigned char *derived_key,
						unsigned int derived_keysize)
{
	int res;

	res = __fscrypt_get_sdp_dek(crypt_info, derived_key, derived_keysize);
	if (unlikely(res)) {
		DEK_LOGE("derive_dek: failed to get dek (err:%d)\n", res);
	}
	return res;
}

int fscrypt_sdp_derive_uninitialized_dek(struct fscrypt_info *crypt_info,
						unsigned char *derived_key,
						unsigned int derived_keysize)
{
	int res;

	res = __fscrypt_get_sdp_uninitialized_dek(crypt_info, derived_key, derived_keysize);
	if (unlikely(res)) {
		DEK_LOGE("derive_uninit_dek: failed to get dek (err:%d)\n", res);
	}
	return res;
}

int fscrypt_sdp_derive_fekey(struct inode *inode,
						struct fscrypt_info *crypt_info,
						struct fscrypt_key *fek)
{
	int res;
	struct fscrypt_key master_key;
	res = fscrypt_get_encryption_kek(inode, crypt_info, &master_key);
	if (unlikely(res))
		goto out;

	res = __fscrypt_get_sdp_fek(crypt_info, &master_key, fek->raw, &fek->size);

	memzero_explicit(master_key.raw, master_key.size);
out:
	if (unlikely(res))
		DEK_LOGE("derive_fekey: failed. (err:%d)\n", res);
	return res;
}

int fscrypt_sdp_derive_fek(struct inode *inode,
						struct fscrypt_info *crypt_info,
						unsigned char *fek, unsigned int fek_len)
{
	int res;
	unsigned int __fek_len = 0;
	struct fscrypt_key master_key;
	res = fscrypt_get_encryption_kek(inode, crypt_info, &master_key);
	if (unlikely(res))
		goto out;

	res = __fscrypt_get_sdp_fek(crypt_info, &master_key, fek, &__fek_len);
	if (unlikely(res))
		DEK_LOGE("derive_fek: failed. (err:%d)\n", res);
//	else if (fek_len != __fek_len) {
//		DEK_LOGE("derive_fek: key length not matched. (%d:%d)\n", fek_len, __fek_len);
//		res = -EFAULT;
//		memzero_explicit(fek, fek_len);
//	}
	memzero_explicit(master_key.raw, master_key.size);
out:
	return res;
}

int fscrypt_sdp_inherit_context(struct inode *parent, struct inode *child, struct fscrypt_context *ctx)
{
	int res = 0;
	int is_sdp_ctx_updated;
	int is_parent_native;
	int is_parent_sensitive;

	struct fscrypt_info *ci = parent->i_crypt_info;
	struct fscrypt_sdp_context sdp_ctx;

	if (!ci || !ci->ci_sdp_info)
		return res;

	if (FSCRYPT_SDP_PARSE_FLAG_OUT_OF_SDP(ci->ci_sdp_info->sdp_flags)) {
		DEK_LOGE("sdp_inherit: flags already occupied: 0x%08x\n",
				ci->ci_sdp_info->sdp_flags);
		return res;
	}

	if (unlikely(!S_ISREG(child->i_mode) && !S_ISDIR(child->i_mode)))
		return res;

	is_sdp_ctx_updated = 0;
	is_parent_native = (ci->ci_sdp_info->sdp_flags & SDP_DEK_SDP_ENABLED);
	is_parent_sensitive = (ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE);

	// ctx->knox_flags shall be 0 from scratch
	if (is_parent_native) {
		ctx->knox_flags |= SDP_DEK_SDP_ENABLED;
		if (S_ISREG(child->i_mode))
			ctx->knox_flags |= SDP_DEK_IS_UNINITIALIZED;
		else /* if (S_ISDIR(child->i_mode)) */
			ctx->knox_flags |= SDP_IS_DIRECTORY;

		sdp_ctx.engine_id = ci->ci_sdp_info->engine_id;
		sdp_ctx.sdp_dek_type = DEK_TYPE_PLAIN;
//		sdp_ctx.sdp_dek_len = DEK_MAXLEN;
//		memset(sdp_ctx.sdp_dek_buf, 0, DEK_MAXLEN); // Keep it as dummy
//		memset(sdp_ctx.sdp_en_buf, 0, MAX_EN_BUF_LEN); // Keep it as dummy
		if (S_ISREG(child->i_mode)) {
			memset(sdp_ctx.sdp_dek_buf, 0, DEK_MAXLEN);
			res = sdp_crypto_generate_key(sdp_ctx.sdp_dek_buf, FS_MAX_KEY_SIZE);
			if (unlikely(res))
				DEK_LOGE("sdp_inherit: failed to generate dek (err:%d)\n", res);
			res = 0;
			sdp_ctx.sdp_dek_len = FS_MAX_KEY_SIZE;
		} else {
			sdp_ctx.sdp_dek_len = 0;
		}
		is_sdp_ctx_updated++;
	}

	if (is_parent_sensitive) {
		DEK_LOGD("sdp_inherit: "
				"parent->i_crypt_info->sdp_flags=0x%08x, ctx->knox_flags=0x%08x\n",
						ci->ci_sdp_info->sdp_flags, ctx->knox_flags);

		if (S_ISREG(child->i_mode))
			ctx->knox_flags |= SDP_DEK_TO_SET_SENSITIVE;
		else { /* if (S_ISDIR(child->i_mode)) */
			ctx->knox_flags |= SDP_IS_DIRECTORY;
			ctx->knox_flags |= SDP_DEK_IS_SENSITIVE;
		}

		if (!is_parent_native) {
			sdp_ctx.engine_id = ci->ci_sdp_info->engine_id;
			sdp_ctx.sdp_dek_type = DEK_TYPE_PLAIN;
			sdp_ctx.sdp_dek_len = 0;
//			sdp_ctx.sdp_dek_len = DEK_MAXLEN;
//			memset(sdp_ctx.sdp_dek_buf, 0, DEK_MAXLEN); // Keep it as dummy
//			memset(sdp_ctx.sdp_en_buf, 0, MAX_EN_BUF_LEN); // Keep it as dummy
		}
		is_sdp_ctx_updated++;

		DEK_LOGD("sdp_inherit: ctx->knox_flags updated: 0x%08x\n", ctx->knox_flags);
	}

	if (is_sdp_ctx_updated)
		res = fscrypt_sdp_set_context_nolock(child, &sdp_ctx, sizeof(sdp_ctx));
	if (unlikely(res)) {
		DEK_LOGE("sdp_inherit: failed to set sdp context (err:%d)\n", res);
		ctx->knox_flags = 0;
	}
	return res;
}

void fscrypt_sdp_finalize_tasks(struct inode *inode, u8 *raw_key, int key_len)
{
	struct fscrypt_info *ci = inode->i_crypt_info;//This pointer has been loaded by get_encryption_info completely

	if (ci && ci->ci_sdp_info
			&& (ci->ci_sdp_info->sdp_flags & FSCRYPT_KNOX_FLG_SDP_MASK)) {
		__fscrypt_sdp_finalize_tasks(inode, ci, raw_key, key_len);
	}
}

inline void __fscrypt_sdp_finalize_tasks(struct inode *inode,
						struct fscrypt_info *ci, u8 *raw_key, int key_len)
{
	int res = 0;
	sdp_thread_type_t thread_type;
	sdp_ess_material *sem = NULL;

	if (ci->ci_sdp_info->sdp_flags & SDP_IS_DIRECTORY)
		return;

	if (ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE)
		__fscrypt_sdp_set_inode_sensitive(inode);

	if (unlikely(key_len <= 0)) {
		DEK_LOGE("finalize_tasks: invalid key size (maybe previous err:%d)\n", key_len);
		return;
	}

	if (ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_UNINITIALIZED)
	{
		thread_type = SDP_THREAD_INITIALIZE;
		DEK_LOGD("Run initialize task\n");
	}
	else if (ci->ci_sdp_info->sdp_flags & SDP_DEK_TO_SET_SENSITIVE)
	{
		thread_type = SDP_THREAD_SET_SENSITIVE;
		DEK_LOGD("Run set sensitive task\n");
	}
	else if (ci->ci_sdp_info->sdp_flags & SDP_DEK_TO_CONVERT_KEY_TYPE)
	{
		thread_type = SDP_THREAD_KEY_CONVERT;
		DEK_LOGD("Run key convert task\n");
	}
	else
	{
		return;
	}

	res = fscrypt_sdp_build_sem(inode, (void *)raw_key, key_len, thread_type, &sem);
	if (res) {
		DEK_LOGE("finalize_tasks: failed to build up sem (%s, err:%d)\n",
				__thread_type_to_name(thread_type), res);
		kzfree(sem);
		return;
	}

	// From hence, the sem shall be managed/freed by its referrer.
	res = fscrypt_sdp_run_thread_if_required(inode, thread_type, sem);
	if (res) {
		DEK_LOGE("finalize_tasks: failed to run task (%s, err:%d)\n",
				__thread_type_to_name(thread_type), res);
	}
	return;
}

void fscrypt_sdp_put_sdp_info(struct sdp_info *ci_sdp_info)
{
	if (ci_sdp_info) {
		kmem_cache_free(sdp_info_cachep, ci_sdp_info);
	}
}

bool fscrypt_sdp_init_sdp_info_cachep(void)
{
	sdp_info_cachep = KMEM_CACHE(sdp_info, SLAB_RECLAIM_ACCOUNT);
	if (!sdp_info_cachep)
		return false;
	return true;
}

void fscrypt_sdp_release_sdp_info_cachep(void)
{
	kmem_cache_destroy(sdp_info_cachep);
}
