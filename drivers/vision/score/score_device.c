/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>

#if defined(CONFIG_EXYNOS_IOVMM)
#include <linux/exynos_iovmm.h>
#endif

#include "score_log.h"
#include "score_dpmu.h"
#include "score_sysfs.h"
#include "score_device.h"

#if defined(CONFIG_EXYNOS_IOVMM)
static int __attribute__((unused)) score_fault_handler(
		struct iommu_domain *domain, struct device *dev,
		unsigned long fault_addr, int fault_flag, void *token)
{
	struct score_device *device = dev_get_drvdata(dev);
	struct score_frame_manager *framemgr;

	pr_err("< SCore IOMMU FAULT HANDLER >\n");
	pr_err("Device virtual(0x%lX) is invalid access\n", fault_addr);
	pr_err("Print SCore device information by using DPMU\n");

	score_dpmu_print_all();

	framemgr = &device->system.interface.framemgr;
	score_info("< frame count >\n");
	score_info(" all:%d, normal:%d, abnormal:%d\n",
			framemgr->all_count, framemgr->normal_count,
			framemgr->abnormal_count);
	score_info(" entire:%d, ready:%d, process:%d(H:%d/N:%d)\n",
			framemgr->entire_count, framemgr->ready_count,
			framemgr->process_count, framemgr->process_high_count,
			framemgr->process_normal_count);
	score_info(" pending:%d(H:%d/N:%d), complete:%d\n",
			framemgr->pending_count, framemgr->pending_high_count,
			framemgr->pending_normal_count,
			framemgr->complete_count);

	return -EINVAL;
}
#endif

#if defined(CONFIG_PM_SLEEP)
static int score_device_suspend(struct device *dev)
{
	int ret = 0;
	struct score_device *device;
	struct score_pm *pm;
	struct score_system *system;
	struct score_frame_manager *framemgr;
	unsigned long flags;

	score_enter();
	device = dev_get_drvdata(dev);
	pm = &device->pm;

	if (score_pm_qos_active(pm)) {
		system = &device->system;
		framemgr = &system->interface.framemgr;

		spin_lock_irqsave(&framemgr->slock, flags);
		score_frame_manager_block(framemgr);
		score_frame_flush_all(framemgr, -ENOSTR);
		spin_unlock_irqrestore(&framemgr->slock, flags);
		wake_up_all(&framemgr->done_wq);

		score_print_close(&system->print);
		ret = score_system_halt(system);
		ret = score_system_sw_reset(system);
		score_pm_qos_suspend(pm);
	}

	score_leave();
	return ret;
}

static int score_device_resume(struct device *dev)
{
	int ret = 0;
	struct score_device *device;
	struct score_pm *pm;
	struct score_system *system;
	struct score_frame_manager *framemgr;
	unsigned long flags;

	score_enter();
	device = dev_get_drvdata(dev);
	pm = &device->pm;

	if (score_pm_qos_active(pm)) {
		system = &device->system;
		framemgr = &system->interface.framemgr;

		score_pm_qos_resume(pm);
		score_print_open(&system->print);
		ret = score_system_boot(system);

		spin_lock_irqsave(&framemgr->slock, flags);
		score_frame_manager_unblock(framemgr);
		spin_unlock_irqrestore(&framemgr->slock, flags);
	}

	score_leave();
	return ret;
}
#endif

static int score_device_runtime_suspend(struct device *dev)
{
	int ret = 0;
	struct score_device *device;

	score_enter();
	device = dev_get_drvdata(dev);

#if defined(CONFIG_EXYNOS_IOVMM)
	iovmm_deactivate(dev);
#endif
	score_clk_close(&device->clk);
	score_pm_close(&device->pm);

	score_leave();
	return ret;
}

static int score_device_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct score_device *device;

	score_enter();
	device = dev_get_drvdata(dev);

	ret = score_pm_open(&device->pm);
	if (ret)
		goto p_err_pm;

	ret = score_clk_open(&device->clk);
	if (ret)
		goto p_err_clk;

