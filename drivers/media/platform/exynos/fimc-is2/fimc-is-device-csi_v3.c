/*
 * Samsung Exynos SoC series FIMC-IS driver
 *
 * exynos fimc-is/mipi-csi functions
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/io.h>
#include <linux/phy/phy.h>

#include "fimc-is-debug.h"
#include "fimc-is-config.h"
#include "fimc-is-core.h"
#include "fimc-is-regs.h"
#include "fimc-is-device-csi.h"
#include "fimc-is-device-sensor.h"

extern int debug_csi;

static void csis_flush_vc_buf_done(struct fimc_is_device_csi *csi, u32 vc,
		enum fimc_is_frame_state target,
		enum vb2_buffer_state state);
static void csis_flush_vc_multibuf(struct fimc_is_device_csi *csi, u32 vc);

#if defined(SUPPORTED_EARLYBUF_DONE_SW)
static enum hrtimer_restart csis_early_buf_done(struct hrtimer *timer)
{
	struct fimc_is_device_sensor *device = container_of(timer, struct fimc_is_device_sensor, early_buf_timer);
	struct fimc_is_device_csi *csi;

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(device->subdev_csi);
	csi->sw_checker = EXPECT_FRAME_START;
	tasklet_schedule(&csi->tasklet_csis_end);

	return HRTIMER_NORESTART;
}
#endif

static inline void csi_frame_start_inline(struct fimc_is_device_csi *csi)
{
	/* frame start interrupt */
	csi->sw_checker = EXPECT_FRAME_END;
	atomic_inc(&csi->fcount);
	dbg_csiisr("<%d %d ", csi->instance,
			atomic_read(&csi->fcount));
	atomic_inc(&csi->vvalid);
	{
		u32 vsync_cnt = atomic_read(&csi->fcount);
		v4l2_subdev_notify(*csi->subdev, CSI_NOTIFY_VSYNC, &vsync_cnt);
	}

	tasklet_schedule(&csi->tasklet_csis_str);
}

static inline void csi_frame_line_inline(struct fimc_is_device_csi *csi)
{
	dbg_csiisr("-%d %d-", csi->instance,
			atomic_read(&csi->fcount));
	/* frame line interrupt */
	tasklet_schedule(&csi->tasklet_csis_line);
}

static inline void csi_frame_end_inline(struct fimc_is_device_csi *csi)
{
	dbg_csiisr("%d %d>", csi->instance,
			atomic_read(&csi->fcount));
	/* frame end interrupt */
	csi->sw_checker = EXPECT_FRAME_START;
	atomic_dec(&csi->vvalid);
	{
		u32 vsync_cnt = atomic_read(&csi->fcount);
		atomic_set(&csi->vblank_count, vsync_cnt);
		v4l2_subdev_notify(*csi->subdev, CSI_NOTIFY_VBLANK, &vsync_cnt);
	}

	tasklet_schedule(&csi->tasklet_csis_end);
}

static inline void csi_s_config_dma(struct fimc_is_device_csi *csi, struct fimc_is_vci_config *vci_config)
{
	int vc = 0;
	struct fimc_is_subdev *dma_subdev = NULL;
	struct fimc_is_queue *queue;
	struct fimc_is_frame_cfg framecfg = {0};
	struct fimc_is_fmt fmt;

	for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++) {
		dma_subdev = csi->dma_subdev[vc];

		if (!dma_subdev ||
			!test_bit(FIMC_IS_SUBDEV_OPEN, &dma_subdev->state))
			continue;

		if (test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &dma_subdev->state)) {
			/* set from internal subdev setting */
			fmt.pixelformat = dma_subdev->pixelformat;
			framecfg.format = &fmt;
			framecfg.width = dma_subdev->output.width;
		} else {
			/* cpy format from vc video context */
			queue = GET_SUBDEV_QUEUE(dma_subdev);
			framecfg = queue->framecfg;
		}

		csi_hw_s_config_dma(csi->base_reg, vc, &framecfg, vci_config[vc].hwformat);
	}
}

static inline void csi_s_buf_addr(struct fimc_is_device_csi *csi, struct fimc_is_frame *frame, u32 index, u32 vc)
{
	FIMC_BUG(!frame);

	csi_hw_s_dma_addr(csi->base_reg, vc, index,
				(u32)frame->dvaddr_buffer[0]);
}

static inline void csi_s_multibuf_addr(struct fimc_is_device_csi *csi, struct fimc_is_frame *frame, u32 index, u32 vc)
{
	FIMC_BUG(!frame);

	csi_hw_s_multibuf_dma_addr(csi->base_reg, vc, index,
				(u32)frame->dvaddr_buffer[0]);
}

static inline void csi_s_output_dma(struct fimc_is_device_csi *csi, u32 vc, bool enable)
{
	csi_hw_s_output_dma(csi->base_reg, vc, enable);
}

static inline void csi_s_frameptr(struct fimc_is_device_csi *csi, u32 vc, u32 number, bool clear)
{
	csi_hw_s_frameptr(csi->base_reg, vc, number, clear);
}

#ifdef SUPPORTED_EARLYBUF_DONE_SW
static void csis_early_buf_done_start(struct v4l2_subdev *subdev)
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
		int msec_timeout = ((1000 / framerate) *
				(100 - (5 * device->early_buf_done_mode))) / 100;
		if (msec_timeout > 0)
			hrtimer_start(&device->early_buf_timer,
					ktime_set(0, msec_timeout * NSEC_PER_MSEC), HRTIMER_MODE_REL);
	}
}
#endif

static struct fimc_is_framemgr *csis_get_vc_framemgr(struct fimc_is_device_csi *csi, u32 vc)
{
	struct fimc_is_subdev *dma_subdev;
	struct fimc_is_framemgr *framemgr = NULL;

	if (vc >= CSI_VIRTUAL_CH_MAX) {
		err("VC(%d of %d) is out-of-range", vc, CSI_VIRTUAL_CH_MAX);
		return NULL;
	}

	dma_subdev = csi->dma_subdev[vc];
	if (!dma_subdev ||
			!test_bit(FIMC_IS_SUBDEV_START, &dma_subdev->state)) {
		return NULL;
	}

	framemgr = GET_SUBDEV_FRAMEMGR(dma_subdev);

	return framemgr;
}

static void csis_s_vc_dma_multibuf(struct fimc_is_device_csi *csi)
{
	u32 vc;
	int i;
	struct fimc_is_subdev *dma_subdev;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;

	/* dma setting for several virtual ch 1 ~ 3 */
	for (vc = CSI_VIRTUAL_CH_1; vc < CSI_VIRTUAL_CH_MAX; vc++) {
		dma_subdev = csi->dma_subdev[vc];
		if (!dma_subdev
			|| (!test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &dma_subdev->state))
			|| (!test_bit(FIMC_IS_SUBDEV_START, &dma_subdev->state)))
			continue;

		framemgr = GET_SUBDEV_FRAMEMGR(dma_subdev);

		FIMC_BUG(!framemgr);

		/* If error happened, return all processing frame to free */
		if (test_bit((CSIS_BUF_ERR_VC0 + vc), &csi->state)) {
			csis_flush_vc_multibuf(csi, vc);
			err("[F%d][VC%d] frame was done with error", atomic_read(&csi->fcount), vc);
			clear_bit((CSIS_BUF_ERR_VC0 + vc), &csi->state);
		}

		if (test_bit((CSIS_SET_MULTIBUF_VC1 + (vc - 1)), &csi->state))
			continue;

		framemgr_e_barrier(framemgr, 0);
		for (i = 0; i < framemgr->num_frames; i++) {
			frame = &framemgr->frames[i];
			csi_s_multibuf_addr(csi, frame, i, vc);
			csi_s_output_dma(csi, vc, true);
			trans_frame(framemgr, frame, FS_FREE);
		}

		framemgr_x_barrier(framemgr, 0);

		set_bit((CSIS_SET_MULTIBUF_VC1 + (vc - 1)), &csi->state);
	}
}

