/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/kthread.h>

#include "fimc-is-err.h"
#include "fimc-is-core.h"
#include "fimc-is-hw-control.h"

#include "fimc-is-hw-3aa.h"
#include "fimc-is-hw-isp.h"
#include "fimc-is-hw-tpu.h"
#if defined(CONFIG_CAMERA_FIMC_SCALER_USE)
#include "fimc-is-hw-scp.h"
#elif defined(CONFIG_CAMERA_MC_SCALER_VER1_USE)
#include "fimc-is-hw-mcscaler-v1.h"
#elif defined(CONFIG_CAMERA_MC_SCALER_VER2_USE)
#include "fimc-is-hw-mcscaler-v2.h"
#endif
#include "fimc-is-hw-vra.h"
#include "fimc-is-hw-dcp.h"
#include "fimc-is-hw-dm.h"
#include "fimc-is-hw-paf-rdma.h"
#include "fimc-is-work.h"

#define INTERNAL_SHOT_EXIST	(1)

static inline void wq_func_schedule(struct fimc_is_interface *itf,
	struct work_struct *work_wq)
{
	if (itf->workqueue)
		queue_work(itf->workqueue, work_wq);
	else
		schedule_work(work_wq);
}

static void prepare_sfr_dump(struct fimc_is_hardware *hardware)
{
	int hw_slot = -1;
	int reg_size = 0;
	struct fimc_is_hw_ip *hw_ip = NULL;

	if (!hardware) {
		err_hw("hardware is null\n");
		return;
	}

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];

		if (hw_ip->id == DEV_HW_END || hw_ip->id == 0)
		       continue;

		if (hw_ip->id == DEV_HW_PAF0 || hw_ip->id == DEV_HW_PAF1)
		       continue;

		if (IS_ERR_OR_NULL(hw_ip->regs) ||
			(hw_ip->regs_start == 0) ||
			(hw_ip->regs_end == 0)) {
			swarn_hw("reg iomem is invalid", hw_ip);
			continue;
		}

		hw_ip->sfr_dump_flag = false;

		/* alloc sfr dump memory */
		reg_size = (hw_ip->regs_end - hw_ip->regs_start + 1);
		hw_ip->sfr_dump = (u8 *)kzalloc(reg_size, GFP_KERNEL);
		if (IS_ERR_OR_NULL(hw_ip->sfr_dump))
			serr_hw("sfr dump memory alloc fail", hw_ip);
		else
			sinfo_hw("sfr dump memory (V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX]\n", hw_ip,
				hw_ip->sfr_dump, (void *)virt_to_phys(hw_ip->sfr_dump),
				reg_size, hw_ip->regs_start, hw_ip->regs_end);

		if (IS_ERR_OR_NULL(hw_ip->regs_b) ||
			(hw_ip->regs_b_start == 0) ||
			(hw_ip->regs_b_end == 0))
			continue;

		/* alloc sfr B dump memory */
		reg_size = (hw_ip->regs_b_end - hw_ip->regs_b_start + 1);
		hw_ip->sfr_b_dump = (u8 *)kzalloc(reg_size, GFP_KERNEL);
		if (IS_ERR_OR_NULL(hw_ip->sfr_b_dump))
			serr_hw("sfr B dump memory alloc fail", hw_ip);
		else
			sinfo_hw("sfr B dump memory (V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX]\n", hw_ip,
				hw_ip->sfr_b_dump, (void *)virt_to_phys(hw_ip->sfr_b_dump),
				reg_size, hw_ip->regs_b_start, hw_ip->regs_b_end);
	}
}

static void _fimc_is_hardware_sfr_dump(struct fimc_is_hw_ip *hw_ip, bool flag_print_log)
{
	int reg_size = 0;
	int reg_b_size = 0;

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return;

	if (IS_ERR_OR_NULL(hw_ip->sfr_dump)) {
		swarn_hw("sfr_dump memory is invalid", hw_ip);
		return;
	}

	reg_size = (hw_ip->regs_end - hw_ip->regs_start + 1);
	reg_b_size = (hw_ip->regs_b_end - hw_ip->regs_b_start + 1);

	if (hw_ip->sfr_dump_flag) {
		sinfo_hw("sfr_dump was already dumped", hw_ip);

/* To dump only valid sfr addresses in VRA2.0 */
#ifdef SKIP_VRA_SFR_DUMP
		if (hw_ip->id == DEV_HW_VRA) {
			sinfo_hw("##### SFR DUMP(V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX, 0x%llX~0x%llX]\n", hw_ip,
					hw_ip->sfr_dump, (void *)virt_to_phys(hw_ip->sfr_dump),	reg_size,
					hw_ip->regs_start, hw_ip->regs_start + 0x4000, hw_ip->regs_start + 0x905c,
					hw_ip->regs_start + 0x905c + 0x7e4);
		} else
			sinfo_hw("##### SFR DUMP(V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX]\n", hw_ip,
					hw_ip->sfr_dump, (void *)virt_to_phys(hw_ip->sfr_dump),
					reg_size, hw_ip->regs_start, hw_ip->regs_end);
#else
		sinfo_hw("##### SFR DUMP(V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX]\n", hw_ip,
				hw_ip->sfr_dump, (void *)virt_to_phys(hw_ip->sfr_dump),
				reg_size, hw_ip->regs_start, hw_ip->regs_end);
#endif
		if (flag_print_log) {
#ifdef SKIP_VRA_SFR_DUMP
			if (hw_ip->id == DEV_HW_VRA) {
				print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
						hw_ip->sfr_dump, 0x4000, false);
				print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
						hw_ip->sfr_dump + 0x905c, 0x7e4, false);
			} else
				print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
						hw_ip->sfr_dump, reg_size, false);
#else
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
					hw_ip->sfr_dump, reg_size, false);
#endif
		}

		if (!IS_ERR_OR_NULL(hw_ip->sfr_b_dump)) {
			sinfo_hw("##### SFR B DUMP(V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX]\n", hw_ip,
				hw_ip->sfr_b_dump, (void *)virt_to_phys(hw_ip->sfr_b_dump),
				reg_b_size, hw_ip->regs_b_start, hw_ip->regs_b_end);
			if (flag_print_log)
				print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
						hw_ip->sfr_b_dump, reg_b_size, false);
		}
		return;
	}

	/* HACK: cannot access 0x3C SFR offset */
	if (hw_ip->id == DEV_HW_DCP)
		return;

	hw_ip->sfr_dump_flag = true;

	/* dump reg */
#ifdef SKIP_VRA_SFR_DUMP
	if (hw_ip->id == DEV_HW_VRA) {
		memcpy(hw_ip->sfr_dump, hw_ip->regs, 0x4000);
		memcpy(hw_ip->sfr_dump, hw_ip->regs + 0x905c, 0x7e4);
	} else
		memcpy(hw_ip->sfr_dump, hw_ip->regs, reg_size);
#else
	memcpy(hw_ip->sfr_dump, hw_ip->regs, reg_size);

#endif
#ifdef SKIP_VRA_SFR_DUMP
	if (hw_ip->id == DEV_HW_VRA) {
		sinfo_hw("##### SFR DUMP(V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX, 0x%llX~0x%llX]\n", hw_ip,
				hw_ip->sfr_dump, (void *)virt_to_phys(hw_ip->sfr_dump),	reg_size,
				hw_ip->regs_start, hw_ip->regs_start + 0x4000, hw_ip->regs_start + 0x905c,
				hw_ip->regs_start + 0x905c + 0x7e4);
	} else
		sinfo_hw("##### SFR DUMP(V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX]\n", hw_ip,
				hw_ip->sfr_dump, (void *)virt_to_phys(hw_ip->sfr_dump),
				reg_size, hw_ip->regs_start, hw_ip->regs_end);
#else
	sinfo_hw("##### SFR DUMP(V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX]\n", hw_ip,
			hw_ip->sfr_dump, (void *)virt_to_phys(hw_ip->sfr_dump),
			reg_size, hw_ip->regs_start, hw_ip->regs_end);
#endif
#ifdef ENABLE_PANIC_SFR_PRINT
#ifdef SKIP_VRA_SFR_DUMP
	if (hw_ip->id == DEV_HW_VRA) {
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
				hw_ip->regs, 0x4000, false);
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
				hw_ip->regs + 0x905c, 0x7e4, false);
	} else
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
				hw_ip->regs, reg_size, false);
#else
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
			hw_ip->regs, reg_size, false);
#endif
#else
	if (flag_print_log) {
#ifdef SKIP_VRA_SFR_DUMP
		if (hw_ip->id == DEV_HW_VRA) {
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
					hw_ip->regs, 0x4000, false);
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
					hw_ip->regs + 0x905c, 0x7e4, false);
		} else
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
					hw_ip->regs + 0x905c, 0x7e4, false);
#else
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
				hw_ip->regs, reg_size, false);
#endif
	}
#endif
	if (IS_ERR_OR_NULL(hw_ip->sfr_b_dump))
		return;

	/* dump reg B */
	memcpy(hw_ip->sfr_b_dump, hw_ip->regs_b, reg_b_size);

	sinfo_hw("##### SFR B DUMP(V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX]\n", hw_ip,
			hw_ip->sfr_b_dump, (void *)virt_to_phys(hw_ip->sfr_b_dump),
			reg_b_size, hw_ip->regs_b_start, hw_ip->regs_b_end);
#ifdef ENABLE_PANIC_SFR_PRINT
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
			hw_ip->regs_b, reg_b_size, false);
#else
	if (flag_print_log)
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
				hw_ip->regs_b, reg_b_size, false);
#endif
}

void print_hw_frame_count(struct fimc_is_hw_ip *hw_ip)
{
	int f_index, p_index;
	struct hw_debug_info *debug_info;
	ulong usec[DEBUG_FRAME_COUNT][DEBUG_POINT_MAX];

	if (!hw_ip) {
		err_hw("hw_ip is null\n");
		return;
	}

	/* skip printing frame count, if hw_ip wasn't opened */
	if (!test_bit(HW_OPEN, &hw_ip->state))
		return;

	sinfo_hw("fs(%d), cl(%d), fe(%d), dma(%d)\n", hw_ip,
			atomic_read(&hw_ip->count.fs),
			atomic_read(&hw_ip->count.cl),
			atomic_read(&hw_ip->count.fe),
			atomic_read(&hw_ip->count.dma));

	for (f_index = 0; f_index < DEBUG_FRAME_COUNT; f_index++) {
		debug_info = &hw_ip->debug_info[f_index];
		for (p_index = 0 ; p_index < DEBUG_POINT_MAX; p_index++)
			usec[f_index][p_index]  = do_div(debug_info->time[p_index], NSEC_PER_SEC);

		info_hw("[%d][F:%d] shot[%5lu.%06lu], fs[c%d][%5lu.%06lu], fe[c%d][%5lu.%06lu], dma[c%d][%5lu.%06lu], \n",
				f_index, debug_info->fcount,
				(ulong)debug_info->time[DEBUG_POINT_HW_SHOT], usec[f_index][DEBUG_POINT_HW_SHOT] / NSEC_PER_USEC,
				debug_info->cpuid[DEBUG_POINT_FRAME_START],
				(ulong)debug_info->time[DEBUG_POINT_FRAME_START], usec[f_index][DEBUG_POINT_FRAME_START] / NSEC_PER_USEC,
				debug_info->cpuid[DEBUG_POINT_FRAME_END],
				(ulong)debug_info->time[DEBUG_POINT_FRAME_END], usec[f_index][DEBUG_POINT_FRAME_END] / NSEC_PER_USEC,
				debug_info->cpuid[DEBUG_POINT_FRAME_DMA_END],
				(ulong)debug_info->time[DEBUG_POINT_FRAME_DMA_END], usec[f_index][DEBUG_POINT_FRAME_DMA_END] / NSEC_PER_USEC);
	}
}

void print_all_hw_frame_count(struct fimc_is_hardware *hardware)
{
	int hw_slot = -1;
	struct fimc_is_hw_ip *_hw_ip = NULL;

	if (!hardware) {
		err_hw("hardware is null\n");
		return;
	}

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		_hw_ip = &hardware->hw_ip[hw_slot];
		print_hw_frame_count(_hw_ip);
	}
}

void fimc_is_hardware_flush_frame(struct fimc_is_hw_ip *hw_ip,
	enum fimc_is_frame_state state,
	enum ShotErrorType done_type)
{
	int ret = 0;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	ulong flags = 0;
	int retry;

	FIMC_BUG_VOID(!hw_ip);

	framemgr = hw_ip->framemgr;

	framemgr_e_barrier_irqs(framemgr, 0, flags);
	while (state <  FS_HW_WAIT_DONE) {
		frame = peek_frame(framemgr, state);
		while (frame) {
			trans_frame(framemgr, frame, state + 1);
			frame = peek_frame(framemgr, state);
		}
		state++;
	}
	frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
	framemgr_x_barrier_irqr(framemgr, 0, flags);

	retry = FIMC_IS_MAX_HW_FRAME;
	while (frame && retry--) {
		if (done_type == IS_SHOT_TIMEOUT) {
			mserr_hw("[F:%d]hardware is timeout", frame->instance, hw_ip, frame->fcount);
			fimc_is_hardware_size_dump(hw_ip);
		}

		ret = fimc_is_hardware_frame_ndone(hw_ip, frame, atomic_read(&hw_ip->instance), done_type);
		if (ret)
			mserr_hw("%s: hardware_frame_ndone fail",
				atomic_read(&hw_ip->instance), hw_ip, __func__);

		framemgr_e_barrier_irqs(framemgr, 0, flags);
		frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
		framemgr_x_barrier_irqr(framemgr, 0, flags);
	}

	if (retry == 0)
		err_hw("frame flush is not completed");
}

u32 get_group_id_from_hw_ip(u32 hw_id)
{
	u32 group_id = GROUP_ID_MAX;

	switch(hw_id) {
	case DEV_HW_3AA0:
		group_id = GROUP_ID_3AA0;
		break;
	case DEV_HW_3AA1:
		group_id = GROUP_ID_3AA1;
		break;
	case DEV_HW_VPP:
		group_id = GROUP_ID_3AA2;
		break;
	case DEV_HW_ISP0:
		group_id = GROUP_ID_ISP0;
		break;
	case DEV_HW_ISP1:
		group_id = GROUP_ID_ISP1;
		break;
	case DEV_HW_TPU0:
		group_id = GROUP_ID_DIS0;
		break;
	case DEV_HW_TPU1:
		group_id = GROUP_ID_DIS1;
		break;
	case DEV_HW_MCSC0:
		group_id = GROUP_ID_MCS0;
		break;
	case DEV_HW_MCSC1:
		group_id = GROUP_ID_MCS1;
		break;
	case DEV_HW_VRA:
		group_id = GROUP_ID_VRA0;
		break;
	case DEV_HW_DCP:
		group_id = GROUP_ID_DCP;
		break;
	case DEV_HW_PAF0:
		group_id = GROUP_ID_PAF0;
		break;
	case DEV_HW_PAF1:
		group_id = GROUP_ID_PAF1;
		break;
	default:
		group_id = GROUP_ID_MAX;
		err_hw("invalid hw_id(%d)", hw_id);
		break;
	}

	return group_id;
}

