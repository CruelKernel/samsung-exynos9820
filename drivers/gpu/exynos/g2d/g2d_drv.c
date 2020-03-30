/*
 * linux/drivers/gpu/exynos/g2d/g2d_drv.c
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 * Contact: Hyesoo Yu <hyesoo.yu@samsung.com>
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

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/exynos_iovmm.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/sched/task.h>

#include "g2d.h"
#include "g2d_regs.h"
#include "g2d_task.h"
#include "g2d_uapi_process.h"
#include "g2d_debug.h"
#include "g2d_perf.h"

#define MODULE_NAME "exynos-g2d"

static int g2d_update_priority(struct g2d_context *ctx,
					    enum g2d_priority priority)
{
	struct g2d_device *g2d_dev = ctx->g2d_dev;
	struct g2d_task *task;
	unsigned long flags;

	if (ctx->priority == priority)
		return 0;

	atomic_dec(&g2d_dev->prior_stats[ctx->priority]);
	atomic_inc(&g2d_dev->prior_stats[priority]);
	ctx->priority = priority;

	/*
	 * check lower priority task in use, and return EBUSY
	 * for higher priority to avoid waiting lower task completion
	 */
	spin_lock_irqsave(&g2d_dev->lock_task, flags);

	for (task = g2d_dev->tasks; task != NULL; task = task->next) {
		if (!is_task_state_idle(task) &&
		    (task->sec.priority < priority)) {
			spin_unlock_irqrestore(&g2d_dev->lock_task, flags);
			return -EBUSY;
		}
	}

	spin_unlock_irqrestore(&g2d_dev->lock_task, flags);

	return 0;
}

void g2d_hw_timeout_handler(unsigned long arg)
{
	struct g2d_task *task = (struct g2d_task *)arg;
	struct g2d_device *g2d_dev = task->g2d_dev;
	unsigned long flags;
	u32 job_state;

	spin_lock_irqsave(&g2d_dev->lock_task, flags);

	job_state = g2d_hw_get_job_state(g2d_dev, task->sec.job_id);

	g2d_stamp_task(task, G2D_STAMP_STATE_TIMEOUT_HW, job_state);

	perrfndev(g2d_dev, "Time is up: %d msec for job %u %lu %u",
		  G2D_HW_TIMEOUT_MSEC, task->sec.job_id, task->state, job_state);

	if (!is_task_state_active(task))
		/*
		 * The task timed out is not currently running in H/W.
		 * It might be just finished by interrupt.
		 */
		goto out;

	if (job_state == G2D_JOB_STATE_DONE)
		/*
		 * The task timed out is not currently running in H/W.
		 * It will be processed in the interrupt handler.
		 */
		goto out;

	if (is_task_state_killed(task) || g2d_hw_stuck_state(g2d_dev)) {
		bool ret;

		g2d_flush_all_tasks(g2d_dev);

		ret = g2d_hw_global_reset(g2d_dev);
		if (!ret)
			g2d_dump_info(g2d_dev, NULL);

		perrdev(g2d_dev,
			"GLOBAL RESET: Fetal error, %s (ret %d)",
			is_task_state_killed(task) ?
			"killed task not dead" :
			"no running task on queued tasks", ret);

		spin_unlock_irqrestore(&g2d_dev->lock_task, flags);

		wake_up(&g2d_dev->freeze_wait);

		return;
	}

	mod_timer(&task->hw_timer,
	  jiffies + msecs_to_jiffies(G2D_HW_TIMEOUT_MSEC));

	if (job_state != G2D_JOB_STATE_RUNNING)
		/* G2D_JOB_STATE_QUEUEING or G2D_JOB_STATE_SUSPENDING */
		/* Time out is not caused by this task */
		goto out;

	g2d_dump_info(g2d_dev, task);

	mark_task_state_killed(task);

	g2d_hw_kill_task(g2d_dev, task->sec.job_id);

out:
	spin_unlock_irqrestore(&g2d_dev->lock_task, flags);
}

int g2d_device_run(struct g2d_device *g2d_dev, struct g2d_task *task)
{
	g2d_hw_push_task(g2d_dev, task);

	task->ktime_end = ktime_get();

	/* record the time between user request and H/W push */
	g2d_stamp_task(task, G2D_STAMP_STATE_PUSH,
		(int)ktime_us_delta(task->ktime_end, task->ktime_begin));

	task->ktime_begin = ktime_get();

	if (IS_HWFC(task->flags))
		hwfc_set_valid_buffer(task->sec.job_id, task->sec.job_id);

	return 0;
}

