/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <video/videonode.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/dma-buf.h>
#include <linux/dma-buf-container.h>
#include <linux/ion_exynos.h>
#include <asm/cacheflush.h>
#include <media/videobuf2-memops.h>

#include "fimc-is-core.h"
#include "fimc-is-cmd.h"
#include "fimc-is-regs.h"
#include "fimc-is-err.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
#include <linux/exynos_iovmm.h>
#else
#include <plat/iovmm.h>
#endif

#if defined(CONFIG_VIDEOBUF2_DMA_SG)
struct vb2_dma_sg_buf {
	struct device			*dev;
	void				*vaddr;
	struct page			**pages;
	struct frame_vector		*vec;
	int				offset;
	enum dma_data_direction		dma_dir;
	struct sg_table			sg_table;
	/*
	 * This will point to sg_table when used with the MMAP or USERPTR
	 * memory model, and to the dma_buf sglist when used with the
	 * DMABUF memory model.
	 */
	struct sg_table			*dma_sgt;
	size_t				size;
	unsigned int			num_pages;
	refcount_t			refcount;
	struct vb2_vmarea_handler	handler;

	struct dma_buf_attachment	*db_attach;
	/*
	 * Our IO address space is not managed by dma-mapping. Therefore
	 * scatterlist.dma_address should not be corrupted by the IO address
	 * returned by iovmm_map() because it is used by cache maintenance.
	 */
	dma_addr_t			iova;
};

/* fimc-is vb2 buffer operations */
static inline ulong is_vb2_dma_sg_plane_kvaddr(
		struct fimc_is_vb2_buf *vbuf, u32 plane)

{
	return (ulong)vb2_plane_vaddr(&vbuf->vb.vb2_buf, plane);
}

static inline dma_addr_t is_vb2_dma_sg_plane_dvaddr(
		struct fimc_is_vb2_buf *vbuf, u32 plane)

{
	return vb2_dma_sg_plane_dma_addr(&vbuf->vb.vb2_buf, plane);
}

static inline ulong is_vb2_dma_sg_plane_kmap(
		struct fimc_is_vb2_buf *vbuf, u32 plane)
{
	struct vb2_buffer *vb = &vbuf->vb.vb2_buf;
	struct vb2_dma_sg_buf *buf = vb->planes[plane].mem_priv;
	struct dma_buf *dbuf;

	if (likely(vbuf->kva[plane]))
		return vbuf->kva[plane];

	dbuf = dma_buf_get(vb->planes[plane].m.fd);
	if (IS_ERR_OR_NULL(dbuf)) {
		pr_err("failed to get dmabuf of fd: %d, plane: %d\n",
				vb->planes[plane].m.fd, plane);
		return 0;
	}

	if (dma_buf_begin_cpu_access(dbuf, buf->dma_dir)) {
		dma_buf_put(dbuf);
		pr_err("failed to access dmabuf of fd: %d, plane: %d\n",
				vb->planes[plane].m.fd, plane);
		return 0;
	}

	vbuf->kva[plane] = (ulong)dma_buf_kmap(dbuf, buf->offset / PAGE_SIZE);
	if (!vbuf->kva[plane]) {
		dma_buf_end_cpu_access(dbuf, buf->dma_dir);
		dma_buf_put(dbuf);
		pr_err("failed to kmapping dmabuf of fd: %d, plane: %d\n",
				vb->planes[plane].m.fd, plane);
		return 0;
	}

	vbuf->dbuf[plane] = dbuf;
	vbuf->kva[plane] += buf->offset & ~PAGE_MASK;

	return vbuf->kva[plane];
}

static inline void is_vb2_dma_sg_plane_kunmap(
		struct fimc_is_vb2_buf *vbuf, u32 plane)
{
	struct dma_buf *dbuf = vbuf->dbuf[plane];

	if (vbuf->kva[plane]) {
		dma_buf_kunmap(dbuf, 0, (void *)vbuf->kva[plane]);
		dma_buf_end_cpu_access(dbuf, 0);
		dma_buf_put(dbuf);

		vbuf->dbuf[plane] = NULL;
		vbuf->kva[plane] = 0;
	}
}

