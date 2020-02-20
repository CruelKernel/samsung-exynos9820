/*
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Functions related to blk-crypt(for inline encryption) handling
 *
 * NOTE :
 * blk-crypt implementation not depends on a inline encryption hardware.
 *
 * TODO :
 * We need to agonize what lock we use between spin_lock_irq and spin_lock
 * for algorithm_manager.
 */

#include <linux/module.h>
#include <linux/blkdev.h>
#include <crypto/skcipher.h>

#define BLK_CRYPT_ALG_NAMELEN_MAX	(15)
#define BLK_CRYPT_VERSION		"1.0.0"
#define BLK_CRYPT_MODE_PRIVATE		(127)

struct blk_crypt_algorithm {
	struct list_head list;
	atomic_t ref;
	unsigned int mode;
	struct block_device *bdev;
	char name[BLK_CRYPT_ALG_NAMELEN_MAX+1];
	const struct blk_crypt_algorithm_cbs *hw_cbs;
};

struct blk_crypt_context {
	struct blk_crypt_algorithm *bc_alg;
	void *bc_private;
};

struct blk_crypt_algorithm_manager {
	struct list_head list;
	spinlock_t lock;
	atomic_t ref;
	struct blk_crypt_algorithm *last_acc;
};

/* Initialize algorithm manager */
static struct blk_crypt_algorithm_manager alg_manager = {
	.list = LIST_HEAD_INIT(alg_manager.list),
	.lock = __SPIN_LOCK_UNLOCKED(alg_manager.lock),
	.ref = ATOMIC_INIT(0),
	.last_acc = NULL,
};

static struct kmem_cache *blk_crypt_cachep = NULL;

/* Static functions */
/**
 * blk_crypt_alloc_context() - allocate blk_crypt_context structure
 * @mode: filesystem encryption mode.
 *
 * Find hardware algorithm matched with specified mode and
 * calling the hw callback function to allocate crypt_info structure.
 *
 * Return: Allocated blk_crypt_context if succeed to find algorithm mode,
 * -ENOKEY otherwise.
 */
static blk_crypt_t *blk_crypt_alloc_context(struct block_device *bdev, const char *cipher_str)
{
	struct blk_crypt_algorithm *alg;
	struct blk_crypt_context *bctx;
	int res = -ENOENT;

	spin_lock_irq(&alg_manager.lock);
	if (alg_manager.last_acc && !strcmp(alg_manager.last_acc->name, cipher_str)) {
		alg = alg_manager.last_acc;
		atomic_inc(&alg->ref);
		goto unlock;
	}

	list_for_each_entry(alg, &alg_manager.list, list) {
		if (strcmp(alg->name, cipher_str)) {
			pr_info("blk-crypt: no matched alg(%s) != requested alg(%s)\n",
				alg->name, cipher_str);
			continue;
		}

		if (alg->bdev && alg->bdev != bdev) {
			pr_info("blk-crypt: no matched alg(%s)->bdev != bdev\n",
				cipher_str);
			continue;
		}

		alg_manager.last_acc = alg;
		atomic_inc(&alg->ref);
		break;
	}
unlock:
	spin_unlock_irq(&alg_manager.lock);

	if (alg == (void *)&alg_manager.list)
		return ERR_PTR(-ENOENT);

	bctx = kmem_cache_zalloc(blk_crypt_cachep, GFP_NOFS);
	if (!bctx) {
		res = -ENOMEM;
		goto err_free;
	}

	bctx->bc_alg = alg;
	bctx->bc_private = alg->hw_cbs->alloc();
	if (IS_ERR(bctx->bc_private)) {
		res = PTR_ERR(bctx->bc_private);
		pr_err("blk-crypt: failed to alloc data for alg(%s), err:%d\n",
				alg->name, res);
		goto err_free;
	}

	return bctx;
err_free:
	atomic_dec(&alg->ref);
	if (bctx)
		kmem_cache_free(blk_crypt_cachep, bctx);
	return ERR_PTR(res);
}


static inline blk_crypt_t *__bio_crypt(const struct bio *bio)
{
	if (bio && (bio->bi_opf & REQ_CRYPT))
		return bio->bi_cryptd;

	return NULL;
}

bool blk_crypt_encrypted(const struct bio *bio)
{
	return __bio_crypt(bio) ? true : false;
}

bool blk_crypt_mergeable(const struct bio *a, const struct bio *b)
{
#ifdef CONFIG_BLK_DEV_CRYPT
	if (__bio_crypt(a) != __bio_crypt(b))
		return false;

#ifdef CONFIG_BLK_DEV_CRYPT_DUN
	if (__bio_crypt(a)) {
		if (!bio_dun(a) ^ !bio_dun(b))
			return false;
		if (bio_dun(a) && bio_end_dun(a) != bio_dun(b))
			return false;
	}
#endif
#endif
	return true;
}

/* Filesystem APIs */
blk_crypt_t *blk_crypt_get_context(struct block_device *bdev, const char *cipher_str)
{
	struct blk_crypt_t *bctx;
	bctx = blk_crypt_alloc_context(bdev, cipher_str);
	if (IS_ERR(bctx)) {
		pr_debug("error allocating diskciher '%s' err: %d",
			cipher_str, PTR_ERR(bctx));
		return bctx;
	}

	return bctx;
}

unsigned char *blk_crypt_get_key(blk_crypt_t *bc_ctx)
{
	struct blk_crypt_context *bctx = bc_ctx;

	if (!bctx)
		return ERR_PTR(-EINVAL);

	return bctx->bc_alg->hw_cbs->get_key(bctx->bc_private);
}

