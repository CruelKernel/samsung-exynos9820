#if !defined(CONFIG_INPUT_BOOSTER) // Input Booster +
#ifndef _INPUT_BOOSTER_H_
#define _INPUT_BOOSTER_H_

#include <linux/pm_qos.h>
#include <linux/of.h>
#include <linux/cpufreq.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/input.h>

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#ifdef CONFIG_SCHED_HMP
#define USE_HMP_BOOST
#endif

#define ITAG	" [Input Booster] "

#define pr_booster(format, ...) { \
	if (debug_flag) \
		printk(ITAG format, ## __VA_ARGS__); \
}
#define MAX_MULTI_TOUCH_EVENTS		10
#define MAX_IB_COUNT	100
#define MAX_EVENTS			(MAX_MULTI_TOUCH_EVENTS * 10)
#define INPUT_BOOSTER_NULL	-1
#define INIT_ZERO	0
#define DEFAULT_LEVEL 0
#define INPUT_LEVEL 2

//+++++++++++++++++++++++++++++++++++++++++++++++  STRUCT & VARIABLE FOR SYSFS  +++++++++++++++++++++++++++++++++++++++++++++++//
#define SYSFS_CLASS(_ATTR_, _ARGU_, _COUNT_) \
		ssize_t input_booster_sysfs_class_show_##_ATTR_(struct class *dev, struct class_attribute *attr, char *buf) \
		{ \
			ssize_t ret; \
			unsigned int enable_event; \
			unsigned int debug_level; \
			unsigned int sendevent; \
			enable_event = enable_event_booster; \
			debug_level = debug_flag; \
			sendevent = send_ev_enable; \
			ret = sprintf _ARGU_; \
			pr_booster("[Input Booster8] %s buf : %s\n", __func__, buf); \
			return ret; \
		} \
		ssize_t input_booster_sysfs_class_store_##_ATTR_(struct class *dev, struct class_attribute *attr, const char *buf, size_t count) \
		{ \
			unsigned int enable_event[1] = {-1}; \
			unsigned int debug_level[1] = {-1}; \
			unsigned int sendevent[1] = {-1}; \
			sscanf _ARGU_; \
			send_ev_enable = sendevent[0]; \
			debug_flag = debug_level[0]; \
			enable_event_booster = enable_event[0]; \
			pr_booster("[Input Booster8] %s buf : %s\n", __func__, buf); \
			if (sscanf _ARGU_ != _COUNT_) { \
				return count; \
			} \
			return count; \
		} \
    static struct class_attribute class_attr_##_ATTR_ = __ATTR(_ATTR_, S_IRUGO | S_IWUSR, input_booster_sysfs_class_show_##_ATTR_, input_booster_sysfs_class_store_##_ATTR_);

