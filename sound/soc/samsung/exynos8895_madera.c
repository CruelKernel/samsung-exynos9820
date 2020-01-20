/*
 *  Driver for Madera CODECs on Exynos8895
 *
 *  Copyright 2013 Wolfson Microelectronics
 *  Copyright 2016 Cirrus Logic
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/debugfs.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/mfd/madera/core.h>
#include <linux/extcon/extcon-madera.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/clk.h>

#include <soc/samsung/exynos-pmu.h>
#include <sound/samsung/abox.h>

#include "../codecs/madera.h"

#define EXYNOS_PMU_PMU_DEBUG_OFFSET		(0x0A00)
#define MADERA_DAI_OFFSET			(13)

/* Used for debugging and test automation */
static u32 voice_trigger_count;

struct clk_conf {
	int id;
	int source;
	int rate;
};

struct exynos8895_drvdata {
	struct device *dev;

	struct clk_conf fll1_refclk;
	struct clk_conf sysclk;
	struct clk_conf dspclk;

	struct notifier_block nb;
};

static struct exynos8895_drvdata exynos8895_drvdata;

static const struct snd_soc_ops rdma_ops = {
};

static const struct snd_soc_ops wdma_ops = {
};

static const struct snd_soc_ops uaif_ops = {
};

static int dsif_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *hw_params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int tx_slot[] = {0, 1};

	/* bclk ratio 64 for DSD64, 128 for DSD128 */
	snd_soc_dai_set_bclk_ratio(cpu_dai, 64);

	/* channel map 0 1 if left is first, 1 0 if right is first */
	snd_soc_dai_set_channel_map(cpu_dai, 2, tx_slot, 0, NULL);
	return 0;
}

static const struct snd_soc_ops dsif_ops = {
	.hw_params = dsif_hw_params,
};

static int exynos8895_set_bias_level(struct snd_soc_card *card,
				  struct snd_soc_dapm_context *dapm,
				  enum snd_soc_bias_level level)
{
	struct exynos8895_drvdata *drvdata = card->drvdata;
	struct snd_soc_pcm_runtime *rtd;
	struct snd_soc_dai *codec_dai;
	struct snd_soc_codec *codec;
	int ret;

	rtd = snd_soc_get_pcm_runtime(card, card->dai_link[MADERA_DAI_OFFSET].name);
	codec_dai = rtd->codec_dai;
	codec = codec_dai->codec;

	if (dapm->dev != codec_dai->dev)
		return 0;

	switch (level) {
	case SND_SOC_BIAS_PREPARE:
		if (dapm->bias_level != SND_SOC_BIAS_STANDBY)
			break;

		ret = snd_soc_codec_set_pll(codec, drvdata->fll1_refclk.id,
					    drvdata->fll1_refclk.source,
					    drvdata->fll1_refclk.rate,
					    drvdata->sysclk.rate);
		if (ret < 0)
			dev_err(drvdata->dev, "Failed to start FLL: %d\n", ret);
		break;
	default:
		break;
	}

	return 0;
}

static int exynos8895_set_bias_level_post(struct snd_soc_card *card,
				       struct snd_soc_dapm_context *dapm,
				       enum snd_soc_bias_level level)
{
	struct exynos8895_drvdata *drvdata = card->drvdata;
	struct snd_soc_pcm_runtime *rtd;
	struct snd_soc_dai *codec_dai;
	struct snd_soc_codec *codec;
	int ret;

	rtd = snd_soc_get_pcm_runtime(card, card->dai_link[MADERA_DAI_OFFSET].name);
	codec_dai = rtd->codec_dai;
	codec = codec_dai->codec;

	if (dapm->dev != codec_dai->dev)
		return 0;

	switch (level) {
	case SND_SOC_BIAS_STANDBY:
		ret = snd_soc_codec_set_pll(codec, drvdata->fll1_refclk.id,
					    0, 0, 0);
		if (ret < 0) {
			dev_err(drvdata->dev, "Failed to stop FLL: %d\n", ret);
			return ret;
		}
		break;
	default:
		break;
	}

	return 0;
}

