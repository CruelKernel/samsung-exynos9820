/* linux/drivers/media/platform/exynos/jsqz-core.c
 *
 * Core file for Samsung Jpeg Squeezer driver
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/ioport.h>
#include <linux/kmod.h>
#include <linux/time.h>
#include <linux/clk.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/exynos_iovmm.h>
#include <asm/page.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/time.h>

#include <linux/exynos_iovmm.h>
#include <linux/ion_exynos.h>
//#include <linux/exynos_ion.h>

#include <linux/pm_runtime.h>

//This holds the enums used to communicate with userspace
#include <linux/hwjsqz.h>

#include "jsqz-core.h"
#include "jsqz-regs.h"
#include "jsqz-helper.h"

//The name of the gate clock. This is set in the device-tree entry
static const char *clk_producer_name = "gate";

static const unsigned int init_q_table[32] = {
	0x01010101, 0x01010101, 0x01010101, 0x03020201,
	0x02020202, 0x03030402, 0x04050302, 0x04050505,
	0x06050404, 0x05050607, 0x04040607, 0x07060906,
	0x08080808, 0x09060508, 0x0a08090a, 0x08080807,
	0x02010101, 0x02040202, 0x05080402, 0x08080504,
	0x08080808, 0x08080808, 0x08080808, 0x08080808,
	0x08080808, 0x08080808, 0x08080808, 0x08080808,
	0x08080808,	0x08080808, 0x08080808, 0x08080808
};

//#define HWJSQZ_PROFILE_ENABLE

#ifdef HWJSQZ_PROFILE_ENABLE
static struct timeval global_time_start;
static struct timeval global_time_end;
#define HWJSQZ_PROFILE(statement, msg, dev) \
	do { \
		/* Use local structs to support nesting HWJSQZ_PROFILE calls */\
		struct timeval time_start; \
		struct timeval time_end; \
		do_gettimeofday(&time_start); \
		statement; \
		do_gettimeofday(&time_end); \
		dev_info(dev, msg ": %ld microsecs\n", \
			1000000 * (time_end.tv_sec - time_start.tv_sec) + \
			(time_end.tv_usec - time_start.tv_usec)); \
	} while (0)
#else
#define HWJSQZ_PROFILE(statement, msg, dev) \
	do { \
		statement; \
	} while (0)
#endif

#define MAX_BUF_NUM	3

/**
 * jsqz_compute_buffer_size:
 *
 * Compute the size that the buffer(s) must have to
 * be able to respect that format. The result is saved in the variable
 * pointed to by bytes_used.
 *
 * The dma_data_direction is used to distinguish between output and capture
 * (in/out) buffers.
 *
 * srcBitsPerPixel is only used to compute the size of the buffer that
 * holds the source image we were asked to process.
 *
 * Returns 0 on success, otherwise < 0 err num
 */
static int jsqz_compute_buffer_size(struct jsqz_ctx *ctx,
					struct jsqz_task *task,
					enum dma_data_direction dir,
					size_t *bytes_used,
					int buf_index)
{
	if (!ctx) {
		pr_err("%s: invalid context!\n", __func__);
		return -EINVAL;
	}
	dev_dbg(ctx->jsqz_dev->dev, "%s: BEGIN\n", __func__);
	if (!task) {
		dev_err(ctx->jsqz_dev->dev
			, "%s: invalid task ptr!\n", __func__);
		return -EINVAL;
	}
	if (!bytes_used) {
		dev_err(ctx->jsqz_dev->dev
			, "%s: invalid ptr to requested buffer size!\n"
			, __func__);
		return -EINVAL;
	}


	//handling the buffer holding the encoding result
	if (dir == DMA_FROM_DEVICE) {
		*bytes_used = 32;
		dev_dbg(ctx->jsqz_dev->dev
			, "%s: DMA_FROM_DEVICE, requesting %zu bytes\n"
			, __func__, *bytes_used);
	}
	else if (dir == DMA_TO_DEVICE) {
		unsigned int y_width, y_height, c_width, c_height;
		struct hwJSQZ_img_info *img_info = &task->user_task.info_out;

		dev_dbg(ctx->jsqz_dev->dev
			, "%s: DMA_TO_DEVICE case, width %u height %u\n"
			, __func__, img_info->width, img_info->height);

		y_width = img_info->width;
		y_height = img_info->height;
		c_width = (y_width + 1) >> 1 << 1;
		if (img_info->cs)
			c_height = y_height;
		else
			c_height = (y_height + 1) >> 1;

		if (task->user_task.num_of_buf == 1)
			*bytes_used = (y_width * y_height) + (c_width * c_height);
		else if (buf_index)
			*bytes_used = c_width * c_height;
		else
			*bytes_used = y_width * y_height;

		dev_dbg(ctx->jsqz_dev->dev
			, "%s: DMA_TO_DEVICE, requesting %zu bytes\n"
			, __func__, *bytes_used);

	} else {
		//this shouldn't happen
		dev_err(ctx->jsqz_dev->dev,
			"%s: invalid DMA direction\n",
			__func__);
		return -EINVAL;
	}

	dev_dbg(ctx->jsqz_dev->dev, "%s: END\n", __func__);

	return 0;
}


/**
 * jsqz_buffer_map:
 *
 * Finalize the necessary preparation to make the buffer
 * accessible to HW, e.g. dma attachment setup and iova mapping
 * (by using IOVMM API, which will use IOMMU).
 * The buffer will be made available to H/W as a contiguous chunk
 * of virtual memory which, thanks to the IOMMU, can be used for
 * DMA transfers even though it the mapping might refer to scattered
 * physical pages.
 *
 * Returns 0 on success, otherwise < 0 err num
 */
static int jsqz_buffer_map(struct jsqz_ctx *ctx,
					struct jsqz_buffer_dma *dma_buffer,
					enum dma_data_direction dir)
{
	int ret = 0;

	dev_dbg(ctx->jsqz_dev->dev, "%s: BEGIN\n", __func__);

	dev_dbg(ctx->jsqz_dev->dev
		, "%s: about to map dma buffer in device address space...\n"
		, __func__);

	//map dma attachment:
	//this is the point where the dma buffer is actually
	//allocated, if it wasn't there before.
	//It also means the calling driver now owns the buffer
	ret = jsqz_map_dma_attachment(ctx->jsqz_dev->dev,
				   &dma_buffer->plane, dir);
	if (ret)
		return ret;

	dev_dbg(ctx->jsqz_dev->dev, "%s: getting dma address of the buffer...\n"
		, __func__);

	//use iommu to map and get the virtual memory address
	ret = jsqz_dma_addr_map(ctx->jsqz_dev->dev, dma_buffer, dir);
	if (ret) {
		dev_info(ctx->jsqz_dev->dev, "%s: mapping FAILED!\n", __func__);
		jsqz_unmap_dma_attachment(ctx->jsqz_dev->dev,
				   &dma_buffer->plane, dir);
		return ret;
	}

