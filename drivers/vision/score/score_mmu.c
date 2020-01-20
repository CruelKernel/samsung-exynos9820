/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <linux/freezer.h>
#include <linux/fdtable.h>
#include <linux/version.h>
#if (KERNEL_VERSION(4, 11, 0) <= LINUX_VERSION_CODE)
#include <uapi/linux/sched/types.h>
#endif

#include "score_log.h"
#include "score_system.h"
#include "score_mmu.h"

enum score_mmu_buffer_type {
	BUF_TYPE_WRONG = 0,
	BUF_TYPE_USERPTR,
	BUF_TYPE_DMABUF
};

enum score_mmu_buffer_state {
	BUF_STATE_NEW = 1,
	BUF_STATE_REGISTERED,
	BUF_STATE_REPLACE,
	BUF_STATE_END
};

static void __score_mmu_context_del_list_dmabuf(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	struct score_mmu *mmu;

	mmu = ctx->mmu;
	spin_lock(&ctx->dbuf_slock);
	if (list_empty(&kbuf->list)) {
		spin_unlock(&ctx->dbuf_slock);
		score_err("buffer is not included in map list (%d/%#lx)\n",
				kbuf->m.fd, kbuf->m.userptr);
		return;
	}

	list_del_init(&kbuf->list);
	ctx->dbuf_count--;
	spin_unlock(&ctx->dbuf_slock);

	mmu->mmu_ops->unmap_dmabuf(mmu, kbuf);
}

static void __score_mmu_context_unmap_dmabuf(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	score_enter();
	if (!kbuf->mirror)
		__score_mmu_context_del_list_dmabuf(ctx, kbuf);

	dma_buf_put(kbuf->dbuf);
	score_leave();
}

static void __score_mmu_context_unmap_buffer(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	score_enter();
	switch (kbuf->type) {
	case BUF_TYPE_USERPTR:
	case BUF_TYPE_DMABUF:
		__score_mmu_context_unmap_dmabuf(ctx, kbuf);
		break;
	default:
		score_err("memory type is invalid (%d)\n", kbuf->type);
		break;
	}
	score_leave();
}

#if defined(CONFIG_EXYNOS_SCORE)
static void __score_mmu_context_freelist_add_dmabuf(
		struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	struct score_mmu *mmu;

	score_enter();
	mmu = ctx->mmu;
	spin_lock(&ctx->dbuf_slock);
	if (list_empty(&kbuf->list)) {
		spin_unlock(&ctx->dbuf_slock);
		score_warn("buffer is not included in map list (%d/%#lx)\n",
				kbuf->m.fd, kbuf->m.userptr);
		return;
	}

	list_del_init(&kbuf->list);
	ctx->dbuf_count--;
	score_mmu_freelist_add(mmu, kbuf);
	spin_unlock(&ctx->dbuf_slock);

	score_leave();
}

static void __score_mmu_context_freelist_add(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	score_enter();
	switch (kbuf->type) {
	case BUF_TYPE_USERPTR:
	case BUF_TYPE_DMABUF:
		__score_mmu_context_freelist_add_dmabuf(ctx, kbuf);
		break;
	default:
		score_err("memory type is invalid (%d)\n", kbuf->type);
		break;
	}
	score_leave();
}
#endif

void score_mmu_unmap_buffer(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	score_enter();
#if defined(CONFIG_EXYNOS_SCORE_FPGA)
	__score_mmu_context_unmap_buffer(ctx, kbuf);
#else
	if (kbuf->mirror)
		__score_mmu_context_unmap_buffer(ctx, kbuf);
	else
		__score_mmu_context_freelist_add(ctx, kbuf);
#endif
	score_leave();
}

static int __score_mmu_copy_dmabuf(struct score_mmu_buffer *cur_buf,
		struct score_mmu_buffer *list_buf)
{
	int retry = 100;

	score_check();
	while (retry) {
		if (!list_buf->dvaddr) {
			retry--;
			udelay(10);
			continue;
		}
		cur_buf->dvaddr = list_buf->dvaddr;
		break;
	}

	if (cur_buf->dvaddr)
		return 0;
	else
		return -EFAULT;
}

static bool __score_mmu_compare_dmabuf(struct score_mmu_buffer *cur_buf,
		struct score_mmu_buffer *list_buf)
{
	struct dma_buf *dbuf, *list_dbuf;

