/*
* Samsung debugging features for Samsung's SoC's.
*
* Copyright (c) 2019 Samsung Electronics Co., Ltd.
*      http://www.samsung.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*/

#ifndef SEC_DEBUG_TYPES_H
#define SEC_DEBUG_TYPES_H

#ifdef CONFIG_SEC_DEBUG_DTASK
enum {
	DTYPE_NONE,
	DTYPE_MUTEX,
	DTYPE_RWSEM,
	DTYPE_WORK,
	DTYPE_CPUHP,
	DTYPE_KTHREAD,
};

struct sec_debug_wait {
	int	type;
	void	*data;
};
#endif

#endif /* SEC_DEBUG_TYPES_H */

