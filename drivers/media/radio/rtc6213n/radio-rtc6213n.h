/*
 *  drivers/media/radio/rtc6213n/radio-rtc6213n.h
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

/* driver definitions */
#define DRIVER_NAME "rtc6213n-fmtuner"

/* kernel includes */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <asm/unaligned.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

/* driver definitions */
#define DRIVER_AUTHOR "TianTsai Chang <changtt@richwave.com.tw>"
#define DRIVER_KERNEL_VERSION KERNEL_VERSION(1, 0, 1)
#define DRIVER_CARD "Richwave rtc6213n FM Tuner"
#define DRIVER_DESC "I2C radio driver for rtc6213n FM Tuner"
#define DRIVER_VERSION "1.0.1"

/**************************************************************************
 * Register Definitions
 **************************************************************************/
#define RADIO_REGISTER_SIZE         2       /* 16 register bit width */
#define RADIO_REGISTER_NUM          16      /* DEVICEID */
#define RDS_REGISTER_NUM            6       /* STATUSRSSI */

#define DEVICEID                    0       /* Device ID Code */
#define DEVICE_ID                   0xffff  /* [15:00] Device ID */
#define DEVICEID_PN                 0xf000  /* [15:12] Part Number */
#define DEVICEID_MFGID              0x0fff  /* [11:00] Manufacturer ID */

#define CHIPID                      1       /* Chip ID Code */
#define CHIPID_REVISION_NO          0xfc00  /* [15:10] Chip Reversion */

#define MPXCFG                      2       /* Power Configuration */
#define MPXCFG_CSR0_DIS_SMUTE       0x8000  /* [15:15] Disable Softmute */
#define MPXCFG_CSR0_DIS_MUTE        0x4000  /* [14:14] Disable Mute */
#define MPXCFG_CSR0_MONO            0x2000  /* [13:13] Mono or Auto Detect */
#define MPXCFG_CSR0_DEEM            0x1000  /* [12:12] DE-emphasis */
#define MPXCFG_CSR0_BLNDADJUST      0x0300  /* [09:08] Blending Adjust */
#define MPXCFG_CSR0_SMUTERATE       0x00c0  /* [07:06] Softmute Rate */
#define MPXCFG_CSR0_SMUTEATT        0x0030  /* [05:04] Softmute Attenuation */
#define MPXCFG_CSR0_VOLUME          0x000f  /* [03:00] Volume */

#define CHANNEL                     3       /* Tuning Channel Setting */
#define CHANNEL_CSR0_TUNE           0x8000  /* [15:15] Tune */
#define CHANNEL_CSR0_BAND           0x3000  /* [13:12] Band Select */
#define CHANNEL_CSR0_CHSPACE        0x0c00  /* [11:10] Channel Sapcing */
#define CHANNEL_CSR0_CH             0x03ff  /* [09:00] Tuning Channel */

#define SYSCFG                      4       /* System Configuration 1 */
#define SYSCFG_CSR0_RDSIRQEN        0x8000  /* [15:15] RDS Interrupt Enable */
#define SYSCFG_CSR0_STDIRQEN        0x4000  /* [14:14] STD Interrupt Enable */
#define SYSCFG_CSR0_DIS_AGC         0x2000  /* [13:13] Disable AGC */
#define SYSCFG_CSR0_RDS_EN          0x1000  /* [12:12] RDS Enable */
#define SYSCFG_CSR0_RBDS_M          0x0300  /* [09:08] MMBS setting */

#define SEEKCFG1                    5       /* Seek Configuration 1 */
#define SEEKCFG1_CSR0_SEEK          0x8000  /* [15:15] Enable Seek Function */
#define SEEKCFG1_CSR0_SEEKUP        0x4000  /* [14:14] Seek Direction */
#define SEEKCFG1_CSR0_SKMODE        0x2000  /* [13:13] Seek Mode */
#define SEEKCFG1_CSR0_SEEKRSSITH    0x00ff  /* [07:00] RSSI Seek Threshold */

