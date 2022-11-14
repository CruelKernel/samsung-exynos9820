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
#include "sx9330_reg.h"
#ifdef CONFIG_CCIC_NOTIFIER
#include <linux/ccic/ccic_notifier.h>
#endif
#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/manager/usb_typec_manager_notifier.h>
#endif

#ifdef TAG
#undef TAG
#define TAG "[GRIP]"
#endif
#define VENDOR_NAME              "SEMTECH"
#define MODEL_NAME               "SX9330"
#define MODULE_NAME              "grip_sensor"
#define NOTI_MODULE_NAME         "grip_notifier"

#define I2C_M_WR                 0 /* for i2c Write */
#define I2c_M_RD                 1 /* for i2c Read */

#define IDLE                     0
#define ACTIVE                   1

#define SX9330_MODE_SLEEP        0
#define SX9330_MODE_NORMAL       1

#define MAIN_SENSOR              1
#define REF_SENSOR               2

#define DIFF_READ_NUM            10
#define GRIP_LOG_TIME            15 /* 30 sec */

/* PH0, PH1, PH2, PH3 */
#define TOTAL_BOTTON_COUNT       1
#define CSX_STATUS_REG           SX9330_STAT0_PROXSTAT_PH0_FLAG

#define IRQ_PROCESS_CONDITION   (MSK_IRQSTAT_TOUCH	\
				| MSK_IRQSTAT_RELEASE	\
				| MSK_IRQSTAT_COMP)

#define UNKNOWN_ON  1
#define UNKNOWN_OFF 2

#define TYPE_USB   1
#define TYPE_HALL  2
#define TYPE_BOOT  3
#define TYPE_FORCE 4

#define HALLIC_PATH		"/sys/class/sec/hall_ic/hall_detect"

struct sx9330_p {
	struct i2c_client *client;
	struct input_dev *input;
	struct input_dev *noti_input_dev;
	struct device *factory_device;
	struct delayed_work init_work;
	struct delayed_work irq_work;
	struct delayed_work debug_work;
	struct wake_lock grip_wake_lock;
	struct mutex read_mutex;
#if defined(CONFIG_CCIC_NOTIFIER)
	struct notifier_block cpuidle_ccic_nb;
#endif

	bool skip_data;
	u8 touchTh;
	u8 releaseTh;

	int irq;
	int gpio_nirq;
	int state;
	int init_done;

	atomic_t enable;


	u16 detect_threshold;
	u16 offset;
	s32 capMain;
	s32 useful;
	s32 avg;
	s32 diff;

	s32 diff_avg;
	int diff_cnt;
	s32 useful_avg;

	int irq_count;
	int abnormal_mode;
	s32 max_diff;
	s32 max_normal_diff;

	int debug_count;
	char hall_ic[6];

	int is_unknown_mode;
	int motion;
	bool first_working;

	int noti_enable;
	int pre_attach;
};

static int sx9330_check_hallic_state(char *file_path, char hall_ic_status[])
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *filep;
	char hall_sysfs[5];

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filep = filp_open(file_path, O_RDONLY, 0440);
	if (IS_ERR(filep)) {
		iRet = PTR_ERR(filep);
		pr_err("[SX9330]: %s - file open fail %d\n", __func__, iRet);
		set_fs(old_fs);
		return iRet;
	}

	iRet = filep->f_op->read(filep, hall_sysfs,
		sizeof(hall_sysfs), &filep->f_pos);

	if (iRet <= 0) {
		pr_err("[SX9330]: %s - file read fail %d\n", __func__, iRet);
		filp_close(filep, current->files);
		set_fs(old_fs);
		return -EIO;
	} else {
		strncpy(hall_ic_status, hall_sysfs, sizeof(hall_sysfs));
	}

	filp_close(filep, current->files);
	set_fs(old_fs);

	return iRet;
}

static int sx9330_get_nirq_state(struct sx9330_p *data)
{
	return gpio_get_value_cansleep(data->gpio_nirq);
}

static int sx9330_i2c_write_16bit(struct sx9330_p *data, u16 reg_addr, u32 buf)
{
	int ret;
	struct i2c_msg msg;
	unsigned char w_buf[6];

	w_buf[0] = (u8)(reg_addr>>8);
	w_buf[1] = (u8)(reg_addr);
	w_buf[2] = (u8)(buf>>24);
	w_buf[3] = (u8)(buf>>16);
	w_buf[4] = (u8)(buf>>8);
	w_buf[5] = (u8)(buf);

	msg.addr = data->client->addr;
	msg.flags = I2C_M_WR;
	msg.len = 6; // 2bytes regaddr + 4bytes data
	msg.buf = (u8 *)w_buf;

	ret = i2c_transfer(data->client->adapter, &msg, 1);
	if (ret < 0)
		pr_err("[SX9330]: %s - i2c write error %d\n", __func__, ret);

	return ret;
}

static int sx9330_i2c_read_16bit(struct sx9330_p *data, u16 reg_addr, u32 *data32)
{
	int ret;
	struct i2c_msg msg[2];
	u8 w_buf[2];
	u8 buf[4];

	w_buf[0] = (u8)(reg_addr>>8);
	w_buf[1] = (u8)(reg_addr);

	msg[0].addr = data->client->addr;
	msg[0].flags = I2C_M_WR;
	msg[0].len = 2;
	msg[0].buf = (u8 *)w_buf;

	msg[1].addr = data->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 4;
	msg[1].buf = (u8 *)buf;

	ret = i2c_transfer(data->client->adapter, msg, 2);
	if (ret < 0)
		pr_err("[SX9330]: %s - i2c read error %d\n", __func__, ret);
	
	data32[0] = ((u32)buf[0]<<24) | ((u32)buf[1]<<16) | ((u32)buf[2]<<8) | ((u32)buf[3]);
	
	return ret;
}

static u8 sx9330_read_irqstate(struct sx9330_p *data)
{
	u32 val;

	if (sx9330_i2c_read_16bit(data, SX9330_HOSTIRQSRC_REG, &val) >= 0)
		return (val & 0x000000FF);

	return 0;
}

static void sx9330_initialize_register(struct sx9330_p *data)
{
	u32 val32 = 0;
	u32 threshold = 0;
	int idx;

	for (idx = 1; idx < (int)(ARRAY_SIZE(setup_reg)); idx++) {
		sx9330_i2c_write_16bit(data, setup_reg[idx].reg, setup_reg[idx].val);
		pr_info("[SX9330]: %s - Write Reg: 0x%x Value: 0x%x\n",
			__func__, setup_reg[idx].reg, setup_reg[idx].val);

		sx9330_i2c_read_16bit(data, setup_reg[idx].reg, &val32);
		pr_info("[SX9330]: %s - Read Reg: 0x%x Value: 0x%x\n\n",
			__func__, setup_reg[idx].reg, val32);
	}

	sx9330_i2c_read_16bit(data, SX9330_ADCFILTPH0_REG +
		(1 << (4 + MAIN_SENSOR)), &threshold);

	threshold = (threshold & 0xFF00) >> 8;
	threshold = threshold * threshold / 2;

	data->detect_threshold = threshold;

	pr_info("[SX9330]: %s - detect threshold: %u\n", __func__, data->detect_threshold);
	data->init_done = ON;
}

static void sx9330_initialize_chip(struct sx9330_p *data)
{
	int cnt = 0;

	while((sx9330_get_nirq_state(data) == 0) && (cnt++ < 10)) {
		sx9330_read_irqstate(data);
		msleep(20);
	}

	if (cnt >= 10)
		pr_err("[SX9330]: %s - s/w reset fail(%d)\n", __func__, cnt);

	sx9330_initialize_register(data);
}

