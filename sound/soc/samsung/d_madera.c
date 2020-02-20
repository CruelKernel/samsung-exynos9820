/*
 *  Sound Machine Driver for Madera CODECs on D
 *
 *  Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#define CHANGE_DEV_PRINT

#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/sec_debug.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <linux/mfd/cs35l41/registers.h>
#include <linux/mfd/cs35l41/calibration.h>
#include <linux/mfd/cs35l41/big_data.h>

#include <soc/samsung/exynos-pmu.h>

#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <sound/samsung/abox.h>
#include <sound/samsung/vts.h>
#include <sound/samsung/sec_audio_debug.h>

#include <linux/mfd/madera/core.h>
#include <linux/mfd/madera/registers.h>
#include "../codecs/madera.h"
#include "abox/abox_soc.h"

#define UAIF_START		24

#define UAIF0_DAI_ID		(1 << UAIF_START)
#define UAIF1_DAI_ID		(1 << (UAIF_START + 1))
#define UAIF2_DAI_ID		(1 << (UAIF_START + 2))
#define UAIF3_DAI_ID		(1 << (UAIF_START + 3))

#define CPCALL_RDMA_DAI_ID	(1 << 4)
#define CPCALL_WDMA_DAI_ID	(1 << 14)

#define CPCALL_RDMA_ID		4
#define CPCALL_WDMA_ID		2

#define MADERA_CODEC_MAX	32
#define MADERA_AUX_MAX		2
#define RDMA_COUNT		12
#define WDMA_COUNT		8
#define UAIF_COUNT		4

#define SYSCLK_RATE_CHECK_PARAM	8000
#define SYSCLK_RATE_48KHZ	98304000
#define SYSCLK_RATE_44_1KHZ	90316800

#define GPIO_MICBIAS_MAX	2

struct clk_conf {
	int id;
	int source;
	int rate;
	int fout;
};

struct d_drvdata {
	struct clk_conf fll1_refclk;
	struct clk_conf fll1_refclk_bclk;
	struct clk_conf fll2_refclk;
	struct clk_conf sysclk;
	struct clk_conf asyncclk;
	struct clk_conf dspclk;
	struct clk_conf ampclk;
	struct wake_lock wake_lock;

	unsigned int wake_lock_switch;
	unsigned int fm_mute_switch;
	unsigned int vss_state;
	unsigned int pcm_state;
	unsigned int clk_bclk_requested;

	int gpio_micbias[GPIO_MICBIAS_MAX];
};

static struct d_drvdata d_drvdata_info;
static struct snd_soc_card d_madera;
static struct clk *xclkout;

#define DEBUG_LEVEL_LOW 0

static struct snd_soc_pcm_runtime *get_rtd(struct snd_soc_card *card, int id)
{
	struct snd_soc_dai_link *dai_link;
	struct snd_soc_pcm_runtime *rtd = NULL;

	for (dai_link = card->dai_link;
		(dai_link - card->dai_link) < card->num_links;
		dai_link++) {
		if (id == dai_link->id) {
			rtd = snd_soc_get_pcm_runtime(card, dai_link->name);
			break;
		}
	}

	return rtd;
}

static int d_dai_ops_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct d_drvdata *drvdata = card->drvdata;
	struct snd_soc_dai_link *dai_link = rtd->dai_link;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	unsigned int channels = params_channels(params);
	unsigned int width = params_width(params);
	unsigned int rate = params_rate(params);
	unsigned int slot_mask;
	size_t buffer_bytes = params_buffer_bytes(params);
	int ret = 0;

	if (dai_link->id)
		drvdata->pcm_state |= (dai_link->id);

	dev_info(card->dev,
		"%s-%d: 0x%x: hw_params: %dch, %dHz, %dbytes, %dbit (pcm_state: 0x%07X)\n",
		rtd->dai_link->name, substream->stream, dai_link->id,
		channels, rate, buffer_bytes, width,
		drvdata->pcm_state);

	switch (dai_link->id) {
	case UAIF0_DAI_ID:
		switch (channels) {
		case 2:
			slot_mask = 0x3;
			break;
		case 4:
			slot_mask = 0xF;
			break;
		default:
			dev_err(card->dev, "Unsupported channels\n");
			return -ENOTSUPP;
		}

		ret = snd_soc_dai_set_tdm_slot(codec_dai, slot_mask, slot_mask,
					channels, width);
		if (ret)
			dev_err(card->dev, "set_tdm_slot error: %d\n", ret);

		drvdata->fll1_refclk_bclk.rate = snd_soc_params_to_bclk(params);
		dev_info(card->dev, "Set FLL1(BCLK) rate: %dHz",
						drvdata->fll1_refclk_bclk.rate);
		break;
	case UAIF2_DAI_ID:
		drvdata->fll2_refclk.rate = snd_soc_params_to_bclk(params);
		dev_info(card->dev, "Set FLL2(BCLK) rate: %dHz",
						drvdata->fll2_refclk.rate);
		break;
	default:
		break;
	}

	return ret;
}

static int d_dai_ops_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct d_drvdata *drvdata = card->drvdata;
	struct snd_soc_dai_link *dai_link = rtd->dai_link;
	int ret = 0;

	if (dai_link->id)
		drvdata->pcm_state &= (~dai_link->id);

	dev_info(card->dev, "%s: %s-%d (pcm_state: 0x%07X)\n",
			__func__, rtd->dai_link->name, substream->stream,
			drvdata->pcm_state);

	return ret;
}

static int d_dai_ops_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_codec *codec = rtd->codec_dai->codec;
	struct snd_soc_dai_link *dai_link = rtd->dai_link;
	struct d_drvdata *drvdata = card->drvdata;
	int ret = 0;

	dev_info(card->dev, "%s-%d: 0x%x: prepare\n",
			rtd->dai_link->name, substream->stream, dai_link->id);

	switch (dai_link->id) {
	case UAIF0_DAI_ID:
		snd_soc_dai_set_tristate(rtd->cpu_dai, 0); /* enable bclk */

		dev_info(card->dev, "Change FLL1(MCLK1->BCLK)\n");
		drvdata->clk_bclk_requested = 1;
		ret = snd_soc_codec_set_pll(codec, drvdata->fll1_refclk_bclk.id,
					drvdata->fll1_refclk_bclk.source,
					drvdata->fll1_refclk_bclk.rate,
					drvdata->fll1_refclk_bclk.fout);
		if (ret < 0)
			dev_err(card->dev, "Fail to change FLL1: %d\n", ret);
		break;
	case UAIF1_DAI_ID:
		snd_soc_dai_set_tristate(rtd->cpu_dai, 0); /* enable bclk */
		break;
	case UAIF2_DAI_ID:
		snd_soc_dai_set_tristate(rtd->cpu_dai, 0); /* enable bclk */

		dev_info(card->dev, "Set ASYNCCLK\n");
		ret = snd_soc_codec_set_sysclk(codec, drvdata->asyncclk.id,
						drvdata->asyncclk.source,
						drvdata->asyncclk.rate,
						SND_SOC_CLOCK_IN);
		if (ret < 0)
			dev_err(card->dev, "Fail to set ASYNCCLK: %d\n", ret);

		dev_info(card->dev, "Set FLL2(BCLK)\n");
		ret = snd_soc_codec_set_pll(codec, drvdata->fll2_refclk.id,
						drvdata->fll2_refclk.source,
						drvdata->fll2_refclk.rate,
						drvdata->fll2_refclk.fout);
		if (ret < 0)
			dev_err(card->dev, "Fail to set FLL2: %d\n", ret);
		break;
	default:
		break;
	}

	return ret;
}