#define HEAD_TAIL_SYSFS_DEVICE(_ATTR_) \
	ssize_t input_booster_sysfs_device_show_##_ATTR_(struct device *dev, struct device_attribute *attr, char *buf) \
	{ \
		int i = 0; \
		ssize_t ret = 0; \
		ssize_t return_value = 0; \
		struct t_ib_device_tree *ib_dt = dev_get_drvdata(dev); \
		if (ib_dt == NULL) { \
			return  ret; \
		} \
		ret = sprintf(buf, "%d", ib_dt->_ATTR_##_time); \
		return_value += ret; \
		buf = buf + ret; \
		for (i = 0; i < MAX_RES_COUNT; ++i) { \
			pr_booster("[Input Booster8] show  i : %d, %s\n", i, #_ATTR_); \
			ret = sprintf(buf, " %d", ib_dt->res[i]._ATTR_##_value); \
			buf = buf + ret; \
			return_value += ret; \
		} \
		ret = sprintf(buf, "\n"); \
		buf = buf + ret; \
		return_value += ret; \
		return return_value; \
	} \
	ssize_t input_booster_sysfs_device_store_##_ATTR_(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) \
	{ \
		struct t_ib_device_tree *ib_dt = dev_get_drvdata(dev); \
		int time = 0; \
		int values[MAX_RES_COUNT + 1] = {0,}; \
		int i = 0; \
		int offset = 0; \
		int dataCnt = 0; \
		pr_booster("[Input Booster8] %s buf : %s\n", __func__, buf); \
		if (ib_dt == NULL) \
			return count; \
		if (!sscanf(buf, "%d%n", &time, &offset)) { \
			pr_booster("### Keep this format : [time cpu_freq hmp_boost ddr_freq lpm_bias] (Ex: 200 1171200 2 1017 5###\n"); \
			return count; \
		} \
		buf += offset; \
		for (i = 0; i < MAX_RES_COUNT; ++i) { \
			if (!sscanf(buf, "%d%n", &values[i], &offset)) { \
				pr_booster("### Keep this format : [time cpu_freq hmp_boost ddr_freq lpm_bias] (Ex: 200 1171200 2 1017 5###\n"); \
				return count; \
			} \
			dataCnt++; \
			buf += offset; \
		} \
		if (sscanf(buf, "%d", &values[i])) { \
			pr_booster("### Keep this format : [time cpu_freq hmp_boost ddr_freq lpm_bias] (Ex: 200 1171200 2 1017 5###\n"); \
			return count; \
		} \
		ib_dt->_ATTR_##_time = time; \
		for (i = 0; i < MAX_RES_COUNT; ++i) { \
			ib_dt->res[i]._ATTR_##_value = values[i]; \
		} \
		return count; \
	} \
	DEVICE_ATTR(_ATTR_, S_IRUGO | S_IWUSR, input_booster_sysfs_device_show_##_ATTR_, input_booster_sysfs_device_store_##_ATTR_);

#define LEVEL_SYSFS_DEVICE(_ATTR_) \
	ssize_t input_booster_sysfs_device_show_##_ATTR_(struct device *dev, struct device_attribute *attr, char *buf) \
	{ \
		ssize_t ret = 0; \
		ret += sprintf(buf, "%d\n", level_value); \
			return  ret; \
		} \
	ssize_t input_booster_sysfs_device_store_##_ATTR_(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) \
	{ \
		int level; \
		sscanf(buf, "%d", &level); \
		if (level < 0 || level > 2) { \
			pr_booster("### Keep this format : greater than 0, and less than 3\n"); \
			return count; \
		} \
		pr_booster("[Input Booster8] %s buf : %s\n", __func__, buf); \
		level_value = level; \
		return count; \
	} \
	DEVICE_ATTR(_ATTR_, S_IRUGO | S_IWUSR, input_booster_sysfs_device_show_##_ATTR_, input_booster_sysfs_device_store_##_ATTR_);

#define INIT_SYSFS_CLASS(_CLASS_) { \
		ret = class_create_file(sysfs_class, &class_attr_##_CLASS_); \
		if (ret) { \
			pr_booster("[Input Booster] Failed to create class\n"); \
			class_destroy(sysfs_class); \
			return; \
		} \
	}

//----------------------------------------------  STRUCT & VARIABLE FOR SYSFS  ----------------------------------------------//

#define TYPE_BITS 8
#define CODE_BITS 12

enum ib_flag_on_off {
	FLAG_OFF = 0,
	FLAG_ON
};

enum booster_head_or_tail {
	IB_HEAD = 0,
	IB_TAIL,
	IB_MAX
};

enum booster_mode_on_off {
	BOOSTER_OFF = 0,
	BOOSTER_ON,
};

enum {
	NONE_TYPE_DEVICE = -1,
	KEY = 0,
	TOUCH_KEY,
	TOUCH,
	MULTI_TOUCH,
	KEYBOARD,
	MOUSE,
	MOUSH_WHEEL,
	HOVER,
	SPEN,
	MAX_DEVICE_TYPE_NUM
};

struct t_ib_trigger {
	char dev_name[100];
	int key_id;
	int event_type;
	int dev_type;

	struct work_struct ib_trigger_work;
};

struct t_ib_info {
	char *dev_name;
	int key_id;
	int uniq_id;
	int press_flag;
	int rel_flag;
	int isHeadFinished;

	struct t_ib_device_tree *ib_dt;

	struct list_head list;

	struct work_struct ib_state_work[IB_MAX];
	struct delayed_work ib_timeout_work[IB_MAX];

	struct mutex lock;
};

struct t_ib_target {
	int uniq_id;
	int value;
	struct list_head list;
};

struct t_ib_res_info {
	int res_id;
	const char *label;
	int head_value;
	int tail_value;
};

struct t_ib_device_tree {
	const char *label;
	int type;
	int head_time;
	int tail_time;
	struct t_ib_res_info *res;
};

struct t_ddr_info {
	long mHz;
	long bps;
};

void trigger_input_booster(struct work_struct *work);
void press_state_func(struct work_struct *work);
void press_timeout_func(struct work_struct *work);
void release_state_func(struct work_struct *work);
void release_timeout_func(struct work_struct *work);

void ib_release_booster(int res_id);
void ib_set_booster(int *qos_values);
unsigned int create_uniq_id(int type, int code, int slot);

void input_booster_init(void);

void init_sysfs_device(struct class *sysfs_class, struct t_ib_device_tree *ib_dt);

void input_booster_init_vendor(int *release_val);
void input_booster_exit_vendor(void);

int set_freq_limit(unsigned long id, unsigned int freq);

#if defined(CONFIG_ARCH_QCOM)
enum booster_res_type {
	CPUFREQ = 0,
	DDRFREQ,
	HMPBOOST,
	LPMBIAS,
	MAX_RES_COUNT
};
#elif defined(CONFIG_ARCH_EXYNOS)
enum booster_res_type {
	CLUSTER2 = 0,
	CLUSTER1,
	CLUSTER0,
	MIF,
	INT,
	HMPBOOST,
	UCC,
	MAX_RES_COUNT
};
#endif

extern int ib_init_succeed;
extern unsigned int debug_flag;
extern unsigned int enable_event_booster;
extern spinlock_t ib_type_lock;

extern struct workqueue_struct *ev_unbound_wq;
extern struct workqueue_struct *ib_unbound_highwq;

extern int trigger_cnt;
// @ ib_trigger : input trigger starts input booster in evdev.c.
extern struct t_ib_trigger *ib_trigger;
// @evdev_mt_slot : save the number of inputed touch slot.
extern int evdev_mt_slot;
// @evdev_mt_event[] : save count of each boooter's events.
extern int evdev_mt_event[MAX_DEVICE_TYPE_NUM];

#endif // _INPUT_BOOSTER_H_
#endif // Input Booster -
