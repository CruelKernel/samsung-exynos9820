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

#include "spu-sign-verify.h"

#define DIGEST_LEN 32
#define SIGN_LEN 512
#define METADATA_LEN ((DIGEST_LEN)+(SIGN_LEN))
#define SIGN_MALLOC_SIZE 4096
#define BUF_KMALLOC_SIZE 4096
#define DIGEST_MISSMATCH 999

static int read_fw_image(const char* path, u8* output, 
							int offset, int size, int is_metadata) {
	int ret = 0;
	struct file *filp = NULL;
	loff_t fsize = 0;
	mm_segment_t old_fs; 

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(path, O_RDONLY, 0);
	
	if(IS_ERR(filp)) {
		pr_err("--> %s) filp_open failed\n", __func__);
		set_fs(old_fs);
		return PTR_ERR(filp);
	}
	
	fsize = i_size_read(file_inode(filp));

	if(is_metadata)
		filp->f_pos = fsize - METADATA_LEN + offset;
	else 
		filp->f_pos = offset;

	ret = vfs_read(filp, output, size, &filp->f_pos);

	filp_close(filp, NULL);
	set_fs(old_fs);

	return ret;
}

static loff_t get_fw_image_size(const char* path) {
	
	loff_t fsize = 0;
	struct file *filp = NULL;
	mm_segment_t old_fs; 

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(path, O_RDONLY, 0);
	
	if(IS_ERR(filp)) {
		pr_err("--> %s) filp_open failed\n", __func__);
		set_fs(old_fs);
		return PTR_ERR(filp);
	}
	
	fsize = i_size_read(file_inode(filp));
	
	filp_close(filp, NULL);
	set_fs(old_fs);

	return fsize;
}

static int get_info_from_image(const char* path, u8* digest, u8* sign) {

	int ret = 0;

	if (!path || !digest || !sign) {
		pr_err("--> %s) invalid parameter\n", __func__);
		return -EINVAL;
	}

	ret = read_fw_image(path, digest, 0, DIGEST_LEN, 1);
	if(ret < 0) {
		pr_err("--> %s) digest read failed\n", __func__);
		goto error_read;
	}

	ret = read_fw_image(path, sign, DIGEST_LEN, SIGN_LEN, 1);
	if(ret < 0) {
		pr_err("--> %s) signatrue read failed\n", __func__);
	}

error_read:
	return ret;
}

static int calc_digest_from_image(const char* path, u8 *digest) {
	int ret = 0;
	struct crypto_shash *tfm = NULL;
	struct shash_desc* desc = NULL; 
	const char* alg_name = "sha256";
	loff_t file_size = 0;
	u64 start_offset = 0;
	u64 end_offset = 0;
	u8* buf = NULL;

	if (!path || !digest) {
		pr_err("--> %s) invalid parameter\n", __func__);
		return -EINVAL;
	}

	file_size = get_fw_image_size(path);

	tfm = crypto_alloc_shash(alg_name, CRYPTO_ALG_TYPE_SHASH, 0);
	if (IS_ERR(tfm)) {
		pr_err("--> %s) can't alloc tfm\n", __func__);
		return PTR_ERR(tfm);
	}

	desc = kmalloc(sizeof(struct shash_desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
	if (!desc) {
		pr_err("--> %s) can't alloc desc\n", __func__);
		ret = PTR_ERR(desc);
		goto fail_desc;
	}

	buf = kmalloc(BUF_KMALLOC_SIZE, GFP_KERNEL);
	if (!buf) {
		pr_err("--> %s) can't alloc buf\n", __func__);
		ret = PTR_ERR(buf);
		goto fail_buf;
	}

	desc->tfm = tfm;
	desc->flags = 0x0;

	ret = crypto_shash_init(desc);
	if(ret < 0) {
		pr_err("--> %s) crypto_shash_init() failed\n", __func__);
		goto out;
	}
	
	while(1) {
		start_offset = end_offset;
		end_offset += BUF_KMALLOC_SIZE;

		if(end_offset > file_size - METADATA_LEN)
			end_offset = file_size - METADATA_LEN;

		if(start_offset == end_offset) 
			break;

		ret = read_fw_image(path, buf, start_offset, end_offset - start_offset, 0);
		if(ret < 0) {
			pr_err("--> %s) read_fw_image failed\n", __func__);
			goto out;
		}
		ret = crypto_shash_update(desc, buf, end_offset - start_offset);
		if(ret < 0) {
			pr_err("--> %s) crypto_shash_update() failed\n", __func__);
			goto out;
		}
	}

	ret = crypto_shash_final(desc, digest);
	if(ret < 0) {
		pr_err("--> %s) crypto_shash_final() failed\n", __func__);
	}

out:
	if(buf)
		kfree(buf);
fail_buf:
	if(desc)
		kfree(desc);
fail_desc:
	if(tfm)
		crypto_free_shash(tfm);

	return ret;
}

int spu_fireware_signature_verify(const char* fw_name, const char* fw_path) {
	
	int ret = 0;
	int i = 0;
	struct public_key* rsa_pub_key = NULL;
	struct public_key_signature* sig = NULL;

	u8 *signature = NULL;
	u8 read_digest[DIGEST_LEN] = { 0x00, };
	u8 calc_digest[DIGEST_LEN] = { 0x00, };

	pr_info("%s requested for signature verify\n", fw_name);
	pr_info("fw binay path is %s\n", fw_path);

	rsa_pub_key = kzalloc(sizeof(struct public_key), GFP_KERNEL);
	if (!rsa_pub_key) {
		pr_err("rsa_pub_key kmalloc ERROR\n");
		return -ENOMEM;
	}

	sig = kzalloc(sizeof(struct public_key_signature), GFP_KERNEL);
	if (!sig) {
		pr_err("sig kmalloc ERROR\n");
		ret = -ENOMEM;
		goto fail_sig;
	}

	signature = kzalloc(SIGN_MALLOC_SIZE, GFP_KERNEL);
	if (!signature) {
		pr_err("signature kmalloc ERROR\n");
		ret = -ENOMEM;
		goto fail_signature;
	}

	ret = get_info_from_image(fw_path, read_digest, signature);
	if(ret < 0) {
		pr_err("get_info_from_image ERROR\n");  
		goto fail_info;
	}
	ret = calc_digest_from_image(fw_path, calc_digest);
	if(ret < 0){
		pr_err("calc_digest_from_image ERROR\n");  
		goto fail_info;
	}

	for(i = 0; i < DIGEST_LEN; ++i) {
		if(read_digest[i] != calc_digest[i]){
			pr_err("digset matching FAILED.\n");
			ret = -DIGEST_MISSMATCH;
			goto fail_info;
		}
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
	if (ret)
		pr_info("verifying ERROR(%d)\n", ret);
	else
		pr_info("verified successfully!!!\n");

fail_info:
	if (signature)
		kfree(signature);
fail_signature:
	if (sig)
		kfree(sig);
fail_sig:
	if (rsa_pub_key)
		kfree(rsa_pub_key);

	return ret;
}
EXPORT_SYMBOL(spu_fireware_signature_verify);