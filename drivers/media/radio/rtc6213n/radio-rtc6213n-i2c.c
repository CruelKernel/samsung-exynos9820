/*
 * drivers/media/radio/rtc6213n/radio-rtc6213n-i2c.c
 *
 * I2C driver for Richwave RTC6213N FM Tuner
 *
 *  Copyright (c) 2009 Tobias Lorenz <tobias.lorenz@gmx.net>
 *  Copyright (c) 2012 Hans de Goede <hdegoede@redhat.com>
 *  Copyright (c) 2013 Richwave Technology Co.Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* kernel includes */
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/of_gpio.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include "radio-rtc6213n.h"

static struct of_device_id rtc6213n_i2c_dt_ids[] = {
	{.compatible = "rtc6213n"},
	{}
};

/* I2C Device ID List */
static const struct i2c_device_id rtc6213n_i2c_id[] = {
    /* Generic Entry */
	{ "rtc6213n", 0 },
	/* Terminating entry */
	{ }
};
MODULE_DEVICE_TABLE(i2c, rtc6213n_i2c_id);

static unsigned short space = 1;
static unsigned short band;
static unsigned short de;

/**************************************************************************
 * Module Parameters
 **************************************************************************/

/* Radio Nr */
static int radio_nr = -1;
module_param(radio_nr, int, 0444);
MODULE_PARM_DESC(radio_nr, "Radio Nr");

/* RDS buffer blocks */
static unsigned int rds_buf = 100;
module_param(rds_buf, uint, 0444);
MODULE_PARM_DESC(rds_buf, "RDS buffer entries: *100*");

/* RDS maximum block errors */
static unsigned short max_rds_errors = 1;
/* 0 means   0  errors requiring correction */
/* 1 means 1-2  errors requiring correction */
/* 2 means 3-5  errors requiring correction */
/* 3 means   6+ errors or errors in checkword, correction not possible */
module_param(max_rds_errors, ushort, 0644);
MODULE_PARM_DESC(max_rds_errors, "RDS maximum block errors: *1*");



/**************************************************************************
 * I2C Definitions
 **************************************************************************/
/* Write starts with the upper byte of register 0x02 */
#define WRITE_REG_NUM       RADIO_REGISTER_NUM
#define WRITE_INDEX(i)      ((i + 0x02)%16)

/* Read starts with the upper byte of register 0x0a */
#define READ_REG_NUM        RADIO_REGISTER_NUM
#define READ_INDEX(i)       ((i + RADIO_REGISTER_NUM - 0x0a) % READ_REG_NUM)

/*static*/
struct tasklet_struct my_tasklet;
/**************************************************************************
 * General Driver Functions - REGISTERs
 **************************************************************************/

int set_fm_lna(struct rtc6213n_device *radio, int state)
{
	if (gpio_is_valid(radio->fm_lna_gpio)) {
		gpio_direction_output(radio->fm_lna_gpio, state);
		dev_info(&radio->videodev->dev, "%s: state=%d\n", __func__, state);
	}

	return 0;
}

/*
 * rtc6213n_get_register - read register
 */
int rtc6213n_get_register(struct rtc6213n_device *radio, int regnr)
{
	u16 buf[READ_REG_NUM];
	struct i2c_msg msgs[1] = {
		{ radio->client->addr, I2C_M_RD, sizeof(u16) * READ_REG_NUM,
			(void *)buf },
	};

	if (i2c_transfer(radio->client->adapter, msgs, 1) != 1)
		return -EIO;

	radio->registers[regnr] = __be16_to_cpu(buf[READ_INDEX(regnr)]);

	return 0;
}

/*
 * rtc6213n_set_register - write register
 */
