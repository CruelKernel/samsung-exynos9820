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

#include <errno.h>

#include <gpex_ifpo.h>
#include <gpex_clock.h>
#include <gpexbe_devicetree.h>
#include <gpexbe_pm.h>

struct ifpo_info {
	ifpo_mode mode;
	ifpo_status status;
};

static struct ifpo_info ifpo;

int gpex_ifpo_power_down()
{
	int ret = 0;

	if (!ifpo.mode)
		return 0;

	gpex_clock_mutex_lock();

	/* inter frame power off */
	ret = gpexbe_pm_pd_control_down();

	if (!ret)
		ifpo.status = IFPO_POWER_DOWN;

	gpex_clock_mutex_unlock();

	return ret;
}

int gpex_ifpo_power_up()
{
	int ret = 0;

	if (!ifpo.mode)
		return 0;

	gpex_clock_mutex_lock();

	/* inter frame power on */
	ret = gpexbe_pm_pd_control_up();

	if (!ret)
		ifpo.status = IFPO_POWER_UP;

	gpex_clock_mutex_unlock();

	return ret;
}

/* return non-zero value when power off for now */
ifpo_status gpex_ifpo_get_status()
{
	return ifpo.status;
}

ifpo_mode gpex_ifpo_get_mode()
{
	return ifpo.mode;
}

int gpex_ifpo_init()
{
	ifpo.mode = (ifpo_mode)gpexbe_devicetree_get_int(gpu_inter_frame_pm);

	/* TODO: delete this line! This is to force enable IFPO regardless of the DT value */
	/* TODO: Or maybe remove inter_frame_pm from DT and let kconfig decide IFPO enable/disable? */
	ifpo.mode = IFPO_ENABLED;
	ifpo.status = IFPO_POWER_UP;

	return 0;
}

void gpex_ifpo_term()
{
	ifpo.mode = IFPO_DISABLED;

	/* TODO: power up if in power down state? */
	if (ifpo.status == IFPO_POWER_DOWN)
		gpex_ifpo_power_up();

	return;
}
