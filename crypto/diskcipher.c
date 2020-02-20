/*
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/crypto.h>
#include <crypto/algapi.h>
#include <crypto/diskcipher.h>
#include <linux/delay.h>

#include "internal.h"

#ifdef CONFIG_CRYPTO_DISKCIPHER_DEBUG
#include <crypto/fmp.h>
#include <linux/mm_types.h>
#include <linux/fs.h>
#include <linux/fscrypt.h>

#define DUMP_MAX 20

struct dump_err {
	struct page *page;
	struct bio bio;
	struct fmp_crypto_info ci;
	enum diskcipher_dbg api;
};

struct diskc_debug_info {
	struct dump_err dump[DUMP_MAX];
	int err;
	int cnt[DISKC_USER_MAX][2];
};

static struct diskc_debug_info diskc_dbg;

void crypto_diskcipher_debug(enum diskcipher_dbg api, int bi_opf)
{
	int idx = 0;
	struct diskc_debug_info *dbg = &diskc_dbg;

	if (api <= DISKC_API_MAX)
		dbg->cnt[api][bi_opf]++;
	else {
		if (bi_opf)
			idx = 1;
		dbg->cnt[api][idx]++;
	}
}

static void print_err(void)
{
	int i, j;
	struct bio_vec *bv;	/* bio page list */
	struct bio *bio;
	struct fmp_crypto_info *ci;
	struct diskc_debug_info *dbg = &diskc_dbg;

	for (j = 0; j < dbg->err; j++) {
		bio = &dbg->dump[j].bio;
		ci = &dbg->dump[j].ci;

		if (bio) {
#ifdef CONFIG_BLK_DEV_CRYPT
			pr_info
			    ("%s(%d/%d): bio:%p ci:%p page:%p flag:%x, opf:%x, crypt:%p\n",
			     __func__, j, dbg->err, bio, ci, &dbg->dump[j].page,
			     bio->bi_flags, bio->bi_opf, bio->bi_cryptd);
#else
			pr_info
			    ("%s(%d/%d): bio:%p ci:%p page:%p flag:%x, opf:%x\n",
			     __func__, j, dbg->err, bio, ci, &dbg->dump[j].page,
			     bio->bi_flags, bio->bi_opf);
#endif
			print_hex_dump(KERN_CONT, "bio:", DUMP_PREFIX_OFFSET,
				16, 1, bio, sizeof(struct bio), false);
			for (i = 0; i < bio->bi_max_vecs; i++) {
				bv = &bio->bi_io_vec[i];
				pr_info("bv[%d] page:%p len:%d offset:%d\n",
					i, bv->bv_page, bv->bv_len, bv->bv_offset);
			}
		}

		if (ci) {
			pr_info("[ci] key_size:%d algo_mode:%d\n",
				ci->key_size, ci->algo_mode);
			print_hex_dump(KERN_CONT, "key:", DUMP_PREFIX_OFFSET,
				       16, 1, ci->key, sizeof(ci->key), false);
		}
	}
}