int rtc6213n_set_register(struct rtc6213n_device *radio, int regnr)
{
	int i;
	u16 buf[WRITE_REG_NUM];
	struct i2c_msg msgs[1] = {
		{ radio->client->addr, 0, sizeof(u16) * WRITE_REG_NUM,
			(void *)buf },
	};

	for (i = 0; i < WRITE_REG_NUM; i++)
		buf[i] = __cpu_to_be16(radio->registers[WRITE_INDEX(i)]);

	if (i2c_transfer(radio->client->adapter, msgs, 1) != 1) {
		for (i = 0; i < WRITE_REG_NUM; i++) {
			dev_err(&radio->videodev->dev, " %s buf[%d] = %d\n",
						__func__, i, buf[i]);
		}
		return -EIO;
	}

	return 0;
}

/*
 * rtc6213n_set_register - write register
 */
int rtc6213n_set_serial_registers(struct rtc6213n_device *radio,
	u16 *data, int bytes)
{
	int i;
	u16 buf[46];
	struct i2c_msg msgs[1] = {
		{ radio->client->addr, 0, sizeof(u16) * bytes,
			(void *)buf },
	};

	for (i = 0; i < bytes; i++)
		buf[i] = __cpu_to_be16(data[i]);

	if (i2c_transfer(radio->client->adapter, msgs, 1) != 1) {
		for (i = 0; i < 46; i++) {
			dev_err(&radio->videodev->dev, " rtc6213n_set_serial_registers buf[%d] = %d\n",
						i, buf[i]);
		}
		return -EIO;
	}

	return 0;
}

/**************************************************************************
 * General Driver Functions - ENTIRE REGISTERS
 **************************************************************************/
/*
 * rtc6213n_get_all_registers - read entire registers
 */
/* changed from static */
int rtc6213n_get_all_registers(struct rtc6213n_device *radio)
{
	int i;
	u16 buf[READ_REG_NUM];
	struct i2c_msg msgs[1] = {
		{ radio->client->addr, I2C_M_RD, sizeof(u16) * READ_REG_NUM,
			(void *)buf },
	};

	if (i2c_transfer(radio->client->adapter, msgs, 1) != 1)
		return -EIO;

	for (i = 0; i < READ_REG_NUM; i++)
		radio->registers[i] = __be16_to_cpu(buf[READ_INDEX(i)]);

	return 0;
}

int rtc6213n_disconnect_check(struct rtc6213n_device *radio)
{
	return 0;
}

/**************************************************************************
 * File Operations Interface
 **************************************************************************/

/*
 * rtc6213n_fops_open - file open
 */
