// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/bug.h>

#include "panel.h"
#include "panel_bl.h"
#include "panel_drv.h"
#include "panel_poc.h"


static u8 *poc_wr_img;
static u8 *poc_rd_img;

const char * const poc_op[] = {
	[POC_OP_NONE] = "POC_OP_NONE",
	[POC_OP_ERASE] = "POC_OP_ERASE",
	[POC_OP_WRITE] = "POC_OP_WRITE",
	[POC_OP_READ] = "POC_OP_READ",
	[POC_OP_CANCEL] = "POC_OP_CANCEL",
	[POC_OP_CHECKSUM] = "POC_OP_CHECKSUM",
	[POC_OP_CHECKPOC] = "POC_OP_CHECKPOC",
	[POC_OP_SECTOR_ERASE] = "POC_OP_SECTOR_ERASE",
	[POC_OP_BACKUP] = "POC_OP_BACKUP",
	[POC_OP_IMG_WRITE] = "POC_OP_IMG_WRITE",
	[POC_OP_IMG_READ] = "POC_OP_IMG_READ",
	[POC_OP_IMG_READ_TEST] = "POC_OP_IMG_READ_TEST",
	[POC_OP_DIM_WRITE] = "POC_OP_DIM_WRITE",
	[POC_OP_DIM_READ] = "POC_OP_DIM_READ",
	[POC_OP_DIM_CHKSUM] = "POC_OP_DIM_CHKSUM",
	[POC_OP_DIM_VALID] = "POC_OP_DIM_VALID",
	[POC_OP_DIM_READ_TEST] = "POC_OP_DIM_READ_TEST",
	[POC_OP_SELF_TEST] = "POC_OP_SELF_TEST",
	[POC_OP_READ_TEST] = "POC_OP_READ_TEST",
	[POC_OP_WRITE_TEST] = "POC_OP_WRITE_TEST",
	[POC_OP_DIM_READ_FROM_FILE] = "POC_OP_DIM_READ_FROM_FILE",
	[POC_OP_MTP_READ] = "POC_OP_MTP_READ",
	[POC_OP_MCD_READ] = "POC_OP_MCD_READ",
#ifdef CONFIG_SUPPORT_POC_SPI
	[POC_OP_SET_CONN_SRC] = "POC_OP_SET_CONN_SRC",
	[POC_OP_SET_SPI_SPEED] = "POC_OP_SET_SPI_SPEED",
	[POC_OP_READ_SPI_STATUS_REG] = "POC_OP_READ_SPI_STATUS_REG",
#endif
	[POC_OP_INITIALIZE] = "POC_OP_INITIALIZE",
	[POC_OP_UNINITIALIZE] = "POC_OP_UNINITIALIZE",
};

static int panel_do_poc_seqtbl_by_index_nolock(struct panel_poc_device *poc_dev, int index)
{
	struct panel_device *panel = to_panel_device(poc_dev);

	if (panel->poc_dev.seqtbl == NULL) {
		panel_err("ERR:PANEL:%s:invalid seqtbl\n", __func__);
		return -EINVAL;
	}

	if (unlikely(index < 0 || index >= MAX_POC_SEQ)) {
		panel_err("%s, invalid paramter (panel %p, index %d)\n",
				__func__, panel, index);
		return -EINVAL;
	}

	return excute_seqtbl_nolock(panel, poc_dev->seqtbl, index);
}

int panel_do_poc_seqtbl_by_index(struct panel_poc_device *poc_dev, int index)
{
	struct panel_device *panel = to_panel_device(poc_dev);
	int ret;

	mutex_lock(&panel->op_lock);
	ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, index);
	mutex_unlock(&panel->op_lock);

	return ret;
}

static struct seqinfo *find_poc_seqtbl_by_index(struct panel_poc_device *poc_dev, u32 index)
{
	struct seqinfo *tbl;

	if (unlikely(!poc_dev->seqtbl)) {
		panel_err("%s, seqtbl not exist\n", __func__);
		return NULL;
	}

	if (unlikely(index >= MAX_POC_SEQ)) {
		panel_err("%s, invalid paramter (index %d)\n",
				__func__, index);
		return NULL;
	}

	tbl = &poc_dev->seqtbl[index];
	if (tbl != NULL)
		pr_debug("%s, found %s panel seqtbl\n", __func__, tbl->name);

	return tbl;
}

static bool panel_poc_seq_exist(struct panel_poc_device *poc_dev, u32 index)
{
	struct seqinfo *tbl;

	tbl = find_poc_seqtbl_by_index(poc_dev, index);
	if (tbl == NULL || tbl->cmdtbl == NULL || tbl->size == 0)
		return false;

	return true;
}

static int poc_get_poc_chksum(struct panel_device *panel)
{
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	struct panel_info *panel_data = &panel->panel_data;
	int ret;

	if (sizeof(poc_info->poc_chksum) != PANEL_POC_CHKSUM_LEN) {
		pr_err("%s invalid poc control length\n", __func__);
		return -EINVAL;
	}
/*
 *	mutex_lock(&panel->op_lock);
 *	panel_set_key(panel, 3, true);
 *	ret = panel_resource_update_by_name(panel, "poc_chksum");
 *	panel_set_key(panel, 3, false);
 *	mutex_unlock(&panel->op_lock);
 *	if (unlikely(ret < 0)) {
 *		pr_err("%s failed to update resource(poc_chksum)\n", __func__);
 *		return ret;
 *	}
 */
	ret = resource_copy_by_name(panel_data, poc_info->poc_chksum, "poc_chksum");
	if (unlikely(ret < 0)) {
		pr_err("%s failed to copy resource(poc_chksum)\n", __func__);
		return ret;
	}

	pr_info("%s poc_chksum 0x%02X 0x%02X 0x%02X 0x%02X, result %d\n",
			__func__, poc_info->poc_chksum[0], poc_info->poc_chksum[1],
			poc_info->poc_chksum[2], poc_info->poc_chksum[3],
			poc_info->poc_chksum[4]);

	return 0;
}

#ifdef CONFIG_SUPPORT_POC_SPI
bool _spi_poc_status_is_busy(struct panel_poc_info *poc_info, int status)
{
	if (status & poc_info->busy_mask)
		return true;
	return false;
}

bool _spi_poc_status_is_init(struct panel_poc_info *poc_info, int status)
{
	if ((status & poc_info->state_mask) == poc_info->state_init)
		return true;
	return false;
}

bool _spi_poc_status_is_uninit(struct panel_poc_info *poc_info, int status)
{
	if ((status & poc_info->state_mask) == poc_info->state_uninit)
		return true;
	return false;
}

int _spi_poc_get_status(struct panel_device *panel)
{
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_info *panel_data = &panel->panel_data;
	int ret;
	u8 status1, status2;

	ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_SPI_STATUS_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to read poc-spi-status-seq seq\n", __func__);
		return ret;
	}

	ret = resource_copy_by_name(panel_data, &status1, "poc_spi_status1");
	if (unlikely(ret < 0)) {
		pr_err("%s failed to copy resource(poc_spi_status1)\n", __func__);
		return ret;
	}

	ret = resource_copy_by_name(panel_data, &status2, "poc_spi_status2");
	if (unlikely(ret < 0)) {
		pr_err("%s failed to copy resource(poc_spi_status2)\n", __func__);
		return ret;
	}
//	panel_dbg("%s, status: 0x%04X", __func__, (status2 << 8) | status1);

	return (status2 << 8) | status1;
}

