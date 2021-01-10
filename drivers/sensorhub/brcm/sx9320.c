/*
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/power_supply.h>
#include "sx9320_reg.h"
#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#ifdef CONFIG_CCIC_NOTIFIER
#include <linux/ccic/ccic_notifier.h>
#endif
#endif
#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/manager/usb_typec_manager_notifier.h>
#endif

#define VENDOR_NAME              "SEMTECH"
#define MODEL_NAME               "SX9320"
#define MODULE_NAME              "grip_sensor"

#define I2C_M_WR                 0 /* for i2c Write */
#define I2c_M_RD                 1 /* for i2c Read */

#define IDLE                     0
#define ACTIVE                   1

#define SX9320_MODE_SLEEP        0
#define SX9320_MODE_NORMAL       1

#define MAIN_SENSOR              1
#define REF_SENSOR               2

#define DIFF_READ_NUM            10
#define GRIP_LOG_TIME            15 /* 30 sec */
#define PHX_STATUS_REG           SX9320_STAT0_PROXSTAT_PH0_FLAG
#define RAW_DATA_BLOCK_SIZE      (SX9320_REGOFFSETLSB - SX9320_REGUSEMSB + 1)

/* CS0, CS1, CS2, CS3 */
#define TOTAL_BOTTON_COUNT       1
#define ENABLE_CSX               ((1 << MAIN_SENSOR) | (1 << REF_SENSOR))

#define IRQ_PROCESS_CONDITION   (SX9320_IRQSTAT_TOUCH_FLAG \
				| SX9320_IRQSTAT_RELEASE_FLAG)

#define NONE_ENABLE		-1
#define IDLE_STATE		0
#define TOUCH_STATE		1

#define HALLIC1_PATH		"/sys/class/sec/hall_ic/hall_detect"

struct sx9320_p {
	struct i2c_client *client;
	struct input_dev *input;
	struct device *factory_device;
	struct delayed_work init_work;
	struct delayed_work irq_work;
	struct delayed_work debug_work;
	struct wake_lock grip_wake_lock;
	struct mutex read_mutex;
#if defined(CONFIG_MUIC_NOTIFIER)
	struct notifier_block cpuidle_muic_nb;
#endif
#if defined(CONFIG_CCIC_NOTIFIER)
	struct notifier_block cpuidle_ccic_nb;
#endif
	bool skip_data;
	bool check_usb;
	u8 normal_th;
	u8 normal_th_buf;
	u8 phen;
	u8 gain;
	u8 again;
	int again_ch;
	u8 scan_period;
	u8 range;
	u8 sampling_freq;
	u8 resolution;
	u8 rawfilt;
	u8 hyst;
	u8 avgposfilt;
	u8 avgnegfilt;
	u8 avgthresh;
	u8 debouncer;
	u8 afeph0;
	u8 afeph1;
	u8 afeph2;
	u8 afeph3;
	int irq;
	int gpio_nirq;
	int state;
	int debug_count;
	int diff_avg;
	int diff_cnt;
	int init_done;
	int irq_count;
	int abnormal_mode;

	s32 capmain;
	s32 useful;
	s32 useful_avg;
	s16 avg;
	s32 diff;
	s32 max_diff;
	s32 max_normal_diff;
	u16 offset;
	u16 freq;

	int ch1_state;
	int ch2_state;

	atomic_t enable;

	unsigned char hall_ic1[6];
	unsigned char hall_ic2[6];
};

static int sx9320_check_hallic_state(char *file_path,
	unsigned char hall_ic_status[])
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *filep;
	u8 hall_sysfs[5];

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filep = filp_open(file_path, O_RDONLY, 0666);
	if (IS_ERR(filep)) {
		iRet = PTR_ERR(filep);
		set_fs(old_fs);
		goto exit;
	}

	iRet = filep->f_op->read(filep, hall_sysfs,
		sizeof(hall_sysfs), &filep->f_pos);

	if (iRet != sizeof(hall_sysfs))
		iRet = -EIO;
	else
		strncpy(hall_ic_status, hall_sysfs, sizeof(hall_sysfs));

	filp_close(filep, current->files);
	set_fs(old_fs);

exit:
	return iRet;
}

static int sx9320_get_nirq_state(struct sx9320_p *data)
{
	return gpio_get_value_cansleep(data->gpio_nirq);
}

static int sx9320_i2c_write(struct sx9320_p *data, u8 reg_addr, u8 buf)
{
	int ret;
	struct i2c_msg msg;
	unsigned char w_buf[2];

	w_buf[0] = reg_addr;
	w_buf[1] = buf;

	msg.addr = data->client->addr;
	msg.flags = I2C_M_WR;
	msg.len = 2;
	msg.buf = (char *)w_buf;

	ret = i2c_transfer(data->client->adapter, &msg, 1);
	if (ret < 0)
		pr_err("[SX9320]: %s - i2c write error %d\n",
			__func__, ret);

	return ret;
}

static int sx9320_i2c_read(struct sx9320_p *data, u8 reg_addr, u8 *buf)
{
	int ret;
	struct i2c_msg msg[2];

	msg[0].addr = data->client->addr;
	msg[0].flags = I2C_M_WR;
	msg[0].len = 1;
	msg[0].buf = &reg_addr;

	msg[1].addr = data->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(data->client->adapter, msg, 2);
	if (ret < 0)
		pr_err("[SX9320]: %s - i2c read error %d\n",
			__func__, ret);

	return ret;
}

static void sx9320_get_again(struct sx9320_p *data)
{
	switch (data->again) {
	case 0x06:
		data->again_ch = 1247;
		break;
	case 0x08:
		data->again_ch = 1000;
		break;
	case 0x0B:
		data->again_ch = 768;
		break;
	case 0x0F:
		data->again_ch = 552;
		break;
	default:
		data->again_ch = 1000;
	}
}

static u8 sx9320_read_irqstate(struct sx9320_p *data)
{
	u8 val = 0;

	if (sx9320_i2c_read(data, SX9320_IRQSTAT_REG, &val) >= 0)
		return val;

	return 0;
}

static void sx9320_initialize_register(struct sx9320_p *data)
{
	u8 val = 0;
	int idx;

	for (idx = 0; idx < (int)(sizeof(setup_reg) >> 1); idx++) {
		sx9320_i2c_write(data, setup_reg[idx].reg, setup_reg[idx].val);
		pr_info("[SX9320]: %s - Write Reg: 0x%x Value: 0x%x\n",
			__func__, setup_reg[idx].reg, setup_reg[idx].val);

		sx9320_i2c_read(data, setup_reg[idx].reg, &val);
		pr_info("[SX9320]: %s - Read Reg: 0x%x Value: 0x%x\n\n",
			__func__, setup_reg[idx].reg, val);
	}

	if (data->phen < 2)
		sx9320_i2c_write(data, SX9320_PROXCTRL6_REG, data->normal_th);
	else
		sx9320_i2c_write(data, SX9320_PROXCTRL7_REG, data->normal_th);

	data->init_done = ON;
}

