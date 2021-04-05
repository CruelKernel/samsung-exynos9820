/*
 *  mfc_s2miw04_charger.c
 *  Samsung MFC IC Charger Driver
 *
 *  Copyright (C) 2016 Samsung Electronics
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

#include "include/charger/mfc_s2miw04_charger.h"
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/ctype.h>
#include <linux/firmware.h>
#include <linux/sec_batt.h>
#include <linux/sec_ext.h>
#include <linux/spu-verify.h>

#define ENABLE 1
#define DISABLE 0
#define CMD_CNT 3
#define MAX_I2C_ERROR_COUNT		30

#define MAX_BUF 255
static u8 ADT_buffer_rdata[MAX_BUF] = {0, };
static u8 adt_readSize;
static bool is_shutdn = false;

#if defined(CONFIG_WIRELESS_CHARGER_HAL_MFC)
extern unsigned int mfc_chip_id_now;
#endif

extern bool sleep_mode;
extern unsigned int lpcharge;
extern bool mfc_fw_update;
extern bool boot_complete;

static char *rx_device_type_str[] = {
	"No Dev",
	"Other Dev",
	"Gear",
	"Phone",
	"Buds",
};

static char *mfc_vout_control_mode_str[] = {
	"Set VOUT Off",
	"Set VOUT NV",
	"Set Vout Rsv",
	"Set Vout HV",
	"Set Vout CV",
	"Set Vout Call",
	"Set Vout 5V",
	"Set Vout 9V",
	"Set Vout 10V",
	"Set Vout 11V",
	"Set Vout 12V",
	"Set Vout 12.5V",
	"Set Vout 5V Step",
	"Set Vout 5.5V Step",
	"Set Vout 9V Step",
	"Set Vout 10V Step",
};

static char *rx_vout_str[] = {
	"Vout 5V",
	"Vout 5.5V",
	"Vout 6V",
	"Vout 7V",
	"Vout 8V",
	"Vout 9V",
	"Vout 10V",
	"Vout 11V",
	"Vout 12V",
	"Vout 12.5V",	
};

static struct device_attribute mfc_attrs[] = {
	MFC_S2MIW04_ATTR(mfc_addr),
	MFC_S2MIW04_ATTR(mfc_size),
	MFC_S2MIW04_ATTR(mfc_data),
	MFC_S2MIW04_ATTR(mfc_packet),	
};

static enum power_supply_property mfc_charger_props[] = {
};

static int mfc_get_firmware_version(struct mfc_charger_data *charger, int firm_mode);
static irqreturn_t mfc_wpc_det_irq_thread(int irq, void *irq_data);
static irqreturn_t mfc_wpc_irq_thread(int irq, void *irq_data);
static int mfc_reg_multi_write_verify(struct i2c_client *client, u16 reg, const u8 * val, int size);

static void mfc_check_i2c_error(struct mfc_charger_data *charger, bool is_error)
{
	charger->i2c_error_count = 
		(charger->wc_w_state && gpio_get_value(charger->pdata->wpc_det) && is_error) ?
		(charger->i2c_error_count + 1) : 0;

	if (charger->i2c_error_count > MAX_I2C_ERROR_COUNT) {
		charger->i2c_error_count = 0;
		queue_delayed_work(charger->wqueue, &charger->wpc_i2c_error_work, 0);
	}
}

static int mfc_reg_read(struct i2c_client *client, u16 reg, u8 *val)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	int ret;
	struct i2c_msg msg[2];
	u8 wbuf[2];
	u8 rbuf[2];

	msg[0].addr = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len = 2;
	msg[0].buf = wbuf;

	wbuf[0] = (reg & 0xFF00) >> 8;
	wbuf[1] = (reg & 0xFF);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = rbuf;

	mutex_lock(&charger->io_lock);
	ret = i2c_transfer(client->adapter, msg, 2);
	mfc_check_i2c_error(charger, (ret < 0));
	mutex_unlock(&charger->io_lock);
	if (ret < 0)
	{
		pr_err("%s: i2c read error, reg: 0x%x, ret: %d (called by %ps)\n",
			__func__, reg, ret, __builtin_return_address(0));
		return ret;
	}
	*val = rbuf[0];

	return ret;
}

static int mfc_reg_multi_read(struct i2c_client *client, u16 reg, u8 *val, int size)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	int ret;
	struct i2c_msg msg[2];
	u8 wbuf[2];

	msg[0].addr = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len = 2;
	msg[0].buf = wbuf;

	wbuf[0] = (reg & 0xFF00) >> 8;
	wbuf[1] = (reg & 0xFF);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = size;
	msg[1].buf = val;

	mutex_lock(&charger->io_lock);
	ret = i2c_transfer(client->adapter, msg, 2);
	mfc_check_i2c_error(charger, (ret < 0));
	mutex_unlock(&charger->io_lock);
	if (ret < 0)
	{
		pr_err("%s: i2c transfer fail", __func__);
		return ret;
	}

	return ret;
}

static int mfc_reg_write(struct i2c_client *client, u16 reg, u8 val)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	int ret;
	unsigned char data[3] = { reg >> 8, reg & 0xff, val };

	//pr_info("%s reg=0x%x, val=0x%x", __func__, reg, val);

	mutex_lock(&charger->io_lock);
	ret = i2c_master_send(client, data, 3);
	mfc_check_i2c_error(charger, (ret < 3));
	mutex_unlock(&charger->io_lock);
	if (ret < 3) {
		pr_err("%s: i2c write error, reg: 0x%x, ret: %d (called by %ps)\n",
				__func__, reg, ret, __builtin_return_address(0));
		return ret < 0 ? ret : -EIO;
	}

	return 0;
}

static int mfc_reg_update(struct i2c_client *client, u16 reg, u8 val, u8 mask)
{
	//val = 0b 0000 0001
	//ms = 0b 1111 1110
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	unsigned char data[3] = { reg >> 8, reg & 0xff, val };
	u8 data2;
	int ret;

	ret = mfc_reg_read(client, reg, &data2);
	if (ret >= 0) {
		u8 old_val = data2 & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		data[2] = new_val;

		mutex_lock(&charger->io_lock);
		ret = i2c_master_send(client, data, 3);
		mfc_check_i2c_error(charger, (ret < 3));
		mutex_unlock(&charger->io_lock);
		if (ret < 3) {
			pr_err("%s: i2c write error, reg: 0x%x, ret: %d\n",
				__func__, reg, ret);
			return ret < 0 ? ret : -EIO;
		}
	}
	mfc_reg_read(client, reg, &data2);

	return ret;
}

#if 0
static void mfc_itoh(u8 *dest, char *hex, int len)
{
	int i = 0;

	while (i < len) {
		snprintf(hex + i*2, len*2, "%x", dest[i]);
		i++;
	}
}

static u8 htoi(const char *hexa)
{
	char ch = 0;
	u8 deci = 0;
	const char *sp = hexa;
	while (*sp) {
		deci *= 16;
		if ('0' <= *sp && *sp <= '9')
			ch = *sp - '0';
		if ('A' <= *sp && *sp <= 'F')
			ch = *sp - 'A' + 10;
		if ('a' <= *sp && *sp <= 'f')
			ch = *sp - 'a' + 10;
		deci += ch;
		sp++;
	}
	return deci;
}

static int mfc_str_to_deci(const char *data, int size, u8 *dest)
{
	int ret = 0;
	int i = 0, str_len = 0;
	char temp_str[3] = {0, };

	if (data == NULL) {
		pr_info("%s: given data is not valid !\n", __func__);
		return -1;
	}

	pr_info("%s : data = %s, size = %d \n", __func__, data, size);

	str_len = size * 2;
	while(i < str_len) {
		strncpy(temp_str, (char *)data + i, 2);
		dest[i / 2] = htoi(temp_str);
		pr_info("%s: 0x%x, %d\n", __func__, dest[i / 2], dest[i / 2]);
		i += 2;
	}

	for(i=0; i < size; i++)
		pr_info("%s: 0x%x, %d\n", __func__, dest[i], dest[i]);

	return ret;
}
#endif

#if defined(CONFIG_WIRELESS_IC_PARAM)
static int mfc_set_wireless_param(u8 chip_id, int fw_ver)
{
	u32 param_data = 0;
	int ret = 0;
#if defined(CONFIG_SEC_PARAM)
	param_data = (chip_id & 0xFF) << 24;
	param_data |= (fw_ver & 0xFFFF) << 8;
	param_data |= (wireless_fw_mode_param & 0xF) << 4;

	pr_info("%s: param data 0x%08X\n", __func__, param_data);
	ret = sec_set_param_u32(wireless_offset, param_data);
	if (ret < 0)
		pr_err("%s: sec_set_param_u32 failed\n", __func__);
#else
	pr_err("%s: config is undefined, not called sec_set_param_u32\n", __func__);
#endif
	return ret;
}

static int mfc_set_wireless_ic_param(struct mfc_charger_data *charger, u8 chip_id, int fw_ver)
{
	u32 param_data = 0;
	int ret = 0;

	if (boot_complete) {
#if defined(CONFIG_SEC_PARAM)
		if (chip_id == 0) {
			pr_info("%s: chip_id(0x%02X) is abnormal, so changed chip_id to (0x%02X, param)\n",
				__func__, chip_id, wireless_chip_id_param);
			chip_id = wireless_chip_id_param;
		}

		if (fw_ver <= 0) {
			pr_info("%s: fw_ver(0x%04X) is abnormal, so changed fw_ver to (0x%04X, param)\n",
				__func__, fw_ver, wireless_fw_ver_param);
			fw_ver = wireless_fw_ver_param;
		}

		if (chip_id == wireless_chip_id_param && fw_ver == wireless_fw_ver_param) {
			pr_info("%s: no changed, chip id(0x%02X), fw_ver(0x%04X)\n",
				__func__, chip_id, fw_ver);
			return 0;
		}

		param_data = (chip_id & 0xFF) << 24;
		param_data |= (fw_ver & 0xFFFF) << 8;
		param_data |= (wireless_fw_mode_param & 0xF) << 4;
		pr_info("%s: param data 0x%08X\n", __func__, param_data);
		ret = sec_set_param_u32(wireless_offset, param_data);
		if (ret < 0) {
			pr_err("%s: sec_set_param_u32 failed\n", __func__);
			goto out;
		} else {
			wireless_chip_id_param = chip_id & 0xFF;
			wireless_fw_ver_param = fw_ver & 0xFFFF;
			pr_info("%s: succeed. chip_id(0x%02X), fw_ver(0x%04X)\n",
				__func__, wireless_chip_id_param, wireless_fw_ver_param);
		}
#else
		pr_err("%s: config is undefined, not called sec_set_param_u32\n", __func__);
#endif		
	} else if (!lpcharge) {
		pr_info("%s: not yet boot_complete\n", __func__);
	}

out:
	return ret;
}
#endif

static int mfc_get_firmware_version(struct mfc_charger_data *charger, int firm_mode)
{
	int version = -1;
	int ret;
	u8 fw_major[2] = {0,};
	u8 fw_minor[2] = {0,};

	pr_info("%s: called by (%ps)\n", __func__, __builtin_return_address(0));
	switch (firm_mode) {
		case MFC_RX_FIRMWARE:
			ret = mfc_reg_read(charger->client, MFC_FW_MAJOR_REV_L_REG, &fw_major[0]);
			ret = mfc_reg_read(charger->client, MFC_FW_MAJOR_REV_H_REG, &fw_major[1]);
			if (ret >= 0) {
				version =  fw_major[0] | (fw_major[1] << 8);
			}
			pr_info("%s rx major firmware version 0x%x\n", __func__, version);

			ret = mfc_reg_read(charger->client, MFC_FW_MINOR_REV_L_REG, &fw_minor[0]);
			ret = mfc_reg_read(charger->client, MFC_FW_MINOR_REV_H_REG, &fw_minor[1]);
			if (ret >= 0) {
				version = fw_minor[0] | (fw_minor[1] << 8);
			}
			pr_info("%s rx minor firmware version 0x%x\n", __func__, version);
			break;
		default:
			pr_err("%s Wrong firmware mode\n", __func__);
			version = -1;
			break;
	}

#if defined(CONFIG_WIRELESS_IC_PARAM)
	if (version > 0 && wireless_fw_ver_param != version) {		
		pr_info("%s: fw_ver (param:0x%04X, version:0x%04X)\n",
			__func__, wireless_fw_ver_param, version);
		mfc_set_wireless_ic_param(charger, charger->chip_id_now, version);
	}
#endif

	return version;
}

static int mfc_get_chip_id(struct mfc_charger_data *charger)
{
	u8 chip_id = 0;
	int ret = 0;

	ret = mfc_reg_read(charger->client, MFC_CHIP_ID_L_REG, &chip_id);
	if (ret >= 0) {
		if (chip_id == 0x04) {
			charger->chip_id = MFC_CHIP_LSI;
			pr_info("%s: LSI CHIP(0x%x)\n", __func__, chip_id);
		} else { /* 0x20 */
			charger->chip_id = MFC_CHIP_IDT;
			pr_info("%s: IDT CHIP(0x%x)\n", __func__, chip_id);
		}
		charger->chip_id_now = chip_id;
	} else
		return -1;
	return charger->chip_id;
}

static int mfc_get_ic_revision(struct mfc_charger_data *charger, int read_mode)
{
	u8 temp;
	int ret;

	pr_info("%s: called by (%ps)\n", __func__, __builtin_return_address(0));

	switch (read_mode) {
		case MFC_IC_REVISION:
			ret = mfc_reg_read(charger->client, MFC_CHIP_REVISION_REG, &temp);

			if (ret >= 0) {
				temp &= MFC_CHIP_REVISION_MASK;
				pr_info("%s ic revision = %d\n", __func__, temp);
				ret =  temp;
			}
			else
				ret = -1;
			break;
		case MFC_IC_FONT:
			ret = mfc_reg_read(charger->client, MFC_CHIP_REVISION_REG, &temp);

			if (ret >= 0) {
				temp &= MFC_CHIP_FONT_MASK;
				pr_info("%s ic font = %d\n", __func__, temp);
				ret =  temp;
			}
			else
				ret = -1;
			break;
		default :
			ret = -1;
			break;
	}
	return ret;
}

static int mfc_get_adc(struct mfc_charger_data *charger, int adc_type)
{
	int ret = 0;
	u8 data[2] = {0,};

	switch (adc_type) {
		case MFC_ADC_VOUT:
			ret = mfc_reg_read(charger->client, MFC_ADC_VOUT_L_REG, &data[0]);
			ret = mfc_reg_read(charger->client, MFC_ADC_VOUT_H_REG, &data[1]);
			if (ret >= 0) {
				ret = (data[0] | (data[1] << 8));
			} else
				ret = -1;
			break;
		case MFC_ADC_VRECT:
			ret = mfc_reg_read(charger->client, MFC_ADC_VRECT_L_REG, &data[0]);
			ret = mfc_reg_read(charger->client, MFC_ADC_VRECT_H_REG, &data[1]);
			if (ret >= 0) {
				ret = (data[0] | (data[1] << 8));
			} else
				ret = -1;
			break;
		case MFC_ADC_RX_IOUT:
			ret = mfc_reg_read(charger->client, MFC_ADC_IOUT_L_REG, &data[0]);
			ret = mfc_reg_read(charger->client, MFC_ADC_IOUT_H_REG, &data[1]);
			if (ret >= 0) {
				ret = (data[0] | (data[1] << 8));
			} else
				ret = -1;
			break;
		case MFC_ADC_DIE_TEMP:
			/* only 4 MSB[3:0] field is used, Celsius */
			ret = mfc_reg_read(charger->client, MFC_ADC_DIE_TEMP_L_REG, &data[0]);
			ret = mfc_reg_read(charger->client, MFC_ADC_DIE_TEMP_H_REG, &data[1]);
			if (ret >= 0) {
				data[1] &= 0x0f;
				ret = (data[0] | (data[1] << 8));
			} else
				ret = -1;
			break;
		case MFC_ADC_OP_FRQ: /* kHz */
			ret = mfc_reg_read(charger->client, MFC_TRX_OP_FREQ_L_REG, &data[0]);
			ret = mfc_reg_read(charger->client, MFC_TRX_OP_FREQ_H_REG, &data[1]);
			if (ret >= 0) {
				ret = (data[0] | (data[1] << 8)) / 10;
			} else
				ret = -1;
			break;
		case MFC_ADC_TX_OP_FRQ:
			ret = mfc_reg_read(charger->client, MFC_TX_MAX_OP_FREQ_L_REG, &data[0]);
			ret = mfc_reg_read(charger->client, MFC_TX_MAX_OP_FREQ_H_REG, &data[1]);
			if (ret >= 0) {
				ret = (int)(60000 / (data[0] | (data[1] << 8)));
			} else
				ret = -1;
			break;
		case MFC_ADC_PING_FRQ:
			ret = mfc_reg_read(charger->client, MFC_RX_PING_FREQ_L_REG, &data[0]);
			ret = mfc_reg_read(charger->client, MFC_RX_PING_FREQ_H_REG, &data[0]);
			if (ret >= 0) {
				ret = (data[0] | (data[1] << 8));
			} else
				ret = -1;
			break;
		case MFC_ADC_TX_VOUT:
			ret = mfc_reg_read(charger->client, MFC_ADC_VOUT_L_REG, &data[0]);
			ret = mfc_reg_read(charger->client, MFC_ADC_VOUT_H_REG, &data[1]);
			if (ret >= 0) {
				ret = (data[0] | (data[1] << 8));
			} else
				ret = -1;
			break;
		case MFC_ADC_TX_IOUT:
			ret = mfc_reg_read(charger->client, MFC_ADC_IOUT_L_REG, &data[0]);
			ret = mfc_reg_read(charger->client, MFC_ADC_IOUT_H_REG, &data[1]);
			if (ret >= 0) {
				ret = (data[0] | (data[1] << 8));
			} else
				ret = -1;
			break;
		default:
			break;
	}

	return ret;
}

static void mfc_set_wpc_en(struct mfc_charger_data *charger, char flag, char on)
{
	union power_supply_propval value = {0, };
	int enable = 0, temp = charger->wpc_en_flag;
	int old_val = 0, new_val = 0;

	mutex_lock(&charger->wpc_en_lock);

	if (on)
		charger->wpc_en_flag |= flag;
	else
		charger->wpc_en_flag &= ~flag;

	if (charger->wpc_en_flag & WPC_EN_FW)
		enable = 1;
	else if (!(charger->wpc_en_flag & WPC_EN_SYSFS) || !(charger->wpc_en_flag & WPC_EN_CCIC))
		enable = 0;
	else if (!(charger->wpc_en_flag & (WPC_EN_CHARGING | WPC_EN_MST | WPC_EN_TX)))
		enable = 0;
	else
		enable = 1;

	if (charger->pdata->wpc_en) {
		old_val = gpio_get_value(charger->pdata->wpc_en);
		if (enable)
			gpio_direction_output(charger->pdata->wpc_en, 0);
		else
			gpio_direction_output(charger->pdata->wpc_en, 1);
		new_val = gpio_get_value(charger->pdata->wpc_en);

		pr_info("%s: before(0x%x), after(0x%x), en(%d), val(%d->%d)\n",
			__func__, temp, charger->wpc_en_flag, enable, old_val, new_val);

		if (old_val != new_val) {
			value.intval = enable;
			psy_do_property("battery", set,
				POWER_SUPPLY_EXT_PROP_WPC_EN, value);
		}
	} else {
		pr_info("%s: there`s no wpc_en\n", __func__);
	}
	mutex_unlock(&charger->wpc_en_lock);
}

static void mfc_set_vout(struct mfc_charger_data *charger, int vout)
{
	u8 data[2] = {0,};

	data[0] = mfc_lsi_vout_val16[vout] & 0xff;
	data[1] = (mfc_lsi_vout_val16[vout] & 0xff00) >> 8;
	mfc_reg_write(charger->client, MFC_VOUT_SET_H_REG, data[1]);
	mfc_reg_write(charger->client, MFC_VOUT_SET_L_REG, data[0]);	
	msleep(100);

	pr_info("%s set vout(%s, 0x%04X) read = %d mV\n", __func__,
		rx_vout_str[vout], (data[0] | (data[1] << 8)), mfc_get_adc(charger, MFC_ADC_VOUT));
	charger->pdata->vout_status = vout;
}

static int mfc_get_vout(struct mfc_charger_data *charger)
{
	u8 data[2] = {0,};
	int ret;

	ret = mfc_reg_read(charger->client, MFC_VOUT_SET_L_REG, &data[0]);
	ret = mfc_reg_read(charger->client, MFC_VOUT_SET_H_REG, &data[1]);

	if (ret < 0) {
		pr_err("%s: fail to read vout. (%d)\n", __func__, ret);
	} else {
		ret = (data[0] | (data[1] << 8));
		pr_info("%s: vout(0x%04x)\n", __func__, ret);
	}

	return ret;
}

static void mfc_uno_on(struct mfc_charger_data *charger, bool on)
{
	union power_supply_propval value = {0, };

	if (charger->wc_tx_enable && on) { /* UNO ON */
		value.intval = SEC_BAT_CHG_MODE_UNO_ONLY;
		psy_do_property(charger->pdata->wired_charger_name, set,
			POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL, value);
		pr_info("%s: ENABLE\n", __func__);
	} else if (on) { /* UNO ON */
		value.intval = 1;
		psy_do_property(charger->pdata->wired_charger_name, set,
			POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL, value);
		pr_info("%s: ENABLE\n", __func__);
	} else { /* UNO OFF */
		value.intval = 0;
		psy_do_property(charger->pdata->wired_charger_name, set,
			POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL, value);

		if (delayed_work_pending(&charger->wpc_tx_op_freq_work)) {
			wake_unlock(&charger->wpc_tx_opfq_lock);
			cancel_delayed_work(&charger->wpc_tx_op_freq_work);
		}
		pr_info("%s: DISABLE\n", __func__);
	}
}

static void mfc_rpp_set(struct mfc_charger_data *charger)
{
	u8 data;
	int ret;

	if (charger->led_cover) {
		pr_info("%s: LED cover exists. RPP 3/4 (0x%x)\n", __func__, charger->pdata->wc_cover_rpp);
		mfc_reg_write(charger->client, MFC_RPP_SCALE_COEF_REG, charger->pdata->wc_cover_rpp);
	} else {
		pr_info("%s: LED cover not exists. RPP 1/2 (0x%x)\n", __func__, charger->pdata->wc_hv_rpp);
		mfc_reg_write(charger->client, MFC_RPP_SCALE_COEF_REG, charger->pdata->wc_hv_rpp);
	}
	msleep(5);
	ret = mfc_reg_read(charger->client, MFC_RPP_SCALE_COEF_REG, &data);
	if (ret < 0) {
		pr_err("%s: fail to read RPP scaling coefficient. (%d)\n", __func__, ret);
	} else
		pr_info("%s: RPP scaling coefficient(0x%x)\n", __func__, data);
}

static void mfc_fod_set(struct mfc_charger_data *charger)
{
	int i = 0;

	pr_info("%s\n", __func__);

	if (charger->pdata->cable_type != SEC_WIRELESS_PAD_NONE) {
		for (i = 0; i < MFC_NUM_FOD_REG; i++)
			mfc_reg_write(charger->client, MFC_WPC_FOD_0A_REG+i, charger->pdata->fod_data[i]);
	}
}

static void mfc_fod_set_cv(struct mfc_charger_data *charger)
{
	int i = 0;

	pr_info("%s\n", __func__);

	if (charger->pdata->cable_type != SEC_WIRELESS_PAD_NONE) {
		for (i = 0; i < MFC_NUM_FOD_REG; i++)
			mfc_reg_write(charger->client, MFC_WPC_FOD_0A_REG+i, charger->pdata->fod_data_cv[i]);
	}
}

static void mfc_fod_set_cs100(struct mfc_charger_data *charger)
{
	int i = 0;

	pr_info("%s\n", __func__);

	if (charger->pdata->cable_type != SEC_WIRELESS_PAD_NONE) {
		for (i = 0; i < MFC_NUM_FOD_REG; i++)
			mfc_reg_write(charger->client, MFC_WPC_FOD_0A_REG+i, 0x7f);
	}
}

static void mfc_set_tx_op_freq(struct mfc_charger_data *charger, unsigned int op_feq)
{
	u8 data[2] = {0,};

	pr_info("%s op feq = %d KHz\n", __func__, op_feq);

	op_feq = (int)(60000 / op_feq); 
	data[0] = op_feq & 0xff;
	data[1] = (op_feq & 0xff00) >> 8;

	mfc_reg_write(charger->client, MFC_TX_MAX_OP_FREQ_L_REG, data[0]);
	mfc_reg_write(charger->client, MFC_TX_MAX_OP_FREQ_H_REG, data[1]);

	msleep(500);
	pr_info("%s op feq = %d KHz\n", __func__, mfc_get_adc(charger, MFC_ADC_TX_OP_FRQ));
}

