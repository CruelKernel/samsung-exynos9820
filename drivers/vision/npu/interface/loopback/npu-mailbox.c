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

#include <linux/delay.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#include "npu-mailbox.h"



int npu_mailbox_init(struct mailbox_ctrl *mBox);
int npu_mailbox_ready(struct mailbox_ctrl *mBox);
int npu_mailbox_read(struct mailbox_ctrl *mBox);
int npu_mailbox_write(struct mailbox_ctrl *mBox);


int npu_mailbox_read(struct mailbox_ctrl *mBox)
{
	int ret = 0;
	return ret;
}
int npu_mailbox_ready(struct mailbox_ctrl *mBox)
{
	int ret = 0;
	return ret;
}
int npu_mailbox_write(struct mailbox_ctrl *mBox)
{
	int ret = 0;
	return ret;
}


