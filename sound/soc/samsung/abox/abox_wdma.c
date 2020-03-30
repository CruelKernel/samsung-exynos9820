/* sound/soc/samsung/abox/abox_wdma.c
 *
 * ALSA SoC Audio Layer - Samsung Abox WDMA driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* #define DEBUG */
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/iommu.h>
#include <linux/delay.h>
#include <linux/memblock.h>
#include <linux/sched/clock.h>
#include <sound/hwdep.h>
#include <linux/miscdevice.h>

#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/sounddev_abox.h>

#include <linux/dma-buf.h>
#include <linux/dma-buf-container.h>
#include <linux/ion_exynos.h>
#include "../../../../drivers/iommu/exynos-iommu.h"
#include "../../../../drivers/staging/android/uapi/ion.h"
#include <sound/samsung/abox.h>
#include "abox_util.h"
#include "abox_gic.h"
#include "abox_dbg.h"
#include "abox_vss.h"
#include "abox_cmpnt.h"
#include "abox_mmapfd.h"
#include "abox.h"

static const struct snd_pcm_hardware abox_wdma_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED
				| SNDRV_PCM_INFO_BLOCK_TRANSFER
				| SNDRV_PCM_INFO_MMAP
				| SNDRV_PCM_INFO_MMAP_VALID,
	.formats		= ABOX_WDMA_SAMPLE_FORMATS,
	.channels_min		= 1,
	.channels_max		= 8,
	.buffer_bytes_max	= BUFFER_BYTES_MAX,
	.period_bytes_min	= PERIOD_BYTES_MIN,
	.period_bytes_max	= PERIOD_BYTES_MAX,
	.periods_min		= BUFFER_BYTES_MAX / PERIOD_BYTES_MAX,
	.periods_max		= BUFFER_BYTES_MAX / PERIOD_BYTES_MIN,
};

static int abox_wdma_request_ipc(struct abox_platform_data *data,
		ABOX_IPC_MSG *msg, int atomic, int sync)
{
	struct device *dev_abox = data->dev_abox;

	return abox_request_ipc(dev_abox, msg->ipcid, msg, sizeof(*msg),
			atomic, sync);
}

