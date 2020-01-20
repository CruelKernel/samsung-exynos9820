/*
 * Copyright (C) 2018 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define VENDOR				"AMS"

#define TCS3407_CHIP_NAME	"TCS3407"

#define VERSION				"1"
#define SUB_VERSION			"23"
#define VENDOR_VERSION		"a"

#define MODULE_NAME_ALS		"als_rear"

#define TCS3407_SLAVE_I2C_ADDR_REVID_V0 0x39
#define TCS3407_SLAVE_I2C_ADDR_REVID_V1 0x29

#define AMSDRIVER_I2C_RETRY_DELAY	10
#define AMSDRIVER_I2C_MAX_RETRIES	5

//#define CONFIG_AMS_OPTICAL_SENSOR_FIFO

#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
/* AWB/Flicker Definition */
#define ALS_AUTOGAIN
#define BYTE				2
#define AWB_INTERVAL		20 /* 20 sample(from 17 to 28) */

#define CONFIG_SKIP_CNT		8
#define FLICKER_FIFO_THR	16
#define FLICKER_DATA_CNT	200
#define FLICKER_FIFO_READ	-2

#define TCS3407_IOCTL_MAGIC		0xFD
#define TCS3407_IOCTL_READ_FLICKER	_IOR(TCS3407_IOCTL_MAGIC, 0x01, int *)
#endif

#if defined(CONFIG_LEDS_S2MPB02) && !defined(CONFIG_AMS_OPTICAL_SENSOR_FIFO)
#define CONFIG_AMS_OPTICAL_SENSOR_EOL_MODE
#endif

#include "tcs3407.h"

#ifdef CONFIG_AMS_OPTICAL_SENSOR_EOL_MODE
#include <linux/leds-s2mpb02.h>

#define DEFAULT_DUTY_50HZ		5000
#define DEFAULT_DUTY_60HZ		4166

#define MAX_TEST_RESULT			256
#define EOL_COUNT				20
#define EOL_SKIP_COUNT			5

#define DEFAULT_IR_SPEC_MIN		0
#define DEFAULT_IR_SPEC_MAX		500000

static u32 gSpec_ir_min = DEFAULT_IR_SPEC_MIN;
static u32 gSpec_ir_max = DEFAULT_IR_SPEC_MAX;
static u32 gSpec_clear_min = DEFAULT_IR_SPEC_MIN;
static u32 gSpec_clear_max = DEFAULT_IR_SPEC_MAX;

#define FREQ100_SPEC_IN(X)	((X == 100)?"PASS":"FAIL")
#define FREQ120_SPEC_IN(X)	((X == 120)?"PASS":"FAIL")

#define IR_SPEC_IN(X)		((X >= gSpec_ir_min && X <= gSpec_ir_max)?"PASS":"FAIL")
#define CLEAR_SPEC_IN(X)	((X >= gSpec_clear_min && X <= gSpec_clear_max)?"PASS":"FAIL")
#endif

static int als_debug = 1;
static int als_info;

module_param(als_debug, int, S_IRUGO | S_IWUSR);
module_param(als_info, int, S_IRUGO | S_IWUSR);

static struct tcs3407_device_data *tcs3407_data;

#define AMS_ROUND_SHFT_VAL				4
#define AMS_ROUND_ADD_VAL				(1 << (AMS_ROUND_SHFT_VAL - 1))
#define AMS_ALS_GAIN_FACTOR				1000
#define CPU_FRIENDLY_FACTOR_1024		1
#define AMS_ALS_Cc						(118 * CPU_FRIENDLY_FACTOR_1024)
#define AMS_ALS_Rc						(112 * CPU_FRIENDLY_FACTOR_1024)
#define AMS_ALS_Gc						(172 * CPU_FRIENDLY_FACTOR_1024)
#define AMS_ALS_Bc						(180 * CPU_FRIENDLY_FACTOR_1024)
#define AMS_ALS_Wbc						(111 * CPU_FRIENDLY_FACTOR_1024)

#define AMS_ALS_FACTOR					1000

#ifdef CONFIG_AMS_OPTICAL_SENSOR_259x
#define AMS_ALS_TIMEBASE				(100000) /* in uSec, see data sheet */
#define AMS_ALS_ADC_MAX_COUNT			(37888) /* see data sheet */
#else
#define AMS_ALS_TIMEBASE				(2780) /* in uSec, see data sheet */
#define AMS_ALS_ADC_MAX_COUNT			(1024) /* see data sheet */
#endif
#define AMS_ALS_THRESHOLD_LOW			(5) /* in % */
#define AMS_ALS_THRESHOLD_HIGH			(5) /* in % */

#define AMS_ALS_ATIME					(50000)

#define WIDEBAND_CONST	4
#define CLEAR_CONST		3

/* REENABLE only enables those that were on record as being enabled */
#define AMS_REENABLE(ret)				{ret = ams_setByte(ctx->portHndl, DEVREG_ENABLE, ctx->shadowEnableReg); }
/* DISABLE_ALS disables ALS w/o recording that as its new state */
#define AMS_DISABLE_ALS(ret)			{ret = ams_setField(ctx->portHndl, DEVREG_ENABLE, LOW, (MASK_AEN)); }
#define AMS_REENABLE_ALS(ret)			{ret = ams_setField(ctx->portHndl, DEVREG_ENABLE, HIGH, (MASK_AEN)); }

#define AMS_SET_ALS_TIME(uSec, ret)		{ret = ams_setByte(ctx->portHndl, DEVREG_ATIME,   alsTimeUsToReg(uSec)); }
#define AMS_GET_ALS_TIME(uSec, ret)		{ret = ams_getByte(ctx->portHndl, DEVREG_ATIME,   alsTimeUsToReg(uSec)); }

#define AMS_GET_ALS_GAIN(scaledGain, gain, ret)	{ret = ams_getByte(ctx->portHndl, DEVREG_ASTATUS, &(gain)); \
	scaledGain = alsGain_conversion[(gain) & 0x0f]; }

#define AMS_SET_ALS_STEP_TIME(uSec, ret)		{ret = ams_setWord(ctx->portHndl, DEVREG_ASTEPL, alsTimeUsToReg(uSec * 1000)); }

#define AMS_SET_ALS_GAIN(mGain, ret)	{ret = ams_setField(ctx->portHndl, DEVREG_CFG1, alsGainToReg(mGain), MASK_AGAIN); }
#define AMS_SET_ALS_PERS(persCode, ret)	{ret = ams_setField(ctx->portHndl, DEVREG_PERS, (persCode), MASK_APERS); }
#define AMS_CLR_ALS_INT(ret)			{ret = ams_setByte(ctx->portHndl, DEVREG_STATUS, (AINT | ASAT_FDSAT)); }
#define AMS_SET_ALS_THRS_LOW(x, ret)	{ret = ams_setWord(ctx->portHndl, DEVREG_AILTL, (x)); }
#define AMS_SET_ALS_THRS_HIGH(x, ret)	{ret = ams_setWord(ctx->portHndl, DEVREG_AIHTL, (x)); }
#define AMS_SET_ALS_AUTOGAIN(x, ret)	{ret = ams_setField(ctx->portHndl, DEVREG_CFG8, (x), MASK_AUTOGAIN); }
#define AMS_SET_ALS_AGC_LOW_HYST(x)		{ams_setField(ctx->portHndl, DEVREG_CFG10, ((x)<<4), MASK_AGC_LOW_HYST); }
#define AMS_SET_ALS_AGC_HIGH_HYST(x) {ams_setField(ctx->portHndl, DEVREG_CFG10, ((x)<<6), MASK_AGC_HIGH_HYST); }

/* Get CRGB and whatever Wideband it may have */
#define AMS_ALS_GET_CRGB_W(x, ret)		{ret = ams_getBuf(ctx->portHndl, DEVREG_ADATA0L, (uint8_t *) (x), 10); }

typedef struct{
	uint8_t deviceId;
	uint8_t deviceIdMask;
	uint8_t deviceRef;
	uint8_t deviceRefMask;
	ams_deviceIdentifier_e device;
} ams_deviceIdentifier_t;

typedef struct _fifo {
	uint16_t AdcClear;
	uint16_t AdcRed;
	uint16_t AdcGreen;
	uint16_t AdcBlue;
	uint16_t AdcWb;
} adcDataSet_t;

#define AMS_PORT_LOG_CRGB_W(dataset) \
		ALS_info("%s - C, R,G,B = %u, %u,%u,%u; WB = %u\n", __func__ \
			, dataset.AdcClear \
			, dataset.AdcRed \
			, dataset.AdcGreen \
			, dataset.AdcBlue \
			, dataset.AdcWb	\
			)

static ams_deviceIdentifier_t deviceIdentifier[] = {
	{AMS_DEVICE_ID, AMS_DEVICE_ID_MASK, AMS_REV_ID, AMS_REV_ID_MASK, AMS_TCS3407},
	{AMS_DEVICE_ID, AMS_DEVICE_ID_MASK, AMS_REV_ID_UNTRIM, AMS_REV_ID_MASK, AMS_TCS3407_UNTRIM},
	{AMS_DEVICE_ID2, AMS_DEVICE_ID2_MASK, AMS_REV_ID2, AMS_REV_ID2_MASK, AMS_TMD4907},
	{AMS_DEVICE_ID3, AMS_DEVICE_ID3_MASK, AMS_REV_ID3, AMS_REV_ID3_MASK, AMS_TMD4906},
	{0, 0, 0, 0, AMS_LAST_DEVICE}
};

deviceRegisterTable_t deviceRegisterDefinition[DEVREG_REG_MAX] = {
	{ 0x80, 0x00 },          /* DEVREG_ENABLE */
	{ 0x81, 0x00 },          /* DEVREG_ATIME */
	{ 0x82, 0x00 },          /* DEVREG_PTIME */
	{ 0x83, 0x00 },          /* DEVREG_WTIME */
	{ 0x84, 0x00 },          /* DEVREG_AILTL */
	{ 0x85, 0x00 },          /* DEVREG_AILTH */
	{ 0x86, 0x00 },          /* DEVREG_AIHTL */
	{ 0x87, 0x00 },          /* DEVREG_AIHTH */

	{ 0x90, 0x00 },          /* DEVREG_AUXID */
	{ 0x91, AMS_REV_ID },    /* DEVREG_REVID */
	{ 0x92, AMS_DEVICE_ID }, /* DEVREG_ID */
	{ 0x93, 0x00 },          /* DEVREG_STATUS */
	{ 0x94, 0x00 },          /* DEVREG_ASTATUS */
	{ 0x95, 0x00 },          /* DEVREG_ADATAOL */
	{ 0x96, 0x00 },          /* DEVREG_ADATAOH */
	{ 0x97, 0x00 },          /* DEVREG_ADATA1L */
	{ 0x98, 0x00 },          /* DEVREG_ADATA1H */
	{ 0x99, 0x00 },          /* DEVREG_ADATA2L */
	{ 0x9A, 0x00 },          /* DEVREG_ADATA2H */
	{ 0x9B, 0x00 },          /* DEVREG_ADATA3L */
	{ 0x9C, 0x00 },          /* DEVREG_ADATA3H */
	{ 0x9D, 0x00 },          /* DEVREG_ADATA4L */
	{ 0x9E, 0x00 },          /* DEVREG_ADATA4H */
	{ 0x9F, 0x00 },          /* DEVREG_ADATA5L */

	{ 0xA0, 0x00 },          /* DEVREG_ADATA5H */
	{ 0xA3, 0x00 },          /* DEVREG_STATUS2 */
	{ 0xA4, 0x00 },          /* DEVREG_STATUS3 */
	{ 0xA6, 0x00 },          /* DEVREG_STATUS5 */
	{ 0xA7, 0x00 },          /* DEVREG_STATUS4 */
	/* 0xA8 Reserved */
	{ 0xA9, 0x40 },          /* DEVREG_CFG0 */
	{ 0xAA, 0x09 },          /* DEVREG_CFG1 */
	{ 0xAC, 0x0C },          /* DEVREG_CFG3 */
	{ 0xAD, 0x00 },          /* DEVREG_CFG4 */
	{ 0xAF, 0x00 },          /* DEVREG_CFG6 */

	{ 0xB1, 0x98 },          /* DEVREG_CFG8 */
	{ 0xB2, 0x00 },          /* DEVREG_CFG9 */
	{ 0xB3, 0xF2 },          /* DEVREG_CFG10 */
	{ 0xB4, 0x4D },          /* DEVREG_CFG11 */
	{ 0xB5, 0x00 },          /* DEVREG_CFG12 */
	{ 0xBD, 0x00 },          /* DEVREG_PERS */
	{ 0xBE, 0x02 },          /* DEVREG_GPIO */

	{ 0xCA, 0xE7 },          /* DEVREG_ASTEPL */
	{ 0xCB, 0x03 },          /* DEVREG_ASTEPH */
	{ 0xCF, 0X97 }, 		 /* DEVREG_AGC_GAIN_MAX */

	{ 0xD6, 0xFf },          /* DEVREG_AZ_CONFIG */
	{ 0xD7, 0x21},           /*DEVREG_FD_CFG0*/
	{ 0xD8, 0x68},           /*DEVREG_FD_CFG1*/
	{ 0xD9, 0x64},           /*DEVREG_FD_CFG2*/
	{ 0xDA, 0x91 },           /*DEVREG_FD_CFG3*/
	{ 0xDB, 0x00 },          /* DEVREG_FD_STATUS */
	/* 0xEF-0xF8 Reserved */
	{ 0xF9, 0x00 },          /* DEVREG_INTENAB */
	{ 0xFA, 0x00 },          /* DEVREG_CONTROL */
	{ 0xFC, 0x00 },          /* DEVREG_FIFO_MAP */
	{ 0xFD, 0x00 },          /* DEVREG_FIFO_STATUS */
	{ 0xFE, 0x00 },          /* DEVREG_FDATAL */
	{ 0xFF, 0x00 },          /* DEVREG_FDATAH */
	{ 0x6f, 0x00 },          /* DEVREG_FLKR_WA_RAMLOC_1 */
	{ 0x71, 0x00 },          /* DEVREG_FLKR_WA_RAMLOC_2 */
	{ 0xF3, 0x00 },          /* DEVREG_SOFT_RESET */
};

uint32_t alsGain_conversion[] = {
	1000 / 2,
	1 * 1000,
	2 * 1000,
	4 * 1000,
	8 * 1000,
	16 * 1000,
	32 * 1000,
	64 * 1000,
	128 * 1000,
	256 * 1000,
	512 * 1000
};

static const struct of_device_id tcs3407_match_table[] = {
	{ .compatible = "ams,tcs3407",},
	{},
};

#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
static long tcs3407_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret = 0;

	struct tcs3407_device_data *data = container_of(file->private_data,
	struct tcs3407_device_data, miscdev);

	ALS_dbg("%s - ioctl start, %d\n", __func__, cmd);
	mutex_lock(&data->flickerdatalock);

	switch (cmd) {
	case TCS3407_IOCTL_READ_FLICKER:
		ALS_dbg("%s - TCS3407_IOCTL_READ_FLICKER = %d\n", __func__, data->flicker_data[0]);

		ret = copy_to_user(argp,
			data->flicker_data,
			sizeof(int)*FLICKER_DATA_CNT);

		if (unlikely(ret))
			goto ioctl_error;

		break;

	default:
		ALS_err("%s - invalid cmd\n", __func__);
		break;
	}

	mutex_unlock(&data->flickerdatalock);
	return ret;

ioctl_error:
	mutex_unlock(&data->flickerdatalock);
	ALS_err("%s - read flicker data err(%d)\n", __func__, ret);
	return -ret;
}

static const struct file_operations tcs3407_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.unlocked_ioctl = tcs3407_ioctl,
};
#endif

static uint8_t alsGainToReg(uint32_t x)
{
	int i;

	for (i = sizeof(alsGain_conversion)/sizeof(uint32_t)-1; i != 0; i--) {
		if (x >= alsGain_conversion[i])
			break;
	}
	return (i << 0);
}

static uint16_t alsTimeUsToReg(uint32_t x)
{
	uint16_t regValue;

	regValue = (x / 2816);

	return regValue;
}

static void tcs3407_debug_var(struct tcs3407_device_data *data)
{
	ALS_dbg("===== %s =====\n", __func__);
	ALS_dbg("%s client %p slave_addr 0x%x\n", __func__,
		data->client, data->client->addr);
	ALS_dbg("%s dev %p\n", __func__, data->dev);
	ALS_dbg("%s als_input_dev %p\n", __func__, data->als_input_dev);
	ALS_dbg("%s als_pinctrl %p\n", __func__, data->als_pinctrl);
	ALS_dbg("%s pins_sleep %p\n", __func__, data->pins_sleep);
	ALS_dbg("%s pins_idle %p\n", __func__, data->pins_idle);
	ALS_dbg("%s vdd_1p8 %s\n", __func__, data->vdd_1p8);
	ALS_dbg("%s i2c_1p8 %s\n", __func__, data->i2c_1p8);
	ALS_dbg("%s als_enabled %d\n", __func__, data->enabled);
	ALS_dbg("%s als_sampling_rate %d\n", __func__, data->sampling_period_ns);
	ALS_dbg("%s regulator_state %d\n", __func__, data->regulator_state);
	ALS_dbg("%s als_int %d\n", __func__, data->pin_als_int);
	ALS_dbg("%s als_en %d\n", __func__, data->pin_als_en);
	ALS_dbg("%s als_irq %d\n", __func__, data->dev_irq);
	ALS_dbg("%s irq_state %d\n", __func__, data->irq_state);
	ALS_dbg("===== %s =====\n", __func__);
}

static int tcs3407_write_reg(struct tcs3407_device_data *device,
	u8 reg_addr, u8 data)
{
	int err = -1;
	int tries = 0;
	int num = 1;
	u8 buffer[2] = { reg_addr, data };
	struct i2c_msg msgs[] = {
		{
			.addr = device->client->addr,
			.flags = device->client->flags & I2C_M_TEN,
			.len = 2,
			.buf = buffer,
		},
	};

	if (!device->pm_state || device->regulator_state == 0) {
		ALS_err("%s - write error, pm suspend or reg_state %d\n",
				__func__, device->regulator_state);
		err = -EFAULT;
		return err;
	}
	mutex_lock(&device->suspendlock);

	do {
		err = i2c_transfer(device->client->adapter, msgs, num);
		if (err != num)
			msleep_interruptible(AMSDRIVER_I2C_RETRY_DELAY);
		if (err < 0)
			ALS_err("%s - i2c_transfer error = %d\n", __func__, err);
	} while ((err != num) && (++tries < AMSDRIVER_I2C_MAX_RETRIES));

	mutex_unlock(&device->suspendlock);

	if (err != num) {
		ALS_err("%s -write transfer error:%d\n", __func__, err);
		err = -EIO;
		device->i2c_err_cnt++;
		return err;
	}

	return 0;
}