static int sx9330_set_offset_calibration(struct sx9330_p *data)
{
	int ret = 0;
	u32 retries = 0;
        u32 status = 0;

	pr_info("[SX9330]: %s\n", __func__);

	// Before Changing Mode ensure device is not processing a previous
	// This timing is very short but good practice to check.
	while (retries < 50) {
		ret = sx9330_i2c_read_16bit(data, SX9330_TOPSTAT0_REG, &status);
		if (ret < 0)
			return ret;

		// If this is 0, we can issue the next command
		if (!(status & MSK_TOPSTAT0_CMDBUSY)) {
			pr_info("[SX9330]: %s - TOPSTAT0 status: 0x%x, retries: %d\n", __func__, status, retries);
			break;
		}

		usleep_range(2*1000, 2*1000);
		retries++;
	}
	// send compensation
	ret = sx9330_i2c_write_16bit(data, SX9330_CMD_REG, I2C_REGCMD_COMPEN);
	if (ret < 0)
		return ret;
	// After sending compensation wait until CONVSTAT=0
	retries = 0;
	while (retries < 50) {
		ret = sx9330_i2c_read_16bit(data, SX9330_STAT1_REG, &status);
		if (ret < 0)
			return ret;	    

		if (!(status & MSK_STAT1_CONVSTAT)) {
			pr_info("[SX9330]: %s - STAT1 status: 0x%x, retries: %d\n", __func__, status, retries);
			break;
		}

		usleep_range(2*1000, 2*1000);
		retries++;
	}

	pr_info("[SX9330]: %s - done!\n", __func__);

	return ret;
}

static void sx9330_send_event(struct sx9330_p *data, u8 state)
{
	if (data->skip_data == true) {
		pr_info("[SX9330]: %s - skip grip event\n", __func__);
		return;
	}

	if (state == ACTIVE) {
		data->state = ACTIVE;
		pr_info("[SX9330]: %s - touched\n", __func__);
	} else {
		data->state = IDLE;
		pr_info("[SX9330]: %s - released\n", __func__);
	}

	if (state == ACTIVE)
		input_report_rel(data->input, REL_MISC, 1);
	else
		input_report_rel(data->input, REL_MISC, 2);

	input_report_rel(data->input, REL_X, data->is_unknown_mode);
	input_sync(data->input);
}

static void sx9330_display_data_reg(struct sx9330_p *data)
{
	u32 val; 
	u16 reg;
	int i;

	for (i = 0; i < TOTAL_BOTTON_COUNT; i++) {
		pr_info("[SX9330]: ############# %d button #############\n", i);
		for (reg = SX9330_USEPH0_REG; reg <= SX9330_DIFFPH7_REG; )
		{
			sx9330_i2c_read_16bit(data, reg, &val);
			pr_info("[SX9330]: %s - Register(0x%4x) data(0x%8x)\n",
				__func__, reg, (val>>10));
			reg += 32;
		}
	}
}

static void sx9330_get_data(struct sx9330_p *data)
{
	u8 msBit, msByte, lsByte;
	u32 uData = 0;
	u16 offset = 0;
	s32 capMain = 0, useful = 0;
	s32 avg = 0, diff = 0;
	u32 again = 0;
	s32 cOffset = 0;
	s64 cUseful = 0;
	const int table[16] = {
		0, 11000, 16500, 22000, 33000, 38500,
		44000, 49500, 55000, 60500, 66000,
		71500, 82500, 88000, 93500, 99000};

	mutex_lock(&data->read_mutex);

	/* Calculate out the Main Cap information */
	sx9330_i2c_read_16bit(data, SX9330_USEPH0_REG + MAIN_SENSOR*4, &uData);

	useful = (s32)uData >> 10;
	if (useful > 0xFFFFF)
		useful -= 0x1FFFFF;

	sx9330_i2c_read_16bit(data, SX9330_OFFSETPH0_REG + MAIN_SENSOR*8, &uData);

	offset = uData & 0x7FFF;

	msBit = (u8)((offset >> 14) & 0x1);
	msByte = (u8)((offset >> 7) & 0x7F);
	lsByte = (u8)((offset)      & 0x7F);

	/* read analog gain */
	sx9330_i2c_read_16bit(data, SX9330_AFEPARAMSPH0_REG + MAIN_SENSOR * 8,
		&again);
	again = (again & 0x1E00) >> 9;

	/* if again is 0 (Reserved), the useful value is not changed */
	if(again == 0)
		again = 1;

	cUseful = ((s64)useful * table[again]);
	cUseful = (s32)(cUseful / 1048575);
	cOffset = ((s32)msBit * 1060800) + ((s32)msByte * 22100) + 
		((s32)lsByte * 500);
	/* Ctotal = Coffset + Cuseful */
	capMain = cOffset + cUseful;

	/* Calculate out the Reference Cap information */
	sx9330_i2c_read_16bit(data, SX9330_USEPH0_REG + REF_SENSOR*4, &uData);

	/* avg read */
	sx9330_i2c_read_16bit(data, SX9330_AVGPH0_REG + MAIN_SENSOR*4, &uData);
	avg = (s32)uData >> 10;
	if (avg > 0xFFFFF)
		avg -= 0x1FFFFF;

	/* diff read */
	sx9330_i2c_read_16bit(data, SX9330_DIFFPH0_REG + MAIN_SENSOR*4, &uData); 
	diff = (s32)uData >> 10;
	if (diff > 0xFFFFF)
		diff -= 0x1FFFFF;

	data->useful = useful;
	data->offset = offset;
	data->capMain = capMain;
	data->avg = avg;
	data->diff = diff;

	mutex_unlock(&data->read_mutex);

	pr_info("[SX9330]: %s - CapsMain: %ld, useful: %ld, avg: %ld, diff: %ld, Offset: %u\n",
		__func__, (long int)data->capMain, (long int)data->useful, (long int)data->avg, (long int)data->diff, offset);
}

static int sx9330_set_mode(struct sx9330_p *data, unsigned char mode)
{
	int ret = -EINVAL;
	u32 retries = 0;
        u32 status = 0;

	// Before Changing Mode ensure device is not processing a previous
	// This timing is very short but good practice to check.
	while (retries < 50) {
		ret = sx9330_i2c_read_16bit(data, SX9330_TOPSTAT0_REG, &status);
		if (ret < 0) {
			pr_info("[SX9330]: %s - change the mode : %u FAILED reading TOPSTAT0!!\n", __func__, mode);
			return ret;
		}
		// If this is 0, we can issue the next command
		if (!(status & MSK_TOPSTAT0_CMDBUSY))
			break;

		usleep_range(2*1000, 2*1000);
		retries++;
	}

	if (mode == SX9330_MODE_SLEEP) {
		ret = sx9330_i2c_write_16bit(data, SX9330_CMD_REG,
			I2C_REGCMD_EN_SLEEP);
	} else if (mode == SX9330_MODE_NORMAL) {
		ret = sx9330_i2c_write_16bit(data, SX9330_CMD_REG,
			I2C_REGCMD_PHEN);
	}

	// After sending compensation wait until CONVSTAT=0
	retries = 0;
	while (retries < 50) {
		ret = sx9330_i2c_read_16bit(data, SX9330_STAT1_REG, &status);
		if (ret < 0) {
			pr_info("[SX9330]: %s - change the mode : %u FAILED reading STAT1!!\n", __func__, mode);
			return ret;	    
		}

		if (!(status & MSK_STAT1_CONVSTAT))
			break;

		usleep_range(2*1000, 2*1000);
		retries++;
	}

        // complete the original command that was sent after NORMAL mode change
	if (mode == SX9330_MODE_NORMAL)
		sx9330_set_offset_calibration(data);

	pr_info("[SX9330]: %s - change the mode : %u\n", __func__, mode);

	return ret;
}