#if defined(CONFIG_EXYNOS_IOVMM)
	ret = iovmm_activate(dev);
#endif
	if (ret) {
		score_err("Failed to activate iommu (%d)\n", ret);
		goto p_err_resume;
	}

	score_leave();
	return ret;
p_err_resume:
	score_clk_close(&device->clk);
p_err_clk:
	score_pm_close(&device->pm);
p_err_pm:
	return ret;
}

static const struct dev_pm_ops score_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(
			score_device_suspend,
			score_device_resume)
	SET_RUNTIME_PM_OPS(
			score_device_runtime_suspend,
			score_device_runtime_resume,
			NULL)
};

static int __score_device_power_on(struct score_device *device)
{
#if defined(CONFIG_PM)
	return pm_runtime_get_sync(device->dev);
#else
	return score_device_runtime_resume(device->dev);
#endif
}

static void __score_device_power_off(struct score_device *device)
{
#if defined(CONFIG_PM)
	pm_runtime_put_sync(device->dev);
#else
	score_device_runtime_suspend(device->dev);
#endif
}

static int __score_device_open(struct score_device *device)
{
	int ret = 0;
	struct score_system *system;

	score_enter();
	system = &device->system;
	ret = score_system_open(system);
	if (ret)
		goto p_err;

	device->cur_firmware_id = -1;
	device->state = BIT(SCORE_DEVICE_STATE_OPEN);
	score_info("The device is opened (not boot)\n");
	score_leave();
	return ret;
p_err:
	device->state = BIT(SCORE_DEVICE_STATE_CLOSE);
	score_info("The device failed to be opened\n");
	return ret;
}

static void __score_device_close(struct score_device *device)
{
	struct score_system *system;

	score_enter();
	if (!((device->state & BIT(SCORE_DEVICE_STATE_OPEN)) |
			(device->state & BIT(SCORE_DEVICE_STATE_STOP))))
		return;

	system = &device->system;
	score_system_close(system);

	device->state = BIT(SCORE_DEVICE_STATE_CLOSE);
	score_info("The device closed\n");
	score_leave();
}

static int __score_device_start(struct score_device *device,
		unsigned int firmware_id, unsigned int boot_qos)
{
	int ret = 0;
	struct score_pm *pm;
	struct score_system *system;
	struct score_frame_manager *framemgr;
	unsigned long flags;

	score_enter();
	pm = &device->pm;
	system = &device->system;
	framemgr = &system->interface.framemgr;

	ret = score_pm_qos_set_default(pm, boot_qos);
	if (ret)
		goto p_err_default_pm;

	ret = __score_device_power_on(device);
	if (ret)
		goto p_err_power_on;

	ret = score_system_start(system, firmware_id);
	if (ret)
		goto p_err_system;

	spin_lock_irqsave(&framemgr->slock, flags);
	score_frame_manager_unblock(framemgr);
	spin_unlock_irqrestore(&framemgr->slock, flags);

	device->cur_firmware_id = firmware_id;
	device->state = BIT(SCORE_DEVICE_STATE_START);
	score_info("The device successfully started (firmware:%u)\n",
			firmware_id);
	score_leave();
	return ret;
p_err_system:
	__score_device_power_off(device);
p_err_power_on:
p_err_default_pm:
	device->state = BIT(SCORE_DEVICE_STATE_STOP);
	score_info("The device failed to be started (firmware:%u)\n",
			firmware_id);
	return ret;
}

static void __score_device_stop(struct score_device *device)
{
	struct score_system *system;
	struct score_frame_manager *framemgr;
	unsigned long flags;

	score_enter();
	if (!(device->state & BIT(SCORE_DEVICE_STATE_START)))
		return;

	system = &device->system;
	framemgr = &system->interface.framemgr;

	spin_lock_irqsave(&framemgr->slock, flags);
	score_frame_manager_block(framemgr);
	score_frame_flush_all(framemgr, -ENOSTR);
	spin_unlock_irqrestore(&framemgr->slock, flags);
	wake_up_all(&framemgr->done_wq);
	score_frame_remove_nonblock_all(framemgr);

	score_system_stop(system);
	device->cur_firmware_id = -1;
	__score_device_power_off(device);

	device->state = BIT(SCORE_DEVICE_STATE_STOP);
	score_info("The device stopped\n");
	score_leave();
}

