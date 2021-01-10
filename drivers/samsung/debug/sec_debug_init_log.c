/*
 * sec_debug_init_log.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/sec_debug.h>

static char *buf_ptr;
static unsigned long buf_size;
static unsigned long buf_idx;

size_t sec_debug_get_curr_init_ptr(void)
{
#ifdef CONFIG_SEC_DEBUG_SYSRQ_KMSG
	if (!buf_ptr || !buf_size)
		return 0;
	else
		return (size_t)(buf_ptr + buf_idx);
#endif
	return 0;
}

static void sec_debug_hook_init_log(const char *str, size_t size)
{
	int len;

	if (buf_idx + size > buf_size) {
		len = buf_size - buf_idx;
		memcpy(buf_ptr + buf_idx, str, len);
		memcpy(buf_ptr, str + len, size - len);
		buf_idx = size - len;
	} else {
		memcpy(buf_ptr + buf_idx, str, size);
		buf_idx = (buf_idx + size) % buf_size;
	}
}

static int __init sec_debug_init_init_log(void)
{
	pr_err("%s: start\n", __func__);

	buf_ptr = (char *)phys_to_virt((sec_debug_get_buf_base(SDN_MAP_INITTASK_LOG)));
	buf_size = sec_debug_get_buf_size(SDN_MAP_INITTASK_LOG);
	pr_err("%s: buffer size 0x%llx at addr 0x%llx\n", __func__, buf_size ,buf_ptr);

	if (!buf_ptr || !buf_size)
		return 0;

	memset(buf_ptr, 0, buf_size);
	register_init_log_hook_func(sec_debug_hook_init_log);
	pr_err("%s: done\n", __func__);

	return 0;
}
late_initcall(sec_debug_init_init_log);

