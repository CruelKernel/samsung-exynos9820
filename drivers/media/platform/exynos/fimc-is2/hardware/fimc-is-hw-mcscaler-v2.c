/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-mcscaler-v2.h"
#include "api/fimc-is-hw-api-mcscaler-v2.h"
#include "../interface/fimc-is-interface-ischain.h"
#include "fimc-is-param.h"
#include "fimc-is-err.h"
#include <linux/videodev2_exynos_media.h>

int debug_subblk_ctrl;
module_param(debug_subblk_ctrl, int, 0644);

spinlock_t	mcsc_out_slock;
static ulong	mcsc_out_st = 0xFFFF;	/* To check shared output state */
#define MCSC_RST_CHK (MCSC_OUTPUT_MAX)
#define check_shared_out_running(cap, out_bit) \
	(cap->enable_shared_output && test_bit(out_id, &mcsc_out_st))

static int fimc_is_hw_mcsc_rdma_cfg(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	struct param_mcs_input *input);
static void fimc_is_hw_mcsc_wdma_cfg(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame);
static void fimc_is_hw_mcsc_size_dump(struct fimc_is_hw_ip *hw_ip);

static int fimc_is_hw_mcsc_handle_interrupt(u32 id, void *context)
{
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_ip *hw_ip = NULL;
	struct fimc_is_group *head;
	struct mcs_param *param;
	u32 status, mask, instance, hw_fcount, index, hl = 0, vl = 0;
	bool f_err = false, f_clk_gate = false;
	int ret = 0;

	hw_ip = (struct fimc_is_hw_ip *)context;
	hardware = hw_ip->hardware;
	hw_fcount = atomic_read(&hw_ip->fcount);
	instance = atomic_read(&hw_ip->instance);
	param = &hw_ip->region[instance]->parameter.mcs;

	if (!test_bit(HW_OPEN, &hw_ip->state)) {
		mserr_hw("invalid interrupt", instance, hw_ip);
		return 0;
	}

	if (test_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag)) {
		mserr_hw("During recovery : invalid interrupt", instance, hw_ip);
		return 0;
	}

	fimc_is_scaler_get_input_status(hw_ip->regs, hw_ip->id, &hl, &vl);
	/* read interrupt status register (sc_intr_status) */
	mask = fimc_is_scaler_get_intr_mask(hw_ip->regs, hw_ip->id);
	status = fimc_is_scaler_get_intr_status(hw_ip->regs, hw_ip->id);
	status = (~mask) & status;

	fimc_is_scaler_clear_intr_src(hw_ip->regs, hw_ip->id, status);

	if (!test_bit(HW_RUN, &hw_ip->state)) {
		mserr_hw("HW disabled!! interrupt(0x%x)", instance, hw_ip, status);
		goto p_err;
	}

	if (status & (1 << INTR_MC_SCALER_OVERFLOW)) {
		mserr_hw("Overflow!! (0x%x)", instance, hw_ip, status);
		f_err = true;
	}

	if (status & (1 << INTR_MC_SCALER_OUTSTALL)) {
		mserr_hw("Output Block BLOCKING!! (0x%x)", instance, hw_ip, status);
		f_err = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF)) {
		mserr_hw("Input OTF Vertical Underflow!! (0x%x)", instance, hw_ip, status);
		f_err = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF)) {
		mserr_hw("Input OTF Vertical Overflow!! (0x%x)", instance, hw_ip, status);
		f_err = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF)) {
		mserr_hw("Input OTF Horizontal Underflow!! (0x%x)", instance, hw_ip, status);
		f_err = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF)) {
		mserr_hw("Input OTF Horizontal Overflow!! (0x%x)", instance, hw_ip, status);
		f_err = true;
	}

	if (status & (1 << INTR_MC_SCALER_WDMA_FINISH))
		mserr_hw("Disabeld interrupt occurred! WDAM FINISH!! (0x%x)", instance, hw_ip, status);

	if (status & (1 << INTR_MC_SCALER_FRAME_START)) {
		atomic_add(hw_ip->num_buffers, &hw_ip->count.fs);
		hw_ip->cur_s_int++;

		if (hw_ip->cur_s_int == 1) {
			hw_ip->debug_index[1] = hw_ip->debug_index[0] % DEBUG_FRAME_COUNT;
			index = hw_ip->debug_index[1];
			hw_ip->debug_info[index].fcount = hw_ip->debug_index[0];
			hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_START] = raw_smp_processor_id();
			hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_START] = cpu_clock(raw_smp_processor_id());
			if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
				msinfo_hw("[F:%d]F.S\n", instance, hw_ip, hw_fcount);

			if (param->input.dma_cmd == DMA_INPUT_COMMAND_ENABLE) {
				fimc_is_hardware_frame_start(hw_ip, instance);
			} else {
				clear_bit(HW_CONFIG, &hw_ip->state);
				atomic_set(&hw_ip->status.Vvalid, V_VALID);
			}
		}

		/* for set shadow register write start */
		head = GET_HEAD_GROUP_IN_DEVICE(FIMC_IS_DEVICE_ISCHAIN, hw_ip->group[instance]);
		if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state))
			fimc_is_scaler_set_shadow_ctrl(hw_ip->regs, hw_ip->id, SHADOW_WRITE_START);

		if ((hw_ip->cur_s_int < hw_ip->num_buffers) && (!hardware->hw_fro_en)) {
			if (hw_ip->mframe) {
				struct fimc_is_frame *mframe = hw_ip->mframe;
				mframe->cur_buf_index = hw_ip->cur_s_int;

				fimc_is_hw_mcsc_wdma_cfg(hw_ip, mframe);

				ret = fimc_is_hw_mcsc_rdma_cfg(hw_ip, mframe, &param->input);
				if (ret) {
					mserr_hw("[F:%d]mcsc rdma_cfg failed\n",
						mframe->instance, hw_ip, mframe->fcount);
					return ret;
				}
			} else {
				serr_hw("mframe is null(s:%d, e:%d, t:%d)", hw_ip,
					hw_ip->cur_s_int, hw_ip->cur_e_int, hw_ip->num_buffers);
			}
		} else {
			struct fimc_is_group *group;
			group = hw_ip->group[instance];
			/*
			 * In case of M2M mcsc, just supports only one buffering.
			 * So, in start irq, "setting to stop mcsc for N + 1" should be assigned.
			 *
			 * TODO: Don't touch global control, but we don't know how to be mapped
			 *       with group-id and scX_ctrl.
			 */
			if (!test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state))
				fimc_is_scaler_stop(hw_ip->regs, hw_ip->id);
		}
	}

	if (status & (1 << INTR_MC_SCALER_FRAME_END)) {
		atomic_add(hw_ip->num_buffers, &hw_ip->count.fe);
		if (hardware->hw_fro_en)
			hw_ip->cur_e_int += hw_ip->num_buffers;
		else
			hw_ip->cur_e_int++;

		if (hw_ip->cur_e_int >= hw_ip->num_buffers) {
			fimc_is_hw_mcsc_frame_done(hw_ip, NULL, IS_SHOT_SUCCESS);

			if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
				sinfo_hw("[F:%d]F.E\n", hw_ip, hw_fcount);

			atomic_set(&hw_ip->status.Vvalid, V_BLANK);
			if (atomic_read(&hw_ip->count.fs) < atomic_read(&hw_ip->count.fe)) {
				mserr_hw("fs(%d), fe(%d), dma(%d), status(0x%x)", instance, hw_ip,
					atomic_read(&hw_ip->count.fs),
					atomic_read(&hw_ip->count.fe),
					atomic_read(&hw_ip->count.dma), status);
			}

			wake_up(&hw_ip->status.wait_queue);
			f_clk_gate = true;
			hw_ip->mframe = NULL;
		}
	}

	/* for handle chip dependant intr */
	f_err |= fimc_is_scaler_handle_extended_intr(status);

	if (f_err) {
		msinfo_hw("[F:%d] Ocurred error interrupt (%d,%d) status(0x%x)\n",
			instance, hw_ip, hw_fcount, hl, vl, status);
		fimc_is_scaler_dump(hw_ip->regs);
		fimc_is_hardware_size_dump(hw_ip);
	}

	if (f_clk_gate)
		CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);

p_err:
	return ret;
}

void fimc_is_hw_mcsc_hw_info(struct fimc_is_hw_ip *hw_ip, struct fimc_is_hw_mcsc_cap *cap)
{
	int i = 0;

	if (!(hw_ip && cap))
		return;

	sinfo_hw("==== h/w info(ver:0x%X) ====\n", hw_ip, cap->hw_ver);
	sinfo_hw("[IN] out:%d, in(otf:%d/dma:%d), hwfc:%d, tdnr:%d, djag:%d, ds_vra:%d, ysum:%d\n", hw_ip,
		cap->max_output, cap->in_otf, cap->in_dma, cap->hwfc,
		cap->tdnr, cap->djag, cap->ds_vra, cap->ysum);
	for (i = MCSC_OUTPUT0; i < cap->max_output; i++)
		info_hw("[OUT%d] out(otf/dma):%d/%d, hwfc:%d\n", i,
			cap->out_otf[i], cap->out_dma[i], cap->out_hwfc[i]);

	sinfo_hw("========================\n", hw_ip);
}

struct fimc_is_hw_ip *get_mcsc_hw_ip(struct fimc_is_hw_ip *hw_ip)
{
	int hw_slot = -1;
	u32 hw_id;

	if (!hw_ip)
		return NULL;

	switch (hw_ip->id) {
	case DEV_HW_MCSC0:
		hw_id = DEV_HW_MCSC1;
		break;
	case DEV_HW_MCSC1:
		hw_id = DEV_HW_MCSC0;
		break;
	default:
		serr_hw("Invalid hw_ip->id\n", hw_ip);
		return NULL;
	}

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (valid_hw_slot_id(hw_slot))
		return &hw_ip->hardware->hw_ip[hw_slot];

	return NULL;
}

int check_sc_core_running(struct fimc_is_hw_ip *hw_ip, struct fimc_is_hw_mcsc_cap *cap)
{
	struct fimc_is_hw_ip *hw_ip_ = NULL;
	u32 ret = 0;

	hw_ip_ = get_mcsc_hw_ip(hw_ip);

	if (cap->enable_shared_output) {
		if (hw_ip && test_bit(HW_RUN, &hw_ip->state))
			ret = hw_ip->id;

		if (hw_ip_ && test_bit(HW_RUN, &hw_ip_->state))
			ret = hw_ip_->id;
	}

	return ret;
}

static int fimc_is_hw_mcsc_open(struct fimc_is_hw_ip *hw_ip, u32 instance,
	struct fimc_is_group *group)
{
	int ret = 0, i, j;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap;
	u32 output_id;

	FIMC_BUG(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	frame_manager_probe(hw_ip->framemgr, FRAMEMGR_ID_HW | (1 << hw_ip->id), "HWMCS");
	frame_manager_probe(hw_ip->framemgr_late, FRAMEMGR_ID_HW | (1 << hw_ip->id) | 0xF000, "HWMCS LATE");
	frame_manager_open(hw_ip->framemgr, FIMC_IS_MAX_HW_FRAME);
	frame_manager_open(hw_ip->framemgr_late, FIMC_IS_MAX_HW_FRAME_LATE);

	hw_ip->priv_info = vzalloc(sizeof(struct fimc_is_hw_mcsc));
	if(!hw_ip->priv_info) {
		mserr_hw("hw_ip->priv_info(null)", instance, hw_ip);
		ret = -ENOMEM;
		goto err_alloc;
	}

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	hw_mcsc->instance = FIMC_IS_STREAM_COUNT;

	cap = GET_MCSC_HW_CAP(hw_ip);

	/* get the mcsc hw info */
	ret = fimc_is_hw_query_cap((void *)cap, hw_ip->id);
	if (ret) {
		mserr_hw("failed to get hw info", instance, hw_ip);
		ret = -EINVAL;
		goto err_query_cap;
	}

	/* print hw info */
	fimc_is_hw_mcsc_hw_info(hw_ip, cap);

	hw_ip->subblk_ctrl = debug_subblk_ctrl;
	hw_ip->mframe = NULL;
	atomic_set(&hw_ip->status.Vvalid, V_BLANK);
	set_bit(HW_OPEN, &hw_ip->state);
	msdbg_hw(2, "open: [G:0x%x], framemgr[%s]", instance, hw_ip,
		GROUP_ID(group->id), hw_ip->framemgr->name);

	for (i = 0; i < SENSOR_POSITION_MAX; i++) {
		for (j = 0; j < FIMC_IS_STREAM_COUNT; j++)
			hw_mcsc->cur_setfile[i][j] = NULL;
	}
	if (check_sc_core_running(hw_ip, cap))
		return 0;

	/* initialize of shared values between MCSC0 and MCSC1 */
	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++)
		set_bit(output_id, &mcsc_out_st);
	clear_bit(MCSC_RST_CHK, &mcsc_out_st);

	msinfo_hw("mcsc_open: done, (0x%lx)\n", instance, hw_ip, mcsc_out_st);

	return 0;

err_query_cap:
	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;
err_alloc:
	frame_manager_close(hw_ip->framemgr);
	frame_manager_close(hw_ip->framemgr_late);

	return ret;
}

static int fimc_is_hw_mcsc_init(struct fimc_is_hw_ip *hw_ip, u32 instance,
	struct fimc_is_group *group, bool flag, u32 module_id)
{
	int ret = 0;
	u32 entry, output_id;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap;
	struct fimc_is_subdev *subdev;
	struct fimc_is_video_ctx *vctx;
	struct fimc_is_video *video;