static irqreturn_t g2d_irq_handler(int irq, void *priv)
{
	struct g2d_device *g2d_dev = priv;
	unsigned int id;
	u32 intflags, errstatus;

	spin_lock(&g2d_dev->lock_task);

	intflags = g2d_hw_finished_job_ids(g2d_dev);

	g2d_stamp_task(NULL, G2D_STAMP_STATE_INT, intflags);

	if (intflags != 0) {
		for (id = 0; id < G2D_MAX_JOBS; id++) {
			if ((intflags & (1 << id)) == 0)
				continue;

			g2d_finish_task_with_id(g2d_dev, id, true);
		}

		g2d_hw_clear_job_ids(g2d_dev, intflags);
	}

	errstatus = g2d_hw_errint_status(g2d_dev);
	if (errstatus != 0) {
		int job_id = g2d_hw_get_current_task(g2d_dev);
		struct g2d_task *task =
				g2d_get_active_task_from_id(g2d_dev, job_id);

		if (job_id < 0)
			perrdev(g2d_dev, "No task is running in HW");
		else if (task == NULL)
			perrfndev(g2d_dev, "Current job %d in HW is not active",
				  job_id);
		else
			perrfndev(g2d_dev,
				  "Error occurred during running job %d",
				  job_id);

		g2d_hw_clear_int(g2d_dev, errstatus);

		g2d_stamp_task(task, G2D_STAMP_STATE_ERR_INT, errstatus);

		g2d_dump_info(g2d_dev, task);

		g2d_flush_all_tasks(g2d_dev);

		perrdev(g2d_dev,
			"GLOBAL RESET: error interrupt (ret %d)",
			g2d_hw_global_reset(g2d_dev));
	}

	spin_unlock(&g2d_dev->lock_task);

	wake_up(&g2d_dev->freeze_wait);

	return IRQ_HANDLED;
}

#ifdef CONFIG_EXYNOS_IOVMM
static int g2d_iommu_fault_handler(struct iommu_domain *domain,
				struct device *dev, unsigned long fault_addr,
				int fault_flags, void *token)
{
	struct g2d_device *g2d_dev = token;
	struct g2d_task *task;
	int job_id = g2d_hw_get_current_task(g2d_dev);
	unsigned long flags;

	spin_lock_irqsave(&g2d_dev->lock_task, flags);
	task = g2d_get_active_task_from_id(g2d_dev, job_id);
	spin_unlock_irqrestore(&g2d_dev->lock_task, flags);

	g2d_dump_info(g2d_dev, task);

	g2d_stamp_task(task, G2D_STAMP_STATE_MMUFAULT, 0);

	return 0;
}
#endif

static __u32 get_hw_version(struct g2d_device *g2d_dev, __u32 *version)
{
	int ret;

	ret = pm_runtime_get_sync(g2d_dev->dev);
	if (ret < 0) {
		perrdev(g2d_dev, "Failed to enable power (%d)", ret);
		return ret;
	}

	ret = clk_prepare_enable(g2d_dev->clock);
	if (ret < 0) {
		perrdev(g2d_dev, "Failed to enable clock (%d)", ret);
	} else {
		*version = readl_relaxed(g2d_dev->reg + G2D_VERSION_INFO_REG);
		clk_disable(g2d_dev->clock);
	}

	pm_runtime_put(g2d_dev->dev);

	return ret;
}

static void g2d_timeout_perf_work(struct work_struct *work)
{
	struct g2d_context *ctx = container_of(work, struct g2d_context,
					      dwork.work);

	g2d_put_performance(ctx, false);
}

static int g2d_open(struct inode *inode, struct file *filp)
{
	struct g2d_device *g2d_dev;
	struct g2d_context *g2d_ctx;
	struct miscdevice *misc = filp->private_data;

	g2d_ctx = kzalloc(sizeof(*g2d_ctx), GFP_KERNEL);
	if (!g2d_ctx)
		return -ENOMEM;

	if (!strcmp(misc->name, "g2d")) {
		g2d_dev = container_of(misc, struct g2d_device, misc[0]);
		g2d_ctx->authority = G2D_AUTHORITY_HIGHUSER;
	} else {
		g2d_dev = container_of(misc, struct g2d_device, misc[1]);
	}

	filp->private_data = g2d_ctx;

	g2d_ctx->g2d_dev = g2d_dev;

	g2d_ctx->priority = G2D_DEFAULT_PRIORITY;
	atomic_inc(&g2d_dev->prior_stats[g2d_ctx->priority]);

	get_task_struct(current->group_leader);
	g2d_ctx->owner = current->group_leader;

	spin_lock(&g2d_dev->lock_ctx_list);
	list_add(&g2d_ctx->node, &g2d_dev->ctx_list);
	spin_unlock(&g2d_dev->lock_ctx_list);

	mutex_init(&g2d_ctx->lock_hwfc_info);

	INIT_LIST_HEAD(&g2d_ctx->qos_node);
	INIT_DELAYED_WORK(&(g2d_ctx->dwork), g2d_timeout_perf_work);

	return 0;
}

