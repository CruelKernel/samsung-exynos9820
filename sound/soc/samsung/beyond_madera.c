/*
 *  Sound Machine Driver for Madera CODECs on Beyond
 *
 *  Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/sec_debug.h>

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
#include <linux/extcon/extcon-madera.h>
#include "../codecs/madera.h"
#include "abox/abox_soc.h"

#include "jack_madera_sysfs_cb.h"

#define UAIF_START		24

#define UAIF0_DAI_ID		(1 << UAIF_START)
#define UAIF1_DAI_ID		(1 << (UAIF_START + 1))
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

#define GPIO_AUX_FN_1		0x2280
#define GPIO_AUX_FN_2		0x1000
#define GPIO_AUX_FN_3		0x2281
#define GPIO_AUX_FN_4		0x1000

#define GPIO_HIZ_FN_1		0x2001
#define GPIO_HIZ_FN_2		0x9000

#define GPIO_AUXPDM_MASK_1	0x0fff
#define GPIO_AUXPDM_MASK_2	0xf000

struct clk_conf {
	int id;
	int source;
	int rate;
};

struct beyond_drvdata {
	struct clk_conf fll1_refclk;
	struct clk_conf fll2_refclk;
	struct clk_conf sysclk;
	struct clk_conf asyncclk;
	struct clk_conf dspclk;
	struct clk_conf outclk;
	struct clk_conf ampclk;
	struct notifier_block nb;
	struct notifier_block panic_nb;
	struct wake_lock wake_lock;
	unsigned int wake_lock_switch;
	unsigned int hp_impedance_step;
	unsigned int hiz_val;
	unsigned int fm_mute_switch;
	unsigned int vss_state;
	unsigned int pcm_state;
};

struct gain_table {
	int min; /* min impedance */
	int max; /* max impedance */
	unsigned int gain; /* offset value added to current register val */
};

static struct impedance_table {
	struct gain_table hp_gain_table[8];
	char imp_region[4];	/* impedance region */
} imp_table = {
	.hp_gain_table = {
		{    0,      13,  0 },
		{   14,      25,  5 },
		{   26,      42,  7 },
		{   43,     100, 14 },
		{  101,     200, 18 },
		{  201,     450, 20 },
		{  451,    1000, 20 },
		{ 1001, INT_MAX, 16 },
	},
};

static struct beyond_drvdata beyond_drvdata_info;
static struct snd_soc_card beyond_madera;
static struct clk *xclkout;

#define DEBUG_LEVEL_LOW 0

static int beyond_madera_panic_cb(struct notifier_block *nb,
					unsigned long event, void *data)
{
	return NOTIFY_OK;
}

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

static int beyond_dai_ops_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct beyond_drvdata *drvdata = card->drvdata;
	struct snd_soc_dai_link *dai_link = rtd->dai_link;
	unsigned int rate = params_rate(params);
	int ret = 0;

	if (dai_link->id)
		drvdata->pcm_state |= (dai_link->id);

	dev_info(card->dev,
		"%s-%d: 0x%x: hw_params: %dch, %dHz, %dbytes, %dbit (pcm_state: 0x%07X)\n",
		rtd->dai_link->name, substream->stream, dai_link->id,
		params_channels(params), params_rate(params),
		params_buffer_bytes(params), params_width(params),
		drvdata->pcm_state);

	if (dai_link->id == UAIF0_DAI_ID) {
		drvdata->fll2_refclk.rate = rate * params_width(params) * 2;

		if (rate % SYSCLK_RATE_CHECK_PARAM)
			drvdata->asyncclk.rate = SYSCLK_RATE_44_1KHZ;
		else
			drvdata->asyncclk.rate = SYSCLK_RATE_48KHZ;
	}

	return ret;
}

static int beyond_dai_ops_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct beyond_drvdata *drvdata = card->drvdata;
	struct snd_soc_dai_link *dai_link = rtd->dai_link;
	int ret = 0;

	if (dai_link->id)
		drvdata->pcm_state &= (~dai_link->id);

	dev_info(card->dev, "%s: %s-%d (pcm_state: 0x%07X)\n",
			__func__, rtd->dai_link->name, substream->stream,
			drvdata->pcm_state);

	return ret;
}