static irqreturn_t abox_wdma_ipc_handler(int ipc, void *dev_id,
		ABOX_IPC_MSG *msg)
{
	struct abox_data *abox_data = dev_id;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg->msg.pcmtask;
	int id = pcmtask_msg->channel_id;
	struct abox_platform_data *data;
	struct device *dev;

	if (id >= ARRAY_SIZE(abox_data->pdev_wdma) || !abox_data->pdev_wdma[id])
		return IRQ_NONE;

	dev = &abox_data->pdev_wdma[id]->dev;
	data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s(%d)\n", __func__, pcmtask_msg->msgtype);

	switch (pcmtask_msg->msgtype) {
	case PCM_PLTDAI_POINTER:
		data->pointer = pcmtask_msg->param.pointer;
		snd_pcm_period_elapsed(data->substream);
		break;
	case PCM_PLTDAI_ACK:
		data->ack_enabled = !!pcmtask_msg->param.trigger;
		break;
	case PCM_PLTDAI_CLOSED:
		complete(&data->closed);
		break;
	default:
		dev_warn(dev, "unknown message: %d\n", pcmtask_msg->msgtype);
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static int abox_wdma_progress(struct abox_platform_data *data)
{
	unsigned int val = 0;

	regmap_read(data->abox_data->regmap, ABOX_WDMA_STATUS(data->id), &val);

	return !!(val & ABOX_WDMA_PROGRESS_MASK);
}

static void abox_wdma_disable_barrier(struct device *dev,
		struct abox_platform_data *data)
{
	int id = data->id;
	struct abox_data *abox_data = data->abox_data;
	u64 timeout = local_clock() + ABOX_DMA_TIMEOUT_NS;

	while (abox_wdma_progress(data)) {
		if (local_clock() <= timeout) {
			cond_resched();
			continue;
		}
		dev_warn_ratelimited(dev, "WDMA disable timeout[%d]\n", id);
		abox_dbg_dump_simple(dev, abox_data, "WDMA disable timeout");
		break;
	}
}

static int abox_wdma_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct abox_data *abox_data = data->abox_data;
	struct device *dev_abox = &data->abox_data->pdev->dev;
	struct snd_pcm_runtime *runtime = substream->runtime;
	int id = data->id;
	size_t buffer_bytes = PAGE_ALIGN(params_buffer_bytes(params));
	int ret;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	if (data->buf_type == BUFFER_TYPE_DMA) {
		if (data->dmab.bytes < buffer_bytes) {
			abox_iommu_unmap(dev_abox, IOVA_WDMA_BUFFER(id));
			snd_dma_free_pages(&data->dmab);
			ret = snd_dma_alloc_pages(SNDRV_DMA_TYPE_DEV,
					dev,
					buffer_bytes,
					&data->dmab);
			if (ret < 0)
				return ret;
			ret = abox_iommu_map(dev_abox, IOVA_WDMA_BUFFER(id),
					data->dmab.addr, data->dmab.bytes,
					data->dmab.area);
			if (ret < 0)
				return ret;
			dev_info(dev, "dma buffer changed\n");
		}
	} else if (data->buf_type == BUFFER_TYPE_ION) {
		dev_info(dev, "ion_buffer %s bytes(%d) size(%d)\n",
				__func__,
				buffer_bytes, data->ion_buf.size);
	} else {
		dev_err(dev, "buf_type is not defined\n");
	}

	snd_pcm_set_runtime_buffer(substream, &data->dmab);
	runtime->dma_bytes = params_buffer_bytes(params);

	pcmtask_msg->channel_id = id;
	msg.ipcid = IPC_PCMCAPTURE;
	msg.task_id = pcmtask_msg->channel_id = id;

	pcmtask_msg->msgtype = PCM_SET_BUFFER;
	pcmtask_msg->param.setbuff.phyaddr = IOVA_WDMA_BUFFER(id);
	pcmtask_msg->param.setbuff.size = params_period_bytes(params);
	pcmtask_msg->param.setbuff.count = params_periods(params);
	ret = abox_wdma_request_ipc(data, &msg, 0, 0);
	if (ret < 0)
		return ret;

	pcmtask_msg->msgtype = PCM_PLTDAI_HW_PARAMS;
	pcmtask_msg->param.hw_params.sample_rate = params_rate(params);
	pcmtask_msg->param.hw_params.bit_depth = params_width(params);
	pcmtask_msg->param.hw_params.channels = params_channels(params);
	ret = abox_wdma_request_ipc(data, &msg, 0, 0);
	if (ret < 0)
		return ret;

	if (params_rate(params) > 48000)
		abox_request_cpu_gear_dai(dev, abox_data, rtd->cpu_dai,
				abox_data->cpu_gear_min - 1);

	dev_info(dev, "%s:Total=%zu PrdSz=%u(%u) #Prds=%u rate=%u, width=%d, channels=%u\n",
			snd_pcm_stream_str(substream), runtime->dma_bytes,
			params_period_size(params), params_period_bytes(params),
			params_periods(params), params_rate(params),
			params_width(params), params_channels(params));

	return 0;
}

static int abox_wdma_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	int id = data->id;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_HW_FREE;
	msg.task_id = pcmtask_msg->channel_id = id;
	abox_wdma_request_ipc(data, &msg, 0, 0);

	switch (data->type) {
	default:
		abox_wdma_disable_barrier(dev, data);
		break;
	}

	snd_pcm_set_runtime_buffer(substream, NULL);
	return 0;
}

static int abox_wdma_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	int id = data->id;
	int ret;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	data->pointer = IOVA_WDMA_BUFFER(id);

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_PREPARE;
	msg.task_id = pcmtask_msg->channel_id = id;
	ret = abox_wdma_request_ipc(data, &msg, 0, 0);

	return ret;
}

