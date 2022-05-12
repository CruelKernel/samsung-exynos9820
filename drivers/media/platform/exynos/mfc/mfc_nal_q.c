/*
 * drivers/media/platform/exynos/mfc/mfc_nal_q.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "mfc_nal_q.h"

#include "mfc_sync.h"

#include "mfc_pm.h"
#include "mfc_reg_api.h"
#include "mfc_hw_reg_api.h"

#include "mfc_qos.h"
#include "mfc_queue.h"
#include "mfc_utils.h"
#include "mfc_buf.h"
#include "mfc_mem.h"

#define CBR_I_LIMIT_MAX			5
int mfc_nal_q_check_enable(struct mfc_dev *dev)
{
	struct mfc_ctx *temp_ctx;
	struct mfc_dec *dec = NULL;
	struct mfc_enc *enc = NULL;
	struct mfc_enc_params *p = NULL;
	int i;

	mfc_debug_enter();

	if (nal_q_disable)
		return 0;

	for (i = 0; i < MFC_NUM_CONTEXTS; i++) {
		temp_ctx = dev->ctx[i];
		if (temp_ctx) {
			/* NAL-Q doesn't support drm */
			if (temp_ctx->is_drm) {
				mfc_debug(2, "There is a drm ctx. Can't start NAL-Q\n");
				return 0;
			}
			/* NAL-Q can be enabled when all ctx are in running state */
			if (temp_ctx->state != MFCINST_RUNNING) {
				mfc_debug(2, "There is a ctx which is not in running state. "
						"index: %d, state: %d\n", i, temp_ctx->state);
				return 0;
			}
			/* NAL-Q can't use the command about last frame */
			if (mfc_check_buf_vb_flag(temp_ctx, MFC_FLAG_LAST_FRAME) == 1) {
				mfc_debug(2, "There is a last frame. index: %d\n", i);
				return 0;
			}
			/* NAL-Q doesn't support OTF mode */
			if (temp_ctx->otf_handle) {
				mfc_debug(2, "There is a OTF node\n");
				return 0;
			}
			/* NAL-Q doesn't support BPG */
			if (IS_BPG_DEC(temp_ctx) || IS_BPG_ENC(temp_ctx)) {
				mfc_debug(2, "BPG codec type\n");
				return 0;
			}
			/* NAL-Q doesn't support multi-frame, interlaced, black bar */
			if (temp_ctx->type == MFCINST_DECODER) {
				dec = temp_ctx->dec_priv;
				if (!dec) {
					mfc_debug(2, "There is no dec\n");
					return 0;
				}
				if ((dec->has_multiframe && CODEC_MULTIFRAME(temp_ctx)) || dec->consumed) {
					mfc_debug(2, "[MULTIFRAME] There is a multi frame or consumed header\n");
					return 0;
				}
				if (dec->is_dpb_full) {
					mfc_debug(2, "[DPB] All buffers are referenced\n");
					return 0;
				}
				if (dec->is_interlaced) {
					mfc_debug(2, "[INTERLACE] There is a interlaced stream\n");
					return 0;
				}
				if (dec->detect_black_bar) {
					mfc_debug(2, "[BLACKBAR] black bar detection is enabled\n");
					return 0;
				}
			/* NAL-Q doesn't support fixed byte(slice mode), CBR_VT(rc mode) */
			} else if (temp_ctx->type == MFCINST_ENCODER) {
				enc = temp_ctx->enc_priv;
				if (!enc) {
					mfc_debug(2, "There is no enc\n");
					return 0;
				}
				if (enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_FIXED_BYTES) {
					mfc_debug(2, "There is fixed bytes option(slice mode)\n");
					return 0;
				}
				p = &enc->params;
				if (p->rc_reaction_coeff <= CBR_I_LIMIT_MAX) {
					mfc_debug(2, "There is CBR_VT option(rc mode)\n");
					return 0;
				}
			}
			mfc_debug(2, "There is a ctx in running state. index: %d\n", i);
		}
	}

	mfc_debug(2, "All working ctx are in running state!\n");

	mfc_debug_leave();

	return 1;
}

void mfc_nal_q_clock_on(struct mfc_dev *dev, nal_queue_handle *nal_q_handle)
{
	unsigned long flags;

	mfc_debug_enter();

	spin_lock_irqsave(&nal_q_handle->lock, flags);

	mfc_debug(2, "[NALQ] continue_clock_on = %d, nal_q_clk_cnt = %d\n",
			dev->continue_clock_on, nal_q_handle->nal_q_clk_cnt);

	if (!dev->continue_clock_on && !nal_q_handle->nal_q_clk_cnt)
		mfc_pm_clock_on(dev);

	nal_q_handle->nal_q_clk_cnt++;
	dev->continue_clock_on = false;

	mfc_debug(2, "[NALQ] nal_q_clk_cnt = %d\n", nal_q_handle->nal_q_clk_cnt);

	spin_unlock_irqrestore(&nal_q_handle->lock, flags);

	mfc_debug_leave();
}

void mfc_nal_q_clock_off(struct mfc_dev *dev, nal_queue_handle *nal_q_handle)
{
	unsigned long flags;

	mfc_debug_enter();

	spin_lock_irqsave(&nal_q_handle->lock, flags);

	mfc_debug(2, "[NALQ] nal_q_clk_cnt = %d\n", nal_q_handle->nal_q_clk_cnt);

	if (!nal_q_handle->nal_q_clk_cnt) {
		spin_unlock_irqrestore(&nal_q_handle->lock, flags);
		mfc_err_dev("[NALQ] nal_q_clk_cnt is already zero\n");
		return;
	}

	nal_q_handle->nal_q_clk_cnt--;

	if (!nal_q_handle->nal_q_clk_cnt)
		mfc_pm_clock_off(dev);

	mfc_debug(2, "[NALQ] nal_q_clk_cnt = %d\n", nal_q_handle->nal_q_clk_cnt);

	spin_unlock_irqrestore(&nal_q_handle->lock, flags);

	mfc_debug_leave();
}

void mfc_nal_q_cleanup_clock(struct mfc_dev *dev)
{
	unsigned long flags;

	mfc_debug_enter();

	spin_lock_irqsave(&dev->nal_q_handle->lock, flags);

	dev->nal_q_handle->nal_q_clk_cnt = 0;

	spin_unlock_irqrestore(&dev->nal_q_handle->lock, flags);

	mfc_debug_leave();
}

static int __mfc_nal_q_find_ctx(struct mfc_dev *dev, EncoderOutputStr *pOutputStr)
{
	int i;

	for(i = 0; i < MFC_NUM_CONTEXTS; i++) {
		if (dev->ctx[i] && dev->ctx[i]->inst_no == pOutputStr->InstanceId)
			return i;
	}
	return -1;
}

static nal_queue_in_handle* __mfc_nal_q_create_in_q(struct mfc_dev *dev,
		nal_queue_handle *nal_q_handle)
{
	nal_queue_in_handle *nal_q_in_handle;

	mfc_debug_enter();

	nal_q_in_handle = kzalloc(sizeof(*nal_q_in_handle), GFP_KERNEL);
	if (!nal_q_in_handle) {
		mfc_err_dev("[NALQ] Failed to get memory for nal_queue_in_handle\n");
		return NULL;
	}

	nal_q_in_handle->nal_q_handle = nal_q_handle;
	nal_q_in_handle->in_buf.buftype = MFCBUF_NORMAL;
	/*
	 * Total nal_q buf size = entry size * num slot * max instance
	 * ex) entry size is 512 byte
	 *     512 byte * 4 slot * 32 instance = 64KB
	 * Plus 1 is needed for margin, because F/W exceeds sometimes.
	 */
	nal_q_in_handle->in_buf.size = dev->pdata->nal_q_entry_size * (NAL_Q_QUEUE_SIZE + 1);
	if (mfc_mem_ion_alloc(dev, &nal_q_in_handle->in_buf)) {
		mfc_err_dev("[NALQ] failed to get memory\n");
		kfree(nal_q_in_handle);
		return NULL;
	}
	nal_q_in_handle->nal_q_in_addr = nal_q_in_handle->in_buf.vaddr;

	mfc_debug_leave();

	return nal_q_in_handle;
}

static nal_queue_out_handle* __mfc_nal_q_create_out_q(struct mfc_dev *dev,
		nal_queue_handle *nal_q_handle)
{
	nal_queue_out_handle *nal_q_out_handle;

	mfc_debug_enter();

	nal_q_out_handle = kzalloc(sizeof(*nal_q_out_handle), GFP_KERNEL);
	if (!nal_q_out_handle) {
		mfc_err_dev("[NALQ] failed to get memory for nal_queue_out_handle\n");
		return NULL;
	}

	nal_q_out_handle->nal_q_handle = nal_q_handle;
	nal_q_out_handle->out_buf.buftype = MFCBUF_NORMAL;
	/*
	 * Total nal_q buf size = entry size * num slot * max instance
	 * ex) entry size is 512 byte
	 *     512 byte * 4 slot * 32 instance = 64KB
	 * Plus 1 is needed for margin, because F/W exceeds sometimes.
	 */
	nal_q_out_handle->out_buf.size = dev->pdata->nal_q_entry_size * (NAL_Q_QUEUE_SIZE + 1);
	if (mfc_mem_ion_alloc(dev, &nal_q_out_handle->out_buf)) {
		mfc_err_dev("[NALQ] failed to get memory\n");
		kfree(nal_q_out_handle);
		return NULL;
	}
	nal_q_out_handle->nal_q_out_addr = nal_q_out_handle->out_buf.vaddr;

	mfc_debug_leave();

	return nal_q_out_handle;
}

static void __mfc_nal_q_destroy_in_q(struct mfc_dev *dev,
			nal_queue_in_handle *nal_q_in_handle)
{
	mfc_debug_enter();

	if (nal_q_in_handle) {
		mfc_mem_ion_free(dev, &nal_q_in_handle->in_buf);
		kfree(nal_q_in_handle);
	}

	mfc_debug_leave();
}

static void __mfc_nal_q_destroy_out_q(struct mfc_dev *dev,
			nal_queue_out_handle *nal_q_out_handle)
{
	mfc_debug_enter();

	if (nal_q_out_handle) {
		mfc_mem_ion_free(dev, &nal_q_out_handle->out_buf);
		kfree(nal_q_out_handle);
	}

	mfc_debug_leave();
}

/*
 * This function should be called after mfc_alloc_firmware() being called.
 */
nal_queue_handle *mfc_nal_q_create(struct mfc_dev *dev)
{
	nal_queue_handle *nal_q_handle;

	mfc_debug_enter();

	nal_q_handle = kzalloc(sizeof(*nal_q_handle), GFP_KERNEL);
	if (!nal_q_handle) {
		mfc_err_dev("[NALQ] no nal_q_handle\n");
		return NULL;
	}

	nal_q_handle->nal_q_in_handle = __mfc_nal_q_create_in_q(dev, nal_q_handle);
	if (!nal_q_handle->nal_q_in_handle) {
		kfree(nal_q_handle);
		mfc_err_dev("[NALQ] no nal_q_in_handle\n");
		return NULL;
	}

	spin_lock_init(&nal_q_handle->lock);

	nal_q_handle->nal_q_out_handle = __mfc_nal_q_create_out_q(dev, nal_q_handle);
	if (!nal_q_handle->nal_q_out_handle) {
		__mfc_nal_q_destroy_in_q(dev, nal_q_handle->nal_q_in_handle);
		kfree(nal_q_handle);
		mfc_err_dev("[NALQ] no nal_q_out_handle\n");
		return NULL;
	}

	nal_q_handle->nal_q_state = NAL_Q_STATE_CREATED;
	MFC_TRACE_DEV("** NAL Q state : %d\n", nal_q_handle->nal_q_state);
	mfc_debug(2, "[NALQ] handle created, state = %d\n", nal_q_handle->nal_q_state);

	mfc_debug_leave();

	return nal_q_handle;
}

void mfc_nal_q_destroy(struct mfc_dev *dev, nal_queue_handle *nal_q_handle)
{
	mfc_debug_enter();

	if (nal_q_handle->nal_q_out_handle)
		__mfc_nal_q_destroy_out_q(dev, nal_q_handle->nal_q_out_handle);

	if (nal_q_handle->nal_q_in_handle)
		__mfc_nal_q_destroy_in_q(dev, nal_q_handle->nal_q_in_handle);

	kfree(nal_q_handle);
	dev->nal_q_handle = NULL;

	mfc_debug_leave();
}