static int beyond_dai_ops_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_codec *codec = rtd->codec_dai->codec;
	struct snd_soc_dai_link *dai_link = rtd->dai_link;
	struct beyond_drvdata *drvdata = card->drvdata;
	int ret = 0;

	dev_info(card->dev, "%s-%d: 0x%x: prepare\n",
			rtd->dai_link->name, substream->stream, dai_link->id);

	/* enable bclk */
	if ((dai_link->id == UAIF0_DAI_ID) || (dai_link->id == UAIF1_DAI_ID))
		snd_soc_dai_set_tristate(rtd->cpu_dai, 0);

	if (dai_link->id == UAIF0_DAI_ID) {
		dev_info(card->dev, "%s: Start ASYNCCLK\n", __func__);

		codec = rtd->codec_dai->codec;

		ret = snd_soc_codec_set_sysclk(codec, drvdata->asyncclk.id,
						drvdata->asyncclk.source,
						drvdata->asyncclk.rate,
						SND_SOC_CLOCK_IN);
		if (ret < 0)
			dev_err(card->dev, "Fail to start ASYNCCLK: %d\n", ret);

		ret = snd_soc_codec_set_pll(codec, drvdata->fll2_refclk.id,
						drvdata->fll2_refclk.source,
						drvdata->fll2_refclk.rate,
						drvdata->asyncclk.rate);
		if (ret < 0)
			dev_err(card->dev, "Fail to start FLL2 REF: %d\n", ret);
	}

	return ret;
}

static void beyond_dai_ops_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_codec *codec = rtd->codec_dai->codec;
	struct snd_soc_dai_link *dai_link = rtd->dai_link;
	struct beyond_drvdata *drvdata = card->drvdata;
	int ret;

	codec = rtd->codec_dai->codec;

	dev_info(card->dev,
		"%s-%d: 0x%x: shutdown: play: %d, cap: %d, active: %d\n",
		rtd->dai_link->name, substream->stream, dai_link->id,
		codec_dai->playback_active, codec_dai->capture_active,
		codec->component.active);

	if ((dai_link->id == UAIF0_DAI_ID) &&
		!codec_dai->playback_active &&
		!codec_dai->capture_active &&
		!codec->component.active
	) {
		dev_info(card->dev, "%s: Stop ASYNCCLK\n", __func__);

		snd_soc_update_bits(codec, MADERA_OUTPUT_ENABLES_1,
					MADERA_OUT2L_ENA | MADERA_OUT2R_ENA, 0);

		usleep_range(1000, 1100);

		ret = snd_soc_codec_set_sysclk(codec, drvdata->asyncclk.id,
						0, 0, 0);
		if (ret != 0)
			dev_err(card->dev, "Fail to stop ASYNCCLK: %d\n", ret);

		ret = snd_soc_codec_set_pll(codec, drvdata->fll2_refclk.id,
						0, 0, 0);
		if (ret < 0)
			dev_err(card->dev, "Fail to stop FLL2 REF: %d\n", ret);
	}

	if ((dai_link->id == UAIF0_DAI_ID) || (dai_link->id == UAIF1_DAI_ID))
		snd_soc_dai_set_tristate(rtd->cpu_dai, 1);

}

static const struct snd_soc_ops rdma_ops = {
	.hw_params = beyond_dai_ops_hw_params,
	.hw_free = beyond_dai_ops_hw_free,
	.prepare = beyond_dai_ops_prepare,
	.shutdown = beyond_dai_ops_shutdown,
};

static const struct snd_soc_ops wdma_ops = {
	.hw_params = beyond_dai_ops_hw_params,
	.hw_free = beyond_dai_ops_hw_free,
	.prepare = beyond_dai_ops_prepare,
	.shutdown = beyond_dai_ops_shutdown,
};

