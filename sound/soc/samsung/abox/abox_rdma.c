/* sound/soc/samsung/abox/abox_rdma.c
 *
 * ALSA SoC Audio Layer - Samsung Abox RDMA driver
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
#include <sound/tlv.h>
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

#define COMPR_USE_COPY
#define COMPR_USE_FIXED_MEMORY

/* Mailbox between driver and firmware for offload */
#define COMPR_CMD_CODE		(0x0004)
#define COMPR_HANDLE_ID		(0x0008)
#define COMPR_IP_TYPE		(0x000C)
#define COMPR_SIZE_OF_FRAGMENT	(0x0010)
#define COMPR_PHY_ADDR_INBUF	(0x0014)
#define COMPR_SIZE_OF_INBUF	(0x0018)
#define COMPR_LEFT_VOL		(0x001C)
#define COMPR_RIGHT_VOL		(0x0020)
#define EFFECT_EXT_ON		(0x0024)
#define COMPR_ALPA_NOTI		(0x0028)
#define COMPR_PARAM_RATE	(0x0034)
#define COMPR_PARAM_SAMPLE	(0x0038)
#define COMPR_PARAM_CH		(0x003C)
#define COMPR_RENDERED_PCM_SIZE	(0x004C)
#define COMPR_RETURN_CMD	(0x0040)
#define COMPR_IP_ID		(0x0044)
#define COMPR_SIZE_OUT_DATA	(0x0048)
#define COMPR_UPSCALE		(0x0050)
#define COMPR_CPU_LOCK_LV	(0x0054)
#define COMPR_CHECK_CMD		(0x0058)
#define COMPR_CHECK_RUNNING	(0x005C)
#define COMPR_ACK		(0x0060)
#define COMPR_INTR_ACK		(0x0064)
#define COMPR_INTR_DMA_ACK	(0x0068)
#define COMPR_MAX		COMPR_INTR_DMA_ACK

/* COMPR_UPSCALE */
#define COMPR_BIT_SHIFT		(0)
#define COMPR_BIT_MASK		(0xFF)
#define COMPR_CH_SHIFT		(8)
#define COMPR_CH_MASK		(0xF)
#define COMPR_RATE_SHIFT	(12)
#define COMPR_RATE_MASK		(0xFFFFF)

/* Interrupt type */
#define INTR_WAKEUP		(0x0)
#define INTR_READY		(0x1000)
#define INTR_DMA		(0x2000)
#define INTR_CREATED		(0x3000)
#define INTR_DECODED		(0x4000)
#define INTR_RENDERED		(0x5000)
#define INTR_FLUSH		(0x6000)
#define INTR_PAUSED		(0x6001)
#define INTR_EOS		(0x7000)
#define INTR_DESTROY		(0x8000)
#define INTR_FX_EXT		(0x9000)
#define INTR_EFF_REQUEST	(0xA000)
#define INTR_SET_CPU_LOCK	(0xC000)
#define INTR_FW_LOG		(0xFFFF)

#define COMPRESSED_LR_VOL_MAX_STEPS     0x2000
#define DMA_VOL_FACTOR_MAX_STEPS	(0xFFFFFF)

enum SEIREN_CMDTYPE {
	/* OFFLOAD */
	CMD_COMPR_CREATE = 0x50,
	CMD_COMPR_DESTROY,
	CMD_COMPR_SET_PARAM,
	CMD_COMPR_WRITE,
	CMD_COMPR_READ,
	CMD_COMPR_START,
	CMD_COMPR_STOP,
	CMD_COMPR_PAUSE,
	CMD_COMPR_EOS,
	CMD_COMPR_GET_VOLUME,
	CMD_COMPR_SET_VOLUME,
	CMD_COMPR_CA5_WAKEUP,
	CMD_COMPR_HPDET_NOTIFY,
};

enum OFFLOAD_IPTYPE {
	COMPR_MP3 = 0x0,
	COMPR_AAC = 0x1,
	COMPR_FLAC = 0x2,
};

static const struct snd_compr_caps abox_rdma_compr_caps = {
	.direction		= SND_COMPRESS_PLAYBACK,
	.min_fragment_size	= SZ_4K,
	.max_fragment_size	= SZ_64K,
	.min_fragments		= 1,
	.max_fragments		= 5,
	.num_codecs		= 3,
	.codecs			= {
		SND_AUDIOCODEC_MP3,
		SND_AUDIOCODEC_AAC,
		SND_AUDIOCODEC_FLAC
	},
};

static void abox_rdma_mailbox_write(struct device *dev, u32 index, u32 value)
{
	struct regmap *regmap = dev_get_regmap(dev, NULL);
	int ret;

	dev_dbg(dev, "%s(0x%x, 0x%x)\n", __func__, index, value);

	if (!regmap) {
		dev_err(dev, "%s: regmap is null\n", __func__);
		return;
	}

	pm_runtime_get(dev);
	ret = regmap_write(regmap, index, value);
	if (ret < 0)
		dev_warn(dev, "%s(%u) failed: %d\n", __func__, index, ret);
	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);
}

static u32 abox_rdma_mailbox_read(struct device *dev, u32 index)
{
	struct regmap *regmap = dev_get_regmap(dev, NULL);
	int ret;
	u32 val = 0;

	dev_dbg(dev, "%s(0x%x)\n", __func__, index);

	if (!regmap) {
		dev_err(dev, "%s: regmap is null\n", __func__);
		return 0;
	}

	pm_runtime_get(dev);
	ret = regmap_read(regmap, index, &val);
	if (ret < 0)
		dev_warn(dev, "%s(%u) failed: %d\n", __func__, index, ret);
	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);

	return val;
}

static void abox_mailbox_save(struct device *dev)
{
	struct regmap *regmap = dev_get_regmap(dev, NULL);

	if (regmap) {
		regcache_cache_only(regmap, true);
		regcache_mark_dirty(regmap);
	}
}

static void abox_mailbox_restore(struct device *dev)
{
	struct regmap *regmap = dev_get_regmap(dev, NULL);

	if (regmap) {
		regcache_cache_only(regmap, false);
		regcache_sync(regmap);
	}
}

static bool abox_mailbox_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case COMPR_ACK:
	case COMPR_INTR_ACK:
	case COMPR_INTR_DMA_ACK:
	case COMPR_RETURN_CMD:
	case COMPR_SIZE_OUT_DATA:
	case COMPR_IP_ID:
	case COMPR_RENDERED_PCM_SIZE:
		return true;
	default:
		return false;
	}
}

static bool abox_mailbox_rw_reg(struct device *dev, unsigned int reg)
{
	return true;
}

static const struct regmap_config abox_mailbox_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.max_register = COMPR_MAX,
	.volatile_reg = abox_mailbox_volatile_reg,
	.readable_reg = abox_mailbox_rw_reg,
	.writeable_reg = abox_mailbox_rw_reg,
	.cache_type = REGCACHE_FLAT,
	.fast_io = true,
};

static int abox_rdma_request_ipc(struct abox_platform_data *data,
		ABOX_IPC_MSG *msg, int atomic, int sync)
{
	return abox_request_ipc(data->dev_abox, msg->ipcid, msg, sizeof(*msg),
			atomic, sync);
}

static int abox_rdma_mailbox_send_cmd(struct device *dev, unsigned int cmd)
{
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct device *dev_abox = platform_data->dev_abox;
	struct abox_compr_data *data = &platform_data->compr_data;
	ABOX_IPC_MSG msg;
	u64 timeout;
	int ret;

	dev_dbg(dev, "%s(%x)\n", __func__, cmd);

	switch (cmd) {
	case CMD_COMPR_CREATE:
		dev_dbg(dev, "%s: CMD_COMPR_CREATE %d\n", __func__, cmd);
		break;
	case CMD_COMPR_DESTROY:
		dev_dbg(dev, "%s: CMD_COMPR_DESTROY %d\n", __func__, cmd);
		break;
	case CMD_COMPR_SET_PARAM:
		dev_dbg(dev, "%s: CMD_COMPR_SET_PARAM %d\n", __func__, cmd);
#ifdef CONFIG_SND_ESA_SA_EFFECT
		abox_rdma_mailbox_write(dev, abox_data, COMPR_PARAM_RATE,
				data->out_sample_rate);
#endif
		break;
	case CMD_COMPR_WRITE:
		dev_dbg(dev, "%s: CMD_COMPR_WRITE %d\n", __func__, cmd);
		break;
	case CMD_COMPR_READ:
		dev_dbg(dev, "%s: CMD_COMPR_READ %d\n", __func__, cmd);
		break;
	case CMD_COMPR_START:
		dev_dbg(dev, "%s: CMD_COMPR_START %d\n", __func__, cmd);
		break;
	case CMD_COMPR_STOP:
		dev_dbg(dev, "%s: CMD_COMPR_STOP %d\n", __func__, cmd);
		break;
	case CMD_COMPR_PAUSE:
		dev_dbg(dev, "%s: CMD_COMPR_PAUSE %d\n", __func__, cmd);
		break;
	case CMD_COMPR_EOS:
		dev_dbg(dev, "%s: CMD_COMPR_EOS %d\n", __func__, cmd);
		break;
	case CMD_COMPR_GET_VOLUME:
		dev_dbg(dev, "%s: CMD_COMPR_GET_VOLUME %d\n", __func__, cmd);
		break;
	case CMD_COMPR_SET_VOLUME:
		dev_dbg(dev, "%s: CMD_COMPR_SET_VOLUME %d\n", __func__, cmd);
		break;
	case CMD_COMPR_CA5_WAKEUP:
		dev_dbg(dev, "%s: CMD_COMPR_CA5_WAKEUP %d\n", __func__, cmd);
		break;
	case CMD_COMPR_HPDET_NOTIFY:
		dev_dbg(dev, "%s: CMD_COMPR_HPDET_NOTIFY %d\n", __func__, cmd);
		break;
	default:
		dev_err(dev, "%s: unknown cmd %d\n", __func__, cmd);
		return -EINVAL;
	}

	mutex_lock(&data->cmd_lock);

	abox_rdma_mailbox_write(dev, COMPR_HANDLE_ID, data->handle_id);
	abox_rdma_mailbox_write(dev, COMPR_CMD_CODE, cmd);
	msg.ipcid = IPC_OFFLOAD;
	ret = abox_request_ipc(dev_abox, msg.ipcid, &msg, sizeof(msg), 0, 0);

	timeout = local_clock() + (1 * NSEC_PER_SEC);
	while (!abox_rdma_mailbox_read(dev, COMPR_ACK)) {
		if (local_clock() <= timeout) {
			cond_resched();
			continue;
		}
		dev_err(dev, "%s: No ack error!(%x)", __func__, cmd);
		ret = -EFAULT;
		break;
	}
	/* clear ACK */
	abox_rdma_mailbox_write(dev, COMPR_ACK, 0);

	mutex_unlock(&data->cmd_lock);

	return ret;
}

