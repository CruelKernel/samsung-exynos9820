/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
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
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/bug.h>

#include "fimc-is-time.h"
#include "fimc-is-core.h"
#include "fimc-is-regs.h"
#include "fimc-is-hw.h"
#include "fimc-is-interface.h"
#include "fimc-is-device-flite.h"

#ifdef SUPPORTED_EARLYBUF_DONE_SW
static void flite_early_buf_done_start(struct v4l2_subdev *subdev)
{
	struct fimc_is_device_sensor *device;
	device = v4l2_get_subdev_hostdata(subdev);
	if (!device) {
		err("device is NULL");
		BUG();
	}

	if (device->early_buf_done_mode) {
		int framerate = fimc_is_sensor_g_framerate(device);
		/* timeout set : frameduration(ms) * early_buf_done_ratio(5~50%) */
		u32 msec_timeout = ((1000 / framerate) *
			(100 - (5 * device->early_buf_done_mode))) / 100;
		pr_info("msec_timeout : %d \n" , msec_timeout);
		hrtimer_start(&device->early_buf_timer,
				ktime_set(0, msec_timeout * NSEC_PER_MSEC), HRTIMER_MODE_REL);
	}
}
#endif

static u32 g_print_cnt;

void tasklet_flite_str_otf(unsigned long data)
{
	struct v4l2_subdev *subdev;
	struct fimc_is_device_flite *flite;
	struct fimc_is_device_sensor *device;
	struct fimc_is_device_ischain *ischain;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group_3aa, *group_isp;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	unsigned long flags;
	u32 fcount, group_3aa_id, group_isp_id;

	subdev = (struct v4l2_subdev *)data;
	flite = v4l2_get_subdevdata(subdev);
	if (!flite) {
		err("flite is NULL");
		BUG();
	}

	device = v4l2_get_subdev_hostdata(subdev);
	if (!device) {
		err("device is NULL");
		BUG();
	}

	fcount = atomic_read(&flite->fcount);
	ischain = device->ischain;
	framemgr = flite->framemgr;

	dbg_tasklet("S%d\n", fcount);

	groupmgr = ischain->groupmgr;
	group_3aa = &ischain->group_3aa;
	group_isp = &ischain->group_isp;

	group_3aa_id = group_3aa->id;
	group_isp_id = group_isp->id;

	if (group_3aa_id >= GROUP_ID_MAX) {
		merr("group 3aa id is invalid(%d)", flite, group_3aa_id);
		goto trigger_skip;
	}

	if (group_isp_id >= GROUP_ID_MAX) {
		merr("group isp id is invalid(%d)", flite, group_isp_id);
		goto trigger_skip;
	}

	if (group_3aa->sync_shots == 0)
		goto trigger_skip;

	if (unlikely(list_empty(&group_3aa->smp_trigger.wait_list))) {
		atomic_set(&group_3aa->sensor_fcount, fcount + group_3aa->skip_shots);

		/*
		 * pcount : program count
		 * current program count(location) in kthread
		 */
		if (((g_print_cnt % LOG_INTERVAL_OF_DROPS) == 0) ||
			(g_print_cnt < LOG_INTERVAL_OF_DROPS)) {
			info("GP%d(res %d, rcnt %d, scnt %d), "
				"grp2(res %d, rcnt %d, scnt %d), "
				"fcount %d pcount %d\n",
				group_3aa_id,
				groupmgr->gtask[group_3aa_id].smp_resource.count,
				atomic_read(&group_3aa->rcount),
				atomic_read(&group_3aa->scount),
				groupmgr->gtask[group_isp_id].smp_resource.count,
				atomic_read(&group_isp->rcount),
				atomic_read(&group_isp->scount),
				fcount + group_3aa->skip_shots,
				group_3aa->pcount);
		}
		g_print_cnt++;
	} else {
		g_print_cnt = 0;
		atomic_set(&group_3aa->sensor_fcount, fcount + group_3aa->skip_shots);
		up(&group_3aa->smp_trigger);
	}

trigger_skip:
	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_9, flags);

	frame = peek_frame(framemgr, FS_REQUEST);
	if (frame) {
		flite_hw_set_dma_addr(flite->base_reg, 0, true,
				(u32)frame->dvaddr_buffer[0]);
		trans_frame(framemgr, frame, FS_PROCESS);
	} else {
		flite_hw_set_dma_addr(flite->base_reg, 0, false, 0);
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_9, flags);