static void mfc_set_min_duty(struct mfc_charger_data *charger, unsigned int duty)
{
	u8 data = 0;

	data = duty;

	pr_info("%s min duty = %d%%(0x%x)\n", __func__, duty, data);

	mfc_reg_write(charger->client, MFC_TX_MIN_DUTY_SETTING_REG, data);
}

#if 0
static void mfc_fod_set_hero_5v(struct mfc_charger_data *charger)
{
	int i = 0;
	u8 data = 0;
	char str[(MFC_NUM_FOD_REG * 5) + 1] = {0,};

	pr_info("%s\n", __func__);

	if (charger->pdata->fod_hero_5v_data) {
		for (i = 0; i < MFC_NUM_FOD_REG; i++)
			mfc_reg_write(charger->client, MFC_WPC_FOD_0A_REG+i, charger->pdata->fod_hero_5v_data[i]);
		msleep(2);
		for (i = 0; i < MFC_NUM_FOD_REG; i++) {
			mfc_reg_read(charger->client, MFC_WPC_FOD_0A_REG+i, &data);
			sprintf(str + strlen(str), "0x%02X ", data);
		}

		pr_info("%s: HERO 5V FOD(%s)\n", __func__, str);
	}
}


static void mfc_fod_set_dream(struct mfc_charger_data *charger)
{
	int i = 0;
	u8 data = 0;
	char str[(MFC_NUM_FOD_REG * 5) + 1] = {0,};

	pr_info("%s\n", __func__);

	if (charger->pdata->fod_dream_data) {
		for (i = 0; i < MFC_NUM_FOD_REG; i++)
			mfc_reg_write(charger->client, MFC_WPC_FOD_0A_REG+i, charger->pdata->fod_dream_data[i]);
		msleep(2);
		for (i = 0; i < MFC_NUM_FOD_REG; i++) {
			mfc_reg_read(charger->client, MFC_WPC_FOD_0A_REG+i, &data);
			sprintf(str + strlen(str), "0x%02X ", data);
		}

		pr_info("%s: DreamPad FOD(%s)\n", __func__, str);
	}
}
static void mfc_fod_set_dream_cv(struct mfc_charger_data *charger)
{
	int i = 0;
	u8 data = 0;
	char str[(MFC_NUM_FOD_REG * 5) + 1] = {0,};

	pr_info("%s\n", __func__);

	if (charger->pdata->fod_dream_cv_data) {
		for (i = 0; i < MFC_NUM_FOD_REG; i++)
			mfc_reg_write(charger->client, MFC_WPC_FOD_0A_REG+i, charger->pdata->fod_dream_cv_data[i]);
		msleep(2);
		for (i = 0; i < MFC_NUM_FOD_REG; i++) {
			mfc_reg_read(charger->client, MFC_WPC_FOD_0A_REG+i, &data);
			sprintf(str + strlen(str), "0x%02X ", data);
		}

		pr_info("%s: DreamPad CV FOD(%s)\n", __func__, str);
	}
}
#endif

static void mfc_set_cmd_l_reg(struct mfc_charger_data *charger, u8 val, u8 mask)
{
	u8 temp = 0;
	int ret = 0, i = 0;

	do {
		pr_info("%s\n", __func__);
		ret = mfc_reg_update(charger->client, MFC_AP2MFC_CMD_L_REG, val, mask); // command
		if (ret >= 0) {
			msleep(10);
			ret = mfc_reg_read(charger->client, MFC_AP2MFC_CMD_L_REG, &temp); // check out set bit exists
			if(temp != 0)
				pr_info("%s CMD is not clear yet, cnt = %d\n", __func__, i);
			if (ret < 0 || i > 3)
				break;
		}
		i++;
	} while ((temp != 0) && (i < 3));
}

#if 0
static void mfc_set_cmd_h_reg(struct mfc_charger_data *charger, u8 val, u8 mask)
{
	u8 temp = 0;
	int ret = 0, i = 0;

	do {
		pr_info("%s\n", __func__);
		ret = mfc_reg_update(charger->client, MFC_AP2MFC_CMD_H_REG, val, mask); // command
		if (ret >= 0) {
			msleep(250);
			ret = mfc_reg_read(charger->client, MFC_AP2MFC_CMD_H_REG, &temp); // check out set bit exists
			if (ret < 0 || i > 3)
				break;
		}
		i++;
	} while ((temp != 0) && (i < 3));
}
#endif

static void mfc_send_eop(struct mfc_charger_data *charger, int health_mode)
{
	int i = 0;
	int ret = 0;

	pr_info("%s: health_mode(0x%x), cable_type(%d)\n",
		__func__, health_mode, charger->pdata->cable_type);
	switch(health_mode) {
	case POWER_SUPPLY_HEALTH_OVERHEAT:
	case POWER_SUPPLY_HEALTH_OVERHEATLIMIT:
	case POWER_SUPPLY_HEALTH_COLD:
		pr_info("%s: ept-ot\n", __func__);
		if (charger->pdata->cable_type == SEC_WIRELESS_PAD_PMA) {
			for (i = 0; i < CMD_CNT; i++) {
				ret = mfc_reg_write(charger->client, MFC_EPT_REG, MFC_WPC_EPT_END_OF_CHG);
				if (ret >= 0) {
					mfc_set_cmd_l_reg(charger, MFC_CMD_SEND_EOP_MASK, MFC_CMD_SEND_EOP_MASK);
					msleep(250);
				} else
					break;
			}
		} else {
			for (i = 0; i < CMD_CNT; i++) {
				ret = mfc_reg_write(charger->client, MFC_EPT_REG, MFC_WPC_EPT_OVER_TEMP);
				if (ret >= 0) {
					mfc_set_cmd_l_reg(charger, MFC_CMD_SEND_EOP_MASK, MFC_CMD_SEND_EOP_MASK);
					msleep(250);
				} else
					break;
			}
		}
		break;
	case POWER_SUPPLY_HEALTH_UNDERVOLTAGE:
		break;
	default:
		break;
	}
}

static void mfc_send_packet(struct mfc_charger_data *charger, u8 header, u8 rx_data_com, u8 *data_val, int data_size)
{
	int i;

	if (data_size > MFC_WPC_RTX_DATA_SIZE_MAX) {
		pr_info("%s: data_size is over(%d)\n", __func__, data_size);
		data_size = MFC_WPC_RTX_DATA_SIZE_MAX;
	}

	mfc_reg_write(charger->client, MFC_WPC_PPP_HEADER_REG, header);
	mfc_reg_write(charger->client, MFC_WPC_RTX_RX_DATA_COM_REG, rx_data_com);

	for (i = 0; i < data_size; i++) {
		mfc_reg_write(charger->client, MFC_WPC_RTX_RX_DATA_VALUE0_REG + i, data_val[i]);
	}
	mfc_set_cmd_l_reg(charger, MFC_CMD_SEND_TRX_DATA_MASK, MFC_CMD_SEND_TRX_DATA_MASK);
}

static int mfc_send_cs100(struct mfc_charger_data *charger)
{
	int i = 0;
	int ret = 0;

	for (i = 0; i < CMD_CNT; i++) {
		ret = mfc_reg_write(charger->client, MFC_BATTERY_CHG_STATUS_REG, 100);
		if (ret >= 0) {
			mfc_set_cmd_l_reg(charger, MFC_CMD_SEND_CHG_STS_MASK, MFC_CMD_SEND_CHG_STS_MASK);
			msleep(250);
			ret = 1;
		} else {
			ret = -1;
			break;
		}
	}
	return ret;
}

static void mfc_send_command(struct mfc_charger_data *charger, int cmd_mode)
{
	u8 data_val[4];
	u8 cmd = 0;
	u8 i;

	switch (cmd_mode) {
	case MFC_AFC_CONF_5V:
		pr_info("%s: WPC/PMA set 5V\n", __func__);
		cmd = WPC_COM_AFC_SET;
		data_val[0] = 0x05; /* Value for WPC AFC_SET 5V */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		msleep(120);

		charger->vout_mode = WIRELESS_VOUT_5V;
		cancel_delayed_work(&charger->wpc_vout_mode_work);
		wake_lock(&charger->wpc_vout_mode_lock);
		queue_delayed_work(charger->wqueue,
			&charger->wpc_vout_mode_work, 0);
		pr_info("%s vout read = %d\n", __func__,  mfc_get_adc(charger, MFC_ADC_VOUT));

		mfc_reg_read(charger->client, MFC_WPC_RTX_RX_DATA_COM_REG, &data_val[0]);
		mfc_reg_read(charger->client, MFC_WPC_RTX_RX_DATA_VALUE0_REG, &data_val[0]);
		mfc_reg_read(charger->client, MFC_AP2MFC_CMD_L_REG, &data_val[0]);
		break;
	case MFC_AFC_CONF_10V:
		pr_info("%s: WPC set 10V\n", __func__);
		//trigger 10V vout work after 8sec
		wake_lock(&charger->wpc_afc_vout_lock);
#if defined(CONFIG_SEC_FACTORY)
		queue_delayed_work(charger->wqueue, &charger->wpc_afc_vout_work, msecs_to_jiffies(3000));
#else
		if (sleep_mode && (charger->non_sleep_mode_cnt >= 2))
			queue_delayed_work(charger->wqueue, &charger->wpc_afc_vout_work, msecs_to_jiffies(3000));
		else
			queue_delayed_work(charger->wqueue, &charger->wpc_afc_vout_work, msecs_to_jiffies(8000));
#endif
		break;
	case MFC_AFC_CONF_5V_TX:
		for (i = 0; i < CMD_CNT; i++) {
			cmd = WPC_COM_AFC_SET;
			data_val[0] = RX_DATA_VAL2_5V; /* Value for WPC AFC_SET 5V */
			pr_info("%s set 5V to TX, cnt = %d \n", __func__, i);
			mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
			mfc_reg_read(charger->client, MFC_WPC_RTX_RX_DATA_COM_REG, &data_val[0]);
			mfc_reg_read(charger->client, MFC_WPC_RTX_RX_DATA_VALUE0_REG, &data_val[0]);
			mfc_reg_read(charger->client, MFC_AP2MFC_CMD_L_REG, &data_val[0]);
			msleep(100);
		}
		break;
	case MFC_AFC_CONF_10V_TX:
		for (i = 0; i < CMD_CNT; i++) {
			cmd = WPC_COM_AFC_SET;
			data_val[0] = RX_DATA_VAL2_10V; /* Value for WPC AFC_SET 10V */
			pr_info("%s set 10V to TX, cnt = %d \n", __func__, i);
			mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
			mfc_reg_read(charger->client, MFC_WPC_RTX_RX_DATA_COM_REG, &data_val[0]);
			mfc_reg_read(charger->client, MFC_WPC_RTX_RX_DATA_VALUE0_REG, &data_val[0]);
			mfc_reg_read(charger->client, MFC_AP2MFC_CMD_L_REG, &data_val[0]);
			msleep(100);
		}
		break;
	case MFC_AFC_CONF_12V_TX:
		for (i = 0; i < CMD_CNT; i++) {
			cmd = WPC_COM_AFC_SET;
			data_val[0] = RX_DATA_VAL2_12V; /* Value for WPC AFC_SET 12V */
			pr_info("%s set 12V to TX, cnt = %d \n", __func__, i);
			mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
			mfc_reg_read(charger->client, MFC_WPC_RTX_RX_DATA_COM_REG, &data_val[0]);
			mfc_reg_read(charger->client, MFC_WPC_RTX_RX_DATA_VALUE0_REG, &data_val[0]);
			mfc_reg_read(charger->client, MFC_AP2MFC_CMD_L_REG, &data_val[0]);
			msleep(100);
		}
		break;
	case MFC_AFC_CONF_12_5V_TX:
		for (i = 0; i < CMD_CNT; i++) {
			cmd = WPC_COM_AFC_SET;
			data_val[0] = RX_DATA_VAL2_12_5V; /* Value for WPC AFC_SET 12V */
			pr_info("%s set 12.5V to TX, cnt = %d \n", __func__, i);
			mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
			mfc_reg_read(charger->client, MFC_WPC_RTX_RX_DATA_COM_REG, &data_val[0]);
			mfc_reg_read(charger->client, MFC_WPC_RTX_RX_DATA_VALUE0_REG, &data_val[0]);
			mfc_reg_read(charger->client, MFC_AP2MFC_CMD_L_REG, &data_val[0]);
			msleep(100);
		}
		break;		
	case MFC_AFC_CONF_20V_TX:
		for (i = 0; i < CMD_CNT; i++) {
			cmd = WPC_COM_AFC_SET;
			data_val[0] = RX_DATA_VAL2_20V; /* Value for WPC AFC_SET 20V */
			pr_info("%s set 20V to TX, cnt = %d \n", __func__, i);
			mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
			mfc_reg_read(charger->client, MFC_WPC_RTX_RX_DATA_COM_REG, &data_val[0]);
			mfc_reg_read(charger->client, MFC_WPC_RTX_RX_DATA_VALUE0_REG, &data_val[0]);
			mfc_reg_read(charger->client, MFC_AP2MFC_CMD_L_REG, &data_val[0]);
			msleep(100);
		}
		break;		
	case MFC_LED_CONTROL_ON:
		pr_info("%s led on\n", __func__);
		cmd = WPC_COM_LED_CONTROL;
		data_val[0] = 0x00; /* Value for WPC LED ON */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_LED_CONTROL_OFF:
		pr_info("%s led off\n", __func__);
		cmd = WPC_COM_LED_CONTROL;
		data_val[0] = 0xff; /* Value for WPC LED OFF */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_LED_CONTROL_DIMMING:
		pr_info("%s led dimming\n", __func__);
		cmd = WPC_COM_LED_CONTROL;
		data_val[0] = 0x55; /* Value for WPC LED DIMMING */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_FAN_CONTROL_ON:
		pr_info("%s fan on\n", __func__);
		cmd = WPC_COM_COOLING_CTRL;
		data_val[0] = 0x00; /* Value for WPC FAN ON */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_FAN_CONTROL_OFF:
		pr_info("%s fan off\n", __func__);
		cmd = WPC_COM_COOLING_CTRL;
		data_val[0] = 0xff; /* Value for WPC FAN OFF */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_REQUEST_AFC_TX:
		pr_info("%s request afc tx, cable(0x%x)\n", __func__, charger->pdata->cable_type);
		cmd = WPC_COM_REQ_AFC_TX;
		data_val[0] = 0x00; /* Value for WPC Request AFC_TX */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_REQUEST_TX_ID:
		pr_info("%s request TX ID\n", __func__);
		cmd = WPC_COM_TX_ID;
		data_val[0] = 0x00; /* Value for WPC TX ID */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_DISABLE_TX:
		pr_info("%s Disable TX\n", __func__);
		cmd = WPC_COM_DISABLE_TX;
		data_val[0] = 0x55; /* Value for Disable TX */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_PHM_ON:
		pr_info("%s Enter PHM\n", __func__);
		cmd = WPC_COM_ENTER_PHM;
		data_val[0] = 0x01; /* Enter PHM */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	default:
		break;
	}
}

static void mfc_send_fsk(struct mfc_charger_data *charger, u8 header, u8 tx_data_com, u8 *data_val, int data_size)
{
	int i, j;

	if (data_size > MFC_WPC_RTX_DATA_SIZE_MAX) {
		pr_info("%s: data_size is over(%d)\n", __func__, data_size);
		data_size = MFC_WPC_RTX_DATA_SIZE_MAX;
	}

	for(j = 0; j < 3; j++) {
		mfc_reg_write(charger->client, MFC_WPC_PPP_HEADER_REG, header);
		mfc_reg_write(charger->client, MFC_WPC_RTX_TX_DATA_COM_REG, tx_data_com);
		
		for (i = 0; i < data_size; i++) {
			mfc_reg_write(charger->client, MFC_WPC_RTX_TX_DATA_VALUE0_REG + i, data_val[i]);
		}
		mfc_set_cmd_l_reg(charger, MFC_CMD_SEND_TRX_DATA_MASK, MFC_CMD_SEND_TRX_DATA_MASK);
		msleep(250);
	}
}

static void mfc_led_control(struct mfc_charger_data *charger, int state)
{
	int i = 0;

	for (i = 0; i < CMD_CNT; i++)
		mfc_send_command(charger, state);
}

static void mfc_fan_control(struct mfc_charger_data *charger, bool on)
{
	int i = 0;

	if (on) {
		for (i = 0; i < CMD_CNT - 1; i++)
			mfc_send_command(charger, MFC_FAN_CONTROL_ON);
	} else {
		for (i = 0; i < CMD_CNT - 1; i++)
			mfc_send_command(charger, MFC_FAN_CONTROL_OFF);
	}
}

static void mfc_set_vrect_adjust(struct mfc_charger_data *charger, int set)
{
	int i = 0;

	switch (set) {
	case MFC_HEADROOM_0:
		for (i = 0; i < 6; i++) {
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x0);
			msleep(50);
		}
		break;
	case MFC_HEADROOM_1:
		for (i = 0; i < 6; i++) {
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x36);
			msleep(50);
		}
		break;
	case MFC_HEADROOM_2:
		for (i = 0; i < 6; i++) {
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x61);
			msleep(50);
		}
		break;
	case MFC_HEADROOM_3:
		for (i = 0; i < 6; i++) {
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x7f);
			msleep(50);
		}
		break;
	case MFC_HEADROOM_4:
		for (i = 0; i < 6; i++) {
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x06);
			msleep(50);
		}
		break;
	case MFC_HEADROOM_5:
		for (i = 0; i < 6; i++) {
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x10);
			msleep(50);
		}
		break;
	default:
		pr_info("%s no headroom mode\n", __func__);
		break;
	}
}

static bool is_sleep_mode_active(int pad_id)
{
	if ((pad_id >= 0x17 && pad_id <= 0x1f) ||
		(pad_id >= 0x25 && pad_id <= 0x2f) ||
		(pad_id >= 0x34 && pad_id <= 0x3f) ||
		(pad_id >= 0x46 && pad_id <= 0x4f) ||
		(pad_id >= 0x90 && pad_id <= 0x9f) ||
		(pad_id >= 0xa1 && pad_id <= 0xbf) ||
		(pad_id >= 0xc0 && pad_id <= 0xef)) {
		pr_info("%s: this pad applies auto mode\n", __func__);
		return 1;
	} else {
	pr_info("%s: this pad applies no auto mode\n", __func__);
		return 0;
	}
}

static void mfc_mis_align(struct mfc_charger_data *charger)
{
	pr_info("%s: Reset M0\n", __func__);
	if (charger->pdata->cable_type == SEC_WIRELESS_PAD_WPC_HV ||
		charger->pdata->cable_type == SEC_WIRELESS_PAD_PMA)
		/* reset MCU of MFC IC */
		mfc_set_cmd_l_reg(charger, MFC_CMD_MCU_RESET_MASK, MFC_CMD_MCU_RESET_MASK);
}

static bool mfc_tx_function_check(struct mfc_charger_data *charger)
{
	u8 reg_f2, reg_f4;

	mfc_reg_read(charger->client, MFC_TX_RXID1_READ_REG, &reg_f2);
	mfc_reg_read(charger->client, MFC_TX_RXID3_READ_REG, &reg_f4);

	pr_info("@Tx_Mode %s: 0x%x 0x%x\n", __func__, reg_f2, reg_f4);

	if ((reg_f2 == 0x42) && (reg_f4 == 0x64))
		return true;
	else
		return false;
}

static void mfc_set_tx_ioffset(struct mfc_charger_data *charger, int ioffset)
{
	u8 ioffset_data_l = 0, ioffset_data_h = 0;

	ioffset_data_l = 0xFF & ioffset;
	ioffset_data_h = 0xFF & (ioffset >> 8);

	mfc_reg_write(charger->client, MFC_TX_IUNO_OFFSET_L_REG, ioffset_data_l);
	mfc_reg_write(charger->client, MFC_TX_IUNO_OFFSET_H_REG, ioffset_data_h);

	pr_info("@Tx_Mode %s: Tx Iout set %d(0x%x 0x%x)\n", __func__, ioffset, ioffset_data_h, ioffset_data_l);


}

static void mfc_set_tx_iout(struct mfc_charger_data *charger, unsigned int iout)
{
	u8 iout_data_l = 0, iout_data_h = 0;

	iout_data_l = 0xFF & iout;
	iout_data_h = 0xFF & (iout >> 8);

	mfc_reg_write(charger->client, MFC_TX_IUNO_LIMIT_L_REG, iout_data_l);
	mfc_reg_write(charger->client, MFC_TX_IUNO_LIMIT_H_REG, iout_data_h);

	pr_info("@Tx_Mode %s: Tx Iout set %d(0x%x 0x%x)\n", __func__, iout, iout_data_h, iout_data_l);

	if (iout == 1100)
		mfc_set_tx_ioffset(charger, 100);
	else
		mfc_set_tx_ioffset(charger, 150);
}

static void mfc_print_buffer(struct mfc_charger_data *charger, u8 * buffer, u8 size)
{
	u8 i;

	for (i = 0; i < size; i++)
		pr_info("0x%x", buffer[i]);

	pr_info("\n");
}

static int mfc_auth_adt_read(struct mfc_charger_data *charger, u8 * readData)
{
	int buffAddr = MFC_ADT_BUFFER_ADT_PARAM_REG;
	union power_supply_propval value = {0, };
	const int sendsz = 16;	
	int i = 0, size = 0;
	int ret = 0;

	mfc_reg_read(charger->client, MFC_ADT_BUFFER_ADT_MSG_SIZE_REG, &adt_readSize);
	pr_info("%s %s read size = %d byte\n", WC_AUTH_MSG, __func__, adt_readSize);
	if (adt_readSize <= 0) {
		pr_err("%s %s : failed to read adt data size\n", WC_AUTH_MSG, __func__);
		return -EINVAL;
	}
	size = adt_readSize;
	if ((buffAddr + adt_readSize) - 1 > MFC_ADT_BUFFER_ADT_PARAM_MAX_REG) {
		pr_err("%s %s : failed to read adt stream, too large data\n", WC_AUTH_MSG, __func__);
		return -EINVAL;
	}

	if (size <= 2) {
		pr_err("%s %s : data from mcu has invalid size \n", WC_AUTH_MSG, __func__);
		//charger->adt_transfer_status = WIRELESS_AUTH_FAIL;
		/* notify auth service to send TX PAD a request key */
		value.intval = WIRELESS_AUTH_FAIL;
		psy_do_property("wireless", set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);
		return -EINVAL;
	}

	while(size > sendsz) {
		ret = mfc_reg_multi_read(charger->client, buffAddr, readData + i, sendsz);
		if (ret < 0) {
			pr_err("%s %s : failed to read adt stream (%d, buff %d)\n", WC_AUTH_MSG, __func__, ret, buffAddr);
			return ret;
		}
		i += sendsz;
		buffAddr += sendsz;
		size = size - sendsz;
		pr_info("%s %s 0x%x %d %d\n", WC_AUTH_MSG, __func__, i, buffAddr, size);
	}
	if(size > 0) {
		ret = mfc_reg_multi_read(charger->client, buffAddr, readData + i, size);
		if (ret < 0) {
			pr_err("%s %s : failed to read adt stream (%d, buff %d)\n", WC_AUTH_MSG, __func__, ret, buffAddr);
			return ret;
		}
	}
	mfc_print_buffer(charger, readData, adt_readSize);

	value.intval = WIRELESS_AUTH_RECEIVED;
	psy_do_property("wireless", set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);

	charger->adt_transfer_status = WIRELESS_AUTH_RECEIVED;

	pr_info("%s %s : succeeded to read ADT\n", WC_AUTH_MSG, __func__);

	return 0;
}

static int mfc_auth_adt_write(struct mfc_charger_data *charger, u8 * srcData, u8 srcSize)
{
	int buffAddr = MFC_ADT_BUFFER_ADT_PARAM_REG;
	u8 wdata = 0;
	u8 sBuf[144] = {0,};
	int ret;
	u8 i;

	pr_info("%s %s : start to write ADT\n", WC_AUTH_MSG, __func__);

	if ((buffAddr + srcSize) - 1 > MFC_ADT_BUFFER_ADT_PARAM_MAX_REG) {
		pr_err("%s %s : failed to write adt stream, too large data.\n", WC_AUTH_MSG, __func__);
		return -EINVAL;
	}

	wdata = 0x08; /* Authentication purpose */
	mfc_reg_write(charger->client, MFC_ADT_BUFFER_ADT_TYPE_REG, wdata);

	wdata = srcSize; /* need to check */
	mfc_reg_write(charger->client, MFC_ADT_BUFFER_ADT_MSG_SIZE_REG, wdata);

	buffAddr = MFC_ADT_BUFFER_ADT_PARAM_REG;
	for (i = 0; i < srcSize; i+=128) { //write each 128byte
		int restSize = srcSize - i;
		if (restSize < 128) {
			memcpy(sBuf, srcData + i, restSize);
			ret = mfc_reg_multi_write_verify(charger->client, buffAddr, sBuf, restSize);
			if (ret < 0) {
				pr_err("%s %s : failed to write adt stream (%d, buff %d)\n", WC_AUTH_MSG, __func__, ret, buffAddr);
				return ret;
			}
			break;
		} else {
			memcpy(sBuf, srcData + i, 128);
			ret = mfc_reg_multi_write_verify(charger->client, buffAddr, sBuf, 128);
			if (ret < 0) {
				pr_err("%s %s : failed to write adt stream (%d, buff %d)\n", WC_AUTH_MSG, __func__, ret, buffAddr);;
				return ret;
			}
			buffAddr += 128;
		}

		if (buffAddr > MFC_ADT_BUFFER_ADT_PARAM_MAX_REG - 128)
			break;
	}
	pr_info("%s %s : succeeded to write ADT\n", WC_AUTH_MSG, __func__);
	return 0;
}

