/*
 *  drivers/media/radio/rtc6213n/radio-rtc6213n-common.c
 *
 *  Driver for Richwave RTC6213N FM Tuner
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

/*
 * History:
 * 2013-05-12   TianTsai Chang <changtt@richwave.com.tw>
 *      Version 1.0.0
 *      - First working version
 */

/* kernel includes */
#include <linux/delay.h>
#include <linux/i2c.h>
#include "radio-rtc6213n.h"
#define New_VolumeControl
/* #define check_validch */
/**************************************************************************
 * Module Parameters
 **************************************************************************/

/* Spacing (kHz) */
/* 0: 200 kHz (USA, Australia) */
/* 1: 100 kHz (Europe, Japan) */
/* 2:  50 kHz */
static unsigned short space = 1;
module_param(space, ushort, 0444);
MODULE_PARM_DESC(space, "Spacing: 0=200kHz *1=100kHz* 2=50kHz");

/* Bottom of Band (MHz) */
/* 0: 87.5 - 108 MHz (USA, Europe)*/
/* 1: 76   - 108 MHz (Japan wide band) */
/* 2: 76   -  90 MHz (Japan) */
static unsigned short band;
module_param(band, ushort, 0444);
MODULE_PARM_DESC(band, "Band: *0=87.5-108MHz* 1=76-108MHz 2=76-91MHz 3=65-76MHz");

/* De-emphasis */
/* 0: 75 us (USA) */
/* 1: 50 us (Europe, Australia, Japan) */
static unsigned short de;
module_param(de, ushort, 0444);
MODULE_PARM_DESC(de, "De-emphasis: *0=75us* 1=50us");

/* Tune timeout */
static unsigned int tune_timeout = 3000;
module_param(tune_timeout, uint, 0644);
MODULE_PARM_DESC(tune_timeout, "Tune timeout: *3000*");

/* Seek timeout */
static unsigned int seek_timeout = 8000;
module_param(seek_timeout, uint, 0644);
MODULE_PARM_DESC(seek_timeout, "Seek timeout: *8000*");

static void wait(void);
wait_queue_head_t rtc6213n_wq;
int rtc6213n_wq_flag = NO_WAIT;
#ifdef New_VolumeControl
unsigned short global_volume;
static int g_previous_vol;
static int g_current_vol;
#endif
/**************************************************************************
 * Generic Functions
 **************************************************************************/

/*
 * rtc6213n_set_chan - set the channel
 */
static int rtc6213n_set_chan(struct rtc6213n_device *radio, unsigned short chan)
{
	int retval;
	unsigned long timeout;
	bool timed_out = 0;
	int i;
	unsigned short current_chan =
		radio->registers[CHANNEL] & CHANNEL_CSR0_CH;

	dev_info(&radio->videodev->dev, "======== %s ========\n", __func__);
	dev_info(&radio->videodev->dev, "RTC6213n tuning process is starting\n");
	dev_info(&radio->videodev->dev, "CHAN=0x%4.4hx SKCFG1=0x%4.4hx STATUS=0x%4.4hx chan=0x%4.4hx\n",
			radio->registers[CHANNEL], radio->registers[SEEKCFG1],
			radio->registers[STATUS], chan);

	/* start tuning */
	radio->registers[CHANNEL] &= ~CHANNEL_CSR0_CH;
	radio->registers[CHANNEL] |= CHANNEL_CSR0_TUNE | chan;
	retval = rtc6213n_set_register(radio, CHANNEL);
	if (retval < 0)	{
		radio->registers[CHANNEL] = current_chan;
		goto done;
	}

	/* currently I2C driver only uses interrupt way to tune */
	if (radio->stci_enabled) {
		rtc6213n_wq_flag = TUNE_WAITING;
		dev_info(&radio->videodev->dev, "========= wait tune time_out ==========\n\n");
		wait();
		rtc6213n_wq_flag = NO_WAIT;

	} else {
		/* wait till tune operation has completed */
		timeout = jiffies + msecs_to_jiffies(tune_timeout);
		do {
			retval = rtc6213n_get_all_registers(radio);
			if (retval < 0)
				goto stop;
			timed_out = time_after(jiffies, timeout);
		} while (((radio->registers[STATUS] & STATUS_STD) == 0)
			&& (!timed_out));
	}
	dev_info(&radio->videodev->dev, "RTC6213n tuning process is done\n");
	dev_info(&radio->videodev->dev,
		"CHAN=0x%4.4hx SKCFG1=0x%4.4hx STATUS=0x%4.4hx, STD = %d, SF = %d, RSSI = %d\n",
		radio->registers[CHANNEL], radio->registers[SEEKCFG1],
		radio->registers[STATUS],
		(radio->registers[STATUS] & STATUS_STD) >> 14,
		(radio->registers[STATUS] & STATUS_SF) >> 13,
		(radio->registers[RSSI] & RSSI_RSSI));

	if ((radio->registers[STATUS] & STATUS_STD) == 0) {
		dev_info(&radio->videodev->dev, "tune does not complete\n");
		retval = rtc6213n_get_all_registers(radio);
		for (i = 0; i < RADIO_REGISTER_NUM; i++)
			dev_info(&radio->videodev->dev, "radio->registers[%d] 0x%4.4hx", i, radio->registers[i]);
	}

	if (timed_out) {
		dev_info(&radio->videodev->dev, "tune timed out after %u ms\n",
			tune_timeout);
		retval = rtc6213n_get_all_registers(radio);
		for (i = 0; i < RADIO_REGISTER_NUM; i++)
			dev_info(&radio->videodev->dev, "radio->registers[%d] 0x%4.4hx", i, radio->registers[i]);
	}

stop:
	/* stop tuning */
	current_chan = radio->registers[CHANNEL] & CHANNEL_CSR0_CH;

	radio->registers[CHANNEL] &= ~CHANNEL_CSR0_TUNE;
	retval = rtc6213n_set_register(radio, CHANNEL);
	if (retval < 0)	{
		radio->registers[CHANNEL] = current_chan;
		goto done;
	}

	retval = rtc6213n_get_register(radio, STATUS);

done:
	dev_info(&radio->videodev->dev, "%s is done\n", __func__);
	dev_info(&radio->videodev->dev, "CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx STATUS=0x%4.4hx\n",
		radio->registers[CHANNEL], radio->registers[SEEKCFG1],
		radio->registers[STATUS]);
	dev_info(&radio->videodev->dev, "========= %s End ==========\n", __func__);

	return retval;
}


/*
 * rtc6213n_get_freq - get the frequency
 */
static int rtc6213n_get_freq(struct rtc6213n_device *radio, unsigned int *freq)
{
	unsigned int spacing, band_bottom;
	unsigned short chan;
	int retval;

	/* Spacing (kHz) */
	switch ((radio->registers[CHANNEL] & CHANNEL_CSR0_CHSPACE) >> 10) {
	/* 0: 200 kHz (USA, Australia) */
	case 0:
		spacing = 0.200 * FREQ_MUL; break;
	/* 1: 100 kHz (Europe, Japan) */
	case 1:
		spacing = 0.100 * FREQ_MUL; break;
	/* 2:  50 kHz */
	default:
		spacing = 0.050 * FREQ_MUL; break;
	};

	/* Bottom of Band (MHz) */
	switch ((radio->registers[CHANNEL] & CHANNEL_CSR0_BAND) >> 12) {
	/* 0: 87.5 - 108 MHz (USA, Europe) */
	case 0:
		band_bottom = 87.5 * FREQ_MUL; break;
	/* 1: 76   - 108 MHz (Japan wide band) */
	default:
		band_bottom = 76   * FREQ_MUL; break;
	/* 2: 76   -  90 MHz (Japan) */
	case 2:
		band_bottom = 76   * FREQ_MUL; break;
	};

stop:
	/* read channel */
	retval = rtc6213n_get_register(radio, STATUS);
	if (retval < 0)
		goto stop;

	chan = radio->registers[STATUS] & STATUS_READCH;

	dev_info(&radio->videodev->dev, "%s is done\n", __func__);
	dev_info(&radio->videodev->dev, "CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx STATUS=0x%4.4hx\n",
		radio->registers[CHANNEL], radio->registers[SEEKCFG1],
		radio->registers[STATUS]);


	/* Frequency (MHz) = Spacing (kHz) x Channel + Bottom of Band (MHz) */
	*freq = chan * spacing + band_bottom;
	dev_info(&radio->videodev->dev,
		"%s is done1 : band_bottom=%d, spacing=%d, chan=%d, freq=%d\n",
		__func__, band_bottom, spacing, chan, *freq);

	return retval;
}