int rtc6213n_fops_open(struct file *file)
{
	struct rtc6213n_device *radio = video_drvdata(file);
	int retval = 0;

	mutex_lock(&radio->lock);
	radio->users++;

	dev_info(&radio->videodev->dev, "%s: user num = %d\n",
			__func__, radio->users);

	if (radio->users == 1) {
		if (radio->use_ext_lna)
			set_fm_lna(radio, 1);
		/* start radio */
		retval = rtc6213n_start(radio);
		if (retval < 0)
			goto done;
		dev_info(&radio->videodev->dev, "rtc6213n_fops_open : after initialization\n");

		/* mpxconfig */
		/* Disable Softmute / Disable Mute / De-emphasis / Volume 8 */
		radio->registers[MPXCFG] = 0x0000 |
			MPXCFG_CSR0_DIS_SMUTE | MPXCFG_CSR0_DIS_MUTE |
			((radio->blend_level << 8) & MPXCFG_CSR0_BLNDADJUST) |
			((de << 12) & MPXCFG_CSR0_DEEM) | 0x0008;
		retval = rtc6213n_set_register(radio, MPXCFG);
		if (retval < 0)
			goto done;

		/* channel */
		/* Band / Space / Default channel 90.1Mhz */
		radio->registers[CHANNEL] =
			((band  << 12) & CHANNEL_CSR0_BAND)  |
			((space << 10) & CHANNEL_CSR0_CHSPACE) | 0x1a;
		retval = rtc6213n_set_register(radio, CHANNEL);
		if (retval < 0)
			goto done;

		/* ==== seeking parameter setting ==== */
		/* seekconfig1 */
		radio->registers[SEEKCFG1] &= ~SEEKCFG1_CSR0_SEEKRSSITH;
		radio->registers[SEEKCFG1] |= 0x0c;
		retval = rtc6213n_set_register(radio, SEEKCFG1);
		if (retval < 0)
			goto done;
		
		/* seekconfig2 */
		/* Seeking TH */
		/* 0x4010 is hexdecimal and combination of DC and spike */
		/* SEEKCFG2[15:8] is DC TH, SEEKCFG2[7:0] is spike TH   */
		/* DC TH default is 0x40, Spike TH default ix 0x50		*/
		/* Suggest to set more destrict for filter out garbage  */
		/* Suggest to adjust DC to 0x38, 0x30, 0x28				*/
		/* Suggest to keep Spike as 0x10						*/
		radio->registers[SEEKCFG2] = 0x4010;
		retval = rtc6213n_set_register(radio, SEEKCFG2);
		if (retval < 0)
			goto done;
		/* ==== seeking parameter setting ==== */

		/* enable RDS / STC interrupt */
		radio->registers[SYSCFG] |= SYSCFG_CSR0_RDSIRQEN;
		radio->registers[SYSCFG] |= SYSCFG_CSR0_STDIRQEN;
		/*radio->registers[SYSCFG] |= SYSCFG_CSR0_RDS_EN;*/
		retval = rtc6213n_set_register(radio, SYSCFG);
		if (retval < 0)
			goto done;

		radio->registers[PADCFG] &= ~PADCFG_CSR0_GPIO;
		radio->registers[PADCFG] |= 0x1 << 2;
		retval = rtc6213n_set_register(radio, PADCFG);
		if (retval < 0)
			goto done;

		/* powerconfig */
		/* Enable FM */
		radio->registers[POWERCFG] = POWERCFG_CSR0_ENABLE;
		retval = rtc6213n_set_register(radio, POWERCFG);
		if (retval < 0)
			goto done;

		dev_info(&radio->videodev->dev, "RTC6213n Tuner1: DeviceID=0x%4.4hx ChipID=0x%4.4hx\n",
			radio->registers[DEVICEID], radio->registers[CHIPID]);
		dev_info(&radio->videodev->dev, "RTC6213n Tuner2: Reg2=0x%4.4hx Reg3=0x%4.4hx\n",
			radio->registers[MPXCFG], radio->registers[CHANNEL]);
		dev_info(&radio->videodev->dev, "RTC6213n Tuner3: Reg4=0x%4.4hx Reg5=0x%4.4hx\n",
			radio->registers[SYSCFG], radio->registers[SEEKCFG1]);
		dev_info(&radio->videodev->dev, "RTC6213n Tuner4: Reg6=0x%4.4hx Reg7=0x%4.4hx\n",
			radio->registers[POWERCFG], radio->registers[PADCFG]);
		dev_info(&radio->videodev->dev, "RTC6213n Tuner5: Reg8=0x%4.4hx Reg9=0x%4.4hx\n",
			radio->registers[8], radio->registers[9]);
		dev_info(&radio->videodev->dev, "RTC6213n Tuner6: regA=0x%4.4hx RegB=0x%4.4hx\n",
			radio->registers[10], radio->registers[11]);
		dev_info(&radio->videodev->dev, "RTC6213n Tuner7: regC=0x%4.4hx RegD=0x%4.4hx\n",
			radio->registers[12], radio->registers[13]);
		dev_info(&radio->videodev->dev, "RTC6213n Tuner8: regE=0x%4.4hx RegF=0x%4.4hx\n",
			radio->registers[14], radio->registers[15]);
	}
	dev_info(&radio->videodev->dev, "rtc6213n_fops_open : Exit\n");

done:
	mutex_unlock(&radio->lock);
	return retval;
}


/*
 * rtc6213n_fops_release - file release
 */
