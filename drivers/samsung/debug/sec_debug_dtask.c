/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung debug platform watchdog
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/sec_debug.h>

void sec_debug_set_wait_data(int type, void *data)
{
	struct sec_debug_wait *cur_wait = &current->ssdbg_wait;
	
	cur_wait->type = type;
	cur_wait->data = data;
}

