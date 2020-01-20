/*
 * driver/muic/sm5508-muic.c - SM5508 MUIC micro USB switch device driver
 *
 * Copyright (C) 2013 Samsung Electronics
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#define pr_fmt(fmt)	"[MUIC] " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/input.h>
#include <linux/switch.h>
#if defined(CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif
#include <linux/muic/muic.h>
#include <linux/battery/sec_battery.h>

#ifdef CONFIG_MUIC_NOTIFIER
#include <linux/muic/muic_notifier.h>
#endif

#include "sm5508-muic.h"

static struct sm5508_muic_usbsw *gusbsw;

#define ENUM_STR(x) {case(x): return #x; }
static const char *dev_to_str(muic_attached_dev_t n)
{
	switch (n) {
	ENUM_STR(ATTACHED_DEV_NONE_MUIC);
	ENUM_STR(ATTACHED_DEV_USB_MUIC);
	ENUM_STR(ATTACHED_DEV_CDP_MUIC);
	ENUM_STR(ATTACHED_DEV_OTG_MUIC);
	ENUM_STR(ATTACHED_DEV_TA_MUIC);
	ENUM_STR(ATTACHED_DEV_UNOFFICIAL_MUIC);
	ENUM_STR(ATTACHED_DEV_UNOFFICIAL_TA_MUIC);
	ENUM_STR(ATTACHED_DEV_UNOFFICIAL_ID_MUIC);
	ENUM_STR(ATTACHED_DEV_UNOFFICIAL_ID_TA_MUIC);
	ENUM_STR(ATTACHED_DEV_UNOFFICIAL_ID_ANY_MUIC);
	ENUM_STR(ATTACHED_DEV_UNOFFICIAL_ID_USB_MUIC);
	ENUM_STR(ATTACHED_DEV_UNOFFICIAL_ID_CDP_MUIC);
	ENUM_STR(ATTACHED_DEV_UNDEFINED_CHARGING_MUIC);
	ENUM_STR(ATTACHED_DEV_DESKDOCK_MUIC);
	ENUM_STR(ATTACHED_DEV_UNKNOWN_VB_MUIC);
	ENUM_STR(ATTACHED_DEV_DESKDOCK_VB_MUIC);
	ENUM_STR(ATTACHED_DEV_CARDOCK_MUIC);
	ENUM_STR(ATTACHED_DEV_JIG_UART_OFF_MUIC);
	ENUM_STR(ATTACHED_DEV_JIG_UART_OFF_VB_MUIC);
	ENUM_STR(ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC);
	ENUM_STR(ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC);
	ENUM_STR(ATTACHED_DEV_JIG_UART_ON_MUIC);
	ENUM_STR(ATTACHED_DEV_JIG_UART_ON_VB_MUIC);
	ENUM_STR(ATTACHED_DEV_JIG_USB_OFF_MUIC);
	ENUM_STR(ATTACHED_DEV_JIG_USB_ON_MUIC);
	ENUM_STR(ATTACHED_DEV_SMARTDOCK_MUIC);
	ENUM_STR(ATTACHED_DEV_SMARTDOCK_VB_MUIC);
	ENUM_STR(ATTACHED_DEV_SMARTDOCK_TA_MUIC);
	ENUM_STR(ATTACHED_DEV_SMARTDOCK_USB_MUIC);
	ENUM_STR(ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC);
	ENUM_STR(ATTACHED_DEV_AUDIODOCK_MUIC);
	ENUM_STR(ATTACHED_DEV_MHL_MUIC);
	ENUM_STR(ATTACHED_DEV_CHARGING_CABLE_MUIC);
	ENUM_STR(ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC);
	ENUM_STR(ATTACHED_DEV_AFC_CHARGER_PREPARE_DUPLI_MUIC);
	ENUM_STR(ATTACHED_DEV_AFC_CHARGER_5V_MUIC);
	ENUM_STR(ATTACHED_DEV_AFC_CHARGER_5V_DUPLI_MUIC);
	ENUM_STR(ATTACHED_DEV_AFC_CHARGER_9V_MUIC);
	ENUM_STR(ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC);
	ENUM_STR(ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC);
	ENUM_STR(ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC);
	ENUM_STR(ATTACHED_DEV_QC_CHARGER_5V_MUIC);
	ENUM_STR(ATTACHED_DEV_QC_CHARGER_ERR_V_MUIC);
	ENUM_STR(ATTACHED_DEV_QC_CHARGER_9V_MUIC);
	ENUM_STR(ATTACHED_DEV_HV_ID_ERR_UNDEFINED_MUIC);
	ENUM_STR(ATTACHED_DEV_HV_ID_ERR_UNSUPPORTED_MUIC);
	ENUM_STR(ATTACHED_DEV_HV_ID_ERR_SUPPORTED_MUIC);
	ENUM_STR(ATTACHED_DEV_HMT_MUIC);
	ENUM_STR(ATTACHED_DEV_VZW_ACC_MUIC);
	ENUM_STR(ATTACHED_DEV_VZW_INCOMPATIBLE_MUIC);
	ENUM_STR(ATTACHED_DEV_USB_LANHUB_MUIC);
	ENUM_STR(ATTACHED_DEV_TYPE2_CHG_MUIC);
	ENUM_STR(ATTACHED_DEV_UNSUPPORTED_ID_MUIC);
	ENUM_STR(ATTACHED_DEV_UNSUPPORTED_ID_VB_MUIC);
	ENUM_STR(ATTACHED_DEV_TIMEOUT_OPEN_MUIC);
	ENUM_STR(ATTACHED_DEV_UNDEFINED_RANGE_MUIC);
	ENUM_STR(ATTACHED_DEV_HICCUP_MUIC);
	ENUM_STR(ATTACHED_DEV_RDU_TA_MUIC);
	ENUM_STR(ATTACHED_DEV_AFC_CHARGER_9V_DUPLI_MUIC);
	ENUM_STR(ATTACHED_DEV_UNKNOWN_MUIC);
	ENUM_STR(ATTACHED_DEV_NUM);
	default:
		return "invalid";
	}
	return "invalid";
}

static int sm5508_muic_write_byte(struct i2c_client *client, int reg, int val)
{
	int ret;
	int retry = 0;
	int written = 0;

	ret = i2c_smbus_write_byte_data(client, reg, val);
	while (ret < 0) {
		pr_err("%s: failed to write reg(0x%x), retrying(%d)...\n",
			__func__, reg, retry);
		if (retry > 3) {
			pr_err("%s: retry failed!!\n", __func__);
			break;
		}
		msleep(100);
		written = i2c_smbus_read_byte_data(client, reg);
		if (written < 0)
			pr_err("%s: failed to write reg(0x%x) written(0x%x)\n",
				__func__, reg, written);
		ret = i2c_smbus_write_byte_data(client, reg, val);
		retry++;
	}

	return ret;
}

static int sm5508_muic_read_byte(struct i2c_client *client, int reg)
{
	int ret = 0;
	int retry = 0;

	ret = i2c_smbus_read_byte_data(client, reg);
	while (ret < 0) {
		pr_err("%s: failed to write reg(0x%x), retrying(%d)...\n",
			__func__, reg, retry);
		if (retry > 3) {
			pr_err("%s: retry failed!!\n", __func__);
			break;
		}
		msleep(100);
		ret = i2c_smbus_read_byte_data(client, reg);
		retry++;
	}

	return ret;
}

static void send_muic_noti(muic_attached_dev_t new_dev,
	bool attach)
{
	pr_info("%stached_dev=%s\n",
		attach ? "at" : "de", dev_to_str(new_dev));
#if defined(CONFIG_USE_SECOND_MUIC)
	new_dev += SECOND_MUIC_DEV;
#endif
	if (attach)
		muic_notifier_attach_attached_dev(new_dev);
	else
		muic_notifier_detach_attached_dev(new_dev);
}

static int sm5508_set_afc_ctrl_reg(struct sm5508_muic_usbsw *usbsw,
	int shift, bool on)
{
	struct i2c_client *client = usbsw->client;
	u8 reg_val = 0;
	int ret = 0;

	ret = sm5508_muic_read_byte(client,
		SM5508_MUIC_REG_AFC_CTRL);
	if (ret < 0)
		pr_err("%s:(%d)\n", __func__, ret);

	if (on)
		reg_val = ret | (0x1 << shift);
	else
		reg_val = ret & ~(0x1 << shift);

	if (reg_val ^ ret) {
		pr_info("%s: AFC_CTRL (0x%x) != (0x%x), update\n",
			__func__, reg_val, ret);
		ret = sm5508_muic_write_byte(client,
			SM5508_MUIC_REG_AFC_CTRL, reg_val);
		if (ret < 0)
			pr_err("failed to write AFC_CTRL(%d)\n", ret);
	} else {
		pr_info("%s: (0x%x), just return\n", __func__, ret);
		return 0;
	}

	return ret;
}

#if !defined(CONFIG_USE_SECOND_MUIC)
static int set_com_sw(struct sm5508_muic_usbsw *usbsw,
	enum sm5508_reg_manual_sw1_value reg_val)
{
	struct i2c_client *client = usbsw->client;
	int ret = 0;
	int value = 0;

	value = sm5508_muic_read_byte(client,
			SM5508_MUIC_REG_MANSW1);
	if (reg_val != value) {
		pr_info("%s: MANSW1 (0x%x) != (0x%x), update\n",
			__func__, reg_val, value);
		ret = sm5508_muic_write_byte(client,
			SM5508_MUIC_REG_MANSW1,	reg_val);
	} else {
		pr_info("%s: reg(0x%x), just return\n", __func__, reg_val);
		return 0;
	}

	return ret;
}

static int com_to_open(struct sm5508_muic_usbsw *usbsw)
{
	int ret = 0;

	ret = set_com_sw(usbsw, SM5508_MANSW1_OPEN);

	return ret;
}

static int com_to_usb(struct sm5508_muic_usbsw *usbsw)
{
	int ret = 0;

	ret = set_com_sw(usbsw, SM5508_MANSW1_TO_USB);

	return ret;
}

static int com_to_uart(struct sm5508_muic_usbsw *usbsw)
{
	int ret = 0;

	ret = set_com_sw(usbsw, SM5508_MANSW1_TO_UART);

	return ret;
}
#endif

static void sm5508_muic_reg_init(struct sm5508_muic_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int ret;

	pr_info("%s\n", __func__);

	ret = sm5508_muic_read_byte(client,
		SM5508_MUIC_REG_DEVID);
	pr_info("SM5508_MUIC_REG_DEVID: 0x%x\n", ret);

	ret = sm5508_muic_write_byte(client,
		SM5508_MUIC_REG_INTMASK1, SM5508_INT_MASK1_VALUE);
	if (ret < 0)
		pr_err("failed to write INTMASK1 %d\n", ret);

	ret = sm5508_muic_write_byte(client,
		SM5508_MUIC_REG_INTMASK2, SM5508_INT_MASK2_VALUE);
	if (ret < 0)
		pr_err("failed to write INTMASK2 %d\n", ret);

	ret = sm5508_muic_write_byte(client,
		SM5508_MUIC_REG_INTMASK3, SM5508_INT_MASK3_VALUE);
	if (ret < 0)
		pr_err("failed to write INTMASK3 %d\n", ret);

	ret = sm5508_muic_write_byte(client,
		SM5508_MUIC_REG_CTRL, SM5508_CONTROL_VALUE);
	if (ret < 0)
		pr_err("failed to write CTRL %d\n", ret);
}

static int sm5508_set_afc_voltage(struct sm5508_muic_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int afctxd = usbsw->afc_txd;

	if (usbsw->is_hv_disabled)
		afctxd = SM5508_MUIC_AFC_5V;

	pr_info("%s AFC_TXD[0x%02x]\n", __func__, afctxd);

	sm5508_muic_write_byte(client, SM5508_MUIC_REG_AFC_TXD, afctxd);
	sm5508_set_afc_ctrl_reg(usbsw, SM5508_AFC_ENAFC_SHIFT, 1);
	usbsw->afc_retry_count = 0;

	return 0;
}

static int sm5508_afc_ta_attach(struct sm5508_muic_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int vbus = 0, ret = 0;

	/* read VBUS VALID */
	vbus = sm5508_muic_read_byte(client, SM5508_MUIC_REG_VBUS_VALID);
	if ((vbus & 0x01) == 0x00) {
		pr_info("%s: VBUS[0x%02x] just return\n", __func__, vbus);
		return 0;
	}

	/* read clear : AFC_STATUS */
	ret = sm5508_muic_read_byte(client, SM5508_MUIC_REG_AFC_STATUS);
	pr_info("%s VBUS[0x%02x] AFC_STATUS[0x%02x]\n",
		__func__, vbus, ret);
	usbsw->attached_dev = ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC;
	send_muic_noti(usbsw->attached_dev, true);

	sm5508_set_afc_voltage(usbsw);

	return 0;
}

