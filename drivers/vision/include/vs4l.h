/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VISION_FOR_LINUX_H_
#define VISION_FOR_LINUX_H_

#define VS4L_VERSION		5
#define VS4L_TARGET_SC		0xFFFF
#define VS4L_TARGET_SC_SHIFT	16
#define VS4L_TARGET_PU		0xFFFF
#define VS4L_TARGET_PU_SHIFT	0

enum vs4l_graph_flag {
	VS4L_GRAPH_FLAG_FIXED,
	VS4L_GRAPH_FLAG_SHARED_AMONG_SUBCHAINS,
	VS4L_GRAPH_FLAG_SHARING_AMONG_SUBCHAINS_PREFERRED,
	VS4L_GRAPH_FLAG_SHARED_AMONG_TASKS,
	VS4L_GRAPH_FLAG_SHARING_AMONG_TASKS_PREFERRED,
	VS4L_GRAPH_FLAG_DSBL_LATENCY_BALANCING,
	VS4L_STATIC_ALLOC_LARGE_MPRB_INSTEAD_SMALL_FLAG,
	VS4L_STATIC_ALLOC_PU_INSTANCE_LSB,
	VS4L_GRAPH_FLAG_PRIMITIVE,
	VS4L_GRAPH_FLAG_EXCLUSIVE,
	VS4L_GRAPH_FLAG_PERIODIC,
	VS4L_GRAPH_FLAG_ROUTING,
	VS4L_GRAPH_FLAG_BYPASS,
	VS4L_GRAPH_FLAG_END
};

struct vs4l_graph {
	__u32			id;
	__u32			priority;
	__u32			time; /* in millisecond */
	__u32			flags;
	__u32			size;
	unsigned long		addr;
};

struct vs4l_format {
	__u32			target;
	__u32			format;
	__u32			plane;
	__u32			width;
	__u32			height;
	__u32			stride;
	__u32			cstride;
	__u32			channels;
	__u32			pixel_format;
};

struct vs4l_format_list {
	__u32			direction;
	__u32			count;
	struct vs4l_format	*formats;
};

struct vs4l_param {
	__u32			target;
	unsigned long		addr;
	__u32			offset;
	__u32			size;
};

struct vs4l_param_list {
	__u32			count;
	struct vs4l_param	*params;
};

struct vs4l_ctrl {
	__u32			ctrl;
	__u32			value;
};

struct vs4l_roi {
	__u32			x;
	__u32			y;
	__u32			w;
	__u32			h;
};

struct vs4l_buffer {
	struct vs4l_roi		roi;
	union {
		unsigned long	userptr;
		__s32		fd;
	} m;
	unsigned long		reserved;
};

enum vs4l_buffer_type {
	VS4L_BUFFER_LIST,
	VS4L_BUFFER_ROI,
	VS4L_BUFFER_PYRAMID
};

enum vs4l_memory {
	VS4L_MEMORY_USERPTR = 1,
	VS4L_MEMORY_VIRTPTR,
	VS4L_MEMORY_DMABUF
};

struct vs4l_container {
	__u32			type;
	__u32			target;
	__u32			memory;
	__u32			reserved[4];
	__u32			count;
	struct vs4l_buffer	*buffers;
};

enum vs4l_direction {
	VS4L_DIRECTION_IN = 1,
	VS4L_DIRECTION_OT
};

enum vs4l_cl_flag {
	VS4L_CL_FLAG_TIMESTAMP,
	VS4L_CL_FLAG_PREPARE = 8,
	VS4L_CL_FLAG_INVALID,
	VS4L_CL_FLAG_DONE
};

struct vs4l_container_list {
	__u32			direction;
	__u32			id;
	__u32			index;
	__u32			flags;
	struct timeval		timestamp[6];
	__u32			count;
	struct vs4l_container	*containers;
};

/*
 *      F O U R C C  C O D E S   F O R   V I S I O N
 *
 */

#define VS4L_DF_IMAGE(a, b, c, d)			((a) | (b << 8) | (c << 16) | (d << 24))
#define VS4L_DF_IMAGE_RGB			VS4L_DF_IMAGE('R', 'G', 'B', '2')
#define VS4L_DF_IMAGE_RGBX			VS4L_DF_IMAGE('R', 'G', 'B', 'A')
#define VS4L_DF_IMAGE_NV12			VS4L_DF_IMAGE('N', 'V', '1', '2')
#define VS4L_DF_IMAGE_NV21			VS4L_DF_IMAGE('N', 'V', '2', '1')
#define VS4L_DF_IMAGE_YUYV			VS4L_DF_IMAGE('Y', 'U', 'Y', 'V')
#define VS4L_DF_IMAGE_YUV4			VS4L_DF_IMAGE('Y', 'U', 'V', '4')
#define VS4L_DF_IMAGE_U8			VS4L_DF_IMAGE('U', '0', '0', '8')
#define VS4L_DF_IMAGE_U16			VS4L_DF_IMAGE('U', '0', '1', '6')
#define VS4L_DF_IMAGE_U32			VS4L_DF_IMAGE('U', '0', '3', '2')
#define VS4L_DF_IMAGE_S16			VS4L_DF_IMAGE('S', '0', '1', '6')
#define VS4L_DF_IMAGE_S32			VS4L_DF_IMAGE('S', '0', '3', '2')
#define VS4L_DF_IMAGE_NPU			VS4L_DF_IMAGE('N', 'P', 'U', '0')

/*
 *      I O C T L   C O D E S   F O R   V E R T E X   D E V I C E
 *
 */

#define VS4L_VERTEXIOC_S_GRAPH			_IOW('V', 0, struct vs4l_graph)
#define VS4L_VERTEXIOC_S_FORMAT			_IOW('V', 1, struct vs4l_format_list)
#define VS4L_VERTEXIOC_S_PARAM			_IOW('V', 2, struct vs4l_param_list)
#define VS4L_VERTEXIOC_S_CTRL			_IOW('V', 3, struct vs4l_ctrl)
#define VS4L_VERTEXIOC_STREAM_ON		_IO('V', 4)
#define VS4L_VERTEXIOC_STREAM_OFF		_IO('V', 5)
#define VS4L_VERTEXIOC_QBUF			_IOW('V', 6, struct vs4l_container_list)
#define VS4L_VERTEXIOC_DQBUF			_IOW('V', 7, struct vs4l_container_list)
#define VS4L_VERTEXIOC_PREPARE			_IOW('V', 8, struct vs4l_container_list)
#define VS4L_VERTEXIOC_UNPREPARE		_IOW('V', 9, struct vs4l_container_list)

#endif