u32 get_hw_id_from_group(u32 group_id)
{
	u32 hw_id = DEV_HW_END;

	switch(group_id) {
	case GROUP_ID_3AA0:
		hw_id = DEV_HW_3AA0;
		break;
	case GROUP_ID_3AA1:
		hw_id = DEV_HW_3AA1;
		break;
	case GROUP_ID_3AA2:
		hw_id = DEV_HW_VPP;
		break;
	case GROUP_ID_ISP0:
		hw_id = DEV_HW_ISP0;
		break;
	case GROUP_ID_ISP1:
		hw_id = DEV_HW_ISP1;
		break;
	case GROUP_ID_DIS0:
		hw_id = DEV_HW_TPU0;
		break;
	case GROUP_ID_DIS1:
		hw_id = DEV_HW_TPU1;
		break;
	case GROUP_ID_DCP:
		hw_id = DEV_HW_DCP;
		break;
	case GROUP_ID_MCS0:
		hw_id = DEV_HW_MCSC0;
		break;
	case GROUP_ID_MCS1:
		hw_id = DEV_HW_MCSC1;
		break;
	case GROUP_ID_VRA0:
		hw_id = DEV_HW_VRA;
		break;
	case GROUP_ID_PAF0:
		hw_id = DEV_HW_PAF0;
		break;
	case GROUP_ID_PAF1:
		hw_id = DEV_HW_PAF1;
		break;
	default:
		hw_id = DEV_HW_END;
		err_hw("invalid group(%d)", group_id);
		break;
	}

	return hw_id;
}

static inline void fimc_is_hardware_fill_frame_info(u32 instance,
	struct fimc_is_frame *hw_frame,
	struct fimc_is_frame *frame)
{
	hw_frame->groupmgr	= frame->groupmgr;
	hw_frame->group		= frame->group;
	hw_frame->shot_ext	= frame->shot_ext;
	hw_frame->shot		= frame->shot;
	hw_frame->shot_size	= frame->shot_size;
	hw_frame->fcount	= frame->fcount;
	hw_frame->rcount	= frame->rcount;
	hw_frame->bak_flag      = GET_OUT_FLAG_IN_DEVICE(FIMC_IS_DEVICE_ISCHAIN, frame->bak_flag);
	hw_frame->out_flag      = GET_OUT_FLAG_IN_DEVICE(FIMC_IS_DEVICE_ISCHAIN, frame->out_flag);
	hw_frame->core_flag	= 0;
	atomic_set(&hw_frame->shot_done_flag, 1);

	memcpy(hw_frame->dvaddr_buffer, frame->dvaddr_buffer, sizeof(frame->dvaddr_buffer));
	memcpy(hw_frame->kvaddr_buffer, frame->kvaddr_buffer, sizeof(frame->kvaddr_buffer));
	memcpy(hw_frame->txcTargetAddress, frame->txcTargetAddress, sizeof(frame->txcTargetAddress));
	memcpy(hw_frame->txpTargetAddress, frame->txpTargetAddress, sizeof(frame->txpTargetAddress));
	memcpy(hw_frame->mrgTargetAddress, frame->mrgTargetAddress, sizeof(frame->mrgTargetAddress));
	memcpy(hw_frame->efdTargetAddress, frame->efdTargetAddress, sizeof(frame->efdTargetAddress));
	memcpy(hw_frame->ixcTargetAddress, frame->ixcTargetAddress, sizeof(frame->ixcTargetAddress));
	memcpy(hw_frame->ixpTargetAddress, frame->ixpTargetAddress, sizeof(frame->ixpTargetAddress));
	memcpy(hw_frame->mexcTargetAddress, frame->mexcTargetAddress, sizeof(frame->mexcTargetAddress));
	memcpy(hw_frame->sccTargetAddress, frame->sccTargetAddress, sizeof(frame->sccTargetAddress));
	memcpy(hw_frame->scpTargetAddress, frame->scpTargetAddress, sizeof(frame->scpTargetAddress));
	memcpy(hw_frame->sc0TargetAddress, frame->sc0TargetAddress, sizeof(frame->sc0TargetAddress));
	memcpy(hw_frame->sc1TargetAddress, frame->sc1TargetAddress, sizeof(frame->sc1TargetAddress));
	memcpy(hw_frame->sc2TargetAddress, frame->sc2TargetAddress, sizeof(frame->sc2TargetAddress));
	memcpy(hw_frame->sc3TargetAddress, frame->sc3TargetAddress, sizeof(frame->sc3TargetAddress));
	memcpy(hw_frame->sc4TargetAddress, frame->sc4TargetAddress, sizeof(frame->sc4TargetAddress));
	memcpy(hw_frame->sc5TargetAddress, frame->sc5TargetAddress, sizeof(frame->sc5TargetAddress));
	memcpy(hw_frame->dxcTargetAddress, frame->dxcTargetAddress, sizeof(frame->dxcTargetAddress));

	hw_frame->instance = instance;
}

static inline void mshot_schedule(struct fimc_is_hw_ip *hw_ip)
{
#if defined(MULTI_SHOT_TASKLET)
	tasklet_schedule(&hw_ip->tasklet_mshot);
#elif defined(MULTI_SHOT_KTHREAD)
	kthread_queue_work(&hw_ip->mshot_worker, &hw_ip->mshot_work);
#endif
}

#if defined(MULTI_SHOT_TASKLET)
static void tasklet_mshot(unsigned long data)
{
	u32 instance;
	int ret = 0;
	ulong hw_map;
	ulong flags = 0;
	struct fimc_is_hw_ip *hw_ip;
	struct fimc_is_hardware *hardware;
	struct fimc_is_frame *frame;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_group *group;

	hw_ip = (struct fimc_is_hw_ip *)data;
	if (!hw_ip) {
		err("hw_ip is NULL");
		BUG();
	}

	hardware = hw_ip->hardware;
	instance = atomic_read(&hw_ip->instance);
	group = hw_ip->group[instance];
	framemgr = hw_ip->framemgr;
	framemgr_e_barrier_irqs(framemgr, 0, flags);
	frame = get_frame(framemgr, FS_HW_REQUEST);
	framemgr_x_barrier_irqr(framemgr, 0, flags);
	hw_map = hardware->hw_map[instance];

	if (!frame) {
		serr_hw("shot frame is empty [G:0x%x]", hw_ip, GROUP_ID(group->id));
		return;
	}

	msdbgs_hw(2, "[F:%d]%s\n", instance, hw_ip, frame->fcount, __func__);

	flags = 0;
	local_irq_save(flags);
	fimc_is_enter_lib_isr();
	ret = fimc_is_hardware_shot(hardware, instance, group, frame, framemgr,
			hw_map, frame->fcount + frame->cur_buf_index);
	if (ret) {
		serr_hw("hardware_shot fail [G:0x%x]", hw_ip, GROUP_ID(group->id));
	}
	fimc_is_exit_lib_isr();
	local_irq_restore(flags);
}
#elif defined(MULTI_SHOT_KTHREAD)
void fimc_is_hardware_mshot_work_fn(struct kthread_work *work)
{
	u32 instance;
	int ret = 0;
	ulong hw_map;
	ulong flags = 0;
	struct fimc_is_hw_ip *hw_ip;
	struct fimc_is_hardware *hardware;
	struct fimc_is_frame *frame;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_group *group;

	hw_ip = container_of(work, struct fimc_is_hw_ip, mshot_work);

	hardware = hw_ip->hardware;
	instance = atomic_read(&hw_ip->instance);
	group = hw_ip->group[instance];
	framemgr = hw_ip->framemgr;
	framemgr_e_barrier_irqs(framemgr, 0, flags);
	frame = get_frame(framemgr, FS_HW_REQUEST);
	framemgr_x_barrier_irqr(framemgr, 0, flags);

	hw_map = hardware->hw_map[instance];

	if (!frame) {
		serr_hw("frame is empty", hw_ip);
	}

	msdbgs_hw(2, "[F:%d]%s\n", instance, hw_ip, frame->fcount, __func__);

	ret = fimc_is_hardware_shot(hardware, instance, group, frame, framemgr,
			hw_map, frame->fcount + frame->cur_buf_index);
	if (ret) {
		serr_hw("hardware_shot fail [G:0x%x]", hw_ip, GROUP_ID(group->id));
	}
}

static int fimc_is_hardware_init_mshot_thread(struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;
	struct sched_param param = {.sched_priority = TASK_MSHOT_WORK_PRIO};

	if (hw_ip->mshot_task == NULL) {
		kthread_init_worker(&hw_ip->mshot_worker);
		hw_ip->mshot_task = kthread_run(kthread_worker_fn,
						&hw_ip->mshot_worker,
						"mshot_work");
		if (IS_ERR_OR_NULL(hw_ip->mshot_task)) {
			err("failed to create kthread");
			hw_ip->mshot_task = NULL;
			return -EFAULT;
		}

		ret = sched_setscheduler_nocheck(hw_ip->mshot_task, SCHED_FIFO, &param);
		if (ret) {
			err("sched_setscheduler_nocheck is fail(%d)", ret);
			return ret;
		}

		kthread_init_work(&hw_ip->mshot_work, fimc_is_hardware_mshot_work_fn);
	}

	return ret;
}

void fimc_is_hardware_deinit_mshot_thread(struct fimc_is_hw_ip *hw_ip)
{
	if (hw_ip->mshot_task != NULL) {
		if (kthread_stop(hw_ip->mshot_task))
			err("kthread_stop fail");

		hw_ip->mshot_task = NULL;
	}
}
#endif

int fimc_is_hardware_probe(struct fimc_is_hardware *hardware,
	struct fimc_is_interface *itf, struct fimc_is_interface_ischain *itfc)
{
	int ret = 0;
	int i, hw_slot = -1;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;

	FIMC_BUG(!hardware);
	FIMC_BUG(!itf);
	FIMC_BUG(!itfc);

	for (i = 0; i < HW_SLOT_MAX; i++) {
		hardware->hw_ip[i].id = DEV_HW_END;
		hardware->hw_ip[i].priv_info = NULL;

	}

#if defined(SOC_PAF0)
	hw_id = DEV_HW_PAF0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_paf_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "PAF0");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if defined(SOC_PAF1)
	hw_id = DEV_HW_PAF1;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_paf_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "PAF1");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if defined(SOC_3AAISP)
	hw_id = DEV_HW_3AA0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_3aa_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "3AA0");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if (defined(SOC_30S) && !defined(SOC_3AAISP))
	hw_id = DEV_HW_3AA0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_3aa_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "3AA0");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if (defined(SOC_31S) && !defined(SOC_3AAISP))
	hw_id = DEV_HW_3AA1;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_3aa_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "3AA1");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if (defined(SOC_32S) && !defined(SOC_3AAISP))
	hw_id = DEV_HW_VPP;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_3aa_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "VPP");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if (defined(SOC_I0S) && !defined(SOC_3AAISP))
	hw_id = DEV_HW_ISP0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_isp_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "ISP0");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if (defined(SOC_I1S) && !defined(SOC_3AAISP))
	hw_id = DEV_HW_ISP1;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_isp_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "ISP1");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if defined(SOC_DIS) || defined(SOC_D0S)
	hw_id = DEV_HW_TPU0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_tpu_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "TPU0");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if defined(SOC_D1S)
	hw_id = DEV_HW_TPU1;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_tpu_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "TPU1");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if (defined(SOC_SCP) && !defined(MCS_USE_SCP_PARAM))
	hw_id = DEV_HW_SCP;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}

	ret = fimc_is_hw_scp_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "SCP");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if (defined(SOC_SCP) && defined(MCS_USE_SCP_PARAM))
	hw_id = DEV_HW_MCSC0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}

	ret = fimc_is_hw_mcsc_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "SCP");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if (defined(SOC_MCS) && defined(SOC_MCS0))
	hw_id = DEV_HW_MCSC0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_mcsc_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "MCS0");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if (defined(SOC_MCS) && defined(SOC_MCS1))
	hw_id = DEV_HW_MCSC1;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_mcsc_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "MCS1");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if defined(SOC_VRA) && defined(ENABLE_VRA)
	hw_id = DEV_HW_VRA;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_vra_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "VRA");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif

#if defined(SOC_DCP) && defined(CONFIG_DCP_V1_0)
	hw_id = DEV_HW_DCP;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
	ret = fimc_is_hw_dcp_probe(&(hardware->hw_ip[hw_slot]), itf, itfc, hw_id, "DCP");
	if (ret) {
		err_hw("probe fail (%d,%d)", hw_id, hw_slot);
		return ret;
	}
#endif
	hardware->base_addr_mcuctl = itfc->regs_mcuctl;

	for (i = 0; i < FIMC_IS_STREAM_COUNT; i++) {
		hardware->hw_map[i] = 0;
		hardware->sensor_position[i] = 0;
	}

	for (i = 0; i < SENSOR_POSITION_MAX; i++)
		atomic_set(&hardware->streaming[i], 0);

	atomic_set(&hardware->rsccount, 0);
	atomic_set(&hardware->bug_count, 0);
	atomic_set(&hardware->log_count, 0);
	hardware->video_mode = false;
	clear_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag);

	prepare_sfr_dump(hardware);

	return ret;
}

int fimc_is_hardware_set_param(struct fimc_is_hardware *hardware, u32 instance,
	struct is_region *region, u32 lindex, u32 hindex, ulong hw_map)
{
	int ret = 0;
	int hw_slot = -1;
	struct fimc_is_hw_ip *hw_ip = NULL;

	FIMC_BUG(!hardware);
	FIMC_BUG(!region);

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];

		if (!test_bit_variables(hw_ip->id, &hw_map))
			continue;

		CALL_HW_OPS(hw_ip, clk_gate, instance, true, false);
		ret = CALL_HW_OPS(hw_ip, set_param, region, lindex, hindex,
				instance, hw_map);
		CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);
		if (ret) {
			mserr_hw("set_param fail (%d)", instance, hw_ip, hw_slot);
			return -EINVAL;
		}
	}

	msdbg_hw(1, "set_param hw_map[0x%lx]\n", instance, hw_ip, hw_map);

	return ret;
}

static void fimc_is_hardware_shot_timer(unsigned long data)
{
	struct fimc_is_hw_ip *hw_ip = (struct fimc_is_hw_ip *)data;

	FIMC_BUG_VOID(!hw_ip);

	fimc_is_hardware_flush_frame(hw_ip, FS_HW_REQUEST, IS_SHOT_TIMEOUT);
}