static int sm5508_afc_ta_accept(struct sm5508_muic_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int dev3 = 0, vol = 0;

	/* ENAFC set '0' */
	sm5508_set_afc_ctrl_reg(usbsw, SM5508_AFC_ENAFC_SHIFT, 0);
	dev3 = sm5508_muic_read_byte(client, SM5508_MUIC_REG_DEV_T3);
	vol = sm5508_muic_read_byte(client, SM5508_MUIC_REG_AFC_TXD);
	pr_info("%s dev3[0x%02x], AFC_TXD[0x%02x]\n",
		__func__, dev3, vol);

	if (dev3 & SM5508_DEV_TYPE3_AFC_TA_CHG) {
		vol = (vol&0xF0)>>4;
		if (vol == 0x07)
			usbsw->attached_dev = ATTACHED_DEV_AFC_CHARGER_12V_MUIC;
		else if (vol == 0x04)
			usbsw->attached_dev = ATTACHED_DEV_AFC_CHARGER_9V_MUIC;
		else
			usbsw->attached_dev = ATTACHED_DEV_AFC_CHARGER_5V_MUIC;

		send_muic_noti(usbsw->attached_dev, true);
	} else {
		usbsw->attached_dev = ATTACHED_DEV_AFC_CHARGER_5V_MUIC;
		send_muic_noti(usbsw->attached_dev, true);
	}

	return 0;
}

