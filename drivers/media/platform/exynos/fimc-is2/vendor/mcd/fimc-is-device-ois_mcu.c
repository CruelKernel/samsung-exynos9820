/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/vmalloc.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#ifdef CONFIG_OIS_FW_UPDATE_THREAD_USE
#include <linux/kthread.h>
#endif

#include <exynos-fimc-is-sensor.h>
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-interface.h"
#include "fimc-is-sec-define.h"
#include "fimc-is-device-ischain.h"
#include "fimc-is-dt.h"
#include "fimc-is-device-ois.h"
#include "fimc-is-vender-specific.h"
#ifdef CONFIG_AF_HOST_CONTROL
#include "fimc-is-device-af.h"
#endif
#include <linux/pinctrl/pinctrl.h>
#include "fimc-is-core.h"
#include "fimc-is-device-ois_mcu.h"
#include "fimc-is-i2c-config.h"

#define MCU_NAME "MCU_STM32"
static const struct v4l2_subdev_ops subdev_ops;

/* Flash memory page(or sector) structure */
sysboot_page_type memory_pages[] = {
	{2048, 32},
	{   0,  0}
};

sysboot_map_type memory_map = {
	0x08000000, /* flash memory starting address */
	0x1FFF0000, /* system memory starting address */
	0x1FFF7800, /* option byte starting address */
	(sysboot_page_type *)memory_pages,
};

static int ois_shift_x[POSITION_NUM];
static int ois_shift_y[POSITION_NUM];
#ifdef OIS_CENTERING_SHIFT_ENABLE
static int ois_centering_shift_x;
static int ois_centering_shift_y;
static int ois_centering_shift_x_rear2;
static int ois_centering_shift_y_rear2;
static bool ois_centering_shift_enable;
#endif
static int ois_shift_x_rear2[POSITION_NUM];
static int ois_shift_y_rear2[POSITION_NUM];
#ifdef USE_OIS_SLEEP_MODE
static bool ois_wide_start;
static bool ois_tele_start;
#else
static bool ois_wide_init;
static bool ois_tele_init;
#endif
static bool ois_hw_check;
static bool ois_fadeupdown;
static u16 ois_center_x;
static u16 ois_center_y;
static u32 sec_hw_rev;
extern struct fimc_is_ois_info ois_minfo;
extern struct fimc_is_ois_info ois_pinfo;
extern struct fimc_is_ois_info ois_uinfo;
extern struct fimc_is_ois_exif ois_exif_data;
static struct mcu_default_data mcu_init;

struct i2c_client *fimc_is_mcu_i2c_get_client(struct fimc_is_core *core)
{
	struct i2c_client *client = NULL;
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	u32 sensor_idx = specific->mcu_sensor_index;

	if (core->sensor[sensor_idx].mcu != NULL)
		client = core->sensor[sensor_idx].mcu->client;

	return client;
};

int fimc_is_mcu_wait_ack(struct i2c_client *client, ulong timeout)
{
	int i;
	int ret = 0;
	u8 recv = 0;

	for (i = 0; i < BOOT_I2C_WAIT_RESP_POLL_RETRY; i++) {
		ret = i2c_master_recv(client, &recv, 1);
		if (ret > 0) {
			if (recv == BOOT_I2C_RESP_ACK) {
				info("mcu ack success");
				return 0;
			} else if (recv == BOOT_I2C_RESP_BUSY) {
				msleep(3);
				ret = -EBUSY;
				continue;
			} else if (recv == BOOT_I2C_RESP_NACK) {
				msleep(3);
				return -EFAULT;
			} else {
				msleep(3);
				ret = -ENODEV;
				continue;
			}
		} else {
			err("receive i2c reve failed");
			if (time_after(jiffies, timeout)) {
				/* Bus was not idle, try to reset */
				err("wait timeout");
				return -EBUSY;
			}
		}
		msleep(5);
	}

	msleep(5);

	return ret;
}

int fimc_is_mcu_info(struct v4l2_subdev *subdev, int info, int size)
{
	int i;
	int ret = 0;
	u8 cmd[2] = {0, };
	u8 recv[BOOT_I2C_RESP_GET_ID_LEN] = {0, };
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	info("%s started", __func__);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if (!mcu) {
		err("mcu is NULL");
		return -EINVAL;
	}

	client = mcu->client;

	/* build command */
	cmd[0] = info;
	cmd[1] = ~cmd[0];

	for (i = 0; i < BOOT_I2C_SYNC_RETRY_COUNT; i++) {
		/* transmit command */
		ret = i2c_master_send(client, &cmd[0], 2);
		if (ret < 0) {
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			err("cmd transfer fail cmd = 0x%x", cmd[0]);
			continue;
		} else
			info("%s cmd transfer success, ret = %d", __func__, ret);

		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret) {
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			err("wait ack fail ret = %d", ret);
			continue;
		}

		/* receive payload */
		ret = i2c_master_recv(client, recv, size);
		if (ret < 0) {
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			err("receive payload fail");
			continue;
		}

		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret) {
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			err("wait ack fail ret = %d", ret);
			continue;
		}

		if (info == BOOT_I2C_CMD_GET_ID) {
			memcpy((void *)&(mcu->id), &recv[1], recv[0] + 1);
			mcu->id = NTOHS(mcu->id);
			info("mcu info(id) = %d", mcu->id);
		} else if (info == BOOT_I2C_CMD_GET_VER) {
			memcpy((void *)&(mcu->ver), recv, 1);
			info("mcu info(ver) = %d", mcu->ver);
		}

		return 0;
	}

	return ret;
}

int fimc_is_mcu_sync(struct i2c_client *client, struct fimc_is_mcu *mcu)
{
	int i;
	int ret = 0;
	u8 data = 0;

	info("%s started", __func__);

	data = 0xFF;

	fimc_is_i2c_pin_config(client, true);

	for (i = 0; i < BOOT_I2C_SYNC_RETRY_COUNT; i++) {
		ret = i2c_master_send(client, &data, 1);
		if (ret >= 0) {
			info("mcu sync success ret = %d", ret);
			return 0;
		} else {
			err("mcu sync failed, ret = %d", ret);
		}
	}

	return ret;
}

int  fimc_is_mcu_connect(struct v4l2_subdev *subdev, struct fimc_is_core *core)
{
	int ret = 0;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;
	int gpio_mcu_reset;
	int gpio_mcu_boot0;

	info("%s started", __func__);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if (!mcu) {
		err("mcu is NULL");
		return -EINVAL;
	}

	client = mcu->client;

	if (mcu->gpio_mcu_boot0) {
		gpio_mcu_boot0 = mcu->gpio_mcu_boot0;
	} else {
		err("gpio_mcu_boot0 is not valid");
		goto exit;
	}

	if (mcu->gpio_mcu_reset) {
		gpio_mcu_reset = mcu->gpio_mcu_reset;
	} else {
		err("gpio_mcu_reset is not valid");
		goto exit;
	}

	gpio_direction_output(gpio_mcu_reset, GPIO_PIN_RESET);
	gpio_direction_output(gpio_mcu_boot0, GPIO_PIN_SET);
	msleep(BOOT_NRST_PULSE_INTVL);
	gpio_direction_output(gpio_mcu_reset, GPIO_PIN_SET);
	msleep(BOOT_I2C_SYNC_RETRY_INTVL);
	gpio_direction_output(gpio_mcu_boot0, GPIO_PIN_RESET);

	ret = fimc_is_mcu_sync(client, mcu);
	if (!ret) {
		info("mcu sync success, reconnect.");
		gpio_direction_output(gpio_mcu_reset, GPIO_PIN_RESET);
		gpio_direction_output(gpio_mcu_boot0, GPIO_PIN_SET);
		msleep(BOOT_NRST_PULSE_INTVL);
		gpio_direction_output(gpio_mcu_reset, GPIO_PIN_SET);
		msleep(BOOT_I2C_SYNC_RETRY_INTVL);
		gpio_direction_output(gpio_mcu_boot0, GPIO_PIN_RESET);
	}

	info("%s end", __func__);

exit:
	return ret;
}

void fimc_is_mcu_disconnect(struct v4l2_subdev *subdev, struct fimc_is_core *core)
{
	struct fimc_is_mcu *mcu = NULL;
	int gpio_mcu_reset;
	int gpio_mcu_boot0;

	info("%s started", __func__);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if (!mcu) {
		err("mcu is NULL");
		return;
	}

	if (mcu->gpio_mcu_boot0) {
		gpio_mcu_boot0 = mcu->gpio_mcu_boot0;
	} else {
		err("gpio_mcu_boot0 is not valid");
		goto exit;
	}

	if (mcu->gpio_mcu_reset) {
		gpio_mcu_reset = mcu->gpio_mcu_reset;
	} else {
		err("gpio_mcu_reset is not valid");
		goto exit;
	}

	gpio_direction_output(gpio_mcu_boot0, GPIO_PIN_RESET);
	msleep(BOOT_NRST_PULSE_INTVL);
	gpio_direction_output(gpio_mcu_reset, GPIO_PIN_RESET);
	msleep(BOOT_NRST_PULSE_INTVL);
	gpio_direction_output(gpio_mcu_reset, GPIO_PIN_SET);

exit:
	return;
}

u8 fimc_is_mcu_checksum(u8 *src, u32 len)
{
	u16 csum = *src++;

	if (len) {
		while (--len) {
			csum ^= *src++;
		}
	} else {
		csum = 0; /* error (no length param) */
	}

	info("mcu checksum = 0x%04x.", csum);

	return csum;
}