static void abox_rdma_compr_clear_intr_ack(struct device *dev)
{
	abox_rdma_mailbox_write(dev, COMPR_INTR_ACK, 0);
}

static int abox_rdma_compr_isr_handler(void *priv)
{
	struct platform_device *pdev = priv;
	struct device *dev = &pdev->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct abox_compr_data *data = &platform_data->compr_data;
	int id = platform_data->id;
	unsigned long flags;
	u32 val, fw_stat;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	val = abox_rdma_mailbox_read(dev, COMPR_RETURN_CMD);

	if (val == 1)
		dev_err(dev, "%s: There is possibility of firmware CMD fail %u\n",
						__func__, val);
	fw_stat = val >> 16;
	dev_dbg(dev, "fw_stat(%08x), val(%08x)\n", fw_stat, val);

	switch (fw_stat) {
	case INTR_CREATED:
		dev_info(dev, "INTR_CREATED\n");
		abox_rdma_compr_clear_intr_ack(dev);
		data->created = true;
		break;
	case INTR_DECODED:
		/* check the error */
		val &= 0xFF;
		if (val) {
			dev_err(dev, "INTR_DECODED err(%d)\n", val);
		} else if (data->cstream && data->cstream->runtime) {
			/* update copied total bytes */
			u32 size = abox_rdma_mailbox_read(dev,
					COMPR_SIZE_OUT_DATA);
			struct snd_compr_stream *cstream = data->cstream;
			struct snd_compr_runtime *runtime = cstream->runtime;

			dev_dbg(dev, "INTR_DECODED(%d)\n", size);

			spin_lock_irqsave(&data->lock, flags);
			/* update copied total bytes */
			data->copied_total += size;
			data->byte_offset += size;
			if (data->byte_offset >= runtime->buffer_size)
				data->byte_offset -= runtime->buffer_size;
			spin_unlock_irqrestore(&data->lock, flags);

			snd_compr_fragment_elapsed(data->cstream);

			if (!data->start &&
				runtime->state != SNDRV_PCM_STATE_PAUSED) {
				/* Writes must be restarted from _copy() */
				dev_err(dev, "%s: write_done received while not started(%d)",
					__func__, runtime->state);
			} else {
				u64 bytes_available = data->received_total -
						data->copied_total;

				dev_dbg(dev, "%s: current free bufsize(%llu)\n",
						__func__, runtime->buffer_size -
						bytes_available);

				if (bytes_available < runtime->fragment_size) {
					dev_dbg(dev, "%s: WRITE_DONE Insufficient data to send.(avail:%llu)\n",
						__func__, bytes_available);
				}
			}
		} else {
			dev_dbg(dev, "%s: INTR_DECODED after compress offload end\n",
					__func__);
		}
		abox_rdma_compr_clear_intr_ack(dev);
		break;
	case INTR_FLUSH:
		/* check the error */
		val &= 0xFF;
		if (val) {
			dev_err(dev, "INTR_FLUSH err(%d)\n", val);
		} else {
			/* flush done */
			data->stop_ack = 1;
			wake_up_interruptible(&data->flush_wait);
		}
		abox_rdma_compr_clear_intr_ack(dev);
		break;
	case INTR_PAUSED:
		/* check the error */
		val &= 0xFF;
		if (val)
			dev_err(dev, "INTR_PAUSED err(%d)\n", val);

		abox_rdma_compr_clear_intr_ack(dev);
		break;
	case INTR_EOS:
		if (data->eos) {
			if (data->copied_total != data->received_total)
				dev_err(dev, "%s: EOS is not sync!(%llu/%llu)\n",
						__func__, data->copied_total,
						data->received_total);

			/* ALSA Framework callback to notify drain complete */
			snd_compr_drain_notify(data->cstream);
			data->eos = 0;
			dev_info(dev, "%s: DATA_CMD_EOS wake up\n", __func__);
		}
		abox_rdma_compr_clear_intr_ack(dev);
		break;
	case INTR_DESTROY:
		/* check the error */
		val &= 0xFF;
		if (val) {
			dev_err(dev, "INTR_DESTROY err(%d)\n", val);
		} else {
			/* destroied */
			data->exit_ack = 1;
			wake_up_interruptible(&data->exit_wait);
		}
		abox_rdma_compr_clear_intr_ack(dev);
		break;
	case INTR_FX_EXT:
		/* To Do */
		abox_rdma_compr_clear_intr_ack(dev);
		break;
#ifdef CONFIG_SND_SAMSUNG_ELPE
	case INTR_EFF_REQUEST:
		/*To Do */
		abox_rdma_compr_clear_intr_ack(dev);
		break;
#endif
	}

	wake_up_interruptible(&data->ipc_wait);

	return IRQ_HANDLED;
}

static int abox_rdma_compr_set_param(struct platform_device *pdev,
		struct snd_compr_runtime *runtime)
{
	struct device *dev = &pdev->dev;
	struct abox_platform_data *platform_data = platform_get_drvdata(pdev);
	struct abox_compr_data *data = &platform_data->compr_data;
	int id = platform_data->id;
	int ret;

	dev_info(dev, "%s[%d] buffer: %llu\n", __func__, id,
			runtime->buffer_size);

#ifdef COMPR_USE_FIXED_MEMORY
	/* free memory allocated by ALSA */
	if (runtime->buffer != data->dma_area)
		kfree(runtime->buffer);

	runtime->buffer = data->dma_area;
	if (runtime->buffer_size > data->dma_size) {
		dev_err(dev, "allocated buffer size is smaller than requested(%llu > %zu)\n",
				runtime->buffer_size, data->dma_size);
		ret = -ENOMEM;
		goto error;
	}
#else
#ifdef COMPR_USE_COPY
	runtime->buffer = dma_alloc_coherent(dev, runtime->buffer_size,
			&data->dma_addr, GFP_KERNEL);
	if (!runtime->buffer) {
		dev_err(dev, "dma memory allocation failed (size=%llu)\n",
				runtime->buffer_size);
		ret = -ENOMEM;
		goto error;
	}
#else
	data->dma_addr = dma_map_single(dev, runtime->buffer,
			runtime->buffer_size, DMA_TO_DEVICE);
	ret = dma_mapping_error(dev, data->dma_addr);
	if (ret) {
		dev_err(dev, "dma memory mapping failed(%d)\n", ret);
		goto error;
	}
#endif
	ret = abox_iommu_map(&platform_data->abox_data->pdev->dev,
			IOVA_COMPR_BUFFER(id), virt_to_phys(runtime->buffer),
			round_up(runtime->buffer_size, PAGE_SIZE), 0);
	if (ret < 0) {
		dev_err(dev, "iommu mapping failed(%d)\n", ret);
		goto error;
	}
#endif
	/* set buffer information at mailbox */
	abox_rdma_mailbox_write(dev, COMPR_SIZE_OF_INBUF, runtime->buffer_size);
	abox_rdma_mailbox_write(dev, COMPR_PHY_ADDR_INBUF,
			IOVA_COMPR_BUFFER(id));
	abox_rdma_mailbox_write(dev, COMPR_PARAM_SAMPLE, data->sample_rate);
	abox_rdma_mailbox_write(dev, COMPR_PARAM_CH, data->channels);
	abox_rdma_mailbox_write(dev, COMPR_IP_TYPE, data->codec_id << 16);
	data->created = false;
	ret = abox_rdma_mailbox_send_cmd(dev, CMD_COMPR_SET_PARAM);
	if (ret < 0) {
		dev_err(dev, "CMD_COMPR_SET_PARAM failed(%d)\n", ret);
		goto error;
	}