static const struct snd_soc_ops uaif_ops = {
	.hw_params = beyond_dai_ops_hw_params,
	.hw_free = beyond_dai_ops_hw_free,
	.prepare = beyond_dai_ops_prepare,
	.shutdown = beyond_dai_ops_shutdown,
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

static int beyond_set_bias_level(struct snd_soc_card *card,
				struct snd_soc_dapm_context *dapm,
				enum snd_soc_bias_level level)
{
	struct snd_soc_dai *codec_dai;
	struct snd_soc_codec *codec;
	struct beyond_drvdata *drvdata = card->drvdata;
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

		dev_info(card->dev, "%s: Start SYSCLK\n", __func__);

		clk_enable(xclkout);

		ret = snd_soc_codec_set_sysclk(codec, drvdata->sysclk.id,
						drvdata->sysclk.source,
						drvdata->sysclk.rate,
						SND_SOC_CLOCK_IN);
		if (ret < 0)
			dev_err(card->dev, "Fail to start SYSCLK: %d\n", ret);

		ret = snd_soc_codec_set_pll(codec, drvdata->fll1_refclk.id,
						drvdata->fll1_refclk.source,
						drvdata->fll1_refclk.rate,
						drvdata->sysclk.rate);
		if (ret < 0)
			dev_err(card->dev, "Fail to start FLL1 REF: %d\n", ret);
		break;
	case SND_SOC_BIAS_OFF:
		dev_info(card->dev, "%s: Stop SYSCLK\n", __func__);

		ret = snd_soc_codec_set_sysclk(codec, drvdata->sysclk.id,
						0, 0, 0);
		if (ret != 0)
			dev_err(card->dev, "Fail to stop SYSCLK: %d\n", ret);

		ret = snd_soc_codec_set_pll(codec, drvdata->fll1_refclk.id,
						0, 0, 0);
		if (ret < 0)
			dev_err(card->dev, "Fail to stop FLL1 REF: %d\n", ret);

		clk_disable(xclkout);

		break;
	default:
		break;
	}

	return 0;
}

static void beyond_madera_hpdet_cb(unsigned int meas)
{
	struct snd_soc_card *card = &beyond_madera;
	struct beyond_drvdata *drvdata = card->drvdata;
	int jack_det, i, min, max, num_hp_gain_table;

	if (meas == (INT_MAX / 100))
		jack_det = 0;
	else
		jack_det = 1;

	madera_jack_det = jack_det;

	dev_info(card->dev, "%s(%d) meas(%d)\n", __func__, jack_det, meas);

	num_hp_gain_table = (int)ARRAY_SIZE(imp_table.hp_gain_table);
	for (i = 0; i < num_hp_gain_table; i++) {
		min = imp_table.hp_gain_table[i].min;
		max = imp_table.hp_gain_table[i].max;

		if ((meas < min) || (meas > max))
			continue;

		dev_info(card->dev, "SET GAIN %d step for %d ohms\n",
					imp_table.hp_gain_table[i].gain, meas);

		drvdata->hp_impedance_step = imp_table.hp_gain_table[i].gain;
	}
}

static void beyond_madera_micd_cb(bool mic)
{
	struct snd_soc_card *card = &beyond_madera;

	madera_ear_mic = mic;

	dev_info(card->dev, "%s: madera_ear_mic = %d\n", __func__,
				madera_ear_mic);
}

static int beyond_madera_notify(struct notifier_block *nb, unsigned long event,
				void *data)
{
	const struct madera_hpdet_notify_data *hp_inf;
	const struct madera_micdet_notify_data *md_inf;
	struct snd_soc_card *card = &beyond_madera;

	switch (event) {
	case MADERA_NOTIFY_HPDET:
		hp_inf = data;
		beyond_madera_hpdet_cb(hp_inf->impedance_x100 / 100);
		dev_info(card->dev, "HPDET val=%d.%02d ohms\n",
				hp_inf->impedance_x100 / 100,
				hp_inf->impedance_x100 % 100);
		break;
	case MADERA_NOTIFY_MICDET:
		md_inf = data;
		beyond_madera_micd_cb(md_inf->present);
		dev_info(card->dev, "MICDET present=%c val=%d.%02d ohms\n",
				md_inf->present ? 'Y' : 'N',
				md_inf->impedance_x100 / 100,
				md_inf->impedance_x100 % 100);
		break;
	default:
		dev_info(card->dev, "notifier event=0x%lx data=0x%p\n",
				event, data);
		break;
	}

	return NOTIFY_DONE;
}

