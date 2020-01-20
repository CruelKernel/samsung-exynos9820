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

#include <linux/slab.h>
#include <linux/ion.h>
#include <linux/dma-buf.h>
#include <linux/iommu.h>
#include <linux/exynos_iovmm.h>

#include "score_log.h"
#include "score_memory.h"
#include "score_ion.h"

struct score_ion_mem {
	struct ion_handle		*handle;
	struct dma_buf			*dma_buf;
	struct dma_buf_attachment	*attachment;
	struct sg_table			*sgt;
	dma_addr_t			ioaddr;
	unsigned long			size;
	void				*kva;
};

struct score_ion_context {
	struct device			*dev;
	struct ion_client		*client;
	long				flags;
	struct score_ion_mem		*mem;
};

int score_ion_alloc(void *alloc_ctx, size_t size,
		dma_addr_t *dvaddr, void **kvaddr)
{
	struct score_ion_context *ctx;
	struct score_ion_mem *mem;
	int heapflags;
	int flags;
	int ret = 0;

	score_enter();
	if (!dvaddr || !kvaddr) {
		ret = -EINVAL;
		score_err("output address is invalid\n");
		goto p_err;
	}

	ctx = alloc_ctx;
	mem = kzalloc(sizeof(*mem), GFP_KERNEL);
	if (!mem) {
		ret = -ENOMEM;
		score_err("Fail to alloc ion memory structure\n");
		goto p_err;
	}
	ctx->mem = mem;

	size = PAGE_ALIGN(size);
	heapflags = EXYNOS_ION_HEAP_SYSTEM_MASK;
	flags = 0;

	mem->handle = ion_alloc(ctx->client, size, 0,
			heapflags, flags);
	if (IS_ERR(mem->handle)) {
		score_err("It is failed to allocate ion\n");
		ret = -ENOMEM;
		goto p_err_alloc;
	}

	mem->dma_buf = ion_share_dma_buf(ctx->client, mem->handle);
	if (IS_ERR(mem->dma_buf)) {
		ret = PTR_ERR(mem->dma_buf);
		score_err("It is failed to create dma_buf (%d)\n", ret);
		goto p_err_share;
	}

	mem->attachment = dma_buf_attach(mem->dma_buf, ctx->dev);
	if (IS_ERR(mem->attachment)) {
		ret = PTR_ERR(mem->attachment);
		score_err("It is failed to attach dma_buf (%d)\n", ret);
		goto p_err_attach;
	}

	mem->sgt = dma_buf_map_attachment(mem->attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR(mem->sgt)) {
		ret = PTR_ERR(mem->sgt);
		score_err("It is failed to map attachment (%d)\n", ret);
		goto p_err_map_attachment;
	}

	mem->size = size;
	mem->ioaddr = ion_iovmm_map(mem->attachment, 0, mem->size,
			DMA_BIDIRECTIONAL, 0);
	if (IS_ERR_VALUE(mem->ioaddr)) {
		ret = (int)mem->ioaddr;
		score_err("It is failed to map ioaddr (%d)\n", ret);
		goto p_err_ion_map_iova;
	}

	mem->kva = ion_map_kernel(ctx->client, mem->handle);
	if (IS_ERR(mem->kva)) {
		score_err("It is failed to map kvaddress (%d)\n", ret);
		ret = PTR_ERR(mem->kva);
		goto p_err_kmap;
	}

	*dvaddr = mem->ioaddr;
	*kvaddr = mem->kva;

	score_enter();
	return 0;
p_err_kmap:
	ion_iovmm_unmap(mem->attachment, mem->ioaddr);
p_err_ion_map_iova:
	dma_buf_unmap_attachment(mem->attachment, mem->sgt, DMA_BIDIRECTIONAL);
p_err_map_attachment:
	dma_buf_detach(mem->dma_buf, mem->attachment);
p_err_attach:
	dma_buf_put(mem->dma_buf);
p_err_share:
	ion_free(ctx->client, mem->handle);
p_err_alloc:
	kfree(mem);
p_err:
	return ret;
}

void score_ion_free(void *alloc_ctx)
{
	struct score_ion_context *ctx;
	struct score_ion_mem *mem;

	score_enter();
	ctx = alloc_ctx;
	mem = ctx->mem;

	ion_iovmm_unmap(mem->attachment, mem->ioaddr);

	dma_buf_unmap_attachment(mem->attachment, mem->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(mem->dma_buf, mem->attachment);
	dma_buf_put(mem->dma_buf);

	ion_unmap_kernel(ctx->client, mem->handle);
	ion_free(ctx->client, mem->handle);

	kfree(mem);
	score_enter();
}

int score_ion_create_context(struct score_memory *mem)
{
	int ret;
	struct score_ion_context *ctx;

	score_enter();
	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		ret = -ENOMEM;
		score_err("Fail to alloc ion context structure\n");
		goto p_err;
	}

	mem->ctx = ctx;
	ctx->dev = mem->dev;
	ctx->client = exynos_ion_client_create(dev_name(ctx->dev));
	if (IS_ERR(ctx->client)) {
		ret = PTR_ERR(ctx->client);
		score_err("It is failed to create ion context (%d)\n", ret);
		goto p_err_client;
	}

	score_enter();
	return 0;
p_err_client:
	kfree(ctx);
p_err:
	return ret;
}

void score_ion_destroy_context(void *alloc_ctx)
{
	struct score_ion_context *ctx = alloc_ctx;

	score_enter();
	ion_client_destroy(ctx->client);
	kfree(ctx);
	score_enter();
}