	/* wait until the parameter is set up */
	ret = wait_event_interruptible_timeout(data->ipc_wait,
			data->created, msecs_to_jiffies(1000));
	if (!ret) {
		dev_err(dev, "%s: compress set param timed out!!! (%d)\n",
			__func__, ret);
		abox_rdma_mailbox_write(dev, COMPR_INTR_ACK, 0);
		ret = -EBUSY;
		goto error;
	}

	/* created instance */
	data->handle_id = abox_rdma_mailbox_read(dev, COMPR_IP_ID);
	dev_info(dev, "%s: codec id:0x%x, ret_val:0x%x, handle_id:0x%x\n",
		__func__, data->codec_id,
		abox_rdma_mailbox_read(dev, COMPR_RETURN_CMD),
		data->handle_id);

	dev_info(dev, "%s: allocated buffer address (0x%pad), size(0x%llx)\n",
		__func__, &data->dma_addr, runtime->buffer_size);
#ifdef CONFIG_SND_ESA_SA_EFFECT
	data->effect_on = false;
#endif

	return 0;

error:
	return ret;
}

static int abox_rdma_compr_open(struct snd_compr_stream *stream)
{
	struct snd_soc_pcm_runtime *rtd = stream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct abox_compr_data *data = &platform_data->compr_data;
	struct abox_data *abox_data = platform_data->abox_data;
	int id = platform_data->id;

	dev_info(dev, "%s[%d]\n", __func__, id);

	abox_wait_restored(abox_data);

	/* init runtime data */
	data->cstream = stream;
	data->byte_offset = 0;
	data->copied_total = 0;
	data->received_total = 0;
	data->sample_rate = 44100;
	data->channels = 0x3; /* stereo channel mask */

	data->eos = false;
	data->start = false;
	data->created = false;
	data->bespoke_start = false;

	pm_runtime_get_sync(dev);
	abox_request_cpu_gear_dai(dev, abox_data, rtd->cpu_dai,
			abox_data->cpu_gear_min);

	return 0;
}

static int abox_rdma_compr_free(struct snd_compr_stream *stream)
{
	struct snd_soc_pcm_runtime *rtd = stream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct abox_compr_data *data = &platform_data->compr_data;
	struct abox_data *abox_data = platform_data->abox_data;
	int id = platform_data->id;
	int ret = 0;

	dev_info(dev, "%s[%d]\n", __func__, id);

	if (data->eos) {
		/* ALSA Framework callback to notify drain complete */
		snd_compr_drain_notify(stream);
		data->eos = 0;
		dev_dbg(dev, "%s Call Drain notify to wakeup\n", __func__);
	}

	if (data->created) {
		data->created = false;
		data->exit_ack = 0;

		ret = abox_rdma_mailbox_send_cmd(dev, CMD_COMPR_DESTROY);
		if (ret) {
			dev_err(dev, "%s: can't send CMD_COMPR_DESTROY (%d)\n",
					__func__, ret);
		} else {
			ret = wait_event_interruptible_timeout(data->exit_wait,
					data->exit_ack, msecs_to_jiffies(1000));
			if (!ret)
				dev_err(dev, "%s: CMD_DESTROY timed out!!!\n",
						__func__);
		}
	}

#ifdef COMPR_USE_FIXED_MEMORY
	/* prevent kfree in ALSA */
	stream->runtime->buffer = NULL;
#else
{
	struct snd_compr_runtime *runtime = stream->runtime;

	abox_iommu_unmap(&abox_data->pdev->dev, IOVA_COMPR_BUFFER(id));

#ifdef COMPR_USE_COPY
	dma_free_coherent(dev, runtime->buffer_size, runtime->buffer,
			data->dma_addr);
	runtime->buffer = NULL;
#else
	dma_unmap_single(dev, data->dma_addr, runtime->buffer_size,
			DMA_TO_DEVICE);
#endif
}
#endif
	abox_request_cpu_gear_dai(dev, abox_data, rtd->cpu_dai, 0);
	pm_runtime_mark_last_busy(dev);
	pm_runtime_put(dev);

	return ret;
}

static int abox_rdma_compr_set_params(struct snd_compr_stream *stream,
			    struct snd_compr_params *params)
{
	struct snd_compr_runtime *runtime = stream->runtime;
	struct snd_soc_pcm_runtime *rtd = stream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct platform_device *pdev = to_platform_device(dev);
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct abox_compr_data *data = &platform_data->compr_data;
	int id = platform_data->id;
	int ret = 0;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	/* COMPR set_params */
	memcpy(&data->codec_param, params, sizeof(data->codec_param));

	data->byte_offset = 0;
	data->copied_total = 0;
	data->channels = data->codec_param.codec.ch_in;
	data->sample_rate = data->codec_param.codec.sample_rate;

	if (data->sample_rate == 0 ||
		data->channels == 0) {
		dev_err(dev, "%s: invalid parameters: sample(%u), ch(%u)\n",
			__func__, data->sample_rate, data->channels);
		return -EINVAL;
	}

	switch (params->codec.id) {
	case SND_AUDIOCODEC_MP3:
		data->codec_id = COMPR_MP3;
		break;
	case SND_AUDIOCODEC_AAC:
		data->codec_id = COMPR_AAC;
		break;
	case SND_AUDIOCODEC_FLAC:
		data->codec_id = COMPR_FLAC;
		break;
	default:
		dev_err(dev, "%s: unknown codec id %d\n", __func__,
				params->codec.id);
		break;
	}

	ret = abox_rdma_compr_set_param(pdev, runtime);
	if (ret) {
		dev_err(dev, "%s: esa_compr_set_param fail(%d)\n", __func__,
				ret);
		return ret;
	}

	dev_info(dev, "%s: sample rate:%u, channels:%u\n", __func__,
		data->sample_rate, data->channels);
	return 0;
}

static int abox_rdma_compr_set_metadata(struct snd_compr_stream *stream,
			      struct snd_compr_metadata *metadata)
{
	struct snd_soc_pcm_runtime *rtd = stream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	int id = platform_data->id;

	dev_info(dev, "%s[%d]\n", __func__, id);

	if (!metadata)
		return -EINVAL;

	if (metadata->key == SNDRV_COMPRESS_ENCODER_PADDING)
		dev_dbg(dev, "%s: got encoder padding %u", __func__,
				metadata->value[0]);
	else if (metadata->key == SNDRV_COMPRESS_ENCODER_DELAY)
		dev_dbg(dev, "%s: got encoder delay %u", __func__,
				metadata->value[0]);

	return 0;
}

static int abox_rdma_compr_trigger(struct snd_compr_stream *stream, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = stream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct abox_compr_data *data = &platform_data->compr_data;
	int id = platform_data->id;
	int ret = 0;

	dev_info(dev, "%s[%d](%d)\n", __func__, id, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		dev_info(dev, "SNDRV_PCM_TRIGGER_PAUSE_PUSH\n");
		ret = abox_rdma_mailbox_send_cmd(dev, CMD_COMPR_PAUSE);
		if (ret < 0)
			dev_err(dev, "%s: pause cmd failed(%d)\n", __func__,
					ret);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		dev_info(dev, "SNDRV_PCM_TRIGGER_STOP\n");

		if (data->eos) {
			/* ALSA Framework callback to notify drain complete */
			snd_compr_drain_notify(stream);
			data->eos = 0;
			dev_dbg(dev, "interrupt drain and eos wait queues\n");
		}

		data->stop_ack = 0;
		ret = abox_rdma_mailbox_send_cmd(dev, CMD_COMPR_STOP);
		if (ret < 0)
			dev_err(dev, "%s: stop cmd failed (%d)\n",
				__func__, ret);

		ret = wait_event_interruptible_timeout(data->flush_wait,
			data->stop_ack, msecs_to_jiffies(1000));
		if (!ret) {
			dev_err(dev, "CMD_STOP cmd timeout(%d)\n", ret);
			ret = -ETIMEDOUT;
		} else {
			ret = 0;
		}

		data->start = false;
		data->bespoke_start = false;

		/* reset */
		data->stop_ack = 0;
		data->byte_offset = 0;
		data->copied_total = 0;
		data->received_total = 0;
		break;
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		dev_info(dev, "%s: %s", __func__,
				(cmd == SNDRV_PCM_TRIGGER_START) ?
				"SNDRV_PCM_TRIGGER_START" :
				"SNDRV_PCM_TRIGGER_PAUSE_RELEASE");

		data->start = 1;
		data->bespoke_start = true;
		data->dirty = false;
		ret = abox_rdma_mailbox_send_cmd(dev, CMD_COMPR_START);
		if (ret < 0)
			dev_err(dev, "%s: start cmd failed\n", __func__);

		break;
	case SND_COMPR_TRIGGER_NEXT_TRACK:
		pr_info("%s: SND_COMPR_TRIGGER_NEXT_TRACK\n", __func__);
		break;
	case SND_COMPR_TRIGGER_PARTIAL_DRAIN:
	case SND_COMPR_TRIGGER_DRAIN:
		dev_info(dev, "%s: %s", __func__,
				(cmd == SND_COMPR_TRIGGER_DRAIN) ?
				"SND_COMPR_TRIGGER_DRAIN" :
				"SND_COMPR_TRIGGER_PARTIAL_DRAIN");
		/* Make sure all the data is sent to F/W before sending EOS */
		if (!data->start) {
			dev_err(dev, "%s: stream is not in started state\n",
				__func__);
			ret = -EPERM;
			break;
		}

		data->eos = 1;
		dev_dbg(dev, "%s: CMD_EOS\n", __func__);
		ret = abox_rdma_mailbox_send_cmd(dev, CMD_COMPR_EOS);
		if (ret < 0)
			dev_err(dev, "%s: can't send eos (%d)\n", __func__,
					ret);
		else
			pr_info("%s: Out of %s Drain", __func__,
					(cmd == SND_COMPR_TRIGGER_DRAIN ?
					"Full" : "Partial"));

		break;
	default:
		break;
	}

	return 0;
}

