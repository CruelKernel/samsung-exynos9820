/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _LINUX_DSMS_H
#define _LINUX_DSMS_H

#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/types.h>

#define DSMS_SUCCESS	(0)
#define DSMS_DENY		(-EPERM)

// DSMS Kernel Interface
extern int noinline dsms_send_message(const char *feature_code,
		const char *detail, int64_t value);

#endif /* _LINUX_DSMS_H */