static void csis_disable_all_vc_dma_buf(struct fimc_is_device_csi *csi)
{
	u32 vc;
	int cur_dma_enable;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_subdev *dma_subdev;

	/* default disable dma setting for several virtual ch 0 ~ 3 */
	for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++) {
		/* skip for internal vc use */
		if(csi->dma_subdev[vc]
			&& test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &csi->dma_subdev[vc]->state))
			continue;

		/* default dma disable */
		csi_s_output_dma(csi, vc, false);

		/* print infomation DMA on/off */
		cur_dma_enable = csi_hw_g_output_cur_dma_enable(csi->base_reg, vc);

		if (test_bit(CSIS_START_STREAM, &csi->state) &&
			csi->pre_dma_enable[vc] != cur_dma_enable) {

			dma_subdev = csi->dma_subdev[vc];
			framemgr = GET_SUBDEV_FRAMEMGR(dma_subdev);

			if (framemgr)
				info("[VC%d][F%d] DMA %s [%d/%d/%d]\n", vc, atomic_read(&csi->fcount),
						(cur_dma_enable ? "on" : "off"),
						framemgr->queued_count[FS_REQUEST],
						framemgr->queued_count[FS_PROCESS],
						framemgr->queued_count[FS_COMPLETE]);

			csi->pre_dma_enable[vc] = cur_dma_enable;
		}
	}
}

static void csis_flush_vc_buf_done(struct fimc_is_device_csi *csi, u32 vc,
		enum fimc_is_frame_state target,
		enum vb2_buffer_state state)
{
	struct fimc_is_device_sensor *device;
	struct fimc_is_subdev *dma_subdev;
	struct fimc_is_framemgr *ldr_framemgr;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *ldr_frame;
	struct fimc_is_frame *frame;
	struct fimc_is_video_ctx *vctx;
	u32 findex;

	device = container_of(csi->subdev, struct fimc_is_device_sensor, subdev_csi);

	FIMC_BUG(!device);

	/* buffer done for several virtual ch 0 ~ 3, internal vc is skipped */
	dma_subdev = csi->dma_subdev[vc];
	if (!dma_subdev
		|| test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &dma_subdev->state)
		|| !test_bit(FIMC_IS_SUBDEV_START, &dma_subdev->state))
		return;

	ldr_framemgr = GET_SUBDEV_FRAMEMGR(dma_subdev->leader);
	framemgr = GET_SUBDEV_FRAMEMGR(dma_subdev);
	vctx = dma_subdev->vctx;

	FIMC_BUG(!ldr_framemgr);
	FIMC_BUG(!framemgr);

	framemgr_e_barrier(framemgr, 0);

	frame = peek_frame(framemgr, target);
	while (frame) {
		if (target == FS_PROCESS) {
			findex = frame->stream->findex;
			ldr_frame = &ldr_framemgr->frames[findex];
			clear_bit(dma_subdev->id, &ldr_frame->out_flag);
		}

		CALL_VOPS(vctx, done, frame->index, state);
		trans_frame(framemgr, frame, FS_COMPLETE);
		frame = peek_frame(framemgr, target);
	}

	framemgr_x_barrier(framemgr, 0);
}

static void csis_flush_vc_multibuf(struct fimc_is_device_csi *csi, u32 vc)
{
	int i;
	struct fimc_is_subdev *subdev;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;

	subdev = csi->dma_subdev[vc];

	if (!subdev
		|| !test_bit(FIMC_IS_SUBDEV_START, &subdev->state)
		|| !test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &subdev->state))
		return;

	framemgr = GET_SUBDEV_FRAMEMGR(subdev);
	if (framemgr) {
		framemgr_e_barrier(framemgr, 0);
		for (i = 0; i < framemgr->num_frames; i++) {
			frame = &framemgr->frames[i];

			if (frame->state == FS_PROCESS
				|| frame->state == FS_COMPLETE) {
				trans_frame(framemgr, frame, FS_FREE);
			}
		}
		framemgr_x_barrier(framemgr, 0);
	}

	clear_bit((CSIS_SET_MULTIBUF_VC1 + (vc - 1)), &csi->state);
}

static void csis_flush_all_vc_buf_done(struct fimc_is_device_csi *csi, u32 state)
{
	u32 i;

	/* buffer done for several virtual ch 0 ~ 3 */
	for (i = CSI_VIRTUAL_CH_0; i < CSI_VIRTUAL_CH_MAX; i++) {
		csis_flush_vc_buf_done(csi, i, FS_PROCESS, state);
		csis_flush_vc_buf_done(csi, i, FS_REQUEST, state);

		csis_flush_vc_multibuf(csi, i);
	}
}

/*
 * Tasklet for handling of start/end interrupt bottom-half
 *
 * tasklet_csis_str_otf : In case of OTF mode (csi->)
 * tasklet_csis_str_m2m : In case of M2M mode (csi->)
 * tasklet_csis_end     : In case of OTF or M2M mode (csi->)
 */
static u32 g_print_cnt;
void tasklet_csis_str_otf(unsigned long data)
{
	struct v4l2_subdev *subdev;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_sensor *device;
	struct fimc_is_device_ischain *ischain;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group_sensor, *group_3aa, *group_isp;
	u32 fcount, group_sensor_id, group_3aa_id, group_isp_id;

	subdev = (struct v4l2_subdev *)data;
	csi = v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		BUG();
	}

	device = v4l2_get_subdev_hostdata(subdev);
	if (!device) {
		err("device is NULL");
		BUG();
	}

	fcount = atomic_read(&csi->fcount);
	ischain = device->ischain;

#ifdef TASKLET_MSG
	pr_info("S%d\n", fcount);