static int abox_rdma_compr_pointer(struct snd_compr_stream *stream,
			 struct snd_compr_tstamp *tstamp)
{
	struct snd_soc_pcm_runtime *rtd = stream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct abox_compr_data *data = &platform_data->compr_data;
	int id = platform_data->id;
	unsigned int num_channel;
	u32 pcm_size;
	unsigned long flags;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	spin_lock_irqsave(&data->lock, flags);
	tstamp->sampling_rate = data->sample_rate;
	tstamp->byte_offset = data->byte_offset;
	tstamp->copied_total = data->copied_total;
	spin_unlock_irqrestore(&data->lock, flags);

	pcm_size = abox_rdma_mailbox_read(dev, COMPR_RENDERED_PCM_SIZE);

	/* set the number of channels */
	num_channel = hweight32(data->channels);

	if (pcm_size) {
		tstamp->pcm_io_frames = pcm_size / (2 * num_channel);
		dev_dbg(dev, "%s: pcm_size(%u), frame_count(%u), copied_total(%u)\n",
				__func__, pcm_size, tstamp->pcm_io_frames,
				tstamp->copied_total);

	}

	return 0;
}

static int abox_rdma_compr_mmap(struct snd_compr_stream *stream,
		struct vm_area_struct *vma)
{
	struct snd_soc_pcm_runtime *rtd = stream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	int id = platform_data->id;
	struct snd_compr_runtime *runtime = stream->runtime;

	dev_info(dev, "%s[%d]\n", __func__, id);

	return dma_mmap_writecombine(dev, vma,
			runtime->buffer,
			virt_to_phys(runtime->buffer),
			runtime->buffer_size);
}

static int abox_rdma_compr_ack(struct snd_compr_stream *stream, size_t bytes)
{
	struct snd_soc_pcm_runtime *rtd = stream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct abox_compr_data *data = &platform_data->compr_data;
	int id = platform_data->id;
	int ret;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	/* write mp3 data to firmware */
	data->received_total += bytes;
	abox_rdma_mailbox_write(dev, COMPR_SIZE_OF_FRAGMENT, bytes);
	ret = abox_rdma_mailbox_send_cmd(dev, CMD_COMPR_WRITE);

	return ret;
}

#ifdef COMPR_USE_COPY
static int abox_compr_write_data(struct snd_compr_stream *stream,
	       const char __user *buf, size_t count)
{
	void *dstn;
	size_t copy;
	struct snd_compr_runtime *runtime = stream->runtime;
	/* 64-bit Modulus */
	u64 app_pointer = div64_u64(runtime->total_bytes_available,
				    runtime->buffer_size);
	app_pointer = runtime->total_bytes_available -
		      (app_pointer * runtime->buffer_size);
	dstn = runtime->buffer + app_pointer;

	pr_debug("copying %ld at %lld\n",
			(unsigned long)count, app_pointer);

	if (count < runtime->buffer_size - app_pointer) {
		if (copy_from_user(dstn, buf, count))
			return -EFAULT;
	} else {
		copy = runtime->buffer_size - app_pointer;
		if (copy_from_user(dstn, buf, copy))
			return -EFAULT;
		if (copy_from_user(runtime->buffer, buf + copy, count - copy))
			return -EFAULT;
	}
	abox_rdma_compr_ack(stream, count);

	return count;
}

static int abox_rdma_compr_copy(struct snd_compr_stream *stream,
		char __user *buf, size_t count)
{
	struct snd_soc_pcm_runtime *rtd = stream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	int id = platform_data->id;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	return abox_compr_write_data(stream, buf, count);
}
#endif

static int abox_rdma_compr_get_caps(struct snd_compr_stream *stream,
		struct snd_compr_caps *caps)
{
	struct snd_soc_pcm_runtime *rtd = stream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	int id = platform_data->id;

	dev_info(dev, "%s[%d]\n", __func__, id);

	memcpy(caps, &abox_rdma_compr_caps, sizeof(*caps));

	return 0;
}

static int abox_rdma_compr_get_codec_caps(struct snd_compr_stream *stream,
		struct snd_compr_codec_caps *codec)
{
	struct snd_soc_pcm_runtime *rtd = stream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	int id = platform_data->id;

	dev_info(dev, "%s[%d]\n", __func__, id);

	return 0;
}

static void __maybe_unused __abox_rdma_compr_get_hw_params_legacy(
		struct device *dev, struct snd_pcm_hw_params *params,
		unsigned int upscale)
{
	struct snd_mask *format_mask;
	struct snd_interval *rate_interval;

	rate_interval = hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE);
	format_mask = hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT);
	switch (upscale) {
	default:
		/* fallback */
	case 0:
		/* 48kHz 16bit */
		rate_interval->min = 48000;
		snd_mask_set(format_mask, SNDRV_PCM_FORMAT_S16);
		dev_info(dev, "%s: 48kHz 16bit\n", __func__);
		break;
	case 1:
		/* 192kHz 24bit */
		rate_interval->min = 192000;
		snd_mask_set(format_mask, SNDRV_PCM_FORMAT_S24);
		dev_info(dev, "%s: 192kHz 24bit\n", __func__);
		break;
	case 2:
		/* 48kHz 24bit */
		rate_interval->min = 48000;
		snd_mask_set(format_mask, SNDRV_PCM_FORMAT_S24);
		dev_info(dev, "%s: 48kHz 24bit\n", __func__);
		break;
	case 3:
		/* 384kHz 32bit */
		rate_interval->min = 384000;
		snd_mask_set(format_mask, SNDRV_PCM_FORMAT_S32);
		dev_info(dev, "%s: 384kHz 32bit\n", __func__);
		break;
	}
}

static void __maybe_unused __abox_rdma_compr_get_hw_params(struct device *dev,
		struct snd_pcm_hw_params *params, unsigned int upscale)
{
	unsigned int bit, ch, rate;
	struct snd_mask *format_mask;
	struct snd_interval *rate_interval, *ch_interval;

	format_mask = hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT);
	ch_interval = hw_param_interval(params, SNDRV_PCM_HW_PARAM_CHANNELS);
	rate_interval = hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE);
	bit = (upscale >> COMPR_BIT_SHIFT) & COMPR_BIT_MASK;
	ch = (upscale >> COMPR_CH_SHIFT) & COMPR_CH_MASK;
	rate = (upscale >> COMPR_RATE_SHIFT) & COMPR_RATE_MASK;

	switch (bit) {
	default:
		/* fallback */
	case 16:
		snd_mask_set(format_mask, SNDRV_PCM_FORMAT_S16);
		break;
	case 24:
		snd_mask_set(format_mask, SNDRV_PCM_FORMAT_S24);
		break;
	case 32:
		snd_mask_set(format_mask, SNDRV_PCM_FORMAT_S32);
		break;
	}
	ch_interval->min = ch ? ch : 2;
	rate_interval->min = rate ? rate : 48000;

	dev_info(dev, "%s: %ubit %uch %uHz\n", __func__, bit, ch, rate);
}

static void __abox_rdma_compr_set_hw_params_legacy(struct device *dev,
		struct snd_pcm_hw_params *params)
{
	struct abox_platform_data *pdata = dev_get_drvdata(dev);
	struct abox_compr_data *cdata = &pdata->compr_data;
	unsigned int width, rate;
	unsigned int upscale;

	rate = params_rate(params);
	width = params_width(params);

	if (rate <= 48000) {
		if (rate != 48000)
			dev_warn(dev, "unsupported offload rate: %u\n", rate);

		if (width >= 24) {
			upscale = 2;
			dev_info(dev, "%s: 48kHz 24bit\n", __func__);
		} else {
			upscale = 0;
			dev_info(dev, "%s: 48kHz 16bit\n", __func__);
		}
	} else {
		if (rate != 192000)
			dev_warn(dev, "unsupported offload rate: %u\n", rate);

		upscale = 1;
		dev_info(dev, "%s: 192kHz 24bit\n", __func__);
	}