static int beyond_madera_put_impedance_volsw(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct snd_soc_card *card = codec->component.card;
	struct soc_mixer_control *mc =
			(struct soc_mixer_control *)kcontrol->private_value;
	struct beyond_drvdata *drvdata = card->drvdata;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int mask = (1 << fls(mc->max)) - 1;
	unsigned int invert = mc->invert;
	unsigned int val, val_mask;
	int max = mc->max;
	int ret;

	val = (unsigned int)(ucontrol->value.integer.value[0] & mask);
	val += drvdata->hp_impedance_step;
	dev_info(card->dev, "SET GAIN %d by impedance, move to %d step\n",
			val, drvdata->hp_impedance_step);

	if (invert)
		val = max - val;

	val_mask = mask << shift;
	val = val << shift;

	ret = snd_soc_update_bits(codec, reg, val_mask, val);
	if (ret < 0)
		dev_err(card->dev, "impedance volume update err %d\n", ret);

	return ret;
}

static int beyond_madera_get_auxpdm_hiz_mode(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct beyond_drvdata *drvdata = codec->component.card->drvdata;

	ucontrol->value.integer.value[0] = drvdata->hiz_val;

	return 0;
}

static int beyond_madera_set_auxpdm_hiz_mode(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct snd_soc_card *card = codec->component.card;
	struct beyond_drvdata *drvdata = codec->component.card->drvdata;

	drvdata->hiz_val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(card->dev, "%s ev: %d\n", __func__, drvdata->hiz_val);

	if (drvdata->hiz_val) {
		snd_soc_update_bits(codec, MADERA_GPIO10_CTRL_1,
					GPIO_AUXPDM_MASK_1, GPIO_AUX_FN_1);
		snd_soc_update_bits(codec, MADERA_GPIO10_CTRL_2,
					GPIO_AUXPDM_MASK_2, GPIO_AUX_FN_2);
		snd_soc_update_bits(codec, MADERA_GPIO9_CTRL_1,
					GPIO_AUXPDM_MASK_1, GPIO_AUX_FN_3);
		snd_soc_update_bits(codec, MADERA_GPIO9_CTRL_2,
					GPIO_AUXPDM_MASK_2, GPIO_AUX_FN_4);
	} else {
		snd_soc_update_bits(codec, MADERA_GPIO10_CTRL_1,
					GPIO_AUXPDM_MASK_1, GPIO_HIZ_FN_1);
		snd_soc_update_bits(codec, MADERA_GPIO10_CTRL_2,
					GPIO_AUXPDM_MASK_2, GPIO_HIZ_FN_2);
		snd_soc_update_bits(codec, MADERA_GPIO9_CTRL_1,
					GPIO_AUXPDM_MASK_1, GPIO_HIZ_FN_1);
		snd_soc_update_bits(codec, MADERA_GPIO9_CTRL_2,
					GPIO_AUXPDM_MASK_2, GPIO_HIZ_FN_2);
	}

	return 0;
}

static int beyond_madera_get_fm_mute_switch(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct snd_soc_card *card = codec->component.card;
	struct beyond_drvdata *drvdata = card->drvdata;

	ucontrol->value.integer.value[0] = drvdata->fm_mute_switch;

	return 0;
}

