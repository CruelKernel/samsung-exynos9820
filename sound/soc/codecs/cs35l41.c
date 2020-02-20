/*
 * cs35l41.c -- CS35l41 ALSA SoC audio driver
 *
 * Copyright 2017 Cirrus Logic, Inc.
 *
 * Author:	David Rhodes	<david.rhodes@cirrus.com>
 *		Brian Austin	<brian.austin@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio/consumer.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/gpio.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/cs35l41.h>
#include <linux/of_irq.h>
#include <linux/completion.h>
#include <linux/spi/spi.h>

#include <linux/mfd/cs35l41/core.h>
#include <linux/mfd/cs35l41/registers.h>
#include <linux/mfd/cs35l41/calibration.h>
#include <linux/mfd/cs35l41/power.h>
#include "wm_adsp.h"

#ifdef CS35L41_DEBUG_LOG
#define cs35l41_dbg(dev, fmt, ...) dev_info(dev, fmt, ##__VA_ARGS__)
#else
#define cs35l41_dbg(dev, fmt, ...) dev_dbg(dev, fmt, ##__VA_ARGS__)
#endif

struct cs35l41_private {
	struct wm_adsp dsp; /* needs to be first member */
	struct snd_soc_codec *codec;
	struct cs35l41_platform_data pdata;
	struct device *dev;
	struct regmap *regmap;
	struct regulator_bulk_data supplies[2];
	int num_supplies;
	int irq;
	int clksrc;
	int extclk_freq;
	int extclk_cfg;
	int sclk;
	int amp_mute;
	int pcm_vol;
	int dsprx2_src;
	int pcm_source_last;
	int pll_freq_last;
	unsigned int spk_3_trim;
	unsigned int spk_4_trim;
	bool tdm_mode;
	bool i2s_mode;
	bool swire_mode;
	bool halo_booted;
	bool halo_routed;
	bool halo_played;
	/* GPIO for /RST */
	struct gpio_desc *reset_gpio;
	struct completion global_pup_done;
	struct completion global_pdn_done;
	struct completion mbox_cmd;
	struct mutex rate_lock;
};

struct cs35l41_pll_sysclk_config {
	int freq;
	int clk_cfg;
};

static const struct cs35l41_pll_sysclk_config cs35l41_pll_sysclk[] = {
	{ 32768,	0x00 },
	{ 8000,		0x01 },
	{ 11025,	0x02 },
	{ 12000,	0x03 },
	{ 16000,	0x04 },
	{ 22050,	0x05 },
	{ 24000,	0x06 },
	{ 32000,	0x07 },
	{ 44100,	0x08 },
	{ 48000,	0x09 },
	{ 88200,	0x0A },
	{ 96000,	0x0B },
	{ 128000,	0x0C },
	{ 176400,	0x0D },
	{ 192000,	0x0E },
	{ 256000,	0x0F },
	{ 352800,	0x10 },
	{ 384000,	0x11 },
	{ 512000,	0x12 },
	{ 705600,	0x13 },
	{ 750000,	0x14 },
	{ 768000,	0x15 },
	{ 1000000,	0x16 },
	{ 1024000,	0x17 },
	{ 1200000,	0x18 },
	{ 1411200,	0x19 },
	{ 1500000,	0x1A },
	{ 1536000,	0x1B },
	{ 2000000,	0x1C },
	{ 2048000,	0x1D },
	{ 2400000,	0x1E },
	{ 2822400,	0x1F },
	{ 3000000,	0x20 },
	{ 3072000,	0x21 },
	{ 3200000,	0x22 },
	{ 4000000,	0x23 },
	{ 4096000,	0x24 },
	{ 4800000,	0x25 },
	{ 5644800,	0x26 },
	{ 6000000,	0x27 },
	{ 6144000,	0x28 },
	{ 6250000,	0x29 },
	{ 6400000,	0x2A },
	{ 6500000,	0x2B },
	{ 6750000,	0x2C },
	{ 7526400,	0x2D },
	{ 8000000,	0x2E },
	{ 8192000,	0x2F },
	{ 9600000,	0x30 },
	{ 11289600,	0x31 },
	{ 12000000,	0x32 },
	{ 12288000,	0x33 },
	{ 12500000,	0x34 },
	{ 12800000,	0x35 },
	{ 13000000,	0x36 },
	{ 13500000,	0x37 },
	{ 19200000,	0x38 },
	{ 22579200,	0x39 },
	{ 24000000,	0x3A },
	{ 24576000,	0x3B },
	{ 25000000,	0x3C },
	{ 25600000,	0x3D },
	{ 26000000,	0x3E },
	{ 27000000,	0x3F },
};

static void cs35l41_log_status(struct cs35l41_private *cs35l41, int mute)
{
	unsigned int status;

	if (!mute) {
		regmap_read(cs35l41->regmap,
			CS35L41_PWR_CTRL1, &status);
		dev_info(cs35l41->dev, "PWR_CTRL1 = 0x%x\n", status);

		regmap_read(cs35l41->regmap,
			CS35L41_PWR_CTRL2, &status);
		dev_info(cs35l41->dev, "PWR_CTRL2 = 0x%x\n", status);

		regmap_read(cs35l41->regmap,
			CS35L41_AMP_DIG_VOL_CTRL, &status);
		dev_info(cs35l41->dev, "DIG_VOL_CTRL = 0x%x\n", status);

		regmap_read(cs35l41->regmap,
			CS35L41_AMP_GAIN_CTRL, &status);
		dev_info(cs35l41->dev, "GAIN_CTRL = 0x%x\n", status);
	} else {
		regmap_read(cs35l41->regmap,
			CS35L41_IRQ1_STATUS1, &status);
		dev_info(cs35l41->dev, "IRQ1_STATUS1 = 0x%x\n", status);

		regmap_read(cs35l41->regmap,
			CS35L41_CSPL_MBOX_STS, &status);
		dev_info(cs35l41->dev, "MBOX status = 0x%x\n", status);

		regmap_read(cs35l41->regmap,
			CS35L41_HALO_STATE, &status);
		dev_info(cs35l41->dev, "HALO status = 0x%x\n", status);
	}
}

static int cs35l41_dsp_power_ev(struct snd_soc_dapm_widget *w,
		       struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (cs35l41->halo_booted == false)
			wm_halo_early_event(w, kcontrol, event);
		else
			cs35l41->dsp.booted = true;

		return 0;

	case SND_SOC_DAPM_POST_PMU:

		if (cs35l41->halo_booted == false) {
			wm_halo_event(w, kcontrol, event);
			dev_info(cs35l41->dev, "%s: loaded\n", __func__);
			cs35l41->halo_booted = true;
			cs35l41->halo_played = false;
		}

		if (cs35l41->pdata.right_channel)
			cirrus_cal_apply_right();
		else
			cirrus_cal_apply_left();

		return 0;
	case SND_SOC_DAPM_PRE_PMD:
		if (cs35l41->halo_booted == false) {
			wm_halo_early_event(w, kcontrol, event);
			wm_halo_event(w, kcontrol, event);
		}
	default:
		return 0;
	}
}

static int cs35l41_halo_booted_get(struct snd_kcontrol *kcontrol,
			   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = cs35l41->halo_booted;

	return 0;
}

static int cs35l41_halo_booted_put(struct snd_kcontrol *kcontrol,
			   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);

	cs35l41->halo_booted = ucontrol->value.integer.value[0];

	return 0;
}

static int cs35l41_pcm_vol_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = cs35l41->pcm_vol;

	return 0;
}

static int cs35l41_pcm_vol_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);

	dev_info(cs35l41->dev, "%s: 0x%lx\n", __func__,
	ucontrol->value.integer.value[0]);

	cs35l41->pcm_vol = ucontrol->value.integer.value[0];

	if (cs35l41->amp_mute == 1) { //Amp is unmuted
		snd_soc_put_volsw_sx(kcontrol, ucontrol);
	}

	return 0;
}

static int cs35l41_amp_mute_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = cs35l41->amp_mute;

	return 0;
}

static int cs35l41_amp_mute_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);
	unsigned int val;

	cs35l41->amp_mute = ucontrol->value.integer.value[0];

	if (cs35l41->amp_mute == 0) //Mute
		val = 0;
	else //Unmute
		val = cs35l41->pcm_vol;

	//convert control val to register val
	if (val < CS35L41_AMP_VOL_CTRL_DEFAULT)
		val += CS35L41_AMP_VOL_PCM_MUTE;
	else
		val -= CS35L41_AMP_VOL_CTRL_DEFAULT;

	regmap_update_bits(cs35l41->regmap, CS35L41_AMP_DIG_VOL_CTRL,
			CS35L41_AMP_VOL_PCM_MASK << CS35L41_AMP_VOL_PCM_SHIFT,
			val << CS35L41_AMP_VOL_PCM_SHIFT);

	dev_info(cs35l41->dev, "%s: %s\n", __func__,
		(cs35l41->amp_mute == 0) ? "Muted" : "Unmuted");

	return 0;
}

static int cs35l41_convert_ramp_rate(struct cs35l41_private *cs35l41,
	unsigned int ramp_rate)
{
	const unsigned int ramp_conv_table[] = {0, 1, 2, 4, 8, 16, 30, 60};

	/* Convert the ramp_rate register setting into a time delay in ms.
	 * This assumes the starting volume is 0 dB
	 */
	if (ramp_rate >= ARRAY_SIZE(ramp_conv_table)) {
		dev_err(cs35l41->dev,
			"Invalid rate (%d) to convert\n", ramp_rate);
		return -EINVAL;
	}

	return ramp_conv_table[ramp_rate] * CS35L41_AMP_VOL_MIN / 12;
}

static const char * const cs35l41_amp_mute_text[] = {"Muted", "Unmuted"};
static const unsigned int cs35l41_amp_mute_values[] = {1, 0};

static SOC_VALUE_ENUM_SINGLE_DECL(amp_mute_ctl,
				SND_SOC_NOPM,
				0,
				0,
				cs35l41_amp_mute_text,
				cs35l41_amp_mute_values);


static const DECLARE_TLV_DB_RANGE(dig_vol_tlv,
		0, 0, TLV_DB_SCALE_ITEM(TLV_DB_GAIN_MUTE, 0, 1),
		1, 913, TLV_DB_SCALE_ITEM(-10200, 25, 0));
static DECLARE_TLV_DB_SCALE(amp_gain_tlv, 0, 1, 1);

static const struct snd_kcontrol_new amp_enable_ctrl =
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 0);

static const struct snd_kcontrol_new dre_ctrl =
	SOC_DAPM_SINGLE("Switch", CS35L41_PWR_CTRL3, 20, 1, 0);

static const char * const cs35l41_pcm_sftramp_text[] =  {
	"Off", ".5ms", "1ms", "2ms", "4ms", "8ms", "15ms", "30ms"};

static SOC_ENUM_SINGLE_DECL(pcm_sft_ramp,
			    CS35L41_AMP_DIG_VOL_CTRL, 0,
			    cs35l41_pcm_sftramp_text);

static const char * const cs35l41_bst_en_text[] = {"Disabled", "Enabled"};
static const unsigned int cs35l41_bst_en_values[] = {
				CS35L41_BST_EN_DISABLE,
				CS35L41_BST_EN_DEFAULT};

static SOC_VALUE_ENUM_SINGLE_DECL(bst_en_ctl,
				CS35L41_PWR_CTRL2,
				CS35L41_BST_EN_SHIFT,
				CS35L41_BST_EN_MASK,
				cs35l41_bst_en_text,
				cs35l41_bst_en_values);

static const char * const cs35l41_pcm_inv_text[] = {"Disabled", "Enabled"};

