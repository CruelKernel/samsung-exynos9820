/*
 * linux/drivers/gpu/exynos/g2d/g2d_uapi_process.c
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/dma-buf.h>
#include <linux/sync_file.h>
#include <linux/iommu.h>
#include <linux/ion_exynos.h>
#include <linux/slab.h>
#include <linux/sched/mm.h>
#include <linux/exynos_iovmm.h>

#include <asm/cacheflush.h>

#include "g2d.h"
#include "g2d_task.h"
#include "g2d_uapi_process.h"
#include "g2d_command.h"
#include "g2d_fence.h"
#include "g2d_regs.h"
#include "g2d_debug.h"

unsigned int get_layer_payload(struct g2d_layer *layer)
{
	unsigned int payload = 0;
	unsigned int i;

	for (i = 0; i < layer->num_buffers; i++)
		payload += layer->buffer[i].payload;

	return payload;
}

static void g2d_clean_caches_layer(struct device *dev, struct g2d_layer *layer,
				   enum dma_data_direction dir)
{
	unsigned int i;

	if (device_get_dma_attr(dev) != DEV_DMA_COHERENT) {
		for (i = 0; i < layer->num_buffers; i++) {
			if (layer->buffer_type == G2D_BUFTYPE_DMABUF) {
				dma_sync_sg_for_device(dev,
					layer->buffer[i].dmabuf.sgt->sgl,
					layer->buffer[i].dmabuf.sgt->orig_nents,
					dir);
			} else {
				exynos_iommu_sync_for_device(dev,
						layer->buffer[i].dma_addr,
						layer->buffer[i].payload, dir);
			}
		}

		return;
	}

	/*
	 * Cache invalidation is required if a cacheable buffer is possibly
	 * written nonshareably and G2D is to read the buffer shareably.
	 */
	for (i = 0; i < layer->num_buffers; i++) {
		struct dma_buf *dmabuf;

		dmabuf = layer->buffer[i].dmabuf.dmabuf;
		if ((layer->buffer_type == G2D_BUFTYPE_DMABUF) &&
				ion_cached_dmabuf(dmabuf) &&
					ion_hwrender_dmabuf(dmabuf)) {
				dma_sync_sg_for_cpu(dev,
					layer->buffer[i].dmabuf.sgt->sgl,
					layer->buffer[i].dmabuf.sgt->orig_nents,
					DMA_FROM_DEVICE);
		}
	}
}

static void g2d_clean_caches_task(struct g2d_task *task)
{
	struct device *dev = task->g2d_dev->dev;
	unsigned int i;

	if ((device_get_dma_attr(dev) == DEV_DMA_COHERENT) &&
				(task->total_hwrender_len == 0))
		return;

	for (i = 0; i < task->num_source; i++)
		g2d_clean_caches_layer(dev, &task->source[i], DMA_TO_DEVICE);

	g2d_clean_caches_layer(dev, &task->target, DMA_FROM_DEVICE);

}

static void g2d_invalidate_caches_task(struct g2d_task *task)
{
	struct device *dev = task->g2d_dev->dev;
	unsigned int i;

	if (device_get_dma_attr(dev) == DEV_DMA_COHERENT)
		return;

	for (i = 0; i < task->target.num_buffers; i++) {
		struct g2d_buffer *buffer;

		buffer = &task->target.buffer[i];
		if (task->target.buffer_type == G2D_BUFTYPE_DMABUF) {
			dma_sync_sg_for_cpu(dev, buffer->dmabuf.sgt->sgl,
					    buffer->dmabuf.sgt->orig_nents,
					    DMA_FROM_DEVICE);
		} else {
			exynos_iommu_sync_for_cpu(dev, buffer->dma_addr,
				buffer->payload, DMA_FROM_DEVICE);
		}
	}
}

#define buferr_show(dev, i, payload, w, h, b, name, len)		       \
perrfndev(dev,								       \
	  "Too small buffer[%d]: expected %u for %ux%u(bt%u)/%s but %u given", \
	  i, payload,w, h, b, name, len)