static void sx9320_initialize_chip(struct sx9320_p *data)
{
	int cnt = 0;

	while ((sx9320_get_nirq_state(data) == 0) && (cnt++ < 10)) {
		sx9320_read_irqstate(data);
		msleep(20);
	}

	if (cnt >= 10)
		pr_err("[SX9320]: %s - s/w reset fail(%d)\n", __func__, cnt);

	sx9320_initialize_register(data);
}

static int sx9320_set_offset_calibration(struct sx9320_p *data)
{
	int ret = 0;

	ret = sx9320_i2c_write(data, SX9320_STAT2_REG, 0x0F);

	return ret;
}

static void sx9320_send_event(struct sx9320_p *data, u8 state)
{
	if (state == ACTIVE) {
		data->state = ACTIVE;
		pr_info("[SX9320]: %s - button touched\n", __func__);
	} else {
		data->state = IDLE;
		pr_info("[SX9320]: %s - button released\n", __func__);
	}

	if (data->skip_data == true) {
		pr_info("[SX9320]: %s - skip grip event\n", __func__);
		return;
	}

	if (state == ACTIVE)
		input_report_rel(data->input, REL_MISC, 1);
	else
		input_report_rel(data->input, REL_MISC, 2);

	input_sync(data->input);
}

static void sx9320_display_data_reg(struct sx9320_p *data)
{
	u8 val = 0;
	int idx;

	sx9320_i2c_write(data, SX9320_REGSENSORSELECT, (u8)data->phen);

	for (idx = 0; idx < (int)(sizeof(setup_reg) >> 1); idx++) {
		sx9320_i2c_read(data, setup_reg[idx].reg, &val);
		pr_info("[SX9320]: %s - Read Reg: 0x%x Value: 0x%x\n\n",
			__func__, setup_reg[idx].reg, val);
	}
}

static void sx9320_get_data(struct sx9320_p *data)
{
	u8 ms_byte = 0;
	u8 ls_byte = 0;
	s16 avg = 0;
	u16 offset = 0;
	s32 capMain = 0, useful = 0, range = 0;
	s32 gain, again;

	mutex_lock(&data->read_mutex);

	sx9320_i2c_write(data, SX9320_REGSENSORSELECT, (u8)data->phen);

	/* useful read */
	sx9320_i2c_read(data, SX9320_REGUSEMSB, &ms_byte);
	sx9320_i2c_read(data, SX9320_REGUSELSB, &ls_byte);

	useful = (s32)ms_byte;
	useful = (useful << 8) | ((s32)ls_byte);
	if (useful > 32767)
		useful -= 65536;

	/* offset read */
	sx9320_i2c_read(data, SX9320_REGOFFSETMSB, &ms_byte);
	sx9320_i2c_read(data, SX9320_REGOFFSETLSB, &ls_byte);

	offset = (u16)ms_byte;
	offset = (offset << 8) | ((u16)ls_byte);

	/* capMain calculate */
	ms_byte = (u8)((offset >> 7) & 0x7F);
	ls_byte = (u8)((offset)      & 0x7F);

	gain = 1 << (data->gain - 1);
	again = data->again_ch;

	/* range small(00) : 1.325 Large(01) : 2.65 */
	if (data->range == 1)
		range = SX9320_LARGE_RANGE_VALUE;
	else
		range = SX9320_SMALL_RANGE_VALUE;

	capMain = (((s32)ms_byte * 21234) + ((s32)ls_byte * 496))
			+ ((useful * range) / ((gain * again * 65536) / 1000));

	/* avg read */
	sx9320_i2c_read(data, SX9320_REGAVGMSB, &ms_byte);
	sx9320_i2c_read(data, SX9320_REGAVGLSB, &ls_byte);

	avg = (u16)ms_byte;
	avg = (avg << 8) | ((u16)ls_byte);

	data->useful = useful;
	data->offset = offset;
	data->capmain = capMain;
	data->avg = avg;
	data->diff = useful - avg;

	mutex_unlock(&data->read_mutex);

	pr_info("[SX9320]: %s - Capmain: %d, Useful: %d, avg: %d, diff: %d, Offset: %u\n",
		__func__, data->capmain, data->useful, data->avg,
		data->diff, data->offset);
}

static int sx9320_set_mode(struct sx9320_p *data, unsigned char mode)
{
	int ret = -EINVAL;

	/* sx9320 enables the each PHASES(PHEN reg) instead of CSX */
	if (mode == SX9320_MODE_SLEEP) {
		ret = sx9320_i2c_write(data, SX9320_GNRLCTRL1_REG,
			setup_reg[2].val);
	} else if (mode == SX9320_MODE_NORMAL) {
		ret = sx9320_i2c_write(data, SX9320_GNRLCTRL1_REG,
			setup_reg[2].val | (1 << data->phen));

		msleep(20);

		sx9320_set_offset_calibration(data);
		msleep(400);
	}

	pr_info("[SX9320]: %s - change the mode : %u\n", __func__, mode);
	return ret;
}

static void sx9320_read_ch_interrupt(struct sx9320_p *data, u8 status)
{
	if (status & (PHX_STATUS_REG << data->phen))
		data->ch1_state = TOUCH_STATE;
	else
		data->ch1_state = IDLE_STATE;

	pr_info("[SX9320]: %s - ch1:%d, ch2:%d\n",
		__func__, data->ch1_state, data->ch2_state);
}

static void sx9320_check_status(struct sx9320_p *data, int enable)
{
	/* this function has to be modified if chs are over 2 for SAR */
	u8 status = 0;

	sx9320_i2c_read(data, SX9320_STAT0_REG, &status);

	if (data->skip_data == true) {
		input_report_rel(data->input, REL_MISC, 2);
		input_sync(data->input);
	} else if (status & (PHX_STATUS_REG << data->phen)) {
		sx9320_send_event(data, ACTIVE);
	} else {
		sx9320_send_event(data, IDLE);
	}
}

static void sx9320_set_enable(struct sx9320_p *data, int enable)
{
	u8 status = 0;

	pr_info("[SX9320]: %s\n", __func__);

	if (enable == ON) {

		pr_info("[SX9320]: %s - enable(status : 0x%x)\n",
			__func__, status);

		data->diff_avg = 0;
		data->diff_cnt = 0;
		data->useful_avg = 0;
		sx9320_get_data(data);
		sx9320_check_status(data, enable);

		msleep(20);
		/*
		 * make sure no interrupts are pending since enabling irq
		 * will only work on next falling edge
		 */
		sx9320_read_irqstate(data);
		/*
		 * enable interrupt
		 */
		sx9320_i2c_write(data, SX9320_IRQ_ENABLE_REG, 0x70);

		enable_irq(data->irq);
		enable_irq_wake(data->irq);
	} else {
		pr_info("[SX9320]: %s - disable\n", __func__);

		/*
		 * disable interrupt
		 */
		sx9320_i2c_write(data, SX9320_IRQ_ENABLE_REG, 0x00);

		disable_irq(data->irq);
		disable_irq_wake(data->irq);
	}
}

