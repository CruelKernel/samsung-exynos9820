/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <linux/pm_qos.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/clk-provider.h>

#if defined(CONFIG_SOC_EMULATOR9820) || defined(CONFIG_SOC_EXYNOS9820)
#include <linux/io.h>
#include <linux/delay.h>
#endif

#include "npu-log.h"
#include "npu-device.h"
#include "npu-system.h"
#include "npu-system-soc.h"

#ifdef CONFIG_NPU_HARDWARE
#include "mailbox_ipc.h"
#endif

#ifdef CONFIG_FIRMWARE_SRAM_DUMP_DEBUGFS
#include "npu-util-memdump.h"
#endif

#if 0
#define CAM_L0 690000
#define CAM_L1 680000
#define CAM_L2 670000
#define CAM_L3 660000
#define CAM_L4 650000
#define CAM_L5 640000

#define MIF_L0 2093000
#define MIF_L1 2002000
#define MIF_L2 1794000
#define MIF_L3 1539000
#define MIF_L4 1352000
#define MIF_L5 1014000
#define MIF_L6 845000
#define MIF_L7 676000
#endif

struct pm_qos_request exynos_npu_qos_cam;
struct pm_qos_request exynos_npu_qos_mem;

struct system_pwr sysPwr;

#if defined(CONFIG_SOC_EMULATOR9820) || defined(CONFIG_SOC_EXYNOS9820)
#define OFFSET_END	0xFFFFFFFF

/* Initialzation steps for system_resume */
enum npu_system_resume_steps {
	NPU_SYS_RESUME_SETUP_WAKELOCK,
	NPU_SYS_RESUME_INIT_FWBUF,
	NPU_SYS_RESUME_FW_LOAD,
	NPU_SYS_RESUME_CLK_PREPARE,
	NPU_SYS_RESUME_SOC,
	NPU_SYS_RESUME_OPEN_INTERFACE,
	NPU_SYS_RESUME_COMPLETED
};

static int npu_firmware_load(struct npu_system *system);

#endif
#ifdef CONFIG_EXYNOS_NPU_DRAM_FW_LOG_BUF
#define DRAM_FW_LOG_BUF_SIZE	(2048*1024)
#define DRAM_FW_REPORT_BUF_SIZE	(1024*1024)
static struct npu_memory_buffer dram_fw_log_buf = {
	.size = DRAM_FW_LOG_BUF_SIZE,
};
static struct npu_memory_buffer fw_report_buf = {
	.size = DRAM_FW_REPORT_BUF_SIZE,
};


int npu_system_alloc_fw_dram_log_buf(struct npu_system *system)
{
	int ret;
	BUG_ON(!system);

	npu_info("start: initialization.\n");

	/* Request log buf allocation */
	ret = npu_memory_alloc(&system->memory, &dram_fw_log_buf);
	if (ret) {
		npu_err("fail(%d) in Log buffer memory allocation\n", ret);
		return ret;
	}

	npu_info("DRAM log buffer for firmware: size(%d) / dv(%pad) / kv(%pK)\n",
		 DRAM_FW_LOG_BUF_SIZE, &dram_fw_log_buf.daddr, dram_fw_log_buf.vaddr);

	/* Initialize memory logger dram log buf */
	npu_store_log_init(dram_fw_log_buf.vaddr, dram_fw_log_buf.size);

	if (!fw_report_buf.vaddr) {
		ret = npu_memory_alloc(&system->memory, &fw_report_buf);
		if (ret) {
			npu_err("fail(%d) in Log buffer memory allocation\n", ret);
			return ret;
		}
		npu_fw_report_init(fw_report_buf.vaddr, fw_report_buf.size);
	} else {//Case of fw_report is already allocated by ion memory
		npu_dbg("fw_report is already initialized - %pK.\n", fw_report_buf.vaddr);
	}

	/* Initialize firmware utc handler with dram log buf */
	ret = npu_fw_test_initialize(system, &dram_fw_log_buf);
	if (ret) {
		npu_err("npu_fw_test_initialize() failed : ret = %d\n", ret);
		return ret;
	}
	npu_info("complete : initialization.\n");
	return 0;
}

static int npu_system_free_fw_dram_log_buf(struct npu_system *system)
{
	BUG_ON(!system);

	/* De-initialize memory logger dram log buf */
	npu_store_log_deinit();

	npu_memory_free(&system->memory, &dram_fw_log_buf);

	npu_info("DRAM log buffer for firmware freed.\n");

	return 0;
}

