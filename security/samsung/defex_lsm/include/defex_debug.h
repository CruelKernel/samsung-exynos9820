/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
*/

#ifndef __DEFEX_DEBUG_H
#define __DEFEX_DEBUG_H

#define DBG_SETUID		0
#define DBG_SET_FSUID		1
#define DBG_SETGID		2

#define DBG_SET_PE_STATUS	3
#define DBG_SET_IM_STATUS	4
#define DBG_SET_SP_STATUS	5
#define DBG_SET_INT_STATUS	6

#define MAX_DATA_LEN		300

void blob(const char *buffer, const size_t bufLen, const int lineSize);
int defex_create_debug(struct kset *defex_kset);

#endif /* __DEFEX_DEBUG_H */