static int g2d_prepare_buffer(struct g2d_device *g2d_dev,
			      struct g2d_layer *layer,
			      struct g2d_layer_data *data)
{
	const struct g2d_fmt *fmt = NULL;
	struct g2d_reg *cmd = layer->commands;
	unsigned int payload;
	int i;

	fmt = g2d_find_format(cmd[G2DSFR_IMG_COLORMODE].value, g2d_dev->caps);

	BUG_ON(!fmt);

	if (data->num_buffers == 0) {
		perrfndev(g2d_dev, "Invalid number of buffer %u for %s",
			  data->num_buffers, fmt->name);
		return -EINVAL;
	}

	if ((data->num_buffers > 1) && (data->num_buffers != fmt->num_planes)) {
		/* NV12 8+2 in two buffers is valid */
		if ((fmt->num_planes != 4) || (data->num_buffers != 2)) {
			perrfndev(g2d_dev, "Invalid number of buffer %u for %s",
				  data->num_buffers, fmt->name);
			return -EINVAL;
		}
	}

	if (data->num_buffers > 1) {
		for (i = 0; i < data->num_buffers; i++) {
			payload = (unsigned int)g2d_get_payload_index(
						cmd, fmt, i, data->num_buffers,
						g2d_dev->caps);
			if (data->buffer[i].length < payload) {
				buferr_show(g2d_dev, i, payload,
					    cmd[G2DSFR_IMG_WIDTH].value,
					    cmd[G2DSFR_IMG_HEIGHT].value,
					    cmd[G2DSFR_IMG_BOTTOM].value,
					    fmt->name, data->buffer[i].length);
				return -EINVAL;
			}

			layer->buffer[i].payload = payload;
		}
	} else {
		payload = (unsigned int)g2d_get_payload(cmd, fmt, layer->flags,
							g2d_dev->caps);
		if (data->buffer[0].length < payload) {
			buferr_show(g2d_dev, 0, payload,
				    cmd[G2DSFR_IMG_WIDTH].value,
				    cmd[G2DSFR_IMG_HEIGHT].value,
				    cmd[G2DSFR_IMG_BOTTOM].value,
				    fmt->name, data->buffer[0].length);
			return -EINVAL;
		}

		layer->buffer[0].payload = payload;
	}

	layer->num_buffers = data->num_buffers;

	return 0;
}

static int g2d_get_dmabuf(struct g2d_task *task,
			  struct g2d_context *ctx,
			  struct g2d_buffer *buffer,
			  struct g2d_buffer_data *data,
			  enum dma_data_direction dir)
{
	struct g2d_device *g2d_dev = task->g2d_dev;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	int ret = -EINVAL;
	int prot = IOMMU_READ;

	if (!IS_HWFC(task->flags) || (dir == DMA_TO_DEVICE)) {
		dmabuf = dma_buf_get(data->dmabuf.fd);
		if (IS_ERR(dmabuf)) {
			perrfndev(g2d_dev, "Failed to get dmabuf from fd %d",
				  data->dmabuf.fd);
			return PTR_ERR(dmabuf);
		}
	} else {
		dmabuf = ctx->hwfc_info->bufs[task->sec.job_id];
		get_dma_buf(dmabuf);
	}

	if (dmabuf->size < data->dmabuf.offset) {
		perrfndev(g2d_dev, "too large offset %u for dmabuf of %zu",
			  data->dmabuf.offset, dmabuf->size);
		goto err;
	}

	if ((dmabuf->size - data->dmabuf.offset) < buffer->payload) {
		perrfndev(g2d_dev, "too small dmabuf %zu/%u but reqiured %u",
			  dmabuf->size, data->dmabuf.offset, buffer->payload);
		goto err;
	}

	if (ion_cached_dmabuf(dmabuf)) {
		task->total_cached_len += buffer->payload;

		if ((dir == DMA_TO_DEVICE) && ion_hwrender_dmabuf(dmabuf))
			task->total_hwrender_len += buffer->payload;
	}

