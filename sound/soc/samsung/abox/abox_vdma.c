/* sound/soc/samsung/abox/abox_vdma.c
 *
 * ALSA SoC Audio Layer - Samsung Abox Virtual DMA driver
 *
 * Copyright (c) 2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#undef DEBUG
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/memblock.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>

#include "abox_util.h"
#include "abox.h"
#include "abox_vdma.h"

#undef TEST
#define VDMA_COUNT_MAX SZ_32
#define NAME_LENGTH SZ_32


struct abox_vdma_rtd {
	struct snd_dma_buffer buffer;
	struct snd_pcm_hardware hardware;
	struct snd_pcm_substream *substream;
	unsigned long iova;
	size_t pointer;
	bool iommu_mapped;
};

struct abox_vdma_info {
	struct device *dev;
	int id;
	char name[NAME_LENGTH];
	struct abox_vdma_rtd rtd[SNDRV_PCM_STREAM_LAST + 1];
};

static struct device *abox_vdma_dev_abox;
static struct abox_vdma_info abox_vdma_list[VDMA_COUNT_MAX];

static int abox_vdma_get_idx(int id)
{
	return id - PCMTASK_VDMA_ID_BASE;
}

static unsigned long abox_vdma_get_iova(int id, int stream)
{
	int idx = abox_vdma_get_idx(id);
	long ret;

	switch (stream) {
	case SNDRV_PCM_STREAM_PLAYBACK:
	case SNDRV_PCM_STREAM_CAPTURE:
		ret = IOVA_VDMA_BUFFER(idx) + (SZ_512K * stream);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static struct abox_vdma_info *abox_vdma_get_info(int id)
{
	int idx = abox_vdma_get_idx(id);

	if (idx < 0 || idx >= ARRAY_SIZE(abox_vdma_list))
		return NULL;

	return &abox_vdma_list[idx];
}

static struct abox_vdma_rtd *abox_vdma_get_rtd(struct abox_vdma_info *info,
		int stream)
{
	if (!info || stream < 0 || stream >= ARRAY_SIZE(info->rtd))
		return NULL;

	return &info->rtd[stream];
}

static int abox_vdma_request_ipc(ABOX_IPC_MSG *msg, int atomic, int sync)
{
	return abox_request_ipc(abox_vdma_dev_abox, msg->ipcid, msg,
			sizeof(*msg), atomic, sync);
}

int abox_vdma_period_elapsed(struct abox_vdma_info *info,
		struct abox_vdma_rtd *rtd, size_t pointer)
{
	dev_dbg(info->dev, "%s[%d:%c](%zx)\n", __func__, info->id,
			substream_to_char(rtd->substream), pointer);

	rtd->pointer = pointer - rtd->iova;
	snd_pcm_period_elapsed(rtd->substream);

	return 0;
}

static int abox_vdma_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *soc_rtd = substream->private_data;
	struct snd_soc_platform *platform = soc_rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	struct abox_vdma_info *info = snd_soc_platform_get_drvdata(platform);
	struct abox_vdma_rtd *rtd = abox_vdma_get_rtd(info, substream->stream);
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d:%c]\n", __func__, id, substream_to_char(substream));

	abox_wait_restored(dev_get_drvdata(abox_vdma_dev_abox));

	rtd->substream = substream;
	snd_soc_set_runtime_hwparams(substream, &rtd->hardware);

	msg.ipcid = abox_stream_to_ipcid(substream->stream);
	msg.task_id = pcmtask_msg->channel_id = id;
	pcmtask_msg->msgtype = PCM_PLTDAI_OPEN;
	return abox_vdma_request_ipc(&msg, 0, 0);
}

static int abox_vdma_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *soc_rtd = substream->private_data;
	struct snd_soc_platform *platform = soc_rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	struct abox_vdma_info *info = snd_soc_platform_get_drvdata(platform);
	struct abox_vdma_rtd *rtd = abox_vdma_get_rtd(info, substream->stream);
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d:%c]\n", __func__, id, substream_to_char(substream));

	rtd->substream = NULL;

	msg.ipcid = abox_stream_to_ipcid(substream->stream);
	msg.task_id = pcmtask_msg->channel_id = id;
	pcmtask_msg->msgtype = PCM_PLTDAI_CLOSE;
	return abox_vdma_request_ipc(&msg, 0, 1);
}

static int abox_vdma_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *soc_rtd = substream->private_data;
	struct snd_soc_platform *platform = soc_rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	struct abox_vdma_info *info = snd_soc_platform_get_drvdata(platform);
	struct abox_vdma_rtd *rtd = abox_vdma_get_rtd(info, substream->stream);
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;
	int ret;

	dev_dbg(dev, "%s[%d:%c]\n", __func__, id, substream_to_char(substream));

	ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (ret < 0)
		return ret;

	msg.ipcid = abox_stream_to_ipcid(substream->stream);
	msg.task_id = pcmtask_msg->channel_id = id;

	pcmtask_msg->msgtype = PCM_SET_BUFFER;
	pcmtask_msg->param.setbuff.phyaddr = rtd->iova;
	pcmtask_msg->param.setbuff.size = params_period_bytes(params);
	pcmtask_msg->param.setbuff.count = params_periods(params);
	ret = abox_vdma_request_ipc(&msg, 0, 0);
	if (ret < 0)
		return ret;

	pcmtask_msg->msgtype = PCM_PLTDAI_HW_PARAMS;
	pcmtask_msg->param.hw_params.sample_rate = params_rate(params);
	pcmtask_msg->param.hw_params.bit_depth = params_width(params);
	pcmtask_msg->param.hw_params.channels = params_channels(params);
	ret = abox_vdma_request_ipc(&msg, 0, 0);
	if (ret < 0)
		return ret;

	return ret;
}

static int abox_vdma_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *soc_rtd = substream->private_data;
	struct snd_soc_platform *platform = soc_rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d:%c]\n", __func__, id, substream_to_char(substream));

	msg.ipcid = abox_stream_to_ipcid(substream->stream);
	msg.task_id = pcmtask_msg->channel_id = id;
	pcmtask_msg->msgtype = PCM_PLTDAI_HW_FREE;
	abox_vdma_request_ipc(&msg, 0, 0);

	return snd_pcm_lib_free_pages(substream);
}

static int abox_vdma_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *soc_rtd = substream->private_data;
	struct snd_soc_platform *platform = soc_rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	struct abox_vdma_info *info = snd_soc_platform_get_drvdata(platform);
	struct abox_vdma_rtd *rtd = abox_vdma_get_rtd(info, substream->stream);
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d:%c]\n", __func__, id, substream_to_char(substream));

	rtd->pointer = 0;

	msg.ipcid = abox_stream_to_ipcid(substream->stream);
	msg.task_id = pcmtask_msg->channel_id = id;
	pcmtask_msg->msgtype = PCM_PLTDAI_PREPARE;
	return abox_vdma_request_ipc(&msg, 0, 0);
}

static int abox_vdma_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *soc_rtd = substream->private_data;
	struct snd_soc_platform *platform = soc_rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;
	int ret;

	dev_info(dev, "%s[%d:%c](%d)\n", __func__, id,
			substream_to_char(substream), cmd);

	msg.ipcid = abox_stream_to_ipcid(substream->stream);
	msg.task_id = pcmtask_msg->channel_id = id;
	pcmtask_msg->msgtype = PCM_PLTDAI_TRIGGER;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		pcmtask_msg->param.trigger = 1;
		ret = abox_vdma_request_ipc(&msg, 1, 0);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pcmtask_msg->param.trigger = 0;
		ret = abox_vdma_request_ipc(&msg, 1, 0);
		break;
	default:
		dev_err(dev, "invalid command: %d\n", cmd);
		ret = -EINVAL;
	}

	return ret;
}

static snd_pcm_uframes_t abox_vdma_pointer(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *soc_rtd = substream->private_data;
	struct snd_soc_platform *platform = soc_rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	struct abox_vdma_info *info = snd_soc_platform_get_drvdata(platform);
	struct abox_vdma_rtd *rtd = abox_vdma_get_rtd(info, substream->stream);

	dev_dbg(dev, "%s[%d:%c]\n", __func__, id, substream_to_char(substream));

	return bytes_to_frames(substream->runtime, rtd->pointer);
}

static int abox_vdma_ack(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *pcm_rtd = substream->runtime;
	struct snd_soc_pcm_runtime *soc_rtd = substream->private_data;
	struct snd_soc_platform *platform = soc_rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	snd_pcm_uframes_t appl_ptr = pcm_rtd->control->appl_ptr;
	snd_pcm_uframes_t appl_ofs = appl_ptr % pcm_rtd->buffer_size;
	ssize_t appl_bytes = frames_to_bytes(pcm_rtd, appl_ofs);
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	/* Firmware doesn't need ack of capture stream. */
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return 0;

	dev_dbg(dev, "%s[%d:%c]: %zd\n", __func__, id,
			substream_to_char(substream), appl_bytes);

	msg.ipcid = abox_stream_to_ipcid(substream->stream);
	msg.task_id = pcmtask_msg->channel_id = id;
	pcmtask_msg->msgtype = PCM_PLTDAI_ACK;
	pcmtask_msg->param.pointer = (unsigned int)appl_bytes;

	return abox_vdma_request_ipc(&msg, 1, 0);
}

