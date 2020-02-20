/* iva_mcu.c
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Authors:
 *	Ilkwan Kim <ilkwan.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/time.h>

#include "regs/iva_base_addr.h"
#include "iva_mcu.h"
#include "iva_pmu.h"
#include "iva_sh_mem.h"
#include "iva_ram_dump.h"
#include "iva_mbox.h"
#include "iva_ipc_queue.h"
#include "iva_ipc_header.h"
#include "iva_vdma.h"
#include "iva_mem.h"
#include "iva_rt_table.h"
#if defined(CONFIG_SOC_EXYNOS9820)
#include "regs/iva_mcu_reg.h"
#endif

#define MCU_SRAM_VIA_MEMCPY
#define MCU_RAMDUMP_REUSE_BOOT_MEM
#define MCU_IVA_QACTIVE_HOLD

#define PRINT_CHARS_ALIGN_SIZE		(sizeof(uint32_t))
#define PRINT_CHARS_ALIGN_MASK		(PRINT_CHARS_ALIGN_SIZE - 1)
#define PRINT_CHARS_ALIGN_POS(a)	(a & (~PRINT_CHARS_ALIGN_MASK))

static inline void iva_mcu_print_putchar(struct device *dev, int ch)
{
	#define MCU_PRINT_BUF_SIZE	(512)
	static char	mcu_print_buf[MCU_PRINT_BUF_SIZE];
	static size_t	mcu_print_pos;
	static const char *mcu_tag = "[mcu] ";

	if (ch != 0 && ch != '\n') {
		if (mcu_print_pos == 0) {
			strncpy(mcu_print_buf, mcu_tag, MCU_PRINT_BUF_SIZE - 1);
			mcu_print_pos = strlen(mcu_tag);
		}
		mcu_print_buf[mcu_print_pos] = ch;
		mcu_print_pos++;
		if (mcu_print_pos == (sizeof(mcu_print_buf) - 1))/* full */
			goto print;
		return;
	}

	if (ch == 0) {
		if (mcu_print_pos == strlen(mcu_tag))
			return;
	}

	/* the case that: if (ch =='\n'): throw it. */
print:
	/* print out first */
	if (mcu_print_pos != 0) {
		mcu_print_buf[mcu_print_pos] = 0;
		dev_info(dev, "%s\n", mcu_print_buf);
	}
	strncpy(mcu_print_buf, mcu_tag, MCU_PRINT_BUF_SIZE - 1);
	mcu_print_pos = strlen(mcu_tag);
}

/* put max 4 bytes to output */
static int iva_mcu_put_uint_chars(struct device *dev, uint32_t chars,
		uint32_t pos_s/*0-3*/, uint32_t pos_e /*0-3*/)
{
	int put_num = 0;
	char ch;

	if (pos_s > sizeof(chars) - 1) {
		dev_err(dev, "%s() error in pos_s(%d), pos_e(%d)\n",
				__func__, pos_s, pos_e);
		return -1;
	}
	if (pos_e > sizeof(chars) - 1) {
		dev_err(dev, "%s() error in pos_s(%d), pos_e(%d)\n",
				__func__, pos_s, pos_e);
		return -1;
	}

	if (pos_e < pos_s)
		return 0;

	while (pos_s <= pos_e) {
		ch = (chars >> (pos_s << 3)) & 0xFF;
		iva_mcu_print_putchar(dev, ch);
		pos_s++;
		put_num++;
	}

	return put_num;
};


