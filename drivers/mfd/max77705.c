/*
 * max77705.c - mfd core driver for the Maxim 77705
 *
 * Copyright (C) 2016 Samsung Electronics
 * Insun Choi <insun77.choi@samsung.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * This driver is based on max8997.c
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#include <linux/mfd/max77705.h>
#include <linux/mfd/max77705-private.h>
#include <linux/regulator/machine.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

#include <linux/muic/muic.h>
#include <linux/muic/max77705-muic.h>
#include <linux/ccic/max77705_usbc.h>
//#include <linux/ccic/max77705_pass2.h>
#include <linux/ccic/max77705_pass3.h>
#include <linux/ccic/max77705_pass4.h>
#if defined(CONFIG_MAX77705_FW_PID03_SUPPORT)
#include <linux/ccic/max77705C_pass2_PID03.h>
#else
#include <linux/ccic/max77705C_pass2.h>
#endif

#include <linux/usb_notify.h>
#if defined(CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif /* CONFIG_OF */
#include "../battery_v2/include/sec_charging_common.h"

#define I2C_ADDR_PMIC	(0xCC >> 1)	/* Top sys, Haptic */
#define I2C_ADDR_MUIC	(0x4A >> 1)
#define I2C_ADDR_CHG    (0xD2 >> 1)
#define I2C_ADDR_FG     (0x6C >> 1)
#define I2C_ADDR_DEBUG  (0xC4 >> 1)

#define I2C_RETRY_CNT	3

/*
 * pmic revision information
 */
struct max77705_revision_struct {
	u8 id;
	u8 rev;
	u8 logical_id;
};

static struct max77705_revision_struct max77705_revision[3] = {
	{ 0x5, 0x3, 0x3},	// MD05 PASS3
	{ 0x5, 0x4, 0x4},	// MD05 PASS4
	{ 0x15,0x2, 0x5},	// MD15 PASS2
};

static struct mfd_cell max77705_devs[] = {
#if defined(CONFIG_CCIC_MAX77705)
	{ .name = "max77705-usbc", },
#endif
#if defined(CONFIG_REGULATOR_MAX77705)
	{ .name = "max77705-safeout", },
#endif /* CONFIG_REGULATOR_MAX77705 */
#if defined(CONFIG_FUELGAUGE_MAX77705)
	{ .name = "max77705-fuelgauge", },
#endif
#if defined(CONFIG_CHARGER_MAX77705)
	{ .name = "max77705-charger", },
#endif
#if defined(CONFIG_MOTOR_DRV_MAX77705)
	{ .name = "max77705-haptic", },
#endif /* CONFIG_MAX77705_HAPTIC */
#if defined(CONFIG_LEDS_MAX77705_RGB)
	{ .name = "leds-max77705-rgb", },
#endif /* CONFIG_LEDS_MAX77705_RGB */
#if defined(CONFIG_LEDS_MAX77705_FLASH)
	{ .name = "max77705-flash", },
#endif
};

