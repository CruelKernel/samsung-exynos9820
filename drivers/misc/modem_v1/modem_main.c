/* linux/drivers/modem/modem.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/if_arp.h>
#include <linux/device.h>

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <linux/mfd/syscon.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-contiguous.h>
#ifdef CONFIG_LINK_FORWARD
#include <linux/linkforward.h>
#endif
#include <uapi/linux/in.h>
#include <linux/inet.h>
#include <net/ipv6.h>

#ifdef CONFIG_LINK_DEVICE_SHMEM
#include <linux/shm_ipc.h>
#include <linux/mcu_ipc.h>
#endif
#if defined(CONFIG_LINK_DEVICE_LLI) &&\
	!defined(CONFIG_LINK_POWER_MANAGEMENT_WITH_FSM)
#include <linux/mipi-lli.h>
#endif

#include "modem_prj.h"
#include "modem_variation.h"
#include "modem_utils.h"
#include "modem_klat.h"

#define FMT_WAKE_TIME   (HZ/2)
#define RAW_WAKE_TIME   (HZ*6)
#define NET_WAKE_TIME	(HZ/2)

#ifdef CONFIG_LINK_FORWARD
#define KOBJ_CLAT "clat"
#endif

static struct modem_shared *create_modem_shared_data(
				struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct modem_shared *msd;
	int size = MAX_MIF_BUFF_SIZE;

	msd = devm_kzalloc(dev, sizeof(struct modem_shared), GFP_KERNEL);
	if (!msd)
		return NULL;

	/* initialize link device list */
	INIT_LIST_HEAD(&msd->link_dev_list);
	INIT_LIST_HEAD(&msd->activated_ndev_list);

	/* initialize tree of io devices */
	msd->iodevs_tree_fmt = RB_ROOT;

	msd->storage.cnt = 0;
	msd->storage.addr = devm_kzalloc(dev, MAX_MIF_BUFF_SIZE +
		(MAX_MIF_SEPA_SIZE * 2), GFP_KERNEL);
	if (!msd->storage.addr) {
		mif_err("IPC logger buff alloc failed!!\n");
		kfree(msd);
		return NULL;
	}
	memset(msd->storage.addr, 0, size + (MAX_MIF_SEPA_SIZE * 2));
	memcpy(msd->storage.addr, MIF_SEPARATOR, strlen(MIF_SEPARATOR));
	msd->storage.addr += MAX_MIF_SEPA_SIZE;
	memcpy(msd->storage.addr, &size, sizeof(int));
	msd->storage.addr += MAX_MIF_SEPA_SIZE;
	spin_lock_init(&msd->lock);
	spin_lock_init(&msd->active_list_lock);

	return msd;
}

static struct modem_ctl *create_modemctl_device(struct platform_device *pdev,
		struct modem_shared *msd)
{
	struct device *dev = &pdev->dev;
	struct modem_data *pdata = pdev->dev.platform_data;
	struct modem_ctl *modemctl;
	int ret;

	/* create modem control device */
	modemctl = devm_kzalloc(dev, sizeof(struct modem_ctl), GFP_KERNEL);
	if (!modemctl) {
		mif_err("%s: modemctl devm_kzalloc fail\n", pdata->name);
		mif_err("%s: xxx\n", pdata->name);
		return NULL;
	}

	modemctl->dev = dev;
	modemctl->name = pdata->name;
	modemctl->mdm_data = pdata;

	modemctl->msd = msd;

	modemctl->phone_state = STATE_OFFLINE;
	modemctl->airplane_mode = 0;

	INIT_LIST_HEAD(&modemctl->modem_state_notify_list);
	spin_lock_init(&modemctl->lock);
	init_completion(&modemctl->init_cmpl);
	init_completion(&modemctl->off_cmpl);

	/* init modemctl device for getting modemctl operations */
	ret = call_modem_init_func(modemctl, pdata);
	if (ret) {
		mif_err("%s: call_modem_init_func fail (err %d)\n",
			pdata->name, ret);
		mif_err("%s: xxx\n", pdata->name);
		devm_kfree(dev, modemctl);
		return NULL;
	}

	mif_info("%s is created!!!\n", pdata->name);

	return modemctl;
}

static struct io_device *create_io_device(struct platform_device *pdev,
		struct modem_io_t *io_t, struct modem_shared *msd,
		struct modem_ctl *modemctl, struct modem_data *pdata)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct io_device *iod;

	iod = devm_kzalloc(dev, sizeof(struct io_device), GFP_KERNEL);
	if (!iod) {
		mif_err("iod == NULL\n");
		return NULL;
	}

	INIT_LIST_HEAD(&iod->list);
	RB_CLEAR_NODE(&iod->node_chan);
	RB_CLEAR_NODE(&iod->node_fmt);

	iod->name = io_t->name;
	iod->id = io_t->id;
	iod->format = io_t->format;
	iod->io_typ = io_t->io_type;
	iod->link_types = io_t->links;
	iod->attrs = io_t->attrs;
	iod->app = io_t->app;
	iod->max_tx_size = io_t->ul_buffer_size;
	iod->net_typ = pdata->modem_net;
	iod->use_handover = pdata->use_handover;
	iod->ipc_version = pdata->ipc_version;
	atomic_set(&iod->opened, 0);
	spin_lock_init(&iod->info_id_lock);

	/* link between io device and modem control */
	iod->mc = modemctl;

	if (iod->format == IPC_FMT && iod->id == SIPC5_CH_ID_FMT_0)
		modemctl->iod = iod;

	if (iod->format == IPC_BOOT) {
		modemctl->bootd = iod;
		mif_err("BOOT device = %s\n", iod->name);
	}

	/* link between io device and modem shared */
	iod->msd = msd;

	/* add iod to rb_tree */
	if (iod->format != IPC_RAW)
		insert_iod_with_format(msd, iod->format, iod);

	if (sipc5_is_not_reserved_channel(iod->id))
		insert_iod_with_channel(msd, iod->id, iod);

	/* register misc device or net device */
	ret = sipc5_init_io_device(iod);
	if (ret) {
		devm_kfree(dev, iod);
		mif_err("sipc5_init_io_device fail (%d)\n", ret);
		return NULL;
	}

	mif_info("%s created\n", iod->name);
	return iod;
}

