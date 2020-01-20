/*
 * drivers/media/platform/exynos/mfc/mfc_dec_ops.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include  "mfc_common.h"

#include "mfc_reg_api.h"

#define NUM_CTRL_CFGS ARRAY_SIZE(mfc_ctrl_list)

struct mfc_ctrl_cfg mfc_ctrl_list[] = {
	{
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_PICTURE_TAG,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_RET_PICTURE_TAG_TOP,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_DISPLAY_STATUS,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_DISPLAY_STATUS,
		.mask = 0x7,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	/* CRC related definitions are based on non-H.264 type */
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_LUMA,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_DISPLAY_FIRST_PLANE_CRC,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_CHROMA,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_DISPLAY_SECOND_PLANE_CRC,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_CHROMA1,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_DISPLAY_THIRD_PLANE_CRC,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_2BIT_LUMA,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_DISPLAY_FIRST_PLANE_2BIT_CRC,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_2BIT_CHROMA,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_DISPLAY_SECOND_PLANE_2BIT_CRC,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_CRC_GENERATED,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_DISPLAY_STATUS,
		.mask = 0x1,
		.shft = 6,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_H264_SEI_FP_AVAIL,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_SEI_AVAIL,
		.mask = 0x1,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_H264_SEI_FP_ARRGMENT_ID,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_FRAME_PACK_ARRGMENT_ID,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_H264_SEI_FP_INFO,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_FRAME_PACK_SEI_INFO,
		.mask = 0x3FFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_H264_SEI_FP_GRID_POS,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_FRAME_PACK_GRID_POS,
		.mask = 0xFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_H264_MVC_VIEW_ID,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_MVC_VIEW_ID,
		.mask = 0xFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_SEI_MAX_PIC_AVERAGE_LIGHT,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_CONTENT_LIGHT_LEVEL_INFO_SEI,
		.mask = 0xFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_SEI_MAX_CONTENT_LIGHT,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_CONTENT_LIGHT_LEVEL_INFO_SEI,
		.mask = 0xFFFF,
		.shft = 16,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_SEI_MAX_DISPLAY_LUMINANCE,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_0,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_SEI_MIN_DISPLAY_LUMINANCE,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_1,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_MATRIX_COEFFICIENTS,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_VIDEO_SIGNAL_TYPE,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_FORMAT,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_VIDEO_SIGNAL_TYPE,
		.mask = 0x7,
		.shft = 26,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_FULL_RANGE_FLAG,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_VIDEO_SIGNAL_TYPE,
		.mask = 0x1,
		.shft = 25,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_COLOUR_PRIMARIES,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_VIDEO_SIGNAL_TYPE,
		.mask = 0xFF,
		.shft = 16,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_TRANSFER_CHARACTERISTICS,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_VIDEO_SIGNAL_TYPE,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_SEI_WHITE_POINT,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_2,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_SEI_DISPLAY_PRIMARIES_0,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_3,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_SEI_DISPLAY_PRIMARIES_1,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_4,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_VIDEO_SEI_DISPLAY_PRIMARIES_2,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_5,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
};

static int mfc_dec_cleanup_ctx_ctrls(struct mfc_ctx *ctx)
{
	struct mfc_ctx_ctrl *ctx_ctrl;

	while (!list_empty(&ctx->ctrls)) {
		ctx_ctrl = list_entry((&ctx->ctrls)->next,
				      struct mfc_ctx_ctrl, list);
		list_del(&ctx_ctrl->list);
		kfree(ctx_ctrl);
	}

	INIT_LIST_HEAD(&ctx->ctrls);

	return 0;
}