	attachment = dma_buf_attach(dmabuf, g2d_dev->dev);
	if (IS_ERR(attachment)) {
		ret = PTR_ERR(attachment);
		perrfndev(g2d_dev, "failed to attach to dmabuf (%d)", ret);
		goto err;
	}

	sgt = dma_buf_map_attachment(attachment, dir);
	if (IS_ERR(sgt)) {
		ret = PTR_ERR(sgt);
		perrfndev(g2d_dev, "failed to map dmabuf (%d)", ret);
		goto err_map;
	}

	if (dir != DMA_TO_DEVICE)
		prot |= IOMMU_WRITE;

	if (device_get_dma_attr(g2d_dev->dev) == DEV_DMA_COHERENT)
		prot |= IOMMU_CACHE;

	dma_addr = ion_iovmm_map(attachment, 0, buffer->payload, dir, prot);
	if (IS_ERR_VALUE(dma_addr)) {
		ret = (int)dma_addr;
		perrfndev(g2d_dev, "failed to iovmm map for dmabuf (%d)", ret);
		goto err_iovmmmap;
	}

	buffer->dmabuf.dmabuf = dmabuf;
	buffer->dmabuf.attachment = attachment;
	buffer->dmabuf.sgt = sgt;
	buffer->dmabuf.offset = data->dmabuf.offset;
	buffer->dma_addr = dma_addr + data->dmabuf.offset;

	return 0;
err_iovmmmap:
	dma_buf_unmap_attachment(attachment, sgt, dir);
err_map:
	dma_buf_detach(dmabuf, attachment);
err:
	dma_buf_put(dmabuf);
	return ret;
}

static int g2d_put_dmabuf(struct g2d_device *g2d_dev, struct g2d_buffer *buffer,
			  enum dma_data_direction dir)
{
	ion_iovmm_unmap(buffer->dmabuf.attachment,
			buffer->dma_addr - buffer->dmabuf.offset);
	dma_buf_unmap_attachment(buffer->dmabuf.attachment,
				 buffer->dmabuf.sgt, dir);
	dma_buf_detach(buffer->dmabuf.dmabuf, buffer->dmabuf.attachment);
	dma_buf_put(buffer->dmabuf.dmabuf);

	memset(buffer, 0, sizeof(*buffer));

	return 0;
}

#define is_vma_cached(vma)						\
	((pgprot_val(pgprot_noncached((vma)->vm_page_prot)) !=		\
			  pgprot_val((vma)->vm_page_prot)) &&		\
	   (pgprot_val(pgprot_writecombine((vma)->vm_page_prot)) !=	\
	    pgprot_val((vma)->vm_page_prot)))

static int g2d_get_userptr(struct g2d_task *task,
			   struct g2d_context *ctx,
			   struct g2d_buffer *buffer,
			   struct g2d_buffer_data *data,
			   enum dma_data_direction dir)
{
	struct g2d_device *g2d_dev = task->g2d_dev;
	unsigned long end = data->userptr + data->length;
	struct vm_area_struct *vma;
	struct vm_area_struct *tvma;
	struct mm_struct *mm;
	int ret = -EINVAL;
	int prot = IOMMU_READ;

	mm = get_task_mm(current);
	if (!mm)
		return ret;

	down_read(&mm->mmap_sem);

	vma = find_vma(mm, data->userptr);
	if (!vma || (data->userptr < vma->vm_start)) {
		perrfndev(g2d_dev, "invalid address %#lx", data->userptr);
		goto err_novma;
	}

	tvma = kmemdup(vma, sizeof(*vma), GFP_KERNEL);
	if (!tvma) {
		ret = -ENOMEM;
		goto err_novma;
	}

	if (dir != DMA_TO_DEVICE)
		prot |= IOMMU_WRITE;
	if (is_vma_cached(vma))
		task->total_cached_len += buffer->payload;

	buffer->userptr.vma = tvma;

