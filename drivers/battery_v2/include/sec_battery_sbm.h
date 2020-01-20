/*
 * sec_battery_log.h
 * Samsung Mobile Charger Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SEC_BATTERY_SBM_H
#define __SEC_BATTERY_SBM_H __FILE__

enum sec_battery_sbm_data_type {
	SBM_DATA_NONE = 0,
	SBM_DATA_COMMON_INFO,
	SBM_DATA_SET_CHARGE,
	SBM_DATA_FULL_1ST,
	SBM_DATA_FULL_2ND,
	SBM_DATA_TEMP,
	SBM_DATA_FG_VEMPTY,
	SBM_DATA_FG_FULL_LOG,
	SBM_DATA_MAX,
};

#define is_sbm_data_type(data_type) \
	(SBM_DATA_NONE < data_type && data_type < SBM_DATA_MAX)
#endif /* __SEC_BATTERY_SBM_H */
