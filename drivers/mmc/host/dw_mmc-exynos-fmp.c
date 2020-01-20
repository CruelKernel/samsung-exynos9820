/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/semaphore.h>
#include <linux/blkdev.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/mmc/dw_mmc.h>
#include <linux/mmc/mmc.h>

#include "dw_mmc.h"
#include "dw_mmc-exynos.h"
#include "../card/queue.h"

static int check_data_equal(void *data1, void *data2)
{
	return data1 == data2;
}

static int is_valid_bio_data(struct bio *bio)
{
	if (bio->fmp_ci.private_enc_mode < 0 ||
			bio->fmp_ci.private_enc_mode > EXYNOS_FMP_DISK_ENC)
		return false;

	if (bio->fmp_ci.private_algo_mode < 0 ||
			bio->fmp_ci.private_algo_mode > EXYNOS_FMP_ALGO_MODE_AES_XTS)
		return false;

	return true;
}

static struct bio *is_get_bio(struct mmc_data *data, bool cmdq_enabled)
{
	struct bio *bio = NULL;

	if (!data) {
		pr_err("%s: Invalid MMC data\n", __func__);
		return NULL;
	}

	if (cmdq_enabled) {
#if 0 /* CMDQ is not implemented on the current kernel */
		struct mmc_cmdq_req *cmdq_req;
		struct mmc_request *mrq;

		cmdq_req = container_of(data, struct mmc_cmdq_req, data);
		if (!cmdq_req)
			return NULL;

		mrq = &cmdq_req->mrq;
		if (!mrq || !mrq->req || !mrq->req->bio)
			return NULL;

		bio = mrq->req->bio;
#endif
	} else {
		struct mmc_queue_req *mq_rq;
		struct mmc_blk_request *brq;

		brq = container_of(data, struct mmc_blk_request, data);
		if (!brq)
			return NULL;

		mq_rq = container_of(brq, struct mmc_queue_req, brq);
		if (!mq_rq || !mq_rq->req || !mq_rq->req->bio)
			return NULL;
		bio = mq_rq->req->bio;

		if (!virt_addr_valid(bio))
			return NULL;
	}

	return bio;
}

static int is_mmc_fmp_test_enabled(struct mmc_data *mmc_data,
				struct platform_device *pdev,
				bool cmdq_enabled)
{
	struct bio *bio;
	struct exynos_fmp *fmp = dev_get_drvdata(&pdev->dev);

	if (!fmp)
		return FALSE;

	bio = is_get_bio(mmc_data, cmdq_enabled);
	if (!bio) {
		fmp->test_mode = 0;
		return FALSE;
	}

	if (check_data_equal((void *)bio->bi_private, (void *)fmp->test_bh)
			&& (uint64_t)fmp->test_bh) {
		fmp->test_mode = 1;
		return TRUE;
	}

	fmp->test_mode = 0;
	return FALSE;
}

