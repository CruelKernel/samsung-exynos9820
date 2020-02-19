/*
* Copyright (c) 2014 JY Kim, jy.kim@maximintegrated.com
* Copyright (c) 2013 Maxim Integrated Products, Inc.
*
* This software is licensed under the terms of the GNU General Public
* License, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define VENDOR				"MAXIM"

#define MAX86915_CHIP_NAME	"MAX86915"
#define MAX86917_CHIP_NAME	"MAX86917"

#define VERSION				"3"
#define SUB_VERSION			"0"
#define VENDOR_VERSION		"m"

#define MODULE_NAME_HRM		"hrm_sensor"

#define SLAVE_ADDR_MAX		0x57

#define MAX86915_I2C_RETRY_DELAY	10
#define MAX86915_I2C_MAX_RETRIES	5

#define DEFAULT_THRESHOLD			-4194303
#define MAX86915_THRESHOLD_DEFAULT	100000

/* Default Register Value */
#define MAX86915_PART_ID			0x2B
#define MAX86915_REV_ID				0x01

#define MAX86915_HRM_DEFAULT_CURRENT1		0x46 /* IR */
#define MAX86915_HRM_DEFAULT_CURRENT2		0x00 /* RED */
#define MAX86915_HRM_DEFAULT_CURRENT3		0x00 /* GREEN */
#define MAX86915_HRM_DEFAULT_CURRENT4		0x00 /* BLUE */

#define MAX86915_HRM_DEFAULT_XTALK_CODE1	0x00 /* IR */
#define MAX86915_HRM_DEFAULT_XTALK_CODE2	0x00 /* RED */
#define MAX86915_HRM_DEFAULT_XTALK_CODE3	0x00 /* GREEN */
#define MAX86915_HRM_DEFAULT_XTALK_CODE4	0x00 /* BLUE */

#define MAX86915_DEFAULT_XTALK_CODE1	0x00 /* IR */
#define MAX86915_DEFAULT_XTALK_CODE2	0x00 /* RED */
#define MAX86915_DEFAULT_XTALK_CODE3	0x00 /* GREEN */
#define MAX86915_DEFAULT_XTALK_CODE4	0x00 /* BLUE */

#define MAX86915_IR_INIT_CURRENT	0x46 /* IR */
#define MAX86915_RED_INIT_CURRENT	0x41 /* RED */
#define MAX86915_GREEN_INIT_CURRENT	0x28 /* GREEN */
#define MAX86915_BLUE_INIT_CURRENT	0x3C /* BLUE */

#define MAX86915_IR_CURRENT_STEP	0x04 /* IR */
#define MAX86915_RED_CURRENT_STEP	0x04 /* RED */
#define MAX86915_GREEN_CURRENT_STEP	0x03 /* GREEN */
#define MAX86915_BLUE_CURRENT_STEP	0x05 /* BLUE */

/* AWB/Flicker Definition */
#define AWB_INTERVAL		20 /* 20 sample(from 17 to 28) */

#define CONFIG_SKIP_CNT		8
#define FLICKER_DATA_CNT	200
#define MAX86915_IOCTL_MAGIC		0xFD
#define MAX86915_IOCTL_READ_FLICKER	_IOR(MAX86915_IOCTL_MAGIC, 0x01, int *)
#define AWB_MAX86915_CONFIG_TH2		480000
#define AWB_MAX86915_CONFIG_TH3		40000

/* AGC Configuration */
#define AGC_IR			0
#define AGC_RED			1
#define AGC_GREEN		2
#define AGC_BLUE		3
#define AGC_SKIP_CNT	5

#define AGC_LED_SKIP_CNT	16

#ifdef MAXIM_AGC
#define MAX86915_DEFAULT_LED_RGE	0x02 /* 150mA AGCCONF*/

#define MAX86915_MIN_CURRENT	0
#define MAX86915_MAX_CURRENT	((MAX86915_DEFAULT_LED_RGE+1)*51000)
#define MAX86915_CURRENT_FULL_SCALE		\
		(MAX86915_MAX_CURRENT - MAX86915_MIN_CURRENT)

#define MAX86915_CURRENT_PER_STEP	((MAX86915_DEFAULT_LED_RGE+1) * 200)
#else
#define MAX86915_DEFAULT_LED_RGE1	0x02 /* 150mA AGCCONF*/
#define MAX86915_DEFAULT_LED_RGE2	0x02 /* 150mA AGCCONF*/
#define MAX86915_DEFAULT_LED_RGE3	0x02 /* 150mA AGCCONF*/
#define MAX86915_DEFAULT_LED_RGE4	0x02 /* 150mA AGCCONF*/

#define MAX86915_MIN_CURRENT		0
#define MAX86915_MAX_CURRENT(X)		(((X)+1)*51000)
#define MAX86915_CURRENT_FULL_SCALE(X)	\
		(MAX86915_MAX_CURRENT(X) - MAX86915_MIN_CURRENT)

#define MAX86915_CURRENT_PER_STEP(X)	(((X)+1) * 200)
#endif

#define MAX86915_MIN_DIODE_VAL	0
#define MAX86915_MAX_DIODE_VAL	((1 << 19) - 1)

#define MAX86915_AGC_DEFAULT_LED_OUT_RANGE				70
#define MAX86915_AGC_DEFAULT_CORRECTION_COEFF			50
#define MAX86915_AGC_DEFAULT_SENSITIVITY_PERCENT		14
#define MAX86915_AGC_DEFAULT_MIN_NUM_PERCENT			(10)

/* Linear AGC */
#define MAX86915_AGC_ERROR_RANGE1	8
#define MAX86915_AGC_ERROR_RANGE2	28

#define ILLEGAL_OUTPUT_POINTER	-1
#define CONSTRAINT_VIOLATION	-2

/* turnning parameter of dc_step. */
#define EOL_NEW_HRM_ARRY_SIZE			3
#define EOL_NEW_MODE_DC_MID				0
#define EOL_NEW_MODE_DC_HIGH			1
#define EOL_NEW_MODE_DC_XTALK			2

/* turnning parameter of skip count */
#define EOL_NEW_START_SKIP_CNT			134

/* turnning parameter of flicker_step. */
#define EOL_NEW_FLICKER_RETRY_CNT		48
#define EOL_NEW_FLICKER_SKIP_CNT		34
#define EOL_NEW_FLICKER_THRESH			8000

#define EOL_NEW_DC_MODE_SKIP_CNT		134
#define EOL_NEW_DC_FREQ_SKIP_CNT		134

#define EOL_NEW_DC_MIDDLE_SKIP_CNT		134
#define EOL_NEW_DC_HIGH_SKIP_CNT		134
#define EOL_NEW_DC_XTALK_SKIP_CNT		134
#define EOL_NEW_STD_TOTAL_CNT			(116 - 12)

/* turnning parameter of frep_step. */
#define EOL_NEW_FREQ_MIN				385
#define EOL_NEW_FREQ_RETRY_CNT			5
#define EOL_NEW_FREQ_SKEW_RETRY_CNT		5
#define EOL_NEW_FREQ_SKIP_CNT			10

/* total buffer size. */
#define EOL_NEW_HRM_SIZE			400 /* <=this size should be lager than below size. */
#define EOL_NEW_FLICKER_SIZE		256
#define EOL_NEW_DC_MIDDLE_SIZE		256
#define EOL_NEW_DC_HIGH_SIZE		256
#define EOL_NEW_DC_XTALK_SIZE		256
#define EOL_NEW_STD2_SIZE			10

#define EOL_NEW_XTALK_SKIP_CNT		50
#define EOL_NEW_XTALK_SIZE			50

/* the led current of DC step */
#define EOL_NEW_DC_MID_LED_IR_CURRENT		0xA6
#define EOL_NEW_DC_MID_LED_RED_CURRENT		0xA6
#define EOL_NEW_DC_MID_LED_GREEN_CURRENT	0xA6
#define EOL_NEW_DC_MID_LED_BLUE_CURRENT		0xA6
#define EOL_NEW_DC_HIGH_LED_IR_CURRENT		0xFA
#define EOL_NEW_DC_HIGH_LED_RED_CURRENT		0xFA
#define EOL_NEW_DC_HIGH_LED_GREEN_CURRENT	0xE9
#define EOL_NEW_DC_HIGH_LED_BLUE_CURRENT	0xE9

/* the led current of Frequency step */
#define EOL_NEW_FREQUENCY_LED_IR_CURRENT	0xFA
#define EOL_NEW_FREQUENCY_LED_RED_CURRENT	0xFA
#define EOL_NEW_FREQUENCY_LED_GREEN_CURRENT	0xFA
#define EOL_NEW_FREQUENCY_LED_BLUE_CURRENT	0xFA

#define EOL_NEW_HRM_COMPLETE_ALL_STEP		0x07
#define EOL_SUCCESS					0x1
#define EOL_NEW_TYPE				0x1

#define DEFAULT_FIFO_CNT	1
#define CONFIG_4CH_INPUT

/* #define CONFIG_HRM_GREEN_BLUE */

#include "max86915.h"

static int hrm_debug = 1;
static int hrm_info;

module_param(hrm_debug, int, S_IRUGO | S_IWUSR);
module_param(hrm_info, int, S_IRUGO | S_IWUSR);

static struct max86915_device_data *max86915_data;
static u8 agc_debug_enabled = 1;
static u8 fifo_full_cnt = DEFAULT_FIFO_CNT;

/* #define DEBUG_HRMSENSOR */
#ifdef DEBUG_HRMSENSOR
static void max86915_debug_var(struct max86915_device_data *data)
{
	HRM_dbg("===== %s =====\n", __func__);
	HRM_dbg("%s client %p slave_addr 0x%x\n", __func__,
		data->client, data->client->addr);
	HRM_dbg("%s dev %p\n", __func__, data->dev);
	HRM_dbg("%s hrm_input_dev %p\n", __func__, data->hrm_input_dev);
	HRM_dbg("%s hrm_pinctrl %p\n", __func__, data->hrm_pinctrl);
	HRM_dbg("%s pins_sleep %p\n", __func__, data->pins_sleep);
	HRM_dbg("%s pins_idle %p\n", __func__, data->pins_idle);
	HRM_dbg("%s led_3p3 %s\n", __func__, data->led_3p3);
	HRM_dbg("%s vdd_1p8 %s\n", __func__, data->vdd_1p8);
	HRM_dbg("%s i2c_1p8 %s\n", __func__, data->i2c_1p8);
	HRM_dbg("%s hrm_enabled_mode %d\n", __func__, data->enabled_mode);
	HRM_dbg("%s hrm_sampling_rate %d\n", __func__, data->hrm_sampling_rate);
	HRM_dbg("%s regulator_state %d\n", __func__, data->regulator_state);
	HRM_dbg("%s hrm_int %d\n", __func__, data->pin_hrm_int);
	HRM_dbg("%s hrm_en %d\n", __func__, data->pin_hrm_en);
	HRM_dbg("%s hrm_irq %d\n", __func__, data->dev_irq);
	HRM_dbg("%s irq_state %d\n", __func__, data->irq_state);
	HRM_dbg("%s hrm_threshold %d\n", __func__, data->hrm_threshold);
	HRM_dbg("%s eol_test_is_enable %d\n", __func__,
		data->eol_test_is_enable);
	HRM_dbg("%s eol_test_status %d\n", __func__,
		data->eol_test_status);
	HRM_dbg("%s pre_eol_test_is_enable %d\n", __func__,
		data->pre_eol_test_is_enable);
	HRM_dbg("%s lib_ver %s\n", __func__, data->lib_ver);
	HRM_dbg("===== %s =====\n", __func__);
}
#endif

static int max86915_write_reg(struct max86915_device_data *device,
	u8 reg_addr, u8 data)
{
	int err;
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
		HRM_err("%s - write error, pm suspend or reg_state %d\n",
				__func__, device->regulator_state);
		err = -EFAULT;
		return err;
	}
	mutex_lock(&device->suspendlock);

	do {
		err = i2c_transfer(device->client->adapter, msgs, num);
		if (err != num)
			msleep_interruptible(MAX86915_I2C_RETRY_DELAY);
		if (err < 0)
			HRM_err("%s - i2c_transfer error = %d\n", __func__, err);
	} while ((err != num) && (++tries < MAX86915_I2C_MAX_RETRIES));

	mutex_unlock(&device->suspendlock);

	if (err != num) {
		HRM_err("%s -write transfer error:%d\n", __func__, err);
		err = -EIO;
		device->i2c_err_cnt++;
		return err;
	}

	return 0;
}

static int max86915_read_reg(struct max86915_device_data *device,
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
		HRM_err("%s - read error, pm suspend or reg_state %d\n",
				__func__, device->regulator_state);
		err = -EFAULT;
		return err;
	}
	mutex_lock(&device->suspendlock);

	do {
		buffer[0] = reg_addr;
		err = i2c_transfer(device->client->adapter, msgs, num);
		if (err != num)
			msleep_interruptible(MAX86915_I2C_RETRY_DELAY);
		if (err < 0)
			HRM_err("%s - i2c_transfer error = %d\n", __func__, err);
	} while ((err != num) && (++tries < MAX86915_I2C_MAX_RETRIES));

	mutex_unlock(&device->suspendlock);

	if (err != num) {
		HRM_err("%s -read transfer error:%d\n", __func__, err);
		err = -EIO;
		device->i2c_err_cnt++;
	} else
		err = 0;

	return err;
}

static int max86915_print_reg_status(void)
{
	int reg, err;
	u8 recvData;

	for (reg = 0; reg < 0x55; reg++) {
		err = max86915_read_reg(max86915_data, reg, &recvData, 1);
		if (err != 0) {
			HRM_err("%s - error reading 0x%02x err:%d\n",
				__func__, reg, err);
		} else {
			HRM_dbg("%s - 0x%02x = 0x%02x\n",
				__func__, reg, recvData);
		}
	}
	return 0;
}

static int max86915_set_sampling_rate(u32 sampling_period_ns)
{
	int err = 0;
	u8 recvData;

	switch (sampling_period_ns) {
	case 10000000:
		fifo_full_cnt = 1;
		break;
	case 5000000:
		fifo_full_cnt = 2;
		break;
	case 2500000:
		fifo_full_cnt = 4;
		break;
	case 1250000:
		fifo_full_cnt = 8;
		break;
	default:
		HRM_err("%s - %dns is not supported.\n", __func__, sampling_period_ns);
		return -EIO;
	}

	if (max86915_data->enabled_mode == MODE_SDK_IR) {

		err = max86915_write_reg(max86915_data, MAX86915_MODE_CONFIGURATION, 0x00);
		if (err != 0) {
			HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION!\n",
				__func__);
			return -EIO;
		}

		if (fifo_full_cnt == 8) {
			/* 16uA, 800Hz, 70us */
			err = max86915_write_reg(max86915_data, MAX86915_MODE_CONFIGURATION_2,
					0x50);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
					__func__);
				return -EIO;
			}
			/* 1 Samples avg */
			err = max86915_write_reg(max86915_data, MAX86915_FIFO_CONFIG,
					0x1f);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_FIFO_CONFIG!\n",
					__func__);
				return -EIO;
			}
		} else if (fifo_full_cnt == 4) {
			/* 16uA, 400Hz, 120us */
			err = max86915_write_reg(max86915_data, MAX86915_MODE_CONFIGURATION_2,
					0x50);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
					__func__);
				return -EIO;
			}
			/* 2 Samples avg */
			err = max86915_write_reg(max86915_data, MAX86915_FIFO_CONFIG,
					0x3f);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_FIFO_CONFIG!\n",
					__func__);
				return -EIO;
			}
		} else if (fifo_full_cnt == 2) {
			/* 16uA, 200Hz, 120us */
			err = max86915_write_reg(max86915_data, MAX86915_MODE_CONFIGURATION_2,
					0x50);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
					__func__);
				return -EIO;
			}
			/* 4 Samples avg */
			err = max86915_write_reg(max86915_data, MAX86915_FIFO_CONFIG,
					0x5f);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_FIFO_CONFIG!\n",
					__func__);
				return -EIO;
			}
		} else if (fifo_full_cnt == 1) {
			/* 32uA, 400Hz, 120us */
			err = max86915_write_reg(max86915_data, MAX86915_MODE_CONFIGURATION_2,
					0x6D);
			/* 4 Samples avg */
			err = max86915_write_reg(max86915_data, MAX86915_FIFO_CONFIG,
					0x5f);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_FIFO_CONFIG!\n",
					__func__);
				return -EIO;
			}
		}
		/* SDK mode only use FLEX MODE 0x03 After changing FIFO CONFIG skip initial sample. */
		err = max86915_read_reg(max86915_data, MAX86915_FIFO_CONFIG, &recvData, 1);

		if (err != 0) {
			HRM_err("%s - error initializing MAX86915_FIFO_CONFIG!\n",
				__func__);
			return -EIO;
		}
		max86915_data->agc_led_set = AGC_LED_SKIP_CNT >>
			((recvData & MAX86915_SMP_AVE_MASK) >> MAX86915_SMP_AVE_OFFSET);
		err = max86915_write_reg(max86915_data, MAX86915_MODE_CONFIGURATION, 0x03);

		if (err != 0) {
			HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
				__func__);
			return -EIO;
		}
		max86915_data->sample_cnt = 0;
		err = sampling_period_ns;
	}

	return err;
}

static int max86915_get_chipid(u64 *chip_id)
{
	u8 recvData;
	int err;
	u32 c1_code = 0;
	u32 c2_code = 0;
	u32 c3_code = 0;
	u32 c4_code = 0;

	*chip_id = 0;

	err = max86915_write_reg(max86915_data, 0xFF, 0x54);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_TEST0!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(max86915_data, 0xFF, 0x4d);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_TEST1!\n",
			__func__);
		return -EIO;
	}

	err = max86915_read_reg(max86915_data, 0xC1, &recvData, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData);
		return -EIO;
	}
	c1_code = recvData;

	err = max86915_read_reg(max86915_data, 0xC2, &recvData, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData);
		return -EIO;
	}
	c2_code = recvData;

	err = max86915_read_reg(max86915_data, 0xC3, &recvData, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData);
		return -EIO;
	}
	c3_code = recvData;

	err = max86915_read_reg(max86915_data, 0xC4, &recvData, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData);
		return -EIO;
	}
	c4_code = recvData;

	err = max86915_write_reg(max86915_data, 0xFF, 0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86900_MODE_TEST0!\n",
			__func__);
		return -EIO;
	}

	*chip_id = (((u64)c1_code) << 24) + (((u64)c2_code) << 16)
		+ (((u64)c3_code) << 8) + (c4_code);

	HRM_info("%s - Device ID = %lld\n", __func__, *chip_id);

	return 0;
}

static int max86915_get_part_id(struct max86915_device_data *data)
{
	u8 recvData;
	u8 recvDataSub;
	int err;

	err = max86915_write_reg(data, 0xFF, 0x54);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_TEST0!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, 0xFF, 0x4d);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_TEST1!\n",
			__func__);
		return -EIO;
	}

	err = max86915_read_reg(data, 0xC1, &recvData, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData);
		return -EIO;
	}

	err = max86915_write_reg(data, 0xFF, 0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86900_MODE_TEST0!\n",
			__func__);
		return -EIO;
	}

	HRM_dbg("%s - 0xC1 :%x\n", __func__, recvData);

	if (recvData == 0x83)
		return PART_TYPE_MAX86915_ES;
	else if (recvData == 0x86)
		return PART_TYPE_MAX86915_CS_15;
	else if (recvData == 0x88) {
		err = max86915_read_reg(data, 0xFE, &recvDataSub, 1);
		if (err != 0) {
			HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
				__func__, err, recvDataSub);
			return -EIO;
		}

		HRM_dbg("%s - 0xFE :0x%02x\n", __func__, recvDataSub);

		if (recvDataSub == 0x41)
			return PART_TYPE_MAX86915_CS_21;
		else if (recvDataSub == 0xC1)
			return PART_TYPE_MAX86915_CS_22;
		else if (recvDataSub == 0x42)
			return PART_TYPE_MAX86915_CS_211;
		else if (recvDataSub == 0xC2)
			return PART_TYPE_MAX86915_CS_221;

		return PART_TYPE_NONE;
	}

	err = max86915_read_reg(data, 0xFE, &recvDataSub, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvDataSub);
		return -EIO;
	}

	HRM_dbg("%s - 0xFE :0x%02x\n", __func__, recvDataSub);

	if (recvDataSub == 0x41)
		return PART_TYPE_MAX86915_CS_21;
	else if (recvDataSub == 0xC1)
		return PART_TYPE_MAX86915_CS_22;
	else if (recvDataSub == 0x42) {
		if ((recvData & 0x0F) == 0x0B)
			return PART_TYPE_MAX86917_1;
		else
			return PART_TYPE_MAX86915_CS_211;
	} else if (recvDataSub == 0xC2)  {
		if ((recvData & 0x0F) == 0x0B)
			return PART_TYPE_MAX86917_2;
		else
			return PART_TYPE_MAX86915_CS_221;
	}

	return PART_TYPE_NONE;
}

static int max86915_get_part_type(u16 *part_type)
{
	int err;
	u8 buffer[2] = {0, };

	err = max86915_read_reg(max86915_data, MAX86915_REV_ID_REG, buffer, 2);
	if (err) {
		HRM_err("%s - Max86915 WHOAMI read fail0\n", __func__);
		err = -ENODEV;
		goto err_of_read_part_type;
	}

	if (buffer[1] == MAX86915_PART_ID) {
		max86915_data->part_type = max86915_get_part_id(max86915_data);
	} else {
		HRM_err("%s - Max86915 WHOAMI read fail2\n", __func__);
		err = -ENODEV;
		goto err_of_read_part_type;
	}

	*part_type = max86915_data->part_type;

	return err;

err_of_read_part_type:
	return -EINVAL;
}

static int max86915_get_led_test(u8 *result)
{
	int err = 0;
	u8 recvData = 0;
	int i = 0;
	int retry = 0;

	*result = 0;

	err = max86915_write_reg(max86915_data, MAX86915_MODE_CONFIGURATION,
		0x1 << MAX86915_RESET_OFFSET);
	if (err != 0) {
		HRM_err("%s - error writing MAX86915_MODE_CONFIGURATION err:%d\n",
			__func__, err);
		return -EIO;
	}

	usleep_range(100000, 110000);

	err = max86915_write_reg(max86915_data, MAX86915_LED1_PA, 0xFF);
	if (err != 0) {
		HRM_err("%s - error writing MAX86915_LED1_PA err:%d\n",
			__func__, err);
		return -EIO;
	}

	err = max86915_write_reg(max86915_data, MAX86915_LED2_PA, 0xFF);
	if (err != 0) {
		HRM_err("%s - error writing MAX86915_LED2_PA err:%d\n",
			__func__, err);
		return -EIO;
	}

	err = max86915_write_reg(max86915_data, MAX86915_LED3_PA, 0x08);
	if (err != 0) {
		HRM_err("%s - error writing MAX86915_LED3_PA err:%d\n",
			__func__, err);
		return -EIO;
	}

	err = max86915_write_reg(max86915_data, MAX86915_LED4_PA, 0x08);
	if (err != 0) {
		HRM_err("%s - error writing MAX86915_LED4_PA err:%d\n",
			__func__, err);
		return -EIO;
	}

	err = max86915_write_reg(max86915_data, MAX86915_MODE_CONFIGURATION, 0x01);
	if (err != 0) {
		HRM_err("%s - error writing MAX86915_MODE_CONFIGURATION err:%d\n",
			__func__, err);
		return -EIO;
	}

	usleep_range(1200, 1210);

	for (retry = 0 ; retry < 3; retry++) {
		*result = 0;

		for (i = 0; i < MAX_LED_NUM; i++) {
			err = max86915_write_reg(max86915_data, 0x31, 0x10 << i);
			if (err != 0) {
				HRM_err("%s - error writing MAX86915_LED_TEST_EN%d err:%d\n",
					__func__, i, err);
				return -EIO;
			}

			usleep_range(100, 110);

			err = max86915_write_reg(max86915_data, 0x31, (0x10 << i) | 0x01);
			if (err != 0) {
				HRM_err("%s - error writing MAX86915_LED_TEST_EN%d err:%d\n",
					__func__, i, err);
				return -EIO;
			}

			usleep_range(1000, 1100);

			err = max86915_read_reg(max86915_data, 0x32, &recvData, 1);
			if (err != 0) {
				HRM_err("%s - error reading MAX86915_COMP_OUT%d err:%d\n",
					__func__, i, err);
				return -EIO;
			}

			*result |= (recvData & (1 << i));
		}

		if (*result == 0)
			break;

		HRM_err("%s - fail retry %d = 0x%02x\n", __func__, retry, *result);
	}

	err = max86915_write_reg(max86915_data, MAX86915_MODE_CONFIGURATION,
		0x1 << MAX86915_SHDN_OFFSET);
	if (err != 0) {
		HRM_err("%s - error writing MAX86915_MODE_CONFIGURATION err:%d\n",
			__func__, err);
		return -EIO;
	}

	return 0;
}

static int max86915_set_current(u8 d1, u8 d2, u8 d3, u8 d4)
{
	int err = 0;
	u8 recvData;

	max86915_data->led_current1 = d1; /* set ir; */
	max86915_data->led_current2 = d2; /* set red; */
	max86915_data->led_current3 = d3; /* set ir; */
	max86915_data->led_current4 = d4; /* set red; */
	HRM_info("%s - 0x%x 0x%x 0x%x 0x%x\n", __func__,
			max86915_data->led_current1, max86915_data->led_current2,
			max86915_data->led_current3, max86915_data->led_current4);

	if (max86915_data->part_type < PART_TYPE_MAX86915_CS_211) {
		err = max86915_write_reg(max86915_data, MAX86915_MODE_CONFIGURATION, 0x00);

		if (err != 0) {
			HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION!\n",
				__func__);
			return -EIO;
		}
	}

	err = max86915_write_reg(max86915_data, MAX86915_LED1_PA,
			max86915_data->led_current1);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED1_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(max86915_data, MAX86915_LED2_PA,
			max86915_data->led_current2);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED2_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(max86915_data, MAX86915_LED3_PA,
			max86915_data->led_current3);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED3_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(max86915_data, MAX86915_LED4_PA,
			max86915_data->led_current4);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED4_PA!\n",
			__func__);
		return -EIO;
	}

	if (max86915_data->part_type < PART_TYPE_MAX86915_CS_211) {
		err = max86915_read_reg(max86915_data, MAX86915_FIFO_CONFIG, &recvData, 1);
		max86915_data->agc_led_set = AGC_LED_SKIP_CNT >>
			((recvData & MAX86915_SMP_AVE_MASK) >> MAX86915_SMP_AVE_OFFSET);
		err = max86915_write_reg(max86915_data, MAX86915_MODE_CONFIGURATION, 0x03);
	}

	return err;
}

static int max86915_set_xtalk(u8 d1, u8 d2, u8 d3, u8 d4)
{
	int err = 0;

	max86915_data->xtalk_code1 = d1; /* set ir; */
	max86915_data->xtalk_code2 = d2; /* set red; */
	max86915_data->xtalk_code3 = d3; /* set ir; */
	max86915_data->xtalk_code4 = d4; /* set red; */
	HRM_info("%s - 0x%x 0x%x 0x%x 0x%x\n", __func__,
			max86915_data->xtalk_code1, max86915_data->xtalk_code2,
			max86915_data->xtalk_code3, max86915_data->xtalk_code4);

	if (max86915_data->led_current1 == MAX86915_MIN_CURRENT)
		max86915_data->default_current1 = max86915_data->init_current[AGC_IR]
					+ MAX86915_IR_CURRENT_STEP * max86915_data->xtalk_code1;

	if (max86915_data->led_current2 == MAX86915_MIN_CURRENT)
		max86915_data->default_current2 = max86915_data->init_current[AGC_RED]
					+ MAX86915_RED_CURRENT_STEP * max86915_data->xtalk_code2;

	if (max86915_data->led_current3 == MAX86915_MIN_CURRENT)
		max86915_data->default_current3 = max86915_data->init_current[AGC_GREEN]
					+ MAX86915_GREEN_CURRENT_STEP * max86915_data->xtalk_code3;

	if (max86915_data->led_current4 == MAX86915_MIN_CURRENT)
		max86915_data->default_current4 = max86915_data->init_current[AGC_BLUE]
					+ MAX86915_BLUE_CURRENT_STEP * max86915_data->xtalk_code4;

	HRM_info("%s - current 0x%x 0x%x 0x%x 0x%x\n", __func__,
			max86915_data->default_current1, max86915_data->default_current2,
			max86915_data->default_current3, max86915_data->default_current4);

	if (max86915_data->enabled_mode) {
		err = max86915_write_reg(max86915_data, MAX86915_DAC1_XTALK_CODE,
				max86915_data->xtalk_code1);
		if (err != 0) {
			HRM_err("%s - error initializing MAX86915_DAC1_XTALK_CODE!\n",
				__func__);
			return -EIO;
		}
		err = max86915_write_reg(max86915_data, MAX86915_DAC2_XTALK_CODE,
				max86915_data->xtalk_code2);
		if (err != 0) {
			HRM_err("%s - error initializing MAX86915_DAC2_XTALK_CODE!\n",
				__func__);
			return -EIO;
		}
		err = max86915_write_reg(max86915_data, MAX86915_DAC3_XTALK_CODE,
				max86915_data->xtalk_code3);
		if (err != 0) {
			HRM_err("%s - error initializing MAX86915_DAC3_XTALK_CODE!\n",
				__func__);
			return -EIO;
		}
		err = max86915_write_reg(max86915_data, MAX86915_DAC4_XTALK_CODE,
				max86915_data->xtalk_code4);
		if (err != 0) {
			HRM_err("%s - error initializing MAX86915_DAC4_XTALK_CODE!\n",
				__func__);
			return -EIO;
		}
	}

	return err;
}

static int max86915_update_led_current(struct max86915_device_data *data,
	int new_led_uA,
	int led_num)
{
	int err;
	u8 led_reg_val;
	int led_reg_addr;
	u8 recvData;

	if (new_led_uA > MAX86915_MAX_CURRENT(data->led_range[led_num])) {
		HRM_dbg("%s - Tried to set LED%d to %duA. Max is %duA\n",
				__func__, led_num, new_led_uA,
				MAX86915_MAX_CURRENT(data->led_range[led_num]));
		new_led_uA = MAX86915_MAX_CURRENT(data->led_range[led_num]);
	} else if (new_led_uA < MAX86915_MIN_CURRENT) {
		HRM_dbg("%s - Tried to set LED%d to %duA. Min is %duA\n",
				__func__, led_num, new_led_uA, MAX86915_MIN_CURRENT);
		new_led_uA = MAX86915_MIN_CURRENT;
	}

	led_reg_val = new_led_uA / MAX86915_CURRENT_PER_STEP(data->led_range[led_num]);

	led_reg_addr = MAX86915_LED1_PA + led_num;

	HRM_dbg("%s - Setting ADD 0x%x, LED%d to 0x%.2X (%duA)\n",
			__func__, led_reg_addr, led_num, led_reg_val, new_led_uA);

	if (data->part_type < PART_TYPE_MAX86915_CS_211) {
		err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION, 0x00);
		if (err != 0) {
			HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION!\n", __func__);
			return -EIO;
		}
	}

	err = max86915_write_reg(data, led_reg_addr, led_reg_val);

	if (data->part_type < PART_TYPE_MAX86915_CS_211) {
		err = max86915_read_reg(data, MAX86915_FIFO_CONFIG, &recvData, 1);
		data->agc_led_set = AGC_LED_SKIP_CNT >> ((recvData &
			MAX86915_SMP_AVE_MASK) >> MAX86915_SMP_AVE_OFFSET);

		err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION, 0x03);
	}

	if (err != 0) {
		HRM_err("%s - error initializing register 0x%.2X!\n",
			__func__, led_reg_addr);
		return -EIO;
	}
	switch (led_num) {
	case AGC_IR:
		data->led_current1 = led_reg_val; /* set ir */
		break;
	case AGC_RED:
		data->led_current2 = led_reg_val; /* set red */
		break;
	case AGC_GREEN:
		data->led_current3 = led_reg_val; /* set green */
		break;
	case AGC_BLUE:
		data->led_current4 = led_reg_val; /* set blue */
		break;
	}

	return 0;
}