static void sx9320_set_debug_work(struct sx9320_p *data, u8 enable,
		unsigned int time_ms)
{
	if (enable == ON) {
		data->debug_count = 0;
		schedule_delayed_work_on(1, &data->debug_work,
			msecs_to_jiffies(time_ms));
	} else {
		cancel_delayed_work_sync(&data->debug_work);
	}
}

static ssize_t sx9320_get_offset_calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 val = 0;
	struct sx9320_p *data = dev_get_drvdata(dev);

	sx9320_i2c_read(data, SX9320_IRQSTAT_REG, &val);

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t sx9320_set_offset_calibration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	struct sx9320_p *data = dev_get_drvdata(dev);

	if (kstrtoul(buf, 10, &val)) {
		pr_err("[SX9320]: %s - Invalid Argument\n", __func__);
		return -EINVAL;
	}

	if (val)
		sx9320_set_offset_calibration(data);

	return count;
}

static ssize_t sx9320_register_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int regist = 0, val = 0;
	struct sx9320_p *data = dev_get_drvdata(dev);

	if (sscanf(buf, "%3d,%3d", &regist, &val) != 2) {
		pr_err("[SX9320]: %s - The number of data are wrong\n",
			__func__);
		return -EINVAL;
	}

	sx9320_i2c_write(data, (unsigned char)regist, (unsigned char)val);
	pr_info("[SX9320]: %s - Register(0x%2x) data(0x%2x)\n",
		__func__, regist, val);
	return count;
}

static ssize_t sx9320_register_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 val = 0;
	int offset = 0, idx = 0;
	struct sx9320_p *data = dev_get_drvdata(dev);

	for (idx = 0; idx < (int)(sizeof(setup_reg) >> 1); idx++) {
		sx9320_i2c_read(data, setup_reg[idx].reg, &val);

		pr_info("[SX9320]: %s - Read Reg: 0x%x Value: 0x%x\n\n",
			__func__, setup_reg[idx].reg, val);

		offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"Reg: 0x%x Value: 0x%x\n", setup_reg[idx].reg, val);
	}

	return offset;
}

static ssize_t sx9320_read_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	sx9320_display_data_reg(data);

	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t sx9320_sw_reset_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	pr_info("[SX9320]: %s\n", __func__);
	sx9320_set_offset_calibration(data);
	msleep(400);
	sx9320_get_data(data);

	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t sx9320_freq_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	struct sx9320_p *data = dev_get_drvdata(dev);

	if (kstrtoul(buf, 10, &val)) {
		pr_err("[SX9320]: %s - Invalid Argument\n", __func__);
		return count;
	}

	data->freq = (u16)val;
	val = ((val << 3) | (setup_reg[4].val & 0x07)) & 0xff;
	sx9320_i2c_write(data, SX9320_AFECTRL4_REG, (u8)val);

	pr_info("[SX9320]: %s - Freq : 0x%x\n", __func__, data->freq);

	return count;
}

static ssize_t sx9320_freq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	pr_info("[SX9320]: %s - Freq : 0x%x\n", __func__, data->freq);

	return snprintf(buf, PAGE_SIZE, "%u\n", data->freq);
}

static ssize_t sx9320_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t sx9320_name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", MODEL_NAME);
}

static ssize_t sx9320_touch_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "1\n");
}

static ssize_t sx9320_raw_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	static int sum_diff;
	struct sx9320_p *data = dev_get_drvdata(dev);

	sx9320_get_data(data);
	if (data->diff_cnt == 0)
		sum_diff = data->diff;
	else
		sum_diff += data->diff;

	if (++data->diff_cnt >= DIFF_READ_NUM) {
		data->diff_avg = sum_diff / DIFF_READ_NUM;
		data->useful_avg = sum_diff / DIFF_READ_NUM;
		data->diff_cnt = 0;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%u,%d,%d\n", data->capmain,
		data->useful, data->offset, data->diff, data->avg);
}

static ssize_t sx9320_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	/* It's for init touch */
	return snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t sx9320_threshold_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t sx9320_normal_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);
	u16 thresh_temp = 0, hysteresis = 0;

	thresh_temp = data->normal_th;
	thresh_temp = thresh_temp * thresh_temp / 2;

	hysteresis = data->hyst;

	switch (hysteresis) {
	case 0x01: /* 6% */
		hysteresis = thresh_temp >> 4;
		break;
	case 0x02: /* 12% */
		hysteresis = thresh_temp >> 3;
		break;
	case 0x03: /* 25% */
		hysteresis = thresh_temp >> 2;
		break;
	default:
		/* None */
		break;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d\n", thresh_temp + hysteresis,
			thresh_temp - hysteresis);
}

static ssize_t sx9320_normal_threshold_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	struct sx9320_p *data = dev_get_drvdata(dev);

	/* It's for normal touch */
	if (kstrtoul(buf, 10, &val)) {
		pr_err("[SX9320]: %s - Invalid Argument\n", __func__);
		return count;
	}

	sx9320_i2c_write(data, SX9320_PROXCTRL7_REG, val);

	pr_info("[SX9320]: %s - normal threshold %lu\n", __func__, val);

	data->normal_th_buf = data->normal_th = (u8)(val);

	return count;
}

static ssize_t sx9320_onoff_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", !data->skip_data);
}

static ssize_t sx9320_onoff_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u8 val;
	int ret;
	struct sx9320_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &val);
	if (ret) {
		pr_err("[SX9320]: %s - Invalid Argument\n", __func__);
		return count;
	}

	if (val == 0) {
		data->skip_data = true;
		if (atomic_read(&data->enable) == ON) {
			data->state = IDLE;
			input_report_rel(data->input, REL_MISC, 2);
			input_sync(data->input);
		}
	} else {
		data->skip_data = false;
	}

	pr_info("[SX9320]: %s -%u\n", __func__, val);
	return count;
}

static ssize_t sx9320_calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "2,0,0\n");
}

static ssize_t sx9320_calibration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t sx9320_gain_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	struct sx9320_p *data = dev_get_drvdata(dev);

	switch (data->gain) {
	case 0x01:
		ret = snprintf(buf, PAGE_SIZE, "x1\n");
		break;
	case 0x02:
		ret = snprintf(buf, PAGE_SIZE, "x2\n");
		break;
	case 0x03:
		ret = snprintf(buf, PAGE_SIZE, "x4\n");
		break;
	case 0x04:
		ret = snprintf(buf, PAGE_SIZE, "x8\n");
		break;
	default:
		ret = snprintf(buf, PAGE_SIZE, "Reserved\n");
		break;
	}

	return ret;
}