	score_check();
	dbuf = cur_buf->dbuf;
	list_dbuf = list_buf->dbuf;

	if ((dbuf == list_dbuf) &&
			(dbuf->size == list_dbuf->size) &&
			(dbuf->priv == list_dbuf->priv) &&
			(dbuf->file == list_dbuf->file))
		return true;
	else
		return false;
}

static int __score_mmu_context_add_list_dmabuf(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	int ret = BUF_STATE_END;
	struct score_mmu *mmu;
	struct score_mmu_buffer *list_buf, *tbuf;

	score_enter();
	mmu = ctx->mmu;
	spin_lock(&ctx->dbuf_slock);
	if (!ctx->dbuf_count) {
		list_add_tail(&kbuf->list, &ctx->dbuf_list);
		ctx->dbuf_count++;
		ret = BUF_STATE_NEW;
		goto p_end;
	}

	list_for_each_entry_safe(list_buf, tbuf, &ctx->dbuf_list, list) {
		if (kbuf->dbuf > list_buf->dbuf) {
			list_add_tail(&kbuf->list, &list_buf->list);
			ctx->dbuf_count++;
			ret = BUF_STATE_NEW;
			goto p_end;
		} else if (kbuf->dbuf == list_buf->dbuf) {
			if (__score_mmu_compare_dmabuf(kbuf, list_buf)) {
				kbuf->mirror = true;
				ret = BUF_STATE_REGISTERED;
			} else {
				list_replace_init(&list_buf->list,
						&kbuf->list);
				score_mmu_freelist_add(mmu, list_buf);
				ret = BUF_STATE_REPLACE;
			}
			goto p_end;
		} else {
			if (file_count(list_buf->dbuf->file) == 1) {
				list_del_init(&list_buf->list);
				ctx->dbuf_count--;
				score_mmu_freelist_add(mmu, list_buf);
			}
		}
	}

	list_add_tail(&kbuf->list, &ctx->dbuf_list);
	ctx->dbuf_count++;
	ret = BUF_STATE_NEW;
p_end:
	spin_unlock(&ctx->dbuf_slock);

	if (ret == BUF_STATE_NEW) {
		ret = mmu->mmu_ops->map_dmabuf(mmu, kbuf);
	} else if (ret == BUF_STATE_REGISTERED) {
		ret = __score_mmu_copy_dmabuf(kbuf, list_buf);
	} else if (ret == BUF_STATE_REPLACE) {
		ret = mmu->mmu_ops->map_dmabuf(mmu, kbuf);
	} else {
		score_err("state(%d) is invalid for mapping buffer\n", ret);
		ret = -EINVAL;
		goto p_err;
	}

	if (!kbuf->mirror && ret) {
		spin_lock(&ctx->dbuf_slock);
		list_del_init(&kbuf->list);
		ctx->dbuf_count--;
		spin_unlock(&ctx->dbuf_slock);
	}

	score_leave();
p_err:
	return ret;
}

static int __score_mmu_context_map_dmabuf(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	int ret = 0;
	struct score_mmu *mmu;
	struct dma_buf *dbuf;

	score_enter();
	if (kbuf->m.fd <= 0) {
		score_err("fd(%d) is invalid\n", kbuf->m.fd);
		ret = -EINVAL;
		goto p_err;
	}

	dbuf = dma_buf_get(kbuf->m.fd);
	if (IS_ERR(dbuf)) {
		score_err("dma_buf is invalid (fd:%d)\n", kbuf->m.fd);
		ret = -EINVAL;
		goto p_err;
	}
	kbuf->dbuf = dbuf;

	if ((dbuf->size - kbuf->offset) < kbuf->size) {
		score_err("user buffer size is small((%zd - %u) < %zd)\n",
				dbuf->size, kbuf->offset, kbuf->size);
		ret = -EINVAL;
		goto p_err_size;
	} else if (kbuf->size != dbuf->size) {
		kbuf->size = dbuf->size;
	}

	mmu = ctx->mmu;
	ret = __score_mmu_context_add_list_dmabuf(ctx, kbuf);
	if (ret)
		goto p_err_list;

	score_leave();
	return ret;
p_err_list:
p_err_size:
	dma_buf_put(dbuf);
p_err:
	return ret;
}

static struct dma_buf *__score_mmu_get_dma_buf_from_file(struct file *filp)
{
	int ret;
	int fd;
	struct dma_buf *dbuf;

