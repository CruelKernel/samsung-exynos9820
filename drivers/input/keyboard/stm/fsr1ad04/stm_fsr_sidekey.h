#ifndef _LINUX_STM_FSR_SIDEKEY_H_
#define _LINUX_STM_FSR_SIDEKEY_H_

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/serio.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/power_supply.h>
#include <linux/firmware.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/input/mt.h>
#ifdef CONFIG_DRV_SAMSUNG
#include <linux/sec_class.h>
#endif

#include <linux/device.h>
#include <linux/input/sec_cmd.h>
#include <linux/proc_fs.h>
#include <linux/power_supply.h>

#define USE_OPEN_CLOSE
#undef ENABLE_POWER_CONTROL
#define ENABLE_IRQ_HANDLER
#define ENABLE_RESET_GPIO

#define STM_FSR_DRV_NAME	"fsr_sidekey"
#define STM_FSR_DRV_VERSION	"201901023"

#define FSR_STATE_WORK_TIMER		(5 * 1000)

#define CMD_RESULT_WORD_LEN		10

#define FSR_DEBUG_PRINT_ALLEVENT	0x01
#define FSR_DEBUG_PRINT_ONEEVENT	0x02
#define FSR_DEBUG_PRINT_I2C_READ_CMD	0x04
#define FSR_DEBUG_PRINT_I2C_WRITE_CMD	0x08

#define FSR_ID0				0x36
#define FSR_ID1				0x70

#define FSR_FIFO_MAX			32
#define FSR_EVENT_SIZE			8

#define FSR_RETRY_COUNT			10

#define CMD_READ_EVENT			0x86
#define CMD_SET_PURE_AUTOTUNE		0x90
#define CMD_CLEAR_PURE_AUTOTUNE		0x91
#define CMD_SENSEOFF			0x92
#define CMD_SENSEON			0x93
#define CMD_TRIM_LOW_POWER_OSC		0x97
#define CMD_NOTI_SCREEN_OFF		0x9E
#define CMD_NOTI_SCREEN_ON		0x9F
#define CMD_CLEAR_ALL_EVENT		0xA1
#define CMD_PERFORM_AUTOTUNE		0xA5
#define CMD_SPEC_JITTER			0xA8
#define CMD_VERSION_INFO		0xAA
#define CMD_NOTI_AP_BOOTUP		0xAC
#define CMD_ENTER_LPM			0xAD
#define CMD_SAVE_CONFIG_DATA		0xFB
#define CMD_SAVE_TUNNING_DATA		0xFC

#define EID_EVENT			0x0E
#define EID_ERROR			0x0F
#define EID_CONTROLLER_READY		0x10
#define EID_STATUS_EVENT		0x16
#define EID_INT_REL_INFO		0x14
#define EID_EXT_REL_INFO		0x15

#define EID_SPEC_JITTER_RESULT		0xE1

#define EID_ERROR_FLASH_CORRUPTION	0x03

#define STATUS_EVENT_CAL_DONE		0x02
#define STATUS_EVENT_SAVE_CX_DONE	0x04
#define STATUS_EVENT_FORCE_CAL_DONE_D3	0x06
#define STATUS_EVENT_ITO_TEST_DONE	0x0C
#define STATUS_EVENT_SPEC_TEST_DONE	0x0E
#define STATUS_EVENT_PURE_SET_DONE	0x10

#define STATUS_EVENT_ESD_FAILURE	0xED

#define INT_ENABLE_D3			0x40
#define INT_DISABLE_D3			0x00

#define INT_ENABLE			0x01
#define INT_DISABLE			0x00

#define FSR_EVENT_VOLUME_UP		0x01
#define FSR_EVENT_VOLUME_DOWN		0x02
#define FSR_EVENT_BIXBY			0x04

enum fsr_error_return {
	FSR_NOT_ERROR = 0,
	FSR_ERROR_INVALID_CHIP_ID,
	FSR_ERROR_INVALID_SW_VERSION,
	FSR_ERROR_EVENT_ID,
	FSR_ERROR_TIMEOUT,
	FSR_ERROR_FW_UPDATE_FAIL,
	FSR_ERROR_FW_CORRUPTION,
};

