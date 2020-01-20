/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/ktime.h>
#include <linux/io.h>
#include <linux/sched/clock.h>

#include <soc/samsung/exynos-bcm_dbg.h>
#include <soc/samsung/exynos-bcm_dbg-dump.h>

static char file_name[128];

int exynos_bcm_dbg_buffer_dump(struct exynos_bcm_dbg_data *data, bool klog)
{
	void __iomem *v_addr = data->dump_addr.v_addr;
	u32 buff_size = data->dump_addr.buff_size - EXYNOS_BCM_KTIME_SIZE;
	u32 buff_cnt = 0;
	u32 dump_entry_size = sizeof(struct exynos_bcm_dump_info);
	struct exynos_bcm_dump_info *dump_info = NULL;
	u32 defined_event, ip_index;
	char *result;
	ssize_t str_size;
	u32 tmp_ktime[2];
	u64 last_ktime;
	struct file *fp = NULL;
	mm_segment_t old_fs = get_fs();

	if (!data->dump_addr.p_addr) {
		BCM_ERR("%s: No memory region for dump\n", __func__);
		return -ENOMEM;
	}

	if (in_interrupt()) {
		BCM_INFO("%s: skip file dump in interrupt context\n", __func__);
		return 0;
	}

	str_size = snprintf(file_name, PAGE_SIZE, "/data/result_bcm_%llu.csv",
				cpu_clock(raw_smp_processor_id()));

	result = kzalloc(sizeof(char) * BCM_DUMP_MAX_STR, GFP_KERNEL);
	if (result == NULL) {
		BCM_ERR("%s: faild allocated of result memory\n", __func__);
		return -ENOMEM;
	}

	tmp_ktime[0] = __raw_readl(v_addr);
	tmp_ktime[1] = __raw_readl(v_addr + 0x4);
	last_ktime = (((u64)tmp_ktime[1] << EXYNOS_BCM_32BIT_SHIFT) &
			EXYNOS_BCM_U64_HIGH_MASK) |
			((u64)tmp_ktime[0] & EXYNOS_BCM_U64_LOW_MASK);

	dump_info = (struct exynos_bcm_dump_info *)(v_addr + EXYNOS_BCM_KTIME_SIZE);

	set_fs(KERNEL_DS);

	fp = filp_open(file_name, O_WRONLY|O_CREAT|O_APPEND, 0);
	if (IS_ERR(fp)) {
		BCM_ERR("%s: name: %s filp_open fail\n", __func__, file_name);
		set_fs(old_fs);
		kfree(result);
		return IS_ERR(fp);
	}

	str_size = snprintf(result, PAGE_SIZE, "last kernel time, %llu\n", last_ktime);
	vfs_write(fp, result, str_size, &fp->f_pos);

	str_size = snprintf(result, PAGE_SIZE, "seq_no, ip_index, define_event, time, ccnt, "
			"pmcnt0, pmcnt1, pmcnt2, pmcnt3, pmcnt4, pmcnt5, pmcnt6, pmcnt7\n");
	vfs_write(fp, result, str_size, &fp->f_pos);

	if (klog)
		pr_info("%s", result);

	while ((buff_size - buff_cnt) > dump_entry_size) {
		defined_event = BCM_CMD_GET(dump_info->dump_header,
			BCM_EVT_PRE_DEFINE_MASK, BCM_DUMP_PRE_DEFINE_SHIFT);
		ip_index = BCM_CMD_GET(dump_info->dump_header, BCM_IP_MASK, 0);

		str_size = snprintf(result, PAGE_SIZE, "%u, %u, %u, %u, "
				"%u, %u, %u, %u, %u, %u, %u, %u, %u\n",
				dump_info->dump_seq_no, ip_index, defined_event,
				dump_info->dump_time, dump_info->out_data.ccnt,
				dump_info->out_data.pmcnt[0], dump_info->out_data.pmcnt[1],
				dump_info->out_data.pmcnt[2], dump_info->out_data.pmcnt[3],
				dump_info->out_data.pmcnt[4], dump_info->out_data.pmcnt[5],
				dump_info->out_data.pmcnt[6], dump_info->out_data.pmcnt[7]);
		vfs_write(fp, result, str_size, &fp->f_pos);

		if (klog)
			pr_info("%s", result);

		dump_info++;
		buff_cnt += dump_entry_size;
	}

	filp_close(fp, NULL);
	set_fs(old_fs);
	kfree(result);

	return 0;
}