static int __score_device_restart(struct score_device *device,
		unsigned int firmware_id)
{
	int ret = 0;
	struct score_system *system;
	struct score_frame_manager *framemgr;
	unsigned long flags;

	score_enter();
	system = &device->system;
	framemgr = &system->interface.framemgr;

	spin_lock_irqsave(&framemgr->slock, flags);
	score_frame_manager_block(framemgr);
	score_frame_flush_all(framemgr, -ENOSTR);
	spin_unlock_irqrestore(&framemgr->slock, flags);
	wake_up_all(&framemgr->done_wq);
	score_frame_remove_nonblock_all(framemgr);

	score_system_stop(system);

	ret = score_system_start(system, firmware_id);
	if (ret)
		goto p_err_system;

	spin_lock_irqsave(&framemgr->slock, flags);
	score_frame_manager_unblock(framemgr);
	spin_unlock_irqrestore(&framemgr->slock, flags);

	device->cur_firmware_id = firmware_id;
	device->state = BIT(SCORE_DEVICE_STATE_START);
	score_info("The device successfully re-started(%u)\n", firmware_id);

	score_leave();
	return ret;
p_err_system:
	device->cur_firmware_id = -1;
	__score_device_power_off(device);
	device->state = BIT(SCORE_DEVICE_STATE_STOP);
	score_info("The device failed to be re-started(%u)\n", firmware_id);
	return ret;
}

int score_device_check_start(struct score_device *device)
{
	if (device->state & BIT(SCORE_DEVICE_STATE_START))
		return 0;
	else
		return device->state;
}

int score_device_start(struct score_device *device, unsigned int firmware_id,
		unsigned int boot_qos, unsigned int flag)
{
	int ret = 0;

	score_enter();
	if (device->state & BIT(SCORE_DEVICE_STATE_OPEN)) {
		ret = __score_device_start(device, firmware_id, boot_qos);
	} else if (device->state & BIT(SCORE_DEVICE_STATE_START)) {
		if (firmware_id != device->cur_firmware_id) {
			if (flag & SCORE_BOOT_FORCE) {
				ret = __score_device_restart(device,
						firmware_id);
			} else {
				ret = -EBUSY;
				score_warn("firmware(%u) is working(fail:%u)\n",
						device->cur_firmware_id,
						firmware_id);
			}
		} else {
			score_info("The device is already ready(%u)\n",
					device->cur_firmware_id);
		}
	} else {
		ret = -EINVAL;
		score_err("State(%#x) of the device is wrong\n", device->state);
	}

	score_leave();
	return ret;
}

int score_device_open(struct score_device *device)
{
	int ret = 0;

	score_enter();
	if (!atomic_read(&device->open_count)) {
		ret = -ENODEV;
		score_err("device is not opened\n");
		goto p_err;
	}

	if (device->state & BIT(SCORE_DEVICE_STATE_CLOSE))
		ret = __score_device_open(device);

	return ret;
p_err:
	return ret;
}

void score_device_get(struct score_device *device)
{
	score_enter();
	atomic_inc(&device->open_count);
	score_info("The device count incresed(count:%d)\n",
			atomic_read(&device->open_count));
	score_leave();
}

void score_device_put(struct score_device *device)
{
	int count;

	score_enter();
	if (!atomic_read(&device->open_count))
		return;

	count = atomic_dec_return(&device->open_count);
	score_info("The device count decresed(count:%d)\n", count);

	if (!count) {
		__score_device_stop(device);
		__score_device_close(device);
	}
	score_leave();
}