static int mfc_dec_init_ctx_ctrls(struct mfc_ctx *ctx)
{
	unsigned long i;
	struct mfc_ctx_ctrl *ctx_ctrl;

	INIT_LIST_HEAD(&ctx->ctrls);

	for (i = 0; i < NUM_CTRL_CFGS; i++) {
		ctx_ctrl = kzalloc(sizeof(struct mfc_ctx_ctrl), GFP_KERNEL);
		if (ctx_ctrl == NULL) {
			mfc_err_dev("Failed to allocate context control "
					"id: 0x%08x, type: %d\n",
					mfc_ctrl_list[i].id,
					mfc_ctrl_list[i].type);

			mfc_dec_cleanup_ctx_ctrls(ctx);

			return -ENOMEM;
		}

		ctx_ctrl->type = mfc_ctrl_list[i].type;
		ctx_ctrl->id = mfc_ctrl_list[i].id;
		ctx_ctrl->addr = mfc_ctrl_list[i].addr;
		ctx_ctrl->has_new = 0;
		ctx_ctrl->val = 0;

		list_add_tail(&ctx_ctrl->list, &ctx->ctrls);
	}

	return 0;
}

static void __mfc_dec_cleanup_buf_ctrls(struct list_head *head)
{
	struct mfc_buf_ctrl *buf_ctrl;

	while (!list_empty(head)) {
		buf_ctrl = list_entry(head->next,
				struct mfc_buf_ctrl, list);
		list_del(&buf_ctrl->list);
		kfree(buf_ctrl);
	}

	INIT_LIST_HEAD(head);
}

static void mfc_dec_reset_buf_ctrls(struct list_head *head)
{
	struct mfc_buf_ctrl *buf_ctrl;

	list_for_each_entry(buf_ctrl, head, list) {
		buf_ctrl->has_new = 0;
		buf_ctrl->val = 0;
		buf_ctrl->old_val = 0;
		buf_ctrl->updated = 0;
	}
}

static int mfc_dec_init_buf_ctrls(struct mfc_ctx *ctx,
	enum mfc_ctrl_type type, unsigned int index)
{
	unsigned long i;
	struct mfc_ctx_ctrl *ctx_ctrl;
	struct mfc_buf_ctrl *buf_ctrl;
	struct list_head *head;

	if (index >= MFC_MAX_BUFFERS) {
		mfc_err_dev("Per-buffer control index is out of range\n");
		return -EINVAL;
	}

	if (type & MFC_CTRL_TYPE_SRC) {
		if (test_bit(index, &ctx->src_ctrls_avail)) {
			mfc_dec_reset_buf_ctrls(&ctx->src_ctrls[index]);

			return 0;
		}

		head = &ctx->src_ctrls[index];
	} else if (type & MFC_CTRL_TYPE_DST) {
		if (test_bit(index, &ctx->dst_ctrls_avail)) {
			mfc_dec_reset_buf_ctrls(&ctx->dst_ctrls[index]);

			return 0;
		}

		head = &ctx->dst_ctrls[index];
	} else {
		mfc_err_dev("Control type mismatch. type : %d\n", type);
		return -EINVAL;
	}

	INIT_LIST_HEAD(head);

	list_for_each_entry(ctx_ctrl, &ctx->ctrls, list) {
		if (!(type & ctx_ctrl->type))
			continue;

		/* find matched control configuration index */
		for (i = 0; i < NUM_CTRL_CFGS; i++) {
			if (ctx_ctrl->id == mfc_ctrl_list[i].id)
				break;
		}

		if (i == NUM_CTRL_CFGS) {
			mfc_err_dev("Failed to find buffer control "
					"id: 0x%08x, type: %d\n",
					ctx_ctrl->id, ctx_ctrl->type);
			continue;
		}

		buf_ctrl = kzalloc(sizeof(struct mfc_buf_ctrl), GFP_KERNEL);
		if (buf_ctrl == NULL) {
			mfc_err_dev("Failed to allocate buffer control "
					"id: 0x%08x, type: %d\n",
					mfc_ctrl_list[i].id,
					mfc_ctrl_list[i].type);

			__mfc_dec_cleanup_buf_ctrls(head);

			return -ENOMEM;
		}

		buf_ctrl->id = ctx_ctrl->id;
		buf_ctrl->type = ctx_ctrl->type;
		buf_ctrl->addr = ctx_ctrl->addr;

		buf_ctrl->is_volatile = mfc_ctrl_list[i].is_volatile;
		buf_ctrl->mode = mfc_ctrl_list[i].mode;
		buf_ctrl->mask = mfc_ctrl_list[i].mask;
		buf_ctrl->shft = mfc_ctrl_list[i].shft;
		buf_ctrl->flag_mode = mfc_ctrl_list[i].flag_mode;
		buf_ctrl->flag_addr = mfc_ctrl_list[i].flag_addr;
		buf_ctrl->flag_shft = mfc_ctrl_list[i].flag_shft;

		list_add_tail(&buf_ctrl->list, head);
	}