static int abox_wdma_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	int id = data->id;
	int ret;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_info(dev, "%s[%d](%d)\n", __func__, id, cmd);

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_TRIGGER;
	msg.task_id = pcmtask_msg->channel_id = id;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		pcmtask_msg->param.trigger = 1;
		ret = abox_wdma_request_ipc(data, &msg, 1, 0);
		switch (data->type) {
		case PLATFORM_REALTIME:
			msg.ipcid = IPC_ERAP;
			msg.msg.erap.msgtype = REALTIME_START;
			ret = abox_wdma_request_ipc(data, &msg, 1, 0);
			break;
		default:
			break;
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pcmtask_msg->param.trigger = 0;
		ret = abox_wdma_request_ipc(data, &msg, 1, 0);
		switch (data->type) {
		case PLATFORM_REALTIME:
			msg.ipcid = IPC_ERAP;
			msg.msg.erap.msgtype = REALTIME_STOP;
			ret = abox_wdma_request_ipc(data, &msg, 1, 0);
			break;
		default:
			break;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static snd_pcm_uframes_t abox_wdma_pointer(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int id = data->id;
	ssize_t pointer;
	unsigned int status = 0;
	bool progress;

	regmap_read(data->abox_data->regmap, ABOX_WDMA_STATUS(id), &status);
	progress = !!(status & ABOX_WDMA_PROGRESS_MASK);

	if (data->pointer >= IOVA_WDMA_BUFFER(id)) {
		pointer = data->pointer - IOVA_WDMA_BUFFER(id);
	} else if (((data->type == PLATFORM_NORMAL) ||
			(data->type == PLATFORM_SYNC)) && progress) {
		ssize_t offset, count;
		ssize_t buffer_bytes, period_bytes;

		buffer_bytes = snd_pcm_lib_buffer_bytes(substream);
		period_bytes = snd_pcm_lib_period_bytes(substream);

		offset = (((status & ABOX_WDMA_RBUF_OFFSET_MASK) >>
				ABOX_WDMA_RBUF_OFFSET_L) << 4);

		if (period_bytes > ABOX_WDMA_RBUF_CNT_MASK + 1)
			count = 0;
		else
			count = (status & ABOX_WDMA_RBUF_CNT_MASK);

		while ((offset % period_bytes) && (buffer_bytes >= 0)) {
			buffer_bytes -= period_bytes;
			if ((buffer_bytes & offset) == offset)
				offset = buffer_bytes;
		}

		pointer = offset + count;
	} else {
		pointer = 0;
	}

	dev_dbg(dev, "%s[%d]: pointer=%08zx\n", __func__, id, pointer);

	return bytes_to_frames(runtime, pointer);
}

static int abox_wdma_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct abox_data *abox_data = data->abox_data;
	int id = data->id;
	int ret;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	abox_wait_restored(abox_data);

	if (data->type == PLATFORM_CALL) {
		if (abox_cpu_gear_idle(dev, ABOX_CPU_GEAR_CALL_VSS))
			abox_request_cpu_gear_sync(dev, abox_data,
					ABOX_CPU_GEAR_CALL_KERNEL,
					ABOX_CPU_GEAR_MAX, rtd->cpu_dai->name);
		ret = abox_vss_notify_call(dev, abox_data, 1);
		if (ret < 0)
			dev_warn(dev, "call notify failed: %d\n", ret);
	}
	abox_request_cpu_gear_dai(dev, abox_data, rtd->cpu_dai,
			abox_data->cpu_gear_min);

	snd_soc_set_runtime_hwparams(substream, &abox_wdma_hardware);

	data->substream = substream;

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_OPEN;
	msg.task_id = pcmtask_msg->channel_id = id;
	ret = abox_wdma_request_ipc(data, &msg, 0, 0);

	return ret;
}

static int abox_wdma_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct abox_data *abox_data = data->abox_data;
	int id = data->id;
	int ret;
	long time;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	data->substream = NULL;

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_CLOSE;
	msg.task_id = pcmtask_msg->channel_id = id;
	ret = abox_wdma_request_ipc(data, &msg, 0, 1);

	abox_request_cpu_gear_dai(dev, abox_data, rtd->cpu_dai, 0);
	if (data->type == PLATFORM_CALL) {
		abox_request_cpu_gear(dev, abox_data, ABOX_CPU_GEAR_CALL_KERNEL,
				ABOX_CPU_GEAR_MIN, rtd->cpu_dai->name);
		ret = abox_vss_notify_call(dev, abox_data, 0);
		if (ret < 0)
			dev_warn(dev, "call notify failed: %d\n", ret);
	}

	time = wait_for_completion_timeout(&data->closed,
			nsecs_to_jiffies(ABOX_DMA_TIMEOUT_NS));
	if (time == 0)
		dev_err(dev, "%s: timeout\n", __func__);

	/* Release ASRC to reuse it in other DMA */
	abox_cmpnt_asrc_release(abox_data->cmpnt, SNDRV_PCM_STREAM_CAPTURE, id);

	return ret;
}

