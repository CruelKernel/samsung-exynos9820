/* sound/soc/samsung/abox/abox_uaif.c
 *
 * ALSA SoC Audio Layer - Samsung Abox UAIF driver
 *
 * Copyright (c) 2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>

#include <sound/pcm_params.h>
#include <sound/samsung/abox.h>

#include "abox_util.h"
#include "abox.h"
#include "abox_cmpnt.h"
#include "abox_if.h"

static int abox_if_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_component *cmpnt = data->cmpnt;
	struct abox_data *abox_data = data->abox_data;
	int ret;

	dev_dbg(dev, "%s[%c]\n", __func__,
			(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ?
			'C' : 'P');

	abox_request_cpu_gear_dai(dev, abox_data, dai, abox_data->cpu_gear_min);
	ret = clk_enable(data->clk_bclk);
	if (ret < 0) {
		dev_err(dev, "Failed to enable bclk: %d\n", ret);
		goto err;
	}
	ret = clk_enable(data->clk_bclk_gate);
	if (ret < 0) {
		dev_err(dev, "Failed to enable bclk_gate: %d\n", ret);
		goto err;
	}

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		snd_soc_component_update_bits(cmpnt, ABOX_UAIF_CTRL0(data->id),
				ABOX_DATA_MODE_MASK | ABOX_IRQ_MODE_MASK,
				(1 << ABOX_DATA_MODE_L) |
				(0 << ABOX_IRQ_MODE_L));
err:
	return ret;
}

static void abox_if_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	struct abox_data *abox_data = data->abox_data;

	dev_dbg(dev, "%s[%c]\n", __func__,
			(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ?
			'C' : 'P');

	clk_disable(data->clk_bclk_gate);
	clk_disable(data->clk_bclk);
	abox_request_cpu_gear_dai(dev, abox_data, dai, 0);
}

static int abox_if_hw_free(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	struct abox_data *abox_data = data->abox_data;

	dev_dbg(dev, "%s[%c]\n", __func__,
			(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ?
			'C' : 'P');

	abox_register_bclk_usage(dev, abox_data, dai->id, 0, 0, 0);

	return 0;
}

static int abox_dsif_set_bclk_ratio(struct snd_soc_dai *dai, unsigned int ratio)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	unsigned int rate = dai->rate;
	int ret;

	dev_dbg(dev, "%s(%u)\n", __func__, ratio);

	ret = clk_set_rate(data->clk_bclk, rate * ratio);
	if (ret < 0)
		dev_err(dev, "bclk set error=%d\n", ret);
	else
		dev_info(dev, "rate=%u, bclk=%lu\n", rate,
				clk_get_rate(data->clk_bclk));

	return ret;
}

static int abox_dsif_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_component *cmpnt = data->cmpnt;
	unsigned int ctrl;
	int ret = 0;

	dev_info(dev, "%s(0x%08x)\n", __func__, fmt);

	pm_runtime_get_sync(dev);

	snd_soc_component_read(cmpnt, ABOX_DSIF_CTRL, &ctrl);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_PDM:
		break;
	default:
		ret = -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
	case SND_SOC_DAIFMT_NB_IF:
		set_value_by_name(ctrl, ABOX_DSIF_BCLK_POLARITY, 1);
		break;
	case SND_SOC_DAIFMT_IB_NF:
	case SND_SOC_DAIFMT_IB_IF:
		set_value_by_name(ctrl, ABOX_DSIF_BCLK_POLARITY, 0);
		break;
	default:
		ret = -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		ret = -EINVAL;
	}

	snd_soc_component_write(cmpnt, ABOX_DSIF_CTRL, ctrl);

	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);

	return ret;
}

static int abox_dsif_set_channel_map(struct snd_soc_dai *dai,
		unsigned int tx_num, unsigned int *tx_slot,
		unsigned int rx_num, unsigned int *rx_slot)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_component *cmpnt = data->cmpnt;

	dev_info(dev, "%s\n", __func__);

	snd_soc_component_update_bits(cmpnt, ABOX_DSIF_CTRL, ABOX_ORDER_MASK,
			(tx_slot[0] ? 1 : 0) << ABOX_ORDER_L);

	return 0;
}

static int abox_dsif_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *hw_params, struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	unsigned int channels, rate, width;

	dev_info(dev, "%s[%c]\n", __func__,
			(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ?
			'C' : 'P');

	channels = params_channels(hw_params);
	rate = params_rate(hw_params);
	width = params_width(hw_params);

	dev_info(dev, "rate=%u, width=%d, channel=%u, bclk=%lu\n",
			rate,
			width,
			channels,
			clk_get_rate(data->clk_bclk));

	switch (params_format(hw_params)) {
	case SNDRV_PCM_FORMAT_S32:
		break;
	default:
		return -EINVAL;
	}

	switch (channels) {
	case 2:
		break;
	default:
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		abox_cmpnt_reset_cnt_val(dev, data->cmpnt, dai->id);

	return 0;
}

static int abox_dsif_trigger(struct snd_pcm_substream *substream,
		int trigger, struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_component *cmpnt = data->cmpnt;

	dev_info(dev, "%s[%c](%d)\n", __func__,
			(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ?
			'C' : 'P', trigger);

	switch (trigger) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		snd_soc_component_update_bits(cmpnt, ABOX_DSIF_CTRL,
				ABOX_ENABLE_MASK, 1 << ABOX_ENABLE_L);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		snd_soc_component_update_bits(cmpnt, ABOX_DSIF_CTRL,
				ABOX_ENABLE_MASK, 0 << ABOX_ENABLE_L);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int abox_uaif_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_component *cmpnt = data->cmpnt;
	int id = data->id;
	unsigned int ctrl0, ctrl1;
	int ret = 0;

	dev_info(dev, "%s(0x%08x)\n", __func__, fmt);

	pm_runtime_get_sync(dev);

	snd_soc_component_read(cmpnt, ABOX_UAIF_CTRL0(id), &ctrl0);
	snd_soc_component_read(cmpnt, ABOX_UAIF_CTRL1(id), &ctrl1);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		set_value_by_name(ctrl1, ABOX_WS_MODE, 0);
		break;
	case SND_SOC_DAIFMT_DSP_A:
		set_value_by_name(ctrl1, ABOX_WS_MODE, 1);
		break;
	default:
		ret = -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		set_value_by_name(ctrl1, ABOX_BCLK_POLARITY, 1);
		set_value_by_name(ctrl1, ABOX_WS_POLAR, 0);
		break;
	case SND_SOC_DAIFMT_NB_IF:
		set_value_by_name(ctrl1, ABOX_BCLK_POLARITY, 1);
		set_value_by_name(ctrl1, ABOX_WS_POLAR, 1);
		break;
	case SND_SOC_DAIFMT_IB_NF:
		set_value_by_name(ctrl1, ABOX_BCLK_POLARITY, 0);
		set_value_by_name(ctrl1, ABOX_WS_POLAR, 0);
		break;
	case SND_SOC_DAIFMT_IB_IF:
		set_value_by_name(ctrl1, ABOX_BCLK_POLARITY, 0);
		set_value_by_name(ctrl1, ABOX_WS_POLAR, 1);
		break;
	default:
		ret = -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		set_value_by_name(ctrl0, ABOX_MODE, 0);
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		set_value_by_name(ctrl0, ABOX_MODE, 1);
		break;
	default:
		ret = -EINVAL;
	}

	snd_soc_component_write(cmpnt, ABOX_UAIF_CTRL0(id), ctrl0);
	snd_soc_component_write(cmpnt, ABOX_UAIF_CTRL1(id), ctrl1);

	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);

	return ret;
}

static int abox_uaif_set_tristate(struct snd_soc_dai *dai, int tristate)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	struct abox_data *abox_data = data->abox_data;
	int id = data->id;
	enum qchannel clk;

	dev_dbg(dev, "%s(%d)\n", __func__, tristate);

	switch (id) {
	case 0:
		clk = ABOX_BCLK_UAIF0;
		break;
	case 1:
		clk = ABOX_BCLK_UAIF1;
		break;
	case 2:
		clk = ABOX_BCLK_UAIF2;
		break;
	case 3:
		clk = ABOX_BCLK_UAIF3;
		break;
	default:
		return -EINVAL;
	}

	return abox_disable_qchannel(dev, abox_data, clk, !tristate);
}

static int abox_uaif_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *hw_params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	struct abox_data *abox_data = data->abox_data;
	struct snd_soc_component *cmpnt = data->cmpnt;
	int id = data->id;
	unsigned int ctrl1;
	unsigned int channels, rate, width;
	int ret;

	dev_info(dev, "%s[%c]\n", __func__,
			(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ?
			'C' : 'P');

	channels = params_channels(hw_params);
	rate = params_rate(hw_params);
	width = params_width(hw_params);

	ret = abox_register_bclk_usage(dev, abox_data, dai->id, rate, channels,
			width);
	if (ret < 0)
		dev_err(dev, "Unable to register bclk usage: %d\n", ret);

	ret = clk_set_rate(data->clk_bclk, rate * channels * width);
	if (ret < 0) {
		dev_err(dev, "bclk set error=%d\n", ret);
		return ret;
	}

	dev_info(dev, "rate=%u, width=%d, channel=%u, bclk=%lu\n",
			rate, width, channels, clk_get_rate(data->clk_bclk));

	ret = snd_soc_component_read(cmpnt, ABOX_UAIF_CTRL1(id), &ctrl1);
	if (ret < 0)
		dev_err(dev, "sfr access failed: %d\n", ret);

	switch (params_format(hw_params)) {
	case SNDRV_PCM_FORMAT_S16:
	case SNDRV_PCM_FORMAT_S24:
	case SNDRV_PCM_FORMAT_S32:
		set_value_by_name(ctrl1, ABOX_SBIT_MAX, (width - 1));
		break;
	default:
		return -EINVAL;
	}

	switch (rtd->dai_link->dai_fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		set_value_by_name(ctrl1, ABOX_VALID_STR, 0);
		set_value_by_name(ctrl1, ABOX_VALID_END, 0);
		break;
	default:
		set_value_by_name(ctrl1, ABOX_VALID_STR, (width - 1));
		set_value_by_name(ctrl1, ABOX_VALID_END, (width - 1));
		break;
	}

	set_value_by_name(ctrl1, ABOX_SLOT_MAX, (channels - 1));
	set_value_by_name(ctrl1, ABOX_FORMAT, abox_get_format(width, channels));

	ret = snd_soc_component_write(cmpnt, ABOX_UAIF_CTRL1(id), ctrl1);
	if (ret < 0)
		dev_err(dev, "sfr access failed: %d\n", ret);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		abox_cmpnt_reset_cnt_val(dev, data->cmpnt, dai->id);

	return 0;
}

static int abox_uaif_trigger(struct snd_pcm_substream *substream,
		int trigger, struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_component *cmpnt = data->cmpnt;
	int id = data->id;
	unsigned long mask, shift;
	int ret = 0;

	dev_info(dev, "%s[%c](%d)\n", __func__,
			(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ?
			'C' : 'P', trigger);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		mask = ABOX_MIC_ENABLE_MASK;
		shift = ABOX_MIC_ENABLE_L;
	} else {
		mask = ABOX_SPK_ENABLE_MASK;
		shift = ABOX_SPK_ENABLE_L;
	}

	switch (trigger) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		ret = snd_soc_component_update_bits(cmpnt, ABOX_UAIF_CTRL0(id),
				mask, 1 << shift);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			ret = snd_soc_component_update_bits(cmpnt,
					ABOX_UAIF_CTRL0(id), mask, 0 << shift);
		}
		break;
	default:
		ret = -EINVAL;
	}
	if (ret < 0)
		dev_err(dev, "sfr access failed: %d\n", ret);

	return ret;
}

static void abox_uaif_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_component *cmpnt = data->cmpnt;
	int id = data->id;

	dev_dbg(dev, "%s[%c]\n", __func__,
			(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ?
			'C' : 'P');

	/* dpcm_be_dai_trigger doesn't call trigger stop on paused stream. */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* Delay to flush FIFO in UAIF */
		usleep_range(600, 1200);

		snd_soc_component_update_bits(cmpnt, ABOX_UAIF_CTRL0(id),
				ABOX_SPK_ENABLE_MASK, 0 << ABOX_SPK_ENABLE_L);
	}
	abox_if_shutdown(substream, dai);
}

