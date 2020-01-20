/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DEVICE_FLITE_H
#define FIMC_IS_DEVICE_FLITE_H

#include <linux/interrupt.h>
#include "fimc-is-type.h"

#define EXPECT_FRAME_START	0
#define EXPECT_FRAME_END	1

#define FLITE_NOTIFY_FSTART	0
#define FLITE_NOTIFY_FEND	1

#define FLITE_ENABLE_FLAG	1
#define FLITE_ENABLE_MASK	0xFFFF
#define FLITE_ENABLE_SHIFT	0

#define FLITE_NOWAIT_FLAG	1
#define FLITE_NOWAIT_MASK	0xFFFF0000
#define FLITE_NOWAIT_SHIFT	16

#define FLITE_OVERFLOW_COUNT	10

#define FLITE_VVALID_TIME_BASE 32 /* ms */


struct fimc_is_device_sensor;

enum fimc_is_flite_state {
	/* finish state */
	FLITE_LAST_CAPTURE = 0,
	/* flite join ischain */
	FLITE_JOIN_ISCHAIN,
	/* one the fly output */
	FLITE_OTF_WITH_3AA,
	/* If it's dummy, H/W setting can't be applied */
	FLITE_DUMMY,
	/* WDMA flag */
	FLITE_DMA_ENABLE,
	FLITE_START_STREAM
};

enum fimc_is_flite_buf_done_mode {
	FLITE_BUF_DONE_NORMAL	= 0, /* when end-irq */
	FLITE_BUF_DONE_EARLY	= 1, /* when delayed work queue since start-irq */
};

/*
 * 10p means 10% early than end irq. We supposed that VVALID time is variable
 * ex. 32 * 0.1 = 3ms, early interval is (33 - 3) = 29ms
 *     32 * 0.2 = 6ms,                   (33 - 6) = 26ms
 *     32 * 0.3 = 9ms,                   (33 - 9) = 23ms
 *     32 * 0.4 = 12ms,                  (33 - 12) = 20ms
 */
enum fimc_is_flite_early_buf_done_mode {
	FLITE_BUF_EARLY_NOTHING	= 0,
	FLITE_BUF_EARLY_10P	= 1, /* 10%(29ms) 3ms */
	FLITE_BUF_EARLY_20P	= 2, /* 20%(26ms) 6ms */
	FLITE_BUF_EARLY_30P	= 3, /* 30%(23ms) 9ms */
	FLITE_BUF_EARLY_40P	= 4, /* 40%(20ms) 12ms */
};

struct fimc_is_device_flite {
	u32				instance;
	u32 __iomem			*base_reg;
	resource_size_t			regs_start;
	resource_size_t			regs_end;
	int				irq;
	unsigned long			state;
	wait_queue_head_t		wait_queue;

	struct fimc_is_image		image;
	struct fimc_is_framemgr		*framemgr;

	u32				overflow_cnt;

	u32				csi; /* which csi channel is connceted */
	u32				group; /* which 3aa gorup is connected when otf is enable */

	u32				sw_checker;
	atomic_t			fcount;
	u32				tasklet_param_str;
	struct tasklet_struct		tasklet_flite_str;
	u32				tasklet_param_end;
	struct tasklet_struct		tasklet_flite_end;

	/* for early buffer done */
	u32				buf_done_mode;
	u32				early_buf_done_mode;
	u32				buf_done_wait_time;
	bool				early_work_skip;
	bool				early_work_called;
	struct tasklet_struct		tasklet_flite_early_end;
	struct workqueue_struct		*early_workqueue;
	struct delayed_work		early_work_wq;
	void				(*chk_early_buf_done)(struct fimc_is_device_flite *flite, u32 framerate, u32 position);

	/* pointer address from device sensor */
	struct v4l2_subdev		**subdev;
};

#if defined(CONFIG_EXYNOS_FIMC_BNS)
int fimc_is_flite_probe(struct fimc_is_device_sensor *device,
	u32 instance);
int fimc_is_flite_open(struct v4l2_subdev *subdev,
	struct fimc_is_framemgr *framemgr);
int fimc_is_flite_close(struct v4l2_subdev *subdev);
#else
static inline int fimc_is_flite_probe(struct fimc_is_device_sensor *device,
	u32 instance) {return 0;}
static inline int fimc_is_flite_open(struct v4l2_subdev *subdev,
	struct fimc_is_framemgr *framemgr) {return 0;}
static inline int fimc_is_flite_close(struct v4l2_subdev *subdev) {return 0;}
#endif

extern u32 __iomem *notify_fcount_sen0;
extern u32 __iomem *notify_fcount_sen1;
extern u32 __iomem *notify_fcount_sen2;
extern u32 __iomem *notify_fcount_sen3;

#endif