static int exynos8895_madera_notify(struct notifier_block *nb,
				   unsigned long event, void *data)
{
	const struct madera_hpdet_notify_data *hp_inf;
	const struct madera_micdet_notify_data *md_inf;
	const struct madera_voice_trigger_info *vt_inf;
	const struct exynos8895_drvdata *drvdata =
		container_of(nb, struct exynos8895_drvdata, nb);

	switch (event) {
	case MADERA_NOTIFY_VOICE_TRIGGER:
		vt_inf = data;
		dev_info(drvdata->dev, "Voice Triggered (core_num=%d)\n",
			 vt_inf->core_num);
		++voice_trigger_count;
		break;
	case MADERA_NOTIFY_HPDET:
		hp_inf = data;
		dev_info(drvdata->dev, "HPDET val=%d.%02d ohms\n",
			 hp_inf->impedance_x100 / 100,
			 hp_inf->impedance_x100 % 100);
		break;
	case MADERA_NOTIFY_MICDET:
		md_inf = data;
		dev_info(drvdata->dev, "MICDET present=%c val=%d.%02d ohms\n",
			 md_inf->present ? 'Y' : 'N',
			 md_inf->impedance_x100 / 100,
			 md_inf->impedance_x100 % 100);
		break;
	default:
		dev_info(drvdata->dev, "notifier event=0x%lx data=0x%p\n",
			 event, data);
		break;
	}

	return NOTIFY_DONE;
}

#ifdef CONFIG_DEBUG_FS
static void exynos8895_init_debugfs(struct snd_soc_card *card)
{
	struct dentry *root;

	if (!card->debugfs_card_root) {
		dev_warn(card->dev, "No card debugfs root\n");
		return;
	}

	root = debugfs_create_dir("test-automation", card->debugfs_card_root);
	if (!root) {
		dev_warn(card->dev, "Failed to create debugfs dir\n");
		return;
	}

	debugfs_create_u32("voice_trigger_count", S_IRUGO, root,
			   &voice_trigger_count);
}
#else
static void arndale_init_debugfs(struct snd_soc_card *card)
{
}
#endif

static int exynos8895_late_probe(struct snd_soc_card *card)
{
	struct exynos8895_drvdata *drvdata = card->drvdata;
	struct snd_soc_pcm_runtime *rtd;
	struct snd_soc_dai *aif1_dai;
	struct snd_soc_codec *codec;
	struct snd_soc_component *cpu;
	int ret;

	rtd = snd_soc_get_pcm_runtime(card, card->dai_link[0].name);
	aif1_dai = rtd->cpu_dai;
	cpu = aif1_dai->component;

	rtd = snd_soc_get_pcm_runtime(card, card->dai_link[MADERA_DAI_OFFSET].name);
	aif1_dai = rtd->codec_dai;
	codec = aif1_dai->codec;

	ret = snd_soc_codec_set_sysclk(codec, drvdata->sysclk.id,
				       drvdata->sysclk.source,
				       drvdata->sysclk.rate,
				       SND_SOC_CLOCK_IN);
	if (ret != 0) {
		dev_err(drvdata->dev, "Failed to set SYSCLK: %d\n", ret);
		return ret;
	}

	ret = snd_soc_codec_set_sysclk(codec, drvdata->dspclk.id,
				       drvdata->dspclk.source,
				       drvdata->dspclk.rate,
				       SND_SOC_CLOCK_IN);
	if (ret != 0) {
		dev_err(drvdata->dev, "Failed to set SYSCLK: %d\n", ret);
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(aif1_dai, drvdata->sysclk.id, 0, 0);
	if (ret != 0) {
		dev_err(drvdata->dev, "Failed to set AIF1 clock: %d\n", ret);
		return ret;
	}

	snd_soc_dapm_ignore_suspend(&card->dapm, "VOUTPUT");
	snd_soc_dapm_ignore_suspend(&card->dapm, "VINPUT1");
	snd_soc_dapm_ignore_suspend(&card->dapm, "VINPUT2");
	snd_soc_dapm_ignore_suspend(&card->dapm, "VOUTPUTCALL");
	snd_soc_dapm_ignore_suspend(&card->dapm, "VINPUTCALL");
	snd_soc_dapm_ignore_suspend(&card->dapm, "HEADSETMIC");
	snd_soc_dapm_ignore_suspend(&card->dapm, "RECEIVER");
	snd_soc_dapm_ignore_suspend(&card->dapm, "HEADPHONE");
	snd_soc_dapm_ignore_suspend(&card->dapm, "SPEAKER");
	snd_soc_dapm_ignore_suspend(&card->dapm, "BLUETOOTH MIC");
	snd_soc_dapm_ignore_suspend(&card->dapm, "BLUETOOTH SPK");
	snd_soc_dapm_ignore_suspend(&card->dapm, "DMIC1");
	snd_soc_dapm_ignore_suspend(&card->dapm, "DMIC2");
	snd_soc_dapm_ignore_suspend(&card->dapm, "VTS Virtual Output");
	snd_soc_dapm_sync(&card->dapm);

	snd_soc_dapm_ignore_suspend(snd_soc_codec_get_dapm(codec), "AIF1 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_codec_get_dapm(codec), "AIF1 Capture");
	snd_soc_dapm_ignore_suspend(snd_soc_codec_get_dapm(codec), "AUXPDM1");
	snd_soc_dapm_sync(snd_soc_codec_get_dapm(codec));

	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA0 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA1 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA2 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA3 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA4 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA5 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA6 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA7 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX WDMA0 Capture");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX WDMA1 Capture");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX WDMA2 Capture");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX WDMA3 Capture");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX WDMA4 Capture");
	snd_soc_dapm_sync(snd_soc_component_get_dapm(cpu));

	exynos8895_init_debugfs(card);

	drvdata->nb.notifier_call = exynos8895_madera_notify;
	madera_register_notifier(codec, &drvdata->nb);

	return 0;
}