static int max86915_init_status(struct max86915_device_data *data)
{
	int err = 0;
	u8 recvData;

	data->agc_enabled = 0;

	err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION, MAX86915_RESET_MASK);
	if (err != 0) {
		HRM_err("%s - error sw shutdown device!\n", __func__);
		return -EIO;
	}

	/* Interrupt Clear */
	err = max86915_read_reg(data, MAX86915_INTERRUPT_STATUS, &recvData, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData);
		return -EIO;
	}

	/* Interrupt2 Clear */
	err = max86915_read_reg(data, MAX86915_INTERRUPT_STATUS_2, &recvData, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData);
		return -EIO;
	}

	HRM_info("%s - init done, part_type = %u\n", __func__, data->part_type);

	return err;
}

static void max86915_div_str(char *left_integer, char *left_decimal, char *right_integer,
				char *right_decimal, char *result_integer, char *result_decimal)
{
	int i;
	unsigned long long j;
	unsigned long long loperand, roperand;
	unsigned long long ldigit, rdigit, temp;
	char str[10] = { 0, };

	ldigit = 0;
	rdigit = 0;

	sscanf(left_integer, "%19llu", &loperand);
	for (i = DECIMAL_LEN - 2; i >= 0; i--) {
		if (left_decimal[i] != '0') {
			ldigit = (unsigned long long)i + 1;
			break;
		}
	}

	sscanf(right_integer, "%19llu", &roperand);
	for (i = DECIMAL_LEN - 2; i >= 0; i--) {
		if (right_decimal[i] != '0') {
			rdigit = (unsigned long long)i + 1;
			break;
		}
	}

	if (ldigit < rdigit)
		ldigit = rdigit;

	for (j = 0; j < ldigit; j++) {
		loperand *= 10;
		roperand *= 10;
	}
	if (ldigit != 0) {
		snprintf(str, sizeof(str), "%%%llullu", ldigit);
		sscanf(left_decimal, str, &temp);
		loperand += temp;

		snprintf(str, sizeof(str), "%%%llullu", ldigit);
		sscanf(right_decimal, str, &temp);
		roperand += temp;
	}
	if (roperand == 0)
		roperand = 1;
	temp = loperand / roperand;
	snprintf(result_integer, INTEGER_LEN, "%llu", temp);
	loperand -= roperand * temp;
	loperand *= 10;
	for (i = 0; i < DECIMAL_LEN; i++) {
		temp = loperand / roperand;
		if (temp != 0) {
			loperand -= roperand * temp;
			loperand *= 10;
		} else
			loperand *= 10;
		result_decimal[i] = '0' + temp;
	}
	result_decimal[DECIMAL_LEN - 1] = 0;
}

static void max86915_div_float(struct max86915_div_data *div_data, u64 left_operand, u64 right_operand)
{
	snprintf(div_data->left_integer, INTEGER_LEN, "%llu", left_operand);
	memset(div_data->left_decimal, '0', sizeof(div_data->left_decimal));
	div_data->left_decimal[DECIMAL_LEN - 1] = 0;

	snprintf(div_data->right_integer, INTEGER_LEN, "%llu", right_operand);
	memset(div_data->right_decimal, '0', sizeof(div_data->right_decimal));
	div_data->right_decimal[DECIMAL_LEN - 1] = 0;

	max86915_div_str(div_data->left_integer, div_data->left_decimal, div_data->right_integer,
			div_data->right_decimal, div_data->result_integer, div_data->result_decimal);
}

static void max86915_plus_str(char *integer_operand,
	char *decimal_operand, char *result_integer, char *result_decimal)
{
	unsigned long long a, b;
	int i;

	for (i = 0; i < DECIMAL_LEN - 1; i++) {
		decimal_operand[i] -= '0';
		result_decimal[i] -= '0';
	}

	for (i = 0; i < DECIMAL_LEN - 1; i++)
		result_decimal[i] += decimal_operand[i];

	sscanf(integer_operand, "%19llu", &a);
	sscanf(result_integer, "%19llu", &b);

	for (i = DECIMAL_LEN - 1; i >= 0; i--) {
		if (result_decimal[i] >= 10) {
			if (i > 0)
				result_decimal[i - 1] += 1;
			else
				a += 1;
			result_decimal[i] -= 10;
		}
	}

	a += b;

	for (i = 0; i < DECIMAL_LEN - 1; i++)
		result_decimal[i] += '0';

	snprintf(result_integer, INTEGER_LEN, "%llu", a);
}

static void max86915_float_sqrt(char *integer_operand, char *decimal_operand)
{
	int i;
	char left_integer[INTEGER_LEN] = { '\0', }, left_decimal[DECIMAL_LEN] = { '\0', };
	char result_integer[INTEGER_LEN] = { '\0', }, result_decimal[DECIMAL_LEN] = { '\0', };
	char temp_integer[INTEGER_LEN] = { '\0', }, temp_decimal[DECIMAL_LEN] = { '\0', };

	strncpy(left_integer, integer_operand, INTEGER_LEN - 1);
	strncpy(left_decimal, decimal_operand, DECIMAL_LEN - 1);

	for (i = 0; i < 100; i++) {
		max86915_div_str(left_integer, left_decimal, integer_operand,
			decimal_operand, result_integer, result_decimal);
		max86915_plus_str(integer_operand, decimal_operand, result_integer, result_decimal);

		snprintf(temp_integer, INTEGER_LEN, "%d", 2);
		memset(temp_decimal, '0', sizeof(temp_decimal));
		temp_decimal[DECIMAL_LEN - 1] = 0;

		max86915_div_str(result_integer, result_decimal, temp_integer,
			temp_decimal, integer_operand, decimal_operand);
	}
}

#ifdef MAXIM_AGC
static int agc_adj_calculator(struct max86915_device_data *data,
			s32 *change_by_percent_of_range,
			s32 *change_by_percent_of_current_setting,
			s32 *change_led_by_absolute_count,
			s32 *set_led_to_absolute_count,
			s32 target_percent_of_range,
			s32 correction_coefficient,
			s32 allowed_error_in_percentage,
			s32 *reached_thresh,
			s32 current_average,
			s32 number_of_samples_averaged,
			s32 led_drive_current_value)
{
	s32 diode_min_val;
	s32 diode_max_val;
	s32 diode_min_current;
	s32 diode_fs_current;

	s32 current_percent_of_range = 0;
	s32 delta = 0;
	s32 desired_delta = 0;
	s32 current_power_percent = 0;

	if (change_by_percent_of_range == 0
			 || change_by_percent_of_current_setting == 0
			 || change_led_by_absolute_count == 0
			 || set_led_to_absolute_count == 0)
		return ILLEGAL_OUTPUT_POINTER;

	if (target_percent_of_range > 90 || target_percent_of_range < 10)
		return CONSTRAINT_VIOLATION;

	if (correction_coefficient > 100 || correction_coefficient < 0)
		return CONSTRAINT_VIOLATION;

	if (allowed_error_in_percentage > 100
			|| allowed_error_in_percentage < 0)
		return CONSTRAINT_VIOLATION;

#if ((MAX86915_MAX_DIODE_VAL-MAX86915_MIN_DIODE_VAL) <= 0 \
			 || (MAX86915_MAX_DIODE_VAL < 0) || (MAX86915_MIN_DIODE_VAL < 0))
	#error "Illegal max86915 diode Min/Max Pair"
#endif

#if ((MAX86915_CURRENT_FULL_SCALE) <= 0 \
		|| (MAX86915_MAX_CURRENT < 0) || (MAX86915_MIN_CURRENT < 0))
	#error "Illegal max86915 LED Min/Max current Pair"
#endif

	diode_min_val = MAX86915_MIN_DIODE_VAL;
	diode_max_val = MAX86915_MAX_DIODE_VAL;
	diode_min_current = MAX86915_MIN_CURRENT;
	diode_fs_current = MAX86915_CURRENT_FULL_SCALE;
	if (led_drive_current_value > MAX86915_MAX_CURRENT
			|| led_drive_current_value < MAX86915_MIN_CURRENT)
		return CONSTRAINT_VIOLATION;

	if (current_average < MAX86915_MIN_DIODE_VAL
				|| current_average > MAX86915_MAX_DIODE_VAL)
		return CONSTRAINT_VIOLATION;

	current_percent_of_range = 100 *
		(current_average - diode_min_val) /
		(diode_max_val - diode_min_val);

	delta = current_percent_of_range - target_percent_of_range;
	delta = delta * correction_coefficient / 100;

	if (!(*reached_thresh))
		allowed_error_in_percentage =
			allowed_error_in_percentage * 2 / 7;

	if (delta > -allowed_error_in_percentage
			&& delta < allowed_error_in_percentage) {
		*change_by_percent_of_range = 0;
		*change_by_percent_of_current_setting = 0;
		*change_led_by_absolute_count = 0;
		*set_led_to_absolute_count = led_drive_current_value;
		*reached_thresh = 1;
		return 0;
	}

	current_power_percent = 100 *
			(led_drive_current_value - diode_min_current) /
			diode_fs_current;

	if (delta < 0) {
		if (current_power_percent > 97
			&& (current_percent_of_range
				< (data->threshold_default * 100) / (diode_max_val - diode_min_val))) {
			desired_delta = -delta * (102 - current_power_percent) /
				(100 - current_percent_of_range);
		} else
			desired_delta = -delta * (100 - current_power_percent) /
				(100 - current_percent_of_range);
	}

	if (delta > 0)
		desired_delta = -delta * (current_power_percent)
				/ (current_percent_of_range);

	*change_by_percent_of_range = desired_delta;

	*change_led_by_absolute_count = (desired_delta
			* diode_fs_current / 100);
	*change_by_percent_of_current_setting =
			(*change_led_by_absolute_count * 100)
			/ (led_drive_current_value);
	*set_led_to_absolute_count = led_drive_current_value
			+ *change_led_by_absolute_count;

	if (*set_led_to_absolute_count > MAX86915_MAX_CURRENT)
		*set_led_to_absolute_count = MAX86915_MAX_CURRENT;

	return 0;
}
#else
static int linear_search_agc(struct max86915_device_data *data, int ppg_average, int led_num)
{

	int xtalk_adc = 0;
	int target_adc = MAX86915_MAX_DIODE_VAL * data->agc_led_out_percent / 100;
	long long target_current;
	int agc_range = MAX86915_AGC_ERROR_RANGE1;

	if (ppg_average < MAX86915_MIN_DIODE_VAL
			|| ppg_average > MAX86915_MAX_DIODE_VAL)
		return CONSTRAINT_VIOLATION;

	HRM_info("%s - (%d) ppg_average : %d\n", __func__, led_num, ppg_average);

	if (ppg_average == MAX86915_MAX_DIODE_VAL) {
		if (data->prev_agc_current[led_num] >= data->agc_current[led_num])
			target_current = data->agc_current[led_num] / 2;
		else
			target_current = (data->agc_current[led_num] + data->prev_agc_current[led_num]) / 2;

		if (target_current > MAX86915_MAX_CURRENT(data->led_range[led_num]))
			target_current = MAX86915_MAX_CURRENT(data->led_range[led_num]);
		else if (target_current < MAX86915_MIN_CURRENT)
			target_current = MAX86915_MIN_CURRENT;

		data->prev_agc_current[led_num] = data->agc_current[led_num];
		data->change_led_by_absolute_count[led_num] = target_current - data->agc_current[led_num];
		data->agc_current[led_num] = target_current;
		return 0;
	} else if (ppg_average == MAX86915_MIN_DIODE_VAL) {
		if (data->prev_agc_current[led_num] < data->agc_current[led_num])
			data->prev_agc_current[led_num] = MAX86915_MAX_CURRENT(data->led_range[led_num]);
		target_current = (data->agc_current[led_num]
			+ data->prev_agc_current[led_num]
			+ MAX86915_CURRENT_PER_STEP(data->led_range[led_num])) / 2;

		if (target_current > MAX86915_MAX_CURRENT(data->led_range[led_num]))
			target_current = MAX86915_MAX_CURRENT(data->led_range[led_num]);
		else if (target_current < MAX86915_MIN_CURRENT)
			target_current = MAX86915_MIN_CURRENT;

		data->prev_agc_current[led_num] = data->agc_current[led_num];
		data->change_led_by_absolute_count[led_num] = target_current - data->agc_current[led_num];
		data->agc_current[led_num] = target_current;
		return 0;
	}

	if (data->reached_thresh[led_num]) {
		agc_range = MAX86915_AGC_ERROR_RANGE2;
		if (ppg_average <= (data->prev_agc_average[led_num] * 4 / 10)) {
			data->remove_cnt[led_num]++;
			if (data->remove_cnt[led_num] > 10) {
				data->remove_cnt[led_num] = 0;
				data->reached_thresh[led_num] = 0;
			}
		} else
			data->remove_cnt[led_num] = 0;
	}

	HRM_info("%s, (%d) target : %d, ppg_average : %d prev_average : %d reach_thd : %d\n",
		__func__, led_num, target_adc, ppg_average,
		data->prev_agc_average[led_num], data->reached_thresh[led_num]);

	if (ppg_average < MAX86915_MAX_DIODE_VAL * (data->agc_led_out_percent + agc_range) / 100
		&& ppg_average > MAX86915_MAX_DIODE_VAL * (data->agc_led_out_percent - agc_range) / 100) {
		data->reached_thresh[led_num] = 1;
		data->change_led_by_absolute_count[led_num] = 0;
		data->prev_agc_average[led_num] = ppg_average;
		return 0;
	}

	switch (led_num) {
	case AGC_IR:
		xtalk_adc = MAX86915_MAX_DIODE_VAL / 2 * data->xtalk_code1 / 31;
		target_adc += xtalk_adc;
		break;
	case AGC_RED:
		xtalk_adc = MAX86915_MAX_DIODE_VAL / 2 * data->xtalk_code2 / 31;
		target_adc += xtalk_adc;
		break;
	case AGC_GREEN:
		xtalk_adc = MAX86915_MAX_DIODE_VAL / 2 * data->xtalk_code3 / 31;
		target_adc += xtalk_adc;
		break;
	case AGC_BLUE:
		xtalk_adc = MAX86915_MAX_DIODE_VAL / 2 * data->xtalk_code4 / 31;
		target_adc += xtalk_adc;
		break;
	default:
		return -EIO;
	}

	HRM_info("%s - (%d) target2 : %d\n", __func__, led_num, target_adc);

	target_current = target_adc - (ppg_average + xtalk_adc);

	if (data->agc_current[led_num] > data->prev_current[led_num])
		target_current *= (data->agc_current[led_num] - data->prev_current[led_num]);
	else if (data->agc_current[led_num] < data->prev_current[led_num])
		target_current *= (data->prev_current[led_num] - data->agc_current[led_num]);

	if (ppg_average + xtalk_adc > data->prev_ppg[led_num])
		target_current /= (ppg_average + xtalk_adc - data->prev_ppg[led_num]);
	else if (ppg_average + xtalk_adc < data->prev_ppg[led_num])
		target_current /= (data->prev_ppg[led_num] - (ppg_average + xtalk_adc));

	target_current += data->agc_current[led_num];

	if (target_current > MAX86915_MAX_CURRENT(data->led_range[led_num]))
		target_current = MAX86915_MAX_CURRENT(data->led_range[led_num]);
	else if (target_current < MAX86915_MIN_CURRENT)
		target_current = MAX86915_MIN_CURRENT;

	data->change_led_by_absolute_count[led_num] = target_current - data->agc_current[led_num];

	data->prev_current[led_num] = data->agc_current[led_num];
	data->agc_current[led_num] = target_current;
	data->prev_ppg[led_num] = ppg_average + xtalk_adc;

	HRM_info("%s - data->agc_current[%d] : %u, %lld\n",
		__func__, led_num, data->agc_current[led_num], target_current);

	return 0;
}
#endif

static int max86915_reset_sdk_agc_var(int channel)
{
	HRM_dbg("%s - channel %d reset\n", __func__, channel);

	/* LED PA to 0 */
	switch (channel) {
	case 0:
		max86915_data->led_current1 = 0x0;
		max86915_data->reached_thresh[AGC_IR] = 0;
		max86915_data->agc_sum[AGC_IR] = 0;
		max86915_data->agc_sample_cnt[AGC_IR] = 0;
		max86915_data->agc_current[AGC_IR] = (max86915_data->led_current1
			* MAX86915_CURRENT_PER_STEP(max86915_data->led_range[channel]));
		break;
	case 1:
		max86915_data->led_current2 = 0x0;
		max86915_data->reached_thresh[AGC_RED] = 0;
		max86915_data->agc_sum[AGC_RED] = 0;
		max86915_data->agc_sample_cnt[AGC_RED] = 0;
		max86915_data->agc_current[AGC_RED] = (max86915_data->led_current2
			* MAX86915_CURRENT_PER_STEP(max86915_data->led_range[channel]));
		break;
	case 2:
		max86915_data->led_current3 = 0x0;
		max86915_data->reached_thresh[AGC_GREEN] = 0;
		max86915_data->agc_sum[AGC_GREEN] = 0;
		max86915_data->agc_sample_cnt[AGC_GREEN] = 0;
		max86915_data->agc_current[AGC_GREEN] = (max86915_data->led_current3
			* MAX86915_CURRENT_PER_STEP(max86915_data->led_range[channel]));
		break;
	case 3:
		max86915_data->led_current4 = 0x0;
		max86915_data->reached_thresh[AGC_BLUE] = 0;
		max86915_data->agc_sum[AGC_BLUE] = 0;
		max86915_data->agc_sample_cnt[AGC_BLUE] = 0;
		max86915_data->agc_current[AGC_BLUE] = (max86915_data->led_current4
			* MAX86915_CURRENT_PER_STEP(max86915_data->led_range[channel]));
		break;
		}
	max86915_set_current(max86915_data->led_current1,
		max86915_data->led_current2,
		max86915_data->led_current3,
		max86915_data->led_current4);
	return 0;
}

static int max86915_hrm_agc(struct max86915_device_data *data, int counts, int led_num)
{
	int err = 0;
	int avg = 0;

	/* HRM_info("%s - led_num(%d) = %d, %d\n", __func__, led_num, counts, data->agc_sample_cnt[led_num]); */

	if (led_num < AGC_IR || led_num > AGC_BLUE)
		return -EIO;

	data->agc_sum[led_num] += counts;
	data->agc_sample_cnt[led_num]++;

	if (data->agc_sample_cnt[led_num] < data->agc_min_num_samples)
		return 0;

	data->agc_sample_cnt[led_num] = 0;

	avg = data->agc_sum[led_num] / data->agc_min_num_samples;
	data->agc_sum[led_num] = 0;
	/* HRM_info("%s - led_num(%d) = %d, %d\n", __func__, led_num, counts, avg); */

#ifdef MAXIM_AGC
	err = agc_adj_calculator(data,
			&data->change_by_percent_of_range[led_num],
			&data->change_by_percent_of_current_setting[led_num],
			&data->change_led_by_absolute_count[led_num],
			&data->agc_current[led_num],
			data->agc_led_out_percent,
			data->agc_corr_coeff,
			data->agc_sensitivity_percent,
			&data->reached_thresh[led_num],
			avg,
			data->agc_min_num_samples,
			data->agc_current[led_num]);
#else
	err = linear_search_agc(data, avg, led_num);
#endif

	if (err)
		return err;


	/* HRM_info("%s - %d-a:%d\n", __func__, led_num, data->change_led_by_absolute_count[led_num]); */

	if (data->change_led_by_absolute_count[led_num] == 0) {

		if (led_num == AGC_IR) {
			data->ir_adc = counts;
			data->ir_curr = data->led_current1;
		} else if (led_num == AGC_RED) {
			data->red_adc = counts;
			data->red_curr = data->led_current2;
		} else if (led_num == AGC_GREEN) {
			data->green_adc = counts;
			data->green_curr = data->led_current3;
		} else if (led_num == AGC_BLUE) {
			data->blue_adc = counts;
			data->blue_curr = data->led_current4;
		}


		/*
		 * HRM_info("%s - ir_curr = 0x%2x, red_curr = 0x%2x, ir_adc = %d, red_adc = %d\n",
		 *	__func__, data->ir_curr, data->red_curr, data->ir_adc, data->red_adc);
		 */

		return 0;
	}

	err = data->update_led(data, data->agc_current[led_num], led_num);

	/* skip 2 sample after changing current */
	max86915_data->agc_enabled = 0;
	max86915_data->sample_cnt = AGC_SKIP_CNT - 2;

	if (err)
		HRM_err("%s - failed\n", __func__);

	return err;
}

static int max86915_cal_agc(int ir, int red, int green, int blue)
{
	int err = 0;
	int led_num0, led_num1, led_num2, led_num3;

	led_num0 = AGC_IR;
	led_num1 = AGC_RED;
	led_num2 = AGC_GREEN;
	led_num3 = AGC_BLUE;

	if (max86915_data->agc_mode == M_HRM) {
		if (ir >= max86915_data->threshold_default) { /* Detect */
			err |= max86915_hrm_agc(max86915_data, ir, led_num0); /* IR */
			if (max86915_data->agc_current[led_num1] != MAX86915_MIN_CURRENT) {
				err |= max86915_hrm_agc(max86915_data, red, led_num1); /* RED */
			} else {
				/* init */
				max86915_data->agc_current[led_num1]
					=  max86915_data->init_current[led_num1]
					* MAX86915_CURRENT_PER_STEP(max86915_data->led_range[led_num1]);
				max86915_data->agc_enabled = 0;
				max86915_data->sample_cnt = 0;
				max86915_data->reached_thresh[led_num1] = 0;
				max86915_data->prev_current[led_num1] = 0;
				max86915_data->prev_agc_current[led_num1]
					= MAX86915_MAX_CURRENT(max86915_data->led_range[led_num1]);
				max86915_data->prev_ppg[led_num1] = 0;
				max86915_data->agc_sum[led_num1] = 0;
				max86915_data->agc_sample_cnt[led_num1] = 0;
				max86915_data->update_led(max86915_data,
						max86915_data->agc_current[led_num1], led_num1);
			}
#ifdef CONFIG_HRM_GREEN_BLUE
			if (max86915_data->agc_current[led_num2] != MAX86915_MIN_CURRENT) {
				err |= max86915_hrm_agc(max86915_data, green, led_num2); /* GREEN */
			} else {
				/* init */
				max86915_data->agc_current[led_num2]
					=  max86915_data->init_current[led_num2]
					* MAX86915_CURRENT_PER_STEP(max86915_data->led_range[led_num2]);
				max86915_data->agc_enabled = 0;
				max86915_data->sample_cnt = 0;
				max86915_data->reached_thresh[led_num2] = 0;
				max86915_data->prev_current[led_num2] = 0;
				max86915_data->prev_agc_current[led_num2]
					= MAX86915_MAX_CURRENT(max86915_data->led_range[led_num2]);
				max86915_data->prev_ppg[led_num2] = 0;
				max86915_data->agc_sum[led_num2] = 0;
				max86915_data->agc_sample_cnt[led_num2] = 0;
				max86915_data->update_led(max86915_data,
						max86915_data->agc_current[led_num2], led_num2);
			}

			if (max86915_data->agc_current[led_num3] != MAX86915_MIN_CURRENT) {
				err |= max86915_hrm_agc(max86915_data, blue, led_num3); /* BLUE */
			} else {
				/* init */
				max86915_data->agc_current[led_num3]
					=  max86915_data->init_current[led_num3]
					* MAX86915_CURRENT_PER_STEP(max86915_data->led_range[led_num3]);
				max86915_data->agc_enabled = 0;
				max86915_data->sample_cnt = 0;
				max86915_data->reached_thresh[led_num3] = 0;
				max86915_data->prev_current[led_num3] = 0;
				max86915_data->prev_agc_current[led_num3]
					= MAX86915_MAX_CURRENT(max86915_data->led_range[led_num3]);
				max86915_data->prev_ppg[led_num3] = 0;
				max86915_data->agc_sum[led_num3] = 0;
				max86915_data->agc_sample_cnt[led_num3] = 0;
				max86915_data->update_led(max86915_data,
						max86915_data->agc_current[led_num3], led_num3);
			}
#endif
		} else {
			if (max86915_data->agc_current[led_num1] != MAX86915_MIN_CURRENT) { /* Disable led_num1 */
				/* init */
				max86915_data->agc_current[led_num1] = MAX86915_MIN_CURRENT;
				max86915_data->reached_thresh[led_num1] = 0;
				max86915_data->prev_current[led_num1] = 0;
				max86915_data->prev_agc_current[led_num1]
					= MAX86915_MAX_CURRENT(max86915_data->led_range[led_num1]);
				max86915_data->prev_ppg[led_num1] = 0;
				max86915_data->agc_sum[led_num1] = 0;
				max86915_data->agc_sample_cnt[led_num1] = 0;
				max86915_data->update_led(max86915_data,
						max86915_data->agc_current[led_num1], led_num1);

				/* init */
				max86915_data->agc_current[led_num0]
					=  max86915_data->init_current[led_num0]
					* MAX86915_CURRENT_PER_STEP(max86915_data->led_range[led_num0]);
				max86915_data->agc_enabled = 0;
				max86915_data->sample_cnt = 0;
				max86915_data->reached_thresh[led_num0] = 0;
				max86915_data->prev_current[led_num0] = 0;
				max86915_data->prev_agc_current[led_num0]
					= MAX86915_MAX_CURRENT(max86915_data->led_range[led_num0]);
				max86915_data->prev_ppg[led_num0] = 0;
				max86915_data->agc_sum[led_num0] = 0;
				max86915_data->agc_sample_cnt[led_num0] = 0;
				max86915_data->update_led(max86915_data,
						max86915_data->agc_current[led_num0], led_num0);
			}
#ifdef CONFIG_HRM_GREEN_BLUE
			if (max86915_data->agc_current[led_num2] != MAX86915_MIN_CURRENT) { /* Disable led_num2 */
				/* init */
				max86915_data->agc_current[led_num2] = MAX86915_MIN_CURRENT;
				max86915_data->reached_thresh[led_num2] = 0;
				max86915_data->prev_current[led_num2] = 0;
				max86915_data->prev_agc_current[led_num2]
					= MAX86915_MAX_CURRENT(max86915_data->led_range[led_num2]);
				max86915_data->prev_ppg[led_num2] = 0;
				max86915_data->agc_sum[led_num2] = 0;
				max86915_data->agc_sample_cnt[led_num2] = 0;
				max86915_data->update_led(max86915_data,
						max86915_data->agc_current[led_num2], led_num2);
			}
			if (max86915_data->agc_current[led_num3] != MAX86915_MIN_CURRENT) { /* Disable led_num3 */
				/* init */
				max86915_data->agc_current[led_num3] = MAX86915_MIN_CURRENT;
				max86915_data->reached_thresh[led_num3] = 0;
				max86915_data->prev_current[led_num3] = 0;
				max86915_data->prev_agc_current[led_num3]
					= MAX86915_MAX_CURRENT(max86915_data->led_range[led_num3]);
				max86915_data->prev_ppg[led_num3] = 0;
				max86915_data->agc_sum[led_num3] = 0;
				max86915_data->agc_sample_cnt[led_num3] = 0;
				max86915_data->update_led(max86915_data,
						max86915_data->agc_current[led_num3], led_num3);
			}
#endif
		}
	} else if (max86915_data->agc_mode == M_SDK) {
		if (max86915_data->mode_sdk_enabled & 0x01) { /* IR channel */
			if (max86915_data->agc_current[led_num0] != MAX86915_MIN_CURRENT) {
				err |= max86915_hrm_agc(max86915_data, ir, led_num0);
			} else {
				max86915_data->agc_current[led_num0]
					= max86915_data->default_current1
					* MAX86915_CURRENT_PER_STEP(max86915_data->led_range[led_num0]);
				max86915_data->agc_enabled = 0;
				max86915_data->sample_cnt = 0;
				max86915_data->reached_thresh[led_num0] = 0;
				max86915_data->prev_current[led_num0] = 0;
				max86915_data->prev_agc_current[led_num0]
					= MAX86915_MAX_CURRENT(max86915_data->led_range[led_num0]);
				max86915_data->prev_ppg[led_num0] = 0;
				max86915_data->agc_sum[led_num0] = 0;
				max86915_data->agc_sample_cnt[led_num0] = 0;
				max86915_data->update_led(max86915_data,
						max86915_data->agc_current[led_num0], led_num0);
			}
		}
		if (max86915_data->mode_sdk_enabled & 0x02) { /* RED channel */
			if (max86915_data->agc_current[led_num1] != MAX86915_MIN_CURRENT) {
				err |= max86915_hrm_agc(max86915_data, red, led_num1);
			} else {
				max86915_data->agc_current[led_num1]
					= max86915_data->default_current2
					* MAX86915_CURRENT_PER_STEP(max86915_data->led_range[led_num1]);
				max86915_data->agc_enabled = 0;
				max86915_data->sample_cnt = 0;
				max86915_data->reached_thresh[led_num1] = 0;
				max86915_data->prev_current[led_num1] = 0;
				max86915_data->prev_agc_current[led_num1]
					= MAX86915_MAX_CURRENT(max86915_data->led_range[led_num1]);
				max86915_data->prev_ppg[led_num1] = 0;
				max86915_data->agc_sum[led_num1] = 0;
				max86915_data->agc_sample_cnt[led_num1] = 0;
				max86915_data->update_led(max86915_data,
						max86915_data->agc_current[led_num1], led_num1);
			}
		}
		if (max86915_data->mode_sdk_enabled & 0x04) { /* Green channel */
			if (max86915_data->agc_current[led_num2] != MAX86915_MIN_CURRENT) {
				err |= max86915_hrm_agc(max86915_data, green, led_num2);
			} else {
				max86915_data->agc_current[led_num2]
					= max86915_data->default_current3
					* MAX86915_CURRENT_PER_STEP(max86915_data->led_range[led_num2]);
				max86915_data->agc_enabled = 0;
				max86915_data->sample_cnt = 0;
				max86915_data->reached_thresh[led_num2] = 0;
				max86915_data->prev_current[led_num2] = 0;
				max86915_data->prev_agc_current[led_num2]
					= MAX86915_MAX_CURRENT(max86915_data->led_range[led_num2]);
				max86915_data->prev_ppg[led_num2] = 0;
				max86915_data->agc_sum[led_num2] = 0;
				max86915_data->agc_sample_cnt[led_num2] = 0;
				max86915_data->update_led(max86915_data,
						max86915_data->agc_current[led_num2], led_num2);
			}
		}
		if (max86915_data->mode_sdk_enabled & 0x08) { /* BLUE channel */
			if (max86915_data->agc_current[led_num3] != MAX86915_MIN_CURRENT) {
				err |= max86915_hrm_agc(max86915_data, blue, led_num3);
			} else {
				max86915_data->agc_current[led_num3]
					= max86915_data->default_current4
					* MAX86915_CURRENT_PER_STEP(max86915_data->led_range[led_num3]);
				max86915_data->agc_enabled = 0;
				max86915_data->sample_cnt = 0;
				max86915_data->reached_thresh[led_num3] = 0;
				max86915_data->prev_current[led_num3] = 0;
				max86915_data->prev_agc_current[led_num3]
					= MAX86915_MAX_CURRENT(max86915_data->led_range[led_num3]);
				max86915_data->prev_ppg[led_num3] = 0;
				max86915_data->agc_sum[led_num3] = 0;
				max86915_data->agc_sample_cnt[led_num3] = 0;
				max86915_data->update_led(max86915_data,
						max86915_data->agc_current[led_num3], led_num3);
			}
		}
	}

	return err;
}