static void disckipher_log_show(struct seq_file *m)
{
	int i;
	struct diskc_debug_info *dbg = &diskc_dbg;
	char name[DISKC_USER_MAX][32] = {
		"ALLOC", "FREE", "FREEREQ", "SETKEY", "SET", "GET", "CRYPT", "CLEAR",
		"DISKC_API_MAX", "FS_PAGEIO", "FS_READP", "FS_DIO", "FS_BLOCK_WRITE",
		"FS_ZEROPAGE", "BLK_BH", "DMCRYPT", "DISKC_MERGE", "DISKC_MERGE_ERR_INODE", "DISKC_MERGE_ERR_DISK",
		"FS_DEC_WARN", "FS_ENC_WARN", "DISKC_MERGE_DIO", "DISKC_FREE_REQ_WARN",
		"DISKC_FREE_WQ_WARN", "DISKC_CRYPT_WARN",
		"DM_CRYPT_NONENCRYPT", "DM_CRYPT_CTR", "DM_CRYPT_DTR", "DM_CRYPT_OVER",
		"F2FS_gc", "F2FS_gc_data_page", "F2FS_gc_data_page_no_key", "F2FS_gc_data_page_no_key_FC",
		"F2FS_gc_data_page_FC",	"F2FS_gc_data_block", "F2FS_gc_data_block_key",
		"F2FS_gc_data_block_err1", "F2FS_gc_data_block_err2", "F2FS_gc_data_block_err3", "F2FS_gc_skip",
		"DISKC_ERR", "DISKC_NO_KEY_ERR", "DISKC_NO_SYNC_ERR", "DISKC_NO_CRYPT_ERR", "DISKC_NO_DISKC_ERR"};

	for (i = 0; i < DISKC_USER_MAX; i++)
		if (dbg->cnt[i][0] || dbg->cnt[i][1])
			seq_printf(m, "%s\t: %6u(err:%u)\n",
				name[i], dbg->cnt[i][0], dbg->cnt[i][1]);

	if (dbg->err)
		print_err();
}

/* check diskcipher for FBE */
#undef DISKC_FS_ENCRYPT_DEBUG /* it's not work on fips */
#ifdef DISKC_FS_ENCRYPT_DEBUG
static void dump_err(struct crypto_diskcipher *ci, enum diskcipher_dbg api,
	      struct bio *bio, struct page *page)
{
	struct diskc_debug_info *dbg = &diskc_dbg;

	if ((dbg->err < DUMP_MAX) && ci) {
		struct crypto_tfm *tfm = crypto_diskcipher_tfm(ci);

		dbg->dump[dbg->err].api = api;
		memcpy(&dbg->dump[dbg->err].ci, crypto_tfm_ctx(tfm),
		       sizeof(struct fmp_crypto_info));

		if (page)
			dbg->dump[dbg->err].page = page;
		if (bio)
			memcpy(&dbg->dump[dbg->err].bio, bio,
				sizeof(struct bio));
	}
	dbg->err++;
}

static bool crypto_diskcipher_check(struct bio *bio)
{
	int ret = 0;
	struct crypto_diskcipher *ci = NULL;
	struct inode *inode = NULL;
	struct page *page = NULL;

	if (!bio) {
		pr_err("%s: doesn't exist bio\n", __func__);
		goto out;
	}

	page = bio->bi_io_vec[0].bv_page;
	if (page && !PageAnon(page) && bio)
		if (page->mapping)
			if (page->mapping->host) {
				if (page->mapping->host->i_crypt_info) {
					inode = page->mapping->host;
					ci = fscrypt_get_bio_cryptd(inode);
					if (ci && (bio->bi_cryptd != ci)
					    && (!(bio->bi_flags & REQ_OP_DISCARD))) {
						pr_err("%s: no sync err\n", __func__);
						dump_err(ci, DISKC_API_GET, bio, page);
						crypto_diskcipher_debug(DISKC_NO_SYNC_ERR, 0);
						ret = -EINVAL;
					}
					if (!ci) {
						crypto_diskcipher_debug(DISKC_NO_DISKC_ERR, 1);
						pr_err("%s: no crypt err\n", __func__);
						ret = -EINVAL;
					}
				} else {
					crypto_diskcipher_debug(DISKC_NO_KEY_ERR, 1);
					ret = -EINVAL;
				}
			}
out:
	crypto_diskcipher_debug(DISKC_API_GET, ret);
	return ret;
}
#else
#define crypto_diskcipher_check(a) (0)
#endif
#else
#define crypto_diskcipher_check(a) (0)
#define disckipher_log_show(a) do { } while (0)
#endif

struct crypto_diskcipher *crypto_diskcipher_get(struct bio *bio)
{
	if (!bio || !virt_addr_valid(bio)) {
		pr_err("%s: Invalid bio:%p\n", __func__, bio);
		return NULL;
	}