static int score_device_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct score_device *device;

	score_enter();
	device = devm_kzalloc(&pdev->dev, sizeof(struct score_device),
			GFP_KERNEL);
	if (!device) {
		ret = -ENOMEM;
		score_err("Fail to alloc device structure\n");
		goto p_exit;
	}

	device->pdev = pdev;
	device->dev = &pdev->dev;
	get_device(device->dev);
	dev_set_drvdata(device->dev, device);

	ret = score_pm_probe(device);
	if (ret)
		goto p_err_pm;

	ret = score_clk_probe(device);
	if (ret)
		goto p_err_clk;

	ret = score_system_probe(device);
	if (ret)
		goto p_err_system;

	ret = score_core_probe(device);
	if (ret)
		goto p_err_core;

#if defined(CONFIG_EXYNOS_IOVMM)
	iovmm_set_fault_handler(device->dev, score_fault_handler, NULL);
#endif
	ret = score_sysfs_probe(device);
	if (ret)
		goto p_err_sysfs;

	atomic_set(&device->open_count, 0);
	device->state = BIT(SCORE_DEVICE_STATE_CLOSE);

	score_leave();
	return ret;
p_err_sysfs:
	score_core_remove(&device->core);
p_err_core:
	score_system_remove(&device->system);
p_err_system:
	score_clk_remove(&device->clk);
p_err_clk:
	score_pm_remove(&device->pm);
p_err_pm:
	devm_kfree(&pdev->dev, device);
p_exit:
	return ret;
}

static int score_device_remove(struct platform_device *pdev)
{
	struct score_device *device = dev_get_drvdata(&pdev->dev);

	score_enter();
	score_sysfs_remove(device);
	score_core_remove(&device->core);
	score_system_remove(&device->system);
	score_clk_remove(&device->clk);
	score_pm_remove(&device->pm);
	put_device(device->dev);
	devm_kfree(&pdev->dev, device);
	score_leave();
	return 0;
}

static void score_device_shutdown(struct platform_device *pdev)
{
	struct score_device *device;
	struct score_pm *pm;
	struct score_system *system;
	struct score_frame_manager *framemgr;
	unsigned long flags;

	score_enter();
	device = dev_get_drvdata(&pdev->dev);
	pm = &device->pm;

	if (score_pm_qos_active(pm)) {
		system = &device->system;
		framemgr = &system->interface.framemgr;

		spin_lock_irqsave(&framemgr->slock, flags);
		score_frame_manager_block(framemgr);
		score_frame_flush_all(framemgr, -ENOSTR);
		spin_unlock_irqrestore(&framemgr->slock, flags);
		wake_up_all(&framemgr->done_wq);

		score_system_halt(system);
		score_system_sw_reset(system);
	}

	score_leave();
}

#if defined(CONFIG_OF)
static const struct of_device_id exynos_score_match[] = {
	{
		.compatible = "samsung,score",
	},
	{}
};
MODULE_DEVICE_TABLE(of, exynos_score_match);
#endif

static struct platform_driver score_driver = {
	.probe          = score_device_probe,
	.remove         = score_device_remove,
	.shutdown       = score_device_shutdown,
	.driver = {
		.name   = "exynos-score",
		.owner  = THIS_MODULE,
		.pm     = &score_pm_ops,
#if defined(CONFIG_OF)
		.of_match_table = of_match_ptr(exynos_score_match)
#endif
	}
};

static int __init score_device_init(void)
{
	int ret = 0;

	score_enter();
	ret = platform_driver_register(&score_driver);
	if (ret)
		score_err("platform_driver_register is fail (%d)\n", ret);

	score_leave();
	return ret;
}

static void __exit score_device_exit(void)
{
	score_enter();
	platform_driver_unregister(&score_driver);
	score_leave();
}

#if defined(MODULE)
module_init(score_device_init);
#else
late_initcall(score_device_init);
#endif
module_exit(score_device_exit);

MODULE_AUTHOR("@samsung.com>");
MODULE_DESCRIPTION("Exynos SCore driver");
MODULE_LICENSE("GPL");
