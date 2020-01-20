/*
 * Copyright (C) 2011 Samsung Electronics.
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

#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#include <linux/vmalloc.h>
#include <linux/netdevice.h>

#include "modem_prj.h"
#include "modem_utils.h"
#include "link_device_memory.h"

#ifdef GROUP_MEM_LINK_DEBUG

void print_pm_status(struct mem_link_device *mld)
{
#if defined(DEBUG_MODEM_IF) && defined(CONFIG_LINK_POWER_MANAGEMENT)
	struct link_device *ld = &mld->link_dev;
	unsigned int magic;
	int ap_wakeup;
	int ap_status;
	int cp_wakeup;
	int cp_status;

	magic = get_magic(mld);
	ap_wakeup = gpio_get_value(mld->gpio_ap_wakeup);
	ap_status = gpio_get_value(mld->gpio_ap_status);
	cp_wakeup = gpio_get_value(mld->gpio_cp_wakeup);
	cp_status = gpio_get_value(mld->gpio_cp_status);

	/*
	** PM {ap_wakeup:cp_wakeup:cp_status:ap_status:magic} <CALLER>
	*/
	mif_err("%s: PM {%d:%d:%d:%d:%X} %d <%pf>\n",
		ld->name, ap_wakeup, cp_wakeup, cp_status, ap_status,
		magic, atomic_read(&mld->ref_cnt), CALLER);
#endif
}

void print_req_ack(struct mem_link_device *mld, struct mem_snapshot *mst,
		   struct mem_ipc_device *dev, enum direction dir)
{
#ifdef DEBUG_MODEM_IF_FLOW_CTRL
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	enum legacy_ipc_map id = dev->id;
	unsigned int qsize = get_size(cq(dev, dir));
	unsigned int in = mst->head[id][dir];
	unsigned int out = mst->tail[id][dir];
	unsigned int usage = circ_get_usage(qsize, in, out);
	unsigned int space = circ_get_space(qsize, in, out);

	mif_err("REQ_ACK: %s%s%s: %s_%s.%d {in:%u out:%u usage:%u space:%u}\n",
		ld->name, arrow(dir), mc->name, dev->name, q_dir(dir),
		dev->req_ack_cnt[dir], in, out, usage, space);
#endif
}

void print_res_ack(struct mem_link_device *mld, struct mem_snapshot *mst,
		   struct mem_ipc_device *dev, enum direction dir)
{
#ifdef DEBUG_MODEM_IF_FLOW_CTRL
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	enum legacy_ipc_map id = dev->id;
	enum direction opp_dir = opposite(dir);	/* opposite direction */
	unsigned int qsize = get_size(cq(dev, opp_dir));
	unsigned int in = mst->head[id][opp_dir];
	unsigned int out = mst->tail[id][opp_dir];
	unsigned int usage = circ_get_usage(qsize, in, out);
	unsigned int space = circ_get_space(qsize, in, out);

	mif_err("RES_ACK: %s%s%s: %s_%s.%d {in:%u out:%u usage:%u space:%u}\n",
		ld->name, arrow(dir), mc->name, dev->name, q_dir(opp_dir),
		dev->req_ack_cnt[opp_dir], in, out, usage, space);
#endif
}

void print_mem_snapshot(struct mem_link_device *mld, struct mem_snapshot *mst)
{
#ifdef DEBUG_MODEM_IF
	struct link_device *ld = &mld->link_dev;

	mif_err("%s: [%s] ACC{%X %d} FMT{TI:%u TO:%u RI:%u RO:%u} "
		"RAW{TI:%u TO:%u RI:%u RO:%u} INTR{RX:0x%X TX:0x%X}\n",
		ld->name, ipc_dir(mst->dir), mst->magic, mst->access,
		mst->head[IPC_MAP_FMT][TX], mst->tail[IPC_MAP_FMT][TX],
		mst->head[IPC_MAP_FMT][RX], mst->tail[IPC_MAP_FMT][RX],
		mst->head[IPC_MAP_NORM_RAW][TX], mst->tail[IPC_MAP_NORM_RAW][TX],
		mst->head[IPC_MAP_NORM_RAW][RX], mst->tail[IPC_MAP_NORM_RAW][RX],
		mst->int2ap, mst->int2cp);
#endif
}

void print_dev_snapshot(struct mem_link_device *mld, struct mem_snapshot *mst,
			struct mem_ipc_device *dev)
{
#ifdef DEBUG_MODEM_IF
	struct link_device *ld = &mld->link_dev;
	enum legacy_ipc_map id = dev->id;

	if (id >= MAX_SIPC_MAP)
		return;

	mif_err("%s: [%s] %s | TXQ{in:%u out:%u} RXQ{in:%u out:%u} | "
		"INTR{0x%02X}\n",
		ld->name, ipc_dir(mst->dir), dev->name,
		mst->head[id][TX], mst->tail[id][TX],
		mst->head[id][RX], mst->tail[id][RX],
		(mst->dir == RX) ? mst->int2ap : mst->int2cp);
#endif
}

#endif
