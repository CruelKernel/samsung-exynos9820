/* drivers/media/tdmb/tdmb_qc_tsi.c
 *
 * Driver file for Qualcomm Transport Stream Interface
 *
 * Copyright (C) (2014, Samsung Electronics)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/qcom_tspp.h>

#include "tdmb.h"

#define CHANNEL_ID 0
#define TSPP_CHANNEL_TIMEOUT 100	/* 100ms */
#define TSPP_BUFFER_SIZE (20 * 1024) /* 20KB */
#define TDMB_TS_SIZE 188

struct tsi_pkt {
	struct list_head list;
	void *buf;
	u32 len;
};

struct tsi_dev {
	spinlock_t tsi_lock;
	struct list_head free_list;
	struct list_head full_list;

	int tsi_running;
	int filter_exists_flag;

	u8 packet_buff[TSPP_BUFFER_SIZE];
};

struct tsi_dev *tsi_priv;

void (*tsi_data_callback)(u8 *data, u32 length) = NULL;
static void tdmb_tsi_pull_data(struct work_struct *work);
static struct workqueue_struct *tdmb_tsi_workqueue;
static DECLARE_WORK(tdmb_tsi_work, tdmb_tsi_pull_data);

static struct tspp_select_source tdmb_tspp_set_source(void)
{
	struct tspp_select_source tspp_source;

	tspp_source.clk_inverse = 0;
	tspp_source.data_inverse = 0;
	tspp_source.sync_inverse = 0;
	tspp_source.enable_inverse = 0;
	tspp_source.mode = TSPP_TSIF_MODE_1;	/* whihout sync */
	tspp_source.source = TSPP_SOURCE_TSIF0;

	return tspp_source;
}

static struct tsi_pkt *tsi_get_pkt(struct tsi_dev *tsi, struct list_head *head)
{
	unsigned long flags;
	struct tsi_pkt *pkt;

	spin_lock_irqsave(&tsi->tsi_lock, flags);

	if (list_empty(head))	{
		/* DPRINTK("TSI %p list is null\n", head); */
		spin_unlock_irqrestore(&tsi->tsi_lock, flags);
		return NULL;
	}
	pkt = list_first_entry(head, struct tsi_pkt, list);
	spin_unlock_irqrestore(&tsi->tsi_lock, flags);

	return pkt;
}

static int tsi_setup_bufs(struct list_head *head, int packet_cnt, u8 *pkt_buff)
{
	struct tsi_pkt *pkt;
	u32 buf_size;
	u8 num_buf;
	int i;

	buf_size = TDMB_TS_SIZE * packet_cnt;
	num_buf = TSPP_BUFFER_SIZE / buf_size;

	for (i = 0; i < num_buf; i++) {
		pkt = kmalloc(sizeof(struct tsi_pkt), GFP_KERNEL);
		if (!pkt)
			return list_empty(head) ? -ENOMEM : 0;

		pkt->buf = (void *)(u8 *)(pkt_buff + i * buf_size);
		pkt->len = buf_size;
		list_add_tail(&pkt->list, head);
	}
	DPRINTK("total nodes calulated %d buf_size %d\n", num_buf, buf_size);

	return 0;
}

static void tsi_free_packets(struct tsi_dev *tsi)
{
	struct tsi_pkt *pkt;
	unsigned long flags;
	struct list_head *full = &tsi->full_list;
	struct list_head *head = &(tsi->free_list);

	spin_lock_irqsave(&tsi->tsi_lock, flags);
	/* move all the packets from full list to free list */
	while (!list_empty(full))	{
		pkt = list_entry(full->next, struct tsi_pkt, list);
		list_move_tail(&pkt->list, &tsi->free_list);
	}
	spin_unlock_irqrestore(&tsi->tsi_lock, flags);

	while (!list_empty(head))	{
		pkt = list_entry(head->next, struct tsi_pkt, list);
		list_del(&pkt->list);
		kfree(pkt);
	}
}

static bool tdmb_tsi_create_workqueue(void)
{
	tdmb_tsi_workqueue = create_singlethread_workqueue("ktdmbtsi");
	if (tdmb_tsi_workqueue)
		return true;
	else
		return false;
}

static bool tdmb_tsi_destroy_workqueue(void)
{
	if (tdmb_tsi_workqueue) {
		flush_workqueue(tdmb_tsi_workqueue);
		destroy_workqueue(tdmb_tsi_workqueue);
		tdmb_tsi_workqueue = NULL;
	}
	return true;
}


