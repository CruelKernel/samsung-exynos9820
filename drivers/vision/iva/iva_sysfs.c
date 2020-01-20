/* iva_sysfs.c
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Authors:
 *	Ilkwan Kim <ilkwan.kim@samsung.com>
 *	Hoon Choi <hoon98.choi@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/sysfs.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/pm_opp.h>
#include <linux/devfreq.h>
#include "iva_ctrl_ioctl.h"
#include "iva_ctrl.h"
#include "iva_mem.h"
#include "iva_ipc_queue.h"
#include "iva_ipc_header.h"

static int iva_sysfs_print(char *buf, size_t buf_sz, const char *fmt, ...)
{
	int	strlen;
	va_list	ap;

	va_start(ap, fmt);
	strlen = vsnprintf(buf, buf_sz, fmt, ap);
	va_end(ap);

	return strlen;
}

#define IVA_SYSFS_PRINT0(buf, buf_size, tmp_buf, tmp_size, format, ...)		\
	do {									\
		tmp_size = iva_sysfs_print(tmp_buf, sizeof(tmp_buf),		\
						format, ## __VA_ARGS__);	\
		if ((buf_size + tmp_size + 1) <= PAGE_SIZE) {			\
			strncat(buf, tmp_buf, tmp_size);			\
			buf_size += tmp_size;					\
		}								\
	} while (0)

#define IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size, format, ...)		\
	do {									\
		tmp_size = iva_sysfs_print(tmp_buf, sizeof(tmp_buf),		\
						format, ## __VA_ARGS__);	\
		if ((buf_size + tmp_size + 1) > PAGE_SIZE)			\
			goto exit;	/* no space */				\
		strncat(buf, tmp_buf, tmp_size);				\
		buf_size += tmp_size;						\
	} while (0)


static ssize_t iva_mem_proc_show(struct device *dev,
		struct device_attribute *dev_attr, char *buf)
{
	struct iva_dev_data	*iva = dev_get_drvdata(dev);
	struct iva_mem_map	*iva_map_node;
	struct iva_proc		*iva_proc_node;
	struct list_head	*work_node_parent;
	int			i = 0;
	ssize_t			buf_size = 0;
	char			tmp_buf[256];
	int			tmp_size;

	mutex_lock(&iva->proc_mutex);
	list_for_each(work_node_parent, &iva->proc_head) {
		iva_proc_node = list_entry(work_node_parent,
				struct iva_proc, proc_node);

		IVA_SYSFS_PRINT0(buf, buf_size, tmp_buf, tmp_size,
			"------- show iva mapped list of pid(%2d) -------\n",
			iva_proc_node->pid);

		mutex_lock(&iva_proc_node->mem_map_lock);
		if (hash_empty(iva_proc_node->h_mem_map)) {
			IVA_SYSFS_PRINT0(buf, buf_size, tmp_buf, tmp_size,
				" (No mem_map in this process) ");
			mutex_unlock(&iva_proc_node->mem_map_lock);
			continue;
		}

		hash_for_each(iva_proc_node->h_mem_map, i, iva_map_node, h_node) {
			IVA_SYSFS_PRINT0(buf, buf_size, tmp_buf, tmp_size,
				"[%d] buf_fd:0x%08x,\tflags:0x%x,\tio_va:0x%08lx,\tsize:0x%08x, 0x%08x\tref_cnt:%d\n",
				i,
				iva_map_node->shared_fd,
				iva_map_node->flags,
				iva_map_node->io_va,
				iva_map_node->req_size, iva_map_node->act_size,
				iva_mem_map_read_refcnt(iva_map_node));
			i++;
		}

		mutex_unlock(&iva_proc_node->mem_map_lock);
		IVA_SYSFS_PRINT0(buf, buf_size, tmp_buf, tmp_size,
			"------------------------------------------------\n");
	}
	mutex_unlock(&iva->proc_mutex);

	return buf_size;
}


