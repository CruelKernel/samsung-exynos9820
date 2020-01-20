/*
 * Exynos FMP cipher driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <crypto/fmp.h>
#include "fmp_fips_info.h"

int fmp_cipher_set_key(struct fmp_test_data *fdata,
		const u8 *key, uint32_t key_len)
{
	int ret = 0;
	struct fmp_crypto_info *ci;

	if (!fdata)
		return -EINVAL;

	ci = &fdata->ci;
	memset(ci->key, 0, FMP_MAX_KEY_SIZE);
	ci->key_size = key_len;

	if (ci->algo_mode == EXYNOS_FMP_ALGO_MODE_AES_CBC) {
		switch (key_len) {
		case FMP_KEY_SIZE_16:
			ci->fmp_key_size = EXYNOS_FMP_KEY_SIZE_16;
			break;
		case FMP_KEY_SIZE_32:
			ci->fmp_key_size = EXYNOS_FMP_KEY_SIZE_32;
			break;
		default:
			pr_err("%s: Invalid FMP CBC key size(%d)\n",
				__func__, key_len);
			ret = -EINVAL;
			goto err;
		}
		memcpy(ci->key, key, key_len);
	} else if (ci->algo_mode == EXYNOS_FMP_ALGO_MODE_AES_XTS) {
		switch (key_len >> 1) {
		case FMP_KEY_SIZE_16:
			ci->fmp_key_size = EXYNOS_FMP_KEY_SIZE_16;
			break;
		case FMP_KEY_SIZE_32:
			ci->fmp_key_size = EXYNOS_FMP_KEY_SIZE_32;
			break;
		default:
			pr_err("%s: Invalid FMP XTS key size(%d)\n",
				__func__, key_len);
			ret = -EINVAL;
			goto err;
		}
		memcpy(ci->key, key, key_len);
	} else {
		pr_err("%s: Invalid FMP encryption mode(%d)\n",
				__func__, ci->algo_mode);
		ret = -EINVAL;
		goto err;
	}
err:
	return ret;
}

int fmp_cipher_set_iv(struct fmp_test_data *fdata,
		const u8 *iv, uint32_t iv_len)
{
	int ret = 0;
	struct fmp_crypto_info *ci;

	if (!fdata) {
		pr_err("%s: Invalid fdata\n", __func__);
		return -EINVAL;
	}

	ci = &fdata->ci;
	if (iv_len != FMP_IV_SIZE_16) {
		pr_err("%s: Invalid FMP iv size(%d)\n", __func__, iv_len);
		ret = -EINVAL;
		goto err;
	}

	memset(fdata->iv, 0, FMP_IV_SIZE_16);
	memcpy(fdata->iv, iv, iv_len);

err:
	return ret;
}