static void iva_mcu_print_emit_pending_chars_nolock(struct mcu_print_info *pi)
{
	u32		ch_vals;
	uint32_t	pos_s;
	uint64_t	pos_e;
	int		align_ch_pos;
	u32		head;

	if (!pi)
		return;

	head = readl(&pi->log_pos->head);
	if (head >= pi->log_buf_size) {	/* wrong value */
		struct device		*dev = pi->dev;
		struct iva_dev_data	*iva = dev_get_drvdata(dev);
		struct sh_mem_info	*sh_mem = sh_mem_get_sm_pointer(iva);

		dev_err(dev,
			"%s() magic3(0x%x), head(%d, %d), tail(%d), size(%d)\n",
			__func__,
			readl(&sh_mem->magic3),
			(int) readl(&sh_mem->log_pos.head),
			(int) head,
			(int) readl(&sh_mem->log_pos.tail),
			(int) readl(&sh_mem->log_buf_size));
		return;
	}
	if (pi->ch_pos == head)
		return;

	if (pi->ch_pos > head) {
		while (pi->ch_pos < pi->log_buf_size) {
			align_ch_pos = PRINT_CHARS_ALIGN_POS(pi->ch_pos);	/* 4 bytes */
			/* aligned access */
			ch_vals = readl(&pi->log_buf[align_ch_pos]);
			pos_s	= pi->ch_pos - align_ch_pos;
			pi->ch_pos += iva_mcu_put_uint_chars(pi->dev, ch_vals,
					pos_s, PRINT_CHARS_ALIGN_SIZE - 1);
		}
		pi->ch_pos = 0;
	}

	while (pi->ch_pos < head) {
		align_ch_pos = PRINT_CHARS_ALIGN_POS(pi->ch_pos);
		/* aligned access */
		ch_vals = readl(&pi->log_buf[align_ch_pos]);
		pos_s	= pi->ch_pos - align_ch_pos;
		pos_e	= (align_ch_pos + PRINT_CHARS_ALIGN_SIZE <= head) ?
				PRINT_CHARS_ALIGN_SIZE - 1 :
					head - align_ch_pos - 1;

		pi->ch_pos += iva_mcu_put_uint_chars(pi->dev, ch_vals, pos_s, (uint32_t)pos_e);
	}
}

static void iva_mcu_print_worker(struct work_struct *work)
{
	struct mcu_print_info *pi = container_of(work, struct mcu_print_info,
					  dwork.work);
	/* emit all pending log */
	iva_mcu_print_emit_pending_chars_nolock(pi);

	queue_delayed_work(system_wq, &pi->dwork, pi->delay_jiffies);
}

static bool iva_mcu_print_init_print_info(struct iva_dev_data *iva,
		struct mcu_print_info *pi)
{
	struct sh_mem_info	*sh_mem = sh_mem_get_sm_pointer(iva);
	struct buf_pos __iomem	*log_pos = &sh_mem->log_pos;
	struct device		*dev = iva->dev;
	u32			tail;
	int			lb_sz;

	if (sh_mem == NULL) return false;

	lb_sz = readl(&sh_mem->log_buf_size);
	if (lb_sz & PRINT_CHARS_ALIGN_MASK) {
		dev_err(dev, "%s() log_buf_size(0x%x) is not aligned to %d. ",
			__func__, lb_sz, (int) PRINT_CHARS_ALIGN_SIZE);
		dev_err(dev, "check the mcu shmem.\n");
		return false;
	}

	if (((long) sh_mem->log_buf) & PRINT_CHARS_ALIGN_MASK) {
		dev_err(dev, "%s() log_buf(%p) is not aligned to %d. ",
			__func__, sh_mem->log_buf, (int) PRINT_CHARS_ALIGN_SIZE);
		dev_err(dev, "check the mcu shmem.\n");
		return false;
	}

	pi->dev			= iva->dev;
	pi->delay_jiffies	= usecs_to_jiffies(iva->mcu_print_delay);
	pi->log_buf_size	= lb_sz;
	pi->log_buf		= &sh_mem->log_buf[0];
	pi->log_pos		= log_pos;
	tail = readl(&log_pos->tail);
	if (tail >= pi->log_buf_size)
		pi->ch_pos = 0;
	else
		pi->ch_pos = tail;

	INIT_DELAYED_WORK(&pi->dwork, iva_mcu_print_worker);

	return true;
}

static void iva_mcu_print_start(struct iva_dev_data *iva)
{
	struct mcu_print_info	*pi;
	struct device		*dev = iva->dev;

	if (iva->mcu_print) {
		dev_warn(dev, "%s() already started, check.\n", __func__);
		return;
	}

	if (iva->mcu_print_delay == 0) {
		dev_dbg(dev, "%s() will not print out iva log\n", __func__);
		return;
	}

	pi = devm_kmalloc(dev, sizeof(*pi), GFP_KERNEL);
	if (!pi) {
		dev_err(dev, "%s() fail to alloc mem\n", __func__);
		return;
	}

	if (!iva_mcu_print_init_print_info(iva, pi)) {
		dev_err(dev, "%s() fail to init print info\n", __func__);
		return;
	}

	queue_delayed_work(system_wq, &pi->dwork, msecs_to_jiffies(1));

	iva->mcu_print = pi;

	dev_dbg(dev, "%s() delay_jiffies(%ld) success!!!\n", __func__,
				pi->delay_jiffies);

}