static void sx9330_check_status(struct sx9330_p *data, int enable)
{

	u32 status = 0;

	sx9330_i2c_read_16bit(data, SX9330_STAT0_REG, &status);
	status = status >> 24;
	pr_info("[SX9330]: %s - (status: 0x%x)\n", __func__, status);

	if (data->skip_data == true) {
		input_report_rel(data->input, REL_MISC, 2);
		input_report_rel(data->input, REL_X, UNKNOWN_OFF);
		input_sync(data->input);
		return;
	}

	if (status & (CSX_STATUS_REG << MAIN_SENSOR)) {
		sx9330_send_event(data, ACTIVE);
	} else {
		sx9330_send_event(data, IDLE);
	}
}

static void sx9330_set_enable(struct sx9330_p *data, int enable)
{
	u8 status = 0;
	u32 val32 = 0;
	int idx;

	pr_info("[SX9330]: %s\n", __func__);

	if (enable) {

		pr_info("[SX9330]: %s - enable(status : 0x%x)\n",
			__func__, status);

		data->diff_avg = 0;
		data->diff_cnt = 0;
		data->useful_avg = 0;
		sx9330_get_data(data);
		sx9330_check_status(data, enable);

		/*
		 * for debugging error case
		 */
		if (data->capMain == 0 || data->offset == 0) {
			for (idx = 0; idx < (int)(ARRAY_SIZE(setup_reg)); idx++) {
				sx9330_i2c_read_16bit(data, setup_reg[idx].reg, &val32);
				pr_info("[SX9330]: %s - Read Reg: 0x%x Value: 0x%x\n",
					__func__, setup_reg[idx].reg, val32);
			}
		}

		msleep(20);

		/*
		 * make sure no interrupts are pending since enabling irq
		 * will only work on next falling edge
		 */
		sx9330_read_irqstate(data);
		/*
		 * enable interrupt
		 */
		sx9330_i2c_write_16bit(data, SX9330_HOSTIRQMSK_REG, 0x70);

		enable_irq(data->irq);
		enable_irq_wake(data->irq);
	} else {

		pr_info("[SX9330]: %s - disable\n", __func__);

		/*
		 * disable interrupt
		 */
		sx9330_i2c_write_16bit(data, SX9330_HOSTIRQMSK_REG, 0x00);

		disable_irq(data->irq);
		disable_irq_wake(data->irq);	
	}
}

static void sx9330_set_debug_work(struct sx9330_p *data, u8 enable,
		unsigned int time_ms)
{
	if (enable == ON) {
		data->debug_count = 0;
		schedule_delayed_work(&data->debug_work,
			msecs_to_jiffies(time_ms));
	} else {
		cancel_delayed_work_sync(&data->debug_work);
	}
}

static void sx9330_enter_unknown_mode(struct sx9330_p *data, int type)
{
	if (data->noti_enable && !data->skip_data) {
		data->motion = 0;
		data->first_working = false;
		if (data->is_unknown_mode == UNKNOWN_OFF) {
			data->is_unknown_mode = UNKNOWN_ON;
			if (!data->skip_data) {
				input_report_rel(data->input, REL_X, UNKNOWN_ON);
				input_sync(data->input);
			}
			pr_info("[SX9330]: UNKNOWN Re-enter\n");
		} else {
			pr_info("[SX9330]: already UNKNOWN\n");
		}
		input_report_rel(data->noti_input_dev, REL_X, type);
		input_sync(data->noti_input_dev);
	}
}

static ssize_t sx9330_get_offset_calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 val = 0;
	struct sx9330_p *data = dev_get_drvdata(dev);

	sx9330_i2c_read_16bit(data, SX9330_HOSTIRQSRC_REG, &val);

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t sx9330_set_offset_calibration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	struct sx9330_p *data = dev_get_drvdata(dev);

	if (kstrtoul(buf, 10, &val)) {
		pr_err("[SX9330]: %s - Invalid Argument\n", __func__);
		return -EINVAL;
	}

	if (val)
		sx9330_set_offset_calibration(data);

	return count;
}

static ssize_t sx9330_register_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int regist = 0, val = 0;
	struct sx9330_p *data = dev_get_drvdata(dev);

	if (sscanf(buf, "%6x,%10x", &regist, &val) != 2) {
		pr_err("[SX9330]: %s - The number of data are wrong\n",
			__func__);
		return -EINVAL;
	}

	sx9330_i2c_write_16bit(data, (u16)regist, (u32)val);
	pr_info("[SX9330]: %s - Register(0x%2x) data(0x%4x)\n",
		__func__, regist, val);

	return count;
}

static ssize_t sx9330_register_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int offset = 0;
	u32 val = 0;
	int idx;
	struct sx9330_p *data = dev_get_drvdata(dev);

	sx9330_i2c_read_16bit(data, SX9330_HOSTIRQMSK_REG, &val);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"Reg: 0x%x Value: 0x%08x\n", SX9330_HOSTIRQMSK_REG, val);

	for (idx = 0x801C; idx <= 0x8030; idx += 4) {
		sx9330_i2c_read_16bit(data, idx, &val);
		offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"Reg: 0x%x Value: 0x%08x\n", idx, val);
	}

	sx9330_i2c_read_16bit(data, SX9330_ADCFILTPH0_REG, &val);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"Reg: 0x%x Value: 0x%08x\n", SX9330_ADCFILTPH0_REG, val);

	for (idx = 0x8084; idx <= 0x80A0; idx += 4) {
		sx9330_i2c_read_16bit(data, idx, &val);
		offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"Reg: 0x%x Value: 0x%08x\n", idx, val);		
	}

	sx9330_i2c_read_16bit(data, SX9330_STEPCANCEL0A_REG, &val);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"Reg: 0x%x Value: 0x%08x\n", SX9330_STEPCANCEL0A_REG, val);	

	sx9330_i2c_read_16bit(data, SX9330_STEPCANCEL1A_REG, &val);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"Reg: 0x%x Value: 0x%08x\n", SX9330_STEPCANCEL1A_REG, val);

	sx9330_i2c_read_16bit(data, SX9330_REFCORRA_REG, &val);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"Reg: 0x%x Value: 0x%08x\n", SX9330_REFCORRA_REG, val);

	sx9330_i2c_read_16bit(data, SX9330_DBGVARSEL_REG, &val);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"Reg: 0x%x Value: 0x%08x\n", SX9330_DBGVARSEL_REG, val);

	return offset;
}

static ssize_t sx9330_read_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);

	sx9330_display_data_reg(data);

	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t sx9330_sw_reset_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);

	pr_info("[SX9330]: %s\n", __func__);
	sx9330_set_offset_calibration(data);
	msleep(400);
	sx9330_get_data(data);

	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t sx9330_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t sx9330_name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", MODEL_NAME);
}

static ssize_t sx9330_touch_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "1\n");
}

static ssize_t sx9330_raw_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	static s32 sum_diff;
	struct sx9330_p *data = dev_get_drvdata(dev);

	sx9330_get_data(data);
	if (data->diff_cnt == 0)
		sum_diff = data->diff;
	else
		sum_diff += data->diff;

	if (++data->diff_cnt >= DIFF_READ_NUM) {
		data->diff_avg = sum_diff / DIFF_READ_NUM;
		data->useful_avg = sum_diff / DIFF_READ_NUM;
		data->diff_cnt = 0;
	}

	return snprintf(buf, PAGE_SIZE, "%ld,%ld,%u,%ld,%ld\n", (long int)data->capMain,
		(long int)data->useful, data->offset, (long int)data->diff, (long int)data->avg);
}

