/*
 * sec_battery_dt.h
 * Samsung Mobile Battery Header
 *
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
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

#ifndef __SEC_BATTERY_DT_H
#define __SEC_BATTERY_DT_H __FILE__

int sec_bat_parse_dt(struct device *dev, struct sec_battery_info *battery);
void sec_bat_parse_mode_dt(struct sec_battery_info *battery);
void sec_bat_parse_mode_dt_work(struct work_struct *work);

#endif /* __SEC_BATTERY_DT_H */
