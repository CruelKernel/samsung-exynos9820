/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _DSMS_DEBUG_H
#define _DSMS_DEBUG_H

#define DSMS_TAG "[DSMS-KERNEL] "

enum loglevel {
	LOG_INFO,
	LOG_ERROR,
	LOG_DEBUG,
};

extern void dsms_log_write(int loglevel, const char* format, ...);

#endif /* _DSMS_DEBUG_H */
