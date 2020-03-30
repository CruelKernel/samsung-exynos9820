/*
 * linux/drivers/gpu/exynos/g2d/g2d.h
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * Samsung Graphics 2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_G2D_H__
#define __EXYNOS_G2D_H__

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <media/exynos_repeater.h>
#include <linux/pm_qos.h>
#include <soc/samsung/exynos-itmon.h>

struct g2d_task; /* defined in g2d_task.h */

enum g2d_priority {
	G2D_LOW_PRIORITY,
	G2D_MEDIUM_PRIORITY,
	G2D_DEFAULT_PRIORITY = G2D_MEDIUM_PRIORITY,
	G2D_HIGH_PRIORITY,
	G2D_HIGHEST_PRIORITY,
	G2D_PRIORITY_END
};

/*
 * G2D_DEVICE_STATE_SUSPEND should be treated under g2d_dev->lock_task held
 * because it should be consistent with the state of all tasks attached to
 * g2d_dev->tasks_active.
 */
#define G2D_DEVICE_STATE_SUSPEND	1
#define G2D_DEVICE_STATE_IOVMM_DISABLED	2

enum g2d_hw_ppc_rot {
	PPC_NO_ROTATE,
	PPC_ROTATE,
	PPC_ROT,
};

enum g2d_hw_ppc_fmt {
	PPC_RGB,
	PPC_YUV2P,
	PPC_YUV2P_82,
	PPC_AFBC,
	PPC_FMT,
};

enum g2d_hw_ppc_sc {
	PPC_SC_UP,
	PPC_NO_SCALE,
	PPC_SC_DOWN_1,
	PPC_SC_DOWN_4,
	PPC_SC_DOWN_9,
	PPC_SC_DOWN_16,
	PPC_SC,
};

enum g2d_hw_ppc {
	PPC_COLORFILL = PPC_FMT * PPC_ROT * PPC_SC,
	PPC_END,
};

struct g2d_dvfs_table {
	u32 lv;
	u32 freq;
};

/* Proved that G2D does not leak protected conents that it is processing. */
#define G2D_DEVICE_CAPS_SELF_PROTECTION		1
/* Separate bitfield to select YCbCr Bitdepth at REG_COLORMODE[29:28] */
#define G2D_DEVICE_CAPS_YUV_BITDEPTH		2

struct g2d_device {
	unsigned long		state;
	unsigned long		caps;

	struct miscdevice	misc[2];
	struct device		*dev;
	struct clk		*clock;
	void __iomem		*reg;

	u64			fence_context;
	atomic_t		fence_timeline;
	spinlock_t		fence_lock;

	spinlock_t		lock_ctx_list;
	struct list_head	ctx_list;

	/* task management */
	spinlock_t		lock_task;
	struct g2d_task		*tasks;
	struct list_head	tasks_free;
	struct list_head	tasks_free_hwfc;
	struct list_head	tasks_prepared;
	struct list_head	tasks_active;
	struct workqueue_struct	*schedule_workq;

	struct notifier_block	pm_notifier;
	wait_queue_head_t	freeze_wait;
	wait_queue_head_t	queued_wait;

	struct dentry *debug_root;
	struct dentry *debug;
	struct dentry *debug_logs;
	struct dentry *debug_contexts;
	struct dentry *debug_tasks;

	atomic_t	prior_stats[G2D_PRIORITY_END];

	struct mutex			lock_qos;
	struct list_head		qos_contexts;
	u32 hw_ppc[PPC_END];
	u32				max_layers;

	struct g2d_dvfs_table *dvfs_table;
	u32 dvfs_table_cnt;

	struct notifier_block	itmon_nb;
};

#define G2D_AUTHORITY_HIGHUSER 1

struct g2d_context {
	struct list_head	node;
	struct g2d_device	*g2d_dev;
	struct shared_buffer_info *hwfc_info;
	u32 priority;
	int authority;
	struct task_struct	*owner;

	struct delayed_work dwork;

	struct pm_qos_request req;
	struct list_head qos_node;
	struct mutex	lock_hwfc_info;
	u64	r_bw;
	u64	w_bw;
};

#define IPPREFIX "[Exynos][G2D] "
#define perr(format, arg...) \
	pr_err(IPPREFIX format "\n", ##arg)

#define perrfn(format, arg...) \
	pr_err(IPPREFIX  "%s: " format "\n", __func__, ##arg)

#define perrdev(g2d, format, arg...) \
	dev_err(g2d->dev, IPPREFIX format "\n", ##arg)

#define perrfndev(g2d, format, arg...) \
	dev_err(g2d->dev, IPPREFIX  "%s: " format "\n", __func__, ##arg)

int g2d_device_run(struct g2d_device *g2d_dev, struct g2d_task *task);
void g2d_hw_timeout_handler(unsigned long arg);

#endif /* __EXYNOS_G2D_H__ */
