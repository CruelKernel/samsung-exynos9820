/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
*/

#include <linux/cred.h>
#include <linux/crypto.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/key.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/version.h>
#include <crypto/hash.h>
#include "include/defex_debug.h"
#include "include/defex_sign.h"


#define SIGN_SIZE		256
#define SHA256_DIGEST_SIZE	32
#define MAX_DATA_LEN		300

extern char defex_public_key_start[];
extern char defex_public_key_end[];

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)

static int __init defex_public_key_verify_signature(unsigned char *pub_key,
					int pub_key_size,
					unsigned char *signature,
					unsigned char *hash_sha256)
{
	(void)pub_key;
	(void)pub_key_size;
	(void)signature;
	(void)hash_sha256;
	/* Skip signarue check at kernel version < 3.7.0 */
	printk("[DEFEX] Skip signature check in current kernel version\n");
	return 0;
}

#else

#include <crypto/public_key.h>

static struct key *defex_keyring;

static struct key* __init defex_keyring_alloc(const char *description,
					      kuid_t uid, kgid_t gid,
					      const struct cred *cred,
					      unsigned long flags)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
	return keyring_alloc(description, uid, gid, cred, flags, NULL)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 7, 0)
	key_perm_t perm = ((KEY_POS_ALL & ~KEY_POS_SETATTR) | KEY_USR_VIEW | KEY_USR_READ | KEY_USR_SEARCH);
	return keyring_alloc(description, uid, gid, cred, perm, flags, NULL);
#else
	key_perm_t perm = ((KEY_POS_ALL & ~KEY_POS_SETATTR) | KEY_USR_VIEW | KEY_USR_READ | KEY_USR_SEARCH);
	return keyring_alloc(description, uid, gid, cred, perm, flags, NULL, NULL);
#endif
}

static int __init defex_keyring_init(void)
{
	int err = 0;
	const struct cred *cred = current_cred();
	static const char keyring_name[] = "defex_keyring";

	defex_keyring = defex_keyring_alloc(keyring_name, KUIDT_INIT(0), KGIDT_INIT(0),
					    cred, KEY_ALLOC_NOT_IN_QUOTA);
	if (!defex_keyring) {
		err = -1;
		pr_info("Can't allocate %s keyring (NULL)\n", keyring_name);
	} else if (IS_ERR(defex_keyring)) {
		err = PTR_ERR(defex_keyring);
		pr_info("Can't allocate %s keyring, err=%d\n", keyring_name, err);
		defex_keyring = NULL;
	}
	return err;
}

static int __init defex_public_key_verify_signature(unsigned char *pub_key,
					int pub_key_size,
					unsigned char *signature,
					unsigned char *hash_sha256)
{
	int ret = -1;
	key_ref_t key_ref;
	struct key *key;
	struct public_key_signature pks;

	if (defex_keyring_init() != 0)
		return ret;

	key_ref = key_create_or_update(make_key_ref(defex_keyring, 1),
				       "asymmetric",
				       NULL,
				       pub_key,
				       pub_key_size,
				       ((KEY_POS_ALL & ~KEY_POS_SETATTR) | KEY_USR_VIEW | KEY_USR_READ),
				       KEY_ALLOC_NOT_IN_QUOTA);

	if (IS_ERR(key_ref)) {
		printk(KERN_INFO "Invalid key reference\n");
		return ret;
	}

	key = key_ref_to_ptr(key_ref);

	memset(&pks, 0, sizeof(pks));
	pks.digest = hash_sha256;
	pks.digest_size = SHA256_DIGEST_SIZE;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
	pks.pkey_algo = PKEY_ALGO_RSA;
#endif
	pks.pkey_hash_algo = HASH_ALGO_SHA256;
	pks.nr_mpi = 1;
	pks.rsa.s = mpi_read_raw_data(signature, SIGN_SIZE);
	if (pks.rsa.s)
		ret = verify_signature(key, &pks);
	mpi_free(pks.rsa.s);
#else
	pks.pkey_algo = "rsa";
	pks.hash_algo = "sha256";
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
	pks.encoding = "pkcs1";
#endif
	pks.s = signature;
	pks.s_size = SIGN_SIZE;
	ret = verify_signature(key, &pks);
#endif
	key_ref_put(key_ref);
	keyring_clear(defex_keyring);
	return ret;
}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0) */