static int attach_devices(struct io_device *iod, enum modem_link tx_link)
{
	struct modem_shared *msd = iod->msd;
	struct link_device *ld;

	/* find link type for this io device */
	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld)) {
			/* The count 1 bits of iod->link_types is count
			 * of link devices of this iod.
			 * If use one link device,
			 * or, 2+ link devices and this link is tx_link,
			 * set iod's link device with ld
			 */
			if ((count_bits(iod->link_types) <= 1) ||
					(tx_link == ld->link_type)) {
				mif_debug("set %s->%s\n", iod->name, ld->name);
				set_current_link(iod, ld);
			}
		}
	}

	/* if use rx dynamic switch, set tx_link at modem_io_t of
	 * board-*-modems.c
	 */
	if (!get_current_link(iod)) {
		mif_err("%s->link == NULL\n", iod->name);
		BUG();
	}

	wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);

	switch (iod->id) {
	case SIPC5_CH_ID_FMT_0 ... SIPC5_CH_ID_FMT_9:
		iod->waketime = FMT_WAKE_TIME;
		break;

	case SIPC5_CH_ID_BOOT_0 ... SIPC5_CH_ID_DUMP_9:
		iod->waketime = RAW_WAKE_TIME;
		break;

	case SIPC_CH_ID_PDP_0 ... SIPC_CH_ID_LOOPBACK2:
		iod->waketime = NET_WAKE_TIME;
		break;

	default:
		iod->waketime = 0;
		break;
	}

	switch (iod->format) {
	case IPC_FMT:
		iod->waketime = FMT_WAKE_TIME;
		break;

	case IPC_BOOT ... IPC_DUMP:
		iod->waketime = RAW_WAKE_TIME;
		break;

	default:
		break;
	}

	return 0;
}

#ifdef CONFIG_OF
static int parse_dt_common_pdata(struct device_node *np,
				 struct modem_data *pdata)
{
	mif_dt_read_string(np, "mif,name", pdata->name);

	mif_dt_read_enum(np, "mif,modem_net", pdata->modem_net);
	mif_dt_read_enum(np, "mif,modem_type", pdata->modem_type);
	mif_dt_read_bool(np, "mif,use_handover", pdata->use_handover);
	mif_dt_read_enum(np, "mif,ipc_version", pdata->ipc_version);

	mif_dt_read_u32(np, "mif,link_types", pdata->link_types);
	mif_dt_read_string(np, "mif,link_name", pdata->link_name);
	mif_dt_read_u32(np, "mif,link_attrs", pdata->link_attrs);

	mif_dt_read_u32(np, "mif,num_iodevs", pdata->num_iodevs);

	return 0;
}

#if !defined(CONFIG_LINK_DEVICE_SHMEM) && !defined(CONFIG_LINK_DEVICE_PCIE)
static int __parse_dt_mandatory_gpio_pdata(struct device_node *np,
					   struct modem_data *pdata)
{
	int ret = 0;

	/* GPIO_CP_ON */
	pdata->gpio_cp_on = of_get_named_gpio(np, "mif,gpio_cp_on", 0);
	if (!gpio_is_valid(pdata->gpio_cp_on)) {
		mif_err("cp_on: Invalied gpio pins\n");
		return -EINVAL;
	}

	mif_err("gpio_cp_on: %d\n", pdata->gpio_cp_on);
	ret = gpio_request(pdata->gpio_cp_on, "CP_ON");
	if (ret)
		mif_err("fail to request gpio %s:%d\n", "CP_ON", ret);
	gpio_direction_output(pdata->gpio_cp_on, 0);

	/* GPIO_CP_RESET */
	pdata->gpio_cp_reset = of_get_named_gpio(np, "mif,gpio_cp_reset", 0);
	if (!gpio_is_valid(pdata->gpio_cp_reset)) {
		mif_err("cp_rst: Invalied gpio pins\n");
		return -EINVAL;
	}

	mif_err("gpio_cp_reset: %d\n", pdata->gpio_cp_reset);
	ret = gpio_request(pdata->gpio_cp_reset, "CP_RST");
	if (ret)
		mif_err("fail to request gpio %s:%d\n", "CP_RST", ret);
	gpio_direction_output(pdata->gpio_cp_reset, 0);

	/* GPIO_PDA_ACTIVE */
	pdata->gpio_pda_active = of_get_named_gpio(np,
						"mif,gpio_pda_active", 0);
	if (!gpio_is_valid(pdata->gpio_pda_active)) {
		mif_err("pda_active: Invalied gpio pins\n");
		return -EINVAL;
	}

	mif_err("gpio_pda_active: %d\n", pdata->gpio_pda_active);
	ret = gpio_request(pdata->gpio_pda_active, "PDA_ACTIVE");
	if (ret)
		mif_err("fail to request gpio %s:%d\n", "PDA_ACTIVE", ret);
	gpio_direction_output(pdata->gpio_pda_active, 0);

	/* GPIO_PHONE_ACTIVE */
	pdata->gpio_phone_active = of_get_named_gpio(np,
						"mif,gpio_phone_active", 0);
	if (!gpio_is_valid(pdata->gpio_phone_active)) {
		mif_err("phone_active: Invalied gpio pins\n");
		return -EINVAL;
	}