	FIMC_BUG(!hw_ip);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	if (!hw_mcsc) {
		mserr_hw("hw_mcsc is null ", instance, hw_ip);
		ret = -ENODATA;
		goto err_priv_info;
	}

	hw_mcsc->rep_flag[instance] = flag;

	cap = GET_MCSC_HW_CAP(hw_ip);
	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++) {
		if (test_bit(output_id, &hw_mcsc->out_en)) {
			msdbg_hw(2, "already set output(%d)\n", instance, hw_ip, output_id);
			continue;
		}

		entry = GET_ENTRY_FROM_OUTPUT_ID(output_id);
		subdev = group->subdev[entry];
		if (!subdev)
			continue;

		vctx = subdev->vctx;
		if (!vctx) {
			continue;
		}

		video = vctx->video;
		if (!video) {
			mserr_hw("video is NULL. entry(%d)", instance, hw_ip, entry);
			BUG();
		}
		set_bit(output_id, &hw_mcsc->out_en);
	}

	set_bit(HW_INIT, &hw_ip->state);
	msdbg_hw(2, "init: out_en[0x%lx]\n", instance, hw_ip, hw_mcsc->out_en);

err_priv_info:

	return ret;
}

static int fimc_is_hw_mcsc_deinit(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	return 0;
}

static int fimc_is_hw_mcsc_close(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;
	frame_manager_close(hw_ip->framemgr);
	frame_manager_close(hw_ip->framemgr_late);

	clear_bit(HW_OPEN, &hw_ip->state);
	clear_bit(HW_MCS_YSUM_CFG, &hw_ip->state);
	clear_bit(HW_MCS_DS_CFG, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_mcsc_enable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	ulong flag = 0;
	struct mcs_param *mcs_param;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	if (test_bit(HW_RUN, &hw_ip->state))
		return ret;

	msdbg_hw(2, "enable: start, (0x%lx)\n", instance, hw_ip, mcsc_out_st);

	spin_lock_irqsave(&mcsc_out_slock, flag);
	ret = fimc_is_hw_mcsc_reset(hw_ip);
	if (ret != 0) {
		mserr_hw("MCSC sw reset fail", instance, hw_ip);
		spin_unlock_irqrestore(&mcsc_out_slock, flag);
		return -ENODEV;
	}

	ret = fimc_is_hw_mcsc_clear_interrupt(hw_ip);
	if (ret != 0) {
		mserr_hw("MCSC sw reset fail", instance, hw_ip);
		spin_unlock_irqrestore(&mcsc_out_slock, flag);
		return -ENODEV;
	}

	msdbg_hw(2, "enable: done, (0x%lx)\n", instance, hw_ip, mcsc_out_st);

	mcs_param = &hw_ip->region[instance]->parameter.mcs;
	fimc_is_hw_mcsc_tdnr_init(hw_ip, mcs_param, instance);

	set_bit(HW_RUN, &hw_ip->state);
	spin_unlock_irqrestore(&mcsc_out_slock, flag);

	return ret;
}

static int fimc_is_hw_mcsc_disable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	u32 input_id, output_id;
	long timetowait;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	struct fimc_is_hw_ip *hw_ip_ = NULL;
	struct mcs_param *mcs_param;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!cap);
	FIMC_BUG(!hw_ip->priv_info);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (atomic_read(&hw_ip->rsccount) > 1)
		return 0;

	msinfo_hw("mcsc_disable: Vvalid(%d)\n", instance, hw_ip,
		atomic_read(&hw_ip->status.Vvalid));

	if (test_bit(HW_RUN, &hw_ip->state)) {
		timetowait = wait_event_timeout(hw_ip->status.wait_queue,
			!atomic_read(&hw_ip->status.Vvalid),
			FIMC_IS_HW_STOP_TIMEOUT);

		if (!timetowait)
			mserr_hw("wait FRAME_END timeout (%ld)", instance,
				hw_ip, timetowait);

		ret = fimc_is_hw_mcsc_clear_interrupt(hw_ip);
		if (ret != 0) {
			mserr_hw("MCSC sw clear_interrupt fail", instance, hw_ip);
			return -ENODEV;
		}

		clear_bit(HW_RUN, &hw_ip->state);
		clear_bit(HW_CONFIG, &hw_ip->state);
	} else {
		msdbg_hw(2, "already disabled\n", instance, hw_ip);
	}
	hw_ip->mframe = NULL;

	if (check_sc_core_running(hw_ip, cap))
		return 0;

	if (hw_ip)
		fimc_is_scaler_stop(hw_ip->regs, hw_ip->id);

	hw_ip_ = get_mcsc_hw_ip(hw_ip);
	if (hw_ip_)
		fimc_is_scaler_stop(hw_ip_->regs, hw_ip_->id);

	/* disable MCSC */
	if (cap->in_dma == MCSC_CAP_SUPPORT)
		fimc_is_scaler_clear_rdma_addr(hw_ip->regs);

	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++) {
		input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
		if (cap->enable_shared_output == false || !test_bit(output_id, &mcsc_out_st)) {
			msinfo_hw("[OUT:%d]hw_mcsc_disable: clear_wdma_addr\n", instance, hw_ip, output_id);
			fimc_is_scaler_clear_wdma_addr(hw_ip->regs, output_id);
		}
	}

	fimc_is_scaler_clear_shadow_ctrl(hw_ip->regs, hw_ip->id);

	/* disable TDNR */
	mcs_param = &hw_ip->region[instance]->parameter.mcs;
	fimc_is_hw_mcsc_tdnr_deinit(hw_ip, mcs_param, instance);

	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++)
		set_bit(output_id, &mcsc_out_st);
	clear_bit(MCSC_RST_CHK, &mcsc_out_st);

	msinfo_hw("mcsc_disable: done, (0x%lx)\n", instance, hw_ip, mcsc_out_st);

	return ret;
}

static int fimc_is_hw_mcsc_rdma_cfg(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	struct param_mcs_input *input)
{
	int ret = 0, i;
	u32 rdma_addr[4] = {0}, plane;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	/* can't support this function */
	if (cap->in_dma != MCSC_CAP_SUPPORT)
		return ret;

	if (input->dma_cmd == DMA_INPUT_COMMAND_DISABLE)
		return ret;

	plane = input->plane;
	for (i = 0; i < plane; i++)
		rdma_addr[i] = (typeof(*rdma_addr))
			frame->dvaddr_buffer[plane * frame->cur_buf_index + i];

	/* DMA in */
	msdbg_hw(2, "[F:%d]rdma_cfg [addr: %x]\n",
		frame->instance, hw_ip, frame->fcount, rdma_addr[0]);

	if (!rdma_addr[0]) {
		mserr_hw("Wrong rdma_addr(%x)\n", frame->instance, hw_ip, rdma_addr[0]);
		fimc_is_scaler_clear_rdma_addr(hw_ip->regs);
		ret = -EINVAL;
		return ret;
	}

	/* use only one buffer (per-frame) */
	fimc_is_scaler_set_rdma_frame_seq(hw_ip->regs, 0x1 << USE_DMA_BUFFER_INDEX);

	if (input->plane == DMA_INPUT_PLANE_4) {
		/* 8+2(10bit) format */
		fimc_is_scaler_set_rdma_addr(hw_ip->regs,
			rdma_addr[0], rdma_addr[1], 0, USE_DMA_BUFFER_INDEX);
		fimc_is_scaler_set_rdma_2bit_addr(hw_ip->regs,
			rdma_addr[2], rdma_addr[3], USE_DMA_BUFFER_INDEX);
	} else {
		fimc_is_scaler_set_rdma_addr(hw_ip->regs,
			rdma_addr[0], rdma_addr[1], rdma_addr[2],
			USE_DMA_BUFFER_INDEX);
	}
	return ret;
}

static u32 *hw_mcsc_get_target_addr(u32 out_id, struct fimc_is_frame *frame)
{
	u32 *addr = NULL;

	switch (out_id) {
	case MCSC_OUTPUT0:
		addr = frame->sc0TargetAddress;
		break;
	case MCSC_OUTPUT1:
		addr = frame->sc1TargetAddress;
		break;
	case MCSC_OUTPUT2:
		addr = frame->sc2TargetAddress;
		break;
	case MCSC_OUTPUT3:
		addr = frame->sc3TargetAddress;
		break;
	case MCSC_OUTPUT4:
		addr = frame->sc4TargetAddress;
		break;
	case MCSC_OUTPUT5:
		addr = frame->sc5TargetAddress;
		break;
	default:
		panic("[F:%d] invalid output id(%d)", frame->fcount, out_id);
		break;
	}

	return addr;
}

static void hw_mcsc_set_wdma_addr(struct fimc_is_hw_ip *hw_ip, u32 *wdma_addr,
	u32 out_id, u32 plane, u32 buf_idx)
{
	u32 addr[4];

	if (plane == DMA_OUTPUT_PLANE_4) {
		/* addr_2bit_y, addr_2bit_uv */
		addr[0] = wdma_addr[0];
		addr[1] = wdma_addr[1];
		addr[2] = 0;
		fimc_is_scaler_set_wdma_2bit_addr(hw_ip->regs, out_id,
				wdma_addr[2], wdma_addr[3], buf_idx);
	} else {
		addr[0] = wdma_addr[0];
		addr[1] = wdma_addr[1];
		addr[2] = wdma_addr[2];
	}
	fimc_is_scaler_set_wdma_addr(hw_ip->regs, out_id,
		addr[0], addr[1], addr[2], buf_idx);
}

static void fimc_is_hw_mcsc_wdma_clear(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	struct mcs_param *param, u32 out_id, struct fimc_is_hw_mcsc_cap *cap)
{
	u32 wdma_enable = 0;
	ulong flag;

	wdma_enable = fimc_is_scaler_get_dma_out_enable(hw_ip->regs, out_id);

	spin_lock_irqsave(&mcsc_out_slock, flag);
	if (wdma_enable && !check_shared_out_running(cap, out_id)) {
		fimc_is_scaler_set_dma_out_enable(hw_ip->regs, out_id, false);
		fimc_is_scaler_clear_wdma_addr(hw_ip->regs, out_id);
		msdbg_hw(2, "[OUT:%d]shot: dma_out disabled\n",
			frame->instance, hw_ip, out_id);

		if (out_id == MCSC_OUTPUT_DS) {
			fimc_is_scaler_set_ds_enable(hw_ip->regs, false);
			msdbg_hw(2, "DS off\n", frame->instance, hw_ip);
		}
	}
	spin_unlock_irqrestore(&mcsc_out_slock, flag);
	msdbg_hw(2, "[OUT:%d]mcsc_wdma_clear: en(%d)[F:%d][T:%d][cmd:%d][addr:0x0]\n",
		frame->instance, hw_ip, out_id, wdma_enable, frame->fcount, frame->type,
		param->output[out_id].dma_cmd);
}

static void fimc_is_hw_mcsc_wdma_cfg(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame)
{
	struct mcs_param *param;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	struct fimc_is_hw_mcsc *hw_mcsc;
	u32 wdma_addr[MCSC_OUTPUT_MAX][4] = {{0} }, *wdma_base = NULL;
	u32 plane, buf_idx, out_id, i;
	ulong flag;

	BUG_ON(!cap);
	BUG_ON(!hw_ip->priv_info);

	param = &hw_ip->region[frame->instance]->parameter.mcs;
	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	for (out_id = MCSC_OUTPUT0; out_id < cap->max_output; out_id++) {
		if ((cap->out_dma[out_id] != MCSC_CAP_SUPPORT)
			|| !test_bit(out_id, &hw_mcsc->out_en))
			continue;

		wdma_base = hw_mcsc_get_target_addr(out_id, frame);
		msdbg_hw(2, "[F:%d]wdma_cfg [T:%d][addr%d: %x]\n", frame->instance, hw_ip,
			frame->fcount, frame->type, out_id, wdma_base[0]);

		if (param->output[out_id].dma_cmd != DMA_OUTPUT_COMMAND_DISABLE
			&& wdma_base && wdma_base[0]
			&& frame->type != SHOT_TYPE_INTERNAL) {

			spin_lock_irqsave(&mcsc_out_slock, flag);
			if (check_shared_out_running(cap, out_id) && frame->type != SHOT_TYPE_MULTI) {
				mswarn_hw("[OUT:%d]DMA_OUTPUT in running state[F:%d]",
					frame->instance, hw_ip, out_id, frame->fcount);
				spin_unlock_irqrestore(&mcsc_out_slock, flag);
				continue;
			}
			set_bit(out_id, &mcsc_out_st);
			spin_unlock_irqrestore(&mcsc_out_slock, flag);

			msdbg_hw(2, "[OUT:%d]dma_out enabled\n", frame->instance, hw_ip, out_id);
			if (out_id != MCSC_OUTPUT_DS)
				fimc_is_scaler_set_dma_out_enable(hw_ip->regs, out_id, true);

			/* use only one buffer (per-frame) */
			fimc_is_scaler_set_wdma_frame_seq(hw_ip->regs, out_id,
				0x1 << USE_DMA_BUFFER_INDEX);

			plane = param->output[out_id].plane;
			for (buf_idx = 0; buf_idx < frame->num_buffers; buf_idx++) {
				for (i = 0; i < plane; i++) {
					/*
					 * If the number of buffers is not same between leader and subdev,
					 * wdma addresses are forcibly set as the same address of first buffer.
					 */
					wdma_addr[out_id][i] = wdma_base[plane * buf_idx + i] ?
						wdma_base[plane * buf_idx + i] : wdma_base[i];
					dbg_hw(2, "M%dP(P:%d)(A:0x%X)\n", out_id, i, wdma_addr[out_id][i]);
				}
				hw_mcsc_set_wdma_addr(hw_ip, wdma_addr[out_id], out_id, plane, buf_idx);
			}

		} else {
			fimc_is_hw_mcsc_wdma_clear(hw_ip, frame, param, out_id, cap);
		}
	}
}

