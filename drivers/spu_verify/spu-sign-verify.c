/*
 * spu-sign-verify.c - When SPU fireware update, image signature verify.
 *
 * Author: JJungs-lee <jhs2.lee@samsung.com>
 *
 */
#define pr_fmt(fmt) "spu_verify: " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/fcntl.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <crypto/public_key.h>
#include <crypto/hash.h>

#include <linux/spu-verify.h>
#include "spu-sign-verify.h"

#define FW_DIGEST_MISMATCH 999

const char *spu_tag_name[] = { TSP_TAG, MFC_TAG, WACOM_TAG, PDIC_TAG, SENSORHUB_TAG };

static int calc_digest_from_image(const u8* data, const long size, u8 *digest) {
	int ret = 0;
	struct crypto_shash *tfm = NULL;
	struct shash_desc* desc = NULL;
	const char* alg_name = "sha256";

	if (!data || !digest || size < 0) {
		pr_err("--> %s) invalid parameter\n", __func__);
		return -EINVAL;
	}

	tfm = crypto_alloc_shash(alg_name, CRYPTO_ALG_TYPE_SHASH, 0);
	if (IS_ERR(tfm)) {
		pr_err("--> %s) can't alloc tfm\n", __func__);
		return PTR_ERR(tfm);
	}

	desc = kzalloc(sizeof(struct shash_desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
	if (!desc) {
		pr_err("--> %s) can't alloc desc\n", __func__);
		ret = PTR_ERR(desc);
		goto fail_desc;
	}

	desc->tfm = tfm;
	desc->flags = 0x0;

	ret = crypto_shash_init(desc);
	if(ret < 0) {
		pr_err("--> %s) crypto_shash_init() failed\n", __func__);
		goto out;
	}

	ret = crypto_shash_update(desc, data, size);
	if(ret < 0) {
		pr_err("--> %s) crypto_shash_update() failed\n", __func__);
		goto out;
	}

	ret = crypto_shash_final(desc, digest);
	if(ret < 0) {
		pr_err("--> %s) crypto_shash_final() failed\n", __func__);
	}

out:
	if(desc)
		kfree(desc);
fail_desc:
	crypto_free_shash(tfm);

	return ret;
}

long spu_firmware_signature_verify(const char* fw_name, const u8* fw_data, const long fw_size) {
	int i = 0;
	int ret = 0;
	long fw_offset = fw_size;
	long TAG_LEN = 0;
	struct public_key* rsa_pub_key = NULL;
	struct public_key_signature* sig = NULL;

	u8 *signature = NULL;
	u8 read_digest[DIGEST_LEN] = { 0x00, };
	u8 calc_digest[DIGEST_LEN] = { 0x00, };

	if(!fw_name || !fw_data || fw_size < 0) {
		pr_info("invalid parameter...\n");
		return -EINVAL;
	}

	pr_info("%s requested for signature verify\n", fw_name);

	for(i = 0; i < ARRAY_SIZE(spu_tag_name); ++i) {
		TAG_LEN = strlen(spu_tag_name[i]);
		if(strncmp(fw_name, spu_tag_name[i], TAG_LEN) == 0) {
			ret = 1;
			break;
		}
	}

	if(!ret) {
		pr_info("This firmware don't support...\n");
		return -1;
	}

	signature = kzalloc(SIGN_LEN, GFP_KERNEL);
	if (!signature) {
		pr_err("signature kmalloc ERROR\n");
		return -ENOMEM;
	}

	fw_offset -= SIGN_LEN;
	memcpy(signature, fw_data + fw_offset, SIGN_LEN);

	fw_offset -= DIGEST_LEN;
	memcpy(read_digest, fw_data + fw_offset, DIGEST_LEN);

	fw_offset -= TAG_LEN;
	if(TAG_LEN <= 0 || strncmp(fw_name, fw_data + fw_offset, TAG_LEN) != 0) {
		pr_info("Firmware mismatch...\n");
		ret = -FW_DIGEST_MISMATCH;
		goto fail_mismatch;
	}

	fw_offset += TAG_LEN;
	ret = calc_digest_from_image(fw_data, fw_offset, calc_digest);
	if(ret < 0) {
		pr_err("calc_digest_from_image ERROR\n");
		goto fail_mismatch;
	}

	if(memcmp(read_digest, calc_digest, DIGEST_LEN) !=0) {
		pr_err("digset matching FAILED...\n");
		ret = -FW_DIGEST_MISMATCH;
		goto fail_mismatch;
	}

	rsa_pub_key = kzalloc(sizeof(struct public_key), GFP_KERNEL);
	if (!rsa_pub_key) {
		pr_err("rsa_pub_key kmalloc ERROR\n");
		ret = -ENOMEM;
		goto fail_mismatch;
	}

	sig = kzalloc(sizeof(struct public_key_signature), GFP_KERNEL);
	if (!sig) {
		pr_err("sig kmalloc ERROR\n");
		ret = -ENOMEM;
		goto fail_sig;
	}

	rsa_pub_key->key = (void*)PublicKey;
	rsa_pub_key->keylen = (u32)(sizeof(PublicKey));
	rsa_pub_key->id_type = "X509";
	rsa_pub_key->pkey_algo = "rsa";

	sig->s = signature;
	sig->s_size = (u32)SIGN_LEN;
	sig->digest = calc_digest;
	sig->digest_size = (u8)DIGEST_LEN;
	sig->pkey_algo = "rsa";
	sig->hash_algo = "sha256";

	ret = public_key_verify_signature(rsa_pub_key, sig);
	if (ret) {
		pr_info("verifying ERROR(%d)\n", ret);
	} else {
		pr_info("verified successfully!!!\n");
		ret = fw_offset - TAG_LEN;
	}

	if (sig)
		kfree(sig);
fail_sig:
	if (rsa_pub_key)
		kfree(rsa_pub_key);
fail_mismatch:
	if (signature)
		kfree(signature);

	return ret;
}