	mif_err("gpio_phone_active: %d\n", pdata->gpio_phone_active);
	ret = gpio_request(pdata->gpio_phone_active, "PHONE_ACTIVE");
	if (ret)
		mif_err("fail to request gpio %s:%d\n", "PHONE_ACTIVE", ret);
	gpio_direction_input(pdata->gpio_phone_active);

	/* GPIO_IPC_INT2CP */
	pdata->gpio_ipc_int2cp = of_get_named_gpio(np,
						"mif,gpio_ipc_int2cp", 0);
	if (gpio_is_valid(pdata->gpio_ipc_int2cp)) {
		mif_err("gpio_ipc_int2cp: %d\n", pdata->gpio_ipc_int2cp);
		ret = gpio_request(pdata->gpio_ipc_int2cp, "IPC_INT2CP");
		if (ret)
			mif_err("fail to request gpio %s:%d\n",
				"SEND_SIG", ret);
		else
			gpio_direction_output(pdata->gpio_ipc_int2cp, 0);
	}

	return ret;
}
#endif

static void __parse_dt_optional_gpio_pdata(struct device_node *np,
					   struct modem_data *pdata)
{
#if !defined(CONFIG_LINK_POWER_MANAGEMENT_WITH_FSM)
	int ret;

	/* GPIO_CP_WAKEUP */
	pdata->gpio_cp_wakeup = of_get_named_gpio(np,
						"mif,gpio_cp_wakeup", 0);
	if (gpio_is_valid(pdata->gpio_cp_wakeup)) {
		mif_err("gpio_cp_wakeup: %d\n", pdata->gpio_cp_wakeup);
		ret = gpio_request(pdata->gpio_cp_wakeup, "CP_WAKEUP");
		if (ret)
			mif_err("fail to request gpio %s:%d\n",
				"CP_WAKEUP", ret);
		else
			gpio_direction_output(pdata->gpio_cp_wakeup, 0);
	}

	/* GPIO_AP_STATUS */
	pdata->gpio_ap_status = of_get_named_gpio(np,
						"mif,gpio_ap_status", 0);
	if (gpio_is_valid(pdata->gpio_ap_status)) {
		mif_err("gpio_ap_status: %d\n", pdata->gpio_ap_status);
		ret = gpio_request(pdata->gpio_ap_status, "AP_STATUS");
		if (ret)
			mif_err("fail to request gpio %s:%d\n",
				"AP_STATUS", ret);
		else
			gpio_direction_output(pdata->gpio_ap_status, 0);
	}

	/* GPIO_AP_WAKEUP */
	pdata->gpio_ap_wakeup = of_get_named_gpio(np,
						"mif,gpio_ap_wakeup", 0);
	if (gpio_is_valid(pdata->gpio_ap_wakeup)) {
		mif_err("gpio_ap_wakeup: %d\n", pdata->gpio_ap_wakeup);
		ret = gpio_request(pdata->gpio_ap_wakeup, "AP_WAKEUP");
		if (ret)
			mif_err("fail to request gpio %s:%d\n",
				"AP_WAKEUP", ret);
		else
			gpio_direction_input(pdata->gpio_ap_wakeup);
	}

	/* GPIO_CP_STATUS */
	pdata->gpio_cp_status = of_get_named_gpio(np,
						"mif,gpio_cp_status", 0);
	if (gpio_is_valid(pdata->gpio_cp_status)) {
		mif_err("gpio_cp_status: %d\n", pdata->gpio_cp_status);
		ret = gpio_request(pdata->gpio_cp_status, "CP_STATUS");
		if (ret)
			mif_err("fail to request gpio %s:%d\n",
				"CP_STATUS", ret);
		else
			gpio_direction_input(pdata->gpio_cp_status);
	}
#endif
}

static int parse_dt_gpio_pdata(struct device_node *np, struct modem_data *pdata)
{
	int err = 0;
#if defined(CONFIG_LINK_POWER_MANAGEMENT_WITH_FSM)
	struct device_node *pm_node;
#endif

#if !defined(CONFIG_LINK_DEVICE_SHMEM) && !defined(CONFIG_LINK_DEVICE_PCIE)
	err = __parse_dt_mandatory_gpio_pdata(np, pdata);
	if (err)
		return err;
#endif
	__parse_dt_optional_gpio_pdata(np, pdata);

#if defined(CONFIG_LINK_POWER_MANAGEMENT_WITH_FSM)
	pm_node = of_get_child_by_name(np, PM_DT_NODE_NAME);
	if (!pm_node) {
		mif_err("ERR! No PM DT node\n");
		return -ENOSYS;
	}

	err = link_pm_parse_dt_gpio_pdata(pm_node, pdata);
	if (err)
		return err;
#endif
	return err;
}

static int parse_dt_mbox_pdata(struct device *dev, struct device_node *np,
					struct modem_data *pdata)
{
	struct modem_mbox *mbox;
	int ret = 0;

	if (pdata->link_types != LINKTYPE(LINKDEV_SHMEM) &&
			pdata->link_types != LINKTYPE(LINKDEV_PCIE)) {
		mif_err("mbox: link type error:0x%08x\n", pdata->link_types);
		return ret;
	}

	mbox = (struct modem_mbox *)devm_kzalloc(dev, sizeof(struct modem_mbox), GFP_KERNEL);
	if (!mbox) {
		mif_err("mbox: failed to alloc memory\n");
		return -ENOMEM;
	}
	pdata->mbx = mbox;

