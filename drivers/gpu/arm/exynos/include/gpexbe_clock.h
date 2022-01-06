/* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#ifndef _GPEXBE_CLOCK_H_
#define _GPEXBE_CLOCK_H_

struct freq_volt {
	int freq;
	int volt;
};

int gpexbe_clock_init(void);
void gpexbe_clock_term(void);

int gpexbe_clock_get_rate_asv_table(struct freq_volt *fv_array, int level_num);
int gpexbe_clock_get_level_num(void);
int gpexbe_clock_get_boot_freq(void);
int gpexbe_clock_get_max_freq(void);
int gpexbe_clock_set_rate(int clk);
int gpexbe_clock_get_rate(void);

#endif /* _GPEXBE_CLOCK_H_ */
