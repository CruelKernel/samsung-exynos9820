/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-interface-ddk.h"
#include "fimc-is-hw-control.h"
#include "sfr/fimc-is-sfr-isp-v310.h"
#include "fimc-is-err.h"
#include <soc/samsung/exynos-bcm_dbg.h>

int debug_irq_ddk;
module_param(debug_irq_ddk, int, 0644);
int debug_time_hw;
module_param(debug_time_hw, int, 0644);

bool check_dma_done(struct fimc_is_hw_ip *hw_ip, u32 instance_id, u32 fcount)
{
	bool ret = false;
	struct fimc_is_frame *frame;
	struct fimc_is_framemgr *framemgr;
	int wq_id0 = WORK_MAX_MAP, wq_id1 = WORK_MAX_MAP;
	int wq_id2 = WORK_MAX_MAP, wq_id3 = WORK_MAX_MAP;
	int wq_id4 = WORK_MAX_MAP, wq_id5 = WORK_MAX_MAP;
	int output_id0 = ENTRY_END, output_id1 = ENTRY_END;
	int output_id2 = ENTRY_END, output_id3 = ENTRY_END;
	int output_id4 = ENTRY_END, output_id5 = ENTRY_END;
	u32 hw_fcount;
	bool flag_get_meta = true;
	ulong flags = 0;

	FIMC_BUG(!hw_ip);

	framemgr = hw_ip->framemgr;
	hw_fcount = atomic_read(&hw_ip->fcount);

	FIMC_BUG(!framemgr);

flush_wait_done_frame:
	framemgr_e_barrier_common(framemgr, 0, flags);
	frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
	framemgr_x_barrier_common(framemgr, 0, flags);

	if (frame) {
		u32 frame_fcount = frame->fcount;

		if (frame->num_buffers == 1)
			frame_fcount += frame->cur_buf_index;

		if (unlikely(frame_fcount < fcount)) {
			/* Flush the old frame which is in HW_WAIT_DONE state & retry. */
			mswarn_hw("queued_count(%d) [ddk:%d,hw:%d] invalid frame(F:%d,idx:%d)",
					instance_id, hw_ip,
					framemgr->queued_count[FS_HW_WAIT_DONE],
					fcount, hw_fcount,
					frame_fcount, frame->cur_buf_index);
			fimc_is_hardware_frame_ndone(hw_ip, frame, frame->instance, IS_SHOT_INVALID_FRAMENUMBER);
			goto flush_wait_done_frame;
		} else if (unlikely(frame_fcount > fcount)) {
			mswarn_hw("%s:[F%d] Too early frame. Skip it.", instance_id, hw_ip,
					__func__, frame_fcount);
			return true;
		}
	} else {
		/* Flush the old frame which is in HW_CONFIGURE state & skip dma_done. */
flush_config_frame:
		framemgr_e_barrier_common(framemgr, 0, flags);
		frame = peek_frame(framemgr, FS_HW_CONFIGURE);
		if (frame) {
			if (unlikely(frame->fcount + frame->cur_buf_index < hw_fcount)) {
				trans_frame(framemgr, frame, FS_HW_WAIT_DONE);
				framemgr_x_barrier_common(framemgr, 0, flags);
				fimc_is_hardware_frame_ndone(hw_ip, frame, frame->instance, IS_SHOT_INVALID_FRAMENUMBER);
				goto flush_config_frame;
			} else if (unlikely(frame->fcount + frame->cur_buf_index == hw_fcount)) {
				trans_frame(framemgr, frame, FS_HW_WAIT_DONE);
				framemgr_x_barrier_common(framemgr, 0, flags);
				fimc_is_hardware_frame_ndone(hw_ip, frame, frame->instance, IS_SHOT_INVALID_FRAMENUMBER);
				return true;
			}
		}
		framemgr_x_barrier_common(framemgr, 0, flags);
		mserr_hw("[F:%d,HWF:%d]check_dma_done: frame(null)!!", instance_id, hw_ip, fcount, hw_fcount);
		return true;
	}

	/*
	 * fcount: This value should be same value that is notified by host at shot time.
	 * In case of FRO or batch mode, this value also should be same between start and end.
	 */
	msdbg_hw(1, "check_dma [ddk:%d,hw:%d] frame(F:%d,idx:%d,num_buffers:%d)\n",
			instance_id, hw_ip,
			fcount, hw_fcount,
			frame->fcount, frame->cur_buf_index, frame->num_buffers);

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
		wq_id0 = WORK_30P_FDONE; /* after BDS */
		output_id0 = ENTRY_3AP;
		wq_id1 = WORK_30C_FDONE; /* before BDS */
		output_id1 = ENTRY_3AC;
		wq_id2 = WORK_30F_FDONE; /* efd output */
		output_id2 = ENTRY_3AF;
		wq_id3 = WORK_30G_FDONE; /* mrg output */
		output_id3 = ENTRY_3AG;
		wq_id4 = WORK_ME0C_FDONE; /* me output */
		output_id4 = ENTRY_MEXC;
		break;
	case DEV_HW_3AA1:
		wq_id0 = WORK_31P_FDONE;
		output_id0 = ENTRY_3AP;
		wq_id1 = WORK_31C_FDONE;
		output_id1 = ENTRY_3AC;
		wq_id2 = WORK_31F_FDONE; /* efd output */
		output_id2 = ENTRY_3AF;
		wq_id3 = WORK_31G_FDONE; /* mrg output */
		output_id3 = ENTRY_3AG;
		wq_id4 = WORK_ME1C_FDONE; /* me output */
		output_id4 = ENTRY_MEXC;
		break;
	case DEV_HW_ISP0:
		wq_id0 = WORK_I0P_FDONE; /* chunk output */
		output_id0 = ENTRY_IXP;
		wq_id1 = WORK_I0C_FDONE; /* yuv output */
		output_id1 = ENTRY_IXC;
		wq_id2 = WORK_ME0C_FDONE; /* me output */
		output_id2 = ENTRY_MEXC;
		break;
	case DEV_HW_ISP1:
		wq_id0 = WORK_I1P_FDONE;
		output_id0 = ENTRY_IXP;
		wq_id1 = WORK_I1C_FDONE;
		output_id1 = ENTRY_IXC;
		wq_id2 = WORK_ME1C_FDONE; /* me output */
		output_id2 = ENTRY_MEXC;
		break;
	case DEV_HW_TPU0:
		wq_id1 = WORK_D0C_FDONE;
		output_id1 = ENTRY_DXC;
		break;
	case DEV_HW_TPU1:
		wq_id1 = WORK_D1C_FDONE;
		output_id1 = ENTRY_DXC;
		break;
	case DEV_HW_DCP:
		wq_id0 = WORK_DC0C_FDONE; /* Master Capture */
		output_id0 = ENTRY_DC0C;

		wq_id1 = WORK_DC1C_FDONE; /* Slave Capture */
		output_id1 = ENTRY_DC1C;

		wq_id2 = WORK_DC2C_FDONE; /* Disparity */
		output_id2 = ENTRY_DC2C;

		wq_id3 = WORK_DC3C_FDONE; /* Master Sub Capture */
		output_id3 = ENTRY_DC3C;

		wq_id4 = WORK_DC4C_FDONE; /* Slave Sub Capture */
		output_id4 = ENTRY_DC4C;

		wq_id5 = WORK_DC1S_FDONE; /* Slave Input */
		output_id5 = ENTRY_DC1S;

		break;
	default:
		mserr_hw("[F:%d] invalid hw ID(%d)!!", instance_id, hw_ip, fcount, hw_ip->id);
		return ret;
		break;
	}

	if (test_bit(output_id0, &frame->out_flag)) {
		msdbg_hw(1, "[F:%d]output_id[0x%x],wq_id[0x%x]\n",
			instance_id, hw_ip, frame->fcount, output_id0, wq_id0);
		fimc_is_hardware_frame_done(hw_ip, NULL, wq_id0, output_id0,
			IS_SHOT_SUCCESS, flag_get_meta);
		ret = true;
		flag_get_meta = false;
	}

	if (test_bit(output_id1, &frame->out_flag)) {
		msdbg_hw(1, "[F:%d]output_id[0x%x],wq_id[0x%x]\n",
			instance_id, hw_ip, frame->fcount, output_id1, wq_id1);
		fimc_is_hardware_frame_done(hw_ip, NULL, wq_id1, output_id1,
			IS_SHOT_SUCCESS, flag_get_meta);
		ret = true;
		flag_get_meta = false;
	}

	if (test_bit(output_id2, &frame->out_flag)) {
		msdbg_hw(1, "[F:%d]output_id[0x%x],wq_id[0x%x]\n",
			instance_id, hw_ip, frame->fcount, output_id2, wq_id2);
		fimc_is_hardware_frame_done(hw_ip, NULL, wq_id2, output_id2,
			IS_SHOT_SUCCESS, flag_get_meta);
		ret = true;
		flag_get_meta = false;
	}

	if (test_bit(output_id3, &frame->out_flag)) {
		msdbg_hw(1, "[F:%d]output_id[0x%x],wq_id[0x%x]\n",
			instance_id, hw_ip, frame->fcount, output_id3, wq_id3);
		fimc_is_hardware_frame_done(hw_ip, NULL, wq_id3, output_id3,
			IS_SHOT_SUCCESS, flag_get_meta);
		ret = true;
		flag_get_meta = false;
	}

	if (test_bit(output_id4, &frame->out_flag)) {
		msdbg_hw(1, "[F:%d]output_id[0x%x],wq_id[0x%x]\n",
			instance_id, hw_ip, frame->fcount, output_id4, wq_id4);
		fimc_is_hardware_frame_done(hw_ip, NULL, wq_id4, output_id4,
			IS_SHOT_SUCCESS, flag_get_meta);
		ret = true;
		flag_get_meta = false;
	}

	if (test_bit(output_id5, &frame->out_flag)) {
		msdbg_hw(1, "[F:%d]output_id[0x%x],wq_id[0x%x]\n",
			instance_id, hw_ip, frame->fcount, output_id5, wq_id5);
		fimc_is_hardware_frame_done(hw_ip, NULL, wq_id5, output_id5,
			IS_SHOT_SUCCESS, flag_get_meta);
		ret = true;
		flag_get_meta = false;
	}

	return ret;
}

