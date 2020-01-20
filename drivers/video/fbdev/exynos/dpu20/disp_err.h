/*
 * linux/drivers/video/fbdev/exynos/dpu_9810/disp_err.h
 *
 * Source file for display error info
 *
 * Copyright (c) 2017 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DISP_ERR_H__
#define __DISP_ERR_H__

enum disp_error_type {
	/* DSIM */
	DSIM_UNKNOWN_ERROR,

	/* PANEL */
	PANEL_DISP_DET_ERROR,
	PANEL_UNKNOWN_ERROR,
	MAX_DISP_ERROR_TYPE,
};

typedef int (disp_check_cb)(void *data);
struct disp_check_cb_info {
	disp_check_cb *check_cb;
	void *data;
};

#define __DISP_CHECK_CB_INFO_INITIALIZER(cb_func, cb_data) \
		{ .check_cb = (disp_check_cb *)cb_func, .data = cb_data }
#define DEFINE_DISP_CHECK_CB_INFO(name, cb_func, cb_data) \
	struct disp_check_cb_info name = __DISP_CHECK_CB_INFO_INITIALIZER(cb_func, cb_data)

typedef int (disp_error_cb)(void *data,
		struct disp_check_cb_info *info);
struct disp_error_cb_info {
	disp_error_cb *error_cb;
	void *data;
};

#define __DISP_ERROR_CB_INFO_INITIALIZER(cb, cb_data) \
		{ .error_cb = (disp_error_cb *)cb, .data = cb_data }
#define DEFINE_DISP_ERROR_CB_INFO(name, cb_func, cb_data) \
	struct disp_error_cb_info name = __DISP_ERROR_CB_INFO_INITIALIZER(cb_func, cb_data)

#define DISP_ERROR_CB_RETRY_CNT	(1)
#define DISP_CHECK_STATUS_OK	(0)
#define DISP_CHECK_STATUS_NODEV	(1 << 0)	/* UB_CON_DET */
#define DISP_CHECK_STATUS_ELOFF	(1 << 1)	/* DISP_DET */
#define IS_DISP_CHECK_STATUS_DISCONNECTED(_status_) ((_status_) & (DISP_CHECK_STATUS_NODEV))
#define IS_DISP_CHECK_STATUS_NOK(_status_) ((_status_) != (DISP_CHECK_STATUS_OK))

static inline int disp_check_status(struct disp_check_cb_info *info)
{
	if (!info || !info->check_cb)
		return -EINVAL;

	return info->check_cb(info->data);
}

#endif /* __DISP_ERR_H__ */