static void mfc_auth_adt_send(struct mfc_charger_data *charger, u8 * srcData, int srcSize)
{
	u8 temp;
	int ret = 0;
	u8 irq_src[2];

	mfc_reg_read(charger->client, MFC_INT_A_L_REG, &irq_src[0]);
	mfc_reg_read(charger->client, MFC_INT_A_H_REG, &irq_src[1]);

	charger->adt_transfer_status = WIRELESS_AUTH_SENT;
	ret = mfc_auth_adt_write(charger, srcData, srcSize); /* write buff fw datas to send fw datas to tx */ 

	mfc_reg_read(charger->client, MFC_AP2MFC_CMD_H_REG, &temp); // check out set bit exists
	pr_info("%s %s: MFC_CMD_H_REG(0x%x)\n", WC_AUTH_MSG, __func__, temp);
	temp |= 0x02;
	pr_info("%s %s: MFC_CMD_H_REG(0x%x)\n", WC_AUTH_MSG, __func__, temp);
	mfc_reg_write(charger->client, MFC_AP2MFC_CMD_H_REG, temp);

	mfc_reg_read(charger->client, MFC_AP2MFC_CMD_H_REG, &temp); // check out set bit exists
	pr_info("%s %s: MFC_CMD_H_REG(0x%x)\n", WC_AUTH_MSG, __func__, temp);

	mfc_reg_update(charger->client, MFC_INT_A_ENABLE_H_REG,
					MFC_STAT_H_ADT_SENT_MASK, MFC_STAT_H_ADT_SENT_MASK);

	/* clear intterupt */
	mfc_reg_write(charger->client, MFC_INT_A_CLEAR_L_REG, irq_src[0]); // clear int
	mfc_reg_write(charger->client, MFC_INT_A_CLEAR_H_REG, irq_src[1]); // clear int
	mfc_set_cmd_l_reg(charger, 0x20, MFC_CMD_CLEAR_INT_MASK); // command	
}

/* uno on/off control function */
static void mfc_set_tx_power(struct mfc_charger_data *charger, bool on)
{
	union power_supply_propval value = {0, };

	charger->wc_tx_enable = on;

	if (on) {
		pr_info("@Tx_Mode %s: Turn On TX Power\n", __func__);
		mfc_uno_on(charger, true);
		msleep(200);

		charger->pdata->otp_firmware_ver = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);

		cancel_delayed_work(&charger->wpc_tx_op_freq_work);
		wake_lock(&charger->wpc_tx_opfq_lock);
		queue_delayed_work(charger->wqueue,
			&charger->wpc_tx_op_freq_work, msecs_to_jiffies(5000));
		charger->wc_rx_fod = false;
		pr_info("%s : tx op freq = %dKhz\n", __func__, mfc_get_adc(charger, MFC_ADC_TX_OP_FRQ));
		mfc_set_min_duty(charger, 30);
	} else {
		pr_info("@Tx_Mode %s: Turn Off TX Power, and reset UNO config\n", __func__);

		mfc_uno_on(charger, false);

		value.intval = WC_TX_VOUT_5_0V;
		psy_do_property(charger->pdata->wired_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);

		charger->wc_rx_connected = false;
		charger->gear_start_time = 0;

		cancel_delayed_work(&charger->wpc_rx_det_work);
		cancel_delayed_work(&charger->wpc_tx_isr_work);
		cancel_delayed_work(&charger->wpc_tx_phm_work);
		alarm_cancel(&charger->phm_alarm);
		wake_unlock(&charger->wpc_tx_phm_lock);
		wake_unlock(&charger->wpc_rx_det_lock);
		wake_unlock(&charger->wpc_tx_wake_lock);

		charger->tx_status = SEC_TX_OFF;
	}
}

/* determine rx connection status with tx sharing mode */
static void mfc_tx_check_rx_connection(struct mfc_charger_data *charger, bool enable)
{
	union power_supply_propval value = {0, };

	if (!charger->wc_tx_enable)
		return;

	if(enable) {
		pr_info("@Tx_Mode %s : Rx(%s) connected\n", __func__, mfc_tx_function_check(charger) ? "Samsung" : "Other");

		cancel_delayed_work(&charger->wpc_rx_det_work);
		wake_lock(&charger->wpc_rx_det_lock);
		queue_delayed_work(charger->wqueue,
			&charger->wpc_rx_det_work, msecs_to_jiffies(1000));
		charger->wc_rx_connected = true;
	} else {
		pr_info("@Tx_Mode %s : Rx disconnected\n", __func__);
		charger->wc_rx_connected = false;
		charger->wc_rx_fod = false;
		charger->gear_start_time = 0;
		charger->wc_rx_type = NO_DEV;

		if (delayed_work_pending(&charger->wpc_rx_det_work)) {
			wake_unlock(&charger->wpc_rx_det_lock);
			cancel_delayed_work(&charger->wpc_rx_det_work);
		}

		if (delayed_work_pending(&charger->wpc_tx_op_freq_work)) {
			wake_unlock(&charger->wpc_tx_opfq_lock);
			cancel_delayed_work(&charger->wpc_tx_op_freq_work);
		}
	}

	/* set rx connection condition */
	value.intval = charger->wc_rx_connected;
	psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONNECTED, value);
}

/* determine rx connection status with tx sharing mode,
	this TX device has MFC_INTA_H_TRX_DATA_RECEIVED_MASK irq and RX device is connected HV wired cable */
static void mfc_tx_handle_rx_packet(struct mfc_charger_data *charger)
{
	u8 cmd_data = 0, val_data = 0, val2_data = 0;
	union power_supply_propval value = {0, };

	mfc_reg_read(charger->client, MFC_WPC_TRX_DATA_COM_REG, &cmd_data);
	mfc_reg_read(charger->client, MFC_WPC_TRX_DATA_VALUE0_REG, &val_data);
	mfc_reg_read(charger->client, MFC_WPC_TRX_DATA_VALUE1_REG, &val2_data);
	charger->pdata->trx_data_cmd = cmd_data;
	charger->pdata->trx_data_val = val_data;

	pr_info("@Tx_Mode %s: CMD : 0x%x, DATA : 0x%x, DATA2 : 0x%x\n",
		__func__, cmd_data, val_data, val2_data);

	/* When RX device has got a AFC TA, this TX device should turn off TX power sharing(uno) */
	if (cmd_data == MFC_HEADER_AFC_CONF) {
		switch (val_data) {
		case WPC_COM_DISABLE_TX:
			if(val2_data == 0xFF) {
				pr_info("@Tx_Mode %s: Gear send TX off packet by Misalign or darkzone issue\n", __func__);
				value.intval = BATT_TX_EVENT_WIRELESS_TX_MISALIGN;
				psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);
			} else if (val2_data == 0x55) {
				pr_info("@Tx_Mode %s: this TX device should turn off TX power sharing(uno) since RX device has a AFC TA\n", __func__);
				value.intval = BATT_TX_EVENT_WIRELESS_RX_CHG_SWITCH;
				psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);
			}
			break;
		case WPC_COM_ENTER_PHM:
			if (val2_data == 0x01) {
				if(charger->wc_rx_type == SS_GEAR) {
					/* start 3min alarm timer */
					pr_info("%s: Start PHM alarm by 3min\n", __func__);
					alarm_start(&charger->phm_alarm,
			    		ktime_add(ktime_get_boottime(), ktime_set(180, 0)));
				} else {
					pr_info("%s: TX entered PHM but no 3min timer\n", __func__);
				}
			}
			break;
		case WPC_COM_RX_ID:
			if (val2_data >= 0x70 && val2_data <= 0x8f) {
				charger->wc_rx_type = SS_BUDS;
				pr_info("@Tx_Mode %s : Buds connected\n", __func__);

				value.intval = charger->wc_rx_type;
				psy_do_property("wireless", set,
						POWER_SUPPLY_EXT_PROP_WIRELESS_RX_TYPE, value);
			}
			break;
		default:
			pr_info("@Tx_Mode %s: unsupport : 0x%x", __func__, val_data);
			break;
		}
	} else if (cmd_data == MFC_HEADER_END_CHARGE_STATUS) {
		if (val_data == 0x64) {
			pr_info("%s: CS100 Received, TX power off\n", __func__);
			value.intval = BATT_TX_EVENT_WIRELESS_RX_CS100;
			psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);
		}
	}
}

static int datacmp(const char *cs, const char *ct, int count)
{
	unsigned char c1, c2;

	while (count) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2) {
			pr_err("%s, cnt %d\n", __func__, count);
			return c1 < c2 ? -1 : 1;
		}
		count--;
	}
	return 0;
}

static int mfc_reg_multi_write_verify(struct i2c_client *client, u16 reg, const u8 * val, int size)
{
	int ret = 0;
	const int sendsz = 16;
	int cnt = 0;
	int retry_cnt = 0;
	unsigned char data[sendsz+2];
	u8 rdata[sendsz+2];

//	dev_dbg(&client->dev, "%s: size: 0x%x\n", __func__, size);
	while(size > sendsz) {
		data[0] = (reg+cnt) >>8;
		data[1] = (reg+cnt) & 0xff;
		memcpy(data+2, val+cnt, sendsz);
//		dev_dbg(&client->dev, "%s: addr: 0x%x, cnt: 0x%x\n", __func__, reg+cnt, cnt);
		ret = i2c_master_send(client, data, sendsz+2);
		if (ret < sendsz+2) {
			pr_err("%s: i2c write error, reg: 0x%x\n", __func__, reg);
			return ret < 0 ? ret : -EIO;
		}
		if (mfc_reg_multi_read(client, reg+cnt, rdata, sendsz) < 0) {
			pr_err("%s, read failed(%d)\n", __func__, reg+cnt);
			return -1;
		}
		if (datacmp(val+cnt, rdata, sendsz)) {
			pr_err("%s, data is not matched. retry(%d)\n", __func__, retry_cnt);
			retry_cnt++;
			if (retry_cnt > 4) {
				pr_err("%s, data is not matched. write failed\n", __func__);
				retry_cnt = 0;
				return -1;
			}
			continue;
		}
//		pr_debug("%s, data is matched!\n", __func__);
		cnt += sendsz;
		size -= sendsz;
		retry_cnt = 0;
	}
	while (size > 0) {
		data[0] = (reg+cnt) >>8;
		data[1] = (reg+cnt) & 0xff;
		memcpy(data+2, val+cnt, size);
//		dev_dbg(&client->dev, "%s: addr: 0x%x, cnt: 0x%x, size: 0x%x\n", __func__, reg+cnt, cnt, size);
		ret = i2c_master_send(client, data, size+2);
		if (ret < size+2) {
			pr_err("%s: i2c write error, reg: 0x%x\n", __func__, reg);
			return ret < 0 ? ret : -EIO;
		}
		if (mfc_reg_multi_read(client, reg+cnt, rdata, size) < 0) {
			pr_err("%s, read failed(%d)\n", __func__, reg+cnt);
			return -1;
		}
		if (datacmp(val+cnt, rdata, size)) {
			pr_err("%s, data is not matched. retry(%d)\n", __func__, retry_cnt);
			retry_cnt++;
			if (retry_cnt > 4) {
				pr_err("%s, data is not matched. write failed\n", __func__);
				retry_cnt = 0;
				return -1;
			}
			continue;
		}
		size = 0;
		pr_err("%s, data is matched!\n", __func__);
	}
	return ret;
}

static int mfc_reg_multi_write(struct i2c_client *client, u16 reg, const u8 * val, int size)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	int ret = 0;
	const int sendsz = 16;
	unsigned char data[sendsz+2];
	int cnt = 0;

	pr_err("%s: size: 0x%x\n",
				__func__, size);
	while(size > sendsz) {
		data[0] = (reg+cnt) >>8;
		data[1] = (reg+cnt) & 0xff;
		memcpy(data+2, val+cnt, sendsz);
		mutex_lock(&charger->io_lock);
		ret = i2c_master_send(client, data, sendsz+2);
		mutex_unlock(&charger->io_lock);
		if (ret < sendsz+2) {
			pr_err("%s: i2c write error, reg: 0x%x\n",
					__func__, reg);
			return ret < 0 ? ret : -EIO;
		}
		cnt = cnt + sendsz;
		size = size - sendsz;
	}
	if (size > 0) {
		data[0] = (reg+cnt) >>8;
		data[1] = (reg+cnt) & 0xff;
		memcpy(data+2, val+cnt, size);
		mutex_lock(&charger->io_lock);
		ret = i2c_master_send(client, data, size+2);
		mutex_unlock(&charger->io_lock);
		if (ret < size+2) {
			dev_err(&client->dev, "%s: i2c write error, reg: 0x%x\n",
					__func__, reg);
			return ret < 0 ? ret : -EIO;
		}
	}

	return ret;
}

#if 0
static int mfc_firmware_verify(struct mfc_charger_data *charger)
{
	int ret = 0;
	const u16 sendsz = 16;
	size_t i = 0;
	int block_len = 0;
	int block_addr = 0;
	u8 rdata[sendsz+2];

/* I2C WR to prepare boot-loader write */

	if (mfc_reg_write(charger->client, 0x3000, 0x5a) < 0) {
		pr_err("%s: key error\n", __func__);
		return 0;
	}

	if (mfc_reg_write(charger->client, 0x3040, 0x11) < 0) {
		pr_err("%s: halt M0, OTP_I2C_EN set error\n", __func__);
		return 0;
	}

	dev_err(&charger->client->dev, "%s, request_firmware\n", __func__);
	ret = request_firmware(&charger->firm_data_bin, MFC_FLASH_FW_HEX_PATH,
		&charger->client->dev);
	if ( ret < 0) {
		dev_err(&charger->client->dev, "%s: failed to request firmware %s (%d)\n",
				__func__, MFC_FLASH_FW_HEX_PATH, ret);
		return 0;
	}
	ret = 1;
	wake_lock(&charger->wpc_update_lock);
	for (i = 0; i < charger->firm_data_bin->size; i += sendsz) {
		block_len = (i + sendsz) > charger->firm_data_bin->size ? charger->firm_data_bin->size - i : sendsz;
		block_addr = 0x8000 + i;

		if (mfc_reg_multi_read(charger->client, block_addr, rdata, block_len) < 0) {
			pr_err("%s, read failed\n", __func__);
			ret = 0;
			break;
		}
		if (datacmp(charger->firm_data_bin->data + i, rdata, block_len)) {
			pr_err("%s, verify data is not matched. block_len(%d), block_addr(%d)\n",
				__func__, block_len, block_addr);
			ret = -1;
			break;
		}
	}
	release_firmware(charger->firm_data_bin);

	wake_unlock(&charger->wpc_update_lock);
	return ret;
}
#endif

static int mfc_write_fw_flash_LSI(struct mfc_charger_data *charger, u8 addr_l, u8 addr_h, const u8 *wData, int length)
{
	int i;
	u8 ucTmpBuf[36];

	if (length <= 0) {
		pr_err("%s: skip. length is 0\n", __func__);
		return -1;
	}

	ucTmpBuf[0] = length / 4;
	ucTmpBuf[1] = addr_l;
	ucTmpBuf[2] = addr_h;

	for (i = 0; i < length; i++)
		ucTmpBuf[i + 3] = wData[i];

	if (mfc_reg_multi_write(charger->client, 0x1F11, ucTmpBuf, length + 3) < 0) {
		pr_err("%s: failed to write ucTmpBuf(%d bytes) at 0x1F11\n", __func__, length + 3);
		return -1;
	}

	if (mfc_reg_write(charger->client, 0x1F10, 0x52) < 0) {
		pr_err("%s: failed to write 0x52 at 0x1F10\n", __func__);
		return -1;
	}

	return 0;
}

//#define LSI_MFC_FW_BOOT_CODE_INCLUDE
#define LSI_MFC_FW_FLASH_START_ADDR		0x0C00
static int PgmOTPwRAM_LSI(struct mfc_charger_data *charger, unsigned short OtpAddr,
					  const u8 * srcData, int srcOffs, int size)
{
	int addr;
	int start_addr;
	u8 wdata[4] = {0,};
//	static int startAddr;
	u16 temp;
	u8 addr_l, addr_h;
	char chksum;
	int i;
	u8 fw_ver[4] = {0,};
	u8 fw_ver_bin[4] = {0,};

	mfc_reg_read(charger->client, MFC_FW_MAJOR_REV_L_REG, &fw_ver[0]);
	mfc_reg_read(charger->client, MFC_FW_MAJOR_REV_H_REG, &fw_ver[1]);
	pr_info("%s BEFORE rx major firmware version 0x%x\n",
		__func__, fw_ver[0] | (fw_ver[1] << 8));
	
	mfc_reg_read(charger->client, MFC_FW_MINOR_REV_L_REG, &fw_ver[2]);
	mfc_reg_read(charger->client, MFC_FW_MINOR_REV_H_REG, &fw_ver[3]);
	pr_info("%s BEFORE rx minor firmware version 0x%x\n",
		__func__, fw_ver[2] | (fw_ver[3] << 8));

	memcpy(fw_ver_bin, &srcData[MFC_FW_VER_BIN_LSI], 4);
	pr_info("%s NEW rx major firmware version 0x%x\n",
		__func__, fw_ver_bin[0] | (fw_ver_bin[1] << 8));
	pr_info("%s NEW rx minor firmware version 0x%x\n",
		__func__, fw_ver_bin[2] | (fw_ver_bin[3] << 8));

	pr_info("%s: Enter the flash mode (0x1F10)\n", __func__);
	if (mfc_reg_write(charger->client, 0x1F10, 0x10) < 0) {
		pr_err("%s: failed to enter the flash mode\n", __func__);
		return MFC_FWUP_ERR_FAIL;
	}
	msleep(100);
	pr_info("%s: Erase the flash memory\n", __func__);
	if (mfc_reg_write(charger->client, 0x1F10, 0x44) < 0) {
		pr_err("%s: failed to erase flash\n", __func__);
		return MFC_FWUP_ERR_FAIL;
	}

	msleep(1000); /* erasing flash needs 200ms delay at least */

#ifdef LSI_MFC_FW_BOOT_CODE_INCLUDE
	pr_info("%s: Erase boot code (0x1F10)\n", __func__);
	if (mfc_reg_write(charger->client, 0x1F10, 0x55) < 0) {
		pr_err("%s: failed to erase boot code\n", __func__);
		return MFC_FWUP_ERR_FAIL;
	}
	msleep(300);
#endif
	pr_info("%s: write fwimg by 32 bytes \n", __func__);

#ifdef LSI_MFC_FW_BOOT_CODE_INCLUDE
	start_addr = 0x00;
#else 
	start_addr = LSI_MFC_FW_FLASH_START_ADDR;
#endif
	chksum = 0;
	for (i = start_addr; i < size; i++)
		chksum += srcData[i];

	for (addr = start_addr; addr < size; addr += 32) // program pages of 32bytes
	{
		temp = addr & 0xff;
		addr_l = (u8)temp;
		temp = ((addr & 0xff00) >> 8);
		addr_h = (u8)temp;
		if (size - addr >= 32)
			mfc_write_fw_flash_LSI(charger, addr_l, addr_h, srcData + addr, 32);
		else
			mfc_write_fw_flash_LSI(charger, addr_l, addr_h, srcData + addr, size - addr);
	}

	pr_info("%s: write fw length --------------------\n", __func__);
	wdata[0] = (u8)(size & 0x00FF);
	wdata[1] = (u8)((size>>8) & 0x00FF);
	wdata[2] = 0x00;
	wdata[3] = 0x00;
	mfc_write_fw_flash_LSI(charger, 0xf4, 0x7e, wdata, 4);

	pr_info("%s: write fw checksum --------------------\n", __func__);
	chksum = -chksum;
	wdata[0] = chksum; /* checksum */
	wdata[1] = 0x00;
	wdata[2] = 0x00;
	wdata[3] = 0x00;
	mfc_write_fw_flash_LSI(charger, 0xf8, 0x7e, wdata, 4);

	pr_info("%s: write flash done flag --------------------*\n", __func__);
	wdata[0] = 0x01;
	wdata[1] = 0x00;
	wdata[2] = 0x00;
	wdata[3] = 0x00;
	mfc_write_fw_flash_LSI(charger, 0xfc, 0x7e, wdata, 4);

	msleep(10);
	pr_info("%s: Enter the normal mode\n", __func__);
	if (mfc_reg_write(charger->client, 0x1F10, 0x20) < 0) {
		pr_err("%s: failed to enter the normal mode\n", __func__);
		return MFC_FWUP_ERR_FAIL;
	}
	msleep(10);

	mfc_reg_read(charger->client, MFC_FW_MAJOR_REV_L_REG, &fw_ver[0]);
	mfc_reg_read(charger->client, MFC_FW_MAJOR_REV_H_REG, &fw_ver[1]);
	pr_info("%s NOW rx major firmware version 0x%x\n",
		__func__, fw_ver[0] | (fw_ver[1] << 8));
	
	mfc_reg_read(charger->client, MFC_FW_MINOR_REV_L_REG, &fw_ver[2]);
	mfc_reg_read(charger->client, MFC_FW_MINOR_REV_H_REG, &fw_ver[3]);
	pr_info("%s NOW rx minor firmware version 0x%x\n",
		__func__, fw_ver[2] | (fw_ver[3] << 8));

	for (i = 0; i < 4; i++)
		if (fw_ver[i] != fw_ver_bin[i])
			return MFC_FWUP_ERR_COMMON_FAIL;

	return MFC_FWUP_ERR_SUCCEEDED;
}

static void mfc_reset_rx_power(struct mfc_charger_data *charger, u8 rx_power)
{
	if(charger->adt_transfer_status == WIRELESS_AUTH_PASS) {
		union power_supply_propval value = {0, };
		switch(rx_power) {
		case TX_RX_POWER_7_5W:
			value.intval = SEC_WIRELESS_RX_POWER_7_5W;
			break;
		case TX_RX_POWER_12W:
			value.intval = SEC_WIRELESS_RX_POWER_12W;
			break;
		case TX_RX_POWER_15W:
			value.intval = SEC_WIRELESS_RX_POWER_15W;
			break;
		case TX_RX_POWER_17_5W:
			value.intval = SEC_WIRELESS_RX_POWER_17_5W;
			break;
		case TX_RX_POWER_20W:
			value.intval = SEC_WIRELESS_RX_POWER_20W;
			break;
		default:
			value.intval = SEC_WIRELESS_RX_POWER_12W;
			pr_info("%s %s : undefine rx power reset\n", WC_AUTH_MSG, __func__);
			break;
		}
		psy_do_property("wireless", set,
			POWER_SUPPLY_PROP_WIRELESS_RX_POWER, value);
	} else
		pr_info("%s %s : undefine rx power scenario, It is auth failed case how dose it get rx power? \n", WC_AUTH_MSG, __func__);
}

static void mfc_wpc_cm_fet_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_cm_fet_work.work);
	u8 tmp = 0;

	/* disable all CM FETs for MST operation */
	mfc_reg_write(charger->client, MFC_RX_COMM_MOD_FET_REG, 0xf0);
	mfc_reg_read(charger->client, MFC_RX_COMM_MOD_FET_REG, &tmp);
	pr_info("%s: disable CM FET (0x%x)\n", __func__, tmp);
}

static void mfc_wpc_afc_vout_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_afc_vout_work.work);
	union power_supply_propval value = {0, };

	pr_info("%s start, current cable(%d)\n", __func__, charger->pdata->cable_type);

	/* change cable type */
	if (charger->pdata->cable_type == SEC_WIRELESS_PAD_PREPARE_HV)
		charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_WPC_HV;
	else
		value.intval = charger->pdata->cable_type;

	psy_do_property("wireless", set,
		POWER_SUPPLY_PROP_ONLINE, value);

#if defined(CONFIG_BATTERY_SWELLING)
	/* check swelling mode */
	psy_do_property("battery", get,
		POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT, value);
	pr_info("%s: check swelling mode(%d)\n", __func__, value.intval);

	if (value.intval)
		goto skip_set_afc_vout;
