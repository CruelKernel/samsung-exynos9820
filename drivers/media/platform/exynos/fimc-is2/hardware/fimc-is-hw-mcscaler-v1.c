/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-mcscaler-v1.h"
#include "api/fimc-is-hw-api-mcscaler-v1.h"
#include "../interface/fimc-is-interface-ischain.h"
#include "fimc-is-param.h"
#include "fimc-is-err.h"

static void fimc_is_hw_mcsc_size_dump(struct fimc_is_hw_ip *hw_ip);

static int fimc_is_hw_mcsc_handle_interrupt(u32 id, void *context)
{
	struct fimc_is_frame *frame;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_hw_ip *hw_ip = NULL;
	u32 status, intr_mask, intr_status;
	bool err_intr_flag = false;
	int ret = 0;
	u32 hl = 0, vl = 0;
	u32 instance;
	u32 hw_fcount, index;

	hw_ip = (struct fimc_is_hw_ip *)context;
	hw_fcount = atomic_read(&hw_ip->fcount);
	instance = atomic_read(&hw_ip->instance);

	fimc_is_scaler_get_input_status(hw_ip->regs, &hl, &vl);
	/* read interrupt status register (sc_intr_status) */
	intr_mask = fimc_is_scaler_get_intr_mask(hw_ip->regs);
	intr_status = fimc_is_scaler_get_intr_status(hw_ip->regs);
	status = (~intr_mask) & intr_status;

	if (status & (1 << INTR_MC_SCALER_OVERFLOW)) {
		mserr_hw("Overflow!! (0x%x)", instance, hw_ip, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_OUTSTALL)) {
		mserr_hw("Output Block BLOCKING!! (0x%x)", instance, hw_ip, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF)) {
		mserr_hw("Input OTF Vertical Underflow!! (0x%x)", instance, hw_ip, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF)) {
		mserr_hw("Input OTF Vertical Overflow!! (0x%x)", instance, hw_ip, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF)) {
		mserr_hw("Input OTF Horizontal Underflow!! (0x%x)", instance, hw_ip, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF)) {
		mserr_hw("Input OTF Horizontal Overflow!! (0x%x)", instance, hw_ip, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_PROTOCOL_ERR)) {
		mserr_hw("Input Protocol Error!! (0x%x)", instance, hw_ip, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_WDMA_FINISH)) {
		mserr_hw("WDMA_FINISH: Disabled interrupt occurred!! (0x%x)", instance, hw_ip, status);
	}

	if (status & (1 << INTR_MC_SCALER_FRAME_START)) {
		atomic_inc(&hw_ip->count.fs);
		hw_ip->debug_index[1] = hw_ip->debug_index[0] % DEBUG_FRAME_COUNT;
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].fcount = hw_ip->debug_index[0];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_START] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_START] = local_clock();
		if (!atomic_read(&hw_ip->hardware->stream_on))
			msinfo_hw("[F:%d]F.S\n", instance, hw_ip, hw_fcount);

		atomic_set(&hw_ip->status.Vvalid, V_VALID);
		clear_bit(HW_CONFIG, &hw_ip->state);
	}

	if (status & (1 << INTR_MC_SCALER_FRAME_END)) {
		framemgr = hw_ip->framemgr;

		framemgr_e_barrier(framemgr, 0);
		frame = peek_frame(framemgr, FS_COMPLETE);
		framemgr_x_barrier(framemgr, 0);

		if (frame == NULL) {
			mserr_hw("[F:%d] frame(null)!!", instance, hw_ip, atomic_read(&hw_ip->fcount));
			FIMC_BUG(1);
		}

		index = hw_ip->debug_index[1];
		if (test_bit(ENTRY_SCP, &frame->out_flag)) {
			hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_DMA_END] = raw_smp_processor_id();
				hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_DMA_END] = local_clock();
			if (!atomic_read(&hw_ip->hardware->stream_on))
				msinfo_hw("[F:%d]F.E DMA\n", instance, hw_ip, hw_fcount);

			atomic_inc(&hw_ip->count.dma);
			fimc_is_hardware_frame_done(hw_ip, NULL, WORK_SCP_FDONE, ENTRY_SCP,
				FRAME_DONE_NORMAL, false);
		} else {
			fimc_is_hardware_frame_done(hw_ip, NULL, -1, FIMC_IS_HW_CORE_END,
				FRAME_DONE_NORMAL, false);
		}
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_END] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_END] = local_clock();
		if (!atomic_read(&hw_ip->hardware->stream_on))
			msinfo_hw("[F:%d]F.E\n", instance, hw_ip, hw_fcount);

		atomic_inc(&hw_ip->count.fe);
		if (atomic_read(&hw_ip->count.fs) < atomic_read(&hw_ip->count.fe)) {
			mserr_hw("fs(%d), fe(%d), dma(%d)\n", instance, hw_ip,
				atomic_read(&hw_ip->count.fs),
				atomic_read(&hw_ip->count.fe),
				atomic_read(&hw_ip->count.dma));
		}
		atomic_set(&hw_ip->status.Vvalid, V_BLANK);
		wake_up(&hw_ip->status.wait_queue);
	}

	if (err_intr_flag) {
		msinfo_hw("[F:%d] Ocurred error interrupt (%d,%d) status(0x%x)\n",
			instance, hw_ip, hw_fcount, hl, vl, status);
		fimc_is_hardware_size_dump(hw_ip);
	}

	fimc_is_scaler_clear_intr_src(hw_ip->regs, status);

	if (status & (1 << INTR_MC_SCALER_FRAME_END))
		CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);

	return ret;
}

