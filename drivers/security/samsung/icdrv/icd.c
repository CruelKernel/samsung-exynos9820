/*
 * ICD Driver
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Egor Uleyskiy, <e.uleyskiy@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/version.h>
#include "oemflag.h"
#include "icd_protect_list.h"
#include "five_hooks.h"
#include "icd.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/mm.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
#include <linux/string_helpers.h>
#endif

static void icd_hook_integrity_reset(struct task_struct *task,
					struct file *file, enum task_integrity_reset_cause cause);

static struct five_hook_list five_ops[] = {
	FIVE_HOOK_INIT(integrity_reset2, icd_hook_integrity_reset),
};

__visible_for_testing bool contains_str(const char * const array[], const char *str)
{
	const char * const *p;

	for (p = &array[0]; *p != NULL; ++p) {
		if (strncmp(str, *p, PATH_MAX) == 0)
			return true;
	}
	return false;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
static void fix_dpath(const struct path *path, char *pathbuf, char *pathname)
{
	/* `d_path' appends " (deleted)" string if a file is unlinked. Below
	 * code removes it.
	 * `d_path' fills the buffer from the end of it therefore we can easily
	 * calculate the length of the pathname.
	 */
	const long pathname_size = pathbuf + PATH_MAX - pathname;
	static const char str_deleted[] = " (deleted)";
	const int deleted_size = sizeof(str_deleted);

	if (pathname_size > deleted_size && d_unlinked(path->dentry)) {
		char *start_deleted = pathbuf + PATH_MAX - deleted_size;

		if (!strncmp(str_deleted, start_deleted, deleted_size))
			*start_deleted = '\0';
	}
}
#else
static const char *get_first_argument_from_cmdline(struct task_struct *task)
{
	char *buffer, *quoted;
	int i, res, pos = 0;

	buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buffer)
		return NULL;

	res = get_cmdline(task, buffer, PAGE_SIZE - 1);
	buffer[res] = '\0';

	/* Collapse trailing NULLs, leave res pointing to last non-NULL. */
	while (--res >= 0 && buffer[res] == '\0')
		;

	/* Find first argument */
	for (i = 0; i <= res; i++) {
		if (buffer[i] == '\0') {
			pos = i;
			break;
		}
	}
	if (pos == 0) {
		if(buffer)
			kfree(buffer);
		return NULL;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	/* Make sure result is printable. */
	quoted = kstrdup_quotable(buffer+pos+1, GFP_KERNEL);
#else
	quoted = kmalloc(PAGE_SIZE, GFP_KERNEL);
	strncpy(quoted, buffer+pos+1, strlen(buffer+pos+1));
#endif
	kfree(buffer);
	return (const char *)quoted;
}
#endif

static bool cmdline_check(struct task_struct *task, const char *str)
{
	const char *first_arg = NULL;
	bool ret = false;

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
	first_arg = kstrdup_quotable_cmdline(task, GFP_KERNEL);
#else
	first_arg = get_first_argument_from_cmdline(task);
#endif

	if (first_arg != NULL) {
		pr_debug("IOF drv: first_arg: %s\n", first_arg);
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
		if (strnstr(first_arg, str, strlen(first_arg)) != NULL)
#else
		if (!strncmp(first_arg, str, strlen(str)))
#endif
			ret = true;
	}
	kfree(first_arg);
	return ret;
}

enum oemflag_id affected_oemflag_id(struct task_struct *task, const char *path)
{
	if (contains_str(tz_drm_list, path)
			|| cmdline_check(task, "--TZ_DRM"))
		return OEMFLAG_TZ_DRM;
	if (contains_str(fido_list, path)
			|| cmdline_check(task, "--FIDO"))
		return OEMFLAG_FIDO;
	if (contains_str(cc_list, path)
			|| cmdline_check(task, "--CC"))
		return OEMFLAG_CC;
	if (contains_str(etc_list, path)
			|| cmdline_check(task, "--ETC"))
		return OEMFLAG_ETC;

	return OEMFLAG_NONE;
}

static const char *get_exec_path(struct task_struct *task, char **pathbuf)
{
	struct mm_struct *mm;
	struct file *exe_file;
	struct path *exe_path;
	char *pathname = NULL;

	mm = get_task_mm(task);
	if (!mm)
		return ERR_PTR(-ENOENT);
	exe_file = get_mm_exe_file(mm);
	mmput(mm);
	if (!exe_file)
		return ERR_PTR(-ENOENT);
	exe_path = &exe_file->f_path;
	*pathbuf = __getname();
	if (*pathbuf) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
		pathname = d_path(exe_path, *pathbuf, PATH_MAX);
#else
		pathname = d_absolute_path(exe_path, *pathbuf, PATH_MAX);
#endif
		if (IS_ERR(pathname)) {
			__putname(*pathbuf);
			*pathbuf = NULL;
			pathname = NULL;
		}
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
		fix_dpath(exe_path, *pathbuf, pathname);
#endif
	}
	if (!pathname || !*pathbuf) {
		pr_err("IOF drv: Can't obtain absolute path: %p %p\n",
				pathname, *pathbuf);
	}
	fput(exe_file);

	return pathname ?: (const char *)exe_path->dentry->d_name.name;
}

static const char *icd_d_path(const struct path *path, char **pathbuf, char *namebuf)
{
	char *pathname = NULL;

	*pathbuf = __getname();
	if (*pathbuf) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
		pathname = d_path(path, *pathbuf, PATH_MAX);
#else
		pathname = d_absolute_path(path, *pathbuf, PATH_MAX);
#endif
		if (IS_ERR(pathname)) {
			__putname(*pathbuf);
			*pathbuf = NULL;
			pathname = NULL;
		}
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
		fix_dpath(path, *pathbuf, pathname);
#endif
	}

	if (!pathname) {
		strlcpy(namebuf, path->dentry->d_name.name, NAME_MAX);
		pathname = namebuf;
	}

	return pathname;
}

static void icd_hook_integrity_reset(struct task_struct *task,
					struct file *file, enum task_integrity_reset_cause cause)
{
	const char *execpath = NULL;
	char *pathbuf = NULL;
	enum oemflag_id oemid;

	pr_debug("IOF drv: observer t=%pi\n", task);

	execpath = get_exec_path(task, &pathbuf);
	if (IS_ERR_OR_NULL(execpath)) {
		if (execpath) {
			pr_err("IOF drv: get_exec_path err: %ld\n",
						PTR_ERR(execpath));
		}
		goto out;
	}

	oemid = affected_oemflag_id(task, execpath);

	if (oemid != OEMFLAG_NONE) {
		int ret;
		const char *pathname = NULL;
		char *pathbuf = NULL;
		char filename[NAME_MAX];

		pr_info("IOF drv: %s: %u\n", execpath, oemid);
		if (file) {
			pathname = icd_d_path(&file->f_path, &pathbuf, filename);
			pr_info("IOF drv: file=%s cause=%d\n", pathname, (int) cause);
			if (pathbuf)
				__putname(pathbuf);
		}

		ret = oem_flags_set(oemid);
		if (ret)
			pr_err("oem_flags_set err: %d\n", ret);
	}

out:
	if (pathbuf)
		__putname(pathbuf);
}

static int __init icd_driver_init(void)
{
	int ret = 0;

	five_add_hooks(five_ops, ARRAY_SIZE(five_ops));

	return ret;
}

static void __exit icd_driver_exit(void)
{
	pr_info("Exit ICDriver\n");
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung ICD Driver");
MODULE_VERSION("1.00");

module_init(icd_driver_init);
module_exit(icd_driver_exit);
