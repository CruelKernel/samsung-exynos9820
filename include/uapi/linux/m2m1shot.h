/*
 *  M2M One-shot header file
 *    - Handling H/W accellerators for non-streaming media processing
 *
 *  Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Alternatively you can redistribute this file under the terms of the
 *  BSD license as stated below:
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *  3. The names of its contributors may not be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _UAPI__M2M1SHOT_H_
#define _UAPI__M2M1SHOT_H_

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/videodev2.h>

#define M2M1SHOT_MAX_PLANES 3

struct m2m1shot_rect {
	__s16 left;
	__s16 top;
	__u16 width;
	__u16 height;
};

struct m2m1shot_pix_format {
	__u32 fmt;
	__u32 width;
	__u32 height;
	struct v4l2_rect crop;
};

enum m2m1shot_buffer_type {
	M2M1SHOT_BUFFER_NONE,	/* no buffer is set */
	M2M1SHOT_BUFFER_DMABUF,
	M2M1SHOT_BUFFER_USERPTR,
};

struct m2m1shot_buffer_plane {
	union {
		__s32 fd;
		unsigned long userptr;
	};
	size_t len;
};

struct m2m1shot_buffer {
	struct m2m1shot_buffer_plane plane[M2M1SHOT_MAX_PLANES];
	__u8 type;
	__u8 num_planes;
};

#define M2M1SHOT_OP_FLIP_VIRT		(1 << 0)
#define M2M1SHOT_OP_FLIP_HORI		(1 << 1)
#define M2M1SHOT_OP_CSC_WIDE		(1 << 8)
#define M2M1SHOT_OP_CSC_NARROW		(1 << 9)
#define M2M1SHOT_OP_CSC_601		(1 << 10)
#define M2M1SHOT_OP_CSC_709		(1 << 11)
#define M2M1SHOT_OP_PREMULTIPLIED_ALPHA	(1 << 16)
#define M2M1SHOT_OP_DITHERING		(1 << 17)

struct m2m1shot_operation {
	__s16 quality_level;
	__s16 rotate;
	__u32 op; /* or-ing M2M1SHOT_FLIP_OP_VIRT/HORI */
};

struct m2m1shot {
	struct m2m1shot_pix_format fmt_out;
	struct m2m1shot_pix_format fmt_cap;
	struct m2m1shot_buffer buf_out;
	struct m2m1shot_buffer buf_cap;
	struct m2m1shot_operation op;
	unsigned long reserved[2];
};

struct m2m1shot_custom_data {
	unsigned int cmd;
	unsigned long arg;
};

#define M2M1SHOT_IOC_PROCESS	_IOWR('M',  0, struct m2m1shot)
#define M2M1SHOT_IOC_CUSTOM	_IOWR('M', 16, struct m2m1shot_custom_data)

#endif /* _UAPI__M2M1SHOT_H_ */
