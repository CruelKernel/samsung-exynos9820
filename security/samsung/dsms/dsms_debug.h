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
#define DSMS_DEBUG_TAG "[DSMS-KERNEL] DEBUG: "

#ifdef DSMS_DEBUG_WHITELIST
#  define DEBUG_WHITELIST (1)
#else
#  define DEBUG_WHITELIST (0)
#endif

static inline char debug_whitelist(void)
{
	return DEBUG_WHITELIST;
}

#ifdef DSMS_DEBUG_TRACE_DSMS_CALLS
#  define DEBUG_TRACE_DSMS_CALLS (1)
#else
#  define DEBUG_TRACE_DSMS_CALLS (0)
#endif

static inline char debug_trace_dsms_calls(void)
{
	return DEBUG_TRACE_DSMS_CALLS;
}

enum loglevel {
	LOG_INFO,
	LOG_ERROR,
};

enum logtype{
	GENERIC,
	WHITELIST,
	TRACE
};

extern void dsms_log_write(int loglevel, const char* format, ...);

#ifdef DSMS_DEBUG_ENABLE

extern void dsms_log_debug(int logtype, const char* format, ...);

#else

static inline void dsms_log_debug(int logtype, const char* format, ...)
{
}

#endif /* DSMS_DEBUG_ENABLE */

#endif /* _DSMS_DEBUG_H */
