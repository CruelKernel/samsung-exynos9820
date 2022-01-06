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

#ifndef _GPEXBE_SMC_H_
#define _GPEXBE_SMC_H_

int gpexbe_smc_protection_enable(void);
int gpexbe_smc_protection_disable(void);

/**
 * gpexbe_smc_notify_power_on() - Notify smc whenever GPU is powered on
 */
void gpexbe_smc_notify_power_on(void);

/**
 * gpexbe_smc_notify_power_off() - Notify smc whenever GPU is powered off
 */
void gpexbe_smc_notify_power_off(void);

int gpexbe_smc_init(void);
void gpexbe_smc_term(void);

#endif