static int tcs3407_read_reg(struct tcs3407_device_data *device,
	u8 reg_addr, u8 *buffer, int length)
{
	int err = -1;
	int tries = 0; /* # of attempts to read the device */
	int num = 2;
	struct i2c_msg msgs[] = {
		{
			.addr = device->client->addr,
			.flags = device->client->flags & I2C_M_TEN,
			.len = 1,
			.buf = buffer,
		},
		{
			.addr = device->client->addr,
			.flags = (device->client->flags & I2C_M_TEN) | I2C_M_RD,
			.len = length,
			.buf = buffer,
		},
	};

	if (!device->pm_state || device->regulator_state == 0) {
		ALS_err("%s - read error, pm suspend or reg_state %d\n",
				__func__, device->regulator_state);
		err = -EFAULT;
		return err;
	}
	mutex_lock(&device->suspendlock);

	do {
		buffer[0] = reg_addr;
		err = i2c_transfer(device->client->adapter, msgs, num);
		if (err != num)
			msleep_interruptible(AMSDRIVER_I2C_RETRY_DELAY);
		if (err < 0)
			ALS_err("%s - i2c_transfer error = %d\n", __func__, err);
	} while ((err != num) && (++tries < AMSDRIVER_I2C_MAX_RETRIES));

	mutex_unlock(&device->suspendlock);

	if (err != num) {
		ALS_err("%s -read transfer error:%d\n", __func__, err);
		err = -EIO;
		device->i2c_err_cnt++;
	} else
		err = 0;

	return err;
}

static int ams_getByte(AMS_PORT_portHndl *portHndl, ams_deviceRegister_t reg, uint8_t *readData)
{
	struct tcs3407_device_data *data = i2c_get_clientdata(portHndl);
	int err = 0;
	uint8_t length = 1;

	/* Sanity check input param */
	if (reg >= DEVREG_REG_MAX)
		return 0;

	err = tcs3407_read_reg(data, deviceRegisterDefinition[reg].address, readData, length);

	return err;
}

static int ams_setByte(AMS_PORT_portHndl *portHndl, ams_deviceRegister_t reg, uint8_t setData)
{
	struct tcs3407_device_data *data = i2c_get_clientdata(portHndl);
	int err = 0;

	/* Sanity check input param */
	if (reg >= DEVREG_REG_MAX)
		return 0;

	err = tcs3407_write_reg(data, deviceRegisterDefinition[reg].address, setData);

	return err;
}

static int ams_getBuf(AMS_PORT_portHndl *portHndl, ams_deviceRegister_t reg, uint8_t *readData, uint8_t length)
{
	struct tcs3407_device_data *data = i2c_get_clientdata(portHndl);
	int err = 0;

	/* Sanity check input param */
	if (reg >= DEVREG_REG_MAX)
		return 0;

	err = tcs3407_read_reg(data, deviceRegisterDefinition[reg].address, readData, length);

	return err;
}

int ams_setBuf(AMS_PORT_portHndl *portHndl, ams_deviceRegister_t reg, uint8_t *setData, uint8_t length)
{
	struct tcs3407_device_data *data = i2c_get_clientdata(portHndl);
	int err = 0;

	/* Sanity check input param */
	if (reg >= DEVREG_REG_MAX)
		return 0;

	err = tcs3407_write_reg(data, deviceRegisterDefinition[reg].address, *setData);

	return err;
}

int ams_getWord(AMS_PORT_portHndl *portHndl, ams_deviceRegister_t reg, uint16_t *readData)
{
	struct tcs3407_device_data *data = i2c_get_clientdata(portHndl);
	int err = 0;
	uint8_t length = sizeof(uint16_t);
	uint8_t buffer[sizeof(uint16_t)];

	/* Sanity check input param */
	if (reg >= DEVREG_REG_MAX)
		return 0;

	err = tcs3407_read_reg(data, deviceRegisterDefinition[reg].address, buffer, length);

	*readData = ((buffer[0] << AMS_ENDIAN_1) + (buffer[1] << AMS_ENDIAN_2));

	return err;
}

static int ams_setWord(AMS_PORT_portHndl *portHndl, ams_deviceRegister_t reg, uint16_t setData)
{
	struct tcs3407_device_data *data = i2c_get_clientdata(portHndl);
	int err = 0;
	uint8_t buffer[sizeof(uint16_t)];

	/* Sanity check input param */
	if (reg >= (DEVREG_REG_MAX - 1))
		return 0;

	buffer[0] = ((setData >> AMS_ENDIAN_1) & 0xff);
	buffer[1] = ((setData >> AMS_ENDIAN_2) & 0xff);

	err = tcs3407_write_reg(data, deviceRegisterDefinition[reg].address, buffer[0]);
	err = tcs3407_write_reg(data, deviceRegisterDefinition[reg + 1].address, buffer[1]);

	return err;
}

int ams_getField(AMS_PORT_portHndl *portHndl, ams_deviceRegister_t reg, uint8_t *setData, ams_regMask_t mask)
{
	struct tcs3407_device_data *data = i2c_get_clientdata(portHndl);
	int err = 0;
	uint8_t length = 1;

	/* Sanity check input param */
	if (reg >= DEVREG_REG_MAX)
		return 0;

	err = tcs3407_read_reg(data, deviceRegisterDefinition[reg].address, setData, length);

	*setData &= mask;

	return err;
}

static int ams_setField(AMS_PORT_portHndl *portHndl, ams_deviceRegister_t reg, uint8_t setData, ams_regMask_t mask)
{
	struct tcs3407_device_data *data = i2c_get_clientdata(portHndl);
	int err = 1;
	uint8_t length = 1;
	uint8_t original_data;
	uint8_t new_data;

	/* Sanity check input param */
	if (reg >= DEVREG_REG_MAX)
		return 0;

	err = tcs3407_read_reg(data, deviceRegisterDefinition[reg].address, &original_data, length);
	if (err < 0)
		return err;

	new_data = original_data & ~mask;
	new_data |= (setData & mask);

	if (new_data != original_data)
		err = tcs3407_write_reg(data, deviceRegisterDefinition[reg].address, new_data);

	return err;
}

static void als_getDefaultCalibrationData(ams_ccb_als_calibration_t *data)
{
	if (data != NULL) {
		data->Time_base = AMS_ALS_TIMEBASE;
		data->thresholdLow = AMS_ALS_THRESHOLD_LOW;
		data->thresholdHigh = AMS_ALS_THRESHOLD_HIGH;
		data->calibrationFactor = 1000;
	}
}

static void als_update_statics(amsAlsContext_t *ctx);

static int amsAlg_als_processData(amsAlsContext_t *ctx, amsAlsDataSet_t *inputData)
{
	uint32_t tempIr = 0, tempWb = 0, tempClear = 0;
	uint32_t UVIR_Clear = 0;
	uint32_t UVIR_wideband = 0;

	ALS_info("%s - raw: %d, %d, %d\n", __func__, ctx->uvir_cpl, ctx->gain, ctx->time_us);
	ALS_info("%s - raw: %d, %d, %d, %d, %d\n", __func__, inputData->datasetArray->clearADC,
				inputData->datasetArray->redADC, inputData->datasetArray->greenADC,
				inputData->datasetArray->blueADC, inputData->datasetArray->widebandADC);

	if (inputData->status & ALS_STATUS_RDY) {
		if (ctx->previousGain != ctx->gain)
			als_update_statics(ctx);

		ctx->results.rawClear = inputData->datasetArray->clearADC;
		ctx->results.rawRed = inputData->datasetArray->redADC;
		ctx->results.rawGreen = inputData->datasetArray->greenADC;
		ctx->results.rawBlue = inputData->datasetArray->blueADC;
		ctx->results.rawWideband = inputData->datasetArray->widebandADC;

	    /* Calculate IR */
		tempIr = inputData->datasetArray->redADC +
					inputData->datasetArray->greenADC +
					inputData->datasetArray->blueADC;
		if (tempIr > inputData->datasetArray->clearADC)
			ctx->results.IR = (tempIr - inputData->datasetArray->clearADC) / 2;
		else
			ctx->results.IR = 0;

		/*Calculate UV+IR*/
		ctx->results.irrRed = ((inputData->datasetArray->redADC * (AMS_ALS_Rc / CPU_FRIENDLY_FACTOR_1024))) / ctx->uvir_cpl;
		ctx->results.irrClear = ((inputData->datasetArray->clearADC * (AMS_ALS_Cc / CPU_FRIENDLY_FACTOR_1024))) / ctx->uvir_cpl;
		ctx->results.irrBlue = ((inputData->datasetArray->blueADC * (AMS_ALS_Bc / CPU_FRIENDLY_FACTOR_1024))) / ctx->uvir_cpl;
		ctx->results.irrGreen = ((inputData->datasetArray->greenADC * (AMS_ALS_Gc / CPU_FRIENDLY_FACTOR_1024))) / ctx->uvir_cpl;

		UVIR_Clear = (inputData->datasetArray->clearADC * (AMS_ALS_Cc / CPU_FRIENDLY_FACTOR_1024)) / ctx->uvir_cpl;
		UVIR_wideband = (inputData->datasetArray->widebandADC * (AMS_ALS_Wbc / CPU_FRIENDLY_FACTOR_1024)) / ctx->uvir_cpl;

		tempWb = WIDEBAND_CONST * UVIR_wideband;
		tempClear = (CLEAR_CONST * UVIR_Clear) >> 1;

		if (tempWb < tempClear)
			ctx->results.irrWideband = 0;
		else
			ctx->results.irrWideband = tempWb - tempClear;
	}

	ALS_info("%s - cal: %d, %d, %d, %d, %d, %d, %d\n", __func__,
				ctx->results.irrClear, ctx->results.irrRed, ctx->results.irrGreen,
				ctx->results.irrBlue, ctx->results.irrWideband,	UVIR_Clear,	UVIR_wideband);

	return 0;
}

static bool ams_getMode(ams_deviceCtx_t *ctx, ams_mode_t *mode)
{
	*mode = ctx->mode;

	return false;
}

uint32_t ams_getResult(ams_deviceCtx_t *ctx)
{
	uint32_t returnValue = ctx->updateAvailable;

	ctx->updateAvailable = 0;

	return returnValue;
}

static int amsAlg_als_initAlg(amsAlsContext_t *ctx, amsAlsInitData_t *initData);
static int amsAlg_als_getAlgInfo(amsAlsAlgoInfo_t *info);
static int amsAlg_als_processData(amsAlsContext_t *ctx, amsAlsDataSet_t *inputData);

static int ccb_alsInit(void *dcbCtx, ams_ccb_als_init_t *initData)
{
	ams_deviceCtx_t *ctx = (ams_deviceCtx_t *)dcbCtx;
	ams_ccb_als_ctx_t *ccbCtx = &ctx->ccbAlsCtx;
	amsAlsInitData_t initAlsData;
	amsAlsAlgoInfo_t infoAls;
	int ret = 0;

	ALS_dbg("%s - ccb_proximityInit\n", __func__);

	if (initData)
		memcpy(&ccbCtx->initData, initData, sizeof(ams_ccb_als_init_t));
	else
		ccbCtx->initData.calibrate = false;

	initAlsData.adaptive = false;
	initAlsData.irRejection = false;
	initAlsData.gain = ccbCtx->initData.configData.gain;
#ifdef AMS_ALS_GAIN_V2
	ctx->alsGain = initAlsData.gain;
#endif
	initAlsData.time_us = ccbCtx->initData.configData.uSecTime;
	initAlsData.calibration.adcMaxCount = ccbCtx->initData.calibrationData.adcMaxCount;
	initAlsData.calibration.calibrationFactor = ccbCtx->initData.calibrationData.calibrationFactor;
	initAlsData.calibration.Time_base = ccbCtx->initData.calibrationData.Time_base;
	initAlsData.calibration.thresholdLow = ccbCtx->initData.calibrationData.thresholdLow;
	initAlsData.calibration.thresholdHigh = ccbCtx->initData.calibrationData.thresholdHigh;
	//initAlsData.calibration.calibrationFactor = ccbCtx->initData.calibrationData.calibrationFactor;
	amsAlg_als_getAlgInfo(&infoAls);

#ifdef CONFIG_AMS_OPTICAL_SENSOR_3407
	/* get gain from HW register if so configured */
	if (ctx->ccbAlsCtx.initData.autoGain) {
		uint32_t scaledGain;
		uint8_t gain;

		AMS_GET_ALS_GAIN(scaledGain, gain, ret);
		if (ret < 0) {
			ALS_err("%s - failed to AMS_GET_ALS_GAIN\n", __func__);
			return ret;
		}
		ctx->ccbAlsCtx.ctxAlgAls.gain = scaledGain;
	}
#endif
	amsAlg_als_initAlg(&ccbCtx->ctxAlgAls, &initAlsData);

	AMS_SET_ALS_TIME(ccbCtx->initData.configData.uSecTime, ret);
	if (ret < 0) {
		ALS_err("%s - failed to AMS_SET_ALS_TIME\n", __func__);
		return ret;
	}
	AMS_SET_ALS_PERS(0x01, ret);
	if (ret < 0) {
		ALS_err("%s - failed to AMS_SET_ALS_PERS\n", __func__);
		return ret;
	}
	AMS_SET_ALS_GAIN(ctx->ccbAlsCtx.initData.configData.gain, ret);
	if (ret < 0) {
		ALS_err("%s - failed to AMS_SET_ALS_GAIN\n", __func__);
		return ret;
	}

	ccbCtx->shadowAiltReg = 0xffff;
	ccbCtx->shadowAihtReg = 0;
	AMS_SET_ALS_THRS_LOW(ccbCtx->shadowAiltReg, ret);
	if (ret < 0) {
		ALS_err("%s - failed to AMS_SET_ALS_THRS_LOW\n", __func__);
		return ret;
	}
	AMS_SET_ALS_THRS_HIGH(ccbCtx->shadowAihtReg, ret);
	if (ret < 0) {
		ALS_err("%s - failed to AMS_SET_ALS_THRS_HIGH\n", __func__);
		return ret;
	}

	ccbCtx->state = AMS_CCB_ALS_RGB;

	return ret;
}

void ccb_alsInit_FIFO(void *dcbCtx, ams_ccb_als_init_t *initData)
{
	ams_deviceCtx_t *ctx = (ams_deviceCtx_t *)dcbCtx;
	ams_ccb_als_ctx_t *ccbCtx = &ctx->ccbAlsCtx;
	amsAlsInitData_t initAlsData;
	int ret = 0;

	if (initData)
		memcpy(&ccbCtx->initData, initData, sizeof(ams_ccb_als_init_t));
	else
		ccbCtx->initData.calibrate = false;

	initAlsData.gain = ccbCtx->initData.configData.gain;
#ifdef AMS_ALS_GAIN_V2
	ctx->alsGain = initAlsData.gain;
#endif
	initAlsData.time_us = ccbCtx->initData.configData.uSecTime;
	initAlsData.calibration.adcMaxCount = ccbCtx->initData.calibrationData.adcMaxCount;
	initAlsData.calibration.calibrationFactor = ccbCtx->initData.calibrationData.calibrationFactor;
	initAlsData.calibration.Time_base = ccbCtx->initData.calibrationData.Time_base;
	initAlsData.calibration.thresholdLow = ccbCtx->initData.calibrationData.thresholdLow;
	initAlsData.calibration.thresholdHigh = ccbCtx->initData.calibrationData.thresholdHigh;
	initAlsData.calibration.Wbc = ccbCtx->initData.calibrationData.Wbc;

	ALS_dbg("%s - ccb_alsInit time %d, gain value %d , autogain %d\n",
		__func__, initAlsData.time_us, initAlsData.gain, ctx->ccbAlsCtx.initData.autoGain);

#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
	ALS_dbg("%s - !!!!start %s!!!!!", __func__, __func__);

	//ams_setByte (ctx->portHndl, DEVREG_CFG6,  0x10);
	//ams_setByte(ctx->portHndl, DEVREG_ALS_CHANNEL_CTRL, 0x1e);
	/* SMUX write command */
	ams_setByte(ctx->portHndl, DEVREG_CONTROL, 0x02); //FIFO Buffer , FINT, FIFO_OV, FIFO_LVL all clear

	AMS_SET_ALS_GAIN(1000, ret);

	//ams_setByte(ctx->portHndl, DEVREG_CFG4, 0x80);
	ams_setByte(ctx->portHndl, DEVREG_FD_CFG0, 0x80);
	ams_setByte(ctx->portHndl, DEVREG_AZ_CONFIG, 0x00);

	//ams_setByte(ctx->portHndl, DEVREG_ATIME, 0x00);
	ams_setByte(ctx->portHndl, DEVREG_WTIME, 0x00);
	ams_setByte(ctx->portHndl, DEVREG_FIFO_MAP, 0x40); // ADATA5, flikcer
	ams_setByte(ctx->portHndl, DEVREG_CFG8, 0xC8); //FIFO_THR:16, FD_AGC_DISABLE:YES
	//ams_setField(ctx->portHndl,  DEVREG_PCFG1, 0x08, 0x08);

	// set 150us
	//ams_setField(ctx->portHndl,  DEVREG_PCFG1, 0x08, 0x08);

	AMS_SET_ALS_TIME(0, ret);
	ccbCtx->initData.configData.uSecTime = 2532;
	AMS_SET_ALS_STEP_TIME(ccbCtx->initData.configData.uSecTime, ret); /*2.78msec*/

	AMS_SET_ALS_PERS(0x00, ret);
	/* force interrupt */
	ccbCtx->shadowAiltReg = 0xffff;
	ccbCtx->shadowAihtReg = 0;
	AMS_SET_ALS_THRS_LOW(ccbCtx->shadowAiltReg, ret);
	AMS_SET_ALS_THRS_HIGH(ccbCtx->shadowAihtReg, ret);

	//ccbCtx->state = AMS_CCB_ALS_INIT;
#else
	AMS_SET_ALS_TIME(ccbCtx->initData.configData.uSecTime, ret);
	AMS_SET_ALS_PERS(0x01, ret);
	AMS_SET_ALS_GAIN(ctx->ccbAlsCtx.initData.configData.gain, ret);

	ccbCtx->shadowAiltReg = 0xffff;
	ccbCtx->shadowAihtReg = 0;
	AMS_SET_ALS_THRS_LOW(ccbCtx->shadowAiltReg, ret);
	AMS_SET_ALS_THRS_HIGH(ccbCtx->shadowAihtReg, ret);

	ccbCtx->state = AMS_CCB_ALS_RGB;
#endif
}

