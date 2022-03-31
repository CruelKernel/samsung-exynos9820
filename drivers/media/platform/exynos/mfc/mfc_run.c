/*
 * drivers/media/platform/exynos/mfc/mfc_run.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "mfc_run.h"

#include "mfc_nal_q.h"
#include "mfc_sync.h"
#include "mfc_hwlock.h"

#include "mfc_pm.h"
#include "mfc_cmd.h"
#include "mfc_reg_api.h"
#include "mfc_hw_reg_api.h"
#include "mfc_enc_param.h"

#include "mfc_queue.h"
#include "mfc_utils.h"
#include "mfc_mem.h"

/* Initialize hardware */
static int __mfc_init_hw(struct mfc_dev *dev, enum mfc_buf_usage_type buf_type)
{
	int fw_ver;
	int ret = 0;
	int curr_ctx_is_drm_backup;

	mfc_debug_enter();

	curr_ctx_is_drm_backup = dev->curr_ctx_is_drm;

	if (!dev->fw_buf.dma_buf)
		return -EINVAL;

	/* 0. MFC reset */
	mfc_debug(2, "MFC reset...\n");

	/* At init time, do not call secure API */
	if (buf_type == MFCBUF_NORMAL)
		dev->curr_ctx_is_drm = 0;
	else if (buf_type == MFCBUF_DRM)
		dev->curr_ctx_is_drm = 1;

	ret = mfc_pm_clock_on(dev);
	if (ret) {
		mfc_err_dev("Failed to enable clock before reset(%d)\n", ret);
		dev->curr_ctx_is_drm = curr_ctx_is_drm_backup;
		return ret;
	}

	mfc_reset_mfc(dev);
	mfc_debug(2, "Done MFC reset...\n");

	/* 1. Set DRAM base Addr */
	mfc_set_risc_base_addr(dev, buf_type);

	/* 2. Release reset signal to the RISC */
	mfc_risc_on(dev);

	mfc_debug(2, "Will now wait for completion of firmware transfer\n");
	if (mfc_wait_for_done_dev(dev, MFC_REG_R2H_CMD_FW_STATUS_RET)) {
		mfc_err_dev("Failed to RISC_ON\n");
		mfc_clean_dev_int_flags(dev);
		ret = -EIO;
		goto err_init_hw;
	}

	/* 3. Initialize firmware */
	mfc_cmd_sys_init(dev, buf_type);

	mfc_debug(2, "Ok, now will write a command to init the system\n");
	if (mfc_wait_for_done_dev(dev, MFC_REG_R2H_CMD_SYS_INIT_RET)) {
		mfc_err_dev("Failed to SYS_INIT\n");
		mfc_clean_dev_int_flags(dev);
		ret = -EIO;
		goto err_init_hw;
	}

	dev->int_condition = 0;
	if (dev->int_err != 0 || dev->int_reason != MFC_REG_R2H_CMD_SYS_INIT_RET) {
		/* Failure. */
		mfc_err_dev("Failed to init firmware - error: %d, int: %d\n",
				dev->int_err, dev->int_reason);
		ret = -EIO;
		goto err_init_hw;
	}

	dev->fw.fimv_info = mfc_get_fimv_info();
	if (dev->fw.fimv_info != 'D' && dev->fw.fimv_info != 'E')
		dev->fw.fimv_info = 'N';

	mfc_info_dev("[F/W] MFC v%x.%x, %02xyy %02xmm %02xdd (%c)\n",
		 MFC_VER_MAJOR(dev),
		 MFC_VER_MINOR(dev),
		 mfc_get_fw_ver_year(),
		 mfc_get_fw_ver_month(),
		 mfc_get_fw_ver_date(),
		 dev->fw.fimv_info);

	dev->fw.date = mfc_get_fw_ver_all();
	/* Check MFC version and F/W version */
	fw_ver = mfc_get_mfc_version();
	if (fw_ver != dev->pdata->ip_ver) {
		mfc_err_dev("Invalid F/W version(0x%x) for MFC H/W(0x%x)\n",
				fw_ver, dev->pdata->ip_ver);
		ret = -EIO;
		goto err_init_hw;
	}

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	/* Cache flush for base address change */
	mfc_cmd_cache_flush(dev);
	if (mfc_wait_for_done_dev(dev, MFC_REG_R2H_CMD_CACHE_FLUSH_RET)) {
		mfc_err_dev("Failed to CACHE_FLUSH\n");
		mfc_clean_dev_int_flags(dev);
		ret = -EIO;
		goto err_init_hw;
	}

	if (buf_type == MFCBUF_DRM && !curr_ctx_is_drm_backup) {
		mfc_pm_clock_off(dev);
		dev->curr_ctx_is_drm = curr_ctx_is_drm_backup;
		mfc_pm_clock_on_with_base(dev, MFCBUF_NORMAL);
	}
#endif

err_init_hw:
	mfc_pm_clock_off(dev);
	dev->curr_ctx_is_drm = curr_ctx_is_drm_backup;
	mfc_debug_leave();

	return ret;
}