#endif

	if (sleep_mode)
		goto skip_set_afc_vout;

	pr_info("%s: check OTG(%d), full(%d) tx_id(%d)\n",
		__func__, charger->is_otg_on, charger->is_full_status, charger->tx_id);

	if (charger->is_otg_on)
		goto skip_set_afc_vout;

	if (charger->is_full_status &&
		(charger->tx_id != TX_ID_DREAM_STAND &&
			charger->tx_id != TX_ID_DREAM_DOWN))
		goto skip_set_afc_vout;

	// need to fix here to get vout setting
	if (charger->pdata->cable_type == SEC_WIRELESS_PAD_WPC_HV_20)
		mfc_send_command(charger, charger->vrect_by_txid);
	else
		mfc_send_command(charger, MFC_AFC_CONF_10V_TX);

	charger->is_afc_tx = true;
	pr_info("%s: is_afc_tx = %d vout read = %d\n",
		__func__, charger->is_afc_tx, mfc_get_adc(charger, MFC_ADC_VOUT));

	pr_info("%s: check state(Vmode:%d, Vstatus:%d, Otg:%d)\n", __func__,
		charger->vout_mode, charger->pdata->vout_status, charger->is_otg_on);

	if (!charger->is_full_status &&
		charger->vout_mode != WIRELESS_VOUT_5V &&
		charger->vout_mode != WIRELESS_VOUT_5V_STEP &&
		charger->vout_mode != WIRELESS_VOUT_5_5V_STEP) {
		// need to fix here
		if (charger->pdata->cable_type == SEC_WIRELESS_PAD_WPC_HV_20)
			charger->vout_mode = charger->vout_by_txid;
		else
			charger->vout_mode = WIRELESS_VOUT_10V;
		cancel_delayed_work(&charger->wpc_vout_mode_work);
		wake_lock(&charger->wpc_vout_mode_lock);
		queue_delayed_work(charger->wqueue,
			&charger->wpc_vout_mode_work, 0);
	}

skip_set_afc_vout:
	wake_unlock(&charger->wpc_afc_vout_lock);
}

static void mfc_wpc_fw_update_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_fw_update_work.work);

	int ret = 0;
	int i = 0;
	char fwtime[8] = {32, 32, 32, 32, 32, 32, 32, 32};
	char fwdate[8] = {32, 32, 32, 32, 32, 32, 32, 32};
	u8 data = 32; /* ascii space */
#if defined(CONFIG_WIRELESS_IC_PARAM)
	u32 param_data = 0;
#endif

	pr_info("%s firmware update mode is = %d\n", __func__, charger->fw_cmd);

	if (gpio_get_value(charger->pdata->wpc_en)) {
		pr_info("%s: wpc_en disabled\n", __func__);
		goto end_of_fw_work;
	}

	mfc_set_wpc_en(charger, WPC_EN_FW, true); /* keep the wpc_en low during fw update */

	switch(charger->fw_cmd) {
	case SEC_WIRELESS_RX_SPU_MODE:
	case SEC_WIRELESS_RX_SDCARD_MODE:
		mfc_uno_on(charger, true);
		msleep(200);
		if (mfc_get_chip_id(charger) < 0 ||
			charger->chip_id != MFC_CHIP_LSI) {
			pr_info("%s: current IC's chip_id(0x%x) is not matching to driver's chip_id(0x%x)\n",
				__func__, charger->chip_id, MFC_CHIP_LSI);
			break;
		}
		charger->pdata->otp_firmware_result = MFC_FWUP_ERR_RUNNING;
		disable_irq(charger->pdata->irq_wpc_int);
		disable_irq(charger->pdata->irq_wpc_det);
#if defined(CONFIG_WIRELESS_IC_PARAM)
		mfc_set_wireless_param(MFC_CHIP_ID_S2MIW04, 0);
#endif
		ret = PgmOTPwRAM_LSI(charger, 0, charger->fw_img, 0, charger->fw_size);

		charger->pdata->otp_firmware_ver = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);
		charger->pdata->wc_ic_rev = mfc_get_ic_revision(charger, MFC_IC_REVISION);

		if (ret == MFC_FWUP_ERR_SUCCEEDED)
			charger->pdata->otp_firmware_result = MFC_FW_RESULT_PASS;
		else
			charger->pdata->otp_firmware_result = ret;

		enable_irq(charger->pdata->irq_wpc_int);
		enable_irq(charger->pdata->irq_wpc_det);
		break;
	case SEC_WIRELESS_RX_INIT:
	case SEC_WIRELESS_RX_BUILT_IN_MODE:
		mfc_uno_on(charger, true);
		msleep(200);
		if (mfc_get_chip_id(charger) < 0 ||
			charger->chip_id != MFC_CHIP_LSI) {
			pr_info("%s: current IC's chip_id(0x%x) is not matching to driver's chip_id(0x%x)\n",
				__func__, charger->chip_id, MFC_CHIP_LSI);
			break;
		}
		if (charger->fw_cmd == SEC_WIRELESS_RX_INIT) {
			charger->pdata->otp_firmware_ver = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);
#if defined(CONFIG_WIRELESS_IC_PARAM)
			if (wireless_fw_mode_param == SEC_WIRELESS_RX_SPU_MODE &&
				charger->pdata->otp_firmware_ver > MFC_FW_BIN_VERSION) {
				pr_info("%s: current version (0x%x) is higher than BIN_VERSION by SPU(0x%x)\n",
					__func__, charger->pdata->otp_firmware_ver, MFC_FW_BIN_VERSION);
				break;
			}
#endif
			if (charger->pdata->otp_firmware_ver == MFC_FW_BIN_VERSION) {
				pr_info("%s: current version (0x%x) is same to BIN_VERSION (0x%x)\n",
					__func__, charger->pdata->otp_firmware_ver, MFC_FW_BIN_VERSION);
				break;
			}
		}
		charger->pdata->otp_firmware_result = MFC_FWUP_ERR_RUNNING;

		disable_irq(charger->pdata->irq_wpc_int);
		disable_irq(charger->pdata->irq_wpc_det);
		dev_err(&charger->client->dev, "%s, request_firmware\n", __func__);

		ret = request_firmware(&charger->firm_data_bin, MFC_FLASH_FW_HEX_LSI_PATH,
			&charger->client->dev);
		if ( ret < 0) {
			dev_err(&charger->client->dev, "%s: failed to request firmware %s (%d)\n", __func__, MFC_FLASH_FW_HEX_LSI_PATH, ret);
			charger->pdata->otp_firmware_result = MFC_FWUP_ERR_REQUEST_FW_BIN;
			goto fw_err;
		}
		wake_lock(&charger->wpc_update_lock);
#if defined(CONFIG_WIRELESS_IC_PARAM)
		mfc_set_wireless_param(MFC_CHIP_ID_S2MIW04, 0);
#endif
		pr_info("%s data size = %ld\n", __func__, charger->firm_data_bin->size);
		ret = PgmOTPwRAM_LSI(charger, 0 ,charger->firm_data_bin->data, 0, charger->firm_data_bin->size);

		release_firmware(charger->firm_data_bin);

		for (i = 0; i < 8; i++) {
			if (mfc_reg_read(charger->client, MFC_FW_DATA_CODE_0+i, &data) > 0)
				fwdate[i] = (char)data;
		}
		for (i = 0; i < 8; i++) {
			if (mfc_reg_read(charger->client, MFC_FW_TIMER_CODE_0+i, &data) > 0)
				fwtime[i] = (char)data;
		}
		pr_info("%s: %d%d%d%d%d%d%d%d, %d%d%d%d%d%d%d%d\n", __func__,
			fwdate[0], fwdate[1], fwdate[2],fwdate[3], fwdate[4], fwdate[5], fwdate[6], fwdate[7],
			fwtime[0], fwtime[1], fwtime[2],fwtime[3], fwtime[4], fwtime[5], fwtime[6], fwtime[7]);

		charger->pdata->otp_firmware_ver = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);
		charger->pdata->wc_ic_rev = mfc_get_ic_revision(charger, MFC_IC_REVISION);

		if (ret == MFC_FWUP_ERR_SUCCEEDED) {
			charger->pdata->otp_firmware_result = MFC_FW_RESULT_PASS;
#if defined(CONFIG_WIRELESS_IC_PARAM)
			ret = sec_get_param_u32(wireless_offset, &param_data);
			if (ret < 0) {
				pr_err("%s: sec_get_param failed\n", __func__);
			} else {
				param_data &= 0xFFFFFF0F;
				param_data |= (charger->fw_cmd & 0xF) << 4;
				pr_info("%s: param data 0x%08X\n", __func__, param_data);
				ret = sec_set_param_u32(wireless_offset, param_data);
				if (ret < 0) {
					pr_err("%s: sec_set_param failed\n", __func__);
				} else {
					wireless_fw_mode_param = charger->fw_cmd & 0xF;
					pr_info("%s: succeed. fw_mode(0x%01X)\n", __func__, wireless_fw_mode_param);
				}
			}
#endif			
		} else
			charger->pdata->otp_firmware_result = ret;

		for (i = 0; i < 8; i++) {
			if (mfc_reg_read(charger->client, MFC_FW_DATA_CODE_0+i, &data) > 0)
				fwdate[i] = (char)data;
		}
		for (i = 0; i < 8; i++) {
			if (mfc_reg_read(charger->client, MFC_FW_TIMER_CODE_0+i, &data) > 0)
				fwtime[i] = (char)data;
		}
		pr_info("%s: %d%d%d%d%d%d%d%d, %d%d%d%d%d%d%d%d\n", __func__,
			fwdate[0], fwdate[1], fwdate[2],fwdate[3], fwdate[4], fwdate[5], fwdate[6], fwdate[7],
			fwtime[0], fwtime[1], fwtime[2],fwtime[3], fwtime[4], fwtime[5], fwtime[6], fwtime[7]);

		enable_irq(charger->pdata->irq_wpc_int);
		enable_irq(charger->pdata->irq_wpc_det);
		wake_unlock(&charger->wpc_update_lock);
		break;
	case SEC_WIRELESS_TX_ON_MODE:
		charger->pdata->cable_type = SEC_WIRELESS_PAD_TX;
		break;
	case SEC_WIRELESS_TX_OFF_MODE:
		/* need to check */
		break;
	default:
		break;
	}

	msleep(200);
	mfc_uno_on(charger, false);
	pr_info("%s ---------------------------------------------------------------\n", __func__);
	mfc_fw_update = false;
	if (charger->fw_img)
		kfree(charger->fw_img);
	charger->fw_img = NULL;

	mfc_set_wpc_en(charger, WPC_EN_FW, false);

	return;
fw_err:
	mfc_uno_on(charger, false);
	mfc_set_wpc_en(charger, WPC_EN_FW, false);
end_of_fw_work:
	mfc_fw_update = false;
}

static int mfc_chg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct mfc_charger_data *charger = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;
//	union power_supply_propval value;
	u8 mst_mode;
	u8 reg_data;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		pr_info("%s charger->pdata->cs100_status %d\n", __func__, charger->pdata->cs100_status);
		val->intval = charger->pdata->cs100_status;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = mfc_reg_read(charger->client, MFC_MST_MODE_SEL_REG, &mst_mode);
		if (val->intval < 0) {
			pr_info("%s mst mode(0x2) i2c write failed, val->intval = %d\n",
					__func__, val->intval);
			break;
		}

		val->intval = mfc_reg_read(charger->client, MFC_SYS_OP_MODE_REG, &reg_data);
		if (val->intval < 0) {
			pr_info("%s mst mode change irq(0x4) read failed, val->intval = %d\n",
					__func__, val->intval);
			break;
		}
		reg_data &= 0x0C; /* use only [3:2]bit of sys_op_mode register for MST */

		pr_info("%s mst mode check: mst_mode = %d, reg_data = %d\n",
				__func__, mst_mode, reg_data);
		val->intval = 0;
		if (reg_data == 0x4)
			val->intval = mst_mode;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_HEALTH:
		return -ENODATA;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		if (charger->pdata->is_charging) {
			val->intval = mfc_get_vout(charger);
		} else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
	case POWER_SUPPLY_PROP_CHARGE_POWERED_OTG_CONTROL:
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		return -ENODATA;
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("%s cable_type =%d\n", __func__, charger->pdata->cable_type);
		val->intval = charger->pdata->cable_type;
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
		val->intval = charger->pdata->vout_status;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		pr_info("%s: POWER_SUPPLY_PROP_MANUFACTURER, intval(0x%x), called by(%ps)\n", __func__, val->intval, __builtin_return_address(0));
		if (val->intval == SEC_WIRELESS_OTP_FIRM_RESULT) {
			pr_info("%s otp firmware result = %d,\n", __func__, charger->pdata->otp_firmware_result);
			val->intval = charger->pdata->otp_firmware_result;
		} else if (val->intval == SEC_WIRELESS_IC_REVISION) {
			pr_info("%s: check ic revision\n", __func__);
			val->intval = mfc_get_ic_revision(charger, MFC_IC_REVISION);
		} else if (val->intval == SEC_WIRELESS_IC_CHIP_ID) {
			pr_info("%s: check ic chip_id(0x%02X)\n", __func__, charger->chip_id);
			val->intval = charger->chip_id;
		} else if (val->intval == SEC_WIRELESS_OTP_FIRM_VER_BIN) {
			/* update latest  kernl f/w version */
			val->intval = MFC_FW_BIN_VERSION;
		} else if (val->intval == SEC_WIRELESS_OTP_FIRM_VER) {
			val->intval = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);
			pr_info("%s: check f/w revision (0x%x)\n", __func__, val->intval);
			if (val->intval < 0 && charger->pdata->otp_firmware_ver > 0)
				val->intval = charger->pdata->otp_firmware_ver;
		} else if (val->intval == SEC_WIRELESS_TX_FIRM_RESULT) {
			val->intval = charger->pdata->tx_firmware_result;
		} else if (val->intval == SEC_WIRELESS_TX_FIRM_VER) {
			val->intval = charger->pdata->tx_firmware_ver;
		} else if (val->intval == SEC_TX_FIRMWARE) {
			val->intval = charger->tx_status;
		} else if (val->intval == SEC_WIRELESS_OTP_FIRM_VERIFY) {
			pr_info("%s: LSI FIRM_VERIFY is not implemented\n", __func__);
			val->intval = 1;
		} else if (val->intval == SEC_WIRELESS_MST_SWITCH_VERIFY) {
			if (gpio_is_valid(charger->pdata->mst_pwr_en)) {
				gpio_direction_output(charger->pdata->mst_pwr_en, 1);
				msleep(charger->pdata->mst_switch_delay);
				val->intval = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);
				pr_info("%s: check f/w revision, mst power on (0x%x)\n", __func__, val->intval);
				gpio_direction_output(charger->pdata->mst_pwr_en, 0);
			} else {
				pr_info("%s: MST_SWITCH_VERIFY, invalid gpio(mst_pwr_en)\n", __func__);
				val->intval = -1;
			}
		} else {
			val->intval = -ENODATA;
			pr_err("%s wrong mode\n", __func__);
		}
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW: /* vout */
		if (charger->pdata->is_charging) {
			val->intval = mfc_get_adc(charger, MFC_ADC_VOUT);
			pr_info("%s: wc vout (%d)\n", __func__, val->intval);
		} else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_ENERGY_AVG: /* vrect */
		if (charger->pdata->is_charging) {
			val->intval = mfc_get_adc(charger, MFC_ADC_VRECT);
		} else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		val->intval = charger->vrect_by_txid;
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		val->intval = mfc_get_adc(charger, val->intval);
		break;
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ:
			val->intval = mfc_get_adc(charger, MFC_ADC_OP_FRQ);
			pr_info("%s: Operating FQ %dkHz\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TRX_CMD:
			val->intval = charger->pdata->trx_data_cmd;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TRX_VAL:
			val->intval = charger->pdata->trx_data_val;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID:
			val->intval = charger->tx_id;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONNECTED:
			val->intval = charger->wc_rx_connected;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_TYPE:
			val->intval = charger->wc_rx_type;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_VIN:
			val->intval = mfc_get_adc(charger, MFC_ADC_TX_VOUT);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN:
			val->intval = mfc_get_adc(charger, MFC_ADC_TX_IOUT);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_POWER:
			val->intval = charger->current_rx_power;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS:
			val->intval = charger->adt_transfer_status;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_DATA:
			{
				//int i = 0;
				//u8 *p_data;

				if(charger->adt_transfer_status == WIRELESS_AUTH_RECEIVED) {
					pr_info("%s %s : MFC_ADT_RECEIVED (%d)\n", WC_AUTH_MSG, __func__, charger->adt_transfer_status);
					val->strval = (u8 *)ADT_buffer_rdata;
					//p_data = ADT_buffer_rdata;
					//for(i=0; i< adt_readSize; i++)
					//	pr_info("%s: auth read data = %x", __func__, p_data[i]);
					//pr_info("\n", __func__);
				} else {
					pr_info("%s: data hasn't been received yet\n", __func__);
					return -ENODATA;
				}
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_SIZE:
			val->intval = adt_readSize;
			pr_info("%s %s : MFC_ADT_RECEIVED (%d), DATA SIZE(%d)\n", WC_AUTH_MSG, __func__, charger->adt_transfer_status, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_VOUT:
			val->intval = charger->vout_mode;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_INITIAL_WC_CHECK:
			val->intval = charger->initial_wc_check;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_CHECK_FW_VER:
#if defined(CONFIG_WIRELESS_IC_PARAM)
			pr_info("%s: fw_ver (param:0x%04X, build:0x%04X)\n",
				__func__, wireless_fw_ver_param, MFC_FW_BIN_VERSION);
			if (wireless_fw_ver_param == MFC_FW_BIN_VERSION)
				val->intval = 1;
			else
				val->intval = 0;
#else
			val->intval = 0;
#endif
			break;
		case POWER_SUPPLY_EXT_PROP_WPC_EN:
			val->intval = gpio_get_value(charger->pdata->wpc_en);
			break;
		case POWER_SUPPLY_EXT_PROP_MONITOR_WORK:
			if (gpio_get_value(charger->pdata->wpc_en))
				pr_info("%s: charger->wpc_en_flag(0x%x)\n", __func__, charger->wpc_en_flag);
			break;
		default:
			return -ENODATA;
		}
		break;
	default:
		return -ENODATA;
	}
	return 0;
}

static void mfc_wpc_vout_mode_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_vout_mode_work.work);
	int vout_step = charger->pdata->vout_status;
	int vout = MFC_VOUT_10V;
	union power_supply_propval value;

	if (is_shutdn) {
		pr_err("%s: Escape by shtudown\n", __func__);
		return;
	}
	pr_info("%s: start - vout_mode(%s), vout_status(%s)\n",
		__func__, mfc_vout_control_mode_str[charger->vout_mode], rx_vout_str[charger->pdata->vout_status]);
	switch (charger->vout_mode) {
	case WIRELESS_VOUT_5V:
		mfc_set_vout(charger, MFC_VOUT_5V);
		break;
	case WIRELESS_VOUT_9V:
		mfc_set_vout(charger, MFC_VOUT_9V);
		break;
	case WIRELESS_VOUT_10V:
		mfc_set_vout(charger, MFC_VOUT_10V);
		/* reset AICL */
		psy_do_property("wireless", set,
			POWER_SUPPLY_PROP_CURRENT_MAX, value);
		break;
	case WIRELESS_VOUT_11V:
		mfc_set_vout(charger, MFC_VOUT_11V);
		/* reset AICL */
		psy_do_property("wireless", set,
			POWER_SUPPLY_PROP_CURRENT_MAX, value);
		break;
	case WIRELESS_VOUT_12V:
		mfc_set_vout(charger, MFC_VOUT_12V);
		/* reset AICL */
		psy_do_property("wireless", set,
			POWER_SUPPLY_PROP_CURRENT_MAX, value);
		break;
	case WIRELESS_VOUT_12_5V:
		mfc_set_vout(charger, MFC_VOUT_12_5V);
		/* reset AICL */
		psy_do_property("wireless", set,
			POWER_SUPPLY_PROP_CURRENT_MAX, value);
		break;
	case WIRELESS_VOUT_5V_STEP:
		vout_step--;
		if (vout_step >= MFC_VOUT_5V) {
			mfc_set_vout(charger, vout_step);
			cancel_delayed_work(&charger->wpc_vout_mode_work);
			queue_delayed_work(charger->wqueue,
				&charger->wpc_vout_mode_work, msecs_to_jiffies(250));
			return;
		}
		break;
	case WIRELESS_VOUT_5_5V_STEP:
		vout_step--;
		if (vout_step >= MFC_VOUT_5_5V) {
			mfc_set_vout(charger, vout_step);
			cancel_delayed_work(&charger->wpc_vout_mode_work);
			queue_delayed_work(charger->wqueue,
				&charger->wpc_vout_mode_work, msecs_to_jiffies(250));
			return;
		}
		break;
	case WIRELESS_VOUT_9V_STEP:
		vout = MFC_VOUT_9V;
	case WIRELESS_VOUT_10V_STEP:
		vout_step++;
		if (vout_step <= vout) {
			mfc_set_vout(charger, vout_step);
			cancel_delayed_work(&charger->wpc_vout_mode_work);
			queue_delayed_work(charger->wqueue,
				&charger->wpc_vout_mode_work, msecs_to_jiffies(250));
			return;
		}
		break;
	case WIRELESS_VOUT_CC_CV_VOUT:
		mfc_set_vout(charger, MFC_VOUT_5_5V);
		break;
	default:
		break;
	}
#if !defined(CONFIG_SEC_FACTORY)
	if ((is_hv_wireless_pad_type(charger->pdata->cable_type)) &&
            (charger->pdata->vout_status <= MFC_VOUT_5_5V && (charger->is_full_status || sleep_mode))) {
		mfc_send_command(charger, MFC_AFC_CONF_5V_TX);
		pr_info("%s: set TX 5V after cs100\n", __func__);
	}
#endif
	pr_info("%s: finish - vout_mode(%s), vout_status(%s)\n",
		__func__, mfc_vout_control_mode_str[charger->vout_mode], rx_vout_str[charger->pdata->vout_status]);
	wake_unlock(&charger->wpc_vout_mode_lock);
}

static void mfc_wpc_i2c_error_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_i2c_error_work.work);

	if (charger->wc_w_state &&
		gpio_get_value(charger->pdata->wpc_det)) {
		union power_supply_propval value;

		psy_do_property("battery", set,
			POWER_SUPPLY_EXT_PROP_WC_CONTROL, value);
	}
}

static void mfc_set_tx_fod_with_gear(struct mfc_charger_data *charger)
{
	struct timespec ts = {0, };

	pr_info("%s: FOD1 1300mA, FOD2 700mA\n", __func__);
	/* FOD1 LIMIT 1300mA */
	mfc_reg_write(charger->client, MFC_TX_OC_FOD1_LIMIT_L_REG, 0x14);
	mfc_reg_write(charger->client, MFC_TX_OC_FOD1_LIMIT_H_REG, 0x5);
	/* FOD2 LIMIT 700mA */
	mfc_reg_write(charger->client, MFC_TX_OC_FOD2_LIMIT_L_REG, 0xBC);
	mfc_reg_write(charger->client, MFC_TX_OC_FOD2_LIMIT_H_REG, 0x2);

	if (charger->gear_start_time == 0) {
		ts = ktime_to_timespec(ktime_get_boottime());
		charger->gear_start_time = ts.tv_sec;
	}
}

static void mfc_wpc_rx_det_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_rx_det_work.work);
	u8 reg_data;
	u8 data[2] = {0,};
	int ret = 0, prmc_id = 0;
	union power_supply_propval value;

	if (!charger->wc_tx_enable) {
		wake_unlock(&charger->wpc_rx_det_lock);
		return;
	}		

	mfc_reg_read(charger->client, MFC_STARTUP_EPT_COUNTER, &reg_data);
	
	ret = mfc_reg_read(charger->client, MFC_TX_RXID1_READ_REG, &data[0]);
//	ret = mfc_reg_read(charger->client, MFC_PRMC_ID_H_REG, &data[1]);
	if (ret >= 0) {
		prmc_id = data[0] | (data[1] << 8);
	}
	pr_info("@Tx_Mode %s prmc_id 0x%x\n", __func__, prmc_id);

	if (prmc_id == 0x42 && reg_data >= 1) {
		pr_info("@Tx_Mode %s : Samsung Gear Connected\n", __func__);
		charger->wc_rx_type = SS_GEAR;

		mfc_set_tx_fod_with_gear(charger);
	} else if (prmc_id == 0x42) {
		pr_info("@Tx_Mode %s : Samsung Phone Connected\n", __func__);
		charger->wc_rx_type = SS_PHONE;
	} else {
		pr_info("@Tx_Mode %s : Unknown device connected\n", __func__);
		charger->wc_rx_type = OTHER_DEV;
	}

	value.intval = charger->wc_rx_type;
	psy_do_property("wireless", set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_RX_TYPE, value);

	wake_unlock(&charger->wpc_rx_det_lock);
}

