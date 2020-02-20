/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _DSMS_RATE_LIMIT_H
#define _DSMS_RATE_LIMIT_H

extern void dsms_rate_limit_init(void);
extern int dsms_check_message_rate_limit(void);

#endif /* _DSMS_RATE_LIMIT_H */