/*
 * rtc6213n_set_freq - set the frequency
 */
int rtc6213n_set_freq(struct rtc6213n_device *radio, unsigned int freq)
{
	unsigned int spacing, band_bottom;
	unsigned short chan;

	/* Spacing (kHz) */
	switch ((radio->registers[CHANNEL] & CHANNEL_CSR0_CHSPACE) >> 10) {
	/* 0: 200 kHz (USA, Australia) */
	case 0:
		spacing = 0.200 * FREQ_MUL; break;
	/* 1: 100 kHz (Europe, Japan) */
	case 1:
		spacing = 0.100 * FREQ_MUL; break;
	/* 2:  50 kHz */
	default:
		spacing = 0.050 * FREQ_MUL; break;
	};

	/* Bottom of Band (MHz) */
	switch ((radio->registers[CHANNEL] & CHANNEL_CSR0_BAND) >> 12) {
	/* 0: 87.5 - 108 MHz (USA, Europe) */
	case 0:
		band_bottom = 87.5 * FREQ_MUL; break;
	/* 1: 76   - 108 MHz (Japan wide band) */
	default:
		band_bottom = 76   * FREQ_MUL; break;
	/* 2: 76   -  90 MHz (Japan) */
	case 2:
		band_bottom = 76   * FREQ_MUL; break;
	};

	if (freq < band_bottom)
		freq = band_bottom;

	/* Chan = [ Freq (Mhz) - Bottom of Band (MHz) ] / Spacing (kHz) */
	chan = (freq - band_bottom) / spacing;

	return rtc6213n_set_chan(radio, chan);
}


/*
 * rtc6213n_set_seek - set seek
 */
static int rtc6213n_set_seek(struct rtc6213n_device *radio,
	unsigned int seek_wrap, unsigned int seek_up)
{
	int retval = 0;
	unsigned long timeout;
	bool timed_out = 0;
	int i;
	unsigned short seekcfg1_val = radio->registers[SEEKCFG1];
#ifdef check_validch
	int	checksi_cnt = 0;
	int si_cnt = 0;
#endif

	dev_info(&radio->videodev->dev, "========= %s ==========\n", __func__);
	dev_info(&radio->videodev->dev, "RTC6213n seeking process is starting\n");
	dev_info(&radio->videodev->dev, "CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx STATUS=0x%4.4hx\n",
		radio->registers[CHANNEL], radio->registers[SEEKCFG1],
		radio->registers[STATUS]);

	if (seek_wrap)
		radio->registers[SEEKCFG1] &= ~SEEKCFG1_CSR0_SKMODE;
	else
		radio->registers[SEEKCFG1] |= SEEKCFG1_CSR0_SKMODE;

	if (seek_up)
		radio->registers[SEEKCFG1] |= SEEKCFG1_CSR0_SEEKUP;
	else
		radio->registers[SEEKCFG1] &= ~SEEKCFG1_CSR0_SEEKUP;

	retval = rtc6213n_set_register(radio, SEEKCFG1);
	if (retval < 0) {
		radio->registers[SEEKCFG1] = seekcfg1_val;
		goto done;
	}

	/* start seeking */
	radio->registers[SEEKCFG1] |= SEEKCFG1_CSR0_SEEK;

	retval = rtc6213n_set_register(radio, SEEKCFG1);
	if (retval < 0) {
		radio->registers[SEEKCFG1] = seekcfg1_val;
		goto done;
	}

	/* currently I2C driver only uses interrupt way to seek */
	if (radio->stci_enabled) {
		rtc6213n_wq_flag = SEEK_WAITING;
		wait();
		if (rtc6213n_wq_flag == SEEK_CANCEL) {
			dev_info(&radio->videodev->dev, "%s: SEEK_CANCEL %d\n",
				__func__, rtc6213n_wq_flag);
			radio->registers[SEEKCFG1] &= ~SEEKCFG1_CSR0_SEEK;
			retval = rtc6213n_set_register(radio, SEEKCFG1);
			if (retval < 0)
				radio->registers[SEEKCFG1] = seekcfg1_val;
		}

		rtc6213n_wq_flag = NO_WAIT;
	} else	{
		/* wait till seek operation has completed */
		timeout = jiffies + msecs_to_jiffies(seek_timeout);
		do {
			retval = rtc6213n_get_register(radio, STATUS);
			if (retval < 0)
				goto stop;
			timed_out = time_after(jiffies, timeout);
		} while (((radio->registers[STATUS] & STATUS_STD) == 0) &&
			(!timed_out));
	}
	dev_info(&radio->videodev->dev, "RTC6213n seeking process is done\n");
	dev_info(&radio->videodev->dev, "CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx STATUS=0x%4.4hx, STD = %d, SF = %d, RSSI = %d\n",
		radio->registers[CHANNEL], radio->registers[SEEKCFG1],
		radio->registers[STATUS],
		(radio->registers[STATUS] & STATUS_STD) >> 14,
		(radio->registers[STATUS] & STATUS_SF) >> 13,
		(radio->registers[RSSI] & RSSI_RSSI));

	if ((radio->registers[STATUS] & STATUS_STD) == 0) {
		dev_info(&radio->videodev->dev, "seek does not complete\n");
		retval = rtc6213n_get_all_registers(radio);
		for (i = 0; i < RADIO_REGISTER_NUM; i++)
			dev_info(&radio->videodev->dev, "radio->registers[%d] 0x%4.4hx", i, radio->registers[i]);
	}

	if (timed_out) {
		dev_info(&radio->videodev->dev, "seek timed out after %u ms\n",
			seek_timeout);
		retval = rtc6213n_get_all_registers(radio);
		for (i = 0; i < RADIO_REGISTER_NUM; i++)
			dev_info(&radio->videodev->dev, "radio->registers[%d] 0x%4.4hx", i, radio->registers[i]);
	}

stop:
	/* stop seeking : clear STD*/
	radio->registers[SEEKCFG1] &= ~SEEKCFG1_CSR0_SEEK;
	retval = rtc6213n_set_register(radio, SEEKCFG1);

done:
	if (radio->registers[STATUS] & STATUS_SF) {
		dev_info(&radio->videodev->dev, "seek failed / band limit reached\n");
		retval = ESPIPE;
	}
	/* try again, if timed out */
	else if ((retval == 0) && timed_out)
		retval = -EAGAIN;
#ifdef check_validch
	else {
		checksi_cnt = 0;
		do {
			retval = rtc6213n_get_register(radio, STATUS);
			dev_info(&radio->videodev->dev, "stereo status = 0x%4.4hx\n", (radio->registers[STATUS] & STATUS_SI));
			checksi_cnt++;
			usleep_range(1);	/* replace msleep with usleep_range */
			if ((radio->registers[STATUS] & STATUS_SI) && (retval >= 0))
				si_cnt++;
		} while (checksi_cnt < 50);
		#if 1
		if (si_cnt < 4)
			retval = -EINVAL;
		else
			retval = 0; 
		#endif
	}
#endif
	dev_info(&radio->videodev->dev, "========= %s End ==========\n\n", __func__);

	return retval;
}