	tvma->vm_next = NULL;
	tvma->vm_prev = NULL;
	tvma->vm_mm = mm;

	while (end > vma->vm_end) {
		if (!!(vma->vm_flags & VM_PFNMAP)) {
			perrfndev(g2d_dev, "nonlinear pfnmap is not supported");
			goto err_vma;
		}

		if ((vma->vm_next == NULL) ||
				(vma->vm_end != vma->vm_next->vm_start)) {
			perrfndev(g2d_dev, "invalid size %u", data->length);
			goto err_vma;
		}

		vma = vma->vm_next;
		tvma->vm_next = kmemdup(vma, sizeof(*vma), GFP_KERNEL);
		if (tvma->vm_next == NULL) {
			perrfndev(g2d_dev, "failed to allocate vma");
			ret = -ENOMEM;
			goto err_vma;
		}
		tvma->vm_next->vm_prev = tvma;
		tvma->vm_next->vm_next = NULL;
		tvma = tvma->vm_next;
	}

	for (vma = buffer->userptr.vma; vma != NULL; vma = vma->vm_next) {
		if (vma->vm_file)
			get_file(vma->vm_file);
		if (vma->vm_ops && vma->vm_ops->open)
			vma->vm_ops->open(vma);
	}

	if (device_get_dma_attr(g2d_dev->dev) == DEV_DMA_COHERENT)
		prot |= IOMMU_CACHE;

	buffer->dma_addr = exynos_iovmm_map_userptr(g2d_dev->dev, data->userptr,
						    data->length, prot);
	if (IS_ERR_VALUE(buffer->dma_addr)) {
		ret = (int)buffer->dma_addr;
		perrfndev(g2d_dev, "failed to iovmm map userptr");
		goto err_map;
	}

	up_read(&mm->mmap_sem);

	buffer->userptr.addr = data->userptr;

	return 0;
err_map:
	buffer->dma_addr = 0;

	for (vma = buffer->userptr.vma; vma != NULL; vma = vma->vm_next) {
		if (vma->vm_file)
			fput(vma->vm_file);
		if (vma->vm_ops && vma->vm_ops->close)
			vma->vm_ops->close(vma);
	}
err_vma:
	while (tvma) {
		vma = tvma;
		tvma = tvma->vm_prev;
		kfree(vma);
	}

	buffer->userptr.vma = NULL;
err_novma:
	up_read(&mm->mmap_sem);

	mmput(mm);

	return ret;
}

static int g2d_put_userptr(struct g2d_device *g2d_dev,
			struct g2d_buffer *buffer, enum dma_data_direction dir)
{
	struct vm_area_struct *vma = buffer->userptr.vma;
	struct mm_struct *mm;

	BUG_ON((vma == NULL) || (buffer->userptr.addr == 0));

	mm = vma->vm_mm;

	exynos_iovmm_unmap_userptr(g2d_dev->dev, buffer->dma_addr);

	/*
	 * Calling to vm_ops->close() actually does not need mmap_sem to be
	 * acquired but some device driver needs mmap_sem to be held.
	 */
	down_read(&mm->mmap_sem);

	while (vma) {
		struct vm_area_struct *tvma;

		if (vma->vm_ops && vma->vm_ops->close)
			vma->vm_ops->close(vma);

		if (vma->vm_file)
			fput(vma->vm_file);

		tvma = vma;
		vma = vma->vm_next;

		kfree(tvma);
	}

	up_read(&mm->mmap_sem);

	mmput(mm);

	memset(buffer, 0, sizeof(*buffer));

	return 0;
}

static int g2d_get_buffer(struct g2d_device *g2d_dev,
				struct g2d_context *ctx,
				struct g2d_layer *layer,
				struct g2d_layer_data *data,
				enum dma_data_direction dir)
{
	int ret = 0;
	unsigned int i;
	int (*get_func)(struct g2d_task *, struct g2d_context *,
			struct g2d_buffer *, struct g2d_buffer_data *,
			enum dma_data_direction);
	int (*put_func)(struct g2d_device *, struct g2d_buffer *,
			enum dma_data_direction);

