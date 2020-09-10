/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/async.h>
#include <linux/delay.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/initrd.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/sysfs.h>
#include <linux/time.h>
#include <linux/version.h>
#include "include/defex_debug.h"
#include "include/defex_internal.h"
#include "include/defex_rules.h"

#define DEFEX_RULES_FILE "/dpolicy"

#ifndef DEFEX_USE_PACKED_RULES
/*
 * Variant 1: Use the static, unpacked rules array
 */
#ifdef DEFEX_INTEGRITY_ENABLE
#error "Packed rules required for Integrity feature"
#endif

#else

/*
 * Variant 2: Platform build, use static packed rules array
 */
#include "defex_packed_rules.inc"

#ifdef DEFEX_RAMDISK_ENABLE
/*
 * Variant 3: Platform build, load rules from kernel ramdisk or system partition
 */
#ifdef DEFEX_SIGN_ENABLE
#include "include/defex_sign.h"
#endif
#if (DEFEX_RULES_ARRAY_SIZE < 8)
#undef DEFEX_RULES_ARRAY_SIZE
#define DEFEX_RULES_ARRAY_SIZE	sizeof(struct rule_item_struct)
#endif
#ifdef DEFEX_KERNEL_ONLY
#undef DEFEX_RULES_ARRAY_SIZE
#define DEFEX_RULES_ARRAY_SIZE	(256 * 1024)
static unsigned char defex_packed_rules[DEFEX_RULES_ARRAY_SIZE] = {0};
int load_rules_late(void);
#else
static unsigned char defex_packed_rules[DEFEX_RULES_ARRAY_SIZE] __ro_after_init = {0};
#endif /* DEFEX_KERNEL_ONLY */
#endif /* DEFEX_RAMDISK_ENABLE */

#endif /* DEFEX_USE_PACKED_RULES */

#ifdef DEFEX_INTEGRITY_ENABLE

#include <linux/fs.h>
#include <crypto/hash.h>
#include <crypto/public_key.h>
#include <crypto/internal/rsa.h>
#include "../../integrity/integrity.h"
#define SHA256_DIGEST_SIZE 32
#endif /* DEFEX_INTEGRITY_ENABLE */

static struct kset *defex_kset;
static int is_recovery = 0;

__visible_for_testing int __init bootmode_setup(char *str)
{
	if (str && *str == '2') {
		is_recovery = 1;
		printk(KERN_ALERT "[DEFEX] recovery mode setup\n");
	}
	return 0;
}
__setup("bootmode=", bootmode_setup);

int check_rules_ready(void)
{
	struct rule_item_struct *base = (struct rule_item_struct *)defex_packed_rules;
	return (!base || !base->data_size)?0:1;
}

__visible_for_testing int check_system_mount(void)
{
	static int mount_system_root = -1;
	struct file *fp;

	if (mount_system_root < 0) {
		fp = local_fopen("/sbin/recovery", O_RDONLY, 0);
		if (IS_ERR(fp))
			fp = local_fopen("/system/bin/recovery", O_RDONLY, 0);

		if (!IS_ERR(fp)) {
			printk(KERN_ALERT "[DEFEX] recovery mode\n");
			filp_close(fp, NULL);
			is_recovery = 1;
		} else {
			printk(KERN_ALERT "[DEFEX] normal mode\n");
		}

		mount_system_root = 0;
		fp = local_fopen("/system_root", O_DIRECTORY | O_PATH, 0);
		if (!IS_ERR(fp)) {
			filp_close(fp, NULL);
			mount_system_root = 1;
			printk(KERN_ALERT "[DEFEX] system_root=TRUE\n");
		} else {
			printk(KERN_ALERT "[DEFEX] system_root=FALSE\n");
		}
	}
	return (mount_system_root > 0);
}

__visible_for_testing void parse_static_rules(const struct static_rule *rules, size_t max_len, int rules_number)
{
	int i;
	size_t count;
	const char *current_rule;
#if (defined(DEFEX_PERMISSIVE_PED) || defined(DEFEX_PERMISSIVE_SP) || defined(DEFEX_PERMISSIVE_IM))
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
#ifdef DEFEX_IMMUTABLE_ENABLE
		case feature_immutable_status:
#ifdef DEFEX_PERMISSIVE_IM
			current_rule = permissive;
#endif /* DEFEX_PERMISSIVE_IM */
			immutable_status_store(global_immutable_obj, NULL, current_rule, count);
			break;
#endif /* DEFEX_IMMUTABLE_ENABLE */
		}
	}
}