static void d_dai_ops_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_codec *codec = rtd->codec_dai->codec;
	struct snd_soc_dai_link *dai_link = rtd->dai_link;
	struct d_drvdata *drvdata = card->drvdata;
	int ret;

	codec = rtd->codec_dai->codec;

	dev_info(card->dev,
		"%s-%d: 0x%x: shutdown: play: %d, cap: %d, active: %d\n",
		rtd->dai_link->name, substream->stream, dai_link->id,
		codec_dai->playback_active, codec_dai->capture_active,
		codec->component.active);

	switch (dai_link->id) {
	case UAIF0_DAI_ID:
		if (codec->component.active ||
			codec_dai->playback_active ||
			codec_dai->capture_active
		) {
			dev_info(card->dev, "Skip shutdown\n");
			break;
		}

		dev_info(card->dev, "Change FLL1(BCLK->MCLK1)\n");
		ret = snd_soc_codec_set_pll(codec, drvdata->fll1_refclk.id,
						drvdata->fll1_refclk.source,
						drvdata->fll1_refclk.rate,
						drvdata->fll1_refclk.fout);
		if (ret < 0)
			dev_err(card->dev, "Fail to change FLL1: %d\n", ret);

		drvdata->clk_bclk_requested = 0;

		snd_soc_dai_set_tristate(rtd->cpu_dai, 1); /* disable bclk */
		break;
	case UAIF1_DAI_ID:
		snd_soc_dai_set_tristate(rtd->cpu_dai, 1); /* disable bclk */
		break;
	case UAIF2_DAI_ID:
		dev_info(card->dev, "Reset ASYNCCLK\n");
		ret = snd_soc_codec_set_sysclk(codec, drvdata->asyncclk.id,
						0, 0, 0);
		if (ret < 0)
			dev_err(card->dev, "Fail to reset ASYNCCLK: %d\n", ret);

		dev_info(card->dev, "Reset FLL2\n");
		ret = snd_soc_codec_set_pll(codec, drvdata->fll2_refclk.id,
						0, 0, 0);
		if (ret < 0)
			dev_err(card->dev, "Fail to reset FLL2: %d\n", ret);

		snd_soc_dai_set_tristate(rtd->cpu_dai, 1); /* disable bclk */
		break;
	default:
		break;
	}
}

static const struct snd_soc_ops rdma_ops = {
	.hw_params = d_dai_ops_hw_params,
	.hw_free = d_dai_ops_hw_free,
	.prepare = d_dai_ops_prepare,
	.shutdown = d_dai_ops_shutdown,
};

static const struct snd_soc_ops wdma_ops = {
	.hw_params = d_dai_ops_hw_params,
	.hw_free = d_dai_ops_hw_free,
	.prepare = d_dai_ops_prepare,
	.shutdown = d_dai_ops_shutdown,
};

static const struct snd_soc_ops uaif_ops = {
	.hw_params = d_dai_ops_hw_params,
	.hw_free = d_dai_ops_hw_free,
	.prepare = d_dai_ops_prepare,
	.shutdown = d_dai_ops_shutdown,
};

static int d_set_bias_level(struct snd_soc_card *card,
				struct snd_soc_dapm_context *dapm,
				enum snd_soc_bias_level level)
{
	struct snd_soc_dai *codec_dai;
	struct snd_soc_codec *codec;
	struct d_drvdata *drvdata = card->drvdata;
	int ret;

	codec_dai = get_rtd(card, UAIF0_DAI_ID)->codec_dai;
	if (codec_dai == NULL) {
		dev_err(card->dev, "%s: Fail to get codec_dai\n", __func__);
		return 0;
	}

	codec = codec_dai->codec;

	if (dapm->dev != codec_dai->dev)
		return 0;

	switch (level) {
	case SND_SOC_BIAS_STANDBY:
		if (dapm->bias_level != SND_SOC_BIAS_OFF)
			break;

		clk_enable(xclkout);

		dev_info(card->dev, "%s: BIAS_STANDBY\n", __func__);

		dev_info(card->dev, "Set SYSCLK\n");
		ret = snd_soc_codec_set_sysclk(codec, drvdata->sysclk.id,
						drvdata->sysclk.source,
						drvdata->sysclk.rate,
						SND_SOC_CLOCK_IN);
		if (ret < 0)
			dev_err(card->dev, "Fail to set SYSCLK: %d\n", ret);

		if (drvdata->clk_bclk_requested) {
			dev_info(card->dev, "Already set FLL1(BCLK)\n");
			break;
		}

		dev_info(card->dev, "Set FLL1(MCLK1)\n", __func__);
		ret = snd_soc_codec_set_pll(codec, drvdata->fll1_refclk.id,
						drvdata->fll1_refclk.source,
						drvdata->fll1_refclk.rate,
						drvdata->fll1_refclk.fout);
		if (ret < 0)
			dev_err(card->dev, "Fail to set FLL1: %d\n", ret);
		break;
	case SND_SOC_BIAS_OFF:
		dev_info(card->dev, "%s: BIAS_OFF\n", __func__);

		dev_info(card->dev, "Reset SYSCLK\n");
		ret = snd_soc_codec_set_sysclk(codec, drvdata->sysclk.id,
						0, 0, 0);
		if (ret < 0)
			dev_err(card->dev, "Fail to reset SYSCLK: %d\n", ret);

		dev_info(card->dev, "Reset FLL1\n");
		ret = snd_soc_codec_set_pll(codec, drvdata->fll1_refclk.id,
						0, 0, 0);
		if (ret < 0)
			dev_err(card->dev, "Fail to reset FLL1: %d\n", ret);

		clk_disable(xclkout);
		break;
	default:
		break;
	}

