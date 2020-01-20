/*
 * drivers/media/m2m1shot-testdev.c
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
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/videodev2.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/freezer.h>

#include <media/m2m1shot.h>
#include <media/m2m1shot-helper.h>

struct m2m1shot_testdev_drvdata {
	struct m2m1shot_device *m21dev;
	struct task_struct *thread;
	wait_queue_head_t waitqueue;
	struct list_head task_list;
	spinlock_t lock;
};

#define M2M1SHOT_TESTDEV_IOC_TIMEOUT _IOW('T', 0, unsigned long)

struct m2m1shot_testdev_fmt {
	__u32 fmt;
	const char *fmt_name;
	__u8 mbpp[3]; /* bytes * 2 per pixel: should be divided by 2 */
	__u8 planes;
	__u8 buf_mbpp[3]; /* bytes * 2 per pixel: should be divided by 2 */
	__u8 buf_planes;
};

#define TO_MBPP(bpp) ((bpp) << 2)
#define TO_MBPPDIV(bpp_diven, bpp_diver) (((bpp_diven) << 2) / (bpp_diver))
#define TO_BPP(mbpp) ((mbpp) >> 2)

static struct m2m1shot_testdev_fmt m2m1shot_testdev_fmtlist[] = {
	{
		.fmt = V4L2_PIX_FMT_RGB565,
		.fmt_name = "RGB565",
		.mbpp = { TO_MBPP(2), 0, 0},
		.planes = 1,
		.buf_mbpp = { TO_MBPP(2), 0, 0},
		.buf_planes = 1,
	}, {
		.fmt = V4L2_PIX_FMT_BGR32,
		.fmt_name = "BGR32",
		.mbpp = { TO_MBPP(4), 0, 0 },
		.planes = 1,
		.buf_mbpp = { TO_MBPP(4), 0, 0 },
		.buf_planes = 1,
	}, {
		.fmt = V4L2_PIX_FMT_RGB32,
		.fmt_name = "RGB32",
		.mbpp = { TO_MBPP(4), 0, 0 },
		.planes = 1,
		.buf_mbpp = { TO_MBPP(4), 0, 0 },
		.buf_planes = 1,
	}, {
		.fmt = V4L2_PIX_FMT_YUV420,
		.fmt_name = "YUV4:2:0",
		.mbpp = { TO_MBPP(1), TO_MBPPDIV(1, 4), TO_MBPPDIV(1, 4) },
		.planes = 3,
		.buf_mbpp = { TO_MBPP(1) + TO_MBPPDIV(1, 4) + TO_MBPPDIV(1, 4),
				0, 0 },
		.buf_planes = 1,
	}, {
		.fmt = V4L2_PIX_FMT_NV12,
		.fmt_name = "Y/CbCr4:2:0(NV12)",
		.mbpp = { TO_MBPP(1), TO_MBPPDIV(1, 4) + TO_MBPPDIV(1, 4), 0 },
		.planes = 2,
		.buf_mbpp = { TO_MBPP(1) + TO_MBPPDIV(1, 4) + TO_MBPPDIV(1, 4),
				0, 0 },
		.buf_planes = 1,
	}, {
		.fmt = V4L2_PIX_FMT_NV21,
		.fmt_name = "Y/CrCb4:2:0(NV21)",
		.mbpp = { TO_MBPP(1), TO_MBPPDIV(1, 4) + TO_MBPPDIV(1, 4), 0 },
		.planes = 2,
		.buf_mbpp = { TO_MBPP(1) + TO_MBPPDIV(1, 4) + TO_MBPPDIV(1, 4),
				0, 0 },
		.buf_planes = 1,
	}, {
		.fmt = V4L2_PIX_FMT_NV12M,
		.fmt_name = "Y/CbCr4:2:0(NV12M)",
		.mbpp = { TO_MBPP(1), TO_MBPPDIV(1, 4) + TO_MBPPDIV(1, 4), 0 },
		.planes = 2,
		.buf_mbpp = { TO_MBPP(1), TO_MBPPDIV(1, 4) + TO_MBPPDIV(1, 4),
				0 },
		.buf_planes = 2,
	}, {
		.fmt = V4L2_PIX_FMT_NV21M,
		.fmt_name = "Y/CrCb4:2:0(NV21M)",
		.mbpp = { TO_MBPP(1), TO_MBPPDIV(1, 4) + TO_MBPPDIV(1, 4), 0 },
		.planes = 2,
		.buf_mbpp = { TO_MBPP(1), TO_MBPPDIV(1, 4) + TO_MBPPDIV(1, 4),
				0 },
		.buf_planes = 2,
	}, {
		.fmt = V4L2_PIX_FMT_YUV422P,
		.fmt_name = "YUV4:2:2",
		.mbpp = { TO_MBPP(1), TO_MBPPDIV(1, 2), TO_MBPPDIV(1, 2) },
		.planes = 3,
		.buf_mbpp = { TO_MBPP(1) + TO_MBPPDIV(1, 2) + TO_MBPPDIV(1, 2),
				0, 0 },
		.buf_planes = 1,
	}, {
		.fmt = V4L2_PIX_FMT_NV16,
		.fmt_name = "Y/CbCr4:2:2(NV16)",
		.mbpp = { TO_MBPP(1), TO_MBPPDIV(1, 2) + TO_MBPPDIV(1, 2), 0 },
		.planes = 2,
		.buf_mbpp = { TO_MBPP(1) + TO_MBPPDIV(1, 2) + TO_MBPPDIV(1, 2),
				0, 0 },
		.buf_planes = 1,
	}, {
		.fmt = V4L2_PIX_FMT_NV61,
		.fmt_name = "Y/CrCb4:2:2(NV61)",
		.mbpp = { TO_MBPP(1), TO_MBPPDIV(1, 2) + TO_MBPPDIV(1, 2), 0 },
		.planes = 2,
		.buf_mbpp = { TO_MBPP(1) + TO_MBPPDIV(1, 2) + TO_MBPPDIV(1, 2),
				0, 0 },
		.buf_planes = 1,
	}, {
		.fmt = V4L2_PIX_FMT_YUYV,
		.fmt_name = "YUV4:2:2(YUYV)",
		.mbpp = { TO_MBPP(1) + TO_MBPPDIV(1, 2) + TO_MBPPDIV(1, 2),
				0, 0},
		.planes = 1,
		.buf_mbpp = { TO_MBPP(1) + TO_MBPPDIV(1, 2) + TO_MBPPDIV(1, 2),
				0, 0 },
		.buf_planes = 1,
	}, {
		.fmt = V4L2_PIX_FMT_YUV444,
		.fmt_name = "YUV4:4:4",
		.mbpp = { TO_MBPP(1), TO_MBPP(1), TO_MBPP(1) },
		.planes = 3,
		.buf_mbpp = { TO_MBPP(1) + TO_MBPP(1) + TO_MBPP(1), 0, 0 },
		.buf_planes = 1,
	},
};