static ssize_t iva_ipcq_show_trans(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iva_dev_data	*iva = dev_get_drvdata(dev);
	struct iva_ipcq_stat	*ipcq_stat = &iva->mcu_ipcq.ipcq_stat;
	struct list_head	*work_node;
	struct iva_ipcq_trans	*trans_item;
	unsigned long		rem_nsec;
	u64			ts_ns;
	int			i = 0;
	ssize_t			buf_size = 0;
	char			tmp_buf[256];
	int			tmp_size;

	IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
			"------- ipc queue transactions(max nr:%d, nr: %d) -------\n",
			ipcq_stat->trans_nr_max, ipcq_stat->trans_nr);

	list_for_each(work_node, &ipcq_stat->trans_head) {
		trans_item = list_entry(work_node, struct iva_ipcq_trans, node);

		ts_ns = trans_item->ts_nsec;
		rem_nsec = do_div(ts_ns, 1000000000);
		IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
				"(%03d) [%05lu.%06lu] ", i,
				(unsigned long) ts_ns,
				rem_nsec / 1000);

		/* TO DO */
		if (trans_item->send) {
			struct ipc_cmd_param *ipc_msg_c = &trans_item->cmd_param;

			IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
					"--> ");

			switch (ipc_msg_c->ipc_cmd_type) {
			case IPC_CMD_SEND_TABLE:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"SCHEDULE_TABLE\n");
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tTSS: 0x%08X\tsize(0x%X, %d)\n",
						ipc_msg_c->data.p.rt_table,
						ipc_msg_c->data.p.rt_table_size,
						ipc_msg_c->data.p.rt_table_size);

				break;
			case IPC_CMD_FRAME_START:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"FRAME_START\n");
				break;
			case IPC_CMD_FRAME_DONE:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"FRAME_DONE\n");
				break;
			case IPC_CMD_NODE_START:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"NODE_START\n");
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tTABLE ID: 0x%08X\tGUID: 0x%08X\n",
						ipc_msg_c->data.m.table_id,
						ipc_msg_c->data.m.guid);
				break;
			case IPC_CMD_NODE_DONE:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"NODE_DONE\n");
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tTABLE ID(0x%08X)\tGUID(0x%08X)\n",
					ipc_msg_c->data.m.table_id,
					ipc_msg_c->data.m.guid);
				break;
			case IPC_CMD_EVICT_TABLE:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"EVICT\n");
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tTABLE ID: 0x%08X\n",
						ipc_msg_c->data.m.table_id);
				break;
			default:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"cmd type(%d)\n",
						ipc_msg_c->ipc_cmd_type);
			}
		} else {
			struct ipc_res_param *ipc_msg_r = &trans_item->res_param;

			IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
					"<-- ");
			switch (ipc_msg_r->ipc_cmd_type) {
			case IPC_CMD_DEINIT:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"DEINIT\n");
				break;
			case IPC_CMD_SEND_TABLE_ACK:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"SCHEDULE_TABLE_ACK\n");
				break;
			case IPC_CMD_FRAME_DONE:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"FRAME_DONE\n");
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tTABLE ID(0x%08X)\tGUID(0x%08X)\n",
					ipc_msg_r->data.m.table_id,
					ipc_msg_r->data.m.guid);
				break;
			case IPC_CMD_NODE_DONE:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"NODE_DONE\n");
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tTABLE ID(0x%08X)\tGUID(0x%08X)\n",
					ipc_msg_r->data.m.table_id,
					ipc_msg_r->data.m.guid);
				break;
			case IPC_CMD_EVICT_TABLE_ACK:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"EVICT_ACK\n");
				break;
			case IPC_CMD_TIMEOUT_NOTIFY:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"TIMEOUT_NOTIFY\n");
				break;
			default:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"cmd type(%d)\n",
						ipc_msg_r->ipc_cmd_type);
			}
		}
		i++;
	}
	IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
			"------------------------------------------------------\n");
exit:
	return buf_size;
}

static ssize_t iva_ipcq_set_trans(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
#define MAX_TRANSACTION_SAVA_NR		(40)
	struct iva_dev_data	*iva = dev_get_drvdata(dev);
	struct iva_ipcq_stat	*ipcq_stat = &iva->mcu_ipcq.ipcq_stat;
	struct list_head	*work_node;
	struct list_head	*tmp_node;
	struct iva_ipcq_trans	*trans_item;
	int		value;
	int		err;

	err = kstrtoint(buf, 10, &value);
	if (err) {
		dev_err(dev, "only decimal value allowed, but(%s)\n", buf);
		return err;
	}

	if (value > MAX_TRANSACTION_SAVA_NR)
		value = MAX_TRANSACTION_SAVA_NR;
	else if (value <= 0)
		value = 0;	/* disable */

	/* clear */
	mutex_lock(&ipcq_stat->trans_mtx);
	list_for_each_safe(work_node, tmp_node,
			&ipcq_stat->trans_head) {
		if (ipcq_stat->trans_nr <= value)
			break;
		trans_item = list_entry(work_node,
			struct iva_ipcq_trans, node);
		list_del(work_node);
		devm_kfree(dev, trans_item);
		ipcq_stat->trans_nr--;
	}
	mutex_unlock(&ipcq_stat->trans_mtx);

	ipcq_stat->trans_nr_max = (uint32_t) value;
	dev_info(dev, "max ipcq trans %d\n", ipcq_stat->trans_nr_max);
	return count;
}

