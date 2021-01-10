/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include "dsms_init.h"
#include "dsms_kernel_api.h"
#include "dsms_netlink.h"
#include "dsms_message_list.h"
#include "dsms_rate_limit.h"
#include "dsms_test.h"

static int is_dsms_initialized_flag = false;

int dsms_is_initialized(void)
{
	return is_dsms_initialized_flag;
}

kunit_init_module(dsms_init)
{
	int ret = 0;
	DSMS_LOG_INFO("Started.");

	if (is_dsms_initialized_flag != true) {
		ret = prepare_userspace_communication();
		if (ret != 0) {
			DSMS_LOG_ERROR("It was not possible to prepare the userspace communication: %d.", ret);
			return ret;
		}
		init_semaphore_list();
		dsms_rate_limit_init();
		is_dsms_initialized_flag = true;
	}
	return ret;
}

static void __exit dsms_exit(void)
{
	int ret = 0;

	DSMS_LOG_INFO("Exited.");
	ret = remove_userspace_communication();
	if (ret != 0)
		DSMS_LOG_ERROR("It was not possible to remove the userspace communication: %d.", ret);
}

module_init(dsms_init);
module_exit(dsms_exit);
