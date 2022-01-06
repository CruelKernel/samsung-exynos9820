/*
 * copyright (c) 2017 Samsung Electronics Co., Ltd.
 * http://www.samsung.com
 *
 * Core file for Samsung EXYNOS TSMUX driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/exynos_iovmm.h>

#include "tsmux_dev.h"
#include "tsmux_reg.h"
#include "tsmux_dbg.h"

#define ORDERING_FIXED_VERSION		0x02010000

#define MAX_JOB_DONE_WAIT_TIME		1000000
#define AUDIO_TIME_PERIOD_US		21333

#define ADD_NULL_TS_PACKET

#ifdef ADD_NULL_TS_PACKET
#define TS_PKT_COUNT_PER_RTP    6
#else
#define TS_PKT_COUNT_PER_RTP    7
#endif

#define RTP_HEADER_SIZE     12
#define TS_PACKET_SIZE      188

#define WATCHDOG_INTERVAL		1000
#define MAX_WATCHDOG_TICK_CNT		5

static struct tsmux_device *g_tsmux_dev;
int g_tsmux_debug_level;
module_param(g_tsmux_debug_level, int, 0600);

static inline int get_pes_len(int src_len, bool hdcp, bool audio)
{
	int pes_len = 0;

	pes_len = src_len;
	pes_len += 14;
	if (hdcp)
		pes_len += 17;
	if (audio)
		pes_len += 2;
	return pes_len;
}

static inline int get_ts_len(int pes_len, bool psi)
{
	int ts_len = 0;

	if (psi)
		ts_len += 3 * 188;
	ts_len += (pes_len + 183) / 184 * 188;
	return ts_len;
}

static inline int get_rtp_len(int ts_len, int num_ts_per_rtp)
{
	int rtp_len = 0;

	rtp_len = (ts_len / (num_ts_per_rtp * 188) + 1) * 12 + ts_len;
	return rtp_len;
}

static inline int get_m2m_buffer_idx(int job_id)
{
	return job_id - 1;
}

static inline int get_m2m_job_id(int buffer_idx)
{
	return buffer_idx + 1;
}

static inline bool is_m2m_job_done(struct tsmux_context *ctx)
{
	int i;
	bool ret = true;

	for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
		if (ctx->m2m_job_done[i] == false)
			ret = false;
	}

	return ret;
}

static inline bool is_otf_job_done(struct tsmux_context *ctx)
{
	int i;
	bool ret = false;
	struct tsmux_device *tsmux_dev = ctx->tsmux_dev;
	unsigned long flags;

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		if (ctx->otf_outbuf_info[i].buf_state == BUF_DONE) {
			ret = true;
			break;
		}
	}

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	return ret;
}

static inline bool is_audio(u32 stream_type)
{
	bool is_audio = false;

	if (stream_type == TSMUX_AAC_STREAM_TYPE)
		is_audio = true;

	return is_audio;
}

static inline bool is_psi_invalid(int len)
{
	if (len < 0 || len >= TSMUX_PSI_SIZE * sizeof(int))
		return true;

	return false;
}

static int tsmux_iommu_fault_handler(
	struct iommu_domain *domain, struct device *dev,
	unsigned long fault_addr, int fault_flags, void *token)
{
	print_tsmux(TSMUX_ERR, "%s++\n", __func__);

	tsmux_sfr_dump();

	print_tsmux(TSMUX_ERR, "%s--\n", __func__);

	return 0;
}

static int increment_ts_continuity_counter(
	int ts_continuity_counter, int rtp_size,
	int ts_packet_count_in_rtp, bool psi_enable)
{
	int rtp_packet_count = rtp_size / (TS_PACKET_SIZE * ts_packet_count_in_rtp + RTP_HEADER_SIZE);
	int ts_packet_count = rtp_packet_count * ts_packet_count_in_rtp;
	int rtp_remain_size = rtp_size % (TS_PACKET_SIZE * ts_packet_count_in_rtp + RTP_HEADER_SIZE);
	int ts_ramain_size = 0;

	if (rtp_remain_size > 0) {
		ts_ramain_size = rtp_remain_size - RTP_HEADER_SIZE;
		ts_packet_count += ts_ramain_size / TS_PACKET_SIZE;
	}
	if (psi_enable)
		ts_packet_count -= 3;

	ts_continuity_counter += ts_packet_count;
	ts_continuity_counter = ts_continuity_counter & 0xf;

	return ts_continuity_counter;
}

static int increment_rtp_sequence_number(
	int rtp_seq_num, int rtp_size,
	int ts_packet_count_in_rtp)
{
	int rtp_packet_size = TS_PACKET_SIZE * ts_packet_count_in_rtp + RTP_HEADER_SIZE;
	int rtp_packet_count = rtp_size / rtp_packet_size;

	if (rtp_size % rtp_packet_size)
		rtp_packet_count++;
	rtp_packet_count += rtp_seq_num;
	rtp_packet_count = rtp_packet_count & 0xFFFF;

	return rtp_packet_count;
}

void tsmux_watchdog_tick_start(struct tsmux_device *tsmux_dev, int job_id)
{
	struct tsmux_watchdog_tick *watchdog_tick;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	if (job_id >= 0 && job_id < TSMUX_MAX_CMD_QUEUE_NUM) {
		watchdog_tick = &tsmux_dev->watchdog_tick[job_id];
		if (atomic_read(&watchdog_tick->watchdog_tick_running)) {
			print_tsmux(TSMUX_COMMON, "job id %d tick was already running\n", job_id);
		} else {
			print_tsmux(TSMUX_COMMON, "job id %d tick is now running\n", job_id);
			atomic_set(&watchdog_tick->watchdog_tick_running, 1);
		}

		/* Reset the timeout watchdog */
		atomic_set(&watchdog_tick->watchdog_tick_count, 0);
	} else
		print_tsmux(TSMUX_ERR, "invalid job id(%d)\n", job_id);

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);
}

void tsmux_watchdog_tick_stop(struct tsmux_device *tsmux_dev, int job_id)
{
	struct tsmux_watchdog_tick *watchdog_tick;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	if (job_id >= 0 && job_id < TSMUX_MAX_CMD_QUEUE_NUM) {
		watchdog_tick = &tsmux_dev->watchdog_tick[job_id];
		if (atomic_read(&watchdog_tick->watchdog_tick_running)) {
			print_tsmux(TSMUX_COMMON, "job id %d tick is now stopped\n", job_id);
			atomic_set(&watchdog_tick->watchdog_tick_running, 0);
		} else {
			print_tsmux(TSMUX_COMMON, "job id %d tick was already stopped\n", job_id);
		}

		/* Reset the timeout watchdog */
		atomic_set(&watchdog_tick->watchdog_tick_count, 0);
	} else
		print_tsmux(TSMUX_ERR, "invalid job id(%d)\n", job_id);

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);
}

void tsmux_watchdog_work_handler(struct work_struct *work)
{
	struct tsmux_device *tsmux_dev;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	tsmux_dev = container_of(work, struct tsmux_device, watchdog_work);

	tsmux_sfr_dump();

	/* If OTF job exists, MFC device driver generates kernel panic */
	/* Otherwise, TSMUX device driver generates kernel panic */
	if (atomic_read(&tsmux_dev->watchdog_tick[0].watchdog_tick_running) == 0)
		BUG();

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);
}

void tsmux_watchdog(struct timer_list *t)
{
	struct tsmux_device *tsmux_dev = from_timer(tsmux_dev, t, watchdog_timer);
	int i = 0;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	for (i = 0; i < TSMUX_MAX_CMD_QUEUE_NUM; i++) {
		if (atomic_read(&tsmux_dev->watchdog_tick[i].watchdog_tick_running))
			atomic_inc(&tsmux_dev->watchdog_tick[i].watchdog_tick_count);
		else
			atomic_set(&tsmux_dev->watchdog_tick[i].watchdog_tick_count, 0);

		if (atomic_read(&tsmux_dev->watchdog_tick[i].watchdog_tick_count) >= MAX_WATCHDOG_TICK_CNT) {
			/* TSMUX H/W is running, but interrupt was not generated */
			schedule_work(&tsmux_dev->watchdog_work);
		}
	}

	mod_timer(&tsmux_dev->watchdog_timer, jiffies + msecs_to_jiffies(WATCHDOG_INTERVAL));

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);
}

irqreturn_t tsmux_irq(int irq, void *priv)
{
	struct tsmux_device *tsmux_dev = priv;
	struct tsmux_context *ctx;
	int job_id;
	int dst_len;
	int i;
	ktime_t ktime;
	int64_t timestamp;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	tsmux_dev->irq = irq;
	ctx = tsmux_dev->ctx[tsmux_dev->ctx_cur];

	spin_lock(&tsmux_dev->device_spinlock);

	if (tsmux_is_job_done_id_0(tsmux_dev)) {
		job_id = 0;
		print_tsmux(TSMUX_COMMON, "Job ID %d is done\n", job_id);
		tsmux_clear_job_done(tsmux_dev, job_id);
		for (i = 0; i < TSMUX_OUT_BUF_CNT; i++)
			if (ctx->otf_outbuf_info[i].buf_state == BUF_Q)
				break;
		dst_len = tsmux_get_dst_len(tsmux_dev, job_id);
		print_tsmux(TSMUX_OTF, "otf outbuf num: %d, dst length: %d\n", i, dst_len);

		if (i < TSMUX_OUT_BUF_CNT) {
			ctx->otf_outbuf_info[i].buf_state = BUF_DONE;
			ctx->otf_cmd_queue.out_buf[i].actual_size = dst_len;

			ktime = ktime_get();
			timestamp = ktime_to_us(ktime);
			ctx->tsmux_end_stamp = timestamp;
		} else
			print_tsmux(TSMUX_ERR, "wrong index: %d\n", i);

		tsmux_watchdog_tick_stop(tsmux_dev, job_id);
		wake_up_interruptible(&ctx->otf_wait_queue);
	}

	if (tsmux_is_job_done_id_1(tsmux_dev)) {
		job_id = 1;
		print_tsmux(TSMUX_COMMON, "Job ID %d is done\n", job_id);
		tsmux_clear_job_done(tsmux_dev, job_id);
		ctx->m2m_job_done[get_m2m_buffer_idx(job_id)] = true;
		tsmux_watchdog_tick_stop(tsmux_dev, job_id);
	}

	if (tsmux_is_job_done_id_2(tsmux_dev)) {
		job_id = 2;
		print_tsmux(TSMUX_COMMON, "Job ID %d is done\n", job_id);
		tsmux_clear_job_done(tsmux_dev, job_id);
		ctx->m2m_job_done[get_m2m_buffer_idx(job_id)] = true;
		tsmux_watchdog_tick_stop(tsmux_dev, job_id);
	}

	if (tsmux_is_job_done_id_3(tsmux_dev)) {
		job_id = 3;
		print_tsmux(TSMUX_COMMON, "Job ID %d is done\n", job_id);
		tsmux_clear_job_done(tsmux_dev, job_id);
		ctx->m2m_job_done[get_m2m_buffer_idx(job_id)] = true;
		tsmux_watchdog_tick_stop(tsmux_dev, job_id);
	}

	spin_unlock(&tsmux_dev->device_spinlock);

	if (is_m2m_job_done(ctx)) {
		print_tsmux(TSMUX_COMMON, "wake_up_interruptible()\n");
		wake_up_interruptible(&ctx->m2m_wait_queue);
	}
	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return IRQ_HANDLED;
}