static int exynos_mmc_fmp_key_size_cfg(struct fmp_crypto_setting *crypto,
					uint32_t key_size)
{
	int ret = 0;
	uint32_t size;

	if (!crypto || !key_size) {
		pr_err("%s: Invalid fmp data or size.\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	size = key_size;
	if (crypto->algo_mode == EXYNOS_FMP_ALGO_MODE_AES_XTS)
		size = size >> 1;

	switch (size) {
	case FMP_KEY_SIZE_16:
		crypto->key_size = EXYNOS_FMP_KEY_SIZE_16;
		break;
	case FMP_KEY_SIZE_32:
		crypto->key_size = EXYNOS_FMP_KEY_SIZE_32;
		break;
	default:
		pr_err("%s: FMP doesn't support key size %d\n", __func__, size);
		ret = -EINVAL;
		goto out;
	}
out:
	return ret;
}

static int exynos_mmc_fmp_iv_cfg(struct fmp_crypto_setting *crypto,
					sector_t sector,
					int sector_offset,
					pgoff_t page_index)
{
	int ret = 0;

	if (!crypto) {
		pr_err("%s: Invalid fmp data\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	crypto->index = page_index;
	crypto->sector = sector + sector_offset;
out:
	return ret;
}

static int exynos_mmc_fmp_key_cfg(struct fmp_crypto_setting *crypto,
				unsigned char *key, unsigned long key_length)
{
	int ret = 0;

	if (!crypto) {
		pr_err("%s: Invalid fmp data\n", __func__);
		ret = -EINVAL;
		goto out;
	}
	memset(crypto->key, 0, FMP_MAX_KEY_SIZE);
	memcpy(crypto->key, key, key_length);
out:
	return ret;
}

static int exynos_mmc_fmp_disk_cfg(struct mmc_data *mmc_data,
				struct fmp_crypto_setting *crypto,
				int sector_offset, bool cmdq_enabled)
{
	int ret = 0;
	struct bio *bio;

	if (!crypto) {
		pr_err("%s: Invalid fmp data\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	memset(crypto, 0, sizeof(struct fmp_crypto_setting));

	crypto->enc_mode = EXYNOS_FMP_DISK_ENC;

	bio = is_get_bio(mmc_data, cmdq_enabled);
	if (!bio)
		goto bypass_out;

	if ((bio->fmp_ci.private_algo_mode == EXYNOS_FMP_BYPASS_MODE) ||
			/* direct IO case */
			(bio->fmp_ci.private_enc_mode == EXYNOS_FMP_FILE_ENC))
		goto bypass_out;

	if (!is_valid_bio_data(bio))
		goto bypass_out;

	if (!bio->fmp_ci.key) {
		pr_err("%s: Invalid disk key\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	crypto->algo_mode = bio->fmp_ci.private_algo_mode;
	ret = exynos_mmc_fmp_key_size_cfg(crypto, bio->fmp_ci.key_length);
	if (ret)
		goto bypass_out;

	ret = exynos_mmc_fmp_iv_cfg(crypto, bio->bi_iter.bi_sector,
					sector_offset, 0);
	if (ret) {
		pr_err("%s: Fail to configure fmp iv. ret(%d)\n",
				__func__, ret);
		ret = -EINVAL;
		goto err;
	}
err:
	return ret;
bypass_out:
	crypto->algo_mode = EXYNOS_FMP_BYPASS_MODE;
	ret = 0;

	return ret;
}

static int exynos_mmc_fmp_direct_io_cfg(struct mmc_data *mmc_data,
				struct fmp_crypto_setting *crypto,
				int sector_offset, bool cmdq_enabled)
{
	int ret = 0;
	struct bio *bio;

	if (!crypto) {
		pr_err("%s: Invalid fmp data\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	memset(crypto, 0, sizeof(struct fmp_crypto_setting));

	crypto->enc_mode = EXYNOS_FMP_FILE_ENC;

	bio = is_get_bio(mmc_data, cmdq_enabled);
	if (!bio || (bio->fmp_ci.private_algo_mode == EXYNOS_FMP_BYPASS_MODE))
		goto bypass_out;

	if (!is_valid_bio_data(bio))
		goto bypass_out;

	crypto->algo_mode = bio->fmp_ci.private_algo_mode;
	ret = exynos_mmc_fmp_key_size_cfg(crypto, bio->fmp_ci.key_length);
	if (ret)
		goto bypass_out;

	ret = exynos_mmc_fmp_iv_cfg(crypto, bio->bi_iter.bi_sector,
					sector_offset, 0);
	if (ret) {
		pr_err("%s: Fail to configure fmp iv. ret(%d)\n",
				__func__, ret);
		ret = -EINVAL;
		goto err;
	}

	ret = exynos_mmc_fmp_key_cfg(crypto, bio->fmp_ci.key,
					bio->fmp_ci.key_length);
	if (ret) {
		pr_err("%s: Fail to configure fmp key. ret(%d)\n",
				__func__, ret);
		ret = -EINVAL;
		goto err;
	}
err:
	return ret;
bypass_out:
	crypto->algo_mode = EXYNOS_FMP_BYPASS_MODE;
	ret = 0;
	return ret;
}

static int exynos_mmc_fmp_file_cfg(struct mmc_data *mmc_data,
				struct page *page,
				struct fmp_crypto_setting *crypto,
				int sector_offset, bool cmdq_enabled)
{
	int ret = 0;
	struct bio *bio;
	pgoff_t page_index;
	struct _fmp_ci *ci;

	if (!crypto) {
		pr_err("%s: Invalid fmp data\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	memset(crypto, 0, sizeof(struct fmp_crypto_setting));

	crypto->enc_mode = EXYNOS_FMP_FILE_ENC;

	if (!page || !page->mapping || PageAnon(page))
		goto bypass_out;

	bio = is_get_bio(mmc_data, cmdq_enabled);
	if (!bio || !virt_addr_valid(bio))
		goto bypass_out;

	ci = &page->mapping->fmp_ci;
	if (ci->private_algo_mode == EXYNOS_FMP_BYPASS_MODE)
		goto bypass_out;

	crypto->algo_mode = ci->private_algo_mode;
	ret = exynos_mmc_fmp_key_size_cfg(crypto, ci->key_length);
	if (ret)
		goto bypass_out;

	ret = exynos_mmc_fmp_iv_cfg(crypto, bio->bi_iter.bi_sector,
					sector_offset, page_index);
	if (ret) {
		pr_err("%s: Fail to configure fmp iv. ret(%d)\n",
				__func__, ret);
		ret = -EINVAL;
		goto err;
	}

	ret = exynos_mmc_fmp_key_cfg(crypto, ci->key, ci->key_length);
	if (ret) {
		pr_err("%s: Fail to configure fmp key. ret(%d)\n",
				__func__, ret);
		ret = -EINVAL;
		goto err;
	}
err:
	return ret;
bypass_out:
	crypto->algo_mode = EXYNOS_FMP_BYPASS_MODE;
	ret = 0;
	return ret;
}

int exynos_mmc_fmp_host_set_device(struct platform_device *host_pdev,
				struct platform_device *pdev,
				struct exynos_fmp_variant_ops *fmp_vops)
{
	struct dw_mci *host;
	struct dw_mci_exynos_priv_data *priv;

	if (!host_pdev || !pdev || !fmp_vops) {
		pr_err("%s: Fail to set device for fmp host\n", __func__);
		return -EINVAL;
	}

	host = dev_get_drvdata(&host_pdev->dev);
	if (!host) {
		pr_err("%s: Invalid Host device\n", __func__);
		return -ENODEV;
	}

	priv = host->priv;
	if (!priv) {
		pr_err("%s: Invalid Host device private data\n", __func__);
		return -ENODEV;
	}

	priv->fmp.pdev = pdev;
	priv->fmp.vops = fmp_vops;

	return 0;
}
EXPORT_SYMBOL(exynos_mmc_fmp_host_set_device);

static inline void exynos_mmc_fmp_bypass(void *desc, bool cmdq_enabled)
{
	if (cmdq_enabled) {
		SET_CMDQ_FAS((struct fmp_table_setting *)desc, 0);
		SET_CMDQ_DAS((struct fmp_table_setting *)desc, 0);
	} else {
		SET_FAS((struct fmp_table_setting *)desc, 0);
		SET_DAS((struct fmp_table_setting *)desc, 0);
	}
	return;
}

int exynos_mmc_fmp_cfg(struct dw_mci *host,
				void *desc,
				struct mmc_data *mmc_data,
				struct page *page,
				int sector_offset,
				bool cmdq_enabled)
{
	int ret;
	struct fmp_data_setting data;
	struct dw_mci_exynos_priv_data *priv;

	if (!host) {
		pr_err("%s: Invalid Host device\n", __func__);
		return -ENODEV;
	}

	priv = host->priv;
	if (!priv || !priv->fmp.pdev) {
		exynos_mmc_fmp_bypass(desc, cmdq_enabled);
		return 0;
	}

	ret = is_mmc_fmp_test_enabled(mmc_data, priv->fmp.pdev, cmdq_enabled);
	if (ret == TRUE)
		goto out_test;

	ret = exynos_mmc_fmp_disk_cfg(mmc_data, &data.disk, sector_offset, cmdq_enabled);
	if (ret) {
		pr_err("%s: Fail to configure FMP Disk Encryption. ret(%d)\n",
				__func__, ret);
		return -EINVAL;
	}

	if (data.disk.algo_mode != EXYNOS_FMP_BYPASS_MODE)
		goto file_cfg;

	ret = exynos_mmc_fmp_direct_io_cfg(mmc_data, &data.file,
			sector_offset, cmdq_enabled);
	if (ret) {
		pr_err("%s: Fail to configure FMP direct IO File Encryption. ret(%d)\n",
				__func__, ret);
		return -EINVAL;
	}

	if (data.file.algo_mode != EXYNOS_FMP_BYPASS_MODE)
		goto out;

file_cfg:
	ret = exynos_mmc_fmp_file_cfg(mmc_data, page, &data.file,
			sector_offset, cmdq_enabled);
	if (ret) {
		pr_err("%s: Fail to configure FMP File Encryption. ret(%d)\n",
				__func__, ret);
		return -EINVAL;
	}

out:
	data.mapping = page->mapping;
out_test:
	data.table = (struct fmp_table_setting *)desc;
	data.cmdq_enabled = cmdq_enabled;
	return priv->fmp.vops->config(priv->fmp.pdev, &data);
}
EXPORT_SYMBOL(exynos_mmc_fmp_cfg);

int exynos_mmc_fmp_clear(struct dw_mci *host, void *desc, bool cmdq_enabled)
{
	int ret = 0;
	struct dw_mci_exynos_priv_data *priv;
	struct fmp_data_setting data;

	if (!host) {
		pr_err("%s: Invalid Host device\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	priv = host->priv;
	if (!priv || !priv->fmp.pdev) {
		ret = 0;
		goto err;
	}

	data.table = (struct fmp_table_setting *)desc;
	if (cmdq_enabled) {
		if (!GET_CMDQ_FAS(data.table))
			return ret;
	} else {
		if (!GET_FAS(data.table))
			return ret;
	}

	ret = priv->fmp.vops->clear(priv->fmp.pdev, &data);
	if (ret) {
		pr_err("%s: Fail to clear FMP desc (%d)\n",
			__func__, ret);
		ret = -EINVAL;
		goto err;
	}
err:
	return ret;
}