static int fimc_is_hw_mcsc_shot(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	int ret_internal = 0;
	struct fimc_is_group *head;
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct is_param_region *param;
	struct mcs_param *mcs_param;
	bool start_flag = true;
	u32 instance;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	ulong flag;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);
	FIMC_BUG(!cap);

	hardware = hw_ip->hardware;
	instance = frame->instance;

	msdbgs_hw(2, "[F:%d]shot\n", instance, hw_ip, frame->fcount);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		msdbg_hw(2, "not initialized\n", instance, hw_ip);
		return -EINVAL;
	}

	if ((!test_bit(ENTRY_M0P, &frame->out_flag))
		&& (!test_bit(ENTRY_M1P, &frame->out_flag))
		&& (!test_bit(ENTRY_M2P, &frame->out_flag))
		&& (!test_bit(ENTRY_M3P, &frame->out_flag))
		&& (!test_bit(ENTRY_M4P, &frame->out_flag))
		&& (!test_bit(ENTRY_M5P, &frame->out_flag)))
		set_bit(hw_ip->id, &frame->core_flag);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	param = &hw_ip->region[instance]->parameter;
	mcs_param = &param->mcs;

	head = hw_ip->group[frame->instance]->head;

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state)) {
		if (!test_bit(HW_CONFIG, &hw_ip->state)
			&& !atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
			start_flag = true;
		else
			start_flag = false;
	} else {
		start_flag = true;
	}

	if (frame->type == SHOT_TYPE_INTERNAL) {
		msdbg_hw(2, "request not exist\n", instance, hw_ip);
		goto config;
	}

	hw_mcsc->back_param = param;

	spin_lock_irqsave(&mcsc_out_slock, flag);

	fimc_is_hw_mcsc_update_param(hw_ip, mcs_param, instance);

	spin_unlock_irqrestore(&mcsc_out_slock, flag);

	msdbg_hw(2, "[F:%d]shot [T:%d]\n", instance, hw_ip, frame->fcount, frame->type);

config:
	/* multi-buffer */
	hw_ip->cur_s_int = 0;
	hw_ip->cur_e_int = 0;
	if (frame->num_buffers)
		hw_ip->num_buffers = frame->num_buffers;
	if (frame->num_buffers > 1)
		hw_ip->mframe = frame;

	/* RDMA cfg */
	ret = fimc_is_hw_mcsc_rdma_cfg(hw_ip, frame, &mcs_param->input);
	if (ret) {
		mserr_hw("[F:%d]mcsc rdma_cfg failed\n",
				instance, hw_ip, frame->fcount);
		return ret;
	}

	/* WDMA cfg */
	fimc_is_hw_mcsc_wdma_cfg(hw_ip, frame);
	fimc_is_scaler_set_lfro_mode_enable(hw_ip->regs, hw_ip->id, hardware->hw_fro_en, frame->num_buffers);

	ret_internal = fimc_is_hw_mcsc_update_dsvra_register(hw_ip, head, mcs_param, instance, frame->shot);
	ret_internal = fimc_is_hw_mcsc_update_tdnr_register(hw_ip, frame, param, start_flag);
	ret_internal = fimc_is_hw_mcsc_update_cac_register(hw_ip, frame, instance);
	ret_internal = fimc_is_hw_mcsc_update_uvsp_register(hw_ip, frame, instance);
	ret_internal = fimc_is_hw_mcsc_update_ysum_register(hw_ip, head, mcs_param, instance, frame->shot);
	if (ret_internal) {
		msdbg_hw(2, "ysum cfg is failed\n", instance, hw_ip);
		fimc_is_scaler_set_ysum_enable(hw_ip->regs, false);
	}

	/* for set shadow register write start
	 * only group input is OTF && not async shot
	 */
	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state) && !start_flag)
		fimc_is_scaler_set_shadow_ctrl(hw_ip->regs, hw_ip->id, SHADOW_WRITE_FINISH);

	if (start_flag) {
		msdbg_hw(2, "[F:%d]start\n", instance, hw_ip, frame->fcount);
		fimc_is_scaler_start(hw_ip->regs, hw_ip->id);
		fimc_is_scaler_set_tdnr_rdma_start(hw_ip->regs, hw_mcsc->cur_tdnr_mode);
		if (mcs_param->input.dma_cmd == DMA_INPUT_COMMAND_ENABLE && cap->in_dma == MCSC_CAP_SUPPORT)
			fimc_is_scaler_rdma_start(hw_ip->regs, hw_ip->id);
	}

	msdbg_hw(2, "shot: mcsc_out_st[0x%lx]\n", instance, hw_ip,
		mcsc_out_st);

	clear_bit(MCSC_RST_CHK, &mcsc_out_st);
	set_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_mcsc_set_param(struct fimc_is_hw_ip *hw_ip, struct is_region *region,
	u32 lindex, u32 hindex, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!region);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	hw_ip->region[instance] = region;
	hw_ip->lindex[instance] = lindex;
	hw_ip->hindex[instance] = hindex;

	/* To execute update_register function in hw_mcsc_shot(),
	 * the value of hw_mcsc->instance is set as default.
	 */
	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	hw_mcsc->instance = FIMC_IS_STREAM_COUNT;

	return ret;
}

void fimc_is_hw_mcsc_check_size(struct fimc_is_hw_ip *hw_ip, struct mcs_param *param,
	u32 instance, u32 output_id)
{
	struct param_mcs_input *input = &param->input;
	struct param_mcs_output *output = &param->output[output_id];

	minfo_hw("[OUT:%d]>>> hw_mcsc_check_size >>>\n", instance, output_id);
	info_hw("otf_input: format(%d),size(%dx%d)\n",
		input->otf_format, input->width, input->height);
	info_hw("dma_input: format(%d),crop_size(%dx%d)\n",
		input->dma_format, input->dma_crop_width, input->dma_crop_height);
	info_hw("output: otf_cmd(%d),dma_cmd(%d),format(%d),stride(y:%d,c:%d)\n",
		output->otf_cmd, output->dma_cmd, output->dma_format,
		output->dma_stride_y, output->dma_stride_c);
	info_hw("output: pos(%d,%d),crop%dx%d),size(%dx%d)\n",
		output->crop_offset_x, output->crop_offset_y,
		output->crop_width, output->crop_height,
		output->width, output->height);
	info_hw("[%d]<<< hw_mcsc_check_size <<<\n", instance);
}

int fimc_is_hw_mcsc_update_register(struct fimc_is_hw_ip *hw_ip,
	struct mcs_param *param, u32 output_id, u32 instance)
{
	int ret = 0;

	if (output_id == MCSC_OUTPUT_DS)
		return ret;

	hw_mcsc_check_size(hw_ip, param, instance, output_id);
	ret = fimc_is_hw_mcsc_poly_phase(hw_ip, &param->input,
			&param->output[output_id], output_id, instance);
	ret = fimc_is_hw_mcsc_post_chain(hw_ip, &param->input,
			&param->output[output_id], output_id, instance);
	ret = fimc_is_hw_mcsc_flip(hw_ip, &param->output[output_id], output_id, instance);
	ret = fimc_is_hw_mcsc_otf_output(hw_ip, &param->output[output_id], output_id, instance);
	ret = fimc_is_hw_mcsc_dma_output(hw_ip, &param->output[output_id], output_id, instance);
	ret = fimc_is_hw_mcsc_output_yuvrange(hw_ip, &param->output[output_id], output_id, instance);
	ret = fimc_is_hw_mcsc_hwfc_output(hw_ip, &param->output[output_id], output_id, instance);

	return ret;
}

int fimc_is_hw_mcsc_update_param(struct fimc_is_hw_ip *hw_ip,
	struct mcs_param *param, u32 instance)
{
	int i = 0;
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	u32 dma_output_ids = 0;
	u32 hwfc_output_ids = 0;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param);
	FIMC_BUG(!cap);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (hw_mcsc->instance != instance) {
		msdbg_hw(2, "update_param: hw_ip->instance(%d)\n",
			instance, hw_ip, hw_mcsc->instance);
		hw_mcsc->instance = instance;
	}

	ret |= fimc_is_hw_mcsc_otf_input(hw_ip, &param->input, instance);
	ret |= fimc_is_hw_mcsc_dma_input(hw_ip, &param->input, instance);

	fimc_is_hw_mcsc_update_djag_register(hw_ip, param, instance);	/* for DZoom */

	for (i = MCSC_OUTPUT0; i < cap->max_output; i++) {
		ret |= fimc_is_hw_mcsc_update_register(hw_ip, param, i, instance);
		fimc_is_scaler_set_wdma_pri(hw_ip->regs, i, param->output[i].plane);	/* FIXME: */

		/* check the hwfc enable in all output */
		if (param->output[i].hwfc)
			hwfc_output_ids |= (1 << i);

		if (param->output[i].dma_cmd == DMA_OUTPUT_COMMAND_ENABLE)
			dma_output_ids |= (1 << i);
	}

	/* setting for hwfc */
	ret |= fimc_is_hw_mcsc_hwfc_mode(hw_ip, &param->input, hwfc_output_ids, dma_output_ids, instance);

	hw_mcsc->prev_hwfc_output_ids = hwfc_output_ids;

	if (ret)
		fimc_is_hw_mcsc_size_dump(hw_ip);

	return ret;
}

int fimc_is_hw_mcsc_reset(struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;
	u32 output_id;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	struct fimc_is_hw_ip *hw_ip_ = NULL;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!cap);

	if (check_sc_core_running(hw_ip, cap))
		return 0;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (hw_ip) {
		sinfo_hw("hw_mcsc_reset: out_en[0x%lx]\n", hw_ip, hw_mcsc->out_en);
		ret = fimc_is_scaler_sw_reset(hw_ip->regs, hw_ip->id, 0, 0);
		if (ret != 0) {
			serr_hw("sw reset fail", hw_ip);
			return -ENODEV;
		}

		/* shadow ctrl register clear */
		fimc_is_scaler_clear_shadow_ctrl(hw_ip->regs, hw_ip->id);
	}

	hw_ip_ = get_mcsc_hw_ip(hw_ip);
	if (hw_ip_) {
		sinfo_hw("hw_mcsc_reset: out_en[0x%lx]\n", hw_ip_, hw_mcsc->out_en);
		ret = fimc_is_scaler_sw_reset(hw_ip_->regs, hw_ip_->id, 0, 0);
		if (ret != 0) {
			serr_hw("sw reset fail", hw_ip_);
			return -ENODEV;
		}

		/* shadow ctrl register clear */
		fimc_is_scaler_clear_shadow_ctrl(hw_ip_->regs, hw_ip_->id);
	}

	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++) {
		if (cap->enable_shared_output == false || test_bit(output_id, &mcsc_out_st)) {
			sinfo_hw("[OUT:%d]set output clear\n", hw_ip, output_id);
			fimc_is_scaler_set_poly_scaler_enable(hw_ip->regs, hw_ip->id, output_id, 0);
			fimc_is_scaler_set_post_scaler_enable(hw_ip->regs, output_id, 0);
			fimc_is_scaler_set_otf_out_enable(hw_ip->regs, output_id, false);
			fimc_is_scaler_set_dma_out_enable(hw_ip->regs, output_id, false);

			fimc_is_scaler_set_wdma_pri(hw_ip->regs, output_id, 0);	/* FIXME: */
			fimc_is_scaler_set_wdma_axi_pri(hw_ip->regs);		/* FIXME: */
			fimc_is_scaler_set_wdma_sram_base(hw_ip->regs, output_id);
			clear_bit(output_id, &mcsc_out_st);
		}
	}
	fimc_is_scaler_set_wdma_sram_base(hw_ip->regs, MCSC_OUTPUT_SSB);	/* FIXME: */

	/* for tdnr */
	if (cap->tdnr == MCSC_CAP_SUPPORT)
		fimc_is_scaler_set_tdnr_wdma_sram_base(hw_ip->regs, TDNR_WEIGHT);

	set_bit(MCSC_RST_CHK, &mcsc_out_st);

	if (cap->in_otf == MCSC_CAP_SUPPORT) {
		if (hw_ip)
			fimc_is_scaler_set_stop_req_post_en_ctrl(hw_ip->regs, hw_ip->id, 0);

		if (hw_ip_)
			fimc_is_scaler_set_stop_req_post_en_ctrl(hw_ip_->regs, hw_ip_->id, 0);
	}

	return ret;
}

int fimc_is_hw_mcsc_clear_interrupt(struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	sinfo_hw("hw_mcsc_clear_interrupt\n", hw_ip);

	fimc_is_scaler_clear_intr_all(hw_ip->regs, hw_ip->id);
	fimc_is_scaler_disable_intr(hw_ip->regs, hw_ip->id);
	fimc_is_scaler_mask_intr(hw_ip->regs, hw_ip->id, MCSC_INTR_MASK);

	return ret;
}