/* Wrapper : Initialize hardware */
int mfc_run_init_hw(struct mfc_dev *dev)
{
	int ret;

	ret = __mfc_init_hw(dev, MFCBUF_NORMAL);
	if (ret)
		return ret;

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	if (dev->fw.drm_status)
		ret = __mfc_init_hw(dev, MFCBUF_DRM);
#endif

	return ret;
}

/* Deinitialize hardware */
void mfc_run_deinit_hw(struct mfc_dev *dev)
{
	int ret;

	mfc_debug(2, "mfc deinit start\n");

	ret = mfc_pm_clock_on(dev);
	if (ret) {
		mfc_err_dev("Failed to enable clock before reset(%d)\n", ret);
		return;
	}

	mfc_mfc_off(dev);

	mfc_pm_clock_off(dev);

	mfc_debug(2, "mfc deinit completed\n");
}

int mfc_run_sleep(struct mfc_dev *dev)
{
	struct mfc_ctx *ctx;
	int i;
	int need_cache_flush = 0;

	mfc_debug_enter();

	ctx = dev->ctx[dev->curr_ctx];
	if (!ctx) {
		for (i = 0; i < MFC_NUM_CONTEXTS; i++) {
			if (dev->ctx[i]) {
				ctx = dev->ctx[i];
				break;
			}
		}

		if (!ctx) {
			mfc_err_dev("no mfc context to run\n");
			return -EINVAL;
		}
		mfc_info_dev("ctx is changed %d -> %d\n", dev->curr_ctx, ctx->num);

		dev->curr_ctx = ctx->num;
		if (dev->curr_ctx_is_drm != ctx->is_drm) {
			need_cache_flush = 1;
			mfc_info_dev("DRM attribute is changed %d->%d\n",
					dev->curr_ctx_is_drm, ctx->is_drm);
		}
	}
	mfc_info_dev("curr_ctx_is_drm:%d\n", dev->curr_ctx_is_drm);

	mfc_pm_clock_on(dev);

	if (need_cache_flush)
		mfc_cache_flush(dev, ctx->is_drm);

	mfc_cmd_sleep(dev);

	if (mfc_wait_for_done_dev(dev, MFC_REG_R2H_CMD_SLEEP_RET)) {
		mfc_err_dev("Failed to SLEEP\n");
		dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_SLEEP);
		call_dop(dev, dump_and_stop_always, dev);
		return -EBUSY;
	}

	dev->int_condition = 0;
	if (dev->int_err != 0 || dev->int_reason != MFC_REG_R2H_CMD_SLEEP_RET) {
		/* Failure. */
		mfc_err_dev("Failed to sleep - error: %d, int: %d\n",
				dev->int_err, dev->int_reason);
		call_dop(dev, dump_and_stop_always, dev);
		return -EBUSY;
	}

	dev->sleep = 1;

	mfc_mfc_off(dev);
	mfc_pm_clock_off(dev);

	mfc_debug_leave();

	return 0;
}