static struct snd_pcm_ops abox_vdma_platform_ops = {
	.open		= abox_vdma_open,
	.close		= abox_vdma_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= abox_vdma_hw_params,
	.hw_free	= abox_vdma_hw_free,
	.prepare	= abox_vdma_prepare,
	.trigger	= abox_vdma_trigger,
	.pointer	= abox_vdma_pointer,
	.ack		= abox_vdma_ack,
};

static int abox_vdma_platform_probe(struct snd_soc_platform *platform)
{
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	snd_soc_platform_set_drvdata(platform, abox_vdma_get_info(id));
	return 0;
}

static int abox_vdma_platform_new(struct snd_soc_pcm_runtime *soc_rtd)
{
	struct device *dev = soc_rtd->platform->dev;
	struct device *dev_abox = abox_vdma_dev_abox;
	struct snd_pcm *pcm = soc_rtd->pcm;
	int id = to_platform_device(dev)->id;
	struct abox_vdma_info *info = abox_vdma_get_info(id);
	int i, ret;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	for (i = 0; i <= SNDRV_PCM_STREAM_LAST; i++) {
		struct snd_pcm_substream *substream = pcm->streams[i].substream;

		if (!substream)
			continue;

		if (info->rtd[i].iova == 0)
			info->rtd[i].iova = abox_vdma_get_iova(id, i);

		if (info->rtd[i].buffer.bytes == 0)
			info->rtd[i].buffer.bytes = BUFFER_BYTES_MIN;

		if (info->rtd[i].buffer.addr) {
			substream->dma_buffer = info->rtd[i].buffer;
		} else {
			size_t size = info->rtd[i].buffer.bytes;

			ret = snd_pcm_lib_preallocate_pages(substream,
					SNDRV_DMA_TYPE_DEV, dev_abox,
					size, size);
			if (ret < 0)
				return ret;
		}

		if (abox_iova_to_phys(dev_abox, info->rtd[i].iova) == 0) {
			ret = abox_iommu_map(dev_abox, info->rtd[i].iova,
					substream->dma_buffer.addr,
					substream->dma_buffer.bytes,
					substream->dma_buffer.area);
			if (ret < 0)
				return ret;

			info->rtd[i].iommu_mapped = true;
		}

		info->rtd[i].substream = substream;
	}

	return 0;
}