int _spi_poc_erase(struct panel_device *panel, int addr, int len)
{
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	int ret, sz_block = 0, erased_size = 0, erase_seq_index;
	int status_retry = 0;

	if (addr % POC_PAGE > 0) {
		pr_err("%s, failed to start erase. invalid addr\n", __func__);
		return -EINVAL;
	}

	if (len < 0 || addr + len > get_poc_partition_size(poc_dev, POC_IMG_PARTITION)) {
		pr_err("%s, failed to start erase. range exceeded\n", __func__);
		return -EINVAL;
	}
	len = ALIGN(len, SZ_4K);

	pr_info("%s poc erase +++, 0x%x, %d\n", __func__, addr, len);

	mutex_lock(&panel->op_lock);
	while (len > erased_size) {
		if (atomic_read(&poc_dev->cancel)) {
			pr_err("%s, stopped by user at erase 0x%x\n", __func__, erased_size);
			ret = -EIO;
			atomic_set(&poc_dev->cancel, 0);
			goto out_poc_erase;
		}

		if ((len >= erased_size + SZ_64K) &&
				panel_poc_seq_exist(poc_dev, POC_SPI_ERASE_64K_SEQ)) {
			erase_seq_index = POC_SPI_ERASE_64K_SEQ;
			sz_block = SZ_64K;
		} else if ((len >= erased_size + SZ_32K) &&
				panel_poc_seq_exist(poc_dev, POC_SPI_ERASE_32K_SEQ)) {
			erase_seq_index = POC_SPI_ERASE_32K_SEQ;
			sz_block = SZ_32K;
		} else {
			erase_seq_index = POC_SPI_ERASE_4K_SEQ;
			sz_block = SZ_4K;
		}

		poc_info->waddr = addr + erased_size;
		ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, erase_seq_index);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to poc-erase-seq 0x%x\n", __func__, addr + erased_size);
			goto out_poc_erase;
		}

		status_retry = 0;
		while (status_retry < POC_SPI_WAIT_ERASE_CNT) {
			ret = _spi_poc_get_status(panel);
			if (unlikely(ret < 0))
				goto out_poc_erase;

			if (!_spi_poc_status_is_busy(poc_info, ret))
				break;

			ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_SPI_WAIT_ERASE_SEQ);
			if (unlikely(ret < 0)) {
				pr_err("%s, failed to poc-spi-wait-erase-seq 0x%x\n", __func__, addr + erased_size);
				goto out_poc_erase;
			}
			status_retry++;
		}

		if (status_retry >= POC_SPI_WAIT_ERASE_CNT) {
			pr_info("%s spi cmd wait timeout cnt: %d\n", __func__, status_retry);
			ret = -EIO;
			goto out_poc_erase;
		}

		pr_info("%s erased addr %06X, sz_block %06X, wait %d\n",
				__func__, addr + erased_size, sz_block, status_retry);
		erased_size += sz_block;
	}
out_poc_erase:
	mutex_unlock(&panel->op_lock);
	pr_info("%s poc erase %d ---\n", __func__, ret);
	return ret;
}
#endif

int _dsi_poc_erase(struct panel_device *panel, int addr, int len)
{
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	int ret, sz_block = 0, erased_size = 0, erase_seq_index;

	if (addr % POC_PAGE > 0) {
		pr_err("%s, failed to start erase. invalid addr\n", __func__);
		return -EINVAL;
	}

	if (len < 0 || addr + len > get_poc_partition_size(poc_dev, POC_IMG_PARTITION)) {
		pr_err("%s, failed to start erase. range exceeded\n", __func__);
		return -EINVAL;
	}
	len = ALIGN(len, SZ_4K);

	pr_info("%s poc erase +++, 0x%x, %d\n", __func__, addr, len);

	mutex_lock(&panel->op_lock);
	ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_ERASE_ENTER_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to poc-erase-enter-seq\n", __func__);
		goto out_poc_erase;
	}
	while (len > erased_size) {
		if ((len >= erased_size + SZ_64K) &&
				panel_poc_seq_exist(poc_dev, POC_ERASE_64K_SEQ)) {
			erase_seq_index = POC_ERASE_64K_SEQ;
			sz_block = SZ_64K;
		} else if ((len >= erased_size + SZ_32K) &&
				panel_poc_seq_exist(poc_dev, POC_ERASE_32K_SEQ)) {
			erase_seq_index = POC_ERASE_32K_SEQ;
			sz_block = SZ_32K;
		} else {
			erase_seq_index = POC_ERASE_4K_SEQ;
			sz_block = SZ_4K;
		}

		poc_info->waddr = addr + erased_size;
		ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, erase_seq_index);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to poc-erase-seq 0x%x\n", __func__, addr + erased_size);
			goto out_poc_erase;
		}
		pr_info("%s erased addr %06X, sz_block %06X\n",
				__func__, addr + erased_size, sz_block);

		if (atomic_read(&poc_dev->cancel)) {
			pr_err("%s, stopped by user at erase 0x%x\n", __func__, erased_size);
			goto cancel_poc_erase;
		}
		erased_size += sz_block;
	}

	ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_ERASE_EXIT_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to poc-erase-exit-seq\n", __func__);
		goto out_poc_erase;
	}

	mutex_unlock(&panel->op_lock);
	pr_info("%s poc erase ---\n", __func__);
	return 0;

cancel_poc_erase:
	ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_ERASE_EXIT_SEQ);
	if (unlikely(ret < 0))
		pr_err("%s, failed to poc-erase-exit-seq\n", __func__);
	ret = -EIO;
	atomic_set(&poc_dev->cancel, 0);

out_poc_erase:
	mutex_unlock(&panel->op_lock);
	return ret;
}

int poc_erase(struct panel_device *panel, int addr, int len)
{
#ifdef CONFIG_SUPPORT_POC_SPI
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;

	if (poc_info->conn_src == POC_CONN_SRC_SPI)
		return _spi_poc_erase(panel, addr, len);
#endif
	return _dsi_poc_erase(panel, addr, len);
}

#ifdef CONFIG_SUPPORT_POC_SPI
static int _spi_poc_read_data(struct panel_device *panel, u8 *buf, u32 addr, u32 len)
{
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_info *panel_data = &panel->panel_data;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	int i, ret = 0;
	u32 poc_addr;
	u8 *read_buf;
	int copy_len;

	pr_info("%s poc read addr 0x%06X, %d(0x%X) bytes +++\n",
			__func__, addr, len, len);


	copy_len = get_resource_size_by_name(panel_data, "poc_spi_read");
	if (copy_len <= 0) {
		pr_info("%s failed to get resource %d\n", __func__, copy_len);
		return copy_len;
	}
	read_buf = (u8 *)devm_kzalloc(panel->dev, copy_len * sizeof(u8), GFP_KERNEL);
	if (!read_buf) {
		pr_err("%s memory allocation failed\n", __func__);
		return -ENOMEM;
	}

	mutex_lock(&panel->op_lock);
	for (i = 0; i < len;) {
		poc_addr = addr + i;
		poc_info->raddr = poc_addr;
		ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_SPI_READ_SEQ);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to read poc-spi-read-seq seq\n", __func__);
			goto out;
		}

		ret = resource_copy_by_name(panel_data, read_buf, "poc_spi_read");
		if (unlikely(ret < 0)) {
			pr_err("%s failed to copy resource(poc_spi_read)\n", __func__);
			goto out;
		}

		if (copy_len > len - i)
			copy_len = len - i;
		memcpy(&buf[i], read_buf, copy_len);

		pr_info("%s [%6d] addr %06X 0x%02X\n", __func__, i, poc_addr, buf[i]);

		i += copy_len;
	}
out:
	mutex_unlock(&panel->op_lock);
	devm_kfree(panel->dev, read_buf);
	return ret;
}
#endif

static int _dsi_poc_read_data(struct panel_device *panel,
		u8 *buf, u32 addr, u32 len)
{
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_info *panel_data = &panel->panel_data;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	int i, ret = 0;
	u32 poc_addr;

	pr_info("%s poc read addr 0x%06X, %d(0x%X) bytes +++\n",
			__func__, addr, len, len);

	ret = poc_get_poc_chksum(panel);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to read poc cheksum seq\n", __func__);
		goto exit;
	}

	mutex_lock(&panel->op_lock);
	poc_info->state = POC_STATE_RD_PROGRESS;

	if (poc_info->poc_chksum[0] != 0x00 || poc_info->poc_chksum[1] != 0x00) {
		ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_READ_PRE_ENTER_SEQ);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to read poc-rd-pre-enter seq\n", __func__);
			goto out_poc_read;
		}
	}

	ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_READ_ENTER_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to read poc-rd-enter seq\n", __func__);
		goto out_poc_read;
	}

	for (i = 0; i < len; i++) {
		if (atomic_read(&poc_dev->cancel)) {
			pr_err("%s, stopped by user at %d bytes\n",
					__func__, i);
			goto cancel_poc_read;
		}

		poc_addr = addr + i;
		poc_info->raddr = poc_addr;
		ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_READ_DAT_SEQ);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to read poc-rd-dat seq\n", __func__);
			goto out_poc_read;
		}

		ret = resource_copy_by_name(panel_data, &buf[i], "poc_data");
		if (unlikely(ret < 0)) {
			pr_err("%s failed to copy resource(poc_data)\n", __func__);
			goto out_poc_read;
		}

		if ((i % 4096) == 0)
			pr_info("%s [%04d] addr %06X %02X\n", __func__, i, poc_addr, buf[i]);
	}

	ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_READ_EXIT_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to read poc-rd-exit seq\n", __func__);
		goto out_poc_read;
	}

	pr_info("%s poc read addr 0x%06X, %d(0x%X) bytes ---\n",
			__func__, addr, len, len);
	poc_info->state = POC_STATE_RD_COMPLETE;
	mutex_unlock(&panel->op_lock);

	return 0;

