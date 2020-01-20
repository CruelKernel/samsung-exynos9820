/*
 *  Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <keys/encrypted-type.h>
#include <keys/user-type.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/scatterlist.h>
#include <uapi/linux/keyctl.h>
#include <crypto/hash.h>
#include <crypto/kbkdf.h>
#include "crypto_sec.h"

/*
 * DRBG is needed for FIPS
 *
 * DRBG: Deterministic Random Bit Generator
 */
#ifdef CONFIG_CRYPTO_FIPS
static struct crypto_rng *fscrypt_rng = NULL;

/**
 * fscrypt_sec_exit_rng() - Shutdown the DRBG
 */
static void fscrypt_sec_exit_rng(void)
{
	if (!fscrypt_rng)
		return;

	printk(KERN_DEBUG "fscrypto: free crypto rng!\n");
	crypto_free_rng(fscrypt_rng);
	fscrypt_rng = NULL;
}

/**
 * fscrypt_sec_init_rng() - Setup for DRBG for random number generating.
 *
 * This function will modify global variable named fscrypt_rng.
 * Return: zero on success, otherwise the proper error value.
 */
static int fscrypt_sec_init_rng(void)
{
	struct crypto_rng *rng = NULL;
	struct file *filp = NULL;
	char *rng_seed = NULL;
	mm_segment_t fs_type;

	int trial = 10;
	int read = 0;
	int res = 0;

	/* already initialized */
	if (fscrypt_rng) {
		printk(KERN_ERR "fscrypto: fscrypt_rng was already initialized\n");
		return 0;
	}

	rng_seed = kmalloc(FS_RNG_SEED_SIZE, GFP_KERNEL);
	if (!rng_seed) {
		printk(KERN_ERR "fscrypto: failed to allocate rng_seed memory\n");
		res = -ENOMEM;
		goto out;
	}
	memset((void *)rng_seed, 0, FS_RNG_SEED_SIZE);

	// open random device for drbg seed number
	filp = filp_open("/dev/random", O_RDONLY, 0);
	if (IS_ERR(filp)) {
		res = PTR_ERR(filp);
		printk(KERN_ERR "fscrypto: failed to open random(err:%d)\n", res);
		filp = NULL;
		goto out;
	}

	fs_type = get_fs();
	set_fs(KERNEL_DS);
	while (trial > 0) {
		int get_bytes = (int)filp->f_op->read(filp, &(rng_seed[read]),
				FS_RNG_SEED_SIZE-read, &filp->f_pos);
		if (likely(get_bytes > 0))
			read += get_bytes;
		if (likely(read == FS_RNG_SEED_SIZE))
			break;
		trial--;
	}
	set_fs(fs_type);

	if (read != FS_RNG_SEED_SIZE) {
		printk(KERN_ERR "fscrypto: failed to get enough random bytes "
			"(read=%d / request=%d)\n", read, FS_RNG_SEED_SIZE);
		res = -EINVAL;
		goto out;
	}

	// create drbg for random number generation
	rng = crypto_alloc_rng("stdrng", 0, 0);
	if (IS_ERR(rng)) {
		printk(KERN_ERR "fscrypto: failed to allocate rng, "
			"not available (%ld)\n", PTR_ERR(rng));
		res = PTR_ERR(rng);
		rng = NULL;
		goto out;
	}

	// push seed to drbg
	res = crypto_rng_reset(rng, rng_seed, FS_RNG_SEED_SIZE);
	if (res < 0)
		printk(KERN_ERR "fscrypto: rng reset fail (%d)\n", res);
out:
	if (res && rng) {
		crypto_free_rng(rng);
		rng = NULL;
	}
	if (filp)
		filp_close(filp, NULL);
	kfree(rng_seed);

	// save rng on global variable
	fscrypt_rng = rng;
	return res;
}

static inline int generate_fek(char *raw_key)
{
	int res;
	int trial = 10;

	if (likely(fscrypt_rng)) {
again:
		BUG_ON(!fscrypt_rng);
		return crypto_rng_get_bytes(fscrypt_rng,
			raw_key, FS_KEY_DERIVATION_NONCE_SIZE);
	}

	do {
		res = fscrypt_sec_init_rng();
		if (!res)
			goto again;

		printk(KERN_DEBUG "fscrypto: try to fscrypt_sec_init_rng(%d)\n", trial);
		msleep(500);
		trial--; 

	} while(trial > 0);

	printk(KERN_ERR "fscrypto: failed to initialize "
			"crypto rng handler again (err:%d)\n", res);
	return res;
}
#else
static void fscrypt_sec_exit_rng(void) { }
static inline int generate_fek(char *raw_key)
{
	get_random_bytes(raw_key, FS_KEY_DERIVATION_NONCE_SIZE);
	return 0;
}
#endif /* CONFIG CRYPTO_FIPS */


