/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */


#include <linux/dcache.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/namei.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#include "include/defex_caches.h"
#include "include/defex_catch_list.h"
#include "include/defex_config.h"
#include "include/defex_internal.h"
#include "include/defex_rules.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/mm.h>
#include <linux/sched/task.h>
#endif


const char unknown_file[] = "<unknown filename>";

void init_defex_context(struct defex_context *dc, int syscall, struct task_struct *p, struct file *f)
{
	dc->syscall_no = syscall;
	dc->task = p;
	dc->process_file = NULL;
	dc->process_dpath = NULL;
	dc->process_name = NULL;
	dc->target_file = f;
	dc->target_dpath = NULL;
	dc->target_name = NULL;
	dc->process_name_buff = NULL;
	dc->target_name_buff = NULL;
}

void release_defex_context(struct defex_context *dc)
{
	kfree(dc->process_name_buff);
	kfree(dc->target_name_buff);
#ifndef DEFEX_CACHES_ENABLE
	if (dc->process_file)
		fput(dc->process_file);
#endif /* DEFEX_CACHES_ENABLE */
}

struct file *get_dc_process_file(struct defex_context *dc)
{
	if (!dc->process_file)
		dc->process_file = defex_get_source_file(dc->task);
	return dc->process_file;
}

const struct path *get_dc_process_dpath(struct defex_context *dc)
{
	const struct path *dpath;
	struct file *exe_file = NULL;

	if (dc->process_dpath)
		return dc->process_dpath;

	exe_file = get_dc_process_file(dc);
	if (!IS_ERR_OR_NULL(exe_file)) {
		dpath = &exe_file->f_path;
		if (dpath->dentry && dpath->dentry->d_inode) {
			dc->process_dpath = dpath;
			return dpath;
		}
	}
	return NULL;
}

char *get_dc_process_name(struct defex_context *dc)
{
	const struct path *dpath;
	char *path = NULL;

	if (!dc->process_name) {
		dpath = get_dc_process_dpath(dc);
		if (dpath) {
			dc->process_name_buff = kzalloc(PATH_MAX, GFP_ATOMIC);
			if (dc->process_name_buff)
				path = d_path(dpath, dc->process_name_buff, PATH_MAX);
		}
		dc->process_name = (IS_ERR_OR_NULL(path)) ? (char *)unknown_file : path;
	}
	return dc->process_name;
}

const struct path *get_dc_target_dpath(struct defex_context *dc)
{
	const struct path *dpath;

	if (dc->target_dpath)
		return dc->target_dpath;

	if (!IS_ERR_OR_NULL(dc->target_file)) {
		dpath = &(dc->target_file->f_path);
		if (dpath->dentry && dpath->dentry->d_inode) {
			dc->target_dpath = dpath;
			return dpath;
		}
	}
	return NULL;
}

char *get_dc_target_name(struct defex_context *dc)
{
	const struct path *dpath;
	char *path = NULL;

	if (!dc->target_name) {
		dpath = get_dc_target_dpath(dc);
		if (dpath) {
			dc->target_name_buff = kzalloc(PATH_MAX, GFP_ATOMIC);
			if (dc->target_name_buff)
				path = d_path(dpath, dc->target_name_buff, PATH_MAX);
		}
		dc->target_name = (IS_ERR_OR_NULL(path)) ? (char *)unknown_file : path;
	}
	return dc->target_name;
}

struct file *defex_get_source_file(struct task_struct *p)
{
	struct file *file_addr = NULL;
	struct mm_struct *proc_mm;

#ifdef DEFEX_CACHES_ENABLE
	bool self;
	file_addr = defex_file_cache_find(p->pid);

	if (!file_addr) {
		proc_mm = get_task_mm(p);
		if (!proc_mm)
			return NULL;
		file_addr = get_mm_exe_file(proc_mm);
		mmput(proc_mm);
		if (!file_addr)
			return NULL;
		defex_file_cache_add(p->pid, file_addr);
	} else {
		self = (p == current);
		proc_mm = (self)?p->mm:get_task_mm(p);
		if (!proc_mm)
			return NULL;
		if (self)
			down_read(&proc_mm->mmap_sem);
		if (file_addr != proc_mm->exe_file) {
			file_addr = proc_mm->exe_file;
			if (!file_addr)
				goto clean_mm;
			get_file(file_addr);
			defex_file_cache_update(file_addr);
		}
clean_mm:
		if (self)
			up_read(&proc_mm->mmap_sem);
		else
			mmput(proc_mm);
	}
#else

	proc_mm = get_task_mm(p);
	if (!proc_mm)
		return NULL;
	file_addr = get_mm_exe_file(proc_mm);
	mmput(proc_mm);
#endif /* DEFEX_CACHES_ENABLE */
	return file_addr;
}

char *defex_get_filename(struct task_struct *p)
{
	struct file *exe_file = NULL;
	const struct path *dpath = NULL;
	char *path = NULL, *buff = NULL;
	char *filename = NULL;

	exe_file = defex_get_source_file(p);
	if (!exe_file)
		goto out_filename;

	dpath = &exe_file->f_path;

	buff = kzalloc(PATH_MAX, GFP_ATOMIC);
	if (buff)
		path = d_path(dpath, buff, PATH_MAX);

#ifndef DEFEX_CACHES_ENABLE
	fput(exe_file);
#endif /* DEFEX_CACHES_ENABLE */

out_filename:
	if (path && !IS_ERR(path))
		filename = kstrdup(path, GFP_ATOMIC);

	if (!filename)
		filename = (char *)unknown_file;

	if (buff)
		kfree(buff);
	return filename;
}

/* Resolve the filename to absolute path, follow the links
   name     - input file name
   out_buff - output pointer to the allocated buffer (should be freed)
   Returns:   pointer to resolved filename or NULL
 */
char* defex_resolve_filename(const char *name, char **out_buff)
{
	char *target_file = NULL, *buff = NULL;
	struct path path;

	if (*out_buff)
		buff = *out_buff;
	else
		buff = kzalloc(PATH_MAX, GFP_ATOMIC);
	if (buff) {
		if (!kern_path(name, LOOKUP_FOLLOW, &path)) {
			target_file = d_path(&path, buff, PATH_MAX);
			path_put(&path);
		}
		if (IS_ERR_OR_NULL(target_file)) {
			kfree(buff);
			buff = NULL;
			target_file = NULL;
		}
	}
	*out_buff = buff;
	return target_file;
}

