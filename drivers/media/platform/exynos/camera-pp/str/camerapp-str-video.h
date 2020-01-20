/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Exynos CAMERA-PP GDC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef CAMERAPP_STR_VIDEO_H
#define CAMERAPP_STR_VIDEO_H

#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>

#include "camerapp-video.h"
#include "camerapp-str-metadata.h"

#define STR_VIDEO_NAME		("exynos-str-video")
#define STR_NUM_METAPLANE	(1)

#define vb2_to_str_buf(x)	container_of(x, struct str_vb2_buffer, vb2_buf)
#define STR_NUM_PLANES(x)	(x + STR_NUM_METAPLANE)

/*
 * struct str_video - The video node for STR video device
 * @v4l2_dev:	v4l2 device structure.
 * @video_dev:	video device structure.
 * @video_id:	video node number.
 * @lock:	mutex for str_core data structure
 * @open_cnt:	number to count the opened video node context.
 */
struct str_video {
	struct v4l2_device	v4l2_dev;
	struct video_device	video_dev;
	struct mutex		lock;
	int			video_id;
	atomic_t		open_cnt;
};

/*
 * struct str_fmt = Image format data structure
 * @name:	human readable format name.
 * @pix_fmt:	V4L2 pixel format code.
 * @bit_depth:	bits per pixel for each plane.
 * @num_planes:	the number of physically non-contiguous image planes
 * @num_comp:	the number of color components (or color channel).
 */
struct str_fmt {
	char	*name;
	u32	pix_fmt;
	u8	bit_depth[VIDEO_MAX_PLANES];
	u8	num_planes;
	u8	num_comp;
};

/*
 * struct str_frame_cfg - Frame configuration
 * @fmt:		Image color format information.
 * @width:		Image horizontal pixel size.
 * @height:		Image vertical pixel size.
 * @bytesperline:	byte size for each line in plane.
 * @size:		byte size for each plane.
 * @meta:		user control metadata structure
 */
struct str_frame_cfg {
	const struct str_fmt	*fmt;
	u32			width;
	u32			height;
	u32			bytesperline[VIDEO_MAX_PLANES];
	u32			size[VIDEO_MAX_PLANES];
	struct str_metadata	*meta;
};

/*
 * struct str_vb2_buffer - Video buffer information for STR device driver
 * @vb2_buf:	vb2 buffer structure.
 * @dva:	device virtual address for each plane.
 * @kva		kernel virtual address for each plane.
 * @size	byte size for each plane.
 */
struct str_vb2_buffer {
	struct vb2_buffer	vb2_buf;
	dma_addr_t		dva[VIDEO_MAX_PLANES];
	ulong			kva[VIDEO_MAX_PLANES];
	u32			size[VIDEO_MAX_PLANES];
};

/*
 * struct str_priv_buf - Internal buffer structure for STR H/W
 */
struct str_priv_buf {
	dma_addr_t			iova;
	size_t				size;

	struct dma_buf			*dma_buf;
	struct dma_buf_attachment	*dma_attach;
	enum dma_data_direction		direction;
	struct sg_table			*sgt;

	struct str_ion_ctx		*ion_ctx;
};

/*
 * struct str_ctx - The instance context structure for STR video node 'open()'
 * @video:	pointer to str_video structure.
 * @vb2_q:	vb2_queue structure for STR video node
 * @frame_cfg:	frame configuration for current video context.
 * @buf:	pointer to str_vb2_buffer structure.
 * @buf_y:	pointer to the internal ION memory for Luma.
 * @buf_c:	pointer to the internal ION memory for Chroma.
 */
struct str_ctx {
	struct str_video	*video;
	struct vb2_queue	vb2_q;
	struct str_frame_cfg	frame_cfg;
	struct str_vb2_buffer	*buf;
	struct str_priv_buf	*buf_y;
	struct str_priv_buf	*buf_c;
};

int str_video_probe(void *data);

#endif //CAMERAPP_STR_VIDEO_H