static SOC_ENUM_SINGLE_DECL(pcm_inv, CS35L41_AMP_DIG_VOL_CTRL, 14,
				cs35l41_pcm_inv_text);

static const char * const cs35l41_vpbr_rel_rate_text[] = {
	"5ms", "10ms", "25ms", "50ms", "100ms", "250ms", "500ms", "1000ms"};

static SOC_ENUM_SINGLE_DECL(vpbr_rel_rate, CS35L41_VPBR_CFG, 21,
				cs35l41_vpbr_rel_rate_text);

static const char * const cs35l41_vpbr_wait_text[] = {
	"10ms", "100ms", "250ms", "500ms"};

static SOC_ENUM_SINGLE_DECL(vpbr_wait, CS35L41_VPBR_CFG, 19,
				cs35l41_vpbr_wait_text);

static const char * const cs35l41_vpbr_atk_rate_text[] = {
	"2.5us", "5us", "10us", "25us", "50us", "100us", "250us", "500us"};

static SOC_ENUM_SINGLE_DECL(vpbr_atk_rate, CS35L41_VPBR_CFG, 16,
				cs35l41_vpbr_atk_rate_text);

static const char * const cs35l41_vpbr_atk_vol_text[] = {
	"0.0625dB", "0.125dB", "0.25dB", "0.5dB",
	"0.75dB", "1dB", "1.25dB", "1.5dB"};

static SOC_ENUM_SINGLE_DECL(vpbr_atk_vol, CS35L41_VPBR_CFG, 12,
				cs35l41_vpbr_atk_vol_text);

static const char * const cs35l41_vpbr_thld1_text[] = {
	"2.402", "2.449", "2.497", "2.544", "2.592", "2.639", "2.687", "2.734",
	"2.782", "2.829", "2.877", "2.924", "2.972", "3.019", "3.067", "3.114",
	"3.162", "3.209", "3.257", "3.304", "3.352", "3.399", "3.447", "3.494",
	"3.542", "3.589", "3.637", "3.684", "3.732", "3.779", "3.827", "3.874"};

static SOC_ENUM_SINGLE_DECL(vpbr_thld1, CS35L41_VPBR_CFG, 0,
				cs35l41_vpbr_thld1_text);

static const char * const cs35l41_vpbr_en_text[] = {"Disabled", "Enabled"};

static SOC_ENUM_SINGLE_DECL(vpbr_enable, CS35L41_PWR_CTRL3, 12,
				cs35l41_vpbr_en_text);

static const char * const cs35l41_pcm_source_texts[] = {"ASP", "DSP"};
static const unsigned int cs35l41_pcm_source_values[] = {0x08, 0x32};
static SOC_VALUE_ENUM_SINGLE_DECL(cs35l41_pcm_source_enum,
				CS35L41_DAC_PCM1_SRC,
				0, CS35L41_ASP_SOURCE_MASK,
				cs35l41_pcm_source_texts,
				cs35l41_pcm_source_values);


static const struct snd_kcontrol_new pcm_source_mux =
	SOC_DAPM_ENUM("PCM Source", cs35l41_pcm_source_enum);

static const char * const cs35l41_tx_input_texts[] = {"Zero", "ASPRX1", "ASPRX2", "VMON",
							"IMON", "VPMON", "DSPTX1", "DSPTX2"};
static const unsigned int cs35l41_tx_input_values[] = {0x00, CS35L41_INPUT_SRC_ASPRX1, CS35L41_INPUT_SRC_ASPRX2,
							CS35L41_INPUT_SRC_VMON, CS35L41_INPUT_SRC_IMON,
							CS35L41_INPUT_SRC_VPMON, CS35L41_INPUT_DSP_TX1,
							CS35L41_INPUT_DSP_TX2};
static SOC_VALUE_ENUM_SINGLE_DECL(cs35l41_asptx1_enum,
				CS35L41_ASP_TX1_SRC,
				0, CS35L41_ASP_SOURCE_MASK,
				cs35l41_tx_input_texts,
				cs35l41_tx_input_values);