int mfc_run_wakeup(struct mfc_dev *dev)
{
	enum mfc_buf_usage_type buf_type;
	int ret = 0;

	mfc_debug_enter();
	mfc_info_dev("curr_ctx_is_drm:%d\n", dev->curr_ctx_is_drm);

	/* 0. MFC reset */
	mfc_debug(2, "MFC reset...\n");

	ret = mfc_pm_clock_on(dev);
	if (ret) {
		mfc_err_dev("Failed to enable clock before reset(%d)\n", ret);
		return ret;
	}

	mfc_reset_mfc(dev);
	mfc_debug(2, "Done MFC reset...\n");

	if (dev->curr_ctx_is_drm)
		buf_type = MFCBUF_DRM;
	else
		buf_type = MFCBUF_NORMAL;

	/* 1. Set DRAM base Addr */
	mfc_set_risc_base_addr(dev, buf_type);

	/* 2. Release reset signal to the RISC */
	mfc_risc_on(dev);

	mfc_debug(2, "Will now wait for completion of firmware transfer\n");
	if (mfc_wait_for_done_dev(dev, MFC_REG_R2H_CMD_FW_STATUS_RET)) {
		mfc_err_dev("Failed to RISC_ON\n");
		dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_RISC_ON);
		call_dop(dev, dump_and_stop_always, dev);
		return -EBUSY;
	}

	mfc_debug(2, "Ok, now will write a command to wakeup the system\n");
	mfc_cmd_wakeup(dev);

	mfc_debug(2, "Will now wait for completion of firmware wake up\n");
	if (mfc_wait_for_done_dev(dev, MFC_REG_R2H_CMD_WAKEUP_RET)) {
		mfc_err_dev("Failed to WAKEUP\n");
		dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_WAKEUP);
		call_dop(dev, dump_and_stop_always, dev);
		return -EBUSY;
	}

	dev->int_condition = 0;
	if (dev->int_err != 0 || dev->int_reason != MFC_REG_R2H_CMD_WAKEUP_RET) {
		/* Failure. */
		mfc_err_dev("Failed to wakeup - error: %d, int: %d\n",
				dev->int_err, dev->int_reason);
		call_dop(dev, dump_and_stop_always, dev);
		return -EBUSY;
	}

	dev->sleep = 0;

	mfc_pm_clock_off(dev);

	mfc_debug_leave();

	return ret;
}

int mfc_run_dec_init(struct mfc_ctx *ctx)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *src_mb;

	/* Initializing decoding - parsing header */

	/* Get the next source buffer */
	src_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
	if (!src_mb) {
		mfc_err_dev("no src buffers\n");
		return -EAGAIN;
	}

	mfc_debug(2, "Preparing to init decoding\n");
	mfc_debug(2, "[STREAM] Header size: %d, (offset: %lu)\n",
		src_mb->vb.vb2_buf.planes[0].bytesused, dec->consumed);

	if (dec->consumed) {
		mfc_set_dec_stream_buffer(ctx, src_mb, dec->consumed, dec->remained_size);
	} else {
		/* decoder src buffer CFW PROT */
		if (ctx->is_drm) {
			int index = src_mb->vb.vb2_buf.index;

			mfc_stream_protect(ctx, src_mb, index);
		}

		mfc_set_dec_stream_buffer(ctx, src_mb,
			0, src_mb->vb.vb2_buf.planes[0].bytesused);
	}

	mfc_debug(2, "[BUFINFO] Header addr: 0x%08llx\n", src_mb->addr[0][0]);
	mfc_clean_ctx_int_flags(ctx);
	mfc_cmd_dec_seq_header(ctx);

	return 0;
}

static int __mfc_check_last_frame(struct mfc_ctx *ctx, struct mfc_buf *mfc_buf)
{
	if (mfc_check_vb_flag(mfc_buf, MFC_FLAG_LAST_FRAME)) {
		mfc_debug(2, "Setting ctx->state to FINISHING\n");
		mfc_change_state(ctx, MFCINST_FINISHING);
		return 1;
	}

	return 0;
}