static long is_vb2_dma_sg_remap_attr(struct fimc_is_vb2_buf *vbuf, int attr)
{
	struct vb2_buffer *vb = &vbuf->vb.vb2_buf;
	struct vb2_dma_sg_buf *buf;
	unsigned int plane;
	long ret;
	int ioprot = IOMMU_READ | IOMMU_WRITE;

	for (plane = 0; plane < vb->num_planes; ++plane) {
		buf = vb->planes[plane].mem_priv;
		vbuf->sgt[plane] = dma_buf_map_attachment(buf->db_attach, buf->dma_dir);

		if (IS_ERR(vbuf->sgt[plane])) {
			pr_err("Error getting dmabuf scatterlist\n");
			ret = PTR_ERR(vbuf->sgt[plane]);
			goto err_get_sgt;
		}

		if ((vbuf->dva[plane] == 0) || IS_ERR_VALUE(vbuf->dva[plane])) {
			if (device_get_dma_attr(buf->dev) == DEV_DMA_COHERENT)
				ioprot |= IOMMU_CACHE;

			vbuf->dva[plane] = ion_iovmm_map_attr(buf->db_attach, 0, buf->size,
					DMA_BIDIRECTIONAL, ioprot, attr);
			if (IS_ERR_VALUE(vbuf->dva[plane])) {
				pr_err("Error from ion_iovmm_map()=%pad\n", &vbuf->dva[plane]);
				ret = vbuf->dva[plane];
				goto err_map_remap;
			}
		}
	}

	return 0;

err_map_remap:
	dma_buf_unmap_attachment(buf->db_attach, vbuf->sgt[plane], buf->dma_dir);
	vbuf->dva[plane] = 0;

err_get_sgt:
	while (plane-- > 0) {
		buf = vb->planes[plane].mem_priv;

		ion_iovmm_unmap_attr(buf->db_attach, vbuf->dva[plane], attr);
		dma_buf_unmap_attachment(buf->db_attach, vbuf->sgt[plane], buf->dma_dir);
		vbuf->dva[plane] = 0;
	}

	return ret;
}

void is_vb2_dma_sg_unremap_attr(struct fimc_is_vb2_buf *vbuf, int attr)
{
	struct vb2_buffer *vb = &vbuf->vb.vb2_buf;
	struct vb2_dma_sg_buf *buf;
	unsigned int plane;

	for (plane = 0; plane < vb->num_planes; ++plane) {
		buf = vb->planes[plane].mem_priv;

		ion_iovmm_unmap_attr(buf->db_attach, vbuf->dva[plane], attr);
		dma_buf_unmap_attachment(buf->db_attach, vbuf->sgt[plane], buf->dma_dir);
		vbuf->dva[plane] = 0;
	}
}