static void fimc_is_lib_io_callback(void *this, enum lib_cb_event_type event_id,
	u32 instance_id)
{
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_ip *hw_ip;
	int wq_id = WORK_MAX_MAP;
	int output_id = ENTRY_END;
	u32 hw_fcount, index;
#if defined(ENABLE_FULLCHAIN_OVERFLOW_RECOVERY)
	int ret = 0;
#endif

	FIMC_BUG_VOID(!this);

	hw_ip = (struct fimc_is_hw_ip *)this;
	hardware = hw_ip->hardware;
	hw_fcount = atomic_read(&hw_ip->fcount);

	FIMC_BUG_VOID(!hw_ip->hardware);

	switch (event_id) {
	case LIB_EVENT_DMA_A_OUT_DONE:
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance_id]]))
			msinfo_hw("[F:%d]DMA A\n", instance_id, hw_ip,
				hw_fcount);
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_DMA_END] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_DMA_END] = local_clock();
#if defined(SOC_3AAISP)
		wq_id = WORK_30C_FDONE;
		output_id = ENTRY_3AC;
#else
		switch (hw_ip->id) {
		case DEV_HW_3AA0: /* after BDS */
			wq_id = WORK_30P_FDONE;
			output_id = ENTRY_3AP;
			break;
		case DEV_HW_3AA1:
			wq_id = WORK_31P_FDONE;
			output_id = ENTRY_3AP;
			break;
		case DEV_HW_ISP0: /* chunk output */
			wq_id = WORK_I0P_FDONE;
			output_id = ENTRY_IXP;
			break;
		case DEV_HW_ISP1:
			wq_id = WORK_I1P_FDONE;
			output_id = ENTRY_IXP;
			break;
		default:
			err_hw("[%d] invalid hw ID(%d)!!", instance_id, hw_ip->id);
			goto p_err;
		}