static int sm5508_afc_multi_byte(struct sm5508_muic_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int multi_byte[6] = {0, 0, 0, 0, 0, 0};
	int i = 0;
	int ret = 0;
	int voltage_find = 0;

	/* ENAFC set '0' */
	sm5508_set_afc_ctrl_reg(usbsw, SM5508_AFC_ENAFC_SHIFT, 0);

	/* read AFC_RXD1 ~ RXD6 */
	voltage_find = 0;

	pr_info("AFC_RXD :");

	for (i = 0 ; i < 6 ; i++) {
		multi_byte[i] = sm5508_muic_read_byte(client,
					SM5508_MUIC_REG_AFC_RXD1 + i);
		if (multi_byte[i] < 0)
			pr_info("failed to read AFC_RXD%d %d\n",
				i + 1, multi_byte[i]);

		pr_info(" [0x%02x]", multi_byte[i]);

		if (multi_byte[i] == 0x00)
			break;

		if (i >= 1) {   /* voltate find */
			if (((multi_byte[i] & 0xF0) >> 4) >= ((multi_byte[voltage_find] & 0xF0) >> 4))
				voltage_find = i;
		}
	}

	pr_info(" => AFC_RXD%d multi_byte[%d]=0x%02x\n",
		voltage_find + 1, voltage_find, multi_byte[voltage_find]);

	if (((multi_byte[i] & 0xF0) >> 4) > 0x04) {
		ret = sm5508_muic_write_byte(client,
			SM5508_MUIC_REG_AFC_TXD, SM5508_MUIC_AFC_9V);
		if (ret < 0)
			pr_err("err write AFC_TXD(%d)\n", ret);
		pr_info("AFC_TXD = 0x%02x\n", SM5508_MUIC_AFC_9V);
	} else {
		ret = sm5508_muic_write_byte(client,
			SM5508_MUIC_REG_AFC_TXD, multi_byte[voltage_find]);
		if (ret < 0)
			pr_err("err write AFC_TXD(%d)\n", ret);
	}

	/* ENAFC set '1' */
	sm5508_set_afc_ctrl_reg(usbsw, SM5508_AFC_ENAFC_SHIFT, 1);

	return 0;
}

