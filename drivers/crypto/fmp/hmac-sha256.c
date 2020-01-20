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

#include <linux/kernel.h>
#include <linux/string.h>

#include "hmac-sha256.h"
#include "sha256.h"

int hmac_sha256_init(struct hmac_sha256_ctx *ctx, const u8 *key, unsigned int key_len)
{
	int ret = -1;
	unsigned int i;
	u8 key_buf[SHA256_BLOCK_SIZE];
	unsigned int key_buf_len;
	u8 inner_pad[SHA256_BLOCK_SIZE];
	u8 outer_pad[SHA256_BLOCK_SIZE];
	unsigned int block_size = SHA256_BLOCK_SIZE;
	unsigned int digest_size = SHA256_DIGEST_SIZE;

	if (!ctx || !key)
		goto err;

	// long keys are hashed
	if (key_len > block_size) {

		if (sha256(key, key_len, key_buf))
			goto err;

		key_buf_len = digest_size;
	} else {
		memcpy(key_buf, key, key_len);
		key_buf_len = key_len;
	}

	// short keys are padded with zeroes to the right
	if (key_buf_len != block_size)
		memset(&key_buf[key_buf_len], 0, sizeof(key_buf) - key_buf_len);

	// inner and outer padding calculation
	for (i = 0; i < block_size; i++) {
		inner_pad[i] = 0x36 ^ key_buf[i];
		outer_pad[i] = 0x5c ^ key_buf[i];
	}

	// inner hash context preparation
	if (sha256_init(&ctx->inner_ctx))
		goto err;

	if (sha256_update(&ctx->inner_ctx, inner_pad, sizeof(inner_pad)))
		goto err;

	// outer hash context preparation
	if (sha256_init(&ctx->outer_ctx))
		goto err;

	if (sha256_update(&ctx->outer_ctx, outer_pad, sizeof(outer_pad)))
		goto err;

	ret = 0;

err:
	memset(key_buf, 0, sizeof(key_buf));
	memset(inner_pad, 0, sizeof(inner_pad));
	memset(outer_pad, 0, sizeof(outer_pad));

	return ret;
}

int hmac_sha256_update(struct hmac_sha256_ctx *ctx, const u8 *data, unsigned int data_len)
{
	if (!ctx || !data)
		return 0;

	return sha256_update(&ctx->inner_ctx, data, data_len);
}

int hmac_sha256_final(struct hmac_sha256_ctx *ctx, u8 *out)
{
	int ret = -1;
	u8 result[SHA256_DIGEST_SIZE];

	if (!ctx || !out)
		goto err;

	if (sha256_final(&ctx->inner_ctx, result))
		goto err;

	if (sha256_update(&ctx->outer_ctx, result, sizeof(result)))
		goto err;

	if (sha256_final(&ctx->outer_ctx, result))
		goto err;

	memcpy(out, result, sizeof(result));

	ret = 0;

err:
	return ret;
}

int hmac_sha256(const u8 *key, unsigned int key_len, const u8 *data, unsigned int data_len, u8 *out)
{
	int ret = -1;
	struct hmac_sha256_ctx ctx;

	if (hmac_sha256_init(&ctx, key, key_len))
		goto err;

	if (hmac_sha256_update(&ctx, data, data_len))
		goto err;

	if (hmac_sha256_final(&ctx, out))
		goto err;

	ret = 0;

err:
	return ret;
}

void hmac_sha256_ctx_cleanup(struct hmac_sha256_ctx *ctx)
{
	memset(ctx, 0, sizeof(struct hmac_sha256_ctx));
}