	if (abox_rdma_mailbox_read(dev, COMPR_UPSCALE) != upscale) {
		abox_rdma_mailbox_write(dev, COMPR_UPSCALE, upscale);
		cdata->dirty = true;
	}
}

static void __abox_rdma_compr_set_hw_params(struct device *dev,
		struct snd_pcm_hw_params *params)
{
	struct abox_platform_data *pdata = dev_get_drvdata(dev);
	struct abox_compr_data *cdata = &pdata->compr_data;
	unsigned int bit, ch, rate, upscale;

	bit = params_width(params);
	ch = params_channels(params);
	rate = params_rate(params);

	upscale = (bit >> COMPR_BIT_SHIFT) & COMPR_BIT_MASK;
	upscale |= (ch >> COMPR_CH_SHIFT) & COMPR_CH_MASK;
	upscale |= (rate >> COMPR_RATE_SHIFT) & COMPR_RATE_MASK;

	if (abox_rdma_mailbox_read(dev, COMPR_UPSCALE) != upscale) {
		abox_rdma_mailbox_write(dev, COMPR_UPSCALE, upscale);
		cdata->dirty = true;
	}

	dev_info(dev, "%s: %ubit %uch %uHz\n", __func__, bit, ch, rate);
}

static int abox_rdma_compr_get_hw_params(struct snd_compr_stream *stream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = stream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	int id = platform_data->id;
	unsigned int upscale;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	abox_hw_params_fixup_helper(rtd, params, stream->direction);

	upscale = abox_rdma_mailbox_read(dev, COMPR_UPSCALE);
	if (upscale <= 0xF) {
		__abox_rdma_compr_set_hw_params_legacy(dev, params);
		upscale = abox_rdma_mailbox_read(dev, COMPR_UPSCALE);
		__abox_rdma_compr_get_hw_params_legacy(dev, params, upscale);
	} else {
		__abox_rdma_compr_set_hw_params(dev, params);
		upscale = abox_rdma_mailbox_read(dev, COMPR_UPSCALE);
		__abox_rdma_compr_get_hw_params(dev, params, upscale);
	}

	return 0;
}

static struct snd_compr_ops abox_rdma_compr_ops = {
	.open		= abox_rdma_compr_open,
	.free		= abox_rdma_compr_free,
	.set_params	= abox_rdma_compr_set_params,
	.set_metadata	= abox_rdma_compr_set_metadata,
	.trigger	= abox_rdma_compr_trigger,
	.pointer	= abox_rdma_compr_pointer,
#ifdef COMPR_USE_COPY
	.copy		= abox_rdma_compr_copy,
#endif
	.mmap		= abox_rdma_compr_mmap,
	.ack		= abox_rdma_compr_ack,
	.get_caps	= abox_rdma_compr_get_caps,
	.get_codec_caps	= abox_rdma_compr_get_codec_caps,
	.get_hw_params	= abox_rdma_compr_get_hw_params,
};

static int abox_rdma_compr_vol_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_soc_kcontrol_platform(kcontrol);
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
			(struct soc_mixer_control *)kcontrol->private_value;
	int id = platform_data->id;
	unsigned int volumes[2];

	volumes[0] = (unsigned int)ucontrol->value.integer.value[0];
	volumes[1] = (unsigned int)ucontrol->value.integer.value[1];
	dev_dbg(dev, "%s[%d]: left_vol=%d right_vol=%d\n",
			__func__, id, volumes[0], volumes[1]);

	abox_rdma_mailbox_write(dev, mc->reg, volumes[0]);
	abox_rdma_mailbox_write(dev, mc->rreg, volumes[1]);

	return 0;
}

static int abox_rdma_compr_vol_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_soc_kcontrol_platform(kcontrol);
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
			(struct soc_mixer_control *)kcontrol->private_value;
	int id = platform_data->id;
	unsigned int volumes[2];

	volumes[0] = abox_rdma_mailbox_read(dev, mc->reg);
	volumes[1] = abox_rdma_mailbox_read(dev, mc->rreg);
	dev_dbg(dev, "%s[%d]: left_vol=%d right_vol=%d\n",
			__func__, id, volumes[0], volumes[1]);

	ucontrol->value.integer.value[0] = volumes[0];
	ucontrol->value.integer.value[1] = volumes[1];

	return 0;
}

static const DECLARE_TLV_DB_LINEAR(abox_rdma_compr_vol_gain, 0,
		COMPRESSED_LR_VOL_MAX_STEPS);

static int abox_rdma_compr_format_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_soc_kcontrol_platform(kcontrol);
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int id = platform_data->id;
	unsigned int upscale;

	dev_warn(dev, "%s is deprecated\n", kcontrol->id.name);

	upscale = ucontrol->value.enumerated.item[0];
	dev_dbg(dev, "%s[%d]: scale=%u\n", __func__, id, upscale);

	abox_rdma_mailbox_write(dev, e->reg, upscale);

	return 0;
}

static int abox_rdma_compr_format_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_soc_kcontrol_platform(kcontrol);
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int id = platform_data->id;
	unsigned int upscale;

	dev_warn(dev, "%s is deprecated\n", kcontrol->id.name);

	upscale = abox_rdma_mailbox_read(dev, e->reg);
	dev_dbg(dev, "%s[%d]: upscale=%u\n", __func__, id, upscale);

	if (upscale >= e->items) {
		unsigned int bit, rate;

		bit = (upscale >> COMPR_BIT_SHIFT) & COMPR_BIT_MASK ;
		rate = (upscale >> COMPR_RATE_SHIFT) & COMPR_RATE_MASK;

		if (rate == 384000) {
			upscale = 3;
		} else if (rate == 192000) {
			upscale = 1;
		} else if (rate == 48000) {
			if (bit == 24)
				upscale = 2;
			else
				upscale = 0;
		} else {
			upscale = 0;
		}
	}

	ucontrol->value.enumerated.item[0] = upscale;

	return 0;
}

static const char * const abox_rdma_compr_format_text[] = {
	"48kHz 16bit",
	"192kHz 24bit",
	"48kHz 24bit",
	"384kHz 32bit",
};

static SOC_ENUM_SINGLE_DECL(abox_rdma_compr_format,
		COMPR_UPSCALE, 0,
		abox_rdma_compr_format_text);

static const struct snd_kcontrol_new abox_rdma_compr_controls[] = {
	SOC_DOUBLE_R_EXT_TLV("ComprTx0 Volume", COMPR_LEFT_VOL, COMPR_RIGHT_VOL,
			0, COMPRESSED_LR_VOL_MAX_STEPS, 0,
			abox_rdma_compr_vol_get, abox_rdma_compr_vol_put,
			abox_rdma_compr_vol_gain),
	SOC_ENUM_EXT("ComprTx0 Format", abox_rdma_compr_format,
			abox_rdma_compr_format_get, abox_rdma_compr_format_put),
	SOC_SINGLE("ComprTx0 Bit", COMPR_UPSCALE, COMPR_BIT_SHIFT,
			COMPR_BIT_MASK, 0),
	SOC_SINGLE("ComprTx0 Ch", COMPR_UPSCALE, COMPR_CH_SHIFT,
			COMPR_CH_MASK, 0),
	SOC_SINGLE("ComprTx0 Rate", COMPR_UPSCALE, COMPR_RATE_SHIFT,
			COMPR_RATE_MASK, 0),
};

static const struct snd_pcm_hardware abox_rdma_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED
				| SNDRV_PCM_INFO_BLOCK_TRANSFER
				| SNDRV_PCM_INFO_MMAP
				| SNDRV_PCM_INFO_MMAP_VALID,
	.formats		= ABOX_SAMPLE_FORMATS,
	.channels_min		= 1,
	.channels_max		= 8,
	.buffer_bytes_max	= BUFFER_BYTES_MAX,
	.period_bytes_min	= PERIOD_BYTES_MIN,
	.period_bytes_max	= PERIOD_BYTES_MAX,
	.periods_min		= BUFFER_BYTES_MAX / PERIOD_BYTES_MAX,
	.periods_max		= BUFFER_BYTES_MAX / PERIOD_BYTES_MIN,
};

