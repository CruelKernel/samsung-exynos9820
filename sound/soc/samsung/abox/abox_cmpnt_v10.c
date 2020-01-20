/* sound/soc/samsung/abox/abox_cmpnt_10.c
 *
 * ALSA SoC Audio Layer - Samsung Abox Component driver for 1.x.x
 *
 * Copyright (c) 2018 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "abox.h"
#include "abox_if.h"
#include "abox_cmpnt.h"

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

static void __maybe_unused set_sif_channels_min(struct abox_data *data,
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

static int get_sif_physical_width(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	snd_pcm_format_t format = get_sif_format(data, configmsg);

	return snd_pcm_format_physical_width(format);
}

static int get_sif_width(struct abox_data *data, enum ABOX_CONFIGMSG configmsg)
{
	return snd_pcm_format_width(get_sif_format(data, configmsg));
}

static void set_sif_width(struct abox_data *data,
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
	if (ret < 0) {
		dev_err(adev, "%s(%u, %#x) failed: %d\n", __func__, val,
				configmsg, ret);
	}

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
	abox_config_msg->param1 = get_sif_width(data, configmsg);
	abox_config_msg->param2 = get_sif_channels(data, configmsg);
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

static int erap_handler_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
			(struct soc_mixer_control *)kcontrol->private_value;
	enum ABOX_ERAP_TYPE type = (enum ABOX_ERAP_TYPE)mc->reg;

	dev_dbg(dev, "%s(%d)\n", __func__, type);

	ucontrol->value.integer.value[0] = data->erap_status[type];

	return 0;
}

static int erap_handler_put_ipc(struct device *dev,
		enum ABOX_ERAP_TYPE type, unsigned int val)
{
	struct abox_data *data = dev_get_drvdata(dev);
	ABOX_IPC_MSG msg;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;
	struct ERAP_ONOFF_PARAM *erap_param = &erap_msg->param.onoff;
	int ret;

	dev_dbg(dev, "%s(%u, %d)\n", __func__, val, type);

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = val ? REALTIME_OPEN : REALTIME_CLOSE;
	erap_param->type = type;
	erap_param->channel_no = 0;
	erap_param->version = val;
	ret = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
	if (ret < 0)
		dev_err(dev, "erap control failed(type:%d, status:%d)\n",
				type, val);

	data->erap_status[type] = val;

	return ret;
}

static int erap_handler_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
			(struct soc_mixer_control *)kcontrol->private_value;
	enum ABOX_ERAP_TYPE type = (enum ABOX_ERAP_TYPE)mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(%u, %d)\n", __func__, val, type);

	return erap_handler_put_ipc(dev, type, val);
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

		switch (mode) {
		case MODE_IN_COMMUNICATION:
			abox_request_big_freq(dev, data,
					(void *)DEFAULT_BIG_FREQ_ID, BIG_FREQ);
			abox_request_lit_freq(dev, data,
					(void *)DEFAULT_LIT_FREQ_ID, LIT_FREQ);
			break;
		default:
			abox_request_big_freq(dev, data,
					(void *)DEFAULT_BIG_FREQ_ID, 0);
			abox_request_lit_freq(dev, data,
					(void *)DEFAULT_LIT_FREQ_ID, 0);
			break;
		}
	}

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

static const struct snd_kcontrol_new cmpnt_controls[] = {
	SOC_SINGLE_EXT("SIFS0 Rate", SET_SIFS0_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS1 Rate", SET_SIFS1_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS2 Rate", SET_SIFS2_RATE, 8000, 384000, 0,
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
	SOC_SINGLE_EXT("SIFS0 Width", SET_SIFS0_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS1 Width", SET_SIFS1_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS2 Width", SET_SIFS2_FORMAT, 16, 32, 0,
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
	SOC_SINGLE_EXT("SIFS0 Rate Min", SET_SIFS0_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFS1 Rate Min", SET_SIFS1_RATE, 8000, 384000, 0,
			rate_min_get, rate_min_put),
	SOC_SINGLE_EXT("SIFS2 Rate Min", SET_SIFS2_RATE, 8000, 384000, 0,
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
	SOC_SINGLE_EXT("SIFS0 Width Min", SET_SIFS0_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFS1 Width Min", SET_SIFS1_FORMAT, 16, 32, 0,
			width_min_get, width_min_put),
	SOC_SINGLE_EXT("SIFS2 Width Min", SET_SIFS2_FORMAT, 16, 32, 0,
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
	SOC_SINGLE_EXT("SIFS0 Auto Config", SET_SIFS0_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFS1 Auto Config", SET_SIFS1_RATE, 0, 1, 0,
			auto_config_get, auto_config_put),
	SOC_SINGLE_EXT("SIFS2 Auto Config", SET_SIFS2_RATE, 0, 1, 0,
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
	SOC_SINGLE_EXT("Echo Cancellation", ERAP_ECHO_CANCEL, 0, 2, 0,
			erap_handler_get, erap_handler_put),
	SOC_SINGLE_EXT("VI Sensing", ERAP_VI_SENSE, 0, 2, 0,
			erap_handler_get, erap_handler_put),
	SOC_VALUE_ENUM_EXT("Audio Mode", audio_mode_enum,
			audio_mode_get, audio_mode_put),
	SOC_VALUE_ENUM_EXT("Sound Type", sound_type_enum,
			sound_type_get, sound_type_put),
	SOC_SINGLE_EXT("Tickle", 0, 0, 1, 0, tickle_get, tickle_put),
};

static const struct snd_kcontrol_new spus_asrc_controls[] = {
	SOC_SINGLE("SPUS ASRC0", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(0), 1, 0),
	SOC_SINGLE("SPUS ASRC1", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(1), 1, 0),
	SOC_SINGLE("SPUS ASRC2", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(2), 1, 0),
	SOC_SINGLE("SPUS ASRC3", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(3), 1, 0),
	SOC_SINGLE("SPUS ASRC4", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(4), 1, 0),
	SOC_SINGLE("SPUS ASRC5", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(5), 1, 0),
	SOC_SINGLE("SPUS ASRC6", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(6), 1, 0),
	SOC_SINGLE("SPUS ASRC7", ABOX_SPUS_CTRL0,
			ABOX_FUNC_CHAIN_SRC_ASRC_L(7), 1, 0),
};

static const struct snd_kcontrol_new spum_asrc_controls[] = {
	SOC_SINGLE("SPUM ASRC0", ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_RSRC_ASRC_L, 1, 0),
	SOC_SINGLE("SPUM ASRC1", ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(0), 1, 0),
	SOC_SINGLE("SPUM ASRC2", ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(1), 1, 0),
	SOC_SINGLE("SPUM ASRC3", ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(2), 1, 0),
};

struct name_rate_format {
	const char *name;
	int stream;
	const enum ABOX_CONFIGMSG rate;
	const enum ABOX_CONFIGMSG format;
	bool slave;
};

static const struct name_rate_format nrf_list[] = {
	{"SIFS0", SNDRV_PCM_STREAM_PLAYBACK, SET_SIFS0_RATE, SET_SIFS0_FORMAT,
			false},
	{"SIFS1", SNDRV_PCM_STREAM_PLAYBACK, SET_SIFS1_RATE, SET_SIFS1_FORMAT,
			false},
	{"SIFS2", SNDRV_PCM_STREAM_PLAYBACK, SET_SIFS2_RATE, SET_SIFS2_FORMAT,
			false},
	{"RECP", SNDRV_PCM_STREAM_CAPTURE, SET_PIFS1_RATE, SET_PIFS1_FORMAT,
			true},
	{"NSRC0", SNDRV_PCM_STREAM_CAPTURE, SET_SIFM0_RATE, SET_SIFM0_FORMAT,
			false},
	{"NSRC1", SNDRV_PCM_STREAM_CAPTURE, SET_SIFM1_RATE, SET_SIFM1_FORMAT,
			false},
	{"NSRC2", SNDRV_PCM_STREAM_CAPTURE, SET_SIFM2_RATE, SET_SIFM2_FORMAT,
			false},
	{"NSRC3", SNDRV_PCM_STREAM_CAPTURE, SET_SIFM3_RATE, SET_SIFM3_FORMAT,
			false},
};

static bool find_nrf_stream(const struct snd_soc_dapm_widget *w,
		int stream, enum ABOX_CONFIGMSG *rate,
		enum ABOX_CONFIGMSG *format, bool *slave)
{
	struct snd_soc_component *cmpnt = w->dapm->component;
	const char *name_prefix = cmpnt ? cmpnt->name_prefix : NULL;
	size_t prefix_len = name_prefix ? strlen(name_prefix) + 1 : 0;
	const char *name = w->name + prefix_len;
	const struct name_rate_format *nrf;

	for (nrf = nrf_list; nrf - nrf_list < ARRAY_SIZE(nrf_list); nrf++) {
		if ((nrf->stream == stream) && (strcmp(nrf->name, name) == 0)) {
			*rate = nrf->rate;
			*format = nrf->format;
			*slave = nrf->slave;
			return true;
		}
	}

	return false;
}

static bool find_nrf(const struct snd_soc_dapm_widget *w,
		enum ABOX_CONFIGMSG *rate, enum ABOX_CONFIGMSG *format,
		int *stream, bool *slave)
{
	struct snd_soc_component *cmpnt = w->dapm->component;
	const char *name_prefix = cmpnt ? cmpnt->name_prefix : NULL;
	size_t prefix_len = name_prefix ? strlen(name_prefix) + 1 : 0;
	const char *name = w->name + prefix_len;
	const struct name_rate_format *nrf;

	for (nrf = nrf_list; nrf - nrf_list < ARRAY_SIZE(nrf_list); nrf++) {
		if (strcmp(nrf->name, name))
			continue;

		*rate = nrf->rate;
		*format = nrf->format;
		*stream = nrf->stream;
		*slave = nrf->slave;
		return true;
	}

	return false;
}

static int find_config(struct snd_soc_dapm_widget *w_src,
		struct abox_data *data, int stream,
		unsigned int *rate, unsigned int *width)
{
	struct snd_soc_component *cmpnt = data->cmpnt;
	struct device *dev = &data->pdev->dev;
	struct snd_soc_dapm_widget *w;
	enum ABOX_CONFIGMSG msg_rate, msg_format;
	LIST_HEAD(widget_list);

	/*
	 * For snd_soc_dapm_connected_{output,input}_ep fully discover the graph
	 * we need to reset the cached number of inputs and outputs.
	 */
	list_for_each_entry(w, &cmpnt->card->widgets, list) {
		w->endpoints[SND_SOC_DAPM_DIR_IN] = -1;
		w->endpoints[SND_SOC_DAPM_DIR_OUT] = -1;
	}

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dapm_connected_output_ep(w_src, &widget_list);
	else
		snd_soc_dapm_connected_input_ep(w_src, &widget_list);

	list_for_each_entry(w, &widget_list, work_list) {
		dev_dbg(dev, "%s", w->name);

		if (!find_nrf_stream(w, stream, &msg_rate, &msg_format, NULL))
			continue;

		*rate = get_sif_rate(data, msg_rate);
		*width = get_sif_width(data, msg_format);
		dev_dbg(dev, "%s: rate=%u, width=%u\n", w->name, *rate, *width);
		return 0;
	}

	return -EINVAL;
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

