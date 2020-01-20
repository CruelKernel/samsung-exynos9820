/*
 * Cryptographic API.
 *
 * SHA-256, as specified in
 * http://csrc.nist.gov/groups/STM/cavp/documents/shs/sha256-384-512.pdf
 *
 * SHA-256 code by Jean-Luc Cooke <jlcooke@certainkey.com>.
 *
 * Copyright (c) Jean-Luc Cooke <jlcooke@certainkey.com>
 * Copyright (c) Andrew McDonald <andrew@mcdonald.org.uk>
 * Copyright (c) 2002 James Morris <jmorris@intercode.com.au>
 * SHA224 Support Copyright 2007 Intel Corporation <jonathan.lynch@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#ifndef SHA256_FMP_H
#define SHA256_FMP_H

#include <linux/types.h>

#define SHA256_DIGEST_SIZE      32
#define SHA256_BLOCK_SIZE       64
#define SHA256_DIGEST_LENGTH  SHA256_DIGEST_SIZE

struct sha256_state {
	u32 state[SHA256_DIGEST_SIZE / 4];
	u64 count;
	u8 buf[SHA256_BLOCK_SIZE];
};

struct shash_desc {
	struct sha256_state __ctx;
};

typedef struct shash_desc SHA256_CTX;

typedef void (sha256_block_fn)(struct sha256_state *sst, u8 const *src,
			int blocks);

/* Initialises desc
 * Returns 0 at success, nonzero otherwise
 */
int sha256_init(struct shash_desc *desc);

/* Adds len bytes from data to desc.
 * Returns 0 at success, nonzero otherwise
 */
int sha256_update(struct shash_desc *desc, const u8 *data,
			unsigned int len);

/* Adds the final padding to desc and writes the resulting digest
 * to out, which must have at least SHA256_DIGEST_SIZE bytes of space.
 * Returns 0 at success, nonzero otherwise
 */
int sha256_final(struct shash_desc *desc, u8 *out);

/* Writes the digest of len bytes from data to out. Returns out.
 * Out should be preallocated at least SHA256_DIGEST_SIZE length.
 * Returns 0 at success, nonzero otherwise
 */
int sha256(const u8 *data, unsigned int len, u8 *out);

/* desc dup
 * Returns 0 at success, nonzero otherwise
 */
int sha256_desc_copy(struct shash_desc *dst, const struct shash_desc *src);

#endif  /* SHA256_FMP_H */
