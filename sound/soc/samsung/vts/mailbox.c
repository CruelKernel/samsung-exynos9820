/* sound/soc/samsung/vts/mailbox.c
 *
 * ALSA SoC - Samsung Mailbox driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
  *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
//#define DEBUG
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <sound/samsung/vts.h>

#include "mailbox.h"

#define DEVICE_NAME "samsung-mailbox-asoc"

struct mailbox_data {
	void __iomem *sfr_base;
	struct irq_domain *irq_domain;
	int irq;
};

int mailbox_generate_interrupt(const struct platform_device *pdev, int hw_irq)
{
	struct mailbox_data *data = platform_get_drvdata(pdev);

	writel(BIT(hw_irq), data->sfr_base + MAILBOX_INTGR0);
	dev_dbg(&pdev->dev, "%s: writel(%lx, %lx)", __func__,
			BIT(hw_irq), (unsigned long)virt_to_phys(data->sfr_base + MAILBOX_INTGR0));

	return 0;
}
EXPORT_SYMBOL(mailbox_generate_interrupt);

void mailbox_write_shared_register(const struct platform_device *pdev,
		const u32 *values, int start, int count)
{
	struct mailbox_data *data = platform_get_drvdata(pdev);
	u32 __iomem *sfr = data->sfr_base + MAILBOX_ISSR0 + (start * sizeof(u32));

	dev_dbg(&pdev->dev, "%s(%p, %p, %d, %d)", __func__, pdev, values, start, count);
	while (count--) {
		writel_relaxed(*values++, sfr++);
	}
}
EXPORT_SYMBOL(mailbox_write_shared_register);

void mailbox_read_shared_register(const struct platform_device *pdev,
		u32 *values, int start, int count)
{
	struct mailbox_data *data = platform_get_drvdata(pdev);
	u32 __iomem *sfr = data->sfr_base + MAILBOX_ISSR0 + (start * sizeof(u32));

	dev_dbg(&pdev->dev, "%s(%p, %p, %d, %d)", __func__, pdev, values, start, count);
	while (count--) {
		*values++ = readl_relaxed(sfr++);
		dev_dbg(&pdev->dev, "value=%d\n", *(values - 1));
	}
}
EXPORT_SYMBOL(mailbox_read_shared_register);

static void mailbox_handle_irq(struct irq_desc *desc)
{
	struct platform_device *pdev = irq_desc_get_handler_data(desc);
	struct mailbox_data *data = platform_get_drvdata(pdev);
	struct irq_domain *irq_domain = data->irq_domain;
	struct irq_chip *chip = irq_desc_get_chip(desc);

	struct irq_chip_generic *gc = irq_get_domain_generic_chip(irq_domain, 0);
	u32 stat = readl_relaxed(gc->reg_base + MAILBOX_INTSR1);

	dev_dbg(&pdev->dev, "%s: stat=%08x\n", __func__, stat);

	chained_irq_enter(chip, desc);

	while (stat) {
		u32 hwirq = __fls(stat);
		generic_handle_irq(irq_find_mapping(irq_domain, gc->irq_base + hwirq));
		stat &= ~(1 << hwirq);
	}

	chained_irq_exit(chip, desc);
}

static int mailbox_suspend(struct device *dev)
{
	return 0;
}

static int mailbox_resume(struct device *dev)
{
	return 0;
}

static const struct of_device_id samsung_mailbox_of_match[] = {
	{
		.compatible = "samsung,mailbox-asoc",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_mailbox_of_match);

SIMPLE_DEV_PM_OPS(samsung_mailbox_pm, mailbox_suspend, mailbox_resume);

static void mailbox_irq_enable(struct irq_data *data)
{
	if (vts_is_on()) {
		irq_gc_mask_clr_bit(data);
	} else {
		pr_debug("%s(%d): vts is already off\n",
				__func__, data->irq);
	}
	return;
}

static void mailbox_irq_disable(struct irq_data *data)
{
	if (vts_is_on()) {
		irq_gc_mask_set_bit(data);
	} else {
		pr_debug("%s(%d): vts is already off\n",
				__func__, data->irq);
	}
	return;
}

static void __iomem *devm_request_and_ioremap(struct platform_device *pdev, const char *name)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	void __iomem *result;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "Failed to get %s\n", name);
		return ERR_PTR(-EINVAL);
	}

	res = devm_request_mem_region(dev, res->start, resource_size(res), name);
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "Failed to request %s\n", name);
		return ERR_PTR(-EFAULT);
	}

	result = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR_OR_NULL(result)) {
		dev_err(dev, "Failed to map %s\n", name);
		return ERR_PTR(-EFAULT);
	}

	return result;
}

static int samsung_mailbox_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct mailbox_data *data;
	struct irq_chip_generic *gc;
	int result;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(data)) {
		dev_err(dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, data);

	data->sfr_base = devm_request_and_ioremap(pdev, "sfr");
	if (IS_ERR(data->sfr_base)) {
		return PTR_ERR(data->sfr_base);
	}

	result = platform_get_irq(pdev, 0);
	if (result < 0) {
		dev_err(dev, "Failed to get irq resource\n");
		return result;
	}
	data->irq = result;

	data->irq_domain = irq_domain_add_linear(np, MAILBOX_INT1_OFFSET + MAILBOX_INT1_SIZE,
			&irq_generic_chip_ops, NULL);
	if (IS_ERR_OR_NULL(data->irq_domain)) {
		dev_err(dev, "Failed to add irq domain\n");
		return -ENOMEM;
	}

	result = irq_alloc_domain_generic_chips(data->irq_domain, MAILBOX_INT1_OFFSET + MAILBOX_INT1_SIZE,
			1, "mailbox_irq_chip", handle_level_irq, 0, 0, IRQ_GC_INIT_MASK_CACHE);
	if (result < 0) {
		dev_err(dev, "Failed to allocation generic irq chips\n");
		return result;
	}

	gc = irq_get_domain_generic_chip(data->irq_domain, 0);
	gc->reg_base				= data->sfr_base;
	gc->chip_types[0].chip.irq_enable	= mailbox_irq_enable;
	gc->chip_types[0].chip.irq_disable	= mailbox_irq_disable;
	gc->chip_types[0].chip.irq_ack		= irq_gc_ack_set_bit;
	gc->chip_types[0].chip.irq_mask		= irq_gc_mask_set_bit;
	gc->chip_types[0].chip.irq_unmask	= irq_gc_mask_clr_bit;
	gc->chip_types[0].regs.mask		= MAILBOX_INTMR1;
	gc->chip_types[0].regs.ack		= MAILBOX_INTCR1;
	gc->wake_enabled			= 0x0000FFFF;
	gc->chip_types[0].chip.irq_set_wake	= irq_gc_set_wake;

	irq_set_handler_data(data->irq, pdev);
	irq_set_chained_handler(data->irq, mailbox_handle_irq);

	dev_info(dev, "Probed successfully\n");

	return 0;
}

static int samsung_mailbox_remove(struct platform_device *pdev)
{
	struct mailbox_data *mailbox_data = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "%s\n", __func__);

	irq_remove_generic_chip(irq_get_domain_generic_chip(mailbox_data->irq_domain, 0),
			IRQ_MSK(MAILBOX_INT1_SIZE), 0, 0);
	irq_domain_remove(mailbox_data->irq_domain);

	return 0;
}

static struct platform_driver samsung_mailbox_driver = {
	.probe  = samsung_mailbox_probe,
	.remove = samsung_mailbox_remove,
	.driver = {
		.name = DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_mailbox_of_match),
		.pm = &samsung_mailbox_pm,
	},
};

module_platform_driver(samsung_mailbox_driver);

/* Module information */
MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung Mailbox");
MODULE_ALIAS("platform:samsung-mailbox");
MODULE_LICENSE("GPL");