cancel_poc_read:
	ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_READ_EXIT_SEQ);
	if (unlikely(ret < 0))
		pr_err("%s, failed to read poc-rd-exit seq\n", __func__);
	ret = -EIO;
	atomic_set(&poc_dev->cancel, 0);

out_poc_read:
	poc_info->state = POC_STATE_RD_FAILED;
	mutex_unlock(&panel->op_lock);
exit:
	return ret;
}

int poc_read_data(struct panel_device *panel, u8 *buf, u32 addr, u32 len)
{
#ifdef CONFIG_SUPPORT_POC_SPI
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;

	if (poc_info->conn_src == POC_CONN_SRC_SPI)
		return _spi_poc_read_data(panel, buf, addr, len);
#endif
	return _dsi_poc_read_data(panel, buf, addr, len);
}

#ifdef CONFIG_SUPPORT_POC_SPI
static int _spi_poc_write_data(struct panel_device *panel, u8 *data, u32 addr, u32 size)
{
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	int i, ret = 0;
	u32 poc_addr;
	int copy_len;
	int status_retry_total = 0, status_retry = 0;

	if (addr % POC_PAGE > 0) {
		pr_err("%s, failed to start write. invalid addr\n", __func__);
		return -EINVAL;
	}

	poc_info->wdata = (u8 *)devm_kzalloc(panel->dev, poc_info->spi_wdata_len * sizeof(u8), GFP_KERNEL);
	if (!poc_info->wdata) {
		pr_err("%s memory allocation failed\n", __func__);
		return -ENOMEM;
	}

	mutex_lock(&panel->op_lock);
	for (i = 0; i < size;) {
		poc_addr = addr + i;
		poc_info->waddr = poc_addr;
		memset(poc_info->wdata, 0x00, poc_info->spi_wdata_len);
		copy_len = size - i;
		if (copy_len > poc_info->spi_wdata_len)
			copy_len = poc_info->spi_wdata_len;
		memcpy(poc_info->wdata, data + i, copy_len);
		poc_info->wdata_len = copy_len;

		ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_SPI_WRITE_SEQ);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to write poc-wr-img seq\n", __func__);
			goto out_poc_write;
		}

		if ((i % 4096) == 0)
			pr_info("%s addr %06X %02X\n", __func__, poc_addr, data[i]);
		i += copy_len;

#ifdef PANEL_POC_SPI_BUSY_WAIT
		status_retry = 0;
		while (status_retry < POC_SPI_WAIT_WRITE_CNT) {
			ret = _spi_poc_get_status(panel);
			if (unlikely(ret < 0))
				goto out_poc_write;

			if (!_spi_poc_status_is_busy(poc_info, ret))
				break;

			ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_SPI_WAIT_WRITE_SEQ);
			if (unlikely(ret < 0)) {
				pr_err("%s, failed to write poc-spi-wait-write seq\n", __func__);
				goto out_poc_write;
			}
			status_retry++;
		}
		if (status_retry >= POC_SPI_WAIT_WRITE_CNT) {
			pr_info("%s spi cmd wait timeout: %d\n", __func__, status_retry);
			ret = -EIO;
			goto out_poc_write;
		}
#endif
		status_retry_total += status_retry;
	}
out_poc_write:

	pr_info("%s poc write addr 0x%06X, %d(0x%X) bytes, ret %d, wait %d\n",
			__func__, addr, size, size, ret, status_retry_total);

	if (poc_info->wdata)
		devm_kfree(panel->dev, poc_info->wdata);
	mutex_unlock(&panel->op_lock);
	return ret;
}
#endif

static int _dsi_poc_write_data(struct panel_device *panel, u8 *data, u32 addr, u32 size)
{
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	int i, ret = 0;
	int copy_len;
	u32 poc_addr;
	bool write_stt_seq_exist;
	bool write_end_seq_exist;

	write_stt_seq_exist = panel_poc_seq_exist(poc_dev, POC_WRITE_STT_SEQ);
	write_end_seq_exist = panel_poc_seq_exist(poc_dev, POC_WRITE_END_SEQ);

	poc_info->wdata = (u8 *)devm_kzalloc(panel->dev, poc_info->wdata_len * sizeof(u8), GFP_KERNEL);
	if (!poc_info->wdata) {
		pr_err("%s memory allocation failed\n", __func__);
		return -ENOMEM;
	}

	mutex_lock(&panel->op_lock);
	ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_WRITE_ENTER_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to read poc-wr-enter-seq\n", __func__);
		goto out_poc_write;
	}

	for (i = 0; i < size;) {
		if (atomic_read(&poc_dev->cancel)) {
			pr_err("%s, stopped by user at %d bytes\n",
					__func__, i);
			goto cancel_poc_write;
		}
		poc_addr = addr + i;
		poc_info->waddr = poc_addr;
		if (write_stt_seq_exist &&
			(i == 0 || (poc_addr & 0xFF) == 0)) {
			ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_WRITE_STT_SEQ);
			if (unlikely(ret < 0)) {
				pr_err("%s, failed to write poc-wr-stt seq\n", __func__);
				goto out_poc_write;
			}
		}

		memset(poc_info->wdata, 0x00, poc_info->wdata_len);
		copy_len = size - i;
		if (copy_len > poc_info->wdata_len)
			copy_len = poc_info->wdata_len;
		memcpy(poc_info->wdata, data + i, copy_len);

		ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_WRITE_DAT_SEQ);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to write poc-wr-img seq\n", __func__);
			goto out_poc_write;
		}

		if ((i % 4096) == 0)
			pr_info("%s addr %06X %02X\n", __func__, poc_addr, data[i]);
		if (write_end_seq_exist &&
			((poc_addr & 0xFF) == 0xFF || i == (size - 1))) {
			ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_WRITE_END_SEQ);
			if (unlikely(ret < 0)) {
				pr_err("%s, failed to write poc-wr-exit seq\n", __func__);
				goto out_poc_write;
			}
		}
		i += copy_len;
	}

	ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_WRITE_EXIT_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write poc-wr-exit seq\n", __func__);
		goto out_poc_write;
	}

	pr_info("%s poc write addr 0x%06X, %d(0x%X) bytes\n",
			__func__, addr, size, size);
	mutex_unlock(&panel->op_lock);
	if (poc_info->wdata)
		devm_kfree(panel->dev, poc_info->wdata);

	return 0;

cancel_poc_write:
	ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_WRITE_EXIT_SEQ);
	if (unlikely(ret < 0))
		pr_err("%s, failed to read poc-wr-exit seq\n", __func__);
	ret = -EIO;
	atomic_set(&poc_dev->cancel, 0);

out_poc_write:
	mutex_unlock(&panel->op_lock);

	if (poc_info->wdata)
		devm_kfree(panel->dev, poc_info->wdata);

	return ret;
}

int poc_write_data(struct panel_device *panel, u8 *data, u32 addr, u32 size)
{
#ifdef CONFIG_SUPPORT_POC_SPI
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;

	if (poc_info->conn_src == POC_CONN_SRC_SPI)
		return _spi_poc_write_data(panel, data, addr, size);
#endif
	return _dsi_poc_write_data(panel, data, addr, size);
}

int poc_memory_initialize(struct panel_device *panel)
{
	int ret = 0;
#ifdef CONFIG_SUPPORT_POC_SPI
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	int retry = 5;

	if (poc_info->conn_src != POC_CONN_SRC_SPI)
		return ret;

	if (!panel_poc_seq_exist(poc_dev, POC_SPI_INIT_SEQ)) {
		pr_err("%s, failed to find poc-spi-init-seq\n", __func__);
		return 0;
	}

	for (; retry > 0; retry--) {
		ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_SPI_INIT_SEQ);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to set poc-spi-init-seq\n", __func__);
			goto out_poc_mem_unlock;
		}
		ret = _spi_poc_get_status(panel);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to get poc-spi-init-seq\n", __func__);
			goto out_poc_mem_unlock;
		}

		if (_spi_poc_status_is_init(poc_info, ret)) {
			pr_info("%s done 0x%04X %d\n", __func__, ret, retry);
			return 0;
		}
	}
	pr_err("%s, failed to check initialized status 0x%04X\n", __func__, ret);
	ret = -EIO;

out_poc_mem_unlock:
#endif
	return ret;
}