#ifdef CONFIG_DMA_BUF_CONTAINER
static long is_dbufcon_prepare(struct fimc_is_vb2_buf *vbuf,
		u32 num_planes, struct device *dev)
{
	struct vb2_buffer *vb = &vbuf->vb.vb2_buf;
	struct dma_buf *dbufcon;
	int count;
	int p, b, i;
	long ret;

	for (p = 0; p < num_planes; p++) {
		dbufcon = dma_buf_get(vb->planes[p].m.fd);
		if (IS_ERR_OR_NULL(dbufcon)) {
			err("failed to get dmabuf of fd: %d, plane: %d",
				vb->planes[p].m.fd, p);
			ret = -EINVAL;
			goto err_get_dbufcon;
		}

		count = dmabuf_container_get_count(dbufcon);
		if (count < 0) {
			ret = 0;
			goto err_get_count;
		} else if (count == 0) {
			err("empty dmabuf-container of fd: %d", vb->planes[p].m.fd);
			ret = -EINVAL;
			goto err_empty_dbufcon;
		}

		/* FIMC_IS_MAX_PLANES includes a meta-plane */
		if ((count * num_planes) >= FIMC_IS_MAX_PLANES) {
			err("out of range plane. count: %d, planes: %d >= supported: %d",
					count, num_planes, FIMC_IS_MAX_PLANES);
			ret = -EINVAL;
			goto err_too_many_planes;

		}

		/* change the order of each plane by the order of buffer */
		for (b = 0, i = p; b < count; b++, i = (b * num_planes) + p) {
			vbuf->dbuf[i] = dmabuf_container_get_buffer(dbufcon, b);
			if (IS_ERR_OR_NULL(vbuf->dbuf[i])) {
				err("failed to get dmabuf-container's dmabuf: %d", b);
				ret = -EINVAL;
				goto err_get_dbuf;
			}

			vbuf->atch[i] = dma_buf_attach(vbuf->dbuf[i], dev);
			if (IS_ERR(vbuf->atch[i])) {
				err("failed to attach dmabuf: %d", b);
				ret = PTR_ERR(vbuf->atch[i]);
				goto err_attach_dbuf;
			}

			vbuf->sgt[i] = dma_buf_map_attachment(vbuf->atch[i],
							DMA_BIDIRECTIONAL);
			if (IS_ERR(vbuf->sgt[i])) {
				err("failed to get dmabuf's sgt: %d", b);
				ret = PTR_ERR(vbuf->sgt[i]);
				goto err_get_sgt;
			}
		}

		dma_buf_put(dbufcon);
	}

	vbuf->num_merged_dbufs = count * num_planes;

	return 0;

err_get_sgt:
	dma_buf_detach(vbuf->dbuf[i], vbuf->atch[i]);
	vbuf->sgt[i] = NULL;

err_attach_dbuf:
	dma_buf_put(vbuf->dbuf[i]);
	vbuf->atch[i] = NULL;
	vbuf->dbuf[i] = NULL;

err_get_dbuf:
	while (b-- > 0) {
		i = (b * num_planes) + p;
		dma_buf_unmap_attachment(vbuf->atch[i], vbuf->sgt[i],
				DMA_BIDIRECTIONAL);
		dma_buf_detach(vbuf->dbuf[i], vbuf->atch[i]);
		dma_buf_put(vbuf->dbuf[i]);
		vbuf->sgt[i] = NULL;
		vbuf->atch[i] = NULL;
		vbuf->dbuf[i] = NULL;
	}

err_too_many_planes:
err_empty_dbufcon:
err_get_count:
	dma_buf_put(dbufcon);

err_get_dbufcon:
	vbuf->num_merged_dbufs = 0;

	while (p-- > 0) {
		dbufcon = dma_buf_get(vb->planes[p].m.fd);
		count = dmabuf_container_get_count(dbufcon);
		dma_buf_put(dbufcon);

		for (b = 0, i = p; b < count; b++, i = (b * num_planes) + p) {
			dma_buf_unmap_attachment(vbuf->atch[i], vbuf->sgt[i],
					DMA_BIDIRECTIONAL);
			dma_buf_detach(vbuf->dbuf[i], vbuf->atch[i]);
			dma_buf_put(vbuf->dbuf[i]);
			vbuf->sgt[i] = NULL;
			vbuf->atch[i] = NULL;
			vbuf->dbuf[i] = NULL;
		}
	}

	return ret;
}

static void is_dbufcon_finish(struct fimc_is_vb2_buf *vbuf)
{
	int i;

	for (i = 0; i < vbuf->num_merged_dbufs; i++) {
		dma_buf_unmap_attachment(vbuf->atch[i], vbuf->sgt[i],
				DMA_BIDIRECTIONAL);
		dma_buf_detach(vbuf->dbuf[i], vbuf->atch[i]);
		dma_buf_put(vbuf->dbuf[i]);
		vbuf->sgt[i] = NULL;
		vbuf->atch[i] = NULL;
		vbuf->dbuf[i] = NULL;
	}

	vbuf->num_merged_dbufs = 0;
}

