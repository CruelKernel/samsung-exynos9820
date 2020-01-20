/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-dcp.h"
#include "fimc-is-regs.h"
#include "fimc-is-param.h"
#include "fimc-is-err.h"

extern struct fimc_is_lib_support gPtr_lib_support;

static int fimc_is_hw_dcp_open(struct fimc_is_hw_ip *hw_ip, u32 instance,
	struct fimc_is_group *group)
{
	int ret = 0;
	struct fimc_is_hw_dcp *hw_dcp = NULL;

	FIMC_BUG(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	frame_manager_probe(hw_ip->framemgr, FRAMEMGR_ID_HW | hw_ip->id, "HWDCP");
	frame_manager_open(hw_ip->framemgr, FIMC_IS_MAX_HW_FRAME);

	frame_manager_probe(hw_ip->framemgr_late, FRAMEMGR_ID_HW | hw_ip->id | 0xF0, "HWDCP LATE");
	frame_manager_open(hw_ip->framemgr_late, FIMC_IS_MAX_HW_FRAME_LATE);

	hw_ip->priv_info = vzalloc(sizeof(struct fimc_is_hw_dcp));
	if(!hw_ip->priv_info) {
		mserr_hw("hw_ip->priv_info(null)", instance, hw_ip);
		ret = -ENOMEM;
		goto err_alloc;
	}

	hw_dcp = (struct fimc_is_hw_dcp *)hw_ip->priv_info;
#ifdef ENABLE_FPSIMD_FOR_USER
	fpsimd_get();
	ret = get_lib_func(LIB_FUNC_DCP, (void **)&hw_dcp->lib_func);
	fpsimd_put();
#else
	ret = get_lib_func(LIB_FUNC_DCP, (void **)&hw_dcp->lib_func);
#endif

	if (!hw_dcp->lib_func) {
		mserr_hw("func_dcp(null)", instance, hw_ip);
		fimc_is_load_clear();
		ret = -EINVAL;
		goto err_lib_func;
	}
	msinfo_hw("get_lib_func is set\n", instance, hw_ip);

	hw_dcp->lib_support = &gPtr_lib_support;
	hw_dcp->lib[instance].func   = hw_dcp->lib_func;

	ret = fimc_is_lib_isp_chain_create(hw_ip, &hw_dcp->lib[instance], instance);
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
	hw_ip->priv_info = NULL;
err_alloc:
	frame_manager_close(hw_ip->framemgr);
	frame_manager_close(hw_ip->framemgr_late);
	return ret;
}

static int fimc_is_hw_dcp_init(struct fimc_is_hw_ip *hw_ip, u32 instance,
	struct fimc_is_group *group, bool flag, u32 module_id)
{
	int ret = 0;
	struct fimc_is_hw_dcp *hw_dcp = NULL;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);
	FIMC_BUG(!group);

	hw_dcp = (struct fimc_is_hw_dcp *)hw_ip->priv_info;

	hw_dcp->lib[instance].object = NULL;
	hw_dcp->lib[instance].func   = hw_dcp->lib_func;
	hw_dcp->param_set[instance].reprocessing = flag;

	if (hw_dcp->lib[instance].object) {
		msdbg_hw(2, "object is already created\n", instance, hw_ip);
	} else {
		ret = fimc_is_lib_isp_object_create(hw_ip, &hw_dcp->lib[instance],
				instance, (u32)flag, module_id);
		if (ret) {
			mserr_hw("object create fail", instance, hw_ip);
			return -EINVAL;
		}
	}

	set_bit(HW_INIT, &hw_ip->state);
	return ret;
}

static int fimc_is_hw_dcp_deinit(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_dcp *hw_dcp;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);

	hw_dcp = (struct fimc_is_hw_dcp *)hw_ip->priv_info;

	fimc_is_lib_isp_object_destroy(hw_ip, &hw_dcp->lib[instance], instance);
	hw_dcp->lib[instance].object = NULL;

	return ret;
}

