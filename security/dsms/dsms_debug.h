/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _DSMS_DEBUG_H
#define _DSMS_DEBUG_H

#define DSMS_TAG		"[DSMS-KERNEL] "
#define DSMS_DEBUG_TAG	"[DSMS-KERNEL] DEBUG: "

#ifdef DSMS_DEBUG_ENABLE

extern void dsms_debug_message(const char *feature_code,
		const char *detail,
		int64_t value);

#endif //DSMS_DEBUG_ENABLE

#endif /* _DSMS_DEBUG_H */
