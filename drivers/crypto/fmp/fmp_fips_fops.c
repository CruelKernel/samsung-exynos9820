/*
 * Exynos FMP func test driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/completion.h>
#include <linux/rtnetlink.h>
#include <linux/slab.h>

#include <crypto/fmp.h>
#include <crypto/authenc.h>

#include <asm/cacheflush.h>
#include <linux/uaccess.h>
#include <asm/memory.h>

#include "fmp_fips_main.h"
#include "fmp_fips_info.h"
#include "fmp_fips_cipher.h"
#include "sha256.h"
#include "hmac-sha256.h"
#include "fmp_test.h"

#define INIT		0
#define UPDATE		1
#define FINAL		2

/* ====== Compile-time config ====== */

/*
 * Default (pre-allocated) and maximum size of the job queue.
 * These are free, pending and done items all together.
 */
#define DEF_COP_RINGSIZE 16
#define MAX_COP_RINGSIZE 64

static int fmp_fips_set_key(struct exynos_fmp *fmp, struct fmp_fips_info *info,
			uint8_t *enckey, uint8_t *twkey, uint32_t key_len)
{
	int ret = 0;
	uint8_t *key;

	if (twkey) {
		key = kzalloc(key_len * 2, GFP_KERNEL);
		if (!key)
			return -ENOMEM;
		memcpy(key, enckey, key_len);
		memcpy(key + key_len, twkey, key_len);
		key_len *= 2;
	} else {
		key = kzalloc(key_len, GFP_KERNEL);
		if (!key)
			return -ENOMEM;
		memcpy(key, enckey, key_len);
	}

	ret = fmp_cipher_set_key(info->data, key, key_len);
	if (ret)
		dev_err(fmp->dev, "%s: Fail to set key. ret(%d)\n",
				__func__, ret);
	kzfree(key);
	return ret;
}

static int fmp_fips_cipher_init(struct fmp_fips_info *info,
			struct cipher_data *out, const char *alg_name,
			uint8_t *enckey, uint8_t *twkey, size_t key_len)
{
	int ret = 0;
	struct exynos_fmp *fmp;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return -ENODEV;
	}
	fmp = info->fmp;
	memset(out, 0, sizeof(*out));

	if (!strcmp(alg_name, "cbc(aes-fmp)"))
		info->data->ci.algo_mode = EXYNOS_FMP_ALGO_MODE_AES_CBC;
	else if (!strcmp(alg_name, "xts(aes-fmp)"))
		info->data->ci.algo_mode = EXYNOS_FMP_ALGO_MODE_AES_XTS;
	else {
		dev_err(fmp->dev, "%s: Invalid mode\n", __func__);
		return -EINVAL;
	}

	out->blocksize = 16;
	out->ivsize = 16;
	ret = fmp_fips_set_key(fmp, info, enckey, twkey, key_len);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set fmp key\n", __func__);
		return ret;
	}

	out->init = 1;
	return ret;
}

static void fmp_fips_cipher_deinit(struct cipher_data *cdata)
{
	if (cdata->init)
		cdata->init = 0;
}

static int fmp_fips_hash_init(struct fmp_fips_info *info, struct hash_data *hdata,
			const char *alg_name,
			int hmac_mode, void *mackey, size_t mackeylen)
{
	int ret = -ENOMSG;
	struct exynos_fmp *fmp;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return -ENODEV;
	}
	fmp = info->fmp;

	hdata->init = 0;

	switch (hmac_mode) {
	case 0:
		hdata->sha = kzalloc(sizeof(*hdata->sha), GFP_KERNEL);
		if (!hdata->sha)
			return -ENOMEM;

		ret = sha256_init(hdata->sha);
		break;
	case 1:
		hdata->hmac = kzalloc(sizeof(*hdata->hmac), GFP_KERNEL);
		if (!hdata->hmac)
			return -ENOMEM;

		ret = hmac_sha256_init(hdata->hmac, mackey, mackeylen);
		break;
	default:
		dev_err(fmp->dev, "Wrong mode\n");
		return ret;
	}

	if (ret == 0)
		hdata->init = 1;

	return ret;
}

static void fmp_fips_hash_deinit(struct hash_data *hdata)
{
	if (hdata->hmac != NULL) {
		hmac_sha256_ctx_cleanup(hdata->hmac);
		kzfree(hdata->hmac);
		return;
	}

	if (hdata->sha != NULL) {
		memset(hdata->sha, 0x00, sizeof(*hdata->sha));
		kzfree(hdata->sha);
		return;
	}
}

static ssize_t fmp_fips_hash_update(struct exynos_fmp *fmp,
				struct hash_data *hdata,
				struct scatterlist *sg, size_t len)
{
	int ret = -ENOMSG;
	int8_t *buf;

	buf = kmalloc(len, GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto exit;
	}

	if (len != sg_copy_to_buffer(sg, 1, buf, len)) {
		dev_err(fmp->dev, "%s: sg_copy_buffer error copying\n", __func__);
		goto exit;
	}

	if (hdata->sha != NULL)
		ret = sha256_update(hdata->sha, buf, len);
	else
		ret = hmac_sha256_update(hdata->hmac, buf, len);

exit:
	kzfree(buf);
	return ret;
}

static int fmp_fips_hash_final(struct exynos_fmp *fmp,
				struct hash_data *hdata, void *output)
{
	int ret_crypto = 0; /* OK if zero */

	if (hdata->sha != NULL)
		ret_crypto = sha256_final(hdata->sha, output);
	else
		ret_crypto = hmac_sha256_final(hdata->hmac, output);