static int beyond_madera_set_fm_mute_switch(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct snd_soc_card *card = codec->component.card;
	struct beyond_drvdata *drvdata = card->drvdata;
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

static const struct snd_kcontrol_new beyond_madera_codec_controls[] = {
	SOC_SINGLE_EXT_TLV("HPOUT2L Impedance Volume",
			MADERA_DAC_DIGITAL_VOLUME_2L,
			MADERA_OUT2L_VOL_SHIFT, 0xbf, 0,
			snd_soc_get_volsw, beyond_madera_put_impedance_volsw,
			madera_digital_tlv),

	SOC_SINGLE_EXT_TLV("HPOUT2R Impedance Volume",
			MADERA_DAC_DIGITAL_VOLUME_2R,
			MADERA_OUT2L_VOL_SHIFT, 0xbf, 0,
			snd_soc_get_volsw, beyond_madera_put_impedance_volsw,
			madera_digital_tlv),

	SOC_SINGLE_BOOL_EXT("AUXPDM Switch", 0,
			beyond_madera_get_auxpdm_hiz_mode,
			beyond_madera_set_auxpdm_hiz_mode),

	SOC_SINGLE_BOOL_EXT("FM Mute Switch", 0,
			beyond_madera_get_fm_mute_switch,
			beyond_madera_set_fm_mute_switch),
};

static int beyond_uaif0_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);
	struct beyond_drvdata *drvdata = snd_soc_card_get_drvdata(card);
	int ret;

	dev_info(card->dev, "%s\n", __func__);

	ret = snd_soc_add_codec_controls(codec, beyond_madera_codec_controls,
				ARRAY_SIZE(beyond_madera_codec_controls));
	if (ret < 0) {
		dev_err(card->dev, "Fail to add controls to codec: %d\n", ret);
		return ret;
	}

	ret = snd_soc_codec_set_sysclk(codec, drvdata->sysclk.id,
					drvdata->sysclk.source,
					drvdata->sysclk.rate,
					SND_SOC_CLOCK_IN);
	if (ret != 0) {
		dev_err(card->dev, "Fail to set SYSCLK: %d\n", ret);
		return ret;
	}

	ret = snd_soc_codec_set_sysclk(codec, drvdata->asyncclk.id,
					drvdata->asyncclk.source,
					drvdata->asyncclk.rate,
					SND_SOC_CLOCK_IN);
	if (ret != 0) {
		dev_err(card->dev, "Fail to set ASYNCCLK: %d\n", ret);
		return ret;
	}

	ret = snd_soc_codec_set_sysclk(codec, drvdata->dspclk.id,
					drvdata->dspclk.source,
					drvdata->dspclk.rate,
					SND_SOC_CLOCK_IN);
	if (ret != 0) {
		dev_err(card->dev, "Fail to set DSPCLK: %d\n", ret);
		return ret;
	}

	ret = snd_soc_codec_set_sysclk(codec, drvdata->outclk.id,
					drvdata->outclk.source,
					drvdata->outclk.rate,
					SND_SOC_CLOCK_IN);
	if (ret != 0) {
		dev_err(card->dev, "Fail to set OUTCLK: %d\n", ret);
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, drvdata->asyncclk.id, 0, 0);
	if (ret != 0) {
		dev_err(card->dev, "Fail to set AIF1 dai clock: %d\n", ret);
		return ret;
	}

	snd_soc_dapm_ignore_suspend(dapm, "AIF1 Playback");
	snd_soc_dapm_ignore_suspend(dapm, "AIF1 Capture");
	snd_soc_dapm_ignore_suspend(dapm, "AUXPDM1");
	snd_soc_dapm_sync(dapm);

	drvdata->nb.notifier_call = beyond_madera_notify;
	madera_register_notifier(codec, &drvdata->nb);

	register_madera_jack_cb(codec);

	return 0;
}

static int beyond_uaif1_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai;
	struct snd_soc_dapm_context *dapm;
	struct beyond_drvdata *drvdata = snd_soc_card_get_drvdata(card);
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
		if (ret != 0) {
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

static int beyond_uaif2_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);
	struct beyond_drvdata *drvdata = snd_soc_card_get_drvdata(card);
	int ret = 0;

	dev_info(card->dev, "%s\n", __func__);

	ret = snd_soc_dai_set_sysclk(codec_dai, drvdata->sysclk.id, 0, 0);
	if (ret != 0) {
		dev_err(card->dev, "Fail to set AIF3 dai clock: %d\n", ret);
		return ret;
	}

	snd_soc_dapm_ignore_suspend(dapm, "AIF3 Playback");
	snd_soc_dapm_ignore_suspend(dapm, "AIF3 Capture");
	snd_soc_dapm_sync(dapm);

	return 0;
}

