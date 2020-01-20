#ifndef _LINUX_WACOM_I2C_H_
#define _LINUX_WACOM_I2C_H_

#ifdef CONFIG_BATTERY_SAMSUNG
extern unsigned int lpcharge;
#endif

struct wacom_g5_platform_data {
	int irq_gpio;
	int pdct_gpio;
	int fwe_gpio;
	int boot_addr;
	int irq_type;
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
	const char *project_name;
	const char *model_name;
	bool use_virtual_softkey;
	bool use_garage;
	bool support_dex;
	u32 dex_rate;
	bool table_swap;
};

#endif /* _LINUX_WACOM_I2C_H */