static void wait(void)
{
	#if 0
	wait_event_interruptible(rtc6213n_wq,
		(rtc6213n_wq_flag == WAIT_OVER) ||
		(rtc6213n_wq_flag == SEEK_CANCEL));
	#else
	wait_event_interruptible_timeout(rtc6213n_wq,
		(rtc6213n_wq_flag == WAIT_OVER) ||
		(rtc6213n_wq_flag == SEEK_CANCEL),
		HZ*8);
	#endif
}

/*
 * rtc6213n_start - switch on radio
 */
int rtc6213n_start(struct rtc6213n_device *radio)
{
	int retval;
	u8 i2c_error;
	u16 swbk1[] = {
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7000, 0x4000,
		0x1A08, 0x0100, 0x0740, 0x0040, 0x005A, 0x02C0, 0x0000,
		0x1440, 0x0080, 0x0840, 0x0000, 0x4002, 0x805A, 0x0D35,
		0x7367, 0x0000};
	u16 swbk2[] = {
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7000, 0x8000,
		0x0000, 0x0000, 0x0333, 0x051C, 0x01EB, 0x01EB, 0x0333,
		0xF2AB, 0x7F8A, 0x0780, 0x0000, 0x1400, 0x405A, 0x0000,
		0x3200, 0x0000};
    /* New Added Register */
	u16 swbk3[] = {
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7000, 0xC000,
		0x188F, 0x9628, 0x4040, 0x80FF, 0xCFB0, 0x06F6, 0x0D40,
		0x0998, 0xC61F, 0x7126, 0x3F4B, 0xEED7, 0xB599, 0x674E,
		0x3112, 0x0000};
	u16 swbk4[] = {
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7000, 0x2000,
		0x050F, 0x0E85, 0x5AA6, 0xDC57, 0x8000, 0x00A3, 0x00A3,
		0xC018, 0x7F80, 0x3C08, 0xB6CF, 0x8100, 0x0000, 0x0140,
		0x4700, 0x0000};
    /* Modified Register */
	u16 swbk5[] = {
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7000, 0x6000,
		0x3590, 0x6311, 0x3008, 0x001C, 0x0D79, 0x7D2F, 0x8000,
		0x02A1, 0x771F, 0x323E, 0x262E, 0xA516, 0x8680, 0x0000,
		0x0000, 0x0000};
	u16 swbk7[] = {
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7000, 0xE000,
		0x11A2, 0x0F92, 0x0000, 0x0000, 0x0000, 0x0000, 0x801D,
		0x0000, 0x0000, 0x0072, 0x00FF, 0x001F, 0x03FF, 0x16D1,
		0x13B7, 0x0000};

	dev_info(&radio->videodev->dev, "RTC6213n_start0 : DeviceID=0x%4.4hx ChipID=0x%4.4hx Addr=0x%4.4hx\n",
		radio->registers[DEVICEID], radio->registers[CHIPID],
		radio->client->addr);

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

	if (retval < 0)	{
		dev_info(&radio->videodev->dev, "RTC6213n_start1 : retval = %d\n",
		retval);
		/* goto done;*/
	}
	msleep(30);

	/* Don't read all between writing 0x16AA and 0x96AA */
	i2c_error = 0;
	radio->registers[DEVICEID] = 0x96AA;
	retval = rtc6213n_set_register(radio, DEVICEID);
	/* recheck TH : 10 */
	while ((retval < 0) && (i2c_error < 10)) {
		retval = rtc6213n_set_register(radio, DEVICEID);
		i2c_error++;
	}

	if (retval < 0) {
		dev_info(&radio->videodev->dev, "RTC6213n_start2 : retval = %d\n",
		retval);
	}
	msleep(30);

	/* get device and chip versions */
	/* Have to update shadow buf from all register */
	if (rtc6213n_get_all_registers(radio) < 0) {
		retval = -EIO;
		goto done;
	}

	dev_info(&radio->videodev->dev, "======== before initail process ========\n");
	dev_info(&radio->videodev->dev, "RTC6213n_start2 : DeviceID=0x%4.4hx ChipID=0x%4.4hx Addr=0x%4.4hx\n",
		radio->registers[DEVICEID], radio->registers[CHIPID],
		radio->client->addr);

	retval = rtc6213n_set_serial_registers(radio, swbk1, 23);
	if (retval < 0)
		goto done;

	retval = rtc6213n_set_serial_registers(radio, swbk2, 23);
	if (retval < 0)
		goto done;

    /* New Added Register */
	retval = rtc6213n_set_serial_registers(radio, swbk3, 23);
	if (retval < 0)
		goto done;

	retval = rtc6213n_set_serial_registers(radio, swbk4, 23);
	if (retval < 0)
		goto done;

	retval = rtc6213n_set_serial_registers(radio, swbk5, 23);
	if (retval < 0)
		goto done;

	retval = rtc6213n_set_serial_registers(radio, swbk7, 23);
	if (retval < 0)
		goto done;

	/* get device and chip versions */
	if (rtc6213n_get_all_registers(radio) < 0) {
		retval = -EIO;
		goto done;
	}

done:
	return retval;
}

/*
 * rtc6213n_stop - switch off radio
 */
int rtc6213n_stop(struct rtc6213n_device *radio)
{
	int retval;

	/* sysconfig */
	radio->registers[SYSCFG] &= ~SYSCFG_CSR0_RDS_EN;
	radio->registers[SYSCFG] &= ~SYSCFG_CSR0_RDSIRQEN;
	radio->registers[SYSCFG] &= ~SYSCFG_CSR0_STDIRQEN;
	retval = rtc6213n_set_register(radio, SYSCFG);
	if (retval < 0)
		goto done;

	/* powerconfig */
	radio->registers[MPXCFG] &= ~MPXCFG_CSR0_DIS_MUTE;
	retval = rtc6213n_set_register(radio, MPXCFG);

	/* POWERCFG_ENABLE has to automatically go low */
	radio->registers[POWERCFG] |= POWERCFG_CSR0_DISABLE;
	radio->registers[POWERCFG] &= ~POWERCFG_CSR0_ENABLE;
	retval = rtc6213n_set_register(radio, POWERCFG);

	/* Set 0x16AA */
	radio->registers[DEVICEID] = 0x16AA;
	retval = rtc6213n_set_register(radio, DEVICEID);

done:
	return retval;
}

/*
 * rtc6213n_rds_on - switch on rds reception
 */
static int rtc6213n_rds_on(struct rtc6213n_device *radio)
{
	int retval;

	/* sysconfig */
	radio->registers[SYSCFG] |= SYSCFG_CSR0_RDS_EN;
	retval = rtc6213n_set_register(radio, SYSCFG);

	if (retval < 0)
		radio->registers[SYSCFG] &= ~SYSCFG_CSR0_RDS_EN;

	return retval;
}

#ifdef CONFIG_RDS
int rtc6213n_reset_rds_data(struct rtc6213n_device *radio)
{
	int retval = 0;

	dev_info(&radio->videodev->dev, "======= rtc6213n_reset_rds_data ======\n");
	//mutex_lock(&radio->lock);

	radio->wr_index = 0;
	radio->rd_index = 0;
	memset(radio->buffer, 0, radio->buf_size);
	//mutex_unlock(&radio->lock);

	return retval;
}
#endif

/**************************************************************************
 * File Operations Interface
 **************************************************************************/

/*
 * rtc6213n_fops_read - read RDS data
 */