static void max86915_init_eol_data(struct max86915_eol_data *eol_data)
{
	int i = 0;

	memset(eol_data, 0, sizeof(struct max86915_eol_data));
	do_gettimeofday(&eol_data->start_time);

	for (i = 0; i < SELF_MAX_CH_NUM; i++)
		eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].min_val[i] = 0x80000;

	eol_data->state = _EOL_STATE_TYPE_NEW_INIT;
}
static void max86915_eol_flicker_data(int ir_data, struct  max86915_eol_data *eol_data)
{
		int i = 0;
		int eol_flicker_retry = eol_data->eol_flicker_data.retry;
		int eol_flicker_count = eol_data->eol_flicker_data.count;
		int eol_flicker_index = eol_data->eol_flicker_data.index;
		u64 data[2];
		u64 average[2];

		if (!eol_flicker_count)
			HRM_dbg("%s - EOL FLICKER STEP 1 STARTED !!\n", __func__);

		if (eol_flicker_count < EOL_NEW_FLICKER_SKIP_CNT)
			goto eol_flicker_exit;

		if (!eol_data->eol_flicker_data.state) {
			if (ir_data < EOL_NEW_FLICKER_THRESH)
				eol_data->eol_flicker_data.state = 1;
			else {
				if (eol_flicker_retry < EOL_NEW_FLICKER_RETRY_CNT) {
					HRM_dbg("%s - EOL FLICKER STEP 1 Retry : %d\n",
						__func__, eol_data->eol_flicker_data.retry);
					eol_data->eol_flicker_data.retry++;
					goto eol_flicker_exit;
				} else
					eol_data->eol_flicker_data.state = 1;
			}
		}

		if (eol_flicker_index < EOL_NEW_FLICKER_SIZE) {
			eol_data->eol_flicker_data.buf[SELF_IR_CH][eol_flicker_index]
				= (u64)ir_data * CONVER_FLOAT;
			eol_data->eol_flicker_data.sum[SELF_IR_CH]
				+= eol_data->eol_flicker_data.buf[SELF_IR_CH][eol_flicker_index];
			if (eol_data->eol_flicker_data.max
					< eol_data->eol_flicker_data.buf[SELF_IR_CH][eol_flicker_index])
				eol_data->eol_flicker_data.max
					= eol_data->eol_flicker_data.buf[SELF_IR_CH][eol_flicker_index];
			eol_data->eol_flicker_data.index++;
		} else if (eol_flicker_index == EOL_NEW_FLICKER_SIZE && eol_data->eol_flicker_data.done != 1) {
			do_div(eol_data->eol_flicker_data.sum[SELF_IR_CH], EOL_NEW_FLICKER_SIZE);
			eol_data->eol_flicker_data.average[SELF_IR_CH] = eol_data->eol_flicker_data.sum[SELF_IR_CH];
			average[SELF_IR_CH] = eol_data->eol_flicker_data.average[SELF_IR_CH];
			do_div(average[SELF_IR_CH], CONVER_FLOAT);

			for (i = 0; i <	EOL_NEW_FLICKER_SIZE; i++) {
				do_div(eol_data->eol_flicker_data.buf[SELF_IR_CH][i], CONVER_FLOAT);
				/*
				 * HRM_dbg("EOL FLICKER STEP 1 ir_data %d, %llu",
				 * i, eol_data->eol_flicker_data.buf[SELF_IR_CH][i]);
				 */
				data[SELF_IR_CH] = eol_data->eol_flicker_data.buf[SELF_IR_CH][i];
				eol_data->eol_flicker_data.std_sum[SELF_IR_CH]
					+= (data[SELF_IR_CH] - average[SELF_IR_CH])
						* (data[SELF_IR_CH] - average[SELF_IR_CH]);
			}
			eol_data->eol_flicker_data.done = 1;
			HRM_dbg("%s - EOL FLICKER STEP 1 Done\n", __func__);
		}
eol_flicker_exit:
		eol_data->eol_flicker_data.count++;

}

static void max86915_eol_std_sum(struct max86915_eol_hrm_data *eol_hrm_data)
{
	int ch_i = 0, std_i = 0;
	int buf_i = 0, buf_idx = 0;
	u64 buf[10] = {0, };
	u64 avg = 0, sum = 0, sum_max = 0;

	HRM_dbg("%s - Skip Pos:%d, %d, %d, %d, %d, %d, %d, %d\n", __func__,
			eol_hrm_data->min_pos[0], eol_hrm_data->max_pos[0],
			eol_hrm_data->min_pos[1], eol_hrm_data->max_pos[1],
			eol_hrm_data->min_pos[2], eol_hrm_data->max_pos[2],
			eol_hrm_data->min_pos[3], eol_hrm_data->max_pos[3]);

	for (ch_i = 0; ch_i < SELF_MAX_CH_NUM; ch_i++) {
		sum_max = 0;
		buf_idx = 0;
		for (std_i = 0; std_i < 25; std_i++) {
			sum = 0;
			avg = 0;
			for (buf_i = 0; buf_i < 10; buf_i++) {
				buf[buf_i] = eol_hrm_data->buf[ch_i][buf_idx];
				if ((buf_idx == eol_hrm_data->min_pos[ch_i])
					|| (buf_idx == eol_hrm_data->max_pos[ch_i]))
					buf_i--;
				else
					avg += buf[buf_i];

				buf_idx++;
			}
			do_div(avg, 10);

			for (buf_i = 0; buf_i < 10; buf_i++)
				sum += (buf[buf_i] - avg)*(buf[buf_i] - avg);

			HRM_dbg("%s - sum %d %d : %lld, %lld\n", __func__, ch_i, std_i, avg, sum);

			if (sum > sum_max)
				sum_max = sum;
		}
		eol_hrm_data->std_sum2[ch_i] = sum_max;
	}
}

static void max86915_eol_hrm_data(int ir_data, int red_data, int green_data, int blue_data,
	struct  max86915_eol_data *eol_data, u32 array_pos, u32 eol_skip_cnt, u32 eol_step_size)
{
	int i = 0;
	int hrm_eol_count = eol_data->eol_hrm_data[array_pos].count;
	int hrm_eol_index = eol_data->eol_hrm_data[array_pos].index;
	int ir_current = eol_data->eol_hrm_data[array_pos].ir_current;
	int red_current = eol_data->eol_hrm_data[array_pos].red_current;
	int green_current = eol_data->eol_hrm_data[array_pos].green_current;
	int blue_current = eol_data->eol_hrm_data[array_pos].blue_current;
	u64 data[4];
	u64 average[4];
	u32 hrm_eol_skip_cnt = eol_skip_cnt;
	u32 hrm_eol_size = eol_step_size;

	if (!hrm_eol_count) {
		HRM_dbg("%s - STEP [%d], [%d] started IR : %d , RED : %d, GREEN : %d, BLUE : %d,eol_skip_cnt : %d, eol_step_size : %d\n",
			__func__, array_pos, array_pos + 2, ir_current, red_current,
			green_current, blue_current, eol_skip_cnt, eol_step_size);
	}
	if (eol_step_size > EOL_NEW_HRM_SIZE) {
		HRM_dbg("%s - FATAL ERROR !!!! the buffer size should be smaller than EOL_NEW_HRM_SIZE\n", __func__);
		goto hrm_eol_exit;
	}
	if (hrm_eol_count < hrm_eol_skip_cnt)
		goto hrm_eol_exit;

	if (hrm_eol_index < hrm_eol_size) {
		eol_data->eol_hrm_data[array_pos].buf[SELF_IR_CH][hrm_eol_index] = (u64)ir_data * CONVER_FLOAT;
		eol_data->eol_hrm_data[array_pos].buf[SELF_RED_CH][hrm_eol_index] = (u64)red_data * CONVER_FLOAT;
		eol_data->eol_hrm_data[array_pos].buf[SELF_GREEN_CH][hrm_eol_index] = (u64)green_data * CONVER_FLOAT;
		eol_data->eol_hrm_data[array_pos].buf[SELF_BLUE_CH][hrm_eol_index] = (u64)blue_data * CONVER_FLOAT;
		eol_data->eol_hrm_data[array_pos].sum[SELF_IR_CH]
			+= eol_data->eol_hrm_data[array_pos].buf[SELF_IR_CH][hrm_eol_index];
		eol_data->eol_hrm_data[array_pos].sum[SELF_RED_CH]
			+= eol_data->eol_hrm_data[array_pos].buf[SELF_RED_CH][hrm_eol_index];
		eol_data->eol_hrm_data[array_pos].sum[SELF_GREEN_CH]
			+= eol_data->eol_hrm_data[array_pos].buf[SELF_GREEN_CH][hrm_eol_index];
		eol_data->eol_hrm_data[array_pos].sum[SELF_BLUE_CH]
			+= eol_data->eol_hrm_data[array_pos].buf[SELF_BLUE_CH][hrm_eol_index];
		if (array_pos == EOL_NEW_MODE_DC_HIGH
				&& eol_data->eol_hrm_data[array_pos].index < EOL_NEW_STD_TOTAL_CNT) {
			if (ir_data >= eol_data->eol_hrm_data[array_pos].max_val[SELF_IR_CH]) {
				eol_data->eol_hrm_data[array_pos].max_val[SELF_IR_CH] = ir_data;
				eol_data->eol_hrm_data[array_pos].max_pos[SELF_IR_CH]
					= eol_data->eol_hrm_data[array_pos].index;
			}
			if (ir_data < eol_data->eol_hrm_data[array_pos].min_val[SELF_IR_CH]) {
				eol_data->eol_hrm_data[array_pos].min_val[SELF_IR_CH] = ir_data;
				eol_data->eol_hrm_data[array_pos].min_pos[SELF_IR_CH]
					= eol_data->eol_hrm_data[array_pos].index;
			}
			if (red_data >= eol_data->eol_hrm_data[array_pos].max_val[SELF_RED_CH]) {
				eol_data->eol_hrm_data[array_pos].max_val[SELF_RED_CH] = red_data;
				eol_data->eol_hrm_data[array_pos].max_pos[SELF_RED_CH]
					= eol_data->eol_hrm_data[array_pos].index;
			}
			if (red_data < eol_data->eol_hrm_data[array_pos].min_val[SELF_RED_CH]) {
				eol_data->eol_hrm_data[array_pos].min_val[SELF_RED_CH] = red_data;
				eol_data->eol_hrm_data[array_pos].min_pos[SELF_RED_CH]
					= eol_data->eol_hrm_data[array_pos].index;
			}
			if (green_data >= eol_data->eol_hrm_data[array_pos].max_val[SELF_GREEN_CH]) {
				eol_data->eol_hrm_data[array_pos].max_val[SELF_GREEN_CH] = green_data;
				eol_data->eol_hrm_data[array_pos].max_pos[SELF_GREEN_CH]
					= eol_data->eol_hrm_data[array_pos].index;
			}
			if (green_data < eol_data->eol_hrm_data[array_pos].min_val[SELF_GREEN_CH]) {
				eol_data->eol_hrm_data[array_pos].min_val[SELF_GREEN_CH] = green_data;
				eol_data->eol_hrm_data[array_pos].min_pos[SELF_GREEN_CH]
					= eol_data->eol_hrm_data[array_pos].index;
			}
			if (blue_data >= eol_data->eol_hrm_data[array_pos].max_val[SELF_BLUE_CH]) {
				eol_data->eol_hrm_data[array_pos].max_val[SELF_BLUE_CH] = blue_data;
				eol_data->eol_hrm_data[array_pos].max_pos[SELF_BLUE_CH]
					= eol_data->eol_hrm_data[array_pos].index;
			}
			if (blue_data < eol_data->eol_hrm_data[array_pos].min_val[SELF_BLUE_CH]) {
				eol_data->eol_hrm_data[array_pos].min_val[SELF_BLUE_CH]
					= blue_data;
				eol_data->eol_hrm_data[array_pos].min_pos[SELF_BLUE_CH]
					= eol_data->eol_hrm_data[array_pos].index;
			}
		}
		eol_data->eol_hrm_data[array_pos].index++;
	} else if (hrm_eol_index == hrm_eol_size && eol_data->eol_hrm_data[array_pos].done != 1) {
		do_div(eol_data->eol_hrm_data[array_pos].sum[SELF_IR_CH], hrm_eol_size);
		eol_data->eol_hrm_data[array_pos].average[SELF_IR_CH]
			= eol_data->eol_hrm_data[array_pos].sum[SELF_IR_CH];
		do_div(eol_data->eol_hrm_data[array_pos].sum[SELF_RED_CH], hrm_eol_size);
		eol_data->eol_hrm_data[array_pos].average[SELF_RED_CH]
			= eol_data->eol_hrm_data[array_pos].sum[SELF_RED_CH];
		do_div(eol_data->eol_hrm_data[array_pos].sum[SELF_GREEN_CH], hrm_eol_size);
		eol_data->eol_hrm_data[array_pos].average[SELF_GREEN_CH]
			= eol_data->eol_hrm_data[array_pos].sum[SELF_GREEN_CH];
		do_div(eol_data->eol_hrm_data[array_pos].sum[SELF_BLUE_CH], hrm_eol_size);
		eol_data->eol_hrm_data[array_pos].average[SELF_BLUE_CH]
			= eol_data->eol_hrm_data[array_pos].sum[SELF_BLUE_CH];

		average[SELF_IR_CH] = eol_data->eol_hrm_data[array_pos].average[SELF_IR_CH];
		do_div(average[SELF_IR_CH], CONVER_FLOAT);
		average[SELF_RED_CH] = eol_data->eol_hrm_data[array_pos].average[SELF_RED_CH];
		do_div(average[SELF_RED_CH], CONVER_FLOAT);
		average[SELF_GREEN_CH] = eol_data->eol_hrm_data[array_pos].average[SELF_GREEN_CH];
		do_div(average[SELF_GREEN_CH], CONVER_FLOAT);
		average[SELF_BLUE_CH] = eol_data->eol_hrm_data[array_pos].average[SELF_BLUE_CH];
		do_div(average[SELF_BLUE_CH], CONVER_FLOAT);

		for (i = 0; i <	hrm_eol_size; i++) {
			do_div(eol_data->eol_hrm_data[array_pos].buf[SELF_IR_CH][i], CONVER_FLOAT);
			data[SELF_IR_CH] = eol_data->eol_hrm_data[array_pos].buf[SELF_IR_CH][i];
			do_div(eol_data->eol_hrm_data[array_pos].buf[SELF_RED_CH][i], CONVER_FLOAT);
			data[SELF_RED_CH] = eol_data->eol_hrm_data[array_pos].buf[SELF_RED_CH][i];
			do_div(eol_data->eol_hrm_data[array_pos].buf[SELF_GREEN_CH][i], CONVER_FLOAT);
			data[SELF_GREEN_CH] = eol_data->eol_hrm_data[array_pos].buf[SELF_GREEN_CH][i];
			do_div(eol_data->eol_hrm_data[array_pos].buf[SELF_BLUE_CH][i], CONVER_FLOAT);
			data[SELF_BLUE_CH] = eol_data->eol_hrm_data[array_pos].buf[SELF_BLUE_CH][i];

			eol_data->eol_hrm_data[array_pos].std_sum[SELF_IR_CH]
				+= (data[SELF_IR_CH] - average[SELF_IR_CH])
					* (data[SELF_IR_CH] - average[SELF_IR_CH]);
			eol_data->eol_hrm_data[array_pos].std_sum[SELF_RED_CH]
				+= (data[SELF_RED_CH] - average[SELF_RED_CH])
					* (data[SELF_RED_CH] - average[SELF_RED_CH]);
			eol_data->eol_hrm_data[array_pos].std_sum[SELF_GREEN_CH]
				+= (data[SELF_GREEN_CH] - average[SELF_GREEN_CH])
					* (data[SELF_GREEN_CH] - average[SELF_GREEN_CH]);
			eol_data->eol_hrm_data[array_pos].std_sum[SELF_BLUE_CH]
				+= (data[SELF_BLUE_CH] - average[SELF_BLUE_CH])
					* (data[SELF_BLUE_CH] - average[SELF_BLUE_CH]);
		}
		if (array_pos == EOL_NEW_MODE_DC_HIGH)
			max86915_eol_std_sum(&eol_data->eol_hrm_data[array_pos]);

		eol_data->eol_hrm_data[array_pos].done = 1;
		eol_data->eol_hrm_flag |= 1 << array_pos;
		HRM_dbg("%s - STEP [%d], [%d], [%x] Done\n",
			__func__, array_pos, array_pos + 2, eol_data->eol_hrm_flag);
	}
hrm_eol_exit:
	eol_data->eol_hrm_data[array_pos].count++;
}

static void max86915_eol_freq_data(struct  max86915_eol_data *eol_data, u8 divid)
{
	int freq_state = eol_data->eol_freq_data.state;
	unsigned long freq_end_time = eol_data->eol_freq_data.end_time;
	unsigned long freq_prev_time = eol_data->eol_freq_data.prev_time;
	unsigned long freq_current_time = jiffies;
	int freq_count = eol_data->eol_freq_data.skip_count;
	unsigned long freq_interval;
	int freq_start_time;
	int freq_work_time;
	u32 freq_skip_cnt = EOL_NEW_FREQ_SKIP_CNT / divid;
	u32 freq_interrupt_min = EOL_NEW_FREQ_MIN / divid;
	u32 freq_timeout = (HZ / (HZ / divid));

	if (freq_count < freq_skip_cnt)
		goto seq3_exit;

	switch (freq_state) {
	case 0:
		HRM_dbg("%s - EOL FREQ 7 stated ###\n", __func__);
		eol_data->eol_freq_data.start_time = jiffies;
		do_gettimeofday(&eol_data->eol_freq_data.start_time_t);
		eol_data->eol_freq_data.end_time = jiffies + HZ;        /* timeout in 1s */
		eol_data->eol_freq_data.state = 1;
		eol_data->eol_freq_data.count++;
		break;
	case 1:
		if (time_is_after_eq_jiffies(freq_end_time)) {
			if (time_is_after_eq_jiffies(freq_prev_time)) {
				eol_data->eol_freq_data.count++;
			} else {
				if (eol_data->eol_freq_data.skew_retry < EOL_NEW_FREQ_RETRY_CNT) {
					eol_data->eol_freq_data.skew_retry++;
					freq_interval = freq_current_time-freq_prev_time;
					HRM_dbg("%s - !!!!!! EOL FREQ 7 Clock skew too large retry : %d ,interval : %lu ms, count : %d\n", __func__,
						eol_data->eol_freq_data.skew_retry, freq_interval*10,
						eol_data->eol_freq_data.count);
					eol_data->eol_freq_data.state = 0;
					eol_data->eol_freq_data.count = 0;
					eol_data->eol_freq_data.prev_time = 0;
				}
			}
			do_gettimeofday(&eol_data->eol_freq_data.work_time_t);
		} else {
			if (eol_data->eol_freq_data.count < freq_interrupt_min
				&& eol_data->eol_freq_data.retry < EOL_NEW_FREQ_SKEW_RETRY_CNT) {
				HRM_dbg("%s - !!!!!! EOL FREQ 7 CNT Gap too large : %d, retry : %d\n", __func__,
					eol_data->eol_freq_data.count, eol_data->eol_freq_data.retry);
				eol_data->eol_freq_data.retry++;
				eol_data->eol_freq_data.state = 0;
				eol_data->eol_freq_data.count = 0;
				eol_data->eol_freq_data.prev_time = 0;
			} else {
				eol_data->eol_freq_data.count *= divid;
				eol_data->eol_freq_data.done = 1;
				freq_start_time = (eol_data->eol_freq_data.start_time_t.tv_sec * 1000)
							+ (eol_data->eol_freq_data.start_time_t.tv_usec / 1000);
				freq_work_time = (eol_data->eol_freq_data.work_time_t.tv_sec * 1000)
							+ (eol_data->eol_freq_data.work_time_t.tv_usec / 1000);
				HRM_dbg("%s - EOL FREQ 7 Done###\n", __func__);
				HRM_dbg("%s - EOL FREQ 7 sample_no_cnt : %d, time : %d ms, divid : %d\n", __func__,
					eol_data->eol_freq_data.count, freq_work_time - freq_start_time, divid);
			}
		}
		break;
	default:
		HRM_dbg("%s - EOL FREQ 7 Should not call this routine !!!###\n", __func__);
		break;
	}
seq3_exit:
	eol_data->eol_freq_data.prev_time = freq_current_time + freq_timeout;  /* timeout in 10 or 40 ms or 100 ms */
	eol_data->eol_freq_data.skip_count++;
}

u32 max86915_eol_sqrt(u64 sqsum)
{
	u64 sq_rt;
	u64 g0, g1, g2, g3, g4;
	unsigned int seed;
	unsigned int next;
	unsigned int step;

	g4 = sqsum / 100000000;
	g3 = (sqsum - g4 * 100000000) / 1000000;
	g2 = (sqsum - g4 * 100000000 - g3 * 1000000) / 10000;
	g1 = (sqsum - g4 * 100000000 - g3 * 1000000 - g2 * 10000) / 100;
	g0 = (sqsum - g4 * 100000000 - g3 * 1000000 - g2 * 10000 - g1 * 100);

	next = g4;
	step = 0;
	seed = 0;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = (u64)seed * 10000;
	next = (next - (seed * step)) * 100 + g3;

	step = 0;
	seed = 2 * seed * 10;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 1000;
	next = (next - seed * step) * 100 + g2;
	seed = (seed + step) * 10;
	step = 0;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	 }

	sq_rt = sq_rt + step * 100;
	next = (next - seed * step) * 100 + g1;
	seed = (seed + step) * 10;
	step = 0;

	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 10;
	next = (next - seed * step) * 100 + g0;
	seed = (seed + step) * 10;
	step = 0;

	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step;

	return (u32) sq_rt;
}

void max86915_eol_xtalk_data(int ir_data, struct max86915_eol_data *eol_data)
{
	int eol_xtalk_count = eol_data->eol_xtalk_data.count;
	int eol_xtalk_index = eol_data->eol_xtalk_data.index;

	if (!eol_xtalk_count)
		HRM_dbg("%s - EOL XTALK STEP 1 STARTED !!\n", __func__);

	if (eol_xtalk_count < EOL_XTALK_SKIP_CNT)
		goto xtalk_eol_exit;

	if (eol_xtalk_index < EOL_XTALK_SIZE) {
		eol_data->eol_xtalk_data.buf[eol_xtalk_index] = (s64)ir_data;
		eol_data->eol_xtalk_data.sum += eol_data->eol_xtalk_data.buf[eol_xtalk_index];
		eol_data->eol_xtalk_data.index++;
	} else {
		eol_data->eol_xtalk_data.average
			= div64_s64(eol_data->eol_xtalk_data.sum, EOL_XTALK_SIZE);
		eol_data->eol_xtalk_data.done = EOL_SUCCESS;
		HRM_dbg("%s - EOL XTALK STEP 1 Done\n", __func__);
	}

xtalk_eol_exit:
	eol_data->eol_xtalk_data.count++;

}