	mif_dt_read_u32(np, "mif,int_ap2cp_msg", mbox->int_ap2cp_msg);
	mif_dt_read_u32(np, "mif,int_ap2cp_wakeup", mbox->int_ap2cp_wakeup);
	mif_dt_read_u32(np, "mif,int_ap2cp_status", mbox->int_ap2cp_status);
	mif_dt_read_u32(np, "mif,int_ap2cp_active", mbox->int_ap2cp_active);

	mif_dt_read_u32(np, "mif,irq_cp2ap_msg", mbox->irq_cp2ap_msg);
	mif_dt_read_u32(np, "mif,irq_cp2ap_status", mbox->irq_cp2ap_status);
	mif_dt_read_u32(np, "mif,irq_cp2ap_perf_req_cpu",
		mbox->irq_cp2ap_perf_req_cpu);
	mif_dt_read_u32(np, "mif,irq_cp2ap_perf_req_mif",
		mbox->irq_cp2ap_perf_req_mif);
	mif_dt_read_u32(np, "mif,irq_cp2ap_perf_req_int",
		mbox->irq_cp2ap_perf_req_int);
	mif_dt_read_u32(np, "mif,irq_cp2ap_active", mbox->irq_cp2ap_active);
	mif_dt_read_u32(np, "mif,irq_cp2ap_wakelock", mbox->irq_cp2ap_wakelock);
	mif_dt_read_u32(np, "mif,irq_cp2ap_ratmode", mbox->irq_cp2ap_rat_mode);

	mif_dt_read_u32(np, "mbx_ap2cp_msg", mbox->mbx_ap2cp_msg);
	mif_dt_read_u32(np, "mbx_cp2ap_msg", mbox->mbx_cp2ap_msg);
	mif_dt_read_u32(np, "mbx_cp2ap_united_status", mbox->mbx_cp2ap_status);
	mif_dt_read_u32(np, "mbx_ap2cp_united_status", mbox->mbx_ap2cp_status);
	mif_dt_read_u32(np, "mbx_cp2ap_dvfsreq_cpu", mbox->mbx_cp2ap_perf_req_cpu);
	mif_dt_read_u32(np, "mbx_cp2ap_dvfsreq_mif", mbox->mbx_cp2ap_perf_req_mif);
	mif_dt_read_u32(np, "mbx_cp2ap_dvfsreq_int", mbox->mbx_cp2ap_perf_req_int);
	mif_dt_read_u32(np, "mbx_ap2cp_kerneltime", mbox->mbx_ap2cp_kerneltime);

	/* Status Bit Info */
	mif_dt_read_u32(np, "sbi_lte_active_mask", mbox->sbi_lte_active_mask);
	mif_dt_read_u32(np, "sbi_lte_active_pos", mbox->sbi_lte_active_pos);
	mif_dt_read_u32(np, "sbi_cp_status_mask", mbox->sbi_cp_status_mask);
	mif_dt_read_u32(np, "sbi_cp_status_pos", mbox->sbi_cp_status_pos);
	mif_dt_read_u32(np, "sbi_cp_rat_mode_mask", mbox->sbi_cp2ap_rat_mode_mask);
	mif_dt_read_u32(np, "sbi_cp_rat_mode_pos", mbox->sbi_cp2ap_rat_mode_pos);
	mif_dt_read_u32(np, "sbi_cp2ap_wakelock_mask", mbox->sbi_cp2ap_wakelock_mask);
	mif_dt_read_u32(np, "sbi_cp2ap_wakelock_pos", mbox->sbi_cp2ap_wakelock_pos);
	mif_dt_read_u32(np, "sbi_pda_active_mask", mbox->sbi_pda_active_mask);
	mif_dt_read_u32(np, "sbi_pda_active_pos", mbox->sbi_pda_active_pos);
	mif_dt_read_u32(np, "sbi_ap_status_mask", mbox->sbi_ap_status_mask);
	mif_dt_read_u32(np, "sbi_ap_status_pos", mbox->sbi_ap_status_pos);
	mif_dt_read_u32(np, "sbi_crash_type_mask", mbox->sbi_crash_type_mask);
	mif_dt_read_u32(np, "sbi_crash_type_pos", mbox->sbi_crash_type_pos);

	mif_dt_read_u32(np, "sbi_ap2cp_kerneltime_sec_mask",
			mbox->sbi_ap2cp_kerneltime_sec_mask);
	mif_dt_read_u32(np, "sbi_ap2cp_kerneltime_sec_pos",
			mbox->sbi_ap2cp_kerneltime_sec_pos);
	mif_dt_read_u32(np, "sbi_ap2cp_kerneltime_usec_mask",
			mbox->sbi_ap2cp_kerneltime_usec_mask);
	mif_dt_read_u32(np, "sbi_ap2cp_kerneltime_usec_pos",
			mbox->sbi_ap2cp_kerneltime_usec_pos);

	return ret;
}

static int parse_dt_iodevs_pdata(struct device *dev, struct device_node *np,
				 struct modem_data *pdata)
{
	struct device_node *child = NULL;
	size_t size = sizeof(struct modem_io_t) * pdata->num_iodevs;
	int i = 0;

