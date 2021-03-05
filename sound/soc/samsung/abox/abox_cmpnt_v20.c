/* sound/soc/samsung/abox/abox_cmpnt_20.c
 *
 * ALSA SoC Audio Layer - Samsung Abox Component driver for 2.0.0
 *
 * Copyright (c) 2018 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/sched/clock.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "abox.h"
#include "abox_dump.h"
#include "abox_vdma.h"
#include "abox_if.h"
#include "abox_cmpnt.h"

enum asrc_tick {
	TICK_CP = 0x0,
	TICK_SYNC,
	TICK_UAIF0 = 0x3,
	TICK_UAIF1 = 0x4,
	TICK_UAIF2 = 0x5,
	TICK_UAIF3 = 0x6,
	TICK_USB = 0x7,
	ASRC_TICK_COUNT,
};

enum asrc_ratio {
	RATIO_1,
	RATIO_2,
	RATIO_4,
	RATIO_8,
	RATIO_16,
	RATIO_32,
	RATIO_64,
	RATIO_128,
	RATIO_256,
};

enum asrc_rate {
	RATE_8000,
	RATE_16000,
	RATE_32000,
	RATE_40000,
	RATE_44100,
	RATE_48000,
	RATE_96000,
	RATE_192000,
	RATE_384000,
	ASRC_RATE_COUNT,
};

static const unsigned int asrc_rates[] = {
	8000, 16000, 32000, 40000, 44100, 48000, 96000, 192000, 384000
};

static enum asrc_rate to_asrc_rate(unsigned int rate)
{
	enum asrc_rate arate;

	for (arate = RATE_8000; arate < ASRC_RATE_COUNT; arate++) {
		if (asrc_rates[arate] == rate)
			break;
	}

	return (arate < ASRC_RATE_COUNT) ? arate : RATE_48000;
}

struct asrc_ctrl {
	unsigned int isr;
	unsigned int osr;
	enum asrc_ratio ovsf;
	unsigned int ifactor;
	enum asrc_ratio dcmf;
};

static const struct asrc_ctrl asrc_table[ASRC_RATE_COUNT][ASRC_RATE_COUNT] = {
		/* isr, osr, ovsf, ifactor, dcmf */
	{
		{  8000,   8000, RATIO_8, 65536, RATIO_8},
		{  8000,  16000, RATIO_8, 65536, RATIO_8},
		{  8000,  32000, RATIO_8, 98304, RATIO_8},
		{  8000,  40000, RATIO_8, 76800, RATIO_8},
		{  8000,  44100, RATIO_8, 88200, RATIO_8},
		{  8000,  48000, RATIO_8, 98304, RATIO_8},
		{  8000,  96000, RATIO_8, 98304, RATIO_4},
		{  8000, 192000, RATIO_8, 98304, RATIO_2},
		{  8000, 384000, RATIO_8, 98304, RATIO_1},
	},
	{
		{ 16000,   8000, RATIO_8, 32768, RATIO_8},
		{ 16000,  16000, RATIO_8, 65536, RATIO_8},
		{ 16000,  32000, RATIO_8, 65536, RATIO_8},
		{ 16000,  40000, RATIO_8, 76800, RATIO_8},
		{ 16000,  44100, RATIO_8, 88200, RATIO_8},
		{ 16000,  48000, RATIO_8, 98304, RATIO_8},
		{ 16000,  96000, RATIO_8, 98304, RATIO_8},
		{ 16000, 192000, RATIO_8, 98304, RATIO_4},
		{ 16000, 384000, RATIO_8, 98304, RATIO_2},
	},
	{
		{ 32000,   8000, RATIO_8, 24576, RATIO_8},
		{ 32000,  16000, RATIO_8, 32768, RATIO_8},
		{ 32000,  32000, RATIO_8, 65536, RATIO_8},
		{ 32000,  40000, RATIO_8, 76800, RATIO_8},
		{ 32000,  44100, RATIO_8, 88200, RATIO_8},
		{ 32000,  48000, RATIO_8, 98304, RATIO_8},
		{ 32000,  96000, RATIO_8, 73728, RATIO_2},
		{ 32000, 192000, RATIO_8, 98304, RATIO_2},
		{ 32000, 384000, RATIO_8, 98304, RATIO_1},
	},
	{
		{ 40000,   8000, RATIO_8, 15360, RATIO_8},
		{ 40000,  16000, RATIO_8, 30720, RATIO_8},
		{ 40000,  32000, RATIO_8, 61440, RATIO_8},
		{ 40000,  40000, RATIO_8, 65536, RATIO_8},
		{ 40000,  44100, RATIO_8, 88200, RATIO_8},
		{ 40000,  48000, RATIO_8, 61440, RATIO_8},
		{ 40000,  96000, RATIO_8, 61440, RATIO_4},
		{ 40000, 192000, RATIO_8, 61440, RATIO_2},
		{ 40000, 384000, RATIO_8, 61440, RATIO_1},
	},
	{
		{ 44100,   8000, RATIO_8, 20480, RATIO_8},
		{ 44100,  16000, RATIO_8, 40960, RATIO_8},
		{ 44100,  32000, RATIO_8, 61440, RATIO_8},
		{ 44100,  40000, RATIO_8, 80000, RATIO_8},
		{ 44100,  44100, RATIO_8, 61440, RATIO_8},
		{ 44100,  48000, RATIO_8, 61440, RATIO_8},
		{ 44100,  96000, RATIO_8, 61440, RATIO_2},
		{ 44100, 192000, RATIO_8, 61440, RATIO_2},
		{ 44100, 384000, RATIO_8, 61440, RATIO_1},
	},
	{
		{ 48000,   8000, RATIO_8, 16384, RATIO_8},
		{ 48000,  16000, RATIO_8, 32768, RATIO_8},
		{ 48000,  32000, RATIO_8, 65536, RATIO_8},
		{ 48000,  40000, RATIO_8, 51200, RATIO_8},
		{ 48000,  44100, RATIO_8, 88200, RATIO_8},
		{ 48000,  48000, RATIO_8, 98304, RATIO_8},
		{ 48000,  96000, RATIO_8, 32768, RATIO_2},
		{ 48000, 192000, RATIO_8, 65536, RATIO_2},
		{ 48000, 384000, RATIO_8, 65536, RATIO_1},
	},
	{
		{ 96000,   8000, RATIO_2, 32768, RATIO_8},
		{ 96000,  16000, RATIO_2, 65536, RATIO_8},
		{ 96000,  32000, RATIO_2, 98304, RATIO_8},
		{ 96000,  40000, RATIO_4, 51200, RATIO_8},
		{ 96000,  44100, RATIO_4, 88200, RATIO_8},
		{ 96000,  48000, RATIO_4, 65536, RATIO_8},
		{ 96000,  96000, RATIO_4, 98304, RATIO_8},
		{ 96000, 192000, RATIO_4, 98304, RATIO_4},
		{ 96000, 384000, RATIO_4, 98304, RATIO_2},
	},
	{
		{192000,   8000, RATIO_2, 16384, RATIO_8},
		{192000,  16000, RATIO_2, 32768, RATIO_8},
		{192000,  32000, RATIO_2, 32768, RATIO_8},
		{192000,  40000, RATIO_2, 51200, RATIO_8},
		{192000,  44100, RATIO_4, 44100, RATIO_8},
		{192000,  48000, RATIO_2, 98304, RATIO_8},
		{192000,  96000, RATIO_1, 98304, RATIO_2},
		{192000, 192000, RATIO_1, 98304, RATIO_1},
		{192000, 384000, RATIO_2, 98304, RATIO_1},
	},
	{
		{384000,   8000, RATIO_1, 16384, RATIO_8},
		{384000,  16000, RATIO_1, 32768, RATIO_8},
		{384000,  32000, RATIO_1, 32768, RATIO_8},
		{384000,  40000, RATIO_1, 51200, RATIO_8},
		{384000,  44100, RATIO_1, 56448, RATIO_8},
		{384000,  48000, RATIO_1, 32768, RATIO_4},
		{384000,  96000, RATIO_1, 65536, RATIO_4},
		{384000, 192000, RATIO_1, 98304, RATIO_2},
		{384000, 384000, RATIO_1, 98304, RATIO_1},
	},
};

static unsigned int cal_ofactor(const struct asrc_ctrl *ctrl)
{
	unsigned int isr, osr, ofactor;

	isr = (ctrl->isr / 100) << ctrl->ovsf;
	osr = (ctrl->osr / 100) << ctrl->dcmf;
	ofactor = ctrl->ifactor * isr / osr;

	return ofactor;
}

static unsigned int is_limit(unsigned int is_default)
{
	return is_default / (100 / 5); /* 5% */
}

static unsigned int os_limit(unsigned int os_default)
{
	return os_default / (100 / 5); /* 5% */
}

static int sif_idx(enum ABOX_CONFIGMSG configmsg)
{
	return configmsg - ((configmsg < SET_SIFS0_FORMAT) ?
			SET_SIFS0_RATE : SET_SIFS0_FORMAT);
}

static unsigned int get_sif_rate_min(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	return data->sif_rate_min[sif_idx(configmsg)];
}

static void set_sif_rate_min(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, unsigned int val)
{
	data->sif_rate_min[sif_idx(configmsg)] = val;
}

static snd_pcm_format_t get_sif_format_min(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	return data->sif_format_min[sif_idx(configmsg)];
}

static void set_sif_format_min(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, snd_pcm_format_t val)
{
	data->sif_format_min[sif_idx(configmsg)] = val;
}

static int get_sif_width_min(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	return snd_pcm_format_width(get_sif_format_min(data, configmsg));
}

static void set_sif_width_min(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, int width)
{
	struct device *dev = &data->pdev->dev;
	snd_pcm_format_t format = SNDRV_PCM_FORMAT_S16;

	switch (width) {
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
		dev_warn(dev, "%s(%d): invalid argument\n", __func__, width);
	}

	set_sif_format_min(data, configmsg, format);
}

static unsigned int get_sif_channels_min(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	return data->sif_channels_min[sif_idx(configmsg)];
}

static void set_sif_channels_min(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, unsigned int val)
{
	data->sif_channels_min[sif_idx(configmsg)] = val;
}

static bool get_sif_auto_config(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	return data->sif_auto_config[sif_idx(configmsg)];
}

static void set_sif_auto_config(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, bool val)
{
	data->sif_auto_config[sif_idx(configmsg)] = val;
}

static unsigned int get_sif_rate(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	unsigned int val = data->sif_rate[sif_idx(configmsg)];
	unsigned int min = get_sif_rate_min(data, configmsg);

	return (get_sif_auto_config(data, configmsg) && (min > val)) ?
			min : val;
}

static void set_sif_rate(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, unsigned int val)
{
	data->sif_rate[sif_idx(configmsg)] = val;
}

static snd_pcm_format_t get_sif_format(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	snd_pcm_format_t val = data->sif_format[sif_idx(configmsg)];
	snd_pcm_format_t min = get_sif_format_min(data, configmsg);

	return (get_sif_auto_config(data, configmsg) && (min > val)) ?
			min : val;
}

static void set_sif_format(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, snd_pcm_format_t val)
{
	data->sif_format[sif_idx(configmsg)] = val;
}

static int __maybe_unused get_sif_physical_width(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	snd_pcm_format_t format = get_sif_format(data, configmsg);

	return snd_pcm_format_physical_width(format);
}

static int get_sif_width(struct abox_data *data, enum ABOX_CONFIGMSG configmsg)
{
	return snd_pcm_format_width(get_sif_format(data, configmsg));
}

static void __maybe_unused set_sif_width(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, int width)
{
	struct device *dev = &data->pdev->dev;
	snd_pcm_format_t format = SNDRV_PCM_FORMAT_S16;

	switch (width) {
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
		dev_err(dev, "%s(%d): invalid argument\n", __func__, width);
	}

	set_sif_format(data, configmsg, format);
}

static unsigned int get_sif_channels(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	unsigned int val = data->sif_channels[sif_idx(configmsg)];
	unsigned int min = get_sif_channels_min(data, configmsg);

	return (get_sif_auto_config(data, configmsg) && (min > val)) ?
			min : val;
}

static void set_sif_channels(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, unsigned int val)
{
	data->sif_channels[sif_idx(configmsg)] = val;
}

static int rate_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = get_sif_rate(data, reg);

	dev_dbg(dev, "%s(%#x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int rate_put_ipc(struct device *adev, unsigned int val,
		enum ABOX_CONFIGMSG configmsg)
{
	struct abox_data *data = dev_get_drvdata(adev);
	ABOX_IPC_MSG msg;
	struct IPC_ABOX_CONFIG_MSG *abox_config_msg = &msg.msg.config;
	int ret;

	dev_dbg(adev, "%s(%u, %#x)\n", __func__, val, configmsg);

	set_sif_rate(data, configmsg, val);

	msg.ipcid = IPC_ABOX_CONFIG;
	abox_config_msg->param1 = get_sif_rate(data, configmsg);
	abox_config_msg->msgtype = configmsg;
	ret = abox_request_ipc(adev, msg.ipcid, &msg, sizeof(msg), 0, 0);
	if (ret < 0)
		dev_err(adev, "%s(%u, %#x) failed: %d\n", __func__, val,
				configmsg, ret);

	return ret;
}

static int rate_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(%#x, %u)\n", __func__, reg, val);

	return rate_put_ipc(dev, val, reg);
}

static int format_put_ipc(struct device *adev, snd_pcm_format_t format,
		unsigned int channels, enum ABOX_CONFIGMSG configmsg)
{
	struct abox_data *data = dev_get_drvdata(adev);
	ABOX_IPC_MSG msg;
	struct IPC_ABOX_CONFIG_MSG *abox_config_msg = &msg.msg.config;
	int width = snd_pcm_format_width(format);
	int ret;

	dev_dbg(adev, "%s(%d, %u, %d)\n", __func__, width, channels, configmsg);

	set_sif_format(data, configmsg, format);
	set_sif_channels(data, configmsg, channels);

	width = get_sif_width(data, configmsg);
	channels = get_sif_channels(data, configmsg);

	/* update manually for regmap cache sync */
	switch (configmsg) {
	case SET_SIFS0_RATE:
	case SET_SIFS0_FORMAT:
		regmap_update_bits(data->regmap, ABOX_SPUS_CTRL1,
				ABOX_MIXP_FORMAT_MASK,
				abox_get_format(width, channels) <<
				ABOX_MIXP_FORMAT_L);
		break;
	case SET_PIFS1_RATE:
	case SET_PIFS1_FORMAT:
		regmap_update_bits(data->regmap, ABOX_SPUM_CTRL1,
				ABOX_RECP_SRC_FORMAT_MASK(1),
				abox_get_format(width, channels) <<
				ABOX_RECP_SRC_FORMAT_L(1));
		break;
	case SET_PIFS0_RATE:
	case SET_PIFS0_FORMAT:
		regmap_update_bits(data->regmap, ABOX_SPUM_CTRL1,
				ABOX_RECP_SRC_FORMAT_MASK(0),
				abox_get_format(width, channels) <<
				ABOX_RECP_SRC_FORMAT_L(0));
		break;
	default:
		/* Nothing to do */
		break;
	}

	msg.ipcid = IPC_ABOX_CONFIG;
	abox_config_msg->param1 = width;
	abox_config_msg->param2 = channels;
	abox_config_msg->msgtype = configmsg;
	ret = abox_request_ipc(adev, msg.ipcid, &msg, sizeof(msg), 0, 0);
	if (ret < 0)
		dev_err(adev, "%d(%d bit, %u channels) failed: %d\n", configmsg,
				width, channels, ret);

	return ret;
}

static int width_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = get_sif_width(data, reg);

	dev_dbg(dev, "%s(%#x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int width_put_ipc(struct device *dev, unsigned int val,
		enum ABOX_CONFIGMSG configmsg)
{
	struct abox_data *data = dev_get_drvdata(dev);
	snd_pcm_format_t format = SNDRV_PCM_FORMAT_S16;
	unsigned int channels = data->sif_channels[sif_idx(configmsg)];

	dev_dbg(dev, "%s(%u, %#x)\n", __func__, val, configmsg);

	switch (val) {
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
		dev_warn(dev, "%s(%u, %#x) invalid argument\n", __func__,
				val, configmsg);
		break;
	}

	if (configmsg == SET_PIFS1_FORMAT)
		format_put_ipc(dev, format, channels, SET_PIFS0_FORMAT);
	return format_put_ipc(dev, format, channels, configmsg);
}

static int width_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(%#x, %u)\n", __func__, reg, val);

	return width_put_ipc(dev, val, reg);
}

static int channels_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = get_sif_channels(data, reg);

	dev_dbg(dev, "%s(%#x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int channels_put_ipc(struct device *dev, unsigned int val,
		enum ABOX_CONFIGMSG configmsg)
{
	struct abox_data *data = dev_get_drvdata(dev);
	snd_pcm_format_t format = data->sif_format[sif_idx(configmsg)];
	unsigned int channels = val;

	dev_dbg(dev, "%s(%u, %#x)\n", __func__, val, configmsg);

	return format_put_ipc(dev, format, channels, configmsg);
}

static int channels_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(%#x, %u)\n", __func__, reg, val);

	return channels_put_ipc(dev, val, reg);
}

static int rate_min_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = get_sif_rate_min(data, reg);

	dev_dbg(dev, "%s(%#x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int rate_min_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(%#x, %u)\n", __func__, reg, val);

	set_sif_rate_min(data, reg, val);

	return 0;
}

static int width_min_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = get_sif_width_min(data, reg);

	dev_dbg(dev, "%s(%#x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int width_min_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(%#x, %u)\n", __func__, reg, val);

	set_sif_width_min(data, reg, val);

	return 0;
}

static int channels_min_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = get_sif_channels_min(data, reg);

	dev_dbg(dev, "%s(%#x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int channels_min_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(%#x, %u)\n", __func__, reg, val);

	set_sif_channels_min(data, reg, val);

	return 0;
}

static int auto_config_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = get_sif_auto_config(data, reg);

	dev_dbg(dev, "%s(%#x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int auto_config_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(%#x, %u)\n", __func__, reg, val);

	set_sif_auto_config(data, reg, !!val);

	return 0;
}

static int audio_mode_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int item;

	dev_dbg(dev, "%s: %u\n", __func__, data->audio_mode);

	item = snd_soc_enum_val_to_item(e, data->audio_mode);
	ucontrol->value.enumerated.item[0] = item;

	return 0;
}

static int audio_mode_put_ipc(struct device *dev, enum audio_mode mode)
{
	struct abox_data *data = dev_get_drvdata(dev);
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	dev_dbg(dev, "%s(%d)\n", __func__, mode);

	if (IS_ENABLED(CONFIG_SOC_EXYNOS9810)) {
		static const unsigned int BIG_FREQ = 1248000;
		static const unsigned int LIT_FREQ = 1274000;
		static const char cookie[] = "audio_mode";

		switch (mode) {
		case MODE_IN_COMMUNICATION:
			abox_qos_request_cl1(dev, DEFAULT_BIG_FREQ_ID, BIG_FREQ,
					cookie);
			abox_qos_request_cl0(dev, DEFAULT_LIT_FREQ_ID, LIT_FREQ,
					cookie);
			break;
		default:
			abox_qos_request_cl1(dev, DEFAULT_BIG_FREQ_ID, 0,
					cookie);
			abox_qos_request_cl0(dev, DEFAULT_LIT_FREQ_ID, 0,
					cookie);
			break;
		}
	}

	data->audio_mode_time = local_clock();

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_SET_MODE;
	system_msg->param1 = data->audio_mode = mode;
	return abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
}

static int audio_mode_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum audio_mode mode;

	if (item[0] >= e->items)
		return -EINVAL;

	mode = snd_soc_enum_item_to_val(e, item[0]);
	dev_info(dev, "%s(%u)\n", __func__, mode);

	return audio_mode_put_ipc(dev, mode);
}

static const char * const audio_mode_enum_texts[] = {
	"NORMAL",
	"RINGTONE",
	"IN_CALL",
	"IN_COMMUNICATION",
	"IN_VIDEOCALL",
};
static const unsigned int audio_mode_enum_values[] = {
	MODE_NORMAL,
	MODE_RINGTONE,
	MODE_IN_CALL,
	MODE_IN_COMMUNICATION,
	MODE_IN_VIDEOCALL,
};
SOC_VALUE_ENUM_SINGLE_DECL(audio_mode_enum, SND_SOC_NOPM, 0, 0,
		audio_mode_enum_texts, audio_mode_enum_values);

static int sound_type_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int item;

	dev_dbg(dev, "%s: %u\n", __func__, data->sound_type);

	item = snd_soc_enum_val_to_item(e, data->sound_type);
	ucontrol->value.enumerated.item[0] = item;

	return 0;
}

static int sound_type_put_ipc(struct device *dev, enum sound_type type)
{
	struct abox_data *data = dev_get_drvdata(dev);
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	dev_dbg(dev, "%s(%d)\n", __func__, type);

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_SET_TYPE;
	system_msg->param1 = data->sound_type = type;

	return abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
}

