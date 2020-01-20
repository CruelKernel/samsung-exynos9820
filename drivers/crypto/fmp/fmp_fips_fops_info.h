/*
 * Exynos FMP device information for FIPS
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _FMP_FIPS_FOPS_INFO_
#define _FMP_FIPS_FOPS_INFO_

/* API extensions for linux */
#define FMP_HMAC_MAX_KEY_LEN	512
#define FMP_CIPHER_MAX_KEY_LEN	64

enum fmpdev_crypto_op_t {
	FMP_AES_CBC = 1,
	FMP_AES_XTS = 2,
	FMP_SHA2_256_HMAC = 3,
	FMP_SHA2_256 = 4,
	FMPFW_SHA2_256_HMAC = 5,
	FMPFW_SHA2_256 = 6,
	FMP_ALGORITHM_ALL, /* Keep updated - see below */
};
#define	FMP_ALGORITHM_MAX	(FMP_ALGORITHM_ALL - 1)

/* the maximum of the above */
#define EALG_MAX_BLOCK_LEN	16

/* Values for hashes/MAC */
#define AALG_MAX_RESULT_LEN	64

/* maximum length of verbose alg names (depends on CRYPTO_MAX_ALG_NAME) */
#define FMPDEV_MAX_ALG_NAME	64

#define DEFAULT_PREALLOC_PAGES	32

/* input of FMPGSESSION */
struct session_op {
	__u32 cipher;		/* cryptodev_crypto_op_t */
	__u32 mac;		/* cryptodev_crypto_op_t */

	__u32 keylen;
	__u8 __user *key;
	__u32 mackeylen;
	__u8 __user *mackey;

	__u32 ses;		/* session identifier */
};

struct session_info_op {
	__u32 ses;		/* session identifier */

	/* verbose names for the requested ciphers */
	struct alg_info {
		char cra_name[FMPDEV_MAX_ALG_NAME];
		char cra_driver_name[FMPDEV_MAX_ALG_NAME];
	} cipher_info, hash_info;

	__u16 alignmask;	/* alignment constraints */
	__u32 flags;          /* SIOP_FLAGS_* */
};

/* If this flag is set then this algorithm uses
 * a driver only available in kernel (software drivers,
 * or drivers based on instruction sets do not set this flag).
 *
 * If multiple algorithms are involved (as in AEAD case), then
 * if one of them is kernel-driver-only this flag will be set.
 */
#define SIOP_FLAG_KERNEL_DRIVER_ONLY 1

#define	COP_ENCRYPT	0
#define COP_DECRYPT	1

/* input of FMPCRYPT */
struct crypt_op {
	__u32 ses;		/* session identifier */
	__u16 op;		/* COP_ENCRYPT or COP_DECRYPT */
	__u16 flags;		/* see COP_FLAG_* */
	__u32 len;		/* length of source data */
	__u8 __user *src;	/* source data */
	__u8 __user *dst;	/* pointer to output data */
	/* pointer to output data for hash/MAC operations */
	__u8 __user *mac;
	/* initialization vector for encryption operations */
	__u8 __user *iv;

	__u32 data_unit_len;
	__u32 data_unit_seqnumber;

	__u8 __user *secondLastEncodedData;
	__u8 __user *thirdLastEncodedData;
};

#define COP_FLAG_NONE		(0 << 0) /* totally no flag */
#define COP_FLAG_UPDATE		(1 << 0) /* multi-update hash mode */
#define COP_FLAG_FINAL		(1 << 1) /* multi-update final hash mode */
#define COP_FLAG_WRITE_IV	(1 << 2) /* update the IV during operation */
#define COP_FLAG_NO_ZC		(1 << 3) /* do not zero-copy */
#define COP_FLAG_AEAD_TLS_TYPE  (1 << 4) /* authenticate and encrypt using the
					* TLS protocol rules */
#define COP_FLAG_AEAD_SRTP_TYPE  (1 << 5) /* authenticate and encrypt using the
					* SRTP protocol rules */
#define COP_FLAG_RESET		(1 << 6) /* multi-update reset the state.
					* should be used in combination
					* with COP_FLAG_UPDATE */
#define COP_FLAG_AES_CBC	(1 << 7)
#define COP_FLAG_AES_XTS	(1 << 8)
#define COP_FLAG_AES_CBC_MCT	(1 << 9)

#define FMPGSESSION		_IOWR('c', 200, struct session_op)
#define FMPFSESSION		_IOWR('c', 201, __u32)
#define FMPGSESSIONINFO		_IOWR('c', 202, struct session_info_op)
#define FMPCRYPT		_IOWR('c', 203, struct crypt_op)
#define FMP_AES_CBC_MCT		_IOWR('c', 204, struct crypt_op)

#endif