int fimc_is_hardware_shot(struct fimc_is_hardware *hardware, u32 instance,
	struct fimc_is_group *group, struct fimc_is_frame *frame,
	struct fimc_is_framemgr *framemgr, ulong hw_map, u32 framenum)
{
	int ret = 0;
	struct fimc_is_hw_ip *hw_ip = NULL;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	struct fimc_is_group *child = NULL;
	ulong flags = 0;
	int hw_list[GROUP_HW_MAX], hw_index, hw_slot;
	int hw_maxnum = 0;
	u32 index;

	FIMC_BUG(!hardware);
	FIMC_BUG(!frame);

	/* do the other device's group shot */
	ret = fimc_is_devicemgr_shot_callback(group, frame, frame->fcount, FIMC_IS_DEVICE_ISCHAIN);
	if (ret) {
		err_hw("fimc_is_devicemgr_shot_callback fail[F:%d].", frame->fcount);
		return -EINVAL;
	}

	framemgr_e_barrier_common(framemgr, 0, flags);
	put_frame(framemgr, frame, FS_HW_CONFIGURE);
	framemgr_x_barrier_common(framemgr, 0, flags);

	child = group->tail;

	if ((group->id == GROUP_ID_3AA0 || group->id == GROUP_ID_3AA1)
		&& !test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state))
		fimc_is_hw_mcsc_set_size_for_uvsp(hardware, frame, hw_map);

	while (child && (child->device_type == FIMC_IS_DEVICE_ISCHAIN)) {
		hw_maxnum = fimc_is_get_hw_list(child->id, hw_list);
		for (hw_index = hw_maxnum - 1; hw_index >= 0; hw_index--) {
			hw_id = hw_list[hw_index];
			hw_slot = fimc_is_hw_slot_id(hw_id);
			if (!valid_hw_slot_id(hw_slot)) {
				merr_hw("invalid slot (%d,%d)", instance,
					hw_id, hw_slot);
				return -EINVAL;
			}

			hw_ip = &hardware->hw_ip[hw_slot];
			/* hw_ip->fcount : frame number of current frame in Vvalid  @ OTF *
			 * hw_ip->fcount is the frame number of next FRAME END interrupt  *
			 * In OTF scenario, hw_ip->fcount is not same as frame->fcount    */
			atomic_set(&hw_ip->fcount, framenum);
			atomic_set(&hw_ip->instance, instance);

			hw_ip->framemgr = &hardware->framemgr[group->id];
			hw_ip->framemgr_late = &hardware->framemgr_late[group->id];

			if (hw_ip->id != DEV_HW_VRA)
				CALL_HW_OPS(hw_ip, clk_gate, instance, true, false);

			index = frame->fcount % DEBUG_FRAME_COUNT;
			hw_ip->debug_index[0] = frame->fcount;
			hw_ip->debug_info[index].cpuid[DEBUG_POINT_HW_SHOT] = raw_smp_processor_id();
			hw_ip->debug_info[index].time[DEBUG_POINT_HW_SHOT] = local_clock();
			ret = CALL_HW_OPS(hw_ip, shot, frame, hw_map);
			if (ret) {
				mserr_hw("shot fail (%d)[F:%d]", instance, hw_ip,
					hw_slot, frame->fcount);
				return -EINVAL;
			}
		}
		child = child->parent;
	}

#ifdef DBG_HW
	if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]])
		&& (hw_ip && atomic_read(&hw_ip->status.otf_start)))
		msinfo_hw("shot [F:%d][G:0x%x][B:0x%lx][O:0x%lx][C:0x%lx][HWF:%d]\n",
			instance, hw_ip,
			frame->fcount, GROUP_ID(group->id),
			frame->bak_flag, frame->out_flag, frame->core_flag, framenum);
#endif

	return ret;
}

int fimc_is_hardware_get_meta(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	u32 instance, ulong hw_map, u32 output_id, enum ShotErrorType done_type)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (((output_id != FIMC_IS_HW_CORE_END) && (done_type == IS_SHOT_SUCCESS)
			&& (test_bit(hw_ip->id, &frame->core_flag)))
		|| (done_type != IS_SHOT_SUCCESS)) {
		/* FIMC-IS v3.x only
		 * There is a chance that the DMA done interrupt occurred before
		 * the core done interrupt. So we skip to call the get_meta function.
		 */
		/* There is no need to call get_meta function in case of NDONE */
		msdbg_hw(1, "%s: skip to get_meta [F:%d][B:0x%lx][C:0x%lx][O:0x%lx]\n",
			instance, hw_ip, __func__, frame->fcount,
			frame->bak_flag, frame->core_flag, frame->out_flag);
		return 0;
	}

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
	case DEV_HW_3AA1:
		copy_ctrl_to_dm(frame->shot);
	case DEV_HW_ISP0:
	case DEV_HW_ISP1:
		ret = CALL_HW_OPS(hw_ip, get_meta, frame, hw_map);
		if (ret) {
			mserr_hw("[F:%d] get_meta fail", instance, hw_ip, frame->fcount);
			return 0;
		}
		if (hw_ip->id == DEV_HW_3AA0 || hw_ip->id == DEV_HW_3AA1)
			fimc_is_hw_mcsc_set_ni(hw_ip->hardware, frame, instance);

		break;
	case DEV_HW_TPU0:
	case DEV_HW_TPU1:
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
	case DEV_HW_VRA:
		ret = CALL_HW_OPS(hw_ip, get_meta, frame, hw_map);
		if (ret) {
			mserr_hw("[F:%d] get_meta fail", instance, hw_ip, frame->fcount);
			return 0;
		}
		break;
	default:
		return 0;
		break;
	}

	msdbg_hw(1, "[F:%d]get_meta [G:0x%x]\n", instance, hw_ip,
		frame->fcount, GROUP_ID(hw_ip->group[instance]->id));

	return ret;
}

int check_shot_exist(struct fimc_is_framemgr *framemgr, u32 fcount)
{
	struct fimc_is_frame *frame;

	if (framemgr->queued_count[FS_HW_WAIT_DONE]) {
		frame = find_frame(framemgr, FS_HW_WAIT_DONE, frame_fcount,
					(void *)(ulong)fcount);
		if (frame) {
			info_hw("[F:%d]is in complete_list\n", fcount);
			return INTERNAL_SHOT_EXIST;
		}
	}

	if (framemgr->queued_count[FS_HW_CONFIGURE]) {
		frame = find_frame(framemgr, FS_HW_CONFIGURE, frame_fcount,
					(void *)(ulong)fcount);
		if (frame) {
			info_hw("[F:%d]is in process_list\n", fcount);
			return INTERNAL_SHOT_EXIST;
		}
	}

	return 0;
}

void fimc_is_set_hw_count(struct fimc_is_hardware *hardware, struct fimc_is_group *head,
	u32 instance, u32 fcount, u32 num_buffers, ulong hw_map)
{
	struct fimc_is_group *child;
	struct fimc_is_hw_ip *hw_ip = NULL;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	int hw_list[GROUP_HW_MAX], hw_index, hw_slot;
	int hw_maxnum = 0;
	u32 fs, cl, fe, dma;

	child = head->tail;
	while (child && (child->device_type == FIMC_IS_DEVICE_ISCHAIN)) {
		hw_maxnum = fimc_is_get_hw_list(child->id, hw_list);
		for (hw_index = hw_maxnum - 1; hw_index >= 0; hw_index--) {
			hw_id = hw_list[hw_index];
			hw_slot = fimc_is_hw_slot_id(hw_id);
			if (!valid_hw_slot_id(hw_slot)) {
				merr_hw("invalid slot (%d,%d)", instance,
						hw_id, hw_slot);
				continue;
			}

			hw_ip = &hardware->hw_ip[hw_slot];
			if (test_bit_variables(hw_ip->id, &hw_map)) {
				fs = atomic_read(&hw_ip->count.fs);
				cl = atomic_read(&hw_ip->count.cl);
				fe = atomic_read(&hw_ip->count.fe);
				dma = atomic_read(&hw_ip->count.dma);
				atomic_set(&hw_ip->count.fs, (fcount - num_buffers));
				atomic_set(&hw_ip->count.cl, (fcount - num_buffers));
				atomic_set(&hw_ip->count.fe, (fcount - num_buffers));
				atomic_set(&hw_ip->count.dma, (fcount - num_buffers));
				msdbg_hw(1, "[F:%d]count clear, fs(%d->%d), fe(%d->%d), dma(%d->%d)\n",
						instance, hw_ip, fcount,
						fs, atomic_read(&hw_ip->count.fs),
						fe, atomic_read(&hw_ip->count.fe),
						dma, atomic_read(&hw_ip->count.dma));
			}
		}
		child = child->parent;
	}
}

int fimc_is_hardware_grp_shot(struct fimc_is_hardware *hardware, u32 instance,
	struct fimc_is_group *group, struct fimc_is_frame *frame, ulong hw_map)
{
	int ret = 0;
	int hw_slot = -1;
	struct fimc_is_hw_ip *hw_ip = NULL;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	struct fimc_is_frame *hw_frame;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_group *head;
	ulong flags = 0;
	u32 shot_timeout = 0;
#if defined(MULTI_SHOT_KTHREAD) || defined(MULTI_SHOT_TASKLET)
	int i;
#endif

	FIMC_BUG(!hardware);
	FIMC_BUG(!frame);

	head = GET_HEAD_GROUP_IN_DEVICE(FIMC_IS_DEVICE_ISCHAIN, group);

	hw_id = get_hw_id_from_group(head->id);
	if (hw_id == DEV_HW_END)
		return -EINVAL;

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		merr_hw("invalid slot (%d,%d)", instance, hw_id, hw_slot);
		return -EINVAL;
	}

	hw_ip = &hardware->hw_ip[hw_slot];

	hw_ip->framemgr = &hardware->framemgr[head->id];
	hw_ip->framemgr_late = &hardware->framemgr_late[head->id];

#ifdef ENABLE_HW_FAST_READ_OUT
	if (frame->num_buffers > 1)
		hardware->hw_fro_en = true;
	mdbg_hw(2, "ischain hw FRO EN(%d)\n", instance, hardware->hw_fro_en);
#endif

	if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
		msinfo_hw("grp_shot [F:%d][G:0x%x][B:0x%lx][O:0x%lx][dva:%pad]\n",
			instance, hw_ip,
			frame->fcount, GROUP_ID(head->id),
			frame->bak_flag, frame->out_flag, &frame->dvaddr_buffer[0]);

	framemgr = hw_ip->framemgr;
	framemgr_e_barrier_irqs(framemgr, 0, flags);
	ret = check_shot_exist(framemgr, frame->fcount);

	/* check late shot */
	if ((hw_ip->internal_fcount[instance] >= frame->fcount || ret == INTERNAL_SHOT_EXIST)
		&& test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state)) {
		msinfo_hw("LATE_SHOT (%d)[F:%d][G:0x%x][B:0x%lx][O:0x%lx][C:0x%lx]\n",
			instance, hw_ip,
			hw_ip->internal_fcount[instance], frame->fcount, GROUP_ID(head->id),
			frame->bak_flag, frame->out_flag, frame->core_flag);
		frame->type = SHOT_TYPE_LATE;
		/* unlock previous framemgr */
		framemgr_x_barrier_irqr(framemgr, 0, flags);
		framemgr = hw_ip->framemgr_late;
		/* lock by late framemgr */
		framemgr_e_barrier_irqs(framemgr, 0, flags);
		if (framemgr->queued_count[FS_HW_REQUEST] > 0) {
			mswarn_hw("LATE_SHOT REQ(%d) > 0, PRO(%d)",
				instance, hw_ip,
				framemgr->queued_count[FS_HW_REQUEST],
				framemgr->queued_count[FS_HW_CONFIGURE]);
		}

		/* set lindex, findex
		 * for after ndone -> param force set
		 */
		frame->lindex = 0xffff;
		frame->hindex = 0xffff;

		ret = 0;
	} else {
		frame->type = SHOT_TYPE_EXTERNAL;
	}

	hw_frame = get_frame(framemgr, FS_HW_FREE);
	if (hw_frame == NULL) {
		framemgr_x_barrier_irqr(framemgr, 0, flags);
		merr_hw("[G:0x%x]free_head(NULL)", instance, GROUP_ID(head->id));
		return -EINVAL;
	}

	fimc_is_hardware_fill_frame_info(instance, hw_frame, frame);
	hw_frame->type = frame->type;

	/* multi-buffer */
	hw_frame->planes	= frame->planes;
	hw_frame->num_buffers	= frame->num_buffers;
	hw_frame->cur_buf_index	= 0;
	hardware->recovery_numbuffers = frame->num_buffers;

	/* for NI (noise index) */
	hw_frame->noise_idx = frame->noise_idx;

	/* shot timer set */
	shot_timeout = head->device->resourcemgr->shot_timeout;

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state)) {
		if (!atomic_read(&hw_ip->status.otf_start)) {
			atomic_set(&hw_ip->status.otf_start, 1);
			msinfo_hw("OTF start [F:%d][G:0x%x][B:0x%lx][O:0x%lx]\n",
				instance, hw_ip,
				hw_frame->fcount, GROUP_ID(head->id),
				hw_frame->bak_flag, hw_frame->out_flag);

			if (frame->type == SHOT_TYPE_LATE) {
				put_frame(framemgr, hw_frame, FS_HW_REQUEST);
				framemgr_x_barrier_irqr(framemgr, 0, flags);
				return ret;
			}
		} else {
			atomic_set(&hw_ip->hardware->log_count, 0);
			put_frame(framemgr, hw_frame, FS_HW_REQUEST);
			framemgr_x_barrier_irqr(framemgr, 0, flags);

			mod_timer(&hw_ip->shot_timer, jiffies + msecs_to_jiffies(shot_timeout));

			return ret;
		}
	} else {
#if defined(MULTI_SHOT_KTHREAD) || defined(MULTI_SHOT_TASKLET)
		put_frame(framemgr, hw_frame, FS_HW_REQUEST);

		if (frame->num_buffers > 1) {
			hw_frame->type = SHOT_TYPE_MULTI;
			hw_frame->planes = 1;
			hw_frame->num_buffers = 1;
			hw_frame->cur_buf_index	= 0;

			for (i = 2; i < frame->num_buffers; i++) {
				hw_frame = get_frame(framemgr, FS_HW_FREE);
				if (hw_frame == NULL) {
					framemgr_x_barrier_irqr(framemgr, 0, flags);
					err_hw("[F%d]free_head(NULL)", frame->fcount);
					return -EINVAL;
				}

				fimc_is_hardware_fill_frame_info(instance, hw_frame, frame);
				hw_frame->type = SHOT_TYPE_MULTI;
				hw_frame->planes = 1;
				hw_frame->num_buffers = 1;
				hw_frame->cur_buf_index = i - 1;

				put_frame(framemgr, hw_frame, FS_HW_REQUEST);
			}
			hw_frame->type = frame->type; /* last buffer */
		}

		hw_frame = get_frame(framemgr, FS_HW_REQUEST);
		if (hw_frame == NULL) {
			framemgr_x_barrier_irqr(framemgr, 0, flags);
			mserr_hw("[F%d][G:0x%x]req_head(NULL)",
				instance, hw_ip, frame->fcount, GROUP_ID(head->id));
			return -EINVAL;
		}
#endif
		mod_timer(&hw_ip->shot_timer, jiffies + msecs_to_jiffies(shot_timeout));
	}

	framemgr_x_barrier_irqr(framemgr, 0, flags);

	fimc_is_set_hw_count(hardware, head, instance, frame->fcount, frame->num_buffers, hw_map);
	ret = fimc_is_hardware_shot(hardware, instance, head, hw_frame, framemgr,
			hw_map, frame->fcount);
	if (ret) {
		mserr_hw("hardware_shot fail [G:0x%x]", instance, hw_ip, GROUP_ID(head->id));
		return -EINVAL;
	}

	return ret;
}