static ssize_t sx9320_range_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;

	struct sx9320_p *data = dev_get_drvdata(dev);

	switch (data->range) {
	case 0x00:
		ret = snprintf(buf, PAGE_SIZE, "Small\n");
		break;
	case 0x01:
		ret = snprintf(buf, PAGE_SIZE, "Large\n");
		break;
	default:
		ret = snprintf(buf, PAGE_SIZE, "Small\n");
		break;
	}

	return ret;
}

static ssize_t sx9320_diff_avg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->diff_avg);
}

static ssize_t sx9320_useful_avg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->useful_avg);
}

static ssize_t sx9320_ch_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	struct sx9320_p *data = dev_get_drvdata(dev);

	if (data->skip_data == true) {
		ret = snprintf(buf, PAGE_SIZE, "%d,%d\n",
			NONE_ENABLE, NONE_ENABLE);
	} else if (atomic_read(&data->enable) == ON) {
		ret = snprintf(buf, PAGE_SIZE, "%d,%d\n",
			data->ch1_state, data->ch2_state);
	} else {
		ret = snprintf(buf, PAGE_SIZE, "%d,%d\n",
			NONE_ENABLE, NONE_ENABLE);
	}

	return ret;
}

static ssize_t sx9320_grip_flush_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sx9320_p *data = dev_get_drvdata(dev);
	int ret = 0;
	u8 handle = 0;

	ret = kstrtou8(buf, 10, &handle);
	if (ret < 0) {
		pr_err("%s - kstrtou8 failed.(%d)\n", __func__, ret);
		return count;
	}

	pr_info("[SX9320]: %s - handle = %d\n", __func__, handle);

	input_report_rel(data->input, REL_MAX, handle);
	input_sync(data->input);
	return count;
}

static ssize_t sx9320_avgnegfilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);
	int avgnegfilt = data->avgnegfilt;

	if (avgnegfilt == 7)
		return snprintf(buf, PAGE_SIZE, "1\n");
	else if (avgnegfilt > 0 && data->avgnegfilt < 7)
		return snprintf(buf, PAGE_SIZE, "1-1/%d\n", 1 << avgnegfilt);
	else if (avgnegfilt == 0)
		return snprintf(buf, PAGE_SIZE, "0\n");

	return snprintf(buf, PAGE_SIZE, "not set\n");
}

static ssize_t sx9320_avgposfilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	switch (data->avgposfilt) {
	case 0x00:
		return snprintf(buf, PAGE_SIZE, "0\n");
	case 0x01:
		return snprintf(buf, PAGE_SIZE, "1-1/16\n");
	case 0x02:
		return snprintf(buf, PAGE_SIZE, "1-1/64\n");
	case 0x03:
		return snprintf(buf, PAGE_SIZE, "1-1/128\n");
	case 0x04:
		return snprintf(buf, PAGE_SIZE, "1-1/256\n");
	case 0x05:
		return snprintf(buf, PAGE_SIZE, "1-1/512\n");
	case 0x06:
		return snprintf(buf, PAGE_SIZE, "1-1/1024\n");
	case 0x07:
		return snprintf(buf, PAGE_SIZE, "1\n");
	default:
		break;
	}
	return snprintf(buf, PAGE_SIZE, "not set\n");
}

static ssize_t sx9320_avgthresh_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", 512 * data->avgthresh);
}

static ssize_t sx9320_rawfilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);
	int rawfilt = data->rawfilt;

	if (rawfilt > 0 && rawfilt < 8)
		return snprintf(buf, PAGE_SIZE, "1-1/%d\n", 1 << rawfilt);
	else if (rawfilt == 0)
		return snprintf(buf, PAGE_SIZE, "0\n");

	return snprintf(buf, PAGE_SIZE, "not set\n");
}

static ssize_t sx9320_sampling_freq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);
	int sampling_freq = data->sampling_freq;

	const char *table[32] = {
		"250", "200", "166.67", "142.86", "125", "111.11", "100",
		"90.91", "83.33", "76.92", "71.43", "66.67", "62.50", "58.82",
		"55.56", "52.63", "50", "45.45", "41.67", "38.46", "35.71",
		"31.25", "27.78", "25", "20.83", "17.86", "13.89", "11.36",
		"8.33", "6.58", "5.43", "4.63"};

	if (sampling_freq < 0 || sampling_freq > 31)
		return snprintf(buf, PAGE_SIZE, "not set\n");

	return snprintf(buf, PAGE_SIZE, "%skHz\n", table[sampling_freq]);
}

static ssize_t sx9320_scan_period_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);
	int scan_period = data->scan_period;

	const char *table[30] = {
		"Min", "2ms", "4ms", "6ms", "8ms", "10ms", "14ms", "18ms",
		"22ms", "26ms", "30ms", "34ms", "38ms", "42ms", "46ms", "50ms",
		"56ms", "62ms", "68ms", "74ms", "80ms", "90ms", "100ms",
		"200ms", "300ms", "400ms", "600ms", "800ms", "1s", "2s"};

	if (scan_period < 0 || scan_period > 29)
		return snprintf(buf, PAGE_SIZE, "not set\n");

	return snprintf(buf, PAGE_SIZE, "%s\n", table[scan_period]);
}

static ssize_t sx9320_again_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	switch (data->again_ch) {
	case 1247:
		return snprintf(buf, PAGE_SIZE, "x1.247\n");
	case 1000:
		return snprintf(buf, PAGE_SIZE, "x1\n");
	case 768:
		return snprintf(buf, PAGE_SIZE, "x0.768\n");
	case 552:
		return snprintf(buf, PAGE_SIZE, "x0.552\n");
	default:
		return snprintf(buf, PAGE_SIZE, "not set\n");
	}
}

static ssize_t sx9320_phase_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->phen);
}

static ssize_t sx9320_hysteresis_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	const char *table[4] = {"None", "+/-6%", "+/-12%", "+/-25%"};
	int hyst = data->hyst;

	if (hyst < 0 || hyst > 3)
		return snprintf(buf, PAGE_SIZE, "not set\n");

	return snprintf(buf, PAGE_SIZE, "%s\n", table[hyst]);
}

static ssize_t sx9320_irq_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	int result = 0;
	s32 max_diff_val = 0;

	if (data->irq_count) {
		result = -1;
		max_diff_val = data->max_diff;
	} else {
		max_diff_val = data->max_normal_diff;
	}

	pr_info("[SX9320]: %s - called\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		result, data->irq_count, max_diff_val);
}