static void ccb_alsInfo(ams_ccb_als_info_t *infoData)
{
	if (infoData != NULL) {
		infoData->algName = "ALS";
		infoData->contextMemSize = sizeof(ams_ccb_als_ctx_t);
		infoData->scratchMemSize = 0;
		infoData->defaultCalibrationData.calibrationFactor = 1000;
		//infoData->defaultCalibrationData.luxTarget = CONFIG_ALS_CAL_TARGET;
		//infoData->defaultCalibrationData.luxTargetError = CONFIG_ALS_CAL_TARGET_TOLERANCE;
		als_getDefaultCalibrationData(&infoData->defaultCalibrationData);
	}
}

static void ccb_alsSetConfig(void *dcbCtx, ams_ccb_als_config_t *configData)
{
	ams_ccb_als_ctx_t *ccbCtx = &((ams_deviceCtx_t *)dcbCtx)->ccbAlsCtx;

	ccbCtx->initData.configData.threshold = configData->threshold;
}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
static bool ccb_FlickerFIFOEvent(void *dcbCtx, ams_ccb_als_dataSet_t *alsData)
{
	ams_deviceCtx_t *ctx = (ams_deviceCtx_t *)dcbCtx;
	ams_ccb_als_ctx_t *ccbCtx = &((ams_deviceCtx_t *)dcbCtx)->ccbAlsCtx;

	//ams_flicker_ctx_t *flickerCtx = (ams_flicker_ctx_t *)&ctx->flickerCtx;
	int i = 0;
	uint8_t fifo_lvl;
	uint8_t fifo_ov;
	uint16_t fifo_size, quotient, remainder;
	uint16_t data = 0;
	static adcDataSet_t adcData;
	int ret = 0;

	//static uint8_t fifo_buffer[256]={0,};

	ams_getByte(ctx->portHndl, DEVREG_FIFO_STATUS, &fifo_lvl); //current fifo count
	ams_getByte(ctx->portHndl, DEVREG_STATUS4, &fifo_ov);

#ifdef FLICKER_FIFO_THR
	fifo_size = FLICKER_FIFO_THR * BYTE;
#else
	fifo_size = fifo_lvl * BYTE;
#endif
	quotient = fifo_size / 32;
	remainder = fifo_size % 32;

	ALS_dbg("%s - FIFO LVL = %d, FIFO_OV = %d\n", __func__, fifo_lvl, fifo_ov);

	if (fifo_lvl >= FLICKER_FIFO_THR) { //>128
		if (quotient == 0) {// fifo size is less than 32 , reading remainder
			ams_getBuf(ctx->portHndl, DEVREG_FDATAL, (uint8_t *)&tcs3407_data->fifodata[0], remainder);
		} else {
			for (i = 0; i < quotient; i++)
				ams_getBuf(ctx->portHndl, DEVREG_FDATAL, (uint8_t *)&tcs3407_data->fifodata[i * 32], 32);

			if (remainder != 0)
				ams_getBuf(ctx->portHndl, DEVREG_FDATAL, (uint8_t *)&tcs3407_data->fifodata[i * 32], remainder);
		}
		ALS_dbg("%s - ~~~~FIFO full~~~~level %d overflow %d size %d\n", __func__, fifo_lvl, fifo_ov, fifo_size);

		for (i = 0; i < fifo_size; i += 2) { // read 256 byte
			data = (u16)((tcs3407_data->fifodata[i] << 0) | (tcs3407_data->fifodata[i + 1] << 8));
			tcs3407_data->flicker_data[tcs3407_data->flicker_data_cnt++] = data;

			ALS_dbg("%s - data[%d] = %u, data_cnt = %d\n", __func__, i, data, tcs3407_data->flicker_data_cnt);

			if (tcs3407_data->flicker_data_cnt >= 200) {
				tcs3407_data->flicker_data_cnt = 0;
				input_report_rel(tcs3407_data->als_input_dev, REL_RZ, (FLICKER_FIFO_READ + 1));
				input_sync(tcs3407_data->als_input_dev);
				break;
			}
		}

		input_report_rel(tcs3407_data->als_input_dev, REL_RY, data + 1);
		input_sync(tcs3407_data->als_input_dev);

		ams_setByte(ctx->portHndl, DEVREG_CONTROL, 0x02); //FIFO Buffer , FINT, FIFO_OV, FIFO_LVL all clear
	}

#ifdef ALS_AUTOGAIN
	// Do S/W AGC
	AMS_ALS_GET_CRGB_W(&adcData, ret);
	{
		uint64_t temp;
		uint32_t recommendedGain;
		uint32_t max_count;
		uint32_t adcObjective;

		max_count = (1024 * ccbCtx->initData.configData.uSecTime) / 2780;
		//uint32_t adcObjective = ctx->ccbAlsCtx.ctxAlgAls.saturation * 128;
		adcObjective = max_count * 128;
		//adcObjective /= 160; /* about 80% (128 / 160) */
		adcObjective /= 400; /* about 80% (128 / 160) */

		if (adcData.AdcClear == 0) {
			/* to avoid divide by zero */
			adcData.AdcClear = 1;
		}
		temp = adcObjective * 2048; /* 2048 to avoid floating point operation later on */
		do_div(temp, adcData.AdcClear);

		temp *= ctx->ccbAlsCtx.ctxAlgAls.gain;
		do_div(temp, 2048);

		recommendedGain = temp & 0xffffffff;

		ALS_dbg("%s - Clear ADC : %d, Red  ADC : %d, Green ADC : %d, Blue ADC : %d\n",
			__func__, adcData.AdcClear, adcData.AdcRed, adcData.AdcGreen, adcData.AdcBlue);

		ALS_dbg("%s - AMS_CCB_ALS_AUTOGAIN: sat=%u, objctv=%u, cur=%u, rec=%u",
			__func__,
			ctx->ccbAlsCtx.ctxAlgAls.saturation,
			adcObjective,
			ctx->ccbAlsCtx.ctxAlgAls.gain,
			recommendedGain);

		recommendedGain = alsGainToReg(recommendedGain);
		recommendedGain = alsGain_conversion[recommendedGain];
		if (recommendedGain != ctx->ccbAlsCtx.ctxAlgAls.gain) {
			ALS_dbg("%s - gain chg to: %u\n", __func__, recommendedGain);
			ctx->ccbAlsCtx.ctxAlgAls.gain = recommendedGain;
			//ccbCtx->alg_config.gain = recommendedGain/1000;
			AMS_DISABLE_ALS(ret);

			ctx->shadowIntenabReg &= ~(FIEN|ASIEN_FDSIEN);
			ams_setField(ctx->portHndl, DEVREG_INTENAB, HIGH, ctx->shadowIntenabReg);/*disable*/

			AMS_SET_ALS_GAIN(ctx->ccbAlsCtx.ctxAlgAls.gain, ret);

			ams_setByte(ctx->portHndl, DEVREG_CONTROL, 0x02); //FIFO Buffer , FINT, FIFO_OV, FIFO_LVL all clear

			ctx->shadowIntenabReg = (FIEN|ASIEN_FDSIEN);
			ams_setField(ctx->portHndl, DEVREG_INTENAB, HIGH, ctx->shadowIntenabReg);/*enable*/

			AMS_REENABLE_ALS(ret);
		} else
			ALS_dbg("%s - no chg, gain %u\n", __func__, ctx->ccbAlsCtx.ctxAlgAls.gain);
	}
#endif

	//AMS_PORT_log_1("Last ccb_alsHd: buffer count %d\n" , buffer_count);
	return false;
}
#endif

static int ccb_alsHandle(void *dcbCtx, ams_ccb_als_dataSet_t *alsData)
{
	ams_deviceCtx_t *ctx = (ams_deviceCtx_t *)dcbCtx;
	ams_ccb_als_ctx_t *ccbCtx = &((ams_deviceCtx_t *)dcbCtx)->ccbAlsCtx;
	amsAlsDataSet_t inputDataAls;
	static adcDataSet_t adcData; /* QC - is this really needed? */
	int ret = 0;
/*
 *	uint64_t temp;
 *	uint32_t recommendedGain;
 *	uint32_t adcObjective;
 *
 *	adcDataSet_t tmpAdcData;
 *	amsAlsContext_t tmp;
 *	amsAlsDataSet_t tmpInputDataAls;
 */
	//ALS_info(AMS_DEBUG, "ccb_alsHandle: case = %d\n", ccbCtx->state);
#if 0
	AMS_ALS_GET_CRGB_W((uint8_t *)&adcData);
	AMS_PORT_LOG_CRGB_W(adcData);
#endif
#ifdef CONFIG_AMS_OPTICAL_SENSOR_3407
	/* get gain from HW register if so configured */
	if (ctx->ccbAlsCtx.initData.autoGain) {
		uint32_t scaledGain;
#ifdef AMS_ALS_GAIN_V2
		uint8_t gainLevel;

		AMS_GET_ALS_GAIN(scaledGain, gainLevel, ret);
		if (ret < 0) {
			ALS_err("%s - failed to AMS_GET_ALS_GAIN\n", __func__);
			return ret;
		}

#else
		uint8_t gain;

		AMS_GET_ALS_GAIN(scaledGain, gain, ret);
		if (ret < 0) {
			ALS_err("%s - failed to AMS_GET_ALS_GAIN\n", __func__);
			return ret;
		}

#endif
		ctx->ccbAlsCtx.ctxAlgAls.gain = scaledGain;
	}
#endif

	switch (ccbCtx->state) {
	case AMS_CCB_ALS_INIT:
		AMS_DISABLE_ALS(ret);
		if (ret < 0) {
			ALS_err("%s - failed to AMS_DISABLE_ALS\n", __func__);
			return ret;
		}
		AMS_SET_ALS_TIME(ccbCtx->initData.configData.uSecTime, ret);
		if (ret < 0) {
			ALS_err("%s - failed to AMS_SET_ALS_TIME\n", __func__);
			return ret;
		}
		AMS_SET_ALS_PERS(0x01, ret);
		if (ret < 0) {
			ALS_err("%s - failed to AMS_SET_ALS_PERS\n", __func__);
			return ret;
		}

		AMS_SET_ALS_GAIN(ctx->ccbAlsCtx.initData.configData.gain, ret);
		if (ret < 0) {
			ALS_err("%s - failed to AMS_SET_ALS_GAIN\n", __func__);
			return ret;
		}
		/* force interrupt */
		ccbCtx->shadowAiltReg = 0xffff;
		ccbCtx->shadowAihtReg = 0;
		AMS_SET_ALS_THRS_LOW(ccbCtx->shadowAiltReg, ret);
		if (ret < 0) {
			ALS_err("%s - failed to AMS_SET_ALS_THRS_LOW\n", __func__);
			return ret;
		}
		AMS_SET_ALS_THRS_HIGH(ccbCtx->shadowAihtReg, ret);
		if (ret < 0) {
			ALS_err("%s - failed to AMS_SET_ALS_THRS_HIGH\n", __func__);
			return ret;
		}
		ccbCtx->state = AMS_CCB_ALS_RGB;
		AMS_REENABLE_ALS(ret);
		if (ret < 0) {
			ALS_err("%s - failed to AMS_REENABLE_ALS\n", __func__);
			return ret;
		}
		break;
	case AMS_CCB_ALS_RGB: /* state to measure RGB */
#ifdef HAVE_OPTION__ALWAYS_READ
		if ((alsData->statusReg & (AINT)) || ctx->alwaysReadAls)
#else
		if (alsData->statusReg & (AINT))
#endif
		{
			AMS_ALS_GET_CRGB_W(&adcData, ret);
			if (ret < 0) {
				ALS_err("%s - failed to AMS_ALS_GET_CRGB_W\n", __func__);
				return ret;
			}
			inputDataAls.status = ALS_STATUS_RDY;
			inputDataAls.datasetArray = (alsData_t *)&adcData;
			AMS_PORT_LOG_CRGB_W(adcData);

			/* if (ctx->ccbAlsCtx.ctxAlgAls.previousGain !=
			 *			ctx->ccbAlsCtx.ctxAlgAls.gain) {
			 *			AMS_DISABLE_ALS();
			 *			ALS_info(AMS_DEBUG, "ccb_alsHandle: ALS Disalbe to Enable\n");
			 *			AMS_REENABLE_ALS();
			 * }
			 */

			amsAlg_als_processData(&ctx->ccbAlsCtx.ctxAlgAls, &inputDataAls);

			if (ctx->mode & MODE_ALS_LUX)
				ctx->updateAvailable |= (1 << AMS_AMBIENT_SENSOR);

			ccbCtx->state = AMS_CCB_ALS_RGB;
		}
		break;

	default:
		ccbCtx->state = AMS_CCB_ALS_RGB;
		break;
	}
	return false;
}

static void ccb_alsGetResult(void *dcbCtx, ams_ccb_als_result_t *exportData)
{
	ams_ccb_als_ctx_t *ccbCtx = &((ams_deviceCtx_t *)dcbCtx)->ccbAlsCtx;

	/* export data */
	exportData->clear = ccbCtx->ctxAlgAls.results.irrClear;
	exportData->red = ccbCtx->ctxAlgAls.results.irrRed;
	exportData->green = ccbCtx->ctxAlgAls.results.irrGreen;
	exportData->blue = ccbCtx->ctxAlgAls.results.irrBlue;
	exportData->ir = ccbCtx->ctxAlgAls.results.irrWideband;
	exportData->time_us = ccbCtx->ctxAlgAls.time_us;
	exportData->gain = ccbCtx->ctxAlgAls.gain;
	exportData->rawClear = ccbCtx->ctxAlgAls.results.rawClear;
	exportData->rawRed = ccbCtx->ctxAlgAls.results.rawRed;
	exportData->rawGreen = ccbCtx->ctxAlgAls.results.rawGreen;
	exportData->rawBlue = ccbCtx->ctxAlgAls.results.rawBlue;
	exportData->rawWideband = ccbCtx->ctxAlgAls.results.rawWideband;
}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS_CCB
static bool _3407_alsSetThreshold(ams_deviceCtx_t *ctx, int32_t threshold)
{
	ams_ccb_als_config_t configData;

	configData.threshold = threshold;
	ccb_alsSetConfig(ctx, &configData);

	return false;
}
#endif

#ifdef CONFIG_AMS_OPTICAL_SENSOR_FLICKER
#ifndef CONFIG_AMS_OPTICAL_SENSOR_FIFO
static int _3407_flickerInit(ams_deviceCtx_t *ctx);
#endif
#endif

static int ams_deviceSetConfig(ams_deviceCtx_t *ctx, ams_configureFeature_t feature, deviceConfigOptions_t option, uint32_t data)
{
	int ret = 0;

	if (feature == AMS_CONFIG_ALS_LUX) {
		ALS_dbg("%s - ams_configureFeature_t AMS_CONFIG_ALS_LUX\n", __func__);
		switch (option)	{
		case AMS_CONFIG_ENABLE: /* ON / OFF */
			ALS_info("%s - deviceConfigOptions_t AMS_CONFIG_ENABLE(%u)\n", __func__, data);
			ALS_info("%s - current mode %d\n", __func__, ctx->mode);
			if (data == 0) {
				if (ctx->mode == MODE_ALS_LUX) {
					/* if no other active features, turn off device */
					ctx->shadowEnableReg = 0;
					ctx->shadowIntenabReg = 0;
					ctx->mode = MODE_OFF;
				} else {
					if ((ctx->mode & MODE_ALS_ALL) == MODE_ALS_LUX) {
						ctx->shadowEnableReg &= ~MASK_AEN;
						ctx->shadowIntenabReg &= ~MASK_ALS_INT_ALL;
					}
					ctx->mode &= ~(MODE_ALS_LUX);
				}
			} else {
				if ((ctx->mode & MODE_ALS_ALL) == 0) {
					ret = ccb_alsInit(ctx, &ctx->ccbAlsCtx.initData);
					if (ret < 0) {
						ALS_err("%s - failed to ccb_alsInit\n", __func__);
						return ret;
					}
					ctx->shadowEnableReg |= (AEN | PON);
					ctx->shadowIntenabReg |= AIEN;
				} else {
					/* force interrupt */
					ret = ams_setWord(ctx->portHndl, DEVREG_AIHTL, 0x00);
					if (ret < 0) {
						ALS_err("%s - failed to set DEVREG_AIHTL\n", __func__);
						return ret;
					}
				}
				ctx->mode |= MODE_ALS_LUX;
			}
			break;
		case AMS_CONFIG_THRESHOLD: /* set threshold */
			ALS_info("%s - deviceConfigOptions_t AMS_CONFIG_THRESHOLD\n", __func__);
			ALS_info("%s - data %d\n", __func__, data);
			_3407_alsSetThreshold(ctx, data);
			break;
		default:
			break;
		}
	}

	if (feature == AMS_CONFIG_FLICKER) {
		ALS_dbg("%s - ams_configureFeature_t AMS_CONFIG_FLICKER\n", __func__);
		switch (option)	{
		case AMS_CONFIG_ENABLE: /* power on */
			ALS_info("%s - deviceConfigOptions_t AMS_CONFIG_ENABLE(%u)\n", __func__, data);
			ALS_info("%s - current mode %d\n", __func__, ctx->mode);
			if (data == 0) {
				ret = ams_setField(ctx->portHndl, DEVREG_CFG9, LOW, MASK_SIEN_FD);
				if (ret < 0) {
					ALS_err("%s - failed to set DEVREG_CFG9\n", __func__);
					return ret;
				}
				if (ctx->mode == MODE_FLICKER) {
					/* if no other active features, turn off device */
					ctx->shadowEnableReg = 0;
					ctx->shadowIntenabReg = 0;
					ctx->mode = MODE_OFF;
					ams_setByte(ctx->portHndl, DEVREG_CONTROL, 0x02); //20180828 FIFO Buffer , FINT, FIFO_OV, FIFO_LVL all clear
				} else {
					ctx->mode &= ~MODE_FLICKER;
					ctx->shadowEnableReg &= ~(FDEN);

					if (!(ctx->mode & MODE_IRBEAM))
						ctx->shadowIntenabReg &= ~(SIEN);
				}
			} else {
#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
				ctx->shadowEnableReg |= (AEN | PON);
				ctx->shadowIntenabReg |= FIEN;
#else
				ctx->shadowEnableReg |= (FDEN | PON);
				ctx->shadowIntenabReg |= SIEN;
				ret = ams_setField(ctx->portHndl, DEVREG_CFG9, SIEN_FD, MASK_SIEN_FD);
				if (ret < 0) {
					ALS_err("%s - failed to set DEVREG_CFG9\n", __func__);
					return ret;
				}
#endif
				ctx->mode |= MODE_FLICKER;
#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
				ccb_alsInit_FIFO(ctx, &ctx->ccbAlsCtx.initData);
#else
				ret = _3407_flickerInit(ctx);
				if (ret < 0) {
					ALS_err("%s - failed to _3407_flickerInit\n", __func__);
					return ret;
				}
				//_3407_flickerEnableWAPre(ctx,ctx->shadowEnableReg,ctx->shadowIntenabReg);
#endif
			}
			break;
		case AMS_CONFIG_THRESHOLD: /* set threshold */
			ALS_info("%s - deviceConfigOptions_t AMS_CONFIG_THRESHOLD\n", __func__);
			/* TODO?:  set FD_COMPARE value? */
			break;
		default:
			break;
		}
	}
	ret = ams_setByte(ctx->portHndl, DEVREG_ENABLE, ctx->shadowEnableReg);
	if (ret < 0) {
		ALS_err("%s - failed to set DEVREG_ENABLE\n", __func__);
		return ret;
	}
	ret = ams_setByte(ctx->portHndl, DEVREG_INTENAB, ctx->shadowIntenabReg);
	if (ret < 0) {
		ALS_err("%s - failed to set DEVREG_INTENAB\n", __func__);
		return ret;
	}

	return 0;
}