#if defined(SUPPORTED_EARLYBUF_DONE_SW)
	flite_early_buf_done_start(subdev);
#endif
	v4l2_subdev_notify(subdev, FLITE_NOTIFY_FSTART, &fcount);
}

void tasklet_flite_str_m2m(unsigned long data)
{
	struct v4l2_subdev *subdev;
	struct fimc_is_device_flite *flite;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	unsigned long flags;
	u32 fcount;

	subdev = (struct v4l2_subdev *)data;
	flite = v4l2_get_subdevdata(subdev);
	if (!flite) {
		err("flite is NULL");
		BUG();
	}

	fcount = atomic_read(&flite->fcount);
	framemgr = flite->framemgr;

	dbg_tasklet("S%d\n", fcount);

	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_10, flags);

	frame = peek_frame(framemgr, FS_REQUEST);
	if (frame) {
		flite_hw_set_dma_addr(flite->base_reg, 0, true,
				(u32)frame->dvaddr_buffer[0]);
		trans_frame(framemgr, frame, FS_PROCESS);
	} else {
		flite_hw_set_dma_addr(flite->base_reg, 0, false, 0);
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_10, flags);
#if defined(SUPPORTED_EARLYBUF_DONE_SW)
	flite_early_buf_done_start(subdev);
#endif
	v4l2_subdev_notify(subdev, FLITE_NOTIFY_FSTART, &fcount);
}

static void tasklet_flite_end(unsigned long data)
{
	struct fimc_is_device_flite *flite;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct fimc_is_frame *frame_done;
	struct v4l2_subdev *subdev;

	frame_done = NULL;
	subdev = (struct v4l2_subdev *)data;
	flite = v4l2_get_subdevdata(subdev);
	if (!flite) {
		err("flite is NULL");
		BUG();
	}

	framemgr = flite->framemgr;

	if (flite_hw_get_interrupt_mask(flite->base_reg, (1 << FLITE_MASK_IRQ_OVERFLOW)))
		flite_hw_set_interrupt_mask(flite->base_reg, true, (1 << FLITE_MASK_IRQ_OVERFLOW));

	dbg_tasklet("E%d\n", atomic_read(&flite->fcount));

	framemgr_e_barrier(framemgr, FMGR_IDX_12);

	if (flite_hw_get_output_dma(flite->base_reg)) {
		if (framemgr->queued_count[FS_PROCESS] == 2) {
			frame = peek_frame(framemgr, FS_PROCESS);
			if (frame) {
				frame_done = frame;
				trans_frame(framemgr, frame, FS_COMPLETE);
			} else {
				merr("sensor process is empty", flite);
				frame_manager_print_queues(framemgr);
				BUG();
			}
		}
	} else {
		frame = peek_frame(framemgr, FS_PROCESS);
		if (frame) {
			frame_done = frame;
			trans_frame(framemgr, frame, FS_COMPLETE);

			/* 2. next frame ready */
			frame = peek_frame(framemgr, FS_REQUEST);
			if (frame) {
				if (flite_hw_get_status(flite->base_reg,
						(1 << FLITE_STATUS_MIPI_VALID), false)) {
					merr("over vblank", flite);
				} else {
					flite_hw_set_dma_addr(flite->base_reg, 0, true,
							(u32)frame->dvaddr_buffer[0]);
					trans_frame(framemgr, frame, FS_PROCESS);
				}
			} else {
				mwarn("sensor request(%d) is empty", flite, atomic_read(&flite->fcount));
				/* frame_manager_print_queues(framemgr); */
			}
		}
	}

	framemgr_x_barrier(framemgr, FMGR_IDX_12);

	v4l2_subdev_notify(subdev, FLITE_NOTIFY_FEND, frame_done);
}

#if defined(SUPPORTED_EARLYBUF_DONE_SW)
static enum hrtimer_restart flite_early_buf_done(struct hrtimer *timer)
{
	struct fimc_is_device_sensor *device = container_of(timer, struct fimc_is_device_sensor, early_buf_timer);
	struct fimc_is_device_flite *flite;

	flite = (struct fimc_is_device_flite *)v4l2_get_subdevdata(device->subdev_flite);
	flite->sw_checker = EXPECT_FRAME_START;
	tasklet_schedule(&flite->tasklet_flite_end);

	return HRTIMER_NORESTART;
}
#endif