	score_enter();
	get_file(filp);
	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		ret = fd;
		score_err("Failed to get unused fd (%d)\n", ret);
		goto p_err_fd;
	}

	fd_install(fd, filp);

	dbuf = dma_buf_get(fd);
	if (IS_ERR(dbuf)) {
		ret = IS_ERR(dbuf);
		score_err("Failed to get dma_buf from file (%d)\n", ret);
		goto p_err_dbuf;
	}

	__close_fd(current->files, fd);

	score_leave();
	return dbuf;
p_err_dbuf:
	__close_fd(current->files, fd);
p_err_fd:
	fput(filp);
	return ERR_PTR(ret);
}

static int __score_mmu_context_map_userptr(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	int ret = 0;
	unsigned long addr;
	struct vm_area_struct *vma;
	unsigned long vm_start;
	struct dma_buf *dbuf;
	struct score_mmu *mmu;

	score_enter();
	addr = kbuf->m.userptr;

	down_read(&current->mm->mmap_sem);
	vma = find_vma(current->mm, addr);
	if (!vma || (addr < vma->vm_start) || (addr > vma->vm_end)) {
		up_read(&current->mm->mmap_sem);
		score_err("User buffer(%#lx) is invalid\n", addr);
		ret = -EINVAL;
		goto p_err;
	}
	vm_start = vma->vm_start;

	if (!vma->vm_file) {
		up_read(&current->mm->mmap_sem);
		score_err("User buffer(%#lx) does not map a file\n", addr);
		ret = -EINVAL;
		goto p_err;
	}

	dbuf = __score_mmu_get_dma_buf_from_file(vma->vm_file);
	up_read(&current->mm->mmap_sem);

	if (IS_ERR(dbuf)) {
		ret = -EINVAL;
		goto p_err;
	}
	kbuf->dbuf = dbuf;

	kbuf->offset += (unsigned int)(addr - vm_start);
	if ((dbuf->size - kbuf->offset) < kbuf->size) {
		score_err("user buffer size is small((%zd - %u) < %zd)\n",
				dbuf->size, kbuf->offset, kbuf->size);
		ret = -EINVAL;
		goto p_err_size;
	} else if (kbuf->size != dbuf->size) {
		kbuf->size = dbuf->size;
	}

	mmu = ctx->mmu;
	ret = __score_mmu_context_add_list_dmabuf(ctx, kbuf);
	if (ret)
		goto p_err_list;

	score_leave();
	return ret;
p_err_list:
p_err_size:
	dma_buf_put(dbuf);
p_err:
	return ret;
}

static int __score_mmu_context_map_buffer(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	int ret;

	score_enter();
	switch (kbuf->type) {
	case BUF_TYPE_USERPTR:
		ret = __score_mmu_context_map_userptr(ctx, kbuf);
		goto p_err;
	case BUF_TYPE_DMABUF:
		ret = __score_mmu_context_map_dmabuf(ctx, kbuf);
		break;
	default:
		ret = -EINVAL;
		score_err("memory type is invalid (%d)\n", kbuf->type);
		goto p_err;
	}

	score_leave();
p_err:
	return ret;
}

int score_mmu_map_buffer(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	int ret;

	score_enter();

	ret = __score_mmu_context_map_buffer(ctx, kbuf);
	if (ret)
		goto p_err;

	score_leave();
	return 0;
p_err:
	return ret;
}

static void score_mmu_check_useless(struct kthread_work *work)
{
	struct score_mmu_context *ctx;
	struct score_mmu_buffer *buf, *tbuf;

	ctx = container_of(work, struct score_mmu_context, work);

	if (!ctx->work_run)
		return;

	if (!ctx->dbuf_count)
		return;

	spin_lock(&ctx->dbuf_slock);
	list_for_each_entry_safe(buf, tbuf, &ctx->dbuf_list, list) {
		if (file_count(buf->dbuf->file) == 1) {
			list_del_init(&buf->list);
			ctx->dbuf_count--;
			score_mmu_freelist_add(ctx->mmu, buf);
		}
	}
	spin_unlock(&ctx->dbuf_slock);
}