static int fimc_is_hw_mcsc_load_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_ip_setfile *setfile;
	struct hw_mcsc_setfile *setfile_addr;
	enum exynos_sensor_position sensor_position;
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;
	int setfile_index = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		msdbg_hw(2, "%s: hw_map(0x%lx)\n", instance, hw_ip, __func__, hw_map);
		return 0;
	}

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -ESRCH;
	}

	if (!unlikely(hw_ip->priv_info)) {
		mserr_hw("priv_info is NULL", instance, hw_ip);
		return -EINVAL;
	}
	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	switch (setfile->version) {
	case SETFILE_V2:
		break;
	case SETFILE_V3:
		break;
	default:
		mserr_hw("invalid version (%d)", instance, hw_ip,
			setfile->version);
		return -EINVAL;
	}

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (setfile->table[0].size != sizeof(struct hw_mcsc_setfile))
		mswarn_hw("tuneset size(%x) is not matched to setfile structure size(%lx)",
			instance, hw_ip, setfile->table[0].size,
			sizeof(struct hw_mcsc_setfile));

	/* copy MCSC setfile set */
	setfile_addr = (struct hw_mcsc_setfile *)setfile->table[0].addr;
	memcpy(hw_mcsc->setfile[sensor_position], setfile_addr,
		sizeof(struct hw_mcsc_setfile) * setfile->using_count);

	/* check each setfile Magic numbers */
	for (setfile_index = 0; setfile_index < setfile->using_count; setfile_index++) {
		setfile_addr = &hw_mcsc->setfile[sensor_position][setfile_index];

		if (setfile_addr->setfile_version != MCSC_SETFILE_VERSION) {
			mserr_hw("setfile[%d] version(0x%x) is incorrect",
				instance, hw_ip, setfile_index,
				setfile_addr->setfile_version);
			return -EINVAL;
		}
	}
#if defined(USE_UVSP_CAC)
	hw_mcsc->uvsp_ctrl.biquad_a = hw_ip->hardware->cal_info[sensor_position].data[2];
	hw_mcsc->uvsp_ctrl.biquad_b = hw_ip->hardware->cal_info[sensor_position].data[3];
#endif

	set_bit(HW_TUNESET, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_mcsc_apply_setfile(struct fimc_is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;
	struct fimc_is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	u32 setfile_index = 0;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!cap);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		msdbg_hw(2, "%s: hw_map(0x%lx)\n", instance, hw_ip, __func__, hw_map);
		return 0;
	}

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -ESRCH;
	}

	if (!unlikely(hw_ip->priv_info)) {
		mserr_hw("priv info is NULL", instance, hw_ip);
		return -EINVAL;
	}

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	setfile_index = setfile->index[scenario];
	if (setfile_index >= setfile->using_count) {
		mserr_hw("setfile index is out-of-range, [%d:%d]",
				instance, hw_ip, scenario, setfile_index);
		return -EINVAL;
	}

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	hw_mcsc->cur_setfile[sensor_position][instance] =
		&hw_mcsc->setfile[sensor_position][setfile_index];

	msinfo_hw("setfile (%d) scenario (%d)\n", instance, hw_ip,
		setfile_index, scenario);

	return ret;
}

static int fimc_is_hw_mcsc_delete_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance,
	ulong hw_map)
{

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		msdbg_hw(2, "%s: hw_map(0x%lx)\n", instance, hw_ip, __func__, hw_map);
		return 0;
	}

	if (!test_bit(HW_TUNESET, &hw_ip->state)) {
		msdbg_hw(2, "setfile already deleted", instance, hw_ip);
		return 0;
	}

	clear_bit(HW_TUNESET, &hw_ip->state);

	return 0;
}

int hw_mcsc_chk_frame_done(struct fimc_is_hw_ip *hw_ip,
	u32 out_id, u32 *wq_id, u32 *out_f_id)
{
	switch (out_id) {
	case MCSC_OUTPUT0:
		*wq_id = WORK_M0P_FDONE;
		*out_f_id = ENTRY_M0P;
		break;
	case MCSC_OUTPUT1:
		*wq_id = WORK_M1P_FDONE;
		*out_f_id = ENTRY_M1P;
		break;
	case MCSC_OUTPUT2:
		*wq_id = WORK_M2P_FDONE;
		*out_f_id = ENTRY_M2P;
		break;
	case MCSC_OUTPUT3:
		*wq_id = WORK_M3P_FDONE;
		*out_f_id = ENTRY_M3P;
		break;
	case MCSC_OUTPUT4:
		*wq_id = WORK_M4P_FDONE;
		*out_f_id = ENTRY_M4P;
		break;
	case MCSC_OUTPUT5:
		*wq_id = WORK_M5P_FDONE;
		*out_f_id = ENTRY_M5P;
		break;
	default:
		swarn_hw("invalid output_id(%d)\n", hw_ip, out_id);
		return -1;
	}

	return 0;
}

void fimc_is_hw_mcsc_frame_done(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	int done_type)
{
	int ret = 0;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *hw_frame;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	u32 index, dbg_pt = DEBUG_POINT_FRAME_END, out_id;
	u32 wq_id = WORK_MAX_MAP, out_f_id = ENTRY_END;
	int instance = atomic_read(&hw_ip->instance);
	bool flag_get_meta = true;
	ulong flags = 0;

	FIMC_BUG_VOID(!hw_ip->priv_info);
	FIMC_BUG_VOID(!cap);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (test_and_clear_bit(HW_MCS_DS_CFG, &hw_ip->state)) {
		fimc_is_scaler_set_dma_out_enable(hw_ip->regs, MCSC_OUTPUT_DS, false);
		fimc_is_scaler_set_ds_enable(hw_ip->regs, false);
	}
	if (test_and_clear_bit(HW_MCS_YSUM_CFG, &hw_ip->state))
		fimc_is_scaler_set_ysum_enable(hw_ip->regs, false);

	switch (done_type) {
	case IS_SHOT_SUCCESS:
		framemgr = hw_ip->framemgr;
		framemgr_e_barrier_common(framemgr, 0, flags);
		hw_frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
		framemgr_x_barrier_common(framemgr, 0, flags);
		if (hw_frame == NULL) {
			mserr_hw("[F:%d] frame(null) @FS_HW_WAIT_DONE!!", instance,
				hw_ip, atomic_read(&hw_ip->fcount));
			framemgr_e_barrier_common(framemgr, 0, flags);
			print_all_hw_frame_count(hw_ip->hardware);
			framemgr_x_barrier_common(framemgr, 0, flags);
			return;
		}
		break;
	case IS_SHOT_UNPROCESSED:
	case IS_SHOT_LATE_FRAME:
	case IS_SHOT_OVERFLOW:
	case IS_SHOT_INVALID_FRAMENUMBER:
	case IS_SHOT_TIMEOUT:
		if (frame == NULL) {
			mserr_hw("[F:%d] frame(null) type(%d)", instance,
				hw_ip, atomic_read(&hw_ip->fcount), done_type);
			return;
		}
		hw_frame = frame;
		break;
	default:
		mserr_hw("[F:%d] invalid done type(%d)\n", instance, hw_ip,
			atomic_read(&hw_ip->fcount), done_type);
		return;
	}

	msdbgs_hw(2, "frame done[F:%d][O:0x%lx]\n", instance, hw_ip,
		hw_frame->fcount, hw_frame->out_flag);

	for (out_id = MCSC_OUTPUT0; out_id < cap->max_output; out_id++) {
		ret = hw_mcsc_chk_frame_done(hw_ip, out_id, &wq_id, &out_f_id);
		if (ret)
			continue;

		if (test_bit(out_f_id, &hw_frame->out_flag)) {
			ret = fimc_is_hardware_frame_done(hw_ip, frame,
					wq_id, out_f_id, done_type, flag_get_meta);
			clear_bit(out_id, &mcsc_out_st);
			flag_get_meta = false;
			dbg_pt = DEBUG_POINT_FRAME_DMA_END;
		}
	}

	index = hw_ip->debug_index[1];
	hw_ip->debug_info[index].cpuid[dbg_pt] = raw_smp_processor_id();
	hw_ip->debug_info[index].time[dbg_pt] = cpu_clock(raw_smp_processor_id());

	dbg_isr("[F:%d][S-E] %05llu us\n", hw_ip, atomic_read(&hw_ip->fcount),
		(hw_ip->debug_info[index].time[dbg_pt] -
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_START]) / 1000);

	if (!flag_get_meta)
		atomic_inc(&hw_ip->count.dma);

	if (flag_get_meta && done_type == IS_SHOT_SUCCESS)
		fimc_is_hardware_frame_done(hw_ip, NULL, -1, FIMC_IS_HW_CORE_END,
				IS_SHOT_SUCCESS, flag_get_meta);

	return;
}

static int fimc_is_hw_mcsc_frame_ndone(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	u32 instance, enum ShotErrorType done_type)
{
	int ret = 0;

	fimc_is_hw_mcsc_frame_done(hw_ip, frame, done_type);

	if (test_bit_variables(hw_ip->id, &frame->core_flag))
		ret = fimc_is_hardware_frame_done(hw_ip, frame, -1, FIMC_IS_HW_CORE_END,
				done_type, false);

	return ret;
}

int fimc_is_hw_mcsc_otf_input(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 instance)
{
	int ret = 0;
	u32 width, height;
	u32 format, bit_width;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!input);
	FIMC_BUG(!cap);

	/* can't support this function */
	if (cap->in_otf != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	msdbg_hw(2, "otf_input_setting cmd(%d)\n", instance, hw_ip, input->otf_cmd);
	width  = input->width;
	height = input->height;
	format = input->otf_format;
	bit_width = input->otf_bitwidth;

	if (input->otf_cmd == OTF_INPUT_COMMAND_DISABLE)
		return ret;

	ret = fimc_is_hw_mcsc_check_format(HW_MCSC_OTF_INPUT,
		format, bit_width, width, height);
	if (ret) {
		mserr_hw("Invalid OTF Input format: fmt(%d),bit(%d),size(%dx%d)",
			instance, hw_ip, format, bit_width, width, height);
		return ret;
	}

	fimc_is_scaler_set_input_source(hw_ip->regs, hw_ip->id, !input->otf_cmd);
	fimc_is_scaler_set_input_img_size(hw_ip->regs, hw_ip->id, width, height);
	fimc_is_scaler_set_dither(hw_ip->regs, hw_ip->id, 0);
	fimc_is_scaler_set_rdma_format(hw_ip->regs, hw_ip->id, format);

	return ret;
}

int fimc_is_hw_mcsc_dma_input(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 instance)
{
	int ret = 0;
	u32 width, height;
	u32 format, bit_width, plane, order;
	u32 y_stride, uv_stride;
	u32 img_format;
	u32 y_2bit_stride = 0, uv_2bit_stride = 0;
	u32 img_10bit_type = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!input);
	FIMC_BUG(!cap);

	/* can't support this function */
	if (cap->in_dma != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	msdbg_hw(2, "dma_input_setting cmd(%d)\n", instance, hw_ip, input->dma_cmd);
	width  = input->dma_crop_width;
	height = input->dma_crop_height;
	format = input->dma_format;
	bit_width = input->dma_bitwidth;
	plane = input->plane;
	order = input->dma_order;
	y_stride = input->dma_stride_y;
	uv_stride = input->dma_stride_c;

	if (input->dma_cmd == DMA_INPUT_COMMAND_DISABLE)
		return ret;

	ret = fimc_is_hw_mcsc_check_format(HW_MCSC_DMA_INPUT,
		format, bit_width, width, height);
	if (ret) {
		mserr_hw("Invalid DMA Input format: fmt(%d),bit(%d),size(%dx%d)",
			instance, hw_ip, format, bit_width, width, height);
		return ret;
	}

	fimc_is_scaler_set_input_source(hw_ip->regs, hw_ip->id, input->dma_cmd);
	fimc_is_scaler_set_input_img_size(hw_ip->regs, hw_ip->id, width, height);
	fimc_is_scaler_set_dither(hw_ip->regs, hw_ip->id, 0);

	fimc_is_scaler_set_rdma_stride(hw_ip->regs, y_stride, uv_stride);

	ret = fimc_is_hw_mcsc_adjust_input_img_fmt(format, plane, order, &img_format);
	if (ret < 0) {
		mswarn_hw("Invalid rdma image format", instance, hw_ip);
		img_format = hw_mcsc->in_img_format;
	} else {
		hw_mcsc->in_img_format = img_format;
	}

	fimc_is_scaler_set_rdma_size(hw_ip->regs, width, height);
	fimc_is_scaler_set_rdma_format(hw_ip->regs, hw_ip->id, img_format);

	if (bit_width == DMA_INPUT_BIT_WIDTH_16BIT)
		img_10bit_type = 2;
	else if (bit_width == DMA_INPUT_BIT_WIDTH_10BIT)
		if (plane == DMA_INPUT_PLANE_4)
			img_10bit_type = 1;
		else
			img_10bit_type = 3;
	else
		img_10bit_type = 0;
	fimc_is_scaler_set_rdma_10bit_type(hw_ip->regs, img_10bit_type);

	if (input->plane == DMA_OUTPUT_PLANE_4) {
		y_2bit_stride = ALIGN(width / 4, 16);
		uv_2bit_stride = ALIGN(width / 4, 16);
		fimc_is_scaler_set_rdma_2bit_stride(hw_ip->regs, y_2bit_stride, uv_2bit_stride);
	}

	return ret;
}