void mfc_nal_q_init(struct mfc_dev *dev, nal_queue_handle *nal_q_handle)
{
	mfc_debug_enter();

	if (!nal_q_handle) {
		mfc_err_dev("[NALQ] There is no nal_q_handle\n");
		return;
	}

	if ((nal_q_handle->nal_q_state != NAL_Q_STATE_CREATED)
		&& (nal_q_handle->nal_q_state != NAL_Q_STATE_STOPPED)) {
		mfc_err_dev("[NALQ] State is wrong, state: %d\n", nal_q_handle->nal_q_state);
		return;
	}

	mfc_reset_nal_queue_registers(dev);

	nal_q_handle->nal_q_in_handle->in_exe_count = 0;
	nal_q_handle->nal_q_out_handle->out_exe_count = 0;

	mfc_debug(2, "[NALQ] MFC_REG_NAL_QUEUE_INPUT_COUNT=%d\n",
		mfc_get_nal_q_input_count());
	mfc_debug(2, "[NALQ] MFC_REG_NAL_QUEUE_OUTPUT_COUNT=%d\n",
		mfc_get_nal_q_output_count());
	mfc_debug(2, "[NALQ] MFC_REG_NAL_QUEUE_INPUT_EXE_COUNT=%d\n",
		mfc_get_nal_q_input_exe_count());
	mfc_debug(2, "[NALQ] MFC_REG_NAL_QUEUE_INFO=%d\n",
		mfc_get_nal_q_info());

	nal_q_handle->nal_q_exception = 0;

	mfc_debug_leave();

	return;
}

void mfc_nal_q_start(struct mfc_dev *dev, nal_queue_handle *nal_q_handle)
{
	dma_addr_t addr;

	mfc_debug_enter();

	if (!nal_q_handle) {
		mfc_err_dev("[NALQ] There is no nal_q_handle\n");
		return;
	}

	if (nal_q_handle->nal_q_state != NAL_Q_STATE_CREATED) {
		mfc_err_dev("[NALQ] State is wrong, state: %d\n", nal_q_handle->nal_q_state);
		return;
	}

	addr = nal_q_handle->nal_q_in_handle->in_buf.daddr;

	mfc_update_nal_queue_input(dev, addr, dev->pdata->nal_q_entry_size * NAL_Q_QUEUE_SIZE);

	mfc_debug(2, "[NALQ] MFC_REG_NAL_QUEUE_INPUT_ADDR=0x%x\n",
		mfc_get_nal_q_input_addr());
	mfc_debug(2, "[NALQ] MFC_REG_NAL_QUEUE_INPUT_SIZE=%d\n",
		mfc_get_nal_q_input_size());

	addr = nal_q_handle->nal_q_out_handle->out_buf.daddr;

	mfc_update_nal_queue_output(dev, addr, dev->pdata->nal_q_entry_size * NAL_Q_QUEUE_SIZE);

	mfc_debug(2, "[NALQ] MFC_REG_NAL_QUEUE_OUTPUT_ADDR=0x%x\n",
		mfc_get_nal_q_output_addr());
	mfc_debug(2, "[NALQ] MFC_REG_NAL_QUEUE_OUTPUT_SIZE=%d\n",
		mfc_get_nal_q_output_ize());

	nal_q_handle->nal_q_state = NAL_Q_STATE_STARTED;
	MFC_TRACE_DEV("** NAL Q state : %d\n", nal_q_handle->nal_q_state);
	mfc_debug(2, "[NALQ] started, state = %d\n", nal_q_handle->nal_q_state);

	MFC_WRITEL(MFC_TIMEOUT_VALUE, MFC_REG_DEC_TIMEOUT_VALUE);
	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_NAL_QUEUE);

	mfc_debug_leave();

	return;
}

void mfc_nal_q_stop(struct mfc_dev *dev, nal_queue_handle *nal_q_handle)
{
	mfc_debug_enter();

	if (!nal_q_handle) {
		mfc_err_dev("[NALQ] There is no nal_q_handle\n");
		return;
	}

	if (nal_q_handle->nal_q_state != NAL_Q_STATE_STARTED) {
		mfc_err_dev("[NALQ] State is wrong, state: %d\n", nal_q_handle->nal_q_state);
		return;
	}

	nal_q_handle->nal_q_state = NAL_Q_STATE_STOPPED;
	MFC_TRACE_DEV("** NAL Q state : %d\n", nal_q_handle->nal_q_state);
	mfc_debug(2, "[NALQ] stopped, state = %d\n", nal_q_handle->nal_q_state);

	mfc_clean_dev_int_flags(dev);

	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_STOP_QUEUE);

	mfc_debug_leave();

	return;
}

void mfc_nal_q_stop_if_started(struct mfc_dev *dev)
{
	nal_queue_handle *nal_q_handle;

	mfc_debug_enter();

	nal_q_handle = dev->nal_q_handle;
	if (!nal_q_handle) {
		mfc_err_dev("[NALQ] There is no nal_q_handle\n");
		return;
	}

	if (nal_q_handle->nal_q_state != NAL_Q_STATE_STARTED) {
		mfc_debug(2, "[NALQ] it is not running, state: %d\n",
				nal_q_handle->nal_q_state);
		return;
	}

	mfc_nal_q_clock_on(dev, nal_q_handle);

	mfc_nal_q_stop(dev, nal_q_handle);
	mfc_info_dev("[NALQ] stop NAL QUEUE during get hwlock\n");
	if (mfc_wait_for_done_dev(dev,
				MFC_REG_R2H_CMD_COMPLETE_QUEUE_RET)) {
		mfc_err_dev("[NALQ] Failed to stop qeueue during get hwlock\n");
		dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_STOP_NAL_Q_FOR_OTHER);
		call_dop(dev, dump_and_stop_always, dev);
	}

	mfc_debug_leave();
	return;
}

void mfc_nal_q_cleanup_queue(struct mfc_dev *dev)
{
	struct mfc_ctx *ctx;
	int i;

	mfc_debug_enter();

	for(i = 0; i < MFC_NUM_CONTEXTS; i++) {
		ctx = dev->ctx[i];
		if (ctx) {
			mfc_cleanup_nal_queue(ctx);
			if (mfc_ctx_ready(ctx)) {
				mfc_set_bit(ctx->num, &dev->work_bits);
				mfc_debug(2, "[NALQ] set work_bits after cleanup,"
						" ctx: %d\n", ctx->num);
			}
		}
	}

	mfc_debug_leave();

	return;
}

static void __mfc_nal_q_set_slice_mode(struct mfc_ctx *ctx, EncoderInputStr *pInStr)
{
	struct mfc_enc *enc = ctx->enc_priv;

	/* multi-slice control */
	if (enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_BYTES)
		pInStr->MsliceMode = enc->slice_mode + 0x4;
	else if (enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_MB_ROW)
		pInStr->MsliceMode = enc->slice_mode - 0x2;
	else if (enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_FIXED_BYTES)
		pInStr->MsliceMode = enc->slice_mode + 0x3;
	else
		pInStr->MsliceMode = enc->slice_mode;

	/* multi-slice MB number or bit size */
	if ((enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_MB) ||
			(enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_MB_ROW)) {
		pInStr->MsliceSizeMb = enc->slice_size_mb;
	} else if ((enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_BYTES) ||
			(enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_FIXED_BYTES)){
		pInStr->MsliceSizeBits = enc->slice_size_bits;
	} else {
		pInStr->MsliceSizeMb = 0;
		pInStr->MsliceSizeBits = 0;
	}
}

static void __mfc_nal_q_set_enc_ts_delta(struct mfc_ctx *ctx, EncoderInputStr *pInStr)
{
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	int ts_delta;

	ts_delta = mfc_enc_get_ts_delta(ctx);

	pInStr->TimeStampDelta &= ~(0xFFFF);
	pInStr->TimeStampDelta |= (ts_delta & 0xFFFF);

	if (ctx->ts_last_interval)
		mfc_debug(3, "[NALQ][DFR] fps %d -> %ld, delta: %d, reg: %#x\n",
				p->rc_framerate, USEC_PER_SEC / ctx->ts_last_interval,
				ts_delta, pInStr->TimeStampDelta);
	else
		mfc_debug(3, "[NALQ][DFR] fps %d -> 0, delta: %d, reg: %#x\n",
				p->rc_framerate, ts_delta, pInStr->TimeStampDelta);
}

static void __mfc_nal_q_get_hdr_plus_info(struct mfc_ctx *ctx, DecoderOutputStr *pOutStr,
		struct hdr10_plus_meta *sei_meta)
{
	struct mfc_dev *dev = ctx->dev;
	unsigned int upper_value, lower_value;
	int num_win, num_distribution;
	int i, j;

	if (dev->pdata->nal_q_entry_size < NAL_Q_ENTRY_SIZE_FOR_HDR10) {
		mfc_err_dev("[NALQ][HDR+] insufficient NAL-Q entry size\n");
		return;
	}

	sei_meta->valid = 1;

	/* iru_t_t35 */
	sei_meta->t35_country_code = pOutStr->St2094_40sei[0] & 0xFF;
	sei_meta->t35_terminal_provider_code = pOutStr->St2094_40sei[0] >> 8 & 0xFF;
	upper_value = pOutStr->St2094_40sei[0] >> 24 & 0xFF;
	lower_value = pOutStr->St2094_40sei[1] & 0xFF;
	sei_meta->t35_terminal_provider_oriented_code = (upper_value << 8) | lower_value;

	/* application */
	sei_meta->application_identifier = pOutStr->St2094_40sei[1] >> 8 & 0xFF;
	sei_meta->application_version = pOutStr->St2094_40sei[1] >> 16 & 0xFF;

	/* window information */
	sei_meta->num_windows = pOutStr->St2094_40sei[1] >> 24 & 0x3;
	num_win = sei_meta->num_windows;
	if (num_win > dev->pdata->max_hdr_win) {
		mfc_err_ctx("NAL Q:[HDR+] num_window(%d) is exceeded supported max_num_window(%d)\n",
				num_win, dev->pdata->max_hdr_win);
		num_win = dev->pdata->max_hdr_win;
	}

	/* luminance */
	sei_meta->target_maximum_luminance = pOutStr->St2094_40sei[2] & 0x7FFFFFF;
	sei_meta->target_actual_peak_luminance_flag = pOutStr->St2094_40sei[2] >> 27 & 0x1;
	sei_meta->mastering_actual_peak_luminance_flag = pOutStr->St2094_40sei[22] >> 10 & 0x1;

	/* per window setting */
	for (i = 0; i < num_win; i++) {
		/* scl */
		for (j = 0; j < HDR_MAX_SCL; j++) {
			sei_meta->win_info[i].maxscl[j] =
				pOutStr->St2094_40sei[3 + j] & 0x1FFFF;
		}
		sei_meta->win_info[i].average_maxrgb =
			pOutStr->St2094_40sei[6] & 0x1FFFF;

		/* distribution */
		sei_meta->win_info[i].num_distribution_maxrgb_percentiles =
			pOutStr->St2094_40sei[6] >> 17 & 0xF;
		num_distribution = sei_meta->win_info[i].num_distribution_maxrgb_percentiles;
		for (j = 0; j < num_distribution; j++) {
			sei_meta->win_info[i].distribution_maxrgb_percentages[j] =
				pOutStr->St2094_40sei[7 + j] & 0x7F;
			sei_meta->win_info[i].distribution_maxrgb_percentiles[j] =
				pOutStr->St2094_40sei[7 + j] >> 7 & 0x1FFFF;
		}

		/* bright pixels */
		sei_meta->win_info[i].fraction_bright_pixels =
			pOutStr->St2094_40sei[22] & 0x3FF;

		/* tone mapping */
		sei_meta->win_info[i].tone_mapping_flag =
			pOutStr->St2094_40sei[22] >> 11 & 0x1;
		if (sei_meta->win_info[i].tone_mapping_flag) {
			sei_meta->win_info[i].knee_point_x =
				pOutStr->St2094_40sei[23] & 0xFFF;
			sei_meta->win_info[i].knee_point_y =
				pOutStr->St2094_40sei[23] >> 12 & 0xFFF;
			sei_meta->win_info[i].num_bezier_curve_anchors =
				pOutStr->St2094_40sei[23] >> 24 & 0xF;
			for (j = 0; j < HDR_MAX_BEZIER_CURVES / 3; j++) {
				sei_meta->win_info[i].bezier_curve_anchors[j * 3] =
					pOutStr->St2094_40sei[24 + j] & 0x3FF;
				sei_meta->win_info[i].bezier_curve_anchors[j * 3 + 1] =
					pOutStr->St2094_40sei[24 + j] >> 10 & 0x3FF;
				sei_meta->win_info[i].bezier_curve_anchors[j * 3 + 2] =
					pOutStr->St2094_40sei[24 + j] >> 20 & 0x3FF;
			}
		}

		/* color saturation */
		sei_meta->win_info[i].color_saturation_mapping_flag =
			pOutStr->St2094_40sei[29] & 0x1;
		if (sei_meta->win_info[i].color_saturation_mapping_flag)
			sei_meta->win_info[i].color_saturation_weight =
				pOutStr->St2094_40sei[29] >> 1 & 0x3F;
	}