static int sm5508_afc_error(struct sm5508_muic_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int value = 0;
	int dev3 = 0;

	pr_info("%s: afc_retry_count(%d)\n",
		__func__, usbsw->afc_retry_count);

	/* ENAFC set '0' */
	sm5508_set_afc_ctrl_reg(usbsw, SM5508_AFC_ENAFC_SHIFT, 0);

	/* read AFC_STATUS */
	value = sm5508_muic_read_byte(client, SM5508_MUIC_REG_AFC_STATUS);
	if (value < 0)
		pr_err("%s: err read AFC_STATUS %d\n", __func__, value);
	pr_info("AFCSTATUS [0x%02x]\n", value);

	if (usbsw->afc_retry_count < 5) {
		dev3 = sm5508_muic_read_byte(client, SM5508_MUIC_REG_DEV_T3);
		pr_info("DEV3[0x%02x]\n", dev3);
		/* QC20_TA */
		if ((dev3 & SM5508_DEV_TYPE3_QC20_TA_CHG) &&
			(usbsw->afc_retry_count >= 2)) {
			if (usbsw->afc_txd == SM5508_MUIC_AFC_9V &&
				!usbsw->is_hv_disabled) {
				usbsw->attached_dev = ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC;
				sm5508_set_afc_ctrl_reg(usbsw,
					SM5508_AFC_ENQC20_9V_SHIFT, 1);
				pr_info("%s: QC2.0 9V\n", __func__);
			} else {
				usbsw->attached_dev = ATTACHED_DEV_QC_CHARGER_5V_MUIC;
				sm5508_set_afc_ctrl_reg(usbsw,
					SM5508_AFC_ENQC20_9V_SHIFT, 0);
				pr_info("%s: QC2.0 5V\n", __func__);
			}
		} else {
			msleep(100); /* 100ms delay */
			/* ENAFC set '1' */
			sm5508_set_afc_ctrl_reg(usbsw,
				SM5508_AFC_ENAFC_SHIFT, 1);
			usbsw->afc_retry_count++;
			pr_info("%s: re-start AFC (afc_retry_count=%d)\n",
				__func__, usbsw->afc_retry_count);
		}
	} else {
		pr_info("%s: ENAFC end, afc_retry_count(%d)\n",
			__func__, usbsw->afc_retry_count);
		usbsw->attached_dev = ATTACHED_DEV_AFC_CHARGER_5V_MUIC;
		send_muic_noti(usbsw->attached_dev, true);
	}

	return 0;
}

static int sm5508_muic_set_voltage(int voltage)
{
	struct sm5508_muic_usbsw *usbsw = gusbsw;
	int now_voltage = 0, ret = 0;

	mutex_lock(&usbsw->muic_mutex);

	switch (voltage) {
	case 5:
		usbsw->afc_txd = SM5508_MUIC_AFC_5V;
		break;
	case 9:
		usbsw->afc_txd = SM5508_MUIC_AFC_9V;
		break;
	default:
		pr_err("%s: invalid value %d, return\n", __func__, voltage);
		ret = -EINVAL;
		goto failed;
	}

	if (usbsw->is_hv_disabled)
		voltage = 5;

	switch (usbsw->attached_dev) {
	case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
		now_voltage = 5;
		break;
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
		now_voltage = 9;
		break;
	default:
		break;
	}

	if (voltage == now_voltage) {
		pr_err("%s: same with current voltage, return\n", __func__);
		ret = -EINVAL;
		goto failed;
	}

	switch (usbsw->attached_dev) {
	case ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
		if (usbsw->afc_txd == SM5508_MUIC_AFC_9V &&
			!usbsw->is_hv_disabled) {
			sm5508_afc_ta_attach(usbsw);
		} else {
			/* reset the afc charger */
			sm5508_set_afc_ctrl_reg(usbsw, SM5508_AFC_DP_RESET_SHIFT, 1);
			sm5508_set_afc_ctrl_reg(usbsw, SM5508_AFC_DM_RESET_SHIFT, 1);
			msleep(60);
		}
		break;
	case ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC:
	case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
		if (usbsw->afc_txd == SM5508_MUIC_AFC_9V &&
			!usbsw->is_hv_disabled)
			sm5508_set_afc_ctrl_reg(usbsw,
				SM5508_AFC_ENQC20_9V_SHIFT, 1);
		else
			sm5508_set_afc_ctrl_reg(usbsw,
				SM5508_AFC_ENQC20_9V_SHIFT, 0);
		break;
	default:
		pr_err("%s: not a HV Charger %d, return\n", __func__, usbsw->attached_dev);
		ret = -EINVAL;
		goto failed;
	}

failed:
	mutex_unlock(&usbsw->muic_mutex);

	return ret;
}

static int sm5508_muic_hv_charger_disable(bool en)
{
	struct sm5508_muic_usbsw *usbsw = gusbsw;
	int voltage = 0;

	usbsw->is_hv_disabled = en;

	if (usbsw->afc_txd == SM5508_MUIC_AFC_9V)
		voltage = 9;
	else
		voltage = 5;

	return sm5508_muic_set_voltage(voltage);
}