static long is_dbufcon_map(struct fimc_is_vb2_buf *vbuf)
{
	int i;
	long ret;

	for (i = 0; i < vbuf->num_merged_dbufs; i++) {
		vbuf->dva[i] = ion_iovmm_map(vbuf->atch[i], 0, 0,
				DMA_BIDIRECTIONAL, 0);
		if (IS_ERR_VALUE(vbuf->dva[i])) {
			err("failed to map dmabuf: %d", i);
			ret = vbuf->dva[i];
			goto err_map_dbuf;
		}
	}

	return 0;

err_map_dbuf:
	vbuf->dva[i] = 0;

	while (i-- > 0) {
		ion_iovmm_unmap(vbuf->atch[i], vbuf->dva[i]);
		vbuf->dva[i] = 0;
	}

	return ret;
}

static void is_dbufcon_unmap(struct fimc_is_vb2_buf *vbuf)
{
	int i;

	for (i = 0; i < vbuf->num_merged_dbufs; i++) {
		ion_iovmm_unmap(vbuf->atch[i], vbuf->dva[i]);
		vbuf->dva[i] = 0;
	}
}
#else
static long is_dbufcon_prepare(struct fimc_is_vb2_buf *vbuf,
		u32 num_planes, struct device *dev)
{
	return 0;
}
static void is_dbufcon_finish(struct fimc_is_vb2_buf *vbuf)
{
}

static long is_dbufcon_map(struct fimc_is_vb2_buf *vbuf)
{
	return 0;
}

static void is_dbufcon_unmap(struct fimc_is_vb2_buf *vbuf)
{
}
#endif

const struct fimc_is_vb2_buf_ops is_vb2_buf_ops_dma_sg = {
	.plane_kvaddr		= is_vb2_dma_sg_plane_kvaddr,
	.plane_dvaddr		= is_vb2_dma_sg_plane_dvaddr,
	.plane_kmap		= is_vb2_dma_sg_plane_kmap,
	.plane_kunmap		= is_vb2_dma_sg_plane_kunmap,
	.remap_attr		= is_vb2_dma_sg_remap_attr,
	.unremap_attr		= is_vb2_dma_sg_unremap_attr,
	.dbufcon_prepare	= is_dbufcon_prepare,
	.dbufcon_finish		= is_dbufcon_finish,
	.dbufcon_map		= is_dbufcon_map,
	.dbufcon_unmap		= is_dbufcon_unmap,
};

/* fimc-is private buffer operations */
static void fimc_is_ion_free(struct fimc_is_priv_buf *pbuf)
{
	struct fimc_is_ion_ctx *alloc_ctx;

	alloc_ctx = pbuf->ctx;
	mutex_lock(&alloc_ctx->lock);
	if (pbuf->iova)
		ion_iovmm_unmap(pbuf->attachment, pbuf->iova);
	mutex_unlock(&alloc_ctx->lock);

	if (pbuf->kva)
		dma_buf_vunmap(pbuf->dma_buf, pbuf->kva);

	dma_buf_unmap_attachment(pbuf->attachment, pbuf->sgt,
				DMA_BIDIRECTIONAL);
	dma_buf_detach(pbuf->dma_buf, pbuf->attachment);
	dma_buf_put(pbuf->dma_buf);

	vfree(pbuf);
}

static ulong fimc_is_ion_kvaddr(struct fimc_is_priv_buf *pbuf)
{
	if (!pbuf)
		return 0;

	if (!pbuf->kva)
		pbuf->kva = dma_buf_vmap(pbuf->dma_buf);

	return (ulong)pbuf->kva;
}

static dma_addr_t fimc_is_ion_dvaddr(struct fimc_is_priv_buf *pbuf)
{
	dma_addr_t dva = 0;

	if (!pbuf)
		return -EINVAL;

	if (pbuf->iova == 0)
		return -EINVAL;

	dva = pbuf->iova;

	return (ulong)dva;
}

static phys_addr_t fimc_is_ion_phaddr(struct fimc_is_priv_buf *pbuf)
{
	/* PHCONTIG option is not supported in ION */
	return 0;
}