	if (debug_level >= 5)
		mfc_print_hdr_plus_info(ctx, sei_meta);
}

static void __mfc_nal_q_set_hdr_plus_info(struct mfc_ctx *ctx, EncoderInputStr *pInStr,
		struct hdr10_plus_meta *sei_meta)
{
	struct mfc_dev *dev = ctx->dev;
	unsigned int val = 0;
	int num_win, num_distribution;
	int i, j;

	if (dev->pdata->nal_q_entry_size < NAL_Q_ENTRY_SIZE_FOR_HDR10) {
		mfc_err_dev("[NALQ][HDR+] insufficient NAL-Q entry size\n");
		return;
	}

	pInStr->HevcNalControl &= ~(sei_meta->valid << 6);
	pInStr->HevcNalControl |= ((sei_meta->valid & 0x1) << 6);

	/* iru_t_t35 */
	val = 0;
	val |= (sei_meta->t35_country_code & 0xFF);
	val |= ((sei_meta->t35_terminal_provider_code & 0xFF) << 8);
	val |= (((sei_meta->t35_terminal_provider_oriented_code >> 8) & 0xFF) << 24);
	pInStr->St2094_40sei[0] = val;

	/* window information */
	num_win = (sei_meta->num_windows & 0x3);
	if (!num_win || (num_win > dev->pdata->max_hdr_win)) {
		mfc_debug(3, "NAL Q:[HDR+] num_window is only supported till %d\n",
				dev->pdata->max_hdr_win);
		num_win = dev->pdata->max_hdr_win;
		sei_meta->num_windows = num_win;
	}

	/* application */
	val = 0;
	val |= (sei_meta->t35_terminal_provider_oriented_code & 0xFF);
	val |= ((sei_meta->application_identifier & 0xFF) << 8);
	val |= ((sei_meta->application_version & 0xFF) << 16);
	val |= ((sei_meta->num_windows & 0x3) << 24);
	pInStr->St2094_40sei[1] = val;

	/* luminance */
	val = 0;
	val |= (sei_meta->target_maximum_luminance & 0x7FFFFFF);
	val |= ((sei_meta->target_actual_peak_luminance_flag & 0x1) << 27);
	pInStr->St2094_40sei[2] = val;

	/* per window setting */
	for (i = 0; i < num_win; i++) {
		/* scl */
		for (j = 0; j < HDR_MAX_SCL; j++)
			pInStr->St2094_40sei[3 + j] = (sei_meta->win_info[i].maxscl[j] & 0x1FFFF);

		/* distribution */
		val = 0;
		val |= (sei_meta->win_info[i].average_maxrgb & 0x1FFFF);
		val |= ((sei_meta->win_info[i].num_distribution_maxrgb_percentiles & 0xF) << 17);
		pInStr->St2094_40sei[6] = val;
		num_distribution = (sei_meta->win_info[i].num_distribution_maxrgb_percentiles & 0xF);
		for (j = 0; j < num_distribution; j++) {
			val = 0;
			val |= (sei_meta->win_info[i].distribution_maxrgb_percentages[j] & 0x7F);
			val |= ((sei_meta->win_info[i].distribution_maxrgb_percentiles[j] & 0x1FFFF) << 7);
			pInStr->St2094_40sei[7 + j] = val;
		}

		/* bright pixels, luminance */
		val = 0;
		val |= (sei_meta->win_info[i].fraction_bright_pixels & 0x3FF);
		val |= ((sei_meta->mastering_actual_peak_luminance_flag & 0x1) << 10);

		/* tone mapping */
		val |= ((sei_meta->win_info[i].tone_mapping_flag & 0x1) << 11);
		pInStr->St2094_40sei[22] = val;
		if (sei_meta->win_info[i].tone_mapping_flag & 0x1) {
			val = 0;
			val |= (sei_meta->win_info[i].knee_point_x & 0xFFF);
			val |= ((sei_meta->win_info[i].knee_point_y & 0xFFF) << 12);
			val |= ((sei_meta->win_info[i].num_bezier_curve_anchors & 0xF) << 24);
			pInStr->St2094_40sei[23] = val;
			for (j = 0; j < HDR_MAX_BEZIER_CURVES / 3; j++) {
				val = 0;
				val |= (sei_meta->win_info[i].bezier_curve_anchors[j * 3] & 0x3FF);
				val |= ((sei_meta->win_info[i].bezier_curve_anchors[j * 3 + 1] & 0x3FF) << 10);
				val |= ((sei_meta->win_info[i].bezier_curve_anchors[j * 3 + 2] & 0x3FF) << 20);
				pInStr->St2094_40sei[24 + j] = val;
			}

		}

		/* color saturation */
		if (sei_meta->win_info[i].color_saturation_mapping_flag & 0x1) {
			val = 0;
			val |= (sei_meta->win_info[i].color_saturation_mapping_flag & 0x1);
			val |= ((sei_meta->win_info[i].color_saturation_weight & 0x3F) << 1);
			pInStr->St2094_40sei[29] = val;
		}
	}

	if (debug_level >= 5)
		mfc_print_hdr_plus_info(ctx, sei_meta);
}

static int __mfc_nal_q_run_in_buf_enc(struct mfc_ctx *ctx, EncoderInputStr *pInStr)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_buf *src_mb, *dst_mb;
	struct mfc_raw_info *raw = NULL;
	struct hdr10_plus_meta dst_sei_meta, *src_sei_meta;
	dma_addr_t src_addr[3] = {0, 0, 0};
	dma_addr_t addr_2bit[2] = {0, 0};
	unsigned int index, i;

	mfc_debug_enter();

	pInStr->StartCode = 0xBBBBBBBB;
	pInStr->CommandId = mfc_get_nal_q_input_count();
	pInStr->InstanceId = ctx->inst_no;

	raw = &ctx->raw_buf;

	if (IS_BUFFER_BATCH_MODE(ctx)) {
		src_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_SET_USED);
		if (!src_mb) {
			mfc_err_dev("[NALQ][BUFCON] no src buffers\n");
			return -EAGAIN;
		}

		/* last image in a buffer container */
		/* move src_queue -> src_queue_nal_q */
		if (src_mb->next_index == (src_mb->num_valid_bufs - 1)) {
			src_mb = mfc_get_move_buf(&ctx->buf_queue_lock,
					&ctx->src_buf_nal_queue, &ctx->src_buf_queue,
					MFC_BUF_SET_USED, MFC_QUEUE_ADD_BOTTOM);
			if (!src_mb) {
				mfc_err_dev("[NALQ][BUFCON] no src buffers\n");
				return -EAGAIN;
			}
		}

		index = src_mb->vb.vb2_buf.index;
		for (i = 0; i < raw->num_planes; i++) {
			src_addr[i] = src_mb->addr[src_mb->next_index][i];
			mfc_debug(2, "[NALQ][BUFCON][BUFINFO] ctx[%d] set src index:%d, batch[%d], addr[%d]: 0x%08llx\n",
					ctx->num, index, src_mb->next_index, i, src_addr[i]);
		}
		src_mb->next_index++;
	} else {
		/* move src_queue -> src_queue_nal_q */
		src_mb = mfc_get_move_buf(&ctx->buf_queue_lock,
				&ctx->src_buf_nal_queue, &ctx->src_buf_queue,
				MFC_BUF_SET_USED, MFC_QUEUE_ADD_BOTTOM);
		if (!src_mb) {
			mfc_err_dev("[NALQ] no src buffers\n");
			return -EAGAIN;
		}

		index = src_mb->vb.vb2_buf.index;
		for (i = 0; i < raw->num_planes; i++) {
			src_addr[i] = src_mb->addr[0][i];
			mfc_debug(2, "[NALQ][BUFINFO] ctx[%d] set src index:%d, addr[%d]: 0x%08llx\n",
					ctx->num, index, i, src_addr[i]);
		}
	}

	for (i = 0; i < raw->num_planes; i++)
		pInStr->FrameAddr[i] = src_addr[i];

	if (ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV12M_S10B ||
		ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV21M_S10B) {
		addr_2bit[0] = src_addr[0] + NV12N_Y_SIZE(ctx->img_width, ctx->img_height);
		addr_2bit[1] = src_addr[1] + NV12N_CBCR_SIZE(ctx->img_width, ctx->img_height);

		for (i = 0; i < raw->num_planes; i++) {
			pInStr->Frame2bitAddr[i] = addr_2bit[i];
			mfc_debug(2, "[NALQ][BUFINFO] ctx[%d] set src index:%d, 2bit addr[%d]: 0x%08llx\n",
					ctx->num, index, i, addr_2bit[i]);
		}
	} else if (ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV16M_S10B ||
		ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV61M_S10B) {
		addr_2bit[0] = src_addr[0] + NV16M_Y_SIZE(ctx->img_width, ctx->img_height);
		addr_2bit[1] = src_addr[1] + NV16M_CBCR_SIZE(ctx->img_width, ctx->img_height);

		for (i = 0; i < raw->num_planes; i++) {
			pInStr->Frame2bitAddr[i] = addr_2bit[i];
			mfc_debug(2, "[NALQ][BUFINFO] ctx[%d] set src index:%d, 2bit addr[%d]: 0x%08llx\n",
					ctx->num, index, i, addr_2bit[i]);
		}
	}

	/* HDR10+ sei meta */
	index = src_mb->vb.vb2_buf.index;
	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->hdr10_plus)) {
		if (enc->sh_handle_hdr.fd == -1) {
			mfc_debug(3, "[NALQ][HDR+] there is no handle for SEI meta\n");
		} else {
			src_sei_meta = (struct hdr10_plus_meta *)enc->sh_handle_hdr.vaddr + index;
			if (src_sei_meta->valid) {
				mfc_debug(3, "[NALQ][HDR+] there is valid SEI meta data in buf[%d]\n",
						index);
				memcpy(&dst_sei_meta, src_sei_meta, sizeof(struct hdr10_plus_meta));
				__mfc_nal_q_set_hdr_plus_info(ctx, pInStr, &dst_sei_meta);
			}
		}
	}

	/* move dst_queue -> dst_queue_nal_q */
	dst_mb = mfc_get_move_buf(&ctx->buf_queue_lock,
		&ctx->dst_buf_nal_queue, &ctx->dst_buf_queue, MFC_BUF_SET_USED, MFC_QUEUE_ADD_BOTTOM);
	if (!dst_mb) {
		mfc_err_dev("[NALQ] no dst buffers\n");
		return -EAGAIN;
	}

	pInStr->StreamBufferAddr = dst_mb->addr[0][0];
	pInStr->StreamBufferSize = (unsigned int)vb2_plane_size(&dst_mb->vb.vb2_buf, 0);
	pInStr->StreamBufferSize = ALIGN(pInStr->StreamBufferSize, 512);

	if (call_cop(ctx, set_buf_ctrls_val_nal_q_enc, ctx, &ctx->src_ctrls[index], pInStr) < 0)
		mfc_err_ctx("[NALQ] failed in set_buf_ctrals_val in nal q\n");

	mfc_debug(2, "[NALQ][BUFINFO] ctx[%d] set dst index: %d, addr: 0x%08x\n",
			ctx->num, dst_mb->vb.vb2_buf.index, pInStr->StreamBufferAddr);
	mfc_debug(2, "[NALQ] input queue, src_buf_queue -> src_buf_nal_queue, index:%d\n",
			src_mb->vb.vb2_buf.index);
	mfc_debug(2, "[NALQ] input queue, dst_buf_queue -> dst_buf_nal_queue, index:%d\n",
			dst_mb->vb.vb2_buf.index);

	__mfc_nal_q_set_slice_mode(ctx, pInStr);
	__mfc_nal_q_set_enc_ts_delta(ctx, pInStr);

	mfc_debug_leave();

	return 0;
}