static int fimc_is_hw_mcsc_open(struct fimc_is_hw_ip *hw_ip, u32 instance,
	struct fimc_is_group *group)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	ret = fimc_is_hw_mcsc_reset(hw_ip);
	if (ret != 0) {
		mserr_hw("sw reset fail", instance, hw_ip);
		ret = -ENODEV;
		goto err_reset;
	}

	hw_ip->priv_info = vzalloc(sizeof(struct fimc_is_hw_mcsc));
	if(!hw_ip->priv_info) {
		mserr_hw("hw_ip->priv_info(null)", instance, hw_ip);
		ret = -ENOMEM;
		goto err_alloc;
	}

	atomic_set(&hw_ip->status.Vvalid, V_BLANK);
	set_bit(HW_OPEN, &hw_ip->state);
	return 0;

err_alloc:
err_reset:
	return ret;
}

static int fimc_is_hw_mcsc_init(struct fimc_is_hw_ip *hw_ip, u32 instance,
	struct fimc_is_group *group, bool flag, u32 module_id)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	set_bit(HW_INIT, &hw_ip->state);
	return ret;
}

static int fimc_is_hw_mcsc_close(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;

	clear_bit(HW_OPEN, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_mcsc_enable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	set_bit(HW_RUN, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_mcsc_disable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	long timetowait;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
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

		/* disable MCSC */
		fimc_is_scaler_clear_dma_out_addr(hw_ip->regs);
		fimc_is_scaler_stop(hw_ip->regs);

		ret = fimc_is_hw_mcsc_reset(hw_ip);
		if (ret != 0) {
			mserr_hw("sw reset fail", instance, hw_ip);
			return -ENODEV;
		}

		clear_bit(HW_RUN, &hw_ip->state);
		clear_bit(HW_CONFIG, &hw_ip->state);
	} else {
		msdbg_hw(2, "already disabled\n", instance, hw_ip);
	}

	return ret;
}

static int fimc_is_hw_mcsc_shot(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	struct fimc_is_group *head;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct scp_param *param;
	u32 target_addr[4] = {0};
	bool start_flag = true;
	u32 lindex, hindex;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	msdbgs_hw(2, "[F:%d]shot\n", frame->instance, hw_ip, frame->fcount);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized\n", frame->instance, hw_ip);
		return -EINVAL;
	}

	if (!test_bit(ENTRY_SCP, &frame->out_flag))
		set_bit(hw_ip->id, &frame->core_flag);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	param = &hw_ip->region[frame->instance]->parameter.scalerp;

	head = hw_ip->group[frame->instance]->head;

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state)) {
		if (!test_bit(HW_CONFIG, &hw_ip->state) && !atomic_read(&hw_ip->hardware->stream_on))
			start_flag = true;
		else
			start_flag = false;
	} else {
		start_flag = true;
	}

	if (frame->type == SHOT_TYPE_INTERNAL) {
		msdbg_hw(2, "request not exist\n", frame->instance, hw_ip);
		goto config;
	}

	lindex = frame->shot->ctl.vendor_entry.lowIndexParam;
	hindex = frame->shot->ctl.vendor_entry.highIndexParam;

	fimc_is_hw_mcsc_update_param(hw_ip, param,
		lindex, hindex, frame->instance);

	/* set mcsc dma out addr */
	target_addr[0] = frame->scpTargetAddress[0];
	target_addr[1] = frame->scpTargetAddress[1];
	target_addr[2] = frame->scpTargetAddress[2];

	msdbg_hw(2, "[F:%d]shot [T:%d] cmd[%d] addr[0x%x]\n",
		frame->instance, hw_ip, frame->fcount, frame->type,
		param->dma_output.cmd, target_addr[0]);
	fimc_is_hw_mcsc_size_dump(hw_ip);

