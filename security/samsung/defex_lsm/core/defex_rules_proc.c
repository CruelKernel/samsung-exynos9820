/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/kobject.h>
#include <linux/types.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
#include <linux/mm.h>
#endif /* < KERNEL_VERSION(4, 12, 0) */

#include "include/defex_debug.h"
#include "include/defex_internal.h"
#include "include/defex_rules.h"
#include "include/defex_sign.h"
#ifdef DEFEX_TRUSTED_MAP_ENABLE
#include "include/defex_tailer.h"
#include "include/ptree.h"
#endif

#define LOAD_FLAG_DPOLICY		0x01
#define LOAD_FLAG_DPOLICY_SYSTEM	0x02
#define LOAD_FLAG_SYSTEM_FIRST		0x04
#define LOAD_FLAG_TIMEOUT		0x08
#define LOAD_FLAG_RECOVERY		0x10

#define DEFEX_RULES_ARRAY_SIZE_MIN	64
#define DEFEX_RULES_ARRAY_SIZE_FIXED	(32 * 1024)
#define DEFEX_RULES_ARRAY_SIZE_MAX	(256 * 1024)


/*
 * Variant 1: Platform build, use static packed rules array
 */
#include "defex_packed_rules.inc"

#ifdef DEFEX_RAMDISK_ENABLE
/*
 * Variant 2: Platform build, load rules from kernel ramdisk or system partition
 */
#ifdef DEFEX_SIGN_ENABLE
#include "include/defex_sign.h"
#endif
#if (DEFEX_RULES_ARRAY_SIZE < 8)
#undef DEFEX_RULES_ARRAY_SIZE
#define DEFEX_RULES_ARRAY_SIZE	DEFEX_RULES_ARRAY_SIZE_MIN
#endif
#ifdef DEFEX_KERNEL_ONLY
#undef DEFEX_RULES_ARRAY_SIZE
#define DEFEX_RULES_ARRAY_SIZE	DEFEX_RULES_ARRAY_SIZE_MAX
__visible_for_testing unsigned char packed_rules_primary[DEFEX_RULES_ARRAY_SIZE] = {0};
#else
#if (DEFEX_RULES_ARRAY_SIZE < DEFEX_RULES_ARRAY_SIZE_FIXED)
#undef DEFEX_RULES_ARRAY_SIZE
#define DEFEX_RULES_ARRAY_SIZE	DEFEX_RULES_ARRAY_SIZE_FIXED
#endif
__visible_for_testing unsigned char packed_rules_primary[DEFEX_RULES_ARRAY_SIZE] __ro_after_init = {0};
#endif /* DEFEX_KERNEL_ONLY */
static unsigned char *packed_rules_secondary;
#ifdef DEFEX_TRUSTED_MAP_ENABLE
struct PPTree dtm_tree;
#endif

#endif /* DEFEX_RAMDISK_ENABLE */

#ifdef DEFEX_TRUSTED_MAP_ENABLE
/* In loaded policy, title of DTM's section; set by tailer -t in buildscript/build_external/defex. */
#define DEFEX_DTM_SECTION_NAME "dtm_rules"
#endif

#ifdef DEFEX_INTEGRITY_ENABLE

#include <linux/fs.h>
#include <crypto/hash.h>
#include <crypto/public_key.h>
#include <crypto/internal/rsa.h>
#include "../../integrity/integrity.h"
#define SHA256_DIGEST_SIZE 32
#endif /* DEFEX_INTEGRITY_ENABLE */

struct rules_file_struct {
	char *name;
	int flags;
};

static const struct rules_file_struct rules_files[4] = {
	{ "/dpolicy",			LOAD_FLAG_DPOLICY },
	{ "/first_stage_ramdisk/dpolicy", LOAD_FLAG_DPOLICY },
	{ "/vendor/etc/dpolicy",	LOAD_FLAG_DPOLICY },
	{ "/dpolicy_system",		LOAD_FLAG_DPOLICY_SYSTEM }
};
static volatile unsigned int load_flags;


__visible_for_testing char *get_rules_ptr(int is_system)
{
	if (load_flags & LOAD_FLAG_SYSTEM_FIRST)
		is_system = !is_system;

	return is_system ? (char *)packed_rules_secondary : (char *)packed_rules_primary;
}