static void fimc_is_ion_sync_for_device(struct fimc_is_priv_buf *pbuf,
		off_t offset, size_t size, enum dma_data_direction dir)
{
	if (pbuf->kva) {
		FIMC_BUG_VOID((offset < 0) || (offset > pbuf->size));
		FIMC_BUG_VOID((offset + size) < size);
		FIMC_BUG_VOID((size > pbuf->size) || ((offset + size) > pbuf->size));

		__dma_map_area(pbuf->kva + offset, size, dir);
	}
}

static void fimc_is_ion_sync_for_cpu(struct fimc_is_priv_buf *pbuf,
		off_t offset, size_t size, enum dma_data_direction dir)
{
	if (pbuf->kva) {
		FIMC_BUG_VOID((offset < 0) || (offset > pbuf->size));
		FIMC_BUG_VOID((offset + size) < size);
		FIMC_BUG_VOID((size > pbuf->size) || ((offset + size) > pbuf->size));

		__dma_unmap_area(pbuf->kva + offset, size, dir);
	}
}

const struct fimc_is_priv_buf_ops fimc_is_priv_buf_ops_ion = {
	.free			= fimc_is_ion_free,
	.kvaddr			= fimc_is_ion_kvaddr,
	.dvaddr			= fimc_is_ion_dvaddr,
	.phaddr			= fimc_is_ion_phaddr,
	.sync_for_device	= fimc_is_ion_sync_for_device,
	.sync_for_cpu		= fimc_is_ion_sync_for_cpu,
};

/* fimc-is memory operations */
static void *fimc_is_ion_init(struct platform_device *pdev)
{
	struct fimc_is_ion_ctx *ctx;

	ctx = vzalloc(sizeof(*ctx));
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	ctx->dev = &pdev->dev;
	ctx->alignment = SZ_4K;
	ctx->flags = ION_FLAG_CACHED;
	mutex_init(&ctx->lock);

	return ctx;
}

static void fimc_is_ion_deinit(void *ctx)
{
	struct fimc_is_ion_ctx *alloc_ctx = ctx;

	mutex_destroy(&alloc_ctx->lock);
	vfree(alloc_ctx);
}

static struct fimc_is_priv_buf *fimc_is_ion_alloc(void *ctx,
		size_t size, const char *heapname, unsigned int flags)
{
	struct fimc_is_ion_ctx *alloc_ctx = ctx;
	struct fimc_is_priv_buf *buf;
	int ret = 0;

	buf = vzalloc(sizeof(*buf));
	if (!buf)
		return ERR_PTR(-ENOMEM);

	size = PAGE_ALIGN(size);

	buf->dma_buf = ion_alloc_dmabuf(heapname ? heapname : "ion_system_heap",
				size, flags ? flags : alloc_ctx->flags);
	if (IS_ERR(buf->dma_buf)) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	buf->attachment = dma_buf_attach(buf->dma_buf, alloc_ctx->dev);
	if (IS_ERR(buf->attachment)) {
		ret = PTR_ERR(buf->attachment);
		goto err_attach;
	}

	buf->sgt = dma_buf_map_attachment(
				buf->attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR(buf->sgt)) {
		ret = PTR_ERR(buf->sgt);
		goto err_map_dmabuf;
	}

	buf->ctx = alloc_ctx;
	buf->size = size;
	buf->direction = DMA_BIDIRECTIONAL;
	buf->ops = &fimc_is_priv_buf_ops_ion;

	mutex_lock(&alloc_ctx->lock);
	buf->iova = ion_iovmm_map(buf->attachment, 0,
				   buf->size, buf->direction, 0);
	if (IS_ERR_VALUE(buf->iova)) {
		ret = (int)buf->iova;
		mutex_unlock(&alloc_ctx->lock);
		goto err_ion_map_io;
	}
	mutex_unlock(&alloc_ctx->lock);

	return buf;

err_ion_map_io:
	dma_buf_unmap_attachment(buf->attachment, buf->sgt,
				DMA_BIDIRECTIONAL);
err_map_dmabuf:
	dma_buf_detach(buf->dma_buf, buf->attachment);
err_attach:
	dma_buf_put(buf->dma_buf);
err_alloc:
	vfree(buf);

	pr_err("%s: Error occured while allocating\n", __func__);
	return ERR_PTR(ret);
}