int make_internal_shot(struct fimc_is_hw_ip *hw_ip, u32 instance, u32 fcount,
	struct fimc_is_frame **in_frame, struct fimc_is_framemgr *framemgr)
{
	int ret = 0;
	int i = 0;
	struct fimc_is_frame *frame;
	u32 shot_timeout;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!framemgr);

	if (framemgr->queued_count[FS_HW_FREE] < 3) {
		mswarn_hw("Free frame is less than 3", instance, hw_ip);
		check_hw_bug_count(hw_ip->hardware, 10);
	}

	ret = check_shot_exist(framemgr, fcount);
	if (ret == INTERNAL_SHOT_EXIST)
		return ret;

	frame = get_frame(framemgr, FS_HW_FREE);
	if (frame == NULL) {
		merr_hw("config_lock: frame(null)", instance);
		frame_manager_print_info_queues(framemgr);
		return -EINVAL;
	}
	frame->groupmgr		= NULL;
	frame->group		= NULL;
	frame->shot_ext		= NULL;
	frame->shot		= NULL;
	frame->shot_size	= 0;
	frame->fcount		= fcount;
	frame->rcount		= 0;
	frame->bak_flag		= 0;
	frame->out_flag		= 0;
	frame->core_flag	= 0;
	atomic_set(&frame->shot_done_flag, 1);
	/* multi-buffer */
	frame->planes		= 0;
	if (hw_ip->hardware->hw_fro_en)
		frame->num_buffers = hw_ip->num_buffers;
	else
		frame->num_buffers = 1;

	for (i = 0; i < FIMC_IS_MAX_PLANES; i++)
		frame->dvaddr_buffer[i]	= 0;

	frame->type = SHOT_TYPE_INTERNAL;
	frame->instance = instance;
	*in_frame = frame;

	shot_timeout = hw_ip->group[instance]->device->resourcemgr->shot_timeout;
	mod_timer(&hw_ip->shot_timer, jiffies + msecs_to_jiffies(shot_timeout));

	return ret;
}

int fimc_is_hardware_config_lock(struct fimc_is_hw_ip *hw_ip, u32 instance, u32 framenum)
{
	int ret = 0;
	struct fimc_is_frame *frame;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_hardware *hardware;
	struct fimc_is_device_sensor *sensor;
	u32 sensor_fcount;
	u32 fcount = framenum + 1;
	u32 log_count;
	bool req_int_shot = false;

	FIMC_BUG(!hw_ip);

	hardware = hw_ip->hardware;

	if (!test_bit(FIMC_IS_GROUP_OTF_INPUT, &hw_ip->group[instance]->head->state))
		return ret;

	msdbgs_hw(2, "[F:%d]C.L\n", instance, hw_ip, framenum);

	sensor = hw_ip->group[instance]->device->sensor;
	sensor_fcount = sensor->fcount;
	if (framenum < sensor_fcount) {
		mswarn_hw("fcount mismatch(hw(%d), sen(%d))\n", instance, hw_ip, framenum, sensor_fcount);
		framenum = sensor_fcount;
		fcount = framenum + 1;
		atomic_set(&hw_ip->count.fs, sensor_fcount);
	}

	framemgr = hw_ip->framemgr;

retry_get_frame:
	framemgr_e_barrier(framemgr, 0);

	/*
	 * Check fcount of the requested frame before handling it.
	 * The fcount of next shot,
	 * which is going to be delivered to HW within N config_lock,
	 * must be N+1 to guarantee the fcount synchronization with sensor device.
	 */
	frame = peek_frame(framemgr, FS_HW_REQUEST);
	if (!IS_ERR_OR_NULL(frame)) {
		if (unlikely(frame->fcount < fcount)) {
			/* Fcount of the requested frame is too old. Flush it. */
			trans_frame(framemgr, frame, FS_HW_WAIT_DONE);

			framemgr_x_barrier(framemgr, 0);

			if (fimc_is_hardware_frame_ndone(hw_ip, frame, instance,
						IS_SHOT_UNPROCESSED)) {
				err_hw("[F:%d][HWID:%d]: failure in hardware_frame_ndone",
					frame->instance, hw_ip->id);
			}

			goto retry_get_frame;
		} else if (unlikely(frame->fcount > fcount)) {
			/* Fcount of the requested frame is too fast. Skip it. */
			req_int_shot = true;
		} else {
			/* It is the expected fcount. Use it. */
			frame = get_frame(framemgr, FS_HW_REQUEST);
		}
	} else {
		req_int_shot = true;
	}

	if (req_int_shot) {
		ret = make_internal_shot(hw_ip, instance, fcount, &frame, framemgr);
		if (ret == INTERNAL_SHOT_EXIST) {
			framemgr_x_barrier(framemgr, 0);
			return ret;
		}
		if (ret) {
			framemgr_x_barrier(framemgr, 0);
			print_all_hw_frame_count(hardware);
			FIMC_BUG(1);
		}
		log_count = atomic_read(&hardware->log_count);
		if (!IS_ERR_OR_NULL(frame) && ((log_count <= 20) || !(log_count % 100)))
			msinfo_hw("config_lock: INTERNAL_SHOT [F:%d](%d) count(%d)\n",
				instance, hw_ip,
				fcount, frame->index, log_count);
	}

	if (IS_ERR_OR_NULL(frame)) {
		mserr_hw("frame is null [G:0x%x]", instance, hw_ip,
			GROUP_ID(hw_ip->group[instance]->id));
		framemgr_x_barrier(framemgr, 0);
		return -EINVAL;
	}

	frame->frame_info[INFO_CONFIG_LOCK].cpu = raw_smp_processor_id();
	frame->frame_info[INFO_CONFIG_LOCK].pid = current->pid;
	frame->frame_info[INFO_CONFIG_LOCK].when = local_clock();

	framemgr_x_barrier(framemgr, 0);

	ret = fimc_is_hardware_shot(hardware, instance, hw_ip->group[instance],
			frame, framemgr, hardware->hw_map[instance], framenum);
	if (ret) {
		mserr_hw("hardware_shot fail [G:0x%x]", instance, hw_ip,
			GROUP_ID(hw_ip->group[instance]->id));
		return -EINVAL;
	}

	return ret;
}

void check_late_shot(struct fimc_is_hw_ip *hw_ip, u32 fcount)
{
	struct fimc_is_frame *frame;
	struct fimc_is_framemgr *framemgr = hw_ip->framemgr_late;

	framemgr_e_barrier(framemgr, 0);

	/*
	 * Handle LATE_FRAME error only for the frame that is being processed
	 * or has been processed.
	 */
	frame = peek_frame(framemgr, FS_HW_REQUEST);
	if (frame && frame->fcount <= fcount) {
		trans_frame(framemgr, frame, FS_HW_WAIT_DONE);

		framemgr_x_barrier(framemgr, 0);

		if (fimc_is_hardware_frame_ndone(hw_ip, frame, frame->instance,
					IS_SHOT_LATE_FRAME)) {
			mserr_hw("failure in hardware_frame_ndone",
					frame->instance, hw_ip);
		}
	} else {
		framemgr_x_barrier(framemgr, 0);
	}
}

void fimc_is_hardware_size_dump(struct fimc_is_hw_ip *hw_ip)
{
#if 0 /* carrotsm */
	int hw_slot = -1;
	struct fimc_is_hardware *hardware;
	u32 instance;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->hardware);

	instance = atomic_read(&hw_ip->instance);
	hardware = hw_ip->hardware;

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];
		if (hw_ip->ops->size_dump) {
			CALL_HW_OPS(hw_ip, clk_gate, instance, true, false);
			hw_ip->ops->size_dump(hw_ip);;
			CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);
		}
	}
#endif

	return;
}

void fimc_is_hardware_clk_gate_dump(struct fimc_is_hardware *hardware)
{
#ifdef ENABLE_DIRECT_CLOCK_GATE
	int hw_slot = -1;
	struct fimc_is_hw_ip *hw_ip;
	struct fimc_is_clk_gate *clk_gate;

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];
		if (hw_ip && hw_ip->clk_gate) {
			clk_gate = hw_ip->clk_gate;
			sinfo_hw("CLOCK_ENABLE(0x%08X) ref(%d)\n", hw_ip,
					__raw_readl(clk_gate->regs), clk_gate->refcnt[hw_slot]);

			/* do clock on for later other dump */
			FIMC_IS_CLOCK_ON(clk_gate->regs, clk_gate->bit[hw_ip->clk_gate_idx]);
		}
	}
#endif
	return;
}

void fimc_is_hardware_frame_start(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	struct fimc_is_frame *frame;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_group *head;

	FIMC_BUG_VOID(!hw_ip);
	head = GET_HEAD_GROUP_IN_DEVICE(FIMC_IS_DEVICE_ISCHAIN,
			hw_ip->group[instance]);
	FIMC_BUG_VOID(!head);

	/*
	 * If there are hw_ips having framestart processing
	 * and they are bound by OTF, the problem that same action was duplicated
	 * maybe happened.
	 * ex. 1) 3A0* => ISP* -> MCSC0* : no problem
	 *     2) 3A0* -> ISP  -> MCSC0* : problem happened!!
	 *      (* : called fimc_is_hardware_frame_start)
	 * Only leader group in OTF groups can control frame.
	 */
	if (hw_ip->group[instance]->id == head->id) {
		framemgr = hw_ip->framemgr;

		framemgr_e_barrier(framemgr, 0);
		frame = get_frame(framemgr, FS_HW_CONFIGURE);
		if (IS_ERR_OR_NULL(frame)) {
			/* error happened..print the frame info */
			frame_manager_print_info_queues(framemgr);
			print_all_hw_frame_count(hw_ip->hardware);
			framemgr_x_barrier(framemgr, 0);
			mserr_hw("FSTART frame null (%d) (%d != %d)", instance, hw_ip,
				hw_ip->internal_fcount[instance], hw_ip->group[instance]->id, head->id);
			return;
		}


#if defined(ENABLE_EARLY_SHOT)
		if (frame->type == SHOT_TYPE_MULTI)
			mshot_schedule(hw_ip);
#endif
		if (atomic_read(&hw_ip->status.otf_start)
				&& frame->fcount != atomic_read(&hw_ip->count.fs)) {
			/* error handling */
			info_hw("frame_start_isr (%d, %d)\n", frame->fcount,
					atomic_read(&hw_ip->count.fs));
			atomic_set(&hw_ip->count.fs, frame->fcount);
		}

		/* TODO: multi-instance */
		frame->frame_info[INFO_FRAME_START].cpu = raw_smp_processor_id();
		frame->frame_info[INFO_FRAME_START].pid = current->pid;
		frame->frame_info[INFO_FRAME_START].when = local_clock();

		put_frame(framemgr, frame, FS_HW_WAIT_DONE);
		framemgr_x_barrier(framemgr, 0);

		if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state))
			check_late_shot(hw_ip, frame->fcount);
	}

	clear_bit(HW_CONFIG, &hw_ip->state);
	atomic_set(&hw_ip->status.Vvalid, V_VALID);
}

int fimc_is_hardware_sensor_start(struct fimc_is_hardware *hardware, u32 instance,
	ulong hw_map)
{
	int ret = 0;
	int hw_slot = -1;
	struct fimc_is_hw_ip *hw_ip;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	u32 shot_timeout = 0;

	FIMC_BUG(!hardware);

	if (test_bit(DEV_HW_3AA0, &hw_map)) {
		hw_id = DEV_HW_3AA0;
	} else if (test_bit(DEV_HW_3AA1, &hw_map)) {
		hw_id = DEV_HW_3AA1;
	} else {
		mwarn_hw("invalid state hw_map[0x%lx]\n", instance, hw_map);
		return 0;
	}

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		merr_hw("invalid slot (%d,%d)", instance, hw_id, hw_slot);
		return -EINVAL;
	}

	hw_ip = &hardware->hw_ip[hw_slot];

	ret = fimc_is_hw_3aa_mode_change(hw_ip, instance, hw_map);
	if (ret) {
		mserr_hw("mode_change fail (%d)", instance, hw_ip, hw_slot);
		return -EINVAL;
	}

	if (atomic_read(&hw_ip->status.otf_start)) {
		shot_timeout = hw_ip->group[instance]->device->resourcemgr->shot_timeout;
		mod_timer(&hw_ip->shot_timer, jiffies + msecs_to_jiffies(shot_timeout));
	}

	atomic_set(&hardware->streaming[hardware->sensor_position[instance]], 1);
	atomic_set(&hardware->bug_count, 0);
	atomic_set(&hardware->log_count, 0);
	clear_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag);

	msinfo_hw("hw_sensor_start [P:0x%lx]\n", instance, hw_ip, hw_map);

	return ret;
}