static int sound_type_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum sound_type type;

	if (item[0] >= e->items)
		return -EINVAL;

	type = snd_soc_enum_item_to_val(e, item[0]);
	dev_info(dev, "%s(%d)\n", __func__, type);

	return sound_type_put_ipc(dev, type);
}
static const char * const sound_type_enum_texts[] = {
	"VOICE",
	"SPEAKER",
	"HEADSET",
	"BTVOICE",
	"USB",
	"CALLFWD",
};
static const unsigned int sound_type_enum_values[] = {
	SOUND_TYPE_VOICE,
	SOUND_TYPE_SPEAKER,
	SOUND_TYPE_HEADSET,
	SOUND_TYPE_BTVOICE,
	SOUND_TYPE_USB,
	SOUND_TYPE_CALLFWD,
};
SOC_VALUE_ENUM_SINGLE_DECL(sound_type_enum, SND_SOC_NOPM, 0, 0,
		sound_type_enum_texts, sound_type_enum_values);

static int tickle_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	ucontrol->value.integer.value[0] = data->enabled;

	return 0;
}

static int tickle_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	long val = ucontrol->value.integer.value[0];

	dev_dbg(dev, "%s(%ld)\n", __func__, val);

	if (!!val)
		pm_request_resume(dev);

	return 0;
}

static unsigned int s_default = 36864;

static int asrc_factor_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = s_default;

	dev_dbg(dev, "%s(%#x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int asrc_factor_put_ipc(struct device *adev, unsigned int val,
		enum ABOX_CONFIGMSG configmsg)
{
	ABOX_IPC_MSG msg;
	struct IPC_ABOX_CONFIG_MSG *abox_config_msg = &msg.msg.config;
	int ret;

	dev_dbg(adev, "%s(%u, %#x)\n", __func__, val, configmsg);

	s_default = val;

	msg.ipcid = IPC_ABOX_CONFIG;
	abox_config_msg->param1 = s_default;
	abox_config_msg->msgtype = configmsg;
	ret = abox_request_ipc(adev, msg.ipcid, &msg, sizeof(msg), 0, 0);
	if (ret < 0)
		dev_err(adev, "%s(%u, %#x) failed: %d\n", __func__, val,
				configmsg, ret);

	return ret;
}

static int asrc_factor_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(%#x, %u)\n", __func__, reg, val);

	return asrc_factor_put_ipc(dev, val, reg);
}

static bool spus_asrc_force_enable[] = {
	false, false, false, false,
	false, false, false, false,
	false, false, false, false
};

static bool spum_asrc_force_enable[] = {
	false, false, false, false,
	false, false, false, false,
};

static int spus_asrc_enable_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	long val = ucontrol->value.integer.value[0];
	int idx, ret;

	ret = sscanf(kcontrol->id.name, "%*s %*s ASRC%d", &idx);
	if (ret < 1) {
		dev_err(dev, "%s(%s): %d\n", __func__, kcontrol->id.name, ret);
		return ret;
	}
	if (idx < 0 || idx >= ARRAY_SIZE(spus_asrc_force_enable)) {
		dev_err(dev, "%s(%s): %d\n", __func__, kcontrol->id.name, idx);
		return -EINVAL;
	}

	dev_info(dev, "%s(%ld, %d)\n", __func__, val, idx);

	spus_asrc_force_enable[idx] = val;

	return snd_soc_put_volsw(kcontrol, ucontrol);
}

static int spum_asrc_enable_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	long val = ucontrol->value.integer.value[0];
	int idx, ret;

	ret = sscanf(kcontrol->id.name, "%*s %*s ASRC%d", &idx);
	if (ret < 1) {
		dev_err(dev, "%s(%s): %d\n", __func__, kcontrol->id.name, ret);
		return ret;
	}
	if (idx < 0 || idx >= ARRAY_SIZE(spum_asrc_force_enable)) {
		dev_err(dev, "%s(%s): %d\n", __func__, kcontrol->id.name, idx);
		return -EINVAL;
	}

	dev_info(dev, "%s(%ld, %d)\n", __func__, val, idx);

	spum_asrc_force_enable[idx] = val;

	return snd_soc_put_volsw(kcontrol, ucontrol);
}

static int get_apf_coef(struct abox_data *data, int stream, int idx)
{
	return data->apf_coef[stream ? 1 : 0][idx];
}

static void set_apf_coef(struct abox_data *data, int stream, int idx, int coef)
{
	data->apf_coef[stream ? 1 : 0][idx] = coef;
}

static int asrc_apf_coef_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	int stream = mc->reg;
	int idx = mc->shift;

	dev_dbg(dev, "%s(%d, %d)\n", __func__, stream, idx);

	ucontrol->value.integer.value[0] = get_apf_coef(data, stream, idx);

	return 0;
}

static int asrc_apf_coef_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	int stream = mc->reg;
	int idx = mc->shift;
	long val = ucontrol->value.integer.value[0];

	dev_dbg(dev, "%s(%d, %d, %ld)\n", __func__, stream, idx, val);

	set_apf_coef(data, stream, idx, val);

	return 0;
}

static int wake_lock_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int val = data->ws.active;

	dev_dbg(dev, "%s: %u\n", __func__, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int wake_lock_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(%u)\n", __func__, val);

	if (val)
		__pm_stay_awake(&data->ws);
	else
		__pm_relax(&data->ws);

	return 0;
}

static enum asrc_tick spus_asrc_os[] = {
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
};
static enum asrc_tick spus_asrc_is[] = {
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC
};

static enum asrc_tick spum_asrc_os[] = {
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
};
static enum asrc_tick spum_asrc_is[] = {
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
};

static int spus_asrc_os_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick = spus_asrc_os[idx];

	dev_dbg(dev, "%s(%d): %d\n", __func__, idx, tick);

	item[0] = snd_soc_enum_val_to_item(e, tick);

	return 0;
}

static int spus_asrc_os_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick;

	if (item[0] >= e->items)
		return -EINVAL;

	tick = snd_soc_enum_item_to_val(e, item[0]);
	dev_dbg(dev, "%s(%d, %d)\n", __func__, idx, tick);
	spus_asrc_os[idx] = tick;

	return 0;
}

static int spus_asrc_is_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick = spus_asrc_is[idx];

	dev_dbg(dev, "%s(%d): %d\n", __func__, idx, tick);

	item[0] = snd_soc_enum_val_to_item(e, tick);

	return 0;
}

static int spus_asrc_is_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick;

	if (item[0] >= e->items)
		return -EINVAL;

	tick = snd_soc_enum_item_to_val(e, item[0]);
	dev_dbg(dev, "%s(%d, %d)\n", __func__, idx, tick);
	spus_asrc_is[idx] = tick;

	return 0;
}

static int spum_asrc_os_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick = spum_asrc_os[idx];

	dev_dbg(dev, "%s(%d): %d\n", __func__, idx, tick);

	item[0] = snd_soc_enum_val_to_item(e, tick);

	return 0;
}

static int spum_asrc_os_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick;

	if (item[0] >= e->items)
		return -EINVAL;

	tick = snd_soc_enum_item_to_val(e, item[0]);
	dev_dbg(dev, "%s(%d, %d)\n", __func__, idx, tick);
	spum_asrc_os[idx] = tick;

	return 0;
}

static int spum_asrc_is_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick = spum_asrc_is[idx];

	dev_dbg(dev, "%s(%d): %d\n", __func__, idx, tick);

	item[0] = snd_soc_enum_val_to_item(e, tick);

	return 0;
}

static int spum_asrc_is_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick;

	if (item[0] >= e->items)
		return -EINVAL;

	tick = snd_soc_enum_item_to_val(e, item[0]);
	dev_dbg(dev, "%s(%d, %d)\n", __func__, idx, tick);
	spum_asrc_is[idx] = tick;

	return 0;
}

