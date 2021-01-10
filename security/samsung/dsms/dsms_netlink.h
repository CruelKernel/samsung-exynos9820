/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _DSMS_NETLINK_H
#define _DSMS_NETLINK_H

#include <linux/kernel.h>
#include <linux/version.h>
#include <net/genetlink.h>
#include "dsms_netlink_protocol.h"

extern int prepare_userspace_communication(void);
extern int remove_userspace_communication(void);

#endif /* _DSMS_NETLINK_H */
