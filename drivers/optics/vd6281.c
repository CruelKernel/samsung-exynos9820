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

#define VENDOR				"STM"

#define VD6281_CHIP_NAME	"VD6281"

#define VERSION				"0"
#define SUB_VERSION			"1"
#define VENDOR_VERSION		"s"

#define MODULE_NAME_ALS		"als_rear"

#define VD6281_SLAVE_I2C_ADDR 0x20

#define VD6281_I2C_RETRY_DELAY	10
#define VD6281_I2C_MAX_RETRIES	5

#if defined(CONFIG_LEDS_S2MPB02)
#define CONFIG_STM_ALS_EOL_MODE
#endif

#include "vd6281.h"

#ifdef CONFIG_STM_ALS_EOL_MODE
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

#define ALS_CHANNELS	(STALS_CHANNEL_1|STALS_CHANNEL_3|STALS_CHANNEL_4|STALS_CHANNEL_5|STALS_CHANNEL_2)
#define FLK_CHANNEL		STALS_CHANNEL_6

static int als_debug = 1;
static int als_info;

module_param(als_debug, int, S_IRUGO | S_IWUSR);
module_param(als_info, int, S_IRUGO | S_IWUSR);

static struct vd6281_device_data *vd6281_data;

/* only gcc compiler is supported */
/*Kernel / user code compatibility macro */
#ifndef __KERNEL__
#define POPCOUNT(a)				__builtin_popcount(a)
#define div64_u64(a, b)				((a) / (b))
#else
#define POPCOUNT(a)				hweight_long(a)
#define assert(c)				BUG_ON(c)
#endif
#define UNUSED_P(x)				x __attribute__((unused))

#define VERSION_MAJOR				1
#define VERSION_MINOR				0
#define VERSION_REVISION			0

#define VD6281_DEVICE				0x70
#define VD6281_REVISION_CUT_2_0			0xBA
#define VD6281_REVISION_CUT_2_1			0xBB

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)				(sizeof(a) / sizeof(a[0]))
#endif

#define VD6281_DC_CHANNELS_MASK			0x1f
#define VD6281_AC_CHANNELS_MASK			0x20

#define VD6281_DEFAULT_HF_TRIM			0x0e3
#define VD6281_DEFAULT_LF_TRIM			0x07
#define VD6281_DEFAULT_FILTER_CONFIG		0
#define VD6281_DEFAULT_GAIN			0x80

#define VD6281_OTP_V1				0x7
#define VD6281_OTP_V2				0x6

#ifndef MAX
#define MAX(a, b)				((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)				((a) < (b) ? (a) : (b))
#endif

#define WA_460857				460857
#define WA_462997				462997
#define WA_461428				461428
#define WA_464174				464174
#define WA_478654				478654

static const uint16_t GainRange[] = {
	0x42AB,		//   66, 67
	0x3200,		//   50, 00
	0x2154,		//   33, 33
	0x1900,		//   25, 00
	0x10AB,		//   16, 67
	0x0A00,		//   10, 00
	0x0723,		//    7, 14
	0x0500,		//    5, 00
	0x0354,		//    3, 33
	0x0280,		//    2, 50
	0x01AB,		//    1, 67
	0x0140,		//    1, 25
	0x0100,		//    1, 00
	0x00D4,		//    0, 83
	0x00B5		//    0, 71
};

static const uint16_t GainRangeThreshold[] = {  //14
	0x3A56,
	0x29AB,
	0x1D2B,
	0x14D6,
	0x0D56,
	0x0892,
	0x0612,
	0x042B,
	0x02EB,
	0x0216,
	0x0176,
	0x0121,
	0x00EB,
	0x00C5
};

//static struct VD6281_device devices[VD6281_CONFIG_DEVICES_MAX];

static const enum STALS_Color_Id_t
channel_2_color_default[VD6281_CHANNEL_NB] = {
	STALS_COLOR_CLEAR, STALS_COLOR_CLEAR, STALS_COLOR_CLEAR,
	STALS_COLOR_CLEAR, STALS_COLOR_CLEAR, STALS_COLOR_CLEAR
};

static const enum STALS_Color_Id_t
channel_2_color_uv_rgb_ir[VD6281_CHANNEL_NB] = {
	STALS_COLOR_RED, STALS_COLOR_GREEN, STALS_COLOR_BLUE,
	STALS_COLOR_UV, STALS_COLOR_IR, STALS_COLOR_CLEAR
};

static const enum STALS_Color_Id_t *filter_mapping_array[8] = {
	channel_2_color_default, channel_2_color_default,
	channel_2_color_uv_rgb_ir, channel_2_color_default,
	channel_2_color_default, channel_2_color_uv_rgb_ir,
	channel_2_color_default, channel_2_color_default
};

#define CHECK_DEVICE_VALID(d) \
do { \
	if (!d) \
		return STALS_ERROR_INVALID_DEVICE_ID; \
} while (0)

#define CHECK_NULL_PTR(a) \
do { \
	if (!a) \
		return STALS_ERROR_INVALID_PARAMS; \
} while (0)

#define CHECK_CHANNEL_VALID(idx) \
do { \
	if ((idx) >= VD6281_CHANNEL_NB) \
		return STALS_ERROR_INVALID_PARAMS; \
} while (0)

#define CHECH_DEV_ST_INIT(d) \
do { \
	if (d->st != DEV_INIT) \
		return STALS_ERROR_ALREADY_STARTED; \
} while (0)

#define CHECK_DEV_ST_FLICKER_STARTED(d) \
do { \
	if (d->st != DEV_FLICKER_RUN && d->st != DEV_BOTH_RUN) \
		return STALS_ERROR_NOT_STARTED; \
} while (0)

#define CHECH_DEV_ST_FLICKER_NOT_STARTED(d) \
do { \
	if (d->st == DEV_FLICKER_RUN) \
		return STALS_ERROR_ALREADY_STARTED; \
	if (d->st == DEV_BOTH_RUN) \
		return STALS_ERROR_ALREADY_STARTED; \
} while (0)

#define CHECK_DEV_ST_ALS_STARTED(d) \
do { \
	if (d->st != DEV_ALS_RUN && d->st != DEV_BOTH_RUN) \
		return STALS_ERROR_NOT_STARTED; \
} while (0)

#define CHECH_DEV_ST_ALS_NOT_STARTED(d) \
do { \
	if (d->st == DEV_ALS_RUN) \
		return STALS_ERROR_ALREADY_STARTED; \
	if (d->st == DEV_BOTH_RUN) \
		return STALS_ERROR_ALREADY_STARTED; \
} while (0)

#define CHECK_CHANNEL_MASK_VALID_FOR_ALS(m) \
do { \
	/* check there is no invalid channel selected */  \
	if ((m) & ~((1 << STALS_ALS_MAX_CHANNELS) - 1)) \
		return STALS_ERROR_INVALID_PARAMS; \
} while (0)

#define CHECK_CHANNEL_MASK_VALID(m) \
do { \
	/* check there is no invalid channel selected */  \
	if ((m) & ~((1 << STALS_ALS_MAX_CHANNELS) - 1)) \
		return STALS_ERROR_INVALID_PARAMS; \
	/* check there is at least one channel */ \
	if (!(m)) \
		return STALS_ERROR_INVALID_PARAMS; \
} while (0)

#define CHECK_CHANNEL_NOT_IN_USE(d, c) \
do { \
	if (d->als.chan & (c)) \
		return STALS_ERROR_INVALID_PARAMS; \
	if (d->flk.chan & (c)) \
		return STALS_ERROR_INVALID_PARAMS; \
} while (0)

#define US_TO_NS(x)	(x * 1E3L)
#define ALS_WORK_DELAY 500

static void vd6281_debug_var(struct vd6281_device_data *data)
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


static DECLARE_WORK(vd6281_flicker_work, vd6281_flicker_work_cb);

int vd6281_get_adc_data()
{
	int adc_data, ret;
	ret = iio_read_channel_raw(vd6281_data->chan, &adc_data);
	if (ret < 0)
		ALS_err("%s : err(%d) returned, skip read\n", __func__, adc_data);

	return adc_data;
}
void vd6281_flicker_work_cb(struct work_struct *w)
{
	int adc = vd6281_get_adc_data();
	vd6281_data->flicker_data[vd6281_data->flicker_cnt - 1] = adc;

	if (vd6281_data->flicker_cnt == 199) {
		input_report_rel(vd6281_data->als_input_dev, REL_RZ, -1);
		input_sync(vd6281_data->als_input_dev);
	}
//	ALS_dbg("===== %s =====(adc_data : %d) cnt  : %d\n", __func__,
//		adc, vd6281_data->flicker_cnt - 1);
}

enum hrtimer_restart my_hrtimer_callback(struct hrtimer *timer)
{
	int ret;
	vd6281_data->flicker_cnt++;

	ret = queue_work(vd6281_data->flicker_work, &vd6281_flicker_work);
	if (ret == 0)
		ALS_err("failed in queue_work\n");

	hrtimer_forward(&vd6281_data->timer.timer, hrtimer_cb_get_time(&vd6281_data->timer.timer), ktime_set(0, US_TO_NS(2500)));
	return HRTIMER_RESTART;
}

int start_flicker_timer(void)
{
	vd6281_data->flicker_cnt = 0;
	tasklet_hrtimer_start(&vd6281_data->timer, ktime_set(0, US_TO_NS(2500)), HRTIMER_MODE_REL);

	return 0;
}

void stop_flicker_timer(void)
{
	tasklet_hrtimer_cancel(&vd6281_data->timer);
	ALS_dbg("%s, timer cleanup success\n", __func__);
}

static void vd6281_als_work(struct work_struct *w)
{
	STALS_ErrCode_t res;
	struct delayed_work *work_queue;
//	struct VD6281_device *dev;
	struct STALS_Als_t meas;
	uint8_t is_valid;
	unsigned long timeout;
	ALS_dbg("%s\n", __func__);

	work_queue = container_of(w, struct delayed_work, work);
//	dev = container_of(work_queue, struct vd6281_device_data, rear_als_work);


	timeout = wait_for_completion_timeout(&vd6281_data->completion, VD6281_ADC_TIMEOUT);
	if (timeout == 0) {
		ALS_err("%s  wait_for_completion_timeout err err err errr\n", __func__);
	}
	stop_flicker_timer();
//	if (work_pending(&vd6281_data->flicker_work)) {
//		ALS_dbg("Gas - cancel_delayed_work\n");
//		cancel_work(&vd6281_data->flicker_work);
//	}

	res = STALS_GetAlsValues(vd6281_data->devices.hdl, ALS_CHANNELS, &meas, &is_valid);
	res = STALS_Stop(vd6281_data->devices.hdl, STALS_MODE_ALS_SINGLE_SHOT);
	res = STALS_Stop(vd6281_data->devices.hdl, STALS_MODE_FLICKER);
	ALS_dbg("%s IR(%d), RED(%d), GREEN(%d), BLUE(%d), CLEAR(%d)\n", __func__,
		meas.CountValue[STALS_COLOR_IR-1],
		meas.CountValue[STALS_COLOR_RED-1],
		meas.CountValue[STALS_COLOR_GREEN-1],
		meas.CountValue[STALS_COLOR_BLUE-1],
		meas.CountValue[STALS_COLOR_CLEAR-1]);
	input_report_rel(vd6281_data->als_input_dev, REL_X, meas.CountValue[STALS_COLOR_IR-1] + 1);
	input_report_rel(vd6281_data->als_input_dev, REL_Y, meas.CountValue[STALS_COLOR_RED-1] + 1);
	input_report_rel(vd6281_data->als_input_dev, REL_Z, meas.CountValue[STALS_COLOR_GREEN-1] + 1);
	input_report_rel(vd6281_data->als_input_dev, REL_RX, meas.CountValue[STALS_COLOR_BLUE-1] + 1);
	input_report_rel(vd6281_data->als_input_dev, REL_RY, meas.CountValue[STALS_COLOR_CLEAR-1] + 1);
//	input_report_abs(vd6281_data->als_input_dev, ABS_X, meas.time_us + 1);
//	input_report_abs(vd6281_data->als_input_dev, ABS_Y, meas.gain + 1);
	input_sync(vd6281_data->als_input_dev);
	memset(vd6281_data->flicker_data, 0, sizeof(vd6281_data->flicker_data));
	res = STALS_Start(vd6281_data->devices.hdl, STALS_MODE_ALS_SINGLE_SHOT, ALS_CHANNELS);
	res = STALS_Start(vd6281_data->devices.hdl, STALS_MODE_FLICKER, FLK_CHANNEL);

	reinit_completion(&vd6281_data->completion);
	start_flicker_timer();
	schedule_delayed_work(&vd6281_data->rear_als_work, msecs_to_jiffies(ALS_WORK_DELAY));
}

