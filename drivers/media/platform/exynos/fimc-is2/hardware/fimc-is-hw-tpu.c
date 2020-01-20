/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-tpu.h"
#include "fimc-is-regs.h"
#include "fimc-is-param.h"
#include "fimc-is-err.h"

extern struct fimc_is_lib_support gPtr_lib_support;

static int fimc_is_hw_tpu_open(struct fimc_is_hw_ip *hw_ip, u32 instance,
	struct fimc_is_group *group)
{
	int ret = 0;
	struct fimc_is_hw_tpu *hw_tpu = NULL;

	FIMC_BUG(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	frame_manager_probe(hw_ip->framemgr, FRAMEMGR_ID_HW | (1 << hw_ip->id), "HWTPU");
	frame_manager_probe(hw_ip->framemgr_late, FRAMEMGR_ID_HW | (1 << hw_ip->id) | 0xF000, "HWTPU LATE");
	frame_manager_open(hw_ip->framemgr, FIMC_IS_MAX_HW_FRAME);
	frame_manager_open(hw_ip->framemgr_late, FIMC_IS_MAX_HW_FRAME_LATE);

	hw_ip->priv_info = vzalloc(sizeof(struct fimc_is_hw_tpu));
	if(!hw_ip->priv_info) {
		mserr_hw("hw_ip->priv_info(null)", instance, hw_ip);
		ret = -ENOMEM;
		goto err_alloc;
	}

	hw_tpu = (struct fimc_is_hw_tpu *)hw_ip->priv_info;
#ifdef ENABLE_FPSIMD_FOR_USER
	fpsimd_get();
	ret = get_lib_func(LIB_FUNC_TPU, (void **)&hw_tpu->lib_func);
	fpsimd_put();
#else
	ret = get_lib_func(LIB_FUNC_TPU, (void **)&hw_tpu->lib_func);
#endif

	if (!hw_tpu->lib_func) {
		mserr_hw("func_tpu(null)", instance, hw_ip);
		fimc_is_load_clear();
		ret = -EINVAL;
		goto err_lib_func;
	}
	msinfo_hw("get_lib_func is set\n", instance, hw_ip);

	hw_tpu->lib_support = &gPtr_lib_support;
	hw_tpu->lib[instance].func = hw_tpu->lib_func;

	fimc_is_hw_s_ctrl(hw_ip->itfc, hw_ip->id, HW_S_CTRL_FULL_BYPASS, (void *)true);

	ret = fimc_is_lib_isp_chain_create(hw_ip, &hw_tpu->lib[instance], instance);
	if (ret) {
		mserr_hw("chain create fail", instance, hw_ip);
		ret = -EINVAL;
		goto err_chain_create;
	}

	set_bit(HW_OPEN, &hw_ip->state);
	msdbg_hw(2, "open: [G:0x%x], framemgr[%s]", instance, hw_ip,
		GROUP_ID(group->id), hw_ip->framemgr->name);

	return 0;

err_chain_create:
err_lib_func:
	vfree(hw_ip->priv_info);
err_alloc:
	frame_manager_close(hw_ip->framemgr);
	frame_manager_close(hw_ip->framemgr_late);
	return ret;
}

static int fimc_is_hw_tpu_init(struct fimc_is_hw_ip *hw_ip, u32 instance,
	struct fimc_is_group *group, bool flag, u32 module_id)
{
	int ret = 0;
	struct fimc_is_hw_tpu *hw_tpu = NULL;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);
	FIMC_BUG(!group);

	hw_tpu = (struct fimc_is_hw_tpu *)hw_ip->priv_info;

	hw_tpu->lib[instance].object = NULL;
	hw_tpu->lib[instance].func   = hw_tpu->lib_func;
	hw_tpu->param_set[instance].reprocessing = flag;

	if (hw_tpu->lib[instance].object) {
		msdbg_hw(2, "object is already created\n", instance, hw_ip);
	} else {
		ret = fimc_is_lib_isp_object_create(hw_ip, &hw_tpu->lib[instance],
				instance, (u32)flag, module_id);
		if (ret) {
			mserr_hw("object create fail", instance, hw_ip);
			return -EINVAL;
		}
	}