static const struct snd_kcontrol_new asp_tx1_mux =
	SOC_DAPM_ENUM("ASPTX1 SRC", cs35l41_asptx1_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(cs35l41_asptx2_enum,
				CS35L41_ASP_TX2_SRC,
				0, CS35L41_ASP_SOURCE_MASK,
				cs35l41_tx_input_texts,
				cs35l41_tx_input_values);

static const struct snd_kcontrol_new asp_tx2_mux =
	SOC_DAPM_ENUM("ASPTX2 SRC", cs35l41_asptx2_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(cs35l41_asptx3_enum,
				CS35L41_ASP_TX3_SRC,
				0, CS35L41_ASP_SOURCE_MASK,
				cs35l41_tx_input_texts,
				cs35l41_tx_input_values);

static const struct snd_kcontrol_new asp_tx3_mux =
	SOC_DAPM_ENUM("ASPTX3 SRC", cs35l41_asptx3_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(cs35l41_asptx4_enum,
				CS35L41_ASP_TX4_SRC,
				0, CS35L41_ASP_SOURCE_MASK,
				cs35l41_tx_input_texts,
				cs35l41_tx_input_values);

static const struct snd_kcontrol_new asp_tx4_mux =
	SOC_DAPM_ENUM("ASPTX4 SRC", cs35l41_asptx4_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(cs35l41_dsprx2_enum,
				CS35L41_DSP1_RX2_SRC,
				0, CS35L41_ASP_SOURCE_MASK,
				cs35l41_tx_input_texts,
				cs35l41_tx_input_values);

static const struct snd_kcontrol_new dsp_rx2_mux =
	SOC_DAPM_ENUM("DSPRX2 SRC", cs35l41_dsprx2_enum);

static const struct snd_kcontrol_new cs35l41_aud_controls[] = {
	{.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = "Digital PCM Volume", \
	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | \
	SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.tlv.p  = dig_vol_tlv,\
	.info = snd_soc_info_volsw_sx, \
	.get = cs35l41_pcm_vol_get,\
	.put = cs35l41_pcm_vol_put, \
	.private_value = (unsigned long)&(struct soc_mixer_control) \
		{.reg = CS35L41_AMP_DIG_VOL_CTRL, .rreg = CS35L41_AMP_DIG_VOL_CTRL, \
		.shift = 3, .rshift = 3, \
		.max = 0x391, .min = CS35L41_AMP_VOL_PCM_MUTE} },
	SOC_ENUM("Invert PCM", pcm_inv),
	SOC_SINGLE_TLV("AMP PCM Gain", CS35L41_AMP_GAIN_CTRL, 5, 0x14, 0,
			amp_gain_tlv),
	SOC_SINGLE_RANGE("ASPTX1 Slot Position", CS35L41_SP_FRAME_TX_SLOT, 0,
			 0, 7, 0),
	SOC_SINGLE_RANGE("ASPTX2 Slot Position", CS35L41_SP_FRAME_TX_SLOT, 8,
			 0, 7, 0),
	SOC_SINGLE_RANGE("ASPTX3 Slot Position", CS35L41_SP_FRAME_TX_SLOT, 16,
			 0, 7, 0),
	SOC_SINGLE_RANGE("ASPTX4 Slot Position", CS35L41_SP_FRAME_TX_SLOT, 24,
			 0, 7, 0),
	SOC_ENUM("VPBR Release Rate", vpbr_rel_rate),
	SOC_ENUM("VPBR Wait", vpbr_wait),
	SOC_ENUM("VPBR Attack Rate", vpbr_atk_rate),
	SOC_ENUM("VPBR Attack Volume", vpbr_atk_vol),
	SOC_SINGLE_RANGE("VPBR Max Attenuation", CS35L41_VPBR_CFG, 8, 0, 15, 0),
	SOC_ENUM("VPBR Threshold 1", vpbr_thld1),
	SOC_ENUM("VPBR Enable", vpbr_enable),
	SOC_ENUM("PCM Soft Ramp", pcm_sft_ramp),
	SOC_ENUM_EXT("AMP Mute", amp_mute_ctl, cs35l41_amp_mute_get,
						cs35l41_amp_mute_put),
	SOC_ENUM("Boost Enable", bst_en_ctl),
	SOC_SINGLE_EXT("DSP Booted", SND_SOC_NOPM, 0, 1, 0,
			cs35l41_halo_booted_get, cs35l41_halo_booted_put),
	WM_ADSP2_PRELOAD_SWITCH("DSP1", 1),
};

static const struct otp_map_element_t *find_otp_map(u32 otp_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(otp_map_map); i++) {
		if (otp_map_map[i].id == otp_id)
			return &otp_map_map[i];
	}

	return NULL;
}

static int cs35l41_otp_unpack(void *data)
{
	struct cs35l41_private *cs35l41 = data;
	u32 otp_mem[32];
	int i;
	/* unpack area starts at byte 10 (0-indexed) */
	int bit_offset = 16, array_offset = 2;
	unsigned int bit_sum = 8;
	u32 otp_val, otp_id_reg;
	const struct otp_map_element_t *otp_map_match;
	const struct otp_packed_element_t *otp_map;

	/*
	 * We need to make sure we are using the bus
	 * for these reads and writes so bypass
	 * cache completely to ensure we hit the
	 * registers correctly
	 */
	regcache_cache_bypass(cs35l41->regmap, true);

	regmap_write(cs35l41->regmap, CS35L41_TEST_KEY_CTL, 0x00005555);
	regmap_write(cs35l41->regmap, CS35L41_TEST_KEY_CTL, 0x0000AAAA);

	regmap_read(cs35l41->regmap, CS35L41_OTPID, &otp_id_reg);
	/* Read from OTP_MEM_IF */
	for (i = 0; i < 32; i++) {
		regmap_read(cs35l41->regmap, CS35L41_OTP_MEM0 + i * 4, &(otp_mem[i]));
		usleep_range(1, 10);
	}

	if (((otp_mem[1] & CS35L41_OTP_HDR_MASK_1) != CS35L41_OTP_HDR_VAL_1)
			|| (otp_mem[2] & CS35L41_OTP_HDR_MASK_2) != CS35L41_OTP_HDR_VAL_2) {
		dev_err(cs35l41->dev, "Bad OTP header vals\n");
		return -EINVAL;
	}

	otp_map_match = find_otp_map(otp_id_reg);

	if (otp_map_match == NULL) {
		dev_err(cs35l41->dev, "OTP Map matching ID %d not found\n",
				otp_id_reg);
		return -EINVAL;
	}

	otp_map = otp_map_match->map;

	for (i = 0; i < otp_map_match->num_elements; i++) {
		cs35l41_dbg(cs35l41->dev, "bitoffset= %d, array_offset=%d, bit_sum mod 32=%d\n",
					bit_offset, array_offset, bit_sum % 32);
		cs35l41_dbg(cs35l41->dev, "i: %d reg: %d, shift: %d size: %d\n",
			i, otp_map[i].reg, otp_map[i].shift, otp_map[i].size);
		if (bit_offset + otp_map[i].size - 1 >= 32) {
			otp_val = (otp_mem[array_offset] &
					GENMASK(31, bit_offset)) >>
					bit_offset;
			otp_val |= (otp_mem[++array_offset] &
					GENMASK(bit_offset +
						otp_map[i].size - 33, 0)) <<
					(32 - bit_offset);
			bit_offset += otp_map[i].size - 32;
		} else {

			otp_val = (otp_mem[array_offset] &
				GENMASK(bit_offset + otp_map[i].size - 1,
					bit_offset)) >>	bit_offset;
			bit_offset += otp_map[i].size;
		}
		bit_sum += otp_map[i].size;

		if (bit_offset == 32) {
			bit_offset = 0;
			array_offset++;
		}

		if (otp_map[i].reg != 0)
			regmap_update_bits(cs35l41->regmap, otp_map[i].reg,
					GENMASK(otp_map[i].shift +
							otp_map[i].size - 1,
						otp_map[i].shift),
					otp_val << otp_map[i].shift);
	}

	regmap_write(cs35l41->regmap, CS35L41_TEST_KEY_CTL, 0x0000CCCC);
	regmap_write(cs35l41->regmap, CS35L41_TEST_KEY_CTL, 0x00003333);

	regcache_cache_bypass(cs35l41->regmap, false);

	return 0;
}

static irqreturn_t cs35l41_irq(int irq, void *data)
{
	struct cs35l41_private *cs35l41 = data;
	unsigned int status[4];
	unsigned int masks[4];
	int i;

	for (i = 0; i < ARRAY_SIZE(status); i++) {
		regmap_read(cs35l41->regmap,
			    CS35L41_IRQ1_STATUS1 + (i * CS35L41_REGSTRIDE),
			    &status[i]);
		regmap_read(cs35l41->regmap,
			    CS35L41_IRQ1_MASK1 + (i * CS35L41_REGSTRIDE),
			    &masks[i]);
	}

	/* Check to see if unmasked bits are active */
	if (!(status[0] & ~masks[0]) && !(status[1] & ~masks[1]) &&
		!(status[2] & ~masks[2]) && !(status[3] & ~masks[3]))
		return IRQ_NONE;


	if (status[0] & CS35L41_PUP_DONE_MASK) {
		regmap_write(cs35l41->regmap, CS35L41_IRQ1_STATUS1,
			     CS35L41_PUP_DONE_MASK);
		complete(&cs35l41->global_pup_done);
	}

	if (status[0] & CS35L41_PDN_DONE_MASK) {
		regmap_write(cs35l41->regmap, CS35L41_IRQ1_STATUS1,
			     CS35L41_PDN_DONE_MASK);
		complete(&cs35l41->global_pdn_done);
	}

	/*
	 * The following interrupts require a
	 * protection release cycle to get the
	 * speaker out of Safe-Mode.
	 */
	if (status[0] & CS35L41_AMP_SHORT_ERR) {
		dev_crit(cs35l41->dev, "Amp short error\n");
		regmap_write(cs35l41->regmap, CS35L41_IRQ1_STATUS1,
					CS35L41_AMP_SHORT_ERR);
		regmap_write(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN, 0);
		regmap_update_bits(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN,
					CS35L41_AMP_SHORT_ERR_RLS,
					CS35L41_AMP_SHORT_ERR_RLS);
		regmap_update_bits(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN,
					CS35L41_AMP_SHORT_ERR_RLS, 0);
	}

	if (status[0] & CS35L41_TEMP_WARN) {
		dev_crit(cs35l41->dev, "Over temperature warning\n");
		regmap_write(cs35l41->regmap, CS35L41_IRQ1_STATUS1,
					CS35L41_TEMP_WARN);
		regmap_write(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN, 0);
		regmap_update_bits(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN,
					CS35L41_TEMP_WARN_ERR_RLS,
					CS35L41_TEMP_WARN_ERR_RLS);
		regmap_update_bits(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN,
					CS35L41_TEMP_WARN_ERR_RLS, 0);
	}

	if (status[0] & CS35L41_TEMP_ERR) {
		dev_crit(cs35l41->dev, "Over temperature error\n");
		regmap_write(cs35l41->regmap, CS35L41_IRQ1_STATUS1,
					CS35L41_TEMP_ERR);
		regmap_write(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN, 0);
		regmap_update_bits(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN,
					CS35L41_TEMP_ERR_RLS,
					CS35L41_TEMP_ERR_RLS);
		regmap_update_bits(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN,
					CS35L41_TEMP_ERR_RLS, 0);
	}

	if (status[0] & CS35L41_BST_OVP_ERR) {
		dev_crit(cs35l41->dev, "VBST Over Voltage error\n");
		regmap_update_bits(cs35l41->regmap, CS35L41_PWR_CTRL2,
					CS35L41_BST_EN_MASK <<
					CS35L41_BST_EN_SHIFT, 0);
		regmap_write(cs35l41->regmap, CS35L41_IRQ1_STATUS1,
					CS35L41_BST_OVP_ERR);
		regmap_write(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN, 0);
		regmap_update_bits(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN,
					CS35L41_BST_OVP_ERR_RLS,
					CS35L41_BST_OVP_ERR_RLS);
		regmap_update_bits(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN,
					CS35L41_BST_OVP_ERR_RLS, 0);
		regmap_update_bits(cs35l41->regmap, CS35L41_PWR_CTRL2,
					CS35L41_BST_EN_MASK <<
					CS35L41_BST_EN_SHIFT,
					CS35L41_BST_EN_DEFAULT <<
					CS35L41_BST_EN_SHIFT);
	}

	if (status[0] & CS35L41_BST_DCM_UVP_ERR) {
		dev_crit(cs35l41->dev, "DCM VBST Under Voltage Error\n");
		regmap_update_bits(cs35l41->regmap, CS35L41_PWR_CTRL2,
					CS35L41_BST_EN_MASK <<
					CS35L41_BST_EN_SHIFT, 0);
		regmap_write(cs35l41->regmap, CS35L41_IRQ1_STATUS1,
					CS35L41_BST_DCM_UVP_ERR);
		regmap_write(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN, 0);
		regmap_update_bits(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN,
					CS35L41_BST_UVP_ERR_RLS,
					CS35L41_BST_UVP_ERR_RLS);
		regmap_update_bits(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN,
					CS35L41_BST_UVP_ERR_RLS, 0);
		regmap_update_bits(cs35l41->regmap, CS35L41_PWR_CTRL2,
					CS35L41_BST_EN_MASK <<
					CS35L41_BST_EN_SHIFT,
					CS35L41_BST_EN_DEFAULT <<
					CS35L41_BST_EN_SHIFT);
	}

	if (status[0] & CS35L41_BST_SHORT_ERR) {
		dev_crit(cs35l41->dev, "LBST error: powering off!\n");
		regmap_update_bits(cs35l41->regmap, CS35L41_PWR_CTRL2,
					CS35L41_BST_EN_MASK <<
					CS35L41_BST_EN_SHIFT, 0);
		regmap_write(cs35l41->regmap, CS35L41_IRQ1_STATUS1,
					CS35L41_BST_SHORT_ERR);
		regmap_write(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN, 0);
		regmap_update_bits(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN,
					CS35L41_BST_SHORT_ERR_RLS,
					CS35L41_BST_SHORT_ERR_RLS);
		regmap_update_bits(cs35l41->regmap, CS35L41_PROTECT_REL_ERR_IGN,
					CS35L41_BST_SHORT_ERR_RLS, 0);
		regmap_update_bits(cs35l41->regmap, CS35L41_PWR_CTRL2,
					CS35L41_BST_EN_MASK <<
					CS35L41_BST_EN_SHIFT,
					CS35L41_BST_EN_DEFAULT <<
					CS35L41_BST_EN_SHIFT);
	}

	if (status[3] & CS35L41_OTP_BOOT_DONE) {
		regmap_update_bits(cs35l41->regmap, CS35L41_IRQ1_MASK4,
				CS35L41_OTP_BOOT_DONE, CS35L41_OTP_BOOT_DONE);
	}

	return IRQ_HANDLED;
}

static const struct reg_sequence cs35l41_pup_patch[] = {
	{0x00000040, 0x00000055},
	{0x00000040, 0x000000AA},
	{0x00002084, 0x002F1AA0},
	{0x00000040, 0x000000CC},
	{0x00000040, 0x00000033},
};

static const struct reg_sequence cs35l41_pdn_patch[] = {
	{0x00000040, 0x00000055},
	{0x00000040, 0x000000AA},
	{0x00002084, 0x002F1AA3},
	{0x00000040, 0x000000CC},
	{0x00000040, 0x00000033},
};

static bool cs35l41_is_csplmboxsts_correct(enum cs35l41_cspl_mboxcmd cmd,
					   enum cs35l41_cspl_mboxstate sts)
{
	switch (cmd) {
	case CSPL_MBOX_CMD_NONE:
	case CSPL_MBOX_CMD_UNKNOWN_CMD:
		return true;
	case CSPL_MBOX_CMD_PAUSE:
		return (sts == CSPL_MBOX_STS_PAUSED);
	case CSPL_MBOX_CMD_RESUME:
		return (sts == CSPL_MBOX_STS_RUNNING);
	default:
		return false;
	}
}

static int cs35l41_set_csplmboxcmd(struct cs35l41_private *cs35l41,
				   enum cs35l41_cspl_mboxcmd cmd)
{
	int		ret = 0;
	unsigned int	sts, i;
	bool		ack = false;

	/* Reset DSP sticky bit */
	regmap_write(cs35l41->regmap, CS35L41_IRQ2_STATUS2,
		     1 << CS35L41_CSPL_MBOX_CMD_DRV_SHIFT);

	/* Reset AP sticky bit */
	regmap_write(cs35l41->regmap, CS35L41_IRQ1_STATUS2,
		     1 << CS35L41_CSPL_MBOX_CMD_FW_SHIFT);

	/*
	 * Set mailbox cmd
	 */
	/* Unmask DSP INT */
	regmap_update_bits(cs35l41->regmap, CS35L41_IRQ2_MASK2,
			   1 << CS35L41_CSPL_MBOX_CMD_DRV_SHIFT, 0);
	regmap_write(cs35l41->regmap, CS35L41_CSPL_MBOX_CMD_DRV, cmd);

	/* Poll for DSP ACK */
	for (i = 0; i < 5; i++) {
		usleep_range(1000, 1010);
		ret = regmap_read(cs35l41->regmap, CS35L41_IRQ1_STATUS2, &sts);
		if (ret < 0) {
			dev_err(cs35l41->dev, "regmap_read failed (%d)\n", ret);
			continue;
		}
		if (sts & (1 << CS35L41_CSPL_MBOX_CMD_FW_SHIFT)) {
			cs35l41_dbg(cs35l41->dev,
				"%u: Received ACK in EINT for mbox cmd (%d)\n",
				i, cmd);
			regmap_write(cs35l41->regmap, CS35L41_IRQ1_STATUS2,
			     1 << CS35L41_CSPL_MBOX_CMD_FW_SHIFT);
			ack = true;
			break;
		}
	}

	if (!ack) {
		dev_err(cs35l41->dev,
			"Timout waiting for DSP to set mbox cmd\n");
		ret = -ETIMEDOUT;
	}

	/* Mask DSP INT */
	regmap_update_bits(cs35l41->regmap, CS35L41_IRQ2_MASK2,
			   1 << CS35L41_CSPL_MBOX_CMD_DRV_SHIFT,
			   1 << CS35L41_CSPL_MBOX_CMD_DRV_SHIFT);

	if (regmap_read(cs35l41->regmap,
			CS35L41_CSPL_MBOX_STS, &sts) < 0) {
		dev_err(cs35l41->dev, "Failed to read %u\n",
			CS35L41_CSPL_MBOX_STS);
		ret = -EACCES;
	}

	if (!cs35l41_is_csplmboxsts_correct(cmd,
					    (enum cs35l41_cspl_mboxstate)sts)) {
		dev_err(cs35l41->dev,
			"Failed to set mailbox(cmd: %u, sts: %u)\n", cmd, sts);
		ret = -ENOMSG;
	}

	return ret;
}

static int cs35l41_cap_trim(struct cs35l41_private *cs35l41, bool rcv_mode)
{
	cs35l41_dbg(cs35l41->dev, "%s rcv_mode=%d\n", __func__, rcv_mode);

	regmap_write(cs35l41->regmap, CS35L41_TEST_KEY_CTL, 0x00005555);
	regmap_write(cs35l41->regmap, CS35L41_TEST_KEY_CTL, 0x0000AAAA);

	if (rcv_mode) {
		regmap_update_bits(cs35l41->regmap, CS35L41_OTP_TRIM_30,
				CS35L41_INT1_CAP_TRIM_MASK, 0);
		regmap_update_bits(cs35l41->regmap, CS35L41_OTP_TRIM_31,
				CS35L41_INT2_CAP_TRIM_MASK, 0);
	} else {
		regmap_write(cs35l41->regmap, CS35L41_OTP_TRIM_30,
				cs35l41->spk_3_trim);
		regmap_write(cs35l41->regmap, CS35L41_OTP_TRIM_31,
				cs35l41->spk_4_trim);
	}

	regmap_write(cs35l41->regmap, CS35L41_TEST_KEY_CTL, 0x0000CCCC);
	regmap_write(cs35l41->regmap, CS35L41_TEST_KEY_CTL, 0x00003333);

	return 0;
}

static int cs35l41_pcm_source_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);
	unsigned int source, global_en;

	regmap_read(cs35l41->regmap, CS35L41_DAC_PCM1_SRC, &source);

	if (source == CS35L41_INPUT_SRC_ASPRX1)
		cs35l41->halo_routed = false;
	else if (source == CS35L41_INPUT_DSP_TX1)
		cs35l41->halo_routed = true;

	if (source != cs35l41->pcm_source_last) {
		cs35l41_dbg(cs35l41->dev, "PCM Source changed\n");
		cs35l41_cap_trim(cs35l41, source == CS35L41_INPUT_SRC_ASPRX1);
		regmap_read(cs35l41->regmap, CS35L41_PWR_CTRL1, &global_en);
		if (cs35l41->halo_booted && global_en & CS35L41_GLOBAL_EN_MASK) {
			if (cs35l41->halo_routed) {
				cs35l41_set_csplmboxcmd(cs35l41,
							CSPL_MBOX_CMD_RESUME);
			} else if (!cs35l41->halo_routed) {
				cs35l41_set_csplmboxcmd(cs35l41,
							CSPL_MBOX_CMD_PAUSE);
				regcache_drop_region(cs35l41->regmap,
							CS35L41_DAC_PCM1_SRC,
							CS35L41_DAC_PCM1_SRC);
				regmap_write(cs35l41->regmap,
						CS35L41_DAC_PCM1_SRC, source);
			}
		}
	}

	cs35l41->pcm_source_last = source;

	cs35l41_dbg(cs35l41->dev, "PCM Source: %s\n",
			(source == CS35L41_INPUT_SRC_ASPRX1) ?
			"ASPRX1" : "DSPTX1");

	return 0;
}

static int cs35l41_dsp_rx2_src_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);
	unsigned int source;

	regmap_read(cs35l41->regmap, CS35L41_DSP1_RX2_SRC, &source);
	if (source != 0)
		cs35l41->dsprx2_src = source;

	return 0;
}