#endif

		if (wq_id != WORK_MAX_MAP)
			fimc_is_hardware_frame_done(hw_ip, NULL, wq_id,
					output_id, IS_SHOT_SUCCESS, true);
		break;
	case LIB_EVENT_DMA_B_OUT_DONE:
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance_id]]))
			msinfo_hw("[F:%d]DMA B\n", instance_id, hw_ip,
				hw_fcount);
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_DMA_END] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_DMA_END] = local_clock();
		switch (hw_ip->id) {
		case DEV_HW_3AA0: /* before BDS */
			wq_id = WORK_30C_FDONE;
			output_id = ENTRY_3AC;
			break;
		case DEV_HW_3AA1:
			wq_id = WORK_31C_FDONE;
			output_id = ENTRY_3AC;
			break;
		case DEV_HW_ISP0: /* yuv output */
			wq_id = WORK_I0C_FDONE;
			output_id = ENTRY_IXC;
			break;
		case DEV_HW_ISP1:
			wq_id = WORK_I1C_FDONE;
			output_id = ENTRY_IXC;
			break;
		default:
			err_hw("[%d] invalid hw ID(%d)!!", instance_id, hw_ip->id);
			goto p_err;
		}

		if (wq_id != WORK_MAX_MAP)
			fimc_is_hardware_frame_done(hw_ip, NULL, wq_id,
					output_id, IS_SHOT_SUCCESS, true);
		break;
	case LIB_EVENT_ERROR_CIN_OVERFLOW:
		fimc_is_debug_event_count(FIMC_IS_EVENT_OVERFLOW_3AA);
		exynos_bcm_dbg_stop(CAMERA_DRIVER);
		msinfo_hw("LIB_EVENT_ERROR_CIN_OVERFLOW\n", instance_id, hw_ip);
		fimc_is_hardware_flush_frame(hw_ip, FS_HW_CONFIGURE, IS_SHOT_OVERFLOW);

#if defined(ENABLE_FULLCHAIN_OVERFLOW_RECOVERY)
		ret = fimc_is_hw_overflow_recovery();
		if (ret < 0)
			panic("OVERFLOW recovery fail!!!!");
#elif defined(OVERFLOW_PANIC_ENABLE_ISCHAIN)
		panic("CIN OVERFLOW!!");
#endif
		break;
	default:
		msinfo_hw("event_id(%d)\n", instance_id, hw_ip, event_id);
		break;
	}

p_err:
	return;
};

