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
#include <linux/delay.h>

#include "regs/iva_base_addr.h"
#include "iva_sh_mem.h"

#define ENABLE_MCU_INIT_CHECK
#define VALUE_CHECK_DECREMENT	(10)	/* us */

static int __maybe_unused sh_mem_wait_mcu_init(struct iva_dev_data *iva, uint32_t to_ms)
{
	int ret	= (int) to_ms;
	struct device	*dev = iva->dev;
	struct sh_mem_info *sh_mem = iva->shmem_va;

	while (sh_mem->magic2 != SHMEM_MAGIC2_INIT &&
			sh_mem->magic2 != SHMEM_MAGIC2_NORMAL &&
			sh_mem->magic2 != SHMEM_MAGIC2_NORM_EXIT &&
			sh_mem->magic2 != SHMEM_MAGIC2_FAULT) {
		if (ret < 0)
			return ret;
		usleep_range(VALUE_CHECK_DECREMENT, VALUE_CHECK_DECREMENT+1);
		ret -= VALUE_CHECK_DECREMENT;
	}

	if (sh_mem->magic1 != SHMEM_MAGIC1) {
		dev_err(dev, "%s() FATAL: magic1(0x%x) != (0x%x)\n",
			__func__, sh_mem->magic1, SHMEM_MAGIC1);
		return -2;
	}

	dev_dbg(dev, "%s() magic1(0x%x) magic2(0x%x)\n",
		__func__, sh_mem->magic1, sh_mem->magic2);

	return 0;
}

/* to_us: timeout in us */
int sh_mem_wait_mcu_ready(struct iva_dev_data *iva, uint32_t to_us)
{
	struct sh_mem_info *sh_mem = sh_mem_get_sm_pointer(iva);
	int ret	= (int) to_us;

	while (sh_mem->magic2 != SHMEM_MAGIC2_NORMAL) {
		if (ret < 0)
			return -1;
		usleep_range(VALUE_CHECK_DECREMENT, VALUE_CHECK_DECREMENT+1);
		ret -= VALUE_CHECK_DECREMENT;
	}

	return 0;
}

int sh_mem_wait_stop(struct iva_dev_data *iva, uint32_t to_us)
{
	struct sh_mem_info *sh_mem = sh_mem_get_sm_pointer(iva);
	int ret = (int) to_us;

	while (sh_mem->magic2 != SHMEM_MAGIC2_NORM_EXIT &&
			sh_mem->magic2 != SHMEM_MAGIC2_FAULT &&
			sh_mem->magic2 != SHMEM_MAGIC2_FATAL) {
		if (ret < 0)
			return -1;
		usleep_range(VALUE_CHECK_DECREMENT, VALUE_CHECK_DECREMENT+1);
		ret -= VALUE_CHECK_DECREMENT;
	}
	return 0;
}


void sh_mem_dump_info(struct iva_dev_data *iva)
{
	struct sh_mem_info	*sh_mem = sh_mem_get_sm_pointer(iva);
	struct device		*dev = iva->dev;

	dev_info(dev, "===============(sh_mem, %p)=================\n", sh_mem);
	dev_info(dev, "magic1(0x%08x), size(0x%08x)\n",
			sh_mem->magic1, sh_mem->shmem_size);
	dev_info(dev, "build_info(%s)\n", sh_mem->build_info);
	dev_info(dev, "magic2(0x%08x)\n", sh_mem->magic2);
	dev_info(dev, "log_pos(%p), log_size(0x%x) head(%d) tail(%d)\n",
			&sh_mem->log_pos, sh_mem->log_buf_size,
			sh_mem->log_pos.head, sh_mem->log_pos.tail);
	dev_info(dev, "---------------------------------------------\n");
}

int sh_mem_init(struct iva_dev_data *iva)
{
	#define WAIT_DELAY_US	(5000)	/* about 5 msec */
#ifdef ENABLE_MCU_INIT_CHECK
	int ret;
#endif
	struct device	*dev = iva->dev;
	uint32_t	iva_base_offset;

	if (iva->shmem_va) {
		dev_warn(dev, "%s() already mapped into shmem_va(%p)\n",
				__func__, iva->shmem_va);
		return 0;
	}

	if (!iva->iva_va) {
		dev_err(dev, "%s() null iva_va\n", __func__);
		return -EINVAL;
	}

#if defined(CONFIG_SOC_EXYNOS8895) || defined(CONFIG_SOC_EXYNOS9810)
	iva_base_offset = IVA_VMCU_MEM_BASE_ADDR +
			iva->mcu_mem_size - iva->mcu_shmem_size;
	iva->shmem_va = iva->iva_va + iva_base_offset;
#elif defined(CONFIG_SOC_EXYNOS9820)
	if (iva->mcu_split_sram) {
		iva_base_offset = IVA_VMCU_MEM_BASE_ADDR +
				iva->mcu_mem_size - iva->mcu_shmem_size;
		iva->shmem_va = iva->iva_va + iva_base_offset;
	} else {
		iva_base_offset = iva->mcu_mem_size - iva->mcu_shmem_size;
		iva->shmem_va = iva->mcu_bin->bin + iva_base_offset;
	}
#endif

	dev_dbg(dev, "%s() mapped addr(0x%p)\n", __func__, iva->shmem_va);

	sh_mem_set_iva_clk_rate(iva, iva->iva_qos_rate);
#ifdef ENABLE_MCU_INIT_CHECK
	ret = sh_mem_wait_mcu_init(iva, WAIT_DELAY_US);
	if (ret) {
		dev_err(dev, "%s() fail to wait for proper mcu init. ret(%d)\n",
				__func__, ret);
		iva->shmem_va = NULL;
		return ret;

	}
#endif
	dev_dbg(dev, "%s() mcu inited successfully, mapped addr(0x%p)\n",
			__func__, iva->shmem_va);

	return 0;
}

int sh_mem_deinit(struct iva_dev_data *iva)
{
	if (iva->shmem_va)
		iva->shmem_va = NULL;

	return 0;
}