static void mfc_tx_op_freq_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_tx_op_freq_work.work);

	pr_info("%s \n", __func__);

	/* recover tx op freq */
	mfc_set_min_duty(charger, 20);
	pr_info("%s : tx op freq = %dKhz\n", __func__, mfc_get_adc(charger, MFC_ADC_TX_OP_FRQ));

	wake_unlock(&charger->wpc_tx_opfq_lock);

}

static void mfc_check_tx_gear_time(struct mfc_charger_data *charger)
{
	struct timespec ts = {0, };
	unsigned long gear_charging_time;

	get_monotonic_boottime(&ts);

	if (ts.tv_sec >= charger->gear_start_time) {
		gear_charging_time = ts.tv_sec - charger->gear_start_time;
	} else {
		gear_charging_time = 0xFFFFFFFF - charger->gear_start_time
			+ ts.tv_sec;
	}

	if ((gear_charging_time >= 90) && charger->gear_start_time) {
		pr_info("@Tx_Mode %s: set OC FOD1 900mA for Gear, gear_charging_time(%ld)\n", __func__, gear_charging_time);
		/* FOD1 LIMIT 900mA */
		mfc_reg_write(charger->client, MFC_TX_OC_FOD1_LIMIT_L_REG, 0x84);
		mfc_reg_write(charger->client, MFC_TX_OC_FOD1_LIMIT_H_REG, 0x3);
		charger->gear_start_time = 0;
	}
}

static void mfc_tx_phm_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_tx_phm_work.work);

	/* noirq resume of devices complete after 200 msecs */
	if(charger->is_suspend)
		msleep(200);

	pr_info("%s \n", __func__);
	mfc_set_cmd_l_reg(charger,MFC_CMD_TOGGLE_PHM_MASK, MFC_CMD_TOGGLE_PHM_MASK);

	wake_unlock(&charger->wpc_tx_phm_lock);
}