static void iva_mcu_print_stop(struct iva_dev_data *iva)
{
	struct mcu_print_info *pi = iva->mcu_print;

	if (pi) {
		cancel_delayed_work_sync(&pi->dwork);
		devm_kfree(iva->dev, pi);
		iva->mcu_print = NULL;
	}
}

void iva_mcu_print_flush_pending_mcu_log(struct iva_dev_data *iva)
{
	struct mcu_print_info	pi;

	if (iva->mcu_print) {
		flush_delayed_work(&iva->mcu_print->dwork);
		return;
	}

	if (!iva_mcu_print_init_print_info(iva, &pi)) {
		dev_err(iva->dev, "%s() fail to init print info\n", __func__);
		return;
	}

	iva_mcu_print_emit_pending_chars_nolock(&pi);
}

void iva_mcu_handle_error_k(struct iva_dev_data *iva,
		enum mcu_err_context from, uint32_t msg)
{
	struct iva_ipcq		*ipcq	= &iva->mcu_ipcq;
	struct device		*dev	= iva->dev;
	struct ipc_res_param	rsp_param;

	dev_err(dev, "%s() mcu err, from(%d) msg(%d), err_cnt(%d)\n",
			__func__, from, msg, atomic_read(&iva->mcu_err_cnt));

	if (from == mcu_err_from_abort)	/* no necessary */
		return;

	if (atomic_inc_return(&iva->mcu_err_cnt) == 1) {
		rsp_param.header	= IPC_MK_HDR(ipc_func_iva_ctrl, 0x0, 0x0);
		rsp_param.rep_id	= 0x0ace0bad;
		rsp_param.ret		= (uint32_t) -1;
		rsp_param.ipc_cmd_type	= msg;
		rsp_param.extra		= (uint16_t) 0;

		iva_ipcq_send_rsp_k(ipcq, &rsp_param,
				(from == mcu_err_from_irq) ? true : false);
	}
}

#define GET_MCU_VIEW_DTCM_VA(addr) ((uint32_t)addr - 0x20000000 + 0x10000)
void iva_mcu_show_status(struct iva_dev_data *iva)
{
	struct mcu_binary	*mcu_bin = iva->mcu_bin;
	struct sh_mem_info	*sh_mem;
	void			*tsst = NULL, *ndl = NULL, *dbg_bin = NULL;
	const char		delims[] = "\n";
	char			*token, *vcm_dbg;

	iva_pmu_show_status(iva);
	iva_vdma_show_status(iva);

	if (!mcu_bin->bin)
		return;

	iva_pmu_prepare_sfr_dump(iva);

	iva_ram_dump_copy_vcm_dbg(iva, mcu_bin->bin, mcu_bin->bin_size);
	vcm_dbg = (char *) mcu_bin->bin;
	while ((token = strsep(&vcm_dbg, delims)) != NULL)
		dev_info(iva->dev, "%s", token);

	iva_ram_dump_copy_mcu_sram(iva, mcu_bin->bin, mcu_bin->bin_size);

	/* always last */
	sh_mem = sh_mem_get_sm_pointer(iva);
	if (sh_mem->gen_purpose[1])
		tsst = (void *) (mcu_bin->bin + GET_MCU_VIEW_DTCM_VA(sh_mem->gen_purpose[1]));
	if (sh_mem->gen_purpose[2])
		ndl = (void *) (mcu_bin->bin + GET_MCU_VIEW_DTCM_VA(sh_mem->gen_purpose[2]));
	iva_rt_print_iva_entries(iva, tsst, ndl);

	if (sh_mem->gen_purpose[3])
		dbg_bin = (void *) (mcu_bin->bin + GET_MCU_VIEW_DTCM_VA(sh_mem->gen_purpose[3]));
	iva_ram_dump_print_iva_bin_log(iva, dbg_bin);

	/* invalid as cached file, anymore */
	memset(&mcu_bin->last_mtime, 0x0, sizeof(mcu_bin->last_mtime));
	mcu_bin->file[0] = 0;
}

