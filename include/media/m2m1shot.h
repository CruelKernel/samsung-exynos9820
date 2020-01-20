/*
 * include/media/m2m1shot.h
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

#ifndef _M2M1SHOT_H_
#define _M2M1SHOT_H_

#include <linux/kernel.h>
#include <linux/videodev2.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/dma-direction.h>
#include <linux/kref.h>
#include <linux/completion.h>

#include <uapi/linux/m2m1shot.h>

struct m2m1shot_devops;
struct m2m1shot_task;

/**
 * struct m2m1shot_device
 *
 * @misc	: misc device desciptor for user-kernel interface
 * @tasks	: the list of the tasks that are to be scheduled
 * @contexts	: the list of the contexts that is created for this device
 * @dev		: the client device desciptor
 * @lock_task	: lock to protect the consistency of @tasks list and
 *                @current_task
 * @lock_ctx	: lock to protect the consistency of @contexts list
 * @timeout_jiffies: timeout jiffoes for a task. If a task is not finished
 *                   until @timeout_jiffies elapsed,
 *                   m2m1shot_devops.timeout_task() is invoked an the task
 *                   is canced. The user will get an error.
 * @current_task: indicate the task that is currently being processed
 * @ops		: callback functions that the client device driver must
 *                implement according to the events.
 */
struct m2m1shot_device {
	struct miscdevice misc;
	struct list_head tasks;		/* pending tasks for processing */
	struct list_head contexts;	/* created contexts of this device */
	struct device *dev;
	spinlock_t lock_task;		/* lock with irqsave for tasks */
	spinlock_t lock_ctx;		/* lock for contexts */
	unsigned long timeout_jiffies;	/* timeout jiffies for a task */
	struct m2m1shot_task *current_task; /* current working task */
	const struct m2m1shot_devops *ops;
};

/**
 * struct m2m1shot_context - context of tasks
 *
 * @node	: node entry to m2m1shot_device.contexts
 * @mutex	: lock to prevent racing between tasks between the same contexts
 * @kref	: usage count of the context not to release the context while a
 *              : task being processed.
 * @m21dev	: the singleton device instance that the context is born
 * @priv	: private data that is allowed to store client drivers' private
 *                data
 */
struct m2m1shot_context {
	struct list_head node;
	struct mutex mutex;
	struct kref kref;
	struct m2m1shot_device *m21dev;
	void *priv;
};

/**
 * enum m2m1shot_state - state of a task
 *
 * @M2M1SHOT_BUFSTATE_READY	: Task is verified and scheduled for processing
 * @M2M1SHOT_BUFSTATE_PROCESSING: Task is being processed by H/W.
 * @M2M1SHOT_BUFSTATE_DONE	: Task is completed.
 * @M2M1SHOT_BUFSTATE_TIMEDOUT	: Task is not completed until a timed out value
 * @M2M1SHOT_BUFSTATE_ERROR:	: Task is not processed due to verification
 *                                failure
 */
enum m2m1shot_state {
	M2M1SHOT_BUFSTATE_READY,
	M2M1SHOT_BUFSTATE_PROCESSING,
	M2M1SHOT_BUFSTATE_DONE,
	M2M1SHOT_BUFSTATE_TIMEDOUT,
	M2M1SHOT_BUFSTATE_ERROR,
};

/**
 * struct m2m1shot_buffer_plane_dma - descriptions of a buffer
 *
 * @bytes_used	: the size of the buffer that is accessed by H/W. This is filled
 *		  by the client device driver when
 *		  m2m1shot_devops.prepare_format() is called.
 * @dmabuf	: pointer to dmabuf descriptor if the buffer type is
 *		  M2M1SHOT_BUFFER_DMABUF.
 * @attachment	: pointer to dmabuf attachment descriptor if the buffer type is
 *		  M2M1SHOT_BUFFER_DMABUF.
 * @sgt		: scatter-gather list that describes physical memory information
 *		: of the buffer
 * @dma_addr	: DMA address that is the address of the buffer in the H/W's
 *		: address space.
 * @priv	: the client device driver's private data
 */
