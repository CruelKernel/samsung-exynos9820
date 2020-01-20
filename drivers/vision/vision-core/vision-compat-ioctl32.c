/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/bug.h>
#include <asm/uaccess.h>

#include "vision-config.h"
#include "vision-dev.h"
#include "vision-ioctl.h"
#include "vs4l.h"

struct vs4l_graph32 {
	__u32			id;
	__u32			priority;
	__u32			time;
	__u32			flags;
	__u32			size;
	compat_caddr_t		addr;
};

struct vs4l_format32 {
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

struct vs4l_format_list32 {
	__u32			direction;
	__u32			count;
	compat_caddr_t		formats;
};

struct vs4l_param32 {
	__u32			target;
	compat_caddr_t		addr;
	__u32			offset;
	__u32			size;
};

struct vs4l_param_list32 {
	__u32			count;
	compat_caddr_t		params;
};

struct vs4l_ctrl32 {
	__u32			ctrl;
	__u32			value;
};

struct vs4l_roi32 {
	__u32			x;
	__u32			y;
	__u32			w;
	__u32			h;
};

struct vs4l_buffer32 {
	struct vs4l_roi		roi;
	union {
		compat_caddr_t	userptr;
		__s32		fd;
	} m;
	compat_caddr_t		reserved;
};

struct vs4l_container32 {
	__u32			type;
	__u32			target;
	__u32			memory;
	__u32			reserved[4];
	__u32			count;
	compat_caddr_t		buffers;
};

struct vs4l_container_list32 {
	__u32			direction;
	__u32			id;
	__u32			index;
	__u32			flags;
	struct compat_timeval	timestamp[6];
	__u32			count;
	compat_caddr_t		containers;
};

#define VS4L_VERTEXIOC_S_GRAPH32		_IOW('V', 0, struct vs4l_graph32)
#define VS4L_VERTEXIOC_S_FORMAT32		_IOW('V', 1, struct vs4l_format_list32)
#define VS4L_VERTEXIOC_S_PARAM32		_IOW('V', 2, struct vs4l_param_list32)
#define VS4L_VERTEXIOC_S_CTRL32		_IOW('V', 3, struct vs4l_ctrl32)
#define VS4L_VERTEXIOC_QBUF32			_IOW('V', 6, struct vs4l_container_list32)
#define VS4L_VERTEXIOC_DQBUF32			_IOW('V', 7, struct vs4l_container_list32)
#define VS4L_VERTEXIOC_PREPARE32		_IOW('V', 8, struct vs4l_container_list32)
#define VS4L_VERTEXIOC_UNPREPARE32		_IOW('V', 9, struct vs4l_container_list32)

static int get_vs4l_graph32(struct vs4l_graph *kp, struct vs4l_graph32 __user *up)
{
	int ret = 0;

	if (!access_ok(VERIFY_READ, up, sizeof(struct vs4l_graph32)) ||
		get_user(kp->id, &up->id) ||
		get_user(kp->flags, &up->flags) ||
		get_user(kp->time, &up->time) ||
		get_user(kp->size, &up->size) ||
		get_user(kp->addr, &up->addr) ||
		get_user(kp->priority, &up->priority)) {
		vision_err("get_user is fail1\n");
		ret = -EFAULT;
		goto p_err;
	}

p_err:
	return ret;
}

static void put_vs4l_graph32(struct vs4l_graph *kp, struct vs4l_graph32 __user *up)
{
}

static int get_vs4l_format32(struct vs4l_format_list *kp, struct vs4l_format_list32 __user *up)
{
	int ret = 0;
	u32 index;
	size_t size;
	compat_caddr_t p;
	struct vs4l_format32 __user *uformats32;
	struct vs4l_format *kformats;

	if (!access_ok(VERIFY_READ, up, sizeof(struct vs4l_format_list32)) ||
		get_user(kp->direction, &up->direction) ||
		get_user(kp->count, &up->count)) {
		vision_err("get_user is fail\n");
		ret = -EFAULT;
		goto p_err;
	}

	if (get_user(p, &up->formats)) {
		vision_err("get_user(formats) is fail\n");
		ret = -EFAULT;
		goto p_err;
	}

	size = kp->count * sizeof(struct vs4l_format32);
	uformats32 = compat_ptr(p);
	if (!access_ok(VERIFY_READ, uformats32, size)) {
		vision_err("acesss is fail\n");
		ret = -EFAULT;
		goto p_err;
	}

	size = kp->count * sizeof(struct vs4l_format);
	kformats = kmalloc(size, GFP_KERNEL);
	if (!kformats) {
		vision_err("kmalloc is fail\n");
		ret = -ENOMEM;
		goto p_err;
	}

	for (index = 0; index < kp->count; ++index) {
		if (get_user(kformats[index].target, &uformats32[index].target)) {
			vision_err("get_user(target) is fail\n");
			ret = -EFAULT;
			goto p_err_format_alloc;
		}

		if (get_user(kformats[index].format, &uformats32[index].format)) {
			vision_err("get_user(format) is fail\n");
			ret = -EFAULT;
			goto p_err_format_alloc;
		}

		if (get_user(kformats[index].plane, &uformats32[index].plane)) {
			vision_err("get_user(plane) is fail\n");
			ret = -EFAULT;
			goto p_err_format_alloc;
		}

		if (get_user(kformats[index].width, &uformats32[index].width)) {
			vision_err("get_user(width) is fail\n");
			ret = -EFAULT;
			goto p_err_format_alloc;
		}

		if (get_user(kformats[index].height, &uformats32[index].height)) {
			vision_err("get_user(height) is fail\n");
			ret = -EFAULT;
			goto p_err_format_alloc;
		}

		if (get_user(kformats[index].stride, &uformats32[index].stride)) {
			vision_err("get_user(stride) is fail\n");
			ret = -EFAULT;
			goto p_err_format_alloc;
		}

		if (get_user(kformats[index].cstride, &uformats32[index].cstride)) {
			vision_err("get_user(cstride) is fail\n");
			ret = -EFAULT;
			goto p_err_format_alloc;
		}

		if (get_user(kformats[index].channels, &uformats32[index].channels)) {
			vision_err("get_user(channels) is fail\n");
			ret = -EFAULT;
			goto p_err_format_alloc;
		}

		if (get_user(kformats[index].pixel_format, &uformats32[index].pixel_format)) {
			vision_err("get_user(pixel_format) is fail\n");
			ret = -EFAULT;
			goto p_err_format_alloc;
		}
	}

	kp->formats = kformats;
	return ret;

p_err_format_alloc:
	kfree(kformats);
	kp->formats = NULL;

p_err:
	vision_err("Return with failure (%d)\n", ret);
	return ret;
}

static void put_vs4l_format32(struct vs4l_format_list *kp, struct vs4l_format_list32 __user *up)
{
	kfree(kp->formats);
}

static int get_vs4l_param32(struct vs4l_param_list *kp, struct vs4l_param_list32 __user *up)
{
	int ret = 0;
	u32 index;
	size_t size;
	compat_caddr_t p;
	struct vs4l_param32 __user *uparams32;
	struct vs4l_param *kparams;

	if (!access_ok(VERIFY_READ, up, sizeof(struct vs4l_param_list32)) ||
		get_user(kp->count, &up->count)) {
		vision_err("get_user is fail\n");
		ret = -EFAULT;
		goto p_err;
	}

	if (get_user(p, &up->params)) {
		vision_err("get_user(params) is fail\n");
		ret = -EFAULT;
		goto p_err;
	}

	size = kp->count * sizeof(struct vs4l_param32);
	uparams32 = compat_ptr(p);
	if (!access_ok(VERIFY_READ, uparams32, size)) {
		vision_err("access is fail\n");
		ret = -EFAULT;
		goto p_err;
	}

	size = kp->count * sizeof(struct vs4l_param);
	kparams = kmalloc(size, GFP_KERNEL);
	if (!kparams) {
		vision_err("kmalloc is fail\n");
		ret = -ENOMEM;
		goto p_err;
	}

	for (index = 0; index < kp->count; ++index) {
		if (get_user(kparams[index].target, &uparams32[index].target)) {
			vision_err("get_user(target) is fail\n");
			ret = -EFAULT;
			goto p_err_alloc;
		}

		if (get_user(kparams[index].addr, &uparams32[index].addr)) {
			vision_err("get_user(addr) is fail\n");
			ret = -EFAULT;
			goto p_err_alloc;
		}

		if (get_user(kparams[index].offset, &uparams32[index].offset)) {
			vision_err("get_user(offset) is fail\n");
			ret = -EFAULT;
			goto p_err_alloc;
		}

		if (get_user(kparams[index].size, &uparams32[index].size)) {
			vision_err("get_user(size) is fail\n");
			ret = -EFAULT;
			goto p_err_alloc;
		}
	}

	kp->params = kparams;
	return ret;

p_err_alloc:
	kfree(kparams);
	kp->params = NULL;

p_err:
	vision_err("Return with failure (%d)\n", ret);
	return ret;
}

static void put_vs4l_param32(struct vs4l_param_list *kp, struct vs4l_param_list32 __user *up)
{
	kfree(kp->params);
}

static int get_vs4l_ctrl32(struct vs4l_ctrl *kp, struct vs4l_ctrl32 __user *up)
{
	int ret = 0;

	if (!access_ok(VERIFY_READ, up, sizeof(struct vs4l_ctrl32)) ||
		get_user(kp->ctrl, &up->ctrl) ||
		get_user(kp->value, &up->value)) {
		vision_err("get_user is fail\n");
		ret = -EFAULT;
		goto p_err;
	}

p_err:
	return ret;
}

static void put_vs4l_ctrl32(struct vs4l_ctrl *kp, struct vs4l_ctrl32 __user *up)
{
}

static int get_vs4l_container32(struct vs4l_container_list *kp, struct vs4l_container_list32 __user *up)
{
	int ret = 0;
	u32 i, j;
	size_t size;
	compat_caddr_t p;
	struct vs4l_container32 *ucontainer32;
	struct vs4l_container *kcontainer;
	struct vs4l_buffer32 *ubuffer32;
	struct vs4l_buffer *kbuffer;

	if (!access_ok(VERIFY_READ, up, sizeof(struct vs4l_container_list32)) ||
		get_user(kp->direction, &up->direction) ||
		get_user(kp->id, &up->id) ||
		get_user(kp->index, &up->index) ||
		get_user(kp->flags, &up->flags) ||
		get_user(kp->count, &up->count)) {
		vision_err("get_user is fail\n");
		ret = -EFAULT;
		goto p_err;
	}

	if (get_user(p, &up->containers)) {
		vision_err("get_user(containers) is fail\n");
		ret = -EFAULT;
		goto p_err;
	}

	size = sizeof(struct vs4l_container32) * kp->count;
	ucontainer32 = compat_ptr(p);
	if (!access_ok(VERIFY_READ, ucontainer32, size)) {
		vision_err("access is fail\n");
		ret = -EFAULT;
		goto p_err;
	}

	size = sizeof(struct vs4l_container) * kp->count;
	kcontainer = kzalloc(size, GFP_KERNEL);
	if (!kcontainer) {
		vision_err("kmalloc is fail(%zu, %u)\n", size, kp->count);
		ret = -ENOMEM;
		goto p_err;
	}

	kp->containers = kcontainer;

	for (i = 0; i < kp->count; ++i) {
		if (get_user(kcontainer[i].type, &ucontainer32[i].type)) {
			vision_err("get_user(type) is fail\n");
			ret = -EFAULT;
			goto p_err_container_alloc;
		}

		if (get_user(kcontainer[i].target, &ucontainer32[i].target)) {
			vision_err("get_user(target) is fail\n");
			ret = -EFAULT;
			goto p_err_container_alloc;
		}

		if (get_user(kcontainer[i].memory, &ucontainer32[i].memory)) {
			vision_err("get_user(memory) is fail\n");
			ret = -EFAULT;
			goto p_err_container_alloc;
		}

		if (get_user(kcontainer[i].reserved[0], &ucontainer32[i].reserved[0])) {
			vision_err("get_user(reserved[0]) is fail\n");
			ret = -EFAULT;
			goto p_err_container_alloc;
		}

		if (get_user(kcontainer[i].reserved[1], &ucontainer32[i].reserved[1])) {
			vision_err("get_user(reserved[1]) is fail\n");
			ret = -EFAULT;
			goto p_err_container_alloc;
		}

		if (get_user(kcontainer[i].reserved[2], &ucontainer32[i].reserved[2])) {
			vision_err("get_user(reserved[2]) is fail\n");
			ret = -EFAULT;
			goto p_err_container_alloc;
		}

		if (get_user(kcontainer[i].reserved[3], &ucontainer32[i].reserved[3])) {
			vision_err("get_user(reserved[3]) is fail\n");
			ret = -EFAULT;
			goto p_err_container_alloc;
		}

		if (get_user(kcontainer[i].count, &ucontainer32[i].count)) {
			vision_err("get_user(count) is fail\n");
			ret = -EFAULT;
			goto p_err_container_alloc;
		}

		if (get_user(p, &ucontainer32[i].buffers)) {
			vision_err("get_user(buffers) is fail\n");
			ret = -EFAULT;
			goto p_err_container_alloc;
		}


		size = sizeof(struct vs4l_buffer32) * kcontainer[i].count;
		ubuffer32 = compat_ptr(p);
		if (!access_ok(VERIFY_READ, ubuffer32, size)) {
			vision_err("access is fail\n");
			ret = -EFAULT;
			goto p_err_container_alloc;
		}

		size = sizeof(struct vs4l_buffer) * kcontainer[i].count;
		kbuffer = kmalloc(size, GFP_KERNEL);
		if (!kbuffer) {
			vision_err("kmalloc is fail(size : %zu)\n", size);
			ret = -ENOMEM;
			goto p_err_container_alloc;
		}

		kcontainer[i].buffers = kbuffer;

		for (j = 0; j < kcontainer[i].count; ++j) {
			if (get_user(kbuffer[j].roi.x, &ubuffer32[j].roi.x)) {
				vision_err("get_user(roi.x) is fail\n");
				ret = -EFAULT;
				goto p_err_container_alloc;
			}

			if (get_user(kbuffer[j].roi.y, &ubuffer32[j].roi.y)) {
				vision_err("get_user(roi.y) is fail\n");
				ret = -EFAULT;
				goto p_err_container_alloc;
			}

			if (get_user(kbuffer[j].roi.w, &ubuffer32[j].roi.w)) {
				vision_err("get_user(roi.w) is fail\n");
				ret = -EFAULT;
				goto p_err_container_alloc;
			}

			if (get_user(kbuffer[j].roi.h, &ubuffer32[j].roi.h)) {
				vision_err("get_user(roi.h) is fail\n");
				ret = -EFAULT;
				goto p_err_container_alloc;
			}

			if (kcontainer[i].memory == VS4L_MEMORY_DMABUF) {
				if (get_user(kbuffer[j].m.fd, &ubuffer32[j].m.fd)) {
					vision_err("get_user(fd) is fail\n");
					ret = -EFAULT;
					goto p_err_container_alloc;
				}
			} else {
				if (get_user(kbuffer[j].m.userptr, &ubuffer32[j].m.userptr)) {
					vision_err("get_user(userptr) is fail\n");
					ret = -EFAULT;
					goto p_err_container_alloc;
				}
			}
		}
	}
	return ret;

p_err_container_alloc:
	for (i = 0; i < kp->count; ++i) {
		if (kcontainer[i].buffers) {
			kfree(kcontainer[i].buffers);
			kcontainer[i].buffers = NULL;
		}
	}

	if (kp->containers) {
		kfree(kp->containers);
		kp->containers = NULL;
	}

p_err:
	return ret;
}

static void put_vs4l_container32(struct vs4l_container_list *kp, struct vs4l_container_list32 __user *up)
{
	u32 i;

	if (!access_ok(VERIFY_WRITE, up, sizeof(struct vs4l_container_list32)) ||
		put_user(kp->flags, &up->flags) ||
		put_user(kp->index, &up->index) ||
		put_user(kp->id, &up->id) ||
		put_user(kp->timestamp[0].tv_sec, &up->timestamp[0].tv_sec) ||
		put_user(kp->timestamp[0].tv_usec, &up->timestamp[0].tv_usec) ||
		put_user(kp->timestamp[1].tv_sec, &up->timestamp[1].tv_sec) ||
		put_user(kp->timestamp[1].tv_usec, &up->timestamp[1].tv_usec) ||
		put_user(kp->timestamp[2].tv_sec, &up->timestamp[2].tv_sec) ||
		put_user(kp->timestamp[2].tv_usec, &up->timestamp[2].tv_usec) ||
		put_user(kp->timestamp[3].tv_sec, &up->timestamp[3].tv_sec) ||
		put_user(kp->timestamp[3].tv_usec, &up->timestamp[3].tv_usec) ||
		put_user(kp->timestamp[4].tv_sec, &up->timestamp[4].tv_sec) ||
		put_user(kp->timestamp[4].tv_usec, &up->timestamp[4].tv_usec) ||
		put_user(kp->timestamp[5].tv_sec, &up->timestamp[5].tv_sec) ||
		put_user(kp->timestamp[5].tv_usec, &up->timestamp[5].tv_usec)) {
		vision_err("write access error\n");
		BUG();
	}

	for (i = 0; i < kp->count; ++i)
		kfree(kp->containers[i].buffers);

	kfree(kp->containers);
}

long vertex_compat_ioctl32(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	union {
		struct vs4l_graph vsgp;
		struct vs4l_format_list vsfl;
		struct vs4l_param_list vspl;
		struct vs4l_ctrl vsct;
		struct vs4l_container_list vscl;
	} karg;
	void __user *up = compat_ptr(arg);
	struct vision_device *vdev = vision_devdata(file);
	const struct vertex_ioctl_ops *ops = vdev->ioctl_ops;

	switch (cmd) {
	case VS4L_VERTEXIOC_S_GRAPH32:
		ret = get_vs4l_graph32(&karg.vsgp, up);
		if (ret) {
			vision_err("get_vs4l_graph32 is fail(%ld)\n", ret);
			goto p_err;
		}

		ret = ops->vertexioc_s_graph(file, &karg.vsgp);
		put_vs4l_graph32(&karg.vsgp, up);
		break;
	case VS4L_VERTEXIOC_S_FORMAT32:
		ret = get_vs4l_format32(&karg.vsfl, up);
		if (ret) {
			vision_err("get_vs4l_format32 is fail(%ld)\n", ret);
			goto p_err;
		}

		ret = ops->vertexioc_s_format(file, &karg.vsfl);
		put_vs4l_format32(&karg.vsfl, up);
		break;
	case VS4L_VERTEXIOC_S_PARAM32:
		ret = get_vs4l_param32(&karg.vspl, up);
		if (ret) {
			vision_err("get_vs4l_param32 is fail(%ld)\n", ret);
			goto p_err;
		}

		ret = ops->vertexioc_s_param(file, &karg.vspl);
		put_vs4l_param32(&karg.vspl, up);
		break;
	case VS4L_VERTEXIOC_S_CTRL32:
		ret = get_vs4l_ctrl32(&karg.vsct, up);
		if (ret) {
			vision_err("get_vs4l_ctrl32 is fail(%ld)\n", ret);
			goto p_err;
		}

		ret = ops->vertexioc_s_ctrl(file, &karg.vsct);
		put_vs4l_ctrl32(&karg.vsct, up);
		break;
	case VS4L_VERTEXIOC_STREAM_ON:
		ret = ops->vertexioc_streamon(file);
		break;
	case VS4L_VERTEXIOC_STREAM_OFF:
		ret = ops->vertexioc_streamoff(file);
		break;
	case VS4L_VERTEXIOC_QBUF32:
		ret = get_vs4l_container32(&karg.vscl, up);
		if (ret) {
			vision_err("qbuf, get_vs4l_container32 is fail(%ld)\n", ret);
			goto p_err;
		}

		ret = ops->vertexioc_qbuf(file, &karg.vscl);
		put_vs4l_container32(&karg.vscl, up);
		break;
	case VS4L_VERTEXIOC_DQBUF32:
		ret = get_vs4l_container32(&karg.vscl, up);
		if (ret) {
			vision_err("dqbuf, get_vs4l_container32 is fail(%ld)\n", ret);
			goto p_err;
		}

		ret = ops->vertexioc_dqbuf(file, &karg.vscl);
		put_vs4l_container32(&karg.vscl, up);
		break;
	case VS4L_VERTEXIOC_PREPARE32:
		ret = get_vs4l_container32(&karg.vscl, up);
		if (ret) {
			vision_err("qbuf, get_vs4l_container32 is fail(%ld)\n", ret);
			goto p_err;
		}

		ret = ops->vertexioc_prepare(file, &karg.vscl);
		put_vs4l_container32(&karg.vscl, up);
		break;
	case VS4L_VERTEXIOC_UNPREPARE32:
		ret = get_vs4l_container32(&karg.vscl, up);
		if (ret) {
			vision_err("qbuf, get_vs4l_container32 is fail(%ld)\n", ret);
			goto p_err;
		}

		ret = ops->vertexioc_unprepare(file, &karg.vscl);
		put_vs4l_container32(&karg.vscl, up);
		break;
	default:
		vision_err("%x iocontrol is not supported(%lx, %zd)\n", cmd, VS4L_VERTEXIOC_S_FORMAT, sizeof(struct vs4l_format_list));
		break;
	}

p_err:
	vision_info("@@ ioctl32(%u) usr arg: (%lx) Return code (%ld/0x%lx)\n", cmd, arg, ret, ret);
	return ret;
}
