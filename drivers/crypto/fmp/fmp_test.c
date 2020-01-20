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

#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/crypto.h>
#include <linux/buffer_head.h>
#include <linux/genhd.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/blk_types.h>
#include <crypto/fmp.h>

#include "fmp_test.h"

#define MAX_SCAN_PART	(50)
#define MAX_RETRY_COUNT (0x100000)

static dev_t find_devt_for_test(struct exynos_fmp *fmp,
				struct fmp_test_data *data)
{
	int i, idx = 0;
	uint32_t count = 0;
	uint64_t size;
	uint64_t size_list[MAX_SCAN_PART];
	dev_t devt_list[MAX_SCAN_PART];
	dev_t devt_scan, devt;
	struct block_device *bdev;
	struct device *dev = fmp->dev;
	fmode_t fmode = FMODE_WRITE | FMODE_READ;

	memset(size_list, 0, sizeof(size_list));
	memset(devt_list, 0, sizeof(devt_list));
	do {
		for (i = 1; i < MAX_SCAN_PART; i++) {
			devt_scan = blk_lookup_devt(data->block_type, i);
			bdev = blkdev_get_by_dev(devt_scan, fmode, NULL);
			if (IS_ERR(bdev))
				continue;
			else {
				size_list[idx] =
				    (uint64_t) i_size_read(bdev->bd_inode);
				devt_list[idx++] = devt_scan;
				blkdev_put(bdev, fmode);
			}
		}
		if (!idx) {
			mdelay(100);
			count++;
			continue;
		}
		for (i = 0; i < idx; i++) {
			if (i == 0) {
				size = size_list[i];
				devt = devt_list[i];
			} else {
				if (size < size_list[i])
					devt = devt_list[i];
			}
		}
		bdev = blkdev_get_by_dev(devt, fmode, NULL);
		dev_dbg(dev, "Found partno %d for FMP test\n",
			bdev->bd_part->partno);
		blkdev_put(bdev, fmode);
		return devt;
	} while (count < MAX_RETRY_COUNT);
	dev_err(dev, "Block device isn't initialized yet for FMP test\n");
	return (dev_t) 0;
}

static int get_fmp_host_type(struct device *dev,
				    struct fmp_test_data *data)
{
	int ret;
	struct device_node *node = dev->of_node;
	const char *type;

	ret =
	    of_property_read_string_index(node, "exynos,block-type", 0, &type);
	if (ret) {
		pr_err("%s: Could not get block type\n", __func__);
		return ret;
	}
	strlcpy(data->block_type, type, FMP_BLOCK_TYPE_NAME_LEN);
	return 0;
}

static int get_fmp_test_block_offset(struct device *dev,
				      struct fmp_test_data *data)
{
	int ret = 0;
	struct device_node *node = dev->of_node;
	uint32_t offset;

	ret = of_property_read_u32(node, "exynos,fips-block_offset", &offset);
	if (ret) {
		pr_err("%s: Could not get fips test block offset\n", __func__);
		ret = -EINVAL;
		goto err;
	}
	data->test_block_offset = offset;
err:
	return ret;
}

/* test block device init for fmp test */
struct fmp_test_data *fmp_test_init(struct exynos_fmp *fmp)
{
	int ret = 0;
	struct fmp_test_data *data;
	struct device *dev;
	struct inode *inode;
	struct super_block *sb;
	unsigned long blocksize;
	unsigned char blocksize_bits;
	fmode_t fmode = FMODE_WRITE | FMODE_READ;

	if (!fmp) {
		pr_err("%s: Invalid exynos fmp struct\n", __func__);
		return NULL;
	}

	dev = fmp->dev;
	data = kmalloc(sizeof(struct fmp_test_data), GFP_KERNEL);
	if (!data)
		return NULL;

	ret = get_fmp_host_type(dev, data);
	if (ret) {
		dev_err(dev, "%s: Fail to get host type. ret(%d)", __func__,
			ret);
		goto err;
	}
	data->devt = find_devt_for_test(fmp, data);
	if (!data->devt) {
		dev_err(dev, "%s: Fail to find devt for self test\n", __func__);
		goto err;
	}
	data->bdev = blkdev_get_by_dev(data->devt, fmode, NULL);
	if (IS_ERR(data->bdev)) {
		dev_err(dev, "%s: Fail to open block device\n", __func__);
		goto err;
	}
	ret = get_fmp_test_block_offset(dev, data);
	if (ret) {
		dev_err(dev, "%s: Fail to get fips offset. ret(%d)\n",
			__func__, ret);
		goto err;
	}
	inode = data->bdev->bd_inode;
	sb = inode->i_sb;
	blocksize = sb->s_blocksize;
	blocksize_bits = sb->s_blocksize_bits;
	data->sector =
	    (i_size_read(inode) -
	     (blocksize * data->test_block_offset)) >> blocksize_bits;

