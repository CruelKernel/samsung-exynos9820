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
 
#include <linux/err.h>
#include <sdp/dek_common.h>
#include <sdp/dek_aes.h>
#include <crypto/skcipher.h>
#include "../../fs/crypto/sdp/sdp_crypto.h"

inline int __dek_aes_encrypt_key32(struct crypto_aead *tfm,
					unsigned char *key, unsigned int key_len,
					unsigned char *out);
inline int __dek_aes_encrypt_key64(struct crypto_aead *tfm,
					unsigned char *key, unsigned int key_len,
					unsigned char *out);
inline int __dek_aes_encrypt_key(struct crypto_aead *tfm,
					unsigned char *key, unsigned int key_len,
					gcm_pack *pack);
inline int __dek_aes_decrypt_key32(struct crypto_aead *tfm,
					unsigned char *key, unsigned int key_len,
					unsigned char *out);
inline int __dek_aes_decrypt_key64(struct crypto_aead *tfm,
					unsigned char *key, unsigned int key_len,
					unsigned char *out);
inline int __dek_aes_decrypt_key(struct crypto_aead *tfm,
					gcm_pack *pack);

inline int __dek_aes_encrypt_key_raw(unsigned char *kek, unsigned int kek_len,
									unsigned char *key, unsigned int key_len,
									unsigned char *out, unsigned int *out_len);
inline int __dek_aes_decrypt_key_raw(unsigned char  *kek, unsigned int kek_len,
									unsigned char *ekey, unsigned int ekey_len,
									unsigned char *out, unsigned int *out_len);

static struct crypto_skcipher *dek_aes_key_setup(kek_t *kek)
{
	struct crypto_skcipher *tfm = NULL;

	tfm = crypto_alloc_skcipher("cbc(aes)", 0, CRYPTO_ALG_ASYNC);
	if (!IS_ERR(tfm)) {
		crypto_skcipher_setkey(tfm, kek->buf, kek->len);
	} else {
		printk("dek: failed to alloc blkcipher\n");
		tfm = NULL;
	}
	return tfm;
}

static void dek_aes_key_free(struct crypto_skcipher *tfm)
{
	crypto_free_skcipher(tfm);
}

#define DEK_AES_CBC_IV_SIZE 16

static int __dek_aes_do_crypt(struct crypto_skcipher *tfm,
		unsigned char *src, unsigned char *dst, int len, bool encrypt) {
	int rc = 0;
	struct skcipher_request *req = NULL;
	DECLARE_CRYPTO_WAIT(wait);
	struct scatterlist src_sg, dst_sg;
	u8 iv[DEK_AES_CBC_IV_SIZE] = {0,};

	req = skcipher_request_alloc(tfm, GFP_NOFS);
	if (!req) {
		return -ENOMEM;
	}

	skcipher_request_set_callback(req,
					CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
					crypto_req_done, &wait);

	sg_init_one(&src_sg, src, len);
	sg_init_one(&dst_sg, dst, len);

	skcipher_request_set_crypt(req, &src_sg, &dst_sg, len, iv);

	if (encrypt)
		rc = crypto_wait_req(crypto_skcipher_encrypt(req), &wait);
	else
		rc = crypto_wait_req(crypto_skcipher_decrypt(req), &wait);

	skcipher_request_free(req);
	return rc;
}

int dek_aes_encrypt(kek_t *kek, unsigned char *src, unsigned char *dst, int len) {
	int rc;
	struct crypto_skcipher *tfm;

	if(kek == NULL) return -EINVAL;

	tfm = dek_aes_key_setup(kek);

	if(tfm) {
		rc = __dek_aes_do_crypt(tfm, src, dst, len, true);
		dek_aes_key_free(tfm);
		return rc;
	} else
		return -ENOMEM;
}

