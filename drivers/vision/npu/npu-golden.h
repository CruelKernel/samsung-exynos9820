/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _NPU_GOLDEN_H_
#define _NPU_GOLDEN_H_

#include <linux/device.h>
#include "npu-util-statekeeper.h"
#include "npu-common.h"

/* Comparing context for golden matching */
#define NPU_GOLDEN_RESULT_BUF_LEN	8192

/*
 * Structure to keep the compare result
 * of a image/buffer
 */
struct npu_golden_compare_result {
	size_t				out_buf_cur;
	size_t				out_buf_len;
	size_t				copied_len;

	char				out_buf[NPU_GOLDEN_RESULT_BUF_LEN];
};

/*
 * Convinient macro to add message on npu_golden_compare_result
 */
#define add_compare_msg(result, ...)				\
({									\
	struct npu_golden_compare_result *__result = (result);		\
	int __ret;							\
	size_t remain = __result->out_buf_len - __result->out_buf_cur;	\
	__ret = scnprintf(__result->out_buf + __result->out_buf_cur	\
	, remain, __VA_ARGS__);			\
	__result->out_buf_cur += __ret;					\
	__ret;								\
})

struct npu_golden_ctx;
int register_golden_matcher(struct device *dev);
int npu_golden_compare(const struct npu_frame *frame);
#endif