static void abox_vdma_platform_free(struct snd_pcm *pcm)
{
	struct snd_soc_pcm_runtime *soc_rtd = pcm->private_data;
	struct device *dev = soc_rtd->platform->dev;
	struct device *dev_abox = abox_vdma_dev_abox;
	int id = to_platform_device(dev)->id;
	struct abox_vdma_info *info = abox_vdma_get_info(id);
	int i;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	for (i = 0; i <= SNDRV_PCM_STREAM_LAST; i++) {
		struct snd_pcm_substream *substream = pcm->streams[i].substream;

		if (!substream)
			continue;

		info->rtd[i].substream = NULL;

		if (info->rtd[i].iommu_mapped) {
			abox_iommu_unmap(dev_abox, info->rtd[i].iova);
			info->rtd[i].iommu_mapped = false;
		}

		if (info->rtd[i].buffer.addr) {
			substream->dma_buffer.area = NULL;
		} else {
			snd_pcm_lib_preallocate_free(substream);
		}
	}
}

struct snd_soc_platform_driver abox_vdma_platform = {
	.probe		= abox_vdma_platform_probe,
	.ops		= &abox_vdma_platform_ops,
	.pcm_new	= abox_vdma_platform_new,
	.pcm_free	= abox_vdma_platform_free,
};

