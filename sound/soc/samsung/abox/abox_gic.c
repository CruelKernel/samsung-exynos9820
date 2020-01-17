/* sound/soc/samsung/abox/abox_gic.c
 *
 * ALSA SoC Audio Layer - Samsung ABOX GIC driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* #define DEBUG */
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/smc.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/delay.h>

#include "abox_util.h"
#include "abox_gic.h"

#define GIC_IS_SECURE_FREE

void abox_gic_enable(struct device *dev, unsigned int irq, bool en)
{
	struct abox_gic_data *data = dev_get_drvdata(dev);
	unsigned int base = en ? GIC_DIST_ENABLE_SET : GIC_DIST_ENABLE_CLEAR;
	unsigned int offset = base + (irq / 32 * 4);
	unsigned int shift = irq % 32;
	unsigned int mask = 0x1 << shift;
	static DEFINE_SPINLOCK(lock);
	unsigned long flags;

	spin_lock_irqsave(&lock, flags);
	writel(mask, data->gicd_base + offset);
	spin_unlock_irqrestore(&lock, flags);
}

void abox_gic_target(struct device *dev, unsigned int irq,
		enum abox_gic_target target)
{
	struct abox_gic_data *data = dev_get_drvdata(dev);
	unsigned int offset = GIC_DIST_TARGET + (irq & 0xfffffffc);
	unsigned int shift = (irq & 0x3) * 8;
	unsigned int mask = 0xff << shift;
	unsigned int val;
	static DEFINE_SPINLOCK(lock);
	unsigned long flags;

	dev_dbg(dev, "%s(%d, %d)\n", __func__, irq, target);

	spin_lock_irqsave(&lock, flags);
	val = readl(data->gicd_base + offset);
	val &= ~mask;
	val |= ((0x1 << target) << shift) & mask;
	writel(val, data->gicd_base + offset);
	spin_unlock_irqrestore(&lock, flags);
}

void abox_gic_generate_interrupt(struct device *dev, unsigned int irq)
{
#ifdef GIC_IS_SECURE_FREE
	struct abox_gic_data *data = dev_get_drvdata(dev);
#endif
	dev_dbg(dev, "%s(%d)\n", __func__, irq);
#ifdef GIC_IS_SECURE_FREE
	writel((0x1 << 16) | (irq & 0xF),
			data->gicd_base + GIC_DIST_SOFTINT);
#else
	dev_dbg(dev, "exynos_smc() is called\n");
	exynos_smc(SMC_CMD_REG,
			SMC_REG_ID_SFR_W(0x13EF1000 + GIC_DIST_SOFTINT),
			(0x1 << 16) | (hw_irq & 0xF), 0);
#endif
}
EXPORT_SYMBOL(abox_gic_generate_interrupt);

int abox_gic_register_irq_handler(struct device *dev, unsigned int irq,
		irq_handler_t handler, void *dev_id)
{
	struct abox_gic_data *data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s(%u, %pf)\n", __func__, irq, handler);

	if (irq >= ARRAY_SIZE(data->handler)) {
		dev_err(dev, "invalid irq: %d\n", irq);
		return -EINVAL;
	}

	data->handler[irq].handler = handler;
	data->handler[irq].dev_id = dev_id;

	return 0;
}
EXPORT_SYMBOL(abox_gic_register_irq_handler);

int abox_gic_unregister_irq_handler(struct device *dev, unsigned int irq)
{
	struct abox_gic_data *data = dev_get_drvdata(dev);

	dev_info(dev, "%s(%u)\n", __func__, irq);

	if (irq >= ARRAY_SIZE(data->handler)) {
		dev_err(dev, "invalid irq: %d\n", irq);
		return -EINVAL;
	}

	data->handler[irq].handler = NULL;
	data->handler[irq].dev_id = NULL;

	return 0;
}
EXPORT_SYMBOL(abox_gic_unregister_irq_handler);

static irqreturn_t __abox_gic_irq_handler(struct abox_gic_data *data, u32 irqnr)
{
	struct abox_gic_irq_handler_t *handler;

	if (irqnr >= ARRAY_SIZE(data->handler))
		return IRQ_NONE;

	handler = &data->handler[irqnr];
	if (!handler->handler)
		return IRQ_NONE;

	return handler->handler(irqnr, handler->dev_id);
}

static irqreturn_t abox_gic_irq_handler(int irq, void *dev_id)
{
	struct device *dev = dev_id;
	struct abox_gic_data *data = dev_get_drvdata(dev);
	irqreturn_t ret = IRQ_NONE;
	u32 irqstat, irqnr;

	dev_dbg(dev, "%s\n", __func__);

	do {
		irqstat = readl(data->gicc_base + GIC_CPU_INTACK);
		irqnr = irqstat & GICC_IAR_INT_ID_MASK;
		dev_dbg(dev, "IAR: %08X\n", irqstat);

		if (irqnr < 16) {
			writel(irqstat, data->gicc_base + GIC_CPU_EOI);
			writel(irqstat, data->gicc_base + GIC_CPU_DEACTIVATE);
			ret |= __abox_gic_irq_handler(data, irqnr);
			continue;
		} else if (irqnr > 15 && irqnr < 1021) {
			writel(irqstat, data->gicc_base + GIC_CPU_EOI);
			ret |= __abox_gic_irq_handler(data, irqnr);
			continue;
		}
		break;
	} while (1);

	return ret;
}