static int cs35l41_main_amp_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_multi_reg_write_bypassed(cs35l41->regmap,
					cs35l41_pup_patch,
					ARRAY_SIZE(cs35l41_pup_patch));

		regmap_update_bits(cs35l41->regmap, CS35L41_PWR_CTRL1,
				CS35L41_GLOBAL_EN_MASK,
				1 << CS35L41_GLOBAL_EN_SHIFT);

		if (cs35l41->halo_booted && cs35l41->halo_routed) {
			cs35l41_set_csplmboxcmd(cs35l41,
						CSPL_MBOX_CMD_RESUME);
		} else {
			if (cs35l41->halo_played == false) {
				cs35l41_set_csplmboxcmd(cs35l41,
						CSPL_MBOX_CMD_PAUSE);
				regcache_drop_region(cs35l41->regmap,
							CS35L41_DAC_PCM1_SRC,
							CS35L41_DAC_PCM1_SRC);
				regmap_write(cs35l41->regmap,
						CS35L41_DAC_PCM1_SRC,
						CS35L41_INPUT_SRC_ASPRX1);
			}
			usleep_range(1000, 1100);
		}

		cirrus_pwr_start(cs35l41->pdata.right_channel);
		cs35l41->halo_played = true;
		dev_info(cs35l41->dev, "%s PMU\n", __func__);
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (cs35l41->halo_booted && cs35l41->halo_routed)
			cs35l41_set_csplmboxcmd(cs35l41,
						CSPL_MBOX_CMD_PAUSE);

		regmap_update_bits(cs35l41->regmap, CS35L41_PWR_CTRL1,
				CS35L41_GLOBAL_EN_MASK, 0);

		usleep_range(1000, 1100);

		regmap_multi_reg_write_bypassed(cs35l41->regmap,
					cs35l41_pdn_patch,
					ARRAY_SIZE(cs35l41_pdn_patch));

		cirrus_pwr_stop(cs35l41->pdata.right_channel);
		dev_info(cs35l41->dev, "%s PMD\n", __func__);
		break;
	default:
		dev_err(codec->dev, "Invalid event = 0x%x\n", event);
		ret = -EINVAL;
	}
	return ret;
}

static const struct snd_soc_dapm_widget cs35l41_dapm_widgets[] = {

	SND_SOC_DAPM_SPK("DSP1 Preload", NULL),
	{	.id = snd_soc_dapm_supply, .name = "DSP1 Preloader",
		.reg = SND_SOC_NOPM, .shift = 0, .event = cs35l41_dsp_power_ev,
		.event_flags = SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD,
		.subseq = 100,},
	{       .id = snd_soc_dapm_out_drv, .name = "DSP1",
		.reg = SND_SOC_NOPM, .shift = 0},

	SND_SOC_DAPM_OUTPUT("AMP SPK"),

	SND_SOC_DAPM_AIF_IN("ASPRX1", NULL, 0, CS35L41_SP_ENABLES, 16, 0),
	SND_SOC_DAPM_AIF_IN("ASPRX2", NULL, 0, CS35L41_SP_ENABLES, 17, 0),
	SND_SOC_DAPM_AIF_OUT("ASPTX1", NULL, 0, CS35L41_SP_ENABLES, 0, 0),
	SND_SOC_DAPM_AIF_OUT("ASPTX2", NULL, 0, CS35L41_SP_ENABLES, 1, 0),
	SND_SOC_DAPM_AIF_OUT("ASPTX3", NULL, 0, CS35L41_SP_ENABLES, 2, 0),
	SND_SOC_DAPM_AIF_OUT("ASPTX4", NULL, 0, CS35L41_SP_ENABLES, 3, 0),

	SND_SOC_DAPM_ADC("VMON ADC", NULL, CS35L41_PWR_CTRL2, 12, 0),
	SND_SOC_DAPM_ADC("IMON ADC", NULL, CS35L41_PWR_CTRL2, 13, 0),
	SND_SOC_DAPM_ADC("VPMON ADC", NULL, CS35L41_PWR_CTRL2, 8, 0),
	SND_SOC_DAPM_ADC("VBSTMON ADC", NULL, CS35L41_PWR_CTRL2, 9, 0),
	SND_SOC_DAPM_ADC("TEMPMON ADC", NULL, CS35L41_PWR_CTRL2, 10, 0),
	SND_SOC_DAPM_ADC("CLASS H", NULL, CS35L41_PWR_CTRL3, 4, 0),

	SND_SOC_DAPM_OUT_DRV_E("Main AMP", CS35L41_PWR_CTRL2, 0, 0, NULL, 0,
				cs35l41_main_amp_event,
				SND_SOC_DAPM_POST_PMD |	SND_SOC_DAPM_POST_PMU),

	SND_SOC_DAPM_INPUT("VP"),
	SND_SOC_DAPM_INPUT("VBST"),
	SND_SOC_DAPM_INPUT("ISENSE"),
	SND_SOC_DAPM_INPUT("VSENSE"),
	SND_SOC_DAPM_INPUT("TEMP"),

	SND_SOC_DAPM_MUX("ASP TX1 Source", SND_SOC_NOPM, 0, 0, &asp_tx1_mux),
	SND_SOC_DAPM_MUX("ASP TX2 Source", SND_SOC_NOPM, 0, 0, &asp_tx2_mux),
	SND_SOC_DAPM_MUX("ASP TX3 Source", SND_SOC_NOPM, 0, 0, &asp_tx3_mux),
	SND_SOC_DAPM_MUX("ASP TX4 Source", SND_SOC_NOPM, 0, 0, &asp_tx4_mux),
	SND_SOC_DAPM_MUX_E("DSP RX2 Source", SND_SOC_NOPM, 0, 0, &dsp_rx2_mux,
			cs35l41_dsp_rx2_src_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_MUX_E("PCM Source", SND_SOC_NOPM, 0, 0, &pcm_source_mux,
				cs35l41_pcm_source_event, SND_SOC_DAPM_PRE_PMU |
				SND_SOC_DAPM_POST_REG),
	SND_SOC_DAPM_SWITCH("DRE", SND_SOC_NOPM, 0, 0, &dre_ctrl),
	SND_SOC_DAPM_SWITCH("AMP Enable", SND_SOC_NOPM, 0, 1, &amp_enable_ctrl),
};

static const struct snd_soc_dapm_route cs35l41_audio_map[] = {

	{ "DSP1", NULL, "ASPRX1" },
	{ "DSP1 Preload", NULL, "DSP1 Preloader" },

	{"DSP RX2 Source", "VMON", "VMON ADC"},
	{"DSP RX2 Source", "IMON", "IMON ADC"},
	{"DSP RX2 Source", "VPMON", "VPMON ADC"},
	{"DSP RX2 Source", "DSPTX1", "DSP1"},
	{"DSP RX2 Source", "DSPTX2", "DSP1"},
	{"DSP RX2 Source", "ASPRX1", "ASPRX1" },
	{"DSP RX2 Source", "ASPRX2", "ASPRX2" },
	{"DSP RX2 Source", "Zero", "ASPRX1" },
	{"DSP1", NULL, "DSP RX2 Source"},

	{"ASP TX1 Source", "VMON", "VMON ADC"},
	{"ASP TX1 Source", "IMON", "IMON ADC"},
	{"ASP TX1 Source", "VPMON", "VPMON ADC"},
	{"ASP TX1 Source", "DSPTX1", "ASPRX1"},
	{"ASP TX1 Source", "DSPTX2", "ASPRX1"},
	{"ASP TX1 Source", "ASPRX1", "ASPRX1" },
	{"ASP TX1 Source", "ASPRX2", "ASPRX2" },
	{"ASP TX2 Source", "VMON", "VMON ADC"},
	{"ASP TX2 Source", "IMON", "IMON ADC"},
	{"ASP TX2 Source", "IMON", "VPMON ADC"},
	{"ASP TX2 Source", "DSPTX1", "ASPRX1"},
	{"ASP TX2 Source", "DSPTX2", "ASPRX1"},
	{"ASP TX2 Source", "ASPRX1", "ASPRX1" },
	{"ASP TX2 Source", "ASPRX2", "ASPRX2" },
	{"ASP TX3 Source", "VMON", "VMON ADC"},
	{"ASP TX3 Source", "IMON", "IMON ADC"},
	{"ASP TX3 Source", "VPMON", "VPMON ADC"},
	{"ASP TX3 Source", "DSPTX1", "ASPRX1"},
	{"ASP TX3 Source", "DSPTX2", "ASPRX1"},
	{"ASP TX3 Source", "ASPRX1", "ASPRX1" },
	{"ASP TX3 Source", "ASPRX2", "ASPRX2" },
	{"ASP TX4 Source", "VMON", "VMON ADC"},
	{"ASP TX4 Source", "IMON", "IMON ADC"},
	{"ASP TX4 Source", "VPMON", "VPMON ADC"},
	{"ASP TX4 Source", "DSPTX1", "ASPRX1"},
	{"ASP TX4 Source", "DSPTX2", "ASPRX1"},
	{"ASP TX4 Source", "ASPRX1", "ASPRX1" },
	{"ASP TX4 Source", "ASPRX2", "ASPRX2" },
	{"ASPTX1", NULL, "ASP TX1 Source"},
	{"ASPTX2", NULL, "ASP TX2 Source"},
	{"ASPTX3", NULL, "ASP TX3 Source"},
	{"ASPTX4", NULL, "ASP TX4 Source"},
	{"AMP Capture", NULL, "ASPTX1"},
	{"AMP Capture", NULL, "ASPTX2"},
	{"AMP Capture", NULL, "ASPTX3"},
	{"AMP Capture", NULL, "ASPTX4"},

	{"VMON ADC", NULL, "ASPRX1"},
	{"IMON ADC", NULL, "ASPRX1"},
	{"VPMON ADC", NULL, "ASPRX1"},
	{"TEMPMON ADC", NULL, "ASPRX1"},
	{"VBSTMON ADC", NULL, "ASPRX1"},

	{"DSP1", NULL, "IMON ADC"},
	{"DSP1", NULL, "VMON ADC"},
	{"DSP1", NULL, "VBSTMON ADC"},
	{"DSP1", NULL, "VPMON ADC"},
	{"DSP1", NULL, "TEMPMON ADC"},

	{"AMP Enable", "Switch", "AMP Playback"},
	{"ASPRX1", NULL, "AMP Enable"},
	{"ASPRX2", NULL, "AMP Enable"},
	{"DRE", "Switch", "CLASS H"},
	{"Main AMP", NULL, "CLASS H"},
	{"Main AMP", NULL, "DRE"},
	{"AMP SPK", NULL, "Main AMP"},

	{"PCM Source", "ASP", "ASPRX1"},
	{"PCM Source", "DSP", "DSP1"},
	{"CLASS H", NULL, "PCM Source"},

};

static const struct wm_adsp_region cs35l41_dsp1_regions[] = {
	{ .type = WMFW_HALO_PM_PACKED,	.base = CS35L41_DSP1_PMEM_0 },
	{ .type = WMFW_HALO_XM_PACKED,	.base = CS35L41_DSP1_XMEM_PACK_0 },
	{ .type = WMFW_HALO_YM_PACKED,	.base = CS35L41_DSP1_YMEM_PACK_0 },
	{. type = WMFW_ADSP2_XM,	.base = CS35L41_DSP1_XMEM_UNPACK24_0},
	{. type = WMFW_ADSP2_YM,	.base = CS35L41_DSP1_YMEM_UNPACK24_0},
};

static int cs35l41_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct cs35l41_private *cs35l41 =
			snd_soc_codec_get_drvdata(codec_dai->codec);
	unsigned int asp_fmt, lrclk_fmt, sclk_fmt, slave_mode;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		slave_mode = 1;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		slave_mode = 0;
		break;
	default:
		dev_warn(cs35l41->dev, "cs35l41_set_dai_fmt: Mixed master mode unsupported\n");
		return -EINVAL;
	}

	regmap_update_bits(cs35l41->regmap, CS35L41_SP_FORMAT,
				CS35L41_SCLK_MSTR_MASK,
				slave_mode << CS35L41_SCLK_MSTR_SHIFT);
	regmap_update_bits(cs35l41->regmap, CS35L41_SP_FORMAT,
				CS35L41_LRCLK_MSTR_MASK,
				slave_mode << CS35L41_LRCLK_MSTR_SHIFT);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_A:
		asp_fmt = 0;
		cs35l41->i2s_mode = false;
		cs35l41->tdm_mode = true;
		break;
	case SND_SOC_DAIFMT_I2S:
		asp_fmt = 2;
		cs35l41->i2s_mode = true;
		cs35l41->tdm_mode = false;
		break;
	default:
		dev_warn(cs35l41->dev, "cs35l41_set_dai_fmt: Invalid or unsupported DAI format\n");
		return -EINVAL;
	}

	regmap_update_bits(cs35l41->regmap, CS35L41_SP_FORMAT,
					CS35L41_ASP_FMT_MASK,
					asp_fmt << CS35L41_ASP_FMT_SHIFT);

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_IF:
		lrclk_fmt = 1;
		sclk_fmt = 0;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		lrclk_fmt = 0;
		sclk_fmt = 1;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		lrclk_fmt = 1;
		sclk_fmt = 1;
		break;
	case SND_SOC_DAIFMT_NB_NF:
		lrclk_fmt = 0;
		sclk_fmt = 0;
		break;
	default:
		dev_warn(cs35l41->dev, "cs35l41_set_dai_fmt: Invalid DAI clock INV\n");
		return -EINVAL;
	}

	regmap_update_bits(cs35l41->regmap, CS35L41_SP_FORMAT,
				CS35L41_LRCLK_INV_MASK,
				lrclk_fmt << CS35L41_LRCLK_INV_SHIFT);
	regmap_update_bits(cs35l41->regmap, CS35L41_SP_FORMAT,
				CS35L41_SCLK_INV_MASK,
				sclk_fmt << CS35L41_SCLK_INV_SHIFT);

	return 0;
}

