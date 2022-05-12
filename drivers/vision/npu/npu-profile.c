/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <asm/cacheflush.h>
#include "npu-device.h"
#include "npu-common.h"
#include "npu-memory.h"
#include "npu-util-statekeeper.h"
#include "npu-if-session-protodrv.h"
#include "npu-system.h"
#include "npu-profile.h"
#include "npu-log.h"
#include "npu-debug.h"

#define SIMPLE_PROFILER

enum npu_golden_state_e {
	NPU_PROFILE_STATE_NOT_INITIALIZED,
	NPU_PROFILE_STATE_PWRON_WAITING,
	NPU_PROFILE_STATE_STARTING,
	NPU_PROFILE_STATE_GATHERING,
	NPU_PROFILE_STATE_STOPPING,
	NPU_PROFILE_STATE_EXPORT_READY,
	NPU_PROFILE_STATE_INVALID,
};

static const char *state_names[NPU_PROFILE_STATE_INVALID + 1]
	= {"NOT INITIALIZED", "PWRON_WAITING", "STARTING", "GATHERING", "STOPPING", "EXPORT_READY", "INVALID"};

static const u8 transition_map[][STATE_KEEPER_MAX_STATE] = {
/* From - To			NOT INITIALIZED	PWRON_WAITING	STARTING	GATHERING	STOPPING	EXPORT_READY	INVALID */
/* NOT INITIALIZED      */  {	1,		1,		1,		0,		0,		0,		0},
/* PWRON_WAITING       */  {	1,		1,		1,		0,		1,		0,		0},
/* STARTING            */  {	1,		0,		0,		1,		0,		0,		0},
/* GATHERING           */  {	1,		0,		0,		1,		1,		0,		0},
/* STOPPING            */  {	1,		0,		0,		0,		0,		1,		0},
/* EXPORT_READY        */  {	1,		1,		1,		0,		0,		1,		0},
/* INVALID             */  {	0,		0,		0,		0,		0,		0,		0},
};

static const char *ERR_STR_NO_PROFILE_AVAILABLE = "No profile data available.\n";

/* Convenient reference to profile control */
struct npu_profile_control *profile_ctl_ref;

#define GET_SYSTEM(pt_profile_ctl)		container_of(pt_profile_ctl, struct npu_system, profile_ctl)


struct read_pos {
	struct npu_profile_control	*profile_ctl;
	const char			*data;
	size_t				size;
	size_t				rpos;
	char				in_buf[32];
};

/* Global configuration for simple profiler */
volatile struct {
	int		enabled;
	int		sysfs_ok;
	struct device	*dev;
} s_profiler_ctl;

/* Set initial value of profile_data */
static void init_profile_data(struct npu_profile *profile_data)
{
	struct probe_meta *meta;

	BUG_ON(!profile_data);
	meta = profile_data->meta;

	/* Clear with 0 */
	memset(profile_data, 0, sizeof(*profile_data));
	profile_data->point_cnt = NPU_PROFILE_POINT_NUM;

	/* Set profile section */
	meta[0].profile_unit_id = PROFILE_UNIT_FIRMWARE;	/* Firmware */
	meta[0].start_idx = 0;
	meta[0].total_count = NPU_PROFILE_POINT_NUM - 2048;
	meta[0].valid_count = 0;

	meta[1].profile_unit_id = PROFILE_UNIT_DEVICE_DRIVER;	/* Driver */
	meta[1].start_idx = meta[0].total_count;
	meta[1].total_count = 2048;
	meta[1].valid_count = 0;

	/* Cache flush */
	__dma_map_area(profile_data, sizeof(*profile_data), DMA_TO_DEVICE);
}