int poc_memory_uninitialize(struct panel_device *panel)
{
	int ret = 0;
#ifdef CONFIG_SUPPORT_POC_SPI
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	int retry = 5;

	if (poc_info->conn_src != POC_CONN_SRC_SPI)
		return ret;

	if (!panel_poc_seq_exist(poc_dev, POC_SPI_EXIT_SEQ)) {
		pr_err("%s, failed to find poc-spi-exit-seq\n", __func__);
		return 0;
	}

	for (; retry > 0; retry--) {
		ret = panel_do_poc_seqtbl_by_index_nolock(poc_dev, POC_SPI_EXIT_SEQ);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to set poc-spi-exit-seq %d\n", __func__, ret);
			goto out_poc_mem_unlock;
		}
		ret = _spi_poc_get_status(panel);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to get poc-spi-exit-seq %d\n", __func__, ret);
			goto out_poc_mem_unlock;
		}

		if (_spi_poc_status_is_uninit(poc_info, ret)) {
			pr_info("%s done 0x%04X %d\n", __func__, ret, retry);
			return 0;
		}
	}
	pr_err("%s, failed to check initialized status 0x%04X\n", __func__, ret);
	ret = -EIO;

out_poc_mem_unlock:

#endif
	return ret;
}

static int poc_get_octa_poc(struct panel_device *panel)
{
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	struct panel_info *panel_data = &panel->panel_data;
	u8 octa_id[PANEL_OCTA_ID_LEN] = { 0, };
	int ret;

	ret = resource_copy_by_name(panel_data, octa_id, "octa_id");
	if (unlikely(ret < 0)) {
		pr_err("%s failed to copy resource(octa_id) (ret %d)\n", __func__, ret);
		return ret;
	}
	poc_info->poc = octa_id[1] & 0x0F;

	pr_info("%s poc %d\n", __func__, poc_info->poc);

	return 0;
}

static int poc_get_poc_ctrl(struct panel_device *panel)
{
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	struct panel_info *panel_data = &panel->panel_data;
	int ret;

	if (sizeof(poc_info->poc_ctrl) != PANEL_POC_CTRL_LEN) {
		pr_err("%s invalid poc control length\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&panel->op_lock);
	panel_set_key(panel, 3, true);
	ret = panel_resource_update_by_name(panel, "poc_ctrl");
	panel_set_key(panel, 3, false);
	mutex_unlock(&panel->op_lock);
	if (unlikely(ret < 0)) {
		pr_err("%s failed to update resource(poc_ctrl)\n", __func__);
		return ret;
	}

	ret = resource_copy_by_name(panel_data, poc_info->poc_ctrl, "poc_ctrl");
	if (unlikely(ret < 0)) {
		pr_err("%s failed to copy resource(poc_ctrl)\n", __func__);
		return ret;
	}

	pr_info("%s poc_ctrl 0x%02X 0x%02X 0x%02X 0x%02X\n",
			__func__, poc_info->poc_ctrl[0], poc_info->poc_ctrl[1],
			poc_info->poc_ctrl[2], poc_info->poc_ctrl[3]);

	return 0;
}

static int poc_data_backup(struct panel_device *panel, u8 *buf, int size, char *filename)
{
	struct file *fp;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pr_info("%s size %d\n", __func__, size);

	fp = filp_open(filename, O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0660);
	if (IS_ERR(fp)) {
		pr_err("%s, fail to open log file\n", __func__);
		goto open_err;
	}

	vfs_write(fp, (u8 __user *)buf, size, &fp->f_pos);
	pr_info("%s write %d bytes done!!\n", __func__, size);

	filp_close(fp, current->files);
	set_fs(old_fs);

	return 0;

 open_err:
	set_fs(old_fs);
	return -1;
}

static int poc_partition_read_magicnum(struct panel_poc_device *poc_dev,
		int index, void *magicnum)
{
	if (poc_dev->partition[index].magicnum_size > 4 ||
			magicnum == NULL)
		return -EINVAL;

	memcpy(magicnum,
			(void *)&poc_rd_img[poc_dev->partition[index].magicnum_addr],
			poc_dev->partition[index].magicnum_size);

	return 0;
}

static u32 poc_partition_read_chksum(struct panel_poc_device *poc_dev, int index)
{
	u16 chksum = 0;

	chksum = ntohs(*(u16 *)&poc_rd_img[poc_dev->partition[index].checksum_addr]);

	return chksum;
}

static u32 poc_partition_calc_chksum(struct panel_poc_device *poc_dev, int index)
{
	u16 chksum = 0;
	int i;

	for (i = 0; i < poc_dev->partition[index].data_size; i++)
		chksum += poc_rd_img[poc_dev->partition[index].data_addr + i];

	return chksum;
}

int check_poc_partition_exists(struct panel_poc_device *poc_dev, int index)
{
	struct panel_device *panel = to_panel_device(poc_dev);
	u32 value = 0;
	int ret;

	if (!IS_PANEL_ACTIVE(panel))
		return -EAGAIN;

	if (unlikely(index >= poc_dev->nr_partition)) {
		panel_err("POC:ERR:%s: invalid partition index %d\n",
				__func__, index);
		return -EINVAL;
	}

	if (poc_dev->partition[index].magicnum_size != 0) {
		/* check exsitance with magic number */
		ret = poc_read_data(panel, (u8 *)&value,
				poc_dev->partition[index].magicnum_addr,
				poc_dev->partition[index].magicnum_size);

		if (unlikely(ret < 0)) {
			pr_err("%s, failed to read poc data\n", __func__);
			return ret;
		}

		return (value != poc_dev->partition[index].magicnum) ?
			PARTITION_NOT_EXIST : PARTITION_EXISTS;
	}

	return PARTITION_NOT_EXIST;
}

int read_poc_partition(struct panel_poc_device *poc_dev, int index)
{
	struct panel_device *panel = to_panel_device(poc_dev);
	int ret;

	if (!IS_PANEL_ACTIVE(panel))
		return -EAGAIN;

	if (unlikely(index >= poc_dev->nr_partition)) {
		panel_err("POC:ERR:%s: invalid partition index %d\n",
				__func__, index);
		return -EINVAL;
	}

	pr_info("%s partition:%d addr %06X size %d\n",
			__func__, index, poc_dev->partition[index].addr,
			poc_dev->partition[index].size);

	ret = poc_read_data(panel, poc_rd_img + poc_dev->partition[index].addr,
			poc_dev->partition[index].addr, poc_dev->partition[index].size);
	if (unlikely(ret < 0)) {
		pr_err("%s[%d] failed to poc_read_data (ret %d)\n",
				__func__, index, ret);
		goto err_read;
	}
//	print_hex_dump(KERN_ERR, "read_poc_partition ", DUMP_PREFIX_ADDRESS, 16, 1, poc_rd_img + poc_dev->partition[index].addr, poc_dev->partition[index].size, false);

	poc_dev->partition[index].preload_done = true;
	poc_dev->partition[index].chksum_by_calc =
		poc_partition_calc_chksum(poc_dev, index);
	poc_dev->partition[index].chksum_by_read =
		poc_partition_read_chksum(poc_dev, index);
	poc_dev->partition[index].chksum_ok =
		(poc_dev->partition[index].chksum_by_calc ==
		 poc_dev->partition[index].chksum_by_read);

	pr_info("%s[%d] chksum %s(calc:%04X, read:%04X)\n",
			__func__, index,
			poc_dev->partition[index].chksum_ok ? "OK" : "NOK",
			poc_dev->partition[index].chksum_by_calc,
			poc_dev->partition[index].chksum_by_read);

	if (poc_dev->partition[index].magicnum_size > 0) {
		ret = poc_partition_read_magicnum(poc_dev, index,
				&poc_dev->partition[index].magicnum_by_read);
		if (ret < 0)
			pr_warn("%s[%d] failed to read magicnum(%d)\n",
					__func__, index, ret);
		else
			pr_info("%s[%d] magic %s(orig:%X, read:%X)\n",
					__func__, index, (poc_dev->partition[index].magicnum ==
					poc_dev->partition[index].magicnum_by_read) ? "OK" : "NOK",
					poc_dev->partition[index].magicnum,
					poc_dev->partition[index].magicnum_by_read);
	}

	return 0;

err_read:
	return ret;
}

int copy_poc_partition(struct panel_poc_device *poc_dev, u8 *dst,
		 int index, int offset, int size)
{
	if (unlikely(index >= poc_dev->nr_partition)) {
		panel_err("POC:ERR:%s: invalid partition index %d\n",
				__func__, index);
		return -EINVAL;
	}

	if (unlikely(offset + size > poc_dev->partition[index].size)) {
		panel_err("POC:ERR:%s: invalid offset %d size %d\n",
				__func__, offset, size);
		return -EINVAL;
	}

	if (!poc_dev->partition[index].preload_done) {
		panel_err("POC:WARN:%s: partition(%d) is not loaded\n",
				__func__, index);
		return -EINVAL;
	}

	if (!poc_dev->partition[index].chksum_ok) {
		panel_err("POC:WARN:%s: partition(%d) checksum error(calc:%04X read:%04X)\n",
				__func__, index,
				poc_dev->partition[index].chksum_by_calc,
				poc_dev->partition[index].chksum_by_read);
	}

	memcpy(dst, poc_rd_img + poc_dev->partition[index].addr + offset, size);
	return size;
}

int get_poc_partition_size(struct panel_poc_device *poc_dev, int index)
{
	if (unlikely(index >= poc_dev->nr_partition)) {
		panel_err("POC:ERR:%s: invalid partition index %d\n",
				__func__, index);
		return -EINVAL;
	}
	return poc_dev->partition[index].size;
}

int set_panel_poc(struct panel_poc_device *poc_dev, u32 cmd, const char *cmd_ext)
{
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	struct panel_device *panel = to_panel_device(poc_dev);
	int ret = 0;
	struct timespec cur_ts, last_ts, delta_ts;
	s64 elapsed_msec;
	int addr = -1, len = -1;

	if (cmd >= MAX_POC_OP) {
		panel_err("%s invalid poc_op %d\n", __func__, cmd);
		return -EINVAL;
	}

	panel_info("%s %s +\n", __func__, poc_op[cmd]);
	ktime_get_ts(&last_ts);

	switch (cmd) {
	case POC_OP_ERASE:
		break;
	case POC_OP_WRITE:
		ret = poc_write_data(panel, &poc_info->wbuf[poc_info->wpos],
				poc_info->wpos, poc_info->wsize);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to write poc-write-seq\n", __func__);
			return ret;
		}
		break;
	case POC_OP_READ:
		ret = poc_read_data(panel, &poc_info->rbuf[poc_info->rpos],
				poc_info->rpos, poc_info->rsize);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to write poc-read-seq\n", __func__);
			return ret;
		}
		break;
	case POC_OP_CHECKSUM:
		ret = poc_get_poc_chksum(panel);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to get poc checksum\n", __func__);
			return ret;
		}
		ret = poc_get_poc_ctrl(panel);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to get poc ctrl\n", __func__);
			return ret;
		}
		break;
	case POC_OP_CHECKPOC:
		ret = poc_get_octa_poc(panel);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to get_octa_poc\n", __func__);
			return ret;
		}
		break;
	case POC_OP_SECTOR_ERASE:
		ret = sscanf(cmd_ext, "%*d %d %d", &addr, &len);
		if (unlikely(ret < 2)) {
			pr_err("%s, failed to get poc erase params\n", __func__);
			return -EINVAL;
		}
		if (unlikely(addr < 0) || unlikely(len < 0)) {
			pr_err("%s, invalid poc erase params\n", __func__);
			return -EINVAL;
		}
		ret = poc_memory_initialize(panel);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to initialize memory\n", __func__);
			return ret;
		}
		poc_info->erase_trycount++;
		ret = poc_erase(panel, addr, len);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to write poc-erase-seq\n", __func__);
			poc_info->erase_failcount++;
			return ret;
		}
		poc_info->erased = true;
		ret = poc_memory_uninitialize(panel);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to uninitialize memory\n", __func__);
			return ret;
		}
		break;
	case POC_OP_IMG_READ:
		ret = read_poc_partition(poc_dev, POC_IMG_PARTITION);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to read img partition\n", __func__);
			return ret;
		}
		ret = poc_data_backup(panel, poc_rd_img + poc_dev->partition[POC_IMG_PARTITION].addr,
			poc_dev->partition[POC_IMG_PARTITION].size, POC_IMG_PATH);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to backup poc img\n", __func__);
			return ret;
		}
		break;
	case POC_OP_DIM_READ:
		poc_dev->partition[POC_DIM_PARTITION].preload_done = false;
		poc_dev->partition[POC_DIM_PARTITION].chksum_ok = false;
		ret = read_poc_partition(poc_dev, POC_DIM_PARTITION);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to read dim partition\n", __func__);
			return ret;
		}
		break;
	case POC_OP_DIM_CHKSUM:
		break;
	case POC_OP_DIM_VALID:
		break;
	case POC_OP_MTP_READ:
		poc_dev->partition[POC_MTP_PARTITION].preload_done = false;
		poc_dev->partition[POC_MTP_PARTITION].chksum_ok = false;
		ret = read_poc_partition(poc_dev, POC_MTP_PARTITION);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to read MTP partition\n", __func__);
			return ret;
		}
		break;
	case POC_OP_MCD_READ:
		ret = read_poc_partition(poc_dev, POC_MCD_PARTITION);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to read MCD partition\n", __func__);
			return ret;
		}
		break;
	case POC_OP_DIM_READ_TEST:
		ret = read_poc_partition(poc_dev, POC_DIM_PARTITION);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to read dim partition\n", __func__);
			return ret;
		}

		ret = poc_data_backup(panel,
				poc_rd_img + poc_dev->partition[POC_DIM_PARTITION].addr,
				poc_dev->partition[POC_DIM_PARTITION].size, POC_DATA_PATH);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to backup gamma flash\n", __func__);
			return ret;
		}
		break;