static int32_t iva_mcu_wait_ready(struct iva_dev_data *iva)
{
	#define WAIT_DELAY_US	(1000)

	int ret = sh_mem_wait_mcu_ready(iva, WAIT_DELAY_US);

	if (ret) {
		dev_err(iva->dev, "%s() mcu is not ready within %d us.\n",
				__func__, WAIT_DELAY_US);
		return ret;
	}

	return 0;
}

static int32_t iva_mcu_map_boot_mem(struct iva_dev_data *iva,
		struct mcu_binary *mcu_bin)
{
	struct device	*dev = iva->dev;
	int		ret;

	if (!(mcu_bin->flag & MCU_BIN_ION))
		return 0;

	if (mcu_bin->bin)	/* reuse */
		return 0;

	ret = dma_buf_begin_cpu_access(mcu_bin->dmabuf, 0);

	if (ret) {
		dev_err(dev, "%s() fail to begin cpu access. ret(%d)\n",
				__func__, ret);
		return ret;
	}

	mcu_bin->bin = dma_buf_kmap(mcu_bin->dmabuf, 0);
	if (!mcu_bin->bin) {
		dev_err(dev, "%s() fail to kmap ion.\n", __func__);
		dma_buf_end_cpu_access(mcu_bin->dmabuf, 0);
		return -ENOMEM;
	}

	return 0;
}

static int32_t iva_mcu_unmap_boot_mem(struct iva_dev_data *iva,
		struct mcu_binary *mcu_bin)
{
	if (!(mcu_bin->flag & MCU_BIN_ION))
		return 0;

	if (!mcu_bin->bin)
		return 0;

	dma_buf_kunmap(mcu_bin->dmabuf, 0, mcu_bin->bin);
	dma_buf_end_cpu_access(mcu_bin->dmabuf, 0);
	mcu_bin->bin = NULL;

	return 0;
}

static inline void iva_mcu_update_iva_sram(struct iva_dev_data *iva,
		struct mcu_binary *mcu_bin)
{
#ifdef MCU_SRAM_VIA_MEMCPY
	void __iomem *mcu_va = iva->iva_va + IVA_VMCU_MEM_BASE_ADDR;
	memcpy(mcu_va, mcu_bin->bin, mcu_bin->bin_size);
#else
#if defined(CONFIG_SOC_EXYNOS8895)
	uint32_t vdma_hd = ID_CH_TO_HANDLE(vdma_id_1, vdma_ch_2);

	iva_vdma_enable(iva, vdma_id_1, true);
	iva_vdma_config_dram_to_mcu(iva, vdma_hd,
			mcu_bin->io_va, 0x0, ALIGN(mcu_bin->file_size, 16),
			true);
	iva_vdma_wait_done(iva, vdma_hd);
	iva_vdma_enable(iva, vdma_id_1, false);

#elif defined(CONFIG_SOC_EXYNOS9810)
	uint32_t vdma_hd = CH_TO_HANDLE(vdma_ch15_type_rw);

	iva_vdma_enable(iva, true);
	iva_vdma_config_dram_to_mcu(iva, vdma_hd,
			mcu_bin->io_va, 0x0, ALIGN(mcu_bin->file_size, 16),
			true);
	iva_vdma_wait_done(iva, vdma_hd);
	iva_vdma_enable(iva, false);

#elif defined(CONFIG_SOC_EXYNOS9820)
	uint32_t vdma_hd = CH_TO_HANDLE(vdma_ch0_type_ro);

	iva_vdma_enable(iva, true);
	iva_vdma_path_dram_to_mcu(iva, vdma_hd, true);
	iva_vdma_config_dram_to_mcu(iva, vdma_hd,
			mcu_bin->io_va, 0x0, ALIGN(mcu_bin->file_size, 16),
			true);
	iva_vdma_wait_done(iva, vdma_hd);
	iva_vdma_path_dram_to_mcu(iva, vdma_hd, false);
	iva_vdma_enable(iva, false);
#endif