static int vd6281_write_reg(struct vd6281_device_data *device,
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
			msleep_interruptible(VD6281_I2C_RETRY_DELAY);
		if (err < 0)
			ALS_err("%s - i2c_transfer error = %d\n", __func__, err);
	} while ((err != num) && (++tries < VD6281_I2C_MAX_RETRIES));

	mutex_unlock(&device->suspendlock);

	if (err != num) {
		ALS_err("%s -write transfer error:%d\n", __func__, err);
		err = -EIO;
		device->i2c_err_cnt++;
		return err;
	}

	return 0;
}

static int vd6281_read_reg(struct vd6281_device_data *device,
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
			msleep_interruptible(VD6281_I2C_RETRY_DELAY);
		if (err < 0)
			ALS_err("%s - i2c_transfer error = %d\n", __func__, err);
	} while ((err != num) && (++tries < VD6281_I2C_MAX_RETRIES));

	mutex_unlock(&device->suspendlock);

	if (err != num) {
		ALS_err("%s -read transfer error:%d\n", __func__, err);
		err = -EIO;
		device->i2c_err_cnt++;
	} else
		err = 0;

	return err;
}

STALS_ErrCode_t STALS_WrByte(void *pClient, uint8_t index, uint8_t data)
{
	vd6281_write_reg(vd6281_data, index, data);

	return STALS_NO_ERROR;
}

STALS_ErrCode_t STALS_RdByte(void *pClient, uint8_t index, uint8_t *data)
{
 	uint8_t length = 1;
	 
	vd6281_read_reg(vd6281_data, index, data, length);

	return STALS_NO_ERROR;
}


STALS_ErrCode_t STALS_RdMultipleBytes(void *pClient, uint8_t index,
	uint8_t *data, int nb)
{
	STALS_ErrCode_t res;
	int i;

	for (i = 0; i < nb; i++) {
		res = vd6281_read_reg(vd6281_data, index + i, &data[i], 1);
		if (res)
			return res;
	}

	return STALS_NO_ERROR;
}

static inline int channelId_2_index(enum STALS_Channel_Id_t ChannelId)
{
	return POPCOUNT(ChannelId) == 1 ? __builtin_ctz(ChannelId) :
		STALS_ALS_MAX_CHANNELS;
}

static int is_cut_2_0(struct VD6281_device *dev)
{
	return (dev->device_id == VD6281_DEVICE) &&
		(dev->revision_id == VD6281_REVISION_CUT_2_0);
}

static int is_cut_2_1(struct VD6281_device *dev)
{
	return (dev->device_id == VD6281_DEVICE) &&
		(dev->revision_id == VD6281_REVISION_CUT_2_1);
}

struct VD6281_device *vd6281_get_device(void **pHandle)
{
#if 0
	uint32_t i;
//
//	for (i = 0; i < VD6281_CONFIG_DEVICES_MAX; i++) {
/		if (!vd6281_data->devices[i].st)
			break;
	}
	*pHandle = (void *)(uintptr_t)i;

	return i == VD6281_CONFIG_DEVICES_MAX ? NULL : &vd6281_data->devices[i];
#endif
	*pHandle = (void *)(uintptr_t) 0;

	return &vd6281_data->devices;
}

static void setup_device(struct VD6281_device *dev, void *pClient, void *hdl)
{
	memset(dev, 0, sizeof(*dev));
	dev->client = pClient;
	dev->hdl = hdl;
	dev->st = DEV_INIT;
	dev->is_otp_usage_enable = STALS_CONTROL_ENABLE;
	dev->is_output_dark_enable = STALS_CONTROL_DISABLE;
	dev->exposure = 80000;
}

static void select_device_wa(struct VD6281_device *dev)
{
	dev->wa_codex_460857 = 1;
	dev->wa_codex_462997 = 1;
	dev->wa_codex_461428 = 1;
	dev->wa_codex_464174 = is_cut_2_0(dev);
	dev->wa_codex_478654 = 1;
}

