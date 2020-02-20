/*
 * Copyright (C) 2016 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MODEM_DUMP_H__
#define __MODEM_DUMP_H__

#include <linux/workqueue.h>
#include "modem_prj.h"
#include "link_device_memory.h"

int save_vss_dump(struct link_device *mld, struct io_device *iod,
		unsigned long arg);
int save_acpm_dump(struct link_device *mld, struct io_device *iod,
		unsigned long arg);
int save_cplog_dump(struct link_device *mld, struct io_device *iod,
		unsigned long arg);
int save_shmem_dump(struct link_device *mld, struct io_device *iod,
		unsigned long arg);
int save_databuf_dump(struct link_device *mld, struct io_device *iod,
		unsigned long arg);

void save_mem_dump(struct mem_link_device *mld);

void mem_dump_work(struct work_struct *ws);

#endif