void max86915_make_eol_item(struct max86915_eol_data *eol_data)
{
	switch (max86915_data->eol_test_type) {
	case EOL_15_MODE:
		snprintf(eol_data->eol_item_data.test_type, 5, "%s", "MAIN");
		eol_data->eol_item_data.system_noise[EOL_SPEC_MIN] = max86915_data->eol_spec[0];
		eol_data->eol_item_data.system_noise[EOL_SPEC_MAX] = max86915_data->eol_spec[1];
		eol_data->eol_item_data.ir_mid[EOL_SPEC_MIN] = max86915_data->eol_spec[2];
		eol_data->eol_item_data.ir_mid[EOL_SPEC_MAX] = max86915_data->eol_spec[3];
		eol_data->eol_item_data.red_mid[EOL_SPEC_MIN] = max86915_data->eol_spec[4];
		eol_data->eol_item_data.red_mid[EOL_SPEC_MAX] = max86915_data->eol_spec[5];
		eol_data->eol_item_data.green_mid[EOL_SPEC_MIN] = max86915_data->eol_spec[6];
		eol_data->eol_item_data.green_mid[EOL_SPEC_MAX] = max86915_data->eol_spec[7];
		eol_data->eol_item_data.blue_mid[EOL_SPEC_MIN] = max86915_data->eol_spec[8];
		eol_data->eol_item_data.blue_mid[EOL_SPEC_MAX] = max86915_data->eol_spec[9];
		eol_data->eol_item_data.ir_high[EOL_SPEC_MIN] = max86915_data->eol_spec[10];
		eol_data->eol_item_data.ir_high[EOL_SPEC_MAX] = max86915_data->eol_spec[11];
		eol_data->eol_item_data.red_high[EOL_SPEC_MIN] = max86915_data->eol_spec[12];
		eol_data->eol_item_data.red_high[EOL_SPEC_MAX] = max86915_data->eol_spec[13];
		eol_data->eol_item_data.green_high[EOL_SPEC_MIN] = max86915_data->eol_spec[14];
		eol_data->eol_item_data.green_high[EOL_SPEC_MAX] = max86915_data->eol_spec[15];
		eol_data->eol_item_data.blue_high[EOL_SPEC_MIN] = max86915_data->eol_spec[16];
		eol_data->eol_item_data.blue_high[EOL_SPEC_MAX] = max86915_data->eol_spec[17];
		eol_data->eol_item_data.ir_xtalk[EOL_SPEC_MIN] = max86915_data->eol_spec[18];
		eol_data->eol_item_data.ir_xtalk[EOL_SPEC_MAX] = max86915_data->eol_spec[19];
		eol_data->eol_item_data.red_xtalk[EOL_SPEC_MIN] = max86915_data->eol_spec[20];
		eol_data->eol_item_data.red_xtalk[EOL_SPEC_MAX] = max86915_data->eol_spec[21];
		eol_data->eol_item_data.green_xtalk[EOL_SPEC_MIN] = max86915_data->eol_spec[22];
		eol_data->eol_item_data.green_xtalk[EOL_SPEC_MAX] = max86915_data->eol_spec[23];
		eol_data->eol_item_data.blue_xtalk[EOL_SPEC_MIN] = max86915_data->eol_spec[24];
		eol_data->eol_item_data.blue_xtalk[EOL_SPEC_MAX] = max86915_data->eol_spec[25];
		eol_data->eol_item_data.ir_noise[EOL_SPEC_MIN] = max86915_data->eol_spec[26];
		eol_data->eol_item_data.ir_noise[EOL_SPEC_MAX] = max86915_data->eol_spec[27];
		eol_data->eol_item_data.red_noise[EOL_SPEC_MIN] = max86915_data->eol_spec[28];
		eol_data->eol_item_data.red_noise[EOL_SPEC_MAX] = max86915_data->eol_spec[29];
		eol_data->eol_item_data.green_noise[EOL_SPEC_MIN] = max86915_data->eol_spec[30];
		eol_data->eol_item_data.green_noise[EOL_SPEC_MAX] = max86915_data->eol_spec[31];
		eol_data->eol_item_data.blue_noise[EOL_SPEC_MIN] = max86915_data->eol_spec[32];
		eol_data->eol_item_data.blue_noise[EOL_SPEC_MAX] = max86915_data->eol_spec[33];
		eol_data->eol_item_data.frequency[EOL_SPEC_MIN] = max86915_data->eol_spec[34];
		eol_data->eol_item_data.frequency[EOL_SPEC_MAX] = max86915_data->eol_spec[35];
		break;
	case EOL_SEMI_FT:
		snprintf(eol_data->eol_item_data.test_type, 5, "%s", "SEMI");
		eol_data->eol_item_data.system_noise[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[0];
		eol_data->eol_item_data.system_noise[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[1];
		eol_data->eol_item_data.ir_mid[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[2];
		eol_data->eol_item_data.ir_mid[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[3];
		eol_data->eol_item_data.red_mid[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[4];
		eol_data->eol_item_data.red_mid[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[5];
		eol_data->eol_item_data.green_mid[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[6];
		eol_data->eol_item_data.green_mid[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[7];
		eol_data->eol_item_data.blue_mid[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[8];
		eol_data->eol_item_data.blue_mid[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[9];
		eol_data->eol_item_data.ir_high[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[10];
		eol_data->eol_item_data.ir_high[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[11];
		eol_data->eol_item_data.red_high[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[12];
		eol_data->eol_item_data.red_high[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[13];
		eol_data->eol_item_data.green_high[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[14];
		eol_data->eol_item_data.green_high[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[15];
		eol_data->eol_item_data.blue_high[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[16];
		eol_data->eol_item_data.blue_high[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[17];
		eol_data->eol_item_data.ir_xtalk[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[18];
		eol_data->eol_item_data.ir_xtalk[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[19];
		eol_data->eol_item_data.red_xtalk[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[20];
		eol_data->eol_item_data.red_xtalk[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[21];
		eol_data->eol_item_data.green_xtalk[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[22];
		eol_data->eol_item_data.green_xtalk[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[23];
		eol_data->eol_item_data.blue_xtalk[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[24];
		eol_data->eol_item_data.blue_xtalk[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[25];
		eol_data->eol_item_data.ir_noise[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[26];
		eol_data->eol_item_data.ir_noise[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[27];
		eol_data->eol_item_data.red_noise[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[28];
		eol_data->eol_item_data.red_noise[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[29];
		eol_data->eol_item_data.green_noise[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[30];
		eol_data->eol_item_data.green_noise[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[31];
		eol_data->eol_item_data.blue_noise[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[32];
		eol_data->eol_item_data.blue_noise[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[33];
		eol_data->eol_item_data.frequency[EOL_SPEC_MIN] = max86915_data->eol_semi_spec[34];
		eol_data->eol_item_data.frequency[EOL_SPEC_MAX] = max86915_data->eol_semi_spec[35];
		break;
	case EOL_XTALK:
		snprintf(eol_data->eol_item_data.test_type, 6, "%s", "XTALK");
		eol_data->eol_item_data.xtalk[EOL_SPEC_MIN] = (s32) max86915_data->eol_xtalk_spec[0];
		eol_data->eol_item_data.xtalk[EOL_SPEC_MAX] = (s32) max86915_data->eol_xtalk_spec[1];
		break;
	default:
		break;
	}

	switch (max86915_data->eol_test_type) {
	case EOL_15_MODE:
	case EOL_SEMI_FT:
		eol_data->eol_item_data.system_noise[EOL_MEASURE] =
			(u32) (eol_data->eol_flicker_data.average[SELF_IR_CH] / CONVER_FLOAT);
		eol_data->eol_item_data.ir_mid[EOL_MEASURE] =
			(u32) (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].average[SELF_IR_CH] / CONVER_FLOAT);
		eol_data->eol_item_data.red_mid[EOL_MEASURE] =
			(u32) (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].average[SELF_RED_CH] / CONVER_FLOAT);
		eol_data->eol_item_data.green_mid[EOL_MEASURE] =
			(u32) (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].average[SELF_GREEN_CH] / CONVER_FLOAT);
		eol_data->eol_item_data.blue_mid[EOL_MEASURE] =
			(u32) (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].average[SELF_BLUE_CH] / CONVER_FLOAT);
		eol_data->eol_item_data.ir_high[EOL_MEASURE] =
			(u32) (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].average[SELF_IR_CH] / CONVER_FLOAT);
		eol_data->eol_item_data.red_high[EOL_MEASURE] =
			(u32) (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].average[SELF_RED_CH] / CONVER_FLOAT);
		eol_data->eol_item_data.green_high[EOL_MEASURE] =
			(u32) (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].average[SELF_GREEN_CH] / CONVER_FLOAT);
		eol_data->eol_item_data.blue_high[EOL_MEASURE] =
			(u32) (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].average[SELF_BLUE_CH] / CONVER_FLOAT);
		eol_data->eol_item_data.ir_xtalk[EOL_MEASURE] =
			(u32) (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].average[SELF_IR_CH] / CONVER_FLOAT);
		eol_data->eol_item_data.red_xtalk[EOL_MEASURE] =
			(u32) (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].average[SELF_RED_CH] / CONVER_FLOAT);
		eol_data->eol_item_data.green_xtalk[EOL_MEASURE] =
			(u32) (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].average[SELF_GREEN_CH] / CONVER_FLOAT);
		eol_data->eol_item_data.blue_xtalk[EOL_MEASURE] =
			(u32) (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].average[SELF_BLUE_CH] / CONVER_FLOAT);

		eol_data->eol_item_data.ir_noise[EOL_MEASURE] =
			max86915_eol_sqrt(eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum2[SELF_IR_CH] / EOL_NEW_STD2_SIZE);
		eol_data->eol_item_data.red_noise[EOL_MEASURE] =
			max86915_eol_sqrt(eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum2[SELF_RED_CH] / EOL_NEW_STD2_SIZE);
		eol_data->eol_item_data.green_noise[EOL_MEASURE] =
			max86915_eol_sqrt (eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum2[SELF_GREEN_CH] / EOL_NEW_STD2_SIZE);
		eol_data->eol_item_data.blue_noise[EOL_MEASURE] =
			max86915_eol_sqrt(eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum2[SELF_BLUE_CH] / EOL_NEW_STD2_SIZE);

		eol_data->eol_item_data.frequency[EOL_MEASURE] = (u32) eol_data->eol_freq_data.count;

		
		if (eol_data->eol_item_data.system_noise[EOL_MEASURE] >= eol_data->eol_item_data.system_noise[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.system_noise[EOL_MEASURE] <= eol_data->eol_item_data.system_noise[EOL_SPEC_MAX])
			eol_data->eol_item_data.system_noise[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.system_noise[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.ir_mid[EOL_MEASURE] >= eol_data->eol_item_data.ir_mid[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.ir_mid[EOL_MEASURE] <= eol_data->eol_item_data.ir_mid[EOL_SPEC_MAX])
			eol_data->eol_item_data.ir_mid[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.ir_mid[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.red_mid[EOL_MEASURE] >= eol_data->eol_item_data.red_mid[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.red_mid[EOL_MEASURE] <= eol_data->eol_item_data.red_mid[EOL_SPEC_MAX])
			eol_data->eol_item_data.red_mid[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.red_mid[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.green_mid[EOL_MEASURE] >= eol_data->eol_item_data.green_mid[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.green_mid[EOL_MEASURE] <= eol_data->eol_item_data.green_mid[EOL_SPEC_MAX])
			eol_data->eol_item_data.green_mid[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.green_mid[EOL_PASS_FAIL] = false;	

		if (eol_data->eol_item_data.blue_mid[EOL_MEASURE] >= eol_data->eol_item_data.blue_mid[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.blue_mid[EOL_MEASURE] <= eol_data->eol_item_data.blue_mid[EOL_SPEC_MAX])
			eol_data->eol_item_data.blue_mid[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.blue_mid[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.ir_high[EOL_MEASURE] >= eol_data->eol_item_data.ir_high[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.ir_high[EOL_MEASURE] <= eol_data->eol_item_data.ir_high[EOL_SPEC_MAX])
			eol_data->eol_item_data.ir_high[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.ir_high[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.red_high[EOL_MEASURE] >= eol_data->eol_item_data.red_high[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.red_high[EOL_MEASURE] <= eol_data->eol_item_data.red_high[EOL_SPEC_MAX])
			eol_data->eol_item_data.red_high[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.red_high[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.green_high[EOL_MEASURE] >= eol_data->eol_item_data.green_high[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.green_high[EOL_MEASURE] <= eol_data->eol_item_data.green_high[EOL_SPEC_MAX])
			eol_data->eol_item_data.green_high[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.green_high[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.blue_high[EOL_MEASURE] >= eol_data->eol_item_data.blue_high[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.blue_high[EOL_MEASURE] <= eol_data->eol_item_data.blue_high[EOL_SPEC_MAX])
			eol_data->eol_item_data.blue_high[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.blue_high[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.ir_xtalk[EOL_MEASURE] >= eol_data->eol_item_data.ir_xtalk[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.ir_xtalk[EOL_MEASURE] <= eol_data->eol_item_data.ir_xtalk[EOL_SPEC_MAX])
			eol_data->eol_item_data.ir_xtalk[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.ir_xtalk[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.red_xtalk[EOL_MEASURE] >= eol_data->eol_item_data.red_xtalk[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.red_xtalk[EOL_MEASURE] <= eol_data->eol_item_data.red_xtalk[EOL_SPEC_MAX])
			eol_data->eol_item_data.red_xtalk[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.red_xtalk[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.green_xtalk[EOL_MEASURE] >= eol_data->eol_item_data.green_xtalk[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.green_xtalk[EOL_MEASURE] <= eol_data->eol_item_data.green_xtalk[EOL_SPEC_MAX])
			eol_data->eol_item_data.green_xtalk[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.green_xtalk[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.blue_xtalk[EOL_MEASURE] >= eol_data->eol_item_data.blue_xtalk[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.blue_xtalk[EOL_MEASURE] <= eol_data->eol_item_data.blue_xtalk[EOL_SPEC_MAX])
			eol_data->eol_item_data.blue_xtalk[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.blue_xtalk[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.ir_noise[EOL_MEASURE] >= eol_data->eol_item_data.ir_noise[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.ir_noise[EOL_MEASURE] <= eol_data->eol_item_data.ir_noise[EOL_SPEC_MAX])
			eol_data->eol_item_data.ir_noise[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.ir_noise[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.red_noise[EOL_MEASURE] >= eol_data->eol_item_data.red_noise[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.red_noise[EOL_MEASURE] <= eol_data->eol_item_data.red_noise[EOL_SPEC_MAX])
			eol_data->eol_item_data.red_noise[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.red_noise[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.green_noise[EOL_MEASURE] >= eol_data->eol_item_data.green_noise[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.green_noise[EOL_MEASURE] <= eol_data->eol_item_data.green_noise[EOL_SPEC_MAX])
			eol_data->eol_item_data.green_noise[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.green_noise[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.blue_noise[EOL_MEASURE] >= eol_data->eol_item_data.blue_noise[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.blue_noise[EOL_MEASURE] <= eol_data->eol_item_data.blue_noise[EOL_SPEC_MAX])
			eol_data->eol_item_data.blue_noise[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.blue_noise[EOL_PASS_FAIL] = false;

		if (eol_data->eol_item_data.frequency[EOL_MEASURE] >= eol_data->eol_item_data.frequency[EOL_SPEC_MIN]
			&& eol_data->eol_item_data.frequency[EOL_MEASURE] <= eol_data->eol_item_data.frequency[EOL_SPEC_MAX])
			eol_data->eol_item_data.frequency[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.frequency[EOL_PASS_FAIL] = false;
		break;
	case EOL_XTALK:
		eol_data->eol_item_data.xtalk[EOL_MEASURE] = (s32) eol_data->eol_xtalk_data.average;

		if (eol_data->eol_item_data.xtalk[EOL_MEASURE] >= eol_data->eol_item_data.xtalk[EOL_SPEC_MIN]
		&& eol_data->eol_item_data.xtalk[EOL_MEASURE] <= eol_data->eol_item_data.xtalk[EOL_SPEC_MAX])
			eol_data->eol_item_data.xtalk[EOL_PASS_FAIL] = true;
		else
			eol_data->eol_item_data.xtalk[EOL_PASS_FAIL] = false;
		break;
	default:
		break;
	}
}

static void max86915_eol_conv2float(struct max86915_eol_data *eol_data, u8 mode, struct max86915_device_data *device)
{
	struct max86915_div_data div_data;
	char flicker_max[2][INTEGER_LEN] = { {'\0', }, };
	char ir_mdc_avg[2][INTEGER_LEN] = { {'\0', }, }, red_mdc_avg[2][INTEGER_LEN] = { {'\0', }, };
	char green_mdc_avg[2][INTEGER_LEN] = { {'\0', }, }, blue_mdc_avg[2][INTEGER_LEN] = { {'\0', }, };
	char ir_hdc_avg[2][INTEGER_LEN] = { {'\0', }, }, ir_hdc_std[2][INTEGER_LEN] = { {'\0', }, };
	char red_hdc_avg[2][INTEGER_LEN] = { {'\0', }, }, red_hdc_std[2][INTEGER_LEN] = { {'\0', }, };
	char green_hdc_avg[2][INTEGER_LEN] = { {'\0', }, }, green_hdc_std[2][INTEGER_LEN] = { {'\0', }, };
	char blue_hdc_avg[2][INTEGER_LEN] = { {'\0', }, }, blue_hdc_std[2][INTEGER_LEN] = { {'\0', }, };
	char ir_xtalk_avg[2][INTEGER_LEN] = { {'\0', }, }, red_xtalk_avg[2][INTEGER_LEN] = { {'\0', }, };
	char green_xtalk_avg[2][INTEGER_LEN] = { {'\0', }, }, blue_xtalk_avg[2][INTEGER_LEN] = { {'\0', }, };
	char ir_hdc_std2[2][INTEGER_LEN] = { {'\0', }, };
	char red_hdc_std2[2][INTEGER_LEN] = { {'\0', }, };
	char green_hdc_std2[2][INTEGER_LEN] = { {'\0', }, };
	char blue_hdc_std2[2][INTEGER_LEN] = { {'\0', }, };

	HRM_info("%s - start this function.\n", __func__);

	if (device->pre_eol_test_is_enable != 2) {
		/* flicker */
		max86915_div_float(&div_data, eol_data->eol_flicker_data.max, CONVER_FLOAT);
		strncpy(flicker_max[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(flicker_max[1], div_data.result_decimal, INTEGER_LEN - 1);
	}

	if (device->pre_eol_test_is_enable == 0) {
		/* dc mid */
		max86915_div_float(&div_data, eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].average[SELF_IR_CH], CONVER_FLOAT);
		strncpy(ir_mdc_avg[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(ir_mdc_avg[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data, eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].average[SELF_RED_CH], CONVER_FLOAT);
		strncpy(red_mdc_avg[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(red_mdc_avg[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data, eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].average[SELF_GREEN_CH], CONVER_FLOAT);
		strncpy(green_mdc_avg[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(green_mdc_avg[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data, eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].average[SELF_BLUE_CH], CONVER_FLOAT);
		strncpy(blue_mdc_avg[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(blue_mdc_avg[1], div_data.result_decimal, INTEGER_LEN - 1);

		/* dc high */
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].average[SELF_IR_CH], CONVER_FLOAT);
		strncpy(ir_hdc_avg[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(ir_hdc_avg[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum[SELF_IR_CH], EOL_NEW_DC_HIGH_SIZE);
		strncpy(ir_hdc_std[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(ir_hdc_std[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum2[SELF_IR_CH], EOL_NEW_STD2_SIZE);
		strncpy(ir_hdc_std2[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(ir_hdc_std2[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].average[SELF_RED_CH], CONVER_FLOAT);
		strncpy(red_hdc_avg[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(red_hdc_avg[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum[SELF_RED_CH], EOL_NEW_DC_HIGH_SIZE);
		strncpy(red_hdc_std[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(red_hdc_std[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum2[SELF_RED_CH], EOL_NEW_STD2_SIZE);
		strncpy(red_hdc_std2[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(red_hdc_std2[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].average[SELF_GREEN_CH], CONVER_FLOAT);
		strncpy(green_hdc_avg[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(green_hdc_avg[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum[SELF_GREEN_CH], EOL_NEW_DC_HIGH_SIZE);
		strncpy(green_hdc_std[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(green_hdc_std[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum2[SELF_GREEN_CH], EOL_NEW_STD2_SIZE);
		strncpy(green_hdc_std2[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(green_hdc_std2[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].average[SELF_BLUE_CH], CONVER_FLOAT);
		strncpy(blue_hdc_avg[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(blue_hdc_avg[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum[SELF_BLUE_CH], EOL_NEW_DC_HIGH_SIZE);
		strncpy(blue_hdc_std[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(blue_hdc_std[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum2[SELF_BLUE_CH], EOL_NEW_STD2_SIZE);
		strncpy(blue_hdc_std2[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(blue_hdc_std2[1], div_data.result_decimal, INTEGER_LEN - 1);

		/* dc xtalk */
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].average[SELF_IR_CH], CONVER_FLOAT);
		strncpy(ir_xtalk_avg[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(ir_xtalk_avg[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].average[SELF_RED_CH], CONVER_FLOAT);
		strncpy(red_xtalk_avg[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(red_xtalk_avg[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].average[SELF_GREEN_CH], CONVER_FLOAT);
		strncpy(green_xtalk_avg[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(green_xtalk_avg[1], div_data.result_decimal, INTEGER_LEN - 1);
		max86915_div_float(&div_data,
			eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].average[SELF_BLUE_CH], CONVER_FLOAT);
		strncpy(blue_xtalk_avg[0], div_data.result_integer, INTEGER_LEN - 1);
		strncpy(blue_xtalk_avg[1], div_data.result_decimal, INTEGER_LEN - 1);

		HRM_info("%s - hdc_std : %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, "
				"%s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c\n", __func__,
				ir_hdc_std[0], ir_hdc_std[1][0], ir_hdc_std[1][1],
				red_hdc_std[0], red_hdc_std[1][0], red_hdc_std[1][1],
				green_hdc_std[0], green_hdc_std[1][0], green_hdc_std[1][1],
				blue_hdc_std[0], blue_hdc_std[1][0], blue_hdc_std[1][1],
				ir_hdc_std2[0], ir_hdc_std2[1][0], ir_hdc_std2[1][1],
				red_hdc_std2[0], red_hdc_std2[1][0], red_hdc_std2[1][1],
				green_hdc_std2[0], green_hdc_std2[1][0], green_hdc_std2[1][1],
				blue_hdc_std2[0], blue_hdc_std2[1][0], blue_hdc_std2[1][1]);

		max86915_float_sqrt(ir_hdc_std[0], ir_hdc_std[1]);
		max86915_float_sqrt(red_hdc_std[0], red_hdc_std[1]);
		max86915_float_sqrt(green_hdc_std[0], green_hdc_std[1]);
		max86915_float_sqrt(blue_hdc_std[0], blue_hdc_std[1]);
		max86915_float_sqrt(ir_hdc_std2[0], ir_hdc_std2[1]);
		max86915_float_sqrt(red_hdc_std2[0], red_hdc_std2[1]);
		max86915_float_sqrt(green_hdc_std2[0], green_hdc_std2[1]);
		max86915_float_sqrt(blue_hdc_std2[0], blue_hdc_std2[1]);
	}

	if (device->pre_eol_test_is_enable == 1) { /* PRE EOL System Noise */
		snprintf(device->eol_test_result, MAX_EOL_RESULT, "%s.%c%c\n",
			flicker_max[0], flicker_max[1][0], flicker_max[1][1]);
	} else if (device->pre_eol_test_is_enable == 2) { /* PRE EOL FREQ */
		snprintf(device->eol_test_result, MAX_EOL_RESULT, "%d\n",
			eol_data->eol_freq_data.count);
	} else if (device->pre_eol_test_is_enable == 3) { /* PRE EOL BOTH */
		snprintf(device->eol_test_result, MAX_EOL_RESULT, "%s.%c%c, %d\n",
			flicker_max[0], flicker_max[1][0], flicker_max[1][1],
			eol_data->eol_freq_data.count);
	} else {
		if (device->part_type < PART_TYPE_MAX86917_1) { /* MAX86915 */
			snprintf(device->eol_test_result, MAX_EOL_RESULT, "%s.%c%c, 0.00, 0.00, 0.00, 0.00, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %s.%c%c, %d\n",
				flicker_max[0], flicker_max[1][0], flicker_max[1][1],
				ir_mdc_avg[0], ir_mdc_avg[1][0], ir_mdc_avg[1][1],
				red_mdc_avg[0], red_mdc_avg[1][0], red_mdc_avg[1][1],
				green_mdc_avg[0], green_mdc_avg[1][0], green_mdc_avg[1][1],
				blue_mdc_avg[0], blue_mdc_avg[1][0], blue_mdc_avg[1][1],
				ir_hdc_avg[0], ir_hdc_avg[1][0], ir_hdc_avg[1][1],
				red_hdc_avg[0], red_hdc_avg[1][0], red_hdc_avg[1][1],
				green_hdc_avg[0], green_hdc_avg[1][0], green_hdc_avg[1][1],
				blue_hdc_avg[0], blue_hdc_avg[1][0], blue_hdc_avg[1][1],
				ir_xtalk_avg[0], ir_xtalk_avg[1][0], ir_xtalk_avg[1][1],
				red_xtalk_avg[0], red_xtalk_avg[1][0], red_xtalk_avg[1][1],
				green_xtalk_avg[0], green_xtalk_avg[1][0], green_xtalk_avg[1][1],
				blue_xtalk_avg[0], blue_xtalk_avg[1][0], blue_xtalk_avg[1][1],
				ir_hdc_std[0], ir_hdc_std[1][0], ir_hdc_std[1][1],
				red_hdc_std[0], red_hdc_std[1][0], red_hdc_std[1][1],
				green_hdc_std[0], green_hdc_std[1][0], green_hdc_std[1][1],
				blue_hdc_std[0], blue_hdc_std[1][0], blue_hdc_std[1][1],
				eol_data->eol_freq_data.count);
		} else { /* MAX86917 */
			max86915_make_eol_item(eol_data);
			snprintf(device->eol_test_result, MAX_EOL_RESULT,
				"%s,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d,"
				"%s,%d,%d,%d,%d",
				eol_data->eol_item_data.test_type, EOL_ITEM_CNT,
				EOL_SYSTEM_NOISE,
				eol_data->eol_item_data.system_noise[EOL_PASS_FAIL],
				eol_data->eol_item_data.system_noise[EOL_MEASURE],
				eol_data->eol_item_data.system_noise[EOL_SPEC_MIN],
				eol_data->eol_item_data.system_noise[EOL_SPEC_MAX],
				EOL_IR_MID,
				eol_data->eol_item_data.ir_mid[EOL_PASS_FAIL],
				eol_data->eol_item_data.ir_mid[EOL_MEASURE],
				eol_data->eol_item_data.ir_mid[EOL_SPEC_MIN],
				eol_data->eol_item_data.ir_mid[EOL_SPEC_MAX],
				EOL_RED_MID,
				eol_data->eol_item_data.red_mid[EOL_PASS_FAIL],
				eol_data->eol_item_data.red_mid[EOL_MEASURE],
				eol_data->eol_item_data.red_mid[EOL_SPEC_MIN],
				eol_data->eol_item_data.red_mid[EOL_SPEC_MAX],
				EOL_GREEN_MID,
				eol_data->eol_item_data.green_mid[EOL_PASS_FAIL],
				eol_data->eol_item_data.green_mid[EOL_MEASURE],
				eol_data->eol_item_data.green_mid[EOL_SPEC_MIN],
				eol_data->eol_item_data.green_mid[EOL_SPEC_MAX],
				EOL_BLUE_MID,
				eol_data->eol_item_data.blue_mid[EOL_PASS_FAIL],
				eol_data->eol_item_data.blue_mid[EOL_MEASURE],
				eol_data->eol_item_data.blue_mid[EOL_SPEC_MIN],
				eol_data->eol_item_data.blue_mid[EOL_SPEC_MAX],
				EOL_IR_HIGH,
				eol_data->eol_item_data.ir_high[EOL_PASS_FAIL],
				eol_data->eol_item_data.ir_high[EOL_MEASURE],
				eol_data->eol_item_data.ir_high[EOL_SPEC_MIN],
				eol_data->eol_item_data.ir_high[EOL_SPEC_MAX],
				EOL_RED_HIGH,
				eol_data->eol_item_data.red_high[EOL_PASS_FAIL],
				eol_data->eol_item_data.red_high[EOL_MEASURE],
				eol_data->eol_item_data.red_high[EOL_SPEC_MIN],
				eol_data->eol_item_data.red_high[EOL_SPEC_MAX],
				EOL_GREEN_HIGH,
				eol_data->eol_item_data.green_high[EOL_PASS_FAIL],
				eol_data->eol_item_data.green_high[EOL_MEASURE],
				eol_data->eol_item_data.green_high[EOL_SPEC_MIN],
				eol_data->eol_item_data.green_high[EOL_SPEC_MAX],
				EOL_BLUE_HIGH,
				eol_data->eol_item_data.blue_high[EOL_PASS_FAIL],
				eol_data->eol_item_data.blue_high[EOL_MEASURE],
				eol_data->eol_item_data.blue_high[EOL_SPEC_MIN],
				eol_data->eol_item_data.blue_high[EOL_SPEC_MAX],
				EOL_IR_XTALK,
				eol_data->eol_item_data.ir_xtalk[EOL_PASS_FAIL],
				eol_data->eol_item_data.ir_xtalk[EOL_MEASURE],
				eol_data->eol_item_data.ir_xtalk[EOL_SPEC_MIN],
				eol_data->eol_item_data.ir_xtalk[EOL_SPEC_MAX],
				EOL_RED_XTALK,
				eol_data->eol_item_data.red_xtalk[EOL_PASS_FAIL],
				eol_data->eol_item_data.red_xtalk[EOL_MEASURE],
				eol_data->eol_item_data.red_xtalk[EOL_SPEC_MIN],
				eol_data->eol_item_data.red_xtalk[EOL_SPEC_MAX],
				EOL_GREEN_XTALK,
				eol_data->eol_item_data.green_xtalk[EOL_PASS_FAIL],
				eol_data->eol_item_data.green_xtalk[EOL_MEASURE],
				eol_data->eol_item_data.green_xtalk[EOL_SPEC_MIN],
				eol_data->eol_item_data.green_xtalk[EOL_SPEC_MAX],
				EOL_BLUE_XTALK,
				eol_data->eol_item_data.blue_xtalk[EOL_PASS_FAIL],
				eol_data->eol_item_data.blue_xtalk[EOL_MEASURE],
				eol_data->eol_item_data.blue_xtalk[EOL_SPEC_MIN],
				eol_data->eol_item_data.blue_xtalk[EOL_SPEC_MAX],
				EOL_IR_NOISE,
				eol_data->eol_item_data.ir_noise[EOL_PASS_FAIL],
				eol_data->eol_item_data.ir_noise[EOL_MEASURE],
				eol_data->eol_item_data.ir_noise[EOL_SPEC_MIN],
				eol_data->eol_item_data.ir_noise[EOL_SPEC_MAX],
				EOL_RED_NOISE,
				eol_data->eol_item_data.red_noise[EOL_PASS_FAIL],
				eol_data->eol_item_data.red_noise[EOL_MEASURE],
				eol_data->eol_item_data.red_noise[EOL_SPEC_MIN],
				eol_data->eol_item_data.red_noise[EOL_SPEC_MAX],
				EOL_GREEN_NOISE,
				eol_data->eol_item_data.green_noise[EOL_PASS_FAIL],
				eol_data->eol_item_data.green_noise[EOL_MEASURE],
				eol_data->eol_item_data.green_noise[EOL_SPEC_MIN],
				eol_data->eol_item_data.green_noise[EOL_SPEC_MAX],
				EOL_BLUE_NOISE,
				eol_data->eol_item_data.blue_noise[EOL_PASS_FAIL],
				eol_data->eol_item_data.blue_noise[EOL_MEASURE],
				eol_data->eol_item_data.blue_noise[EOL_SPEC_MIN],
				eol_data->eol_item_data.blue_noise[EOL_SPEC_MAX],
				EOL_FREQUENCY,
				eol_data->eol_item_data.frequency[EOL_PASS_FAIL],
				eol_data->eol_item_data.frequency[EOL_MEASURE],
				eol_data->eol_item_data.frequency[EOL_SPEC_MIN],
				eol_data->eol_item_data.frequency[EOL_SPEC_MAX]);
		}
	}
	HRM_dbg("%s - result : %s\n", __func__, device->eol_test_result);
}

static void max86915_eol_check_done(struct max86915_eol_data *eol_data, u8 mode, struct max86915_device_data *device)
{
	int i = 0;
	int start_time;
	int end_time;

	if (device->eol_test_result != NULL)
		kfree(device->eol_test_result);

	device->eol_test_result = kzalloc(sizeof(char) * MAX_EOL_RESULT, GFP_KERNEL);
	if (device->eol_test_result == NULL) {
		HRM_err("%s - couldn't eol_test_result allocate memory\n",
			__func__);
		return;
	}

	if ((eol_data->eol_flicker_data.done || (device->pre_eol_test_is_enable == 2))
		&& ((eol_data->eol_hrm_flag == EOL_NEW_HRM_COMPLETE_ALL_STEP) || (device->pre_eol_test_is_enable))
		&& (eol_data->eol_freq_data.done || (device->pre_eol_test_is_enable == 1))) {
		do_gettimeofday(&eol_data->end_time);
		start_time = (eol_data->start_time.tv_sec * 1000) + (eol_data->start_time.tv_usec / 1000);
		end_time = (eol_data->end_time.tv_sec * 1000) + (eol_data->end_time.tv_usec / 1000);
		HRM_dbg("%s - EOL Tested Time :  %d ms Tested Count : %d\n",
			__func__, end_time - start_time, device->sample_cnt);

		HRM_dbg("%s - SETTING CUREENT %x, %x\n", __func__,
			EOL_NEW_DC_HIGH_LED_IR_CURRENT, EOL_NEW_DC_HIGH_LED_RED_CURRENT);

		for (i = 0; i < EOL_NEW_HRM_ARRY_SIZE; i++)
			HRM_dbg("%s - DOUBLE CHECK STEP[%d], done : %d\n",
				__func__, i + 2, eol_data->eol_hrm_data[i].done);

		if (device->pre_eol_test_is_enable != 2) {
			for (i = 0; i < EOL_NEW_FLICKER_SIZE; i++)
				HRM_dbg("%s - EOL_NEW_FLICKER_MODE EOLRAW,%llu,%d\n",
					__func__, eol_data->eol_flicker_data.buf[SELF_IR_CH][i], 0);
		}

		if (device->pre_eol_test_is_enable == 0) {
			for (i = 0; i < EOL_NEW_DC_MIDDLE_SIZE; i++)
				HRM_dbg("%s - EOL_NEW_DC_MIDDLE_MODE EOLRAW,%llu,%llu,%llu,%llu\n", __func__,
					eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].buf[SELF_IR_CH][i],
					eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].buf[SELF_RED_CH][i],
					eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].buf[SELF_GREEN_CH][i],
					eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].buf[SELF_BLUE_CH][i]);

			for (i = 0; i < EOL_NEW_DC_HIGH_SIZE; i++)
				HRM_dbg("%s - EOL_NEW_DC_HIGH_MODE EOLRAW,%llu,%llu,%llu,%llu\n", __func__,
					eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].buf[SELF_IR_CH][i],
					eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].buf[SELF_RED_CH][i],
					eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].buf[SELF_GREEN_CH][i],
					eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].buf[SELF_BLUE_CH][i]);

			for (i = 0; i < EOL_NEW_DC_XTALK_SIZE; i++)
				HRM_dbg("%s - EOL_NEW_DC_XTALK EOLRAW,%llu,%llu,%llu,%llu\n", __func__,
					eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].buf[SELF_IR_CH][i],
					eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].buf[SELF_RED_CH][i],
					eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].buf[SELF_GREEN_CH][i],
					eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].buf[SELF_BLUE_CH][i]);

			HRM_dbg("%s - EOL_NEW_FLICKER_MODE RESULT,%llu,%llu\n", __func__,
				eol_data->eol_flicker_data.average[SELF_IR_CH],
				eol_data->eol_flicker_data.std_sum[SELF_IR_CH]);
			HRM_dbg("%s - EOL_NEW_DC_MIDDLE_MODE RESULT,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",	__func__,
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].average[SELF_IR_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].std_sum[SELF_IR_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].average[SELF_RED_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].std_sum[SELF_RED_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].average[SELF_GREEN_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].std_sum[SELF_GREEN_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].average[SELF_BLUE_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_MID].std_sum[SELF_BLUE_CH]);
			HRM_dbg("%s - EOL_NEW_DC_HIGH_MODE RESULT,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n", __func__,
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].average[SELF_IR_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum[SELF_IR_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].average[SELF_RED_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum[SELF_RED_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].average[SELF_GREEN_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum[SELF_GREEN_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].average[SELF_BLUE_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_HIGH].std_sum[SELF_BLUE_CH]);
			HRM_dbg("%s - EOL_NEW_DC_XTALK RESULT,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n", __func__,
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].average[SELF_IR_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].std_sum[SELF_IR_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].average[SELF_RED_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].std_sum[SELF_RED_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].average[SELF_GREEN_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].std_sum[SELF_GREEN_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].average[SELF_BLUE_CH],
				eol_data->eol_hrm_data[EOL_NEW_MODE_DC_XTALK].std_sum[SELF_BLUE_CH]);
		}

		if (device->pre_eol_test_is_enable != 1) {
			HRM_dbg("%s - EOL FREQ RESULT,%d\n", __func__,
				eol_data->eol_freq_data.count);
		}

		max86915_eol_conv2float(eol_data, mode, device);
		device->eol_test_status = 1;
		device->eol_data.state = _EOL_STATE_TYPE_NEW_STOP;
	}
	if (eol_data->eol_xtalk_data.done) {
		for (i = 0; i < EOL_XTALK_SIZE; i++)
			HRM_dbg("%s - EOL_XTALK_MODE EOLRAW,%lld\n",
				 __func__, eol_data->eol_xtalk_data.buf[i]);

		max86915_make_eol_item(eol_data);

		snprintf(device->eol_test_result, MAX_EOL_RESULT,
			"%s,%d,%s,%d,%d,%d,%d",
			eol_data->eol_item_data.test_type, 1,
			EOL_HRM_XTALK,
			eol_data->eol_item_data.xtalk[EOL_PASS_FAIL],
			eol_data->eol_item_data.xtalk[EOL_MEASURE],
			eol_data->eol_item_data.xtalk[EOL_SPEC_MIN],
			eol_data->eol_item_data.xtalk[EOL_SPEC_MAX]);

		HRM_dbg("%s result - %s\n", __func__, device->eol_test_result);
		device->eol_test_status = 1;
	}
}

static int max86915_enable_eol_flicker(struct max86915_device_data *data)
{
	int err;

	data->sample_cnt = 0;
	data->num_samples = 1;
	data->flex_mode = 1;

	err = max86915_init_status(data);

	/* 400Hz, LED_PW=400us, SPO2_ADC_RANGE=4096nA */
	err = max86915_write_reg(data,
		MAX86915_MODE_CONFIGURATION_2, 0x0F);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data,
		MAX86915_FIFO_CONFIG, 0x0C | MAX86915_FIFO_ROLLS_ON_MASK);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_FIFO_CONFIG!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data,
		MAX86915_INTERRUPT_ENABLE, A_FULL_MASK);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_INTERRUPT_ENABLE!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED_SEQ_REG_1,
			0x01);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_SEQ_REG_1!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data,
			MAX86915_MODE_CONFIGURATION, 0x07);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION!\n",
			__func__);
		return -EIO;
	}

	data->eol_data.state = _EOL_STATE_TYPE_NEW_FLICKER_INIT;
	return err;
}


static int max86915_enable_eol_dc(struct max86915_device_data *data)
{
	int err;
	u8 flex_config[2] = {0, };

	/* data->led = 1; */ /* Prevent resetting MAX86915_LED_CONFIGURATION */
	data->num_samples = 0;
	data->flex_mode = 0;

	flex_config[0] = (RED_LED_CH << MAX86915_LED2_OFFSET) | IR_LED_CH;
	flex_config[1] = (BLUE_LED_CH << MAX86915_LED4_OFFSET) | GREEN_LED_CH;

	if (flex_config[0] & MAX86915_LED1_MASK) {
		data->num_samples++;
		data->flex_mode |= (1 << 0);
	}
	if (flex_config[0] & MAX86915_LED2_MASK) {
		data->num_samples++;
		data->flex_mode |= (1 << 1);
	}
	if (flex_config[1] & MAX86915_LED3_MASK) {
		data->num_samples++;
		data->flex_mode |= (1 << 2);
	}
	if (flex_config[1] & MAX86915_LED4_MASK) {
		data->num_samples++;
		data->flex_mode |= (1 << 3);
	}

	HRM_info("%s - flexmode : 0x%02x, num_samples : %d\n", __func__,
			data->flex_mode, data->num_samples);

	err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION, 0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_LED1_PA,
			EOL_NEW_DC_MID_LED_IR_CURRENT);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED1_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED2_PA,
			EOL_NEW_DC_MID_LED_RED_CURRENT);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED2_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED3_PA,
			EOL_NEW_DC_MID_LED_GREEN_CURRENT);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED3_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED4_PA,
			EOL_NEW_DC_MID_LED_BLUE_CURRENT);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED4_PA!\n",
			__func__);
		return -EIO;
	}

	/* LED Range */
	err = max86915_write_reg(data, MAX86915_LED_RANGE,
		  (MAX86915_DEFAULT_LED_RGE1 << MAX86915_LED1_RGE_OFFSET)
		| (MAX86915_DEFAULT_LED_RGE2 << MAX86915_LED2_RGE_OFFSET)
		| (MAX86915_DEFAULT_LED_RGE3 << MAX86915_LED3_RGE_OFFSET)
		| (MAX86915_DEFAULT_LED_RGE4 << MAX86915_LED4_RGE_OFFSET));

	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_RANGE!\n",
			__func__);
		return -EIO;
	}

	/* XTALK */
	err = max86915_write_reg(data, MAX86915_DAC1_XTALK_CODE,
			0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC1_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_DAC2_XTALK_CODE,
			0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC2_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_DAC3_XTALK_CODE,
			0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC3_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_DAC4_XTALK_CODE,
			0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC4_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_INTERRUPT_ENABLE, A_FULL_MASK);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_INTERRUPT_ENABLE!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_LED_SEQ_REG_1,
			flex_config[0]);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_SEQ_REG_1!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED_SEQ_REG_2,
			flex_config[1]);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_SEQ_REG_2!\n",
			__func__);
		return -EIO;
	}

	/* 32uA, 400Hz, 120us */
	err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION_2,
			0x0D | (0x03 << MAX86915_ADC_RGE_OFFSET));
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_FIFO_CONFIG, 0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_FIFO_CONFIG!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION, 0x03);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION!\n",
			__func__);
		return -EIO;
	}

	data->eol_data.state = _EOL_STATE_TYPE_NEW_DC_MODE_MID;
	return 0;
}

static int max86915_enable_eol_freq(struct max86915_device_data *data)
{
	int err;
	u8 flex_config[2] = {0, };

	data->num_samples = 0;
	data->flex_mode = 0;

	flex_config[0] = (RED_LED_CH << MAX86915_LED2_OFFSET) | IR_LED_CH;
	flex_config[1] = (BLUE_LED_CH << MAX86915_LED4_OFFSET) | GREEN_LED_CH;

	if (flex_config[0] & MAX86915_LED1_MASK) {
		data->num_samples++;
		data->flex_mode |= (1 << 0);
	}
	if (flex_config[0] & MAX86915_LED2_MASK) {
		data->num_samples++;
		data->flex_mode |= (1 << 1);
	}
	if (flex_config[1] & MAX86915_LED3_MASK) {
		data->num_samples++;
		data->flex_mode |= (1 << 2);
	}
	if (flex_config[1] & MAX86915_LED4_MASK) {
		data->num_samples++;
		data->flex_mode |= (1 << 3);
	}

	HRM_info("%s - flexmode : 0x%02x, num_samples : %d\n", __func__,
			data->flex_mode, data->num_samples);

	err = max86915_init_status(data);
	if (err < 0)
		HRM_err("%s - failed to init_status\n", __func__);

	err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION, 0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_LED_SEQ_REG_1,
			flex_config[0]);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_SEQ_REG_1!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED_SEQ_REG_2,
			flex_config[1]);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_SEQ_REG_2!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_LED1_PA,
			EOL_NEW_FREQUENCY_LED_IR_CURRENT);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED1_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED2_PA,
			EOL_NEW_FREQUENCY_LED_RED_CURRENT);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED2_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED3_PA,
			EOL_NEW_FREQUENCY_LED_GREEN_CURRENT);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED3_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED4_PA,
			EOL_NEW_FREQUENCY_LED_BLUE_CURRENT);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED4_PA!\n",
			__func__);
		return -EIO;
	}

	/* LED Range */
	err = max86915_write_reg(data, MAX86915_LED_RANGE,
		  (MAX86915_DEFAULT_LED_RGE1 << MAX86915_LED1_RGE_OFFSET)
		| (MAX86915_DEFAULT_LED_RGE2 << MAX86915_LED2_RGE_OFFSET)
		| (MAX86915_DEFAULT_LED_RGE3 << MAX86915_LED3_RGE_OFFSET)
		| (MAX86915_DEFAULT_LED_RGE4 << MAX86915_LED4_RGE_OFFSET));

	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_RANGE!\n",
			__func__);
		return -EIO;
	}

	/* XTALK */
	err = max86915_write_reg(data, MAX86915_DAC1_XTALK_CODE,
			0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC1_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_DAC2_XTALK_CODE,
			0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC2_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_DAC3_XTALK_CODE,
			0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC3_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_DAC4_XTALK_CODE,
			0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC4_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}

	/* 32uA, 400Hz, 120us */
	err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION_2,
			0x0D | (0x03 << MAX86915_ADC_RGE_OFFSET)); /* Maximum PW is 100us for 4ch/400Hz */
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
			__func__);
		return -EIO;
	}

	/* AVERAGE 4 */
	err = max86915_write_reg(data, MAX86915_FIFO_CONFIG,
			(0x02 << MAX86915_SMP_AVE_OFFSET) & MAX86915_SMP_AVE_MASK);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_FIFO_CONFIG!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_INTERRUPT_ENABLE, PPG_RDY_MASK);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_INTERRUPT_ENABLE!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION, 0x03);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION!\n",
			__func__);
		return -EIO;
	}

	/* set the mode */
	data->eol_data.state = _EOL_STATE_TYPE_NEW_FREQ_MODE;
	return err;
}

static int max86915_get_num_samples_in_fifo(struct max86915_device_data *data)
{
	int fifo_rd_ptr;
	int fifo_wr_ptr;
	int fifo_ovf_cnt;
	char fifo_data[3];
	int num_samples = 0;
	int ret;

	ret = max86915_read_reg(data, MAX86915_FIFO_WRITE_POINTER, fifo_data, 3);
	if (ret < 0) {
		HRM_err("%s - read reg err:%d\n", __func__, ret);
		return ret;
	}

	fifo_wr_ptr = fifo_data[0] & 0x1F;
	fifo_ovf_cnt = fifo_data[1] & 0x1F;
	fifo_rd_ptr = fifo_data[2] & 0x1F;

	if (fifo_ovf_cnt || fifo_rd_ptr == fifo_wr_ptr) {
		if (fifo_ovf_cnt > 0)
			HRM_err("%s - FIFO is Full. OVF: %d RD:%d WR:%d\n",
					__func__, fifo_ovf_cnt, fifo_rd_ptr, fifo_wr_ptr);
		num_samples = MAX86915_FIFO_SIZE;
	} else {
		if (fifo_wr_ptr > fifo_rd_ptr)
			num_samples = fifo_wr_ptr - fifo_rd_ptr;
		else if (fifo_wr_ptr < fifo_rd_ptr)
			num_samples = MAX86915_FIFO_SIZE +
						fifo_wr_ptr - fifo_rd_ptr;
	}
	return num_samples;
}

static int max86915_get_fifo_settings(struct max86915_device_data *data, u16 *ch)
{
	int num_item = 0;
	int i;
	u16 tmp;
	int ret;
	u8 buf[2];

	/*
	 * TODO: Somehow when the data corrupted in the bus,
	 * and i2c functions don't return error messages.
	 */

	ret = max86915_read_reg(data, MAX86915_LED_SEQ_REG_1, buf, 2);
	if (ret < 0)
		return ret;

	tmp = ((u16)buf[1] << 8) | (u16)buf[0];
	*ch = tmp;
	for (i = 0; i < 4; i++) {
		if (tmp & 0x000F)
			num_item++;
		tmp >>= 4;
	}

	return num_item;
}

static int max86915_read_fifo(struct max86915_device_data *sd, char *fifo_buf, int num_bytes)
{
	int ret = 0;
	int data_offset = 0;
	int to_read = 0;

	while (num_bytes > 0) {
		if (num_bytes > MAX_I2C_BLOCK_SIZE) {
			to_read = MAX_I2C_BLOCK_SIZE;
			num_bytes -= to_read;
		} else {
			to_read = num_bytes;
			num_bytes = 0;
		}
		ret = max86915_read_reg(sd, MAX86915_FIFO_DATA, &fifo_buf[data_offset], to_read);

		if (ret < 0)
			break;

		data_offset += to_read;
	}
	return ret;
}

static int max86915_fifo_read_data(struct max86915_device_data *data)
{
	int ret = 0;
	int num_samples;
	int num_bytes;
	int num_channel = 0;
	u16 fd_settings = 0;
	int i;
	int j;
	int offset1;
	int offset2;
	int index;
	int samples[16] = {0, };
	u8 *fifo_buf = data->fifo_buf;
	int fifo_mode = -1;

	if (data->enabled_mode == MODE_SDK_IR) {
		num_samples = fifo_full_cnt;
		num_channel = 4;
	} else if (data->eol_test_is_enable == 1) {
		num_samples = 20;
		num_channel = data->num_samples;
	} else {
		num_samples = max86915_get_num_samples_in_fifo(data);
		if (num_samples <= 0 || num_samples > MAX86915_FIFO_SIZE) {
			ret = num_samples;
			goto fail;
		}
		num_channel = max86915_get_fifo_settings(data, &fd_settings);
		if (num_channel < 0) {
			ret = num_channel;
			goto fail;
		}
	}

	num_bytes = num_channel * num_samples * NUM_BYTES_PER_SAMPLE;
	ret = max86915_read_fifo(data, fifo_buf, num_bytes);
	if (ret < 0)
		goto fail;

	/* clear the fifo_data buffer */
	for (i = 0; i < MAX86915_FIFO_SIZE; i++)
		for (j = 0; j < MAX_LED_NUM; j++)
			data->fifo_data[j][i] = 0;

	data->fifo_samples = 0;

	for (i = 0; i < num_samples; i++) {
		offset1 = i * NUM_BYTES_PER_SAMPLE * num_channel;
		offset2 = 0;

		for (j = 0; j < MAX_LED_NUM; j++) {
			if (data->flex_mode & (1 << j)) {
				index = offset1 + offset2;
				samples[j] = ((int)fifo_buf[index + 0] << 16)
						| ((int)fifo_buf[index + 1] << 8)
						| ((int)fifo_buf[index + 2]);
				samples[j] &= 0x7ffff;
				data->fifo_data[j][i] = samples[j];
				offset2 += NUM_BYTES_PER_SAMPLE;
			}
		}
	}
	data->fifo_samples = num_samples;

	return ret;
fail:
	HRM_err("%s - failed, ret: %d, fifo_mode: %d\n", __func__, ret, fifo_mode);
	return ret;
}

static int max86915_fifo_irq_handler(struct max86915_device_data *data)
{
	int ret;

	ret = max86915_fifo_read_data(data);

	if (ret < 0) {
		HRM_dbg("%s fifo. ret: %d\n", __func__, ret);
		return ret;
	}

	return ret;
}

static int max86915_eol_read_data(struct output_data *data, struct max86915_device_data *device, int *raw_data)
{
	int err = 0;
	int i = 0, j = 0;
	int total_eol_cnt = (330 + (400 * 20)); /* maximnum count */
	u8 recvData[MAX_LED_NUM * NUM_BYTES_PER_SAMPLE] = { 0x00, };

	switch (device->eol_data.state) {
	case _EOL_STATE_TYPE_NEW_FREQ_MODE:
	case _EOL_STATE_TYPE_NEW_END:
	case _EOL_STATE_TYPE_NEW_STOP:
	case _EOL_STATE_TYPE_NEW_XTALK_MODE:
		err = max86915_read_reg(device, MAX86915_FIFO_DATA, recvData,
				device->num_samples * NUM_BYTES_PER_SAMPLE);
		if (err != 0) {
			HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
				__func__, err, recvData[0]);
			return -EIO;
		}
		for (i = 0; i < MAX_LED_NUM; i++) {
			if (device->flex_mode & (1 << i)) {
				raw_data[i] =  recvData[j++] << 16 & 0x30000;
				raw_data[i] += recvData[j++] << 8;
				raw_data[i] += recvData[j++] << 0;
			} else
				raw_data[i] = 0;
		}
		device->sample_cnt++;
		device->eol_data.eol_count++;
		break;
	default:
		err = max86915_fifo_irq_handler(device);
		if (err != 0) {
			HRM_err("%s - max86915_fifo_irq_handler err:%d\n",
				__func__, err);
			return -EIO;
		}
		data->fifo_num = device->fifo_samples;
		device->sample_cnt += device->fifo_samples;
		device->eol_data.eol_count += device->fifo_samples;
		break;
	}

	switch (device->eol_data.state) {
	case _EOL_STATE_TYPE_NEW_INIT:
		HRM_dbg("%s - EOL_STATE_TYPE_NEW_INIT\n", __func__);
		data->fifo_num = 0; /* doesn't report the data */
		err = 1;
		if (device->eol_data.eol_count >= EOL_NEW_START_SKIP_CNT) {
			device->eol_data.state = _EOL_STATE_TYPE_NEW_FLICKER_INIT;
			device->eol_data.eol_count = 0;
		}
		break;
	case _EOL_STATE_TYPE_NEW_FLICKER_INIT:
		for (i = 0; i < device->fifo_samples; i++) {
			data->fifo_main[AGC_IR][i] = device->fifo_data[AGC_IR][i];
			data->fifo_main[AGC_RED][i] = 0;
			device->eol_data.eol_flicker_data.max = 0;
		}

		if (device->eol_data.eol_count >= EOL_NEW_START_SKIP_CNT) {
			HRM_dbg("%s - FINISH : _EOL_STATE_TYPE_NEW_FLICKER_INIT\n", __func__);
			device->eol_data.state = _EOL_STATE_TYPE_NEW_FLICKER_MODE;
			device->eol_data.eol_count = 0;
		}
		break;
	case _EOL_STATE_TYPE_NEW_FLICKER_MODE:
		for (i = 0; i < device->fifo_samples; i++) {
			device->fifo_data[AGC_IR][i] = device->fifo_data[AGC_IR][i] >> 3;
			data->fifo_main[AGC_IR][i] = device->fifo_data[AGC_IR][i];
			data->fifo_main[AGC_RED][i] = 0;
			max86915_eol_flicker_data(data->fifo_main[AGC_IR][i], &device->eol_data);
		}
		if (device->eol_data.eol_flicker_data.done == EOL_SUCCESS) {
			HRM_dbg("%s - FINISH : _EOL_STATE_TYPE_NEW_FLICKER_MODE : %d, %d\n", __func__,
				device->eol_data.eol_flicker_data.index, device->eol_data.eol_flicker_data.done);

			if (device->pre_eol_test_is_enable == 3) /* PRE EOL for both */
				max86915_enable_eol_freq(device);
			else if (device->pre_eol_test_is_enable == 1) /* Finish PRE EOL */
				max86915_eol_check_done(&device->eol_data, SELF_MODE_400HZ, device);
			else
				max86915_enable_eol_dc(device);

			device->eol_data.eol_count = 0;

			/* clear the remained datasheet after changing mode (interrupt TRIGGER mode). */
			err = max86915_fifo_irq_handler(device);
			if (err != 0) {
				HRM_err("%s - max86915_fifo_irq_handler err:%d\n",
					__func__, err);
				return -EIO;
			}
		}
		break;
	case _EOL_STATE_TYPE_NEW_DC_MODE_MID:
		for (i = 0; i < device->fifo_samples; i++) {
			data->fifo_main[AGC_IR][i] = device->fifo_data[AGC_IR][i];
			data->fifo_main[AGC_RED][i] = device->fifo_data[AGC_RED][i];
			data->fifo_main[AGC_GREEN][i] = device->fifo_data[AGC_GREEN][i];
			data->fifo_main[AGC_BLUE][i] = device->fifo_data[AGC_BLUE][i];
			max86915_eol_hrm_data(data->fifo_main[AGC_IR][i], data->fifo_main[AGC_RED][i],
				data->fifo_main[AGC_GREEN][i], data->fifo_main[AGC_BLUE][i], &device->eol_data,
				EOL_NEW_MODE_DC_MID, EOL_NEW_DC_MIDDLE_SKIP_CNT, EOL_NEW_DC_MIDDLE_SIZE);
		}
		if (device->eol_data.eol_hrm_data[EOL_NEW_MODE_DC_MID].done == EOL_SUCCESS) {
			HRM_dbg("%s - FINISH : _EOL_STATE_TYPE_NEW_DC_MODE_MID\n", __func__);

			err = max86915_set_current(EOL_NEW_DC_HIGH_LED_IR_CURRENT, EOL_NEW_DC_HIGH_LED_RED_CURRENT,
				EOL_NEW_DC_HIGH_LED_GREEN_CURRENT, EOL_NEW_DC_HIGH_LED_BLUE_CURRENT);

			device->eol_data.state = _EOL_STATE_TYPE_NEW_DC_MODE_HIGH;
			device->eol_data.eol_count = 0;
		}
		break;
	case _EOL_STATE_TYPE_NEW_DC_MODE_HIGH:
		for (i = 0; i < device->fifo_samples; i++) {
			data->fifo_main[AGC_IR][i] = device->fifo_data[AGC_IR][i];
			data->fifo_main[AGC_RED][i] = device->fifo_data[AGC_RED][i];
			data->fifo_main[AGC_GREEN][i] = device->fifo_data[AGC_GREEN][i];
			data->fifo_main[AGC_BLUE][i] = device->fifo_data[AGC_BLUE][i];
			max86915_eol_hrm_data(data->fifo_main[AGC_IR][i], data->fifo_main[AGC_RED][i],
				data->fifo_main[AGC_GREEN][i], data->fifo_main[AGC_BLUE][i], &device->eol_data,
				EOL_NEW_MODE_DC_HIGH, EOL_NEW_DC_HIGH_SKIP_CNT, EOL_NEW_DC_HIGH_SIZE);
		}

		if (device->eol_data.eol_hrm_data[EOL_NEW_MODE_DC_HIGH].done == EOL_SUCCESS) {
			HRM_dbg("%s - FINISH : _EOL_STATE_TYPE_NEW_DC_MODE_HIGH\n", __func__);

			err = max86915_write_reg(device, MAX86915_DAC1_XTALK_CODE,
					0x1f);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_DAC1_XTALK_CODE!\n",
					__func__);
				return -EIO;
			}
			err = max86915_write_reg(device, MAX86915_DAC2_XTALK_CODE,
					0x1f);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_DAC2_XTALK_CODE!\n",
					__func__);
				return -EIO;
			}
			err = max86915_write_reg(device, MAX86915_DAC3_XTALK_CODE,
					0x1f);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_DAC3_XTALK_CODE!\n",
					__func__);
				return -EIO;
			}
			err = max86915_write_reg(device, MAX86915_DAC4_XTALK_CODE,
					0x1f);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_DAC4_XTALK_CODE!\n",
					__func__);
				return -EIO;
			}

			device->eol_data.state = _EOL_STATE_TYPE_NEW_DC_XTALK;
			device->eol_data.eol_count = 0;
		}
		break;
	case _EOL_STATE_TYPE_NEW_DC_XTALK:
		for (i = 0; i < device->fifo_samples; i++) {
			data->fifo_main[AGC_IR][i] = device->fifo_data[AGC_IR][i];
			data->fifo_main[AGC_RED][i] = device->fifo_data[AGC_RED][i];
			data->fifo_main[AGC_GREEN][i] = device->fifo_data[AGC_GREEN][i];
			data->fifo_main[AGC_BLUE][i] = device->fifo_data[AGC_BLUE][i];
			max86915_eol_hrm_data(data->fifo_main[AGC_IR][i], data->fifo_main[AGC_RED][i],
				data->fifo_main[AGC_GREEN][i], data->fifo_main[AGC_BLUE][i], &device->eol_data,
				EOL_NEW_MODE_DC_XTALK, EOL_NEW_DC_XTALK_SKIP_CNT, EOL_NEW_DC_XTALK_SIZE);
		}

		if (device->eol_data.eol_hrm_data[EOL_NEW_MODE_DC_XTALK].done == EOL_SUCCESS) {
			HRM_dbg("%s - FINISH : _EOL_STATE_TYPE_NEW_DC_XTALK\n", __func__);

			max86915_enable_eol_freq(device);
			device->eol_data.eol_count = 0;
		}
		break;

	case _EOL_STATE_TYPE_NEW_FREQ_MODE:
		if (device->eol_data.eol_freq_data.done != EOL_SUCCESS) {
			max86915_eol_freq_data(&device->eol_data, SELF_DIVID_100HZ);
			max86915_eol_check_done(&device->eol_data, SELF_MODE_400HZ, device);
		}
		if (!(device->eol_test_status) && device->sample_cnt >= total_eol_cnt)
			device->eol_data.state = _EOL_STATE_TYPE_NEW_END;
		break;
	case _EOL_STATE_TYPE_NEW_END:
		HRM_err("%s - FAIL EOL TEST  : HR_STATE: _EOL_STATE_TYPE_NEW_END !!!\n", __func__);
		max86915_eol_check_done(&device->eol_data, SELF_MODE_400HZ, device);
		device->eol_data.state = _EOL_STATE_TYPE_NEW_STOP;
		break;
	case _EOL_STATE_TYPE_NEW_STOP:
		max86915_eol_set_mode(device, PWR_OFF);
		device->pre_eol_test_is_enable = 0;
		device->enabled_mode = MODE_NONE;
		break;
	case _EOL_STATE_TYPE_NEW_XTALK_MODE:
		max86915_eol_xtalk_data(raw_data[0], &device->eol_data);
	
		if (device->eol_data.eol_xtalk_data.done == EOL_SUCCESS) {
			HRM_dbg("%s FINISH : _EOL_STATE_TYPE_XTALK_MODE\n", __func__);
			max86915_eol_check_done(&device->eol_data, SELF_MODE_400HZ, device);
			device->eol_data.state = _EOL_STATE_TYPE_NEW_STOP;
			device->eol_data.eol_count = 0;
		}

		break;
	default:
		break;
	}
	return err;
}

static int max86915_hrm_read_data(struct max86915_device_data *device, int *data)
{
	int err;
	u8 recvData[MAX_LED_NUM * NUM_BYTES_PER_SAMPLE] = { 0x00, };
	int i, j = 0;
	int ret = 0;

	if (device->sample_cnt == MAX86915_COUNT_MAX)
		device->sample_cnt = 0;

	err = max86915_read_reg(device, MAX86915_FIFO_DATA, recvData,
			device->num_samples * NUM_BYTES_PER_SAMPLE);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData[0]);
		return -EIO;
	}

	for (i = 0; i < MAX_LED_NUM; i++)	{
		if (device->flex_mode & (1 << i)) {
			data[i] = recvData[j++] << 16 & 0x70000;
			data[i] += recvData[j++] << 8;
			data[i] += recvData[j++] << 0;
		} else
			data[i] = 0;
	}

	if ((device->sample_cnt % 1000) == 0)
		HRM_info("%s - %u, %u, %u, %u\n", __func__,
			data[0], data[1], data[2], data[3]);

	if (device->agc_led_set == 0) {
		device->agc_ch_adc[0] = data[0];
		device->agc_ch_adc[1] = data[1];
		device->agc_ch_adc[2] = data[2];
		device->agc_ch_adc[3] = data[3];
		device->sample_cnt++;
	} else {
		data[0] = device->agc_ch_adc[0];
		data[1] = device->agc_ch_adc[1];
		data[2] = device->agc_ch_adc[2];
		data[3] = device->agc_ch_adc[3];
	}

	return ret;
}

static int max86915_awb_flicker_read_data(struct max86915_device_data *device, int *data)
{
	int err;
	u8 *recvData = device->fifo_buf;
	int ret = 0;
	int mode_changed = 0;
	int i;
	u8 irq_status = 0;
	int previous_awb_data = 0;

	err = max86915_read_reg(device, MAX86915_INTERRUPT_STATUS, recvData, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData[0]);
		return -EIO;
	}
	irq_status = recvData[0];
	if (irq_status != 0x80) {
		HRM_err("%s - irq_status 0x%x, return 1\n", __func__, irq_status);
		return 1;
	}

	err = max86915_read_reg(device, MAX86915_FIFO_DATA, recvData,
		AWB_INTERVAL * NUM_BYTES_PER_SAMPLE);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData[0]);
		return -EIO;
	}

	*data = (recvData[0 + (AWB_INTERVAL - 1)*NUM_BYTES_PER_SAMPLE] << 16 & 0x70000)
		+ (recvData[1 + (AWB_INTERVAL - 1)*NUM_BYTES_PER_SAMPLE] << 8)
		+ (recvData[2 + (AWB_INTERVAL - 1)*NUM_BYTES_PER_SAMPLE] << 0);

	previous_awb_data = (recvData[0 + (AWB_INTERVAL - 2)*NUM_BYTES_PER_SAMPLE] << 16 & 0x70000)
		+ (recvData[1 + (AWB_INTERVAL - 2)*NUM_BYTES_PER_SAMPLE] << 8)
		+ (recvData[2 + (AWB_INTERVAL - 2)*NUM_BYTES_PER_SAMPLE] << 0);

	if (device->awb_sample_cnt > CONFIG_SKIP_CNT) {
		mutex_lock(&device->flickerdatalock);
		for (i = 0; i < AWB_INTERVAL; i++) {
			if (device->flicker_data_cnt < FLICKER_DATA_CNT) {
				device->flicker_data[device->flicker_data_cnt++] =
					(recvData[0 + i*NUM_BYTES_PER_SAMPLE] << 16 & 0x70000)
					+ (recvData[1 + i*NUM_BYTES_PER_SAMPLE] << 8)
					+ (recvData[2 + i*NUM_BYTES_PER_SAMPLE] << 0);
			}
		}
		mutex_unlock(&device->flickerdatalock);
	}

	/* Change Configuation */
	if (device->awb_flicker_status == AWB_CONFIG1) {
		if (*data > AWB_MAX86915_CONFIG_TH2
			&& previous_awb_data > AWB_MAX86915_CONFIG_TH2) { /* Change to AWB_CONFIG2 */
			err = max86915_write_reg(device, MAX86915_MODE_CONFIGURATION, 0x00);
			/* 16384 uA setting */
			err = max86915_write_reg(device,
				MAX86915_MODE_CONFIGURATION_2, 0x6F);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
					__func__);
				return -EIO;
			}
			device->awb_flicker_status = AWB_CONFIG2;
			mode_changed = 1;
		}
	} else if (device->awb_flicker_status == AWB_CONFIG2) {
		if (*data < AWB_MAX86915_CONFIG_TH3
			&& previous_awb_data < AWB_MAX86915_CONFIG_TH3) { /* Change to AWB_CONFIG1 */
			err = max86915_write_reg(device, MAX86915_MODE_CONFIGURATION, 0x00);
			/* 4096 uA setting */
			err = max86915_write_reg(device,
				MAX86915_MODE_CONFIGURATION_2, 0x0F);
			if (err != 0) {
				HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
					__func__);
				return -EIO;
			}
			device->awb_flicker_status = AWB_CONFIG1;
			mode_changed = 1;
		}
	}

	if (device->awb_sample_cnt > CONFIG_SKIP_CNT) {
		if (device->awb_flicker_status < AWB_CONFIG2)
			*data = *data >> 3; /* 2uA setting should devided by 8 */
		ret = 0;
	} else {
		ret = 1;
		HRM_dbg("%s - awb sample cnt %d, less than CONFIG SKIP CNT\n", __func__,
			device->awb_sample_cnt);
	}

	if (mode_changed == 1) {
		/* Flush Buffer */
		err = max86915_write_reg(device,
			MAX86915_MODE_CONFIGURATION, 0x07);
		if (err != 0) {
			HRM_err("%s - error init MAX86915_MODE_CONFIGURATION!\n",
				__func__);
			return -EIO;
		}
		device->flicker_data_cnt = 0;
		device->awb_sample_cnt = 0;
		ret = 1;
		HRM_dbg("%s - mode changed to : %d\n", __func__, device->awb_flicker_status);
	} else
		device->awb_sample_cnt += AWB_INTERVAL;

	return ret;
}

static long max86915_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret = 0;

	struct max86915_device_data *data = container_of(file->private_data,
	struct max86915_device_data, miscdev);

	HRM_info("%s - ioctl start\n", __func__);
	mutex_lock(&data->flickerdatalock);

	switch (cmd) {
	case MAX86915_IOCTL_READ_FLICKER:
		ret = copy_to_user(argp,
			data->flicker_data,
			sizeof(int)*FLICKER_DATA_CNT);

		if (unlikely(ret))
			goto ioctl_error;

		break;

	default:
		HRM_err("%s - invalid cmd\n", __func__);
		break;
	}

	mutex_unlock(&data->flickerdatalock);
	return ret;

ioctl_error:
	mutex_unlock(&data->flickerdatalock);
	HRM_err("%s - read flicker data err(%d)\n", __func__, ret);
	return -ret;
}

static const struct file_operations max86915_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.unlocked_ioctl = max86915_ioctl,
};

static int max86915_read_data(struct output_data *data)
{
	int err = 0;
	int ret;
	int raw_data[4] = {0x00, };
	u8 recvData;
	int i = 0;
	int j = 0;
	int data_fifo[MAX_LED_NUM][MAX86915_FIFO_SIZE];

	data->sub_num = 0;
	data->fifo_num = 0;

	for (i = 0; i < MAX86915_FIFO_SIZE; i++) {
		for (j = 0; j < MAX_LED_NUM; j++) {
			data_fifo[j][i] = 0;
			data->fifo_main[j][i] = 0;
		}
	}

	switch (max86915_data->enabled_mode) {
	case  MODE_HRM:
#ifndef ENABLE_POLL_DELAY
	case MODE_SDK_IR:
#endif
	case  MODE_PROX:
		if (max86915_data->eol_test_is_enable)
			err = max86915_eol_read_data(data, max86915_data, raw_data);
		else
			err = max86915_hrm_read_data(max86915_data, raw_data);

		break;
	case MODE_AMBIENT:
		err = max86915_awb_flicker_read_data(max86915_data, raw_data);
		break;
#ifdef ENABLE_POLL_DELAY
	case MODE_SDK_IR:
		err = max86915_fifo_irq_handler(max86915_data);
#ifdef CONFIG_4CH_IOCTL
		mutex_lock(&max86915_data->flickerdatalock);

		for (i = 0; i < max86915_data->fifo_samples; i++) {
			if (max86915_data->agc_led_set > 0) {
				max86915_data->flicker_data[i*4+0] = max86915_data->agc_ch_adc[0];
				max86915_data->flicker_data[i*4+1] = max86915_data->agc_ch_adc[1];
				max86915_data->flicker_data[i*4+2] = max86915_data->agc_ch_adc[2];
				max86915_data->flicker_data[i*4+3] = max86915_data->agc_ch_adc[3];
			} else {
				max86915_data->flicker_data[i*4+0] = max86915_data->fifo_data[0][i];
				max86915_data->flicker_data[i*4+1] = max86915_data->fifo_data[1][i];
				max86915_data->flicker_data[i*4+2] = max86915_data->fifo_data[2][i];
				max86915_data->flicker_data[i*4+3] = max86915_data->fifo_data[3][i];

				max86915_data->agc_ch_adc[0] = max86915_data->fifo_data[0][max86915_data->fifo_samples-1];
				max86915_data->agc_ch_adc[1] = max86915_data->fifo_data[1][max86915_data->fifo_samples-1];
				max86915_data->agc_ch_adc[2] = max86915_data->fifo_data[2][max86915_data->fifo_samples-1];
				max86915_data->agc_ch_adc[3] = max86915_data->fifo_data[3][max86915_data->fifo_samples-1];
			}
		}

		mutex_unlock(&max86915_data->flickerdatalock);
#endif
#ifdef CONFIG_4CH_INPUT
		for (i = 0; i < max86915_data->fifo_samples; i++) {
			if (max86915_data->agc_led_set > 0) {
				data->fifo_main[AGC_IR][i] = max86915_data->agc_ch_adc[0];
				data->fifo_main[AGC_RED][i] = max86915_data->agc_ch_adc[1];
				data->fifo_main[AGC_GREEN][i] = max86915_data->agc_ch_adc[2];
				data->fifo_main[AGC_BLUE][i] = max86915_data->agc_ch_adc[3];
			} else {
				data->fifo_main[AGC_IR][i] = max86915_data->fifo_data[0][i];
				data->fifo_main[AGC_RED][i] = max86915_data->fifo_data[1][i];
				data->fifo_main[AGC_GREEN][i] = max86915_data->fifo_data[2][i];
				data->fifo_main[AGC_BLUE][i] = max86915_data->fifo_data[3][i];

				max86915_data->agc_ch_adc[0] = max86915_data->fifo_data[0][max86915_data->fifo_samples-1];
				max86915_data->agc_ch_adc[1] = max86915_data->fifo_data[1][max86915_data->fifo_samples-1];
				max86915_data->agc_ch_adc[2] = max86915_data->fifo_data[2][max86915_data->fifo_samples-1];
				max86915_data->agc_ch_adc[3] = max86915_data->fifo_data[3][max86915_data->fifo_samples-1];
			}
		}
		data->fifo_num = fifo_full_cnt;
#endif
		if (max86915_data->agc_led_set == 0) /* for AGC */
			max86915_data->sample_cnt += max86915_data->fifo_samples;
		break;
#endif

	default:
		err = 1; /* error case */
		break;
	}

	if (err < 0)
		HRM_err("%s - read_data err : %d\n", __func__, err);

	if (err == 0) {
		data->main_num = 2;
		if (max86915_data->enabled_mode == MODE_AMBIENT) {
			data->data_main[0] = raw_data[0];
			if (max86915_data->flicker_data_cnt == FLICKER_DATA_CNT) {
				data->data_main[1] = -2;
				max86915_data->flicker_data_cnt = 0;
			} else {
				data->data_main[1] = 0;
			}
#ifndef ENABLE_POLL_DELAY
		} else if (max86915_data->enabled_mode == MODE_SDK_IR) {
			data->main_num = 4;
			data->data_main[0] = raw_data[0];
			data->data_main[1] = raw_data[1];
			data->data_main[2] = raw_data[2];
			data->data_main[3] = raw_data[3];
#else
		} else if (max86915_data->enabled_mode == MODE_SDK_IR) {
#ifdef CONFIG_4CH_IOCTL
			data->data_main[1] = -3;
#endif
#ifdef CONFIG_4CH_INPUT
			data->data_main[1] = -3;
			raw_data[0] = data->fifo_main[AGC_IR][0];
			raw_data[1] = data->fifo_main[AGC_RED][0];
			raw_data[2] = data->fifo_main[AGC_GREEN][0];
			raw_data[3] = data->fifo_main[AGC_BLUE][0];
#endif
#endif
		} else if (max86915_data->enabled_mode == MODE_HRM) {
			data->main_num = 2;
			data->data_main[0] = raw_data[0];
			data->data_main[1] = raw_data[1];
#ifdef CONFIG_HRM_GREEN_BLUE
			data->main_num = 4;
			data->data_main[2] = raw_data[2];
			data->data_main[3] = raw_data[3];
#endif
		} else {
			data->main_num = 2;
			data->data_main[0] = raw_data[0];
			data->data_main[1] = raw_data[1];
		}

		data->mode = max86915_data->enabled_mode;
		ret = 0;
	} else
		ret = 1;

	/* Interrupt Clear */
	err = max86915_read_reg(max86915_data, MAX86915_INTERRUPT_STATUS, &recvData, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData);
		return -EIO;
	}

	if (max86915_data->agc_enabled == 0 && max86915_data->enabled_mode != MODE_AMBIENT) {
		if (max86915_data->sample_cnt > AGC_SKIP_CNT)
			max86915_data->agc_enabled = 1;
		else {
			data->main_num = 0;
			data->fifo_num = 0;
		}
	}

	if (max86915_data->agc_mode != M_NONE
		&& agc_debug_enabled
		&& max86915_data->agc_enabled
		&& (max86915_data->agc_led_set == 0)) {
		max86915_cal_agc(raw_data[0], raw_data[1], raw_data[2], raw_data[3]);
	}

	if (max86915_data->agc_led_set > 0) {
		if (max86915_data->enabled_mode == MODE_SDK_IR) {
			if (max86915_data->agc_led_set < fifo_full_cnt)
				max86915_data->agc_led_set = 0;
			else
				max86915_data->agc_led_set -= fifo_full_cnt;
		} else {
			max86915_data->agc_led_set--;
		}
	}

	return ret;
}

static int max86915_init_hrm_reg(struct max86915_device_data *data)
{
	int err, i;
	u8 flex_config[2] = {0, };

	data->sample_cnt = 0;
	data->agc_led_set = 0;

	data->agc_ch_adc[0] = 0;
	data->agc_ch_adc[1] = 0;
	data->agc_ch_adc[2] = 0;
	data->agc_ch_adc[3] = 0;

	flex_config[0] = (RED_LED_CH << MAX86915_LED2_OFFSET) | IR_LED_CH;
	flex_config[1] = 0x00;
#ifdef CONFIG_HRM_GREEN_BLUE
	flex_config[1] = (BLUE_LED_CH << MAX86915_LED4_OFFSET) | GREEN_LED_CH;
#endif

	data->num_samples = 2;
	data->flex_mode = 3;
#ifdef CONFIG_HRM_GREEN_BLUE
	data->num_samples = 4;
	data->flex_mode = 0xf;
#endif

	data->led_current1 = data->init_current[AGC_IR];
	data->led_current2 = MAX86915_HRM_DEFAULT_CURRENT2;
#ifdef CONFIG_HRM_GREEN_BLUE
	data->led_current3 = MAX86915_HRM_DEFAULT_CURRENT3;
	data->led_current4 = MAX86915_HRM_DEFAULT_CURRENT4;
#endif
	data->xtalk_code1 = MAX86915_HRM_DEFAULT_XTALK_CODE1;
	data->xtalk_code2 = MAX86915_HRM_DEFAULT_XTALK_CODE2;
#ifdef CONFIG_HRM_GREEN_BLUE
	data->xtalk_code3 = MAX86915_HRM_DEFAULT_XTALK_CODE3;
	data->xtalk_code4 = MAX86915_HRM_DEFAULT_XTALK_CODE4;
#endif

	HRM_info("%s - flexmode : 0x%02x, num_samples : %d\n", __func__,
			data->flex_mode, data->num_samples);

	/*write LED currents ir=1, red=2, violet=4*/
	err = max86915_write_reg(data, MAX86915_LED1_PA,
		data->led_current1);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED1_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED2_PA,
			data->led_current2);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED2_PA!\n",
			__func__);
		return -EIO;
	}

#ifdef CONFIG_HRM_GREEN_BLUE
	err = max86915_write_reg(data, MAX86915_LED3_PA,
			data->led_current3);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED3_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED4_PA,
			data->led_current4);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED4_PA!\n",
			__func__);
		return -EIO;
	}
#endif
	/* LED Range */
	err = max86915_write_reg(data, MAX86915_LED_RANGE,
			  (data->led_range[0] << MAX86915_LED1_RGE_OFFSET)
			| (data->led_range[1] << MAX86915_LED2_RGE_OFFSET)
			| (data->led_range[2] << MAX86915_LED3_RGE_OFFSET)
			| (data->led_range[3] << MAX86915_LED4_RGE_OFFSET));
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_RANGE!\n",
			__func__);
		return -EIO;
	}
	/* XTALK */
	err = max86915_write_reg(data, MAX86915_DAC1_XTALK_CODE,
			data->xtalk_code1);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC1_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_DAC2_XTALK_CODE,
			data->xtalk_code2);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC2_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
#ifdef CONFIG_HRM_GREEN_BLUE
	err = max86915_write_reg(data, MAX86915_DAC3_XTALK_CODE,
			data->xtalk_code3);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC3_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_DAC4_XTALK_CODE,
			data->xtalk_code4);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC4_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
#endif

	err = max86915_write_reg(data, MAX86915_INTERRUPT_ENABLE, PPG_RDY_MASK);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_INTERRUPT_ENABLE!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_LED_SEQ_REG_1,
			flex_config[0]);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_SEQ_REG_1!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED_SEQ_REG_2,
			flex_config[1]);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_SEQ_REG_2!\n",
			__func__);
		return -EIO;
	}

	/* 400 Hz Sampling Rate */
	err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION_2,
			0x0D | (0x03 << MAX86915_ADC_RGE_OFFSET));
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_FIFO_CONFIG,
			(0x02 << MAX86915_SMP_AVE_OFFSET) & MAX86915_SMP_AVE_MASK);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_FIFO_CONFIG!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION, 0x03);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION!\n",
			__func__);
		return -EIO;
	}

	/* AGC control */
	data->agc_current[AGC_IR] =
		(data->led_current1 * MAX86915_CURRENT_PER_STEP(data->led_range[0]));
	data->agc_current[AGC_RED] =
		(data->led_current2 * MAX86915_CURRENT_PER_STEP(data->led_range[1]));
#ifdef CONFIG_HRM_GREEN_BLUE
	data->agc_current[AGC_GREEN] =
		(data->led_current3 * MAX86915_CURRENT_PER_STEP(data->led_range[2]));
	data->agc_current[AGC_BLUE] =
		(data->led_current4 * MAX86915_CURRENT_PER_STEP(data->led_range[3]));
#endif

	for (i = 0; i < data->num_samples; i++) {
		data->reached_thresh[i] = 0;
		data->agc_sum[i] = 0;
		data->prev_current[i] = 0;
		data->prev_agc_current[i] = MAX86915_MAX_CURRENT(data->led_range[i]);
		data->prev_ppg[i] = 0;
	}

	return 0;
}

static int max86915_init_sdk_reg(struct max86915_device_data *data)
{
	int err, i;
	u8 flex_config[2] = {0, };

	data->sample_cnt = 0;
	data->agc_led_set = 0;

	data->agc_ch_adc[0] = 0;
	data->agc_ch_adc[1] = 0;
	data->agc_ch_adc[2] = 0;
	data->agc_ch_adc[3] = 0;

	data->num_samples = 4;
	data->flex_mode = 0xf;

	flex_config[0] = (RED_LED_CH << MAX86915_LED2_OFFSET) | IR_LED_CH;
	flex_config[1] = (BLUE_LED_CH << MAX86915_LED4_OFFSET) | GREEN_LED_CH;

	data->led_current1 = MAX86915_MIN_CURRENT;
	data->led_current2 = MAX86915_MIN_CURRENT;
	data->led_current3 = MAX86915_MIN_CURRENT;
	data->led_current4 = MAX86915_MIN_CURRENT;

	/*
	 * data->led_current1 = (data->mode_sdk_enabled & (1<<0)) ? 0xff : 0x00;
	 * data->led_current2 = (data->mode_sdk_enabled & (1<<1)) ? 0xff : 0x00;
	 * data->led_current3 = (data->mode_sdk_enabled & (1<<2)) ? 0xff : 0x00;
	 * data->led_current4 = (data->mode_sdk_enabled & (1<<3)) ? 0xff : 0x00;
	 * data->xtalk_code1 = MAX86915_DEFAULT_XTALK_CODE1;
	 * data->xtalk_code2 = MAX86915_DEFAULT_XTALK_CODE2;
	 * data->xtalk_code3 = MAX86915_DEFAULT_XTALK_CODE3;
	 * data->xtalk_code4 = MAX86915_DEFAULT_XTALK_CODE4;
	 */

	HRM_info("%s - flexmode : 0x%02x, num_samples : %d\n", __func__,
			data->flex_mode, data->num_samples);

	/* write LED currents ir=1, red=2, violet=4 */
	err = max86915_write_reg(data, MAX86915_LED1_PA,
		data->led_current1);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED1_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED2_PA,
			data->led_current2);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED2_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED3_PA,
			data->led_current3);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED3_PA!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED4_PA,
			data->led_current4);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED4_PA!\n",
			__func__);
		return -EIO;
	}

	/* LED Range */
	err = max86915_write_reg(data, MAX86915_LED_RANGE,
		  (data->led_range[0] << MAX86915_LED1_RGE_OFFSET)
		| (data->led_range[1] << MAX86915_LED2_RGE_OFFSET)
		| (data->led_range[2] << MAX86915_LED3_RGE_OFFSET)
		| (data->led_range[3] << MAX86915_LED4_RGE_OFFSET));
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_RANGE!\n",
			__func__);
		return -EIO;
	}

	/* XTALK */
	err = max86915_write_reg(data, MAX86915_DAC1_XTALK_CODE,
			data->xtalk_code1);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC1_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_DAC2_XTALK_CODE,
			data->xtalk_code2);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC2_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_DAC3_XTALK_CODE,
			data->xtalk_code3);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC3_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_DAC4_XTALK_CODE,
			data->xtalk_code4);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_DAC4_XTALK_CODE!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_INTERRUPT_ENABLE, PPG_RDY_MASK);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_INTERRUPT_ENABLE!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_LED_SEQ_REG_1,
			flex_config[0]);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_SEQ_REG_1!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data, MAX86915_LED_SEQ_REG_2,
			flex_config[1]);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_SEQ_REG_2!\n",
			__func__);
		return -EIO;
	}

