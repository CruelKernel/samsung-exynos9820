/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/dma-buf.h>
#include <linux/iommu.h>
#include <linux/ion_exynos.h>

#include "score_log.h"
#include "score_regs.h"
#include "score_system.h"
#include "score_mem_table.h"
#include "score_mmu.h"
#include "score_memory.h"

int score_memory_kmap_packet(struct score_memory *mem,
		struct score_mmu_packet *packet)
{
	int ret = 0;
	int fd;
	struct dma_buf *dbuf;
	void *kvaddr;

	score_enter();
	fd = packet->fd;
	if (fd <= 0) {
		score_err("fd(%d) is invalid\n", fd);
		ret = -EINVAL;
		goto p_err;
	}

	dbuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dbuf)) {
		score_err("dma_buf is invalid (fd:%d)\n", fd);
		ret =  -EINVAL;
		goto p_err;
	}
	packet->dbuf = dbuf;
	packet->size = dbuf->size;

	kvaddr = dma_buf_vmap(dbuf);
	if (!kvaddr) {
		score_err("vmap failed (dbuf:%p)\n", dbuf);
		ret = -EFAULT;
		goto p_err_vmap;
	}

	packet->kvaddr = kvaddr;

	score_leave();
	return ret;
p_err_vmap:
	dma_buf_put(dbuf);
p_err:
	return ret;
}

void score_memory_kunmap_packet(struct score_memory *mem,
		struct score_mmu_packet *packet)
{
	struct dma_buf *dbuf;
	void *kvaddr;

	score_enter();
	dbuf = packet->dbuf;
	kvaddr = packet->kvaddr;

	dma_buf_vunmap(dbuf, kvaddr);
	dma_buf_put(dbuf);

	score_leave();
}

/**
 * score_memory_get_internal_mem_kvaddr - Get kvaddr of
 *      kernel space memory allocated for SCore device
 * @mem:        [in]    object about score_memory structure
 * @kvaddr:     [out]   kernel virtual address
 */
void *score_memory_get_internal_mem_kvaddr(struct score_memory *mem)
{
	score_enter();
	return mem->kvaddr;
}

/**
 * score_memory_get_internal_mem_dvaddr - Get dvaddr of
 *      kernel space memory allocated for SCore device
 * @mem:        [in]    object about score_memory structure
 * @dvaddr:     [out]   device virtual address
 */
dma_addr_t score_memory_get_internal_mem_dvaddr(struct score_memory *mem)
{
	score_check();
	return mem->dvaddr;
}

/**
 * score_memory_get_profiler_kvaddr - Get kvaddr of given core
 *      kernel space memory allocated for SCore device
 * @mem:        [in]    object about score_memory structure
 * @id:         [in]    id of requested core memory
 * @kvaddr:     [out]   kernel virtual address
 */
void *score_memory_get_profiler_kvaddr(struct score_memory *mem,
		unsigned int id)
{
	score_check();
	if (id == SCORE_TS)
		return mem->profiler_ts_kvaddr;
	else if (id == SCORE_BARON1)
		return mem->profiler_br1_kvaddr;
	else if (id == SCORE_BARON2)
		return mem->profiler_br2_kvaddr;
	return 0;
}

/**
 * score_memory_get_print_kvaddr - Get kvaddr of
 *      kernel space memory allocated for SCore device
 * @mem:        [in]    object about score_memory structure
 * @kvaddr:     [out]   kernel virtual address
 */
void *score_memory_get_print_kvaddr(struct score_memory *mem)
{
	score_check();
	return mem->print_kvaddr;
}

static int score_exynos_memory_map_dmabuf(struct score_mmu *mmu,
		struct score_mmu_buffer *buf)
{
	int ret = 0;
	struct score_memory *mem;
	struct dma_buf *dbuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dvaddr;

	score_enter();
	mem = &mmu->mem;
	dbuf = buf->dbuf;

