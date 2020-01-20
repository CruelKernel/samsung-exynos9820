/*
 * Samsung Exynos SoC series SCore driver
 *
 * Memory manager for memory area that SCore device uses in kernel space
 * and sc_buffer memory for target
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_MMU_H__
#define __SCORE_MMU_H__

#include <linux/device.h>
#include "score_memory.h"

#define UNMAP_TRY	(30)

struct score_system;
struct score_mmu;
struct score_mmu_buffer;

/**
 * struct score_mem_ops - Operations controlling memory for SCore device
 * @map_dmabuf: map data of sc_buffer by iommu for SCore device to access
 * @unmap_dmabuf: unmap data of sc_buffer by iommu
 * @map_userptr: map data of sc_buffer by iommu
 * @unmap_userptr: unmap data of sc_buffer by iommu
 */
struct score_mmu_ops {
	int (*map_dmabuf)(struct score_mmu *mmu,
			struct score_mmu_buffer *buf);
	int (*unmap_dmabuf)(struct score_mmu *mmu,
			struct score_mmu_buffer *buf);
	int (*map_userptr)(struct score_mmu *mmu,
			struct score_mmu_buffer *buf);
	int (*unmap_userptr)(struct score_mmu *mmu,
			struct score_mmu_buffer *buf);
};

/**
 * struct score_memory - Object controlling memory for SCore device
 * @dev: device struct registered in platform
 * @mem_ops: operations controlling only vb2_ion for SCore
 * @kavddr: kernel virtual address of internal memory
 * @dvaddr: device virtual address of internal memory
 * @malloc_kvaddr: kernel virtual address of memory for malloc at SCore device
 * @malloc_dvaddr: device virtual address of memory for malloc at SCore device
 * @print_kvaddr: kernel virtual address of memory for print of SCore device
 * @print_dvaddr: device virtual address of memory for print of SCore device
 */
struct score_mmu {
	struct device			*dev;
	const struct score_mmu_ops	*mmu_ops;
	struct score_memory		mem;
	struct score_system		*system;

	struct list_head                free_list;
	spinlock_t                      free_slock;
	int				free_count;
	wait_queue_head_t		waitq;
	struct task_struct		*free_task;
	struct kthread_worker		unmap_worker;
	struct task_struct		*unmap_task;
};

/**
 * struct score_mmu_context - Object controlling memory for SCore device
 */
struct score_mmu_context {
	struct score_mmu		*mmu;
	struct list_head                dbuf_list;
	int                             dbuf_count;
	spinlock_t			dbuf_slock;

	struct list_head                userptr_list;
	int                             userptr_count;
	spinlock_t			userptr_slock;

	struct kthread_work		work;
	bool				work_run;
};

/**
 * struct score_mmu_buffer - Buffer of kernel side matched with sc_buffer
 * @list: list to be included in list of frame
 * @fd: fd allocated by ion device
 * @size: size of buffer
 * @dbuf: object about dmabuf structure
 * @dvaddr: device virtual address of buffer
 * @kvaddr: kernel virtual address of buffer
 */
struct score_mmu_buffer {
	struct list_head                list;
	struct list_head                frame_list;

	int				type;
	size_t                          size;
	union {
		int                     fd;
		unsigned long           userptr;
		unsigned long long	mem_info;
	} m;
	struct dma_buf                  *dbuf;
	struct dma_buf_attachment       *attachment;
	struct sg_table                 *sgt;
	dma_addr_t                      dvaddr;
	void                            *kvaddr;
	unsigned int			offset;
	bool				mirror;
};

/**
 * struct score_mmu_packet - Buffer of kernel side matched with sc_buffer
 * @fd: fd allocated by ion device
 * @size: size of buffer
 * @dbuf: object about dmabuf structure
 * @kvaddr: kernel virtual address of buffer
 */
struct score_mmu_packet {
	int				fd;
	size_t				size;
	void				*dbuf;
	void				*kvaddr;
};

int score_mmu_map_buffer(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf);
void score_mmu_unmap_buffer(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf);
void *score_mmu_create_context(struct score_mmu *mmu);
void score_mmu_destroy_context(void *alloc_ctx);

void score_mmu_freelist_add(struct score_mmu *mmu,
		struct score_mmu_buffer *kbuf);
int score_mmu_packet_prepare(struct score_mmu *mmu,
		struct score_mmu_packet *packet);
void score_mmu_packet_unprepare(struct score_mmu *mmu,
		struct score_mmu_packet *packet);
void *score_mmu_get_internal_mem_kvaddr(struct score_mmu *mmu);
dma_addr_t score_mmu_get_internal_mem_dvaddr(struct score_mmu *mmu);
void *score_mmu_get_profiler_kvaddr(struct score_mmu *mmu, unsigned int id);
void *score_mmu_get_print_kvaddr(struct score_mmu *mmu);

void score_mmu_init(struct score_mmu *mmu);
int score_mmu_open(struct score_mmu *mmu);
void score_mmu_close(struct score_mmu *mmu);

int score_mmu_probe(struct score_system *system);
void score_mmu_remove(struct score_mmu *mmu);

#endif