static ssize_t sx9330_diff_avg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%ld\n", (long int)data->diff_avg);
}

static ssize_t sx9330_useful_avg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%ld\n", (long int)data->useful_avg);
}

static ssize_t sx9330_avgnegfilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	u32 avgnegfilt = 0;

	sx9330_i2c_read_16bit(data, SX9330_AVGBFILTPH0_REG +
		(1 << (4 + MAIN_SENSOR)), &avgnegfilt);
	avgnegfilt = (avgnegfilt & 0x3800) >> 11;

	if (avgnegfilt == 7)
		return snprintf(buf, PAGE_SIZE, "1\n");
	else if (avgnegfilt > 0 && avgnegfilt < 7)
		return snprintf(buf, PAGE_SIZE, "1-1/%d\n", 1 << avgnegfilt);
	else if (avgnegfilt == 0)
		return snprintf(buf, PAGE_SIZE, "0\n");

	return snprintf(buf, PAGE_SIZE, "not set\n");
}

static ssize_t sx9330_avgposfilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	u32 avgposfilt = 0;

	sx9330_i2c_read_16bit(data, SX9330_AVGBFILTPH0_REG +
		(1 << (4 + MAIN_SENSOR)), &avgposfilt);
	avgposfilt = (avgposfilt & 0x700) >> 8;

	switch (avgposfilt) {
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

static ssize_t sx9330_gain_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "None\n");
}

static ssize_t sx9330_range_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "None\n");
}

static ssize_t sx9330_avgthresh_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	u32 avgthresh = 0;

	sx9330_i2c_read_16bit(data, SX9330_AVGBFILTPH0_REG +
		(1 << (4 + MAIN_SENSOR)), &avgthresh);
	avgthresh = (avgthresh & 0x3F000000) >> 24;

	return snprintf(buf, PAGE_SIZE, "%ld\n", 16384 * avgthresh);
}

static ssize_t sx9330_rawfilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	u32 rawfilt = 0;

	sx9330_i2c_read_16bit(data, SX9330_ADCFILTPH0_REG +
		(1 << (4 + MAIN_SENSOR)), &rawfilt);
	rawfilt = (rawfilt & 0x700000) >> 20;

	if (rawfilt > 0 && rawfilt < 8)
		return snprintf(buf, PAGE_SIZE, "1-1/%d\n", 1 << rawfilt);
	else if (rawfilt == 0)
		return snprintf(buf, PAGE_SIZE, "0\n");

	return snprintf(buf, PAGE_SIZE, "not set\n");
}

static ssize_t sx9330_sampling_freq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	u32 sampling_freq = 0;
	const char *table[32] = {
		"250", "200", "166.67", "142.86", "125", "111.11", "100",
		"90.91", "83.33", "76.92", "71.43", "66.67", "62.50", "58.82",
		"55.56", "52.63", "50", "45.45", "41.67", "38.46", "35.71",
		"31.25", "27.78", "25", "20.83", "17.86", "13.89", "11.36",
		"8.33", "6.58", "5.43", "4.63"};

	sx9330_i2c_read_16bit(data, SX9330_AFEPARAMSPH0_REG + MAIN_SENSOR * 8,
		&sampling_freq);
	sampling_freq = (sampling_freq & 0xF8) >> 3;

	return snprintf(buf, PAGE_SIZE, "%skHz\n", table[sampling_freq]);
}

static ssize_t sx9330_scan_period_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	s32 scan_period = 0;

	sx9330_i2c_read_16bit(data, SX9330_SCANPERIOD_REG, &scan_period);
	scan_period = (s32)(scan_period & 0x7FF);
	scan_period = (s32)((scan_period << 11) / 1000);

	return snprintf(buf, PAGE_SIZE, "%ld\n", (long int)scan_period);
}

static ssize_t sx9330_again_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	const char *table[16] = {
		"Reserved", "+/-1.1", "+/-1.65", "+/-2.2",
		"+/-3.3", "+/-3.85", "+/-4.4", "+/-4.95",
		"+/-5.5", "+/-6.05", "+/-6.6F", "+/-7.15",
		"+/-8.25", "+/-8.8", "+/-9.35", "+/-9.9"};
	u32 again = 0;

	sx9330_i2c_read_16bit(data, SX9330_AFEPARAMSPH0_REG + MAIN_SENSOR * 8,
		&again);
	again = (again & 0x1E00) >> 9;

	return snprintf(buf, PAGE_SIZE, "%spF\n", table[again]);
}

static ssize_t sx9330_phase_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", MAIN_SENSOR);
}

static ssize_t sx9330_hysteresis_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	const char *table[4] = {"None", "+/-6%", "+/-12%", "+/-25%"};
	u32 hyst = 0;

	sx9330_i2c_read_16bit(data, SX9330_ADCFILTPH0_REG +
		(1 << (4 + MAIN_SENSOR)), &hyst);
	hyst = (hyst & 0x30) >> 4;

	return snprintf(buf, PAGE_SIZE, "%s\n", table[hyst]);
}

static ssize_t sx9330_resolution_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	u32 resolution = 0;

	sx9330_i2c_read_16bit(data,
		SX9330_AFEPARAMSPH0_REG + MAIN_SENSOR*8, &resolution);
	resolution = resolution & 0x7;

	return snprintf(buf, PAGE_SIZE, "%d\n", 1 << (resolution + 3));
}

static ssize_t sx9330_adc_filt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	u32 adc_filt = 0;

	sx9330_i2c_read_16bit(data, SX9330_ADCFILTPH0_REG +
		(1 << (4 + MAIN_SENSOR)), &adc_filt);
	adc_filt = (adc_filt & 0xC0000) >> 18;

	return snprintf(buf, PAGE_SIZE, "%d\n", 1 << adc_filt);
}

static ssize_t sx9330_useful_filt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	u32 useful_filt = 0;

	sx9330_i2c_read_16bit(data, SX9330_AVGAFILTPH0_REG +
		(1 << (4 + MAIN_SENSOR)), &useful_filt);
	useful_filt = (useful_filt & 0x1000) >> 12;

	return snprintf(buf, PAGE_SIZE, "%s\n", useful_filt ? "on" : "off");
}

static ssize_t sx9330_irq_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);

	int result = 0;
	s32 max_diff_val = 0;

	if (data->irq_count) {
		result = -1;
		max_diff_val = data->max_diff;
	} else {
		max_diff_val = data->max_normal_diff;
	}

	pr_info("[SX9330]: %s - called\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		result, data->irq_count, max_diff_val);
}

static ssize_t sx9330_irq_count_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sx9330_p *data = dev_get_drvdata(dev);

	u8 onoff;
	int ret;

	ret = kstrtou8(buf, 10, &onoff);
	if (ret < 0) {
		pr_err("[SX9330]: %s - kstrtou8 failed.(%d)\n", __func__, ret);
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
		pr_err("[SX9330]: %s - unknown value %d\n", __func__, onoff);
	}

	mutex_unlock(&data->read_mutex);

	pr_info("[SX9330]: %s - %d\n", __func__, onoff);

	return count;
}

