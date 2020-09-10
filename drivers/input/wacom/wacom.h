#ifndef _LINUX_WACOM_H_
#define _LINUX_WACOM_H_

#include <linux/wakelock.h>
#include <linux/input/sec_cmd.h>

#include "wacom_i2c.h"

/* wacom features */
#define USE_OPEN_CLOSE
#define WACOM_USE_SURVEY_MODE /* SURVEY MODE is LPM mode : Only detect grage(pdct) & aop */

#define RETRY_COUNT			3

#define WACOM_I2C_MODE_NORMAL		false
#define WACOM_I2C_MODE_BOOT		true


#define PDCT_NOSIGNAL			true
#define PDCT_DETECT_PEN			false


/* wacom command */
#define COM_ASYNC_VSYNC			0X28
#define COM_SYNC_VSYNC			0X29
#define COM_QUERY			0x2A
#define COM_SURVEYSCAN			0x2B
#define COM_SURVEYEXIT			0x2D
#define COM_SURVEYSYNCSCAN		0x2E

#define COM_SAMPLERATE_STOP		0x30
#define COM_SAMPLERATE_133		0x31
#define COM_SAMPLERATE_80		0x32
#define COM_SAMPLERATE_40		0x33
#define COM_SAMPLERATE_START		COM_SAMPLERATE_133

#define COM_SURVEY_TARGET_GARAGE_AOP	0x3A
#define COM_SURVEY_TARGET_GARAGEONLY	0x3B

#define COM_CHECKSUM			0x63

#define COM_NORMAL_COMPENSATION		0x80
#define COM_SPECIAL_COMPENSATION	0x81

#define COM_ELEC_SCAN_START		0xC8
#define COM_OPEN_CHECK_START		0xC9
#define COM_ELEC_XSCAN			0xCA
#define COM_ELEC_YSCAN			0xCB
#define COM_ELEC_TRSCON			0xCE
#define COM_ELEC_TRSX0			0xCF

#define COM_ELEC_REQUESTDATA		0xD6
#define COM_OPEN_CHECK_STATUS		0xD8
#define COM_NORMAL_SENSE_MODE		0xDB
#define COM_LOW_SENSE_MODE		0xDC
#define COM_REQUEST_SENSE_MODE		0xDD

#define COM_REQUESTGARAGEDATA		0XE5
#define COM_REQUESTSCANMODE		0xE6
#define COM_LOW_SENSE_MODE2		0xE7
#define COM_BLE_C_DISABLE		0XE8
#define COM_BLE_C_ENABLE		0XE9
#define COM_BLE_C_RESET			0XEA
#define COM_BLE_C_START			0XEB
#define COM_BLE_C_KEEP_ON		0XEC
#define COM_BLE_C_KEEP_Off		0XED
#define COM_BLE_C_M_RETURN		0XEE
#define COM_RESET_DSP			0xEF

#define COM_BLE_C_FULL			0xF3

#define COM_FLASH			0xFF

/* pen ble charging */
enum epen_ble_charge_mode {
	EPEN_BLE_C_DIABLE	= 0,
	EPEN_BLE_C_ENABLE,
	EPEN_BLE_C_RESET,
	EPEN_BLE_C_START,
	EPEN_BLE_C_KEEP_ON,
	EPEN_BLE_C_KEEP_OFF,
	EPEN_BLE_C_M_RETURN,
	EPEN_BLE_C_DSPX,
	EPEN_BLE_C_FULL,
	EPEN_BLE_C_MAX,
};

enum epen_ble_charge_state {
	BLE_C_OFF	= 0,
	BLE_C_START,
	BLE_C_TRANSIT,
	BLE_C_RESET,
	BLE_C_AFTER_START,
	BLE_C_AFTER_RESET,
	BLE_C_ON_KEEP_1,
	BLE_C_OFF_KEEP_1,
	BLE_C_ON_KEEP_2,
	BLE_C_OFF_KEEP_2,
	BLE_C_FULL,
};