static int tsmux_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev = container_of(filp->private_data,
						struct tsmux_device, misc_dev);
	struct tsmux_context *ctx;
	unsigned long flags;
	int i;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	if (tsmux_dev->ctx_cnt >= TSMUX_MAX_CONTEXTS_NUM) {
		print_tsmux(TSMUX_ERR, "%s, too many context\n", __func__);
		ret = -ENOMEM;
		goto err_init;
	}

	ctx = kzalloc(sizeof(struct tsmux_context), GFP_KERNEL);
	if (!ctx) {
		ret = -ENOMEM;
		goto err_init;
	}

	/* init ctx */
	for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
		ctx->m2m_cmd_queue.m2m_job[i].in_buf.ion_buf_fd = -1;
		ctx->m2m_cmd_queue.m2m_job[i].out_buf.ion_buf_fd = -1;
	}

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++)
		ctx->otf_cmd_queue.out_buf[i].ion_buf_fd = -1;

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

	ctx->tsmux_dev = tsmux_dev;
	ctx->ctx_num = tsmux_dev->ctx_cnt;
	tsmux_dev->ctx[ctx->ctx_num] = ctx;
	tsmux_dev->ctx_cnt++;

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	pm_runtime_get_sync(tsmux_dev->dev);
	if (ret < 0) {
		print_tsmux(TSMUX_ERR, "pm_runtime_get_sync err(%d)\n", ret);
		return ret;
	}
#ifdef CLK_ENABLE
	ret = clk_enable(tsmux_dev->tsmux_clock);
	if (ret < 0) {
		print_tsmux(TSMUX_ERR, "clk_enable err (%d)\n", ret);
		return ret;
	}
#endif

	ctx->audio_frame_count = 0;
	ctx->video_frame_count = 0;
	ctx->set_hex_info = true;

	filp->private_data = ctx;
	print_tsmux(TSMUX_COMMON, "filp->private_data 0x%pK\n",
		filp->private_data);

	g_tsmux_dev = tsmux_dev;

	init_waitqueue_head(&ctx->m2m_wait_queue);
	init_waitqueue_head(&ctx->otf_wait_queue);

	ctx->otf_buf_mapped = false;

	//print_tsmux_sfr(tsmux_dev);
	//print_dbg_info_all(tsmux_dev);

	tsmux_reset_pkt_ctrl(tsmux_dev);

	//print_tsmux_sfr(tsmux_dev);
	//print_dbg_info_all(tsmux_dev);

	tsmux_dev->hw_version = tsmux_get_hw_version(tsmux_dev);

	if (tsmux_dev->ctx_cnt == 1)
		mod_timer(&tsmux_dev->watchdog_timer, jiffies + msecs_to_jiffies(1000));

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return ret;

err_init:
	print_tsmux(TSMUX_COMMON, "%s--, err_init\n", __func__);

	return ret;
}

static int tsmux_release(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct tsmux_context *ctx = filp->private_data;
	struct tsmux_device *tsmux_dev = ctx->tsmux_dev;
	unsigned long flags;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	tsmux_clear_hex_ctrl();

#ifdef CLK_ENABLE
	clk_disable(tsmux_dev->tsmux_clock);
#endif
	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);
	g_tsmux_dev = NULL;

	if (tsmux_dev->ctx_cnt == 1)
		del_timer(&tsmux_dev->watchdog_timer);

	ctx->tsmux_dev->ctx_cnt--;
	kfree(ctx);
	filp->private_data = NULL;

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	pm_runtime_put_sync(tsmux_dev->dev);
	if (ret < 0) {
		print_tsmux(TSMUX_ERR, "pm_runtime_put_sync err(%d)\n", ret);
		return ret;
	}

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);
	return ret;
}

int tsmux_set_info(struct tsmux_context *ctx,
		struct tsmux_swp_ctrl *swp_ctrl,
		struct tsmux_hex_ctrl *hex_ctrl)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	if (ctx == NULL || ctx->tsmux_dev == NULL)
		return -ENOMEM;

	tsmux_dev = ctx->tsmux_dev;

	/* set swap_ctrl reg*/
//	tsmux_set_swp_ctrl(tsmux_dev, swp_ctrl);

	/* secure OS will set hex regs */
	tsmux_set_hex_ctrl(ctx, hex_ctrl);

	/* enable interrupt */
	tsmux_enable_int_job_done(tsmux_dev);

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return ret;
}

int tsmux_job_queue(struct tsmux_context *ctx,
		struct tsmux_pkt_ctrl *pkt_ctrl,
		struct tsmux_pes_hdr *pes_hdr,
		struct tsmux_ts_hdr *ts_hdr,
		struct tsmux_rtp_hdr *rtp_hdr,
		int32_t src_len,
		struct tsmux_buffer_info *inbuf, struct tsmux_buffer_info *outbuf)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	if (ctx == NULL || ctx->tsmux_dev == NULL)
		return -ENOMEM;

	tsmux_dev = ctx->tsmux_dev;

	/* m2m only */
	if (pkt_ctrl->mode == 0) {
		if (IS_ERR(inbuf->dmabuf) || IS_ERR(inbuf->dmabuf_att) ||
			IS_ERR_VALUE(inbuf->dma_addr) || inbuf->dma_addr == 0) {
			print_tsmux(TSMUX_ERR, "tsmux_job_queue() inbuf is invalid\n");
			return -ENOMEM;
		}
	}

	if (IS_ERR(outbuf->dmabuf) || IS_ERR(outbuf->dmabuf_att) ||
		IS_ERR_VALUE(outbuf->dma_addr) || outbuf->dma_addr == 0) {
		print_tsmux(TSMUX_ERR, "tsmux_job_queue() outbuf is invalid\n");
		return -ENOMEM;
	}

	/* set pck_ctrl */
	tsmux_set_pkt_ctrl(tsmux_dev, pkt_ctrl);

	/* set pes_hdr */
	tsmux_set_pes_hdr(tsmux_dev, pes_hdr);

	/* set ts_hdr */
	tsmux_set_ts_hdr(tsmux_dev, ts_hdr);

	/* set rtp_hdr */
	tsmux_set_rtp_hdr(tsmux_dev, rtp_hdr);

	/* set src_addr_reg */
	tsmux_set_src_addr(tsmux_dev, inbuf);

	/* set src_len_reg */
	tsmux_set_src_len(tsmux_dev, src_len);

	/* set dst_addr_reg */
	tsmux_set_dst_addr(tsmux_dev, outbuf);

	/* set pkt_ctrl_reg */
	tsmux_job_queue_pkt_ctrl(tsmux_dev);

	tsmux_watchdog_tick_start(tsmux_dev, pkt_ctrl->id);

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return ret;
}

int tsmux_ioctl_m2m_map_buf(struct tsmux_context *ctx, int buf_fd, int buf_size,
	struct tsmux_buffer_info *buf_info)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev;
	unsigned long flags;
	struct dma_buf *temp_dmabuf;
	struct dma_buf_attachment *temp_dmabuf_att;
	dma_addr_t temp_dma_addr;

	print_tsmux(TSMUX_M2M, "%s++\n", __func__);

	if (ctx == NULL || ctx->tsmux_dev == NULL)
		return -ENOMEM;

	tsmux_dev = ctx->tsmux_dev;

	print_tsmux(TSMUX_M2M, "map m2m in_buf\n");

	temp_dmabuf = dma_buf_get(buf_fd);
	print_tsmux(TSMUX_M2M, "dma_buf_get(%d) ret dmabuf %pK\n",
		buf_fd, temp_dmabuf);

	if (IS_ERR(temp_dmabuf)) {
		temp_dmabuf_att = ERR_PTR(-EINVAL);
		print_tsmux(TSMUX_ERR, "m2m dma_buf_get() error\n");
		ret = -ENOMEM;
	} else {
		temp_dmabuf_att = dma_buf_attach(temp_dmabuf, tsmux_dev->dev);
		print_tsmux(TSMUX_M2M, "dma_buf_attach() ret dmabuf_att %pK\n",
			temp_dmabuf_att);
	}

	if (IS_ERR(temp_dmabuf_att)) {
		temp_dma_addr = -EINVAL;
		print_tsmux(TSMUX_ERR, "m2m dma_buf_attach() error\n");
		ret = -ENOMEM;
	} else {
		temp_dma_addr = ion_iovmm_map(temp_dmabuf_att, 0, buf_size,
			DMA_TO_DEVICE, 0);
		print_tsmux(TSMUX_M2M, "ion_iovmm_map() ret dma_addr_t 0x%llx\n",
			temp_dma_addr);
	}

	if (IS_ERR_VALUE(temp_dma_addr) || temp_dma_addr == 0) {
		print_tsmux(TSMUX_ERR, "m2m ion_iovmm_map() error\n");
		ret = -ENOMEM;
	}

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);
	buf_info->dmabuf = temp_dmabuf;
	buf_info->dmabuf_att = temp_dmabuf_att;
	buf_info->dma_addr = temp_dma_addr;
	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	print_tsmux(TSMUX_M2M, "%s--\n", __func__);

	return ret;
}

int tsmux_ioctl_m2m_unmap_buf(struct tsmux_context *ctx,
	struct tsmux_buffer_info *buf_info)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev;
	unsigned long flags;
	struct dma_buf *temp_dmabuf;
	struct dma_buf_attachment *temp_dmabuf_att;
	dma_addr_t temp_dma_addr;

	print_tsmux(TSMUX_M2M, "%s++\n", __func__);

	if (ctx == NULL || ctx->tsmux_dev == NULL)
		return -ENOMEM;

	tsmux_dev = ctx->tsmux_dev;

	print_tsmux(TSMUX_M2M, "unmap m2m in_buf\n");

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);
	temp_dma_addr = buf_info->dma_addr;
	temp_dmabuf_att = buf_info->dmabuf_att;
	temp_dmabuf = buf_info->dmabuf;
	buf_info->dma_addr = 0;
	buf_info->dmabuf_att = 0;
	buf_info->dmabuf = 0;
	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	if (!IS_ERR_VALUE(temp_dma_addr) && temp_dma_addr) {
		print_tsmux(TSMUX_M2M, "ion_iovmm_unmap(%pK, %llx)\n",
			temp_dmabuf_att, temp_dma_addr);
		ion_iovmm_unmap(temp_dmabuf_att, temp_dma_addr);
	}

	if (!IS_ERR_OR_NULL(temp_dmabuf_att)) {
		print_tsmux(TSMUX_M2M, "dma_buf_detach(%pK, %pK)\n",
			temp_dmabuf, temp_dmabuf_att);
		dma_buf_detach(temp_dmabuf, temp_dmabuf_att);
	}

	if (!IS_ERR_OR_NULL(temp_dmabuf)) {
		print_tsmux(TSMUX_M2M, "dma_buf_put(%pK)\n", temp_dmabuf);
		dma_buf_put(temp_dmabuf);
	}

	print_tsmux(TSMUX_M2M, "%s--\n", __func__);

	return ret;
}