static int __mfc_nal_q_run_in_buf_dec(struct mfc_ctx *ctx, DecoderInputStr *pInStr)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_buf *src_mb, *dst_mb;
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_raw_info *raw = &ctx->raw_buf;
	dma_addr_t buf_addr;
	unsigned int strm_size;
	unsigned int cpb_buf_size;
	int src_index, dst_index;
	int i;

	mfc_debug_enter();

	if (mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0) &&
			mfc_is_queue_count_smaller(&ctx->buf_queue_lock,
				&ctx->ref_buf_queue, (ctx->dpb_count + 5))) {
		mfc_err_dev("[NALQ] no dst buffers\n");
		return -EAGAIN;
	}

	if (mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->src_buf_queue, 0)) {
		mfc_err_dev("[NALQ] no src buffers\n");
		return -EAGAIN;
	}

	pInStr->StartCode = 0xAAAAAAAA;
	pInStr->CommandId = mfc_get_nal_q_input_count();
	pInStr->InstanceId = ctx->inst_no;
	pInStr->NalStartOptions = 0;

	/* Try to use the non-referenced DPB on dst-queue */
	dst_mb = mfc_search_move_dpb_nal_q(ctx, dec->dynamic_used);
	if (!dst_mb) {
		mfc_debug(2, "[NALQ][DPB] couldn't find dst buffers\n");
		return -EAGAIN;
	}

	/* move src_queue -> src_queue_nal_q */
	src_mb = mfc_get_move_buf(&ctx->buf_queue_lock,
			&ctx->src_buf_nal_queue, &ctx->src_buf_queue,
			MFC_BUF_SET_USED, MFC_QUEUE_ADD_BOTTOM);
	if (!src_mb) {
		mfc_err_dev("[NALQ] no src buffers\n");
		return -EAGAIN;
	}

	/* src buffer setting */
	src_index = src_mb->vb.vb2_buf.index;
	buf_addr = src_mb->addr[0][0];
	strm_size = src_mb->vb.vb2_buf.planes[0].bytesused;
	cpb_buf_size = ALIGN(dec->src_buf_size, STREAM_BUF_ALIGN);

	if (strm_size > set_strm_size_max(cpb_buf_size)) {
		mfc_info_ctx("[NALQ] Decrease strm_size : %u -> %u, gap : %d\n",
				strm_size, set_strm_size_max(cpb_buf_size), STREAM_BUF_ALIGN);
		strm_size = set_strm_size_max(cpb_buf_size);
		src_mb->vb.vb2_buf.planes[0].bytesused = strm_size;
	}

	mfc_debug(2, "[NALQ][BUFINFO] ctx[%d] set src index: %d, addr: 0x%08llx\n",
			ctx->num, src_index, buf_addr);
	mfc_debug(2, "[NALQ][STREAM] strm_size: %#x(%d), buf_size: %u\n",
			strm_size, strm_size, cpb_buf_size);

	if (strm_size == 0)
		mfc_info_ctx("stream size is 0\n");

	pInStr->StreamDataSize = strm_size;
	pInStr->CpbBufferAddr = buf_addr;
	pInStr->CpbBufferSize = cpb_buf_size;
	pInStr->CpbBufferOffset = 0;
	ctx->last_src_addr = buf_addr;

	/* dst buffer setting */
	dst_index = dst_mb->vb.vb2_buf.index;
	set_bit(dst_index, &dec->available_dpb);
	dec->dynamic_set = 1 << dst_index;

	for (i = 0; i < raw->num_planes; i++) {
		pInStr->FrameSize[i] = raw->plane_size[i];
		pInStr->FrameAddr[i] = dst_mb->addr[0][i];
		ctx->last_dst_addr[i] = dst_mb->addr[0][i];
		if (ctx->is_10bit)
			pInStr->Frame2BitSize[i] = raw->plane_size_2bits[i];
		mfc_debug(2, "[NALQ][BUFINFO][DPB] ctx[%d] set dst index: %d, addr[%d]: 0x%08llx\n",
				ctx->num, dst_index, i, dst_mb->addr[0][i]);
	}

	pInStr->ScratchBufAddr = ctx->codec_buf.daddr;
	pInStr->ScratchBufSize = ctx->scratch_buf_size;

	if (call_cop(ctx, set_buf_ctrls_val_nal_q_dec, ctx,
				&ctx->src_ctrls[src_index], pInStr) < 0)
		mfc_err_ctx("[NALQ] failed in set_buf_ctrls_val\n");

	pInStr->DynamicDpbFlagLower = dec->dynamic_set;

	/* use dynamic_set value to available dpb in NAL Q */
	// pInStr->AvailableDpbFlagLower = dec->available_dpb;
	pInStr->AvailableDpbFlagLower = dec->dynamic_set;

	MFC_TRACE_CTX("Set dst[%d] fd: %d, %#llx / avail %#lx used %#x\n",
			dst_index, dst_mb->vb.vb2_buf.planes[0].m.fd, dst_mb->addr[0][0],
			dec->available_dpb, dec->dynamic_used);

	mfc_debug_leave();

	return 0;
}

static void __mfc_nal_q_get_enc_frame_buffer(struct mfc_ctx *ctx,
		dma_addr_t addr[], int num_planes, EncoderOutputStr *pOutStr)
{
	unsigned long enc_recon_y_addr, enc_recon_c_addr;
	int i;

	for (i = 0; i < num_planes; i++)
		addr[i] = pOutStr->EncodedFrameAddr[i];

	enc_recon_y_addr = pOutStr->ReconLumaDpbAddr;
	enc_recon_c_addr = pOutStr->ReconChromaDpbAddr;

	mfc_debug(2, "[NALQ][MEMINFO] recon y: 0x%08lx c: 0x%08lx\n",
			enc_recon_y_addr, enc_recon_c_addr);
}

static void __mfc_nal_q_handle_stream_copy_timestamp(struct mfc_ctx *ctx, struct mfc_buf *src_mb)
{
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	struct mfc_buf *dst_mb;
	u64 interval;
	u64 start_timestamp;
	u64 new_timestamp;

	start_timestamp = src_mb->vb.vb2_buf.timestamp;
	interval = NSEC_PER_SEC / p->rc_framerate;
	if (debug_ts == 1)
		mfc_info_ctx("[NALQ][BUFCON][TS] %dfps, start timestamp: %lld, base interval: %lld\n",
				p->rc_framerate, start_timestamp, interval);

	new_timestamp = start_timestamp + (interval * src_mb->done_index);
	if (debug_ts == 1)
		mfc_info_ctx("[NALQ][BUFCON][TS] new timestamp: %lld, interval: %lld\n",
				new_timestamp, interval * src_mb->done_index);

	/* Get the destination buffer */
	dst_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->dst_buf_nal_queue, MFC_BUF_NO_TOUCH_USED);
	if (dst_mb)
		dst_mb->vb.vb2_buf.timestamp = new_timestamp;
}

static void __mfc_nal_q_handle_stream_input(struct mfc_ctx *ctx, EncoderOutputStr *pOutStr)
{
	struct mfc_buf *src_mb, *ref_mb;
	dma_addr_t enc_addr[3] = { 0, 0, 0 };
	struct mfc_raw_info *raw;
	int found_in_src_queue = 0;
	unsigned int i;

	raw = &ctx->raw_buf;

	__mfc_nal_q_get_enc_frame_buffer(ctx, &enc_addr[0], raw->num_planes, pOutStr);
	if (enc_addr[0] == 0) {
		mfc_debug(3, "[NALQ] no encoded src\n");
		goto move_buf;
	}

	for (i = 0; i < raw->num_planes; i++)
		mfc_debug(2, "[NALQ][BUFINFO] ctx[%d] get src addr[%d]: 0x%08llx\n",
				ctx->num, i, enc_addr[i]);

	if (IS_BUFFER_BATCH_MODE(ctx)) {
		src_mb = mfc_find_first_buf(&ctx->buf_queue_lock,
				&ctx->src_buf_queue, enc_addr[0]);
		if (src_mb) {
			found_in_src_queue = 1;

			__mfc_nal_q_handle_stream_copy_timestamp(ctx, src_mb);
			src_mb->done_index++;
			mfc_debug(4, "[NALQ][BUFCON] batch buf done_index: %d\n", src_mb->done_index);
		} else {
			src_mb = mfc_find_first_buf(&ctx->buf_queue_lock,
					&ctx->src_buf_nal_queue, enc_addr[0]);
			if (src_mb) {
				found_in_src_queue = 1;

				__mfc_nal_q_handle_stream_copy_timestamp(ctx, src_mb);
				src_mb->done_index++;
				mfc_debug(4, "[NALQ][BUFCON] batch buf done_index: %d\n", src_mb->done_index);

				/* last image in a buffer container */
				if (src_mb->done_index == src_mb->num_valid_bufs) {
					src_mb = mfc_find_del_buf(&ctx->buf_queue_lock,
							&ctx->src_buf_nal_queue, enc_addr[0]);
					if (src_mb) {
						for (i = 0; i < raw->num_planes; i++)
							mfc_bufcon_put_daddr(ctx, src_mb, i);
						vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
					}
				}
			}
		}
	} else {
		src_mb = mfc_find_del_buf(&ctx->buf_queue_lock,
				&ctx->src_buf_nal_queue, enc_addr[0]);
		if (src_mb) {
			mfc_debug(3, "[NALQ] find src buf in src_queue\n");
			found_in_src_queue = 1;
			vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
		} else {
			mfc_debug(3, "[NALQ] no src buf in src_queue\n");
			ref_mb = mfc_find_del_buf(&ctx->buf_queue_lock,
					&ctx->ref_buf_queue, enc_addr[0]);
			if (ref_mb) {
				mfc_debug(3, "[NALQ] find src buf in ref_queue\n");
				vb2_buffer_done(&ref_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
			} else {
				mfc_err_ctx("[NALQ] couldn't find src buffer\n");
			}
		}
	}

move_buf:
	/* move enqueued src buffer: src nal queue -> ref queue */
	if (!found_in_src_queue) {
		src_mb = mfc_get_move_buf_used(&ctx->buf_queue_lock,
				&ctx->ref_buf_queue, &ctx->src_buf_nal_queue);
		if (!src_mb)
			mfc_err_dev("[NALQ] no src buffers\n");

		mfc_debug(2, "[NALQ] enc src_buf_nal_queue(%d) -> ref_buf_queue(%d)\n",
				mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue),
				mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->ref_buf_queue));
	}
}

static void __mfc_nal_q_handle_stream_output(struct mfc_ctx *ctx, int slice_type,
				unsigned int strm_size, EncoderOutputStr *pOutStr)
{
	struct mfc_buf *dst_mb;
	unsigned int index;

	if (strm_size == 0) {
		mfc_debug(3, "[NALQ] no encoded dst (reuse)\n");
		dst_mb = mfc_get_move_buf(&ctx->buf_queue_lock,
				&ctx->dst_buf_queue, &ctx->dst_buf_nal_queue,
				MFC_BUF_RESET_USED, MFC_QUEUE_ADD_TOP);
		if (!dst_mb) {
			mfc_err_dev("[NALQ] no dst buffers\n");
			return;
		}

		mfc_debug(2, "[NALQ] no output, dst_buf_nal_queue(%d) -> dst_buf_queue(%d) index:%d\n",
				mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_nal_queue),
				mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_queue),
				dst_mb->vb.vb2_buf.index);
		return;
	}

	/* at least one more dest. buffers exist always  */
	dst_mb = mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->dst_buf_nal_queue, MFC_BUF_NO_TOUCH_USED);
	if (!dst_mb) {
		mfc_err_dev("[NALQ] no dst buffers\n");
		return;
	}

	mfc_debug(2, "[NALQ][BUFINFO] ctx[%d] get dst addr: 0x%08llx\n",
			ctx->num, dst_mb->addr[0][0]);

	dst_mb->vb.flags &= ~(V4L2_BUF_FLAG_KEYFRAME |
				V4L2_BUF_FLAG_PFRAME |
				V4L2_BUF_FLAG_BFRAME);

	switch (slice_type) {
	case MFC_REG_E_SLICE_TYPE_I:
		dst_mb->vb.flags |= V4L2_BUF_FLAG_KEYFRAME;
		break;
	case MFC_REG_E_SLICE_TYPE_P:
		dst_mb->vb.flags |= V4L2_BUF_FLAG_PFRAME;
		break;
	case MFC_REG_E_SLICE_TYPE_B:
		dst_mb->vb.flags |= V4L2_BUF_FLAG_BFRAME;
		break;
	default:
		dst_mb->vb.flags |= V4L2_BUF_FLAG_KEYFRAME;
		break;
	}
	mfc_debug(2, "[NALQ][STREAM] Slice type flag: %d\n", dst_mb->vb.flags);

	vb2_set_plane_payload(&dst_mb->vb.vb2_buf, 0, strm_size);

	index = dst_mb->vb.vb2_buf.index;
	if (call_cop(ctx, get_buf_ctrls_val_nal_q_enc, ctx,
				&ctx->dst_ctrls[index], pOutStr) < 0)
		mfc_err_ctx("[NALQ] failed in get_buf_ctrls_val in nal q\n");

	vb2_buffer_done(&dst_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
}