static int sm5508_muic_attach_dev(struct sm5508_muic_usbsw *usbsw,
	muic_attached_dev_t new_dev)
{
#if defined(CONFIG_USE_SECOND_MUIC)
	switch (new_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_UNKNOWN_VB_MUIC:
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
		usbsw->attached_dev = new_dev;
		send_muic_noti(new_dev, true);
		break;
	default:
		pr_info("%s unkown charger %s\n",
			__func__, dev_to_str(new_dev));
		break;
	}
#else
	bool noti_f = true;

	pr_info("%s:attached_dev:%d new_dev:%d\n",
		__func__, usbsw->attached_dev, new_dev);

	noti_f = true;

	switch (new_dev) {
	case ATTACHED_DEV_USB_MUIC:
		/* To do : USB */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_CDP_MUIC:
		/* To do : CDP */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_TA_MUIC:
		/* To do : TA */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
		/* To do : Not used*/
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_USB_LANHUB_MUIC:
		/* To do : Not used */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_OTG_MUIC:
		/* To do : Not used : OTG */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		/* To do : JIG_USB_ON */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
		/* To do : JIG_USB_OFF */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		/* To do : JIG_UART_ON */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		/* To do : JIG_UART_OFF */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
		/* To do : Not used : PPD */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC:
		/* To do : Not used : TTY */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_DESKDOCK_MUIC:
		/* To do : Not used : AV */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_MHL_MUIC:
		/* To do : Not used : Others */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		/* To do : Not used : DESKDOCK + VB */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
		/* To do : AFC 9V */
		usbsw->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
		/* To do : QC20 9V */
		usbsw->attached_dev = new_dev;
		break;
	default:
		/* To do : Others */
		usbsw->attached_dev = new_dev;
		pr_info("%s: unsupported dev=%d, adc=0x%x, vbus=0x%x\n",
			__func__, new_dev, usbsw->adc, usbsw->vbus);
		break;
	}

#if defined(CONFIG_MUIC_NOTIFIER)
	if (noti_f)
		send_muic_noti(new_dev, true);
	else
		pr_info("%s: attach Noti. for (%d) discarded.\n",
			__func__, new_dev);
#endif	/* CONFIG_MUIC_NOTIFIER */
#endif	/* CONFIG_USE_SECOND_MUIC */
	return 0;
}

static int sm5508_muic_detach_dev(struct sm5508_muic_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int ret = 0;
#if defined(CONFIG_USE_SECOND_MUIC)

	pr_info("%s, 0x%x send noti\n", __func__, usbsw->attached_dev);

	/* AFC_CNTL : reset*/
	ret = sm5508_muic_write_byte(client, SM5508_MUIC_REG_AFC_CTRL, 0x00);
	if (ret < 0)
		pr_err("%s: err %d\n", __func__, ret);

	usbsw->afc_txd = SM5508_MUIC_AFC_9V;
	usbsw->attached_dev = ATTACHED_DEV_NONE_MUIC;

	send_muic_noti(usbsw->attached_dev, false);
#else
	bool noti_f = true;

	pr_info("%s: attached_dev:%d\n", __func__, usbsw->attached_dev);

	ret = com_to_open(usbsw);

	/* AFC_CNTL : reset*/
	ret = sm5508_muic_write_byte(client, SM5508_MUIC_REG_AFC_CTRL, 0x00);
	if (ret < 0)
		pr_err("%s: err %d\n", __func__, ret);

	switch (usbsw->attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
		/* To do */
		break;
	case ATTACHED_DEV_TA_MUIC:
		/* To do */
		break;
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
	case ATTACHED_DEV_USB_LANHUB_MUIC:
		/* To do */
		break;
	case ATTACHED_DEV_OTG_MUIC:
		/* To do */
		break;
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		/* To do */
		break;
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
	case ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		/* To do */
		break;
	case ATTACHED_DEV_MHL_MUIC:
		/* To do */
		break;
	case ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
		/* To do */
		break;
	default:
		pr_info("%s: invalid attached_dev type(%d)\n",
			__func__, usbsw->attached_dev);
		break;
	}

	usbsw->intr1	= 0;
	usbsw->intr2	= 0;
	usbsw->intr3	= 0;

	usbsw->dev1		= 0;
	usbsw->dev2		= 0;
	usbsw->dev3		= 0;
	usbsw->adc		= 0x1F;
	usbsw->vbus		= 0;
	usbsw->attached_dev	= ATTACHED_DEV_NONE_MUIC;

#if defined(CONFIG_MUIC_NOTIFIER)
	if (noti_f)
		send_muic_noti(usbsw->attached_dev, false);
	else
		pr_info("%s: detach Noti. for (%d) discarded.\n",
			__func__, usbsw->attached_dev);
#endif /* CONFIG_MUIC_NOTIFIER */
#endif	/* CONFIG_USE_SECOND_MUIC */

	return 0;

}