static int g2d_release(struct inode *inode, struct file *filp)
{
	struct g2d_context *g2d_ctx = filp->private_data;
	struct g2d_device *g2d_dev = g2d_ctx->g2d_dev;

	atomic_dec(&g2d_dev->prior_stats[g2d_ctx->priority]);

	if (g2d_ctx->hwfc_info) {
		int i;

		for (i = 0; i < g2d_ctx->hwfc_info->buffer_count; i++)
			dma_buf_put(g2d_ctx->hwfc_info->bufs[i]);
		kfree(g2d_ctx->hwfc_info);
	}

	g2d_put_performance(g2d_ctx, true);
	flush_delayed_work(&g2d_ctx->dwork);

	spin_lock(&g2d_dev->lock_ctx_list);
	list_del(&g2d_ctx->node);
	spin_unlock(&g2d_dev->lock_ctx_list);

	put_task_struct(g2d_ctx->owner);

	kfree(g2d_ctx);

	return 0;
}

static long g2d_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct g2d_context *ctx = filp->private_data;
	struct g2d_device *g2d_dev = ctx->g2d_dev;
	int ret = 0;

	switch (cmd) {
	case G2D_IOC_PROCESS:
	{
		struct g2d_task_data __user *uptr =
				(struct g2d_task_data __user *)arg;
		struct g2d_task_data data;
		struct g2d_task *task;
		int i;

		/*
		 * A process that has lower priority is not allowed
		 * to execute and simply returns -EBUSY
		 */
		for (i = ctx->priority + 1; i < G2D_PRIORITY_END; i++) {
			if (atomic_read(&g2d_dev->prior_stats[i]) > 0) {
				ret =  -EBUSY;
				break;
			}
		}

		if (ret) {
			if (ctx->authority == G2D_AUTHORITY_HIGHUSER)
				perrfndev(g2d_dev,
					  "prio %d/%d found higher than %d", i,
					  atomic_read(&g2d_dev->prior_stats[i]),
					  ctx->priority);
			break;
		}

		if (copy_from_user(&data, uptr, sizeof(data))) {
			perrfndev(g2d_dev, "Failed to read g2d_task_data");
			ret = -EFAULT;
			break;
		}

		/*
		 * If the task should be run by hardware flow control with MFC,
		 * driver must request shared buffer to repeater driver if it
		 * has not been previously requested at current context.
		 * hwfc_request_buffer has increased the reference count inside
		 * function, so driver must reduce the reference
		 * when context releases.
		 */
		if (IS_HWFC(data.flags)) {
			if (ctx->authority != G2D_AUTHORITY_HIGHUSER) {
				ret = -EPERM;
				break;
			}

			mutex_lock(&ctx->lock_hwfc_info);

			if (!ctx->hwfc_info) {
				ctx->hwfc_info = kzalloc(
					sizeof(*ctx->hwfc_info), GFP_KERNEL);
				if (!ctx->hwfc_info) {
					mutex_unlock(&ctx->lock_hwfc_info);
					ret = -ENOMEM;
					break;
				}

				ret = hwfc_request_buffer(ctx->hwfc_info, 0);
				if (ret || (ctx->hwfc_info->buffer_count >
				    MAX_SHARED_BUF_NUM)) {
					kfree(ctx->hwfc_info);
					ctx->hwfc_info = NULL;
					mutex_unlock(&ctx->lock_hwfc_info);
					perrfndev(g2d_dev,
						  "Failed to get hwfc info");
					break;
				}
			}

			mutex_unlock(&ctx->lock_hwfc_info);
		}

		task = g2d_get_free_task(g2d_dev, ctx, IS_HWFC(data.flags));
		if (task == NULL) {
			ret = -EBUSY;
			break;
		}

		kref_init(&task->starter);

		ret = g2d_get_userdata(g2d_dev, ctx, task, &data);
		if (ret < 0) {
			/* release hwfc buffer */
			if (IS_HWFC(task->flags) && (task->bufidx >= 0))
				hwfc_set_valid_buffer(task->bufidx, -1);
			g2d_put_free_task(g2d_dev, task);
			break;
		}

		g2d_stamp_task(task, G2D_STAMP_STATE_BEGIN, task->sec.priority);

		g2d_start_task(task);

		if (!(task->flags & G2D_FLAG_NONBLOCK))
			ret = g2d_wait_put_user(g2d_dev, task,
						uptr, data.flags);

		break;
	}
	case G2D_IOC_PRIORITY:
	{
		enum g2d_priority data;

		if (!(ctx->authority & G2D_AUTHORITY_HIGHUSER)) {
			ret = -EPERM;
			break;
		}

		if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
			perrfndev(g2d_dev, "Failed to get priority");
			ret = -EFAULT;
			break;
		}

		if ((data < G2D_LOW_PRIORITY) || (data >= G2D_PRIORITY_END)) {
			perrfndev(g2d_dev, "Wrong priority %u", data);
			ret = -EINVAL;
			break;
		}

		ret = g2d_update_priority(ctx, data);

		break;
	}
	case G2D_IOC_PERFORMANCE:
	{
		struct g2d_performance_data data;

		if (!(ctx->authority & G2D_AUTHORITY_HIGHUSER)) {
			ret = -EPERM;
			break;
		}

		if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
			perrfndev(g2d_dev, "Failed to read perf data");
			ret = -EFAULT;
			break;
		}
		g2d_set_performance(ctx, &data, false);

		break;
	}
	}

	return ret;
}

