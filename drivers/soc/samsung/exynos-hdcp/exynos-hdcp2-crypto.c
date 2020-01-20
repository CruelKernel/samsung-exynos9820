/* drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-crypto.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/err.h>
#include <linux/crypto.h>
#include <linux/ctype.h>
#include <crypto/hash.h>
#include <crypto/sha.h>

struct sdesc {
	struct shash_desc shash;
	char ctx[];
};

static struct sdesc *alloc_sdesc(struct crypto_shash *alg)
{
	struct sdesc *sdesc;
	int size;

	size = sizeof(struct shash_desc) + crypto_shash_descsize(alg);
	sdesc = kmalloc(size, GFP_KERNEL);
	if (!sdesc)
		return ERR_PTR(-ENOMEM);
	sdesc->shash.tfm = alg;
	sdesc->shash.flags = 0x0;
	return sdesc;
}

int hdcp_calc_sha1(u8 *digest, const u8 *buf, unsigned int buflen)
{
	struct crypto_shash *hashalg;
	const char hash_alg[] = "sha1";
	struct sdesc *sdesc;
	int ret;

	hashalg = crypto_alloc_shash(hash_alg, 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(hashalg)) {
		pr_info("encrypted_key: could not allocate crypto %s\n",
			hash_alg);
		return PTR_ERR(hashalg);
	}

	sdesc = alloc_sdesc(hashalg);
	if (IS_ERR(sdesc)) {
		pr_err("alloc_sdesc: can't alloc %s\n", hash_alg);
		if (hashalg)
			crypto_free_shash(hashalg);
		return PTR_ERR(sdesc);
	}

	ret = crypto_shash_digest(&sdesc->shash, buf, buflen, digest);
	kfree(sdesc);

	if (hashalg)
		crypto_free_shash(hashalg);

	return ret;
}