static inline void notify_fcount(struct fimc_is_device_flite *flite)
{
#ifdef ENABLE_IS_CORE
	if (test_bit(FLITE_JOIN_ISCHAIN, &flite->state)) {
		if (flite->csi == CSI_ID_A)
			writel(atomic_read(&flite->fcount), notify_fcount_sen0);
		else if (flite->csi == CSI_ID_B)
			writel(atomic_read(&flite->fcount), notify_fcount_sen1);
		else if (flite->csi == CSI_ID_C)
			writel(atomic_read(&flite->fcount), notify_fcount_sen2);
		else if (flite->csi == CSI_ID_D)
			writel(atomic_read(&flite->fcount), notify_fcount_sen3);
		else
			err("unresolved channel(%d)", flite->instance);
	}
#endif
}

irqreturn_t fimc_is_isr_flite(int irq, void *data)
{
	u32 status1;
	u32 frame_start_end, frame_start_line;
	struct fimc_is_device_flite *flite;

	flite = data;
	status1 = flite_hw_get_status(flite->base_reg, ((1 << FLITE_STATUS_ALL) - 1), true);

	if (test_bit(FLITE_LAST_CAPTURE, &flite->state)) {
		if (status1) {
			info("[CamIF%d] last status1 : 0x%08X\n", flite->instance, status1);
			goto clear_status;
		}

		err("[CamIF%d] unintended intr is occured", flite->instance);

		flite_hw_dump(flite->base_reg);
		flite_hw_reset(flite->base_reg);

		goto clear_status;
	}

	/* Both start irq and end irq occured */
	frame_start_end = (1 << FLITE_STATUS_IRQ_SRC_START) | (1 << FLITE_STATUS_IRQ_SRC_END);
	/* Both start irq and line irq occured */
	frame_start_line = (1 << FLITE_STATUS_IRQ_SRC_START) | (1 << FLITE_STATUS_IRQ_SRC_LINE);

	if ((status1 & frame_start_end) == frame_start_end ||
			(status1 & frame_start_line) == frame_start_line) {
		dbg_fliteisr("*");
		/* frame both interrupt since latency */
		if (flite->sw_checker) {
			dbg_fliteisr(">");
			/* frame end interrupt */
			flite->sw_checker = EXPECT_FRAME_START;
			tasklet_schedule(&flite->tasklet_flite_end);
			dbg_fliteisr("<");
			/* frame start interrupt */
			flite->sw_checker = EXPECT_FRAME_END;
			atomic_inc(&flite->fcount);
			notify_fcount(flite);
			tasklet_schedule(&flite->tasklet_flite_str);
		} else {
			dbg_fliteisr("<");
			/* frame start interrupt */
			flite->sw_checker = EXPECT_FRAME_END;
			atomic_inc(&flite->fcount);
			notify_fcount(flite);
			tasklet_schedule(&flite->tasklet_flite_str);
			dbg_fliteisr(">");
			/* frame end interrupt */
			flite->sw_checker = EXPECT_FRAME_START;
			tasklet_schedule(&flite->tasklet_flite_end);
		}
	} else if (status1 & (1 << FLITE_STATUS_IRQ_SRC_START)) {
		/* W/A: Skip start tasklet at interrupt lost case */
		if (flite->sw_checker != EXPECT_FRAME_START) {
			warn("[CamIF%d] Lost end interupt\n",
					flite->instance);
			goto clear_status;
		}
		dbg_fliteisr("<");
		/* frame start interrupt */
		flite->sw_checker = EXPECT_FRAME_END;
		atomic_inc(&flite->fcount);
		notify_fcount(flite);
		tasklet_schedule(&flite->tasklet_flite_str);
	} else if (status1 & (1 << FLITE_STATUS_IRQ_SRC_END)) {
		/* W/A: Skip end tasklet at interrupt lost case */
		if (flite->sw_checker != EXPECT_FRAME_END) {
			warn("[CamIF%d] Lost start interupt\n",
					flite->instance);
			goto clear_status;
		}
		dbg_fliteisr(">");
		/* frame end interrupt */
		flite->sw_checker = EXPECT_FRAME_START;
		tasklet_schedule(&flite->tasklet_flite_end);
	} else if (status1 & (1 << FLITE_STATUS_IRQ_SRC_LINE)) {
		/* For Line Interrupt */
		/* W/A: Skip end tasklet at interrupt lost case */
		if (flite->sw_checker != EXPECT_FRAME_END) {
			warn("[CamIF%d] Lost start interupt\n",
					flite->instance);
			goto clear_status;
		}
		dbg_fliteisr(">");
		/* frame end interrupt */
		flite->sw_checker = EXPECT_FRAME_START;
		tasklet_schedule(&flite->tasklet_flite_end);
	}

clear_status:
	if (status1 & (1 << FLITE_STATUS_IRQ_SRC_LAST_CAPTURE)) {
		/* Last Frame Capture Interrupt */
		info("[CamIF%d] Last Frame Capture(fcount : %d)\n",
			flite->instance, atomic_read(&flite->fcount));

		/* Notify last capture */
		set_bit(FLITE_LAST_CAPTURE, &flite->state);
		wake_up(&flite->wait_queue);
	}

	if (status1 & (1 << FLITE_STATUS_OFCR)) {
		flite_hw_set_interrupt_mask(flite->base_reg, false, (1 << FLITE_MASK_IRQ_OVERFLOW));

		if (flite->overflow_cnt % FLITE_OVERFLOW_COUNT == 0)
			pr_err("[CamIF%d] OFCR(cnt:%u)\n", flite->instance, flite->overflow_cnt);
		flite_hw_get_status(flite->base_reg, (1 << FLITE_STATUS_OFCR), true);
		flite->overflow_cnt++;
	}

	if (status1 & (1 << FLITE_STATUS_OFCB)) {
		flite_hw_set_interrupt_mask(flite->base_reg, false, (1 << FLITE_MASK_IRQ_OVERFLOW));

		if (flite->overflow_cnt % FLITE_OVERFLOW_COUNT == 0)
			pr_err("[CamIF%d] OFCB(cnt:%u)\n", flite->instance, flite->overflow_cnt);

		flite_hw_get_status(flite->base_reg, (1 << FLITE_STATUS_OFCB), true);
		flite->overflow_cnt++;
	}

	if (status1 & (1 << FLITE_STATUS_OFY)) {
		flite_hw_set_interrupt_mask(flite->base_reg, false, (1 << FLITE_MASK_IRQ_OVERFLOW));

		if (flite->overflow_cnt % FLITE_OVERFLOW_COUNT == 0)
			pr_err("[CamIF%d] OFY(cnt:%u)\n", flite->instance, flite->overflow_cnt);
		flite_hw_get_status(flite->base_reg, (1 << FLITE_STATUS_OFY), true);
		flite->overflow_cnt++;
	}

	return IRQ_HANDLED;
}