#ifdef ENABLE_POLL_DELAY
	if (fifo_full_cnt == 8) {
		/* 16uA, 800Hz, 70us */
		err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION_2,
				0x50);
		/* 1 Samples avg */
		err = max86915_write_reg(data, MAX86915_FIFO_CONFIG,
				0x1f);
	} else if (fifo_full_cnt == 4) {
		/* 16uA, 400Hz, 120us */
		err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION_2,
				0x50);
		/* 2 Samples avg */
		err = max86915_write_reg(data, MAX86915_FIFO_CONFIG,
				0x3f);
	} else if (fifo_full_cnt == 2) {
		/* 16uA, 200Hz, 120us */
		err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION_2,
				0x50);
		/* 4 Samples avg */
		err = max86915_write_reg(data, MAX86915_FIFO_CONFIG,
				0x5f);
	} else if (fifo_full_cnt == 1) {
		/* 16uA, 400Hz, 120us */
		err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION_2,
				0x4D);
		/* 4 Samples avg */
		err = max86915_write_reg(data, MAX86915_FIFO_CONFIG,
				0x5f);
	}
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
			__func__);
		HRM_err("%s - error initializing MAX86915_FIFO_CONFIG!\n",
			__func__);
		return -EIO;
	}
#else

	/* 400 Hz Sampling Rate & 120us Pulse Width */
	err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION_2,
			0x0D | (0x03 << MAX86915_ADC_RGE_OFFSET));
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_FIFO_CONFIG,
			(0x02 << MAX86915_SMP_AVE_OFFSET) & MAX86915_SMP_AVE_MASK);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_FIFO_CONFIG!\n",
			__func__);
		return -EIO;
	}