static void fimc_is_lib_camera_callback(void *this, enum lib_cb_event_type event_id,
	u32 instance_id, void *data)
{
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_ip *hw_ip;
	ulong fcount;
	u32 hw_fcount, index;
	bool ret = false;
	bool frame_done = false;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	ulong flags = 0;

	FIMC_BUG_VOID(!this);

	hw_ip = (struct fimc_is_hw_ip *)this;
	hardware = hw_ip->hardware;
	hw_fcount = atomic_read(&hw_ip->fcount);

	FIMC_BUG_VOID(!hw_ip->hardware);

	if (test_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag)) {
		err_hw("[ID:%d] During recovery : invalid interrupt", hw_ip->id);
		return;
	}

	switch (event_id) {
	case LIB_EVENT_CONFIG_LOCK:
		fcount = (ulong)data;
		atomic_add(hw_ip->num_buffers, &hw_ip->count.cl);
		if (unlikely(!atomic_read(&hardware->streaming[hardware->sensor_position[instance_id]])))
			msinfo_hw("[F:%d]C.L %d\n", instance_id, hw_ip,
				hw_fcount, (u32)fcount);

		/* fcount : frame number of current frame in Vvalid */
		fimc_is_hardware_config_lock(hw_ip, instance_id, (u32)fcount);
		break;
	case LIB_EVENT_FRAME_START_ISR:
		if (sysfs_debug.pattern_en && (hw_ip->id == DEV_HW_3AA0 || hw_ip->id == DEV_HW_3AA1)) {
			struct fimc_is_group *group;
			struct v4l2_subdev *subdev;
			struct fimc_is_device_csi *csi;

			group = hw_ip->group[instance_id];
			if (IS_ERR_OR_NULL(group)) {
				mserr_hw("group is NULL", instance_id, hw_ip);
				return;
			}

			if (IS_ERR_OR_NULL(group->device)) {
				mserr_hw("device is NULL", instance_id, hw_ip);
				return;
			}

			if (IS_ERR_OR_NULL(group->device->sensor)) {
				mserr_hw("sensor is NULL", instance_id, hw_ip);
				return;
			}

			if (IS_ERR_OR_NULL(group->device->sensor->subdev_csi)) {
				mserr_hw("subdev_csi is NULL", instance_id, hw_ip);
				return;
			}

			subdev = group->device->sensor->subdev_csi;
			csi = v4l2_get_subdevdata(subdev);
			if (IS_ERR_OR_NULL(csi)) {
				mserr_hw("csi is NULL", instance_id, hw_ip);
				return;
			}

			csi_frame_start_inline(csi);
		}

		hw_ip->debug_index[1] = hw_ip->debug_index[0] % DEBUG_FRAME_COUNT;
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].fcount = hw_ip->debug_index[0];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_START] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_START] = local_clock();
		if (unlikely(!atomic_read(&hardware->streaming[hardware->sensor_position[instance_id]])
			|| debug_irq_ddk))
			msinfo_hw("[F:%d]F.S\n", instance_id, hw_ip,
				hw_fcount);

		fimc_is_hardware_frame_start(hw_ip, instance_id);
		atomic_add(hw_ip->num_buffers, &hw_ip->count.fs);
		break;
	case LIB_EVENT_FRAME_END:
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_END] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_END] = local_clock();
		atomic_add(hw_ip->num_buffers, &hw_ip->count.fe);
		if (unlikely(!atomic_read(&hardware->streaming[hardware->sensor_position[instance_id]])
			|| debug_irq_ddk))
			msinfo_hw("[F:%d]F.E\n", instance_id, hw_ip,
				hw_fcount);

		if (unlikely(debug_time_hw)) {
			msinfo_hw("[TIM] S-E %05llu us\n", instance_id, hw_ip,
				(hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_END] -
				hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_START]) / 1000);
		}

		fcount = (ulong)data;
		fimc_is_hw_g_ctrl(hw_ip, hw_ip->id, HW_G_CTRL_FRM_DONE_WITH_DMA, (void *)&frame_done);
		if (frame_done)
			ret = check_dma_done(hw_ip, instance_id, (u32)fcount);
		else
			msdbg_hw(1, "dma done interupt separate\n", instance_id, hw_ip);

		if (!ret) {
			fimc_is_hardware_frame_done(hw_ip, NULL, -1, FIMC_IS_HW_CORE_END,
				IS_SHOT_SUCCESS, true);
		}
		atomic_set(&hw_ip->status.Vvalid, V_BLANK);
		wake_up(&hw_ip->status.wait_queue);

		CALL_HW_OPS(hw_ip, clk_gate, instance_id, false, false);
		break;
	case LIB_EVENT_ERROR_CONFIG_LOCK_DELAY:
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_END] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_END] = local_clock();
		atomic_add(hw_ip->num_buffers, &hw_ip->count.fe);

		framemgr = hw_ip->framemgr;
		framemgr_e_barrier_common(framemgr, 0, flags);
		frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
		framemgr_x_barrier_common(framemgr, 0, flags);

		if (frame) {
			if (fimc_is_hardware_frame_ndone(hw_ip, frame, frame->instance,
						IS_SHOT_CONFIG_LOCK_DELAY)) {
				mserr_hw("failure in hardware_frame_ndone",
						frame->instance, hw_ip);
			}
		} else {
			serr_hw("[F:%lu]camera_callback: frame(null)!!(E%d)", hw_ip,
				(ulong)data, event_id);
		}

		atomic_set(&hw_ip->status.Vvalid, V_BLANK);
		wake_up(&hw_ip->status.wait_queue);

		CALL_HW_OPS(hw_ip, clk_gate, instance_id, false, false);
		break;
	default:
		break;
	}

	return;
};

