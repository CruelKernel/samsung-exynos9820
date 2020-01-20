
#ifndef _SEC_TCLM_H_
#define _SEC_TCLM_H_

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/i2c.h>

/* TCLM_CONCEPT  - start */
#define TCLM_LEVEL_NONE			0x00
#define TCLM_LEVEL_CLEAR_NV		0x01
#define TCLM_LEVEL_LOCKDOWN		0x02
#define TCLM_LEVEL_EVERYTIME	0x05

#define CAL_HISTORY_QUEUE_MAX				10
#define CAL_HISTORY_QUEUE_SHORT_DISPLAY		3

#define TCLM_AMBIENT_CAL			0
#define TCLM_OFFSET_CAL_SDC			1
#define TCLM_OFFSET_CAL_SEC			2

#define SEC_TCLM_NVM_OFFSET			1
#define SEC_TCLM_NVM_OFFSET_LENGTH		4
/* [0]: tclm_level, [1] afe_base_high, [2] afe_base_low = 3byte */

#define SEC_CAL_PASS				1

enum tclm_offset {
	SEC_TCLM_NVM_OFFSET_IC_FIRMWARE_VER		= 1,
	SEC_TCLM_NVM_ALL_DATA				= 2,
	SEC_TCLM_NVM_TEST				= 3,
};

#define CAL_POS_CMD(full_name, short_name)		.f_name = full_name, .s_name = short_name

enum tclm_root {
	CALPOSITION_NONE			= 0,
	CALPOSITION_INITIAL			= 1,
	CALPOSITION_FACTORY			= 2,
	CALPOSITION_OUTSIDE			= 3,
	CALPOSITION_LCIA			= 4,
	CALPOSITION_SVCCENTER		= 5,
	CALPOSITION_ABNORMAL		= 6,
	CALPOSITION_FIRMUP			= 7,
	CALPOSITION_SPECOUT			= 8,
	CALPOSITION_TUNEUP			= 9,
	CALPOSITION_EVERYTIME		= 10,
	CALPOSITION_TESTMODE		= 11,
	CALPOSITION_UNDEFINE		= 12,
	CALPOSITION_MAX			= 16,
};

enum tclm_result {
	TCLM_RESULT_DONE		= 0,
	TCLM_RESULT_ABNORMAL		= 1,
	TCLM_RESULT_CAL_DONE		= 2,
};

struct sec_cal_position {
	const char *f_name;
	const char s_name;
};

struct sec_tclm_nvdata {
	u8 cal_count;
	u16 tune_fix_ver;
	u8 cal_position;
	u8 cal_pos_hist_cnt;
	u8 cal_pos_hist_lastp;
	u8 cal_pos_hist_queue[2 * CAL_HISTORY_QUEUE_MAX];
	u8 cal_fail_falg; /* pass : 1 fail : etc */ 
	u8 cal_fail_cnt; /* history cnt */ 
} __attribute__ ((packed));

/* TCLM_CONCEPT  - end */
struct sec_tclm_data {
	int tclm_level;
	int afe_base;
	bool external_factory;
	u8 root_of_calibration;
	struct sec_cal_position *tclm_string;
	u8 cal_pos_hist_last3[2 * CAL_HISTORY_QUEUE_SHORT_DISPLAY + 1];	/* 7 */
	struct i2c_client *client;
	int (*tclm_read)(struct i2c_client *client, int address);
	int (*tclm_write)(struct i2c_client *client,int address);
	int (*tclm_execute_force_calibration)(struct i2c_client *client, int cal_mode);
	void (*tclm_parse_dt)(struct i2c_client *client, struct sec_tclm_data *tdata);
	struct sec_tclm_nvdata nvdata;
	u8 tclm[SEC_TCLM_NVM_OFFSET_LENGTH];
	bool support_tclm_test;
};

void sec_tclm_case(struct sec_tclm_data *data, int tclm_case);
int sec_tclm_get_nvm_all(struct sec_tclm_data *data);
void sec_tclm_position_history(struct sec_tclm_data *data);
void sec_tclm_root_of_cal(struct sec_tclm_data *data, int pos);
void sec_tclm_debug_info(struct sec_tclm_data *data);
void sec_tclm_reposition_history(struct sec_tclm_data *data);
int sec_execute_tclm_package(struct sec_tclm_data *data, int factory_mode);
int sec_tclm_check_cal_case(struct sec_tclm_data *data);
void sec_tclm_initialize(struct sec_tclm_data *data);
int tclm_test_command(struct sec_tclm_data *data, int test_case, int cmd_param1, int cmd_param2, char *buff);
int sec_tclm_test_on_probe(struct sec_tclm_data *data);

#endif