void *score_mmu_create_context(struct score_mmu *mmu)
{
	int ret;
	struct score_mmu_context *ctx;

	score_enter();
	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		ret = -ENOMEM;
		score_err("Fail to alloc mmu context\n");
		goto p_err;
	}
	ctx->mmu = mmu;

	INIT_LIST_HEAD(&ctx->dbuf_list);
	ctx->dbuf_count = 0;
	spin_lock_init(&ctx->dbuf_slock);

	kthread_init_work(&ctx->work, score_mmu_check_useless);
	ctx->work_run = true;

	score_leave();
	return ctx;
p_err:
	return ERR_PTR(ret);
}

void score_mmu_destroy_context(void *alloc_ctx)
{
	struct score_mmu_context *ctx;

	score_enter();
	ctx = alloc_ctx;

	ctx->work_run = false;
	kthread_flush_work(&ctx->work);

	if (ctx->dbuf_count) {
		struct score_mmu_buffer *buf, *tbuf;

		spin_lock(&ctx->dbuf_slock);
		list_for_each_entry_safe(buf, tbuf, &ctx->dbuf_list, list) {
			list_del_init(&buf->list);
			ctx->dbuf_count--;
			score_mmu_freelist_add(ctx->mmu, buf);
		}
		spin_unlock(&ctx->dbuf_slock);
	}

	kfree(ctx);
	score_leave();
}

int score_mmu_packet_prepare(struct score_mmu *mmu,
		struct score_mmu_packet *packet)
{
	return score_memory_kmap_packet(&mmu->mem, packet);
}

void score_mmu_packet_unprepare(struct score_mmu *mmu,
		struct score_mmu_packet *packet)
{
	return score_memory_kunmap_packet(&mmu->mem, packet);
}

void score_mmu_freelist_add(struct score_mmu *mmu,
		struct score_mmu_buffer *kbuf)
{
	score_enter();
	spin_lock(&mmu->free_slock);
	list_add_tail(&kbuf->list, &mmu->free_list);
	mmu->free_count++;
	spin_unlock(&mmu->free_slock);
	wake_up(&mmu->waitq);
	score_leave();
}

/**
 * score_mmu_get_internal_mem_kvaddr - Get kvaddr of
 *	kernel space memory allocated for SCore device
 * @mem:	[in]	object about score_memory structure
 * @kvaddr:	[out]	kernel virtual address
 */
void *score_mmu_get_internal_mem_kvaddr(struct score_mmu *mmu)
{
	score_check();
	return score_memory_get_internal_mem_kvaddr(&mmu->mem);
}

/**
 * score_mmu_get_internal_mem_dvaddr - Get  dvaddr of
 *	kernel space memory allocated for SCore device
 * @mem:	[in]	object about score_memory structure
 * @dvaddr:	[out]	device virtual address
 */
dma_addr_t score_mmu_get_internal_mem_dvaddr(struct score_mmu *mmu)
{
	score_check();
	return score_memory_get_internal_mem_dvaddr(&mmu->mem);
}

/**
 * score_mmu_get_profiler_kvaddr - Get kvaddr of
 *	kernel space memory allocated for SCore device
 * @mem:	[in]	object about score_memory structure
 * @id:         [in]    id of requested core memory
 * @kvaddr:	[out]	kernel virtual address
 */
void *score_mmu_get_profiler_kvaddr(struct score_mmu *mmu, unsigned int id)
{
	score_check();
	return score_memory_get_profiler_kvaddr(&mmu->mem, id);
}

/**
 * score_mmu_get_print_kvaddr - Get kvaddr and dvaddr of
 *	kernel space memory allocated for SCore device
 * @mem:	[in]	object about score_memory structure
 * @kvaddr:	[out]	kernel virtual address
 */
void *score_mmu_get_print_kvaddr(struct score_mmu *mmu)
{
	score_check();
	return score_memory_get_print_kvaddr(&mmu->mem);
}

void score_mmu_init(struct score_mmu *mmu)
{
	score_memory_init(&mmu->mem);
}

int score_mmu_open(struct score_mmu *mmu)
{
	int ret = 0;
	void *kvaddr;
	dma_addr_t dvaddr;

	score_enter();
	ret = score_memory_open(&mmu->mem);

	if (!ret) {
		kvaddr = score_mmu_get_internal_mem_kvaddr(mmu);
		dvaddr = score_mmu_get_internal_mem_dvaddr(mmu);
		score_info("Internal memory (%p, %#8x)\n",
				kvaddr, (unsigned int)dvaddr);
	}
	score_leave();
	return ret;
}