static int fimc_is_ion_resume(void *ctx)
{
	struct fimc_is_ion_ctx *alloc_ctx = ctx;
	int ret = 0;

	if (!alloc_ctx)
		return -ENOENT;

	mutex_lock(&alloc_ctx->lock);
	if (alloc_ctx->iommu_active_cnt == 0)
		ret = iovmm_activate(alloc_ctx->dev);
	if (!ret)
		alloc_ctx->iommu_active_cnt++;
	mutex_unlock(&alloc_ctx->lock);

	return ret;
}

static void fimc_is_ion_suspend(void *ctx)
{
	struct fimc_is_ion_ctx *alloc_ctx = ctx;

	if (!alloc_ctx)
		return;

	mutex_lock(&alloc_ctx->lock);
	FIMC_BUG_VOID(alloc_ctx->iommu_active_cnt == 0);

	if (--alloc_ctx->iommu_active_cnt == 0)
		iovmm_deactivate(alloc_ctx->dev);
	mutex_unlock(&alloc_ctx->lock);
}

const struct fimc_is_mem_ops fimc_is_mem_ops_ion = {
	.init			= fimc_is_ion_init,
	.cleanup		= fimc_is_ion_deinit,
	.resume			= fimc_is_ion_resume,
	.suspend		= fimc_is_ion_suspend,
	.alloc			= fimc_is_ion_alloc,
};
#endif

/* fimc-is private buffer operations */
static void fimc_is_km_free(struct fimc_is_priv_buf *pbuf)
{
	kfree(pbuf->kvaddr);
	kfree(pbuf);
}

static ulong fimc_is_km_kvaddr(struct fimc_is_priv_buf *pbuf)
{
	if (!pbuf)
		return 0;

	return (ulong)pbuf->kvaddr;
}

static phys_addr_t fimc_is_km_phaddr(struct fimc_is_priv_buf *pbuf)
{
	phys_addr_t pa = 0;

	if (!pbuf)
		return 0;

	pa = virt_to_phys(pbuf->kvaddr);

	return pa;
}

const struct fimc_is_priv_buf_ops fimc_is_priv_buf_ops_km = {
	.free			= fimc_is_km_free,
	.kvaddr			= fimc_is_km_kvaddr,
	.phaddr			= fimc_is_km_phaddr,
};

static struct fimc_is_priv_buf *fimc_is_kmalloc(size_t size, size_t align)
{
	struct fimc_is_priv_buf *buf = NULL;
	int ret = 0;

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->kvaddr = kzalloc(DEBUG_REGION_SIZE + 0x10, GFP_KERNEL);
	if (!buf->kvaddr) {
		ret = -ENOMEM;
		goto err_priv_alloc;
	}

	buf->size = size;
	buf->align = align;
	buf->ops = &fimc_is_priv_buf_ops_km;

	return buf;

err_priv_alloc:
	kfree(buf);

	return ERR_PTR(ret);
}

int fimc_is_mem_init(struct fimc_is_mem *mem, struct platform_device *pdev)
{
#if defined(CONFIG_VIDEOBUF2_DMA_SG)
	mem->fimc_is_mem_ops = &fimc_is_mem_ops_ion;
	mem->vb2_mem_ops = &vb2_dma_sg_memops;
	mem->fimc_is_vb2_buf_ops = &is_vb2_buf_ops_dma_sg;
	mem->kmalloc = &fimc_is_kmalloc;
#endif

	mem->default_ctx = CALL_PTR_MEMOP(mem, init, pdev);
	if (IS_ERR_OR_NULL(mem->default_ctx)) {
		if (IS_ERR(mem->default_ctx))
			return PTR_ERR(mem->default_ctx);
		else
			return -EINVAL;
	}

	return 0;
}