static void __mfc_nal_q_handle_stream(struct mfc_ctx *ctx, EncoderOutputStr *pOutStr)
{
	struct mfc_enc *enc = ctx->enc_priv;
	int slice_type;
	unsigned int strm_size;
	unsigned int pic_count;

	mfc_debug_enter();

	slice_type = pOutStr->SliceType;
	strm_size = pOutStr->StreamSize;
	pic_count = pOutStr->EncCnt;

	mfc_debug(2, "[NALQ][STREAM] encoded slice type: %d, size: %d, display order: %d\n",
			slice_type, strm_size, pic_count);

	/* buffer full handling */
	if (ctx->state == MFCINST_RUNNING_BUF_FULL)
		mfc_change_state(ctx, MFCINST_RUNNING);

	/* set encoded frame type */
	enc->frame_type = slice_type;
	ctx->sequence++;

	/* handle input buffer */
	__mfc_nal_q_handle_stream_input(ctx, pOutStr);

	/* handle output buffer */
	__mfc_nal_q_handle_stream_output(ctx, slice_type, strm_size, pOutStr);

	mfc_debug_leave();

	return;
}

static void __mfc_nal_q_handle_reuse_buffer(struct mfc_ctx *ctx, DecoderOutputStr *pOutStr)
{
	struct mfc_dec *dec = ctx->dec_priv;
	unsigned int prev_flag, released_flag = 0;
	struct mfc_buf *dst_mb;
	dma_addr_t disp_addr;
	int i;

	/* reuse not used buf: dst_buf_nal_queue -> dst_queue */
	disp_addr = pOutStr->DisplayAddr[0];
	if (disp_addr) {
		mfc_debug(2, "[NALQ][DPB] decoding only but there is disp addr: 0x%llx\n", disp_addr);
		dst_mb = mfc_get_move_buf_addr(&ctx->buf_queue_lock,
				&ctx->dst_buf_queue, &ctx->dst_buf_nal_queue,
				disp_addr);
		if (dst_mb) {
			mfc_debug(2, "[NALQ][DPB] buf[%d] will reused. addr: 0x%08llx\n",
					dst_mb->vb.vb2_buf.index, disp_addr);
			dst_mb->used = 0;
			clear_bit(dst_mb->vb.vb2_buf.index, &dec->available_dpb);
		}
	}

	/* reuse not referenced buf anymore: ref_queue -> dst_queue */
	prev_flag = dec->dynamic_used;
	dec->dynamic_used = pOutStr->UsedDpbFlagLower;
	released_flag = prev_flag & (~dec->dynamic_used);

	if (!released_flag)
		return;

	for (i = 0; i < MFC_MAX_DPBS; i++)
		if (released_flag & (1 << i))
			if (mfc_move_reuse_buffer(ctx, i))
				released_flag &= ~(1 << i);

	/* Not reused buffer should be released when there is a display frame */
	dec->dec_only_release_flag |= released_flag;
	for (i = 0; i < MFC_MAX_DPBS; i++)
		if (released_flag & (1 << i))
			clear_bit(i, &dec->available_dpb);
}

static void __mfc_nal_q_handle_ref_frame(struct mfc_ctx *ctx, DecoderOutputStr *pOutStr)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *dst_mb;
	dma_addr_t dec_addr;
	int index = 0;

	mfc_debug_enter();

	dec_addr = pOutStr->DecodedAddr[0];

	dst_mb = mfc_find_move_buf_used(&ctx->buf_queue_lock,
		&ctx->ref_buf_queue, &ctx->dst_buf_nal_queue, dec_addr);
	if (dst_mb) {
		mfc_debug(2, "[NALQ][DPB] Found in dst queue = 0x%08llx, buf = 0x%08llx\n",
				dec_addr, dst_mb->addr[0][0]);

		index = dst_mb->vb.vb2_buf.index;
		if (!((1 << index) & pOutStr->UsedDpbFlagLower))
			dec->dynamic_used |= 1 << index;
	} else {
		mfc_debug(2, "[NALQ][DPB] Can't find buffer for addr: 0x%08llx\n", dec_addr);
	}

	mfc_debug_leave();
}

static void __mfc_nal_q_handle_frame_copy_timestamp(struct mfc_ctx *ctx, DecoderOutputStr *pOutStr)
{
	struct mfc_buf *ref_mb, *src_mb;
	dma_addr_t dec_y_addr;

	mfc_debug_enter();

	dec_y_addr = pOutStr->DecodedAddr[0];

	/* Get the next source buffer */
	src_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue, MFC_BUF_NO_TOUCH_USED);
	if (!src_mb) {
		mfc_err_dev("[NALQ][TS] no src buffers\n");
		return;
	}

	ref_mb = mfc_find_buf(&ctx->buf_queue_lock, &ctx->ref_buf_queue, dec_y_addr);
	if (ref_mb)
		ref_mb->vb.vb2_buf.timestamp = src_mb->vb.vb2_buf.timestamp;

	mfc_debug_leave();
}

static void __mfc_nal_q_handle_frame_output_move_vc1(struct mfc_ctx *ctx,
		dma_addr_t dspl_y_addr, unsigned int released_flag)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *ref_mb;
	int index = 0;
	int i;

	ref_mb = mfc_find_move_buf(&ctx->buf_queue_lock,
			&ctx->dst_buf_queue, &ctx->ref_buf_queue, dspl_y_addr, released_flag);
	if (ref_mb) {
		index = ref_mb->vb.vb2_buf.index;

		/* Check if this is the buffer we're looking for */
		mfc_debug(2, "[NALQ][DPB] Found buf[%d] 0x%08llx, looking for disp addr 0x%08llx\n",
				index, ref_mb->addr[0][0], dspl_y_addr);

		if (released_flag & (1 << index)) {
			dec->available_dpb &= ~(1 << index);
			released_flag &= ~(1 << index);
			mfc_debug(2, "[NALQ][DPB] Corrupted frame(%d), it will be re-used(release)\n",
					mfc_get_warn(mfc_get_int_err()));
		} else {
			dec->err_reuse_flag |= 1 << index;
			dec->dynamic_used |= (1 << index);
			mfc_debug(2, "[NALQ][DPB] Corrupted frame(%d), it will be re-used(not released)\n",
					mfc_get_warn(mfc_get_int_err()));
		}
	}

	if (!released_flag)
		return;

	for (i = 0; i < MFC_MAX_DPBS; i++) {
		if (released_flag & (1 << i)) {
			if (mfc_move_reuse_buffer(ctx, i)) {
				/*
				 * If the released buffer is in ref_buf_q,
				 * it means that driver owns that buffer.
				 * In that case, move buffer from ref_buf_q to dst_buf_q to reuse it.
				 */
				dec->available_dpb &= ~(1 << i);
				mfc_debug(2, "[NALQ][DPB] released buf[%d] is reused\n", i);
			} else {
				/*
				 * Otherwise, because the user owns the buffer
				 * the buffer should be included in release_info when display frame.
				 */
				dec->dec_only_release_flag |= (1 << i);
				mfc_debug(2, "[NALQ][DPB] released buf[%d] is in dec_only flag\n", i);
			}
		}
	}
}


static void __mfc_nal_q_handle_frame_output_move(struct mfc_ctx *ctx,
			dma_addr_t dspl_y_addr, unsigned int released_flag)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_dev *dev = ctx->dev;
	struct mfc_buf *dst_mb;
	unsigned int index;

	dst_mb = mfc_find_move_buf(&ctx->buf_queue_lock,
			&ctx->dst_buf_queue, &ctx->ref_buf_queue, dspl_y_addr, released_flag);
	if (dst_mb) {
		index = dst_mb->vb.vb2_buf.index;

		/* Check if this is the buffer we're looking for */
		mfc_debug(2, "[NALQ][DPB] Found buf[%d] 0x%08llx, looking for disp addr 0x%08llx\n",
				index, dst_mb->addr[0][0], dspl_y_addr);

		if (released_flag & (1 << index)) {
			dec->available_dpb &= ~(1 << index);
			released_flag &= ~(1 << index);
			mfc_debug(2, "[NALQ][DPB] Corrupted frame(%d), it will be re-used(release)\n",
					mfc_get_warn(mfc_get_int_err()));
		} else {
			dec->err_reuse_flag |= 1 << index;
			mfc_debug(2, "[NALQ][DPB] Corrupted frame(%d), it will be re-used(not released)\n",
					mfc_get_warn(mfc_get_int_err()));
		}
		dec->dynamic_used |= released_flag;
	}
}

static void __mfc_nal_q_handle_frame_output_del(struct mfc_ctx *ctx,
		DecoderOutputStr *pOutStr, unsigned int err, unsigned int released_flag)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_dev *dev = ctx->dev;
	struct mfc_raw_info *raw = &ctx->raw_buf;
	struct mfc_buf *ref_mb;
	dma_addr_t dspl_y_addr;
	unsigned int frame_type;
	unsigned int dst_frame_status;
	unsigned int is_video_signal_type = 0, is_colour_description = 0;
	unsigned int is_content_light = 0, is_display_colour = 0;
	unsigned int is_hdr10_plus_sei = 0;
	unsigned int disp_err;
	int i, index;

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->color_aspect_dec)) {
		is_video_signal_type = ((pOutStr->VideoSignalType
					>> MFC_REG_D_VIDEO_SIGNAL_TYPE_FLAG_SHIFT)
					& MFC_REG_D_VIDEO_SIGNAL_TYPE_FLAG_MASK);
		is_colour_description = ((pOutStr->VideoSignalType
					>> MFC_REG_D_COLOUR_DESCRIPTION_FLAG_SHIFT)
					& MFC_REG_D_COLOUR_DESCRIPTION_FLAG_MASK);
	}

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->static_info_dec)) {
		is_content_light = ((pOutStr->SeiAvail >> MFC_REG_D_SEI_AVAIL_CONTENT_LIGHT_SHIFT)
					& MFC_REG_D_SEI_AVAIL_CONTENT_LIGHT_MASK);
		is_display_colour = ((pOutStr->SeiAvail >> MFC_REG_D_SEI_AVAIL_MASTERING_DISPLAY_SHIFT)
					& MFC_REG_D_SEI_AVAIL_MASTERING_DISPLAY_MASK);
	}

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->hdr10_plus))
		is_hdr10_plus_sei = ((pOutStr->SeiAvail >> MFC_REG_D_SEI_AVAIL_ST_2094_40_SHIFT)
					& MFC_REG_D_SEI_AVAIL_ST_2094_40_MASK);
	if (dec->immediate_display == 1) {
		dspl_y_addr = pOutStr->DecodedAddr[0];
		frame_type = pOutStr->DecodedFrameType & MFC_REG_DECODED_FRAME_MASK;
	} else {
		dspl_y_addr = pOutStr->DisplayAddr[0];
		frame_type = pOutStr->DisplayFrameType & MFC_REG_DISPLAY_FRAME_MASK;
	}

	ref_mb = mfc_find_del_buf(&ctx->buf_queue_lock, &ctx->ref_buf_queue, dspl_y_addr);
	if (ref_mb) {
		index = ref_mb->vb.vb2_buf.index;

		/* Check if this is the buffer we're looking for */
		mfc_debug(2, "[NALQ][BUFINFO][DPB] ctx[%d] get dst index: %d, addr[0]: 0x%08llx\n",
				ctx->num, index, ref_mb->addr[0][0]);

		ref_mb->vb.sequence = ctx->sequence;

		/* Set reserved2 bits in order to inform SEI information */
		mfc_clear_vb_flag(ref_mb);

		if (is_content_light) {
			mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_CONTENT_LIGHT);
			mfc_debug(2, "[NALQ][HDR] content light level parsed\n");
		}
		if (is_display_colour) {
			mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_DISPLAY_COLOUR);
			mfc_debug(2, "[NALQ][HDR] mastering display colour parsed\n");
		}
		if (is_video_signal_type) {
			mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_VIDEO_SIGNAL_TYPE);
			mfc_debug(2, "[NALQ][HDR] video signal type parsed\n");
			if (is_colour_description) {
				mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_MAXTIX_COEFF);
				mfc_debug(2, "[NALQ][HDR] matrix coefficients parsed\n");
				mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_COLOUR_DESC);
				mfc_debug(2, "[NALQ][HDR] colour description parsed\n");
			}
		}

		if (IS_VP9_DEC(ctx) && MFC_FEATURE_SUPPORT(dev, dev->pdata->color_aspect_dec)) {
			if (dec->color_space != MFC_REG_D_COLOR_UNKNOWN) {
				mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_COLOUR_DESC);
				mfc_debug(2, "[NALQ][HDR] color space parsed\n");
			}
			mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_VIDEO_SIGNAL_TYPE);
			mfc_debug(2, "[NALQ][HDR] color range parsed\n");
		}

		if (is_hdr10_plus_sei) {
			if (dec->hdr10_plus_info) {
				__mfc_nal_q_get_hdr_plus_info(ctx, pOutStr, &dec->hdr10_plus_info[index]);
				mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_PLUS);
				mfc_debug(2, "[NALQ][HDR+] HDR10 plus dyanmic SEI metadata parsed\n");
			} else {
				mfc_err_ctx("[NALQ][HDR+] HDR10 plus cannot be parsed\n");
			}
		} else {
			if (dec->hdr10_plus_info)
				dec->hdr10_plus_info[index].valid = 0;
		}

		for (i = 0; i < raw->num_planes; i++)
			vb2_set_plane_payload(&ref_mb->vb.vb2_buf, i,
					raw->plane_size[i]);

		clear_bit(index, &dec->available_dpb);

		ref_mb->vb.flags &= ~(V4L2_BUF_FLAG_KEYFRAME |
					V4L2_BUF_FLAG_PFRAME |
					V4L2_BUF_FLAG_BFRAME |
					V4L2_BUF_FLAG_ERROR);

		switch (frame_type) {
			case MFC_REG_DISPLAY_FRAME_I:
				ref_mb->vb.flags |= V4L2_BUF_FLAG_KEYFRAME;
				break;
			case MFC_REG_DISPLAY_FRAME_P:
				ref_mb->vb.flags |= V4L2_BUF_FLAG_PFRAME;
				break;
			case MFC_REG_DISPLAY_FRAME_B:
				ref_mb->vb.flags |= V4L2_BUF_FLAG_BFRAME;
				break;
			default:
				break;
		}

		disp_err = mfc_get_warn(pOutStr->ErrorCode);
		if (disp_err) {
			mfc_err_ctx("[NALQ] Warning for displayed frame: %d\n",
					disp_err);
			ref_mb->vb.flags |= V4L2_BUF_FLAG_ERROR;
		}

		if (call_cop(ctx, get_buf_ctrls_val_nal_q_dec, ctx,
					&ctx->dst_ctrls[index], pOutStr) < 0)
			mfc_err_ctx("[NALQ] failed in get_buf_ctrls_val\n");

		mfc_handle_released_info(ctx, released_flag, index);

		if (dec->immediate_display == 1) {
			dst_frame_status = pOutStr->DecodedStatus
				& MFC_REG_DEC_STATUS_DECODED_STATUS_MASK;

			call_cop(ctx, get_buf_update_val, ctx,
					&ctx->dst_ctrls[index],
					V4L2_CID_MPEG_MFC51_VIDEO_DISPLAY_STATUS,
					dst_frame_status);

			call_cop(ctx, get_buf_update_val, ctx,
					&ctx->dst_ctrls[index],
					V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
					dec->stored_tag);

			dec->immediate_display = 0;
		}

		mfc_qos_update_last_framerate(ctx, ref_mb->vb.vb2_buf.timestamp);
		vb2_buffer_done(&ref_mb->vb.vb2_buf, disp_err ?
				VB2_BUF_STATE_ERROR : VB2_BUF_STATE_DONE);
	}
}