static irqreturn_t abox_vdma_ipc_handler(int ipc_id, void *dev_id,
		ABOX_IPC_MSG *msg)
{
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg->msg.pcmtask;
	int id = pcmtask_msg->channel_id;
	int stream = abox_ipcid_to_stream(ipc_id);
	struct abox_vdma_info *info = abox_vdma_get_info(id);
	struct abox_vdma_rtd *rtd = abox_vdma_get_rtd(info, stream);

	if (!info || !rtd)
		return IRQ_NONE;

	switch (pcmtask_msg->msgtype) {
	case PCM_PLTDAI_POINTER:
		abox_vdma_period_elapsed(info, rtd, pcmtask_msg->param.pointer);
		break;
	case PCM_PLTDAI_ACK:
		/* Ignore ack request and always send ack IPC to firmware. */
		break;
	case PCM_PLTDAI_REGISTER:
	{
		struct PCMTASK_HARDWARE *hardware;
		struct device *dev_abox = dev_id;
		struct abox_data *data = dev_get_drvdata(dev_abox);
		void *area;
		phys_addr_t addr;

		hardware = &pcmtask_msg->param.hardware;
		area = abox_addr_to_kernel_addr(data, hardware->addr);
		addr = abox_addr_to_phys_addr(data, hardware->addr);
		abox_vdma_register(dev_abox, id, stream, area, addr, hardware);
		break;
	}
	default:
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static struct snd_soc_dai_link abox_vdma_dai_links[VDMA_COUNT_MAX];
static struct snd_soc_card abox_vdma_card = {
	.name = "abox_vdma",
	.owner = THIS_MODULE,
	.dai_link = abox_vdma_dai_links,
	.num_links = 0,
};

void abox_vdma_register_work_func(struct work_struct *work)
{
	int id;
	struct abox_vdma_info *info;

	dev_dbg(abox_vdma_dev_abox, "%s\n", __func__);

	for (info = abox_vdma_list; (info - abox_vdma_list) <
			ARRAY_SIZE(abox_vdma_list); info++) {
		id = info->id;
		if (info->dev == abox_vdma_dev_abox) {
			dev_dbg(info->dev, "%s[%d]\n", __func__, id);
			platform_device_register_data(info->dev,
					"samsung-abox-vdma", id, NULL, 0);
		}
	}
}

static DECLARE_WORK(abox_vdma_register_work, abox_vdma_register_work_func);

int abox_vdma_register(struct device *dev, int id, int stream,
		void *area, phys_addr_t addr,
		const struct PCMTASK_HARDWARE *pcm_hardware)
{
	struct abox_vdma_info *info = abox_vdma_get_info(id);
	struct abox_vdma_rtd *rtd = abox_vdma_get_rtd(info, stream);
	struct snd_dma_buffer *buffer = &rtd->buffer;
	struct snd_pcm_hardware *hardware = &rtd->hardware;

	if (!info || !rtd)
		return -EINVAL;

	if (info->dev)
		return -EEXIST;

	dev_info(dev, "%s(%d, %s, %d, %u)\n", __func__, id, pcm_hardware->name,
			stream, pcm_hardware->buffer_bytes_max);

	info->id = id;
	strncpy(info->name, pcm_hardware->name, sizeof(info->name) - 1);

	rtd->iova = pcm_hardware->addr;

	buffer->dev.type = SNDRV_DMA_TYPE_DEV;
	buffer->dev.dev = dev;
	buffer->area = area;
	buffer->addr = addr;
	buffer->bytes = pcm_hardware->buffer_bytes_max;

	hardware->info = SNDRV_PCM_INFO_INTERLEAVED
			| SNDRV_PCM_INFO_BLOCK_TRANSFER
			| SNDRV_PCM_INFO_MMAP
			| SNDRV_PCM_INFO_MMAP_VALID;
	hardware->formats = width_range_to_bits(pcm_hardware->width_min,
			pcm_hardware->width_max);
	hardware->rates = (pcm_hardware->rate_max > 192000) ?
			SNDRV_PCM_RATE_KNOT : snd_pcm_rate_range_to_bits(
			pcm_hardware->rate_min, pcm_hardware->rate_max);
	hardware->rate_min = pcm_hardware->rate_min;
	hardware->rate_max = pcm_hardware->rate_max;
	hardware->channels_min = pcm_hardware->channels_min;
	hardware->channels_max = pcm_hardware->channels_max;
	hardware->buffer_bytes_max = pcm_hardware->buffer_bytes_max;
	hardware->period_bytes_min = pcm_hardware->period_bytes_min;
	hardware->period_bytes_max = pcm_hardware->period_bytes_max;
	hardware->periods_min = pcm_hardware->periods_min;
	hardware->periods_max = pcm_hardware->periods_max;

	info->dev = dev;
	schedule_work(&abox_vdma_register_work);

	return 0;
}

static void abox_vdma_register_card_work_func(struct work_struct *work)
{
	int i;

	dev_dbg(abox_vdma_dev_abox, "%s\n", __func__);

	for (i = 0; i < abox_vdma_card.num_links; i++) {
		struct snd_soc_dai_link *link = &abox_vdma_card.dai_link[i];

		if (link->name)
			continue;

		link->name = link->stream_name =
				kasprintf(GFP_KERNEL, "dummy%d", i);
		link->stream_name = link->name;
		link->cpu_name = "snd-soc-dummy";
		link->cpu_dai_name = "snd-soc-dummy-dai";
		link->codec_name = "snd-soc-dummy";
		link->codec_dai_name = "snd-soc-dummy-dai";
		link->no_pcm = 1;
	}
	abox_register_extra_sound_card(abox_vdma_card.dev, &abox_vdma_card, 1);
}

static DECLARE_DELAYED_WORK(abox_vdma_register_card_work,
		abox_vdma_register_card_work_func);

static int abox_vdma_add_dai_link(struct device *dev)
{
	int id = to_platform_device(dev)->id;
	int idx = abox_vdma_get_idx(id);
	struct abox_vdma_info *info = abox_vdma_get_info(id);
	struct snd_soc_dai_link *link = &abox_vdma_dai_links[idx];
	struct abox_vdma_rtd *playback, *capture;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	if (idx > ARRAY_SIZE(abox_vdma_dai_links)) {
		dev_err(dev, "Too many request\n");
		return -ENOMEM;
	}

	cancel_delayed_work_sync(&abox_vdma_register_card_work);

	kfree(link->name);
	link->name = link->stream_name = kstrdup(info->name, GFP_KERNEL);
	link->cpu_name = "snd-soc-dummy";
	link->cpu_dai_name = "snd-soc-dummy-dai";
	link->platform_name = dev_name(dev);
	link->codec_name = "snd-soc-dummy";
	link->codec_dai_name = "snd-soc-dummy-dai";
	link->ignore_suspend = 1;
	link->ignore_pmdown_time = 1;
	link->no_pcm = 0;
	playback = &info->rtd[SNDRV_PCM_STREAM_PLAYBACK];
	capture = &info->rtd[SNDRV_PCM_STREAM_CAPTURE];
	link->playback_only = playback->buffer.area && !capture->buffer.area;
	link->capture_only = !playback->buffer.area && capture->buffer.area;

	if (abox_vdma_card.num_links <= idx)
		abox_vdma_card.num_links = idx + 1;

	schedule_delayed_work(&abox_vdma_register_card_work, HZ);

	return 0;
}

static int samsung_abox_vdma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int id = to_platform_device(dev)->id;
	struct abox_vdma_info *info = abox_vdma_get_info(id);
	int ret = 0;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	if (id < 0) {
		abox_vdma_card.dev = &pdev->dev;
		schedule_delayed_work(&abox_vdma_register_card_work, 0);
	} else {
		info->dev = dev;
		pm_runtime_no_callbacks(dev);
		pm_runtime_enable(dev);
		devm_snd_soc_register_platform(dev, &abox_vdma_platform);
		ret = abox_vdma_add_dai_link(dev);
	}

	return ret;
}

static int samsung_abox_vdma_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int id = to_platform_device(dev)->id;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	return 0;
}