static struct snd_soc_dai_link exynos8895_dai[] = {
	{
		.name = "RDMA0",
		.stream_name = "RDMA0",
		.cpu_dai_name = "RDMA0",
		.platform_name = "13e51000.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA1",
		.stream_name = "RDMA1",
		.cpu_dai_name = "RDMA1",
		.platform_name = "13e51100.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA2",
		.stream_name = "RDMA2",
		.cpu_dai_name = "RDMA2",
		.platform_name = "13e51200.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA3",
		.stream_name = "RDMA3",
		.cpu_dai_name = "RDMA3",
		.platform_name = "13e51300.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA4",
		.stream_name = "RDMA4",
		.cpu_dai_name = "RDMA4",
		.platform_name = "13e51400.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA5",
		.stream_name = "RDMA5",
		.cpu_dai_name = "RDMA5",
		.platform_name = "13e51500.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA6",
		.stream_name = "RDMA6",
		.cpu_dai_name = "RDMA6",
		.platform_name = "13e51600.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA7",
		.stream_name = "RDMA7",
		.cpu_dai_name = "RDMA7",
		.platform_name = "13e51700.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "WDMA0",
		.stream_name = "WDMA0",
		.cpu_dai_name = "WDMA0",
		.platform_name = "13e52000.abox_wdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
	},
	{
		.name = "WDMA1",
		.stream_name = "WDMA1",
		.cpu_dai_name = "WDMA1",
		.platform_name = "13e52100.abox_wdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
	},
	{
		.name = "WDMA2",
		.stream_name = "WDMA2",
		.cpu_dai_name = "WDMA2",
		.platform_name = "13e52200.abox_wdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
	},
	{
		.name = "WDMA3",
		.stream_name = "WDMA3",
		.cpu_dai_name = "WDMA3",
		.platform_name = "13e52300.abox_wdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
	},
	{
		.name = "WDMA4",
		.stream_name = "WDMA4",
		.cpu_dai_name = "WDMA4",
		.platform_name = "13e52400.abox_wdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
	},
	{
		.name = "UAIF0",
		.stream_name = "UAIF0",
		.cpu_dai_name = "UAIF0",
		.platform_name = "snd-soc-dummy",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.ops = &uaif_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "UAIF1",
		.stream_name = "UAIF1",
		.cpu_dai_name = "UAIF1",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.ops = &uaif_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "UAIF2",
		.stream_name = "UAIF2",
		.cpu_dai_name = "UAIF2",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.ops = &uaif_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "UAIF3",
		.stream_name = "UAIF3",
		.cpu_dai_name = "UAIF3",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.ops = &uaif_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "UAIF4",
		.stream_name = "UAIF4",
		.cpu_dai_name = "UAIF4",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.ops = &uaif_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "DSIF",
		.stream_name = "DSIF",
		.cpu_dai_name = "DSIF",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_PDM | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.ops = &dsif_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "ABOX Internal",
		.stream_name = "ABOX Internal",
		.cpu_dai_name = "Internal",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_PDM | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "VTS-Trigger",
		.stream_name = "VTS-Trigger",
		.cpu_dai_name = "vts-tri",
		.platform_name = "vts_dma0",
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
		.platform_name = "vts_dma1",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.capture_only = true,
	},
	{
		.name = "DP Audio",
		.stream_name = "DP Audio",
		.cpu_dai_name = "snd-soc-dummy-dai",
		.platform_name = "15a40000.displayport_adma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
	},
};

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

