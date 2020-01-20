#ifndef _HRMSENSOR_H_
#define _HRMSENSOR_H_

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/leds.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/file.h>
#include <linux/syscalls.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm_qos.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif


#define HRM_ON		1
#define HRM_OFF		0
#define PM_RESUME	1
#define PM_SUSPEND	0
#define NAME_LEN		32
#define MAX_BUF_LEN	512

/* #define HRM_DBG */
/* #define HRM_INFO */

#ifdef HRM_DBG
#define HRM_dbg(format, arg...)		\
				printk(KERN_DEBUG "HRM_dbg : "format, ##arg);
#else
#define HRM_dbg(format, arg...)		{if (hrm_debug)\
				printk(KERN_DEBUG "HRM_dbg : "format, ##arg);\
					}
#endif

#ifdef HRM_INFO
#define HRM_info(format, arg...)	\
				printk(KERN_INFO "HRM_info : "format, ##arg);
#else
#define HRM_info(format, arg...)	{if (hrm_info)\
				printk(KERN_INFO "HRM_info : "format, ##arg);\
					}
#endif

enum hrm_mode {
	MODE_NONE			= 0,
	MODE_HRM			= 1,
	MODE_AMBIENT		= 2,
	MODE_PROX			= 3,
	MODE_HRMLED_IR		= 4,
	MODE_HRMLED_RED		= 5,
	MODE_HRMLED_BOTH	= 6,
	MODE_SDK_IR			= 10,
	MODE_SDK_RED		= 11,
	MODE_SDK_GREEN		= 12,
	MODE_SDK_BLUE		= 13,
	MODE_UNKNOWN		= 14,
};

struct hrm_enable_count {
	s32 hrm_cnt;
	s32 amb_cnt;
	s32 prox_cnt;
	s32 sdk_cnt;
	s32 cgm_cnt;
	s32 unkn_cnt;
};

struct hrm_output_data {
	enum hrm_mode mode;
	s32 main_num;
	s32 data_main[6];
	s32 sub_num;
	s32 data_sub[8];
	s32 fifo_main[4][32];
	s32 fifo_num;
};

struct hrm_func {
	int (*i2c_read)(u32 reg, u32 *value, u32 *size);
	int (*i2c_write)(u32 reg, u32 value);
	int (*init_device)(struct i2c_client *client);
	int (*deinit_device)(void);
	int (*enable)(enum hrm_mode);
	int (*disable)(enum hrm_mode);
	int (*set_sampling_rate)(u32);
	int (*get_led_current)(u8 *d1, u8 *d2, u8 *d3, u8 *d4);
	int (*set_led_current)(u8 d1, u8 d2, u8 d3, u8 d4);
	int (*get_led_test)(u8 *result);
	int (*read_data)(struct hrm_output_data *data);
	int (*get_chipid)(u64 *chip_id);
	int (*get_part_type)(u16 *part_type);
	int (*get_i2c_err_cnt)(u32 *err_cnt);
	int (*set_i2c_err_cnt)(void);
	int (*get_curr_adc)(u16 *ir_curr, u16 *red_curr, u32 *ir_adc, u32 *red_adc);
	int (*set_curr_adc)(void);
	int (*get_name_chipset)(char *name);
	int (*get_name_vendor)(char *name);
	int (*get_threshold)(s32 *threshold);
	int (*set_threshold)(s32 threshold);
	int (*set_eol_enable)(u8 enable);
	int (*get_eol_result)(char *result);
	int (*get_eol_status)(u8 *status);
	int (*set_pre_eol_enable)(u8 enable);
	int (*hrm_debug_set)(u8 mode);
	int (*get_fac_cmd)(char *cmd_result);
	int (*get_version)(char *version);
	int (*get_sensor_info)(char *sensor_info_data);
	int (*get_xtalk_code)(u8 *d1, u8 *d2, u8 *d3, u8 *d4);
	int (*set_xtalk_code)(u8 d1, u8 d2, u8 d3, u8 d4);
	int (*get_sdk_enabled)(u8 *enabled);
	int (*set_sdk_enabled)(int channel, int enable);
};

struct hrm_device_data {
	struct i2c_client *hrm_i2c_client;
	struct device *dev;
	struct input_dev *hrm_input_dev;
	struct mutex i2clock;
	struct mutex activelock;
	struct mutex suspendlock;
	struct pinctrl *hrm_pinctrl;
	struct pinctrl_state *pins_sleep;
	struct pinctrl_state *pins_idle;
	char *led_3p3;
	char *vdd_1p8;
	char *i2c_1p8;
	struct hrm_func *h_func;
	enum hrm_mode hrm_enabled_mode;
	enum hrm_mode hrm_prev_mode;
	u32 sampling_period_ns;
	u8 mode_sdk_enabled;
	u8 mode_sdk_prev;
	u8 regulator_state;
	s32 hrm_int;
	s32 hrm_en;
	s32 hrm_irq;
	u8 irq_state;
	u32 led_current;
	u32 xtalk_code;
	s32 hrm_threshold;
	s32 prox_threshold;
	u32 init_current[4];
	s32 eol_test_is_enable;
	u8 eol_test_status;
	s32 pre_eol_test_is_enable;
	u32 reg_read_buf;
	u8 debug_mode;
	struct hrm_enable_count mode_cnt;
	char *lib_ver;
#ifdef CONFIG_ARCH_QCOM
	struct pm_qos_request pm_qos_req_fpm;
#endif
	bool pm_state;
};

extern int sensors_create_symlink(struct input_dev *inputdev);
extern void sensors_remove_symlink(struct input_dev *inputdev);
#ifdef CONFIG_ARCH_QCOM
extern int sensors_register(struct device **dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
#else
extern int sensors_register(struct device *dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
#endif
extern void sensors_unregister(struct device *dev,
	struct device_attribute *attributes[]);

extern unsigned int lpcharge;

#endif /* _HRMSENSOR_H_ */
