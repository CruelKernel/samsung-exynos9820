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

#include <linux/device.h>
#include <soc/samsung/bts.h>

#include <gpexbe_bts.h>

int gpexbe_bts_set_bts_mo(int val)
{
	bts_update_scen(BS_G3D_PERFORMANCE, val);

	return 0;
}

int gpexbe_bts_init()
{
	return 0;
}

void gpexbe_bts_term()
{
	return;
}