static int fimc_is_hw_dcp_close(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_dcp *hw_dcp;

	FIMC_BUG(!hw_ip);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	FIMC_BUG(!hw_ip->priv_info);
	hw_dcp = (struct fimc_is_hw_dcp *)hw_ip->priv_info;

	fimc_is_lib_isp_chain_destroy(hw_ip, &hw_dcp->lib[instance], instance);
	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;
	frame_manager_close(hw_ip->framemgr);
	frame_manager_close(hw_ip->framemgr_late);

	clear_bit(HW_OPEN, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_dcp_enable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
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

static int fimc_is_hw_dcp_disable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	long timetowait;
	struct fimc_is_hw_dcp *hw_dcp;
	struct dcp_param_set *param_set;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	msinfo_hw("dcp_disable: Vvalid(%d)\n", instance, hw_ip,
		atomic_read(&hw_ip->status.Vvalid));

	FIMC_BUG(!hw_ip->priv_info);
	hw_dcp = (struct fimc_is_hw_dcp *)hw_ip->priv_info;
	param_set = &hw_dcp->param_set[instance];

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
		fimc_is_lib_isp_stop(hw_ip, &hw_dcp->lib[instance], instance);
	} else {
		msdbg_hw(2, "already disabled\n", instance, hw_ip);
	}

	if (atomic_read(&hw_ip->rsccount) > 1)
		return 0;

	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static void fimc_is_hw_dcp_check_param(struct dcp_param *param,
	struct dcp_param_set *param_set, u32 *lindex, u32 *hindex)
{
	if (param->control.cmd != param_set->control.cmd) {
		*lindex |= LOWBIT_OF(PARAM_DCP_CONTROL);
		*hindex |= HIGHBIT_OF(PARAM_DCP_CONTROL);
	}

	if (param->dma_input_m.cmd != param_set->dma_input_m.cmd) {
		*lindex |= LOWBIT_OF(PARAM_DCP_INPUT_MASTER);
		*hindex |= HIGHBIT_OF(PARAM_DCP_INPUT_MASTER);
	}

	if (param->dma_input_s.cmd != param_set->dma_input_s.cmd) {
		*lindex |= LOWBIT_OF(PARAM_DCP_INPUT_SLAVE);
		*hindex |= HIGHBIT_OF(PARAM_DCP_INPUT_SLAVE);
	}

	if (param->dma_output_m.cmd != param_set->dma_output_m.cmd) {
		*lindex |= LOWBIT_OF(PARAM_DCP_OUTPUT_MASTER);
		*hindex |= HIGHBIT_OF(PARAM_DCP_OUTPUT_MASTER);
	}

	if (param->dma_output_s.cmd != param_set->dma_output_s.cmd) {
		*lindex |= LOWBIT_OF(PARAM_DCP_OUTPUT_SLAVE);
		*hindex |= HIGHBIT_OF(PARAM_DCP_OUTPUT_SLAVE);
	}

	if (param->dma_output_m_ds.cmd != param_set->dma_output_m_ds.cmd) {
		*lindex |= LOWBIT_OF(PARAM_DCP_OUTPUT_MASTER_DS);
		*hindex |= HIGHBIT_OF(PARAM_DCP_OUTPUT_MASTER_DS);
	}

	if (param->dma_output_s_ds.cmd != param_set->dma_output_s_ds.cmd) {
		*lindex |= LOWBIT_OF(PARAM_DCP_OUTPUT_SLAVE_DS);
		*hindex |= HIGHBIT_OF(PARAM_DCP_OUTPUT_SLAVE_DS);
	}

	if (param->dma_input_disparity.cmd != param_set->dma_input_disparity.cmd) {
		*lindex |= LOWBIT_OF(PARAM_DCP_INPIT_DISPARITY);
		*hindex |= HIGHBIT_OF(PARAM_DCP_INPIT_DISPARITY);
	}

	if (param->dma_output_disparity.cmd != param_set->dma_output_disparity.cmd) {
		*lindex |= LOWBIT_OF(PARAM_DCP_OUTPUT_DISPARITY);
		*hindex |= HIGHBIT_OF(PARAM_DCP_OUTPUT_DISPARITY);
	}
}

static void fimc_is_hw_dcp_update_param(struct dcp_param *param,
	struct dcp_param_set *param_set, u32 lindex, u32 hindex, u32 instance)
{
	param_set->instance_id = instance;

	if ((lindex & LOWBIT_OF(PARAM_DCP_CONTROL))
		|| (hindex & HIGHBIT_OF(PARAM_DCP_CONTROL))) {
		memcpy(&param_set->control, &param->control,
			sizeof(struct param_control));
	}

	if ((lindex & LOWBIT_OF(PARAM_DCP_INPUT_MASTER))
		|| (hindex & HIGHBIT_OF(PARAM_DCP_INPUT_MASTER))) {
		memcpy(&param_set->dma_input_m, &param->dma_input_m,
			sizeof(struct param_dma_input));
	}

	if ((lindex & LOWBIT_OF(PARAM_DCP_INPUT_SLAVE))
		|| (hindex & HIGHBIT_OF(PARAM_DCP_INPUT_SLAVE))) {
		memcpy(&param_set->dma_input_s, &param->dma_input_s,
			sizeof(struct param_dma_input));
	}

	if ((lindex & LOWBIT_OF(PARAM_DCP_OUTPUT_MASTER))
		|| (hindex & HIGHBIT_OF(PARAM_DCP_OUTPUT_MASTER))) {
		memcpy(&param_set->dma_output_m, &param->dma_output_m,
			sizeof(struct param_dma_output));
	}

	if ((lindex & LOWBIT_OF(PARAM_DCP_OUTPUT_SLAVE))
		|| (hindex & HIGHBIT_OF(PARAM_DCP_OUTPUT_SLAVE))) {
		memcpy(&param_set->dma_output_s, &param->dma_output_s,
			sizeof(struct param_dma_output));
	}

	if ((lindex & LOWBIT_OF(PARAM_DCP_OUTPUT_MASTER_DS))
		|| (hindex & HIGHBIT_OF(PARAM_DCP_OUTPUT_MASTER_DS))) {
		memcpy(&param_set->dma_output_m_ds, &param->dma_output_m_ds,
			sizeof(struct param_dma_output));
	}

	if ((lindex & LOWBIT_OF(PARAM_DCP_OUTPUT_SLAVE_DS))
		|| (hindex & HIGHBIT_OF(PARAM_DCP_OUTPUT_SLAVE_DS))) {
		memcpy(&param_set->dma_output_s_ds, &param->dma_output_s_ds,
			sizeof(struct param_dma_output));
	}

	if ((lindex & LOWBIT_OF(PARAM_DCP_INPIT_DISPARITY))
		|| (hindex & HIGHBIT_OF(PARAM_DCP_INPIT_DISPARITY))) {
		memcpy(&param_set->dma_input_disparity, &param->dma_input_disparity,
			sizeof(struct param_dma_output));
	}

	if ((lindex & LOWBIT_OF(PARAM_DCP_OUTPUT_DISPARITY))
		|| (hindex & HIGHBIT_OF(PARAM_DCP_OUTPUT_DISPARITY))) {
		memcpy(&param_set->dma_output_disparity, &param->dma_output_disparity,
			sizeof(struct param_dma_output));
	}
}

static int fimc_is_hw_dcp_shot(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	int i = 0;
	int plane;
	struct fimc_is_hw_dcp *hw_dcp;
	struct dcp_param_set *param_set;
	struct is_region *region;
	struct dcp_param *param;
	u32 lindex, hindex, instance;
	bool frame_done = false;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	instance = frame->instance;
	msdbgs_hw(2, "[F:%d]shot\n", instance, hw_ip, frame->fcount);

	if (!test_bit(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!\n", instance, hw_ip);
		return -EINVAL;
	}

	fimc_is_hw_g_ctrl(hw_ip, hw_ip->id, HW_G_CTRL_FRM_DONE_WITH_DMA, (void *)&frame_done);
	if ((!frame_done)
		|| (!test_bit(ENTRY_DC0C, &frame->out_flag) && !test_bit(ENTRY_DC1C, &frame->out_flag)))
		set_bit(hw_ip->id, &frame->core_flag);

	FIMC_BUG(!hw_ip->priv_info);
	hw_dcp = (struct fimc_is_hw_dcp *)hw_ip->priv_info;
	param_set = &hw_dcp->param_set[instance];
	region = hw_ip->region[instance];
	FIMC_BUG(!region);

	param = &region->parameter.dcp;

	if (frame->type == SHOT_TYPE_INTERNAL) {
		hw_ip->internal_fcount[instance] = frame->fcount;
		mserr_hw("[F:%d]frame->type(%d) invalid\n", instance, hw_ip, frame->fcount,
			frame->type);
		return 0;
	} else {
		FIMC_BUG(!frame->shot);
		/* per-frame control
		 * check & update size from region */
		lindex = frame->shot->ctl.vendor_entry.lowIndexParam;
		hindex = frame->shot->ctl.vendor_entry.highIndexParam;

		if (hw_ip->internal_fcount[instance]) {
			hw_ip->internal_fcount[instance] = 0;
			fimc_is_hw_dcp_check_param(param, param_set, &lindex, &hindex);
		}
	}

	fimc_is_hw_dcp_update_param(param, param_set, lindex, hindex, instance);

	/* DMA settings */
	plane = param->dma_input_m.plane;
	if (param_set->dma_input_m.cmd == DMA_INPUT_COMMAND_ENABLE) {
		for (i = 0; i < plane; i++) {
			param_set->input_dva[DCP_DMA_IN_GDC_MASTER][i] =
				(typeof(**param_set->input_dva))frame->dvaddr_buffer[i];
			if (param_set->input_dva[DCP_DMA_IN_GDC_MASTER][i] == 0) {
				mserr_hw("[F:%d]DCP_DMA_IN_GDC_MASTER plane[%d] dva is zero",
					instance, hw_ip, frame->fcount, i);
				return -EINVAL;
			}
		}
	}

	/* Slave input */
	plane = param->dma_input_s.plane;
	if (param_set->dma_input_s.cmd == DMA_INPUT_COMMAND_ENABLE) {
		for (i = 0; i < plane; i++) {
			param_set->input_dva[DCP_DMA_IN_GDC_SLAVE][i] =
				frame->sourceAddress[i];
			if (param_set->input_dva[DCP_DMA_IN_GDC_SLAVE][i] == 0) {
				mserr_hw("[F:%d]DCP_DMA_IN_GDC_SLAVE plane[%d]dva is zero",
					instance, hw_ip, frame->fcount, i);
				return -EINVAL;
			}
		}
	}

	/* Master Main output: plane order is [Y, C, Y2, C2] */
	plane = param->dma_output_m.plane;
	if (param_set->dma_output_m.cmd == DMA_INPUT_COMMAND_ENABLE) {
		for (i = 0; i < plane; i++) {
			param_set->output_dva[DCP_DMA_OUT_MASTER][i] =
				frame->sccTargetAddress[i];
			if (param_set->output_dva[DCP_DMA_OUT_MASTER][i] == 0) {
				mserr_hw("[F:%d]DCP_DMA_OUT_MASTER plane[%d] dva is zero",
					instance, hw_ip, frame->fcount, i);
				return -EINVAL;
			}
		}
	}

	/* Slave Main output: plane order is [Y, C] */
	plane = param->dma_output_s.plane;
	if (param_set->dma_output_s.cmd == DMA_INPUT_COMMAND_ENABLE) {
		for (i = 0; i < plane; i++) {
			param_set->output_dva[DCP_DMA_OUT_SLAVE][i] =
				frame->scpTargetAddress[i];
			if (param_set->output_dva[DCP_DMA_OUT_SLAVE][i] == 0) {
				mserr_hw("[F:%d]DCP_DMA_OUT_SLAVE plane[%d] dva is zero",
					instance, hw_ip, frame->fcount, i);
				return -EINVAL;
			}
		}
	}

	/* Disparity */
	if (param_set->dma_output_disparity.cmd == DMA_INPUT_COMMAND_ENABLE) {
		/* disparity input: previous data */
		param_set->input_dva[DCP_DMA_IN_DISPARITY][0] =
			param_set->output_dva[DCP_DMA_OUT_DISPARITY][0];

		/* disparity output: currnet data */
		param_set->output_dva[DCP_DMA_OUT_DISPARITY][0] =
			frame->dxcTargetAddress[0];
		if (param_set->output_dva[DCP_DMA_OUT_DISPARITY][0] == 0) {
			mserr_hw("[F:%d]DCP_DMA_OUT_DISPARITY plane[%d] dva is zero",
				instance, hw_ip, frame->fcount, 0);
			return -EINVAL;
		}
	}

	/* Master Sub output: plane order is [Y, C] */
	plane = param->dma_output_m_ds.plane;
	if (param_set->dma_output_m_ds.cmd == DMA_INPUT_COMMAND_ENABLE) {
		for (i = 0; i < plane; i++) {
			param_set->output_dva[DCP_DMA_OUT_MASTER_DS][i] =
				frame->sccTargetAddress[8 + i];

			if (param_set->output_dva[DCP_DMA_OUT_MASTER_DS][i] == 0) {
				mserr_hw("[F:%d]DCP_DMA_OUT_MASTER_DS plane[%d] dva is zero",
					instance, hw_ip, frame->fcount, i);
				return -EINVAL;
			}
		}
	}

	/* Slave Sub output: plane order is [Y, C] */
	plane = param->dma_output_s_ds.plane;
	if (param_set->dma_output_s_ds.cmd == DMA_INPUT_COMMAND_ENABLE) {
		for (i = 0; i < plane; i++) {
			param_set->output_dva[DCP_DMA_OUT_SLAVE_DS][i] =
				frame->scpTargetAddress[8 + i];
			if (param_set->output_dva[DCP_DMA_OUT_SLAVE_DS][i] == 0) {
				mserr_hw("[F:%d]DCP_DMA_OUT_SLAVE_DS plane[%d] dva is zero",
					instance, hw_ip, frame->fcount, i);
				return -EINVAL;
			}
		}
	}

	/* param_set->instance_id = instance; */ /* TODO: remove */
	param_set->fcount = frame->fcount;

	/* multi-buffer: currently not support HFR */
	if (frame->num_buffers)
		hw_ip->num_buffers = frame->num_buffers;

	if (param_set->control.cmd == CONTROL_COMMAND_STOP)
		return 0;

	if (frame->shot) {
		ret = fimc_is_lib_isp_set_ctrl(hw_ip, &hw_dcp->lib[instance], frame);
		if (ret)
			mserr_hw("set_ctrl fail", instance, hw_ip);
	}

	fimc_is_lib_isp_shot(hw_ip, &hw_dcp->lib[instance],
			param_set, frame->shot);

	set_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_dcp_set_param(struct fimc_is_hw_ip *hw_ip, struct is_region *region,
	u32 lindex, u32 hindex, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_dcp *hw_dcp;
	struct dcp_param_set *param_set;
	struct dcp_param *param = NULL;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!region);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		msdbg_hw(2, "not initialized\n", instance, hw_ip);
		return -EINVAL;
	}

	FIMC_BUG(!hw_ip->priv_info);
	hw_dcp = (struct fimc_is_hw_dcp *)hw_ip->priv_info;
	param_set = &hw_dcp->param_set[instance];

	hw_ip->region[instance] = region;
	hw_ip->lindex[instance] = lindex;
	hw_ip->hindex[instance] = hindex;

	param = &(hw_ip->region[instance]->parameter.dcp);

	fimc_is_hw_dcp_update_param(param, param_set,
				lindex, hindex, instance);

	return ret;
}

static int fimc_is_hw_dcp_get_meta(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_dcp *hw_dcp;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	if (!test_bit(hw_ip->id, &hw_map))
		return 0;

	FIMC_BUG(!hw_ip->priv_info);
	hw_dcp = (struct fimc_is_hw_dcp *)hw_ip->priv_info;

	ret = fimc_is_lib_isp_get_meta(hw_ip, &hw_dcp->lib[frame->instance], frame);
	if (ret)
		mserr_hw("get_meta fail", frame->instance, hw_ip);

	return ret;
}

static int fimc_is_hw_dcp_frame_ndone(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	u32 instance, enum ShotErrorType done_type)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	if (test_bit(ENTRY_DC1S, &frame->out_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			WORK_DC1S_FDONE, ENTRY_DC1S, done_type, false);
	}


	if (test_bit(ENTRY_DC0C, &frame->out_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			WORK_DC0C_FDONE, ENTRY_DC0C, done_type, false);
	}


	if (test_bit(ENTRY_DC1C, &frame->out_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			WORK_DC1C_FDONE, ENTRY_DC1C, done_type, false);
	}


	if (test_bit(ENTRY_DC2C, &frame->out_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			WORK_DC2C_FDONE, ENTRY_DC2C, done_type, false);
	}

	if (test_bit(ENTRY_DC3C, &frame->out_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			WORK_DC3C_FDONE, ENTRY_DC3C, done_type, false);
	}

	if (test_bit(ENTRY_DC4C, &frame->out_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			WORK_DC4C_FDONE, ENTRY_DC4C, done_type, false);
	}

	if (test_bit(hw_ip->id, &frame->core_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			-1, FIMC_IS_HW_CORE_END, done_type, false);
	}

	return ret;
}