static int abox_wdma_mmap(struct snd_pcm_substream *substream,
		struct vm_area_struct *vma)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	int id = data->id;
	struct snd_pcm_runtime *runtime = substream->runtime;

	dev_info(dev, "%s[%d]\n", __func__, id);

	/* Increased cpu gear for sound camp.
	 * Only sound camp uses mmap now.
	 */
	abox_request_cpu_gear_dai(dev, data->abox_data, rtd->cpu_dai,
			data->abox_data->cpu_gear_min - 1);

	dev_info(dev, "%s bytes(%d) size(%d)\n", __func__,
			runtime->dma_bytes, data->ion_buf.size);

	if (data->buf_type == BUFFER_TYPE_ION)
		return dma_buf_mmap(data->ion_buf.dma_buf, vma, 0);
	else
		return dma_mmap_writecombine(dev, vma,
				runtime->dma_area,
				runtime->dma_addr,
				runtime->dma_bytes);
}

static int abox_wdma_ack(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	int id = data->id;
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_pcm_uframes_t appl_ptr = runtime->control->appl_ptr;
	snd_pcm_uframes_t appl_ofs = appl_ptr % runtime->buffer_size;
	ssize_t appl_bytes = frames_to_bytes(runtime, appl_ofs);
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	if (!data->ack_enabled)
		return 0;

	dev_dbg(dev, "%s[%d]: %zd\n", __func__, id, appl_bytes);

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_ACK;
	pcmtask_msg->param.pointer = (unsigned int)appl_bytes;
	msg.task_id = pcmtask_msg->channel_id = id;

	return abox_wdma_request_ipc(data, &msg, 1, 0);
}

static struct snd_pcm_ops abox_wdma_ops = {
	.open		= abox_wdma_open,
	.close		= abox_wdma_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= abox_wdma_hw_params,
	.hw_free	= abox_wdma_hw_free,
	.prepare	= abox_wdma_prepare,
	.trigger	= abox_wdma_trigger,
	.pointer	= abox_wdma_pointer,
	.mmap		= abox_wdma_mmap,
	.ack		= abox_wdma_ack,
};

static int abox_wdma_fio_ioctl(struct snd_hwdep *hw, struct file *file,
		unsigned int cmd, unsigned long _arg);

#ifdef CONFIG_COMPAT
static int abox_wdma_fio_compat_ioctl(struct snd_hwdep *hw,
		struct file *file,
		unsigned int cmd, unsigned long _arg);
#endif

static int abox_pcm_add_hwdep_dev(struct snd_soc_pcm_runtime *runtime,
		struct abox_platform_data *data)
{
	struct snd_hwdep *hwdep;
	int rc;
	char id[] = "ABOX_MMAP_FD_NN";