static int m2m1shot_testdev_init_context(struct m2m1shot_context *ctx)
{
	ctx->priv = NULL; /* no timeout generated */

	return 0;
}

static int m2m1shot_testdev_free_context(struct m2m1shot_context *ctx)
{
	return 0;
}

static int m2m1shot_testdev_prepare_format(struct m2m1shot_context *ctx,
			struct m2m1shot_pix_format *fmt,
			enum dma_data_direction dir,
			size_t bytes_used[])
{
	size_t i, j;
	for (i = 0; i < ARRAY_SIZE(m2m1shot_testdev_fmtlist); i++) {
		if (fmt->fmt == m2m1shot_testdev_fmtlist[i].fmt)
			break;
	}

	if (i == ARRAY_SIZE(m2m1shot_testdev_fmtlist)) {
		dev_err(ctx->m21dev->dev, "Unknown format %#x\n", fmt->fmt);
		return -EINVAL;
	}

	for (j = 0; j < m2m1shot_testdev_fmtlist[i].buf_planes; j++)
		bytes_used[j] = TO_BPP(m2m1shot_testdev_fmtlist[i].buf_mbpp[j] *
					fmt->width * fmt->height);
	return m2m1shot_testdev_fmtlist[i].buf_planes;
}

static int m2m1shot_testdev_prepare_buffer(struct m2m1shot_context *ctx,
			struct m2m1shot_buffer_dma *dma_buffer,
			int plane,
			enum dma_data_direction dir)
{
	return m2m1shot_map_dma_buf(ctx->m21dev->dev,
				&dma_buffer->plane[plane], dir);
}

static void m2m1shot_testdev_finish_buffer(struct m2m1shot_context *ctx,
			struct m2m1shot_buffer_dma *dma_buffer,
			int plane,
			enum dma_data_direction dir)
{
	m2m1shot_unmap_dma_buf(ctx->m21dev->dev, &dma_buffer->plane[plane], dir);
}

struct m2m1shot_testdev_work {
	struct list_head node;
	struct m2m1shot_context *ctx;
	struct m2m1shot_task *task;
};

static int m2m1shot_testdev_worker(void *data)
{
	struct m2m1shot_testdev_drvdata *drvdata = data;

	while (true) {
		struct m2m1shot_testdev_work *work;
		struct m2m1shot_task *task;

		wait_event_freezable(drvdata->waitqueue,
				     !list_empty(&drvdata->task_list));

		spin_lock(&drvdata->lock);
		BUG_ON(list_empty(&drvdata->task_list));
		work = list_first_entry(&drvdata->task_list,
					struct m2m1shot_testdev_work, node);
		list_del(&work->node);
		BUG_ON(!list_empty(&drvdata->task_list));
		spin_unlock(&drvdata->lock);

		msleep(20);

		task = m2m1shot_get_current_task(drvdata->m21dev);
		BUG_ON(!task);

		BUG_ON(task != work->task);
		BUG_ON(task->ctx != work->ctx);

		kfree(work);

		m2m1shot_task_finish(drvdata->m21dev, task, true);
	}

	return 0;

}