static const struct snd_kcontrol_new exynos8895_controls[] = {
	SOC_DAPM_PIN_SWITCH("DMIC1"),
	SOC_DAPM_PIN_SWITCH("DMIC2"),
};

static struct snd_soc_dapm_widget exynos8895_widgets[] = {
	SND_SOC_DAPM_OUTPUT("VOUTPUT"),
	SND_SOC_DAPM_INPUT("VINPUT1"),
	SND_SOC_DAPM_INPUT("VINPUT2"),
	SND_SOC_DAPM_OUTPUT("VOUTPUTCALL"),
	SND_SOC_DAPM_INPUT("VINPUTCALL"),
	SND_SOC_DAPM_MIC("DMIC1", NULL),
	SND_SOC_DAPM_MIC("DMIC2", NULL),
	SND_SOC_DAPM_MIC("HEADSETMIC", NULL),
	SND_SOC_DAPM_SPK("RECEIVER", NULL),
	SND_SOC_DAPM_HP("HEADPHONE", NULL),
	SND_SOC_DAPM_SPK("SPEAKER", NULL),
	SND_SOC_DAPM_MIC("BLUETOOTH MIC", NULL),
	SND_SOC_DAPM_SPK("BLUETOOTH SPK", NULL),
	SND_SOC_DAPM_OUTPUT("VTS Virtual Output"),
	SND_SOC_DAPM_MUX("VTS Virtual Output Mux", SND_SOC_NOPM, 0, 0,
                      &vts_output_mux[0]),
};

static struct snd_soc_codec_conf codec_conf[] = {
	{.name_prefix = "ABOX", },
	{.name_prefix = "ABOX", },
	{.name_prefix = "ABOX", },
	{.name_prefix = "ABOX", },
	{.name_prefix = "ABOX", },
	{.name_prefix = "ABOX", },
	{.name_prefix = "ABOX", },
	{.name_prefix = "VTS", },
};

static struct snd_soc_aux_dev aux_dev[] = {
	{
		.name = "EFFECT",
	},
};

static struct snd_soc_card exynos8895 = {
	.name = "Exynos8895-Madera",
	.owner = THIS_MODULE,
	.dai_link = exynos8895_dai,
	.num_links = ARRAY_SIZE(exynos8895_dai),

	.late_probe = exynos8895_late_probe,

	.controls = exynos8895_controls,
	.num_controls = ARRAY_SIZE(exynos8895_controls),
	.dapm_widgets = exynos8895_widgets,
	.num_dapm_widgets = ARRAY_SIZE(exynos8895_widgets),

	.set_bias_level = exynos8895_set_bias_level,
	.set_bias_level_post = exynos8895_set_bias_level_post,

	.drvdata = (void *)&exynos8895_drvdata,

	.codec_conf = codec_conf,
	.num_configs = ARRAY_SIZE(codec_conf),

	.aux_dev = aux_dev,
	.num_aux_devs = ARRAY_SIZE(aux_dev),
};

static int exynos8895_read_clk_conf(struct device_node *np,
				   const char * const prop,
				   struct clk_conf *conf)
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

	conf->source = tmp;
	conf->source--;

	ret = of_property_read_u32_index(np, prop, 2, &tmp);
	if (ret)
		return ret;

	conf->rate = tmp;

	return 0;
}

static int exynos8895_read_dai(struct device_node *np, const char * const prop,
			      struct device_node **dai, const char **name)
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

	if (*name == NULL) {
		/* Ignoring the return as we don't register DAIs to the platform */
		ret = snd_soc_of_get_dai_name(np, name);
		if (ret && !*name)
			return ret;
	}

out:
	of_node_put(np);

	return ret;
}

static struct clk *xclkout;

static void control_xclkout(bool on)
{
	if (on) {
		clk_enable(xclkout);
	} else {
		clk_disable(xclkout);
	}
}