#define POWERCFG                    6       /* Power Configuration */
#define POWERCFG_CSR0_ENABLE        0x8000  /* [15:15] Power-up Enable */
#define POWERCFG_CSR0_DISABLE       0x4000  /* [14:14] Power-up Disable */
#define POWERCFG_CSR0_BLNDOFS       0x0f00  /* [11:08] Blending Offset Value */

#define PADCFG                      7       /* PAD Configuration */
#define PADCFG_CSR0_RC_EN           0xf000  /* Internal crystal RC enable*/
#define PADCFG_CSR0_GPIO            0x0004  /* [03:02] General purpose I/O */

#define BANKCFG                     8       /* Bank Serlection */
/* contains reserved */

#define SEEKCFG2                    9       /* Seek Configuration 2 */
#define SEEKCFG2_CSR0_OFSTH         0xff00  /* [15:08] DC offset fail TH */
#define SEEKCFG2_CSR0_QLTTH         0x00ff  /* [07:00] Spike fail TH */

/* BOOTCONFIG only contains reserved [*/
#define STATUS                      10      /* Status and Work channel */
#define STATUS_RDS_RDY              0x8000  /* [15:15] RDS Ready */
#define STATUS_STD                  0x4000  /* [14:14] Seek/Tune Done */
#define STATUS_SF                   0x2000  /* [13:13] Seek Fail */
#define STATUS_RDS_SYNC             0x0800  /* [11:11] RDS synchronization */
#define STATUS_SI                   0x0400  /* [10:10] Stereo Indicator */
#define STATUS_READCH               0x03ff  /* [09:00] Read Channel */

#define RSSI                        11      /* RSSI and RDS error */
#define RSSI_RDS_BA_ERRS            0xc000  /* [15:14] RDS Block A Errors */
#define RSSI_RDS_BB_ERRS            0x3000  /* [15:14] RDS Block B Errors */
#define RSSI_RDS_BC_ERRS            0x0c00  /* [13:12] RDS Block C Errors */
#define RSSI_RDS_BD_ERRS            0x0300  /* [11:10] RDS Block D Errors */
#define RSSI_RSSI                   0x00ff  /* [09:00] Read Channel */

#define BA_DATA                     12      /* Block A data */
#define RDSA_RDSA                   0xffff  /* [15:00] RDS Block A Data */

#define BB_DATA                     13      /* Block B data */
#define RDSB_RDSB                   0xffff  /* [15:00] RDS Block B Data */

#define BC_DATA                     14      /* Block C data */
#define RDSC_RDSC                   0xffff  /* [15:00] RDS Block C Data */

#define BD_DATA                     15      /* Block D data */
#define RDSD_RDSD                   0xffff  /* [15:00] RDS Block D Data */

/* (V4L2_CID_PRIVATE_BASE + (<Register> << 4) + (<Bit Position> << 0)) */
/* already defined in <linux/videodev2.h> */
/* #define V4L2_CID_PRIVATE_BASE        0x08000000 */

#define RW_PRIBASE	V4L2_CID_PRIVATE_BASE
/* #define RTC6213N_IOC_POWERUP	(RW_PRIBASE + (POWERCFG<<4) + 15)
 * #define RTC6213N_IOC_POWERDOWN	(RW_PRIBASE + (POWERCFG<<4) + 14)
 */

#define V4L2_CID_PRIVATE_DEVICEID		(RW_PRIBASE + (DEVICEID<<4) + 0)
#define V4L2_CID_PRIVATE_CSR0_DIS_SMUTE	(RW_PRIBASE + (MPXCFG<<4) + 15)
#define V4L2_CID_PRIVATE_CSR0_DEEM		(RW_PRIBASE + (MPXCFG<<4) + 12)
#define V4L2_CID_PRIVATE_CSR0_BLNDADJUST (RW_PRIBASE + (MPXCFG<<4) + 8)

#define V4L2_CID_PRIVATE_CSR0_BAND		(RW_PRIBASE + (CHANNEL<<4) + 12)
#define V4L2_CID_PRIVATE_CSR0_CHSPACE	(RW_PRIBASE + (CHANNEL<<4) + 10)

