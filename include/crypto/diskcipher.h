/*
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _DISKCIPHER_H_
#define _DISKCIPHER_H_

#include <linux/crypto.h>
#include <linux/blk_types.h>

struct diskcipher_alg;

struct crypto_diskcipher {
	u32 algo;
	unsigned int ivsize;
#ifdef USE_FREE_REQ
	/* for crypto_free_req_diskcipher */
	unsigned long req_jiffies;
	struct list_head node;
#endif
	atomic_t status;
	struct crypto_tfm base;
};

struct diskcipher_test_request {
	unsigned int cryptlen;
	const u8 *iv;
	struct scatterlist *src;
	struct scatterlist *dst;
	bool enc;
};

/**
 * struct diskcipher_alg - symmetric key cipher definition
 * for inline crypto engine on disk host device
 *
 * @setkey
 * @clearkey
 * @crypt
 * @clear
 * @do_crypt
 * @base:	Common crypto API algorithm data structure.
 *
 * Diskcipher supports APIs to set crypto information for dm-crypt and fscrypt
 * And pass the crypto information to disk host device via bio.
 * Crypt operation executes on inline crypto on disk host device.
 */
#ifdef USE_FREE_REQ
struct diskcipher_freectrl {
	spinlock_t freelist_lock;
	struct list_head freelist;
	struct delayed_work freewq;
	atomic_t freewq_active;
	u32 max_io_ms;
};
#endif

struct diskcipher_alg {
	int (*setkey)(struct crypto_tfm *tfm, const char *key, u32 keylen,
		       bool persistent);
	int (*clearkey)(struct crypto_tfm *tfm);
	int (*crypt)(struct crypto_tfm *tfm, void *req);
	int (*clear)(struct crypto_tfm *tfm, void *req);
#ifndef CONFIG_CRYPTO_MANAGER_DISABLE_TESTS
	int (*do_crypt)(struct crypto_tfm *tfm,
		struct diskcipher_test_request *req);
#endif
#ifdef USE_FREE_REQ
	struct diskcipher_freectrl freectrl;
#endif
	struct crypto_alg base;
};

static inline unsigned int crypto_diskcipher_ivsize(struct crypto_diskcipher *tfm)
{
	return tfm->ivsize;
}

static inline struct crypto_tfm *crypto_diskcipher_tfm(struct crypto_diskcipher
						       *tfm)
{
	return &tfm->base;
}

static inline struct diskcipher_alg *crypto_diskcipher_alg(struct crypto_alg
							   *alg)
{
	return container_of(alg, struct diskcipher_alg, base);
}

static inline struct crypto_diskcipher *__crypto_diskcipher_cast(
	struct crypto_tfm *tfm)
{
	return container_of(tfm, struct crypto_diskcipher, base);
}

int crypto_register_diskcipher(struct diskcipher_alg *alg);
void crypto_unregister_diskcipher(struct diskcipher_alg *alg);
int crypto_register_diskciphers(struct diskcipher_alg *algs, int count);
void crypto_unregister_diskciphers(struct diskcipher_alg *algs, int count);

#if defined(CONFIG_CRYPTO_DISKCIPHER)
/**
 * crypto_alloc_diskcipher() - allocate symmetric cipher running on disk device
 * @alg_name: is the cra_name / name or cra_driver_name / driver name of the
 *	      skcipher cipher
 * @type: specifies the type of the cipher
 * @mask: specifies the mask for the cipher
 * @force: add diskcipher postfix '-disk' on algo_name
 *
 * Allocate a cipher handle for an diskcipher. The returned struct
 * crypto_diskcipher is the cipher handle that is required for any subsequent
 * API invocation for that diskcipher.
 *
 * Return: allocated cipher handle in case of success; IS_ERR() is true in case
 *	   of an error, PTR_ERR() returns the error code.
 */
struct crypto_diskcipher *crypto_alloc_diskcipher(const char *alg_name,
			  u32 type, u32 mask, bool force);

/**
 * crypto_free_diskcipher() - zeroize and free cipher handle
 * @tfm: cipher handle to be freed
 */
void crypto_free_diskcipher(struct crypto_diskcipher *tfm);
void crypto_free_req_diskcipher(struct crypto_diskcipher *tfm);

/**
 * crypto_diskcipher_get() - get diskcipher from bio
 * @bio: bio structure
 */
struct crypto_diskcipher *crypto_diskcipher_get(struct bio *bio);

/**
 * crypto_diskcipher_set() - set diskcipher to bio
 * @bio: bio structure to contain diskcipher
 * @tfm: cipher handle
 *
 * This functions set thm to bio->bi_aux_private to pass it to host driver.
 *
 */
void crypto_diskcipher_set(struct bio *bio, struct crypto_diskcipher *tfm, u64 dun);

/**
 * crypto_diskcipher_setkey() - set key for cipher
 * @tfm: cipher handle
 * @key: buffer holding the key
 * @keylen: length of the key in bytes
 * @persistent: option of key storage option
 *
 * The caller provided key is set for the skcipher referenced by the cipher
 * handle.
 *
 * Return: 0 if the setting of the key was successful; < 0 if an error occurred
 */
