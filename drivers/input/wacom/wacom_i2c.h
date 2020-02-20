#ifndef _LINUX_WACOM_I2C_H_
#define _LINUX_WACOM_I2C_H_

#ifdef CONFIG_BATTERY_SAMSUNG
extern unsigned int lpcharge;
#endif

#define CMD_RESULT_WORD_LEN	20
#define POWER_OFFSET		1000000000000

struct wacom_elec_data {
	long long spec_ver;
	u8 max_x_ch;
	u8 max_y_ch;
	u8 shift_value;

	u16 *elec_data;

	u16 *xx;
	u16 *xy;
	u16 *yx;
	u16 *yy;

	long long *xx_xx;
	long long *xy_xy;
	long long *yx_yx;
	long long *yy_yy;

	long long *rxx;
	long long *rxy;
	long long *ryx;
	long long *ryy;

	long long *drxx;
	long long *drxy;
	long long *dryx;
	long long *dryy;

	long long *xx_ref;
	long long *xy_ref;
	long long *yx_ref;
	long long *yy_ref;

	long long *xx_spec;
	long long *xy_spec;
	long long *yx_spec;
	long long *yy_spec;

	long long *rxx_ref;
	long long *rxy_ref;
	long long *ryx_ref;
	long long *ryy_ref;

	long long *drxx_spec;
	long long *drxy_spec;
	long long *dryx_spec;
	long long *dryy_spec;
};

struct wacom_g5_platform_data {
	struct wacom_elec_data *edata;
	int irq_gpio;
	int pdct_gpio;
	int fwe_gpio;
	int boot_addr;
	int irq_type;
	int pdct_type;
	int x_invert;
	int y_invert;
	int xy_switch;
	bool use_dt_coord;
	u32 origin[2];
	int max_x;
	int max_y;
	int max_pressure;
	int max_x_tilt;
	int max_y_tilt;
	int max_height;
	const char *fw_path;
#ifdef CONFIG_SEC_FACTORY
	const char *fw_fac_path;
#endif
	u32 ic_type;
	u32 module_ver;
	bool use_garage;
	u32 table_swap;
	bool use_vddio;
	u32 bringup;

	u32	area_indicator;
	u32	area_navigation;
	u32	area_edge;
};

#endif /* _LINUX_WACOM_I2C_H */