static void sm5508_muic_detect_dev(struct sm5508_muic_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	muic_attached_dev_t new_dev = ATTACHED_DEV_NONE_MUIC;
	int intr = MUIC_INTR_DETACH;
	int dev1 = 0, dev2 = 0, dev3 = 0, vbus = 0, adc = 0, chg_type = 0, status = 0;

	dev1 = sm5508_muic_read_byte(client, SM5508_MUIC_REG_DEV_T1);
	dev2 = sm5508_muic_read_byte(client, SM5508_MUIC_REG_DEV_T2);
	dev3 = sm5508_muic_read_byte(client, SM5508_MUIC_REG_DEV_T3);
	vbus = sm5508_muic_read_byte(client, SM5508_MUIC_REG_VBUS_VALID);
	adc  = sm5508_muic_read_byte(client, SM5508_MUIC_REG_ADC);
	chg_type = sm5508_muic_read_byte(client, SM5508_MUIC_REG_CHG_TYPE);
	status = sm5508_muic_read_byte(client, SM5508_MUIC_REG_STATUS);
	pr_info("dev[1:0x%02x, 2:0x%02x, 3:0x%02x]\n",
		dev1, dev2, dev3);
	pr_info("adc:0x%02x, vbus:0x%02x, chg_type:0x%02x, status:0x%02x\n",
		adc, vbus, chg_type, status);

	usbsw->dev1	= dev1;
	usbsw->dev2	= dev2;
	usbsw->dev3	= dev3;
	usbsw->adc	= adc;
	usbsw->vbus	= vbus;

	switch (dev1) {
	case SM5508_DEV_TYPE1_USB_OTG:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_OTG_MUIC;
		break;
	case SM5508_DEV_TYPE1_TA:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_TA_MUIC;
		break;
	case SM5508_DEV_TYPE1_CDP:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_CDP_MUIC;
		break;
	case SM5508_DEV_TYPE1_CARKIT_CHG:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_UNDEFINED_CHARGING_MUIC;
		break;
	case SM5508_DEV_TYPE1_UART:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_UNDEFINED_CHARGING_MUIC;
		break;
	case SM5508_DEV_TYPE1_USB:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_USB_MUIC;
		break;
	case SM5508_DEV_TYPE1_AUDIO_2:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_USB_LANHUB_MUIC;
		break;
	case SM5508_DEV_TYPE1_AUDIO_1:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_UNDEFINED_CHARGING_MUIC;
		break;
	default:
		break;
	}

	switch (dev2) {
	case SM5508_DEV_TYPE2_AV:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_DESKDOCK_MUIC;
		break;
	case SM5508_DEV_TYPE2_TTY:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC;
		break;
	case SM5508_DEV_TYPE2_PPD:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_CHARGING_CABLE_MUIC;
		break;
	case SM5508_DEV_TYPE2_JIG_UART_OFF:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
		break;
	case SM5508_DEV_TYPE2_JIG_UART_ON:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_JIG_UART_ON_MUIC;
		break;
	case SM5508_DEV_TYPE2_JIG_USB_OFF:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_JIG_USB_OFF_MUIC;
		break;
	case SM5508_DEV_TYPE2_JIG_USB_ON:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_JIG_USB_ON_MUIC;
		break;
	default:
		break;
	}

	switch (dev3) {
	case SM5508_DEV_TYPE3_AFC_TA_CHG:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_AFC_CHARGER_9V_MUIC;
		break;
	case SM5508_DEV_TYPE3_U200_CHG:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_TA_MUIC;
		break;
	case SM5508_DEV_TYPE3_LO_TA_CHG:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_TA_MUIC;
		break;
	case SM5508_DEV_TYPE3_AV_WITH_VBUS:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_DESKDOCK_VB_MUIC;
		break;
	case SM5508_DEV_TYPE3_DCD_OUT_SDP_CHG:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_USB_MUIC;
		break;
	case SM5508_DEV_TYPE3_QC20_TA_CHG:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_QC_CHARGER_9V_MUIC;
		break;
	case SM5508_DEV_TYPE3_MHL:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_MHL_MUIC;
		break;
	default:
		break;
	}

	if ((new_dev != ATTACHED_DEV_NONE_MUIC) &&
		(adc != SM5508_ADC_OPEN) && (intr == MUIC_INTR_DETACH)) {
		intr = MUIC_INTR_ATTACH;
		if (vbus & 0x01)
			new_dev = ATTACHED_DEV_UNKNOWN_VB_MUIC;
	}

#if defined(CONFIG_USE_SECOND_MUIC)
	if (intr == MUIC_INTR_ATTACH && vbus)
#else
	if (intr == MUIC_INTR_ATTACH)
#endif
		sm5508_muic_attach_dev(usbsw, new_dev);
	else
		sm5508_muic_detach_dev(usbsw);
}