int tsmux_ioctl_m2m_run(struct tsmux_context *ctx)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev;
	unsigned long flags;
	struct tsmux_job *m2m_job;
	int i = 0;
	int dst_len;
	int job_id;
	int cur_rtp_seq_num;
	int cur_ts_audio_cc;
	uint8_t *psi_data = NULL;
	int psi_validation = 0;
	int psi_len = 0;

	print_tsmux(TSMUX_M2M, "%s++\n", __func__);

	if (ctx == NULL)
		return -EFAULT;

	tsmux_dev = ctx->tsmux_dev;

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

	tsmux_dev->ctx_cur = ctx->ctx_num;

	/* init job_done[] */
	for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
		m2m_job = &ctx->m2m_cmd_queue.m2m_job[i];
		if (m2m_job->pes_hdr.pts39_16 != -1)
			ctx->m2m_job_done[i] = false;
		else
			ctx->m2m_job_done[i] = true;
		print_tsmux(TSMUX_M2M, "ctx->m2m_job_done[%d] %d\n",
			i, ctx->m2m_job_done[i]);
	}

	for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
		m2m_job = &ctx->m2m_cmd_queue.m2m_job[i];
		if (m2m_job->pes_hdr.pts39_16 != -1) {
			tsmux_set_info(ctx, &m2m_job->swp_ctrl, &m2m_job->hex_ctrl);

			psi_validation = 1;
			if (is_psi_invalid(ctx->psi_info.pat_len) ||
				is_psi_invalid(ctx->psi_info.pmt_len) ||
				is_psi_invalid(ctx->psi_info.pcr_len))
				psi_validation = 0;
			psi_len = ctx->psi_info.pat_len + ctx->psi_info.pmt_len + ctx->psi_info.pcr_len;
			if (is_psi_invalid(psi_len))
				psi_validation = 0;
			print_tsmux(TSMUX_M2M, "pkt_ctrl.psi_en %d, psi_validation %d\n",
				m2m_job->pkt_ctrl.psi_en, psi_validation);

			if (m2m_job->pkt_ctrl.psi_en && psi_validation) {
				/* PAT CC should be set by tsmux device driver */
				psi_data = (char *)ctx->psi_info.psi_data;
				psi_data[3] = psi_data[3] & 0xF0;
				psi_data[3] |= ctx->rtp_ts_info.ts_pat_cc & 0xF;
				print_tsmux(TSMUX_M2M, "ts pat %.2x %.2x %.2x %.2x, ts_pat_cc %.2x, pat_len %d\n",
					psi_data[0], psi_data[1], psi_data[2], psi_data[3],
					ctx->rtp_ts_info.ts_pat_cc, ctx->psi_info.pat_len);
				ctx->rtp_ts_info.ts_pat_cc++;
				if (ctx->rtp_ts_info.ts_pat_cc == 16)
					ctx->rtp_ts_info.ts_pat_cc = 0;
				psi_data += ctx->psi_info.pat_len;

				/* PMT CC should be set by tsmux device driver */
				psi_data[3] = psi_data[3] & 0xF0;
				psi_data[3] |= ctx->rtp_ts_info.ts_pmt_cc & 0xF;
				print_tsmux(TSMUX_M2M, "ts pmt %.2x %.2x %.2x %.2x, ts_pmt_cc %.2x, pmt_len %d\n",
					psi_data[0], psi_data[1], psi_data[2], psi_data[3],
					ctx->rtp_ts_info.ts_pmt_cc, ctx->psi_info.pmt_len);
				ctx->rtp_ts_info.ts_pmt_cc++;
				if (ctx->rtp_ts_info.ts_pmt_cc == 16)
					ctx->rtp_ts_info.ts_pmt_cc = 0;
				psi_data += ctx->psi_info.pmt_len;

				tsmux_set_psi_info(ctx->tsmux_dev, &ctx->psi_info);
			}

			if (ctx->rtp_ts_info.rtp_seq_override == 1) {
				m2m_job->rtp_hdr.seq = ctx->rtp_ts_info.rtp_seq_number;
				m2m_job->pkt_ctrl.rtp_seq_override = 1;
				m2m_job->ts_hdr.continuity_counter = ctx->rtp_ts_info.ts_audio_cc;
				ctx->rtp_ts_info.rtp_seq_override = 0;
				print_tsmux(TSMUX_COMMON, "m2m job_queue, rtp seq 0x%x\n",
					ctx->rtp_ts_info.rtp_seq_number);

			} else {
				m2m_job->pkt_ctrl.rtp_seq_override = 0;
			}
			m2m_job->ts_hdr.continuity_counter = ctx->rtp_ts_info.ts_audio_cc;
			m2m_job->pkt_ctrl.rtp_size = TS_PKT_COUNT_PER_RTP;
			print_tsmux(TSMUX_COMMON, "m2m job_queue, a_cc %.2x\n", ctx->rtp_ts_info.ts_audio_cc);

			ret = tsmux_job_queue(ctx, &m2m_job->pkt_ctrl,
				&m2m_job->pes_hdr, &m2m_job->ts_hdr,
				&m2m_job->rtp_hdr, m2m_job->in_buf.actual_size,
				&ctx->m2m_inbuf_info[i], &ctx->m2m_outbuf_info[i]);
			if (ret) {
				print_tsmux(TSMUX_ERR, "tsmux_job_queue() failed\n");
				break;
			}
		}
	}

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	wait_event_interruptible_timeout(ctx->m2m_wait_queue,
		is_m2m_job_done(ctx), usecs_to_jiffies(MAX_JOB_DONE_WAIT_TIME));

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

	for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
		m2m_job = &ctx->m2m_cmd_queue.m2m_job[i];
		if (m2m_job->pes_hdr.pts39_16 != -1) {
			m2m_job->out_buf.offset = 0;
			ctx->audio_frame_count++;
			m2m_job->out_buf.time_stamp =
				ctx->audio_frame_count * AUDIO_TIME_PERIOD_US;
			job_id = get_m2m_job_id(i);
			dst_len = tsmux_get_dst_len(tsmux_dev, job_id);
			m2m_job->out_buf.actual_size = dst_len;

			cur_ts_audio_cc = ctx->rtp_ts_info.ts_audio_cc;
			ctx->rtp_ts_info.ts_audio_cc = increment_ts_continuity_counter(
				cur_ts_audio_cc, dst_len,
				m2m_job->pkt_ctrl.rtp_size, m2m_job->pkt_ctrl.psi_en);

			cur_rtp_seq_num = ctx->rtp_ts_info.rtp_seq_number;
			ctx->rtp_ts_info.rtp_seq_number = increment_rtp_sequence_number(
				cur_rtp_seq_num, dst_len,
				m2m_job->pkt_ctrl.rtp_size);

			print_tsmux(TSMUX_COMMON, "m2m job_done, cur seq 0x%x, next seq 0x%x\n",
				cur_rtp_seq_num, ctx->rtp_ts_info.rtp_seq_number);
			print_tsmux(TSMUX_COMMON, "m2m job_done, cur a_cc 0x%x, next a_cc 0x%x\n",
				cur_ts_audio_cc, ctx->rtp_ts_info.ts_audio_cc);
			print_tsmux(TSMUX_M2M, "m2m %d, dst_len_reg %d",
				i, dst_len);
		}
	}

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	print_tsmux(TSMUX_M2M, "%s--\n", __func__);

	return ret;
}

int g2d_blending_start(int32_t index)
{
	int ret = 0;
	struct tsmux_context *ctx = NULL;
	ktime_t ktime;
	int64_t timestamp;

	ktime = ktime_get();
	timestamp = ktime_to_us(ktime);

	if (g_tsmux_dev == NULL) {
		print_tsmux(TSMUX_ERR, "tsmux was removed\n");
		ret = -1;
		return ret;
	}

	ctx = g_tsmux_dev->ctx[g_tsmux_dev->ctx_cur];

	ctx->g2d_start_stamp[index] = timestamp;
	print_tsmux(TSMUX_COMMON, "g2d_blending_start() index %d, start timestamp %lld\n",
		index, timestamp);

	return ret;
}

int g2d_blending_end(int32_t index)
{
	int ret = 0;
	struct tsmux_context *ctx = NULL;
	ktime_t ktime;
	int64_t timestamp;

	ktime = ktime_get();
	timestamp = ktime_to_us(ktime);

	if (g_tsmux_dev == NULL) {
		print_tsmux(TSMUX_ERR, "tsmux was removed\n");
		ret = -1;
		return ret;
	}

	ctx = g_tsmux_dev->ctx[g_tsmux_dev->ctx_cur];

	ctx->g2d_end_stamp[index] = timestamp;
	print_tsmux(TSMUX_COMMON, "g2d_blending_end() index %d, end timestamp %lld\n",
		index, timestamp);

	return ret;
}

int mfc_encoding_start(int32_t index)
{
	int ret = 0;
	struct tsmux_context *ctx = NULL;
	ktime_t ktime;
	int64_t timestamp;

	ktime = ktime_get();
	timestamp = ktime_to_us(ktime);

	if (g_tsmux_dev == NULL) {
		print_tsmux(TSMUX_ERR, "tsmux was removed\n");
		ret = -1;
		return ret;
	}

	ctx = g_tsmux_dev->ctx[g_tsmux_dev->ctx_cur];

	ctx->mfc_start_stamp = timestamp;
	ctx->mfc_encoding_index = index;

	print_tsmux(TSMUX_COMMON, "mfc_encoding_start() index %d, start timestamp %lld\n",
		ctx->mfc_encoding_index, timestamp);

	return ret;
}

int mfc_encoding_end(void)
{
	int ret = 0;
	struct tsmux_context *ctx = NULL;
	ktime_t ktime;
	int64_t timestamp;

	ktime = ktime_get();
	timestamp = ktime_to_us(ktime);

	if (g_tsmux_dev == NULL) {
		print_tsmux(TSMUX_ERR, "tsmux was removed\n");
		ret = -1;
		return ret;
	}

	ctx = g_tsmux_dev->ctx[g_tsmux_dev->ctx_cur];

	ctx->mfc_end_stamp = timestamp;
	print_tsmux(TSMUX_COMMON, "mfc_encoding_end() end timestamp %lld\n",
		timestamp);

	return ret;
}