	if (layer->buffer_type == G2D_BUFTYPE_DMABUF) {
		get_func = g2d_get_dmabuf;
		put_func = g2d_put_dmabuf;
	} else if (layer->buffer_type == G2D_BUFTYPE_USERPTR) {
		get_func = g2d_get_userptr;
		put_func = g2d_put_userptr;
	} else {
		BUG();
	}

	for (i = 0; i < layer->num_buffers; i++) {
		ret = get_func(layer->task, ctx, &layer->buffer[i],
			       &data->buffer[i], dir);
		if (ret) {
			while (i-- > 0)
				put_func(g2d_dev, &layer->buffer[i], dir);
			return ret;
		}

		layer->buffer[i].length = data->buffer[i].length;
	}

	return 0;
}

static void g2d_put_buffer(struct g2d_device *g2d_dev,
			u32 buffer_type, struct g2d_buffer buffer[],
			unsigned int num_buffer, enum dma_data_direction dir)
{
	unsigned int i;
	int (*put_func)(struct g2d_device *, struct g2d_buffer *,
			enum dma_data_direction);

	switch (buffer_type) {
	case G2D_BUFTYPE_DMABUF:
		put_func = g2d_put_dmabuf;
		break;
	case G2D_BUFTYPE_USERPTR:
		put_func = g2d_put_userptr;
		break;
	case G2D_BUFTYPE_EMPTY:
		return;
	default:
		BUG();
	}

	for (i = 0; i < num_buffer; i++)
		put_func(g2d_dev, &buffer[i], dir);
}

static void g2d_put_image(struct g2d_device *g2d_dev, struct g2d_layer *layer,
			  enum dma_data_direction dir)
{
	g2d_put_buffer(g2d_dev, layer->buffer_type,
			layer->buffer, layer->num_buffers, dir);

	if (layer->fence)
		dma_fence_remove_callback(layer->fence, &layer->fence_cb);
	dma_fence_put(layer->fence);

	layer->buffer_type = G2D_BUFTYPE_NONE;
}

static int g2d_get_source(struct g2d_device *g2d_dev, struct g2d_task *task,
			  struct g2d_layer *layer, struct g2d_layer_data *data,
			  int index)
{
	int ret;

	layer->flags = data->flags;
	layer->buffer_type = data->buffer_type;
	layer->fence = NULL;

	if (!G2D_BUFTYPE_VALID(layer->buffer_type)) {
		perrfndev(g2d_dev, "invalid buffer type %u specified",
			  layer->buffer_type);
		return -EINVAL;
	}

	/* color fill has no buffer */
	if (layer->buffer_type == G2D_BUFTYPE_EMPTY) {
		if (layer->flags & G2D_LAYERFLAG_COLORFILL) {
			layer->num_buffers = 0;
			layer->flags &= ~G2D_LAYERFLAG_ACQUIRE_FENCE;
			/* g2d_prepare_source() always successes for colofill */
			g2d_prepare_source(task, layer, index);
			return 0;
		}

		perrfndev(g2d_dev, "DMA layer %d has no buffer - flags: %#x",
			  index, layer->flags);
		return -EINVAL;
	}

	if (!g2d_validate_source_commands(
			g2d_dev, task, index, layer, &task->target))
		return -EINVAL;

	ret = g2d_prepare_buffer(g2d_dev, layer, data);
	if (ret)
		return ret;

	layer->fence = g2d_get_acquire_fence(g2d_dev, layer, data->fence);
	if (IS_ERR(layer->fence)) {
		perrfndev(g2d_dev, "Invalid fence fd %d on source[%d]",
			  data->fence, index);
		return PTR_ERR(layer->fence);
	}

	ret = g2d_get_buffer(g2d_dev, NULL, layer, data, DMA_TO_DEVICE);
	if (ret)
		goto err_buffer;

	if (!g2d_prepare_source(task, layer, index)) {
		ret = -EINVAL;
		perrfndev(g2d_dev, "Failed to prepare source layer %d", index);
		goto err_prepare;
	}

	return 0;
err_prepare:
	g2d_put_buffer(g2d_dev, layer->buffer_type, layer->buffer,
		       layer->num_buffers, DMA_TO_DEVICE);
err_buffer:
	if (layer->fence)
		dma_fence_remove_callback(layer->fence, &layer->fence_cb);
	dma_fence_put(layer->fence); /* dma_fence_put() checkes NULL */

	return ret;
}

