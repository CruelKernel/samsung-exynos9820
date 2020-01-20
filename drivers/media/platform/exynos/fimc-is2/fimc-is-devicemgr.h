/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef FIMC_IS_DEVICE_MGR_H
#define FIMC_IS_DEVICE_MGR_H

#include "fimc-is-config.h"
#include "fimc-is-device-ischain.h"
#include "fimc-is-device-sensor.h"

#ifdef CONFIG_USE_SENSOR_GROUP
#define GET_DEVICE_TYPE_BY_GRP(group_id)	\
	({enum fimc_is_device_type type;	\
	 switch (group_id) {		\
	 case GROUP_ID_SS0:		\
	 case GROUP_ID_SS1:		\
	 case GROUP_ID_SS2:		\
	 case GROUP_ID_SS3:		\
	 case GROUP_ID_SS4:		\
	 case GROUP_ID_SS5:		\
		type = FIMC_IS_DEVICE_SENSOR;	\
		break;			\
	 default:			\
		type = FIMC_IS_DEVICE_ISCHAIN;	\
		break;			\
	 }; type;})

#define GET_HEAD_GROUP_IN_DEVICE(type, group)	\
	({ struct fimc_is_group *head;			\
	 head = group->head;				\
	 while (head) {					\
		if (head->device_type == type)		\
			break;				\
		else					\
			head = head->child;		\
	 }; head;})

#define GET_OUT_FLAG_IN_DEVICE(device_type, out_flag)				\
	({unsigned long tmp_out_flag;						\
	if (device_type == FIMC_IS_DEVICE_ISCHAIN)				\
		tmp_out_flag = ((out_flag) & (~((1 << ENTRY_3AA) - 1)));	\
	else									\
		tmp_out_flag = ((out_flag) & ((1 << ENTRY_3AA) - 1));		\
	tmp_out_flag;})
#else
#define GET_DEVICE_TYPE_BY_GRP(group_id) FIMC_IS_DEVICE_ISCHAIN
#define GET_HEAD_GROUP_IN_DEVICE(type, group) (group->head)
#define GET_OUT_FLAG_IN_DEVICE(device_type, out_flag) (out_flag)
#endif

struct devicemgr_sensor_tag_data {
	struct fimc_is_devicemgr	*devicemgr;
	struct fimc_is_group		*group;
	u32				fcount;
	u32				stream;
};

struct fimc_is_devicemgr {
	struct fimc_is_device_sensor		*sensor[FIMC_IS_STREAM_COUNT];
	struct fimc_is_device_ischain		*ischain[FIMC_IS_STREAM_COUNT];
	struct tasklet_struct			tasklet_sensor_tag[FIMC_IS_STREAM_COUNT];
	struct devicemgr_sensor_tag_data	sensor_tag_data[FIMC_IS_STREAM_COUNT];
};

struct fimc_is_group *get_ischain_leader_group(struct fimc_is_device_ischain *device);
int fimc_is_devicemgr_probe(struct fimc_is_devicemgr *devicemgr);
int fimc_is_devicemgr_open(struct fimc_is_devicemgr *devicemgr,
		void *device, enum fimc_is_device_type type);
int fimc_is_devicemgr_binding(struct fimc_is_devicemgr *devicemgr,
		struct fimc_is_device_sensor *sensor,
		struct fimc_is_device_ischain *ischain,
		enum fimc_is_device_type type);
int fimc_is_devicemgr_start(struct fimc_is_devicemgr *devicemgr,
		void *device, enum fimc_is_device_type type);
int fimc_is_devicemgr_stop(struct fimc_is_devicemgr *devicemgr,
		void *device, enum fimc_is_device_type type);
int fimc_is_devicemgr_close(struct fimc_is_devicemgr *devicemgr,
		void *device, enum fimc_is_device_type type);
int fimc_is_devicemgr_shot_callback(struct fimc_is_group *group,
		struct fimc_is_frame *frame,
		u32 fcount,
		enum fimc_is_device_type type);
int fimc_is_devicemgr_shot_done(struct fimc_is_group *group,
		struct fimc_is_frame *ldr_frame,
		u32 status);
#endif