int dek_aes_decrypt(kek_t *kek, unsigned char *src, unsigned char *dst, int len) {
	int rc;
	struct crypto_skcipher *tfm;

	if(kek == NULL) return -EINVAL;

	tfm = dek_aes_key_setup(kek);

	if(tfm) {
		rc = __dek_aes_do_crypt(tfm, src, dst, len, false);
		dek_aes_key_free(tfm);
		return rc;
	} else
		return -ENOMEM;
}

int dek_aes_encrypt_key(kek_t *kek, unsigned char *key, unsigned int key_len,
						unsigned char *out, unsigned int *out_len)
{
	if (kek == NULL)
		return -EINVAL;
	return __dek_aes_encrypt_key_raw(kek->buf, kek->len, key, key_len, out, out_len);
}

int dek_aes_encrypt_key_raw(unsigned char *kek, unsigned int kek_len,
							unsigned char *key, unsigned int key_len,
							unsigned char *out, unsigned int *out_len)
{
	if (kek == NULL || kek_len == 0)
		return -EINVAL;
	return __dek_aes_encrypt_key_raw(kek, kek_len, key, key_len, out, out_len);
}

inline int __dek_aes_encrypt_key_raw(unsigned char *kek, unsigned int kek_len,
									unsigned char *key, unsigned int key_len,
									unsigned char *out, unsigned int *out_len)
{
	int rc;
	int type;
	int ekey_len = 0;
	struct crypto_aead *tfm;

	tfm = sdp_crypto_aes_gcm_key_setup(kek, kek_len);
	if (unlikely(IS_ERR(tfm))) {
		rc = PTR_ERR(tfm);
		goto end;
	}

	type = CONV_DLEN_TO_TYPE(key_len);
	ekey_len = CONV_TYPE_TO_PLEN(type);

	switch(type) {
		case SDP_CRYPTO_GCM_PACK32:
			rc = __dek_aes_encrypt_key32(tfm, key, key_len, out);
			break;
		case SDP_CRYPTO_GCM_PACK64:
			rc = __dek_aes_encrypt_key64(tfm, key, key_len, out);
			break;
		default:
			printk(KERN_ERR
				"dek_aes_encrypt_key: not supported key length(%d)\n", key_len);
			rc = -EINVAL;
			break;
	}
	sdp_crypto_aes_gcm_key_free(tfm);

end:
	if (rc)
		*out_len = 0;
	else
		*out_len = ekey_len;
	return rc;
}

inline int __dek_aes_encrypt_key32(struct crypto_aead *tfm,
					unsigned char *key, unsigned int key_len,
					unsigned char *out)
{
	int rc;
	gcm_pack32 pack;
	gcm_pack __pack;

	memset(&pack, 0, sizeof(pack));
	__pack.type = SDP_CRYPTO_GCM_PACK32;
	__pack.iv = pack.iv;
	__pack.data = pack.data;
	__pack.auth = pack.auth;

	rc = __dek_aes_encrypt_key(tfm, key, key_len, &__pack);
	if (!rc)
		memcpy(out, &pack, sizeof(pack));
	memzero_explicit(&pack, sizeof(gcm_pack32));
	return rc;
}

inline int __dek_aes_encrypt_key64(struct crypto_aead *tfm,
					unsigned char *key, unsigned int key_len,
					unsigned char *out)
{
	int rc;
	gcm_pack64 pack;
	gcm_pack __pack;

	memset(&pack, 0, sizeof(gcm_pack64));
	__pack.type = SDP_CRYPTO_GCM_PACK64;
	__pack.iv = pack.iv;
	__pack.data = pack.data;
	__pack.auth = pack.auth;

	rc = __dek_aes_encrypt_key(tfm, key, key_len, &__pack);
	if (!rc)
		memcpy(out, &pack, sizeof(pack));
	memzero_explicit(&pack, sizeof(pack));
	return rc;
}