int packetize(struct packetizing_param *param)
{
	int ret = 0;
	int index = -1;
	int i = 0;
	unsigned long flags;
	struct tsmux_context *ctx = NULL;
	struct tsmux_buffer_info *out_buf_info = NULL;
	struct tsmux_otf_config *config = NULL;
	uint8_t *psi_data = NULL;
	uint64_t pcr;
	uint64_t pcr_base;
	uint32_t pcr_ext;
	uint64_t pts = 0;
	ktime_t ktime;
	int64_t timestamp;
	bool otf_job_queued;
	int64_t wait_us;
	int psi_validation = 0;
	int psi_len = 0;

	ktime = ktime_get();
	timestamp = ktime_to_us(ktime);

	if (g_tsmux_dev == NULL) {
		print_tsmux(TSMUX_ERR, "tsmux was removed\n");
		ret = -1;
		return ret;
	}

	ctx = g_tsmux_dev->ctx[g_tsmux_dev->ctx_cur];

	wait_us = 0;
	do {
		spin_lock_irqsave(&g_tsmux_dev->device_spinlock, flags);

		otf_job_queued = ctx->otf_job_queued;

		spin_unlock_irqrestore(&g_tsmux_dev->device_spinlock, flags);

		if (otf_job_queued) {
			udelay(1000);
			wait_us += 1000;
		}

		if (wait_us > 10000) {
			print_tsmux(TSMUX_ERR, "otf_dq_buf is not finished\n");
			break;
		}

	} while (otf_job_queued);

	spin_lock_irqsave(&g_tsmux_dev->device_spinlock, flags);

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		out_buf_info = &ctx->otf_outbuf_info[i];
		if (out_buf_info->buf_state == BUF_Q) {
			print_tsmux(TSMUX_ERR, "otf command queue is full\n");
			ret = -1;
			spin_unlock_irqrestore(&g_tsmux_dev->device_spinlock, flags);
			return ret;
		}
	}

	if (ctx->otf_buf_mapped == false) {
		print_tsmux(TSMUX_ERR, "otf_buf_mapped is false\n");
		ret = -1;
		spin_unlock_irqrestore(&g_tsmux_dev->device_spinlock, flags);
		return ret;
	}

	if (ctx->otf_outbuf_info[0].dma_addr == 0) {
		print_tsmux(TSMUX_ERR, "otf_out_buf is NULL\n");
		ret = -1;
		spin_unlock_irqrestore(&g_tsmux_dev->device_spinlock, flags);
		return ret;
	}

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		out_buf_info = &ctx->otf_outbuf_info[i];
		if (out_buf_info->buf_state == BUF_FREE) {
			index = i;
			print_tsmux(TSMUX_OTF, "otf buf index is %d\n", index);
			break;
		}
	}

	if (index == -1) {
		print_tsmux(TSMUX_ERR, "otf buf full\n");
		ret = -1;
		spin_unlock_irqrestore(&g_tsmux_dev->device_spinlock, flags);
		return ret;
	}

	ctx->tsmux_start_stamp = timestamp;

	pts = (param->time_stamp * 9ll) / 100ll;
	config = &ctx->otf_cmd_queue.config;

	ctx->otf_psi_enabled[index] = ctx->otf_cmd_queue.config.pkt_ctrl.psi_en;

	psi_validation = 1;
	if (is_psi_invalid(ctx->psi_info.pat_len) ||
		is_psi_invalid(ctx->psi_info.pmt_len) ||
		is_psi_invalid(ctx->psi_info.pcr_len))
		psi_validation = 0;
	psi_len = ctx->psi_info.pat_len + ctx->psi_info.pmt_len + ctx->psi_info.pcr_len;
	if (is_psi_invalid(psi_len))
		psi_validation = 0;
	print_tsmux(TSMUX_OTF, "pkt_ctrl.psi_en %d, psi_validation %d\n",
		ctx->otf_cmd_queue.config.pkt_ctrl.psi_en, psi_validation);

	if (ctx->otf_cmd_queue.config.pkt_ctrl.psi_en == 1 && psi_validation) {
		/* PAT CC should be set by tsmux device driver */
		psi_data = (char *)ctx->psi_info.psi_data;
		psi_data[3] = psi_data[3] & 0xF0;
		psi_data[3] |= ctx->rtp_ts_info.ts_pat_cc & 0xF;
		print_tsmux(TSMUX_OTF, "ts pat %.2x %.2x %.2x %.2x, ts_pat_cc %.2x, pat_len %d\n",
			psi_data[0], psi_data[1], psi_data[2], psi_data[3],
			ctx->rtp_ts_info.ts_pat_cc, ctx->psi_info.pat_len);
		ctx->rtp_ts_info.ts_pat_cc++;
		if (ctx->rtp_ts_info.ts_pat_cc == 16)
			ctx->rtp_ts_info.ts_pat_cc = 0;
		psi_data += ctx->psi_info.pat_len;

		/* PMT CC should be set by tsmux device driver */
		psi_data[3] = psi_data[3] & 0xF0;
		psi_data[3] |= ctx->rtp_ts_info.ts_pmt_cc & 0xF;
		print_tsmux(TSMUX_OTF, "ts pmt %.2x %.2x %.2x %.2x, ts_pmt_cc %.2x, pmt_len %d\n",
			psi_data[0], psi_data[1], psi_data[2], psi_data[3],
			ctx->rtp_ts_info.ts_pmt_cc, ctx->psi_info.pmt_len);
		ctx->rtp_ts_info.ts_pmt_cc++;
		if (ctx->rtp_ts_info.ts_pmt_cc == 16)
			ctx->rtp_ts_info.ts_pmt_cc = 0;
		psi_data += ctx->psi_info.pmt_len;

		/* PCR should be set by tsmux device driver */
		pcr = param->time_stamp * 27;	// PCR based on a 27MHz clock
		pcr_base = pcr / 300;
		pcr_ext = pcr % 300;

		print_tsmux(TSMUX_OTF, "pcr header %.2x %.2x %.2x %.2x %.2x %.2x\n",
			psi_data[0], psi_data[1], psi_data[2],
			psi_data[3], psi_data[4], psi_data[5]);

		psi_data += 6;		// pcr ts packet header

		*psi_data++ = (pcr_base >> 25) & 0xff;
		*psi_data++ = (pcr_base >> 17) & 0xff;
		*psi_data++ = (pcr_base >> 9) & 0xff;
		*psi_data++ = ((pcr_base & 1) << 7) | 0x7e | ((pcr_ext >> 8) & 1);
		*psi_data++ = (pcr_ext & 0xff);

		tsmux_set_psi_info(ctx->tsmux_dev, &ctx->psi_info);
	}

	/* in case of otf, PTS should be set by tsmux device driver */
	config->pes_hdr.pts39_16 = (0x20 | (((pts >> 30) & 7) << 1) | 1) << 16;
	config->pes_hdr.pts39_16 |= ((pts >> 22) & 0xff) << 8;
	config->pes_hdr.pts39_16 |= ((((pts >> 15) & 0x7f) << 1) | 1);
	config->pes_hdr.pts15_0 = ((pts >> 7) & 0xff) << 8;
	config->pes_hdr.pts15_0 |= (((pts & 0x7f) << 1) | 1);

	tsmux_set_info(ctx, &config->swp_ctrl, &config->hex_ctrl);

	if (ctx->rtp_ts_info.rtp_seq_override == 1) {
		config->rtp_hdr.seq = ctx->rtp_ts_info.rtp_seq_number;
		config->pkt_ctrl.rtp_seq_override = 1;
		config->ts_hdr.continuity_counter = ctx->rtp_ts_info.ts_video_cc;
		ctx->rtp_ts_info.rtp_seq_override = 0;
		print_tsmux(TSMUX_COMMON, "otf job_queue, seq 0x%x\n",
			ctx->rtp_ts_info.rtp_seq_number);
	} else {
		config->pkt_ctrl.rtp_seq_override = 0;
	}
	config->ts_hdr.continuity_counter = ctx->rtp_ts_info.ts_video_cc;
	print_tsmux(TSMUX_COMMON, "otf job_queue, v_cc 0x%x\n",
		ctx->rtp_ts_info.ts_video_cc);

	ctx->otf_cmd_queue.config.pkt_ctrl.rtp_size = TS_PKT_COUNT_PER_RTP;
	ret = tsmux_job_queue(ctx,
			&config->pkt_ctrl,
			&config->pes_hdr,
			&config->ts_hdr,
			&config->rtp_hdr,
			0, 0, &ctx->otf_outbuf_info[index]);
	if (ret == 0) {
		ctx->otf_cmd_queue.out_buf[index].time_stamp = param->time_stamp;
		ctx->otf_outbuf_info[index].buf_state = BUF_Q;
		print_tsmux(TSMUX_OTF, "otf buf status: BUF_FREE -> BUF_Q, index: %d\n", index);
		ctx->otf_job_queued = true;
	}

	spin_unlock_irqrestore(&g_tsmux_dev->device_spinlock, flags);

	return ret;
}

void set_es_size(unsigned int size) {
	print_tsmux(TSMUX_OTF, "es_size: %d\n", size);
}

void tsmux_print_context_info(struct tsmux_context *ctx)
{
	int i = 0;
	struct tsmux_buffer_info *buf_info = NULL;
	struct tsmux_buffer *buf = NULL;

	if (ctx == NULL) {
		print_tsmux(TSMUX_ERR, "tsmux ctx is null\n");
		return;
	}

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		buf_info = &ctx->otf_outbuf_info[i];
		buf = &ctx->otf_cmd_queue.out_buf[i];
		print_tsmux(TSMUX_ERR,
				"otf_outbuf_info[%d] buf_state: %d\n",
				i, buf_info->buf_state);
		print_tsmux(TSMUX_ERR,
				"otf_outbuf_info[%d] fd: %d, buffer_size: %d, actual_size: %d, offset: %d\n",
				i, buf->ion_buf_fd, buf->buffer_size,
				buf->actual_size, buf->offset);
		print_tsmux(TSMUX_ERR,
				"otf_outbuf_info[%d] job_done: %d, part_done: %d, timestamp: %lld\n",
				i, buf->job_done, buf->partial_done, buf->time_stamp);
		print_tsmux(TSMUX_ERR,
				"otf_psi_enabled[%d]: %d\n", i, ctx->otf_psi_enabled[i]);
	}

	print_tsmux(TSMUX_ERR, "otf_job_queued: %d, set_hex_info: %d\n",
			ctx->otf_job_queued, ctx->set_hex_info);

	for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
		buf = &ctx->m2m_cmd_queue.m2m_job[i].in_buf;
		print_tsmux(TSMUX_ERR,
				"m2m_inbuf_info[%d] fd: %d, buffer_size: %d, actual_size: %d\n",
				i, buf->ion_buf_fd, buf->buffer_size, buf->actual_size);
		buf = &ctx->m2m_cmd_queue.m2m_job[i].out_buf;
		print_tsmux(TSMUX_ERR,
				"m2m_outbuf_info[%d] fd: %d, buffer_size: %d, actual_size: %d, time_stamp: %lld\n",
				i, buf->ion_buf_fd, buf->buffer_size, buf->actual_size, buf->time_stamp);
		print_tsmux(TSMUX_ERR,
				"m2m_job_done[%d]: %d\n", i, ctx->m2m_job_done[i]);
	}

	print_tsmux(TSMUX_ERR,
			"rtp_ts_info: rtp 0x%x, overr %d, pat_cc 0x%x, pmt_cc 0x%x, v_cc 0x%x, a_cc 0x%x\n",
			ctx->rtp_ts_info.rtp_seq_number, ctx->rtp_ts_info.rtp_seq_override,
			ctx->rtp_ts_info.ts_pat_cc, ctx->rtp_ts_info.ts_pmt_cc,
			ctx->rtp_ts_info.ts_video_cc, ctx->rtp_ts_info.ts_audio_cc);
	print_tsmux(TSMUX_ERR, "audio_frame_cnt: %lld, video_frame_cnt: %lld\n",
			ctx->audio_frame_count, ctx->video_frame_count);
}