#endif

	groupmgr = ischain->groupmgr;
	group_sensor = &device->group_sensor;
	group_3aa = &ischain->group_3aa;
	group_isp = &ischain->group_isp;

	group_sensor_id = group_sensor->id;
	group_3aa_id = group_3aa->id;
	group_isp_id = group_isp->id;

	if (!test_bit(FIMC_IS_SENSOR_STAND_ALONE, &device->state)) {
		if (group_3aa_id >= GROUP_ID_MAX) {
			merr("group 3aa id is invalid(%d)", csi, group_3aa_id);
			goto trigger_skip;
		}

		if (group_isp_id >= GROUP_ID_MAX) {
			merr("group isp id is invalid(%d)", csi, group_isp_id);
			goto trigger_skip;
		}
	}

	if (unlikely(list_empty(&group_sensor->smp_trigger.wait_list))) {
		atomic_set(&group_sensor->sensor_fcount, fcount + group_sensor->skip_shots);

		/*
		 * pcount : program count
		 * current program count(location) in kthread
		 */
		if (((g_print_cnt % LOG_INTERVAL_OF_DROPS) == 0) ||
			(g_print_cnt < LOG_INTERVAL_OF_DROPS)) {
			if (!test_bit(FIMC_IS_SENSOR_STAND_ALONE, &device->state)) {
				info("[CSI] GP%d(res %d, rcnt %d, scnt %d), "
						"GP%d(res %d, rcnt %d, scnt %d), "
						"fcount %d pcount %d\n",
						group_sensor_id,
						groupmgr->gtask[group_sensor_id].smp_resource.count,
						atomic_read(&group_sensor->rcount),
						atomic_read(&group_sensor->scount),
						group_isp_id,
						groupmgr->gtask[group_isp_id].smp_resource.count,
						atomic_read(&group_isp->rcount),
						atomic_read(&group_isp->scount),
						fcount + group_sensor->skip_shots,
						group_sensor->pcount);
			} else {
				info("[CSI] GP%d(res %d, rcnt %d, scnt %d), "
						"fcount %d pcount %d\n",
						group_sensor_id,
						groupmgr->gtask[group_sensor_id].smp_resource.count,
						atomic_read(&group_sensor->rcount),
						atomic_read(&group_sensor->scount),
						fcount + group_sensor->skip_shots,
						group_sensor->pcount);
			}
		}
		g_print_cnt++;
	} else {
		g_print_cnt = 0;
		if (fcount + group_sensor->skip_shots > atomic_read(&group_sensor->backup_fcount)) {
			atomic_set(&group_sensor->sensor_fcount, fcount + group_sensor->skip_shots);
			up(&group_sensor->smp_trigger);
		}
	}

trigger_skip:
	/* disable all virtual channel's dma */
	csis_disable_all_vc_dma_buf(csi);
	/* re-set internal vc dma if flushed */
	csis_s_vc_dma_multibuf(csi);

#if defined(SUPPORTED_EARLYBUF_DONE_SW)
	csis_early_buf_done_start(subdev);
#endif
	v4l2_subdev_notify(subdev, CSIS_NOTIFY_FSTART, &fcount);
	clear_bit(FIMC_IS_SENSOR_WAIT_STREAMING, &device->state);
}

void tasklet_csis_str_m2m(unsigned long data)
{
	struct v4l2_subdev *subdev;
	struct fimc_is_device_csi *csi;
	u32 fcount;

	subdev = (struct v4l2_subdev *)data;
	csi = v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("[CSI] csi is NULL");
		BUG();
	}

	fcount = atomic_read(&csi->fcount);

#ifdef TASKLET_MSG
	pr_info("S%d\n", fcount);
#endif
	/* disable all virtual channel's dma */
	csis_disable_all_vc_dma_buf(csi);
	/* re-set internal vc dma if flushed */
	csis_s_vc_dma_multibuf(csi);

#if defined(SUPPORTED_EARLYBUF_DONE_SW)
	csis_early_buf_done_start(subdev);
#endif
	v4l2_subdev_notify(subdev, CSIS_NOTIFY_FSTART, &fcount);
}

static void csi_dma_tag(struct v4l2_subdev *subdev,
	struct fimc_is_device_csi *csi,
	struct fimc_is_framemgr *framemgr, u32 vc)
{
	u32 findex;
	u32 done_state = 0;
	struct fimc_is_subdev *f_subdev;
	struct fimc_is_framemgr *ldr_framemgr;
	struct fimc_is_video_ctx *vctx = NULL;
	struct fimc_is_frame *ldr_frame;
	struct fimc_is_frame *frame = NULL;
	struct fimc_is_frame *frame_done = NULL;
	int i = 0;

	framemgr_e_barrier(framemgr, 0);

	frame = peek_frame(framemgr, FS_PROCESS);
	if (frame) {
		frame_done = frame;
		trans_frame(framemgr, frame, FS_COMPLETE);

		/* get subdev and video context */
		f_subdev = frame->subdev;
		FIMC_BUG(!f_subdev);

		vctx = f_subdev->vctx;
		FIMC_BUG(!vctx);

		/* get the leader's framemgr */
		ldr_framemgr = GET_SUBDEV_FRAMEMGR(f_subdev->leader);
		FIMC_BUG(!ldr_framemgr);

		findex = frame->stream->findex;
		ldr_frame = &ldr_framemgr->frames[findex];
		clear_bit(f_subdev->id, &ldr_frame->out_flag);

		/* for debug */
		for (i = 0; i < frame->num_buffers; i++)
			DBG_DIGIT_TAG(GROUP_SLOT_MAX, 0, GET_QUEUE(vctx), frame,
				frame->fcount - frame->num_buffers + i, i);

		done_state = (frame->result) ? VB2_BUF_STATE_ERROR : VB2_BUF_STATE_DONE;
		if (frame->result)
			msrinfo("[ERR] NDONE(%d, E%X)\n", f_subdev, f_subdev, ldr_frame, frame->index, frame->result);
		else
			msrdbgs(1, " DONE(%d)\n", f_subdev, f_subdev, ldr_frame, frame->index);

		CALL_VOPS(vctx, done, frame->index, done_state);
	}

	framemgr_x_barrier(framemgr, 0);

	v4l2_subdev_notify(subdev, CSIS_NOTIFY_DMA_END, frame_done);
}

static void csi_err_check(struct fimc_is_device_csi *csi, u32 *err_id)
{
	int vc, err, err_flag = 0;

	/* 1. Check error */
	for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++)
		err_flag |= csi->error_id[vc];

	/* 2. If err occurs first in 1 frame, request DMA abort */
	if (!err_flag)
		csi_hw_s_control(csi->base_reg, CSIS_CTRL_DMA_ABORT_REQ, true);

	/* 3. Cumulative error */
	for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++)
		csi->error_id[vc] |= err_id[vc];

	/* 4. VC ch0 only exception case */
	err = find_first_bit((unsigned long *)&err_id[CSI_VIRTUAL_CH_0], CSIS_ERR_END);
	while (err < CSIS_ERR_END) {
		switch (err) {
		case CSIS_ERR_LOST_FE_VC:
			/* 1. disable next dma */
			csi_s_output_dma(csi, CSI_VIRTUAL_CH_0, false);
			/* 2. schedule the end tasklet */
			csi_frame_end_inline(csi);
			/* 3. increase the sensor fcount */
			/* 4. schedule the start tasklet */
			csi_frame_start_inline(csi);
			break;
		case CSIS_ERR_LOST_FS_VC:
			/* 1. disable next dma */
			csi_s_output_dma(csi, CSI_VIRTUAL_CH_0, false);
			/* 2. increase the sensor fcount */
			/* 3. schedule the start tasklet */
			csi_frame_start_inline(csi);
			/* 4. schedule the end tasklet */
			csi_frame_end_inline(csi);
			break;
		default:
			break;
		}

		/* Check next bit */
		err = find_next_bit((unsigned long *)&err_id[CSI_VIRTUAL_CH_0], CSIS_ERR_END, err + 1);
	}
}