	if (ret_crypto != 0)
		return -ENOMSG;

	hdata->digestsize = 32;
	return 0;
}

static int fmp_n_crypt(struct fmp_fips_info *info, struct csession *ses_ptr,
		struct crypt_op *cop,
		struct scatterlist *src_sg, struct scatterlist *dst_sg,
		uint32_t len)
{
	int ret;
	struct exynos_fmp *fmp;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return -ENODEV;
	}
	fmp = info->fmp;

	if (cop->op == COP_ENCRYPT) {
		if (ses_ptr->hdata.init != 0) {
			ret = fmp_fips_hash_update(fmp, &ses_ptr->hdata,
					src_sg, len);
			if (unlikely(ret))
				goto out_err;
		}

		if (ses_ptr->cdata.init != 0) {
			if (info->data) {
				ret = fmp_test_crypt(fmp, info->data,
					sg_virt(src_sg), sg_virt(dst_sg), len,
					ENCRYPT, NULL, &info->data->ci);
				if (unlikely(ret))
					goto out_err;
			} else {
				dev_err(fmp->dev,
					"%s: no crypt for enc\n", __func__);
				ret = -EINVAL;
			}
		}
	} else {
		if (ses_ptr->cdata.init != 0) {
			if (info->data) {
				ret = fmp_test_crypt(fmp, info->data,
					sg_virt(src_sg), sg_virt(dst_sg), len,
					DECRYPT, NULL, &info->data->ci);
				if (unlikely(ret))
					goto out_err;
			} else {
				dev_err(fmp->dev,
					"%s: no crypt for dec\n", __func__);
				ret = -EINVAL;
			}
		}

		if (ses_ptr->hdata.init != 0) {
			ret = fmp_fips_hash_update(fmp, &ses_ptr->hdata,
					dst_sg, len);
			if (unlikely(ret))
				goto out_err;
		}
	}

	return 0;
out_err:
	dev_err(fmp->dev, "%s: FMP crypt failure: %d\n", __func__, ret);
	return ret;
}

static int __fmp_run_std(struct fmp_fips_info *info,
		struct csession *ses_ptr, struct crypt_op *cop)
{
	char *data;
	struct exynos_fmp *fmp;
	char __user *src, *dst;
	size_t nbytes, bufsize;
	struct scatterlist sg;
	int ret = 0;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return -ENODEV;
	}
	fmp = info->fmp;

	nbytes = cop->len;
	data = (char *)__get_free_page(GFP_KERNEL);
	if (unlikely(!data)) {
		dev_err(fmp->dev, "Error getting free page.\n");
		return -ENOMEM;
	}

	bufsize = (nbytes > PAGE_SIZE) ? PAGE_SIZE : nbytes;

	src = cop->src;
	dst = cop->dst;

	while (nbytes > 0) {
		size_t current_len = nbytes > bufsize ? bufsize : nbytes;

		if (unlikely(copy_from_user(data, src, current_len))) {
			dev_err(fmp->dev, "Error copying %d bytes from user address %p\n",
						(int)current_len, src);
			ret = -EFAULT;
			break;
		}

		sg_init_one(&sg, data, current_len);
		ret = fmp_n_crypt(info, ses_ptr, cop, &sg, &sg, current_len);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "fmp_n_crypt failed\n");
			break;
		}

		if (ses_ptr->cdata.init != 0) {
			if (unlikely(copy_to_user(dst, data, current_len))) {
				dev_err(fmp->dev, "could not copy to user\n");
				ret = -EFAULT;
				break;
			}
		}

		dst += current_len;
		nbytes -= current_len;
		src += current_len;
	}
	free_page((unsigned long)data);

	return ret;
}

static int fmp_run(struct fmp_fips_info *info, struct fcrypt *fcr,
				struct kernel_crypt_op *kcop)
{
	struct exynos_fmp *fmp;
	struct csession *ses_ptr;
	struct crypt_op *cop = &kcop->cop;
	int ret = -EINVAL;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return -ENODEV;
	}
	fmp = info->fmp;

	if (unlikely(cop->op != COP_ENCRYPT && cop->op != COP_DECRYPT)) {
		dev_err(fmp->dev, "invalid operation op=%u\n", cop->op);
		return -EINVAL;
	}

	/* this also enters ses_ptr->sem */
	ses_ptr = fmp_get_session_by_sid(fcr, cop->ses);
	if (unlikely(!ses_ptr)) {
		dev_err(fmp->dev, "invalid session ID=0x%08X\n", cop->ses);
		return -EINVAL;
	}

	if ((ses_ptr->cdata.init != 0) && (cop->len > PAGE_SIZE)) {
		dev_err(fmp->dev, "Invalid input length. len = %d\n", cop->len);
		ret = -EINVAL;
		goto out_unlock;
	}

	if (ses_ptr->cdata.init != 0) {
		int blocksize = ses_ptr->cdata.blocksize;

		if (unlikely(cop->len % blocksize)) {
			dev_err(fmp->dev,
				"data size (%u) isn't a multiple of block size(%u)\n",
				cop->len, blocksize);
			ret = -EINVAL;
			goto out_unlock;
		}

		if (cop->flags == COP_FLAG_AES_CBC)
			ret = fmp_cipher_set_iv(info->data, kcop->iv, 16);
		else if (cop->flags == COP_FLAG_AES_XTS)
			ret = fmp_cipher_set_iv(info->data,
				(uint8_t *)&cop->data_unit_seqnumber, 16);
		else
			ret = -EINVAL;

		if (unlikely(ret)) {
			dev_err(fmp->dev, "%s: set_iv failed\n", __func__);
			goto out_unlock;
		}
	}

	if (likely(cop->len)) {
		ret = __fmp_run_std(info, ses_ptr, &kcop->cop);
		if (unlikely(ret))
			goto out_unlock;
	}

	if (ses_ptr->hdata.init != 0 &&
		((cop->flags & COP_FLAG_FINAL) ||
		   (!(cop->flags & COP_FLAG_UPDATE) || cop->len == 0))) {
		ret = fmp_fips_hash_final(fmp, &ses_ptr->hdata, kcop->hash_output);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "CryptoAPI failure: %d\n", ret);
			goto out_unlock;
		}
		kcop->digestsize = ses_ptr->hdata.digestsize;
	}