#ifdef DEFEX_INTEGRITY_ENABLE
__visible_for_testing int defex_check_integrity(struct file *f, unsigned char *hash)
{
	struct crypto_shash *handle = NULL;
	struct shash_desc* shash = NULL;
	static const unsigned char buff_zero[SHA256_DIGEST_SIZE] = {0};
	unsigned char hash_sha256[SHA256_DIGEST_SIZE];
	unsigned char *buff = NULL;
	size_t buff_size = PAGE_SIZE;
	loff_t file_size = 0;
	int ret = 0, err = 0, read_size = 0;

	// A saved hash is zero, skip integrity check
	if (!memcmp(buff_zero, hash, SHA256_DIGEST_SIZE))
		return ret;

	if (IS_ERR(f))
		goto hash_error;

	handle = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR(handle)) {
		err = PTR_ERR(handle);
		pr_err("[DEFEX] Can't alloc sha256, error : %d", err);
		return -1;
	}

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
		read_size = local_fread(f, file_size, (char*)buff, buff_size);
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

__visible_for_testing int defex_integrity_default(const char *file_path)
{
	static const char integrity_default[] = "/system/bin/install-recovery.sh";
	return strncmp(integrity_default, file_path, sizeof(integrity_default));
}

#endif /* DEFEX_INTEGRITY_ENABLE */

#ifdef DEFEX_USE_PACKED_RULES
__visible_for_testing struct rule_item_struct *lookup_dir(struct rule_item_struct *base, const char *name, int l, int for_recovery)
{
	struct rule_item_struct *item = NULL;
	unsigned int offset;

	if (!base || !base->next_level)
		return item;
	item = GET_ITEM_PTR(base->next_level);
	do {
		if ((!(item->feature_type & feature_is_file)
			|| (!!(item->feature_type & feature_for_recovery)) == for_recovery)
			&& item->size == l
			&& !memcmp(name, item->name, l)) return item;
		offset = item->next_file;
		item = GET_ITEM_PTR(offset);
	} while(offset);
	return NULL;
}

