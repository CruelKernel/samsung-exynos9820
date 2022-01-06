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

#include <gpexbe_devicetree.h>

static char dummy_string[] = "dummy";
static dt_clock_item dummy_clock;
static dt_clqos_item dummy_clqos;
static gpu_dt dummy_dt;

int gpexbe_devicetree_get_int_internal(size_t offset, const char *str)
{
	return 0;
}

char *gpexbe_devicetree_get_str_internal(size_t offset, const char *str)
{
	return dummy_string;
}

dt_clock_item *gpexbe_devicetree_get_clock_table(void)
{
	return &dummy_clock;
}

dt_clqos_item *gpexbe_devicetree_get_clqos_table(void)
{
	return &dummy_clqos;
}

gpu_dt *gpexbe_devicetree_get_gpu_dt(void)
{
	return &dummy_dt;
}

int gpexbe_devicetree_init(struct device *dev)
{
	return 0;
}

void gpexbe_devicetree_term(void)
{
	return;
}