static int asrc_put_active(struct snd_soc_dapm_widget *w, int stream, int on)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_component *cmpnt = dapm->component;
	const struct snd_kcontrol_new *kcontrol;
	struct soc_mixer_control *mc;
	unsigned int reg, mask, val;

	kcontrol = asrc_get_kcontrol(asrc_get_idx(w), stream);
	if (IS_ERR(kcontrol))
		return PTR_ERR(kcontrol);

	mc = (struct soc_mixer_control *)kcontrol->private_value;
	reg = mc->reg;
	mask = 1 << mc->shift;
	val = !!on << mc->shift;
	return snd_soc_component_update_bits(cmpnt, reg, mask, val);
}

static int asrc_event(struct snd_soc_dapm_widget *w, int e,
		int stream, struct platform_device **pdev_dma)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct device *dev = dapm->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	int idx = asrc_get_idx(w);
	struct snd_soc_pcm_runtime *rtd;
	struct snd_pcm_hw_params *params;
	unsigned int tgt_rate = 0, tgt_width = 0;
	unsigned int rate = 0, width = 0, channels = 0;
	int on, ret = 0;

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

		ret = find_config(w, data, stream, &tgt_rate, &tgt_width);
		if (ret < 0) {
			dev_warn(dev, "%s: incomplete path: %s\n",
				__func__, w->name);
			break;
		}

		on = (rate != tgt_rate) || (width != tgt_width);
		dev_info(dev, "turn %s %s\n", w->name, on ? "on" : "off");
		ret = asrc_put_active(w, stream, on);
		if (ret < 0)
			dev_err(dev, "%s: set failed: %d\n", __func__, ret);
		break;
	case SND_SOC_DAPM_POST_PMD:
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
static SOC_ENUM_SINGLE_DECL(spus_in6_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_IN_L(6), spus_inx_texts);
static const struct snd_kcontrol_new spus_in6_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in6_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in7_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_IN_L(7), spus_inx_texts);
static const struct snd_kcontrol_new spus_in7_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in7_enum),
};

static const char * const spus_outx_texts[] = {"SIFS1", "SIFS0", "SIFS2"};
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
static SOC_ENUM_SINGLE_DECL(spus_out6_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_OUT_L(6), spus_outx_texts);
static const struct snd_kcontrol_new spus_out6_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out6_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out7_enum, ABOX_SPUS_CTRL0,
		ABOX_FUNC_CHAIN_SRC_OUT_L(7), spus_outx_texts);