struct cs35l41_global_fs_config {
	int rate;
	int fs_cfg;
};

static const struct cs35l41_global_fs_config cs35l41_fs_rates[] = {
	{ 12000,	0x01 },
	{ 24000,	0x02 },
	{ 48000,	0x03 },
	{ 96000,	0x04 },
	{ 192000,	0x05 },
	{ 11025,	0x09 },
	{ 22050,	0x0A },
	{ 44100,	0x0B },
	{ 88200,	0x0C },
	{ 176400,	0x0D },
	{ 8000,		0x11 },
	{ 16000,	0x12 },
	{ 32000,	0x13 },
};

static int cs35l41_pcm_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);
	unsigned int vol, vol_ramp, dsprx2_src;
	int vol_ramp_ms;

	dev_info(cs35l41->dev, "%s mute=%d\n", __func__, mute);

	if (mute) {
		regmap_update_bits(cs35l41->regmap,
			CS35L41_AMP_DIG_VOL_CTRL,
			CS35L41_AMP_VOL_PCM_MASK <<
			CS35L41_AMP_VOL_PCM_SHIFT,
			CS35L41_AMP_VOL_PCM_MUTE <<
			CS35L41_AMP_VOL_PCM_SHIFT);

		regmap_read(cs35l41->regmap,
			CS35L41_AMP_DIG_VOL_CTRL, &vol_ramp);
		vol_ramp &= CS35L41_AMP_VOL_RAMP_MASK;

		vol_ramp_ms = cs35l41_convert_ramp_rate(cs35l41,
							vol_ramp);
		if (vol_ramp_ms < 0)
			dev_err(cs35l41->dev,
				"%s: Could not convert ramp rate\n",
				__func__);
		else if (vol_ramp_ms < 20)
			usleep_range(vol_ramp_ms * 1000,
					vol_ramp_ms * 1000 + 100);
		else
			msleep(vol_ramp_ms);

		regmap_update_bits(cs35l41->regmap,
				CS35L41_AMP_OUT_MUTE,
				CS35L41_AMP_MUTE_MASK <<
				CS35L41_AMP_MUTE_SHIFT,
				CS35L41_AMP_MUTE_MASK <<
				CS35L41_AMP_MUTE_SHIFT);

		regmap_read(cs35l41->regmap, CS35L41_DSP1_RX2_SRC, &dsprx2_src);
		if (dsprx2_src == CS35L41_INPUT_SRC_ASPRX1 ||
			dsprx2_src == CS35L41_INPUT_SRC_ASPRX2)
			cs35l41->dsprx2_src = dsprx2_src;
		regmap_write(cs35l41->regmap, CS35L41_DSP1_RX1_SRC, 0);
		regmap_write(cs35l41->regmap, CS35L41_DSP1_RX2_SRC, 0);

	} else {
		regmap_update_bits(cs35l41->regmap,
				CS35L41_AMP_OUT_MUTE,
				CS35L41_AMP_MUTE_MASK <<
				CS35L41_AMP_MUTE_SHIFT, 0);
		regmap_write(cs35l41->regmap, CS35L41_DSP1_RX1_SRC,
					CS35L41_INPUT_SRC_ASPRX1);
		if (cs35l41->dsprx2_src == CS35L41_INPUT_SRC_ASPRX1 ||
			cs35l41->dsprx2_src == CS35L41_INPUT_SRC_ASPRX2)
			regmap_write(cs35l41->regmap, CS35L41_DSP1_RX2_SRC,
					cs35l41->dsprx2_src);

		dev_info(cs35l41->dev, "%s: %s\n", __func__,
				(cs35l41->amp_mute == 0) ? "Muted" : "Unmuted");

		if (cs35l41->amp_mute) {
			vol = cs35l41->pcm_vol;
			/* convert control val to register val */
			if (vol < CS35L41_AMP_VOL_CTRL_DEFAULT)
				vol += CS35L41_AMP_VOL_PCM_MUTE;
			else
				vol -= CS35L41_AMP_VOL_CTRL_DEFAULT;
			/* unmute */
			regmap_update_bits(cs35l41->regmap,
					CS35L41_AMP_DIG_VOL_CTRL,
					CS35L41_AMP_VOL_PCM_MASK <<
					CS35L41_AMP_VOL_PCM_SHIFT,
					vol << CS35L41_AMP_VOL_PCM_SHIFT);
		}
	}

	cs35l41_log_status(cs35l41, mute);
	cs35l41_dbg(cs35l41->dev, "%s exit\n", __func__);

	return 0;
}

static int cs35l41_pcm_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(dai->codec);
	int i;
	unsigned int rate;
	u8 asp_width, asp_wl;

	if (cs35l41->pdata.fixed_params) {
		if (cs35l41->pdata.fixed_rate)
			rate = cs35l41->pdata.fixed_rate;
		else
			rate = params_rate(params);

		if (cs35l41->pdata.fixed_width)
			asp_width = cs35l41->pdata.fixed_width;
		else
			asp_width = params_physical_width(params);

		if (cs35l41->pdata.fixed_wl)
			asp_wl = cs35l41->pdata.fixed_wl;
		else
			asp_wl = params_width(params);

	} else {
		rate = params_rate(params);
		asp_width = params_physical_width(params);
		asp_wl = params_width(params);
	}

	dev_info(cs35l41->dev, "%s\trate:%d, width:%d, wl:%d\n",
			__func__, rate, asp_width, asp_wl);

	for (i = 0; i < ARRAY_SIZE(cs35l41_fs_rates); i++) {
		if (rate == cs35l41_fs_rates[i].rate)
			break;
	}
	if (i < ARRAY_SIZE(cs35l41_fs_rates))
		regmap_update_bits(cs35l41->regmap, CS35L41_GLOBAL_CLK_CTRL,
			CS35L41_GLOBAL_FS_MASK,
			cs35l41_fs_rates[i].fs_cfg << CS35L41_GLOBAL_FS_SHIFT);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(cs35l41->regmap, CS35L41_SP_FORMAT,
				CS35L41_ASP_WIDTH_RX_MASK,
				asp_width << CS35L41_ASP_WIDTH_RX_SHIFT);
		regmap_update_bits(cs35l41->regmap, CS35L41_SP_RX_WL,
				CS35L41_ASP_RX_WL_MASK,
				asp_wl << CS35L41_ASP_RX_WL_SHIFT);
		/* setup TX for DSP inputs */
		regmap_update_bits(cs35l41->regmap, CS35L41_SP_FORMAT,
				CS35L41_ASP_WIDTH_TX_MASK,
				asp_width << CS35L41_ASP_WIDTH_TX_SHIFT);
		regmap_update_bits(cs35l41->regmap, CS35L41_SP_TX_WL,
				CS35L41_ASP_TX_WL_MASK,
				asp_wl << CS35L41_ASP_TX_WL_SHIFT);
		if (cs35l41->i2s_mode || cs35l41->tdm_mode) {
			regmap_update_bits(cs35l41->regmap,
					CS35L41_SP_FRAME_RX_SLOT,
					CS35L41_ASP_RX1_SLOT_MASK,
					((cs35l41->pdata.right_channel) ? 1 : 0)
					 << CS35L41_ASP_RX1_SLOT_SHIFT);
			regmap_update_bits(cs35l41->regmap,
					CS35L41_SP_FRAME_RX_SLOT,
					CS35L41_ASP_RX2_SLOT_MASK,
					((cs35l41->pdata.right_channel) ? 0 : 1)
					 << CS35L41_ASP_RX2_SLOT_SHIFT);
		}
	} else {
		regmap_update_bits(cs35l41->regmap, CS35L41_SP_FORMAT,
				CS35L41_ASP_WIDTH_TX_MASK,
				asp_width << CS35L41_ASP_WIDTH_TX_SHIFT);
		regmap_update_bits(cs35l41->regmap, CS35L41_SP_TX_WL,
				CS35L41_ASP_TX_WL_MASK,
				asp_wl << CS35L41_ASP_TX_WL_SHIFT);
	}

	return 0;
}