	pdata->iodevs = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!pdata->iodevs) {
		mif_err("iodevs: failed to alloc memory\n");
		return -ENOMEM;
	}

	for_each_child_of_node(np, child) {
		struct modem_io_t *iod;

		iod = &pdata->iodevs[i];

		mif_dt_read_string(child, "iod,name", iod->name);
		mif_dt_read_u32(child, "iod,id", iod->id);
		mif_dt_read_enum(child, "iod,format", iod->format);
		mif_dt_read_enum(child, "iod,io_type", iod->io_type);
		mif_dt_read_u32(child, "iod,links", iod->links);
		if (count_bits(iod->links) > 1)
			mif_dt_read_enum(child, "iod,tx_link", iod->tx_link);
		mif_dt_read_u32(child, "iod,attrs", iod->attrs);
		/* mif_dt_read_string(child, "iod,app", iod->app); */
		mif_dt_read_u32_noerr(child, "iod,max_tx_size",
				iod->ul_buffer_size);
		if (iod->attrs & IODEV_ATTR(ATTR_SBD_IPC)) {
			mif_dt_read_u32(child, "iod,ul_num_buffers",
					iod->ul_num_buffers);
			mif_dt_read_u32(child, "iod,ul_buffer_size",
					iod->ul_buffer_size);
			mif_dt_read_u32(child, "iod,dl_num_buffers",
					iod->dl_num_buffers);
			mif_dt_read_u32(child, "iod,dl_buffer_size",
					iod->dl_buffer_size);
		}

		if (iod->attrs & IODEV_ATTR(ATTR_OPTION_REGION))
			mif_dt_read_string(child, "iod,option_region",
					iod->option_region);

		i++;
	}

	return 0;
}

static struct modem_data *modem_if_parse_dt_pdata(struct device *dev)
{
	struct modem_data *pdata;
	struct device_node *iodevs = NULL;

	pdata = devm_kzalloc(dev, sizeof(struct modem_data), GFP_KERNEL);
	if (!pdata) {
		mif_err("modem_data: alloc fail\n");
		return ERR_PTR(-ENOMEM);
	}

	if (parse_dt_common_pdata(dev->of_node, pdata)) {
		mif_err("DT error: failed to parse common\n");
		goto error;
	}

	if (parse_dt_gpio_pdata(dev->of_node, pdata)) {
		mif_err("DT error: failed to parse gpio\n");
		goto error;
	}

	if (parse_dt_mbox_pdata(dev, dev->of_node, pdata)) {
		mif_err("DT error: failed to parse mbox\n");
		goto error;
	}

	iodevs = of_get_child_by_name(dev->of_node, "iodevs");
	if (!iodevs) {
		mif_err("DT error: failed to get child node\n");
		goto error;
	}

	if (parse_dt_iodevs_pdata(dev, iodevs, pdata)) {
		mif_err("DT error: failed to parse iodevs\n");
		goto error;
	}

	dev->platform_data = pdata;
	mif_info("DT parse complete!\n");
	return pdata;

error:
	if (pdata) {
		if (pdata->iodevs)
			devm_kfree(dev, pdata->iodevs);
		devm_kfree(dev, pdata);
	}
	return ERR_PTR(-EINVAL);
}

static const struct of_device_id sec_modem_match[] = {
	{ .compatible = "sec_modem,modem_pdata", },
	{},
};
MODULE_DEVICE_TABLE(of, sec_modem_match);
#else
static struct modem_data *modem_if_parse_dt_pdata(struct device *dev)
{
	return ERR_PTR(-ENODEV);
}
#endif

enum mif_sim_mode {
	MIF_SIM_NONE = 0,
	MIF_SIM_SINGLE,
	MIF_SIM_DUAL,
	MIF_SIM_TRIPLE,
};

static int simslot_count(struct seq_file *m, void *v)
{
	enum mif_sim_mode mode = (enum mif_sim_mode)m->private;

	seq_printf(m, "%u\n", mode);
	return 0;
}

static int simslot_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, simslot_count, PDE_DATA(inode));
}

static const struct file_operations simslot_count_fops = {
	.open	= simslot_count_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release = single_release,
};

#ifdef CONFIG_GPIO_DS_DETECT
static enum mif_sim_mode get_sim_mode(struct device_node *of_node)
{
	enum mif_sim_mode mode = MIF_SIM_SINGLE;
	int gpio_ds_det;
	int retval;

	gpio_ds_det = of_get_named_gpio(of_node, "mif,gpio_ds_det", 0);
	if (!gpio_is_valid(gpio_ds_det)) {
		mif_err("DT error: failed to get sim mode\n");
		goto make_proc;
	}

	retval = gpio_request(gpio_ds_det, "DS_DET");
	if (retval) {
		mif_err("Failed to reqeust GPIO(%d)\n", retval);
		goto make_proc;
	} else {
		gpio_direction_input(gpio_ds_det);
	}

	retval = gpio_get_value(gpio_ds_det);
	if (retval)
		mode = MIF_SIM_DUAL;

	mif_info("sim_mode: %d\n", mode);
	gpio_free(gpio_ds_det);

make_proc:
	if (!proc_create_data("simslot_count", 0, NULL, &simslot_count_fops,
			(void *)(long)mode)) {
		mif_err("Failed to create proc\n");
		mode = MIF_SIM_SINGLE;
	}

	return mode;
}
#else
static enum mif_sim_mode get_sim_mode(struct device_node *of_node)
{
	return MIF_SIM_SINGLE;
}
#endif

static ssize_t do_cp_crash_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long flags;
	struct modem_ctl *mc = dev_get_drvdata(dev);
	spin_lock_irqsave(&mc->lock, flags);
	if (mc->bootd && atomic_read(&mc->bootd->opened) > 0)
		mc->bootd->modem_state_changed(mc->bootd, STATE_CRASH_EXIT);
	spin_unlock_irqrestore(&mc->lock, flags);
	return count;
}

static ssize_t modem_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct modem_ctl *mc = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", cp_state_str(mc->phone_state));
}

#ifdef CONFIG_LINK_FORWARD
static ssize_t linkforward_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return linkforward_get_state(buf);
}

static ssize_t linkforward_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "linkforward global mode:%d\n", get_linkforward_mode());
}

static ssize_t linkforward_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	int val = 0;

	ret = sscanf(buf, "%u", &val);
	if ((ret > 3) || (val < 0))
		return -EINVAL;

	set_linkforward_mode(val);

	ret = count;
	return ret;
}