static const struct platform_device_id samsung_abox_vdma_driver_ids[] = {
	{
		.name = "samsung-abox-vdma",
	},
	{},
};
MODULE_DEVICE_TABLE(platform, samsung_abox_vdma_driver_ids);

static struct platform_driver samsung_abox_vdma_driver = {
	.probe  = samsung_abox_vdma_probe,
	.remove = samsung_abox_vdma_remove,
	.driver = {
		.name = "samsung-abox-vdma",
		.owner = THIS_MODULE,
	},
	.id_table = samsung_abox_vdma_driver_ids,
};

module_platform_driver(samsung_abox_vdma_driver);

#ifdef TEST
static unsigned char test_buf[4096];

static void test_work_func(struct work_struct *work)
{
	struct abox_vdma_info *info = &abox_vdma_list[2];
	static unsigned char i;
	int j;

	pr_debug("%s: %d\n", __func__, i);

	for (j = 0; j < 1024; j++, i++) {
		test_buf[i % ARRAY_SIZE(test_buf)] = i;
	}
	abox_vdma_period_elapsed(info, &info->rtd[0], i % ARRAY_SIZE(test_buf));
	abox_vdma_period_elapsed(info, &info->rtd[1], i % ARRAY_SIZE(test_buf));
	schedule_delayed_work(to_delayed_work(work), msecs_to_jiffies(1000));
}
DECLARE_DELAYED_WORK(test_work, test_work_func);

