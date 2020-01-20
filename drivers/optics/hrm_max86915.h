#ifndef _HRM_MAX86915_H_
#define _HRM_MAX86915_H_

int max869_i2c_read(u32 reg, u32 *value, u32 *size);
int max869_i2c_write(u32 reg, u32 value);
int max869_init_device(struct i2c_client *client);
int max869_deinit_device(void);
int max869_enable(enum hrm_mode mode);
int max869_disable(enum hrm_mode mode);
int max869_get_current(u8 *d1, u8 *d2, u8 *d3, u8 *d4);
int max869_set_current(u8 d1, u8 d2, u8 d3, u8 d4);
int max869_get_led_test(u8 *result);
int max869_read_data(struct hrm_output_data *data);
int max869_get_chipid(u64 *chip_id);
int max869_get_part_type(u16 *part_type);
int max869_get_i2c_err_cnt(u32 *err_cnt);
int max869_set_i2c_err_cnt(void);
int max869_get_curr_adc(u16 *ir_curr, u16 *red_curr, u32 *ir_adc, u32 *red_adc);
int max869_set_curr_adc(void);
int max869_get_name_chipset(char *name);
int max869_get_name_vendor(char *name);
int max869_get_threshold(s32 *threshold);
int max869_set_threshold(s32 threshold);
int max869_set_eol_enable(u8 enable);
int max869_get_eol_result(char *result);
int max869_get_eol_status(u8 *status);
int max869_set_pre_eol_enable(u8 enable);
int max869_debug_set(u8 mode);
int max869_get_fac_cmd(char *cmd_result);
int max869_get_version(char *version);
int max869_get_xtalk(u8 *d1, u8 *d2, u8 *d3, u8 *d4);
int max869_set_xtalk(u8 d1, u8 d2, u8 d3, u8 d4);
int max869_get_sdk_enabled(u8 *enabled);
int max869_set_sdk_enabled(int channel, int enable);
int max869_set_sampling_rate(u32 sampling_period_ns);

#endif /* _HRM_MAX86915_H_ */