int blk_crypt_set_key(blk_crypt_t *bc_ctx, u8 *raw_key, u32 keysize)
{
	struct blk_crypt_context *bctx = bc_ctx;

	if (!bctx)
		return -EINVAL;

	return bctx->bc_alg->hw_cbs->set_key(bctx->bc_private, raw_key, keysize);
}

void *blk_crypt_get_data(blk_crypt_t *bc_ctx)
{
	struct blk_crypt_context *bctx = bc_ctx;
	return bctx->bc_private;
}

void blk_crypt_put_context(blk_crypt_t *bc_ctx)
{
	struct blk_crypt_context *bctx = bc_ctx;

	if (!bctx)
		return;

	bctx->bc_alg->hw_cbs->free(bctx->bc_private);
	atomic_dec(&(bctx->bc_alg->ref));
	kmem_cache_free(blk_crypt_cachep, bctx);
}

/* H/W algorithm APIs */
static int blk_crypt_initialize()
{
	if (likely(blk_crypt_cachep))
		return 0;

	blk_crypt_cachep = KMEM_CACHE(blk_crypt_context, SLAB_RECLAIM_ACCOUNT);
	if (!blk_crypt_cachep)
		return -ENOMEM;

	return 0;
}

/**
 * blk_crypt_alg_register() - register a hw algorithm in algorithm manager
 * @name: name of algorithm
 * @mode: filesystem encryption mode
 * @hw_cbs: hardware callback functions
 *
 * A mode and name must be unique value so that these values are used to
 * differentiate each algorithm. If the algorithm manager finds duplicated
 * mode or name in algorithm list, it considers that as existing algorithm.
 *
 * Return: hw algorithm pointer on success,
 * -EEXIST if already registered.
 */
void *blk_crypt_alg_register(struct block_device *bdev, const char *name,
	const unsigned int mode, const struct blk_crypt_algorithm_cbs *hw_cbs)
{
	struct blk_crypt_algorithm *hw_alg, *new_alg;

	if (unlikely(blk_crypt_initialize())) {
		pr_err("blk-crypt: failed to initialize\n");
		return ERR_PTR(-ENOMEM);
	}

	if (!name || strlen(name) > BLK_CRYPT_ALG_NAMELEN_MAX) {
		pr_err("blk-crypt: invalid algorithm name or namelen\n");
		return ERR_PTR(-EINVAL);
	}

	/* Allocate new dhe_algorithm */
	new_alg = kzalloc(sizeof(struct blk_crypt_algorithm), GFP_KERNEL);
	if (!new_alg)
		return ERR_PTR(-ENOMEM);

	/* Initialize new hw_algorithm */
	strncpy(new_alg->name, name, BLK_CRYPT_ALG_NAMELEN_MAX);
	new_alg->bdev = bdev;
	new_alg->mode = mode;
	new_alg->hw_cbs = hw_cbs;
	INIT_LIST_HEAD(&new_alg->list);

	/* Check the existence of same thing. */
	spin_lock_irq(&alg_manager.lock);
	list_for_each_entry(hw_alg, &alg_manager.list, list) {
		/* an algorithm can be allocated for multiple devices */
		if (hw_alg->bdev && bdev && (hw_alg->bdev != bdev))
			continue;
		if (hw_alg->mode == mode)
			break;
		if (!strcmp(hw_alg->name, name))
			break;
	}

	/* Already exist, can not register new one */
	if (&alg_manager.list != &hw_alg->list) {
		spin_unlock_irq(&alg_manager.lock);
		pr_err("blk-crypt: algorithm(%s) already exists\n", name);
		kfree(new_alg);
		return ERR_PTR(-EEXIST);
	}

	/* Add to algorithm manager */
	list_add(&new_alg->list, &alg_manager.list);
	spin_unlock_irq(&alg_manager.lock);
	pr_info("blk_crypt: registed algorithm(%s)\n", name);

	return new_alg;
}
EXPORT_SYMBOL(blk_crypt_alg_register);

/**
 * blk_crypt_alg_unregister() - unregister hw algorithm from the algorithm manager
 * @handle: The pointer of hw algorithm that returned by blk_crypt_alg_register().
 *
 * Free and unregister specified handle if there is no ref count.
 */
int blk_crypt_alg_unregister(void *handle)
{
	struct blk_crypt_algorithm *alg;

	if (!handle)
		return -EINVAL;

	alg = (struct blk_crypt_algorithm *)handle;

	/* prevent unregister if an algorithm is  being referenced */
	if (atomic_read(&alg->ref))
		return -EBUSY;

	/* just unlink this algorithm from the manager */
	spin_lock_irq(&alg_manager.lock);
	list_del_init(&alg->list);
	spin_unlock_irq(&alg_manager.lock);

	kfree(handle);
	return 0;
}
EXPORT_SYMBOL(blk_crypt_alg_unregister);

/* Blk-crypt core functions */
static int __init blk_crypt_init(void)
{
	pr_info("%s\n", __func__);
	blk_crypt_initialize();
	return 0;
}

static void __exit blk_crypt_exit(void)
{
	pr_info("%s\n", __func__);
	if (atomic_read(&alg_manager.ref)) {
		pr_warn("blk-crypt: remailed %d registered algorithm\n",
				atomic_read(&alg_manager.ref));
	}

	kmem_cache_destroy(blk_crypt_cachep);
	blk_crypt_cachep = NULL;
}

module_init(blk_crypt_init);
module_exit(blk_crypt_exit);
