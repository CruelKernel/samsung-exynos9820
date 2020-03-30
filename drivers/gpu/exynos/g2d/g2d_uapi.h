/*
 * linux/drivers/gpu/exynos/g2d/g2d_uapi.h
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 * Samsung Graphics 2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _G2D_UAPI_H_
#define _G2D_UAPI_H_

enum g2dsfr_img_register {
	G2DSFR_IMG_STRIDE,
	G2DSFR_IMG_COLORMODE,
	G2DSFR_IMG_LEFT,
	G2DSFR_IMG_TOP,
	G2DSFR_IMG_RIGHT,
	G2DSFR_IMG_BOTTOM,
	G2DSFR_IMG_WIDTH,
	G2DSFR_IMG_HEIGHT,

	G2DSFR_IMG_FIELD_COUNT,
};

enum g2dsfr_src_register {
	G2DSFR_SRC_COMMAND = G2DSFR_IMG_FIELD_COUNT,
	G2DSFR_SRC_SELECT,
	G2DSFR_SRC_ROTATE,
	G2DSFR_SRC_DSTLEFT,
	G2DSFR_SRC_DSTTOP,
	G2DSFR_SRC_DSTRIGHT,
	G2DSFR_SRC_DSTBOTTOM,
	G2DSFR_SRC_SCALECONTROL,
	G2DSFR_SRC_XSCALE,
	G2DSFR_SRC_YSCALE,
	G2DSFR_SRC_XPHASE,
	G2DSFR_SRC_YPHASE,
	G2DSFR_SRC_COLOR,
	G2DSFR_SRC_ALPHA,
	G2DSFR_SRC_BLEND,
	G2DSFR_SRC_YCBCRMODE,
	G2DSFR_SRC_HDRMODE,

	G2DSFR_SRC_FIELD_COUNT
};

enum g2dsfr_dst_register {
	G2DSFR_DST_YCBCRMODE = G2DSFR_IMG_FIELD_COUNT,

	G2DSFR_DST_FIELD_COUNT,
};

struct g2d_reg {
	__u32 offset;
	__u32 value;
};

#define G2D_MAX_PLANES		4
#define G2D_MAX_SFR_COUNT	1024
#define G2D_MAX_BUFFERS		4
#define G2D_MAX_IMAGES		16
#define G2D_MAX_PRIORITY	3
#define G2D_MAX_RELEASE_FENCES	(G2D_MAX_IMAGES + 1)

struct g2d_commands {
	__u32		target[G2DSFR_DST_FIELD_COUNT];
	__u32		*source[G2D_MAX_IMAGES];
	struct g2d_reg	*extra;
	__u32		num_extra_regs;
};

/*
 * The buffer types
 * - G2D_BUFTYPE_NONE    - Invalid or no buffer is configured
 * - G2D_BUFTYPE_EMPTY   - The buffer of the layer has no buffer from user
 * - G2D_BUFTYPE_USERPTR - The buffer is identified by user address
 * - G2D_BUFTYPE_DMABUF  - The buffer is identified by fd and offset
 *
 * G2D_BUFTYPE_EMPTY is only valid if the layer is solid fill for source layers
 * or the buffer is prepared in the kernel space for the target when HWFC is
 * enabled.
 */
#define G2D_BUFTYPE_NONE	0
#define G2D_BUFTYPE_EMPTY	1
#define G2D_BUFTYPE_USERPTR	2
#define G2D_BUFTYPE_DMABUF	3

#define G2D_BUFTYPE_VALID(type) \
	(((type) > 0) && ((type) <= G2D_BUFTYPE_DMABUF))

/*
 * struct g2d_buffer_data - Layer buffer description
 *
 * @userptr	: the buffer address in the address space of the user. It is
 *		  only valid if G2D_BUFTYPE_USERPTR is set in
 *		  g2d_layer_data.buffer_type.
 * @fd		: the open file descriptor of a buffer valid in the address
 *		  space of the user. It is only valid if
 *		  G2D_BUFTYPE_DMABUF is set in g2d_layer_data.buffer_type.
 * @offset	: the offset where the effective data starts from in the given
 *		  buffer. It is only valid if g2d_layer_data.buffer_type is
 *		  G2D_BUFTYPE_DMABUF.
 * @length	: Length in bytes of the buffer.
 * @reserved	: reserved for later use or for the purpose of the client
 *		  driver's private use.
 */
struct g2d_buffer_data {
	union {
		unsigned long userptr;
		struct {
			__s32 fd;
			__u32 offset;
		} dmabuf;
	};
	__u32		length;
};

/* informs the driver that should wait for the given fence to be signaled */
#define G2D_LAYERFLAG_ACQUIRE_FENCE	(1 << 1)
/* informs the driver that sholud protect the buffer */
#define G2D_LAYERFLAG_SECURE		(1 << 2)
/* informs the driver that the layer should be filled with one color */
#define G2D_LAYERFLAG_COLORFILL		(1 << 3)
/* Informs that the image has stride restrictions for MFC */
#define G2D_LAYERFLAG_MFC_STRIDE	(1 << 4)
/* cleaning of the CPU caches is unneccessary before processing */
#define G2D_LAYERFLAG_NO_CACHECLEAN	(1 << 16)
/* invalidation of the CPU caches is unneccessary after processing */
#define G2D_LAYERFLAG_NO_CACHEINV	(1 << 17)