static int fimc_is_hw_dcp_load_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -ESRCH;
	}

	set_bit(HW_TUNESET, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_dcp_apply_setfile(struct fimc_is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	u32 setfile_index = 0;
	int ret = 0;
	struct fimc_is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
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

	return ret;
}

static int fimc_is_hw_dcp_delete_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance,
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

static void fimc_is_hw_dcp_size_dump(struct fimc_is_hw_ip *hw_ip)
{
	return;
}

const struct fimc_is_hw_ip_ops fimc_is_hw_dcp_ops = {
	.open			= fimc_is_hw_dcp_open,
	.init			= fimc_is_hw_dcp_init,
	.deinit			= fimc_is_hw_dcp_deinit,
	.close			= fimc_is_hw_dcp_close,
	.enable			= fimc_is_hw_dcp_enable,
	.disable		= fimc_is_hw_dcp_disable,
	.shot			= fimc_is_hw_dcp_shot,
	.set_param		= fimc_is_hw_dcp_set_param,
	.get_meta		= fimc_is_hw_dcp_get_meta,
	.frame_ndone		= fimc_is_hw_dcp_frame_ndone,
	.load_setfile		= fimc_is_hw_dcp_load_setfile,
	.apply_setfile		= fimc_is_hw_dcp_apply_setfile,
	.delete_setfile		= fimc_is_hw_dcp_delete_setfile,
	.size_dump		= fimc_is_hw_dcp_size_dump,
	.clk_gate		= fimc_is_hardware_clk_gate
};

int fimc_is_hw_dcp_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
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
	hw_ip->ops  = &fimc_is_hw_dcp_ops;
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
