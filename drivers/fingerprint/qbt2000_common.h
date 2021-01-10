#ifndef _UAPI_QBT2000_H_
#define _UAPI_QBT2000_H_

#include <linux/ktime.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/pinctrl/consumer.h>
#include <linux/wakelock.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/pm_qos.h>
#include <linux/cpufreq.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include "../pinctrl/core.h"
#if defined(ENABLE_SENSORS_FPRINT_SECURE) && defined(CONFIG_ARCH_EXYNOS9)
#include <linux/smc.h>
#endif

#undef DISABLED_GPIO_PROTECTION

#define MAX_NAME_SIZE					 32
#define SPI_CLOCK_MAX 25000000

#define QBT2000_DEV "qbt2000"
#define MAX_FW_EVENTS 128
#define FW_MAX_IPC_MSG_DATA_SIZE 0x500
#define IPC_MSG_ID_CBGE_REQUIRED 29
#define IPC_MSG_ID_FINGER_ON_SENSOR 55
#define IPC_MSG_ID_FINGER_OFF_SENSOR 56
#define QBT2000_WAKELOCK_HOLD_TIME 500

#define MINOR_NUM_FD 0
#define MINOR_NUM_IPC 1

#define VENDOR						"QCOM"
#define CHIP_ID						"QBT2000"

#define FINGER_DOWN_GPIO_STATE 1
#define FINGER_LEAVE_GPIO_STATE 0

#if defined(CONFIG_EPEN_WACOM_W9020) || defined(CONFIG_EPEN_WACOM_W9021)
#define QBT2000_AVOID_NOISE
#define QBT2000_NOISE_BLOCK_DELAY 20

enum qbt2000_noise_status {
	QBT2000_NOISE_NO_CHARGING = 0,
	QBT2000_NOISE_CHARGING = 1,
	QBT2000_NOISE_MODE_CHANGED = 2,
	QBT2000_NOISE_I2C_FAILED = 3,
};

enum qbt2000_noise_onoff {
	QBT2000_NOISE_BLOCK = 0,
	QBT2000_NOISE_UNBLOCK = 1,
};

extern int get_wacom_scan_info(bool mode);
extern int set_wacom_ble_charge_mode(bool mode);
#endif

enum qbt2000_commands {
	QBT2000_POWER_CONTROL = 21,
	QBT2000_ENABLE_SPI_CLOCK = 22,
	QBT2000_DISABLE_SPI_CLOCK = 23,
	QBT2000_ENABLE_IPC = 24,
	QBT2000_DISABLE_IPC = 25,
	QBT2000_ENABLE_WUHB = 26,
	QBT2000_DISABLE_WUHB = 27,
	QBT2000_CPU_SPEEDUP = 28,
	QBT2000_SET_SENSOR_TYPE = 29,
	QBT2000_SET_LOCKSCREEN = 30,
	QBT2000_SENSOR_RESET = 31,
	QBT2000_SENSOR_TEST = 32,
	QBT2000_NOISE_REQUEST_STOP = 33,
	QBT2000_NOISE_REQUEST_START = 34,
	QBT2000_NOISE_STATUS_GET = 35,
	QBT2000_NOISE_I2C_RESULT_GET = 36,
	QBT2000_NOISE_REQUEST_STATUS = 37,
	QBT2000_GET_MODELINFO = 38,
	QBT2000_IS_WHUB_CONNECTED = 105,
};

#define QBT2000_SENSORTEST_DONE 		0x0000 // do nothing or test done
#define QBT2000_SENSORTEST_BGECAL 		0x0001 // begin the BGECAL
#define QBT2000_SENSORTEST_NORMALSCAN 	0x0002 // begin the normalscan
#define QBT2000_SENSORTEST_SNR 			0x0004 // begin the snr
#define QBT2000_SENSORTEST_CAPTURE 		0x0008 // begin the image capture. it also needs liveness capture.

/*
 * enum qbt2000_fw_event -
 *      enumeration of firmware events
 * @FW_EVENT_FINGER_DOWN - finger down detected
 * @FW_EVENT_FINGER_UP - finger up detected
 * @FW_EVENT_INDICATION - an indication IPC from the firmware is pending
 */
enum qbt2000_fw_event {
	FW_EVENT_FINGER_DOWN = 1,
	FW_EVENT_FINGER_UP = 2,
	FW_EVENT_CBGE_REQUIRED = 3,
};

struct finger_detect_gpio {
	int gpio;
	int active_low;
	int irq;
	struct work_struct work;
#ifdef QBT2000_AVOID_NOISE
	struct work_struct work_noise_down;
	struct delayed_work delayed_noise_down_work;
#endif
	int last_gpio_state;
};

struct fw_event_desc {
	enum qbt2000_fw_event ev;
};

struct fw_ipc_info {
	int gpio;
	int irq;
};

struct qbt2000_drvdata {
	struct class *qbt2000_class;
	struct cdev	qbt2000_fd_cdev;
	struct cdev	qbt2000_ipc_cdev;
	struct device	*dev;
	struct device *fp_device;
	atomic_t	fd_available;
	atomic_t	ipc_available;
	struct mutex	mutex;
	struct mutex	ioctl_mutex;
	struct mutex	fd_events_mutex;
	struct mutex	ipc_events_mutex;
	struct fw_ipc_info	fw_ipc;
	struct finger_detect_gpio fd_gpio;
	DECLARE_KFIFO(fd_events, struct fw_event_desc, MAX_FW_EVENTS);
	DECLARE_KFIFO(ipc_events, struct fw_event_desc, MAX_FW_EVENTS);
	wait_queue_head_t read_wait_queue_fd;
	wait_queue_head_t read_wait_queue_ipc;

	int ldogpio;
	int spi_speed;
	int sensortype;
	int cbge_count;
	int wuhb_count;
	int reset_count;
	int now_state;
	bool enabled_ipc;
	bool enabled_wuhb;
	bool enabled_ldo;
	bool enabled_clk;
	bool tz_mode;
	bool wuhb_test_flag;
	int wuhb_test_result;
	const char *model_info;
#ifdef QBT2000_AVOID_NOISE
	int noise_status;
	int noise_onoff_flag;
	int ignored_cbge_count;
	int noise_i2c_result;
	int i2c_error_set;
	int i2c_error_get;
	int i2c_charging;
	struct mutex fod_event_mutex;
	struct work_struct work_ipc_noise_status;
	struct work_struct work_noise_control;
	struct delayed_work delayed_work_noiseon;
#endif
	struct pinctrl *p;
	struct pinctrl_state *pins_poweron;
	struct pinctrl_state *pins_poweroff;
	const char *sensor_position;
	const char *btp_vdd;
	struct regulator *regulator_1p8;
	struct work_struct work_debug;
	struct workqueue_struct *wq_dbg;
	struct timer_list dbg_timer;
	struct wake_lock fp_spi_lock;
	struct wake_lock fp_signal_lock;
	struct pm_qos_request pm_qos; // only for QCOM AP
	unsigned int min_cpufreq_limit;
};

int qbt2000_set_clk(struct qbt2000_drvdata *drvdata, bool onoff);
int qbt2000_set_cpu_speedup(struct qbt2000_drvdata *drvdata, int onoff);
int qbt2000_register_platform_variable(struct qbt2000_drvdata *drvdata);
int qbt2000_unregister_platform_variable(struct qbt2000_drvdata *drvdata);

#ifdef CONFIG_BATTERY_SAMSUNG
extern unsigned int lpcharge;
#endif

#endif /* _UAPI_QBT2000_H_ */