static STALS_ErrCode_t otp_read_bank(struct VD6281_device *dev, int bank,
	int bit_start, int bit_end, uint32_t *opt)
{
	int bit_nb = bit_end - bit_start + 1;
	uint32_t bit_mask = (1 << bit_nb) - 1;

	*opt = (dev->otp_bits[bank] >> bit_start) & bit_mask;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t otp_read(struct VD6281_device *dev, int bit_start,
	int bit_nb, uint32_t *otp, int bit_swap)
{
	STALS_ErrCode_t res;
	const int bit_end = bit_start + bit_nb - 1;
	uint32_t bank0 = 0;
	uint32_t bank1 = 0;
	uint32_t result;
	uint32_t result_swap;
	int bank1_shift = 0;
	int i;

#ifndef __KERNEL__
	assert(bit_nb <= 24);
	assert(bit_end < 120);
#endif

	if (bit_start < 64) {
		res = otp_read_bank(dev, 0, bit_start, MIN(63, bit_end),
			&bank0);
		if (res)
			return res;
		bank1_shift = MIN(63, bit_end) - bit_start + 1;
	}
	if (bit_end >= 64) {
		res = otp_read_bank(dev, 1, MAX(0, bit_start - 64),
			bit_end - 64, &bank1);
		if (res)
			return res;
	}

	result = bank0 | (bank1 << bank1_shift);
	if (bit_swap) {
		result_swap = 0;
		for (i = 0; i < bit_nb; i++) {
			if ((result >> i) & 1)
				result_swap |= 1 << (bit_nb - 1 - i);
		}
	}
	*otp = bit_swap ? result_swap : result;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t check_supported_device(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;

	res = STALS_RdByte(dev->client, VD6281_DEVICE_ID, &dev->device_id);
	if (res)
		return res;
	res = STALS_RdByte(dev->client, VD6281_REVISION_ID, &dev->revision_id);
	if (res)
		return res;

	if (is_cut_2_0(dev))
		return STALS_NO_ERROR;
	if (is_cut_2_1(dev))
		return STALS_NO_ERROR;

	return STALS_ERROR_INIT;
}

static STALS_ErrCode_t is_data_ready(struct VD6281_device *dev,
	uint8_t *is_data_ready)
{
	STALS_ErrCode_t res;
	uint8_t isr_ctrl_st;

	res = STALS_RdByte(dev->client, VD6281_IRQ_CTRL_ST, &isr_ctrl_st);
	*is_data_ready = !(isr_ctrl_st & 0x02);

	return res;
}

static STALS_ErrCode_t ac_mode_update(struct VD6281_device *dev, uint8_t mask,
	uint8_t data)
{
	STALS_ErrCode_t res;
	uint8_t ac_mode;

	res = STALS_RdByte(dev->client, VD6281_AC_MODE, &ac_mode);
	if (res)
		return res;
	ac_mode = (ac_mode & ~mask) | (data & mask);
	return STALS_WrByte(dev->client, VD6281_AC_MODE, ac_mode);
}

static STALS_ErrCode_t read_channel(struct VD6281_device *dev, int chan,
	uint32_t *measure)
{
	STALS_ErrCode_t res;
	uint8_t values[3];

	res = STALS_RdMultipleBytes(dev->client, VD6281_CHANNELx_MM(chan),
		values, 3);
	if (res)
		return res;

	*measure = (values[0] << 16) + (values[1] << 8) + values[2];

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t acknowledge_irq(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;

	res = STALS_WrByte(dev->client, VD6281_IRQ_CTRL_ST, 1);
	if (res)
		return res;
	res = STALS_WrByte(dev->client, VD6281_IRQ_CTRL_ST, 0);
	if (res)
		return res;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t dev_sw_reset(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;

	res = STALS_WrByte(dev->client, VD6281_GLOBAL_RESET, 1);
	if (res)
		return res;
	res = STALS_WrByte(dev->client, VD6281_GLOBAL_RESET, 0);
	if (res)
		return res;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t opt_reset(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;
	uint8_t status;
	int max_loop_nb = 100;

	res = STALS_WrByte(dev->client, VD6281_OTP_CTRL1, 0);
	if (res)
		return res;

	do {
		res = STALS_RdByte(dev->client, VD6281_OTP_STATUS, &status);
		if (res)
			return res;
	} while ((status & VD6281_OTP_DATA_READY) !=
		 VD6281_OTP_DATA_READY && --max_loop_nb);
	if (!max_loop_nb)
		return STALS_ERROR_TIME_OUT;

	return STALS_NO_ERROR;
}

static uint8_t byte_bit_reversal(uint8_t d)
{
	d = ((d & 0xaa) >> 1) | ((d & 0x55) << 1);
	d = ((d & 0xcc) >> 2) | ((d & 0x33) << 2);
	d = ((d & 0xf0) >> 4) | ((d & 0x0f) << 4);

	return d;
}

static STALS_ErrCode_t opt_read_init(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;
	uint8_t reg[8];
	int i;

	dev->otp_bits[0] = 0;

	res = STALS_RdMultipleBytes(dev->client, VD6281_OTP_BANK_0, reg, 8);
	if (res)
		return res;

	for (i = 0; i < 7; i++) {
		dev->otp_bits[0] |=
			(uint64_t) byte_bit_reversal(reg[i]) << (i * 8);
	}
	reg[7] = byte_bit_reversal(reg[7]) >> 4;
	dev->otp_bits[0] |=  (uint64_t) reg[7] << 56;

	res = STALS_RdMultipleBytes(dev->client, VD6281_OTP_BANK_1, reg, 8);
	if (res)
		return res;

	reg[0] = byte_bit_reversal(reg[0]);
	dev->otp_bits[0] |=  (uint64_t) (reg[0] & 0x0f) << 60;
	dev->otp_bits[1] = reg[0] >> 4;

	for (i = 1; i < 7; i++) {
		dev->otp_bits[1] |=
			(uint64_t) byte_bit_reversal(reg[i]) << (i * 8 - 4);
	}
	reg[7] = byte_bit_reversal(reg[7]) >> 4;
	dev->otp_bits[1] |=  (uint64_t) reg[7] << 52;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t otp_read_param_v1_v2(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;
	uint32_t otp_data;
	int i;

	res = otp_read(dev, 51, 9, &otp_data, 1);
	if (res)
		goto error;
	dev->otp.hf_trim = otp_data;
	res = otp_read(dev, 116, 4, &otp_data, 1);
	if (res)
		goto error;
	dev->otp.lf_trim = otp_data;
	res = otp_read(dev, 48, 3, &otp_data, 1);
	if (res)
		goto error;
	dev->otp.filter_config = otp_data;
	for (i = 0; i < STALS_ALS_MAX_CHANNELS; i++) {
		res = otp_read(dev, i * 8, 8, &otp_data, 1);
		if (res)
			goto error;
		/* 0 is not a valid value */
		dev->otp.gains[i] = otp_data ? otp_data : VD6281_DEFAULT_GAIN;
	}

	return STALS_NO_ERROR;

error:
	return res;
}

static STALS_ErrCode_t otp_read_param(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;
	uint32_t otp_data;
	int i;

	/* default values in case we don't recognize otp version */
	dev->otp.hf_trim = VD6281_DEFAULT_HF_TRIM;
	dev->otp.lf_trim = VD6281_DEFAULT_LF_TRIM;
	dev->otp.filter_config = VD6281_DEFAULT_FILTER_CONFIG;
	for (i = 0; i < STALS_ALS_MAX_CHANNELS; i++)
		dev->otp.gains[i] = VD6281_DEFAULT_GAIN;

	/* read otp version */
	res = otp_read(dev, 113, 3, &otp_data, 1);
	if (res)
		goto error;

	switch (otp_data) {
	case VD6281_OTP_V1:
	case VD6281_OTP_V2:
		res = otp_read_param_v1_v2(dev);
		break;
	default:
		/* default values will be applied */
		break;
	}

error:
	return res;
}

static STALS_ErrCode_t trim_oscillators(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;
	uint16_t hf_trim = dev->is_otp_usage_enable ? dev->otp.hf_trim :
		VD6281_DEFAULT_HF_TRIM;
	uint8_t lf_trim = dev->is_otp_usage_enable ? dev->otp.lf_trim :
		VD6281_DEFAULT_LF_TRIM;

	/* 10.24Mhz */
	res = STALS_WrByte(dev->client, VD6281_OSC10M, 1);
	if (res)
		return res;

	res = STALS_WrByte(dev->client, VD6281_OSC10M_TRIM_M,
		(hf_trim >> 8) & 0x01);
	if (res)
		return res;
	res = STALS_WrByte(dev->client, VD6281_OSC10M_TRIM_L,
		(hf_trim >> 0) & 0xff);
	if (res)
		return res;

	/* 48.7 Khz */
	res = STALS_WrByte(dev->client, VD6281_OSC50K_TRIM, lf_trim & 0x0f);
	if (res)
		return res;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t clamp_enable(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;

	res = STALS_WrByte(dev->client, VD6281_AC_CLAMP_EN, 1);
	if (res)
		return res;
	res = STALS_WrByte(dev->client, VD6281_DC_CLAMP_EN, 0x1f);
	if (res)
		return res;
	/* set count to be independent of pd nummbers */
	res = STALS_WrByte(dev->client, VD6281_SPARE_1, 0x3f);
	if (res)
		return res;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t dev_configuration(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;
	const uint8_t pds[] = {0x07, 0x07, 0x07, 0x1f, 0x0f, 0x1f};
	int c;

	res = opt_reset(dev);
	if (res)
		return res;

	res = opt_read_init(dev);
	if (res)
		return res;

	res = otp_read_param(dev);
	if (res)
		return res;

	res = trim_oscillators(dev);
	if (res)
		return res;

	res = clamp_enable(dev);
	if (res)
		return res;

	for (c = 0; c < STALS_ALS_MAX_CHANNELS; c++) {
		res = STALS_WrByte(dev->client, VD6281_SEL_PD_x(c), pds[c]);
		if (res)
			return res;
	}

	for (c = 0; c < STALS_ALS_MAX_CHANNELS; c++) {
		res = set_channel_gain(dev, c, 0x0100);   // default 25x
		if (res)
			return res;
	}

	res = set_pedestal_value(dev, 3);
	if (res)
		return res;

	return STALS_NO_ERROR;
}

void vd6281_put_device(struct VD6281_device *dev)
{
	dev_sw_reset(dev);
	dev->st = DEV_FREE;
}

static struct VD6281_device *get_active_device(void *pHandle)
{
	uint32_t handle = (uint32_t)(uintptr_t) pHandle;

	if (handle >= VD6281_CONFIG_DEVICES_MAX)
		return NULL;
	if (vd6281_data->devices.st == DEV_FREE)
		return NULL;

	return &vd6281_data->devices;
}

static STALS_ErrCode_t get_channel_gain(struct VD6281_device *dev, int c,
	uint16_t *pAppliedGain)
{
	*pAppliedGain = dev->gains[c];

	return STALS_NO_ERROR;
}

STALS_ErrCode_t set_channel_gain(struct VD6281_device *dev, int c,
	uint16_t Gain)
{
	STALS_ErrCode_t res;
	uint8_t vref;

	for (vref = ARRAY_SIZE(GainRange) - 1; vref > 0; vref--) {
		if (Gain < GainRangeThreshold[vref - 1])
			break;
	}

	res = STALS_WrByte(dev->client, VD6281_CHANNEL_VREF(c), vref + 1);
	if (res)
		return res;
	dev->gains[c] = GainRange[vref];

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t get_exposure(struct VD6281_device *dev,
	uint32_t *pAppliedExpoTimeUs)
{
	*pAppliedExpoTimeUs = dev->exposure;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t set_exposure(struct VD6281_device *dev,
	uint32_t ExpoTimeInUs, int is_cache_updated)
{
	const uint32_t step_size_us = 1600;
	const uint32_t rounding = step_size_us / 2;
	uint32_t exposure;
	uint64_t exposure_acc;
	STALS_ErrCode_t res;

	/* avoid integer overflow using intermediate 64 bits arithmetics */
	exposure_acc = ExpoTimeInUs + (uint64_t) rounding;
	exposure = div64_u64(MIN(exposure_acc, 0xffffffffULL), step_size_us);
	exposure = MAX(exposure, 1);
	exposure = MIN(exposure, 0x3ff);
	res = STALS_WrByte(dev->client, VD6281_EXPOSURE_L, exposure & 0xff);
	if (res)
		return res;
	res = STALS_WrByte(dev->client, VD6281_EXPOSURE_M,
		(exposure >> 8) & 0x3);
	if (res)
		return res;

	if (is_cache_updated)
		dev->exposure = exposure * step_size_us;

	return STALS_NO_ERROR;
}

static uint8_t dev_enable_dc_chan_en_for_mode(struct VD6281_device *dev,
	enum STALS_Mode_t mode, uint8_t channels)
{
	int is_flk = mode == STALS_MODE_FLICKER;
	uint8_t res;

	res = (channels & VD6281_DC_CHANNELS_MASK) | dev->dc_chan_en;

	if (dev->wa_codex_460857) {
		/* wa is easy. We force channel 4 activation when we are in
		 * condition of the codex => als mode and als only use channel 6
		 * (note this is not an optimal way to fix the problem)
		 */
		if (!is_flk && channels == STALS_CHANNEL_6)
			res |= STALS_CHANNEL_4;
	}

	return res;
}

static STALS_ErrCode_t dev_enable_channels_for_mode(struct VD6281_device *dev,
	enum STALS_Mode_t mode, uint8_t channels)
{
	uint8_t active_chan = dev->als.chan | dev->flk.chan;
	int is_flk = mode == STALS_MODE_FLICKER;
	uint8_t dc_chan_en;
	STALS_ErrCode_t res;

	dc_chan_en = dev_enable_dc_chan_en_for_mode(dev, mode, channels);
	if (channels & active_chan)
		return STALS_ERROR_INVALID_PARAMS;

	if (dc_chan_en ^ dev->dc_chan_en) {
		res = STALS_WrByte(dev->client, VD6281_DC_EN, dc_chan_en);
		if (res)
			return res;
	}

	/* if wa_codex_464174 active then be sure to always turn on ac channel
	 */
	if (((channels & VD6281_AC_CHANNELS_MASK) || dev->wa_codex_464174) &&
	    !dev->ac_chan_en) {
		res = STALS_WrByte(dev->client, VD6281_AC_EN, 1);
		if (res) {
			STALS_WrByte(dev->client, VD6281_DC_EN,
				dev->dc_chan_en);
			return res;
		}
		dev->ac_chan_en = 1;
	}
	dev->dc_chan_en = dc_chan_en;

	if (is_flk)
		dev->flk.chan = channels;
	else
		dev->als.chan = channels;

	return STALS_NO_ERROR;
}

static uint8_t dev_disable_dc_chan_en_for_mode(struct VD6281_device *dev,
	enum STALS_Mode_t mode)
{
	int is_flk = mode == STALS_MODE_FLICKER;
	uint8_t channels = is_flk ? dev->flk.chan : dev->als.chan;
	uint8_t res;

	res = dev->dc_chan_en & ~(channels & VD6281_DC_CHANNELS_MASK);

	if (dev->wa_codex_460857) {
		/* We need to be sure to keep channel 4 if still need but we
		 * also to be sure is't turn off when no more need
		 */
		if (is_flk) {
			/* avoid to turn off channel 4 in case we need it to wa
			 * als complete codex
			 */
			if (dev->als.chan == STALS_CHANNEL_6
			    && dev->flk.chan == STALS_CHANNEL_4)
				res = res | STALS_CHANNEL_4;
		} else {
			/* be sure to turn off channel 4 in case we turn it on
			 * to wa als complete codex
			 */
			if (dev->als.chan == STALS_CHANNEL_6 &&
			    dev->flk.chan != STALS_CHANNEL_4)
				res = res & ~STALS_CHANNEL_4;
		}
	}

	return res;
}

static STALS_ErrCode_t dev_disable_channels_for_mode(struct VD6281_device *dev,
	enum STALS_Mode_t mode)
{
	int is_flk = mode == STALS_MODE_FLICKER;
	uint8_t channels = is_flk ? dev->flk.chan : dev->als.chan;
	uint8_t dc_chan_en = dev_disable_dc_chan_en_for_mode(dev, mode);
	STALS_ErrCode_t res;

	if (dc_chan_en ^ dev->dc_chan_en) {
		res = STALS_WrByte(dev->client, VD6281_DC_EN, dc_chan_en);
		if (res)
			return res;
	}

	/* if wa_codex_464174 active then only disable ac channel when no
	 * channels are active
	 */
	if (dev->wa_codex_464174) {
		uint8_t next_flk_chan = is_flk ? 0 : dev->flk.chan;
		uint8_t next_als_chan = is_flk ? dev->als.chan : 0;

		if (next_flk_chan == 0 && next_als_chan == 0) {
			res = STALS_WrByte(dev->client, VD6281_AC_EN, 0);
			if (res) {
				STALS_WrByte(dev->client, VD6281_DC_EN,
					dev->dc_chan_en);
				return res;
			}
			dev->ac_chan_en = 0;
		}
	} else if (channels & VD6281_AC_CHANNELS_MASK) {
		res = STALS_WrByte(dev->client, VD6281_AC_EN, 0);
		if (res) {
			STALS_WrByte(dev->client, VD6281_DC_EN,
				dev->dc_chan_en);
			return res;
		}
		dev->ac_chan_en = 0;
	}
	dev->dc_chan_en = dc_chan_en;

	if (is_flk)
		dev->flk.chan = 0;
	else
		dev->als.chan = 0;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t enable_flicker_output_mode(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;
	uint8_t pdm_select_output = PDM_SELECT_GPIO;
	uint8_t pdm_select_clk = PDM_SELECT_INTERNAL_CLK;
	uint8_t intr_config = 0x00;
	uint8_t sel_dig = 0x00;
	enum STALS_FlickerOutputType_t FlickerOutputType =
		dev->flicker_output_type;

	/* STALS_FLICKER_OUTPUT_ANALOG_CFG_1 : pdm data on GPIO pin through
	 *internal resistance and use internal clock for sigma-delta 1 bit dac.
	 * STALS_FLICKER_OUTPUT_DIGITAL_PDM : pdm data on GPIO pin and use
	 * external clock feed from CLKIN pin to drive sigma-delta 1 bit dac.
	 * STALS_FLICKER_OUTPUT_ANALOG_CFG_2 : pdm data on CLKIN pin and use
	 * internal clock for sigma-delta 1 bit dac.
	 */
	switch (FlickerOutputType) {
	case STALS_FLICKER_OUTPUT_ANALOG_CFG_1:
		intr_config = 0x02;
		sel_dig = 0x0f;
		break;
	case STALS_FLICKER_OUTPUT_DIGITAL_PDM:
		pdm_select_clk = PDM_SELECT_EXTERNAL_CLK;
		intr_config = 0x01;
		sel_dig = 0x0f;
		break;
	case STALS_FLICKER_OUTPUT_ANALOG_CFG_2:
		pdm_select_output = PDM_SELECT_CLKIN;
		break;
	default:
		assert(1);
	}
	res = ac_mode_update(dev, PDM_SELECT_OUTPUT | PDM_SELECT_CLK,
		pdm_select_output | pdm_select_clk);
	if (res)
		return res;

	/* interrupt pin output stage configuration */
	res = STALS_WrByte(dev->client, VD6281_INTR_CFG, intr_config);
	if (res)
		return res;

	/* interrupt output selection */
	res = STALS_WrByte(dev->client, VD6281_DTEST_SELECTION, sel_dig);
	if (res)
		return res;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t disable_flicker_output_mode(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;

	/* restore ac_mode bit 4 and bit 5 */
	res = ac_mode_update(dev, PDM_SELECT_OUTPUT | PDM_SELECT_CLK,
		PDM_SELECT_GPIO | PDM_SELECT_INTERNAL_CLK);
	if (res)
		return res;

	/* restore OD output stage configuration */
	res = STALS_WrByte(dev->client, VD6281_INTR_CFG, 0x00);
	if (res)
		return res;

	/* and output als_complete signal */
	res = STALS_WrByte(dev->client, VD6281_DTEST_SELECTION, 0x00);
	if (res)
		return res;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t stop_als(struct VD6281_device *dev,
	enum STALS_Mode_t mode)
{
	STALS_ErrCode_t res;

	if (dev->st != DEV_ALS_RUN && dev->st != DEV_BOTH_RUN)
		return STALS_ERROR_NOT_STARTED;

	if (dev->als_started_mode != mode)
		return STALS_ERROR_NOT_STARTED;

	res = STALS_WrByte(dev->client, VD6281_ALS_CTRL, 0);
	if (res)
		return res;

	res = dev_disable_channels_for_mode(dev, mode);
	if (res)
		return res;

	dev->st = dev->st == DEV_ALS_RUN ? DEV_INIT : DEV_FLICKER_RUN;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t start_als(struct VD6281_device *dev, uint8_t channels,
	enum STALS_Mode_t mode)
{
	STALS_ErrCode_t res;
	uint32_t cmd = (mode == STALS_MODE_ALS_SYNCHRONOUS) ? VD6281_ALS_START |
		VD6281_ALS_CONTINUOUS | VD6281_ALS_CONTINUOUS_SLAVED :
		VD6281_ALS_START;

	if (dev->st == DEV_ALS_RUN || dev->st == DEV_BOTH_RUN)
		return STALS_ERROR_ALREADY_STARTED;

	if (dev->wa_codex_461428 && mode == STALS_MODE_ALS_SYNCHRONOUS &&
	    (channels & STALS_CHANNEL_6))
		return STALS_ERROR_INVALID_PARAMS;

	if (dev->wa_codex_462997 && dev->st == DEV_FLICKER_RUN) {
		/* can't do als synchronous and flicker at the same time */
		if (mode == STALS_MODE_ALS_SYNCHRONOUS)
			return STALS_ERROR_INVALID_PARAMS;
	}

	res = dev_enable_channels_for_mode(dev, mode, channels);
	if (res)
		return res;

	if (dev->wa_codex_478654 && dev->st == DEV_FLICKER_RUN &&
	    (dev->als.chan & STALS_CHANNEL_6)) {
		/* stop flicker */
		res = ac_mode_update(dev , AC_EXTRACTOR, AC_EXTRACTOR_DISABLE);
		if (res)
			goto start_als_error;
	}

	if (dev->wa_codex_462997 && dev->st == DEV_FLICKER_RUN) {
		/* need this dummy write so als single is working well with
		 * flicker
		 */
		res = STALS_WrByte(dev->client,
			VD6281_ALS_CTRL, VD6281_ALS_START |
			VD6281_ALS_CONTINUOUS);
		if (res)
			goto start_als_error;
	}
	res = STALS_WrByte(dev->client, VD6281_ALS_CTRL, cmd);
	if (res)
		goto start_als_error;

	if (dev->wa_codex_478654 && dev->st == DEV_FLICKER_RUN &&
	    (dev->als.chan & STALS_CHANNEL_6)) {
		/* restart flicker */
		res = ac_mode_update(dev, AC_EXTRACTOR, AC_EXTRACTOR_ENABLE);
	}

	dev->st = dev->st == DEV_INIT ? DEV_ALS_RUN : DEV_BOTH_RUN;
	dev->als_started_mode = mode;

	return STALS_NO_ERROR;

start_als_error:
	dev_disable_channels_for_mode(dev, mode);

	return res;
}

static STALS_ErrCode_t start_single_shot(struct VD6281_device *dev,
	uint8_t channels)
{
	return start_als(dev, channels, STALS_MODE_ALS_SINGLE_SHOT);
}

static STALS_ErrCode_t start_synchronous(struct VD6281_device *dev,
	uint8_t channels)
{
	return start_als(dev, channels, STALS_MODE_ALS_SYNCHRONOUS);
}

static STALS_ErrCode_t start_flicker(struct VD6281_device *dev,
	uint8_t channels)
{
	STALS_ErrCode_t res;
	uint8_t ac_channel_select;

	/* This check only one channel is selected */
	CHECK_CHANNEL_VALID(channelId_2_index(channels));

	if (dev->st == DEV_FLICKER_RUN || dev->st == DEV_BOTH_RUN)
		return STALS_ERROR_ALREADY_STARTED;

	if (dev->wa_codex_462997 && dev->st == DEV_ALS_RUN) {
		uint8_t value;

		/* can't do als synchronous and flicker at the same time */
		if (dev->als_started_mode == STALS_MODE_ALS_SYNCHRONOUS)
			return STALS_ERROR_INVALID_PARAMS;
		/* we can do als single and flicker at the same time but only
		 *if inter-measurement is zero
		 */
		res = STALS_RdByte(dev->client, VD6281_CONTINUOUS_PERIOD,
			&value);
		if (res)
			return res;
		if (value)
			return STALS_ERROR_INVALID_PARAMS;
	}

	res = dev_enable_channels_for_mode(dev, STALS_MODE_FLICKER, channels);
	if (res)
		return res;

	res = enable_flicker_output_mode(dev);
	if (res) {
		dev_disable_channels_for_mode(dev, STALS_MODE_FLICKER);
		return res;
	}

	/* now enable ac and select channel ac channel */
	ac_channel_select = channels == STALS_CHANNEL_6 ? 1 :
		channelId_2_index(channels) + 2;
	res = ac_mode_update(dev, AC_CHANNEL_SELECT | AC_EXTRACTOR,
		(ac_channel_select << 1) | AC_EXTRACTOR_ENABLE);
	if (res) {
		disable_flicker_output_mode(dev);
		dev_disable_channels_for_mode(dev, STALS_MODE_FLICKER);
		return res;
	}

	dev->st = dev->st == DEV_INIT ? DEV_FLICKER_RUN : DEV_BOTH_RUN;

	return res;
}

static STALS_ErrCode_t stop_single_shot(struct VD6281_device *dev)
{
	return stop_als(dev, STALS_MODE_ALS_SINGLE_SHOT);
}

static STALS_ErrCode_t stop_synchronous(struct VD6281_device *dev)
{
	return stop_als(dev, STALS_MODE_ALS_SYNCHRONOUS);
}

static STALS_ErrCode_t stop_flicker(struct VD6281_device *dev)
{
	STALS_ErrCode_t res;

	if (dev->st != DEV_FLICKER_RUN && dev->st != DEV_BOTH_RUN)
		return STALS_ERROR_NOT_STARTED;

	res = ac_mode_update(dev, AC_EXTRACTOR, AC_EXTRACTOR_DISABLE);
	if (res)
		return res;
	disable_flicker_output_mode(dev);
	dev_disable_channels_for_mode(dev, STALS_MODE_FLICKER);
	dev->st = dev->st == DEV_FLICKER_RUN ? DEV_INIT : DEV_ALS_RUN;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t get_pedestal_enable(struct VD6281_device *dev,
	uint32_t *value)
{
	STALS_ErrCode_t res;
	uint8_t ac_mode;

	res = STALS_RdByte(dev->client, VD6281_AC_MODE, &ac_mode);
	*value = (ac_mode & AC_PEDESTAL) ? STALS_CONTROL_DISABLE :
		STALS_CONTROL_ENABLE;

	return res;
}

static STALS_ErrCode_t get_pedestal_value(struct VD6281_device *dev,
	uint32_t *value)
{
	STALS_ErrCode_t res;
	uint8_t data;

	res = STALS_RdByte(dev->client, VD6281_AC_PEDESTAL, &data);
	*value = data & VD6281_PEDESTAL_VALUE_MASK;

	return res;
}

static STALS_ErrCode_t get_otp_usage_enable(struct VD6281_device *dev,
	uint32_t *value)
{
	*value = dev->is_otp_usage_enable;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t get_output_dark_enable(struct VD6281_device *dev,
	uint32_t *value)
{
	*value = dev->is_output_dark_enable;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t get_sda_drive_value(struct VD6281_device *dev,
	uint32_t *value)
{
	STALS_ErrCode_t res;
	uint8_t data;

	res = STALS_RdByte(dev->client, VD6281_SDA_DRIVE, &data);
	data &= VD6281_SDA_DRIVE_MASK;
	*value = (data + 1) * 4;

	return res;
}

static STALS_ErrCode_t get_wa_status(struct VD6281_device *dev, uint32_t *value)
{
	STALS_ErrCode_t res = STALS_NO_ERROR;
	uint32_t value_wa = (*value) & 0x7fffffff;
	uint32_t state;

	switch (value_wa) {
	case WA_460857:
		state = dev->wa_codex_460857;
		break;
	case WA_462997:
		state = dev->wa_codex_462997;
		break;
	case WA_461428:
		state = dev->wa_codex_461428;
		break;
	case WA_464174:
		state = dev->wa_codex_464174;
		break;
	case WA_478654:
		state = dev->wa_codex_478654;
		break;
	default:
		res = STALS_ERROR_INVALID_PARAMS;
	}
	if (res == STALS_NO_ERROR)
		*value |= state << 31;

	return res;
}

static STALS_ErrCode_t set_pedestal_enable(struct VD6281_device *dev,
	uint32_t value)
{
	return ac_mode_update(dev, AC_PEDESTAL,
		value ? AC_PEDESTAL_ENABLE : AC_PEDESTAL_DISABLE);
}

STALS_ErrCode_t set_pedestal_value(struct VD6281_device *dev,
	uint32_t value)
{
	return STALS_WrByte(dev->client, VD6281_AC_PEDESTAL,
		value & VD6281_PEDESTAL_VALUE_MASK);
}

static STALS_ErrCode_t set_otp_usage_enable(struct VD6281_device *dev,
	uint32_t value)
{
	STALS_ErrCode_t res;

	dev->is_otp_usage_enable = value;
	/* oscillator trimming configuration need to be done again */
	res = trim_oscillators(dev);
	if (res)
		return res;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t set_output_dark_enable(struct VD6281_device *dev,
	uint32_t value)
{
	STALS_ErrCode_t res;
	uint8_t pd_value = value ? 0x18 : 0x07;

	res = STALS_WrByte(dev->client, VD6281_SEL_PD_x(1), pd_value);
	if (res)
		return res;

	dev->is_output_dark_enable = value;

	return STALS_NO_ERROR;
}

static STALS_ErrCode_t set_sda_drive_value(struct VD6281_device *dev,
	uint32_t value)
{
	STALS_ErrCode_t res;
	uint8_t data;
	uint8_t sda_drive_reg_value;

	/* valid values are 4, 8, 12, 16, 20 */
	if (value > 20 || value == 0 || value % 4)
		return STALS_ERROR_INVALID_PARAMS;

	sda_drive_reg_value = value / 4 - 1;
	res = STALS_RdByte(dev->client, VD6281_SDA_DRIVE, &data);
	if (res)
		return res;

	data = (data & ~VD6281_SDA_DRIVE_MASK) | sda_drive_reg_value;
	return STALS_WrByte(dev->client, VD6281_SDA_DRIVE, data);
}

static STALS_ErrCode_t set_wa_status(struct VD6281_device *dev, uint32_t value)
{
	STALS_ErrCode_t res = STALS_NO_ERROR;
	int next_state = (value >> 31);

	value &= 0x7fffffff;
	switch (value) {
	case WA_460857:
		dev->wa_codex_460857 = next_state;
		break;
	case WA_462997:
		dev->wa_codex_462997 = next_state;
		break;
	case WA_461428:
		dev->wa_codex_461428 = next_state;
		break;
	case WA_464174:
		dev->wa_codex_464174 = next_state;
		break;
	case WA_478654:
		dev->wa_codex_478654 = next_state;
		break;
	default:
		res = STALS_ERROR_INVALID_PARAMS;
	}

	return res;
}

static uint32_t cook_values_slope_channel(uint8_t gain, uint32_t raw)
{
	uint32_t cooked;

	/* note that no overflow can occur */
	cooked = (raw * gain) >> 7;

	/* be sure to not generate values greater that saturation level ... */
	cooked = MIN(cooked, 0x00ffffff);
	/* ... but also avoid value below 0x100 except for zero */
	cooked = cooked < 0x100 ? 0 : cooked;

	return cooked;
}

static void cook_values_slope(struct VD6281_device *dev,
	struct STALS_Als_t *pAlsValue)
{
	uint8_t otp_gains[STALS_ALS_MAX_CHANNELS];
	int c;

	for (c = 0; c < VD6281_CHANNEL_NB; c++)
		otp_gains[c] = dev->is_otp_usage_enable ? dev->otp.gains[c] :
				VD6281_DEFAULT_GAIN;

	for (c = 0; c < VD6281_CHANNEL_NB; c++) {
		if (!(dev->als.chan & (1 << c)))
			continue;
		pAlsValue->CountValue[c] = cook_values_slope_channel(
			otp_gains[c], pAlsValue->CountValueRaw[c]);
	}
}

static void cook_values(struct VD6281_device *dev,
	struct STALS_Als_t *pAlsValue)
{
	cook_values_slope(dev, pAlsValue);
}

/* public API */
/* STALS_ERROR_INVALID_PARAMS */
STALS_ErrCode_t STALS_Init(char *UNUSED_P(pDeviceName), void *pClient,
	void **pHandle)
{
	STALS_ErrCode_t res;
	struct VD6281_device *dev;

	CHECK_NULL_PTR(pHandle);
	dev = vd6281_get_device(pHandle);
	if (!dev) {
		ALS_err("%s - vd6281_get_device failed.\n", __func__);
		goto get_device_error;
	}
	setup_device(dev, pClient, *pHandle);
	res = check_supported_device(dev);
	if (res) {
		ALS_err("%s - check_supported_device failed.\n", __func__);
		goto check_supported_device_error;
	}
	select_device_wa(dev);
	res = dev_sw_reset(dev);
	if (res) {
		ALS_err("%s - select_device_wa failed.\n", __func__);
		goto dev_sw_reset_error;
	}
	res = dev_configuration(dev);
	if (res) {
		ALS_err("%s - dev_configuration failed.\n", __func__);
		goto dev_configuration_error;
	}

	return STALS_NO_ERROR;

dev_configuration_error:
dev_sw_reset_error:
check_supported_device_error:
	vd6281_put_device(dev);
get_device_error:
	return STALS_ERROR_INIT;
}

STALS_ErrCode_t STALS_Term(void *pHandle)
{
	struct VD6281_device *dev = get_active_device(pHandle);

	CHECK_DEVICE_VALID(dev);

	vd6281_put_device(dev);

	return STALS_NO_ERROR;
}

STALS_ErrCode_t STALS_GetVersion(uint32_t *pVersion, uint32_t *pRevision)
{
	CHECK_NULL_PTR(pVersion);
	CHECK_NULL_PTR(pRevision);

	*pVersion = (VERSION_MAJOR << 16) + VERSION_MINOR;
	*pRevision = VERSION_REVISION;

	return STALS_NO_ERROR;
}

STALS_ErrCode_t STALS_GetChannelColor(void *pHandle,
	enum STALS_Channel_Id_t ChannelId, enum STALS_Color_Id_t *pColor)
{
	struct VD6281_device *dev = get_active_device(pHandle);
	uint8_t filter_config;
	const enum STALS_Color_Id_t *mapping_array;

	CHECK_DEVICE_VALID(dev);
	CHECK_NULL_PTR(pColor);
	CHECK_CHANNEL_VALID(channelId_2_index(ChannelId));

	filter_config = dev->is_otp_usage_enable ? dev->otp.filter_config :
			VD6281_DEFAULT_FILTER_CONFIG;
	mapping_array = filter_mapping_array[filter_config];

	*pColor = mapping_array[channelId_2_index(ChannelId)];

	return STALS_NO_ERROR;
}

STALS_ErrCode_t STALS_SetExposureTime(void *pHandle, uint32_t ExpoTimeInUs,
	uint32_t *pAppliedExpoTimeUs)
{
	struct VD6281_device *dev = get_active_device(pHandle);
	STALS_ErrCode_t res;

	CHECK_DEVICE_VALID(dev);
	CHECK_NULL_PTR(pAppliedExpoTimeUs);
	CHECH_DEV_ST_ALS_NOT_STARTED(dev);

	res = set_exposure(dev, ExpoTimeInUs, 1);
	if (res)
		return res;

	res = get_exposure(dev, pAppliedExpoTimeUs);

	return res;
}

STALS_ErrCode_t STALS_GetExposureTime(void *pHandle,
	uint32_t *pAppliedExpoTimeUs)
{
	struct VD6281_device *dev = get_active_device(pHandle);

	CHECK_DEVICE_VALID(dev);
	CHECK_NULL_PTR(pAppliedExpoTimeUs);

	return get_exposure(dev, pAppliedExpoTimeUs);
}

STALS_ErrCode_t STALS_SetInterMeasurementTime(void *pHandle,
	uint32_t InterMeasurmentInUs, uint32_t *pAppliedInterMeasurmentInUs)
{
	const uint32_t step_size_us = 20500;
	const uint32_t rounding = step_size_us / 2;
	struct VD6281_device *dev = get_active_device(pHandle);
	STALS_ErrCode_t res;
	uint64_t value_acc;
	uint8_t value;

	CHECK_DEVICE_VALID(dev);
	CHECK_NULL_PTR(pAppliedInterMeasurmentInUs);
	/* in that case also forbid change when flicker is running */
	if (dev->wa_codex_462997)
		CHECH_DEV_ST_INIT(dev);
	else
		CHECH_DEV_ST_ALS_NOT_STARTED(dev);

	/* avoid integer overflow using intermediate 64 bits arithmetics */
	value_acc = InterMeasurmentInUs + (uint64_t) rounding;
	value_acc = div64_u64(MIN(value_acc, 0xffffffffULL), step_size_us);
	value = MIN(value_acc, 0xff);
	res = STALS_WrByte(dev->client, VD6281_CONTINUOUS_PERIOD, value);
	if (res)
		return res;

	return STALS_GetInterMeasurementTime(pHandle,
		pAppliedInterMeasurmentInUs);
}

STALS_ErrCode_t STALS_GetInterMeasurementTime(void *pHandle,
	uint32_t *pAppliedInterMeasurmentInUs)
{
	const uint32_t step_size_us = 20500;
	struct VD6281_device *dev = get_active_device(pHandle);
	STALS_ErrCode_t res;
	uint8_t value;

	CHECK_DEVICE_VALID(dev);
	CHECK_NULL_PTR(pAppliedInterMeasurmentInUs);

	res = STALS_RdByte(dev->client, VD6281_CONTINUOUS_PERIOD, &value);
	if (res)
		return res;

	*pAppliedInterMeasurmentInUs = step_size_us * value;

	return STALS_NO_ERROR;
}

STALS_ErrCode_t STALS_GetProductVersion(void *pHandle, uint8_t *pDeviceID,
	uint8_t *pRevisionID)
{
	struct VD6281_device *dev = get_active_device(pHandle);

	CHECK_DEVICE_VALID(dev);
	CHECK_NULL_PTR(pDeviceID);
	CHECK_NULL_PTR(pRevisionID);

	*pDeviceID = dev->device_id;
	*pRevisionID = dev->revision_id;

	return STALS_NO_ERROR;
}

STALS_ErrCode_t STALS_SetGain(void *pHandle, enum STALS_Channel_Id_t ChannelId,
	uint16_t Gain, uint16_t *pAppliedGain)
{
	struct VD6281_device *dev = get_active_device(pHandle);
	int chan = channelId_2_index(ChannelId);
	STALS_ErrCode_t res;

	CHECK_DEVICE_VALID(dev);
	CHECK_NULL_PTR(pAppliedGain);
	CHECK_CHANNEL_VALID(chan);
	CHECK_CHANNEL_NOT_IN_USE(dev, ChannelId);

	res = set_channel_gain(dev, chan, Gain);
	if (res)
		return res;

	res = get_channel_gain(dev, chan, pAppliedGain);

	return res;
}
STALS_ErrCode_t STALS_GetGain(void *pHandle, enum STALS_Channel_Id_t ChannelId,
	uint16_t *pAppliedGain)
{
	struct VD6281_device *dev = get_active_device(pHandle);
	int chan = channelId_2_index(ChannelId);
	STALS_ErrCode_t res;

	CHECK_DEVICE_VALID(dev);
	CHECK_NULL_PTR(pAppliedGain);
	CHECK_CHANNEL_VALID(chan);

	res = get_channel_gain(dev, chan, pAppliedGain);
	if (res)
		return res;

	return STALS_NO_ERROR;
}

STALS_ErrCode_t STALS_SetFlickerOutputType(void *pHandle,
	enum STALS_FlickerOutputType_t FlickerOutputType)
{
	struct VD6281_device *dev = get_active_device(pHandle);
	STALS_ErrCode_t res;

	CHECK_DEVICE_VALID(dev);
	CHECH_DEV_ST_FLICKER_NOT_STARTED(dev);

	switch (FlickerOutputType) {
	case STALS_FLICKER_OUTPUT_ANALOG_CFG_1:
	case STALS_FLICKER_OUTPUT_DIGITAL_PDM:
		dev->flicker_output_type = FlickerOutputType;
		res = STALS_NO_ERROR;
		break;
	case STALS_FLICKER_OUTPUT_ANALOG_CFG_2:
		res = is_cut_2_0(dev) ? STALS_ERROR_INVALID_PARAMS :
			STALS_NO_ERROR;
		if (res == STALS_NO_ERROR)
			dev->flicker_output_type = FlickerOutputType;
		break;
	default:
		res = STALS_ERROR_INVALID_PARAMS;
	}

	return res;
}

STALS_ErrCode_t STALS_Start(void *pHandle, enum STALS_Mode_t Mode,
	uint8_t Channels)
{
	struct VD6281_device *dev = get_active_device(pHandle);
	STALS_ErrCode_t res;

	CHECK_DEVICE_VALID(dev);
	CHECK_CHANNEL_MASK_VALID(Channels);

	switch (Mode) {
	case STALS_MODE_ALS_SINGLE_SHOT:
		res = start_single_shot(dev, Channels);
		break;
	case STALS_MODE_ALS_SYNCHRONOUS:
		res = start_synchronous(dev, Channels);
		break;
	case STALS_MODE_FLICKER:
		res = start_flicker(dev, Channels);
		break;
	default:
		res = STALS_ERROR_INVALID_PARAMS;
	}

	return res;
}

STALS_ErrCode_t STALS_Stop(void *pHandle, enum STALS_Mode_t Mode)
{
	struct VD6281_device *dev = get_active_device(pHandle);
	STALS_ErrCode_t res;

	CHECK_DEVICE_VALID(dev);

	switch (Mode) {
	case STALS_MODE_ALS_SINGLE_SHOT:
		res = stop_single_shot(dev);
		break;
	case STALS_MODE_ALS_SYNCHRONOUS:
		res = stop_synchronous(dev);
		break;
	case STALS_MODE_FLICKER:
		res = stop_flicker(dev);
		break;
	default:
		res = STALS_ERROR_INVALID_PARAMS;
	}

	return res;
}

STALS_ErrCode_t STALS_GetAlsValues(void *pHandle, uint8_t Channels,
	struct STALS_Als_t *pAlsValue, uint8_t *pMeasureValid)
{
	struct VD6281_device *dev = get_active_device(pHandle);
	STALS_ErrCode_t res;
	uint8_t c;

	CHECK_DEVICE_VALID(dev);
	CHECK_NULL_PTR(pAlsValue);
	CHECK_NULL_PTR(pMeasureValid);
	CHECK_CHANNEL_MASK_VALID_FOR_ALS(Channels);
	CHECK_DEV_ST_ALS_STARTED(dev);

	if (dev->st != DEV_ALS_RUN && dev->st != DEV_BOTH_RUN)
		return STALS_ERROR_NOT_STARTED;

	/* do we have ready datas */
	res = is_data_ready(dev, pMeasureValid);
	if (res)
		return res;
	if (!*pMeasureValid)
		return STALS_NO_ERROR;

	/* yes so read them */
	Channels &= dev->als.chan;
	for (c = 0; c < VD6281_CHANNEL_NB; c++) {
		if (!(Channels & (1 << c)))
			continue;
		res = read_channel(dev, c, &pAlsValue->CountValueRaw[c]);
		if (res)
			return res;
	}
	pAlsValue->Channels = Channels;
	cook_values(dev, pAlsValue);

	/* acknowledge irq */
	return acknowledge_irq(dev);
}

STALS_ErrCode_t STALS_GetFlickerFrequency(void *pHandle,
	struct STALS_FlickerInfo_t *pFlickerInfo)
{
	const uint32_t osc_freq = 10240000;
	const uint32_t cnt_divisor = 640;
	const uint32_t coeff = osc_freq / cnt_divisor;
	struct VD6281_device *dev = get_active_device(pHandle);
	STALS_ErrCode_t res;
	uint8_t ac_sat_metric_m, ac_sat_metric_l, ac_acc_periods_m,
		ac_acc_periods_l, ac_nb_of_periods, ac_amplitude_m,
		ac_amplitude_l;
	struct {
		uint8_t addr;
		uint8_t *value;
	} toread[] = {{VD6281_AC_SAT_METRIC_M, &ac_sat_metric_m},
			{VD6281_AC_SAT_METRIC_L, &ac_sat_metric_l},
			{VD6281_AC_ACC_PERIODS_M, &ac_acc_periods_m},
			{VD6281_AC_ACC_PERIODS_L, &ac_acc_periods_l},
			{VD6281_AC_NB_PERIODS, &ac_nb_of_periods},
			{VD6281_AC_AMPLITUDE_M, &ac_amplitude_m},
			{VD6281_AC_AMPLITUDE_L, &ac_amplitude_l} };
	unsigned int i;
	uint16_t ac_sat_metric, ac_acc_periods, ac_amplitude;

	CHECK_DEVICE_VALID(dev);
	CHECK_NULL_PTR(pFlickerInfo);
	CHECK_DEV_ST_FLICKER_STARTED(dev);

	for (i = 0; i < ARRAY_SIZE(toread); i++) {
		res = STALS_RdByte(dev->client, toread[i].addr,
			toread[i].value);
		if (res)
			return res;
	}

	ac_sat_metric = (ac_sat_metric_m << 8) + ac_sat_metric_l;
	ac_acc_periods = (ac_acc_periods_m << 8) + ac_acc_periods_l;
	ac_amplitude = (ac_amplitude_m << 8) + ac_amplitude_l;

	pFlickerInfo->ConfidenceLevel = 0;
	pFlickerInfo->Frequency = 0;
	pFlickerInfo->IsMeasureFinish = 0;

	/* hardware measure stop when either ac_nb_of_periods or
	 * ac_acc_periods reach maximum counter value
	 */
	if (ac_nb_of_periods == 255 || ac_acc_periods == 65535) {
		pFlickerInfo->Frequency = (coeff * ac_nb_of_periods) /
			ac_acc_periods;
		pFlickerInfo->IsMeasureFinish = 1;
		/* FIXME : confidence level need to be tune */
		if (ac_amplitude < 10 || ac_sat_metric > 2)
			pFlickerInfo->ConfidenceLevel = 1;
		else
			pFlickerInfo->ConfidenceLevel = 100;
	}

	return STALS_NO_ERROR;
}

STALS_ErrCode_t STALS_SetControl(void *pHandle,
	enum STALS_Control_Id_t ControlId, uint32_t ControlValue)
{
	struct VD6281_device *dev = get_active_device(pHandle);
	STALS_ErrCode_t res;

	CHECK_DEVICE_VALID(dev);

	switch (ControlId) {
	case STALS_PEDESTAL_ENABLE:
		CHECH_DEV_ST_FLICKER_NOT_STARTED(dev);
		res = set_pedestal_enable(dev, ControlValue);
		break;
	case STALS_PEDESTAL_VALUE:
		CHECH_DEV_ST_FLICKER_NOT_STARTED(dev);
		res = set_pedestal_value(dev, ControlValue);
		break;
	case STALS_OTP_USAGE_ENABLE:
		CHECH_DEV_ST_INIT(dev);
		res = set_otp_usage_enable(dev, ControlValue);
		break;
	case STALS_OUTPUT_DARK_ENABLE:
		CHECH_DEV_ST_INIT(dev);
		res = set_output_dark_enable(dev, ControlValue);
		break;
	case STALS_SDA_DRIVE_VALUE_MA:
		CHECH_DEV_ST_INIT(dev);
		res = set_sda_drive_value(dev, ControlValue);
		break;
	case STALS_WA_STATE:
		CHECH_DEV_ST_INIT(dev);
		res = set_wa_status(dev, ControlValue);
		break;
	default:
		res = STALS_ERROR_INVALID_PARAMS;
	}

	return res;
}

STALS_ErrCode_t STALS_GetControl(void *pHandle,
	enum STALS_Control_Id_t ControlId, uint32_t *pControlValue)
{
	struct VD6281_device *dev = get_active_device(pHandle);
	STALS_ErrCode_t res;

	CHECK_DEVICE_VALID(dev);
	CHECK_NULL_PTR(pControlValue);

	switch (ControlId) {
	case STALS_PEDESTAL_ENABLE:
		res = get_pedestal_enable(dev, pControlValue);
		break;
	case STALS_PEDESTAL_VALUE:
		res = get_pedestal_value(dev, pControlValue);
		break;
	case STALS_OTP_USAGE_ENABLE:
		res = get_otp_usage_enable(dev, pControlValue);
		break;
	case STALS_OUTPUT_DARK_ENABLE:
		res = get_output_dark_enable(dev, pControlValue);
		break;
	case STALS_SDA_DRIVE_VALUE_MA:
		res = get_sda_drive_value(dev, pControlValue);
		break;
	case STALS_WA_STATE:
		res = get_wa_status(dev, pControlValue);
		break;
	default:
		res = STALS_ERROR_INVALID_PARAMS;
	}

	return res;
}


static int vd6281_print_reg_status(void)
{
	int reg, err;
	u8 recvData;

	for (reg = 0; reg <= VD6281_INTR_CFG; reg++) {
		err = vd6281_read_reg(vd6281_data, reg, &recvData, 1);
		if (err != 0) {
			ALS_err("%s - error reading 0x%02x err:%d\n",
				__func__, reg, err);
		} else {
			ALS_dbg("%s - 0x%02x = 0x%02x\n",
				__func__, reg, recvData);
		}
	}
	return 0;
}

static int vd6281_set_sampling_rate(u32 sampling_period_ns)
{
	ALS_dbg("%s - sensor_info_data not support\n", __func__);

	return 0;
}
/*
static void vd6281_irq_set_state(struct vd6281_device_data *data, int irq_enable)
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
*/
static int vd6281_power_ctrl(struct vd6281_device_data *data, int onoff)
{
	int rc = 0;
	static int i2c_1p8_enable;
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

	if (onoff == PWR_ON) {
		if (data->i2c_1p8 != NULL && i2c_1p8_enable == 0) {
			rc = regulator_enable(regulator_i2c_1p8);
			i2c_1p8_enable = 1;
			if (rc) {
				ALS_err("%s - enable i2c_1p8 failed, rc=%d\n", __func__, rc);
				goto enable_i2c_1p8_failed;
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
		if (data->pin_als_en >= 0) {
			gpio_set_value(data->pin_als_en, 0);
			rc = gpio_direction_input(data->pin_als_en);
			if (rc) {
				ALS_err("%s - gpio direction input failed, rc=%d\n",
					__func__, rc);
				goto gpio_direction_input_failed;
			}
		}

		if (data->i2c_1p8 != NULL) {
			rc = regulator_disable(regulator_i2c_1p8);
			i2c_1p8_enable = 0;
			if (rc) {
				ALS_err("%s - disable i2c_1p8 failed, rc=%d\n", __func__, rc);
				goto done;
			}
		}
	}

	goto done;

gpio_direction_input_failed:
gpio_direction_output_failed:
	if (data->i2c_1p8 != NULL) {
		regulator_disable(regulator_i2c_1p8);
		i2c_1p8_enable = 0;
	}
enable_i2c_1p8_failed:
done:
	if (data->i2c_1p8 != NULL)
		regulator_put(regulator_i2c_1p8);
get_i2c_1p8_failed:
	return rc;
}

static STALS_ErrCode_t vd6281_enable_set(struct vd6281_device_data *data, int onoff)
{
	STALS_ErrCode_t res;
	void *hdl;
	uint32_t current_inter;
	uint32_t current_exposure;
	uint16_t current_gain;
	int c;

	if (onoff) {
		res = STALS_Init("VD6281", data->client, &hdl);
		if (res != STALS_NO_ERROR) {
			ALS_err("%s - STALS_Init failed. %d\n", __func__, res);
			goto err_device_init;
		} else {
			ALS_dbg("%s - STALS_Init ok\n", __func__);
		}

		/* wa for VD6281 cut 2.0 to allow als // with flicker */
		res = STALS_SetInterMeasurementTime(hdl, 0, &current_inter);
		if (res != STALS_NO_ERROR) {
			ALS_err("%s - STALS_SetInterMeasurementTime failed. %d\n", __func__, res);
			goto err_device_init;
		} else {
			ALS_dbg("%s - STALS_SetInterMeasurementTime ok\n", __func__);
		}

		res = STALS_SetExposureTime(hdl, 100000, &current_exposure);
		if (res != STALS_NO_ERROR) {
			ALS_err("%s - STALS_SetExposureTime failed. %d\n", __func__, res);
			goto err_device_init;
		} else {
			ALS_dbg("%s - Exposure set to %d us\n", __func__, current_exposure);
		}

		for(c = 0; c < STALS_ALS_MAX_CHANNELS; c++) {
			if ((1 << c) & ALS_CHANNELS) {
				res = STALS_SetGain(hdl, 1 << c, 0x0a00, &current_gain);
				if (res != STALS_NO_ERROR) {
					ALS_err("%s - STALS_SetGain failed. %d %d\n", __func__, 1 << c, res);
					goto err_device_init;
				} else {
					ALS_dbg("%s - Channel%d gain set to 0x%04x\n", __func__, c + 1, current_gain);
				}
			}
		}
		
		res = STALS_SetGain(hdl, FLK_CHANNEL, 0x0100, &current_gain);
		if (res != STALS_NO_ERROR) {
			ALS_err("%s - STALS_SetGain failed. %d %d\n", __func__, FLK_CHANNEL, res);
			goto err_device_init;
		} else {
			ALS_dbg("%s - Channel%d gain set to 0x%04x\n", __func__, FLK_CHANNEL, current_gain);
		}

		res = STALS_SetFlickerOutputType(hdl, STALS_FLICKER_OUTPUT_ANALOG_CFG_2);
		if (res != STALS_NO_ERROR) {
			ALS_err("%s - STALS_SetFlickerOutputType failed. %d\n", __func__, res);
			goto err_device_init;
		} else {
			ALS_dbg("%s - STALS_SetFlickerOutputType ok\n", __func__);
		}
		res = STALS_Start(hdl, STALS_MODE_ALS_SINGLE_SHOT, ALS_CHANNELS);
		res = STALS_Start(hdl, STALS_MODE_FLICKER, FLK_CHANNEL);
		start_flicker_timer();

		schedule_delayed_work(&data->rear_als_work, msecs_to_jiffies(ALS_WORK_DELAY));
	} else {
		cancel_delayed_work_sync(&data->rear_als_work);
		stop_flicker_timer();

		if (data->is_als_start) {
			res = STALS_Stop(hdl, STALS_MODE_ALS_SINGLE_SHOT);
			if (res != 0)
				ALS_err("%s - STALS_Stop STALS_MODE_ALS : %d\n", __func__, res);
		}
	
		if (data->is_flk_start) {
			res = STALS_Stop(hdl, STALS_MODE_FLICKER);
			if (res != 0)
				ALS_err("%s - STALS_Stop STALS_MODE_FLICKER : %d\n", __func__, res);
		}
	
		res = STALS_Term(hdl);
		if (res != 0)
			ALS_err("%s - STALS_Term %d : %d\n", __func__, hdl, res);
	}

	return STALS_NO_ERROR;

err_device_init:
	res = STALS_Term(hdl);
	if (res != 0)
		ALS_err("%s - STALS_Term %d : %d\n", __func__, hdl, res);

	return STALS_ERROR_INIT;
}

/* als input enable/disable sysfs */
static ssize_t vd6281_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, strlen(buf), "%d\n", data->enabled);
}

static ssize_t vd6281_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);
	bool value;
	int err = 0;

	if (strtobool(buf, &value))
		return -EINVAL;

	ALS_dbg("%s - en : %d, c : %d\n", __func__, value, data->enabled);

	mutex_lock(&data->activelock);

	if (value) {
//		vd6281_irq_set_state(data, PWR_ON);

		err = vd6281_power_ctrl(data, PWR_ON);
		if (err < 0)
			ALS_err("%s - als_regulator_on fail err = %d\n",
				__func__, err);

		err = vd6281_enable_set(data, value);
		if (err == STALS_NO_ERROR) {
			ALS_dbg("%s - vd6281_enable_set ok\n", __func__);
		} else {
			input_report_rel(data->als_input_dev,
				REL_RZ, -5 + 1); /* F_ERR_I2C -5 detected i2c error */
			input_sync(data->als_input_dev);
			ALS_err("%s - enable error %d\n", __func__, err);
			
			goto err_device_init;
		}

		data->enabled = 1;

		data->mode_cnt.amb_cnt++;
	} else {
		if (data->regulator_state == 0) {
			ALS_dbg("%s - already power off - disable skip\n",
				__func__);
			goto err_already_off;
		}

		err = vd6281_enable_set(data, value);
		if (err != 0)
			ALS_err("%s - disable err : %d\n", __func__, err);

		data->enabled = 0;

err_device_init:
		err = vd6281_power_ctrl(data, PWR_OFF);
		if (err < 0)
			ALS_err("%s - als_regulator_off fail err = %d\n",
				__func__, err);
//		vd6281_irq_set_state(data, PWR_OFF);
	}


err_already_off:
	mutex_unlock(&data->activelock);

	return count;
}

static ssize_t vd6281_poll_delay_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->sampling_period_ns);
}

static ssize_t vd6281_poll_delay_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);
	u32 sampling_period_ns = 0;
	int err = 0;

	mutex_lock(&data->activelock);

	err = kstrtoint(buf, 10, &sampling_period_ns);

	if (err < 0) {
		ALS_err("%s - kstrtoint failed.(%d)\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}

	err = vd6281_set_sampling_rate(sampling_period_ns);

	if (err > 0)
		data->sampling_period_ns = err;

	ALS_dbg("%s - als sensor sampling rate is setted as %dns\n", __func__, sampling_period_ns);

	mutex_unlock(&data->activelock);

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
	vd6281_enable_show, vd6281_enable_store);
static DEVICE_ATTR(poll_delay, S_IRUGO|S_IWUSR|S_IWGRP,
	vd6281_poll_delay_show, vd6281_poll_delay_store);

static struct attribute *als_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group als_attribute_group = {
	.attrs = als_sysfs_attrs,
};

/* als_sensor sysfs */
static ssize_t vd6281_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char chip_name[NAME_LEN];

	strlcpy(chip_name, VD6281_CHIP_NAME, strlen(VD6281_CHIP_NAME) + 1);

	return snprintf(buf, PAGE_SIZE, "%s\n", chip_name);
}

static ssize_t vd6281_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char vendor[NAME_LEN];

	strlcpy(vendor, VENDOR, strlen(VENDOR) + 1);

	return snprintf(buf, PAGE_SIZE, "%s\n", vendor);
}

static ssize_t vd6281_flush_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);
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

static ssize_t vd6281_int_pin_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	/* need to check if this should be implemented */
	ALS_dbg("%s - not implement\n", __func__);
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t vd6281_read_reg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

	ALS_info("%s - val=0x%06x\n", __func__, data->reg_read_buf);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->reg_read_buf);
}

static ssize_t vd6281_read_reg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

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

	err = vd6281_read_reg(data, (u8)cmd, &val, 1);
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
static ssize_t vd6281_write_reg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

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

	err = vd6281_write_reg(data, (u8)cmd, (u8)val);
	if (err < 0) {
		ALS_err("%s - fail err = %d\n", __func__, err);
		mutex_unlock(&data->i2clock);
		return err;
	}
	mutex_unlock(&data->i2clock);

	return size;
}

static ssize_t vd6281_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

	ALS_info("%s - debug mode = %u\n", __func__, data->debug_mode);

	return snprintf(buf, PAGE_SIZE, "%u\n", data->debug_mode);
}

static ssize_t vd6281_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);
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
		vd6281_print_reg_status();
		break;
	case DEBUG_VAR:
		vd6281_debug_var(data);
		break;
	default:
		break;
	}
	mutex_unlock(&data->activelock);

	return size;
}

static ssize_t vd6281_device_id_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	ALS_dbg("%s - device_id not support\n", __func__);

	return snprintf(buf, PAGE_SIZE, "NOT SUPPORT\n");
}

static ssize_t vd6281_part_type_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);
	
	return snprintf(buf, PAGE_SIZE, "%d\n", data->part_type);
}

static ssize_t vd6281_i2c_err_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);
	u32 err_cnt = 0;

	err_cnt = data->i2c_err_cnt;

	return snprintf(buf, PAGE_SIZE, "%d\n", err_cnt);
}