int fimc_is_hw_mcsc_poly_phase(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, u32 output_id, u32 instance)
{
	int ret = 0;
	u32 src_pos_x, src_pos_y, src_width, src_height;
	u32 poly_dst_width, poly_dst_height;
	u32 out_width, out_height;
	ulong temp_width, temp_height;
	u32 hratio, vratio;
	u32 input_id = 0;
	bool config = true;
	bool post_en = false;
	bool round_mode_en = true;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	enum exynos_sensor_position sensor_position;
	struct hw_mcsc_setfile *setfile;
	struct scaler_coef_cfg *sc_coef;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!input);
	FIMC_BUG(!output);
	FIMC_BUG(!hw_ip->priv_info);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	msdbg_hw(2, "[OUT:%d]poly_phase_setting cmd(O:%d,D:%d)\n",
		instance, hw_ip, output_id, output->otf_cmd, output->dma_cmd);

	input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
	config = (input_id == hw_ip->id ? true: false);

	if (output->otf_cmd == OTF_OUTPUT_COMMAND_DISABLE
		&& output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		if (cap->enable_shared_output == false
			|| (config && !test_bit(output_id, &mcsc_out_st)))
			fimc_is_scaler_set_poly_scaler_enable(hw_ip->regs, hw_ip->id, output_id, 0);
		return ret;
	}

	fimc_is_scaler_set_poly_scaler_enable(hw_ip->regs, hw_ip->id, output_id, 1);

	src_pos_x = output->crop_offset_x;
	src_pos_y = output->crop_offset_y;
	src_width = output->crop_width;
	src_height = output->crop_height;

	fimc_is_hw_mcsc_adjust_size_with_djag(hw_ip, input, cap, &src_pos_x, &src_pos_y, &src_width, &src_height);

	out_width = output->width;
	out_height = output->height;

	fimc_is_scaler_set_poly_src_size(hw_ip->regs, output_id, src_pos_x, src_pos_y,
		src_width, src_height);

	if (((src_width <= (out_width * MCSC_POLY_QUALITY_RATIO_DOWN))
		&& (out_width <= (src_width * MCSC_POLY_RATIO_UP)))
		|| (output_id == MCSC_OUTPUT3 || output_id == MCSC_OUTPUT4)) {
		poly_dst_width = out_width;
		post_en = false;
	} else if ((src_width <= (out_width * MCSC_POLY_QUALITY_RATIO_DOWN * MCSC_POST_RATIO_DOWN))
		&& ((out_width * MCSC_POLY_QUALITY_RATIO_DOWN) < src_width)) {
		poly_dst_width = MCSC_ROUND_UP(src_width / MCSC_POLY_QUALITY_RATIO_DOWN, 2);
		if (poly_dst_width > MCSC_POST_MAX_WIDTH)
			poly_dst_width = MCSC_POST_MAX_WIDTH;
		post_en = true;
	} else if ((src_width <= (out_width * MCSC_POLY_RATIO_DOWN * MCSC_POST_RATIO_DOWN))
		&& ((out_width *  MCSC_POLY_QUALITY_RATIO_DOWN * MCSC_POST_RATIO_DOWN) < src_width)) {
		poly_dst_width = MCSC_ROUND_UP(out_width * MCSC_POST_RATIO_DOWN, 2);
		if (poly_dst_width > MCSC_POST_MAX_WIDTH)
			poly_dst_width = MCSC_POST_MAX_WIDTH;
		post_en = true;
	} else {
		mserr_hw("hw_mcsc_poly_phase: Unsupported W ratio, (%dx%d)->(%dx%d)\n",
			instance, hw_ip, src_width, src_height, out_width, out_height);
		poly_dst_width = MCSC_ROUND_UP(src_width / MCSC_POLY_RATIO_DOWN, 2);
		post_en = true;
	}

	if (((src_height <= (out_height * MCSC_POLY_QUALITY_RATIO_DOWN))
		&& (out_height <= (src_height * MCSC_POLY_RATIO_UP)))
		|| (output_id == MCSC_OUTPUT3 || output_id == MCSC_OUTPUT4)) {
		poly_dst_height = out_height;
		post_en = false;
	} else if ((src_height <= (out_height * MCSC_POLY_QUALITY_RATIO_DOWN * MCSC_POST_RATIO_DOWN))
		&& ((out_height * MCSC_POLY_QUALITY_RATIO_DOWN) < src_height)) {
		poly_dst_height = (src_height / MCSC_POLY_QUALITY_RATIO_DOWN);
		post_en = true;
	} else if ((src_height <= (out_height * MCSC_POLY_RATIO_DOWN * MCSC_POST_RATIO_DOWN))
		&& ((out_height * MCSC_POLY_QUALITY_RATIO_DOWN * MCSC_POST_RATIO_DOWN) < src_height)) {
		poly_dst_height = (out_height * MCSC_POST_RATIO_DOWN);
		post_en = true;
	} else {
		mserr_hw("hw_mcsc_poly_phase: Unsupported H ratio, (%dx%d)->(%dx%d)\n",
			instance, hw_ip, src_width, src_height, out_width, out_height);
		poly_dst_height = (src_height / MCSC_POLY_RATIO_DOWN);
		post_en = true;
	}
#if defined(MCSC_POST_WA)
	/* The post scaler guarantee the quality of image          */
	/*  in case the scaling ratio equals to multiple of x1/256 */
	if ((post_en && ((poly_dst_width << MCSC_POST_WA_SHIFT) % out_width))
		|| (post_en && ((poly_dst_height << MCSC_POST_WA_SHIFT) % out_height))) {
		u32 multiple_hratio = 1;
		u32 multiple_vratio = 1;
		u32 shift = 0;
		for (shift = 0; shift <= MCSC_POST_WA_SHIFT; shift++) {
			if (((out_width % (1 << (MCSC_POST_WA_SHIFT-shift))) == 0)
				&& (out_height % (1 << (MCSC_POST_WA_SHIFT-shift)) == 0)) {
				multiple_hratio = out_width  / (1 << (MCSC_POST_WA_SHIFT-shift));
				multiple_vratio = out_height / (1 << (MCSC_POST_WA_SHIFT-shift));
				break;
			}
		}
		msdbg_hw(2, "[OUT:%d]poly_phase: shift(%d), ratio(%d,%d), "
			"size(%dx%d) before calculation\n",
			instance, hw_ip, output_id, shift, multiple_hratio, multiple_hratio,
			poly_dst_width, poly_dst_height);
		poly_dst_width  = MCSC_ROUND_UP(poly_dst_width, multiple_hratio);
		poly_dst_height = MCSC_ROUND_UP(poly_dst_height, multiple_vratio);
		if (poly_dst_width % 2) {
			poly_dst_width  = poly_dst_width  + multiple_hratio;
			poly_dst_height = poly_dst_height + multiple_vratio;
		}
		msdbg_hw(2, "[OUT:%d]poly_phase: size(%dx%d) after  calculation\n",
			instance, hw_ip, output_id, poly_dst_width, poly_dst_height);
	}
#endif

	fimc_is_scaler_set_poly_dst_size(hw_ip->regs, output_id,
		poly_dst_width, poly_dst_height);

	temp_width  = (ulong)src_width;
	temp_height = (ulong)src_height;
	hratio = (u32)((temp_width  << MCSC_PRECISION) / poly_dst_width);
	vratio = (u32)((temp_height << MCSC_PRECISION) / poly_dst_height);

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = hw_mcsc->cur_setfile[sensor_position][instance];
#if defined(MCSC_COEF_USE_TUNING)
	sc_coef = &setfile->sc_coef;
#else
	sc_coef = NULL;
#endif
	fimc_is_scaler_set_poly_scaling_ratio(hw_ip->regs, output_id, hratio, vratio);
	fimc_is_scaler_set_poly_scaler_coef(hw_ip->regs, output_id, hratio, vratio,
		sc_coef, sensor_position);
	fimc_is_scaler_set_poly_round_mode(hw_ip->regs, output_id, round_mode_en);

	return ret;
}

int fimc_is_hw_mcsc_post_chain(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, u32 output_id, u32 instance)
{
	int ret = 0;
	u32 img_width, img_height;
	u32 dst_width, dst_height;
	ulong temp_width, temp_height;
	u32 hratio, vratio;
	u32 input_id = 0;
	bool config = true;
	bool round_mode_en = true;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	enum exynos_sensor_position sensor_position;
	struct hw_mcsc_setfile *setfile;
	struct scaler_coef_cfg *sc_coef;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!input);
	FIMC_BUG(!output);
	FIMC_BUG(!hw_ip->priv_info);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	msdbg_hw(2, "[OUT:%d]post_chain_setting cmd(O:%d,D:%d)\n",
		instance, hw_ip, output_id, output->otf_cmd, output->dma_cmd);

	input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
	config = (input_id == hw_ip->id ? true: false);

	if (output->otf_cmd == OTF_OUTPUT_COMMAND_DISABLE
		&& output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		if (cap->enable_shared_output == false
			|| (config && !test_bit(output_id, &mcsc_out_st)))
			fimc_is_scaler_set_post_scaler_enable(hw_ip->regs, output_id, 0);
		return ret;
	}

	fimc_is_scaler_get_poly_dst_size(hw_ip->regs, output_id, &img_width, &img_height);

	dst_width = output->width;
	dst_height = output->height;

	/* if x1 ~ x1/4 scaling, post scaler bypassed */
	if ((img_width == dst_width) && (img_height == dst_height)) {
		fimc_is_scaler_set_post_scaler_enable(hw_ip->regs, output_id, 0);
	} else {
		fimc_is_scaler_set_post_scaler_enable(hw_ip->regs, output_id, 1);
	}

	fimc_is_scaler_set_post_img_size(hw_ip->regs, output_id, img_width, img_height);
	fimc_is_scaler_set_post_dst_size(hw_ip->regs, output_id, dst_width, dst_height);

	temp_width  = (ulong)img_width;
	temp_height = (ulong)img_height;
	hratio = (u32)((temp_width  << MCSC_PRECISION) / dst_width);
	vratio = (u32)((temp_height << MCSC_PRECISION) / dst_height);

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = hw_mcsc->cur_setfile[sensor_position][instance];
#if defined(MCSC_COEF_USE_TUNING)
	sc_coef = &setfile->sc_coef;
#else
	sc_coef = NULL;
#endif
	fimc_is_scaler_set_post_scaling_ratio(hw_ip->regs, output_id, hratio, vratio);
	fimc_is_scaler_set_post_scaler_coef(hw_ip->regs, output_id, hratio, vratio, sc_coef);
	fimc_is_scaler_set_post_round_mode(hw_ip->regs, output_id, round_mode_en);

	return ret;
}

int fimc_is_hw_mcsc_flip(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!output);
	FIMC_BUG(!hw_ip->priv_info);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	msdbg_hw(2, "[OUT:%d]flip_setting flip(%d),cmd(O:%d,D:%d)\n",
		instance, hw_ip, output_id, output->flip, output->otf_cmd, output->dma_cmd);

	if (output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE)
		return ret;

	fimc_is_scaler_set_flip_mode(hw_ip->regs, output_id, output->flip);

	return ret;
}

int fimc_is_hw_mcsc_otf_output(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	u32 out_width, out_height;
	u32 format, bitwidth;
	u32 input_id = 0;
	bool config = true;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!output);
	FIMC_BUG(!cap);
	FIMC_BUG(!hw_ip->priv_info);

	/* can't support this function */
	if (cap->out_otf[output_id] != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	msdbg_hw(2, "[OUT:%d]otf_output_setting cmd(O:%d,D:%d)\n",
		instance, hw_ip, output_id, output->otf_cmd, output->dma_cmd);

	input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
	config = (input_id == hw_ip->id ? true: false);

	if (output->otf_cmd == OTF_OUTPUT_COMMAND_DISABLE) {
		if (cap->enable_shared_output == false
			|| (config && !test_bit(output_id, &mcsc_out_st)))
			fimc_is_scaler_set_otf_out_enable(hw_ip->regs, output_id, false);
		return ret;
	}

	out_width  = output->width;
	out_height = output->height;
	format     = output->otf_format;
	bitwidth  = output->otf_bitwidth;

	ret = fimc_is_hw_mcsc_check_format(HW_MCSC_OTF_OUTPUT,
		format, bitwidth, out_width, out_height);
	if (ret) {
		mserr_hw("[OUT:%d]Invalid MCSC OTF Output format: fmt(%d),bit(%d),size(%dx%d)",
			instance, hw_ip, output_id, format, bitwidth, out_width, out_height);
		return ret;
	}

	fimc_is_scaler_set_otf_out_enable(hw_ip->regs, output_id, true);
	fimc_is_scaler_set_otf_out_path(hw_ip->regs, output_id);

	return ret;
}