static int m2m1shot_testdev_device_run(struct m2m1shot_context *ctx,
		struct m2m1shot_task *task)
{
	struct m2m1shot_testdev_work *work;
	struct device *dev = ctx->m21dev->dev;
	struct m2m1shot_testdev_drvdata *drvdata = dev_get_drvdata(dev);

	if (ctx->priv) /* timeout generated */
		return 0;

	work = kmalloc(sizeof(*work), GFP_KERNEL);
	if (!work) {
		pr_err("%s: Failed to allocate work struct\n", __func__);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&work->node);
	work->ctx = ctx;
	work->task = task;

	spin_lock(&drvdata->lock);
	BUG_ON(!list_empty(&drvdata->task_list));
	list_add_tail(&work->node, &drvdata->task_list);
	spin_unlock(&drvdata->lock);

	if (current != drvdata->thread);
		wake_up(&drvdata->waitqueue);

	return 0;
}

static void m2m1shot_testdev_timeout_task(struct m2m1shot_context *ctx,
		struct m2m1shot_task *task)
{
	dev_info(ctx->m21dev->dev, "%s: Timeout occurred\n", __func__);
}

static long m2m1shot_testdev_ioctl(struct m2m1shot_context *ctx,
			unsigned int cmd, unsigned long arg)
{
	unsigned long timeout;

	if (cmd != M2M1SHOT_TESTDEV_IOC_TIMEOUT) {
		dev_err(ctx->m21dev->dev, "%s: Unknown ioctl cmd %#x\n",
			__func__, cmd);
		return -ENOSYS;
	}

	if (get_user(timeout, (unsigned long __user *)arg)) {
		dev_err(ctx->m21dev->dev,
			"%s: Failed to read user data\n", __func__);
		return -EFAULT;
	}

	if (timeout)
		ctx->priv = (void *)1; /* timeout generated */
	else
		ctx->priv = NULL; /* timeout not generated */

	dev_info(ctx->m21dev->dev, "%s: Timeout geration is %s",
			__func__, timeout ? "set" : "unset");

	return 0;
}

static const struct m2m1shot_devops m2m1shot_testdev_ops = {
	.init_context = m2m1shot_testdev_init_context,
	.free_context = m2m1shot_testdev_free_context,
	.prepare_format = m2m1shot_testdev_prepare_format,
	.prepare_buffer = m2m1shot_testdev_prepare_buffer,
	.finish_buffer = m2m1shot_testdev_finish_buffer,
	.device_run = m2m1shot_testdev_device_run,
	.timeout_task = m2m1shot_testdev_timeout_task,
	.custom_ioctl = m2m1shot_testdev_ioctl,
};

static struct platform_device m2m1shot_testdev_pdev = {
	.name = "m2m1shot_testdev",
};

static int m2m1shot_testdev_init(void)
{
	int ret = 0;
	struct m2m1shot_device *m21dev;
	struct m2m1shot_testdev_drvdata *drvdata;

	drvdata = kzalloc(sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata) {
		pr_err("%s: Failed allocate drvdata\n", __func__);
		return -ENOMEM;
	}
	ret = platform_device_register(&m2m1shot_testdev_pdev);
	if (ret) {
		pr_err("%s: Failed to register platform device\n", __func__);
		goto err_register;
	}

	m21dev = m2m1shot_create_device(&m2m1shot_testdev_pdev.dev,
		&m2m1shot_testdev_ops, "testdev", -1, msecs_to_jiffies(500));
	if (IS_ERR(m21dev)) {
		pr_err("%s: Failed to create m2m1shot device\n", __func__);
		ret = PTR_ERR(m21dev);
		goto err_create;
	}

	drvdata->thread = kthread_run(m2m1shot_testdev_worker, drvdata,
				"%s", "m2m1shot_tesdev_worker");
	if (IS_ERR(drvdata->thread)) {
		pr_err("%s: Failed create worker thread\n", __func__);
		ret = PTR_ERR(drvdata->thread);
		goto err_worker;
	}

	drvdata->m21dev = m21dev;
	INIT_LIST_HEAD(&drvdata->task_list);
	spin_lock_init(&drvdata->lock);
	init_waitqueue_head(&drvdata->waitqueue);

	dev_set_drvdata(&m2m1shot_testdev_pdev.dev, drvdata);

	return 0;

err_worker:
	m2m1shot_destroy_device(m21dev);
err_create:
	platform_device_unregister(&m2m1shot_testdev_pdev);
err_register:
	kfree(drvdata);
	return ret;
}
module_init(m2m1shot_testdev_init);