	return 0;
}

static int d_madera_get_fm_mute_switch(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct snd_soc_card *card = codec->component.card;
	struct d_drvdata *drvdata = card->drvdata;

	ucontrol->value.integer.value[0] = drvdata->fm_mute_switch;

	return 0;
}

static int d_madera_set_fm_mute_switch(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct snd_soc_card *card = codec->component.card;
	struct d_drvdata *drvdata = card->drvdata;
	unsigned int val, fm_in_reg = MADERA_ADC_DIGITAL_VOLUME_2L;
	int i, ret = 0;

	val = (unsigned int)ucontrol->value.integer.value[0];
	drvdata->fm_mute_switch = val;

	dev_info(card->dev, "%s: mute %d\n", __func__, drvdata->fm_mute_switch);

#ifdef CONFIG_SEC_FACTORY
	return ret;
#endif

	for (i = 0; i < 2; i++) {
		ret = snd_soc_update_bits(codec, fm_in_reg + (i * 4),
			MADERA_IN2L_MUTE_MASK,
			drvdata->fm_mute_switch << MADERA_IN2L_MUTE_SHIFT);

		if (ret < 0)
			dev_err(card->dev, "Fail to update registers 0x%x\n",
					fm_in_reg + (i * 4));
	}

	return ret;
}

static const struct snd_kcontrol_new d_madera_codec_controls[] = {
	SOC_SINGLE_BOOL_EXT("FM Mute Switch", 0,
			d_madera_get_fm_mute_switch,
			d_madera_set_fm_mute_switch),
};

static int d_uaif0_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);
	struct d_drvdata *drvdata = snd_soc_card_get_drvdata(card);
	int ret;

	dev_info(card->dev, "%s\n", __func__);

	ret = snd_soc_add_codec_controls(codec, d_madera_codec_controls,
				ARRAY_SIZE(d_madera_codec_controls));
	if (ret < 0) {
		dev_err(card->dev, "Fail to add controls to codec: %d\n", ret);
		return ret;
	}

	ret = snd_soc_codec_set_sysclk(codec, drvdata->sysclk.id,
					drvdata->sysclk.source,
					drvdata->sysclk.rate,
					SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(card->dev, "Fail to set SYSCLK: %d\n", ret);
		return ret;
	}

	ret = snd_soc_codec_set_sysclk(codec, drvdata->dspclk.id,
					drvdata->dspclk.source,
					drvdata->dspclk.rate,
					SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(card->dev, "Fail to set DSPCLK: %d\n", ret);
		return ret;
	}

	dev_info(card->dev, "set AIF1 dai to sync domain\n");
	ret = snd_soc_dai_set_sysclk(codec_dai, drvdata->sysclk.id, 0, 0);
	if (ret != 0) {
		dev_err(card->dev, "Fail to set AIF1 dai clock: %d\n", ret);
		return ret;
	}

	snd_soc_dapm_ignore_suspend(dapm, "AIF1 Playback");
	snd_soc_dapm_ignore_suspend(dapm, "AIF1 Capture");
	snd_soc_dapm_sync(dapm);

	return 0;
}

static int d_uaif1_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai;
	struct snd_soc_dapm_context *dapm;
	struct d_drvdata *drvdata = snd_soc_card_get_drvdata(card);
	unsigned int num_codecs = rtd->num_codecs;
	char name[SZ_32];
	const char *prefix;
	int ret, i;

	dev_info(card->dev, "%s (%d)\n", __func__, num_codecs);

	for (i = 0; i < num_codecs; i++) {
		codec_dai = rtd->codec_dais[i];
		ret = snd_soc_codec_set_sysclk(codec_dai->codec,
						drvdata->ampclk.id,
						drvdata->ampclk.source,
						drvdata->ampclk.rate,
						SND_SOC_CLOCK_IN);
		if (ret < 0) {
			dev_err(card->dev, "Fail to set AMPCLK: %d\n", ret);
			return ret;
		}

		dapm = snd_soc_codec_get_dapm(codec_dai->codec);
		prefix = dapm->component->name_prefix;
		snprintf(name, sizeof(name), "%s AMP Playback", prefix);
		snd_soc_dapm_ignore_suspend(dapm, name);
		snprintf(name, sizeof(name), "%s AMP Capture", prefix);
		snd_soc_dapm_ignore_suspend(dapm, name);
		snd_soc_dapm_sync(dapm);
	}

	return 0;
}

static int d_uaif2_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);
	struct d_drvdata *drvdata = snd_soc_card_get_drvdata(card);
	int ret = 0;

	dev_info(card->dev, "%s\n", __func__);

	dev_info(card->dev, "Set ASYNCCLK\n");
	ret = snd_soc_codec_set_sysclk(codec, drvdata->asyncclk.id,
					drvdata->asyncclk.source,
					drvdata->asyncclk.rate,
					SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(card->dev, "Fail to set ASYNCCLK: %d\n", ret);
		return ret;
	}

	dev_info(card->dev, "set AIF3 dai to async domain\n");
	ret = snd_soc_dai_set_sysclk(codec_dai, drvdata->asyncclk.id, 0, 0);
	if (ret != 0) {
		dev_err(card->dev, "Fail to set AIF3 dai clock: %d\n", ret);
		return ret;
	}

	snd_soc_dapm_ignore_suspend(dapm, "AIF3 Playback");
	snd_soc_dapm_ignore_suspend(dapm, "AIF3 Capture");
	snd_soc_dapm_sync(dapm);

	return 0;
}

