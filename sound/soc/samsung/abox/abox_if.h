/* sound/soc/samsung/abox/abox_if.h
 *
 * ALSA SoC - Samsung Abox UAIF/DSIF driver
 *
 * Copyright (c) 2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_IF_H
#define __SND_SOC_ABOX_IF_H

#include "abox.h"

enum abox_if_config {
	ABOX_IF_WIDTH,
	ABOX_IF_CHANNEL,
	ABOX_IF_RATE,
	ABOX_IF_FMT_COUNT,
};

struct abox_if_of_data {
	enum abox_dai (*get_dai_id)(int id);
	const char *(*get_dai_name)(int id);
	const char *(*get_str_name)(int id, int stream);
	struct snd_soc_dai_driver *base_dai_drv;
};

struct abox_if_data {
	int id;
	void __iomem *sfr_base;
	struct clk *clk_bclk;
	struct clk *clk_bclk_gate;
	struct snd_soc_component *cmpnt;
	struct snd_soc_dai_driver *dai_drv;
	struct abox_data *abox_data;
	const struct abox_if_of_data *of_data;
	unsigned int config[ABOX_IF_FMT_COUNT];
};

/**
 * UAIF/DSIF hw params fixup helper by dai
 * @param[in]	dai	snd_soc_dai
 * @param[out]	params	snd_pcm_hw_params
 * @param[in]	stream	SNDRV_PCM_STREAM_PLAYBACK or SNDRV_PCM_STREAM_CAPTURE
 * @return		error code if any
 */
extern int abox_if_hw_params_fixup(struct snd_soc_dai *dai,
		struct snd_pcm_hw_params *params, int stream);

/**
 * UAIF/DSIF hw params fixup helper
 * @param[in]	rtd	snd_soc_pcm_runtime
 * @param[out]	params	snd_pcm_hw_params
 * @param[in]	stream	SNDRV_PCM_STREAM_PLAYBACK or SNDRV_PCM_STREAM_CAPTURE
 * @return		error code if any
 */
extern int abox_if_hw_params_fixup_helper(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params, int stream);

#endif /* __SND_SOC_ABOX_IF_H */