/**
 * fscrypt_sec_get_key_aes() - Get a key using AES-256-CBC
 * Return: Zero on success; non-zero otherwise.
 */
int fscrypt_sec_get_key_aes(const u8 *master_key, const struct fscrypt_context *ctx,
										u8 *derived_key, unsigned int derived_keysize, u8 *iv_key)
{
	int res = 0;
	char derived_key_input[SEC_FS_DERIVED_KEY_INPUT_SIZE];
	char derived_key_output[SEC_FS_DERIVED_KEY_OUTPUT_SIZE];
	int derived_key_length = 0;

	memcpy(derived_key_input, master_key, SEC_FS_MASTER_KEY_SIZE);
	memcpy(derived_key_input+SEC_FS_MASTER_KEY_SIZE, ctx->nonce, FS_KEY_DERIVATION_NONCE_SIZE);
	if((iv_key != NULL) && (ctx->filenames_encryption_mode == FS_ENCRYPTION_MODE_AES_256_CTS)) {
		memcpy(derived_key_input+SEC_FS_MASTER_KEY_SIZE+FS_KEY_DERIVATION_NONCE_SIZE, "FN",
					SEC_FS_ENCRYPION_MODE_SIZE);
	} else {
		memcpy(derived_key_input+SEC_FS_MASTER_KEY_SIZE+FS_KEY_DERIVATION_NONCE_SIZE, "FC",
					SEC_FS_ENCRYPION_MODE_SIZE);
	}

	// Using KBKDF API
	res = crypto_calc_kdf_hmac_sha512_ctr(KDF_DEFAULT, KDF_RLEN_08BIT
					, derived_key_input
					, SEC_FS_DERIVED_KEY_INPUT_SIZE
					, derived_key_output
					, &derived_key_length
					, NULL
					, 0
					, NULL
					, 0);
	if (res) {		
		printk(KERN_ERR "fscrypto: crypto_calc_kdf_hmac_sha512_ctr (err:%d)\n", res);
		goto out;
	}

	memcpy(derived_key, derived_key_output, derived_keysize);
	
	if((iv_key != NULL) && (ctx->filenames_encryption_mode == FS_ENCRYPTION_MODE_AES_256_CTS)
		&& (derived_keysize == SEC_FS_AES_256_CTS_CBC_SIZE))
		memcpy(iv_key, derived_key_output+derived_keysize, FS_CRYPTO_BLOCK_SIZE);
out:
	memzero_explicit(derived_key_input, SEC_FS_DERIVED_KEY_INPUT_SIZE);
	memzero_explicit(derived_key_output, SEC_FS_DERIVED_KEY_OUTPUT_SIZE);
	return res;
}

/**
 * fscrypt_sec_set_key_aes() - Generate and save a random key for AES-256.
 * Return: Zero on success; non-zero otherwise.
 */
int fscrypt_sec_set_key_aes(char *raw_key)
{
	int res = 0;

	res = generate_fek(raw_key);
	if (res < 0) {
		printk(KERN_ERR "fscrypto: failed to generate FEK (%d)\n", res);
	}

	return res;
}

int __init fscrypt_sec_crypto_init(void)
{
/* It's able to block fscrypto-fs module initialization. is there alternative? */
#if 0
	int res;

	res = fscrypt_sec_init_rng();
	if (res) {
		printk(KERN_WARNING "fscrypto: failed to initialize "
			"crypto rng handler (err:%d), postponed\n", res);
		/* return SUCCESS, although init_rng() failed */
	}
#endif
	return 0;
}

void __exit fscrypt_sec_crypto_exit(void)
{
	fscrypt_sec_exit_rng();
}

void fscrypto_dump_hex(char *data, int bytes)
{
	int i = 0;
	int add_newline = 1;

	if (bytes != 0) {
		printk(KERN_DEBUG "0x%.2x.", (unsigned char)data[i]);
		i++;
	}
	while (i < bytes) {
		printk("0x%.2x.", (unsigned char)data[i]);
		i++;
		if (i % 16 == 0) {
			printk("\n");
			add_newline = 0;
		} else
			add_newline = 1;
	}
	if (add_newline)
		printk("\n");
}
