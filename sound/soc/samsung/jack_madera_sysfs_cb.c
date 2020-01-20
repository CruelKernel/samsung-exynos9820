/*
 *  jack_madera_sysfs_cb.c
 *  Copyright (c) Samsung Electronics
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 */

#include <linux/extcon.h>
#include <linux/input.h>
#include <linux/extcon/extcon-madera.h>
#include <linux/mfd/madera/core.h>
#include <sound/soc.h>
#include <sound/samsung/sec_audio_sysfs.h>
#include "jack_madera_sysfs_cb.h"

static struct snd_soc_codec *madera_codec;
int madera_jack_det;
int madera_ear_mic;

static int select_jack(int state)
{
	struct snd_soc_codec *codec = madera_codec;

	dev_info(codec->dev, "%s: %d\n", __func__, state);

	/* To be developed */

	return 0;
}

static int get_jack_status(void)
{
	struct snd_soc_codec *codec = madera_codec;

	dev_info(codec->dev, "%s: %d\n", __func__, madera_jack_det);

	return (madera_jack_det || madera_ear_mic);
}

static int get_key_status(void)
{
	struct snd_soc_codec *codec = madera_codec;
	struct madera *madera = dev_get_drvdata(codec->dev->parent);
	unsigned int val, lvl;
	int ret, key;

	ret = regmap_read(madera->regmap, MADERA_MIC_DETECT_1_CONTROL_3, &val);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to read MICDET: %d\n", ret);
		return 0;
	}

	dev_info(codec->dev, "MICDET: %x\n", val);

	ret = 0;

	if (val & MADERA_MICD_LVL_0_TO_7) {
		dev_info(codec->dev, "val LVL_0_TO_7\n");
		if (madera_ear_mic) {
			lvl = val & MADERA_MICD_LVL_MASK;
			lvl >>= MADERA_MICD_LVL_SHIFT;

			WARN_ON(!lvl);
			WARN_ON(ffs(lvl) - 1 >= madera->pdata.accdet[0].num_micd_ranges);
			if (lvl && ffs(lvl) - 1 < madera->pdata.accdet[0].num_micd_ranges) {
				key = madera->pdata.accdet[0].micd_ranges[ffs(lvl) - 1].key;

				if (key == KEY_MEDIA)
					ret = 1;
			}
		}
	}

	dev_info(codec->dev, "%s: %d\n", __func__, ret);

	return ret;
}

static int get_mic_adc(void)
{
	struct snd_soc_codec *codec = madera_codec;
	struct madera *madera = dev_get_drvdata(codec->dev->parent);
	struct madera_extcon *info = madera->extcon_info;
	int adc;

	adc = madera_extcon_manual_mic_reading(info);

	dev_info(codec->dev, "%s: %d\n", __func__, adc);

	return adc;
}

void register_madera_jack_cb(struct snd_soc_codec *codec)
{
	madera_codec = codec;

	audio_register_jack_select_cb(select_jack);
	audio_register_jack_state_cb(get_jack_status);
	audio_register_key_state_cb(get_key_status);
	audio_register_mic_adc_cb(get_mic_adc);
}
EXPORT_SYMBOL_GPL(register_madera_jack_cb);