	dev_dbg(iva->dev, "%s() prepare vdma iova(0x%x) size(0x%x, 0x%x)\n",
			__func__, (uint32_t) mcu_bin->io_va,
			mcu_bin->file_size, ALIGN(mcu_bin->file_size, 16));

#endif
}

#if defined(CONFIG_SOC_EXYNOS9820)
/* return masked old values */
static uint32_t __iva_mcu_write_mask(void __iomem *sys_reg,
                uint32_t mask, bool set)
{
	uint32_t old_val, val;

	old_val = readl(sys_reg);
	if (set)
		val = old_val | mask;
	else
		val = old_val & ~mask;
	writel(val, sys_reg);

	return old_val & mask;
}

uint32_t iva_mcu_ctrl(struct iva_dev_data *iva,
			enum mcu_boot_hold ctrl, bool set)
{
	return __iva_mcu_write_mask(iva->sys_va + IVA_MCU_CTRL_BOOTHOOLD_ADDR,
	        ctrl, set);
}

uint32_t iva_mcu_init_tcm(struct iva_dev_data *iva,
			enum init_tcm ctrl, bool set)
{
	return __iva_mcu_write_mask(iva->sys_va + IVA_MCU_CTRL_INITTCM_ADDR,
	        ctrl, set);
}

void iva_mcu_prepare_mcu_reset(struct iva_dev_data *iva,
                        uint32_t init_vtor)
{
	iva_pmu_ctrl(iva, pmu_ctrl_qactive, true);
	iva_pmu_ctrl_mcu(iva, pmu_mcu_reset_n, false);
	iva_mcu_ctrl(iva, mcu_boothold, true);
	writel(init_vtor, iva->sys_va + IVA_MCU_CTRL_INITVTOR_ADDR);

	if (init_vtor == 0x0)
	        iva_mcu_init_tcm(iva, itcm_en | dtcm_en, true);
	else
	        iva_mcu_init_tcm(iva, itcm_en | dtcm_en, false);

	iva_pmu_ctrl_mcu(iva, pmu_mcu_reset_n, true);
}

void iva_mcu_reset_mcu(struct iva_dev_data *iva)
{
#if defined(CONFIG_SOC_EXYNOS9820)
#ifndef MCU_IVA_QACTIVE_HOLD
	iva_pmu_ctrl(iva, pmu_ctrl_qactive, false);
#endif
#endif
        iva_mcu_ctrl(iva, mcu_boothold, false);
}
#endif

static int32_t iva_mcu_boot(struct iva_dev_data *iva, struct mcu_binary *mcu_bin,
		int32_t wait_ready)
{
	int ret;
	struct device	*dev = iva->dev;

	if (!mcu_bin) {
		dev_err(dev, "%s() check null pointer\n", __func__);
		return -EINVAL;
	}

#if defined(CONFIG_SOC_EXYNOS8895) || defined(CONFIG_SOC_EXYNOS9810)
	iva_pmu_prepare_mcu_sram(iva);
	iva_mcu_update_iva_sram(iva, mcu_bin);
	/* start to boot */
	iva_pmu_reset_mcu(iva);

#elif defined(CONFIG_SOC_EXYNOS9820)
	if (iva->mcu_split_sram)
		iva_mcu_update_iva_sram(iva, mcu_bin);
	iva_mcu_reset_mcu(iva);
#endif

	/* monitor shared memory initialization from mcu */
	ret = sh_mem_init(iva);
	if (ret) {
		dev_err(dev, "%s() mcu seems to fail to boot.\n", __func__);
		return ret;
	}

	/* enable mcu print */
	iva_mcu_print_start(iva);

	if (wait_ready)
		iva_mcu_wait_ready(iva);

	/* enable ipq queue */
	ret = iva_ipcq_init(&iva->mcu_ipcq,
			&sh_mem_get_sm_pointer(iva)->ap_cmd_q,
			&sh_mem_get_sm_pointer(iva)->ap_rsp_q);
	if (ret < 0) {
		dev_err(dev, "%s() fail to init ipcq (%d)\n", __func__, ret);
		goto err_ipcq;
	}

	return 0;
err_ipcq:
	sh_mem_deinit(iva);
	iva_mcu_print_stop(iva);
	return ret;
}