/* Allocate profile_data */
static int alloc_profile_data(struct npu_profile_control *profile_ctl)
{
	int ret;
	struct npu_system *system;

	BUG_ON(!profile_ctl);

	system = GET_SYSTEM(profile_ctl);
	BUG_ON(!system);

	if (profile_ctl->buf.size > 0)
		npu_warn("profile data was not cleared. memory leak might be happened.\n");

	/* TODO: Size dynamic adjustment might be necessary */
	profile_ctl->buf.size = sizeof(*(profile_ctl->profile_data));
	ret = npu_memory_alloc(&system->memory, &profile_ctl->buf);
	if (ret) {
		npu_err("npu_memory_alloc for profile buffer memory, ret(%d)\n", ret);
		goto err_exit;
	}
	npu_info("profiling buffer allocated, size(%lu) / dv(%pad) / kv(%pK)",
		profile_ctl->buf.size, &profile_ctl->buf.daddr, profile_ctl->buf.vaddr);

	/* Save pointer */
	profile_ctl->profile_data = profile_ctl->buf.vaddr;

	/* Initialize profile data */
	init_profile_data(profile_ctl->profile_data);
	ret = 0;

err_exit:
	return ret;
}

/* Deallocate profile data */
static void dealloc_profile_data(struct npu_profile_control *profile_ctl)
{
	struct npu_system *system;

	BUG_ON(!profile_ctl);

	system = GET_SYSTEM(profile_ctl);
	BUG_ON(!system);

	if (profile_ctl->buf.size == 0) {
		npu_warn("profile data was not allocated. skipping deallocation.\n");
		return;
	}
	npu_memory_free(&system->memory, &profile_ctl->buf);
	npu_info("profiling buffer deallocated.\n");

	/* Clear pointer */
	profile_ctl->profile_data = NULL;
	profile_ctl->buf.size = 0;
}

/* Call-back from Protodrv */
static int profile_save_result(struct npu_session *dummy, struct nw_result result)
{
	BUG_ON(!profile_ctl_ref);
	npu_trace("Profiling request completed : result = %u\n", result.result_code);

	profile_ctl_ref->result_code = result.result_code;
	atomic_set(&profile_ctl_ref->result_available, 1);

	wake_up_all(&profile_ctl_ref->wq);
	return 0;
}

/* Send profiling request (start or stop) to HW and check its response */
static int request_profile_ctl_to_hw(struct npu_profile_control *profile_ctl,
	nw_cmd_e cmd, struct npu_memory_buffer *buf_handle)
{
	int ret;
	struct npu_nw nw;
	int retry_cnt;

	memset(&nw, 0, sizeof(nw));
	nw.cmd = cmd;

	/* Set callback function on completion */
	nw.notify_func = profile_save_result;

	switch (cmd) {
	case NPU_NW_CMD_PROFILE_START:
		if (buf_handle) {
			nw.ncp_addr.av_index = 1;
			nw.uid = PROFILE_UID;
			nw.ncp_addr.vaddr = buf_handle->vaddr;
			nw.ncp_addr.daddr = buf_handle->daddr;
			nw.ncp_addr.size = (u32)buf_handle->size;
			npu_dbg("prepare profile start command: cmd(%u) v(%pK) d(%pad) size(%zu)\n",
				nw.cmd, nw.ncp_addr.vaddr, &nw.ncp_addr.daddr, nw.ncp_addr.size);
		} else {
			npu_err("buf_handle is null on profile start command.\n");
			ret = -EINVAL;
			goto err_exit;
		}
		break;

	case NPU_NW_CMD_PROFILE_STOP:
		npu_dbg("prepare profile stop command: cmd(%u)\n", nw.cmd);
		break;
	default:
		npu_err("invalid cmd(%d)\n", cmd);
		ret = -EINVAL;
		goto err_exit;
	}

	retry_cnt = 0;
	atomic_set(&profile_ctl->result_available, 0);
	while ((ret = npu_ncp_mgmt_put(&nw)) <= 0) {
		npu_dbg("queue full when insearting profile control message. Retrying...");
		if (retry_cnt++ >= PROFILE_CMD_POST_RETRY_CNT) {
			npu_err("timeout exceeded.\n");
			ret = -EWOULDBLOCK;
			goto err_exit;
		}
		msleep(PROFILE_CMD_POST_RETRY_INTERVAL);
	}
	/* Success */
	npu_dbg("profile control message has posted\n");

	ret = wait_event_timeout(
		profile_ctl->wq,
		atomic_read(&profile_ctl->result_available),
		PROFILE_HW_RESP_TIMEOUT);
	if (ret < 0) {
		npu_err("wait_event_timeout error(%d)\n", ret);
		goto err_exit;
	}
	if (!atomic_read(&profile_ctl->result_available)) {
		npu_err("timeout waiting H/W response\n");
		ret = -ETIMEDOUT;
		goto err_exit;
	}
	if (profile_ctl->result_code != NPU_ERR_NO_ERROR) {
		npu_err("hardware reply with NDONE(%d)\n", profile_ctl->result_code);
		ret = -EFAULT;
		goto err_exit;
	}
	ret = 0;

err_exit:
	return ret;
}

