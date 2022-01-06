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

/* Implements */
#include <gpexwa_peak_notify.h>

/* Uses */
#include <linux/notifier.h>
#include <linux/types.h>
#include <gpex_clock.h>

static RAW_NOTIFIER_HEAD(g3d_peak_mode_chain);

static bool is_peak_mode = false;

static bool gpu_clock_at_max_clock(void)
{
	return gpex_clock_get_cur_clock() >= gpex_clock_get_max_clock();
}

static int g3d_notify_peak_mode_update(bool is_set)
{
	return raw_notifier_call_chain(&g3d_peak_mode_chain, 0, &is_set);
}

void gpexwa_peak_notify_update(void)
{
	if (!is_peak_mode && gpu_clock_at_max_clock()) {
		is_peak_mode = true;
		g3d_notify_peak_mode_update(true);
	} else if (is_peak_mode && !gpu_clock_at_max_clock()) {
		is_peak_mode = false;
		g3d_notify_peak_mode_update(false);
	}
}

/* These 2 functions are called by CPU EMS driver */
int g3d_register_peak_mode_update_notifier(struct notifier_block *nb)
{
	return raw_notifier_chain_register(&g3d_peak_mode_chain, nb);
}

int g3d_unregister_peak_mode_notifier(struct notifier_block *nb)
{
	return raw_notifier_chain_unregister(&g3d_peak_mode_chain, nb);
}