#else
#define npu_system_alloc_fw_dram_log_buf(t)	(0)
#define npu_system_free_fw_dram_log_buf(t)	(0)
#endif	/* CONFIG_EXYNOS_NPU_DRAM_FW_LOG_BUF */

int npu_system_probe(struct npu_system *system, struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev;
	void *addr;
	int irq;

	BUG_ON(!system);
	BUG_ON(!pdev);

	dev = &pdev->dev;
	system->pdev = pdev;	/* TODO: Reference counting ? */
	system->cam_qos = 0;
	system->mif_qos = 0;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		probe_err("fail(%d) in platform_get_irq(0)\n", irq);
		ret = -EINVAL;
		goto p_err;
	}
	system->irq0 = irq;

	irq = platform_get_irq(pdev, 1);
	if (irq < 0) {
		probe_err("fail(%d) in platform_get_irq(1)\n", irq);
		ret = -EINVAL;
		goto p_err;
	}
	system->irq1 = irq;

	ret = npu_memory_probe(&system->memory, dev);
	if (ret) {
		npu_err("fail(%d) in npu_memory_probe\n", ret);
		goto p_err;
	}

	/* Invoke platform specific probe routine */
	ret = npu_system_soc_probe(system, pdev);
	if (ret) {
		probe_err("fail(%d) in npu_system_soc_probe\n", ret);
		goto p_err;
	}

	addr = (void *)(system->mbox_sfr.vaddr);
	ret = npu_interface_probe(dev, addr);
	if (ret) {
		npu_err("fail(%d) in npu_interface_probe\n", ret);
		goto p_err;
	}

	ret = npu_binary_init(&system->binary,
		dev,
		NPU_FW_PATH1,
		NPU_FW_PATH2,
		NPU_FW_NAME);
	if (ret) {
		npu_err("fail(%d) in npu_binary_init\n", ret);
		goto p_err;
	}
	ret = npu_util_memdump_probe(system);
	if (ret) {
		npu_err("fail(%d) in npu_util_memdump_probe\n", ret);
		goto p_err;
	}

	/* get npu clock */
	system->npu_clk = devm_clk_get(dev, "clk_npu");
	if (IS_ERR(system->npu_clk)) {
		npu_err("%s() fail to get clk_npu\n", __func__);
		ret = PTR_ERR(system->npu_clk);
		goto p_err;
	}

	/* enable runtime pm */
	pm_runtime_enable(dev);

	ret = npu_qos_probe(system);
	if (ret) {
		npu_err("npu_qos_probe is fail(%d)\n", ret);
		goto p_qos_err;
	}

#ifdef CONFIG_PM_SLEEP
	/* initialize the npu wake lock */
	wake_lock_init(&system->npu_wake_lock, WAKE_LOCK_SUSPEND, "npu_run_wlock");
#endif
	init_waitqueue_head(&sysPwr.wq);
	sysPwr.system_result.result_code = NPU_SYSTEM_JUST_STARTED;
	goto p_exit;

p_qos_err:
	pm_runtime_disable(dev);
	if (system->npu_clk)
		devm_clk_put(dev, system->npu_clk);
p_err:
p_exit:
	return ret;
}

/* TODO: Implement throughly */
int npu_system_release(struct npu_system *system, struct platform_device *pdev)
{
	int ret;
	struct device *dev;

	BUG_ON(!system);
	BUG_ON(!pdev);

	dev = &pdev->dev;

#ifdef CONFIG_PM_SLEEP
	wake_lock_destroy(&system->npu_wake_lock);
#endif

	pm_runtime_disable(dev);

	if (system->npu_clk) {
		devm_clk_put(dev, system->npu_clk);
		platform_set_drvdata(pdev, NULL);
	}

	ret = npu_qos_release(system);
	if (ret)
		npu_err("fail(%d) in npu_qos_release\n", ret);

	/* Invoke platform specific release routine */
	ret = npu_system_soc_release(system, pdev);
	if (ret)
		npu_err("fail(%d) in npu_system_soc_release\n", ret);

	return 0;
}

int npu_system_open(struct npu_system *system)
{
	int ret = 0;
	struct device *dev;

	BUG_ON(!system);
	BUG_ON(!system->pdev);
	dev = &system->pdev->dev;

	ret = npu_memory_open(&system->memory);
	if (ret) {
		npu_err("fail(%d) in npu_memory_open\n", ret);
		goto p_err;
	}

	ret = npu_util_memdump_open(system);
	if (ret) {
		npu_err("fail(%d) in npu_util_memdump_open\n", ret);
		goto p_err;
	}

	/* Clear resume steps */
	system->resume_steps = 0;
p_err:
	return ret;
}