static void csi_err_print(struct fimc_is_device_csi *csi)
{
	const char* err_str = NULL;
	int vc, err;

	for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++) {
		/* Skip error handling if there's no error in this virtual ch. */
		if (!csi->error_id[vc])
			continue;

		err = find_first_bit((unsigned long *)&csi->error_id[vc], CSIS_ERR_END);
		while (err < CSIS_ERR_END) {
			switch (err) {
			case CSIS_ERR_ID:
				err_str = GET_STR(CSIS_ERR_ID);
				break;
			case CSIS_ERR_CRC:
				err_str = GET_STR(CSIS_ERR_CRC);
				break;
			case CSIS_ERR_ECC:
				err_str = GET_STR(CSIS_ERR_ECC);
				break;
			case CSIS_ERR_WRONG_CFG:
				err_str = GET_STR(CSIS_ERR_WRONG_CFG);
				break;
			case CSIS_ERR_OVERFLOW_VC:
				err_str = GET_STR(CSIS_ERR_OVERFLOW_VC);
				break;
			case CSIS_ERR_LOST_FE_VC:
				err_str = GET_STR(CSIS_ERR_LOST_FE_VC);
				break;
			case CSIS_ERR_LOST_FS_VC:
				err_str = GET_STR(CSIS_ERR_LOST_FS_VC);
				break;
			case CSIS_ERR_SOT_VC:
				err_str = GET_STR(CSIS_ERR_SOT_VC);
				break;
			case CSIS_ERR_OTF_OVERLAP:
				err_str = GET_STR(CSIS_ERR_OTF_OVERLAP);
				break;
			case CSIS_ERR_DMA_ERR_DMAFIFO_FULL:
				fimc_is_debug_event_count(FIMC_IS_EVENT_OVERFLOW_CSI);
				err_str = GET_STR(CSIS_ERR_DMA_ERR_DMAFIFO_FULL);
#ifdef OVERFLOW_PANIC_ENABLE_CSIS
#ifdef USE_CAMERA_HW_BIG_DATA
				fimc_is_vender_csi_err_handler(csi);
				fimc_is_sec_copy_err_cnt_to_file();
#endif
				panic("CSIS error!! %s", err_str);
#endif
				break;
			case CSIS_ERR_DMA_ERR_TRXFIFO_FULL:
				fimc_is_debug_event_count(FIMC_IS_EVENT_OVERFLOW_CSI);
				err_str = GET_STR(CSIS_ERR_DMA_ERR_TRXFIFO_FULL);
#ifdef OVERFLOW_PANIC_ENABLE_CSIS
#ifdef USE_CAMERA_HW_BIG_DATA
				fimc_is_vender_csi_err_handler(csi);
				fimc_is_sec_copy_err_cnt_to_file();
#endif
				panic("CSIS error!! %s", err_str);
#endif
				break;
			case CSIS_ERR_DMA_ERR_BRESP_ERR:
				fimc_is_debug_event_count(FIMC_IS_EVENT_OVERFLOW_CSI);
				err_str = GET_STR(CSIS_ERR_DMA_ERR_BRESP_ERR);
#ifdef OVERFLOW_PANIC_ENABLE_CSIS
#ifdef USE_CAMERA_HW_BIG_DATA
				fimc_is_vender_csi_err_handler(csi);
				fimc_is_sec_copy_err_cnt_to_file();
#endif
				panic("CSIS error!! %s", err_str);
#endif
				break;
			case CSIS_ERR_DMA_ABORT_DONE:
				err_str = GET_STR(CSIS_ERR_DMA_ABORT_DONE);
				break;
			}

			/* Print error log */
			merr("[VC%d][F%d] Occured the %s(ID %d)", csi, vc, atomic_read(&csi->fcount), err_str, err);

			/* Check next bit */
			err = find_next_bit((unsigned long *)&csi->error_id[vc], CSIS_ERR_END, err + 1);
		}
	}

#ifdef USE_CAMERA_HW_BIG_DATA
	if (err_str)
		fimc_is_vender_csi_err_handler(csi);
#endif
}

static void csi_err_handle(struct fimc_is_device_csi *csi)
{
	struct fimc_is_core *core;
	struct fimc_is_device_sensor *device;
	struct fimc_is_subdev *dma_subdev;
	int vc;

	/* 1. Print error & Clear error status */
	csi_err_print(csi);

	for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++) {
		/* 2. Clear err */
		csi->error_id[vc] = 0;

		/* 3. Frame flush */
		dma_subdev = csi->dma_subdev[vc];
		if (dma_subdev && test_bit(FIMC_IS_SUBDEV_OPEN, &dma_subdev->state)) {
			csis_flush_vc_buf_done(csi, vc, FS_PROCESS, VB2_BUF_STATE_ERROR);
			csis_flush_vc_multibuf(csi, vc);
			err("[F%d][VC%d] frame was done with error", atomic_read(&csi->fcount), vc);
			set_bit((CSIS_BUF_ERR_VC0 + vc), &csi->state);
		}
	}

	/* 4. Add error count */
	csi->error_count++;

	/* 5. Call sensor DTP if Err count is more than a certain amount */
	if (csi->error_count >= CSI_ERR_COUNT) {
		device = container_of(csi->subdev, struct fimc_is_device_sensor, subdev_csi);
		core = device->private_data;

		/* Call sensor DTP */
		set_bit(FIMC_IS_SENSOR_FRONT_DTP_STOP, &device->state);

		/* Disable CSIS */
		csi_hw_disable(csi->base_reg);

		/* CSIS register dump */
		csi_hw_dump(csi->base_reg);

		/* CLK dump */
		CALL_POPS(core, print_clk);
	}
}

static void wq_csis_dma_vc0(struct work_struct *data)
{
	struct fimc_is_device_csi *csi;
	struct fimc_is_framemgr *framemgr;

	csi = container_of(data, struct fimc_is_device_csi, wq_csis_dma[CSI_VIRTUAL_CH_0]);
	if (!csi) {
		err("[CSI] csi is NULL");
		BUG();
	}

	framemgr = csis_get_vc_framemgr(csi, CSI_VIRTUAL_CH_0);
	if (framemgr)
		csi_dma_tag(*csi->subdev, csi, framemgr, CSI_VIRTUAL_CH_0);
}

static void wq_csis_dma_vc1(struct work_struct *data)
{
	struct fimc_is_device_csi *csi;
	struct fimc_is_framemgr *framemgr;

	csi = container_of(data, struct fimc_is_device_csi, wq_csis_dma[CSI_VIRTUAL_CH_1]);
	if (!csi) {
		err("[CSI] csi is NULL");
		BUG();
	}

	framemgr = csis_get_vc_framemgr(csi, CSI_VIRTUAL_CH_1);
	if (framemgr) {
		if (!test_bit(FIMC_IS_SUBDEV_INTERNAL_USE,
			&csi->dma_subdev[CSI_VIRTUAL_CH_1]->state))
			csi_dma_tag(*csi->subdev, csi, framemgr, CSI_VIRTUAL_CH_1);
	}
}

static void wq_csis_dma_vc2(struct work_struct *data)
{
	struct fimc_is_device_csi *csi;
	struct fimc_is_framemgr *framemgr;

	csi = container_of(data, struct fimc_is_device_csi, wq_csis_dma[CSI_VIRTUAL_CH_2]);
	if (!csi) {
		err("[CSI] csi is NULL");
		BUG();
	}

	framemgr = csis_get_vc_framemgr(csi, CSI_VIRTUAL_CH_2);
	if (framemgr) {
		if (!test_bit(FIMC_IS_SUBDEV_INTERNAL_USE,
			&csi->dma_subdev[CSI_VIRTUAL_CH_2]->state))
			csi_dma_tag(*csi->subdev, csi, framemgr, CSI_VIRTUAL_CH_2);
	}
}