static ssize_t vd6281_i2c_err_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

	data->i2c_err_cnt = 0;

	return size;
}

static ssize_t vd6281_curr_adc_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE,
		"\"HRIC\":\"%d\",\"HRRC\":\"%d\",\"HRIA\":\"%d\",\"HRRA\":\"%d\"\n",
		0, 0, data->user_ir_data, data->user_flicker_data);
}

static ssize_t vd6281_curr_adc_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

	data->user_ir_data = 0;
	data->user_flicker_data = 0;

	return size;
}

static ssize_t vd6281_mode_cnt_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE,
		"\"CNT_HRM\":\"%d\",\"CNT_AMB\":\"%d\",\"CNT_PROX\":\"%d\",\"CNT_SDK\":\"%d\",\"CNT_CGM\":\"%d\",\"CNT_UNKN\":\"%d\"\n",
		data->mode_cnt.hrm_cnt, data->mode_cnt.amb_cnt, data->mode_cnt.prox_cnt,
		data->mode_cnt.sdk_cnt, data->mode_cnt.cgm_cnt, data->mode_cnt.unkn_cnt);
}

static ssize_t vd6281_mode_cnt_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

	data->mode_cnt.hrm_cnt = 0;
	data->mode_cnt.amb_cnt = 0;
	data->mode_cnt.prox_cnt = 0;
	data->mode_cnt.sdk_cnt = 0;
	data->mode_cnt.cgm_cnt = 0;
	data->mode_cnt.unkn_cnt = 0;

	return size;
}