struct lib_callback_func fimc_is_lib_cb_func = {
	.camera_callback	= fimc_is_lib_camera_callback,
	.io_callback		= fimc_is_lib_io_callback,
};

int fimc_is_lib_isp_chain_create(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id)
{
	int ret = 0;
	u32 chain_id;
	ulong base_addr, base_addr_b, set_b_offset;
	struct lib_system_config config;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!this);
	FIMC_BUG(!this->func);

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
	case DEV_HW_ISP0:
	case DEV_HW_TPU0:
	case DEV_HW_DCP:
		chain_id = 0;
		break;
	case DEV_HW_3AA1:
	case DEV_HW_ISP1:
	case DEV_HW_TPU1:
		chain_id = 1;
		break;
	default:
		err_lib("invalid hw (%d)", hw_ip->id);
		return -EINVAL;
		break;
	}

	base_addr    = (ulong)hw_ip->regs;
	base_addr_b  = (ulong)hw_ip->regs_b;
	set_b_offset = (base_addr_b < base_addr) ? 0 : base_addr_b - base_addr;

	ret = CALL_LIBOP(this, chain_create, chain_id, base_addr, set_b_offset,
				&fimc_is_lib_cb_func);
	if (ret) {
		err_lib("chain_create fail (%d)", hw_ip->id);
		return -EINVAL;
	}

	/* set system config */
	memset(&config, 0, sizeof(struct lib_system_config));
	config.overflow_recovery = DDK_OVERFLOW_RECOVERY;

	ret = CALL_LIBOP(this, set_system_config, &config);
	if (ret) {
		err_lib("set_system_config fail (%d)", hw_ip->id);
		return -EINVAL;
	}

	msinfo_lib("chain_create done [reg_base:0x%lx][b_offset:0x%lx](%d)\n",
		instance_id, hw_ip, base_addr, set_b_offset, config.overflow_recovery);

	return ret;
}

int fimc_is_lib_isp_object_create(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id, u32 rep_flag, u32 module_id)
{
	int ret = 0;
	u32 chain_id, input_type, obj_info = 0;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!this);
	FIMC_BUG(!this->func);
	FIMC_BUG(!hw_ip->group[instance_id]);

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
	case DEV_HW_ISP0:
	case DEV_HW_TPU0:
	case DEV_HW_DCP:
		chain_id = 0;
		break;
	case DEV_HW_3AA1:
	case DEV_HW_ISP1:
	case DEV_HW_TPU1:
		chain_id = 1;
		break;
	default:
		err_lib("invalid hw (%d)", hw_ip->id);
		return -EINVAL;
		break;
	}

	/* input_type : use only in 3AA (guide by DDK) */
	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &hw_ip->group[instance_id]->state))
		input_type = 0; /* default */
	else
		input_type = 1;

	obj_info |= chain_id << CHAIN_ID_SHIFT;
	obj_info |= instance_id << INSTANCE_ID_SHIFT;
	obj_info |= rep_flag << REPROCESSING_FLAG_SHIFT;
	obj_info |= (input_type) << INPUT_TYPE_SHIFT;
	obj_info |= (module_id) << MODULE_ID_SHIFT;

	info_lib("obj_create: chain(%d), instance(%d), rep(%d), in_type(%d),"
		" obj_info(0x%08x), module_id(%d)\n",
		chain_id, instance_id, rep_flag, input_type, obj_info, module_id);

	ret = CALL_LIBOP(this, object_create, &this->object, obj_info, hw_ip);
	if (ret) {
		err_lib("object_create fail (%d)", hw_ip->id);
		return -EINVAL;
	}

	msinfo_lib("object_create done\n", instance_id, hw_ip);

	return ret;
}