/*
 * struct g2d_layer_data - description of an layer
 *
 * @flags	: flags to determine if an attribute of the layer is valid or
 *		  not. The value in @flags is a combination of
 *		  G2D_LAYERFLAG_XXX.
 * @fence	: a open file descriptor of the acquire fence if
 *		  G2D_IMGFLAG_ACQUIRE_FENCE is set in
 *		  g2d_layer_data.flags.
 * @buffer_type : determines whether g2d_buffer_data.fd or
 *		  g2d_buffer_data.userptr is valid. It sholud be one of
 *		  G2D_BUFTYPE_USERPTR, G2D_BUFTYPE_DMABUF, G2D_BUFTYPE_EMPTY.
 * @num_buffers	: the number of valid elements in @buffer array.
 * @buffer	 : the description of the buffers.
 */
struct g2d_layer_data {
	__u32			flags;
	__s32			fence;
	__u32			buffer_type;
	__u32			num_buffers;
	struct g2d_buffer_data	buffer[G2D_MAX_BUFFERS];
};

/* g2d_task.flags */
/* dithering required at the end of the layer processing */
#define G2D_FLAG_DITHER		(1 << 1)
/*
 * G2D_IOC_PROCESS ioctl returns immediately after validation of the
 * values in the parameters finishes. The user should call ioctl with
 * G2D_IOC_WAIT_PROCESS command to get the result
 */
#define G2D_FLAG_NONBLOCK	(1 << 2)
/*
 * indicate that the task should be processed using hwfc between
 * g2d and mfc device.
 */
#define G2D_FLAG_HWFC		(1 << 3)
/*
 * indicate that the task will be operated one task at a time
 * by APB interface mode.
 */
#define G2D_FLAG_APB		(1 << 4)
/*
 * indicate that an error occurred during layer processing. Configured by the
 * framework. The users should check if an error occurred on return of ioctl.
 */
#define G2D_FLAG_ERROR		(1 << 5)

/*
 * struct g2d_task - description of a layer processing task
 * @version: the version of the format of struct g2d_task_data
 * @flags: the flags to indicate the status or the attributes of the layer
 *         processing. It should be the combination of the values defined by
 *         G2D_FLAG_XXX.
 * @laptime_in_usec: delay in processing
 * @priority: the task's priority for H/W job manager.
 * @num_sources	: the number of valid source layers.
 * @num_release_fences: the number of requested release fence.
 * @release_fences: pointer to an array of release fences with
 *                  @num_release_fences elements
 * @target: the description of the target layer that is the result of
 *          the layer processing
 * @source: the descriptions of source layers.
 * @commands: command data
 */
struct g2d_task_data {
	__u32			version;
	__u32			flags;
	__u32			laptime_in_usec;
	__u32			priority;
	__u32			num_source;
	__u32			num_release_fences;
	__s32			*release_fences;
	struct g2d_layer_data	target;
	struct g2d_layer_data	*source;
	struct g2d_commands	commands;
};

/* flags of g2d_performance_layer_data.layer_attr */
#define G2D_PERF_LAYER_ROTATE		(1 << 0)
#define G2D_PERF_LAYER_SCALING		(1 << 1)
#define G2D_PERF_LAYER_YUV2P		(1 << 4)
#define G2D_PERF_LAYER_YUV2P_82		(1 << 5)
#define G2D_PERF_LAYER_COMPRESSED	(1 << 6)

#define G2D_PERF_LAYER_FMTMASK		(7 << 4)

/*
 * struct g2d_performance_frame_data - description of needed performance.
 * @crop_width/height : the width and height of the image that
 *		participates in the image processing
 * @window_width/height : the width and height of valid rectangle,
 *		the region is drawn on target
 * @layer_attr : attribute of layer affecting performance.
 */
struct g2d_performance_layer_data {
	__u16 crop_w;
	__u16 crop_h;
	__u16 window_w;
	__u16 window_h;
	__u32 layer_attr;
};

/* flags of g2d_performance_frame_data.frame_attr */
#define G2D_PERF_FRAME_SOLIDCOLORFILL	(1 << 0)
#define G2D_PERF_FRAME_YUV2P	(1 << 1)

/*
 * struct g2d_performance_frame_data - description of needed performance.
 * @layer : the pixel count of each layer to be processed.
 * @bandwidth_read : the size of bandwidth to read when processing.
 * @bandwidth_write : the size of bandwidth to write when processing.
 * @frame_rate : frame per second of the job.
 * @frame_attr : frame attribute
 * @num_layers : the number of layers to be processed.
 */
struct g2d_performance_frame_data {
	struct g2d_performance_layer_data layer[G2D_MAX_IMAGES];
	__u32 bandwidth_read;
	__u32 bandwidth_write;
	__u32 target_pixelcount;
	__u32 frame_rate;
	__u32 frame_attr;
	__u32 num_layers;
};

#define G2D_PERF_MAX_FRAMES 4

/*
 * struct g2d_performance_data - description the needed performance.
 * @frame: the descriptions of each request's bandwidth and cycles in a frame.
 * @num_frame : the number of g2d job requested in a frame.
 */
struct g2d_performance_data {
	struct g2d_performance_frame_data frame[G2D_PERF_MAX_FRAMES];
	__u32 num_frame;
	__u32 reserved;
};

#define G2D_IOC_PROCESS		_IOWR('M', 4, struct g2d_task_data)
#define G2D_IOC_PRIORITY		_IOR('M', 5, int32_t)
#define G2D_IOC_PERFORMANCE	_IOR('M', 6, struct g2d_performance_data)

#endif /* _G2D_UAPI_H_ */