struct m2m1shot_buffer_plane_dma {
	size_t bytes_used;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	void *priv;
	off_t offset;
};

/**
 * struct m2m1shot_buffer_dma - description of buffers for a task
 *
 * @buffers	: pointer to m2m1shot.buf_out or m2m1shot.buf_cap that are specified
 *		  by user
 * @plane	: descriptions of buffers to pin the buffers while the task is
 *		  processed.
 */
struct m2m1shot_buffer_dma {
	 /* pointer to m2m1shot_task.task.buf_out/cap */
	const struct m2m1shot_buffer *buffer;
	struct m2m1shot_buffer_plane_dma plane[M2M1SHOT_MAX_PLANES];
};

/**
 * struct m2m1shot_task - describes a task to process
 *
 * @task	: descriptions about the frames and format to process
 * @task_node	: list entry to m2m1shot_device.tasks
 * @ctx		: pointer to m2m1shot_context that the task is valid under.
 * @complete	: waiter to finish the task
 * @dma_buf_out	: descriptions of the capture buffers
 * @dma_buf_cap	: descriptions of the output buffers
 * @state	: state of the task
 */
struct m2m1shot_task {
	struct m2m1shot task;
	struct list_head task_node;
	struct m2m1shot_context *ctx;
	struct completion complete;
	struct m2m1shot_buffer_dma dma_buf_out;
	struct m2m1shot_buffer_dma dma_buf_cap;
	enum m2m1shot_state state;
};

/**
 * struct vb2_mem_ops - memory handling/memory allocator operations
 *
 * @init_context: [MANDATORY]
 *                called on creation of struct m2m1shot_context to give a chance
 *                to the driver for registering the driver's private data.
 *                New m2m1shot_context is created when user opens a device node
 *                of m2m1shot.
 * @free_context: [MANDATORY]
 *                called on destruction of struct m2m1shot_context to inform
 *                the driver for unregistring the driver's private data.
 *                m2m1shot_context is destroyed when a user releases all
 *                references to the open file of the device node of m2m1shot.
 * @prepare_format: [MANDATORY]
 *                  called on user's request to process a task. The driver
 *                  receives the format and resolutions of the images to
 *                  process, should return the sizes in bytes and the number of
 *                  buffers(planes) that the driver needs to process the images.
 * @prepare_operation [OPTIONAL]
 *                    called after format checking is finished. The drivers can
 *                    check or prepare the followings:
 *                    - scaling constraint check
 *                    - constructing private or constext data
 * @prepare_buffer: [MANDATORY]
 *                  called after all size validation are passed. The driver
 *                  should complete all procedures for safe access by H/W
 *                  accelerators to process the given buffers.
 * @finish_buffer: [MANDATORY]
 *                 called after the task is finished to release all references
 *                 to the buffers. The drivers should release all resources
 *                 related to the buffer. This is called for every buffers that
 *                 is called @prepare_buffer regardless of the processing the
 *                 task is successful.
 * @device_run: [MANDATORY]
 *              called on the time of the processing the given task. The driver
 *              should run H/W. The driver does not wait for an IRQ.
 * @timeout_task: [MANDATORY]
 *                called when timed out of processing of the given task. The
 *                driver can reset the H/W to cancle the current task.
 * @custom_ioctl: [OPTIONAL]
 *                The driver can directly interact with this @custom_ioctl.
 */