static int start_fimc_lite(u32 __iomem *base_reg,
	struct fimc_is_image *image,
	u32 instance,
	u32 bns,
	u32 csi_ch,
	u32 flite_ch,
	struct fimc_is_device_sensor *sensor)
{
	/* source size, format setting */
	flite_hw_set_fmt_source(base_reg, image);

	/* dma size, format setting */
	flite_hw_set_fmt_dma(base_reg, image);

	/* set interrupt source */
	flite_hw_set_interrupt_mask(base_reg, true, ((1 << FLITE_MASK_IRQ_ALL) - 1));

#if defined(SUPPORTED_EARLYBUF_DONE_SW) || defined(SUPPORTED_EARLYBUF_DONE_HW)
	/* in case of early buffer done mode, disabled end interrupt */
	if (sensor->early_buf_done_mode) {
		flite_hw_set_interrupt_mask(base_reg, false, (1 << FLITE_MASK_IRQ_END));
#if defined(SUPPORTED_EARLYBUF_DONE_SW)
		flite_hw_set_interrupt_mask(base_reg, false, (1 << FLITE_MASK_IRQ_LINE));
#elif defined(SUPPORTED_EARLYBUF_DONE_HW)
		flite_hw_s_control(base_reg, FLITE_CTRL_LINE_RATIO, sensor->early_buf_done_mode);
#endif
	} else {
		flite_hw_set_interrupt_mask(base_reg, false, (1 << FLITE_MASK_IRQ_LINE));
	}
#endif

	/* path setting */
	if (sensor->ischain && !test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &sensor->ischain->state))
		/* set mux setting for otf between 3aa and isp */
		flite_hw_set_path(base_reg, instance,
			csi_ch, flite_ch,
			sensor->ischain->group_3aa.id - GROUP_ID_3AA0,
			sensor->ischain->group_isp.id - GROUP_ID_ISP0);
	else
		flite_hw_set_path(base_reg, instance,
			csi_ch, flite_ch, -1, -1);

