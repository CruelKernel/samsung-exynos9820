/*
 * drivers/media/platform/exynos/mfc/mfc_otf.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_OTF_H
#define __MFC_OTF_H __FILE__

#ifdef CONFIG_VIDEO_EXYNOS_TSMUX
#include <media/exynos_tsmux.h>
#endif

#include "mfc_common.h"

extern struct mfc_dev *g_mfc_dev;

int mfc_otf_create(struct mfc_ctx *ctx);
void mfc_otf_destroy(struct mfc_ctx *ctx);
int mfc_otf_init(struct mfc_ctx *ctx);
void mfc_otf_deinit(struct mfc_ctx *ctx);
int mfc_otf_ctx_ready(struct mfc_ctx *ctx);
int mfc_otf_run_enc_init(struct mfc_ctx *ctx);
int mfc_otf_run_enc_frame(struct mfc_ctx *ctx);
int mfc_otf_handle_seq(struct mfc_ctx *ctx);
int mfc_otf_handle_stream(struct mfc_ctx *ctx);
void mfc_otf_handle_error(struct mfc_ctx *ctx, unsigned int reason, unsigned int err);

#endif /* __MFC_OTF_H  */