static int cs35l41_get_clk_config(int freq)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cs35l41_pll_sysclk); i++) {
		if (cs35l41_pll_sysclk[i].freq == freq)
			return cs35l41_pll_sysclk[i].clk_cfg;
	}

	return -EINVAL;
}

static const unsigned int cs35l41_src_rates[] = {
	8000, 12000, 11025, 16000, 22050, 24000, 32000,
	44100, 48000, 88200, 96000, 176400, 192000
};

static const struct snd_pcm_hw_constraint_list cs35l41_constraints = {
	.count  = ARRAY_SIZE(cs35l41_src_rates),
	.list   = cs35l41_src_rates,
};

static int cs35l41_pcm_startup(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *dai)
{
	if (substream->runtime)
		return snd_pcm_hw_constraint_list(substream->runtime, 0,
				SNDRV_PCM_HW_PARAM_RATE, &cs35l41_constraints);
	return 0;
}

static int cs35l41_codec_set_sysclk(struct snd_soc_codec *codec,
				int clk_id, int source, unsigned int freq,
				int dir)
{
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);

	cs35l41_dbg(cs35l41->dev, "%s\n", __func__);

	cs35l41->extclk_freq = freq;

	switch (clk_id) {
	case 0:
		cs35l41->clksrc = CS35L41_PLLSRC_SCLK;
		break;
	case 1:
		cs35l41->clksrc = CS35L41_PLLSRC_LRCLK;
		break;
	case 2:
		cs35l41->clksrc = CS35L41_PLLSRC_PDMCLK;
		break;
	case 3:
		cs35l41->clksrc = CS35L41_PLLSRC_SELF;
		break;
	case 4:
		cs35l41->clksrc = CS35L41_PLLSRC_MCLK;
		break;
	default:
		dev_err(codec->dev, "Invalid CLK Config\n");
		return -EINVAL;
	}

	cs35l41->extclk_cfg = cs35l41_get_clk_config(freq);

	if (cs35l41->extclk_cfg < 0) {
		dev_err(codec->dev, "Invalid CLK Config: %d, freq: %u\n",
			cs35l41->extclk_cfg, freq);
		return -EINVAL;
	}

	if (freq != cs35l41->pll_freq_last) {
		cs35l41_dbg(cs35l41->dev, "PLL freq changed\n");

		if (cs35l41->clksrc == CS35L41_PLLSRC_SCLK)
			regmap_update_bits(cs35l41->regmap,
						CS35L41_SP_RATE_CTRL,
						0x3F, cs35l41->extclk_cfg);

		regmap_update_bits(cs35l41->regmap, CS35L41_PLL_CLK_CTRL,
				CS35L41_PLL_OPENLOOP_MASK,
				1 << CS35L41_PLL_OPENLOOP_SHIFT);
		regmap_update_bits(cs35l41->regmap, CS35L41_PLL_CLK_CTRL,
				CS35L41_REFCLK_FREQ_MASK,
				cs35l41->extclk_cfg << CS35L41_REFCLK_FREQ_SHIFT);
		regmap_update_bits(cs35l41->regmap, CS35L41_PLL_CLK_CTRL,
				CS35L41_PLL_CLK_EN_MASK,
				0 << CS35L41_PLL_CLK_EN_SHIFT);
		regmap_update_bits(cs35l41->regmap, CS35L41_PLL_CLK_CTRL,
				CS35L41_PLL_CLK_SEL_MASK, cs35l41->clksrc);
		regmap_update_bits(cs35l41->regmap, CS35L41_PLL_CLK_CTRL,
				CS35L41_PLL_OPENLOOP_MASK,
				0 << CS35L41_PLL_OPENLOOP_SHIFT);
		regmap_update_bits(cs35l41->regmap, CS35L41_PLL_CLK_CTRL,
				CS35L41_PLL_CLK_EN_MASK,
				1 << CS35L41_PLL_CLK_EN_SHIFT);
	}

	cs35l41->pll_freq_last = freq;

	return 0;
}

static int cs35l41_dai_set_sysclk(struct snd_soc_dai *dai,
					int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);

	if (cs35l41_get_clk_config(freq) < 0) {
		dev_err(codec->dev, "Invalid CLK Config freq: %u\n", freq);
		return -EINVAL;
	}

	if (clk_id == CS35L41_PLLSRC_SCLK)
		cs35l41->sclk = freq;

	return 0;
}

static const struct reg_sequence cs35l41_fsync_errata_patch[] = {
	{0x00000040,			0x00005555},
	{0x00000040,			0x0000AAAA},
	{CS35L41_VIMON_SPKMON_RESYNC,	0x00000000},
	{0x00004310,			0x00000000},
	{CS35L41_VPVBST_FS_SEL,		0x00000000},
	{CS35L41_ASP_CONTROL4,		0x01010000},
	{0x00000040,			0x0000CCCC},
	{0x00000040,			0x00003333},
};

static int cs35l41_codec_probe(struct snd_soc_codec *codec)
{
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);
	struct classh_cfg *classh = &cs35l41->pdata.classh_config;

	/* Set Platform Data */
	if (cs35l41->pdata.sclk_frc)
		regmap_update_bits(cs35l41->regmap, CS35L41_SP_FORMAT,
				CS35L41_SCLK_FRC_MASK,
				cs35l41->pdata.sclk_frc <<
				CS35L41_SCLK_FRC_SHIFT);

	if (cs35l41->pdata.lrclk_frc)
		regmap_update_bits(cs35l41->regmap, CS35L41_SP_FORMAT,
				CS35L41_LRCLK_FRC_MASK,
				cs35l41->pdata.lrclk_frc <<
				CS35L41_LRCLK_FRC_SHIFT);

	if (cs35l41->pdata.amp_gain_zc)
		regmap_update_bits(cs35l41->regmap, CS35L41_AMP_GAIN_CTRL,
				CS35L41_AMP_GAIN_ZC_MASK,
				cs35l41->pdata.amp_gain_zc <<
				CS35L41_AMP_GAIN_ZC_SHIFT);

	if (cs35l41->pdata.bst_vctrl)
		regmap_update_bits(cs35l41->regmap, CS35L41_BSTCVRT_VCTRL1,
				CS35L41_BST_CTL_MASK, cs35l41->pdata.bst_vctrl);

	if (cs35l41->pdata.bst_ipk)
		regmap_update_bits(cs35l41->regmap, CS35L41_BSTCVRT_PEAK_CUR,
				CS35L41_BST_IPK_MASK, cs35l41->pdata.bst_ipk);

	if (cs35l41->pdata.temp_warn_thld)
		regmap_update_bits(cs35l41->regmap, CS35L41_DTEMP_WARN_THLD,
				CS35L41_TEMP_THLD_MASK,
				cs35l41->pdata.temp_warn_thld);

	if (cs35l41->pdata.dout_hiz <= CS35L41_ASP_DOUT_HIZ_MASK &&
	    cs35l41->pdata.dout_hiz >= 0)
		regmap_update_bits(cs35l41->regmap, CS35L41_SP_HIZ_CTRL,
				CS35L41_ASP_DOUT_HIZ_MASK,
				cs35l41->pdata.dout_hiz);

	if (cs35l41->pdata.inv_pcm)
		regmap_update_bits(cs35l41->regmap, CS35L41_AMP_DIG_VOL_CTRL,
				CS35l41_INV_PCM_MASK, CS35l41_INV_PCM_MASK);

	if (cs35l41->pdata.use_fsync_errata)
		regmap_register_patch(cs35l41->regmap,
				cs35l41_fsync_errata_patch,
				ARRAY_SIZE(cs35l41_fsync_errata_patch));

	if (cs35l41->pdata.dsp_ng_enable) {
		regmap_update_bits(cs35l41->regmap,
				CS35L41_MIXER_NGATE_CH1_CFG,
				CS35L41_DSP_NG_ENABLE_MASK,
				CS35L41_DSP_NG_ENABLE_MASK);
		regmap_update_bits(cs35l41->regmap,
				CS35L41_MIXER_NGATE_CH2_CFG,
				CS35L41_DSP_NG_ENABLE_MASK,
				CS35L41_DSP_NG_ENABLE_MASK);

		if (cs35l41->pdata.dsp_ng_pcm_thld) {
			regmap_update_bits(cs35l41->regmap,
				CS35L41_MIXER_NGATE_CH1_CFG,
				CS35L41_DSP_NG_THLD_MASK,
				cs35l41->pdata.dsp_ng_pcm_thld);
			regmap_update_bits(cs35l41->regmap,
				CS35L41_MIXER_NGATE_CH2_CFG,
				CS35L41_DSP_NG_THLD_MASK,
				cs35l41->pdata.dsp_ng_pcm_thld);
		}

		if (cs35l41->pdata.dsp_ng_delay) {
			regmap_update_bits(cs35l41->regmap,
				CS35L41_MIXER_NGATE_CH1_CFG,
				CS35L41_DSP_NG_DELAY_MASK,
				cs35l41->pdata.dsp_ng_delay <<
				CS35L41_DSP_NG_DELAY_SHIFT);
			regmap_update_bits(cs35l41->regmap,
				CS35L41_MIXER_NGATE_CH2_CFG,
				CS35L41_DSP_NG_DELAY_MASK,
				cs35l41->pdata.dsp_ng_delay <<
				CS35L41_DSP_NG_DELAY_SHIFT);
		}
	}

	if (cs35l41->pdata.hw_ng_sel)
		regmap_update_bits(cs35l41->regmap,
				CS35L41_NG_CFG,
				CS35L41_HW_NG_SEL_MASK,
				cs35l41->pdata.hw_ng_sel <<
				CS35L41_HW_NG_SEL_SHIFT);

	if (cs35l41->pdata.hw_ng_thld)
		regmap_update_bits(cs35l41->regmap,
				CS35L41_NG_CFG,
				CS35L41_HW_NG_THLD_MASK,
				cs35l41->pdata.hw_ng_thld <<
				CS35L41_HW_NG_THLD_SHIFT);

	if (cs35l41->pdata.hw_ng_delay)
		regmap_update_bits(cs35l41->regmap,
				CS35L41_NG_CFG,
				CS35L41_HW_NG_DLY_MASK,
				cs35l41->pdata.hw_ng_delay <<
				CS35L41_HW_NG_DLY_SHIFT);

	if (classh->classh_algo_enable) {
		if (classh->classh_bst_override)
			regmap_update_bits(cs35l41->regmap,
					CS35L41_BSTCVRT_VCTRL2,
					CS35L41_BST_CTL_SEL_MASK,
					CS35L41_BST_CTL_SEL_REG);
		if (classh->classh_bst_max_limit)
			regmap_update_bits(cs35l41->regmap,
					CS35L41_BSTCVRT_VCTRL2,
					CS35L41_BST_LIM_MASK,
					classh->classh_bst_max_limit <<
					CS35L41_BST_LIM_SHIFT);
		if (classh->classh_mem_depth)
			regmap_update_bits(cs35l41->regmap,
					CS35L41_CLASSH_CFG,
					CS35L41_CH_MEM_DEPTH_MASK,
					classh->classh_mem_depth <<
					CS35L41_CH_MEM_DEPTH_SHIFT);
		if (classh->classh_headroom)
			regmap_update_bits(cs35l41->regmap,
					CS35L41_CLASSH_CFG,
					CS35L41_CH_HDRM_CTL_MASK,
					classh->classh_headroom <<
					CS35L41_CH_HDRM_CTL_SHIFT);
		if (classh->classh_release_rate)
			regmap_update_bits(cs35l41->regmap,
					CS35L41_CLASSH_CFG,
					CS35L41_CH_REL_RATE_MASK,
					classh->classh_release_rate <<
					CS35L41_CH_REL_RATE_SHIFT);
		if (classh->classh_wk_fet_delay)
			regmap_update_bits(cs35l41->regmap,
					CS35L41_WKFET_CFG,
					CS35L41_CH_WKFET_DLY_MASK,
					classh->classh_wk_fet_delay <<
					CS35L41_CH_WKFET_DLY_SHIFT);
		if (classh->classh_wk_fet_thld)
			regmap_update_bits(cs35l41->regmap,
					CS35L41_WKFET_CFG,
					CS35L41_CH_WKFET_THLD_MASK,
					classh->classh_wk_fet_thld <<
					CS35L41_CH_WKFET_THLD_SHIFT);
	}

	wm_adsp2_codec_probe(&cs35l41->dsp, codec);

	return 0;
}