int fimc_is_mcu_i2c_read(struct i2c_client *client, u32 address, u8 *dst, size_t len)
{
	u8 cmd[2] = {0, };
	u8 startaddr[5] = {0, };
	u8 nbytes[2] = {0, };
	int ret = 0;
	int retry = 0;

	/* build command */
	cmd[0] = BOOT_I2C_CMD_READ;
	cmd[1] = ~cmd[0];

	/* build address + checksum */
	*(u32 *)startaddr = HTONL(address);
	startaddr[4] = fimc_is_mcu_checksum(startaddr, 4);

	/* build number of bytes + checksum */
	nbytes[0] = len - 1;
	nbytes[1] = ~nbytes[0];

	for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry) {
		/* transmit command */
		ret = i2c_master_send(client, cmd, 2);
		if (ret < 0) {
			err("[MCU] failed to send cmd");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			err("wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* transmit address */
		ret = i2c_master_send(client, startaddr, 5);
		if (ret < 0) {
			err("[MCU] failed to send address");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			err("wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* transmit number of bytes + datas */
		ret = i2c_master_send(client, nbytes, sizeof(nbytes));
		if (ret < 0) {
			err("[MCU] failed to send data");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			err("wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* receive payload */
		ret = i2c_master_recv(client, dst, len);
		if (ret < 0) {
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		return 0;
	}

	return -EINVAL;
}

int fimc_is_mcu_i2c_write(struct i2c_client *client, u32 address, u8 *src, size_t len)
{
	u8 cmd[2] = {0, };
	u8 startaddr[5] = {0, };
	int ret = 0;
	int retry = 0;
	char * buf = NULL;

	/* build command */
	cmd[0] = BOOT_I2C_CMD_WRITE;
	cmd[1] = ~cmd[0];

	/* build address + checksum */
	*(u32 *)startaddr = HTONL(address);
	startaddr[4] = fimc_is_mcu_checksum(startaddr, 4);

	/* build number of bytes + checksum */
	buf = kzalloc(len + 2, GFP_KERNEL);
	if (!buf) {
		err("[MCU] failed to alloc memory");
		return -ENOMEM;
	}

	buf[0] = len -1;
	memcpy(&buf[1], src, len);
	buf[len+1] = fimc_is_mcu_checksum(buf, len + 1);

	info("mcu write cmd started");

	for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry) {
		/* transmit command */
		ret = i2c_master_send(client, cmd, 2);
		if (ret < 0) {
			err("[MCU] failed to send cmd");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			err("wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* transmit address */
		ret = i2c_master_send(client, startaddr, 5);
		if (ret < 0) {
			err("[MCU] failed to send address");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			err("wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* transmit number of bytes + datas */
		ret = i2c_master_send(client, buf, len + 2);
		if (ret < 0) {
			err("[MCU] failed to send data");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_WRITE_TMOUT);
		if (ret < 0) {
			err("wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		kfree(buf);

		return 0;
	}

	kfree(buf);

	return -EINVAL;
}

int fimc_is_mcu_conv_memory_map(uint32_t address, size_t len, sysboot_erase_param_type *erase)
{
	sysboot_page_type *map = memory_map.pages;
	int found = 0;
	int total_bytes = 0, total_pages = 0;
	int ix = 0;
	int unit = 0;

	info("%s started", __func__);

	/* find out the matched starting page number and total page count */
	for (ix = 0; map[ix].size != 0; ++ix) {
		for (unit = 0; unit < map[ix].count; ++unit) {
			/* MATCH CASE: Starting address aligned and page number to be erased */
			if (address == memory_map.flashbase + total_bytes) {
				found++;
				erase->page = total_pages;
			}
			total_bytes += map[ix].size;
			total_pages++;
			/* MATCH CASE: End of page number to be erased */
			if ((found == 1) && (len <= total_bytes)) {
				found++;
				erase->count = total_pages - erase->page;
			}
		}
	}

	if (found < 2) {
		/* Not aligned address or too much length inputted */
		err("conv failed.");
		return -EFAULT;
	}

	if ((address == memory_map.flashbase) && (erase->count == total_pages))
		erase->page = 0xFFFF; /* mark the full erase */

	info("mcu conv cmd end.");

	return 0;
}

int fimc_is_mcu_erase(struct v4l2_subdev *subdev, u32 address, size_t len)
{
	u8 cmd[2] = {0, };
	sysboot_erase_param_type erase;
	u8 xmit_bytes = 0;
	int ret = 0;
	int retry = 0;
	uint8_t *xmit = NULL;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	info("%s started", __func__);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if (!mcu) {
		err("mcu is NULL");
		return -EINVAL;
	}

	client = mcu->client;

	/* build command */
	cmd[0] = BOOT_I2C_CMD_ERASE;
	cmd[1] = ~cmd[0];

	/* build erase parameter */
	ret = fimc_is_mcu_conv_memory_map(address, len, &erase);
	if (ret < 0)
		return -EINVAL;

	info("mcu erase page 0x%x", erase.page);

	xmit = kzalloc(1024, GFP_KERNEL);
	if (!xmit) {
		err("xmit is NULL");
		return -EINVAL;
	}

	for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry) {
		/* build full erase command */
		if (erase.page == 0xFFFF) {
			*(u16 *)xmit = (uint16_t)erase.page;
		}

		/* build page erase command */
		else {
			*(u16 *)xmit = HTONS((erase.count - 1));
		}
		xmit_bytes = sizeof(u16);
		xmit[xmit_bytes] = fimc_is_mcu_checksum(xmit, xmit_bytes);
		xmit_bytes++;

		/* transmit command */
		ret = i2c_master_send(client, cmd, sizeof(cmd));
		if (ret < 0) {
			err("send data failed");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		} else
			info("%s cmd transfer success, ret = %d", __func__, ret);

		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			err("wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		/* transmit parameter */
		ret = i2c_master_send(client, xmit, xmit_bytes);
		if (ret < 0) {
			err("send data failed");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		} else
			info("%s xmit transfer success, ret = %d", __func__, ret);

		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, (erase.page == 0xFFFF) ?
			BOOT_I2C_FULL_ERASE_TMOUT : BOOT_I2C_WAIT_RESP_TMOUT);
		if (ret < 0) {
			err("wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		info("mcu erase page started.");

		/* case of page erase */
		if (erase.page != 0xFFFF) {
			/* build page erase parameter */
			register int ix;
			register u16 *pbuf = (uint16_t *)xmit;

			for (ix = 0; ix < erase.count; ++ix)
				pbuf[ix] = HTONS((erase.page + ix));

			xmit_bytes = 2 * erase.count;
			*((uint8_t *)&pbuf[ix]) = fimc_is_mcu_checksum(xmit, xmit_bytes);
			xmit_bytes++;
			info("mcu xmit_bytess = %d, erase.count = %d");

			/* transmit parameter */
			ret = i2c_master_send(client, xmit, xmit_bytes);
			if (ret < 0) {
				err("send data failed");
				msleep(BOOT_I2C_SYNC_RETRY_INTVL);
				continue;
			} else
				info("%s xmit transfer success, ret = %d", __func__, ret);

			/* wait for ACK response */
			ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_PAGE_ERASE_TMOUT(erase.count + 1));
			if (ret < 0) {
				err("wait ack fail ret = %d", ret);
				msleep(BOOT_I2C_SYNC_RETRY_INTVL);
				continue;
			}
		}

		if (xmit)
			kfree(xmit);

		return 0;
	}

	if (xmit)
		kfree(xmit);

	return -EINVAL;
}

int fimc_is_mcu_write_unprotect(struct i2c_client *client)
{
	u8 cmd[2] = {0, };
	int ret = 0;
	int retry;

	info("%s started", __func__);

	/* build command */
	cmd[0] = BOOT_I2C_CMD_WRITE_UNPROTECT;
	cmd[1] = ~cmd[0];

	for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry) {
		/* transmit co`mmand */
		ret = i2c_master_send(client, cmd, sizeof(cmd));
		if (ret < 0) {
			err("send data failed");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		} else
			info("%s cmd transfer success, ret = %d", __func__, ret);
		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_FULL_ERASE_TMOUT);
		if (ret < 0) {
			err("wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_FULL_ERASE_TMOUT);
		if (ret < 0) {
			err("wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		return 0;
	}

	return -EINVAL;
}

int fimc_is_mcu_read_unprotect(struct i2c_client *client)
{
	uint8_t cmd[2] = {0, };
	int ret = 0;
	int retry;

	info("%s started", __func__);

	/* build command */
	cmd[0] = BOOT_I2C_CMD_READ_UNPROTECT;
	cmd[1] = ~cmd[0];

	for (retry = 0; retry < BOOT_I2C_SYNC_RETRY_COUNT; ++retry) {
		/* transmit command */
		ret = i2c_master_send(client, cmd, sizeof(cmd));
		if (ret < 0) {
			err("send data failed");
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		} else
			info("%s cmd transfer success, ret = %d", __func__, ret);
		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_FULL_ERASE_TMOUT);
		if (ret < 0) {
			err("wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}
		/* wait for ACK response */
		ret = fimc_is_mcu_wait_ack(client, BOOT_I2C_FULL_ERASE_TMOUT);
		if (ret < 0) {
			err("wait ack fail ret = %d", ret);
			msleep(BOOT_I2C_SYNC_RETRY_INTVL);
			continue;
		}

		return 0;
	}

	return -EINVAL;
}

int fimc_is_mcu_empty_check_status(struct v4l2_subdev *subdev)
{
	u32 value = 0;
	int ret = 0;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	info("%s started", __func__);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if (!mcu) {
		err("mcu is NULL");
		return -EINVAL;
	}

	client = mcu->client;

	/* Read first flash memory word ------------------------------------------- */
	ret = fimc_is_mcu_i2c_read(client, memory_map.flashbase, (u8 *)&value, sizeof(value));
	if (ret < 0) {
		err("failed to read word for empty check (%d)", ret);
		goto empty_check_status_fail;
	}

	info("mcu flash word: 0x%08X", value);

	if (value == 0xFFFFFFFF) {
		return 1;
	}

	return 0;

empty_check_status_fail:

	return -1;
}

int fimc_is_mcu_empty_check_clear(struct v4l2_subdev *subdev, struct fimc_is_core *core)
{
	int ret = 0;
	uint32_t optionbyte = 0;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	info("%s started", __func__);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if (!mcu) {
		err("mcu is NULL");
		return -EINVAL;
	}

	client = mcu->client;

	/* Option Byte read ------------------------------------------------------- */
	ret = fimc_is_mcu_i2c_read(client, memory_map.optionbyte, (uint8_t *)&optionbyte, sizeof(optionbyte));
	if (ret < 0) {
		err("mcu read  failed (%d)", ret);
		goto empty_check_clear_fail;
	}

	/* Option byte write (dummy: readed value) -------------------------------- */
	ret = fimc_is_mcu_i2c_write(client, memory_map.optionbyte, (uint8_t *)&optionbyte, sizeof(optionbyte));
	if (ret < 0) {
		err("mcu write  failed (%d)", ret);
		goto empty_check_clear_fail;
	}

	/* Put little delay for Target program option byte and self-reset */
	mdelay(150);

	/* Option byte read for checking protection status ------------------------ */
	/* 1> Re-connect to the target */
	ret = fimc_is_mcu_connect(subdev, core);
	if (ret) {
		err("[INF] Cannot connect to the target for RDP check (%d)", ret);
		goto empty_check_clear_fail;
	}

	info("[INF] Re-Connection OK");

	/* 2> Read from target for status checking and recover it if needed */
	ret = fimc_is_mcu_i2c_read(client, memory_map.optionbyte, (uint8_t *)&optionbyte, sizeof(optionbyte));
	if ((ret < 0) || ((optionbyte & 0x000000FF) != 0xAA)) {
		err("[INF]  Failed to read option byte from target (%d)", ret);

		/* Tryout the RDP level to 0 */
		ret = fimc_is_mcu_read_unprotect(client);
		if (ret) {
			info("[INF] Readout unprotect KO ... Host restart and try again");
		} else {
			info("[INF] Readout unprotect OK ... Host restart and try again");
		}

		/* Put little delay for Target erase all of pages */
		msleep(50);
		goto empty_check_clear_fail;
	}

	return 0;

empty_check_clear_fail:

	return -1;
}

int fimc_is_mcu_optionbyte_update(struct v4l2_subdev *subdev, struct fimc_is_core *core)
{
	int ret = 0;
	u32 optionbyte = 0;
	int retry = 3;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	info("%s started", __func__);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if (!mcu) {
		err("mcu is NULL");
		return -EINVAL;
	}

	client = mcu->client;

optionbyte_update_entry:

	if (--retry <= 0) {
		goto optionbyte_update_fail;
	}

	/* Option Byte read ------------------------------------------------------- */
	ret = fimc_is_mcu_i2c_read(client, memory_map.optionbyte, (u8 *)&optionbyte, sizeof(optionbyte));
	if ((ret < 0) ||((optionbyte & 0x000000ff) != 0xaa)) {
		info("mcu read fail. read unprotest.");
		/* Tryout the RDP level to 0 */
		ret = fimc_is_mcu_read_unprotect(client);
		if (ret) {
			err("read unprotected fail");
		} else {
			info("mcu read unprotected success");
		}

		/* Put little delay for Target erase all of pages */
		msleep(60);

		/* Re-connect to the target */
		ret = fimc_is_mcu_connect(subdev, core);
		if (ret) {
			err("mcu connect at optionbyte failed.");
			goto optionbyte_update_fail;
		}
		info("mcu connect at optionbyte success.");
		goto optionbyte_update_entry;
	}

	info("mcu optionbyte update processing.");

	/* Clear nBOOT_SEL bit in Option byte1 if it is set.
	* NOTE: nBOOT_SEL bit is 24th-bit */
	if (optionbyte & (1 << 24)) {
		/* Option byte write ---------------------------------------------------- */
		optionbyte &= ~(1 << 24);
		ret = fimc_is_mcu_i2c_write(client, memory_map.optionbyte, (uint8_t *)&optionbyte, sizeof(optionbyte));
		if (ret) {
			err("mcu write failed.");
			goto optionbyte_update_fail;
		}
		info("mcu write option byte OK");

		/* Put little delay for Target program option byte and self-reset */
		msleep(100);

		/* Reconnection required after the target option byte will be written */
		ret = fimc_is_mcu_connect(subdev, core);
		if (ret) {
			err("[MCU] mcu connect failed.");
			goto optionbyte_update_fail;
		}

		info("mcu Re-Connection OK");

		/* Retry sequence for verification of option byte */
		goto optionbyte_update_entry;
	}

	info("mcu optionbyte update done.");

	return 0;

optionbyte_update_fail:
	err("mcu option byte failed.");

	return -1;
}

int fimc_is_mcu_validation(struct v4l2_subdev *subdev, struct fimc_is_core *core)
{
	int ret = 0;

	info("%s started", __func__);

	ret = fimc_is_mcu_connect(subdev, core);
	if (ret) {
		err("[MCU] mcu connect failed.");
		goto validation_fail;
	}

	ret = fimc_is_mcu_info(subdev, BOOT_I2C_CMD_GET_ID, 3);
	if (ret < 0) {
		err("get id info failed");
		goto validation_fail;
	}

	ret = fimc_is_mcu_info(subdev, BOOT_I2C_CMD_GET_VER, 1);
	if (ret < 0) {
		err("get ver info failed");
		goto validation_fail;
	}

	ret = fimc_is_mcu_optionbyte_update(subdev, core);
	if (ret < 0) {
		err("optionbyte update failed");
		goto validation_fail;
	}

	return 0;

validation_fail:
	fimc_is_mcu_disconnect(subdev, core);

	return -1;
}

bool fimc_is_mcu_fw_version(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u8 hwver[4] = {0, };
	u8 vdrinfo[4] = {0, };
	u32 addr = 0;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	info("%s started", __FUNCTION__);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if (!mcu) {
		err("mcu is NULL");
		return -EINVAL;
	}

	client = mcu->client;

	fimc_is_i2c_pin_config(client, true);

	addr = 0x00F8;

	ret = fimc_is_ois_i2c_read_multi(client, addr, &hwver[0], 4);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	addr = 0x007C;

	ret = fimc_is_ois_i2c_read_multi(client, addr, &vdrinfo[0], 4);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	memcpy(&mcu->vdrinfo_mcu[0], &vdrinfo[0], 4);
	mcu->hw_mcu[0] = hwver[3];
	mcu->hw_mcu[1] = hwver[2];
	mcu->hw_mcu[2] = hwver[1];
	mcu->hw_mcu[3] = hwver[0];
	memcpy(ois_minfo.header_ver, &mcu->hw_mcu[0], 4);
	memcpy(&ois_minfo.header_ver[4], &vdrinfo[0], 4);

	info("[%s] mcu module hw ver = %c%c%c%c, vdrinfo ver = %c%c%c%c", __func__,
		hwver[3], hwver[2], hwver[1], hwver[0], vdrinfo[0], vdrinfo[1], vdrinfo[2], vdrinfo[3]);

	return true;

exit:
	return false;
}

int fimc_is_mcu_open_fw(struct v4l2_subdev *subdev, char *name, u8 **buf, ulong *buf_size)
{
	int ret = 0;
	ulong size = 0;
	const struct firmware *fw_blob = NULL;
	static char fw_name[100];
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nread;
	int fw_requested = 1;
	int retry_count = 0;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	info("%s started", __func__);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if (!mcu) {
		err("mcu is NULL");
		return -EINVAL;
	}

	client = mcu->client;

	//fw_sdcard = false;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	snprintf(fw_name, sizeof(fw_name), "%s%s", FIMC_IS_OIS_SDCARD_PATH, name);
	fp = filp_open(fw_name, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		info("mcu failed to open SDCARD fw!!!\n");
		goto request_fw;
	}

	fw_requested = 0;
	size = fp->f_path.dentry->d_inode->i_size;
	info("mcu start read sdcard, file path %s, size %lu Bytes\n", fw_name, size);

	*buf = vmalloc(size);
	if (!(*buf)) {
		err("failed to allocate memory");
		ret = -ENOMEM;
		goto p_err;
	}

	nread = vfs_read(fp, (char __user *)(*buf), size, &fp->f_pos);
	if (nread != size) {
		err("failed to read firmware file, %ld Bytes\n", nread);
		ret = -EIO;
		goto p_err;
	}

	memcpy(&mcu->vdrinfo_bin[0], *buf + 0x807C, sizeof(mcu->vdrinfo_bin));
	mcu->hw_bin[0] = *(*buf + 0x80FB);
	mcu->hw_bin[1] = *(*buf + 0x80FA);
	mcu->hw_bin[2] = *(*buf + 0x80F9);
	mcu->hw_bin[3] = *(*buf + 0x80F8);
	memcpy(ois_pinfo.header_ver, mcu->hw_bin, 4);
	memcpy(&ois_pinfo.header_ver[4], *buf + 0x807C, 4);

	//fw_sdcard = true;
	if (OIS_BIN_LEN >= nread) {
		//ois_device->not_crc_bin = true;
		err("mcu fw binary size = %ld.\n", nread);
	}

request_fw:
	if (fw_requested) {
		snprintf(fw_name, sizeof(fw_name), "%s", name);
		set_fs(old_fs);
		retry_count = 3;
		ret = request_firmware(&fw_blob, fw_name, &client->dev);
		while (--retry_count && ret == -EAGAIN) {
			err("request_firmware retry(count:%d)", retry_count);
			ret = request_firmware(&fw_blob, fw_name, &client->dev);
		}

		if (ret) {
			err("request_firmware is fail(ret:%d)", ret);
			ret = -EINVAL;
			goto p_err;
		}

		if (!fw_blob) {
			err("fw_blob is NULL");
			ret = -EINVAL;
			goto p_err;
		}

		if (!fw_blob->data) {
			err("fw_blob->data is NULL");
			ret = -EINVAL;
			goto p_err;
		}

		size = fw_blob->size;

		*buf = vmalloc(size);
		if (!(*buf)) {
			err("failed to allocate memory");
			ret = -ENOMEM;
			goto p_err;
		}

		memcpy((void *)(*buf), fw_blob->data, size);
		memcpy(&mcu->vdrinfo_bin[0], *buf + 0x807C, sizeof(mcu->vdrinfo_bin));
		mcu->hw_bin[0] = *(*buf + 0x80FB);
		mcu->hw_bin[1] = *(*buf + 0x80FA);
		mcu->hw_bin[2] = *(*buf + 0x80F9);
		mcu->hw_bin[3] = *(*buf + 0x80F8);
		memcpy(ois_pinfo.header_ver, mcu->hw_bin, 4);
		memcpy(&ois_pinfo.header_ver[4], *buf + 0x807C, 4);

		if (OIS_BIN_LEN >= size) {
			//ois_device->not_crc_bin = true;
			info("mcu fw binary size = %lu.", size);
		}

		info("mcu firmware is loaded from Phone binary.");
	}

p_err:
	*buf_size = size;
	info("[%s] mcu binary hw ver = %c%c%c%c, vdrinfo ver = %c%c%c%c", __func__,
		mcu->hw_bin[0], mcu->hw_bin[1], mcu->hw_bin[2], mcu->hw_bin[3],
		mcu->vdrinfo_bin[0], mcu->vdrinfo_bin[1], mcu->vdrinfo_bin[2], mcu->vdrinfo_bin[3]);

	if (!fw_requested) {
		if (!IS_ERR_OR_NULL(fp)) {
			filp_close(fp, current->files);
		}
		set_fs(old_fs);
	} else {
		if (!IS_ERR_OR_NULL(fw_blob)) {
			release_firmware(fw_blob);
		}
	}

	return ret;
}

int fimc_is_mcu_fw_revision_vdrinfo(u8 *fw_ver)
{
	int revision = 0;
	revision = revision + ((int)fw_ver[FW_RELEASE_YEAR] - 58) * 10000;
	revision = revision + ((int)fw_ver[FW_RELEASE_MONTH] - 64) * 100;
	revision = revision + ((int)fw_ver[FW_RELEASE_COUNT] - 48) * 10;
	revision = revision + (int)fw_ver[FW_RELEASE_COUNT + 1] - 48;

	return revision;
}

bool fimc_is_mcu_version_compare(u8 *fw_ver1, u8 *fw_ver2)
{
	if (fw_ver1[FW_DRIVER_IC] != fw_ver2[FW_DRIVER_IC]
		|| fw_ver1[FW_GYRO_SENSOR] != fw_ver2[FW_GYRO_SENSOR]
		|| fw_ver1[FW_MODULE_TYPE] != fw_ver2[FW_MODULE_TYPE]
		|| fw_ver1[FW_PROJECT] != fw_ver2[FW_PROJECT]) {
		return false;
	}

	return true;
}

void fimc_is_mcu_fw_update(struct fimc_is_core *core)
{

	u8 *buf = NULL;
	u8 *buf_cal = NULL;
	int ret = 0;
	int empty_check = 0;
	bool need_reset = false;
	int retry_count = 3;
	u32 address = 0;
	u32 data_size = 0;
	ulong size = 0;
	u8 SendData[256] = {0, };
	int vdrinfo_bin = 0;
	int vdrinfo_mcu = 0;
	struct i2c_client *client = NULL;
	struct fimc_is_mcu *mcu = NULL;
	struct v4l2_subdev *subdev = NULL;

	client = fimc_is_mcu_i2c_get_client(core);
	mcu = i2c_get_clientdata(client);
	subdev = mcu->subdev;

	info("%s started", __func__);

	ret = fimc_is_mcu_open_fw(subdev, FIMC_MCU_FW_NAME, &buf, &size);
	if (ret < 0) {
		err("mcu fw open failed");
		goto p_err;
	}

	ret = fimc_is_mcu_fw_version(subdev);
	if (ret) {
		if (!fimc_is_mcu_version_compare(mcu->hw_bin, mcu->hw_mcu)) {
			info("Do not update MCU firmware. HW binary ver = %c%c%c%c, module ver = %c%c%c%c",
				mcu->hw_bin[0], mcu->hw_bin[1], mcu->hw_bin[2], mcu->hw_bin[3],
				mcu->hw_mcu[0], mcu->hw_mcu[1], mcu->hw_mcu[2], mcu->hw_mcu[3]);
			goto p_err;
		}

		vdrinfo_bin = fimc_is_mcu_fw_revision_vdrinfo(mcu->vdrinfo_bin);
		vdrinfo_mcu = fimc_is_mcu_fw_revision_vdrinfo(mcu->vdrinfo_mcu);

		if (vdrinfo_bin <= vdrinfo_mcu) {
			err("Do not update MCU firmware. VDRINFO binary ver = %c%c%c%c, module ver = %c%c%c%c",
				mcu->vdrinfo_bin[0], mcu->vdrinfo_bin[1], mcu->vdrinfo_bin[2], mcu->vdrinfo_bin[3],
				mcu->vdrinfo_mcu[0], mcu->vdrinfo_mcu[1], mcu->vdrinfo_mcu[2], mcu->vdrinfo_mcu[3]);
			goto p_err;
		}
	}

	msleep(50);

retry:
	ret = fimc_is_mcu_validation(subdev, core);
	if (ret) {
		err("mcu fw validation failed, retry count = %d", retry_count);
		if (retry_count > 0) {
			msleep(500);
			need_reset = true;
			retry_count--;
			goto retry;
		} else {
			err("mcu fw validation failed. Do not try again.");
			goto p_err;
		}
	}

	empty_check = fimc_is_mcu_empty_check_status(subdev);

	ret = fimc_is_mcu_erase(subdev, memory_map.flashbase, 65536 - 2048);
	if (ret < 0) {
		err("mcu erase failed, retry count = %d", retry_count);
		if (retry_count > 0) {
			msleep(500);
			need_reset = true;
			retry_count--;
			goto retry;
		} else {
			err("mcu erase failed. Do not try again.");
			goto p_err;
		}
	}

	address = memory_map.flashbase;

	info("mcu start write fw data");

	buf_cal = buf;

	/* Write Data */
	while (size > 0) {
		int i;

		data_size = (u32)((size > FW_TRANS_SIZE) ? FW_TRANS_SIZE : size);
		/* write fw */
		for(i = 0; i < data_size; i++ ){
			SendData[i] = buf_cal[i];
		}

		ret = fimc_is_mcu_i2c_write(client, address, SendData, data_size);
		if (ret < 0) {
			err("failed to write fw");
			break;
		}
		address += data_size;
		buf_cal += data_size;
		size -= data_size;
	}

	info("mcu end write fw data");

	if (empty_check > 0) {
		if (fimc_is_mcu_empty_check_clear(subdev, core) < 0) {
			if (retry_count > 0) {
				retry_count--;
				goto retry;
			} else {
				goto p_err;
			}
		} else {			
			fimc_is_mcu_disconnect(subdev, core);
		}
	} else {
		fimc_is_mcu_disconnect(subdev, core);
	}

	msleep(100);

	info("%s mcu fw update completed.", __func__);

p_err:
	if (buf) {
		vfree(buf);
	}

	info("%s end", __func__);

	fimc_is_mcu_set_aperture_onboot(core);

	return;
}

bool fimc_is_ois_sine_wavecheck_mcu(struct fimc_is_core *core,
	int threshold, int *sinx, int *siny, int *result)
{
	u8 buf = 0, val = 0;
	int ret = 0, retries = 10;
	int sinx_count = 0, siny_count = 0;
	u8 u8_sinx_count[2] = {0, }, u8_siny_count[2] = {0, };
	u8 u8_sinx[2] = {0, }, u8_siny[2] = {0, };
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	struct fimc_is_mcu *mcu = NULL;

	client = fimc_is_mcu_i2c_get_client(core);
	mcu = i2c_get_clientdata(client);

	info("%s autotest started", __func__);

	ret = fimc_is_ois_i2c_write(client, 0x0052, (u8)threshold); /* error threshold level. */
	ret |= fimc_is_ois_i2c_write(client, 0x00BE, 0x01); /* OIS SEL (wide : 1 , tele : 2, both : 3 ). */
	ret |= fimc_is_ois_i2c_write(client, 0x0053, 0x0); /* count value for error judgement level. */
	ret |= fimc_is_ois_i2c_write(client, 0x0054, 0x05); /* frequency level for measurement. */
	ret |= fimc_is_ois_i2c_write(client, 0x0055, 0x34); /* amplitude level for measurement. */
	ret |= fimc_is_ois_i2c_write(client, 0x0056, 0x03); /* dummy pulse setting. */
	ret |= fimc_is_ois_i2c_write(client, 0x0057, 0x02); /* vyvle level for measurement. */
	ret |= fimc_is_ois_i2c_write(client, 0x0050, 0x01); /* start sine wave check operation */
	if (ret) {
		err("i2c write fail\n");
		goto exit;
	} else
		err("i2c write success\n");

	retries = 10;
	do {
		ret = fimc_is_ois_i2c_read(client, 0x0050, &val);
		if (ret) {
			err("i2c read fail\n");
			goto exit;
		}

		msleep(100);

		if (--retries < 0) {
			err("sine wave operation fail.\n");
			break;
		}
	} while (val);

	ret = fimc_is_ois_i2c_read(client, 0x0051, &buf);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	*result = (int)buf;

	ret = fimc_is_ois_i2c_read_multi(client, 0x00E4, u8_sinx_count, 2);
	sinx_count = (u8_sinx_count[1] << 8) | u8_sinx_count[0];
	if (sinx_count > 0x7FFF) {
		sinx_count = -((sinx_count ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00E6, u8_siny_count, 2);
	siny_count = (u8_siny_count[1] << 8) | u8_siny_count[0];
	if (siny_count > 0x7FFF) {
		siny_count = -((siny_count ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00E8, u8_sinx, 2);
	*sinx = (u8_sinx[1] << 8) | u8_sinx[0];
	if (*sinx > 0x7FFF) {
		*sinx = -((*sinx ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00EA, u8_siny, 2);
	*siny = (u8_siny[1] << 8) | u8_siny[0];
	if (*siny > 0x7FFF) {
		*siny = -((*siny ^ 0xFFFF) + 1);
	}
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	dbg_ois("threshold = %d, sinx = %d, siny = %d, sinx_count = %d, syny_count = %d\n",
		threshold, *sinx, *siny, sinx_count, siny_count);

	if (buf == 0x0) {
		return true;
	} else {
		return false;
	}

exit:
	*sinx = -1;
	*siny = -1;

	return false;
}

bool fimc_is_ois_auto_test_mcu(struct fimc_is_core *core,
					int threshold, bool *x_result, bool *y_result, int *sin_x, int *sin_y)
{
	int result = 0;
	bool value = false;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);
	struct fimc_is_mcu *mcu = NULL;
	//struct fimc_is_device_sensor *device;

#ifdef CONFIG_AF_HOST_CONTROL
	fimc_is_af_move_lens(core);
	msleep(100);
#endif

	info("%s autotest started", __func__);

	client = fimc_is_mcu_i2c_get_client(core);
	mcu = i2c_get_clientdata(client);

	value = fimc_is_ois_sine_wavecheck_mcu(core, threshold, sin_x, sin_y, &result);
	if (*sin_x == -1 && *sin_y == -1) {
		err("OIS device is not prepared.");
		*x_result = false;
		*y_result = false;

		return false;
	}

	if (value == true) {
		*x_result = true;
		*y_result = true;

		return true;
	} else {
		dbg_ois("OIS autotest is failed value = 0x%x\n", result);
		if ((result & 0x03) == 0x01) {
			*x_result = false;
			*y_result = true;
		} else if ((result & 0x03) == 0x02) {
			*x_result = true;
			*y_result = false;
		} else {
			*x_result = false;
			*y_result = false;
		}

		return false;
	}
}

int fimc_is_mcu_set_aperture(struct v4l2_subdev *subdev, int onoff)
{
	int ret = 0;
	u8 data = 0;
	int retry = 5;
	int value = 0;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	client = mcu->client;

	info("%s started onoff = %d", __FUNCTION__, onoff);

	switch (onoff) {
	case F1_5:
		value = 2;
		break;
	case F2_4:
		value = 1;
		break;
	default:
		info("%s: mode is not set.(mode = %d)\n", __func__, onoff);
		mcu->aperture->step = APERTURE_STEP_STATIONARY;
		goto exit;
	}

	/* wait control register to idle */
	do {
		ret = fimc_is_ois_i2c_read(client, 0x61, &data);
		if (ret) {
			err("i2c read fail\n");
			goto exit;
		}

		if (retry-- < 0) {
			err("wait idle state failed..");
			break;
		}
	} while (data);

	info("mcu status = %d", data);

	ret = fimc_is_ois_i2c_write(client, 0x63, value);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	/* start aperture control */
	ret = fimc_is_ois_i2c_write(client, 0x61, 0x01);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	if (value == 2)
		mcu->aperture->cur_value = F1_5;
	else if (value == 1)
		mcu->aperture->cur_value = F2_4;

	mcu->aperture->step = APERTURE_STEP_STATIONARY;

	msleep(mcu_init.aperture_delay_list[0]);

	return true;

exit:
	info("% Do not set aperture. onoff = %d", __FUNCTION__, onoff);

	return false;
}

int fimc_is_mcu_deinit_aperture(struct v4l2_subdev *subdev, int onoff)
{
	int ret = 0;
	u8 data = 0;
	int retry = 5;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	client = mcu->client;

	info("%s started onoff = %d", __FUNCTION__, onoff);

	/* wait control register to idle */
	do {
		ret = fimc_is_ois_i2c_read(client, 0x61, &data);
		if (ret) {
			err("i2c read fail\n");
			goto exit;
		}

		if (retry-- < 0) {
			err("wait idle state failed..");
			break;
		}
	} while (data);

	info("mcu status = %d", data);

	ret = fimc_is_ois_i2c_write(client, 0x63, 0x2);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	/* start aperture control */
	ret = fimc_is_ois_i2c_write(client, 0x61, 0x01);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	mcu->aperture->cur_value = F1_5;

	msleep(mcu_init.aperture_delay_list[0]);

	return true;

exit:
	return false;
}

void fimc_is_mcu_set_aperture_onboot(struct fimc_is_core *core)
{
	int ret = 0;
	u8 data = 0;
	int retry = 5;
	struct fimc_is_device_sensor *device = NULL;
	struct i2c_client *client = NULL;

	info("%s : E\n", __func__);

	device = &core->sensor[0];

	if (!device->mcu || !device->mcu->aperture) {
		err("%s, ois subdev is NULL", __func__);
		return;
	}

	client = device->mcu->client;

	/* wait control register to idle */
	do {
		ret = fimc_is_ois_i2c_read(client, 0x61, &data);
		if (ret) {
			err("i2c read fail\n");
		}

		if (retry-- < 0) {
			err("wait idle state failed..");
			break;
		}
	} while (data);

	info("mcu status = %d", data);

	ret = fimc_is_ois_i2c_write(client, 0x63, 0x2);
	if (ret) {
		err("i2c read fail\n");
	}

	/* start aperture control */
	ret = fimc_is_ois_i2c_write(client, 0x61, 0x01);
	if (ret) {
		err("i2c read fail\n");
	}

	device->mcu->aperture->cur_value = F1_5;

	msleep(mcu_init.aperture_delay_list[1]);

	info("%s : X\n", __func__);
}

bool fimc_is_mcu_halltest_aperture(struct v4l2_subdev *subdev, u16 *hall_value)
{
	int ret = 0;
	u8 data = 0;
	u8 data_array[2] = {0, };
	int retry = 3;
	bool result = true;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if (!mcu) {
		err("%s, mcu is NULL", __func__);
		return false;
	}

	client = mcu->client;

	info("%s started hall check", __FUNCTION__);

	/* wait control register to idle */
	do {
		ret = fimc_is_ois_i2c_read(client, 0x61, &data);
		if (ret) {
			err("i2c read fail\n");
			result = false;
			goto exit;
		}

		if (retry-- < 0) {
			err("wait idle state failed..");
			result = false;
			goto exit;
		}

		msleep(15);
	} while (data);

	info("mcu status = %d", data);

	ret = fimc_is_ois_i2c_write(client, 0x61, 0x10);
	if (ret) {
		err("i2c read fail\n");
		result = false;
		goto exit;
	}

	/* wait control register to idle */
	retry = 3;

	do {
		ret = fimc_is_ois_i2c_read(client, 0x61, &data);
		if (ret) {
			err("i2c read fail\n");
			result = false;
			goto exit;
		}

		if (retry-- < 0) {
			err("wait idle state failed..");
			result = false;
			goto exit;
		}

		msleep(5);
	} while (data);

	ret = fimc_is_ois_i2c_read_multi(client, 0x002C, data_array, 2);
	if (ret) {
		err("i2c read fail\n");
		result = false;
		goto exit;
	}

	*hall_value = (data_array[1] << 8) | data_array[0];

exit:
	info("%s aperture mode = %d, hall_value = 0x%04x, result = %d",
		__func__, mcu->aperture->cur_value, *hall_value, result);

	return result;
}

signed long long hex2float_kernel(unsigned int hex_data, int endian)
{
	const signed long long scale = SCALE;
	unsigned int s,e,m;
	signed long long res;

	if(endian == eBIG_ENDIAN) hex_data = SWAP32(hex_data);

	s = hex_data>>31, e = (hex_data>>23)&0xff, m = hex_data&0x7fffff;
	res = (e >= 150) ? ((scale*(8388608 + m))<<(e-150)) : ((scale*(8388608 + m))>>(150-e));
	if(s == 1) res *= -1;

	return res;
}

void fimc_is_status_check_mcu(struct i2c_client *client)
{
	u8 ois_status_check = 0;
	int retry_count = 0;

	do {
		fimc_is_ois_i2c_read(client, 0x000E, &ois_status_check);
		if (ois_status_check == 0x14)
			break;
		usleep_range(1000,1000);
		retry_count++;
	} while (retry_count < OIS_MEM_STATUS_RETRY);

	if (retry_count == OIS_MEM_STATUS_RETRY)
		err("%s, ois status check fail, retry_count(%d)\n", __func__, retry_count);

	if (ois_status_check == 0)
		err("%s, ois Memory access fail\n", __func__);
}

int fimc_calculate_shift_value_mcu(struct v4l2_subdev *subdev)
{
	int ret = 0;
	int i = 0;
	short ois_shift_x_cal = 0;
	short ois_shift_y_cal = 0;
	u8 cal_data[2];
	u8 shift_available = 0;
	struct fimc_is_ois *ois = NULL;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;
#ifdef OIS_CENTERING_SHIFT_ENABLE
	char *cal_buf;
	u32 Wide_XGG_Hex, Wide_YGG_Hex, Tele_XGG_Hex, Tele_YGG_Hex;
	signed long long Wide_XGG, Wide_YGG, Tele_XGG, Tele_YGG;
	u32 Image_Shift_x_Hex, Image_Shift_y_Hex;
	signed long long Image_Shift_x, Image_Shift_y;
	u8 read_multi[4] = {0, };
	signed long long Coef_angle_X , Coef_angle_Y;
	signed long long Wide_Xshift, Tele_Xshift, Wide_Yshift, Tele_Yshift;
	const signed long long scale = SCALE*SCALE;
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
	struct cam_hw_param *hw_param = NULL;
	struct fimc_is_device_sensor *device = NULL;
#endif

	WARN_ON(!subdev);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	client = mcu->client;
	ois = mcu->ois;

	I2C_MUTEX_LOCK(ois->i2c_lock);

	/* use user data */
	ret |= fimc_is_ois_i2c_write(ois->client, 0x000F, 0x40);
	cal_data[0] = 0x00;
	cal_data[1] = 0x02;
	ret |= fimc_is_ois_i2c_write_multi(client, 0x0010, cal_data, 4);
	ret |= fimc_is_ois_i2c_write(client, 0x000E, 0x04);
	if (ret < 0) {
#ifdef USE_CAMERA_HW_BIG_DATA
		device = v4l2_get_subdev_hostdata(subdev);
		if (device) {
			fimc_is_sec_get_hw_param(&hw_param, device->position);
		}
		if (hw_param)
			hw_param->i2c_ois_err_cnt++;
#endif
		err("ois user data write is fail");
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}

	fimc_is_status_check_mcu(client);
#ifdef OIS_CENTERING_SHIFT_ENABLE
	fimc_is_ois_i2c_read_multi(client, 0x0254, read_multi, 4);
	Wide_XGG_Hex = (read_multi[3] << 24) | (read_multi[2] << 16) | (read_multi[1] << 8) | (read_multi[0]);

	fimc_is_ois_i2c_read_multi(client, 0x0554, read_multi, 4);
	Tele_XGG_Hex = (read_multi[3] << 24) | (read_multi[2] << 16) | (read_multi[1] << 8) | (read_multi[0]);

	fimc_is_ois_i2c_read_multi(client, 0x0258, read_multi, 4);
	Wide_YGG_Hex = (read_multi[3] << 24) | (read_multi[2] << 16) | (read_multi[1] << 8) | (read_multi[0]);

	fimc_is_ois_i2c_read_multi(client, 0x0558, read_multi, 4);
	Tele_YGG_Hex = (read_multi[3] << 24) | (read_multi[2] << 16) | (read_multi[1] << 8) | (read_multi[0]);

	Wide_XGG = hex2float_kernel(Wide_XGG_Hex,eLIT_ENDIAN); // unit : 1/SCALE
	Wide_YGG = hex2float_kernel(Wide_YGG_Hex,eLIT_ENDIAN); // unit : 1/SCALE
	Tele_XGG = hex2float_kernel(Tele_XGG_Hex,eLIT_ENDIAN); // unit : 1/SCALE
	Tele_YGG = hex2float_kernel(Tele_YGG_Hex,eLIT_ENDIAN); // unit : 1/SCALE

	fimc_is_sec_get_cal_buf(&cal_buf);

	Image_Shift_x_Hex = (cal_buf[0x6C7C+3] << 24) | (cal_buf[0x6C7C+2] << 16) | (cal_buf[0x6C7C+1] << 8) | (cal_buf[0x6C7C]);
	Image_Shift_y_Hex = (cal_buf[0x6C80+3] << 24) | (cal_buf[0x6C80+2] << 16) | (cal_buf[0x6C80+1] << 8) | (cal_buf[0x6C80]);

	Image_Shift_x = hex2float_kernel(Image_Shift_x_Hex,eLIT_ENDIAN); // unit : 1/SCALE
	Image_Shift_y = hex2float_kernel(Image_Shift_y_Hex,eLIT_ENDIAN); // unit : 1/SCALE

	Image_Shift_y += 90 * SCALE;

	#define ABS(a)				((a) > 0 ? (a) : -(a))

	// Calc w/t x shift
	//=======================================================
	Coef_angle_X = (ABS(Image_Shift_x) > SH_THRES) ? Coef_angle_max : RND_DIV(ABS(Image_Shift_x), 228);

	Wide_Xshift = Gyrocode * Coef_angle_X * Wide_XGG;
	Tele_Xshift = Gyrocode * Coef_angle_X * Tele_XGG;

	Wide_Xshift = (Image_Shift_x > 0) ? Wide_Xshift	: Wide_Xshift*-1;
	Tele_Xshift = (Image_Shift_x > 0) ? Tele_Xshift*-1 : Tele_Xshift;

	// Calc w/t y shift
	//=======================================================
	Coef_angle_Y = (ABS(Image_Shift_y) > SH_THRES) ? Coef_angle_max : RND_DIV(ABS(Image_Shift_y), 228);

	Wide_Yshift = Gyrocode * Coef_angle_Y * Wide_YGG;
	Tele_Yshift = Gyrocode * Coef_angle_Y * Tele_YGG;

	Wide_Yshift = (Image_Shift_y > 0) ? Wide_Yshift*-1 : Wide_Yshift;
	Tele_Yshift = (Image_Shift_y > 0) ? Tele_Yshift*-1 : Tele_Yshift;

	// Calc output variable
	//=======================================================
	ois_centering_shift_x = (int)RND_DIV(Wide_Xshift, scale);
	ois_centering_shift_y = (int)RND_DIV(Wide_Yshift, scale);
	ois_centering_shift_x_rear2 = (int)RND_DIV(Tele_Xshift, scale);
	ois_centering_shift_y_rear2 = (int)RND_DIV(Tele_Yshift, scale);
#endif

	fimc_is_ois_i2c_read(client, 0x0100, &shift_available);
	if (shift_available != 0x11) {
		ois->ois_shift_available = false;
		dbg_ois("%s, OIS AF CAL(0x%x) does not installed.\n", __func__, shift_available);
	} else {
		ois->ois_shift_available = true;

		/* OIS Shift CAL DATA */
		for (i = 0; i < 9; i++) {
			cal_data[0] = 0;
			cal_data[1] = 0;
			fimc_is_ois_i2c_read_multi(client, 0x0110 + (i * 2), cal_data, 2);
			ois_shift_x_cal = (cal_data[1] << 8) | (cal_data[0]);

			cal_data[0] = 0;
			cal_data[1] = 0;
			fimc_is_ois_i2c_read_multi(client, 0x0122 + (i * 2), cal_data, 2);
			ois_shift_y_cal = (cal_data[1] << 8) | (cal_data[0]);

			if (ois_shift_x_cal > (short)32767)
				ois_shift_x[i] = ois_shift_x_cal - 65536;
			else
				ois_shift_x[i] = ois_shift_x_cal;

			if (ois_shift_y_cal > (short)32767)
				ois_shift_y[i] = ois_shift_y_cal - 65536;
			else
				ois_shift_y[i] = ois_shift_y_cal;
		}
	}

	shift_available = 0;
	fimc_is_ois_i2c_read(client, 0x0101, &shift_available);
	if (shift_available != 0x11) {
		dbg_ois("%s, REAR2 OIS AF CAL(0x%x) does not installed.\n", __func__, shift_available);
	} else {
		/* OIS Shift CAL DATA REAR2 */
		for (i = 0; i < 9; i++) {
			cal_data[0] = 0;
			cal_data[1] = 0;
			fimc_is_ois_i2c_read_multi(client, 0x0140 + (i * 2), cal_data, 2);
			ois_shift_x_cal = (cal_data[1] << 8) | (cal_data[0]);

			cal_data[0] = 0;
			cal_data[1] = 0;
			fimc_is_ois_i2c_read_multi(client, 0x0152 + (i * 2), cal_data, 2);
			ois_shift_y_cal = (cal_data[1] << 8) | (cal_data[0]);

			if (ois_shift_x_cal > (short)32767)
				ois_shift_x_rear2[i] = ois_shift_x_cal - 65536;
			else
				ois_shift_x_rear2[i] = ois_shift_x_cal;

			if (ois_shift_y_cal > (short)32767)
				ois_shift_y_rear2[i] = ois_shift_y_cal - 65536;
			else
				ois_shift_y_rear2[i] = ois_shift_y_cal;

		}
	}

	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	return ret;
}

static int __init fimc_is_mcu_get_hw_rev(char *arg)
{
	get_option(&arg, &sec_hw_rev);
	return 0;
}

early_param("androidboot.revision", fimc_is_mcu_get_hw_rev);

int fimc_is_ois_init_mcu(struct v4l2_subdev *subdev)
{
	int ret = 0;
#ifdef USE_OIS_SLEEP_MODE
	u8 read_gyrocalcen = 0;
#endif
	u8 val = 0;
	u8 gyro_orientation = 0;
	u8 wx_pole = 0;
	u8 wy_pole = 0;
#ifdef CAMERA_2ND_OIS
	u8 tx_pole = 0;
	u8 ty_pole = 0;
#endif
	int retries = 10;
	struct fimc_is_mcu *mcu = NULL;
	struct fimc_is_ois *ois = NULL;
	struct i2c_client *client = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!subdev);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	sensor_peri = mcu->sensor_peri;
	if (!sensor_peri) {
		err("%s, sensor_peri is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	module = sensor_peri->module;
	if (!module) {
		err("%s, module is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	client = mcu->client;
	if (!client) {
		err("%s, client is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	ois = mcu->ois;
	ois->ois_mode = OPTICAL_STABILIZATION_MODE_OFF;
	ois->pre_ois_mode = OPTICAL_STABILIZATION_MODE_OFF;
	ois->coef = 0;
	ois->pre_coef = 255;
	ois->fadeupdown = false;
	ois->initial_centering_mode = false;
	ois->af_pos_wide = 0;
	ois->af_pos_tele = 0;
#ifdef CAMERA_2ND_OIS
	ois->ois_power_mode = -1;
#endif
	ois_pinfo.reset_check = false;

	if (mcu->aperture) {
		mcu->aperture->cur_value = F2_4;
		mcu->aperture->new_value = F2_4;
		mcu->aperture->start_value = F2_4;
		mcu->aperture->step = APERTURE_STEP_STATIONARY;
	}

#ifdef USE_OIS_SLEEP_MODE
	I2C_MUTEX_LOCK(ois->i2c_lock);
	fimc_is_ois_i2c_read(ois->client, 0x00BF, &read_gyrocalcen);
	if ((read_gyrocalcen == 0x01 && module->position == SENSOR_POSITION_REAR2) || //tele already enabled
		(read_gyrocalcen == 0x02 && module->position == SENSOR_POSITION_REAR)) { //wide already enabled
		ois->ois_shift_available = true;
		info("%s %d sensor(%d) is already initialized.\n", __func__, __LINE__, ois->device);
		ret = fimc_is_ois_i2c_write(ois->client, 0x00BF, 0x03);
		if (ret < 0)
			err("ois 0x00BF write is fail");
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}
	I2C_MUTEX_UNLOCK(ois->i2c_lock);
#else
	if ((ois_wide_init == true && module->position == SENSOR_POSITION_REAR) ||
		(ois_tele_init == true && module->position == SENSOR_POSITION_REAR2)) {
		info("%s %d sensor(%d) is already initialized.\n", __func__, __LINE__, ois->device);
		ois_wide_init = ois_tele_init = true;
		ois->ois_shift_available = true;
	}
#endif

	 if (!ois_hw_check) {
		I2C_MUTEX_LOCK(ois->i2c_lock);

		do {
			ret = fimc_is_ois_i2c_read(client, 0x01, &val);
			if (ret != 0) {
				val = -EIO;
				break;
			}
			msleep(3);
			if (--retries < 0) {
				err("Read status failed!!!!, data = 0x%04x\n", val);
				break;
			}
		} while (val != 0x01);

		if (val == 0x01) {
			ret = fimc_is_ois_i2c_write_multi(client, 0x0254, ois_pinfo.wide_xgg, 6);
			ret |= fimc_is_ois_i2c_write_multi(client, 0x0258, ois_pinfo.wide_ygg, 6);
			ret |= fimc_is_ois_i2c_write_multi(client, 0x0554, ois_pinfo.tele_xgg, 6);
			ret |= fimc_is_ois_i2c_write_multi(client, 0x0558, ois_pinfo.tele_ygg, 6);
			if (ret < 0)
				err("ois gyro data write is fail");

			ret = fimc_is_ois_i2c_write_multi(client, 0x0442, ois_pinfo.wide_xcoef, 4);
			ret |= fimc_is_ois_i2c_write_multi(client, 0x0444, ois_pinfo.wide_ycoef, 4);
			ret |= fimc_is_ois_i2c_write_multi(client, 0x0446, ois_pinfo.tele_xcoef, 4);
			ret |= fimc_is_ois_i2c_write_multi(client, 0x0448, ois_pinfo.tele_ycoef, 4);
			if (ret < 0)
				err("ois coef data write is fail");

			ret = fimc_is_ois_i2c_write(client, 0x0440, 0x01);
			if (ret < 0)
				err("ois dual shift is fail");

			wx_pole = mcu_init.ois_gyro_list[0];
			wy_pole = mcu_init.ois_gyro_list[1];
			gyro_orientation = mcu_init.ois_gyro_list[2];
#ifdef CAMERA_2ND_OIS
			tx_pole = mcu_init.ois_gyro_list[3];
			ty_pole = mcu_init.ois_gyro_list[4];
#endif
			ret = fimc_is_ois_i2c_write(client, 0x0240, wx_pole);
			ret |= fimc_is_ois_i2c_write(client, 0x0241, wy_pole);
			ret |= fimc_is_ois_i2c_write(client, 0x0242, gyro_orientation);
#ifdef CAMERA_2ND_OIS
			ret |= fimc_is_ois_i2c_write(client, 0x0552, tx_pole);
			ret |= fimc_is_ois_i2c_write(client, 0x0553, ty_pole);
#endif
			info("%s gyro init data applied\n", __func__);
		}

		I2C_MUTEX_UNLOCK(ois->i2c_lock);

		ois_hw_check = true;
 	}

	if (module->position == SENSOR_POSITION_REAR2) {
		ois_tele_init = true;
	} else if (module->position == SENSOR_POSITION_REAR) {
		ois_wide_init = true;
	}

	info("%s\n", __func__);
	return ret;
}

int fimc_is_ois_deinit_mcu(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	WARN_ON(!subdev);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu subdev is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	client = mcu->client;
	if (!client) {
		err("client is NULL");
		ret = -EINVAL;
		return ret;
	}

	if (ois_hw_check) {
		ret = fimc_is_ois_i2c_write(client, 0x0000, 0x00);
		if (ret) {
			err("i2c read fail\n");
		}

		usleep_range(2000, 2100);
	}

	ois_fadeupdown = false;
	ois_hw_check = false;

	dbg_ois("%s\n", __func__);

	return ret;
}

int fimc_is_ois_set_ggfadeupdown_mcu(struct v4l2_subdev *subdev, int up, int down)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;
	u8 status = 0;
	int retries = 100;
	u8 data[2];
	u8 write_data[4] = {0,};
#ifdef USE_OIS_SLEEP_MODE
	u8 read_sensorStart = 0;
#endif

	WARN_ON(!subdev);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu subdev is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	ois = mcu->ois;
	client = mcu->client;
	if (!client) {
		err("client is NULL");
		return -EINVAL;
	}

	dbg_ois("%s up:%d down:%d\n", __func__, up, down);

	I2C_MUTEX_LOCK(ois->i2c_lock);

#ifdef CAMERA_2ND_OIS
	if (ois->ois_power_mode < OIS_POWER_MODE_SINGLE) {
		ret = fimc_is_ois_i2c_write(client, 0x00BE, 0x03);
		if (ret < 0) {
			err("ois Actuator output write is fail");
			I2C_MUTEX_UNLOCK(ois->i2c_lock);
			return ret;
		}
	}
#else
	ret = fimc_is_ois_i2c_write(client, 0x00BE, 0x03);
	if (ret < 0) {
		err("ois Actuator output write is fail");
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}
#endif

	/* Wide af position value */
	ret = fimc_is_ois_i2c_write(client, 0x003A, 0x00);
	if (ret < 0) {
		err("ois wide af write is fail");
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}

	/* Tele af position value */
	ret = fimc_is_ois_i2c_write(client, 0x003B, 0x00);
	if (ret < 0) {
		err("ois tele af write is fail");
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}

	ret = fimc_is_ois_i2c_write(client, 0x0039, 0x01);
	if (ret < 0) {
		err("ois cactrl write is fail");
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}

	/* angle compensation 1.5->1.25
	 * before addr:0x0000, data:0x01
	 * write 0x3F558106
	 * write 0x3F558106
	 */
	write_data[0] = 0x06;
	write_data[1] = 0x81;
	write_data[2] = 0x55;
	write_data[3] = 0x3F;
	fimc_is_ois_i2c_write_multi(client, 0x0348, write_data, 6);

	write_data[0] = 0x06;
	write_data[1] = 0x81;
	write_data[2] = 0x55;
	write_data[3] = 0x3F;
	fimc_is_ois_i2c_write_multi(client, 0x03D8, write_data, 6);

#ifdef USE_OIS_SLEEP_MODE
	/* if camera is already started, skip VDIS setting */
	fimc_is_ois_i2c_read(client, 0x00BF, &read_sensorStart);
	if (read_sensorStart == 0x03) {
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}
#endif
	/* set fadeup */
	data[0] = up & 0xFF;
	data[1] = (up >> 8) & 0xFF;
	ret = fimc_is_ois_i2c_write_multi(client, 0x0238, data, 4);
	if (ret < 0)
		err("%s i2c write fail\n", __func__);

	/* set fadedown */
	data[0] = down & 0xFF;
	data[1] = (down >> 8) & 0xFF;
	ret = fimc_is_ois_i2c_write_multi(client, 0x023a, data, 4);
	if (ret < 0)
		err("%s i2c write fail\n", __func__);

	/* wait idle status
	 * 100msec delay is needed between "ois_power_on" and "ois_mode_s6".
	 */
	do {
		fimc_is_ois_i2c_read(client, 0x0001, &status);
		if (status == 0x01 || status == 0x13)
			break;
		if (--retries < 0) {
			err("%s : read register fail!. status: 0x%x\n", __func__, status);
			ret = -1;
			break;
		}
		usleep_range(1000, 1100);
	} while (status != 0x01);

	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	dbg_ois("%s retryCount = %d , status = 0x%x\n", __func__, 100 - retries, status);

	return ret;
}

int fimc_is_set_ois_mode_mcu(struct v4l2_subdev *subdev, int mode)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	WARN_ON(!subdev);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu subdev is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	ois = mcu->ois;

	if (ois->fadeupdown == false) {
		if (ois_fadeupdown == false) {
			ois_fadeupdown = true;
			fimc_is_ois_set_ggfadeupdown_mcu(subdev, 1000, 1000);
		}
		ois->fadeupdown = true;
	}

	client = mcu->client;
	if (!client) {
		err("client is NULL");
		ret = -EINVAL;
		return ret;
	}

	if (mode == ois->pre_ois_mode) {
		return ret;
	}

	ois->pre_ois_mode = mode;
	info("%s: ois_mode value(%d)\n", __func__, mode);

	I2C_MUTEX_LOCK(ois->i2c_lock);
	switch(mode) {
		case OPTICAL_STABILIZATION_MODE_STILL:
			fimc_is_ois_i2c_write(client, 0x0002, 0x00);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		case OPTICAL_STABILIZATION_MODE_VIDEO:
			fimc_is_ois_i2c_write(client, 0x0002, 0x01);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		case OPTICAL_STABILIZATION_MODE_CENTERING:
			fimc_is_ois_i2c_write(client, 0x0002, 0x05);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		case OPTICAL_STABILIZATION_MODE_STILL_ZOOM:
			fimc_is_ois_i2c_write(client, 0x0002, 0x13);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		case OPTICAL_STABILIZATION_MODE_VDIS:
			fimc_is_ois_i2c_write(client, 0x0002, 0x14);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		case OPTICAL_STABILIZATION_MODE_SINE_X:
			fimc_is_ois_i2c_write(client, 0x0018, 0x01);
			fimc_is_ois_i2c_write(client, 0x0019, 0x01);
			fimc_is_ois_i2c_write(client, 0x001A, 0x2D);
			fimc_is_ois_i2c_write(client, 0x0002, 0x03);
			msleep(20);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		case OPTICAL_STABILIZATION_MODE_SINE_Y:
			fimc_is_ois_i2c_write(client, 0x0018, 0x02);
			fimc_is_ois_i2c_write(client, 0x0019, 0x01);
			fimc_is_ois_i2c_write(client, 0x001A, 0x2D);
			fimc_is_ois_i2c_write(client, 0x0002, 0x03);
			msleep(20);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		default:
			dbg_ois("%s: ois_mode value(%d)\n", __func__, mode);
			break;
	}
	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	return ret;
}

int fimc_is_ois_shift_compensation_mcu(struct v4l2_subdev *subdev, int position, int resolution)
{
	int ret = 0;
	struct fimc_is_ois *ois;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	int position_changed;

	WARN_ON(!subdev);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu subdev is NULL", __func__);
		ret = -EINVAL;
		goto p_err;
	}

	sensor_peri = mcu->sensor_peri;
	if (!sensor_peri) {
		err("%s, sensor_peri is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	module = sensor_peri->module;
	if (!module) {
		err("%s, module is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	ois = mcu->ois;
	client = mcu->client;

	position_changed = position / 2;

	I2C_MUTEX_LOCK(ois->i2c_lock);

	if (module->position == SENSOR_POSITION_REAR && ois->af_pos_wide != position_changed) {
		/* Wide af position value */
		ret = fimc_is_ois_i2c_write(client, 0x003A, (u8)position_changed);
		if (ret < 0) {
			err("ois wide af write is fail");
			I2C_MUTEX_UNLOCK(ois->i2c_lock);
			return ret;
		}
		ois->af_pos_wide = position_changed;
	} else if (module->position == SENSOR_POSITION_REAR2 && ois->af_pos_tele != position_changed) {
	/* Tele af position value */
		ret = fimc_is_ois_i2c_write(client, 0x003B, (u8)position_changed);
		if (ret < 0) {
			err("ois tele af write is fail");
			I2C_MUTEX_UNLOCK(ois->i2c_lock);
			return ret;
		}
		ois->af_pos_tele = position_changed;
	}

	I2C_MUTEX_UNLOCK(ois->i2c_lock);

p_err:
	return ret;
}

int fimc_is_ois_self_test_mcu(struct fimc_is_core *core)
{
	int ret = 0;
	u8 val = 0;
	u8 reg_val = 0, x = 0, y = 0;
	u16 x_gyro_log = 0, y_gyro_log = 0;
	int retries = 30;
	struct i2c_client *client = fimc_is_mcu_i2c_get_client(core);

	info("%s : E\n", __func__);

	ret = fimc_is_ois_i2c_write(client, 0x0014, 0x08);
	if (ret) {
		err("i2c write fail\n");
	}

	do {
		ret = fimc_is_ois_i2c_read(client, 0x0014, &val);
		if (ret != 0) {
			val = -EIO;
			break;
		}
		msleep(20);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%04x\n", val);
			break;
		}
	} while (val);

	ret = fimc_is_ois_i2c_read(client, 0x0004, &val);
	if (ret != 0) {
		val = -EIO;
	}

	/* Gyro selfTest result */
	fimc_is_ois_i2c_read(client, 0x00EC, &reg_val);
	x = reg_val;
	fimc_is_ois_i2c_read(client, 0x00ED, &reg_val);
	x_gyro_log = (reg_val << 8) | x;

	fimc_is_ois_i2c_read(client, 0x00EE, &reg_val);
	y = reg_val;
	fimc_is_ois_i2c_read(client, 0x00EF, &reg_val);
	y_gyro_log = (reg_val << 8) | y;

	info("%s(GSTLOG0=%d, GSTLOG1=%d)\n", __func__, x_gyro_log, y_gyro_log);

	info("%s(%d) : X\n", __func__, val);
	return (int)val;
}

#ifdef CAMERA_2ND_OIS
bool fimc_is_ois_sine_wavecheck_rear2_mcu(struct fimc_is_core *core,
					int threshold, int *sinx, int *siny, int *result,
					int *sinx_2nd, int *siny_2nd)
{
	u8 buf = 0, val = 0;
	int ret = 0, retries = 10;
	int sinx_count = 0, siny_count = 0;
	int sinx_count_2nd = 0, siny_count_2nd = 0;
	u8 u8_sinx_count[2] = {0, }, u8_siny_count[2] = {0, };
	u8 u8_sinx[2] = {0, }, u8_siny[2] = {0, };
	struct i2c_client *client = fimc_is_mcu_i2c_get_client(core);

	ret = fimc_is_ois_i2c_write(client, 0x00BE, 0x03); /* OIS SEL (wide : 1 , tele : 2, both : 3 ). */
	ret |= fimc_is_ois_i2c_write(client, 0x0052, (u8)threshold); /* error threshold level. */
	ret |= fimc_is_ois_i2c_write(client, 0x005B, (u8)threshold); /* error threshold level. */
	ret |= fimc_is_ois_i2c_write(client, 0x0053, 0x0); /* count value for error judgement level. */
	ret |= fimc_is_ois_i2c_write(client, 0x0054, 0x05); /* frequency level for measurement. */
	ret |= fimc_is_ois_i2c_write(client, 0x0055, 0x2A); /* amplitude level for measurement. */
	ret |= fimc_is_ois_i2c_write(client, 0x0056, 0x03); /* dummy pulse setting. */
	ret |= fimc_is_ois_i2c_write(client, 0x0057, 0x02); /* vyvle level for measurement. */
	ret |= fimc_is_ois_i2c_write(client, 0x0050, 0x01); /* start sine wave check operation */
	if (ret) {
		err("i2c write fail\n");
		goto exit;
	}

	retries = 10;
	do {
		ret = fimc_is_ois_i2c_read(client, 0x0050, &val);
		if (ret) {
			err("i2c read fail\n");
			goto exit;
		}

		msleep(100);

		if (--retries < 0) {
			err("sine wave operation fail.\n");
			break;
		}
	} while (val);

	ret = fimc_is_ois_i2c_read(client, 0x0051, &buf);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	*result = (int)buf;

	ret = fimc_is_ois_i2c_read_multi(client, 0x00C0, u8_sinx_count, 2);
	sinx_count = (u8_sinx_count[1] << 8) | u8_sinx_count[0];
	if (sinx_count > 0x7FFF) {
		sinx_count = -((sinx_count ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00C2, u8_siny_count, 2);
	siny_count = (u8_siny_count[1] << 8) | u8_siny_count[0];
	if (siny_count > 0x7FFF) {
		siny_count = -((siny_count ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00C4, u8_sinx, 2);
	*sinx = (u8_sinx[1] << 8) | u8_sinx[0];
	if (*sinx > 0x7FFF) {
		*sinx = -((*sinx ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00C6, u8_siny, 2);
	*siny = (u8_siny[1] << 8) | u8_siny[0];
	if (*siny > 0x7FFF) {
		*siny = -((*siny ^ 0xFFFF) + 1);
	}

	ret |= fimc_is_ois_i2c_read_multi(client, 0x00E4, u8_sinx_count, 2);
	sinx_count_2nd = (u8_sinx_count[1] << 8) | u8_sinx_count[0];
	if (sinx_count_2nd > 0x7FFF) {
		sinx_count_2nd = -((sinx_count_2nd ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00E6, u8_siny_count, 2);
	siny_count_2nd = (u8_siny_count[1] << 8) | u8_siny_count[0];
	if (siny_count_2nd > 0x7FFF) {
		siny_count_2nd = -((siny_count_2nd ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00E8, u8_sinx, 2);
	*sinx_2nd = (u8_sinx[1] << 8) | u8_sinx[0];
	if (*sinx_2nd > 0x7FFF) {
		*sinx_2nd = -((*sinx_2nd ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00EA, u8_siny, 2);
	*siny_2nd = (u8_siny[1] << 8) | u8_siny[0];
	if (*siny_2nd > 0x7FFF) {
		*siny_2nd = -((*siny_2nd ^ 0xFFFF) + 1);
	}

	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	info("threshold = %d, sinx = %d, siny = %d, sinx_count = %d, syny_count = %d\n",
		threshold, *sinx, *siny, sinx_count, siny_count);

	info("threshold = %d, sinx_2nd = %d, siny_2nd = %d, sinx_count_2nd = %d, syny_count_2nd = %d\n",
		threshold, *sinx_2nd, *siny_2nd, sinx_count_2nd, siny_count_2nd);

	if (buf == 0x0) {
		return true;
	} else {
		return false;
	}

exit:
	*sinx = -1;
	*siny = -1;
	*sinx_2nd = -1;
	*siny_2nd = -1;
	return false;
}

bool fimc_is_ois_auto_test_rear2_mcu(struct fimc_is_core *core,
					int threshold, bool *x_result, bool *y_result, int *sin_x, int *sin_y,
					bool *x_result_2nd, bool *y_result_2nd, int *sin_x_2nd, int *sin_y_2nd)
{
	int result = 0;
	bool value = false;

#ifdef CONFIG_AF_HOST_CONTROL
#ifdef CAMERA_REAR2_AF
	fimc_is_af_move_lens_rear2(core);
	msleep(100);
#endif
	fimc_is_af_move_lens(core);
	msleep(100);
#endif

	value = fimc_is_ois_sine_wavecheck_rear2_mcu(core, threshold, sin_x, sin_y, &result,
				sin_x_2nd, sin_y_2nd);

	if (*sin_x == -1 && *sin_y == -1) {
		err("OIS device is not prepared.");
		*x_result = false;
		*y_result = false;

		return false;
	}

	if (*sin_x_2nd == -1 && *sin_y_2nd == -1) {
		err("OIS 2 device is not prepared.");
		*x_result_2nd = false;
		*y_result_2nd = false;

		return false;
	}

	if (value == true) {
		*x_result = true;
		*y_result = true;
		*x_result_2nd = true;
		*y_result_2nd = true;

		return true;
	} else {
		err("OIS autotest_2nd is failed result (0x0051) = 0x%x\n", result);
		if ((result & 0x03) == 0x00) {
			*x_result = true;
			*y_result = true;
		} else if ((result & 0x03) == 0x01) {
			*x_result = false;
			*y_result = true;
		} else if ((result & 0x03) == 0x02) {
			*x_result = true;
			*y_result = false;
		} else {
			*x_result = false;
			*y_result = false;
		}

		if ((result & 0x30) == 0x00) {
			*x_result_2nd = true;
			*y_result_2nd = true;
		} else if ((result & 0x30) == 0x10) {
			*x_result_2nd = false;
			*y_result_2nd = true;
		} else if ((result & 0x30) == 0x20) {
			*x_result_2nd = true;
			*y_result_2nd = false;
		} else {
			*x_result_2nd = false;
			*y_result_2nd = false;
		}

		return false;
	}
}

int fimc_is_ois_set_power_mode_mcu(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;
	struct fimc_is_core *core;
	struct fimc_is_dual_info *dual_info = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	mcu = (struct fimc_is_mcu*)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu subdev is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	sensor_peri = mcu->sensor_peri;
	if (!sensor_peri) {
		err("%s, sensor_peri is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	module = sensor_peri->module;
	if (!module) {
		err("%s, module is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	client = mcu->client;
	if (!client) {
		err("client is NULL");
		return -EINVAL;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core is null");
		return -EINVAL;
	}

	ois = mcu->ois;
	if(!ois) {
		err("%s, ois subdev is NULL", __func__);
		return -EINVAL;
	}

	dual_info = &core->dual_info;

	I2C_MUTEX_LOCK(ois->i2c_lock);

	if ((dual_info->mode != FIMC_IS_DUAL_MODE_NOTHING)
		|| (dual_info->mode == FIMC_IS_DUAL_MODE_NOTHING &&
			module->position == SENSOR_POSITION_REAR2)) {
		ret = fimc_is_ois_i2c_write(client, 0x00BE, 0x03);
		ois->ois_power_mode = OIS_POWER_MODE_DUAL;
	} else {
		ret = fimc_is_ois_i2c_write(client, 0x00BE, 0x01);
		ois->ois_power_mode = OIS_POWER_MODE_SINGLE;
	}

	if (ret < 0)
		err("ois dual setting is fail");
	else
		info("%s ois power setting is %d\n", __func__, ois->ois_power_mode);

	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	return ret;
}
#endif

bool fimc_is_ois_check_fw_mcu(struct fimc_is_core *core)
{
	u8 *buf = NULL;
	int ret = 0;
	ulong size = 0;
	struct i2c_client *client = NULL;
	struct fimc_is_mcu *mcu = NULL;
	struct v4l2_subdev *subdev = NULL;

	client = fimc_is_mcu_i2c_get_client(core);
	mcu = i2c_get_clientdata(client);
	subdev = mcu->subdev;

	ret = fimc_is_mcu_fw_version(subdev);
	if (!ret) {
		err("Failed to read ois fw version.");
		return false;
	}

	ret = fimc_is_mcu_open_fw(subdev, FIMC_MCU_FW_NAME, &buf, &size);
	if (ret < 0) {
		err("mcu fw open failed");
	}

	if (buf) {
		vfree(buf);
	}

	return true;
}

void fimc_is_ois_enable_mcu(struct fimc_is_core *core)
{
	int ret = 0;
	struct i2c_client *client = fimc_is_mcu_i2c_get_client(core);

	dbg_ois("%s : E\n", __func__);

	ret = fimc_is_ois_i2c_write(client, 0x02, 0x00);
	if (ret) {
		err("i2c write fail\n");
	}

	ret = fimc_is_ois_i2c_write(client, 0x00, 0x01);
	if (ret) {
		err("i2c write fail\n");
	}

	dbg_ois("%s : X\n", __func__);
}

#ifdef CONFIG_SENSORCORE_MCU_CONTROL
void fimc_is_ois_reset_mcu(void *ois_core)
{
	int ret = 0;
	struct fimc_is_core *core = (struct fimc_is_core *)ois_core;
	struct fimc_is_device_sensor *device = NULL;
	struct i2c_client *client = NULL;
	bool camera_running = false;

	info("%s : E\n", __func__);

	device = &core->sensor[0];

	if (!device->mcu || !device->mcu->ois) {
		err("%s, ois subdev is NULL", __func__);
		return;
	}

	client = device->mcu->client;

	camera_running = fimc_is_vendor_check_camera_running(SENSOR_POSITION_REAR);

	if (camera_running) {
		info("%s : camera is running. reset ois gyro.\n", __func__);

		ret = fimc_is_ois_i2c_write(client, 0x0002, 0x16);
		if (ret) {
			err("i2c write fail\n");
		}

		ois_pinfo.reset_check= true;
	} else {
		ois_pinfo.reset_check= false;
		info("%s : camera is not running.\n", __func__);
	}

	info("%s : X\n", __func__);
}
#endif

bool fimc_is_ois_gyro_cal_mcu(struct fimc_is_core *core, long *x_value, long *y_value)
{
	int ret = 0;
	u8 val = 0, x = 0, y = 0;
	int retries = 30;
	int scale_factor = OIS_GYRO_SCALE_FACTOR_LSM6DSO;
	int x_sum = 0, y_sum = 0;
	bool result = false;
	struct i2c_client *client = fimc_is_mcu_i2c_get_client(core);

	info("%s : E\n", __func__);

	/* check ois status */
	do {
		ret = fimc_is_ois_i2c_read(client, 0x01, &val);
		if (ret != 0) {
			val = -EIO;
			break;
		}
		msleep(20);
		if (--retries < 0) {
			err("Read status failed!!!!, data = 0x%04x\n", val);
			break;
		}
	} while (val != 0x01);

	retries = 30;

	ret = fimc_is_ois_i2c_write(client, 0x0014, 0x01);
	if (ret) {
		err("i2c write fail\n");
	}

	do {
		ret = fimc_is_ois_i2c_read(client, 0x0014, &val);
		if (ret != 0) {
			val = -EIO;
			break;
		}
		msleep(15);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%04x\n", val);
			break;
		}
	} while (val);

	/* Gyro result check */
	ret = fimc_is_ois_i2c_read(client, 0x0004, &val);
	if (ret != 0) {
		val = -EIO;
		err("i2c read fail\n");
		goto exit;
	}

	if ((val & 0x23) == 0x0) {
		/* Execute OIS DATA AREA WRITE start */
		info("[%s] Gyro result check success. Start Data write.", __func__);
		ret = fimc_is_ois_i2c_write(client, 0x03, 0x01);
		if (ret) {
			err("i2c write fail\n");
		}
		msleep(170);
		retries = 20;
		do {
			ret = fimc_is_ois_i2c_read(client, 0x03, &val);
			if (ret != 0) {
				val = -EIO;
				err("i2c read fail\n");
				break;
			}
			msleep(10);
			if (--retries < 0) {
				err("Read register failed!!!!, data = 0x%04x\n", val);
				break;
			}
		} while (val);

		ret = fimc_is_ois_i2c_read(client, 0x0027, &val);
		if (ret != 0) {
			val = -EIO;
			err("i2c read fail\n");
			goto exit;
		}

		if (val == 0xAA) {
			info("[%s] Written cal is OK. val = 0x%02x.", __func__, val);
			result = true;
		} else {
			info("[%s] Written cal is NG. val = 0x%02x.", __func__, val);
		}
	}

	fimc_is_ois_i2c_read(client, 0x0248, &val);
	x = val;
	fimc_is_ois_i2c_read(client, 0x0249, &val);
	x_sum = (val << 8) | x;
	if (x_sum > 0x7FFF) {
		x_sum = -((x_sum ^ 0xFFFF) + 1);
	}

	fimc_is_ois_i2c_read(client, 0x024A, &val);
	y = val;
	fimc_is_ois_i2c_read(client, 0x024B, &val);
	y_sum = (val << 8) | y;
	if (y_sum > 0x7FFF) {
		y_sum = -((y_sum ^ 0xFFFF) + 1);
	}

	*x_value = x_sum * 1000 / scale_factor;
	*y_value = y_sum * 1000 / scale_factor;

exit:
	info("%s X (x = %ld/y = %ld) : result = %d\n", __func__, *x_value, *y_value, result);

	return result;
}

bool fimc_is_ois_offset_test_mcu(struct fimc_is_core *core, long *raw_data_x, long *raw_data_y)
{
	int ret = 0, i = 0;
	u8 val = 0, x = 0, y = 0;
	int x_sum = 0, y_sum = 0, sum = 0;
	int retries = 0, avg_count = 30;
	bool result = false;
	int scale_factor = OIS_GYRO_SCALE_FACTOR_LSM6DSO;
	struct i2c_client *client = fimc_is_mcu_i2c_get_client(core);

	info("%s : E\n", __func__);

	ret = fimc_is_ois_i2c_write(client, 0x0014, 0x01);
	if (ret) {
		err("i2c write fail\n");
	}

	retries = avg_count;
	do {
		ret = fimc_is_ois_i2c_read(client, 0x0014, &val);
		if (ret != 0) {
			break;
		}
		msleep(20);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%04x\n", val);
			break;
		}
	} while (val);

	/* Gyro result check */
	ret = fimc_is_ois_i2c_read(client, 0x0004, &val);
	if (ret != 0) {
		val = -EIO;
		err("i2c read fail\n");
		goto exit;
	}

	if ((val & 0x23) == 0x0) {
		info("[%s] Gyro result check success. Result is OK.", __func__);
		result = true;
	} else {
		info("[%s] Gyro result check fail. Result is NG.", __func__);
		result = false;
	}

	sum = 0;
	retries = avg_count;
	for (i = 0; i < retries; retries--) {
		fimc_is_ois_i2c_read(client, 0x0248, &val);
		x = val;
		fimc_is_ois_i2c_read(client, 0x0249, &val);
		x_sum = (val << 8) | x;
		if (x_sum > 0x7FFF) {
			x_sum = -((x_sum ^ 0xFFFF) + 1);
		}
		sum += x_sum;
	}
	sum = sum * 10 / avg_count;
	*raw_data_x = sum * 1000 / scale_factor / 10;

	sum = 0;
	retries = avg_count;
	for (i = 0; i < retries; retries--) {
		fimc_is_ois_i2c_read(client, 0x024A, &val);
		y = val;
		fimc_is_ois_i2c_read(client, 0x024B, &val);
		y_sum = (val << 8) | y;
		if (y_sum > 0x7FFF) {
			y_sum = -((y_sum ^ 0xFFFF) + 1);
		}
		sum += y_sum;
	}
	sum = sum * 10 / avg_count;
	*raw_data_y = sum * 1000 / scale_factor / 10;

exit:
	//fimc_is_mcu_fw_version(core);
	info("%s : X raw_x = %ld, raw_y = %ld\n", __func__, *raw_data_x, *raw_data_y);

	return result;
}

void fimc_is_ois_get_offset_data_mcu(struct fimc_is_core *core, long *raw_data_x, long *raw_data_y)
{
	int i = 0;
	int ret = 0;
	u8 val = 0, x = 0, y = 0;
	int x_sum = 0, y_sum = 0, sum = 0;
	int retries = 0, avg_count = 30;
	int scale_factor = OIS_GYRO_SCALE_FACTOR_LSM6DSO;
	struct i2c_client *client = fimc_is_mcu_i2c_get_client(core);

	info("%s : E\n", __func__);

	/* check ois status */
	retries = avg_count;
	do {
		ret = fimc_is_ois_i2c_read(client, 0x01, &val);
		if (ret != 0) {
			val = -EIO;
			break;
		}
		msleep(20);
		if (--retries < 0) {
			err("Read status failed!!!!, data = 0x%04x\n", val);
			break;
		}
	} while (val != 0x01);

	retries = avg_count;
	for (i = 0; i < retries; retries--) {
		fimc_is_ois_i2c_read(client, 0x0248, &val);
		x = val;
		fimc_is_ois_i2c_read(client, 0x0249, &val);
		x_sum = (val << 8) | x;
		if (x_sum > 0x7FFF) {
			x_sum = -((x_sum ^ 0xFFFF) + 1);
		}
		sum += x_sum;
	}
	sum = sum * 10 / avg_count;
	*raw_data_x = sum * 1000 / scale_factor / 10;

	sum = 0;
	retries = avg_count;
	for (i = 0; i < retries; retries--) {
		fimc_is_ois_i2c_read(client, 0x024A, &val);
		y = val;
		fimc_is_ois_i2c_read(client, 0x024B, &val);
		y_sum = (val << 8) | y;
		if (y_sum > 0x7FFF) {
			y_sum = -((y_sum ^ 0xFFFF) + 1);
		}
		sum += y_sum;
	}
	sum = sum * 10 / avg_count;
	*raw_data_y = sum * 1000 / scale_factor / 10;

	//fimc_is_mcu_fw_version(core);
	info("%s : X raw_x = %ld, raw_y = %ld\n", __func__, *raw_data_x, *raw_data_y);
	return;
}

void fimc_is_ois_gyro_sleep_mcu(struct fimc_is_core *core)
{
	int ret = 0;
	u8 val = 0;
	int retries = 20;
	struct i2c_client *client = fimc_is_mcu_i2c_get_client(core);

	ret = fimc_is_ois_i2c_write(client, 0x0000, 0x00);
	if (ret) {
		err("i2c read fail\n");
	}

	do {
		ret = fimc_is_ois_i2c_read(client, 0x0001, &val);
		if (ret != 0) {
			break;
		}

		if (val == 0x01 || val == 0x13)
			break;

		msleep(1);
	} while (--retries > 0);

	if (retries <= 0) {
		err("Read register failed!!!!, data = 0x%04x\n", val);
	}

	ret = fimc_is_ois_i2c_write(client, 0x0030, 0x03);
	if (ret) {
		err("i2c read fail\n");
	}
	msleep(1);

	return;
}

void fimc_is_ois_exif_data_mcu(struct fimc_is_core *core)
{
	int ret = 0;
	u8 error_reg[2], status_reg;
	u16 error_sum;
	struct i2c_client *client = fimc_is_mcu_i2c_get_client(core);

	ret = fimc_is_ois_i2c_read(client, 0x0004, &error_reg[0]);
	if (ret) {
		err("i2c read fail\n");
	}

	ret = fimc_is_ois_i2c_read(client, 0x0005, &error_reg[1]);
	if (ret) {
		err("i2c read fail\n");
	}

	error_sum = (error_reg[1] << 8) | error_reg[0];

	ret = fimc_is_ois_i2c_read(client, 0x0001, &status_reg);
	if (ret) {
		err("i2c read fail\n");
	}

	ois_exif_data.error_data = error_sum;
	ois_exif_data.status_data = status_reg;

	return;
}

u8 fimc_is_ois_read_status_mcu(struct fimc_is_core *core)
{
	int ret = 0;
	u8 status = 0;
	struct i2c_client *client = fimc_is_mcu_i2c_get_client(core);

	ret = fimc_is_ois_i2c_read(client, 0x0006, &status);
	if (ret) {
		err("i2c read fail\n");
	}

	return status;
}

u8 fimc_is_ois_read_cal_checksum_mcu(struct fimc_is_core *core)
{
	int ret = 0;
	u8 status = 0;
	struct i2c_client *client = fimc_is_mcu_i2c_get_client(core);

	ret = fimc_is_ois_i2c_read(client, 0x0005, &status);
	if (ret) {
		err("i2c read fail\n");
	}

	return status;
}

int fimc_is_ois_set_coef_mcu(struct v4l2_subdev *subdev, u8 coef)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	WARN_ON(!subdev);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	ois = mcu->ois;

	if (ois->pre_coef == coef)
		return ret;

	client = mcu->client;
	if (!client) {
		err("client is NULL");
		return -EINVAL;
	}

	dbg_ois("%s %d\n", __func__, coef);

	I2C_MUTEX_LOCK(ois->i2c_lock);
	ret = fimc_is_ois_i2c_write(client, 0x005e, coef);
	if (ret) {
		err("%s i2c write fail\n", __func__);
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}
	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	ois->pre_coef = coef;

	return ret;
}

int fimc_is_mcu_read_fw_ver(char *name, char *ver)
{
	int ret = 0;
	ulong size = 0;
	char buf[100] = {0, };
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nread;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(name, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		info("failed to open fw!!!\n");
		ret = -EIO;
		goto exit;
	}

	size = 4;
	fp->f_pos = 0x80F8;

	nread = vfs_read(fp, (char __user *)(buf), size, &fp->f_pos);
	if (nread != size) {
		err("failed to read firmware file, %ld Bytes\n", nread);
		ret = -EIO;
		goto exit;
	}

exit:
	if (!IS_ERR_OR_NULL(fp))
		filp_close(fp, current->files);
	set_fs(old_fs);

	memcpy(ver, &buf[4], 3);
	memcpy(&ver[3], buf, 4);

	return ret;
}

int fimc_is_ois_shift_mcu(struct v4l2_subdev *subdev)
{
	struct fimc_is_ois *ois = NULL;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;
	u8 data[2];
	int ret = 0;

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	ois = mcu->ois;

	client = mcu->client;
	if (!client) {
		err("client is NULL");
		return -EINVAL;
	}

	I2C_MUTEX_LOCK(ois->i2c_lock);

	data[0] = (ois_center_x & 0xFF);
	data[1] = (ois_center_x & 0xFF00) >> 8;
	ret = fimc_is_ois_i2c_write_multi(client, 0x0022, data, 4);
	if (ret < 0)
		err("i2c write multi is fail\n");

	data[0] = (ois_center_y & 0xFF);
	data[1] = (ois_center_y & 0xFF00) >> 8;
	ret = fimc_is_ois_i2c_write_multi(client, 0x0024, data, 4);
	if (ret < 0)
		err("i2c write multi is fail\n");

	ret = fimc_is_ois_i2c_write(client, 0x0002, 0x02);
	if (ret < 0)
		err("i2c write multi is fail\n");

	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	return ret;
}

int fimc_is_ois_set_centering_mcu(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	WARN_ON(!subdev);

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	client = mcu->client;
	if (!client) {
		err("client is NULL");
		return -EINVAL;
	}

	ois = mcu->ois;

	ret = fimc_is_ois_i2c_write(client, 0x0002, 0x05);
	if (ret) {
		err("i2c write fail\n");
		return ret;
	}

	ois->pre_ois_mode = OPTICAL_STABILIZATION_MODE_CENTERING;

	return ret;
}

u8 fimc_is_read_ois_mode_mcu(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u8 mode = OPTICAL_STABILIZATION_MODE_OFF;
	struct fimc_is_mcu *mcu = NULL;
	struct i2c_client *client = NULL;

	mcu = (struct fimc_is_mcu *)v4l2_get_subdevdata(subdev);
	if(!mcu) {
		err("%s, mcu is NULL", __func__);
		return OPTICAL_STABILIZATION_MODE_OFF;
	}

	client = mcu->client;
	if (!client) {
		err("client is NULL");
		return OPTICAL_STABILIZATION_MODE_OFF;
	}

	ret = fimc_is_ois_i2c_read(client, 0x0002, &mode);
	if (ret) {
		err("i2c read fail\n");
		return OPTICAL_STABILIZATION_MODE_OFF;
	}

	switch(mode) {
		case 0x00:
			mode = OPTICAL_STABILIZATION_MODE_STILL;
			break;
		case 0x01:
			mode = OPTICAL_STABILIZATION_MODE_VIDEO;
			break;
		case 0x05:
			mode = OPTICAL_STABILIZATION_MODE_CENTERING;
			break;
		case 0x13:
			mode = OPTICAL_STABILIZATION_MODE_STILL_ZOOM;
			break;
		case 0x14:
			mode = OPTICAL_STABILIZATION_MODE_VDIS;
			break;
		default:
			dbg_ois("%s: ois_mode value(%d)\n", __func__, mode);
			break;
	}

	return mode;
}

static struct fimc_is_ois_ops ois_ops_mcu = {
	.ois_init = fimc_is_ois_init_mcu,
	.ois_deinit = fimc_is_ois_deinit_mcu,
	.ois_set_mode = fimc_is_set_ois_mode_mcu,
	.ois_shift_compensation = fimc_is_ois_shift_compensation_mcu,
	.ois_fw_update = fimc_is_mcu_fw_update,
	.ois_self_test = fimc_is_ois_self_test_mcu,
	.ois_auto_test = fimc_is_ois_auto_test_mcu,
#ifdef CAMERA_2ND_OIS
	.ois_auto_test_rear2 = fimc_is_ois_auto_test_rear2_mcu,
	.ois_set_power_mode = fimc_is_ois_set_power_mode_mcu,
#endif
	.ois_check_fw = fimc_is_ois_check_fw_mcu,
	.ois_enable = fimc_is_ois_enable_mcu,
	.ois_offset_test = fimc_is_ois_offset_test_mcu,
	.ois_get_offset_data = fimc_is_ois_get_offset_data_mcu,
	.ois_gyro_sleep = fimc_is_ois_gyro_sleep_mcu,
	.ois_exif_data = fimc_is_ois_exif_data_mcu,
	.ois_read_status = fimc_is_ois_read_status_mcu,
	.ois_read_cal_checksum = fimc_is_ois_read_cal_checksum_mcu,
	.ois_set_coef = fimc_is_ois_set_coef_mcu,
	.ois_read_fw_ver = fimc_is_mcu_read_fw_ver,
	.ois_center_shift = fimc_is_ois_shift_mcu,
	.ois_set_center = fimc_is_ois_set_centering_mcu,
	.ois_read_mode = fimc_is_read_ois_mode_mcu,
	.ois_calibration_test = fimc_is_ois_gyro_cal_mcu,
};

static struct fimc_is_aperture_ops aperture_ops_mcu = {
	.set_aperture_value = fimc_is_mcu_set_aperture,
	.aperture_deinit = fimc_is_mcu_deinit_aperture,
};

#ifdef CONFIG_SENSORCORE_MCU_CONTROL
struct ois_sensor_interface {
	void *core;
	void (*ois_func)(void *);
};

static struct ois_sensor_interface ois_control;
static struct ois_sensor_interface ois_reset;

extern int ois_fw_update_register(struct ois_sensor_interface *ois);
extern void ois_fw_update_unregister(void);
extern int ois_reset_register(struct ois_sensor_interface *ois);
extern void ois_reset_unregister(void);
#endif

int fimc_is_mcu_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct device *dev;
	struct device_node *dnode;
	struct fimc_is_mcu *mcu = NULL;
	struct fimc_is_device_sensor *device;
	struct v4l2_subdev *subdev_mcu = NULL;
	struct fimc_is_vender_specific *specific;
	struct v4l2_subdev *subdev_ois = NULL;
	struct fimc_is_device_ois *ois_device = NULL;
	struct fimc_is_ois *ois = NULL;
	struct fimc_is_aperture *aperture = NULL;
	struct v4l2_subdev *subdev_aperture = NULL;
	u32 sensor_id_len;
	const u32 *sensor_id_spec;
	const u32 *ois_gyro_spec;
	const u32 *aperture_delay_spec;
	u32 sensor_id[FIMC_IS_SENSOR_COUNT] = {0, };
	int i = 0;
	int gpio_mcu_reset = 0;
	int gpio_mcu_boot0 = 0;

	ois_wide_init = false;
	ois_tele_init = false;
	ois_hw_check = false;

	WARN_ON(!fimc_is_dev);
	WARN_ON(!client);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core device is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

	dev = &client->dev;
	dnode = dev->of_node;

	sensor_id_spec = of_get_property(dnode, "id", &sensor_id_len);
	if (!sensor_id_spec) {
		err("sensor_id num read is fail(%d)", ret);
		goto p_err;
	}

	sensor_id_len /= (unsigned int)sizeof(*sensor_id_spec);

	ret = of_property_read_u32_array(dnode, "id", sensor_id, sensor_id_len);
	if (ret) {
		err("sensor_id read is fail(%d)", ret);
		goto p_err;
	}

	ois_gyro_spec = of_get_property(dnode, "ois_gyro_list", &mcu_init.ois_gyro_list_len);
	if (ois_gyro_spec) {
		mcu_init.ois_gyro_list_len /= (unsigned int)sizeof(*ois_gyro_spec);
		ret = of_property_read_u32_array(dnode, "ois_gyro_list",
		        mcu_init.ois_gyro_list, mcu_init.ois_gyro_list_len);
		if (ret)
		        info("ois_gyro_list read is fail(%d)", ret);
	}

	aperture_delay_spec = of_get_property(dnode, "aperture_control_delay", &mcu_init.aperture_delay_list_len);
	if (aperture_delay_spec) {
		mcu_init.aperture_delay_list_len /= (unsigned int)sizeof(*aperture_delay_spec);
		ret = of_property_read_u32_array(dnode, "aperture_control_delay",
		        mcu_init.aperture_delay_list, mcu_init.aperture_delay_list_len);
		if (ret)
		        info("aperture_control_delay read is fail(%d)", ret);
	}

	for (i = 0; i < sensor_id_len; i++) {
		device = &core->sensor[sensor_id[i]];
		if (!device) {
			err("sensor device is NULL");
			ret = -EPROBE_DEFER;
			goto p_err;
		}
	}

	mcu = kzalloc(sizeof(struct fimc_is_mcu) * sensor_id_len, GFP_KERNEL);
	if (!mcu) {
		err("fimc_is_mcu is NULL");
		ret -ENOMEM;
		goto p_err;
	}

	subdev_mcu = kzalloc(sizeof(struct v4l2_subdev) * sensor_id_len, GFP_KERNEL);
	if (!subdev_mcu) {
		err("subdev_mcu is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	ois = kzalloc(sizeof(struct fimc_is_ois) * sensor_id_len, GFP_KERNEL);
	if (!ois) {
		err("fimc_is_ois is NULL");
		ret -ENOMEM;
		goto p_err;
	}

	subdev_ois = kzalloc(sizeof(struct v4l2_subdev) * sensor_id_len, GFP_KERNEL);
	if (!subdev_ois) {
		err("subdev_ois is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	ois_device = kzalloc(sizeof(struct fimc_is_device_ois), GFP_KERNEL);
	if (!ois_device) {
		err("fimc_is_device_ois is NULL");
		ret -ENOMEM;
		goto p_err;
	}

	aperture = kzalloc(sizeof(struct fimc_is_aperture) * sensor_id_len, GFP_KERNEL);
	if (!aperture) {
		err("aperture is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	subdev_aperture = kzalloc(sizeof(struct v4l2_subdev)  * sensor_id_len, GFP_KERNEL);
	if (!subdev_aperture) {
		err("subdev_aperture is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	gpio_mcu_boot0 = of_get_named_gpio(dnode, "gpio_mcu_boot0", 0);
	if (gpio_is_valid(gpio_mcu_boot0)) {
		gpio_request_one(gpio_mcu_boot0, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_mcu_boot0);
	} else {
		err("[MCU] Fail to get mcu boot0 gpio.");
		gpio_mcu_boot0 = 0;
	}

	gpio_mcu_reset = of_get_named_gpio(dnode, "gpio_mcu_reset", 0);
	if (gpio_is_valid(gpio_mcu_reset)) {
		gpio_request_one(gpio_mcu_reset, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_mcu_reset);
	} else {
		err("[MCU] Fail to get mcu reset gpio.");
		gpio_mcu_reset = 0;
	}

	specific = core->vender.private_data;
	ois_device->ois_ops = &ois_ops_mcu;

	for (i = 0; i < sensor_id_len; i++) {
		probe_info("%s sensor_id %d\n", __func__, sensor_id[i]);

		mcu[i].client = client;
		mcu[i].name = MCU_NAME_STM32;
		mcu[i].subdev = &subdev_mcu[i];
		mcu[i].device = sensor_id[i];
		mcu[i].gpio_mcu_boot0 = gpio_mcu_boot0;
		mcu[i].gpio_mcu_reset = gpio_mcu_reset;
		mcu[i].private_data = core;
		mcu[i].i2c_lock = NULL;

		ois[i].subdev = &subdev_ois[i];
		ois[i].device = sensor_id[i];
		ois[i].ois_mode = OPTICAL_STABILIZATION_MODE_OFF;
		ois[i].pre_ois_mode = OPTICAL_STABILIZATION_MODE_OFF;
		ois[i].ois_shift_available = false;
		ois[i].i2c_lock = NULL;
		ois[i].client = client;
		ois[i].ois_ops = &ois_ops_mcu;

		mcu[i].subdev_ois = &subdev_ois[i];
		mcu[i].ois = &ois[i];
		mcu[i].ois_device = ois_device;

		if (i == 0) {
			aperture[i].start_value = F2_4;
			aperture[i].new_value = 0;
			aperture[i].cur_value = 0;
			aperture[i].step = APERTURE_STEP_STATIONARY;
			aperture[i].subdev = &subdev_aperture[i];
			aperture[i].aperture_ops = &aperture_ops_mcu;

			mutex_init(&aperture[i].control_lock);

			mcu[i].aperture = &aperture[i];
			mcu[i].subdev_aperture = aperture[i].subdev;
			mcu[i].aperture_ops = &aperture_ops_mcu;
		}

		device = &core->sensor[sensor_id[i]];
		device->subdev_mcu = &subdev_mcu[i];
		device->mcu = &mcu[i];

		v4l2_i2c_subdev_init(&subdev_mcu[i], client, &subdev_ops);
		v4l2_set_subdevdata(&subdev_mcu[i], &mcu[i]);
		v4l2_set_subdev_hostdata(&subdev_mcu[i], device);

		/* reset i2c client's data to mcu device */
		i2c_set_clientdata(client, &mcu[i]);

		probe_info("%s done\n", __func__);
	}

#ifdef CONFIG_SENSORCORE_MCU_CONTROL
	ois_control.core = core;
	ois_control.ois_func = &fimc_is_ois_fw_update_from_sensor;
	ret = ois_fw_update_register(&ois_control);
	if (ret)
		err("ois_fw_update_register failed: %d\n", ret);

	ois_reset.core = core;
	ois_reset.ois_func = &fimc_is_ois_reset_mcu;
	ret = ois_reset_register(&ois_reset);
	if (ret)
		err("ois_reset_register failed: %d\n", ret);
#endif

	return ret;

p_err:
	if (mcu)
		kfree(mcu);

	if (subdev_mcu)
		kfree(subdev_mcu);

	if (ois)
		kfree(ois);

	if (subdev_ois)
		kfree(subdev_ois);

	if (ois_device)
		kfree(ois_device);

	if (aperture)
		kfree(aperture);

	if (subdev_aperture)
		kfree(subdev_aperture);

	return ret;
}

static int mcu_remove(struct i2c_client *client)
{
	int ret = 0;

#ifdef CONFIG_SENSORCORE_MCU_CONTROL
	ois_fw_update_unregister();
	ois_reset_unregister();
#endif

	return ret;
}

static const struct of_device_id exynos_fimc_is_mcu_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-ois-mcu",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_mcu_match);

static const struct i2c_device_id mcu_idt[] = {
	{ MCU_NAME, 0 },
	{},
};

static struct i2c_driver mcu_driver = {
	.driver = {
		.name	= MCU_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = exynos_fimc_is_mcu_match
	},
	.probe	= fimc_is_mcu_probe,
	.remove	= mcu_remove,
	.id_table = mcu_idt
};
module_i2c_driver(mcu_driver);

MODULE_DESCRIPTION("MCU driver for STM32");
MODULE_AUTHOR("kyoungho yun <kyoungho.yun@samsung.com>");
MODULE_LICENSE("GPL v2");