void tsmux_sfr_dump(void)
{
	struct tsmux_context *ctx = NULL;
	struct tsmux_device *tsmux_dev = NULL;
	int prev_tsmux_debug_level;

	print_tsmux(TSMUX_ERR, "%s++\n", __func__);

	if (g_tsmux_dev == NULL) {
		print_tsmux(TSMUX_ERR, "g_tsmux_dev is null\n");
		return;
	}

	ctx = g_tsmux_dev->ctx[g_tsmux_dev->ctx_cur];
	if (ctx == NULL || ctx->tsmux_dev == NULL) {
		print_tsmux(TSMUX_ERR, "tsmux ctx is null\n");
		return;
	}

	tsmux_dev = ctx->tsmux_dev;
	prev_tsmux_debug_level = g_tsmux_debug_level;
	g_tsmux_debug_level |= TSMUX_SFR;
	g_tsmux_debug_level |= TSMUX_DBG_SFR;
	tsmux_print_tsmux_sfr(tsmux_dev);
	tsmux_print_dbg_info_all(tsmux_dev);
	tsmux_print_context_info(ctx);
	tsmux_print_cmu_mfc_sfr(tsmux_dev);
	g_tsmux_debug_level = prev_tsmux_debug_level;

	print_tsmux(TSMUX_ERR, "%s--\n", __func__);
}

static int get_job_done_buf(struct tsmux_context *ctx)
{
	struct tsmux_buffer_info *out_buf_info = NULL;
	struct tsmux_buffer *user_info = NULL;
	struct tsmux_device *tsmux_dev = ctx->tsmux_dev;
	unsigned long flags;
	int index = -1;
	int i = 0;

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		out_buf_info = &ctx->otf_outbuf_info[i];
		user_info = &ctx->otf_cmd_queue.out_buf[i];

		if (out_buf_info->buf_state == BUF_DONE) {
			if (index == -1)
				index = i;
			else {
				if (ctx->otf_cmd_queue.out_buf[index].time_stamp
						> user_info->time_stamp)
					index = i;
			}
		}
	}

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	return index;
}

void reordering_pes_private_data(char *packetized_data, bool psi)
{
	char *ptr = 0;
	char adaptation_field_control = 0;
	int adaptation_field_length = 0;
	int psi_packet = 3;
	int remain_ts_packet = 7;
	uint32_t PTS_DTS_flags = 0;
	uint32_t ESCR_flag = 0;
	uint32_t ES_rate_flag = 0;
	uint32_t DSM_trick_mode_flag = 0;
	uint32_t additional_copy_info_flag = 0;
	uint32_t PES_CRC_flag = 0;
	uint32_t PES_extension_flag = 0;
	uint32_t PES_private_data_flag = 0;
	uint32_t StreamCounter = 0;
	uint64_t InputCounter = 0;
	uint8_t PES_private_data[16] = {0};

	ptr = packetized_data;	/* ptr will point the rtp header */
	ptr += 12;  /* skip rtp header, ptr will point the ts header */
	while (remain_ts_packet > 0) {
		if (*ptr != 0x47u) {	/* check sync byte */
			print_tsmux(TSMUX_ERR, "wrong sync byte: 0x%x", *ptr);
			return;
		}

		ptr += 1; /* skip sync byte(8b) */
		ptr += 2; /* skip err(1b), start(1b), priority(1b), PID(13b) */
		adaptation_field_control = ((*ptr) >> 4) & 0x3;
		ptr += 1; /* skip scarmbling(2b), adaptation(2b), continuity counter(4b) */

		if (adaptation_field_control == 0x3) {
			adaptation_field_length = *ptr;
			ptr += 1; /* skip adaptation_field_length(8b) */
			ptr += adaptation_field_length; /* skip adaptation field */
		}

		if (psi && psi_packet > 0) {
			ptr += 184; /* skip ts payload */
			psi_packet--;
		} else {
			/* ptr points the pes header */
			ptr += 3; /* skip packet_startcode_prefix(24b) */
			ptr += 1; /* skip stream_id(8b) */
			ptr += 2; /* skip PES_packet_length(16b) */

			/* skip marker bit(2b), scrambling(2b),
			 * priority(1b), data_alignment_indicator(1b),
			 * copyright(1b), original_or_copy(1b)
			 */
			ptr += 1;

			PTS_DTS_flags = ((*ptr) >> 6) & 0x3;
			ESCR_flag = ((*ptr) >> 5) & 0x1;
			ES_rate_flag = ((*ptr) >> 4) & 0x1;
			DSM_trick_mode_flag = ((*ptr) >> 3) & 0x1;
			additional_copy_info_flag = ((*ptr) >> 2) & 0x1;
			PES_CRC_flag = ((*ptr) >> 1) & 0x1;
			PES_extension_flag = *ptr & 0x1;

			/* skip PTS_DTS_flags(2b), ESCR_flag(1b),
			 * ES_rate_flag(1b), DSM_trick_mode_flag(1b),
			 * additional_copy_info_flag(1b),
			 * PES_CRC_flag(1b), PES_extension_flag(1b)
			 */
			ptr += 1;

			ptr += 1; /* skip PES_header_data_length(8b) */

			if (PTS_DTS_flags == 2 || PTS_DTS_flags == 3) {
				ptr += 5; /* skip PTS(40b) */
				if (PTS_DTS_flags == 3)
					ptr += 5; /* skip DTS(40b) */
			}

			if (ESCR_flag)
				ptr += 6; /* skip ESCR(48b) */

			if (ES_rate_flag)
				ptr += 3; /* skip ES_rate(24b) */

			if (DSM_trick_mode_flag)
				ptr += 1; /* skip DSM_trick_mode(8b) */

			if (additional_copy_info_flag)
				ptr += 1; /* skip additional_copy_info(8b) */

			if (PES_CRC_flag)
				ptr += 2; /* skip PES_CRC(16b) */

			print_tsmux(TSMUX_OTF, "PES_extension_flag: %d\n", PES_extension_flag);
			if (PES_extension_flag) {
				PES_private_data_flag = ((*ptr) >> 7) & 0x1;

				/* skip PES_private_data_flag(1b),
				 * pack_header_field_flag(1b),
				 * program_packet_sequence_counter_flag(1b),
				 * P_STD_buffer_flag(1b),
				 * marker bit(3b),
				 * PES_entension2_flag(1)
				 */

				ptr += 1;

				if (PES_private_data_flag) {
					memcpy(PES_private_data, ptr, sizeof(PES_private_data));

					StreamCounter |= (((PES_private_data[1] >> 1) & 3) << 30) & 0xc0000000;
					StreamCounter |= (PES_private_data[2] << 22) & 0x3fc00000;
					StreamCounter |= (((PES_private_data[3] >> 1) & 0x7f) << 15) & 0x003f8000;
					StreamCounter |= (PES_private_data[4] << 7) & 0x00007f80;
					StreamCounter |= ((PES_private_data[5] >> 1) & 0x7f) & 0x0000007f;

					InputCounter |=
						((uint64_t)((PES_private_data[7] >> 1) & 0x0f) << 60)
						& 0xf000000000000000;
					InputCounter |=
						((uint64_t)PES_private_data[8] << 52)
						& 0x0ff0000000000000;
					InputCounter |=
						((uint64_t)((PES_private_data[9] >> 1) & 0x7f) << 45)
						& 0x000fe00000000000;
					InputCounter |=
						((uint64_t)PES_private_data[10] << 37)
						& 0x00001fe000000000;
					InputCounter |=
						((uint64_t)((PES_private_data[11] >> 1) & 0x7f) << 30)
						& 0x0000001fc0000000;
					InputCounter |=
						((uint64_t)PES_private_data[12] << 22)
						& 0x000000003fc00000;
					InputCounter |=
						((uint64_t)((PES_private_data[13] >> 1) & 0x7f) << 15)
						& 0x00000000003f8000;
					InputCounter |=
						((uint64_t)PES_private_data[14] << 7)
						& 0x0000000000007f80;
					InputCounter |=
						((uint64_t)((PES_private_data[15] >> 1) & 0x7f))
						& 0x000000000000007f;

					/* reordering stream counter */
					StreamCounter =
						(StreamCounter & 0xff000000) >> 24 |
						(StreamCounter & 0x00ff0000) >> 8 |
						(StreamCounter & 0x0000ff00) << 8 |
						(StreamCounter & 0x000000ff) << 24;

					/* reordering input counter */
					InputCounter =
						(InputCounter & 0xff00000000000000) >> 56 |
						(InputCounter & 0x00ff000000000000) >> 40 |
						(InputCounter & 0x0000ff0000000000) >> 24 |
						(InputCounter & 0x000000ff00000000) >> 8 |
						(InputCounter & 0x00000000ff000000) << 8 |
						(InputCounter & 0x0000000000ff0000) << 24 |
						(InputCounter & 0x000000000000ff00) << 40 |
						(InputCounter & 0x00000000000000ff) << 56;

					PES_private_data[0] = 0x00;
					PES_private_data[1] = (((StreamCounter >> 30) & 3) << 1) | 1;
					PES_private_data[2] = (StreamCounter >> 22) & 0xff;
					PES_private_data[3] = (((StreamCounter >> 15) & 0x7f) << 1) | 1;
					PES_private_data[4] = (StreamCounter >> 7) & 0xff;
					PES_private_data[5] = ((StreamCounter & 0x7f) << 1) | 1;
					PES_private_data[6] = 0x00;
					PES_private_data[7] = (((InputCounter >> 60) & 0x0f) << 1) | 1;
					PES_private_data[8] = (InputCounter >> 52) & 0xff;
					PES_private_data[9] = (((InputCounter >> 45) & 0x7f) << 1) | 1;
					PES_private_data[10] = (InputCounter >> 37) & 0xff;
					PES_private_data[11] = (((InputCounter >> 30) & 0x7f) << 1) | 1;
					PES_private_data[12] = (InputCounter >> 22) & 0xff;
					PES_private_data[13] = (((InputCounter >> 15) & 0x7f) << 1) | 1;
					PES_private_data[14] = (InputCounter >> 7) & 0xff;
					PES_private_data[15] = ((InputCounter & 0x7f) << 1) | 1;

					memcpy(ptr, PES_private_data, sizeof(PES_private_data));
				}
			}
			break;
		}
		remain_ts_packet -= 1;
	}
}