void fimc_is_lib_isp_chain_destroy(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id)
{
	int ret = 0;
	u32 chain_id;

	FIMC_BUG_VOID(!hw_ip);
	FIMC_BUG_VOID(!this->func);

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
	case DEV_HW_ISP0:
	case DEV_HW_TPU0:
	case DEV_HW_DCP:
		chain_id = 0;
		break;
	case DEV_HW_3AA1:
	case DEV_HW_ISP1:
	case DEV_HW_TPU1:
		chain_id = 1;
		break;
	default:
		err_lib("invalid hw (%d)", hw_ip->id);
		return;
		break;
	}

	ret = CALL_LIBOP(this, chain_destroy, chain_id);
	if (ret) {
		err_lib("chain_destroy fail (%d)", hw_ip->id);
		return;
	}
	msinfo_lib("chain_destroy done\n", instance_id, hw_ip);

	return;
}

void fimc_is_lib_isp_object_destroy(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id)
{
	int ret = 0;

	FIMC_BUG_VOID(!hw_ip);
	FIMC_BUG_VOID(!this);
	FIMC_BUG_VOID(!this->func);

	if (!this->object) {
		err_lib("object(NULL) destroy fail (%d)", hw_ip->id);
		return;
	}

	ret = CALL_LIBOP(this, object_destroy, this->object, instance_id);
	if (ret) {
		err_lib("object_destroy fail (%d)", hw_ip->id);
		return;
	}

	msinfo_lib("object_destroy done\n", instance_id, hw_ip);

	return;
}

int fimc_is_lib_isp_set_param(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, void *param)
{
	int ret;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!this);
	FIMC_BUG(!this->func);
	FIMC_BUG(!this->object);

	ret = CALL_LIBOP(this, set_param, this->object, param);
	if (ret)
		mserr_lib("set_param fail", atomic_read(&hw_ip->instance), hw_ip);

	return ret;
}

int fimc_is_lib_isp_set_ctrl(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, struct fimc_is_frame *frame)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!this);
	FIMC_BUG(!frame);
	FIMC_BUG(!this->func);
	FIMC_BUG(!this->object);

	ret = CALL_LIBOP(this, set_ctrl, this->object, frame->instance,
				frame->fcount, frame->shot);
	if (ret)
		mserr_lib("set_ctrl fail", atomic_read(&hw_ip->instance), hw_ip);

	return 0;
}

void fimc_is_lib_isp_shot(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, void *param_set, struct camera2_shot *shot)
{
	int ret = 0;

	FIMC_BUG_VOID(!hw_ip);
	FIMC_BUG_VOID(!this);
	FIMC_BUG_VOID(!param_set);
	FIMC_BUG_VOID(!this->func);
	FIMC_BUG_VOID(!this->object);

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
	case DEV_HW_3AA1:
		ret = CALL_LIBOP(this, shot, this->object,
					(struct taa_param_set *)param_set,
					shot, hw_ip->num_buffers);
		if (ret)
			err_lib("3aa shot fail (%d)", hw_ip->id);

		break;
	case DEV_HW_ISP0:
	case DEV_HW_ISP1:
		ret = CALL_LIBOP(this, shot, this->object,
					(struct isp_param_set *)param_set,
					shot, hw_ip->num_buffers);
		if (ret)
			err_lib("isp shot fail (%d)", hw_ip->id);
		break;
	case DEV_HW_TPU0:
	case DEV_HW_TPU1:
		ret = CALL_LIBOP(this, shot, this->object,
					(struct tpu_param_set *)param_set,
					shot, hw_ip->num_buffers);
		if (ret)
			err_lib("tpu shot fail (%d)", hw_ip->id);
		break;
	case DEV_HW_DCP:
		ret = CALL_LIBOP(this, shot, this->object,
					(struct dcp_param_set *)param_set,
					shot, hw_ip->num_buffers);
		if (ret)
			err_lib("dcp shot fail (%d)", hw_ip->id);
		break;
	default:
		err_lib("invalid hw (%d)", hw_ip->id);
		break;
	}
}

int fimc_is_lib_isp_get_meta(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, struct fimc_is_frame *frame)

{
	int ret = 0;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!this);
	FIMC_BUG(!frame);
	FIMC_BUG(!this->func);
	FIMC_BUG(!this->object);
	FIMC_BUG(!frame->shot);

	ret = CALL_LIBOP(this, get_meta, this->object, frame->instance,
				frame->fcount, frame->shot);
	if (ret)
		mserr_lib("get_meta fail", atomic_read(&hw_ip->instance), hw_ip);

	return ret;
}

void fimc_is_lib_isp_stop(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id)
{
	int ret = 0;

	FIMC_BUG_VOID(!hw_ip);
	FIMC_BUG_VOID(!this);
	FIMC_BUG_VOID(!this->func);
	FIMC_BUG_VOID(!this->object);

	ret = CALL_LIBOP(this, stop, this->object, instance_id);
	if (ret) {
		err_lib("object_suspend fail (%d)", hw_ip->id);
		return;
	}
	msinfo_lib("object_suspend done\n", instance_id, hw_ip);

	return;
}