	snprintf(id, sizeof(id), "ABOX_MMAP_FD_%d", SNDRV_PCM_STREAM_CAPTURE);
	pr_debug("%s: pcm dev %d\n", __func__, runtime->pcm->device);
	rc = snd_hwdep_new(runtime->card->snd_card,
			   &id[0],
			   0 + runtime->pcm->device,
			   &hwdep);
	if (!hwdep || rc < 0) {
		pr_err("%s: hwdep intf failed to create %s - hwdep\n", __func__,
		       id);
		return rc;
	}

	hwdep->iface = 0;
	hwdep->private_data = data;
	hwdep->ops.ioctl = abox_wdma_fio_ioctl;
	hwdep->ops.ioctl_compat = abox_wdma_fio_compat_ioctl;
	data->hwdep = hwdep;

	return 0;
}

static int abox_wdma_new(struct snd_soc_pcm_runtime *runtime)
{
	struct snd_soc_platform *platform = runtime->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct device *dev_abox = &data->abox_data->pdev->dev;
	int id = data->id;
	size_t buffer_bytes = data->dmab.bytes;
	int ret;

	if (data->buf_type == BUFFER_TYPE_ION) {
		buffer_bytes = BUFFER_ION_BYTES_MAX;
		data->ion_buf.fd = -2;
		ret = abox_ion_alloc(data,
				&data->ion_buf,
				IOVA_WDMA_BUFFER(id),
				buffer_bytes,
				0);
		if (ret < 0)
			return ret;

		/* update buffer infomation using ion allocated buffer  */
		data->dmab.area = data->ion_buf.kva;
		data->dmab.addr = data->ion_buf.iova;

		ret = abox_pcm_add_hwdep_dev(runtime, data);
		if (ret < 0) {
			dev_err(dev, "snd_hwdep_new() failed: %d\n", ret);
			return ret;
		}
	} else {
		ret = snd_dma_alloc_pages(SNDRV_DMA_TYPE_DEV,
				dev,
				buffer_bytes,
				&data->dmab);
		if (ret < 0)
			return ret;

		ret = abox_iommu_map(dev_abox, IOVA_WDMA_BUFFER(id),
				data->dmab.addr, data->dmab.bytes,
				data->dmab.area);
	}

	return ret;
}

static void abox_wdma_free(struct snd_pcm *pcm)
{
	struct snd_soc_pcm_runtime *runtime = pcm->private_data;
	struct snd_soc_platform *platform = runtime->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct device *dev_abox = &data->abox_data->pdev->dev;
	int id = data->id;

	int ret = 0;

	if (data->buf_type == BUFFER_TYPE_ION) {
		ret = abox_ion_free(data);
		if (ret < 0)
			dev_err(dev, "abox_ion_free() failed %d\n", ret);

		if (data->hwdep) {
			snd_device_free(runtime->card->snd_card, data->hwdep);
			data->hwdep = NULL;
		}
	} else {
		abox_iommu_unmap(dev_abox, IOVA_WDMA_BUFFER(id));
		snd_dma_free_pages(&data->dmab);
	}
}

static struct snd_soc_platform_driver abox_wdma = {
	.ops		= &abox_wdma_ops,
	.pcm_new	= abox_wdma_new,
	.pcm_free	= abox_wdma_free,
};

static int abox_wdma_cmpnt_probe(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;
	struct abox_platform_data *data = snd_soc_component_get_drvdata(cmpnt);
	u32 id;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	data->cmpnt = cmpnt;
	snd_soc_component_init_regmap(cmpnt, data->abox_data->regmap);
	abox_cmpnt_register_wdma(data->abox_data->pdev, to_platform_device(dev),
			data->id, snd_soc_component_get_dapm(cmpnt),
			data->of_data->get_dai_name(dev, data->id));

	ret = of_samsung_property_read_u32(dev, dev->of_node, "asrc-id", &id);
	if (ret >= 0) {
		ret = abox_cmpnt_asrc_lock(data->abox_data->cmpnt,
				SNDRV_PCM_STREAM_CAPTURE, data->id, id);
		if (ret < 0)
			dev_err(dev, "asrc id lock failed\n");
		else
			dev_info(dev, "asrc id locked: %u\n", id);
	}

	return 0;
}