inline int __dek_aes_encrypt_key(struct crypto_aead *tfm,
					unsigned char *key, unsigned int key_len,
					gcm_pack *pack)
{
	int rc = sdp_crypto_generate_key(pack->iv, SDP_CRYPTO_GCM_IV_LEN);
	if (rc)
		memset(pack->iv, 0, SDP_CRYPTO_GCM_IV_LEN);

	memcpy(pack->data, key, key_len);
	rc = sdp_crypto_aes_gcm_encrypt_pack(tfm, pack);
	return rc;
}

int dek_aes_decrypt_key(kek_t *kek, unsigned char *ekey, unsigned int ekey_len,
						unsigned char *out, unsigned int *out_len)
{
	if (kek == NULL)
		return -EINVAL;
	return __dek_aes_decrypt_key_raw(kek->buf, kek->len, ekey, ekey_len, out, out_len);
}

int dek_aes_decrypt_key_raw(unsigned char *kek, unsigned int kek_len,
							unsigned char *ekey, unsigned int ekey_len,
							unsigned char *out, unsigned int *out_len)
{
	if (kek == NULL || kek_len == 0)
		return -EINVAL;
	return __dek_aes_decrypt_key_raw(kek, kek_len, ekey, ekey_len, out, out_len);
}

inline int __dek_aes_decrypt_key_raw(unsigned char  *kek, unsigned int kek_len,
									unsigned char *ekey, unsigned int ekey_len,
									unsigned char *out, unsigned int *out_len)
{
	int rc;
	int type;
	int key_len = 0;
	struct crypto_aead *tfm;

	tfm = sdp_crypto_aes_gcm_key_setup(kek, kek_len);
	if (unlikely(IS_ERR(tfm))) {
		rc = PTR_ERR(tfm);
		goto end;
	}

	type = CONV_PLEN_TO_TYPE(ekey_len);
	key_len = CONV_TYPE_TO_DLEN(type);

	switch(type) {
		case SDP_CRYPTO_GCM_PACK32:
			rc = __dek_aes_decrypt_key32(tfm, ekey, key_len, out);
			break;
		case SDP_CRYPTO_GCM_PACK64:
			rc = __dek_aes_decrypt_key64(tfm, ekey, key_len, out);
			break;
		default:
			printk(KERN_ERR
				"dek_aes_decrypt_key: not supported ekey length(%d)\n", ekey_len);
			rc = -EINVAL;
			break;
	}
	sdp_crypto_aes_gcm_key_free(tfm);

end:
	if (rc)
		*out_len = 0;
	else
		*out_len = key_len;
	return rc;
}

inline int __dek_aes_decrypt_key32(struct crypto_aead *tfm,
					unsigned char *ekey, unsigned int key_len,
					unsigned char *out)
{
	int rc;
	gcm_pack32 pack;
	gcm_pack __pack;

	memcpy(&pack, ekey, sizeof(pack));
	__pack.type = SDP_CRYPTO_GCM_PACK32;
	__pack.iv = pack.iv;
	__pack.data = pack.data;
	__pack.auth = pack.auth;

	rc = __dek_aes_decrypt_key(tfm, &__pack);
	if (!rc)
		memcpy(out, pack.data, key_len);
	memzero_explicit(&pack, sizeof(pack));
	return rc;
}

inline int __dek_aes_decrypt_key64(struct crypto_aead *tfm,
					unsigned char *ekey, unsigned int key_len,
					unsigned char *out)
{
	int rc;
	gcm_pack64 pack;
	gcm_pack __pack;

	memcpy(&pack, ekey, sizeof(pack));
	__pack.type = SDP_CRYPTO_GCM_PACK64;
	__pack.iv = pack.iv;
	__pack.data = pack.data;
	__pack.auth = pack.auth;

	rc = __dek_aes_decrypt_key(tfm, &__pack);
	if (!rc)
		memcpy(out, pack.data, key_len);
	memzero_explicit(&pack, sizeof(pack));
	return rc;
}

inline int __dek_aes_decrypt_key(struct crypto_aead *tfm,
					gcm_pack *pack)
{
	return sdp_crypto_aes_gcm_decrypt_pack(tfm, pack);
}