struct fsr_sidekey_plat_data {
	const char *firmware_name;
	const char *project_name;
	const char *model_name;

	u16 system_info_addr;

	int bringup;
	int irq_gpio;	/* Interrupt GPIO */
	int rst_gpio;
	int rst_active_low;
	unsigned int irq_type;
	bool irq_invalid;
	bool rst_invalid;

	bool firmware_utype;

#ifdef ENABLE_POWER_CONTROL
	struct pinctrl *pinctrl;

	const char *regulator_dvdd;
	const char *regulator_avdd;

	int (*power)(void *data, bool on);
#endif
};

#define BUFFER_MAX			((256 * 1024) - 16)

static int G1[4] = { -2400, -2400, -4800, -4800 };
static int G2[4][4] = {
	{-250, -250, -500, -500},
	{-125, -125, -250, -250},
	{-500, -500, -1000, -1000},
	{-250, -250, -500, -500}
};

enum {
	TYPE_RAW_DATA,
	TYPE_BASELINE_DATA,
	TYPE_STRENGTH_DATA,
};

struct cx1_data {
	u8 data : 7;
	u8 sign : 1;
} __attribute__((packed));

struct cx2_data {
	u8 data : 6;
	u8 sign : 1;
	u8 reserved : 1;
} __attribute__((packed));

struct cx_data_raw {
	struct cx2_data cx2;
	struct cx1_data cx1;
} __attribute__((packed));

struct channel_cx_data_raw {
	u8 dummy;
	struct cx_data_raw ch12;
	u16 reserved1;
	struct cx_data_raw ch06;
	u16 reserved2[3];
	struct cx_data_raw ch13;
	u16 reserved3;
	struct cx_data_raw ch09;
	u16 reserved4[3];
	struct cx_data_raw ch14;
	u16 reserved5;
	struct cx_data_raw ch07;
	u16 reserved6[3];
	struct cx_data_raw ch15;
	u16 reserved7;
	struct cx_data_raw ch08;
} __attribute__((packed));

struct cx_data {
	short cx1;
	short cx2;
	int total;
};

struct fsr_system_info {
	u8 dummy;
	//Formosa&D3 Signature
	u16 D3Fmsasig;			///< 00 - D3&Formosa signature

	//System information
	u16 fwVer;			///< 02 - firmware version
	u8 cfgid0;			///< 04 - cfg id0
	u8 cfgid1;			///< 05 - cfg id 1
	u16 chipid;			///< 06 - D3 chip id
	u16 fmsaid;			///< 08 - Formosa chip id
	u16 reservedbuffer[9];		///< 0A - Reserved

	//Afe setting
	u8 linelen;			///< 1C - line length
	u8 afelen;			///< 1D - afe length

	//Force Touch Frames
	u16 rawFrcTouchAddr;		///< 1E - Touch raw frame address
	u16 filterFrcTouchAddr;		///< 20 - Touch filtered frame address
	u16 normFrcTouchAddr;		///< 22 - Touch normalized frame address
	u16 calibFrcTouchAddr;		///< 24 - Touch calibrated frame address

	u16 compensationAddr;  		///< 26 - Compensation frame address

	u16 gpiostatusAddr;		///< 28 - GPIO status
	u16 crc_msb;			///< 2A - Checksum (MSB)
	u16 crc_lsb;			///< 2C - Checksum (LSB)

	// Threshold
	u16 key01;			///< 2E - Volume up
	u16 key02;			///< 30 - Volume down
	u16 key03;			///< 32 - AI

	// Reserved
	u16 Reserved1;			///< 34
	// Gain
	u8 gain1;			///< 36
	u8 gain2;			///< 37

	u16 finalAFE;			///< 38
	u16 PureSetFlag;		///< 3A
} __attribute__((packed));

struct fsr_frame_data_info {
	short ch12;
	short ch06;
	short ch14;
	short ch07;
	short ch15;
	short ch08;
} __attribute__((packed));

struct fsr_sidekey_info {
	struct i2c_client *client;
	struct input_dev *input_dev;

	u8 event_state;
	int vol_dn_count;
	int irq;
	int irq_type;
	bool irq_enabled;
	struct fsr_sidekey_plat_data *board;