int fimc_is_hw_mcsc_dma_output(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	u32 out_width, out_height, scaled_width = 0, scaled_height = 0;
	u32 format, plane, order,bitwidth;
	u32 y_stride, uv_stride;
	u32 y_2bit_stride, uv_2bit_stride;
	u32 img_format;
	u32 input_id = 0;
	bool config = true;
	bool conv420_en = false;
	enum exynos_sensor_position sensor_position;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!output);
	FIMC_BUG(!cap);
	FIMC_BUG(!hw_ip->priv_info);

	/* can't support this function */
	if (cap->out_dma[output_id] != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	sensor_position = hw_ip->hardware->sensor_position[instance];

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	msdbg_hw(2, "[OUT:%d]dma_output_setting cmd(O:%d,D:%d)\n",
		instance, hw_ip, output_id, output->otf_cmd, output->dma_cmd);

	input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
	config = (input_id == hw_ip->id ? true: false);

	if (output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		if (cap->enable_shared_output == false
			|| (config && !test_bit(output_id, &mcsc_out_st)))
			fimc_is_scaler_set_dma_out_enable(hw_ip->regs, output_id, false);
		return ret;
	}

	out_width  = output->width;
	out_height = output->height;
	format     = output->dma_format;
	plane      = output->plane;
	order      = output->dma_order;
	bitwidth   = output->dma_bitwidth;
	y_stride   = output->dma_stride_y;
	uv_stride  = output->dma_stride_c;

	ret = fimc_is_hw_mcsc_check_format(HW_MCSC_DMA_OUTPUT,
		format, bitwidth, out_width, out_height);
	if (ret) {
		mserr_hw("[OUT:%d]Invalid MCSC DMA Output format: fmt(%d),bit(%d),size(%dx%d)",
			instance, hw_ip, output_id, format, bitwidth, out_width, out_height);
		return ret;
	}

	ret = fimc_is_hw_mcsc_adjust_output_img_fmt(format, plane, order,
			&img_format, &conv420_en);
	if (ret < 0) {
		mswarn_hw("Invalid dma image format", instance, hw_ip);
		img_format = hw_mcsc->out_img_format[output_id];
		conv420_en = hw_mcsc->conv420_en[output_id];
	} else {
		hw_mcsc->out_img_format[output_id] = img_format;
		hw_mcsc->conv420_en[output_id] = conv420_en;
	}

	fimc_is_scaler_set_wdma_format(hw_ip->regs, hw_ip->id, output_id, img_format);
	fimc_is_scaler_set_420_conversion(hw_ip->regs, output_id, 0, conv420_en);

	fimc_is_scaler_get_post_dst_size(hw_ip->regs, output_id, &scaled_width, &scaled_height);
	if ((scaled_width != 0) && (scaled_height != 0)) {
		if ((scaled_width != out_width) || (scaled_height != out_height)) {
			msdbg_hw(2, "Invalid output[%d] scaled size (%d/%d)(%d/%d)\n",
				instance, hw_ip, output_id, scaled_width, scaled_height,
				out_width, out_height);
			return -EINVAL;
		}
	}

	fimc_is_scaler_set_wdma_size(hw_ip->regs, output_id, out_width, out_height);

	fimc_is_scaler_set_wdma_stride(hw_ip->regs, output_id, y_stride, uv_stride);

	fimc_is_scaler_set_wdma_10bit_type(hw_ip->regs, output_id, format, bitwidth, sensor_position);

	if (output->plane == DMA_OUTPUT_PLANE_4) {
		y_2bit_stride = ALIGN(out_width / 4, 16);
		uv_2bit_stride = ALIGN(out_width / 4, 16);
		fimc_is_scaler_set_wdma_2bit_stride(hw_ip->regs, output_id, y_2bit_stride, uv_2bit_stride);
	}
	return ret;
}

int fimc_is_hw_mcsc_hwfc_mode(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 hwfc_output_ids, u32 dma_output_ids, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	u32 input_id = 0, output_id;
	bool config = true;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!input);
	FIMC_BUG(!cap);
	FIMC_BUG(!hw_ip->priv_info);

	/* can't support this function */
	if (cap->hwfc != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (cap->enable_shared_output && !hwfc_output_ids)
		return 0;

	/* skip hwfc mode when..
	 *  - one core share Preview, Reprocessing
	 *  - at Preview stream, DMA output port shared to previous reprocessing port
	 *  ex> at 1x3 scaler,
	 *     1. preview - output 0 used, reprocessing - output 1, 2 used
	 *        above output is not overlapped
	 *        at this case, when CAPTURE -> PREVIEW, preview shot should not set hwfc_mode
	 *        due to not set hwfc_mode off during JPEG is operating.
	 *     2. preview - output 0, 1 used (1: preview callback), reprocessing - output 1, 2 used
	 *        above output is overlapped "output 1"
	 *        at this case, when CAPTURE -> PREVIEW, preview shot shuold set hwfc_mode to "0"
	 *        for avoid operate at Preview stream.
	 */
	if (!hw_mcsc->rep_flag[instance] && !(hw_mcsc->prev_hwfc_output_ids & dma_output_ids))
		return ret;

	msdbg_hw(2, "hwfc_mode_setting output[0x%08X]\n", instance, hw_ip, hwfc_output_ids);

	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++) {
		input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
		config = (input_id == hw_ip->id ? true: false);

		if ((config && (hwfc_output_ids & (1 << output_id)))
			|| (fimc_is_scaler_get_dma_out_enable(hw_ip->regs, output_id))) {
			msdbg_hw(2, "hwfc_mode_setting hwfc_output_ids(0x%x)\n",
				instance, hw_ip, hwfc_output_ids);
			fimc_is_scaler_set_hwfc_mode(hw_ip->regs, hwfc_output_ids);
			break;
		}
	}

	if (!config) {
		msdbg_hw(2, "hwfc_mode_setting skip\n", instance, hw_ip);
		return ret;
	}

	return ret;
}

int fimc_is_hw_mcsc_hwfc_output(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	u32 width, height, format, plane;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!output);
	FIMC_BUG(!cap);
	FIMC_BUG(!hw_ip->priv_info);

	/* can't support this function */
	if (cap->out_hwfc[output_id] != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	width  = output->width;
	height = output->height;
	format = output->dma_format;
	plane = output->plane;

	msdbg_hw(2, "hwfc_config_setting mode(%dx%d, %d, %d)\n", instance, hw_ip,
			width, height, format, plane);

	if (output->hwfc)
		fimc_is_scaler_set_hwfc_config(hw_ip->regs, output_id, format, plane,
			(output_id * 3), width, height);

	return ret;
}

void fimc_is_hw_bchs_range(void __iomem *base_addr, u32 output_id, int yuv)
{
	u32 y_ofs, y_gain, c_gain00, c_gain01, c_gain10, c_gain11;

#ifdef ENABLE_10BIT_MCSC
	if (yuv == SCALER_OUTPUT_YUV_RANGE_FULL) {
		/* Y range - [0:1024], U/V range - [0:1024] */
		y_ofs = 0;
		y_gain = 1024;
		c_gain00 = 1024;
		c_gain01 = 0;
		c_gain10 = 0;
		c_gain11 = 1024;
	} else {
		/* YUV_RANGE_NARROW */
		/* Y range - [64:940], U/V range - [64:960] */
		y_ofs = 0;
		y_gain = 1024;
		c_gain00 = 1024;
		c_gain01 = 0;
		c_gain10 = 0;
		c_gain11 = 1024;
	}
#else
	if (yuv == SCALER_OUTPUT_YUV_RANGE_FULL) {
		/* Y range - [0:255], U/V range - [0:255] */
		y_ofs = 0;
		y_gain = 256;
		c_gain00 = 256;
		c_gain01 = 0;
		c_gain10 = 0;
		c_gain11 = 256;
	} else {
		/* YUV_RANGE_NARROW */
		/* Y range - [16:235], U/V range - [16:239] */
		y_ofs = 16;
		y_gain = 220;
		c_gain00 = 224;
		c_gain01 = 0;
		c_gain10 = 0;
		c_gain11 = 224;
	}
#endif
	fimc_is_scaler_set_b_c(base_addr, output_id, y_ofs, y_gain);
	fimc_is_scaler_set_h_s(base_addr, output_id, c_gain00, c_gain01, c_gain10, c_gain11);
}

void fimc_is_hw_bchs_clamp(void __iomem *base_addr, u32 output_id, int yuv,
	struct scaler_bchs_clamp_cfg *sc_bchs)
{
	u32 y_max, y_min, c_max, c_min;

#ifdef ENABLE_10BIT_MCSC
	if (yuv == SCALER_OUTPUT_YUV_RANGE_FULL) {
		y_max = 1023;
		y_min = 0;
		c_max = 1023;
		c_min = 0;
	} else {
		/* YUV_RANGE_NARROW */
		y_max = 940;
		y_min = 64;
		c_max = 960;
		c_min = 64;
	}
#else
	if (yuv == SCALER_OUTPUT_YUV_RANGE_FULL) {
		y_max = 255;
		y_min = 0;
		c_max = 255;
		c_min = 0;
	} else {
		/* YUV_RANGE_NARROW */
		y_max = 235;
		y_min = 16;
		c_max = 240;
		c_min = 16;
	}
#endif
	if (sc_bchs) {
		fimc_is_scaler_set_bchs_clamp(base_addr, output_id,
			sc_bchs->y_max, sc_bchs->y_min,
			sc_bchs->c_max, sc_bchs->c_min);
		dbg_hw(2, "[Y:max(%d),min(%d)][C:max(%d),min(%d)]\n",
			sc_bchs->y_max, sc_bchs->y_min,
			sc_bchs->c_max, sc_bchs->c_min);
	} else {
		fimc_is_scaler_set_bchs_clamp(base_addr, output_id, y_max, y_min, c_max, c_min);
	}
}

int fimc_is_hw_mcsc_output_yuvrange(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	int yuv = 0;
	u32 input_id = 0;
	bool config = true;
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;
	struct hw_mcsc_setfile *setfile;
	enum exynos_sensor_position sensor_position;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	struct scaler_bchs_clamp_cfg *sc_bchs;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!output);
	FIMC_BUG(!hw_ip->priv_info);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
	config = (input_id == hw_ip->id ? true: false);

	if (output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		if (cap->enable_shared_output == false
			|| (config && !test_bit(output_id, &mcsc_out_st)))
			fimc_is_scaler_set_bchs_enable(hw_ip->regs, output_id, 0);
		return ret;
	}

	yuv = output->yuv_range;
	hw_mcsc->yuv_range = yuv; /* save for ISP */

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = hw_mcsc->cur_setfile[sensor_position][instance];
#if defined(MCSC_COEF_USE_TUNING)
	sc_bchs = &setfile->sc_bchs[yuv];
#else
	sc_bchs = NULL;
#endif

	fimc_is_scaler_set_bchs_enable(hw_ip->regs, output_id, 1);
#if !defined(USE_YUV_RANGE_BY_ISP)
	if (test_bit(HW_TUNESET, &hw_ip->state)) {
		/* set yuv range config value by scaler_param yuv_range mode */
		fimc_is_scaler_set_b_c(hw_ip->regs, output_id,
			setfile->sc_base[yuv].y_offset, setfile->sc_base[yuv].y_gain);
		fimc_is_scaler_set_h_s(hw_ip->regs, output_id,
			setfile->sc_base[yuv].c_gain00, setfile->sc_base[yuv].c_gain01,
			setfile->sc_base[yuv].c_gain10, setfile->sc_base[yuv].c_gain11);
		msdbg_hw(2, "set YUV range(%d) by setfile parameter\n",
			instance, hw_ip, yuv);
		msdbg_hw(2, "[OUT:%d]output_yuv_setting: yuv(%d), cmd(O:%d,D:%d)\n",
			instance, hw_ip, output_id, yuv, output->otf_cmd, output->dma_cmd);
		dbg_hw(2, "[Y:offset(%d),gain(%d)][C:gain00(%d),01(%d),10(%d),11(%d)]\n",
			setfile->sc_base[yuv].y_offset, setfile->sc_base[yuv].y_gain,
			setfile->sc_base[yuv].c_gain00, setfile->sc_base[yuv].c_gain01,
			setfile->sc_base[yuv].c_gain10, setfile->sc_base[yuv].c_gain11);
	} else {
		fimc_is_hw_bchs_range(hw_ip->regs, output_id, yuv);
		msdbg_hw(2, "YUV range set default settings\n", instance, hw_ip);
	}
	fimc_is_hw_bchs_clamp(hw_ip->regs, output_id, yuv, sc_bchs);
#else
	fimc_is_hw_bchs_range(hw_ip->regs, output_id, yuv);
	fimc_is_hw_bchs_clamp(hw_ip->regs, output_id, yuv, sc_bchs);
#endif
	return ret;
}