void add_null_ts_packet(uint8_t *ptr, int out_buf_size, struct tsmux_ts_hdr *ts_hdr)
{
	uint8_t payload_unit_start_indicator = 1;
	/* Adaptation_field, payload */
	uint8_t adapt_ctrl = 0x2;
	/* When the adaptation_field_control value is 'b10' */
	/* the value of the adaptation_field_length shall be 183 */
	uint8_t adapt_field_length = 183;
	uint8_t last_ts_continuity_counter = 0;
	uint32_t last_rtp_size = 0;
	uint8_t *ts_data = 0;

	last_ts_continuity_counter = *(ptr + out_buf_size - (TS_PACKET_SIZE - 3));
	last_ts_continuity_counter = last_ts_continuity_counter & 0xf;
	last_rtp_size = out_buf_size % (TS_PACKET_SIZE * TS_PKT_COUNT_PER_RTP + RTP_HEADER_SIZE);

	print_tsmux(TSMUX_COMMON, "out_buf_size %d\n", out_buf_size);
	print_tsmux(TSMUX_COMMON, "last_ts_continuity_counter %d, last_rtp_size %d\n",
			last_ts_continuity_counter, last_rtp_size);

	last_ts_continuity_counter++;
	if (last_ts_continuity_counter == 16)
		last_ts_continuity_counter = 0;

	ptr += out_buf_size;
	ts_data = ptr;
	*ptr++ = ts_hdr->sync;
	*ptr++ = ts_hdr->error << 7 | payload_unit_start_indicator << 6
		| ts_hdr->priority << 5 | (ts_hdr->pid >> 8);
	*ptr++ = ts_hdr->pid & 0xff;
	*ptr++ = ts_hdr->scramble << 6 | adapt_ctrl << 4 | last_ts_continuity_counter;
	*ptr++ = adapt_field_length;
	*ptr++ = 0x0; /* 8 flags */
	adapt_field_length -= 1;
	/* stuffing bytes */
	memset(ptr, 0xff, adapt_field_length);
	ptr += adapt_field_length;

	print_tsmux(TSMUX_COMMON, "ts data %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
			ts_data[0], ts_data[1], ts_data[2], ts_data[3],
			ts_data[4], ts_data[5], ts_data[6], ts_data[7]);

	print_tsmux(TSMUX_COMMON, "pes data %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
			ts_data[171], ts_data[172], ts_data[173], ts_data[174], ts_data[175],
			ts_data[176], ts_data[177], ts_data[178], ts_data[179]);
}

static bool tsmux_ioctl_otf_dq_buf(struct tsmux_context *ctx)
{
	long wait_time = 0;
	struct tsmux_device *tsmux_dev = ctx->tsmux_dev;
	unsigned long flags;
	int index = -1;
	int out_size = 0;
	int rtp_size = 0;
	int cur_rtp_seq_num;
	int cur_ts_video_cc;
	ktime_t ktime;
	int64_t timestamp;
	char *temp_p;
	int first_ts_video_cc;
	int last_ts_video_cc;

	while ((index = get_job_done_buf(ctx)) == -1) {
		wait_time = wait_event_interruptible_timeout(ctx->otf_wait_queue,
				is_otf_job_done(ctx), HZ / 10);
		print_tsmux(TSMUX_OTF, "dq buf wait_time: %ld\n", wait_time);
		if (wait_time <= 0)
			break;
	}

	if (wait_time > 0 || index != -1) {
		spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

		ctx->otf_job_queued = false;

		ctx->otf_cmd_queue.config.pkt_ctrl.rtp_size = TS_PKT_COUNT_PER_RTP;
		out_size = ctx->otf_cmd_queue.out_buf[index].actual_size;
		rtp_size = ctx->otf_cmd_queue.config.pkt_ctrl.rtp_size;

		first_ts_video_cc = ctx->rtp_ts_info.ts_video_cc;

		cur_ts_video_cc = ctx->rtp_ts_info.ts_video_cc;
		ctx->rtp_ts_info.ts_video_cc = increment_ts_continuity_counter(
			cur_ts_video_cc, out_size, rtp_size, ctx->otf_psi_enabled[index]);
#ifdef ADD_NULL_TS_PACKET
		// TSMUX_HAL will add null ts packet after end of frame
		ctx->rtp_ts_info.ts_video_cc++;
		if (ctx->rtp_ts_info.ts_video_cc == 16)
			ctx->rtp_ts_info.ts_video_cc = 0;
#endif
		cur_rtp_seq_num = ctx->rtp_ts_info.rtp_seq_number;
		ctx->rtp_ts_info.rtp_seq_number = increment_rtp_sequence_number(
			cur_rtp_seq_num, out_size,
			ctx->otf_cmd_queue.config.pkt_ctrl.rtp_size);

		temp_p = (char *)ctx->otf_outbuf_info[index].vaddr;
		temp_p += out_size - 188;
		last_ts_video_cc = (*(temp_p + 3) & 0xf);
		if (((last_ts_video_cc + 2) & 0xF) != ctx->rtp_ts_info.ts_video_cc) {
			print_tsmux(TSMUX_ERR, "1st cc %.2x last cc %.2x, cc %.2x, out_size %d, rtp_size %d, psi %d\n",
				first_ts_video_cc, last_ts_video_cc, ctx->rtp_ts_info.ts_video_cc,
				out_size, rtp_size, ctx->otf_psi_enabled[index]);
		}

		print_tsmux(TSMUX_COMMON, "otf job_done, last ts %.2x %.2x %.2x %.2x\n",
			*(temp_p), *(temp_p + 1), *(temp_p + 2), *(temp_p + 3));
		print_tsmux(TSMUX_COMMON, "otf job_done, cur rtp seq 0x%x, next rtp seq 0x%x\n",
			cur_rtp_seq_num, ctx->rtp_ts_info.rtp_seq_number);
		print_tsmux(TSMUX_COMMON, "otf job_done, cur v_cc 0x%x, next v_cc 0x%x\n",
			cur_ts_video_cc, ctx->rtp_ts_info.ts_video_cc);

		ctx->otf_cmd_queue.cur_buf_num = index;
		ctx->otf_outbuf_info[index].buf_state = BUF_DQ;
		print_tsmux(TSMUX_OTF, "dq buf index: %d\n", index);

		if (tsmux_dev->hw_version < ORDERING_FIXED_VERSION &&
				ctx->otf_cmd_queue.config.hex_ctrl.otf_enable) {
			print_tsmux(TSMUX_OTF, "reordering pes private data, hw_version: 0x%.8x\n",
					tsmux_dev->hw_version);
			reordering_pes_private_data(
					(char *)ctx->otf_outbuf_info[index].vaddr,
					ctx->otf_psi_enabled[index]);
		}

#ifdef ADD_NULL_TS_PACKET
	add_null_ts_packet((uint8_t *)ctx->otf_outbuf_info[index].vaddr,
		out_size, &ctx->otf_cmd_queue.config.ts_hdr);

	ctx->otf_cmd_queue.out_buf[index].actual_size += TS_PACKET_SIZE;
#endif

		ctx->video_frame_count++;
		spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);
	} else {
		print_tsmux(TSMUX_ERR, "time out: wait_time: %ld\n", wait_time);
		return false;
	}

	ctx->otf_cmd_queue.out_buf[index].g2d_start_stamp = ctx->g2d_start_stamp[ctx->mfc_encoding_index];
	ctx->otf_cmd_queue.out_buf[index].g2d_end_stamp = ctx->g2d_end_stamp[ctx->mfc_encoding_index];
	ctx->otf_cmd_queue.out_buf[index].mfc_start_stamp = ctx->mfc_start_stamp;
	ctx->otf_cmd_queue.out_buf[index].mfc_end_stamp = ctx->mfc_end_stamp;
	ctx->otf_cmd_queue.out_buf[index].tsmux_start_stamp = ctx->tsmux_start_stamp;
	ctx->otf_cmd_queue.out_buf[index].tsmux_end_stamp = ctx->tsmux_end_stamp;
	ktime = ktime_get();
	timestamp = ktime_to_us(ktime);
	ctx->otf_cmd_queue.out_buf[index].kernel_end_stamp = timestamp;

	print_tsmux(TSMUX_COMMON, "mfc_encoding_index %d, g2d %lld %lld\n",
		ctx->mfc_encoding_index,
		ctx->otf_cmd_queue.out_buf[index].g2d_start_stamp,
		ctx->otf_cmd_queue.out_buf[index].g2d_end_stamp);

	return true;
}

static bool tsmux_ioctl_otf_q_buf(struct tsmux_context *ctx)
{
	struct tsmux_device *tsmux_dev = ctx->tsmux_dev;
	unsigned long flags;
	int32_t index;

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

	index = ctx->otf_cmd_queue.cur_buf_num;
	if (index >= 0 && index < TSMUX_OUT_BUF_CNT) {
		if (ctx->otf_outbuf_info[index].buf_state == BUF_DQ) {
			ctx->otf_outbuf_info[index].buf_state = BUF_FREE;
			print_tsmux(TSMUX_OTF, "otf buf status: BUF_FREE, index: %d\n", index);
		} else {
			print_tsmux(TSMUX_ERR, "otf buf unexpected state: %d\n",
				ctx->otf_outbuf_info[index].buf_state);
		}
	} else {
		print_tsmux(TSMUX_ERR, "otf buf index is invalid %d\n", index);
	}

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	return true;
}

static int tsmux_ioctl_otf_map_buf(struct tsmux_context *ctx)
{
	int i = 0;
	int ret = 0;
	unsigned long flags;
	struct tsmux_buffer_info *out_buf_info = NULL;
	struct tsmux_buffer *user_info = NULL;
	struct tsmux_device *tsmux_dev = NULL;
	struct dma_buf *temp_dmabuf;
	struct dma_buf_attachment *temp_dmabuf_att;
	dma_addr_t temp_dma_addr;
	void *temp_vaddr;

	if (ctx == NULL || ctx->tsmux_dev == NULL)
		return -ENOMEM;

	tsmux_dev = ctx->tsmux_dev;

	print_tsmux(TSMUX_OTF, "map otf out_buf\n");

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		out_buf_info = &ctx->otf_outbuf_info[i];
		user_info = &ctx->otf_cmd_queue.out_buf[i];

		temp_dmabuf =
			dma_buf_get(user_info->ion_buf_fd);
		print_tsmux(TSMUX_OTF, "dma_buf_get(%d) ret dmabuf %pK\n",
			user_info->ion_buf_fd, temp_dmabuf);

		if (IS_ERR(temp_dmabuf)) {
			temp_dmabuf_att = ERR_PTR(-EINVAL);
			print_tsmux(TSMUX_ERR, "otf dma_buf_get() error\n");
			ret = -ENOMEM;
		} else {
			temp_dmabuf_att = dma_buf_attach(temp_dmabuf, tsmux_dev->dev);
			print_tsmux(TSMUX_OTF, "dma_buf_attach() ret dmabuf_att %pK\n",
				temp_dmabuf_att);
		}

		if (IS_ERR(temp_dmabuf_att)) {
			temp_dma_addr = -EINVAL;
			print_tsmux(TSMUX_ERR, "otf dma_buf_attach() error\n");
			ret = -ENOMEM;
		} else {
			temp_dma_addr = ion_iovmm_map(
				temp_dmabuf_att, 0, user_info->buffer_size, DMA_TO_DEVICE, 0);
			print_tsmux(TSMUX_OTF, "ion_iovmm_map() ret dma_addr_t 0x%llx\n",
				temp_dma_addr);
		}

		if (IS_ERR_VALUE(temp_dma_addr) || temp_dma_addr == 0) {
			temp_vaddr = NULL;
			print_tsmux(TSMUX_ERR, "otf ion_iovmm_map() error\n");
			ret = -ENOMEM;
		} else {
			temp_vaddr = dma_buf_vmap(temp_dmabuf);
			print_tsmux(TSMUX_OTF, "dma_buf_vmap(%pK) ret vaddr %pK\n",
				temp_dmabuf, temp_vaddr);
		}

		if (temp_vaddr == NULL) {
			print_tsmux(TSMUX_ERR, "otf dma_buf_vmap() error\n");
			ret = -ENOMEM;
		}

		spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);
		out_buf_info->dmabuf = temp_dmabuf;
		out_buf_info->dmabuf_att = temp_dmabuf_att;
		out_buf_info->dma_addr = temp_dma_addr;
		out_buf_info->vaddr = temp_vaddr;
		out_buf_info->buf_state = BUF_FREE;
		spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);
	}

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);
	ctx->otf_buf_mapped = true;
	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	return ret;
}