#endif

	err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION, 0x03);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION!\n",
			__func__);
		return -EIO;
	}

	/* AGC control */
	data->agc_current[AGC_IR] =
		(data->led_current1 * MAX86915_CURRENT_PER_STEP(data->led_range[0]));
	data->agc_current[AGC_RED] =
		(data->led_current2 * MAX86915_CURRENT_PER_STEP(data->led_range[1]));
	data->agc_current[AGC_GREEN] =
		(data->led_current3 * MAX86915_CURRENT_PER_STEP(data->led_range[2]));
	data->agc_current[AGC_BLUE] =
		(data->led_current4 * MAX86915_CURRENT_PER_STEP(data->led_range[3]));

	for (i = 0; i < data->num_samples; i++) {
		data->reached_thresh[i] = 0;
		data->agc_sum[i] = 0;
		data->prev_current[i] = 0;
		data->prev_agc_current[i] = MAX86915_MAX_CURRENT(data->led_range[i]);
		data->prev_ppg[i] = 0;
	}

	return 0;
}

static int max86915_set_reg_hrm(struct max86915_device_data *data)
{
	int err = 0;

	err = max86915_init_status(data);
	err = max86915_init_hrm_reg(data);
	data->agc_mode = M_HRM;

	return err;
}

static int max86915_set_reg_sdk(struct max86915_device_data *data)
{
	int err = 0;

	err = max86915_init_status(data);
	err = max86915_init_sdk_reg(data);
	data->agc_mode = M_SDK;
	data->agc_enabled = 1;

	return err;
}

static int max86915_set_reg_svc(struct max86915_device_data *data)
{
	int err = 0;

	err = max86915_init_status(data);
	err = max86915_init_sdk_reg(data);

	data->agc_mode = M_NONE;
	err = max86915_write_reg(data, MAX86915_INTERRUPT_ENABLE, 0);
	err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION_2, 0x44);

	return err;
}

static int max86915_set_reg_ambient(struct max86915_device_data *data)
{
	int err = 0;
	u8 recvData = 0;

	err = max86915_init_status(data);
	err = max86915_init_hrm_reg(data);

	/* Mode change to AWB */
	err = max86915_write_reg(data,
		MAX86915_MODE_CONFIGURATION, MAX86915_RESET_MASK);
	if (err != 0) {
		HRM_err("%s - error sw shutdown data!\n", __func__);
		return -EIO;
	}

	/* Interrupt Clear */
	err = max86915_read_reg(data, MAX86915_INTERRUPT_STATUS, &recvData, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData);
		return -EIO;
	}

	/* Interrupt2 Clear */
	err = max86915_read_reg(data, MAX86915_INTERRUPT_STATUS_2, &recvData, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData);
		return -EIO;
	}
	/* 400Hz, LED_PW=400us, SPO2_ADC_RANGE=4096nA */
	err = max86915_write_reg(data,
		MAX86915_MODE_CONFIGURATION_2, 0x0F);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION_2!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data,
		MAX86915_FIFO_CONFIG, ((32 - AWB_INTERVAL) & MAX86915_FIFO_A_FULL_MASK));
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_FIFO_CONFIG!\n",
			__func__);
		return -EIO;
	}
	err = max86915_write_reg(data,
		MAX86915_INTERRUPT_ENABLE, A_FULL_MASK);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_INTERRUPT_ENABLE!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data, MAX86915_LED_SEQ_REG_1,
			0x01);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_LED_SEQ_REG_1!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(data,
			MAX86915_MODE_CONFIGURATION, 0x07);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_CONFIGURATION!\n",
			__func__);
		return -EIO;
	}
	data->awb_sample_cnt = 0;
	data->flicker_data_cnt = 0;
	data->awb_flicker_status = AWB_CONFIG1;
	data->agc_mode = M_NONE;

	return err;
}

static int max86915_set_reg_prox(struct max86915_device_data *data)
{
	int err = 0;

	err = max86915_init_status(data);
	err = max86915_init_hrm_reg(data);

	max86915_set_current(0xff, 0, 0, 0);
	data->agc_mode = M_NONE;

	return err;
}

static int max86915_enable(struct max86915_device_data *data, enum op_mode mode)
{
	int err = 0;

	HRM_dbg("%s - enable_m : %d cur_m : %d", __func__, mode, max86915_data->enabled_mode);

	if (data->hrm_threshold != DEFAULT_THRESHOLD) {
		if (data->threshold_default != data->hrm_threshold)
			data->threshold_default = data->hrm_threshold;
	} else
		data->hrm_threshold = data->threshold_default;

	if (mode == MODE_HRM)
		err = max86915_set_reg_hrm(max86915_data);
	else if (mode == MODE_AMBIENT)
		err = max86915_set_reg_ambient(max86915_data);
	else if (mode == MODE_PROX)
		err = max86915_set_reg_prox(max86915_data);
	else if (mode == MODE_SDK_IR)
		err = max86915_set_reg_sdk(max86915_data);
	else if (mode == MODE_SVC_IR)
		err = max86915_set_reg_svc(max86915_data);
	else
		HRM_err("%s - MODE_UNKNOWN\n", __func__);

	max86915_data->agc_sample_cnt[0] = 0;
	max86915_data->agc_sample_cnt[1] = 0;
	max86915_data->agc_sample_cnt[2] = 0;
	max86915_data->agc_sample_cnt[3] = 0;

	if (err < 0) {
		HRM_err("%s - fail err = %d\n", __func__, err);
		return err;
	}

	return 0;
}

static int max86915_disable(struct max86915_device_data *data, enum op_mode mode)
{
	int err = 0;

	HRM_dbg("%s - disable_m : %d cur_m : %d\n", __func__, mode, max86915_data->enabled_mode);

	if (mode == 0) {
		err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION, MAX86915_RESET_MASK);
		if (err != 0) {
			HRM_err("%s - error init MAX86915_MODE_CONFIGURATION!\n",
				__func__);
			return -EIO;
		}
		err = max86915_write_reg(data, MAX86915_MODE_CONFIGURATION, MAX86915_SHDN_MASK);
		if (err != 0) {
			HRM_err("%s - error init MAX86915_MODE_CONFIGURATION!\n",
				__func__);
			return -EIO;
		}
		data->awb_sample_cnt = 0;
		data->flicker_data_cnt = 0;

		data->led_current1 = 0;
		data->led_current2 = 0;
		data->led_current3 = 0;
		data->led_current4 = 0;
	}

	if (err < 0) {
		HRM_err("%s - fail err = %d\n", __func__, err);
		return err;
	}

	return 0;
}

static void max86915_pin_control(struct max86915_device_data *data, bool pin_set)
{
	int status = 0;

	if (!data->hrm_pinctrl) {
		HRM_err("%s - hrm_pinctrl is null\n", __func__);
		return;
	}
	if (pin_set) {
		if (!IS_ERR_OR_NULL(data->pins_idle)) {
			status = pinctrl_select_state(data->hrm_pinctrl,
				data->pins_idle);
			if (status)
				HRM_err("%s - can't set pin default state\n",
					__func__);
			HRM_info("%s idle\n", __func__);
		}
	} else {
		if (!IS_ERR_OR_NULL(data->pins_sleep)) {
			status = pinctrl_select_state(data->hrm_pinctrl,
				data->pins_sleep);
			if (status)
				HRM_err("%s - can't set pin sleep state\n",
					__func__);
			HRM_info("%s sleep\n", __func__);
		}
	}
}

static void max86915_irq_set_state(struct max86915_device_data *data, int irq_enable)
{
	HRM_dbg("%s - irq_enable : %d, irq_state : %d\n",
		__func__, irq_enable, data->irq_state);

	if (irq_enable) {
		if (data->irq_state++ == 0) {
			max86915_pin_control(data, true);
		}
	} else {
		if (data->irq_state == 0)
			return;
		if (--data->irq_state <= 0) {
			max86915_pin_control(data, false);
			data->irq_state = 0;
		}
	}
}

static int max86915_led_power_ctrl(struct max86915_device_data *data, int onoff)
{
	int rc = 0;

	if (onoff == PWR_ON) {
		rc = gpio_direction_output(data->pin_hrm_en, 1);
		if (rc) {
			HRM_err("%s - gpio direction output failed, rc=%d\n",
				__func__, rc);
		}
		usleep_range(2000, 2100);
	} else {
		gpio_set_value(data->pin_hrm_en, 0);
		rc = gpio_direction_input(data->pin_hrm_en);
		if (rc) {
			HRM_err("%s - gpio direction input failed, rc=%d\n",
				__func__, rc);
		}
	}
	return rc;
}

static int max86915_power_ctrl(struct max86915_device_data *data, int onoff)
{
	int rc = 0;
	static int i2c_1p8_enable;

	struct regulator *regulator_vdd_1p8 = NULL;
	struct regulator *regulator_i2c_1p8 = NULL;

	HRM_dbg("%s - onoff : %d, state : %d\n",
		__func__, onoff, data->regulator_state);

	if (onoff == PWR_ON) {
		if (data->regulator_state != 0) {
			HRM_dbg("%s - duplicate regulator\n", __func__);
			data->regulator_state++;
			return 0;
		}
		data->regulator_state++;
		data->pm_state = PM_RESUME;
	} else {
		if (data->regulator_state == 0) {
			HRM_dbg("%s - already off the regulator\n", __func__);
			return 0;
		} else if (data->regulator_state != 1) {
			HRM_dbg("%s - duplicate regulator\n", __func__);
			data->regulator_state--;
			return 0;
		}
		data->regulator_state--;
	}

	if (data->i2c_1p8 != NULL) {
		regulator_i2c_1p8 = regulator_get(NULL, data->i2c_1p8);
		if (IS_ERR(regulator_i2c_1p8) || regulator_i2c_1p8 == NULL) {
			HRM_err("%s - get i2c_1p8 regulator failed, %d\n", __func__, PTR_ERR(regulator_i2c_1p8));
			rc = -EINVAL;
			regulator_i2c_1p8 = NULL;
			goto get_i2c_1p8_failed;
		}
	}

	if (data->vdd_1p8 != NULL) {
		regulator_vdd_1p8 =
			regulator_get(&data->client->dev, data->vdd_1p8);
		if (IS_ERR(regulator_vdd_1p8) || regulator_vdd_1p8 == NULL) {
			HRM_dbg("%s - get vdd_1p8 regulator failed, %d\n", __func__, PTR_ERR(regulator_vdd_1p8));
			regulator_vdd_1p8 = NULL;
			goto get_vdd_1p8_failed;
		}
	}

	if (onoff == PWR_ON) {
		if (data->i2c_1p8 != NULL && i2c_1p8_enable == 0) {
			rc = regulator_enable(regulator_i2c_1p8);
			i2c_1p8_enable = 1;
			if (rc) {
				HRM_err("%s - enable i2c_1p8 failed, rc=%d\n", __func__, rc);
				goto enable_i2c_1p8_failed;
			}
		}
		if (regulator_vdd_1p8 != NULL) {
			rc = regulator_enable(regulator_vdd_1p8);
			if (rc) {
				HRM_err("%s - enable vdd_1p8 failed, rc=%d\n",
					__func__, rc);
				goto enable_vdd_1p8_failed;
			}
		}
	} else {
		if (regulator_vdd_1p8 != NULL) {
			rc = regulator_disable(regulator_vdd_1p8);
			if (rc) {
				HRM_err("%s - disable vdd_1p8 failed, rc=%d\n",
					__func__, rc);
				goto done;
			}
		}
#ifdef I2C_1P8_DISABLE
		if (data->i2c_1p8 != NULL) {
			rc = regulator_disable(regulator_i2c_1p8);
			i2c_1p8_enable = 0;
			if (rc) {
				HRM_err("%s - disable i2c_1p8 failed, rc=%d\n", __func__, rc);
				goto done;
			}
		}
#endif
	}

	goto done;

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
get_vdd_1p8_failed:
	if (data->i2c_1p8 != NULL)
		regulator_put(regulator_i2c_1p8);
get_i2c_1p8_failed:
	return rc;
}

static int max86915_eol_set_mode(struct max86915_device_data *data, int onoff)
{
	int err = 0;

	HRM_dbg("%s - onoff = %d\n", __func__, onoff);

	if (onoff == PWR_ON) {
		data->eol_test_is_enable = 1;
		data->agc_mode = M_NONE;

		max86915_irq_set_state(data, PWR_ON);

		if (!data->always_on) {
			err = max86915_power_ctrl(data, PWR_ON);
			if (err < 0)
				HRM_err("%s - max86915_power_ctrl fail err = %d\n", __func__, err);
		}
		err = max86915_led_power_ctrl(data, PWR_ON);
		if (err)
			HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);

		max86915_init_eol_data(&data->eol_data);

		if (data->eol_test_type == EOL_XTALK) {
			data->eol_data.state = _EOL_STATE_TYPE_NEW_XTALK_MODE;
			err = max86915_set_reg_hrm(data);
		} else {
			if (data->pre_eol_test_is_enable == 2) /* Pre Test Only for Frequency */
				err = max86915_enable_eol_freq(data);
			else
				err = max86915_enable_eol_flicker(data);
		}
		if (err < 0) {
			data->eol_test_is_enable = 0;

			err = max86915_disable(data, MODE_NONE);
			if (err != 0)
				HRM_err("%s - max86915_disable err : %d\n", __func__, err);

			err = max86915_led_power_ctrl(data, PWR_OFF);
			if (err)
				HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);

			if (!data->always_on) {
				err = max86915_power_ctrl(data, PWR_OFF);
				if (err < 0)
					HRM_err("%s - hrm_regulator_on fail err = %d\n",
						__func__, err);
			}
			max86915_irq_set_state(data, PWR_OFF);

			HRM_err("%s - eol test enable fail err = %d\n", __func__, err);
			return err;
		}
	} else {
		data->eol_test_is_enable = 0;

		err = max86915_disable(data, MODE_NONE);
		if (err != 0) {
			HRM_err("%s - max86915_disable err : %d\n", __func__, err);
			return err;
		}

		err = max86915_led_power_ctrl(data, PWR_OFF);
		if (err) {
			HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);
			return err;
		}
		
		if (!data->always_on) {
			err = max86915_power_ctrl(data, PWR_OFF);
			if (err < 0)
				HRM_err("%s - hrm_regulator_on fail err = %d\n",
					__func__, err);
		}
		max86915_irq_set_state(data, PWR_OFF);
	}

	return err;
}