static void abox_wdma_cmpnt_remove(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;

	dev_info(dev, "%s\n", __func__);
}

static const struct snd_soc_component_driver abox_wdma_cmpnt = {
	.probe			= abox_wdma_cmpnt_probe,
	.remove			= abox_wdma_cmpnt_remove,
};

static struct snd_soc_dai_driver abox_wdma_dai_drv = {
	.capture = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = ABOX_SAMPLING_RATES,
		.rate_min = 8000,
		.rate_max = 384000,
		.formats = ABOX_WDMA_SAMPLE_FORMATS,
	},
};

enum abox_dai abox_wdma_get_dai_id(int id)
{
	int ret = ABOX_WDMA0 + id;

	return (ret < ABOX_UAIF0) ? ret : -EINVAL;
}

const char *abox_wdma_get_dai_name(struct device *dev, int id)
{
	return devm_kasprintf(dev, GFP_KERNEL, "WDMA%d", id);
}

const char *abox_wdma_get_str_name(struct device *dev, int id, int stream)
{
	const char *ret;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = ERR_PTR(-EINVAL);
	else
		ret = devm_kasprintf(dev, GFP_KERNEL, "WDMA%d Capture", id);

	return ret;
}

static const struct of_device_id samsung_abox_wdma_match[] = {
	{
		.compatible = "samsung,abox-wdma",
		.data = (void *)&(struct abox_dma_of_data){
			.get_dai_id = abox_wdma_get_dai_id,
			.get_dai_name = abox_wdma_get_dai_name,
			.get_str_name = abox_wdma_get_str_name,
			.base_dai_drv = &abox_wdma_dai_drv,
		},
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_wdma_match);

static int abox_wdma_fio_common_ioctl(struct snd_hwdep *hw, struct file *filp,
		unsigned int cmd, unsigned long __user *_arg)
{
	struct abox_platform_data *data = hw->private_data;
	struct device *dev;
	struct snd_pcm_mmap_fd mmap_fd;

	int ret = 0;
	unsigned long arg;

	if (!data || (((cmd >> 8) & 0xff) != 'U'))
		return -ENOTTY;

	dev = &data->pdev->dev;

	if (get_user(arg, _arg))
		return -EFAULT;

	dev_dbg(dev, "%s: ioctl(0x%x)\n", __func__, cmd);

	switch (cmd) {
	case SNDRV_PCM_IOCTL_MMAP_DATA_FD:
		ret = abox_mmap_fd(data, &mmap_fd);
		if (ret < 0)
			dev_err(dev, "%s MMAP_FD failed: %d\n", __func__, ret);

		if (copy_to_user(_arg, &mmap_fd, sizeof(mmap_fd)))
			return -EFAULT;
		break;
	default:
		dev_err(dev, "unknown ioctl = 0x%x\n", cmd);
		return -EINVAL;
	}

	return 0;
}

static int abox_wdma_fio_ioctl(struct snd_hwdep *hw, struct file *file,
		unsigned int cmd, unsigned long _arg)
{
	return abox_wdma_fio_common_ioctl(hw, file,
			cmd, (unsigned long __user *)_arg);
}

#ifdef CONFIG_COMPAT
static int abox_wdma_fio_compat_ioctl(struct snd_hwdep *hw,
		struct file *file,
		unsigned int cmd, unsigned long _arg)
{
	return abox_wdma_fio_common_ioctl(hw, file, cmd, compat_ptr(_arg));
}
#endif /* CONFIG_COMPAT */

static int samsung_abox_wdma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct abox_platform_data *data;
	int ret;
	u32 value;
	const char *type;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	platform_set_drvdata(pdev, data);
	data->pdev = pdev;

	data->sfr_base = devm_get_ioremap(pdev, "sfr", NULL, NULL);
	if (IS_ERR(data->sfr_base))
		return PTR_ERR(data->sfr_base);

	data->dev_abox = pdev->dev.parent;
	if (!data->dev_abox) {
		dev_err(dev, "Failed to get abox device\n");
		return -EPROBE_DEFER;
	}
	data->abox_data = dev_get_drvdata(data->dev_abox);

	init_completion(&data->closed);

	abox_register_ipc_handler(data->dev_abox, IPC_PCMCAPTURE,
			abox_wdma_ipc_handler, data->abox_data);

	ret = of_samsung_property_read_u32(dev, np, "id", &data->id);
	if (ret < 0)
		return ret;

	ret = of_samsung_property_read_string(dev, np, "type", &type);
	if (ret < 0)
		type = "";
	if (!strncmp(type, "call", sizeof("call")))
		data->type = PLATFORM_CALL;
	else if (!strncmp(type, "compress", sizeof("compress")))
		data->type = PLATFORM_COMPRESS;
	else if (!strncmp(type, "realtime", sizeof("realtime")))
		data->type = PLATFORM_REALTIME;
	else if (!strncmp(type, "vi-sensing", sizeof("vi-sensing")))
		data->type = PLATFORM_VI_SENSING;
	else if (!strncmp(type, "sync", sizeof("sync")))
		data->type = PLATFORM_SYNC;
	else
		data->type = PLATFORM_NORMAL;

	ret = of_samsung_property_read_u32(dev, np, "buffer_bytes", &value);
	if (ret < 0)
		value = BUFFER_BYTES_MIN;
	data->dmab.bytes = value;

	ret = of_samsung_property_read_string(dev, np, "buffer_type", &type);
	if (ret < 0)
		type = "";
	if (!strncmp(type, "ion", sizeof("ion")))
		data->buf_type = BUFFER_TYPE_ION;
	else if (!strncmp(type, "dma", sizeof("dma")))
		data->buf_type = BUFFER_TYPE_DMA;
	else
		data->buf_type = BUFFER_TYPE_DMA;

	data->of_data = of_match_node(samsung_abox_wdma_match,
			pdev->dev.of_node)->data;
	data->dai_drv = devm_kzalloc(dev, sizeof(*data->dai_drv), GFP_KERNEL);
	if (!data->dai_drv)
		return -ENOMEM;
	memcpy(data->dai_drv, data->of_data->base_dai_drv,
			sizeof(struct snd_soc_dai_driver));
	data->dai_drv->id = data->of_data->get_dai_id(data->id);
	data->dai_drv->name = data->of_data->get_dai_name(dev, data->id);
	if (data->dai_drv->capture.formats)
		data->dai_drv->capture.stream_name =
			data->of_data->get_str_name(dev, data->id,
					SNDRV_PCM_STREAM_CAPTURE);

	ret = devm_snd_soc_register_component(dev, &abox_wdma_cmpnt,
			data->dai_drv, 1);
	if (ret < 0)
		return ret;

	pm_runtime_no_callbacks(dev);
	pm_runtime_enable(dev);

	ret = snd_soc_register_platform(&pdev->dev, &abox_wdma);
	if (ret < 0)
		return ret;

	data->hwdep = NULL;

	return 0;
}

static int samsung_abox_wdma_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static struct platform_driver samsung_abox_wdma_driver = {
	.probe  = samsung_abox_wdma_probe,
	.remove = samsung_abox_wdma_remove,
	.driver = {
		.name = "samsung-abox-wdma",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_wdma_match),
	},
};

module_platform_driver(samsung_abox_wdma_driver);

/* Module information */
MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box WDMA Driver");
MODULE_ALIAS("platform:samsung-abox-wdma");
MODULE_LICENSE("GPL");