static ssize_t rtc6213n_fops_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	struct rtc6213n_device *radio = video_drvdata(file);
	int retval = 0;
	unsigned int block_count = 0;

	/* switch on rds reception */
	mutex_lock(&radio->lock);
	/* if RDS is not on, then turn on RDS */
	if ((radio->registers[SYSCFG] & SYSCFG_CSR0_RDS_EN) == 0)
		rtc6213n_rds_on(radio);

	/* block if no new data available */
	while (radio->wr_index == radio->rd_index) {
		if (file->f_flags & O_NONBLOCK) {
			retval = -EWOULDBLOCK;
			goto done;
		}
		if (wait_event_interruptible(radio->read_queue,
				radio->wr_index != radio->rd_index) < 0) {
			retval = -EINTR;
			goto done;
		}
	}

	/* calculate block count from byte count */
	count /= 3;
	dev_info(&radio->videodev->dev, "%s: count = %zu\n",
		__func__, count);

	/* copy RDS block out of internal buffer and to user buffer */
	while (block_count < count) {
		if (radio->rd_index == radio->wr_index)
			break;
		/* always transfer rds complete blocks */
		if (copy_to_user(buf, &radio->buffer[radio->rd_index], 3))
			/* retval = -EFAULT; */
			break;
		/* increment and wrap read pointer */
		radio->rd_index += 3;
		if (radio->rd_index >= radio->buf_size)
			radio->rd_index = 0;
		/* increment counters */
		block_count++;
		buf += 3;
		retval += 3;
		dev_info(&radio->videodev->dev, "%s: block_count = %d, count = %zu\n",
			__func__, block_count, count);
	}

done:
	mutex_unlock(&radio->lock);
	return retval;
}

/*
 * rtc6213n_fops_poll - poll RDS data
 */
static unsigned int rtc6213n_fops_poll(struct file *file,
		struct poll_table_struct *pts)
{
	struct rtc6213n_device *radio = video_drvdata(file);
	int retval = 0;

	/* switch on rds reception */
	mutex_lock(&radio->lock);
	if ((radio->registers[SYSCFG] & SYSCFG_CSR0_RDS_EN) == 0)
		rtc6213n_rds_on(radio);
	mutex_unlock(&radio->lock);

	poll_wait(file, &radio->read_queue, pts);

	if (radio->rd_index != radio->wr_index)
		retval = POLLIN | POLLRDNORM;

	return retval;
}

/*
 * rtc6213n_fops - file operations interface
 */
static const struct v4l2_file_operations rtc6213n_fops = {
	.owner          =   THIS_MODULE,
	.read           =   rtc6213n_fops_read,
	.poll           =   rtc6213n_fops_poll,
	.unlocked_ioctl =   video_ioctl2,
	.open           =   rtc6213n_fops_open,
	.release        =   rtc6213n_fops_release,
};

/**************************************************************************
 * Video4Linux Interface
 **************************************************************************/
/*
 * rtc6213n_vidioc_queryctrl - enumerate control items
 */
static int rtc6213n_vidioc_queryctrl(struct file *file, void *priv,
	struct v4l2_queryctrl *qc)
{
	struct rtc6213n_device *radio = video_drvdata(file);
	int retval = -EINVAL;

	/* abort if qc->id is below V4L2_CID_BASE */
	if (qc->id < V4L2_CID_BASE)
		goto done;

	/* search video control */
	switch (qc->id) {
	case V4L2_CID_AUDIO_VOLUME:
		return v4l2_ctrl_query_fill(qc, 0, 15, 1, 15);
	case V4L2_CID_AUDIO_MUTE:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	}

	/* disable unsupported base controls */
	/* to satisfy kradio and such apps */
	if ((retval == -EINVAL) && (qc->id < V4L2_CID_LASTP1)) {
		qc->flags = V4L2_CTRL_FLAG_DISABLED;
		retval = 0;
	}

done:
	if (retval < 0)
		dev_warn(&radio->videodev->dev,
		"query controls failed with %d\n", retval);
	return retval;
}


/*
 * rtc6213n_vidioc_g_ctrl - get the value of a control
 */
static int rtc6213n_vidioc_g_ctrl(struct file *file, void *priv,
	struct v4l2_control *ctrl)
{
	struct rtc6213n_device *radio = video_drvdata(file);
	int retval = 0;

	mutex_lock(&radio->lock);

	/* safety checks */
	retval = rtc6213n_disconnect_check(radio);
	if (retval)
		goto done;

	dev_info(&radio->videodev->dev, "========= Get V4L2_CONTROL ==========\n");

	switch (ctrl->id) {
	case V4L2_CID_AUDIO_VOLUME:
	#ifdef New_VolumeControl
		ctrl->value = global_volume;
	#else
		ctrl->value = radio->registers[MPXCFG] & MPXCFG_CSR0_VOLUME;
	#endif
		break;
	case V4L2_CID_AUDIO_MUTE:
		ctrl->value = ((radio->registers[MPXCFG] &
			MPXCFG_CSR0_DIS_MUTE) == 0) ? 1 : 0;
		break;
	case V4L2_CID_PRIVATE_CSR0_DIS_SMUTE:
		ctrl->value = ((radio->registers[MPXCFG] &
			MPXCFG_CSR0_DIS_SMUTE) == 0) ? 1 : 0;
		break;
	case V4L2_CID_PRIVATE_CSR0_BLNDADJUST:
		ctrl->value = ((radio->registers[MPXCFG] &
			MPXCFG_CSR0_BLNDADJUST) >> 8);	
		break;		
	case V4L2_CID_PRIVATE_CSR0_BAND:
		ctrl->value = ((radio->registers[CHANNEL] &
			CHANNEL_CSR0_BAND) >> 12);
		break;
	case V4L2_CID_PRIVATE_CSR0_SEEKRSSITH:
		ctrl->value = radio->registers[SEEKCFG1] &
			SEEKCFG1_CSR0_SEEKRSSITH;
		break;
	case V4L2_CID_PRIVATE_RSSI:
		rtc6213n_get_all_registers(radio);
		ctrl->value = radio->registers[RSSI] & RSSI_RSSI;
		dev_info(&radio->videodev->dev, "Get V4L2_CONTROL V4L2_CID_PRIVATE_RSSI: STATUS=0x%4.4hx RSSI = %d\n",
			radio->registers[STATUS],
			radio->registers[RSSI] & RSSI_RSSI);
		dev_info(&radio->videodev->dev, "Get V4L2_CONTROL V4L2_CID_PRIVATE_RSSI: regC=0x%4.4hx RegD=0x%4.4hx\n",
			radio->registers[BA_DATA], radio->registers[BB_DATA]);
		dev_info(&radio->videodev->dev, "Get V4L2_CONTROL V4L2_CID_PRIVATE_RSSI: regE=0x%4.4hx RegF=0x%4.4hx\n",
			radio->registers[BC_DATA], radio->registers[BD_DATA]);
		break;
	case V4L2_CID_PRIVATE_DEVICEID:
		ctrl->value = radio->registers[DEVICEID] & DEVICE_ID;
		dev_info(&radio->videodev->dev, "Get V4L2_CONTROL V4L2_CID_PRIVATE_DEVICEID: DEVICEID=0x%4.4hx\n",
			radio->registers[DEVICEID]);
		break;
	case V4L2_CID_PRIVATE_CSR0_OFSTH:
		rtc6213n_get_all_registers(radio);
		ctrl->value = ((radio->registers[SEEKCFG2] &
			SEEKCFG2_CSR0_OFSTH) >> 8);
		dev_info(&radio->videodev->dev, "Get V4L2_CONTROL V4L2_CID_PRIVATE_CSR0_OFSTH : SEEKCFG2=0x%4.4hx\n",
			radio->registers[SEEKCFG2]);
		break;
	case V4L2_CID_PRIVATE_CSR0_QLTTH:
		rtc6213n_get_all_registers(radio);
		ctrl->value = radio->registers[SEEKCFG2] & SEEKCFG2_CSR0_QLTTH;
		dev_info(&radio->videodev->dev, "V4L2_CID_PRIVATE_CSR0_QLTTH : SEEKCFG2=0x%4.4hx\n",
			radio->registers[SEEKCFG2]);
		break;

	default:
		retval = -EINVAL;
	}

done:
	if (retval < 0)
		dev_warn(&radio->videodev->dev,
			"get control failed with %d\n", retval);
	dev_info(&radio->videodev->dev, "========= Get V4L2_CONTROL End ==========\n\n");