static irqreturn_t abox_rdma_ipc_handler(int ipc, void *dev_id,
		ABOX_IPC_MSG *msg)
{
	struct abox_data *abox_data = dev_id;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg->msg.pcmtask;
	int id = pcmtask_msg->channel_id;
	struct abox_platform_data *data;
	struct device *dev;

	if (id >= ARRAY_SIZE(abox_data->pdev_rdma) || !abox_data->pdev_rdma[id])
		return IRQ_NONE;

	dev = &abox_data->pdev_rdma[id]->dev;
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

static int abox_rdma_progress(struct abox_platform_data *data)
{
	unsigned int val = 0;

	regmap_read(data->abox_data->regmap, ABOX_RDMA_STATUS(data->id), &val);

	return !!(val & ABOX_RDMA_PROGRESS_MASK);
}

static void abox_rdma_disable_barrier(struct device *dev,
		struct abox_platform_data *data)
{
	int id = data->id;
	struct abox_data *abox_data = data->abox_data;
	u64 timeout = local_clock() + ABOX_DMA_TIMEOUT_NS;

	while (abox_rdma_progress(data)) {
		if (local_clock() <= timeout) {
			cond_resched();
			continue;
		}
		dev_warn_ratelimited(dev, "RDMA disable timeout[%d]\n", id);
		abox_dbg_dump_simple(dev, abox_data, "RDMA disable timeout");
		break;
	}
}

static int abox_rdma_hw_params(struct snd_pcm_substream *substream,
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
	unsigned int freq;
	int sbank_size, burst_len;
	int ret;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	if (data->buf_type == BUFFER_TYPE_DMA) {
		if (data->dmab.bytes < buffer_bytes) {
			abox_iommu_unmap(dev_abox, IOVA_RDMA_BUFFER(id));
			snd_dma_free_pages(&data->dmab);
			ret = snd_dma_alloc_pages(SNDRV_DMA_TYPE_DEV,
					dev,
					buffer_bytes,
					&data->dmab);
			if (ret < 0)
				return ret;
			ret = abox_iommu_map(dev_abox, IOVA_RDMA_BUFFER(id),
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

	sbank_size = abox_cmpnt_adjust_sbank(data->cmpnt,
			rtd->cpu_dai->id, params);
	if (sbank_size > 0 && sbank_size < SZ_128)
		burst_len = 0x1;
	else
		burst_len = 0x8;
	ret = snd_soc_component_update_bits(data->cmpnt,
			ABOX_RDMA_CTRL0(id),
			ABOX_RDMA_BURST_LEN_MASK,
			burst_len << ABOX_RDMA_BURST_LEN_L);
	if (ret < 0)
		dev_err(dev, "burst length write error: %d\n", ret);

	pcmtask_msg->channel_id = id;
	msg.ipcid = IPC_PCMPLAYBACK;
	msg.task_id = pcmtask_msg->channel_id = id;

	pcmtask_msg->msgtype = PCM_SET_BUFFER;
	pcmtask_msg->param.setbuff.phyaddr = IOVA_RDMA_BUFFER(id);
	pcmtask_msg->param.setbuff.size = params_period_bytes(params);
	pcmtask_msg->param.setbuff.count = params_periods(params);
	ret = abox_rdma_request_ipc(data, &msg, 0, 0);
	if (ret < 0)
		return ret;

	pcmtask_msg->msgtype = PCM_PLTDAI_HW_PARAMS;
	pcmtask_msg->param.hw_params.sample_rate = params_rate(params);
	pcmtask_msg->param.hw_params.bit_depth = params_width(params);
	pcmtask_msg->param.hw_params.channels = params_channels(params);
	ret = abox_rdma_request_ipc(data, &msg, 0, 0);
	if (ret < 0)
		return ret;

	if (params_rate(params) > 48000)
		abox_request_cpu_gear_dai(dev, abox_data, rtd->cpu_dai,
				abox_data->cpu_gear_min - 1);

	freq = data->pm_qos_cl0[abox_get_rate_type(params_rate(params))];
	abox_request_cl0_freq_dai(dev, rtd->cpu_dai, freq);
	freq = data->pm_qos_cl1[abox_get_rate_type(params_rate(params))];
	abox_request_cl1_freq_dai(dev, rtd->cpu_dai, freq);
	freq = data->pm_qos_cl2[abox_get_rate_type(params_rate(params))];
	abox_request_cl2_freq_dai(dev, rtd->cpu_dai, freq);

	dev_info(dev, "%s:Total=%zu PrdSz=%u(%u) #Prds=%u rate=%u, width=%d, channels=%u\n",
			snd_pcm_stream_str(substream), runtime->dma_bytes,
			params_period_size(params), params_period_bytes(params),
			params_periods(params), params_rate(params),
			params_width(params), params_channels(params));

	return 0;
}

static int abox_rdma_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	int id = data->id;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	msg.ipcid = IPC_PCMPLAYBACK;
	pcmtask_msg->msgtype = PCM_PLTDAI_HW_FREE;
	msg.task_id = pcmtask_msg->channel_id = id;
	abox_rdma_request_ipc(data, &msg, 0, 0);
	abox_request_cl0_freq_dai(dev, rtd->cpu_dai, 0);
	abox_request_cl1_freq_dai(dev, rtd->cpu_dai, 0);
	abox_request_cl2_freq_dai(dev, rtd->cpu_dai, 0);

	switch (data->type) {
	default:
		abox_rdma_disable_barrier(dev, data);
		break;
	}

	snd_pcm_set_runtime_buffer(substream, NULL);
	return 0;
}

static int abox_rdma_prepare(struct snd_pcm_substream *substream)
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

	data->pointer = IOVA_RDMA_BUFFER(id);

	/* Clear preallocated DMA buffer */
	switch (data->type) {
	case PLATFORM_NORMAL:
	case PLATFORM_SYNC:
		if (substream->runtime->dma_area)
			memset(substream->runtime->dma_area, 0,
					substream->runtime->dma_bytes);
		break;
	default:
		/* nothing to do */
		break;
	}

	msg.ipcid = IPC_PCMPLAYBACK;
	pcmtask_msg->msgtype = PCM_PLTDAI_PREPARE;
	msg.task_id = pcmtask_msg->channel_id = id;
	ret = abox_rdma_request_ipc(data, &msg, 0, 0);

	return ret;
}

static int abox_rdma_trigger(struct snd_pcm_substream *substream, int cmd)
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

	msg.ipcid = IPC_PCMPLAYBACK;
	pcmtask_msg->msgtype = PCM_PLTDAI_TRIGGER;
	msg.task_id = pcmtask_msg->channel_id = id;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		pcmtask_msg->param.trigger = 1;
		ret = abox_rdma_request_ipc(data, &msg, 1, 0);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pcmtask_msg->param.trigger = 0;
		ret = abox_rdma_request_ipc(data, &msg, 1, 0);
		switch (data->type) {
		case PLATFORM_REALTIME:
			msg.ipcid = IPC_ERAP;
			msg.msg.erap.msgtype = REALTIME_STOP;
			ret = abox_rdma_request_ipc(data, &msg, 1, 0);
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

static snd_pcm_uframes_t abox_rdma_pointer(struct snd_pcm_substream *substream)
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

	regmap_read(data->abox_data->regmap, ABOX_RDMA_STATUS(id), &status);
	progress = !!(status & ABOX_RDMA_PROGRESS_MASK);

	if (data->pointer >= IOVA_RDMA_BUFFER(id)) {
		pointer = data->pointer - IOVA_RDMA_BUFFER(id);
	} else if (((data->type == PLATFORM_NORMAL) ||
			(data->type == PLATFORM_SYNC)) && progress) {
		ssize_t offset, count;
		ssize_t buffer_bytes, period_bytes;

		buffer_bytes = snd_pcm_lib_buffer_bytes(substream);
		period_bytes = snd_pcm_lib_period_bytes(substream);

		offset = (((status & ABOX_RDMA_RBUF_OFFSET_MASK) >>
				ABOX_RDMA_RBUF_OFFSET_L) << 4);

		if (period_bytes > ABOX_RDMA_RBUF_CNT_MASK + 1)
			count = 0;
		else
			count = (status & ABOX_RDMA_RBUF_CNT_MASK);

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

static int abox_rdma_open(struct snd_pcm_substream *substream)
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

	snd_soc_set_runtime_hwparams(substream, &abox_rdma_hardware);

	data->substream = substream;

	msg.ipcid = IPC_PCMPLAYBACK;
	pcmtask_msg->msgtype = PCM_PLTDAI_OPEN;
	msg.task_id = pcmtask_msg->channel_id = id;
	ret = abox_rdma_request_ipc(data, &msg, 0, 0);

	return ret;
}

static int abox_rdma_close(struct snd_pcm_substream *substream)
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

	msg.ipcid = IPC_PCMPLAYBACK;
	pcmtask_msg->msgtype = PCM_PLTDAI_CLOSE;
	msg.task_id = pcmtask_msg->channel_id = id;
	ret = abox_rdma_request_ipc(data, &msg, 0, 1);

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
	abox_cmpnt_asrc_release(abox_data->cmpnt, SNDRV_PCM_STREAM_PLAYBACK, id);

	return ret;
}

static int abox_rdma_mmap(struct snd_pcm_substream *substream,
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

	if (data->buf_type == BUFFER_TYPE_ION)
		return dma_buf_mmap(data->ion_buf.dma_buf, vma, 0);
	else
		return dma_mmap_writecombine(dev, vma,
				runtime->dma_area,
				runtime->dma_addr,
				runtime->dma_bytes);
}

static int abox_rdma_ack(struct snd_pcm_substream *substream)
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

	msg.ipcid = IPC_PCMPLAYBACK;
	pcmtask_msg->msgtype = PCM_PLTDAI_ACK;
	pcmtask_msg->param.pointer = (unsigned int)appl_bytes;
	msg.task_id = pcmtask_msg->channel_id = id;

	return abox_rdma_request_ipc(data, &msg, 1, 0);
}

static struct snd_pcm_ops abox_rdma_ops = {
	.open		= abox_rdma_open,
	.close		= abox_rdma_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= abox_rdma_hw_params,
	.hw_free	= abox_rdma_hw_free,
	.prepare	= abox_rdma_prepare,
	.trigger	= abox_rdma_trigger,
	.pointer	= abox_rdma_pointer,
	.mmap		= abox_rdma_mmap,
	.ack		= abox_rdma_ack,
};

static int abox_rdma_fio_ioctl(struct snd_hwdep *hw, struct file *file,
		unsigned int cmd, unsigned long _arg);

#ifdef CONFIG_COMPAT
static int abox_rdma_fio_compat_ioctl(struct snd_hwdep *hw,
		struct file *file,
		unsigned int cmd, unsigned long _arg);
#endif

static int abox_pcm_add_hwdep_dev(struct snd_soc_pcm_runtime *runtime,
		struct abox_platform_data *data)
{
	struct snd_hwdep *hwdep;
	int rc;
	char id[] = "ABOX_MMAP_FD_NN";

	snprintf(id, sizeof(id), "ABOX_MMAP_FD_%d", SNDRV_PCM_STREAM_PLAYBACK);
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
	hwdep->ops.ioctl = abox_rdma_fio_ioctl;
	hwdep->ops.ioctl_compat = abox_rdma_fio_compat_ioctl;
	data->hwdep = hwdep;

	return 0;
}

static int abox_rdma_vol_factor_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_soc_kcontrol_platform(kcontrol);
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	int id = platform_data->id;
	unsigned int volumes;
	unsigned int value = 0;
	int ret = 0;

	ret = snd_soc_component_read(platform_data->cmpnt,
			ABOX_RDMA_VOL_FACTOR(id), &value);
	if (ret < 0) {
		dev_err(dev, "sfr access failed: %d\n", ret);

		return ret;
	}

	volumes = (value & ABOX_RDMA_VOL_FACTOR_MASK);

	dev_dbg(dev, "%s[%d]: %u\n", __func__, id, volumes);

	ucontrol->value.integer.value[0] = volumes;

	return 0;
}