int fimc_is_hardware_sensor_stop(struct fimc_is_hardware *hardware, u32 instance,
	ulong hw_map)
{
	int ret = 0;
	int hw_slot = -1;
	int retry;
	struct fimc_is_frame *frame;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_group *group;
	struct fimc_is_hw_ip *hw_ip = NULL;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	ulong flags = 0;

	FIMC_BUG(!hardware);

	atomic_set(&hardware->streaming[hardware->sensor_position[instance]], 0);
	atomic_set(&hardware->bug_count, 0);
	atomic_set(&hardware->log_count, 0);
	clear_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag);

	if (test_bit(DEV_HW_3AA0, &hw_map)) {
		hw_id = DEV_HW_3AA0;
	} else if (test_bit(DEV_HW_3AA1, &hw_map)) {
		hw_id = DEV_HW_3AA1;
	} else {
		mwarn_hw("invalid state hw_map[0x%lx]\n", instance, hw_map);
		return 0;
	}

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		merr_hw("invalid slot (%d,%d)", instance,
			hw_id, hw_slot);
		return -EINVAL;
	}

	hw_ip = &hardware->hw_ip[hw_slot];
	group = hw_ip->group[instance];
	framemgr = hw_ip->framemgr;
	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state)) {
		retry = 99;
		while (--retry) {
			framemgr_e_barrier_irqs(framemgr, 0, flags);
			if (!framemgr->queued_count[FS_HW_WAIT_DONE]) {
				framemgr_x_barrier_irqr(framemgr, 0, flags);
				break;
			}

			frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
			if (frame == NULL) {
				framemgr_x_barrier_irqr(framemgr, 0, flags);
				break;
			}

			if (frame->num_buffers > 1) {
				retry = 0;
				framemgr_x_barrier_irqr(framemgr, 0, flags);
				break;
			}

			msinfo_hw("hw_sensor_stop: com_list: [F:%d][%d][O:0x%lx][C:0x%lx][(%d)",
				instance, hw_ip,
				frame->fcount, frame->type, frame->out_flag, frame->core_flag,
				framemgr->queued_count[FS_HW_WAIT_DONE]);
			mswarn_hw(" %d com waiting...", instance, hw_ip,
				framemgr->queued_count[FS_HW_WAIT_DONE]);

			framemgr_x_barrier_irqr(framemgr, 0, flags);
			usleep_range(1000, 1000);
		}

		if (!retry) {
			framemgr_e_barrier_irqs(framemgr, 0, flags);
			if (!framemgr->queued_count[FS_HW_WAIT_DONE]) {
				framemgr_x_barrier_irqr(framemgr, 0, flags);
				goto print_frame_info;
			}

			frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
			if (frame == NULL) {
				framemgr_x_barrier_irqr(framemgr, 0, flags);
				goto print_frame_info;
			}

			framemgr_x_barrier_irqr(framemgr, 0, flags);
			ret = fimc_is_hardware_frame_ndone(hw_ip, frame, instance, IS_SHOT_UNPROCESSED);
			if (ret)
				mserr_hw("hardware_frame_ndone fail", instance, hw_ip);
		}
print_frame_info:
		/* for last fcount */
		print_all_hw_frame_count(hardware);
	}

	msinfo_hw("hw_sensor_stop: done[P:0x%lx]\n", instance, hw_ip, hw_map);

	return ret;
}

int fimc_is_hardware_process_start(struct fimc_is_hardware *hardware, u32 instance,
	u32 group_id)
{
	int ret = 0;
	int hw_slot = -1;
	ulong hw_map;
	int hw_list[GROUP_HW_MAX];
	int hw_index, hw_maxnum;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	struct fimc_is_hw_ip *hw_ip = NULL;

	FIMC_BUG(!hardware);

	mdbg_hw(1, "process_start [G:0x%x]\n", instance, GROUP_ID(group_id));

	hw_map = hardware->hw_map[instance];
	hw_maxnum = fimc_is_get_hw_list(group_id, hw_list);
	for (hw_index = 0; hw_index < hw_maxnum; hw_index++) {
		hw_id = hw_list[hw_index];
		hw_slot = fimc_is_hw_slot_id(hw_id);
		if (!valid_hw_slot_id(hw_slot)) {
			merr_hw("invalid slot (%d,%d)", instance,
				hw_id, hw_slot);
			return -EINVAL;
		}

		hw_ip = &hardware->hw_ip[hw_slot];

		CALL_HW_OPS(hw_ip, clk_gate, instance, true, false);
		ret = CALL_HW_OPS(hw_ip, enable, instance, hw_map);
		CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);
		if (ret) {
			mserr_hw("enable fail (%d)", instance, hw_ip, hw_slot);
			return -EINVAL;
		}

		hw_ip->internal_fcount[instance] = 0;
	}

	return ret;
}

static int flush_frames_in_instance(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_framemgr *framemgr, u32 instance,
	enum fimc_is_frame_state state, enum ShotErrorType done_type)
{
	int retry = 150;
	struct fimc_is_frame *frame;
	int ret = 0;
	ulong flags = 0;
	u32 queued_count = 0;

	framemgr_e_barrier_irqs(framemgr, 0, flags);
	queued_count = framemgr->queued_count[state];
	framemgr_x_barrier_irqr(framemgr, 0, flags);

	while (--retry && queued_count) {
		framemgr_e_barrier_irqs(framemgr, 0, flags);
		frame = peek_frame(framemgr, state);
		if (!frame) {
			framemgr_x_barrier_irqr(framemgr, 0, flags);
			break;
		}

		if (frame->instance != instance) {
			msinfo_hw("different instance's frame was detected\n",
				instance, hw_ip);
			info_hw("\t frame's instance: %d, queued count: %d\n",
				frame->instance, framemgr->queued_count[state]);

			/* FIXME: consider mixing frames among instances */
			framemgr_x_barrier_irqr(framemgr, 0, flags);
			break;
		}

		info_hw("frame info: %s(queued count: %d) [F:%d][T:%d][O:0x%lx][C:0x%lx]",
			hw_frame_state_name[frame->state], framemgr->queued_count[state],
			frame->fcount, frame->type, frame->out_flag, frame->core_flag);

		set_bit(hw_ip->id, &frame->core_flag);

		framemgr_x_barrier_irqr(framemgr, 0, flags);
		ret = fimc_is_hardware_frame_ndone(hw_ip, frame, instance, done_type);
		if (ret) {
			mserr_hw("failed to process as NDONE", instance, hw_ip);
			break;
		}

		framemgr_e_barrier_irqs(framemgr, 0, flags);
		warn_hw("flushed a frame in %s, queued count: %d ",
			hw_frame_state_name[frame->state], framemgr->queued_count[state]);

		queued_count = framemgr->queued_count[state];
		framemgr_x_barrier_irqr(framemgr, 0, flags);

		if (queued_count > 0)
			usleep_range(1000, 1000);
	}

	return ret;
}

void fimc_is_hardware_force_stop(struct fimc_is_hardware *hardware,
	struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_framemgr *framemgr_late;
	enum fimc_is_frame_state state;

	FIMC_BUG_VOID(!hw_ip);

	framemgr = hw_ip->framemgr;
	msinfo_hw("frame manager queued count (%s: %d)(%s: %d)(%s: %d)\n",
		instance, hw_ip,
		hw_frame_state_name[FS_HW_WAIT_DONE],
		framemgr->queued_count[FS_HW_WAIT_DONE],
		hw_frame_state_name[FS_HW_CONFIGURE],
		framemgr->queued_count[FS_HW_CONFIGURE],
		hw_frame_state_name[FS_HW_REQUEST],
		framemgr->queued_count[FS_HW_REQUEST]);

	/* reverse order */
	for (state = FS_HW_WAIT_DONE; state > FS_HW_FREE; state--) {
		ret = flush_frames_in_instance(hw_ip, framemgr, instance, state, IS_SHOT_UNPROCESSED);
		if (ret) {
			mserr_hw("failed to flush frames in %s", instance, hw_ip,
				hw_frame_state_name[state]);
			return;
		}
	}

	framemgr_late = hw_ip->framemgr_late;
	msinfo_hw("late frame manager queued count (%s: %d)(%s: %d)(%s: %d)\n",
		instance, hw_ip,
		hw_frame_state_name[FS_HW_WAIT_DONE],
		framemgr_late->queued_count[FS_HW_WAIT_DONE],
		hw_frame_state_name[FS_HW_CONFIGURE],
		framemgr_late->queued_count[FS_HW_CONFIGURE],
		hw_frame_state_name[FS_HW_REQUEST],
		framemgr_late->queued_count[FS_HW_REQUEST]);

	for (state = FS_HW_REQUEST; state < FS_HW_INVALID; state++) {
		ret = flush_frames_in_instance(hw_ip, framemgr_late, instance, state, IS_SHOT_LATE_FRAME);
		if (ret) {
			mserr_hw("failed to flush frames in late %s", instance, hw_ip,
				hw_frame_state_name[state]);
			return;
		}
	}
}

void fimc_is_hardware_process_stop(struct fimc_is_hardware *hardware, u32 instance,
	u32 group_id, u32 mode)
{
	int ret;
	int hw_slot = -1;
	int hw_list[GROUP_HW_MAX];
	int hw_index, hw_maxnum;
	ulong hw_map;
	struct fimc_is_hw_ip *hw_ip;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	int retry;
	ulong flags = 0;

	FIMC_BUG_VOID(!hardware);

	mdbg_hw(1, "process_stop [G:0x%x](%d)\n", instance, GROUP_ID(group_id), mode);

	hw_id = get_hw_id_from_group(group_id);
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		merr_hw("invalid slot (%d,%d)", instance,
			hw_id, hw_slot);
		return;
	}
	hw_ip = &hardware->hw_ip[hw_slot];
	FIMC_BUG_VOID(!hw_ip);

	framemgr = hw_ip->framemgr;
	FIMC_BUG_VOID(!framemgr);

	framemgr_e_barrier_common(framemgr, 0, flags);
	frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
	framemgr_x_barrier_common(framemgr, 0, flags);
	if (frame && frame->instance != instance) {
		msinfo_hw("frame->instance(%d), queued_count(%d)\n",
			instance, hw_ip,
			frame->instance, framemgr->queued_count[FS_HW_WAIT_DONE]);
	} else {
		retry = 10;
		while (--retry && framemgr->queued_count[FS_HW_WAIT_DONE]) {
			mswarn_hw("HW_WAIT_DONE(%d) com waiting...", instance, hw_ip,
				framemgr->queued_count[FS_HW_WAIT_DONE]);

			usleep_range(5000, 5000);
		}
		if (!retry)
			mswarn_hw("waiting(until frame empty) is fail", instance, hw_ip);
	}

	hw_map = hardware->hw_map[instance];
	hw_maxnum = fimc_is_get_hw_list(group_id, hw_list);
	for (hw_index = 0; hw_index < hw_maxnum; hw_index++) {
		hw_id = hw_list[hw_index];
		hw_slot = fimc_is_hw_slot_id(hw_id);
		if (!valid_hw_slot_id(hw_slot)) {
			merr_hw("invalid slot (%d,%d)", instance,
				hw_id, hw_slot);
			return;
		}

		hw_ip = &hardware->hw_ip[hw_slot];

		CALL_HW_OPS(hw_ip, clk_gate, instance, true, false);
		ret = CALL_HW_OPS(hw_ip, disable, instance, hw_map);
		CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);
		if (ret) {
			mserr_hw("disable fail (%d)", instance, hw_ip, hw_slot);
		}
	}

	hw_id = get_hw_id_from_group(group_id);
	if (hw_id == DEV_HW_END)
		return;

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		if (test_bit(hw_id, &hw_map))
			merr_hw("invalid slot (%d,%d)", instance, hw_id, hw_slot);
		return;
	}

	hw_ip = &hardware->hw_ip[hw_slot];

	/* reset shot timer after process stop */
	if (!test_bit(HW_RUN, &hw_ip->state)) {
		if (hw_ip->shot_timer.data)
			del_timer_sync(&hw_ip->shot_timer);
	}

	if (mode == 0)
		return;

	CALL_HW_OPS(hw_ip, clk_gate, instance, true, false);
	fimc_is_hardware_force_stop(hardware, hw_ip, instance);
	CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);
	hw_ip->internal_fcount[instance] = 0;
	atomic_set(&hw_ip->status.otf_start, 0);

	return;
}

int fimc_is_hardware_open(struct fimc_is_hardware *hardware, u32 hw_id,
	struct fimc_is_group *group, u32 instance, bool rep_flag, u32 module_id)
{
	int ret = 0;
	int ret_err = 0;
	int hw_slot = -1;
	struct fimc_is_hw_ip *hw_ip = NULL;
	int refcount;

	FIMC_BUG(!hardware);

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		merr_hw("[ID:%d]invalid hw_slot_id [SLOT:%d]", instance, hw_id, hw_slot);
		ret = -EINVAL;
		goto err_slot;
	}

	hw_ip = &(hardware->hw_ip[hw_slot]);
	refcount = atomic_inc_return(&hw_ip->rsccount);
	if (refcount == 1) {
		u32 group_id = get_group_id_from_hw_ip(hw_ip->id);

		hw_ip->hardware = hardware;
		hw_ip->framemgr = &hardware->framemgr[group_id];
		hw_ip->framemgr_late = &hardware->framemgr_late[group_id];
		msdbg_hw(1, "%s: [G:0x%x], framemgr[ID:0x%x]\n",
			instance, hw_ip, __func__, GROUP_ID(group_id),
			(FRAMEMGR_ID_HW | (1 << hw_ip->id)));

		CALL_HW_OPS(hw_ip, clk_gate, instance, true, false);
		ret = CALL_HW_OPS(hw_ip, open, instance, group);
		CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);
		if (ret) {
			mserr_hw("open fail", instance, hw_ip);
			goto err_open;
		}

		memset(hw_ip->debug_info, 0x00, sizeof(struct hw_debug_info) * DEBUG_FRAME_COUNT);
		memset(hw_ip->setfile, 0x00, sizeof(struct fimc_is_hw_ip_setfile) * SENSOR_POSITION_MAX);
		hw_ip->applied_scenario = -1;
		hw_ip->debug_index[0] = 0;
		hw_ip->debug_index[1] = 0;
		hw_ip->sfr_dump_flag = false;
		atomic_set(&hw_ip->count.fs, 0);
		atomic_set(&hw_ip->count.cl, 0);
		atomic_set(&hw_ip->count.fe, 0);
		atomic_set(&hw_ip->count.dma, 0);
		atomic_set(&hw_ip->status.Vvalid, V_BLANK);
		atomic_set(&hw_ip->run_rsccount, 0);
		setup_timer(&hw_ip->shot_timer, fimc_is_hardware_shot_timer, (unsigned long)hw_ip);

		if (!test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state)) {
#if defined(MULTI_SHOT_TASKLET)
			tasklet_init(&hw_ip->tasklet_mshot, tasklet_mshot, (unsigned long)hw_ip);
#elif defined(MULTI_SHOT_KTHREAD)
			ret = fimc_is_hardware_init_mshot_thread(hw_ip);
			if (ret) {
				serr_hw("fimc_is_hardware_init_mshot_thread is fail(%d)", hw_ip, ret);
				goto err_kthread;
			}
#ifdef ENABLE_FPSIMD_FOR_USER
			fpsimd_set_task_using(hw_ip->mshot_task);
#endif

			sinfo_hw("multi-shot kthread: %s[%d] started\n", hw_ip,
				hw_ip->mshot_task->comm, hw_ip->mshot_task->pid);
#endif
		}
	}

	hw_ip->group[instance] = group;
	CALL_HW_OPS(hw_ip, clk_gate, instance, true, false);
	ret = CALL_HW_OPS(hw_ip, init, instance, group, rep_flag, module_id);
	CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);
	if (ret) {
		mserr_hw("init fail", instance, hw_ip);
		goto err_init;
	}

	set_bit(hw_id, &hardware->hw_map[instance]);
	msinfo_hw("open (%d)\n", instance, hw_ip, refcount);

	return 0;