static int d_late_probe(struct snd_soc_card *card)
{
	struct snd_soc_dai *aif_dai;
	struct snd_soc_component *cpu;
	struct snd_soc_dapm_context *dapm;
	struct d_drvdata *drvdata = card->drvdata;
	char name[SZ_32];
	const char *prefix;
	int i;

	dev_info(card->dev, "%s\n", __func__);

	snd_soc_dapm_ignore_suspend(&card->dapm, "DMIC1");
	snd_soc_dapm_ignore_suspend(&card->dapm, "DMIC2");
	snd_soc_dapm_ignore_suspend(&card->dapm, "DMIC3");
	snd_soc_dapm_ignore_suspend(&card->dapm, "DMIC4");
	snd_soc_dapm_ignore_suspend(&card->dapm, "BLUETOOTH MIC");
	snd_soc_dapm_ignore_suspend(&card->dapm, "FM IN");
	snd_soc_dapm_ignore_suspend(&card->dapm, "RECEIVER");
	snd_soc_dapm_ignore_suspend(&card->dapm, "SPEAKER");
	snd_soc_dapm_ignore_suspend(&card->dapm, "BLUETOOTH SPK");
	snd_soc_dapm_ignore_suspend(&card->dapm, "VTS Virtual Output");
	snd_soc_dapm_sync(&card->dapm);

	for (i = 0; i < RDMA_COUNT; i++) {
		aif_dai = get_rtd(card, (1 << i))->cpu_dai;
		if (aif_dai == NULL) {
			dev_err(card->dev, "%s: Fail to get RDMA%d dai ptr\n",
				__func__, i);
			continue;
		}
		cpu = aif_dai->component;
		dapm = snd_soc_component_get_dapm(cpu);
		prefix = dapm->component->name_prefix;
		snprintf(name, sizeof(name), "%s RDMA%d Playback", prefix, i);
		snd_soc_dapm_ignore_suspend(dapm, name);
		snd_soc_dapm_sync(dapm);
	}

	for (i = 0; i < WDMA_COUNT; i++) {
		aif_dai = get_rtd(card, 1 << (RDMA_COUNT + i))->cpu_dai;
		if (aif_dai == NULL) {
			dev_err(card->dev, "%s: Fail to get WDMA%d dai ptr\n",
				__func__, i);
			continue;
		}
		cpu = aif_dai->component;
		dapm = snd_soc_component_get_dapm(cpu);
		prefix = dapm->component->name_prefix;
		snprintf(name, sizeof(name), "%s WDMA%d Capture", prefix, i);
		snd_soc_dapm_ignore_suspend(dapm, name);
		snd_soc_dapm_sync(dapm);
	}

	for (i = 0; i < UAIF_COUNT; i++) {
		aif_dai = get_rtd(card, 1 << (UAIF_START + i))->cpu_dai;
		if (aif_dai == NULL) {
			dev_err(card->dev, "%s: Fail to get UAIF%d dai ptr\n",
				__func__, i);
			continue;
		}
		cpu = aif_dai->component;
		dapm = snd_soc_component_get_dapm(cpu);
		prefix = dapm->component->name_prefix;
		snprintf(name, sizeof(name), "%s UAIF%d Capture", prefix, i);
		snd_soc_dapm_ignore_suspend(dapm, name);
		snprintf(name, sizeof(name), "%s UAIF%d Playback", prefix, i);
		snd_soc_dapm_ignore_suspend(dapm, name);
		snd_soc_dapm_sync(dapm);
	}

	wake_lock_init(&drvdata->wake_lock, WAKE_LOCK_SUSPEND, "madera-sound");
	drvdata->wake_lock_switch = 0;

	register_debug_mixer(card);

	return 0;
}

static struct snd_soc_dai_link d_dai[] = {
	{
		.name = "RDMA0",
		.stream_name = "RDMA0",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
		.id = (1 << 0),
	},
	{
		.name = "RDMA1",
		.stream_name = "RDMA1",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
		.id = (1 << 1),
	},
	{
		.name = "RDMA2",
		.stream_name = "RDMA2",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
		.id = (1 << 2),
	},
	{
		.name = "RDMA3",
		.stream_name = "RDMA3",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
		.id = (1 << 3),
	},
	{
		.name = "RDMA4",
		.stream_name = "RDMA4",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
		.id = CPCALL_RDMA_DAI_ID,
	},
	{
		.name = "RDMA5",
		.stream_name = "RDMA5",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_BESPOKE,
			SND_SOC_DPCM_TRIGGER_BESPOKE
		},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
		.id = (1 << 5),
	},
	{
		.name = "RDMA6",
		.stream_name = "RDMA6",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
		.id = (1 << 6),
	},
	{
		.name = "RDMA7",
		.stream_name = "RDMA7",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
		.id = (1 << 7),
	},
	{
		.name = "RDMA8",
		.stream_name = "RDMA8",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
		.id = (1 << 8),
	},
	{
		.name = "RDMA9",
		.stream_name = "RDMA9",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
		.id = (1 << 9),
	},
	{
		.name = "RDMA10",
		.stream_name = "RDMA10",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
		.id = (1 << 10),
	},
	{
		.name = "RDMA11",
		.stream_name = "RDMA11",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
		.id = (1 << 11),
	},
	{
		.name = "WDMA0",
		.stream_name = "WDMA0",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
		.id = (1 << 12),
	},
	{
		.name = "WDMA1",
		.stream_name = "WDMA1",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
		.id = (1 << 13),
	},
	{
		.name = "WDMA2",
		.stream_name = "WDMA2",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
		.id = CPCALL_WDMA_DAI_ID,
	},
	{
		.name = "WDMA3",
		.stream_name = "WDMA3",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
		.id = (1 << 15),
	},
	{
		.name = "WDMA4",
		.stream_name = "WDMA4",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
		.id = (1 << 16),
	},
	{
		.name = "WDMA5",
		.stream_name = "WDMA5",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
		.id = (1 << 17),
	},
	{
		.name = "WDMA6",
		.stream_name = "WDMA6",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
		.id = (1 << 18),
	},
	{
		.name = "WDMA7",
		.stream_name = "WDMA7",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST_PRE,
			SND_SOC_DPCM_TRIGGER_PRE_POST
		},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
		.id = (1 << 19),
	},