static void abox_gicd_enable(struct device *dev, bool en)
{
	struct abox_gic_data *data = dev_get_drvdata(dev);
	void __iomem *gicd_base = data->gicd_base;

	if (en) {
		writel(0x1, gicd_base + GIC_DIST_CTRL);
		writel(0x0, gicd_base + GIC_DIST_IGROUP + 0x0);
		writel(0x0, gicd_base + GIC_DIST_IGROUP + 0x4);
		writel(0x0, gicd_base + GIC_DIST_IGROUP + 0x8);
		writel(0x0, gicd_base + GIC_DIST_IGROUP + 0xC);
	} else {
		writel(0x0, gicd_base + GIC_DIST_CTRL);
	}
}

void abox_gic_init_gic(struct device *dev)
{
	struct abox_gic_data *data = dev_get_drvdata(dev);
	unsigned long arg;
	int i, ret;

	dev_info(dev, "%s\n", __func__);

#ifdef GIC_IS_SECURE_FREE
	writel(0x000000FF, data->gicc_base + GIC_CPU_PRIMASK);
	writel(0x3, data->gicd_base + GIC_DIST_CTRL);
#else
	arg = SMC_REG_ID_SFR_W(data->gicc_base_phys + GIC_CPU_PRIMASK);
	ret = exynos_smc(SMC_CMD_REG, arg, 0x000000FF, 0);

	arg = SMC_REG_ID_SFR_W(data->gicd_base_phys + GIC_DIST_CTRL);
	ret = exynos_smc(SMC_CMD_REG, arg, 0x3, 0);
#endif
	if (is_secure_gic()) {
		for (i = 0; i < 1; i++) {
			arg = SMC_REG_ID_SFR_W(data->gicd_base_phys +
					GIC_DIST_IGROUP + (i * 4));
			ret = exynos_smc(SMC_CMD_REG, arg, 0xFFFFFFFF, 0);
		}
	}
	for (i = 0; i < 40; i++) {
#ifdef GIC_IS_SECURE_FREE
		writel(0x10101010, data->gicd_base + GIC_DIST_PRI + (i * 4));
#else
		arg = SMC_REG_ID_SFR_W(data->gicd_base_phys +
				GIC_DIST_PRI + (i * 4));
		ret = exynos_smc(SMC_CMD_REG, arg, 0x10101010, 0);
#endif
	}

	writel(0x3, data->gicc_base + GIC_CPU_CTRL);
}
EXPORT_SYMBOL(abox_gic_init_gic);

int abox_gic_enable_irq(struct device *dev)
{
	struct abox_gic_data *data = dev_get_drvdata(dev);

	if (likely(data->disabled)) {
		dev_info(dev, "%s\n", __func__);

		data->disabled = false;
		enable_irq(data->irq);
		abox_gicd_enable(dev, true);
	}
	return 0;
}

int abox_gic_disable_irq(struct device *dev)
{
	struct abox_gic_data *data = dev_get_drvdata(dev);

	if (likely(!data->disabled)) {
		dev_info(dev, "%s\n", __func__);

		data->disabled = true;
		disable_irq(data->irq);
	}

	return 0;
}

static int samsung_abox_gic_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct abox_gic_data *data;
	int ret;

	dev_info(dev, "%s\n", __func__);

	data = devm_kzalloc(dev, sizeof(struct abox_gic_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	platform_set_drvdata(pdev, data);

	data->gicd_base = devm_get_request_ioremap(pdev, "gicd",
			&data->gicd_base_phys, NULL);
	if (IS_ERR(data->gicd_base))
		return PTR_ERR(data->gicd_base);

	data->gicc_base = devm_get_request_ioremap(pdev, "gicc",
			&data->gicc_base_phys, NULL);
	if (IS_ERR(data->gicc_base))
		return PTR_ERR(data->gicc_base);

	data->irq = platform_get_irq(pdev, 0);
	if (data->irq < 0) {
		dev_err(dev, "Failed to get irq\n");
		return data->irq;
	}

	ret = devm_request_irq(dev, data->irq, abox_gic_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_GIC_MULTI_TARGET,
			pdev->name, dev);
	if (ret < 0) {
		dev_err(dev, "Failed to request irq\n");
		return ret;
	}

	ret = enable_irq_wake(data->irq);
	if (ret < 0)
		dev_err(dev, "Failed to enable irq wake\n");

#ifndef CONFIG_PM
	abox_gic_resume(dev);
#endif
	dev_info(dev, "%s: probe complete\n", __func__);

	return 0;
}

static int samsung_abox_gic_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "%s\n", __func__);

	return 0;
}

static const struct of_device_id samsung_abox_gic_of_match[] = {
	{
		.compatible = "samsung,abox-gic",
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_gic_of_match);

static struct platform_driver samsung_abox_gic_driver = {
	.probe  = samsung_abox_gic_probe,
	.remove = samsung_abox_gic_remove,
	.driver = {
		.name = "samsung-abox-gic",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_gic_of_match),
	},
};

module_platform_driver(samsung_abox_gic_driver);

/* Module information */
MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box GIC Driver");
MODULE_ALIAS("platform:samsung-abox-gic");
MODULE_LICENSE("GPL");
