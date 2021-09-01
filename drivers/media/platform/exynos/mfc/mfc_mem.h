/*
 * drivers/media/platform/exynos/mfc/mfc_mem.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_MEM_H
#define __MFC_MEM_H __FILE__

#include "mfc_common.h"

/* Offset base used to differentiate between CAPTURE and OUTPUT
*  while mmaping */
#define DST_QUEUE_OFF_BASE      (TASK_SIZE / 2)

static inline dma_addr_t mfc_mem_get_daddr_vb(
	struct vb2_buffer *vb, unsigned int n)
{
	dma_addr_t addr = 0;

	addr = vb2_dma_sg_plane_dma_addr(vb, n);
	WARN_ON((addr == 0) || IS_ERR_VALUE(addr));

	return addr;
}

static inline int mfc_bufcon_get_buf_count(struct dma_buf *dmabuf)
{
	return dmabuf_container_get_count(dmabuf);
}

struct vb2_mem_ops *mfc_mem_ops(void);

void mfc_mem_set_cacheable(bool cacheable);
void mfc_mem_clean(struct mfc_dev *dev,
			struct mfc_special_buf *specail_buf,
			off_t offset, size_t size);
void mfc_mem_invalidate(struct mfc_dev *dev,
			struct mfc_special_buf *specail_buf,
			off_t offset, size_t size);

int mfc_mem_get_user_shared_handle(struct mfc_ctx *ctx,
		struct mfc_user_shared_handle *handle);
void mfc_mem_cleanup_user_shared_handle(struct mfc_ctx *ctx,
		struct mfc_user_shared_handle *handle);

void mfc_put_iovmm(struct mfc_ctx *ctx, int num_planes, int index, int spare);
void mfc_get_iovmm(struct mfc_ctx *ctx, struct vb2_buffer *vb);
void mfc_move_iovmm_to_spare(struct mfc_ctx *ctx, int num_planes, int index);
void mfc_cleanup_assigned_iovmm(struct mfc_ctx *ctx);

int mfc_mem_ion_alloc(struct mfc_dev *dev,
		struct mfc_special_buf *special_buf);
void mfc_mem_ion_free(struct mfc_dev *dev,
		struct mfc_special_buf *special_buf);

void mfc_bufcon_put_daddr(struct mfc_ctx *ctx, struct mfc_buf *mfc_buf, int plane);
int mfc_bufcon_get_daddr(struct mfc_ctx *ctx, struct mfc_buf *mfc_buf,
					struct dma_buf *bufcon_dmabuf, int plane);
#endif /* __MFC_MEM_H */