out_unlock:
	fmp_put_session(ses_ptr);
	return ret;
}

static int fmp_run_AES_CBC_MCT(struct fmp_fips_info *info, struct fcrypt *fcr,
			struct kernel_crypt_op *kcop)
{
	struct csession *ses_ptr;
	struct crypt_op *cop = &kcop->cop;
	char **Ct = 0;
	char **Pt = 0;
	int ret = 0, k = 0;
	struct exynos_fmp *fmp;
	char *data = NULL;
	char __user *src, *dst, *secondLast;
	struct scatterlist sg;
	size_t nbytes, bufsize;
	int y = 0;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return -ENODEV;
	}
	fmp = info->fmp;

	if (unlikely(cop->op != COP_ENCRYPT && cop->op != COP_DECRYPT)) {
		dev_err(fmp->dev, "invalid operation op=%u\n", cop->op);
		return -EINVAL;
	}

	/* this also enters ses_ptr->sem */
	ses_ptr = fmp_get_session_by_sid(fcr, cop->ses);
	if (unlikely(!ses_ptr)) {
		dev_err(fmp->dev, "invalid session ID=0x%08X\n", cop->ses);
		return -EINVAL;
	}

	if (cop->len > PAGE_SIZE) {
		dev_err(fmp->dev, "Invalid input length. len = %d\n", cop->len);
		ret = -EINVAL;
		goto out_unlock;
	}

	if (ses_ptr->cdata.init != 0) {
		int blocksize = ses_ptr->cdata.blocksize;

		if (unlikely(cop->len % blocksize)) {
			dev_err(fmp->dev,
				"data size (%u) isn't a multiple of block size(%u)\n",
				cop->len, blocksize);
			ret = -EINVAL;
			goto out_unlock;
		}

		ret = fmp_cipher_set_iv(info->data, kcop->iv, 16);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "%s: set_iv failed\n", __func__);
			goto out_unlock;
		}
	}

	if (!cop->len || !(cop->flags & COP_FLAG_AES_CBC_MCT)) {
		ret = -EINVAL;
		goto out_unlock;
	}

	nbytes = cop->len;
	data = (char *)__get_free_page(GFP_KERNEL);
	if (unlikely(!data)) {
		ret = -ENOMEM;
		goto out_unlock;
	}

	Pt = kzalloc(1000 * sizeof(char *), GFP_KERNEL);
	if (!Pt) {
		ret = -ENOMEM;
		goto out_err_mem_pt;
	}

	for (k = 0; k < 1000; k++) {
		Pt[k] = kzalloc(nbytes, GFP_KERNEL);
		if (!Pt[k]) {
			ret = -ENOMEM;
			goto out_err_mem_ct;
		}
	}

	Ct = kzalloc(1000 * sizeof(char *), GFP_KERNEL);
	if (!Ct) {
		ret = -ENOMEM;
		goto out_err_mem_ct;
	}

	for (k = 0; k < 1000; k++) {
		Ct[k] = kzalloc(nbytes, GFP_KERNEL);
		if (!Ct[k]) {
			ret = -ENOMEM;
			goto out_err_fail;
		}
	}

	bufsize = (nbytes > PAGE_SIZE) ? PAGE_SIZE : nbytes;

	src = cop->src;
	dst = cop->dst;
	secondLast = cop->secondLastEncodedData;

	if (unlikely(copy_from_user(data, src, nbytes))) {
		dev_err(fmp->dev,
			"Error copying %d bytes from user address %p.\n",
			(int)nbytes, src);
		ret = -EFAULT;
		goto out_err_fail;
	}

	sg_init_one(&sg, data, nbytes);
	for (y = 0; y < 1000; y++) {
		memcpy(Pt[y], data, nbytes);
		ret = fmp_n_crypt(info, ses_ptr, cop, &sg, &sg, nbytes);
		memcpy(Ct[y], data, nbytes);

		if (!memcmp(Pt[y], Ct[y], nbytes))
			dev_warn(fmp->dev,
				"plaintext and ciphertext is the same\n");

		if (y == 998) {
			if (unlikely(copy_to_user(secondLast, data, nbytes)))
				dev_err(fmp->dev,
					"unable to copy second last data for AES_CBC_MCT\n");
		}

		if (y == 0)
			memcpy(data, kcop->iv, kcop->ivlen);
		else {
			if (y != 999)
				memcpy(data, Ct[y-1], nbytes);
		}

		if (unlikely(ret)) {
			dev_err(fmp->dev, "fmp_n_crypt failed.\n");
			ret = -EFAULT;
			goto out_err_fail;
		}

		if (cop->op == COP_ENCRYPT)
			ret = fmp_cipher_set_iv(info->data, Ct[y], 16);
		else if (cop->op == COP_DECRYPT)
			ret = fmp_cipher_set_iv(info->data, Pt[y], 16);
		else
			ret = -EINVAL;
		if (unlikely(ret)) {
			dev_err(fmp->dev, "%s: set_iv failed\n", __func__);
			goto out_unlock;
		}
	}

	if (ses_ptr->cdata.init != 0) {
		if (unlikely(copy_to_user(dst, data, nbytes))) {
			dev_err(fmp->dev, "could not copy to user.\n");
			ret = -EFAULT;
			goto out_err_fail;
		}
	}