	mutex_unlock(&radio->lock);
	return retval;
}

#ifdef New_VolumeControl
/* Test for Richwave internal only */
/* SSMC can remove this part due to assignment from DT */
#if 1
/* Arrays of volume level 0 ~ 15 mapping to dB */
/* 0 is always kept to -44, and 15 is always kept to 0 */
int vol2db_dream_no1[16] = {
	-44, -43, -41, -37, -33, -29, -25, -22,
	-19, -16, -14, -12, -9, -6, -3, 0};

int vol2db_dream_no2[16] = {
	-44, -43, -41, -39, -35, -31, -27, -24,
	-21, -18, -15, -12, -9, -6, -3, 0};

int vol2db_dream_no3[16] = {
	-44, -43, -41, -39, -37, -35, -31, -27,
	-24, -20, -16, -12, -9, -6, -3, 0};

int vol2db_grace_no1[16] = {
	-44, -43, -39, -35, -31, -27, -24, -22,
	-20, -18, -14, -10, -6, -4, -2, 0};

int vol2db_grace_no2[16] = {
	-44, -43, -39, -35, -31, -28, -26, -24,
	-22, -20, -16, -12, -8, -4, -2, 0};

int vol2db_hero_no1[16] = {
	-44, -43, -39, -35, -31, -27, -23, -19,
	-15, -12, -10, -8, -6, -4, -2, 0};
#endif

int rtc6213n_set_vol(struct rtc6213n_device *radio, int db)
{
	int i;
	int retval = -EINVAL;
	u16 swbk7[] = {
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7000, 0xE000,
		0x11A2, 0x0F92, 0x0000, 0x0000, 0x0000, 0x0000, 0x801D,
		0x0000, 0x0000, 0x0072, 0x00FF, 0x001F, 0x03FF, 0x16D1,
		0x13B7, 0x0000};

	radio->registers[POWERCFG] = (db < -24) ?
		((db == -26) || (db == -28) ?
			(radio->registers[POWERCFG] & 0xFFF7) :
			(radio->registers[POWERCFG] | 0x0008)) :
		(radio->registers[POWERCFG] & 0xFFF7);
	if ((db == -25) || (db == -27) || (db == -29))
		retval = rtc6213n_set_register(radio, POWERCFG);

	/* clear bits[15:14] of bank7/addr0x6 */
	swbk7[20] &= 0x3FFF;

	if ((db > -25) && (db < -2))
		swbk7[20] |= ((db + 24) % 2) ? 0x4000 : 0x0000;

	/* Copy the values of bank0/addr0x2 - bank0/addr0x6 to bank7 array */
	for (i = 0; i < 6; i++)
		swbk7[i] = radio->registers[i+2];

	radio->registers[MPXCFG] &= 0xFFF0;
	switch (db) {
	case 0:
		radio->registers[MPXCFG] |= 0xF; break;
	case -1:
	case -2:
		radio->registers[MPXCFG] |= 0xE; break;
	case -3:
		radio->registers[MPXCFG] |= 0xC; break;
	case -4:
		radio->registers[MPXCFG] |= 0xD; break;
	case -5:
		radio->registers[MPXCFG] |= 0xB; break;
	case -6:
		radio->registers[MPXCFG] |= 0xC; break;
	case -7:
		radio->registers[MPXCFG] |= 0xA; break;
	case -8:
		radio->registers[MPXCFG] |= 0xB; break;
	case -9:
		radio->registers[MPXCFG] |= 0x9; break;
	case -10:
		radio->registers[MPXCFG] |= 0xA; break;
	case -11:
		radio->registers[MPXCFG] |= 0x8; break;
	case -12:
		radio->registers[MPXCFG] |= 0x9; break;
	case -13:
		radio->registers[MPXCFG] |= 0x7; break;
	case -14:
		radio->registers[MPXCFG] |= 0x8; break;
	case -15:
		radio->registers[MPXCFG] |= 0x6; break;
	case -16:
		radio->registers[MPXCFG] |= 0x7; break;
	case -17:
		radio->registers[MPXCFG] |= 0x5; break;
	case -18:
		radio->registers[MPXCFG] |= 0x6; break;
	case -19:
		radio->registers[MPXCFG] |= 0x4; break;
	case -20:
		radio->registers[MPXCFG] |= 0x5; break;
	case -21:
		radio->registers[MPXCFG] |= 0x3; break;
	case -22:
		radio->registers[MPXCFG] |= 0x4; break;
	case -23:
		radio->registers[MPXCFG] |= 0x2; break;
	case -24:
		radio->registers[MPXCFG] |= 0x3; break;
	case -25:
		radio->registers[MPXCFG] |= 0xA; break;
	case -26:
		radio->registers[MPXCFG] |= 0x2; break;
	case -27:
		radio->registers[MPXCFG] |= 0x9; break;
	case -28:
		radio->registers[MPXCFG] |= 0x1; break;
	case -29:
		radio->registers[MPXCFG] |= 0x8; break;
	case -30:
	case -31:
		radio->registers[MPXCFG] |= 0x7; break;
	case -32:
	case -33:
		radio->registers[MPXCFG] |= 0x6; break;
	case -34:
	case -35:
		radio->registers[MPXCFG] |= 0x5; break;
	case -36:
	case -37:
		radio->registers[MPXCFG] |= 0x4; break;
	case -38:
	case -39:
		radio->registers[MPXCFG] |= 0x3; break;
	case -40:
	case -41:
		radio->registers[MPXCFG] |= 0x2; break;
	case -42:
	case -43:
		radio->registers[MPXCFG] |= 0x1; break;
	case -44:
		radio->registers[MPXCFG] |= 0x0; break;
	/* Wrong db, set to level 7 */
	default:
		radio->registers[MPXCFG] |= 0x0; break;
	}

	g_current_vol = (int)(radio->registers[MPXCFG] & 0x000F);
	if ((g_current_vol - g_previous_vol) > 2)
		retval = rtc6213n_set_serial_registers(radio, swbk7, 23);

	g_previous_vol = g_current_vol;

	dev_info(&radio->videodev->dev,
		"set db=%d reg0_2=0x%4.4hx reg0_6=0x%4.4hx reg7_6=0x%4.4hx\n",
		db, radio->registers[MPXCFG],
		radio->registers[POWERCFG], swbk7[20]);

	swbk7[0] = radio->registers[MPXCFG];

	/* Write values of swbk7 array to rtc6213n */
	retval = rtc6213n_set_serial_registers(radio, swbk7, 23);

	return retval;
}

#endif

/*
 * rtc6213n_vidioc_s_ctrl - set the value of a control
 */
static int rtc6213n_vidioc_s_ctrl(struct file *file, void *priv,
	struct v4l2_control *ctrl)
{
	struct rtc6213n_device *radio = video_drvdata(file);
	int retval = 0;

	if (ctrl->id != V4L2_CID_PRIVATE_SEEK_CANCEL)
		mutex_lock(&radio->lock);

	/* safety checks */
	retval = rtc6213n_disconnect_check(radio);
	if (retval)
		goto done;

	dev_info(&radio->videodev->dev, "========= Set V4L2_CONTROL [0x%x]==========\n", ctrl->id);