#ifdef CONFIG_SUPPORT_POC_SPI
	case POC_OP_SET_CONN_SRC:
		ret = sscanf(cmd_ext, "%*d %d", &addr);
		if (unlikely(ret < 1)) {
			pr_err("%s, failed to get poc set conn params\n", __func__);
			return -EINVAL;
		}
		if (unlikely(addr < 0) || addr >= MAX_POC_CONN_SRC) {
			pr_err("%s, invalid poc set conn params\n", __func__);
			return -EINVAL;
		}
		poc_info->conn_src = addr;
		break;
	case POC_OP_READ_SPI_STATUS_REG:
		ret = _spi_poc_get_status(panel);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to get status reg\n", __func__);
			return ret;
		}
		panel_info("%s spi_status_reg 0x%04X\n", __func__, ret);
		break;
#endif
	case POC_OP_INITIALIZE:
		ret = poc_memory_initialize(panel);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to initialize memory\n", __func__);
			return ret;
		}
		break;
	case POC_OP_UNINITIALIZE:
		ret = poc_memory_uninitialize(panel);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to uninitialize memory\n", __func__);
			return ret;
		}
		break;
	case POC_OP_NONE:
		panel_info("%s none operation\n", __func__);
		break;
	default:
		panel_err("%s invalid poc op\n", __func__);
		break;
	}

	ktime_get_ts(&cur_ts);
	delta_ts = timespec_sub(cur_ts, last_ts);
	elapsed_msec = timespec_to_ns(&delta_ts) / 1000000;
	panel_info("%s %s (elapsed %lld.%03lld sec) -\n", __func__, poc_op[cmd],
			elapsed_msec / 1000, elapsed_msec % 1000);

	return 0;
};

#ifdef CONFIG_SUPPORT_POC_FLASH
static long panel_poc_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	struct panel_poc_device *poc_dev = file->private_data;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	struct panel_device *panel = to_panel_device(poc_dev);
	int ret;

	if (unlikely(!poc_dev->opened)) {
		panel_err("POC:ERR:%s: poc device not opened\n", __func__);
		return -EIO;
	}


	panel_wake_lock(panel);
	if (!IS_PANEL_ACTIVE(panel)) {
		panel_wake_unlock(panel);
		return -EAGAIN;
	}

	mutex_lock(&panel->io_lock);
	switch (cmd) {
	case IOC_GET_POC_STATUS:
		if (copy_to_user((u32 __user *)arg, &poc_info->state,
					sizeof(poc_info->state))) {
			ret = -EFAULT;
			break;
		}
		break;
	case IOC_GET_POC_CHKSUM:
		ret = set_panel_poc(poc_dev, POC_OP_CHECKSUM, NULL);
		if (ret) {
			panel_err("%s error set_panel_poc\n", __func__);
			ret = -EFAULT;
			break;
		}
		if (copy_to_user((u8 __user *)arg, &poc_info->poc_chksum[4],
					sizeof(poc_info->poc_chksum[4]))) {
			ret = -EFAULT;
			break;
		}
		break;
	case IOC_GET_POC_CSDATA:
		ret = set_panel_poc(poc_dev, POC_OP_CHECKSUM, NULL);
		if (ret) {
			panel_err("%s error set_panel_poc\n", __func__);
			ret = -EFAULT;
			break;
		}
		if (copy_to_user((u8 __user *)arg, poc_info->poc_chksum,
					sizeof(poc_info->poc_chksum))) {
			ret = -EFAULT;
			break;
		}
		break;
	case IOC_GET_POC_ERASED:
		if (copy_to_user((u8 __user *)arg, &poc_info->erased,
					sizeof(poc_info->erased))) {
			ret = -EFAULT;
			break;
		}
		break;
	case IOC_GET_POC_FLASHED:
		ret = set_panel_poc(poc_dev, POC_OP_CHECKPOC, NULL);
		if (ret) {
			panel_err("%s error set_panel_poc\n", __func__);
			ret = -EFAULT;
			break;
		}
		if (copy_to_user((u8 __user *)arg, &poc_info->poc,
					sizeof(poc_info->poc))) {
			ret = -EFAULT;
			break;
		}
		break;
	case IOC_SET_POC_ERASE:
		ret = set_panel_poc(poc_dev, POC_OP_ERASE, NULL);
		if (ret) {
			panel_err("%s error set_panel_poc\n", __func__);
			ret = -EFAULT;
			break;
		}
		break;
	default:
		break;
	};
	mutex_unlock(&panel->io_lock);
	panel_wake_unlock(panel);

	return 0;
}

