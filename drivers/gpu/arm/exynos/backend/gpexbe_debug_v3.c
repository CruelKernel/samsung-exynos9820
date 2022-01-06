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

#include <soc/samsung/debug-snapshot.h>

#include <gpexbe_debug.h>

struct _debug_backend_info {
	int dss_freq_id;
};

static struct _debug_backend_info dbg_info;

void gpexbe_debug_dbg_snapshot_thermal(int frequency)
{
	dbg_snapshot_thermal(NULL, 0, "GPU", frequency);
}

void gpexbe_debug_dbg_snapshot_freq_in(int freq_before, int freq_after)
{
	if (dbg_info.dss_freq_id)
		dbg_snapshot_freq(dbg_info.dss_freq_id, freq_before, freq_after, DSS_FLAG_IN);
}

void gpexbe_debug_dbg_snapshot_freq_out(int freq_before, int freq_after)
{
	if (dbg_info.dss_freq_id)
		dbg_snapshot_freq(dbg_info.dss_freq_id, freq_before, freq_after, DSS_FLAG_OUT);
}

int gpexbe_debug_init()
{
	dbg_info.dss_freq_id = dbg_snapshot_get_freq_idx("G3D");

	return 0;
}

void gpexbe_debug_term()
{
	dbg_info.dss_freq_id = 0;
}