	dev_dbg(ctx->jsqz_dev->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * jsqz_buffer_unmap:
 *
 * called after the task is finished to release all references
 * to the buffers. This is called for every buffer that
 * jsqz_buffer_map was previously called for.
 */
static void jsqz_buffer_unmap(struct jsqz_ctx *ctx,
					struct jsqz_buffer_dma *dma_buffer,
					enum dma_data_direction dir)
{
	dev_dbg(ctx->jsqz_dev->dev, "%s: BEGIN\n", __func__);
	dev_dbg(ctx->jsqz_dev->dev, "%s: about to unmap the DMA address\n"
		, __func__);

	//use iommu to unmap the dma address
	jsqz_dma_addr_unmap(ctx->jsqz_dev->dev, dma_buffer);

	dev_dbg(ctx->jsqz_dev->dev, "%s: about to unmap the DMA buffer\n"
		, __func__);

	//unmap dma attachment, i.e. unmap the buffer so others can request it
	jsqz_unmap_dma_attachment(ctx->jsqz_dev->dev,
			   &dma_buffer->plane, dir);

	//hwjsqz_task will dma detach and decrease the refcount on the dma fd

	dev_dbg(ctx->jsqz_dev->dev, "%s: END\n", __func__);

}

/**
 * Use the Runtime Power Management framework to request device power on/off.
 * This will cause the Runtime PM handlers to be called when appropriate (i.e.
 * when the device is asleep and needs to be woken up or nothing is using it
 * and can be suspended).
 *
 * Returns 0 on success, < 0 err num otherwise.
 */
static int enable_jsqz(struct jsqz_dev *jsqz)
{
	int ret;

	dev_dbg(jsqz->dev, "%s: runtime pm, getting device\n", __func__);

	//we're assuming our runtime_resume callback will enable the clock
	ret = in_irq() ? pm_runtime_get(jsqz->dev) :
			 pm_runtime_get_sync(jsqz->dev);
	if (ret < 0) {
		dev_err(jsqz->dev
			, "%s: failed to increment Power Mgmt usage cnt (%d)\n"
			, __func__, ret);
		return ret;
	}

	//No need to enable the clock, our runtime pm callback will do it

	return 0;
}

static int disable_jsqz(struct jsqz_dev *jsqz)
{
	int ret;

	dev_dbg(jsqz->dev
		, "%s: disabling clock, decrementing runtime pm usage\n"
		, __func__);

	//No need to disable the clock, our runtime pm callback will do it

	//we're assuming our runtime_suspend callback will disable the clock
	if (in_irq()) {
		dev_dbg(jsqz->dev
			, "%s: IN IRQ! DELAYED autosuspend\n" , __func__);
		ret = pm_runtime_put_autosuspend(jsqz->dev);
	} else {
		dev_dbg(jsqz->dev
			, "%s: SYNC autosuspend\n" , __func__);
		ret = pm_runtime_put_sync_autosuspend(jsqz->dev);
	}
	//ret = in_irq() ? pm_runtime_put_autosuspend(jsqz->dev) :
	//		 pm_runtime_put_sync_autosuspend(jsqz->dev);

	if (ret < 0) {
		dev_err(jsqz->dev
			, "%s: failed to decrement Power Mgmt usage cnt (%d)\n"
			, __func__, ret);
		return ret;
	}
	return 0;
}


/**
 * jsqz_set_source_n_config()
 * @jsqz_dev: the jsqzc device struct
 * @task: the task that is about to be executed
 *
 * Set the H/W register representing the DMA address of the source buffer and configure.
 */
static int jsqz_set_source_n_config(struct jsqz_dev *jsqz_device,
			   struct jsqz_task *task)
{
	u32 cs, width, height, stride, format, mode, func, time_out;
	dma_addr_t y_addr = 0;
	dma_addr_t u_addr = 0;
	dma_addr_t v_addr = 0;

	if (!jsqz_device) {
		pr_err("%s: invalid jsqz device!\n", __func__);
		return -EINVAL;
	}
	if (!task) {
		dev_err(jsqz_device->dev, "%s: invalid task!\n", __func__);
		return -EINVAL;
	}

	y_addr = task->dma_buf_out[0].plane.dma_addr;
	cs = task->user_task.info_out.cs;
	stride = task->user_task.info_out.stride;
	width = task->user_task.info_out.width;
	height = task->user_task.info_out.height;
	mode = task->user_task.config.mode;
	func = task->user_task.config.function;
	time_out = 0;

	jsqz_set_stride_on_n_value(jsqz_device->regs, stride);
	jsqz_on_off_time_out(jsqz_device->regs, time_out);

	if (stride == 0) {
		stride = width;
	}
	if (task->user_task.num_of_buf == 2) {
		if (cs) {
			u_addr = task->dma_buf_out[1].plane.dma_addr;
			v_addr = u_addr + ((width + 1) >> 1 * height);
		}
		else {
			u_addr = v_addr = task->dma_buf_out[1].plane.dma_addr;
		}
	}
	else {
		if (cs) {
			u_addr = y_addr + (stride * height);
			v_addr = u_addr + ((width + 1) >> 1 * height);
		}
		else {
			u_addr = v_addr = y_addr + (stride * height);
		}
	}

	format = 0;

	format  = (width & 0x1fff) | ((height & 0xfff) << 16) | ((cs & 0x1) << 30) | ((mode & 0x1) << 29) | ((func & 0x1) << 28);

	dev_dbg(jsqz_device->dev, "%s: BEGIN setting source\n", __func__);

	/* set source address */
	jsqz_set_input_addr_luma(jsqz_device->regs, y_addr);
	jsqz_set_input_addr_chroma(jsqz_device->regs, u_addr, v_addr);

	jsqz_set_input_format(jsqz_device->regs, format);

	dev_dbg(jsqz_device->dev, "%s: END source set\n", __func__);

	return 0;
}


static int jsqz_run_sqz(struct jsqz_ctx *jsqz_context,
			   struct jsqz_task *task)
{
	struct jsqz_dev *jsqz_device = jsqz_context->jsqz_dev;
	int ret = 0;
	unsigned long flags = 0;

	if (!jsqz_device) {
		pr_err("jsqz is null\n");
		return -EINVAL;
	}

	dev_dbg(jsqz_device->dev, "%s: BEGIN\n", __func__);

	spin_lock_irqsave(&jsqz_device->slock, flags);
	if (test_bit(DEV_SUSPEND, &jsqz_device->state)) {
		spin_unlock_irqrestore(&jsqz_device->slock, flags);

		dev_err(jsqz_device->dev,
			"%s: unexpected suspended device state right before running a task\n", __func__);
		return -1;
	}
	spin_unlock_irqrestore(&jsqz_device->slock, flags);


	dev_dbg(jsqz_device->dev, "%s: resetting jsqz device\n", __func__);

	/* H/W initialization: this will reset all registers to reset values */
	jsqz_sw_reset(jsqz_device->regs);

	dev_dbg(jsqz_device->dev, "%s: setting source addresses\n", __func__);

	/* setting for source */
	ret = jsqz_set_source_n_config(jsqz_device, task);
	if (ret) {
		dev_err(jsqz_device->dev, "%s: error setting source\n", __func__);
		return ret;
	}

	dev_dbg(jsqz_device->dev, "%s: enabling interrupts\n", __func__);
	jsqz_interrupt_enable(jsqz_device->regs);

#ifdef HWJSQZ_PROFILE_ENABLE
	do_gettimeofday(&global_time_start);
#endif

	/* run H/W */
	jsqz_hw_start(jsqz_device->regs);

	dev_dbg(jsqz_device->dev, "%s: END\n", __func__);

	return 0;
}


void jsqz_task_schedule(struct jsqz_task *task)
{
	struct jsqz_dev *jsqz_device;
	unsigned long flags;
	int ret = 0;
	int i = 0;

	jsqz_device = task->ctx->jsqz_dev;

	dev_dbg(jsqz_device->dev, "%s: BEGIN\n", __func__);

	if (test_bit(DEV_RUN, &jsqz_device->state)) {
		dev_dbg(jsqz_device->dev
			 , "%s: unexpected state (ctx %p task %p)\n"
			 , __func__, task->ctx, task);
	}

	while (true) {
		spin_lock_irqsave(&jsqz_device->slock, flags);
		if (!test_bit(DEV_SUSPEND, &jsqz_device->state)) {
			set_bit(DEV_RUN, &jsqz_device->state);
			dev_dbg(jsqz_device->dev,
				"%s: HW is active, continue.\n", __func__);
			spin_unlock_irqrestore(&jsqz_device->slock, flags);
			break;
		}
		spin_unlock_irqrestore(&jsqz_device->slock, flags);

		dev_dbg(jsqz_device->dev,
			"%s: waiting on system resume...\n", __func__);

		wait_event(jsqz_device->suspend_wait,
			   !test_bit(DEV_SUSPEND, &jsqz_device->state));
	}

	ret = enable_jsqz(jsqz_device);
	if (ret) {
		task->state = JSQZ_BUFSTATE_RETRY;
		goto err;
	}

/*
	if (device_get_dma_attr(jsqz_device->dev) == DEV_DMA_NON_COHERENT) {
		if (task->dma_buf_out.plane.dmabuf)
			exynos_ion_sync_dmabuf_for_device(jsqz_device->dev,
				task->dma_buf_out.plane.dmabuf, task->dma_buf_out.plane.bytes_used, DMA_TO_DEVICE);
	}
*/

	spin_lock_irqsave(&jsqz_device->lock_task, flags);
	jsqz_device->current_task = task;
	spin_unlock_irqrestore(&jsqz_device->lock_task, flags);

	if (jsqz_run_sqz(task->ctx, task)) {
		task->state = JSQZ_BUFSTATE_ERROR;

		spin_lock_irqsave(&jsqz_device->lock_task, flags);
		jsqz_device->current_task = NULL;
		spin_unlock_irqrestore(&jsqz_device->lock_task, flags);
	}
	else {
		dev_dbg(jsqz_device->dev, "%s: wait completion\n", __func__);
		wait_for_completion(&task->complete);
#ifdef DEBUG
		jsqz_print_all_regs(jsqz_device);
#endif
		if (jsqz_get_bug_config(jsqz_device->regs) & 0x10000000) {
			for (i = 0; i < 32; i++) {
				task->user_task.buf_q[i] = init_q_table[i];
			}
			dev_info(jsqz_device->dev, "%s: jsqz device time out!!\n", __func__);
			jsqz_print_all_regs(jsqz_device);
		}
		else {
			jsqz_get_output_regs(jsqz_device->regs, task->user_task.buf_q);
		}
		//copy_to_user(task->user_task.buf_q, output_qt, 32);
	}

	disable_jsqz(jsqz_device);

err:
	clear_bit(DEV_RUN, &jsqz_device->state);
	if (test_bit(DEV_SUSPEND, &jsqz_device->state)) {
		wake_up(&jsqz_device->suspend_wait);
	}
	dev_dbg(jsqz_device->dev, "%s: END\n", __func__);
	return;
}

static void jsqz_task_schedule_work(struct work_struct *work)
{
	jsqz_task_schedule(container_of(work, struct jsqz_task, work));
}


static int jsqz_task_signal_completion(struct jsqz_dev *jsqz_device,
					   struct jsqz_task *task, bool success)
{
	unsigned long flags;

	//dereferencing the ptr will produce an oops and the program will be
	//killed, but at least we don't crash the kernel using BUG_ON
	if (!jsqz_device) {
		pr_err("HWJSQZ: NULL jsqz device!\n");
		return -EINVAL;
	}
	if (!task) {
		dev_err(jsqz_device->dev
			, "%s; finishing a NULL task!\n", __func__);
		return -EINVAL;
	}

	spin_lock_irqsave(&jsqz_device->lock_task, flags);
	dev_dbg(jsqz_device->dev
		, "%s: resetting current task var...\n", __func__);

	if (!jsqz_device->current_task) {
		dev_warn(jsqz_device->dev
			 , "%s: invalid current task!\n", __func__);
	}
	if (jsqz_device->current_task != task) {
		dev_warn(jsqz_device->dev
			 , "%s: finishing an unexpected task!\n", __func__);
	}

	jsqz_device->current_task = NULL;
	spin_unlock_irqrestore(&jsqz_device->lock_task, flags);

	task->state = success ? JSQZ_BUFSTATE_DONE : JSQZ_BUFSTATE_ERROR;

	dev_dbg(jsqz_device->dev
		, "%s: signalling task completion...\n", __func__);

	//wake up the task processing function and let it return
	complete(&task->complete);

	return 0;
}

/*
static void jsqz_task_cancel(struct jsqz_dev *jsqz_device,
				 struct jsqz_task *task,
				 enum jsqz_state reason)
{
	unsigned long flags;

	spin_lock_irqsave(&jsqz_device->lock_task, flags);
	dev_dbg(jsqz_device->dev
		, "%s: resetting current task var...\n", __func__);

	jsqz_device->current_task = NULL;
	spin_unlock_irqrestore(&jsqz_device->lock_task, flags);

	task->state = reason;

	dev_dbg(jsqz_device->dev
		, "%s: signalling task completion...\n", __func__);

	complete(&task->complete);

	dev_dbg(jsqz_device->dev, "%s: scheduling a new task...\n", __func__);

	jsqz_task_schedule(task);
}
*/

/**
 * Find the VMA that holds the input virtual address, and check
 * if it's associated to a fd representing a DMA buffer.
 * If it is, acquire that dma buffer (i.e. increase refcount on its fd).
 */
static struct dma_buf *jsqz_get_dmabuf_from_userptr(
		struct jsqz_dev *jsqz_device, unsigned long start, size_t len,
		off_t *out_offset)
{
	struct dma_buf *dmabuf = NULL;
#if 0
	struct vm_area_struct *vma;

	dev_dbg(jsqz_device->dev, "%s: BEGIN\n", __func__);

	down_read(&current->mm->mmap_sem);
	dev_dbg(jsqz_device->dev
		, "%s: virtual address space semaphore acquired\n", __func__);

	vma = find_vma(current->mm, start);

	if (!vma || (start < vma->vm_start)) {
		dev_err(jsqz_device->dev
			, "%s: Incorrect user buffer @ %#lx/%#zx\n"
			, __func__, start, len);
		dmabuf = ERR_PTR(-EINVAL);
		goto finish;
	}

	dev_dbg(jsqz_device->dev
		, "%s: VMA found, %p. Starting at %#lx within parent area\n"
		, __func__, vma, vma->vm_start);

	//vm_file is the file that this vma is a memory mapping of
	if (!vma->vm_file) {
		dev_dbg(jsqz_device->dev, "%s: this VMA does not map a file\n",
			__func__);
		goto finish;
	}

	/* get_dma_buf_file is not support now */
	//dmabuf = get_dma_buf_file(vma->vm_file);
	dev_dbg(jsqz_device->dev
		, "%s: got dmabuf %p of vm_file %p\n", __func__, dmabuf
		, vma->vm_file);

	if (dmabuf != NULL)
		*out_offset = start - vma->vm_start;
finish:
	up_read(&current->mm->mmap_sem);
	dev_dbg(jsqz_device->dev
		, "%s: virtual address space semaphore released\n", __func__);
#endif
	return dmabuf;
}

/**
 * If the buffer we received from userspace is an fd representing a DMA buffer,
 * (HWSQZ_BUFFER_DMABUF) then we attach to it.
 *
 * If it's a pointer to user memory (HWSQZ_BUFFER_USERPTR), then we check if
 * that memory is associated to a dmabuf. If it is, we attach to it, thus
 * reusing that dmabuf.
 *
 * If it's just a pointer to user memory with no dmabuf associated to it, we
 * don't need to do anything here. We will use IOVMM later to get access to it.
 */
static int jsqz_buffer_get_and_attach(struct jsqz_dev *jsqz_device,
					  struct hwSQZ_buffer *buffer,
					  struct jsqz_buffer_dma *dma_buffer)
{
	struct jsqz_buffer_plane_dma *plane;
	int ret = 0; //PTR_ERR returns a long
	off_t offset = 0;

	dev_dbg(jsqz_device->dev
		, "%s: BEGIN, acquiring plane from buf %p into dma buffer %p\n"
		, __func__, buffer, dma_buffer);

	plane = &dma_buffer->plane;

	if (buffer->type == HWSQZ_BUFFER_DMABUF) {
		plane->dmabuf = dma_buf_get(buffer->fd);

		dev_dbg(jsqz_device->dev, "%s: dmabuf of fd %d is %p\n", __func__
			, buffer->fd, plane->dmabuf);
	} else if (buffer->type == HWSQZ_BUFFER_USERPTR) {
		// Check if there's already a dmabuf associated to the
		// chunk of user memory our client is pointing us to
		plane->dmabuf = jsqz_get_dmabuf_from_userptr(jsqz_device,
								 buffer->userptr,
								 buffer->len,
								 &offset);
		dev_dbg(jsqz_device->dev, "%s: dmabuf of userptr %p is %p\n"
			, __func__, (void *) buffer->userptr, plane->dmabuf);
	}

	if (!plane->dmabuf) {
		ret = -EINVAL;
		dev_err(jsqz_device->dev,
			"%s: failed to get dmabuf, err %d\n", __func__, ret);
		goto err;
	}

	if (IS_ERR(plane->dmabuf)) {
		ret = PTR_ERR(plane->dmabuf);
		dev_err(jsqz_device->dev,
			"%s: failed to get dmabuf, err %d\n", __func__, ret);
		goto err;
	}

	// this is NULL when the buffer is of type HWSQZ_BUFFER_USERPTR
	// but the memory it points to is not associated to a dmabuf
	if (plane->dmabuf) {
		if (plane->dmabuf->size < plane->bytes_used) {
			dev_err(jsqz_device->dev,
				"%s: driver requires %zu bytes but user gave %zu\n",
				__func__, plane->bytes_used,
				plane->dmabuf->size);
			ret = -EINVAL;
			goto err;
		}

		plane->attachment = dma_buf_attach(plane->dmabuf
						   , jsqz_device->dev);

		dev_dbg(jsqz_device->dev
			, "%s: attachment of dmabuf %p is %p\n", __func__
			, plane->dmabuf, plane->attachment);

		plane->offset = offset;

		if (IS_ERR(plane->attachment)) {
			dev_err(jsqz_device->dev,
				"%s: Failed to attach dmabuf\n", __func__);
			ret = PTR_ERR(plane->attachment);
			goto err;
		}
	}

	dev_dbg(jsqz_device->dev, "%s: END\n", __func__);

	return 0;
err:
	dev_dbg(jsqz_device->dev
		, "%s: ERROR releasing dma resources\n", __func__);

	if ((plane->dmabuf != NULL) && !IS_ERR(plane->dmabuf)) /* release dmabuf */
		dma_buf_put(plane->dmabuf);

	// NOTE: attach was not reached or did not complete,
	//       so no need to detach here

	return ret;
}

/**
 * This should be called after unmapping the dma buffer.
 *
 * We detach the device from the buffer to signal that we're not
 * interested in it anymore, and we use "put" to decrease the usage
 * refcount of the dmabuf struct.
 * Detach is at the end of the function name just to make it easier
 * to associate jsqz_buffer_get_* to jsqz_buffer_put_*, but  it's
 * actually called in the inverse order.
 */
static void jsqz_buffer_put_and_detach(struct jsqz_buffer_dma *dma_buffer)
{
	const struct hwSQZ_buffer *user_buffer = dma_buffer->buffer;
	struct jsqz_buffer_plane_dma *plane = &dma_buffer->plane;

	// - buffer type DMABUF:
	//     in this case dmabuf should always be set
	// - buffer type USERPTR:
	//     in this case dmabuf is only set if we reuse any
	//     preexisting dmabuf (see jsqz_buffer_get_*)
	if (user_buffer->type == HWSQZ_BUFFER_DMABUF
			|| (user_buffer->type == HWSQZ_BUFFER_USERPTR
				&& plane->dmabuf)) {
		dma_buf_detach(plane->dmabuf, plane->attachment);
		dma_buf_put(plane->dmabuf);
		plane->dmabuf = NULL;
	}
}

/**
 * This is the entry point for buffer dismantling.
 *
 * It is used whenever we need to dismantle a buffer that had been fully
 * setup, for instance after we finish processing a task, independently of
 * whether the processing was successful or not.
 *
 * It unmaps the buffer, detaches the device from it, and releases it.
 */
static void jsqz_buffer_teardown(struct jsqz_dev *jsqz_device,
					   struct jsqz_ctx *ctx,
					   struct jsqz_buffer_dma *dma_buffer,
					   enum dma_data_direction dir)
{
	dev_dbg(jsqz_device->dev, "%s: BEGIN\n", __func__);

	jsqz_buffer_unmap(ctx, dma_buffer, dir);

	jsqz_buffer_put_and_detach(dma_buffer);

	dev_dbg(jsqz_device->dev, "%s: END\n", __func__);
}

/**
 * This is the entry point for complete buffer setup.
 *
 * It does the necessary mapping and allocation of the DMA buffer that
 * the HW will access.
 *
 * At the end of this function, the buffer is ready for DMA transfers.
 *
 * Note: this is needed when the input is a HWSQZ_BUFFER_DMABUF as well
 *       as when it is a HWSQZ_BUFFER_USERPTR, because the H/W ultimately
 *       needs a DMA address to operate on.
 */
static int jsqz_buffer_setup(struct jsqz_ctx *ctx,
				 struct hwSQZ_buffer *buffer,
				 struct jsqz_buffer_dma *dma_buffer,
				 enum dma_data_direction dir)
{
	struct jsqz_dev *jsqz_device = ctx->jsqz_dev;
	int ret = 0;
	struct jsqz_buffer_plane_dma *plane = &dma_buffer->plane;

	dev_dbg(jsqz_device->dev, "%s: BEGIN\n", __func__);

	dev_dbg(jsqz_device->dev
		, "%s: computing bytes needed for dma buffer %p\n"
		, __func__, dma_buffer);

	if (plane->bytes_used == 0) {
		dev_err(jsqz_device->dev,
			"%s: BUG! driver expects 0 bytes! (user gave %zu)\n",
			__func__, buffer->len);
		return -EINVAL;

	} else if (buffer->len < plane->bytes_used) {
		dev_err(jsqz_device->dev,
			"%s: driver expects %zu bytes but user gave %zu\n",
			__func__, plane->bytes_used,
			buffer->len);
		return -EINVAL;
	}

	if ((buffer->type != HWSQZ_BUFFER_USERPTR) &&
			(buffer->type != HWSQZ_BUFFER_DMABUF)) {
		dev_err(jsqz_device->dev, "%s: unknown buffer type %u\n",
			__func__, buffer->type);
		return -EINVAL;
	}

	ret = jsqz_buffer_get_and_attach(jsqz_device, buffer, dma_buffer);

	if (ret)
		return ret;

	dma_buffer->buffer = buffer;

	dev_dbg(jsqz_device->dev
		, "%s: about to prepare buffer %p, dir DMA_TO_DEVICE? %d\n"
		, __func__, dma_buffer, dir == DMA_TO_DEVICE);

	/* the callback function should fill 'dma_addr' field */
	ret = jsqz_buffer_map(ctx, dma_buffer, dir);
	if (ret) {
		dev_err(jsqz_device->dev, "%s: Failed to prepare plane"
			, __func__);

		dev_dbg(jsqz_device->dev, "%s: releasing buffer %p\n"
			, __func__, dma_buffer);

		jsqz_buffer_put_and_detach(dma_buffer);

		return ret;
	}

	dev_dbg(jsqz_device->dev, "%s: END\n", __func__);

	return 0;
}


/**
 * Check validity of source/dest formats, i.e. image size and pixel format.
 *
 * dma_data_direction is used to distinguish between the source/dest formats.
 *
 * When the DMA direction is DMA_TO_DEVICE, we also check that the source
 * pixel format is something the H/W can handle, and we save the number of bits
 * per pixel needed to handle that format in the variable referenced by the
 * srcBitsPerPixel pointer.
 */
static int jsqz_validate_format(struct jsqz_ctx *ctx,
				struct jsqz_task *task,
				enum dma_data_direction dir,
				int *srcBitsPerPixel)
{
	struct hwJSQZ_img_info *img_info = NULL;

	if (!ctx) {
		pr_err("%s: invalid context!\n", __func__);
		return -EINVAL;
	}
	if (!task) {
		dev_err(ctx->jsqz_dev->dev
			, "%s: invalid task ptr!\n", __func__);
		return -EINVAL;
	}

	img_info = &task->user_task.info_out;

	//handling the buffer holding the encoding result
	if (dir == DMA_FROM_DEVICE) {
		dev_dbg(ctx->jsqz_dev->dev
			, "%s: DMA_FROM_DEVICE case, width %u height %u\n"
			, __func__, img_info->width, img_info->height);
	} else if (dir == DMA_TO_DEVICE) {
		dev_dbg(ctx->jsqz_dev->dev
			, "%s: DMA_TO_DEVICE case, width %u height %u\n"
			, __func__, img_info->width, img_info->height);
	} else {
		//this shouldn't happen
		dev_err(ctx->jsqz_dev->dev, "%s: invalid DMA direction\n",
			__func__);
		return -EINVAL;
	}

	if ((img_info->width < JSQZ_INPUT_MIN_WIDTH) ||
		(img_info->height < JSQZ_INPUT_MIN_HEIGHT))	{
		dev_err(ctx->jsqz_dev->dev
			, "%s: invalid size of width or height (%u, %u)\n",
			__func__, img_info->width, img_info->height);
		return -EINVAL;
	}

	return 0;
}


/**
 * Validates the image formats (size, pixel formats) provided by userspace and
 * computes the size of the DMA buffers required to be able to process the task.
 *
 * See description of jsqz_compute_buffer_size for more info.
 */
static int jsqz_prepare_formats(struct jsqz_dev *jsqz_device,
				struct jsqz_ctx *ctx,
				struct jsqz_task *task)
{
	int ret = 0;
	int i = 0;
	size_t out_size = 0;
	int srcBitsPerPixel = 0;

	dev_dbg(jsqz_device->dev, "%s: BEGIN\n", __func__);

	dev_dbg(ctx->jsqz_dev->dev, "%s: Validating format...\n", __func__);

	ret = jsqz_validate_format(ctx, task, DMA_TO_DEVICE, &srcBitsPerPixel);
	if (ret < 0)
		return ret;

	for (i = 0; i < task->user_task.num_of_buf; i++)
	{
		ret = jsqz_compute_buffer_size(ctx, task, DMA_TO_DEVICE, &out_size, i);
		if (ret < 0)
			return ret;

		dev_dbg(jsqz_device->dev, "%s: input buffer, plane requires %zu bytes\n"
			, __func__,  out_size);

		task->dma_buf_out[i].plane.bytes_used = out_size;
	}

	dev_dbg(jsqz_device->dev
		, "%s: hardcoding result img height/width to be same as input\n"
		, __func__);

	dev_dbg(jsqz_device->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * Main entry point for preparing a task before it can be processed.
 *
 * Checks the validity of requested formats/sizes and sets the buffers up.
 */
static int jsqz_task_setup(struct jsqz_dev *jsqz_device,
				 struct jsqz_ctx *ctx,
				 struct jsqz_task *task)
{
	int ret, i;

	dev_dbg(jsqz_device->dev, "%s: BEGIN\n", __func__);

	HWJSQZ_PROFILE(ret = jsqz_prepare_formats(jsqz_device, ctx, task),
			   "PREPARE FORMATS TIME", jsqz_device->dev);
	if (ret)
		return ret;

	for (i = 0; i < task->user_task.num_of_buf; i++)
	{
		dev_dbg(jsqz_device->dev, "%s: OUT buf, USER GAVE %zu WE REQUIRE %zu\n"
			, __func__,  task->user_task.buf_out[i].len
			, task->dma_buf_out[i].plane.bytes_used);

		HWJSQZ_PROFILE(ret = jsqz_buffer_setup(ctx, &task->user_task.buf_out[i],
						  &task->dma_buf_out[i], DMA_TO_DEVICE),
				   "OUT BUFFER SETUP TIME", jsqz_device->dev);
		if (ret) {
			dev_err(jsqz_device->dev, "%s: Failed to get output buffer\n",
				__func__);
			if (i > 0) {
				jsqz_buffer_teardown(jsqz_device, ctx,
							 &task->dma_buf_out[i-1], DMA_TO_DEVICE);
				dev_err(jsqz_device->dev, "%s: Failed to get capture buffer\n",
					__func__);
			}
			return ret;
		}
	}

	dev_dbg(jsqz_device->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * Main entry point for task teardown.
 *
 * It takes care of doing all that is needed to teardown the buffers
 * that were used to process the task.
 */
static void jsqz_task_teardown(struct jsqz_dev *jsqz_dev,
				 struct jsqz_ctx *ctx,
				 struct jsqz_task *task)
{
	int i;
	dev_dbg(jsqz_dev->dev, "%s: BEGIN\n", __func__);

	for (i = 0; i < task->user_task.num_of_buf; i++)
	{
		jsqz_buffer_teardown(jsqz_dev, ctx, &task->dma_buf_out[i],
				 DMA_TO_DEVICE);
	}

	dev_dbg(jsqz_dev->dev, "%s: END\n", __func__);
}

static void jsqz_destroy_context(struct kref *kreference)
{
	unsigned long flags;
	//^^ this function is called with ctx->kref as parameter,
	//so we can use ptr arithmetics to go back to ctx
	struct jsqz_ctx *ctx = container_of(kreference,
						struct jsqz_ctx, kref);
	struct jsqz_dev *jsqz_device = ctx->jsqz_dev;

	dev_dbg(jsqz_device->dev, "%s: BEGIN\n", __func__);

	spin_lock_irqsave(&jsqz_device->lock_ctx, flags);
	dev_dbg(jsqz_device->dev, "%s: deleting context %p from list\n"
		, __func__, ctx);
	list_del(&ctx->node);
	spin_unlock_irqrestore(&jsqz_device->lock_ctx, flags);

	kfree(ctx);

	dev_dbg(jsqz_device->dev, "%s: END\n", __func__);
}

static int jsqz_check_image_align(struct jsqz_dev *jsqz_device, struct hwJSQZ_img_info *user_img_info)
{
	unsigned int stride, width;
	unsigned int check_var = 0;

	dev_dbg(jsqz_device->dev, "%s: BEGIN\n", __func__);
	stride = user_img_info->stride;
	width = user_img_info->width;
	if (stride != 0)
		check_var = stride;
	else
		check_var = width;

	if (user_img_info->cs) {		//yuv422
		if (((check_var % 128) == 0) || (((check_var % 64) == 0) && (((check_var / 64) % 4) == 1)))
			return 0;
		else
			return 1;
	}
	else {							//nv21
		if (((check_var % 64) == 0) || (((check_var % 32) == 0) && (((check_var / 32) % 4) == 1)))
			return 0;
		else
			return 1;
	}
	dev_dbg(jsqz_device->dev, "%s: END\n", __func__);
}

/**
 * jsqz_process() - Main entry point for task processing
 * @ctx: context of the task that is about to be processed
 * @task: the task to process
 *
 * Handle all steps needed to process a task, from setup to teardown.
 *
 * First perform the necessary validations and setup of buffers, then
 * wake the H/W up, add the task to the queue, schedule it for execution
 * and wait for the H/W to signal completion.
 *
 * After the @task has been processed (independently of whether it was
 * successful or not), release all the resources and power the H/W down.
 *
 * Return:
 *	 0 on success,
 *	<0 error code on error.
 */
static int jsqz_process(struct jsqz_ctx *ctx,
			struct jsqz_task *task)
{
	struct jsqz_dev *jsqz_device = NULL;
	int ret = 0;
	int num_retries = -1;
	int i = 0;

	if (!ctx)
	{
		pr_err("%s: invalid context!\n", __func__);
		return -EINVAL;
	}

	jsqz_device = ctx->jsqz_dev;
	dev_dbg(jsqz_device->dev, "%s: BEGIN\n", __func__);

	if (jsqz_check_image_align(jsqz_device, &task->user_task.info_out)) {
		for (i = 0; i < 32; i++) {
			task->user_task.buf_q[i] = init_q_table[i];
		}
		return 0;
	}

	init_completion(&task->complete);

	dev_dbg(jsqz_device->dev, "%s: acquiring kref of ctx %p\n",
		__func__, ctx);
	kref_get(&ctx->kref);

	mutex_lock(&ctx->mutex);

	dev_dbg(jsqz_device->dev, "%s: inside context lock (ctx %p task %p)\n"
		, __func__, ctx, task);

	ret = jsqz_task_setup(jsqz_device, ctx, task);

	if (ret)
		goto err_prepare_task;

	task->ctx = ctx;
	task->state = JSQZ_BUFSTATE_READY;

	//INIT_WORK(&task->work, jsqz_task_schedule_work);
	INIT_WORK_ONSTACK(&task->work, jsqz_task_schedule_work);

	do {
		num_retries++;

		if (!queue_work(jsqz_device->workqueue, &task->work)) {
			dev_err(jsqz_device->dev
				, "%s: work was already on a workqueue!\n"
				, __func__);
		}

		/* wait for the job to be completed, in UNINTERRUPTIBLE mode */
		flush_work(&task->work);

	} while (num_retries < 2
		 && task->state == JSQZ_BUFSTATE_RETRY);

	if (task->state == JSQZ_BUFSTATE_READY) {
		dev_err(jsqz_device->dev
			, "%s: invalid task state after task completion\n"
			, __func__);
	}

	HWJSQZ_PROFILE(jsqz_task_teardown(jsqz_device, ctx, task),
			   "TASK TEARDOWN TIME",
			   jsqz_device->dev);

err_prepare_task:
	HWJSQZ_PROFILE(

	dev_dbg(jsqz_device->dev, "%s: releasing lock of ctx %p\n"
		, __func__, ctx);
	mutex_unlock(&ctx->mutex);

	dev_dbg(jsqz_device->dev, "%s: releasing kref of ctx %p\n"
		, __func__, ctx);

	kref_put(&ctx->kref, jsqz_destroy_context);

	dev_dbg(jsqz_device->dev, "%s: returning %d, task state %d\n"
		, __func__, ret, task->state);

	, "FINISH PROCESS", jsqz_device->dev);

	if (ret)
		return ret;
	else
		return (task->state == JSQZ_BUFSTATE_DONE) ? 0 : -EINVAL;
}

/**

 * jsqz_open() - Handle device file open calls
 * @inode: the inode struct representing the device file on disk
 * @filp: the kernel file struct representing the abstract device file
 *
 * Function passed as "open" member of the file_operations struct of
 * the device in order to handle dev node open calls.
 *
 * Return:
 *	0 on success,
 *	-ENOMEM if there is not enough memory to allocate the context
 */
static int jsqz_open(struct inode *inode, struct file *filp)
{

	unsigned long flags;
	//filp->private_data holds jsqz_dev->misc (misc_open sets it)
	//this uses pointers arithmetics to go back to jsqz_dev
	struct jsqz_dev *jsqz_device = container_of(filp->private_data,
							struct jsqz_dev, misc);
	struct jsqz_ctx *ctx;

	dev_dbg(jsqz_device->dev, "%s: BEGIN\n", __func__);
	// GFP_KERNEL is used when kzalloc is allowed to sleep while
	// the memory we're requesting is not immediately available
	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	INIT_LIST_HEAD(&ctx->node);
	kref_init(&ctx->kref);
	mutex_init(&ctx->mutex);

	ctx->jsqz_dev = jsqz_device;

	spin_lock_irqsave(&jsqz_device->lock_ctx, flags);
	dev_dbg(jsqz_device->dev, "%s: adding new context %p to contexts list\n"
		, __func__, ctx);
	list_add_tail(&ctx->node, &jsqz_device->contexts);
	spin_unlock_irqrestore(&jsqz_device->lock_ctx, flags);

	filp->private_data = ctx;

	dev_dbg(jsqz_device->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * jsqz_release() - Handle device file close calls
 * @inode: the inode struct representing the device file on disk
 * @filp: the kernel file struct representing the abstract device file
 *
 * Function passed as "close" member of the file_operations struct of
 * the device in order to handle dev node close calls.
 *
 * Decrease the refcount of the context associated to the given
 * device node.
 */
static int jsqz_release(struct inode *inode, struct file *filp)
{
	struct jsqz_ctx *ctx = filp->private_data;

	dev_dbg(ctx->jsqz_dev->dev, "%s: releasing context %p\n"
		, __func__, ctx);

	//the release callback will not be invoked if counter
	//is already 0 before this line
	kref_put(&ctx->kref, jsqz_destroy_context);

	return 0;
}

/**
 * jsqz_ioctl() - Handle ioctl calls coming from userspace.
  * @filp: the kernel file struct representing the abstract device file
  * @cmd: the ioctl command
  * @arg: the argument passed to the ioctl sys call
  *
  * Receive the task information from userspace, setup and execute the task.
  * Wait for the task to be completed and then return control to userspace.
  *
  * Return:
  *   0 on success,
  *  <0 error codes otherwise
 */
static long jsqz_ioctl(struct file *filp,
			   unsigned int cmd, unsigned long arg)
{
	struct jsqz_ctx *ctx = filp->private_data;
	struct jsqz_dev *jsqz_device = ctx->jsqz_dev;

	dev_dbg(jsqz_device->dev, "%s: BEGIN\n", __func__);

	switch (cmd) {
	case HWJSQZ_IOC_PROCESS:
	{
		struct jsqz_task data;
		int ret;

		dev_dbg(jsqz_device->dev, "%s: PROCESS ioctl received\n"
			, __func__);

		memset(&data, 0, sizeof(data));

		if (copy_from_user(&data.user_task
				   , (void __user *)arg
				   , sizeof(data.user_task))) {
			dev_err(jsqz_device->dev,
				"%s: Failed to read userdata\n", __func__);
			return -EFAULT;
		}

		if ((data.user_task.num_of_buf > MAX_BUF_NUM - 1) || (data.user_task.num_of_buf < 1)) {
			dev_err(jsqz_device->dev,
				"%s: number of buffer is wrong, num_of_buf is %d\n",
				__func__, data.user_task.num_of_buf);
			return -EINVAL;
		}

		dev_dbg(jsqz_device->dev
			, "%s: user data copied, now launching processing...\n"
			, __func__);

		/*
		 * jsqz_process() does not wake up
		 * until the given task finishes
		 */
		HWJSQZ_PROFILE(ret = jsqz_process(ctx, &data),
				   "WHOLE PROCESS TIME",
				   jsqz_device->dev);

		dev_dbg(jsqz_device->dev
			, "%s: processing done! Copying data back to user space...\n", __func__);

		if (copy_to_user((void __user *)arg, &data.user_task, sizeof(data.user_task))) {
			dev_err(jsqz_device->dev,
				"%s: Failed to write userdata\n", __func__);
		}

		if (ret) {
			dev_err(jsqz_device->dev,
				"%s: Failed to process task, error %d\n"
				, __func__, ret);
			return ret;
		}

		return ret;
	}
	default:
		dev_err(jsqz_device->dev, "%s: Unknown ioctl cmd %x, %x\n",
			__func__, cmd, HWJSQZ_IOC_PROCESS);
		return -EINVAL;
	}

	return 0;
}


#ifdef CONFIG_COMPAT

struct compat_hwjsqz_img_info {
	compat_uint_t width;
	compat_uint_t height;
	compat_uint_t stride;
	enum hwJSQZ_input_cs_format cs;
};

struct compat_jsqz_buffer {
	union {
		compat_int_t fd;
		compat_ulong_t userptr;
	};
	compat_size_t len;
	__u8 type;
};

struct compat_hwjsqz_config {
	enum hwJSQZ_processing_mode		mode;
	enum hwJSQZ_processing_function	function;
};


struct compat_hwjsqz_task {
	//What we're giving as input to the device
	struct compat_hwjsqz_img_info info_out;
	struct compat_jsqz_buffer buf_out[2];
	struct compat_hwjsqz_config config;
	compat_uint_t buf_q[32];
	compat_uint_t buf_init_q[32];
	compat_int_t num_of_buf;
};

// jsqz_config struct  only uses elements with fixed width and it
// has explicit padding, so no compat structure should be needed

#define COMPAT_HWJSQZ_IOC_PROCESS	_IOWR('M',   0, struct compat_hwjsqz_task)

static long jsqz_compat_ioctl32(struct file *filp,
				unsigned int cmd, unsigned long arg)
{
	int i = 0;
	struct jsqz_ctx *ctx = filp->private_data;
	struct jsqz_dev *jsqz_device = ctx->jsqz_dev;

	switch (cmd) {
	case COMPAT_HWJSQZ_IOC_PROCESS:
	{
		struct compat_hwjsqz_task data;
		struct jsqz_task task;
		int ret;

		memset(&task, 0, sizeof(task));

		if (copy_from_user(&data, compat_ptr(arg), sizeof(data))) {
			dev_err(jsqz_device->dev,
				"%s: Failed to read userdata\n", __func__);
			return -EFAULT;
		}

		task.user_task.info_out.width = data.info_out.width;
		task.user_task.info_out.height = data.info_out.height;
		task.user_task.info_out.stride = data.info_out.stride;

		task.user_task.config.mode = data.config.mode;
		task.user_task.config.function = data.config.function;

		task.user_task.num_of_buf = data.num_of_buf;
		if (task.user_task.num_of_buf > 0 && task.user_task.num_of_buf < MAX_BUF_NUM) {
			for (i = 0; i < task.user_task.num_of_buf; i++) {
				task.user_task.buf_out[i].len = data.buf_out[i].len;
				if (data.buf_out[i].type == HWSQZ_BUFFER_DMABUF)
					task.user_task.buf_out[i].fd = data.buf_out[i].fd;
				else
					task.user_task.buf_out[i].userptr = data.buf_out[i].userptr;
				task.user_task.buf_out[i].type = data.buf_out[i].type;
			}
		} else {
			dev_err(jsqz_device->dev,
				"%s: number of buffer is wrong, num_of_buf is %d\n",
				__func__, task.user_task.num_of_buf);
			return -EINVAL;
		}

		/*
		 * jsqz_process() does not wake up
		 * until the given task finishes
		 */
		//ret = jsqz_process(ctx, &task);
		HWJSQZ_PROFILE(ret = jsqz_process(ctx, &task),
				   "WHOLE PROCESS TIME compat",
				   jsqz_device->dev);
		if (ret) {
			dev_err(jsqz_device->dev,
				"%s: Failed to process hwjsqz_task task\n",
				__func__);
			return ret;
		}


		dev_dbg(jsqz_device->dev
			, "%s: processing done! Copying data back to user space...\n", __func__);

		for (i = 0; i < 32; i++)
		{
			data.buf_q[i] = task.user_task.buf_q[i];
		}

		if (copy_to_user(compat_ptr(arg), &data, sizeof(data))) {
			dev_err(jsqz_device->dev,
				"%s: Failed to write userdata\n", __func__);
		}

		return 0;
	}
	default:
		dev_err(jsqz_device->dev, "%s: Unknown compat_ioctl cmd %x\n",
			__func__, cmd);
		return -EINVAL;
	}

	return 0;
}
#endif

static const struct file_operations jsqz_fops = {
	.owner          = THIS_MODULE,
	.open           = jsqz_open,
	.release        = jsqz_release,
	.unlocked_ioctl	= jsqz_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= jsqz_compat_ioctl32,
#endif
};

static void jsqz_deregister_device(struct jsqz_dev *jsqz_device)
{
	dev_info(jsqz_device->dev, "%s: deregistering jsqz device\n", __func__);
	misc_deregister(&jsqz_device->misc);
}



/**
 * Acquire and "prepare" the clock producer resource.
 * This is the setup needed before enabling the clock source.
 */
static int jsqz_clk_devm_get_prepare(struct jsqz_dev *jsqz)
{
	struct device *dev = jsqz->dev;
	int ret = 0;

	dev_dbg(jsqz->dev, "%s: acquiring clock with devm\n", __func__);

	jsqz->clk_producer = devm_clk_get(dev, clk_producer_name);

	if (IS_ERR(jsqz->clk_producer)) {
		dev_err(dev, "%s clock is not present\n",
			clk_producer_name);
		return PTR_ERR(jsqz->clk_producer);
	}

	dev_dbg(jsqz->dev, "%s: preparing the clock\n", __func__);

	if (!IS_ERR(jsqz->clk_producer)) {
		ret = clk_prepare(jsqz->clk_producer);
		if (ret) {
			dev_err(jsqz->dev, "%s: clk preparation failed (%d)\n",
				__func__, ret);

			//invalidate clk var so that none else will use it
			jsqz->clk_producer = ERR_PTR(-EINVAL);

			//no need to release anything as devm will take care of
			//if eventually
			return ret;
		}
	}

	return 0;
}

/**
 * Request the system to Enable/Disable the clock source.
 *
 * We cannot read/write to H/W registers or use the device in any way
 * without enabling the clock source first.
 */
static int jsqz_clock_gating(struct jsqz_dev *jsqz, bool on)
{
	int ret = 0;

	if (on) {
		ret = clk_enable(jsqz->clk_producer);
		if (ret) {
			dev_err(jsqz->dev, "%s failed to enable clock\n"
				, __func__);
			return ret;
		}
		dev_dbg(jsqz->dev, "jsqz clock enabled\n");
	} else {
		clk_disable(jsqz->clk_producer);
		dev_dbg(jsqz->dev, "jsqz clock disabled\n");
	}

	return 0;
}


static int jsqz_sysmmu_fault_handler(struct iommu_domain *domain,
					 struct device *dev,
					 unsigned long fault_addr,
					 int fault_flags, void *p)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jsqz_dev *jsqz = platform_get_drvdata(pdev);

	dev_info(jsqz->dev, "%s: sysmmu fault!\n", __func__);

	if (test_bit(DEV_RUN, &jsqz->state)) {
		dev_info(jsqz->dev, "System MMU fault at %#lx\n", fault_addr);
		jsqz_print_all_regs(jsqz);
	}

	return 0;
}

static irqreturn_t jsqz_irq_handler(int irq, void *priv)
{
	unsigned long flags = 0;
	unsigned int int_status;
	struct jsqz_dev *jsqz = priv;
	struct jsqz_task *task;
	bool status = true;

#ifdef HWJSQZ_PROFILE_ENABLE
	do_gettimeofday(&global_time_end);
	dev_info(jsqz->dev, "HW PROCESS TIME: %ld micorsec\n",
		 (global_time_end.tv_sec - global_time_start.tv_sec) +
		 (global_time_end.tv_usec - global_time_start.tv_usec));
#endif

	dev_dbg(jsqz->dev, "%s: BEGIN\n", __func__);

	dev_dbg(jsqz->dev, "%s: getting irq status\n", __func__);

	int_status = jsqz_get_interrupt_status(jsqz->regs);
	dev_dbg(jsqz->dev, "%s: irq status : %x\n", __func__, int_status);
	//Check that the board intentionally triggered the interrupt
	//if (int_status) {
	//	dev_warn(jsqz->dev, "bogus interrupt\n");
	//	return IRQ_NONE;
	//}

	spin_lock_irqsave(&jsqz->slock, flags);
	//HW is done processing
	clear_bit(DEV_RUN, &jsqz->state);
	spin_unlock_irqrestore(&jsqz->slock, flags);

	dev_dbg(jsqz->dev, "%s: clearing irq status\n", __func__);

	jsqz_interrupt_clear(jsqz->regs);

	spin_lock_irqsave(&jsqz->lock_task, flags);
	task = jsqz->current_task;
	if (!task) {
		spin_unlock_irqrestore(&jsqz->lock_task, flags);
		dev_err(jsqz->dev, "received null task in irq handler\n");
		return IRQ_HANDLED;
	}
	spin_unlock_irqrestore(&jsqz->lock_task, flags);

	dev_dbg(jsqz->dev, "%s: finishing task\n", __func__);

	//signal task completion and schedule next task
	if (jsqz_task_signal_completion(jsqz, task, status) < 0) {
		dev_err(jsqz->dev, "%s: error while finishing a task!\n"
			, __func__);

		return IRQ_HANDLED;
	}

	dev_dbg(jsqz->dev, "%s: END\n", __func__);

	return IRQ_HANDLED;
}


/**
 * Initialization routine.
 *
 * This is called at boot time, when the system finds a device that this
 * driver requested to be a handler of (see platform_driver struct below).
 *
 * Set up access to H/W registers, register the device, register the IRQ
 * resource, IOVMM page fault handler, spinlocks, etc.
 */
static int jsqz_probe(struct platform_device *pdev)
{
	struct jsqz_dev *jsqz = NULL;
	struct resource *res = NULL;
	int ret, irq = -1;

	// This replaces the old pre-device-tree dev.platform_data
	// To be populated with data coming from device tree
	jsqz = devm_kzalloc(&pdev->dev, sizeof(struct jsqz_dev), GFP_KERNEL);
	if (!jsqz) {
		//not enough memory
		return -ENOMEM;
	}

	jsqz->dev = &pdev->dev;

	spin_lock_init(&jsqz->slock);

	/* Get the memory resource and map it to kernel space. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	jsqz->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(jsqz->regs)) {
		dev_err(&pdev->dev, "failed to claim/map register region\n");
		return PTR_ERR(jsqz->regs);
	}

	/* Get IRQ resource and register IRQ handler. */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		if (irq != -EPROBE_DEFER)
			dev_err(&pdev->dev, "cannot get irq\n");
		return irq;
	}

	// passing 0 as flags means the flags will be read from DT
	ret = devm_request_irq(&pdev->dev, irq,
				   (void *)jsqz_irq_handler, 0,
				   pdev->name, jsqz);

	if (ret) {
		dev_err(&pdev->dev, "failed to install irq\n");
		return ret;
	}

	jsqz->misc.minor = MISC_DYNAMIC_MINOR;
	jsqz->misc.name = NODE_NAME;
	jsqz->misc.fops = &jsqz_fops;
	jsqz->misc.parent = &pdev->dev;
	ret = misc_register(&jsqz->misc);
	if (ret)
		return ret;

	INIT_LIST_HEAD(&jsqz->contexts);

	spin_lock_init(&jsqz->lock_task);
	spin_lock_init(&jsqz->lock_ctx);

	jsqz->qos_req_level = 533000;

	//jsqz->timeout_jiffies = -1; //msecs_to_jiffies(500);

	/* clock */
	ret = jsqz_clk_devm_get_prepare(jsqz);
	if (ret)
		goto err_clkget;

	platform_set_drvdata(pdev, jsqz);

#if defined(CONFIG_PM_DEVFREQ)
	if (!of_property_read_u32(pdev->dev.of_node, "jsqz,int_qos_minlock",
				(u32 *)&jsqz->qos_req_level)) {
		if (jsqz->qos_req_level > 0) {
			pm_qos_add_request(&jsqz->qos_req,
					   PM_QOS_DEVICE_THROUGHPUT, 0);

			dev_info(&pdev->dev, "INT Min.Lock Freq. = %u\n",
						jsqz->qos_req_level);
		}
	}
#endif
	pm_runtime_set_autosuspend_delay(&pdev->dev, 1000);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	iovmm_set_fault_handler(&pdev->dev,
				jsqz_sysmmu_fault_handler, jsqz);

	ret = iovmm_activate(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s: Failed to activate iommu (%d)\n",
			__func__, ret);
		goto err_iovmm;
	}

	//Power device on to access registers
	ret = pm_runtime_get_sync(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev
			, "%s: failed to pwr on device with runtime pm (%d)\n",
			__func__, ret);
		goto err_pm;
	}

	pm_runtime_put_sync(&pdev->dev);
	jsqz->workqueue = alloc_ordered_workqueue(NODE_NAME, WQ_MEM_RECLAIM);
	if (!jsqz->workqueue) {
		ret = -ENOMEM;
		goto err_pm;
	}

	dev_info(&pdev->dev, "HWJSQZ probe completed successfully.\n");

	return 0;

err_pm:
	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	iovmm_deactivate(&pdev->dev);
err_iovmm:
	if (jsqz->qos_req_level > 0)
		pm_qos_remove_request(&jsqz->qos_req);
	if (!IS_ERR(jsqz->clk_producer)) /* devm will call clk_put */
		clk_unprepare(jsqz->clk_producer);
	platform_set_drvdata(pdev, NULL);
err_clkget:
	jsqz_deregister_device(jsqz);

	dev_err(&pdev->dev, "HWJSQZ probe is failed.\n");

	return ret;
}

//Note: drv->remove() will not be called by the kernel framework because this
//      is a builtin driver. (see builtin_platform_driver at the bottom)
static int jsqz_remove(struct platform_device *pdev)
{
	struct jsqz_dev *jsqz = platform_get_drvdata(pdev);

	dev_dbg(jsqz->dev, "%s: BEGIN removing device\n", __func__);

	if (jsqz->qos_req_level > 0 && pm_qos_request_active(&jsqz->qos_req))
		pm_qos_remove_request(&jsqz->qos_req);

	//From the docs:
	//Drivers in ->remove() callback should undo the runtime PM changes done
	//in ->probe(). Usually this means calling pm_runtime_disable(),
	//pm_runtime_dont_use_autosuspend() etc.
	//disable runtime pm before releasing the clock,
	//or pm runtime could still use it
	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	iovmm_deactivate(&pdev->dev);

	if (!IS_ERR(jsqz->clk_producer)) /* devm will call clk_put */
		clk_unprepare(jsqz->clk_producer);

	platform_set_drvdata(pdev, NULL);

	jsqz_deregister_device(jsqz);

	dev_dbg(jsqz->dev, "%s: END removing device\n", __func__);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int jsqz_suspend(struct device *dev)
{
	struct jsqz_dev *jsqz = dev_get_drvdata(dev);

	dev_dbg(dev, "%s: BEGIN\n", __func__);

	set_bit(DEV_SUSPEND, &jsqz->state);

	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}

static int jsqz_resume(struct device *dev)
{
	struct jsqz_dev *jsqz = dev_get_drvdata(dev);

	dev_dbg(dev, "%s: BEGIN\n", __func__);

	clear_bit(DEV_SUSPEND, &jsqz->state);

	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}
#endif

#ifdef CONFIG_PM
static int jsqz_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jsqz_dev *jsqz = platform_get_drvdata(pdev);
	unsigned long flags = 0;

	dev_dbg(dev, "%s: BEGIN\n", __func__);

	spin_lock_irqsave(&jsqz->slock, flags);
	//According to official runtime PM docs, when using runtime PM
	//autosuspend, it might happen that the autosuspend timer expires,
	//runtime_suspend is entered, and at the same time we receive new job.
	//In that case we return -EBUSY to tell runtime PM core that we're
	//not ready to go in low-power state.
	if (test_bit(DEV_RUN, &jsqz->state)) {
		dev_info(dev, "deferring suspend, device is still processing!");
		spin_unlock_irqrestore(&jsqz->slock, flags);

		return -EBUSY;
	}
	spin_unlock_irqrestore(&jsqz->slock, flags);

	dev_dbg(dev, "%s: gating clock, suspending\n", __func__);

	HWJSQZ_PROFILE(jsqz_clock_gating(jsqz, false), "SWITCHING OFF CLOCK",
			   jsqz->dev);

	HWJSQZ_PROFILE(
	if (jsqz->qos_req_level > 0)
		pm_qos_update_request(&jsqz->qos_req, 0);,
	"UPDATE QoS REQUEST", jsqz->dev
	);

	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}

static int jsqz_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jsqz_dev *jsqz = platform_get_drvdata(pdev);

	dev_dbg(dev, "%s: BEGIN\n", __func__);
	dev_dbg(dev, "%s: ungating clock, resuming\n", __func__);

	HWJSQZ_PROFILE(jsqz_clock_gating(jsqz, true), "SWITCHING ON CLOCK",
			   jsqz->dev);

//	HWJSQZ_PROFILE(
//	if (jsqz->qos_req_level > 0)
//		pm_qos_update_request(&jsqz->qos_req, jsqz->qos_req_level);,
//	"UPDATE QoS REQUEST", jsqz->dev
//	);

	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}
#endif


static const struct dev_pm_ops jsqz_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(jsqz_suspend, jsqz_resume)
	SET_RUNTIME_PM_OPS(jsqz_runtime_suspend, jsqz_runtime_resume, NULL)
};

// of == OpenFirmware, aka DeviceTree
// This allows matching devices which are specified as "vendorid,deviceid"
// in the device tree to this driver
static const struct of_device_id exynos_jsqz_match[] = {
{
	.compatible = "samsung,exynos-jsqz",
},
{},
};

// Load the kernel module when the device is matched against this driver
MODULE_DEVICE_TABLE(of, exynos_jsqz_match);

static struct platform_driver jsqz_driver = {
	.probe		= jsqz_probe,
	.remove		= jsqz_remove,
	.driver = {
		// Any device advertising itself as MODULE_NAME will be given
		// this driver.
		// No ID table needed (this was more relevant in the past,
		// when platform_device struct was used instead of device-tree)
		.name	= MODULE_NAME,
		.owner	= THIS_MODULE,
		.pm	= &jsqz_pm_ops,
		// Tell the kernel what dev tree names this driver can handle.
		// This is needed because device trees are meant to work with
		// more than one OS, so the driver name might not match the name
		// which is in the device tree.
		.of_match_table = of_match_ptr(exynos_jsqz_match),
	}
};

builtin_platform_driver(jsqz_driver);

MODULE_AUTHOR("Jungik Seo <jungik.seo@samsung.com>");
MODULE_DESCRIPTION("Samsung JPEG SQZ Driver");
MODULE_LICENSE("GPL");

