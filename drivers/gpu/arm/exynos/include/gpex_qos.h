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

#ifndef _MALI_EXYNOS_QOS_H_
#define _MALI_EXYNOS_QOS_H_

#include <linux/device.h>

/**
 * typedef enum gpex_qos_flag - flags used as input to gpex_qos_set
 *
 * @QOS_MIN
 * @QOS_MAX
 * @QOS_LITTLE
 * @QOS_MIDDLE
 * @QOS_BIG
 * @QOS_MIF
 */
typedef enum {
	QOS_MIN = 1 << 0,
	QOS_MAX = 1 << 1,
	QOS_LITTLE = 1 << 2,
	QOS_MIDDLE = 1 << 3,
	QOS_BIG = 1 << 4,
	QOS_MIF = 1 << 5,
} gpex_qos_flag;

/**
 * gpex_qos_init() - Initialize QOS module
 *
 * Return: 0 on success
 */
int gpex_qos_init(void);

/**
 * gpex_qos_term() - Terminate QOS module to initial state
 */
void gpex_qos_term(void);

/**
 * gpex_qos_set() - request PMQOS of other IPs
 *
 * @flags: bit vector flags stating what PMQOS operation to request (which IP, min/max lock etc)
 * @val: value used for PMQOS operation. Most likely the frequency to set the other IP to.
 *
 * Request MIN or MAX lock on other IPs. Depending on the SOC, the call may be ignored.
 * For example, requesting a MIN lock on BIG cpu cluster on SOCs with no BIG cluster will
 * be ignored.
 *
 * Return: 0 on success
 */
int gpex_qos_set(gpex_qos_flag flags, int val);

/**
 * gpex_qos_unset() - unset PMQOS of other IPs
 *
 * @flags: bit vector flags stating which IP and lock to unset
 *
 * Unset the QOS locks of given IP as indicated in @flags. The call will be ignored if the IP
 * does not exist QOS was not set in the first place.
 *
 * Return: 0 on success
 */
int gpex_qos_unset(gpex_qos_flag flags);

/**
 * gpex_qos_set_from_clock() - set PMQOS depending on given gpu clock
 *
 * @ clk: gpu clock to get the values for PMQOS
 *
 * Set memory min clock, cpu min clock etc depending on the given gpu clock.
 * The target PMQOS clocks are derived frome a lookup table with gpu clock as the
 * key.
 *
 * Return: 0 on success.
 */
int gpex_qos_set_from_clock(int clk);

/**
 * gpex_qos_set_bts_mo() - set or unset BTS MO depending on given gpu_clock
 *
 * @gpu_clock: clock the gpu will be set to or have been set to.
 *             This value will be used to determine if BTS MO should be set or unset
 *
 * Determine whether the GPU should hold high priority in BTS MO depending on the given
 * @gpu_clock. The minimum frequency at which BTS MO for GPU is set in devicetree as
 * "gpu_mo_min_clock"
 */
int gpex_qos_set_bts_mo(int gpu_clock);

#endif /* _MALI_EXYNOS_QOS_H_ */