out_err_fail:
	for (k = 0; k < 1000; k++)
		kzfree(Ct[k]);
	kzfree(Ct);

out_err_mem_ct:
	for (k = 0; k < 1000; k++)
		kzfree(Pt[k]);
	kzfree(Pt);
out_err_mem_pt:
	free_page((unsigned long)data);
out_unlock:
	fmp_put_session(ses_ptr);

	return ret;
}

static int fmp_create_session(struct fmp_fips_info *info,
			struct fcrypt *fcr, struct session_op *sop)
{
	int ret = 0;
	struct exynos_fmp *fmp;
	struct csession *ses_new = NULL, *ses_ptr;
	const char *alg_name = NULL;
	const char *hash_name = NULL;
	int hmac_mode = 1;

	/*
	 * With composite aead ciphers, only ckey is used and it can cover all the
	 * structure space; otherwise both keys may be used simultaneously but they
	 * are confined to their spaces
	 */
	struct {
		uint8_t mkey[FMP_HMAC_MAX_KEY_LEN];
	} keys;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return -ENODEV;
	}
	fmp = info->fmp;

	if (unlikely(!sop->cipher && !sop->mac)) {
		dev_err(fmp->dev, "Both 'cipher' and 'mac' unset.\n");
		return -EINVAL;
	}

	switch (sop->cipher) {
	case 0:
		break;
	case FMP_AES_CBC:
		alg_name = "cbc(aes-fmp)";
		break;
	case FMP_AES_XTS:
		alg_name = "xts(aes-fmp)";
		break;
	default:
		dev_err(fmp->dev, "Invalid cipher : %d\n", sop->cipher);
		return -EINVAL;
	}

	switch (sop->mac) {
	case 0:
		break;
	case FMP_SHA2_256_HMAC:
		hash_name = "hmac-fmp(sha256-fmp)";
		break;
	case FMP_SHA2_256:
		hash_name = "sha256-fmp";
		hmac_mode = 0;
		break;
	default:
		dev_err(fmp->dev, "Invalid mac : %d\n", sop->mac);
		return -EINVAL;
	}

	/* Create a session and put it to the list. */
	ses_new = kzalloc(sizeof(*ses_new), GFP_KERNEL);
	if (!ses_new)
		return -ENOMEM;

	if (!fmp->test_data) {
		dev_err(fmp->dev, "Invalid fips data\n");
		ret = -EINVAL;
		goto error_cipher;
	}

	/* Set-up crypto transform. */
	if (alg_name) {
		uint8_t keyp[FMP_CIPHER_MAX_KEY_LEN];

		if (unlikely(sop->keylen > FMP_CIPHER_MAX_KEY_LEN)) {
			dev_err(fmp->dev, "Setting key failed for %s-%zu.\n",
					alg_name, (size_t)sop->keylen*8);
			ret = -EINVAL;
			goto error_cipher;
		}
		if (unlikely(copy_from_user(keyp, sop->key, sop->keylen))) {
			ret = -EFAULT;
			goto error_cipher;
		}

		if (!strcmp(alg_name, "xts(aes-fmp)"))
			ret = fmp_fips_cipher_init(info, &ses_new->cdata, alg_name,
					keyp, keyp + (sop->keylen >> 1),
					sop->keylen >> 1);
		else
			ret = fmp_fips_cipher_init(info, &ses_new->cdata, alg_name,
					keyp, NULL, sop->keylen);
		if (ret < 0) {
			dev_err(fmp->dev, "%s: Fail to load cipher for %s\n",
					__func__, alg_name);
			ret = -EINVAL;
			goto error_cipher;
		}
	}

	if (hash_name) {
		if (unlikely(sop->mackeylen > FMP_HMAC_MAX_KEY_LEN)) {
			dev_err(fmp->dev, "Setting key failed for %s-%zu.",
				hash_name, (size_t)sop->mackeylen*8);
			ret = -EINVAL;
			goto error_hash;
		}

		if (sop->mackey && unlikely(copy_from_user(keys.mkey, sop->mackey,
					    sop->mackeylen))) {
			ret = -EFAULT;
			goto error_hash;
		}

		ret = fmp_fips_hash_init(info, &ses_new->hdata, hash_name,
							hmac_mode, keys.mkey,
							sop->mackeylen);
		if (ret != 0) {
			dev_err(fmp->dev, "Failed to load hash for %s", hash_name);
			ret = -EINVAL;
			goto error_hash;
		}
	}

	ses_new->alignmask = max(ses_new->cdata.alignmask,
			ses_new->hdata.alignmask);
	ses_new->array_size = DEFAULT_PREALLOC_PAGES;
	ses_new->pages = kzalloc(ses_new->array_size *
			sizeof(struct page *), GFP_KERNEL);
	ses_new->sg = kzalloc(ses_new->array_size *
			sizeof(struct scatterlist), GFP_KERNEL);
	if (ses_new->sg == NULL || ses_new->pages == NULL) {
		dev_err(fmp->dev, "Memory error\n");
		ret = -ENOMEM;
		goto error_hash;
	}

	/* put the new session to the list */
	get_random_bytes(&ses_new->sid, sizeof(ses_new->sid));
	mutex_init(&ses_new->sem);

	mutex_lock(&fcr->sem);