config:
	if (param->dma_output.cmd != DMA_OUTPUT_COMMAND_DISABLE
		&& target_addr[0] != 0
		&& frame->type != SHOT_TYPE_INTERNAL) {
		fimc_is_scaler_set_dma_enable(hw_ip->regs, true);
		/* use only one buffer (per-frame) */
		fimc_is_scaler_set_dma_out_frame_seq(hw_ip->regs,
			0x1 << USE_DMA_BUFFER_INDEX);
		fimc_is_scaler_set_dma_out_addr(hw_ip->regs,
			target_addr[0], target_addr[1], target_addr[2],
			USE_DMA_BUFFER_INDEX);
	} else {
		fimc_is_scaler_set_dma_enable(hw_ip->regs, false);
		fimc_is_scaler_clear_dma_out_addr(hw_ip->regs);
		msdbg_hw(2, "Disable dma out\n", frame->instance, hw_ip);
	}

	if (start_flag) {
		msinfo_hw("mcsc_start[F:%d]\n", frame->instance, hw_ip, frame->fcount);
		fimc_is_scaler_start(hw_ip->regs);
		if (ret) {
			mserr_hw("scaler_start failed!!\n",
				frame->instance, hw_ip);
			return -EINVAL;
		}
	}
	set_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_mcsc_set_param(struct fimc_is_hw_ip *hw_ip, struct is_region *region,
	u32 lindex, u32 hindex, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct scp_param *param;

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

	param = &region->parameter.scalerp;
	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	fimc_is_hw_mcsc_update_param(hw_ip, param,
		lindex, hindex, instance);

	return ret;
}

void fimc_is_hw_mcsc_check_size(struct fimc_is_hw_ip *hw_ip, struct scp_param *param, u32 instance)
{
	struct param_otf_input *otf_input = &param->otf_input;
	struct param_scaler_input_crop *in_crop = &param->input_crop;
	struct param_scaler_output_crop *out_crop = &param->output_crop;
	struct param_otf_output *otf_output = &param->otf_output;
	struct param_dma_output *dma_output = &param->dma_output;

	minfo_hw(">>> hw_mcsc_check_size >>>\n", instance);
	info_hw("otf_input: cmd(%d),format(%d),size(%dx%d)\n",
		otf_input->cmd, otf_input->format, otf_input->width, otf_input->height);
	info_hw("input_crop: cmd(%d),pos(%d,%d),crop(%dx%d),in_size(%dx%d),out_size(%dx%d)\n",
		in_crop->cmd, in_crop->pos_x, in_crop->pos_y,
		in_crop->crop_width, in_crop->crop_height,
		in_crop->in_width, in_crop->in_height,
		in_crop->out_width, in_crop->out_height);
	info_hw("output_crop: cmd(%d),pos(%d,%d),crop(%dx%d),format(%d)\n",
		out_crop->cmd, out_crop->pos_x, out_crop->pos_y,
		out_crop->crop_width, out_crop->crop_height, out_crop->format);
	info_hw("otf_output: cmd(%d),format(%d),size(%dx%d)\n",
		otf_output->cmd, otf_output->format, otf_output->width, otf_output->height);
	info_hw("dma_output: cmd(%d),format(%d),size(%dx%d)\n",
		dma_output->cmd, dma_output->format, dma_output->width, dma_output->height);
	minfo_hw("<<< hw_mcsc_check_size <<<\n", instance);
}

int fimc_is_hw_mcsc_update_param(struct fimc_is_hw_ip *hw_ip,
	struct scp_param *param, u32 lindex, u32 hindex, u32 instance)
{
	int ret = 0;
	bool control_cmd = false;
	struct fimc_is_hw_mcsc *hw_mcsc;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (hw_mcsc->instance != instance) {
		control_cmd = true;
		msinfo_hw("update_param: hw_ip->instance(%d), control_cmd(%d)\n",
			instance, hw_ip, hw_mcsc->instance, control_cmd);
		hw_mcsc->instance = instance;
	}

	if (control_cmd || (lindex & LOWBIT_OF(PARAM_SCALERP_OTF_INPUT))
		|| (hindex & HIGHBIT_OF(PARAM_SCALERP_OTF_INPUT)))
		ret = fimc_is_hw_mcsc_otf_input(hw_ip, &param->otf_input, instance);

	if (control_cmd || (lindex & LOWBIT_OF(PARAM_SCALERP_IMAGE_EFFECT))
		|| (hindex & HIGHBIT_OF(PARAM_SCALERP_IMAGE_EFFECT)))
		ret = fimc_is_hw_mcsc_output_yuvrange(hw_ip, param, &param->effect, instance);

	if (control_cmd || (lindex & LOWBIT_OF(PARAM_SCALERP_FLIP))
		|| (hindex & HIGHBIT_OF(PARAM_SCALERP_FLIP)))
		ret = fimc_is_hw_mcsc_flip(hw_ip, param, &param->flip, instance);