static ssize_t sx9320_irq_count_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	u8 onoff;
	int ret;

	ret = kstrtou8(buf, 10, &onoff);
	if (ret < 0) {
		pr_err("[SX9320]: %s - kstrtou8 failed.(%d)\n", __func__, ret);
		return count;
	}

	mutex_lock(&data->read_mutex);

	if (onoff == 0) {
		data->abnormal_mode = OFF;
	} else if (onoff == 1) {
		data->abnormal_mode = ON;
		data->irq_count = 0;
		data->max_diff = 0;
		data->max_normal_diff = 0;
	} else {
		pr_err("[SX9320]: %s - unknown value %d\n", __func__, onoff);
	}

	mutex_unlock(&data->read_mutex);

	pr_info("[SX9320]: %s - %d\n", __func__, onoff);

	return count;
}

static DEVICE_ATTR(menual_calibrate, 0664,
		sx9320_get_offset_calibration_show,
		sx9320_set_offset_calibration_store);
static DEVICE_ATTR(register_write, 0220, NULL, sx9320_register_write_store);
static DEVICE_ATTR(register_read, 0444,	sx9320_register_read_show, NULL);
static DEVICE_ATTR(readback, 0444, sx9320_read_data_show, NULL);
static DEVICE_ATTR(reset, 0444, sx9320_sw_reset_show, NULL);
static DEVICE_ATTR(name, 0444, sx9320_name_show, NULL);
static DEVICE_ATTR(vendor, 0444, sx9320_vendor_show, NULL);
static DEVICE_ATTR(mode, 0444, sx9320_touch_mode_show, NULL);
static DEVICE_ATTR(raw_data, 0444, sx9320_raw_data_show, NULL);
static DEVICE_ATTR(diff_avg, 0444, sx9320_diff_avg_show, NULL);
static DEVICE_ATTR(useful_avg, 0444, sx9320_useful_avg_show, NULL);
static DEVICE_ATTR(calibration, 0664,
		sx9320_calibration_show, sx9320_calibration_store);
static DEVICE_ATTR(onoff, 0664,
		sx9320_onoff_show, sx9320_onoff_store);
static DEVICE_ATTR(threshold, 0664,
		sx9320_threshold_show, sx9320_threshold_store);
static DEVICE_ATTR(normal_threshold, 0664,
		sx9320_normal_threshold_show, sx9320_normal_threshold_store);
static DEVICE_ATTR(freq, 0664,
		sx9320_freq_show, sx9320_freq_store);
static DEVICE_ATTR(ch_state, 0444, sx9320_ch_state_show, NULL);
static DEVICE_ATTR(avg_negfilt, 0444, sx9320_avgnegfilt_show, NULL);
static DEVICE_ATTR(avg_posfilt, 0444, sx9320_avgposfilt_show, NULL);
static DEVICE_ATTR(avg_thresh, 0444, sx9320_avgthresh_show, NULL);
static DEVICE_ATTR(rawfilt, 0444, sx9320_rawfilt_show, NULL);
static DEVICE_ATTR(sampling_freq, 0444, sx9320_sampling_freq_show, NULL);
static DEVICE_ATTR(scan_period, 0444, sx9320_scan_period_show, NULL);
static DEVICE_ATTR(gain, 0444, sx9320_gain_show, NULL);
static DEVICE_ATTR(range, 0444, sx9320_range_show, NULL);
static DEVICE_ATTR(analog_gain, 0444, sx9320_again_show, NULL);
static DEVICE_ATTR(phase, 0444, sx9320_phase_show, NULL);
static DEVICE_ATTR(hysteresis, 0444, sx9320_hysteresis_show, NULL);
static DEVICE_ATTR(irq_count, 0664,
		sx9320_irq_count_show, sx9320_irq_count_store);

static DEVICE_ATTR(grip_flush, 0220, NULL, sx9320_grip_flush_store);

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_menual_calibrate,
	&dev_attr_register_write,
	&dev_attr_register_read,
	&dev_attr_readback,
	&dev_attr_reset,
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_mode,
	&dev_attr_diff_avg,
	&dev_attr_useful_avg,
	&dev_attr_raw_data,
	&dev_attr_threshold,
	&dev_attr_normal_threshold,
	&dev_attr_onoff,
	&dev_attr_calibration,
	&dev_attr_freq,
	&dev_attr_ch_state,
	&dev_attr_grip_flush,
	&dev_attr_avg_negfilt,
	&dev_attr_avg_posfilt,
	&dev_attr_avg_thresh,
	&dev_attr_rawfilt,
	&dev_attr_sampling_freq,
	&dev_attr_scan_period,
	&dev_attr_gain,
	&dev_attr_range,
	&dev_attr_analog_gain,
	&dev_attr_phase,
	&dev_attr_hysteresis,
	&dev_attr_irq_count,
	NULL,
};

static ssize_t sx9320_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u8 enable;
	int ret;
	struct sx9320_p *data = dev_get_drvdata(dev);
	int pre_enable = atomic_read(&data->enable);

	ret = kstrtou8(buf, 2, &enable);
	if (ret) {
		pr_err("[SX9320]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	pr_info("[SX9320]: %s - new_value = %u old_value = %d\n",
		__func__, enable, pre_enable);

	if (pre_enable == enable)
		return size;

	atomic_set(&data->enable, enable);
	sx9320_set_enable(data, enable);

	return size;
}

static ssize_t sx9320_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&data->enable));
}

static ssize_t sx9320_flush_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u8 enable;
	int ret;
	struct sx9320_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &enable);
	if (ret) {
		pr_err("[SX9320]: %s - Invalid Argument\n", __func__);
		return count;
	}

	if (enable == 1) {
		input_report_rel(data->input, REL_MAX, 1);
		input_sync(data->input);
	}

	return count;
}

static DEVICE_ATTR(enable, 0664,
		sx9320_enable_show, sx9320_enable_store);
static DEVICE_ATTR(flush, 0220,
		NULL, sx9320_flush_store);

static struct attribute *sx9320_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_flush.attr,
	NULL
};

static struct attribute_group sx9320_attribute_group = {
	.attrs = sx9320_attributes
};

static void sx9320_touch_process(struct sx9320_p *data, u8 flag)
{
	u8 status = 0;

	sx9320_i2c_read(data, SX9320_STAT0_REG, &status);
	pr_info("[SX9320]: %s - (status: 0x%x)\n", __func__, status);

	sx9320_get_data(data);

	if (data->abnormal_mode) {
		if (status) {
			if (data->max_diff < data->diff)
				data->max_diff = data->diff;
			data->irq_count++;
		}
	}

	sx9320_read_ch_interrupt(data, status);

	if (data->state == IDLE) {
		if (status & (PHX_STATUS_REG << data->phen))
			sx9320_send_event(data, ACTIVE);
		else
			pr_info("[SX9320]: %s - already released\n",
				__func__);
	} else {
		if (!(status & (PHX_STATUS_REG << data->phen)))
			sx9320_send_event(data, IDLE);
		else
			pr_info("[SX9320]: %s - still touched\n",
				__func__);
	}
}