int rtc6213n_fops_release(struct file *file)
{
	struct rtc6213n_device *radio = video_drvdata(file);
	int retval = 0;

	/* safety check */
	if (!radio)
		return -ENODEV;

	mutex_lock(&radio->lock);
	radio->users--;
	if (radio->users == 0) {
		/* stop radio */
		retval = rtc6213n_stop(radio);
		tasklet_kill(&my_tasklet);
		if (radio->use_ext_lna)
			set_fm_lna(radio, 0);
	}
	mutex_unlock(&radio->lock);
	dev_info(&radio->videodev->dev, "rtc6213n_fops_release Exit retval = %d\n",
		retval);

	return retval;
}



/**************************************************************************
 * Video4Linux Interface
 **************************************************************************/

/*
 * rtc6213n_vidioc_querycap - query device capabilities
 */
int rtc6213n_vidioc_querycap(struct file *file, void *priv,
	struct v4l2_capability *capability)
{
	strlcpy(capability->driver, DRIVER_NAME, sizeof(capability->driver));
	strlcpy(capability->card, DRIVER_CARD, sizeof(capability->card));
	capability->version = DRIVER_KERNEL_VERSION;
	capability->device_caps = V4L2_CAP_HW_FREQ_SEEK | V4L2_CAP_READWRITE |
		V4L2_CAP_TUNER | V4L2_CAP_RADIO | V4L2_CAP_RDS_CAPTURE;
	capability->capabilities = capability->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}



/**************************************************************************
 * I2C Interface
 **************************************************************************/

/*
 * rtc6213n_i2c_interrupt - interrupt handler
 */