	if (control_cmd || (lindex & LOWBIT_OF(PARAM_SCALERP_INPUT_CROP))
		|| (hindex & HIGHBIT_OF(PARAM_SCALERP_INPUT_CROP))
		|| (lindex & LOWBIT_OF(PARAM_SCALERP_OUTPUT_CROP))
		|| (hindex & HIGHBIT_OF(PARAM_SCALERP_OUTPUT_CROP))
		|| (lindex & LOWBIT_OF(PARAM_SCALERP_OTF_OUTPUT))
		|| (hindex & HIGHBIT_OF(PARAM_SCALERP_OTF_OUTPUT))
		|| (lindex & LOWBIT_OF(PARAM_SCALERP_DMA_OUTPUT))
		|| (hindex & HIGHBIT_OF(PARAM_SCALERP_DMA_OUTPUT))) {
		ret = fimc_is_hw_mcsc_poly_phase(hw_ip, param, &param->input_crop, instance);
		ret = fimc_is_hw_mcsc_post_chain(hw_ip, param, instance);
		ret = fimc_is_hw_mcsc_otf_output(hw_ip, param, &param->otf_output, instance);
		ret = fimc_is_hw_mcsc_dma_output(hw_ip, param, &param->dma_output, instance);
		hw_mcsc_check_size(hw_ip, param, instance);
	}

	if (ret)
		fimc_is_hw_mcsc_size_dump(hw_ip);

	return ret;
}

int fimc_is_hw_mcsc_reset(struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	ret = fimc_is_scaler_sw_reset(hw_ip->regs);
	if (ret != 0) {
		serr_hw("sw reset fail", hw_ip);
		return -ENODEV;
	}

	fimc_is_scaler_clear_intr_all(hw_ip->regs);
	fimc_is_scaler_disable_intr(hw_ip->regs);
	fimc_is_scaler_mask_intr(hw_ip->regs, MCSC_INTR_MASK);

	/* set stop req post en to bypass(guided to 2) */
	fimc_is_scaler_set_stop_req_post_en_ctrl(hw_ip->regs, 0);

	return ret;
}

static int fimc_is_hw_mcsc_load_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;
	u32 index;

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
	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
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

	for (index = 0; index < setfile->using_count; index++) {
		hw_mcsc->setfile = (struct hw_api_scaler_setfile *)setfile->table[index].addr;
		if (hw_mcsc->setfile->setfile_version != MCSC_SETFILE_VERSION) {
			mserr_hw("setfile version(0x%x) is incorrect",
				instance, hw_ip, hw_mcsc->setfile->setfile_version);
			return -EINVAL;
		}
	}

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
	u32 setfile_index = 0;

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
		mserr_hw("priv info is NULL", instance, hw_ip);
		return -EINVAL;
	}

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	if (!hw_mcsc->setfile)
		return 0;

	setfile_index = setfile->index[scenario];
	if (setfile_index >= setfile->using_count) {
		mserr_hw("setfile index is out-of-range, [%d:%d]",
				instance, hw_ip, scenario, setfile_index);
		return -EINVAL;
	}

	msinfo_hw("setfile (%d) scenario (%d)\n", instance, hw_ip,
		setfile_index, scenario);

	return ret;
}

static int fimc_is_hw_mcsc_delete_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance,
	ulong hw_map)
{
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		msdbg_hw(2, "%s: hw_map(0x%lx)\n", instance, hw_ip, __func__, hw_map);
		return 0;
	}

	if (test_bit(HW_TUNESET, &hw_ip->state)) {
		msdbg_hw(2, "setfile already deleted", instance, hw_ip);
		return 0;
	}

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	hw_mcsc->setfile = NULL;
	clear_bit(HW_TUNESET, &hw_ip->state);

	return 0;
}

static int fimc_is_hw_mcsc_frame_ndone(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	u32 instance, enum ShotErrorType done_type)
{
	int ret = 0;
	int wq_id, output_id;

	wq_id     = WORK_SCP_FDONE;
	output_id = ENTRY_SCP;
	if (test_bit(output_id, &frame->out_flag))
		ret = fimc_is_hardware_frame_done(hw_ip, frame, wq_id, output_id,
				done_type, false);

	wq_id     = -1;
	output_id = FIMC_IS_HW_CORE_END;
	if (test_bit(hw_ip->id, &frame->core_flag))
		ret = fimc_is_hardware_frame_done(hw_ip, frame, wq_id, output_id,
				done_type, false);

	return ret;
}

int fimc_is_hw_mcsc_otf_input(struct fimc_is_hw_ip *hw_ip,
	struct param_otf_input *input, u32 instance)
{
	int ret = 0;
	u32 width, height;
	u32 format, bit_width;
	struct fimc_is_hw_mcsc *hw_mcsc;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!input);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	msdbg_hw(2, "input_setting cmd(%d)\n", instance, hw_ip, input->cmd);
	width  = input->width;
	height = input->height;
	format = input->format;
	bit_width = input->bitwidth;

	if (input->cmd == OTF_INPUT_COMMAND_DISABLE)
		return ret;

	ret = fimc_is_hw_mcsc_check_format(HW_MCSC_OTF_INPUT,
		format, bit_width, width, height);
	if (ret) {
		mserr_hw("Invalid OTF Input format: fmt(%d),bit(%d),size(%dx%d)",
			instance, hw_ip, format, bit_width, width, height);
		return ret;
	}

	fimc_is_scaler_set_poly_img_size(hw_ip->regs, width, height);
	fimc_is_scaler_set_dither(hw_ip->regs, input->cmd);

	return ret;
}

