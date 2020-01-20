/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for mfc driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _MFC_HWFC_H
#define _MFC_HWFC_H

#include <linux/types.h>

#define HWFC_ERR_NONE			0
#define HWFC_ERR_TSMUX			1
#define HWFC_ERR_MFC			2
#define HWFC_ERR_MFC_NOT_PREPARED	3
#define HWFC_ERR_MFC_TIMEOUT		4
#define HWFC_ERR_MFC_NOT_ENABLED	5

/*
 * struct encoding_param
 * @time_stamp	: timestamp value
 */
struct encoding_param {
	u64 time_stamp;
};

/*
 * mfc_hwfc_encode - Request encoding
 * @encoding_param : parameters for encoding
 *
 * repeater calls it to start encoding
 *
 */
#ifdef CONFIG_VIDEO_EXYNOS_MFC
int mfc_hwfc_encode(int buf_index, int job_id, struct encoding_param *param);
#else
static inline int mfc_hwfc_encode(int buf_index, int job_id, struct encoding_param *param)
{
	return -HWFC_ERR_MFC_NOT_ENABLED;
}
#endif

#endif /* _MFC_HWFC_H */
