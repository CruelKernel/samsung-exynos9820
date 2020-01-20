/*
 * drivers/media/m2m1shot-helper.c
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Cho KyongHo <pullip.cho@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */
#include <linux/kernel.h>
#include <linux/exynos_iovmm.h>
#include <linux/ion_exynos.h>

#include <media/m2m1shot-helper.h>

int m2m1shot_map_dma_buf(struct device *dev,
			struct m2m1shot_buffer_plane_dma *plane,
			enum dma_data_direction dir)
{
	if (plane->dmabuf) {
		plane->sgt = dma_buf_map_attachment(plane->attachment, dir);
		if (IS_ERR(plane->sgt)) {
			dev_err(dev, "%s: failed to map attacment of dma_buf\n",
					__func__);
			return PTR_ERR(plane->sgt);
		}
	}

	return 0;
}
EXPORT_SYMBOL(m2m1shot_map_dma_buf);

void m2m1shot_unmap_dma_buf(struct device *dev,
			struct m2m1shot_buffer_plane_dma *plane,
			enum dma_data_direction dir)
{
	if (plane->dmabuf)
		dma_buf_unmap_attachment(plane->attachment, plane->sgt, dir);
}
EXPORT_SYMBOL(m2m1shot_unmap_dma_buf);

static inline bool is_dma_coherent(struct device *dev)
{
	return device_get_dma_attr(dev) == DEV_DMA_COHERENT;
}

int m2m1shot_dma_addr_map(struct device *dev,
			struct m2m1shot_buffer_dma *buf,
			int plane_idx, enum dma_data_direction dir)
{
	struct m2m1shot_buffer_plane_dma *plane = &buf->plane[plane_idx];
	dma_addr_t iova;
	int prot = IOMMU_READ;

	if (dir == DMA_FROM_DEVICE)
		prot |= IOMMU_WRITE;
	if (is_dma_coherent(dev))
		prot |= IOMMU_CACHE;

	if (plane->dmabuf) {
		iova = ion_iovmm_map(plane->attachment, 0,
					plane->bytes_used, dir, prot);
	} else {
		down_read(&current->mm->mmap_sem);
		iova = exynos_iovmm_map_userptr(dev,
				buf->buffer->plane[plane_idx].userptr,
				plane->bytes_used, prot);
		up_read(&current->mm->mmap_sem);
	}

	if (IS_ERR_VALUE(iova))
		return (int)iova;

	buf->plane[plane_idx].dma_addr = iova + plane->offset;

	return 0;
}

void m2m1shot_dma_addr_unmap(struct device *dev,
			struct m2m1shot_buffer_dma *buf, int plane_idx)
{
	struct m2m1shot_buffer_plane_dma *plane = &buf->plane[plane_idx];
	dma_addr_t dma_addr = plane->dma_addr - plane->offset;

	if (plane->dmabuf)
		ion_iovmm_unmap(plane->attachment, dma_addr);
	else
		exynos_iovmm_unmap_userptr(dev, dma_addr);

	plane->dma_addr = 0;
}

void m2m1shot_sync_for_device(struct device *dev,
			      struct m2m1shot_buffer_plane_dma *plane,
			      enum dma_data_direction dir)
{
	if (is_dma_coherent(dev))
		return;

	if (plane->dmabuf)
		dma_sync_sg_for_device(dev, plane->sgt->sgl,
				       plane->sgt->orig_nents, dir);
	else
		exynos_iommu_sync_for_device(dev, plane->dma_addr,
					     plane->bytes_used, dir);
}
EXPORT_SYMBOL(m2m1shot_sync_for_device);

void m2m1shot_sync_for_cpu(struct device *dev,
			   struct m2m1shot_buffer_plane_dma *plane,
			   enum dma_data_direction dir)
{
	if (is_dma_coherent(dev))
		return;

	if (plane->dmabuf)
		dma_sync_sg_for_cpu(dev, plane->sgt->sgl,
				       plane->sgt->orig_nents, dir);
	else
		exynos_iommu_sync_for_cpu(dev, plane->dma_addr,
					  plane->bytes_used, dir);
}
EXPORT_SYMBOL(m2m1shot_sync_for_cpu);
