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
 * blk-crypt implementation depends on a inline encryption hardware.
 *
 */

#ifndef _BLK_CRYPT_H
#define _BLK_CRYPT_H

#include <linux/types.h>

struct bio;
struct block_device;
typedef void	blk_crypt_t;

#define BLK_CRYPT_MODE_INLINE_PRIVATE          (127)

struct blk_crypt_algorithm_cbs {
	void *(*alloc)(void);
	void (*free)(void *data);
	unsigned char *(*get_key)(void *);
	int (*set_key)(void *, const char *, int);
};

extern bool blk_crypt_mergeable(const struct bio *a, const struct bio *b);

extern void *blk_crypt_alg_register(struct block_device *, const char *, const unsigned int,
		                const struct blk_crypt_algorithm_cbs *);
extern int blk_crypt_alg_unregister(void *);

extern blk_crypt_t *blk_crypt_get_context(struct block_device *, const char *);
extern void blk_crypt_put_context(blk_crypt_t *);
extern void *blk_crypt_get_data(blk_crypt_t *);
extern unsigned char *blk_crypt_get_key(blk_crypt_t *);
extern int blk_crypt_set_key(blk_crypt_t *, u8 *raw_key, u32 keysize);
#endif /* _BLK_CRYPT_H */
