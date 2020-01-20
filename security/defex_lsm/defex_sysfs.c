/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/sysfs.h>
#include "include/defex_debug.h"
#include "include/defex_internal.h"
#include "include/defex_rules.h"

#ifdef DEFEX_USE_PACKED_RULES
#if defined(DEFEX_KERNEL_ONLY) && defined(DEFEX_INTEGRITY_ENABLE)
#else
#include "defex_packed_rules.inc"
#endif
#endif

#ifdef DEFEX_INTEGRITY_ENABLE
#include <linux/fs.h>
#include <crypto/hash.h>
#include <crypto/public_key.h>
#include <crypto/internal/rsa.h>
#include "../integrity/integrity.h"
#define SHA256_DIGEST_SIZE 32
#ifdef DEFEX_KERNEL_ONLY
#define RULES "/system/etc/defex_packed_rules.bin"
unsigned char *defex_packed_rules;
#endif /* DEFEX_KERNEL_ONLY */
#endif /* DEFEX_INTEGRITY_ENABLE */

static struct kset *defex_kset;


int check_system_mount(void)
{
	static int mount_system_root = -1;
	struct file *fp;

	if (mount_system_root < 0) {
		fp = filp_open("/sbin/recovery", O_RDONLY, 0);
		if (IS_ERR(fp)) {
			printk(KERN_ALERT "[DEFEX] normal mode\n");
			mount_system_root = 0;
		} else {
			printk(KERN_ALERT "[DEFEX] recovery mode\n");
			filp_close(fp, NULL);
			fp = filp_open("/system_root", O_DIRECTORY | O_PATH, 0);
			if (IS_ERR(fp)) {
				printk(KERN_ALERT "[DEFEX] system_root=FALSE\n");
				mount_system_root = 0;
			} else {
				printk(KERN_ALERT "[DEFEX] system_root=TRUE\n");
				filp_close(fp, NULL);
				mount_system_root = 1;
			}
		}
	}
	return (mount_system_root > 0);
}

static void parse_static_rules(const struct static_rule *rules, size_t max_len, int rules_number)
{
	int i;
	size_t count;
	const char *current_rule;
#if (defined(DEFEX_PERMISSIVE_PED) || defined(DEFEX_PERMISSIVE_SP))
	static const char permissive[2] = "2";
#endif /* DEFEX_PERMISSIVE_**/

	for (i = 0; i < rules_number; i++) {
		count = strnlen(rules[i].rule, max_len);
		current_rule = rules[i].rule;
		switch (rules[i].feature_type) {
#ifdef DEFEX_PED_ENABLE
		case feature_ped_status:
#ifdef DEFEX_PERMISSIVE_PED
			current_rule = permissive;
#endif /* DEFEX_PERMISSIVE_PED */
			task_defex_privesc_store_status(global_privesc_obj, NULL, current_rule, count);
			break;
#endif /* DEFEX_PED_ENABLE */
#ifdef DEFEX_SAFEPLACE_ENABLE
		case feature_safeplace_status:
#ifdef DEFEX_PERMISSIVE_SP
			current_rule = permissive;
#endif /* DEFEX_PERMISSIVE_SP */
			safeplace_status_store(global_safeplace_obj, NULL, current_rule, count);
			break;
#endif /* DEFEX_SAFEPLACE_ENABLE */
		}
	}

	printk(KERN_INFO "DEFEX_LSM started");
}

#ifdef DEFEX_USE_PACKED_RULES
struct rule_item_struct *lookup_dir(struct rule_item_struct *base, const char *name, int l)
{
	struct rule_item_struct *item = NULL;
	unsigned int offset;

	if (!base || !base->next_level)
		return item;
	item = GET_ITEM_PTR(base->next_level);
	do {
		if (item->size == l && !memcmp(name, item->name, l)) return item;
		offset = item->next_file;
		item = GET_ITEM_PTR(offset);
	} while(offset);
	return NULL;
}