int fimc_is_hw_mcsc_adjust_input_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format)
{
	int ret = 0;

	switch (format) {
	case DMA_INPUT_FORMAT_YUV420:
		switch (plane) {
		case 2:
		case 4:
			switch (order) {
			case DMA_INPUT_ORDER_CbCr:
				*img_format = MCSC_YUV420_2P_UFIRST;
				break;
			case DMA_INPUT_ORDER_CrCb:
				* img_format = MCSC_YUV420_2P_VFIRST;
				break;
			default:
				err_hw("input order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = MCSC_YUV420_3P;
			break;
		default:
			err_hw("input plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	case DMA_INPUT_FORMAT_YUV422:
		switch (plane) {
		case 1:
			switch (order) {
			case DMA_INPUT_ORDER_CrYCbY:
				*img_format = MCSC_YUV422_1P_VYUY;
				break;
			case DMA_INPUT_ORDER_CbYCrY:
				*img_format = MCSC_YUV422_1P_UYVY;
				break;
			case DMA_INPUT_ORDER_YCrYCb:
				*img_format = MCSC_YUV422_1P_YVYU;
				break;
			case DMA_INPUT_ORDER_YCbYCr:
				*img_format = MCSC_YUV422_1P_YUYV;
				break;
			default:
				err_hw("img order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 2:
		case 4:
			switch (order) {
			case DMA_INPUT_ORDER_CbCr:
				*img_format = MCSC_YUV422_2P_UFIRST;
				break;
			case DMA_INPUT_ORDER_CrCb:
				*img_format = MCSC_YUV422_2P_VFIRST;
				break;
			default:
				err_hw("img order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = MCSC_YUV422_3P;
			break;
		default:
			err_hw("img plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	case DMA_INPUT_FORMAT_Y:
		*img_format = MCSC_MONO_Y8;
		break;
	default:
		err_hw("img format error - (%d/%d/%d)", format, order, plane);
		ret = -EINVAL;
		break;
	}

	return ret;
}


int fimc_is_hw_mcsc_adjust_output_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format,
	bool *conv420_flag)
{
	int ret = 0;

	switch (format) {
	case DMA_OUTPUT_FORMAT_YUV420:
		if (conv420_flag)
			*conv420_flag = true;
		switch (plane) {
		case 2:
		case 4:
			switch (order) {
			case DMA_OUTPUT_ORDER_CbCr:
				*img_format = MCSC_YUV420_2P_UFIRST;
				break;
			case DMA_OUTPUT_ORDER_CrCb:
				* img_format = MCSC_YUV420_2P_VFIRST;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = MCSC_YUV420_3P;
			break;
		default:
			err_hw("output plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	case DMA_OUTPUT_FORMAT_YUV422:
		if (conv420_flag)
			*conv420_flag = false;
		switch (plane) {
		case 1:
			switch (order) {
			case DMA_OUTPUT_ORDER_CrYCbY:
				*img_format = MCSC_YUV422_1P_VYUY;
				break;
			case DMA_OUTPUT_ORDER_CbYCrY:
				*img_format = MCSC_YUV422_1P_UYVY;
				break;
			case DMA_OUTPUT_ORDER_YCrYCb:
				*img_format = MCSC_YUV422_1P_YVYU;
				break;
			case DMA_OUTPUT_ORDER_YCbYCr:
				*img_format = MCSC_YUV422_1P_YUYV;
				break;
			default:
				err_hw("img order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 2:
		case 4:
			switch (order) {
			case DMA_OUTPUT_ORDER_CbCr:
				*img_format = MCSC_YUV422_2P_UFIRST;
				break;
			case DMA_OUTPUT_ORDER_CrCb:
				*img_format = MCSC_YUV422_2P_VFIRST;
				break;
			default:
				err_hw("img order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = MCSC_YUV422_3P;
			break;
		default:
			err_hw("img plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	case DMA_OUTPUT_FORMAT_RGB:
		switch (order) {
		case DMA_OUTPUT_ORDER_ARGB:
			*img_format = MCSC_RGB_ARGB8888;
			break;
		case DMA_OUTPUT_ORDER_BGRA:
			*img_format = MCSC_RGB_BGRA8888;
			break;
		case DMA_OUTPUT_ORDER_RGBA:
			*img_format = MCSC_RGB_RGBA8888;
			break;
		case DMA_OUTPUT_ORDER_ABGR:
			*img_format = MCSC_RGB_ABGR8888;
			break;
		default:
			*img_format = MCSC_RGB_RGBA8888;
			break;
		}
		break;
	case DMA_OUTPUT_FORMAT_Y:
		*img_format = MCSC_MONO_Y8;
		break;
	default:
		err_hw("img format error - (%d/%d/%d)", format, order, plane);
		ret = -EINVAL;
		break;
	}

	return ret;
}

int fimc_is_hw_mcsc_check_format(enum mcsc_io_type type, u32 format, u32 bit_width,
	u32 width, u32 height)
{
	int ret = 0;

	switch (type) {
	case HW_MCSC_OTF_INPUT:
		/* check otf input */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input width(%d)", width);
		}

		if (height < 16 || height > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input height(%d)", height);
		}

		if (!(format == OTF_INPUT_FORMAT_YUV422
			|| format == OTF_INPUT_FORMAT_Y)) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input format(%d)", format);
		}

		if (bit_width != OTF_INPUT_BIT_WIDTH_8BIT) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input format(%d)", bit_width);
		}
		break;
	case HW_MCSC_OTF_OUTPUT:
		/* check otf output */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Output width(%d)", width);
		}

		if (height < 16 || height > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Output height(%d)", height);
		}

		if (format != OTF_OUTPUT_FORMAT_YUV422) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Output format(%d)", format);
		}

		if (bit_width != OTF_OUTPUT_BIT_WIDTH_8BIT) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Output format(%d)", bit_width);
		}
		break;
	case HW_MCSC_DMA_INPUT:
		/* check dma input */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Input width(%d)", width);
		}

		if (height < 16 || height > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Input height(%d)", height);
		}

		if (!(format == DMA_INPUT_FORMAT_YUV422
			|| format == DMA_INPUT_FORMAT_Y)) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Input format(%d)", format);
		}

		if (!(bit_width == DMA_INPUT_BIT_WIDTH_8BIT ||
			bit_width == DMA_INPUT_BIT_WIDTH_10BIT ||
			bit_width == DMA_INPUT_BIT_WIDTH_16BIT)) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Input format(%d)", bit_width);
		}
		break;
	case HW_MCSC_DMA_OUTPUT:
		/* check dma output */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output width(%d)", width);
		}

		if (height < 16 || height > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output height(%d)", height);
		}

		if (!(format == DMA_OUTPUT_FORMAT_YUV422
			|| format == DMA_OUTPUT_FORMAT_YUV420
			|| format == DMA_OUTPUT_FORMAT_Y
			|| format == DMA_OUTPUT_FORMAT_RGB)) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output format(%d)", format);
		}

		if (!(bit_width == DMA_OUTPUT_BIT_WIDTH_8BIT ||
			bit_width == DMA_OUTPUT_BIT_WIDTH_10BIT ||
			bit_width == DMA_OUTPUT_BIT_WIDTH_16BIT ||
			bit_width == DMA_OUTPUT_BIT_WIDTH_32BIT)) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output bit_width(%d)", bit_width);
		}
		break;
	default:
		err_hw("Invalid MCSC type(%d)", type);
		break;
	}

	return ret;
}

int fimc_is_hw_mcsc_update_dsvra_register(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_group *head, struct mcs_param *mcs_param,
	u32 instance, struct camera2_shot *shot)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	u32 img_width, img_height;
	u32 src_width, src_height, src_x_pos, src_y_pos;
	u32 out_width, out_height;
	u32 hratio, vratio;
	ulong temp_width, temp_height;
	enum mcsc_port dsvra_inport;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);
	FIMC_BUG(!mcs_param);

	if (cap->ds_vra != MCSC_CAP_SUPPORT)
		return ret;

	if (!test_bit(MCSC_OUTPUT_DS, &mcsc_out_st))
		return -EINVAL;

	dsvra_inport = shot ? shot->uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] : MCSC_PORT_NONE;

	if (dsvra_inport == MCSC_PORT_NONE)
		return -EINVAL;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	/* DS input image size */
	img_width = mcs_param->output[dsvra_inport].width;
	img_height = mcs_param->output[dsvra_inport].height;

	/* DS input src size (src_size + src_pos <= img_size) */
	src_width = mcs_param->output[MCSC_OUTPUT_DS].crop_width;
	src_height = mcs_param->output[MCSC_OUTPUT_DS].crop_height;
	src_x_pos = mcs_param->output[MCSC_OUTPUT_DS].crop_offset_x;
	src_y_pos = mcs_param->output[MCSC_OUTPUT_DS].crop_offset_y;

	out_width = mcs_param->output[MCSC_OUTPUT_DS].width;
	out_height = mcs_param->output[MCSC_OUTPUT_DS].height;

	if (src_width == 0 || src_height == 0) {
		src_x_pos = 0;
		src_y_pos = 0;
		src_width = img_width;
		src_height = img_height;
	}

	if ((int)src_x_pos < 0 || (int)src_y_pos < 0)
		panic("%s: wrong DS crop position (x:%d, y:%d)", __func__, src_x_pos, src_y_pos);

	if (src_width + src_x_pos > img_width) {
		warn_hw("%s: Out of crop width region of DS (%d < (%d + %d))",
			__func__, img_width, src_width, src_x_pos);
		src_x_pos = 0;
		src_width = img_width;
	}

	if (src_height + src_y_pos > img_height) {
		warn_hw("%s: Out of crop height region of DS (%d < (%d + %d))",
			__func__, img_height, src_height, src_y_pos);
		src_y_pos = 0;
		src_height = img_height;
	}

	temp_width = (ulong)src_width;
	temp_height = (ulong)src_height;
	hratio = (u32)((temp_width << MCSC_PRECISION) / out_width);
	vratio = (u32)((temp_height << MCSC_PRECISION) / out_height);

	fimc_is_scaler_set_ds_img_size(hw_ip->regs, img_width, img_height);
	fimc_is_scaler_set_ds_src_size(hw_ip->regs, src_width, src_height, src_x_pos, src_y_pos);
	fimc_is_scaler_set_ds_dst_size(hw_ip->regs, out_width, out_height);
	fimc_is_scaler_set_ds_scaling_ratio(hw_ip->regs, hratio, vratio);
	fimc_is_scaler_set_ds_init_phase_offset(hw_ip->regs, 0, 0);
	fimc_is_scaler_set_ds_gamma_table_enable(hw_ip->regs, true);
	ret = fimc_is_hw_mcsc_dma_output(hw_ip, &mcs_param->output[MCSC_OUTPUT_DS], MCSC_OUTPUT_DS, instance);

	fimc_is_scaler_set_otf_out_path(hw_ip->regs, dsvra_inport);
	fimc_is_scaler_set_ds_enable(hw_ip->regs, true);
	fimc_is_scaler_set_dma_out_enable(hw_ip->regs, MCSC_OUTPUT_DS, true);

	if (!test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state))
		set_bit(HW_MCS_DS_CFG, &hw_ip->state);

	msdbg_hw(2, "%s: dsvra_inport(%d) configured\n", instance, hw_ip, __func__, dsvra_inport);

	return ret;
}

int fimc_is_hw_mcsc_update_ysum_register(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_group *head, struct mcs_param *mcs_param,
	u32 instance, struct camera2_shot *shot)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;
	struct fimc_is_hw_mcsc_cap *cap;
	u32 width, height;
	u32 start_x = 0, start_y = 0;
	enum mcsc_port ysumport;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);
	FIMC_BUG(!mcs_param);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	cap = GET_MCSC_HW_CAP(hw_ip);

	if (cap->ysum != MCSC_CAP_SUPPORT)
		return ret;

	ysumport = shot ? shot->uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_YSUM] : MCSC_PORT_NONE;

	if (ysumport == MCSC_PORT_NONE)
		return -EINVAL;

	if (!fimc_is_scaler_get_dma_out_enable(hw_ip->regs, ysumport))
		return -EINVAL;

	switch (ysumport) {
	case MCSC_PORT_0:
		width = mcs_param->output[MCSC_OUTPUT0].width;
		height = mcs_param->output[MCSC_OUTPUT0].height;
		break;
	case MCSC_PORT_1:
		width = mcs_param->output[MCSC_OUTPUT1].width;
		height = mcs_param->output[MCSC_OUTPUT1].height;
		break;
	case MCSC_PORT_2:
		width = mcs_param->output[MCSC_OUTPUT2].width;
		height = mcs_param->output[MCSC_OUTPUT2].height;
		break;
	case MCSC_PORT_3:
		width = mcs_param->output[MCSC_OUTPUT3].width;
		height = mcs_param->output[MCSC_OUTPUT3].height;
		break;
	case MCSC_PORT_4:
		width = mcs_param->output[MCSC_OUTPUT4].width;
		height = mcs_param->output[MCSC_OUTPUT4].height;
		break;
	default:
		goto no_setting_ysum;
		break;
	}

	fimc_is_scaler_set_ysum_image_size(hw_ip->regs, width, height, start_x, start_y);
	fimc_is_scaler_set_ysum_input_sourece_enable(hw_ip->regs, ysumport, true);

	if (!test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state))
		set_bit(HW_MCS_YSUM_CFG, &hw_ip->state);

	msdbg_hw(2, "%s: ysum_port(%d) configured\n", instance, hw_ip, __func__, ysumport);

no_setting_ysum:
	return ret;
}

static void fimc_is_hw_mcsc_size_dump(struct fimc_is_hw_ip *hw_ip)
{
	int i;
	u32 input_src = 0;
	u32 in_w, in_h = 0;
	u32 rdma_w, rdma_h = 0;
	u32 poly_src_w, poly_src_h = 0;
	u32 poly_dst_w, poly_dst_h = 0;
	u32 post_in_w, post_in_h = 0;
	u32 post_out_w, post_out_h = 0;
	u32 wdma_enable = 0;
	u32 wdma_w, wdma_h = 0;
	u32 rdma_y_stride, rdma_uv_stride = 0;
	u32 wdma_y_stride, wdma_uv_stride = 0;
	struct fimc_is_hw_mcsc_cap *cap;

	FIMC_BUG_VOID(!hw_ip);

	/* skip size dump, if hw_ip wasn't opened */
	if (!test_bit(HW_OPEN, &hw_ip->state))
		return;

	cap = GET_MCSC_HW_CAP(hw_ip);
	if (!cap) {
		err_hw("failed to get hw_mcsc_cap(%p)", cap);
		return;
	}

	input_src = fimc_is_scaler_get_input_source(hw_ip->regs, hw_ip->id);
	fimc_is_scaler_get_input_img_size(hw_ip->regs, hw_ip->id, &in_w, &in_h);
	fimc_is_scaler_get_rdma_size(hw_ip->regs, &rdma_w, &rdma_h);
	fimc_is_scaler_get_rdma_stride(hw_ip->regs, &rdma_y_stride, &rdma_uv_stride);

	sdbg_hw(2, "=SIZE=====================================\n"
		"[INPUT] SRC:%d(0:OTF, 1:DMA), SIZE:%dx%d\n"
		"[RDMA] SIZE:%dx%d, STRIDE: Y:%d, UV:%d\n",
		hw_ip, input_src, in_w, in_h,
		rdma_w, rdma_h, rdma_y_stride, rdma_uv_stride);

	for (i = MCSC_OUTPUT0; i < cap->max_output; i++) {
		fimc_is_scaler_get_poly_src_size(hw_ip->regs, i, &poly_src_w, &poly_src_h);
		fimc_is_scaler_get_poly_dst_size(hw_ip->regs, i, &poly_dst_w, &poly_dst_h);
		fimc_is_scaler_get_post_img_size(hw_ip->regs, i, &post_in_w, &post_in_h);
		fimc_is_scaler_get_post_dst_size(hw_ip->regs, i, &post_out_w, &post_out_h);
		fimc_is_scaler_get_wdma_size(hw_ip->regs, i, &wdma_w, &wdma_h);
		fimc_is_scaler_get_wdma_stride(hw_ip->regs, i, &wdma_y_stride, &wdma_uv_stride);
		wdma_enable = fimc_is_scaler_get_dma_out_enable(hw_ip->regs, i);

		dbg_hw(2, "[POLY%d] SRC:%dx%d, DST:%dx%d\n"
			"[POST%d] SRC:%dx%d, DST:%dx%d\n"
			"[WDMA%d] ENABLE:%d, SIZE:%dx%d, STRIDE: Y:%d, UV:%d\n",
			i, poly_src_w, poly_src_h, poly_dst_w, poly_dst_h,
			i, post_in_w, post_in_h, post_out_w, post_out_h,
			i, wdma_enable, wdma_w, wdma_h, wdma_y_stride, wdma_uv_stride);
	}
	sdbg_hw(2, "==========================================\n", hw_ip);

	return;
}