static void __mfc_nal_q_handle_frame_new(struct mfc_ctx *ctx, unsigned int err,
					DecoderOutputStr *pOutStr)
{
	struct mfc_dec *dec = ctx->dec_priv;
	dma_addr_t dspl_y_addr;
	unsigned int frame_type;
	unsigned int prev_flag, released_flag = 0;
	unsigned int disp_err;

	mfc_debug_enter();

	frame_type = pOutStr->DisplayFrameType & MFC_REG_DISPLAY_FRAME_MASK;
	disp_err = mfc_get_warn(pOutStr->ErrorCode);

	ctx->sequence++;

	dspl_y_addr = pOutStr->DisplayAddr[0];

	if (dec->immediate_display == 1) {
		dspl_y_addr = pOutStr->DecodedAddr[0];
		frame_type = pOutStr->DecodedFrameType & MFC_REG_DECODED_FRAME_MASK;
	}

	mfc_debug(2, "[NALQ][FRAME] frame type: %d\n", frame_type);

	/* If frame is same as previous then skip and do not dequeue */
	if (frame_type == MFC_REG_DISPLAY_FRAME_NOT_CODED) {
		if (!CODEC_NOT_CODED(ctx))
			return;
	}

	prev_flag = dec->dynamic_used;
	dec->dynamic_used = pOutStr->UsedDpbFlagLower;
	released_flag = prev_flag & (~dec->dynamic_used);

	mfc_debug(2, "[NALQ][DPB] Used flag: old = %08x, new = %08x, Released buffer = %08x\n",
			prev_flag, dec->dynamic_used, released_flag);

	if (IS_VC1_RCV_DEC(ctx) &&
		disp_err == MFC_REG_ERR_SYNC_POINT_NOT_RECEIVED)
		__mfc_nal_q_handle_frame_output_move_vc1(ctx, dspl_y_addr, released_flag);
	else if (disp_err == MFC_REG_ERR_BROKEN_LINK)
		__mfc_nal_q_handle_frame_output_move(ctx, dspl_y_addr, released_flag);
	else
		__mfc_nal_q_handle_frame_output_del(ctx, pOutStr, err, released_flag);

	mfc_debug_leave();
}

static void __mfc_nal_q_handle_frame_input(struct mfc_ctx *ctx, unsigned int err,
					DecoderOutputStr *pOutStr)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_dev *dev = ctx->dev;
	struct mfc_buf *src_mb;
	unsigned int index;
	int deleted = 0;
	unsigned long consumed;

	/* If there is consumed byte, it is abnormal status,
	 * We have to return remained stream buffer
	 */
	if (dec->consumed) {
		mfc_err_dev("[NALQ] previous buffer was not fully consumed\n");
		src_mb = mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue,
				MFC_BUF_NO_TOUCH_USED);
		if (src_mb)
			vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
	}

	/* Check multi-frame */
	consumed = pOutStr->DecodedNalSize;
	src_mb = mfc_get_del_if_consumed(ctx, &ctx->src_buf_nal_queue,
			consumed, STUFF_BYTE, err, &deleted);
	if (!src_mb) {
		mfc_err_dev("[NALQ] no src buffers\n");
		return;
	}

	index = src_mb->vb.vb2_buf.index;
	mfc_debug(2, "[NALQ][BUFINFO] ctx[%d] get src index: %d, addr: 0x%08llx\n",
			ctx->num, index, src_mb->addr[0][0]);

	if (!deleted) {
		/* Run MFC again on the same buffer */
		mfc_debug(2, "[NALQ][MULTIFRAME] Running again the same buffer\n");

		if (CODEC_MULTIFRAME(ctx))
			dec->y_addr_for_pb = (dma_addr_t)pOutStr->DecodedAddr[0];

		dec->consumed = consumed;
		dec->remained_size = src_mb->vb.vb2_buf.planes[0].bytesused
			- dec->consumed;
		dec->has_multiframe = 1;
		dev->nal_q_handle->nal_q_exception = 1;

		MFC_TRACE_CTX("** consumed:%ld, remained:%ld, addr:0x%08llx\n",
			dec->consumed, dec->remained_size, dec->y_addr_for_pb);
		/* Do not move src buffer to done_list */
		return;
	}

	if (call_cop(ctx, get_buf_ctrls_val_nal_q_dec, ctx,
				&ctx->src_ctrls[index], pOutStr) < 0)
		mfc_err_ctx("[NALQ] failed in get_buf_ctrls_val\n");

	dec->consumed = 0;
	dec->remained_size = 0;

	vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
}

void __mfc_nal_q_handle_frame(struct mfc_ctx *ctx, DecoderOutputStr *pOutStr)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	unsigned int dst_frame_status, sei_avail_status, need_empty_dpb;
	unsigned int res_change, need_dpb_change, need_scratch_change;
	unsigned int is_interlaced, err;

	mfc_debug_enter();

	dst_frame_status = pOutStr->DisplayStatus
				& MFC_REG_DISP_STATUS_DISPLAY_STATUS_MASK;
	need_empty_dpb = (pOutStr->DisplayStatus
				>> MFC_REG_DISP_STATUS_NEED_EMPTY_DPB_SHIFT)
				& MFC_REG_DISP_STATUS_NEED_EMPTY_DPB_MASK;
	res_change = (pOutStr->DisplayStatus
				>> MFC_REG_DISP_STATUS_RES_CHANGE_SHIFT)
				& MFC_REG_DISP_STATUS_RES_CHANGE_MASK;
	need_dpb_change = (pOutStr->DisplayStatus
				>> MFC_REG_DISP_STATUS_NEED_DPB_CHANGE_SHIFT)
				& MFC_REG_DISP_STATUS_NEED_DPB_CHANGE_MASK;
	need_scratch_change = (pOutStr->DisplayStatus
				 >> MFC_REG_DISP_STATUS_NEED_SCRATCH_CHANGE_SHIFT)
				& MFC_REG_DISP_STATUS_NEED_SCRATCH_CHANGE_MASK;
	is_interlaced = (pOutStr->DecodedStatus
				>> MFC_REG_DEC_STATUS_INTERLACE_SHIFT)
				& MFC_REG_DEC_STATUS_INTERLACE_MASK;
	sei_avail_status = pOutStr->SeiAvail;
	err = pOutStr->ErrorCode;

	if (dec->immediate_display == 1)
		dst_frame_status = pOutStr->DecodedStatus
				& MFC_REG_DEC_STATUS_DECODED_STATUS_MASK;

	mfc_debug(2, "[NALQ][FRAME] frame status: %d\n", dst_frame_status);
	mfc_debug(2, "[NALQ][FRAME] display status: %d, type: %d, yaddr: %#x\n",
			pOutStr->DisplayStatus & MFC_REG_DISP_STATUS_DISPLAY_STATUS_MASK,
			pOutStr->DisplayFrameType & MFC_REG_DISPLAY_FRAME_MASK,
			pOutStr->DisplayAddr[0]);
	mfc_debug(2, "[NALQ][FRAME] decoded status: %d, type: %d, yaddr: %#x\n",
			pOutStr->DecodedStatus & MFC_REG_DEC_STATUS_DECODED_STATUS_MASK,
			pOutStr->DecodedFrameType & MFC_REG_DECODED_FRAME_MASK,
			pOutStr->DecodedAddr[0]);
	mfc_debug(4, "[NALQ][HDR] SEI available status: %x\n", sei_avail_status);
	mfc_debug(2, "[NALQ][DPB] Used flag: old = %08x, new = %08x\n",
			dec->dynamic_used, pOutStr->UsedDpbFlagLower);

	if (ctx->state == MFCINST_RES_CHANGE_INIT) {
		mfc_debug(2, "[NALQ][DRC] return until NAL-Q stopped in try_run\n");
		goto leave_handle_frame;
	}
	if (res_change) {
		mfc_debug(2, "[NALQ][DRC] Resolution change set to %d\n", res_change);
		mfc_change_state(ctx, MFCINST_RES_CHANGE_INIT);
		ctx->wait_state = WAIT_G_FMT | WAIT_STOP;
		dev->nal_q_handle->nal_q_exception = 1;
		mfc_info_ctx("[NALQ][DRC] nal_q_exception is set (res change)\n");
		goto leave_handle_frame;
	}
	if (need_empty_dpb) {
		mfc_debug(2, "[NALQ][MULTIFRAME] There is multi-frame. consumed:%ld\n", dec->consumed);
		dec->has_multiframe = 1;
		dev->nal_q_handle->nal_q_exception = 1;
		mfc_info_ctx("[NALQ][MULTIFRAME] nal_q_exception is set\n");
		goto leave_handle_frame;
	}
	if (need_dpb_change || need_scratch_change) {
		mfc_err_ctx("[NALQ][DRC] Interframe resolution change is not supported\n");
		dev->nal_q_handle->nal_q_exception = 1;
		mfc_info_ctx("[NALQ][DRC] nal_q_exception is set (interframe res change)\n");
		mfc_change_state(ctx, MFCINST_ERROR);
		goto leave_handle_frame;
	}
	/*
	 * H264/VC1/MPEG2/MPEG4 can have interlace type
	 * Only MPEG4 can continue to use NALQ
	 * because MPEG4 doesn't handle field unit.
	 */
	if (is_interlaced && !IS_MPEG4_DEC(ctx)) {
		mfc_debug(2, "[NALQ][INTERLACE] Progressive -> Interlaced\n");
		dec->is_interlaced = is_interlaced;
		dev->nal_q_handle->nal_q_exception = 1;
		mfc_info_ctx("[NALQ][INTERLACE] nal_q_exception is set\n");
		goto leave_handle_frame;
	}

	if (mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue, 0) &&
		mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_nal_queue, 0)) {
		mfc_err_dev("[NALQ] Queue count is zero for src/dst\n");
		goto leave_handle_frame;
	}

	/* Detection for QoS weight */
	if (!dec->num_of_tile_over_4 && (((pOutStr->DisplayStatus
				>> MFC_REG_DEC_STATUS_NUM_OF_TILE_SHIFT)
				& MFC_REG_DEC_STATUS_NUM_OF_TILE_MASK) >= 4)) {
		dec->num_of_tile_over_4 = 1;
		mfc_qos_on(ctx);
	}
	if (!dec->super64_bframe && IS_SUPER64_BFRAME(ctx,
				(pOutStr->HevcInfo & MFC_REG_D_HEVC_INFO_LCU_SIZE_MASK),
				(pOutStr->DecodedFrameType & MFC_REG_DECODED_FRAME_MASK))) {
		dec->super64_bframe = 1;
		mfc_qos_on(ctx);
	}

	switch (dst_frame_status) {
	case MFC_REG_DEC_STATUS_DECODING_DISPLAY:
		__mfc_nal_q_handle_ref_frame(ctx, pOutStr);
		__mfc_nal_q_handle_frame_copy_timestamp(ctx, pOutStr);
		break;
	case MFC_REG_DEC_STATUS_DECODING_ONLY:
		__mfc_nal_q_handle_ref_frame(ctx, pOutStr);
		__mfc_nal_q_handle_reuse_buffer(ctx, pOutStr);
		break;
	default:
		break;
	}

	/* A frame has been decoded and is in the buffer  */
	if (mfc_dec_status_display(dst_frame_status))
		__mfc_nal_q_handle_frame_new(ctx, err, pOutStr);
	else
		mfc_debug(2, "[NALQ] No display frame\n");

	/* Mark source buffer as complete */
	if (dst_frame_status != MFC_REG_DEC_STATUS_DISPLAY_ONLY)
		__mfc_nal_q_handle_frame_input(ctx, err, pOutStr);
	else
		mfc_debug(2, "[NALQ][DPB] can't support display only in NAL-Q, is_dpb_full: %d\n",
				dec->is_dpb_full);

