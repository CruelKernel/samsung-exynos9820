/*
 *  sec_audio_debug_v2.c
 *
 *  Copyright (c) 2018 Samsung Electronics
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <sound/samsung/sec_audio_debug.h>

#include <linux/io.h>

#include <linux/sec_debug.h>
#include <sound/samsung/abox.h>
#include "abox/abox.h"
#include "abox/abox_core.h"

#define DBG_STR_BUFF_SZ 256

struct sec_audio_debug_data {
	char *dbg_str_buf;
	unsigned long long mode_time;
	unsigned long mode_nanosec_t;
	struct mutex dbg_lock;
};

static struct sec_audio_debug_data *p_debug_data;

int is_abox_rdma_enabled(int id)
{
	struct abox_data *data = abox_get_abox_data();

	return (readl(data->sfr_base + ABOX_RDMA_CTRL0(id)) & ABOX_RDMA_ENABLE_MASK);
}

int is_abox_wdma_enabled(int id)
{
	struct abox_data *data = abox_get_abox_data();

	return (readl(data->sfr_base + ABOX_WDMA_CTRL(id)) & ABOX_WDMA_ENABLE_MASK);
}

void abox_debug_string_update(void)
{
	struct abox_data *data = abox_get_abox_data();
	int core_id, gpr_id;
	unsigned int len = 0;

	if (!p_debug_data)
		return;

	mutex_lock(&p_debug_data->dbg_lock);

	p_debug_data->mode_time = data->audio_mode_time;
	p_debug_data->mode_nanosec_t = do_div(p_debug_data->mode_time, NSEC_PER_SEC);

	p_debug_data->dbg_str_buf = kzalloc(sizeof(char) * DBG_STR_BUFF_SZ, GFP_KERNEL);
	if (!p_debug_data->dbg_str_buf) {
		pr_err("%s: no memory\n", __func__);
		mutex_unlock(&p_debug_data->dbg_lock);
		return;
	}

	if (!abox_is_on()) {
		snprintf(p_debug_data->dbg_str_buf, DBG_STR_BUFF_SZ, "Abox disabled");
		goto buff_done;
	}

	
	for (core_id = 0; core_id < 2; core_id++) {
		len += snprintf(p_debug_data->dbg_str_buf + len, DBG_STR_BUFF_SZ - len,
						"core%d ", core_id+1);
		if (len >= DBG_STR_BUFF_SZ - 1)
			goto buff_done;

		/* R0~R4, R14, PC */
		for (gpr_id = 0; gpr_id < 5; gpr_id++) {
			len += snprintf(p_debug_data->dbg_str_buf + len, DBG_STR_BUFF_SZ - len,
							"%08x ", abox_core_read_gpr(core_id, gpr_id));
			if (len >= DBG_STR_BUFF_SZ - 1)
				goto buff_done;
		}

		gpr_id = 14;
		len += snprintf(p_debug_data->dbg_str_buf + len, DBG_STR_BUFF_SZ - len,
						"%08x ", abox_core_read_gpr(core_id, gpr_id));
		if (len >= DBG_STR_BUFF_SZ - 1)
			goto buff_done;

		gpr_id = 31;
		len += snprintf(p_debug_data->dbg_str_buf + len, DBG_STR_BUFF_SZ - len,
						"%08x ", abox_core_read_gpr(core_id, gpr_id));
		if (len >= DBG_STR_BUFF_SZ - 1)
			goto buff_done;
	}

	len += snprintf(p_debug_data->dbg_str_buf + len, DBG_STR_BUFF_SZ - len,
						"m%d:%05lu", data->audio_mode, (unsigned long) p_debug_data->mode_time);

buff_done:
	pr_err("%s: Bigdata Upload = %s\n", __func__, p_debug_data->dbg_str_buf);
	//sec_debug_set_extra_info_rvd1(p_debug_data->dbg_str_buf);

	kfree(p_debug_data->dbg_str_buf);
	p_debug_data->dbg_str_buf = NULL;
	mutex_unlock(&p_debug_data->dbg_lock);
}
EXPORT_SYMBOL_GPL(abox_debug_string_update);

static int __init sec_audio_debug_init(void)
{
	struct sec_audio_debug_data *data;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		pr_err("%s: failed to alloc memory\n", __func__);
		return -ENOMEM;
	}

	p_debug_data = data;
	mutex_init(&p_debug_data->dbg_lock);

	return 0;
}
module_init(sec_audio_debug_init);

static void __exit sec_audio_debug_exit(void)
{
	kfree(p_debug_data);
	p_debug_data = NULL;
}
module_exit(sec_audio_debug_exit);
MODULE_DESCRIPTION("Samsung Electronics Audio Debug driver");
MODULE_LICENSE("GPL");