static void max86915_set_mode(struct max86915_device_data *data,
	int onoff, enum op_mode mode)
{
	int err;

	if (onoff == PWR_ON) {
		max86915_irq_set_state(data, PWR_ON);

		if (!data->always_on) {
			err = max86915_power_ctrl(data, PWR_ON);
			if (err < 0)
				HRM_err("%s - hrm_regulator_on fail err = %d\n",
					__func__, err);
		}
		err = max86915_led_power_ctrl(data, PWR_ON);
		if (err)
			HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);

		err = max86915_enable(data, mode);
		if (err == 0)
			data->enabled_mode = mode;
		else {
			data->enabled_mode = MODE_NONE;

			err = max86915_disable(data, MODE_NONE);
			if (err != 0)
				HRM_err("%s - max86915_disable err : %d\n", __func__, err);

			err = max86915_led_power_ctrl(data, PWR_OFF);
			if (err)
				HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);

			if (!data->always_on) {
				err = max86915_power_ctrl(data, PWR_OFF);
				if (err < 0)
					HRM_err("%s - hrm_regulator_on fail err = %d\n", __func__, err);
			}
			max86915_irq_set_state(data, PWR_OFF);

			HRM_err("%s - enable err : %d\n", __func__, err);
		}

		if (err < 0 && mode == MODE_AMBIENT) {
			input_report_rel(data->hrm_input_dev,
				REL_Y, -5 + 1); /* F_ERR_I2C -5 detected i2c error */
			input_sync(data->hrm_input_dev);
			HRM_err("%s - awb mode enable error %d\n", __func__, err);
		}
	} else {
		if (data->regulator_state == 0) {
			HRM_dbg("%s - already power off - disable skip\n",
				__func__);
			return;
		}

		err = max86915_disable(data, mode);
		if (err != 0)
			HRM_err("%s - disable err : %d\n", __func__, err);

		data->enabled_mode = 0;
		data->mode_sdk_enabled = 0;
		data->mode_svc_enabled = 0;

		err = max86915_led_power_ctrl(data, PWR_OFF);
		if (err)
			HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);

		if (!data->always_on) {
			err = max86915_power_ctrl(data, PWR_OFF);
			if (err < 0)
				HRM_err("%s - hrm_regulator_off fail err = %d\n", __func__, err);
		}
		max86915_irq_set_state(data, PWR_OFF);
	}
	HRM_info("%s - set mode complete, onoff : %d m : %d c : %d\n",
		__func__, onoff, mode, data->enabled_mode);
}

/* hrm input enable/disable sysfs */
static ssize_t max86915_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, strlen(buf), "%d\n", data->enabled_mode);
}

static ssize_t max86915_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	int on_off;
	enum op_mode mode;

	mutex_lock(&data->activelock);
	if (sysfs_streq(buf, "0")) {
		on_off = PWR_OFF;
		mode = MODE_NONE;
	} else if (sysfs_streq(buf, "1")) {
		on_off = PWR_ON;
		mode = MODE_HRM;
		data->mode_cnt.hrm_cnt++;
	} else if (sysfs_streq(buf, "2")) {
		on_off = PWR_ON;
		mode = MODE_AMBIENT;
		data->mode_cnt.amb_cnt++;
	} else if (sysfs_streq(buf, "3")) {
		on_off = PWR_ON;
		mode = MODE_PROX;
		data->mode_cnt.prox_cnt++;
	} else if (sysfs_streq(buf, "10")) {
		on_off = PWR_ON;
		mode = MODE_SDK_IR;
		data->mode_cnt.sdk_cnt++;
	} else if (sysfs_streq(buf, "11")) {
		on_off = PWR_ON;
		mode = MODE_SDK_RED;
	} else if (sysfs_streq(buf, "12")) {
		on_off = PWR_ON;
		mode = MODE_SDK_GREEN;
	} else if (sysfs_streq(buf, "13")) {
		on_off = PWR_ON;
		mode = MODE_SDK_BLUE;
	} else if (sysfs_streq(buf, "-10")) {
		on_off = PWR_OFF;
		mode = MODE_SDK_IR;
	} else if (sysfs_streq(buf, "-11")) {
		on_off = PWR_OFF;
		mode = MODE_SDK_RED;
	} else if (sysfs_streq(buf, "-12")) {
		on_off = PWR_OFF;
		mode = MODE_SDK_GREEN;
	} else if (sysfs_streq(buf, "-13")) {
		on_off = PWR_OFF;
		mode = MODE_SDK_BLUE;
	} else if (sysfs_streq(buf, "14")) {
		on_off = PWR_ON;
		mode = MODE_SVC_IR;
		data->mode_cnt.unkn_cnt++;
	} else if (sysfs_streq(buf, "15")) {
		on_off = PWR_ON;
		mode = MODE_SVC_RED;
	} else if (sysfs_streq(buf, "16")) {
		on_off = PWR_ON;
		mode = MODE_SVC_GREEN;
	} else if (sysfs_streq(buf, "17")) {
		on_off = PWR_ON;
		mode = MODE_SVC_BLUE;
	} else if (sysfs_streq(buf, "-14")) {
		on_off = PWR_OFF;
		mode = MODE_SVC_IR;
	} else if (sysfs_streq(buf, "-15")) {
		on_off = PWR_OFF;
		mode = MODE_SVC_RED;
	} else if (sysfs_streq(buf, "-16")) {
		on_off = PWR_OFF;
		mode = MODE_SVC_GREEN;
	} else if (sysfs_streq(buf, "-17")) {
		on_off = PWR_OFF;
		mode = MODE_SVC_BLUE;
	} else {
		HRM_err("%s - invalid value %d\n", __func__, *buf);
		data->mode_cnt.unkn_cnt++;
		mutex_unlock(&data->activelock);
		return -EINVAL;
	}

	if (mode == MODE_SDK_IR || mode == MODE_SDK_RED
		|| mode == MODE_SDK_GREEN || mode == MODE_SDK_BLUE) {
		HRM_dbg("%s - SDK en : %d m : %d c : %d\n", __func__, on_off, mode, data->mode_sdk_enabled);
		if (on_off == PWR_ON) {
			if (data->mode_sdk_enabled & (1<<(mode - MODE_SDK_IR))) { /* already enabled */
				/* Do Nothing */
				HRM_dbg("%s - SDK %d mode already enabled\n", __func__, mode);
				mutex_unlock(&data->activelock);
				return count;
			}
			if (data->mode_sdk_enabled == 0) {
				data->mode_sdk_enabled |= (1<<(mode - MODE_SDK_IR));
				mode = MODE_SDK_IR;
				data->mode_cnt.sdk_cnt++;
			} else {
				data->mode_sdk_enabled |= (1<<(mode - MODE_SDK_IR));
				mutex_unlock(&data->activelock);
				return count;
			}
		} else {
			if ((data->mode_sdk_enabled & (1<<(mode - MODE_SDK_IR))) == 0) { /* Not Enabled */
				/* Do Nothing */
				HRM_dbg("%s - SDK %d mode not enabled\n", __func__, mode);
				mutex_unlock(&data->activelock);
				return count;
			}
			/* Oring disable mode */
			data->mode_sdk_enabled &= ~(1<<(mode - MODE_SDK_IR));
			max86915_reset_sdk_agc_var(mode - MODE_SDK_IR);

			if (data->mode_sdk_enabled == 0) {
				mode = MODE_NONE;
			} else {
				mutex_unlock(&data->activelock);
				return count;
			}
		}
	} else if (mode == MODE_SVC_IR || mode == MODE_SVC_RED
		|| mode == MODE_SVC_GREEN || mode == MODE_SVC_BLUE) {
		HRM_dbg("%s - SVC en : %d m : %d c : %d\n", __func__, on_off, mode, data->mode_svc_enabled);
		if (on_off == PWR_ON) {
			if (data->mode_svc_enabled & (1<<(mode - MODE_SVC_IR))) { /* already enabled */
				/* Do Nothing */
				HRM_dbg("%s - SVC %d mode already enabled\n", __func__, mode);
				mutex_unlock(&data->activelock);
				return count;
			}
			if (data->mode_svc_enabled == 0) {
				data->mode_svc_enabled |= (1<<(mode - MODE_SVC_IR));
				mode = MODE_SVC_IR;
				data->mode_cnt.unkn_cnt++;
			} else {
				data->mode_svc_enabled |= (1<<(mode - MODE_SVC_IR));
				mutex_unlock(&data->activelock);
				return count;
			}
		} else {
			if ((data->mode_svc_enabled & (1<<(mode - MODE_SVC_IR))) == 0) { /* Not Enabled */
				/* Do Nothing */
				HRM_dbg("%s - SVC %d mode not enabled\n", __func__, mode);
				mutex_unlock(&data->activelock);
				return count;
			}
			/* Oring disable mode */
			data->mode_svc_enabled &= ~(1<<(mode - MODE_SVC_IR));

			if (data->mode_svc_enabled == 0) {
				mode = MODE_NONE;
			} else {
				mutex_unlock(&data->activelock);
				return count;
			}
		}
	}

	HRM_dbg("%s - en : %d m : %d c : %d\n", __func__, on_off, mode, data->enabled_mode);
	max86915_set_mode(data, on_off, mode);
	mutex_unlock(&data->activelock);

	return count;
}

static ssize_t max86915_poll_delay_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->sampling_period_ns);
}

static ssize_t max86915_poll_delay_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	u32 sampling_period_ns = 0;
	int err = 0;

	mutex_lock(&data->activelock);

	err = kstrtoint(buf, 10, &sampling_period_ns);

	if (err < 0) {
		HRM_err("%s - kstrtoint failed.(%d)\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}

	err = max86915_set_sampling_rate(sampling_period_ns);

	if (err > 0)
		data->sampling_period_ns = err;

	HRM_dbg("%s - hrm sensor sampling rate is setted as %dns\n", __func__, sampling_period_ns);

	mutex_unlock(&data->activelock);

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
	max86915_enable_show, max86915_enable_store);
static DEVICE_ATTR(poll_delay, S_IRUGO|S_IWUSR|S_IWGRP,
	max86915_poll_delay_show, max86915_poll_delay_store);

static struct attribute *hrm_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group hrm_attribute_group = {
	.attrs = hrm_sysfs_attrs,
};

/* hrm_sensor sysfs */
static ssize_t max86915_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char chip_name[NAME_LEN];

	switch (max86915_data->part_type) {
	case PART_TYPE_MAX86915_ES:
	case PART_TYPE_MAX86915_CS_15:
	case PART_TYPE_MAX86915_CS_21:
	case PART_TYPE_MAX86915_CS_22:
	case PART_TYPE_MAX86915_CS_211:
	case PART_TYPE_MAX86915_CS_221:
		strlcpy(chip_name, MAX86915_CHIP_NAME, strlen(MAX86915_CHIP_NAME) + 1);
		break;
	case PART_TYPE_MAX86917_1:
	case PART_TYPE_MAX86917_2:
		strlcpy(chip_name, MAX86917_CHIP_NAME, strlen(MAX86917_CHIP_NAME) + 1);
		break;
	default:
		strlcpy(chip_name, "NONE", 5);
		break;
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", chip_name);
}

static ssize_t max86915_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char vendor[NAME_LEN];

	strlcpy(vendor, VENDOR, strlen(VENDOR) + 1);

	return snprintf(buf, PAGE_SIZE, "%s\n", vendor);
}

static ssize_t max86915_led_current_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int err;
	int led1, led2, led3, led4;
	int range1, range2, range3, range4;
	u32 led_current, led_range;
	u8 reg_value;
	struct max86915_device_data *data = dev_get_drvdata(dev);

	led_range = (max86915_data->led_range[0] << 0)
		| (max86915_data->led_range[1] << 4)
		| (max86915_data->led_range[2] << 8)
		| (max86915_data->led_range[3] << 12);

	mutex_lock(&data->activelock);
	err = sscanf(buf, "%8x, %4x", &led_current, &led_range);
	if (err < 0) {
		HRM_err("%s - sscanf failed, err = %x\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	led1 = 0x000000ff & led_current;
	led2 = (0x0000ff00 & led_current) >> 8;
	led3 = (0x00ff0000 & led_current) >> 16;
	led4 = (0xff000000 & led_current) >> 24;
	HRM_dbg("%s - led_current = %08x : 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
		__func__, led_current, led1, led2, led3, led4);

	range1 = 0x0003 & led_range;
	range2 = (0x0030 & led_range) >> 4;
	range3 = (0x0300 & led_range) >> 8;
	range4 = (0x3000 & led_range) >> 12;
	HRM_dbg("%s - led_range = %04x : 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
		__func__, led_range, range1, range2, range3, range4);

	max86915_data->led_range[0] = range1;
	max86915_data->led_range[1] = range2;
	max86915_data->led_range[2] = range3;
	max86915_data->led_range[3] = range4;

	reg_value = (0x0003 & led_range) | ((0x0030 & led_range) >> 2)
		| ((0x0300 & led_range) >> 4) | ((0x3000 & led_range) >> 6);

	err = max86915_write_reg(max86915_data, MAX86915_LED_RANGE, reg_value);

	if (err != 0) {
		HRM_err("%s - error MAX86915_LED_RANGE!\n",
			__func__);
		mutex_unlock(&data->activelock);
		return -EIO;
	}

	err = max86915_set_current(led1, led2, led3, led4);
	if (err < 0) {
		HRM_err("%s - set current failed, err = %x\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	mutex_unlock(&data->activelock);

	return size;
}

static ssize_t max86915_led_current_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	u32 led_current, led_range;

	mutex_lock(&data->activelock);
	led_current = (data->led_current1 & 0xff) | ((data->led_current2 & 0xff) << 8)
		| ((data->led_current3 & 0xff) << 16) | ((data->led_current4 & 0xff) << 24);

	led_range = (data->led_range[0] & 0x03) | ((data->led_range[1] & 0x03) << 4)
		| ((data->led_range[2] & 0x03) << 8) | ((data->led_range[3] & 0x03) << 12);
	mutex_unlock(&data->activelock);

	HRM_info("%s - led1 0x%02x, led2 0x%02x, led3 0x%02x, led4 0x%02x\n",
		__func__, data->led_current1, data->led_current2, data->led_current3, data->led_current4);

	HRM_info("%s - range1 0x%02x, range2 0x%02x, range3 0x%02x, range4 0x%02x\n",
		__func__, data->led_range[0], data->led_range[1], data->led_range[2], data->led_range[3]);

	return snprintf(buf, PAGE_SIZE, "%08X, %04X\n", led_current, led_range);
}

static ssize_t max86915_led_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int err;
	u8 result;
	struct max86915_device_data *data = dev_get_drvdata(dev);

	if (!data->always_on) {
		err = max86915_power_ctrl(data, PWR_ON);
		if (err < 0)
			HRM_err("%s - hrm_regulator_on fail err = %d\n", __func__, err);
	}
	err = max86915_led_power_ctrl(data, PWR_ON);
	if (err)
		HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);

	mutex_lock(&data->activelock);

	err = max86915_get_led_test(&result);
	if (err < 0) {
		HRM_err("%s - led test failed, err = %x\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}

	mutex_unlock(&data->activelock);

	err = max86915_disable(data, MODE_NONE);
	if (err != 0)
		HRM_err("%s - max86915_disable err : %d\n", __func__, err);

	err = max86915_led_power_ctrl(data, PWR_OFF);
	if (err)
		HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);

	if (!data->always_on) {
		err = max86915_power_ctrl(data, PWR_OFF);
		if (err < 0)
			HRM_err("%s - hrm_regulator_off fail err = %d\n", __func__, err);
	}
	HRM_info("%s - result = 0x%02x\n", __func__, result);

	return snprintf(buf, PAGE_SIZE, "0x%02x\n", result);
}

static ssize_t max86915_flush_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	int ret = 0;
	u8 handle = 0;

	mutex_lock(&data->activelock);
	ret = kstrtou8(buf, 10, &handle);
	if (ret < 0) {
		HRM_err("%s - kstrtou8 failed.(%d)\n", __func__, ret);
		mutex_unlock(&data->activelock);
		return ret;
	}
	HRM_dbg("%s - handle = %d\n", __func__, handle);
	mutex_unlock(&data->activelock);

	input_report_rel(data->hrm_input_dev, REL_MISC, handle);

	return size;
}

static ssize_t max86915_int_pin_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	/* need to check if this should be implemented */
	HRM_dbg("%s - not implement\n", __func__);
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t max86915_lib_ver_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	size_t buf_len;

	mutex_lock(&data->activelock);
	buf_len = strlen(buf) + 1;
	if (buf_len > NAME_LEN)
		buf_len = NAME_LEN;

	if (data->lib_ver != NULL)
		kfree(data->lib_ver);

	data->lib_ver = kzalloc(sizeof(char) * buf_len, GFP_KERNEL);
	if (data->lib_ver == NULL) {
		HRM_err("%s - couldn't allocate memory\n", __func__);
		mutex_unlock(&data->activelock);
		return -ENOMEM;
	}
	strlcpy(data->lib_ver, buf, buf_len);

	HRM_info("%s - lib_ver = %s\n", __func__, data->lib_ver);
	mutex_unlock(&data->activelock);

	return size;
}

static ssize_t max86915_lib_ver_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	if (data->lib_ver == NULL) {
		HRM_err("%s - data->lib_ver is NULL\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%s\n", "NULL");
	}
	HRM_info("%s - lib_ver = %s\n", __func__, data->lib_ver);
	return snprintf(buf, PAGE_SIZE, "%s\n", data->lib_ver);
}

static ssize_t max86915_threshold_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	int err = 0;

	mutex_lock(&data->activelock);
	err = kstrtoint(buf, 10, &data->hrm_threshold);
	if (err < 0) {
		HRM_err("%s - kstrtoint failed.(%d)\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	HRM_info("%s - threshold = %d\n", __func__, data->hrm_threshold);

	mutex_unlock(&data->activelock);
	return size;
}

static ssize_t max86915_threshold_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	if (data->hrm_threshold) {
		HRM_info("%s - threshold = %d\n", __func__, data->hrm_threshold);
		return snprintf(buf, PAGE_SIZE, "%d\n", data->hrm_threshold);
	}

	HRM_info("%s - threshold = 0\n", __func__);
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t max86915_prox_thd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	int err = 0;

	mutex_lock(&data->activelock);
	err = kstrtoint(buf, 10, &data->prox_threshold);
	if (err < 0) {
		HRM_err("%s - kstrtoint failed.(%d)\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	HRM_info("%s - prox threshold = %d\n", __func__, data->prox_threshold);

	mutex_unlock(&data->activelock);
	return size;
}

static ssize_t max86915_prox_thd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	if (data->prox_threshold) {
		HRM_info("%s - prox threshold = %d\n",
			__func__, data->prox_threshold);
		return snprintf(buf, PAGE_SIZE, "%d\n", data->prox_threshold);
	}

	HRM_info("%s - prox threshold = 0\n", __func__);
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t max86915_eol_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int test_onoff;
	struct max86915_device_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->activelock);
	if (sysfs_streq(buf, "1")) { /* eol_test start */
		test_onoff = 1;
		data->eol_test_type = EOL_15_MODE;
		data->enabled_mode = MODE_HRM;
	} else if (sysfs_streq(buf, "2")) { /* semi ft test start */
		test_onoff = 1;
		data->eol_test_type = EOL_SEMI_FT;
		data->enabled_mode = MODE_HRM;
	} else if (sysfs_streq(buf, "3")) {	/* xtalk test start */
		test_onoff = 1;
		data->eol_test_type = EOL_XTALK;
		data->enabled_mode = MODE_HRM;
	} else if (sysfs_streq(buf, "0")) { /* eol_test stop */
		test_onoff = 0;
		data->pre_eol_test_is_enable = 0;
		data->enabled_mode = MODE_NONE;
	} else {
		HRM_err("%s - invalid value %d\n", __func__, *buf);
		mutex_unlock(&data->activelock);
		return -EINVAL;
	}
	HRM_dbg("%s - %d\n", __func__, test_onoff);
	if (data->eol_test_is_enable == test_onoff) {
		HRM_dbg("%s - invalid eol status Pre: %d, AF : %d\n", __func__,
			data->eol_test_is_enable, test_onoff);
		mutex_unlock(&data->activelock);
		return -EINVAL;
	}
	if (max86915_eol_set_mode(data, test_onoff) < 0)
		data->eol_test_is_enable = 0;

	mutex_unlock(&data->activelock);
	return size;
}

static ssize_t max86915_eol_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", data->eol_test_is_enable);
}

static ssize_t max86915_eol_test_result_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->activelock);

	if (data->eol_test_status == 0) {
		HRM_err("%s - data->eol_test_status is NULL\n",
		__func__);
		data->eol_test_status = 0;
		mutex_unlock(&data->activelock);
		return snprintf(buf, PAGE_SIZE, "%s\n", "NO_EOL_TEST");
	}
	HRM_dbg("%s - result = %d\n", __func__, data->eol_test_status);
	data->eol_test_status = 0;

	mutex_unlock(&data->activelock);

	return snprintf(buf, PAGE_SIZE, "%s\n", data->eol_test_result);
}

static ssize_t max86915_eol_test_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", data->eol_test_status);
}

static ssize_t max86915_pre_eol_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->activelock);
	if (sysfs_streq(buf, "1")) { /* PRE EOL SYSTEM NOISE */
		data->pre_eol_test_is_enable = 1;
	} else if (sysfs_streq(buf, "2")) { /* PRE EOL FREQ */
		data->pre_eol_test_is_enable = 2;
	} else if (sysfs_streq(buf, "3")) { /* PRE EOL BOTH */
		data->pre_eol_test_is_enable = 3;
	} else if (sysfs_streq(buf, "0")) { /* pre_eol_test stop */
		data->pre_eol_test_is_enable = 0;
	} else {
		HRM_err("%s - invalid value %d\n", __func__, *buf);
		mutex_unlock(&data->activelock);
		return -EINVAL;
	}
	HRM_dbg("%s - %d\n", __func__, data->pre_eol_test_is_enable);

	mutex_unlock(&data->activelock);
	return size;
}

static ssize_t max86915_pre_eol_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", data->pre_eol_test_is_enable);
}


static ssize_t max86915_read_reg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	HRM_info("%s - val=0x%06x\n", __func__, data->reg_read_buf);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->reg_read_buf);
}

static ssize_t max86915_read_reg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	int err = -1;
	unsigned int cmd = 0;
	u8 val = 0;

	mutex_lock(&data->i2clock);
	if (data->regulator_state == 0) {
		HRM_dbg("%s - need to power on\n", __func__);
		mutex_unlock(&data->i2clock);
		return size;
	}
	err = sscanf(buf, "%8x", &cmd);
	if (err == 0) {
		HRM_err("%s - sscanf fail\n", __func__);
		mutex_unlock(&data->i2clock);
		return size;
	}

	err = max86915_read_reg(data, (u8)cmd, &val, 1);
	if (err != 0) {
		HRM_err("%s - err=%d, val=0x%06x\n",
			__func__, err, val);
		mutex_unlock(&data->i2clock);
		return size;
	}
	data->reg_read_buf = (u32)val;
	mutex_unlock(&data->i2clock);

	return size;
}
static ssize_t max86915_write_reg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	int err = -1;
	unsigned int cmd = 0;
	unsigned int val = 0;

	mutex_lock(&data->i2clock);
	if (data->regulator_state == 0) {
		HRM_dbg("%s - need to power on.\n", __func__);
		mutex_unlock(&data->i2clock);
		return size;
	}
	err = sscanf(buf, "%8x, %8x", &cmd, &val);
	if (err == 0) {
		HRM_err("%s - sscanf fail %s\n", __func__, buf);
		mutex_unlock(&data->i2clock);
		return size;
	}

	err = max86915_write_reg(data, (u8)cmd, (u8)val);
	if (err < 0) {
		HRM_err("%s - fail err = %d\n", __func__, err);
		mutex_unlock(&data->i2clock);
		return err;
	}
	mutex_unlock(&data->i2clock);

	return size;
}

static ssize_t max86915_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	HRM_info("%s - debug mode = %u\n", __func__, data->debug_mode);

	return snprintf(buf, PAGE_SIZE, "%u\n", data->debug_mode);
}

static ssize_t max86915_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	int err;
	s32 mode;

	mutex_lock(&data->activelock);
	err = kstrtoint(buf, 10, &mode);
	if (err < 0) {
		HRM_err("%s - kstrtoint failed.(%d)\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	data->debug_mode = (u8)mode;
	HRM_info("%s - mode = %d\n", __func__, mode);

	switch (data->debug_mode) {
	case DEBUG_REG_STATUS:
		max86915_print_reg_status();
		break;
	case DEBUG_ENABLE_AGC:
		agc_debug_enabled = 1;
		break;
	case DEBUG_DISABLE_AGC:
		agc_debug_enabled = 0;
		break;
	default:
		break;
	}
	mutex_unlock(&data->activelock);

	return size;
}

static ssize_t max86915_device_id_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	int err;
	u64 device_id = 0;

	if (!data->always_on) {
		err = max86915_power_ctrl(data, PWR_ON);
		if (err < 0)
			HRM_err("%s - hrm_regulator_on fail err = %d\n", __func__, err);
	}
	err = max86915_led_power_ctrl(data, PWR_ON);
	if (err)
		HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);

	mutex_lock(&data->activelock);

	max86915_get_chipid(&device_id);

	mutex_unlock(&data->activelock);

	err = max86915_disable(data, MODE_NONE);
	if (err != 0)
		HRM_err("%s - disable err : %d\n", __func__, err);

	err = max86915_led_power_ctrl(data, PWR_OFF);
	if (err)
		HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);

	if (!data->always_on) {
		err = max86915_power_ctrl(data, PWR_OFF);
		if (err < 0)
			HRM_err("%s - hrm_regulator_off fail err = %d\n", __func__, err);
	}
	return snprintf(buf, PAGE_SIZE, "%lld\n", device_id);
}

static ssize_t max86915_part_type_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	int err;
	u16 part_type = 0;

	if (!data->always_on) {
		err = max86915_power_ctrl(data, PWR_ON);
		if (err < 0)
			HRM_err("%s - hrm_regulator_on fail err = %d\n", __func__, err);
	}
	err = max86915_led_power_ctrl(data, PWR_ON);
	if (err)
		HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);

	mutex_lock(&data->activelock);

	max86915_get_part_type(&part_type);

	mutex_unlock(&data->activelock);

	err = max86915_disable(data, MODE_NONE);
	if (err != 0)
		HRM_err("%s - disable err : %d\n", __func__, err);

	err = max86915_led_power_ctrl(data, PWR_OFF);
	if (err)
		HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);

	if (!data->always_on) {
		err = max86915_power_ctrl(data, PWR_OFF);
		if (err < 0)
			HRM_err("%s - hrm_regulator_off fail err = %d\n", __func__, err);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", part_type);
}

static ssize_t max86915_i2c_err_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	u32 err_cnt = 0;

	err_cnt = data->i2c_err_cnt;

	return snprintf(buf, PAGE_SIZE, "%d\n", err_cnt);
}

static ssize_t max86915_i2c_err_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	data->i2c_err_cnt = 0;

	return size;
}

static ssize_t max86915_curr_adc_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	u16 ir_curr = 0;
	u16 red_curr = 0;
	u32 ir_adc = 0;
	u32 red_adc = 0;

	ir_curr = data->ir_curr;
	red_curr = data->red_curr;
	/*
	 * green_curr = data->green_curr;
	 * blue_curr = data->blue_curr;
	 */
	ir_adc = data->ir_adc;
	red_adc = data->red_adc;
	/*
	 * green_adc = data->green_adc;
	 * blue_adc = data->blue_adc;
	 */

	return snprintf(buf, PAGE_SIZE,
		"\"HRIC\":\"%d\",\"HRRC\":\"%d\",\"HRIA\":\"%d\",\"HRRA\":\"%d\"\n",
		ir_curr, red_curr, ir_adc, red_adc);
}

static ssize_t max86915_curr_adc_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	data->ir_curr = 0;
	data->red_curr = 0;
	data->ir_adc = 0;
	data->red_adc = 0;

	return size;
}

static ssize_t max86915_mode_cnt_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE,
		"\"CNT_HRM\":\"%d\",\"CNT_AMB\":\"%d\",\"CNT_PROX\":\"%d\",\"CNT_SDK\":\"%d\",\"CNT_CGM\":\"%d\",\"CNT_UNKN\":\"%d\"\n",
		data->mode_cnt.hrm_cnt, data->mode_cnt.amb_cnt, data->mode_cnt.prox_cnt,
		data->mode_cnt.sdk_cnt, data->mode_cnt.cgm_cnt, data->mode_cnt.unkn_cnt);
}

static ssize_t max86915_mode_cnt_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);

	data->mode_cnt.hrm_cnt = 0;
	data->mode_cnt.amb_cnt = 0;
	data->mode_cnt.prox_cnt = 0;
	data->mode_cnt.sdk_cnt = 0;
	data->mode_cnt.cgm_cnt = 0;
	data->mode_cnt.unkn_cnt = 0;

	return size;
}

static ssize_t max86915_factory_cmd_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	static int cmd_result;

	mutex_lock(&data->activelock);

	if (data->isTrimmed)
		cmd_result = 1;
	else
		cmd_result = 0;

	HRM_dbg("%s - cmd_result = %d\n", __func__, cmd_result);

	mutex_unlock(&data->activelock);

	return snprintf(buf, PAGE_SIZE, "%d\n", cmd_result);
}

static ssize_t max86915_version_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	HRM_info("%s - cmd_result = %s.%s.%s%s\n", __func__,
		VERSION, SUB_VERSION, HEADER_VERSION, VENDOR_VERSION);

	return snprintf(buf, PAGE_SIZE, "%s.%s.%s%s\n",
		VERSION, SUB_VERSION, HEADER_VERSION, VENDOR_VERSION);
}

static ssize_t max86915_sensor_info_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	HRM_dbg("%s - sensor_info_data not support\n", __func__);

	return snprintf(buf, PAGE_SIZE, "NOT SUPPORT\n");
}

