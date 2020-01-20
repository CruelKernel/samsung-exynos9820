/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _DSMS_ACCESS_CONTROL_H
#define _DSMS_ACCESS_CONTROL_H

#include <linux/types.h>

#define CALLER_FRAME	(0)

struct dsms_policy_entry {
	const char *file_path;
	const char *function_name;
};

extern struct dsms_policy_entry dsms_policy[];

extern size_t dsms_policy_size(void);

extern int dsms_verify_access(const void *address);

#endif /* _DSMS_ACCESS_CONTROL_H */