#ifdef CONFIG_COMPAT
struct compat_g2d_commands {
	__u32		target[G2DSFR_DST_FIELD_COUNT];
	compat_uptr_t	source[G2D_MAX_IMAGES];
	compat_uptr_t	extra;
	__u32		num_extra_regs;
};

struct compat_g2d_buffer_data {
	union {
		compat_ulong_t userptr;
		struct {
			__s32 fd;
			__u32 offset;
		} dmabuf;
	};
	__u32		length;
};

struct compat_g2d_layer_data {
	__u32			flags;
	__s32			fence;
	__u32			buffer_type;
	__u32			num_buffers;
	struct compat_g2d_buffer_data	buffer[G2D_MAX_BUFFERS];
};

struct compat_g2d_task_data {
	__u32			version;
	__u32			flags;
	__u32			laptime_in_usec;
	__u32			priority;
	__u32			num_source;
	__u32			num_release_fences;
	compat_uptr_t	release_fences;
	struct compat_g2d_layer_data	target;
	compat_uptr_t	source;
	struct compat_g2d_commands	commands;
};

#define COMPAT_G2D_IOC_PROCESS	_IOWR('M', 4, struct compat_g2d_task_data)

static int g2d_compat_get_layerdata(struct g2d_layer_data __user *img,
				    struct compat_g2d_layer_data __user *cimg)
{
	__u32 uw, num_buffers, buftype;
	__s32 sw;
	compat_ulong_t l;
	unsigned int i;
	int ret;

	ret =  get_user(uw, &cimg->flags);
	ret |= put_user(uw, &img->flags);
	ret |= get_user(sw, &cimg->fence);
	ret |= put_user(sw, &img->fence);
	ret |= get_user(buftype, &cimg->buffer_type);
	ret |= put_user(buftype, &img->buffer_type);
	ret |= get_user(num_buffers, &cimg->num_buffers);
	ret |= put_user(num_buffers, &img->num_buffers);

	for (i = 0; i < num_buffers; i++) {
		if (buftype == G2D_BUFTYPE_DMABUF) {
			ret |= get_user(uw, &cimg->buffer[i].dmabuf.offset);
			ret |= put_user(uw, &img->buffer[i].dmabuf.offset);
			ret |= get_user(sw, &cimg->buffer[i].dmabuf.fd);
			ret |= put_user(sw, &img->buffer[i].dmabuf.fd);
		} else {
			ret |= get_user(l, &cimg->buffer[i].userptr);
			ret |= put_user(l, &img->buffer[i].userptr);
		}
		ret |= get_user(uw, &cimg->buffer[i].length);
		ret |= put_user(uw, &img->buffer[i].length);
	}

	return ret ? -EFAULT : 0;
}