static irqreturn_t sm5508_muic_irq_thread(int irq, void *data)
{
	struct sm5508_muic_usbsw *usbsw = data;
	struct i2c_client *client = usbsw->client;
	int intr1 = 0, intr2 = 0, intr3 = 0;
	int adc = 0, vbus = 0;

	mutex_lock(&usbsw->muic_mutex);

	intr1 = sm5508_muic_read_byte(client, SM5508_MUIC_REG_INT1);
	if (intr1 < 0) {
		pr_err("%s failed to read INT1 %d\n", __func__, intr1);
		sm5508_muic_write_byte(client, SM5508_MUIC_REG_RESET, 0x01);
		sm5508_muic_reg_init(usbsw);
		goto irq_end;
	}
	intr2 = sm5508_muic_read_byte(client, SM5508_MUIC_REG_INT2);
	if (intr2 < 0) {
		pr_err("%s failed to read INT2 %d\n", __func__, intr2);
		sm5508_muic_write_byte(client, SM5508_MUIC_REG_RESET, 0x01);
		sm5508_muic_reg_init(usbsw);
		goto irq_end;
	}
	intr3 = sm5508_muic_read_byte(client, SM5508_MUIC_REG_INT3);
	if (intr3 < 0) {
		pr_err("%s failed to read INT3 %d\n", __func__, intr3);
		sm5508_muic_write_byte(client, SM5508_MUIC_REG_RESET, 0x01);
		sm5508_muic_reg_init(usbsw);
		goto irq_end;
	}
	vbus  = sm5508_muic_read_byte(client, SM5508_MUIC_REG_VBUS_VALID);
	if (vbus < 0) {
		pr_err("%s failed to read vbus %d\n", __func__, vbus);
		sm5508_muic_write_byte(client, SM5508_MUIC_REG_RESET, 0x01);
		sm5508_muic_reg_init(usbsw);
		goto irq_end;
	}
	adc   = sm5508_muic_read_byte(client, SM5508_MUIC_REG_ADC);
	if (adc < 0) {
		pr_err("%s failed to read adc %d\n", __func__, adc);
		sm5508_muic_write_byte(client, SM5508_MUIC_REG_RESET, 0x01);
		sm5508_muic_reg_init(usbsw);
		goto irq_end;
	}

	pr_info("%s INT[0x%02x, 0x%02x, 0x%02x] vbus:0x%02x, adc:0x%02x",
		__func__, intr1, intr2, intr3, vbus, adc);
	if ((intr1 == 0x00) && (intr2 == 0x00) && (intr3 == 0x00) && (vbus == 0x00)) {
		sm5508_muic_write_byte(client, SM5508_MUIC_REG_RESET, 0x01);
		sm5508_muic_reg_init(usbsw);
		goto irq_end;
	}

	usbsw->intr1	= intr1;
	usbsw->intr2	= intr2;
	usbsw->intr3	= intr3;
	usbsw->adc = adc;
	usbsw->vbus = vbus;

	if ((intr1 & SM5508_INT1_ATTACH_MASK) ||
		(intr1 & SM5508_INT1_DETACH_MASK) ||
		(intr1 & SM5508_INT1_KP_MASK) ||
		(intr1 & SM5508_INT1_LKP_MASK) ||
		(intr1 & SM5508_INT1_LKR_MASK) ||
		(intr2 & SM5508_INT2_VBUS_U_OFF_MASK) ||
		(intr2 & SM5508_INT2_RSRV_ATTACH_MASK) ||
		(intr2 & SM5508_INT2_ADC_CHANGE_MASK) ||
		(intr2 & SM5508_INT2_STUCK_KEY_MASK) ||
		(intr2 & SM5508_INT2_STUCK_KEY_RCV_MASK) ||
		(intr2 & SM5508_INT2_MHL_MASK) ||
		(intr2 & SM5508_INT2_RID_CHARGER_MASK) ||
		(intr2 & SM5508_INT2_VBUS_U_ON_MASK)) {
		sm5508_muic_detect_dev(usbsw);
	/* AFC TA */
	} else if ((intr3 & SM5508_INT3_AFC_TA_ATTACHED_MASK) ||
				(intr3 & SM5508_INT3_AFC_ACCEPTED_MASK) ||
				(intr3 & SM5508_INT3_AFC_AFC_VBUS_9V_MASK) ||
				(intr3 & SM5508_INT3_AFC_MULTI_BYTE_MASK) ||
				(intr3 & SM5508_INT3_AFC_STA_CHG_MASK) ||
				(intr3 & SM5508_INT3_AFC_ERROR_MASK) ||
				(intr3 & SM5508_INT3_AFC_QC20ACCEPTED_MASK) ||
				(intr3 & SM5508_INT3_AFC_QC_VBUS_9V_MASK)) {
		if (intr3 & SM5508_INT3_AFC_ERROR_MASK) {
			sm5508_afc_error(usbsw);
			goto irq_end;
		}

		if (intr3 & SM5508_INT3_AFC_TA_ATTACHED_MASK) {
			sm5508_afc_ta_attach(usbsw);
			goto irq_end;
		}

		if (intr3 & SM5508_INT3_AFC_ACCEPTED_MASK) {
			sm5508_afc_ta_accept(usbsw);
			goto irq_end;
		}

		if (intr3 & SM5508_INT3_AFC_MULTI_BYTE_MASK) {
			sm5508_afc_multi_byte(usbsw);
			goto irq_end;
		}

		if (intr3 & SM5508_INT3_AFC_STA_CHG_MASK)
			pr_info("INT3_AFC_STA_CHG_MASK\n");

		if (intr3 & SM5508_INT3_AFC_AFC_VBUS_9V_MASK)
			pr_info("INT3_AFC_AFC_VBUS_9V_MASK\n");

		if (intr3 & SM5508_INT3_AFC_QC20ACCEPTED_MASK)
			pr_info("INT3_AFC_QC20ACCEPTED_MASK\n");

		if (intr3 & SM5508_INT3_AFC_QC_VBUS_9V_MASK) {
			pr_info("INT3_AFC_QC_VBUS_9V_MASK\n");
			sm5508_muic_attach_dev(usbsw, ATTACHED_DEV_QC_CHARGER_9V_MUIC);
		}

	} else {
		sm5508_muic_detect_dev(usbsw);
	}

irq_end:
	mutex_unlock(&usbsw->muic_mutex);

	return IRQ_HANDLED;
}

static int sm5508_muic_irq_init(struct sm5508_muic_usbsw *usbsw)
{
	struct i2c_client *i2c = usbsw->client;
	int ret = 0;

	if (!usbsw->pdata->gpio_irq) {
		pr_err("%s: No interrupt specified\n", __func__);
		return -ENXIO;
	}

	i2c->irq = gpio_to_irq(usbsw->pdata->gpio_irq);
	if (i2c->irq) {
		ret = request_threaded_irq(i2c->irq, NULL,
				sm5508_muic_irq_thread,
				(IRQF_TRIGGER_LOW | IRQF_ONESHOT),
				"sm5508 muic micro USB", usbsw);
		if (ret < 0) {
			pr_err("%s: failed to reqeust IRQ(%d)\n",
				__func__, i2c->irq);
			return ret;
		}

		enable_irq_wake(i2c->irq);
		if (ret < 0)
			pr_err("%s: failed to enable wakeup src\n", __func__);
	}

	return 0;
}

static void sm5508_muic_afc_init_detect(struct work_struct *work)
{
	struct sm5508_muic_usbsw *usbsw = container_of(work,
		struct sm5508_muic_usbsw, init_work.work);
	int ret = 0;
	int dev1 = 0, dev3 = 0, vbus = 0, afc_status = 0;
	int value = 0;

	dev1 = sm5508_muic_read_byte(usbsw->client, SM5508_MUIC_REG_DEV_T1);
	dev3 = sm5508_muic_read_byte(usbsw->client, SM5508_MUIC_REG_DEV_T3);
	vbus = sm5508_muic_read_byte(usbsw->client, SM5508_MUIC_REG_VBUS_VALID);
	afc_status = sm5508_muic_read_byte(usbsw->client, SM5508_MUIC_REG_STATUS);

	pr_info("%s dev1[0x%02x], dev3[0x%02x], vbus[0x%02x], afc[0x%02x]\n",
		__func__, dev1, dev3, vbus, afc_status);

	/* AFC is conncect durring the power off */
	if ((dev1 == SM5508_DEV_TYPE1_TA) &&
		(dev3 == 0x00) && (afc_status == 0x01)) {
		value = SM5508_MUIC_AFC_9V; /* 0x46 : 9V / 1.65A */
		ret = sm5508_muic_write_byte(usbsw->client,
			SM5508_MUIC_REG_AFC_TXD, value);
		if (ret != 0)
			pr_err("failed to write AFC_TXD(0x%X)\n", ret);

		/* ENAFC set '1' */
		sm5508_afc_ta_attach(usbsw);
	}
}