#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_VTS)
	{
		.name = "VTS-Trigger",
		.stream_name = "VTS-Trigger",
		.cpu_dai_name = "vts-tri",
		.platform_name = "15510000.vts:vts_dma@0",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.capture_only = true,
	},
	{
		.name = "VTS-Record",
		.stream_name = "VTS-Record",
		.cpu_dai_name = "vts-rec",
		.platform_name = "15510000.vts:vts_dma@1",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.capture_only = true,
	},
#endif
#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_DISPLAYPORT)
	{
		.name = "DP Audio",
		.stream_name = "DP Audio",
		.cpu_dai_name = "audio_cpu_dummy",
		.platform_name = "dp_dma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
	},
#endif
	{
		.name = "UAIF0",
		.stream_name = "UAIF0",
		.platform_name = "snd-soc-dummy",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.init = d_uaif0_init,
		.ops = &uaif_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.id = UAIF0_DAI_ID,
	},
	{
		.name = "UAIF1",
		.stream_name = "UAIF1",
		.platform_name = "snd-soc-dummy",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.init = d_uaif1_init,
		.ops = &uaif_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.id = UAIF1_DAI_ID,
	},
	{
		.name = "UAIF2",
		.stream_name = "UAIF2",
		.platform_name = "snd-soc-dummy",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.init = d_uaif2_init,
		.ops = &uaif_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.id = UAIF2_DAI_ID,
	},
	{
		.name = "UAIF3",
		.stream_name = "UAIF3",
		.cpu_dai_name = "UAIF3",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.ops = &uaif_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.id = UAIF3_DAI_ID,
	},
	{
		.name = "DSIF",
		.stream_name = "DSIF",
		.cpu_dai_name = "DSIF",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
	},
	{
		.name = "SIFS0",
		.stream_name = "SIFS0",
		.cpu_dai_name = "SIFS0",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_capture = 1,
	},
	{
		.name = "SIFS1",
		.stream_name = "SIFS1",
		.cpu_dai_name = "SIFS1",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_capture = 1,
	},
	{
		.name = "SIFS2",
		.stream_name = "SIFS2",
		.cpu_dai_name = "SIFS2",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_capture = 1,
	},
	{
		.name = "SIFS3",
		.stream_name = "SIFS3",
		.cpu_dai_name = "SIFS3",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_capture = 1,
	},
	{
		.name = "SIFS4",
		.stream_name = "SIFS4",
		.cpu_dai_name = "SIFS4",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_capture = 1,
	},
	{
		.name = "RSRC0",
		.stream_name = "RSRC0",
		.cpu_dai_name = "RSRC0",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "RSRC1",
		.stream_name = "RSRC1",
		.cpu_dai_name = "RSRC1",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "NSRC0",
		.stream_name = "NSRC0",
		.cpu_dai_name = "NSRC0",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "NSRC1",
		.stream_name = "NSRC1",
		.cpu_dai_name = "NSRC1",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "NSRC2",
		.stream_name = "NSRC2",
		.cpu_dai_name = "NSRC2",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "NSRC3",
		.stream_name = "NSRC3",
		.cpu_dai_name = "NSRC3",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "NSRC4",
		.stream_name = "NSRC4",
		.cpu_dai_name = "NSRC4",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "NSRC5",
		.stream_name = "NSRC5",
		.cpu_dai_name = "NSRC5",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "NSRC6",
		.stream_name = "NSRC6",
		.cpu_dai_name = "NSRC6",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{ /* pcm dump interface */
		.name = "CPU-DSP trace",
		.stream_name = "CPU-DSP trace",
		.cpu_dai_name = "cs47l92-cpu-trace",
		.platform_name = "cs47l92-codec",
		.codec_dai_name = "cs47l92-dsp-trace",
		.codec_name = "cs47l92-codec",
	},
};

static void set_gpio_micbias(struct snd_soc_card *card,
				unsigned int index, int event)
{
	struct d_drvdata *drvdata = card->drvdata;
	int gpio = -ENOENT, value = 0;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		value = 1;
		break;
	case SND_SOC_DAPM_POST_PMD:
		value = 0;
		break;
	default:
		return;
	}

	if (index < GPIO_MICBIAS_MAX)
		gpio = drvdata->gpio_micbias[index];

	if (gpio_is_valid(gpio)) {
		gpio_set_value_cansleep(gpio, value);
		dev_info(card->dev, "%s(%d): %d\n", __func__, index, value);
	}
};

static int d_gpio_micbias3(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	set_gpio_micbias(card, 0, event);

	return 0;
}

static int d_gpio_micbias4(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	set_gpio_micbias(card, 1, event);

	return 0;
}

