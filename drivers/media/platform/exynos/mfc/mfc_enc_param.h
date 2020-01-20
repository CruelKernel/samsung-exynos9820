/*
 * drivers/media/platform/exynos/mfc/mfc_enc_param.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_ENC_PARAM_H
#define __MFC_ENC_PARAM_H __FILE__

#include "mfc_common.h"

void mfc_set_slice_mode(struct mfc_ctx *ctx);
void mfc_set_aso_slice_order_h264(struct mfc_ctx *ctx);
int mfc_set_enc_params(struct mfc_ctx *ctx);
void mfc_set_test_params(struct mfc_dev *dev);

#endif /* __MFC_ENC_PARAM_H */