static const struct snd_soc_dai_ops abox_dsif_dai_ops = {
	.set_bclk_ratio	= abox_dsif_set_bclk_ratio,
	.set_fmt	= abox_dsif_set_fmt,
	.set_channel_map	= abox_dsif_set_channel_map,
	.startup	= abox_if_startup,
	.shutdown	= abox_if_shutdown,
	.hw_params	= abox_dsif_hw_params,
	.hw_free	= abox_if_hw_free,
	.trigger	= abox_dsif_trigger,
};

static struct snd_soc_dai_driver abox_dsif_dai_drv = {
	.playback = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = ABOX_SAMPLING_RATES,
		.rate_min = 8000,
		.rate_max = 384000,
		.formats = SNDRV_PCM_FMTBIT_S32,
	},
	.ops = &abox_dsif_dai_ops,
};

static const struct snd_soc_dai_ops abox_uaif_dai_ops = {
	.set_fmt	= abox_uaif_set_fmt,
	.set_tristate	= abox_uaif_set_tristate,
	.startup	= abox_if_startup,
	.shutdown	= abox_uaif_shutdown,
	.hw_params	= abox_uaif_hw_params,
	.hw_free	= abox_if_hw_free,
	.trigger	= abox_uaif_trigger,
};

static struct snd_soc_dai_driver abox_uaif_dai_drv = {
	.playback = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = ABOX_SAMPLING_RATES,
		.rate_min = 8000,
		.rate_max = 384000,
		.formats = ABOX_SAMPLE_FORMATS,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = ABOX_SAMPLING_RATES,
		.rate_min = 8000,
		.rate_max = 384000,
		.formats = ABOX_SAMPLE_FORMATS,
	},
	.ops = &abox_uaif_dai_ops,
	.symmetric_rates = 1,
	.symmetric_channels = 1,
	.symmetric_samplebits = 1,
};