	set_bit(HW_INIT, &hw_ip->state);
	return ret;
}

static int fimc_is_hw_tpu_deinit(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_tpu *hw_tpu;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);

	hw_tpu = (struct fimc_is_hw_tpu *)hw_ip->priv_info;

	fimc_is_lib_isp_object_destroy(hw_ip, &hw_tpu->lib[instance], instance);
	hw_tpu->lib[instance].object = NULL;

	return ret;
}

static int fimc_is_hw_tpu_close(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_tpu *hw_tpu;

	FIMC_BUG(!hw_ip);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	FIMC_BUG(!hw_ip->priv_info);
	hw_tpu = (struct fimc_is_hw_tpu *)hw_ip->priv_info;

	fimc_is_lib_isp_chain_destroy(hw_ip, &hw_tpu->lib[instance], instance);
	vfree(hw_ip->priv_info);
	frame_manager_close(hw_ip->framemgr);
	frame_manager_close(hw_ip->framemgr_late);

	clear_bit(HW_OPEN, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_tpu_enable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized\n", instance, hw_ip);
		return -EINVAL;
	}

	set_bit(HW_RUN, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_tpu_disable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	long timetowait;
	struct fimc_is_hw_tpu *hw_tpu;
	struct tpu_param_set *param_set;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	msinfo_hw("tpu_disable: Vvalid(%d)\n", instance, hw_ip,
		atomic_read(&hw_ip->status.Vvalid));

	FIMC_BUG(!hw_ip->priv_info);
	hw_tpu = (struct fimc_is_hw_tpu *)hw_ip->priv_info;
	param_set = &hw_tpu->param_set[instance];

	timetowait = wait_event_timeout(hw_ip->status.wait_queue,
		!atomic_read(&hw_ip->status.Vvalid),
		FIMC_IS_HW_STOP_TIMEOUT);

	if (!timetowait) {
		mserr_hw("wait FRAME_END timeout (%ld)", instance,
			hw_ip, timetowait);
		ret = -ETIME;
	}

	param_set->fcount = 0;
	if (test_bit(HW_RUN, &hw_ip->state)) {
		fimc_is_lib_isp_stop(hw_ip, &hw_tpu->lib[instance], instance);
		clear_bit(HW_RUN, &hw_ip->state);
	} else {
		msdbg_hw(2, "already disabled\n", instance, hw_ip);
	}

	clear_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static void fimc_is_hw_tpu_check_param(struct tpu_param *param,
	struct tpu_param_set *param_set, u32 *lindex, u32 *hindex)
{
	if (param->control.cmd != param_set->control.cmd) {
		*lindex |= LOWBIT_OF(PARAM_TPU_CONTROL);
		*hindex |= HIGHBIT_OF(PARAM_TPU_CONTROL);
	}

	if ((param->config.odc_bypass != param_set->config.odc_bypass)
		|| (param->config.dis_bypass != param_set->config.dis_bypass)
		|| (param->config.tdnr_bypass != param_set->config.tdnr_bypass)) {
		*lindex |= LOWBIT_OF(PARAM_TPU_CONFIG);
		*hindex |= HIGHBIT_OF(PARAM_TPU_CONFIG);
	}

	if (param->otf_input.cmd != param_set->otf_input.cmd) {
		*lindex |= LOWBIT_OF(PARAM_TPU_OTF_INPUT);
		*hindex |= HIGHBIT_OF(PARAM_TPU_OTF_INPUT);
	}

	if (param->dma_input.cmd != param_set->dma_input.cmd) {
		*lindex |= LOWBIT_OF(PARAM_TPU_DMA_INPUT);
		*hindex |= HIGHBIT_OF(PARAM_TPU_DMA_INPUT);
	}

	if (param->otf_output.cmd != param_set->otf_output.cmd) {
		*lindex |= LOWBIT_OF(PARAM_TPU_OTF_OUTPUT);
		*hindex |= HIGHBIT_OF(PARAM_TPU_OTF_OUTPUT);
	}

	if (param->dma_output.cmd != param_set->dma_output.cmd) {
		*lindex |= LOWBIT_OF(PARAM_TPU_DMA_OUTPUT);
		*hindex |= HIGHBIT_OF(PARAM_TPU_DMA_OUTPUT);
	}
}

static void fimc_is_hw_tpu_update_param(struct tpu_param *param,
	struct tpu_param_set *param_set, u32 lindex, u32 hindex)
{
	if ((lindex & LOWBIT_OF(PARAM_TPU_CONTROL))
		|| (hindex & HIGHBIT_OF(PARAM_TPU_CONTROL))) {
		memcpy(&param_set->control, &param->control,
			sizeof(struct param_control));
	}

	if ((lindex & LOWBIT_OF(PARAM_TPU_CONFIG))
		|| (hindex & HIGHBIT_OF(PARAM_TPU_CONFIG))) {
		memcpy(&param_set->config, &param->config,
			sizeof(struct param_tpu_config));
	}

	if ((lindex & LOWBIT_OF(PARAM_TPU_OTF_INPUT))
		|| (hindex & HIGHBIT_OF(PARAM_TPU_OTF_INPUT))) {
		memcpy(&param_set->otf_input, &param->otf_input,
			sizeof(struct param_otf_input));
	}

	if ((lindex & LOWBIT_OF(PARAM_TPU_DMA_INPUT))
		|| (hindex & HIGHBIT_OF(PARAM_TPU_DMA_INPUT))) {
		memcpy(&param_set->dma_input, &param->dma_input,
			sizeof(struct param_dma_input));
	}

	if ((lindex & LOWBIT_OF(PARAM_TPU_OTF_OUTPUT))
		|| (hindex & HIGHBIT_OF(PARAM_TPU_OTF_OUTPUT))) {
		memcpy(&param_set->otf_output, &param->otf_output,
			sizeof(struct param_otf_output));
	}

	if ((lindex & LOWBIT_OF(PARAM_TPU_DMA_OUTPUT))
		|| (hindex & HIGHBIT_OF(PARAM_TPU_DMA_OUTPUT))) {
		memcpy(&param_set->dma_output, &param->dma_output,
			sizeof(struct param_dma_output));
	}
}

static int fimc_is_hw_tpu_shot(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	int i;
	struct fimc_is_hw_tpu *hw_tpu;
	struct tpu_param_set *param_set;
	struct is_region *region;
	struct tpu_param *param;
	u32 lindex, hindex, instance;
	bool frame_done = false;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	instance = frame->instance;
	msdbgs_hw(2, "[F:%d] shot\n", instance, hw_ip, frame->fcount);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	if (!test_bit(hw_ip->id, &hw_map))
		return 0;

	fimc_is_hw_g_ctrl(hw_ip, hw_ip->id, HW_G_CTRL_FRM_DONE_WITH_DMA, (void *)&frame_done);
	if ((!frame_done)
		|| (!test_bit(ENTRY_DXC, &frame->out_flag)))
		set_bit(hw_ip->id, &frame->core_flag);

	FIMC_BUG(!hw_ip->priv_info);
	hw_tpu = (struct fimc_is_hw_tpu *)hw_ip->priv_info;
	param_set = &hw_tpu->param_set[instance];
	region = hw_ip->region[instance];
	FIMC_BUG(!region);

	param = &region->parameter.tpu;

	if (frame->type == SHOT_TYPE_INTERNAL) {
		param_set->dma_input.cmd = DMA_INPUT_COMMAND_DISABLE;
		param_set->dma_output.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		param_set->input_dva[0] = 0;
		param_set->output_dva[0] = 0;
		hw_ip->internal_fcount[instance] = frame->fcount;
		goto config;
	} else {
		FIMC_BUG(!frame->shot);
		/* per-frame control
		 * check & update size from region */
		lindex = frame->shot->ctl.vendor_entry.lowIndexParam;
		hindex = frame->shot->ctl.vendor_entry.highIndexParam;

		if (hw_ip->internal_fcount[instance]) {
			hw_ip->internal_fcount[instance] = 0;
			fimc_is_hw_tpu_check_param(param, param_set, &lindex, &hindex);
			param_set->dma_output.cmd  = param->dma_output.cmd;
		}
	}

	fimc_is_hw_tpu_update_param(param, param_set, lindex, hindex);

	/* DMA settings */
	if (param_set->dma_input.cmd != DMA_INPUT_COMMAND_DISABLE) {
		for (i = 0; i < frame->num_buffers; i++) {
			param_set->input_dva[i] = (typeof(*param_set->input_dva))
				frame->dvaddr_buffer[frame->cur_buf_index + i];
			if (!frame->dvaddr_buffer[i]) {
				mserr_hw("[F:%d]dvaddr_buffer[%d] is zero",
					instance, hw_ip, frame->fcount, i);
				FIMC_BUG(1);
			}
		}
	}

	if (param_set->dma_output.cmd != DMA_OUTPUT_COMMAND_DISABLE) {
		for (i = 0; i < frame->num_buffers; i++) {
			param_set->output_dva[i] = frame->dxcTargetAddress[frame->cur_buf_index + i];
			if (frame->dxcTargetAddress[i] == 0) {
				msinfo_hw("[F:%d]dxcTargetAddress[%d] is zero",
					instance, hw_ip, frame->fcount, i);
				param_set->dma_output.cmd = DMA_OUTPUT_COMMAND_DISABLE;
			}
		}
	}

config:
	param_set->instance_id = instance;
	param_set->fcount = frame->fcount;

	/* multi-buffer */
	if (frame->num_buffers)
		hw_ip->num_buffers = frame->num_buffers;

	if (param_set->control.cmd == CONTROL_COMMAND_STOP) {
		if (param_set->control.bypass == CONTROL_BYPASS_ENABLE) {
			fimc_is_hw_s_ctrl(hw_ip->itfc, hw_ip->id, HW_S_CTRL_FULL_BYPASS, (void *)true);
			clear_bit(hw_ip->id, &frame->core_flag);
		} else {
			mswarn_hw("[F:%d]shot: control invalid: cmd(%d), bypass(%d)",
				instance, hw_ip, frame->fcount,
				param_set->control.cmd, param_set->control.bypass);
		}
		return 0;
	} else if (param_set->control.cmd == CONTROL_COMMAND_START) {
		fimc_is_hw_s_ctrl(hw_ip->itfc, hw_ip->id, HW_S_CTRL_FULL_BYPASS, (void *)false);
	}

	if (frame->shot) {
		ret = fimc_is_lib_isp_set_ctrl(hw_ip, &hw_tpu->lib[param_set->instance_id], frame);
		if (ret)
			mserr_hw("set_ctrl fail", instance, hw_ip);
	}

	fimc_is_lib_isp_shot(hw_ip, &hw_tpu->lib[instance],
			param_set, frame->shot);

	set_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_tpu_set_param(struct fimc_is_hw_ip *hw_ip, struct is_region *region,
	u32 lindex, u32 hindex, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_tpu *hw_tpu;
	struct tpu_param *param = NULL;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!region);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		msdbg_hw(2, "not initialized\n", instance, hw_ip);
		return -EINVAL;
	}

	FIMC_BUG(!hw_ip->priv_info);
	hw_tpu = (struct fimc_is_hw_tpu *)hw_ip->priv_info;

	hw_ip->region[instance] = region;
	hw_ip->lindex[instance] = lindex;
	hw_ip->hindex[instance] = hindex;

	/* set full-bypass */
	param = &(hw_ip->region[instance]->parameter.tpu);

	fimc_is_hw_tpu_update_param(param, &hw_tpu->param_set[instance],
				lindex, hindex);

	return ret;
}

static int fimc_is_hw_tpu_get_meta(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_tpu *hw_tpu;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	if (!test_bit(hw_ip->id, &hw_map))
		return 0;

	FIMC_BUG(!hw_ip->priv_info);
	hw_tpu = (struct fimc_is_hw_tpu *)hw_ip->priv_info;

	ret = fimc_is_lib_isp_get_meta(hw_ip, &hw_tpu->lib[frame->instance], frame);
	if (ret)
		mserr_hw("get_meta fail", frame->instance, hw_ip);

	return ret;
}

static int fimc_is_hw_tpu_frame_ndone(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	u32 instance, enum ShotErrorType done_type)
{
	int ret = 0;
	int wq_id, output_id;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	wq_id     = -1;
	output_id = FIMC_IS_HW_CORE_END;
	if (test_bit(hw_ip->id, &frame->core_flag))
		ret = fimc_is_hardware_frame_done(hw_ip, frame, wq_id,
				output_id, done_type, false);

	return ret;
}

static int fimc_is_hw_tpu_load_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		msdbg_hw(2, "Not initialized\n", instance, hw_ip);
		return -ESRCH;
	}

	return ret;
}