static irqreturn_t rtc6213n_i2c_interrupt(int irq, void *dev_id)
{
	struct rtc6213n_device *radio = dev_id;
	unsigned char regnr;
	unsigned char blocknum;
	unsigned short bler; /* rds block errors */
	unsigned short rds;
	unsigned char tmpbuf[3];
	int retval = 0;

	/* check Seek/Tune Complete */
	retval = rtc6213n_get_register(radio, STATUS);
	if (retval < 0)
		goto end;

#ifdef CONFIG_RDS
	retval = rtc6213n_get_register(radio, RSSI);
	if (retval < 0)
		goto end;
#endif

	if ((rtc6213n_wq_flag == SEEK_WAITING) ||
		(rtc6213n_wq_flag == TUNE_WAITING)) {
		if (radio->registers[STATUS] & STATUS_STD) {
			rtc6213n_wq_flag = WAIT_OVER;
			wake_up_interruptible(&rtc6213n_wq);
			/* ori: complete(&radio->completion); */
			dev_info(&radio->videodev->dev, "rtc6213n_i2c_interrupt Seek/Tune Done\n");
			dev_info(&radio->videodev->dev, "STATUS=0x%4.4hx, STD = %d, SF = %d, RSSI = %d\n",
				radio->registers[STATUS],
				(radio->registers[STATUS] & STATUS_STD) >> 14,
				(radio->registers[STATUS] & STATUS_SF) >> 13,
				(radio->registers[RSSI] & RSSI_RSSI));
		}
		goto end;
	}

	/* Update RDS registers */
	for (regnr = 1; regnr < RDS_REGISTER_NUM; regnr++) {
		retval = rtc6213n_get_register(radio, STATUS + regnr);
		if (retval < 0)
			goto end;
	}

	/* get rds blocks */
	if ((radio->registers[STATUS] & STATUS_RDS_RDY) == 0)
		/* No RDS group ready, better luck next time */
		goto end;
	dev_info(&radio->videodev->dev, "interrupt : STATUS=0x%4.4hx, RSSI=%d\n",
		radio->registers[STATUS], radio->registers[RSSI] & RSSI_RSSI);
	dev_info(&radio->videodev->dev, "BAErr %d, BBErr %d, BCErr %d, BDErr %d\n",
		(radio->registers[RSSI] & RSSI_RDS_BA_ERRS) >> 14,
		(radio->registers[RSSI] & RSSI_RDS_BB_ERRS) >> 12,
		(radio->registers[RSSI] & RSSI_RDS_BC_ERRS) >> 10,
		(radio->registers[RSSI] & RSSI_RDS_BD_ERRS) >> 8);
	dev_info(&radio->videodev->dev, "RDS_RDY=%d, RDS_SYNC=%d\n",
		(radio->registers[STATUS] & STATUS_RDS_RDY) >> 15,
		(radio->registers[STATUS] & STATUS_RDS_SYNC) >> 11);
#ifdef CONFIG_RDS
	for (blocknum = 0; blocknum < 5; blocknum++) {
#else
	for (blocknum = 0; blocknum < 4; blocknum++) {
#endif
		switch (blocknum) {
		case 1:
			bler = (radio->registers[RSSI] &
					RSSI_RDS_BB_ERRS) >> 12;
			rds = radio->registers[BB_DATA];
			break;
		case 2:
			bler = (radio->registers[RSSI] &
					RSSI_RDS_BC_ERRS) >> 10;
			rds = radio->registers[BC_DATA];
			break;
		case 3:
			bler = (radio->registers[RSSI] &
					RSSI_RDS_BD_ERRS) >> 8;
			rds = radio->registers[BD_DATA];
			break;
#ifdef CONFIG_RDS			
		case 4:		/* block index 4 for RSSI */
			bler = 0;
			rds = radio->registers[RSSI] & RSSI;
			break;
#endif
		default:	/* case 0 */
			bler = (radio->registers[RSSI] &
					RSSI_RDS_BA_ERRS) >> 14;
			rds = radio->registers[BA_DATA];
			break;
		};

		/* Fill the V4L2 RDS buffer */
		put_unaligned_le16(rds, &tmpbuf);
		tmpbuf[2] = blocknum;       /* offset name */

		tmpbuf[2] |= blocknum << 3; /* received offset */
		tmpbuf[2] |= bler << 6;

		/* copy RDS block to internal buffer */
		memcpy(&radio->buffer[radio->wr_index], &tmpbuf, 3);
		radio->wr_index += 3;

		/* wrap write pointer */
		if (radio->wr_index >= radio->buf_size)
			radio->wr_index = 0;

		/* check for overflow */
		if (radio->wr_index == radio->rd_index) {
			/* increment and wrap read pointer */
			radio->rd_index += 3;
			if (radio->rd_index >= radio->buf_size)
				radio->rd_index = 0;
		}
	}

	if (radio->wr_index != radio->rd_index)
		wake_up_interruptible(&radio->read_queue);

end:

	return IRQ_HANDLED;
}

/*
 * rtc6213n_i2c_probe - probe for the device
 */
static int rtc6213n_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct rtc6213n_device *radio;
	int retval = 0;
	u8 i2c_error;
	int fmint_gpio = 0;
	int irq;
	u32 data[VOLUME_NUM];
	int i;
	struct v4l2_device *v4l2_dev;

	/* private data allocation and initialization */
	radio = kzalloc(sizeof(struct rtc6213n_device), GFP_KERNEL);
	if (!radio) {
		retval = -ENOMEM;
		goto err_initial;
	}

	radio->users = 0;
	radio->client = client;
	mutex_init(&radio->lock);

	/* video device allocation and initialization */
	radio->videodev = video_device_alloc();
	if (!radio->videodev) {
		retval = -ENOMEM;
		goto err_radio;
	}
	memcpy(radio->videodev, &rtc6213n_viddev_template,
		sizeof(rtc6213n_viddev_template));
	video_set_drvdata(radio->videodev, radio);

	v4l2_dev = kzalloc(sizeof(struct v4l2_device), GFP_KERNEL);
	if (WARN_ON(!v4l2_dev)) {
		retval = -ENOMEM;
		goto err_video;
	}

	v4l2_dev->notify = NULL;
	radio->videodev->v4l2_dev = v4l2_dev;

	retval = v4l2_device_register(&client->dev, radio->videodev->v4l2_dev);
	if (retval < 0)
		goto err_video;

	dev_info(&client->dev, "%s before retrying\n", __func__);

	radio->registers[BANKCFG] = 0x0000;

	i2c_error = 0;
	/* Keep in case of any unpredicted control */
	/* Set 0x16AA */
	radio->registers[DEVICEID] = 0x16AA;
	/* released the I2C from unexpected I2C start condition */
	retval = rtc6213n_set_register(radio, DEVICEID);
	/* recheck TH : 10 */
	while ((retval < 0) && (i2c_error < 10)) {
		retval = rtc6213n_set_register(radio, DEVICEID);
		i2c_error++;
	}

	dev_info(&client->dev, "%s retrying %d times\n", __func__, i2c_error);

	/* get device and chip versions */
	if (rtc6213n_get_all_registers(radio) < 0) {
		dev_info(&client->dev, "%s get Device ID failed!\n", __func__);
		retval = -EIO;
		goto err_video;
	}

	dev_info(&client->dev, "rtc6213n_i2c_probe DeviceID=0x%4.4hx ChipID=0x%4.4hx\n",
		radio->registers[DEVICEID], radio->registers[CHIPID]);

	/* rds buffer allocation */
	radio->buf_size = rds_buf * 3;
	radio->buffer = kmalloc(radio->buf_size, GFP_KERNEL);
	if (!radio->buffer) {
		retval = -EIO;
		goto err_video;
	}

	/* rds buffer configuration */
	radio->wr_index = 0;
	radio->rd_index = 0;
	init_waitqueue_head(&radio->read_queue);
	init_waitqueue_head(&rtc6213n_wq);

	/* fmint-gpio */
	fmint_gpio = of_get_named_gpio(client->dev.of_node, "fmint-gpio", 0);
	if (!gpio_is_valid(fmint_gpio)) {
		dev_err(&client->dev, "%s: fmint-gpio invalid %d\n",
		__func__, fmint_gpio);
	}
	retval = gpio_request(fmint_gpio, "FM_INT");
	if (retval < 0) {
		dev_err(&client->dev, "%s: error requesting sv gpio\n",
			__func__);
	}
	gpio_direction_input(fmint_gpio);

	/* interrupt gpio */
	irq = gpio_to_irq(fmint_gpio);
	if (retval < 0) {
		dev_err(&client->dev, "%s: cannot map gpio to irq\n",
			__func__);
	}

	if (of_property_read_bool(client->dev.of_node, "volume_db")) {
		dev_info(&client->dev, "%s: use fm radio volume db\n", __func__);
		radio->vol_db = true;
	} else
		radio->vol_db = false;

	if (!of_property_read_u32_array(client->dev.of_node, "radio_vol", data, VOLUME_NUM)) {
		for (i = 0; i < VOLUME_NUM; i++) {
			radio->rx_vol[i] = (~data[i]) + 1;
			dev_info(&client->dev, "%s: rx_vol = %d\n", __func__,
				radio->rx_vol[i]);
		}
	} else
		dev_info(&client->dev, "%s: can not find the volume in the dt\n", __func__);

	radio->use_ext_lna = of_property_read_bool(client->dev.of_node, "fm-lna-gpio");
	if (radio->use_ext_lna) {
		radio->fm_lna_gpio = of_get_named_gpio(client->dev.of_node, "fm-lna-gpio", 0);
		if (!gpio_is_valid(radio->fm_lna_gpio))
			dev_info(&client->dev, "%s: fm lna gpio is invalid(%d)\n", __func__, radio->fm_lna_gpio);
		else {
			retval = gpio_request(radio->fm_lna_gpio, "FM_LNA_GPIO");
			if (retval)
				dev_err(&client->dev, "%s: fm lna gpio request failed(%d)\n", __func__, radio->fm_lna_gpio);
			else
				dev_info(&client->dev, "%s: fm lna gpio(%d)\n", __func__, radio->fm_lna_gpio);
		}
	} else 
		dev_info(&client->dev, "%s: use_ext_lna=%d\n", __func__, radio->use_ext_lna);

	if (!of_property_read_u8(client->dev.of_node, "blend_lvl", &radio->blend_level)) {
		dev_info(&client->dev, "%s: blend_level = %d\n", __func__,
				radio->blend_level);
	} else {
		radio->blend_level = 0;
		dev_info(&client->dev, "%s: can not find the blend level in the dt\n",
		__func__);
	}

	/* mark Seek/Tune Complete Interrupt enabled */
	radio->stci_enabled = true;
	init_completion(&radio->completion);

	dev_info(&client->dev, "rtc6213n_i2c_probe DeviceID=0x%4.4hx ChipID=0x%4.4hx\n",
		radio->registers[DEVICEID], radio->registers[CHIPID]);

	retval = devm_request_threaded_irq(&client->dev, irq, NULL,
		rtc6213n_i2c_interrupt,	IRQF_TRIGGER_FALLING|IRQF_ONESHOT,
		DRIVER_NAME, radio);
	if (retval) {
		dev_err(&client->dev, "Failed to register interrupt\n");
		goto err_rds;
	}

	/* register video device */
	retval = video_register_device(radio->videodev, VFL_TYPE_RADIO,
		radio_nr);
	if (retval) {
		dev_warn(&client->dev, "Could not register video device\n");
		goto err_all;
	}
	i2c_set_clientdata(client, radio);

	return 0;
err_all:
	free_irq(client->irq, radio);
err_rds:
	kfree(radio->buffer);
err_video:
	video_unregister_device(radio->videodev);
err_radio:
	kfree(radio);
err_initial:
	return retval;
}