#ifdef DEFEX_DEBUG_ENABLE
void __init blob(const char *buffer, const size_t bufLen, const int lineSize)
{
	size_t i = 0, line;
	size_t j = 0, len = bufLen;
	int offset = 0;
	char c, stringToPrint[MAX_DATA_LEN];

	do {
		line = (len > lineSize)?lineSize:len;
		offset  = 0;
		offset += snprintf(stringToPrint + offset, MAX_DATA_LEN - offset, "| 0x%0*zx | ", PR_HEX(i));

		for(j = 0; j < line; j++)
			offset += snprintf(stringToPrint + offset, MAX_DATA_LEN - offset, "%02X ", (unsigned char)buffer[i + j]);
		offset += snprintf(stringToPrint + offset, MAX_DATA_LEN - offset, "| ");

		for(j = 0; j < lineSize; j++) {
			c = buffer[i + j];
			c = (c < 0x20)?'.':c;
			offset += snprintf(stringToPrint + offset, MAX_DATA_LEN - offset, "%c", c);
		}
		if (line < lineSize) {
			for(j = 0; j < lineSize - line; j++)
				offset += snprintf(stringToPrint + offset, MAX_DATA_LEN - offset, " ");
		}

		offset += snprintf(stringToPrint + offset, MAX_DATA_LEN - offset, " |");
		printk(KERN_INFO "%s\n", stringToPrint);
		memset(stringToPrint, 0, MAX_DATA_LEN);
		i += line;
		len -= line;
	} while(len);
}
#endif

int __init defex_calc_hash(const char *data, unsigned int size, unsigned char *hash)
{
	struct crypto_shash *handle;
	struct shash_desc* shash;
	int err = -1;

	handle = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR_OR_NULL(handle)) {
		pr_err("[DEFEX] Can't alloc sha256");
		return err;
	}

	shash = kzalloc(sizeof(struct shash_desc) + crypto_shash_descsize(handle), GFP_KERNEL);
	if (!shash)
		goto clean_handle;

	shash->flags = 0;
	shash->tfm = handle;

	do {
		err = crypto_shash_init(shash);
		if (err < 0)
			break;

		err = crypto_shash_update(shash, data, size);
		if (err < 0)
			break;

		err = crypto_shash_final(shash, hash);
		if (err < 0)
			break;
	} while(0);

	kfree(shash);
clean_handle:
	crypto_free_shash(handle);
	return err;
}

int __init defex_rules_signature_check(const char *rules_buffer, unsigned int rules_data_size, unsigned int *rules_size)
{
	int res = -1;
	unsigned int defex_public_key_size = (unsigned int)((defex_public_key_end - defex_public_key_start) & 0xffffffff);
	unsigned char *hash_sha256;
	unsigned char *hash_sha256_first;
	unsigned char *signature;
	unsigned char *pub_key;

	hash_sha256_first = kzalloc(SHA256_DIGEST_SIZE, GFP_KERNEL);
	if (!hash_sha256_first)
		return res;
	hash_sha256 = kzalloc(SHA256_DIGEST_SIZE, GFP_KERNEL);
	if (!hash_sha256)
		goto clean_hash_sha256_first;
	signature = kzalloc(SIGN_SIZE, GFP_KERNEL);
	if (!signature)
		goto clean_hash_sha256;
	pub_key = kzalloc(defex_public_key_size, GFP_KERNEL);
	if (!pub_key)
		goto clean_signature;

	memcpy(pub_key, defex_public_key_start, defex_public_key_size);
	memcpy(signature, (u8*)(rules_buffer + rules_data_size - SIGN_SIZE), SIGN_SIZE);

	defex_calc_hash(rules_buffer, rules_data_size - SIGN_SIZE, hash_sha256_first);
	defex_calc_hash(hash_sha256_first, SHA256_DIGEST_SIZE, hash_sha256);

#ifdef DEFEX_DEBUG_ENABLE
	printk("[DEFEX] Rules signature size = %d\n", SIGN_SIZE);
	blob(signature, SIGN_SIZE, 16);
	printk("[DEFEX] Key size = %d\n", defex_public_key_size);
	blob(pub_key, defex_public_key_size, 16);
	printk("[DEFEX] Final Hash:\n");
	blob(hash_sha256, SHA256_DIGEST_SIZE, 16);
#endif

	res = defex_public_key_verify_signature(pub_key, defex_public_key_size, signature, hash_sha256);
	if (rules_size && !res)
		*rules_size = rules_data_size - SIGN_SIZE;
	kfree(pub_key);
clean_signature:
	kfree(signature);
clean_hash_sha256:
	kfree(hash_sha256);
clean_hash_sha256_first:
	kfree(hash_sha256_first);
	return res;
}
