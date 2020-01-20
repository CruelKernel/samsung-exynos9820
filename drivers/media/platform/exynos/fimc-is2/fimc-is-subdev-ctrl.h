/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_SUBDEV_H
#define FIMC_IS_SUBDEV_H

#include "fimc-is-video.h"

#define SUBDEV_INTERNAL_BUF_MAX		(8)

struct fimc_is_device_sensor;
struct fimc_is_device_ischain;
struct fimc_is_groupmgr;
struct fimc_is_group;

enum fimc_is_subdev_device_type {
	FIMC_IS_SENSOR_SUBDEV,
	FIMC_IS_ISCHAIN_SUBDEV,
};

enum fimc_is_subdev_state {
	FIMC_IS_SUBDEV_OPEN,
	FIMC_IS_SUBDEV_START,
	FIMC_IS_SUBDEV_RUN,
	FIMC_IS_SUBDEV_FORCE_SET,
	FIMC_IS_SUBDEV_PARAM_ERR,
	FIMC_IS_SUBDEV_INTERNAL_USE,
	FIMC_IS_SUBDEV_INTERNAL_S_FMT
};

struct fimc_is_subdev_path {
	u32					width;
	u32					height;
	struct fimc_is_crop			canv;
	struct fimc_is_crop			crop;
};

/* Caution: Do not exceed 64 */
enum fimc_is_subdev_id {
	ENTRY_SENSOR,
	ENTRY_SSVC0,
	ENTRY_SSVC1,
	ENTRY_SSVC2,
	ENTRY_SSVC3,
	ENTRY_BNS,
	ENTRY_3AA,
	ENTRY_3AC,
	ENTRY_3AP,
	ENTRY_3AF,
	ENTRY_3AG,
	ENTRY_ISP,
	ENTRY_IXC,
	ENTRY_IXP,
	ENTRY_MEXC, /* Motion Estimation */
	ENTRY_DRC,
	ENTRY_DIS,
	ENTRY_DXC,
	ENTRY_ODC,
	ENTRY_DNR,
	ENTRY_DCP, /* Master Input */
	ENTRY_DC1S, /* Slave Input */
	ENTRY_DC0C, /* Master Main Capture */
	ENTRY_DC1C, /* Slave Main Capture */
	ENTRY_DC2C, /* Disparity + Confidence + Master */
	ENTRY_DC3C, /* Master Sub Capture */
	ENTRY_DC4C, /* Slave Sub Capture */
	ENTRY_MCS,
	ENTRY_M0P,
	ENTRY_M1P,
	ENTRY_M2P,
	ENTRY_M3P,
	ENTRY_M4P,
	ENTRY_M5P,
	ENTRY_VRA,
	ENTRY_PAF, /* PDP(PATSTAT) RDMA */
	ENTRY_END
};

struct fimc_is_subdev_ops {
	int (*bypass)(struct fimc_is_subdev *subdev,
		void *device_data,
		struct fimc_is_frame *frame,
		bool bypass);
	int (*cfg)(struct fimc_is_subdev *subdev,
		void *device_data,
		struct fimc_is_frame *frame,
		struct fimc_is_crop *incrop,
		struct fimc_is_crop *otcrop,
		u32 *lindex,
		u32 *hindex,
		u32 *indexes);
	int (*tag)(struct fimc_is_subdev *subdev,
		void *device_data,
		struct fimc_is_frame *frame,
		struct camera2_node *node);
};

enum subdev_ch_mode {
	SCM_WO_PAF_HW,
	SCM_W_PAF_HW,
	SCM_MAX,
};

struct fimc_is_subdev {
	u32					id;
	u32					vid; /* video id */
	u32					cid; /* capture node id */
	char					name[4];
	u32					instance;
	unsigned long				state;

	u32					constraints_width; /* spec in width */
	u32					constraints_height; /* spec in height */

	u32					param_otf_in;
	u32					param_dma_in;
	u32					param_otf_ot;
	u32					param_dma_ot;

	struct fimc_is_subdev_path		input;
	struct fimc_is_subdev_path		output;

	struct list_head			list;