/*
 * rtc6213n_i2c_remove - remove the device
 */
static int rtc6213n_i2c_remove(struct i2c_client *client)
{
	struct rtc6213n_device *radio = i2c_get_clientdata(client);

	free_irq(client->irq, radio);
	kfree(radio->buffer);
	video_unregister_device(radio->videodev);
	kfree(radio);

	return 0;
}


#ifdef CONFIG_PM
/*
 * rtc6213n_i2c_suspend - suspend the device
 */
static int rtc6213n_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rtc6213n_device *radio = i2c_get_clientdata(client);

	dev_info(&radio->videodev->dev, "rtc6213n_i2c_suspend\n");

	return 0;
}


/*
 * rtc6213n_i2c_resume - resume the device
 */
static int rtc6213n_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rtc6213n_device *radio = i2c_get_clientdata(client);

	dev_info(&radio->videodev->dev, "rtc6213n_i2c_resume\n");

	return 0;
}

static SIMPLE_DEV_PM_OPS(rtc6213n_i2c_pm, rtc6213n_i2c_suspend,
						rtc6213n_i2c_resume);
#endif


/*
 * rtc6213n_i2c_driver - i2c driver interface
 */
struct i2c_driver rtc6213n_i2c_driver = {
	.driver = {
		.name			= "rtc6213n",
		.owner			= THIS_MODULE,
		.of_match_table = of_match_ptr(rtc6213n_i2c_dt_ids),
#ifdef CONFIG_PM
		.pm				= &rtc6213n_i2c_pm,
#endif
	},
	.probe				= rtc6213n_i2c_probe,
	.remove				= rtc6213n_i2c_remove,
	.id_table			= rtc6213n_i2c_id,
};

/*
 * rtc6213n_i2c_init
 */
int rtc6213n_i2c_init(void)
{
	pr_info(KERN_INFO DRIVER_DESC ", Version " DRIVER_VERSION "\n");
	return i2c_add_driver(&rtc6213n_i2c_driver);
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