	mfc_dec_reset_buf_ctrls(head);

	if (type & MFC_CTRL_TYPE_SRC)
		set_bit(index, &ctx->src_ctrls_avail);
	else
		set_bit(index, &ctx->dst_ctrls_avail);

	return 0;
}

static int mfc_dec_cleanup_buf_ctrls(struct mfc_ctx *ctx,
	enum mfc_ctrl_type type, unsigned int index)
{
	struct list_head *head;

	if (index >= MFC_MAX_BUFFERS) {
		mfc_err_dev("Per-buffer control index is out of range\n");
		return -EINVAL;
	}

	if (type & MFC_CTRL_TYPE_SRC) {
		if (!(test_and_clear_bit(index, &ctx->src_ctrls_avail))) {
			return 0;
		}

		head = &ctx->src_ctrls[index];
	} else if (type & MFC_CTRL_TYPE_DST) {
		if (!(test_and_clear_bit(index, &ctx->dst_ctrls_avail))) {
			return 0;
		}

		head = &ctx->dst_ctrls[index];
	} else {
		mfc_err_dev("Control type mismatch. type : %d\n", type);
		return -EINVAL;
	}

	__mfc_dec_cleanup_buf_ctrls(head);

	return 0;
}

static int mfc_dec_to_buf_ctrls(struct mfc_ctx *ctx, struct list_head *head)
{
	struct mfc_ctx_ctrl *ctx_ctrl;
	struct mfc_buf_ctrl *buf_ctrl;

	list_for_each_entry(ctx_ctrl, &ctx->ctrls, list) {
		if (!(ctx_ctrl->type & MFC_CTRL_TYPE_SET) || !ctx_ctrl->has_new)
			continue;

		list_for_each_entry(buf_ctrl, head, list) {
			if (!(buf_ctrl->type & MFC_CTRL_TYPE_SET))
				continue;

			if (buf_ctrl->id == ctx_ctrl->id) {
				buf_ctrl->has_new = 1;
				buf_ctrl->val = ctx_ctrl->val;
				if (buf_ctrl->is_volatile)
					buf_ctrl->updated = 0;

				ctx_ctrl->has_new = 0;
				break;
			}
		}
	}

	return 0;
}

static int mfc_dec_to_ctx_ctrls(struct mfc_ctx *ctx, struct list_head *head)
{
	struct mfc_ctx_ctrl *ctx_ctrl;
	struct mfc_buf_ctrl *buf_ctrl;

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_GET) || !buf_ctrl->has_new)
			continue;

		list_for_each_entry(ctx_ctrl, &ctx->ctrls, list) {
			if (!(ctx_ctrl->type & MFC_CTRL_TYPE_GET))
				continue;

			if (ctx_ctrl->id == buf_ctrl->id) {
				ctx_ctrl->has_new = 1;
				ctx_ctrl->val = buf_ctrl->val;

				buf_ctrl->has_new = 0;
			}
		}
	}

	return 0;
}

