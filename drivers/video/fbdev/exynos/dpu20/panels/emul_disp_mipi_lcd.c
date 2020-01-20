/* drivers/video/fbdev/exynos/dpu/panels/emul_disp_mipi_lcd.c
 *
 * Samsung SoC MIPI LCD driver.
 *
 * Copyright (c) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>
#include <video/mipi_display.h>

#include "../dsim.h"
#include "lcd_ctrl.h"
#include "decon_lcd.h"

#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 0
#define DEFAULT_BRIGHTNESS 0

static struct backlight_device *bd;

static int emul_disp_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}

static int emul_disp_set_brightness(struct backlight_device *bd)
{
	return 1;
}

static const struct backlight_ops emul_disp_backlight_ops = {
	.get_brightness = emul_disp_get_brightness,
	.update_status = emul_disp_set_brightness,
};

static int emul_disp_probe(struct dsim_device *dsim)
{
	bd = backlight_device_register("pwm-backlight.0", NULL,
		NULL, &emul_disp_backlight_ops, NULL);
	if (IS_ERR(bd))
		pr_alert("failed to register backlight device!\n");

	bd->props.max_brightness = MAX_BRIGHTNESS;
	bd->props.brightness = DEFAULT_BRIGHTNESS;

	return 1;
}

static int emul_disp_displayon(struct dsim_device *dsim)
{
	lcd_init(dsim->id, &dsim->lcd_info);
	lcd_enable(dsim->id);
	return 1;
}

static int emul_disp_suspend(struct dsim_device *dsim)
{
	return 1;
}

static int emul_disp_resume(struct dsim_device *dsim)
{
	return 1;
}

struct dsim_lcd_driver emul_disp_mipi_lcd_driver = {
	.probe		= emul_disp_probe,
	.displayon	= emul_disp_displayon,
	.suspend	= emul_disp_suspend,
	.resume		= emul_disp_resume,
};