static ssize_t sx9330_normal_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	u32 threshold = 0;
	u32 hyst = 0;

	sx9330_i2c_read_16bit(data, SX9330_ADCFILTPH0_REG +
		(1 << (4 + MAIN_SENSOR)), &threshold);

	threshold = (threshold & 0xFF00) >> 8;
	threshold = threshold * threshold / 2;

	sx9330_i2c_read_16bit(data, SX9330_ADCFILTPH0_REG +
		(1 << (4 + MAIN_SENSOR)), &hyst);
	hyst = (hyst & 0x30) >> 4;

	switch (hyst) {
	case 0x01: /* 6% */
		hyst = threshold >> 4;
		break;
	case 0x02: /* 12% */
		hyst = threshold >> 3;
		break;
	case 0x03: /* 25% */
		hyst = threshold >> 2;
		break;
	default:
		/* None */
		break;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d\n",
		threshold + hyst, threshold - hyst);
}

static ssize_t sx9330_onoff_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", !data->skip_data);
}

static ssize_t sx9330_onoff_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u8 val;
	int ret;
	struct sx9330_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &val);
	if (ret) {
		pr_err("[SX9330]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	if (val == 0) {
		data->skip_data = true;
		if (atomic_read(&data->enable) == ON) {
			data->state = IDLE;
			input_report_rel(data->input, REL_MISC, 2);
			input_report_rel(data->input, REL_X, UNKNOWN_OFF);
			input_sync(data->input);
		}		
		data->motion = 1;
		data->is_unknown_mode = UNKNOWN_OFF;
		data->first_working = false;
	} else {
		data->skip_data = false;
	}
	pr_info("[SX9330]: %s -%u\n", __func__, val);
	return count;
}

static ssize_t sx9330_motion_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%s\n",
		data->motion == 1 ? "motion_detect" : "motion_non_detect");
}

static ssize_t sx9330_motion_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u8 val;
	int ret;
	struct sx9330_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &val);
	if (ret) {
		pr_err("[SX9330]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	data->motion = val;

	pr_info("[SX9330]: %s - %u\n", __func__, val);
	return count;
}

static ssize_t sx9330_unknown_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%s\n",
		(data->is_unknown_mode == UNKNOWN_ON) ?	"UNKNOWN" : "NORMAL");
}

static ssize_t sx9330_unknown_state_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u8 val;
	int ret;
	struct sx9330_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &val);
	if (ret) {
		pr_err("[SX9330]: Invalid Argument\n");
		return ret;
	}

	if (val == 1)
		sx9330_enter_unknown_mode(data, TYPE_FORCE);
	else if (val == 0)
		data->is_unknown_mode = UNKNOWN_OFF;
	else
		pr_info("[SX9330]: Invalid Argument(%u)\n", val);

	pr_info("[SX9330]: %u\n", val);

	return count;
}

static ssize_t sx9330_noti_enable_store(struct device *dev,
				     struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	u8 enable;
	struct sx9330_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &enable);
	if (ret) {
		pr_err("[SX9330]: %s - argument\n", __func__);
		return size;
	}

	pr_info("[SX9330]: %s - new_value = %d\n", __func__, (int)enable);

	data->noti_enable = enable;

	if (data->noti_enable)
		sx9330_enter_unknown_mode(data, TYPE_BOOT);
	else {
		data->motion = 1;
		data->first_working = false;
		data->is_unknown_mode = UNKNOWN_OFF;
	}

	return size;
}

static ssize_t sx9330_noti_enable_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->noti_enable);
}

static DEVICE_ATTR(menual_calibrate, S_IRUGO | S_IWUSR | S_IWGRP,
		sx9330_get_offset_calibration_show,
		sx9330_set_offset_calibration_store);
static DEVICE_ATTR(register_write,0220,
		NULL, sx9330_register_write_store);
static DEVICE_ATTR(register_read, 0444,
		sx9330_register_read_show, NULL);
static DEVICE_ATTR(readback, S_IRUGO, sx9330_read_data_show, NULL);
static DEVICE_ATTR(reset, S_IRUGO, sx9330_sw_reset_show, NULL);

static DEVICE_ATTR(name, S_IRUGO, sx9330_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, sx9330_vendor_show, NULL);
static DEVICE_ATTR(mode, S_IRUGO, sx9330_touch_mode_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO, sx9330_raw_data_show, NULL);
static DEVICE_ATTR(diff_avg, 0444, sx9330_diff_avg_show, NULL);
static DEVICE_ATTR(useful_avg, 0444, sx9330_useful_avg_show, NULL);
static DEVICE_ATTR(onoff, S_IRUGO | S_IWUSR | S_IWGRP,
		sx9330_onoff_show, sx9330_onoff_store);
static DEVICE_ATTR(normal_threshold, 0444,
		sx9330_normal_threshold_show, NULL);

static DEVICE_ATTR(avg_negfilt, 0444, sx9330_avgnegfilt_show, NULL);
static DEVICE_ATTR(avg_posfilt, 0444, sx9330_avgposfilt_show, NULL);
static DEVICE_ATTR(avg_thresh, 0444, sx9330_avgthresh_show, NULL);
static DEVICE_ATTR(rawfilt, 0444, sx9330_rawfilt_show, NULL);
static DEVICE_ATTR(sampling_freq, 0444, sx9330_sampling_freq_show, NULL);
static DEVICE_ATTR(scan_period, 0444, sx9330_scan_period_show, NULL);
static DEVICE_ATTR(gain, 0444, sx9330_gain_show, NULL);
static DEVICE_ATTR(range, 0444, sx9330_range_show, NULL);
static DEVICE_ATTR(analog_gain, 0444, sx9330_again_show, NULL);
static DEVICE_ATTR(phase, 0444, sx9330_phase_show, NULL);
static DEVICE_ATTR(hysteresis, 0444, sx9330_hysteresis_show, NULL);
static DEVICE_ATTR(irq_count, 0664,
		sx9330_irq_count_show, sx9330_irq_count_store);
static DEVICE_ATTR(resolution, 0444, sx9330_resolution_show, NULL);
static DEVICE_ATTR(adc_filt, 0444, sx9330_adc_filt_show, NULL);
static DEVICE_ATTR(useful_filt, 0444, sx9330_useful_filt_show, NULL);
static DEVICE_ATTR(motion, S_IRUGO | S_IWUSR | S_IWGRP,
	sx9330_motion_show, sx9330_motion_store);
static DEVICE_ATTR(unknown_state, S_IRUGO | S_IWUSR | S_IWGRP,
	sx9330_unknown_state_show, sx9330_unknown_state_store);
static DEVICE_ATTR(noti_enable, S_IRUGO | S_IWUSR | S_IWGRP,
		   sx9330_noti_enable_show, sx9330_noti_enable_store);

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_menual_calibrate,
	&dev_attr_register_write,
	&dev_attr_register_read,
	&dev_attr_readback,
	&dev_attr_reset,
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_mode,
	&dev_attr_raw_data,
	&dev_attr_diff_avg,
	&dev_attr_useful_avg,
	&dev_attr_onoff,
	&dev_attr_normal_threshold,	
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
	&dev_attr_resolution,
	&dev_attr_adc_filt,
	&dev_attr_useful_filt,
	&dev_attr_motion,
	&dev_attr_unknown_state,
	&dev_attr_noti_enable,
	NULL,
};

/*****************************************************************************/

static ssize_t sx9330_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u8 enable;
	int ret;
	struct sx9330_p *data = dev_get_drvdata(dev);
	int pre_enable = atomic_read(&data->enable);

	ret = kstrtou8(buf, 2, &enable);
	if (ret) {
		pr_err("[SX9330]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	pr_info("[SX9330]: %s - new_value = %u old_value = %d\n",
		__func__, enable, pre_enable);

	if (pre_enable == enable)
		return size;

	atomic_set(&data->enable, enable);
	sx9330_set_enable(data, (int)enable);

	return size;
}

