/*
 *
 * drivers/media/tdmb/tdmb.h
 *
 * tdmb driver
 *
 * Copyright (C) (2011, Samsung Electronics)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _TDMB_H_
#define _TDMB_H_

#include <linux/types.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/pinctrl/consumer.h>

#define TDMB_DEBUG

#ifdef TDMB_DEBUG
#define DPRINTK(fmt, arg...) pr_info("TDMB " fmt, ##arg)
#else
#define DPRINTK(x...) /* null */
#endif

#define TDMB_UNIT_TEST

#define TDMB_DEV_NAME	"tdmb"
#define TDMB_DEV_MAJOR	225
#define TDMB_DEV_MINOR	0

#define DMB_TS_SIZE	188

#define TDMB_RING_BUFFER_SIZE		(188 * 150 + 4 + 4)
#define TDMB_RING_BUFFER_MAPPING_SIZE	\
		(((TDMB_RING_BUFFER_SIZE - 1) / PAGE_SIZE + 1) * PAGE_SIZE)

/* commands */
#define IOCTL_MAGIC	't'
#define IOCTL_MAXNR	32

#define IOCTL_TDMB_GET_DATA_BUFFSIZE	_IO(IOCTL_MAGIC, 0)
#define IOCTL_TDMB_GET_CMD_BUFFSIZE	_IO(IOCTL_MAGIC, 1)
#define IOCTL_TDMB_POWER_ON		_IO(IOCTL_MAGIC, 2)
#define IOCTL_TDMB_POWER_OFF		_IO(IOCTL_MAGIC, 3)
#define IOCTL_TDMB_SCAN_FREQ_ASYNC	_IO(IOCTL_MAGIC, 4)
#define IOCTL_TDMB_SCAN_FREQ_SYNC	_IO(IOCTL_MAGIC, 5)
#define IOCTL_TDMB_SCANSTOP		_IO(IOCTL_MAGIC, 6)
#define IOCTL_TDMB_ASSIGN_CH		_IO(IOCTL_MAGIC, 7)
#define IOCTL_TDMB_GET_DM		_IO(IOCTL_MAGIC, 8)
#define IOCTL_TDMB_ASSIGN_CH_TEST	_IO(IOCTL_MAGIC, 9)
#define IOCTL_TDMB_SET_AUTOSTART	_IO(IOCTL_MAGIC, 10)


struct tdmb_dm {
	unsigned int	rssi;
	unsigned int	ber;
	unsigned int	per;
	unsigned int	antenna;
};

#define SUB_CH_NUM_MAX				64

#define ENSEMBLE_LABEL_MAX	16
#define SVC_LABEL_MAX	16

enum {
	TMID_MSC_STREAM_AUDIO		= 0x00,
	TMID_MSC_STREAM_DATA		= 0x01,
	TMID_FIDC					= 0x02,
	TMID_MSC_PACKET_DATA		= 0x03
};

enum {
	DSCTy_TDMB					= 0x18,
	/* Used for All-Zero Test */
	DSCTy_UNSPECIFIED			= 0x00
};

struct sub_ch_info_type {
	/* Sub Channel Information */
	unsigned char sub_ch_id; /* 6 bits */
	unsigned short start_addr; /* 10 bits */

	/* FIG 0/2	*/
	unsigned char tmid; /* 2 bits */
	unsigned char svc_type; /* 6 bits */
	unsigned int svc_id; /* 16/32 bits */
	unsigned char svc_label[SVC_LABEL_MAX+1]; /* 16*8 bits */
	unsigned char ecc;	/* 8 bits */
	unsigned char scids;	/* 4 bits */

	unsigned char ca_flags;
};

struct ensemble_info_type {
	unsigned int ensem_freq;	/* 4 bytes */
	unsigned char tot_sub_ch;	/* MAX: 64 */

	unsigned short ensem_id;
	unsigned char ensem_label[ENSEMBLE_LABEL_MAX+1];
	struct sub_ch_info_type sub_ch[SUB_CH_NUM_MAX];
};


#define TDMB_CMD_START_FLAG	0x7F
#define TDMB_CMD_END_FLAG		0x7E
#define TDMB_CMD_SIZE			30

