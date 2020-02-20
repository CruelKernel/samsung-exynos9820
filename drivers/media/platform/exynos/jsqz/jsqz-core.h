/* linux/drivers/media/platform/jsqz-encoder/jsqz-core.h
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

#ifndef JSQZ_CORE_H_
#define JSQZ_CORE_H_

#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/pm_qos.h>
#include <linux/wait.h>

#include <linux/hwjsqz.h>

#define MODULE_NAME	"exynos-jsqz"
#define NODE_NAME		"jsqz"

#define JSQZ_HEADER_BYTES		16
#define JSQZ_BYTES_PER_BLOCK	16

/* jsqz hardware device state */
#define DEV_RUN			1
#define DEV_SUSPEND	2

#define JSQZ_INPUT_MIN_WIDTH	32
#define JSQZ_INPUT_MIN_HEIGHT	8

/**
 * struct jsqz_dev - the abstraction for jsqz encoder device
 * @dev         : pointer to the jsqz encoder device
 * @clk_producer: the clock producer for this device
 * @regs        : the mapped hardware registers
 * @state       : device state flags
 * @slock       : spinlock for this data structure, mainly used for state var
 * @version     : IP version number
 *
 * @misc	: misc device descriptor for user-kernel interface
 * @tasks	: the list of the tasks that are to be scheduled
 * @contexts	: the list of the contexts that is created for this device
 * @lock_task	: lock to protect the consistency of @tasks list and
 *                @current_task
 * @lock_ctx	: lock to protect the consistency of @contexts list
 * @timeout_jiffies: timeout jiffoes for a task. If a task is not finished
 *                   until @timeout_jiffies elapsed,
 *                   timeout_task() is invoked an the task
 *                   is canced. The user will get an error.
 * @current_task: indicate the task that is currently being processed
 *                NOTE: this pointer is only valid while the task is being
 *                      PROCESSED, not while it's being prepared for processing.
 */
struct jsqz_dev {
	struct device				*dev;
	struct clk					*clk_producer;
	void __iomem				*regs;
	unsigned long				state;
	spinlock_t					slock;
	u32							version;
	struct pm_qos_request		qos_req;
	s32							qos_req_level;

	struct miscdevice			misc;
	struct list_head			contexts;
	spinlock_t					lock_task;		/* lock with irqsave for tasks */
	spinlock_t					lock_ctx;		/* lock for contexts */
	struct jsqz_task			*current_task;
	struct workqueue_struct		*workqueue;
	wait_queue_head_t			suspend_wait;
};

/**
 * jsqz_ctx - the device context data
 * @jsqz_dev:		JSQZ ENCODER IP device for this context
 * @blocks_on_x         For the encoding process: the number of blocks
 *                      required on X axis.
 *                      This is computed based on the task and config received
 *                      from userspace and is then passed to the HW.
 * @blocks_on_y         For the encoding process: the number of blocks required
 *                      on Y axis
 *
 * @flags:		TBD: maybe not required
 *
 * @node	: node entry to jsqz_dev.contexts
 * @mutex	: lock to prevent racing between tasks between the same contexts
 * @kref	: usage count of the context not to release the context while a
 *              : task being processed.
 * @priv	: private data that is allowed to store client drivers' private
 *                data
 */
struct jsqz_ctx {
	struct jsqz_dev		*jsqz_dev;
	u32					flags;

	struct list_head	node;
	struct mutex		mutex;
	struct kref			kref;
	void *priv;
};


/**
 * enum jsqz_state - state of a task
 *
 * @JSQZ_BUFSTATE_READY	: Task is verified and scheduled for processing
 * @JSQZ_BUFSTATE_PROCESSING  : Task is being processed by H/W.
 * @JSQZ_BUFSTATE_DONE	: Task is completed.
 * @JSQZ_BUFSTATE_TIMEDOUT	: Task was not completed before timeout expired
 * @JSQZ_BUFSTATE_ERROR:	: Task is not processed due to verification
 *                                failure
 */
enum jsqz_state {
	JSQZ_BUFSTATE_READY,
	JSQZ_BUFSTATE_PROCESSING,
	JSQZ_BUFSTATE_DONE,
	JSQZ_BUFSTATE_TIMEDOUT,
	JSQZ_BUFSTATE_RETRY,
	JSQZ_BUFSTATE_ERROR,
};

/**
 * struct jsqz_buffer_plane_dma - descriptions of a buffer
 *
 * @bytes_used	: the size of the buffer that is accessed by H/W. This is filled
 *                based on the task information received from userspace.
 * @dmabuf	: pointer to dmabuf descriptor if the buffer type is
 *		  HWJSQZ_BUFFER_DMABUF.
 *		  This is NULL when the buffer type is HWJSQZ_BUFFER_USERPTR and
 *		  the ptr points to user memory which did NOT already have a
 *		  dmabuf associated to it.
 * @attachment	: pointer to dmabuf attachment descriptor if the buffer type is
 *		  HWJSQZ_BUFFER_DMABUF (or if it's HWJSQZ_BUFFER_USERPTR and we
 *		  found and reused the dmabuf that was associated to that chunk
 *		  of memory, if there was any).
 * @sgt		: scatter-gather list that describes physical memory information
 *		: of the buffer. This is valid under the same condition as
 *		  @attachment and @dmabuf.
 * @dma_addr	: DMA address that is the address of the buffer in the H/W's
 *		: address space.
 *		  We use the IOMMU to map the input dma buffer or user memory
 *		  to a contiguous mapping in H/W's address space, so that H/W
 *		  can perform a DMA transfer on it.
 * @priv	: the client device driver's private data
 * @offset      : when buffer is of type HWJSQZ_BUFFER_USERPTR, this is the
 *		  offset of the buffer inside the VMA that holds it.
 */
struct jsqz_buffer_plane_dma {
	size_t						bytes_used;
	struct dma_buf				*dmabuf;
	struct dma_buf_attachment	*attachment;
	struct sg_table				*sgt;
	dma_addr_t					dma_addr;
	off_t						offset;
};

/**
 * struct jsqz_buffer_dma - description of buffers for a task
 *
 * @buffers	: pointer to buffers received from userspace
 * @plane	: the corresponding internal buffer structure
 */
struct jsqz_buffer_dma {
	/* pointer to jsqz_task.task.buf_out/cap */
	const struct hwSQZ_buffer		*buffer;
	struct jsqz_buffer_plane_dma	plane;
};

/**
 * struct jsqz_task - describes a task to process
 *
 * @user_task	: descriptions about the surfaces and format to process,
 *                provided by userspace.
 *                It also includes the encoding configuration (block size,
 *                dual plane, refinement iterations, partitioning, block mode)
 * @task_node	: list entry to jsqz_dev.tasks
 * @ctx		: pointer to jsqz_ctx that the task is valid under.
 * @complete	: waiter to finish the task
 * @dma_buf_out	: descriptions of the capture buffer (i.e. result from device)
 * @dma_buf_cap	: descriptions of the output buffer
 *                (i.e. what we're giving as input to device)
 * @state	: state of the task
 */
struct jsqz_task {
	struct hwJSQZ_task		user_task;
	struct jsqz_ctx			*ctx;
	struct completion		complete;
	struct jsqz_buffer_dma	dma_buf_out[2];
	struct work_struct		work;
	enum jsqz_state			state;
};

#endif /* JSQZ_CORE_H_ */