static const struct snd_kcontrol_new cmpnt_controls[] = {
	SOC_SINGLE_EXT("SIFS0 Rate", SET_SIFS0_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS1 Rate", SET_SIFS1_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS2 Rate", SET_SIFS2_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS3 Rate", SET_SIFS3_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS4 Rate", SET_SIFS4_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("PIFS Rate", SET_PIFS1_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM0 Rate", SET_SIFM0_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM1 Rate", SET_SIFM1_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM2 Rate", SET_SIFM2_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM3 Rate", SET_SIFM3_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM4 Rate", SET_SIFM4_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM5 Rate", SET_SIFM5_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM6 Rate", SET_SIFM6_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS0 Width", SET_SIFS0_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS1 Width", SET_SIFS1_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS2 Width", SET_SIFS2_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS3 Width", SET_SIFS3_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS4 Width", SET_SIFS4_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("PIFS Width", SET_PIFS1_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM0 Width", SET_SIFM0_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM1 Width", SET_SIFM1_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM2 Width", SET_SIFM2_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM3 Width", SET_SIFM3_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM4 Width", SET_SIFM4_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM5 Width", SET_SIFM5_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM6 Width", SET_SIFM6_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS0 Channel", SET_SIFS0_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("PIFS0 Channel", SET_PIFS0_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("PIFS1 Channel", SET_PIFS1_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFS0 Rate Min", SET_SIFS0_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFS1 Rate Min", SET_SIFS1_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFS2 Rate Min", SET_SIFS2_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFS3 Rate Min", SET_SIFS3_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFS4 Rate Min", SET_SIFS4_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("PIFS Rate Min", SET_PIFS1_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFM0 Rate Min", SET_SIFM0_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFM1 Rate Min", SET_SIFM1_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFM2 Rate Min", SET_SIFM2_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFM3 Rate Min", SET_SIFM3_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFM4 Rate Min", SET_SIFM4_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFM5 Rate Min", SET_SIFM5_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFM6 Rate Min", SET_SIFM6_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFS0 Width Min", SET_SIFS0_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFS1 Width Min", SET_SIFS1_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFS2 Width Min", SET_SIFS2_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFS3 Width Min", SET_SIFS3_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFS4 Width Min", SET_SIFS4_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("PIFS Width Min", SET_PIFS1_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFM0 Width Min", SET_SIFM0_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFM1 Width Min", SET_SIFM1_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFM2 Width Min", SET_SIFM2_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFM3 Width Min", SET_SIFM3_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFM4 Width Min", SET_SIFM4_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFM5 Width Min", SET_SIFM5_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFM6 Width Min", SET_SIFM6_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFS0 Channel Min", SET_SIFS0_FORMAT, 1, 8, 0,
			channels_min_get, channels_min_put),
	SOC_SINGLE_EXT("PIFS0 Channel Min", SET_PIFS0_FORMAT, 1, 8, 0,
			channels_min_get, channels_min_put),
	SOC_SINGLE_EXT("PIFS1 Channel Min", SET_PIFS1_FORMAT, 1, 8, 0,
			channels_min_get, channels_min_put),
	SOC_SINGLE_EXT("SIFS0 Auto Config", SET_SIFS0_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFS1 Auto Config", SET_SIFS1_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFS2 Auto Config", SET_SIFS2_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFS3 Auto Config", SET_SIFS3_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFS4 Auto Config", SET_SIFS4_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("PIFS Auto Config", SET_PIFS1_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFM0 Auto Config", SET_SIFM0_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFM1 Auto Config", SET_SIFM1_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFM2 Auto Config", SET_SIFM2_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFM3 Auto Config", SET_SIFM3_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFM4 Auto Config", SET_SIFM4_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFM5 Auto Config", SET_SIFM5_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFM6 Auto Config", SET_SIFM6_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_VALUE_ENUM_EXT("Audio Mode", audio_mode_enum,
			audio_mode_get, audio_mode_put),
	SOC_VALUE_ENUM_EXT("Sound Type", sound_type_enum,
			sound_type_get, sound_type_put),
	SOC_SINGLE_EXT("Tickle", 0, 0, 1, 0, tickle_get, tickle_put),
	SOC_SINGLE_EXT("Wakelock", 0, 0, 1, 0,
			wake_lock_get, wake_lock_put),
	SOC_SINGLE("RSRC0 Bridge", ABOX_ROUTE_CTRL2,
			ABOX_RSRC_CONNECTION_TYPE_L(0), 1, 0),
	SOC_SINGLE("RSRC1 Bridge", ABOX_ROUTE_CTRL2,
			ABOX_RSRC_CONNECTION_TYPE_L(1), 1, 0),
	SOC_SINGLE("NSRC0 Bridge", ABOX_ROUTE_CTRL2,
			ABOX_NSRC_CONNECTION_TYPE_L(0), 1, 0),
	SOC_SINGLE("NSRC1 Bridge", ABOX_ROUTE_CTRL2,
			ABOX_NSRC_CONNECTION_TYPE_L(1), 1, 0),
	SOC_SINGLE("NSRC2 Bridge", ABOX_ROUTE_CTRL2,
			ABOX_NSRC_CONNECTION_TYPE_L(2), 1, 0),
	SOC_SINGLE("NSRC3 Bridge", ABOX_ROUTE_CTRL2,
			ABOX_NSRC_CONNECTION_TYPE_L(3), 1, 0),
	SOC_SINGLE("NSRC4 Bridge", ABOX_ROUTE_CTRL2,
			ABOX_NSRC_CONNECTION_TYPE_L(4), 1, 0),
	SOC_SINGLE("NSRC5 Bridge", ABOX_ROUTE_CTRL2,
			ABOX_NSRC_CONNECTION_TYPE_L(5), 1, 0),
	SOC_SINGLE("NSRC6 Bridge", ABOX_ROUTE_CTRL2,
			ABOX_NSRC_CONNECTION_TYPE_L(6), 1, 0),
	SOC_SINGLE_EXT("ASRC Factor CP", SET_ASRC_FACTOR_CP, 0, 0x1ffff, 0,
			asrc_factor_get, asrc_factor_put),
};

#if (ABOX_SOC_VERSION(2, 0, 1) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
static const struct snd_kcontrol_new spus_asrc_controls[] = {
	SOC_SINGLE_EXT("SPUS ASRC0", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(0), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC1", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(1), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC2", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(2), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC3", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(3), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC4", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(4), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC5", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(5), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC6", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(6), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC7", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(7), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC8", ABOX_SPUS_CTRL4,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(8), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC9", ABOX_SPUS_CTRL4,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(9), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC10", ABOX_SPUS_CTRL4,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(10), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC11", ABOX_SPUS_CTRL4,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(11), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
};
#else
static const struct snd_kcontrol_new spus_asrc_controls[] = {
	SOC_SINGLE_EXT("SPUS ASRC0", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(0), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC1", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(1), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC2", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(2), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC3", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(3), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC4", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(4), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC5", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(5), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC6", ABOX_SPUS_CTRL_FC1,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(0), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC7", ABOX_SPUS_CTRL_FC1,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(1), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC8", ABOX_SPUS_CTRL_FC1,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(2), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC9", ABOX_SPUS_CTRL_FC1,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(3), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC10", ABOX_SPUS_CTRL_FC1,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(4), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC11", ABOX_SPUS_CTRL_FC1,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(5), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
};
#endif
static const struct snd_kcontrol_new spum_asrc_controls[] = {
	SOC_SINGLE_EXT("SPUM ASRC0", ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_RSRC_ASRC_L, 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC1", ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(0), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC2", ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(1), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC3", ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(2), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC4", ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(3), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC5", ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(4), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC6", ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(5), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC7", ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(6), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
};

static const struct snd_kcontrol_new spus_asrc_id_controls[] = {
	SOC_SINGLE("SPUS ASRC0 ID", ABOX_SPUS_CTRL4,
			ABOX_SRC_ASRC_ID_L(0), 11, 0),
	SOC_SINGLE("SPUS ASRC1 ID", ABOX_SPUS_CTRL4,
			ABOX_SRC_ASRC_ID_L(1), 11, 0),
	SOC_SINGLE("SPUS ASRC2 ID", ABOX_SPUS_CTRL4,
			ABOX_SRC_ASRC_ID_L(2), 11, 0),
	SOC_SINGLE("SPUS ASRC3 ID", ABOX_SPUS_CTRL4,
			ABOX_SRC_ASRC_ID_L(3), 11, 0),
	SOC_SINGLE("SPUS ASRC4 ID", ABOX_SPUS_CTRL5,
			ABOX_SRC_ASRC_ID_L(4), 11, 0),
	SOC_SINGLE("SPUS ASRC5 ID", ABOX_SPUS_CTRL5,
			ABOX_SRC_ASRC_ID_L(5), 11, 0),
	SOC_SINGLE("SPUS ASRC6 ID", ABOX_SPUS_CTRL5,
			ABOX_SRC_ASRC_ID_L(6), 11, 0),
	SOC_SINGLE("SPUS ASRC7 ID", ABOX_SPUS_CTRL5,
			ABOX_SRC_ASRC_ID_L(7), 11, 0),
	SOC_SINGLE("SPUS ASRC8 ID", ABOX_SPUS_CTRL5,
			ABOX_SRC_ASRC_ID_L(8), 11, 0),
	SOC_SINGLE("SPUS ASRC9 ID", ABOX_SPUS_CTRL5,
			ABOX_SRC_ASRC_ID_L(9), 11, 0),
	SOC_SINGLE("SPUS ASRC10 ID", ABOX_SPUS_CTRL5,
			ABOX_SRC_ASRC_ID_L(10), 11, 0),
	SOC_SINGLE("SPUS ASRC11 ID", ABOX_SPUS_CTRL5,
			ABOX_SRC_ASRC_ID_L(11), 11, 0),
};

static const struct snd_kcontrol_new spum_asrc_id_controls[] = {
	SOC_SINGLE("SPUM ASRC0 ID", ABOX_SPUM_CTRL4,
			ABOX_RSRC_ASRC_ID_L, 7, 0),
	SOC_SINGLE("SPUM ASRC1 ID", ABOX_SPUM_CTRL4,
			ABOX_NSRC_ASRC_ID_L(0), 7, 0),
	SOC_SINGLE("SPUM ASRC2 ID", ABOX_SPUM_CTRL4,
			ABOX_NSRC_ASRC_ID_L(1), 7, 0),
	SOC_SINGLE("SPUM ASRC3 ID", ABOX_SPUM_CTRL4,
			ABOX_NSRC_ASRC_ID_L(2), 7, 0),
	SOC_SINGLE("SPUM ASRC4 ID", ABOX_SPUM_CTRL4,
			ABOX_NSRC_ASRC_ID_L(3), 7, 0),
	SOC_SINGLE("SPUM ASRC5 ID", ABOX_SPUM_CTRL4,
			ABOX_NSRC_ASRC_ID_L(4), 7, 0),
	SOC_SINGLE("SPUM ASRC6 ID", ABOX_SPUM_CTRL4,
			ABOX_NSRC_ASRC_ID_L(5), 7, 0),
	SOC_SINGLE("SPUM ASRC7 ID", ABOX_SPUM_CTRL4,
			ABOX_NSRC_ASRC_ID_L(6), 7, 0),
};

static const struct snd_kcontrol_new spus_asrc_apf_coef_controls[] = {
	SOC_SINGLE_EXT("SPUS ASRC0 APF COEF", SNDRV_PCM_STREAM_PLAYBACK, 0, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC1 APF COEF", SNDRV_PCM_STREAM_PLAYBACK, 1, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC2 APF COEF", SNDRV_PCM_STREAM_PLAYBACK, 2, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC3 APF COEF", SNDRV_PCM_STREAM_PLAYBACK, 3, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC4 APF COEF", SNDRV_PCM_STREAM_PLAYBACK, 4, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC5 APF COEF", SNDRV_PCM_STREAM_PLAYBACK, 5, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC6 APF COEF", SNDRV_PCM_STREAM_PLAYBACK, 6, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC7 APF COEF", SNDRV_PCM_STREAM_PLAYBACK, 7, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC8 APF COEF", SNDRV_PCM_STREAM_PLAYBACK, 8, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC9 APF COEF", SNDRV_PCM_STREAM_PLAYBACK, 9, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC10 APF COEF", SNDRV_PCM_STREAM_PLAYBACK, 10, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC11 APF COEF", SNDRV_PCM_STREAM_PLAYBACK, 11, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
};

static const struct snd_kcontrol_new spum_asrc_apf_coef_controls[] = {
	SOC_SINGLE_EXT("SPUM ASRC0 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 0, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC1 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 1, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC2 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 2, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC3 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 3, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC4 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 4, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC5 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 5, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC6 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 6, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC7 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 7, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
};

static const char * const asrc_source_enum_texts[] = {
	"CP",
	"ABOX",
	"UAIF0",
	"UAIF1",
	"UAIF2",
	"UAIF3",
	"USB",
};

static const unsigned int asrc_source_enum_values[] = {
	TICK_CP,
	TICK_SYNC,
	TICK_UAIF0,
	TICK_UAIF1,
	TICK_UAIF2,
	TICK_UAIF3,
	TICK_USB,
};

static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc0_os_enum, 0, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc1_os_enum, 1, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc2_os_enum, 2, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc3_os_enum, 3, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc4_os_enum, 4, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc5_os_enum, 5, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc6_os_enum, 6, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc7_os_enum, 7, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc8_os_enum, 8, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc9_os_enum, 9, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc10_os_enum, 10, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc11_os_enum, 11, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);

static const struct snd_kcontrol_new spus_asrc_os_controls[] = {
	SOC_VALUE_ENUM_EXT("SPUS ASRC0 OS", spus_asrc0_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC1 OS", spus_asrc1_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC2 OS", spus_asrc2_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC3 OS", spus_asrc3_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC4 OS", spus_asrc4_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC5 OS", spus_asrc5_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC6 OS", spus_asrc6_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC7 OS", spus_asrc7_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC8 OS", spus_asrc8_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC9 OS", spus_asrc9_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC10 OS", spus_asrc10_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC11 OS", spus_asrc11_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
};

static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc0_is_enum, 0, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc1_is_enum, 1, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc2_is_enum, 2, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc3_is_enum, 3, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc4_is_enum, 4, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc5_is_enum, 5, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc6_is_enum, 6, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc7_is_enum, 7, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc8_is_enum, 8, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc9_is_enum, 9, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc10_is_enum, 10, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc11_is_enum, 11, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);

static const struct snd_kcontrol_new spus_asrc_is_controls[] = {
	SOC_VALUE_ENUM_EXT("SPUS ASRC0 IS", spus_asrc0_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC1 IS", spus_asrc1_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC2 IS", spus_asrc2_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC3 IS", spus_asrc3_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC4 IS", spus_asrc4_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC5 IS", spus_asrc5_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC6 IS", spus_asrc6_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC7 IS", spus_asrc7_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC8 IS", spus_asrc8_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC9 IS", spus_asrc9_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC10 IS", spus_asrc10_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC11 IS", spus_asrc11_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
};

static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc0_os_enum, 0, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc1_os_enum, 1, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc2_os_enum, 2, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc3_os_enum, 3, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc4_os_enum, 4, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc5_os_enum, 5, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc6_os_enum, 6, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc7_os_enum, 7, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);

static const struct snd_kcontrol_new spum_asrc_os_controls[] = {
	SOC_VALUE_ENUM_EXT("SPUM ASRC0 OS", spum_asrc0_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC1 OS", spum_asrc1_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC2 OS", spum_asrc2_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC3 OS", spum_asrc3_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC4 OS", spum_asrc4_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC5 OS", spum_asrc5_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC6 OS", spum_asrc6_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
};

static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc0_is_enum, 0, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc1_is_enum, 1, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc2_is_enum, 2, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc3_is_enum, 3, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc4_is_enum, 4, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc5_is_enum, 5, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc6_is_enum, 6, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc7_is_enum, 7, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);

static const struct snd_kcontrol_new spum_asrc_is_controls[] = {
	SOC_VALUE_ENUM_EXT("SPUM ASRC0 IS", spum_asrc0_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC1 IS", spum_asrc1_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC2 IS", spum_asrc2_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC3 IS", spum_asrc3_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC4 IS", spum_asrc4_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC5 IS", spum_asrc5_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC6 IS", spum_asrc6_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC7 IS", spum_asrc7_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
};

struct snd_soc_dapm_widget *spus_asrc_widgets[ARRAY_SIZE(spus_asrc_controls)];
struct snd_soc_dapm_widget *spum_asrc_widgets[ARRAY_SIZE(spum_asrc_controls)];

static void asrc_cache_widget(struct snd_soc_dapm_widget *w,
		int idx, int stream)
{
	struct device *dev = w->dapm->dev;

	dev_dbg(dev, "%s(%s, %d, %d)\n", __func__, w->name, idx, stream);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		spus_asrc_widgets[idx] = w;
	else
		spum_asrc_widgets[idx] = w;
}

static struct snd_soc_dapm_widget *asrc_find_widget(
		struct snd_soc_card *card, int idx, int stream)
{
	struct device *dev = card->dev;
	struct snd_soc_dapm_widget *w;
	const char *name;

	dev_dbg(dev, "%s(%s, %d, %d)\n", __func__, card->name, idx, stream);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		name = spus_asrc_controls[idx].name;
	else
		name = spum_asrc_controls[idx].name;

	list_for_each_entry(w, &card->widgets, list) {
		struct snd_soc_component *cmpnt = w->dapm->component;
		const char *name_prefix = cmpnt ? cmpnt->name_prefix : NULL;
		size_t prefix_len = name_prefix ? strlen(name_prefix) + 1 : 0;
		const char *w_name = w->name + prefix_len;

		if (!strcmp(name, w_name))
			return w;
	}

	return NULL;
}

static struct snd_soc_dapm_widget *asrc_get_widget(
		struct snd_soc_component *cmpnt, int idx, int stream)
{
	struct snd_soc_dapm_widget *w;
	struct device *dev;

	if (idx < 0)
		return NULL;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		w = spus_asrc_widgets[idx];
	else
		w = spum_asrc_widgets[idx];

	if (!w) {
		w = asrc_find_widget(cmpnt->card, idx, stream);
		asrc_cache_widget(w, idx, stream);
	}

	dev = w->dapm->dev;
	dev_dbg(dev, "%s(%d, %d): %s\n", __func__, idx, stream, w->name);

	return w;
}

struct name_rate_format {
	const char *name;
	int stream;
	const enum ABOX_CONFIGMSG rate;
	const enum ABOX_CONFIGMSG format;
	unsigned int sfr;
	unsigned int shift;
};

static const struct name_rate_format nrf_list[] = {
	{"SIFS0", SNDRV_PCM_STREAM_PLAYBACK, SET_SIFS0_RATE, SET_SIFS0_FORMAT,
			0, 0},
	{"SIFS1", SNDRV_PCM_STREAM_PLAYBACK, SET_SIFS1_RATE, SET_SIFS1_FORMAT,
			0, 0},
	{"SIFS2", SNDRV_PCM_STREAM_PLAYBACK, SET_SIFS2_RATE, SET_SIFS2_FORMAT,
			0, 0},
	{"SIFS3", SNDRV_PCM_STREAM_PLAYBACK, SET_SIFS3_RATE, SET_SIFS3_FORMAT,
			0, 0},
	{"SIFS4", SNDRV_PCM_STREAM_PLAYBACK, SET_SIFS4_RATE, SET_SIFS4_FORMAT,
			0, 0},
	{"RSRC0", SNDRV_PCM_STREAM_CAPTURE, SET_PIFS0_RATE, SET_PIFS0_FORMAT,
			ABOX_ROUTE_CTRL2, ABOX_ROUTE_RSRC_H(0)},
	{"RSRC1", SNDRV_PCM_STREAM_CAPTURE, SET_PIFS1_RATE, SET_PIFS1_FORMAT,
			ABOX_ROUTE_CTRL2, ABOX_ROUTE_RSRC_H(1)},
	{"NSRC0", SNDRV_PCM_STREAM_CAPTURE, SET_SIFM0_RATE, SET_SIFM0_FORMAT,
			ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_H(0)},
	{"NSRC1", SNDRV_PCM_STREAM_CAPTURE, SET_SIFM1_RATE, SET_SIFM1_FORMAT,
			ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_H(1)},
	{"NSRC2", SNDRV_PCM_STREAM_CAPTURE, SET_SIFM2_RATE, SET_SIFM2_FORMAT,
			ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_H(2)},
	{"NSRC3", SNDRV_PCM_STREAM_CAPTURE, SET_SIFM3_RATE, SET_SIFM3_FORMAT,
			ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_H(3)},
	{"NSRC4", SNDRV_PCM_STREAM_CAPTURE, SET_SIFM4_RATE, SET_SIFM4_FORMAT,
			ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_H(4)},
	{"NSRC5", SNDRV_PCM_STREAM_CAPTURE, SET_SIFM5_RATE, SET_SIFM5_FORMAT,
			ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_H(5)},
	{"NSRC6", SNDRV_PCM_STREAM_CAPTURE, SET_SIFM6_RATE, SET_SIFM6_FORMAT,
			ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_H(6)},
};

static bool find_nrf_stream(const struct snd_soc_dapm_widget *w,
		int stream, enum ABOX_CONFIGMSG *rate,
		enum ABOX_CONFIGMSG *format, bool *slave)
{
	struct snd_soc_component *cmpnt = w->dapm->component;
	struct device *dev = cmpnt ? cmpnt->dev : NULL;
	const char *name_prefix = cmpnt ? cmpnt->name_prefix : NULL;
	size_t prefix_len = name_prefix ? strlen(name_prefix) + 1 : 0;
	const char *name = w->name + prefix_len;
	const struct name_rate_format *nrf;
	unsigned int val = 0xffffffff;
	int ret;

	for (nrf = nrf_list; nrf - nrf_list < ARRAY_SIZE(nrf_list); nrf++) {
		if ((nrf->stream != stream) || strcmp(nrf->name, name))
			continue;

		*rate = nrf->rate;
		*format = nrf->format;
		if (nrf->sfr) {
			ret = snd_soc_component_read(cmpnt, nrf->sfr, &val);
			if (ret < 0)
				dev_err(dev, "%s: read failed: %d\n",
					__func__, ret);
		}
		*slave = !((val >> nrf->shift) & 0x1);

		return true;
	}

	return false;
}

static bool find_nrf(const struct snd_soc_dapm_widget *w,
		enum ABOX_CONFIGMSG *rate, enum ABOX_CONFIGMSG *format,
		int *stream, bool *slave)
{
	struct snd_soc_component *cmpnt = w->dapm->component;
	struct device *dev = cmpnt ? cmpnt->dev : NULL;
	const char *name_prefix = cmpnt ? cmpnt->name_prefix : NULL;
	size_t prefix_len = name_prefix ? strlen(name_prefix) + 1 : 0;
	const char *name = w->name + prefix_len;
	const struct name_rate_format *nrf;
	unsigned int val = 0xffffffff;
	int ret;

	for (nrf = nrf_list; nrf - nrf_list < ARRAY_SIZE(nrf_list); nrf++) {
		if (strcmp(nrf->name, name))
			continue;

		*rate = nrf->rate;
		*format = nrf->format;
		*stream = nrf->stream;
		if (nrf->sfr) {
			ret = snd_soc_component_read(cmpnt, nrf->sfr, &val);
			if (ret < 0)
				dev_err(dev, "%s: read failed: %d\n",
					__func__, ret);
		}
		*slave = !((val >> nrf->shift) & 0x1);

		return true;
	}

	return false;
}

static struct snd_soc_dapm_widget *sync_params(struct abox_data *data,
		struct list_head *widget_list, int *stream,
		const struct snd_soc_dapm_widget *w_src,
		enum ABOX_CONFIGMSG rate, enum ABOX_CONFIGMSG format)
{
	struct device *dev = &data->pdev->dev;
	enum ABOX_CONFIGMSG msg_rate, msg_format;
	struct snd_soc_dapm_widget *w = NULL, *w_ret = NULL;
	bool slave;

	list_for_each_entry(w, widget_list, work_list) {
		if (!find_nrf(w, &msg_rate, &msg_format, stream, &slave))
			continue;
		w_ret = w;
		if (slave)
			continue;

		dev_dbg(dev, "%s: %s => %s\n", __func__, w->name, w_src->name);
		set_sif_format(data, format, get_sif_format(data, msg_format));
		set_sif_rate(data, rate, get_sif_rate(data, msg_rate));
		set_sif_channels(data, format,
				get_sif_channels(data, msg_format));
		break;
	}

	return w_ret;
}

static struct snd_soc_dapm_widget *find_nearst_dai_widget(
		struct list_head *widget_list, struct snd_soc_dapm_widget *w)
{
	list_for_each_entry_continue(w, widget_list, work_list) {
		switch (w->id) {
		case snd_soc_dapm_dai_in:
		case snd_soc_dapm_dai_out:
			return w;
		default:
			continue;
		}
	}
	return NULL;
}

static int find_config(struct snd_soc_dapm_widget *w_src,
		struct abox_data *data, int stream,
		unsigned int *rate, unsigned int *width, unsigned int *channels)
{
	struct snd_soc_component *cmpnt = data->cmpnt;
	struct device *dev = &data->pdev->dev;
	struct snd_soc_dapm_widget *w, *w_tgt = NULL, *w_mst = NULL;
	enum ABOX_CONFIGMSG msg_rate, msg_format;
	struct snd_pcm_hw_params params = { 0, };
	struct snd_soc_dai *dai;
	LIST_HEAD(w_list);
	int stream_mst;
	bool slave;

	/*
	 * For snd_soc_dapm_connected_{output,input}_ep fully discover the graph
	 * we need to reset the cached number of inputs and outputs.
	 */
	list_for_each_entry(w, &cmpnt->card->widgets, list) {
		w->endpoints[SND_SOC_DAPM_DIR_IN] = -1;
		w->endpoints[SND_SOC_DAPM_DIR_OUT] = -1;
	}

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dapm_connected_output_ep(w_src, &w_list);
	else
		snd_soc_dapm_connected_input_ep(w_src, &w_list);

	list_for_each_entry(w, &w_list, work_list) {
		dev_dbg(dev, "%s\n", w->name);

		if (!find_nrf_stream(w, stream, &msg_rate, &msg_format, &slave))
			continue;

		if (slave)
			w_mst = sync_params(data, &w_list, &stream_mst,
					w, msg_rate, msg_format);

		w_tgt = w;
		break;
	}

	if (!w_tgt)
		goto out;

	if (!w_mst) {
		w_mst = w_tgt;
		stream_mst = stream;
	}

	/* override formats to UAIF format, if it is connected */
	list_for_each_entry(w, &cmpnt->card->widgets, list) {
		w->endpoints[SND_SOC_DAPM_DIR_IN] = -1;
		w->endpoints[SND_SOC_DAPM_DIR_OUT] = -1;
	}
	INIT_LIST_HEAD(&w_list);
	if (stream_mst == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dapm_connected_output_ep(w_mst, &w_list);
	else
		snd_soc_dapm_connected_input_ep(w_mst, &w_list);

	list_for_each_entry(w, &w_list, work_list) {
		if (!w->sname)
			continue;

		dai = w->priv;
		if (abox_if_hw_params_fixup(dai, &params, stream) >= 0) {
			set_sif_format(data, msg_format,
					params_format(&params));
			set_sif_rate(data, msg_rate, params_rate(&params));
			set_sif_channels(data, msg_format,
					params_channels(&params));
			break;
		}
	}

	*width = get_sif_width(data, msg_format);
	*rate = get_sif_rate(data, msg_rate);
	*channels = get_sif_channels(data, msg_format);
out:
	dev_info(dev, "%s(%s): %s: %d bit, %u channel, %uHz\n",
			__func__, w_src->name, w_tgt ? w_tgt->name : "(null)",
			*width, *channels, *rate);
	return 0;
}

static int asrc_get_idx(struct snd_soc_dapm_widget *w)
{
	return w->shift;
}

static const struct snd_kcontrol_new *asrc_get_kcontrol(int idx, int stream)
{
	if (idx < 0)
		return ERR_PTR(-EINVAL);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (idx < ARRAY_SIZE(spus_asrc_controls))
			return &spus_asrc_controls[idx];
		else
			return ERR_PTR(-EINVAL);
	} else {
		if (idx < ARRAY_SIZE(spum_asrc_controls))
			return &spum_asrc_controls[idx];
		else
			return ERR_PTR(-EINVAL);
	}
}

static const struct snd_kcontrol_new *asrc_get_id_kcontrol(int idx, int stream)
{
	if (idx < 0)
		return ERR_PTR(-EINVAL);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (idx < ARRAY_SIZE(spus_asrc_id_controls))
			return &spus_asrc_id_controls[idx];
		else
			return ERR_PTR(-EINVAL);
	} else {
		if (idx < ARRAY_SIZE(spum_asrc_id_controls))
			return &spum_asrc_id_controls[idx];
		else
			return ERR_PTR(-EINVAL);
	}
}

static int asrc_get_id(struct snd_soc_component *cmpnt, int idx, int stream)
{
	struct device *dev = cmpnt->dev;
	const struct snd_kcontrol_new *kcontrol;
	struct soc_mixer_control *mc;
	unsigned int reg, mask, val;
	int ret;

	dev_dbg(dev, "%s(%d, %d)\n", __func__, idx, stream);

	kcontrol = asrc_get_id_kcontrol(idx, stream);
	if (IS_ERR(kcontrol))
		return PTR_ERR(kcontrol);

	mc = (struct soc_mixer_control *)kcontrol->private_value;
	reg = mc->reg;
	mask = ((1 << fls(mc->max)) - 1) << mc->shift;
	ret = snd_soc_component_read(cmpnt, reg, &val);
	if (ret < 0)
		return ret;

	return (val & mask) >> mc->shift;
}

static bool asrc_get_active(struct snd_soc_component *cmpnt, int idx,
		int stream)
{
	struct device *dev = cmpnt->dev;
	const struct snd_kcontrol_new *kcontrol;
	struct soc_mixer_control *mc;
	unsigned int reg, mask, val;
	int ret;

	dev_dbg(dev, "%s(%d, %d)\n", __func__, idx, stream);

	kcontrol = asrc_get_kcontrol(idx, stream);
	if (IS_ERR(kcontrol))
		return false;

	mc = (struct soc_mixer_control *)kcontrol->private_value;
	reg = mc->reg;
	mask = 1 << mc->shift;
	ret = snd_soc_component_read(cmpnt, reg, &val);
	if (ret < 0)
		return false;

	return !!(val & mask);
}

static int asrc_get_idx_of_id(struct snd_soc_component *cmpnt, int id,
		int stream)
{
	struct device *dev = cmpnt->dev;
	int idx;
	size_t len;

	dev_dbg(dev, "%s(%d, %d)\n", __func__, id, stream);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		len = ARRAY_SIZE(spus_asrc_controls);
	else
		len = ARRAY_SIZE(spum_asrc_controls);

	for (idx = 0; idx < len; idx++) {
		if (id == asrc_get_id(cmpnt, idx, stream))
			return idx;
	}

	return -EINVAL;
}

static bool asrc_get_id_active(struct snd_soc_component *cmpnt, int id,
		int stream)
{
	struct device *dev = cmpnt->dev;
	int idx;

	dev_dbg(dev, "%s(%d, %d)\n", __func__, id, stream);

	idx = asrc_get_idx_of_id(cmpnt, id, stream);
	if (idx < 0)
		return false;

	return asrc_get_active(cmpnt, idx, stream);
}

static int asrc_put_id(struct snd_soc_component *cmpnt, int idx, int stream,
		int id)
{
	struct device *dev = cmpnt->dev;
	const struct snd_kcontrol_new *kcontrol;
	struct soc_mixer_control *mc;
	unsigned int reg, mask, val;

	dev_dbg(dev, "%s(%d, %d, %d)\n", __func__, idx, stream, id);

	kcontrol = asrc_get_id_kcontrol(idx, stream);
	if (IS_ERR(kcontrol))
		return PTR_ERR(kcontrol);

	mc = (struct soc_mixer_control *)kcontrol->private_value;
	reg = mc->reg;
	mask = ((1 << fls(mc->max)) - 1) << mc->shift;
	val = id << mc->shift;
	return snd_soc_component_update_bits(cmpnt, reg, mask, val);
}

static int asrc_put_active(struct snd_soc_dapm_widget *w, int stream, int on)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_component *cmpnt = dapm->component;
	struct device *dev = cmpnt->dev;
	const struct snd_kcontrol_new *kcontrol;
	struct soc_mixer_control *mc;
	unsigned int reg, mask, val;

	dev_dbg(dev, "%s(%s, %d, %d)\n", __func__, w->name, stream, on);

	kcontrol = asrc_get_kcontrol(asrc_get_idx(w), stream);
	if (IS_ERR(kcontrol))
		return PTR_ERR(kcontrol);

	mc = (struct soc_mixer_control *)kcontrol->private_value;
	reg = mc->reg;
	mask = 1 << mc->shift;
	val = !!on << mc->shift;
	return snd_soc_component_update_bits(cmpnt, reg, mask, val);
}

static int asrc_exchange_id(struct snd_soc_component *cmpnt, int stream,
		int idx1, int idx2)
{
	int id1 = asrc_get_id(cmpnt, idx1, stream);
	int id2 = asrc_get_id(cmpnt, idx2, stream);
	int ret;

	if (idx1 == idx2)
		return 0;

	ret = asrc_put_id(cmpnt, idx1, stream, id2);
	if (ret < 0)
		return ret;
	ret = asrc_put_id(cmpnt, idx2, stream, id1);
	if (ret < 0)
		asrc_put_id(cmpnt, idx1, stream, id1);

	return ret;
}

static const int spus_asrc_max_channels[] = {
	8, 4, 4, 2, 8, 4, 4, 2, 8, 4, 4, 2,
};

static const int spum_asrc_max_channels[] = {
	8, 4, 4, 2, 8, 4, 4, 2,
};

static const int spus_asrc_sorted_id[] = {
	3, 7, 11, 2, 6, 10, 1, 5, 9, 0, 4, 8,
};

static const int spum_asrc_sorted_id[] = {
	3, 7, 2, 6, 1, 5, 0, 4,
};

static int spus_asrc_channels[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static int spum_asrc_channels[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
};

static int spus_asrc_lock[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

static int spum_asrc_lock[] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
};

int abox_cmpnt_asrc_lock(struct snd_soc_component *cmpnt, int stream,
		int idx, int id)
{
	int ret = 0;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		spus_asrc_lock[idx] = id;
	else
		spum_asrc_lock[idx] = id;

	if (id != asrc_get_id(cmpnt, idx, stream)) {
		ret = asrc_exchange_id(cmpnt, stream, idx,
				asrc_get_idx_of_id(cmpnt, id, stream));
		if (ret < 0)
			return ret;
		ret = asrc_put_active(asrc_get_widget(cmpnt, idx, stream),
				stream, 1);
		if (ret < 0)
			return ret;
	}

	return ret;
}

static bool asrc_is_lock(int stream, int id)
{
	int i;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		for (i = 0; i < ARRAY_SIZE(spus_asrc_lock); i++) {
			if (spus_asrc_lock[i] == id)
				return true;
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(spum_asrc_lock); i++) {
			if (spum_asrc_lock[i] == id)
				return true;
		}
	}

	return false;
}

static int asrc_get_lock_id(int stream, int idx)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return spus_asrc_lock[idx];
	else
		return spum_asrc_lock[idx];
}

static int asrc_get_num(int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return ARRAY_SIZE(spus_asrc_channels);
	else
		return ARRAY_SIZE(spum_asrc_channels);
}

static int asrc_get_max_channels(int id, int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return spus_asrc_max_channels[id];
	else
		return spum_asrc_max_channels[id];
}

static int asrc_get_sorted_id(int i, int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return spus_asrc_sorted_id[i];
	else
		return spum_asrc_sorted_id[i];
}

static int asrc_get_channels(int id, int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return spus_asrc_channels[id];
	else
		return spum_asrc_channels[id];
}

static void asrc_set_channels(int id, int stream, int channels)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		spus_asrc_channels[id] = channels;
	else
		spum_asrc_channels[id] = channels;
}

static int asrc_is_avail_id(struct snd_soc_dapm_widget *w, int id,
		int stream, int channels)
{
	struct snd_soc_component *cmpnt = w->dapm->component;
	struct device *dev = cmpnt->dev;
	int idx = asrc_get_idx_of_id(cmpnt, id, stream);
	struct snd_soc_dapm_widget *w_t = asrc_get_widget(cmpnt, idx, stream);
	int ret;

	if (asrc_get_max_channels(id, stream) < channels) {
		ret = false;
		goto out;
	}

	if (w_t != w && asrc_is_lock(stream, id)) {
		ret = false;
		goto out;
	}

	if (w_t != w && asrc_get_id_active(cmpnt, id, stream)) {
		ret = false;
		goto out;
	}

	if (id % 2) {
		if (asrc_get_id_active(cmpnt, id - 1, stream))
			ret = asrc_get_channels(id - 1, stream) <
					asrc_get_max_channels(id - 1, stream);
		else
			ret = true;
	} else {
		if (channels < asrc_get_max_channels(id, stream))
			ret = true;
		else
			ret = !asrc_get_id_active(cmpnt, id + 1, stream);
	}
out:
	dev_dbg(dev, "%s(%d, %d, %d): %d\n", __func__,
			id, stream, channels, ret);
	return ret;
}

static int asrc_assign_id(struct snd_soc_dapm_widget *w, int stream,
		unsigned int channels)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_component *cmpnt = dapm->component;
	int i, id, ret = -EINVAL;

	id = asrc_get_lock_id(stream, asrc_get_idx(w));
	if (id >= 0) {
		ret = asrc_exchange_id(cmpnt, stream, asrc_get_idx(w),
				asrc_get_idx_of_id(cmpnt, id, stream));
		if (ret >= 0)
			asrc_set_channels(id, stream, channels);
		return ret;
	}

	for (i = 0; i < asrc_get_num(stream); i++) {
		id = asrc_get_sorted_id(i, stream);

		if (asrc_is_avail_id(w, id, stream, channels)) {
			ret = asrc_exchange_id(cmpnt, stream, asrc_get_idx(w),
					asrc_get_idx_of_id(cmpnt, id, stream));
			if (ret >= 0)
				asrc_set_channels(id, stream, channels);
			break;
		}
	}

	return ret;
}

static void asrc_release_id(struct snd_soc_dapm_widget *w, int stream)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_component *cmpnt = dapm->component;
	int idx = asrc_get_idx(w);
	int id = asrc_get_id(cmpnt, idx, stream);

	if (id < 0)
		return;

	if (asrc_get_lock_id(stream, idx) >= 0)
		return;

	asrc_set_channels(id, stream, 0);
	asrc_put_active(w, stream, 0);
}

void abox_cmpnt_asrc_release(struct snd_soc_component *cmpnt, int stream,
		int idx)
{
	dev_info(cmpnt->dev, "release asrc(%d, %d)\n", stream, idx);

	asrc_release_id(asrc_get_widget(cmpnt, idx, stream), stream);
}

static int asrc_find_id(struct snd_soc_dapm_widget *w, int stream)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_component *cmpnt = dapm->component;

	return asrc_get_id(cmpnt, asrc_get_idx(w), stream);
}

static int update_bits_async(struct device *dev,
		struct snd_soc_component *cmpnt, const char *name,
		unsigned int reg, unsigned int mask, unsigned int val)
{
	int ret;

	dev_dbg(dev, "%s(%s, %#x, %#x, %#x)\n", __func__, name, reg,
			mask, val);

	ret = snd_soc_component_update_bits_async(cmpnt, reg, mask, val);
	if (ret < 0)
		dev_err(dev, "%s(%s, %#x, %#x, %#x): %d\n", __func__, name, reg,
				mask, val, ret);

	return ret;
}

static int asrc_update_tick(struct abox_data *data, int stream, int id)
{
	static const int tick_table[][3] = {
		/* aclk, ticknum, tickdiv */
		{600000000, 1, 1},
		{400000000, 3, 2},
		{300000000, 2, 1},
		{200000000, 3, 1},
		{150000000, 4, 1},
		{100000000, 6, 1},
	};
	struct snd_soc_component *cmpnt = data->cmpnt;
	struct device *dev = cmpnt->dev;
	unsigned int reg, mask, val;
	enum asrc_tick itick, otick;
	int idx = asrc_get_idx_of_id(cmpnt, id, stream);
	unsigned long aclk = clk_get_rate(data->clk_bus);
	int ticknum, tickdiv;
	int i, res, ret = 0;

	dev_dbg(dev, "%s(%d, %d, %ulHz)\n", __func__, stream, id, aclk);

	if (idx < 0) {
		dev_err(dev, "%s(%d, %d): invalid idx: %d\n", __func__,
				stream, id, idx);
		return -EINVAL;
	}

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reg = ABOX_SPUS_ASRC_CTRL(id);
		itick = spus_asrc_is[idx];
		otick = spus_asrc_os[idx];
	} else {
		reg = ABOX_SPUM_ASRC_CTRL(id);
		itick = spum_asrc_is[idx];
		otick = spum_asrc_os[idx];
	}

	if ((itick == TICK_SYNC) && (otick == TICK_SYNC))
		return 0;

	res = snd_soc_component_read(cmpnt, reg, &val);
	if (res < 0) {
		dev_err(dev, "%s: read fail(%#x): %d\n", __func__, reg, res);
		ret = res;
	}

	ticknum = tickdiv = 1;
	for (i = 0; i < ARRAY_SIZE(tick_table); i++) {
		if (aclk > tick_table[i][0])
			break;

		ticknum = tick_table[i][1];
		tickdiv = tick_table[i][2];
	}

	mask = ABOX_ASRC_TICKNUM_MASK;
	val = ticknum << ABOX_ASRC_TICKNUM_L;
	res = update_bits_async(dev, cmpnt, "ticknum", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_TICKDIV_MASK;
	val = tickdiv << ABOX_ASRC_TICKDIV_L;
	res = update_bits_async(dev, cmpnt, "tickdiv", reg, mask, val);
	if (res < 0)
		ret = res;

	/* Todo: change it to dev_dbg() */
	dev_info(dev, "asrc tick(%d, %d) aclk=%luHz: %d, %d\n",
			stream, id, aclk, ticknum, tickdiv);

	return ret;
}

int abox_cmpnt_update_asrc_tick(struct device *adev)
{
	struct device *dev = adev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct snd_soc_component *cmpnt = data->cmpnt;
	int stream, id, res, ret = 0;

	if (!cmpnt)
		return 0;

	dev_dbg(dev, "%s\n", __func__);

	for (stream = 0; stream <= SNDRV_PCM_STREAM_LAST; stream++) {
		for (id = 0; id < asrc_get_num(stream); id++) {
			res = asrc_update_tick(data, stream, id);
			if (res < 0)
				ret = res;
		}
	}

	snd_soc_component_async_complete(cmpnt);

	return ret;
}

static int asrc_config_async(struct abox_data *data, int id, int stream,
		enum asrc_tick itick, unsigned int isr,
		enum asrc_tick otick, unsigned int osr,
		unsigned int s_default, unsigned int owidth, int apf_coef)
{
	struct device *dev = &data->pdev->dev;
	struct snd_soc_component *cmpnt = data->cmpnt;
	bool spus = (stream == SNDRV_PCM_STREAM_PLAYBACK);
	unsigned int ofactor;
	unsigned int reg, mask, val;
	struct asrc_ctrl ctrl;
	int res, ret = 0;

	dev_dbg(dev, "%s(%d, %d, %d, %uHz, %d, %uHz, %u, %ubit, %d)\n",
			__func__, id, stream, itick, isr,
			otick, osr, s_default, owidth, apf_coef);

	if ((itick == TICK_SYNC) == (otick == TICK_SYNC)) {
		dev_err(dev, "%s: itick=%d otick=%d\n", __func__, itick, otick);
		return -EINVAL;
	}

	if (s_default == 0) {
		dev_err(dev, "invalid default\n");
		return -EINVAL;
	}

	/* set async side to input side */
	ctrl.isr = (itick != TICK_SYNC) ? isr : osr;
	ctrl.osr = (itick != TICK_SYNC) ? osr : isr;
	ctrl.ovsf = RATIO_8;
	ctrl.ifactor = s_default;
	ctrl.dcmf = RATIO_8;
	ofactor = cal_ofactor(&ctrl);
	while (ofactor > ABOX_ASRC_OS_DEFAULT_MASK) {
		ctrl.ovsf--;
		ofactor = cal_ofactor(&ctrl);
	}

	if (itick == TICK_SYNC) {
		swap(ctrl.isr, ctrl.osr);
		swap(ctrl.ovsf, ctrl.dcmf);
		swap(ctrl.ifactor, ofactor);
	}

	/* Todo: change it to dev_dbg() */
	dev_dbg(dev, "asrc(%d, %d): %d, %uHz, %d, %uHz, %d, %d, %u, %u, %u, %u\n",
			stream, id, itick, ctrl.isr, otick, ctrl.osr,
			1 << ctrl.ovsf, 1 << ctrl.dcmf, ctrl.ifactor, ofactor,
			is_limit(ctrl.ifactor), os_limit(ofactor));

	reg = spus ? ABOX_SPUS_ASRC_CTRL(id) : ABOX_SPUM_ASRC_CTRL(id);

	mask = ABOX_ASRC_OUT_BIT_WIDTH_MASK;
	val = ((owidth / 8) - 1) << ABOX_ASRC_OUT_BIT_WIDTH_L;
	res = update_bits_async(dev, cmpnt, "out width", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_IS_SYNC_MODE_MASK;
	val = (itick != TICK_SYNC) << ABOX_ASRC_IS_SYNC_MODE_L;
	res = update_bits_async(dev, cmpnt, "is sync", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_OS_SYNC_MODE_MASK;
	val = (otick != TICK_SYNC) << ABOX_ASRC_OS_SYNC_MODE_L;
	res = update_bits_async(dev, cmpnt, "os sync", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_OVSF_RATIO_MASK;
	val = ctrl.ovsf << ABOX_ASRC_OVSF_RATIO_L;
	res = update_bits_async(dev, cmpnt, "ovsf ratio", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_DCMF_RATIO_MASK;
	val = ctrl.dcmf << ABOX_ASRC_DCMF_RATIO_L;
	res = update_bits_async(dev, cmpnt, "dcmf ratio", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_IS_SOURCE_SEL_MASK;
	val = itick << ABOX_ASRC_IS_SOURCE_SEL_L;
	res = update_bits_async(dev, cmpnt, "is source", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_OS_SOURCE_SEL_MASK;
	val = otick << ABOX_ASRC_OS_SOURCE_SEL_L;
	res = update_bits_async(dev, cmpnt, "os source", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_IS_PARA0(id) : ABOX_SPUM_ASRC_IS_PARA0(id);
	mask = ABOX_ASRC_IS_DEFAULT_MASK;
	val = ctrl.ifactor;
	res = update_bits_async(dev, cmpnt, "is default", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_IS_PARA1(id) : ABOX_SPUM_ASRC_IS_PARA1(id);
	mask = ABOX_ASRC_IS_TPERIOD_LIMIT_MASK;
	val = is_limit(val);
	res = update_bits_async(dev, cmpnt, "is tperiod limit", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_OS_PARA0(id) : ABOX_SPUM_ASRC_OS_PARA0(id);
	mask = ABOX_ASRC_OS_DEFAULT_MASK;
	val = ofactor;
	res = update_bits_async(dev, cmpnt, "os default", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_OS_PARA1(id) : ABOX_SPUM_ASRC_OS_PARA1(id);
	mask = ABOX_ASRC_OS_TPERIOD_LIMIT_MASK;
	val = os_limit(val);
	res = update_bits_async(dev, cmpnt, "os tperiod limit", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_FILTER_CTRL(id) :
			ABOX_SPUM_ASRC_FILTER_CTRL(id);
	mask = ABOX_ASRC_APF_COEF_SEL_MASK;
	val = apf_coef << ABOX_ASRC_APF_COEF_SEL_L;
	res = update_bits_async(dev, cmpnt, "apf coef sel", reg, mask, val);
	if (res < 0)
		ret = res;

	res = asrc_update_tick(data, stream, id);
	if (res < 0)
		ret = res;

	snd_soc_component_async_complete(cmpnt);

	return ret;
}

static int asrc_config_sync(struct abox_data *data, int id, int stream,
		unsigned int isr, unsigned int osr, unsigned int owidth,
		int apf_coef)
{
	struct device *dev = &data->pdev->dev;
	struct snd_soc_component *cmpnt = data->cmpnt;
	bool spus = (stream == SNDRV_PCM_STREAM_PLAYBACK);
	const struct asrc_ctrl *ctrl;
	unsigned int reg, mask, val;
	int res, ret = 0;

	dev_dbg(dev, "%s(%d, %d, %uHz, %uHz, %ubit, %d)\n", __func__,
			id, stream, isr, osr, owidth, apf_coef);

	ctrl = &asrc_table[to_asrc_rate(isr)][to_asrc_rate(osr)];

	reg = spus ? ABOX_SPUS_ASRC_CTRL(id) : ABOX_SPUM_ASRC_CTRL(id);
	mask = ABOX_ASRC_OUT_BIT_WIDTH_MASK;
	val = ((owidth / 8) - 1) << ABOX_ASRC_OUT_BIT_WIDTH_L;
	res = update_bits_async(dev, cmpnt, "out width", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_IS_SYNC_MODE_MASK;
	val = 0 << ABOX_ASRC_IS_SYNC_MODE_L;
	res = update_bits_async(dev, cmpnt, "is sync", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_OS_SYNC_MODE_MASK;
	val = 0 << ABOX_ASRC_OS_SYNC_MODE_L;
	res = update_bits_async(dev, cmpnt, "os sync", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_OVSF_RATIO_MASK;
	val = ctrl->ovsf << ABOX_ASRC_OVSF_RATIO_L;
	res = update_bits_async(dev, cmpnt, "ovsf ratio", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_DCMF_RATIO_MASK;
	val = ctrl->dcmf << ABOX_ASRC_DCMF_RATIO_L;
	res = update_bits_async(dev, cmpnt, "dcmf ratio", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_IS_SOURCE_SEL_MASK;
	val = TICK_SYNC << ABOX_ASRC_IS_SOURCE_SEL_L;
	res = update_bits_async(dev, cmpnt, "is source", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_OS_SOURCE_SEL_MASK;
	val = TICK_SYNC << ABOX_ASRC_OS_SOURCE_SEL_L;
	res = update_bits_async(dev, cmpnt, "os source", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_IS_PARA0(id) : ABOX_SPUM_ASRC_IS_PARA0(id);
	mask = ABOX_ASRC_IS_DEFAULT_MASK;
	val = ctrl->ifactor;
	res = update_bits_async(dev, cmpnt, "is default", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_IS_PARA1(id) : ABOX_SPUM_ASRC_IS_PARA1(id);
	mask = ABOX_ASRC_IS_TPERIOD_LIMIT_MASK;
	val = is_limit(val);
	res = update_bits_async(dev, cmpnt, "is tperiod limit", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_OS_PARA0(id) : ABOX_SPUM_ASRC_OS_PARA0(id);
	mask = ABOX_ASRC_OS_DEFAULT_MASK;
	val = cal_ofactor(ctrl);
	res = update_bits_async(dev, cmpnt, "os default", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_OS_PARA1(id) : ABOX_SPUM_ASRC_OS_PARA1(id);
	mask = ABOX_ASRC_OS_TPERIOD_LIMIT_MASK;
	val = os_limit(val);
	res = update_bits_async(dev, cmpnt, "os tperiod limit", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_FILTER_CTRL(id) :
			ABOX_SPUM_ASRC_FILTER_CTRL(id);
	mask = ABOX_ASRC_APF_COEF_SEL_MASK;
	val = apf_coef << ABOX_ASRC_APF_COEF_SEL_L;
	res = update_bits_async(dev, cmpnt, "apf coef sel", reg, mask, val);
	if (res < 0)
		ret = res;

	snd_soc_component_async_complete(cmpnt);

	return ret;
}

static int asrc_config(struct abox_data *data, int id, int stream,
		enum asrc_tick itick, unsigned int isr,
		enum asrc_tick otick, unsigned int osr, unsigned int owidth,
		int apf_coef)
{
	struct device *dev = &data->pdev->dev;
	int ret;

	dev_info(dev, "%s(%d, %d, %d, %uHz, %d, %uHz, %ubit, %d)\n",
			__func__, id, stream, itick, isr, otick, osr,
			owidth, apf_coef);

	if ((itick == TICK_SYNC) && (otick == TICK_SYNC)) {
		ret = asrc_config_sync(data, id, stream, isr, osr, owidth,
				apf_coef);
	} else {
		if (itick == TICK_CP) {
			ret = asrc_config_async(data, id, stream,
					itick, isr, otick, osr, s_default,
					owidth, apf_coef);
		} else if (otick == TICK_CP) {
			ret = asrc_config_async(data, id, stream,
					itick, isr, otick, osr, s_default,
					owidth, apf_coef);
		} else {
			dev_err(dev, "not supported\n");
			ret = -EINVAL;
		}
	}

	return ret;
}

static int asrc_set(struct abox_data *data, int stream, int idx,
		unsigned int channels, unsigned int rate, unsigned int tgt_rate,
		unsigned int width, unsigned int tgt_width)
{
	struct device *dev = &data->pdev->dev;
	struct snd_soc_component *cmpnt = data->cmpnt;
	struct snd_soc_dapm_widget *w = asrc_get_widget(cmpnt, idx, stream);
	enum asrc_tick itick, otick;
	int apf_coef = get_apf_coef(data, stream, idx);
	bool force_enable;
	int on, ret;

	dev_dbg(dev, "%s(%d, %d, %uHz, %uHz, %ubit, %ubit)\n", __func__,
			idx, channels, rate, tgt_rate, width, tgt_width);

	ret = asrc_assign_id(w, stream, channels);
	if (ret < 0)
		dev_err(dev, "%s: assign failed: %d\n", __func__, ret);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		force_enable = spus_asrc_force_enable[idx];
		itick = spus_asrc_is[idx];
		otick = spus_asrc_os[idx];
		ret = asrc_config(data, asrc_find_id(w, stream), stream,
				itick, rate, otick, tgt_rate,
				tgt_width, apf_coef);
	} else {
		force_enable = spum_asrc_force_enable[idx];
		itick = spum_asrc_is[idx];
		otick = spum_asrc_os[idx];
		ret = asrc_config(data, asrc_find_id(w, stream), stream,
				itick, tgt_rate, otick, rate,
				tgt_width, apf_coef);
	}
	if (ret < 0)
		dev_err(dev, "%s: config failed: %d\n", __func__, ret);

	on = force_enable || (rate != tgt_rate) || (width != tgt_width) ||
			(itick != TICK_SYNC) || (otick != TICK_SYNC);
	dev_info(dev, "turn %s %s\n", w->name, on ? "on" : "off");
	ret = asrc_put_active(w, stream, on);

	return ret;
}

static int asrc_set_by_dai(struct abox_data *data, enum abox_dai id,
		unsigned int channels, unsigned int rate, unsigned int tgt_rate,
		unsigned int width, unsigned int tgt_width)
{
	int idx, stream;

	switch (id) {
	case ABOX_RDMA0:
	case ABOX_RDMA1:
	case ABOX_RDMA2:
	case ABOX_RDMA3:
	case ABOX_RDMA4:
	case ABOX_RDMA5:
	case ABOX_RDMA6:
	case ABOX_RDMA7:
	case ABOX_RDMA8:
	case ABOX_RDMA9:
	case ABOX_RDMA10:
	case ABOX_RDMA11:
		idx = id - ABOX_RDMA0;
		stream = SNDRV_PCM_STREAM_PLAYBACK;
		break;
	case ABOX_WDMA0:
	case ABOX_WDMA1:
	case ABOX_WDMA2:
	case ABOX_WDMA3:
	case ABOX_WDMA4:
	case ABOX_WDMA5:
	case ABOX_WDMA6:
	case ABOX_WDMA7:
		idx = id - ABOX_WDMA0;
		stream = SNDRV_PCM_STREAM_CAPTURE;
		break;
	default:
		return -EINVAL;
	}

	return asrc_set(data, stream, idx, channels, rate, tgt_rate,
			width, tgt_width);
}

static int asrc_event(struct snd_soc_dapm_widget *w, int e,
		int stream, struct platform_device **pdev_dma)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct device *dev = dapm->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	int idx = asrc_get_idx(w);
	struct snd_soc_pcm_runtime *rtd;
	struct snd_pcm_hw_params *params = NULL;
	unsigned int tgt_rate = 0, tgt_width = 0, tgt_channels = 0;
	unsigned int rate = 0, width = 0, channels = 0;
	int ret = 0;

	dev_dbg(dev, "%s(%s, %d)\n", __func__, w->name, e);

	switch (e) {
	case SND_SOC_DAPM_PRE_PMU:
		list_for_each_entry(rtd, &dapm->card->rtd_list, list)  {
			if (rtd->cpu_dai->dev == &pdev_dma[idx]->dev) {
				params = &rtd->dpcm[stream].hw_params;
				rate = params_rate(params);
				width = params_width(params);
				channels = params_channels(params);
				break;
			}
		}
		if (!params) {
			ret = -EINVAL;
			break;
		}

		ret = find_config(w, data, stream, &tgt_rate, &tgt_width,
				&tgt_channels);
		if (ret < 0) {
			dev_warn(dev, "%s: incomplete path: %s\n",
				__func__, w->name);
			break;
		}

		ret = asrc_set(data, stream, idx, channels, rate, tgt_rate,
				width, tgt_width);
		if (ret < 0)
			dev_err(dev, "%s: set failed: %d\n", __func__, ret);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* ASRC will be released in DMA stop. */
		break;
	}

	return ret;
}

static int spus_asrc_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *k, int e)
{
	const int stream = SNDRV_PCM_STREAM_PLAYBACK;
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct device *dev = dapm->dev;
	struct abox_data *data = dev_get_drvdata(dev);

	return asrc_event(w, e, stream, data->pdev_rdma);
}

static int spum_asrc_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *k, int e)
{
	const int stream = SNDRV_PCM_STREAM_CAPTURE;
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct device *dev = dapm->dev;
	struct abox_data *data = dev_get_drvdata(dev);

	return asrc_event(w, e, stream, data->pdev_wdma);
}

static const char * const spus_inx_texts[] = {"RDMA", "SIFSM"};
static SOC_ENUM_SINGLE_DECL(spus_in0_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_IN_L(0), spus_inx_texts);
static const struct snd_kcontrol_new spus_in0_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in0_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in1_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_IN_L(1), spus_inx_texts);
static const struct snd_kcontrol_new spus_in1_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in1_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in2_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_IN_L(2), spus_inx_texts);
static const struct snd_kcontrol_new spus_in2_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in2_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in3_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_IN_L(3), spus_inx_texts);
static const struct snd_kcontrol_new spus_in3_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in3_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in4_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_IN_L(4), spus_inx_texts);
static const struct snd_kcontrol_new spus_in4_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in4_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in5_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_IN_L(5), spus_inx_texts);
static const struct snd_kcontrol_new spus_in5_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in5_enum),
};
#if (ABOX_SOC_VERSION(2, 0, 1) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
static SOC_ENUM_SINGLE_DECL(spus_in6_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_IN_L(6), spus_inx_texts);
static SOC_ENUM_SINGLE_DECL(spus_in7_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_IN_L(7), spus_inx_texts);
static SOC_ENUM_SINGLE_DECL(spus_in8_enum, ABOX_SPUS_CTRL4,
		ABOX_FUNC_CHAIN_SRC_IN_L(8), spus_inx_texts);
static SOC_ENUM_SINGLE_DECL(spus_in9_enum, ABOX_SPUS_CTRL4,
		ABOX_FUNC_CHAIN_SRC_IN_L(9), spus_inx_texts);
static SOC_ENUM_SINGLE_DECL(spus_in10_enum, ABOX_SPUS_CTRL4,
		ABOX_FUNC_CHAIN_SRC_IN_L(10), spus_inx_texts);
static SOC_ENUM_SINGLE_DECL(spus_in11_enum, ABOX_SPUS_CTRL4,
		ABOX_FUNC_CHAIN_SRC_IN_L(11), spus_inx_texts);
#else
static SOC_ENUM_SINGLE_DECL(spus_in6_enum, ABOX_SPUS_CTRL_FC1,
		ABOX_FUNC_CHAIN_SRC_IN_L(0), spus_inx_texts);
static SOC_ENUM_SINGLE_DECL(spus_in7_enum, ABOX_SPUS_CTRL_FC1,
		ABOX_FUNC_CHAIN_SRC_IN_L(1), spus_inx_texts);
static SOC_ENUM_SINGLE_DECL(spus_in8_enum, ABOX_SPUS_CTRL_FC1,
		ABOX_FUNC_CHAIN_SRC_IN_L(2), spus_inx_texts);
static SOC_ENUM_SINGLE_DECL(spus_in9_enum, ABOX_SPUS_CTRL_FC1,
		ABOX_FUNC_CHAIN_SRC_IN_L(3), spus_inx_texts);
static SOC_ENUM_SINGLE_DECL(spus_in10_enum, ABOX_SPUS_CTRL_FC1,
		ABOX_FUNC_CHAIN_SRC_IN_L(4), spus_inx_texts);
static SOC_ENUM_SINGLE_DECL(spus_in11_enum, ABOX_SPUS_CTRL_FC1,
		ABOX_FUNC_CHAIN_SRC_IN_L(5), spus_inx_texts);
#endif
static const struct snd_kcontrol_new spus_in6_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in6_enum),
};
static const struct snd_kcontrol_new spus_in7_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in7_enum),
};
static const struct snd_kcontrol_new spus_in8_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in8_enum),
};
static const struct snd_kcontrol_new spus_in9_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in9_enum),
};
static const struct snd_kcontrol_new spus_in10_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in10_enum),
};
static const struct snd_kcontrol_new spus_in11_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in11_enum),
};

#if (ABOX_SOC_VERSION(2, 0, 1) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
static const char * const spus_outx_texts[] = {
	"SIFS1", "SIFS0", "SIFS2", "SIFS3", "SIFS4",
};
#else
static const char * const spus_outx_texts[] = {
	"SIFS1", "SIFS0", "SIFS2", "RESERVED", "SIFS3", "RESERVED", "SIFS4",
};
#endif
static SOC_ENUM_SINGLE_DECL(spus_out0_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_OUT_L(0), spus_outx_texts);
static const struct snd_kcontrol_new spus_out0_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out0_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out1_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_OUT_L(1), spus_outx_texts);
static const struct snd_kcontrol_new spus_out1_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out1_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out2_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_OUT_L(2), spus_outx_texts);
static const struct snd_kcontrol_new spus_out2_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out2_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out3_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_OUT_L(3), spus_outx_texts);
static const struct snd_kcontrol_new spus_out3_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out3_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out4_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_OUT_L(4), spus_outx_texts);
static const struct snd_kcontrol_new spus_out4_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out4_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out5_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_OUT_L(5), spus_outx_texts);
static const struct snd_kcontrol_new spus_out5_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out5_enum),
};
#if (ABOX_SOC_VERSION(2, 0, 1) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
static SOC_ENUM_SINGLE_DECL(spus_out6_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_OUT_L(6), spus_outx_texts);
static SOC_ENUM_SINGLE_DECL(spus_out7_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_OUT_L(7), spus_outx_texts);
static SOC_ENUM_SINGLE_DECL(spus_out8_enum, ABOX_SPUS_CTRL4,
		ABOX_FUNC_CHAIN_SRC_OUT_L(8), spus_outx_texts);