/* Result Value */
#define DMB_FIC_RESULT_FAIL	0x00
#define DMB_FIC_RESULT_DONE	0x01
#define DMB_TS_PACKET_RESYNC	0x02

irqreturn_t tdmb_irq_handler(int irq, void *dev_id);
unsigned long tdmb_get_chinfo(void);
void tdmb_pull_data(struct work_struct *work);
bool tdmb_control_irq(bool set);
void tdmb_control_gpio(bool poweron);
bool tdmb_create_workqueue(void);
bool tdmb_destroy_workqueue(void);
bool tdmb_create_databuffer(int int_size);
void tdmb_destroy_databuffer(void);
void tdmb_init_data(void);
#if defined(CONFIG_TDMB_ANT_DET)
bool tdmb_ant_det_irq_set(bool set);
#endif


struct tdmb_if_gpio_func {
	void (*gpio_cfg_on)(void);
	void (*gpio_cfg_off)(void);
};

#if defined(CONFIG_TDMB_EBI)
struct tdmb_ebi_dt_data {
	u32 cs_base;
	u32 mem_size;
};
#endif

#if defined(CONFIG_TDMB_I2C)
struct tdmb_i2c_dev {
	struct i2c_client *client;
	struct mutex lock;
};
#endif
struct tdmb_dt_platform_data {
	int tdmb_irq;
	int tdmb_en;
	int tdmb_rst;
	int tdmb_use_rst;
	int tdmb_use_irq;
#ifdef CONFIG_TDMB_XTAL_FREQ
	int tdmb_xtal_freq;
#endif
	struct pinctrl *tdmb_pinctrl;
	struct pinctrl_state *pwr_on, *pwr_off;
#ifdef CONFIG_TDMB_ANT_DET
	int tdmb_ant_irq;
#endif
#ifdef CONFIG_TDMB_VREG_SUPPORT
	struct regulator *tdmb_vreg;
	const char *tdmb_vreg_name;
#endif
};

#define TDMB_ENUM_STR(x)		#x

#ifdef TDMB_UNIT_TEST
enum tdmb_unit_test_cmd {
	TDMB_UTCMD_GET_ID = 0,
};
#endif

struct tdmb_drv_data {


#ifdef TDMB_UNIT_TEST
	enum tdmb_unit_test_cmd test_cmd;
#endif


};

unsigned char tdmb_make_result
(
	unsigned char cmd,
	unsigned short data_len,
	unsigned char  *data
);
bool tdmb_store_data(unsigned char *data, unsigned long len);

struct tdmb_drv_func {
	bool (*init)(void);
	bool (*power_on)(int param);
	void (*power_off)(void);
	bool (*scan_ch)(struct ensemble_info_type *ensembleInfo,
						unsigned long freq);
	void (*get_dm)(struct tdmb_dm *info);
	bool (*set_ch)(unsigned long freq, unsigned char subchid,
						bool factory_test);
	void (*pull_data)(void);
	int (*get_int_size)(void);
};

extern unsigned int *tdmb_ts_head;
extern unsigned int *tdmb_ts_tail;
extern char *tdmb_ts_buffer;
extern unsigned int tdmb_ts_size;

#if defined(CONFIG_TDMB_FC8080)
struct tdmb_drv_func *fc8080_drv_func(void);
#endif
#if defined(CONFIG_TDMB_MTV318)
struct tdmb_drv_func *mtv318_drv_func(void);
#endif
#if defined(CONFIG_TDMB_MTV319)
struct tdmb_drv_func *mtv319_drv_func(void);
#endif
#if defined(CONFIG_TDMB_TCC3170)
struct tdmb_drv_func *tcc3170_drv_func(void);
extern struct tcbd_fic_ensbl *tcbd_fic_get_ensbl_info(s32 _disp);
#endif

extern unsigned long tdmb_get_if_handle(void);

#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
extern int tdmb_tsi_start(void (*callback)(u8 *data, u32 length), int packet_cnt);
extern int tdmb_tsi_stop(void);
extern int tdmb_tsi_init(void);
extern void tdmb_tsi_deinit(void);
#endif
#endif