static ssize_t iva_mcu_log_ctrl_show(struct device *dev,
		struct device_attribute *dev_attr, char *buf)
{
	struct iva_dev_data	*iva = dev_get_drvdata(dev);
	ssize_t			buf_size = 0;
	char			tmp_buf[256];
	int			tmp_size;

	buf[0] = 0;
	IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
			"mcu_print_delay (%d usecs)\n",
			iva->mcu_print_delay);
exit:
	return buf_size;
}

static ssize_t iva_mcu_log_ctrl(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
#define MAX_TRANSACTION_SAVA_NR		(40)
	struct iva_dev_data	*iva = dev_get_drvdata(dev);
	int		value;
	int		err;

	err = kstrtoint(buf, 10, &value);
	if (err) {
		dev_err(dev, "only decimal value allowed, but(%s)\n", buf);
		return count;
	}

	if (value <= 0)
		value = 0;

	iva->mcu_print_delay = (uint32_t) value;

	return count;
}

static ssize_t iva_sysfs_show_qos(struct device *dev,
		struct device_attribute *dev_attr, char *buf)
{
	struct iva_dev_data	*iva = dev_get_drvdata(dev);
	ssize_t			buf_size = 0;
	char			tmp_buf[256];
	int			tmp_size;
	bool			hit = false;

	buf[0] = 0;
	if (iva->iva_dvfs_dev) {
		struct device *dev = &iva->iva_dvfs_dev->dev;
		struct dev_pm_opp *opp;
		unsigned long freq = 0;

		do {
			opp = dev_pm_opp_find_freq_ceil(dev, &freq);
			if (IS_ERR(opp))
				break;

			if (freq == iva->iva_qos_rate)
				hit = true;

			IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
					"%lu%s ", freq,
					(freq == iva->iva_qos_rate) ? "(*)" : "");
			freq++;
			dev_pm_opp_put(opp);
		} while (1);
	}

	if (hit) {
		IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size, "\n");

	} else {
		IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
				"iva qos rate (%d Khz)\n", iva->iva_qos_rate);
	}

exit:
	return buf_size;
}

static ssize_t iva_sysfs_set_qos(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct iva_dev_data	*iva = dev_get_drvdata(dev);
	bool		hit = false;
	int		value;
	int		err;

	err = kstrtoint(buf, 10, &value);
	if (err) {
		dev_err(dev, "only decimal value allowed, but(%s)\n", buf);
		return count;
	}

	if (iva->iva_dvfs_dev) {
		struct device *dev = &iva->iva_dvfs_dev->dev;
		struct dev_pm_opp *opp;
		unsigned long freq = 0;

		do {
			opp = dev_pm_opp_find_freq_ceil(dev, &freq);
			if (IS_ERR(opp))
				break;

			if (freq == value) {
				hit = true;
				dev_pm_opp_put(opp);
				break;
			}

			dev_pm_opp_put(opp);
			freq++;
		} while (1);
	}

	if (hit)
		iva->iva_qos_rate = (int) value;
	else
		dev_warn(dev, "fail to set qos rate, req(%dKhz), cur(%dKhz)\n",
				value, iva->iva_qos_rate);

	return count;
}

static DEVICE_ATTR(iva_mem_proc, 0444,
			iva_mem_proc_show, NULL);
static DEVICE_ATTR(ipcq_trans, 0644,
			iva_ipcq_show_trans, iva_ipcq_set_trans);
static DEVICE_ATTR(mcu_log, 0644,
			iva_mcu_log_ctrl_show, iva_mcu_log_ctrl);
static DEVICE_ATTR(qos_rate, 0644,
			iva_sysfs_show_qos, iva_sysfs_set_qos);

static struct attribute *iva_mem_attributes[] = {
	&dev_attr_iva_mem_proc.attr,
	&dev_attr_ipcq_trans.attr,
	&dev_attr_mcu_log.attr,
	&dev_attr_qos_rate.attr,
	NULL,
};

static struct attribute_group iva_attr_group = {
	.attrs = iva_mem_attributes,
};

int32_t iva_sysfs_init(struct device *dev)
{
	return sysfs_create_group(&dev->kobj, &iva_attr_group);
}

void iva_sysfs_deinit(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &iva_attr_group);
}
