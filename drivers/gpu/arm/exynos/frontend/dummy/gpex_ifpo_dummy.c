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

#include <gpex_ifpo.h>

int gpex_ifpo_init(void)
{
	return 0;
}

void gpex_ifpo_term(void)
{
	return;
}

int gpex_ifpo_power_up(void)
{
	return 0;
}

int gpex_ifpo_power_down(void)
{
	return 0;
}

ifpo_status gpex_ifpo_get_status(void)
{
	return IFPO_POWER_UP;
}

ifpo_mode gpex_ifpo_get_mode(void)
{
	return IFPO_DISABLED;
}