static int panel_poc_open(struct inode *inode, struct file *file)
{
	struct miscdevice *dev = file->private_data;
	struct panel_poc_device *poc_dev = container_of(dev, struct panel_poc_device, dev);
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	struct panel_device *panel = to_panel_device(poc_dev);
	int ret;

	panel_info("%s was called\n", __func__);

	if (poc_dev->opened) {
		panel_err("POC:ERR:%s: already opend\n", __func__);
		return -EBUSY;
	}

	panel_wake_lock(panel);
	if (!IS_PANEL_ACTIVE(panel)) {
		panel_wake_unlock(panel);
		return -EAGAIN;
	}

	mutex_lock(&panel->io_lock);
	mutex_lock(&panel->op_lock);

	ret = set_panel_poc(poc_dev, POC_OP_INITIALIZE, NULL);
	if (ret < 0)
		goto err_open;

	poc_info->state = 0;
	memset(poc_info->poc_chksum, 0, sizeof(poc_info->poc_chksum));
	memset(poc_info->poc_ctrl, 0, sizeof(poc_info->poc_ctrl));

	poc_info->wbuf = poc_wr_img;
	poc_info->wpos = 0;
	poc_info->wsize = 0;

	poc_info->rbuf = poc_rd_img;
	poc_info->rpos = 0;
	poc_info->rsize = 0;

	file->private_data = poc_dev;
	poc_dev->opened = 1;
	atomic_set(&poc_dev->cancel, 0);
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel->io_lock);
	panel_wake_unlock(panel);

	return 0;
err_open:
	panel_err("POC:ERR:%s: failed to initialize %d\n", __func__, ret);
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel->io_lock);
	panel_wake_unlock(panel);
	return ret;
}

static int panel_poc_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct panel_poc_device *poc_dev = file->private_data;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	struct panel_device *panel = to_panel_device(poc_dev);

	panel_info("%s was called\n", __func__);

	panel_wake_lock(panel);

	mutex_lock(&panel->io_lock);
	mutex_lock(&panel->op_lock);

	poc_info->state = 0;
	memset(poc_info->poc_chksum, 0, sizeof(poc_info->poc_chksum));
	memset(poc_info->poc_ctrl, 0, sizeof(poc_info->poc_ctrl));

	poc_info->wbuf = NULL;
	poc_info->wpos = 0;
	poc_info->wsize = 0;

	poc_info->rbuf = NULL;
	poc_info->rpos = 0;
	poc_info->rsize = 0;

	poc_dev->opened = 0;
	atomic_set(&poc_dev->cancel, 0);

	ret = set_panel_poc(poc_dev, POC_OP_UNINITIALIZE, NULL);
	if (ret < 0)
		panel_err("POC:ERR:%s: failed to uninitialize %d\n", __func__, ret);
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel->io_lock);
	panel_wake_unlock(panel);
	return ret;
}

static ssize_t panel_poc_read(struct file *file, char __user *buf, size_t count,
		loff_t *ppos)
{
	struct panel_poc_device *poc_dev = file->private_data;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	struct panel_device *panel = to_panel_device(poc_dev);
	ssize_t res;
	int partition_size;

	panel_info("%s : size : %d, ppos %d\n", __func__, (int)count, (int)*ppos);
	poc_info->read_trycount++;

	if (unlikely(!poc_dev->opened)) {
		panel_err("POC:ERR:%s: poc device not opened\n", __func__);
		poc_info->read_failcount++;
		return -EIO;
	}

	if (unlikely(!buf)) {
		panel_err("POC:ERR:%s: invalid read buffer\n", __func__);
		poc_info->read_failcount++;
		return -EINVAL;
	}

	partition_size = get_poc_partition_size(poc_dev, POC_IMG_PARTITION);
	if (partition_size < 0) {
		poc_info->read_failcount++;
		return -EINVAL;
	}

	if (unlikely(*ppos < 0 || *ppos >= partition_size)) {
		panel_err("POC:ERR:%s: invalid read pos %d\n",
				__func__, (int)*ppos);
		poc_info->read_failcount++;
		return -EINVAL;
	}

	panel_wake_lock(panel);

	if (!IS_PANEL_ACTIVE(panel)) {
		poc_info->read_failcount++;
		panel_wake_unlock(panel);
		return -EAGAIN;
	}

	mutex_lock(&panel->io_lock);
	poc_info->rbuf = poc_rd_img;
	poc_info->rpos = *ppos;
	if (count > partition_size - *ppos) {
		panel_warn("POC:WARN:%s: adjust count %d -> %d\n",
				__func__, (int)count, (int)(partition_size - *ppos));
		count = partition_size - *ppos;
	}
	poc_info->rsize = (u32)count;

	res = set_panel_poc(poc_dev, POC_OP_READ, NULL);
	if (res < 0)
		goto err_read;

	res = simple_read_from_buffer(buf, poc_info->rsize,
			ppos, poc_info->rbuf, partition_size);
	if (res < 0)
		goto err_read;

	panel_info("%s read %ld bytes (count %ld)\n", __func__, res, count);
	mutex_unlock(&panel->io_lock);
	panel_wake_unlock(panel);
	return res;

err_read:
	mutex_unlock(&panel->io_lock);
	poc_info->read_failcount++;
	panel_wake_unlock(panel);
	return res;
}

static ssize_t panel_poc_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	struct panel_poc_device *poc_dev = file->private_data;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	struct panel_device *panel = to_panel_device(poc_dev);
	ssize_t res;
	int partition_size;

	panel_info("%s : size : %d, ppos %d\n", __func__, (int)count, (int)*ppos);
	poc_info->write_trycount++;

	if (unlikely(!poc_dev->opened)) {
		panel_err("POC:ERR:%s: poc device not opened\n", __func__);
		poc_info->write_failcount++;
		return -EIO;
	}

	if (unlikely(!buf)) {
		panel_err("POC:ERR:%s: invalid write buffer\n", __func__);
		poc_info->write_failcount++;
		return -EINVAL;
	}

	partition_size = get_poc_partition_size(poc_dev, POC_IMG_PARTITION);
	if (partition_size < 0) {
		poc_info->write_failcount++;
		return -EINVAL;
	}

	if (unlikely(*ppos < 0 || *ppos >= partition_size)) {
		panel_err("POC:ERR:%s: invalid write size pos %d, size %d\n",
				__func__, (int)*ppos, (int)count);
		poc_info->write_failcount++;
		return -EINVAL;
	}

	panel_wake_lock(panel);

	if (!IS_PANEL_ACTIVE(panel)) {
		poc_info->write_failcount++;
		panel_wake_unlock(panel);
		return -EAGAIN;
	}

	mutex_lock(&panel->io_lock);
	poc_info->wbuf = poc_wr_img;
	poc_info->wpos = *ppos;
	if (count > partition_size - *ppos) {
		panel_warn("POC:WARN:%s: adjust count %d -> %d\n",
				__func__, (int)count, (int)(partition_size - *ppos));
		count = partition_size - *ppos;
	}
	poc_info->wsize = (u32)count;

	res = simple_write_to_buffer(poc_info->wbuf, partition_size,
			ppos, buf, poc_info->wsize);
	if (res < 0)
		goto err_write;

	panel_info("%s write %ld bytes (count %ld)\n", __func__, res, count);

	res = set_panel_poc(poc_dev, POC_OP_WRITE, NULL);
	if (res < 0)
		goto err_write;
	mutex_unlock(&panel->io_lock);
	panel_wake_unlock(panel);

	return count;

err_write:
	poc_info->write_failcount++;
	mutex_unlock(&panel->io_lock);
	panel_wake_unlock(panel);
	return res;
}

loff_t panel_poc_llseek(struct file *file, loff_t offset, int whence)
{
	struct inode *inode = file->f_mapping->host;

	pr_info("s_maxbytes %d, i_size_read %d\n",
			(int)inode->i_sb->s_maxbytes, (int)i_size_read(inode));

	return generic_file_llseek_size(file, offset, whence,
					inode->i_sb->s_maxbytes,
					i_size_read(inode));
}