static const struct snd_kcontrol_new spus_out7_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out7_enum),
};

static const char * const spusm_texts[] = {
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"UAIF0", "UAIF1", "UAIF2", "UAIF3", "UAIF4",
};
static SOC_ENUM_SINGLE_DECL(spusm_enum, ABOX_ROUTE_CTRL1, ABOX_ROUTE_SPUSM_L,
		spusm_texts);
static const struct snd_kcontrol_new spusm_controls[] = {
	SOC_DAPM_ENUM("MUX", spusm_enum),
};

static int abox_flush_mixp(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct device *dev = cmpnt->dev;

	dev_dbg(dev, "%s\n", __func__);

	dev_info(dev, "%s: flush\n", __func__);
	snd_soc_component_update_bits(cmpnt, ABOX_SPUS_CTRL2,
			ABOX_MIXP_FLUSH_MASK,
			1 << ABOX_MIXP_FLUSH_L);

	return 0;
}

static int abox_flush_sifm(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct device *dev = cmpnt->dev;

	dev_dbg(dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_input_ep(w, NULL)) {
		dev_info(dev, "%s: flush\n", __func__);
		snd_soc_component_update_bits(cmpnt, ABOX_SPUS_CTRL3,
				ABOX_SIFSM_FLUSH_MASK,
				1 << ABOX_SIFSM_FLUSH_L);
	}

	return 0;
}

static int abox_flush_sifs1(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct device *dev = cmpnt->dev;

	dev_dbg(dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_input_ep(w, NULL)) {
		dev_info(dev, "%s: flush\n", __func__);
		snd_soc_component_update_bits(cmpnt, ABOX_SPUS_CTRL3,
				ABOX_SIFS_FLUSH_MASK(1),
				1 << ABOX_SIFS_FLUSH_L(1));
	}

	return 0;
}

static int abox_flush_sifs2(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct device *dev = cmpnt->dev;

	dev_dbg(dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_input_ep(w, NULL)) {
		dev_info(dev, "%s: flush\n", __func__);
		snd_soc_component_update_bits(cmpnt, ABOX_SPUS_CTRL3,
				ABOX_SIFS_FLUSH_MASK(2),
				1 << ABOX_SIFS_FLUSH_L(2));
	}

	return 0;
}

static int abox_flush_recp(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct device *dev = cmpnt->dev;

	dev_dbg(dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_output_ep(w, NULL)) {
		dev_info(dev, "%s: flush\n", __func__);
		snd_soc_component_update_bits(cmpnt, ABOX_SPUM_CTRL2,
				ABOX_RECP_FLUSH_MASK,
				1 << ABOX_RECP_FLUSH_L);
	}

	return 0;
}

static int abox_flush_sifm0(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct device *dev = cmpnt->dev;

	dev_dbg(dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_output_ep(w, NULL)) {
		dev_info(dev, "%s: flush\n", __func__);
		snd_soc_component_update_bits(cmpnt, ABOX_SPUM_CTRL3,
				ABOX_SIFM_FLUSH_MASK(0),
				1 << ABOX_SIFM_FLUSH_L(0));
	}

	return 0;
}

static int abox_flush_sifm1(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct device *dev = cmpnt->dev;

	dev_dbg(dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_output_ep(w, NULL)) {
		dev_info(dev, "%s: flush\n", __func__);
		snd_soc_component_update_bits(cmpnt, ABOX_SPUM_CTRL3,
				ABOX_SIFM_FLUSH_MASK(1),
				1 << ABOX_SIFM_FLUSH_L(1));
	}

	return 0;
}

static int abox_flush_sifm2(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct device *dev = cmpnt->dev;

	dev_dbg(dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_output_ep(w, NULL)) {
		dev_info(dev, "%s: flush\n", __func__);
		snd_soc_component_update_bits(cmpnt, ABOX_SPUM_CTRL3,
				ABOX_SIFM_FLUSH_MASK(2),
				1 << ABOX_SIFM_FLUSH_L(2));
	}

	return 0;
}

static int abox_flush_sifm3(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct device *dev = cmpnt->dev;

	dev_dbg(dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_output_ep(w, NULL)) {
		dev_info(dev, "%s: flush\n", __func__);
		snd_soc_component_update_bits(cmpnt, ABOX_SPUM_CTRL3,
				ABOX_SIFM_FLUSH_MASK(3),
				1 << ABOX_SIFM_FLUSH_L(3));
	}

	return 0;
}

static const char * const sifsx_texts[] = {
	"SPUS OUT0", "SPUS OUT1", "SPUS OUT2", "SPUS OUT3",
	"SPUS OUT4", "SPUS OUT5", "SPUS OUT6", "SPUS OUT7",
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

static const char * const sifsm_texts[] = {
	"SPUS IN0", "SPUS IN1", "SPUS IN2", "SPUS IN3",
	"SPUS IN4", "SPUS IN5", "SPUS IN6", "SPUS IN7",
};
static SOC_ENUM_SINGLE_DECL(sifsm_enum, ABOX_SPUS_CTRL1, ABOX_SIFSM_IN_SEL_L,
		sifsm_texts);
static const struct snd_kcontrol_new sifsm_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifsm_enum),
};

static const char * const uaif_spkx_texts[] = {
	"RESERVED", "SIFS0", "SIFS1", "SIFS2",
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
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
	"RESERVED", "RESERVED", "SIFS1", "SIFS2",
};
static SOC_ENUM_SINGLE_DECL(dsif_spk_enum, ABOX_ROUTE_CTRL0, ABOX_ROUTE_DSIF_L,
		dsif_spk_texts);
static const struct snd_kcontrol_new dsif_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", dsif_spk_enum),
};

