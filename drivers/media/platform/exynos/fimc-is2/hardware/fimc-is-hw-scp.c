/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-scp.h"
#include "api/fimc-is-hw-api-scp.h"
#include "../interface/fimc-is-interface-ischain.h"
#include "fimc-is-err.h"

static int fimc_is_hw_scp_handle_interrupt(u32 id, void *context)
{
	struct fimc_is_hw_ip *hw_ip = NULL;
	u32 status, intr_mask, intr_status;
	bool err_intr_flag = false;
	int ret = 0;

	hw_ip = (struct fimc_is_hw_ip *)context;

	/* read interrupt status register (sc_intr_status) */
	intr_mask = fimc_is_scp_get_intr_mask(hw_ip->regs);
	intr_status = fimc_is_scp_get_intr_status(hw_ip->regs);
	status = (~intr_mask) & intr_status;

	if (status & (1 << INTR_SCALER_STALL_BLOCKING)) {
		serr_hw("STALL BLOCKING!!", hw_ip);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_SCALER_OUTSTALL_BLOCKING)) {
		serr_hw("BACK STALL BLOCKING!!", hw_ip);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_SCALER_CORE_START)) {
		hw_ip->fs_time = lock_clock();
		if (!atomic_read(&hw_ip->hardware->stream_on))
			sinfo_hw("[F:%d]F.S\n", hw_ip, hw_ip->fcount);

		atomic_set(&hw_ip->status.Vvalid, V_VALID);
		clear_bit(HW_CONFIG, &hw_ip->state);
		atomic_inc(&hw_ip->count.fs);
	}

	if (status & (1 << INTR_SCALER_CORE_END)) {
		hw_ip->fe_time = local_clock();
		if (!atomic_read(&hw_ip->hardware->stream_on))
			sinfo_hw("[F:%d]F.E\n", hw_ip, hw_ip->fcount);

		atomic_set(&hw_ip->status.Vvalid, V_BLANK);
		if (atomic_read(&hw_ip->count.fs) == atomic_read(&hw_ip->count.fe)) {
			serr_hw("fs(%d), fe(%d), dma(%d)\n", hw_ip,
				atomic_read(&hw_ip->count.fs),
				atomic_read(&hw_ip->count.fe),
				atomic_read(&hw_ip->count.dma));
		}
		atomic_inc(&hw_ip->count.fe);
		fimc_is_hardware_frame_done(hw_ip, NULL, -1, FIMC_IS_HW_CORE_END,
			FRAME_DONE_NORMAL);
	}

	if (status & (1 << INTR_SCALER_FRAME_END)) {
		hw_ip->dma_time = local_clock();
		if (!atomic_read(&hw_ip->hardware->stream_on))
			sinfo_hw("[F:%d]F.E DMA\n", hw_ip, hw_ip->fcount);

		atomic_inc(&hw_ip->count.dma);
		fimc_is_hardware_frame_done(hw_ip, NULL, WORK_SCP_FDONE, ENTRY_SCP,
			FRAME_DONE_NORMAL);
	}

	fimc_is_scp_clear_intr_src(hw_ip->regs, status);

	return ret;
}

static int fimc_is_hw_scp_open(struct fimc_is_hw_ip *hw_ip, u32 instance,
	struct fimc_is_group *group)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	ret = fimc_is_hw_scp_reset(hw_ip);
	if (ret != 0) {
		mserr_hw("reset fail", instance, hw_ip);
		ret = -ENODEV;
		goto err_reset;
	}

	hw_ip->priv_info = vzalloc(sizeof(struct fimc_is_hw_scp));
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

static int fimc_is_hw_scp_init(struct fimc_is_hw_ip *hw_ip, u32 instance,
	struct fimc_is_group *group, bool flag, u32 module_id)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	set_bit(HW_INIT, &hw_ip->state);
	return ret;
}