static const struct file_operations panel_poc_fops = {
	.owner = THIS_MODULE,
	.read = panel_poc_read,
	.write = panel_poc_write,
	.unlocked_ioctl = panel_poc_ioctl,
	.open = panel_poc_open,
	.release = panel_poc_release,
	.llseek	= generic_file_llseek,
};
#endif /* CONFIG_SUPPORT_POC_FLASH */

#ifdef CONFIG_DISPLAY_USE_INFO
#ifdef CONFIG_SUPPORT_POC_FLASH
#define EPOCEFS_IMGIDX (100)
enum {
	EPOCEFS_NOENT = 1,		/* No such file or directory */
	EPOCEFS_EMPTY = 2,		/* Empty file */
	EPOCEFS_READ = 3,		/* Read failed */
	MAX_EPOCEFS,
};

#if 0
static int poc_get_efs_s32(char *filename, int *value)
{
	mm_segment_t old_fs;
	struct file *filp = NULL;
	int fsize = 0, nread, ret = 0;

	if (!filename || !value) {
		pr_err("%s invalid parameter\n", __func__);
		return -EINVAL;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(filename, O_RDONLY, 0440);
	if (IS_ERR(filp)) {
		ret = PTR_ERR(filp);
		if (ret == -ENOENT)
			pr_err("%s file(%s) not exist\n", __func__, filename);
		else
			pr_info("%s file(%s) open error(ret %d)\n",
					__func__, filename, ret);
		set_fs(old_fs);
		return -EPOCEFS_NOENT;
	}

	if (filp->f_path.dentry && filp->f_path.dentry->d_inode)
		fsize = filp->f_path.dentry->d_inode->i_size;

	if (fsize == 0) {
		pr_err("%s invalid file(%s) size %d\n",
				__func__, filename, fsize);
		ret = -EPOCEFS_EMPTY;
		goto exit;
	}

	nread = vfs_read(filp, (char __user *)value, 4, &filp->f_pos);
	if (nread != 4) {
		pr_err("%s failed to read (ret %d)\n", __func__, nread);
		ret = -EPOCEFS_READ;
		goto exit;
	}

	pr_info("%s %s(size %d) : %d\n", __func__, filename, fsize, *value);

exit:
	filp_close(filp, current->files);
	set_fs(old_fs);

	return ret;
}
#endif

static int poc_get_efs_count(char *filename, int *value)
{
	mm_segment_t old_fs;
	struct file *filp = NULL;
	int fsize = 0, nread, rc, ret = 0;
	int count;
	u8 buf[128];

	if (!filename || !value) {
		pr_err("%s invalid parameter\n", __func__);
		return -EINVAL;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(filename, O_RDONLY, 0440);
	if (IS_ERR(filp)) {
		ret = PTR_ERR(filp);
		if (ret == -ENOENT)
			pr_err("%s file(%s) not exist\n", __func__, filename);
		else
			pr_info("%s file(%s) open error(ret %d)\n",
					__func__, filename, ret);
		set_fs(old_fs);
		return -EPOCEFS_NOENT;
	}

	if (filp->f_path.dentry && filp->f_path.dentry->d_inode)
		fsize = filp->f_path.dentry->d_inode->i_size;

	if (fsize == 0 || fsize > ARRAY_SIZE(buf)) {
		pr_err("%s invalid file(%s) size %d\n",
				__func__, filename, fsize);
		ret = -EPOCEFS_EMPTY;
		goto exit;
	}

	memset(buf, 0, sizeof(buf));
	nread = vfs_read(filp, (char __user *)buf, fsize, &filp->f_pos);
	if (nread != fsize) {
		pr_err("%s failed to read (ret %d)\n", __func__, nread);
		ret = -EPOCEFS_READ;
		goto exit;
	}

	rc = sscanf(buf, "%d", &count);
	if (rc != 1) {
		pr_err("%s failed to sscanf %d\n", __func__, rc);
		ret = -EINVAL;
		goto exit;
	}

	pr_info("%s %s(size %d) : %d\n",
			__func__, filename, fsize, count);

	*value = count;

exit:
	filp_close(filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int poc_get_efs_image_index_org(char *filename, int *value)
{
	mm_segment_t old_fs;
	struct file *filp = NULL;
	int fsize = 0, nread, rc, ret = 0;
	char binary;
	int image_index, chksum;
	u8 buf[128];

	if (!filename || !value) {
		pr_err("%s invalid parameter\n", __func__);
		return -EINVAL;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(filename, O_RDONLY, 0440);
	if (IS_ERR(filp)) {
		ret = PTR_ERR(filp);
		if (ret == -ENOENT)
			pr_err("%s file(%s) not exist\n", __func__, filename);
		else
			pr_info("%s file(%s) open error(ret %d)\n",
					__func__, filename, ret);
		set_fs(old_fs);
		return -EPOCEFS_NOENT;
	}

	if (filp->f_path.dentry && filp->f_path.dentry->d_inode)
		fsize = filp->f_path.dentry->d_inode->i_size;

	if (fsize == 0 || fsize > ARRAY_SIZE(buf)) {
		pr_err("%s invalid file(%s) size %d\n",
				__func__, filename, fsize);
		ret = -EPOCEFS_EMPTY;
		goto exit;
	}

	memset(buf, 0, sizeof(buf));
	nread = vfs_read(filp, (char __user *)buf, fsize, &filp->f_pos);
	if (nread != fsize) {
		pr_err("%s failed to read (ret %d)\n", __func__, nread);
		ret = -EPOCEFS_READ;
		goto exit;
	}

	rc = sscanf(buf, "%c %d %d", &binary, &image_index, &chksum);
	if (rc != 3) {
		pr_err("%s failed to sscanf %d\n", __func__, rc);
		ret = -EINVAL;
		goto exit;
	}

	pr_info("%s %s(size %d) : %c %d %d\n",
			__func__, filename, fsize, binary, image_index, chksum);

	*value = image_index;

exit:
	filp_close(filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int poc_get_efs_image_index(char *filename, int *value)
{
	mm_segment_t old_fs;
	struct file *filp = NULL;
	int fsize = 0, nread, rc, ret = 0;
	int image_index, seek;
	u8 buf[128];

	if (!filename || !value) {
		pr_err("%s invalid parameter\n", __func__);
		return -EINVAL;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(filename, O_RDONLY, 0440);
	if (IS_ERR(filp)) {
		ret = PTR_ERR(filp);
		if (ret == -ENOENT)
			pr_err("%s file(%s) not exist\n", __func__, filename);
		else
			pr_info("%s file(%s) open error(ret %d)\n",
					__func__, filename, ret);
		set_fs(old_fs);
		return -EPOCEFS_NOENT;
	}

	if (filp->f_path.dentry && filp->f_path.dentry->d_inode)
		fsize = filp->f_path.dentry->d_inode->i_size;

	if (fsize == 0 || fsize > ARRAY_SIZE(buf)) {
		pr_err("%s invalid file(%s) size %d\n",
				__func__, filename, fsize);
		ret = -EPOCEFS_EMPTY;
		goto exit;
	}

	memset(buf, 0, sizeof(buf));
	nread = vfs_read(filp, (char __user *)buf, fsize, &filp->f_pos);
	if (nread != fsize) {
		pr_err("%s failed to read (ret %d)\n", __func__, nread);
		ret = -EPOCEFS_READ;
		goto exit;
	}

	rc = sscanf(buf, "%d,%d", &image_index, &seek);
	if (rc != 2) {
		pr_err("%s failed to sscanf %d\n", __func__, rc);
		ret = -EINVAL;
		goto exit;
	}

	pr_info("%s %s(size %d) : %d %d\n",
			__func__, filename, fsize, image_index, seek);

	*value = image_index;

exit:
	filp_close(filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int poc_dpui_callback(struct panel_poc_device *poc_dev)
{
	struct panel_poc_info *poc_info;
	char tbuf[MAX_DPUI_VAL_LEN];
	int size, ret, poci, poci_org;

	poc_info = &poc_dev->poc_info;

	ret = poc_get_efs_count(POC_TOTAL_TRY_COUNT_FILE_PATH, &poc_info->total_trycount);
	if (ret < 0)
		poc_info->total_trycount = (ret > -MAX_EPOCEFS) ? ret : -1;
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", poc_info->total_trycount);
	set_dpui_field(DPUI_KEY_PNPOCT, tbuf, size);

	ret = poc_get_efs_count(POC_TOTAL_FAIL_COUNT_FILE_PATH, &poc_info->total_failcount);
	if (ret < 0)
		poc_info->total_failcount = (ret > -MAX_EPOCEFS) ? ret : -1;
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", poc_info->total_failcount);
	set_dpui_field(DPUI_KEY_PNPOCF, tbuf, size);

	ret = poc_get_efs_image_index_org(POC_INFO_FILE_PATH, &poci_org);
	if (ret < 0)
		poci_org = -EPOCEFS_IMGIDX + ret;
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", poci_org);
	set_dpui_field(DPUI_KEY_PNPOCI_ORG, tbuf, size);

	ret = poc_get_efs_image_index(POC_USER_FILE_PATH, &poci);
	if (ret < 0)
		poci = -EPOCEFS_IMGIDX + ret;
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", poci);
	set_dpui_field(DPUI_KEY_PNPOCI, tbuf, size);

	inc_dpui_u32_field(DPUI_KEY_PNPOC_ER_TRY, poc_info->erase_trycount);
	poc_info->erase_trycount = 0;
	inc_dpui_u32_field(DPUI_KEY_PNPOC_ER_FAIL, poc_info->erase_failcount);
	poc_info->erase_failcount = 0;

	inc_dpui_u32_field(DPUI_KEY_PNPOC_WR_TRY, poc_info->write_trycount);
	poc_info->write_trycount = 0;
	inc_dpui_u32_field(DPUI_KEY_PNPOC_WR_FAIL, poc_info->write_failcount);
	poc_info->write_failcount = 0;

	inc_dpui_u32_field(DPUI_KEY_PNPOC_RD_TRY, poc_info->read_trycount);
	poc_info->read_trycount = 0;
	inc_dpui_u32_field(DPUI_KEY_PNPOC_RD_FAIL, poc_info->read_failcount);
	poc_info->read_failcount = 0;

	return 0;
}
#else
static int poc_dpui_callback(struct panel_poc_device *poc_dev) { return 0; }
#endif /* CONFIG_SUPPORT_POC_FLASH */

static int poc_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct panel_poc_device *poc_dev;
	struct dpui_info *dpui = data;

	if (dpui == NULL) {
		panel_err("%s: dpui is null\n", __func__);
		return 0;
	}

	poc_dev = container_of(self, struct panel_poc_device, poc_notif);
	poc_dpui_callback(poc_dev);

	return 0;
}
#endif /* CONFIG_DISPLAY_USE_INFO */

int panel_poc_probe(struct panel_device *panel, struct panel_poc_data *poc_data)
{
	struct panel_poc_device *poc_dev = &panel->poc_dev;
	struct panel_poc_info *poc_info = &poc_dev->poc_info;
	int ret = 0, i, exists;
	static bool initialized;

	if (!poc_data) {
		panel_warn("%s poc_data is null\n", __func__);
		return -EINVAL;
	}

	if (!initialized) {
#ifdef CONFIG_SUPPORT_POC_FLASH
		poc_dev->dev.minor = MISC_DYNAMIC_MINOR;
		poc_dev->dev.name = "poc";
		poc_dev->dev.fops = &panel_poc_fops;
		poc_dev->dev.parent = NULL;

		ret = misc_register(&poc_dev->dev);
		if (ret) {
			panel_err("PANEL:ERR:%s: failed to register panel misc driver (ret %d)\n",
					__func__, ret);
			goto exit_probe;
		}
#endif
	}
	poc_info->version = poc_data->version;
#ifdef CONFIG_SUPPORT_POC_SPI
	poc_info->spi_wdata_len = poc_data->spi_wdata_len;
	poc_info->conn_src = poc_data->conn_src;
	poc_info->state_mask = poc_data->state_mask;
	poc_info->state_init = poc_data->state_init;
	poc_info->state_uninit = poc_data->state_uninit;
	poc_info->busy_mask = poc_data->busy_mask;
#endif
	poc_info->wdata_len = poc_data->wdata_len;
	poc_dev->seqtbl = poc_data->seqtbl;
	poc_dev->nr_seqtbl = poc_data->nr_seqtbl;
	poc_dev->maptbl = poc_data->maptbl;
	poc_dev->nr_maptbl = poc_data->nr_maptbl;
	poc_dev->partition = poc_data->partition;
	poc_dev->nr_partition = poc_data->nr_partition;

	for (i = 0; i < poc_dev->nr_maptbl; i++)
		poc_dev->maptbl[i].pdata = poc_dev;

	poc_info->erased = false;
	poc_info->poc = 1;	/* default enabled */
	poc_dev->opened = 0;

	for (i = 0; i < poc_dev->nr_maptbl; i++)
		maptbl_init(&poc_dev->maptbl[i]);

	for (i = 0; i < poc_dev->nr_partition; i++) {
		poc_dev->partition[i].preload_done = false;
		poc_dev->partition[i].chksum_ok = false;
		panel_info("%s %s addr:0x%x size:%d\n", __func__, poc_dev->partition[i].name,
				poc_dev->partition[i].addr, poc_dev->partition[i].size);
		if (poc_info->total_size <
				poc_dev->partition[i].addr + poc_dev->partition[i].size)
			poc_info->total_size =
				poc_dev->partition[i].addr + poc_dev->partition[i].size;
	}

	if (poc_wr_img)
		devm_kfree(panel->dev, poc_wr_img);
	poc_wr_img = (u8 *)devm_kzalloc(panel->dev,
			poc_info->total_size * sizeof(u8), GFP_KERNEL);

	if (poc_rd_img)
		devm_kfree(panel->dev, poc_rd_img);
	poc_rd_img = (u8 *)devm_kzalloc(panel->dev,
			poc_info->total_size * sizeof(u8), GFP_KERNEL);

#ifdef CONFIG_DISPLAY_USE_INFO
	poc_info->total_trycount = -1;
	poc_info->total_failcount = -1;

	if (!initialized) {
		poc_dev->poc_notif.notifier_call = poc_notifier_callback;
		ret = dpui_logging_register(&poc_dev->poc_notif, DPUI_TYPE_PANEL);
		if (ret) {
			panel_err("ERR:PANEL:%s:failed to register dpui notifier callback\n", __func__);
			goto exit_probe;
		}
	}
#endif

	for (i = 0; i < poc_dev->nr_partition; i++) {
		if (poc_dev->partition[i].need_preload) {
			exists = check_poc_partition_exists(poc_dev, i);
			if (exists < 0) {
				panel_err("ERR:PANEL:%s failed to check partition(%d)\n",
						__func__, i);
				ret = exists;
				goto exit_probe;
			}

			if (!exists) {
				pr_warn("%s partition(%d) not exist\n", __func__, i);
				continue;
			}

			ret = read_poc_partition(poc_dev, i);
			if (unlikely(ret < 0)) {
				pr_err("%s, failed to read partition(%d)\n",
						__func__, i);
				goto exit_probe;
			}
		}
	}
	initialized = true;

	pr_info("%s total_size:%d registered successfully\n",
			__func__, poc_info->total_size);

exit_probe:
	return ret;
}

void copy_poc_wr_addr_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct panel_poc_device *poc_dev;
	struct panel_poc_info *poc_info;

	if (!tbl || !dst)
		return;

	poc_dev = (struct panel_poc_device *)tbl->pdata;
	if (unlikely(!poc_dev))
		return;

	poc_info = &poc_dev->poc_info;

	dst[0] = (poc_info->waddr & 0xFF0000) >> 16;
	dst[1] = (poc_info->waddr & 0x00FF00) >> 8;
	dst[2] = (poc_info->waddr & 0x0000FF);
}

void copy_poc_wr_data_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct panel_poc_device *poc_dev;
	struct panel_poc_info *poc_info;

	if (!tbl || !dst)
		return;

	poc_dev = (struct panel_poc_device *)tbl->pdata;
	if (unlikely(!poc_dev))
		return;

	poc_info = &poc_dev->poc_info;
	memcpy(dst, poc_info->wdata, poc_info->wdata_len);
}

void copy_poc_rd_addr_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct panel_poc_device *poc_dev;
	struct panel_poc_info *poc_info;

	if (!tbl || !dst)
		return;

	poc_dev = (struct panel_poc_device *)tbl->pdata;
	if (unlikely(!poc_dev))
		return;

	poc_info = &poc_dev->poc_info;

	dst[0] = (poc_info->raddr & 0xFF0000) >> 16;
	dst[1] = (poc_info->raddr & 0x00FF00) >> 8;
	dst[2] = (poc_info->raddr & 0x0000FF);
}

void copy_poc_er_addr_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct panel_poc_device *poc_dev;
	struct panel_poc_info *poc_info;

	if (!tbl || !dst)
		return;

	poc_dev = (struct panel_poc_device *)tbl->pdata;
	if (unlikely(!poc_dev))
		return;

	poc_info = &poc_dev->poc_info;

	dst[0] = (poc_info->waddr & 0xFF0000) >> 16;
	dst[1] = (poc_info->waddr & 0x00FF00) >> 8;
	dst[2] = (poc_info->waddr & 0x0000FF);
}