__visible_for_testing int lookup_tree(const char *file_path, int attribute, struct file *f)
{
	const char *ptr, *next_separator;
	struct rule_item_struct *base, *cur_item = NULL;
	int l;

	if (!file_path || *file_path != '/')
		return 0;

#ifdef DEFEX_KERNEL_ONLY
try_to_load:
#endif
	base = (struct rule_item_struct *)defex_packed_rules;
	if (!base || !base->data_size) {
#ifdef DEFEX_KERNEL_ONLY
		/* allow all requests if rules were not loaded for Recovery mode */
		l = load_rules_late();
		if (l > 0)
			goto try_to_load;
		if (!l || is_recovery)
			return (attribute == feature_ped_exception || attribute == feature_safeplace_path)?1:0;
#endif /* DEFEX_KERNEL_ONLY */
		/* block all requests if rules were not loaded instead */
		return 0;
	}

	ptr = file_path + 1;
	do {
		next_separator = strchr(ptr, '/');
		if (!next_separator)
			l = strlen(ptr);
		else
			l = next_separator - ptr;
		if (!l)
			return 0;
		cur_item = lookup_dir(base, ptr, l, is_recovery);
		if (!cur_item)
			cur_item = lookup_dir(base, ptr, l, !is_recovery);

		if (!cur_item)
			break;
		if (cur_item->feature_type & attribute) {
#ifdef DEFEX_INTEGRITY_ENABLE
			/* Integrity acceptable only for files */
			if ((cur_item->feature_type & feature_is_file) && f) {
				if (defex_integrity_default(file_path)
					&& defex_check_integrity(f, cur_item->integrity))
					return DEFEX_INTEGRITY_FAIL;
			}
#endif /* DEFEX_INTEGRITY_ENABLE */
			if (attribute & (feature_immutable_path_open | feature_immutable_path_write)
				&& !(cur_item->feature_type & feature_is_file)) {
				/* Allow open the folder by default */
				if (!next_separator || *(ptr + l + 1) == 0)
					return 0;
			}
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

int rules_lookup2(const char *target_file, int attribute, struct file *f)
{
	int ret = 0;
#if (defined(DEFEX_SAFEPLACE_ENABLE) || defined(DEFEX_IMMUTABLE_ENABLE) || defined(DEFEX_PED_ENABLE))
	static const char system_root_txt[] = "/system_root";
#ifndef DEFEX_USE_PACKED_RULES
	int i, count, end;
	const struct static_rule *current_rule;
#endif
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
#endif
	return ret;
}

int rules_lookup(const struct path *dpath, int attribute, struct file *f)
{
	int ret = 0;
	char *target_file, *buff;

	buff = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!buff)
		return ret;
	target_file = d_path(dpath, buff, PATH_MAX);
	if (!IS_ERR(target_file)) {
		ret = rules_lookup2(target_file, attribute, f);
	}
	kfree(buff);
	return ret;
}


int __init defex_init_sysfs(void)
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

#ifdef DEFEX_IMMUTABLE_ENABLE
	global_immutable_obj = task_defex_create_immutable_obj(defex_kset);
	if (!global_immutable_obj)
		goto immutable_error;
#endif /* DEFEX_IMMUTABLE_ENABLE */

	parse_static_rules(defex_static_rules, STATIC_RULES_MAX_STR, static_rule_count);
	return 0;

#ifdef DEFEX_IMMUTABLE_ENABLE
immutable_error:
#endif /* DEFEX_IMMUTABLE_ENABLE */

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


#if defined(DEFEX_RAMDISK_ENABLE)

#ifdef DEFEX_KERNEL_ONLY
int load_rules_late(void)
{
	struct file *f;
	int rc, data_size, rules_size, res = 0;
	unsigned char *data_buff;
	static unsigned long start_time = 0;
	static unsigned long last_time = 0;
	unsigned long cur_time = get_seconds();
	static DEFINE_SPINLOCK(load_lock);
	static atomic_t in_progress = ATOMIC_INIT(0);

	if (!spin_trylock(&load_lock))
		return res;

	if (atomic_read(&in_progress) != 0) {
		spin_unlock(&load_lock);
		return res;
	}

	atomic_set(&in_progress, 1);
	spin_unlock(&load_lock);

	/* The first try to load, initialize time values */
	if (!start_time)
		start_time = get_seconds();
	/* Skip this try, wait for next second */
	if (cur_time == last_time)
		goto do_exit;
	/* Load has been attempted for 30 seconds, give up. */
	if ((cur_time - start_time) > 30) {
		res = -1;
		goto do_exit;
	}
	last_time = cur_time;

	f = local_fopen(DEFEX_RULES_FILE, O_RDONLY, 0);
	if (IS_ERR(f)) {
		rc = PTR_ERR(f);
		pr_err("[DEFEX] Failed to open rules file (%d)\n", rc);
		goto do_exit;
	}

	data_size = i_size_read(file_inode(f));
	if (data_size <= 0)
		goto do_exit;
	data_buff = kmalloc(data_size, GFP_KERNEL);
	if (!data_buff)
		goto do_exit;

	rules_size = local_fread(f, 0, data_buff, data_size);
	printk(KERN_INFO "[DEFEX] Late load rules file: %s.\n", DEFEX_RULES_FILE);
	printk(KERN_INFO "[DEFEX] Read %d bytes.\n", rules_size);
	filp_close(f, NULL);

	memcpy(defex_packed_rules, data_buff, rules_size);
	kfree(data_buff);
	res = (rules_size > 0);
do_exit:
	atomic_set(&in_progress, 0);
	return res;
}
#endif /* DEFEX_KERNEL_ONLY */

__visible_for_testing int __init do_load_rules(void)
{
	struct file *f;
	int res = -1, rc, data_size, rules_size;
	unsigned char *data_buff;

	memset(defex_packed_rules, 0, sizeof(defex_packed_rules));
	printk(KERN_INFO "[DEFEX] Load rules file: %s.\n", DEFEX_RULES_FILE);
	f = local_fopen(DEFEX_RULES_FILE, O_RDONLY, 0);
	if (IS_ERR(f)) {
		rc = PTR_ERR(f);
		pr_err("[DEFEX] Failed to open rules file (%d)\n", rc);
#ifdef DEFEX_KERNEL_ONLY
		if (is_recovery)
			res = 0;
#endif /* DEFEX_KERNEL_ONLY */
		return res;
	}

	data_size = i_size_read(file_inode(f));
	if (data_size <= 0)
		return res;
	data_buff = kmalloc(data_size, GFP_KERNEL);
	if (!data_buff)
		return res;

	rules_size = local_fread(f, 0, data_buff, data_size);
	printk(KERN_INFO "[DEFEX] Read %d bytes.\n", rules_size);
	filp_close(f, NULL);

#ifdef DEFEX_SIGN_ENABLE
	res = defex_rules_signature_check(data_buff, rules_size, &rules_size);
	if (!res && rules_size > sizeof(defex_packed_rules))
		res = -1;

	if (!res)
		printk("[DEFEX] Rules signature verified successfully.\n");
	else
		pr_err("[DEFEX] Rules signature incorrect!!!\n");
#else
	res = 0;
#endif

	if (!res)
		memcpy(defex_packed_rules, data_buff, rules_size);
	kfree(data_buff);

#ifdef DEFEX_KERNEL_ONLY
	if (is_recovery && res != 0) {
		res = 0;
		printk("[DEFEX] Kernel Only & recovery mode, rules loading is passed.\n");
	}
#endif

	return res;
}

#endif /* DEFEX_RAMDISK_ENABLE */

void __init defex_load_rules(void)
{
#if defined(DEFEX_RAMDISK_ENABLE)
	if ( !boot_state_unlocked && do_load_rules() != 0) {
#if !(defined(DEFEX_DEBUG_ENABLE) || defined(DEFEX_KERNEL_ONLY))
		panic("[DEFEX] Signature mismatch.\n");
#endif
	}
#endif /* DEFEX_RAMDISK_ENABLE */
}
