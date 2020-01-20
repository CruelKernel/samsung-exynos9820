#ifndef _SSP_PLATFORMDATA_H_
#define _SSP_PLATFORMDATA_H_

#ifdef CONFIG_SENSORS_SSP_SHTC1
#include <linux/shtc1_data.h>
#endif

struct ssp_platform_data {
	int (*wakeup_mcu)(void);
	int (*set_mcu_reset)(int);
	int (*check_ap_rev)(void);
	void (*get_positions)(int *, int *);
	u8 mag_matrix_size;
	u8 *mag_matrix;
#ifdef CONFIG_SENSORS_SSP_SHTC1
	u8 cp_thm_adc_channel;
	u8 cp_thm_adc_arr_size;
	u8 batt_thm_adc_arr_size;
	u8 chg_thm_adc_arr_size;
	struct thm_adc_table *cp_thm_adc_table;
	struct thm_adc_table *batt_thm_adc_table;
	struct thm_adc_table *chg_thm_adc_table;
#endif
	int ap_int;
	int mcu_int1;
	int mcu_int2;
};
#endif
