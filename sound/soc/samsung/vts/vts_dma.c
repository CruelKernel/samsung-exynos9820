/* sound/soc/samsung/vts/vts-plat.c
 *
 * ALSA SoC - Samsung VTS platfrom driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
  *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
//#define DEBUG
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/pm_runtime.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/wakelock.h>

#include <asm-generic/delay.h>

#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>

#include <sound/samsung/mailbox.h>
#include <sound/samsung/vts.h>
#include <soc/samsung/exynos-pmu.h>

#include "vts.h"

static const struct snd_pcm_hardware vts_platform_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED
				| SNDRV_PCM_INFO_BLOCK_TRANSFER
				| SNDRV_PCM_INFO_MMAP
				| SNDRV_PCM_INFO_MMAP_VALID,
	.formats		= SNDRV_PCM_FMTBIT_S16,
	.rates			= SNDRV_PCM_RATE_16000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= BUFFER_BYTES_MAX,
	.period_bytes_min	= PERIOD_BYTES_MIN,
	.period_bytes_max	= PERIOD_BYTES_MAX,
	.periods_min		= BUFFER_BYTES_MAX / PERIOD_BYTES_MAX,
	.periods_max		= BUFFER_BYTES_MAX / PERIOD_BYTES_MIN,
};

static int vts_platform_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct vts_platform_data *data = dev_get_drvdata(dev);
	struct snd_pcm_runtime *runtime = substream->runtime;

	if (data->type == PLATFORM_VTS_TRIGGER_RECORD) {
		snd_pcm_set_runtime_buffer(substream, &data->vts_data->dmab);
	} else {
		snd_pcm_set_runtime_buffer(substream, &data->vts_data->dmab_rec);
	}
	dev_info(dev, "%s:%s:DmaAddr=%pad Total=%zu PrdSz=%u(%u) #Prds=%u dma_area=%p\n",
			__func__, snd_pcm_stream_str(substream), &runtime->dma_addr,
			runtime->dma_bytes, params_period_size(params),
			params_period_bytes(params), params_periods(params),
			runtime->dma_area);

	data->pointer = 0;

	return 0;
}

static int vts_platform_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;

	dev_dbg(dev, "%s\n", __func__);

	return 0;
}

static int vts_platform_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;

	dev_info(dev, "%s\n", __func__);

	return 0;
}

static int vts_platform_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct vts_platform_data *data = dev_get_drvdata(dev);
	u32 values[3] = {0,0,0};
	int result = 0;

	dev_info(dev, "%s ++ CMD: %d\n", __func__, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (data->type == PLATFORM_VTS_TRIGGER_RECORD) {
			dev_dbg(dev, "%s VTS_IRQ_AP_START_COPY\n", __func__);
			result = vts_start_ipc_transaction(dev, data->vts_data, VTS_IRQ_AP_START_COPY, &values, 1, 1);
		} else {
			dev_dbg(dev, "%s VTS_IRQ_AP_START_REC\n", __func__);
			result = vts_start_ipc_transaction(dev, data->vts_data, VTS_IRQ_AP_START_REC, &values, 1, 1);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (data->type == PLATFORM_VTS_TRIGGER_RECORD) {
			dev_dbg(dev, "%s VTS_IRQ_AP_STOP_COPY\n", __func__);
			result = vts_start_ipc_transaction(dev, data->vts_data, VTS_IRQ_AP_STOP_COPY, &values, 1, 1);
		} else {
			dev_dbg(dev, "%s VTS_IRQ_AP_STOP_REC\n", __func__);
			result = vts_start_ipc_transaction(dev, data->vts_data, VTS_IRQ_AP_STOP_REC, &values, 1, 1);
		}
		break;
	default:
		result = -EINVAL;
		break;
	}

	dev_info(dev, "%s -- CMD: %d\n", __func__, cmd);
	return result;
}

static snd_pcm_uframes_t vts_platform_pointer(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct vts_platform_data *data = dev_get_drvdata(dev);
	struct snd_pcm_runtime *runtime = substream->runtime;

	dev_dbg(dev, "%s: pointer=%08x\n", __func__, data->pointer);

	return bytes_to_frames(runtime, data->pointer);
}

static int vts_platform_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct vts_platform_data *data = dev_get_drvdata(dev);
	int result = 0;

	dev_info(dev, "%s\n", __func__);

	if (data->vts_data->voicecall_enabled) {
		dev_warn(dev, "%s VTS SRAM is Used for CP call\n",
					__func__);
		return -EBUSY;
	}

	pm_runtime_get_sync(dev);
	snd_soc_set_runtime_hwparams(substream, &vts_platform_hardware);
	if (data->type == PLATFORM_VTS_NORMAL_RECORD) {
		dev_info(dev, "%s open --\n", __func__);
		result = vts_set_dmicctrl(data->vts_data->pdev,
					VTS_MICCONF_FOR_RECORD, true);
		if (result < 0) {
			dev_err(dev, "%s: MIC control configuration failed\n", __func__);
			pm_runtime_put_sync(dev);
		}
	}

	return result;
}

static int vts_platform_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct vts_platform_data *data = dev_get_drvdata(dev);
	int result = 0;

	dev_info(dev, "%s\n", __func__);

	if (data->vts_data->voicecall_enabled) {
		dev_warn(dev, "%s VTS SRAM is Used for CP call\n",
					__func__);
		pm_runtime_get_sync(dev);
		return -EBUSY;
	}

	if (data->type == PLATFORM_VTS_NORMAL_RECORD) {
		dev_info(dev, "%s close --\n", __func__);
		result = vts_set_dmicctrl(data->vts_data->pdev,
					VTS_MICCONF_FOR_RECORD, false);
		if (result < 0)
			dev_warn(dev, "%s: MIC control configuration failed\n", __func__);
	}

	pm_runtime_put_sync(dev);
	return result;
}

static int vts_platform_mmap(struct snd_pcm_substream *substream,
		struct vm_area_struct *vma)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct snd_pcm_runtime *runtime = substream->runtime;

	dev_info(dev, "%s\n", __func__);

	return dma_mmap_writecombine(dev, vma,
			runtime->dma_area,
			runtime->dma_addr,
			runtime->dma_bytes);
}

static struct snd_pcm_ops vts_platform_ops = {
	.open		= vts_platform_open,
	.close		= vts_platform_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= vts_platform_hw_params,
	.hw_free	= vts_platform_hw_free,
	.prepare	= vts_platform_prepare,
	.trigger	= vts_platform_trigger,
	.pointer	= vts_platform_pointer,
	.mmap		= vts_platform_mmap,
};

static int vts_platform_new(struct snd_soc_pcm_runtime *runtime)
{
	struct snd_soc_platform *platform = runtime->platform;
	struct device *dev = platform->dev;
	struct vts_platform_data *data = dev_get_drvdata(dev);
	struct snd_pcm_substream *substream = runtime->pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;

	dev_info(dev, "%s \n", __func__);
	data->substream = substream;
	dev_info(dev, "%s Update Soc Card from runtime!!\n", __func__);
	data->vts_data->card = runtime->card;

	return 0;
}

static void vts_platform_free(struct snd_pcm *pcm)
{
	return;
}

static const struct snd_soc_platform_driver vts_dma = {
	.ops		= &vts_platform_ops,
	.pcm_new	= vts_platform_new,
	.pcm_free	= vts_platform_free,
};

static int samsung_vts_dma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *np_vts;
	struct vts_platform_data *data;
	int result;
	const char *type;

	dev_info(dev, "%s \n", __func__);
	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, data);

	np_vts = of_parse_phandle(np, "vts", 0);
	if (!np_vts) {
		dev_err(dev, "Failed to get vts device node\n");
		return -EPROBE_DEFER;
	}
	data->pdev_vts = of_find_device_by_node(np_vts);
	if (!data->pdev_vts) {
		dev_err(dev, "Failed to get vts platform device\n");
		return -EPROBE_DEFER;
	}
	data->vts_data = platform_get_drvdata(data->pdev_vts);

	result = of_property_read_u32_index(np, "id", 0, &data->id);
	if (result < 0) {
		dev_err(dev, "id property reading fail\n");
		return result;
	}

	result = of_property_read_string(np, "type", &type);
	if (result < 0) {
		dev_err(dev, "type property reading fail\n");
		return result;
	}

	if (!strncmp(type, "vts-record", sizeof("vts-record"))) {
		data->type = PLATFORM_VTS_NORMAL_RECORD;
		dev_info(dev, "%s - vts-record Probed \n", __func__);
	} else {
		data->type = PLATFORM_VTS_TRIGGER_RECORD;
		dev_info(dev, "%s - vts-trigger-record Probed \n", __func__);
	}

	vts_register_dma(data->vts_data->pdev, pdev, data->id);

	return snd_soc_register_platform(&pdev->dev, &vts_dma);
}

static int samsung_vts_dma_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static const struct of_device_id samsung_vts_dma_match[] = {
	{
		.compatible = "samsung,vts-dma",
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_vts_dma_match);

static struct platform_driver samsung_vts_dma_driver = {
	.probe  = samsung_vts_dma_probe,
	.remove = samsung_vts_dma_remove,
	.driver = {
		.name = "samsung-vts-dma",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_vts_dma_match),
	},
};

module_platform_driver(samsung_vts_dma_driver);

/* Module information */
MODULE_AUTHOR("Palli Satish Kumar Reddy, <palli.satish@samsung.com>");
MODULE_DESCRIPTION("Samsung VTS DMA");
MODULE_ALIAS("platform:samsung-vts-dma");
MODULE_LICENSE("GPL");