#ifdef COLORBAR_MODE
	flite_hw_s_control(base_reg, FLITE_CTRL_TEST_PATTERN, 1);
#endif

	/* binning scale setting */
	if (bns)
		flite_hw_set_bns(base_reg, true, image);

	/* start fimc-bns(lite) */
	flite_hw_enable(base_reg);

	return 0;
}

int fimc_is_flite_open(struct v4l2_subdev *subdev,
	struct fimc_is_framemgr *framemgr)
{
	int ret = 0;
	struct fimc_is_device_flite *flite;
	struct fimc_is_device_sensor *device = v4l2_get_subdev_hostdata(subdev);

	FIMC_BUG(!device);
	FIMC_BUG(!subdev);
	FIMC_BUG(!framemgr);

	flite = v4l2_get_subdevdata(subdev);
	if (!flite) {
		err("flite is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	flite->group = 0;
	flite->framemgr = framemgr;
	atomic_set(&flite->fcount, 0);

	clear_bit(FLITE_JOIN_ISCHAIN, &flite->state);
	clear_bit(FLITE_OTF_WITH_3AA, &flite->state);
	clear_bit(FLITE_LAST_CAPTURE, &flite->state);
	clear_bit(FLITE_START_STREAM, &flite->state);

	if (!test_bit(FLITE_DMA_ENABLE, &flite->state))
		goto p_err;

	ret = request_irq(flite->irq,
		fimc_is_isr_flite,
		FIMC_IS_HW_IRQ_FLAG,
		"fimc-lite",
		flite);
	if (ret)
		err("request_irq(FIMC-LITE %d) failed\n", flite->irq);

p_err:
	return ret;
}

int fimc_is_flite_close(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_device_flite *flite;
	struct fimc_is_device_sensor *device = v4l2_get_subdev_hostdata(subdev);

	FIMC_BUG(!device);
	FIMC_BUG(!subdev);

	flite = v4l2_get_subdevdata(subdev);
	if (!flite) {
		err("flite is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (!test_bit(FLITE_DMA_ENABLE, &flite->state))
		goto p_err;

	free_irq(flite->irq, flite);
p_err:
	return ret;
}

/* value : csi ch */
static int flite_init(struct v4l2_subdev *subdev, u32 value)
{
	int ret = 0;
	struct fimc_is_device_flite *flite;

	FIMC_BUG(!subdev);

	flite = v4l2_get_subdevdata(subdev);
	if (!flite) {
		err("flite is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (flite->instance == FLITE_ID_NOTHING) {
		info("can't not link fimc-lite(bns)\n");
		return 0;
	}

	flite->csi = value;

p_err:
	return ret;
}

static int flite_stream_on(struct v4l2_subdev *subdev,
	struct fimc_is_device_flite *flite)
{
	int ret = 0;
	u32 otf_setting, bns;
	bool buffer_ready;
	unsigned long flags;
	struct fimc_is_image *image;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct fimc_is_device_sensor *device = v4l2_get_subdev_hostdata(subdev);

	FIMC_BUG(!flite);
	FIMC_BUG(!flite->framemgr);
	FIMC_BUG(!device);

	if (test_bit(FLITE_START_STREAM, &flite->state)) {
		merr("already start", flite);
		ret = -EINVAL;
		goto p_err;
	}

	otf_setting = 0;
	buffer_ready = false;
	framemgr = flite->framemgr;
	image = &flite->image;
	bns = device->pdata->is_bns;

	flite->overflow_cnt = 0;
	flite->sw_checker = EXPECT_FRAME_START;
	flite->tasklet_param_str = 0;
	flite->tasklet_param_end = 0;
	clear_bit(FLITE_LAST_CAPTURE, &flite->state);

	/* 1. init */
	flite_hw_reset(flite->base_reg);

	if (!test_bit(FLITE_DMA_ENABLE, &flite->state))
		goto p_dma_skip;

	/* 2. dma setting */
	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_13, flags);

	flite_hw_set_dma_addr(flite->base_reg, 0, false, 0);

	if (framemgr->queued_count[FS_REQUEST] >= 1) {
		frame = peek_frame(framemgr, FS_REQUEST);
		if (frame) {
			flite_hw_set_dma_addr(flite->base_reg, 0, true,
					(u32)frame->dvaddr_buffer[0]);
			trans_frame(framemgr, frame, FS_PROCESS);
			buffer_ready = true;
		} else {
			merr("framemgr's state was invalid", device);
			frame_manager_print_queues(framemgr);
			ret = -EINVAL;
		}
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_13, flags);

p_dma_skip:
	flite_hw_set_output_dma(flite->base_reg, buffer_ready);

	/* 3. otf setting */
	if (device->ischain)
		set_bit(FLITE_JOIN_ISCHAIN, &flite->state);
	else
		clear_bit(FLITE_JOIN_ISCHAIN, &flite->state);

	if (device->ischain &&
		test_bit(FIMC_IS_GROUP_OTF_INPUT, &device->ischain->group_3aa.state)) {
		tasklet_init(&flite->tasklet_flite_str, tasklet_flite_str_otf, (unsigned long)subdev);
		tasklet_init(&flite->tasklet_flite_end, tasklet_flite_end, (unsigned long)subdev);

		if (device->ischain->group_3aa.id == GROUP_ID_3AA0) {
			flite->group = GROUP_ID_3AA0;
		} else if (device->ischain->group_3aa.id == GROUP_ID_3AA1) {
			flite->group = GROUP_ID_3AA1;
		} else {
			merr("invalid otf path(%d)", device, device->ischain->group_3aa.id);
			ret = -EINVAL;
			goto p_err;
		}

		mdbgd_back("Enabling OTF path. target 3aa(%d)\n", flite, flite->group);
		flite_hw_set_output_otf(flite->base_reg, true);
		set_bit(FLITE_OTF_WITH_3AA, &flite->state);
	} else {
#if defined(SUPPORTED_EARLYBUF_DONE_SW)
		if (device->early_buf_done_mode)
			device->early_buf_timer.function = flite_early_buf_done;
#endif
		tasklet_init(&flite->tasklet_flite_str, tasklet_flite_str_m2m, (unsigned long)subdev);
		tasklet_init(&flite->tasklet_flite_end, tasklet_flite_end, (unsigned long)subdev);

		flite_hw_set_output_otf(flite->base_reg, true);
		clear_bit(FLITE_OTF_WITH_3AA, &flite->state);
	}

	/* 4. register setting */
	start_fimc_lite(flite->base_reg, image, flite->instance, bns, flite->csi, flite->instance, device);

#ifdef DBG_DUMPREG
	flite_hw_dump(flite->base_reg);
#endif

	set_bit(FLITE_START_STREAM, &flite->state);

p_err:
	return ret;
}

static int flite_stream_off(struct v4l2_subdev *subdev,
	struct fimc_is_device_flite *flite,
	bool nowait)
{
	int ret = 0;
	u32 __iomem *base_reg;
	unsigned long flags;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct fimc_is_device_sensor *device = v4l2_get_subdev_hostdata(subdev);

	FIMC_BUG(!flite);
	FIMC_BUG(!flite->base_reg);
	FIMC_BUG(!flite->framemgr);
	FIMC_BUG(!device);

	if (!test_bit(FLITE_START_STREAM, &flite->state)) {
		merr("already stop", flite);
		ret = -EINVAL;
		goto p_err;
	}

	base_reg = flite->base_reg;
	framemgr = flite->framemgr;

	/* for preventing invalid memory access */
	flite_hw_set_dma_addr(base_reg, 0, false, 0);
	flite_hw_set_output_dma(base_reg, false);
	flite_hw_set_output_otf(base_reg, false);

	/* stop fimc-bns(lite) */
	flite_hw_disable(base_reg);
#if defined(SUPPORTED_EARLYBUF_DONE_SW)
	if (device->early_buf_done_mode) {
		info("early buffer done flushed..\n");
		hrtimer_cancel(&device->early_buf_timer);
	}
#endif

	if (!nowait) {
		ulong timetowait;

		timetowait = wait_event_timeout(flite->wait_queue,
			test_bit(FLITE_LAST_CAPTURE, &flite->state),
			FIMC_IS_FLITE_STOP_TIMEOUT);
		if (!timetowait) {
			/* forcely stop */
			flite_hw_disable(base_reg);
			set_bit(FLITE_LAST_CAPTURE, &flite->state);
			err("last capture timeout:%s", __func__);
			msleep(200);
			flite_hw_reset(base_reg);
			ret = -ETIME;
		}
	} else {
		/*
		 * DTP test can make iommu fault because senosr is streaming
		 * therefore it need  force reset
		 */
		flite_hw_reset(base_reg);
	}
	/* clr interrupt source */
	flite_hw_set_interrupt_mask(flite->base_reg, false, ((1 << FLITE_MASK_IRQ_ALL) - 1));

	if (!test_bit(FLITE_DMA_ENABLE, &flite->state))
		goto dma_skip;

	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_14, flags);

	frame = peek_frame(framemgr, FS_PROCESS);
	while (frame) {
		CALL_VOPS(device->vctx, done, frame->index, VB2_BUF_STATE_ERROR);
		trans_frame(framemgr, frame, FS_COMPLETE);
		frame = peek_frame(framemgr, FS_PROCESS);
	}

	frame = peek_frame(framemgr, FS_REQUEST);
	while (frame) {
		CALL_VOPS(device->vctx, done, frame->index, VB2_BUF_STATE_ERROR);
		trans_frame(framemgr, frame, FS_COMPLETE);
		frame = peek_frame(framemgr, FS_REQUEST);
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_14, flags);

dma_skip:
	clear_bit(FLITE_START_STREAM, &flite->state);

p_err:
	return ret;
}

/*
 * enable
 * @X0 : disable
 * @X1 : enable
 * @1X : no waiting flag
 * @0X : waiting flag
 */
static int flite_s_stream(struct v4l2_subdev *subdev, int enable)
{
	int ret = 0;
	bool nowait;
	struct fimc_is_device_flite *flite;

	FIMC_BUG(!subdev);

	nowait = (enable & FLITE_NOWAIT_MASK) >> FLITE_NOWAIT_SHIFT;
	enable = enable & FLITE_ENABLE_MASK;

	flite = (struct fimc_is_device_flite *)v4l2_get_subdevdata(subdev);
	if (!flite) {
		err("flite is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (flite->instance == FLITE_ID_NOTHING) {
		info("can't not link fimc-lite(bns)\n");
		return 0;
	}

	/* H/W setting skip */
	if (test_bit(FLITE_DUMMY, &flite->state)) {
		mdbgd_back("%s(dummy)\n", flite, __func__);
		goto p_err;
	}

	if (enable) {
		ret = flite_stream_on(subdev, flite);
		if (ret) {
			err("flite_stream_on is fail(%d)", ret);
			goto p_err;
		}
	} else {
		ret = flite_stream_off(subdev, flite, nowait);
		if (ret) {
			err("flite_stream_off is fail(%d)", ret);
			goto p_err;
		}
	}

p_err:
	mdbgd_back("%s(%d, %d)\n", flite, __func__, enable, ret);
	return 0;
}

static int flite_s_format(struct v4l2_subdev *subdev,
	struct v4l2_subdev_pad_config *cfg,
	struct v4l2_subdev_format *fmt)
{
	int ret = 0;
	struct fimc_is_device_flite *flite;

	FIMC_BUG(!subdev);
	FIMC_BUG(!fmt);

	flite = (struct fimc_is_device_flite *)v4l2_get_subdevdata(subdev);
	if (!flite) {
		err("flite is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (flite->instance == FLITE_ID_NOTHING) {
		info("can't not link fimc-lite(bns)\n");
		return 0;
	}

	flite->image.window.offs_h = 0;
	flite->image.window.offs_v = 0;
	flite->image.window.width = fmt->format.width;
	flite->image.window.height = fmt->format.height;
	flite->image.window.o_width = fmt->format.width;
	flite->image.window.o_height = fmt->format.height;
	flite->image.format.pixelformat = fmt->format.code;

p_err:
	mdbgd_back("%s(%dx%d, %X)\n", flite, __func__, fmt->format.width,
			fmt->format.height, fmt->format.code);
	return ret;
}

static int flite_s_buffer(struct v4l2_subdev *subdev, void *buf, unsigned int *size)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_device_flite *flite;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;

	FIMC_BUG(!subdev);

	flite = (struct fimc_is_device_flite *)v4l2_get_subdevdata(subdev);
	if (unlikely(flite == NULL)) {
		err("flite is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (!test_bit(FLITE_DMA_ENABLE, &flite->state))
		goto p_err;

	framemgr = flite->framemgr;
	if (unlikely(framemgr == NULL)) {
		merr("framemgr is NULL", flite);
		ret = -EINVAL;
		goto p_err;
	}

	if (test_bit(FLITE_START_STREAM, &flite->state)) {
		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_15, flags);

		frame = peek_frame(framemgr, FS_REQUEST);
		if (frame) {
			if (!flite_hw_get_output_dma(flite->base_reg)) {
				flite_hw_set_dma_addr(flite->base_reg, 0, true,
						(u32)frame->dvaddr_buffer[0]);
				trans_frame(framemgr, frame, FS_PROCESS);
			}
		}

		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_15, flags);
	}

p_err:
	return ret;
}

static int flite_s_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;
	struct fimc_is_device_flite *flite;

	FIMC_BUG(!subdev);
	FIMC_BUG(!ctrl);

	flite = (struct fimc_is_device_flite *)v4l2_get_subdevdata(subdev);
	if (!flite) {
		err("flite is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (flite->instance == FLITE_ID_NOTHING) {
		info("can't not link fimc-lite(bns)\n");
		return 0;
	}

	switch (ctrl->id) {
	case V4L2_CID_IS_S_BNS:
		{
			u32 width, height, ratio;

			width = flite->image.window.width;
			height = flite->image.window.height;
			ratio = ctrl->value;

			flite->image.window.otf_width
				= rounddown((width * 1000 / ratio), 4);
			flite->image.window.otf_height
				= rounddown((height * 1000 / ratio), 2);
		}
		break;
	default:
		err("unsupported ioctl(%d)\n", ctrl->id);
		ret = -EINVAL;
		break;
	}

p_err:
	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = flite_init,
	.s_ctrl = flite_s_ctrl,
};

static const struct v4l2_subdev_video_ops video_ops = {
	.s_stream = flite_s_stream,
	.s_rx_buffer= flite_s_buffer
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = flite_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

int fimc_is_flite_probe(struct fimc_is_device_sensor *device,
	u32 instance)
{
	int ret = 0;
	struct v4l2_subdev *subdev_flite;
	struct fimc_is_device_flite *flite;
	struct resource *mem_res;
	struct platform_device *pdev;

	FIMC_BUG(!device);

	subdev_flite = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_flite) {
		merr("subdev_flite is NULL", device);
		ret = -ENOMEM;
		goto err_alloc_subdev_flite;
	}
	device->subdev_flite = subdev_flite;

	flite = kzalloc(sizeof(struct fimc_is_device_flite), GFP_KERNEL);
	if (!flite) {
		merr("flite is NULL", device);
		ret = -ENOMEM;
		goto err_alloc_flite;
	}

	pdev = device->pdev;
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 10);
	if (!mem_res) {
		probe_err("Failed to get io memory region(%p)", mem_res);
		ret = -EBUSY;
		goto err_get_resource;
	}

	flite->regs_start = mem_res->start;
	flite->regs_end = mem_res->end;
	flite->base_reg =  devm_ioremap_nocache(&pdev->dev, mem_res->start, resource_size(mem_res));
	if (!flite->base_reg) {
		probe_err("Failed to remap io region(%p)", flite->base_reg);
		ret = -ENOMEM;
		goto err_ioremap;
	}

	flite->irq = platform_get_irq(pdev, 1);
	if (flite->irq < 0) {
		probe_err("Failed to get flite->irq(%d)", flite->irq);
		ret = -EBUSY;
		goto err_get_irq;
	}

	/* pointer to me from device sensor */
	flite->subdev = &device->subdev_flite;
	flite->instance = instance;
	init_waitqueue_head(&flite->wait_queue);

	/* default state setting */
	clear_bit(FLITE_DUMMY, &flite->state);
	set_bit(FLITE_DMA_ENABLE, &flite->state);

	v4l2_subdev_init(subdev_flite, &subdev_ops);
	v4l2_set_subdevdata(subdev_flite, (void *)flite);
	v4l2_set_subdev_hostdata(subdev_flite, device);
	snprintf(subdev_flite->name, V4L2_SUBDEV_NAME_SIZE, "flite-subdev.%d", instance);
	ret = v4l2_device_register_subdev(&device->v4l2_dev, subdev_flite);
	if (ret) {
		merr("v4l2_device_register_subdev is fail(%d)", device, ret);
		goto err_reg_v4l2_subdev;
	}

	info("[%d][BAK:D] %s(%d)\n", instance, __func__, ret);
	return 0;

err_reg_v4l2_subdev:
err_get_irq:
	iounmap(flite->base_reg);

err_ioremap:
err_get_resource:
	kfree(flite);

err_alloc_flite:
	kfree(subdev_flite);
	device->subdev_flite = NULL;

err_alloc_subdev_flite:
	err("[BAK:D:%d] %s(%d)\n", instance, __func__, ret);
	return ret;
}
