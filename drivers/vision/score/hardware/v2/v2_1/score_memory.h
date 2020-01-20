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

#ifndef __SCORE_EXYNOS_MEMORY_H__
#define __SCORE_EXYNOS_MEMORY_H__

#include <linux/device.h>
#include "score_ion.h"

struct score_system;
struct score_mmu;
struct score_mmu_packet;

/**
 * struct score_memory - Object controlling memory for SCore device
 * @dev: device struct registered in platform
 * @ctx: data for ion_context
 * @kavddr: kernel virtual address of internal memory
 * @dvaddr: device virtual address of internal memory
 * @malloc_kvaddr: kernel virtual address of memory for malloc
 * @malloc_dvaddr: device virtual address of memory for malloc
 * @print_kvaddr: kernel virtual address of memory for print
 * @print_dvaddr: device virtual address of memory for print
 * @profiler_kvaddr: kernel virtual address of memory for profiler
 * @profiler_dvaddr: device virtual address of memory for profiler
 */
struct score_memory {
	struct device			*dev;
	struct score_system		*system;
	void				*ctx;

	void				*kvaddr;
	dma_addr_t			dvaddr;
	void				*malloc_kvaddr;
	dma_addr_t			malloc_dvaddr;
	void				*print_kvaddr;
	dma_addr_t			print_dvaddr;
	void				*profiler_mc_kvaddr;
	dma_addr_t			profiler_mc_dvaddr;
	void				*profiler_kc1_kvaddr;
	dma_addr_t			profiler_kc1_dvaddr;
	void				*profiler_kc2_kvaddr;
	dma_addr_t			profiler_kc2_dvaddr;
};

int score_memory_kmap_packet(struct score_memory *mem,
		struct score_mmu_packet *packet);
void score_memory_kunmap_packet(struct score_memory *mem,
		struct score_mmu_packet *packet);
void *score_memory_get_internal_mem_kvaddr(struct score_memory *mem);
dma_addr_t score_memory_get_internal_mem_dvaddr(struct score_memory *mem);
void *score_memory_get_profiler_kvaddr(struct score_memory *mem,
							unsigned int id);
void *score_memory_get_print_kvaddr(struct score_memory *mem);

void score_memory_init(struct score_memory *mem);
int score_memory_open(struct score_memory *mem);
void score_memory_close(struct score_memory *mem);

int score_memory_probe(struct score_mmu *mmu);
void score_memory_remove(struct score_memory *mem);

#endif