restart:
	list_for_each_entry(ses_ptr, &fcr->list, entry) {
		/* Check for duplicate SID */
		if (unlikely(ses_new->sid == ses_ptr->sid)) {
			get_random_bytes(&ses_new->sid, sizeof(ses_new->sid));
			/*
			 * Unless we have a broken RNG this
			 * shouldn't loop forever... ;-)
			 */
			goto restart;
		}
	}

	list_add(&ses_new->entry, &fcr->list);
	mutex_unlock(&fcr->sem);

	/* Fill in some values for the user. */
	sop->ses = ses_new->sid;

	return 0;

error_hash:
	fmp_fips_cipher_deinit(&ses_new->cdata);
	kzfree(ses_new->sg);
	kzfree(ses_new->pages);
error_cipher:
	kzfree(ses_new);
	return ret;
}

static inline void fmp_destroy_session(struct fmp_fips_info *info, struct csession *ses_ptr)
{
	struct exynos_fmp *fmp;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return;
	}
	fmp = info->fmp;

	if (!mutex_trylock(&ses_ptr->sem)) {
		dev_err(fmp->dev, "Waiting for semaphore of sid=0x%08X\n", ses_ptr->sid);
		mutex_lock(&ses_ptr->sem);
	}
	fmp_fips_cipher_deinit(&ses_ptr->cdata);
	fmp_fips_hash_deinit(&ses_ptr->hdata);
	kzfree(ses_ptr->pages);
	kzfree(ses_ptr->sg);
	mutex_unlock(&ses_ptr->sem);
	mutex_destroy(&ses_ptr->sem);
	kzfree(ses_ptr);
}

/* Look up a session by ID and remove. */
static int fmp_finish_session(struct fmp_fips_info *info, struct fcrypt *fcr, uint32_t sid)
{
	struct exynos_fmp *fmp;
	struct csession *tmp, *ses_ptr;
	struct list_head *head;
	int ret = 0;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&fcr->sem);
	head = &fcr->list;
	fmp = info->fmp;

	list_for_each_entry_safe(ses_ptr, tmp, head, entry) {
		if (ses_ptr->sid == sid) {
			list_del(&ses_ptr->entry);
			fmp_destroy_session(info, ses_ptr);
			break;
		}
	}

	if (unlikely(!ses_ptr)) {
		dev_err(fmp->dev, "%s: Session with sid=0x%08X not found!\n",
				__func__, sid);
		ret = -ENOENT;
	}
	mutex_unlock(&fcr->sem);

	return ret;
}

/* Remove all sessions when closing the file */
static int fmp_finish_all_sessions(struct fmp_fips_info *info, struct fcrypt *fcr)
{
	struct csession *tmp, *ses_ptr;
	struct list_head *head;

	mutex_lock(&fcr->sem);

	head = &fcr->list;
	list_for_each_entry_safe(ses_ptr, tmp, head, entry) {
		list_del(&ses_ptr->entry);
		fmp_destroy_session(info, ses_ptr);
	}
	mutex_unlock(&fcr->sem);

	return 0;
}

/* Look up session by session ID. The returned session is locked. */
struct csession *fmp_get_session_by_sid(struct fcrypt *fcr, uint32_t sid)
{
	struct csession *ses_ptr, *retval = NULL;

	if (unlikely(fcr == NULL))
		return NULL;

	mutex_lock(&fcr->sem);
	list_for_each_entry(ses_ptr, &fcr->list, entry) {
		if (ses_ptr->sid == sid) {
			mutex_lock(&ses_ptr->sem);
			retval = ses_ptr;
			break;
		}
	}
	mutex_unlock(&fcr->sem);

	return retval;
}

static void fmptask_routine(struct work_struct *work)
{
	struct fmp_fips_info *info = container_of(work, struct fmp_fips_info, fmptask);
	struct exynos_fmp *fmp;
	struct todo_list_item *item;
	LIST_HEAD(tmp);

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return;
	}
	fmp = info->fmp;

	/* fetch all pending jobs into the temporary list */
	mutex_lock(&info->todo.lock);
	list_cut_position(&tmp, &info->todo.list, info->todo.list.prev);
	mutex_unlock(&info->todo.lock);

	/* handle each job locklessly */
	list_for_each_entry(item, &tmp, __hook) {
		item->result = fmp_run(info, &info->fcrypt, &item->kcop);
		if (unlikely(item->result))
			dev_err(fmp->dev, "%s: crypto_run() failed: %d\n",
					__func__, item->result);
	}

	/* push all handled jobs to the done list at once */
	mutex_lock(&info->done.lock);
	list_splice_tail(&tmp, &info->done.list);
	mutex_unlock(&info->done.lock);

	/* wake for POLLIN */
	wake_up_interruptible(&info->user_waiter);
}