static ssize_t vd6281_factory_cmd_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);
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

static ssize_t vd6281_version_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	ALS_info("%s - cmd_result = %s.%s.%s%s\n", __func__,
		VERSION, SUB_VERSION, HEADER_VERSION, VENDOR_VERSION);

	return snprintf(buf, PAGE_SIZE, "%s.%s.%s%s\n",
		VERSION, SUB_VERSION, HEADER_VERSION, VENDOR_VERSION);
}

static ssize_t vd6281_sensor_info_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	ALS_dbg("%s - sensor_info_data not support\n", __func__);

	return snprintf(buf, PAGE_SIZE, "NOT SUPPORT\n");
}

#ifdef CONFIG_STM_ALS_EOL_MODE
static int vd6281_eol_mode(struct vd6281_device_data *data)
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

static ssize_t vd6281_eol_mode_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

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

static ssize_t vd6281_eol_mode_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);
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
//	vd6281_irq_set_state(data, PWR_ON);

	err = vd6281_power_ctrl(data, PWR_ON);
	if (err < 0)
		ALS_err("%s - als_regulator_on fail err = %d\n",
			__func__, err);

	err = vd6281_enable_set(data, 1);
	if (err == STALS_NO_ERROR) {
		ALS_dbg("%s - vd6281_enable_set ok\n", __func__);
	} else {
		ALS_err("%s - enable error %d\n", __func__, err);
		goto err_device_init;
	}

	data->enabled = 1;

	mutex_unlock(&data->activelock);
	vd6281_eol_mode(data);
	mutex_lock(&data->activelock);

	if (data->regulator_state == 0) {
		ALS_dbg("%s - already power off - disable skip\n",
			__func__);
		goto err_already_off;
	}

	err = vd6281_enable_set(data, 0);
	if (err != 0)
		ALS_err("%s - disable err : %d\n", __func__, err);

	data->enabled = 0;