err_init:
#if defined(MULTI_SHOT_KTHREAD)
	if (refcount == 1)
		fimc_is_hardware_deinit_mshot_thread(hw_ip);
err_kthread:
#endif
	if (refcount == 1) {
		CALL_HW_OPS(hw_ip, clk_gate, instance, true, true);
		ret_err = CALL_HW_OPS(hw_ip, close, instance);
		if (ret_err)
			mserr_hw("close fail (%d)", instance, hw_ip, ret_err);
	}
err_open:
	atomic_dec(&hw_ip->rsccount);
err_slot:
	return ret;
}

int fimc_is_hardware_close(struct fimc_is_hardware *hardware,u32 hw_id, u32 instance)
{
	int ret = 0;
	int hw_slot = -1;
	int refcount;
	struct fimc_is_hw_ip *hw_ip = NULL;

	FIMC_BUG(!hardware);

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		merr_hw("[ID:%d]invalid hw_slot_id [SLOT:%d]", instance, hw_id, hw_slot);
		return -EINVAL;
	}

	if (!test_bit(hw_id, &hardware->hw_map[instance])) {
		merr_hw("[ID:%d]invalid hw_map state", instance, hw_id);
		return -EINVAL;
	}

	hw_ip = &(hardware->hw_ip[hw_slot]);

	CALL_HW_OPS(hw_ip, clk_gate, instance, true, false);
	ret = CALL_HW_OPS(hw_ip, deinit, instance);
	CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);
	if (ret)
		mserr_hw("deinit fail", instance, hw_ip);

	refcount = atomic_dec_return(&hw_ip->rsccount);
	if (refcount == 0) {
		u32 group_id = get_group_id_from_hw_ip(hw_ip->id);
		if (group_id >= GROUP_ID_MAX) {
			merr_hw("[ID:%d]invalid group_id %d", instance, hw_ip->id, group_id);
			return -EINVAL;
		}

		msdbg_hw(1, "%s: [G:0x%x], framemgr[ID:0x%x]->framemgr[ID:0x%x]\n",
			instance, hw_ip, __func__, GROUP_ID(group_id),
			hw_ip->framemgr->id, hardware->framemgr[group_id].id);
		hw_ip->framemgr = &hardware->framemgr[group_id];
		hw_ip->framemgr_late = &hardware->framemgr_late[group_id];

		if (hw_ip->shot_timer.data)
			del_timer_sync(&hw_ip->shot_timer);

		CALL_HW_OPS(hw_ip, clk_gate, instance, true, true);
		ret = CALL_HW_OPS(hw_ip, close, instance);
		if (ret)
			mserr_hw("close fail", instance, hw_ip);

		memset(hw_ip->debug_info, 0x00, sizeof(struct hw_debug_info) * DEBUG_FRAME_COUNT);
		hw_ip->debug_index[0] = 0;
		hw_ip->debug_index[1] = 0;
		clear_bit(HW_OPEN, &hw_ip->state);
		clear_bit(HW_INIT, &hw_ip->state);
		clear_bit(HW_CONFIG, &hw_ip->state);
		clear_bit(HW_RUN, &hw_ip->state);
		clear_bit(HW_TUNESET, &hw_ip->state);
		atomic_set(&hw_ip->status.otf_start, 0);
		atomic_set(&hw_ip->fcount, 0);
		atomic_set(&hw_ip->instance, 0);
		atomic_set(&hw_ip->run_rsccount, 0);

#if defined(MULTI_SHOT_KTHREAD)
		fimc_is_hardware_deinit_mshot_thread(hw_ip);
#endif
	}

	clear_bit(hw_id, &hardware->hw_map[instance]);
	msinfo_hw("close (%d)\n", instance, hw_ip, refcount);
	return ret;
}

int do_frame_done_work_func(struct fimc_is_interface *itf, int wq_id, u32 instance,
	u32 group_id, u32 fcount, u32 rcount, u32 status)
{
	int ret = 0;
	bool retry_flag = false;
	struct work_struct *work_wq;
	struct fimc_is_work_list *work_list;
	struct fimc_is_work *work;

	work_wq   = &itf->work_wq[wq_id];
	work_list = &itf->work_list[wq_id];
retry:
	get_free_work(work_list, &work);
	if (work) {
		work->msg.id		= 0;
		work->msg.command	= IHC_FRAME_DONE;
		work->msg.instance	= instance;
		work->msg.group		= GROUP_ID(group_id);
		work->msg.param1	= fcount;
		work->msg.param2	= rcount;
		work->msg.param3	= status; /* status: enum ShotErrorType */
		work->msg.param4	= 0;

		work->fcount = work->msg.param1;
		set_req_work(work_list, work);

		if (!work_pending(work_wq))
			wq_func_schedule(itf, work_wq);
	} else {
		err_hw("free work item is empty (%d)", (int)retry_flag);
		if (retry_flag == false) {
			retry_flag = true;
			goto retry;
		}
		ret = -EINVAL;
	}

	return ret;
}

static int check_frame_end(struct fimc_is_hw_ip *hw_ip, u32 hw_fcount,
	struct fimc_is_frame **in_frame, struct fimc_is_framemgr *framemgr,
	u32 output_id, enum ShotErrorType done_type)
{
	int ret = 0;
	struct fimc_is_frame *frame = *in_frame;
	char *type = (output_id == FIMC_IS_HW_CORE_END) ? "CORE" : "FRAM";
	ulong flags = 0;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);
	FIMC_BUG(!framemgr);

	if (hw_fcount > frame->fcount) {
		msinfo_hw("force_done for LATE %s END [F:%d][HWF:%d][0x%x][C:0x%lx][O:0x%lx]\n",
			frame->instance, hw_ip, type, frame->fcount, hw_fcount,
			output_id, frame->core_flag, frame->out_flag);

		ret = fimc_is_hardware_frame_ndone(hw_ip, frame,
				frame->instance, IS_SHOT_UNPROCESSED);
		if (ret) {
			mserr_hw("hardware_frame_ndone fail",
				frame->instance, hw_ip);
			return -EINVAL;
		}

		framemgr_e_barrier_common(framemgr, 0, flags);
		*in_frame = find_frame(framemgr, FS_HW_WAIT_DONE, frame_fcount,
					(void *)(ulong)hw_fcount);
		framemgr_x_barrier_common(framemgr, 0, flags);
		frame = *in_frame;

		if (frame == NULL) {
			serr_hw("[F:%d]frame(null)!!(%d)", hw_ip,
				hw_fcount, done_type);
			framemgr_e_barrier_common(framemgr, 0, flags);
			frame_manager_print_info_queues(framemgr);
			print_all_hw_frame_count(hw_ip->hardware);
			framemgr_x_barrier_common(framemgr, 0, flags);
			return -EINVAL;
		}
	} else if (hw_fcount < frame->fcount) {
		mswarn_hw("%s END is ignored [F:%d][HWF:%d]\n",
			frame->instance, hw_ip, type, frame->fcount, hw_fcount);
		return -EINVAL;
	}

	return ret;
}

struct fimc_is_hw_ip *fimc_is_get_hw_ip(u32 id, struct fimc_is_hardware *hardware)
{
	struct fimc_is_hw_ip *hw_ip = NULL;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	int hw_list[GROUP_HW_MAX], hw_slot;
	int hw_maxnum = 0;

	hw_maxnum = fimc_is_get_hw_list(id, hw_list);
	hw_id = hw_list[0];
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid slot (%d,%d)", hw_id, hw_slot);
		return NULL;
	}

	hw_ip = &hardware->hw_ip[hw_slot];

	return hw_ip;
}

int fimc_is_hardware_shot_done(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	struct fimc_is_framemgr *framemgr, enum ShotErrorType done_type)
{
	int ret = 0;
	struct work_struct *work_wq;
	struct fimc_is_work_list *work_list;
	struct fimc_is_work *work;
	struct fimc_is_group *head;
	u32  req_id;
	ulong flags = 0;

	FIMC_BUG(!hw_ip);

	if (frame == NULL) {
		serr_hw("frame(null)!!", hw_ip);
		framemgr_e_barrier_common(framemgr, 0, flags);
		frame_manager_print_info_queues(framemgr);
		print_all_hw_frame_count(hw_ip->hardware);
		framemgr_x_barrier_common(framemgr, 0, flags);
		FIMC_BUG(1);
	}

	head = GET_HEAD_GROUP_IN_DEVICE(FIMC_IS_DEVICE_ISCHAIN,
			hw_ip->group[frame->instance]);

	msdbgs_hw(2, "shot_done [F:%d][G:0x%x][B:0x%lx][C:0x%lx][O:0x%lx]\n",
		frame->instance, hw_ip, frame->fcount, GROUP_ID(head->id),
		frame->bak_flag, frame->core_flag, frame->out_flag);

	if (frame->type == SHOT_TYPE_INTERNAL || frame->type == SHOT_TYPE_MULTI)
		goto free_frame;

	switch (head->id) {
	case GROUP_ID_PAF0:
	case GROUP_ID_PAF1:
	case GROUP_ID_3AA0:
	case GROUP_ID_3AA1:
	case GROUP_ID_ISP0:
	case GROUP_ID_ISP1:
	case GROUP_ID_DIS0:
	case GROUP_ID_DIS1:
	case GROUP_ID_DCP:
	case GROUP_ID_MCS0:
	case GROUP_ID_MCS1:
	case GROUP_ID_VRA0:
		req_id = head->leader.id;
		break;
	default:
		err_hw("invalid group (G%d)", head->id);
		goto exit;
	}

	if (!test_bit_variables(req_id, &frame->out_flag)) {
		mserr_hw("invalid bak_flag [F:%d][0x%x][B:0x%lx][O:0x%lx]",
			frame->instance, hw_ip, frame->fcount, req_id,
			frame->bak_flag, frame->out_flag);
		goto free_frame;
	}

	work_wq   = &hw_ip->itf->work_wq[WORK_SHOT_DONE];
	work_list = &hw_ip->itf->work_list[WORK_SHOT_DONE];

	get_free_work(work_list, &work);
	if (work) {
		work->msg.id		= 0;
		work->msg.command	= IHC_FRAME_DONE;
		work->msg.instance	= frame->instance;
		work->msg.group		= GROUP_ID(head->id);
		work->msg.param1	= frame->fcount;
		work->msg.param2	= done_type; /* status: enum ShotErrorType */
		work->msg.param3	= 0;
		work->msg.param4	= 0;

		work->fcount = work->msg.param1;
		set_req_work(work_list, work);

		if (!work_pending(work_wq))
			wq_func_schedule(hw_ip->itf, work_wq);
	} else {
		err_hw("free work item is empty\n");
	}
	clear_bit(req_id, &frame->out_flag);

free_frame:
	if (done_type) {
		msinfo_hw("SHOT_NDONE [E%d][F:%d][G:0x%x]\n", frame->instance, hw_ip,
			done_type, frame->fcount, GROUP_ID(head->id));
		goto exit;
	}

	if (frame->type == SHOT_TYPE_INTERNAL) {
		msdbg_hw(1, "INTERNAL_SHOT_DONE [F:%d][G:0x%x]\n",
			frame->instance, hw_ip, frame->fcount, GROUP_ID(head->id));
		atomic_inc(&hw_ip->hardware->log_count);
	} else if (frame->type == SHOT_TYPE_MULTI) {
#if !defined(ENABLE_EARLY_SHOT)
		struct fimc_is_hw_ip *hw_ip_ldr = fimc_is_get_hw_ip(head->id, hw_ip->hardware);

		mshot_schedule(hw_ip_ldr);
		msdbg_hw(1, "SHOT_TYPE_MULTI [F:%d][G:0x%x]\n",
			frame->instance, hw_ip, frame->fcount, GROUP_ID(head->id));
#endif
	} else {
		msdbg_hw(1, "SHOT_DONE [F:%d][G:0x%x]\n",
			frame->instance, hw_ip, frame->fcount, GROUP_ID(head->id));
		atomic_set(&hw_ip->hardware->log_count, 0);
	}
exit:
	framemgr_e_barrier_common(framemgr, 0, flags);
	trans_frame(framemgr, frame, FS_HW_FREE);
	framemgr_x_barrier_common(framemgr, 0, flags);
	atomic_set(&frame->shot_done_flag, 0);

	/* Force flush the old H/W frames with DONE state */
	framemgr_e_barrier_common(framemgr, 0, flags);
	if (framemgr->queued_count[FS_HW_WAIT_DONE] > 0) {
		struct fimc_is_frame *temp;
		u32 fcount = frame->fcount;
		u32 instance = frame->instance;

		list_for_each_entry_safe(frame, temp, &framemgr->queued_list[FS_HW_WAIT_DONE], list) {
			if (frame && frame->instance == instance && frame->fcount < fcount) {
				msinfo_hw("[F%d]force flush\n",
						frame->instance, hw_ip, frame->fcount);
				trans_frame(framemgr, frame, FS_HW_FREE);
				atomic_set(&frame->shot_done_flag, 0);
			}
		}
	}
	framemgr_x_barrier_common(framemgr, 0, flags);

	if (framemgr->queued_count[FS_HW_FREE] > 10)
		atomic_set(&hw_ip->hardware->bug_count, 0);

	return ret;
}