#define STAR_ATIME  50 //50 msec
#define STAR_D_FACTOR  2266

static void als_update_statics(amsAlsContext_t *ctx)
{
	uint64_t tempCpl;
	uint64_t tempTime_us = ctx->time_us;
	uint64_t tempGain = ctx->gain;

	/* test for the potential of overflowing */
	uint32_t maxOverFlow;
#ifdef __KERNEL__
	u64 tmpTerm1;
	u64 tmpTerm2;
#endif
#ifdef __KERNEL__
	u64 tmp = ULLONG_MAX;

	do_div(tmp, ctx->time_us);

	maxOverFlow = (uint32_t)tmp;
#else
	maxOverFlow = (uint64_t)ULLONG_MAX / ctx->time_us;
#endif

	if (maxOverFlow < ctx->gain) {
		/* TODO: need to find use-case to test */
#ifdef __KERNEL__
		tmpTerm1 = tempTime_us;
		do_div(tmpTerm1, 2);
		tmpTerm2 = tempGain;
		do_div(tmpTerm2, 2);
		tempCpl = tmpTerm1 * tmpTerm2;
		do_div(tempCpl, (AMS_ALS_GAIN_FACTOR/4));
#else
		tempCpl = ((tempTime_us / 2) * (tempGain / 2)) / (AMS_ALS_GAIN_FACTOR/4);
#endif

	} else {
#ifdef __KERNEL__
		tempCpl = (tempTime_us * tempGain);
		do_div(tempCpl, AMS_ALS_GAIN_FACTOR);
#else
		tempCpl = (tempTime_us * tempGain) / AMS_ALS_GAIN_FACTOR;
#endif
	}
	if (tempCpl > (uint32_t)ULONG_MAX) {
		/* if we get here, we have a problem */
		//AMS_PORT_log_Msg_1(AMS_ERROR, "als_update_statics: overflow, setting cpl=%u\n", (uint32_t)ULONG_MAX);
		tempCpl = (uint32_t)ULONG_MAX;
	}

#ifdef __KERNEL__
	/*UVIR CPL refer as a const value with STAR Proejct */
	ctx->uvir_cpl = (tempTime_us * tempGain);
	do_div(ctx->uvir_cpl, AMS_ALS_GAIN_FACTOR);
	do_div(ctx->uvir_cpl, STAR_D_FACTOR);
#else
	/*UVIR CPL refer as a const value with STAR Proejct */
	ctx->uvir_cpl = (tempTime_us * tempGain) / AMS_ALS_GAIN_FACTOR;
	ctx->uvir_cpl = ctx->uvir_cpl / STAR_D_FACTOR;
#endif
	ctx->previousGain = ctx->gain;

//	AMS_PORT_log_Msg_4(AMS_DEBUG, "als_update_statics: time=%d, gain=%d, dFactor=%d => cpl=%u\n", ctx->time_us, ctx->gain, ctx->calibration.D_factor, ctx->cpl);
}

static int amsAlg_als_setConfig(amsAlsContext_t *ctx, amsAlsConf_t *inputData)
{
	int ret = 0;

	if (inputData != NULL) {
		ctx->gain = inputData->gain;
		ctx->time_us = inputData->time_us;
	}
	//als_update_statics(ctx);

	return ret;
}

/*
 * getConfig: is used to quarry the algorithm's configuration
 */
static int amsAlg_als_getConfig(amsAlsContext_t *ctx, amsAlsConf_t *outputData)
{
	int ret = 0;

	outputData->gain = ctx->gain;
	outputData->time_us = ctx->time_us;

	return ret;
}

static int amsAlg_als_getResult(amsAlsContext_t *ctx, amsAlsResult_t *outData)
{
	int ret = 0;

	outData->rawClear  = ctx->results.rawClear;
	outData->rawRed  = ctx->results.rawRed;
	outData->rawGreen  = ctx->results.rawGreen;
	outData->rawBlue  = ctx->results.rawBlue;
	outData->irrBlue  = ctx->results.irrBlue;
	outData->irrClear = ctx->results.irrClear;
	outData->irrGreen = ctx->results.irrGreen;
	outData->irrRed   = ctx->results.irrRed;
	outData->irrWideband = ctx->results.irrWideband;
	outData->mLux_ave  = ctx->results.mLux_ave / AMS_LUX_AVERAGE_COUNT;
	outData->IR  = ctx->results.IR;
	outData->CCT = ctx->results.CCT;
	outData->adaptive = ctx->results.adaptive;

	if (ctx->notStableMeasurement)
		ctx->notStableMeasurement = false;

	outData->mLux = ctx->results.mLux;

	return ret;
}

static int amsAlg_als_initAlg(amsAlsContext_t *ctx, amsAlsInitData_t *initData)
{
	int ret = 0;

	memset(ctx, 0, sizeof(amsAlsContext_t));

	if (initData != NULL) {
		ctx->calibration.Time_base = initData->calibration.Time_base;
		ctx->calibration.thresholdLow = initData->calibration.thresholdLow;
		ctx->calibration.thresholdHigh = initData->calibration.thresholdHigh;
		ctx->calibration.calibrationFactor = initData->calibration.calibrationFactor;
	}

	if (initData != NULL) {
		ctx->gain = initData->gain;
		ctx->time_us = initData->time_us;
		ctx->adaptive = initData->adaptive;
	} else {
		ALS_dbg("error: initData == NULL\n");
	}

	als_update_statics(ctx);
	return ret;
}

static int amsAlg_als_getAlgInfo(amsAlsAlgoInfo_t *info)
{
	int ret = 0;

	info->algName = "AMS_ALS";
	info->contextMemSize = sizeof(amsAlsContext_t);
	info->scratchMemSize = 0;

	info->initAlg = &amsAlg_als_initAlg;
	info->processData = &amsAlg_als_processData;
	info->getResult = &amsAlg_als_getResult;
	info->setConfig = &amsAlg_als_setConfig;
	info->getConfig = &amsAlg_als_getConfig;

	return ret;
}

static int tcs3407_print_reg_status(void)
{
	int reg, err;
	u8 recvData;

	for (reg = 0; reg < DEVREG_REG_MAX; reg++) {
		err = tcs3407_read_reg(tcs3407_data, deviceRegisterDefinition[reg].address, &recvData, 1);
		if (err != 0) {
			ALS_err("%s - error reading 0x%02x err:%d\n",
				__func__, reg, err);
		} else {
			ALS_dbg("%s - 0x%02x = 0x%02x\n",
				__func__, deviceRegisterDefinition[reg].address, recvData);
		}
	}
	return 0;
}

static int tcs3407_set_sampling_rate(u32 sampling_period_ns)
{
	ALS_dbg("%s - sensor_info_data not support\n", __func__);

	return 0;
}

static void tcs3407_irq_set_state(struct tcs3407_device_data *data, int irq_enable)
{
	ALS_dbg("%s - irq_enable : %d, irq_state : %d\n",
		__func__, irq_enable, data->irq_state);

	if (irq_enable) {
		if (data->irq_state++ == 0)
			enable_irq(data->dev_irq);
	} else {
		if (data->irq_state == 0)
			return;
		if (--data->irq_state <= 0) {
			disable_irq(data->dev_irq);
			data->irq_state = 0;
		}
	}
}

static int tcs3407_power_ctrl(struct tcs3407_device_data *data, int onoff)
{
	int rc = 0;
	static int i2c_1p8_enable;

	struct regulator *regulator_vdd_1p8 = NULL;
	struct regulator *regulator_i2c_1p8 = NULL;

	ALS_dbg("%s - onoff : %d, state : %d\n",
		__func__, onoff, data->regulator_state);

	if (onoff == PWR_ON) {
		if (data->regulator_state != 0) {
			ALS_dbg("%s - duplicate regulator\n", __func__);
			data->regulator_state++;
			return 0;
		}
		data->regulator_state++;
		data->pm_state = PM_RESUME;
	} else {
		if (data->regulator_state == 0) {
			ALS_dbg("%s - already off the regulator\n", __func__);
			return 0;
		} else if (data->regulator_state != 1) {
			ALS_dbg("%s - duplicate regulator\n", __func__);
			data->regulator_state--;
			return 0;
		}
		data->regulator_state--;
	}

	if (data->i2c_1p8 != NULL) {
		regulator_i2c_1p8 = regulator_get(NULL, data->i2c_1p8);
		if (IS_ERR(regulator_i2c_1p8) || regulator_i2c_1p8 == NULL) {
			ALS_err("%s - get i2c_1p8 regulator failed\n", __func__);
			rc = PTR_ERR(regulator_i2c_1p8);
			regulator_i2c_1p8 = NULL;
			goto get_i2c_1p8_failed;
		}
	}

	regulator_vdd_1p8 =
		regulator_get(&data->client->dev, data->vdd_1p8);
	if (IS_ERR(regulator_vdd_1p8) || regulator_vdd_1p8 == NULL) {
		ALS_dbg("%s - get vdd_1p8 regulator failed\n", __func__);
		rc = PTR_ERR(regulator_vdd_1p8);
		regulator_vdd_1p8 = NULL;
	}

	if (onoff == PWR_ON) {
		if (data->i2c_1p8 != NULL && i2c_1p8_enable == 0) {
			rc = regulator_enable(regulator_i2c_1p8);
			i2c_1p8_enable = 1;
			if (rc) {
				ALS_err("%s - enable i2c_1p8 failed, rc=%d\n", __func__, rc);
				goto enable_i2c_1p8_failed;
			}
		}
		if (regulator_vdd_1p8 != NULL) {
			rc = regulator_enable(regulator_vdd_1p8);
			if (rc) {
				ALS_err("%s - enable vdd_1p8 failed, rc=%d\n",
					__func__, rc);
				goto enable_vdd_1p8_failed;
			}
		}
		if (data->pin_als_en >= 0) {
			rc = gpio_direction_output(data->pin_als_en, 1);
			if (rc) {
				ALS_err("%s - gpio direction output failed, rc=%d\n",
					__func__, rc);
				goto gpio_direction_output_failed;
			}
		}

		usleep_range(1000, 1100);
	} else {
		if (regulator_vdd_1p8 != NULL) {
			rc = regulator_disable(regulator_vdd_1p8);
			if (rc) {
				ALS_err("%s - disable vdd_1p8 failed, rc=%d\n",
					__func__, rc);
				goto done;
			}
		}

		if (data->pin_als_en >= 0) {
			gpio_set_value(data->pin_als_en, 0);
			rc = gpio_direction_input(data->pin_als_en);
			if (rc) {
				ALS_err("%s - gpio direction input failed, rc=%d\n",
					__func__, rc);
			}
		}

#ifdef I2C_1P8_DISABLE
		if (data->i2c_1p8 != NULL) {
			rc = regulator_disable(regulator_i2c_1p8);
			i2c_1p8_enable = 0;
			if (rc) {
				ALS_err("%s - disable i2c_1p8 failed, rc=%d\n", __func__, rc);
				goto done;
			}
		}
#endif
	}

	goto done;

gpio_direction_output_failed:
	if (regulator_vdd_1p8 != NULL)
		regulator_disable(regulator_vdd_1p8);
enable_vdd_1p8_failed:
#ifdef I2C_1P8_DISABLE
	if (data->i2c_1p8 != NULL) {
		regulator_disable(regulator_i2c_1p8);
		i2c_1p8_enable = 0;
	}
#endif
enable_i2c_1p8_failed:
done:
	regulator_put(regulator_vdd_1p8);
	if (data->i2c_1p8 != NULL)
		regulator_put(regulator_i2c_1p8);
get_i2c_1p8_failed:
	return rc;
}

static bool ams_deviceGetAls(ams_deviceCtx_t *ctx, ams_apiAls_t *exportData);
static bool ams_deviceGetFlicker(ams_deviceCtx_t *ctx, ams_apiAlsFlicker_t *exportData);

static void report_als(struct tcs3407_device_data *chip)
{
	ams_apiAls_t outData;
	static unsigned int als_cnt;

	if (chip->als_input_dev) {
		ams_deviceGetAls(chip->deviceCtx, &outData);
		input_report_rel(chip->als_input_dev, REL_X, outData.ir + 1);
		input_report_rel(chip->als_input_dev, REL_Y, outData.red + 1);
		input_report_rel(chip->als_input_dev, REL_Z, outData.green + 1);
		input_report_rel(chip->als_input_dev, REL_RX, outData.blue + 1);
		input_report_rel(chip->als_input_dev, REL_RY, outData.clear + 1);
		input_report_abs(chip->als_input_dev, ABS_X, outData.time_us + 1);
		input_report_abs(chip->als_input_dev, ABS_Y, outData.gain + 1);
		input_sync(chip->als_input_dev);

		if (als_cnt++ > 10) {
			ALS_dbg("%s - I:%d, R:%d, G:%d, B:%d, C:%d TIME:%d, GAIN:%d\n", __func__,
				outData.ir, outData.red, outData.green, outData.blue, outData.clear, outData.time_us, outData.gain);
			als_cnt = 0;
		} else {
			ALS_info("%s - I:%d, R:%d, G:%d, B:%d, C:%d TIME:%d, GAIN:%d\n", __func__,
				outData.ir, outData.red, outData.green, outData.blue, outData.clear, outData.time_us, outData.gain);
		}

		chip->user_ir_data = outData.ir;
#ifdef CONFIG_AMS_OPTICAL_SENSOR_EOL_MODE
		if (chip->eol_enable && chip->eol_count >= EOL_SKIP_COUNT) {
			chip->eol_awb += outData.ir;
			chip->eol_clear += outData.clear;
		}
#endif
	}
}

static void report_flicker(struct tcs3407_device_data *chip)
{
	ams_apiAlsFlicker_t outData;
	uint flicker = 0;
	static unsigned int flicker_cnt;

	if (chip->als_input_dev) {
		ams_deviceGetFlicker
			(chip->deviceCtx, &outData);

		if (outData.mHz == 100000 || outData.mHz == 120000)
			flicker = outData.mHz / 1000;

		input_report_rel(chip->als_input_dev, REL_RZ, flicker + 1);
		input_sync(chip->als_input_dev);

		if (flicker_cnt++ > 10) {
			ALS_dbg("%s - flicker = %d\n", __func__, flicker);
			flicker_cnt = 0;
		} else {
			ALS_info("%s - flicker = %d, %d\n", __func__, flicker, outData.mHz);
		}

		chip->user_flicker_data = outData.mHz;
#ifdef CONFIG_AMS_OPTICAL_SENSOR_EOL_MODE
		if (chip->eol_enable && flicker != 0 && chip->eol_count >= EOL_SKIP_COUNT) {
			chip->eol_flicker += flicker;
			chip->eol_flicker_count++;
		}
#endif

	}
}

static ssize_t als_ir_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ams_apiAls_t outData;
	struct tcs3407_device_data *chip = dev_get_drvdata(dev);

	ams_deviceGetAls(chip->deviceCtx, &outData);

	return snprintf(buf, PAGE_SIZE, "%d\n", outData.ir);
}

static ssize_t als_red_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ams_apiAls_t outData;
	struct tcs3407_device_data *chip = dev_get_drvdata(dev);

	ams_deviceGetAls(chip->deviceCtx, &outData);

	return snprintf(buf, PAGE_SIZE, "%d\n", outData.red);
}

static ssize_t als_green_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ams_apiAls_t outData;
	struct tcs3407_device_data *chip = dev_get_drvdata(dev);

	ams_deviceGetAls(chip->deviceCtx, &outData);

	return snprintf(buf, PAGE_SIZE, "%d\n", outData.green);
}

static ssize_t als_blue_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ams_apiAls_t outData;
	struct tcs3407_device_data *chip = dev_get_drvdata(dev);

	ams_deviceGetAls(chip->deviceCtx, &outData);

	return snprintf(buf, PAGE_SIZE, "%d\n", outData.blue);
}

static ssize_t als_clear_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ams_apiAls_t outData;
	struct tcs3407_device_data *chip = dev_get_drvdata(dev);

	ams_deviceGetAls(chip->deviceCtx, &outData);

	return snprintf(buf, PAGE_SIZE, "%d\n", outData.clear);
}