int mfc_run_dec_frame(struct mfc_ctx *ctx)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *src_mb, *dst_mb;
	int last_frame = 0;
	unsigned int index;

	if (mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0) &&
			mfc_is_queue_count_smaller(&ctx->buf_queue_lock,
				&ctx->ref_buf_queue, (ctx->dpb_count + 5))) {
		return -EAGAIN;
	}

	/* Get the next source buffer */
	src_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_SET_USED);
	if (!src_mb) {
		mfc_debug(2, "no src buffers\n");
		return -EAGAIN;
	}

	/* decoder src buffer CFW PROT */
	if (ctx->is_drm) {
		if (!dec->consumed) {
			index = src_mb->vb.vb2_buf.index;
			mfc_stream_protect(ctx, src_mb, index);
		}
	}

	if (mfc_check_vb_flag(src_mb, MFC_FLAG_EMPTY_DATA))
		src_mb->vb.vb2_buf.planes[0].bytesused = 0;

	if (dec->consumed)
		mfc_set_dec_stream_buffer(ctx, src_mb, dec->consumed, dec->remained_size);
	else
		mfc_set_dec_stream_buffer(ctx, src_mb, 0, src_mb->vb.vb2_buf.planes[0].bytesused);

	/* Try to use the non-referenced DPB on dst-queue */
	dst_mb = mfc_search_for_dpb(ctx, dec->dynamic_used);
	if (!dst_mb) {
		mfc_debug(2, "[DPB] couldn't find dst buffers\n");
		return -EAGAIN;
	}

	index = src_mb->vb.vb2_buf.index;
	if (call_cop(ctx, set_buf_ctrls_val, ctx, &ctx->src_ctrls[index]) < 0)
		mfc_err_ctx("failed in set_buf_ctrls_val\n");

	mfc_set_dynamic_dpb(ctx, dst_mb);

	mfc_clean_ctx_int_flags(ctx);

	last_frame = __mfc_check_last_frame(ctx, src_mb);
	mfc_cmd_dec_one_frame(ctx, last_frame);

	return 0;
}

int mfc_run_dec_last_frames(struct mfc_ctx *ctx)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *src_mb, *dst_mb;

	if (mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0)) {
		mfc_debug(2, "no dst buffer\n");
		return -EAGAIN;
	}

	/* Get the next source buffer */
	src_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_SET_USED);

	/* Frames are being decoded */
	if (!src_mb) {
		mfc_debug(2, "no src buffers\n");
		mfc_set_dec_stream_buffer(ctx, 0, 0, 0);
	} else {
		if (dec->consumed) {
			mfc_set_dec_stream_buffer(ctx, src_mb, dec->consumed, dec->remained_size);
		} else {
			/* decoder src buffer CFW PROT */
			if (ctx->is_drm) {
				int index = src_mb->vb.vb2_buf.index;

				mfc_stream_protect(ctx, src_mb, index);
			}

			mfc_set_dec_stream_buffer(ctx, src_mb, 0, 0);
		}
	}

	/* Try to use the non-referenced DPB on dst-queue */
	dst_mb = mfc_search_for_dpb(ctx, dec->dynamic_used);
	if (!dst_mb) {
		mfc_debug(2, "[DPB] couldn't find dst buffers\n");
		return -EAGAIN;
	}

	mfc_set_dynamic_dpb(ctx, dst_mb);

	mfc_clean_ctx_int_flags(ctx);
	mfc_cmd_dec_one_frame(ctx, 1);

	return 0;
}

int mfc_run_enc_init(struct mfc_ctx *ctx)
{
	struct mfc_buf *dst_mb;
	int ret;

	dst_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, MFC_BUF_NO_TOUCH_USED);
	if (!dst_mb) {
		mfc_debug(2, "no dst buffers\n");
		return -EAGAIN;
	}

	/* encoder dst buffer CFW PROT */
	if (ctx->is_drm) {
		int index = dst_mb->vb.vb2_buf.index;

		mfc_stream_protect(ctx, dst_mb, index);
	}
	mfc_set_enc_stream_buffer(ctx, dst_mb);

	mfc_set_enc_stride(ctx);

	mfc_debug(2, "[BUFINFO] Header addr: 0x%08llx\n", dst_mb->addr[0][0]);
	mfc_clean_ctx_int_flags(ctx);

	ret = mfc_cmd_enc_seq_header(ctx);
	return ret;
}