leave_handle_frame:
	mfc_debug_leave();
}

int __mfc_nal_q_handle_error(struct mfc_ctx *ctx, EncoderOutputStr *pOutStr, int err)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec;
	struct mfc_enc *enc;
	struct mfc_buf *src_mb;
	int stop_nal_q = 1;

	mfc_debug_enter();

	mfc_err_ctx("[NALQ] Interrupt Error: %d\n", pOutStr->ErrorCode);

	dev->nal_q_handle->nal_q_exception = 1;
	mfc_info_ctx("[NALQ] nal_q_exception is set (error)\n");

	if (ctx->type == MFCINST_DECODER) {
		dec = ctx->dec_priv;
		if (!dec) {
			mfc_err_dev("[NALQ] no mfc decoder to run\n");
			goto end;
		}
		src_mb = mfc_get_del_buf(&ctx->buf_queue_lock,
				&ctx->src_buf_nal_queue, MFC_BUF_NO_TOUCH_USED);

		if (!src_mb) {
			mfc_err_dev("[NALQ] no src buffers\n");
		} else {
			dec->consumed = 0;
			vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_ERROR);
		}
	} else if (ctx->type == MFCINST_ENCODER) {
		enc = ctx->enc_priv;
		if (!enc) {
			mfc_err_dev("[NALQ] no mfc encoder to run\n");
			goto end;
		}

		/*
		 * If the buffer full error occurs in NAL-Q mode,
		 * one input buffer is returned and the NAL-Q mode continues.
		 */
		if (err == MFC_REG_ERR_BUFFER_FULL) {
			src_mb = mfc_get_del_buf(&ctx->buf_queue_lock,
					&ctx->src_buf_nal_queue, MFC_BUF_NO_TOUCH_USED);

			if (!src_mb)
				mfc_err_dev("[NALQ] no src buffers\n");
			else
				vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_ERROR);

			dev->nal_q_handle->nal_q_exception = 0;
			stop_nal_q = 0;
		}
	}

end:
	mfc_debug_leave();

	return stop_nal_q;
}

int mfc_nal_q_handle_out_buf(struct mfc_dev *dev, EncoderOutputStr *pOutStr)
{
	struct mfc_ctx *ctx;
	struct mfc_enc *enc;
	struct mfc_dec *dec;
	int ctx_num;

	mfc_debug_enter();

	ctx_num = dev->nal_q_handle->nal_q_out_handle->nal_q_ctx;
	if (ctx_num < 0) {
		mfc_err_dev("[NALQ] Can't find ctx in nal q\n");
		return -EINVAL;
	}

	ctx = dev->ctx[ctx_num];
	if (!ctx) {
		mfc_err_dev("[NALQ] no mfc context to run\n");
		return -EINVAL;
	}

	mfc_debug(2, "[NALQ] Int ctx is %d(%s)\n", ctx_num,
			 ctx->type == MFCINST_ENCODER ? "enc" : "dec");

	if (mfc_get_err(pOutStr->ErrorCode) &&
			(mfc_get_err(pOutStr->ErrorCode) < MFC_REG_ERR_FRAME_CONCEAL)) {
		if (__mfc_nal_q_handle_error(ctx, pOutStr, mfc_get_err(pOutStr->ErrorCode)))
			return 0;
	}

	if (ctx->type == MFCINST_ENCODER) {
		enc = ctx->enc_priv;
		if (!enc) {
			mfc_err_dev("[NALQ] no mfc encoder to run\n");
			return -EINVAL;
		}
		__mfc_nal_q_handle_stream(ctx, pOutStr);
	} else if (ctx->type == MFCINST_DECODER) {
		dec = ctx->dec_priv;
		if (!dec) {
			mfc_err_dev("[NALQ] no mfc decoder to run\n");
			return -EINVAL;
		}
		__mfc_nal_q_handle_frame(ctx, (DecoderOutputStr *)pOutStr);
	}

	mfc_debug_leave();

	return 0;
}

/*
 * This function should be called in NAL_Q_STATE_STARTED state.
 */
int mfc_nal_q_enqueue_in_buf(struct mfc_dev *dev, struct mfc_ctx *ctx,
	nal_queue_in_handle *nal_q_in_handle)
{
	unsigned long flags;
	unsigned int input_count = 0;
	unsigned int input_exe_count = 0;
	int input_diff = 0;
	unsigned int index = 0, offset = 0;
	EncoderInputStr *pStr = NULL;
	int ret = 0;

	mfc_debug_enter();

	if (!nal_q_in_handle) {
		mfc_err_dev("[NALQ] There is no nal_q_handle\n");
		return -EINVAL;
	}

	if (nal_q_in_handle->nal_q_handle->nal_q_state != NAL_Q_STATE_STARTED) {
		mfc_err_dev("[NALQ] State is wrong, state: %d\n",
				nal_q_in_handle->nal_q_handle->nal_q_state);
		return -EINVAL;
	}

	spin_lock_irqsave(&nal_q_in_handle->nal_q_handle->lock, flags);

	input_count = mfc_get_nal_q_input_count();
	input_exe_count = mfc_get_nal_q_input_exe_count();
	nal_q_in_handle->in_exe_count = input_exe_count;
	input_diff = input_count - input_exe_count;

	/*
	 * meaning of the variable input_diff
	 * 0:				number of available slots = NAL_Q_QUEUE_SIZE
	 * 1:				number of available slots = NAL_Q_QUEUE_SIZE - 1
	 * ...
	 * NAL_Q_QUEUE_SIZE-1:		number of available slots = 1
	 * NAL_Q_QUEUE_SIZE:		number of available slots = 0
	 */

	mfc_debug(2, "[NALQ] input_diff = %d(in: %d, exe: %d)\n",
			input_diff, input_count, input_exe_count);

	if ((input_diff < 0) || (input_diff >= NAL_Q_QUEUE_SIZE)) {
		mfc_err_dev("[NALQ] No available input slot(%d)\n", input_diff);
		spin_unlock_irqrestore(&nal_q_in_handle->nal_q_handle->lock, flags);
		return -EINVAL;
	}

	index = input_count % NAL_Q_QUEUE_SIZE;
	offset = dev->pdata->nal_q_entry_size * index;
	pStr = (EncoderInputStr *)(nal_q_in_handle->nal_q_in_addr + offset);

	memset(pStr, 0, dev->pdata->nal_q_entry_size);

	if (ctx->type == MFCINST_ENCODER)
		ret = __mfc_nal_q_run_in_buf_enc(ctx, pStr);
	else if (ctx->type == MFCINST_DECODER)
		ret = __mfc_nal_q_run_in_buf_dec(ctx, (DecoderInputStr *)pStr);

	if (ret != 0) {
		mfc_debug(2, "[NALQ] Failed to set input queue\n");
		spin_unlock_irqrestore(&nal_q_in_handle->nal_q_handle->lock, flags);
		return ret;
	}

	if (nal_q_dump == 1) {
		mfc_err_dev("[NAL-Q][DUMP][%s INPUT][c: %d] diff: %d, count: %d, exe: %d\n",
				ctx->type == MFCINST_ENCODER ? "ENC" : "DEC", dev->curr_ctx,
				input_diff, input_count, input_exe_count);
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
				(int *)pStr, dev->pdata->nal_q_dump_size, false);
		printk("...\n");
	}
	input_count++;

	mfc_update_nal_queue_input_count(dev, input_count);

	if (input_diff == 0)
		mfc_watchdog_start_tick(dev);
	MFC_TRACE_LOG_DEV("N%d", input_diff);

	spin_unlock_irqrestore(&nal_q_in_handle->nal_q_handle->lock, flags);

	MFC_TRACE_CTX("NAL %s in: diff %d count %d exe %d\n",
			ctx->type == MFCINST_ENCODER ? "ENC" : "DEC",
			input_diff, input_count, input_exe_count);

	mfc_debug_leave();

	return ret;
}

/*
 * This function should be called in NAL_Q_STATE_STARTED state.
 */
EncoderOutputStr *mfc_nal_q_dequeue_out_buf(struct mfc_dev *dev,
	nal_queue_out_handle *nal_q_out_handle, unsigned int *reason)
{
	struct mfc_ctx *ctx;
	unsigned long flags;
	unsigned int output_count = 0;
	unsigned int output_exe_count = 0;
	int input_diff = 0;
	int output_diff = 0;
	unsigned int index = 0, offset = 0;
	EncoderOutputStr *pStr = NULL;

	mfc_debug_enter();

	if (!nal_q_out_handle || !nal_q_out_handle->nal_q_out_addr) {
		mfc_err_dev("[NALQ] There is no handle\n");
		return pStr;
	}

	spin_lock_irqsave(&nal_q_out_handle->nal_q_handle->lock, flags);

	output_count = mfc_get_nal_q_output_count();
	output_exe_count = nal_q_out_handle->out_exe_count;
	output_diff = output_count - output_exe_count;

	/*
	 * meaning of the variable output_diff
	 * 0:				number of output slots = 0
	 * 1:				number of output slots = 1
	 * ...
	 * NAL_Q_QUEUE_SIZE-1:		number of output slots = NAL_Q_QUEUE_SIZE - 1
	 * NAL_Q_QUEUE_SIZE:		number of output slots = NAL_Q_QUEUE_SIZE
	 */

	mfc_debug(2, "[NALQ] output_diff = %d(out: %d, exe: %d)\n",
			output_diff, output_count, output_exe_count);
	if ((output_diff <= 0) || (output_diff > NAL_Q_QUEUE_SIZE)) {
		spin_unlock_irqrestore(&nal_q_out_handle->nal_q_handle->lock, flags);
		mfc_debug(2, "[NALQ] No available output slot(%d)\n", output_diff);
		return pStr;
	}

	index = output_exe_count % NAL_Q_QUEUE_SIZE;
	offset = dev->pdata->nal_q_entry_size * index;
	pStr = (EncoderOutputStr *)(nal_q_out_handle->nal_q_out_addr + offset);

	nal_q_out_handle->nal_q_ctx = __mfc_nal_q_find_ctx(dev, pStr);
	if (nal_q_out_handle->nal_q_ctx < 0) {
		mfc_err_dev("[NALQ] Can't find ctx in nal q\n");
		pStr = NULL;
		return pStr;
	}

	ctx = dev->ctx[nal_q_out_handle->nal_q_ctx];
	if (nal_q_dump == 1) {
		mfc_err_dev("[NALQ][DUMP][%s OUTPUT][c: %d] diff: %d, count: %d, exe: %d\n",
				ctx->type == MFCINST_ENCODER ? "ENC" : "DEC",
				nal_q_out_handle->nal_q_ctx,
				output_diff, output_count, output_exe_count);
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
				(int *)pStr, dev->pdata->nal_q_dump_size, false);
		printk("...\n");
	}
	nal_q_out_handle->out_exe_count++;

	if (pStr->ErrorCode) {
		*reason = MFC_REG_R2H_CMD_ERR_RET;
		mfc_err_dev("[NALQ] Error : %d\n", pStr->ErrorCode);
	}

	input_diff = mfc_get_nal_q_input_count() - mfc_get_nal_q_input_exe_count();
	if (input_diff == 0) {
		mfc_watchdog_stop_tick(dev);
	} else if (input_diff > 0) {
		mfc_watchdog_reset_tick(dev);
	}

	spin_unlock_irqrestore(&nal_q_out_handle->nal_q_handle->lock, flags);

	MFC_TRACE_CTX("NAL %s out: diff %d count %d exe %d\n",
			ctx->type == MFCINST_ENCODER ? "ENC" : "DEC",
			output_diff, output_count, output_exe_count);

	mfc_debug_leave();

	return pStr;
}