static ssize_t xlat_plat_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
#ifdef CONFIG_CP_DIT
	return sprintf(buf, "plat prefix: %pI6\n", &nf_linkfwd.plat_prfix);
#else
	return sprintf(buf, "DIT NOT supported\n");
#endif
}

static ssize_t xlat_plat_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf, size_t count)
{
#ifdef CONFIG_CP_DIT
	struct in6_addr val;
	char *ptr = NULL;

	mif_err("-- plat prefix: %s\n", buf);

	ptr = strstr(buf, "@");
	if (!ptr)
		return -EINVAL;
	*ptr++ = '\0';

	if (in6_pton(buf, strlen(buf), val.s6_addr, '\0', NULL) == 0)
		return -EINVAL;

	nf_linkfwd.plat_prfix = val;

	if (strstr(ptr, "rmnet0")) {
		dit_set_clat_plat_prfix(0, val);
	} else if (strstr(ptr, "rmnet1")) {
		dit_set_clat_plat_prfix(1, val);
	} else {
		mif_err("-- unhandled plat prefix for device %s\n", ptr);
	}

	*(--ptr) = '@';
	(void)klat_plat_store(kobj, attr, buf, count);

	mif_err("plat prefix: %pI6\n", &nf_linkfwd.plat_prfix);
#else
	mif_err("DIT NOT supported\n");
#endif

	return count;
}

static ssize_t xlat_addrs_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
#ifdef CONFIG_CP_DIT
	return sprintf(buf, "%pI6\n%pI6\n", &nf_linkfwd.ctun[0].v6.in.addr6, &nf_linkfwd.ctun[1].v6.in.addr6);
#else
	return sprintf(buf, "DIT NOT supported\n");
#endif
}

static ssize_t xlat_addrs_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf, size_t count)
{
#ifdef CONFIG_CP_DIT
	struct in6_addr val;
	char *ptr = NULL;

	mif_err("-- v6 addr: %s\n", buf);

	ptr = strstr(buf, "@");
	if (!ptr)
		return -EINVAL;
	*ptr++ = '\0';

	if (in6_pton(buf, strlen(buf), val.s6_addr, '\0', NULL) == 0)
		return -EINVAL;

	if (strstr(ptr, "rmnet0")) {
		nf_linkfwd.ctun[0].v6.in.addr6 = val;
		dit_set_clat_saddr(0, val);
		mif_err("clat rmnet0: %pI6\n", &nf_linkfwd.ctun[0].v6.in.addr6);
	} else if (strstr(ptr, "rmnet1")) {
		nf_linkfwd.ctun[1].v6.in.addr6 = val;
		dit_set_clat_saddr(1, val);
		mif_err("clat rmnet0: %pI6\n", &nf_linkfwd.ctun[1].v6.in.addr6);
	} else {
		mif_err("-- unhandled clat addr for device %s\n", ptr);
	}

	*(--ptr) = '@';
	(void)klat_addrs_store(kobj, attr, buf, count);
#else
	mif_err("DIT NOT supported\n");
#endif
	return count;
}

static ssize_t xlat_v4_addrs_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
#ifdef CONFIG_CP_DIT
	return sprintf(buf, "%pI4\n%pI4\n", &nf_linkfwd.ctun[0].v4.in.addr4, &nf_linkfwd.ctun[1].v4.in.addr4);
#else
	return sprintf(buf, "DIT NOT supported\n");
#endif
}

static ssize_t xlat_v4_addrs_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf, size_t count)
{
#ifdef CONFIG_CP_DIT
	struct in_addr val;
	char *ptr = NULL;

	mif_err("-- v4 addr: %s\n", buf);

	ptr = strstr(buf, "@");
	if (!ptr)
		return -EINVAL;
	*ptr++ = '\0';

	if (in4_pton(buf, strlen(buf), (u8 *)&val.s_addr, '\0', NULL) == 0)
		return -EINVAL;

	if (strstr(ptr, "rmnet0")) {
		nf_linkfwd.ctun[0].v4.in.addr4.s_addr = val.s_addr;
		mif_err("clat v4_rmnet0: %pI4\n", &nf_linkfwd.ctun[0].v4.in.addr4.s_addr);
		dit_set_clat_filter(0, val.s_addr);
	} else if (strstr(ptr, "rmnet1")) {
		nf_linkfwd.ctun[1].v4.in.addr4.s_addr = val.s_addr;
		mif_err("clat v4_rmnet1: %pI4\n", &nf_linkfwd.ctun[1].v4.in.addr4.s_addr);
		dit_set_clat_filter(1, val.s_addr);
	} else {
		mif_err("-- unhandled clat v4 addr for device %s\n", ptr);
	}

	*(--ptr) = '@';
	(void)klat_v4_addrs_store(kobj, attr, buf, count);
#else
	mif_err("DIT NOT supported\n");
#endif
	return count;
}
#endif

static DEVICE_ATTR_WO(do_cp_crash);
static DEVICE_ATTR_RO(modem_state);

#ifdef CONFIG_LINK_FORWARD
static DEVICE_ATTR_RO(linkforward_state);
static DEVICE_ATTR_RW(linkforward_mode);

static struct kobject *clat_kobject;
static struct kobj_attribute xlat_plat_attribute = {
	.attr = {.name = "xlat_plat", .mode = 0660},
	.show = xlat_plat_show,
	.store = xlat_plat_store,
};
static struct kobj_attribute xlat_addrs_attribute = {
	.attr = {.name = "xlat_addrs", .mode = 0660},
	.show = xlat_addrs_show,
	.store = xlat_addrs_store,
};
static struct kobj_attribute xlat_v4_addrs_attribute = {
	.attr = {.name = "xlat_v4_addrs", .mode = 0660},
	.show = xlat_v4_addrs_show,
	.store = xlat_v4_addrs_store,
};
static struct attribute *clat_attrs[] = {
	&xlat_plat_attribute.attr,
	&xlat_addrs_attribute.attr,
	&xlat_v4_addrs_attribute.attr,
	NULL,
};
ATTRIBUTE_GROUPS(clat);
#endif