#ifdef DEFEX_INTEGRITY_ENABLE
int defex_check_integrity(struct file *f, unsigned char *hash)
{
	struct crypto_shash *handle = NULL;
	struct shash_desc* shash = NULL;
	unsigned char hash_sha256[SHA256_DIGEST_SIZE];
	unsigned char *buff = NULL;
	size_t buff_size = PAGE_SIZE;
	loff_t file_size = 0;
	int ret = 0, err = 0, read_size = 0;
	int i = 0; //TEST

	if (IS_ERR(f))
		goto hash_error;

	handle = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR(handle))
		goto hash_error;

	shash = kzalloc(sizeof(struct shash_desc) + crypto_shash_descsize(handle), GFP_KERNEL);
	if (NULL == shash)
		goto hash_error;

	shash->flags = 0;
	shash->tfm = handle;

	buff = kmalloc(buff_size, GFP_KERNEL);
	if (NULL == buff)
		goto hash_error;

	err = crypto_shash_init(shash);
	if (err < 0)
		goto hash_error;


	while(1) {
		read_size = integrity_kernel_read(f, file_size, (char*)buff, buff_size);
		if (0 > read_size)
			goto hash_error;
		if (0 == read_size)
			break;
		file_size += read_size;
		err = crypto_shash_update(shash, buff, read_size);
		if (err < 0)
			goto hash_error;
	}

	err = crypto_shash_final(shash, hash_sha256);
	if (err < 0)
		goto hash_error;

	ret = memcmp(hash_sha256, hash, SHA256_DIGEST_SIZE);

	/* TEST */
	if (ret){
		for (i = 0; i < 32; i++){
			printk("%02x%02x : %d", hash_sha256[i], hash[i], ret);
		}
	}
	goto hash_exit;

  hash_error:
	ret = -1;
  hash_exit:
	if (buff)
		kfree(buff);
	if (shash)
		kfree(shash);
	if (handle)
		crypto_free_shash(handle);
	return ret;

}

int defex_integrity_default(const char *file_path)
{
	static const char integrity_default[] = "/system/bin/install-recovery.sh";
	return strncmp(integrity_default, file_path, sizeof(integrity_default));
}

#endif /* DEFEX_INTEGRITY_ENABLE */

#if defined DEFEX_INTEGRITY_ENABLE && defined DEFEX_KERNEL_ONLY
int defex_read_rules(const char *path, unsigned char **data)
{
	struct file *file;
	loff_t size;
	unsigned char *buf;
	int rc = -EINVAL;
	mm_segment_t old_fs;

	if (!path || !*path)
		return -EINVAL;

	old_fs = get_fs();
	set_fs(get_ds());
	file = filp_open(path, O_RDONLY, 0);
	set_fs(old_fs);
	if (IS_ERR(file)) {
		rc = PTR_ERR(file);
		pr_err("[DEFEX] Unable to open file: %s (%d)", path, rc);
		return rc;
	}

	size = i_size_read(file_inode(file));
	if (size <= 0)
		goto out;

	buf = kmalloc(size, GFP_KERNEL);
	if (!buf) {
		rc = -ENOMEM;
		goto out;
	}

	rc = integrity_kernel_read(file, 0, buf, size);
	if (rc == size) {
		*data = buf;
	} else {
		kfree(buf);
		if (rc >= 0)
			rc = -EIO;
	}
  out:
	fput(file);
	return rc;
}

#endif /* defined DEFEX_INTEGRITY_ENABLE && defined DEFEX_KERNEL_ONLY */

int lookup_tree(const char *file_path, int attribute, struct file *f)
{
	const char *ptr, *next_separator;
	struct rule_item_struct *base, *cur_item = NULL;
	int l;

	if (!file_path || *file_path != '/')
		return 0;


	/* load packed binary rules during the first-time access */
#if defined DEFEX_INTEGRITY_ENABLE && defined DEFEX_KERNEL_ONLY
	if (!defex_packed_rules) {
		defex_read_rules(RULES, &defex_packed_rules);
		if (!defex_packed_rules) {
			printk("[DEFEX] Rules loading Failed, process: %s\n", file_path);
			/* allow all while filesystem is not ready */
			return 1;
		} else {
			printk("[DEFEX] Rules loading OK, process: %s\n", file_path);
		}
	}
#endif /* defined DEFEX_INTEGRITY_ENABLE && defined DEFEX_KERNEL_ONLY */

	base = (struct rule_item_struct *)defex_packed_rules;
	ptr = file_path + 1;
	do {
		next_separator = strchr(ptr, '/');
		if (!next_separator)
			l = strlen(ptr);
		else
			l = next_separator - ptr;
		if (!l)
			return 0;
		cur_item = lookup_dir(base, ptr, l);
		if (!cur_item)
			break;
		if (cur_item->feature_type & attribute) {
#ifdef DEFEX_INTEGRITY_ENABLE
			if (defex_integrity_default(file_path)
				&& defex_check_integrity(f, cur_item->integrity))
				return DEFEX_INTEGRITY_FAIL;
#endif /* DEFEX_INTEGRITY_ENABLE */
			return 1;
		}
		base = cur_item;
		ptr += l;
		if (next_separator)
			ptr++;
	} while(*ptr);
	return 0;
}
#endif /* DEFEX_USE_PACKED_RULES */