int fmp_fips_open(struct inode *inode, struct file *file)
{
	int i, ret = 0;
	struct fmp_fips_info *info = NULL;
	struct todo_list_item *tmp;
	struct exynos_fmp *fmp = container_of(file->private_data,
			struct exynos_fmp, miscdev);

	if (!fmp || !fmp->dev) {
		pr_err("%s: Invalid fmp driver\n", __func__);
		return -EINVAL;
	}

	dev_info(fmp->dev, "fmp fips driver name : %s\n", dev_name(fmp->dev));
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		ret = -ENOMEM;
		goto err;
	}
	memset(info, 0, sizeof(*info));

	info->data = fmp_test_init(fmp);
	if (!info->data) {
		dev_err(fmp->dev, "%s: Fail to initialize fips test.\n",
				__func__);
		goto err;
	}

	mutex_init(&info->fcrypt.sem);
	INIT_LIST_HEAD(&info->fcrypt.list);
	INIT_LIST_HEAD(&info->free.list);
	INIT_LIST_HEAD(&info->todo.list);
	INIT_LIST_HEAD(&info->done.list);
	INIT_WORK(&info->fmptask, fmptask_routine);
	mutex_init(&info->free.lock);
	mutex_init(&info->todo.lock);
	mutex_init(&info->done.lock);
	init_waitqueue_head(&info->user_waiter);

	for (i = 0; i < DEF_COP_RINGSIZE; i++) {
		tmp = kzalloc(sizeof(struct todo_list_item), GFP_KERNEL);
		info->itemcount++;
		dev_info(fmp->dev, "%s: allocated new item at %lx\n",
				__func__, (unsigned long)tmp);
		list_add(&tmp->__hook, &info->free.list);
	}

	info->fmp = fmp;
	file->private_data = info;

	dev_info(fmp->dev, "%s opened.\n", dev_name(fmp->dev));
	return ret;
err:
	if (info)
		fmp_test_exit(info->data);
	kfree(info);
	return ret;
}

/* this function has to be called from process context */
static int fill_kcop_from_cop(struct fmp_fips_info *info,
		struct kernel_crypt_op *kcop, struct fcrypt *fcr)
{
	struct exynos_fmp *fmp;
	struct crypt_op *cop = &kcop->cop;
	struct csession *ses_ptr;
	int rc;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return -ENODEV;
	}
	fmp = info->fmp;

	/* this also enters ses_ptr->sem */
	ses_ptr = fmp_get_session_by_sid(fcr, cop->ses);
	if (unlikely(!ses_ptr)) {
		dev_err(fmp->dev, "invalid session ID=0x%08X\n", cop->ses);
		return -EINVAL;
	}
	kcop->digestsize = 0; /* will be updated during operation */

	fmp_put_session(ses_ptr);

	kcop->task = current;
	kcop->mm = current->mm;

	if (cop->iv) {
		kcop->ivlen = ses_ptr->cdata.ivsize;
		rc = copy_from_user(kcop->iv, cop->iv, kcop->ivlen);
		if (unlikely(rc)) {
			dev_err(fmp->dev,
				"error copying IV (%d bytes), copy_from_user returned %d for address %lx\n",
					kcop->ivlen, rc, (unsigned long)cop->iv);
			return -EFAULT;
		}
	} else {
		kcop->ivlen = 0;
	}

	return 0;
}

/* this function has to be called from process context */
static int fill_cop_from_kcop(struct kernel_crypt_op *kcop, struct fcrypt *fcr)
{
	int ret;

	if (kcop->digestsize) {
		ret = copy_to_user(kcop->cop.mac,
				kcop->hash_output, kcop->digestsize);
		if (unlikely(ret))
			return -EFAULT;
	}
	if (kcop->ivlen && kcop->cop.flags & COP_FLAG_WRITE_IV) {
		ret = copy_to_user(kcop->cop.iv,
				kcop->iv, kcop->ivlen);
		if (unlikely(ret))
			return -EFAULT;
	}
	return 0;
}


static int kcop_from_user(struct fmp_fips_info *info, struct kernel_crypt_op *kcop,
		struct fcrypt *fcr, void __user *arg)
{
	if (unlikely(copy_from_user(&kcop->cop, arg, sizeof(kcop->cop))))
		return -EFAULT;

	return fill_kcop_from_cop(info, kcop, fcr);
}

static int kcop_to_user(struct fmp_fips_info *info, struct kernel_crypt_op *kcop,
		struct fcrypt *fcr, void __user *arg)
{
	int ret;
	struct exynos_fmp *fmp;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return -ENODEV;
	}
	fmp = info->fmp;

	ret = fill_cop_from_kcop(kcop, fcr);
	if (unlikely(ret)) {
		dev_err(fmp->dev, "Error in fill_cop_from_kcop\n");
		return ret;
	}

	if (unlikely(copy_to_user(arg, &kcop->cop, sizeof(kcop->cop)))) {
		dev_err(fmp->dev, "Cannot copy to userspace\n");
		return -EFAULT;
	}
	return 0;
}

static int get_session_info(struct fmp_fips_info *info,
		struct fcrypt *fcr, struct session_info_op *siop)
{
	struct csession *ses_ptr;
	struct exynos_fmp *fmp;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return -ENODEV;
	}
	fmp = info->fmp;

	/* this also enters ses_ptr->sem */
	ses_ptr = fmp_get_session_by_sid(fcr, siop->ses);
	if (unlikely(!ses_ptr)) {
		dev_err(fmp->dev, "invalid session ID=0x%08X\n", siop->ses);
		return -EINVAL;
	}

	siop->flags = 0;
	siop->alignmask = ses_ptr->alignmask;
	fmp_put_session(ses_ptr);

	return 0;
}