void fimc_is_hw_mcsc_set_size_for_uvsp(struct fimc_is_hardware *hardware,
	struct fimc_is_frame *frame, ulong hw_map)
{
	u32 hw_id;
	int hw_slot = -1;
	struct fimc_is_hw_ip *hw_ip;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap;
	struct is_region *region;
	struct taa_param *param;
	struct camera2_shot_ext *shot_ext;
	u32 input_w, input_h, crop_x, crop_y, output_w = 0, output_h = 0;

	FIMC_BUG_VOID(!frame);

	hw_id = DEV_HW_MCSC0;
	if (test_bit_variables(hw_id, &hw_map))
		hw_slot = fimc_is_hw_slot_id(hw_id);

	hw_id = DEV_HW_MCSC1;
	if (!valid_hw_slot_id(hw_slot) && test_bit_variables(hw_id, &hw_map))
		hw_slot = fimc_is_hw_slot_id(hw_id);

	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("[%d]Can't find proper hw_slot for MCSC", frame->instance);
		return;
	}

	hw_ip = &hardware->hw_ip[hw_slot];
	FIMC_BUG_VOID(!hw_ip->priv_info);
	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	region = hw_ip->region[frame->instance];
	FIMC_BUG_VOID(!region);

	shot_ext = frame->shot_ext;
	if (!frame->shot_ext) {
		sdbg_hw(2, "[F:%d] shot_ext(NULL)\n", hw_ip, frame->fcount);
		return;
	}

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	cap = GET_MCSC_HW_CAP(hw_ip);

	if (cap->uvsp != MCSC_CAP_SUPPORT)
		return;

	msdbg_hw(2, "[F:%d]: set_size_for_uvsp: binning_r(%d,%d), crop_taa(%d,%d), bds(%d,%d)\n",
		frame->instance, hw_ip, frame->fcount,
		shot_ext->binning_ratio_x, shot_ext->binning_ratio_y,
		shot_ext->crop_taa_x, shot_ext->crop_taa_y,
		shot_ext->bds_ratio_x, shot_ext->bds_ratio_y);

	param = &region->parameter.taa;

	input_w = param->otf_input.bayer_crop_width;
	input_h = param->otf_input.bayer_crop_height;
	crop_x = param->otf_input.bayer_crop_offset_x;
	crop_y = param->otf_input.bayer_crop_offset_y;
	if (param->vdma1_input.cmd != DMA_INPUT_COMMAND_DISABLE) {
		input_w = param->vdma1_input.bayer_crop_width;
		input_h = param->vdma1_input.bayer_crop_height;
		if (param->otf_output.crop_enable) {
			crop_x = param->otf_output.crop_offset_x;
			crop_y = param->otf_output.crop_offset_y;
		} else {
			crop_x = param->vdma1_input.bayer_crop_offset_x;
			crop_y = param->vdma1_input.bayer_crop_offset_y;
		}
	}
	if (param->vdma2_output.cmd != DMA_OUTPUT_COMMAND_DISABLE) {
		output_w = param->vdma2_output.width;
		output_h = param->vdma2_output.height;
	} else {
		output_w = param->vdma1_input.bayer_crop_width;
		output_h = param->vdma1_input.bayer_crop_height;
	}
	frame->shot_ext->binning_ratio_x = (u16)region->parameter.sensor.config.sensor_binning_ratio_x;
	frame->shot_ext->binning_ratio_y = (u16)region->parameter.sensor.config.sensor_binning_ratio_y;
	frame->shot_ext->crop_taa_x = crop_x;
	frame->shot_ext->crop_taa_y = crop_y;
	if (output_w && output_h) {
		frame->shot_ext->bds_ratio_x = (input_w / output_w);
		frame->shot_ext->bds_ratio_y = (input_h / output_h);
	} else {
		frame->shot_ext->bds_ratio_x = 1;
		frame->shot_ext->bds_ratio_y = 1;
	}

	msdbg_hw(2, "[F:%d]: set_size_for_uvsp: in(%d,%d, %dx%d), out(%dx%d), bin_r(%d,%d), crop(%d,%d), bds(%d,%d)\n",
		frame->instance, hw_ip, frame->fcount,
		crop_x, crop_y, input_w, input_h, output_w, output_h,
		shot_ext->binning_ratio_x, shot_ext->binning_ratio_y,
		shot_ext->crop_taa_x, shot_ext->crop_taa_y,
		shot_ext->bds_ratio_x, shot_ext->bds_ratio_y);
}

void fimc_is_hw_mcsc_set_ni(struct fimc_is_hardware *hardware, struct fimc_is_frame *frame,
	u32 instance)
{
	u32 hw_id, index, core_num;
	int hw_slot;
	struct fimc_is_hw_ip *hw_ip;

	FIMC_BUG_VOID(!frame);

	index = frame->fcount % NI_BACKUP_MAX;
	hw_id = DEV_HW_MCSC0;
	core_num = GET_CORE_NUM(hw_id);
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (valid_hw_slot_id(hw_slot) && frame->shot) {
		hw_ip = &hardware->hw_ip[hw_slot];
		hardware->ni_udm[core_num][index] = frame->shot->udm.ni;
		msdbg_hw(2, "set_ni: [F:%d], %d,%d,%d -> %d,%d,%d\n",
				instance, hw_ip, frame->fcount,
				frame->shot->udm.ni.currentFrameNoiseIndex,
				frame->shot->udm.ni.nextFrameNoiseIndex,
				frame->shot->udm.ni.nextNextFrameNoiseIndex,
				hardware->ni_udm[core_num][index].currentFrameNoiseIndex,
				hardware->ni_udm[core_num][index].nextFrameNoiseIndex,
				hardware->ni_udm[core_num][index].nextNextFrameNoiseIndex);
	}

	hw_id = DEV_HW_MCSC1;
	core_num = GET_CORE_NUM(hw_id);
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (valid_hw_slot_id(hw_slot) && frame->shot) {
		hw_ip = &hardware->hw_ip[hw_slot];
		hardware->ni_udm[core_num][index] = frame->shot->udm.ni;
		msdbg_hw(2, "set_ni: [F:%d], %d,%d,%d -> %d,%d,%d\n",
				instance, hw_ip, frame->fcount,
				frame->shot->udm.ni.currentFrameNoiseIndex,
				frame->shot->udm.ni.nextFrameNoiseIndex,
				frame->shot->udm.ni.nextNextFrameNoiseIndex,
				hardware->ni_udm[core_num][index].currentFrameNoiseIndex,
				hardware->ni_udm[core_num][index].nextFrameNoiseIndex,
				hardware->ni_udm[core_num][index].nextNextFrameNoiseIndex);
	}
}

static int fimc_is_hw_mcsc_get_meta(struct fimc_is_hw_ip *hw_ip,
		struct fimc_is_frame *frame, unsigned long hw_map)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;

	if (unlikely(!frame)) {
		mserr_hw("get_meta: frame is null", atomic_read(&hw_ip->instance), hw_ip);
		return 0;
	}

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	if (unlikely(!hw_mcsc)) {
		mserr_hw("priv_info is NULL", frame->instance, hw_ip);
		return -EINVAL;
	}

	fimc_is_scaler_get_ysum_result(hw_ip->regs,
		&frame->shot->udm.scaler.ysumdata.higher_ysum_value,
		&frame->shot->udm.scaler.ysumdata.lower_ysum_value);

	return ret;
}

int fimc_is_hw_mcsc_restore(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct is_param_region *param;

	BUG_ON(!hw_ip);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	fimc_is_hw_mcsc_reset(hw_ip);

	param = hw_mcsc->back_param;
	param->tpu.config.tdnr_bypass = true;

	/* setting for MCSC */
	fimc_is_hw_mcsc_update_param(hw_ip, &param->mcs, instance);
	info_hw("[RECOVERY]: mcsc update param\n");

	/* setting for TDNR */
	ret = fimc_is_hw_mcsc_recovery_tdnr_register(hw_ip, param, instance);
	info_hw("[RECOVERY]: tdnr update param\n");

	fimc_is_hw_mcsc_clear_interrupt(hw_ip);
	fimc_is_scaler_start(hw_ip->regs, hw_ip->id);

	return ret;
}

const struct fimc_is_hw_ip_ops fimc_is_hw_mcsc_ops = {
	.open			= fimc_is_hw_mcsc_open,
	.init			= fimc_is_hw_mcsc_init,
	.deinit			= fimc_is_hw_mcsc_deinit,
	.close			= fimc_is_hw_mcsc_close,
	.enable			= fimc_is_hw_mcsc_enable,
	.disable		= fimc_is_hw_mcsc_disable,
	.shot			= fimc_is_hw_mcsc_shot,
	.set_param		= fimc_is_hw_mcsc_set_param,
	.get_meta		= fimc_is_hw_mcsc_get_meta,
	.frame_ndone		= fimc_is_hw_mcsc_frame_ndone,
	.load_setfile		= fimc_is_hw_mcsc_load_setfile,
	.apply_setfile		= fimc_is_hw_mcsc_apply_setfile,
	.delete_setfile		= fimc_is_hw_mcsc_delete_setfile,
	.size_dump		= fimc_is_hw_mcsc_size_dump,
	.clk_gate		= fimc_is_hardware_clk_gate,
	.restore		= fimc_is_hw_mcsc_restore
};

int fimc_is_hw_mcsc_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
	struct fimc_is_interface_ischain *itfc, int id, const char *name)
{
	int ret = 0;
	int hw_slot = -1;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!itf);
	FIMC_BUG(!itfc);

	/* initialize device hardware */
	hw_ip->id   = id;
	snprintf(hw_ip->name, sizeof(hw_ip->name), "%s", name);
	hw_ip->ops  = &fimc_is_hw_mcsc_ops;
	hw_ip->itf  = itf;
	hw_ip->itfc = itfc;
	atomic_set(&hw_ip->fcount, 0);
	hw_ip->is_leader = true;
	atomic_set(&hw_ip->status.Vvalid, V_BLANK);
	atomic_set(&hw_ip->rsccount, 0);
	init_waitqueue_head(&hw_ip->status.wait_queue);

	/* set mcsc sfr base address */
	hw_slot = fimc_is_hw_slot_id(id);
	if (!valid_hw_slot_id(hw_slot)) {
		serr_hw("invalid hw_slot (%d)", hw_ip, hw_slot);
		return -EINVAL;
	}

	/* set mcsc interrupt handler */
	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler = &fimc_is_hw_mcsc_handle_interrupt;

	clear_bit(HW_OPEN, &hw_ip->state);
	clear_bit(HW_INIT, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);
	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_TUNESET, &hw_ip->state);
	clear_bit(HW_MCS_YSUM_CFG, &hw_ip->state);
	clear_bit(HW_MCS_DS_CFG, &hw_ip->state);

	spin_lock_init(&mcsc_out_slock);

	sinfo_hw("probe done\n", hw_ip);

	return ret;
}

void fimc_is_hw_mcsc_get_force_block_control(struct fimc_is_hw_ip *hw_ip, u32 ip_offset, u32 num_of_block,
	u32 *input_sel, bool *en)
{
	u32 value = hw_ip->subblk_ctrl;

	if (!GET_SUBBLK_CTRL_BIT(value, ip_offset, SUBBLK_CTRL_SYSFS))
		return;

	switch (num_of_block) {
	case 1:
		*input_sel = GET_SUBBLK_CTRL_BIT(value, ip_offset, SUBBLK_CTRL_INPUT) + DEV_HW_MCSC0;
		*en = GET_SUBBLK_CTRL_BIT(value, ip_offset, SUBBLK_CTRL_EN);
		sinfo_hw("force control single sub block: value(0x%08X), ip(%u) input_sel(%u), en(%u)\n",
			hw_ip, value, ip_offset, *input_sel, *en);
		break;
	case 2:
		*en = GET_SUBBLK_CTRL_BIT(value, ip_offset,
			SUBBLK_CTRL_EN + GET_CORE_NUM(hw_ip->id));
		sinfo_hw("force control dual sub block: value(0x%08X), ip(%u), en(%u)\n",
			hw_ip, value, ip_offset, *en);
		break;
	default:
		serr_hw("num_of_block(%u) of ip(%u) is wrong\n", hw_ip, num_of_block, ip_offset);
		break;
	}
}