static long g2d_compat_ioctl(struct file *filp,
			     unsigned int cmd, unsigned long arg)
{
	struct g2d_context *ctx = filp->private_data;
	struct g2d_task_data __user *data;
	struct g2d_layer_data __user *src;
	struct g2d_commands __user *command;
	struct g2d_reg __user *extra;
	struct compat_g2d_task_data __user *cdata = compat_ptr(arg);
	struct compat_g2d_layer_data __user *csc;
	struct compat_g2d_commands __user *ccmd;
	size_t alloc_size;
	__s32 __user *fences;
	__u32 __user *ptr;
	compat_uptr_t cptr;
	__u32 w, num_source, num_release_fences;
	int ret;

	switch (cmd) {
	case COMPAT_G2D_IOC_PROCESS:
		cmd = (unsigned int)G2D_IOC_PROCESS;
		break;
	case G2D_IOC_PRIORITY:
	case G2D_IOC_PERFORMANCE:
		if (!filp->f_op->unlocked_ioctl)
			return -ENOTTY;

		return filp->f_op->unlocked_ioctl(filp, cmd,
						(unsigned long)compat_ptr(arg));
	default:
		perrfndev(ctx->g2d_dev, "unknown ioctl command %#x", cmd);
		return -EINVAL;
	}

	alloc_size = sizeof(*data);
	data = compat_alloc_user_space(alloc_size);

	ret  = get_user(w, &cdata->version);
	ret |= put_user(w, &data->version);
	ret |= get_user(w, &cdata->flags);
	ret |= put_user(w, &data->flags);
	ret |= get_user(w, &cdata->laptime_in_usec);
	ret |= put_user(w, &data->laptime_in_usec);
	ret |= get_user(w, &cdata->priority);
	ret |= put_user(w, &data->priority);
	ret |= get_user(num_source, &cdata->num_source);
	ret |= put_user(num_source, &data->num_source);
	ret |= get_user(num_release_fences, &cdata->num_release_fences);
	ret |= put_user(num_release_fences, &data->num_release_fences);
	alloc_size += sizeof(__s32) * num_release_fences;
	fences = compat_alloc_user_space(alloc_size);
	ret |= put_user(fences, &data->release_fences);
	if (ret) {
		perrfndev(ctx->g2d_dev, "failed to read task data");
		return -EFAULT;
	}

	ret = g2d_compat_get_layerdata(&data->target, &cdata->target);
	if (ret) {
		perrfndev(ctx->g2d_dev, "failed to read the target data");
		return ret;
	}

	ret = get_user(cptr, &cdata->source);
	csc = compat_ptr(cptr);
	alloc_size += sizeof(*src) * num_source;
	src = compat_alloc_user_space(alloc_size);
	for (w = 0; w < num_source; w++)
		ret |= g2d_compat_get_layerdata(&src[w], &csc[w]);
	ret |= put_user(src, &data->source);
	if (ret) {
		perrfndev(ctx->g2d_dev, "failed to read source layer data");
		return ret;
	}

	command = &data->commands;
	ccmd = &cdata->commands;
	ret = copy_in_user(&command->target, &ccmd->target,
			sizeof(__u32) * G2DSFR_DST_FIELD_COUNT);
	if (ret) {
		perrfndev(ctx->g2d_dev, "failed to read target command data");
		return ret;
	}

	for (w = 0; w < num_source; w++) {
		get_user(cptr, &ccmd->source[w]);
		alloc_size += sizeof(__u32) * G2DSFR_SRC_FIELD_COUNT;
		ptr = compat_alloc_user_space(alloc_size);
		ret = copy_in_user(ptr, compat_ptr(cptr),
				sizeof(__u32) * G2DSFR_SRC_FIELD_COUNT);
		ret |= put_user(ptr, &command->source[w]);
		if (ret) {
			perrfndev(ctx->g2d_dev,
				  "failed to read source %u command data", w);
			return ret;
		}
	}

	ret = get_user(w, &ccmd->num_extra_regs);
	ret |= put_user(w, &command->num_extra_regs);

	/* w contains num_extra_regs */
	get_user(cptr, &ccmd->extra);
	alloc_size += sizeof(*extra) * w;
	extra = compat_alloc_user_space(alloc_size);
	ret |= copy_in_user(extra, compat_ptr(cptr),
				sizeof(*extra) * w);
	ret |= put_user(extra, &command->extra);
	if (ret) {
		perrfndev(ctx->g2d_dev, "failed to read extra command data");
		return ret;
	}

	ret = g2d_ioctl(filp, cmd, (unsigned long)data);
	if (ret)
		return ret;

	ret = get_user(w, &data->laptime_in_usec);
	ret |= put_user(w, &cdata->laptime_in_usec);

	get_user(cptr, &cdata->release_fences);
	ret |= copy_in_user(compat_ptr(cptr), fences,
					sizeof(__s32) * num_release_fences);
	if (ret)
		perrfndev(ctx->g2d_dev, "failed to write userdata");

	return ret;
}
#endif

static const struct file_operations g2d_fops = {
	.owner          = THIS_MODULE,
	.open           = g2d_open,
	.release        = g2d_release,
	.unlocked_ioctl	= g2d_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= g2d_compat_ioctl,
#endif
};