static void sx9320_process_interrupt(struct sx9320_p *data)
{
	u8 flag = 0;

	/* since we are not in an interrupt don't need to disable irq. */
	flag = sx9320_read_irqstate(data);

	if (flag & IRQ_PROCESS_CONDITION)
		sx9320_touch_process(data, flag);
	else
		pr_info("[SX9320]: %s interrupt generated but skip\n",
			__func__);
}

static void sx9320_init_work_func(struct work_struct *work)
{
	struct sx9320_p *data = container_of((struct delayed_work *)work,
		struct sx9320_p, init_work);

	sx9320_initialize_chip(data);

	/* set the threshold at init. register depends on phase */
	if (data->phen < 2)
		sx9320_i2c_write(data, SX9320_PROXCTRL6_REG, data->normal_th);
	else
		sx9320_i2c_write(data, SX9320_PROXCTRL7_REG, data->normal_th);

	sx9320_set_mode(data, SX9320_MODE_NORMAL);
	/*
	 * make sure no interrupts are pending since enabling irq
	 * will only work on next falling edge
	 */
	sx9320_read_irqstate(data);
}

static void sx9320_irq_work_func(struct work_struct *work)
{
	struct sx9320_p *data = container_of((struct delayed_work *)work,
		struct sx9320_p, irq_work);

	if (sx9320_get_nirq_state(data) == 0)
		sx9320_process_interrupt(data);
	else
		pr_err("[SX9320]: %s - nirq read high %d\n",
			__func__, sx9320_get_nirq_state(data));
}

static void sx9320_debug_work_func(struct work_struct *work)
{
	struct sx9320_p *data = container_of((struct delayed_work *)work,
		struct sx9320_p, debug_work);

	static int hall_flag = 1;

	sx9320_check_hallic_state(HALLIC1_PATH, data->hall_ic1);

	/* Hall IC closed : offset cal (once) */
	if (strcmp(data->hall_ic1, "CLOSE") == 0) {
		if (hall_flag) {
			pr_info("[SX9320]: %s - hall IC1 is closed\n",
				__func__);
			sx9320_set_offset_calibration(data);
			hall_flag = 0;
		}
	} else
		hall_flag = 1;

	if (atomic_read(&data->enable) == ON) {
		if (data->abnormal_mode) {
			sx9320_get_data(data);
			if (data->max_normal_diff < data->diff)
				data->max_normal_diff = data->diff;
		} else {
			if (data->debug_count >= GRIP_LOG_TIME) {
				sx9320_get_data(data);
				data->debug_count = 0;
			} else {
				data->debug_count++;
			}
		}
	}

	schedule_delayed_work_on(1, &data->debug_work, msecs_to_jiffies(2000));
}

static irqreturn_t sx9320_interrupt_thread(int irq, void *pdata)
{
	struct sx9320_p *data = pdata;

	if (sx9320_get_nirq_state(data) == 1) {
		pr_err("[SX9320]: %s - nirq read high\n", __func__);
	} else {
		wake_lock_timeout(&data->grip_wake_lock, 3 * HZ);
		schedule_delayed_work(&data->irq_work, msecs_to_jiffies(100));
	}

	return IRQ_HANDLED;
}

static int sx9320_init_input(struct sx9320_p *data)
{
	int ret = 0;

	/* Create the input device */
	data->input = input_allocate_device();

	if (!data->input)
		return -ENOMEM;

	data->input->name = MODULE_NAME;
	data->input->id.bustype = BUS_I2C;

	input_set_capability(data->input, EV_REL, REL_MISC);
	input_set_capability(data->input, EV_REL, REL_MAX);
	input_set_drvdata(data->input, data);

	ret = input_register_device(data->input);
	if (ret < 0) {
		input_free_device(data->input);
		return ret;
	}

	ret = sensors_create_symlink(data->input);
	if (ret < 0) {
		input_unregister_device(data->input);
		return ret;
	}

	ret = sysfs_create_group(&data->input->dev.kobj,
		&sx9320_attribute_group);
	if (ret < 0) {
		sensors_remove_symlink(data->input);
		input_unregister_device(data->input);
		return ret;
	}

	return 0;
}

static int sx9320_setup_pin(struct sx9320_p *data)
{
	int ret;

	ret = gpio_request(data->gpio_nirq, "SX9320_nIRQ");
	if (ret < 0) {
		pr_err("[SX9320]: %s - gpio %d request failed (%d)\n",
			__func__, data->gpio_nirq, ret);
		return ret;
	}

	ret = gpio_direction_input(data->gpio_nirq);
	if (ret < 0) {
		pr_err("[SX9320]: %s - failed to set gpio %d(%d)\n",
			__func__, data->gpio_nirq, ret);
		gpio_free(data->gpio_nirq);
		return ret;
	}

	return 0;
}

static void sx9320_set_specific_register(int regi_num, int end, int start,
		u8 val)
{
	u16 clear_bit = 0x00;
	unsigned char temp_val;

	temp_val = setup_reg[regi_num].val;

	clear_bit = ~((1 << (end + 1)) - (1 << start));

	temp_val = (temp_val & clear_bit) | (val << start);

	setup_reg[regi_num].val = temp_val;
}

static void sx9320_initialize_variable(struct sx9320_p *data)
{
	data->state = IDLE;
	data->skip_data = false;
	data->check_usb = false;
	data->debug_count = 0;
	data->normal_th_buf = data->normal_th;
	data->ch1_state = IDLE;
	data->init_done = OFF;

	atomic_set(&data->enable, OFF);

	/* sampling freq, rawfilt, gain, hyst are depends on phase */
	if (data->phen < 2) {
		/* regi_num, end bit, start bit, val */
		sx9320_set_specific_register(5,  7, 3, data->sampling_freq);
		sx9320_set_specific_register(5,  2, 0, data->resolution);
		sx9320_set_specific_register(12, 2, 0, data->rawfilt);
		sx9320_set_specific_register(12, 5, 3, data->gain);
	} else {
		sx9320_set_specific_register(7,  7, 3, data->sampling_freq);
		sx9320_set_specific_register(7,  2, 0, data->resolution);
		sx9320_set_specific_register(13, 2, 0, data->rawfilt);
		sx9320_set_specific_register(13, 5, 3, data->gain);
	}
	sx9320_set_specific_register(0,  4, 0, data->scan_period);
	sx9320_set_specific_register(43, 3, 0, data->again);
	sx9320_set_specific_register(14, 5, 0, data->avgthresh);
	sx9320_set_specific_register(16, 2, 0, data->avgposfilt);
	sx9320_set_specific_register(16, 5, 3, data->avgnegfilt);
	sx9320_set_specific_register(17, 3, 2, data->debouncer);
	sx9320_set_specific_register(17, 5, 4, data->hyst);
	sx9320_set_specific_register(8, 5, 0, data->afeph0);
	sx9320_set_specific_register(9, 5, 0, data->afeph1);
	sx9320_set_specific_register(10, 5, 0, data->afeph2);
	sx9320_set_specific_register(11, 5, 0, data->afeph3);

	sx9320_get_again(data);
}