int fimc_is_hw_mcsc_poly_phase(struct fimc_is_hw_ip *hw_ip, struct scp_param *param,
	struct param_scaler_input_crop *in_crop, u32 instance) /* poly_phase */
{
	int ret = 0;
	u32 src_pos_x, src_pos_y, src_width, src_height;
	u32 poly_dst_width, poly_dst_height;
	u32 out_width, out_height;
	ulong temp_width, temp_height;
	u32 hratio, vratio;
	bool post_en = false;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct param_otf_output *otf_output;
	struct param_dma_output *dma_output;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!in_crop);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	otf_output = &param->otf_output;
	dma_output = &param->dma_output;

	msdbg_hw(2, "poly_phase_setting cmd(O:%d,D:%d)\n",
		instance, hw_ip, otf_output->cmd, dma_output->cmd);

	if ((otf_output->cmd == OTF_OUTPUT_COMMAND_DISABLE)
		&& (dma_output->cmd == DMA_OUTPUT_COMMAND_DISABLE)) {
		fimc_is_scaler_set_poly_scaler_enable(hw_ip->regs, 0);
		return ret;
	}

	fimc_is_scaler_set_poly_scaler_enable(hw_ip->regs, 1);

	src_pos_x  = in_crop->pos_x;
	src_pos_y  = in_crop->pos_y;
	src_width  = in_crop->crop_width;
	src_height = in_crop->crop_height;

	out_width  = in_crop->out_width;
	out_height = in_crop->out_height;

	hw_mcsc_check_size(hw_ip, param, instance);
	fimc_is_scaler_set_poly_src_size(hw_ip->regs, src_pos_x, src_pos_y,
		src_width, src_height);

	if ((src_width <= (out_width * MCSC_POLY_RATIO_DOWN))
		&& (out_width <= (src_width * MCSC_POLY_RATIO_UP))) {
		poly_dst_width = out_width;
		post_en = false;
	} else if ((src_width <= (out_width * MCSC_POLY_RATIO_DOWN * MCSC_POST_RATIO_DOWN))
		&& ((out_width * MCSC_POLY_RATIO_DOWN) < src_width)) {
		poly_dst_width = MCSC_ROUND_UP(src_width / MCSC_POLY_RATIO_DOWN, 2);
		post_en = true;
	} else {
		mserr_hw("poly_phase: Unsupported H ratio, (%dx%d)->(%dx%d)\n",
			instance, hw_ip, src_width, src_height, out_width, out_height);
		poly_dst_width = MCSC_ROUND_UP(src_width / MCSC_POLY_RATIO_DOWN, 2);
		post_en = true;
	}

	if ((src_height <= (out_height * MCSC_POLY_RATIO_DOWN))
		&& (out_height <= (src_height * MCSC_POLY_RATIO_UP))) {
		poly_dst_height = out_height;
		post_en = false;
	} else if ((src_height <= (out_height * MCSC_POLY_RATIO_DOWN * MCSC_POST_RATIO_DOWN))
		&& ((out_height * MCSC_POLY_RATIO_DOWN) < src_height)) {
		poly_dst_height = (src_height / MCSC_POLY_RATIO_DOWN);
		post_en = true;
	} else {
		mserr_hw("poly_phase: Unsupported H ratio, (%dx%d)->(%dx%d)\n",
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
		msdbg_hw(2, "poly_phase: shift(%d), ratio(%d,%d), "
			"size(%dx%d) before calculation\n",
			instance, hw_ip, shift, multiple_hratio, multiple_hratio,
			poly_dst_width, poly_dst_height);
		poly_dst_width  = MCSC_ROUND_UP(poly_dst_width, multiple_hratio);
		poly_dst_height = MCSC_ROUND_UP(poly_dst_height, multiple_vratio);
		if (poly_dst_width % 2) {
			poly_dst_width  = poly_dst_width  + multiple_hratio;
			poly_dst_height = poly_dst_height + multiple_vratio;
		}
		msdbg_hw(2, "poly_phase: size(%dx%d) after  calculation\n",
			instance, hw_ip, poly_dst_width, poly_dst_height);
	}
#endif

	fimc_is_scaler_set_poly_dst_size(hw_ip->regs,
		poly_dst_width, poly_dst_height);

	temp_width  = (ulong)src_width;
	temp_height = (ulong)src_height;
	hratio = (u32)((temp_width  << MCSC_PRECISION) / poly_dst_width);
	vratio = (u32)((temp_height << MCSC_PRECISION) / poly_dst_height);

	fimc_is_scaler_set_poly_scaling_ratio(hw_ip->regs, hratio, vratio);
	fimc_is_scaler_set_poly_scaler_coef(hw_ip->regs, hratio, vratio);

	return ret;
}