static const struct PCMTASK_HARDWARE test_hardware = {
	.name = "test01",
	.addr = 0x12345678,
	.width_min = 16,
	.width_max = 24,
	.rate_min = 48000,
	.rate_max = 192000,
	.channels_min = 2,
	.channels_max = 2,
	.buffer_bytes_max = 4096,
	.period_bytes_min = 1024,
	.period_bytes_max = 2048,
	.periods_min = 2,
	.periods_max = 4,
};
#endif

void abox_vdma_init(struct device *dev_abox)
{
	dev_info(dev_abox, "%s\n", __func__);

	abox_vdma_dev_abox = dev_abox;
	abox_register_ipc_handler(dev_abox, IPC_PCMPLAYBACK,
			abox_vdma_ipc_handler, dev_abox);
	abox_register_ipc_handler(dev_abox, IPC_PCMCAPTURE,
			abox_vdma_ipc_handler, dev_abox);
	platform_device_register_data(dev_abox,
			"samsung-abox-vdma", -1, NULL, 0);
#ifdef TEST
	abox_vdma_register(dev_abox, 102, SNDRV_PCM_STREAM_PLAYBACK, test_buf,
			virt_to_phys(test_buf), &test_hardware);
	abox_vdma_register(dev_abox, 102, SNDRV_PCM_STREAM_CAPTURE, test_buf,
			virt_to_phys(test_buf), &test_hardware);
	schedule_delayed_work(&test_work, HZ * 5);
#endif
}

/* Module information */
MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box Virtual DMA Driver");
MODULE_ALIAS("platform:samsung-abox-vdma");
MODULE_LICENSE("GPL");