long fmp_fips_ioctl(struct file *file, unsigned int cmd, unsigned long arg_)
{
	int ret;
	struct session_op sop;
	struct kernel_crypt_op kcop;
	struct fmp_fips_info *info = file->private_data;
	struct exynos_fmp *fmp;
	void __user *arg = (void __user *)arg_;
	struct fcrypt *fcr;
	struct session_info_op siop;
	uint32_t ses;

	if (!info || !info->fmp) {
		pr_err("%s: Invalid fmp info\n", __func__);
		return -ENODEV;
	}
	fmp = info->fmp;
	fcr = &info->fcrypt;

	switch (cmd) {
	case FMPGSESSION:
		if (unlikely(copy_from_user(&sop, arg, sizeof(sop))))
			return -EFAULT;

		ret = fmp_create_session(info, fcr, &sop);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "%s: Fail to create fmp session. ret = %d\n",
					__func__, ret);
			return ret;
		}
		ret = copy_to_user(arg, &sop, sizeof(sop));
		if (unlikely(ret)) {
			fmp_finish_session(info, fcr, sop.ses);
			return -EFAULT;
		}
		return ret;
	case FMPFSESSION:
		ret = get_user(ses, (uint32_t __user *)arg);
		if (unlikely(ret))
			return ret;
		ret = fmp_finish_session(info, fcr, ses);
		if (unlikely(ret))
			dev_err(fmp->dev, "%s: Fail to finish fmp session. ret = %d\n",
					__func__, ret);
		return ret;
	case FMPGSESSIONINFO:
		if (unlikely(copy_from_user(&siop, arg, sizeof(siop))))
			return -EFAULT;
		ret = get_session_info(info, fcr, &siop);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "%s: Fail to get fmp session. ret = %d\n",
					__func__, ret);
			return ret;
		}
		return copy_to_user(arg, &siop, sizeof(siop));
	case FMPCRYPT:
		ret = kcop_from_user(info, &kcop, fcr, arg);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "%s: Error copying from user", __func__);
			return ret;
		}

		if (unlikely(in_fmp_fips_err())) {
			dev_err(fmp->dev, "%s: Fail to run fmp due to fips in error.",
					__func__);
			return -EPERM;
		}

		ret = fmp_run(info, fcr, &kcop);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "%s: Fail to run fmp crypt. ret = %d\n",
				       __func__, ret);
			return ret;
		}
		return kcop_to_user(info, &kcop, fcr, arg);
	case FMP_AES_CBC_MCT:
		ret = kcop_from_user(info, &kcop, fcr, arg);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "%s: Error copying from user", __func__);
			return ret;
		}

		if (unlikely(in_fmp_fips_err())) {
			dev_err(fmp->dev, "%s: Fail to run fmp due to fips in error.",
					__func__);
			return -EPERM;
		}

		ret = fmp_run_AES_CBC_MCT(info, fcr, &kcop);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "%s: Error in fmp_run_AES_CBC_MCT", __func__);
			return ret;
		}
		return kcop_to_user(info, &kcop, fcr, arg);
	default:
		dev_err(fmp->dev, "%s: Invalid command : 0x%x\n", __func__, cmd);
		return -EINVAL;
	}
}

/* compatibility code for 32bit userlands */
#ifdef CONFIG_COMPAT
static inline void compat_to_session_op(struct compat_session_op *compat,
					struct session_op *sop)
{
	sop->cipher = compat->cipher;
	sop->mac = compat->mac;
	sop->keylen = compat->keylen;

	sop->key       = compat_ptr(compat->key);
	sop->mackeylen = compat->mackeylen;
	sop->mackey    = compat_ptr(compat->mackey);
	sop->ses       = compat->ses;
}

static inline void session_op_to_compat(struct session_op *sop,
				struct compat_session_op *compat)
{
	compat->cipher = sop->cipher;
	compat->mac = sop->mac;
	compat->keylen = sop->keylen;

	compat->key       = ptr_to_compat(sop->key);
	compat->mackeylen = sop->mackeylen;
	compat->mackey    = ptr_to_compat(sop->mackey);
	compat->ses       = sop->ses;
}

static inline void compat_to_crypt_op(struct compat_crypt_op *compat,
					struct crypt_op *cop)
{
	cop->ses = compat->ses;
	cop->op = compat->op;
	cop->flags = compat->flags;
	cop->len = compat->len;

	cop->src = compat_ptr(compat->src);
	cop->dst = compat_ptr(compat->dst);
	cop->mac = compat_ptr(compat->mac);
	cop->iv  = compat_ptr(compat->iv);

	cop->data_unit_len = compat->data_unit_len;
	cop->data_unit_seqnumber = compat->data_unit_seqnumber;

	cop->secondLastEncodedData = compat_ptr(compat->secondLastEncodedData);
	cop->thirdLastEncodedData = compat_ptr(compat->thirdLastEncodedData);
}

static inline void crypt_op_to_compat(struct crypt_op *cop,
				struct compat_crypt_op *compat)
{
	compat->ses = cop->ses;
	compat->op = cop->op;
	compat->flags = cop->flags;
	compat->len = cop->len;

	compat->src = ptr_to_compat(cop->src);
	compat->dst = ptr_to_compat(cop->dst);
	compat->mac = ptr_to_compat(cop->mac);
	compat->iv  = ptr_to_compat(cop->iv);

	compat->data_unit_len = cop->data_unit_len;
	compat->data_unit_seqnumber = cop->data_unit_seqnumber;

	compat->secondLastEncodedData  = ptr_to_compat(cop->secondLastEncodedData);
	compat->thirdLastEncodedData  = ptr_to_compat(cop->thirdLastEncodedData);
}