void score_mmu_close(struct score_mmu *mmu)
{
	score_enter();
	score_memory_close(&mmu->mem);
	score_leave();
}

static int score_mmu_free(void *data)
{
	struct score_mmu *mmu;
	struct score_mmu_buffer *buf, *tbuf;
	struct list_head to_free_list;

	score_enter();

	INIT_LIST_HEAD(&to_free_list);
	mmu = data;
	set_freezable();

	while (true) {
		wait_event_freezable(mmu->waitq,
				mmu->free_count > 0 || kthread_should_stop());

		spin_lock(&mmu->free_slock);
		if (list_empty(&mmu->free_list)) {
			spin_unlock(&mmu->free_slock);
			if (kthread_should_stop())
				break;
			continue;
		}

		// move bufs into 'to_free_list' and unlock mmu->free_list
		// as fast as possible
		list_for_each_entry_safe(buf, tbuf, &mmu->free_list, list) {
			list_move_tail(&buf->list, &to_free_list);
			mmu->free_count--;
		}

		spin_unlock(&mmu->free_slock);

		list_for_each_entry_safe(buf, tbuf, &to_free_list, list) {
			mmu->mmu_ops->unmap_dmabuf(mmu, buf);
			dma_buf_put(buf->dbuf);
			list_del(&buf->list);
			kfree(buf);
		}
	}

	score_leave();
	return 0;
}

static int __score_mmu_freelist_init(struct score_mmu *mmu)
{
	int ret = 0;
	struct sched_param param = { .sched_priority = 0 };

	INIT_LIST_HEAD(&mmu->free_list);
	spin_lock_init(&mmu->free_slock);
	mmu->free_count = 0;

	init_waitqueue_head(&mmu->waitq);
	mmu->free_task = kthread_run(score_mmu_free, mmu, "score_mmu_free");
	if (IS_ERR(mmu->free_task)) {
		ret = PTR_ERR(mmu->free_task);
		score_err("creating thread for mmu free failed(%d)\n", ret);
		goto p_end;
	}
	sched_setscheduler(mmu->free_task, SCHED_IDLE, &param);
p_end:
	return ret;
}

static void __score_mmu_freelist_deinit(struct score_mmu *mmu)
{
	wake_up(&mmu->waitq);
	kthread_stop(mmu->free_task);
}

static int __score_mmu_unmap_worker_init(struct score_mmu *mmu)
{
	int ret = 0;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

	kthread_init_worker(&mmu->unmap_worker);

	mmu->unmap_task = kthread_run(kthread_worker_fn,
			&mmu->unmap_worker, "score_unmap_worker");
	if (IS_ERR(mmu->unmap_task)) {
		ret = PTR_ERR(mmu->unmap_task);
		score_err("kthread for unmap_worker is not running (%d)\n",
				ret);
		goto p_err_kthread;
	}

	ret = sched_setscheduler_nocheck(mmu->unmap_task, SCHED_FIFO, &param);
	if (ret) {
		score_err("scheduler setting of unmap_worker failed(%d)\n",
				ret);
		goto p_err_sched;
	}

	return ret;
p_err_sched:
	kthread_stop(mmu->unmap_task);
p_err_kthread:
	return ret;
}

static void __score_mmu_unmap_worker_deinit(struct score_mmu *mmu)
{
	kthread_stop(mmu->unmap_task);
}

int score_mmu_probe(struct score_system *system)
{
	int ret = 0;
	struct score_mmu *mmu;

	score_enter();
	mmu = &system->mmu;
	mmu->system = system;

	ret = score_memory_probe(mmu);
	if (ret)
		goto p_end;

	ret = __score_mmu_freelist_init(mmu);
	if (ret)
		goto p_err_freelist;

	ret = __score_mmu_unmap_worker_init(mmu);
	if (ret)
		goto p_err_unmap;

	score_leave();
	return ret;
p_err_unmap:
	__score_mmu_freelist_deinit(mmu);
p_err_freelist:
	score_memory_remove(&mmu->mem);
p_end:
	return ret;
}

void score_mmu_remove(struct score_mmu *mmu)
{
	score_enter();
	__score_mmu_unmap_worker_deinit(mmu);
	__score_mmu_freelist_deinit(mmu);
	score_memory_remove(&mmu->mem);
	score_leave();
}