int32_t iva_mcu_boot_file(struct iva_dev_data *iva,
			const char *mcu_file, int32_t wait_ready)
{
	struct device	*dev = iva->dev;
	struct file	*mcu_fp;
	struct kstat	mcu_st;
	uint32_t	mcu_size;
	uint32_t	vmcu_mem_size = iva->mcu_mem_size;
	struct mcu_binary *mcu_bin = iva->mcu_bin;
	int		ret = 0;
	int		read_bytes;
	loff_t		pos = 0;

	dev_dbg(dev, "%s() try to boot with %s", __func__, mcu_file);

	if (test_bit(IVA_ST_MCU_BOOT_DONE, &iva->state)) {
		dev_warn(dev, "%s() already iva booted(0x%lx)\n",
				__func__, iva->state);
		return 0;
	}

	if (strncmp(mcu_file, IVA_MCU_FILE_PATH, sizeof(IVA_MCU_FILE_PATH))) {
		dev_err(dev, "%s() mcu_file(%s) is wrong file path.\n",
				__func__, mcu_file);
		return 0;
	}

	mcu_fp = filp_open(mcu_file, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(mcu_fp)) {
		dev_err(dev, "%s() unable to open mcu file(%s)\n",
				__func__, mcu_file);
		return -EINVAL;
	}

	ret = vfs_getattr(&mcu_fp->f_path, &mcu_st,
			(STATX_MODE | STATX_SIZE | STATX_MTIME),
			 AT_STATX_SYNC_AS_STAT);
	if (ret) {
		dev_err(dev, "%s() fail to get attr for file(%s)\n",
				__func__, mcu_file);
		goto err_mcu_file;
	}

	if (!S_ISREG(mcu_st.mode)) {
		dev_err(dev, "%s() file(%s) is not regular. mode(0x%x)\n",
				__func__, mcu_file, mcu_st.mode);
		ret = -EINVAL;
		goto err_mcu_file;
	}

	mcu_size = (uint32_t) mcu_st.size;
	if (mcu_size == 0 || mcu_size > vmcu_mem_size) {
		dev_err(dev, "%s() file size(0x%x) is larger that expected %d\n",
				__func__, mcu_size, vmcu_mem_size);
		ret = -EFBIG;
		goto err_mcu_file;
	}

#ifndef MCU_SRAM_VIA_MEMCPY
	if (timespec_equal(&mcu_st.mtime, &mcu_bin->last_mtime) &&
			strcmp(mcu_file, mcu_bin->file) == 0) {
		dev_dbg(dev, "%s() regard the same as previous(%s, 0x%x)\n",
				__func__, mcu_file, mcu_size);
		goto skip_read;
	}
#endif

	ret = iva_mcu_map_boot_mem(iva, mcu_bin);
	if (ret) {
		dev_err(dev, "%s() fail to map boot mem, ret(%d)\n",
				__func__, ret);
		goto err_mcu_file;
	}

#if defined(CONFIG_SOC_EXYNOS9820)
	if (iva->mcu_split_sram)
		iva_mcu_prepare_mcu_reset(iva, 0x0);  /* split i/d srams */
	else {
		memset(mcu_bin->bin, 0, vmcu_mem_size);
		iva_mcu_prepare_mcu_reset(iva, mcu_bin->io_va);
	}
#endif

	read_bytes = kernel_read(mcu_fp, mcu_bin->bin, (size_t)mcu_size, &pos);
	if ((read_bytes <= 0) || ((uint32_t) read_bytes != mcu_size)) {
		dev_err(dev, "%s() fail to read %s. read(%d) file size(%d)\n",
				__func__, mcu_file, read_bytes, mcu_size);
		ret = -EIO;
		goto err_mcu_file;
	}

	mcu_bin->last_mtime = mcu_st.mtime;
	mcu_bin->file_size = mcu_size;
	strncpy(mcu_bin->file, mcu_file, sizeof(mcu_bin->file) - 1);
	mcu_bin->file[sizeof(mcu_bin->file) - 1] = 0x0;

#ifndef MCU_SRAM_VIA_MEMCPY
skip_read:
#endif
	ret = iva_mcu_boot(iva, mcu_bin, wait_ready);
	if (ret) {
		dev_err(dev, "%s() fail to boot mcu. ret(%d)\n",
				__func__, ret);
	} else {
		set_bit(IVA_ST_MCU_BOOT_DONE, &iva->state);
	}

err_mcu_file:
	filp_close(mcu_fp, NULL);

	return ret;
}