static int g2d_notifier_event(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	struct g2d_device *g2d_dev;

	g2d_dev = container_of(this, struct g2d_device, pm_notifier);

	switch (event) {
	case PM_SUSPEND_PREPARE:
		g2d_prepare_suspend(g2d_dev);
		break;

	case PM_POST_SUSPEND:
		g2d_suspend_finish(g2d_dev);
		break;
	}

	return NOTIFY_OK;
}

static unsigned int g2d_default_ppc[] = {
	/* sc_up none x1 x1/4 x1/9 x1/16 */
	3400, 3100, 2200, 3600, 5100, 7000, //rgb32 non-rotated
	3300, 2700, 2000, 3000, 5200, 6500, //rgb32 rotated
	3000, 2900, 2600, 3400, 5100, 11900, //yuv2p non-rotated
	3200, 2000, 1900, 3300, 5200, 7000, //yuv2p rotated
	2400, 1900, 1900, 2700, 3100, 4100, //8+2 non-rotated
	2500, 900, 900, 2200, 2900, 3700, //8+2 rotated
	2800, 3000, 1600, 1600, 2400, 3200, //afbc non-rotated
	2800, 3000, 1600, 1600, 2400, 3200, //afbc rotated
	3800, //colorfill
};

static struct g2d_dvfs_table g2d_default_dvfs_table[] = {
	{534000, 711000},
	{400000, 534000},
	{336000, 400000},
	{267000, 356000},
	{178000, 200000},
	{107000, 134000},
};

static int g2d_parse_dt(struct g2d_device *g2d_dev)
{
	struct device *dev = g2d_dev->dev;
	int len;

	if (of_property_read_u32_array(dev->of_node, "hw_ppc",
			(u32 *)g2d_dev->hw_ppc,
			(size_t)(ARRAY_SIZE(g2d_dev->hw_ppc)))) {
		perrdev(g2d_dev, "Failed to parse device tree for hw ppc");

		memcpy(g2d_dev->hw_ppc, g2d_default_ppc,
			sizeof(g2d_default_ppc[0]) * PPC_END);
	}

	len = of_property_count_u32_elems(dev->of_node, "g2d_dvfs_table");
	if (len < 0)
		g2d_dev->dvfs_table_cnt = ARRAY_SIZE(g2d_default_dvfs_table);
	else
		g2d_dev->dvfs_table_cnt = len / 2;

	g2d_dev->dvfs_table = devm_kzalloc(dev,
				sizeof(struct g2d_dvfs_table) *
				g2d_dev->dvfs_table_cnt,
				GFP_KERNEL);
	if (!g2d_dev->dvfs_table)
		return -ENOMEM;

	if (len < 0) {
		memcpy(g2d_dev->dvfs_table, g2d_default_dvfs_table,
			sizeof(struct g2d_dvfs_table) *
			g2d_dev->dvfs_table_cnt);
	} else {
		of_property_read_u32_array(dev->of_node, "g2d_dvfs_table",
				(unsigned int *)g2d_dev->dvfs_table, len);
	}

	return 0;
}

#ifdef CONFIG_EXYNOS_ITMON

#define MAX_ITMON_STRATTR 4

bool g2d_itmon_check(struct g2d_device *g2d_dev, char *str_itmon, char *str_attr)
{
	const char *name[MAX_ITMON_STRATTR];
	int size, i;

	if (!str_itmon)
		return false;

	size = of_property_count_strings(g2d_dev->dev->of_node, str_attr);
	if (size < 0 || size > MAX_ITMON_STRATTR)
		return false;

	of_property_read_string_array(g2d_dev->dev->of_node,
				      str_attr, name, size);
	for (i = 0; i < size; i++) {
		if (strncmp(str_itmon, name[i], strlen(name[i])) == 0)
			return true;
	}

	return false;
}