int npu_system_close(struct npu_system *system)
{
	int ret = 0;

#ifdef CONFIG_FIRMWARE_SRAM_DUMP_DEBUGFS
	ret = npu_util_memdump_close(system);
	if (ret)
		npu_err("fail(%d) in npu_util_memdump_close\n", ret);
#endif
	ret = npu_memory_close(&system->memory);
	if (ret)
		npu_err("fail(%d) in npu_memory_close\n", ret);
	return ret;
}

int npu_system_resume(struct npu_system *system, u32 mode)
{
	int ret = 0;
	void *addr;
	struct device *dev;
	struct clk *npu_clk;
	struct npu_device *device;

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	dev = &system->pdev->dev;
	npu_clk = system->npu_clk;
	device = container_of(system, struct npu_device, system);

	/* Clear resume steps */
	system->resume_steps = 0;

#ifdef CONFIG_PM_SLEEP
	/* prevent the system to suspend */
	if (!wake_lock_active(&system->npu_wake_lock)) {
		wake_lock(&system->npu_wake_lock);
		npu_info("wake_lock, now(%d)\n", wake_lock_active(&system->npu_wake_lock));
	}
	set_bit(NPU_SYS_RESUME_SETUP_WAKELOCK, &system->resume_steps);
#endif

	ret = npu_system_alloc_fw_dram_log_buf(system);
	if (ret) {
		npu_err("fail(%d) in npu_system_alloc_fw_dram_log_buf\n", ret);
		goto p_err;
	}
	set_bit(NPU_SYS_RESUME_INIT_FWBUF, &system->resume_steps);

	ret = npu_firmware_load(system);
	if (ret) {
		npu_err("fail(%d) in npu_firmware_load\n", ret);
		goto p_err;
	}
	set_bit(NPU_SYS_RESUME_FW_LOAD, &system->resume_steps);

	if (npu_clk) {
		ret = clk_prepare_enable(npu_clk);
		if (ret) {
			npu_err("fail to enable npu_clk(%d)\n", ret);
			goto p_err;
		}
	}
	set_bit(NPU_SYS_RESUME_CLK_PREPARE, &system->resume_steps);

	/* Invoke platform specific resume routine */
	ret = npu_system_soc_resume(system, mode);
	if (ret) {
		npu_err("fail(%d) in npu_system_soc_resume\n", ret);
		goto p_err;
	}
	set_bit(NPU_SYS_RESUME_SOC, &system->resume_steps);

	addr = (void *)(system->tcu_sram.vaddr);
	system->mbox_hdr = (volatile struct mailbox_hdr *)(addr + NPU_MAILBOX_BASE - sizeof(struct mailbox_hdr));
	ret = npu_interface_open(system);
	if (ret) {
		npu_err("fail(%d) in npu_interface_open\n", ret);
		goto p_err;
	}
	set_bit(NPU_SYS_RESUME_OPEN_INTERFACE, &system->resume_steps);

	set_bit(NPU_SYS_RESUME_COMPLETED, &system->resume_steps);

	return ret;
p_err:
	npu_err("Failure detected[%d]. Set emergency recovery flag.\n", ret);
	set_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state);
	ret = 0;//emergency case will be cared by suspend func
	return ret;
}

int npu_system_suspend(struct npu_system *system)
{
	int ret = 0;
	struct device *dev;
	struct clk *npu_clk;
	struct npu_device *device;

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	dev = &system->pdev->dev;
	npu_clk = system->npu_clk;
	device = container_of(system, struct npu_device, system);

	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_COMPLETED, &system->resume_steps, NULL, ;);

	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_OPEN_INTERFACE, &system->resume_steps, "Close interface", {
		ret = npu_interface_close(system);
		if (ret)
			npu_err("fail(%d) in npu_interface_close\n", ret);
	});

	/* Invoke platform specific suspend routine */
	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_SOC, &system->resume_steps, "SoC suspend", {
		ret = npu_system_soc_suspend(system);
		if (ret)
			npu_err("fail(%d) in npu_system_soc_suspend\n", ret);
	});

#ifdef CONFIG_PM_SLEEP
	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_SETUP_WAKELOCK, &system->resume_steps, "Unlock wake lock", {
		if (wake_lock_active(&system->npu_wake_lock)) {
			wake_unlock(&system->npu_wake_lock);
			npu_dbg("wake_unlock, now(%d)\n", wake_lock_active(&system->npu_wake_lock));
		}
	});
#endif

	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_CLK_PREPARE, &system->resume_steps, "Unprepare clk", {
		if (npu_clk) {
			clk_disable_unprepare(npu_clk);
			/* check */
			if (__clk_is_enabled(npu_clk))
				npu_err("%s() req npu_clk off but on\n", __func__);
		}
	});