int rules_lookup(const struct path *dpath, int attribute, struct file *f)
{
	int ret = 0;
	static const char system_root_txt[] = "/system_root";
#if (defined(DEFEX_SAFEPLACE_ENABLE) || defined(DEFEX_PED_ENABLE))
	char *target_file, *buff;
#ifndef DEFEX_USE_PACKED_RULES
	int i, count, end;
	const struct static_rule *current_rule;
#endif
	buff = kzalloc(PATH_MAX, GFP_ATOMIC);
	if (!buff)
		return ret;
	target_file = d_path(dpath, buff, PATH_MAX);
	if (IS_ERR(target_file)) {
		kfree(buff);
		return ret;
	}
	if (check_system_mount() &&
		!strncmp(target_file, system_root_txt, sizeof(system_root_txt) - 1))
		target_file += (sizeof(system_root_txt) - 1);

#ifdef DEFEX_USE_PACKED_RULES
	ret = lookup_tree(target_file, attribute, f);
#else
	for (i = 0; i < static_rule_count; i++) {
		current_rule = &defex_static_rules[i];
		if (current_rule->feature_type == attribute) {
			end = strnlen(current_rule->rule, STATIC_RULES_MAX_STR);
			if (current_rule->rule[end - 1] == '/') {
				count = end;
			} else {
				count = strnlen(target_file, STATIC_RULES_MAX_STR);
				if (end > count) count = end;
			}
			if (!strncmp(current_rule->rule, target_file, count)) {
				ret = 1;
				break;
			}
		}
	}
#endif /* DEFEX_USE_PACKED_RULES */
	kfree(buff);
#endif
	return ret;
}

int defex_init_sysfs(void)
{
	defex_kset = kset_create_and_add("defex", NULL, NULL);
	if (!defex_kset)
		return -ENOMEM;

#if defined(DEFEX_DEBUG_ENABLE) && defined(DEFEX_SYSFS_ENABLE)
	if (defex_create_debug(defex_kset) != DEFEX_OK)
		goto kset_error;
#endif /* DEFEX_DEBUG_ENABLE && DEFEX_SYSFS_ENABLE */

#ifdef DEFEX_PED_ENABLE
	global_privesc_obj = task_defex_create_privesc_obj(defex_kset);
	if (!global_privesc_obj)
		goto privesc_error;
#endif /* DEFEX_PED_ENABLE */

#ifdef DEFEX_SAFEPLACE_ENABLE
	global_safeplace_obj = task_defex_create_safeplace_obj(defex_kset);
	if (!global_safeplace_obj)
		goto safeplace_error;
#endif /* DEFEX_SAFEPLACE_ENABLE */

	parse_static_rules(defex_static_rules, STATIC_RULES_MAX_STR, static_rule_count);
	return 0;

#ifdef DEFEX_SAFEPLACE_ENABLE
	task_defex_destroy_safeplace_obj(global_safeplace_obj);
  safeplace_error:
#endif /* DEFEX_SAFEPLACE_ENABLE */

#ifdef DEFEX_PED_ENABLE
	task_defex_destroy_privesc_obj(global_privesc_obj);
  privesc_error:
#endif /* DEFEX_PED_ENABLE */

#if defined(DEFEX_DEBUG_ENABLE) && defined(DEFEX_SYSFS_ENABLE)
  kset_error:
	kset_unregister(defex_kset);
	defex_kset = NULL;
#endif /* DEFEX_DEBUG_ENABLE && DEFEX_SYSFS_ENABLE */
	return -ENOMEM;
}