static int abox_if_config_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_if_data *data = snd_soc_component_get_drvdata(cmpnt);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = data->config[reg];

	dev_dbg(dev, "%s(0x%08x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int abox_if_config_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_if_data *data = snd_soc_component_get_drvdata(cmpnt);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(0x%08x, %u)\n", __func__, reg, val);

	data->config[reg] = val;

	return 0;
}

static const struct snd_kcontrol_new abox_if_controls[] = {
	SOC_SINGLE_EXT("%s Width", ABOX_IF_WIDTH, 0, 32, 0,
			abox_if_config_get, abox_if_config_put),
	SOC_SINGLE_EXT("%s Channel", ABOX_IF_CHANNEL, 0, 8, 0,
			abox_if_config_get, abox_if_config_put),
	SOC_SINGLE_EXT("%s Rate", ABOX_IF_RATE, 0, 384000, 0,
			abox_if_config_get, abox_if_config_put),
};

static int abox_if_cmpnt_probe(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;
	struct abox_if_data *data = snd_soc_component_get_drvdata(cmpnt);
	struct snd_kcontrol_new (*controls)[3];
	struct snd_kcontrol_new *control;
	size_t i;

	dev_info(dev, "%s\n", __func__);

	controls = devm_kmemdup(dev, abox_if_controls,
			sizeof(abox_if_controls), GFP_KERNEL);
	if (!controls)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(*controls); i++) {
		control = &(*controls)[i];
		control->name = devm_kasprintf(dev, GFP_KERNEL, control->name,
				data->of_data->get_dai_name(data->id));
	}
	snd_soc_add_component_controls(cmpnt, *controls, ARRAY_SIZE(*controls));

	data->cmpnt = cmpnt;
	snd_soc_component_init_regmap(cmpnt, data->abox_data->regmap);
	abox_cmpnt_register_if(data->abox_data->pdev, to_platform_device(dev),
			data->id, snd_soc_component_get_dapm(cmpnt),
			data->of_data->get_dai_name(data->id),
			!!data->dai_drv->playback.formats,
			!!data->dai_drv->capture.formats);

	return 0;
}