	if (bio->bi_opf & REQ_CRYPT) {
		if (bio->bi_cryptd) {
			if (!crypto_diskcipher_check(bio))
				return bio->bi_cryptd;
			else
				return ERR_PTR(-EINVAL);
		} else {
			crypto_diskcipher_debug(DISKC_NO_CRYPT_ERR, 0);
			return ERR_PTR(-EINVAL);
		}
	}

	return NULL;
}

void crypto_diskcipher_set(struct bio *bio,
			struct crypto_diskcipher *tfm, u64 dun)
{
	if (bio && tfm) {
		bio->bi_opf |= REQ_CRYPT;
		bio->bi_cryptd = tfm;
#ifdef CONFIG_CRYPTO_DISKCIPHER_DUN
		if (dun)
			bio->bi_iter.bi_dun = dun;
#endif
	}
	crypto_diskcipher_debug(DISKC_API_SET, 0);
}

/* debug freerq */
enum diskc_status {
	DISKC_ST_INIT,
	DISKC_ST_FREE_REQ,
	DISKC_ST_FREE,
};

int crypto_diskcipher_setkey(struct crypto_diskcipher *tfm, const char *in_key,
			     unsigned int key_len, bool persistent)
{
	struct crypto_tfm *base = crypto_diskcipher_tfm(tfm);
	struct diskcipher_alg *cra = crypto_diskcipher_alg(base->__crt_alg);

	if (!cra) {
		pr_err("%s: doesn't exist cra. base:%p", __func__, base);
		return -EINVAL;
	}

	crypto_diskcipher_debug(DISKC_API_SETKEY, 0);
	return cra->setkey(base, in_key, key_len, persistent);
}

int crypto_diskcipher_clearkey(struct crypto_diskcipher *tfm)
{
	struct crypto_tfm *base = crypto_diskcipher_tfm(tfm);
	struct diskcipher_alg *cra = crypto_diskcipher_alg(base->__crt_alg);

	if (!cra) {
		pr_err("%s: doesn't exist cra. base:%p", __func__, base);
		return -EINVAL;
	}
	return cra->clearkey(base);
}

int crypto_diskcipher_set_crypt(struct crypto_diskcipher *tfm, void *req)
{
	int ret = 0;
	struct crypto_tfm *base = crypto_diskcipher_tfm(tfm);
	struct diskcipher_alg *cra = NULL;

	if (!base) {
		pr_err("%s: doesn't exist cra. base:%p", __func__, base);
		ret = -EINVAL;
		goto out;
	}

	cra = crypto_diskcipher_alg(base->__crt_alg);

	if (!cra) {
		pr_err("%s: doesn't exist cra. base:%p\n", __func__, base);
		ret = -EINVAL;
		goto out;
	}

	if (atomic_read(&tfm->status) == DISKC_ST_FREE) {
		pr_err("%s: tfm is free\n", __func__);
		crypto_diskcipher_debug(DISKC_CRYPT_WARN, 0);
		return -EINVAL;
	}

	ret = cra->crypt(base, req);

#ifdef USE_FREE_REQ
	if (!list_empty(&cra->freectrl.freelist)) {
		if (!atomic_read(&cra->freectrl.freewq_active)) {
			atomic_set(&cra->freectrl.freewq_active, 1);
			schedule_delayed_work(&cra->freectrl.freewq, 0);
		}
	}
#endif
out:
	if (ret)
		pr_err("%s fails ret:%d, cra:%p\n", __func__, ret, cra);
	crypto_diskcipher_debug(DISKC_API_CRYPT, ret);
	return ret;
}

int crypto_diskcipher_clear_crypt(struct crypto_diskcipher *tfm, void *req)
{
	int ret = 0;
	struct crypto_tfm *base = crypto_diskcipher_tfm(tfm);
	struct diskcipher_alg *cra = NULL;

	if (!base) {
		pr_err("%s: doesn't exist base, tfm:%p\n", __func__, tfm);
		ret = -EINVAL;
		goto out;
	}

	cra = crypto_diskcipher_alg(base->__crt_alg);

	if (!cra) {
		pr_err("%s: doesn't exist cra. base:%p\n", __func__, base);
		ret = -EINVAL;
		goto out;
	}

	if (atomic_read(&tfm->status) == DISKC_ST_FREE) {
		pr_warn("%s: tfm is free\n", __func__);
		return -EINVAL;
	}

	ret = cra->clear(base, req);
	if (ret)
		pr_err("%s fails", __func__);

out:
	crypto_diskcipher_debug(DISKC_API_CLEAR, ret);
	return ret;
}

