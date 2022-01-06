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

#include <gpex_pm.h>

#include "pm_state_runtime_tests.h"

static int state = 0;

/**
 * test_gpex_pm_state_init - Store current state
 *
 * Return: 0 if successful, -1 if unsuccessful
 */
int test_gpex_pm_state_init(void)
{
	if (gpex_pm_get_state(&state)) {
		return -1;
	}

	return 0;
}

/**
 * test_gpex_pm_state_set_bad_state - Try to set pm state using bad value
 *
 * Return: 0 if successful, -1 if unsuccessful
 */
int test_gpex_pm_state_set_bad_state(void)
{
	if (gpex_pm_set_state(-1)) {
		return 0;
	}

	return -1;
}

/**
 * test_gpex_pm_state_set_good_state - Try to set pm state using good value
 *
 * Return: 0 if successful, -1 if unsuccessful
 */
int test_gpex_pm_state_set_good_state(void)
{
	int temp = 0;

	if (gpex_pm_set_state(GPEX_PM_STATE_RESUME_BEGIN)) {
		return -1;
	}

	if (gpex_pm_get_state(&temp)) {
		return -1;
	}

	return (temp == GPEX_PM_STATE_RESUME_BEGIN) ? 0 : -1;
}

/**
 * test_gpex_pm_state_term - Restore previous state
 *
 * Return: 0 if successful, -1 if unsuccessful
 */
int test_gpex_pm_state_term(void)
{
	if (gpex_pm_set_state(state)) {
		return -1;
	}

	return 0;
}