static void npu_profile_clear(struct npu_profile_control *profile_ctl)
{
	BUG_ON(!profile_ctl);

	npu_dbg("start in npu_profile_clear\n");

	dealloc_profile_data(profile_ctl);

	npu_statekeeper_transition(&profile_ctl->statekeeper, NPU_PROFILE_STATE_NOT_INITIALIZED);

	npu_dbg("complete in npu_profile_clear.\n");
}

static int npu_profile_stop(struct npu_profile_control *profile_ctl)
{
	int ret;
	struct npu_profile *profile_data;
	struct probe_meta *fw_probe_meta;
	void *fw_probe_start;
	size_t	fw_probe_size;

	BUG_ON(!profile_ctl);

	npu_dbg("start in npu_profile_stop.\n");

	profile_data = profile_ctl->profile_data;

	if (SKEEPER_COMPARE_STATE(&profile_ctl->statekeeper, NPU_PROFILE_STATE_PWRON_WAITING)) {
		/* Noting need to be done but just change state */
		npu_info("postponed starting is canceled by stop request.\n");
		npu_statekeeper_transition(&profile_ctl->statekeeper, NPU_PROFILE_STATE_NOT_INITIALIZED);
		ret = 0;
		goto ok_exit;
	}
	/* Start stopping */
	npu_statekeeper_transition(&profile_ctl->statekeeper, NPU_PROFILE_STATE_STOPPING);

	/* Send stop request to firmware */
	npu_dbg("sending NPU_NW_CMD_PROFILE_STOP to NPU H/W.\n");
	ret = request_profile_ctl_to_hw(profile_ctl, NPU_NW_CMD_PROFILE_STOP, NULL);
	if (ret) {
		npu_err("fail(%d) in request_profile_ctl_to_hw\n", ret);
		goto err_exit;
	}

	/* Invalidate cache of firmware area */
	fw_probe_meta = &profile_data->meta[PROFILE_UNIT_FIRMWARE];
	__dma_unmap_area(fw_probe_meta, sizeof(*fw_probe_meta), DMA_FROM_DEVICE);
	npu_dbg("fw profile id(%u), start(%u) total(%u) valid(%u)\n",
		fw_probe_meta->profile_unit_id, fw_probe_meta->start_idx,
		fw_probe_meta->total_count, fw_probe_meta->valid_count);

	fw_probe_start = &profile_data->point[fw_probe_meta->start_idx];
	fw_probe_size = sizeof(struct probe_point) * fw_probe_meta->total_count;
	npu_dbg("fw profile invalidate addr(%pK), size(%ld)\n",
		fw_probe_start, fw_probe_size);
	__dma_unmap_area(fw_probe_start, fw_probe_size, DMA_FROM_DEVICE);

	/* Save index on meta */
	profile_data->meta[PROFILE_UNIT_DEVICE_DRIVER].valid_count = atomic_read(&profile_ctl->next_probe_point_idx);

	/* Transition to data ready state */
	npu_statekeeper_transition(&profile_ctl->statekeeper, NPU_PROFILE_STATE_EXPORT_READY);
	ret = 0;
	goto ok_exit;

err_exit:
	/* Stop failed */
	npu_err("stopping failed. clean-up profile data.\n");
	npu_profile_clear(profile_ctl);

ok_exit:
	npu_dbg("complete in npu_profile_stop\n");
	return ret;
}

