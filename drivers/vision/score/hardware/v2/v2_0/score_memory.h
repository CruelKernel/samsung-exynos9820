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

#ifndef __SCORE_ZYNQ_MEMORY_H__
#define __SCORE_ZYNQ_MEMORY_H__

struct score_system;
struct score_mmu;
struct score_mmu_packet;
/*
 * Kernel space memory to be used by SCore device
 * This is only used for FPGA (zynq + s2c)
 */
#define SCORE_INTERNAL_MEMORY_ADDR              (0x80000000)

/**
 * struct score_memory - Object controlling memory for SCore device
 * @dev: device struct registered in platform
 * @image_paddr: s2c physical address of memory for image buffer
 * @image_kvaddr: kernel virtual address of memory for image buffer
 * @image_mem_size: size of memory for image buffer
 * @used_image_buf_size: size of already used memory among memory for image
 *	buffer
 * @lock: lock for image buffer
 * @size: size of total memory for image buffer
 * @kavddr: kernel virtual address of internal memory
 * @dvaddr: device virtual address of internal memory
 * @malloc_kvaddr: kernel virtual address of memory for malloc
 * @malloc_dvaddr: device virtual address of memory for malloc
 * @print_kvaddr: kernel virtual address of memory for print
 * @print_dvaddr: device virtual address of memory for print
 * @profiler_mc_kvaddr: kernel virtual address of MC memory for profiler
 * @profiler_mc_dvaddr: device virtual address of MC memory for profiler
 * @profiler_kc1_kvaddr: kernel virtual address of KC1 memory for profiler
 * @profiler_kc1_dvaddr: device virtual address of KC1 memory for profiler
 * @profiler_kc2_kvaddr: kernel virtual address of KC2 memory for profiler
 * @profiler_kc2_dvaddr: device virtual address of KC2 memory for profiler
 */
struct score_memory {
	struct device			*dev;
	struct score_system		*system;

	unsigned int			image_paddr;
	void __iomem			*image_kvaddr;
	resource_size_t			image_mem_size;
	size_t				used_image_buf_size;
	struct mutex			lock;

	resource_size_t			size;
	void __iomem			*kvaddr;
	dma_addr_t			dvaddr;
	void __iomem			*malloc_kvaddr;
	dma_addr_t			malloc_dvaddr;
	void __iomem			*print_kvaddr;
	dma_addr_t			print_dvaddr;
	void __iomem			*profiler_mc_kvaddr;
	dma_addr_t			profiler_mc_dvaddr;
	void __iomem			*profiler_kc1_kvaddr;
	dma_addr_t			profiler_kc1_dvaddr;
	void __iomem			*profiler_kc2_kvaddr;
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