static void tdmb_tsi_pull_data(struct work_struct *work)
{
	struct tsi_pkt *pkt;
	unsigned long flags;
	const struct tspp_data_descriptor *tspp_data_desc;

	if (!tsi_priv->tsi_running) {
		DPRINTK("%s : tsi_running : %d\n",
			__func__, tsi_priv->tsi_running);
		return;
	}
	while ((tspp_data_desc = tspp_get_buffer(0, CHANNEL_ID)) != NULL) {

		pkt = tsi_get_pkt(tsi_priv, &tsi_priv->free_list);
		if (pkt == NULL)	{
			DPRINTK("TSI..No more free bufs..\n");
			tspp_release_buffer(0, CHANNEL_ID, tspp_data_desc->id);
			return;
		}

		if (tspp_data_desc->size == pkt->len)
			memcpy(pkt->buf, tspp_data_desc->virt_base, tspp_data_desc->size);
		else
			DPRINTK("Size err tspp_size:(%d) pkt_len:(%d)\n",
						tspp_data_desc->size, pkt->len);

		spin_lock_irqsave(&tsi_priv->tsi_lock, flags);
		list_move_tail(&pkt->list, &tsi_priv->full_list);
		spin_unlock_irqrestore(&tsi_priv->tsi_lock, flags);

		tspp_release_buffer(0, CHANNEL_ID, tspp_data_desc->id);
	}

	while ((pkt = tsi_get_pkt(tsi_priv, &tsi_priv->full_list)) != NULL) {
#ifdef CONFIG_TSI_LIST_DEBUG
		DPRINTK("full_list virt:0x%p length:%d\n", pkt->buf, pkt->len);
#endif
		if (tsi_data_callback)
			tsi_data_callback(pkt->buf, pkt->len);
		spin_lock_irqsave(&tsi_priv->tsi_lock, flags);
		list_move(&pkt->list, &tsi_priv->free_list);
		spin_unlock_irqrestore(&tsi_priv->tsi_lock, flags);
	}
}

static void tdmb_tspp_callback(int channel_id, void *user)
{
	if (!tsi_priv->tsi_running) {
		DPRINTK("%s : tsi_running : %d\n",
			__func__, tsi_priv->tsi_running);
		return;
	}
	if (tdmb_tsi_workqueue) {
		int ret;

		ret = queue_work(tdmb_tsi_workqueue, &tdmb_tsi_work);
		if (ret == 0)
			DPRINTK("failed in queue_work\n");
	}

}

static int tdmb_tspp_add_accept_all_filter(int channel_id,
				enum tspp_source source)
{
	struct tspp_filter tspp_filter;
	int ret;

	if (tsi_priv->filter_exists_flag) {
		DPRINTK("%s: accept all filter already exists\n",
				__func__);
		return 0;
	}

	tspp_filter.priority = 15;
	tspp_filter.pid = 0;
	tspp_filter.mask = 0;
	tspp_filter.mode = TSPP_MODE_RAW_NO_SUFFIX;
	tspp_filter.source = source;
	tspp_filter.decrypt = 0;

	ret = tspp_add_filter(0, channel_id, &tspp_filter);
	if (!ret) {
		tsi_priv->filter_exists_flag = 1;
		DPRINTK("accept all filter added successfully\n");
	}

	return ret;
}

static int tdmb_tspp_remove_accept_all_filter(int channel_id,
				enum tspp_source source)
{
	struct tspp_filter tspp_filter;
	int ret;

	if (tsi_priv->filter_exists_flag == 0) {
		DPRINTK("%s: accept all filter doesn't exist\n",
				__func__);
		return 0;
	}
	tspp_filter.priority = 15;

	ret = tspp_remove_filter(0, channel_id, &tspp_filter);
	if (!ret) {
		tsi_priv->filter_exists_flag = 0;
		DPRINTK("accept all filter removed successfully\n");
	}

	return ret;
}