static int sm5508_muic_parse_dt(struct device *dev,
	struct sm5508_muic_platform_data *pdata)
{
	struct device_node *np = dev->of_node;

	if (np == NULL) {
		pr_err("%s: np NULL\n", __func__);
		return -EINVAL;
	}

	pdata->gpio_irq = of_get_named_gpio(np, "sm5508_muic,irq-gpio",	0);
	if (pdata->gpio_irq < 0) {
		pr_err("%s: failed to get gpio_irq : %d\n",
			__func__, pdata->gpio_irq);
		pdata->gpio_irq = 0;
	}

	pr_info("%s: gpio_irq: %u\n", __func__, pdata->gpio_irq);

	return 1;
}

static int sm5508_muic_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct sm5508_muic_usbsw *usbsw;
	struct sm5508_muic_platform_data *pdata;
	int ret = 0;

	pr_info("%s i2c addr : 0x%x\n", __func__, client->addr);

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			pr_err("Failed to allocate memory pdata\n");
			return -ENOMEM;
		}
		ret = sm5508_muic_parse_dt(&client->dev, pdata);
		if (ret < 0)
			return ret;
	} else {
		pdata = client->dev.platform_data;
	}

	if (!pdata)
		return -EINVAL;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	usbsw = kzalloc(sizeof(*usbsw), GFP_KERNEL);
	if (!usbsw) {
		pr_err("Failed to allocate memory usbsw\n");
		kfree(usbsw);
		return -ENOMEM;
	}

	usbsw->client = client;
	usbsw->pdata = pdata;
	usbsw->afc_txd = SM5508_MUIC_AFC_9V;
	gusbsw = usbsw;
#if defined(CONFIG_USE_SECOND_MUIC)
	usbsw->muic_pdata = &muic_pdata2;
	usbsw->muic_pdata->muic_afc_set_voltage_cb = sm5508_muic_set_voltage;
	usbsw->muic_pdata->muic_hv_charger_disable_cb = sm5508_muic_hv_charger_disable;
#else
	usbsw->muic_pdata = &muic_pdata;
	usbsw->muic_pdata->muic_afc_set_voltage_cb = NULL;
	usbsw->muic_pdata->muic_hv_charger_disable_cb = NULL;
#endif

	i2c_set_clientdata(client, usbsw);
	mutex_init(&usbsw->muic_mutex);
	sm5508_muic_reg_init(usbsw);
	ret = sm5508_muic_irq_init(usbsw);
	if (ret) {
		pr_info("failed to enable irq init\n");
		goto fail2;
	}

	/* initial cable detection */
	INIT_DELAYED_WORK(&usbsw->init_work, sm5508_muic_afc_init_detect);
	schedule_delayed_work(&usbsw->init_work, msecs_to_jiffies(2000));

	/* initial cable detection */
	sm5508_muic_irq_thread(-1, usbsw);
	return 0;

fail2:
	if (usbsw->pdata->gpio_irq)
		free_irq(usbsw->pdata->gpio_irq, usbsw);

	mutex_destroy(&usbsw->muic_mutex);
	i2c_set_clientdata(client, NULL);
	kfree(usbsw);
	return ret;
}

static int sm5508_muic_remove(struct i2c_client *client)
{
	struct sm5508_muic_usbsw *usbsw = i2c_get_clientdata(client);

	cancel_delayed_work(&usbsw->init_work);
	if (usbsw->pdata->gpio_irq) {
		disable_irq_wake(usbsw->pdata->gpio_irq);
		free_irq(usbsw->pdata->gpio_irq, usbsw);
	}
	mutex_destroy(&usbsw->muic_mutex);
	i2c_set_clientdata(client, NULL);

	kfree(usbsw);
	return 0;
}

static const struct i2c_device_id sm5508_muic_id[] = {
	{"sm5508_muic", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, sm5508_muic_id);

static const struct of_device_id sm5508_muic_i2c_match_table[] = {
	{ .compatible = "sm5508_muic",},
	{},
};
MODULE_DEVICE_TABLE(of, sm5508_muic_i2c_match_table);

static struct i2c_driver sm5508_muic_i2c_driver = {
	.driver = {
		.name = "sm5508_muic",
		.owner = THIS_MODULE,
		.of_match_table = sm5508_muic_i2c_match_table,
	},
	.probe  = sm5508_muic_probe,
	.remove = sm5508_muic_remove,
	.id_table = sm5508_muic_id,
};

static int __init sm5508_muic_init(void)
{
	return i2c_add_driver(&sm5508_muic_i2c_driver);
}
late_initcall(sm5508_muic_init);

static void __exit sm5508_muic_exit(void)
{
	i2c_del_driver(&sm5508_muic_i2c_driver);
}
module_exit(sm5508_muic_exit);

MODULE_DESCRIPTION("SM5508 MUIC Micro USB Switch driver");
MODULE_LICENSE("GPL");