static const char * const rsrcx_texts[] = {
	"RESERVED", "SIFS0", "SIFS1", "SIFS2",
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"NSRC0", "NSRC1", "NSRC2", "NSRC3",
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
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"UAIF0", "UAIF1", "UAIF2", "UAIF3", "UAIF4",
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

static const char * const sifms_texts[] = {
	"RESERVED", "SIFM0", "SIFM1", "SIFM2", "SIFM3",
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
	SND_SOC_DAPM_DEMUX_E("SIFSM", SND_SOC_NOPM, 0, 0, sifsm_controls,
			abox_flush_sifm,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX("SPUS IN0", SND_SOC_NOPM, 0, 0, spus_in0_controls),
	SND_SOC_DAPM_MUX("SPUS IN1", SND_SOC_NOPM, 0, 0, spus_in1_controls),
	SND_SOC_DAPM_MUX("SPUS IN2", SND_SOC_NOPM, 0, 0, spus_in2_controls),
	SND_SOC_DAPM_MUX("SPUS IN3", SND_SOC_NOPM, 0, 0, spus_in3_controls),
	SND_SOC_DAPM_MUX("SPUS IN4", SND_SOC_NOPM, 0, 0, spus_in4_controls),
	SND_SOC_DAPM_MUX("SPUS IN5", SND_SOC_NOPM, 0, 0, spus_in5_controls),
	SND_SOC_DAPM_MUX("SPUS IN6", SND_SOC_NOPM, 0, 0, spus_in6_controls),
	SND_SOC_DAPM_MUX("SPUS IN7", SND_SOC_NOPM, 0, 0, spus_in7_controls),
	SND_SOC_DAPM_PGA_E("SPUS ASRC0", SND_SOC_NOPM, 0, 0, NULL, 0,
			spus_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("SPUS ASRC1", SND_SOC_NOPM, 1, 0, NULL, 0,
			spus_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("SPUS ASRC2", SND_SOC_NOPM, 2, 0, NULL, 0,
			spus_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("SPUS ASRC3", SND_SOC_NOPM, 3, 0, NULL, 0,
			spus_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("SPUS ASRC4", SND_SOC_NOPM, 4, 0, NULL, 0,
			spus_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("SPUS ASRC5", SND_SOC_NOPM, 5, 0, NULL, 0,
			spus_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("SPUS ASRC6", SND_SOC_NOPM, 6, 0, NULL, 0,
			spus_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("SPUS ASRC7", SND_SOC_NOPM, 7, 0, NULL, 0,
			spus_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_DEMUX("SPUS OUT0", SND_SOC_NOPM, 0, 0, spus_out0_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT1", SND_SOC_NOPM, 0, 0, spus_out1_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT2", SND_SOC_NOPM, 0, 0, spus_out2_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT3", SND_SOC_NOPM, 0, 0, spus_out3_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT4", SND_SOC_NOPM, 0, 0, spus_out4_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT5", SND_SOC_NOPM, 0, 0, spus_out5_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT6", SND_SOC_NOPM, 0, 0, spus_out6_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT7", SND_SOC_NOPM, 0, 0, spus_out7_controls),

	SND_SOC_DAPM_PGA("SPUS OUT0-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT1-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT2-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT3-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT4-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT5-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT6-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT7-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT0-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT1-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT2-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT3-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT4-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT5-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT6-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT7-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT0-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT1-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT2-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT3-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT4-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT5-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT6-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT7-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER_E("SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0,
			abox_flush_mixp,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("SIFS1", SND_SOC_NOPM, 0, 0, sifs1_controls,
			abox_flush_sifs1,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("SIFS2", SND_SOC_NOPM, 0, 0, sifs2_controls,
			abox_flush_sifs2,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX("UAIF0 SPK", SND_SOC_NOPM, 0, 0, uaif0_spk_controls),
	SND_SOC_DAPM_MUX("UAIF1 SPK", SND_SOC_NOPM, 0, 0, uaif1_spk_controls),
	SND_SOC_DAPM_MUX("UAIF2 SPK", SND_SOC_NOPM, 0, 0, uaif2_spk_controls),
	SND_SOC_DAPM_MUX("UAIF3 SPK", SND_SOC_NOPM, 0, 0, uaif3_spk_controls),
	SND_SOC_DAPM_MUX("UAIF4 SPK", SND_SOC_NOPM, 0, 0, uaif4_spk_controls),
	SND_SOC_DAPM_MUX("DSIF SPK", SND_SOC_NOPM, 0, 0, dsif_spk_controls),

	SND_SOC_DAPM_MUX("RSRC0", SND_SOC_NOPM, 0, 0, rsrc0_controls),
	SND_SOC_DAPM_MUX("RSRC1", SND_SOC_NOPM, 0, 0, rsrc1_controls),
	SND_SOC_DAPM_MUX("NSRC0", SND_SOC_NOPM, 0, 0, nsrc0_controls),
	SND_SOC_DAPM_MUX("NSRC1", SND_SOC_NOPM, 0, 0, nsrc1_controls),
	SND_SOC_DAPM_MUX("NSRC2", SND_SOC_NOPM, 0, 0, nsrc2_controls),
	SND_SOC_DAPM_MUX("NSRC3", SND_SOC_NOPM, 0, 0, nsrc3_controls),

	SOC_MIXER_E_ARRAY("RECP", SND_SOC_NOPM, 0, 0, recp_controls,
			abox_flush_recp,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_PGA_E("SPUM ASRC0", SND_SOC_NOPM, 0, 0, NULL, 0,
			spum_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("SPUM ASRC1", SND_SOC_NOPM, 1, 0, NULL, 0,
			spum_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("SPUM ASRC2", SND_SOC_NOPM, 2, 0, NULL, 0,
			spum_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("SPUM ASRC3", SND_SOC_NOPM, 3, 0, NULL, 0,
			spum_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("SPUM ASRC4", SND_SOC_NOPM, 4, 0, NULL, 0,
			spum_asrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_DEMUX_E("SIFM0", SND_SOC_NOPM, 0, 0, sifm0_controls,
			abox_flush_sifm0,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_DEMUX_E("SIFM1", SND_SOC_NOPM, 0, 0, sifm1_controls,
			abox_flush_sifm1,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_DEMUX_E("SIFM2", SND_SOC_NOPM, 0, 0, sifm2_controls,
			abox_flush_sifm2,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_DEMUX_E("SIFM3", SND_SOC_NOPM, 0, 0, sifm3_controls,
			abox_flush_sifm3,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_PGA("SIFM0-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFM1-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFM2-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFM3-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
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

	{"SPUS IN0", "SIFSM", "SIFSM-SPUS IN0"},
	{"SPUS IN1", "SIFSM", "SIFSM-SPUS IN1"},
	{"SPUS IN2", "SIFSM", "SIFSM-SPUS IN2"},
	{"SPUS IN3", "SIFSM", "SIFSM-SPUS IN3"},
	{"SPUS IN4", "SIFSM", "SIFSM-SPUS IN4"},
	{"SPUS IN5", "SIFSM", "SIFSM-SPUS IN5"},
	{"SPUS IN6", "SIFSM", "SIFSM-SPUS IN6"},
	{"SPUS IN7", "SIFSM", "SIFSM-SPUS IN7"},

	{"SPUS ASRC0", NULL, "SPUS IN0"},
	{"SPUS ASRC1", NULL, "SPUS IN1"},
	{"SPUS ASRC2", NULL, "SPUS IN2"},
	{"SPUS ASRC3", NULL, "SPUS IN3"},
	{"SPUS ASRC4", NULL, "SPUS IN4"},
	{"SPUS ASRC5", NULL, "SPUS IN5"},
	{"SPUS ASRC6", NULL, "SPUS IN6"},
	{"SPUS ASRC7", NULL, "SPUS IN7"},

	{"SPUS OUT0", NULL, "SPUS ASRC0"},
	{"SPUS OUT1", NULL, "SPUS ASRC1"},
	{"SPUS OUT2", NULL, "SPUS ASRC2"},
	{"SPUS OUT3", NULL, "SPUS ASRC3"},
	{"SPUS OUT4", NULL, "SPUS ASRC4"},
	{"SPUS OUT5", NULL, "SPUS ASRC5"},
	{"SPUS OUT6", NULL, "SPUS ASRC6"},
	{"SPUS OUT7", NULL, "SPUS ASRC7"},

	{"SPUS OUT0-SIFS0", "SIFS0", "SPUS OUT0"},
	{"SPUS OUT1-SIFS0", "SIFS0", "SPUS OUT1"},
	{"SPUS OUT2-SIFS0", "SIFS0", "SPUS OUT2"},
	{"SPUS OUT3-SIFS0", "SIFS0", "SPUS OUT3"},
	{"SPUS OUT4-SIFS0", "SIFS0", "SPUS OUT4"},
	{"SPUS OUT5-SIFS0", "SIFS0", "SPUS OUT5"},
	{"SPUS OUT6-SIFS0", "SIFS0", "SPUS OUT6"},
	{"SPUS OUT7-SIFS0", "SIFS0", "SPUS OUT7"},
	{"SPUS OUT0-SIFS1", "SIFS1", "SPUS OUT0"},
	{"SPUS OUT1-SIFS1", "SIFS1", "SPUS OUT1"},
	{"SPUS OUT2-SIFS1", "SIFS1", "SPUS OUT2"},
	{"SPUS OUT3-SIFS1", "SIFS1", "SPUS OUT3"},
	{"SPUS OUT4-SIFS1", "SIFS1", "SPUS OUT4"},
	{"SPUS OUT5-SIFS1", "SIFS1", "SPUS OUT5"},
	{"SPUS OUT6-SIFS1", "SIFS1", "SPUS OUT6"},
	{"SPUS OUT7-SIFS1", "SIFS1", "SPUS OUT7"},
	{"SPUS OUT0-SIFS2", "SIFS2", "SPUS OUT0"},
	{"SPUS OUT1-SIFS2", "SIFS2", "SPUS OUT1"},
	{"SPUS OUT2-SIFS2", "SIFS2", "SPUS OUT2"},
	{"SPUS OUT3-SIFS2", "SIFS2", "SPUS OUT3"},
	{"SPUS OUT4-SIFS2", "SIFS2", "SPUS OUT4"},
	{"SPUS OUT5-SIFS2", "SIFS2", "SPUS OUT5"},
	{"SPUS OUT6-SIFS2", "SIFS2", "SPUS OUT6"},
	{"SPUS OUT7-SIFS2", "SIFS2", "SPUS OUT7"},

	{"SIFS0", NULL, "SPUS OUT0-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT1-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT2-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT3-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT4-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT5-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT6-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT7-SIFS0"},
	{"SIFS1", "SPUS OUT0", "SPUS OUT0-SIFS1"},
	{"SIFS1", "SPUS OUT1", "SPUS OUT1-SIFS1"},
	{"SIFS1", "SPUS OUT2", "SPUS OUT2-SIFS1"},
	{"SIFS1", "SPUS OUT3", "SPUS OUT3-SIFS1"},
	{"SIFS1", "SPUS OUT4", "SPUS OUT4-SIFS1"},
	{"SIFS1", "SPUS OUT5", "SPUS OUT5-SIFS1"},
	{"SIFS1", "SPUS OUT6", "SPUS OUT6-SIFS1"},
	{"SIFS1", "SPUS OUT7", "SPUS OUT7-SIFS1"},
	{"SIFS2", "SPUS OUT0", "SPUS OUT0-SIFS2"},
	{"SIFS2", "SPUS OUT1", "SPUS OUT1-SIFS2"},
	{"SIFS2", "SPUS OUT2", "SPUS OUT2-SIFS2"},
	{"SIFS2", "SPUS OUT3", "SPUS OUT3-SIFS2"},
	{"SIFS2", "SPUS OUT4", "SPUS OUT4-SIFS2"},
	{"SIFS2", "SPUS OUT5", "SPUS OUT5-SIFS2"},
	{"SIFS2", "SPUS OUT6", "SPUS OUT6-SIFS2"},
	{"SIFS2", "SPUS OUT7", "SPUS OUT7-SIFS2"},

	{"SIFS0 Playback", NULL, "SIFS0"},
	{"SIFS1 Playback", NULL, "SIFS1"},
	{"SIFS2 Playback", NULL, "SIFS2"},

	{"UAIF0 SPK", "SIFS0", "SIFS0"},
	{"UAIF0 SPK", "SIFS1", "SIFS1"},
	{"UAIF0 SPK", "SIFS2", "SIFS2"},
	{"UAIF0 SPK", "SIFMS", "SIFMS"},
	{"UAIF1 SPK", "SIFS0", "SIFS0"},
	{"UAIF1 SPK", "SIFS1", "SIFS1"},
	{"UAIF1 SPK", "SIFS2", "SIFS2"},
	{"UAIF1 SPK", "SIFMS", "SIFMS"},
	{"UAIF2 SPK", "SIFS0", "SIFS0"},
	{"UAIF2 SPK", "SIFS1", "SIFS1"},
	{"UAIF2 SPK", "SIFS2", "SIFS2"},
	{"UAIF2 SPK", "SIFMS", "SIFMS"},
	{"UAIF3 SPK", "SIFS0", "SIFS0"},
	{"UAIF3 SPK", "SIFS1", "SIFS1"},
	{"UAIF3 SPK", "SIFS2", "SIFS2"},
	{"UAIF3 SPK", "SIFMS", "SIFMS"},
	{"UAIF4 SPK", "SIFS0", "SIFS0"},
	{"UAIF4 SPK", "SIFS1", "SIFS1"},
	{"UAIF4 SPK", "SIFS2", "SIFS2"},
	{"UAIF4 SPK", "SIFMS", "SIFMS"},
	{"DSIF SPK", "SIFS1", "SIFS1"},
	{"DSIF SPK", "SIFS2", "SIFS2"},

	{"RSRC0", "NSRC0", "NSRC0"},
	{"RSRC0", "NSRC1", "NSRC1"},
	{"RSRC0", "NSRC2", "NSRC2"},
	{"RSRC0", "NSRC3", "NSRC3"},
	{"RSRC1", "NSRC0", "NSRC0"},
	{"RSRC1", "NSRC1", "NSRC1"},
	{"RSRC1", "NSRC2", "NSRC2"},
	{"RSRC1", "NSRC3", "NSRC3"},

	{"RSRC0", "SIFS0", "SIFS0 Capture"},
	{"RSRC0", "SIFS1", "SIFS1 Capture"},
	{"RSRC0", "SIFS2", "SIFS2 Capture"},
	{"RSRC1", "SIFS0", "SIFS0 Capture"},
	{"RSRC1", "SIFS1", "SIFS1 Capture"},
	{"RSRC1", "SIFS2", "SIFS2 Capture"},

	{"NSRC0", "SIFS0", "SIFS0 Capture"},
	{"NSRC0", "SIFS1", "SIFS1 Capture"},
	{"NSRC0", "SIFS2", "SIFS2 Capture"},
	{"NSRC1", "SIFS0", "SIFS0 Capture"},
	{"NSRC1", "SIFS1", "SIFS1 Capture"},
	{"NSRC1", "SIFS2", "SIFS2 Capture"},
	{"NSRC2", "SIFS0", "SIFS0 Capture"},
	{"NSRC2", "SIFS1", "SIFS1 Capture"},
	{"NSRC2", "SIFS2", "SIFS2 Capture"},
	{"NSRC3", "SIFS0", "SIFS0 Capture"},
	{"NSRC3", "SIFS1", "SIFS1 Capture"},
	{"NSRC3", "SIFS2", "SIFS2 Capture"},

	{"RECP", "PIFS0", "RSRC0"},
	{"RECP", "PIFS1", "RSRC1"},

	{"SPUM ASRC0", NULL, "RECP"},
	{"SPUM ASRC1", NULL, "NSRC0"},
	{"SPUM ASRC2", NULL, "NSRC1"},
	{"SPUM ASRC3", NULL, "NSRC2"},

	{"SIFM0", NULL, "SPUM ASRC1"},
	{"SIFM1", NULL, "SPUM ASRC2"},
	{"SIFM2", NULL, "SPUM ASRC3"},
	{"SIFM3", NULL, "NSRC3"},

	{"SIFM0-SIFMS", "SIFMS", "SIFM0"},
	{"SIFM1-SIFMS", "SIFMS", "SIFM1"},
	{"SIFM2-SIFMS", "SIFMS", "SIFM2"},
	{"SIFM3-SIFMS", "SIFMS", "SIFM3"},

	{"SIFMS", "SIFM0", "SIFM0-SIFMS"},
	{"SIFMS", "SIFM1", "SIFM1-SIFMS"},
	{"SIFMS", "SIFM2", "SIFM2-SIFMS"},
	{"SIFMS", "SIFM3", "SIFM3-SIFMS"},
};

static int cmpnt_probe(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	snd_soc_add_component_controls(cmpnt, spus_asrc_controls,
			ARRAY_SIZE(spus_asrc_controls));
	snd_soc_add_component_controls(cmpnt, spum_asrc_controls,
			ARRAY_SIZE(spum_asrc_controls));

	data->cmpnt = cmpnt;
	snd_soc_component_update_bits(cmpnt, ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_RSRC_RECP_MASK,
			ABOX_FUNC_CHAIN_RSRC_RECP_MASK);

	return 0;
}

static void cmpnt_remove(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;

	dev_dbg(dev, "%s\n", __func__);
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

static unsigned int sifsx_cnt_val(unsigned long aclk, unsigned int rate,
		unsigned int physical_width, unsigned int channels)
{
	static const int correction = -2;
	unsigned int n, d;

	/* k = n / d */
	d = channels;
	n = 2 * (32 / physical_width);

	return ((aclk * n) / d / rate) - 5 + correction;
}

static int sifs_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *be = substream->private_data;
	struct snd_soc_dpcm *dpcm;
	int stream = substream->stream;
	struct device *dev = dai->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct regmap *regmap = data->regmap;
	enum abox_dai id = dai->id;
	unsigned int rate = params_rate(params);
	unsigned int width = params_width(params);
	unsigned int pwidth = params_physical_width(params);
	unsigned int channels = params_channels(params);
	unsigned long aclk;
	unsigned int cnt_val;
	bool skip = true;
	int ret = 0;

	if (stream != SNDRV_PCM_STREAM_CAPTURE)
		goto out;

	/* sifs count is needed only when SIFS is connected to NSRC */
	list_for_each_entry(dpcm, &be->dpcm[stream].fe_clients, list_fe) {
		if (dpcm->fe->cpu_dai->id != ABOX_WDMA0) {
			skip = false;
			break;
		}
	}
	if (skip)
		goto out;

	abox_request_cpu_gear_sync(dev, data, substream, ABOX_CPU_GEAR_MAX);
	aclk = clk_get_rate(data->clk_bus);
	cnt_val = sifsx_cnt_val(aclk, rate, pwidth, channels);

	dev_info(dev, "%s[%d](%ubit %uchannel %uHz at %luHz): %u\n",
			__func__, id, width, channels, rate, aclk, cnt_val);

	switch (id) {
	case ABOX_SIFS0:
		ret = regmap_update_bits(regmap, ABOX_SPUS_CTRL_SIFS_CNT(0),
			ABOX_SIFS_CNT_VAL_MASK(0),
			cnt_val << ABOX_SIFS_CNT_VAL_L(0));
		break;
	case ABOX_SIFS1:
		ret = regmap_update_bits(regmap, ABOX_SPUS_CTRL_SIFS_CNT(0),
			ABOX_SIFS_CNT_VAL_MASK(1),
			cnt_val << ABOX_SIFS_CNT_VAL_L(1));
		break;
	case ABOX_SIFS2:
		ret = regmap_update_bits(regmap, ABOX_SPUS_CTRL_SIFS_CNT(1),
			ABOX_SIFS_CNT_VAL_MASK(2),
			cnt_val << ABOX_SIFS_CNT_VAL_L(2));
		break;
	default:
		dev_err(dev, "%s: invalid id(%d)\n", __func__, id);
		ret = -EINVAL;
		break;
	}
out:
	return ret;
}

static int sifs_hw_free(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct regmap *regmap = data->regmap;
	enum abox_dai id = dai->id;
	int ret = 0;

	if (substream->stream != SNDRV_PCM_STREAM_CAPTURE)
		goto out;

	dev_info(dev, "%s[%d]\n", __func__, id);

	switch (id) {
	case ABOX_SIFS0:
		ret = regmap_update_bits(regmap, ABOX_SPUS_CTRL_SIFS_CNT(0),
			ABOX_SIFS_CNT_VAL_MASK(0), 0);
		break;
	case ABOX_SIFS1:
		ret = regmap_update_bits(regmap, ABOX_SPUS_CTRL_SIFS_CNT(0),
			ABOX_SIFS_CNT_VAL_MASK(1), 0);
		break;
	case ABOX_SIFS2:
		ret = regmap_update_bits(regmap, ABOX_SPUS_CTRL_SIFS_CNT(1),
			ABOX_SIFS_CNT_VAL_MASK(2), 0);
		break;
	default:
		dev_err(dev, "%s: invalid id(%d)\n", __func__, id);
		ret = -EINVAL;
		break;
	}

	abox_request_cpu_gear(dev, data, substream, ABOX_CPU_GEAR_MIN);
out:
	return ret;
}

static const struct snd_soc_dai_ops sifs_dai_ops = {
	.hw_params	= sifs_hw_params,
	.hw_free	= sifs_hw_free,
};

static struct snd_soc_dai_driver abox_cmpnt_dai_drv[] = {
	{
		.name = "SIFS0",
		.id = ABOX_SIFS0,
		.playback = {
			.stream_name = "SIFS0 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
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
		.playback = {
			.stream_name = "SIFS1 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
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
		.playback = {
			.stream_name = "SIFS2 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
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
};

int abox_cmpnt_update_cnt_val(struct device *adev)
{
	struct abox_data *data = dev_get_drvdata(adev);
	unsigned long aclk = clk_get_rate(data->clk_bus);
	unsigned int sifs_cnt0, sifs_cnt1, cnt_val, rate, pwidth, channels;
	unsigned long sifs0_cnt, sifs1_cnt, sifs2_cnt;
	int ret;

	dev_dbg(adev, "%s\n", __func__);

	ret = regmap_read(data->regmap, ABOX_SPUS_CTRL_SIFS_CNT(0), &sifs_cnt0);
	if (ret < 0) {
		dev_err(adev, "%s: SPUS_CTRL_SIFS_CNT0 read fail: %d\n",
				__func__, ret);
		goto out;
	}
	ret = regmap_read(data->regmap, ABOX_SPUS_CTRL_SIFS_CNT(1), &sifs_cnt1);
	if (ret < 0) {
		dev_err(adev, "%s: SPUS_CTRL_SIFS_CNT1 read fail: %d\n",
				__func__, ret);
		goto out;
	}

	sifs0_cnt = (sifs_cnt0 & ABOX_SIFS_CNT_VAL_MASK(0)) >>
			ABOX_SIFS_CNT_VAL_L(0);
	sifs1_cnt = (sifs_cnt0 & ABOX_SIFS_CNT_VAL_MASK(1)) >>
			ABOX_SIFS_CNT_VAL_L(1);
	sifs2_cnt = (sifs_cnt1 & ABOX_SIFS_CNT_VAL_MASK(2)) >>
			ABOX_SIFS_CNT_VAL_L(2);

	if (sifs0_cnt) {
		rate = get_sif_rate(data, SET_SIFS0_RATE);
		pwidth = get_sif_physical_width(data, SET_SIFS0_FORMAT);
		channels = get_sif_channels(data, SET_SIFS0_FORMAT);
		cnt_val = sifsx_cnt_val(aclk, rate, pwidth, channels);
		dev_info(adev, "%s: %s <= %u\n", __func__, "SIFS0_CNT_VAL",
				cnt_val);
		ret = regmap_update_bits(data->regmap,
				ABOX_SPUS_CTRL_SIFS_CNT(0),
				ABOX_SIFS_CNT_VAL_MASK(0),
				cnt_val << ABOX_SIFS_CNT_VAL_L(0));
		if (ret < 0)
			dev_err(adev, "regmap update failed: %d\n", ret);
	}
	if (sifs1_cnt) {
		rate = get_sif_rate(data, SET_SIFS1_RATE);
		pwidth = get_sif_physical_width(data, SET_SIFS1_FORMAT);
		channels = get_sif_channels(data, SET_SIFS1_FORMAT);
		cnt_val = sifsx_cnt_val(aclk, rate, pwidth, channels);
		dev_info(adev, "%s: %s <= %u\n", __func__, "SIFS0_CNT_VAL",
				cnt_val);
		ret = regmap_update_bits(data->regmap,
				ABOX_SPUS_CTRL_SIFS_CNT(0),
				ABOX_SIFS_CNT_VAL_MASK(1),
				cnt_val << ABOX_SIFS_CNT_VAL_L(1));
		if (ret < 0)
			dev_err(adev, "regmap update failed: %d\n", ret);
	}
	if (sifs2_cnt) {
		rate = get_sif_rate(data, SET_SIFS2_RATE);
		pwidth = get_sif_physical_width(data, SET_SIFS2_FORMAT);
		channels = get_sif_channels(data, SET_SIFS2_FORMAT);
		cnt_val = sifsx_cnt_val(aclk, rate, pwidth, channels);
		dev_info(adev, "%s: %s <= %u\n", __func__, "SIFS1_CNT_VAL",
				cnt_val);
		ret = regmap_update_bits(data->regmap,
				ABOX_SPUS_CTRL_SIFS_CNT(1),
				ABOX_SIFS_CNT_VAL_MASK(2),
				cnt_val << ABOX_SIFS_CNT_VAL_L(2));
		if (ret < 0)
			dev_err(adev, "regmap update failed: %d\n", ret);
	}
out:
	return 0;
}

static struct snd_soc_dapm_widget *sync_params(struct abox_data *data,
		struct list_head *widget_list, int *stream,
		const struct snd_soc_dapm_widget *w_src,
		enum ABOX_CONFIGMSG rate, enum ABOX_CONFIGMSG format)
{
	struct device *dev = &data->pdev->dev;
	enum ABOX_CONFIGMSG msg_rate, msg_format;
	struct snd_soc_dapm_widget *w = NULL;
	bool slave;


	list_for_each_entry(w, widget_list, work_list) {
		if (!find_nrf(w, &msg_rate, &msg_format, stream, &slave))
			continue;
		if (slave)
			continue;

		dev_dbg(dev, "%s: %s => %s\n", __func__, w->name, w_src->name);
		set_sif_format(data, format, get_sif_format(data, msg_format));
		set_sif_rate(data, rate, get_sif_rate(data, msg_rate));
		set_sif_channels(data, format,
				get_sif_channels(data, msg_format));
		break;
	}

	return w;
}

int abox_cmpnt_hw_params_fixup_helper(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params, int stream)
{
	struct snd_soc_dai *dai = rtd->cpu_dai;
	struct snd_soc_component *cmpnt = dai->component;
	struct device *dev = dai->dev;
	struct device *adev = is_abox(dai->dev) ? dai->dev : dai->dev->parent;
	struct abox_data *data = dev_get_drvdata(adev);
	struct snd_soc_dapm_widget *w, *w_tgt = NULL;
	struct snd_soc_dapm_path *path;
	LIST_HEAD(widget_list);
	enum ABOX_CONFIGMSG msg_rate, msg_format;
	unsigned int rate, channels, width;
	snd_pcm_format_t format;
	struct snd_soc_dapm_widget *w_mst = NULL;
	int stream_mst;

	dev_info(dev, "%s[%s](%d)\n", __func__, dai->name, stream);

	if (params_channels(params) < 1) {
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

	/*
	 * For snd_soc_dapm_connected_{output,input}_ep fully discover the graph
	 * we need to reset the cached number of inputs and outputs.
	 */
	list_for_each_entry(w, &cmpnt->card->widgets, list) {
		w->endpoints[SND_SOC_DAPM_DIR_IN] = -1;
		w->endpoints[SND_SOC_DAPM_DIR_OUT] = -1;
	}
	snd_soc_dapm_widget_for_each_source_path(dai->playback_widget, path) {
		if (path->connect) {
			w = path->node[SND_SOC_DAPM_DIR_IN];
			snd_soc_dapm_connected_input_ep(w, &widget_list);
		}
	}
	snd_soc_dapm_widget_for_each_sink_path(dai->capture_widget, path) {
		if (path->connect) {
			w = path->node[SND_SOC_DAPM_DIR_OUT];
			snd_soc_dapm_connected_output_ep(w, &widget_list);
		}
	}

	/* find current params */
	list_for_each_entry(w, &widget_list, work_list) {
		bool slave;

		dev_dbg(dev, "%s\n", w->name);
		if (!find_nrf_stream(w, stream, &msg_rate, &msg_format,
				&slave))
			continue;

		if (slave)
			w_mst = sync_params(data, &widget_list, &stream_mst,
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

	/* channel mixing isn't supported */
	set_sif_channels(data, msg_format, params_channels(params));

	/* override formats to UAIF format, if it is connected */
	if (abox_if_hw_params_fixup_helper(rtd, params, stream) >= 0) {
		set_sif_auto_config(data, msg_rate, true);
	} else if (w_mst) {
		list_for_each_entry(w, &cmpnt->card->widgets, list) {
			w->endpoints[SND_SOC_DAPM_DIR_IN] = -1;
			w->endpoints[SND_SOC_DAPM_DIR_OUT] = -1;
		}
		list_del_init(&widget_list);
		if (stream_mst == SNDRV_PCM_STREAM_PLAYBACK)
			snd_soc_dapm_connected_output_ep(w_mst, &widget_list);
		else
			snd_soc_dapm_connected_input_ep(w_mst, &widget_list);

		list_for_each_entry(w, &widget_list, work_list) {
			struct snd_soc_dai *dai;

			if (!w->sname)
				continue;

			dai = w->priv;
			if (abox_if_hw_params_fixup(dai, params, stream)
					>= 0) {
				set_sif_auto_config(data, msg_rate, true);
				break;
			}
		}
	}

	if (!get_sif_auto_config(data, msg_rate))
		goto unlock;

	format = params_format(params);
	width = params_width(params);
	rate = params_rate(params);
	channels = params_channels(params);

	if (dai->driver->symmetric_samplebits && dai->sample_width &&
			dai->sample_width != width) {
		width = dai->sample_width;
		set_sif_width(data, msg_format, dai->sample_width);
		format = get_sif_format(data, msg_format);
	}

	if (dai->driver->symmetric_channels && dai->channels &&
			dai->channels != channels)
		channels = dai->channels;

	if (dai->driver->symmetric_rates && dai->rate && dai->rate != rate)
		rate = dai->rate;

	dev_dbg(dev, "%s: set to %u bit, %u channel, %uHz\n", __func__,
			width, channels, rate);
unlock:
	snd_soc_dapm_mutex_unlock(snd_soc_component_get_dapm(cmpnt));

	if (!w_tgt)
		goto out;

	rate_put_ipc(adev, rate, msg_rate);
	format_put_ipc(adev, format, channels, msg_format);

	hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE)->min =
			get_sif_rate(data, msg_rate);
	hw_param_interval(params, SNDRV_PCM_HW_PARAM_CHANNELS)->min =
			get_sif_channels(data, msg_format);
	snd_mask_none(hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT));
	snd_mask_set(hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT),
			get_sif_format(data, msg_format));
	dev_info(dev, "%s: %s: %d bit, %u channel, %uHz\n",
			__func__, w_tgt->name,
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
	{"%s Playback", NULL, "%s SPK"},
};

static const struct snd_soc_dapm_route route_base_if_cap[] = {
	/* sink, control, source */
	{"SPUSM", "%s", "%s Capture"},
	{"NSRC0", "%s", "%s Capture"},
	{"NSRC1", "%s", "%s Capture"},
	{"NSRC2", "%s", "%s Capture"},
	{"NSRC3", "%s", "%s Capture"},
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
		if (!route[i].sink)
			break;
		route[i].sink = devm_kasprintf(dev, GFP_KERNEL,
				route[i].sink, id);

		if (route[i].control)
			route[i].control = devm_kasprintf(dev, GFP_KERNEL,
					route[i].control, name);

		if (!route[i].source)
			break;
		route[i].source = devm_kasprintf(dev, GFP_KERNEL,
				route[i].source, id);
	}

	snd_soc_dapm_add_routes(dapm, route, i);
	kfree(route);

	return 0;
}

static const struct snd_soc_dapm_route route_base_wdma[][2] = {
	{
		/* sink, control, source */
		{"WDMA%d Capture", NULL, "RECP"},
		{"WDMA%d Capture", NULL, "SPUM ASRC0"},
	},
	{
		{"WDMA%d Capture", "WDMA", "SIFM0"},
	},
	{
		{"WDMA%d Capture", "WDMA", "SIFM1"},
	},
	{
		{"WDMA%d Capture", "WDMA", "SIFM2"},
	},
	{
		{"WDMA%d Capture", "WDMA", "SIFM3"},
	},
};

int abox_cmpnt_register_wdma(struct platform_device *pdev_abox,
		struct platform_device *pdev_wdma, unsigned int id,
		struct snd_soc_dapm_context *dapm, const char *name)
{
	struct device *dev = &pdev_wdma->dev;
	struct abox_data *data = platform_get_drvdata(pdev_abox);
	int ret;

	if (id < ARRAY_SIZE(data->pdev_wdma)) {
		data->pdev_wdma[id] = pdev_wdma;
		if (id > data->wdma_count)
			data->wdma_count = id + 1;
	} else {
		dev_err(&data->pdev->dev, "%s: invalid id(%u)\n", __func__, id);
		return -EINVAL;
	}

	ret = register_wdma_routes(dev, route_base_wdma[id],
			sizeof(route_base_wdma[id]), dapm, name, id);
	if (ret < 0)
		return ret;

	return 0;
}

int abox_cmpnt_restore(struct device *adev)
{
	struct abox_data *data = dev_get_drvdata(adev);
	int i;

	dev_info(adev, "%s\n", __func__);

	for (i = SET_SIFS0_RATE; i <= SET_SIFM4_RATE; i++)
		rate_put_ipc(adev, data->sif_rate[sif_idx(i)], i);
	for (i = SET_SIFS0_FORMAT; i <= SET_SIFM4_FORMAT; i++)
		format_put_ipc(adev, data->sif_format[sif_idx(i)],
				data->sif_channels[sif_idx(i)], i);
	erap_handler_put_ipc(adev, ERAP_ECHO_CANCEL,
			data->erap_status[ERAP_ECHO_CANCEL]);
	erap_handler_put_ipc(adev, ERAP_VI_SENSE,
			data->erap_status[ERAP_VI_SENSE]);
	audio_mode_put_ipc(adev, data->audio_mode);
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