static struct attribute *modem_attrs[] = {
	&dev_attr_do_cp_crash.attr,
	&dev_attr_modem_state.attr,
#ifdef CONFIG_LINK_FORWARD
	&dev_attr_linkforward_state.attr,
	&dev_attr_linkforward_mode.attr,
#endif
	NULL,
};
ATTRIBUTE_GROUPS(modem);

static int modem_probe(struct platform_device *pdev)
{
	int i;
	struct device *dev = &pdev->dev;
	struct modem_data *pdata = dev->platform_data;
	struct modem_shared *msd;
	struct modem_ctl *modemctl;
	struct io_device **iod;
	size_t size;
	struct link_device *ld;
	enum mif_sim_mode sim_mode;
	int err;

	mif_err("%s: +++ (%s)\n",
		pdev->name, CONFIG_OPTION_REGION);

	if (dev->of_node) {
		pdata = modem_if_parse_dt_pdata(dev);
		if (IS_ERR(pdata)) {
			mif_err("MIF DT parse error!\n");
			return PTR_ERR(pdata);
		}
	} else {
		if (!pdata) {
			mif_err("Non-DT, incorrect pdata!\n");
			return -EINVAL;
		}
	}

	msd = create_modem_shared_data(pdev);
	if (!msd) {
		mif_err("%s: msd == NULL\n", pdata->name);
		return -ENOMEM;
	}

	modemctl = create_modemctl_device(pdev, msd);
	if (!modemctl) {
		mif_err("%s: modemctl == NULL\n", pdata->name);
		devm_kfree(dev, msd);
		return -ENOMEM;
	}

	/* create link device */
	/* support multi-link device */
	for (i = 0; i < LINKDEV_MAX; i++) {
		/* find matching link type */
		if (pdata->link_types & LINKTYPE(i)) {
			ld = call_link_init_func(pdev, i);
			if (!ld)
				goto free_mc;

			mif_err("%s: %s link created\n", pdata->name, ld->name);

			ld->link_type = i;
			ld->mc = modemctl;
			ld->msd = msd;

			list_add(&ld->list, &msd->link_dev_list);
		}
	}

	sim_mode = get_sim_mode(dev->of_node);

	/* create io deivces and connect to modemctl device */
	size = sizeof(struct io_device *) * pdata->num_iodevs;
	iod = kzalloc(size, GFP_KERNEL);
	for (i = 0; i < pdata->num_iodevs; i++) {
		if (sim_mode < MIF_SIM_DUAL &&
			pdata->iodevs[i].attrs & IODEV_ATTR(ATTR_DUALSIM))
			continue;

		if (pdata->iodevs[i].attrs & IODEV_ATTR(ATTR_OPTION_REGION)
				&& strcmp(pdata->iodevs[i].option_region,
					CONFIG_OPTION_REGION))
			continue;

		iod[i] = create_io_device(pdev, &pdata->iodevs[i], msd,
					  modemctl, pdata);
		if (!iod[i]) {
			mif_err("%s: iod[%d] == NULL\n", pdata->name, i);
			goto free_iod;
		}

		if (iod[i]->format == IPC_FMT || iod[i]->format == IPC_BOOT
			|| iod[i]->id == SIPC_CH_ID_CASS)
			list_add_tail(&iod[i]->list,
					&modemctl->modem_state_notify_list);

		attach_devices(iod[i], pdata->iodevs[i].tx_link);
	}

	platform_set_drvdata(pdev, modemctl);

	/* reserved buff pool memory init */
	of_reserved_mem_device_init(dev);

#ifdef CONFIG_CP_RAM_LOGGING
	/* Send SMC call if cp_ram_logging function is enabled */
	if (shm_get_cplog_flag())
		security_request_cp_ram_logging();
#endif
	kfree(iod);

	if (sysfs_create_groups(&dev->kobj, modem_groups))
		mif_err("failed to create modem groups node\n");

	err = mif_init_argos_notifier();
	if (err < 0)
		mif_err("failed to initialize argos_notifier(%d)\n", err);

#ifdef CONFIG_LINK_FORWARD
	clat_kobject = kobject_create_and_add(KOBJ_CLAT, kernel_kobj); /* ipv4_kobject */
	if (!clat_kobject)
		mif_err("%s: done ---\n", KOBJ_CLAT);

	if (sysfs_create_groups(clat_kobject, clat_groups))
		mif_err("failed to create clat groups node\n");
#endif

	mif_err("%s: done ---\n", pdev->name);
	return 0;

free_iod:
	for (i = 0; i < pdata->num_iodevs; i++) {
		if (iod[i]) {
			sipc5_deinit_io_device(iod[i]);
			devm_kfree(dev, iod[i]);
		}
	}
	kfree(iod);

free_mc:
	if (modemctl)
		devm_kfree(dev, modemctl);

	if (msd)
		devm_kfree(dev, msd);

	mif_err("%s: xxx\n", pdev->name);
	return -ENOMEM;
}

static void modem_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct modem_ctl *mc = dev_get_drvdata(dev);

	mc->ops.modem_shutdown(mc);
	mc->phone_state = STATE_OFFLINE;

#ifdef CONFIG_LINK_FORWARD
	kobject_put(clat_kobject);
#endif

	mif_err("%s\n", mc->name);
}