static int fimc_is_hw_scp_close(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	vfree(hw_ip->priv_info);

	clear_bit(HW_OPEN, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_scp_enable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	set_bit(HW_RUN, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_scp_disable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit(hw_ip->id, &hw_map))
		return 0;

	msinfo_hw("disable: Vvalid(%d)\n", instance, hw_ip,
		atomic_read(&hw_ip->status.Vvalid));

	if (test_bit(HW_RUN, &hw_ip->state)) {
		/* disable SCP */
		fimc_is_scp_clear_dma_out_addr(hw_ip->regs);
		fimc_is_scp_stop(hw_ip->regs);

		ret = fimc_is_hw_scp_reset(hw_ip);
		if (ret != 0) {
			serr_hw("reset fail", hw_ip);
			return -ENODEV;
		}

		atomic_set(&hw_ip->status.Vvalid, V_BLANK);
		clear_bit(HW_RUN, &hw_ip->state);
		clear_bit(HW_CONFIG, &hw_ip->state);
	} else {
		msdbg_hw(2, "already disabled\n", instance, hw_ip);
	}


	return ret;
}

static int fimc_is_hw_scp_shot(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_scp *hw_scp;
	struct scp_param *param;
	u32 target_addr[4] = {0};

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	msdbgs_hw(2, "[F:%d] shot\n", frame->instance, hw_ip, frame->fcount);

	hw_scp = (struct fimc_is_hw_scp *)hw_ip->priv_info;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		msdbg_hw(2, "not initialized\n", frame->instance, hw_ip);
		return -EINVAL;
	}

	if (!test_bit(hw_ip->id, &hw_map)) {
		return 0;
	}
	set_bit(hw_ip->id, &frame->core_flag);

	param = (struct scp_param *)&hw_ip->region[frame->instance]->parameter.scalerp;

	if (frame->type == SHOT_TYPE_INTERNAL) {
		msdbg_hw(2, "request not exist\n", frame->instance, hw_ip);
		goto config;
	}

	fimc_is_hw_scp_update_register(frame->instance, hw_ip, param);

	/* set scp dma out addr */
	target_addr[0] = frame->scpTargetAddress[0];
	target_addr[1] = frame->scpTargetAddress[1];
	target_addr[2] = frame->scpTargetAddress[2];

	msdbg_hw(2, "[F:%d] target addr [0x%x]\n", frame->instance, hw_ip,
		frame->fcount, target_addr[0]);

config:
	if (param->dma_output.cmd != DMA_OUTPUT_COMMAND_DISABLE
		&& target_addr[0] != 0
		&& frame->type != SHOT_TYPE_INTERNAL) {
		fimc_is_scp_set_dma_enable(hw_ip->regs, true);
		/* use only one buffer (per-frame) */
		fimc_is_scp_set_dma_out_frame_seq(hw_ip->regs,
			0x1 << USE_DMA_BUFFER_INDEX);
		fimc_is_scp_set_dma_out_addr(hw_ip->regs,
			target_addr[0], target_addr[1], target_addr[2],
			USE_DMA_BUFFER_INDEX);
	} else {
		fimc_is_scp_set_dma_enable(hw_ip->regs, false);
		fimc_is_scp_clear_dma_out_addr(hw_ip->regs);
		msdbg_hw(2, "Disable dma out\n", frame->instance, hw_ip);
	}

	if (!test_bit(HW_CONFIG, &hw_ip->state)) {
		msdbg_hw(2, "[F:%d]start\n", frame->instance, hw_ip, frame->fcount);
		fimc_is_scp_start(hw_ip->regs);
		if (ret) {
			mserr_hw("enable failed!!\n", frame->instance, hw_ip);
			return -EINVAL;
		}
	}
	set_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_scp_set_param(struct fimc_is_hw_ip *hw_ip, struct is_region *region,
	u32 lindex, u32 hindex, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_scp *hw_scp;
	struct scp_param *param;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!region);

	if (!test_bit(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	hw_ip->region[instance] = region;
	hw_ip->lindex[instance] = lindex;
	hw_ip->hindex[instance] = hindex;

	param = &region->parameter.scalerp;
	hw_scp = (struct fimc_is_hw_scp *)hw_ip->priv_info;

	fimc_is_hw_scp_update_register(instance, hw_ip, param);

	return ret;
}

int fimc_is_hw_scp_update_register(u32 instance, struct fimc_is_hw_ip *hw_ip,
	struct scp_param *param)
{
	int ret = 0;

	if (param->otf_input.cmd == OTF_INPUT_COMMAND_ENABLE)
		ret = fimc_is_hw_scp_otf_input(instance, hw_ip);

	if (param->effect.cmd != SCALER_IMAGE_EFFECT_COMMNAD_DISABLE)
		ret = fimc_is_hw_scp_effect(instance, hw_ip);

	if (param->input_crop.cmd == SCALER_CROP_COMMAND_ENABLE)
		ret = fimc_is_hw_scp_input_crop(instance, hw_ip);

	if (param->output_crop.cmd == SCALER_CROP_COMMAND_ENABLE)
		ret = fimc_is_hw_scp_output_crop(instance, hw_ip);

	if (param->otf_output.cmd == OTF_OUTPUT_COMMAND_ENABLE)
		ret = fimc_is_hw_scp_otf_output(instance, hw_ip);

	if (param->dma_output.cmd == DMA_OUTPUT_COMMAND_ENABLE)
		ret = fimc_is_hw_scp_dma_output(instance, hw_ip);

	if (param->flip.cmd >= SCALER_FLIP_COMMAND_NORMAL)
		ret = fimc_is_hw_scp_flip(instance, hw_ip);

	if (param->effect.yuv_range >= SCALER_OUTPUT_YUV_RANGE_FULL)
		ret = fimc_is_hw_scp_output_yuvrange(instance, hw_ip);

	return 0;
}

int fimc_is_hw_scp_reset(struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	ret = fimc_is_scp_sw_reset(hw_ip->regs);
	if (ret != 0) {
		serr_hw("sw reset fail", hw_ip);
		return -ENODEV;
	}

	fimc_is_scp_set_trigger_mode(hw_ip->regs, false);
	fimc_is_scp_clear_intr_all(hw_ip->regs);
	fimc_is_scp_disable_intr(hw_ip->regs);
	fimc_is_scp_mask_intr(hw_ip->regs, SCALER_INTR_MASK);

	return ret;
}

static int fimc_is_hw_scp_load_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_scp *hw_scp = NULL;
	struct fimc_is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;
	u32 index;

	FIMC_BUG(!hw_ip);

	if (!test_bit(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -ESRCH;
	}

	if (!unlikely(hw_ip->priv_info)) {
		mserr_hw("priv info is NULL", instance, hw_ip);
		return -EINVAL;
	}
	hw_scp = (struct fimc_is_hw_scp *)hw_ip->priv_info;
	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	switch (info->version) {
	case SETFILE_V2:
		break;
	case SETFILE_V3:
		break;
	default:
		mserr_hw("invalid version (%d)", instance, hw_ip,
			info->version);
		return -EINVAL;
	}

	for (index = 0; index < setfile->using_count; index++) {
		hw_scp->setfile = (struct hw_api_scaler_setfile *)setfile->table[index].addr;
		if (hw_scp->setfile->setfile_version != SCP_SETFILE_VERSION) {
			mserr_hw("setfile version(0x%x) is incorrect\n",
				instance, hw_ip, hw_scp->setfile->setfile_version);
			return -EINVAL;
		}
	}

	set_bit(HW_TUNESET, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_scp_apply_setfile(struct fimc_is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_scp *hw_scp = NULL;
	struct fimc_is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;
	u32 setfile_index = 0;

	if (!test_bit(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -ESRCH;
	}

	if (!unlikely(hw_ip->priv_info)) {
		mserr_hw("priv info is NULL", instance, hw_ip);
		return -EINVAL;
	}

	hw_scp = (struct fimc_is_hw_scp *)hw_ip->priv_info;
	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	if (!hw_scp->setfile)
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

static int fimc_is_hw_scp_delete_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance,
	ulong hw_map)
{
	struct fimc_is_hw_scp *hw_scp = NULL;

	FIMC_BUG(!hw_ip);

	if (!test_bit(hw_ip->id, &hw_map))
		return 0;

	if (test_bit(HW_TUNESET, &hw_ip->state)) {
		mswarn_hw("setfile already deleted", instance, hw_ip);
		return 0;
	}

	hw_scp = (struct fimc_is_hw_scp *)hw_ip->priv_info;
	hw_scp->setfile = NULL;
	clear_bit(HW_TUNESET, &hw_ip->state);

	return 0;
}

static int fimc_is_hw_scp_frame_ndone(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	u32 instance, enum ShotErrorType done_type)
{
	int ret = 0;
	int wq_id, output_id;

	wq_id     = WORK_SCP_FDONE;
	output_id = ENTRY_SCP;
	if (test_bit(output_id, &frame->out_flag))
		ret = fimc_is_hardware_frame_done(hw_ip, frame, wq_id, output_id,
				false);

	wq_id     = -1;
	output_id = FIMC_IS_HW_CORE_END;
	if (test_bit(hw_ip->id, &frame->core_flag))
		ret = fimc_is_hardware_frame_done(hw_ip, frame, wq_id, output_id,
				false);

	return ret;
}

int fimc_is_hw_scp_otf_input(u32 instance, struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;
	u32 width, height;
	u32 format;
	u32 bit_width;
	u32 enable = 1;
	struct scp_param *param = &hw_ip->region[instance]->parameter.scalerp;
	struct param_otf_input *otf_input = &(param->otf_input);

	width = otf_input->width;
	height = otf_input->height;
	format = otf_input->format;
	bit_width = otf_input->bitwidth;

	/* check otf input param */
	if (width < 16 || width > 8192) {
		merr_hw("Invalid MCSC input width(%d)", instance, width);
		return 0;
	}

	if (height < 16 || height > 8192) {
		merr_hw("Invalid MCSC input height(%d)", instance, width);
		return 0;
	}

	if (format != OTF_INPUT_FORMAT_YUV444) {
		merr_hw("Invalid MCSC input format(%d)", instance, format);
		return 0;
	}

	if (bit_width != OTF_INPUT_BIT_WIDTH_8BIT) {
		merr_hw("Invalid MCSC input bit_width(%d)", instance, bit_width);
		return 0;
	}

	fimc_is_scp_set_pre_size(hw_ip->regs, width, height);
	fimc_is_scp_set_dither(hw_ip->regs, enable);

	return ret;
}

int fimc_is_hw_scp_effect(u32 instance, struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;

	return ret;
}

int fimc_is_hw_scp_input_crop(u32 instance, struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;
	u32 poly_src_width, poly_src_height;
	u32 dst_width, dst_height;
	u32 sh_factor;
	u32 pre_hratio, pre_vratio;
	u32 hratio, vratio;
	struct scp_param *param = &hw_ip->region[instance]->parameter.scalerp;
	struct param_scaler_input_crop *input_crop = &(param->input_crop);
	struct param_otf_input *otf_input = &(param->otf_input);
	struct param_otf_output *otf_output = &(param->otf_output);
	struct param_dma_output *dma_output = &(param->dma_output);

	if (param->otf_output.cmd == OTF_OUTPUT_COMMAND_ENABLE) {
		dst_width  = otf_output->width;
		dst_height = otf_output->height;
	} else if (dma_output->cmd == DMA_OUTPUT_COMMAND_ENABLE) {
		dst_width  = dma_output->width;
		dst_height = dma_output->height;
	}

	fimc_is_hw_scp_adjust_pre_ratio(otf_input->width, otf_input->height,
		dst_width, dst_height,
		&pre_hratio, &pre_vratio, &sh_factor);

	poly_src_width  = otf_input->width  / pre_hratio;
	poly_src_height = otf_input->height / pre_vratio;

	hratio = (poly_src_width  << 16) / dst_width;
	vratio = (poly_src_height << 16) / dst_height;

	fimc_is_scp_set_pre_scaling_ratio(hw_ip->regs, sh_factor, pre_hratio, pre_vratio);
	fimc_is_scp_set_scaling_ratio(hw_ip->regs, hratio, vratio);

	fimc_is_scp_set_horizontal_coef(hw_ip->regs, hratio, vratio);
	fimc_is_scp_set_vertical_coef(hw_ip->regs, hratio, vratio);

	fimc_is_scp_set_img_size(hw_ip->regs,
		poly_src_width,
		poly_src_height);
	fimc_is_scp_set_src_size(hw_ip->regs,
		input_crop->pos_x,
		input_crop->pos_y,
		poly_src_width,
		poly_src_height);

	return ret;
}

int fimc_is_hw_scp_output_crop(u32 instance, struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;
	u32 position_x, position_y;
	u32 crop_width, crop_height;
	u32 format, plane;
	u32 y_h_Size, y_v_size, c_h_size, c_v_size;
	bool conv420_flag = false;
	struct scp_param *param = &hw_ip->region[instance]->parameter.scalerp;
	struct param_scaler_output_crop *output_crop = &(param->output_crop);

	position_x = output_crop->pos_x;
	position_y = output_crop->pos_y;
	crop_width = output_crop->crop_width;
	crop_height = output_crop->crop_height;
	format = output_crop->format;
	plane = param->dma_output.plane;

	if (output_crop->cmd != SCALER_CROP_COMMAND_ENABLE) {
		msdbg_hw(2, "Skip output crop\n", instance, hw_ip);
		return 0;
	}

	fimc_is_scp_set_dma_offset(hw_ip->regs, position_x, position_y,
		position_x, position_y,
		position_x, position_y);

	if (format == DMA_OUTPUT_FORMAT_YUV420)
		conv420_flag = true;

	fimc_is_hw_scp_calc_canv_size(crop_width, crop_height, plane, conv420_flag,
		&y_h_Size, &y_v_size, &c_h_size, &c_v_size);
	fimc_is_scp_set_canv_size(hw_ip->regs, y_h_Size, y_v_size, c_h_size, c_v_size);

	return ret;
}

int fimc_is_hw_scp_flip(u32 instance, struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;
	struct scp_param *param = &hw_ip->region[instance]->parameter.scalerp;
	struct param_scaler_flip *flip = &(param->flip);

	if (flip->cmd > SCALER_FLIP_COMMAND_XY_MIRROR) {
		mserr_hw("flip cmd(%d) is invalid", instance, hw_ip, flip->cmd);
		return 0;
	}

	fimc_is_scp_set_flip_mode(hw_ip->regs, flip->cmd);

	return ret;
}

int fimc_is_hw_scp_otf_output(u32 instance, struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;
	u32 dst_width, dst_height;
	u32 format;
	u32 bit_witdh;
	struct scp_param *param = &hw_ip->region[instance]->parameter.scalerp;
	struct param_otf_output *otf_output = &(param->otf_output);

	dst_width = otf_output->width;
	dst_height = otf_output->height;
	format = otf_output->format;
	bit_witdh = otf_output->bitwidth;

	/* check otf output param */
	if (dst_width < 32 || dst_width > 8192) {
		mserr_hw("Invalid Scaler output width(%d)", instance, hw_ip, dst_width);
		return 0;
	}

	if (dst_height < 16 || dst_height > 8192) {
		mserr_hw("Invalid Scaler output height(%d)", instance, hw_ip, dst_height);
		return 0;
	}

	if (format != OTF_OUTPUT_FORMAT_YUV444) {
		mserr_hw("output format(%d) is invalid!", instance, hw_ip, dst_height);
		return 0;
	}

	if (bit_witdh != OTF_OUTPUT_BIT_WIDTH_8BIT) {
		mserr_hw("output bit width(%d) is invalid!", instance, hw_ip, dst_height);
		return 0;
	}

	fimc_is_scp_set_dst_size(hw_ip->regs, dst_width, dst_height);

	if (otf_output->cmd == OTF_OUTPUT_COMMAND_ENABLE) {
		/* set direct out path : DIRECT_SEL : 01 :  YCbCr444 + Scaled output + NO Image effect */
		fimc_is_scp_set_direct_out_path(hw_ip->regs, 0, 1);
	} else {
		fimc_is_scp_set_direct_out_path(hw_ip->regs, 0, 0);
	}

	return ret;
}

int fimc_is_hw_scp_dma_output(u32 instance, struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;
	u32 scaled_width, scaled_height;
	u32 dst_width, dst_height;
	u32 format, plane, order;
	u32 selection;
	u32 order2p_out, order422_out;
	u32 img_format;
	bool conv420_en = false;
	struct scp_param *param = &hw_ip->region[instance]->parameter.scalerp;
	struct param_dma_output *dma_output = &(param->dma_output);

	dst_width = dma_output->width;
	dst_height = dma_output->height;
	format = dma_output->format;
	plane = dma_output->plane;
	order = dma_output->order;
	selection = dma_output->selection;

	if (dma_output->cmd != DMA_OUTPUT_COMMAND_ENABLE) {
		msdbg_hw(2, "Skip dma out\n", instance, hw_ip);
		fimc_is_scp_set_dma_out_path(hw_ip->regs, false, selection);
		return 0;
	} else if (dma_output->cmd == DMA_OUTPUT_COMMAND_ENABLE) {
		fimc_is_hw_scp_adjust_img_fmt(format, plane, order, &img_format, &conv420_en);
		fimc_is_hw_scp_adjust_order(plane, img_format, &order2p_out, &order422_out);
		fimc_is_scp_set_dma_out_path(hw_ip->regs, true, selection);
		fimc_is_scp_set_420_conversion(hw_ip->regs, 0, conv420_en);
		fimc_is_scp_set_dma_out_img_fmt(hw_ip->regs, order2p_out, order422_out, plane);

		if (fimc_is_scp_get_otf_out_enable(hw_ip->regs)) {
			fimc_is_scp_get_dst_size(hw_ip->regs,  &scaled_width, &scaled_height);

			if ((scaled_width != dst_width) || (scaled_height != dst_height)) {
				mswarn_hw("Invalid scaled size (%d/%d)(%d/%d)",
					instance, hw_ip, scaled_width, scaled_height,
					dst_width, dst_height);
				return 0;
			}
		}
		fimc_is_scp_set_dma_offset(hw_ip->regs, 0, 0, 0, 0, 0, 0);
	}

	fimc_is_scp_set_dst_size(hw_ip->regs, dst_width, dst_height);

	return ret;
}

int fimc_is_hw_scp_output_yuvrange(u32 instance, struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;
	int yuv_range = 0;
	struct scp_param *param;
	struct param_scaler_imageeffect *image_effect;
	struct fimc_is_hw_scp *hw_scp = NULL;
	scaler_setfile_contents contents;

	hw_scp = (struct fimc_is_hw_scp *)hw_ip->priv_info;
	param = &hw_ip->region[instance]->parameter.scalerp;
	image_effect = &(param->effect);
	yuv_range = image_effect->yuv_range;

	if (test_bit(HW_TUNESET, &hw_ip->state)) {
		/* set yuv range config value by scaler_param yuv_range mode */
		contents = hw_scp->setfile->contents[yuv_range];
		fimc_is_scp_set_b_c(hw_ip->regs, contents.y_offset,
						contents.y_gain);
		fimc_is_scp_set_h_s(hw_ip->regs,
			contents.c_gain00, contents.c_gain01,
			contents.c_gain10, contents.c_gain11);
		msdbg_hw(2, "set YUV range(%d) by setfile parameter\n",
			instance, hw_ip, yuv_range);
	} else {
		if (yuv_range == SCALER_OUTPUT_YUV_RANGE_FULL) {
			/* Y range - [0:255], U/V range - [0:255] */
			fimc_is_scp_set_b_c(hw_ip->regs, 0, 1024);
			fimc_is_scp_set_h_s(hw_ip->regs, 1024, 0, 0, 1024);
		} else if (yuv_range == SCALER_OUTPUT_YUV_RANGE_NARROW) {
			/* Y range - [16:235], U/V range - [16:239] */
			fimc_is_scp_set_b_c(hw_ip->regs, 256, 878);
			fimc_is_scp_set_h_s(hw_ip->regs, 900, 0, 0, 900);
		}
		msdbg_hw(2, "YUV range set default settings\n", instance, hw_ip);
	}

	return ret;
}

int fimc_is_hw_scp_adjust_img_fmt(u32 format, u32 plane, u32 order,
	u32 *img_format, bool *conv420_flag)
{
	int ret = 0;

	switch (format) {
	case DMA_OUTPUT_FORMAT_YUV420:
		*conv420_flag = true;
		switch (plane) {
		case 2:
			switch (order) {
			case DMA_OUTPUT_ORDER_CbCr:
				*img_format = SCP_YC420_2P_CBCR;
				break;
			case DMA_OUTPUT_ORDER_CrCb:
				*img_format = SCP_YC420_2P_CRCB;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = SCP_YC420_3P;
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
				*img_format = SCP_YC422_1P_CRYCBY;
				break;
			case DMA_OUTPUT_ORDER_CbYCrY:
				*img_format = SCP_YC422_1P_CBYCRY;
				break;
			case DMA_OUTPUT_ORDER_YCrYCb:
				*img_format = SCP_YC422_1P_YCRYCB;
				break;
			case DMA_OUTPUT_ORDER_YCbYCr:
				*img_format = SCP_YC422_1P_YCBYCR;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				break;
			}
			break;
		case 2:
			switch (order) {
			case DMA_OUTPUT_ORDER_CbCr:
				*img_format = SCP_YC422_2P_CBCR;
				break;
			case DMA_OUTPUT_ORDER_CrCb:
				*img_format = SCP_YC422_2P_CRCB;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = SCP_YC420_3P;
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


static void fimc_is_hw_scp_size_dump(struct fimc_is_hw_ip *hw_ip)
{
	return;
}

void fimc_is_hw_scp_adjust_order(u32 plane, u32 img_format, u32 *order2p_out,
	u32 *order422_out)
{
	if (plane == 2) {
		switch (img_format) {
		case SCP_YC420_2P_CRCB:
		case SCP_YC422_2P_CRCB:
			*order2p_out = 1;
			break;
		case SCP_YC420_2P_CBCR:
		case SCP_YC422_2P_CBCR:
			*order2p_out = 0;
			break;
		default:
			err_hw("output format error - (%d)", img_format);
			return;
		}
	} else if (plane == 1) {
		switch (img_format) {
		case SCP_YC422_1P_YCBYCR:
			*order422_out = 3;
			break;
		case SCP_YC422_1P_YCRYCB:
			*order422_out = 2;
			break;
		case SCP_YC422_1P_CBYCRY:
			*order422_out = 1;
			break;
		case SCP_YC422_1P_CRYCBY:
			*order422_out = 0;
			break;
		default:
			err_hw("output format error - (%d)", img_format);
			return;
		}
	}

	return;
}

void fimc_is_hw_scp_adjust_pre_ratio(u32 pre_width, u32 pre_height,
	u32 dst_width, u32 dst_height, u32 *pre_hratio, u32 *pre_vratio, u32 *sh_factor)
{
	/* 1/4 <= h ratio <= 8 */
	if (pre_width <= (dst_width << 2) && dst_width <= (pre_width << 3)) {
		/* 1/4 <= v ratio <= 8 */
		if (pre_height <= (dst_height << 2) && dst_height <= (pre_height << 3)) {
			/* bypass pre-scaler */
			*sh_factor = 0;
			*pre_hratio = 1;
			*pre_vratio = 1;
		/* 1/8 <= v ratio < 1/4 */
		} else if (pre_height <= (dst_height << 3) && (dst_height << 2) < pre_height) {
			*sh_factor = 1;
			*pre_hratio = 1;
			*pre_vratio = 2;
		/* 1/16 <= v ratio < 1/8 */
		} else if (pre_height <= (dst_height << 4) && (dst_height << 3) < pre_height) {
			*sh_factor = 2;
			*pre_hratio = 1;
			*pre_vratio = 4;
		} else {
			err_hw("out of range of v ratio");
			return;
		}
	/* 1/8 <= h ratio < 1/4 */
	} else if (pre_width <= (dst_width << 3) && (dst_width << 2) < pre_width) {
		if (pre_width <= (dst_width << 2) && dst_width <= (pre_width << 3)) {
			*sh_factor = 1;
			*pre_hratio = 2;
			*pre_vratio = 1;
		} else if (pre_height <= (dst_height << 3) && (dst_height << 2) < pre_height) {
			*sh_factor = 2;
			*pre_hratio = 2;
			*pre_vratio = 2;
		} else if (pre_height <= (dst_height << 4) && (dst_height << 3) < pre_height) {
			*sh_factor = 3;
			*pre_hratio = 2;
			*pre_vratio = 4;
		} else {
			err_hw("out of range of v ratio");
			return;
		}
	/* 1/16 <= h ratio < 1/8 */
	} else if (pre_width <= (dst_width << 4) && (dst_width << 3) < pre_width) {
		if (pre_width <= (dst_width << 2) && dst_width <= (pre_width << 3)) {
			*sh_factor = 2;
			*pre_hratio = 4;
			*pre_vratio = 1;
		} else if (pre_height <= (dst_height << 3) && (dst_height << 2) < pre_height) {
			*sh_factor = 3;
			*pre_hratio = 4;
			*pre_vratio = 2;
		} else if (pre_height <= (dst_height << 4) && (dst_height << 3) < pre_height) {
			*sh_factor = 4;
			*pre_hratio = 4;
			*pre_vratio = 4;
		} else {
			err_hw("out of range of v ratio");
			return;
		}
	} else {
		err_hw("out of range of h ratio");
			return;
	}

	return;
}

void fimc_is_hw_scp_calc_canv_size(u32 width, u32 height, u32 plane, bool conv420_flag,
	u32 *y_h_size, u32 *y_v_size, u32 *c_h_size, u32 *c_v_size)
{
	/* calculate canvas size (pre size) */
	*y_h_size = *c_h_size = width;
	*y_v_size = *c_v_size = height;

	if (conv420_flag == false) {
		if (plane == 1)
			*y_h_size = width * 2;
	} else {
		*c_v_size = height/2;
		if (plane == 3)
			*c_h_size = width/2;
	}

	*y_h_size = 8 * ((*y_h_size / 8) + ((*y_h_size % 8) > 0));
	*c_h_size = 8 * ((*c_h_size / 8) + ((*c_h_size % 8) > 0));
}

const struct fimc_is_hw_ip_ops fimc_is_hw_scp_ops = {
	.open			= fimc_is_hw_scp_open,
	.init			= fimc_is_hw_scp_init,
	.close			= fimc_is_hw_scp_close,
	.enable			= fimc_is_hw_scp_enable,
	.disable		= fimc_is_hw_scp_disable,
	.shot			= fimc_is_hw_scp_shot,
	.set_param		= fimc_is_hw_scp_set_param,
	.frame_ndone		= fimc_is_hw_scp_frame_ndone,
	.load_setfile		= fimc_is_hw_scp_load_setfile,
	.apply_setfile		= fimc_is_hw_scp_apply_setfile,
	.delete_setfile		= fimc_is_hw_scp_delete_setfile,
	.size_dump		= fimc_is_hw_scp_size_dump,
	.clk_gate		= fimc_is_hardware_clk_gate
};

int fimc_is_hw_scp_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
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
	hw_ip->ops  = &fimc_is_hw_scp_ops;
	hw_ip->itf  = itf;
	hw_ip->itfc = itfc;
	atomic_set(&hw_ip->fcount, 0);
	hw_ip->is_leader = false;
	atomic_set(&hw_ip->status.Vvalid, V_BLANK);
	atomic_set(&hw_ip->rsccount, 0);
	init_waitqueue_head(&hw_ip->status.wait_queue);

	/* set scp sfr base address */
	hw_slot = fimc_is_hw_slot_id(id);
	if (!valid_hw_slot_id(hw_slot)) {
		serr_hw("invalid hw_slot (%d)", hw_ip, hw_slot);
		return 0;
	}

	/* set scp interrupt handler */
	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler = &fimc_is_hw_scp_handle_interrupt;

	clear_bit(HW_OPEN, &hw_ip->state);
	clear_bit(HW_INIT, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);
	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_TUNESET, &hw_ip->state);

	sinfo_hw("probe done\n", hw_ip);

	return ret;
}