	attachment = dma_buf_attach(dbuf, mem->dev);
	if (IS_ERR(attachment)) {
		ret = PTR_ERR(attachment);
		score_err("failed to attach dmabuf (%d)\n", ret);
		goto p_err_attach;
	}
	buf->attachment = attachment;

	sgt = dma_buf_map_attachment(attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		ret = PTR_ERR(sgt);
		score_err("failed to map attachment (%d)\n", ret);
		goto p_err_map_attach;
	}
	buf->sgt = sgt;

	dvaddr = ion_iovmm_map(attachment, 0, buf->size, DMA_BIDIRECTIONAL,
			IOMMU_CACHE);
	if (IS_ERR_VALUE(dvaddr)) {
		ret = (int)dvaddr;
		score_err("failed to map iova (%d)\n", ret);
		goto p_err_iovmm_map;
	}
	buf->dvaddr = dvaddr;

	score_leave();
	return ret;
p_err_iovmm_map:
	dma_buf_unmap_attachment(attachment, sgt, DMA_BIDIRECTIONAL);
p_err_map_attach:
	dma_buf_detach(dbuf, attachment);
p_err_attach:
	return ret;
}

static int score_exynos_memory_unmap_dmabuf(struct score_mmu *mmu,
		struct score_mmu_buffer *buf)
{
	score_enter();
	ion_iovmm_unmap(buf->attachment, buf->dvaddr);
	dma_buf_unmap_attachment(buf->attachment, buf->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(buf->dbuf, buf->attachment);
	buf->dvaddr = 0;

	score_leave();
	return 0;
}

const struct score_mmu_ops score_exynos_mmu_ops = {
	.map_dmabuf	= score_exynos_memory_map_dmabuf,
	.unmap_dmabuf	= score_exynos_memory_unmap_dmabuf,
};

static int __score_memory_alloc(struct score_memory *mem)
{
	int ret = 0;

	score_enter();
	mem->size = SCORE_MEMORY_INTERNAL_SIZE;

	mem->dma_buf = ion_alloc_dmabuf("ion_system_heap", mem->size, 0);
	if (IS_ERR(mem->dma_buf)) {
		ret = PTR_ERR(mem->dma_buf);
		score_err("It is failed to allocate dma_buf (%d)\n", ret);
		goto p_err_alloc;
	}

	mem->attachment = dma_buf_attach(mem->dma_buf, mem->dev);
	if (IS_ERR(mem->attachment)) {
		ret = PTR_ERR(mem->attachment);
		score_err("It is failed to attach dma_buf (%d)\n", ret);
		goto p_err_attach;
	}

	mem->sgt = dma_buf_map_attachment(mem->attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR(mem->sgt)) {
		ret = PTR_ERR(mem->sgt);
		score_err("It is failed to map attachment (%d)\n", ret);
		goto p_err_attachment;
	}

	mem->dvaddr = ion_iovmm_map(mem->attachment, 0, mem->size,
			DMA_BIDIRECTIONAL, 0);
	if (IS_ERR_VALUE(mem->dvaddr)) {
		ret = (int)mem->dvaddr;
		score_err("It is failed to map dvaddr (%d)\n", ret);
		goto p_err_map_dva;
	}

	mem->kvaddr = dma_buf_vmap(mem->dma_buf);
	if (IS_ERR(mem->kvaddr)) {
		ret = PTR_ERR(mem->kvaddr);
		score_err("It is failed to map kva (%d)\n", ret);
		goto p_err_kmap;
	}

	mem->malloc_kvaddr = mem->kvaddr + SCORE_FW_SIZE;
	mem->malloc_dvaddr = mem->dvaddr + SCORE_FW_SIZE;

	mem->print_kvaddr = mem->malloc_kvaddr + SCORE_MALLOC_SIZE;
	mem->print_dvaddr = mem->malloc_dvaddr + SCORE_MALLOC_SIZE;

	mem->profiler_ts_dvaddr = mem->print_dvaddr + SCORE_PRINT_SIZE;
	mem->profiler_ts_kvaddr = mem->print_kvaddr + SCORE_PRINT_SIZE;
	mem->profiler_br1_dvaddr =
			mem->profiler_ts_dvaddr + SCORE_TS_PROFILER_SIZE;
	mem->profiler_br1_kvaddr =
			mem->profiler_ts_kvaddr + SCORE_TS_PROFILER_SIZE;
	mem->profiler_br2_dvaddr =
			mem->profiler_br1_dvaddr + SCORE_BR_PROFILER_SIZE;
	mem->profiler_br2_kvaddr =
			mem->profiler_br1_kvaddr + SCORE_BR_PROFILER_SIZE;

	score_leave();
	return ret;
p_err_kmap:
	ion_iovmm_unmap(mem->attachment, mem->dvaddr);
p_err_map_dva:
	dma_buf_unmap_attachment(mem->attachment, mem->sgt, DMA_BIDIRECTIONAL);
p_err_attachment:
	dma_buf_detach(mem->dma_buf, mem->attachment);
p_err_attach:
	dma_buf_put(mem->dma_buf);
p_err_alloc:
	return ret;
}

static void __score_memory_free(struct score_memory *mem)
{
	score_enter();
	dma_buf_vunmap(mem->dma_buf, mem->kvaddr);
	ion_iovmm_unmap(mem->attachment, mem->dvaddr);
	dma_buf_unmap_attachment(mem->attachment, mem->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(mem->dma_buf, mem->attachment);
	dma_buf_put(mem->dma_buf);
	score_leave();
}

void score_memory_init(struct score_memory *mem)
{
	void __iomem *sfr;
	dma_addr_t br1_malloc, br2_malloc;

	score_enter();
	sfr = mem->system->sfr;
	br1_malloc = mem->malloc_dvaddr + SCORE_TS_MALLOC_SIZE;
	br2_malloc = br1_malloc + SCORE_BR_MALLOC_SIZE;

	writel(mem->malloc_dvaddr, sfr + TS_MALLOC_BASE_ADDR);
	writel(SCORE_TS_MALLOC_SIZE, sfr + TS_MALLOC_SIZE);
	writel(br1_malloc, sfr + BR1_MALLOC_BASE_ADDR);
	writel(br2_malloc, sfr + BR2_MALLOC_BASE_ADDR);
	writel(SCORE_BR_MALLOC_SIZE, sfr + BR_MALLOC_SIZE);

	writel(mem->print_dvaddr, sfr + PRINT_BASE_ADDR);
	writel(SCORE_PRINT_SIZE, sfr + PRINT_SIZE);

	writel(SCORE_TS_PROFILER_SIZE, sfr + PROFILER_TS_SIZE);
	writel(SCORE_BR_PROFILER_SIZE, sfr + PROFILER_BR_SIZE);
	writel(mem->profiler_ts_dvaddr, sfr + PROFILER_TS_BASE_ADDR);
	writel(mem->profiler_br1_dvaddr, sfr + PROFILER_BR1_BASE_ADDR);
	writel(mem->profiler_br2_dvaddr, sfr + PROFILER_BR2_BASE_ADDR);

	score_leave();
}

int score_memory_open(struct score_memory *mem)
{
	int ret = 0;

	score_enter();
	ret = __score_memory_alloc(mem);
	score_leave();
	return ret;
}

void score_memory_close(struct score_memory *mem)
{
	score_enter();
	__score_memory_free(mem);
	score_leave();
}

int score_memory_probe(struct score_mmu *mmu)
{
	int ret = 0;
	struct score_memory *mem;

	score_enter();
	mem = &mmu->mem;
	mem->dev = mmu->system->dev;
	mem->system = mmu->system;
	mmu->mmu_ops = &score_exynos_mmu_ops;

	dma_set_mask(mem->dev, DMA_BIT_MASK(36));

	score_leave();
	return ret;
}

void score_memory_remove(struct score_memory *mem)
{
	score_enter();
	score_leave();
}
