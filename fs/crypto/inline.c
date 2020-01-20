/*
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Functions related to inline encryption handling
 *
 * NOTE :
 * blkcrypt implementation depends on a inline encryption hardware.
 * Refactoring is needed to remove any dependency on specified hardwae.
 */

#include <linux/pagemap.h>
#include <linux/module.h>
#include <linux/buffer_head.h>
#include <linux/bio.h>
#include <linux/namei.h>
#include "fscrypt_private.h"
#include <linux/blk-crypt.h>

int fscrypt_inline_encrypted(const struct inode *inode)
{
	return __fscrypt_inline_encrypted(inode);
}
EXPORT_SYMBOL(fscrypt_inline_encrypted);

static inline void *__fscrypt_get_bio_cryptd(const struct inode *inode)
{
	return blk_crypt_get_data(inode->i_crypt_info->ci_private);
}

void *fscrypt_get_bio_cryptd(const struct inode *inode)
{
	if (!__fscrypt_inline_encrypted(inode))
		return NULL;

	return __fscrypt_get_bio_cryptd(inode);
}
EXPORT_SYMBOL(fscrypt_get_bio_cryptd);


void fscrypt_set_bio_cryptd(const struct inode *inode, struct bio *bio)
{
	BUG_ON(!__fscrypt_inline_encrypted(inode));

	BUG_ON(!S_ISREG(inode->i_mode));
	bio->bi_opf |= REQ_CRYPT;
	bio->bi_cryptd = __fscrypt_get_bio_cryptd(inode);
#if defined(CONFIG_CRYPTO_DISKCIPHER_DEBUG)
	//crypto_diskcipher_set(bio, inode->i_crypt_info->ci_dtfm);
	crypto_diskcipher_debug(DISKC_API_SET, 0);
#endif
}
EXPORT_SYMBOL(fscrypt_set_bio_cryptd);

int fscrypt_submit_bh(int op, int op_flags, struct buffer_head *bh, struct inode *inode)
{
	void *bi_cryptd = NULL;

	/*
	 * Inline-encrypted also means S_ISREG() is true.
	 * And ll_rw_block does not use bh->b_private for end_io.
	 * So, we use it temporarily while ll_rw_block is doing
	 * in order to pass blk_crypt data, if REQ_CRYPT is set.
	 */
	if (!__fscrypt_inline_encrypted(inode))
		return -EOPNOTSUPP;

	BUG_ON(!S_ISREG(inode->i_mode));
	BUG_ON(op_flags & REQ_CRYPT);
	op_flags |= REQ_CRYPT;
	bi_cryptd = __fscrypt_get_bio_cryptd(inode);

	/* Attach blk-crypt data to bh->b_private */
	BUG_ON(!bi_cryptd);
	BUG_ON(bh->b_private);
	bh->b_private = bi_cryptd;

	ll_rw_block(op, op_flags, 1, &bh);

	/* Restore bh->b_private */
	bh->b_private = NULL;
	return 0;
}
EXPORT_SYMBOL(fscrypt_submit_bh);