#if 0 // 0521_CLEAN_CODE
	ret = npu_memory_close(&system->memory);
	if (ret) {
		npu_err("fail(%d) in vpu_memory_close\n", ret);
		goto p_err;
	}
#endif

	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_FW_LOAD, &system->resume_steps, NULL, ;);
	BIT_CHECK_AND_EXECUTE(NPU_SYS_RESUME_INIT_FWBUF, &system->resume_steps, "Free DRAM fw log buf", {
		ret = npu_system_free_fw_dram_log_buf(system);
		if (ret)
			npu_err("fail(%d) in npu_cpu_off\n", ret);
	});

	if (system->resume_steps != 0)
		npu_warn("Missing clean-up steps [%lu] found.\n", system->resume_steps);

	/* Function itself never be failed, even thought there was some error */
	return 0;
}

int npu_system_start(struct npu_system *system)
{
	int ret = 0;
	struct device *dev;

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	dev = &system->pdev->dev;

#ifdef CONFIG_FIRMWARE_SRAM_DUMP_DEBUGFS
	ret = npu_util_memdump_start(system);
	if (ret) {
		npu_err("fail(%d) in npu_util_memdump_start\n", ret);
		goto p_err;
	}
#endif

	ret = npu_qos_start(system);
	if (ret) {
		npu_err("fail(%d) in npu_qos_start\n", ret);
		goto p_err;
	}

p_err:

	return ret;
}

int npu_system_stop(struct npu_system *system)
{
	int ret = 0;
	struct device *dev;

	BUG_ON(!system);
	BUG_ON(!system->pdev);

	dev = &system->pdev->dev;
#ifdef CONFIG_FIRMWARE_SRAM_DUMP_DEBUGFS
	ret = npu_util_memdump_stop(system);
	if (ret) {
		npu_err("fail(%d) in npu_util_memdump_stop\n", ret);
		goto p_err;
	}
#endif

	ret = npu_qos_stop(system);
	if (ret) {
		npu_err("fail(%d) in npu_qos_stop\n", ret);
		goto p_err;
	}
p_err:

	return 0;
}

int npu_system_save_result(struct npu_session *session, struct nw_result nw_result)
{
	int ret = 0;
	sysPwr.system_result.result_code = nw_result.result_code;
	wake_up(&sysPwr.wq);
	return ret;
}

static int npu_firmware_load(struct npu_system *system)
{
	int ret = 0;
	u32 v;

	BUG_ON(!system);
	npu_info("Firmware load : Start\n");
#ifdef CLEAR_SRAM_ON_FIRMWARE_LOADING
#ifdef CLEAR_ON_SECOND_LOAD_ONLY
	v = readl(system->tcu_sram.vaddr + system->tcu_sram.size - sizeof(u32));
	npu_dbg("firmware load: Check current signature value : 0x%08x (%s)\n",
		v, (v == 0)?"First load":"Second load");
#else
	v = 1;
#endif
	if (v != 0) {
		npu_dbg("firmware load : clear TCU SRAM at %pK, Len(%llu)\n",
			system->tcu_sram.vaddr, system->tcu_sram.size);
		/* Using memset here causes unaligned access fault.
		Refer: https://patchwork.kernel.org/patch/6362401/ */
		memset_io(system->tcu_sram.vaddr, 0, system->tcu_sram.size);
		npu_dbg("firmware load: clear IDP SRAM at %pK, Len(%llu)\n",
			system->idp_sram.vaddr, system->idp_sram.size);
		memset_io(system->idp_sram.vaddr, 0, system->idp_sram.size);
	}
#else
	npu_dbg("firmware load: clear firmware signature at %pK(u64)\n",
		system->tcu_sram.vaddr + system->tcu_sram.size - sizeof(u64));
	writel(0, system->tcu_sram.vaddr + system->tcu_sram.size - sizeof(u64));
#endif
	npu_dbg("firmware load: read and locate firmware to %pK\n", system->tcu_sram.vaddr);
	ret = npu_firmware_file_read(&system->binary, system->tcu_sram.vaddr, system->tcu_sram.size);
	if (ret) {
		npu_err("error(%d) in npu_binary_read\n", ret);
		goto err_exit;
	}
	npu_dbg("checking firmware head MAGIC(0x%08x)\n", *(u32 *)system->tcu_sram.vaddr);

	npu_info("complete in npu_firmware_load\n");
	return 0;
err_exit:

	npu_info("error(%d) in npu_firmware_load\n", ret);
	return ret;
}