__visible_for_testing int get_rules_size(int is_system)
{
	struct rule_item_struct *rules_ptr = (struct rule_item_struct *)get_rules_ptr(is_system);

	return rules_ptr ? rules_ptr->data_size : 0;
}

int check_rules_ready(void)
{
	struct rule_item_struct *base = (struct rule_item_struct *)packed_rules_primary;

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
			pr_alert("[DEFEX] recovery mode\n");
			filp_close(fp, NULL);
			load_flags |= LOAD_FLAG_RECOVERY;
		} else {
			pr_alert("[DEFEX] normal mode\n");
		}

		mount_system_root = 0;
		fp = local_fopen("/system_root", O_DIRECTORY | O_PATH, 0);
		if (!IS_ERR(fp)) {
			filp_close(fp, NULL);
			mount_system_root = 1;
			pr_alert("[DEFEX] system_root=TRUE\n");
		} else {
			pr_alert("[DEFEX] system_root=FALSE\n");
		}
	}
	return (mount_system_root > 0);
}

#ifdef DEFEX_INTEGRITY_ENABLE
__visible_for_testing int defex_check_integrity(struct file *f, unsigned char *hash)
{
	struct crypto_shash *handle = NULL;
	struct shash_desc *shash = NULL;
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
	shash = (struct shash_desc *)kvzalloc(sizeof(struct shash_desc) + crypto_shash_descsize(handle), GFP_KERNEL);
#else
	shash = kzalloc(sizeof(struct shash_desc) + crypto_shash_descsize(handle), GFP_KERNEL);
#endif /* < KERNEL_VERSION(4, 12, 0) */

	if (shash == NULL)
		goto hash_error;

	shash->tfm = handle;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
	buff = kvmalloc(buff_size, GFP_KERNEL);
#else
	buff = kmalloc(buff_size, GFP_KERNEL);
#endif /* < KERNEL_VERSION(4, 12, 0) */

	if (buff == NULL)
		goto hash_error;

	err = crypto_shash_init(shash);
	if (err < 0)
		goto hash_error;


	while (1) {
		read_size = local_fread(f, file_size, (char *)buff, buff_size);
		if (read_size < 0)
			goto hash_error;
		if (read_size == 0)
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
	kvfree(buff);
	kvfree(shash);
#else
	kfree(buff);
	kfree(shash);
#endif /* < KERNEL_VERSION(4, 12, 0) */

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

#if defined(DEFEX_RAMDISK_ENABLE)

#ifdef DEFEX_TRUSTED_MAP_ENABLE
static const unsigned char *find_policy_section(const char *name, const char *data, int data_size, long *section_size)
{
	return data_size > 0 ? defex_tailerp_find(data, data_size, name, section_size) : 0;
}
#endif

__visible_for_testing int load_rules_common(struct file *f, int flags)
{
	int res = -1, data_size, rules_size;
	unsigned char *data_buff = NULL;

	data_size = i_size_read(file_inode(f));
	if (data_size <= 0 || data_size > DEFEX_RULES_ARRAY_SIZE_MAX)
		goto do_clean;
	data_buff = vmalloc(data_size);
	if (!data_buff)
		goto do_clean;

	rules_size = local_fread(f, 0, data_buff, data_size);
	if (rules_size <= 0) {
		pr_err("[DEFEX] Failed to read rules file (%d)\n", rules_size);
		goto do_clean;
	}
	pr_info("[DEFEX] Read %d bytes.\n", rules_size);

#ifdef DEFEX_SIGN_ENABLE
	res = defex_rules_signature_check((char *)data_buff, (unsigned int)rules_size, (unsigned int *)&rules_size);

	if (!res)
		pr_info("[DEFEX] Rules signature verified successfully.\n");
	else
		pr_err("[DEFEX] Rules signature incorrect!!!\n");
#else
	res = 0;
#endif

	if (!res) {
		const unsigned char *policy_data = NULL; /* where additional features like DTM could look for policy data */
		if (!(load_flags & (LOAD_FLAG_DPOLICY | LOAD_FLAG_DPOLICY_SYSTEM))) {
			if (rules_size > sizeof(packed_rules_primary)) {
				res = -1;
				goto do_clean;
			}
			memcpy(packed_rules_primary, data_buff, rules_size);
			policy_data = packed_rules_primary;
			if (flags & LOAD_FLAG_DPOLICY_SYSTEM)
				load_flags |= LOAD_FLAG_SYSTEM_FIRST;
		} else {
			if (rules_size > 0) {
				policy_data = packed_rules_secondary = data_buff;
				data_buff = NULL;
			}
		}
#ifdef DEFEX_TRUSTED_MAP_ENABLE
		if (policy_data && !dtm_tree.data) { /* DTM not yet initialized */
			const unsigned char *dtm_section = find_policy_section(DEFEX_DTM_SECTION_NAME, policy_data, rules_size, 0);
			if (dtm_section)
				pptree_set_data(&dtm_tree, dtm_section);
		}
#endif
		load_flags |= flags;
		res = rules_size;
	}

do_clean:
	filp_close(f, NULL);
	vfree(data_buff);
	return res;
}

int load_rules_late(unsigned int is_system)
{
	struct file *f = NULL;
	int f_index, res = 0;
	static unsigned long start_time;
	static unsigned long last_time;
	unsigned long cur_time = get_seconds();
	static DEFINE_SPINLOCK(load_lock);
	static atomic_t in_progress = ATOMIC_INIT(0);
	const struct rules_file_struct *item;

	if (load_flags & LOAD_FLAG_TIMEOUT)
		return -1;

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
	/* Load has been attempted for 20 seconds, give up. */
	if ((cur_time - start_time) > 20) {
		res = -1;
		load_flags |= LOAD_FLAG_TIMEOUT;
		goto do_exit;
	}
	last_time = cur_time;

	if (is_system > 1) {
		pr_info("[DEFEX] Something wrong input.\n");
		res = -1;
		goto do_exit;
	}

	for (f_index = 0; f_index < ARRAY_SIZE(rules_files); f_index++) {
		item = &rules_files[f_index];
		if (item->flags & (is_system ? LOAD_FLAG_DPOLICY_SYSTEM : LOAD_FLAG_DPOLICY)) {
			f = local_fopen(item->name, O_RDONLY, 0);
			if (!IS_ERR_OR_NULL(f)) {
				pr_info("[DEFEX] Late load rules file: %s.\n", item->name);
				break;
			}
		}
	}
	if (IS_ERR_OR_NULL(f)) {
#ifdef DEFEX_KERNEL_ONLY
		pr_err("[DEFEX] Failed to open rules file (%ld)\n", (long)PTR_ERR(f));
#endif /* DEFEX_KERNEL_ONLY */
		goto do_exit;
	}

	res = load_rules_common(f, item->flags);
	res = (res < 0) ? res : (res > 0);

do_exit:
	atomic_set(&in_progress, 0);
	return res;
}

int __init do_load_rules(void)
{
	struct file *f = NULL;
	int res = -1;
	unsigned int f_index = 0;
	const struct rules_file_struct *item;
	static int executed __initdata;

	if (!executed) {
		memset(packed_rules_primary, 0, sizeof(packed_rules_primary));
		executed = 1;
	}
	if (boot_state_recovery)
		load_flags |= LOAD_FLAG_RECOVERY;

load_next:
	while (f_index < ARRAY_SIZE(rules_files)) {
		item = &rules_files[f_index];
		if (!(load_flags & item->flags)) {
			f = local_fopen(item->name, O_RDONLY, 0);
			if (!IS_ERR_OR_NULL(f)) {
				pr_info("[DEFEX] Load rules file: %s.\n", item->name);
				break;
			}
		}
		f_index++;
	};

	if (f_index == ARRAY_SIZE(rules_files)) {
		if (load_flags & (LOAD_FLAG_DPOLICY_SYSTEM | LOAD_FLAG_DPOLICY))
			return 0;
		pr_err("[DEFEX] Failed to open rules file (%ld)\n", (long)PTR_ERR(f));

#ifdef DEFEX_KERNEL_ONLY
		if (load_flags & LOAD_FLAG_RECOVERY)
			res = 0;
#endif /* DEFEX_KERNEL_ONLY */
		return res;
	}

	res = load_rules_common(f, item->flags);
	res = (res < 0) ? res : 0;

#ifdef DEFEX_KERNEL_ONLY
	if ((load_flags & LOAD_FLAG_RECOVERY) && res != 0) {
		res = 0;
		pr_info("[DEFEX] Kernel Only & recovery mode, rules loading is passed.\n");
	}
#endif
	if (++f_index < ARRAY_SIZE(rules_files))
		goto load_next;
	return res;
}

#endif /* DEFEX_RAMDISK_ENABLE */

__visible_for_testing struct rule_item_struct *lookup_dir(struct rule_item_struct *base,
								const char *name,
								int l,
								int for_recovery,
								char *base_start)
{
	struct rule_item_struct *item = NULL;
	unsigned int offset;

	if (!base || !base->next_level || !base_start)
		return item;
	item = GET_ITEM_PTR(base->next_level, base_start);
	do {
		if ((!(item->feature_type & feature_is_file)
			 || (!!(item->feature_type & feature_for_recovery)) == for_recovery)
				&& item->size == l && !memcmp(name, item->name, l))
			return item;
		offset = item->next_file;
		item = GET_ITEM_PTR(offset, base_start);
	} while (offset);
	return NULL;
}

__visible_for_testing int lookup_tree(const char *file_path, int attribute, struct file *f)
{
	const char *ptr, *next_separator;
	struct rule_item_struct *base, *cur_item = NULL;
	int l;
	char *base_start;
	unsigned int is_system = 0, is_recovery = !!(load_flags & LOAD_FLAG_RECOVERY);

	if (!file_path || *file_path != '/')
		return 0;

	is_system = ((strncmp("/system/", file_path, 8) == 0) ||
			(strncmp("/product/", file_path, 9) == 0) ||
			(strncmp("/apex/", file_path, 6) == 0) ||
			(strncmp("/system_ext/", file_path, 12) == 0))?1:0;
try_to_load:

	if (!(load_flags & LOAD_FLAG_DPOLICY)) {
		l = load_rules_late(0);
		if (l > 0 && !is_system)
			goto try_to_load;
		/* allow all requests if rules were not loaded for Recovery mode */
		if (!is_system && (!l || is_recovery))
			return (attribute == feature_ped_exception || attribute == feature_safeplace_path)?1:0;
	}

	if (!(load_flags & LOAD_FLAG_DPOLICY_SYSTEM)) {
		l = load_rules_late(1);
		if (l > 0 && is_system)
			goto try_to_load;
		/* allow all requests if rules were not loaded for Recovery mode */
		if (is_system && (!l || is_recovery))
			return (attribute == feature_ped_exception || attribute == feature_safeplace_path)?1:0;
	}

try_not_system:

	base_start = get_rules_ptr(get_rules_size(is_system) ? is_system : (!is_system));
	base = (struct rule_item_struct *)base_start;

	if (!base || !base->data_size) {
		/* block all requests if rules were not loaded */
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
		cur_item = lookup_dir(base, ptr, l, is_recovery, base_start);
		if (!cur_item)
			cur_item = lookup_dir(base, ptr, l, !is_recovery, base_start);
		if (!cur_item)
			break;
		if (cur_item->feature_type & attribute) {
#ifdef DEFEX_INTEGRITY_ENABLE
			/* Integrity acceptable only for files */
			if ((cur_item->feature_type & feature_integrity_check) &&
				(cur_item->feature_type & feature_is_file) && f) {
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
	} while (*ptr);
	if (is_system && ((void *)base_start == (void *)get_rules_ptr(is_system))) {
		is_system = !is_system;
		goto try_not_system;
	}
	return 0;
}

int rules_lookup(const char *target_file, int attribute, struct file *f)
{
	int ret = 0;
#if (defined(DEFEX_SAFEPLACE_ENABLE) || defined(DEFEX_IMMUTABLE_ENABLE) || defined(DEFEX_PED_ENABLE))
	static const char system_root_txt[] = "/system_root";

	if (check_system_mount() &&
		!strncmp(target_file, system_root_txt, sizeof(system_root_txt) - 1))
		target_file += (sizeof(system_root_txt) - 1);

	ret = lookup_tree(target_file, attribute, f);
#endif
	return ret;
}

void __init defex_load_rules(void)
{
#if defined(DEFEX_RAMDISK_ENABLE)
	if (!boot_state_unlocked && do_load_rules() != 0) {
#if !(defined(DEFEX_DEBUG_ENABLE) || defined(DEFEX_KERNEL_ONLY))
		panic("[DEFEX] Signature mismatch.\n");
#endif
	}
#endif /* DEFEX_RAMDISK_ENABLE */
}