err_device_init:
	err = vd6281_power_ctrl(data, PWR_OFF);
	if (err < 0)
		ALS_err("%s - als_regulator_off fail err = %d\n",
			__func__, err);
//	vd6281_irq_set_state(data, PWR_OFF);

err_already_off:
	mutex_unlock(&data->activelock);

	return size;
}

static ssize_t vd6281_eol_spec_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);

	ALS_dbg("%s - ir_spec = %d, %d, %d, %d, %d, %d, %d, %d\n", __func__,
		data->eol_ir_spec[0], data->eol_ir_spec[1], data->eol_clear_spec[0], data->eol_clear_spec[1],
		data->eol_ir_spec[2], data->eol_ir_spec[3], data->eol_clear_spec[2], data->eol_clear_spec[3]);

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d, %d, %d, %d, %d, %d\n",
		data->eol_ir_spec[0], data->eol_ir_spec[1], data->eol_clear_spec[0], data->eol_clear_spec[1],
		data->eol_ir_spec[2], data->eol_ir_spec[3], data->eol_clear_spec[2], data->eol_clear_spec[3]);
}
#endif

static DEVICE_ATTR(name, S_IRUGO, vd6281_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, vd6281_vendor_show, NULL);
static DEVICE_ATTR(als_flush, S_IWUSR | S_IWGRP, NULL, vd6281_flush_store);
static DEVICE_ATTR(int_pin_check, S_IRUGO, vd6281_int_pin_check_show, NULL);
static DEVICE_ATTR(read_reg, S_IRUGO | S_IWUSR | S_IWGRP,
	vd6281_read_reg_show, vd6281_read_reg_store);