static SOC_ENUM_SINGLE_DECL(spus_out9_enum, ABOX_SPUS_CTRL4,
		ABOX_FUNC_CHAIN_SRC_OUT_L(9), spus_outx_texts);
static SOC_ENUM_SINGLE_DECL(spus_out10_enum, ABOX_SPUS_CTRL4,
		ABOX_FUNC_CHAIN_SRC_OUT_L(10), spus_outx_texts);
static SOC_ENUM_SINGLE_DECL(spus_out11_enum, ABOX_SPUS_CTRL4,
		ABOX_FUNC_CHAIN_SRC_OUT_L(11), spus_outx_texts);
#else
static SOC_ENUM_SINGLE_DECL(spus_out6_enum, ABOX_SPUS_CTRL_FC1,
		ABOX_FUNC_CHAIN_SRC_OUT_L(0), spus_outx_texts);
static SOC_ENUM_SINGLE_DECL(spus_out7_enum, ABOX_SPUS_CTRL_FC1,
		ABOX_FUNC_CHAIN_SRC_OUT_L(1), spus_outx_texts);
static SOC_ENUM_SINGLE_DECL(spus_out8_enum, ABOX_SPUS_CTRL_FC1,
		ABOX_FUNC_CHAIN_SRC_OUT_L(2), spus_outx_texts);