#if 0
/* not used function - only for reference sfr <-> structure */
void mfc_nal_q_fill_DecoderInputStr(struct mfc_dev *dev, DecoderInputStr *pStr)
{
	pStr->StartCode			= 0xAAAAAAAA; // Decoder input start
//	pStr->CommandId			= MFC_READL(MFC_REG_HOST2RISC_CMD);		// 0x1100
	pStr->InstanceId		= MFC_READL(MFC_REG_INSTANCE_ID);		// 0xF008
	pStr->PictureTag		= MFC_READL(MFC_REG_D_PICTURE_TAG);		// 0xF5C8
	pStr->CpbBufferAddr		= MFC_READL(MFC_REG_D_CPB_BUFFER_ADDR);	// 0xF5B0
	pStr->CpbBufferSize		= MFC_READL(MFC_REG_D_CPB_BUFFER_SIZE);	// 0xF5B4
	pStr->CpbBufferOffset		= MFC_READL(MFC_REG_D_CPB_BUFFER_OFFSET);	// 0xF5C0
	pStr->StreamDataSize		= MFC_READL(MFC_REG_D_STREAM_DATA_SIZE);	// 0xF5D0
	pStr->AvailableDpbFlagUpper	= MFC_READL(MFC_REG_D_AVAILABLE_DPB_FLAG_UPPER);// 0xF5B8
	pStr->AvailableDpbFlagLower	= MFC_READL(MFC_REG_D_AVAILABLE_DPB_FLAG_LOWER);// 0xF5BC
	pStr->DynamicDpbFlagUpper	= MFC_READL(MFC_REG_D_DYNAMIC_DPB_FLAG_UPPER);	// 0xF5D4
	pStr->DynamicDpbFlagLower	= MFC_READL(MFC_REG_D_DYNAMIC_DPB_FLAG_LOWER);	// 0xF5D8
	pStr->FirstPlaneDpb		= MFC_READL(MFC_REG_D_FIRST_PLANE_DPB0);	// 0xF160+(index*4)
	pStr->SecondPlaneDpb		= MFC_READL(MFC_REG_D_SECOND_PLANE_DPB0);	// 0xF260+(index*4)
	pStr->ThirdPlaneDpb		= MFC_READL(MFC_REG_D_THIRD_PLANE_DPB0);	// 0xF360+(index*4)
	pStr->FirstPlaneDpbSize		= MFC_READL(MFC_REG_D_FIRST_PLANE_DPB_SIZE);	// 0xF144
	pStr->SecondPlaneDpbSize	= MFC_READL(MFC_REG_D_SECOND_PLANE_DPB_SIZE);	// 0xF148
	pStr->ThirdPlaneDpbSize		= MFC_READL(MFC_REG_D_THIRD_PLANE_DPB_SIZE);	// 0xF14C
	pStr->NalStartOptions		= MFC_READL(0xF5AC);// MFC_REG_D_NAL_START_OPTIONS 0xF5AC
	pStr->FirstPlaneStrideSize	= MFC_READL(MFC_REG_D_FIRST_PLANE_DPB_STRIDE_SIZE);// 0xF138
	pStr->SecondPlaneStrideSize	= MFC_READL(MFC_REG_D_SECOND_PLANE_DPB_STRIDE_SIZE);// 0xF13C
	pStr->ThirdPlaneStrideSize	= MFC_READL(MFC_REG_D_THIRD_PLANE_DPB_STRIDE_SIZE);// 0xF140
	pStr->FirstPlane2BitDpbSize	= MFC_READL(MFC_REG_D_FIRST_PLANE_2BIT_DPB_SIZE);// 0xF578
	pStr->SecondPlane2BitDpbSize	= MFC_READL(MFC_REG_D_SECOND_PLANE_2BIT_DPB_SIZE);// 0xF57C
	pStr->FirstPlane2BitStrideSize	= MFC_READL(MFC_REG_D_FIRST_PLANE_2BIT_DPB_STRIDE_SIZE);// 0xF580
	pStr->SecondPlane2BitStrideSize	= MFC_READL(MFC_REG_D_SECOND_PLANE_2BIT_DPB_STRIDE_SIZE);// 0xF584
	pStr->ScratchBufAddr		= MFC_READL(MFC_REG_D_SCRATCH_BUFFER_ADDR);	// 0xF560
	pStr->ScratchBufSize		= MFC_READL(MFC_REG_D_SCRATCH_BUFFER_SIZE);	// 0xF564
}

void mfc_nal_q_flush_DecoderOutputStr(struct mfc_dev *dev, DecoderOutputStr *pStr)
{
	//pStr->StartCode; // 0xAAAAAAAA; // Decoder output start
//	MFC_WRITEL(pStr->CommandId, MFC_REG_RISC2HOST_CMD);				// 0x1104
	MFC_WRITEL(pStr->InstanceId, MFC_REG_RET_INSTANCE_ID);				// 0xF070
	MFC_WRITEL(pStr->ErrorCode, MFC_REG_ERROR_CODE);				// 0xF074
	MFC_WRITEL(pStr->PictureTagTop, MFC_REG_D_RET_PICTURE_TAG_TOP);		// 0xF674
	MFC_WRITEL(pStr->PictureTimeTop, MFC_REG_D_RET_PICTURE_TIME_TOP);		// 0xF67C
	MFC_WRITEL(pStr->DisplayFrameWidth, MFC_REG_D_DISPLAY_FRAME_WIDTH);		// 0xF600
	MFC_WRITEL(pStr->DisplayFrameHeight, MFC_REG_D_DISPLAY_FRAME_HEIGHT);		// 0xF604
	MFC_WRITEL(pStr->DisplayStatus, MFC_REG_D_DISPLAY_STATUS);			// 0xF608
	MFC_WRITEL(pStr->DisplayFirstPlaneAddr, MFC_REG_D_DISPLAY_FIRST_PLANE_ADDR);	// 0xF60C
	MFC_WRITEL(pStr->DisplaySecondPlaneAddr, MFC_REG_D_DISPLAY_SECOND_PLANE_ADDR);	// 0xF610
	MFC_WRITEL(pStr->DisplayThirdPlaneAddr,MFC_REG_D_DISPLAY_THIRD_PLANE_ADDR);	// 0xF614
	MFC_WRITEL(pStr->DisplayFrameType, MFC_REG_D_DISPLAY_FRAME_TYPE);		// 0xF618
	MFC_WRITEL(pStr->DisplayCropInfo1, MFC_REG_D_DISPLAY_CROP_INFO1);		// 0xF61C
	MFC_WRITEL(pStr->DisplayCropInfo2, MFC_REG_D_DISPLAY_CROP_INFO2);		// 0xF620
	MFC_WRITEL(pStr->DisplayPictureProfile, MFC_REG_D_DISPLAY_PICTURE_PROFILE);	// 0xF624
	MFC_WRITEL(pStr->DisplayAspectRatio, MFC_REG_D_DISPLAY_ASPECT_RATIO);		// 0xF634
	MFC_WRITEL(pStr->DisplayExtendedAr, MFC_REG_D_DISPLAY_EXTENDED_AR);		// 0xF638
	MFC_WRITEL(pStr->DecodedNalSize, MFC_REG_D_DECODED_NAL_SIZE);			// 0xF664
	MFC_WRITEL(pStr->UsedDpbFlagUpper, MFC_REG_D_USED_DPB_FLAG_UPPER);		// 0xF720
	MFC_WRITEL(pStr->UsedDpbFlagLower, MFC_REG_D_USED_DPB_FLAG_LOWER);		// 0xF724
	MFC_WRITEL(pStr->SeiAvail, MFC_REG_D_SEI_AVAIL);				// 0xF6DC
	MFC_WRITEL(pStr->FramePackArrgmentId, MFC_REG_D_FRAME_PACK_ARRGMENT_ID);	// 0xF6E0
	MFC_WRITEL(pStr->FramePackSeiInfo, MFC_REG_D_FRAME_PACK_SEI_INFO);		// 0xF6E4
	MFC_WRITEL(pStr->FramePackGridPos, MFC_REG_D_FRAME_PACK_GRID_POS);		// 0xF6E8
	MFC_WRITEL(pStr->DisplayRecoverySeiInfo, MFC_REG_D_DISPLAY_RECOVERY_SEI_INFO);	// 0xF6EC
	MFC_WRITEL(pStr->H264Info, MFC_REG_D_H264_INFO);				// 0xF690
	MFC_WRITEL(pStr->DisplayFirstCrc, MFC_REG_D_DISPLAY_FIRST_PLANE_CRC);		// 0xF628
	MFC_WRITEL(pStr->DisplaySecondCrc, MFC_REG_D_DISPLAY_SECOND_PLANE_CRC);	// 0xF62C
	MFC_WRITEL(pStr->DisplayThirdCrc, MFC_REG_D_DISPLAY_THIRD_PLANE_CRC);		// 0xF630
	MFC_WRITEL(pStr->DisplayFirst2BitCrc, MFC_REG_D_DISPLAY_FIRST_PLANE_2BIT_CRC);	// 0xF6FC
	MFC_WRITEL(pStr->DisplaySecond2BitCrc, MFC_REG_D_DISPLAY_SECOND_PLANE_2BIT_CRC);// 0xF700
	MFC_WRITEL(pStr->DecodedFrameWidth, MFC_REG_D_DECODED_FRAME_WIDTH);		// 0xF63C
	MFC_WRITEL(pStr->DecodedFrameHeight, MFC_REG_D_DECODED_FRAME_HEIGHT);		// 0xF640
	MFC_WRITEL(pStr->DecodedStatus, MFC_REG_D_DECODED_STATUS);			// 0xF644
	MFC_WRITEL(pStr->DecodedFirstPlaneAddr, MFC_REG_D_DECODED_FIRST_PLANE_ADDR);	// 0xF648
	MFC_WRITEL(pStr->DecodedSecondPlaneAddr, MFC_REG_D_DECODED_SECOND_PLANE_ADDR);	// 0xF64C
	MFC_WRITEL(pStr->DecodedThirdPlaneAddr, MFC_REG_D_DECODED_THIRD_PLANE_ADDR);	// 0xF650
	MFC_WRITEL(pStr->DecodedFrameType, MFC_REG_D_DECODED_FRAME_TYPE);		// 0xF654
	MFC_WRITEL(pStr->DecodedCropInfo1, MFC_REG_D_DECODED_CROP_INFO1);		// 0xF658
	MFC_WRITEL(pStr->DecodedCropInfo2, MFC_REG_D_DECODED_CROP_INFO2);		// 0xF65C
	MFC_WRITEL(pStr->DecodedPictureProfile, MFC_REG_D_DECODED_PICTURE_PROFILE);	// 0xF660
	MFC_WRITEL(pStr->DecodedRecoverySeiInfo, MFC_REG_D_DECODED_RECOVERY_SEI_INFO);	// 0xF6F0
	MFC_WRITEL(pStr->DecodedFirstCrc, MFC_REG_D_DECODED_FIRST_PLANE_CRC);		// 0xF668
	MFC_WRITEL(pStr->DecodedSecondCrc, MFC_REG_D_DECODED_SECOND_PLANE_CRC);	// 0xF66C
	MFC_WRITEL(pStr->DecodedThirdCrc, MFC_REG_D_DECODED_THIRD_PLANE_CRC);		// 0xF670
	MFC_WRITEL(pStr->DecodedFirst2BitCrc, MFC_REG_D_DECODED_FIRST_PLANE_2BIT_CRC);	// 0xF704
	MFC_WRITEL(pStr->DecodedSecond2BitCrc, MFC_REG_D_DECODED_SECOND_PLANE_2BIT_CRC);// 0xF708
	MFC_WRITEL(pStr->PictureTagBot, MFC_REG_D_RET_PICTURE_TAG_BOT);		// 0xF678
	MFC_WRITEL(pStr->PictureTimeBot, MFC_REG_D_RET_PICTURE_TIME_BOT);		// 0xF680
}
#endif