static int mfc_dec_set_buf_ctrls_val(struct mfc_ctx *ctx, struct list_head *head)
{
	struct mfc_buf_ctrl *buf_ctrl;
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_dev *dev = ctx->dev;
	unsigned int value = 0;

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_SET) || !buf_ctrl->has_new)
			continue;

		/* read old vlaue */
		value = MFC_READL(buf_ctrl->addr);

		/* save old vlaue for recovery */
		if (buf_ctrl->is_volatile)
			buf_ctrl->old_val = (value >> buf_ctrl->shft) & buf_ctrl->mask;

		/* write new value */
		value &= ~(buf_ctrl->mask << buf_ctrl->shft);
		value |= ((buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft);
		MFC_WRITEL(value, buf_ctrl->addr);

		/* set change flag bit */
		if (buf_ctrl->flag_mode == MFC_CTRL_MODE_SFR) {
			value = MFC_READL(buf_ctrl->flag_addr);
			value |= (1 << buf_ctrl->flag_shft);
			MFC_WRITEL(value, buf_ctrl->flag_addr);
		}

		buf_ctrl->has_new = 0;
		buf_ctrl->updated = 1;

		if (buf_ctrl->id == V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG)
			dec->stored_tag = buf_ctrl->val;

		mfc_debug(6, "[CTRLS] Set buffer control id: 0x%08x, val: %d\n",
				buf_ctrl->id, buf_ctrl->val);
	}

	return 0;
}

static int mfc_dec_get_buf_ctrls_val(struct mfc_ctx *ctx, struct list_head *head)
{
	struct mfc_buf_ctrl *buf_ctrl;
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	unsigned int value = 0;

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_GET))
			continue;

		value = MFC_READL(buf_ctrl->addr);
		value = (value >> buf_ctrl->shft) & buf_ctrl->mask;

		buf_ctrl->val = value;
		buf_ctrl->has_new = 1;

		if (IS_VP9_DEC(ctx)) {
			if (buf_ctrl->id == V4L2_CID_MPEG_VIDEO_FULL_RANGE_FLAG)
				buf_ctrl->val = dec->color_range;
			else if (buf_ctrl->id == V4L2_CID_MPEG_VIDEO_COLOUR_PRIMARIES)
				buf_ctrl->val = dec->color_space;
		}

		mfc_debug(6, "[CTRLS] Get buffer control id: 0x%08x, val: %d\n",
				buf_ctrl->id, buf_ctrl->val);
	}

	return 0;
}

static int mfc_dec_set_buf_ctrls_val_nal_q(struct mfc_ctx *ctx,
			struct list_head *head, DecoderInputStr *pInStr)
{
	struct mfc_buf_ctrl *buf_ctrl;
	struct mfc_dec *dec = ctx->dec_priv;

	mfc_debug_enter();

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_SET) || !buf_ctrl->has_new)
			continue;
		switch (buf_ctrl->id) {
		case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG:
			pInStr->PictureTag &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->PictureTag |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			dec->stored_tag = buf_ctrl->val;
			break;
		/* If new dynamic controls are added, insert here */
		default:
			mfc_info_ctx("[NALQ] can't find control, id: 0x%x\n",
					buf_ctrl->id);
		}
		buf_ctrl->has_new = 0;
		buf_ctrl->updated = 1;

		mfc_debug(6, "[NALQ][CTRLS] Set buffer control id: 0x%08x, val: %d\n",
				buf_ctrl->id, buf_ctrl->val);
	}

	mfc_debug_leave();

	return 0;
}