#if defined(CONFIG_UPDATE_BATTERY_DATA)
static int mfc_chg_parse_dt(struct device *dev, mfc_charger_platform_data_t *pdata);
#endif
static int mfc_chg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct mfc_charger_data *charger = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;
	int vout, vrect, iout, freq, tx_freq, i = 0;
	u8 tmp = 0;
	/* int ret; */
	u8 fod[MFC_NUM_FOD_REG] = {0, };

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (val->intval == POWER_SUPPLY_STATUS_FULL) {
			pr_info("%s set cs100\n", __func__);
			if (is_wireless_pad_type(charger->pdata->cable_type)) {
				/* set fake FOD values before send cs100, need to tune */
				mfc_fod_set_cs100(charger);
			}
			charger->pdata->cs100_status = mfc_send_cs100(charger);
#if !defined(CONFIG_SEC_FACTORY)
			if (!charger->is_full_status) {
				charger->is_full_status = 1;
				if (is_hv_wireless_pad_type(charger->pdata->cable_type) &&
					(charger->tx_id != TX_ID_DREAM_STAND &&
					charger->tx_id != TX_ID_DREAM_DOWN)) {
					charger->vout_mode = WIRELESS_VOUT_5_5V_STEP;
					cancel_delayed_work(&charger->wpc_vout_mode_work);
					wake_lock(&charger->wpc_vout_mode_lock);
					queue_delayed_work(charger->wqueue,
						&charger->wpc_vout_mode_work, msecs_to_jiffies(250));
				}
			}
#endif

		} else if (val->intval == POWER_SUPPLY_STATUS_NOT_CHARGING) {
			mfc_mis_align(charger);
		} else if (val->intval == POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE) {
#if 0
			if (charger->tx_id == TX_ID_DREAM_STAND) {
				mfc_fod_set_dream_cv(charger);
			} else
#endif
				mfc_fod_set_cv(charger);

		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	{
		union power_supply_propval value;
		value.intval = charger->pdata->cable_type;
		psy_do_property("wireless", set, POWER_SUPPLY_PROP_ONLINE, value);

		if (charger->is_otg_on) {
			psy_do_property("wireless", set,
				POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
		}
	}
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		if (val->intval == POWER_SUPPLY_HEALTH_OVERHEAT ||
			val->intval == POWER_SUPPLY_HEALTH_OVERHEATLIMIT ||
			val->intval == POWER_SUPPLY_HEALTH_COLD) {
			mfc_send_eop(charger, val->intval);
		}
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("%s: ept-internal fault\n", __func__);
		mfc_reg_write(charger->client, MFC_EPT_REG, MFC_WPC_EPT_INT_FAULT);
		mfc_set_cmd_l_reg(charger, MFC_CMD_SEND_EOP_MASK, MFC_CMD_SEND_EOP_MASK);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		if (val->intval) {
			charger->is_mst_on = MST_MODE_2;
			pr_info("%s: set MST mode 2\n", __func__);
			/* disable CM FETs to avoid MST/WPC crash situation  */
			queue_delayed_work(charger->wqueue,
				&charger->wpc_cm_fet_work, msecs_to_jiffies(1000));
		} else {
			pr_info("%s: set MST mode off\n", __func__);
			charger->is_mst_on = MST_MODE_0;
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		charger->pdata->siop_level = val->intval;
		if (charger->pdata->siop_level == 100) {
			pr_info("%s vrect headroom set ROOM 2, siop = %d\n", __func__, charger->pdata->siop_level);
			mfc_set_vrect_adjust(charger, MFC_HEADROOM_2);
		} else if (charger->pdata->siop_level < 100) {
			pr_info("%s vrect headroom set ROOM 0, siop = %d\n", __func__, charger->pdata->siop_level);
			mfc_set_vrect_adjust(charger, MFC_HEADROOM_0);
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		if (val->intval) {
			pr_info("%s: mfc otg on\n", __func__);
			charger->is_otg_on = true;
		} else {
			pr_info("%s: mfc otg off\n", __func__);
			charger->is_otg_on = false;
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_POWERED_OTG_CONTROL:
		{
			unsigned int work_state;
			struct file *fp;
			mm_segment_t old_fs;

			mutex_lock(&charger->fw_lock);
			/* check delayed work state */
			work_state = work_busy(&charger->wpc_fw_update_work.work);
			pr_info("%s: check fw_work state(0x%x)\n", __func__, work_state);
			if (work_state & (WORK_BUSY_PENDING | WORK_BUSY_RUNNING)) {
				pr_info("%s: skip update_fw!!\n", __func__);
				mutex_unlock(&charger->fw_lock);
				return 0;
			}

			charger->fw_cmd = val->intval;
			if (charger->fw_cmd == SEC_WIRELESS_RX_SPU_MODE ||
				charger->fw_cmd == SEC_WIRELESS_RX_SDCARD_MODE) {
				long nread;
				const char *fw_path = MFC_FW_SDCARD_BIN_PATH;

				if (charger->fw_cmd == SEC_WIRELESS_RX_SPU_MODE)
					fw_path = MFC_FW_SPU_BIN_PATH;

				old_fs = get_fs();
				set_fs(KERNEL_DS);

				fp = filp_open(fw_path, O_RDONLY, S_IRUSR);

				if (IS_ERR(fp)) {
					pr_err("%s: failed to open %s, (%d)\n", __func__, fw_path, (int)PTR_ERR(fp));
					set_fs(old_fs);
					mutex_unlock(&charger->fw_lock);
					return 0;
				}

				charger->fw_size = fp->f_path.dentry->d_inode->i_size;
				pr_err("%s: start, file path %s, size %ld Bytes\n",
					__func__, fw_path, charger->fw_size);

				charger->fw_img = kmalloc(charger->fw_size, GFP_KERNEL);
				if (charger->fw_img == NULL) {
					pr_err("%s, kmalloc failed\n", __func__);
					goto out;
				}

				nread = vfs_read(fp, (char __user *)charger->fw_img,
							charger->fw_size, &fp->f_pos);
				pr_err("nread %ld Bytes\n", nread);
				if (nread != charger->fw_size) {
					pr_err("failed to read firmware file, nread %ld Bytes\n", nread);
					goto out;
				}

				if (charger->fw_cmd == SEC_WIRELESS_RX_SPU_MODE) {
					if (spu_firmware_signature_verify("MFC", charger->fw_img, charger->fw_size) == (charger->fw_size - SPU_METADATA_SIZE(MFC))) {
						pr_err("%s, spu_firmware_signature_verify success\n", __func__);
						charger->fw_size -= SPU_METADATA_SIZE(MFC);
					} else {
						pr_err("%s, spu_firmware_signature_verify failed\n", __func__);
						goto out;
					}
				}

				filp_close(fp, current->files);
				set_fs(old_fs);
			}
			queue_delayed_work(charger->wqueue, &charger->wpc_fw_update_work, 0);
			pr_info("%s: rx result = %d, tx result = %d\n", __func__,
				charger->pdata->otp_firmware_result, charger->pdata->tx_firmware_result);
			mutex_unlock(&charger->fw_lock);
			return 0;

out:
			filp_close(fp, current->files);
			set_fs(old_fs);
			if (charger->fw_img)
				kfree(charger->fw_img);
			charger->fw_img = NULL;
			mutex_unlock(&charger->fw_lock);
		}
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
		if (val->intval == WIRELESS_VOUT_NORMAL_VOLTAGE) {
			pr_info("%s: Wireless Vout forced set to 5V. + PAD CMD\n", __func__);
			for (i = 0; i < CMD_CNT; i++) {
				mfc_send_command(charger, MFC_AFC_CONF_5V);
				msleep(250);
			}
		} else if (val->intval == WIRELESS_VOUT_HIGH_VOLTAGE) {
			pr_info("%s: Wireless Vout forced set to 10V. + PAD CMD\n", __func__);
			for (i = 0; i < CMD_CNT; i++) {
				mfc_send_command(charger, MFC_AFC_CONF_10V);
				msleep(250);
			}
		} else if (val->intval == WIRELESS_VOUT_CC_CV_VOUT) {
			charger->vout_mode = val->intval;
			cancel_delayed_work(&charger->wpc_vout_mode_work);
			wake_lock(&charger->wpc_vout_mode_lock);
			queue_delayed_work(charger->wqueue,
				&charger->wpc_vout_mode_work, 0);
		} else if (val->intval == WIRELESS_VOUT_5V ||
				val->intval == WIRELESS_VOUT_5V_STEP ||
				val->intval == WIRELESS_VOUT_5_5V_STEP) {
			int def_delay = 0;
			charger->vout_mode = val->intval;
			if (is_hv_wireless_pad_type(charger->pdata->cable_type)) {
				def_delay = 250;
			}
			cancel_delayed_work(&charger->wpc_vout_mode_work);
			wake_lock(&charger->wpc_vout_mode_lock);
			queue_delayed_work(charger->wqueue,
				&charger->wpc_vout_mode_work, msecs_to_jiffies(def_delay));
		} else if (val->intval == WIRELESS_VOUT_9V ||
			val->intval == WIRELESS_VOUT_10V ||
			val->intval == WIRELESS_VOUT_11V ||			
			val->intval == WIRELESS_VOUT_12V ||
			val->intval == WIRELESS_VOUT_12_5V ||
			val->intval == WIRELESS_VOUT_9V_STEP ||
			val->intval == WIRELESS_VOUT_10V_STEP) {
			if (!charger->is_full_status && !is_shutdn) {
				if (!charger->is_afc_tx) {
					u8 cmd = 0;
					pr_info("%s: need to set afc tx before vout control\n", __func__);

					if (charger->pdata->cable_type == SEC_WIRELESS_PAD_WPC_HV_20)
						mfc_send_command(charger, charger->vrect_by_txid);
					else
						mfc_send_command(charger, MFC_AFC_CONF_10V_TX);
					charger->is_afc_tx = true;
					pr_info("%s: is_afc_tx = %d vout read = %d \n",
						__func__, charger->is_afc_tx, mfc_get_adc(charger, MFC_ADC_VOUT));

					/* use all CM FETs for 10V wireless charging */
					/* used when VOUT <= 8.5V OR VOUT > 8.5V AND IOUT >= 320mA */
					mfc_reg_write(charger->client, MFC_RX_COMM_MOD_FET_REG, 0x00);
					mfc_reg_read(charger->client, MFC_RX_COMM_MOD_FET_REG, &cmd);
					pr_info("%s: CM FET setting(0x%x) \n", __func__, cmd);
				}
				charger->vout_mode = val->intval;
				cancel_delayed_work(&charger->wpc_vout_mode_work);
				wake_lock(&charger->wpc_vout_mode_lock);
				queue_delayed_work(charger->wqueue,
					&charger->wpc_vout_mode_work, msecs_to_jiffies(250));
			} else {
				pr_info("%s: block to set high vout level(vs=%s) because full status(%d), shutdn(%d)\n",
					__func__, rx_vout_str[charger->pdata->vout_status], charger->is_full_status, is_shutdn);
			}
		} else if (val->intval == WIRELESS_PAD_FAN_OFF) {
			pr_info("%s: fan off\n", __func__);
			mfc_fan_control(charger, 0);
		} else if (val->intval == WIRELESS_PAD_FAN_ON) {
			pr_info("%s: fan on\n", __func__);
			mfc_fan_control(charger, 1);
		} else if (val->intval == WIRELESS_PAD_LED_OFF) {
			pr_info("%s: led off\n", __func__);
			mfc_led_control(charger, MFC_LED_CONTROL_OFF);
		} else if (val->intval == WIRELESS_PAD_LED_ON) {
			pr_info("%s: led on\n", __func__);
			mfc_led_control(charger, MFC_LED_CONTROL_ON);
		} else if (val->intval == WIRELESS_PAD_LED_DIMMING) {
			pr_info("%s: led dimming\n", __func__);
			mfc_led_control(charger, MFC_LED_CONTROL_DIMMING);
		} else if (val->intval == WIRELESS_VRECT_ADJ_ON) {
			pr_info("%s: vrect adjust to have big headroom(default value)\n", __func__);
			mfc_set_vrect_adjust(charger, MFC_HEADROOM_1);
		} else if (val->intval == WIRELESS_VRECT_ADJ_OFF) {
			pr_info("%s: vrect adjust to have small headroom\n", __func__);
			mfc_set_vrect_adjust(charger, MFC_HEADROOM_0);
		} else if (val->intval == WIRELESS_VRECT_ADJ_ROOM_0) {
			pr_info("%s: vrect adjust to have headroom 0(0mV)\n", __func__);
			mfc_set_vrect_adjust(charger, MFC_HEADROOM_0);
		} else if (val->intval == WIRELESS_VRECT_ADJ_ROOM_1) {
			pr_info("%s: vrect adjust to have headroom 1(277mV)\n", __func__);
			mfc_set_vrect_adjust(charger, MFC_HEADROOM_1);
		} else if (val->intval == WIRELESS_VRECT_ADJ_ROOM_2) {
			pr_info("%s: vrect adjust to have headroom 2(497mV)\n", __func__);
			mfc_set_vrect_adjust(charger, MFC_HEADROOM_2);
		} else if (val->intval == WIRELESS_VRECT_ADJ_ROOM_3) {
			pr_info("%s: vrect adjust to have headroom 3(650mV)\n", __func__);
			mfc_set_vrect_adjust(charger, MFC_HEADROOM_3);
		} else if (val->intval == WIRELESS_VRECT_ADJ_ROOM_4) {
			pr_info("%s: vrect adjust to have headroom 4(30mV)\n", __func__);
			mfc_set_vrect_adjust(charger, MFC_HEADROOM_4);
		} else if (val->intval == WIRELESS_VRECT_ADJ_ROOM_5) {
			pr_info("%s: vrect adjust to have headroom 5(82mV)\n", __func__);
			mfc_set_vrect_adjust(charger, MFC_HEADROOM_5);
		} else if (val->intval == WIRELESS_CLAMP_ENABLE) {
			pr_info("%s: enable clamp1, clamp2 for WPC modulation\n", __func__);
			//default enabled state. no need to config.
			//mfc_reg_update(charger->client, MFC_RX_COMM_MOD_FET_REG, 0x00, 0x00);
		} else if (val->intval == WIRELESS_SLEEP_MODE_ENABLE) {
			if (is_sleep_mode_active(charger->tx_id)) {
				pr_info("%s: sleep_mode enable\n", __func__);

				pr_info("%s: led dimming\n", __func__);
				mfc_led_control(charger, MFC_LED_CONTROL_DIMMING);

				pr_info("%s: fan off\n", __func__);
				mfc_fan_control(charger, 0);
			} else {
				pr_info("%s: sleep_mode inactive\n", __func__);
			}
		} else if (val->intval == WIRELESS_SLEEP_MODE_DISABLE) {
			if (is_sleep_mode_active(charger->tx_id)) {
				pr_info("%s: sleep_mode disable\n", __func__);

				pr_info("%s: led on\n", __func__);
				mfc_led_control(charger, MFC_LED_CONTROL_ON);

				pr_info("%s: fan on\n", __func__);
				mfc_fan_control(charger, 1);
			} else {
				pr_info("%s: sleep_mode inactive\n", __func__);
			}
		} else {
			pr_info("%s: Unknown Command(%d)\n", __func__, val->intval);
		}
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		charger->pdata->otp_firmware_result = val->intval;
		pr_info("%s: otp_firmware result initialize (%d)\n", __func__,
			charger->pdata->otp_firmware_result);
		break;
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	case POWER_SUPPLY_PROP_POWER_DESIGN:
		mfc_chg_parse_dt(charger->dev, charger->pdata);
		break;
#endif
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		charger->pdata->capacity = val->intval;
		if (charger->wc_tx_enable) {
			vout = mfc_get_adc(charger, MFC_ADC_TX_VOUT);
			iout = mfc_get_adc(charger, MFC_ADC_TX_IOUT);
			freq = mfc_get_adc(charger, MFC_ADC_OP_FRQ);
			tx_freq = mfc_get_adc(charger, MFC_ADC_OP_FRQ);
			pr_info("@Tx_Mode %s Firmware Ver = %x TX_VOUT = %dmV, TX_IOUT = %dmA, OP_FREQ = %dKHz, TX_OP_FREQ = %dKHZ, %s connected\n",
				__func__, charger->pdata->otp_firmware_ver, vout, iout, freq, tx_freq, rx_device_type_str[charger->wc_rx_type]);
		} else if (charger->pdata->cable_type != SEC_WIRELESS_PAD_NONE) {
			vout = mfc_get_adc(charger, MFC_ADC_VOUT);
			vrect = mfc_get_adc(charger, MFC_ADC_VRECT);
			iout = mfc_get_adc(charger, MFC_ADC_RX_IOUT);
			freq = mfc_get_adc(charger, MFC_ADC_OP_FRQ);
			pr_info("%s Firmware Ver = %x, RX_VOUT = %dmV, RX_VRECT = %dmV, RX_IOUT = %dmA, OP_FREQ = %dKHz, IC Rev = 0x%x, cable=%d\n",
				__func__, charger->pdata->otp_firmware_ver, vout, vrect, iout, freq,
				charger->pdata->wc_ic_rev, charger->pdata->cable_type);

			if ((vout < 6500) && (charger->pdata->capacity >= 85)) {
				mfc_reg_read(charger->client, MFC_RX_COMM_MOD_FET_REG, &tmp);
				if (tmp != 0x00) {
					/* use all CM FETs for 5V wireless charging */
					/* used when VOUT <= 8.5V OR VOUT > 8.5V AND IOUT >= 320mA */
					mfc_reg_write(charger->client, MFC_RX_COMM_MOD_FET_REG, 0x00);
					mfc_reg_read(charger->client, MFC_RX_COMM_MOD_FET_REG, &tmp);
					pr_info("%s: CM FET setting(0x%x)\n", __func__, tmp);
				}
			}

			for (i = 0; i < MFC_NUM_FOD_REG; i++)
				mfc_reg_read(charger->client, MFC_WPC_FOD_0A_REG+i, &fod[i]);
			pr_info("%s: FOD(0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X"
				"0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X"
				"0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X)\n", __func__,
				fod[0], fod[1], fod[2], fod[3], fod[4], fod[5], fod[6],
				fod[7], fod[8], fod[9], fod[10], fod[11], fod[12], fod[13],
				fod[14], fod[15], fod[16], fod[17], fod[18], fod[19]);
		}
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		charger->pdata->capacity = val->intval;
		if (charger->wc_rx_type == SS_GEAR) {
			mfc_check_tx_gear_time(charger);
		}
		break;
	case POWER_SUPPLY_PROP_FILTER_CFG:
		charger->led_cover = val->intval;
		pr_info("%s: LED_COVER(%d)\n", __func__, charger->led_cover);
		break;
	case POWER_SUPPLY_PROP_CHARGE_EMPTY:
	{
		int vout = 0, vrect = 0;
		u8 is_vout_on = 0;

		mfc_reg_read(charger->client, MFC_STATUS_L_REG, &is_vout_on);
		is_vout_on = is_vout_on >> 7;
		vout = mfc_get_adc(charger, MFC_ADC_VOUT);
		vrect = mfc_get_adc(charger, MFC_ADC_VRECT);
		pr_info("%s: SET MFC LDO (%s), Current VOUT STAT (%d), RX_VOUT = %dmV, RX_VRECT = %dmV\n", __func__, (val->intval == MFC_LDO_ON ? "ON" : "OFF"), is_vout_on, vout, vrect);
		if ((val->intval == MFC_LDO_ON) && !is_vout_on) { /* LDO ON */
			pr_info("%s: MFC LDO ON toggle ------------ cable_work\n", __func__);
			mfc_set_cmd_l_reg(charger, MFC_CMD_TOGGLE_LDO_MASK, MFC_CMD_TOGGLE_LDO_MASK);
			msleep(400);				
			mfc_reg_read(charger->client, MFC_STATUS_L_REG, &is_vout_on);
			is_vout_on = is_vout_on >> 7;
			vout = mfc_get_adc(charger, MFC_ADC_VOUT);
			vrect = mfc_get_adc(charger, MFC_ADC_VRECT);
			pr_info("%s: LDO Should ON -> MFC LDO STAT(%d), RX_VOUT = %dmV, RX_VRECT = %dmV\n", __func__, is_vout_on, vout, vrect);				
			charger->wc_ldo_status = MFC_LDO_ON;
		} else if ((val->intval == MFC_LDO_OFF) && is_vout_on) { /* LDO OFF */
			pr_info("%s: MFC LDO OFF toggle ------------ cable_work\n", __func__);
			mfc_set_cmd_l_reg(charger, MFC_CMD_TOGGLE_LDO_MASK, MFC_CMD_TOGGLE_LDO_MASK);
			msleep(400);
			mfc_reg_read(charger->client, MFC_STATUS_L_REG, &is_vout_on);
			is_vout_on = is_vout_on >> 7;
			vout = mfc_get_adc(charger, MFC_ADC_VOUT);
			vrect = mfc_get_adc(charger, MFC_ADC_VRECT);
			pr_info("%s: LDO Should OFF -> MFC LDO STAT(%d), RX_VOUT = %dmV, RX_VRECT = %dmV\n", __func__, is_vout_on, vout, vrect);
			charger->wc_ldo_status = MFC_LDO_OFF;
		}
	}
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		return -ENODATA;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_WC_CONTROL:
			if (val->intval == 0) {
				tmp = 0x01;
				mfc_send_packet(charger, MFC_HEADER_AFC_CONF,
					0x20, &tmp, 1);
				pr_info("%s: send command after wc control\n", __func__);
				msleep(150);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_SWITCH:
			/* It is a RX device , send a packet to TX device to stop power sharing.
				TX device will have MFC_INTA_H_TRX_DATA_RECEIVED_MASK irq */
			if (charger->pdata->cable_type == SEC_WIRELESS_PAD_TX) {
				if (val->intval) {
					pr_info("%s: It is a RX device , send a packet to TX device to stop power sharing\n", __func__);
					mfc_send_command(charger, MFC_DISABLE_TX);
				}
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_SEND_FSK:
			/* send fsk packet for rx device aicl reset */
			if (val->intval && (charger->wc_rx_type != SS_GEAR)) {
				pr_info("@Tx_mode %s: Send FSK packet for Rx device aicl reset\n", __func__);
				mfc_send_fsk(charger, WPC_TX_COM_WPS, WPS_AICL_RESET, NULL, 0);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ENABLE:
			/* on/off tx power */
			mfc_set_tx_power(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONNECTED:
			mfc_tx_check_rx_connection(charger, val->intval);			
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS: /* it has only PASS and FAIL */
#if !defined(CONFIG_SEC_FACTORY)
			if((charger->pdata->cable_type != SEC_WIRELESS_PAD_NONE) &&
				!lpcharge &&
				(charger->adt_transfer_status != WIRELESS_AUTH_WAIT)) {
				if(charger->adt_transfer_status != val->intval) {
					charger->adt_transfer_status = val->intval;

					if(charger->adt_transfer_status == WIRELESS_AUTH_PASS) {
						/* this cable type will call a proper RX power having done vout_work */
						//charger->pdata->cable_type = SEC_WIRELESS_PAD_WPC_HV_20_LIMIT;
						charger->pdata->cable_type = SEC_WIRELESS_PAD_WPC_HV_20;
						pr_info("%s %s: type = %d \n", WC_AUTH_MSG, __func__, charger->pdata->cable_type);
						pr_info("%s %s: pass\n", WC_AUTH_MSG, __func__);
					} else if (charger->adt_transfer_status == WIRELESS_AUTH_FAIL) {
						/* when auth is failed, cable type is HV wireless */
						charger->pdata->cable_type = SEC_WIRELESS_PAD_WPC_HV;
						pr_info("%s %s: fail\n", WC_AUTH_MSG, __func__);
					} else
						pr_info("%s %s: undefined pass/fail result\n", WC_AUTH_MSG, __func__);
					wake_lock(&charger->wpc_afc_vout_lock);
					queue_delayed_work(charger->wqueue, &charger->wpc_afc_vout_work, msecs_to_jiffies(0));
				} else
					pr_info("%s %s: skip a same pass/fail result\n", WC_AUTH_MSG, __func__);
			}else
				pr_info("%s %s: auth service sent wrong cmd \n", WC_AUTH_MSG, __func__);
#endif
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_DATA: /* data from auth service will be sent */
			if(charger->pdata->cable_type != SEC_WIRELESS_PAD_NONE && !lpcharge) {
				u8 *p_data;
				int i;
				p_data = (u8 *)val->strval;

				for (i = 0 ; i < adt_readSize ; i++)
					pr_info("%s %s : p_data[%d] = %x\n", WC_AUTH_MSG, __func__, i, p_data[i]);
				mfc_auth_adt_send(charger, p_data, adt_readSize);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_SIZE:
			if(charger->pdata->cable_type != SEC_WIRELESS_PAD_NONE)
				adt_readSize = val->intval;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_TYPE:
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT:
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_IOUT:
			mfc_set_tx_iout(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TIMER_ON:
			pr_info("%s %s : TX receiver detecting timer enable(%d)\n", WC_AUTH_MSG, __func__, val->intval);
			if (charger->wc_tx_enable) {
				if (val->intval) {
					pr_info("%s %s : enable TX OFF timer (90sec)", WC_AUTH_MSG, __func__);
					mfc_reg_update(charger->client, MFC_INT_A_ENABLE_H_REG, (0x1<<5), (0x1<<5));
				} else {
					pr_info("%s %s : disable TX OFF timer (90sec)", WC_AUTH_MSG, __func__);
					mfc_reg_update(charger->client, MFC_INT_A_ENABLE_H_REG, 0x0, (0x1<<5));
				}
			} else
				pr_info("%s %s : Don't need to set TX 90sec timer, on TX OFF state\n", WC_AUTH_MSG, __func__);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_MIN_DUTY:
			cancel_delayed_work(&charger->wpc_tx_op_freq_work);
			if (val->intval == 50) {
				mfc_set_tx_op_freq(charger, 1100);
			} else {
				mfc_set_tx_op_freq(charger, 1480);
			}
			mfc_set_min_duty(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_CALL_EVENT:
			if (val->intval & BATT_EXT_EVENT_CALL) {
				charger->device_event |= BATT_EXT_EVENT_CALL;

				/* call in is after wireless connection */
				if(charger->pdata->cable_type == SEC_WIRELESS_PAD_WPC_PACK ||
					charger->pdata->cable_type == SEC_WIRELESS_PAD_WPC_PACK_HV ||
					charger->pdata->cable_type == SEC_WIRELESS_PAD_TX) {
					union power_supply_propval value2;
					pr_info("%s %s : enter PHM \n", WC_TX_MSG, __func__);
					/* notify "wireless" PHM status */
					value2.intval = 1;
					psy_do_property("wireless", set,
						POWER_SUPPLY_EXT_PROP_CALL_EVENT, value2);
					mfc_send_command(charger, MFC_PHM_ON);
					msleep(250);
					mfc_send_command(charger, MFC_PHM_ON);
				}
			} else if (val->intval == BATT_EXT_EVENT_NONE) {
				charger->device_event &= ~BATT_EXT_EVENT_CALL;
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WPC_EN:
			mfc_set_wpc_en(charger, val->strval[0], val->strval[1]);
			break;
		case POWER_SUPPLY_EXT_PROP_WPC_EN_MST:
			if (val->intval)
				mfc_set_wpc_en(charger, WPC_EN_MST, true);
			else
				mfc_set_wpc_en(charger, WPC_EN_MST, false);
			break;
#if defined(CONFIG_WIRELESS_IC_PARAM)
		case POWER_SUPPLY_EXT_PROP_WIRELESS_PARAM_INFO:
			mfc_set_wireless_ic_param(charger, charger->chip_id_now, charger->pdata->otp_firmware_ver);
			break;
#endif
		default:
			return -ENODATA;
		}
		break;
	default:
		return -ENODATA;
	}

	return 0;
}

#define FREQ_OFFSET	384000 /* 64*6000 */
static void mfc_wpc_opfq_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_opfq_work.work);
#if 0
	u16 op_fq;
	u8 pad_mode;
	union power_supply_propval value;

	mfc_reg_read(charger->client, MFC_SYS_OP_MODE_REG, &pad_mode);
	if ((pad_mode == PAD_MODE_WPC_BASIC) ||\
			(pad_mode == PAD_MODE_WPC_ADV)) {
		op_fq = mfc_get_adc(charger, MFC_ADC_OP_FRQ);
		pr_info("%s: Operating FQ %dkHz(0x%x)\n", __func__, op_fq, op_fq);
		if (op_fq > 230) { /* wpc threshold 230kHz */
			pr_info("%s: Reset M0\n", __func__);
			mfc_reg_write(charger->client, 0x3040, 0x80); /*restart M0 */

			charger->pdata->opfq_cnt++;
			if (charger->pdata->opfq_cnt <= CMD_CNT) {
				queue_delayed_work(charger->wqueue, &charger->wpc_opfq_work, msecs_to_jiffies(10000));
				return;
			}
		}
	} else if ((pad_mode == PAD_MODE_PMA_SR1) ||\
			(pad_mode == PAD_MODE_PMA_SR1E)) {
			charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_PMA;
			psy_do_property("wireless", set, POWER_SUPPLY_PROP_ONLINE, value);
	}
	charger->pdata->opfq_cnt = 0;
#endif	
	wake_unlock(&charger->wpc_opfq_lock);

}

static u32 mfc_get_wireless20_vout_by_txid(struct mfc_charger_data *charger, u8 txid)
{
	u32 vout = 0, i = 0;

	if(txid < TX_ID_AUTH_PAD || txid > TX_ID_AUTH_PAD_END) {
		pr_info("%s : wrong tx id(0x%x) for wireless 2.0\n", __func__, txid);
		i = 0; /* defalut value is WIRELESS_VOUT_10V */
	} else if(txid >= TX_ID_AUTH_PAD + charger->pdata->len_wc20_list) {
		pr_info("%s : undefined tx id(0x%x) of the device\n", __func__, txid);
		i = charger->pdata->len_wc20_list-1;
	} else
		i = txid - TX_ID_AUTH_PAD;

	vout = charger->pdata->wireless20_vout_list[i];
	pr_info("%s vout = %d\n", __func__, vout);
	return vout;
}

static u32 mfc_get_wireless20_vrect_by_txid(struct mfc_charger_data *charger, u8 txid)
{
	u32 vrect = 0, i = 0;

	if(txid < TX_ID_AUTH_PAD || txid > TX_ID_AUTH_PAD_END) {
		pr_info("%s : wrong tx id(0x%x) for wireless 2.0\n", __func__, txid);
		i = 0; /* defalut value is MFC_AFC_CONF_10V_TX */
	} else if(txid >= TX_ID_AUTH_PAD + charger->pdata->len_wc20_list) {
		pr_info("%s : undefined tx id(0x%x) of the device\n", __func__, txid);
		i = charger->pdata->len_wc20_list-1;
	} else
		i = txid - TX_ID_AUTH_PAD;

	vrect = charger->pdata->wireless20_vrect_list[i];
	pr_info("%s vrect = %d\n", __func__, vrect);
	return vrect;
}

static u32 mfc_get_wireless20_max_power_by_txid(struct mfc_charger_data *charger, u8 txid)
{
	u32 max_power = 0, i = 0;

	if(txid < TX_ID_AUTH_PAD || txid > TX_ID_AUTH_PAD_END) {
		pr_info("%s : wrong tx id(0x%x) for wireless 2.0\n", __func__, txid);
		i = 0; /* defalut value is SEC_WIRELESS_RX_POWER_12W */
	} else if(txid >= TX_ID_AUTH_PAD + charger->pdata->len_wc20_list) {
		pr_info("%s : undefined tx id(0x%x) of the device\n", __func__, txid);
		i = charger->pdata->len_wc20_list-1;
	} else
		i = txid - TX_ID_AUTH_PAD;

	max_power = charger->pdata->wireless20_max_power_list[i];
	pr_info("%s max rx power = %d\n", __func__, max_power);
	return max_power;
}

static void mfc_wpc_det_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_det_work.work);
	int wc_w_state;
	union power_supply_propval value;
	u8 pad_mode;
	u8 vrect;

	mfc_get_chip_id(charger);
	pr_info("%s : start\n", __func__);

	/*
	* We don't have to handle the wpc detect handling,
	* when it's the MST mode.
	*/
	if (charger->is_mst_on == MST_MODE_2) {
		pr_info("%s MST RETURN!\n", __func__);
		return;
	}

	if (charger->is_mst_on == MST_MODE_2) {
		pr_info("%s: check wpc-state(%d - %d)\n", __func__,
			charger->wc_w_state, gpio_get_value(charger->pdata->wpc_det));

		if (charger->wc_w_state == 0) {
			pr_info("%s: skip wpc_det_work for MST operation\n", __func__);
			return;
		}
	}

	wake_lock(&charger->wpc_wake_lock);
	pr_info("%s\n", __func__);
	wc_w_state = gpio_get_value(charger->pdata->wpc_det);

	if ((charger->wc_w_state == 0) && (wc_w_state == 1)) {
		charger->initial_wc_check = true;

		charger->pdata->otp_firmware_ver = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);
		charger->pdata->wc_ic_rev = mfc_get_ic_revision(charger, MFC_IC_REVISION);

		if (charger->is_otg_on)
			charger->pdata->vout_status = MFC_VOUT_5V;
		else
			charger->pdata->vout_status = MFC_VOUT_5_5V;
		mfc_set_vout(charger, charger->pdata->vout_status);

		charger->wc_tx_enable = false;
		charger->gear_start_time = 0;

		/* enable Mode Change INT */
		mfc_reg_update(charger->client, MFC_INT_A_ENABLE_L_REG,
						MFC_STAT_L_OP_MODE_MASK, MFC_STAT_L_OP_MODE_MASK);

		mfc_reg_update(charger->client, MFC_INT_A_ENABLE_L_REG,
						MFC_STAT_H_ADT_RECEIVED_MASK, MFC_STAT_H_ADT_RECEIVED_MASK);

		/* read vrect adjust */
		mfc_reg_read(charger->client, MFC_VRECT_ADJ_REG, &vrect);

		pr_info("%s: wireless charger activated, set V_INT as PN\n", __func__);

		/* default value of CMA and CMB all enable
			use all CM FETs for wireless charging */
#if 0 //S2MIW04 not support
		mfc_reg_write(charger->client, MFC_RX_COMM_MOD_AFC_FET_REG, 0x00);
#endif
		mfc_reg_write(charger->client, MFC_RX_COMM_MOD_FET_REG, 0x30);

		/* read pad mode */
		mfc_reg_read(charger->client, MFC_SYS_OP_MODE_REG, &pad_mode);
		pad_mode = pad_mode >> 5;
		pr_info("%s: Pad type (0x%x)\n", __func__, pad_mode);
		if ((pad_mode == PAD_MODE_PMA_SR1) ||
			(pad_mode == PAD_MODE_PMA_SR1E)) {
			charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_PMA;
			psy_do_property("wireless", set,
					POWER_SUPPLY_PROP_ONLINE, value);
		} else { /* WPC */
			charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_WPC;
			psy_do_property("wireless", set,
					POWER_SUPPLY_PROP_ONLINE, value);
			wake_lock(&charger->wpc_opfq_lock);
			queue_delayed_work(charger->wqueue, &charger->wpc_opfq_work, msecs_to_jiffies(10000));
		}

		/* set fod value */
		if (charger->pdata->fod_data_check)
			mfc_fod_set(charger);
#if !defined(CONFIG_WIRELESS_NO_HV)
		if (!sleep_mode) {
			int vrect_level, vout_level;

			vrect_level = mfc_get_adc(charger, MFC_ADC_VRECT);
			vout_level = mfc_get_adc(charger, MFC_ADC_VOUT);
			pr_info("%s: read vrect(%dmV), vout(%dmV)\n", __func__, vrect_level, vout_level);
			if (vrect_level >= 8500 && vout_level >= 8500) {
				/* re-set vout level */
				charger->pad_vout = PAD_VOUT_10V;
				mfc_set_vout(charger, MFC_VOUT_10V);

				/* change cable type */
				charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_WPC_HV;
				psy_do_property("wireless", set,
					POWER_SUPPLY_PROP_ONLINE, value);
				wake_lock(&charger->wpc_tx_id_lock);
				queue_delayed_work(charger->wqueue, &charger->wpc_tx_id_work, msecs_to_jiffies(500));
			} else {
				/* send request afc_tx */
				msleep(200);
				mfc_send_command(charger, MFC_REQUEST_AFC_TX);
			}
		} else {
			wake_lock(&charger->wpc_tx_id_lock);
			queue_delayed_work(charger->wqueue, &charger->wpc_tx_id_work, msecs_to_jiffies(500));
		}
#endif
		/* set rpp scaling factor for LED cover */
		mfc_rpp_set(charger);
#if 0
		/* set request TX_ID */
		mfc_send_command(charger, MFC_REQUEST_TX_ID);
#endif
		charger->pdata->is_charging = 1;
	} else if ((charger->wc_w_state == 1) && (wc_w_state == 0)) {
		/* Send last tx_id to battery to cound tx_id */ 
		value.intval = charger->tx_id;
		psy_do_property("wireless", set, POWER_SUPPLY_PROP_AUTHENTIC, value);
		charger->pdata->cable_type = SEC_WIRELESS_PAD_NONE;
		charger->pdata->is_charging = 0;
		charger->pdata->vout_status = MFC_VOUT_5V;
		charger->pad_vout = PAD_VOUT_5V;
		charger->pdata->opfq_cnt = 0;
		charger->pdata->trx_data_cmd = 0;
		charger->pdata->trx_data_val = 0;
		charger->vout_mode = 0;
		charger->is_full_status = 0;
		charger->pdata->capacity = 101;
		charger->is_afc_tx = false;
		charger->tx_id = TX_ID_UNKNOWN;
		charger->i2c_error_count = 0;
		charger->non_sleep_mode_cnt = 0;
		charger->adt_transfer_status = WIRELESS_AUTH_WAIT;
		charger->current_rx_power = TX_RX_POWER_0W;
		charger->tx_id_cnt = 0;
		charger->wc_ldo_status = MFC_LDO_ON;
		charger->tx_id_done = false;

		value.intval = SEC_WIRELESS_PAD_NONE;
		psy_do_property("wireless", set,
				POWER_SUPPLY_PROP_ONLINE, value);
		pr_info("%s: wpc deactivated, set V_INT as PD\n", __func__);

		if (delayed_work_pending(&charger->wpc_opfq_work)) {
			wake_unlock(&charger->wpc_opfq_lock);
			cancel_delayed_work(&charger->wpc_opfq_work);
		}
		if (delayed_work_pending(&charger->wpc_afc_vout_work)) {
			wake_unlock(&charger->wpc_afc_vout_lock);
			cancel_delayed_work(&charger->wpc_afc_vout_work);
		}
		if (delayed_work_pending(&charger->wpc_vout_mode_work)) {
			wake_unlock(&charger->wpc_vout_mode_lock);
			cancel_delayed_work(&charger->wpc_vout_mode_work);
		}

		cancel_delayed_work(&charger->wpc_isr_work);
		cancel_delayed_work(&charger->wpc_tx_isr_work);
		cancel_delayed_work(&charger->wpc_tx_id_work);
		cancel_delayed_work(&charger->wpc_i2c_error_work);
		wake_unlock(&charger->wpc_rx_wake_lock);
		wake_unlock(&charger->wpc_tx_wake_lock);
		wake_unlock(&charger->wpc_tx_id_lock);
	}

	pr_info("%s: w(%d to %d)\n", __func__,
		charger->wc_w_state, wc_w_state);

	charger->wc_w_state = wc_w_state;
	wake_unlock(&charger->wpc_wake_lock);
}

/* INT_A */
static void mfc_wpc_tx_isr_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_tx_isr_work.work);

	u8 status_l = 0, status_h = 0;
	int ret = 0;

	pr_info("@Tx_Mode %s\n", __func__);

	if (!charger->wc_tx_enable) {
		wake_unlock(&charger->wpc_tx_wake_lock);
		return;
	}

	ret = mfc_reg_read(charger->client, MFC_STATUS_L_REG, &status_l);
	ret = mfc_reg_read(charger->client, MFC_STATUS_H_REG, &status_h);

	pr_info("@Tx_Mode %s: status(0x%x) \n", __func__, status_h << 8 | status_l);

	pr_info("@Tx_Mode %s DATA RECEIVED\n", __func__);
	mfc_tx_handle_rx_packet(charger);

	wake_unlock(&charger->wpc_tx_wake_lock);
}

/* INT_A */
static void mfc_wpc_isr_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_isr_work.work);

	u8 cmd_data, val_data;

	union power_supply_propval value;

	if (!charger->wc_w_state) {
		pr_info("%s: charger->wc_w_state is 0. exit wpc_isr_work.\n", __func__);
		wake_unlock(&charger->wpc_rx_wake_lock);
		return;
	}

	pr_info("%s: cable_type (0x%x)\n", __func__, charger->pdata->cable_type);
	pr_info("%s\n", __func__);

	mfc_reg_read(charger->client, MFC_WPC_TRX_DATA_COM_REG, &cmd_data);
	mfc_reg_read(charger->client, MFC_WPC_TRX_DATA_VALUE0_REG, &val_data);
	charger->pdata->trx_data_cmd = cmd_data;
	charger->pdata->trx_data_val = val_data;

	pr_info("%s: WPC Interrupt Occured, CMD : 0x%x, DATA : 0x%x\n",
		__func__, cmd_data, val_data);

	/* None or RX mode */
	if (cmd_data == WPC_TX_COM_AFC_SET) {
		switch (val_data) {
		case TX_AFC_SET_5V:
			pr_info("%s data = 0x%x, might be 5V irq\n", __func__, val_data);
			charger->pad_vout = PAD_VOUT_5V;
			break;
		case TX_AFC_SET_10V:
			pr_info("%s data = 0x%x, might be 10V irq\n", __func__, val_data);
			if (!gpio_get_value(charger->pdata->wpc_det)) {
				pr_err("%s Wireless charging is paused during set high voltage.\n", __func__);
				wake_unlock(&charger->wpc_rx_wake_lock);
				return;
			} else if (sleep_mode) {
				if(charger->non_sleep_mode_cnt >= 2)
					pr_info("%s: will have 10V at sleep_mode, cnt = %d \n", __func__, charger->non_sleep_mode_cnt);
				else {
					charger->non_sleep_mode_cnt++;
					pr_info("%s: does not needs to check afc mode at sleep_mode, cnt = %d \n", __func__, charger->non_sleep_mode_cnt);
					charger->pad_vout = PAD_VOUT_5V;
					break;
				}
			}
#if !defined(CONFIG_WIRELESS_NO_HV)
			if (is_hv_wireless_pad_type(charger->pdata->cable_type) ||
				charger->pdata->cable_type == SEC_WIRELESS_PAD_PREPARE_HV) {
				pr_err("%s: Is is already HV wireless cable. No need to set again\n", __func__);
				wake_unlock(&charger->wpc_rx_wake_lock);
				return;
			}

			/* send AFC_SET */
			mfc_send_command(charger, MFC_AFC_CONF_10V);
			msleep(500);

			/* change cable type */
			charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_PREPARE_HV;
			psy_do_property("wireless", set,
				POWER_SUPPLY_PROP_ONLINE, value);

			charger->pad_vout = PAD_VOUT_10V;
#endif
			break;
		case TX_AFC_SET_12V:
			break;
		case TX_AFC_SET_18V:
		case TX_AFC_SET_19V:
		case TX_AFC_SET_20V:
		case TX_AFC_SET_24V:
			break;
		default:
			pr_info("%s: unsupport : 0x%x", __func__, val_data);
			break;
		}

		wake_lock(&charger->wpc_tx_id_lock);
		queue_delayed_work(charger->wqueue, &charger->wpc_tx_id_work, msecs_to_jiffies(500));
	} else if (cmd_data == WPC_TX_COM_TX_ID) {
		if(!charger->tx_id_done) {
			switch (val_data) {
			case TX_ID_UNKNOWN:
			break;
			case TX_ID_VEHICLE_PAD:
				if (charger->pad_vout == PAD_VOUT_10V) {
					if (charger->pdata->cable_type == SEC_WIRELESS_PAD_PREPARE_HV) {
						charger->pdata->cable_type = SEC_WIRELESS_PAD_VEHICLE_HV;
						value.intval = SEC_WIRELESS_PAD_PREPARE_HV;
					} else {
						charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_VEHICLE_HV;
					}
				} else {
					charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_VEHICLE;
				}
				pr_info("%s: VEHICLE Wireless Charge PAD %s\n", __func__,
					charger->pad_vout == PAD_VOUT_10V ? "HV" : "");

				break;
			case TX_ID_STAND_TYPE_START:
				if (charger->pad_vout == PAD_VOUT_10V) {
					if (charger->pdata->cable_type == SEC_WIRELESS_PAD_PREPARE_HV) {
						charger->pdata->cable_type = SEC_WIRELESS_PAD_WPC_STAND_HV;
						value.intval = SEC_WIRELESS_PAD_PREPARE_HV;
					} else {
						charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_WPC_STAND_HV;
					}
				} else {
					charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_WPC_STAND;
					//mfc_fod_set_hero_5v(charger);
				}
				pr_info("%s: STAND Wireless Charge PAD %s\n", __func__,
					charger->pad_vout == PAD_VOUT_10V ? "HV" : "");
				pr_info("%s: cable_type(%d)\n", __func__, charger->pdata->cable_type);
				break;
			case TX_ID_MULTI_PORT_START ... TX_ID_MULTI_PORT_END:
				value.intval = charger->pdata->cable_type;
				pr_info("%s: MULTI PORT PAD : 0x%x\n", __func__, val_data);
				break;
			case TX_ID_BATT_PACK ... TX_ID_BATT_PACK_END:
				if (charger->pad_vout == PAD_VOUT_10V) {
					if (charger->pdata->cable_type == SEC_WIRELESS_PAD_PREPARE_HV) {
						charger->pdata->cable_type = SEC_WIRELESS_PAD_WPC_PACK_HV;
						value.intval = SEC_WIRELESS_PAD_PREPARE_HV;
						pr_info("%s: WIRELESS HV BATTERY PACK (PREP) \n", __func__);
					} else {
						charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_WPC_PACK_HV;
						pr_info("%s: WIRELESS HV BATTERY PACK\n", __func__);
					}
				} else {
					charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_WPC_PACK;
					pr_info("%s: WIRELESS BATTERY PACK\n", __func__);
				}
				if (charger->device_event & BATT_EXT_EVENT_CALL) {
					union power_supply_propval value2;
					pr_info("%s: enter PHM \n", __func__);
					/* notify "wireless" PHM status */
					value2.intval = 1;
					psy_do_property("wireless", set,
						POWER_SUPPLY_EXT_PROP_CALL_EVENT, value2);
					mfc_send_command(charger, MFC_PHM_ON);
					msleep(250);
					mfc_send_command(charger, MFC_PHM_ON);
				}
				break;
			case TX_ID_UNO_TX:
				charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_TX;
				pr_info("@Tx_Mode %s: TX by UNO\n", __func__);
				if (charger->device_event & BATT_EXT_EVENT_CALL) {
					pr_info("%s: enter PHM \n", __func__);
					mfc_send_command(charger, MFC_PHM_ON);
					msleep(250);
					mfc_send_command(charger, MFC_PHM_ON);
				}
				break;
			case TX_ID_AUTH_PAD ... TX_ID_AUTH_PAD_END:
				charger->vout_by_txid = mfc_get_wireless20_vout_by_txid(charger, val_data);
				charger->vrect_by_txid = mfc_get_wireless20_vrect_by_txid(charger, val_data);
				charger->max_power_by_txid = mfc_get_wireless20_max_power_by_txid(charger, val_data);
#if !defined(CONFIG_SEC_FACTORY)
				/* do not process during lpm and wired charging */
				if(!lpcharge) {
					if(charger->adt_transfer_status == WIRELESS_AUTH_WAIT) {
						if (delayed_work_pending(&charger->wpc_afc_vout_work)) {
							wake_unlock(&charger->wpc_afc_vout_lock);
							cancel_delayed_work(&charger->wpc_afc_vout_work);
						}
						charger->adt_transfer_status = WIRELESS_AUTH_START;
						/* notify auth service to send TX PAD a request key */
						value.intval = WIRELESS_AUTH_START;
						psy_do_property("wireless", set,
							POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);

						charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_WPC_PREPARE_HV_20;
						pr_info("%s %s : AUTH PAD for WIRELESS 2.0 : 0x%x\n", WC_AUTH_MSG, __func__, val_data);
					} else {
						value.intval = charger->pdata->cable_type;
						pr_info("%s %s : undefined auth TX-ID scenario, auth service works strange...\n", WC_AUTH_MSG, __func__);
					}
				} else {
					value.intval = charger->pdata->cable_type;
					pr_info("%s %s : It is AUTH PAD for WIRELESS 2.0 but LPM, %d\n", WC_AUTH_MSG, __func__, charger->pdata->cable_type);
				}
#else /* FACTORY MODE dose not process auth, set 12W right after */
				if(charger->adt_transfer_status == WIRELESS_AUTH_WAIT) {
					if (delayed_work_pending(&charger->wpc_afc_vout_work)) {
						wake_unlock(&charger->wpc_afc_vout_lock);
						cancel_delayed_work(&charger->wpc_afc_vout_work);
					}
					charger->adt_transfer_status = WIRELESS_AUTH_PASS;
					charger->pdata->cable_type = value.intval = SEC_WIRELESS_PAD_WPC_HV_20;
					wake_lock(&charger->wpc_afc_vout_lock);
					queue_delayed_work(charger->wqueue, &charger->wpc_afc_vout_work, msecs_to_jiffies(0));
				}
				pr_info("%s %s : AUTH PAD for WIRELESS 2.0 but FACTORY MODE\n", WC_AUTH_MSG, __func__);
#endif
				break;
			case TX_ID_DREAM_STAND:
				pr_info("%s: Dream pad FOD, SOC(%d)\n", __func__, charger->pdata->capacity);
				if (charger->pdata->fod_dream_data) {
					//if (charger->pdata->capacity > 85)
					//	mfc_fod_set_dream_cv(charger);
					//else
					//	mfc_fod_set_dream(charger);
				}
			case TX_ID_DREAM_DOWN:
			default:
				value.intval = charger->pdata->cable_type;
				pr_info("%s: UNDEFINED PAD : 0x%x\n", __func__, val_data);
				break;
			}

			if (value.intval != SEC_WIRELESS_PAD_PREPARE_HV &&
				value.intval != SEC_WIRELESS_PAD_WPC_HV_20) {
				pr_info("%s: change cable type\n", __func__);
				psy_do_property("wireless", set, POWER_SUPPLY_PROP_ONLINE, value);
			}

			/* send rx power informaion of this pad such as SEC_WIRELESS_RX_POWER_12W, SEC_WIRELESS_RX_POWER_17_5W */
			value.intval = charger->max_power_by_txid;
			psy_do_property("wireless", set, POWER_SUPPLY_PROP_WIRELESS_RX_POWER, value);

			/* send max vout informaion of this pad such as WIRELESS_VOUT_10V, WIRELESS_VOUT_12_5V */
			value.intval = charger->vout_by_txid;
			psy_do_property("wireless", set, POWER_SUPPLY_PROP_WIRELESS_MAX_VOUT, value);

			charger->tx_id_done = true;
			charger->tx_id = val_data;
			pr_info("%s: TX_ID : 0x%x\n", __func__, val_data);
		} else {
			pr_err("%s: TX ID isr is already done\n", __func__);
		}
	} else if (cmd_data == WPC_TX_COM_CHG_ERR) {
		switch (val_data) {
		case TX_CHG_ERR_OTP:
		case TX_CHG_ERR_OCP:
		case TX_CHG_ERR_DARKZONE:
			pr_info("%s: Received CHG error from the TX(0x%x)\n",
				__func__, val_data);
			break;
		case TX_CHG_ERR_FOD:
			pr_info("%s: Received FOD state from the TX(0x%x)\n",
				__func__, val_data);
			value.intval = val_data;
			psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_ERR, value);
			break;
		default:
			pr_info("%s: Undefined Type(0x%x)\n", __func__, val_data);
			break;
		}
	} else if (cmd_data == WPC_TX_COM_RX_POWER) {
		if(charger->current_rx_power != val_data) {
			switch (val_data) {
			case TX_RX_POWER_3W:
				pr_info("%s %s : RX Power is 3W\n", WC_AUTH_MSG, __func__);
				charger->current_rx_power = TX_RX_POWER_3W;
				break;
			case TX_RX_POWER_5W:
				pr_info("%s %s : RX Power is 5W\n", WC_AUTH_MSG, __func__);
				charger->current_rx_power = TX_RX_POWER_5W;
				break;
			case TX_RX_POWER_6_5W:
				pr_info("%s %s : RX Power is 6.5W\n", WC_AUTH_MSG, __func__);
				charger->current_rx_power = TX_RX_POWER_6_5W;
				break;
			case TX_RX_POWER_7_5W:
				pr_info("%s %s : RX Power is 7.5W\n", WC_AUTH_MSG, __func__);
				mfc_reset_rx_power(charger, TX_RX_POWER_7_5W);
				charger->current_rx_power = TX_RX_POWER_7_5W;
				break;
			case TX_RX_POWER_10W:
				pr_info("%s %s : RX Power is 10W\n", WC_AUTH_MSG, __func__);
				charger->current_rx_power = TX_RX_POWER_10W;
				break;
			case TX_RX_POWER_12W:
				pr_info("%s %s : RX Power is 12W\n", WC_AUTH_MSG, __func__);
				mfc_reset_rx_power(charger, TX_RX_POWER_12W);
				charger->current_rx_power = TX_RX_POWER_12W;
				break;
			case TX_RX_POWER_15W:
				pr_info("%s %s : RX Power is 15W\n", WC_AUTH_MSG, __func__);
				mfc_reset_rx_power(charger, TX_RX_POWER_15W);
				charger->current_rx_power = TX_RX_POWER_15W;
				break;
			case TX_RX_POWER_17_5W:
				pr_info("%s %s : RX Power is 17.5W\n", WC_AUTH_MSG, __func__);
				mfc_reset_rx_power(charger, TX_RX_POWER_17_5W);
				charger->current_rx_power = TX_RX_POWER_17_5W;
				break;
			case TX_RX_POWER_20W:
				pr_info("%s %s : RX Power is 20W\n", WC_AUTH_MSG, __func__);
				mfc_reset_rx_power(charger, TX_RX_POWER_20W);
				charger->current_rx_power = TX_RX_POWER_20W;
				break;
			default:
				pr_info("%s %s : Undefined RX Power(0x%x)\n", WC_AUTH_MSG, __func__, val_data);
				break;
			}
		}else 
		pr_info("%s: skip same RX Power\n", __func__);
	} else if (cmd_data == WPC_TX_COM_WPS) {
		switch (val_data) {
		case WPS_AICL_RESET:
			value.intval = 1;
			pr_info("@Tx_mode %s Rx devic AICL Reset\n", __func__);
			psy_do_property("wireless", set, POWER_SUPPLY_PROP_CURRENT_MAX, value);
			break;
		default:
			pr_info("%s %s : Undefined RX Power(0x%x)\n", WC_AUTH_MSG, __func__, val_data);
			break;
		}
	}
	wake_unlock(&charger->wpc_rx_wake_lock);
}