	switch (ctrl->id) {
	case V4L2_CID_AUDIO_VOLUME:
		dev_info(&radio->videodev->dev, "V4L2_CID_AUDIO_VOLUME : MPXCFG=0x%4.4hx POWERCFG=0x%4.4hx\n",
			radio->registers[MPXCFG], radio->registers[POWERCFG]);
		/* Check range to avoid out of bounds access */
		if ((ctrl->value < 0) || (ctrl->value > (VOLUME_NUM - 1))) {
			dev_err(&radio->videodev->dev, "out of bounds volume %d", ctrl->value);
			retval = -EINVAL;
			goto done;
		}
	#ifdef New_VolumeControl
		/* Volume Setting No1 - 20160714 */
		global_volume = ctrl->value;
		if (radio->vol_db)
			retval = rtc6213n_set_vol(radio, radio->rx_vol[ctrl->value]);
		else {
			radio->registers[POWERCFG] = (ctrl->value < 9) ? (radio->registers[POWERCFG] | 0x0008) :
				(radio->registers[POWERCFG] & 0xFFF7);
			if (ctrl->value == 8)
				retval = rtc6213n_set_register(radio, POWERCFG);

			radio->registers[MPXCFG] &= ~MPXCFG_CSR0_VOLUME;
			radio->registers[MPXCFG] |=
				(ctrl->value < 9) ? ((ctrl->value == 0) ? 0 : (2*ctrl->value - 1)) : ctrl->value;

			dev_info(&radio->videodev->dev, "V4L2_CID_AUDIO_VOLUME No1: MPXCFG=0x%4.4hx POWERCFG=0x%4.4hx\n",
				radio->registers[MPXCFG], radio->registers[POWERCFG]);
			retval = rtc6213n_set_register(radio, POWERCFG);
		}
	#else
		radio->registers[MPXCFG] &= ~MPXCFG_CSR0_VOLUME;
		radio->registers[MPXCFG] |=
			(ctrl->value > 15) ? 8 : ctrl->value;
		dev_info(&radio->videodev->dev, "V4L2_CID_AUDIO_VOLUME : MPXCFG=0x%4.4hx POWERCFG=0x%4.4hx\n",
			radio->registers[MPXCFG], radio->registers[POWERCFG]);
		retval = rtc6213n_set_register(radio, MPXCFG);
	#endif
		break;
	case V4L2_CID_AUDIO_MUTE:
		if (ctrl->value == 1)
			radio->registers[MPXCFG] &= ~MPXCFG_CSR0_DIS_MUTE;
		else
			radio->registers[MPXCFG] |= MPXCFG_CSR0_DIS_MUTE;
		dev_info(&radio->videodev->dev, "MPXCFG_CSR0_DIS_MUTE : MPXCFG=0x%4.4hx\n",
			radio->registers[MPXCFG]);
		retval = rtc6213n_set_register(radio, MPXCFG);
		break;
	/* PRIVATE CID */
	case V4L2_CID_PRIVATE_CSR0_DIS_SMUTE:
		dev_info(&radio->videodev->dev,
				"V4L2_CID_PRIVATE_CSR0_DIS_SMUTE : before V4L2_CID_PRIVATE_CSR0_DIS_SMUTE");
		if (ctrl->value == 1)
			radio->registers[MPXCFG] &= ~MPXCFG_CSR0_DIS_SMUTE;
		else
			radio->registers[MPXCFG] |= MPXCFG_CSR0_DIS_SMUTE;
		retval = rtc6213n_set_register(radio, MPXCFG);
		break;
	case V4L2_CID_PRIVATE_CSR0_DEEM:
		dev_info(&radio->videodev->dev,
				"V4L2_CID_PRIVATE_CSR0_DEEM : before V4L2_CID_PRIVATE_CSR0_DEEM");
		if (ctrl->value == 1)
			radio->registers[MPXCFG] |= MPXCFG_CSR0_DEEM;
		else
			radio->registers[MPXCFG] &= ~MPXCFG_CSR0_DEEM;
		retval = rtc6213n_set_register(radio, MPXCFG);
		break;
	case V4L2_CID_PRIVATE_CSR0_BLNDADJUST:
		dev_info(&radio->videodev->dev,	"V4L2_CID_PRIVATE_CSR0_BLNDADJUST : MPXCFG=0x%4.4hx POWERCFG=0x%4.4hx\n",
				radio->registers[MPXCFG], radio->registers[POWERCFG]);
			radio->registers[MPXCFG] &= ~MPXCFG_CSR0_BLNDADJUST;
			radio->registers[MPXCFG] |= (ctrl->value << 8);
		retval = rtc6213n_set_register(radio, MPXCFG);
		break;
	case V4L2_CID_PRIVATE_CSR0_BAND:
		dev_info(&radio->videodev->dev,
				"V4L2_CID_PRIVATE_CSR0_BAND : CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx\n",
			radio->registers[CHANNEL], radio->registers[SEEKCFG1]);
		radio->registers[CHANNEL] &= ~CHANNEL_CSR0_BAND;
		radio->registers[CHANNEL] |= (ctrl->value << 12);
		dev_info(&radio->videodev->dev,
				"V4L2_CID_PRIVATE_CSR0_BAND : CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx\n",
			radio->registers[CHANNEL], radio->registers[SEEKCFG1]);
		retval = rtc6213n_set_register(radio, CHANNEL);
		break;
	case V4L2_CID_PRIVATE_CSR0_CHSPACE:
		dev_info(&radio->videodev->dev,
				"V4L2_CID_PRIVATE_CSR0_CHSPACE : CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx\n",
			radio->registers[CHANNEL], radio->registers[SEEKCFG1]);
		radio->registers[CHANNEL] &= ~CHANNEL_CSR0_CHSPACE;
		radio->registers[CHANNEL] |= (ctrl->value << 10);
		dev_info(&radio->videodev->dev,
				"V4L2_CID_PRIVATE_CSR0_CHSPACE : CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx\n",
			radio->registers[CHANNEL], radio->registers[SEEKCFG1]);
		retval = rtc6213n_set_register(radio, CHANNEL);
		break;
	case V4L2_CID_PRIVATE_CSR0_RDS_EN:
		dev_info(&radio->videodev->dev,
				"V4L2_CID_PRIVATE_CSR0_RDS_EN : CHANNEL=0x%4.4hx SYSCFG=0x%4.4hx\n",
			radio->registers[CHANNEL], radio->registers[SYSCFG]);
		rtc6213n_reset_rds_data(radio);
		radio->registers[SYSCFG] &= ~SYSCFG_CSR0_RDS_EN;
		radio->registers[SYSCFG] &= ~SYSCFG_CSR0_RDSIRQEN;
		radio->registers[SYSCFG] |= (ctrl->value << 15);
		radio->registers[SYSCFG] |= (ctrl->value << 12);
		dev_info(&radio->videodev->dev,
				"V4L2_CID_PRIVATE_CSR0_RDS_EN : CHANNEL=0x%4.4hx SYSCFG=0x%4.4hx\n",
			radio->registers[CHANNEL], radio->registers[SYSCFG]);
		retval = rtc6213n_set_register(radio, SYSCFG);
		break;
	case V4L2_CID_PRIVATE_SEEK_CANCEL:
		dev_info(&radio->videodev->dev,
				"V4L2_CID_PRIVATE_SEEK_CANCEL : MPXCFG=0x%4.4hx SEEKCFG1=0x%4.4hx\n",
			radio->registers[MPXCFG], radio->registers[SEEKCFG1]);
		if (rtc6213n_wq_flag == SEEK_WAITING) {
			rtc6213n_wq_flag = SEEK_CANCEL;
			wake_up_interruptible(&rtc6213n_wq);
			dev_info(&radio->videodev->dev,
					"V4L2_CID_PRIVATE_SEEK_CANCEL : sent SEEK_CANCEL signal\n");
		}
		dev_info(&radio->videodev->dev,
				"V4L2_CID_PRIVATE_SEEK_CANCEL : MPXCFG=0x%4.4hx SEEKCFG1=0x%4.4hx\n",
			radio->registers[MPXCFG], radio->registers[SEEKCFG1]);
		break;
	case V4L2_CID_PRIVATE_CSR0_SEEKRSSITH:
		dev_info(&radio->videodev->dev,
				"V4L2_CID_PRIVATE_CSR0_SEEKRSSITH : MPXCFG=0x%4.4hx SEEKCFG1=0x%4.4hx\n",
			radio->registers[MPXCFG], radio->registers[SEEKCFG1]);
		radio->registers[SEEKCFG1] &= ~SEEKCFG1_CSR0_SEEKRSSITH;
		radio->registers[SEEKCFG1] |= ctrl->value;

		dev_info(&radio->videodev->dev,
				"V4L2_CID_PRIVATE_CSR0_SEEKRSSITH : MPXCFG=0x%4.4hx SEEKCFG1=0x%4.4hx\n",
			radio->registers[MPXCFG], radio->registers[SEEKCFG1]);
		retval = rtc6213n_set_register(radio, SEEKCFG1);
		break;
	case V4L2_CID_PRIVATE_CSR0_OFSTH:
		dev_info(&radio->videodev->dev, "V4L2_CID_PRIVATE_CSR0_OFSTH : SEEKCFG2=0x%4.4hx\n",
			radio->registers[SEEKCFG2]);
		radio->registers[SEEKCFG2] &= ~SEEKCFG2_CSR0_OFSTH;
		radio->registers[SEEKCFG2] |= (ctrl->value << 8);

		dev_info(&radio->videodev->dev, "V4L2_CID_PRIVATE_CSR0_OFSTH : SEEKCFG2=0x%4.4hx\n",
			radio->registers[SEEKCFG2]);
		retval = rtc6213n_set_register(radio, SEEKCFG2);
		break;
	case V4L2_CID_PRIVATE_CSR0_QLTTH:
		dev_info(&radio->videodev->dev, "V4L2_CID_PRIVATE_CSR0_QLTTH : SEEKCFG2=0x%4.4hx\n",
			radio->registers[SEEKCFG2]);
		radio->registers[SEEKCFG2] &= ~SEEKCFG2_CSR0_QLTTH;
		radio->registers[SEEKCFG2] |= ctrl->value;
		dev_info(&radio->videodev->dev, "V4L2_CID_PRIVATE_CSR0_QLTTH : SEEKCFG2=0x%4.4hx\n",
			radio->registers[SEEKCFG2]);
		retval = rtc6213n_set_register(radio, SEEKCFG2);
		break;
	default:
		retval = -EINVAL;
		break;
	}

done:
	if (retval < 0)
		dev_warn(&radio->videodev->dev,
		"set control failed with %d\n", retval);
	dev_info(&radio->videodev->dev, "========= Set V4L2_CONTROL End [0x%x] =====\n\n",
		ctrl->id);

