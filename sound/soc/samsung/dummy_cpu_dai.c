/*
 * ALSA SoC CP dummy cpu dai driver
 *
 *  This driver provides one dummy dai.
 *
 * Copyright (c) 2014 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <sound/soc.h>

static struct snd_soc_dai_driver dummy_cpu_dai_drv = {
	.name = "dummy_cpu_dai",
	.playback = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_S24_LE |
			   SNDRV_PCM_FMTBIT_U24_LE |
			   SNDRV_PCM_FMTBIT_S20_3LE |
			   SNDRV_PCM_FMTBIT_U20_3LE |
			   SNDRV_PCM_FMTBIT_S16_LE |
			   SNDRV_PCM_FMTBIT_U16_LE,
	},
};

static const struct snd_soc_component_driver dummy_cpudai_component = {
	.name		= "dummy-cpudai",
};

static int dummy_cpu_probe(struct platform_device *pdev)
{
	snd_soc_register_component(&pdev->dev, &dummy_cpudai_component,
			&dummy_cpu_dai_drv, 1);
	return 0;
}

static int dummy_cpu_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static const struct of_device_id dummy_cpu_of_match[] = {
	{ .compatible = "samsung,dummy-cpu", },
	{},
};
MODULE_DEVICE_TABLE(of, dummy_cpu_of_match);

static struct platform_driver dummy_cpu_driver = {
	.probe		= dummy_cpu_probe,
	.remove		= dummy_cpu_remove,
	.driver		= {
		.name	= "dummy_cpu_dai",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(dummy_cpu_of_match),
	},
};

module_platform_driver(dummy_cpu_driver);

MODULE_AUTHOR("Hyunwoong Kim <khw0178.kim@samsung.com>");
MODULE_DESCRIPTION("Dummy dai driver");
MODULE_LICENSE("GPL");