int tdmb_tsi_start(void (*callback)(u8 *data, u32 length), int packet_cnt)
{
	struct tspp_select_source tspp_source = tdmb_tspp_set_source();

	int ret = 0;

	if (tsi_priv->tsi_running)
		return -1;

	tsi_priv->tsi_running = 1;

	if (tsi_setup_bufs(&tsi_priv->free_list, packet_cnt, tsi_priv->packet_buff)) {
		DPRINTK("TSI failed to setup pkt list");
		ret = -ENXIO;
		return ret;
	}

	if (tdmb_tsi_create_workqueue() == false) {
		DPRINTK("tdmb_tsi_create_workqueue fail\n");
		ret = ENXIO;
		goto err_create_workqueue;
	}
	tsi_data_callback = callback;

	ret = tspp_open_channel(0, CHANNEL_ID);
	if (ret < 0) {
		DPRINTK("%s: tspp_open_channel(%d) failed (%d)\n",
			__func__,
			CHANNEL_ID,
			ret);
		goto err_open_channel;
	}
	ret = tspp_open_stream(0, CHANNEL_ID, &tspp_source);
	if (ret < 0) {
		DPRINTK("%s: tspp_select_source(%d,%d) failed (%d)\n",
			__func__,
			CHANNEL_ID,
			tspp_source.source,
			ret);

		goto err_open_stream;
	}

	ret = tspp_register_notification(0,
						   CHANNEL_ID,
						   tdmb_tspp_callback,
						   NULL,
						   TSPP_CHANNEL_TIMEOUT); /* 100ms */
	if (ret < 0) {
		DPRINTK(
			"%s: tspp_register_notification(%d) failed (%d)\n",
			__func__, CHANNEL_ID, ret);

		goto err_channel_unregister_notif;
	}

	ret = tspp_allocate_buffers(0,
							CHANNEL_ID,
							TSPP_BUFFER_SIZE / TDMB_TS_SIZE,
							TDMB_TS_SIZE * packet_cnt,
							packet_cnt,  /* NOTIFICATION_SIZE */
							NULL, NULL, NULL);
	if (ret < 0) {
		DPRINTK(
			"%s: tspp_allocate_buffers(%d) failed (%d)\n",
			__func__, CHANNEL_ID, ret);

		goto err_allocate_buffers;
	}

	ret = tdmb_tspp_add_accept_all_filter(CHANNEL_ID,
						tspp_source.source);
	if (ret < 0) {
		DPRINTK(
			"%s: tdmb_tspp_add_accept_all_filter(%d, %d) failed\n",
			__func__, CHANNEL_ID, tspp_source.source);
		goto err_add_filter;
	}

	return ret;

err_add_filter:
err_allocate_buffers:
	tspp_unregister_notification(0, CHANNEL_ID);
err_channel_unregister_notif:
	tspp_close_stream(0, CHANNEL_ID);
err_open_stream:
	tspp_close_channel(0, CHANNEL_ID);
err_open_channel:
	tdmb_tsi_destroy_workqueue();
err_create_workqueue:
	tsi_free_packets(tsi_priv);

	tsi_priv->tsi_running = 0;

	return ret;
}
EXPORT_SYMBOL_GPL(tdmb_tsi_start);

int tdmb_tsi_stop(void)
{
	struct tspp_select_source tspp_source = tdmb_tspp_set_source();

	if (!tsi_priv->tsi_running)
		return -1;

	tsi_priv->tsi_running = 0;

	tdmb_tsi_destroy_workqueue();
	tsi_data_callback = NULL;
	tdmb_tspp_remove_accept_all_filter(CHANNEL_ID, tspp_source.source);
	tspp_unregister_notification(0, CHANNEL_ID);
	tspp_close_stream(0, CHANNEL_ID);
	tspp_close_channel(0, CHANNEL_ID);
	tsi_free_packets(tsi_priv);
	return 0;
}
EXPORT_SYMBOL_GPL(tdmb_tsi_stop);

int tdmb_tsi_init(void)
{
	tsi_priv = kmalloc(sizeof(struct tsi_dev), GFP_KERNEL);
	if (tsi_priv == NULL)	{
		DPRINTK("NO Memory for tsi allocation\n");
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&tsi_priv->full_list);
	INIT_LIST_HEAD(&tsi_priv->free_list);
	spin_lock_init(&tsi_priv->tsi_lock);

	tsi_priv->tsi_running = 0;
	tsi_priv->filter_exists_flag = 0;

	return 0;
}
EXPORT_SYMBOL_GPL(tdmb_tsi_init);

void tdmb_tsi_deinit(void)
{
	kfree(tsi_priv);
}
EXPORT_SYMBOL_GPL(tdmb_tsi_deinit);