#ifndef CONFIG_CRYPTO_MANAGER_DISABLE_TESTS
int diskcipher_do_crypt(struct crypto_diskcipher *tfm,
			struct diskcipher_test_request *req)
{
	int ret;
	struct crypto_tfm *base = crypto_diskcipher_tfm(tfm);
	struct diskcipher_alg *cra = crypto_diskcipher_alg(base->__crt_alg);

	if (!cra) {
		pr_err("%s: doesn't exist cra. base:%p\n", __func__, base);
		ret = -EINVAL;
		goto out;
	}

	if (cra->do_crypt)
		ret = cra->do_crypt(base, req);
	else
		ret = -EINVAL;
	if (ret)
		pr_err("%s fails ret:%d", __func__, ret);

out:
	return ret;
}
#endif

static int crypto_diskcipher_init_tfm(struct crypto_tfm *base)
{
	struct crypto_diskcipher *tfm = __crypto_diskcipher_cast(base);

	atomic_set(&tfm->status, DISKC_ST_INIT);
	return 0;
}

#ifdef USE_FREE_REQ
static void free_workq_func(struct work_struct *work)
{
	struct diskcipher_alg *cra =
		container_of(work, struct diskcipher_alg, freectrl.freewq.work);
	struct diskcipher_freectrl *fctrl = &cra->freectrl;
	struct crypto_diskcipher *_tfm, *tmp;
	unsigned long cur_jiffies = jiffies;
	struct list_head poss_free_list;
	unsigned long flags;

	INIT_LIST_HEAD(&poss_free_list);

	/* pickup freelist */
	spin_lock_irqsave(&fctrl->freelist_lock, flags);
	list_for_each_entry_safe(_tfm, tmp, &fctrl->freelist, node) {
		if (jiffies_to_msecs(cur_jiffies - _tfm->req_jiffies) > fctrl->max_io_ms)
			list_move_tail(&_tfm->node, &poss_free_list);
	}
	spin_unlock_irqrestore(&fctrl->freelist_lock, flags);

	list_for_each_entry_safe(_tfm, tmp, &poss_free_list, node) {
		if (atomic_read (&_tfm->status) != DISKC_ST_FREE_REQ)
			crypto_diskcipher_debug(DISKC_FREE_WQ_WARN, 0);
		crypto_free_diskcipher(_tfm);
	}

	if (!list_empty(&fctrl->freelist))
		schedule_delayed_work(&fctrl->freewq, msecs_to_jiffies(fctrl->max_io_ms));
	else
		atomic_set(&fctrl->freewq_active, 0);
}
#endif

void crypto_free_req_diskcipher(struct crypto_diskcipher *tfm)
{
#ifdef USE_FREE_REQ
	struct crypto_tfm *base = crypto_diskcipher_tfm(tfm);
	struct diskcipher_alg *cra = crypto_diskcipher_alg(base->__crt_alg);
	struct diskcipher_freectrl *fctrl = &cra->freectrl;
	unsigned long flags;

	if (atomic_read(&tfm->status) != DISKC_ST_INIT) {
		crypto_diskcipher_debug(DISKC_FREE_REQ_WARN, 0);
		pr_warn("%s: already submit status:%d\n", __func__, atomic_read(&tfm->status));
		return;
	}

	atomic_set(&tfm->status, DISKC_ST_FREE_REQ);
	INIT_LIST_HEAD(&tfm->node);
	tfm->req_jiffies = jiffies;
	spin_lock_irqsave(&fctrl->freelist_lock, flags);
	list_move_tail(&tfm->node, &fctrl->freelist);
	spin_unlock_irqrestore(&fctrl->freelist_lock, flags);
	crypto_diskcipher_debug(DISKC_API_FREEREQ, 0);
#else
	crypto_free_diskcipher(tfm);
#endif
}