int mfc_run_enc_frame(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_buf *dst_mb;
	struct mfc_buf *src_mb;
	struct mfc_raw_info *raw;
	struct hdr10_plus_meta dst_sei_meta, *src_sei_meta;
	unsigned int index, i;
	int last_frame = 0;

	raw = &ctx->raw_buf;

	/* Get the next source buffer */
	src_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_SET_USED);
	if (!src_mb) {
		mfc_debug(2, "no src buffers\n");
		return -EAGAIN;
	}

	if (src_mb->num_valid_bufs > 0) {
		/* last image in a buffer container */
		if (src_mb->next_index == (src_mb->num_valid_bufs - 1)) {
			mfc_debug(4, "[BUFCON] last image in a container\n");
			last_frame = __mfc_check_last_frame(ctx, src_mb);
		}
	} else {
		last_frame = __mfc_check_last_frame(ctx, src_mb);
	}

	index = src_mb->vb.vb2_buf.index;

	/* encoder src buffer CFW PROT */
	if (ctx->is_drm)
		mfc_raw_protect(ctx, src_mb, index);

	mfc_set_enc_frame_buffer(ctx, src_mb, raw->num_planes);

	/* HDR10+ sei meta */
	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->hdr10_plus)) {
		if (enc->sh_handle_hdr.fd == -1) {
			mfc_debug(3, "[HDR+] there is no handle for SEI meta\n");
		} else {
			src_sei_meta = (struct hdr10_plus_meta *)enc->sh_handle_hdr.vaddr + index;
			if (src_sei_meta->valid) {
				mfc_debug(3, "[HDR+] there is valid SEI meta data in buf[%d]\n",
						index);
				memcpy(&dst_sei_meta, src_sei_meta, sizeof(struct hdr10_plus_meta));
				mfc_set_hdr_plus_info(ctx, &dst_sei_meta);
			}
		}
	}

	dst_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, MFC_BUF_SET_USED);
	if (!dst_mb) {
		mfc_debug(2, "no dst buffers\n");
		return -EAGAIN;
	}

	/* encoder dst buffer CFW PROT */
	if (ctx->is_drm) {
		i = dst_mb->vb.vb2_buf.index;
		mfc_stream_protect(ctx, dst_mb, i);
	}
	mfc_debug(2, "nal start : src index from src_buf_queue:%d\n",
		src_mb->vb.vb2_buf.index);
	mfc_debug(2, "nal start : dst index from dst_buf_queue:%d\n",
		dst_mb->vb.vb2_buf.index);

	mfc_set_enc_stream_buffer(ctx, dst_mb);

	if (call_cop(ctx, set_buf_ctrls_val, ctx, &ctx->src_ctrls[index]) < 0)
		mfc_err_ctx("failed in set_buf_ctrls_val\n");

	mfc_clean_ctx_int_flags(ctx);

	if (IS_H264_ENC(ctx))
		mfc_set_aso_slice_order_h264(ctx);

	if (!reg_test)
		mfc_set_slice_mode(ctx);
	mfc_set_enc_ts_delta(ctx);

	mfc_cmd_enc_one_frame(ctx, last_frame);

	return 0;
}

int mfc_run_enc_last_frames(struct mfc_ctx *ctx)
{
	struct mfc_buf *dst_mb = NULL;
	struct mfc_raw_info *raw;

	raw = &ctx->raw_buf;

	dst_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, MFC_BUF_SET_USED);
	if (!dst_mb)
		mfc_debug(2, "no dst buffers set to zero\n");

	mfc_debug(2, "Set address zero for all planes\n");
	mfc_set_enc_frame_buffer(ctx, 0, raw->num_planes);

	/* encoder dst buffer CFW PROT */
	if (ctx->is_drm && dst_mb) {
		int index = dst_mb->vb.vb2_buf.index;

		mfc_stream_protect(ctx, dst_mb, index);
	}

	mfc_set_enc_stream_buffer(ctx, dst_mb);

	mfc_clean_ctx_int_flags(ctx);
	mfc_cmd_enc_one_frame(ctx, 1);

	return 0;
}