static ssize_t als_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ams_apiAls_t outData;
	struct tcs3407_device_data *chip = dev_get_drvdata(dev);

	ams_deviceGetAls(chip->deviceCtx, &outData);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d\n", outData.rawWideband,
				outData.rawRed, outData.rawGreen, outData.rawBlue, outData.rawClear);
}

static size_t als_enable_set(struct tcs3407_device_data *chip, uint8_t valueToSet)
{
	int rc = 0;

#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS_CCB
	rc = ams_deviceSetConfig(chip->deviceCtx, AMS_CONFIG_ALS_LUX, AMS_CONFIG_ENABLE, valueToSet);
	if (rc < 0) {
		ALS_err("%s - ams_deviceSetConfig ALS_LUX fail, rc=%d\n", __func__, rc);
		return rc;
	}
#endif
#ifdef CONFIG_AMS_OPTICAL_SENSOR_FLICKER

	rc = ams_deviceSetConfig(chip->deviceCtx, AMS_CONFIG_FLICKER, AMS_CONFIG_ENABLE, valueToSet);
	if (rc < 0) {
		ALS_err("%s - ams_deviceSetConfig FLICKER fail, rc=%d\n", __func__, rc);
		return rc;
	}

#endif
	return 0;
}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_FLICKER
static ssize_t flicker_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ams_apiAlsFlicker_t outData;
	struct tcs3407_device_data *chip = dev_get_drvdata(dev);

	ams_deviceGetFlicker(chip->deviceCtx, &outData);

	return snprintf(buf, PAGE_SIZE, "%d\n", outData.mHz);
}
#endif

/* als input enable/disable sysfs */
static ssize_t tcs3407_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);
	ams_mode_t mode;

	ams_getMode(data->deviceCtx, &mode);

	if (mode & MODE_ALS_ALL)
		return snprintf(buf, strlen(buf), "%d\n", 1);
	else
		return snprintf(buf, strlen(buf), "%d\n", 0);
}

static int ams_deviceInit(ams_deviceCtx_t *ctx, AMS_PORT_portHndl *portHndl, ams_calibrationData_t *calibrationData);

static ssize_t tcs3407_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);
	bool value;
	int err = 0;

	if (strtobool(buf, &value))
		return -EINVAL;

	ALS_dbg("%s - en : %d, c : %d\n", __func__, value, data->enabled);

	mutex_lock(&data->activelock);

	if (value) {
		tcs3407_irq_set_state(data, PWR_ON);

		err = tcs3407_power_ctrl(data, PWR_ON);
		if (err < 0)
			ALS_err("%s - als_regulator_on fail err = %d\n",
				__func__, err);

		err = ams_deviceInit(data->deviceCtx, data->client, NULL);
		if (err < 0) {
			ALS_err("%s - ams_deviceInit failed.\n", __func__);
			goto err_device_init;
		} else {
			ALS_dbg("%s - ams_amsDeviceInit ok\n", __func__);
		}

		err = als_enable_set(data, AMSDRIVER_ALS_ENABLE);

		if (err == 0)
			data->enabled = 1;

		if (err < 0) {
			input_report_rel(data->als_input_dev,
				REL_RZ, -5 + 1); /* F_ERR_I2C -5 detected i2c error */
			input_sync(data->als_input_dev);
			ALS_err("%s - enable error %d\n", __func__, err);
		}

		data->mode_cnt.amb_cnt++;
	} else {
		if (data->regulator_state == 0) {
			ALS_dbg("%s - already power off - disable skip\n",
				__func__);
			goto err_already_off;
		}

		err = als_enable_set(data, AMSDRIVER_ALS_DISABLE);
		if (err != 0)
			ALS_err("%s - disable err : %d\n", __func__, err);

		data->enabled = 0;

		err = tcs3407_power_ctrl(data, PWR_OFF);
		if (err < 0)
			ALS_err("%s - als_regulator_off fail err = %d\n",
				__func__, err);
		tcs3407_irq_set_state(data, PWR_OFF);
	}

err_device_init:
err_already_off:
	mutex_unlock(&data->activelock);

	return count;
}

static ssize_t tcs3407_poll_delay_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->sampling_period_ns);
}

static ssize_t tcs3407_poll_delay_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);
	u32 sampling_period_ns = 0;
	int err = 0;

	mutex_lock(&data->activelock);

	err = kstrtoint(buf, 10, &sampling_period_ns);

	if (err < 0) {
		ALS_err("%s - kstrtoint failed.(%d)\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}

	err = tcs3407_set_sampling_rate(sampling_period_ns);

	if (err > 0)
		data->sampling_period_ns = err;

	ALS_dbg("%s - als sensor sampling rate is setted as %dns\n", __func__, sampling_period_ns);

	mutex_unlock(&data->activelock);

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
	tcs3407_enable_show, tcs3407_enable_store);
static DEVICE_ATTR(poll_delay, S_IRUGO|S_IWUSR|S_IWGRP,
	tcs3407_poll_delay_show, tcs3407_poll_delay_store);

static struct attribute *als_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group als_attribute_group = {
	.attrs = als_sysfs_attrs,
};

/* als_sensor sysfs */
static ssize_t tcs3407_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char chip_name[NAME_LEN];

	strlcpy(chip_name, TCS3407_CHIP_NAME, strlen(TCS3407_CHIP_NAME) + 1);

	return snprintf(buf, PAGE_SIZE, "%s\n", chip_name);
}

static ssize_t tcs3407_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char vendor[NAME_LEN];

	strlcpy(vendor, VENDOR, strlen(VENDOR) + 1);

	return snprintf(buf, PAGE_SIZE, "%s\n", vendor);
}

static ssize_t tcs3407_flush_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);
	int ret = 0;
	u8 handle = 0;

	mutex_lock(&data->activelock);
	ret = kstrtou8(buf, 10, &handle);
	if (ret < 0) {
		ALS_err("%s - kstrtou8 failed.(%d)\n", __func__, ret);
		mutex_unlock(&data->activelock);
		return ret;
	}
	ALS_dbg("%s - handle = %d\n", __func__, handle);
	mutex_unlock(&data->activelock);

	input_report_rel(data->als_input_dev, REL_MISC, handle);

	return size;
}

static ssize_t tcs3407_int_pin_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	/* need to check if this should be implemented */
	ALS_dbg("%s - not implement\n", __func__);
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t tcs3407_read_reg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);

	ALS_info("%s - val=0x%06x\n", __func__, data->reg_read_buf);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->reg_read_buf);
}

static ssize_t tcs3407_read_reg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);

	int err = -1;
	unsigned int cmd = 0;
	u8 val = 0;

	mutex_lock(&data->i2clock);
	if (data->regulator_state == 0) {
		ALS_dbg("%s - need to power on\n", __func__);
		mutex_unlock(&data->i2clock);
		return size;
	}
	err = sscanf(buf, "%8x", &cmd);
	if (err == 0) {
		ALS_err("%s - sscanf fail\n", __func__);
		mutex_unlock(&data->i2clock);
		return size;
	}

	err = tcs3407_read_reg(data, (u8)cmd, &val, 1);
	if (err != 0) {
		ALS_err("%s - err=%d, val=0x%06x\n",
			__func__, err, val);
		mutex_unlock(&data->i2clock);
		return size;
	}
	data->reg_read_buf = (u32)val;
	mutex_unlock(&data->i2clock);

	return size;
}
static ssize_t tcs3407_write_reg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);

	int err = -1;
	unsigned int cmd = 0;
	unsigned int val = 0;

	mutex_lock(&data->i2clock);
	if (data->regulator_state == 0) {
		ALS_dbg("%s - need to power on.\n", __func__);
		mutex_unlock(&data->i2clock);
		return size;
	}
	err = sscanf(buf, "%8x, %8x", &cmd, &val);
	if (err == 0) {
		ALS_err("%s - sscanf fail %s\n", __func__, buf);
		mutex_unlock(&data->i2clock);
		return size;
	}

	err = tcs3407_write_reg(data, (u8)cmd, (u8)val);
	if (err < 0) {
		ALS_err("%s - fail err = %d\n", __func__, err);
		mutex_unlock(&data->i2clock);
		return err;
	}
	mutex_unlock(&data->i2clock);

	return size;
}

static ssize_t tcs3407_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);

	ALS_info("%s - debug mode = %u\n", __func__, data->debug_mode);

	return snprintf(buf, PAGE_SIZE, "%u\n", data->debug_mode);
}

static ssize_t tcs3407_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);
	int err;
	s32 mode;

	mutex_lock(&data->activelock);
	err = kstrtoint(buf, 10, &mode);
	if (err < 0) {
		ALS_err("%s - kstrtoint failed.(%d)\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	data->debug_mode = (u8)mode;
	ALS_info("%s - mode = %d\n", __func__, mode);

	switch (data->debug_mode) {
	case DEBUG_REG_STATUS:
		tcs3407_print_reg_status();
		break;
	case DEBUG_VAR:
		tcs3407_debug_var(data);
		break;
	default:
		break;
	}
	mutex_unlock(&data->activelock);

	return size;
}

static ssize_t tcs3407_device_id_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	ALS_dbg("%s - device_id not support\n", __func__);

	return snprintf(buf, PAGE_SIZE, "NOT SUPPORT\n");
}

static ssize_t tcs3407_part_type_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);
	ams_deviceCtx_t *ctx = data->deviceCtx;

	return snprintf(buf, PAGE_SIZE, "%d\n", ctx->deviceId);
}

static ssize_t tcs3407_i2c_err_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);
	u32 err_cnt = 0;

	err_cnt = data->i2c_err_cnt;

	return snprintf(buf, PAGE_SIZE, "%d\n", err_cnt);
}

static ssize_t tcs3407_i2c_err_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);

	data->i2c_err_cnt = 0;

	return size;
}

static ssize_t tcs3407_curr_adc_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE,
		"\"HRIC\":\"%d\",\"HRRC\":\"%d\",\"HRIA\":\"%d\",\"HRRA\":\"%d\"\n",
		0, 0, data->user_ir_data, data->user_flicker_data);
}

static ssize_t tcs3407_curr_adc_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);

	data->user_ir_data = 0;
	data->user_flicker_data = 0;

	return size;
}

static ssize_t tcs3407_mode_cnt_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE,
		"\"CNT_HRM\":\"%d\",\"CNT_AMB\":\"%d\",\"CNT_PROX\":\"%d\",\"CNT_SDK\":\"%d\",\"CNT_CGM\":\"%d\",\"CNT_UNKN\":\"%d\"\n",
		data->mode_cnt.hrm_cnt, data->mode_cnt.amb_cnt, data->mode_cnt.prox_cnt,
		data->mode_cnt.sdk_cnt, data->mode_cnt.cgm_cnt, data->mode_cnt.unkn_cnt);
}

static ssize_t tcs3407_mode_cnt_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);

	data->mode_cnt.hrm_cnt = 0;
	data->mode_cnt.amb_cnt = 0;
	data->mode_cnt.prox_cnt = 0;
	data->mode_cnt.sdk_cnt = 0;
	data->mode_cnt.cgm_cnt = 0;
	data->mode_cnt.unkn_cnt = 0;

	return size;
}

static ssize_t tcs3407_factory_cmd_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);
	static int cmd_result;

	mutex_lock(&data->activelock);

	if (data->isTrimmed)
		cmd_result = 1;
	else
		cmd_result = 0;

	ALS_dbg("%s - cmd_result = %d\n", __func__, cmd_result);

	mutex_unlock(&data->activelock);

	return snprintf(buf, PAGE_SIZE, "%d\n", cmd_result);
}

static ssize_t tcs3407_version_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	ALS_info("%s - cmd_result = %s.%s.%s%s\n", __func__,
		VERSION, SUB_VERSION, HEADER_VERSION, VENDOR_VERSION);

	return snprintf(buf, PAGE_SIZE, "%s.%s.%s%s\n",
		VERSION, SUB_VERSION, HEADER_VERSION, VENDOR_VERSION);
}

static ssize_t tcs3407_sensor_info_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	ALS_dbg("%s - sensor_info_data not support\n", __func__);

	return snprintf(buf, PAGE_SIZE, "NOT SUPPORT\n");
}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_EOL_MODE

static int tcs3407_eol_mode(struct tcs3407_device_data *data)
{
	int led_curr = 0;
	int pulse_duty = 0;
	int curr_state = EOL_STATE_INIT;
	int ret = 0;

	data->eol_state = EOL_STATE_INIT;
	data->eol_enable = 1;
	data->eol_result_status = 1;

	ret = gpio_request(data->pin_led_en, NULL);
	if (ret < 0)
		return ret;
		
	s2mpb02_led_en(S2MPB02_TORCH_LED_1, led_curr, S2MPB02_LED_TURN_WAY_GPIO);

	while (data->eol_state < EOL_STATE_DONE) {
		switch (data->eol_state) {
		case EOL_STATE_INIT:
			break;
		case EOL_STATE_100:
			led_curr = S2MPB02_TORCH_OUT_I_100MA;
			pulse_duty = data->eol_pulse_duty[0];
			break;
		case EOL_STATE_120:
			led_curr = S2MPB02_TORCH_OUT_I_100MA;
			pulse_duty = data->eol_pulse_duty[1];
			break;
		default:
			break;
		}

		if (data->eol_state >= EOL_STATE_100) {
			if (curr_state != data->eol_state) {
				s2mpb02_led_en(S2MPB02_TORCH_LED_1, led_curr, S2MPB02_LED_TURN_WAY_GPIO);
				curr_state = data->eol_state;
			} else
				gpio_direction_output(data->pin_led_en, 1);

			udelay(pulse_duty);

			gpio_direction_output(data->pin_led_en, 0);

			data->eol_pulse_count++;
		}
		udelay(pulse_duty);
	}
	s2mpb02_led_en(S2MPB02_TORCH_LED_1, 0, S2MPB02_LED_TURN_WAY_GPIO);
	gpio_free(data->pin_led_en);

	data->eol_enable = 0;

	if (data->eol_state >= EOL_STATE_DONE) {
		snprintf(data->eol_result, MAX_TEST_RESULT, "%d, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d, %s\n",
			data->eol_flicker_awb[EOL_STATE_100][0], FREQ100_SPEC_IN(data->eol_flicker_awb[EOL_STATE_100][0]),
			data->eol_flicker_awb[EOL_STATE_100][1], IR_SPEC_IN(data->eol_flicker_awb[EOL_STATE_100][1]),
			data->eol_flicker_awb[EOL_STATE_100][2], CLEAR_SPEC_IN(data->eol_flicker_awb[EOL_STATE_100][2]),
			data->eol_flicker_awb[EOL_STATE_120][0], FREQ120_SPEC_IN(data->eol_flicker_awb[EOL_STATE_120][0]),
			data->eol_flicker_awb[EOL_STATE_120][1], IR_SPEC_IN(data->eol_flicker_awb[EOL_STATE_120][1]),
			data->eol_flicker_awb[EOL_STATE_120][2], CLEAR_SPEC_IN(data->eol_flicker_awb[EOL_STATE_120][2]));
	} else
		ALS_err("%s - abnormal termination\n", __func__);

	ALS_dbg("%s - %d, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d, %s\n",
		__func__,
		data->eol_flicker_awb[EOL_STATE_100][0], FREQ100_SPEC_IN(data->eol_flicker_awb[EOL_STATE_100][0]),
		data->eol_flicker_awb[EOL_STATE_100][1], IR_SPEC_IN(data->eol_flicker_awb[EOL_STATE_100][1]),
		data->eol_flicker_awb[EOL_STATE_100][2], CLEAR_SPEC_IN(data->eol_flicker_awb[EOL_STATE_100][2]),
		data->eol_flicker_awb[EOL_STATE_120][0], FREQ120_SPEC_IN(data->eol_flicker_awb[EOL_STATE_120][0]),
		data->eol_flicker_awb[EOL_STATE_120][1], IR_SPEC_IN(data->eol_flicker_awb[EOL_STATE_120][1]),
		data->eol_flicker_awb[EOL_STATE_120][2], CLEAR_SPEC_IN(data->eol_flicker_awb[EOL_STATE_120][2]));

	return 0;
}

static ssize_t tcs3407_eol_mode_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->activelock);

	if (data->eol_result == NULL) {
		ALS_err("%s - data->eol_result is NULL\n", __func__);
		mutex_unlock(&data->activelock);
		return snprintf(buf, PAGE_SIZE, "%s\n", "NO_EOL_TEST");
	}
	if (data->eol_enable == 1) {
		mutex_unlock(&data->activelock);
		return snprintf(buf, PAGE_SIZE, "%s\n", "EOL_RUNNING");
	} else if (data->eol_enable == 0 && data->eol_result_status == 0) {
		mutex_unlock(&data->activelock);
		return snprintf(buf, PAGE_SIZE, "%s\n", "NO_EOL_TEST");
	}
	mutex_unlock(&data->activelock);

	data->eol_result_status = 0;
	return snprintf(buf, PAGE_SIZE, "%s\n", data->eol_result);
}

