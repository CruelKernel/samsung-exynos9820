/*
 * drivers/media/platform/exynos/mfc/mfc_debug.c
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include "mfc_debugfs.h"
#include "mfc_sync.h"

#include "mfc_pm.h"

#include "mfc_queue.h"

unsigned int debug_level;
unsigned int debug_ts;
unsigned int dbg_enable;
unsigned int nal_q_dump;
unsigned int nal_q_disable;
unsigned int nal_q_parallel_disable;
unsigned int otf_dump;
unsigned int perf_measure_option;
unsigned int sfr_dump;
unsigned int mmcache_dump;
unsigned int mmcache_disable;
unsigned int perf_boost_mode;
unsigned int reg_test;

static int __mfc_info_show(struct seq_file *s, void *unused)
{
	struct mfc_dev *dev = s->private;
	struct mfc_ctx *ctx = NULL;
	int i;
	char *codec_name = NULL;

	seq_puts(s, ">> MFC device information(common)\n");
	seq_printf(s, "[VERSION] H/W: v%x.%x, F/W: %06x(%c), DRV: %d\n",
		 MFC_VER_MAJOR(dev), MFC_VER_MINOR(dev), dev->fw.date,
		 dev->fw.fimv_info, MFC_DRIVER_INFO);
	seq_printf(s, "[PM] power: %d, clock: %d, QoS level: %d\n",
			mfc_pm_get_pwr_ref_cnt(dev), mfc_pm_get_clk_ref_cnt(dev),
			atomic_read(&dev->qos_req_cur) - 1);
	seq_printf(s, "[CTX] num_inst: %d, num_drm_inst: %d, curr_ctx: %d(is_drm: %d)\n",
			dev->num_inst, dev->num_drm_inst, dev->curr_ctx, dev->curr_ctx_is_drm);
	seq_printf(s, "[HWLOCK] bits: %#lx, dev: %#lx, owned_by_irq = %d, wl_count = %d\n",
			dev->hwlock.bits, dev->hwlock.dev,
			dev->hwlock.owned_by_irq, dev->hwlock.wl_count);
	seq_printf(s, "[DEBUG MODE] %s\n", dev->pdata->debug_mode ? "enabled" : "disabled");
	seq_printf(s, "[MMCACHE] %s(%s)\n",
			dev->has_mmcache ? "supported" : "not supported",
			dev->mmcache.is_on_status ? "enabled" : "disabled");
	seq_printf(s, "[PERF BOOST] %s\n", perf_boost_mode ? "enabled" : "disabled");
	seq_printf(s, "[FEATURES] nal_q: %d(0x%x), skype: %d(0x%x), black_bar: %d(0x%x)\n",
			dev->pdata->nal_q.support, dev->pdata->nal_q.version,
			dev->pdata->skype.support, dev->pdata->skype.version,
			dev->pdata->black_bar.support, dev->pdata->black_bar.version);
	seq_printf(s, "           color_aspect_dec: %d(0x%x), enc: %d(0x%x)\n",
			dev->pdata->color_aspect_dec.support, dev->pdata->color_aspect_dec.version,
			dev->pdata->color_aspect_enc.support, dev->pdata->color_aspect_enc.version);
	seq_printf(s, "           static_info_dec: %d(0x%x), enc: %d(0x%x)\n",
			dev->pdata->static_info_dec.support, dev->pdata->static_info_dec.version,
			dev->pdata->static_info_enc.support, dev->pdata->static_info_enc.version);
	seq_printf(s, "[FORMATS] 10bit: %s, 422: %s, RGB: %s\n",
			dev->pdata->support_10bit ? "supported" : "not supported",
			dev->pdata->support_422 ? "supported" : "not supported",
			dev->pdata->support_rgb ? "supported" : "not supported");
	seq_printf(s, "[LOWMEM] is_low_mem: %d\n", IS_LOW_MEM);
	if (dev->nal_q_handle)
		seq_printf(s, "[NAL-Q] state: %d\n", dev->nal_q_handle->nal_q_state);

	seq_puts(s, ">> MFC device information(instance)\n");
	for (i = 0; i < MFC_NUM_CONTEXTS; i++) {
		ctx = dev->ctx[i];
		if (ctx) {
			if (ctx->type == MFCINST_DECODER)
				codec_name = ctx->src_fmt->name;
			else
				codec_name = ctx->dst_fmt->name;

			seq_printf(s, "[CTX:%d] %s %s, %s, %s, size: %dx%d@%ldfps(op: %ldfps), crop: %d %d %d %d, state: %d\n",
				ctx->num,
				ctx->type == MFCINST_DECODER ? "DEC" : "ENC",
				ctx->is_drm ? "Secure" : "Normal",
				ctx->state > MFCINST_INIT ? ctx->src_fmt->name : "undefined src fmt",
				ctx->state > MFCINST_INIT ? ctx->dst_fmt->name : "undefined dst fmt",
				ctx->img_width, ctx->img_height,
				ctx->last_framerate / 1000,
				ctx->operating_framerate,
				ctx->crop_width, ctx->crop_height,
				ctx->crop_left, ctx->crop_top, ctx->state);
			seq_printf(s, "        prio %d, rt %d, queue(src: %d, dst: %d, src_nal: %d, dst_nal: %d, ref: %d)\n",
				ctx->prio, ctx->rt,
				mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_queue),
				mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_queue),
				mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue),
				mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_nal_queue),
				mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->ref_buf_queue));
		}
	}

	return 0;
}

static int __mfc_debug_info_show(struct seq_file *s, void *unused)
{
	seq_puts(s, ">> MFC debug information\n");

	seq_puts(s, "-----SFR dump options (bit setting)\n");
	seq_puts(s, "ex) echo 0xff > /d/mfc/sfr_dump (all dump mode)\n");
	seq_puts(s, "1   (1 << 0): dec SEQ_START\n");
	seq_puts(s, "2   (1 << 1): dec INIT_BUFS\n");
	seq_puts(s, "4   (1 << 2): dec NAL_START\n");
	seq_puts(s, "8   (1 << 3): enc SEQ_START\n");
	seq_puts(s, "16  (1 << 4): enc INIT_BUFS\n");
	seq_puts(s, "32  (1 << 5): enc NAL_START\n");
	seq_puts(s, "64  (1 << 6): ERR interrupt\n");
	seq_puts(s, "128 (1 << 7): WARN interrupt\n");

	seq_puts(s, "-----Performance boost options (bit setting)\n");
	seq_puts(s, "ex) echo 7 > /d/mfc/perf_boost_mode (max freq)\n");
	seq_puts(s, "1   (1 << 0): DVFS (INT/MFC/MIF)\n");
	seq_puts(s, "2   (1 << 1): MO value\n");
	seq_puts(s, "4   (1 << 2): CPU frequency\n");

	return 0;
}

#ifdef CONFIG_MFC_REG_TEST
static int __mfc_reg_info_show(struct seq_file *s, void *unused)
{
	struct mfc_dev *dev = g_mfc_dev;
	unsigned int i;

	seq_puts(s, ">> MFC REG test(encoder)\n");

	seq_printf(s, "-----Register test on/off: %s\n", reg_test ? "on" : "off");
	seq_printf(s, "-----Register number: %d\n", dev->reg_cnt);

	if (dev->reg_val) {
		seq_puts(s, "-----Register values\n");
		for (i = 0; i < dev->reg_cnt; i++) {
			seq_printf(s, "%08x ", dev->reg_val[i]);
			if ((i % 8) == 7)
				seq_printf(s, "\n");
		}
	} else {
		seq_puts(s, "-----There is no reg_buf, set the register value\n");
	}

	return 0;
}

static ssize_t __mfc_reg_info_write(struct file *file, const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct mfc_dev *dev = g_mfc_dev;
	ssize_t len;
	unsigned int index = 0;
	char temp_char;

	if (!dev->reg_buf) {
		dev->reg_buf = kzalloc(SZ_1M, GFP_KERNEL);
		mfc_info_dev("[REGTEST] alloc reg_buf\n");
	}

	if (!dev->reg_val) {
		dev->reg_val = kzalloc(SZ_1M, GFP_KERNEL);
		mfc_info_dev("[REGTEST] alloc reg_val\n");
	}

	len = simple_write_to_buffer(dev->reg_buf, SZ_1M - 1,
					ppos, user_buf, count);
	if (len <= 0)
		return len;

	mfc_info_dev("[REGTEST] len: %d, ppos: %llu\n", len, *ppos);

	dev->reg_buf[*ppos] = '\0';

	dev->reg_cnt = 0;
	while (index + 7 < *ppos) {
		temp_char = dev->reg_buf[index];
		if ((temp_char >= '0' && temp_char <= '9') ||
				(temp_char >= 'A' && temp_char <= 'F') ||
				(temp_char >= 'a' && temp_char <= 'f')) {
			sscanf(&dev->reg_buf[index], "%08x", &dev->reg_val[dev->reg_cnt]);
			index += 8;
			dev->reg_cnt++;
		} else {
			index++;
		}
	}

	return len;
}
#endif

static int __mfc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, __mfc_info_show, inode->i_private);
}

static int __mfc_debug_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, __mfc_debug_info_show, inode->i_private);
}

#ifdef CONFIG_MFC_REG_TEST
static int __mfc_reg_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, __mfc_reg_info_show, inode->i_private);
}
#endif

static const struct file_operations mfc_info_fops = {
	.open = __mfc_info_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations debug_info_fops = {
	.open = __mfc_debug_info_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#ifdef CONFIG_MFC_REG_TEST
static const struct file_operations reg_info_fops = {
	.open = __mfc_reg_info_open,
	.read = seq_read,
	.write = __mfc_reg_info_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

void mfc_init_debugfs(struct mfc_dev *dev)
{
	struct mfc_debugfs *debugfs = &dev->debugfs;

	debugfs->root = debugfs_create_dir("mfc", NULL);
	if (!debugfs->root) {
		mfc_err_dev("debugfs: failed to create root directory\n");
		return;
	}

	debugfs->mfc_info = debugfs_create_file("mfc_info",
			0444, debugfs->root, dev, &mfc_info_fops);
	debugfs->debug_info = debugfs_create_file("debug_info",
			0444, debugfs->root, dev, &debug_info_fops);
#ifdef CONFIG_MFC_REG_TEST
	debugfs->reg_info = debugfs_create_file("reg_info",
			0644, debugfs->root, dev, &reg_info_fops);
	debugfs->reg_test = debugfs_create_u32("reg_test",
			0644, debugfs->root, &reg_test);
#endif
	debugfs->debug_level = debugfs_create_u32("debug",
			0644, debugfs->root, &debug_level);
	debugfs->debug_ts = debugfs_create_u32("debug_ts",
			0644, debugfs->root, &debug_ts);
	debugfs->dbg_enable = debugfs_create_u32("dbg_enable",
			0644, debugfs->root, &dbg_enable);
	debugfs->nal_q_dump = debugfs_create_u32("nal_q_dump",
			0644, debugfs->root, &nal_q_dump);
	debugfs->nal_q_disable = debugfs_create_u32("nal_q_disable",
			0644, debugfs->root, &nal_q_disable);
	debugfs->nal_q_parallel_disable = debugfs_create_u32("nal_q_parallel_disable",
			0644, debugfs->root, &nal_q_parallel_disable);
	debugfs->otf_dump = debugfs_create_u32("otf_dump",
			0644, debugfs->root, &otf_dump);
	debugfs->perf_measure_option = debugfs_create_u32("perf_measure_option",
			0644, debugfs->root, &perf_measure_option);
	debugfs->sfr_dump = debugfs_create_u32("sfr_dump",
			0644, debugfs->root, &sfr_dump);
	debugfs->mmcache_dump = debugfs_create_u32("mmcache_dump",
			0644, debugfs->root, &mmcache_dump);
	debugfs->mmcache_disable = debugfs_create_u32("mmcache_disable",
			0644, debugfs->root, &mmcache_disable);
	debugfs->perf_boost_mode = debugfs_create_u32("perf_boost_mode",
			0644, debugfs->root, &perf_boost_mode);
}