int fimc_is_hw_mcsc_post_chain(struct fimc_is_hw_ip *hw_ip, struct scp_param *param,
	u32 instance)
{
	int ret = 0;
	u32 img_width, img_height;
	u32 dst_width, dst_height;
	ulong temp_width, temp_height;
	u32 hratio, vratio;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct param_otf_output *otf_output;
	struct param_dma_output *dma_output;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	otf_output = &param->otf_output;
	dma_output = &param->dma_output;

	msdbg_hw(2, "post_chain_setting cmd(O:%d,D:%d)\n",
		instance, hw_ip, otf_output->cmd, dma_output->cmd);

	if ((otf_output->cmd == OTF_OUTPUT_COMMAND_DISABLE)
		&& (dma_output->cmd == DMA_OUTPUT_COMMAND_DISABLE)) {
		fimc_is_scaler_set_post_scaler_enable(hw_ip->regs, 0);
		return ret;
	}
	fimc_is_scaler_get_poly_dst_size(hw_ip->regs, &img_width, &img_height);

	if ((otf_output->width != dma_output->width)
		|| (otf_output->height!= dma_output->height)) {
		mserr_hw("output_size invalid otf(%dx%d), dma(%dx%d)",
			instance, hw_ip, otf_output->width, otf_output->height,
			dma_output->width, dma_output->height);
		return -EINVAL;
	}

	dst_width  = otf_output->width;
	dst_height = otf_output->height;

	/* if x1 ~ x1/4 scaling, post scaler bypassed */
	if ((img_width == dst_width) && (img_height == dst_height))
		fimc_is_scaler_set_post_scaler_enable(hw_ip->regs, 0);
	else
		fimc_is_scaler_set_post_scaler_enable(hw_ip->regs, 1);

	fimc_is_scaler_set_post_img_size(hw_ip->regs, img_width, img_height);
	fimc_is_scaler_set_post_dst_size(hw_ip->regs, dst_width, dst_height);

	temp_width  = (ulong)img_width;
	temp_height = (ulong)img_height;
	hratio = (u32)((temp_width  << MCSC_PRECISION) / dst_width);
	vratio = (u32)((temp_height << MCSC_PRECISION) / dst_height);

	fimc_is_scaler_set_post_scaling_ratio(hw_ip->regs, hratio, vratio);

	return ret;
}

int fimc_is_hw_mcsc_flip(struct fimc_is_hw_ip *hw_ip, struct scp_param *param,
	struct param_scaler_flip *flip, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct param_dma_output *dma_output;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param);
	FIMC_BUG(!flip);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	dma_output = &param->dma_output;

	msdbg_hw(2, "flip_setting flip(%d),cmd(D:%d)\n",
		instance, hw_ip, flip->cmd, dma_output->cmd);

	if (dma_output->cmd == DMA_OUTPUT_COMMAND_DISABLE)
		return ret;

	fimc_is_scaler_set_flip_mode(hw_ip->regs, flip->cmd);

	return ret;
}

int fimc_is_hw_mcsc_otf_output(struct fimc_is_hw_ip *hw_ip, struct scp_param *param,
	struct param_otf_output *otf_output, u32 instance)
{
	int ret = 0;
	u32 out_width, out_height;
	u32 format, bitwidth;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct param_dma_output *dma_output;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param);
	FIMC_BUG(!otf_output);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	dma_output = &param->dma_output;

	msdbg_hw(2, "otf_output_setting cmd(O:%d,D:%d)\n",
		instance, hw_ip, otf_output->cmd, dma_output->cmd);

	if (otf_output->cmd == OTF_OUTPUT_COMMAND_DISABLE) {
		fimc_is_scaler_set_direct_out_path(hw_ip->regs, 0, 0);
		return ret;
	}

	out_width  = otf_output->width;
	out_height = otf_output->height;
	format     = otf_output->format;
	bitwidth   = otf_output->bitwidth;

	ret = fimc_is_hw_mcsc_check_format(HW_MCSC_OTF_OUTPUT,
		format, bitwidth, out_width, out_height);
	if (ret) {
		mserr_hw("Invalid OTF Output format: fmt(%d),bit(%d),size(%dx%d)",
			instance, hw_ip, format, bitwidth, out_width, out_height);
		return ret;
	}

	fimc_is_scaler_set_direct_out_path(hw_ip->regs, 0, 1);

	return ret;
}