static int npu_profile_start(struct npu_profile_control *profile_ctl)
{
	int ret;
	int startidx;
	struct npu_profile *profile_data;

	BUG_ON(!profile_ctl);

	npu_dbg("start in npu_profile_start\n");

	if (SKEEPER_COMPARE_STATE(&profile_ctl->statekeeper, NPU_PROFILE_STATE_EXPORT_READY)) {
		npu_info("profiling data is remaining. cleaning it first.");
		npu_profile_clear(profile_ctl);
	}
	if (!npu_if_session_protodrv_is_opened()) {
		/* NPU is not working now. Delay initialization until it is opened */
		npu_info("not opened in NPU\n");
		npu_statekeeper_transition(&profile_ctl->statekeeper, NPU_PROFILE_STATE_PWRON_WAITING);
		ret = 0;
		goto ok_exit;
	}

	npu_statekeeper_transition(&profile_ctl->statekeeper, NPU_PROFILE_STATE_STARTING);

	ret = alloc_profile_data(profile_ctl);
	if (ret) {
		npu_err("fail(%d) in alloc_profile_data", ret);
		goto err_exit;
	}

	profile_data = profile_ctl->profile_data;
	startidx = profile_data->meta[PROFILE_UNIT_DEVICE_DRIVER].start_idx;
	atomic_set(&profile_ctl->next_probe_point_idx, 0);
	npu_dbg("initializing startidx (%d), writing from index (%d)\n",
		atomic_read(&profile_ctl->next_probe_point_idx), startidx);

	npu_dbg("sending NPU_NW_CMD_PROFILE_START to NPU H/W.\n");
	ret = request_profile_ctl_to_hw(profile_ctl, NPU_NW_CMD_PROFILE_START, &profile_ctl->buf);
	if (ret) {
		npu_err("fail(%d) in request_profile_ctl_to_hw\n", ret);
		goto err_exit;
	}

	npu_statekeeper_transition(&profile_ctl->statekeeper, NPU_PROFILE_STATE_GATHERING);
	ret = 0;
	goto ok_exit;

err_exit:
	npu_dbg("error occurred, deaalocated profiling data\n");
	dealloc_profile_data(profile_ctl);
	npu_statekeeper_transition(&profile_ctl->statekeeper, NPU_PROFILE_STATE_NOT_INITIALIZED);
ok_exit:
	npu_dbg("complete in npu_profile_start\n");
	return ret;
}

/**************************************************************************
 * fops definitions
 */