static int abox_rdma_vol_factor_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_soc_kcontrol_platform(kcontrol);
	struct device *dev = platform->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	int id = platform_data->id;
	unsigned int volumes;
	unsigned int value = 0;
	int ret = 0;

	volumes = (unsigned int)ucontrol->value.integer.value[0];
	dev_dbg(dev, "%s[%d]: %u\n", __func__, id, volumes);

	ret = snd_soc_component_read(platform_data->cmpnt,
			ABOX_RDMA_VOL_FACTOR(id), &value);
	if (ret < 0) {
		dev_err(dev, "sfr access failed: %d\n", ret);

		return ret;
	}

	set_value_by_name(value, ABOX_RDMA_VOL_FACTOR, volumes);

	ret = snd_soc_component_write(platform_data->cmpnt,
			ABOX_RDMA_VOL_FACTOR(id), value);
	if (ret < 0) {
		dev_err(dev, "sfr access failed: %d\n", ret);

		return ret;
	}

	return 0;
}

static const DECLARE_TLV_DB_LINEAR(abox_rdma_vol_factor_gain, 0,
		DMA_VOL_FACTOR_MAX_STEPS);

struct snd_kcontrol_new abox_rdma_vol_factor_controls[] = {
	SOC_SINGLE_EXT_TLV(NULL, 0, 0, DMA_VOL_FACTOR_MAX_STEPS, 0,
			abox_rdma_vol_factor_get, abox_rdma_vol_factor_put,
			abox_rdma_vol_factor_gain),
};

static int abox_rdma_new(struct snd_soc_pcm_runtime *runtime)
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
				IOVA_RDMA_BUFFER(id),
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

		ret = abox_iommu_map(dev_abox, IOVA_RDMA_BUFFER(id),
				data->dmab.addr, data->dmab.bytes,
				data->dmab.area);
	}

	return ret;
}

static void abox_rdma_free(struct snd_pcm *pcm)
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
		abox_iommu_unmap(dev_abox, IOVA_RDMA_BUFFER(id));
		snd_dma_free_pages(&data->dmab);
	}
}

static int abox_rdma_probe(struct snd_soc_platform *platform)
{
	struct device *dev = platform->dev;
	struct abox_platform_data *data =
			snd_soc_platform_get_drvdata(platform);
	int ret;

	if (data->type == PLATFORM_COMPRESS) {
		struct abox_compr_data *compr_data = &data->compr_data;
		struct device *dev_abox = &data->abox_data->pdev->dev;

		ret = snd_soc_add_platform_controls(platform,
				abox_rdma_compr_controls,
				ARRAY_SIZE(abox_rdma_compr_controls));
		if (ret < 0) {
			dev_err(dev, "add platform control failed: %d\n", ret);
			return ret;
		}
#ifdef COMPR_USE_FIXED_MEMORY
		compr_data->dma_size = abox_rdma_compr_caps.max_fragments *
				abox_rdma_compr_caps.max_fragment_size;
		compr_data->dma_area = dmam_alloc_coherent(dev,
				compr_data->dma_size, &compr_data->dma_addr,
				GFP_KERNEL);
		if (compr_data->dma_area == NULL) {
			dev_err(dev, "dma memory allocation failed: %lu\n",
					PTR_ERR(compr_data->dma_area));
			return -ENOMEM;
		}
		ret = abox_iommu_map(dev_abox, IOVA_COMPR_BUFFER(data->id),
				compr_data->dma_addr,
				PAGE_ALIGN(compr_data->dma_size),
				compr_data->dma_area);
		if (ret < 0) {
			dev_err(dev, "dma memory iommu map failed: %d\n", ret);
			return ret;
		}
#endif
	}

	if (data->buf_type == BUFFER_TYPE_ION) {
		abox_rdma_vol_factor_controls[0].name = kasprintf(GFP_KERNEL,
				"RDMA VOL FACTOR%d", data->id);
		ret = snd_soc_add_platform_controls(platform,
				abox_rdma_vol_factor_controls,
				ARRAY_SIZE(abox_rdma_vol_factor_controls));
		kfree_const(abox_rdma_vol_factor_controls[0].name);
		if (ret < 0) {
			dev_err(dev, "add platform control failed: %d\n", ret);

			return ret;
		}
	}

	return 0;
}

static int abox_rdma_remove(struct snd_soc_platform *platform)
{
	struct abox_platform_data *data =
			snd_soc_platform_get_drvdata(platform);

	if (data->type == PLATFORM_COMPRESS) {
		struct device *dev_abox = &data->abox_data->pdev->dev;

		abox_iommu_unmap(dev_abox, IOVA_COMPR_BUFFER(data->id));
	}

	return 0;
}

struct snd_soc_platform_driver abox_rdma = {
	.probe		= abox_rdma_probe,
	.remove		= abox_rdma_remove,
	.compr_ops	= &abox_rdma_compr_ops,
	.ops		= &abox_rdma_ops,
	.pcm_new	= abox_rdma_new,
	.pcm_free	= abox_rdma_free,
};

static int abox_rdma_runtime_suspend(struct device *dev)
{
	struct abox_platform_data *data = dev_get_drvdata(dev);
	int id = data->id;

	dev_info(dev, "%s[%d]\n", __func__, id);

	abox_mailbox_save(dev);
	return 0;
}

static int abox_rdma_runtime_resume(struct device *dev)
{
	struct abox_platform_data *data = dev_get_drvdata(dev);
	int id = data->id;

	dev_info(dev, "%s[%d]\n", __func__, id);

	abox_mailbox_restore(dev);
	return 0;
}

static int abox_rdma_cmpnt_probe(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;
	struct abox_platform_data *data = snd_soc_component_get_drvdata(cmpnt);
	u32 id;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	data->cmpnt = cmpnt;
	snd_soc_component_init_regmap(cmpnt, data->abox_data->regmap);
	abox_cmpnt_register_rdma(data->abox_data->pdev, to_platform_device(dev),
			data->id, snd_soc_component_get_dapm(cmpnt),
			data->of_data->get_dai_name(dev, data->id));

	ret = of_samsung_property_read_u32(dev, dev->of_node, "asrc-id", &id);
	if (ret >= 0) {
		ret = abox_cmpnt_asrc_lock(data->abox_data->cmpnt,
				SNDRV_PCM_STREAM_PLAYBACK, data->id, id);
		if (ret < 0)
			dev_err(dev, "asrc id lock failed\n");
		else
			dev_info(dev, "asrc id locked: %u\n", id);
	}

	return 0;
}

static void abox_rdma_cmpnt_remove(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;

	dev_info(dev, "%s\n", __func__);
}

static const struct snd_soc_component_driver abox_rdma_cmpnt = {
	.probe			= abox_rdma_cmpnt_probe,
	.remove			= abox_rdma_cmpnt_remove,
};

