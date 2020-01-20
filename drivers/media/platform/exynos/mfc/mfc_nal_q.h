/*
 * drivers/media/platform/exynos/mfc/mfc_nal_q.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_NAL_Q_H
#define __MFC_NAL_Q_H __FILE__

#include "mfc_common.h"

int mfc_nal_q_check_enable(struct mfc_dev *dev);

void mfc_nal_q_clock_on(struct mfc_dev *dev, nal_queue_handle *nal_q_handle);
void mfc_nal_q_clock_off(struct mfc_dev *dev, nal_queue_handle *nal_q_handle);
void mfc_nal_q_cleanup_clock(struct mfc_dev *dev);

nal_queue_handle *mfc_nal_q_create(struct mfc_dev *dev);
void mfc_nal_q_destroy(struct mfc_dev *dev, nal_queue_handle *nal_q_handle);

void mfc_nal_q_init(struct mfc_dev *dev, nal_queue_handle *nal_q_handle);
void mfc_nal_q_start(struct mfc_dev *dev, nal_queue_handle *nal_q_handle);
void mfc_nal_q_stop(struct mfc_dev *dev, nal_queue_handle *nal_q_handle);
void mfc_nal_q_stop_if_started(struct mfc_dev *dev);
void mfc_nal_q_cleanup_queue(struct mfc_dev *dev);

int mfc_nal_q_handle_out_buf(struct mfc_dev *dev, EncoderOutputStr *pOutStr);
int mfc_nal_q_enqueue_in_buf(struct mfc_dev *dev, struct mfc_ctx *ctx,
			nal_queue_in_handle *nal_q_in_handle);
EncoderOutputStr *mfc_nal_q_dequeue_out_buf(struct mfc_dev *dev,
			nal_queue_out_handle *nal_q_out_handle, unsigned int *reason);

#endif /* __MFC_NAL_Q_H  */