static int g2d_get_sources(struct g2d_device *g2d_dev, struct g2d_task *task,
			   struct g2d_layer_data __user *src)
{
	unsigned int i;
	int ret;

	for (i = 0; i < task->num_source; i++, src++) {
		struct g2d_layer_data data;

		if (copy_from_user(&data, src, sizeof(*src))) {
			perrfndev(g2d_dev,
				  "Failed to read source image data %d/%d",
				  i, task->num_source);
			ret = -EFAULT;
			break;
		}

		ret = g2d_get_source(g2d_dev, task,
				     &task->source[i], &data, i);
		if (ret)
			break;
	}

	if (i < task->num_source) {
		while (i-- > 0)
			g2d_put_image(g2d_dev, &task->source[i], DMA_TO_DEVICE);
	}

	return ret;
}

static int g2d_get_target(struct g2d_device *g2d_dev, struct g2d_context *ctx,
			  struct g2d_task *task, struct g2d_layer_data *data)
{
	struct g2d_layer *target = &task->target;
	int ret;

	target->flags = data->flags;
	target->fence = NULL;

	target->buffer_type = data->buffer_type;

	if (!G2D_BUFTYPE_VALID(target->buffer_type)) {
		perrfndev(g2d_dev, "invalid buffer type %u specified to target",
			  target->buffer_type);
		return -EINVAL;
	}

	if (IS_HWFC(task->flags)) {
		struct g2d_task *ptask;
		unsigned long flags;

		target->buffer_type = G2D_BUFTYPE_DMABUF;

		/*
		 * The index from repeater driver used on both buffer index and
		 * job id, and this index is managed by repeater driver to
		 * avoid overwriting the buffer index and job id while MFC is
		 * running.
		 */
		ret = hwfc_get_valid_buffer(&task->bufidx);
		if (ret < 0) {
			perrfndev(g2d_dev, "Failed to get buffer for HWFC");
			return ret;
		}

		BUG_ON(task->bufidx >= ctx->hwfc_info->buffer_count);

		spin_lock_irqsave(&task->g2d_dev->lock_task, flags);

		ptask = task->g2d_dev->tasks;

		while (ptask != NULL) {
			if (ptask == task) {
				ptask = ptask->next;
				continue;
			}
			if ((ptask->sec.job_id == task->bufidx) &&
					!is_task_state_idle(ptask)) {
				perrfndev(g2d_dev, "The %d task is not idle",
					  task->bufidx);

				spin_unlock_irqrestore(
					&task->g2d_dev->lock_task, flags);

				return -EINVAL;
			}
			ptask = ptask->next;
		}

		g2d_stamp_task(task, G2D_STAMP_STATE_HWFCBUF, task->bufidx);

		task->sec.job_id = task->bufidx;

		spin_unlock_irqrestore(&task->g2d_dev->lock_task, flags);

		data->num_buffers = 1;
		data->buffer[0].dmabuf.offset = 0;
		data->buffer[0].length =
			(__u32)ctx->hwfc_info->bufs[task->bufidx]->size;
	}

	if (target->buffer_type == G2D_BUFTYPE_EMPTY) {
		perrfndev(g2d_dev, "target has no buffer - flags: %#x",
			  task->flags);
		return -EINVAL;
	}

	if (!g2d_validate_target_commands(g2d_dev, task))
		return -EINVAL;

	ret = g2d_prepare_buffer(g2d_dev, target, data);
	if (ret)
		return ret;

