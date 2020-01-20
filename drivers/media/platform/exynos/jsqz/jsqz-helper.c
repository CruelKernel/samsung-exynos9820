/* drivers/media/platform/exynos/jsqz/jsqz-helper.c
 *
 * Implementation of helper functions for Samsung JPEG Squeezer driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Jungik Seo <jungik.seo@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

//#define DEBUG

#include <linux/kernel.h>
#include <linux/exynos_iovmm.h>
//#include <linux/exynos_ion.h>

#include "jsqz-helper.h"

int jsqz_map_dma_attachment(struct device *dev,
		     struct jsqz_buffer_plane_dma *plane,
		     enum dma_data_direction dir)
{
	dev_dbg(dev, "%s: mapping dmabuf %p, attachment %p, DMA_TO_DEVICE? %d\n"
		, __func__, plane->dmabuf, plane->attachment
		, dir == DMA_TO_DEVICE);

	if (plane->dmabuf) {
		//request access to the actual dma buffer
		//this will allocate and return a scatterlist mapped in our
		//device address space. The scatterlist point to physical pages.
		//We will then use IOMMU to map a CONTIGUOUS chunk of virtual
		//memory to it, that we will use for the DMA transfers.
		plane->sgt = dma_buf_map_attachment(plane->attachment, dir);
		if (IS_ERR(plane->sgt)) {
			dev_err(dev, "%s: failed to map attacment of dma_buf\n",
				__func__);
			return PTR_ERR(plane->sgt);
		}
		dev_dbg(dev, "%s: successfully got scatterlist %p for dmabuf %p, DMA_TO_DEVICE? %d\n"
			, __func__, plane->sgt, plane->dmabuf
			, dir == DMA_TO_DEVICE);
	}

	return 0;
}

void jsqz_unmap_dma_attachment(struct device *dev,
			struct jsqz_buffer_plane_dma *plane,
			enum dma_data_direction dir)
{
	dev_dbg(dev, "%s: unmapping dmabuf %p, attachment %p, scatter table %p, DMA_TO_DEVICE? %d\n"
		, __func__, plane->dmabuf, plane->attachment, plane->sgt
		, dir == DMA_TO_DEVICE);

	//tell the exporter of the DMA buffer that we're done using the buffer
	if (plane->dmabuf)
		dma_buf_unmap_attachment(plane->attachment, plane->sgt, dir);
}

int jsqz_dma_addr_map(struct device *dev,
		      struct jsqz_buffer_dma *buf,
		      enum dma_data_direction dir)
{
	struct jsqz_buffer_plane_dma *plane = &buf->plane;
	int prot = IOMMU_READ;
	dma_addr_t iova;

	dev_dbg(dev, "%s: using iovmm to get dma address, DMA_TO_DEVICE? %d\n"
		, __func__, dir == DMA_TO_DEVICE);

	if (dir != DMA_TO_DEVICE)
		prot |= IOMMU_WRITE;
	if (device_get_dma_attr(dev) == DEV_DMA_COHERENT)
		prot |= IOMMU_CACHE;

	if (plane->dmabuf) {
		dev_dbg(dev, "%s: getting dma addr of plane, DMA attachment %p bytes %zu, DMA_TO_DEVICE? %d\n"
			, __func__, plane->attachment, plane->bytes_used
			, dir == DMA_TO_DEVICE);
		iova = ion_iovmm_map(plane->attachment, 0,
				     plane->bytes_used, dir, prot);
	} else {
		dev_dbg(dev, "%s: getting dma addr of plane, userptr %lu bytes %zu, DMA_TO_DEVICE? %d\n"
			, __func__, buf->buffer->userptr
			, plane->bytes_used, dir == DMA_TO_DEVICE);
		down_read(&current->mm->mmap_sem);
		iova = exynos_iovmm_map_userptr(dev,
						buf->buffer->userptr,
						plane->bytes_used, prot);
		up_read(&current->mm->mmap_sem);
	}

	if (IS_ERR_VALUE(iova))
		return (int)iova;

	buf->plane.dma_addr = iova + plane->offset;


	dev_dbg(dev, "%s: got dma address %pad, DMA_TO_DEVICE? %d\n"
		,  __func__, &buf->plane.dma_addr, dir == DMA_TO_DEVICE);
	return 0;
}

void jsqz_dma_addr_unmap(struct device *dev,
			 struct jsqz_buffer_dma *buf)
{
	struct jsqz_buffer_plane_dma *plane = &buf->plane;
	dma_addr_t dma_addr = plane->dma_addr - plane->offset;

	dev_dbg(dev, "%s: umapping dma address %pad\n"
		,  __func__, &dma_addr);

	if (plane->dmabuf)
		ion_iovmm_unmap(plane->attachment, dma_addr);
	else
		exynos_iovmm_unmap_userptr(dev, dma_addr);

	plane->dma_addr = 0;
}

/*
void jsqz_sync_for_device(struct device *dev,
			  struct jsqz_buffer_plane_dma *plane,
			  enum dma_data_direction dir)
{
	if (plane->dmabuf)
		exynos_ion_sync_dmabuf_for_device(dev, plane->dmabuf,
						  plane->bytes_used, dir);
	else
		exynos_iommu_sync_for_device(dev, plane->dma_addr,
					     plane->bytes_used, dir);
}

void jsqz_sync_for_cpu(struct device *dev,
		       struct jsqz_buffer_plane_dma *plane,
		       enum dma_data_direction dir)
{
	if (plane->dmabuf)
		exynos_ion_sync_dmabuf_for_cpu(dev, plane->dmabuf,
					       plane->bytes_used, dir);
	else
		exynos_iommu_sync_for_cpu(dev, plane->dma_addr,
					  plane->bytes_used, dir);
}
*/