static void mfc_wpc_tx_id_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_tx_id_work.work);

	pr_info("%s\n", __func__);

	mfc_send_command(charger, MFC_REQUEST_TX_ID);
	charger->tx_id_cnt++;

	if ((charger->tx_id_cnt <= 10) && !charger->tx_id) {
		pr_info("%s: request TX ID (%d)\n", __func__, charger->tx_id_cnt);
		queue_delayed_work(charger->wqueue, &charger->wpc_tx_id_work, msecs_to_jiffies(1500));
		return;
	} else {
		if (charger->tx_id) {
			pr_info("%s: TX ID (0x%x)\n", __func__, charger->tx_id);
		} else {
			pr_info("%s: TX ID not Received, cable_type(%d)\n",
				__func__, charger->pdata->cable_type);
			if (is_hv_wireless_pad_type(charger->pdata->cable_type)) {
				/* SET OV 13V */
				mfc_reg_write(charger->client, MFC_RX_OV_CLAMP_REG, 0x1);
			} else {
				/* SET OV 11V */
				mfc_reg_write(charger->client, MFC_RX_OV_CLAMP_REG, 0x0);
			}
		}
		charger->tx_id_cnt = 0;
	}
	wake_unlock(&charger->wpc_tx_id_lock);
}

static irqreturn_t mfc_wpc_det_irq_thread(int irq, void *irq_data)
{
	struct mfc_charger_data *charger = irq_data;

	pr_info("%s !\n", __func__);

	if (charger->is_probed) {
		queue_delayed_work(charger->wqueue, &charger->wpc_det_work, 0);
	}
	else
		pr_info("%s: prevent work thread before device is probed.\n", __func__);

	return IRQ_HANDLED;
}

/* mfc_mst_routine : MST dedicated codes */
static void mfc_mst_routine(struct mfc_charger_data *charger, u8 irq_src_l, u8 irq_src_h)
{
	u8 data = 0;

	pr_info("%s\n", __func__);

	if (charger->is_mst_on == MST_MODE_2) {
		charger->wc_tx_enable = false;
		mfc_reg_write(charger->client, MFC_MST_MODE_SEL_REG, 0x02); /* set MST mode2 */
		pr_info("%s 2AC Missing ! : MST on REV : %d\n", __func__, charger->pdata->wc_ic_rev);

		/* clear intterupt */
		mfc_reg_write(charger->client, MFC_INT_A_CLEAR_L_REG, irq_src_l); // clear int
		mfc_reg_write(charger->client, MFC_INT_A_CLEAR_H_REG, irq_src_h); // clear int
		mfc_set_cmd_l_reg(charger, 0x20, MFC_CMD_CLEAR_INT_MASK); // command
	} else if (charger->wc_tx_enable) {
		mfc_reg_read(charger->client,MFC_STATUS_H_REG, &data);
		data &= 0x4; /* AC MISSING DETECT */
		msleep(100);
		pr_info("@Tx_Mode %s: 0x21 Register AC Missing(%d)\n", __func__, data);
		if (data) {
			mfc_reg_write(charger->client, MFC_MST_MODE_SEL_REG, 0x03); /* set TX-ON mode */
			pr_info("@Tx_Mode %s: TX-ON Mode : %d\n", __func__, charger->pdata->wc_ic_rev);
		} //ac missing is 0, ie, TX detected
	}
}

static void mfc_check_sys_op_mode(struct mfc_charger_data *charger, u8 mode)
{
	mode &= 0xF;

	if (mode == MFC_TX_MODE_TX_PWR_HOLD) {
		if(charger->wc_rx_type == SS_GEAR) {
			/* start 3min alarm timer */
			pr_info("@Tx_Mode %s: Start PHM alarm by 3min\n", __func__);
			alarm_start(&charger->phm_alarm,
	    		ktime_add(ktime_get_boottime(), ktime_set(180, 0)));
		} else {
			pr_info("%s: TX entered PHM but no 3min timer\n", __func__);
		}
	} else {
		if(charger->phm_alarm.state & ALARMTIMER_STATE_ENQUEUED) {
			pr_info("@Tx_Mode %s: escape PHM mode, cancel PHM alarm\n", __func__);
			cancel_delayed_work(&charger->wpc_tx_phm_work);
			wake_unlock(&charger->wpc_tx_phm_lock);
			alarm_cancel(&charger->phm_alarm);
		}
	}
}

static int mfc_clear_irq(struct mfc_charger_data *charger, u8 *src_l, u8 *src_h, u8 *status_l, u8 *status_h)
{
	int wc_w_state_irq = 0;
	u8 new_src_l = 0, new_src_h = 0;
	u8 new_status_l = 0, new_status_h = 0;
	int ret = 0, rerun_ret = 1;

	pr_info("%s start\n", __func__);

	//ret = mfc_reg_write(charger->client, MFC_INT_A_CLEAR_L_REG, *src_l); // clear int
	//ret = mfc_reg_write(charger->client, MFC_INT_A_CLEAR_H_REG, *src_h); // clear int
	ret = mfc_reg_write(charger->client, MFC_INT_A_CLEAR_L_REG, 0xff); // clear int
	ret = mfc_reg_write(charger->client, MFC_INT_A_CLEAR_H_REG, 0xff); // clear int	
	mfc_set_cmd_l_reg(charger, 0x20, MFC_CMD_CLEAR_INT_MASK); // command

	//msleep(2);
	wc_w_state_irq = gpio_get_value(charger->pdata->wpc_int);
	pr_info("%s wc_w_state_irq = %d\n", __func__, wc_w_state_irq);

	if (!wc_w_state_irq && (ret >= 0)) {
		pr_info("%s wc_w_state_irq is not cleared\n", __func__);		
		ret = mfc_reg_read(charger->client, MFC_INT_A_L_REG, &new_src_l);
		ret = mfc_reg_read(charger->client, MFC_INT_A_H_REG, &new_src_h);
			
		ret = mfc_reg_read(charger->client, MFC_STATUS_L_REG, &new_status_l);
		ret = mfc_reg_read(charger->client, MFC_STATUS_H_REG, &new_status_h);

		pr_info("%s src_l[0x%x -> 0x%x], src_h[0x%x -> 0x%x], status_l[0x%x -> 0x%x],  status_h[0x%x -> 0x%x]\n",
			__func__, *src_l, new_src_l, *src_h, new_src_h, *status_l, new_status_l, *status_h, new_status_h);

		rerun_ret = 0; // re-run isr

		/* do not try irq again with i2c fail status, need to end up the irq */
		if ((*src_l != new_src_l ||
			*src_h != new_src_h ||
			*status_l != new_status_l ||
			*status_h != new_status_h) &&
			(ret >= 0)) {
			*src_l = new_src_l;
			*src_h = new_src_h;
			*status_l = new_status_l;
			*status_h = new_status_h;
			pr_info("%s re-run isr\n", __func__);
		} else if (ret < 0) {
			rerun_ret = 1; // do not re-run isr
			pr_info("%s i2c fail, do not re-run isr\n", __func__);
		} else {
			pr_info("%s re-run isr with same src, status\n", __func__);
		}
	}

	pr_info("%s end (%d)\n", __func__, rerun_ret);
	return rerun_ret;
}

static irqreturn_t mfc_wpc_irq_thread(int irq, void *irq_data)
{
	struct mfc_charger_data *charger = irq_data;
	int wc_w_state_irq = 0;
	int ret = 0;
	u8 irq_src_l = 0, irq_src_h = 0;
	u8 status_l = 0, status_h = 0;
	u8 reg_data = 0;
	bool end_irq = false;
	union power_supply_propval value;

	pr_info("%s start!\n", __func__);

	wc_w_state_irq = gpio_get_value(charger->pdata->wpc_int);
	pr_info("%s wc_w_state_irq = %d\n", __func__, wc_w_state_irq);

	wake_lock(&charger->wpc_wake_lock);

	ret = mfc_reg_read(charger->client, MFC_INT_A_L_REG, &irq_src_l);
	ret = mfc_reg_read(charger->client, MFC_INT_A_H_REG, &irq_src_h);

	ret = mfc_reg_read(charger->client, MFC_STATUS_L_REG, &status_l);
	ret = mfc_reg_read(charger->client, MFC_STATUS_H_REG, &status_h);

	pr_info("%s: interrupt source(0x%x), status(0x%x) \n", __func__, irq_src_h << 8 | irq_src_l, status_h << 8 | status_l);

	if (ret < 0) {
		pr_err("%s: Failed to read interrupt source: %d\n",
			__func__, ret);
		wake_unlock(&charger->wpc_wake_lock);
		return IRQ_HANDLED;
	}

	if (irq_src_h & MFC_INTA_H_AC_MISSING_DET_MASK) {
		pr_info("%s 1AC Missing ! : MFC is on\n", __func__);
		mfc_mst_routine(charger, irq_src_l, irq_src_h);
	}

	if (irq_src_l & MFC_INTA_L_OP_MODE_MASK) {
		pr_info("%s MODE CHANGE IRQ !\n", __func__);
		if(charger->wc_tx_enable) {
			ret = mfc_reg_read(charger->client, MFC_SYS_OP_MODE_REG, &reg_data);
			mfc_check_sys_op_mode(charger, reg_data);
		}
	}

	if ((irq_src_l & MFC_INTA_L_OVER_VOL_MASK) ||
		(irq_src_l & MFC_INTA_L_OVER_CURR_MASK)) {
		pr_info("%s ABNORMAL STAT IRQ !\n", __func__);
	}

	if (irq_src_l & MFC_INTA_L_OVER_TEMP_MASK) {
		pr_info("%s OVER TEMP IRQ !\n", __func__);
		if (charger->wc_tx_enable) {
			value.intval = BATT_TX_EVENT_WIRELESS_RX_UNSAFE_TEMP;
			end_irq = true;
			goto INT_END;
		}
	}

	if (irq_src_l & MFC_INTA_L_STAT_VRECT_MASK) {
		pr_info("%s: Vrect IRQ !\n", __func__);
		if (charger->is_mst_on == MST_MODE_2 || charger->wc_tx_enable) {
			pr_info("%s: Noise made false Vrect IRQ !\n", __func__);
			if (charger->wc_tx_enable) {
				value.intval = BATT_TX_EVENT_WIRELESS_TX_ETC;
				end_irq = true;
				goto INT_END;
			}
		}
	}

	if (irq_src_l & MFC_INTA_L_TXCONFLICT_MASK) {
		pr_info("@Tx_Mode %s TXCONFLICT IRQ !\n", __func__);
		if (charger->wc_tx_enable && (status_l & MFC_STAT_L_TXCONFLICT_MASK)) {
			value.intval = BATT_TX_EVENT_WIRELESS_TX_ETC;
			end_irq = true;
			goto INT_END;
		}
	}

	if (irq_src_h & MFC_INTA_H_TRX_DATA_RECEIVED_MASK) {
		pr_info("%s TX RECEIVED IRQ !\n", __func__);
		if (charger->wc_tx_enable && !delayed_work_pending(&charger->wpc_tx_isr_work)) {
			wake_lock(&charger->wpc_tx_wake_lock);
			queue_delayed_work(charger->wqueue, &charger->wpc_tx_isr_work, msecs_to_jiffies(1000));
		}
		else if (charger->pdata->cable_type == SEC_WIRELESS_PAD_WPC_STAND ||
			charger->pdata->cable_type == SEC_WIRELESS_PAD_WPC_STAND_HV)
			pr_info("%s Don't run ISR_WORK for NO ACK !\n", __func__);
		else if (!delayed_work_pending(&charger->wpc_isr_work)) {
			wake_lock(&charger->wpc_rx_wake_lock);
			queue_delayed_work(charger->wqueue, &charger->wpc_isr_work, msecs_to_jiffies(500));
		}
	}

	if (irq_src_h & MFC_INTA_H_TX_CON_DISCON_MASK) {
		pr_info("@Tx_Mode %s TX CONNECT IRQ !\n", __func__);
		charger->tx_status = SEC_TX_POWER_TRANSFER;
		if (status_h & MFC_STAT_H_TX_CON_DISCON_MASK) {
			/* determine rx connection status with tx sharing mode */
			if (!charger->wc_rx_connected)
				mfc_tx_check_rx_connection(charger, true);
		} else {
			/* determine rx connection status with tx sharing mode */
			if (!charger->wc_rx_connected) {
				pr_info("@Tx_Mode %s Ignore IRQ!! already Rx disconnected!\n", __func__);
			} else {
				mfc_tx_check_rx_connection(charger, false);
			}
		}
	}

	/* when rx is not detacted in 3 mins */
	if (irq_src_h & MFC_INTA_H_TX_MODE_RX_NOT_DET_MASK) {
		pr_info("@Tx_Mode %s Receiver NOT DETECTED IRQ !\n", __func__);
		if (charger->wc_tx_enable) {
			value.intval = BATT_TX_EVENT_WIRELESS_TX_ETC;
			end_irq = true;
			goto INT_END;
		}
	}

	if (irq_src_h & MFC_STAT_H_ADT_RECEIVED_MASK) {
		pr_info("%s %s ADT Received IRQ !\n", WC_AUTH_MSG, __func__);
		mfc_auth_adt_read(charger, ADT_buffer_rdata);
	}

	if (irq_src_h & MFC_STAT_H_ADT_SENT_MASK) {
		pr_info("%s %s ADT Sent successful IRQ !\n", WC_AUTH_MSG, __func__);
		mfc_reg_update(charger->client, MFC_INT_A_ENABLE_H_REG,
						0, MFC_STAT_H_ADT_SENT_MASK);
	}

	if (irq_src_h & MFC_INTA_H_TX_FOD_MASK) {
		pr_info("%s TX FOD IRQ !\n", __func__);
		// this code will move to wpc_tx_isr_work
		if (charger->wc_tx_enable) {
			if (status_h & MFC_STAT_H_TX_FOD_MASK) {
				charger->wc_rx_fod = true;
				value.intval = BATT_TX_EVENT_WIRELESS_TX_FOD;
				end_irq = true;
				goto INT_END;
			}
		}
	}

	if (irq_src_h & MFC_INTA_H_TX_OCP_MASK) {
		pr_info("%s TX Over Current IRQ !\n", __func__);
	}

INT_END:
	/* clear intterupt */
	mfc_clear_irq(charger, &irq_src_l, &irq_src_h, &status_l, &status_h);

	/* tx off should work having done i2c */
	if (end_irq)
		psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);

	pr_info("%s end!\n", __func__);

	wake_unlock(&charger->wpc_wake_lock);

	return IRQ_HANDLED;
}