static int abox_rdma_dai_bespoke_trigger(struct snd_pcm_substream *substream,
		int cmd, struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_platform_data *platform_data = dev_get_drvdata(dev);
	struct snd_soc_pcm_runtime *runtime = substream->private_data;
	struct snd_soc_platform *platform = runtime->platform;
	struct abox_compr_data *compr_data = &platform_data->compr_data;
	struct snd_compr_stream *cstream = compr_data->cstream;
	struct snd_compr_params *params = &compr_data->codec_param;
	int stream = substream->stream;
	struct list_head *clients = &runtime->dpcm[stream].be_clients;
	struct snd_soc_dpcm *dpcm;

	dev_info(dev, "%s(%d)\n", __func__, cmd);

	if (!platform || !platform->driver->compr_ops ||
			!platform->driver->compr_ops->trigger) {
		dev_err(dev, "%s: invalid substream\n", __func__);

		return -EINVAL;
	}

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (compr_data->bespoke_start == false) {
			if (compr_data->dirty) {
				platform->driver->compr_ops->set_params(cstream,
						params);
				platform->driver->compr_ops->trigger(cstream,
						SNDRV_PCM_TRIGGER_START);
			}
			list_for_each_entry(dpcm, clients, list_be) {
				struct snd_soc_dai *be_dai = dpcm->be->cpu_dai;

				if (be_dai->driver->ops &&
						be_dai->driver->ops->trigger)
					be_dai->driver->ops->trigger(substream,
							cmd,
							be_dai);
			}

			compr_data->bespoke_start = true;
		} else {
			dev_info(dev, "%s(%d) already started\n",
					__func__, cmd);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (compr_data->bespoke_start == true) {
			list_for_each_entry(dpcm, clients, list_be) {
				struct snd_soc_dai *be_dai = dpcm->be->cpu_dai;

				if (be_dai->driver->ops && be_dai->driver->ops->trigger)
					be_dai->driver->ops->trigger(substream, cmd,
							be_dai);
			}
			if (compr_data->dirty)
				platform->driver->compr_ops->trigger(cstream,
						SNDRV_PCM_TRIGGER_STOP);
			compr_data->bespoke_start = false;
		} else {
			dev_info(dev, "%s(%d) already stopped\n",
					__func__, cmd);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

const struct snd_soc_dai_ops abox_rdma_dai_ops = {
	.bespoke_trigger = abox_rdma_dai_bespoke_trigger,
};

static struct snd_soc_dai_driver abox_rdma_dai_drv = {
	.ops = &abox_rdma_dai_ops,
	.playback = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = ABOX_SAMPLING_RATES,
		.rate_min = 8000,
		.rate_max = 384000,
		.formats = ABOX_SAMPLE_FORMATS,
	},
};

enum abox_dai abox_rdma_get_dai_id(int id)
{
	int ret = ABOX_RDMA0 + id;

	return (ret < ABOX_WDMA0) ? ret : -EINVAL;
}

const char *abox_rdma_get_dai_name(struct device *dev, int id)
{
	return devm_kasprintf(dev, GFP_KERNEL, "RDMA%d", id);
}

const char *abox_rdma_get_str_name(struct device *dev, int id, int stream)
{
	const char *ret;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = devm_kasprintf(dev, GFP_KERNEL, "RDMA%d Playback", id);
	else
		ret = ERR_PTR(-EINVAL);

	return ret;
}

static const struct of_device_id samsung_abox_rdma_match[] = {
	{
		.compatible = "samsung,abox-rdma",
		.data = (void *)&(struct abox_dma_of_data){
			.get_dai_id = abox_rdma_get_dai_id,
			.get_dai_name = abox_rdma_get_dai_name,
			.get_str_name = abox_rdma_get_str_name,
			.base_dai_drv = &abox_rdma_dai_drv,
		},
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_rdma_match);

static int abox_rdma_fio_common_ioctl(struct snd_hwdep *hw, struct file *filp,
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
		if (ret < 0) {
			dev_err(dev, "%s MMAP_FD failed: %d\n", __func__, ret);
			return ret;
		}

		if (copy_to_user(_arg, &mmap_fd, sizeof(mmap_fd)))
			return -EFAULT;
		break;
	default:
		dev_err(dev, "unknown ioctl = 0x%x\n", cmd);
		return -EINVAL;
	}

	return 0;
}

static int abox_rdma_fio_ioctl(struct snd_hwdep *hw, struct file *file,
		unsigned int cmd, unsigned long _arg)
{
	return abox_rdma_fio_common_ioctl(hw, file,
			cmd, (unsigned long __user *)_arg);
}

#ifdef CONFIG_COMPAT
static int abox_rdma_fio_compat_ioctl(struct snd_hwdep *hw,
		struct file *file,
		unsigned int cmd, unsigned long _arg)
{
	return abox_rdma_fio_common_ioctl(hw, file, cmd, compat_ptr(_arg));
}
#endif /* CONFIG_COMPAT */

static int samsung_abox_rdma_probe(struct platform_device *pdev)
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
		dev_err(dev, "Failed to get abox platform device\n");
		return -EPROBE_DEFER;
	}
	data->abox_data = dev_get_drvdata(data->dev_abox);

	spin_lock_init(&data->compr_data.lock);
	mutex_init(&data->compr_data.cmd_lock);
	init_waitqueue_head(&data->compr_data.flush_wait);
	init_waitqueue_head(&data->compr_data.exit_wait);
	init_waitqueue_head(&data->compr_data.ipc_wait);
	init_completion(&data->closed);
	data->compr_data.isr_handler = abox_rdma_compr_isr_handler;

	abox_register_ipc_handler(data->dev_abox, IPC_PCMPLAYBACK,
			abox_rdma_ipc_handler, data->abox_data);

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

	of_samsung_property_read_u32_array(dev, np, "pm-qos-lit",
			data->pm_qos_cl0, ARRAY_SIZE(data->pm_qos_cl0));
	ret = of_samsung_property_read_u32_array(dev, np, "pm-qos-mid",
			data->pm_qos_cl1, ARRAY_SIZE(data->pm_qos_cl1));
	if (ret < 0)
		of_samsung_property_read_u32_array(dev, np, "pm-qos-big",
			data->pm_qos_cl1, ARRAY_SIZE(data->pm_qos_cl1));
	else
		of_samsung_property_read_u32_array(dev, np, "pm-qos-big",
			data->pm_qos_cl2, ARRAY_SIZE(data->pm_qos_cl2));

	data->of_data = of_match_node(samsung_abox_rdma_match,
			pdev->dev.of_node)->data;
	data->dai_drv = devm_kzalloc(dev, sizeof(*data->dai_drv), GFP_KERNEL);
	if (!data->dai_drv)
		return -ENOMEM;
	memcpy(data->dai_drv, data->of_data->base_dai_drv,
			sizeof(struct snd_soc_dai_driver));
	data->dai_drv->id = data->of_data->get_dai_id(data->id);
	data->dai_drv->name = data->of_data->get_dai_name(dev, data->id);
	if (data->dai_drv->playback.formats)
		data->dai_drv->playback.stream_name =
				data->of_data->get_str_name(dev, data->id,
				SNDRV_PCM_STREAM_PLAYBACK);
	if (data->type == PLATFORM_COMPRESS)
		data->dai_drv->compress_new = snd_soc_new_compress;

	ret = devm_snd_soc_register_component(dev, &abox_rdma_cmpnt,
			data->dai_drv, 1);
	if (ret < 0)
		return ret;

	if (data->type == PLATFORM_COMPRESS) {
		data->mailbox_base = devm_get_ioremap(pdev, "mailbox",
				NULL, NULL);
		if (IS_ERR(data->mailbox_base))
			return PTR_ERR(data->mailbox_base);

		data->mailbox = devm_regmap_init_mmio(dev,
				data->mailbox_base,
				&abox_mailbox_config);
		if (IS_ERR(data->mailbox))
			return PTR_ERR(data->mailbox);

		pm_runtime_set_autosuspend_delay(dev, 1);
		pm_runtime_use_autosuspend(dev);
	} else {
		pm_runtime_no_callbacks(dev);
	}
	pm_runtime_enable(dev);

	ret = snd_soc_register_platform(&pdev->dev, &abox_rdma);
	if (ret < 0)
		return ret;

	data->hwdep = NULL;

	return 0;
}

static int samsung_abox_rdma_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static const struct dev_pm_ops samsung_abox_rdma_pm = {
	SET_RUNTIME_PM_OPS(abox_rdma_runtime_suspend,
			abox_rdma_runtime_resume, NULL)
};

static struct platform_driver samsung_abox_rdma_driver = {
	.probe  = samsung_abox_rdma_probe,
	.remove = samsung_abox_rdma_remove,
	.driver = {
		.name = "samsung-abox-rdma",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_rdma_match),
		.pm = &samsung_abox_rdma_pm,
	},
};

module_platform_driver(samsung_abox_rdma_driver);

/* Module information */
MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box RDMA Driver");
MODULE_ALIAS("platform:samsung-abox-rdma");
MODULE_LICENSE("GPL");
