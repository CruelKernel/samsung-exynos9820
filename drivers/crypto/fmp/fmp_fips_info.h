/*
 * Exynos FMP device header for FIPS
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __FMP_FIPS_INFO_H__
#define __FMP_FIPS_INFO_H__

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/scatterlist.h>

#include "fmp_fips_fops_info.h"
#include "sha256.h"
#include "hmac-sha256.h"

#define BYPASS_MODE	0
#define CBC_MODE	1
#define XTS_MODE	2

struct fcrypt {
	struct list_head list;
	struct mutex sem;
};

/* kernel-internal extension to struct crypt_op */
struct kernel_crypt_op {
	struct crypt_op cop;

	int ivlen;
	__u8 iv[EALG_MAX_BLOCK_LEN];

	int digestsize;
	uint8_t hash_output[AALG_MAX_RESULT_LEN];

	struct task_struct *task;
	struct mm_struct *mm;
};

struct todo_list_item {
	struct list_head __hook;
	struct kernel_crypt_op kcop;
	int result;
};

struct locked_list {
	struct list_head list;
	struct mutex lock;
};

struct fmp_fips_info {
	struct fcrypt fcrypt;
	struct locked_list free, todo, done;
	int itemcount;
	struct work_struct fmptask;
	wait_queue_head_t user_waiter;
	struct exynos_fmp *fmp;
	struct fmp_test_data *data;
};

/* compatibility stuff */
#ifdef CONFIG_COMPAT
#include <linux/compat.h>

/* input of FMPGSESSION */
struct compat_session_op {
	/* Specify either cipher or mac
	 */
	uint32_t	cipher;		/* cryptodev_crypto_op_t */
	uint32_t	mac;		/* cryptodev_crypto_op_t */

	uint32_t	keylen;
	compat_uptr_t	key;		/* pointer to key data */
	uint32_t	mackeylen;
	compat_uptr_t	mackey;		/* pointer to mac key data */

	uint32_t	ses;		/* session identifier */
};

/* input of FMPCRYPT */
struct compat_crypt_op {
	uint32_t	ses;		/* session identifier */
	uint16_t	op;		/* COP_ENCRYPT or COP_DECRYPT */
	uint16_t	flags;		/* see COP_FLAG_* */
	uint32_t	len;		/* length of source data */
	compat_uptr_t	src;		/* source data */
	compat_uptr_t	dst;		/* pointer to output data */
	compat_uptr_t	mac;/* pointer to output data for hash/MAC operations */
	compat_uptr_t	iv;/* initialization vector for encryption operations */

	__u32 data_unit_len;
	__u32 data_unit_seqnumber;

	compat_uptr_t secondLastEncodedData;
	compat_uptr_t thirdLastEncodedData;
};

#define COMPAT_FMPGSESSION    _IOWR('c', 200, struct compat_session_op)
#define COMPAT_FMPCRYPT       _IOWR('c', 203, struct compat_crypt_op)
#define COMPAT_FMP_AES_CBC_MCT	_IOWR('c', 204, struct compat_crypt_op)
#endif

/* the maximum of the above */
#define EALG_MAX_BLOCK_LEN	16

struct cipher_data {
	int init; /* 0 uninitialized */
	int blocksize;
	int aead;
	int stream;
	int ivsize;
	int alignmask;
	struct {
		/* block ciphers */
		struct crypto_ablkcipher *s;
		struct ablkcipher_request *request;

		/* AEAD ciphers */
		struct crypto_aead *as;
		struct aead_request *arequest;

		struct fmp_fips_result *result;
		uint8_t iv[EALG_MAX_BLOCK_LEN];
	} async;
};

struct hash_data {
	int init; /* 0 uninitialized */
	int digestsize;
	int alignmask;
	SHA256_CTX *sha;
	HMAC_SHA256_CTX *hmac;
	struct {
		struct crypto_ahash *s;
		struct fmp_fips_result *result;
		struct ahash_request *request;
	} async;
};

struct fmp_fips_result {
	struct completion completion;
	int err;
};

/* other internal structs */
struct csession {
	struct list_head entry;
	struct mutex sem;
	struct cipher_data cdata;
	struct hash_data hdata;
	uint32_t sid;
	uint32_t alignmask;

	unsigned int array_size;
	unsigned int used_pages; /* the number of pages that are used */
	/* the number of pages marked as writable (first are the readable) */
	unsigned int readable_pages;
	struct page **pages;
	struct scatterlist *sg;
};

struct csession *fmp_get_session_by_sid(struct fcrypt *fcr, uint32_t sid);

static inline void fmp_put_session(struct csession *ses_ptr)
{
	mutex_unlock(&ses_ptr->sem);
}
int adjust_sg_array(struct csession *ses, int pagecount);

#define MAX_TAP		8
#define XBUFSIZE	8

#define FIPS_MAX_LEN_KEY    128
#define FIPS_MAX_LEN_IV    32
#define FIPS_MAX_LEN_PCTEXT    512
#define FIPS_MAX_LEN_DIGEST    64

struct cipher_testvec {
	const char key[FIPS_MAX_LEN_KEY];
	const char iv[FIPS_MAX_LEN_IV];
	const char input[FIPS_MAX_LEN_PCTEXT];
	const char result[FIPS_MAX_LEN_PCTEXT];
	unsigned short tap[MAX_TAP];
	int np;
	unsigned char also_non_np;
	unsigned char klen;
	unsigned short ilen;
	unsigned short rlen;
};

struct hash_testvec {
	/* only used with keyed hash algorithms */
	const char key[FIPS_MAX_LEN_KEY];
	const char plaintext[FIPS_MAX_LEN_PCTEXT];
	const char digest[FIPS_MAX_LEN_DIGEST];
	unsigned char tap[MAX_TAP];
	unsigned short psize;
	unsigned char np;
	unsigned char ksize;
};

struct cipher_test_suite {
	struct {
		const struct cipher_testvec *vecs;
		unsigned int count;
	} enc;
};

struct hash_test_suite {
	const struct hash_testvec *vecs;
	unsigned int count;
};

struct exynos_fmp_fips_test_vops {
	int	(*integrity)(HMAC_SHA256_CTX *desc, unsigned long *start_addr);
	int	(*zeroization)(struct fmp_table_setting *table, char *str);
	int	(*hmac_sha256)(char *digest, char *algorithm);
	int	(*aes)(const int mode, char *key, unsigned char klen);
};

#endif