/* query data */
#define COM_COORD_NUM			16
#define COM_RESERVED_NUM		0
#define COM_QUERY_NUM			15
#define COM_QUERY_POS			(COM_COORD_NUM+COM_RESERVED_NUM)
#define COM_QUERY_BUFFER		(COM_QUERY_POS+COM_QUERY_NUM)

/* elec data */
#define COM_ELEC_NUM			38
#define COM_ELEC_DATA_NUM		12
#define COM_ELEC_DATA_POS		14


/* wacom query data format */
#define EPEN_REG_HEADER			0x00
#define EPEN_REG_X1			0x01
#define EPEN_REG_X2			0x02
#define EPEN_REG_Y1			0x03
#define EPEN_REG_Y2			0x04
#define EPEN_REG_PRESSURE1		0x05
#define EPEN_REG_PRESSURE2		0x06
#define EPEN_REG_FWVER1			0x07
#define EPEN_REG_FWVER2			0x08
#define EPEN_REG_MPUVER			0x09
#define EPEN_REG_BLVER			0x0A
#define EPEN_REG_TILT_X			0x0B
#define EPEN_REG_TILT_Y			0x0C
#define EPEN_REG_HEIGHT			0x0D
#define EPEN_REG_FMTREV			0x0F


/* skip event for keyboard cover */
#define KEYBOARD_COVER_BOUNDARY  10400
enum epen_virtual_event_mode {
	EPEN_POS_NONE	= 0,
	EPEN_POS_VIEW	= 1,
	EPEN_POS_COVER	= 2,
};

/*--------------------------------------------------
 * event
 * wac_i2c->function_result
 * 7. ~ 4. reserved || 3. Garage | 2. Power Save | 1. AOP | 0. Pen In/Out |
 *
 * 0. Pen In/Out ( pen_insert )
 * ( 0: IN / 1: OUT )
 *--------------------------------------------------
 */
#define EPEN_EVENT_PEN_OUT		(0x1<<0)
#define EPEN_EVENT_GARAGE		(0x1<<1)
#define EPEN_EVENT_AOP			(0x1<<2)
#define EPEN_EVENT_SURVEY		(EPEN_EVENT_GARAGE | EPEN_EVENT_AOP)


/*--------------------------------------------------
 * function setting by user or default
 * wac_i2c->function_set
 * 7~4. reserved | 3. AOT | 2. ScreenOffMemo | 1. AOD | 0. Depend on AOD
 *
 * 3. AOT - aot_enable sysfs
 * 2. ScreenOffMemo - screen_off_memo_enable sysfs
 * 1. AOD - aod_enable sysfs
 * 0. Depend on AOD - 0 : lcd off status, 1 : lcd on status
 *--------------------------------------------------
 */
#define EPEN_SETMODE_AOP_OPTION_AOD_LCD_STATUS	(0x1<<0)
#define EPEN_SETMODE_AOP_OPTION_AOD		(0x1<<1)
#define EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO	(0x1<<2)
#define EPEN_SETMODE_AOP_OPTION_AOT		(0x1<<3)
#define EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON	(EPEN_SETMODE_AOP_OPTION_AOD | EPEN_SETMODE_AOP_OPTION_AOD_LCD_STATUS)
#define EPEN_SETMODE_AOP_OPTION_AOD_LCD_OFF	EPEN_SETMODE_AOP_OPTION_AOD
#define EPEN_SETMODE_AOP			(EPEN_SETMODE_AOP_OPTION_AOD | EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO | \
						 EPEN_SETMODE_AOP_OPTION_AOT)


#define EPEN_SURVEY_MODE_NONE		0x0
#define EPEN_SURVEY_MODE_GARAGE_ONLY	EPEN_EVENT_GARAGE
#define EPEN_SURVEY_MODE_GARAGE_AOP	EPEN_EVENT_AOP


#ifdef WACOM_USE_SURVEY_MODE
#define EPEN_RESUME_DELAY		20
#else
#define EPEN_RESUME_DELAY		180
#endif
#define EPEN_OFF_TIME_LIMIT		10000 // usec