	bool probe_done;
	volatile bool reset_is_on_going;
	volatile bool fwup_is_on_going;
	unsigned int debug_string;

	struct delayed_work reset_work;
	struct delayed_work state_work;
	struct delayed_work read_info_work;

#ifdef ENABLE_POWER_CONTROL
	int (*power)(void *data, bool on);
#endif

	struct mutex i2c_mutex;
	struct mutex eventlock;

	const char *firmware_name;

	/* thermistor */
	union power_supply_propval psy_value;
	struct power_supply *psy;
	u8 temperature;

	short thd_of_ic[2];
	short thd_of_bin[2];

	u8 product_id_of_ic;			/* product id of ic */
	u16 fw_version_of_ic;			/* firmware version of IC */
	u16 config_version_of_ic;		/* Config release data from IC */
	u16 fw_main_version_of_ic;		/* firmware main version of IC */

	u16 fw_version_of_bin;			/* firmware version of binary */
	u16 config_version_of_bin;		/* Config release data from IC */
	u16 fw_main_version_of_bin;		/* firmware main version of binary */

	struct sec_cmd_data sec;
	struct fsr_system_info fsr_sys_info;
	struct cx_data ch_cx_data[6];

	struct fsr_frame_data_info fsr_frame_data_raw;
	struct fsr_frame_data_info fsr_frame_data_delta;
	struct fsr_frame_data_info fsr_frame_data_reference;
	struct fsr_frame_data_info fsr_cal_strength;
	short calibration_target[4];
};

int fsr_read_reg(struct fsr_sidekey_info *info, unsigned char *reg, int cnum,
		 unsigned char *buf, int num);
int fsr_write_reg(struct fsr_sidekey_info *info,
		  unsigned char *reg, unsigned short num_com);
void fsr_command(struct fsr_sidekey_info *info, unsigned char cmd);
int fsr_wait_for_ready(struct fsr_sidekey_info *info);
int fsr_get_version_info(struct fsr_sidekey_info *info);
int fsr_systemreset(struct fsr_sidekey_info *info, unsigned int delay);
int fsr_systemreset_gpio(struct fsr_sidekey_info *info, unsigned int delay, bool retune);
int fsr_crc_check(struct fsr_sidekey_info *info);
void fsr_interrupt_set(struct fsr_sidekey_info *info, int enable);
void fsr_delay(unsigned int ms);
void fsr_data_dump(struct fsr_sidekey_info *info, u8 *data, int len);
int fsr_read_system_info(struct fsr_sidekey_info *info);

int fsr_fw_wait_for_event(struct fsr_sidekey_info *info, unsigned char eid);
int fsr_fw_wait_for_event_D3(struct fsr_sidekey_info *info, unsigned char eid0, unsigned char eid1);
int fsr_execute_autotune(struct fsr_sidekey_info *info);
int fsr_fw_update_on_probe(struct fsr_sidekey_info *info);
int fsr_fw_update_on_hidden_menu(struct fsr_sidekey_info *info, int update_type);

int fsr_functions_init(struct fsr_sidekey_info *info);
void fsr_functions_remove(struct fsr_sidekey_info *info);
int fsr_read_cx(struct fsr_sidekey_info *info);
int fsr_read_frame(struct fsr_sidekey_info *info, u16 type, struct fsr_frame_data_info *data, bool print);
int fsr_get_calibration_strength(struct fsr_sidekey_info *info);
int fsr_get_threshold(struct fsr_sidekey_info *info);


#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
int fsr_vendor_functions_init(struct fsr_sidekey_info *info);
void fsr_vendor_functions_remove(struct fsr_sidekey_info *info);
#endif

#if defined(CONFIG_DISPLAY_SAMSUNG)
int get_lcd_attached(char *mode);
#endif
#if defined(CONFIG_EXYNOS_DPU20)
int get_lcd_info(char *arg);
#endif

#if defined(CONFIG_ARCH_EXYNOS9)
extern int get_voldn_press(void);
#elif defined(CONFIG_SEC_PM)
extern int get_resinkey_press(void);
#endif

#endif /* _LINUX_STM_FSR_SIDEKEY_H_ */
