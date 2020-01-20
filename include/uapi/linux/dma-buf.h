/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Framework for buffer objects that can be shared across devices/subsystems.
 *
 * Copyright(C) 2015 Intel Ltd
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

#ifndef _DMA_BUF_UAPI_H_
#define _DMA_BUF_UAPI_H_

#include <linux/types.h>

/* begin/end dma-buf functions used for userspace mmap. */
struct dma_buf_sync {
	__u64 flags;
};

#define DMA_BUF_SYNC_READ      (1 << 0)
#define DMA_BUF_SYNC_WRITE     (2 << 0)
#define DMA_BUF_SYNC_RW        (DMA_BUF_SYNC_READ | DMA_BUF_SYNC_WRITE)
#define DMA_BUF_SYNC_START     (0 << 2)
#define DMA_BUF_SYNC_END       (1 << 2)
#define DMA_BUF_SYNC_VALID_FLAGS_MASK \
	(DMA_BUF_SYNC_RW | DMA_BUF_SYNC_END)

#define DMA_BUF_BASE		'b'
#define DMA_BUF_IOCTL_SYNC	_IOW(DMA_BUF_BASE, 0, struct dma_buf_sync)

/*
 * create a dma-buf that is a container of the given dma-bufs.
 * The result dma-buf, dmabuf-container has dma_buf_merge.count+1 buffers
 * that includes the dma-buf that is the first argument to ioctl and the
 * dma-bufs given by dma_buf_merge.dma_bufs.
 */
struct dma_buf_merge {
	int   *dma_bufs;
	__s32 count;
	__s32 dmabuf_container;	/* output: result dmabuf of dmabuf_container */
	__u32 reserved[2];
};
#define DMA_BUF_IOCTL_MERGE	_IOWR(DMA_BUF_BASE, 13, struct dma_buf_merge)
#define DMA_BUF_IOCTL_CONTAINER_SET_MASK	_IOW(DMA_BUF_BASE, 14, __u32)
#define DMA_BUF_IOCTL_CONTAINER_GET_MASK	_IOR(DMA_BUF_BASE, 14, __u32)

#define DMA_BUF_IOCTL_TRACK			_IO(DMA_BUF_BASE, 8)
#define DMA_BUF_IOCTL_UNTRACK			_IO(DMA_BUF_BASE, 9)

#endif