static ssize_t tcs3407_eol_mode_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);
	int err = 0;
	int mode = 0;

	err = kstrtoint(buf, 10, &mode);
	if (err < 0) {
		ALS_err("%s - kstrtoint failed.(%d)\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}

	switch (mode) {
	case 1:
		gSpec_ir_min = data->eol_ir_spec[0];
		gSpec_ir_max = data->eol_ir_spec[1];
		gSpec_clear_min = data->eol_clear_spec[0];
		gSpec_clear_max = data->eol_clear_spec[1];
		break;

	case 2:
		gSpec_ir_min = data->eol_ir_spec[2];
		gSpec_ir_max = data->eol_ir_spec[3];
		gSpec_clear_min = data->eol_clear_spec[2];
		gSpec_clear_max = data->eol_clear_spec[3];
		break;

	default:
		break;
	}

	ALS_dbg("%s - mode = %d, gSpec_ir_min = %d, gSpec_ir_max = %d, gSpec_clear_min = %d, gSpec_clear_max = %d\n",
		__func__, mode, gSpec_ir_min, gSpec_ir_max, gSpec_clear_min, gSpec_clear_max);

	mutex_lock(&data->activelock);
	tcs3407_irq_set_state(data, PWR_ON);

	err = tcs3407_power_ctrl(data, PWR_ON);
	if (err < 0)
		ALS_err("%s - als_regulator_on fail err = %d\n",
			__func__, err);

	err = ams_deviceInit(data->deviceCtx, data->client, NULL);
	if (err < 0) {
		ALS_err("%s - ams_deviceInit failed.\n", __func__);
		goto err_device_init;
	} else {
		ALS_dbg("%s - ams_amsDeviceInit ok\n", __func__);
	}

	err = als_enable_set(data, AMSDRIVER_ALS_ENABLE);
	if (err == 0) {
		data->enabled = 1;
	} else if (err < 0) {
		input_report_rel(data->als_input_dev,
			REL_Y, -5 + 1); /* F_ERR_I2C -5 detected i2c error */
		input_sync(data->als_input_dev);
		ALS_err("%s - enable error %d\n", __func__, err);
	}
	mutex_unlock(&data->activelock);
	tcs3407_eol_mode(data);
	mutex_lock(&data->activelock);

	if (data->regulator_state == 0) {
		ALS_dbg("%s - already power off - disable skip\n",
			__func__);
		goto err_already_off;
	}

	err = als_enable_set(data, AMSDRIVER_ALS_DISABLE);
	if (err != 0)
		ALS_err("%s - disable err : %d\n", __func__, err);

	data->enabled = 0;

	err = tcs3407_power_ctrl(data, PWR_OFF);
	if (err < 0)
		ALS_err("%s - als_regulator_off fail err = %d\n",
			__func__, err);
	tcs3407_irq_set_state(data, PWR_OFF);

err_device_init:
err_already_off:
	mutex_unlock(&data->activelock);

	return size;
}

static ssize_t tcs3407_eol_spec_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);

	ALS_dbg("%s - ir_spec = %d, %d, %d, %d, %d, %d, %d, %d\n", __func__,
		data->eol_ir_spec[0], data->eol_ir_spec[1], data->eol_clear_spec[0], data->eol_clear_spec[1],
		data->eol_ir_spec[2], data->eol_ir_spec[3], data->eol_clear_spec[2], data->eol_clear_spec[3]);

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d, %d, %d, %d, %d, %d\n",
		data->eol_ir_spec[0], data->eol_ir_spec[1], data->eol_clear_spec[0], data->eol_clear_spec[1],
		data->eol_ir_spec[2], data->eol_ir_spec[3], data->eol_clear_spec[2], data->eol_clear_spec[3]);
}
#endif

static DEVICE_ATTR(name, S_IRUGO, tcs3407_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, tcs3407_vendor_show, NULL);
static DEVICE_ATTR(als_flush, S_IWUSR | S_IWGRP, NULL, tcs3407_flush_store);
static DEVICE_ATTR(int_pin_check, S_IRUGO, tcs3407_int_pin_check_show, NULL);
static DEVICE_ATTR(read_reg, S_IRUGO | S_IWUSR | S_IWGRP,
	tcs3407_read_reg_show, tcs3407_read_reg_store);
static DEVICE_ATTR(write_reg, S_IWUSR | S_IWGRP, NULL, tcs3407_write_reg_store);
static DEVICE_ATTR(als_debug, S_IRUGO | S_IWUSR | S_IWGRP,
	tcs3407_debug_show, tcs3407_debug_store);
static DEVICE_ATTR(device_id, S_IRUGO, tcs3407_device_id_show, NULL);
static DEVICE_ATTR(part_type, S_IRUGO, tcs3407_part_type_show, NULL);
static DEVICE_ATTR(i2c_err_cnt, S_IRUGO | S_IWUSR | S_IWGRP, tcs3407_i2c_err_show, tcs3407_i2c_err_store);
static DEVICE_ATTR(curr_adc, S_IRUGO | S_IWUSR | S_IWGRP, tcs3407_curr_adc_show, tcs3407_curr_adc_store);
static DEVICE_ATTR(mode_cnt, S_IRUGO | S_IWUSR | S_IWGRP, tcs3407_mode_cnt_show, tcs3407_mode_cnt_store);
static DEVICE_ATTR(als_factory_cmd, S_IRUGO, tcs3407_factory_cmd_show, NULL);
static DEVICE_ATTR(als_version, S_IRUGO, tcs3407_version_show, NULL);
static DEVICE_ATTR(sensor_info, S_IRUGO, tcs3407_sensor_info_show, NULL);
static DEVICE_ATTR(als_ir, S_IRUGO, als_ir_show, NULL);
static DEVICE_ATTR(als_red, S_IRUGO, als_red_show, NULL);
static DEVICE_ATTR(als_green, S_IRUGO, als_green_show, NULL);
static DEVICE_ATTR(als_blue, S_IRUGO, als_blue_show, NULL);
static DEVICE_ATTR(als_clear, S_IRUGO, als_clear_show, NULL);
static DEVICE_ATTR(als_raw_data, S_IRUGO, als_raw_data_show, NULL);
#ifdef CONFIG_AMS_OPTICAL_SENSOR_FLICKER
static DEVICE_ATTR(flicker_data, S_IRUGO, flicker_data_show, NULL);
#endif
#ifdef CONFIG_AMS_OPTICAL_SENSOR_EOL_MODE
static DEVICE_ATTR(eol_mode, S_IRUGO | S_IWUSR | S_IWGRP, tcs3407_eol_mode_show, tcs3407_eol_mode_store);
static DEVICE_ATTR(eol_spec, S_IRUGO, tcs3407_eol_spec_show, NULL);
#endif

static struct device_attribute *tcs3407_sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_als_flush,
	&dev_attr_int_pin_check,
	&dev_attr_read_reg,
	&dev_attr_write_reg,
	&dev_attr_als_debug,
	&dev_attr_device_id,
	&dev_attr_part_type,
	&dev_attr_i2c_err_cnt,
	&dev_attr_curr_adc,
	&dev_attr_mode_cnt,
	&dev_attr_als_factory_cmd,
	&dev_attr_als_version,
	&dev_attr_sensor_info,
	&dev_attr_als_ir,
	&dev_attr_als_red,
	&dev_attr_als_green,
	&dev_attr_als_blue,
	&dev_attr_als_clear,
	&dev_attr_als_raw_data,
#ifdef CONFIG_AMS_OPTICAL_SENSOR_FLICKER
	&dev_attr_flicker_data,
#endif
#ifdef CONFIG_AMS_OPTICAL_SENSOR_EOL_MODE
	&dev_attr_eol_mode,
	&dev_attr_eol_spec,
#endif
	NULL,
};

static int _3407_handleAlsEvent(ams_deviceCtx_t *ctx);
#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
static void _3407_handleFlickerFIFOEvent(ams_deviceCtx_t *ctx);
#else
static int _3407_handleFlickerEvent(ams_deviceCtx_t *ctx);
#endif

static int ams_deviceEventHandler(ams_deviceCtx_t *ctx)
{
	int ret = 0;
	uint8_t status5 = 0;

	ret = ams_getByte(ctx->portHndl, DEVREG_STATUS, &ctx->shadowStatus1Reg);
	if (ret < 0) {
		ALS_err("%s - failed to get DEVREG_STATUS\n", __func__);
		return ret;
	}

	if (ctx->shadowStatus1Reg & SINT) {
		ret = ams_getByte(ctx->portHndl, DEVREG_STATUS5, &status5);
		if (ret < 0) {
			ALS_err("%s - failed to get DEVREG_STATUS5\n", __func__);
			return ret;
		}
		ALS_info("%s - ctx->shadowStatus1Reg %x, status5 %x, mode %x", __func__, ctx->shadowStatus1Reg, status5, ctx->mode);
	}

	if (ctx->shadowStatus1Reg != 0) {
		/* this clears interrupt(s) and STATUS5 */
		ret = ams_setByte(ctx->portHndl, DEVREG_STATUS, ctx->shadowStatus1Reg);
		if (ret < 0) {
			ALS_err("%s - failed to set DEVREG_STATUS\n", __func__);
			return ret;
		}
	} else {
		ALS_err("%s - ams_devEventHd Error Case!!!!\n", __func__);
		//ams_setByte(ctx->portHndl, DEVREG_STATUS, 0xff);
		return ret;
	}

loop:
	ALS_info("%s - loop: DCB 0x%02x, STATUS 0x%02x, STATUS5 0x%02x\n", __func__, ctx->mode, ctx->shadowStatus1Reg, status5);

	if ((ctx->shadowStatus1Reg & ALS_INT_ALL) /*|| ctx->alwaysReadAls*/) {
		if ((ctx->mode & MODE_ALS_ALL) && (!(ctx->mode & MODE_IRBEAM))) {
			ALS_info("%s - AlsEvent INT:%d alwaysReadAls = %d\n", __func__, (ctx->shadowStatus1Reg & ALS_INT_ALL), ctx->alwaysReadAls);
			ret = _3407_handleAlsEvent(ctx);
			if (ret < 0) {
				ALS_err("%s - failed to _3407_handleAlsEvent\n", __func__);
				return ret;
			}
		}
	}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
	if ((ctx->shadowStatus1Reg & FIFOINT) || ((status5 & SINT_FD))) {
		if (ctx->mode & MODE_FLICKER) {
			//AMS_PORT_log("_3407_handleFlickerEvent\n");
			_3407_handleFlickerFIFOEvent(ctx);
		}
	}
#else
	if ((status5 & SINT_FD)) {
		if (ctx->mode & MODE_FLICKER) {
			ALS_info("%s - FlickerEvent status5:0x%02x  alwaysReadFlicker = %d\n", __func__, (status5 & SINT_FD), ctx->alwaysReadFlicker);
			ret = _3407_handleFlickerEvent(ctx);
			if (ret < 0) {
				ALS_err("%s - failed to _3407_handleFlickerEvent\n", __func__);
				return ret;
			}
		}
	}
#endif

	ret = ams_getByte(ctx->portHndl, DEVREG_STATUS, &ctx->shadowStatus1Reg);
	if (ret < 0) {
		ALS_err("%s - failed to get DEVREG_STATUS\n", __func__);
		return ret;
	}

	if (ctx->shadowStatus1Reg & SINT) {
		ret = ams_getByte(ctx->portHndl, DEVREG_STATUS5, &status5);
		if (ret < 0) {
			ALS_err("%s - failed to get DEVREG_STATUS5\n", __func__);
			return ret;
		}
	} else {
		status5 = 0;
	}

	if (ctx->shadowStatus1Reg != 0) {
		/* this clears interrupt(s) and STATUS5 */
		ret = ams_setByte(ctx->portHndl, DEVREG_STATUS, ctx->shadowStatus1Reg);
		if (ret < 0) {
			ALS_err("%s - failed to set DEVREG_STATUS\n", __func__);
			return ret;
		}
#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
		if (!(ctx->shadowStatus1Reg & (PSAT)))/*changed to update event data during saturation*/
#else
		if (!(ctx->shadowStatus1Reg & (ASAT_FDSAT | PSAT)))/*changed to update event data during saturation*/
#endif
		{
			// AMS_PORT_log_1( "ams_devEventHd loop:go loop shadowStatus1Reg %x!!!!!!!!!!\n",ctx->shadowStatus1Reg);
			ALS_dbg("%s - go_loop DCB 0x%02x, STATUS 0x%02x, STATUS2 0x%02x\n",
			__func__, ctx->mode, ctx->shadowStatus1Reg, ctx->shadowStatus2Reg);

			goto loop;
		}
	}

/*
 *	the individual handlers may have temporarily disabled things
 *	AMS_REENABLE(ret);
 *	if (ret < 0) {
 *		ALS_err("%s - failed to AMS_REENABLE\n", __func__);
 *		return ret;
 *	}
 */
	return ret;
}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_EOL_MODE
static int tcs3407_eol_mode_handler(struct tcs3407_device_data *data)
{
	int i;
	ams_deviceCtx_t *ctx = data->deviceCtx;

	switch (data->eol_state) {
	case EOL_STATE_INIT:
		if (data->eol_result == NULL)
			data->eol_result = devm_kzalloc(&data->client->dev, MAX_TEST_RESULT, GFP_KERNEL);

		for (i = 0; i < EOL_STATE_DONE; i++) {
			data->eol_flicker_awb[i][0] = 0;
			data->eol_flicker_awb[i][1] = 0;
			data->eol_flicker_awb[i][2] = 0;
		}
		data->eol_count = 0;
		data->eol_awb = 0;
		data->eol_clear = 0;
		data->eol_flicker = 0;
		data->eol_flicker_count = 0;
		data->eol_state = EOL_STATE_100;
		data->eol_pulse_count = 0;
		break;
	default:
		data->eol_count++;
		ALS_dbg("%s - %d, %d, %d, %d, %d\n", __func__,
			data->eol_state, data->eol_count, data->eol_flicker, data->eol_awb, data->eol_clear);
		ALS_dbg("%s - raw: %d, %d, %d, %d, %d\n", __func__, ctx->ccbAlsCtx.ctxAlgAls.uvir_cpl,
			ctx->ccbAlsCtx.ctxAlgAls.results.rawClear, ctx->ccbAlsCtx.ctxAlgAls.results.rawWideband,
			ctx->ccbAlsCtx.ctxAlgAls.results.irrClear, ctx->ccbAlsCtx.ctxAlgAls.results.irrWideband); 

		if (data->eol_count >= (EOL_COUNT + EOL_SKIP_COUNT)) {
			data->eol_flicker_awb[data->eol_state][0] = data->eol_flicker / data->eol_flicker_count;
			data->eol_flicker_awb[data->eol_state][1] = data->eol_awb / EOL_COUNT;
			data->eol_flicker_awb[data->eol_state][2] = data->eol_clear / EOL_COUNT;

			ALS_dbg("%s - eol_state = %d, pulse_duty = %d %d, pulse_count = %d\n",
				__func__, data->eol_state, data->eol_pulse_duty[0], data->eol_pulse_duty[1], data->eol_pulse_count);

			data->eol_count = 0;
			data->eol_awb = 0;
			data->eol_clear = 0;
			data->eol_flicker = 0;
			data->eol_flicker_count = 0;
			data->eol_pulse_count = 0;
			data->eol_state++;
		}
		break;
	}

	return 0;
}
#endif

irqreturn_t tcs3407_irq_handler(int dev_irq, void *device)
{
	int err;
	struct tcs3407_device_data *data = device;
	int interruptsHandled = 0;

	ALS_info("%s - als_irq = %d\n", __func__, dev_irq);

	if (data->regulator_state == 0 || data->enabled == 0) {
		ALS_dbg("%s - stop irq handler (reg_state : %d, enabled : %d)\n",
				__func__, data->regulator_state, data->enabled);

		ams_setByte(data->client, DEVREG_STATUS, (AINT | ASAT_FDSAT));
		return IRQ_HANDLED;
	}

#ifdef CONFIG_ARCH_QCOM
	pm_qos_add_request(&data->pm_qos_req_fpm, PM_QOS_CPU_DMA_LATENCY,
		PM_QOS_DEFAULT_VALUE);
#endif
	err = ams_deviceEventHandler(data->deviceCtx);
	interruptsHandled = ams_getResult(data->deviceCtx);

	if (err == 0) {
		if (data->als_input_dev == NULL) {
			ALS_err("%s - als_input_dev is NULL\n", __func__);
		} else {
#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS
			if (interruptsHandled & (1 << AMS_AMBIENT_SENSOR)) {
				report_als(data);
#ifdef CONFIG_AMS_OPTICAL_SENSOR_EOL_MODE
				if (data->eol_enable)
					tcs3407_eol_mode_handler(data);
#endif
			}
#endif

#ifdef CONFIG_AMS_OPTICAL_SENSOR_FLICKER
			if (interruptsHandled & (1 << AMS_FLICKER_SENSOR))
				report_flicker(data);
#endif
		}
	} else {
		ALS_err("%s - ams_deviceEventHandler failed\n", __func__);
	}
#ifdef CONFIG_ARCH_QCOM
	pm_qos_remove_request(&data->pm_qos_req_fpm);
#endif

	return IRQ_HANDLED;
}

static int tcs3407_setup_irq(struct tcs3407_device_data *data)
{
	int errorno = -EIO;

	errorno = request_threaded_irq(data->dev_irq, NULL,
		tcs3407_irq_handler, IRQF_TRIGGER_FALLING|IRQF_ONESHOT,
		"als_rear_sensor_irq", data);

	if (errorno < 0) {
		ALS_err("%s - failed for setup dev_irq errono= %d\n",
			   __func__, errorno);
		errorno = -ENODEV;
		return errorno;
	}

	disable_irq(data->dev_irq);

	return errorno;
}

static void tcs3407_init_var(struct tcs3407_device_data *data)
{
	data->client = NULL;
	data->dev = NULL;
	data->als_input_dev = NULL;
	data->als_pinctrl = NULL;
	data->pins_sleep = NULL;
	data->pins_idle = NULL;
	data->vdd_1p8 = NULL;
	data->i2c_1p8 = NULL;
	data->enabled = 0;
	data->sampling_period_ns = 0;
	data->regulator_state = 0;
	data->irq_state = 0;
	data->reg_read_buf = 0;
	data->pm_state = PM_RESUME;
	data->i2c_err_cnt = 0;
	data->user_ir_data = 0;
	data->user_flicker_data = 0;
#ifdef CONFIG_AMS_OPTICAL_SENSOR_EOL_MODE
	data->eol_pulse_duty[0] = DEFAULT_DUTY_50HZ;
	data->eol_pulse_duty[1] = DEFAULT_DUTY_60HZ;
	data->eol_pulse_count = 0;
#endif
#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
	data->awb_sample_cnt = 0;
	data->flicker_data_cnt = 0;
#endif
}