int tsmux_ioctl_otf_unmap_buf(struct tsmux_context *ctx)
{
	int i = 0;
	struct tsmux_buffer_info *out_buf_info = NULL;
	int ret = 0;
	unsigned long flags;
	bool otf_job_queued = false;
	int64_t wait_us = 0;
	struct dma_buf *temp_dmabuf;
	struct dma_buf_attachment *temp_dmabuf_att;
	dma_addr_t temp_dma_addr;
	void *temp_vaddr;

	print_tsmux(TSMUX_OTF, "%s++\n", __func__);

	if (ctx == NULL || ctx->tsmux_dev == NULL)
		return -ENOMEM;

	do {
		spin_lock_irqsave(&ctx->tsmux_dev->device_spinlock, flags);
		otf_job_queued = ctx->otf_job_queued;
		if (!otf_job_queued)
			ctx->otf_buf_mapped = false;
		spin_unlock_irqrestore(&ctx->tsmux_dev->device_spinlock, flags);
		if (otf_job_queued) {
			udelay(1000);
			wait_us += 1000;
			if (wait_us > 1000000) {
				print_tsmux(TSMUX_ERR, "%s, unmap buf failed\n", __func__);
				return -EBUSY;
			}
		}
	} while (otf_job_queued);

	/* free otf buffer */
	print_tsmux(TSMUX_OTF, "unmap otf out_buf\n");
	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		out_buf_info = &ctx->otf_outbuf_info[i];
		spin_lock_irqsave(&ctx->tsmux_dev->device_spinlock, flags);
		temp_dma_addr = out_buf_info->dma_addr;
		temp_dmabuf_att = out_buf_info->dmabuf_att;
		temp_dmabuf = out_buf_info->dmabuf;
		temp_vaddr = out_buf_info->vaddr;
		out_buf_info->dma_addr = 0;
		out_buf_info->dmabuf_att = 0;
		out_buf_info->dmabuf = 0;
		out_buf_info->vaddr = 0;
		spin_unlock_irqrestore(&ctx->tsmux_dev->device_spinlock, flags);

		if (temp_vaddr) {
			print_tsmux(TSMUX_OTF, "dma_buf_vunmap(%pK, %pK)\n",
					temp_dmabuf, temp_vaddr);
			dma_buf_vunmap(temp_dmabuf, temp_vaddr);
		}

		if (!IS_ERR_VALUE(temp_dma_addr) && temp_dma_addr) {
			print_tsmux(TSMUX_OTF, "ion_iovmm_unmmap(%pK, %llx)\n",
					temp_dmabuf_att, temp_dma_addr);
			ion_iovmm_unmap(temp_dmabuf_att, temp_dma_addr);
		}

		print_tsmux(TSMUX_OTF, "ion_iovmm_unmap() ok\n");

		if (!IS_ERR_OR_NULL(temp_dmabuf_att)) {
			print_tsmux(TSMUX_OTF, "dma_buf_detach(%pK, %pK)\n",
					temp_dmabuf, temp_dmabuf_att);
			dma_buf_detach(temp_dmabuf, temp_dmabuf_att);
		}

		print_tsmux(TSMUX_OTF, "dma_buf_detach() ok\n");

		if (!IS_ERR_OR_NULL(temp_dmabuf)) {
			print_tsmux(TSMUX_OTF, "dma_buf_put(%pK)\n", temp_dmabuf);
			dma_buf_put(temp_dmabuf);
		}

		print_tsmux(TSMUX_OTF, "dma_buf_put() ok\n");
	}

	print_tsmux(TSMUX_OTF, "%s--\n", __func__);

	return ret;
}

static long tsmux_ioctl(struct file *filp,
			    unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct tsmux_context *ctx;
	struct tsmux_device *tsmux_dev;
	int i = 0;
	int buf_fd = 0;
	int buf_size = 0;
	unsigned long flags;
	struct tsmux_psi_info temp_psi_info;
	struct tsmux_otf_config temp_otf_config;
	struct tsmux_m2m_cmd_queue temp_m2m_cmd_queue;
	struct tsmux_otf_cmd_queue temp_otf_cmd_queue;
	int32_t temp_cur_buf_num;
	struct tsmux_rtp_ts_info temp_rtp_ts_info;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	ctx = filp->private_data;
	if (!ctx) {
		ret = -ENOTTY;
		return ret;
	}

	tsmux_dev = ctx->tsmux_dev;

	switch (cmd) {
	case TSMUX_IOCTL_SET_INFO:
		print_tsmux(TSMUX_COMMON, "TSMUX_IOCTL_SET_PSI\n");

		if (copy_from_user(&temp_psi_info,
					(struct tsmux_psi_info __user *)arg,
					sizeof(struct tsmux_psi_info))) {
			ret = -EFAULT;
			break;
		}

		spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);
		memcpy(&ctx->psi_info, &temp_psi_info, sizeof(struct tsmux_psi_info));
		spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

		tsmux_set_psi_info(ctx->tsmux_dev, &ctx->psi_info);
	break;

	case TSMUX_IOCTL_M2M_MAP_BUF:
		print_tsmux(TSMUX_M2M, "TSMUX_IOCTL_M2M_MAP_BUF\n");

		if (copy_from_user(&temp_m2m_cmd_queue,
			(struct tsmux_m2m_cmd_queue __user *)arg,
			sizeof(struct tsmux_m2m_cmd_queue))) {
			ret = -EFAULT;
			break;
		}

		spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);
		memcpy(&ctx->m2m_cmd_queue, &temp_m2m_cmd_queue,
			sizeof(struct tsmux_m2m_cmd_queue));
		spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

		for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
			buf_fd = ctx->m2m_cmd_queue.m2m_job[i].in_buf.ion_buf_fd;
			buf_size = ctx->m2m_cmd_queue.m2m_job[i].in_buf.buffer_size;
			if (buf_fd > 0 && buf_size > 0) {
				tsmux_ioctl_m2m_map_buf(ctx, buf_fd, buf_size,
					&ctx->m2m_inbuf_info[i]);
			}
		}

		for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
			buf_fd = ctx->m2m_cmd_queue.m2m_job[i].out_buf.ion_buf_fd;
			buf_size = ctx->m2m_cmd_queue.m2m_job[i].out_buf.buffer_size;
			if (buf_fd > 0 && buf_size > 0) {
				tsmux_ioctl_m2m_map_buf(ctx, buf_fd, buf_size,
					&ctx->m2m_outbuf_info[i]);
			}
		}

		if (copy_to_user((struct tsmux_m2m_cmd_queue __user *)arg,
			&ctx->m2m_cmd_queue,
			sizeof(struct tsmux_m2m_cmd_queue))) {
			ret = -EFAULT;
			break;
		}
	break;

	case TSMUX_IOCTL_M2M_UNMAP_BUF:
		print_tsmux(TSMUX_M2M, "TSMUX_IOCTL_M2M_UNMAP_BUF\n");

		for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
			buf_fd = ctx->m2m_cmd_queue.m2m_job[i].in_buf.ion_buf_fd;
			buf_size = ctx->m2m_cmd_queue.m2m_job[i].in_buf.buffer_size;
			if (buf_fd > 0 && buf_size > 0) {
				tsmux_ioctl_m2m_unmap_buf(ctx,
					&ctx->m2m_inbuf_info[i]);
			}
		}

		for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
			buf_fd = ctx->m2m_cmd_queue.m2m_job[i].out_buf.ion_buf_fd;
			buf_size = ctx->m2m_cmd_queue.m2m_job[i].out_buf.buffer_size;
			if (buf_fd > 0 && buf_size > 0) {
				tsmux_ioctl_m2m_unmap_buf(ctx,
					&ctx->m2m_outbuf_info[i]);
			}
		}

	break;

	case TSMUX_IOCTL_M2M_RUN:
		print_tsmux(TSMUX_M2M, "TSMUX_IOCTL_M2M_RUN\n");

		if (copy_from_user(&temp_m2m_cmd_queue,
			(struct tsmux_m2m_cmd_queue __user *)arg,
			sizeof(struct tsmux_m2m_cmd_queue))) {
			ret = -EFAULT;
			break;
		}

		spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);
		memcpy(&ctx->m2m_cmd_queue, &temp_m2m_cmd_queue,
			sizeof(struct tsmux_m2m_cmd_queue));
		spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

		ret = tsmux_ioctl_m2m_run(ctx);

		if (copy_to_user((struct tsmux_m2m_cmd_queue __user *)arg,
			&ctx->m2m_cmd_queue,
			sizeof(struct tsmux_m2m_cmd_queue))) {
			ret = -EFAULT;
			break;
		}
	break;

	case TSMUX_IOCTL_OTF_MAP_BUF:
		print_tsmux(TSMUX_OTF, "TSMUX_IOCTL_OTF_MAP_BUF\n");
		if (copy_from_user(&temp_otf_cmd_queue,
					(struct tsmux_otf_cmd_queue __user *)arg,
					sizeof(struct tsmux_otf_cmd_queue))) {
			ret = -EFAULT;
			break;
		}

		spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);
		memcpy(&ctx->otf_cmd_queue, &temp_otf_cmd_queue,
			sizeof(struct tsmux_otf_cmd_queue));
		spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

		ret = tsmux_ioctl_otf_map_buf(ctx);

		if (copy_to_user((struct tsmux_otf_cmd_queue __user *) arg,
					&ctx->otf_cmd_queue, sizeof(struct tsmux_otf_cmd_queue))) {
			print_tsmux(TSMUX_ERR, "TSMUX_IOCTL_OTF_OTF_BUF: fail to copy_to_user\n");
			ret = -EFAULT;
			break;
		}
	break;

	case TSMUX_IOCTL_OTF_UNMAP_BUF:
		print_tsmux(TSMUX_OTF, "TSMUX_IOCTL_OTF_REL_BUF\n");
		tsmux_ioctl_otf_unmap_buf(ctx);
	break;

	case TSMUX_IOCTL_OTF_DQ_BUF:
		print_tsmux(TSMUX_OTF, "TSMUX_IOCTL_OTF_DQ_BUF\n");
		if (!tsmux_ioctl_otf_dq_buf(ctx)) {
			print_tsmux(TSMUX_ERR, "dq buf fail\n");
			ret = -EFAULT;
			break;
		}

		if (copy_to_user((struct tsmux_otf_cmd_queue __user *) arg,
					&ctx->otf_cmd_queue, sizeof(struct tsmux_otf_cmd_queue))) {
			print_tsmux(TSMUX_ERR, "TSMUX_IOCTL_OTF_DQ_BUF: fail to copy_to_user\n");
			ret = -EFAULT;
			break;
		}
	break;

	case TSMUX_IOCTL_OTF_Q_BUF:
		print_tsmux(TSMUX_OTF, "TSMUX_IOCTL_OTF_Q_BUF\n");
		if (copy_from_user(&temp_cur_buf_num,
					(int32_t *)arg,
					sizeof(int32_t))) {
			ret = -EFAULT;
			break;
		}

		spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);
		ctx->otf_cmd_queue.cur_buf_num = temp_cur_buf_num;
		spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

		if (!tsmux_ioctl_otf_q_buf(ctx))
			ret = -EFAULT;
	break;

	case TSMUX_IOCTL_OTF_SET_CONFIG:
		print_tsmux(TSMUX_OTF, "TSMUX_IOCTL_OTF_SET_CONFIG\n");

		if (copy_from_user(&temp_otf_config,
					(struct tsmux_otf_config __user *)arg,
					sizeof(struct tsmux_otf_config))) {
			ret = -EFAULT;
			break;
		}

		spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);
		memcpy(&ctx->otf_cmd_queue.config, &temp_otf_config, sizeof(struct tsmux_otf_config));
		spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);
	break;

	case TSMUX_IOCTL_SET_RTP_TS_INFO:
		if (copy_from_user(&temp_rtp_ts_info,
			(struct tsmux_rtp_ts_info __user *)arg,
			sizeof(struct tsmux_rtp_ts_info))) {
			ret = -EFAULT;
			break;
		}
		spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);
		memcpy(&ctx->rtp_ts_info, &temp_rtp_ts_info, sizeof(struct tsmux_rtp_ts_info));
		spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

		print_tsmux(TSMUX_COMMON, "set, rtp 0x%x, overr %d, pat_cc 0x%x, pmt_cc 0x%x, v_cc 0x%x, a_cc 0x%x\n",
			ctx->rtp_ts_info.rtp_seq_number, ctx->rtp_ts_info.rtp_seq_override,
			ctx->rtp_ts_info.ts_pat_cc, ctx->rtp_ts_info.ts_pmt_cc,
			ctx->rtp_ts_info.ts_video_cc, ctx->rtp_ts_info.ts_audio_cc);
	break;

	case TSMUX_IOCTL_GET_RTP_TS_INFO:
		print_tsmux(TSMUX_COMMON, "TSMUX_IOCTL_GET_RTP_TS_INFO\n");
		print_tsmux(TSMUX_COMMON, "get, rtp 0x%x, overr %d, pat_cc 0x%x, pmt_cc 0x%x, v_cc 0x%x, a_cc 0x%x\n",
			ctx->rtp_ts_info.rtp_seq_number, ctx->rtp_ts_info.rtp_seq_override,
			ctx->rtp_ts_info.ts_pat_cc, ctx->rtp_ts_info.ts_pmt_cc,
			ctx->rtp_ts_info.ts_video_cc, ctx->rtp_ts_info.ts_audio_cc);
		if (copy_to_user((struct tsmux_rtp_ts_info __user *) arg,
				&ctx->rtp_ts_info, sizeof(struct tsmux_rtp_ts_info))) {
			print_tsmux(TSMUX_ERR, "TSMUX_IOCTL_GET_RTP_TS_INFO: fail to copy_to_user\n");
			ret = -EFAULT;
			break;
		}
	break;

	default:
		ret = -ENOTTY;
	}

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);
	return ret;
}