static int beyond_late_probe(struct snd_soc_card *card)
{
	struct snd_soc_dai *aif_dai;
	struct snd_soc_component *cpu;
	struct snd_soc_dapm_context *dapm;
	struct beyond_drvdata *drvdata = card->drvdata;
	char name[SZ_32];
	const char *prefix;
	int i;

	dev_info(card->dev, "%s\n", __func__);

	snd_soc_dapm_ignore_suspend(&card->dapm, "DMIC1");
	snd_soc_dapm_ignore_suspend(&card->dapm, "DMIC2");
	snd_soc_dapm_ignore_suspend(&card->dapm, "HEADSETMIC");
	snd_soc_dapm_ignore_suspend(&card->dapm, "BLUETOOTH MIC");
	snd_soc_dapm_ignore_suspend(&card->dapm, "FM IN");
	snd_soc_dapm_ignore_suspend(&card->dapm, "RECEIVER");
	snd_soc_dapm_ignore_suspend(&card->dapm, "SPEAKER");
	snd_soc_dapm_ignore_suspend(&card->dapm, "HEADPHONE");
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

	drvdata->panic_nb.notifier_call = beyond_madera_panic_cb;

	atomic_notifier_chain_register(&panic_notifier_list,
				&drvdata->panic_nb);

	register_debug_mixer(card);

	return 0;
}