static ssize_t sx9330_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9330_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&data->enable));
}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		sx9330_enable_show, sx9330_enable_store);

static struct attribute *sx9330_attributes[] = {
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group sx9330_attribute_group = {
	.attrs = sx9330_attributes
};

static void sx9330_touch_process(struct sx9330_p *data)
{
	u32 status = 0;

	sx9330_i2c_read_16bit(data, SX9330_STAT0_REG, &status);
	status = status >> 24;
	pr_info("[SX9330]: %s - (status: 0x%x)\n", __func__, status);

	sx9330_get_data(data);
	if (data->abnormal_mode) {
		if (status & (CSX_STATUS_REG << MAIN_SENSOR)) {
			if (data->max_diff < data->diff)
				data->max_diff = data->diff;
			data->irq_count++;
		}
	}

	if (data->state == IDLE) {
		if (status & (CSX_STATUS_REG << MAIN_SENSOR)) {
			if (data->is_unknown_mode == UNKNOWN_ON && data->motion)
				data->first_working = true;
			sx9330_send_event(data, ACTIVE);
		} else {
			pr_info("[SX9330]: %s - %x already released.\n",
					__func__, status);
		}
	} else { // User released
		if (!(status & (CSX_STATUS_REG << MAIN_SENSOR))) {
			if (data->is_unknown_mode == UNKNOWN_ON && data->motion) {
				pr_info("[SX9330]: %s - unknown mode off\n",
					__func__);
				data->is_unknown_mode = UNKNOWN_OFF;
			}
			sx9330_send_event(data, IDLE);
		} else {
			pr_info("[SX9330]: %s - %x still touched\n",
					__func__, status);
		}
	}
}

static void sx9330_process_interrupt(struct sx9330_p *data)
{
	u8 status = 0;

	/* since we are not in an interrupt don't need to disable irq. */
	status = sx9330_read_irqstate(data);

	if (status & IRQ_PROCESS_CONDITION) {
		sx9330_touch_process(data);
	} else {
		pr_info("[SX9330]: %s interrupt generated but skip - status : %d\n",
			__func__, status);
	}
}

static void sx9330_init_work_func(struct work_struct *work)
{
	struct sx9330_p *data = container_of((struct delayed_work *)work,
		struct sx9330_p, init_work);

	int retry = 0;

	sx9330_initialize_chip(data);

	sx9330_set_mode(data, SX9330_MODE_NORMAL);
	/* make sure no interrupts are pending since enabling irq
	 * will only work on next falling edge */
	sx9330_read_irqstate(data);
	msleep(20);

	while(retry++ < 10) {
		sx9330_get_data(data);
		/* Defence code */
		if (data->capMain == 0 && data->avg == 0 && data->diff == 0
			&& data->useful == 0 && data->offset == 0) {
			pr_info("[SX9330]: Defece code for grip sensor - retry: %d\n", retry);

			sx9330_i2c_write_16bit(data, SX9330_RESET_REG, I2C_SOFTRESET_VALUE);
			msleep(300);
			sx9330_initialize_chip(data);
			sx9330_set_mode(data, SX9330_MODE_NORMAL);
			sx9330_read_irqstate(data);
			msleep(20);
		} else {
			break;
		}
	}
}

static void sx9330_irq_work_func(struct work_struct *work)
{
	struct sx9330_p *data = container_of((struct delayed_work *)work,
		struct sx9330_p, irq_work);

	if (sx9330_get_nirq_state(data) == 0)
		sx9330_process_interrupt(data);
	else
		pr_err("[SX9330]: %s - nirq read high %d\n",
			__func__, sx9330_get_nirq_state(data));
}

static void sx9330_check_first_working(struct sx9330_p *data)
{
	if (data->noti_enable && data->motion) {
		if (data->detect_threshold < data->diff) {
			data->first_working = true;
			pr_info("[SX9330]: first working detected %d\n", data->diff);
		} else {
			if (data->first_working) {
				data->is_unknown_mode = UNKNOWN_OFF;
				pr_info("[SX9330]: Release detected %d, unknown mode off\n", data->diff);
			}
		}
	}
}

static void sx9330_debug_work_func(struct work_struct *work)
{
	struct sx9330_p *data = container_of((struct delayed_work *)work,
		struct sx9330_p, debug_work);

	static int hall_flag = 1;

	sx9330_check_hallic_state(HALLIC_PATH, data->hall_ic);

	if (strcmp(data->hall_ic, "CLOSE") == 0) {
		if (hall_flag) {
			pr_info("[SX9330]: %s - hall IC is closed\n", __func__);
			sx9330_set_offset_calibration(data);
			sx9330_enter_unknown_mode(data, TYPE_HALL);
			hall_flag = 0;
		}
	} else {
		if (!hall_flag) {
			pr_info("[SX9330]: %s - hall IC is open\n", __func__);
			sx9330_enter_unknown_mode(data, TYPE_HALL);
			hall_flag = 1;
		}
	}

	if (atomic_read(&data->enable) == ON) {
		if (data->abnormal_mode) {
			sx9330_get_data(data);
			if (data->max_normal_diff < data->diff)
				data->max_normal_diff = data->diff;
		} else {
			if (data->debug_count >= GRIP_LOG_TIME) {
				sx9330_get_data(data);
				data->debug_count = 0;
			} else {
				data->debug_count++;
			}
		}
	}

	if (data->debug_count >= GRIP_LOG_TIME) {
		sx9330_get_data(data);
		if (data->is_unknown_mode == UNKNOWN_ON && data->motion)
			sx9330_check_first_working(data);
		data->debug_count = 0;
	} else {
		if (data->is_unknown_mode == UNKNOWN_ON && data->motion) {
			sx9330_get_data(data);
			sx9330_check_first_working(data);
		}
		data->debug_count++;
	}
	schedule_delayed_work(&data->debug_work, msecs_to_jiffies(2000));
}

static irqreturn_t sx9330_interrupt_thread(int irq, void *pdata)
{
	struct sx9330_p *data = pdata;

	wake_lock_timeout(&data->grip_wake_lock, 3 * HZ);
	schedule_delayed_work(&data->irq_work, msecs_to_jiffies(100));

	return IRQ_HANDLED;
}

static int sx9330_input_init(struct sx9330_p *data)
{
	int ret = 0;
	struct input_dev *dev = NULL;
	struct input_dev *noti_input_dev = NULL;

	/* Create the input device */
	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_MISC);
	input_set_capability(dev, EV_REL, REL_X);
	input_set_drvdata(dev, data);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		return ret;
	}

	ret = sensors_create_symlink(dev);
	if (ret < 0) {
		input_unregister_device(dev);
		return ret;
	}

	ret = sysfs_create_group(&dev->dev.kobj, &sx9330_attribute_group);
	if (ret < 0) {
		sensors_remove_symlink(dev);
		input_unregister_device(dev);
		return ret;
	}

	/* save the input pointer and finish initialization */
	data->input = dev;

	noti_input_dev = input_allocate_device();
	if (!noti_input_dev) {
		pr_err("[SX9330]: %s - input_allocate_device failed\n", __func__);
		input_unregister_device(dev);
		return -ENOMEM;
	}

	noti_input_dev->name = NOTI_MODULE_NAME;
	noti_input_dev->id.bustype = BUS_I2C;

	input_set_capability(noti_input_dev, EV_REL, REL_X);
	input_set_drvdata(noti_input_dev, data);

	ret = input_register_device(noti_input_dev);
	if (ret < 0) {
		pr_err("[SX9330]: %s - failed to register input dev for noti (%d)\n",
			__func__, ret);
		input_unregister_device(dev);
		input_free_device(noti_input_dev);
		return ret;
	}

	data->noti_input_dev = noti_input_dev;

	return 0;
}