unsigned int crypto_diskcipher_extsize(struct crypto_alg *alg)
{
	return alg->cra_ctxsize +
	    (alg->cra_alignmask & ~(crypto_tfm_ctx_alignment() - 1));
}

static void crypto_diskcipher_show(struct seq_file *m, struct crypto_alg *alg)
{
	seq_printf(m, "type         : diskcipher\n");
	disckipher_log_show(m);
}

static const struct crypto_type crypto_diskcipher_type = {
	.extsize = crypto_diskcipher_extsize,
	.init_tfm = crypto_diskcipher_init_tfm,
#ifdef CONFIG_PROC_FS
	.show = crypto_diskcipher_show,
#endif
	.maskclear = ~CRYPTO_ALG_TYPE_MASK,
	.maskset = CRYPTO_ALG_TYPE_MASK,
	.type = CRYPTO_ALG_TYPE_DISKCIPHER,
	.tfmsize = offsetof(struct crypto_diskcipher, base),
};

#define DISKC_NAME "-disk"
#define DISKC_NAME_SIZE (5)
#define DISKCIPHER_MAX_IO_MS (1000)
struct crypto_diskcipher *crypto_alloc_diskcipher(const char *alg_name,
			u32 type, u32 mask, bool force)
{
	crypto_diskcipher_debug(DISKC_API_ALLOC, 0);
	if (force) {
		if (strlen(alg_name) + DISKC_NAME_SIZE < CRYPTO_MAX_ALG_NAME) {
			char diskc_name[CRYPTO_MAX_ALG_NAME];

			strcpy(diskc_name, alg_name);
			strcat(diskc_name, DISKC_NAME);
			return crypto_alloc_tfm(diskc_name,
				&crypto_diskcipher_type, type, mask);
		}
	} else {
		return crypto_alloc_tfm(alg_name, &crypto_diskcipher_type, type, mask);
	}
	return NULL;
}

void crypto_free_diskcipher(struct crypto_diskcipher *tfm)
{
	crypto_diskcipher_debug(DISKC_API_FREE, 0);
	atomic_set(&tfm->status, DISKC_ST_FREE);
	crypto_destroy_tfm(tfm, crypto_diskcipher_tfm(tfm));
}

int crypto_register_diskcipher(struct diskcipher_alg *alg)
{
	struct crypto_alg *base = &alg->base;

#ifdef USE_FREE_REQ
	struct diskcipher_freectrl *fctrl = &alg->freectrl;

	INIT_LIST_HEAD(&fctrl->freelist);
	INIT_DELAYED_WORK(&fctrl->freewq, free_workq_func);
	spin_lock_init(&fctrl->freelist_lock);
	if (!fctrl->max_io_ms)
		fctrl->max_io_ms = DISKCIPHER_MAX_IO_MS;
#endif
	base->cra_type = &crypto_diskcipher_type;
	base->cra_flags = CRYPTO_ALG_TYPE_DISKCIPHER;
	return crypto_register_alg(base);
}

void crypto_unregister_diskcipher(struct diskcipher_alg *alg)
{
	crypto_unregister_alg(&alg->base);
}

int crypto_register_diskciphers(struct diskcipher_alg *algs, int count)
{
	int i, ret;

	for (i = 0; i < count; i++) {
		ret = crypto_register_diskcipher(algs + i);
		if (ret)
			goto err;
	}
	return 0;

err:
	for (--i; i >= 0; --i)
		crypto_unregister_diskcipher(algs + i);
	return ret;
}

void crypto_unregister_diskciphers(struct diskcipher_alg *algs, int count)
{
	int i;

	for (i = count - 1; i >= 0; --i)
		crypto_unregister_diskcipher(algs + i);
}
