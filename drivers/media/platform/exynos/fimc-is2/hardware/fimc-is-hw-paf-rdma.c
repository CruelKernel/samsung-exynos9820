/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-paf-rdma.h"
#include "fimc-is-err.h"
#include "fimc-is-param.h"
#if defined(CONFIG_CAMERA_PAFSTAT)
#include "../sensor/module_framework/pafstat/fimc-is-hw-pafstat.h"
#elif defined(CONFIG_CAMERA_PDP)
#include "../sensor/module_framework/pdp/fimc-is-hw-pdp.h"
#endif
static int fimc_is_hw_paf_handle_interrupt(u32 id, void *context)
{
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_ip *hw_ip = NULL;
	struct fimc_is_hw_paf *hw_paf = NULL;
	u32 hw_fcount, instance;
#if defined(CONFIG_CAMERA_PAFSTAT)
	void __iomem *paf_ctx_addr;
	u32 irq_src, irq_mask, status;
#endif

	hw_ip = (struct fimc_is_hw_ip *)context;
	hardware = hw_ip->hardware;
	hw_fcount = atomic_read(&hw_ip->fcount);
	instance = atomic_read(&hw_ip->instance);

	if (!test_bit(HW_INIT, &hw_ip->state))
		return IRQ_NONE;

	FIMC_BUG(!hw_ip->priv_info);
	hw_paf = (struct fimc_is_hw_paf *)hw_ip->priv_info;


#if defined(CONFIG_CAMERA_PAFSTAT)
	paf_ctx_addr = (hw_ip->id == DEV_HW_PAF1) ? hw_paf->paf_ctx1_regs : hw_paf->paf_ctx0_regs;

	irq_src = pafstat_hw_g_irq_src(paf_ctx_addr);
	irq_mask = pafstat_hw_g_irq_mask(paf_ctx_addr);
	status = (~irq_mask) & irq_src;

	pafstat_hw_s_irq_src(paf_ctx_addr, status);

	msdbg_hw(2, "PAFSTAT RDMA IRQ : %08X\n", instance, hw_ip, irq_src);

	if (status & (1 << PAFSTAT_INT_FRAME_START)) {
		msdbg_hw(2, "PAF: F.S[F:%d]", instance, hw_ip, hw_fcount);
		fimc_is_hardware_frame_start(hw_ip, instance);
	}

	if (status & (1 << PAFSTAT_INT_TOTAL_FRAME_END)) {
		msdbg_hw(2, "PAF: F.E[F:%d]", instance, hw_ip, hw_fcount);
		fimc_is_hardware_frame_done(hw_ip, NULL, -1, FIMC_IS_HW_CORE_END,
						IS_SHOT_SUCCESS, true);
		atomic_set(&hw_ip->status.Vvalid, V_BLANK);
		wake_up(&hw_ip->status.wait_queue);
		CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);
	}
#endif

	return 0;
}

static int fimc_is_hw_paf_open(struct fimc_is_hw_ip *hw_ip, u32 instance,
	struct fimc_is_group *group)
{
	int ret = 0;
	struct fimc_is_hw_paf *hw_paf = NULL;

	FIMC_BUG(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	frame_manager_probe(hw_ip->framemgr, FRAMEMGR_ID_HW | (1 << hw_ip->id), "HWPAF");
	frame_manager_probe(hw_ip->framemgr_late, FRAMEMGR_ID_HW | (1 << hw_ip->id) | 0xF000, "HWPAF LATE");
	frame_manager_open(hw_ip->framemgr, FIMC_IS_MAX_HW_FRAME);
	frame_manager_open(hw_ip->framemgr_late, FIMC_IS_MAX_HW_FRAME_LATE);

	hw_ip->priv_info = vzalloc(sizeof(struct fimc_is_hw_paf));
	if(!hw_ip->priv_info) {
		mserr_hw("hw_ip->priv_info(null)", instance, hw_ip);
		ret = -ENOMEM;
		goto err_alloc;
	}

	hw_paf = (struct fimc_is_hw_paf *)hw_ip->priv_info;

#if defined(CONFIG_CAMERA_PAFSTAT)
	/* set baseaddress for context */
	hw_paf->paf_core_regs = hw_ip->regs_b;
	hw_paf->paf_ctx0_regs = hw_paf->paf_core_regs + PAF_CONTEXT0_OFFSET;
	hw_paf->paf_ctx1_regs = hw_paf->paf_core_regs + PAF_CONTEXT1_OFFSET;

	hw_paf->paf_rdma_core_regs = hw_ip->regs;
	hw_paf->paf_rdma0_regs = hw_paf->paf_rdma_core_regs + PAF_RDMA0_OFFSET;
	hw_paf->paf_rdma1_regs = hw_paf->paf_rdma_core_regs + PAF_RDMA1_OFFSET;
#endif

	set_bit(HW_OPEN, &hw_ip->state);
	msdbg_hw(2, "open: [G:0x%x], framemgr[%s]", instance, hw_ip,
		GROUP_ID(group->id), hw_ip->framemgr->name);

	return 0;

err_alloc:
	frame_manager_close(hw_ip->framemgr);
	frame_manager_close(hw_ip->framemgr_late);
	return ret;
}

static int fimc_is_hw_paf_init(struct fimc_is_hw_ip *hw_ip, u32 instance,
	struct fimc_is_group *group, bool flag, u32 module_id)
{
	int ret = 0;
	struct fimc_is_hw_paf *hw_paf = NULL;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);
	FIMC_BUG(!group);