int32_t iva_mcu_exit(struct iva_dev_data *iva)
{
	if (!test_and_clear_bit(IVA_ST_MCU_BOOT_DONE, &iva->state)) {
		dev_info(iva->dev, "%s() already mcu exited(0x%lx)\n",
				__func__, iva->state);
		return 0;
	}

	iva_ipcq_deinit(&iva->mcu_ipcq);

	iva_mcu_print_stop(iva);

	sh_mem_deinit(iva);

	iva_mcu_unmap_boot_mem(iva, iva->mcu_bin);

	iva_pmu_ctrl_mcu(iva, pmu_mcu_reset_n, false);
#if defined(CONFIG_SOC_EXYNOS8895) || defined(CONFIG_SOC_EXYNOS9810)
	iva_pmu_ctrl_mcu(iva, pmu_boothold, true);
#elif defined(CONFIG_SOC_EXYNOS9820)
	iva_mcu_ctrl(iva, mcu_boothold, true);
#endif

#if defined(CONFIG_SOC_EXYNOS9820)
#ifdef MCU_IVA_QACTIVE_HOLD
	iva_pmu_ctrl(iva, pmu_ctrl_qactive, false);
	iva_pmu_show_qactive_status(iva);
#endif
#endif
	dev_dbg(iva->dev, "%s() mcu exited(0x%lx) successfully\n",
			__func__, iva->state);

	return 0;
}

static int32_t iva_mcu_boot_mem_alloc(struct iva_dev_data *iva,
			struct mcu_binary *mcu_bin)
{
	int32_t			ret = 0;

#ifdef CONFIG_ION_EXYNOS
    struct device	*dev = iva->dev;
	struct dma_buf		*dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table		*sgt;
	dma_addr_t		io_va;
	const char		*heapname = "ion_system_heap";
	uint32_t		vmcu_mem_size = iva->mcu_mem_size;

	dmabuf = ion_alloc_dmabuf(heapname, vmcu_mem_size, 0);
	if (!dmabuf) {
		dev_err(dev, "%s() ion with dma_buf sharing failed.\n",
				__func__);
		return -ENOMEM;
	}

	attachment = dma_buf_attach(dmabuf, dev);
	if (!attachment) {
		dev_err(dev, "%s() fail to attach dma buf, dmabuf(%p).\n",
				__func__, dmabuf);
		goto err_attach;
	}

	sgt = dma_buf_map_attachment(attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		ret = PTR_ERR(sgt);
		goto err_map_dmabuf;
	}

	io_va = ion_iovmm_map(attachment, 0, vmcu_mem_size, DMA_BIDIRECTIONAL, 0);

	if (!io_va || IS_ERR_VALUE(io_va)) {
		dev_err(dev, "%s() fail to map iovm (%ld)\n", __func__,
				(unsigned long) io_va);
		if (!io_va)
			ret = -ENOMEM;
		else
			ret = (int) io_va;
		goto err_get_iova;
	}

	mcu_bin->flag		= MCU_BIN_ION;
	mcu_bin->bin_size	= vmcu_mem_size;
	mcu_bin->dmabuf		= dmabuf;
	mcu_bin->attachment	= attachment;
	mcu_bin->io_va		= io_va;
	mcu_bin->sgt		= sgt;

	return 0;

err_get_iova:
	dma_buf_unmap_attachment(attachment, sgt, DMA_BIDIRECTIONAL);
err_map_dmabuf:
	dma_buf_detach(dmabuf, attachment);
err_attach:
	dma_buf_put(dmabuf);

	return 0;
#else
    struct device	*dev = iva->dev;

	mcu_bin->bin = devm_kmalloc(dev, VMCU_MEM_SIZE, GFP_KERNEL);
	if (!mcu_bin->bin) {
		dev_err(dev, "%s() unable to alloc mem for mcu boot\n",
				__func__);
		return -ENOMEM;
	}
	mcu_bin->flag		= MCU_BIN_MALLOC;
	mcu_bin->bin_size	= VMCU_MEM_SIZE;
#endif
	return ret;
}