static void abox_if_cmpnt_remove(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;

	dev_info(dev, "%s\n", __func__);
}

static const struct snd_soc_component_driver abox_if_cmpnt = {
	.probe			= abox_if_cmpnt_probe,
	.remove			= abox_if_cmpnt_remove,
};

enum abox_dai abox_dsif_get_dai_id(int id)
{
	return ABOX_DSIF;
}

const char *abox_dsif_get_dai_name(int id)
{
	return "DSIF";
}

const char *abox_dsif_get_str_name(int id, int stream)
{
	return (stream == SNDRV_PCM_STREAM_PLAYBACK) ?
			"DSIF Playback" : "DSIF Capture";
}

enum abox_dai abox_uaif_get_dai_id(int id)
{
	return ABOX_UAIF0 + id;
}

const char *abox_uaif_get_dai_name(int id)
{
	static const char * const names[] = {
		"UAIF0", "UAIF1", "UAIF2", "UAIF3", "UAIF4",
		"UAIF5", "UAIF6", "UAIF7", "UAIF8", "UAIF9",
	};

	return (id < ARRAY_SIZE(names)) ? names[id] : ERR_PTR(-EINVAL);
}

const char *abox_uaif_get_str_name(int id, int stream)
{
	static const char * const names_pla[] = {
		"UAIF0 Playback", "UAIF1 Playback", "UAIF2 Playback",
		"UAIF3 Playback", "UAIF4 Playback", "UAIF5 Playback",
		"UAIF6 Playback", "UAIF7 Playback", "UAIF8 Playback",
		"UAIF9 Playback",
	};
	static const char * const names_cap[] = {
		"UAIF0 Capture", "UAIF1 Capture", "UAIF2 Capture",
		"UAIF3 Capture", "UAIF4 Capture", "UAIF5 Capture",
		"UAIF6 Capture", "UAIF7 Capture", "UAIF8 Capture",
		"UAIF9 Capture",
	};
	const char *ret;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK && id < ARRAY_SIZE(names_pla))
		ret = names_pla[id];
	else if (stream == SNDRV_PCM_STREAM_CAPTURE &&
			id < ARRAY_SIZE(names_cap))
		ret = names_cap[id];
	else
		ret = ERR_PTR(-EINVAL);

	return ret;
}