/*--------------------------------------------------
 * Set S-Pen mode for TSP
 * 1 byte input/output parameter
 * byte[0]: S-pen mode
 * - 0: non block tsp scan
 * - 1: block tsp scan
 * - others: Reserved for future use
 *--------------------------------------------------
 */
#define DISABLE_TSP_SCAN_BLCOK		0x00
#define ENABLE_TSP_SCAN_BLCOK		0x01


/* packet ID for wacom data */
enum PACKET_ID {
	COORD_PACKET	= 1,
	AOP_PACKET	= 3,
	NOTI_PACKET	= 13,
	REPLY_PACKET,
	SEPC_PACKET,
};

enum NOTI_SUB_ID {
	ERROR_PACKET		= 0,
	TABLE_SWAP_PACKET,
	EM_NOISE_PACKET,
	TSP_STOP_PACKET,
	OOK_PACKET,
	CMD_PACKET,
};

enum REPLY_SUB_ID {
	OPEN_CHECK_PACKET	= 1,
	SWAP_PACKET,
	SENSITIVITY_PACKET,
	TSP_STOP_COND_PACKET,
	GARAGE_CHARGE_PACKET	= 6,
	ELEC_TEST_PACKET	= 101,
};

enum SEPC_SUB_ID {
	QUERY_PACKET		= 0,
	CHECKSUM_PACKET,
};

enum TABLE_SWAP {
	TABLE_SWAP_DEX_STATION = 1,
	TABLE_SWAP_KBD_COVER = 2,
};

#define FW_UPDATE_RUNNING		1
#define FW_UPDATE_PASS			2
#define FW_UPDATE_FAIL			-1

enum {
	FW_NONE = 0,
	FW_BUILT_IN,
	FW_SPU,
	FW_HEADER,
	FW_IN_SDCARD,
	FW_IN_SDCARD_SIGNED,
#ifdef CONFIG_SEC_FACTORY
	FW_FACTORY_GARAGE,
	FW_FACTORY_UNIT,
#endif
};


/* header ver 1 */
struct fw_image {
	u8 hdr_ver;
	u8 hdr_len;
	u16 fw_ver1;
	u16 fw_ver2;
	u16 fw_ver3;
	u32 fw_len;
	u8 checksum[5];
	u8 alignment_dummy[3];
	u8 data[0];
} __attribute__ ((packed));


enum {
	EPEN_POS_ID_SCREEN_OF_MEMO = 1,
};
/*
 * struct epen_pos - for using to send coordinate in survey mode
 * @id: for extension of function
 *      0 -> not use
 *      1 -> for Screen off Memo
 *      2 -> for oter app or function
 * @x: x coordinate
 * @y: y coordinate
 */
struct epen_pos {
	u8 id;
	int x;
	int y;
};

enum {
	SEC_NORMAL = 0,
	SEC_SHORT,
	SEC_OPEN,
};