static int mfc_chg_parse_dt(struct device *dev, 
		mfc_charger_platform_data_t *pdata)
{
	int ret = 0;
	struct device_node *np  = dev->of_node;
	enum of_gpio_flags irq_gpio_flags;
	int len,i;
	const u32 *p;

	if (!np) {
		pr_err("%s np NULL\n", __func__);
		return 1;
	} else {
		p = of_get_property(np, "battery,fod_data", &len);
		if (p) {
			len = len / sizeof(u32);
			pdata->fod_data = kzalloc(sizeof(*pdata->fod_data) * len, GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,fod_data",
							 pdata->fod_data, len);
			pdata->fod_data_check = 1;

			for (i = 0; i < len; i++)
				pr_info("%s fod data = %d ", __func__, pdata->fod_data[i]);
		} else {
			pdata->fod_data_check = 0;
			pr_err("%s there is not fod_data\n", __func__);
		}

		p = of_get_property(np, "battery,fod_data_cv", &len);
		if (p) {
			len = len / sizeof(u32);
			pdata->fod_data_cv = kzalloc(sizeof(*pdata->fod_data_cv) * len, GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,fod_data_cv",
							 pdata->fod_data_cv, len);
			pdata->fod_data_check = 1;

			for (i = 0; i < len; i++)
				pr_info("%s fod data_cv = %d ", __func__, pdata->fod_data_cv[i]);
		} else {
			pdata->fod_data_check = 0;
			pr_err("%s there is not fod_data_cv\n", __func__);
		}

		p = of_get_property(np, "battery,fod_hero_5v_data", &len);
		if (p) {
			len = len / sizeof(u32);
			pdata->fod_hero_5v_data = kzalloc(sizeof(*pdata->fod_hero_5v_data) * len, GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,fod_hero_5v_data",
							 pdata->fod_hero_5v_data, len);

			for (i = 0; i < len; i++)
				pr_info("%s fod Hero 5V data = 0x%x ", __func__, pdata->fod_hero_5v_data[i]);
		} else {
			pr_err("%s there is not fod_hero_5v_data\n", __func__);
		}

		p = of_get_property(np, "battery,fod_dream_data", &len);
		if (p) {
			len = len / sizeof(u32);
			pdata->fod_dream_data = kzalloc(sizeof(*pdata->fod_dream_data) * len, GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,fod_dream_data",
							 pdata->fod_dream_data, len);

			for (i = 0; i < len; i++)
				pr_info("%s fod Dream data = 0x%x ", __func__, pdata->fod_dream_data[i]);
		} else {
			pr_err("%s there is not fod_dream_data\n", __func__);
		}

		p = of_get_property(np, "battery,fod_dream_cv_data", &len);
		if (p) {
			len = len / sizeof(u32);
			pdata->fod_dream_cv_data = kzalloc(sizeof(*pdata->fod_dream_cv_data) * len, GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,fod_dream_cv_data",
							 pdata->fod_dream_cv_data, len);

			for (i = 0; i < len; i++)
				pr_info("%s fod Dream CV data = 0x%x ", __func__, pdata->fod_dream_cv_data[i]);
		} else {
			pr_err("%s there is not fod_dream_cv_data\n", __func__);
		}

		ret = of_property_read_string(np,
			"battery,wireless_charger_name", (char const **)&pdata->wireless_charger_name);
		if (ret < 0)
			pr_info("%s: Wireless Charger name is Empty\n", __func__);

		ret = of_property_read_string(np,
			"battery,charger_name", (char const **)&pdata->wired_charger_name);
		if (ret < 0)
			pr_info("%s: Charger name is Empty\n", __func__);

		ret = of_property_read_string(np,
			"battery,fuelgauge_name", (char const **)&pdata->fuelgauge_name);
		if (ret < 0)
			pr_info("%s: Fuelgauge name is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,mst_switch_delay",
						&pdata->mst_switch_delay);
		if (ret < 0) {
			pr_info("%s: mst_switch_delay is Empty\n", __func__);
			pdata->mst_switch_delay = 1000; /* set default value (dream) */
		}

		ret = of_property_read_u32(np, "battery,wc_cover_rpp",
						&pdata->wc_cover_rpp);
		if (ret < 0) {
			pr_info("%s: fail to read wc_cover_rpp.\n", __func__);
			pdata->wc_cover_rpp = 0x55;
		}

		ret = of_property_read_u32(np, "battery,wc_hv_rpp",
						&pdata->wc_hv_rpp);
		if (ret < 0) {
			pr_info("%s: fail to read wc_hv_rpp.\n", __func__);
			pdata->wc_hv_rpp = 0x40;
		}

		/* wpc_det */
		ret = pdata->wpc_det = of_get_named_gpio_flags(np, "battery,wpc_det",
				0, &irq_gpio_flags);
		if (ret < 0) {
			dev_err(dev, "%s : can't get wpc_det\r\n", __FUNCTION__);
		} else {
			pdata->irq_wpc_det = gpio_to_irq(pdata->wpc_det);
			pr_info("%s wpc_det = 0x%x, irq_wpc_det = 0x%x\n", __func__, pdata->wpc_det, pdata->irq_wpc_det);
		}
		/* wpc_int (This GPIO means MFC_AP_INT) */
		ret = pdata->wpc_int = of_get_named_gpio_flags(np, "battery,wpc_int",
				0, &irq_gpio_flags);
		if (ret < 0) {
			dev_err(dev, "%s : can't wpc_int\r\n", __FUNCTION__);
		} else {
			pdata->irq_wpc_int = gpio_to_irq(pdata->wpc_int);
			pr_info("%s wpc_int = 0x%x, irq_wpc_int = 0x%x\n", __func__, pdata->wpc_int, pdata->irq_wpc_int);
		}

		/* vrect_int  */
		ret = pdata->vrect_int = of_get_named_gpio_flags(np, "battery,vrect_int",
				0, &irq_gpio_flags);
		if (ret < 0) {
			dev_err(dev, "%s : can't vrect_int\r\n", __FUNCTION__);
		} else {
			pdata->irq_vrect_int = gpio_to_irq(pdata->vrect_int);
			pr_info("%s vrect_int = 0x%x, irq_vrect_int = 0x%x\n", __func__, pdata->vrect_int, pdata->irq_vrect_int);
		}

		/* mst_pwr_en (MST PWR EN) */
		ret = pdata->mst_pwr_en = of_get_named_gpio_flags(np, "battery,mst_pwr_en",
				0, &irq_gpio_flags);
		if (ret < 0) {
			dev_err(dev, "%s : can't mst_pwr_en\r\n", __FUNCTION__);
		}

		/* wpc_en (MFC EN) */
		ret = pdata->wpc_en = of_get_named_gpio_flags(np, "battery,wpc_en",
				0, &irq_gpio_flags);
		if (ret < 0) {
			dev_err(dev, "%s : can't wpc_en\r\n", __FUNCTION__);
		}

		/* ping_nen (PING nEN) */
		ret = pdata->ping_nen = of_get_named_gpio_flags(np, "battery,ping_nen",
				0, &irq_gpio_flags);
		if (ret < 0) {
			dev_err(dev, "%s : can't ping_nen\r\n", __FUNCTION__);
		}

		p = of_get_property(np, "battery,wireless20_vout_list", &len);
		if (p) {
			len = len / sizeof(u32);
			pdata->len_wc20_list = len;
			pdata->wireless20_vout_list = kzalloc(sizeof(*pdata->wireless20_vout_list) * len, GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,wireless20_vout_list",
							 pdata->wireless20_vout_list, len);

			for (i = 0; i < len; i++)
				pr_info("%s wireless20_vout_list = %d ", __func__, pdata->wireless20_vout_list[i]);
			pr_info("%s len_wc20_list = %d ", __func__, pdata->len_wc20_list);
		} else {
			pr_err("%s there is no wireless20_vout_list\n", __func__);
		}

		p = of_get_property(np, "battery,wireless20_vrect_list", &len);
		if (p) {
			len = len / sizeof(u32);
			pdata->wireless20_vrect_list = kzalloc(sizeof(*pdata->wireless20_vrect_list) * len, GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,wireless20_vrect_list",
							 pdata->wireless20_vrect_list, len);

			for (i = 0; i < len; i++)
				pr_info("%s wireless20_vrect_list = %d ", __func__, pdata->wireless20_vrect_list[i]);
		} else {
			pr_err("%s there is no wireless20_vrect_list\n", __func__);
		}

		p = of_get_property(np, "battery,wireless20_max_power_list", &len);
		if (p) {
			len = len / sizeof(u32);
			pdata->wireless20_max_power_list = kzalloc(sizeof(*pdata->wireless20_max_power_list) * len, GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,wireless20_max_power_list",
							 pdata->wireless20_max_power_list, len);

			for (i = 0; i < len; i++)
				pr_info("%s wireless20_max_power_list = %d ", __func__, pdata->wireless20_max_power_list[i]);
		} else {
			pr_err("%s there is no wireless20_max_power_list\n", __func__);
		}

		return 0;
	}
}

static int mfc_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < (int)ARRAY_SIZE(mfc_attrs); i++) {
		rc = device_create_file(dev, &mfc_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	return rc;

create_attrs_failed:
	dev_err(dev, "%s: failed (%d)\n", __func__, rc);
	while (i--)
		device_remove_file(dev, &mfc_attrs[i]);
	return rc;
}

ssize_t mfc_s2miw04_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mfc_charger_data *charger = power_supply_get_drvdata(psy);

	const ptrdiff_t offset = attr - mfc_attrs;
	int i = 0;

	dev_info(charger->dev, "%s \n", __func__);

	switch (offset) {
	case MFC_ADDR:
		i += sprintf(buf, "0x%x\n", charger->addr);
		break;
	case MFC_SIZE:
		i += sprintf(buf, "0x%x\n", charger->size);
		break;
	case MFC_DATA:
		if (charger->size == 0) {
			charger->size = 1;
		} else if (charger->size + charger->addr <= 0xFFFF) {
			u8 data;
			int j;

			for (j = 0; j < charger->size; j++) {
				if (mfc_reg_read(charger->client, charger->addr + j, &data) < 0) {
					dev_info(charger->dev,
							"%s: read fail\n", __func__);
					i += sprintf(buf + i, "addr: 0x%x read fail\n", charger->addr + j);
					continue;
				}
				i += sprintf(buf + i, "addr: 0x%x, data: 0x%x\n", charger->addr + j, data);
			}
		}		
		break;
	case MFC_PACKET:
		break;
	default:
		return -EINVAL;
	}
	return i;
}

ssize_t mfc_s2miw04_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mfc_charger_data *charger = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - mfc_attrs;	
	int x, ret;
	u8 header, data_com, data_val;

	dev_info(charger->dev, "%s \n", __func__);

	switch (offset) {
	case MFC_ADDR:
		if (sscanf(buf, "0x%4x\n", &x) == 1)
			charger->addr = x;
		ret = count;
		break;
	case MFC_SIZE:
		if (sscanf(buf, "%5d\n", &x) == 1)
			charger->size = x;
		ret = count;
		break;
	case MFC_DATA:
		if (sscanf(buf, "0x%10x", &x) == 1) {
			u8 data = x;
		
			if (mfc_reg_write(charger->client, charger->addr, data) < 0) {
				dev_info(charger->dev,
						"%s: addr: 0x%x write fail\n", __func__, charger->addr);
			}
		}
		ret = count;
		break;
	case MFC_PACKET:
		if (sscanf(buf, "0x%4x 0x%4x 0x%4x\n", &header, &data_com, &data_val) == 3) {
			dev_info(charger->dev, "%s 0x%x, 0x%x, 0x%x \n", __func__, header, data_com, data_val);
			mfc_send_packet(charger, header, data_com, &data_val, 1);
		}
		ret = count;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static const struct power_supply_desc mfc_charger_power_supply_desc = {
	.name = "mfc-charger",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.properties = mfc_charger_props,
	.num_properties = ARRAY_SIZE(mfc_charger_props),
	.get_property = mfc_chg_get_property,
	.set_property = mfc_chg_set_property,
};

static void mfc_wpc_int_req_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_int_req_work.work);

	int ret = 0;

	pr_info("%s\n", __func__);
	/* wpc_irq */
	if (charger->pdata->irq_wpc_int) {
		msleep(100);
		ret = request_threaded_irq(charger->pdata->irq_wpc_int,
				NULL, mfc_wpc_irq_thread,
				IRQF_TRIGGER_FALLING |
				IRQF_ONESHOT,
				"wpc-irq", charger);
		if (ret) {
			pr_err("%s: Failed to Reqeust IRQ\n", __func__);
		}
	}
	if (ret < 0)
		free_irq(charger->pdata->irq_wpc_det, NULL);
}

static enum alarmtimer_restart mfc_phm_alarm(
	struct alarm *alarm, ktime_t now)
{
	struct mfc_charger_data *charger = container_of(alarm,
				struct mfc_charger_data, phm_alarm);

	pr_info("%s: forced escape to PHM\n", __func__);
	wake_lock(&charger->wpc_tx_phm_lock);
	queue_delayed_work(charger->wqueue, &charger->wpc_tx_phm_work, 0);

	return ALARMTIMER_NORESTART;
}

static int mfc_s2miw04_charger_probe(
						struct i2c_client *client,
						const struct i2c_device_id *id)
{
	struct device_node *of_node = client->dev.of_node;
	struct mfc_charger_data *charger;
	mfc_charger_platform_data_t *pdata = client->dev.platform_data;
	struct power_supply_config mfc_cfg = {};
	int ret = 0;
	int wc_w_state_irq;

	dev_info(&client->dev,
		"%s: MFC s2miw04 Charger Driver Loading\n", __func__);

#if defined(CONFIG_WIRELESS_CHARGER_HAL_MFC)
	if (client->addr != 0x3b)
		client->addr = 0x3b;
#endif

	if (of_node) {
		pdata = devm_kzalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		ret = mfc_chg_parse_dt(&client->dev, pdata);
		if (ret < 0)
			goto err_parse_dt;
	} else {
		pdata = client->dev.platform_data;
		return -ENOMEM;
	}

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (charger == NULL) {
		dev_err(&client->dev, "Memory is not enough.\n");
		ret = -ENOMEM;
		goto err_wpc_nomem;
	}
	charger->dev = &client->dev;

	ret = i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA |
		I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_I2C_BLOCK);
	if (!ret) {
		ret = i2c_get_functionality(client->adapter);
		dev_err(charger->dev, "I2C functionality is not supported.\n");
		ret = -ENOSYS;
		goto err_i2cfunc_not_support;
	}

	is_shutdn = false;
	charger->client = client;
	charger->pdata = pdata;

    pr_info("%s: %s\n", __func__, charger->pdata->wireless_charger_name);

	i2c_set_clientdata(client, charger);

	charger->pdata->cable_type = SEC_WIRELESS_PAD_NONE;
	charger->pdata->is_charging = 0;
	charger->tx_status = 0;
	charger->pdata->cs100_status = 0;
	charger->pdata->capacity = 101;
	charger->pdata->vout_status = MFC_VOUT_5V;
	charger->pdata->opfq_cnt = 0;

	charger->is_mst_on = MST_MODE_0;
	charger->chip_id = MFC_CHIP_LSI;
	charger->chip_id_now = MFC_CHIP_ID_S2MIW04;
	charger->is_otg_on = false;
	charger->led_cover = 0;
	charger->vout_mode = WIRELESS_VOUT_OFF;
	charger->is_full_status = 0;
	charger->is_afc_tx = false;
	charger->wc_tx_enable = false;
	charger->initial_wc_check = false;
	charger->wc_rx_connected = false;
	charger->wc_rx_fod = false;
	charger->non_sleep_mode_cnt = 0;
	charger->adt_transfer_status = WIRELESS_AUTH_WAIT;
	charger->current_rx_power = TX_RX_POWER_0W;
	charger->tx_id_cnt = 0;
	charger->wc_ldo_status = MFC_LDO_ON;
	charger->tx_id_done = false;
	charger->wc_rx_type = NO_DEV;
	charger->is_suspend = false;
	charger->device_event = 0;
	charger->wpc_en_flag = (WPC_EN_SYSFS | WPC_EN_CHARGING | WPC_EN_CCIC);

	mutex_init(&charger->io_lock);
	mutex_init(&charger->wpc_en_lock);
	mutex_init(&charger->fw_lock);

	/* wpc_det */
	if (charger->pdata->irq_wpc_det) {
		INIT_DELAYED_WORK(&charger->wpc_det_work, mfc_wpc_det_work);
		INIT_DELAYED_WORK(&charger->wpc_opfq_work, mfc_wpc_opfq_work);
	}

	/* wpc_irq (INT_A) */
	if (charger->pdata->irq_wpc_int) {
		INIT_DELAYED_WORK(&charger->wpc_isr_work, mfc_wpc_isr_work);
		INIT_DELAYED_WORK(&charger->wpc_tx_isr_work, mfc_wpc_tx_isr_work);
		INIT_DELAYED_WORK(&charger->wpc_tx_id_work, mfc_wpc_tx_id_work);
		INIT_DELAYED_WORK(&charger->wpc_int_req_work, mfc_wpc_int_req_work);
	}
	INIT_DELAYED_WORK(&charger->wpc_vout_mode_work, mfc_wpc_vout_mode_work);
	INIT_DELAYED_WORK(&charger->wpc_afc_vout_work, mfc_wpc_afc_vout_work);
	INIT_DELAYED_WORK(&charger->wpc_fw_update_work, mfc_wpc_fw_update_work);
	INIT_DELAYED_WORK(&charger->wpc_cm_fet_work, mfc_wpc_cm_fet_work);
	INIT_DELAYED_WORK(&charger->wpc_i2c_error_work, mfc_wpc_i2c_error_work);
	INIT_DELAYED_WORK(&charger->wpc_rx_det_work, mfc_wpc_rx_det_work);
	INIT_DELAYED_WORK(&charger->wpc_tx_op_freq_work, mfc_tx_op_freq_work);
	INIT_DELAYED_WORK(&charger->wpc_tx_phm_work, mfc_tx_phm_work);

	/*
	* Default Idle voltage of the INT_A is LOW.
	* Prevent the un-wanted INT_A Falling handling.
	* This is a work-around, and will be fixed by the revision.
	*/
	//INIT_DELAYED_WORK(&charger->mst_off_work, mfc_mst_off_work);

	alarm_init(&charger->phm_alarm, ALARM_BOOTTIME,
				mfc_phm_alarm);

	mfc_cfg.drv_data = charger;
	charger->psy_chg = power_supply_register(charger->dev, &mfc_charger_power_supply_desc, &mfc_cfg);
	if ((void *)charger->psy_chg < 0) {
		pr_err("%s: Failed to Register psy_chg\n", __func__);
		goto err_supply_unreg;
	}

	charger->wqueue = create_singlethread_workqueue("mfc_workqueue");
	if (!charger->wqueue) {
		pr_err("%s: Fail to Create Workqueue\n", __func__);
		goto err_pdata_free;
	}

	wake_lock_init(&charger->wpc_wake_lock, WAKE_LOCK_SUSPEND,
			"wpc_wakelock");
	wake_lock_init(&charger->wpc_rx_wake_lock, WAKE_LOCK_SUSPEND,
			"wpc_rx_wakelock");
	wake_lock_init(&charger->wpc_tx_wake_lock, WAKE_LOCK_SUSPEND,
			"wpc_tx_wakelock");
	wake_lock_init(&charger->wpc_update_lock, WAKE_LOCK_SUSPEND,
			"wpc_update_lock");
	wake_lock_init(&charger->wpc_opfq_lock, WAKE_LOCK_SUSPEND,
			"wpc_opfq_lock");
	wake_lock_init(&charger->wpc_tx_opfq_lock, WAKE_LOCK_SUSPEND,
			"wpc_tx_opfq_lock");
	wake_lock_init(&charger->wpc_afc_vout_lock, WAKE_LOCK_SUSPEND,
			"wpc_afc_vout_lock");
	wake_lock_init(&charger->wpc_vout_mode_lock, WAKE_LOCK_SUSPEND,
			"wpc_vout_mode_lock");
	wake_lock_init(&charger->wpc_rx_det_lock, WAKE_LOCK_SUSPEND,
			"wpc_rx_det_lock");
	wake_lock_init(&charger->wpc_tx_phm_lock, WAKE_LOCK_SUSPEND,
			"wpc_tx_phm_lock");
	wake_lock_init(&charger->wpc_tx_id_lock, WAKE_LOCK_SUSPEND,
			"wpc_tx_id_lock");

	/* Enable interrupts after battery driver load */
	/* wpc_det */
	if (charger->pdata->irq_wpc_det) {
		ret = request_threaded_irq(charger->pdata->irq_wpc_det,
				NULL, mfc_wpc_det_irq_thread,
				IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
				IRQF_ONESHOT,
				"wpd-det-irq", charger);
		if (ret) {
			pr_err("%s: Failed to Reqeust IRQ\n", __func__);
			goto err_irq_wpc_det;
		}
	}

	/* wpc_irq */
	queue_delayed_work(charger->wqueue, &charger->wpc_int_req_work, msecs_to_jiffies(100));

	wc_w_state_irq = gpio_get_value(charger->pdata->wpc_int);
	pr_info("%s wc_w_state_irq = %d\n", __func__, wc_w_state_irq);
	if (gpio_get_value(charger->pdata->wpc_det)) {
		u8 irq_src[2];
		pr_info("%s: Charger interrupt occured during lpm\n", __func__);

		mfc_reg_read(charger->client, MFC_INT_A_L_REG, &irq_src[0]);
		mfc_reg_read(charger->client, MFC_INT_A_H_REG, &irq_src[1]);
		/* clear intterupt */
		mfc_reg_write(charger->client, MFC_INT_A_CLEAR_L_REG, irq_src[0]); // clear int
		mfc_reg_write(charger->client, MFC_INT_A_CLEAR_H_REG, irq_src[1]); // clear int
		mfc_set_cmd_l_reg(charger, 0x20, MFC_CMD_CLEAR_INT_MASK); // command
		queue_delayed_work(charger->wqueue, &charger->wpc_det_work, 0);
		if (!wc_w_state_irq && !delayed_work_pending(&charger->wpc_isr_work)) {
			wake_lock(&charger->wpc_rx_wake_lock);
			queue_delayed_work(charger->wqueue, &charger->wpc_isr_work, msecs_to_jiffies(2000));
		}
	}

	ret = mfc_create_attrs(&charger->psy_chg->dev);
	if (ret) {
		dev_err(charger->dev,
			"%s : Failed to create_attrs\n", __func__);
	}

	charger->is_probed = true;
	dev_info(&client->dev,
		"%s: MFC s2miw04 Charger Driver Loaded\n", __func__);

	device_init_wakeup(charger->dev, 1);
	return 0;

err_irq_wpc_det:
	wake_lock_destroy(&charger->wpc_wake_lock);
	wake_lock_destroy(&charger->wpc_rx_wake_lock);
	wake_lock_destroy(&charger->wpc_tx_wake_lock);
	wake_lock_destroy(&charger->wpc_update_lock);
	wake_lock_destroy(&charger->wpc_opfq_lock);
	wake_lock_destroy(&charger->wpc_tx_opfq_lock);	
	wake_lock_destroy(&charger->wpc_afc_vout_lock);
	wake_lock_destroy(&charger->wpc_vout_mode_lock);
	wake_lock_destroy(&charger->wpc_rx_det_lock);
	wake_lock_destroy(&charger->wpc_tx_phm_lock);
	wake_lock_destroy(&charger->wpc_tx_id_lock);
err_pdata_free:
	power_supply_unregister(charger->psy_chg);
err_supply_unreg:
	mutex_destroy(&charger->io_lock);
	mutex_destroy(&charger->wpc_en_lock);
	mutex_destroy(&charger->fw_lock);
err_i2cfunc_not_support:
	kfree(charger);
err_wpc_nomem:
err_parse_dt:
	devm_kfree(&client->dev, pdata);

	return ret;
}

static int mfc_s2miw04_charger_remove(struct i2c_client *client)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);

	alarm_cancel(&charger->phm_alarm);

	return 0;
}

#if defined(CONFIG_PM)
static int mfc_charger_suspend(struct device *dev)
{
	struct mfc_charger_data *charger = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);

	charger->is_suspend = true;

	if (device_may_wakeup(charger->dev)){
		enable_irq_wake(charger->pdata->irq_wpc_int);
		enable_irq_wake(charger->pdata->irq_wpc_det);
	}
	disable_irq(charger->pdata->irq_wpc_int);
	disable_irq(charger->pdata->irq_wpc_det);

	return 0;
}

static int mfc_charger_resume(struct device *dev)
{
	struct mfc_charger_data *charger = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);

	charger->is_suspend = false;

	if (device_may_wakeup(charger->dev)) {
		disable_irq_wake(charger->pdata->irq_wpc_int);
		disable_irq_wake(charger->pdata->irq_wpc_det);
	}
	enable_irq(charger->pdata->irq_wpc_int);
	enable_irq(charger->pdata->irq_wpc_det);

	return 0;
}
#else
#define mfc_charger_suspend NULL
#define mfc_charger_resume NULL
#endif

static void mfc_s2miw04_charger_shutdown(struct i2c_client *client)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	is_shutdn = true;
	pr_info("%s\n", __func__);

	cancel_delayed_work(&charger->wpc_vout_mode_work);
	alarm_cancel(&charger->phm_alarm);

	if (gpio_get_value(charger->pdata->wpc_det)) {
		pr_info("%s: forced 5V Vout\n", __func__);
		mfc_set_vrect_adjust(charger, MFC_HEADROOM_1);
		mfc_set_vout(charger, MFC_VOUT_5V);
	}
}

static const struct i2c_device_id mfc_s2miw04_charger_id_table[] = {
	{ "mfc-s2miw04-charger", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, mfc_s2miw04_charger_id_table);

#ifdef CONFIG_OF
static struct of_device_id mfc_s2miw04_charger_match_table[] = {
	{ .compatible = "lsi,mfc-s2miw04-charger",},
	{},
};
MODULE_DEVICE_TABLE(of, mfc_s2miw04_charger_match_table);
#else
#define mfc_s2miw04_charger_match_table NULL
#endif

const struct dev_pm_ops mfc_s2miw04_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(mfc_charger_suspend,mfc_charger_resume)
};

static struct i2c_driver mfc_s2miw04_charger_driver = {
	.driver = {
		.name	= "mfc-s2miw04-charger",
		.owner	= THIS_MODULE,
#if defined(CONFIG_PM)
		.pm = &mfc_s2miw04_pm,
#endif /* CONFIG_PM */
		.of_match_table = mfc_s2miw04_charger_match_table,
	},
	.shutdown	= mfc_s2miw04_charger_shutdown,
	.probe	= mfc_s2miw04_charger_probe,
	.remove	= mfc_s2miw04_charger_remove,
	.id_table	= mfc_s2miw04_charger_id_table,
};

static int __init mfc_s2miw04_charger_init(void)
{
#if defined(CONFIG_WIRELESS_CHARGER_HAL_MFC)
	if (mfc_chip_id_now != 0xFF && mfc_chip_id_now != MFC_CHIP_ID_S2MIW04)
		return -ENODEV;
#endif

	pr_info("%s\n", __func__);
	return i2c_add_driver(&mfc_s2miw04_charger_driver);
}

static void __exit mfc_s2miw04_charger_exit(void)
{
	pr_info("%s\n", __func__);
	i2c_del_driver(&mfc_s2miw04_charger_driver);
}

module_init(mfc_s2miw04_charger_init);
module_exit(mfc_s2miw04_charger_exit);

MODULE_DESCRIPTION("Samsung MFC Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