static void wq_csis_dma_vc3(struct work_struct *data)
{
	struct fimc_is_device_csi *csi;
	struct fimc_is_framemgr *framemgr = NULL;

	csi = container_of(data, struct fimc_is_device_csi, wq_csis_dma[CSI_VIRTUAL_CH_2]);
	if (!csi) {
		err("[CSI] csi is NULL");
		BUG();
	}

	framemgr = csis_get_vc_framemgr(csi, CSI_VIRTUAL_CH_3);
	if (framemgr) {
		if (!test_bit(FIMC_IS_SUBDEV_INTERNAL_USE,
			&csi->dma_subdev[CSI_VIRTUAL_CH_3]->state))
			csi_dma_tag(*csi->subdev, csi, framemgr, CSI_VIRTUAL_CH_3);
	}
}

static void tasklet_csis_end(unsigned long data)
{
	u32 vc, ch, err_flag = 0;
	u32 status = IS_SHOT_SUCCESS;
	struct fimc_is_device_csi *csi;
	struct v4l2_subdev *subdev;

	subdev = (struct v4l2_subdev *)data;
	csi = v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("[CSI] csi is NULL");
		BUG();
	}

#ifdef TASKLET_MSG
	pr_info("E%d\n", atomic_read(&csi->fcount));
#endif

	for (ch = CSI_VIRTUAL_CH_0; ch < CSI_VIRTUAL_CH_MAX; ch++)
		err_flag |= csi->error_id[ch];

	if (err_flag)
		csi_err_handle(csi);
	else
		/* If error does not occur continously, the system should error count clear */
		csi->error_count = 0;

	for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++) {
		if (test_bit((CSIS_BUF_ERR_VC0 + vc), &csi->state)) {
			clear_bit((CSIS_BUF_ERR_VC0 + vc), &csi->state);
			status = IS_SHOT_CORRUPTED_FRAME;
		}
	}

	v4l2_subdev_notify(subdev, CSIS_NOTIFY_FEND, &status);
}

static void tasklet_csis_line(unsigned long data)
{
	struct fimc_is_device_csi *csi;
	struct v4l2_subdev *subdev;
	u32 line_fcount;

	subdev = (struct v4l2_subdev *)data;
	csi = v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("[CSI] csi is NULL");
		BUG();
	}

	line_fcount = atomic_read(&csi->fcount) + 1;

#ifdef TASKLET_MSG
	pr_info("L%d(%d)\n", atomic_read(&csi->fcount), line_fcount);
#endif
	v4l2_subdev_notify(subdev, CSIS_NOTIFY_LINE, &line_fcount);
}

static inline void csi_wq_func_schedule(struct fimc_is_device_csi *csi,
	struct work_struct *work_wq)
{
	if (csi->workqueue)
		queue_work(csi->workqueue, work_wq);
	else
		schedule_work(work_wq);
}

static irqreturn_t fimc_is_isr_csi(int irq, void *data)
{
	struct fimc_is_device_csi *csi;
	int frame_start, frame_end;
	int dma_frame_end;
	struct csis_irq_src irq_src;
	int ret;

	csi = data;
	memset(&irq_src, 0x0, sizeof(struct csis_irq_src));
	ret = csi_hw_g_irq_src(csi->base_reg, &irq_src, true);

	/* Get Frame Start Status */
	frame_start = irq_src.otf_start & (1 << CSI_VIRTUAL_CH_0);

	/* Get Frame End Status */
	frame_end = irq_src.otf_end & (1 << CSI_VIRTUAL_CH_0);

	/* Get DMA END Status */
#if defined(SUPPORTED_EARLYBUF_DONE_HW)
	frame_end = irq_src.line_end & (1 << CSI_VIRTUAL_CH_0);
	dma_frame_end = irq_src.line_end;
#else
	dma_frame_end = irq_src.dma_end;
#endif
	/* LINE Irq */
	if (irq_src.line_end & (1 << CSI_VIRTUAL_CH_0))
		csi_frame_line_inline(csi);

	/* DMA End */
	if (dma_frame_end) {
		/* VC0 */
		if (csi->dma_subdev[CSI_VIRTUAL_CH_0] && (dma_frame_end & (1 << CSI_VIRTUAL_CH_0)))
			csi_wq_func_schedule(csi, &csi->wq_csis_dma[CSI_VIRTUAL_CH_0]);
		/* VC1 */
		if (csi->dma_subdev[CSI_VIRTUAL_CH_1] && (dma_frame_end & (1 << CSI_VIRTUAL_CH_1)))
			csi_wq_func_schedule(csi, &csi->wq_csis_dma[CSI_VIRTUAL_CH_1]);
		/* VC2 */
		if (csi->dma_subdev[CSI_VIRTUAL_CH_2] && (dma_frame_end & (1 << CSI_VIRTUAL_CH_2)))
			csi_wq_func_schedule(csi, &csi->wq_csis_dma[CSI_VIRTUAL_CH_2]);
		/* VC3 */
		if (csi->dma_subdev[CSI_VIRTUAL_CH_3] && (dma_frame_end & (1 << CSI_VIRTUAL_CH_3)))
			csi_wq_func_schedule(csi, &csi->wq_csis_dma[CSI_VIRTUAL_CH_3]);
	}

	/* Frame Start and End */
	if (frame_start && frame_end) {
		warn("[CSIS%d] start/end overlapped",
					csi->instance);
		/* frame both interrupt since latency */
		if (csi->sw_checker == EXPECT_FRAME_START) {
			csi_frame_start_inline(csi);
			csi_frame_end_inline(csi);
		} else {
			csi_frame_end_inline(csi);
			csi_frame_start_inline(csi);
		}
	/* Frame Start */
	} else if (frame_start) {
		/* W/A: Skip start tasklet at interrupt lost case */
		if (csi->sw_checker != EXPECT_FRAME_START) {
			warn("[CSIS%d] Lost end interupt\n",
					csi->instance);
			goto clear_status;
		}
		csi_frame_start_inline(csi);
	/* Frame End */
	} else if (frame_end) {
		/* W/A: Skip end tasklet at interrupt lost case */
		if (csi->sw_checker != EXPECT_FRAME_END) {
			warn("[CSIS%d] Lost start interupt\n",
					csi->instance);
			goto clear_status;
		}
		csi_frame_end_inline(csi);
	}

	/* Check error */
	if (irq_src.err_flag)
		csi_err_check(csi, (u32 *)irq_src.err_id);

clear_status:
	return IRQ_HANDLED;
}

int fimc_is_csi_open(struct v4l2_subdev *subdev,
	struct fimc_is_framemgr *framemgr)
{
	int ret = 0;
	int vc;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_sensor *device;

	FIMC_BUG(!subdev);

	csi = v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	csi->sensor_cfg = NULL;
	csi->error_count = 0;

	for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++)
		csi->error_id[vc] = 0;

	memset(&csi->image, 0, sizeof(struct fimc_is_image));

	device = container_of(csi->subdev, struct fimc_is_device_sensor, subdev_csi);

	if (!test_bit(CSIS_DMA_ENABLE, &csi->state))
		goto p_err;

	minfo("[CSI] registered isr handler(%d) state(%d)\n", csi,
				csi->instance, test_bit(CSIS_DMA_ENABLE, &csi->state));
	csi->framemgr = framemgr;
	atomic_set(&csi->fcount, 0);
	atomic_set(&csi->vblank_count, 0);
	atomic_set(&csi->vvalid, 0);

