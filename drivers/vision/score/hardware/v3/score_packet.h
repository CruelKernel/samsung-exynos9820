/*
 * Samsung Exynos SoC series SCore driver
 *
 * Definition of SCore packet (v2.0)
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_PACKET_H__
#define __SCORE_PACKET_H__

#define MIN_PACKET_SIZE (sizeof(struct score_host_packet) + \
		sizeof(struct score_host_packet_info))

#define MAX_PACKET_SIZE		(2048)

enum score_host_packet_version {
	HOST_PKT_V1 = 0x1,
	HOST_PKT_V2 = 0x2,
};

enum score_host_task_option {
	HOST_TKT_NORMAL = 0x0,
	HOST_TKT_PRIORITY = 0x1,
	HOST_TKT_BOUND = 0x2,
};

enum host_info_type {
	MEMORY_TYPE = 1,
	TASK_ID_TYPE = 2
};

struct score_host_buffer {
	unsigned int			memory_type :3;
	unsigned int			offset      :29;
	unsigned int			memory_size;
	union {
		int			fd;
		unsigned long		userptr;
		unsigned long long	mem_info;
	} m;
	unsigned int			addr_offset :29;
	unsigned int			type        :3;
	unsigned int			reserved;
};

struct score_host_packet_info {
	unsigned int			valid_size;
	unsigned int			task_type;
	unsigned int			reserved;
	unsigned int			buf_count;
	unsigned int			payload[0];
};

struct score_host_packet {
	unsigned int			size;
	unsigned int			packet_offset;
	unsigned int			version  :4;
	unsigned int			reserved :28;
	unsigned int			payload[0];
};

#endif