int fimc_is_hardware_frame_done(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	int wq_id, u32 output_id, enum ShotErrorType done_type, bool get_meta)
{
	int ret = 0;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_group *head;
	ulong flags = 0;

	FIMC_BUG(!hw_ip);

	framemgr = hw_ip->framemgr;

	switch (done_type) {
	case IS_SHOT_SUCCESS:
		if (frame == NULL) {
			framemgr_e_barrier_common(framemgr, 0, flags);
			frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
			framemgr_x_barrier_common(framemgr, 0, flags);
		} else {
			sdbg_hw(2, "frame NOT null!!(%d)", hw_ip, done_type);
		}
		break;
	case IS_SHOT_LATE_FRAME:
		framemgr = hw_ip->framemgr_late;
		if (frame == NULL) {
			swarn_hw("frame null!!(%d)", hw_ip, done_type);
			framemgr_e_barrier_common(framemgr, 0, flags);
			frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
			framemgr_x_barrier_common(framemgr, 0, flags);
		}

		if (frame!= NULL && frame->type != SHOT_TYPE_LATE) {
			swarn_hw("invalid frame type", hw_ip);
			frame->type = SHOT_TYPE_LATE;
		}
		break;
	case IS_SHOT_UNPROCESSED:
	case IS_SHOT_OVERFLOW:
	case IS_SHOT_INVALID_FRAMENUMBER:
	case IS_SHOT_TIMEOUT:
	case IS_SHOT_CONFIG_LOCK_DELAY:
		break;
	default:
		serr_hw("invalid done_type(%d)", hw_ip, done_type);
		return -EINVAL;
	}

	if (frame == NULL) {
		serr_hw("[F:%d]frame_done: frame(null)!!(%d)(0x%x)", hw_ip,
			atomic_read(&hw_ip->fcount), done_type, output_id);
		framemgr_e_barrier_common(framemgr, 0, flags);
		frame_manager_print_info_queues(framemgr);
		print_all_hw_frame_count(hw_ip->hardware);
		framemgr_x_barrier_common(framemgr, 0, flags);
		return -EINVAL;
	}

	head = GET_HEAD_GROUP_IN_DEVICE(FIMC_IS_DEVICE_ISCHAIN,
			hw_ip->group[frame->instance]);

	msdbgs_hw(2, "[0x%x]frame_done [F:%d][G:0x%x][B:0x%lx][C:0x%lx][O:0x%lx]\n",
		frame->instance, hw_ip, output_id, frame->fcount,
		GROUP_ID(head->id), frame->bak_flag, frame->core_flag, frame->out_flag);

	/* check core_done */
	if (output_id == FIMC_IS_HW_CORE_END) {
		switch (done_type) {
		case IS_SHOT_SUCCESS:
			if (!test_bit_variables(hw_ip->id, &frame->core_flag)) {
				ret = check_frame_end(hw_ip, atomic_read(&hw_ip->fcount), &frame,
					framemgr, output_id, done_type);
				if (ret)
					return ret;
			}
			break;
		case IS_SHOT_UNPROCESSED:
		case IS_SHOT_LATE_FRAME:
		case IS_SHOT_OVERFLOW:
		case IS_SHOT_INVALID_FRAMENUMBER:
		case IS_SHOT_TIMEOUT:
		case IS_SHOT_CONFIG_LOCK_DELAY:
			break;
		default:
			break;
		}

		if (!test_bit_variables(hw_ip->id, &frame->core_flag)) {
			info_hw("[%d]invalid core_flag [ID:%d][F:%d][0x%x][C:0x%lx][O:0x%lx]",
				frame->instance, hw_ip->id, frame->fcount,
				output_id, frame->core_flag, frame->out_flag);
			goto shot_done;
		}

		if (hw_ip->is_leader) {
			frame->frame_info[INFO_FRAME_END_PROC].cpu = raw_smp_processor_id();
			frame->frame_info[INFO_FRAME_END_PROC].pid = current->pid;
			frame->frame_info[INFO_FRAME_END_PROC].when = local_clock();
		}

		if (frame->shot && get_meta)
			fimc_is_hardware_get_meta(hw_ip, frame,
				frame->instance, hw_ip->hardware->hw_map[frame->instance],
				output_id, done_type);
	} else {
		if (frame->type == SHOT_TYPE_INTERNAL)
			goto shot_done;

		if (frame->type == SHOT_TYPE_MULTI) {
			clear_bit(output_id, &frame->out_flag);
			goto shot_done;
		}

		switch(done_type) {
		case IS_SHOT_SUCCESS:
			if (!test_bit_variables(output_id, &frame->out_flag)) {
				ret = check_frame_end(hw_ip, atomic_read(&hw_ip->fcount), &frame,
					framemgr, output_id, done_type);
				if (ret)
					return ret;
			}
			break;
		case IS_SHOT_UNPROCESSED:
		case IS_SHOT_LATE_FRAME:
		case IS_SHOT_OVERFLOW:
		case IS_SHOT_INVALID_FRAMENUMBER:
		case IS_SHOT_TIMEOUT:
		case IS_SHOT_CONFIG_LOCK_DELAY:
			break;
		default:
			break;
		}

		if (!test_bit_variables(output_id, &frame->out_flag)) {
			info_hw("[%d]invalid output_id [ID:%d][F:%d][0x%x][C:0x%lx][O:0x%lx]",
				frame->instance, hw_ip->id, frame->fcount,
				output_id, frame->core_flag, frame->out_flag);
			goto shot_done;
		}

		if (frame->shot && get_meta)
			fimc_is_hardware_get_meta(hw_ip, frame,
				frame->instance, hw_ip->hardware->hw_map[frame->instance],
				output_id, done_type);

		ret = do_frame_done_work_func(hw_ip->itf,
				wq_id,
				frame->instance,
				head->id,
				frame->fcount,
				frame->rcount,
				done_type);
		if (ret)
			FIMC_BUG(1);

		clear_bit(output_id, &frame->out_flag);
	}

shot_done:
	if (output_id == FIMC_IS_HW_CORE_END)
		clear_bit(hw_ip->id, &frame->core_flag);

	framemgr_e_barrier_common(framemgr, 0, flags);
	if (!OUT_FLAG(frame->out_flag, head->leader.id)
		&& !frame->core_flag
		&& atomic_dec_and_test(&frame->shot_done_flag)) {
		framemgr_x_barrier_common(framemgr, 0, flags);
		ret = fimc_is_hardware_shot_done(hw_ip, frame, framemgr, done_type);
		return ret;
	}
	framemgr_x_barrier_common(framemgr, 0, flags);

	return ret;
}

int fimc_is_hardware_frame_ndone(struct fimc_is_hw_ip *ldr_hw_ip,
	struct fimc_is_frame *frame, u32 instance,
	enum ShotErrorType done_type)
{
	int ret = 0;
	int hw_slot = -1;
	struct fimc_is_hw_ip *hw_ip = NULL;
	struct fimc_is_group *group = NULL;
	struct fimc_is_group *head = NULL;
	struct fimc_is_hardware *hardware;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	int hw_list[GROUP_HW_MAX], hw_index;
	int hw_maxnum = 0;

	if (!frame) {
		mserr_hw("ndone frame is NULL(%d)", instance, ldr_hw_ip, done_type);
		return -EINVAL;
	} else {
		msinfo_hw("%s[F:%d][E%d][O:0x%lx][C:0x%lx]\n",
				instance, ldr_hw_ip, __func__,
				frame->fcount, done_type,
				frame->out_flag, frame->core_flag);
	}

	group = ldr_hw_ip->group[instance];
	head = GET_HEAD_GROUP_IN_DEVICE(FIMC_IS_DEVICE_ISCHAIN, group);

	hardware = ldr_hw_ip->hardware;

	/* if there is not any out_flag without leader, forcely set the core flag */
	if (!OUT_FLAG(frame->out_flag, group->leader.id))
		set_bit(ldr_hw_ip->id, &frame->core_flag);

	while (head) {
		hw_maxnum = fimc_is_get_hw_list(head->id, hw_list);
		for (hw_index = 0; hw_index < hw_maxnum; hw_index++) {
			hw_id = hw_list[hw_index];
			hw_slot = fimc_is_hw_slot_id(hw_id);
			if (!valid_hw_slot_id(hw_slot)) {
				merr_hw("invalid slot (%d,%d)", instance, hw_id, hw_slot);
				return -EINVAL;
			}

			hw_ip = &(hardware->hw_ip[hw_slot]);
			ret = CALL_HW_OPS(hw_ip, frame_ndone, frame, instance, done_type);
			if (ret) {
				mserr_hw("frame_ndone fail (%d)", instance, hw_ip, hw_slot);
				return -EINVAL;
			}

			if (done_type == IS_SHOT_TIMEOUT)
#ifdef SKIP_VRA_SFR_DUMP
			{
				/* HACK
				 * To access the register with offset value 4098 in VRA
				 * occurs ITMON read time-out error.
				 */
				if (hw_ip->id != DEV_HW_VRA)
					fimc_is_hardware_sfr_dump(hardware, hw_id, false);
			}
#else
				fimc_is_hardware_sfr_dump(hardware, hw_id, false);
#endif
		}
		head = head->child;
	}

	return ret;
}

static int parse_setfile_header(ulong addr, struct fimc_is_setfile_header *header)
{
	union __setfile_header *file_header;

	/* 1. check setfile version */
	/* 2. load version specific header information */
	file_header = (union __setfile_header *)addr;
	if (file_header->magic_number == (SET_FILE_MAGIC_NUMBER - 1)) {
		header->version = SETFILE_V2;

		header->num_ips = file_header->ver_2.subip_num;
		header->num_scenarios = file_header->ver_2.scenario_num;

		header->scenario_table_base = addr + sizeof(struct __setfile_header_ver_2);
		header->setfile_entries_base = addr + file_header->ver_2.setfile_offset;

		header->designed_bits = 0;
		memset(header->version_code, 0, 5);
		memset(header->revision_code, 0, 5);
	} else if (file_header->magic_number == SET_FILE_MAGIC_NUMBER) {
		header->version = SETFILE_V3;

		header->num_ips = file_header->ver_3.subip_num;
		header->num_scenarios = file_header->ver_3.scenario_num;

		header->scenario_table_base = addr + sizeof(struct __setfile_header_ver_3);
		header->setfile_entries_base = addr + file_header->ver_3.setfile_offset;

		header->designed_bits = file_header->ver_3.designed_bit;
		memcpy(header->version_code, file_header->ver_3.version_code, 4);
		header->version_code[4] = 0;
		memcpy(header->revision_code, file_header->ver_3.revision_code, 4);
		header->revision_code[4] = 0;
	} else {
		err_hw("invalid magic number[0x%08x]", file_header->magic_number);
		return -EINVAL;
	}

	/* 3. process more header information */
	header->num_setfile_base = header->scenario_table_base
		+ (header->num_ips * header->num_scenarios * sizeof(u32));
	header->setfile_table_base = header->num_setfile_base
		+ (header->num_ips * sizeof(u32));

	info_hw("%s: version(%d)(%s)\n", __func__, header->version, header->revision_code);

	dbg_hw(1, "%s: number of IPs: %d\n", __func__, header->num_ips);
	dbg_hw(1, "%s: number of scenario: %d\n", __func__, header->num_scenarios);
	dbg_hw(1, "%s: scenario table base: 0x%lx\n", __func__, header->scenario_table_base);
	dbg_hw(1, "%s: number of setfile base: 0x%lx\n", __func__, header->num_setfile_base);
	dbg_hw(1, "%s: setfile table base: 0x%lx\n", __func__, header->setfile_table_base);
	dbg_hw(1, "%s: setfile entries base: 0x%lx\n", __func__, header->setfile_entries_base);

	return 0;
}

static int parse_setfile_info(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_setfile_header header,
	u32 instance,
	u32 num_ips,
	struct __setfile_table_entry *setfile_table_entry)
{
	unsigned long base;
	size_t blk_size;
	u32 idx;
	struct fimc_is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	/* skip setfile parsing if it alreay parsed at each sensor position */
	if (setfile->using_count > 0)
		return 0;

	/* set version */
	setfile->version = header.version;

	/* set what setfile index is used at each scenario */
	base = header.scenario_table_base;
	blk_size = header.num_scenarios * sizeof(u32);
	memcpy(setfile->index, (void *)(base + (num_ips * blk_size)), blk_size);

	/* fill out-of-range index for each not-used scenario to check sanity */
	memset((u32 *)&setfile->index[header.num_scenarios],
		0xff, (FIMC_IS_MAX_SCENARIO - header.num_scenarios) * sizeof(u32));
	for (idx = 0; idx < header.num_scenarios; idx++)
		msdbg_hw(1, "scenario table [%d:%d]\n", instance, hw_ip,
			idx, setfile->index[idx]);

	/* set the number of setfile at each sub IP */
	base = header.num_setfile_base;
	blk_size = sizeof(u32);
	setfile->using_count = (u32)*(ulong *)(base + (num_ips * blk_size));

	if (setfile->using_count > FIMC_IS_MAX_SETFILE) {
		mserr_hw("too many setfile entries: %d", instance, hw_ip, setfile->using_count);
		return -EINVAL;
	}

	msdbg_hw(1, "number of setfile: %d\n", instance, hw_ip, setfile->using_count);

	/* set each setfile address and size */
	for (idx = 0; idx < setfile->using_count; idx++) {
		setfile->table[idx].addr =
			(ulong)(header.setfile_entries_base + setfile_table_entry[idx].offset),
		setfile->table[idx].size = setfile_table_entry[idx].size;

		msdbg_hw(1, "setfile[%d] addr: 0x%lx, size: %x\n",
			instance, hw_ip, idx,
			setfile->table[idx].addr,
			setfile->table[idx].size);
	}

	return 0;
}