struct wacom_i2c {
	struct wacom_g5_platform_data *pdata;
	struct i2c_client *client;
	struct i2c_client *client_boot;
	struct input_dev *input_dev;
	struct sec_cmd_data sec;
	struct mutex lock;
	struct mutex update_lock;
	struct mutex irq_lock;
	struct mutex mode_lock;
	struct mutex ble_charge_mode_lock;
	struct wake_lock fw_wakelock;
	struct delayed_work nb_reg_work;
	struct notifier_block kbd_nb;
	struct work_struct kbd_work;
	u8 kbd_conn_state;
	u8 kbd_cur_conn_state;
	struct notifier_block typec_nb;
	struct work_struct typec_work;
	u8 dp_connect_state;
	u8 dp_connect_cmd;
	int irq;
	int irq_pdct;
	int pen_pdct;
	int pen_prox;
	int pen_pressed;
	int side_pressed;
	bool is_tsp_block;
	int tsp_scan_mode;
	int tsp_block_cnt;
	int tool;
	struct delayed_work pen_insert_dwork;
	bool checksum_result;
	struct delayed_work resume_work;
	struct delayed_work work_print_info;
	u32	print_info_cnt_open;
	u32	scan_info_fail_cnt;
	bool connection_check;
	int  fail_channel;
	int  min_adc_val;
	int  error_cal;
	int  min_cal_val;
	bool battery_saving_mode;
	volatile bool screen_on;
	bool power_enable;
	volatile bool probe_done;
	struct completion resume_done;
	struct wake_lock wakelock;
	bool pm_suspend;
	u8 survey_mode;
	u8 check_survey_mode;
	bool epen_blocked;
	u8 function_set;
	u8 function_result;
	volatile bool reset_flag;
	struct epen_pos survey_pos;
	bool query_status;
	int wcharging_mode;
	u32 i2c_fail_count;
	u32 abnormal_reset_count;
	u32 pen_out_count;
	u32 fw_ver_ic;
	u32 fw_ver_bin;
	int update_status;
	struct work_struct update_work;
	const struct firmware *firm_data;
	struct fw_image *fw_img;
	u8 *fw_data;
	char fw_chksum[5];
	u8 fw_update_way;
	bool do_crc_check;
	bool keyboard_cover_mode;
	bool keyboard_area;
	int virtual_tracking;
	u32 mcount;
	volatile bool is_open_test;
	bool samplerate_state;
	volatile u8 ble_mode;
	volatile bool is_mode_change;
	volatile bool ble_block_flag;
	bool ble_disable_flag;
	u32 chg_time_stamp;
	u32 check_elec;
#ifdef CONFIG_SEC_FACTORY
	volatile bool fac_garage_mode;
	u32 garage_gain0;
	u32 garage_gain1;
	u32 garage_freq0;
	u32 garage_freq1;
#endif
};

struct wacom_i2c *wacom_get_drv_data(void *data);
int wacom_power(struct wacom_i2c *, bool on);
void wacom_reset_hw(struct wacom_i2c *);

void wacom_compulsory_flash_mode(struct wacom_i2c *, bool enable);
int wacom_get_irq_state(struct wacom_i2c *);

void wacom_wakeup_sequence(struct wacom_i2c *);
void wacom_sleep_sequence(struct wacom_i2c *);

int wacom_i2c_load_fw(struct wacom_i2c *wac_i2c, u8 fw_path);
void wacom_i2c_unload_fw(struct wacom_i2c *wac_i2c);
int wacom_fw_update(struct wacom_i2c *, u8 fw_update_way, bool bforced);
int wacom_i2c_flash(struct wacom_i2c *);

void wacom_enable_irq(struct wacom_i2c *, bool enable);
void wacom_enable_pdct_irq(struct wacom_i2c *, bool enable);

int wacom_i2c_send(struct wacom_i2c *, const char *buf, int count, bool mode);
int wacom_i2c_recv(struct wacom_i2c *, char *buf, int count, bool mode);

int wacom_i2c_query(struct wacom_i2c *);
int wacom_checksum(struct wacom_i2c *);
int wacom_i2c_set_sense_mode(struct wacom_i2c *);

void forced_release(struct wacom_i2c *);
void forced_release_fullscan(struct wacom_i2c *wac_i2c);

void wacom_select_survey_mode(struct wacom_i2c *, bool enable);
int wacom_i2c_set_survey_mode(struct wacom_i2c *, int mode);

int wacom_open_test(struct wacom_i2c *wac_i2c);

int wacom_sec_init(struct wacom_i2c *);
void wacom_sec_remove(struct wacom_i2c *);

void wacom_print_info(struct wacom_i2c *wac_i2c);

int wacom_ble_charge_mode(struct wacom_i2c *wac_i2c, int mode);

extern int set_scan_mode(int mode);
#ifdef CONFIG_SEC_FACTORY
bool wacom_check_ub(struct wacom_i2c *wac_i2c);
#endif
#endif /* _LINUX_WACOM_H_ */