static int compat_kcop_from_user(struct fmp_fips_info *info,
		struct kernel_crypt_op *kcop,
		struct fcrypt *fcr, void __user *arg)
{
	struct compat_crypt_op compat_cop;

	if (unlikely(copy_from_user(&compat_cop, arg, sizeof(compat_cop))))
		return -EFAULT;
	compat_to_crypt_op(&compat_cop, &kcop->cop);

	return fill_kcop_from_cop(info, kcop, fcr);
}

static int compat_kcop_to_user(struct fmp_fips_info *info,
		struct kernel_crypt_op *kcop,
		struct fcrypt *fcr, void __user *arg)
{
	int ret;
	struct exynos_fmp *fmp;
	struct compat_crypt_op compat_cop;

	if (!info || !info->fmp) {
		pr_err("%s: fmp info is already free.\n", __func__);
		return 0;
	}
	fmp = info->fmp;

	ret = fill_cop_from_kcop(kcop, fcr);
	if (unlikely(ret)) {
		dev_err(fmp->dev, "Error in fill_cop_from_kcop");
		return ret;
	}
	crypt_op_to_compat(&kcop->cop, &compat_cop);

	if (unlikely(copy_to_user(arg, &compat_cop, sizeof(compat_cop)))) {
		dev_err(fmp->dev, "Error copying to user");
		return -EFAULT;
	}
	return 0;
}

long fmp_fips_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg_)
{
	int ret;
	struct session_op sop;
	struct compat_session_op compat_sop;
	struct kernel_crypt_op kcop;
	struct fmp_fips_info *info = file->private_data;
	struct exynos_fmp *fmp;
	void __user *arg = (void __user *)arg_;
	struct fcrypt *fcr;

	if (!info || !info->fmp) {
		pr_err("%s: fmp info is already free.\n", __func__);
		return 0;
	}
	fmp = info->fmp;

	fcr = &info->fcrypt;

	switch (cmd) {
	case FMPFSESSION:
	case FMPGSESSIONINFO:
		return fmp_fips_ioctl(file, cmd, arg_);

	case COMPAT_FMPGSESSION:
		if (unlikely(copy_from_user(&compat_sop, arg, sizeof(compat_sop))))
			return -EFAULT;
		compat_to_session_op(&compat_sop, &sop);

		ret = fmp_create_session(info, fcr, &sop);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "Fail to create fmp session. ret = %d\n", ret);
			return ret;
		}

		session_op_to_compat(&sop, &compat_sop);
		ret = copy_to_user(arg, &compat_sop, sizeof(compat_sop));
		if (unlikely(ret)) {
			fmp_finish_session(info, fcr, sop.ses);
			return -EFAULT;
		}
		return ret;
	case COMPAT_FMPCRYPT:
		ret = compat_kcop_from_user(info, &kcop, fcr, arg);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "Error copying from user");
			return ret;
		}

		if (unlikely(in_fmp_fips_err())) {
			dev_err(fmp->dev, "Fail to run fmp due to fips in error.");
			return -EPERM;
		}

		ret = fmp_run(info, fcr, &kcop);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "Fail to run fm)p crypt. ret = %d\n", ret);
			return ret;
		}
		return compat_kcop_to_user(info, &kcop, fcr, arg);
	case COMPAT_FMP_AES_CBC_MCT:
		ret = compat_kcop_from_user(info, &kcop, fcr, arg);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "Error copying from user");
			return ret;
		}

		if (unlikely(in_fmp_fips_err())) {
			dev_err(fmp->dev, "Fail to run fmp due to fips in error.");
			return -EPERM;
		}

		ret = fmp_run_AES_CBC_MCT(info, fcr, &kcop);
		if (unlikely(ret)) {
			dev_err(fmp->dev, "Error in fmp_run_AES_CBC_MCT");
			return ret;
		}
		return compat_kcop_to_user(info, &kcop, fcr, arg);
	default:
		dev_err(fmp->dev, "Invalid command : 0x%x\n", cmd);
		return -EINVAL;
	}
}
#endif /* CONFIG_COMPAT */

int fmp_fips_release(struct inode *inode, struct file *file)
{
	struct fmp_fips_info *info = file->private_data;
	struct exynos_fmp *fmp;
	struct todo_list_item *item, *item_safe;
	int items_freed = 0;

	if (!info || !info->fmp) {
		pr_err("%s: fmp info is already free.\n", __func__);
		return 0;
	}
	fmp = info->fmp;

	cancel_work_sync(&info->fmptask);

	mutex_destroy(&info->todo.lock);
	mutex_destroy(&info->done.lock);
	mutex_destroy(&info->free.lock);

	list_splice_tail(&info->todo.list, &info->free.list);
	list_splice_tail(&info->done.list, &info->free.list);

	list_for_each_entry_safe(item, item_safe, &info->free.list, __hook) {
		dev_err(fmp->dev, "%s: freeing item at %lx\n",
				__func__, (unsigned long)item);
		list_del(&item->__hook);
		kzfree(item);
		items_freed++;
	}
	if (items_freed != info->itemcount)
		dev_err(fmp->dev, "%s: freed %d items, but %d should exist!\n",
				__func__, items_freed, info->itemcount);

	fmp_finish_all_sessions(info, &info->fcrypt);
	fmp_test_exit(info->data);
	kzfree(info);
	file->private_data = NULL;
	dev_info(fmp->dev, "%s released.\n", dev_name(fmp->dev));

	return 0;
}