static struct snd_soc_dai_link beyond_dai[] = {
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
		.init = beyond_uaif0_init,
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
		.init = beyond_uaif1_init,
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
		.init = beyond_uaif2_init,
		.ops = &uaif_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.id = (1 << 26),
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
		.id = (1 << 27),
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
		.ops = &dsif_ops,
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

static int beyond_dmic1(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int beyond_dmic2(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int beyond_headset_mic(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d, ear_mic: %d\n", __func__, event,
			madera_ear_mic);

	return 0;
}

static int beyond_btsco_mic(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int beyond_fm_in(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int beyond_receiver(struct snd_soc_dapm_widget *w,
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

static int beyond_headphone(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d, jack_det: %d\n", __func__, event,
			madera_jack_det);

	return 0;
}

static int beyond_speaker(struct snd_soc_dapm_widget *w,
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

static int beyond_btsco(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int beyond_suspend_post(struct snd_soc_card *card)
{
	struct snd_soc_dai *aif_dai;
	struct snd_soc_codec *codec;
	struct beyond_drvdata *drvdata = card->drvdata;
	int ret = 0;

	aif_dai = get_rtd(card, UAIF0_DAI_ID)->codec_dai;
	if (aif_dai == NULL) {
		dev_err(card->dev, "%s: Fail to get aif_dai\n", __func__);
		return ret;
	}

	codec = aif_dai->codec;

	dev_info(card->dev, "%s component active %d earmic %d vts %d\n",
			__func__, codec->component.active, madera_ear_mic,
			vts_is_recognitionrunning());

	if (codec->component.active)
		return ret;

	if (madera_ear_mic) {
		if (!vts_is_recognitionrunning())
			madera_enable_force_bypass(codec);
	} else if (vts_is_recognitionrunning()) {
		madera_enable_force_bypass(codec);

		ret = snd_soc_codec_set_sysclk(codec, drvdata->sysclk.id,
						0, 0, 0);
		if (ret != 0)
			dev_err(card->dev, "Fail to stop SYSCLK: %d\n", ret);

		ret = snd_soc_codec_set_pll(codec, drvdata->fll1_refclk.id,
						0, 0, 0);
		if (ret < 0)
			dev_err(card->dev, "Fail to stop FLL1 REF: %d\n", ret);
	}

	return ret;
}

static int beyond_resume_pre(struct snd_soc_card *card)
{
	struct snd_soc_dai *aif_dai;
	struct snd_soc_codec *codec;
	struct beyond_drvdata *drvdata = card->drvdata;
	int ret = 0;

	aif_dai = get_rtd(card, UAIF0_DAI_ID)->codec_dai;
	if (aif_dai == NULL) {
		dev_err(card->dev, "%s: Fail to get aif_dai\n", __func__);
		return ret;
	}

	codec = aif_dai->codec;

	dev_info(card->dev, "%s component active %d earmic %d vts %d\n",
			__func__, codec->component.active, madera_ear_mic,
			vts_is_recognitionrunning());

	if (codec->component.active)
		return ret;

	if (madera_ear_mic) {
		if (!vts_is_recognitionrunning())
			madera_disable_force_bypass(codec);
	} else if (vts_is_recognitionrunning()) {
		ret = snd_soc_codec_set_sysclk(codec, drvdata->sysclk.id,
						drvdata->sysclk.source,
						drvdata->sysclk.rate,
						SND_SOC_CLOCK_IN);
		if (ret < 0)
			dev_err(card->dev, "Fail to start SYSCLK: %d\n", ret);

		ret = snd_soc_codec_set_pll(codec, drvdata->fll1_refclk.id,
						drvdata->fll1_refclk.source,
						drvdata->fll1_refclk.rate,
						drvdata->sysclk.rate);
		if (ret < 0)
			dev_err(card->dev, "Fail to start FLL1 REF: %d\n", ret);

		madera_disable_force_bypass(codec);
	}

	return ret;
}

static int beyond_get_sound_wakelock(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct beyond_drvdata *drvdata = &beyond_drvdata_info;

	ucontrol->value.integer.value[0] = drvdata->wake_lock_switch;

	return 0;
}

static int beyond_set_sound_wakelock(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	struct beyond_drvdata *drvdata = &beyond_drvdata_info;
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

static int beyond_vss_state_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct beyond_drvdata *drvdata = &beyond_drvdata_info;

	ucontrol->value.enumerated.item[0] = drvdata->vss_state;

	return 0;
}

static int beyond_vss_state_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	struct abox_data *data = abox_get_abox_data();
	struct beyond_drvdata *drvdata = &beyond_drvdata_info;
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

static const struct snd_kcontrol_new beyond_controls[] = {
	SOC_DAPM_PIN_SWITCH("DMIC1"),
	SOC_DAPM_PIN_SWITCH("DMIC2"),
	SOC_DAPM_PIN_SWITCH("FM IN"),

	SOC_SINGLE_BOOL_EXT("Sound Wakelock", 0,
			beyond_get_sound_wakelock, beyond_set_sound_wakelock),

	SOC_ENUM_EXT("ABOX VSS State", vss_state_enum,
			beyond_vss_state_get, beyond_vss_state_put),
};

static struct snd_soc_dapm_widget beyond_widgets[] = {
	SND_SOC_DAPM_MIC("DMIC1", beyond_dmic1),
	SND_SOC_DAPM_MIC("DMIC2", beyond_dmic2),
	SND_SOC_DAPM_MIC("HEADSETMIC", beyond_headset_mic),
	SND_SOC_DAPM_MIC("BLUETOOTH MIC", beyond_btsco_mic),
	SND_SOC_DAPM_MIC("FM IN", beyond_fm_in),
	SND_SOC_DAPM_SPK("RECEIVER", beyond_receiver),
	SND_SOC_DAPM_SPK("SPEAKER", beyond_speaker),
	SND_SOC_DAPM_HP("HEADPHONE", beyond_headphone),
	SND_SOC_DAPM_SPK("BLUETOOTH SPK", beyond_btsco),

	SND_SOC_DAPM_OUTPUT("VTS Virtual Output"),
	SND_SOC_DAPM_MUX("VTS Virtual Output Mux", SND_SOC_NOPM, 0, 0,
		&vts_output_mux[0]),
};

static struct snd_soc_dapm_route beyond_routes[] = {
	{"VTS Virtual Output Mux", "DMIC1", "DMIC1"},
};

static struct snd_soc_codec_conf codec_conf[MADERA_CODEC_MAX];

static struct snd_soc_aux_dev aux_dev[MADERA_AUX_MAX];

static struct snd_soc_card beyond_madera = {
	.name = "beyond-madera",
	.owner = THIS_MODULE,
	.dai_link = beyond_dai,
	.num_links = ARRAY_SIZE(beyond_dai),

	.late_probe = beyond_late_probe,

	.controls = beyond_controls,
	.num_controls = ARRAY_SIZE(beyond_controls),
	.dapm_widgets = beyond_widgets,
	.num_dapm_widgets = ARRAY_SIZE(beyond_widgets),
	.dapm_routes = beyond_routes,
	.num_dapm_routes = ARRAY_SIZE(beyond_routes),

	.set_bias_level = beyond_set_bias_level,

	.drvdata = (void *)&beyond_drvdata_info,

	.codec_conf = codec_conf,
	.num_configs = ARRAY_SIZE(codec_conf),

	.aux_dev = aux_dev,
	.num_aux_devs = ARRAY_SIZE(aux_dev),

	.suspend_post = beyond_suspend_post,
	.resume_pre = beyond_resume_pre,
};

static int read_clk_conf(struct device_node *np, const char * const prop,
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

	if (tmp < 0xffff)
		conf->source = tmp;
	else
		conf->source = -1;

	ret = of_property_read_u32_index(np, prop, 2, &tmp);
	if (ret)
		return ret;

	conf->rate = tmp;

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

static int beyond_audio_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &beyond_madera;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *dai;
	struct beyond_drvdata *drvdata = card->drvdata;
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

	ret = read_clk_conf(np, "cirrus,sysclk", &drvdata->sysclk);
	if (ret) {
		dev_err(card->dev, "Fail to parse sysclk: %d\n", ret);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,asyncclk", &drvdata->asyncclk);
	if (ret) {
		dev_err(card->dev, "Fail to parse asyncclk: %d\n", ret);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,dspclk", &drvdata->dspclk);
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

	ret = read_clk_conf(np, "cirrus,fll1-refclk", &drvdata->fll1_refclk);
	if (ret) {
		dev_err(card->dev, "Fail to parse fll1-refclk: %d\n", ret);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,fll2-refclk", &drvdata->fll2_refclk);
	if (ret) {
		dev_err(card->dev, "Fail to parse fll2-refclk: %d\n", ret);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,outclk", &drvdata->outclk);
	if (ret) {
		dev_err(card->dev, "Fail to parse outclk: %d\n", ret);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,ampclk", &drvdata->ampclk);
	if (ret) {
		dev_err(card->dev, "Fail to parse ampclk: %d\n", ret);
		goto err_clk_prepare;
	}

	for_each_child_of_node(np, dai) {
		struct snd_soc_dai_link *link = &beyond_dai[nlink];

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

	return ret;

err_clk_prepare:
	clk_unprepare(xclkout);
err_clk_get:
	devm_clk_put(&pdev->dev, xclkout);

	return ret;
}

static int beyond_audio_remove(struct platform_device *pdev)
{
	clk_unprepare(xclkout);
	devm_clk_put(&pdev->dev, xclkout);

	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id beyond_audio_of_match[] = {
	{ .compatible = "samsung,beyond-madera", },
	{},
};
MODULE_DEVICE_TABLE(of, beyond_audio_of_match);
#endif /* CONFIG_OF */

static struct platform_driver beyond_audio_driver = {
	.driver		= {
		.name	= "beyond-madera",
		.owner	= THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = of_match_ptr(beyond_audio_of_match),
	},

	.probe		= beyond_audio_probe,
	.remove		= beyond_audio_remove,
};

module_platform_driver(beyond_audio_driver);

MODULE_DESCRIPTION("ALSA SoC Beyond Audio Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:beyond-madera");