int fimc_is_lib_isp_create_tune_set(struct fimc_is_lib_isp *this,
	ulong addr, u32 size, u32 index, int flag, u32 instance_id)
{
	int ret = 0;
	struct lib_tune_set tune_set;

	FIMC_BUG(!this);
	FIMC_BUG(!this->func);
	FIMC_BUG(!this->object);

	tune_set.index = index;
	tune_set.addr = addr;
	tune_set.size = size;
	tune_set.decrypt_flag = flag;

	ret = CALL_LIBOP(this, create_tune_set, this->object, instance_id,
				&tune_set);
	if (ret) {
		err_lib("create_tune_set fail (%d)", ret);
		return ret;
	}

	minfo_lib("create_tune_set index(%d)\n", instance_id, index);

	return ret;
}

int fimc_is_lib_isp_apply_tune_set(struct fimc_is_lib_isp *this,
	u32 index, u32 instance_id)
{
	int ret = 0;

	FIMC_BUG(!this);
	FIMC_BUG(!this->func);
	FIMC_BUG(!this->object);

	ret = CALL_LIBOP(this, apply_tune_set, this->object, instance_id, index);
	if (ret) {
		err_lib("apply_tune_set fail (%d)", ret);
		return ret;
	}

	return ret;
}

int fimc_is_lib_isp_delete_tune_set(struct fimc_is_lib_isp *this,
	u32 index, u32 instance_id)
{
	int ret = 0;

	FIMC_BUG(!this);
	FIMC_BUG(!this->func);
	FIMC_BUG(!this->object);

	ret = CALL_LIBOP(this, delete_tune_set, this->object, instance_id, index);
	if (ret) {
		err_lib("delete_tune_set fail (%d)", ret);
		return ret;
	}

	minfo_lib("delete_tune_set index(%d)\n", instance_id, index);

	return ret;
}

int fimc_is_lib_isp_load_cal_data(struct fimc_is_lib_isp *this,
	u32 instance_id, ulong addr)
{
	char version[32];
	int ret = 0;

	FIMC_BUG(!this);
	FIMC_BUG(!this->func);
	FIMC_BUG(!this->object);

	memcpy(version, (void *)(addr + 0x20), (FIMC_IS_CAL_VER_SIZE - 1));
	version[FIMC_IS_CAL_VER_SIZE] = '\0';
	minfo_lib("CAL version: %s\n", instance_id, version);

	ret = CALL_LIBOP(this, load_cal_data, this->object, instance_id, addr);
	if (ret) {
		err_lib("apply_tune_set fail (%d)", ret);
		return ret;
	}

	return ret;
}

int fimc_is_lib_isp_get_cal_data(struct fimc_is_lib_isp *this,
	u32 instance_id, struct cal_info *c_info, int type)
{
	int ret = 0;

	FIMC_BUG(!this);
	FIMC_BUG(!this->func);
	FIMC_BUG(!this->object);

	ret = CALL_LIBOP(this, get_cal_data, this->object, instance_id,
				c_info, type);
	if (ret) {
		err_lib("apply_tune_set fail (%d)", ret);
		return ret;
	}
	dbg_lib(3, "%s: data(%d,%d,%d,%d)\n",
		__func__, c_info->data[0], c_info->data[1], c_info->data[2], c_info->data[3]);

	return ret;
}

int fimc_is_lib_isp_sensor_info_mode_chg(struct fimc_is_lib_isp *this,
	u32 instance_id, struct camera2_shot *shot)
{
	int ret = 0;

	FIMC_BUG(!this);
	FIMC_BUG(!shot);
	FIMC_BUG(!this->func);
	FIMC_BUG(!this->object);

	ret = CALL_LIBOP(this, sensor_info_mode_chg, this->object, instance_id,
				shot);
	if (ret) {
		err_lib("sensor_info_mode_chg fail (%d)", ret);
		return ret;
	}

	return ret;
}

int fimc_is_lib_isp_sensor_update_control(struct fimc_is_lib_isp *this,
	u32 instance_id, u32 frame_count, struct camera2_shot *shot)
{
	int ret = 0;

	FIMC_BUG(!this);
	FIMC_BUG(!this->func);
	FIMC_BUG(!this->object);

	ret = CALL_LIBOP(this, sensor_update_ctl, this->object, instance_id,
				frame_count, shot);
	if (ret) {
		err_lib("sensor_update_ctl fail (%d)", ret);
		return ret;
	}

	return ret;
}

int fimc_is_lib_isp_reset_recovery(struct fimc_is_hw_ip *hw_ip,
		struct fimc_is_lib_isp *this, u32 instance_id)
{
	int ret = 0;

	BUG_ON(!hw_ip);
	BUG_ON(!this->func);

	ret = CALL_LIBOP(this, recovery, instance_id);
	if (ret) {
		err_lib("chain_reset_recovery fail (%d)", hw_ip->id);
		return ret;
	}
	msinfo_lib("chain_reset_recovery done\n", instance_id, hw_ip);

	return ret;
}