	/* for internal use */
	struct fimc_is_framemgr			internal_framemgr;
	u32					buffer_num;
	u32					bytes_per_pixel;
	struct fimc_is_priv_buf			*pb_subdev[SUBDEV_INTERNAL_BUF_MAX];
	char					data_type[15];

	struct fimc_is_video_ctx		*vctx;
	struct fimc_is_subdev			*leader;
	const struct fimc_is_subdev_ops		*ops;

	/*
	 * Parameter for DMA abstraction:
	 * This value is physical DMA & VC.
	 * [0]: Bypass PAF HW (Use this when none PD mode is enabled.)
	 * [1]: Processing PAF HW (Use this when PD mode is enabled.)
	 */
	int					dma_ch[SCM_MAX];
	int					vc_ch[SCM_MAX];
};

int fimc_is_sensor_subdev_open(struct fimc_is_device_sensor *device,
	struct fimc_is_video_ctx *vctx);
int fimc_is_sensor_subdev_close(struct fimc_is_device_sensor *device,
	struct fimc_is_video_ctx *vctx);

int fimc_is_ischain_subdev_open(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx);
int fimc_is_ischain_subdev_close(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx);

/*common subdev*/
int fimc_is_subdev_probe(struct fimc_is_subdev *subdev,
	u32 instance,
	u32 id,
	char *name,
	const struct fimc_is_subdev_ops *sops);
int fimc_is_subdev_open(struct fimc_is_subdev *subdev,
	struct fimc_is_video_ctx *vctx,
	void *ctl_data);
int fimc_is_subdev_close(struct fimc_is_subdev *subdev);
int fimc_is_subdev_buffer_queue(struct fimc_is_subdev *subdev, struct vb2_buffer *vb);
int fimc_is_subdev_buffer_finish(struct fimc_is_subdev *subdev, struct vb2_buffer *vb);

void fimc_is_subdev_dis_start(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes);
void fimc_is_subdev_dis_stop(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes);
void fimc_is_subdev_dis_bypass(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes, bool bypass);
void fimc_is_subdev_dnr_start(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes);
void fimc_is_subdev_dnr_stop(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes);
void fimc_is_subdev_dnr_bypass(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes, bool bypass);
void fimc_is_subdev_drc_start(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes);
void fimc_is_subdev_drc_stop(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes);
void fimc_is_subdev_drc_bypass(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes, bool bypass);
void fimc_is_subdev_odc_start(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes);
void fimc_is_subdev_odc_stop(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes);
void fimc_is_subdev_odc_bypass(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes, bool bypass);
int fimc_is_vra_trigger(struct fimc_is_device_ischain *device,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *frame);

int fimc_is_sensor_subdev_reqbuf(void *qdevice,
	struct fimc_is_queue *queue, u32 count);
struct fimc_is_subdev * video2subdev(enum fimc_is_subdev_device_type device_type,
	void *device, u32 vid);

/* internal subdev use */
int fimc_is_subdev_internal_open(void *device, enum fimc_is_device_type type);
int fimc_is_subdev_internal_close(void *device, enum fimc_is_device_type type);
int fimc_is_subdev_internal_s_format(void *device, enum fimc_is_device_type type);
int fimc_is_subdev_internal_start(void *device, enum fimc_is_device_type type);
int fimc_is_subdev_internal_stop(void *device, enum fimc_is_device_type type);

#define GET_SUBDEV_FRAMEMGR(subdev) \
	({ struct fimc_is_framemgr *framemgr;						\
	if ((subdev) && test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &((subdev)->state)))	\
		framemgr = &(subdev)->internal_framemgr;				\
	else if ((subdev) && (subdev)->vctx)						\
		framemgr = &(subdev)->vctx->queue.framemgr;				\
	else										\
		framemgr = NULL;							\
	framemgr;})
#define GET_SUBDEV_QUEUE(subdev) \
	(((subdev) && (subdev)->vctx) ? (&(subdev)->vctx->queue) : NULL)
#define CALL_SOPS(s, op, args...)	(((s) && (s)->ops && (s)->ops->op) ? ((s)->ops->op(s, args)) : 0)

#endif