	hw_paf = (struct fimc_is_hw_paf *)hw_ip->priv_info;

	set_bit(HW_INIT, &hw_ip->state);
	return ret;
}

static int fimc_is_hw_paf_deinit(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_paf *hw_paf;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);

	hw_paf = (struct fimc_is_hw_paf *)hw_ip->priv_info;

	return ret;
}

static int fimc_is_hw_paf_close(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_paf *hw_paf;
#if defined(CONFIG_CAMERA_PAFSTAT)
	void __iomem *paf_rdma_addr;
#endif

	FIMC_BUG(!hw_ip);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	FIMC_BUG(!hw_ip->priv_info);

	hw_paf = (struct fimc_is_hw_paf *)hw_ip->priv_info;

#if defined(CONFIG_CAMERA_PAFSTAT)
	paf_rdma_addr = (hw_ip->id == DEV_HW_PAF1) ? hw_paf->paf_rdma1_regs : hw_paf->paf_rdma0_regs;
	fimc_is_hw_paf_rdma_enable(hw_paf->paf_rdma_core_regs, paf_rdma_addr, 0);
#endif

	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;
	frame_manager_close(hw_ip->framemgr);
	frame_manager_close(hw_ip->framemgr_late);

	clear_bit(HW_OPEN, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_paf_enable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
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

static int fimc_is_hw_paf_disable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	long timetowait;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	msinfo_hw("disable: Vvalid(%d)\n", instance, hw_ip,
		atomic_read(&hw_ip->status.Vvalid));

	timetowait = wait_event_timeout(hw_ip->status.wait_queue,
		!atomic_read(&hw_ip->status.Vvalid),
		FIMC_IS_HW_STOP_TIMEOUT);

	if (!timetowait) {
		mserr_hw("wait FRAME_END timeout (%ld)", instance,
			hw_ip, timetowait);
		ret = -ETIME;
	}

	if (atomic_read(&hw_ip->rsccount) > 1)
		return 0;

	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static int fimc_is_hw_paf_shot(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	int i;
	struct fimc_is_hw_paf *hw_paf;
	struct is_region *region;
	struct paf_rdma_param *param;
	u32 lindex, hindex, instance;
#if defined(CONFIG_CAMERA_PAFSTAT)
	void __iomem *paf_rdma_addr;
	void __iomem *paf_ctx_addr;
#endif

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	instance = frame->instance;
	msdbgs_hw(2, "[F:%d]shot\n", instance, hw_ip, frame->fcount);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	FIMC_BUG(!hw_ip->priv_info);
	hw_paf = (struct fimc_is_hw_paf *)hw_ip->priv_info;
	region = hw_ip->region[instance];
	FIMC_BUG(!region);

	FIMC_BUG(!frame->shot);
	/* per-frame control
	 * check & update size from region */
	lindex = frame->shot->ctl.vendor_entry.lowIndexParam;
	hindex = frame->shot->ctl.vendor_entry.highIndexParam;

	if (hw_ip->internal_fcount[instance] != 0) {
		hw_ip->internal_fcount[instance] = 0;
		lindex |= LOWBIT_OF(PARAM_PAF_DMA_INPUT);
		lindex |= LOWBIT_OF(PARAM_PAF_OTF_OUTPUT);

		hindex |= HIGHBIT_OF(PARAM_PAF_DMA_INPUT);
		hindex |= HIGHBIT_OF(PARAM_PAF_OTF_OUTPUT);
	}

	/* DMA settings */
	param = &hw_ip->region[instance]->parameter.paf;
	if (param->dma_input.cmd != DMA_INPUT_COMMAND_DISABLE) {
		for (i = 0; i < frame->planes; i++) {
			hw_paf->input_dva[i] = frame->dvaddr_buffer[i];
			if (frame->dvaddr_buffer[i] == 0) {
				msinfo_hw("[F:%d]dvaddr_buffer[%d] is zero\n",
					instance, hw_ip, frame->fcount, i);
				FIMC_BUG(1);
			}
		}
	}

	/* multi-buffer */
	if (frame->num_buffers)
		hw_ip->num_buffers = frame->num_buffers;

#if defined(CONFIG_CAMERA_PAFSTAT)
	paf_ctx_addr = (hw_ip->id == DEV_HW_PAF1) ? hw_paf->paf_ctx1_regs : hw_paf->paf_ctx0_regs;
	paf_rdma_addr = (hw_ip->id == DEV_HW_PAF1) ? hw_paf->paf_rdma1_regs : hw_paf->paf_rdma0_regs;

	ret = fimc_is_hw_paf_update_param(hw_ip,
		region, param,
		lindex, hindex, instance);

	fimc_is_hw_paf_rdma_enable(hw_paf->paf_rdma_core_regs, paf_rdma_addr, 1);
	fimc_is_hw_paf_oneshot_enable(paf_ctx_addr, 1);
#endif

	set_bit(hw_ip->id, &frame->core_flag);
	set_bit(HW_CONFIG, &hw_ip->state);

#if 0
	fimc_is_hw_paf_sfr_dump(hw_paf->paf_core_regs, hw_paf->paf_rdma_core_regs);
	fimc_is_hw_paf_sfr_dump(hw_paf->paf_ctx0_regs, hw_paf->paf_rdma0_regs);
	fimc_is_hw_paf_sfr_dump(hw_paf->paf_ctx1_regs, hw_paf->paf_rdma1_regs);
#endif

	return ret;
}

static int fimc_is_hw_paf_set_param(struct fimc_is_hw_ip *hw_ip, struct is_region *region,
	u32 lindex, u32 hindex, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_paf *hw_paf;
	struct paf_rdma_param *param;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	FIMC_BUG(!hw_ip->priv_info);

	hw_paf = (struct fimc_is_hw_paf *)hw_ip->priv_info;
	param = &hw_ip->region[instance]->parameter.paf;

	hw_ip->region[instance] = region;
	hw_ip->lindex[instance] = lindex;
	hw_ip->hindex[instance] = hindex;

	return ret;
}

int fimc_is_hw_paf_update_param(struct fimc_is_hw_ip *hw_ip, struct is_region *region,
	struct paf_rdma_param *param, u32 lindex, u32 hindex, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_paf *hw_paf;
	u32 hw_format, bitwidth;
	u32 width, height;
#if defined(CONFIG_CAMERA_PAFSTAT)
	u32 paf_ch;
	void __iomem *paf_ctx_addr;
	void __iomem *paf_rdma_addr;
#endif

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!region);
	FIMC_BUG(!param);
	FIMC_BUG(!hw_ip->priv_info);

	hw_paf = (struct fimc_is_hw_paf *)hw_ip->priv_info;
	param = &hw_ip->region[instance]->parameter.paf;

	hw_format = param->dma_input.format;
	bitwidth = param->dma_input.bitwidth;
	width = param->dma_input.width;
	height = param->dma_input.height;

#if defined(CONFIG_CAMERA_PAFSTAT)
	paf_ch = (hw_ip->id == DEV_HW_PAF1) ? 1 : 0;
	paf_ctx_addr = (hw_ip->id == DEV_HW_PAF1) ? hw_paf->paf_ctx1_regs : hw_paf->paf_ctx0_regs;
	paf_rdma_addr = (hw_ip->id == DEV_HW_PAF1) ? hw_paf->paf_rdma1_regs : hw_paf->paf_rdma0_regs;

	if (width == 0 || height == 0) {
		mserr_hw("pafstat size is zero : not configured\n", instance, hw_ip);
		return 0;
	}

	fimc_is_hw_paf_common_config(hw_paf->paf_core_regs, paf_ctx_addr, paf_ch, width, height);
	fimc_is_hw_paf_rdma_set_addr(paf_rdma_addr, hw_paf->input_dva[0]);
	fimc_is_hw_paf_rdma_config(paf_rdma_addr, hw_format, bitwidth, width, height);
#endif

	return ret;
}

static int fimc_is_hw_paf_frame_ndone(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	u32 instance, enum ShotErrorType done_type)
{
	int output_id;
	int ret = 0;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	output_id = FIMC_IS_HW_CORE_END;
	if (test_bit_variables(hw_ip->id, &frame->core_flag))
		ret = fimc_is_hardware_frame_done(hw_ip, frame, -1,
				output_id, done_type, false);

	return ret;
}

const struct fimc_is_hw_ip_ops fimc_is_hw_paf_ops = {
	.open			= fimc_is_hw_paf_open,
	.init			= fimc_is_hw_paf_init,
	.deinit			= fimc_is_hw_paf_deinit,
	.close			= fimc_is_hw_paf_close,
	.enable			= fimc_is_hw_paf_enable,
	.disable		= fimc_is_hw_paf_disable,
	.shot			= fimc_is_hw_paf_shot,
	.set_param		= fimc_is_hw_paf_set_param,
	.frame_ndone		= fimc_is_hw_paf_frame_ndone,
	.clk_gate		= fimc_is_hardware_clk_gate,
};

int fimc_is_hw_paf_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
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
	hw_ip->ops  = &fimc_is_hw_paf_ops;
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
		return -EINVAL;
	}

	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler = &fimc_is_hw_paf_handle_interrupt;

	clear_bit(HW_OPEN, &hw_ip->state);
	clear_bit(HW_INIT, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);
	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_TUNESET, &hw_ip->state);

	sinfo_hw("probe done\n", hw_ip);

	return ret;
}