static DEVICE_ATTR(write_reg, S_IWUSR | S_IWGRP, NULL, vd6281_write_reg_store);
static DEVICE_ATTR(als_debug, S_IRUGO | S_IWUSR | S_IWGRP,
	vd6281_debug_show, vd6281_debug_store);
static DEVICE_ATTR(device_id, S_IRUGO, vd6281_device_id_show, NULL);
static DEVICE_ATTR(part_type, S_IRUGO, vd6281_part_type_show, NULL);
static DEVICE_ATTR(i2c_err_cnt, S_IRUGO | S_IWUSR | S_IWGRP, vd6281_i2c_err_show, vd6281_i2c_err_store);
static DEVICE_ATTR(curr_adc, S_IRUGO | S_IWUSR | S_IWGRP, vd6281_curr_adc_show, vd6281_curr_adc_store);
static DEVICE_ATTR(mode_cnt, S_IRUGO | S_IWUSR | S_IWGRP, vd6281_mode_cnt_show, vd6281_mode_cnt_store);
static DEVICE_ATTR(als_factory_cmd, S_IRUGO, vd6281_factory_cmd_show, NULL);
static DEVICE_ATTR(als_version, S_IRUGO, vd6281_version_show, NULL);
static DEVICE_ATTR(sensor_info, S_IRUGO, vd6281_sensor_info_show, NULL);
//static DEVICE_ATTR(als_raw_data, S_IRUGO, als_raw_data_show, NULL);
#ifdef CONFIG_STM_ALS_EOL_MODE
static DEVICE_ATTR(eol_mode, S_IRUGO | S_IWUSR | S_IWGRP, vd6281_eol_mode_show, vd6281_eol_mode_store);
static DEVICE_ATTR(eol_spec, S_IRUGO, vd6281_eol_spec_show, NULL);
#endif

static struct device_attribute *vd6281_sensor_attrs[] = {
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
//	&dev_attr_als_raw_data,
#ifdef CONFIG_STM_ALS_EOL_MODE
	&dev_attr_eol_mode,
	&dev_attr_eol_spec,
#endif
	NULL,
};