struct m2m1shot_devops {
	int (*init_context)(struct m2m1shot_context *ctx);
	int (*free_context)(struct m2m1shot_context *ctx);
	/* return value: number of planes */
	int (*prepare_format)(struct m2m1shot_context *ctx,
				struct m2m1shot_pix_format *fmt,
				enum dma_data_direction dir,
				size_t bytes_used[]);
	int (*prepare_operation)(struct m2m1shot_context *ctx,
				struct m2m1shot_task *task);
	int (*prepare_buffer)(struct m2m1shot_context *ctx,
				struct m2m1shot_buffer_dma *dma_buffer,
				int plane,
				enum dma_data_direction dir);
	void (*finish_buffer)(struct m2m1shot_context *ctx,
				struct m2m1shot_buffer_dma *dma_buffer,
				int plane,
				enum dma_data_direction dir);
	int (*device_run)(struct m2m1shot_context *ctx,
			struct m2m1shot_task *task);
	void (*timeout_task)(struct m2m1shot_context *ctx,
			struct m2m1shot_task *task);
	/* optional */
	long (*custom_ioctl)(struct m2m1shot_context *ctx,
			unsigned int cmd, unsigned long arg);
};

/**
 * m2m1shot_task_finish - notify a task is finishes and schedule new task
 *
 * - m21dev: pointer to struct m2m1shot_device
 * - task: The task that is finished
 * - success: true if task is processed successfully, false otherwise.
 *
 * This function wakes up the process that is waiting for the completion of
 * @task. An IRQ handler is the best place to call this function. If @error
 * is true, the user that requested @task will receive an error.
 */
void m2m1shot_task_finish(struct m2m1shot_device *m21dev,
				struct m2m1shot_task *task, bool error);

/**
 * m2m1shot_task_cancel - cancel a task and schedule the next task
 *
 * - m21dev: pointer to struct m2m1shot_device
 * - task: The task that is finished
 * - reason: the reason of canceling the task
 *
 * This function is called by the driver that wants to cancel @task and
 * schedule the next task. Most drivers do not need to call this function.
 * Keep in mind that this function does not wake up the process that is blocked
 * for the completion of @task.
 */
void m2m1shot_task_cancel(struct m2m1shot_device *m21dev,
				struct m2m1shot_task *task,
				enum m2m1shot_state reason);

/**
 * m2m1shot_create_device - create and initialze constructs for m2m1shot
 *
 * - dev: the device that m2m1shot provides services
 * - ops: callbacks to m2m1shot
 * - suffix: suffix of device node in /dev after 'm2m1shot_'
 * - id: device instance number if a device has multiple instance. @id will be
 *       attached after @suffix. If @id is -1, no instance number is attached to
 *       @suffix.
 * - timeout_jiffies: timeout jiffies for a task being processed. For 0,
 *                    m2m1shot waits for completion of a task infinitely.
 *
 * Returns the pointer to struct m2m1shot_device on success.
 * Returns -error on failure.
 * This function is most likely called in probe() of a driver.
 */
struct m2m1shot_device *m2m1shot_create_device(struct device *dev,
					const struct m2m1shot_devops *ops,
					const char *suffix, int id,
					unsigned long timeout_jiffies);

/**
 * m2m1shot_destroy_device - destroy all constructs for m2m1shot
 *
 * m21dev - pointer to struct m2m1shot_device that is returned by
 *          m2m1shot_create_device()
 *
 * The driver that has a valid pointer to struct m2m1shot_device returned by
 * m2m1shot_create_device() must call m2m1shot_destroy_device() before removing
 * the driver.
 */
void m2m1shot_destroy_device(struct m2m1shot_device *m21dev);

/**
 * m2m1shot_get_current_task - request the current task under processing
 *
 * m21dev - pointer to struct m2m1shot_device.
 *
 * Returns the pointer to struct m2m1shot_task that is processed by the device
 * whose driver is mostly the caller. The returned pointer is valid until
 * m2m1shot_task_finish() which schedules another task.
 * NULL is returned if no task is currently processed.
 */
static inline struct m2m1shot_task *m2m1shot_get_current_task(
					struct m2m1shot_device *m21dev)
{
	return m21dev->current_task;
}

/**
 * m2m1shot_set_dma_address - set DMA address
 */
static inline void m2m1shot_set_dma_address(
		struct m2m1shot_buffer_dma *buffer_dma,
		int plane, dma_addr_t dma_addr)
{
	buffer_dma->plane[plane].dma_addr = dma_addr;
}

#endif /* _M2M1SHOT_H_ */