int crypto_diskcipher_setkey(struct crypto_diskcipher *tfm, const char *key,
			     u32 keylen, bool persistent);

/**
 * crypto_diskcipher_clearkey() - clear key
 * @tfm: cipher handle
 */
int crypto_diskcipher_clearkey(struct crypto_diskcipher *tfm);

/**
 * crypto_diskcipher_set_crypt() - set crypto info for inline crypto engine
 * @tfm: cipher handle
 * @req: request handle. it's specific structure for inline crypt hardware
 *
 * Return: 0 if the setting of the key was successful; < 0 if an error occurred
 */
int crypto_diskcipher_set_crypt(struct crypto_diskcipher *tfm, void *req);

/**
 * crypto_diskcipher_clear_crypt() - clear crypto info on inline crypt hardware
 * @tfm: cipher handle
 * @req: request handle. it's specific structure for inline crypt hardware
 *
 * Return: 0 if the setting of the key was successful; < 0 if an error occurred
 */
int crypto_diskcipher_clear_crypt(struct crypto_diskcipher *tfm, void *req);

#ifndef CONFIG_CRYPTO_MANAGER_DISABLE_TESTS
/**
 * diskcipher_do_crypt() - execute crypto for test
 * @tfm: cipher handle
 * @req: diskcipher_test_request handle
 *
 * The caller uses this function to request crypto
 * Diskcipher_algo allocates the block area for test and then request block I/O
 *
 */
int diskcipher_do_crypt(struct crypto_diskcipher *tfm,
				struct diskcipher_test_request *req);

/**
 * diskcipher_request_set_crypt() - fill diskcipher_test_requeust
 * @req: request handle
 * @src: source scatter / gather list
 * @dst: destination scatter / gather list
 * @cryptlen: number of bytes to process from @src
 * @iv: IV for the cipher operation which must comply with the IV size defined
 *      by crypto_skcipher_ivsize
 * @enc: encrypt(1) / decrypt(0)
 *
 * This function allows setting of the source data and destination data
 * scatter / gather lists.
 *
 * For encryption, the source is treated as the plaintext and the
 * destination is the ciphertext. For a decryption operation, the use is
 * reversed - the source is the ciphertext and the destination is the plaintext.
 */
static inline void diskcipher_request_set_crypt(
	struct diskcipher_test_request *req,
	struct scatterlist *src, struct scatterlist *dst,
	unsigned int cryptlen, void *iv, bool enc)
{
	req->src = src;
	req->dst = dst;
	req->cryptlen = cryptlen;
	req->iv = iv;
	req->enc = enc;
}
#endif

enum diskcipher_dbg {
	DISKC_API_ALLOC, DISKC_API_FREE, DISKC_API_FREEREQ, DISKC_API_SETKEY,
	DISKC_API_SET, DISKC_API_GET, DISKC_API_CRYPT, DISKC_API_CLEAR,
	DISKC_API_MAX, FS_PAGEIO, FS_READP, FS_DIO, FS_BLOCK_WRITE,
	FS_ZEROPAGE, BLK_BH, DMCRYPT, DISKC_MERGE, DISKC_MERGE_ERR_INODE, DISKC_MERGE_ERR_DISKC,
	FS_DEC_WARN, FS_ENC_WARN, DISKC_MERGE_DIO, DISKC_FREE_REQ_WARN,
	DISKC_FREE_WQ_WARN, DISKC_CRYPT_WARN,
	DM_CRYPT_NONENCRYPT, DM_CRYPT_CTR, DM_CRYPT_DTR, DM_CRYPT_OVER,
	F2FS_gc, F2FS_gc_data_page, F2FS_gc_data_page_key, F2FS_gc_data_page_key_FC,
	F2FS_gc_data_page_FC, F2FS_gc_data_block, F2FS_gc_data_block_key,
	F2FS_gc_data_block_err1, F2FS_gc_data_block_err2, F2FS_gc_data_block_err3, F2FS_gc_skip,
	DISKC_ERR, DISKC_NO_KEY_ERR, DISKC_NO_SYNC_ERR, DISKC_NO_CRYPT_ERR, DISKC_NO_DISKC_ERR,
	DISKC_USER_MAX
};

#ifdef CONFIG_CRYPTO_DISKCIPHER_DEBUG
void crypto_diskcipher_debug(enum diskcipher_dbg dbg, int idx);
#else
#define crypto_diskcipher_debug(a, b) ((void)0)
#endif
#else
#define crypto_alloc_diskcipher(a, b, c, d) ((void *)-EINVAL)
#define crypto_free_diskcipher(a) ((void)0)
#define crypto_free_req_diskcipher(a) ((void)0)
#define crypto_diskcipher_get(a) ((void *)NULL)
#define crypto_diskcipher_set(a, b, c, d) ((void)0)
#define crypto_diskcipher_clearkey(a) ((void)0)
#define crypto_diskcipher_setkey(a, b, c, d) (-1)
#define crypto_diskcipher_debug(a, b) ((void)0)
#endif
#endif /* _DISKCIPHER_H_ */