static const struct of_device_id samsung_abox_if_match[] = {
	{
		.compatible = "samsung,abox-uaif",
		.data = (void *)&(struct abox_if_of_data){
			.get_dai_id = abox_uaif_get_dai_id,
			.get_dai_name = abox_uaif_get_dai_name,
			.get_str_name = abox_uaif_get_str_name,
			.base_dai_drv = &abox_uaif_dai_drv,
		},
	},
	{
		.compatible = "samsung,abox-dsif",
		.data = (void *)&(struct abox_if_of_data){
			.get_dai_id = abox_dsif_get_dai_id,
			.get_dai_name = abox_dsif_get_dai_name,
			.get_str_name = abox_dsif_get_str_name,
			.base_dai_drv = &abox_dsif_dai_drv,
		},
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_if_match);

static int samsung_abox_if_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device *dev_abox = dev->parent;
	struct device_node *np = dev->of_node;
	struct abox_if_data *data;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	platform_set_drvdata(pdev, data);

	data->sfr_base = devm_get_ioremap(pdev, "sfr", NULL, NULL);
	if (IS_ERR(data->sfr_base))
		return PTR_ERR(data->sfr_base);

	ret = of_property_read_u32_index(np, "id", 0, &data->id);
	if (ret < 0) {
		dev_err(dev, "id property reading fail\n");
		return ret;
	}

	data->clk_bclk = devm_clk_get_and_prepare(pdev, "bclk");
	if (IS_ERR(data->clk_bclk))
		return PTR_ERR(data->clk_bclk);

	data->clk_bclk_gate = devm_clk_get_and_prepare(pdev, "bclk_gate");
	if (IS_ERR(data->clk_bclk_gate))
		return PTR_ERR(data->clk_bclk_gate);

	data->of_data = of_match_node(samsung_abox_if_match,
			pdev->dev.of_node)->data;
	data->abox_data = dev_get_drvdata(dev_abox);
	data->dai_drv = devm_kzalloc(dev, sizeof(struct snd_soc_dai_driver),
			GFP_KERNEL);
	if (!data->dai_drv)
		return -ENOMEM;
	memcpy(data->dai_drv, data->of_data->base_dai_drv,
			sizeof(struct snd_soc_dai_driver));
	data->dai_drv->id = data->of_data->get_dai_id(data->id);
	data->dai_drv->name = data->of_data->get_dai_name(data->id);
	if (data->dai_drv->capture.formats)
		data->dai_drv->capture.stream_name =
				data->of_data->get_str_name(data->id,
				SNDRV_PCM_STREAM_CAPTURE);
	if (data->dai_drv->playback.formats)
		data->dai_drv->playback.stream_name =
				data->of_data->get_str_name(data->id,
				SNDRV_PCM_STREAM_PLAYBACK);

	ret = devm_snd_soc_register_component(dev, &abox_if_cmpnt,
			data->dai_drv, 1);
	if (ret < 0)
		return ret;

	pm_runtime_enable(dev);
	pm_runtime_no_callbacks(dev);

	return ret;
}

