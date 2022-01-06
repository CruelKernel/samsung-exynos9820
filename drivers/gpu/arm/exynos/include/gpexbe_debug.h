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

#ifndef _GPEXBE_DEBUG_H_
#define _GPEXBE_DEBUG_H_

int gpexbe_debug_init(void);
void gpexbe_debug_term(void);
void gpexbe_debug_dbg_snapshot_thermal(int frequency);
void gpexbe_debug_dbg_snapshot_freq_in(int freq_before, int freq_after);
void gpexbe_debug_dbg_snapshot_freq_out(int freq_before, int freq_after);

#endif /* _GPEXBE_DEBUG_H_ */