int fimc_is_lib_isp_convert_face_map(struct fimc_is_hardware *hardware,
	struct taa_param_set *param_set, struct fimc_is_frame *frame)
{
	int ret = 0;
#ifdef ENABLE_VRA
	int i;
	u32 fd_width = 0, fd_height = 0;
	u32 bayer_crop_width, bayer_crop_height;
	struct fimc_is_group *group = NULL;
	struct fimc_is_group *group_vra;
	struct camera2_shot *shot = NULL;
	struct fimc_is_device_ischain *device = NULL;
	struct param_otf_input *fd_otf_input;
	struct param_dma_input *fd_dma_input;

	FIMC_BUG(!hardware);
	FIMC_BUG(!param_set);
	FIMC_BUG(!frame);

	shot = frame->shot;
	FIMC_BUG(!shot);

	group = (struct fimc_is_group *)frame->group;
	FIMC_BUG(!group);

	device = group->device;
	FIMC_BUG(!device);

	if (shot->uctl.fdUd.faceDetectMode == FACEDETECT_MODE_OFF)
		return 0;

	/*
	 * The face size which an algorithm uses is determined
	 * by the input bayer crop size of 3aa.
	 */
	if (param_set->otf_input.cmd == OTF_INPUT_COMMAND_ENABLE) {
		bayer_crop_width = param_set->otf_input.bayer_crop_width;
		bayer_crop_height = param_set->otf_input.bayer_crop_height;
	} else if (param_set->dma_input.cmd == DMA_INPUT_COMMAND_ENABLE) {
		bayer_crop_width = param_set->dma_input.bayer_crop_width;
		bayer_crop_height = param_set->dma_input.bayer_crop_height;
	} else {
		err_hw("invalid hw input!!\n");
		return -EINVAL;
	}

	if (bayer_crop_width == 0 || bayer_crop_height == 0) {
		warn_hw("%s: invalid crop size (%d * %d)!!",
			__func__, bayer_crop_width, bayer_crop_height);
		return 0;
	}

	/* The face size is determined by the fd input size */
	group_vra = &device->group_vra;
	if (test_bit(FIMC_IS_GROUP_INIT, &group_vra->state)
		&& (!test_bit(FIMC_IS_GROUP_OTF_INPUT, &group_vra->state))) {

		fd_dma_input = fimc_is_itf_g_param(device, frame, PARAM_FD_DMA_INPUT);
		if (fd_dma_input->cmd == DMA_INPUT_COMMAND_ENABLE) {
			fd_width = fd_dma_input->width;
			fd_height = fd_dma_input->height;
		}
	} else {
		fd_otf_input = fimc_is_itf_g_param(device, frame, PARAM_FD_OTF_INPUT);
		if (fd_otf_input->cmd == OTF_INPUT_COMMAND_ENABLE) {
			fd_width = fd_otf_input->width;
			fd_height = fd_otf_input->height;
		}
	}

	if (fd_width == 0 || fd_height == 0) {
		warn_hw("%s: invalid fd size (%d * %d)!!",
			__func__, fd_width, fd_height);
		return 0;
	}

	/* Convert face size */
	for (i = 0; i < CAMERA2_MAX_FACES; i++) {
		if (shot->uctl.fdUd.faceScores[i] == 0)
			continue;

		shot->uctl.fdUd.faceRectangles[i][0] =
				CONVRES(shot->uctl.fdUd.faceRectangles[i][0],
				fd_width, bayer_crop_width);
		shot->uctl.fdUd.faceRectangles[i][1] =
				CONVRES(shot->uctl.fdUd.faceRectangles[i][1],
				fd_height, bayer_crop_height);
		shot->uctl.fdUd.faceRectangles[i][2] =
				CONVRES(shot->uctl.fdUd.faceRectangles[i][2],
				fd_width, bayer_crop_width);
		shot->uctl.fdUd.faceRectangles[i][3] =
				CONVRES(shot->uctl.fdUd.faceRectangles[i][3],
				fd_height, bayer_crop_height);

		dbg_lib(3, "%s: ID(%d), x_min(%d), y_min(%d), x_max(%d), y_max(%d)\n",
			__func__, shot->uctl.fdUd.faceIds[i],
			shot->uctl.fdUd.faceRectangles[i][0],
			shot->uctl.fdUd.faceRectangles[i][1],
			shot->uctl.fdUd.faceRectangles[i][2],
			shot->uctl.fdUd.faceRectangles[i][3]);
	}

#endif
	return ret;
}

void fimc_is_isp_get_bcrop1_size(void __iomem *base_addr, u32 *width, u32 *height)
{
	*width  = fimc_is_hw_get_field(base_addr, &isp_regs[ISP_R_BCROP1_SIZE_X], &isp_fields[ISP_F_BCROP1_SIZE_X]);
	*height = fimc_is_hw_get_field(base_addr, &isp_regs[ISP_R_BCROP1_SIZE_Y], &isp_fields[ISP_F_BCROP1_SIZE_Y]);
}