	return data;
err:
	kzfree(data);
	return NULL;
}

int fmp_cipher_run(struct exynos_fmp *fmp, struct fmp_test_data *fdata,
		uint8_t *data, uint32_t len, bool bypass, uint32_t write,
		void *priv, struct fmp_crypto_info *ci)
{
	int ret = 0;
	struct device *dev;
	static struct buffer_head *bh;
	u32 org_algo_mode;
	int op_flags;

	if (!fmp || !fdata || !ci) {
		pr_err("%s: Invalid fmp struct: %p, %p, %p\n",
			__func__, fmp, fdata, ci);
		return -EINVAL;
	}
	dev = fmp->dev;

	bh = __getblk(fdata->bdev, fdata->sector, FMP_BLK_SIZE);
	if (!bh) {
		dev_err(dev, "%s: Fail to get block from bdev\n", __func__);
		return -ENODEV;
	}

	/* set algo_mode for test */
	org_algo_mode = ci->algo_mode;
	if (bypass)
		ci->algo_mode = EXYNOS_FMP_BYPASS_MODE;
	ci->algo_mode |= EXYNOS_FMP_ALGO_MODE_TEST;

	get_bh(bh);
	/* priv is diskc for crypto test. */
	if (!priv) {
		/* ci is fmp_test_data->ci */
		fmp->test_data = fdata;
		ci->ctx = fmp;
		ci->use_diskc = 0;
		ci->enc_mode = EXYNOS_FMP_FILE_ENC;
		bh->b_private = ci;
	} else {
		/* ci is crypto_tfm_ctx(tfm) */
		bh->b_private = priv;
	}
	op_flags = REQ_CRYPT;

	if (write == WRITE_MODE) {
		memcpy(bh->b_data, data, len);
		set_buffer_dirty(bh);
		ret = __sync_dirty_buffer(bh, op_flags | REQ_SYNC);
		if (ret) {
			dev_err(dev, "%s: IO error syncing for write mode\n",
				__func__);
			ret = -EIO;
			goto out;
		}
		memset(bh->b_data, 0, FMP_BLK_SIZE);
	} else {
		lock_buffer(bh);
		bh->b_end_io = end_buffer_read_sync;
		submit_bh(REQ_OP_READ, REQ_SYNC | REQ_PRIO | op_flags, bh);
		wait_on_buffer(bh);
		if (unlikely(!buffer_uptodate(bh))) {
			ret = -EIO;
			goto out;
		}
		memcpy(data, bh->b_data, len);
	}
out:
	if (ci)
		ci->algo_mode = org_algo_mode;
	put_bh(bh);
	return ret;
}

int fmp_test_crypt(struct exynos_fmp *fmp, struct fmp_test_data *fdata,
		uint8_t *src, uint8_t *dst, uint32_t len, uint32_t enc,
		void *priv, struct fmp_crypto_info *ci)
{
	int ret = 0;

	if (!fdata) {
		pr_err("%s: Invalid exynos fmp struct\n", __func__);
		return -1;
	}

	if (enc == ENCRYPT) {
		ret = fmp_cipher_run(fmp, fdata, src, len, 0,
				WRITE_MODE, priv, ci);
		if (ret) {
			pr_err("Fail to run fmp cipher ret(%d)\n",
				ret);
			goto err;
		}
		ret = fmp_cipher_run(fmp, fdata, dst, len, 1,
				READ_MODE, priv, ci);
		if (ret) {
			pr_err("Fail to run fmp cipher ret(%d)\n",
				ret);
			goto err;
		}
	} else if (enc == DECRYPT) {
		ret = fmp_cipher_run(fmp, fdata, src, len, 1,
				WRITE_MODE, priv, ci);
		if (ret) {
			pr_err("Fail to run fmp cipher ret(%d)\n",
				ret);
			goto err;
		}
		ret = fmp_cipher_run(fmp, fdata, dst, len, 0,
				READ_MODE, priv, ci);
		if (ret) {
			pr_err("Fail to run fmp cipher ret(%d)\n",
				ret);
			goto err;
		}
	} else {
		pr_err("%s: Invalid enc %d mode\n", __func__, enc);
		goto err;
	}

	return 0;
err:
	return -EINVAL;
}

/* test block device release for fmp test */
void fmp_test_exit(struct fmp_test_data *fdata)
{
	fmode_t fmode = FMODE_WRITE | FMODE_READ;

	if (!fdata) {
		pr_err("%s: Invalid exynos fmp struct\n", __func__);
		return;
	}
	if (fdata->bdev)
		blkdev_put(fdata->bdev, fmode);
	kzfree(fdata);
}
