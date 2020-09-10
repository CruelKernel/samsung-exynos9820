// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Kimyung Lee <kernel.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/miscdevice.h>

#include "panel_drv.h"
#include "panel_spi.h"

#define PANEL_SPI_MAX_CMD_SIZE 16
#define PANEL_SPI_RX_BUF_SIZE 2048


static const struct file_operations panel_spi_drv_fops = {
	.owner = THIS_MODULE,
//	.unlocked_ioctl = aod_drv_fops_ioctl,
//	.open = panel_spi_drv_open,
//	.release = panel_spi_drv_release,
//	.write = aod_drv_fops_write,
};

#define PANEL_SPI_DEV_NAME	"panel_spi"

static int panel_spi_reset(struct spi_device *spi)
{
	int status, i;
	u8 reset_enable = 0x66;
	u8 reset_device = 0x99;
	struct spi_message msg;
	struct spi_transfer xfer[] = {
		{ .bits_per_word = 8, .len = 1, .tx_buf = &reset_enable, },
		{ .bits_per_word = 8, .len = 1, .tx_buf = &reset_device, .delay_usecs = 30 },
	};

	pr_info("%s: reset called!", __func__);

	if (IS_ERR_OR_NULL(spi)) {
		panel_err("PANEL:ERR:%s:spi device is not initialized\n", __func__);
		return -ENODEV;
	}

	if (unlikely(!spi)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	spi_message_init(&msg);
	for (i = 0; i < (unsigned int)ARRAY_SIZE(xfer); i++)
		if (xfer[i].len > 0)
			spi_message_add_tail(&xfer[i], &msg);

	status = spi_sync(spi, &msg);
	if (status < 0) {
		pr_err("%s, failed to spi_sync %d\n", __func__, status);
		return status;
	}

	return 0;
}
static int panel_spi_ctrl(struct spi_device *spi, int ctrl_msg)
{
	int ret = 0;

	switch (ctrl_msg) {
	case PANEL_SPI_CTRL_RESET:
		ret = panel_spi_reset(spi);
		break;
	case PANEL_SPI_STATUS_1:
		break;
	case PANEL_SPI_STATUS_2:
		break;
	default:
		break;
	}

	return ret;
}

static int panel_spi_cmd_setparam(struct panel_spi_dev *spi_dev, const u8 *wbuf, int wsize)
{
	if (wsize < 1 || !wbuf)
		return -EINVAL;

	memcpy(spi_dev->setparam_buffer, wbuf, wsize);
	spi_dev->setparam_buffer_size = wsize;
	return wsize;
}

static int panel_spi_command(struct panel_spi_dev *spi_dev, const u8 *wbuf, int wsize, u8 *rbuf, int rsize)
{
	int ret = 0;
	u32 speed_hz;
	struct spi_device *spi;
	struct spi_message msg;
	struct spi_transfer x_write = { .bits_per_word = 8, };
	struct spi_transfer x_read = { .bits_per_word = 8, };

	spi = spi_dev->spi;

	if (IS_ERR_OR_NULL(spi)) {
		panel_err("PANEL:ERR:%s:spi device is not initialized\n", __func__);
		return -ENODEV;
	}

	if (spi_dev->speed_hz > 0)
		spi->max_speed_hz = spi_dev->speed_hz;

	spi_message_init(&msg);
	if (wsize < 1 || !wbuf)
		return -EINVAL;

	speed_hz = spi_dev->speed_hz;
	if (speed_hz <= 0)
		speed_hz = 0;
	else if (speed_hz > spi->max_speed_hz)
		speed_hz = spi->max_speed_hz;

	x_write.len = wsize;
	x_write.tx_buf = wbuf;
	x_write.speed_hz = speed_hz;
	spi_message_add_tail(&x_write, &msg);

	if (rsize) {
		memset(spi_dev->read_buf_data, 0, PANEL_SPI_RX_BUF_SIZE);
		x_read.len = rsize;
		x_read.rx_buf = spi_dev->read_buf_data;
		x_read.speed_hz = speed_hz;
		spi_message_add_tail(&x_read, &msg);
		spi_dev->read_buf_cmd = wbuf[0];
		spi_dev->read_buf_size = rsize;
	}

	ret = spi_sync(spi, &msg);
	if (ret < 0) {
		pr_err("%s, failed to spi_sync %d\n", __func__, ret);
		return ret;
	}

	if (rbuf != NULL)
		memcpy(rbuf, spi_dev->read_buf_data, rsize);

//	print_hex_dump(KERN_ERR, "spi_cmd_print ", DUMP_PREFIX_ADDRESS, 16, 1, wbuf, wsize, false);
	return rsize;
}

static int panel_spi_read(struct panel_spi_dev *spi_dev, const u8 rcmd, u8 *rbuf, int rsize)
{
	int ret = 0;
	const u8 *wbuf;
	int wsize;

	//check setparam cmd for read operation
	if (!spi_dev->setparam_buffer)
		return -EINVAL;

	wbuf = &rcmd;
	wsize = 1;

	if (spi_dev->setparam_buffer_size > 0 && spi_dev->setparam_buffer[0] == rcmd) {
		//setparam cmd
		wbuf = spi_dev->setparam_buffer;
		wsize = spi_dev->setparam_buffer_size;
	}
	ret = panel_spi_command(spi_dev, wbuf, wsize, rbuf, rsize);

	if (ret < 0)
		return ret;

	if (!ret)
		return -EINVAL;

	//setparam_buffer_size set to 0 if setparam once
	spi_dev->setparam_buffer_size = 0;

	return rsize;
}

static int panel_spi_read_id(struct panel_spi_dev *spi_dev, u32 *id)
{
	int ret = 0;
	const u8 wbuf[] = { 0x9F };
	u8 rbuf[3] = { 0x00, };
	int wsize = 1;
	int rsize = 3;

	ret = panel_spi_command(spi_dev, wbuf, wsize, rbuf, rsize);

	if (ret < 0)
		return ret;

	if (!ret)
		return -EIO;

	*id = (rbuf[0] << 16) | (rbuf[1] << 8) | rbuf[2];
	pr_debug("%s: 0x06X\n", __func__, *id);

	return 0;
}

static int panel_spi_probe_dt(struct spi_device *spi)
{
	struct device_node *nc = spi->dev.of_node;
	int rc;
	u32 value;

	rc = of_property_read_u32(nc, "bits-per-word", &value);
	if (rc) {
		dev_err(&spi->dev, "%s has no valid 'bits-per-word' property (%d)\n",
				nc->full_name, rc);
		return rc;
	}
	spi->bits_per_word = value;

	rc = of_property_read_u32(nc, "spi-max-frequency", &value);
	if (rc) {
		dev_err(&spi->dev, "%s has no valid 'spi-max-frequency' property (%d)\n",
				nc->full_name, rc);
		return rc;
	}
	spi->max_speed_hz = value;

	return 0;
}

static int panel_spi_probe(struct spi_device *spi)
{
	int ret;

	if (unlikely(!spi)) {
		pr_err("%s, invalid spi\n", __func__);
		return -EINVAL;
	}

	ret = panel_spi_probe_dt(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "%s, failed to parse device tree, ret %d\n",
				__func__, ret);
		return ret;
	}

	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "%s, failed to setup spi, ret %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int panel_spi_remove(struct spi_device *spi)
{
	return 0;
}

static const struct of_device_id panel_spi_match_table[] = {
	{   .compatible = "poc_spi",},
	{}
};

static struct spi_drv_ops panel_spi_drv_ops = {
	.read = panel_spi_read,
	.ctl = panel_spi_ctrl,
	.cmd = panel_spi_command,
	.setparam = panel_spi_cmd_setparam,
};

static int __match_panel_spi(struct device *dev, void *data)
{
	struct spi_device *spi;

	spi = (struct spi_device *)dev;
	if (spi == NULL) {
		pr_err("PANEL:ERR:%s:failed to get spi drvdata\n", __func__);
		return 0;
	}

	pr_info("%s spi driver %s found!\n",
		__func__, spi->dev.driver->name);

	return 1;
}

static struct spi_driver panel_spi_driver = {
	.driver = {
		.name		= PANEL_SPI_DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = panel_spi_match_table,
	},
	.probe		= panel_spi_probe,
	.remove		= panel_spi_remove,
};

int panel_spi_drv_probe(struct panel_device *panel, struct spi_data *spi_data)
{
	int ret = 0;
	struct panel_spi_dev *spi_dev;
	struct device_driver *driver;
	struct device *dev;
	u32 id;

	if (!panel || !spi_data) {
		pr_err("%s panel(%p) or spi_data(%p) not exist\n",
				__func__, panel, spi_data);
		goto rest_init;
	}

	spi_dev = &panel->panel_spi_dev;
	spi_dev->ops = &panel_spi_drv_ops;
	spi_dev->speed_hz = spi_data->speed_hz;
	spi_dev->setparam_buffer = (u8 *)devm_kzalloc(panel->dev, PANEL_SPI_MAX_CMD_SIZE * sizeof(u8), GFP_KERNEL);
	spi_dev->read_buf_data = (u8 *)devm_kzalloc(panel->dev, PANEL_SPI_RX_BUF_SIZE * sizeof(u8), GFP_KERNEL);
	spi_dev->dev.minor = MISC_DYNAMIC_MINOR;
	spi_dev->dev.fops = &panel_spi_drv_fops;
	spi_dev->dev.name = DRIVER_NAME;
	ret = misc_register(&spi_dev->dev);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to register for spi_dev\n", __func__);
		goto rest_init;
	}

	ret = spi_register_driver(&panel_spi_driver);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to register for spi device\n", __func__);
		goto rest_init;
	}

	driver = driver_find(PANEL_SPI_DRIVER_NAME, &spi_bus_type);
	if (IS_ERR_OR_NULL(driver)) {
		panel_err("PANEL:ERR:%s failed to find driver\n", __func__);
		return -ENODEV;
	}

	dev = driver_find_device(driver, NULL, NULL, __match_panel_spi);
	if (IS_ERR_OR_NULL(dev)) {
		panel_err("PANEL:ERR:%s:failed to find device\n", __func__);
		return -ENODEV;
	}

	spi_dev->spi = to_spi_device(dev);
	if (IS_ERR_OR_NULL(spi_dev->spi)) {
		panel_err("PANEL:ERR:%s:failed to find spi device\n", __func__);
		return -ENODEV;
	}

	ret = panel_spi_read_id(spi_dev, &id);
	if (ret < 0)
		panel_err("PANEL:ERR:%s:failed to read id. check cable connection\n", __func__);

	panel_info("%s: spi_dev probe done! id: 0x%06X\n", __func__, id);

rest_init:
	return ret;
}