static int sx9320_read_setupreg(struct device_node *dnode, char *str, u8 *val)
{
	u32 temp_val;
	int ret;

	ret = of_property_read_u32(dnode, str, &temp_val);

	if (!ret)
		*val = (u8)temp_val;
	else
		pr_err("[SX9320]: %s - %s: property read err 0x%2x (%d)\n",
			__func__, str, temp_val, ret);

	return ret;
}

static int sx9320_parse_dt(struct sx9320_p *data, struct device *dev)
{
	struct device_node *node = dev->of_node;
	enum of_gpio_flags flags;

	if (node == NULL)
		return -ENODEV;

	data->gpio_nirq = of_get_named_gpio_flags(node,
		"sx9320,nirq-gpio", 0, &flags);
	if (data->gpio_nirq < 0) {
		pr_err("[SX9320]: %s - get gpio_nirq error\n", __func__);
		return -ENODEV;
	}

	sx9320_read_setupreg(node, SX9320_PHEN, &data->phen);
	sx9320_read_setupreg(node, SX9320_GAIN, &data->gain);
	sx9320_read_setupreg(node, SX9320_AGAIN, &data->again);
	sx9320_read_setupreg(node, SX9320_SCANPERIOD, &data->scan_period);
	sx9320_read_setupreg(node, SX9320_RANGE, &data->range);
	sx9320_read_setupreg(node, SX9320_SAMPLING_FREQ, &data->sampling_freq);
	sx9320_read_setupreg(node, SX9320_RESOLUTION, &data->resolution);
	sx9320_read_setupreg(node, SX9320_RAWFILT, &data->rawfilt);
	sx9320_read_setupreg(node, SX9320_HYST, &data->hyst);
	sx9320_read_setupreg(node, SX9320_AVGPOSFILT, &data->avgposfilt);
	sx9320_read_setupreg(node, SX9320_AVGNEGFILT, &data->avgnegfilt);
	sx9320_read_setupreg(node, SX9320_AVGTHRESH, &data->avgthresh);
	sx9320_read_setupreg(node, SX9320_DEBOUNCER, &data->debouncer);
	sx9320_read_setupreg(node, SX9320_NORMALTHD, &data->normal_th);

	if (sx9320_read_setupreg(node, SX9320_AFEPH0, &data->afeph0))
		data->afeph0 = setup_reg[8].val;
	if (sx9320_read_setupreg(node, SX9320_AFEPH1, &data->afeph1))
		data->afeph1 = setup_reg[9].val;
	if (sx9320_read_setupreg(node, SX9320_AFEPH2, &data->afeph2))
		data->afeph2 = setup_reg[10].val;
	if (sx9320_read_setupreg(node, SX9320_AFEPH3, &data->afeph3))
		data->afeph3 = setup_reg[11].val;

	pr_info("[SX9320]: afeph0:%x, afeph1:%x, afeph2:%x, afeph3:%x\n",
		data->afeph0, data->afeph1, data->afeph2, data->afeph3);

	return 0;
}
#if defined(CONFIG_CCIC_NOTIFIER) && defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
static int sx9320_ccic_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	CC_NOTI_USB_STATUS_TYPEDEF usb_status =
		*(CC_NOTI_USB_STATUS_TYPEDEF *)data;

	struct sx9320_p *pdata =
		container_of(nb, struct sx9320_p, cpuidle_ccic_nb);
	static int pre_attach;

	if (pre_attach == usb_status.attach)
		return 0;
	/*
	 * USB_STATUS_NOTIFY_DETACH = 0,
	 * USB_STATUS_NOTIFY_ATTACH_DFP = 1, // Host
	 * USB_STATUS_NOTIFY_ATTACH_UFP = 2, // Device
	 * USB_STATUS_NOTIFY_ATTACH_DRP = 3, // Dual role
	 */

	if (pdata->init_done == ON) {
		switch (usb_status.drp) {
		case USB_STATUS_NOTIFY_ATTACH_UFP:
		case USB_STATUS_NOTIFY_ATTACH_DFP:
		case USB_STATUS_NOTIFY_DETACH:
			pr_info("[SX9320]: %s - drp = %d attat = %d\n",
				__func__, usb_status.drp,
				usb_status.attach);
			sx9320_set_offset_calibration(pdata);
			break;
		default:
			pr_info("[SX9320]: %s - DRP type : %d\n",
				__func__, usb_status.drp);
			break;
		}
	}

	pre_attach = usb_status.attach;

	return 0;
}
#elif defined(CONFIG_MUIC_NOTIFIER)
static int sx9320_cpuidle_muic_notifier(struct notifier_block *nb,
				unsigned long action, void *data)
{
	struct sx9320_p *grip_data;

	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;

	grip_data = container_of(nb, struct sx9320_p, cpuidle_muic_nb);
	switch (attached_dev) {
	case ATTACHED_DEV_OTG_MUIC:
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH)
			pr_info("[SX9320]: %s - TA/USB insert\n", __func__);
		else if (action == MUIC_NOTIFY_CMD_DETACH)
			pr_info("[SX9320]: %s - TA/USB remove\n", __func__);

		if (grip_data->init_done == ON)
			sx9320_set_offset_calibration(grip_data);
		else
			pr_info("[SX9320]: %s - not initialized\n", __func__);
		break;
	default:
		break;
	}

	pr_info("[SX9320]: %s dev=%d, action=%lu\n",
		__func__, attached_dev, action);

	return NOTIFY_DONE;
}
#endif

static int sx9320_check_chip_id(struct sx9320_p *data)
{
	int ret;
	unsigned char value = 0;

	ret = sx9320_i2c_read(data, SX9320_WHOAMI_REG, &value);
	if (ret < 0) {
		pr_err("[SX9320]: whoami[0x%x] read failed %d\n", value, ret);
		return ret;
	}

	switch (value) {
	case 0x20:
		return 0;
	case 0x21:
		return 0;
	default:
		pr_err("[SX9320]: invalid whoami(%x)\n", value);
		return -1;
	}
}

