/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 * Author: Boojin Kim <boojin.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <crypto/diskcipher.h>
#include <crypto/fmp.h>
#include "ufshcd.h"
#include "ufs-exynos.h"

/* smu functions */
#if defined(CONFIG_SCSI_UFS_EXYNOS_SMU)
static void exynos_ufs_smu_entry0_init(struct exynos_ufs *ufs)
{
	writel(0x0, ufs->reg_ufsp + UFSPSBEGIN0);
	writel(0xffffffff, ufs->reg_ufsp + UFSPSEND0);
	writel(0xff, ufs->reg_ufsp + UFSPSLUN0);
	writel(0xf1, ufs->reg_ufsp + UFSPSCTRL0);
}

int exynos_ufs_smu_init(struct exynos_ufs *ufs)
{
	if (!ufs)
		return 0;

	if (ufs->smu == SMU_ID_MAX) {
		exynos_ufs_smu_entry0_init(ufs);
		return 0;
	}

	dev_info(ufs->dev, "%s with id:%d\n", __func__, ufs->smu);
	return exynos_smu_init(ufs->smu, SMU_INIT);
}

int exynos_ufs_smu_resume(struct exynos_ufs *ufs)
{
	int fmp_id;

	if (!ufs)
		return 0;

	if (ufs->smu < SMU_ID_MAX)
		fmp_id = ufs->smu;
	else if (ufs->fmp < SMU_ID_MAX)
		fmp_id = ufs->fmp;
	else {
		exynos_ufs_smu_entry0_init(ufs);
		return 0;
	}

	dev_info(ufs->dev, "%s with id:%d\n", __func__, fmp_id);
	return exynos_smu_resume(fmp_id);
}

int exynos_ufs_smu_abort(struct exynos_ufs *ufs)
{
	if (!ufs || (ufs->smu == SMU_ID_MAX))
		return 0;

	dev_info(ufs->dev, "%s with id:%d\n", __func__, ufs->smu);
	return exynos_smu_abort(ufs->smu, SMU_ABORT);
}
#endif
/* fmp functions */
#ifdef CONFIG_SCSI_UFS_EXYNOS_FMP
int exynos_ufs_fmp_sec_cfg(struct exynos_ufs *ufs)
{
	if (!ufs || (ufs->fmp == SMU_ID_MAX))
		return 0;

	if (ufs->fmp != SMU_EMBEDDED)
		dev_err(ufs->dev, "%s is fails id:%d\n",
				__func__, ufs->fmp);


	dev_info(ufs->dev, "%s with id:%d\n", __func__, ufs->fmp);
	return exynos_fmp_sec_config(ufs->fmp);
}

static struct bio *get_bio(struct ufs_hba *hba, struct ufshcd_lrb *lrbp)
{
	struct exynos_ufs *ufs;

	if (!hba || !lrbp) {
		pr_err("%s: Invalid MMC:%p data:%p\n", __func__, hba, lrbp);
		return NULL;
	}

	ufs = dev_get_platdata(hba->dev);
	if (ufs->fmp == SMU_ID_MAX)
		return NULL;

	if (!virt_addr_valid(lrbp->cmd)) {
		dev_err(hba->dev, "Invalid cmd:%p\n", lrbp->cmd);
		return NULL;
	}

	if (!virt_addr_valid(lrbp->cmd->request->bio)) {
		if (lrbp->cmd->request->bio)
			dev_err(hba->dev, "Invalid bio:%p\n", lrbp->cmd->request->bio);
		return NULL;
	}
	else
		return lrbp->cmd->request->bio;
}

int exynos_ufs_fmp_cfg(struct ufs_hba *hba,
		       struct ufshcd_lrb *lrbp,
		       struct scatterlist *sg,
		       uint32_t index, int sector_offset, int page_index)
{
	struct fmp_request req;
	struct crypto_diskcipher *dtfm;
	u64 iv;
	struct bio *bio = get_bio(hba, lrbp);

	if (!bio)
		return 0;

	dtfm = crypto_diskcipher_get(bio);
	if (unlikely(IS_ERR(dtfm))) {
		pr_warn("%s: fails to get crypt\n", __func__);
		return -EINVAL;
	} else if (dtfm) {
#ifdef CONFIG_CRYPTO_DISKCIPHER_DUN
		if (bio_dun(bio))
			iv = bio_dun(bio) + page_index;
		else
			iv = bio->bi_iter.bi_sector + (sector_t) sector_offset;
#else
		iv = bio->bi_iter.bi_sector + (sector_t) sector_offset;
#endif

		req.table = (void *)&lrbp->ucd_prdt_ptr[index];
		req.cmdq_enabled = 0;
		req.iv = &iv;
		req.ivsize = sizeof(iv);
#ifdef CONFIG_EXYNOS_FMP_FIPS
		/* check fips flag. use fmp without diskcipher */
		if (!dtfm->algo) {
			if (exynos_fmp_crypt((void *)dtfm, &req))
			    pr_warn("%s: fails to test fips\n", __func__);
			return 0;
		}
#endif
		if (crypto_diskcipher_set_crypt(dtfm, &req)) {
			pr_warn("%s: fails to set crypt\n", __func__);
			return -EINVAL;
		}
		return 0;
	}

	exynos_fmp_bypass(&lrbp->ucd_prdt_ptr[index], 0);
	return 0;
}

int exynos_ufs_fmp_clear(struct ufs_hba *hba, struct ufshcd_lrb *lrbp)
{
	int ret = 0;
	int sg_segments, idx;
	struct scatterlist *sg;
	struct ufshcd_sg_entry *prd_table;
	struct crypto_diskcipher *dtfm;
	struct fmp_crypto_info *ci;
	struct fmp_request req;
	struct bio *bio = get_bio(hba, lrbp);

	if (!bio)
		return 0;

	sg_segments = scsi_sg_count(lrbp->cmd);
	if (!sg_segments)
		return 0;

	dtfm = crypto_diskcipher_get(bio);
	if (dtfm) {
#ifdef CONFIG_EXYNOS_FMP_FIPS
		/* check fips flag. use fmp without diskcipher */
		if (!dtfm->algo) {
			prd_table =
				(struct ufshcd_sg_entry *)lrbp->ucd_prdt_ptr;
			scsi_for_each_sg(lrbp->cmd, sg, sg_segments, idx) {
				req.table = (void *)&prd_table[idx];
				ret = exynos_fmp_clear((void *)dtfm, &req);
				if (ret) {
					pr_warn("%s: fails to clear fips\n",
						__func__);
					return 0;
				}
			}
			return 0;
		}
#endif
		/* clear key on descrptor */
		ci = crypto_tfm_ctx(crypto_diskcipher_tfm(dtfm));
		if (ci && (ci->enc_mode == EXYNOS_FMP_FILE_ENC)) {
			prd_table =
			    (struct ufshcd_sg_entry *)lrbp->ucd_prdt_ptr;
			scsi_for_each_sg(lrbp->cmd, sg, sg_segments, idx) {
				req.table = (void *)&prd_table[idx];
				ret = crypto_diskcipher_clear_crypt(dtfm, &req);
				if (ret) {
					pr_err("%s: fail to clear desc (%d)\n",
						__func__, ret);
					return 0;
				}
			}
		}
	}
	return 0;
}
#endif
