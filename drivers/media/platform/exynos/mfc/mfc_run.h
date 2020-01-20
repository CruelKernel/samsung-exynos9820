/*
 * drivers/media/platform/exynos/mfc/mfc_run.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_RUN_H
#define __MFC_RUN_H __FILE__

#include "mfc_common.h"

int mfc_run_init_hw(struct mfc_dev *dev);
void mfc_run_deinit_hw(struct mfc_dev *dev);

int mfc_run_sleep(struct mfc_dev *dev);
int mfc_run_wakeup(struct mfc_dev *dev);

int mfc_run_dec_init(struct mfc_ctx *ctx);
int mfc_run_dec_frame(struct mfc_ctx *ctx);
int mfc_run_dec_last_frames(struct mfc_ctx *ctx);

int mfc_run_enc_init(struct mfc_ctx *ctx);
int mfc_run_enc_frame(struct mfc_ctx *ctx);
int mfc_run_enc_last_frames(struct mfc_ctx *ctx);

#endif /* __MFC_RUN_H */