int g2d_itmon_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct g2d_device *g2d_dev = container_of(nb, struct g2d_device, itmon_nb);
	struct itmon_notifier *itmon_info = nb_data;
	struct g2d_task *task;
	static int called_count;
	bool is_power_on = false, is_g2d_itmon = true;

	if (g2d_itmon_check(g2d_dev, itmon_info->port, "itmon,port"))
		is_power_on = true;
	else if (g2d_itmon_check(g2d_dev, itmon_info->dest, "itmon,dest"))
		is_power_on = (itmon_info->onoff) ? true : false;
	else
		is_g2d_itmon = false;

	if (is_g2d_itmon) {
		unsigned long flags;
		int job_id;

		if (called_count++ != 0) {
			perrfndev(g2d_dev,
				  "called %d times, ignore it.", called_count);
			return NOTIFY_DONE;
		}

		for (task = g2d_dev->tasks; task; task = task->next) {
			perrfndev(g2d_dev, "TASK[%d]: state %#lx flags %#x",
				  task->sec.job_id, task->state, task->flags);
			perrfndev(g2d_dev, "prio %d begin@%llu end@%llu nr_src %d",
				  task->sec.priority, ktime_to_us(task->ktime_begin),
				  ktime_to_us(task->ktime_end), task->num_source);
		}

		if (!is_power_on)
			return NOTIFY_DONE;

		job_id = g2d_hw_get_current_task(g2d_dev);

		spin_lock_irqsave(&g2d_dev->lock_task, flags);

		task = g2d_get_active_task_from_id(g2d_dev, job_id);

		spin_unlock_irqrestore(&g2d_dev->lock_task, flags);

		g2d_dump_info(g2d_dev, task);
		exynos_sysmmu_show_status(g2d_dev->dev);
	}

	return NOTIFY_DONE;
}
#endif

struct g2d_device_data {
	unsigned long caps;
	unsigned int max_layers;
};

const struct g2d_device_data g2d_9610_data __initconst = {
	.max_layers = G2D_MAX_IMAGES_HALF,
};

const struct g2d_device_data g2d_9810_data __initconst = {
	.max_layers = G2D_MAX_IMAGES,
};

const struct g2d_device_data g2d_9820_data __initconst = {
	.caps = G2D_DEVICE_CAPS_SELF_PROTECTION | G2D_DEVICE_CAPS_YUV_BITDEPTH,
	.max_layers = G2D_MAX_IMAGES,
};

static const struct of_device_id of_g2d_match[] __refconst = {
	{
		.compatible = "samsung,exynos9810-g2d",
		.data = &g2d_9810_data,
	}, {
		.compatible = "samsung,exynos9610-g2d",
		.data = &g2d_9610_data,
	}, {
		.compatible = "samsung,exynos9820-g2d",
		.data = &g2d_9820_data,
	},
	{},
};

static int g2d_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id;
	struct g2d_device *g2d_dev;
	struct resource *res;
	__u32 version;
	int ret;

	g2d_dev = devm_kzalloc(&pdev->dev, sizeof(*g2d_dev), GFP_KERNEL);
	if (!g2d_dev)
		return -ENOMEM;

	platform_set_drvdata(pdev, g2d_dev);
	g2d_dev->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	g2d_dev->reg = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(g2d_dev->reg))
		return PTR_ERR(g2d_dev->reg);

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		perrdev(g2d_dev, "Failed to get IRQ resource");
		return -ENOENT;
	}

	ret = devm_request_irq(&pdev->dev, res->start,
			       g2d_irq_handler, 0, pdev->name, g2d_dev);
	if (ret) {
		perrdev(g2d_dev, "Failed to install IRQ handler");
		return ret;
	}

	dma_set_mask(&pdev->dev, DMA_BIT_MASK(36));

	g2d_dev->clock = devm_clk_get(&pdev->dev, "gate");
	if (IS_ERR(g2d_dev->clock)) {
		ret = PTR_ERR(g2d_dev->clock);
		perrdev(g2d_dev, "Failed to get clock (%d)", ret);
		return ret;
	}

	iovmm_set_fault_handler(&pdev->dev, g2d_iommu_fault_handler, g2d_dev);

	ret = g2d_parse_dt(g2d_dev);
	if (ret < 0)
		return ret;

	of_id = of_match_node(of_g2d_match, pdev->dev.of_node);
	if (of_id->data) {
		const struct g2d_device_data *devdata = of_id->data;

		g2d_dev->caps = devdata->caps;
		g2d_dev->max_layers = devdata->max_layers;
	}

	ret = iovmm_activate(&pdev->dev);
	if (ret < 0) {
		perrdev(g2d_dev, "Failed to activate iommu");
		return ret;
	}

	/* prepare clock and enable runtime pm */
	pm_runtime_enable(&pdev->dev);

	ret = get_hw_version(g2d_dev, &version);
	if (ret < 0)
		goto err;

	g2d_dev->misc[0].minor = MISC_DYNAMIC_MINOR;
	g2d_dev->misc[0].name = "g2d";
	g2d_dev->misc[0].fops = &g2d_fops;

	/* misc register */
	ret = misc_register(&g2d_dev->misc[0]);
	if (ret) {
		perrdev(g2d_dev, "Failed to register misc device for 0");
		goto err;
	}

	g2d_dev->misc[1].minor = MISC_DYNAMIC_MINOR;
	g2d_dev->misc[1].name = "fimg2d";
	g2d_dev->misc[1].fops = &g2d_fops;

	ret = misc_register(&g2d_dev->misc[1]);
	if (ret) {
		perrdev(g2d_dev, "Failed to register misc device for 1");
		goto err_misc;
	}

	spin_lock_init(&g2d_dev->lock_task);
	spin_lock_init(&g2d_dev->lock_ctx_list);

	INIT_LIST_HEAD(&g2d_dev->tasks_free);
	INIT_LIST_HEAD(&g2d_dev->tasks_free_hwfc);
	INIT_LIST_HEAD(&g2d_dev->tasks_prepared);
	INIT_LIST_HEAD(&g2d_dev->tasks_active);
	INIT_LIST_HEAD(&g2d_dev->qos_contexts);
	INIT_LIST_HEAD(&g2d_dev->ctx_list);

	mutex_init(&g2d_dev->lock_qos);

	ret = g2d_create_tasks(g2d_dev);
	if (ret < 0) {
		perrdev(g2d_dev, "Failed to create tasks");
		goto err_task;
	}

	init_waitqueue_head(&g2d_dev->freeze_wait);
	init_waitqueue_head(&g2d_dev->queued_wait);

	g2d_dev->pm_notifier.notifier_call = &g2d_notifier_event;
	ret = register_pm_notifier(&g2d_dev->pm_notifier);
	if (ret)
		goto err_pm;

	spin_lock_init(&g2d_dev->fence_lock);
	g2d_dev->fence_context = dma_fence_context_alloc(1);

