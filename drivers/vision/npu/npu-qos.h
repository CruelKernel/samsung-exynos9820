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

#ifndef _NPU_QOS_H_
#define _NPU_QOS_H_
#include <linux/pm_qos.h>
#include <linux/mutex.h>
#include <linux/list.h>

struct npu_qos_setting {
	struct mutex		npu_qos_lock;
	struct platform_device	*dvfs_npu_dev;

	struct pm_qos_request	npu_qos_req_npu;
	struct pm_qos_request	npu_qos_req_mif;
	struct pm_qos_request	npu_qos_req_int;
	struct pm_qos_request	npu_qos_req_cpu_cl0;
	struct pm_qos_request	npu_qos_req_cpu_cl1;
	struct pm_qos_request	npu_qos_req_cpu_cl2;

	s32		default_freq;
	s32		request_freq;
	s32		current_freq;

	s32		req_cl0_freq;
	s32		req_cl1_freq;
	s32		req_cl2_freq;

	s32		req_npu_freq;
	s32		req_mif_freq;
	s32		req_int_freq;
};

struct npu_session_qos_req {
	s32		sessionUID;
	s32		req_freq;
	__u32		eCategory;
	struct list_head list;
};

int npu_qos_probe(struct npu_system *system);
int npu_qos_release(struct npu_system *system);
int npu_qos_start(struct npu_system *system);
int npu_qos_stop(struct npu_system *system);
npu_s_param_ret npu_qos_param_handler(struct npu_session *sess,
	struct vs4l_param *param, int *retval);

#endif	/* _NPU_QOS_H_ */