static void iva_mcu_boot_mem_free(struct iva_dev_data *iva,
		struct mcu_binary *mcu_bin)
{
#ifdef CONFIG_ION_EXYNOS
	if (mcu_bin->attachment) {
		ion_iovmm_unmap(mcu_bin->attachment, mcu_bin->io_va);
		if (mcu_bin->sgt) {
			dma_buf_unmap_attachment(mcu_bin->attachment,
					mcu_bin->sgt, DMA_BIDIRECTIONAL);
			mcu_bin->sgt = NULL;
		}
		if (mcu_bin->dmabuf)
			dma_buf_detach(mcu_bin->dmabuf, mcu_bin->attachment);
		mcu_bin->io_va		= 0x0;
		mcu_bin->attachment	= NULL;
	}

	if (mcu_bin->dmabuf) {
		dma_buf_put(mcu_bin->dmabuf);
		mcu_bin->dmabuf = NULL;
	}
#else
	if (mcu_bin->bin) {
		devm_kfree(iva->dev, mcu_bin->bin);
		mcu_bin->bin = NULL;
		mcu_bin->bin_size = 0;
	}
#endif
	mcu_bin->flag = 0x0;
}

int32_t iva_mcu_probe(struct iva_dev_data *iva)
{
	int32_t		ret;
	struct device	*dev = iva->dev;

#if defined(CONFIG_SOC_EXYNOS9820)
        if (iva->sys_va) {
                dev_warn(dev, "%s() already mapped into sys_va(%p)\n", __func__,
                                iva->pmu_va);
                return 0;
        }

        if (!iva->iva_va) {
                dev_err(dev, "%s() null iva_va\n", __func__);
                return -EINVAL;
        }

        iva->sys_va = iva->iva_va + IVA_SYS_REG_BASE_ADDR;

        dev_dbg(iva->dev, "%s() succeed to get sys va\n", __func__);
#endif

	iva->mcu_bin = devm_kzalloc(dev, sizeof(*iva->mcu_bin), GFP_KERNEL);
	if (!iva->mcu_bin) {
		dev_err(dev, "%s() fail to alloc mem for mcu_bin struct\n",
				__func__);
		return -ENOMEM;
	}

	ret = iva_mcu_boot_mem_alloc(iva, iva->mcu_bin);
	if (ret) {
		dev_err(dev, "%s() fail to prepare mcu boot mem ret(%d)\n",
				__func__,  ret);
		goto err_boot_mem;

	}

	ret = iva_mbox_probe(iva);
	if (ret) {
		dev_err(dev, "%s() fail to probe iva mbox. ret(%d)\n",
			__func__,  ret);
		goto err_mbox_prb;
	}

	ret = iva_ipcq_probe(iva);
	if (ret) {
		dev_err(dev, "%s() fail to probe iva mbox. ret(%d)\n",
			__func__,  ret);
		goto err_ipcq_prb;
	}

	/* enable ram dump, just registration*/
	iva_ram_dump_init(iva);

	return 0;
err_ipcq_prb:
	iva_mbox_remove(iva);
err_mbox_prb:
	iva_mcu_boot_mem_free(iva, iva->mcu_bin);
err_boot_mem:
	devm_kfree(dev, iva->mcu_bin);
	iva->mcu_bin = NULL;
	return ret;
}

void iva_mcu_remove(struct iva_dev_data *iva)
{
	iva_ram_dump_deinit(iva);

	iva_ipcq_remove(iva);

	iva_mbox_remove(iva);

	if (iva->mcu_bin) {
		iva_mcu_boot_mem_free(iva, iva->mcu_bin);

		devm_kfree(iva->dev, iva->mcu_bin);
		iva->mcu_bin = NULL;
	}
}

void iva_mcu_shutdown(struct iva_dev_data *iva)
{
	/* forced to block safely at minimum: - report to mcu? */
	iva_mcu_print_stop(iva);

	/* forced to block mbox interrupt */
	iva_mbox_remove(iva);
}