static int mfc_dec_get_buf_ctrls_val_nal_q(struct mfc_ctx *ctx,
			struct list_head *head, DecoderOutputStr *pOutStr)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf_ctrl *buf_ctrl;
	unsigned int value = 0;

	mfc_debug_enter();

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_GET))
			continue;
		switch (buf_ctrl->id) {
		case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG:
			value = pOutStr->PictureTagTop;
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_DISPLAY_STATUS:
			value = pOutStr->DisplayStatus;
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_LUMA:
			value = pOutStr->DisplayFirstCrc;
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_CHROMA:
			value = pOutStr->DisplaySecondCrc;
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_CHROMA1:
			value = pOutStr->DisplayThirdCrc;
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_2BIT_LUMA:
			value = pOutStr->DisplayFirst2BitCrc;
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_2BIT_CHROMA:
			value = pOutStr->DisplaySecond2BitCrc;
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_CRC_GENERATED:
			value = pOutStr->DisplayStatus;
			break;
		case V4L2_CID_MPEG_VIDEO_H264_SEI_FP_AVAIL:
			value = pOutStr->SeiAvail;
			break;
		case V4L2_CID_MPEG_VIDEO_H264_SEI_FP_ARRGMENT_ID:
			value = pOutStr->FramePackArrgmentId;
			break;
		case V4L2_CID_MPEG_VIDEO_H264_SEI_FP_INFO:
			value = pOutStr->FramePackSeiInfo;
			break;
		case V4L2_CID_MPEG_VIDEO_H264_SEI_FP_GRID_POS:
			value = pOutStr->FramePackGridPos;
			break;
		case V4L2_CID_MPEG_VIDEO_H264_MVC_VIEW_ID:
			value = 0;
			break;
		/* TO-DO: SEI information has to be added in NAL-Q */
		case V4L2_CID_MPEG_VIDEO_SEI_MAX_PIC_AVERAGE_LIGHT:
		case V4L2_CID_MPEG_VIDEO_SEI_MAX_CONTENT_LIGHT:
			value = pOutStr->ContentLightLevelInfoSei;
			break;
		case V4L2_CID_MPEG_VIDEO_SEI_MAX_DISPLAY_LUMINANCE:
			value = pOutStr->MasteringDisplayColourVolumeSei0;
			break;
		case V4L2_CID_MPEG_VIDEO_SEI_MIN_DISPLAY_LUMINANCE:
			value = pOutStr->MasteringDisplayColourVolumeSei1;
			break;
		case V4L2_CID_MPEG_VIDEO_FULL_RANGE_FLAG:
		case V4L2_CID_MPEG_VIDEO_COLOUR_PRIMARIES:
		case V4L2_CID_MPEG_VIDEO_FORMAT:
		case V4L2_CID_MPEG_VIDEO_TRANSFER_CHARACTERISTICS:
		case V4L2_CID_MPEG_VIDEO_MATRIX_COEFFICIENTS:
			value = pOutStr->VideoSignalType;
			break;
		case V4L2_CID_MPEG_VIDEO_SEI_WHITE_POINT:
			value = pOutStr->MasteringDisplayColourVolumeSei2;
			break;
		case V4L2_CID_MPEG_VIDEO_SEI_DISPLAY_PRIMARIES_0:
			value = pOutStr->MasteringDisplayColourVolumeSei3;
			break;
		case V4L2_CID_MPEG_VIDEO_SEI_DISPLAY_PRIMARIES_1:
			value = pOutStr->MasteringDisplayColourVolumeSei4;
			break;
		case V4L2_CID_MPEG_VIDEO_SEI_DISPLAY_PRIMARIES_2:
			value = pOutStr->MasteringDisplayColourVolumeSei5;
			break;
			/* If new dynamic controls are added, insert here */
		default:
			mfc_info_ctx("[NALQ] can't find control, id: 0x%x\n",
					buf_ctrl->id);
		}
		value = (value >> buf_ctrl->shft) & buf_ctrl->mask;

		buf_ctrl->val = value;
		buf_ctrl->has_new = 1;

		if (IS_VP9_DEC(ctx)) {
			if (buf_ctrl->id == V4L2_CID_MPEG_VIDEO_FULL_RANGE_FLAG)
				buf_ctrl->val = dec->color_range;
			else if (buf_ctrl->id == V4L2_CID_MPEG_VIDEO_COLOUR_PRIMARIES)
				buf_ctrl->val = dec->color_space;
		}

		mfc_debug(6, "[NALQ][CTRLS] Get buffer control id: 0x%08x, val: %d\n",
				buf_ctrl->id, buf_ctrl->val);
	}

	mfc_debug_leave();

	return 0;
}