static int fimc_is_hw_tpu_apply_setfile(struct fimc_is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	u32 setfile_index = 0;
	int ret = 0;
	struct fimc_is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		msdbg_hw(2, "Not initialized\n", instance, hw_ip);
		return -ESRCH;
	}

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	if (setfile->using_count == 0)
		return 0;

	setfile_index = setfile->index[scenario];
	if (setfile_index >= setfile->using_count) {
		mserr_hw("setfile index is out-of-range, [%d:%d]",
				instance, hw_ip, scenario, setfile_index);
		return -EINVAL;
	}

	msinfo_hw("setfile (%d) scenario (%d)\n", instance, hw_ip,
		setfile_index, scenario);

	set_bit(HW_TUNESET, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_tpu_delete_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance,
	ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	if (setfile->using_count == 0)
		return 0;

	clear_bit(HW_TUNESET, &hw_ip->state);

	return ret;
}

static void fimc_is_hw_tpu_size_dump(struct fimc_is_hw_ip *hw_ip)
{
	return;
}

const struct fimc_is_hw_ip_ops fimc_is_hw_tpu_ops = {
	.open			= fimc_is_hw_tpu_open,
	.init			= fimc_is_hw_tpu_init,
	.deinit			= fimc_is_hw_tpu_deinit,
	.close			= fimc_is_hw_tpu_close,
	.enable			= fimc_is_hw_tpu_enable,
	.disable		= fimc_is_hw_tpu_disable,
	.shot			= fimc_is_hw_tpu_shot,
	.set_param		= fimc_is_hw_tpu_set_param,
	.get_meta		= fimc_is_hw_tpu_get_meta,
	.frame_ndone		= fimc_is_hw_tpu_frame_ndone,
	.load_setfile		= fimc_is_hw_tpu_load_setfile,
	.apply_setfile		= fimc_is_hw_tpu_apply_setfile,
	.delete_setfile		= fimc_is_hw_tpu_delete_setfile,
	.size_dump		= fimc_is_hw_tpu_size_dump,
	.clk_gate		= fimc_is_hardware_clk_gate
};

int fimc_is_hw_tpu_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
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
	hw_ip->ops  = &fimc_is_hw_tpu_ops;
	hw_ip->itf  = itf;
	hw_ip->itfc = itfc;
	atomic_set(&hw_ip->fcount, 0);
	hw_ip->is_leader = true;
	atomic_set(&hw_ip->status.Vvalid, V_BLANK);
	atomic_set(&hw_ip->status.otf_start, 0);
	atomic_set(&hw_ip->rsccount, 0);
	init_waitqueue_head(&hw_ip->status.wait_queue);

	hw_slot = fimc_is_hw_slot_id(id);
	if (!valid_hw_slot_id(hw_slot)) {
		serr_hw("invalid hw_slot (%d)", hw_ip, hw_slot);
		return 0;
	}

	clear_bit(HW_OPEN, &hw_ip->state);
	clear_bit(HW_INIT, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);
	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_TUNESET, &hw_ip->state);

	sinfo_hw("probe done\n", hw_ip);

	return ret;
}
