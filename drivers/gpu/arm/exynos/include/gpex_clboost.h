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

#ifndef _GPEX_CLBOOST_H_
#define _GPEX_CLBOOST_H_

typedef enum clboost_state {
	CLBOOST_DISABLE = 0,
	CLBOOST_ENABLE,
} clboost_state;

int gpex_clboost_check_activation_condition(void);
void gpex_clboost_set_state(clboost_state state);
int gpex_clboost_init(void);
void gpex_clboost_term(void);

#endif /* _GPEX_CLBOOST_H_ */