static int samsung_abox_if_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_dbg(dev, "%s\n", __func__);

	return 0;
}

static struct platform_driver samsung_abox_if_driver = {
	.probe  = samsung_abox_if_probe,
	.remove = samsung_abox_if_remove,
	.driver = {
		.name = "samsung-abox-if",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_if_match),
	},
};

module_platform_driver(samsung_abox_if_driver);

int abox_if_hw_params_fixup(struct snd_soc_dai *dai,
		struct snd_pcm_hw_params *params, int stream)
{
	struct device *dev = dai->dev;
	struct abox_if_data *data = dev_get_drvdata(dev);
	unsigned int rate, channels, width;
	int ret = 0;

	if (dev->driver != &samsung_abox_if_driver.driver)
		return -EINVAL;

	dev_dbg(dev, "%s[%s](%d)\n", __func__, dai->name, stream);

	rate = data->config[ABOX_IF_RATE];
	channels = data->config[ABOX_IF_CHANNEL];
	width = data->config[ABOX_IF_WIDTH];

	/* don't break symmetric limitation */
	if (dai->driver->symmetric_rates && dai->rate && dai->rate != rate)
		rate = dai->rate;
	if (dai->driver->symmetric_channels && dai->channels &&
			dai->channels != channels)
		channels = dai->channels;
	if (dai->driver->symmetric_samplebits && dai->sample_width &&
			dai->sample_width != width)
		width = dai->sample_width;

	if (rate)
		hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE)->min = rate;

	if (channels)
		hw_param_interval(params, SNDRV_PCM_HW_PARAM_CHANNELS)->min =
				channels;

	if (width) {
		unsigned int format = 0;

		switch (width) {
		case 8:
			format = SNDRV_PCM_FORMAT_S8;
			break;
		case 16:
			format = SNDRV_PCM_FORMAT_S16;
			break;
		case 24:
			format = SNDRV_PCM_FORMAT_S24;
			break;
		case 32:
			format = SNDRV_PCM_FORMAT_S32;
			break;
		default:
			width = format = 0;
			break;
		}

		if (format) {
			struct snd_mask *mask;

			mask = hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT);
			snd_mask_none(mask);
			snd_mask_set(mask, format);
		}
	}

	if (rate || channels || width)
		dev_info(dev, "%s: %s: %d bit, %u channel, %uHz\n",
				__func__, dai->name, width, channels, rate);
	else
		ret = -EINVAL;

	return ret;
}

int abox_if_hw_params_fixup_helper(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params, int stream)
{
	struct snd_soc_dai *dai = rtd->cpu_dai;

	return abox_if_hw_params_fixup(dai, params, stream);
}

/* Module information */
MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box UAIF/DSIF Driver");
MODULE_ALIAS("platform:samsung-abox-if");
MODULE_LICENSE("GPL");