static SOC_ENUM_SINGLE_DECL(spus_out9_enum, ABOX_SPUS_CTRL_FC1,
		ABOX_FUNC_CHAIN_SRC_OUT_L(3), spus_outx_texts);
static SOC_ENUM_SINGLE_DECL(spus_out10_enum, ABOX_SPUS_CTRL_FC1,
		ABOX_FUNC_CHAIN_SRC_OUT_L(4), spus_outx_texts);
static SOC_ENUM_SINGLE_DECL(spus_out11_enum, ABOX_SPUS_CTRL_FC1,
		ABOX_FUNC_CHAIN_SRC_OUT_L(5), spus_outx_texts);
#endif
static const struct snd_kcontrol_new spus_out6_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out6_enum),
};
static const struct snd_kcontrol_new spus_out7_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out7_enum),
};
static const struct snd_kcontrol_new spus_out8_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out8_enum),
};
static const struct snd_kcontrol_new spus_out9_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out9_enum),
};
static const struct snd_kcontrol_new spus_out10_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out10_enum),
};
static const struct snd_kcontrol_new spus_out11_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out11_enum),
};

static const char * const spusm_texts[] = {
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"UAIF0", "UAIF1", "UAIF2", "UAIF3",
};
static SOC_ENUM_SINGLE_DECL(spusm_enum, ABOX_ROUTE_CTRL0, ABOX_ROUTE_SPUSM_L,
		spusm_texts);
static const struct snd_kcontrol_new spusm_controls[] = {
	SOC_DAPM_ENUM("MUX", spusm_enum),
};

static const char * const sifsx_texts[] = {
	"SPUS OUT0", "SPUS OUT1", "SPUS OUT2", "SPUS OUT3",
	"SPUS OUT4", "SPUS OUT5", "SPUS OUT6", "SPUS OUT7",
	"SPUS OUT8", "SPUS OUT9", "SPUS OUT10", "SPUS OUT11",
};
static SOC_ENUM_SINGLE_DECL(sifs1_enum, ABOX_SPUS_CTRL1, ABOX_SIFS_OUT_SEL_L(1),
		sifsx_texts);
static const struct snd_kcontrol_new sifs1_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs1_enum),
};
static SOC_ENUM_SINGLE_DECL(sifs2_enum, ABOX_SPUS_CTRL1, ABOX_SIFS_OUT_SEL_L(2),
		sifsx_texts);
static const struct snd_kcontrol_new sifs2_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs2_enum),
};
static SOC_ENUM_SINGLE_DECL(sifs3_enum, ABOX_SPUS_CTRL1, ABOX_SIFS_OUT_SEL_L(3),
		sifsx_texts);
static const struct snd_kcontrol_new sifs3_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs3_enum),
};
static SOC_ENUM_SINGLE_DECL(sifs4_enum, ABOX_SPUS_CTRL1, ABOX_SIFS_OUT_SEL_L(4),
		sifsx_texts);
static const struct snd_kcontrol_new sifs4_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs4_enum),
};

static const struct snd_kcontrol_new sifs0_out_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new sifs1_out_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new sifs2_out_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new sifs3_out_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new sifs4_out_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};

static const char * const sifsm_texts[] = {
	"SPUS IN0", "SPUS IN1", "SPUS IN2", "SPUS IN3",
	"SPUS IN4", "SPUS IN5", "SPUS IN6", "SPUS IN7",
	"SPUS IN8", "SPUS IN9", "SPUS IN10", "SPUS IN11",
};
static SOC_ENUM_SINGLE_DECL(sifsm_enum, ABOX_SPUS_CTRL1, ABOX_SIFSM_IN_SEL_L,
		sifsm_texts);
static const struct snd_kcontrol_new sifsm_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifsm_enum),
};

static const char * const uaif_spkx_texts[] = {
	"RESERVED", "SIFS0", "SIFS1", "SIFS2",
	"SIFS3", "SIFS4", "RESERVED", "RESERVED",
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"SIFMS",
};
static SOC_ENUM_SINGLE_DECL(uaif0_spk_enum, ABOX_ROUTE_CTRL0,
		ABOX_ROUTE_UAIF_SPK_L(0), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif0_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif0_spk_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif1_spk_enum, ABOX_ROUTE_CTRL0,
		ABOX_ROUTE_UAIF_SPK_L(1), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif1_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif1_spk_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif2_spk_enum, ABOX_ROUTE_CTRL0,
		ABOX_ROUTE_UAIF_SPK_L(2), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif2_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif2_spk_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif3_spk_enum, ABOX_ROUTE_CTRL0,
		ABOX_ROUTE_UAIF_SPK_L(3), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif3_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif3_spk_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif4_spk_enum, ABOX_ROUTE_CTRL0,
		ABOX_ROUTE_UAIF_SPK_L(4), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif4_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif4_spk_enum),
};

static const char * const dsif_spk_texts[] = {
	"RESERVED", "RESERVED", "SIFS1", "SIFS2", "SIFS3", "SIFS4",
};
static SOC_ENUM_SINGLE_DECL(dsif_spk_enum, ABOX_ROUTE_CTRL0, ABOX_ROUTE_DSIF_L,
		dsif_spk_texts);
static const struct snd_kcontrol_new dsif_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", dsif_spk_enum),
};

static const struct snd_kcontrol_new uaif0_controls[] = {
	SOC_DAPM_SINGLE("UAIF0 Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new uaif1_controls[] = {
	SOC_DAPM_SINGLE("UAIF1 Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new uaif2_controls[] = {
	SOC_DAPM_SINGLE("UAIF2 Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new uaif3_controls[] = {
	SOC_DAPM_SINGLE("UAIF3 Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new dsif_controls[] = {
	SOC_DAPM_SINGLE("DSIF Switch", SND_SOC_NOPM, 0, 1, 1),
};

static const char * const rsrcx_texts[] = {
	"RESERVED", "SIFS0", "SIFS1", "SIFS2",
	"SIFS3", "SIFS4", "RESERVED", "RESERVED",
	"UAIF0", "UAIF1", "UAIF2", "UAIF3",
};
static SOC_ENUM_SINGLE_DECL(rsrc0_enum, ABOX_ROUTE_CTRL2, ABOX_ROUTE_RSRC_L(0),
		rsrcx_texts);
static const struct snd_kcontrol_new rsrc0_controls[] = {
	SOC_DAPM_ENUM("DEMUX", rsrc0_enum),
};
static SOC_ENUM_SINGLE_DECL(rsrc1_enum, ABOX_ROUTE_CTRL2, ABOX_ROUTE_RSRC_L(1),
		rsrcx_texts);
static const struct snd_kcontrol_new rsrc1_controls[] = {
	SOC_DAPM_ENUM("DEMUX", rsrc1_enum),
};

static const char * const nsrcx_texts[] = {
	"RESERVED", "SIFS0", "SIFS1", "SIFS2",
	"SIFS3", "SIFS4", "RESERVED", "RESERVED",
	"UAIF0", "UAIF1", "UAIF2", "UAIF3",
};
static SOC_ENUM_SINGLE_DECL(nsrc0_enum, ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_L(0),
		nsrcx_texts);
static const struct snd_kcontrol_new nsrc0_controls[] = {
	SOC_DAPM_ENUM("DEMUX", nsrc0_enum),
};
static SOC_ENUM_SINGLE_DECL(nsrc1_enum, ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_L(1),
		nsrcx_texts);
static const struct snd_kcontrol_new nsrc1_controls[] = {
	SOC_DAPM_ENUM("DEMUX", nsrc1_enum),
};
static SOC_ENUM_SINGLE_DECL(nsrc2_enum, ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_L(2),
		nsrcx_texts);
static const struct snd_kcontrol_new nsrc2_controls[] = {
	SOC_DAPM_ENUM("DEMUX", nsrc2_enum),
};
static SOC_ENUM_SINGLE_DECL(nsrc3_enum, ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_L(3),
		nsrcx_texts);
static const struct snd_kcontrol_new nsrc3_controls[] = {
	SOC_DAPM_ENUM("DEMUX", nsrc3_enum),
};
static SOC_ENUM_SINGLE_DECL(nsrc4_enum, ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_L(4),
		nsrcx_texts);
static const struct snd_kcontrol_new nsrc4_controls[] = {
	SOC_DAPM_ENUM("DEMUX", nsrc4_enum),
};
static SOC_ENUM_SINGLE_DECL(nsrc5_enum, ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_L(5),
		nsrcx_texts);
static const struct snd_kcontrol_new nsrc5_controls[] = {
	SOC_DAPM_ENUM("DEMUX", nsrc5_enum),
};
static SOC_ENUM_SINGLE_DECL(nsrc6_enum, ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_L(6),
		nsrcx_texts);
static const struct snd_kcontrol_new nsrc6_controls[] = {
	SOC_DAPM_ENUM("DEMUX", nsrc6_enum),
};

static const struct snd_kcontrol_new recp_controls[] = {
	SOC_DAPM_SINGLE("PIFS0", ABOX_SPUM_CTRL1, ABOX_RECP_SRC_VALID_L, 1, 0),
	SOC_DAPM_SINGLE("PIFS1", ABOX_SPUM_CTRL1, ABOX_RECP_SRC_VALID_H, 1, 0),
};

static const char * const sifmx_texts[] = {
	"WDMA", "SIFMS",
};
static SOC_ENUM_SINGLE_DECL(sifm0_enum, ABOX_SPUM_CTRL0,
		ABOX_FUNC_CHAIN_NSRC_OUT_L(0), sifmx_texts);
static const struct snd_kcontrol_new sifm0_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm0_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm1_enum, ABOX_SPUM_CTRL0,
		ABOX_FUNC_CHAIN_NSRC_OUT_L(1), sifmx_texts);
static const struct snd_kcontrol_new sifm1_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm1_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm2_enum, ABOX_SPUM_CTRL0,
		ABOX_FUNC_CHAIN_NSRC_OUT_L(2), sifmx_texts);
static const struct snd_kcontrol_new sifm2_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm2_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm3_enum, ABOX_SPUM_CTRL0,
		ABOX_FUNC_CHAIN_NSRC_OUT_L(3), sifmx_texts);
static const struct snd_kcontrol_new sifm3_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm3_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm4_enum, ABOX_SPUM_CTRL0,
		ABOX_FUNC_CHAIN_NSRC_OUT_L(4), sifmx_texts);
static const struct snd_kcontrol_new sifm4_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm4_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm5_enum, ABOX_SPUM_CTRL0,
		ABOX_FUNC_CHAIN_NSRC_OUT_L(5), sifmx_texts);
static const struct snd_kcontrol_new sifm5_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm5_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm6_enum, ABOX_SPUM_CTRL0,
		ABOX_FUNC_CHAIN_NSRC_OUT_L(6), sifmx_texts);
static const struct snd_kcontrol_new sifm6_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm6_enum),
};

static const char * const sifms_texts[] = {
	"RESERVED", "SIFM0", "SIFM1", "SIFM2",
	"SIFM3", "SIFM4", "SIFM5", "SIFM6",
};
static SOC_ENUM_SINGLE_DECL(sifms_enum, ABOX_SPUM_CTRL1, ABOX_SIFMS_OUT_SEL_L,
		sifms_texts);
static const struct snd_kcontrol_new sifms_controls[] = {
	SOC_DAPM_ENUM("MUX", sifms_enum),
};

