/*
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Functions related to blkcrypt(for inline encryption) handling
 *
 * NOTE :
 * blkcrypt implementation depends on a inline encryption hardware.
 */

#include <linux/module.h>
#include <linux/blkdev.h>
#include <crypto/diskcipher.h>

/* For fmp 2.0 driver */
static void *blk_crypt_fmp_alloc_aes_xts(void)
{
	struct crypto_diskcipher *bctx;
	const char *cipher_str = "xts(aes)";

	/* try discipher first */
	bctx = crypto_alloc_diskcipher(cipher_str, 0, 0, 1);
	if (!bctx) {
		pr_debug("error allocating diskciher '%s' transform: %d",
			cipher_str, -ENOMEM);
		return ERR_PTR(-ENOMEM);
	}

	if (IS_ERR(bctx)) {
		pr_debug("error allocating diskciher '%s' transform: %d",
			cipher_str, PTR_ERR(bctx));
		return bctx;
	}

	return bctx;
}

static void blk_crypt_fmp_free_aes_xts(void *data)
{
	if (!data)
		return;

	crypto_free_req_diskcipher((struct crypto_diskcipher *)data);
}

static int blk_crypt_fmp_set_key(void *data, const char *raw_key, int keysize)
{
	return crypto_diskcipher_setkey((struct crypto_diskcipher *)data,
					raw_key, keysize, 0);
}

static const struct blk_crypt_algorithm_cbs fmp_hw_xts_cbs = {
	.alloc = blk_crypt_fmp_alloc_aes_xts,
	.free = blk_crypt_fmp_free_aes_xts,
	.set_key = blk_crypt_fmp_set_key,
};

static void *blk_crypt_handle = NULL;

/* Blk-crypt core functions */
static int __init blk_crypt_alg_fmp_init(void)
{
	pr_info("%s\n", __func__);
	blk_crypt_handle = blk_crypt_alg_register(NULL, "xts(aes)",
				BLK_CRYPT_MODE_INLINE_PRIVATE, &fmp_hw_xts_cbs);
	if (IS_ERR(blk_crypt_handle)) {
		pr_err("%s: failed to register alg(xts(aes)), err:%d\n",
			__func__, PTR_ERR(blk_crypt_handle) );
		blk_crypt_handle = NULL;
	}
		
	return 0;
}

static void __exit blk_crypt_alg_fmp_exit(void)
{
	pr_info("%s\n", __func__);
	blk_crypt_alg_unregister(blk_crypt_handle);
	blk_crypt_handle = NULL;
}

module_init(blk_crypt_alg_fmp_init);
module_exit(blk_crypt_alg_fmp_exit);
