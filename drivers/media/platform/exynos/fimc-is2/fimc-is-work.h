/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_WORK_H
#define FIMC_IS_WORK_H

/*#define TRACE_WORK*/

#define MAX_WORK_COUNT		10

enum work_map {
	WORK_SHOT_DONE,
	WORK_30C_FDONE,
	WORK_30P_FDONE,
	WORK_30F_FDONE,
	WORK_30G_FDONE,
	WORK_31C_FDONE,
	WORK_31P_FDONE,
	WORK_31F_FDONE,
	WORK_31G_FDONE,
	WORK_32P_FDONE,
	WORK_I0C_FDONE,
	WORK_I0P_FDONE,
	WORK_I1C_FDONE,
	WORK_I1P_FDONE,
	WORK_ME0C_FDONE,	/* ME */
	WORK_ME1C_FDONE,	/* ME */
	WORK_D0C_FDONE,		/* TPU0 */
	WORK_D1C_FDONE,		/* TPU1 */
	WORK_DC1S_FDONE, 	/* DCP Master Input */
	WORK_DC0C_FDONE, 	/* DCP Master Capture */
	WORK_DC1C_FDONE, 	/* DCP Slave Capture */
	WORK_DC2C_FDONE, 	/* DCP Disparity Capture */
	WORK_DC3C_FDONE, 	/* DCP Master Sub Capture */
	WORK_DC4C_FDONE, 	/* DCP Slave Sub Capture */
	WORK_M0P_FDONE,
	WORK_M1P_FDONE,
	WORK_M2P_FDONE,
	WORK_M3P_FDONE,
	WORK_M4P_FDONE,
	WORK_M5P_FDONE,
	WORK_MAX_MAP
};

struct fimc_is_msg {
	u32	id;
	u32	command;
	u32	instance;
	u32	group;
	u32	param1;
	u32	param2;
	u32	param3;
	u32	param4;
};

struct fimc_is_work {
	struct list_head	list;
	struct fimc_is_msg	msg;
	u32			fcount;
	struct fimc_is_frame	*frame;
};

struct fimc_is_work_list {
	u32			id;
	struct fimc_is_work	work[MAX_WORK_COUNT];
	spinlock_t		slock_free;
	spinlock_t		slock_request;
	struct list_head	work_free_head;
	u32			work_free_cnt;
	struct list_head	work_request_head;
	u32			work_request_cnt;
	wait_queue_head_t	wait_queue;
};

void init_work_list(struct fimc_is_work_list *this, u32 id, u32 count);

int set_free_work(struct fimc_is_work_list *this, struct fimc_is_work *work);
int get_free_work(struct fimc_is_work_list *this, struct fimc_is_work **work);
int set_req_work(struct fimc_is_work_list *this, struct fimc_is_work *work);
int get_req_work(struct fimc_is_work_list *this, struct fimc_is_work **work);

/*for debugging*/
int print_fre_work_list(struct fimc_is_work_list *this);
int print_req_work_list(struct fimc_is_work_list *this);

#endif /* FIMC_IS_WORK_H */