static int modem_suspend(struct device *pdev)
{
	struct modem_ctl *mc = dev_get_drvdata(pdev);
#ifdef CONFIG_LINK_DEVICE_SHMEM
	struct modem_mbox *mbox = mc->mdm_data->mbx;
	struct utc_time t;
#endif

#if !defined(CONFIG_LINK_DEVICE_HSIC)
	if (mc->gpio_pda_active)
		gpio_set_value(mc->gpio_pda_active, 0);
#endif

#if defined(CONFIG_LINK_DEVICE_LLI) &&\
	!defined(CONFIG_LINK_POWER_MANAGEMENT_WITH_FSM)
	mipi_lli_suspend();
#endif

#ifdef CONFIG_LINK_DEVICE_SHMEM
	get_utc_time(&t);
	mif_err("time = %d.%d\n", t.sec + (t.min * 60), t.us);
	mbox_update_value(MCU_CP, mbox->mbx_ap2cp_kerneltime,
			t.sec + (t.min * 60),
			mbox->sbi_ap2cp_kerneltime_sec_mask,
			mbox->sbi_ap2cp_kerneltime_sec_pos);
	mbox_update_value(MCU_CP, mbox->mbx_ap2cp_kerneltime, t.us,
			mbox->sbi_ap2cp_kerneltime_usec_mask,
			mbox->sbi_ap2cp_kerneltime_usec_pos);

	mif_err("%s: pda_active:0\n", mc->name);
	mbox_update_value(MCU_CP, mc->mbx_ap_status, 0,
			mc->sbi_pda_active_mask, mc->sbi_pda_active_pos);
	mbox_set_interrupt(MCU_CP, mc->int_pda_active);
#endif

#ifdef CONFIG_LINK_DEVICE_PCIE
	if (mc->ops.modem_suspend)
		mc->ops.modem_suspend(mc);
#endif
	mif_err("%s\n", mc->name);
	set_wakeup_packet_log(true);

	return 0;
}

static int modem_resume(struct device *pdev)
{
	struct modem_ctl *mc = dev_get_drvdata(pdev);
#ifdef CONFIG_LINK_DEVICE_SHMEM
	struct modem_mbox *mbox = mc->mdm_data->mbx;
	struct utc_time t;
#endif

	set_wakeup_packet_log(false);

#if !defined(CONFIG_LINK_DEVICE_HSIC)
	if (mc->gpio_pda_active)
		gpio_set_value(mc->gpio_pda_active, 1);
#endif

#if defined(CONFIG_LINK_DEVICE_LLI) &&\
	!defined(CONFIG_LINK_POWER_MANAGEMENT_WITH_FSM)
	mipi_lli_resume();

	/* CP initiated resume case */
	if (gpio_get_value(mc->gpio_ap_wakeup)) {
		mif_err("gpio_ap_wakeup high\n");
		mipi_lli_set_link_status(LLI_WAITFORMOUNT);
		gpio_set_value(mc->gpio_ap_status, 1);
	}
#endif

#ifdef CONFIG_LINK_DEVICE_SHMEM
	get_utc_time(&t);
	mif_err("time = %d.%d\n", t.sec + (t.min * 60), t.us);
	mbox_update_value(MCU_CP, mbox->mbx_ap2cp_kerneltime,
			t.sec + (t.min * 60),
			mbox->sbi_ap2cp_kerneltime_sec_mask,
			mbox->sbi_ap2cp_kerneltime_sec_pos);
	mbox_update_value(MCU_CP, mbox->mbx_ap2cp_kerneltime, t.us,
			mbox->sbi_ap2cp_kerneltime_usec_mask,
			mbox->sbi_ap2cp_kerneltime_usec_pos);

	mif_err("%s: pda_active:1\n", mc->name);
	mbox_update_value(MCU_CP, mc->mbx_ap_status, 1,
			mc->sbi_pda_active_mask, mc->sbi_pda_active_pos);
	mbox_set_interrupt(MCU_CP, mc->int_pda_active);
#endif

#ifdef CONFIG_LINK_DEVICE_PCIE
	if (mc->ops.modem_resume)
		mc->ops.modem_resume(mc);
#endif

	mif_err("%s\n", mc->name);

	return 0;
}

#if defined(CONFIG_PM) && defined(CONFIG_LINK_DEVICE_PCIE)
static int modem_runtime_suspend(struct device *pdev)
{
	struct modem_ctl *mc = dev_get_drvdata(pdev);

	if (mc->ops.modem_runtime_suspend != NULL)
		mc->ops.modem_runtime_suspend(mc);

	return 0;
}

static int modem_runtime_resume(struct device *pdev)
{
	struct modem_ctl *mc = dev_get_drvdata(pdev);

	if (mc->ops.modem_runtime_resume != NULL)
		mc->ops.modem_runtime_resume(mc);

	return 0;
}
#endif



static const struct dev_pm_ops modem_pm_ops = {
#if defined(CONFIG_LINK_DEVICE_PCIE)
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(modem_suspend, modem_resume)
#else
	SET_SYSTEM_SLEEP_PM_OPS(modem_suspend, modem_resume)
#endif

#if defined(CONFIG_PM) && defined(CONFIG_LINK_DEVICE_PCIE)
	SET_RUNTIME_PM_OPS(modem_runtime_suspend, modem_runtime_resume, NULL)
#endif
};

static struct platform_driver modem_driver = {
	.probe = modem_probe,
	.shutdown = modem_shutdown,
	.driver = {
		.name = "mif_sipc5",
		.owner = THIS_MODULE,
		.pm = &modem_pm_ops,
		.suppress_bind_attrs = true,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(sec_modem_match),
#endif
	},
};

module_platform_driver(modem_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung Modem Interface Driver");