	if (ctrl->id != V4L2_CID_PRIVATE_SEEK_CANCEL)
		mutex_unlock(&radio->lock);

	return retval;
}


/*
 * rtc6213n_vidioc_g_audio - get audio attributes
 */
static int rtc6213n_vidioc_g_audio(struct file *file, void *priv,
	struct v4l2_audio *audio)
{
	/* driver constants */
	audio->index = 0;
	strcpy(audio->name, "Radio");
	audio->capability = V4L2_AUDCAP_STEREO;
	audio->mode = 0;

	return 0;
}

/*
 * rtc6213n_vidioc_g_tuner - get tuner attributes
 */
static int rtc6213n_vidioc_g_tuner(struct file *file, void *priv,
	struct v4l2_tuner *tuner)
{
	struct rtc6213n_device *radio = video_drvdata(file);
	int retval = 0;

	mutex_lock(&radio->lock);

	/* safety checks */
	retval = rtc6213n_disconnect_check(radio);
	if (retval)
		goto done;

	dev_info(&radio->videodev->dev,
		"%s: CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx RSSI=0x%4.4hx FREQ_MUL=0x%4.4hx\n",
		__func__, radio->registers[CHANNEL], radio->registers[SEEKCFG1],
		radio->registers[RSSI], FREQ_MUL);

	if (tuner->index != 0) {
		retval = -EINVAL;
		goto done;
	}

	retval = rtc6213n_get_register(radio, RSSI);
	if (retval < 0)
		goto done;

	/* driver constants */
	strcpy(tuner->name, "FM");
	tuner->type = V4L2_TUNER_RADIO;
	tuner->capability = V4L2_TUNER_CAP_LOW | V4L2_TUNER_CAP_STEREO |
		V4L2_TUNER_CAP_RDS | V4L2_TUNER_CAP_RDS_BLOCK_IO;

    /* range limits */
	switch ((radio->registers[CHANNEL] & CHANNEL_CSR0_BAND) >> 12) {
	/* 0: 87.5 - 108 MHz (USA, Europe, default) */
	default:
		tuner->rangelow  =  87.5 * FREQ_MUL;
		tuner->rangehigh = 108   * FREQ_MUL;
	break;
	/* 1: 76   - 108 MHz (Japan wide band) */
	case 1:
		tuner->rangelow  =  76   * FREQ_MUL;
		tuner->rangehigh = 108   * FREQ_MUL;
	break;
	/* 2: 76   -  90 MHz (Japan) */
	case 2:
		tuner->rangelow  =  76   * FREQ_MUL;
		tuner->rangehigh =  90   * FREQ_MUL;
	break;
	};

	/* stereo indicator == stereo (instead of mono) */
	if ((radio->registers[STATUS] & STATUS_SI) == 0)
		tuner->rxsubchans = V4L2_TUNER_SUB_MONO;
	else
		tuner->rxsubchans = V4L2_TUNER_SUB_MONO | V4L2_TUNER_SUB_STEREO;
	/* If there is a reliable method of detecting an RDS channel,
	 * then this code should check for that before setting this
	 * RDS subchannel.
	 */
	tuner->rxsubchans |= V4L2_TUNER_SUB_RDS;

	/* mono/stereo selector */
	if ((radio->registers[MPXCFG] & MPXCFG_CSR0_MONO) == 0)
		tuner->audmode = V4L2_TUNER_MODE_STEREO;
	else
		tuner->audmode = V4L2_TUNER_MODE_MONO;

	/* min is worst, max is best; rssi: 0..0xff */
	tuner->signal = (radio->registers[RSSI] & RSSI_RSSI);

done:
	if (retval < 0)
		dev_warn(&radio->videodev->dev,
			"get tuner failed with %d\n", retval);

	dev_info(&radio->videodev->dev,
		"%s: CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx RSSI=0x%4.4hx FREQ_MUL=0x%4.4hx\n",
		__func__, radio->registers[CHANNEL], radio->registers[SEEKCFG1],
		radio->registers[RSSI], FREQ_MUL);

	mutex_unlock(&radio->lock);
	return retval;
}


/*
 * rtc6213n_vidioc_s_tuner - set tuner attributes
 */
static int rtc6213n_vidioc_s_tuner(struct file *file, void *priv,
	const struct v4l2_tuner *tuner)
{
	struct rtc6213n_device *radio = video_drvdata(file);
	int retval = 0;

	mutex_lock(&radio->lock);

	/* safety checks */
	retval = rtc6213n_disconnect_check(radio);
	if (retval)
		goto done;

	if (tuner->index != 0)
		goto done;

	dev_info(&radio->videodev->dev,
		"rtc6213n_vidioc_s_tuner1: DeviceID=0x%4.4hx ChipID=0x%4.4hx MPXCFG=0x%4.4hx\n",
		radio->registers[DEVICEID], radio->registers[CHIPID],
		radio->registers[MPXCFG]);

	/* mono/stereo selector */
	switch (tuner->audmode) {
	case V4L2_TUNER_MODE_MONO:
		radio->registers[MPXCFG] |= MPXCFG_CSR0_MONO;  /* force mono */
		break;
	case V4L2_TUNER_MODE_STEREO:
		radio->registers[MPXCFG] &= ~MPXCFG_CSR0_MONO; /* try stereo */
		break;
	default:
		goto done;
	}

	retval = rtc6213n_set_register(radio, MPXCFG);

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

	dev_info(&radio->videodev->dev,
		"rtc6213n_vidioc_s_tuner2: DeviceID=0x%4.4hx ChipID=0x%4.4hx MPXCFG=0x%4.4hx\n",
		radio->registers[DEVICEID], radio->registers[CHIPID],
		radio->registers[MPXCFG]);
	retval = rtc6213n_get_register(radio, MPXCFG);
	dev_info(&radio->videodev->dev,
		"rtc6213n_vidioc_s_tuner3: DeviceID=0x%4.4hx ChipID=0x%4.4hx MPXCFG=0x%4.4hx\n",
		radio->registers[DEVICEID], radio->registers[CHIPID],
		radio->registers[MPXCFG]);

done:
	if (retval < 0)
		dev_warn(&radio->videodev->dev,
			"set tuner failed with %d\n", retval);
	mutex_unlock(&radio->lock);
	return retval;
}