int fimc_is_hw_mcsc_dma_output(struct fimc_is_hw_ip *hw_ip, struct scp_param *param,
	struct param_dma_output *dma_output, u32 instance)
{
	int ret = 0;
	u32 out_width, out_height, scaled_width, scaled_height;
	u32 format, plane, order,bitwidth;
	u32 y_stride, uv_stride;
	u32 img_format;
	bool conv420_en = false;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct param_otf_output *otf_output;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param);
	FIMC_BUG(!dma_output);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	otf_output = &param->otf_output;

	msdbg_hw(2, "dma_output_setting cmd(O:%d,D:%d)\n",
		instance, hw_ip, otf_output->cmd, dma_output->cmd);

	if (dma_output->cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		fimc_is_scaler_set_dma_out_path(hw_ip->regs, 0, 0);
		return ret;
	}

	out_width  = dma_output->width;
	out_height = dma_output->height;
	format     = dma_output->format;
	plane      = dma_output->plane;
	order      = dma_output->order;
	bitwidth   = dma_output->bitwidth;

	ret = fimc_is_hw_mcsc_check_format(HW_MCSC_DMA_OUTPUT,
		format, bitwidth, out_width, out_height);
	if (ret) {
		mserr_hw("Invalid DMA Output format: fmt(%d),bit(%d),size(%dx%d)",
			instance, hw_ip, format, bitwidth, out_width, out_height);
		return ret;
	}

	ret = fimc_is_hw_mcsc_adjust_img_fmt(format, plane, order,
			&img_format, &conv420_en);
	if (ret < 0) {
		mswarn_hw("Invalid dma image format", instance, hw_ip);
		img_format = hw_mcsc->img_format;
		conv420_en = hw_mcsc->conv420_en;
	} else {
		hw_mcsc->img_format = img_format;
		hw_mcsc->conv420_en = conv420_en;
	}

	fimc_is_scaler_set_dma_out_path(hw_ip->regs, img_format, 1);
	fimc_is_scaler_set_420_conversion(hw_ip->regs, 0, conv420_en);

	fimc_is_scaler_get_post_dst_size(hw_ip->regs, &scaled_width, &scaled_height);
	if ((scaled_width != out_width) || (scaled_height != out_height)) {
		msdbg_hw(2, "Invalid scaled size (%d/%d)(%d/%d)\n",
			instance, hw_ip, scaled_width, scaled_height,
			out_width, out_height);
		return -EINVAL;
	}

	fimc_is_hw_mcsc_adjust_stride(out_width, plane, conv420_en, &y_stride, &uv_stride);
	fimc_is_scaler_set_stride_size(hw_ip->regs, y_stride, uv_stride);
	fimc_is_scaler_set_dma_size(hw_ip->regs, out_width, out_height);

	return ret;
}

int fimc_is_hw_mcsc_output_yuvrange(struct fimc_is_hw_ip *hw_ip, struct scp_param *param,
	struct param_scaler_imageeffect *image_effect, u32 instance)
{
	int ret = 0;
	int yuv_range = 0;
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;
	struct param_otf_output *otf_output;
	struct param_dma_output *dma_output;
	scaler_setfile_contents contents;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param);
	FIMC_BUG(!image_effect);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	otf_output = &param->otf_output;
	dma_output = &param->dma_output;

	if (dma_output->cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		fimc_is_scaler_set_bchs_enable(hw_ip->regs, 0);
		return ret;
	}

	yuv_range = image_effect->yuv_range;

	fimc_is_scaler_set_bchs_enable(hw_ip->regs, 1);

	if (test_bit(HW_TUNESET, &hw_ip->state)) {
		/* set yuv range config value by scaler_param yuv_range mode */
		contents = hw_mcsc->setfile->contents[yuv_range];
		fimc_is_scaler_set_b_c(hw_ip->regs,
			contents.y_offset, contents.y_gain);
		fimc_is_scaler_set_h_s(hw_ip->regs,
			contents.c_gain00, contents.c_gain01,
			contents.c_gain10, contents.c_gain11);
		msdbg_hw(2, "set YUV range(%d) by setfile parameter\n",
			instance, hw_ip, yuv_range);
	} else {
		if (yuv_range == SCALER_OUTPUT_YUV_RANGE_FULL) {
			/* Y range - [0:255], U/V range - [0:255] */
			fimc_is_scaler_set_b_c(hw_ip->regs, 0, 256);
			fimc_is_scaler_set_h_s(hw_ip->regs, 256, 0, 0, 256);
		} else if (yuv_range == SCALER_OUTPUT_YUV_RANGE_NARROW) {
			/* Y range - [16:235], U/V range - [16:239] */
			fimc_is_scaler_set_b_c(hw_ip->regs, 16, 220);
			fimc_is_scaler_set_h_s(hw_ip->regs, 224, 0, 0, 224);
		}
		msdbg_hw(2, "YUV range set default settings\n", instance, hw_ip);
	}
	msinfo_hw("output_yuv_setting: yuv_range(%d), cmd(O:%d,D:%d)\n",
		instance, hw_ip, yuv_range, otf_output->cmd, dma_output->cmd);
	info_hw("[Y:offset(%d),gain(%d)][C:gain00(%d),01(%d),10(%d),11(%d)]\n",
		contents.y_offset, contents.y_gain,
		contents.c_gain00, contents.c_gain01,
		contents.c_gain10, contents.c_gain11);

	return ret;
}