static void set_hw_slots_bit(unsigned long *slots, int nslots, int hw_id)
{
	int hw_slot;

	switch (hw_id) {
	/* setfile chain (3AA0, 3AA1, ISP0, ISP1) */
	case DEV_HW_3AA0:
		hw_slot = fimc_is_hw_slot_id(hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		hw_id = DEV_HW_3AA1;
	case DEV_HW_3AA1:
		hw_slot = fimc_is_hw_slot_id(hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		hw_id = DEV_HW_ISP0;
	case DEV_HW_ISP0:
		hw_slot = fimc_is_hw_slot_id(hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		hw_id = DEV_HW_ISP1;
		break;

	/* setfile chain (MCSC0, MCSC1) */
	case DEV_HW_MCSC0:
		hw_slot = fimc_is_hw_slot_id(hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		hw_id = DEV_HW_MCSC1;
		break;
	}

	switch (hw_id) {
	/* every leaf of each setfile chain */
	case DEV_HW_ISP1:

	case DEV_HW_DRC:
	case DEV_HW_DIS:
	case DEV_HW_3DNR:
	case DEV_HW_SCP:
	case DEV_HW_FD:
	case DEV_HW_VRA:

	case DEV_HW_MCSC1:
		hw_slot = fimc_is_hw_slot_id(hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		break;
	}
}

static void get_setfile_hw_slots_no_hint(unsigned long *slots, int ip, u32 num_ips)
{
	int hw_id = 0;
	bool has_mcsc;

	bitmap_zero(slots, HW_SLOT_MAX);

	if (num_ips == 3) {
		/* ISP, DRC, VRA */
		switch (ip) {
		case 0:
			hw_id = DEV_HW_3AA0;
			break;
		case 1:
			hw_id = DEV_HW_DRC;
			break;
		case 2:
			hw_id = DEV_HW_VRA;
			break;
		}
	} else if (num_ips == 4) {
		/* ISP, DRC, TDNR, VRA */
		switch (ip) {
		case 0:
			hw_id = DEV_HW_3AA0;
			break;
		case 1:
			hw_id = DEV_HW_DRC;
			break;
		case 2:
			hw_id = DEV_HW_3DNR;
			break;
		case 3:
			hw_id = DEV_HW_VRA;
			break;
		}
	} else if (num_ips == 5) {
		/* ISP, DRC, DIS, TDNR, VRA */
		switch (ip) {
		case 0:
			hw_id = DEV_HW_3AA0;
			break;
		case 1:
			hw_id = DEV_HW_DRC;
			break;
		case 2:
			hw_id = DEV_HW_DIS;
			break;
		case 3:
			hw_id = DEV_HW_3DNR;
			break;
		case 4:
			hw_id = DEV_HW_VRA;
			break;
		}
	} else if (num_ips == 6) {
		/* ISP, DRC, DIS, TDNR, MCSC, VRA */
		switch (ip) {
		case 0:
			hw_id = DEV_HW_3AA0;
			break;
		case 1:
			hw_id = DEV_HW_DRC;
			break;
		case 2:
			hw_id = DEV_HW_DIS;
			break;
		case 3:
			hw_id = DEV_HW_3DNR;
			break;
		case 4:
			fimc_is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_MCSC, (void *)&has_mcsc);
			hw_id = has_mcsc ? DEV_HW_MCSC0 : DEV_HW_SCP;
			break;
		case 5:
			hw_id = DEV_HW_VRA;
			break;
		}
	}

	dbg_hw(1, "%s: hw_id: %d, IP: %d, number of IPs: %d\n", __func__, hw_id, ip, num_ips);

	if (hw_id > 0)
		set_hw_slots_bit(slots, HW_SLOT_MAX, hw_id);
}

static void get_setfile_hw_slots(unsigned long *slots, unsigned long *hint)
{
	bool has_mcsc;

	dbg_hw(1, "%s: designed bits(0x%lx) ", __func__, *hint);

	bitmap_zero(slots, HW_SLOT_MAX);

	if (test_and_clear_bit(SETFILE_DESIGN_BIT_3AA_ISP, hint)) {
		set_hw_slots_bit(slots, HW_SLOT_MAX, DEV_HW_3AA0);

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_DRC, hint)) {
		set_hw_slots_bit(slots, HW_SLOT_MAX, DEV_HW_DRC);

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_SCC, hint)) {
		/* not supported yet */
		/* set_hw_slots_bit(slots, HW_SLOT_MAX, DEV_HW_SCC); */

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_ODC, hint)) {
		/* not supported yet */
		/* set_hw_slots_bit(slots, HW_SLOT_MAX, DEV_HW_ODC); */

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_VDIS, hint)) {
		set_hw_slots_bit(slots, HW_SLOT_MAX, DEV_HW_DIS);

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_TDNR, hint)) {
		set_hw_slots_bit(slots, HW_SLOT_MAX, DEV_HW_3DNR);

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_SCX_MCSC, hint)) {
		fimc_is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_MCSC, (void *)&has_mcsc);
		set_hw_slots_bit(slots, HW_SLOT_MAX,
				has_mcsc ? DEV_HW_MCSC0 : DEV_HW_SCP);

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_FD_VRA, hint)) {
		set_hw_slots_bit(slots, HW_SLOT_MAX, DEV_HW_VRA);

	}

	dbg_hw(1, "              -> (0x%lx)\n", *hint);
}

int fimc_is_hardware_load_setfile(struct fimc_is_hardware *hardware, ulong addr,
	u32 instance, ulong hw_map)
{
	struct fimc_is_setfile_header header;
	struct __setfile_table_entry *setfile_table_entry;
	unsigned long slots[DIV_ROUND_UP(HW_SLOT_MAX, BITS_PER_LONG)];
	struct fimc_is_hw_ip *hw_ip;
	unsigned long hw_slot;
	unsigned long hint;
	u32 ip;
	ulong setfile_table_idx = 0;
	int ret = 0;
	enum exynos_sensor_position sensor_position;

	ret = parse_setfile_header(addr, &header);
	if (ret) {
		merr_hw("failed to parse setfile header(%d)", instance, ret);
		return ret;
	}

	if (header.num_scenarios > FIMC_IS_MAX_SCENARIO) {
		merr_hw("too many scenarios: %d", instance, header.num_scenarios);
		return -EINVAL;
	}

	hint = header.designed_bits;
	setfile_table_entry = (struct __setfile_table_entry *)header.setfile_table_base;
	sensor_position = hardware->sensor_position[instance];

	for (ip = 0; ip < header.num_ips; ip++) {
		if (header.version == SETFILE_V3)
			get_setfile_hw_slots(slots, &hint);
		else
			get_setfile_hw_slots_no_hint(slots, ip, header.num_ips);

		hw_ip = NULL;

		hw_slot = find_first_bit(slots, HW_SLOT_MAX);
		while (hw_slot != HW_SLOT_MAX) {
			hw_ip = &hardware->hw_ip[hw_slot];

			clear_bit(hw_slot, slots);
			hw_slot = find_first_bit(slots, HW_SLOT_MAX);

			if (!test_bit(hw_ip->id, &hardware->hw_map[instance])) {
				msdbg_hw(1, "skip parsing at not mapped hw_ip", instance, hw_ip);
				if (hw_slot) {
					unsigned long base = header.num_setfile_base;
					size_t blk_size = sizeof(u32);

					setfile_table_idx = (u32)*(ulong *)(base + (ip * blk_size));
				}
				continue;
			}

			ret = parse_setfile_info(hw_ip, header, instance, ip, setfile_table_entry);
			if (ret) {
				mserr_hw("parse setfile info failed\n", instance, hw_ip);
				return ret;
			}

			ret = CALL_HW_OPS(hw_ip, load_setfile, instance, hw_map);
			if (ret) {
				mserr_hw("failed to load setfile(%d)", instance, hw_ip, ret);
				return ret;
			}

			/* set setfile table idx for next setfile_table base */
			setfile_table_idx = (ulong)hw_ip->setfile[sensor_position].using_count;
		}

		/* increase setfile table base even though there is no valid HW slot */
		if (hw_ip)
			setfile_table_entry += setfile_table_idx;
		else
			setfile_table_entry++;
	}

	return ret;
};

int fimc_is_hardware_apply_setfile(struct fimc_is_hardware *hardware, u32 instance,
	u32 scenario, ulong hw_map)
{
	struct fimc_is_hw_ip *hw_ip = NULL;
	int hw_id = 0;
	int ret = 0;
	int hw_slot = -1;
	enum exynos_sensor_position sensor_position;

	FIMC_BUG(!hardware);

	if (FIMC_IS_MAX_SCENARIO <= scenario) {
		merr_hw("invalid scenario id: scenario(%d)", instance, scenario);
		return -EINVAL;
	}

	minfo_hw("apply_setfile: hw_map (0x%lx)\n", instance, hw_map);

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];
		hw_id = hw_ip->id;
		if(!test_bit(hw_id, &hardware->hw_map[instance]))
			continue;

		ret = CALL_HW_OPS(hw_ip, apply_setfile, scenario, instance, hw_map);
		if (ret) {
			mserr_hw("apply_setfile fail (%d)", instance, hw_ip, ret);
			return -EINVAL;
		}

		sensor_position = hardware->sensor_position[instance];
		hw_ip->applied_scenario = scenario;
	}

	return ret;
}

int fimc_is_hardware_delete_setfile(struct fimc_is_hardware *hardware, u32 instance,
	ulong hw_map)
{
	int ret = 0;
	int hw_slot = -1;
	struct fimc_is_hw_ip *hw_ip = NULL;

	FIMC_BUG(!hardware);

	minfo_hw("delete_setfile: hw_map (0x%lx)\n", instance, hw_map);
	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];

		if (!test_bit_variables(hw_ip->id, &hw_map))
			continue;

		ret = CALL_HW_OPS(hw_ip, delete_setfile, instance, hw_map);
		if (ret) {
			mserr_hw("delete_setfile fail", instance, hw_ip);
			return -EINVAL;
		}
	}

	return ret;
}

int fimc_is_hardware_runtime_resume(struct fimc_is_hardware *hardware)
{
	int ret = 0;
#ifdef ENABLE_DIRECT_CLOCK_GATE
	int hw_slot = -1;
	struct fimc_is_hw_ip *hw_ip = NULL;

	FIMC_BUG(!hardware);

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];
		/* init clk gating variable */
		if (hw_ip->clk_gate != NULL)
			memset(hw_ip->clk_gate->refcnt, 0x0, sizeof(int) * HW_SLOT_MAX);
	}
#endif
	return ret;
}

int fimc_is_hardware_runtime_suspend(struct fimc_is_hardware *hardware)
{
	return 0;
}

void fimc_is_hardware_clk_gate(struct fimc_is_hw_ip *hw_ip, u32 instance,
	bool on, bool close)
{
#ifdef ENABLE_DIRECT_CLOCK_GATE
	struct fimc_is_group *head;
	struct fimc_is_clk_gate *clk_gate;
	u32 idx;
	ulong flag;

	FIMC_BUG_VOID(!hw_ip);

	if (!sysfs_debug.en_clk_gate || hw_ip->clk_gate == NULL)
		return;

	clk_gate = hw_ip->clk_gate;
	idx = hw_ip->clk_gate_idx;

	if (close) {
		spin_lock_irqsave(&clk_gate->slock, flag);
		FIMC_IS_CLOCK_ON(clk_gate->regs, clk_gate->bit[idx]);
		spin_unlock_irqrestore(&clk_gate->slock, flag);
		return;
	}

	if (!hw_ip->group[instance])
		return;

	head = hw_ip->group[instance]->head;
	if (!head) {
		err_hw("[%d]head is NULL", instance);
		spin_lock_irqsave(&clk_gate->slock, flag);
		FIMC_IS_CLOCK_ON(clk_gate->regs, clk_gate->bit[idx]);
		spin_unlock_irqrestore(&clk_gate->slock, flag);
		return;
	}

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state))
		return;

	spin_lock_irqsave(&clk_gate->slock, flag);

	if(on) {
		clk_gate->refcnt[idx]++;

		if (clk_gate->refcnt[idx] > 1)
			goto exit;

		FIMC_IS_CLOCK_ON(clk_gate->regs, clk_gate->bit[idx]);
	} else {
		clk_gate->refcnt[idx]--;

		if (clk_gate->refcnt[idx] >= 1)
			goto exit;

		if(clk_gate->refcnt[idx] < 0){
			warn("[%d][ID:%d] clock is already disable(%d)", instance, hw_ip->id, clk_gate->refcnt[idx]);
			clk_gate->refcnt[idx] = 0;
			goto exit;
		}

		FIMC_IS_CLOCK_OFF(clk_gate->regs, clk_gate->bit[idx]);
	}
exit:
	spin_unlock_irqrestore(&clk_gate->slock, flag);

	if (clk_gate->refcnt[idx] > FIMC_IS_STREAM_COUNT)
		warn("[%d][ID:%d] abnormal clk_gate refcnt(%d)", instance, hw_ip->id, clk_gate->refcnt[idx]);

	return;
#endif
}

void fimc_is_hardware_sfr_dump(struct fimc_is_hardware *hardware, u32 hw_id, bool flag_print_log)
{
	int hw_slot = -1;
	struct fimc_is_hw_ip *hw_ip = NULL;

	if (!hardware) {
		err_hw("hardware is null\n");
		return;
	}

	if (hw_id == HW_SLOT_MAX) {
		for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
			hw_ip = &hardware->hw_ip[hw_slot];
			_fimc_is_hardware_sfr_dump(hw_ip, flag_print_log);
		}
	} else {
		hw_slot = fimc_is_hw_slot_id(hw_id);
		if (hw_slot >= 0) {
			hw_ip = &hardware->hw_ip[hw_slot];
			_fimc_is_hardware_sfr_dump(hw_ip, flag_print_log);
		}
	}
}

int fimc_is_hardware_flush_frame_by_group(struct fimc_is_hardware *hardware,
	struct fimc_is_group *group, u32 instance)
{
	struct fimc_is_hw_ip *hw_ip = NULL;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	int hw_list[GROUP_HW_MAX], hw_slot;
	int hw_maxnum = 0;

	hw_maxnum = fimc_is_get_hw_list(group->id, hw_list);
	hw_id = hw_list[0];
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		merr_hw("invalid slot (%d,%d)", instance,
				hw_id, hw_slot);
		return -EINVAL;
	}
	hw_ip = &hardware->hw_ip[hw_slot];

	msdbg_hw(1, "flush_frame by group(%d)\n", instance, hw_ip, group->id);
	fimc_is_hardware_flush_frame(hw_ip, FS_HW_REQUEST, IS_SHOT_UNPROCESSED);

	return 0;
}

int fimc_is_hardware_restore_by_group(struct fimc_is_hardware *hardware,
	struct fimc_is_group *group, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_ip *hw_ip = NULL;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	int hw_list[GROUP_HW_MAX], hw_index, hw_slot;
	int hw_maxnum = 0;

	hw_maxnum = fimc_is_get_hw_list(group->id, hw_list);
	for (hw_index = hw_maxnum - 1; hw_index >= 0; hw_index--) {
		hw_id = hw_list[hw_index];
		hw_slot = fimc_is_hw_slot_id(hw_id);
		if (!valid_hw_slot_id(hw_slot)) {
			merr_hw("invalid slot (%d,%d)", instance,
					hw_id, hw_slot);
			return -EINVAL;
		}
		hw_ip = &hardware->hw_ip[hw_slot];

		ret = CALL_HW_OPS(hw_ip, restore, instance);
		if (ret) {
			mserr_hw("reset & restore fail = %x", instance, hw_ip, ret);
			goto exit;
		}
	}

exit:
	return ret;
}

int fimc_is_hardware_recovery_shot(struct fimc_is_hardware *hardware, u32 instance,
	struct fimc_is_group *group, u32 recov_fcount, struct fimc_is_framemgr *framemgr)
{
	int ret = 0;
	struct fimc_is_frame *frame = NULL;
	struct fimc_is_hw_ip *hw_ip = NULL;
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	int hw_list[GROUP_HW_MAX], hw_slot;
	int hw_maxnum = 0;
	ulong flags = 0;

	hw_maxnum = fimc_is_get_hw_list(group->id, hw_list);
	hw_id = hw_list[0];
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		merr_hw("invalid slot (%d,%d)", instance,
				hw_id, hw_slot);
		return -EINVAL;
	}
	hw_ip = &hardware->hw_ip[hw_slot];

	framemgr_e_barrier_common(framemgr, 0, flags);
	frame = get_frame(framemgr, FS_HW_REQUEST);
	if (!frame) {
		ret = make_internal_shot(hw_ip, instance, recov_fcount, &frame, framemgr);
		if (ret) {
			framemgr_x_barrier_common(framemgr, 0, flags);
			goto exit;
		}
	}
	framemgr_x_barrier_common(framemgr, 0, flags);

	ret = fimc_is_hardware_shot(hardware, instance, hw_ip->group[instance],
			frame, framemgr, hardware->hw_map[instance], frame->fcount);

exit:
	return ret;
}