static int mfc_dec_recover_buf_ctrls_val(struct mfc_ctx *ctx, struct list_head *head)
{
	struct mfc_buf_ctrl *buf_ctrl;
	struct mfc_dev *dev = ctx->dev;
	unsigned int value = 0;

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_SET)
			|| !buf_ctrl->is_volatile
			|| !buf_ctrl->updated)
			continue;

		value = MFC_READL(buf_ctrl->addr);
		value &= ~(buf_ctrl->mask << buf_ctrl->shft);
		value |= ((buf_ctrl->old_val & buf_ctrl->mask) << buf_ctrl->shft);
		MFC_WRITEL(value, buf_ctrl->addr);

		/* clear change flag bit */
		if (buf_ctrl->flag_mode == MFC_CTRL_MODE_SFR) {
			value = MFC_READL(buf_ctrl->flag_addr);
			value &= ~(1 << buf_ctrl->flag_shft);
			MFC_WRITEL(value, buf_ctrl->flag_addr);
		}

		buf_ctrl->updated = 0;
		mfc_debug(6, "[CTRLS] Recover buffer control id: 0x%08x, old val: %d\n",
				buf_ctrl->id, buf_ctrl->old_val);
	}

	return 0;
}

static int mfc_dec_get_buf_update_val(struct mfc_ctx *ctx,
			struct list_head *head, unsigned int id, int value)
{
	struct mfc_buf_ctrl *buf_ctrl;

	list_for_each_entry(buf_ctrl, head, list) {
		if (buf_ctrl->id == id) {
			buf_ctrl->val = value;
			mfc_debug(6, "[CTRLS] Update buffer control id: 0x%08x, val: %d\n",
					buf_ctrl->id, buf_ctrl->val);
			break;
		}
	}

	return 0;
}

static int mfc_dec_recover_buf_ctrls_nal_q(struct mfc_ctx *ctx,
		struct list_head *head)
{
	struct mfc_buf_ctrl *buf_ctrl;

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_SET)
				|| !(buf_ctrl->updated))
			continue;

		buf_ctrl->has_new = 1;
		buf_ctrl->updated = 0;
		mfc_debug(6, "[NALQ][CTRLS] Recover buffer control id: 0x%08x, val: %d\n",
				buf_ctrl->id, buf_ctrl->val);
	}

	return 0;
}

struct mfc_ctrls_ops decoder_ctrls_ops = {
	.init_ctx_ctrls			= mfc_dec_init_ctx_ctrls,
	.cleanup_ctx_ctrls		= mfc_dec_cleanup_ctx_ctrls,
	.init_buf_ctrls			= mfc_dec_init_buf_ctrls,
	.reset_buf_ctrls		= mfc_dec_reset_buf_ctrls,
	.cleanup_buf_ctrls		= mfc_dec_cleanup_buf_ctrls,
	.to_buf_ctrls			= mfc_dec_to_buf_ctrls,
	.to_ctx_ctrls			= mfc_dec_to_ctx_ctrls,
	.set_buf_ctrls_val		= mfc_dec_set_buf_ctrls_val,
	.get_buf_ctrls_val		= mfc_dec_get_buf_ctrls_val,
	.set_buf_ctrls_val_nal_q_dec	= mfc_dec_set_buf_ctrls_val_nal_q,
	.get_buf_ctrls_val_nal_q_dec	= mfc_dec_get_buf_ctrls_val_nal_q,
	.recover_buf_ctrls_val		= mfc_dec_recover_buf_ctrls_val,
	.get_buf_update_val		= mfc_dec_get_buf_update_val,
	.recover_buf_ctrls_nal_q	= mfc_dec_recover_buf_ctrls_nal_q,
};
