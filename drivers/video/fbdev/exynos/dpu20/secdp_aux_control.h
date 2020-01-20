/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _SECDP_AUX_CONTROL_H_
#define _SECDP_AUX_CONTROL_H_

int secdp_aux_dev_init(ssize_t (*secdp_i2c_write)(void *buffer, size_t size),
			ssize_t (*secdp_i2c_read)(void *buffer, size_t size),
			ssize_t (*secdp_dpcd_write)(unsigned int offset, void *buffer, size_t size),
			ssize_t (*secdp_dpcd_read)(unsigned int offset,	void *buffer, size_t size),
			int (*secdp_get_hpd_status)(void));

#endif /* _SECDP_AUX_CONTROL_H_ */