int fimc_is_hw_mcsc_adjust_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format,
	bool *conv420_flag)
{
	int ret = 0;

	switch (format) {
	case DMA_OUTPUT_FORMAT_YUV420:
		*conv420_flag = true;
		switch (plane) {
		case 2:
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
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 2:
			switch (order) {
			case DMA_OUTPUT_ORDER_CbCr:
				*img_format = MCSC_YUV422_2P_UFIRST;
				break;
			case DMA_OUTPUT_ORDER_CrCb:
				*img_format = MCSC_YUV422_2P_VFIRST;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = MCSC_YUV422_3P;
			break;
		default:
			err_hw("output plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	default:
		err_hw("output format error - (%d/%d/%d)", format, order, plane);
		ret = -EINVAL;
		break;
	}

	return ret;
}

void fimc_is_hw_mcsc_adjust_stride(u32 width, u32 plane, bool conv420_flag,
	u32 *y_stride, u32 *uv_stride)
{
	if ((conv420_flag == false) && (plane == 1)) {
		*y_stride = width * 2;
		*uv_stride = width;
	} else {
		*y_stride = width;
		if (plane == 3)
			*uv_stride = width / 2;
		else
			*uv_stride = width;
	}

	*y_stride = 2 * ((*y_stride / 2) + ((*y_stride % 2) > 0));
	*uv_stride = 2 * ((*uv_stride / 2) + ((*uv_stride % 2) > 0));
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

		if (format != OTF_INPUT_FORMAT_YUV422) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input format(%d)", format);
		}

		if (bit_width != OTF_INPUT_BIT_WIDTH_12BIT) {
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
		ret = -EINVAL;
		err_hw("MCSC DMA Input is not supported");
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

		if (!(format == DMA_OUTPUT_FORMAT_YUV422 ||
			format == DMA_OUTPUT_FORMAT_YUV420)) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output format(%d)", format);
		}

		if (bit_width != DMA_OUTPUT_BIT_WIDTH_8BIT) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output format(%d)", bit_width);
		}
		break;
	default:
		err_hw("Invalid MCSC type(%d)", type);
		break;
	}

	return ret;
}

static void fimc_is_hw_mcsc_size_dump(struct fimc_is_hw_ip *hw_ip)
{
	u32 otf_path, dma_path = 0;
	u32 poly_in_w, poly_in_h = 0;
	u32 poly_crop_w, poly_crop_h = 0;
	u32 poly_out_w, poly_out_h = 0;
	u32 post_in_w, post_in_h = 0;
	u32 post_out_w, post_out_h = 0;
	u32 dma_y_stride, dma_uv_stride = 0;

	otf_path = fimc_is_scaler_get_otf_out_enable(hw_ip->regs);
	dma_path = fimc_is_scaler_get_dma_enable(hw_ip->regs);

	fimc_is_scaler_get_poly_img_size(hw_ip->regs, &poly_in_w, &poly_in_h);
	fimc_is_scaler_get_poly_src_size(hw_ip->regs, &poly_crop_w, &poly_crop_h);
	fimc_is_scaler_get_poly_dst_size(hw_ip->regs, &poly_out_w, &poly_out_h);
	fimc_is_scaler_get_post_img_size(hw_ip->regs, &post_in_w, &post_in_h);
	fimc_is_scaler_get_post_dst_size(hw_ip->regs, &post_out_w, &post_out_h);
	fimc_is_scaler_get_stride_size(hw_ip->regs, &dma_y_stride, &dma_uv_stride);

	sdbg_hw(2, "=SIZE=====================================\n"
		"[OTF:%d], [DMA:%d]\n"
		"[POLY] IN:%dx%d, CROP:%dx%d, OUT:%dx%d\n"
		"[POST] IN:%dx%d, OUT:%dx%d\n"
		"[DMA_STRIDE] Y:%d, UV:%d\n"
		"[MCSC]==========================================\n",
		hw_ip, otf_path, dma_path,
		poly_in_w, poly_in_h, poly_crop_w, poly_crop_h, poly_out_w, poly_out_h,
		post_in_w, post_in_h, post_out_w, post_out_h,
		dma_y_stride, dma_uv_stride);

	return;
}

const struct fimc_is_hw_ip_ops fimc_is_hw_mcsc_ops = {
	.open			= fimc_is_hw_mcsc_open,
	.init			= fimc_is_hw_mcsc_init,
	.close			= fimc_is_hw_mcsc_close,
	.enable			= fimc_is_hw_mcsc_enable,
	.disable		= fimc_is_hw_mcsc_disable,
	.shot			= fimc_is_hw_mcsc_shot,
	.set_param		= fimc_is_hw_mcsc_set_param,
	.frame_ndone		= fimc_is_hw_mcsc_frame_ndone,
	.load_setfile		= fimc_is_hw_mcsc_load_setfile,
	.apply_setfile		= fimc_is_hw_mcsc_apply_setfile,
	.delete_setfile		= fimc_is_hw_mcsc_delete_setfile,
	.size_dump		= fimc_is_hw_mcsc_size_dump,
	.clk_gate		= fimc_is_hardware_clk_gate
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
	hw_ip->is_leader = false;
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

	sinfo_hw("probe done\n", hw_ip);

	return ret;
}