#ifdef CONFIG_EXYNOS_ITMON
	g2d_dev->itmon_nb.notifier_call = g2d_itmon_notifier;
	itmon_notifier_chain_register(&g2d_dev->itmon_nb);
#endif
	dev_info(&pdev->dev, "Probed FIMG2D version %#010x", version);

	g2d_init_debug(g2d_dev);

	return 0;
err_pm:
	g2d_destroy_tasks(g2d_dev);
err_task:
	misc_deregister(&g2d_dev->misc[1]);
err_misc:
	misc_deregister(&g2d_dev->misc[0]);
err:
	pm_runtime_disable(&pdev->dev);
	iovmm_deactivate(g2d_dev->dev);

	perrdev(g2d_dev, "Failed to probe FIMG2D");

	return ret;
}

static void g2d_shutdown(struct platform_device *pdev)
{
	struct g2d_device *g2d_dev = platform_get_drvdata(pdev);

	g2d_stamp_task(NULL, G2D_STAMP_STATE_SHUTDOWN, 0);
	g2d_prepare_suspend(g2d_dev);

	wait_event(g2d_dev->freeze_wait, list_empty(&g2d_dev->tasks_active));

	if (test_and_set_bit(G2D_DEVICE_STATE_IOVMM_DISABLED, &g2d_dev->state))
		iovmm_deactivate(g2d_dev->dev);

	g2d_stamp_task(NULL, G2D_STAMP_STATE_SHUTDOWN, 1);
}

static int g2d_remove(struct platform_device *pdev)
{
	struct g2d_device *g2d_dev = platform_get_drvdata(pdev);

	g2d_destroy_debug(g2d_dev);

	g2d_shutdown(pdev);

	g2d_destroy_tasks(g2d_dev);

	misc_deregister(&g2d_dev->misc[0]);
	misc_deregister(&g2d_dev->misc[1]);

	pm_runtime_disable(&pdev->dev);

	return 0;
}

#ifdef CONFIG_PM
static int g2d_runtime_resume(struct device *dev)
{
	g2d_stamp_task(NULL, G2D_STAMP_STATE_RUNTIME_PM, 0);

	return 0;
}

static int g2d_runtime_suspend(struct device *dev)
{
	struct g2d_device *g2d_dev = dev_get_drvdata(dev);

	clk_unprepare(g2d_dev->clock);
	g2d_stamp_task(NULL, G2D_STAMP_STATE_RUNTIME_PM, 1);

	return 0;
}
#endif

static const struct dev_pm_ops g2d_pm_ops = {
	SET_RUNTIME_PM_OPS(NULL, g2d_runtime_resume, g2d_runtime_suspend)
};

static struct platform_driver g2d_driver = {
	.probe		= g2d_probe,
	.remove		= g2d_remove,
	.shutdown	= g2d_shutdown,
	.driver = {
		.name	= MODULE_NAME,
		.owner	= THIS_MODULE,
		.pm	= &g2d_pm_ops,
		.of_match_table = of_match_ptr(of_g2d_match),
	}
};

module_platform_driver(g2d_driver);