p_err:
	return ret;
}

int fimc_is_csi_close(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_sensor *device;

	FIMC_BUG(!subdev);

	csi = v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	device = container_of(csi->subdev, struct fimc_is_device_sensor, subdev_csi);

	if (!test_bit(CSIS_DMA_ENABLE, &csi->state))
		goto p_err;

p_err:
	return ret;
}

/* value : module enum */
static int csi_init(struct v4l2_subdev *subdev, u32 value)
{
	int ret = 0;
	int ch;
	struct fimc_is_device_csi *csi;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *device;

	FIMC_BUG(!subdev);

	csi = v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	device = container_of(csi->subdev, struct fimc_is_device_sensor, subdev_csi);
	module = &device->module_enum[value];

	/* default value */
	csi->image.framerate = SENSOR_DEFAULT_FRAMERATE; /* default frame rate */

	csi_hw_reset(csi->base_reg);
p_err:
	return ret;
}

static int csi_s_power(struct v4l2_subdev *subdev,
	int on)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!subdev);

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	if (on) {
		ret = phy_power_on(csi->phy);
	} else {
#if defined(CONFIG_SECURE_CAMERA_USE) && defined(NOT_SEPERATED_SYSREG)
		if (csi->phy->power_count > 0)
#endif
		{
			ret = phy_power_off(csi->phy);
		}
	}

	if (ret) {
		err("fail to csi%d power on/off(%d)", csi->instance, on);
		goto p_err;
	}

#if defined(CONFIG_SECURE_CAMERA_USE) && defined(NOT_SEPERATED_SYSREG)
	if (csi->extra_phy) {
		if (on && (csi->extra_phy->power_count == 0))
			ret = phy_power_on(csi->extra_phy);
		else if (!on && (csi->extra_phy->power_count == 1)
			&& csi->extra_phy_off)
			ret = phy_power_off(csi->extra_phy);

		if (ret)
			warn("fail to extra csi%d power on/off(%d)",
						csi->instance, on);
	}
#endif

p_err:
	mdbgd_front("%s(%d, %d)\n", csi, __func__, on, ret);
	return ret;
}

static long csi_ioctl(struct v4l2_subdev *subdev, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_sensor *device;

	FIMC_BUG(!subdev);

	csi = v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	device = container_of(csi->subdev, struct fimc_is_device_sensor, subdev_csi);

	switch (cmd) {
	/* cancel the current and next dma setting */
	case SENSOR_IOCTL_DMA_CANCEL:
		csi_hw_s_control(csi->base_reg, CSIS_CTRL_DMA_ABORT_REQ, true);
		csi_s_output_dma(csi, CSI_VIRTUAL_CH_0, false);
		csi_s_output_dma(csi, CSI_VIRTUAL_CH_1, false);
		csi_s_output_dma(csi, CSI_VIRTUAL_CH_2, false);
		csi_s_output_dma(csi, CSI_VIRTUAL_CH_3, false);
		break;
	default:
		break;
	}

p_err:
	return ret;
}

static int csi_g_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	struct fimc_is_device_csi *csi;
	int ret = 0;
	int vc = 0;

	FIMC_BUG(!subdev);

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	switch (ctrl->id) {
	case V4L2_CID_IS_G_VC1_FRAMEPTR:
	case V4L2_CID_IS_G_VC2_FRAMEPTR:
	case V4L2_CID_IS_G_VC3_FRAMEPTR:
		vc = CSI_VIRTUAL_CH_1 + (ctrl->id - V4L2_CID_IS_G_VC1_FRAMEPTR);
		ctrl->value = csi_hw_g_frameptr(csi->base_reg, vc);

		break;
	default:
		break;
	}

	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = csi_init,
	.s_power = csi_s_power,
	.ioctl = csi_ioctl,
	.g_ctrl = csi_g_ctrl
};

static int csi_stream_on(struct v4l2_subdev *subdev,
	struct fimc_is_device_csi *csi)
{
	int ret = 0;
	u32 settle;
	u32 __iomem *base_reg;
	struct fimc_is_device_sensor *device = v4l2_get_subdev_hostdata(subdev);
	struct fimc_is_device_csi_dma *csi_dma = csi->csi_dma;
	struct fimc_is_sensor_cfg *sensor_cfg;

	FIMC_BUG(!csi);
	FIMC_BUG(!device);

	fimc_is_vendor_csi_stream_on(csi);

	if (test_bit(CSIS_START_STREAM, &csi->state)) {
		merr("[CSI] already start", csi);
		ret = -EINVAL;
		goto p_err;
	}

	sensor_cfg = csi->sensor_cfg;
	if (!sensor_cfg) {
		merr("[CSI] sensor cfg is null", csi);
		ret = -EINVAL;
		goto p_err;
	}

	if (test_bit(CSIS_DMA_ENABLE, &csi->state)) {
		ret = request_irq(csi->irq,
				fimc_is_isr_csi,
				FIMC_IS_HW_IRQ_FLAG,
				"mipi-csi",
				csi);
		if (ret) {
			merr("request_irq(IRQ_MIPICSI %d) is fail(%d)", csi, csi->irq, ret);
			goto p_err;
		}
	}

	base_reg = csi->base_reg;

	if (!device->cfg) {
		merr("[CSI] cfg is NULL", csi);
		ret = -EINVAL;
		goto p_err;
	}

	settle = device->cfg->settle;
	if (!settle) {
		if (sensor_cfg->mipi_speed)
			settle = fimc_is_hw_find_settle(sensor_cfg->mipi_speed);
		else
			merr("[CSI] mipi_speed is invalid", csi);
	}

	minfo("[CSI] settle(%dx%d@%d) = %d\n", csi,
		csi->image.window.width,
		csi->image.window.height,
		csi->image.framerate,
		settle);

	if (device->ischain)
		set_bit(CSIS_JOIN_ISCHAIN, &csi->state);
	else
		clear_bit(CSIS_JOIN_ISCHAIN, &csi->state);

	csi_hw_s_phy_default_value(base_reg, csi->instance);

	csi_hw_phy_otp_config(base_reg, csi->instance);
	csi_hw_s_settle(base_reg, settle);

	csi_hw_s_lane(base_reg, &csi->image, sensor_cfg->lanes, sensor_cfg->mipi_speed);
	csi_hw_s_control(base_reg, CSIS_CTRL_INTERLEAVE_MODE, sensor_cfg->interleave_mode);