static const struct file_operations tsmux_fops = {
	.owner          = THIS_MODULE,
	.open           = tsmux_open,
	.release        = tsmux_release,
	.unlocked_ioctl	= tsmux_ioctl,
	.compat_ioctl = tsmux_ioctl,
};

#ifdef CONFIG_EXYNOS_ITMON
static int tsmux_itmon_notifier(struct notifier_block *nb, unsigned long action, void *nb_data)
{
	struct tsmux_device *tsmux_dev;
	struct itmon_notifier *itmon_info = nb_data;
	int is_port = 0, is_master = 0, is_dest = 0;

	tsmux_dev = container_of(nb, struct tsmux_device, itmon_nb);

	if (IS_ERR_OR_NULL(itmon_info))
		return NOTIFY_DONE;

	if (itmon_info->port && strncmp("WFD", itmon_info->port, sizeof("WFD") - 1) == 0)
		is_port = 1;
	if (itmon_info->master && strncmp("WFD", itmon_info->master, sizeof("WFD") - 1) == 0)
		is_master = 1;
	if (itmon_info->dest && strncmp("WFD", itmon_info->dest, sizeof("WFD") - 1) == 0)
		is_dest = 1;

	if (is_port || is_master || is_dest) {
		tsmux_sfr_dump();
		return NOTIFY_BAD;
	} else {
		return NOTIFY_DONE;
	}
}
#endif

static int tsmux_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev;
	struct resource *res;
	int i;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	g_tsmux_debug_level = TSMUX_ERR;

	tsmux_dev = devm_kzalloc(&pdev->dev, sizeof(struct tsmux_device),
			GFP_KERNEL);
	if (!tsmux_dev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		print_tsmux(TSMUX_ERR, "failed to get memory region resource\n");
		ret = -ENOENT;
		goto err_res_mem;
	}

	tsmux_dev->tsmux_mem = request_mem_region(res->start,
					resource_size(res), pdev->name);
	if (tsmux_dev->tsmux_mem == NULL) {
		print_tsmux(TSMUX_ERR, "failed to get memory region\n");
		ret = -ENOENT;
		goto err_req_mem;
	}

	tsmux_dev->regs_base = ioremap(tsmux_dev->tsmux_mem->start,
				resource_size(tsmux_dev->tsmux_mem));
	if (tsmux_dev->regs_base == NULL) {
		print_tsmux(TSMUX_ERR, "failed to ioremap address region\n");
		ret = -ENOENT;
		goto err_ioremap;
	}

	tsmux_ioremap_cmu_mfc_sfr(tsmux_dev);

	pm_runtime_enable(&pdev->dev);
	if (ret < 0) {
		print_tsmux(TSMUX_ERR, "Failed to pm_runtime_enable (%d)\n", ret);
		return ret;
	}

	iovmm_set_fault_handler(&pdev->dev, tsmux_iommu_fault_handler,
		tsmux_dev);

	ret = iovmm_activate(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to activate iommu\n");
		goto err_iovmm_act;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		print_tsmux(TSMUX_ERR, "failed to get irq resource\n");
		ret = -ENOENT;
		goto err_res_irq;
	}

	tsmux_dev->irq = res->start;
	ret = devm_request_irq(&pdev->dev, res->start, tsmux_irq,
				0, pdev->name, tsmux_dev);
	if (ret != 0) {
		print_tsmux(TSMUX_ERR, "failed to install irq (%d)\n", ret);
		goto err_req_irq;
	}
#ifdef CLK_ENABLE
	tsmux_dev->tsmux_clock = devm_clk_get(tsmux_dev->dev, "gate");
	if (IS_ERR(tsmux_dev->tsmux_clock)) {
		dev_err(tsmux_dev->dev, "Failed to get clock (%ld)\n",
		PTR_ERR(tsmux_dev->tsmux_clock));
		return PTR_ERR(tsmux_dev->tsmux_clock);
	}
#endif

	spin_lock_init(&tsmux_dev->device_spinlock);

	dma_set_mask(&pdev->dev, DMA_BIT_MASK(36));

	tsmux_dev->ctx_cnt = 0;

	tsmux_dev->dev = &pdev->dev;
	tsmux_dev->misc_dev.minor = MISC_DYNAMIC_MINOR;
	tsmux_dev->misc_dev.fops = &tsmux_fops;
	tsmux_dev->misc_dev.name = NODE_NAME;
	ret = misc_register(&tsmux_dev->misc_dev);
	if (ret)
		goto err_misc_register;

	platform_set_drvdata(pdev, tsmux_dev);

#ifdef CONFIG_EXYNOS_ITMON
	tsmux_dev->itmon_nb.notifier_call = tsmux_itmon_notifier;
	itmon_notifier_chain_register(&tsmux_dev->itmon_nb);
#endif

	timer_setup(&tsmux_dev->watchdog_timer, tsmux_watchdog, 0);
	INIT_WORK(&tsmux_dev->watchdog_work, tsmux_watchdog_work_handler);
	for (i = 0; i < TSMUX_MAX_CMD_QUEUE_NUM; i++) {
		atomic_set(&tsmux_dev->watchdog_tick[i].watchdog_tick_running, 0);
		atomic_set(&tsmux_dev->watchdog_tick[i].watchdog_tick_count, 0);
	}

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return ret;

err_misc_register:
	print_tsmux(TSMUX_ERR, "err_misc_dev\n");

err_req_irq:
err_res_irq:
err_iovmm_act:
err_ioremap:
err_req_mem:
err_res_mem:

	return ret;
}

static int tsmux_remove(struct platform_device *pdev)
{
	struct tsmux_device *tsmux_dev = platform_get_drvdata(pdev);

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	if (tsmux_dev == NULL)
		return -EFAULT;

	iovmm_deactivate(tsmux_dev->dev);

	free_irq(tsmux_dev->irq, tsmux_dev);

	if (tsmux_dev->regs_base_cmu_mfc)
		iounmap(tsmux_dev->regs_base_cmu_mfc);

	iounmap(tsmux_dev->regs_base);

	if (tsmux_dev) {
		misc_deregister(&tsmux_dev->misc_dev);
		kfree(tsmux_dev);
	}

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return 0;
}

static void tsmux_shutdown(struct platform_device *pdev)
{
	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);
}

static const struct of_device_id exynos_tsmux_match[] = {
	{
		.compatible = "samsung,exynos-tsmux",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_tsmux_match);

static struct platform_driver tsmux_driver = {
	.probe		= tsmux_probe,
	.remove		= tsmux_remove,
	.shutdown	= tsmux_shutdown,
	.driver = {
		.name	= MODULE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(exynos_tsmux_match),
	}
};

module_platform_driver(tsmux_driver);

MODULE_AUTHOR("Shinwon Lee <shinwon.lee@samsung.com>");
MODULE_DESCRIPTION("EXYNOS tsmux driver");
MODULE_LICENSE("GPL");