static int tcs3407_parse_dt(struct tcs3407_device_data *data)
{
	struct device *dev = &data->client->dev;
	struct device_node *dNode = dev->of_node;
	enum of_gpio_flags flags;
	u32 gain_max = 0;

	if (dNode == NULL)
		return -ENODEV;

	data->pin_als_int = of_get_named_gpio_flags(dNode,
		"als_rear,int-gpio", 0, &flags);
	if (data->pin_als_int < 0) {
		ALS_err("%s - get als_rear_int error\n", __func__);
		return -ENODEV;
	}

	data->pin_als_en = of_get_named_gpio_flags(dNode,
		"als_rear,als_en-gpio", 0, &flags);
	if (data->pin_als_en < 0)
		ALS_dbg("%s - get als_en failed\n", __func__);

#ifdef CONFIG_AMS_OPTICAL_SENSOR_EOL_MODE
	data->pin_led_en = of_get_named_gpio_flags(dNode,
		"als_rear,led_en-gpio", 0, &flags);
	if (data->pin_led_en < 0) {
		ALS_err("%s - get pin_led_en error\n", __func__);
		return -ENODEV;
	}
#endif

	if (of_property_read_string(dNode, "als_rear,vdd_1p8",
		(char const **)&data->vdd_1p8) < 0)
		ALS_dbg("%s - vdd_1p8 doesn`t exist\n", __func__);

	if (of_property_read_string(dNode, "als_rear,i2c_1p8",
		(char const **)&data->i2c_1p8) < 0)
		ALS_dbg("%s - i2c_1p8 doesn`t exist\n", __func__);

	data->als_pinctrl = devm_pinctrl_get(dev);

	if (IS_ERR_OR_NULL(data->als_pinctrl)) {
		ALS_err("%s - get pinctrl(%li) error\n",
			__func__, PTR_ERR(data->als_pinctrl));
		data->als_pinctrl = NULL;
		return -EINVAL;
	}

	data->pins_sleep =
		pinctrl_lookup_state(data->als_pinctrl, "sleep");
	if (IS_ERR_OR_NULL(data->pins_sleep)) {
		ALS_err("%s - get pins_sleep(%li) error\n",
			__func__, PTR_ERR(data->pins_sleep));
		devm_pinctrl_put(data->als_pinctrl);
		data->pins_sleep = NULL;
		return -EINVAL;
	}

	data->pins_idle =
		pinctrl_lookup_state(data->als_pinctrl, "idle");
	if (IS_ERR_OR_NULL(data->pins_idle)) {
		ALS_err("%s - get pins_idle(%li) error\n",
			__func__, PTR_ERR(data->pins_idle));

		devm_pinctrl_put(data->als_pinctrl);
		data->pins_idle = NULL;
		return -EINVAL;
	}

	if (of_property_read_u32(dNode, "als_rear,gain_max", &gain_max) == 0) {
		deviceRegisterDefinition[DEVREG_AGC_GAIN_MAX].resetValue = gain_max;

		ALS_dbg("%s - DEVREG_AGC_GAIN_MAX = 0x%x\n", __func__, gain_max);
	}
		
#ifdef CONFIG_AMS_OPTICAL_SENSOR_EOL_MODE
	if (of_property_read_u32_array(dNode, "als_rear,ir_spec",
		data->eol_ir_spec, ARRAY_SIZE(data->eol_ir_spec)) < 0) {
		ALS_err("%s - get ir_spec error\n", __func__);

		data->eol_ir_spec[0] = DEFAULT_IR_SPEC_MIN;
		data->eol_ir_spec[1] = DEFAULT_IR_SPEC_MAX;
		data->eol_ir_spec[2] = DEFAULT_IR_SPEC_MIN;
		data->eol_ir_spec[3] = DEFAULT_IR_SPEC_MAX;
	}
	ALS_dbg("%s - ir_spec = %d, %d, %d, %d\n", __func__,
		data->eol_ir_spec[0], data->eol_ir_spec[1], data->eol_ir_spec[2], data->eol_ir_spec[3]);
	
	if (of_property_read_u32_array(dNode, "als_rear,clear_spec",
		data->eol_clear_spec, ARRAY_SIZE(data->eol_clear_spec)) < 0) {
		ALS_err("%s - get clear_spec error\n", __func__);

		data->eol_clear_spec[0] = DEFAULT_IR_SPEC_MIN;
		data->eol_clear_spec[1] = DEFAULT_IR_SPEC_MAX;
		data->eol_clear_spec[2] = DEFAULT_IR_SPEC_MIN;
		data->eol_clear_spec[3] = DEFAULT_IR_SPEC_MAX;
	}
	ALS_dbg("%s - clear_spec = %d, %d, %d, %d\n", __func__,
		data->eol_clear_spec[0], data->eol_clear_spec[1], data->eol_clear_spec[2], data->eol_clear_spec[3]);
#endif

	ALS_dbg("%s - done.\n", __func__);

	return 0;
}

static int tcs3407_setup_gpio(struct tcs3407_device_data *data)
{
	int errorno = -EIO;

	errorno = gpio_request(data->pin_als_int, "als_rear_int");
	if (errorno) {
		ALS_err("%s - failed to request als_int\n", __func__);
		return errorno;
	}

	errorno = gpio_direction_input(data->pin_als_int);
	if (errorno) {
		ALS_err("%s - failed to set als_int as input\n", __func__);
		goto err_gpio_direction_input;
	}
	data->dev_irq = gpio_to_irq(data->pin_als_int);

	if (data->pin_als_en >= 0) {
		errorno = gpio_request(data->pin_als_en, "als_rear_en");
		if (errorno) {
			ALS_err("%s - failed to request als_en\n", __func__);
			goto err_gpio_direction_input;
		}
	}
	goto done;

err_gpio_direction_input:
	gpio_free(data->pin_als_int);
done:
	return errorno;
}

static int _3407_resetAllRegisters(AMS_PORT_portHndl *portHndl)
{
	int err = 0;
/*
 *	ams_deviceRegister_t i;
 *
 *	for (i = DEVREG_ENABLE; i <= DEVREG_CFG1; i++) {
 *		ams_setByte(portHndl, i, deviceRegisterDefinition[i].resetValue);
 *	}
 *	for (i = DEVREG_STATUS; i < DEVREG_REG_MAX; i++) {
 *		ams_setByte(portHndl, i, deviceRegisterDefinition[i].resetValue);
 *	}
 */
	// To prevent SIDE EFFECT , below register should be written
	err = ams_setByte(portHndl, DEVREG_CFG6, deviceRegisterDefinition[DEVREG_CFG6].resetValue);
	if (err < 0) {
		ALS_err("%s - failed to set DEVREG_CFG6\n", __func__);
		return err;
	}
	err = ams_setByte(portHndl, DEVREG_AGC_GAIN_MAX, deviceRegisterDefinition[DEVREG_AGC_GAIN_MAX].resetValue);
	if (err < 0) {
		ALS_err("%s - failed to set DEVREG_AGC_GAIN_MAX\n", __func__);
		return err;
	}

	err = ams_setByte(portHndl, DEVREG_FD_CFG3, deviceRegisterDefinition[DEVREG_FD_CFG3].resetValue);
	if (err < 0) {
		ALS_err("%s - failed to set DEVREG_FD_CFG3\n", __func__);
		return err;
	}

	return err;
}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS_CCB
static int _3407_alsInit(ams_deviceCtx_t *ctx, ams_calibrationData_t *calibrationData)
{
	int ret = 0;

	if (calibrationData == NULL) {
		ams_ccb_als_info_t infoData;

		ALS_info("%s - calibrationData is null\n", __func__);
		ccb_alsInfo(&infoData);
	   // ctx->ccbAlsCtx.initData.calibrationData.luxTarget = infoData.defaultCalibrationData.luxTarget;
	   // ctx->ccbAlsCtx.initData.calibrationData.luxTargetError = infoData.defaultCalibrationData.luxTargetError;
		ctx->ccbAlsCtx.initData.calibrationData.calibrationFactor = infoData.defaultCalibrationData.calibrationFactor;
		ctx->ccbAlsCtx.initData.calibrationData.Time_base = infoData.defaultCalibrationData.Time_base;
		ctx->ccbAlsCtx.initData.calibrationData.thresholdLow = infoData.defaultCalibrationData.thresholdLow;
		ctx->ccbAlsCtx.initData.calibrationData.thresholdHigh = infoData.defaultCalibrationData.thresholdHigh;
		ctx->ccbAlsCtx.initData.calibrationData.calibrationFactor = infoData.defaultCalibrationData.calibrationFactor;
	} else {
		ALS_info("%s - calibrationData is non-null\n", __func__);
		//ctx->ccbAlsCtx.initData.calibrationData.luxTarget = calibrationData->alsCalibrationLuxTarget;
		//ctx->ccbAlsCtx.initData.calibrationData.luxTargetError = calibrationData->alsCalibrationLuxTargetError;
		ctx->ccbAlsCtx.initData.calibrationData.calibrationFactor = calibrationData->alsCalibrationFactor;
		ctx->ccbAlsCtx.initData.calibrationData.Time_base = calibrationData->timeBase_us;
		ctx->ccbAlsCtx.initData.calibrationData.thresholdLow = calibrationData->alsThresholdLow;
		ctx->ccbAlsCtx.initData.calibrationData.thresholdHigh = calibrationData->alsThresholdHigh;
		//ctx->ccbAlsCtx.initData.calibrationData.calibrationFactor = calibrationData->alsCalibrationFactor;
	}
	ctx->ccbAlsCtx.initData.calibrate = false;
	ctx->ccbAlsCtx.initData.configData.gain = 64000;//AGAIN
	ctx->ccbAlsCtx.initData.configData.uSecTime = AMS_ALS_ATIME; /*ALS Inegration time 50msec*/

	ctx->alwaysReadAls = false;
	ctx->alwaysReadFlicker = false;
	ctx->ccbAlsCtx.initData.autoGain = true; //AutoGainCtrol on
	ctx->ccbAlsCtx.initData.hysteresis = 0x02; /*Lower threshold for adata in AGC */
	if (ctx->ccbAlsCtx.initData.autoGain) {
		AMS_SET_ALS_AUTOGAIN(HIGH, ret);
		if (ret < 0) {
			ALS_err("%s - failed to AMS_SET_ALS_AUTOGAIN\n", __func__);
			return ret;
		}
/*******************************/
/*
 * - ALS_AGC_LOW_HYST -
 * 0 -> 12.5 %
 * 1 -> 25 %
 * 2 -> 37.5 %
 * 3 -> 50 %
 *
 * - ALS_AGC_HIGH_HYST -
 * 0 -> 50 %
 * 1 -> 62.5 %
 * 2 -> 75 %
 * 3 -> 87.5 %
 */
/*******************************/
		AMS_SET_ALS_AGC_LOW_HYST(0);		// Low HYST -> 12.5 %
		AMS_SET_ALS_AGC_HIGH_HYST(3);		//  High HYST -> 87.5 %
/*
 *		AMS_SET_ALS_AGC_HYST(ctx->ccbAlsCtx.initData.hysteresis, ret);
 *		if (ret < 0) {
 *			ALS_err("%s - failed to AMS_SET_ALS_AGC_HYST\n", __func__);
 *			return ret;
 *		}
 */
	}
	return ret;
}

static bool ams_deviceGetAls(ams_deviceCtx_t *ctx, ams_apiAls_t *exportData)
{
	ams_ccb_als_result_t result;

	ccb_alsGetResult(ctx, &result);
	exportData->clear		= result.clear;
	exportData->red         = result.red;
	exportData->green       = result.green;
	exportData->blue        = result.blue;
	exportData->ir          = result.ir;
	exportData->time_us		= result.time_us;
	exportData->gain		= result.gain;
//	exportData->wideband    = result.wideband;
	exportData->rawClear    = result.rawClear;
	exportData->rawRed      = result.rawRed;
	exportData->rawGreen    = result.rawGreen;
	exportData->rawBlue     = result.rawBlue;
	exportData->rawWideband = result.rawWideband;
	return false;
}

static int _3407_handleAlsEvent(ams_deviceCtx_t *ctx)
{
	int ret = 0;
	ams_ccb_als_dataSet_t ccbAlsData;

	ccbAlsData.statusReg = ctx->shadowStatus1Reg;
	ret = ccb_alsHandle(ctx, &ccbAlsData);

	return ret;
}

#endif

#ifdef CONFIG_AMS_OPTICAL_SENSOR_FLICKER
#ifndef CONFIG_AMS_OPTICAL_SENSOR_FIFO
static int _3407_flickerInit(ams_deviceCtx_t *ctx)
{
	int err = 0;
	ams_flicker_ctx_t *flickerCtx = (ams_flicker_ctx_t *)&ctx->flickerCtx;

	flickerCtx->lastValid.freq100Hz = NOT_VALID;
	flickerCtx->lastValid.freq120Hz = NOT_VALID;
	flickerCtx->lastValid.mHz = 0;

	err = ams_setByte(ctx->portHndl, DEVREG_FD_CFG0, ((FD_SAMPLES_128) | (FD_COMPARE_8_32NDS)));
	if (err < 0) {
		ALS_err("%s - failed to set DEVREG_FD_CFG0\n", __func__);
		return err;
	}
	err = ams_setByte(ctx->portHndl, DEVREG_FD_CFG3, (FD_GAIN_128 | (1 & MASK_FD_TIME_MSBits)));
	if (err < 0) {
		ALS_err("%s - failed to set DEVREG_FD_CFG3\n", __func__);
		return err;
	}
	err = ams_setField(ctx->portHndl, DEVREG_CFG10, FD_PERS_ALWAYS, MASK_FD_PERS);
	if (err < 0) {
		ALS_err("%s - failed to set DEVREG_CFG10\n", __func__);
		return err;
	}

	return err;
}
#endif

static bool ams_deviceGetFlicker(ams_deviceCtx_t *ctx, ams_apiAlsFlicker_t *exportData)
{
	ams_flicker_ctx_t *flickerCtx = (ams_flicker_ctx_t *)&ctx->flickerCtx;

	if (flickerCtx->statusReg & FD_100HZ_VALID)
		exportData->freq100Hz = flickerCtx->lastValid.freq100Hz =
			(flickerCtx->statusReg & FD_100HZ_FLICKER) ? PRESENT : ABSENT;
	else
		exportData->freq100Hz = flickerCtx->lastValid.freq100Hz;

	if (flickerCtx->statusReg & FD_120HZ_VALID)
		exportData->freq120Hz = flickerCtx->lastValid.freq120Hz =
			(flickerCtx->statusReg & FD_120HZ_FLICKER) ? PRESENT : ABSENT;
	else
		exportData->freq120Hz = flickerCtx->lastValid.freq120Hz;

	if ((exportData->freq100Hz == PRESENT) && (exportData->freq120Hz == PRESENT))
		exportData->mHz = flickerCtx->lastValid.mHz = (uint32_t)(ULONG_MAX);
	else if (exportData->freq100Hz == PRESENT)
		exportData->mHz = flickerCtx->lastValid.mHz = 100000;
	else if (exportData->freq120Hz == PRESENT)
		exportData->mHz = flickerCtx->lastValid.mHz = 120000;
	else
		exportData->mHz = 0;
	return false;
}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
static void _3407_handleFlickerFIFOEvent(ams_deviceCtx_t *ctx)
{
	ams_ccb_als_dataSet_t ccbAlsData;

	ccb_FlickerFIFOEvent(ctx, &ccbAlsData);
}
#else
static int _3407_handleFlickerEvent(ams_deviceCtx_t *ctx)
{
	int ret = 0;
	ams_flicker_ctx_t *flickerCtx = (ams_flicker_ctx_t *)&ctx->flickerCtx;

	ret = ams_getByte(ctx->portHndl, DEVREG_FD_STATUS, &flickerCtx->statusReg);
	if (ret < 0) {
		ALS_err("%s - failed to get DEVREG_FD_STATUS\n", __func__);
		return ret;
	}
	ret = ams_setByte(ctx->portHndl, DEVREG_FD_STATUS, MASK_CLEAR_FLICKER_STATUS);
	if (ret < 0) {
		ALS_err("%s - failed to set DEVREG_FD_STATUS\n", __func__);
		return ret;
	}
//	ams_setByte(ctx->portHndl, DEVREG_FD_STATUS, flickerCtx->statusReg);

	if (flickerCtx->statusReg & MASK_FLICKER_VALID)
		ctx->updateAvailable |= (1 << AMS_FLICKER_SENSOR);
	ALS_info("%s - FD status = 0x%02x\n", __func__, flickerCtx->statusReg);

	return ret;
}
#endif
#endif
static int ams_deviceSoftReset(ams_deviceCtx_t *ctx)
{
	int err = 0;

	ALS_dbg("%s - Start\n", __func__);

	// Before S/W reset, the PON has to be asserted
	err = ams_setByte(ctx->portHndl, DEVREG_ENABLE, PON);
	if (err < 0) {
		ALS_err("%s - failed to set DEVREG_ENABLE\n", __func__);
		return err;
	}

	err = ams_setField(ctx->portHndl, DEVREG_SOFT_RESET, HIGH, MASK_SOFT_RESET);
	if (err < 0) {
		ALS_err("%s - failed to set DEVREG_SOFT_RESET\n", __func__);
		return err;
	}
	// Need 1 msec delay
	usleep_range(1000, 1100);

	// Recover the previous enable setting
	err = ams_setByte(ctx->portHndl, DEVREG_ENABLE, ctx->shadowEnableReg);
	if (err < 0) {
		ALS_err("%s - failed to set DEVREG_ENABLE\n", __func__);
		return err;
	}

	return err;
}

static ams_deviceIdentifier_e ams_validateDevice(AMS_PORT_portHndl *portHndl)
{
	uint8_t chipId;
	uint8_t revId;
	uint8_t auxId;
	uint8_t i = 0;
	int err = 0;

	struct tcs3407_device_data *data = i2c_get_clientdata(portHndl);

	err = ams_getByte(portHndl, DEVREG_ID, &chipId);
	if (err < 0) {
		ALS_err("%s - failed to get DEVREG_ID\n", __func__);
		return AMS_UNKNOWN_DEVICE;
	}
	err = ams_getByte(portHndl, DEVREG_REVID, &revId);
	if (err < 0) {
		ALS_err("%s - failed to get DEVREG_REVID\n", __func__);
		return AMS_UNKNOWN_DEVICE;
	}

	do {
		if (((chipId & deviceIdentifier[i].deviceIdMask) ==
			(deviceIdentifier[i].deviceId & deviceIdentifier[i].deviceIdMask)) &&
			((revId & deviceIdentifier[i].deviceRefMask) ==
			 (deviceIdentifier[i].deviceRef & deviceIdentifier[i].deviceRefMask))) {

			err = ams_getByte(portHndl, DEVREG_AUXID, &auxId);
			if (err < 0) {
				ALS_err("%s - failed to get DEVREG_ID\n", __func__);
				return AMS_UNKNOWN_DEVICE;
			}

			if (auxId & 0x03)
				data->isTrimmed = 1;
			else
				data->isTrimmed = 0;

			ALS_dbg("%s - ID:0x%02x, revID:0x%02x, auxID:0x%02x\n", __func__, chipId, revId, auxId);

			return deviceIdentifier[i].device;
		}
		i++;
	} while (deviceIdentifier[i].device != AMS_LAST_DEVICE);

	return AMS_UNKNOWN_DEVICE;
}

