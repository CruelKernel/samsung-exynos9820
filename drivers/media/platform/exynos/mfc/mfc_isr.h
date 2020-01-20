/*
 * drivers/media/platform/exynos/mfc/mfc_isr.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_ISR_H
#define __MFC_ISR_H __FILE__

#include <linux/interrupt.h>

#include "mfc_common.h"

#include "mfc_utils.h"

irqreturn_t mfc_top_half_irq(int irq, void *priv);
irqreturn_t mfc_irq(int irq, void *priv);

static inline void mfc_handle_force_change_status(struct mfc_ctx *ctx)
{
	if (ctx->state != MFCINST_ABORT && ctx->state != MFCINST_HEAD_PARSED &&
			ctx->state != MFCINST_RES_CHANGE_FLUSH)
		mfc_change_state(ctx, MFCINST_RUNNING);
}

#endif /* __MFC_ISR_H */
