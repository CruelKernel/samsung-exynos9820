/* sound/soc/samsung/vts/vts_dump.c
 *
 * ALSA SoC - Samsung VTS dump driver
 *
 * Copyright (c) 2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
//#define DEBUG
#include <linux/debugfs.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <sound/soc.h>

#include "vts_dump.h"

#define S_IRWUG (S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP)

struct vts_dump_info {
	struct device *dev;
	struct mutex audiolock;
	struct mutex loglock;
	u32 audioaddr_offset;
	u32 audiodump_sz;
	u32 logaddr_offset;
	u32 logdump_sz;
};

static struct vts_dump_info gdump_info;

static void vts_audiodump_flush_work_func(struct work_struct *work)
{
	struct device *dev = gdump_info.dev;
	struct vts_dump_info *dump_info = &gdump_info;
	struct vts_data *data = dev_get_drvdata(dev);
	char filename[SZ_64];
	struct file *filp;

	dev_dbg(dev, "%s: SRAM Offset: 0x%x Size: %d\n", __func__,
		dump_info->audioaddr_offset, dump_info->audiodump_sz);

	mutex_lock(&gdump_info.audiolock);
	sprintf(filename, "/data/vts_audiodump.raw");

	filp = filp_open(filename, O_RDWR | O_APPEND |
				O_CREAT, S_IRUSR | S_IWUSR);
	dev_info(dev, "AudioDump appended mode\n");
	if (!IS_ERR_OR_NULL(filp)) {
		void *area = (void *)(data->sram_base +
					dump_info->audioaddr_offset);
		size_t bytes = (size_t)dump_info->audiodump_sz;

		/* dev_dbg(dev, " %p, %zx\n", area, bytes); */
		kernel_write(filp, area, bytes, &filp->f_pos);
		dev_dbg(dev, "kernel_write %p, %zx\n", area, bytes);

		vfs_fsync(filp, 1);
		filp_close(filp, NULL);
	} else {
		dev_err(dev, "VTS Audiodump file [%s] open error: %ld\n",
			filename, PTR_ERR(filp));
	}

	vts_send_ipc_ack(data, 1);
	mutex_unlock(&gdump_info.audiolock);
}

static void vts_logdump_flush_work_func(struct work_struct *work)
{
	struct device *dev = gdump_info.dev;
	struct vts_dump_info *dump_info = &gdump_info;
	struct vts_data *data = dev_get_drvdata(dev);
	char filename[SZ_64];
	struct file *filp;

	dev_dbg(dev, "%s: SRAM Offset: 0x%x Size: %d\n", __func__,
		dump_info->logaddr_offset, dump_info->logdump_sz);

	mutex_lock(&gdump_info.loglock);
	sprintf(filename, "/data/vts_logdump.txt");

	filp = filp_open(filename, O_RDWR | O_APPEND |
				O_CREAT, S_IRUSR | S_IWUSR);
	dev_info(dev, "LogDump appended mode\n");
	if (!IS_ERR_OR_NULL(filp)) {
		void *area = (void *)(data->sram_base +
					dump_info->logaddr_offset);
		size_t bytes = (size_t)dump_info->logdump_sz;

		/* dev_dbg(dev, " %p, %zx\n", area, bytes); */
		kernel_write(filp, area, bytes, &filp->f_pos);
		dev_dbg(dev, "kernel_write %p, %zx\n", area, bytes);

		vfs_fsync(filp, 1);
		filp_close(filp, NULL);
	} else {
		dev_err(dev, "VTS Logdump file [%s] open error: %ld\n",
			filename, PTR_ERR(filp));
	}

	vts_send_ipc_ack(data, 1);
	mutex_unlock(&gdump_info.loglock);
}

static DECLARE_WORK(vts_audiodump_work, vts_audiodump_flush_work_func);
static DECLARE_WORK(vts_logdump_work, vts_logdump_flush_work_func);
void vts_audiodump_schedule_flush(struct device *dev)
{
	struct vts_data *data = dev_get_drvdata(dev);

	if (gdump_info.audioaddr_offset) {
		schedule_work(&vts_audiodump_work);
		dev_dbg(dev, "%s: AudioDump Scheduled\n", __func__);
	} else {
		dev_warn(dev, "%s: AudioDump address not registered\n",
				__func__);
		/* send ipc ack to unblock vts firmware */
		vts_send_ipc_ack(data, 1);
	}
}
EXPORT_SYMBOL(vts_audiodump_schedule_flush);

void vts_logdump_schedule_flush(struct device *dev)
{
	struct vts_data *data = dev_get_drvdata(dev);

	if (gdump_info.logaddr_offset) {
		schedule_work(&vts_logdump_work);
		dev_dbg(dev, "%s: LogDump Scheduled\n", __func__);
	} else {
		dev_warn(dev, "%s: LogDump address not registered\n",
				__func__);
		/* send ipc ack to unblock vts firmware */
		vts_send_ipc_ack(data, 1);
	}
}
EXPORT_SYMBOL(vts_logdump_schedule_flush);

void vts_dump_addr_register(
	struct device *dev,
	u32 addroffset,
	u32 dumpsz,
	u32 dump_mode)
{
	struct vts_data *data = dev_get_drvdata(dev);

	gdump_info.dev = dev;
	if ((addroffset + dumpsz) > data->sram_size) {
		dev_warn(dev, "%s: wrong offset[0x%x] or size[0x%x]\n",
				__func__, addroffset, dumpsz);
		return;
	}

	if (dump_mode == VTS_AUDIO_DUMP) {
		gdump_info.audioaddr_offset = addroffset;
		gdump_info.audiodump_sz = dumpsz;
	} else if (dump_mode == VTS_LOG_DUMP) {
		gdump_info.logaddr_offset = addroffset;
		gdump_info.logdump_sz = dumpsz;
	} else
		dev_warn(dev, "%s: Unknown dump mode\n", __func__);

	dev_info(dev, "%s: %sDump offset[0x%x] size [%d]Scheduled\n",
			__func__, (dump_mode ? "Log" : "Audio"),
			addroffset, dumpsz);
}
EXPORT_SYMBOL(vts_dump_addr_register);

static int __init samsung_vts_dump_late_initcall(void)
{
	pr_info("%s\n", __func__);
	mutex_init(&gdump_info.audiolock);
	mutex_init(&gdump_info.loglock);
	gdump_info.audioaddr_offset = 0;
	gdump_info.audiodump_sz = 0;
	gdump_info.logaddr_offset = 0;
	gdump_info.logdump_sz = 0;

	return 0;
}
late_initcall(samsung_vts_dump_late_initcall);