	if (sensor_cfg->interleave_mode == CSI_MODE_CH0_ONLY) {
		csi_hw_s_config(base_reg,
			CSI_VIRTUAL_CH_0,
			&sensor_cfg->input[CSI_VIRTUAL_CH_0],
			csi->image.window.width,
			csi->image.window.height);
	} else {
		u32 vc = 0;

		for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++) {
			csi_hw_s_config(base_reg,
					vc, &sensor_cfg->input[vc],
					sensor_cfg->input[vc].width,
					sensor_cfg->input[vc].height);

			minfo("[CSI] VC%d: size(%dx%d)\n", csi, vc,
				sensor_cfg->input[vc].width, sensor_cfg->input[vc].height);
		}
	}

	csi_hw_s_irq_msk(base_reg, true);

	if (test_bit(CSIS_DMA_ENABLE, &csi->state)) {
		/* runtime buffer done state for error */
		clear_bit(CSIS_BUF_ERR_VC0, &csi->state);
		clear_bit(CSIS_BUF_ERR_VC1, &csi->state);
		clear_bit(CSIS_BUF_ERR_VC2, &csi->state);
		clear_bit(CSIS_BUF_ERR_VC3, &csi->state);

		csi->sw_checker = EXPECT_FRAME_START;
		csi->overflow_cnt = 0;
		csi_s_config_dma(csi, sensor_cfg->output);
		memset(csi->pre_dma_enable, -1, ARRAY_SIZE(csi->pre_dma_enable));

		/* for multi frame buffer setting for internal vc */
		csis_s_vc_dma_multibuf(csi);

		/* Tasklet Setting */
		/* OTF */
		tasklet_init(&csi->tasklet_csis_str, tasklet_csis_str_otf, (unsigned long)subdev);
		tasklet_init(&csi->tasklet_csis_end, tasklet_csis_end, (unsigned long)subdev);

		if (device->ischain &&
			!test_bit(FIMC_IS_SENSOR_OTF_OUTPUT, &device->state)) {
#if defined(SUPPORTED_EARLYBUF_DONE_SW) || defined(SUPPORTED_EARLYBUF_DONE_HW)
			if (device->early_buf_done_mode) {
#if defined(SUPPORTED_EARLYBUF_DONE_SW)
				device->early_buf_timer.function = csis_early_buf_done;
#elif defined(SUPPORTED_EARLYBUF_DONE_HW)
				csi_hw_s_control(base_reg, CSIS_CTRL_LINE_RATIO, device->early_buf_done_mode);
#endif
			}
#endif
		}

		/* DMA Workqueue Setting */
		if (csi->dma_subdev[CSI_VIRTUAL_CH_0])
			INIT_WORK(&csi->wq_csis_dma[CSI_VIRTUAL_CH_0], wq_csis_dma_vc0);
		if (csi->dma_subdev[CSI_VIRTUAL_CH_1])
			INIT_WORK(&csi->wq_csis_dma[CSI_VIRTUAL_CH_1], wq_csis_dma_vc1);
		if (csi->dma_subdev[CSI_VIRTUAL_CH_2])
			INIT_WORK(&csi->wq_csis_dma[CSI_VIRTUAL_CH_2], wq_csis_dma_vc2);
		if (csi->dma_subdev[CSI_VIRTUAL_CH_3])
			INIT_WORK(&csi->wq_csis_dma[CSI_VIRTUAL_CH_3], wq_csis_dma_vc3);
	}

	/* if sensor's output otf was enabled, enable line irq */
	if (!test_bit(FIMC_IS_SENSOR_OTF_OUTPUT, &device->state)) {
		csi_hw_s_control(base_reg, CSIS_CTRL_LINE_RATIO, csi->image.window.height * CSI_LINE_RATIO / 20);
		csi_hw_s_control(base_reg, CSIS_CTRL_ENABLE_LINE_IRQ, 0x1);
		tasklet_init(&csi->tasklet_csis_line, tasklet_csis_line, (unsigned long)subdev);
		minfo("[CSI] ENABLE Line irq(%d)\n", csi, CSI_LINE_RATIO);
	}

	spin_lock(&csi_dma->barrier);
	/* common dma register setting */
	if (atomic_inc_return(&csi_dma->rcount) == 1)
		csi_hw_s_dma_common(csi_dma->base_reg);
	spin_unlock(&csi_dma->barrier);

	csi_hw_enable(base_reg);

	if (unlikely(debug_csi))
		csi_hw_dump(base_reg);

	set_bit(CSIS_START_STREAM, &csi->state);
p_err:
	return ret;
}

static int csi_stream_off(struct v4l2_subdev *subdev,
	struct fimc_is_device_csi *csi)
{
	int ret = 0;
	int vc;
	u32 __iomem *base_reg;
	struct fimc_is_device_sensor *device = v4l2_get_subdev_hostdata(subdev);
	struct fimc_is_device_csi_dma *csi_dma = csi->csi_dma;

	FIMC_BUG(!csi);
	FIMC_BUG(!device);

	if (!test_bit(CSIS_START_STREAM, &csi->state)) {
		merr("[CSI] already stop", csi);
		ret = -EINVAL;
		goto p_err;
	}

	base_reg = csi->base_reg;

	csi_hw_s_irq_msk(base_reg, false);

	csi_hw_disable(base_reg);

	csi_hw_reset(base_reg);

	spin_lock(&csi_dma->barrier);
	atomic_dec(&csi_dma->rcount);
	spin_unlock(&csi_dma->barrier);

	if (!test_bit(FIMC_IS_SENSOR_OTF_OUTPUT, &device->state))
		tasklet_kill(&csi->tasklet_csis_line);

	if (!test_bit(CSIS_DMA_ENABLE, &csi->state))
		goto p_dma_skip;

	tasklet_kill(&csi->tasklet_csis_str);
	tasklet_kill(&csi->tasklet_csis_end);

	for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++) {
		if (csi->dma_subdev[vc]) {
			ret = flush_work(&csi->wq_csis_dma[vc]);
			if (ret)
				minfo("[CSI] flush_work executed! vc(%d)\n", csi, vc);
		}
	}

	csis_flush_all_vc_buf_done(csi, VB2_BUF_STATE_ERROR);

	atomic_set(&csi->vvalid, 0);

	free_irq(csi->irq, csi);

p_dma_skip:
	clear_bit(CSIS_START_STREAM, &csi->state);
p_err:
	return ret;
}

static int csi_s_stream(struct v4l2_subdev *subdev, int enable)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!subdev);

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	/* H/W setting skip */
	if (test_bit(CSIS_DUMMY, &csi->state)) {
		mdbgd_front("%s(dummy)\n", csi, __func__);
		goto p_err;
	}

	if (enable) {
		ret = csi_stream_on(subdev, csi);
		if (ret) {
			merr("[CSI] csi_stream_on is fail(%d)", csi, ret);
			goto p_err;
		}
	} else {
		ret = csi_stream_off(subdev, csi);
		if (ret) {
			merr("[CSI] csi_stream_off is fail(%d)", csi, ret);
			goto p_err;
		}
	}

p_err:
	return 0;
}

static int csi_s_param(struct v4l2_subdev *subdev, struct v4l2_streamparm *param)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;
	struct v4l2_captureparm *cp;
	struct v4l2_fract *tpf;

	FIMC_BUG(!subdev);
	FIMC_BUG(!param);

	cp = &param->parm.capture;
	tpf = &cp->timeperframe;

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	csi->image.framerate = tpf->denominator / tpf->numerator;

	mdbgd_front("%s(%d, %d)\n", csi, __func__, csi->image.framerate, ret);
	return ret;
}

static int csi_s_format(struct v4l2_subdev *subdev,
	struct v4l2_subdev_pad_config *cfg,
	struct v4l2_subdev_format *fmt)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_sensor *device;

	FIMC_BUG(!subdev);
	FIMC_BUG(!fmt);

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		ret = -ENODEV;
		goto p_err;
	}

	csi->image.window.offs_h = 0;
	csi->image.window.offs_v = 0;
	csi->image.window.width = fmt->format.width;
	csi->image.window.height = fmt->format.height;
	csi->image.window.o_width = fmt->format.width;
	csi->image.window.o_height = fmt->format.height;
	csi->image.format.pixelformat = fmt->format.code;
	csi->image.format.field = fmt->format.field;

	device = v4l2_get_subdev_hostdata(subdev);
	if (!device) {
		merr("device is NULL", csi);
		ret = -ENODEV;
		goto p_err;
	}

	csi->sensor_cfg = device->cfg;
	if (!device->cfg) {
		merr("sensor cfg is invalid", csi);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	mdbgd_front("%s(%dx%d, %X)\n", csi, __func__, fmt->format.width, fmt->format.height, fmt->format.code);
	return ret;
}