#define V4L2_CID_PRIVATE_CSR0_DIS_AGC	(RW_PRIBASE + (SYSCFG<<4) + 13)
#define V4L2_CID_PRIVATE_CSR0_RDS_EN	(RW_PRIBASE + (SYSCFG<<4) + 12)

#define V4L2_CID_PRIVATE_SEEK_CANCEL	(RW_PRIBASE + (SEEKCFG1<<4) + 10)
#define V4L2_CID_PRIVATE_CSR0_SEEKRSSITH (RW_PRIBASE + (SEEKCFG1<<4) + 0)

#define V4L2_CID_PRIVATE_CSR0_OFSTH		(RW_PRIBASE + (SEEKCFG2<<4) + 7)
#define V4L2_CID_PRIVATE_CSR0_QLTTH		(RW_PRIBASE + (SEEKCFG2<<4) + 0)
#define V4L2_CID_PRIVATE_RDS_RDY	    (RW_PRIBASE + (STATUS<<4) + 15)
#define V4L2_CID_PRIVATE_STD	        (RW_PRIBASE + (STATUS<<4) + 14)
#define V4L2_CID_PRIVATE_SF		        (RW_PRIBASE + (STATUS<<4) + 13)
#define V4L2_CID_PRIVATE_RDS_SYNC		(RW_PRIBASE + (STATUS<<4) + 11)
#define V4L2_CID_PRIVATE_SI		        (RW_PRIBASE + (STATUS<<4) + 10)

#define V4L2_CID_PRIVATE_RSSI			(RW_PRIBASE + (RSSI<<4) + 0)

#define INIT_COMPLETION(x) ((x).done = 0)
#define WAIT_OVER			0
#define SEEK_WAITING		1
#define NO_WAIT				2
#define TUNE_WAITING		4
#define RDS_WAITING			5
#define SEEK_CANCEL			6

#define VOLUME_NUM 16

/**************************************************************************
 * General Driver Definitions
 **************************************************************************/

/*
 * rtc6213n_device - private data
 */
struct rtc6213n_device {
	struct video_device *videodev;

	/* driver management */
	unsigned int users;

	/* Richwave internal registers (0..15) */
	unsigned short registers[RADIO_REGISTER_NUM];

	/* RDS receive buffer */
	wait_queue_head_t read_queue;
	struct mutex lock;			/* buffer locking */
	unsigned char *buffer;      /* size is always multiple of three */
	unsigned int buf_size;
	unsigned int rd_index;
	unsigned int wr_index;

	struct completion completion;
	bool stci_enabled;      /* Seek/Tune Complete Interrupt */

	struct i2c_client *client;
	bool vol_db;
	int rx_vol[VOLUME_NUM];
	unsigned char blend_level;

	bool use_ext_lna;
	int fm_lna_gpio;
};

/**************************************************************************
 * Frequency Multiplicator
 **************************************************************************/
#define FREQ_MUL 1000
#define CONFIG_RDS

/**************************************************************************
 * Common Functions
 **************************************************************************/
extern struct i2c_driver rtc6213n_i2c_driver;
extern struct video_device rtc6213n_viddev_template;
extern struct tasklet_struct my_tasklet;
extern int rtc6213n_wq_flag;
extern wait_queue_head_t rtc6213n_wq;
extern int rtc6213n_get_all_registers(struct rtc6213n_device *radio);
extern int rtc6213n_get_register(struct rtc6213n_device *radio, int regnr);
extern int rtc6213n_set_register(struct rtc6213n_device *radio, int regnr);
extern int rtc6213n_set_serial_registers(struct rtc6213n_device *radio,
	u16 *data, int bytes);
int rtc6213n_i2c_init(void);
int rtc6213n_disconnect_check(struct rtc6213n_device *radio);
int rtc6213n_set_freq(struct rtc6213n_device *radio, unsigned int freq);
int rtc6213n_start(struct rtc6213n_device *radio);
int rtc6213n_stop(struct rtc6213n_device *radio);
int rtc6213n_fops_open(struct file *file);
int rtc6213n_fops_release(struct file *file);
int rtc6213n_vidioc_querycap(struct file *file, void *priv,
	struct v4l2_capability *capability);
