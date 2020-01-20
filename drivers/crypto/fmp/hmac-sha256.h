/*
 * Cryptographic API.
 *
 * HMAC: Keyed-Hashing for Message Authentication (RFC2104).
 * SHA-256 is used as an underlying hash function.
 *
 * Author : Igor Shcheglakov (i.shcheglako@samsung.com)
 * Date   : 15 Dec 2017
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 */

#ifndef _FMP_HMAC_SHA256_H
#define _FMP_HMAC_SHA256_H

#include "sha256.h"

struct hmac_sha256_ctx {
	struct shash_desc inner_ctx;
	struct shash_desc outer_ctx;
};

typedef struct hmac_sha256_ctx HMAC_SHA256_CTX;

/* Initialize hashing context using a key of key_len bytes.
 *
 * Return zero on success and negative otherwise
 */
int hmac_sha256_init(struct hmac_sha256_ctx *ctx, const u8 *key, unsigned int key_len);

/*  Hash data_len bytes of data
 *
 * Return zero on success and negative otherwise
 */
int hmac_sha256_update(struct hmac_sha256_ctx *ctx, const u8 *data, unsigned int data_len);

/* Add final padding to a hash and write resulting HMAC to out.
 * There must be at least SHA256_DIGEST_LENGTH bytes of space in out
 *
 * Return zero on success and negative otherwise
 */
int hmac_sha256_final(struct hmac_sha256_ctx *ctx, u8 *out);

/* Hash data_len bytes of data using key of key_len bytes.
 * Write resulting HMAC to out. There must be at least SHA256_DIGEST_LENGTH
 * bytes of space in out
 *
 * Return zero on success and negative otherwise
 */
int hmac_sha256(const u8 *key, unsigned int key_len, const u8 *data, unsigned int data_len, u8 *out);

/* Cleanup allocated resources */
void hmac_sha256_ctx_cleanup(struct hmac_sha256_ctx *ctx);

#endif /* _FMP_HMAC_SHA256_H */