static int sx9320_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct sx9320_p *data = NULL;

	pr_info("[SX9320]: %s - Probe Start!\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[SX9320]: %s - i2c_check_functionality error\n",
			__func__);
		goto exit;
	}

	/* create memory for main struct */
	data = kzalloc(sizeof(struct sx9320_p), GFP_KERNEL);

	i2c_set_clientdata(client, data);
	data->client = client;
	data->factory_device = &client->dev;

	ret = sx9320_init_input(data);
	if (ret < 0)
		goto exit_input_init;

	wake_lock_init(&data->grip_wake_lock,
		WAKE_LOCK_SUSPEND, "grip_wake_lock");
	mutex_init(&data->read_mutex);

	ret = sx9320_parse_dt(data, &client->dev);
	if (ret < 0) {
		pr_err("[SX9320]: %s - of_node error\n", __func__);
		ret = -ENODEV;
		goto exit_of_node;
	}

	ret = sx9320_setup_pin(data);
	if (ret) {
		pr_err("[SX9320]: %s - could not setup pin\n", __func__);
		goto exit_setup_pin;
	}

	/* read chip id */
	ret = sx9320_check_chip_id(data);
	if (ret < 0) {
		pr_err("[SX9320]: %s - chip id check failed %d\n", __func__, ret);
		goto exit_chip_reset;
	}

	ret = sx9320_i2c_write(data, SX9320_SOFTRESET_REG, SX9320_SOFTRESET);
	if (ret < 0) {
		pr_err("[SX9320]: %s - reset failed %d\n", __func__, ret);
		goto exit_chip_reset;
	}

	sx9320_initialize_variable(data);
	INIT_DELAYED_WORK(&data->init_work, sx9320_init_work_func);
	INIT_DELAYED_WORK(&data->irq_work, sx9320_irq_work_func);
	INIT_DELAYED_WORK(&data->debug_work, sx9320_debug_work_func);

	data->irq = gpio_to_irq(data->gpio_nirq);
	ret = request_threaded_irq(data->irq, NULL, sx9320_interrupt_thread,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			"sx9320_irq", data);
	if (ret < 0) {
		pr_err("[SX9320]: %s - failed to set request irq %d(%d)\n",
			__func__, data->irq, ret);
		goto exit_request_threaded_irq;
	}
	disable_irq(data->irq);

	ret = sensors_register(data->factory_device,
		data, sensor_attrs, MODULE_NAME);
	if (ret) {
		pr_err("[SX9320] %s - cound not register sensor(%d).\n",
			__func__, ret);
		goto grip_sensor_register_failed;
	}

	schedule_delayed_work(&data->init_work, msecs_to_jiffies(300));
	sx9320_set_debug_work(data, ON, 20000);

#if defined(CONFIG_CCIC_NOTIFIER) && defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	manager_notifier_register(&data->cpuidle_ccic_nb,
					sx9320_ccic_handle_notification,
					MANAGER_NOTIFY_CCIC_SENSORHUB);
#elif defined(CONFIG_MUIC_NOTIFIER)
	muic_notifier_register(&data->cpuidle_muic_nb,
		sx9320_cpuidle_muic_notifier, MUIC_NOTIFY_DEV_CPUIDLE);
#endif

	pr_info("[SX9320]: %s - Probe done!\n", __func__);

	return 0;

grip_sensor_register_failed:
	free_irq(data->irq, data);
exit_request_threaded_irq:
exit_chip_reset:
	gpio_free(data->gpio_nirq);
exit_setup_pin:
exit_of_node:
	mutex_destroy(&data->read_mutex);
	wake_lock_destroy(&data->grip_wake_lock);
	sysfs_remove_group(&data->input->dev.kobj, &sx9320_attribute_group);
	sensors_remove_symlink(data->input);
	input_unregister_device(data->input);
exit_input_init:
	kfree(data);
exit:
	pr_err("[SX9320]: %s - Probe fail!\n", __func__);
	return ret;
}

static int sx9320_remove(struct i2c_client *client)
{
	struct sx9320_p *data = (struct sx9320_p *)i2c_get_clientdata(client);

	if (atomic_read(&data->enable) == ON)
		sx9320_set_enable(data, OFF);

	sx9320_set_mode(data, SX9320_MODE_SLEEP);

	cancel_delayed_work_sync(&data->init_work);
	cancel_delayed_work_sync(&data->irq_work);
	cancel_delayed_work_sync(&data->debug_work);
	free_irq(data->irq, data);
	gpio_free(data->gpio_nirq);

	wake_lock_destroy(&data->grip_wake_lock);
	sensors_unregister(data->factory_device, sensor_attrs);
	sensors_remove_symlink(data->input);
	sysfs_remove_group(&data->input->dev.kobj, &sx9320_attribute_group);
	input_unregister_device(data->input);
	mutex_destroy(&data->read_mutex);

	kfree(data);

	return 0;
}

static int sx9320_suspend(struct device *dev)
{
	struct sx9320_p *data = dev_get_drvdata(dev);
	int cnt = 0;

	pr_info("[SX9320]: %s\n", __func__);
	/* before go to sleep, make the interrupt pin as high*/
	while ((sx9320_get_nirq_state(data) == 0) && (cnt++ < 3)) {
		sx9320_read_irqstate(data);
		msleep(20);
	}
	if (cnt >= 3)
		pr_err("[SX9320]: %s - s/w reset fail(%d)\n", __func__, cnt);

	sx9320_set_debug_work(data, OFF, 1000);

	return 0;
}

static int sx9320_resume(struct device *dev)
{
	struct sx9320_p *data = dev_get_drvdata(dev);

	pr_info("[SX9320]: %s\n", __func__);
	sx9320_set_debug_work(data, ON, 1000);

	return 0;
}

static void sx9320_shutdown(struct i2c_client *client)
{
	struct sx9320_p *data = i2c_get_clientdata(client);

	pr_info("[SX9320]: %s\n", __func__);
	sx9320_set_debug_work(data, OFF, 1000);
	if (atomic_read(&data->enable) == ON)
		sx9320_set_enable(data, OFF);
	sx9320_set_mode(data, SX9320_MODE_SLEEP);
}

static const struct of_device_id sx9320_match_table[] = {
	{ .compatible = "sx9320",},
	{},
};

static const struct i2c_device_id sx9320_id[] = {
	{ "sx9320_match_table", 0 },
	{ }
};

static const struct dev_pm_ops sx9320_pm_ops = {
	.suspend = sx9320_suspend,
	.resume = sx9320_resume,
};

static struct i2c_driver sx9320_driver = {
	.driver = {
		.name	= MODEL_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sx9320_match_table,
		.pm = &sx9320_pm_ops
	},
	.probe		= sx9320_probe,
	.remove		= sx9320_remove,
	.shutdown	= sx9320_shutdown,
	.id_table	= sx9320_id,
};

static int __init sx9320_init(void)
{
	return i2c_add_driver(&sx9320_driver);
}

static void __exit sx9320_exit(void)
{
	i2c_del_driver(&sx9320_driver);
}

module_init(sx9320_init);
module_exit(sx9320_exit);

MODULE_DESCRIPTION("Semtech Corp. SX9320 Capacitive Touch Controller Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