static int npu_profile_ctl_open(struct inode *inode, struct file *file)
{
	int ret;
	struct read_pos *rp;
	struct npu_profile_control *profile_ctl = inode->i_private;
	const char *state_name;

	npu_dbg("start in npu_profile_ctl_open\n");
	BUG_ON(!profile_ctl);

	mutex_lock(&profile_ctl->lock);
	rp = kzalloc(sizeof(*rp), GFP_KERNEL);
	if (!rp) {
		npu_err("fail in struct read_pos. allocation\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	state_name = npu_statekeeper_state_name(&profile_ctl->statekeeper);
	rp->size = scnprintf(rp->in_buf, sizeof(rp->in_buf), "%s\n", state_name);
	rp->data = rp->in_buf;
	rp->rpos = 0;
	rp->profile_ctl = profile_ctl;

	file->private_data = rp;
	ret = 0;

err_exit:
	npu_dbg("complete in npu_profile_ctl_open.\n");
	mutex_unlock(&profile_ctl->lock);
	return ret;
}

static int npu_profile_ctl_close(struct inode *inode, struct file *file)
{
	struct read_pos *rp = file->private_data;
	struct npu_profile_control *profile_ctl;

	npu_dbg("start in npu_profile_ctl_close\n");

	BUG_ON(!rp);
	profile_ctl = rp->profile_ctl;
	BUG_ON(!profile_ctl);

	mutex_lock(&profile_ctl->lock);
	kfree(rp);
	mutex_unlock(&profile_ctl->lock);

	npu_dbg("completed in npu_profile_ctl_close\n");
	return 0;
}

static ssize_t npu_profile_ctl_read(
	struct file *file,
	char __user *outbuf,
	size_t outlen, loff_t *loff)
{
	int ret;
	size_t copy_len;
	struct read_pos *rp = file->private_data;
	struct npu_profile_control *profile_ctl;

	BUG_ON(!rp);
	profile_ctl = rp->profile_ctl;
	BUG_ON(!profile_ctl);

	npu_dbg("start in npu_profile_ctl_read\n");
	mutex_lock(&profile_ctl->lock);

	if (rp->rpos >= rp->size) {
		ret = 0;
	} else {
		copy_len = min(outlen, rp->size - rp->rpos);
		npu_trace("Copy [%lu] bytes.", copy_len);
		if (copy_to_user(outbuf, &rp->data[rp->rpos], copy_len) != 0) {
			npu_err("copy_to_user failed.\n");
			ret = -EFAULT;
		} else {
			rp->rpos += copy_len;
			ret = copy_len;
		}
	}

	npu_dbg("complete(%d) in npu_profile_ctl_read\n", ret);
	mutex_unlock(&profile_ctl->lock);
	return ret;
}

static ssize_t npu_profile_ctl_write(
	struct file *file,
	const char *__user userbuf,
	size_t count, loff_t *offp)
{
	int ret;
	struct read_pos *rp = file->private_data;
	struct npu_profile_control *profile_ctl;
	char c;

	BUG_ON(!rp);
	profile_ctl = rp->profile_ctl;
	BUG_ON(!profile_ctl);

	npu_dbg("start in npu_profile_ctl_write\n");
	mutex_lock(&profile_ctl->lock);

	if (count <= 0) {
		ret = 0;
	} else {
		ret = get_user(c, userbuf);
		if (ret) {
			npu_err("fail(%d) in get_user\n", ret);
		} else {
			switch (c) {
			case '0':
				/* Stop */
				if (SKEEPER_IS_TRANSITABLE(&profile_ctl->statekeeper, NPU_PROFILE_STATE_STOPPING))
					ret = npu_profile_stop(profile_ctl);
				break;
			case '1':
				/* Start */
				if (SKEEPER_IS_TRANSITABLE(&profile_ctl->statekeeper, NPU_PROFILE_STATE_STARTING))
					ret = npu_profile_start(profile_ctl);
				break;
			default:
				npu_err("invalid character (%c)\n", c);
				ret = -EINVAL;
				break;
			}
		}
	}

	npu_dbg("complete(%d) in npu_profile_ctl_write\n", ret);
	mutex_unlock(&profile_ctl->lock);

	if (ret == 0)
		/* No error -> return number of character */
		return count;
	else
		return ret;
}

static int npu_profile_result_open(struct inode *inode, struct file *file)
{
	int ret;
	struct read_pos *rp;
	struct npu_profile_control *profile_ctl = inode->i_private;

	npu_dbg("start in npu_profile_result_open\n");
	BUG_ON(!profile_ctl);

	mutex_lock(&profile_ctl->lock);
	rp = kzalloc(sizeof(*rp), GFP_KERNEL);
	if (!rp) {
		npu_err("fail in struct read_pos. allocation\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	if (SKEEPER_COMPARE_STATE(&profile_ctl->statekeeper, NPU_PROFILE_STATE_EXPORT_READY)) {
		/* Result is available */
		rp->data = (char *)profile_ctl->profile_data;
		rp->size = sizeof(struct npu_profile);
	} else {
		/* Test data is not ready */
		npu_dbg("not ready in profiling data\n");
		rp->data = ERR_STR_NO_PROFILE_AVAILABLE;
		rp->size = strlen(rp->data);
	}
	rp->rpos = 0;
	rp->profile_ctl = profile_ctl;

	file->private_data = rp;
	ret = 0;

err_exit:
	npu_dbg("complete in npu_profile_result_open\n");
	mutex_unlock(&profile_ctl->lock);
	return ret;
}

static int npu_profile_result_close(struct inode *inode, struct file *file)
{
	struct read_pos *rp = file->private_data;
	struct npu_profile_control *profile_ctl;

	npu_dbg("start in npu_profile_result_close\n");

	BUG_ON(!rp);
	profile_ctl = rp->profile_ctl;
	BUG_ON(!profile_ctl);

	mutex_lock(&profile_ctl->lock);
	kfree(rp);
	mutex_unlock(&profile_ctl->lock);

	npu_dbg("complete in npu_profile_result_close\n");
	return 0;
}

static ssize_t npu_profile_result_read(struct file *file, char __user *outbuf, size_t outlen, loff_t *loff)
{
	int ret;
	size_t copy_len;
	struct read_pos *rp = file->private_data;
	struct npu_profile_control *profile_ctl;

	BUG_ON(!rp);
	profile_ctl = rp->profile_ctl;
	BUG_ON(!profile_ctl);

	npu_dbg("start in npu_profile_result_read\n");
	mutex_lock(&profile_ctl->lock);
	if ((rp->data != ERR_STR_NO_PROFILE_AVAILABLE)
	    && !SKEEPER_COMPARE_STATE(&profile_ctl->statekeeper, NPU_PROFILE_STATE_EXPORT_READY)) {
		npu_warn("profiling data had gone while reading data.\n");
		ret = 0;
	} else if (rp->rpos >= rp->size) {
		ret = 0;
	} else {
		copy_len = min(outlen, rp->size - rp->rpos);
		npu_trace("copy (%lu) bytes.", copy_len);
		if (copy_to_user(outbuf, &rp->data[rp->rpos], copy_len) != 0) {
			npu_err("faile in copy_to_user\n");
			ret = -EFAULT;
		} else {
			rp->rpos += copy_len;
			ret = copy_len;
		}
	}

	npu_dbg("complete(%d) in npu_profile_result_read\n", ret);
	mutex_unlock(&profile_ctl->lock);
	return ret;
}

static const struct file_operations profile_ctl_fops = {
	.owner = THIS_MODULE,
	.open = npu_profile_ctl_open,
	.release = npu_profile_ctl_close,
	.read = npu_profile_ctl_read,
	.write = npu_profile_ctl_write,
};

static const struct file_operations profile_result_fops = {
	.owner = THIS_MODULE,
	.open = npu_profile_result_open,
	.release = npu_profile_result_close,
	.read = npu_profile_result_read,
};


/**************************************************************************
 * simple profiler sysfs control interface
 */
#define S_PROFILE_PREFIX	"S-PROFILE: "

static ssize_t s_profiler_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;

	ret = scnprintf(buf, PAGE_SIZE,
		"Simple profiler state : %s\n"
		" - echo 0 > s_profiler : Disable simple profiler\n"
		"   echo 1 > s_profiler : Enable simple profiler\n",
		(s_profiler_ctl.enabled) ? "Enabled" : "Disabled");
	return ret;
}

static ssize_t s_profiler_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	if (len <= 0)
		return 0;

	switch (buf[0]) {
	case '0':
		npu_info(S_PROFILE_PREFIX "Simple profiler disabled\n");
		s_profiler_ctl.enabled = 0;
		break;
	case '1':
		npu_info(S_PROFILE_PREFIX "Simple profiler enabled\n");
		s_profiler_ctl.enabled = 1;
		break;
	default:
		npu_info(S_PROFILE_PREFIX "Invalid input [%c]\n", buf[0]);
		break;
	}

	return len;
}

static inline int is_s_profile_enabled(void)
{
	return (s_profiler_ctl.enabled == 1);
}

/*
 * sysfs attribute for simple profiler
 *   name = dev_attr_s_profiler
 *   show = s_profiler_show
 *   store = s_profiler_store
 */
static DEVICE_ATTR_RW(s_profiler);

/**************************************************************************
 * npu_profile driver lifecycle management
 */
int npu_profile_probe(struct npu_system *system)
{
	int ret = 0;
	struct npu_profile_control	*profile_ctl;
	struct npu_device		*npu_device;
	struct device			*dev;

	BUG_ON(!system);
	npu_device = container_of(system, struct npu_device, system);
	dev = npu_device->dev;
	BUG_ON(!dev);
	probe_trace("start in npu_profile_probe\n");

	/* Register simple profiler sysfs entry */
	memset((void *)&s_profiler_ctl, 0, sizeof(s_profiler_ctl));
	s_profiler_ctl.dev = dev;
	ret = sysfs_create_file(&dev->kobj, &dev_attr_s_profiler.attr);
	if (ret) {
		probe_err("sysfs_create_file error : ret = %d\n", ret);
		s_profiler_ctl.sysfs_ok = 0;
		goto err_exit;
	} else {
		s_profiler_ctl.sysfs_ok = 1;
	}

	/* Initialize profile_ctl */
	profile_ctl = &system->profile_ctl;
	memset(profile_ctl, 0, sizeof(*profile_ctl));
	mutex_init(&profile_ctl->lock);

	/* Save PWM register address */
	profile_ctl->pwm = system->pwm_npu.vaddr + TCNTO0_OFF;
	probe_info("PWM timer address = %pK\n", profile_ctl->pwm);

	/* Initialize state keeper */
	npu_statekeeper_initialize(
		&profile_ctl->statekeeper,
		NPU_PROFILE_STATE_INVALID,
		state_names, transition_map);

	/* Initialized debugfs interface */
	ret = npu_debug_register_arg("profile-control", profile_ctl, &profile_ctl_fops);
	if (ret) {
		probe_err("fail(%d) in npu_debug_register_arg(profile-control)\n", ret);
		goto err_exit;
	}

	ret = npu_debug_register_arg("profile-result", profile_ctl, &profile_result_fops);
	if (ret) {
		probe_err("fail(%d) in npu_debug_register_arg(profile-result)\n", ret);
		goto err_exit;
	}

	init_waitqueue_head(&profile_ctl->wq);

	/* Save reference in global variable for future use */
	profile_ctl_ref = profile_ctl;

	probe_info("complete in npu_profile_probe\n");

err_exit:
	return ret;
}

int npu_profile_open(struct npu_system *system)
{
	int ret;
	struct npu_profile_control *profile_ctl;

	BUG_ON(!system);
	npu_dbg("start in npu_profile_open\n");
	profile_ctl = &system->profile_ctl;
	mutex_lock(&profile_ctl->lock);

	if (SKEEPER_COMPARE_STATE(&profile_ctl->statekeeper, NPU_PROFILE_STATE_PWRON_WAITING)) {
		/* Continue postponed initialization */
		npu_dbg("initiating delayed starting procedure.\n");
		ret = npu_profile_start(profile_ctl);
		if (ret) {
			npu_err("fail(%d) in npu_profile_start\n", ret);
			goto err_exit;
		}
	}
	ret = 0;
err_exit:
	mutex_unlock(&profile_ctl->lock);

	npu_dbg("complete in npu_profile_open\n");
	return ret;
}

int npu_profile_close(struct npu_system *system)
{
	int ret;
	struct npu_profile_control *profile_ctl;

	BUG_ON(!system);
	npu_dbg("start in npu_profile_close\n");
	profile_ctl = &system->profile_ctl;
	mutex_lock(&profile_ctl->lock);

	if (SKEEPER_COMPARE_STATE(&profile_ctl->statekeeper, NPU_PROFILE_STATE_GATHERING)) {
		/* Stop current profiling session if gathering is in progress */
		npu_dbg("stopping current profiling session.\n");
		ret = npu_profile_stop(profile_ctl);
		if (ret) {
			npu_err("fail(%d) in npu_profile_stop\n", ret);
			goto err_exit;
		}
	}
	ret = 0;
err_exit:
	mutex_unlock(&profile_ctl->lock);

	npu_dbg("complete in npu_profile_close\n");
	return 0;
}

int npu_profile_release(void)
{
	int ret = 0;
	struct npu_profile_control *profile_ctl = profile_ctl_ref;

	BUG_ON(!profile_ctl);

	npu_dbg("start in npu_profile_release\n");

	if (s_profiler_ctl.sysfs_ok) {
		sysfs_remove_file(&s_profiler_ctl.dev->kobj, &dev_attr_s_profiler.attr);
		s_profiler_ctl.sysfs_ok = 0;
	}

	mutex_lock(&profile_ctl->lock);

	if (SKEEPER_COMPARE_STATE(&profile_ctl->statekeeper, NPU_PROFILE_STATE_GATHERING)) {
		npu_info("profiling is GATHERING state. stopping it first.");
		ret = npu_profile_stop(profile_ctl);
		if (ret) {
			mutex_unlock(&profile_ctl->lock);
			npu_err("fail(%d) in npu_profile_stop\n", ret);
			goto err_exit;
		}
	}

	if (SKEEPER_COMPARE_STATE(&profile_ctl->statekeeper, NPU_PROFILE_STATE_EXPORT_READY)) {
		npu_info("profiling data is remaining. cleaning it first.");
		npu_profile_clear(profile_ctl);
	}

	if (SKEEPER_EXPECT_STATE(&profile_ctl->statekeeper, NPU_PROFILE_STATE_NOT_INITIALIZED))
		BUG_ON(1);

	profile_ctl_ref = NULL;

	mutex_unlock(&profile_ctl->lock);

	npu_dbg("Completed.\n");

err_exit:
	return ret;
}

static inline void __profile_point(
	const u32 point_id, const u32 uid, const u32 frame_id,
	const u32 p0, const u32 p1, const u32 p2)
{
	struct probe_point *pt;
	int write_idx;
	struct timeval tv;

	/* Check for simple profiler */
	if (is_s_profile_enabled()) {
		do_gettimeofday(&tv);
		npu_dbg(S_PROFILE_PREFIX "[%08lu]s[%06lu]us id:%u uid:%u fid:%u p[] = [%u %u %u]\n",
			tv.tv_sec, tv.tv_usec,
			point_id, uid, frame_id, p0, p1, p2);
	}

	BUG_ON(!profile_ctl_ref);

	if (!SKEEPER_COMPARE_STATE(&profile_ctl_ref->statekeeper, NPU_PROFILE_STATE_GATHERING)) {
		/* Profiling is not startted */
		return;
	}

	BUG_ON(!profile_ctl_ref->profile_data);

	write_idx = atomic_inc_return(&profile_ctl_ref->next_probe_point_idx);
	if (write_idx >= profile_ctl_ref->profile_data->meta[PROFILE_UNIT_DEVICE_DRIVER].total_count) {
		npu_dbg("profiling buffer is full(try:%d / capacity:%u). ignore profiling data.\n",
			write_idx, profile_ctl_ref->profile_data->meta[PROFILE_UNIT_DEVICE_DRIVER].total_count);
		return;
	}
	write_idx += profile_ctl_ref->profile_data->meta[PROFILE_UNIT_DEVICE_DRIVER].start_idx;
	if (write_idx >= profile_ctl_ref->profile_data->point_cnt) {
		npu_dbg("invalid write_idx(%u). point_cnt(%u)\n",
			write_idx, profile_ctl_ref->profile_data->point_cnt);
		return;
	}

	pt = &(profile_ctl_ref->profile_data->point[write_idx]);
	npu_dbg("profiling data is written to (%pK), index(%d)\n", pt, write_idx);

	pt->id = point_id;
	pt->timestamp = readl(profile_ctl_ref->pwm);
	pt->session_id = uid;
	pt->frame_id = frame_id;
	pt->param[0] = p0;
	pt->param[1] = p1;
	pt->param[2] = p2;
}

void profile_point1(const u32 point_id, const u32 uid, const u32 frame_id, const u32 p0)
{
	__profile_point(point_id, uid, frame_id, p0, 0, 0);
}

void profile_point3(
	const u32 point_id, const u32 uid, const u32 frame_id,
	const u32 p0, const u32 p1, const u32 p2)

{
	__profile_point(point_id, uid, frame_id, p0, p1, p2);
}