int max77705_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct max77705_dev *max77705 = i2c_get_clientdata(i2c);
	struct otg_notify *o_notify = get_otg_notify();
	int ret, i;

	mutex_lock(&max77705->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; ++i) {
		ret = i2c_smbus_read_byte_data(i2c, reg);
		if (ret >= 0)
			break;
		pr_info("%s:%s reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
			MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
	}
	mutex_unlock(&max77705->i2c_lock);
	if (ret < 0) {
		pr_info("%s:%s reg(0x%x), ret(%d)\n", MFD_DEV_NAME, __func__, reg, ret);
#if defined(CONFIG_USB_HW_PARAM)
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif
		return ret;
	}

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL_GPL(max77705_read_reg);

int max77705_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77705_dev *max77705 = i2c_get_clientdata(i2c);
	struct otg_notify *o_notify = get_otg_notify();
	int ret, i;

	mutex_lock(&max77705->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; ++i) {
		ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
		if (ret >= 0)
			break;
		pr_info("%s:%s reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
			MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
	}
	mutex_unlock(&max77705->i2c_lock);
	if (ret < 0) {
#if defined(CONFIG_USB_HW_PARAM)
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif
		return ret;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(max77705_bulk_read);

int max77705_read_word(struct i2c_client *i2c, u8 reg)
{
	struct max77705_dev *max77705 = i2c_get_clientdata(i2c);
	struct otg_notify *o_notify = get_otg_notify();
	int ret, i;

	mutex_lock(&max77705->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; ++i) {
		ret = i2c_smbus_read_word_data(i2c, reg);
		if (ret >= 0)
			break;
		pr_info("%s:%s reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
			MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
	}
	mutex_unlock(&max77705->i2c_lock);
#if defined(CONFIG_USB_HW_PARAM)
	if (ret < 0) {
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
	}
#endif
	return ret;
}
EXPORT_SYMBOL_GPL(max77705_read_word);

int max77705_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct max77705_dev *max77705 = i2c_get_clientdata(i2c);
	struct otg_notify *o_notify = get_otg_notify();
	int ret = -EIO, i;
	int timeout = 2000; /* 2sec */
	int interval = 100;

	while (ret == -EIO) {
		mutex_lock(&max77705->i2c_lock);
		for (i = 0; i < I2C_RETRY_CNT; ++i) {
			ret = i2c_smbus_write_byte_data(i2c, reg, value);
			if ((ret >= 0) || (ret == -EIO))
				break;
			pr_info("%s:%s reg(0x%02x), ret(%d), i2c_retry_cnt(%d/%d)\n",
				MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
		}
		mutex_unlock(&max77705->i2c_lock);

		if (ret < 0) {
			pr_info("%s:%s reg(0x%x), ret(%d), timeout %d\n",
					MFD_DEV_NAME, __func__, reg, ret, timeout);

			if (timeout < 0)
				break;

			msleep(interval);
			timeout -= interval;
		}
	}
#if defined(CONFIG_USB_HW_PARAM)
	if (o_notify && ret < 0)
		inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif
	return ret;
}
EXPORT_SYMBOL_GPL(max77705_write_reg);

int max77705_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77705_dev *max77705 = i2c_get_clientdata(i2c);
	struct otg_notify *o_notify = get_otg_notify();
	int ret = -EIO, i;
	int timeout = 2000; /* 2sec */
	int interval = 100;

	while (ret == -EIO) {
		mutex_lock(&max77705->i2c_lock);
		for (i = 0; i < I2C_RETRY_CNT; ++i) {
			ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
			if ((ret >= 0) || (ret == -EIO))
				break;
			pr_info("%s:%s reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
				MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
		}
		mutex_unlock(&max77705->i2c_lock);

		if (ret < 0) {
			pr_info("%s:%s reg(0x%x), ret(%d), timeout %d\n",
					MFD_DEV_NAME, __func__, reg, ret, timeout);

			if (timeout < 0)
				break;

			msleep(interval);
			timeout -= interval;
		}
	}
#if defined(CONFIG_USB_HW_PARAM)
	if (o_notify && ret < 0)
		inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif
	return ret;
}
EXPORT_SYMBOL_GPL(max77705_bulk_write);

int max77705_write_word(struct i2c_client *i2c, u8 reg, u16 value)
{
	struct max77705_dev *max77705 = i2c_get_clientdata(i2c);
	struct otg_notify *o_notify = get_otg_notify();
	int ret, i;

	mutex_lock(&max77705->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; ++i) {
		ret = i2c_smbus_write_word_data(i2c, reg, value);
		if (ret >= 0)
			break;
		pr_info("%s:%s reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
			MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
	}
	mutex_unlock(&max77705->i2c_lock);
	if (ret < 0) {
#if defined(CONFIG_USB_HW_PARAM)
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif
		return ret;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(max77705_write_word);

int max77705_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct max77705_dev *max77705 = i2c_get_clientdata(i2c);
	struct otg_notify *o_notify = get_otg_notify();
	int ret, i;
	u8 old_val, new_val;

	mutex_lock(&max77705->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; ++i) {
		ret = i2c_smbus_read_byte_data(i2c, reg);
		if (ret >= 0)
			break;
		pr_info("%s:%s read reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
			MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
	}
	if (ret < 0) {
#if defined(CONFIG_USB_HW_PARAM)
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif
		goto err;
	}
	if (ret >= 0) {
		old_val = ret & 0xff;
		new_val = (val & mask) | (old_val & (~mask));
		for (i = 0; i < I2C_RETRY_CNT; ++i) {
			ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
			if (ret >= 0)
				break;
			pr_info("%s:%s write reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
				MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
		}
		if (ret < 0) {
#if defined(CONFIG_USB_HW_PARAM)
			if (o_notify)
				inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif
			goto err;
		}
	}
err:
	mutex_unlock(&max77705->i2c_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(max77705_update_reg);

#if defined(CONFIG_OF)
static int of_max77705_dt(struct device *dev, struct max77705_platform_data *pdata)
{
	struct device_node *np_max77705 = dev->of_node;
	struct device_node *np_battery;
	int ret = 0;

	if (!np_max77705)
		return -EINVAL;

	pdata->irq_gpio = of_get_named_gpio(np_max77705, "max77705,irq-gpio", 0);
	pdata->wakeup = of_property_read_bool(np_max77705, "max77705,wakeup");
	if (of_property_read_u32(np_max77705, "max77705,fw_product_id", &pdata->fw_product_id))
		pdata->fw_product_id = 0;
	
#if defined(CONFIG_SEC_FACTORY)
	pdata->blocking_waterevent = 0;
#else
	pdata->blocking_waterevent = of_property_read_bool(np_max77705, "max77705,blocking_waterevent");
#endif
	pr_info("%s: irq-gpio: %u\n", __func__, pdata->irq_gpio);

	np_battery = of_find_node_by_name(NULL, "battery");
	if (!np_battery) {
		pr_info("%s: np_battery NULL\n", __func__);
	} else {
		pdata->wpc_en = of_get_named_gpio(np_battery, "battery,wpc_en", 0);
		if (pdata->wpc_en < 0) {
			pr_info("%s: can't get wpc_en (%d)\n", __func__, pdata->wpc_en);
			pdata->wpc_en = 0;
		}
		ret = of_property_read_string(np_battery,
				"battery,wireless_charger_name", (char const **)&pdata->wireless_charger_name);
		if (ret)
			pr_info("%s: Wireless charger name is Empty\n", __func__);
	}
	pdata->support_audio = of_property_read_bool(np_max77705, "max77705,support-audio");
	pr_info("%s: support_audio %d\n", __func__, pdata->support_audio);

	return 0;
}
#endif /* CONFIG_OF */
/* samsung */
#ifdef CONFIG_CCIC_MAX77705
static void max77705_reset_ic(struct max77705_dev *max77705)
{
	msg_maxim("Reset!!");
	max77705_write_reg(max77705->muic, 0x80, 0x0F);
	msleep(100);
}

static void max77705_usbc_wait_response_q(struct work_struct *work)
{
	struct max77705_dev *max77705;
	u8 read_value = 0x00;
	u8 dummy[2] = { 0, };

	max77705 = container_of(work, struct max77705_dev, fw_work);

	while (max77705->fw_update_state == FW_UPDATE_WAIT_RESP_START) {
		switch (max77705->pmic_rev) {
		case MAX77705_PASS1:
			max77705_bulk_read(max77705->muic, REG_UIC_INT-1, 2, dummy);
			read_value = dummy[1];
			break;
		default:
			max77705_bulk_read(max77705->muic, REG_UIC_INT, 1, dummy);
			read_value = dummy[0];
			break;
		}
		if ((read_value & BIT_APCmdResI) == BIT_APCmdResI)
			break;
	}

	complete_all(&max77705->fw_completion);
}

static int max77705_usbc_wait_response(struct max77705_dev *max77705)
{
	unsigned long time_remaining = 0;

	max77705->fw_update_state = FW_UPDATE_WAIT_RESP_START;

	init_completion(&max77705->fw_completion);
	queue_work(max77705->fw_workqueue, &max77705->fw_work);

	time_remaining = wait_for_completion_timeout(
			&max77705->fw_completion,
			msecs_to_jiffies(FW_WAIT_TIMEOUT));

	max77705->fw_update_state = FW_UPDATE_WAIT_RESP_STOP;

	if (!time_remaining) {
		msg_maxim("Failed to update due to timeout");
		cancel_work_sync(&max77705->fw_work);
		return FW_UPDATE_TIMEOUT_FAIL;
	}

	return 0;
}

static int __max77705_usbc_fw_update(
		struct max77705_dev *max77705, const u8 *fw_bin)
{
	u8 fw_cmd = FW_CMD_END;
	u8 fw_len = 0;
	u8 fw_opcode = 0;
	u8 fw_data_len = 0;
	u8 fw_data[FW_CMD_WRITE_SIZE] = { 0, };
	u8 verify_data[FW_VERIFY_DATA_SIZE] = { 0, };
	int ret = -FW_UPDATE_CMD_FAIL;

	/*
	 * fw_bin[0] = Write Command (0x01)
	 * or
	 * fw_bin[0] = Read Command (0x03)
	 * or
	 * fw_bin[0] = End Command (0x00)
	 */
	fw_cmd = fw_bin[0];

	/*
	 * Check FW Command
	 */
	if (fw_cmd == FW_CMD_END) {
		max77705_reset_ic(max77705);
		max77705->fw_update_state = FW_UPDATE_END;
		return FW_UPDATE_END;
	}

	/*
	 * fw_bin[1] = Length ( OPCode + Data )
	 */
	fw_len = fw_bin[1];

	/*
	 * Check fw data length
	 * We support 0x22 or 0x04 only
	 */
	if (fw_len != 0x22 && fw_len != 0x04)
		return FW_UPDATE_MAX_LENGTH_FAIL;

	/*
	 * fw_bin[2] = OPCode
	 */
	fw_opcode = fw_bin[2];

	/*
	 * In case write command,
	 * fw_bin[35:3] = Data
	 *
	 * In case read command,
	 * fw_bin[5:3]  = Data
	 */
	fw_data_len = fw_len - 1; /* exclude opcode */
	memcpy(fw_data, &fw_bin[3], fw_data_len);

	switch (fw_cmd) {
	case FW_CMD_WRITE:
		if (fw_data_len > I2C_SMBUS_BLOCK_MAX) {
			/* write the half data */
			max77705_bulk_write(max77705->muic,
					fw_opcode,
					I2C_SMBUS_BLOCK_HALF,
					fw_data);
			max77705_bulk_write(max77705->muic,
					fw_opcode + I2C_SMBUS_BLOCK_HALF,
					fw_data_len - I2C_SMBUS_BLOCK_HALF,
					&fw_data[I2C_SMBUS_BLOCK_HALF]);
		} else
			max77705_bulk_write(max77705->muic,
					fw_opcode,
					fw_data_len,
					fw_data);

		ret = max77705_usbc_wait_response(max77705);
		if (ret)
			return ret;

		/*
		 * Why do we need 1ms sleep in case MQ81?
		 */
		/* msleep(1); */

		return FW_CMD_WRITE_SIZE;


	case FW_CMD_READ:
		max77705_bulk_read(max77705->muic,
				fw_opcode,
				fw_data_len,
				verify_data);
		/*
		 * Check fw data sequence number
		 * It should be increased from 1 step by step.
		 */
		if (memcmp(verify_data, &fw_data[1], 2)) {
			msg_maxim("[0x%02x 0x%02x], [0x%02x, 0x%02x], [0x%02x, 0x%02x]",
					verify_data[0], fw_data[0],
					verify_data[1], fw_data[1],
					verify_data[2], fw_data[2]);
			return FW_UPDATE_VERIFY_FAIL;
		}

		return FW_CMD_READ_SIZE;
	}

	msg_maxim("Command error");

	return ret;
}

int max77705_write_jig_on(struct max77705_dev *max77705)
{
	u8 write_values[OPCODE_MAX_LENGTH] = { 0, };
	int ret = 0;
	int length = 0x2;
	int i = 0;

	write_values[0] = OPCODE_CTRL3_W;
	write_values[1] = 0x1;
	write_values[2] = 0x1;

	for (i = 0; i < length + OPCODE_SIZE; i++)
		msg_maxim("[%d], 0x[%x]", i, write_values[i]);

	/* Write opcode and data */
	ret = max77705_bulk_write(max77705->muic, OPCODE_WRITE,
			length + OPCODE_SIZE, write_values);
	/* Write end of data by 0x00 */
	if (length < OPCODE_DATA_LENGTH)
		max77705_write_reg(max77705->muic, OPCODE_WRITE_END, 0x00);
	return 0;
}

static int max77705_fuelgauge_read_vcell(struct max77705_dev *max77705)
{
	u8 data[2];
	u32 vcell;
	u16 w_data;
	u32 temp;
	u32 temp2;

	if (max77705_bulk_read(max77705->fuelgauge, VCELL_REG, 2, data) < 0) {
		pr_err("%s: Failed to read VCELL\n", __func__);
		return -1;
	}

	w_data = (data[1] << 8) | data[0];

	temp = (w_data & 0xFFF) * 78125;
	vcell = temp / 1000000;

	temp = ((w_data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	vcell += (temp2 << 4);

	return vcell;
}

static void max77705_wc_control(struct max77705_dev *max77705, bool enable)
{
	struct power_supply *psy = NULL;
	union power_supply_propval value = {0, };
	char wpc_en_status[2];

	psy = get_power_supply_by_name(max77705->pdata->wireless_charger_name);

	if (psy) {
		wpc_en_status[0] = WPC_EN_CCIC;
		wpc_en_status[1] = enable ? true : false;
		value.strval= wpc_en_status;
		if ((psy->desc->set_property != NULL) &&
			(psy->desc->set_property(psy, (enum power_supply_property)POWER_SUPPLY_EXT_PROP_WPC_EN, &value) >= 0))
			pr_info("%s: WC CONTROL: %s\n", __func__, wpc_en_status[1] ? "Enable" : "Disable");
		power_supply_put(psy);
	} else {
		if (max77705->pdata->wpc_en) {
			if (enable) {
				gpio_direction_output(max77705->pdata->wpc_en, 0);
				pr_info("%s: WC CONTROL: ENABLE\n", __func__);
			} else {
				gpio_direction_output(max77705->pdata->wpc_en, 1);
				pr_info("%s: WC CONTROL: DISABLE\n", __func__);
			}
		} else {
			pr_info("%s : no wpc_en\n", __func__);
		}
	}

	pr_info("%s: wpc_en(%d)\n", __func__, gpio_get_value(max77705->pdata->wpc_en));
}

int max77705_usbc_fw_update(struct max77705_dev *max77705,
		const u8 *fw_bin, int fw_bin_len, int enforce_do)
{
	max77705_fw_header *fw_header;
	struct otg_notify *o_notify = get_otg_notify();
	int offset = 0;
	unsigned long duration = 0;
	int size = 0;
	int try_count = 0;
	int ret = 0;
	u8 pd_status1 = 0x0;
	static u8 fct_id; /* FCT cable */
	u8 try_command = 0;
	u8 sw_boot = 0;
	u8 chg_cnfg_00 = 0;
	bool chg_mode_changed = 0;
	bool wpc_en_changed = 0;
	int vcell = 0;
	u8 vbvolt = 0;
	u8 wcin_dtls = 0;
	u8 chg_curr = 0;
	u8 vchgin = 0;
	int error = 0;


	fw_header = (max77705_fw_header *)fw_bin;
	msg_maxim("FW: magic/%x/ major/%x/ minor/%x/ product_id/%x/ rev/%x/ id/%x/",
		  fw_header->magic, fw_header->major, fw_header->minor, fw_header->product_id, fw_header->rev, fw_header->id);
	if(max77705->device_product_id != fw_header->product_id) {
		msg_maxim("product indicator mismatch");
		return 0;
	}
	if(fw_header->magic == MAX77705_SIGN)
		msg_maxim("FW: matched");

	max77705_read_reg(max77705->charger, MAX77705_CHG_REG_CNFG_00, &chg_cnfg_00);
retry:
	disable_irq(max77705->irq);
	max77705_write_reg(max77705->muic, REG_PD_INT_M, 0xFF);
	max77705_write_reg(max77705->muic, REG_CC_INT_M, 0xFF);
	max77705_write_reg(max77705->muic, REG_UIC_INT_M, 0xFF);
	max77705_write_reg(max77705->muic, REG_VDM_INT_M, 0xFF);

	offset = 0;
	duration = 0;
	size = 0;
	ret = 0;

	/* to do (unmask interrupt) */
	ret = max77705_read_reg(max77705->muic, REG_UIC_FW_REV, &max77705->FW_Revision);
	ret = max77705_read_reg(max77705->muic, REG_UIC_FW_MINOR, &max77705->FW_Minor_Revision);
	if (ret < 0 && (try_count == 0 && try_command == 0)) {
		msg_maxim("Failed to read FW_REV");
		error = -EIO;
		goto out;
	}

	duration = jiffies;

	max77705->FW_Product_ID = max77705->FW_Minor_Revision >> FW_PRODUCT_ID_REG;
	max77705->FW_Minor_Revision &= MINOR_VERSION_MASK;
	msg_maxim("chip : %02X.%02X(PID 0x%x), FW : %02X.%02X(PID 0x%x)",
			max77705->FW_Revision, max77705->FW_Minor_Revision, max77705->FW_Product_ID,
			fw_header->major, fw_header->minor, fw_header->product_id);
	store_ccic_bin_version(&fw_header->major, &sw_boot);

#ifdef CONFIG_SEC_FACTORY
	if ((max77705->FW_Revision != fw_header->major) || (max77705->FW_Minor_Revision != fw_header->minor)) {
#else
	if ((max77705->FW_Revision == 0xff) || (max77705->FW_Revision < fw_header->major) ||
		((max77705->FW_Revision == fw_header->major) && (max77705->FW_Minor_Revision < fw_header->minor)) ||
		(max77705->FW_Product_ID != fw_header->product_id) ||
		enforce_do) {
#endif
		if (!enforce_do && max77705->pmic_rev > MAX77705_PASS1) { /* on Booting time */
			max77705_read_reg(max77705->muic, REG_PD_STATUS1, &pd_status1);
			fct_id = (pd_status1 & BIT_FCT_ID) >> FFS(BIT_FCT_ID);
			msg_maxim("FCT_ID : 0x%x", fct_id);
		}

		if (try_count == 0 && try_command == 0) {
			/* change chg_mode during FW update */
			vcell = max77705_fuelgauge_read_vcell(max77705);

			if (vcell < 3600) {
				pr_info("%s: keep chg_mode(0x%x), vcell(%dmv)\n",
					__func__, chg_cnfg_00 & 0x0F, vcell);
				error = -EAGAIN;
				goto out;
			}
		}

		max77705_read_reg(max77705->charger, MAX77705_CHG_REG_DETAILS_00, &wcin_dtls);
		wcin_dtls = (wcin_dtls & 0x18) >> 3;

		wpc_en_changed = true;
		max77705_wc_control(max77705, false);

		max77705_read_reg(max77705->muic, MAX77705_USBC_REG_BC_STATUS, &vbvolt);

		pr_info("%s: BC:0x%02x, vbvolt:0x%x, wcin_dtls:0x%x\n",
			__func__, vbvolt, ((vbvolt & 0x80) >> 7), wcin_dtls);

		if (!(vbvolt & 0x80) && (wcin_dtls != 0x3)) {
			chg_mode_changed = true;
					/* Switching Frequency : 3MHz */
			max77705_update_reg(max77705->charger,
						MAX77705_CHG_REG_CNFG_08, 0x00,	0x3);
			pr_info("%s: +set Switching Frequency 3Mhz\n", __func__);

					/* Disable skip mode */
			max77705_update_reg(max77705->charger,
						MAX77705_CHG_REG_CNFG_12, 0x1, 0x1);
			pr_info("%s: +set Disable skip mode\n", __func__);

			max77705_update_reg(max77705->charger,
						MAX77705_CHG_REG_CNFG_00, 0x09, 0x0F);
			pr_info("%s: +change chg_mode(0x9), vcell(%dmv)\n",
						__func__, vcell);
		} else {
			max77705_update_reg(max77705->charger,
				MAX77705_CHG_REG_CNFG_12, 0x18, 0x18);
			max77705_read_reg(max77705->charger, MAX77705_CHG_REG_CNFG_12, &vchgin);
			pr_info("%s: -set aicl, (0x%02x)\n", __func__, vchgin);

			max77705_update_reg(max77705->charger,
				MAX77705_CHG_REG_CNFG_02, 0x0, 0x3F);
			max77705_read_reg(max77705->charger, MAX77705_CHG_REG_CNFG_02, &chg_curr);
			pr_info("%s: -set charge curr 100mA, (0x%02x)\n", __func__, chg_curr);

			if (chg_mode_changed) {
				chg_mode_changed = false;
				/* Auto skip mode */
				max77705_update_reg(max77705->charger,
					MAX77705_CHG_REG_CNFG_12, 0x0, 0x1);
				pr_info("%s: -set Auto skip mode\n", __func__);

				max77705_update_reg(max77705->charger,
					MAX77705_CHG_REG_CNFG_12, 0x0, 0x20);
				pr_info("%s: -disable CHGINSEL\n", __func__);

				max77705_update_reg(max77705->charger,
					MAX77705_CHG_REG_CNFG_00, 0x4, 0x0F);
				pr_info("%s: -set chg_mode(0x4)\n", __func__);

				max77705_update_reg(max77705->charger,
					MAX77705_CHG_REG_CNFG_12, 0x20, 0x20);
				pr_info("%s: -enable CHGINSEL\n", __func__);

				max77705_update_reg(max77705->charger,
					MAX77705_CHG_REG_CNFG_00, chg_cnfg_00, 0x0F);
				pr_info("%s: -recover chg_mode(0x%x), vcell(%dmv)\n",
					__func__, chg_cnfg_00 & 0x0F, vcell);

				/* Switching Frequency : 1.5MHz */
				max77705_update_reg(max77705->charger,
					MAX77705_CHG_REG_CNFG_08, 0x02, 0x3);

				pr_info("%s: -set Switching Frequency 1.5MHz\n", __func__);
			}
		}
		msleep(500);

		max77705_write_reg(max77705->muic, 0x21, 0xD0);
		max77705_write_reg(max77705->muic, 0x41, 0x00);
		msleep(500);

		max77705_read_reg(max77705->muic, REG_UIC_FW_REV, &max77705->FW_Revision);
		max77705_read_reg(max77705->muic, REG_UIC_FW_MINOR, &max77705->FW_Minor_Revision);
		max77705->FW_Minor_Revision &= MINOR_VERSION_MASK;
		msg_maxim("Start FW updating (%02X.%02X)", max77705->FW_Revision, max77705->FW_Minor_Revision);
		if (max77705->FW_Revision != 0xFF && try_command < 3) {
			try_command++;
			max77705_reset_ic(max77705);
			msleep(1000);
			goto retry;
		}

		if (max77705->FW_Revision != 0xFF) {
			if (++try_count < FW_VERIFY_TRY_COUNT) {
				msg_maxim("the Fail to enter secure mode %d",
						try_count);
				max77705_reset_ic(max77705);
				msleep(1000);
				goto retry;
			} else {
				msg_maxim("the Secure Update Fail!!");
#if defined(CONFIG_USB_HW_PARAM)
				if (o_notify)
					inc_hw_param(o_notify, USB_CCIC_FWUP_ERROR_COUNT);
#endif
				error = -EIO;
				goto out;
			}
		}

		for (offset = FW_HEADER_SIZE;
				offset < fw_bin_len && size != FW_UPDATE_END;) {

			size = __max77705_usbc_fw_update(max77705, &fw_bin[offset]);

			switch (size) {
			case FW_UPDATE_VERIFY_FAIL:
				offset -= FW_CMD_WRITE_SIZE;
				/* FALLTHROUGH */
			case FW_UPDATE_TIMEOUT_FAIL:
				/*
				 * Retry FW updating
				 */
				if (++try_count < FW_VERIFY_TRY_COUNT) {
					msg_maxim("Retry fw write. ret %d, count %d, offset %d",
							size, try_count, offset);
					max77705_reset_ic(max77705);
					msleep(1000);
					goto retry;
				} else {
					msg_maxim("Failed to update FW. ret %d, offset %d",
							size, (offset + size));
#if defined(CONFIG_USB_HW_PARAM)
					if (o_notify)
						inc_hw_param(o_notify, USB_CCIC_FWUP_ERROR_COUNT);
#endif
					error = -EIO;
					goto out;
				}
				break;
			case FW_UPDATE_CMD_FAIL:
			case FW_UPDATE_MAX_LENGTH_FAIL:
				msg_maxim("Failed to update FW. ret %d, offset %d",
						size, (offset + size));
#if defined(CONFIG_USB_HW_PARAM)
				if (o_notify)
					inc_hw_param(o_notify, USB_CCIC_FWUP_ERROR_COUNT);
#endif
				error = -EIO;
				goto out;
			case FW_UPDATE_END: /* 0x00 */
				/* JIG PIN for setting HIGH. */
				if (fct_id == FCT_523Kohm  && !enforce_do)
					max77705_write_jig_on(max77705);
				max77705_read_reg(max77705->muic,
						REG_UIC_FW_REV, &max77705->FW_Revision);
				max77705_read_reg(max77705->muic,
						REG_UIC_FW_MINOR, &max77705->FW_Minor_Revision);
				max77705->FW_Minor_Revision &= MINOR_VERSION_MASK;
				msg_maxim("chip : %02X.%02X, FW : %02X.%02X",
						max77705->FW_Revision, max77705->FW_Minor_Revision,
						fw_header->major, fw_header->minor);
				msg_maxim("Completed");
				break;
			default:
				offset += size;
				break;
			}
			if (offset == fw_bin_len) {
				max77705_reset_ic(max77705);
				max77705_read_reg(max77705->muic,
						REG_UIC_FW_REV, &max77705->FW_Revision);
				max77705_read_reg(max77705->muic,
						REG_UIC_FW_MINOR, &max77705->FW_Minor_Revision);
				max77705->FW_Minor_Revision &= MINOR_VERSION_MASK;
				msg_maxim("chip : %02X.%02X, FW : %02X.%02X",
						max77705->FW_Revision, max77705->FW_Minor_Revision,
						fw_header->major, fw_header->minor);
				msg_maxim("Completed via SYS path");
			}
		}
	} else {
		msg_maxim("Don't need to update!");
		goto out;
	}

	duration = jiffies - duration;
	msg_maxim("Duration : %dms", jiffies_to_msecs(duration));
out:
	if (chg_mode_changed) {
		vcell = max77705_fuelgauge_read_vcell(max77705);
		/* Auto skip mode */
		max77705_update_reg(max77705->charger,
			MAX77705_CHG_REG_CNFG_12, 0x0, 0x1);
		pr_info("%s: -set Auto skip mode\n", __func__);

		max77705_update_reg(max77705->charger,
			MAX77705_CHG_REG_CNFG_12, 0x0, 0x20);
		pr_info("%s: -disable CHGINSEL\n", __func__);

		max77705_update_reg(max77705->charger,
			MAX77705_CHG_REG_CNFG_00, 0x4, 0x0F);
		pr_info("%s: -set chg_mode(0x4)\n", __func__);

		max77705_update_reg(max77705->charger,
			MAX77705_CHG_REG_CNFG_12, 0x20, 0x20);
		pr_info("%s: -enable CHGINSEL\n", __func__);

		max77705_update_reg(max77705->charger,
			MAX77705_CHG_REG_CNFG_00, chg_cnfg_00, 0x0F);
		pr_info("%s: -recover chg_mode(0x%x), vcell(%dmv)\n",
			__func__, chg_cnfg_00 & 0x0F, vcell);

		/* Switching Frequency : 1.5MHz */
		max77705_update_reg(max77705->charger,
			MAX77705_CHG_REG_CNFG_08, 0x02, 0x3);
		pr_info("%s: -set Switching Frequency 1.5MHz\n", __func__);
	}

	if (wpc_en_changed) {
		max77705_wc_control(max77705, true);
	}
	enable_irq(max77705->irq);
	return error;
}
EXPORT_SYMBOL_GPL(max77705_usbc_fw_update);

void max77705_usbc_fw_setting(struct max77705_dev *max77705, int enforce_do)
{
	switch (max77705->pmic_rev) {
	case MAX77705_PASS1:
		pr_info("[MAX77705] Doesn't update the MAX77705_PASS1\n");
		break;
	case MAX77705_PASS2:
		pr_info("[MAX77705] Doesn't update the MAX77705_PASS2\n");
		break;
	case MAX77705_PASS3:
		max77705_usbc_fw_update(max77705, BOOT_FLASH_FW_PASS3,  ARRAY_SIZE(BOOT_FLASH_FW_PASS3), enforce_do);
		break;
	case MAX77705_PASS4:
		max77705_usbc_fw_update(max77705, BOOT_FLASH_FW_PASS4,  ARRAY_SIZE(BOOT_FLASH_FW_PASS4), enforce_do);
		break;
	case MAX77705_PASS5:
		max77705_usbc_fw_update(max77705, BOOT_FLASH_FW_PASS2,  ARRAY_SIZE(BOOT_FLASH_FW_PASS2), enforce_do);
		break;
	default:
		pr_info("[MAX77705] FAIL F/W ON BOOTING TIME and PMIC_REVISION isn't valid\n");
		break;
	};
}
EXPORT_SYMBOL_GPL(max77705_usbc_fw_setting);
#endif

static u8 max77705_revision_check(u8 pmic_id, u8 pmic_rev) {
	int i, logical_id = 0;
	int pmic_arrary = ARRAY_SIZE(max77705_revision);
	for (i = 0; i < pmic_arrary; i++) {
		if (max77705_revision[i].id == pmic_id &&
		    max77705_revision[i].rev  == pmic_rev)
			logical_id = max77705_revision[i].logical_id;
	}
	return logical_id;
}

void max77705_register_pdmsg_func(struct max77705_dev *max77705,
	void (*check_pdmsg)(void *data, u8 pdmsg), void *data)
{
	if (!max77705) {
		pr_err("%s max77705 is null\n", __func__);
		return;
	}

	max77705->check_pdmsg = check_pdmsg;
	max77705->usbc_data = data;
}
EXPORT_SYMBOL_GPL(max77705_register_pdmsg_func);

static int max77705_i2c_probe(struct i2c_client *i2c,
				const struct i2c_device_id *dev_id)
{
	struct max77705_dev *max77705;
	struct max77705_platform_data *pdata = i2c->dev.platform_data;
	int ret = 0;
	u8 pmic_id, pmic_rev = 0;

	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);

	max77705 = kzalloc(sizeof(struct max77705_dev), GFP_KERNEL);
	if (!max77705)
		return -ENOMEM;

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev, sizeof(struct max77705_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			ret = -ENOMEM;
			goto err;
		}

		ret = of_max77705_dt(&i2c->dev, pdata);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to get device of_node\n");
			goto err;
		}

		i2c->dev.platform_data = pdata;
	} else
		pdata = i2c->dev.platform_data;

	max77705->dev = &i2c->dev;
	max77705->i2c = i2c;
	max77705->irq = i2c->irq;
	if (pdata) {
		max77705->pdata = pdata;

		pdata->irq_base = irq_alloc_descs(-1, 0, MAX77705_IRQ_NR, -1);
		if (pdata->irq_base < 0) {
			pr_err("%s:%s irq_alloc_descs Fail! ret(%d)\n",
					MFD_DEV_NAME, __func__, pdata->irq_base);
			ret = -EINVAL;
			goto err;
		} else
			max77705->irq_base = pdata->irq_base;

		max77705->irq_gpio = pdata->irq_gpio;
		max77705->wakeup = pdata->wakeup;
		max77705->blocking_waterevent = pdata->blocking_waterevent;
		max77705->device_product_id = pdata->fw_product_id;
	} else {
		ret = -EINVAL;
		goto err;
	}
	mutex_init(&max77705->i2c_lock);

	i2c_set_clientdata(i2c, max77705);

	if (max77705_read_reg(i2c, MAX77705_PMIC_REG_PMICID1, &pmic_id) < 0) {
		dev_err(max77705->dev, "device not found on this channel (this is not an error)\n");
		ret = -ENODEV;
		goto err_w_lock;
	}
	if (max77705_read_reg(i2c, MAX77705_PMIC_REG_PMICREV, &pmic_rev) < 0) {
		dev_err(max77705->dev, "device not found on this channel (this is not an error)\n");
		ret = -ENODEV;
		goto err_w_lock;
	}

	pr_info("%s:%s pmic_id:%x, pmic_rev:%x\n",
		MFD_DEV_NAME, __func__, pmic_id, pmic_rev);

	max77705->pmic_rev = max77705_revision_check(pmic_id, pmic_rev & 0x7);
	if (max77705->pmic_rev == 0) {
		dev_err(max77705->dev, "Can not find matched revision\n");
		ret = -ENODEV;
		goto err_w_lock;
	}
	max77705->pmic_ver = ((pmic_rev & 0xF8) >> 0x3);

	/* print rev */
	pr_info("%s:%s device found: rev:%x ver:%x\n",
		MFD_DEV_NAME, __func__, max77705->pmic_rev, max77705->pmic_ver);

	/* No active discharge on safeout ldo 1,2 */
	/* max77705_update_reg(i2c, MAX77705_PMIC_REG_SAFEOUT_CTRL, 0x00, 0x30); */
#ifdef CONFIG_CCIC_MAX77705
	init_completion(&max77705->fw_completion);
	max77705->fw_workqueue = create_singlethread_workqueue("fw_update");
	if (max77705->fw_workqueue == NULL)
		return -ENOMEM;
	INIT_WORK(&max77705->fw_work, max77705_usbc_wait_response_q);
#endif
	max77705->muic = i2c_new_dummy(i2c->adapter, I2C_ADDR_MUIC);
	i2c_set_clientdata(max77705->muic, max77705);

	max77705->charger = i2c_new_dummy(i2c->adapter, I2C_ADDR_CHG);
	i2c_set_clientdata(max77705->charger, max77705);

	max77705->fuelgauge = i2c_new_dummy(i2c->adapter, I2C_ADDR_FG);
	i2c_set_clientdata(max77705->fuelgauge, max77705);

#ifdef CONFIG_CCIC_MAX77705
	max77705_usbc_fw_setting(max77705, 0);
#endif

	max77705->debug = i2c_new_dummy(i2c->adapter, I2C_ADDR_DEBUG);
	i2c_set_clientdata(max77705->debug, max77705);

	disable_irq(max77705->irq);
	ret = max77705_irq_init(max77705);

	if (ret < 0)
		goto err_irq_init;

	ret = mfd_add_devices(max77705->dev, -1, max77705_devs,
			ARRAY_SIZE(max77705_devs), NULL, 0, NULL);
	if (ret < 0)
		goto err_mfd;

	device_init_wakeup(max77705->dev, pdata->wakeup);

	return ret;

err_mfd:
	mfd_remove_devices(max77705->dev);
err_irq_init:
	i2c_unregister_device(max77705->muic);
	i2c_unregister_device(max77705->charger);
	i2c_unregister_device(max77705->fuelgauge);
	i2c_unregister_device(max77705->debug);
err_w_lock:
	mutex_destroy(&max77705->i2c_lock);
err:
	kfree(max77705);
	return ret;
}

static int max77705_i2c_remove(struct i2c_client *i2c)
{
	struct max77705_dev *max77705 = i2c_get_clientdata(i2c);

	device_init_wakeup(max77705->dev, 0);
	max77705_irq_exit(max77705);
	mfd_remove_devices(max77705->dev);
	i2c_unregister_device(max77705->muic);
	i2c_unregister_device(max77705->charger);
	i2c_unregister_device(max77705->fuelgauge);
	i2c_unregister_device(max77705->debug);
	kfree(max77705);

	return 0;
}

static const struct i2c_device_id max77705_i2c_id[] = {
	{ MFD_DEV_NAME, TYPE_MAX77705 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max77705_i2c_id);

#if defined(CONFIG_OF)
static const struct of_device_id max77705_i2c_dt_ids[] = {
	{ .compatible = "maxim,max77705" },
	{ },
};
MODULE_DEVICE_TABLE(of, max77705_i2c_dt_ids);
#endif /* CONFIG_OF */

#if defined(CONFIG_PM)
static int max77705_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77705_dev *max77705 = i2c_get_clientdata(i2c);

	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);

	if (device_may_wakeup(dev))
		enable_irq_wake(max77705->irq);

	disable_irq(max77705->irq);

	return 0;
}

static int max77705_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77705_dev *max77705 = i2c_get_clientdata(i2c);

	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);

	if (device_may_wakeup(dev))
		disable_irq_wake(max77705->irq);

	enable_irq(max77705->irq);

	return 0;
}
#else
#define max77705_suspend	NULL
#define max77705_resume		NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_HIBERNATION

#if 0
u8 max77705_dumpaddr_pmic[] = {
#if 0
	MAX77705_LED_REG_IFLASH,
	MAX77705_LED_REG_IFLASH1,
	MAX77705_LED_REG_IFLASH2,
	MAX77705_LED_REG_ITORCH,
	MAX77705_LED_REG_ITORCHTORCHTIMER,
	MAX77705_LED_REG_FLASH_TIMER,
	MAX77705_LED_REG_FLASH_EN,
	MAX77705_LED_REG_MAX_FLASH1,
	MAX77705_LED_REG_MAX_FLASH2,
	MAX77705_LED_REG_VOUT_CNTL,
	MAX77705_LED_REG_VOUT_FLASH,
	MAX77705_LED_REG_VOUT_FLASH1,
	MAX77705_LED_REG_FLASH_INT_STATUS,
#endif
	MAX77705_PMIC_REG_PMICID1,
	MAX77705_PMIC_REG_PMICREV,
	MAX77705_PMIC_REG_MAINCTRL1,
	MAX77705_PMIC_REG_MCONFIG,
};
#endif

u8 max77705_dumpaddr_muic[] = {
	MAX77705_MUIC_REG_INTMASK_MAIN,
	MAX77705_MUIC_REG_INTMASK_BC,
	MAX77705_MUIC_REG_INTMASK_FC,
	MAX77705_MUIC_REG_INTMASK_GP,
	MAX77705_MUIC_REG_STATUS1_BC,
	MAX77705_MUIC_REG_STATUS2_BC,
	MAX77705_MUIC_REG_STATUS_GP,
	MAX77705_MUIC_REG_CONTROL1_BC,
	MAX77705_MUIC_REG_CONTROL2_BC,
	MAX77705_MUIC_REG_CONTROL1,
	MAX77705_MUIC_REG_CONTROL2,
	MAX77705_MUIC_REG_CONTROL3,
	MAX77705_MUIC_REG_CONTROL4,
	MAX77705_MUIC_REG_HVCONTROL1,
	MAX77705_MUIC_REG_HVCONTROL2,
};

#if 0
u8 max77705_dumpaddr_haptic[] = {
	MAX77705_HAPTIC_REG_CONFIG1,
	MAX77705_HAPTIC_REG_CONFIG2,
	MAX77705_HAPTIC_REG_CONFIG_CHNL,
	MAX77705_HAPTIC_REG_CONFG_CYC1,
	MAX77705_HAPTIC_REG_CONFG_CYC2,
	MAX77705_HAPTIC_REG_CONFIG_PER1,
	MAX77705_HAPTIC_REG_CONFIG_PER2,
	MAX77705_HAPTIC_REG_CONFIG_PER3,
	MAX77705_HAPTIC_REG_CONFIG_PER4,
	MAX77705_HAPTIC_REG_CONFIG_DUTY1,
	MAX77705_HAPTIC_REG_CONFIG_DUTY2,
	MAX77705_HAPTIC_REG_CONFIG_PWM1,
	MAX77705_HAPTIC_REG_CONFIG_PWM2,
	MAX77705_HAPTIC_REG_CONFIG_PWM3,
	MAX77705_HAPTIC_REG_CONFIG_PWM4,
};
#endif

u8 max77705_dumpaddr_led[] = {
	MAX77705_RGBLED_REG_LEDEN,
	MAX77705_RGBLED_REG_LED0BRT,
	MAX77705_RGBLED_REG_LED1BRT,
	MAX77705_RGBLED_REG_LED2BRT,
	MAX77705_RGBLED_REG_LED3BRT,
	MAX77705_RGBLED_REG_LEDBLNK,
	MAX77705_RGBLED_REG_LEDRMP,
};

static int max77705_freeze(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77705_dev *max77705 = i2c_get_clientdata(i2c);
	int i;

	for (i = 0; i < ARRAY_SIZE(max77705_dumpaddr_pmic); i++)
		max77705_read_reg(i2c, max77705_dumpaddr_pmic[i],
				&max77705->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77705_dumpaddr_muic); i++)
		max77705_read_reg(i2c, max77705_dumpaddr_muic[i],
				&max77705->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77705_dumpaddr_led); i++)
		max77705_read_reg(i2c, max77705_dumpaddr_led[i],
				&max77705->reg_led_dump[i]);

	disable_irq(max77705->irq);

	return 0;
}

static int max77705_restore(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77705_dev *max77705 = i2c_get_clientdata(i2c);
	int i;

	enable_irq(max77705->irq);

	for (i = 0; i < ARRAY_SIZE(max77705_dumpaddr_pmic); i++)
		max77705_write_reg(i2c, max77705_dumpaddr_pmic[i],
				max77705->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77705_dumpaddr_muic); i++)
		max77705_write_reg(i2c, max77705_dumpaddr_muic[i],
				max77705->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77705_dumpaddr_led); i++)
		max77705_write_reg(i2c, max77705_dumpaddr_led[i],
				max77705->reg_led_dump[i]);

	return 0;
}
#endif

const struct dev_pm_ops max77705_pm = {
	.suspend = max77705_suspend,
	.resume = max77705_resume,
#ifdef CONFIG_HIBERNATION
	.freeze =  max77705_freeze,
	.thaw = max77705_restore,
	.restore = max77705_restore,
#endif
};

static struct i2c_driver max77705_i2c_driver = {
	.driver		= {
		.name	= MFD_DEV_NAME,
		.owner	= THIS_MODULE,
#if defined(CONFIG_PM)
		.pm	= &max77705_pm,
#endif /* CONFIG_PM */
#if defined(CONFIG_OF)
		.of_match_table	= max77705_i2c_dt_ids,
#endif /* CONFIG_OF */
	},
	.probe		= max77705_i2c_probe,
	.remove		= max77705_i2c_remove,
	.id_table	= max77705_i2c_id,
};

static int __init max77705_i2c_init(void)
{
	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);
	return i2c_add_driver(&max77705_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(max77705_i2c_init);

static void __exit max77705_i2c_exit(void)
{
	i2c_del_driver(&max77705_i2c_driver);
}
module_exit(max77705_i2c_exit);

MODULE_DESCRIPTION("MAXIM 77705 multi-function core driver");
MODULE_AUTHOR("Insun Choi <insun77.choi@samsung.com>");
MODULE_LICENSE("GPL");
