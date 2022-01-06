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

struct _bts_backend_info {
	unsigned int bts_scen_idx;
};

static struct _bts_backend_info bts_info;

int gpexbe_bts_set_bts_mo(int val)
{
	int ret = 0;

	if (val > 0)
		ret = bts_add_scenario(bts_info.bts_scen_idx);
	else
		ret = bts_del_scenario(bts_info.bts_scen_idx);

	return ret;
}

int gpexbe_bts_init()
{
	bts_info.bts_scen_idx = bts_get_scenindex("g3d_performance");

	return 0;
}

void gpexbe_bts_term()
{
	bts_info.bts_scen_idx = -1;
}
