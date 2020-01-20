/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _NPU_MAILBOX_MGR_MOCK_H_
#define _NPU_MAILBOX_MGR_MOCK_H_

#include "npu-common.h"
#include "../../npu-if-protodrv-mbox.h"
#include "npu-config.h"
#include "../../npu-device.h"

int mailbox_mgr_mock_probe(struct npu_device* npu_device);
int mailbox_mgr_mock_release(void);
int mailbox_mgr_mock_open(struct npu_device* npu_device);
int mailbox_mgr_mock_close(struct npu_device* npu_device);
int mailbox_mgr_mock_start(struct npu_device* npu_device);
int mailbox_mgr_mock_stop(struct npu_device* npu_device);

#endif	/* _NPU_MAILBOX_MGR_MOCK_H_ */