	target->fence = g2d_get_acquire_fence(g2d_dev, target, data->fence);
	if (IS_ERR(target->fence)) {
		perrfndev(g2d_dev, "Invalid taret fence fd %d", data->fence);
		return PTR_ERR(target->fence);
	}

	ret = g2d_get_buffer(g2d_dev, ctx, target, data, DMA_FROM_DEVICE);
	if (ret)
		goto err_buffer;

	if (!g2d_prepare_target(task)) {
		ret = -EINVAL;
		perrfndev(g2d_dev, "Failed to prepare target layer");
		goto err_prepare;
	}

	return 0;
err_prepare:
	g2d_put_buffer(g2d_dev, target->buffer_type, target->buffer,
				target->num_buffers, DMA_FROM_DEVICE);
err_buffer:
	if (target->fence)
		dma_fence_remove_callback(target->fence, &target->fence_cb);
	dma_fence_put(target->fence); /* dma_fence_put() checkes NULL */

	return ret;
}

int g2d_get_userdata(struct g2d_device *g2d_dev, struct g2d_context *ctx,
		     struct g2d_task *task, struct g2d_task_data *data)
{
	unsigned int i;
	int ret;

	/* invalid range check */
	if ((data->num_source < 1) || (data->num_source > g2d_dev->max_layers)) {
		perrfndev(g2d_dev, "Invalid number of source images %u",
			  data->num_source);
		return -EINVAL;
	}

	if (data->priority > G2D_MAX_PRIORITY) {
		perrfndev(g2d_dev, "Invalid number of priority %u",
			  data->priority);
		return -EINVAL;
	}

	task->flags = data->flags;
	task->num_source = data->num_source;
	task->total_cached_len = 0;
	task->total_hwrender_len = 0;

	ret = g2d_import_commands(g2d_dev, task, data, task->num_source);
	if (ret < 0)
		return ret;

	ret = g2d_get_target(g2d_dev, ctx, task, &data->target);
	if (ret)
		return ret;

	ret = g2d_get_sources(g2d_dev, task, data->source);
	if (ret)
		goto err_src;

	task->release_fence = g2d_create_release_fence(g2d_dev, task, data);
	if (IS_ERR(task->release_fence)) {
		ret = PTR_ERR(task->release_fence);
		goto err_fence;
	}

	g2d_clean_caches_task(task);

	return 0;
err_fence:
	for (i = 0; i < task->num_source; i++)
		g2d_put_image(g2d_dev, &task->source[i], DMA_TO_DEVICE);
err_src:
	g2d_put_image(g2d_dev, &task->target, DMA_FROM_DEVICE);

	return ret;
}

void g2d_put_images(struct g2d_device *g2d_dev, struct g2d_task *task)
{
	unsigned int i;

	g2d_invalidate_caches_task(task);

	if (task->release_fence) {
		if (is_task_state_error(task))
			dma_fence_set_error(task->release_fence->fence, -EIO);

		dma_fence_signal(task->release_fence->fence);
		fput(task->release_fence->file);
	}

	for (i = 0; i < task->num_source; i++)
		g2d_put_image(g2d_dev, &task->source[i], DMA_TO_DEVICE);

	g2d_put_image(g2d_dev, &task->target, DMA_FROM_DEVICE);
	task->num_source = 0;
}

int g2d_wait_put_user(struct g2d_device *g2d_dev, struct g2d_task *task,
		      struct g2d_task_data __user *uptr, u32 userflag)
{
	int ret;

	if (!g2d_task_wait_completion(task)) {
		userflag |= G2D_FLAG_ERROR;
		ret = put_user(userflag, &uptr->flags);
	} else {
		u32 laptime_in_usec = (u32)ktime_us_delta(task->ktime_end,
						  task->ktime_begin);

		ret = put_user(laptime_in_usec, &uptr->laptime_in_usec);
	}

	g2d_put_images(g2d_dev, task);

	g2d_put_free_task(g2d_dev, task);

	return ret;
}