static int ams_deviceInit(ams_deviceCtx_t *ctx, AMS_PORT_portHndl *portHndl, ams_calibrationData_t *calibrationData)
{
	int ret = 0;

	ctx->portHndl = portHndl;
	ctx->mode = MODE_OFF;
	ctx->systemCalibrationData = calibrationData;
	ctx->deviceId = ams_validateDevice(ctx->portHndl);
	ctx->shadowEnableReg = deviceRegisterDefinition[DEVREG_ENABLE].resetValue;
	ret = ams_deviceSoftReset(ctx);
	if (ret < 0) {
		ALS_err("%s - failed to ams_deviceSoftReset\n", __func__);
		return ret;
	}

	ret = _3407_resetAllRegisters(ctx->portHndl);
	if (ret < 0) {
		ALS_err("%s - failed to _3407_resetAllRegisters\n", __func__);
		return ret;
	}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS_CCB
	ret = _3407_alsInit(ctx, calibrationData);
	if (ret < 0) {
		ALS_err("%s - failed to _3407_alsInit\n", __func__);
		return ret;
	}
#endif
/*
 *	ret = ams_setByte(ctx->portHndl, DEVREG_ENABLE, ctx->shadowEnableReg);
 *	if (ret < 0) {
 *		ALS_err("%s - failed to set DEVREG_ENABLE\n", __func__);
 *		return ret;
 *	}
 */
	return ret;
}

static bool ams_getDeviceInfo(ams_deviceInfo_t *info)
{
	memset(info, 0, sizeof(ams_deviceInfo_t));

	info->defaultCalibrationData.timeBase_us = AMS_USEC_PER_TICK;
	info->numberOfSubSensors = 0;
	info->memorySize =  sizeof(ams_deviceCtx_t);
	info->deviceModel = "TCS3407";
	memcpy(info->defaultCalibrationData.deviceName, info->deviceModel, sizeof(info->defaultCalibrationData.deviceName));
	info->deviceName  = "ALS/PRX/FLKR";
	info->driverVersion = "Alpha";
#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS_CCB
		{
		/* TODO */
		ams_ccb_als_info_t infoData;

		ccb_alsInfo(&infoData);
		info->tableSubSensors[info->numberOfSubSensors] = AMS_AMBIENT_SENSOR;
		info->numberOfSubSensors++;

		info->alsSensor.driverName = infoData.algName;
		info->alsSensor.adcBits = 8;
		info->alsSensor.maxPolRate = 50;
		info->alsSensor.activeCurrent_uA = 100;
		info->alsSensor.standbyCurrent_uA = 5;
		info->alsSensor.rangeMax = 1;
		info->alsSensor.rangeMin = 0;

		info->defaultCalibrationData.alsCalibrationFactor = infoData.defaultCalibrationData.calibrationFactor;
//		info->defaultCalibrationData.alsCalibrationLuxTarget = infoData.defaultCalibrationData.luxTarget;
//		info->defaultCalibrationData.alsCalibrationLuxTargetError = infoData.defaultCalibrationData.luxTargetError;
#if defined(CONFIG_AMS_ALS_CRWBI) || defined(CONFIG_AMS_ALS_CRGBW)
		info->tableSubSensors[info->numberOfSubSensors] = AMS_WIDEBAND_ALS_SENSOR;
		info->numberOfSubSensors++;
#endif
	}
#endif
	return false;
}

int tcs3407_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = -ENODEV;
	struct device *dev = &client->dev;
	static struct tcs3407_device_data *data;
	struct amsdriver_i2c_platform_data *pdata = dev->platform_data;
	ams_deviceInfo_t amsDeviceInfo;
	ams_deviceIdentifier_e deviceId;

	ALS_dbg("%s - start\n", __func__);
	/* check to make sure that the adapter supports I2C */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		ALS_err("%s - I2C_FUNC_I2C not supported\n", __func__);
		return -ENODEV;
	}
	/* allocate some memory for the device */
	data = devm_kzalloc(dev, sizeof(struct tcs3407_device_data), GFP_KERNEL);
	if (data == NULL) {
		ALS_err("%s - couldn't allocate device data memory\n", __func__);
		return -ENOMEM;
	}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
	data->flicker_data = devm_kzalloc(dev, sizeof(int)*FLICKER_DATA_CNT, GFP_KERNEL);
	if (data == NULL) {
		ALS_err("%s - couldn't allocate device flicker_data memory\n", __func__);
		return -ENOMEM;
	}
#endif

	tcs3407_data = data;
	tcs3407_init_var(data);

	if (!pdata) {
		pdata = devm_kzalloc(dev, sizeof(struct amsdriver_i2c_platform_data),
				GFP_KERNEL);
		if (pdata == NULL) {
			ALS_err("%s - couldn't allocate device pdata memory\n", __func__);
			goto err_malloc_pdata;
		}
		if (of_match_device(tcs3407_match_table, &client->dev))
			pdata->of_node = client->dev.of_node;
	}

	data->client = client;
	data->miscdev.minor = MISC_DYNAMIC_MINOR;
	data->miscdev.name = MODULE_NAME_ALS;
#ifdef CONFIG_AMS_OPTICAL_SENSOR_FIFO
	data->miscdev.fops = &tcs3407_fops;
#endif
	data->miscdev.mode = S_IRUGO;
	data->pdata = pdata;
	i2c_set_clientdata(client, data);
	ALS_info("%s client = %p\n", __func__, client);

	err = misc_register(&data->miscdev);
	if (err < 0) {
		ALS_err("%s - failed to misc device register\n", __func__);
		goto err_misc_register;
	}
	mutex_init(&data->i2clock);
	mutex_init(&data->activelock);
	mutex_init(&data->suspendlock);
	mutex_init(&data->flickerdatalock);

	err = tcs3407_parse_dt(data);
	if (err < 0) {
		ALS_err("%s - failed to parse dt\n", __func__);
		err = -ENODEV;
		goto err_parse_dt;
	}
	err = tcs3407_setup_gpio(data);
	if (err) {
		ALS_err("%s - failed to setup gpio\n", __func__);
		goto err_setup_gpio;
	}
	err = tcs3407_power_ctrl(data, PWR_ON);
	if (err < 0) {
		ALS_err("%s - failed to power on ctrl\n", __func__);
		goto err_power_on;
	}
	if (data->client->addr == TCS3407_SLAVE_I2C_ADDR_REVID_V0) {
		ALS_dbg("%s - slave address is REVID_V0\n", __func__);
	} else if (data->client->addr == TCS3407_SLAVE_I2C_ADDR_REVID_V1) {
		ALS_dbg("%s - slave address is REVID_V1\n", __func__);
	} else {
		err = -EIO;
		ALS_err("%s - slave address error, 0x%02x\n", __func__, data->client->addr);
		goto err_init_fail;
	}

	/********************************************************************/
	/* Validate the appropriate ams device is available for this driver */
	/********************************************************************/
	deviceId = ams_validateDevice(data->client);

	if (deviceId == AMS_UNKNOWN_DEVICE) {
		ALS_err("%s - ams_validateDevice failed: AMS_UNKNOWN_DEVICE\n", __func__);
		err = -EIO;
		goto err_id_failed;
	}
	ALS_dbg("%s - deviceId: %d\n", __func__, deviceId);

	ams_getDeviceInfo(&amsDeviceInfo);
	ALS_dbg("%s - name: %s, model: %s, driver ver:%s\n", __func__,
				amsDeviceInfo.deviceName, amsDeviceInfo.deviceModel, amsDeviceInfo.driverVersion);

	data->deviceCtx = devm_kzalloc(dev, amsDeviceInfo.memorySize, GFP_KERNEL);
	if (data->deviceCtx == NULL) {
		ALS_err("%s - couldn't allocate device deviceCtx memory\n", __func__);
		err = -ENOMEM;
		goto err_malloc_deviceCtx;
	}

	err = ams_deviceInit(data->deviceCtx, data->client, NULL);
	if (err < 0) {
		ALS_err("%s - ams_deviceInit failed.\n", __func__);
		goto err_id_failed;
	} else {
		ALS_dbg("%s - ams_amsDeviceInit ok\n", __func__);
	}

	data->als_input_dev = input_allocate_device();
	if (!data->als_input_dev) {
		ALS_err("%s - could not allocate input device\n", __func__);
		err = -EIO;
		goto err_input_allocate_device;
	}
	data->als_input_dev->name = MODULE_NAME_ALS;
	data->als_input_dev->id.bustype = BUS_I2C;
	input_set_drvdata(data->als_input_dev, data);
	input_set_capability(data->als_input_dev, EV_REL, REL_X);
	input_set_capability(data->als_input_dev, EV_REL, REL_Y);
	input_set_capability(data->als_input_dev, EV_REL, REL_Z);
	input_set_capability(data->als_input_dev, EV_REL, REL_RX);
	input_set_capability(data->als_input_dev, EV_REL, REL_RY);
	input_set_capability(data->als_input_dev, EV_REL, REL_RZ);
	input_set_capability(data->als_input_dev, EV_REL, REL_MISC);
	input_set_capability(data->als_input_dev, EV_ABS, ABS_X);
	input_set_capability(data->als_input_dev, EV_ABS, ABS_Y);
	input_set_capability(data->als_input_dev, EV_ABS, ABS_Z);

	err = input_register_device(data->als_input_dev);
	if (err < 0) {
		input_free_device(data->als_input_dev);
		ALS_err("%s - could not register input device\n", __func__);
		goto err_input_register_device;
	}
#ifdef CONFIG_ARCH_QCOM
	err = sensors_create_symlink(&data->als_input_dev->dev.kobj,
				data->als_input_dev->name);
#else
	err = sensors_create_symlink(data->als_input_dev);
#endif
	if (err < 0) {
		ALS_err("%s - could not create_symlink\n", __func__);
		goto err_sensors_create_symlink;
	}
	err = sysfs_create_group(&data->als_input_dev->dev.kobj,
				 &als_attribute_group);
	if (err) {
		ALS_err("%s - could not create sysfs group\n", __func__);
		goto err_sysfs_create_group;
	}
#ifdef CONFIG_ARCH_QCOM
	err = sensors_register(&data->dev, data, tcs3407_sensor_attrs,
			MODULE_NAME_ALS);
#else
	err = sensors_register(data->dev, data, tcs3407_sensor_attrs,
			MODULE_NAME_ALS);
#endif
	if (err) {
		ALS_err("%s - cound not register als_sensor(%d).\n", __func__, err);
		goto als_sensor_register_failed;
	}

	err = tcs3407_setup_irq(data);
	if (err) {
		ALS_err("%s - could not setup dev_irq\n", __func__);
		goto err_setup_irq;
	}

	err = tcs3407_power_ctrl(data, PWR_OFF);
	if (err < 0) {
		ALS_err("%s - failed to power off ctrl\n", __func__);
		goto dev_set_drvdata_failed;
	}
	ALS_dbg("%s - success\n", __func__);
	goto done;

dev_set_drvdata_failed:
	free_irq(data->dev_irq, data);
err_setup_irq:
	sensors_unregister(data->dev, tcs3407_sensor_attrs);
als_sensor_register_failed:
	sysfs_remove_group(&data->als_input_dev->dev.kobj,
				&als_attribute_group);
err_sysfs_create_group:
#ifdef CONFIG_ARCH_QCOM
	sensors_remove_symlink(&data->als_input_dev->dev.kobj,
				data->als_input_dev->name);
#else
	sensors_remove_symlink(data->als_input_dev);
#endif
err_sensors_create_symlink:
	input_unregister_device(data->als_input_dev);
err_input_register_device:
err_input_allocate_device:
err_id_failed:
//	devm_kfree(data->deviceCtx);
err_malloc_deviceCtx:
err_init_fail:
	tcs3407_power_ctrl(data, PWR_OFF);
err_power_on:
	gpio_free(data->pin_als_int);
	if (data->pin_als_en >= 0)
		gpio_free(data->pin_als_en);
err_setup_gpio:
err_parse_dt:
//	devm_kfree(pdata);
err_malloc_pdata:
	if (data->als_pinctrl) {
		devm_pinctrl_put(data->als_pinctrl);
		data->als_pinctrl = NULL;
	}
	if (data->pins_idle)
		data->pins_idle = NULL;
	if (data->pins_sleep)
		data->pins_sleep = NULL;

	mutex_destroy(&data->i2clock);
	mutex_destroy(&data->activelock);
	mutex_destroy(&data->suspendlock);
	mutex_destroy(&data->flickerdatalock);
	misc_deregister(&data->miscdev);
err_misc_register:
//	devm_kfree(data);
	ALS_err("%s failed\n", __func__);
done:
	return err;
}

int tcs3407_remove(struct i2c_client *client)
{
	struct tcs3407_device_data *data = i2c_get_clientdata(client);

	ALS_dbg("%s - start\n", __func__);
	tcs3407_power_ctrl(data, PWR_OFF);

	sensors_unregister(data->dev, tcs3407_sensor_attrs);
	sysfs_remove_group(&data->als_input_dev->dev.kobj,
			   &als_attribute_group);
#ifdef CONFIG_ARCH_QCOM
	sensors_remove_symlink(&data->als_input_dev->dev.kobj,
				data->als_input_dev->name);
#else
	sensors_remove_symlink(data->als_input_dev);
#endif
	input_unregister_device(data->als_input_dev);

	if (data->als_pinctrl) {
		devm_pinctrl_put(data->als_pinctrl);
		data->als_pinctrl = NULL;
	}
	if (data->pins_idle)
		data->pins_idle = NULL;
	if (data->pins_sleep)
		data->pins_sleep = NULL;
	disable_irq(data->dev_irq);
	free_irq(data->dev_irq, data);
	gpio_free(data->pin_als_int);
	if (data->pin_als_en >= 0)
		gpio_free(data->pin_als_en);
	mutex_destroy(&data->i2clock);
	mutex_destroy(&data->activelock);
	mutex_destroy(&data->suspendlock);
	mutex_destroy(&data->flickerdatalock);
	misc_deregister(&data->miscdev);

//	devm_kfree(data->deviceCtx);
//	devm_kfree(data->pdata);
//	devm_kfree(data);
	i2c_set_clientdata(client, NULL);

	data = NULL;
	return 0;
}

static void tcs3407_shutdown(struct i2c_client *client)
{
	ALS_dbg("%s - start\n", __func__);
}

void tcs3407_pin_control(struct tcs3407_device_data *data, bool pin_set)
{
	int status = 0;

	if (!data->als_pinctrl) {
		ALS_err("%s - als_pinctrl is null\n", __func__);
		return;
	}
	if (pin_set) {
		if (!IS_ERR_OR_NULL(data->pins_idle)) {
			status = pinctrl_select_state(data->als_pinctrl,
				data->pins_idle);
			if (status)
				ALS_err("%s - can't set pin default state\n",
					__func__);
			ALS_info("%s idle\n", __func__);
		}
	} else {
		if (!IS_ERR_OR_NULL(data->pins_sleep)) {
			status = pinctrl_select_state(data->als_pinctrl,
				data->pins_sleep);
			if (status)
				ALS_err("%s - can't set pin sleep state\n",
					__func__);
			ALS_info("%s sleep\n", __func__);
		}
	}
}

#ifdef CONFIG_PM
static int tcs3407_suspend(struct device *dev)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);
	int err = 0;

	ALS_dbg("%s - %d\n", __func__, data->enabled);

	if (data->enabled != 0 || data->regulator_state != 0) {
		mutex_lock(&data->activelock);

		als_enable_set(data, AMSDRIVER_ALS_DISABLE);

		err = tcs3407_power_ctrl(data, PWR_OFF);
		if (err < 0)
			ALS_err("%s - als_regulator_off fail err = %d\n",
				__func__, err);
		tcs3407_irq_set_state(data, PWR_OFF);

		mutex_unlock(&data->activelock);
	}
	mutex_lock(&data->suspendlock);

	data->pm_state = PM_SUSPEND;
	tcs3407_pin_control(data, false);

	mutex_unlock(&data->suspendlock);

	return err;
}

static int tcs3407_resume(struct device *dev)
{
	struct tcs3407_device_data *data = dev_get_drvdata(dev);
	int err = 0;

	ALS_dbg("%s - %d\n", __func__, data->enabled);

	mutex_lock(&data->suspendlock);

	tcs3407_pin_control(data, true);

	data->pm_state = PM_RESUME;

	mutex_unlock(&data->suspendlock);

	if (data->enabled != 0) {
		mutex_lock(&data->activelock);

		tcs3407_irq_set_state(data, PWR_ON);

		err = tcs3407_power_ctrl(data, PWR_ON);
		if (err < 0)
			ALS_err("%s - als_regulator_on fail err = %d\n",
				__func__, err);

		als_enable_set(data, AMSDRIVER_ALS_ENABLE);

		if (err < 0) {
			input_report_rel(data->als_input_dev,
				REL_RZ, -5 + 1); /* F_ERR_I2C -5 detected i2c error */
			input_sync(data->als_input_dev);
			ALS_err("%s - awb mode enable error : %d\n", __func__, err);
		}

		mutex_unlock(&data->activelock);
	}
	return err;
}

static const struct dev_pm_ops tcs3407_pm_ops = {
	.suspend = tcs3407_suspend,
	.resume = tcs3407_resume
};
#endif

static const struct i2c_device_id tcs3407_idtable[] = {
	{ "tcs3407", 0 },
	{ }
};
/* descriptor of the tcs3407 I2C driver */
static struct i2c_driver tcs3407_driver = {
	.driver = {
		.name = "tcs3407",
		.owner = THIS_MODULE,
#if defined(CONFIG_PM)
		.pm = &tcs3407_pm_ops,
#endif
		.of_match_table = tcs3407_match_table,
	},
	.probe = tcs3407_probe,
	.remove = tcs3407_remove,
	.shutdown = tcs3407_shutdown,
	.id_table = tcs3407_idtable,
};

/* initialization and exit functions */
static int __init tcs3407_init(void)
{
	if (!lpcharge)
		return i2c_add_driver(&tcs3407_driver);
	else
		return 0;
}

static void __exit tcs3407_exit(void)
{
	i2c_del_driver(&tcs3407_driver);
}

module_init(tcs3407_init);
module_exit(tcs3407_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("TCS3407 ALS Driver");
MODULE_LICENSE("GPL");