static ssize_t max86915_xtalk_code_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int err;
	int xtalk1, xtalk2, xtalk3, xtalk4;
	u32 xtalk_code;
	struct max86915_device_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->activelock);
	err = sscanf(buf, "%8x", &xtalk_code);
	if (err < 0) {
		HRM_err("%s - sscanf failed, err = %x\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	xtalk1 = 0x0000001f & xtalk_code;
	xtalk2 = (0x00001f00 & xtalk_code) >> 8;
	xtalk3 = (0x001f0000 & xtalk_code) >> 16;
	xtalk4 = (0x1f000000 & xtalk_code) >> 24;
	HRM_info("%s - 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", __func__, xtalk1, xtalk2, xtalk3, xtalk4);

	err = max86915_set_xtalk(xtalk1, xtalk2, xtalk3, xtalk4);
	if (err < 0) {
		HRM_err("%s - set xtalk failed, err = %x\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	mutex_unlock(&data->activelock);

	return size;
}

static ssize_t max86915_xtalk_code_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u32 xtalk_code;
	struct max86915_device_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->activelock);

	xtalk_code = (data->xtalk_code1 & 0x1f) | ((data->xtalk_code2 & 0x1f) << 8)
		| ((data->xtalk_code3 & 0x1f) << 16) | ((data->xtalk_code4 & 0x1f) << 24);

	mutex_unlock(&data->activelock);
	HRM_info("%s - xtalk1 0x%02x, xtalk2 0x%02x, xtalk3 0x%02x, xtalk4 0x%02x\n",
		__func__, data->xtalk_code1, data->xtalk_code2, data->xtalk_code3, data->xtalk_code4);

	return snprintf(buf, PAGE_SIZE, "%08X\n", xtalk_code);
}

static DEVICE_ATTR(name, S_IRUGO, max86915_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, max86915_vendor_show, NULL);
static DEVICE_ATTR(led_current, S_IRUGO | S_IWUSR | S_IWGRP,
	max86915_led_current_show, max86915_led_current_store);
static DEVICE_ATTR(led_test, S_IRUGO, max86915_led_test_show, NULL);
static DEVICE_ATTR(hrm_flush, S_IWUSR | S_IWGRP, NULL, max86915_flush_store);
static DEVICE_ATTR(int_pin_check, S_IRUGO, max86915_int_pin_check_show, NULL);
static DEVICE_ATTR(lib_ver, S_IRUGO | S_IWUSR | S_IWGRP,
	max86915_lib_ver_show, max86915_lib_ver_store);
static DEVICE_ATTR(threshold, S_IRUGO | S_IWUSR | S_IWGRP,
	max86915_threshold_show, max86915_threshold_store);
static DEVICE_ATTR(prox_thd, S_IRUGO | S_IWUSR | S_IWGRP,
	max86915_prox_thd_show, max86915_prox_thd_store);
static DEVICE_ATTR(eol_test, S_IRUGO | S_IWUSR | S_IWGRP,
	max86915_eol_test_show, max86915_eol_test_store);
static DEVICE_ATTR(eol_test_result, S_IRUGO, max86915_eol_test_result_show, NULL);
static DEVICE_ATTR(eol_test_status, S_IRUGO, max86915_eol_test_status_show, NULL);
static DEVICE_ATTR(pre_eol_test, S_IRUGO | S_IWUSR | S_IWGRP,
	max86915_pre_eol_test_show, max86915_pre_eol_test_store);
static DEVICE_ATTR(read_reg, S_IRUGO | S_IWUSR | S_IWGRP,
	max86915_read_reg_show, max86915_read_reg_store);
static DEVICE_ATTR(write_reg, S_IWUSR | S_IWGRP, NULL, max86915_write_reg_store);
static DEVICE_ATTR(hrm_debug, S_IRUGO | S_IWUSR | S_IWGRP,
	max86915_debug_show, max86915_debug_store);
static DEVICE_ATTR(device_id, S_IRUGO, max86915_device_id_show, NULL);
static DEVICE_ATTR(part_type, S_IRUGO, max86915_part_type_show, NULL);
static DEVICE_ATTR(i2c_err_cnt, S_IRUGO | S_IWUSR | S_IWGRP, max86915_i2c_err_show, max86915_i2c_err_store);
static DEVICE_ATTR(curr_adc, S_IRUGO | S_IWUSR | S_IWGRP, max86915_curr_adc_show, max86915_curr_adc_store);
static DEVICE_ATTR(mode_cnt, S_IRUGO | S_IWUSR | S_IWGRP, max86915_mode_cnt_show, max86915_mode_cnt_store);
static DEVICE_ATTR(hrm_factory_cmd, S_IRUGO, max86915_factory_cmd_show, NULL);
static DEVICE_ATTR(hrm_version, S_IRUGO, max86915_version_show, NULL);
static DEVICE_ATTR(sensor_info, S_IRUGO, max86915_sensor_info_show, NULL);
static DEVICE_ATTR(xtalk_code, S_IRUGO | S_IWUSR | S_IWGRP,
	max86915_xtalk_code_show, max86915_xtalk_code_store);

static struct device_attribute *max86915_sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_led_current,
	&dev_attr_led_test,
	&dev_attr_hrm_flush,
	&dev_attr_int_pin_check,
	&dev_attr_lib_ver,
	&dev_attr_threshold,
	&dev_attr_prox_thd,
	&dev_attr_eol_test,
	&dev_attr_eol_test_result,
	&dev_attr_eol_test_status,
	&dev_attr_pre_eol_test,
	&dev_attr_read_reg,
	&dev_attr_write_reg,
	&dev_attr_device_id,
	&dev_attr_part_type,
	&dev_attr_i2c_err_cnt,
	&dev_attr_curr_adc,
	&dev_attr_mode_cnt,
	&dev_attr_hrm_debug,
	&dev_attr_hrm_factory_cmd,
	&dev_attr_hrm_version,
	&dev_attr_sensor_info,
	&dev_attr_xtalk_code,
	NULL,
};

irqreturn_t max86915_irq_handler(int dev_irq, void *device)
{
	int err;
	struct max86915_device_data *data = device;
	struct output_data read_data;
	int i;
	static unsigned int sample_cnt;

	HRM_info("%s - hrm_irq = %d\n", __func__, dev_irq);

	memset(&read_data, 0, sizeof(struct output_data));

	if (data->regulator_state == 0 || data->enabled_mode == 0 || data->mode_svc_enabled != 0) {
		HRM_dbg("%s - stop irq handler (reg_state : %d, enabled_mode : %d, rear_led : 0x%x)\n",
				__func__, data->regulator_state, data->enabled_mode, data->mode_svc_enabled);
		return IRQ_HANDLED;
	}

#ifdef CONFIG_ARCH_QCOM
	pm_qos_add_request(&data->pm_qos_req_fpm, PM_QOS_CPU_DMA_LATENCY,
		PM_QOS_DEFAULT_VALUE);
#endif
	err = max86915_read_data(&read_data);

	if (err == 0) {
		if (data->hrm_input_dev == NULL) {
			HRM_err("%s - hrm_input_dev is NULL\n", __func__);
		} else {
			if (read_data.fifo_num) {
				for (i = 0; i < read_data.fifo_num; i++) {
					input_report_rel(data->hrm_input_dev,
						REL_X, read_data.fifo_main[0][i] + 1);
					input_report_rel(data->hrm_input_dev,
						REL_Y,  read_data.fifo_main[1][i] + 1);
					input_report_rel(data->hrm_input_dev,
						REL_Z, read_data.fifo_main[2][i] + 1);
					input_report_rel(data->hrm_input_dev,
						REL_RX,  read_data.fifo_main[3][i] + 1);
					input_sync(data->hrm_input_dev);
				}
			} else {
				for (i = 0; i < read_data.main_num; i++)
					input_report_rel(data->hrm_input_dev,
						REL_X + i, read_data.data_main[i] + 1);

				for (i = 0; i < read_data.sub_num; i++)
					input_report_abs(data->hrm_input_dev,
						ABS_X + i, read_data.data_sub[i] + 1);

				if (read_data.main_num || read_data.sub_num)
					input_sync(data->hrm_input_dev);
			}
			if (sample_cnt++ > 100) {
				HRM_dbg("%s - mode:0x%x main:%d,%d,%d,%d sub:%d,%d,%d,%d,%d,%d,%d,%d\n", __func__,
						read_data.mode, read_data.data_main[0], read_data.data_main[1],
						read_data.data_main[2], read_data.data_main[3], read_data.data_sub[0],
						read_data.data_sub[1], read_data.data_sub[2], read_data.data_sub[3], read_data.data_sub[4],
						read_data.data_sub[5], read_data.data_sub[6], read_data.data_sub[7]);
				sample_cnt = 0;
			} else {
				HRM_info("%s - mode:0x%x main:%d,%d,%d,%d sub:%d,%d,%d,%d,%d,%d,%d,%d\n", __func__,
					read_data.mode, read_data.data_main[0], read_data.data_main[1],
					read_data.data_main[2], read_data.data_main[3], read_data.data_sub[0],
					read_data.data_sub[1], read_data.data_sub[2], read_data.data_sub[3], read_data.data_sub[4],
					read_data.data_sub[5], read_data.data_sub[6], read_data.data_sub[7]);
			}
		}
	}
#ifdef CONFIG_ARCH_QCOM
	pm_qos_remove_request(&data->pm_qos_req_fpm);
#endif

	return IRQ_HANDLED;
}

static int max86915_setup_irq(struct max86915_device_data *data)
{
	int errorno = -EIO;

	errorno = request_threaded_irq(data->dev_irq, NULL,
		max86915_irq_handler, IRQF_TRIGGER_FALLING|IRQF_ONESHOT,
		"hrm_sensor_irq", data);

	if (errorno < 0) {
		HRM_err("%s - failed for setup dev_irq errono= %d\n",
			   __func__, errorno);
		errorno = -ENODEV;
		return errorno;
	}
	max86915_pin_control(data, false);

	return errorno;
}

static void max86915_init_var1(struct max86915_device_data *data)
{
	data->client = NULL;
	data->dev = NULL;
	data->hrm_input_dev = NULL;
	data->hrm_pinctrl = NULL;
	data->pins_sleep = NULL;
	data->pins_idle = NULL;
	data->led_3p3 = NULL;
	data->vdd_1p8 = NULL;
	data->i2c_1p8 = NULL;
	data->enabled_mode = 0;
	data->sampling_period_ns = 0;
	data->mode_sdk_enabled = 0;
	data->mode_svc_enabled = 0;
	data->regulator_state = 0;
	data->irq_state = 0;
	data->hrm_threshold = DEFAULT_THRESHOLD;
	data->eol_test_is_enable = 0;
	data->eol_test_status = 0;
	data->pre_eol_test_is_enable = 0;
	data->reg_read_buf = 0;
	data->lib_ver = NULL;
	data->pm_state = PM_RESUME;
}

static int max86915_init_var2(struct max86915_device_data *data)
{
	int err;
	u8 buffer[2] = {0, };

	err = max86915_read_reg(data, MAX86915_REV_ID_REG, buffer, 2);
	if (err) {
		HRM_err("%s MAX86915 WHOAMI read fail0\n", __func__);
		return -EINVAL;
	}
	data->threshold_default = MAX86915_THRESHOLD_DEFAULT;
	data->i2c_err_cnt = 0;
	data->ir_curr = 0;
	data->red_curr = 0;
	data->green_curr = 0;
	data->blue_curr = 0;
	data->ir_adc = 0;
	data->red_adc = 0;
	data->green_adc = 0;
	data->blue_adc = 0;
	data->led_range[0] = MAX86915_DEFAULT_LED_RGE1;
	data->led_range[1] = MAX86915_DEFAULT_LED_RGE2;
	data->led_range[2] = MAX86915_DEFAULT_LED_RGE3;
	data->led_range[3] = MAX86915_DEFAULT_LED_RGE4;

	if (data->init_current[AGC_IR] == 0) {
		data->init_current[AGC_IR] = MAX86915_IR_INIT_CURRENT;
		data->init_current[AGC_RED] = MAX86915_RED_INIT_CURRENT;
		data->init_current[AGC_GREEN] = MAX86915_GREEN_INIT_CURRENT;
		data->init_current[AGC_BLUE] = MAX86915_BLUE_INIT_CURRENT;
	}

	if (buffer[1] == MAX86915_PART_ID) {
		data->default_current1 = data->init_current[AGC_IR];
		data->default_current2 = data->init_current[AGC_RED];
		data->default_current3 = data->init_current[AGC_GREEN];
		data->default_current4 = data->init_current[AGC_BLUE];

		data->default_xtalk_code1 = MAX86915_DEFAULT_XTALK_CODE1;
		data->default_xtalk_code2 = MAX86915_DEFAULT_XTALK_CODE2;
		data->default_xtalk_code3 = MAX86915_DEFAULT_XTALK_CODE3;
		data->default_xtalk_code4 = MAX86915_DEFAULT_XTALK_CODE4;

		data->part_type = max86915_get_part_id(data);
	} else {
		HRM_err("%s MAX86915 WHOAMI read fail\n", __func__);
		return -EINVAL;
	}
	/* AGC setting */
	data->update_led = max86915_update_led_current;
	data->agc_led_out_percent = MAX86915_AGC_DEFAULT_LED_OUT_RANGE;
	data->agc_corr_coeff = MAX86915_AGC_DEFAULT_CORRECTION_COEFF;
	data->agc_min_num_samples = MAX86915_AGC_DEFAULT_MIN_NUM_PERCENT;
	data->agc_sensitivity_percent = MAX86915_AGC_DEFAULT_SENSITIVITY_PERCENT;
	data->threshold_default = MAX86915_THRESHOLD_DEFAULT;
	data->remove_cnt[AGC_IR] = 0;
	data->remove_cnt[AGC_RED] = 0;
	data->remove_cnt[AGC_GREEN] = 0;
	data->remove_cnt[AGC_BLUE] = 0;

	return 0;
}

static int max86915_parse_dt(struct max86915_device_data *data)
{
	struct device *dev = &data->client->dev;
	struct device_node *dNode = dev->of_node;
	enum of_gpio_flags flags;
	u32 threshold[2];
	int i;

	if (dNode == NULL)
		return -ENODEV;

	data->pin_hrm_int = of_get_named_gpio_flags(dNode,
		"hrmsensor,hrm_int-gpio", 0, &flags);
	if (data->pin_hrm_int < 0) {
		HRM_err("%s - get hrm_int error\n", __func__);
		return -ENODEV;
	}

	data->pin_hrm_en = of_get_named_gpio_flags(dNode,
	"hrmsensor,hrm_boost_en-gpio", 0, &flags);
	if (data->pin_hrm_en < 0) {
		HRM_err("%s - get hrm_en error\n", __func__);
		return -ENODEV;
	}

	if (of_property_read_string(dNode, "hrmsensor,vdd_1p8",
		(char const **)&data->vdd_1p8) < 0)
		HRM_dbg("%s - vdd_1p8 doesn`t exist\n", __func__);

	if (of_property_read_string(dNode, "hrmsensor,i2c_1p8",
		(char const **)&data->i2c_1p8) < 0)
		HRM_dbg("%s -  i2c_1p8 doesn`t exist\n", __func__);

	data->hrm_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(data->hrm_pinctrl)) {
		HRM_err("%s - get pinctrl(%li) error\n",
			__func__, PTR_ERR(data->hrm_pinctrl));
		data->hrm_pinctrl = NULL;
		return -EINVAL;
	}

	data->pins_sleep =
		pinctrl_lookup_state(data->hrm_pinctrl, "sleep");
	if (IS_ERR_OR_NULL(data->pins_sleep)) {
		HRM_err("%s - get pins_sleep(%li) error\n",
			__func__, PTR_ERR(data->pins_sleep));
		devm_pinctrl_put(data->hrm_pinctrl);
		data->pins_sleep = NULL;
		return -EINVAL;
	}

	data->pins_idle =
		pinctrl_lookup_state(data->hrm_pinctrl, "idle");
	if (IS_ERR_OR_NULL(data->pins_idle)) {
		HRM_err("%s - get pins_idle(%li) error\n",
			__func__, PTR_ERR(data->pins_idle));

		devm_pinctrl_put(data->hrm_pinctrl);
		data->pins_idle = NULL;
		return -EINVAL;
	}

	data->always_on = of_property_read_bool(dev->of_node, "hrmsensor,always");
	if (data->always_on)
		HRM_dbg("%s - VDD_1p8 always on\n", __func__);

	if (of_property_read_u32_array(dNode, "hrmsensor,thd",
		threshold, ARRAY_SIZE(threshold)) < 0) {
		HRM_err("%s - get threshold error\n", __func__);
	} else {
		data->hrm_threshold = threshold[0];
		data->prox_threshold = threshold[1];
	}
	HRM_dbg("%s - hrm_threshold = %d, prox_threshold = %d\n",
		__func__, data->hrm_threshold, data->prox_threshold);

	if (of_property_read_u32_array(dNode, "hrmsensor,init_curr",
		data->init_current, ARRAY_SIZE(data->init_current)) < 0) {
		HRM_err("%s - get init_curr error\n", __func__);

		data->init_current[AGC_IR] = 0;
		data->init_current[AGC_RED] = 0;
		data->init_current[AGC_GREEN] = 0;
		data->init_current[AGC_BLUE] = 0;
	}
	HRM_dbg("%s - init_curr = 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n", __func__,
		data->init_current[AGC_IR], data->init_current[AGC_RED],
		data->init_current[AGC_GREEN], data->init_current[AGC_BLUE]);

	if (of_property_read_u32_array(dNode, "hrmsensor,eol",
		 data->eol_spec, ARRAY_SIZE(data->eol_spec)) < 0) {
		HRM_err("%s - get eol_spec error\n", __func__);
	}
	for (i=0 ; i < ARRAY_SIZE(data->eol_spec) ; i+=2) {
		HRM_info("%s - eol_spec : min : %d, max %d\n", __func__, data->eol_spec[i], data->eol_spec[i+1]);
	}

	if (of_property_read_u32_array(dNode, "hrmsensor,eol_semi",
		 data->eol_semi_spec, ARRAY_SIZE(data->eol_semi_spec)) < 0) {
		HRM_err("%s - get eol_semi_spec error\n", __func__);
	}
	for (i=0 ; i < ARRAY_SIZE(data->eol_semi_spec) ; i+=2) {
		HRM_info("%s - semi_spec : min : %d, max %d\n",
			__func__, data->eol_semi_spec[i], data->eol_semi_spec[i+1]);
	}

	if (of_property_read_u32_array(dNode, "hrmsensor,eol_xtalk",
		 data->eol_xtalk_spec, ARRAY_SIZE(data->eol_xtalk_spec)) < 0) {
		HRM_err("%s - get eol_xtalk_spec error\n", __func__);
	}
	HRM_info("%s - eol_xtalk_spec : min : %d, max %d\n", __func__, data->eol_spec[0], data->eol_spec[1]);

	return 0;
}

static int max86915_setup_gpio(struct max86915_device_data *data)
{
	int errorno = -EIO;

	errorno = gpio_request(data->pin_hrm_int, "hrm_int");
	if (errorno) {
		HRM_err("%s - failed to request hrm_int\n", __func__);
		return errorno;
	}

	errorno = gpio_direction_input(data->pin_hrm_int);
	if (errorno) {
		HRM_err("%s - failed to set hrm_int as input\n", __func__);
		goto err_gpio_direction_input;
	}
	data->dev_irq = gpio_to_irq(data->pin_hrm_int);

	errorno = gpio_request(data->pin_hrm_en, "hrm_en");
	if (errorno) {
		HRM_err("%s - failed to request hrm_en\n", __func__);
		goto err_gpio_direction_input;
	}
	goto done;

err_gpio_direction_input:
	gpio_free(data->pin_hrm_int);
done:
	return errorno;
}

static int max86915_trim_check(void)
{
	u8 recvData;
	u8 reg93_val;
	int err;

	max86915_data->isTrimmed = 0;

	err = max86915_write_reg(max86915_data, 0xFF, 0x54);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_TEST0!\n",
			__func__);
		return -EIO;
	}

	err = max86915_write_reg(max86915_data, 0xFF, 0x4d);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_TEST1!\n",
			__func__);
		return -EIO;
	}

	err = max86915_read_reg(max86915_data, 0x93, &recvData, 1);
	if (err != 0) {
		HRM_err("%s - max86915_read_reg err:%d, address:0x%02x\n",
			__func__, err, recvData);
		return -EIO;
	}
	reg93_val = recvData;

	err = max86915_write_reg(max86915_data, 0xFF, 0x00);
	if (err != 0) {
		HRM_err("%s - error initializing MAX86915_MODE_TEST3!\n",
			__func__);
		return -EIO;
	}

	max86915_data->isTrimmed = (reg93_val & 0x20);
	HRM_dbg("%s - isTrimmed :%d, reg93_val : %d\n", __func__, max86915_data->isTrimmed, reg93_val);

	return err;
}

int max86915_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = -ENODEV;
	struct max86915_device_data *data;

	HRM_dbg("%s - start\n", __func__);
	/* check to make sure that the adapter supports I2C */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		HRM_err("%s - I2C_FUNC_I2C not supported\n", __func__);
		return -ENODEV;
	}
	/* allocate some memory for the device */
	data = kzalloc(sizeof(struct max86915_device_data), GFP_KERNEL);
	if (data == NULL) {
		HRM_err("%s - couldn't allocate device data memory\n", __func__);
		return -ENOMEM;
	}
	data->flicker_data = kzalloc(sizeof(int)*FLICKER_DATA_CNT, GFP_KERNEL);
	if (data->flicker_data == NULL) {
		HRM_err("%s - couldn't allocate flicker data memory\n", __func__);
		goto err_flicker_alloc_fail;
	}

	data->fifo_buf = kzalloc(NUM_BYTES_PER_SAMPLE*MAX86915_FIFO_SIZE*MAX_LED_NUM, GFP_KERNEL);
	if (data->fifo_buf == NULL) {
		HRM_err("%s - couldn't allocate fifo_buf data memory\n", __func__);
		goto err_fifo_buf_alloc_fail;
	}

	max86915_data = data;
	max86915_init_var1(data);

	data->client = client;
	data->miscdev.minor = MISC_DYNAMIC_MINOR;
	data->miscdev.name = "max_hrm";
	data->miscdev.fops = &max86915_fops;
	data->miscdev.mode = S_IRUGO;
	i2c_set_clientdata(client, data);
	HRM_info("%s client  = %p\n", __func__, client);

	err = misc_register(&data->miscdev);
	if (err < 0) {
		HRM_err("%s - failed to misc device register\n", __func__);
		goto err_misc_register;
	}
	mutex_init(&data->i2clock);
	mutex_init(&data->activelock);
	mutex_init(&data->suspendlock);
	mutex_init(&data->flickerdatalock);

	err = max86915_parse_dt(data);
	if (err < 0) {
		HRM_err("%s - failed to parse dt\n", __func__);
		err = -ENODEV;
		goto err_parse_dt;
	}
	err = max86915_setup_gpio(data);
	if (err) {
		HRM_err("%s - failed to setup gpio\n", __func__);
		goto err_setup_gpio;
	}

	err = max86915_power_ctrl(data, PWR_ON);
	if (err < 0) {
		HRM_err("%s - failed to power on ctrl\n", __func__);
		goto err_power_on;
	}
	
	err = max86915_led_power_ctrl(data, PWR_ON);
	if (err) {
		HRM_err("%s - max86915_led_power_ctrl fail err = %d\n", __func__, err);
		goto err_led_power_ctrl_fail;
	}

	if (data->client->addr != SLAVE_ADDR_MAX) {
		err = -EIO;
		HRM_err("%s - slave address error, 0x%02x\n", __func__, data->client->addr);
		goto err_init_fail;
	}
	err = max86915_init_var2(data);
	if (err < 0) {
		HRM_err("%s - failed to init_var2\n", __func__);
		goto err_init_fail;
	}
	err = max86915_init_status(data);
	if (err < 0) {
		HRM_err("%s - failed to init_status\n", __func__);
		goto err_init_fail;
	}

	max86915_trim_check();

	data->hrm_input_dev = input_allocate_device();
	if (!data->hrm_input_dev) {
		HRM_err("%s - could not allocate input device\n", __func__);
		goto err_input_allocate_device;
	}
	data->hrm_input_dev->name = MODULE_NAME_HRM;
	input_set_drvdata(data->hrm_input_dev, data);
	input_set_capability(data->hrm_input_dev, EV_REL, REL_X);
	input_set_capability(data->hrm_input_dev, EV_REL, REL_Y);
	input_set_capability(data->hrm_input_dev, EV_REL, REL_Z);
	input_set_capability(data->hrm_input_dev, EV_REL, REL_RX);
	input_set_capability(data->hrm_input_dev, EV_REL, REL_MISC);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_X);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_Y);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_Z);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_RX);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_RY);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_RZ);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_THROTTLE);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_RUDDER);

	err = input_register_device(data->hrm_input_dev);
	if (err < 0) {
		input_free_device(data->hrm_input_dev);
		HRM_err("%s - could not register input device\n", __func__);
		goto err_input_register_device;
	}
#ifdef CONFIG_ARCH_QCOM
	err = sensors_create_symlink(&data->hrm_input_dev->dev.kobj,
				data->hrm_input_dev->name);
#else
	err = sensors_create_symlink(data->hrm_input_dev);
#endif
	if (err < 0) {
		HRM_err("%s - could not create_symlink\n", __func__);
		goto err_sensors_create_symlink;
	}
	err = sysfs_create_group(&data->hrm_input_dev->dev.kobj,
				 &hrm_attribute_group);
	if (err) {
		HRM_err("%s - could not create sysfs group\n", __func__);
		goto err_sysfs_create_group;
	}
#ifdef CONFIG_ARCH_QCOM
	err = sensors_register(&data->dev, data, max86915_sensor_attrs,
			MODULE_NAME_HRM);
#else
	err = sensors_register(data->dev, data, max86915_sensor_attrs,
			MODULE_NAME_HRM);
#endif
	if (err) {
		HRM_err("%s - cound not register hrm_sensor(%d).\n", __func__, err);
		goto hrm_sensor_register_failed;
	}

	err = max86915_setup_irq(data);
	if (err) {
		HRM_err("%s - could not setup dev_irq\n", __func__);
		goto err_setup_irq;
	}

	err = max86915_disable(data, MODE_NONE);
	if (err != 0) {
		HRM_err("%s - max86915_disable err : %d\n", __func__, err);
		goto dev_set_drvdata_failed;
	}
	err = max86915_led_power_ctrl(data, PWR_OFF);
	if (err) {
		HRM_err("%s - failed to power off ctrl\n", __func__);
		goto dev_set_drvdata_failed;
	}

	if (!data->always_on) {
		err = max86915_power_ctrl(data, PWR_OFF);
		if (err < 0) {
			HRM_err("%s - failed to power off ctrl\n", __func__);
			goto dev_set_drvdata_failed;
		}
	}
	HRM_dbg("%s - success\n", __func__);
	goto done;

dev_set_drvdata_failed:
	free_irq(data->dev_irq, data);
err_setup_irq:
	sensors_unregister(data->dev, max86915_sensor_attrs);
hrm_sensor_register_failed:
	sysfs_remove_group(&data->hrm_input_dev->dev.kobj,
			   &hrm_attribute_group);
err_sysfs_create_group:
#ifdef CONFIG_ARCH_QCOM
	sensors_remove_symlink(&data->hrm_input_dev->dev.kobj,
				data->hrm_input_dev->name);
#else
	sensors_remove_symlink(data->hrm_input_dev);
#endif
err_sensors_create_symlink:
	input_unregister_device(data->hrm_input_dev);
err_input_register_device:
err_input_allocate_device:
err_init_fail:
	max86915_led_power_ctrl(data, PWR_OFF);
err_led_power_ctrl_fail:
	max86915_power_ctrl(data, PWR_OFF);
err_power_on:
	gpio_free(data->pin_hrm_int);
	gpio_free(data->pin_hrm_en);
err_setup_gpio:
err_parse_dt:
	if (data->hrm_pinctrl) {
		devm_pinctrl_put(data->hrm_pinctrl);
		data->hrm_pinctrl = NULL;
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
	kfree(data->fifo_buf);
err_fifo_buf_alloc_fail:
	kfree(data->flicker_data);
err_flicker_alloc_fail:
	kfree(data);
	HRM_err("%s failed\n", __func__);
done:
	return err;
}


int max86915_remove(struct i2c_client *client)
{
	struct max86915_device_data *data = i2c_get_clientdata(client);

	HRM_dbg("%s - start\n", __func__);
	max86915_power_ctrl(data, PWR_OFF);

	sensors_unregister(data->dev, max86915_sensor_attrs);
	sysfs_remove_group(&data->hrm_input_dev->dev.kobj,
			   &hrm_attribute_group);
#ifdef CONFIG_ARCH_QCOM
	sensors_remove_symlink(&data->hrm_input_dev->dev.kobj,
				data->hrm_input_dev->name);
#else
	sensors_remove_symlink(data->hrm_input_dev);
#endif
	input_unregister_device(data->hrm_input_dev);

	if (data->hrm_pinctrl) {
		devm_pinctrl_put(data->hrm_pinctrl);
		data->hrm_pinctrl = NULL;
	}
	if (data->pins_idle)
		data->pins_idle = NULL;
	if (data->pins_sleep)
		data->pins_sleep = NULL;
	disable_irq(data->dev_irq);
	free_irq(data->dev_irq, data);
	gpio_free(data->pin_hrm_int);
	gpio_free(data->pin_hrm_en);
	mutex_destroy(&data->i2clock);
	mutex_destroy(&data->activelock);
	mutex_destroy(&data->suspendlock);
	mutex_destroy(&data->flickerdatalock);
	misc_deregister(&data->miscdev);

	if (data->eol_test_result != NULL)
		kfree(data->eol_test_result);
	kfree(data->fifo_buf);
	kfree(data->flicker_data);
	kfree(data->lib_ver);
	kfree(data);
	i2c_set_clientdata(client, NULL);

	data = NULL;
	return 0;
}

static void max86915_shutdown(struct i2c_client *client)
{
	HRM_dbg("%s - start\n", __func__);
}

#ifdef CONFIG_PM
static int max86915_pm_suspend(struct device *dev)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	int err = 0;

	HRM_dbg("%s - %d\n", __func__, data->enabled_mode);

	if (data->enabled_mode == MODE_SVC_IR) {
		return err;
	}

	if (data->enabled_mode != 0 || data->regulator_state != 0) {
		mutex_lock(&data->activelock);

		err = max86915_disable(data, MODE_NONE);
		if (err != 0)
			HRM_err("%s - disable err : %d\n", __func__, err);

		err = max86915_led_power_ctrl(data, PWR_OFF);
		if (err)
			HRM_err("%s - failed to power off ctrl\n", __func__);

		if (!data->always_on) {
			err = max86915_power_ctrl(data, PWR_OFF);
			if (err < 0)
				HRM_err("%s - hrm_regulator_off fail err = %d\n",
					__func__, err);
		}
		max86915_irq_set_state(data, PWR_OFF);

		mutex_unlock(&data->activelock);
	}
	mutex_lock(&data->suspendlock);

	data->pm_state = PM_SUSPEND;

	mutex_unlock(&data->suspendlock);

	return err;
}

static int max86915_pm_resume(struct device *dev)
{
	struct max86915_device_data *data = dev_get_drvdata(dev);
	int err = 0;

	HRM_dbg("%s - %d\n", __func__, data->enabled_mode);

	if (data->enabled_mode == MODE_SVC_IR) {
		return err;
	}

	mutex_lock(&data->suspendlock);

	data->pm_state = PM_RESUME;

	mutex_unlock(&data->suspendlock);

	if (data->enabled_mode != 0) {
		mutex_lock(&data->activelock);

		max86915_irq_set_state(data, PWR_ON);

		if (!data->always_on) {
			err = max86915_power_ctrl(data, PWR_ON);
			if (err < 0)
				HRM_err("%s - hrm_regulator_on fail err = %d\n", __func__, err);
		}
		err = max86915_led_power_ctrl(data, PWR_ON);
		if (err < 0)
			HRM_err("%s - failed to power off ctrl\n", __func__);

		err = max86915_enable(data, data->enabled_mode);
		if (err != 0)
			HRM_err("%s - enable err : %d\n", __func__, err);

		if (err < 0 && data->enabled_mode == MODE_AMBIENT) {
			input_report_rel(data->hrm_input_dev,
				REL_Y, -5 + 1); /* F_ERR_I2C -5 detected i2c error */
			input_sync(data->hrm_input_dev);
			HRM_err("%s - awb mode enable error : %d\n", __func__, err);
		}

		mutex_unlock(&data->activelock);
	}
	return err;
}

static const struct dev_pm_ops max86915_pm_ops = {
	.suspend = max86915_pm_suspend,
	.resume = max86915_pm_resume
};
#endif

static const struct of_device_id max86915_match_table[] = {
	{ .compatible = "hrm_max",},
	{},
};

static const struct i2c_device_id max86915_device_id[] = {
	{ "max86915", 0 },
	{ }
};
/* descriptor of the hrmsensor I2C driver */
static struct i2c_driver max86915_i2c_driver = {
	.driver = {
	    .name = "max86915",
	    .owner = THIS_MODULE,
#if defined(CONFIG_PM)
	    .pm = &max86915_pm_ops,
#endif
	    .of_match_table = max86915_match_table,
	},
	.probe = max86915_probe,
	.remove = max86915_remove,
	.shutdown = max86915_shutdown,
	.id_table = max86915_device_id,
};

/* initialization and exit functions */
static int __init max86915_init(void)
{
	if (!lpcharge)
		return i2c_add_driver(&max86915_i2c_driver);
	else
		return 0;
}

static void __exit max86915_exit(void)
{
	i2c_del_driver(&max86915_i2c_driver);
}

module_init(max86915_init);
module_exit(max86915_exit);

MODULE_DESCRIPTION("max86915 Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");