/*
 * rtc6213n_vidioc_g_frequency - get tuner or modulator radio frequency
 */
static int rtc6213n_vidioc_g_frequency(struct file *file, void *priv,
	struct v4l2_frequency *freq)
{
	struct rtc6213n_device *radio = video_drvdata(file);
	int retval = 0;

	/* safety checks */
	mutex_lock(&radio->lock);

	retval = rtc6213n_disconnect_check(radio);
	if (retval)
		goto done;

	if (freq->tuner != 0) {
		retval = -EINVAL;
		goto done;
	}

	dev_info(&radio->videodev->dev, "rtc6213n_vidioc_g_frequency1 : *freq=%d\n",
		freq->frequency);
	freq->type = V4L2_TUNER_RADIO;
	retval = rtc6213n_get_freq(radio, &freq->frequency);
	dev_info(&radio->videodev->dev, "rtc6213n_vidioc_g_frequency2 : *freq=%d, retval=%d\n",
		freq->frequency, retval);

done:
	if (retval < 0)
		dev_warn(&radio->videodev->dev,
			"get frequency failed with %d\n", retval);
	mutex_unlock(&radio->lock);
	return retval;
}


/*
 * rtc6213n_vidioc_s_frequency - set tuner or modulator radio frequency
 */
static int rtc6213n_vidioc_s_frequency(struct file *file, void *priv,
	const struct v4l2_frequency *freq)
{
	struct rtc6213n_device *radio = video_drvdata(file);
	int retval = 0;

	mutex_lock(&radio->lock);

	/* safety checks */
	retval = rtc6213n_disconnect_check(radio);
	if (retval)
		goto done;

	if (freq->tuner != 0) {
		retval = -EINVAL;
		goto done;
	}

	dev_warn(&radio->videodev->dev, "%s freq = %d\n",
		__func__, freq->frequency);
	retval = rtc6213n_set_freq(radio, freq->frequency);

done:
	if (retval < 0)
		dev_warn(&radio->videodev->dev,
			"set frequency failed with %d\n", retval);

	mutex_unlock(&radio->lock);
	return retval;
}


/*
 * rtc6213n_vidioc_s_hw_freq_seek - set hardware frequency seek
 */
static int rtc6213n_vidioc_s_hw_freq_seek(struct file *file, void *priv,
	const struct v4l2_hw_freq_seek *seek)
{
	struct rtc6213n_device *radio = video_drvdata(file);
	int retval = 0;
#ifdef check_validch
	u16 swbk3[] = {
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7000, 0xC000,
		0x188F, 0x9628, 0x4040, 0x80FF, 0xCFB0, 0x06F6, 0x0D40,
		0x0998, 0xC61F, 0x7126, 0x3F4B, 0xEED7, 0xB599, 0x674E,
		0x3112, 0x0000};
	int invalidch_cnt = 0;
#endif

	mutex_lock(&radio->lock);
	dev_info(&radio->videodev->dev, "%s2: MPXCFG=0x%4.4hx CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx\n",
		__func__, radio->registers[MPXCFG], radio->registers[CHANNEL],
		radio->registers[SEEKCFG1]);

	/* safety checks */
	retval = rtc6213n_disconnect_check(radio);
	if (retval)
		goto done;

#ifdef check_validch
	swbk3[13] = 0x0C40;
	retval = rtc6213n_set_serial_registers(radio, swbk3, 23);
	if (retval < 0)
		goto done;
#endif

	dev_info(&radio->videodev->dev,
		"%s: MPXCFG=0x%4.4hx CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx seek_tuner=%d\n",
		__func__, radio->registers[MPXCFG], radio->registers[CHANNEL],
		radio->registers[SEEKCFG1], seek->tuner);

	if (seek->tuner != 0) {
		retval = -EINVAL;
		goto done;
	}

	dev_info(&radio->videodev->dev, "%s: MPXCFG=0x%4.4hx CHANNEL=0x%4.4hx SEEKCFG1=0x%4.4hx\n",
		__func__, radio->registers[MPXCFG], radio->registers[CHANNEL],
		radio->registers[SEEKCFG1]);

	retval = rtc6213n_set_seek(radio, seek->wrap_around, seek->seek_upward);

#ifdef check_validch
	invalidch_cnt = 0;
	while((invalidch_cnt < 5) && (retval == -EINVAL))
	{
		dev_info(&radio->videodev->dev, "%s: hardware seek fake channel no %d, retval = %d\n",
			__func__, invalidch_cnt, retval);
		retval = rtc6213n_set_seek(radio, seek->wrap_around, seek->seek_upward);
		invalidch_cnt++;
	}
#endif

done:
#ifdef check_validch
	swbk3[13] = 0x0D40;
	retval = rtc6213n_set_serial_registers(radio, swbk3, 23);
#endif
	if (retval < 0)
		dev_info(&radio->videodev->dev,
			"set hardware frequency seek failed with %d\n", retval);
	mutex_unlock(&radio->lock);
	return retval;
}


/*
 * rtc6213n_ioctl_ops - video device ioctl operations
 */
static const struct v4l2_ioctl_ops rtc6213n_ioctl_ops = {
	.vidioc_querycap            =   rtc6213n_vidioc_querycap,
	.vidioc_queryctrl           =   rtc6213n_vidioc_queryctrl,
	.vidioc_g_ctrl              =   rtc6213n_vidioc_g_ctrl,
	.vidioc_s_ctrl              =   rtc6213n_vidioc_s_ctrl,
	.vidioc_g_audio             =   rtc6213n_vidioc_g_audio,
	.vidioc_g_tuner             =   rtc6213n_vidioc_g_tuner,
	.vidioc_s_tuner             =   rtc6213n_vidioc_s_tuner,
	.vidioc_g_frequency         =   rtc6213n_vidioc_g_frequency,
	.vidioc_s_frequency         =   rtc6213n_vidioc_s_frequency,
	.vidioc_s_hw_freq_seek      =   rtc6213n_vidioc_s_hw_freq_seek,
};


/*
 * rtc6213n_viddev_template - video device interface
 */
struct video_device rtc6213n_viddev_template = {
	.fops           =   &rtc6213n_fops,
	.name           =   DRIVER_NAME,
	.release        =   video_device_release,
	.ioctl_ops      =   &rtc6213n_ioctl_ops,
};

/**************************************************************************
 * Module Interface
 **************************************************************************/

/*
 * rtc6213n_i2c_init - module init
 */
static __init int rtc6213n_init(void)
{
	pr_info(DRIVER_DESC ", Version " DRIVER_VERSION "\n");
	return rtc6213n_i2c_init();
}

/*
 * rtc6213n_i2c_exit - module exit
 */
static void __exit rtc6213n_exit(void)
{
	i2c_del_driver(&rtc6213n_i2c_driver);
}

module_init(rtc6213n_init);
module_exit(rtc6213n_exit);