static int cs35l41_irq_gpio_config(struct cs35l41_private *cs35l41)
{
	struct cs35l41_irq_cfg *irq_gpio_cfg1 = &cs35l41->pdata.irq_config1;
	struct cs35l41_irq_cfg *irq_gpio_cfg2 = &cs35l41->pdata.irq_config2;
	int irq_pol = 0;

	if (irq_gpio_cfg1->is_present) {
		if (irq_gpio_cfg1->irq_pol_inv)
			regmap_update_bits(cs35l41->regmap,
						CS35L41_GPIO1_CTRL1,
						CS35L41_GPIO_POL_MASK,
						CS35L41_GPIO_POL_MASK);
		if (irq_gpio_cfg1->irq_out_en)
			regmap_update_bits(cs35l41->regmap,
						CS35L41_GPIO1_CTRL1,
						CS35L41_GPIO_DIR_MASK,
						0);
		if (irq_gpio_cfg1->irq_src_sel)
			regmap_update_bits(cs35l41->regmap,
						CS35L41_GPIO_PAD_CONTROL,
						CS35L41_GPIO1_CTRL_MASK,
						irq_gpio_cfg1->irq_src_sel <<
						CS35L41_GPIO1_CTRL_SHIFT);
	}

	if (irq_gpio_cfg2->is_present) {
		if (irq_gpio_cfg2->irq_pol_inv)
			regmap_update_bits(cs35l41->regmap,
						CS35L41_GPIO2_CTRL1,
						CS35L41_GPIO_POL_MASK,
						CS35L41_GPIO_POL_MASK);
		if (irq_gpio_cfg2->irq_out_en)
			regmap_update_bits(cs35l41->regmap,
						CS35L41_GPIO2_CTRL1,
						CS35L41_GPIO_DIR_MASK,
						0);
		if (irq_gpio_cfg2->irq_src_sel)
			regmap_update_bits(cs35l41->regmap,
						CS35L41_GPIO_PAD_CONTROL,
						CS35L41_GPIO2_CTRL_MASK,
						irq_gpio_cfg2->irq_src_sel <<
						CS35L41_GPIO2_CTRL_SHIFT);

	}

	if (irq_gpio_cfg2->irq_src_sel ==
			(CS35L41_GPIO_CTRL_ACTV_LO | CS35L41_VALID_PDATA))
		irq_pol = IRQF_TRIGGER_LOW;
	else if (irq_gpio_cfg2->irq_src_sel ==
			(CS35L41_GPIO_CTRL_ACTV_HI | CS35L41_VALID_PDATA))
		irq_pol = IRQF_TRIGGER_HIGH;

	return irq_pol;
}

static int cs35l41_codec_remove(struct snd_soc_codec *codec)
{
	struct cs35l41_private *cs35l41 = snd_soc_codec_get_drvdata(codec);

	wm_adsp2_codec_remove(&cs35l41->dsp, codec);
	return 0;
}

static const struct snd_soc_dai_ops cs35l41_ops = {
	.digital_mute = cs35l41_pcm_mute,
	.startup = cs35l41_pcm_startup,
	.set_fmt = cs35l41_set_dai_fmt,
	.hw_params = cs35l41_pcm_hw_params,
	.set_sysclk = cs35l41_dai_set_sysclk,
};

static struct snd_soc_dai_driver cs35l41_dai[] = {
	{
		.name = "cs35l41-pcm",
		.id = 0,
		.playback = {
			.stream_name = "AMP Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_KNOT,
			.formats = CS35L41_RX_FORMATS,
		},
		.capture = {
			.stream_name = "AMP Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_KNOT,
			.formats = CS35L41_TX_FORMATS,
		},
		.ops = &cs35l41_ops,
	},
};

static struct regmap *cs35l41_get_regmap(struct device *dev)
{
	struct cs35l41_private *cs35l41 = dev_get_drvdata(dev);

	return cs35l41->regmap;
}

static struct snd_soc_codec_driver soc_codec_dev_cs35l41 = {
	.probe = cs35l41_codec_probe,
	.remove = cs35l41_codec_remove,
	.get_regmap = cs35l41_get_regmap,

	.set_sysclk = cs35l41_codec_set_sysclk,

	.component_driver = {
		.dapm_widgets = cs35l41_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(cs35l41_dapm_widgets),
		.dapm_routes = cs35l41_audio_map,
		.num_dapm_routes = ARRAY_SIZE(cs35l41_audio_map),

		.controls = cs35l41_aud_controls,
		.num_controls = ARRAY_SIZE(cs35l41_aud_controls),
	},

	.ignore_pmdown_time = true,
};

static int cs35l41_handle_of_data(struct device *dev,
				struct cs35l41_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	unsigned int val;
	int ret;
	struct device_node *classh, *irq_gpio1, *irq_gpio2, *fixed_params;
	struct classh_cfg *classh_config = &pdata->classh_config;
	struct cs35l41_irq_cfg *irq_gpio1_config = &pdata->irq_config1;
	struct cs35l41_irq_cfg *irq_gpio2_config = &pdata->irq_config2;

	if (!np)
		return 0;

	pdata->right_channel = of_property_read_bool(np,
					"cirrus,right-channel-amp");
	pdata->sclk_frc = of_property_read_bool(np,
					"cirrus,sclk-force-output");
	pdata->lrclk_frc = of_property_read_bool(np,
					"cirrus,lrclk-force-output");
	pdata->amp_gain_zc = of_property_read_bool(np,
					"cirrus,amp-gain-zc");

	if (of_property_read_u32(np, "cirrus,temp-warn_threshold", &val) >= 0)
		pdata->temp_warn_thld = val | CS35L41_VALID_PDATA;

	ret = of_property_read_u32(np, "cirrus,boost-ctl-millivolt", &val);
	if (ret >= 0) {
		if (val < 2550 || val > 11000) {
			dev_err(dev,
				"Invalid Boost Voltage %u mV\n", val);
			return -EINVAL;
		}
		pdata->bst_vctrl = ((val - 2550) / 50) + 1;
	}

	ret = of_property_read_u32(np, "cirrus,boost-peak-milliamp", &val);
	if (ret >= 0) {
		if (val < 1600 || val > 4500) {
			dev_err(dev,
				"Invalid Boost Peak Current %u mA\n", val);
			return -EINVAL;
		}
		pdata->bst_ipk = ((val - 1600) / 50) + 0x10;
	}

	ret = of_property_read_u32(np, "cirrus,asp-sdout-hiz", &val);
	if (ret >= 0)
		pdata->dout_hiz = val;
	else
		pdata->dout_hiz = -1;

	pdata->dsp_ng_enable = of_property_read_bool(np,
					"cirrus,dsp-noise-gate-enable");
	if (of_property_read_u32(np, "cirrus,dsp-noise-gate-threshold", &val) >= 0)
		pdata->dsp_ng_pcm_thld = val | CS35L41_VALID_PDATA;
	if (of_property_read_u32(np, "cirrus,dsp-noise-gate-delay", &val) >= 0)
		pdata->dsp_ng_delay = val | CS35L41_VALID_PDATA;

	if (of_property_read_u32(np, "cirrus,hw-noise-gate-select", &val) >= 0)
		pdata->hw_ng_sel = val | CS35L41_VALID_PDATA;
	if (of_property_read_u32(np, "cirrus,hw-noise-gate-threshold", &val) >= 0)
		pdata->hw_ng_thld = val | CS35L41_VALID_PDATA;
	if (of_property_read_u32(np, "cirrus,hw-noise-gate-delay", &val) >= 0)
		pdata->hw_ng_delay = val | CS35L41_VALID_PDATA;

	classh = of_get_child_by_name(np, "cirrus,classh-internal-algo");
	classh_config->classh_algo_enable = classh ? true : false;

	pdata->inv_pcm = of_property_read_bool(np, "cirrus,invert-pcm");
	pdata->use_fsync_errata = of_property_read_bool(np,
					"cirrus,use-fsync-errata");

	if (classh_config->classh_algo_enable) {
		classh_config->classh_bst_override =
			of_property_read_bool(classh, "cirrus,classh-bst-overide");

		ret = of_property_read_u32(classh,
					   "cirrus,classh-bst-max-limit",
					   &val);
		if (ret >= 0) {
			val |= CS35L41_VALID_PDATA;
			classh_config->classh_bst_max_limit = val;
		}

		ret = of_property_read_u32(classh, "cirrus,classh-mem-depth",
					   &val);
		if (ret >= 0) {
			val |= CS35L41_VALID_PDATA;
			classh_config->classh_mem_depth = val;
		}

		ret = of_property_read_u32(classh, "cirrus,classh-release-rate",
					   &val);
		if (ret >= 0)
			classh_config->classh_release_rate = val;

		ret = of_property_read_u32(classh, "cirrus,classh-headroom",
					   &val);
		if (ret >= 0) {
			val |= CS35L41_VALID_PDATA;
			classh_config->classh_headroom = val;
		}

		ret = of_property_read_u32(classh, "cirrus,classh-wk-fet-delay",
					   &val);
		if (ret >= 0) {
			val |= CS35L41_VALID_PDATA;
			classh_config->classh_wk_fet_delay = val;
		}

		ret = of_property_read_u32(classh, "cirrus,classh-wk-fet-thld",
					   &val);
		if (ret >= 0)
			classh_config->classh_wk_fet_thld = val;
	}
	of_node_put(classh);

	/* GPIO1 Pin Config */
	irq_gpio1 = of_get_child_by_name(np, "cirrus,gpio-config1");
	irq_gpio1_config->is_present = irq_gpio1 ? true : false;
	if (irq_gpio1_config->is_present) {
		irq_gpio1_config->irq_pol_inv = of_property_read_bool(irq_gpio1,
						"cirrus,gpio-polarity-invert");
		irq_gpio1_config->irq_out_en = of_property_read_bool(irq_gpio1,
						"cirrus,gpio-output-enable");
		ret = of_property_read_u32(irq_gpio1, "cirrus,gpio-src-select",
					&val);
		if (ret >= 0) {
			val |= CS35L41_VALID_PDATA;
			irq_gpio1_config->irq_src_sel = val;
		}
	}
	of_node_put(irq_gpio1);

	/* GPIO2 Pin Config */
	irq_gpio2 = of_get_child_by_name(np, "cirrus,gpio-config2");
	irq_gpio2_config->is_present = irq_gpio2 ? true : false;
	if (irq_gpio2_config->is_present) {
		irq_gpio2_config->irq_pol_inv = of_property_read_bool(irq_gpio2,
						"cirrus,gpio-polarity-invert");
		irq_gpio2_config->irq_out_en = of_property_read_bool(irq_gpio2,
						"cirrus,gpio-output-enable");
		ret = of_property_read_u32(irq_gpio2, "cirrus,gpio-src-select",
					&val);
		if (ret >= 0) {
			val |= CS35L41_VALID_PDATA;
			irq_gpio2_config->irq_src_sel = val;
		}
	}
	of_node_put(irq_gpio2);

	fixed_params = of_get_child_by_name(np, "cirrus,fixed-hw-params");
	pdata->fixed_params = fixed_params ? true : false;
	if (fixed_params) {
		ret = of_property_read_u32(fixed_params,
					"cirrus,fixed-rate",
					   &val);
		if (ret >= 0)
			pdata->fixed_rate = val;
		else
			pdata->fixed_rate = 0;

		ret = of_property_read_u32(fixed_params,
					"cirrus,fixed-width",
					   &val);
		if (ret >= 0)
			pdata->fixed_width = val;
		else
			pdata->fixed_width = 0;

		ret = of_property_read_u32(fixed_params,
					"cirrus,fixed-wl",
					   &val);
		if (ret >= 0)
			pdata->fixed_wl = val;
		else
			pdata->fixed_wl = 0;
	}
	of_node_put(fixed_params);

	ret = of_property_read_string(np, "cirrus,dsp-part-name",
						&pdata->dsp_part_name);
	if (ret < 0)
		pdata->dsp_part_name = "cs35l41";

	return 0;
}