static const struct snd_soc_dapm_widget cmpnt_widgets[] = {
	SND_SOC_DAPM_MUX("SPUSM", SND_SOC_NOPM, 0, 0, spusm_controls),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN5", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN6", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN7", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN8", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN9", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN10", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN11", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_DEMUX("SIFSM", SND_SOC_NOPM, 0, 0, sifsm_controls),

	SND_SOC_DAPM_MUX("SPUS IN0", SND_SOC_NOPM, 0, 0, spus_in0_controls),
	SND_SOC_DAPM_MUX("SPUS IN1", SND_SOC_NOPM, 0, 0, spus_in1_controls),
	SND_SOC_DAPM_MUX("SPUS IN2", SND_SOC_NOPM, 0, 0, spus_in2_controls),
	SND_SOC_DAPM_MUX("SPUS IN3", SND_SOC_NOPM, 0, 0, spus_in3_controls),
	SND_SOC_DAPM_MUX("SPUS IN4", SND_SOC_NOPM, 0, 0, spus_in4_controls),
	SND_SOC_DAPM_MUX("SPUS IN5", SND_SOC_NOPM, 0, 0, spus_in5_controls),
	SND_SOC_DAPM_MUX("SPUS IN6", SND_SOC_NOPM, 0, 0, spus_in6_controls),
	SND_SOC_DAPM_MUX("SPUS IN7", SND_SOC_NOPM, 0, 0, spus_in7_controls),
	SND_SOC_DAPM_MUX("SPUS IN8", SND_SOC_NOPM, 0, 0, spus_in8_controls),
	SND_SOC_DAPM_MUX("SPUS IN9", SND_SOC_NOPM, 0, 0, spus_in9_controls),
	SND_SOC_DAPM_MUX("SPUS IN10", SND_SOC_NOPM, 0, 0, spus_in10_controls),
	SND_SOC_DAPM_MUX("SPUS IN11", SND_SOC_NOPM, 0, 0, spus_in11_controls),
	SND_SOC_DAPM_PGA_E("SPUS ASRC0", SND_SOC_NOPM, 0, 0, NULL, 0,
			spus_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUS ASRC1", SND_SOC_NOPM, 1, 0, NULL, 0,
			spus_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUS ASRC2", SND_SOC_NOPM, 2, 0, NULL, 0,
			spus_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUS ASRC3", SND_SOC_NOPM, 3, 0, NULL, 0,
			spus_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUS ASRC4", SND_SOC_NOPM, 4, 0, NULL, 0,
			spus_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUS ASRC5", SND_SOC_NOPM, 5, 0, NULL, 0,
			spus_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUS ASRC6", SND_SOC_NOPM, 6, 0, NULL, 0,
			spus_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUS ASRC7", SND_SOC_NOPM, 7, 0, NULL, 0,
			spus_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUS ASRC8", SND_SOC_NOPM, 8, 0, NULL, 0,
			spus_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUS ASRC9", SND_SOC_NOPM, 9, 0, NULL, 0,
			spus_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUS ASRC10", SND_SOC_NOPM, 10, 0, NULL, 0,
			spus_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUS ASRC11", SND_SOC_NOPM, 11, 0, NULL, 0,
			spus_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_DEMUX("SPUS OUT0", SND_SOC_NOPM, 0, 0, spus_out0_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT1", SND_SOC_NOPM, 0, 0, spus_out1_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT2", SND_SOC_NOPM, 0, 0, spus_out2_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT3", SND_SOC_NOPM, 0, 0, spus_out3_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT4", SND_SOC_NOPM, 0, 0, spus_out4_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT5", SND_SOC_NOPM, 0, 0, spus_out5_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT6", SND_SOC_NOPM, 0, 0, spus_out6_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT7", SND_SOC_NOPM, 0, 0, spus_out7_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT8", SND_SOC_NOPM, 0, 0, spus_out8_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT9", SND_SOC_NOPM, 0, 0, spus_out9_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT10", SND_SOC_NOPM, 0, 0, spus_out10_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT11", SND_SOC_NOPM, 0, 0, spus_out11_controls),

	SND_SOC_DAPM_PGA("SPUS OUT0-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT1-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT2-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT3-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT4-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT5-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT6-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT7-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT8-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT9-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT10-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT11-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT0-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT1-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT2-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT3-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT4-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT5-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT6-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT7-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT8-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT9-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT10-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT11-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT0-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT1-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT2-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT3-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT4-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT5-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT6-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT7-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT8-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT9-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT10-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT11-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT0-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT1-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT2-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT3-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT4-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT5-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT6-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT7-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT8-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT9-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT10-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT11-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT0-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT1-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT2-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT3-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT4-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT5-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT6-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT7-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT8-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT9-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT10-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT11-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MUX("SIFS1", SND_SOC_NOPM, 0, 0, sifs1_controls),
	SND_SOC_DAPM_MUX("SIFS2", SND_SOC_NOPM, 0, 0, sifs2_controls),
	SND_SOC_DAPM_MUX("SIFS3", SND_SOC_NOPM, 0, 0, sifs3_controls),
	SND_SOC_DAPM_MUX("SIFS4", SND_SOC_NOPM, 0, 0, sifs4_controls),

	SND_SOC_DAPM_SWITCH("SIFS0 OUT", SND_SOC_NOPM, 0, 0, sifs0_out_controls),
	SND_SOC_DAPM_SWITCH("SIFS1 OUT", SND_SOC_NOPM, 0, 0, sifs1_out_controls),
	SND_SOC_DAPM_SWITCH("SIFS2 OUT", SND_SOC_NOPM, 0, 0, sifs2_out_controls),
	SND_SOC_DAPM_SWITCH("SIFS3 OUT", SND_SOC_NOPM, 0, 0, sifs3_out_controls),
	SND_SOC_DAPM_SWITCH("SIFS4 OUT", SND_SOC_NOPM, 0, 0, sifs4_out_controls),

	SND_SOC_DAPM_MUX("UAIF0 SPK", SND_SOC_NOPM, 0, 0, uaif0_spk_controls),
	SND_SOC_DAPM_MUX("UAIF1 SPK", SND_SOC_NOPM, 0, 0, uaif1_spk_controls),
	SND_SOC_DAPM_MUX("UAIF2 SPK", SND_SOC_NOPM, 0, 0, uaif2_spk_controls),
	SND_SOC_DAPM_MUX("UAIF3 SPK", SND_SOC_NOPM, 0, 0, uaif3_spk_controls),
	SND_SOC_DAPM_MUX("DSIF SPK", SND_SOC_NOPM, 0, 0, dsif_spk_controls),

	SND_SOC_DAPM_SWITCH("UAIF0 PLA", SND_SOC_NOPM, 0, 0, uaif0_controls),
	SND_SOC_DAPM_SWITCH("UAIF1 PLA", SND_SOC_NOPM, 0, 0, uaif1_controls),
	SND_SOC_DAPM_SWITCH("UAIF2 PLA", SND_SOC_NOPM, 0, 0, uaif2_controls),
	SND_SOC_DAPM_SWITCH("UAIF3 PLA", SND_SOC_NOPM, 0, 0, uaif3_controls),
	SND_SOC_DAPM_SWITCH("DSIF PLA", SND_SOC_NOPM, 0, 0, dsif_controls),

	SND_SOC_DAPM_SWITCH("UAIF0 CAP", SND_SOC_NOPM, 0, 0, uaif0_controls),
	SND_SOC_DAPM_SWITCH("UAIF1 CAP", SND_SOC_NOPM, 0, 0, uaif1_controls),
	SND_SOC_DAPM_SWITCH("UAIF2 CAP", SND_SOC_NOPM, 0, 0, uaif2_controls),
	SND_SOC_DAPM_SWITCH("UAIF3 CAP", SND_SOC_NOPM, 0, 0, uaif3_controls),

	SND_SOC_DAPM_MUX("RSRC0", SND_SOC_NOPM, 0, 0, rsrc0_controls),
	SND_SOC_DAPM_MUX("RSRC1", SND_SOC_NOPM, 0, 0, rsrc1_controls),
	SND_SOC_DAPM_MUX("NSRC0", SND_SOC_NOPM, 0, 0, nsrc0_controls),
	SND_SOC_DAPM_MUX("NSRC1", SND_SOC_NOPM, 0, 0, nsrc1_controls),
	SND_SOC_DAPM_MUX("NSRC2", SND_SOC_NOPM, 0, 0, nsrc2_controls),
	SND_SOC_DAPM_MUX("NSRC3", SND_SOC_NOPM, 0, 0, nsrc3_controls),
	SND_SOC_DAPM_MUX("NSRC4", SND_SOC_NOPM, 0, 0, nsrc4_controls),
	SND_SOC_DAPM_MUX("NSRC5", SND_SOC_NOPM, 0, 0, nsrc5_controls),
	SND_SOC_DAPM_MUX("NSRC6", SND_SOC_NOPM, 0, 0, nsrc6_controls),

	SOC_MIXER_ARRAY("RECP", SND_SOC_NOPM, 0, 0, recp_controls),

	SND_SOC_DAPM_PGA_E("SPUM ASRC0", SND_SOC_NOPM, 0, 0, NULL, 0,
			spum_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUM ASRC1", SND_SOC_NOPM, 1, 0, NULL, 0,
			spum_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUM ASRC2", SND_SOC_NOPM, 2, 0, NULL, 0,
			spum_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUM ASRC3", SND_SOC_NOPM, 3, 0, NULL, 0,
			spum_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUM ASRC4", SND_SOC_NOPM, 4, 0, NULL, 0,
			spum_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUM ASRC5", SND_SOC_NOPM, 5, 0, NULL, 0,
			spum_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUM ASRC6", SND_SOC_NOPM, 6, 0, NULL, 0,
			spum_asrc_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_E("SPUM ASRC7", SND_SOC_NOPM, 7, 0, NULL, 0,
			spum_asrc_event, SND_SOC_DAPM_PRE_PMU),

	SND_SOC_DAPM_DEMUX("SIFM0", SND_SOC_NOPM, 0, 0, sifm0_controls),
	SND_SOC_DAPM_DEMUX("SIFM1", SND_SOC_NOPM, 0, 0, sifm1_controls),
	SND_SOC_DAPM_DEMUX("SIFM2", SND_SOC_NOPM, 0, 0, sifm2_controls),
	SND_SOC_DAPM_DEMUX("SIFM3", SND_SOC_NOPM, 0, 0, sifm3_controls),
	SND_SOC_DAPM_DEMUX("SIFM4", SND_SOC_NOPM, 0, 0, sifm4_controls),
	SND_SOC_DAPM_DEMUX("SIFM5", SND_SOC_NOPM, 0, 0, sifm5_controls),
	SND_SOC_DAPM_DEMUX("SIFM6", SND_SOC_NOPM, 0, 0, sifm6_controls),

	SND_SOC_DAPM_PGA("SIFM0-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFM1-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFM2-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFM3-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFM4-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFM5-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFM6-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MUX("SIFMS", SND_SOC_NOPM, 0, 0, sifms_controls),
};

static const struct snd_soc_dapm_route cmpnt_routes[] = {
	/* sink, control, source */
	{"SIFSM", NULL, "SPUSM"},
	{"SIFSM-SPUS IN0", "SPUS IN0", "SIFSM"},
	{"SIFSM-SPUS IN1", "SPUS IN1", "SIFSM"},
	{"SIFSM-SPUS IN2", "SPUS IN2", "SIFSM"},
	{"SIFSM-SPUS IN3", "SPUS IN3", "SIFSM"},
	{"SIFSM-SPUS IN4", "SPUS IN4", "SIFSM"},
	{"SIFSM-SPUS IN5", "SPUS IN5", "SIFSM"},
	{"SIFSM-SPUS IN6", "SPUS IN6", "SIFSM"},
	{"SIFSM-SPUS IN7", "SPUS IN7", "SIFSM"},
	{"SIFSM-SPUS IN8", "SPUS IN8", "SIFSM"},
	{"SIFSM-SPUS IN9", "SPUS IN9", "SIFSM"},
	{"SIFSM-SPUS IN10", "SPUS IN10", "SIFSM"},
	{"SIFSM-SPUS IN11", "SPUS IN11", "SIFSM"},

	{"SPUS IN0", "SIFSM", "SIFSM-SPUS IN0"},
	{"SPUS IN1", "SIFSM", "SIFSM-SPUS IN1"},
	{"SPUS IN2", "SIFSM", "SIFSM-SPUS IN2"},
	{"SPUS IN3", "SIFSM", "SIFSM-SPUS IN3"},
	{"SPUS IN4", "SIFSM", "SIFSM-SPUS IN4"},
	{"SPUS IN5", "SIFSM", "SIFSM-SPUS IN5"},
	{"SPUS IN6", "SIFSM", "SIFSM-SPUS IN6"},
	{"SPUS IN7", "SIFSM", "SIFSM-SPUS IN7"},
	{"SPUS IN8", "SIFSM", "SIFSM-SPUS IN8"},
	{"SPUS IN9", "SIFSM", "SIFSM-SPUS IN9"},
	{"SPUS IN10", "SIFSM", "SIFSM-SPUS IN10"},
	{"SPUS IN11", "SIFSM", "SIFSM-SPUS IN11"},

	{"SPUS ASRC0", NULL, "SPUS IN0"},
	{"SPUS ASRC1", NULL, "SPUS IN1"},
	{"SPUS ASRC2", NULL, "SPUS IN2"},
	{"SPUS ASRC3", NULL, "SPUS IN3"},
	{"SPUS ASRC4", NULL, "SPUS IN4"},
	{"SPUS ASRC5", NULL, "SPUS IN5"},
	{"SPUS ASRC6", NULL, "SPUS IN6"},
	{"SPUS ASRC7", NULL, "SPUS IN7"},
	{"SPUS ASRC8", NULL, "SPUS IN8"},
	{"SPUS ASRC9", NULL, "SPUS IN9"},
	{"SPUS ASRC10", NULL, "SPUS IN10"},
	{"SPUS ASRC11", NULL, "SPUS IN11"},

	{"SPUS OUT0", NULL, "SPUS ASRC0"},
	{"SPUS OUT1", NULL, "SPUS ASRC1"},
	{"SPUS OUT2", NULL, "SPUS ASRC2"},
	{"SPUS OUT3", NULL, "SPUS ASRC3"},
	{"SPUS OUT4", NULL, "SPUS ASRC4"},
	{"SPUS OUT5", NULL, "SPUS ASRC5"},
	{"SPUS OUT6", NULL, "SPUS ASRC6"},
	{"SPUS OUT7", NULL, "SPUS ASRC7"},
	{"SPUS OUT8", NULL, "SPUS ASRC8"},
	{"SPUS OUT9", NULL, "SPUS ASRC9"},
	{"SPUS OUT10", NULL, "SPUS ASRC10"},
	{"SPUS OUT11", NULL, "SPUS ASRC11"},

	{"SPUS OUT0-SIFS0", "SIFS0", "SPUS OUT0"},
	{"SPUS OUT1-SIFS0", "SIFS0", "SPUS OUT1"},
	{"SPUS OUT2-SIFS0", "SIFS0", "SPUS OUT2"},
	{"SPUS OUT3-SIFS0", "SIFS0", "SPUS OUT3"},
	{"SPUS OUT4-SIFS0", "SIFS0", "SPUS OUT4"},
	{"SPUS OUT5-SIFS0", "SIFS0", "SPUS OUT5"},
	{"SPUS OUT6-SIFS0", "SIFS0", "SPUS OUT6"},
	{"SPUS OUT7-SIFS0", "SIFS0", "SPUS OUT7"},
	{"SPUS OUT8-SIFS0", "SIFS0", "SPUS OUT8"},
	{"SPUS OUT9-SIFS0", "SIFS0", "SPUS OUT9"},
	{"SPUS OUT10-SIFS0", "SIFS0", "SPUS OUT10"},
	{"SPUS OUT11-SIFS0", "SIFS0", "SPUS OUT11"},
	{"SPUS OUT0-SIFS1", "SIFS1", "SPUS OUT0"},
	{"SPUS OUT1-SIFS1", "SIFS1", "SPUS OUT1"},
	{"SPUS OUT2-SIFS1", "SIFS1", "SPUS OUT2"},
	{"SPUS OUT3-SIFS1", "SIFS1", "SPUS OUT3"},
	{"SPUS OUT4-SIFS1", "SIFS1", "SPUS OUT4"},
	{"SPUS OUT5-SIFS1", "SIFS1", "SPUS OUT5"},
	{"SPUS OUT6-SIFS1", "SIFS1", "SPUS OUT6"},
	{"SPUS OUT7-SIFS1", "SIFS1", "SPUS OUT7"},
	{"SPUS OUT8-SIFS1", "SIFS1", "SPUS OUT8"},
	{"SPUS OUT9-SIFS1", "SIFS1", "SPUS OUT9"},
	{"SPUS OUT10-SIFS1", "SIFS1", "SPUS OUT10"},
	{"SPUS OUT11-SIFS1", "SIFS1", "SPUS OUT11"},
	{"SPUS OUT0-SIFS2", "SIFS2", "SPUS OUT0"},
	{"SPUS OUT1-SIFS2", "SIFS2", "SPUS OUT1"},
	{"SPUS OUT2-SIFS2", "SIFS2", "SPUS OUT2"},
	{"SPUS OUT3-SIFS2", "SIFS2", "SPUS OUT3"},
	{"SPUS OUT4-SIFS2", "SIFS2", "SPUS OUT4"},
	{"SPUS OUT5-SIFS2", "SIFS2", "SPUS OUT5"},
	{"SPUS OUT6-SIFS2", "SIFS2", "SPUS OUT6"},
	{"SPUS OUT7-SIFS2", "SIFS2", "SPUS OUT7"},
	{"SPUS OUT8-SIFS2", "SIFS2", "SPUS OUT8"},
	{"SPUS OUT9-SIFS2", "SIFS2", "SPUS OUT9"},
	{"SPUS OUT10-SIFS2", "SIFS2", "SPUS OUT10"},
	{"SPUS OUT11-SIFS2", "SIFS2", "SPUS OUT11"},
	{"SPUS OUT0-SIFS3", "SIFS3", "SPUS OUT0"},
	{"SPUS OUT1-SIFS3", "SIFS3", "SPUS OUT1"},
	{"SPUS OUT2-SIFS3", "SIFS3", "SPUS OUT2"},
	{"SPUS OUT3-SIFS3", "SIFS3", "SPUS OUT3"},
	{"SPUS OUT4-SIFS3", "SIFS3", "SPUS OUT4"},
	{"SPUS OUT5-SIFS3", "SIFS3", "SPUS OUT5"},
	{"SPUS OUT6-SIFS3", "SIFS3", "SPUS OUT6"},
	{"SPUS OUT7-SIFS3", "SIFS3", "SPUS OUT7"},
	{"SPUS OUT8-SIFS3", "SIFS3", "SPUS OUT8"},
	{"SPUS OUT9-SIFS3", "SIFS3", "SPUS OUT9"},
	{"SPUS OUT10-SIFS3", "SIFS3", "SPUS OUT10"},
	{"SPUS OUT11-SIFS3", "SIFS3", "SPUS OUT11"},
	{"SPUS OUT0-SIFS4", "SIFS4", "SPUS OUT0"},
	{"SPUS OUT1-SIFS4", "SIFS4", "SPUS OUT1"},
	{"SPUS OUT2-SIFS4", "SIFS4", "SPUS OUT2"},
	{"SPUS OUT3-SIFS4", "SIFS4", "SPUS OUT3"},
	{"SPUS OUT4-SIFS4", "SIFS4", "SPUS OUT4"},
	{"SPUS OUT5-SIFS4", "SIFS4", "SPUS OUT5"},
	{"SPUS OUT6-SIFS4", "SIFS4", "SPUS OUT6"},
	{"SPUS OUT7-SIFS4", "SIFS4", "SPUS OUT7"},
	{"SPUS OUT8-SIFS4", "SIFS4", "SPUS OUT8"},
	{"SPUS OUT9-SIFS4", "SIFS4", "SPUS OUT9"},
	{"SPUS OUT10-SIFS4", "SIFS4", "SPUS OUT10"},
	{"SPUS OUT11-SIFS4", "SIFS4", "SPUS OUT11"},

	{"SIFS0", NULL, "SPUS OUT0-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT1-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT2-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT3-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT4-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT5-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT6-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT7-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT8-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT9-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT10-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT11-SIFS0"},
	{"SIFS1", "SPUS OUT0", "SPUS OUT0-SIFS1"},
	{"SIFS1", "SPUS OUT1", "SPUS OUT1-SIFS1"},
	{"SIFS1", "SPUS OUT2", "SPUS OUT2-SIFS1"},
	{"SIFS1", "SPUS OUT3", "SPUS OUT3-SIFS1"},
	{"SIFS1", "SPUS OUT4", "SPUS OUT4-SIFS1"},
	{"SIFS1", "SPUS OUT5", "SPUS OUT5-SIFS1"},
	{"SIFS1", "SPUS OUT6", "SPUS OUT6-SIFS1"},
	{"SIFS1", "SPUS OUT7", "SPUS OUT7-SIFS1"},
	{"SIFS1", "SPUS OUT8", "SPUS OUT8-SIFS1"},
	{"SIFS1", "SPUS OUT9", "SPUS OUT9-SIFS1"},
	{"SIFS1", "SPUS OUT10", "SPUS OUT10-SIFS1"},
	{"SIFS1", "SPUS OUT11", "SPUS OUT11-SIFS1"},
	{"SIFS2", "SPUS OUT0", "SPUS OUT0-SIFS2"},
	{"SIFS2", "SPUS OUT1", "SPUS OUT1-SIFS2"},
	{"SIFS2", "SPUS OUT2", "SPUS OUT2-SIFS2"},
	{"SIFS2", "SPUS OUT3", "SPUS OUT3-SIFS2"},
	{"SIFS2", "SPUS OUT4", "SPUS OUT4-SIFS2"},
	{"SIFS2", "SPUS OUT5", "SPUS OUT5-SIFS2"},
	{"SIFS2", "SPUS OUT6", "SPUS OUT6-SIFS2"},
	{"SIFS2", "SPUS OUT7", "SPUS OUT7-SIFS2"},
	{"SIFS2", "SPUS OUT8", "SPUS OUT8-SIFS2"},
	{"SIFS2", "SPUS OUT9", "SPUS OUT9-SIFS2"},
	{"SIFS2", "SPUS OUT10", "SPUS OUT10-SIFS2"},
	{"SIFS2", "SPUS OUT11", "SPUS OUT11-SIFS2"},
	{"SIFS3", "SPUS OUT0", "SPUS OUT0-SIFS3"},
	{"SIFS3", "SPUS OUT1", "SPUS OUT1-SIFS3"},
	{"SIFS3", "SPUS OUT2", "SPUS OUT2-SIFS3"},
	{"SIFS3", "SPUS OUT3", "SPUS OUT3-SIFS3"},
	{"SIFS3", "SPUS OUT4", "SPUS OUT4-SIFS3"},
	{"SIFS3", "SPUS OUT5", "SPUS OUT5-SIFS3"},
	{"SIFS3", "SPUS OUT6", "SPUS OUT6-SIFS3"},
	{"SIFS3", "SPUS OUT7", "SPUS OUT7-SIFS3"},
	{"SIFS3", "SPUS OUT8", "SPUS OUT8-SIFS3"},
	{"SIFS3", "SPUS OUT9", "SPUS OUT9-SIFS3"},
	{"SIFS3", "SPUS OUT10", "SPUS OUT10-SIFS3"},
	{"SIFS3", "SPUS OUT11", "SPUS OUT11-SIFS3"},
	{"SIFS4", "SPUS OUT0", "SPUS OUT0-SIFS4"},
	{"SIFS4", "SPUS OUT1", "SPUS OUT1-SIFS4"},
	{"SIFS4", "SPUS OUT2", "SPUS OUT2-SIFS4"},
	{"SIFS4", "SPUS OUT3", "SPUS OUT3-SIFS4"},
	{"SIFS4", "SPUS OUT4", "SPUS OUT4-SIFS4"},
	{"SIFS4", "SPUS OUT5", "SPUS OUT5-SIFS4"},
	{"SIFS4", "SPUS OUT6", "SPUS OUT6-SIFS4"},
	{"SIFS4", "SPUS OUT7", "SPUS OUT7-SIFS4"},
	{"SIFS4", "SPUS OUT8", "SPUS OUT8-SIFS4"},
	{"SIFS4", "SPUS OUT9", "SPUS OUT9-SIFS4"},
	{"SIFS4", "SPUS OUT10", "SPUS OUT10-SIFS4"},
	{"SIFS4", "SPUS OUT11", "SPUS OUT11-SIFS4"},

	{"SIFS0 OUT", "Switch", "SIFS0"},
	{"SIFS1 OUT", "Switch", "SIFS1"},
	{"SIFS2 OUT", "Switch", "SIFS2"},
	{"SIFS3 OUT", "Switch", "SIFS3"},
	{"SIFS4 OUT", "Switch", "SIFS4"},

	{"UAIF0 SPK", "SIFS0", "SIFS0 OUT"},
	{"UAIF0 SPK", "SIFS1", "SIFS1 OUT"},
	{"UAIF0 SPK", "SIFS2", "SIFS2 OUT"},
	{"UAIF0 SPK", "SIFS3", "SIFS3 OUT"},
	{"UAIF0 SPK", "SIFS4", "SIFS4 OUT"},
	{"UAIF0 SPK", "SIFMS", "SIFMS"},
	{"UAIF1 SPK", "SIFS0", "SIFS0 OUT"},
	{"UAIF1 SPK", "SIFS1", "SIFS1 OUT"},
	{"UAIF1 SPK", "SIFS2", "SIFS2 OUT"},
	{"UAIF1 SPK", "SIFS3", "SIFS3 OUT"},
	{"UAIF1 SPK", "SIFS4", "SIFS4 OUT"},
	{"UAIF1 SPK", "SIFMS", "SIFMS"},
	{"UAIF2 SPK", "SIFS0", "SIFS0 OUT"},
	{"UAIF2 SPK", "SIFS1", "SIFS1 OUT"},
	{"UAIF2 SPK", "SIFS2", "SIFS2 OUT"},
	{"UAIF2 SPK", "SIFS3", "SIFS3 OUT"},
	{"UAIF2 SPK", "SIFS4", "SIFS4 OUT"},
	{"UAIF2 SPK", "SIFMS", "SIFMS"},
	{"UAIF3 SPK", "SIFS0", "SIFS0 OUT"},
	{"UAIF3 SPK", "SIFS1", "SIFS1 OUT"},
	{"UAIF3 SPK", "SIFS2", "SIFS2 OUT"},
	{"UAIF3 SPK", "SIFS3", "SIFS3 OUT"},
	{"UAIF3 SPK", "SIFS4", "SIFS4 OUT"},
	{"UAIF3 SPK", "SIFMS", "SIFMS"},
	{"DSIF SPK", "SIFS1", "SIFS1 OUT"},
	{"DSIF SPK", "SIFS2", "SIFS2 OUT"},
	{"DSIF SPK", "SIFS3", "SIFS3 OUT"},
	{"DSIF SPK", "SIFS4", "SIFS4 OUT"},

	{"UAIF0 PLA", "UAIF0 Switch", "UAIF0 SPK"},
	{"UAIF1 PLA", "UAIF1 Switch", "UAIF1 SPK"},
	{"UAIF2 PLA", "UAIF2 Switch", "UAIF2 SPK"},
	{"UAIF3 PLA", "UAIF3 Switch", "UAIF3 SPK"},
	{"DSIF PLA", "DSIF Switch", "DSIF SPK"},

	{"SIFS0", NULL, "SIFS0 Capture"},
	{"SIFS1", NULL, "SIFS1 Capture"},
	{"SIFS2", NULL, "SIFS2 Capture"},
	{"SIFS3", NULL, "SIFS3 Capture"},
	{"SIFS4", NULL, "SIFS4 Capture"},

	{"RSRC0 Playback", NULL, "RSRC0"},
	{"RSRC1 Playback", NULL, "RSRC1"},
	{"NSRC0 Playback", NULL, "NSRC0"},
	{"NSRC1 Playback", NULL, "NSRC1"},
	{"NSRC2 Playback", NULL, "NSRC2"},
	{"NSRC3 Playback", NULL, "NSRC3"},
	{"NSRC4 Playback", NULL, "NSRC4"},
	{"NSRC5 Playback", NULL, "NSRC5"},
	{"NSRC6 Playback", NULL, "NSRC6"},

	{"RSRC0", NULL, "RSRC0 Capture"},
	{"RSRC1", NULL, "RSRC1 Capture"},
	{"NSRC0", NULL, "NSRC0 Capture"},
	{"NSRC1", NULL, "NSRC1 Capture"},
	{"NSRC2", NULL, "NSRC2 Capture"},
	{"NSRC3", NULL, "NSRC3 Capture"},
	{"NSRC4", NULL, "NSRC4 Capture"},
	{"NSRC5", NULL, "NSRC5 Capture"},
	{"NSRC6", NULL, "NSRC6 Capture"},

	{"RSRC0", "SIFS0", "SIFS0 OUT"},
	{"RSRC0", "SIFS1", "SIFS1 OUT"},
	{"RSRC0", "SIFS2", "SIFS2 OUT"},
	{"RSRC0", "SIFS3", "SIFS3 OUT"},
	{"RSRC0", "SIFS4", "SIFS4 OUT"},
	{"RSRC0", "UAIF0", "UAIF0 CAP"},
	{"RSRC0", "UAIF1", "UAIF1 CAP"},
	{"RSRC0", "UAIF2", "UAIF2 CAP"},
	{"RSRC0", "UAIF3", "UAIF3 CAP"},

	{"RSRC1", "SIFS0", "SIFS0 OUT"},
	{"RSRC1", "SIFS1", "SIFS1 OUT"},
	{"RSRC1", "SIFS2", "SIFS2 OUT"},
	{"RSRC1", "SIFS3", "SIFS3 OUT"},
	{"RSRC1", "SIFS4", "SIFS4 OUT"},
	{"RSRC1", "UAIF0", "UAIF0 CAP"},
	{"RSRC1", "UAIF1", "UAIF1 CAP"},
	{"RSRC1", "UAIF2", "UAIF2 CAP"},
	{"RSRC1", "UAIF3", "UAIF3 CAP"},

	{"NSRC0", "SIFS0", "SIFS0 OUT"},
	{"NSRC0", "SIFS1", "SIFS1 OUT"},
	{"NSRC0", "SIFS2", "SIFS2 OUT"},
	{"NSRC0", "SIFS3", "SIFS3 OUT"},
	{"NSRC0", "SIFS4", "SIFS4 OUT"},
	{"NSRC0", "UAIF0", "UAIF0 CAP"},
	{"NSRC0", "UAIF1", "UAIF1 CAP"},
	{"NSRC0", "UAIF2", "UAIF2 CAP"},
	{"NSRC0", "UAIF3", "UAIF3 CAP"},

	{"NSRC1", "SIFS0", "SIFS0 OUT"},
	{"NSRC1", "SIFS1", "SIFS1 OUT"},
	{"NSRC1", "SIFS2", "SIFS2 OUT"},
	{"NSRC1", "SIFS3", "SIFS3 OUT"},
	{"NSRC1", "SIFS4", "SIFS4 OUT"},
	{"NSRC1", "UAIF0", "UAIF0 CAP"},
	{"NSRC1", "UAIF1", "UAIF1 CAP"},
	{"NSRC1", "UAIF2", "UAIF2 CAP"},
	{"NSRC1", "UAIF3", "UAIF3 CAP"},

	{"NSRC2", "SIFS0", "SIFS0 OUT"},
	{"NSRC2", "SIFS1", "SIFS1 OUT"},
	{"NSRC2", "SIFS2", "SIFS2 OUT"},
	{"NSRC2", "SIFS3", "SIFS3 OUT"},
	{"NSRC2", "SIFS4", "SIFS4 OUT"},
	{"NSRC2", "UAIF0", "UAIF0 CAP"},
	{"NSRC2", "UAIF1", "UAIF1 CAP"},
	{"NSRC2", "UAIF2", "UAIF2 CAP"},
	{"NSRC2", "UAIF3", "UAIF3 CAP"},

	{"NSRC3", "SIFS0", "SIFS0 OUT"},
	{"NSRC3", "SIFS1", "SIFS1 OUT"},
	{"NSRC3", "SIFS2", "SIFS2 OUT"},
	{"NSRC3", "SIFS3", "SIFS3 OUT"},
	{"NSRC3", "SIFS4", "SIFS4 OUT"},
	{"NSRC3", "UAIF0", "UAIF0 CAP"},
	{"NSRC3", "UAIF1", "UAIF1 CAP"},
	{"NSRC3", "UAIF2", "UAIF2 CAP"},
	{"NSRC3", "UAIF3", "UAIF3 CAP"},

	{"NSRC4", "SIFS0", "SIFS0 OUT"},
	{"NSRC4", "SIFS1", "SIFS1 OUT"},
	{"NSRC4", "SIFS2", "SIFS2 OUT"},
	{"NSRC4", "SIFS3", "SIFS3 OUT"},
	{"NSRC4", "SIFS4", "SIFS4 OUT"},
	{"NSRC4", "UAIF0", "UAIF0 CAP"},
	{"NSRC4", "UAIF1", "UAIF1 CAP"},
	{"NSRC4", "UAIF2", "UAIF2 CAP"},
	{"NSRC4", "UAIF3", "UAIF3 CAP"},

	{"NSRC5", "SIFS0", "SIFS0 OUT"},
	{"NSRC5", "SIFS1", "SIFS1 OUT"},
	{"NSRC5", "SIFS2", "SIFS2 OUT"},
	{"NSRC5", "SIFS3", "SIFS3 OUT"},
	{"NSRC5", "SIFS4", "SIFS4 OUT"},
	{"NSRC5", "UAIF0", "UAIF0 CAP"},
	{"NSRC5", "UAIF1", "UAIF1 CAP"},
	{"NSRC5", "UAIF2", "UAIF2 CAP"},
	{"NSRC5", "UAIF3", "UAIF3 CAP"},

	{"NSRC6", "SIFS0", "SIFS0 OUT"},
	{"NSRC6", "SIFS1", "SIFS1 OUT"},
	{"NSRC6", "SIFS2", "SIFS2 OUT"},
	{"NSRC6", "SIFS3", "SIFS3 OUT"},
	{"NSRC6", "SIFS4", "SIFS4 OUT"},
	{"NSRC6", "UAIF0", "UAIF0 CAP"},
	{"NSRC6", "UAIF1", "UAIF1 CAP"},
	{"NSRC6", "UAIF2", "UAIF2 CAP"},
	{"NSRC6", "UAIF3", "UAIF3 CAP"},

	{"RECP", "PIFS0", "RSRC0"},
	{"RECP", "PIFS1", "RSRC1"},

	{"SPUM ASRC0", NULL, "RECP"},
	{"SPUM ASRC1", NULL, "NSRC0"},
	{"SPUM ASRC2", NULL, "NSRC1"},
	{"SPUM ASRC3", NULL, "NSRC2"},
	{"SPUM ASRC4", NULL, "NSRC3"},
	{"SPUM ASRC5", NULL, "NSRC4"},
	{"SPUM ASRC6", NULL, "NSRC5"},
	{"SPUM ASRC7", NULL, "NSRC6"},

	{"SIFM0", NULL, "SPUM ASRC1"},
	{"SIFM1", NULL, "SPUM ASRC2"},
	{"SIFM2", NULL, "SPUM ASRC3"},
	{"SIFM3", NULL, "SPUM ASRC4"},
	{"SIFM4", NULL, "SPUM ASRC5"},
	{"SIFM5", NULL, "SPUM ASRC6"},
	{"SIFM6", NULL, "SPUM ASRC7"},

	{"SIFM0-SIFMS", "SIFMS", "SIFM0"},
	{"SIFM1-SIFMS", "SIFMS", "SIFM1"},
	{"SIFM2-SIFMS", "SIFMS", "SIFM2"},
	{"SIFM3-SIFMS", "SIFMS", "SIFM3"},
	{"SIFM4-SIFMS", "SIFMS", "SIFM4"},
	{"SIFM5-SIFMS", "SIFMS", "SIFM5"},
	{"SIFM6-SIFMS", "SIFMS", "SIFM6"},

	{"SIFMS", "SIFM0", "SIFM0-SIFMS"},
	{"SIFMS", "SIFM1", "SIFM1-SIFMS"},
	{"SIFMS", "SIFM2", "SIFM2-SIFMS"},
	{"SIFMS", "SIFM3", "SIFM3-SIFMS"},
	{"SIFMS", "SIFM4", "SIFM4-SIFMS"},
	{"SIFMS", "SIFM5", "SIFM5-SIFMS"},
	{"SIFMS", "SIFM6", "SIFM6-SIFMS"},
};

static int cmpnt_probe(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	snd_soc_component_init_regmap(cmpnt, data->regmap);

	snd_soc_add_component_controls(cmpnt, spus_asrc_controls,
			ARRAY_SIZE(spus_asrc_controls));
	snd_soc_add_component_controls(cmpnt, spum_asrc_controls,
			ARRAY_SIZE(spum_asrc_controls));
	snd_soc_add_component_controls(cmpnt, spus_asrc_id_controls,
			ARRAY_SIZE(spus_asrc_id_controls));
	snd_soc_add_component_controls(cmpnt, spum_asrc_id_controls,
			ARRAY_SIZE(spum_asrc_id_controls));
	snd_soc_add_component_controls(cmpnt, spus_asrc_apf_coef_controls,
			ARRAY_SIZE(spus_asrc_apf_coef_controls));
	snd_soc_add_component_controls(cmpnt, spum_asrc_apf_coef_controls,
			ARRAY_SIZE(spum_asrc_apf_coef_controls));
	snd_soc_add_component_controls(cmpnt, spus_asrc_os_controls,
			ARRAY_SIZE(spus_asrc_os_controls));
	snd_soc_add_component_controls(cmpnt, spus_asrc_is_controls,
			ARRAY_SIZE(spus_asrc_is_controls));
	snd_soc_add_component_controls(cmpnt, spum_asrc_os_controls,
			ARRAY_SIZE(spum_asrc_os_controls));
	snd_soc_add_component_controls(cmpnt, spum_asrc_is_controls,
			ARRAY_SIZE(spum_asrc_is_controls));

	data->cmpnt = cmpnt;

	/* vdma and dump are initialized in abox component probe
	 * to set vdma to sound card 1 and dump to sound card 2.
	 */
	abox_vdma_init(dev);
	abox_dump_init(dev);

	wakeup_source_init(&data->ws, "abox");

	return 0;
}

static void cmpnt_remove(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	wakeup_source_trash(&data->ws);
}

static const struct snd_soc_component_driver abox_cmpnt_drv = {
	.probe			= cmpnt_probe,
	.remove			= cmpnt_remove,
	.controls		= cmpnt_controls,
	.num_controls		= ARRAY_SIZE(cmpnt_controls),
	.dapm_widgets		= cmpnt_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(cmpnt_widgets),
	.dapm_routes		= cmpnt_routes,
	.num_dapm_routes	= ARRAY_SIZE(cmpnt_routes),
	.probe_order		= SND_SOC_COMP_ORDER_FIRST,
};

int abox_cmpnt_adjust_sbank(struct snd_soc_component *cmpnt,
		enum abox_dai id, struct snd_pcm_hw_params *params)
{
	unsigned int time, size, reg, val;
	unsigned int mask = ABOX_SBANK_SIZE_MASK;
	int ret;

	switch (id) {
	case ABOX_RDMA0:
	case ABOX_RDMA1:
	case ABOX_RDMA2:
	case ABOX_RDMA3:
	case ABOX_RDMA4:
	case ABOX_RDMA5:
	case ABOX_RDMA6:
	case ABOX_RDMA7:
	case ABOX_RDMA8:
	case ABOX_RDMA9:
	case ABOX_RDMA10:
	case ABOX_RDMA11:
		reg = ABOX_SPUS_SBANK_RDMA(id - ABOX_RDMA0);
		break;
	case ABOX_RSRC0:
	case ABOX_RSRC1:
		reg = ABOX_SPUM_SBANK_RSRC(id - ABOX_RSRC0);
		break;
	case ABOX_NSRC0:
	case ABOX_NSRC1:
	case ABOX_NSRC2:
	case ABOX_NSRC3:
	case ABOX_NSRC4:
	case ABOX_NSRC5:
	case ABOX_NSRC6:
		reg = ABOX_SPUM_SBANK_NSRC(id - ABOX_NSRC0);
		break;
	default:
		return -EINVAL;
	}

	time = hw_param_interval_c(params, SNDRV_PCM_HW_PARAM_PERIOD_TIME)->min;
	time /= 1000;
	if (time <= 4)
		size = SZ_32;
	else if (time <= 10)
		size = SZ_128;
	else
		size = SZ_512;

	/* Sbank size unit is 16byte(= 4 shifts) */
	val = size << (ABOX_SBANK_SIZE_L - 4);

	dev_info(cmpnt->dev, "%s(%#x, %ums): %u\n", __func__, id, time, size);

	ret = snd_soc_component_update_bits(cmpnt, reg, mask, val);
	if (ret < 0) {
		dev_err(cmpnt->dev, "sbank write error: %d\n", ret);
		return ret;
	}

	return size;
}

static unsigned int sifsx_cnt_val(unsigned long aclk, unsigned int rate,
		unsigned int physical_width, unsigned int channels)
{
	static const int correction = 4;
	unsigned int n, d;

	/* k = n / d */
	d = channels * rate;
	n = 2 * (32 / physical_width);

	return DIV_ROUND_CLOSEST_ULL(aclk * n, d) - 5 + correction;
}

int abox_cmpnt_reset_cnt_val(struct device *dev,
		struct snd_soc_component *cmpnt, enum abox_dai id)
{
	unsigned int idx, val, sifs_id;
	int ret = 0;

	dev_dbg(dev, "%s(%#x)\n", __func__, id);

	ret = snd_soc_component_read(cmpnt, ABOX_ROUTE_CTRL0, &val);
	if (ret < 0)
		return ret;

	switch (id) {
	case ABOX_UAIF0:
	case ABOX_UAIF1:
	case ABOX_UAIF2:
	case ABOX_UAIF3:
	case ABOX_UAIF4:
		idx = id - ABOX_UAIF0;
		sifs_id = val & ABOX_ROUTE_UAIF_SPK_MASK(idx);
		sifs_id = (sifs_id >> ABOX_ROUTE_UAIF_SPK_L(idx)) - 1;
		break;
	case ABOX_DSIF:
		sifs_id = val & ABOX_ROUTE_DSIF_MASK;
		sifs_id = (sifs_id >> ABOX_ROUTE_DSIF_L) - 1;
		break;
	default:
		return -EINVAL;
	}

	if (sifs_id < 12) {
		ret = snd_soc_component_write(cmpnt,
				ABOX_SPUS_CTRL_SIFS_CNT(sifs_id), 0);
		if (ret < 0)
			return ret;
		dev_info(dev, "reset sifs%d_cnt_val\n", sifs_id);
	}

	return ret;
}

static int sifs_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct regmap *regmap = data->regmap;
	enum abox_dai id = dai->id;
	int idx = id - ABOX_SIFS0;
	unsigned int rate = params_rate(params);
	unsigned int width = params_width(params);
	unsigned int pwidth = params_physical_width(params);
	unsigned int channels = params_channels(params);
	unsigned long clk;
	unsigned int cnt_val;
	int ret = 0;

	if (substream->stream != SNDRV_PCM_STREAM_CAPTURE)
		goto out;

	ret = abox_register_bclk_usage(dev, data, id, rate, channels, width);
	if (ret < 0)
		dev_err(dev, "Unable to register bclk usage: %d\n", ret);

	clk = clk_get_rate(data->clk_cnt);
	cnt_val = sifsx_cnt_val(clk, rate, pwidth, channels);

	dev_info(dev, "%s[%#x](%ubit %uchannel %uHz at %luHz): %u\n",
			__func__, id, width, channels, rate, clk, cnt_val);

	ret = regmap_update_bits(regmap, ABOX_SPUS_CTRL_SIFS_CNT(idx),
			ABOX_SIFS_CNT_VAL_MASK(idx),
			cnt_val << ABOX_SIFS_CNT_VAL_L(idx));
out:
	return ret;
}

static const struct snd_soc_dai_ops sifs_dai_ops = {
	.hw_params	= sifs_hw_params,
};

static int src_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	enum abox_dai id = dai->id;
	struct snd_soc_component *cmpnt = dai->component;
	int ret = 0;

	if (substream->stream != SNDRV_PCM_STREAM_CAPTURE)
		return 0;

	dev_dbg(dev, "%s[%#x]\n", __func__, id);

	ret = abox_cmpnt_adjust_sbank(cmpnt, id, params);
	if (ret < 0)
		return ret;

	return 0;
}

static const struct snd_soc_dai_ops src_dai_ops = {
	.hw_params	= src_hw_params,
};

static struct snd_soc_dai_driver abox_cmpnt_dai_drv[] = {
	{
		.name = "SIFS0",
		.id = ABOX_SIFS0,
		.capture = {
			.stream_name = "SIFS0 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &sifs_dai_ops,
	},
	{
		.name = "SIFS1",
		.id = ABOX_SIFS1,
		.capture = {
			.stream_name = "SIFS1 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &sifs_dai_ops,
	},
	{
		.name = "SIFS2",
		.id = ABOX_SIFS2,
		.capture = {
			.stream_name = "SIFS2 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &sifs_dai_ops,
	},
	{
		.name = "SIFS3",
		.id = ABOX_SIFS3,
		.capture = {
			.stream_name = "SIFS3 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &sifs_dai_ops,
	},
	{
		.name = "SIFS4",
		.id = ABOX_SIFS4,
		.capture = {
			.stream_name = "SIFS4 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &sifs_dai_ops,
	},
	{
		.name = "RSRC0",
		.id = ABOX_RSRC0,
		.playback = {
			.stream_name = "RSRC0 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "RSRC0 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &src_dai_ops,
	},
	{
		.name = "RSRC1",
		.id = ABOX_RSRC1,
		.playback = {
			.stream_name = "RSRC1 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "RSRC1 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &src_dai_ops,
	},
	{
		.name = "NSRC0",
		.id = ABOX_NSRC0,
		.playback = {
			.stream_name = "NSRC0 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "NSRC0 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &src_dai_ops,
	},
	{
		.name = "NSRC1",
		.id = ABOX_NSRC1,
		.playback = {
			.stream_name = "NSRC1 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "NSRC1 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &src_dai_ops,
	},
	{
		.name = "NSRC2",
		.id = ABOX_NSRC2,
		.playback = {
			.stream_name = "NSRC2 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "NSRC2 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &src_dai_ops,
	},
	{
		.name = "NSRC3",
		.id = ABOX_NSRC3,
		.playback = {
			.stream_name = "NSRC3 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "NSRC3 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &src_dai_ops,
	},
	{
		.name = "NSRC4",
		.id = ABOX_NSRC4,
		.playback = {
			.stream_name = "NSRC4 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "NSRC4 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &src_dai_ops,
	},
	{
		.name = "NSRC5",
		.id = ABOX_NSRC5,
		.playback = {
			.stream_name = "NSRC5 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "NSRC5 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &src_dai_ops,
	},
	{
		.name = "NSRC6",
		.id = ABOX_NSRC6,
		.playback = {
			.stream_name = "NSRC6 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "NSRC6 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &src_dai_ops,
	},
};

int abox_cmpnt_update_cnt_val(struct device *adev)
{
	/* nothing to do anymore */
	return 0;
}

int abox_cmpnt_hw_params_fixup_helper(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params, int stream)
{
	struct snd_soc_dai *dai = rtd->cpu_dai;
	struct snd_soc_component *cmpnt = dai->component;
	struct device *dev = dai->dev;
	struct device *adev = is_abox(dai->dev) ? dai->dev : dai->dev->parent;
	struct abox_data *data = dev_get_drvdata(adev);
	struct snd_soc_dpcm *dpcm;
	struct snd_soc_pcm_runtime *fe = NULL;
	struct snd_soc_dapm_widget *w, *w_tgt = NULL, *w_src;
	struct snd_soc_dapm_path *path;
	LIST_HEAD(w_list);
	enum ABOX_CONFIGMSG msg_rate, msg_format;
	struct snd_pcm_hw_params fe_params = *params;
	unsigned int rate, channels, width;
	snd_pcm_format_t format;
	struct snd_soc_dapm_widget *w_mst = NULL;
	int stream_mst;

	dev_dbg(dev, "%s[%s](%d)\n", __func__, dai->name, stream);

	if (params_channels(params) < 2) {
		dev_info(dev, "channel is fixed from %u to 2\n",
				params_channels(params));
		hw_param_interval(params, SNDRV_PCM_HW_PARAM_CHANNELS)->min = 2;
	}

	if (params_width(params) < 16) {
		dev_info(dev, "width is fixed from %d to 16\n",
				params_width(params));
		params_set_format(params, SNDRV_PCM_FORMAT_S16);
	}

	snd_soc_dapm_mutex_lock(snd_soc_component_get_dapm(cmpnt));

	list_for_each_entry(dpcm, &rtd->dpcm[stream].fe_clients, list_fe) {
		if (dpcm->fe && snd_soc_dpcm_fe_can_update(dpcm->fe, stream)) {
			switch (dpcm->fe->dpcm[stream].state) {
			case SND_SOC_DPCM_STATE_OPEN:
			case SND_SOC_DPCM_STATE_HW_PARAMS:
			case SND_SOC_DPCM_STATE_HW_FREE:
				fe = dpcm->fe;
				break;
			default:
				/* nothing */
				break;
			}
		}
	}

	/*
	 * For snd_soc_dapm_connected_{output,input}_ep fully discover the graph
	 * we need to reset the cached number of inputs and outputs.
	 */
	list_for_each_entry(w, &cmpnt->card->widgets, list) {
		w->endpoints[SND_SOC_DAPM_DIR_IN] = -1;
		w->endpoints[SND_SOC_DAPM_DIR_OUT] = -1;
	}

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		w_src = dai->playback_widget;
		snd_soc_dapm_widget_for_each_source_path(w_src, path) {
			if (path->connect) {
				w = path->node[SND_SOC_DAPM_DIR_IN];
				snd_soc_dapm_connected_input_ep(w, &w_list);
			}
		}
	} else {
		w_src = dai->capture_widget;
		snd_soc_dapm_widget_for_each_sink_path(w_src, path) {
			if (path->connect) {
				w = path->node[SND_SOC_DAPM_DIR_OUT];
				snd_soc_dapm_connected_output_ep(w, &w_list);
			}
		}
	}

	/* if widget list is empty, assume the rtd is fe not be */
	if (list_empty(&w_list)) {
		if (stream == SNDRV_PCM_STREAM_PLAYBACK)
			snd_soc_dapm_connected_output_ep(dai->playback_widget,
					&w_list);
		else
			snd_soc_dapm_connected_input_ep(dai->capture_widget,
					&w_list);
	}

	/* find current params */
	list_for_each_entry(w, &w_list, work_list) {
		bool slave;

		dev_dbg(dev, "%s\n", w->name);

		if (!find_nrf_stream(w, stream, &msg_rate, &msg_format, &slave))
			continue;

		/* NSRC and RSRC can be connected to same BE.
		 * To find out right ABOX_CONFIGMSG, compare nearest DAI widget
		 * with FE DAI widget, if stream is CAPTURE.
		 */
		if (stream == SNDRV_PCM_STREAM_CAPTURE && fe) {
			struct snd_soc_dapm_widget *w_dai;

			w_dai = find_nearst_dai_widget(&w_list, w);
			if (fe->cpu_dai && w_dai && fe->cpu_dai->capture_widget
					!= w_dai)
				continue;
		}

		if (slave)
			w_mst = sync_params(data, &w_list, &stream_mst,
					w, msg_rate, msg_format);

		format = get_sif_format(data, msg_format);
		width = snd_pcm_format_width(format);
		rate = get_sif_rate(data, msg_rate);
		channels = get_sif_channels(data, msg_format);
		dev_dbg(dev, "%s: %s: find %d bit, %u channel, %uHz\n",
				__func__, w->name, width, channels, rate);
		w_tgt = w;
		break;
	}

	if (!w_tgt)
		goto unlock;

	if (!w_mst) {
		w_mst = w_tgt;
		stream_mst = stream;
	}

	/* override formats to UAIF format, if it is connected */
	if (abox_if_hw_params_fixup_helper(rtd, params, stream) >= 0) {
		format = params_format(params);
		width = params_width(params);
		rate = params_rate(params);
		channels = params_channels(params);
	} else if (w_mst) {
		list_for_each_entry(w, &cmpnt->card->widgets, list) {
			w->endpoints[SND_SOC_DAPM_DIR_IN] = -1;
			w->endpoints[SND_SOC_DAPM_DIR_OUT] = -1;
		}
		INIT_LIST_HEAD(&w_list);
		if (stream_mst == SNDRV_PCM_STREAM_PLAYBACK)
			snd_soc_dapm_connected_output_ep(w_mst, &w_list);
		else
			snd_soc_dapm_connected_input_ep(w_mst, &w_list);

		list_for_each_entry(w, &w_list, work_list) {
			struct snd_soc_dai *dai;

			if (!w->sname)
				continue;

			dai = w->priv;
			if (abox_if_hw_params_fixup(dai, params, stream) >= 0) {
				format = params_format(params);
				width = params_width(params);
				rate = params_rate(params);
				channels = params_channels(params);
				break;
			}
		}
	}

	dev_dbg(dev, "%s: set to %u bit, %u channel, %uHz\n", __func__,
			width, channels, rate);
unlock:
	snd_soc_dapm_mutex_unlock(snd_soc_component_get_dapm(cmpnt));

	if (!w_tgt)
		goto out;

	if (fe && fe->cpu_dai) {
		struct snd_soc_dai *fe_dai = fe->cpu_dai;

		asrc_set_by_dai(data, fe_dai->id, fe_dai->channels,
				params_rate(&fe_params),
				get_sif_rate(data, msg_rate),
				params_width(&fe_params),
				get_sif_width(data, msg_format));
	}
	rate_put_ipc(adev, rate, msg_rate);
	format_put_ipc(adev, format, channels, msg_format);

	hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE)->min =
			get_sif_rate(data, msg_rate);
	hw_param_interval(params, SNDRV_PCM_HW_PARAM_CHANNELS)->min =
			get_sif_channels(data, msg_format);
	snd_mask_none(hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT));
	snd_mask_set(hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT),
			get_sif_format(data, msg_format));
	dev_info(dev, "%s(%s): %s: %d bit, %u channel, %uHz\n",
			__func__, dai->name, w_tgt->name,
			get_sif_width(data, msg_format),
			get_sif_channels(data, msg_format),
			get_sif_rate(data, msg_rate));
out:
	return 0;
}

static int register_if_routes(struct device *dev,
		const struct snd_soc_dapm_route *route_base, int num,
		struct snd_soc_dapm_context *dapm, const char *name)
{
	struct snd_soc_dapm_route *route;
	int i;

	route = devm_kmemdup(dev, route_base, sizeof(*route_base) * num,
			GFP_KERNEL);
	if (!route) {
		dev_err(dev, "%s: insufficient memory\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < num; i++) {
		if (route[i].sink)
			route[i].sink = devm_kasprintf(dev, GFP_KERNEL,
					route[i].sink, name);
		if (route[i].control)
			route[i].control = devm_kasprintf(dev, GFP_KERNEL,
					route[i].control, name);
		if (route[i].source)
			route[i].source = devm_kasprintf(dev, GFP_KERNEL,
					route[i].source, name);
	}

	snd_soc_dapm_add_routes(dapm, route, num);
	devm_kfree(dev, route);

	return 0;
}

static const struct snd_soc_dapm_route route_base_if_pla[] = {
	/* sink, control, source */
	{"%s Playback", NULL, "%s PLA"},
};

static const struct snd_soc_dapm_route route_base_if_cap[] = {
	/* sink, control, source */
	{"SPUSM", "%s", "%s CAP"},
	{"%s CAP", "%s Switch", "%s Capture"},
};

int abox_cmpnt_register_if(struct platform_device *pdev_abox,
		struct platform_device *pdev_if, unsigned int id,
		struct snd_soc_dapm_context *dapm, const char *name,
		bool playback, bool capture)
{
	struct device *dev = &pdev_if->dev;
	struct abox_data *data = platform_get_drvdata(pdev_abox);
	int ret;

	if (id >= ARRAY_SIZE(data->pdev_if)) {
		dev_err(dev, "%s: invalid id(%u)\n", __func__, id);
		return -EINVAL;
	}

	if (data->cmpnt->name_prefix && dapm->component->name_prefix &&
			strcmp(data->cmpnt->name_prefix,
			dapm->component->name_prefix)) {
		dev_err(dev, "%s: name prefix is different: %s != %s\n",
				__func__, data->cmpnt->name_prefix,
				dapm->component->name_prefix);
		return -EINVAL;
	}

	data->pdev_if[id] = pdev_if;
	if (id > data->if_count)
		data->if_count = id + 1;

	if (playback) {
		ret = register_if_routes(dev, route_base_if_pla,
				ARRAY_SIZE(route_base_if_pla), dapm, name);
		if (ret < 0)
			return ret;
	}

	if (capture) {
		ret = register_if_routes(dev, route_base_if_cap,
				ARRAY_SIZE(route_base_if_cap), dapm, name);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int register_rdma_routes(struct device *dev,
		const struct snd_soc_dapm_route *route_base, int num,
		struct snd_soc_dapm_context *dapm, const char *name, int id)
{
	struct snd_soc_dapm_route *route;
	int i;

	route = kmemdup(route_base, sizeof(*route_base) * num,
			GFP_KERNEL);
	if (!route) {
		dev_err(dev, "%s: insufficient memory\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < num; i++) {
		if (route[i].sink)
			route[i].sink = devm_kasprintf(dev, GFP_KERNEL,
					route[i].sink, id);
		if (route[i].control)
			route[i].control = devm_kasprintf(dev, GFP_KERNEL,
					route[i].control, name);
		if (route[i].source)
			route[i].source = devm_kasprintf(dev, GFP_KERNEL,
					route[i].source, id);
	}

	snd_soc_dapm_add_routes(dapm, route, num);
	kfree(route);

	return 0;
}

static const struct snd_soc_dapm_route route_base_rdma[] = {
	/* sink, control, source */
	{"SPUS IN%d", "RDMA", "RDMA%d Playback"},
};

int abox_cmpnt_register_rdma(struct platform_device *pdev_abox,
		struct platform_device *pdev_rdma, unsigned int id,
		struct snd_soc_dapm_context *dapm, const char *name)
{
	struct device *dev = &pdev_rdma->dev;
	struct abox_data *data = platform_get_drvdata(pdev_abox);
	int ret;

	if (id < ARRAY_SIZE(data->pdev_rdma)) {
		data->pdev_rdma[id] = pdev_rdma;
		if (id > data->rdma_count)
			data->rdma_count = id + 1;
	} else {
		dev_err(&data->pdev->dev, "%s: invalid id(%u)\n", __func__, id);
		return -EINVAL;
	}

	ret = register_rdma_routes(dev, route_base_rdma,
			ARRAY_SIZE(route_base_rdma), dapm, name, id);
	if (ret < 0)
		return ret;

	return 0;
}

static int register_wdma_routes(struct device *dev,
		const struct snd_soc_dapm_route *route_base, int num,
		struct snd_soc_dapm_context *dapm, const char *name, int id)
{
	struct snd_soc_dapm_route *route;
	int i;

	route = kmemdup(route_base, sizeof(*route_base) * num,
			GFP_KERNEL);
	if (!route) {
		dev_err(dev, "%s: insufficient memory\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < num; i++) {
		if (route[i].sink)
			route[i].sink = devm_kasprintf(dev, GFP_KERNEL,
					route[i].sink, id);
		if (route[i].control)
			route[i].control = devm_kasprintf(dev, GFP_KERNEL,
					route[i].control, name);
		if (route[i].source)
			route[i].source = devm_kasprintf(dev, GFP_KERNEL,
					route[i].source, id);
	}

	snd_soc_dapm_add_routes(dapm, route, num);
	kfree(route);

	return 0;
}

static const struct snd_soc_dapm_route route_base_wdma[] = {
	/* sink, control, source */
	{"WDMA%d Capture", NULL, "SPUM ASRC0"},
	{"WDMA%d Capture", "WDMA", "SIFM0"},
	{"WDMA%d Capture", "WDMA", "SIFM1"},
	{"WDMA%d Capture", "WDMA", "SIFM2"},
	{"WDMA%d Capture", "WDMA", "SIFM3"},
	{"WDMA%d Capture", "WDMA", "SIFM4"},
	{"WDMA%d Capture", "WDMA", "SIFM5"},
	{"WDMA%d Capture", "WDMA", "SIFM6"},
};

int abox_cmpnt_register_wdma(struct platform_device *pdev_abox,
		struct platform_device *pdev_wdma, unsigned int id,
		struct snd_soc_dapm_context *dapm, const char *name)
{
	struct device *dev = &pdev_wdma->dev;
	struct abox_data *data = platform_get_drvdata(pdev_abox);

	if (id >= ARRAY_SIZE(data->pdev_wdma) ||
			id >= ARRAY_SIZE(route_base_wdma)) {
		dev_err(dev, "%s: invalid id(%u)\n", __func__, id);
		return -EINVAL;
	}

	data->pdev_wdma[id] = pdev_wdma;
	if (id > data->wdma_count)
		data->wdma_count = id + 1;

	return register_wdma_routes(dev, &route_base_wdma[id],
			1, dapm, name, id);
}

int abox_cmpnt_restore(struct device *adev)
{
	struct abox_data *data = dev_get_drvdata(adev);
	int i;

	dev_dbg(adev, "%s\n", __func__);

	for (i = SET_SIFS0_RATE; i <= SET_PIFS0_RATE; i++)
		rate_put_ipc(adev, data->sif_rate[sif_idx(i)], i);
	for (i = SET_SIFS0_FORMAT; i <= SET_PIFS0_FORMAT; i++)
		format_put_ipc(adev, data->sif_format[sif_idx(i)],
				data->sif_channels[sif_idx(i)], i);
	if (s_default)
		asrc_factor_put_ipc(adev, s_default, SET_ASRC_FACTOR_CP);
	if (data->audio_mode)
		audio_mode_put_ipc(adev, data->audio_mode);
	if (data->sound_type)
		sound_type_put_ipc(adev, data->sound_type);

	return 0;
}

int abox_cmpnt_register(struct device *adev)
{
	const struct snd_soc_component_driver *cmpnt_drv;
	struct snd_soc_dai_driver *dai_drv;
	int num_dai;

	cmpnt_drv = &abox_cmpnt_drv;
	dai_drv = abox_cmpnt_dai_drv;
	num_dai = ARRAY_SIZE(abox_cmpnt_dai_drv);

	return snd_soc_register_component(adev, cmpnt_drv, dai_drv, num_dai);
}
