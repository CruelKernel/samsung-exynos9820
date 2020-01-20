/*
 * sec_thermistor.h - SEC Thermistor
 *
 *  Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *  Minsung Kim <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __LINUX_SEC_THERMISTOR_H
#define __LINUX_SEC_THERMISTOR_H __FILE__

/**
 * struct sec_therm_adc_table - adc to temperature table for sec thermistor
 * driver
 * @adc: adc value
 * @temperature: temperature(Celsius) * 10
 */
struct sec_therm_adc_table {
	int adc;
	int temperature;
};

/**
 * struct sec_bat_plaform_data - init data for sec batter driver
 * @adc_channel: adc channel that connected to thermistor
 * @adc_table: array of adc to temperature data
 * @adc_arr_size: size of adc_table
 */
struct sec_therm_platform_data {
	unsigned int adc_channel;
	unsigned int adc_arr_size;
	struct sec_therm_adc_table *adc_table;
};

#endif /* __LINUX_SEC_THERMISTOR_H */