static int sx9330_setup_pin(struct sx9330_p *data)
{
	int ret;

	ret = gpio_request(data->gpio_nirq, "SX9330_nIRQ");
	if (ret < 0) {
		pr_err("[SX9330]: %s - gpio %d request failed (%d)\n",
			__func__, data->gpio_nirq, ret);
		return ret;
	}

	ret = gpio_direction_input(data->gpio_nirq);
	if (ret < 0) {
		pr_err("[SX9330]: %s - failed to set gpio %d as input (%d)\n",
			__func__, data->gpio_nirq, ret);
		gpio_free(data->gpio_nirq);
		return ret;
	}

	return 0;
}

#if 0
static void sx9330_set_specific_register(int regi_num, int end, int start,
		u8 val)
{
	u16 clear_bit = 0x00;
	unsigned char temp_val;

	temp_val = setup_reg[regi_num].val;
	clear_bit = ~((1 << (end + 1)) - (1 << start));
	temp_val = (temp_val & clear_bit) | (val << start);
	setup_reg[regi_num].val = temp_val;
}
#endif

static void sx9330_initialize_variable(struct sx9330_p *data)
{
	data->init_done = OFF;
	data->skip_data = false;
	data->state = IDLE;

	data->is_unknown_mode = UNKNOWN_OFF;
	data->motion = 1;
	data->first_working = false;

	atomic_set(&data->enable, OFF);
}


static int sx9330_read_setupreg(struct device_node *dnode, char *str, u32 *val)
{
	u32 temp_val;
	int ret;

	ret = of_property_read_u32(dnode, str, &temp_val);

	if (!ret)
		*val = temp_val;
	else
		pr_err("[SX9330]: %s - %s: property read err 0x%08x (%d)\n",
			__func__, str, temp_val, ret);

	return ret;
}

static int sx9330_parse_dt(struct sx9330_p *data, struct device *dev)
{
	struct device_node *node = dev->of_node;
	enum of_gpio_flags flags;

	u32 scanperiod;
	u32 gnrlctrl2;
	u32 adcfiltph0, adcfiltph1;
	u32 afeparamsph0, afeparamsph1;
	u32 afephph0;
	u32 avgbfilt;
	u32 avgafilt;
	u32 refcorra;
	u32 advdig3, advdig4;

	if (node == NULL)
		return -ENODEV;

	data->gpio_nirq = of_get_named_gpio_flags(node,
		"sx9330,nirq-gpio", 0, &flags);
	if (data->gpio_nirq < 0) {
		pr_err("[SX9330]: %s - get gpio_nirq error\n", __func__);
		return -ENODEV;
	}

	if (!sx9330_read_setupreg(node, SX9330_SCANPERIOD, &scanperiod))
		setup_reg[SX9330_SCANPERIOD_REG_IDX].val = scanperiod;
	if (!sx9330_read_setupreg(node, SX9330_GNRLCTRL2, &gnrlctrl2))
		setup_reg[SX9330_GNRLCTRL2_REG_IDX].val = gnrlctrl2;

	/* phase 0 */
	if (!sx9330_read_setupreg(node, SX9330_AFEPARAMSPH0, &afeparamsph0))
		setup_reg[SX9330_AFEPARAMSPH0_REG_IDX].val = afeparamsph0;
	if (!sx9330_read_setupreg(node, SX9330_AFEPHPH0, &afephph0))
		setup_reg[SX9330_AFEPHPH0_REG_IDX].val = afephph0;
	if (!sx9330_read_setupreg(node, SX9330_ADCFILTPH0, &adcfiltph0))
		setup_reg[SX9330_ADCFILTPH0_REG_IDX].val = adcfiltph0;
	
	/* phase 1 */
	if (!sx9330_read_setupreg(node, SX9330_AFEPARAMSPH1, &afeparamsph1))
		setup_reg[SX9330_AFEPARAMSPH0_REG_IDX + MAIN_SENSOR*2].val = afeparamsph1;
	if (!sx9330_read_setupreg(node, SX9330_ADCFILTPH1, &adcfiltph1))
		setup_reg[SX9330_ADCFILTPH0_REG_IDX + MAIN_SENSOR*8].val = adcfiltph1;
	if (!sx9330_read_setupreg(node, SX9330_AVGBFILT, &avgbfilt))
		setup_reg[SX9330_AVGBFILTPH0_REG_IDX + MAIN_SENSOR*8].val = avgbfilt;
	if (!sx9330_read_setupreg(node, SX9330_AVGAFILT, &avgafilt))
		setup_reg[SX9330_AVGAFILTPH0_REG_IDX + MAIN_SENSOR*8].val = avgafilt;
	if (!sx9330_read_setupreg(node, SX9330_ADVDIG3, &advdig3))
		setup_reg[SX9330_ADVDIG3PH0_REG_IDX + MAIN_SENSOR*8].val = advdig3;
	if (!sx9330_read_setupreg(node, SX9330_ADVDIG4, &advdig4))
		setup_reg[SX9330_ADVDIG4PH0_REG_IDX + MAIN_SENSOR*8].val = advdig4;
	if (!sx9330_read_setupreg(node, SX9330_REFCORRA, &refcorra))
		setup_reg[SX9330_REFCORRA_REG_IDX].val = refcorra;

	return 0;
}

#if defined(CONFIG_CCIC_NOTIFIER) && defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
static int sx9330_ccic_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	CC_NOTI_ATTACH_TYPEDEF usb_typec_info =
		*(CC_NOTI_ATTACH_TYPEDEF *)data;
	struct sx9330_p *pdata =
		container_of(nb, struct sx9330_p, cpuidle_ccic_nb);
	static int pre_attach;

	if (usb_typec_info.src != CCIC_NOTIFY_DEV_MUIC ||
		usb_typec_info.dest != CCIC_NOTIFY_DEV_BATTERY)
		return 0;

	if (pre_attach == usb_typec_info.attach)
		return 0;

	if (pdata->init_done == ON) {
		switch (usb_typec_info.cable_type) {
		case ATTACHED_DEV_NONE_MUIC:
		case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:	/* VBUS enabled */
		case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:	/* for otg test */
		case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:	/* for fuelgauge test */
		case ATTACHED_DEV_JIG_UART_ON_MUIC:
		case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:	/* VBUS enabled */
		case ATTACHED_DEV_JIG_USB_OFF_MUIC:
		case ATTACHED_DEV_JIG_USB_ON_MUIC:
			pr_info("[SX9330]: %s skip cable = %d, attach = %d\n",
				__func__, usb_typec_info.cable_type, usb_typec_info.attach);
			break;
		default:
			pr_info("[SX9330]: %s accept cable = %d, attach = %d\n",
				__func__, usb_typec_info.cable_type, usb_typec_info.attach);
			sx9330_enter_unknown_mode(pdata, TYPE_USB);
			sx9330_set_offset_calibration(pdata);
			break;
		}
	}

	pre_attach = usb_typec_info.attach;

	return 0;
}
#endif