static int d_dmic1(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int d_dmic2(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int d_dmic3(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int d_dmic4(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int d_btsco_mic(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int d_fm_in(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int d_receiver(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMD:
		cirrus_bd_store_values_left();
		break;
	}

	return 0;
}

static int d_speaker(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMD:
		cirrus_bd_store_values_right();
		break;
	}

	return 0;
}

static int d_btsco(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int d_suspend_post(struct snd_soc_card *card)
{
	struct snd_soc_dai *aif_dai;
	struct snd_soc_codec *codec;
	struct d_drvdata *drvdata = card->drvdata;
	int ret = 0;

	aif_dai = get_rtd(card, UAIF0_DAI_ID)->codec_dai;
	if (aif_dai == NULL) {
		dev_err(card->dev, "%s: Fail to get aif_dai\n", __func__);
		return ret;
	}

	codec = aif_dai->codec;

	dev_info(card->dev, "%s component active: %d, vts: %d\n", __func__,
			codec->component.active, vts_is_recognitionrunning());

	if (codec->component.active)
		return ret;

	if (vts_is_recognitionrunning()) {
		madera_enable_force_bypass(codec);

		dev_info(card->dev, "Reset SYSCLK\n");
		ret = snd_soc_codec_set_sysclk(codec, drvdata->sysclk.id,
						0, 0, 0);
		if (ret < 0)
			dev_err(card->dev, "Fail to reset SYSCLK: %d\n", ret);

		dev_info(card->dev, "Reset FLL1\n");
		ret = snd_soc_codec_set_pll(codec, drvdata->fll1_refclk.id,
						0, 0, 0);
		if (ret < 0)
			dev_err(card->dev, "Fail to reset FLL1: %d\n", ret);
	}

	return ret;
}

static int d_resume_pre(struct snd_soc_card *card)
{
	struct snd_soc_dai *aif_dai;
	struct snd_soc_codec *codec;
	struct d_drvdata *drvdata = card->drvdata;
	int ret = 0;

	aif_dai = get_rtd(card, UAIF0_DAI_ID)->codec_dai;
	if (aif_dai == NULL) {
		dev_err(card->dev, "%s: Fail to get aif_dai\n", __func__);
		return ret;
	}

	codec = aif_dai->codec;

	dev_info(card->dev, "%s component active: %d, vts: %d\n", __func__,
			codec->component.active, vts_is_recognitionrunning());

	if (codec->component.active)
		return ret;

	if (vts_is_recognitionrunning()) {
		dev_info(card->dev, "Set SYSCLK\n");
		ret = snd_soc_codec_set_sysclk(codec, drvdata->sysclk.id,
						drvdata->sysclk.source,
						drvdata->sysclk.rate,
						SND_SOC_CLOCK_IN);
		if (ret < 0)
			dev_err(card->dev, "Fail to set SYSCLK: %d\n", ret);

		dev_info(card->dev, "Set FLL1(MCLK1)\n");
		ret = snd_soc_codec_set_pll(codec, drvdata->fll1_refclk.id,
						drvdata->fll1_refclk.source,
						drvdata->fll1_refclk.rate,
						drvdata->fll1_refclk.fout);
		if (ret < 0)
			dev_err(card->dev, "Fail to set FLL1: %d\n", ret);

		madera_disable_force_bypass(codec);
	}

	return ret;
}

static int d_get_sound_wakelock(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct d_drvdata *drvdata = &d_drvdata_info;

	ucontrol->value.integer.value[0] = drvdata->wake_lock_switch;

	return 0;
}

static int d_set_sound_wakelock(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	struct d_drvdata *drvdata = &d_drvdata_info;
	unsigned int val;

	val = (unsigned int)ucontrol->value.integer.value[0];
	drvdata->wake_lock_switch = val;

	dev_info(card->dev, "%s: %d\n", __func__, drvdata->wake_lock_switch);

	if (drvdata->wake_lock_switch)
		wake_lock(&drvdata->wake_lock);
	else
		wake_unlock(&drvdata->wake_lock);

	return 0;
}

static const char * const vss_state_enum_texts[] = {
	"NORMAL",
	"INIT FAIL",
	"SUSPENDED",
	"PCM OPEN FAIL",
	"ABOX STUCK",
};

SOC_ENUM_SINGLE_DECL(vss_state_enum, SND_SOC_NOPM, 0, vss_state_enum_texts);

static int d_vss_state_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct d_drvdata *drvdata = &d_drvdata_info;

	ucontrol->value.enumerated.item[0] = drvdata->vss_state;

	return 0;
}

static int d_vss_state_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	struct abox_data *data = abox_get_abox_data();
	struct d_drvdata *drvdata = &d_drvdata_info;
	const unsigned int item = ucontrol->value.enumerated.item[0];
	int rd_state = 0, wr_state = 0;

	if (item >= e->items) {
		dev_err(card->dev, "%s item %d overflow\n", __func__, item);
		return -EINVAL;
	}

	drvdata->vss_state = item;
	dev_info(card->dev, "VSS State: %s\n", vss_state_enum_texts[item]);

	/* Make Call BUG_ON if ABOX vss delivers the stuck state */
	if (!strcmp(vss_state_enum_texts[item], "ABOX STUCK")) {
		if (data->audio_mode != MODE_IN_CALL) {
			dev_info(card->dev, "%s: audio_mode is not call %d\n",
					__func__, data->audio_mode);
			return -EPERM;
		}

		rd_state = is_abox_rdma_enabled(CPCALL_RDMA_ID);
		wr_state = is_abox_wdma_enabled(CPCALL_WDMA_ID);

		dev_info(card->dev, "%s: Abox RDMA[%d](%d), WDMA[%d](%d)\n",
			__func__, CPCALL_RDMA_ID, rd_state,
			CPCALL_WDMA_ID, wr_state);

		dev_info(card->dev,
			"%s: audio_mode=%d, rdma_state=%1d, wdma_state=%1d\n",
			__func__, data->audio_mode,
			((drvdata->pcm_state & CPCALL_RDMA_DAI_ID) ? 1:0),
			((drvdata->pcm_state & CPCALL_WDMA_DAI_ID) ? 1:0));

		if ((data->audio_mode == MODE_IN_CALL)
			&& ((drvdata->pcm_state & CPCALL_RDMA_DAI_ID) ? 1:0)
			&& ((drvdata->pcm_state & CPCALL_WDMA_DAI_ID) ? 1:0)
		) {
			dev_info(card->dev, "%s: Abox is normal\n", __func__);
			return -EAGAIN;
		}

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
		return -EPERM;
#else
		BUG_ON(1);
#endif
	}

	return 0;
}

static const char * const vts_output_texts[] = {
	"None",
	"DMIC1",
};

static const struct soc_enum vts_output_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(vts_output_texts),
			vts_output_texts);

static const struct snd_kcontrol_new vts_output_mux[] = {
	SOC_DAPM_ENUM("VTS Virtual Output Mux", vts_output_enum),
};

static const struct snd_kcontrol_new d_controls[] = {
	SOC_DAPM_PIN_SWITCH("DMIC1"),
	SOC_DAPM_PIN_SWITCH("DMIC2"),
	SOC_DAPM_PIN_SWITCH("DMIC3"),
	SOC_DAPM_PIN_SWITCH("DMIC4"),
	SOC_DAPM_PIN_SWITCH("FM IN"),

	SOC_SINGLE_BOOL_EXT("Sound Wakelock", 0,
			d_get_sound_wakelock, d_set_sound_wakelock),

	SOC_ENUM_EXT("ABOX VSS State", vss_state_enum,
			d_vss_state_get, d_vss_state_put),
};

static struct snd_soc_dapm_widget d_widgets[] = {
	SND_SOC_DAPM_SUPPLY("GPIO_MICBIAS3", SND_SOC_NOPM, 0, 0,
		d_gpio_micbias3, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY("GPIO_MICBIAS4", SND_SOC_NOPM, 0, 0,
		d_gpio_micbias4, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_REGULATOR_SUPPLY("LDO_MICBIAS3", 0, 0),
	SND_SOC_DAPM_REGULATOR_SUPPLY("LDO_MICBIAS4", 0, 0),

	SND_SOC_DAPM_MIC("DMIC1", d_dmic1),
	SND_SOC_DAPM_MIC("DMIC2", d_dmic2),
	SND_SOC_DAPM_MIC("DMIC3", d_dmic3),
	SND_SOC_DAPM_MIC("DMIC4", d_dmic4),
	SND_SOC_DAPM_MIC("BLUETOOTH MIC", d_btsco_mic),
	SND_SOC_DAPM_MIC("FM IN", d_fm_in),
	SND_SOC_DAPM_SPK("RECEIVER", d_receiver),
	SND_SOC_DAPM_SPK("SPEAKER", d_speaker),
	SND_SOC_DAPM_SPK("BLUETOOTH SPK", d_btsco),

	SND_SOC_DAPM_OUTPUT("VTS Virtual Output"),
	SND_SOC_DAPM_MUX("VTS Virtual Output Mux", SND_SOC_NOPM, 0, 0,
		&vts_output_mux[0]),
};

static struct snd_soc_dapm_route d_routes[] = {
	{"VTS Virtual Output Mux", "DMIC1", "DMIC1"},
};

static struct snd_soc_codec_conf codec_conf[MADERA_CODEC_MAX];

static struct snd_soc_aux_dev aux_dev[MADERA_AUX_MAX];

static struct snd_soc_card d_madera = {
	.name = "d-madera",
	.owner = THIS_MODULE,
	.dai_link = d_dai,
	.num_links = ARRAY_SIZE(d_dai),

	.late_probe = d_late_probe,

	.controls = d_controls,
	.num_controls = ARRAY_SIZE(d_controls),
	.dapm_widgets = d_widgets,
	.num_dapm_widgets = ARRAY_SIZE(d_widgets),
	.dapm_routes = d_routes,
	.num_dapm_routes = ARRAY_SIZE(d_routes),

	.set_bias_level = d_set_bias_level,

	.drvdata = (void *)&d_drvdata_info,

	.codec_conf = codec_conf,
	.num_configs = ARRAY_SIZE(codec_conf),

	.aux_dev = aux_dev,
	.num_aux_devs = ARRAY_SIZE(aux_dev),

	.suspend_post = d_suspend_post,
	.resume_pre = d_resume_pre,
};

static int read_clk_conf(struct device_node *np, const char * const prop,
			struct clk_conf *conf, bool is_fll)
{
	u32 tmp;
	int ret;

	ret = of_property_read_u32_index(np, prop, 0, &tmp);
	if (ret)
		return ret;

	conf->id = tmp;

	ret = of_property_read_u32_index(np, prop, 1, &tmp);
	if (ret)
		return ret;

	if (tmp < 0xffff)
		conf->source = tmp;
	else
		conf->source = -1;

	ret = of_property_read_u32_index(np, prop, 2, &tmp);
	if (ret)
		return ret;

	conf->rate = tmp;

	if (is_fll) {
		ret = of_property_read_u32_index(np, prop, 3, &tmp);
		if (ret)
			return ret;

		conf->fout = tmp;
	}

	return 0;
}

static int read_platform(struct device_node *np, const char * const prop,
			struct device_node **dai)
{
	int ret = 0;

	np = of_get_child_by_name(np, prop);
	if (!np)
		return -ENOENT;

	*dai = of_parse_phandle(np, "sound-dai", 0);
	if (!*dai) {
		ret = -ENODEV;
		goto out;
	}
out:
	of_node_put(np);

	return ret;
}

static int read_cpu(struct device_node *np, struct device *dev,
			struct snd_soc_dai_link *dai_link)
{
	int ret = 0;

	np = of_get_child_by_name(np, "cpu");
	if (!np)
		return -ENOENT;

	dai_link->cpu_of_node = of_parse_phandle(np, "sound-dai", 0);
	if (!dai_link->cpu_of_node) {
		ret = -ENODEV;
		goto out;
	}

	if (dai_link->cpu_dai_name == NULL) {
		/* Ignoring return as we don't register DAIs to the platform */
		ret = snd_soc_of_get_dai_name(np, &dai_link->cpu_dai_name);
		if (ret)
			goto out;
	}
out:
	of_node_put(np);

	return ret;
}

static int read_codec(struct device_node *np, struct device *dev,
			struct snd_soc_dai_link *dai_link)
{
	np = of_get_child_by_name(np, "codec");
	if (!np)
		return -ENOENT;

	return snd_soc_of_get_dai_link_codecs(dev, np, dai_link);
}

static void gpio_micbias_config(struct snd_soc_card *card,
				struct device_node *np)
{
	struct d_drvdata *drvdata = card->drvdata;
	int ret, gpio, i;
	char gpio_name[32];

	for (i = 0; i < GPIO_MICBIAS_MAX; i++) {
		drvdata->gpio_micbias[i] = -ENOENT;

		gpio = of_get_named_gpio(np, "gpio_micbias", i);
		if (!gpio_is_valid(gpio))
			continue;

		snprintf(gpio_name, sizeof(gpio_name), "gpio_micbias-%d", i);

		ret = devm_gpio_request_one(card->dev, gpio,
						GPIOF_OUT_INIT_LOW, gpio_name);
		if (ret < 0)
			continue;

		drvdata->gpio_micbias[i] = gpio;
	}
}

static int d_audio_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &d_madera;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *dai;
	struct d_drvdata *drvdata = card->drvdata;
	int i, ret, nlink = 0;

	card->dev = &pdev->dev;

	snd_soc_card_set_drvdata(card, drvdata);

	xclkout = devm_clk_get(&pdev->dev, "xclkout");
	if (IS_ERR(xclkout)) {
		dev_err(&pdev->dev, "Fail to get xclkout\n");
		xclkout = NULL;
	}

	ret = clk_prepare(xclkout);
	if (ret < 0) {
		dev_err(card->dev, "Fail to prepare xclkout\n");
		goto err_clk_get;
	}

	ret = read_clk_conf(np, "cirrus,sysclk", &drvdata->sysclk, false);
	if (ret) {
		dev_err(card->dev, "Fail to parse sysclk: %d\n", ret);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,asyncclk", &drvdata->asyncclk, false);
	if (ret) {
		dev_err(card->dev, "Fail to parse asyncclk: %d\n", ret);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,dspclk", &drvdata->dspclk, false);
	if (ret) {
		dev_err(card->dev, "Fail to parse dspclk: %d\n", ret);
		goto err_clk_prepare;
	} else if ((drvdata->dspclk.source == drvdata->sysclk.source) &&
		((drvdata->dspclk.rate / 3) != (drvdata->sysclk.rate / 2))) {
		/* DSPCLK & SYSCLK, if sharing a source, should be an
		 * appropriate ratio of one another, or both be zero (which
		 * signifies "automatic" mode).
		 */
		dev_err(card->dev, "%s %s %d vs %d\n",
			"DSPCLK & SYSCLK share src",
			"but request incompatible frequencies:",
			drvdata->dspclk.rate, drvdata->sysclk.rate);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,fll1-refclk",
					&drvdata->fll1_refclk, true);
	if (ret) {
		dev_err(card->dev, "Fail to parse fll1-refclk: %d\n", ret);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,fll1-refclk-bclk",
					&drvdata->fll1_refclk_bclk, true);
	if (ret) {
		dev_err(card->dev, "Fail to parse fll1-refclk-bclk: %d\n", ret);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,fll2-refclk",
					&drvdata->fll2_refclk, true);
	if (ret) {
		dev_err(card->dev, "Fail to parse fll2-refclk: %d\n", ret);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,ampclk", &drvdata->ampclk, false);
	if (ret) {
		dev_err(card->dev, "Fail to parse ampclk: %d\n", ret);
		goto err_clk_prepare;
	}

	for_each_child_of_node(np, dai) {
		struct snd_soc_dai_link *link = &d_dai[nlink];

		if (!link->name)
			link->name = dai->name;

		if (!link->stream_name)
			link->stream_name = dai->name;

		if (!link->cpu_name) {
			ret = read_cpu(dai, card->dev, link);
			if (ret) {
				dev_err(card->dev,
					"Fail to parse cpu DAI %s: %d\n",
					dai->name, ret);
				goto err_clk_prepare;
			}
		}

		if (!link->platform_name) {
			ret = read_platform(dai, "platform",
						&link->platform_of_node);
			if (ret) {
				link->platform_of_node = link->cpu_of_node;
				dev_info(card->dev,
					"Cpu node is used as platform %s: %d\n",
					dai->name, ret);
			}
		}

		if (!link->codec_name) {
			ret = read_codec(dai, card->dev, link);
			if (ret) {
				dev_err(card->dev,
					"Fail to parse codec DAI %s: %d\n",
					dai->name, ret);

				link->codec_name = "snd-soc-dummy";
				link->codec_dai_name = "snd-soc-dummy-dai";
			}
		}

		link->dai_fmt = snd_soc_of_parse_daifmt(dai, NULL, NULL, NULL);

		if (++nlink == card->num_links)
			break;
	}

	if (!nlink)
		dev_warn(card->dev, "No DAIs specified\n");

	card->num_links = nlink;

	if (of_property_read_bool(np, "samsung,routing")) {
		ret = snd_soc_of_parse_audio_routing(card, "samsung,routing");
		if (ret)
			goto err_clk_prepare;
	}

	for (i = 0; i < ARRAY_SIZE(codec_conf); i++) {
		codec_conf[i].of_node = of_parse_phandle(np,
						"samsung,codec", i);
		if (!codec_conf[i].of_node)
			break;

		ret = of_property_read_string_index(np, "samsung,prefix", i,
						&codec_conf[i].name_prefix);
		if (ret < 0)
			codec_conf[i].name_prefix = "";
	}
	card->num_configs = i;

	for (i = 0; i < ARRAY_SIZE(aux_dev); i++) {
		aux_dev[i].codec_of_node = of_parse_phandle(np,
						"samsung,aux", i);
		if (!aux_dev[i].codec_of_node)
			break;
	}
	card->num_aux_devs = i;

	ret = devm_snd_soc_register_card(card->dev, card);
	if (ret) {
		dev_err(card->dev, "snd_soc_register_card() failed: %d\n", ret);
		goto err_clk_prepare;
	}

#ifdef CONFIG_SEC_DEBUG
	if (sec_debug_get_debug_level() == DEBUG_LEVEL_LOW) {
		ret = abox_set_profiling(0);
		dev_info(card->dev, "disable abox profiling, ret=%d\n", ret);
	}
#endif

	gpio_micbias_config(card, np);

	return ret;

err_clk_prepare:
	clk_unprepare(xclkout);
err_clk_get:
	devm_clk_put(&pdev->dev, xclkout);

	return ret;
}

static int d_audio_remove(struct platform_device *pdev)
{
	clk_unprepare(xclkout);
	devm_clk_put(&pdev->dev, xclkout);

	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id d_audio_of_match[] = {
	{ .compatible = "samsung,d-madera", },
	{},
};
MODULE_DEVICE_TABLE(of, d_audio_of_match);
#endif /* CONFIG_OF */

static struct platform_driver d_audio_driver = {
	.driver		= {
		.name	= "d-madera",
		.owner	= THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = of_match_ptr(d_audio_of_match),
	},

	.probe		= d_audio_probe,
	.remove		= d_audio_remove,
};

module_platform_driver(d_audio_driver);

MODULE_DESCRIPTION("ALSA SoC D Audio Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:d-madera");
