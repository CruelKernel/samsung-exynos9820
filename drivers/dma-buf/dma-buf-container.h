/*
 * Copyright(C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DMA_BUF_CONTAINER_H_
#define _DMA_BUF_CONTAINER_H_

#include <linux/types.h>
#include <linux/compat.h>

#include <uapi/linux/dma-buf.h>

#ifdef CONFIG_COMPAT

struct compat_dma_buf_merge {
	compat_uptr_t dma_bufs;
	__s32 count;
	__s32 dmabuf_container;
	__u32 reserved[2];
};
#define DMA_BUF_COMPAT_IOCTL_MERGE \
		_IOWR(DMA_BUF_BASE, 13, struct compat_dma_buf_merge)

#endif

struct dma_buf;

#ifdef CONFIG_DMA_BUF_CONTAINER

long dma_buf_merge_ioctl(struct dma_buf *dmabuf,
			 unsigned int cmd, unsigned long arg);

int dmabuf_container_set_mask_user(struct dma_buf *dmabuf, unsigned long arg);
int dmabuf_container_get_mask_user(struct dma_buf *dmabuf, unsigned long arg);
#else

static inline long dma_buf_merge_ioctl(struct dma_buf *dmabuf,
				       unsigned int cmd, unsigned long arg)
{
	return -ENOTTY;
}

static inline int dmabuf_container_set_mask_user(struct dma_buf *dmabuf,
						 unsigned long arg)
{
	return -ENOTTY;
}
static inline int dmabuf_container_get_mask_user(struct dma_buf *dmabuf,
						 unsigned long arg)
{
	return -ENOTTY;
}
#endif

#endif /* _DMA_BUF_CONTAINER_H_ */