#if 0
static int vd6281_eol_mode_handler(struct vd6281_device_data *data)
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

irqreturn_t vd6281_irq_handler(int dev_irq, void *device)
{
//	int err;
	struct vd6281_device_data *data = device;
//	int interruptsHandled = 0;

	ALS_info("%s - als_irq = %d\n", __func__, dev_irq);

	if (data->regulator_state == 0 || data->enabled == 0) {
		ALS_dbg("%s - stop irq handler (reg_state : %d, enabled : %d)\n",
				__func__, data->regulator_state, data->enabled);

		return IRQ_HANDLED;
	}

#ifdef CONFIG_ARCH_QCOM
	pm_qos_add_request(&data->pm_qos_req_fpm, PM_QOS_CPU_DMA_LATENCY,
		PM_QOS_DEFAULT_VALUE);
#endif
/*
	err = ams_deviceEventHandler(data->deviceCtx);
	interruptsHandled = ams_getResult(data->deviceCtx);

	if (err == 0) {
		if (data->als_input_dev == NULL) {
			ALS_err("%s - als_input_dev is NULL\n", __func__);
		} else {
#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS
			if (interruptsHandled & (1 << AMS_AMBIENT_SENSOR)) {
				report_als(data);
#ifdef CONFIG_STM_ALS_EOL_MODE
				if (data->eol_enable)
					vd6281_eol_mode_handler(data);
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
*/
#ifdef CONFIG_ARCH_QCOM
	pm_qos_remove_request(&data->pm_qos_req_fpm);
#endif

	return IRQ_HANDLED;
}
/*
static int vd6281_setup_irq(struct vd6281_device_data *data)
{
	int errorno = -EIO;

	errorno = request_threaded_irq(data->dev_irq, NULL,
		vd6281_irq_handler, IRQF_TRIGGER_FALLING|IRQF_ONESHOT,
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
*/
static void vd6281_init_var(struct vd6281_device_data *data)
{
	data->client = NULL;
	data->dev = NULL;
	data->als_input_dev = NULL;
	data->als_pinctrl = NULL;
	data->pins_sleep = NULL;
	data->pins_idle = NULL;
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
	data->flicker_cnt = 0;
#ifdef CONFIG_STM_ALS_EOL_MODE
	data->eol_pulse_duty[0] = DEFAULT_DUTY_50HZ;
	data->eol_pulse_duty[1] = DEFAULT_DUTY_60HZ;
	data->eol_pulse_count = 0;
#endif
}

static int vd6281_parse_dt(struct vd6281_device_data *data)
{
	struct device *dev = &data->client->dev;
	struct device_node *dNode = dev->of_node;
	enum of_gpio_flags flags;
	const char *buf;
//	u32 gain_max = 0;

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

#ifdef CONFIG_STM_ALS_EOL_MODE
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

//	if (of_property_read_u32(dNode, "als_rear,gain_max", &gain_max) == 0) {
//		deviceRegisterDefinition[DEVREG_AGC_GAIN_MAX].resetValue = gain_max;
//
//		ALS_dbg("%s - DEVREG_AGC_GAIN_MAX = 0x%x\n", __func__, gain_max);
//	}
		
#ifdef CONFIG_STM_ALS_EOL_MODE
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
	if (of_property_read_string(dNode, "io-channel-names", &buf)) {
		return -ENOENT;
	} else {
		data->chan = iio_channel_get(&data->client->dev, buf);
		ALS_dbg("%s - get iio_channel : %d\n", __func__, !IS_ERR_OR_NULL(data->chan));
	}

	ALS_dbg("%s - done.\n", __func__);

	return 0;
}

static int vd6281_setup_gpio(struct vd6281_device_data *data)
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

static long vd6281_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret = 0;

	struct vd6281_device_data *data = container_of(file->private_data,
	struct vd6281_device_data, miscdev);

	ALS_info("%s - ioctl start\n", __func__);
	mutex_lock(&data->flickerdatalock);

	switch (cmd) {
	case VD6281_IOCTL_READ_FLICKER:
		ret = copy_to_user(argp,
			data->flicker_data,
			sizeof(int)*FLICKER_DATA_CNT);

		if (unlikely(ret))
			goto ioctl_error;

		complete(&data->completion);

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


static const struct file_operations vd6281_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.unlocked_ioctl = vd6281_ioctl,
};
int vd6281_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = -ENODEV;
//	STALS_ErrCode_t res;
	struct device *dev = &client->dev;
	struct vd6281_device_data *data;
//	void *hdl;

	ALS_dbg("%s - start\n", __func__);
	/* check to make sure that the adapter supports I2C */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		ALS_err("%s - I2C_FUNC_I2C not supported\n", __func__);
		return -ENODEV;
	}
	/* allocate some memory for the device */
	data = devm_kzalloc(dev, sizeof(struct vd6281_device_data), GFP_KERNEL);
	if (data == NULL) {
		ALS_err("%s - couldn't allocate device data memory\n", __func__);
		return -ENOMEM;
	}
/*
	data->flicker_data = kzalloc(sizeof(int)*FLICKER_DATA_CNT * 2, GFP_KERNEL);
	if (data->flicker_data == NULL) {
		HRM_err("%s - couldn't allocate flicker data memory\n", __func__);
		goto err_flicker_alloc_fail;
	}
*/
	vd6281_data = data;
	vd6281_init_var(data);

	data->client = client;
	data->miscdev.minor = MISC_DYNAMIC_MINOR;
	data->miscdev.name = MODULE_NAME_ALS;
	data->miscdev.mode = S_IRUGO;
	data->miscdev.fops = &vd6281_fops;
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
	init_completion(&data->completion);
	tasklet_hrtimer_init(&data->timer,
		my_hrtimer_callback, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	err = vd6281_parse_dt(data);
	if (err < 0) {
		ALS_err("%s - failed to parse dt\n", __func__);
		err = -ENODEV;
		goto err_parse_dt;
	}
	err = vd6281_setup_gpio(data);
	if (err) {
		ALS_err("%s - failed to setup gpio\n", __func__);
		goto err_setup_gpio;
	}
	err = vd6281_power_ctrl(data, PWR_ON);
	if (err < 0) {
		ALS_err("%s - failed to power on ctrl\n", __func__);
		goto err_power_on;
	}
/*
	res = STALS_Init("VD6281", data->client, &hdl);
	if (res != STALS_NO_ERROR) {
		ALS_err("%s - STALS_Init failed. %d\n", __func__, res);
		goto err_device_init;
	} else {
		ALS_dbg("%s - STALS_Init ok\n", __func__);
	}
	

	err = vd6281_enable_set(data, 1);
	if (err == STALS_NO_ERROR) {
		ALS_dbg("%s - vd6281_enable_set ok\n", __func__);
	} else {
		ALS_err("%s - enable error %d\n", __func__, err);
		goto err_device_init;
	}
*/
	data->enabled = 1;

	data->flicker_work = create_singlethread_workqueue("flicker");
	INIT_DELAYED_WORK(&data->rear_als_work, vd6281_als_work);

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
	err = sensors_register(&data->dev, data, vd6281_sensor_attrs,
			MODULE_NAME_ALS);
#else
	err = sensors_register(data->dev, data, vd6281_sensor_attrs,
			MODULE_NAME_ALS);
#endif
	if (err) {
		ALS_err("%s - cound not register als_sensor(%d).\n", __func__, err);
		goto als_sensor_register_failed;
	}
/*
	err = vd6281_setup_irq(data);
	if (err) {
		ALS_err("%s - could not setup dev_irq\n", __func__);
		goto err_setup_irq;
	}
*/
	err = vd6281_enable_set(data, 0);
	if (err != 0)
		ALS_err("%s - disable err : %d\n", __func__, err);

	data->enabled = 0;

	err = vd6281_power_ctrl(data, PWR_OFF);
	if (err < 0) {
		ALS_err("%s - failed to power off ctrl\n", __func__);
		goto dev_set_drvdata_failed;
	}
	ALS_dbg("%s - success\n", __func__);
	goto done;

dev_set_drvdata_failed:
//	free_irq(data->dev_irq, data);
//err_setup_irq:
	sensors_unregister(data->dev, vd6281_sensor_attrs);
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
//err_device_init:
	vd6281_power_ctrl(data, PWR_OFF);
err_power_on:
	gpio_free(data->pin_als_int);
	if (data->pin_als_en >= 0)
		gpio_free(data->pin_als_en);
err_setup_gpio:
err_parse_dt:
//err_malloc_pdata:
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
//	kfree(data->flicker_data);
//err_flicker_alloc_fail:
//	devm_kfree(data);
	ALS_err("%s failed\n", __func__);
done:
	return err;
}

int vd6281_remove(struct i2c_client *client)
{
	struct vd6281_device_data *data = i2c_get_clientdata(client);

	vd6281_power_ctrl(data, PWR_OFF);

	sensors_unregister(data->dev, vd6281_sensor_attrs);
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
	destroy_workqueue(data->flicker_work);
	iio_channel_release(data->chan);

//	devm_kfree(data->deviceCtx);
//	devm_kfree(data->pdata);
//	devm_kfree(data);
	i2c_set_clientdata(client, NULL);

	data = NULL;
	return 0;
}

static void vd6281_shutdown(struct i2c_client *client)
{
	ALS_dbg("%s - start\n", __func__);
}

void vd6281_pin_control(struct vd6281_device_data *data, bool pin_set)
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
static int vd6281_suspend(struct device *dev)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);
	int err = 0;

	ALS_dbg("%s - %d\n", __func__, data->enabled);

	if (data->enabled != 0 || data->regulator_state != 0) {
		mutex_lock(&data->activelock);

		vd6281_enable_set(data, 0);

		err = vd6281_power_ctrl(data, PWR_OFF);
		if (err < 0)
			ALS_err("%s - als_regulator_off fail err = %d\n",
				__func__, err);
//		vd6281_irq_set_state(data, PWR_OFF);

		mutex_unlock(&data->activelock);
	}
	mutex_lock(&data->suspendlock);

	data->pm_state = PM_SUSPEND;
	vd6281_pin_control(data, false);

	mutex_unlock(&data->suspendlock);

	return err;
}

static int vd6281_resume(struct device *dev)
{
	struct vd6281_device_data *data = dev_get_drvdata(dev);
	int err = 0;

	ALS_dbg("%s - %d\n", __func__, data->enabled);

	mutex_lock(&data->suspendlock);

	vd6281_pin_control(data, true);

	data->pm_state = PM_RESUME;

	mutex_unlock(&data->suspendlock);

	if (data->enabled != 0) {
		mutex_lock(&data->activelock);

//		vd6281_irq_set_state(data, PWR_ON);

		err = vd6281_power_ctrl(data, PWR_ON);
		if (err < 0)
			ALS_err("%s - als_regulator_on fail err = %d\n",
				__func__, err);

		vd6281_enable_set(data, 1);

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

static const struct dev_pm_ops vd6281_pm_ops = {
	.suspend = vd6281_suspend,
	.resume = vd6281_resume
};
#endif

static const struct of_device_id vd6281_match_table[] = {
	{ .compatible = "stm,vd6281",},
	{},
};

static const struct i2c_device_id vd6281_idtable[] = {
	{ "vd6281", 0 },
	{ }
};
/* descriptor of the vd6281 I2C driver */
static struct i2c_driver vd6281_driver = {
	.driver = {
		.name = "vd6281",
		.owner = THIS_MODULE,
#if defined(CONFIG_PM)
		.pm = &vd6281_pm_ops,
#endif
		.of_match_table = vd6281_match_table,
	},
	.probe = vd6281_probe,
	.remove = vd6281_remove,
	.shutdown = vd6281_shutdown,
	.id_table = vd6281_idtable,
};

/* initialization and exit functions */
static int __init vd6281_init(void)
{
	if (!lpcharge)
		return i2c_add_driver(&vd6281_driver);
	else
		return 0;
}

static void __exit vd6281_exit(void)
{
	i2c_del_driver(&vd6281_driver);
}

module_init(vd6281_init);
module_exit(vd6281_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("VD6281 ALS Driver");
MODULE_LICENSE("GPL");
