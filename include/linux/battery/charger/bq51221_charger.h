/*
 * p9015_charger.h
 * Samsung P9015 Charger Header
 *
 * Copyright (C) 2014 Samsung Electronics, Inc.
 * Yeongmi Ha <yeongmi86.ha@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __BQ51221_CHARGER_H
#define __BQ51221_CHARGER_H __FILE__

#include <linux/mfd/core.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <linux/battery/sec_charging_common.h>
#include <linux/wakelock.h>

/* REGISTER MAPS */
#define BQ51221_REG_CURRENT_REGISTER			0X01
#define BQ51221_REG_CURRENT_REGISTER2			0X02
#define BQ51221_REG_MAILBOX						0XE0
#define BQ51221_REG_POD_RAM						0XE1
#define BQ51221_REG_USER_HEADER					0XE2
#define BQ51221_REG_VRECT_STATUS				0XE3
#define BQ51221_REG_VOUT_STATUS					0XE4
#define BQ51221_REG_PWR_BYTE_STATUS				0XE8
#define BQ51221_REG_INDICATOR					0XEF
#define BQ51221_REG_PROP_PACKET_PAYLOAD			0XF1

/* WPC HEADER */
#define BQ51221_EPT_HEADER_EPT						0x02
#define BQ51221_EPT_HEADER_CS100					0x05

/* END POWER TRANSFER CODES IN WPC */
#define BQ51221_EPT_CODE_UNKOWN						0x00
#define BQ51221_EPT_CODE_CHARGE_COMPLETE			0x01
#define BQ51221_EPT_CODE_INTERNAL_FAULT				0x02
#define BQ51221_EPT_CODE_OVER_TEMPERATURE			0x03
#define BQ51221_EPT_CODE_OVER_VOLTAGE				0x04
#define BQ51221_EPT_CODE_OVER_CURRENT				0x05
#define BQ51221_EPT_CODE_BATTERY_FAILURE			0x06
#define BQ51221_EPT_CODE_RECONFIGURE				0x07
#define BQ51221_EPT_CODE_NO_RESPONSE				0x08

#define BQ51221_POWER_MODE_MASK					(0x1 << 0)
#define BQ51221_SEND_USER_PKT_DONE_MASK			(0x1 << 7)
#define BQ51221_SEND_USER_PKT_ERR_MASK			(0x3 << 5)
#define BQ51221_SEND_ALIGN_MASK					(0x1 << 3)
#define BQ51221_SEND_EPT_CC_MASK				(0x1 << 0)
#define BQ51221_SEND_EOC_MASK					(0x1 << 0)

#define BQ51221_CS100_VALUE						0x64
#define BQ51221_IOREG_100_VALUE					0x07
#define BQ51221_IOREG_90_VALUE					0x06
#define BQ51221_IOREG_60_VALUE					0x05
#define BQ51221_IOREG_50_VALUE					0x04
#define BQ51221_IOREG_40_VALUE					0x03
#define BQ51221_IOREG_30_VALUE					0x02
#define BQ51221_IOREG_20_VALUE					0x01
#define BQ51221_IOREG_10_VALUE					0x00

#define BQ51221_PTK_ERR_NO_ERR					0x00
#define BQ51221_PTK_ERR_ERR						0x01
#define BQ51221_PTK_ERR_ILLEGAL_HD				0x02
#define BQ51221_PTK_ERR_NO_DEF					0x03

enum {
	END_POWER_TRANSFER_CODE_UNKOWN = 0,
	END_POWER_TRANSFER_CODE_CHARGECOMPLETE,
	END_POWER_TRANSFER_CODE_INTERNAL_FAULT,
	END_POWER_TRANSFER_CODE_OVER_TEMPERATURE,
	END_POWER_TRANSFER_CODE_OVER_VOLTAGE,
	END_POWER_TRANSFER_CODE_OVER_CURRENT,
	END_POWER_TRANSFER_CODE_BATTERY_FAILURE,
	END_POWER_TRANSFER_CODE_RECONFIGURE,
	END_POWER_TRANSFER_CODE_NO_RESPONSE,
};

struct bq51221_charger_platform_data {
	int irq_gpio;
	int irq_base;
	int tsb_gpio;
	int cs100_status;
	int pad_mode;
	int wireless_cc_cv;
	int siop_level;
	bool default_voreg;
	char *wireless_charger_name;
};

#define bq51221_charger_platform_data_t \
	struct bq51221_charger_platform_data

struct bq51221_charger_data {
	struct i2c_client				*client;
	struct device           *dev;
	bq51221_charger_platform_data_t *pdata;
	struct mutex io_lock;

	struct power_supply psy_chg;
	struct wake_lock wpc_wake_lock;
	struct workqueue_struct *wqueue;
	struct work_struct	chgin_work;
	struct delayed_work	wpc_work;
	struct delayed_work	isr_work;

	int chg_irq;
	int irq_base;
	int irq_gpio;

	int wpc_state;
};

enum {
    BQ51221_EVENT_IRQ = 0,
    BQ51221_IRQS_NR,
};

enum {
    BQ51221_PAD_MODE_NONE = 0,
    BQ51221_PAD_MODE_WPC,
    BQ51221_PAD_MODE_PMA,
};

#endif /* __BQ51221_CHARGER_H */