static int sx9330_check_chip_id(struct sx9330_p *data)
{
	int ret;
	u32 value = 0;
	
	ret = sx9330_i2c_read_16bit(data, SX9330_INFO_REG, &value);
	if (ret < 0) {
		pr_err("[SX9330]: whoami[0x%x] read failed %d\n", value, ret);
		return ret;
	}

	value &= 0xFF00;
	value = value >> 8;

	switch (value) {
	case 0x30:
		return 0;
	case 0x34:
		return 0;
	default:
		pr_err("[SX9330]: invalid whoami(%x)\n", value);
		return -1;
	}
}

static int sx9330_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct sx9330_p *data = NULL;

	pr_info("[SX9330]: %s - Probe Start!\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[SX9330]: %s - i2c_check_functionality error\n",
			__func__);
		goto exit;
	}

	/* create memory for main struct */
	data = kzalloc(sizeof(struct sx9330_p), GFP_KERNEL);
	if (data == NULL) {
		pr_err("[SX9330]: %s - kzalloc error\n", __func__);
		ret = -ENOMEM;
		goto exit_kzalloc;
	}

	i2c_set_clientdata(client, data);
	data->client = client;
	data->factory_device = &client->dev;

	ret = sx9330_input_init(data);
	if (ret < 0)
		goto exit_input_init;

	wake_lock_init(&data->grip_wake_lock,
		WAKE_LOCK_SUSPEND, "grip_wake_lock");
	mutex_init(&data->read_mutex);

	ret = sx9330_parse_dt(data, &client->dev);
	if (ret < 0) {
		pr_err("[SX9330]: %s - of_node error\n", __func__);
		ret = -ENODEV;
		goto exit_of_node;
	}

	ret = sx9330_setup_pin(data);
	if (ret) {
		pr_err("[SX9330]: %s - could not setup pin\n", __func__);
		goto exit_setup_pin;
	}

	/* read chip id */
	ret = sx9330_check_chip_id(data);
	if (ret < 0) {
		pr_err("[SX9330]: %s - chip id check failed %d\n", __func__, ret);
		goto exit_chip_reset;
	}

	ret = sx9330_i2c_write_16bit(data, SX9330_RESET_REG, I2C_SOFTRESET_VALUE);
	if (ret < 0) {
		pr_err("[SX9330]: %s - chip reset failed %d\n", __func__, ret);
		goto exit_chip_reset;
	}

	sx9330_initialize_variable(data);
	INIT_DELAYED_WORK(&data->init_work, sx9330_init_work_func);
	INIT_DELAYED_WORK(&data->irq_work, sx9330_irq_work_func);
	INIT_DELAYED_WORK(&data->debug_work, sx9330_debug_work_func);

	data->irq = gpio_to_irq(data->gpio_nirq);
	/* initailize interrupt reporting */
	ret = request_threaded_irq(data->irq, NULL, sx9330_interrupt_thread,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			"sx9330_irq", data);
	if (ret < 0) {
		pr_err("[SX9330]: %s - failed to set request_threaded_irq %d"
			" as returning (%d)\n", __func__, data->irq, ret);
		goto exit_request_threaded_irq;
	}
	disable_irq(data->irq);

	ret = sensors_register(data->factory_device,
		data, sensor_attrs, MODULE_NAME);
	if (ret) {
		pr_err("[SX9330] %s - cound not register sensor(%d).\n",
			__func__, ret);
		goto exit_register_failed;
	}

	schedule_delayed_work(&data->init_work, msecs_to_jiffies(800));
	sx9330_set_debug_work(data, ON, 20000);

#if defined(CONFIG_CCIC_NOTIFIER) && defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	manager_notifier_register(&data->cpuidle_ccic_nb,
					sx9330_ccic_handle_notification,
					MANAGER_NOTIFY_CCIC_SENSORHUB);
#endif

	pr_info("[SX9330]: %s - Probe done!\n", __func__);

	return 0;

exit_register_failed:
	free_irq(data->irq, data);
exit_request_threaded_irq:
exit_chip_reset:
	gpio_free(data->gpio_nirq);
exit_setup_pin:
exit_of_node:	
	mutex_destroy(&data->read_mutex);
	wake_lock_destroy(&data->grip_wake_lock);
	sysfs_remove_group(&data->input->dev.kobj, &sx9330_attribute_group);
	sensors_remove_symlink(data->input);
	input_unregister_device(data->input);
	input_unregister_device(data->noti_input_dev);
exit_input_init:
	kfree(data);	
exit_kzalloc:
exit:
	pr_err("[SX9330]: %s - Probe fail!\n", __func__);
	return ret;
}

static int sx9330_remove(struct i2c_client *client)
{
	struct sx9330_p *data = (struct sx9330_p *)i2c_get_clientdata(client);

	if (atomic_read(&data->enable) == ON)
		sx9330_set_enable(data, OFF);

	sx9330_set_mode(data, SX9330_MODE_SLEEP);

	cancel_delayed_work_sync(&data->init_work);
	cancel_delayed_work_sync(&data->irq_work);
	cancel_delayed_work_sync(&data->debug_work);	
	free_irq(data->irq, data);
	gpio_free(data->gpio_nirq);

	wake_lock_destroy(&data->grip_wake_lock);
	sensors_unregister(data->factory_device, sensor_attrs);
	sensors_remove_symlink(data->input);
	sysfs_remove_group(&data->input->dev.kobj, &sx9330_attribute_group);
	input_unregister_device(data->input);
	input_unregister_device(data->noti_input_dev);
	mutex_destroy(&data->read_mutex);

	kfree(data);

	return 0;
}

static int sx9330_suspend(struct device *dev)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	int cnt = 0;

	pr_info("[SX9330]: %s\n", __func__);
	/* before go to sleep, make the interrupt pin as high*/
	while ((sx9330_get_nirq_state(data) == 0) && (cnt++ < 3)) {
		sx9330_read_irqstate(data);
		msleep(20);
	}
	if (cnt >= 3)
		pr_err("[SX9330]: %s - s/w reset fail(%d)\n", __func__, cnt);

	sx9330_set_debug_work(data, OFF, 1000);

	return 0;
}

static int sx9330_resume(struct device *dev)
{
	struct sx9330_p *data = dev_get_drvdata(dev);
	pr_info("[SX9330]: %s\n", __func__);
	sx9330_set_debug_work(data, ON, 1000);

	return 0;
}

static void sx9330_shutdown(struct i2c_client *client)
{
	struct sx9330_p *data = i2c_get_clientdata(client);

	pr_info("[SX9330]: %s\n", __func__);
	sx9330_set_debug_work(data, OFF, 1000);	
	if (atomic_read(&data->enable) == ON)
		sx9330_set_enable(data, OFF);
	sx9330_set_mode(data, SX9330_MODE_SLEEP);
}

static struct of_device_id sx9330_match_table[] = {
	{ .compatible = "sx9330",},
	{},
};

static const struct i2c_device_id sx9330_id[] = {
	{ "sx9330_match_table", 0 },
	{ }
};

static const struct dev_pm_ops sx9330_pm_ops = {
	.suspend = sx9330_suspend,
	.resume = sx9330_resume,
};

static struct i2c_driver sx9330_driver = {
	.driver = {
		.name	= MODEL_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sx9330_match_table,
		.pm = &sx9330_pm_ops
	},
	.probe		= sx9330_probe,
	.remove		= sx9330_remove,
	.shutdown	= sx9330_shutdown,
	.id_table	= sx9330_id,
};

static int __init sx9330_init(void)
{
	return i2c_add_driver(&sx9330_driver);
}

static void __exit sx9330_exit(void)
{
	i2c_del_driver(&sx9330_driver);
}

module_init(sx9330_init);
module_exit(sx9330_exit);

MODULE_DESCRIPTION("Semtech Corp. SX9330 Capacitive Touch Controller Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