static int csi_s_buffer(struct v4l2_subdev *subdev, void *buf, unsigned int *size)
{
	int ret = 0;
	u32 vc = 0;
	struct fimc_is_device_csi *csi;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_subdev *dma_subdev = NULL;
	struct fimc_is_frame *frame;

	FIMC_BUG(!subdev);

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(subdev);
	if (unlikely(csi == NULL)) {
		err("csi is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (!test_bit(CSIS_DMA_ENABLE, &csi->state))
		goto p_err;

	/* for virtual channels */
	frame = (struct fimc_is_frame *)buf;
	if (!frame) {
		err("frame is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	dma_subdev = frame->subdev;
	vc = CSI_ENTRY_TO_CH(dma_subdev->id);

	framemgr = GET_SUBDEV_FRAMEMGR(dma_subdev);
	if (!framemgr) {
		err("framemgr is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (csi_hw_g_output_dma_enable(csi->base_reg, vc)) {
		err("[VC%d][F%d] already DMA enabled!!", vc, frame->fcount);
		ret = -EINVAL;
	} else {
		csi_s_buf_addr(csi, frame, 0, vc);
		csi_s_output_dma(csi, vc, true);

		trans_frame(framemgr, frame, FS_PROCESS);
	}

p_err:
	return ret;
}


static int csi_g_errorCode(struct v4l2_subdev *subdev, u32 *errorCode)
{
	int ret = 0;
	int vc;
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!subdev);

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++)
		*errorCode |= csi->error_id[vc];

	return ret;
}


static const struct v4l2_subdev_video_ops video_ops = {
	.s_stream = csi_s_stream,
	.s_parm = csi_s_param,
	.s_rx_buffer = csi_s_buffer,
	.g_input_status = csi_g_errorCode
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = csi_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

int fimc_is_csi_probe(void *parent, u32 instance)
{
	int i = 0;
	int ret = 0;
	struct v4l2_subdev *subdev_csi;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_sensor *device = parent;
	struct resource *mem_res;
	struct platform_device *pdev;
	struct fimc_is_core *core;

	FIMC_BUG(!device);

	subdev_csi = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_csi) {
		merr("subdev_csi is NULL", device);
		ret = -ENOMEM;
		goto p_err;
	}
	device->subdev_csi = subdev_csi;
	core = device->private_data;

	csi = kzalloc(sizeof(struct fimc_is_device_csi), GFP_KERNEL);
	if (!csi) {
		merr("csi is NULL", device);
		ret = -ENOMEM;
		goto p_err_free1;
	}

	/* Get SFR base register */
	pdev = device->pdev;
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_res) {
		probe_err("Failed to get io memory region(%p)", mem_res);
		ret = -EBUSY;
		goto err_get_resource;
	}

	csi->regs_start = mem_res->start;
	csi->regs_end = mem_res->end;
	csi->base_reg =  devm_ioremap_nocache(&pdev->dev, mem_res->start, resource_size(mem_res));
	if (!csi->base_reg) {
		probe_err("Failed to remap io region(%p)", csi->base_reg);
		ret = -ENOMEM;
		goto err_get_resource;
	}

	/* Get IRQ SPI number */
	csi->irq = platform_get_irq(pdev, 0);
	if (csi->irq < 0) {
		probe_err("Failed to get csi_irq(%d)", csi->irq);
		ret = -EBUSY;
		goto err_get_irq;
	}

	csi->phy = devm_phy_get(&pdev->dev, "csis_dphy");
	if (IS_ERR(csi->phy))
		return PTR_ERR(csi->phy);

#if defined(CONFIG_SECURE_CAMERA_USE) && defined(NOT_SEPERATED_SYSREG)
	csi->extra_phy = devm_phy_get(&pdev->dev, "extra_csis_dphy");
	if (IS_ERR(csi->extra_phy))
		csi->extra_phy = NULL;
#endif

	/* pointer to me from device sensor */
	csi->subdev = &device->subdev_csi;

	csi->instance = instance;

	/* Get common dma */
	csi->csi_dma = &core->csi_dma;

	/* default state setting */
	clear_bit(CSIS_DUMMY, &csi->state);
	clear_bit(CSIS_SET_MULTIBUF_VC1, &csi->state);
	clear_bit(CSIS_SET_MULTIBUF_VC2, &csi->state);
	clear_bit(CSIS_SET_MULTIBUF_VC3, &csi->state);
	set_bit(CSIS_DMA_ENABLE, &csi->state);

	/* init dma subdev slots */
	for (i = CSI_VIRTUAL_CH_0; i < CSI_VIRTUAL_CH_MAX; i++)
		csi->dma_subdev[i] = NULL;

	csi->workqueue = alloc_workqueue("fimc-csi/[H/U]", WQ_HIGHPRI | WQ_UNBOUND, 0);
	if (!csi->workqueue)
		probe_warn("failed to alloc CSI own workqueue, will be use global one");

	v4l2_subdev_init(subdev_csi, &subdev_ops);
	v4l2_set_subdevdata(subdev_csi, csi);
	v4l2_set_subdev_hostdata(subdev_csi, device);
	snprintf(subdev_csi->name, V4L2_SUBDEV_NAME_SIZE, "csi-subdev.%d", instance);
	ret = v4l2_device_register_subdev(&device->v4l2_dev, subdev_csi);
	if (ret) {
		merr("v4l2_device_register_subdev is fail(%d)", device, ret);
		goto err_reg_v4l2_subdev;
	}

	info("[%d][FRT:D] %s(%d)\n", instance, __func__, ret);
	return 0;

err_reg_v4l2_subdev:
err_get_irq:
	iounmap(csi->base_reg);

err_get_resource:
	kfree(csi);

p_err_free1:
	kfree(subdev_csi);
	device->subdev_csi = NULL;

p_err:
	err("[%d][FRT:D] %s(%d)\n", instance, __func__, ret);
	return ret;
}

int fimc_is_csi_dma_probe(struct fimc_is_device_csi_dma *csi_dma, struct platform_device *pdev)
{
	int ret = 0;
	struct resource *mem_res;

	/* Get SFR base register */
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_CSIS_DMA);
	if (!mem_res) {
		probe_err("Failed to get io memory region(%p)", mem_res);
		ret = -EBUSY;
		goto err_get_resource;
	}

	csi_dma->regs_start = mem_res->start;
	csi_dma->regs_end = mem_res->end;
	csi_dma->base_reg =  devm_ioremap_nocache(&pdev->dev, mem_res->start, resource_size(mem_res));
	if (!csi_dma->base_reg) {
		probe_err("Failed to remap io region(%p)", csi_dma->base_reg);
		ret = -ENOMEM;
		goto err_get_base;
	}

	atomic_set(&csi_dma->rcount, 0);

	spin_lock_init(&csi_dma->barrier);

	return 0;

err_get_resource:
err_get_base:
	err("[CSI:D] %s(%d)\n", __func__, ret);
	return ret;
}
