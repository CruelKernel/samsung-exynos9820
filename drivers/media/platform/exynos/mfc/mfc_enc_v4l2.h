/*
 * drivers/media/platform/exynos/mfc/mfc_enc_v4l2.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_ENC_V4L2_H
#define __MFC_ENC_V4L2_H __FILE__

#include "mfc_common.h"

const struct v4l2_ioctl_ops *mfc_get_enc_v4l2_ioctl_ops(void);

#endif /* __MFC_ENC_V4L2_H */
