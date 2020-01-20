/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/debug-snapshot.h>

#include <soc/samsung/exynos-cpupm.h>
#include <soc/samsung/exynos-adv-tracer.h>
#include <soc/samsung/exynos-adv-tracer-ipc.h>

#define DONE_ARRYDUMP 0xADADADAD

static struct adv_tracer_info *exynos_adv_tracer;
static int arraydump_done = DONE_ARRYDUMP;

int adv_tracer_arraydump(void)
{
	struct adv_tracer_ipc_cmd cmd;
	int ret = 0;
	u32 cpu_mask = (1 << CONFIG_NR_CPUS) - 1;

	if (arraydump_done == DONE_ARRYDUMP) {
		dev_info(exynos_adv_tracer->dev, "Arraydump already done(0x%x)\n", cpu_mask);
		return -1;
	}
	arraydump_done = DONE_ARRYDUMP;

	disable_power_mode(6, POWERMODE_TYPE_CLUSTER);
	dev_info(exynos_adv_tracer->dev, "Start Arraydump (0x%x)\n", cpu_mask);
	cmd.cmd_raw.cmd = EAT_IPC_CMD_ARRAYDUMP;
	cmd.cmd_raw.id = ARR_IPC_CMD_ID_KERNEL_ARRAYDUMP;
	cmd.buffer[1] = dbg_snapshot_get_item_paddr("log_arraydump");
	cmd.buffer[2] = cpu_mask;
	ret = adv_tracer_ipc_send_data_polling_timeout(EAT_FRM_CHANNEL, &cmd, EAT_IPC_TIMEOUT * 100);
	if (ret < 0)
		goto end;

	dev_info(exynos_adv_tracer->dev, "Finish Arraydump (0x%x)\n", cmd.buffer[1]);
end:
	enable_power_mode(6, POWERMODE_TYPE_CLUSTER);
	return ret;
}

static int adv_tracer_probe(struct platform_device *pdev)
{
	struct adv_tracer_info *adv_tracer;
	int ret = 0;

	dev_info(&pdev->dev, "[EAT+] %s\n", __func__);

	adv_tracer = devm_kzalloc(&pdev->dev,
				sizeof(struct adv_tracer_info), GFP_KERNEL);

	if (IS_ERR(adv_tracer))
		return PTR_ERR(adv_tracer);

	adv_tracer->dev = &pdev->dev;

	if (adv_tracer_ipc_init(pdev))
		goto out;

	exynos_adv_tracer = adv_tracer;

	dev_info(&pdev->dev, "[EAT-] %s\n", __func__);
out:
	return ret;
}

static int adv_tracer_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id adv_tracer_match[] = {
	{ .compatible = "samsung,exynos-adv-tracer" },
	{},
};

static struct platform_driver samsung_adv_tracer_driver = {
	.probe	= adv_tracer_probe,
	.remove	= adv_tracer_remove,
	.driver	= {
		.name = "exynos-adv-tracer",
		.owner	= THIS_MODULE,
		.of_match_table	= adv_tracer_match,
	},
};

static int __init exynos_adv_tracer_init(void)
{
	return platform_driver_register(&samsung_adv_tracer_driver);
}
arch_initcall_sync(exynos_adv_tracer_init);