static int exynos8895_audio_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &exynos8895;
	struct exynos8895_drvdata *drvdata = card->drvdata;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *dai;
	int nlink = 0, n;
	int ret;

	card->dev = &pdev->dev;
	drvdata->dev = card->dev;

	xclkout = devm_clk_get(&pdev->dev, "xclkout");
	if (IS_ERR(xclkout)) {
		dev_err(&pdev->dev, "xclkout get failed\n");
		return PTR_ERR(xclkout);
	}

	ret = clk_prepare(xclkout);
	if (ret < 0) {
		dev_err(&pdev->dev, "xclkout prepare failed\n");
		return ret;
	}

	ret = exynos8895_read_clk_conf(np, "cirrus,sysclk", &drvdata->sysclk);
	if (ret) {
		dev_err(card->dev, "Failed to parse sysclk: %d\n", ret);
		return ret;
	}
	ret = exynos8895_read_clk_conf(np, "cirrus,dspclk", &drvdata->dspclk);
	if (ret) {
		dev_err(card->dev, "Failed to parse dspclk: %d\n", ret);
		return ret;
	}
	ret = exynos8895_read_clk_conf(np, "cirrus,fll1-refclk",
				      &drvdata->fll1_refclk);
	if (ret) {
		dev_err(card->dev, "Failed to parse fll1-refclk: %d\n", ret);
		return ret;
	}

	for_each_child_of_node(np, dai) {
		ret = exynos8895_read_dai(dai, "cpu",
					 &exynos8895_dai[nlink].cpu_of_node,
					 &exynos8895_dai[nlink].cpu_dai_name);
		if (ret) {
			dev_warn(card->dev,
				"Failed to parse cpu DAI for %s: %d\n",
				dai->name, ret);
			return ret;
		}

		if (!exynos8895_dai[nlink].platform_name) {
			ret = exynos8895_read_dai(dai, "platform",
						 &exynos8895_dai[nlink].platform_of_node,
						 &exynos8895_dai[nlink].platform_name);
		}

		if (!exynos8895_dai[nlink].codec_name) {
			ret = exynos8895_read_dai(dai, "codec",
						 &exynos8895_dai[nlink].codec_of_node,
						 &exynos8895_dai[nlink].codec_dai_name);
			if (ret) {
				dev_warn(card->dev,
					"Failed to parse codec DAI for %s: %d\n",
					dai->name, ret);
				return ret;
			}
		}

		if (++nlink == card->num_links)
			break;
	}

	if (!nlink) {
		dev_warn(card->dev, "No DAIs specified\n");
	}

	if (of_property_read_bool(np, "samsung,routing")) {
		ret = snd_soc_of_parse_audio_routing(card, "samsung,routing");
		if (ret)
			return ret;
	}

	for (n = 0; n < ARRAY_SIZE(codec_conf); n++) {
		codec_conf[n].of_node = of_parse_phandle(np, "samsung,codec", n);

		if (!codec_conf[n].of_node) {
			dev_err(&pdev->dev,
				"Property 'samsung,codec' missing\n");
			return -EINVAL;
		}
	}

	for (n = 0; n < ARRAY_SIZE(aux_dev); n++) {
		aux_dev[n].codec_of_node = of_parse_phandle(np, "samsung,aux", n);

		if (!aux_dev[n].codec_of_node) {
			dev_err(&pdev->dev,
				"Property 'samsung,aux' missing\n");
			return -EINVAL;
		}
	}

	control_xclkout(true);

	ret = devm_snd_soc_register_card(card->dev, card);
	if (ret)
		dev_err(card->dev, "snd_soc_register_card() failed:%d\n", ret);

	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id exynos8895_of_match[] = {
	{ .compatible = "samsung,exynos8895-madera", },
	{},
};
MODULE_DEVICE_TABLE(of, exynos8895_of_match);
#endif /* CONFIG_OF */

static struct platform_driver exynos8895_audio_driver = {
	.driver		= {
		.name	= "exynos8895-madera",
		.owner	= THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = of_match_ptr(exynos8895_of_match),
	},

	.probe		= exynos8895_audio_probe,
};

module_platform_driver(exynos8895_audio_driver);

MODULE_DESCRIPTION("ALSA SoC Exynos8895 Madera Driver");
MODULE_AUTHOR("Charles Keepax <ckeepax@opensource.wolfsonmicro.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:exynos8895-madera");