static const struct of_device_id cs35l41_of_match[] = {
	{.compatible = "cirrus,cs35l40"},
	{.compatible = "cirrus,cs35l41"},
	{},
};
MODULE_DEVICE_TABLE(of, cs35l41_of_match);

static const struct spi_device_id cs35l41_id_spi[] = {
	{"cs35l40", 0},
	{"cs35l41", 0},
	{}
};

MODULE_DEVICE_TABLE(spi, cs35l41_id_spi);

static const struct i2c_device_id cs35l41_id_i2c[] = {
	{"cs35l40", 0},
	{"cs35l41", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, cs35l41_id_i2c);

static const struct reg_sequence cs35l41_reva0_errata_patch[] = {
	{0x00000040,			0x00005555},
	{0x00000040,			0x0000AAAA},
	{0x00003854,			0x05180240},
	{CS35L41_OTP_TRIM_30,		0x9091A1C8},
	{0x00003014,			0x0200EE0E},
	{CS35L41_BSTCVRT_DCM_CTRL,	0x00000051},
	{0x00000054,			0x00000004},
	{CS35L41_IRQ1_DB3,		0x00000000},
	{CS35L41_IRQ2_DB3,		0x00000000},
	{0x00000040,			0x0000CCCC},
	{0x00000040,			0x00003333},
};

static int cs35l41_dsp_init(struct cs35l41_private *cs35l41)
{
	struct wm_adsp *dsp;
	int ret, i;

	dsp = &cs35l41->dsp;
	dsp->part = cs35l41->pdata.dsp_part_name;
	dsp->num = 1;
	dsp->suffix = "";
	dsp->type = WMFW_HALO;
	dsp->rev = 0;
	dsp->dev = cs35l41->dev;
	dsp->regmap = cs35l41->regmap;

	dsp->base = CS35L41_DSP1_CTRL_BASE;
	dsp->base_sysinfo = CS35L41_DSP1_SYS_ID;
	dsp->mem = cs35l41_dsp1_regions;
	dsp->num_mems = ARRAY_SIZE(cs35l41_dsp1_regions);
	cs35l41->halo_booted = false;
	dsp->unlock_all = true;

	dsp->n_rx_channels = CS35L41_DSP_N_RX_RATES;
	dsp->n_tx_channels = CS35L41_DSP_N_TX_RATES;
	ret = wm_halo_init(dsp, &cs35l41->rate_lock);

	if (cs35l41->pdata.use_fsync_errata) {
		for (i = 0; i < CS35L41_DSP_N_RX_RATES; i++)
			dsp->rx_rate_cache[i] = 0x1;
		for (i = 0; i < CS35L41_DSP_N_TX_RATES; i++)
			dsp->tx_rate_cache[i] = 0x1;
	}

	regmap_write(cs35l41->regmap, CS35L41_DSP1_RX5_SRC, CS35L41_INPUT_SRC_VPMON);
	regmap_write(cs35l41->regmap, CS35L41_DSP1_RX6_SRC, CS35L41_INPUT_SRC_CLASSH);
	regmap_write(cs35l41->regmap, CS35L41_DSP1_RX7_SRC, CS35L41_INPUT_SRC_TEMPMON);
	regmap_write(cs35l41->regmap, CS35L41_DSP1_RX8_SRC, CS35L41_INPUT_SRC_RSVD);

	return ret;
}

static int cs35l41_probe(struct platform_device *pdev)
{
	struct cs35l41_data *cs35l41_mfd = dev_get_drvdata(pdev->dev.parent);
	struct cs35l41_private *cs35l41;
	int ret;
	u32 regid, reg_revid, i, mtl_revid, int_status, chipid_match;
	int timeout = 100;
	int irq_pol = IRQF_TRIGGER_HIGH;

	cs35l41 = devm_kzalloc(&pdev->dev, sizeof(struct cs35l41_private), GFP_KERNEL);
	if (!cs35l41)
		return -ENOMEM;

	platform_set_drvdata(pdev, cs35l41);

	mutex_init(&cs35l41->rate_lock);

	cs35l41->regmap = cs35l41_mfd->regmap;
	cs35l41->dev = cs35l41_mfd->dev;
	cs35l41->irq = cs35l41_mfd->irq;
	cs35l41->num_supplies = cs35l41_mfd->num_supplies;
	cs35l41->reset_gpio = cs35l41_mfd->reset_gpio;

	for (i = 0; i < cs35l41->num_supplies; i++)
		cs35l41->supplies[i].supply = cs35l41_mfd->supplies[i].supply;

	pdev->dev.of_node = cs35l41->dev->of_node;

	if (cs35l41->dev->of_node) {
		ret = cs35l41_handle_of_data(cs35l41->dev, &cs35l41->pdata);
		if (ret != 0)
			return ret;
	} else {
		ret = -EINVAL;
		goto err;
	}

	ret = regmap_read(cs35l41->regmap, CS35L41_DEVID, &regid);
	if (ret != 0) {
		dev_err(cs35l41->dev, "Get Device ID failed\n");
		goto err;
	}

	ret = regmap_read(cs35l41->regmap, CS35L41_REVID, &reg_revid);
	if (ret != 0) {
		dev_err(cs35l41->dev, "Get Revision ID failed\n");
		goto err;
	}

	mtl_revid = reg_revid & CS35L41_MTLREVID_MASK;

	/* CS35L41 will have even MTLREVID
	*  CS35L41R will have odd MTLREVID
	*/
	chipid_match = (mtl_revid % 2) ? CS35L41R_CHIP_ID : CS35L41_CHIP_ID;
	if (regid != chipid_match) {
		dev_err(cs35l41->dev, "CS35L41 Device ID (%X). Expected ID %X\n",
			regid, chipid_match);
		ret = -ENODEV;
		goto err;
	}

	irq_pol = cs35l41_irq_gpio_config(cs35l41);

	init_completion(&cs35l41->global_pdn_done);
	init_completion(&cs35l41->global_pup_done);

	ret = devm_request_threaded_irq(cs35l41->dev, cs35l41->irq, NULL,
				cs35l41_irq, IRQF_ONESHOT | IRQF_SHARED |
				irq_pol, "cs35l41", cs35l41);

	/* CS35L41 needs INT for PDN_DONE */
	if (ret != 0) {
		dev_err(cs35l41->dev, "Failed to request IRQ: %d\n", ret);
		goto err;
	}

	/* Set interrupt masks for critical errors */
	regmap_write(cs35l41->regmap, CS35L41_IRQ1_MASK1,
			CS35L41_INT1_MASK_DEFAULT);

	cs35l41_dsp_init(cs35l41);

	switch (reg_revid) {
	case CS35L41_REVID_A0:
		ret = regmap_register_patch(cs35l41->regmap,
				cs35l41_reva0_errata_patch,
				ARRAY_SIZE(cs35l41_reva0_errata_patch));
		if (ret < 0) {
			dev_err(cs35l41->dev, "Failed to apply A0 errata patch %d\n", ret);
			goto err;
		}
	}

	do {
		if (timeout == 0) {
			dev_err(cs35l41->dev,
				"Timeout waiting for OTP_BOOT_DONE\n");
			goto err;
		}
		usleep_range(1000, 1100);
		regmap_read(cs35l41->regmap, CS35L41_IRQ1_STATUS4, &int_status);
		timeout--;
	} while (!(int_status & CS35L41_OTP_BOOT_DONE));

	cs35l41_otp_unpack(cs35l41);

	regmap_read(cs35l41->regmap, CS35L41_OTP_TRIM_30, &cs35l41->spk_3_trim);
	regmap_read(cs35l41->regmap, CS35L41_OTP_TRIM_31, &cs35l41->spk_4_trim);

	cs35l41->pcm_vol = CS35L41_AMP_VOL_CTRL_DEFAULT;
	cs35l41->amp_mute = 1;

	ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_cs35l41,
				     cs35l41_dai, ARRAY_SIZE(cs35l41_dai));
	if (ret < 0) {
		dev_err(cs35l41->dev, "%s: Register codec failed\n", __func__);
		goto err;
	}

	dev_info(cs35l41->dev, "Cirrus Logic CS35L41 (%x), Revision: %02X\n",
			regid, reg_revid);

err:
	return ret;
}

static int cs35l41_remove(struct platform_device *pdev)
{
	struct cs35l41_private *cs35l41 = platform_get_drvdata(pdev);

	cs35l41_dbg(cs35l41->dev, "%s\n", __func__);

	regmap_write(cs35l41->regmap, CS35L41_IRQ1_MASK1, 0xFFFFFFFF);
	wm_adsp2_remove(&cs35l41->dsp);
	snd_soc_unregister_codec(&pdev->dev);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static struct platform_driver cs35l41_platform_driver = {
	.driver = {
		.name		= "cs35l41-codec",
		.owner = THIS_MODULE,
	},
	.probe		= cs35l41_probe,
	.remove		= cs35l41_remove,
};

module_platform_driver(cs35l41_platform_driver);

MODULE_DESCRIPTION("ASoC CS35L41 driver");
MODULE_AUTHOR("David Rhodes, Cirrus Logic Inc, <david.rhodes@cirrus.com>");
MODULE_LICENSE("GPL");
