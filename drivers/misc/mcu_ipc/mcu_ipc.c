/*
 * Copyright (C) 2014 Samsung Electronics Co.Ltd
 * http://www.samsung.com
 *
 * MCU IPC driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
*/

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/bitops.h>

#include "regs-mcu_ipc.h"
#include "mcu_ipc.h"

#define	USE_FIXED_AFFINITY

void mcu_ipc_reg_dump(enum mcu_ipc_region id)
{
	unsigned long flags;
	u32 i, value;

	if (id >= MCU_MAX)
		return;

	spin_lock_irqsave(&mcu_dat[id].reg_lock, flags);

	for (i = 0; i < 4; i++) {
		value = mcu_ipc_readl(id, EXYNOS_MCU_IPC_ISSR0 + (4 * i));
		dev_err(mcu_dat[id].mcu_ipc_dev, "mbox dump: 0x%02x: 0x%04x\n",
			i, value);
	}

	spin_unlock_irqrestore(&mcu_dat[id].reg_lock, flags);
}
EXPORT_SYMBOL(mcu_ipc_reg_dump);

static irqreturn_t mcu_ipc_handler(int irq, void *data)
{
	u32 irq_stat, i;
	u32 id;

	id = ((struct mcu_ipc_drv_data *)data)->id;

	spin_lock(&mcu_dat[id].reg_lock);

	/* Check raised interrupts */
	irq_stat = mcu_ipc_readl(id, EXYNOS_MCU_IPC_INTSR0) & 0xFFFF0000;


	/* Only clear and handle unmasked interrupts */
	irq_stat &= mcu_dat[id].unmasked_irq << 16;


	/* Interrupt Clear */
	mcu_ipc_writel(id, irq_stat, EXYNOS_MCU_IPC_INTCR0);
	spin_unlock(&mcu_dat[id].reg_lock);

	for (i = 0; i < 16; i++) {
		if (irq_stat & (1 << (i + 16))) {
			if ((1 << (i + 16)) & mcu_dat[id].registered_irq)
				mcu_dat[id].hd[i].handler(mcu_dat[id].hd[i].data);
			else
				dev_err(mcu_dat[id].mcu_ipc_dev,
					"Unregistered INT received.\n");

			irq_stat &= ~(1 << (i + 16));
		}

		if (!irq_stat)
			break;
	}

	return IRQ_HANDLED;
}

int mbox_request_irq(enum mcu_ipc_region id, u32 int_num,
					void (*handler)(void *), void *data)
{
	unsigned long flags;

	if ((!handler) || (int_num > 15))
		return -EINVAL;

	spin_lock_irqsave(&mcu_dat[id].reg_lock, flags);

	mcu_dat[id].hd[int_num].data = data;
	mcu_dat[id].hd[int_num].handler = handler;
	mcu_dat[id].registered_irq |= 1 << (int_num + 16);
	set_bit(int_num, &mcu_dat[id].unmasked_irq);

	spin_unlock_irqrestore(&mcu_dat[id].reg_lock, flags);

	return 0;
}
EXPORT_SYMBOL(mbox_request_irq);

/*
 * mbox_enable_irq
 *
 * This function unmasks a single mailbox interrupt.
 */
int mbox_enable_irq(enum mcu_ipc_region id, u32 int_num)
{
	unsigned long flags;
	unsigned long tmp;

	/* The irq should have been registered. */
	if (!(mcu_dat[id].registered_irq & BIT(int_num + 16)))
		return -EINVAL;

	spin_lock_irqsave(&mcu_dat[id].reg_lock, flags);

	tmp = mcu_ipc_readl(id, EXYNOS_MCU_IPC_INTMR0);

	/* Clear the mask if it was set. */
	if (test_and_clear_bit(int_num + 16, &tmp))
		mcu_ipc_writel(id, tmp, EXYNOS_MCU_IPC_INTMR0);

	/* Mark the irq as unmasked */
	set_bit(int_num, &mcu_dat[id].unmasked_irq);

	spin_unlock_irqrestore(&mcu_dat[id].reg_lock, flags);

	return 0;
}
EXPORT_SYMBOL(mbox_enable_irq);

/*
 * mbox_check_irq
 *
 * This function is used to check the state of the mailbox interrupt
 * when the interrupt after the interrupt has been masked. This can be
 * used to check if a new interrupt has been set after being masked. A
 * masked interrupt will have its status set but will not generate a hard
 * interrupt. This function will check and clear the status.
 */
int mbox_check_irq(enum mcu_ipc_region id, u32 int_num)
{
	unsigned long flags;
	u32 irq_stat;

	/* Interrupt must have been registered. */
	if (!(mcu_dat[id].registered_irq & BIT(int_num + 16)))
		return -EINVAL;

	spin_lock_irqsave(&mcu_dat[id].reg_lock, flags);

	/* Interrupt must have been masked. */
	if (test_bit(int_num, &mcu_dat[id].unmasked_irq)) {
		spin_unlock_irqrestore(&mcu_dat[id].reg_lock, flags);
		return -EINVAL;
	}

	/* Check and clear the interrupt status bit. */
	irq_stat = mcu_ipc_readl(id, EXYNOS_MCU_IPC_INTSR0) & BIT(int_num + 16);
	if (irq_stat)
		mcu_ipc_writel(id, irq_stat, EXYNOS_MCU_IPC_INTCR0);

	spin_unlock_irqrestore(&mcu_dat[id].reg_lock, flags);

	return irq_stat != 0;
}
EXPORT_SYMBOL(mbox_check_irq);

/*
 * mbox_disable_irq
 *
 * This function masks and single mailbox interrupt.
 */
int mbox_disable_irq(enum mcu_ipc_region id, u32 int_num)
{
	unsigned long flags;
	unsigned long irq_mask;

	/* The interrupt must have been registered. */
	if (!(mcu_dat[id].registered_irq & BIT(int_num + 16)))
		return -EINVAL;

	/* Set the mask */
	spin_lock_irqsave(&mcu_dat[id].reg_lock, flags);

	irq_mask = mcu_ipc_readl(id, EXYNOS_MCU_IPC_INTMR0);

	/* Set the mask if it was not already set */
	if (!test_and_set_bit(int_num + 16, &irq_mask)) {
		mcu_ipc_writel(id, irq_mask, EXYNOS_MCU_IPC_INTMR0);

		udelay(5);

		/* Reset the status bit to signal interrupt needs handling */
		mcu_ipc_writel(id, BIT(int_num + 16), EXYNOS_MCU_IPC_INTGR0);

		udelay(5);
	}

	/* Remove the irq from the umasked irqs */
	clear_bit(int_num, &mcu_dat[id].unmasked_irq);

	spin_unlock_irqrestore(&mcu_dat[id].reg_lock, flags);

	return 0;
}
EXPORT_SYMBOL(mbox_disable_irq);

int mcu_ipc_unregister_handler(enum mcu_ipc_region id, u32 int_num,
								void (*handler)(void *))
{
	unsigned long flags;

	if (!handler || (mcu_dat[id].hd[int_num].handler != handler))
		return -EINVAL;

	spin_lock_irqsave(&mcu_dat[id].reg_lock, flags);

	mcu_dat[id].hd[int_num].data = NULL;
	mcu_dat[id].hd[int_num].handler = NULL;
	mcu_dat[id].registered_irq &= ~(1 << (int_num + 16));
	clear_bit(int_num, &mcu_dat[id].unmasked_irq);

	spin_unlock_irqrestore(&mcu_dat[id].reg_lock, flags);

	return 0;
}
EXPORT_SYMBOL(mcu_ipc_unregister_handler);

void mbox_set_interrupt(enum mcu_ipc_region id, u32 int_num)
{
	/* generate interrupt */
	if (int_num < 16)
		mcu_ipc_writel(id, 0x1 << int_num, EXYNOS_MCU_IPC_INTGR1);
}
EXPORT_SYMBOL(mbox_set_interrupt);

void mcu_ipc_send_command(enum mcu_ipc_region id, u32 int_num, u16 cmd)
{
	/* write command */
	if (int_num < 16)
		mcu_ipc_writel(id, cmd, EXYNOS_MCU_IPC_ISSR0 + (8 * int_num));

	/* generate interrupt */
	mbox_set_interrupt(id, int_num);
}
EXPORT_SYMBOL(mcu_ipc_send_command);

u32 mbox_get_value(enum mcu_ipc_region id, u32 mbx_num)
{
	if (mbx_num < 64)
		return mcu_ipc_readl(id, EXYNOS_MCU_IPC_ISSR0 + (4 * mbx_num));
	else
		return 0;
}
EXPORT_SYMBOL(mbox_get_value);

void mbox_set_value(enum mcu_ipc_region id, u32 mbx_num, u32 msg)
{
	if (mbx_num < 64)
		mcu_ipc_writel(id, msg, EXYNOS_MCU_IPC_ISSR0 + (4 * mbx_num));
}
EXPORT_SYMBOL(mbox_set_value);

u32 mbox_extract_value(enum mcu_ipc_region id, u32 mbx_num, u32 mask, u32 pos)
{
	if (mbx_num < 64)
		return (mbox_get_value(id, mbx_num) >> pos) & mask;
	else
		return 0;
}
EXPORT_SYMBOL(mbox_extract_value);

void mbox_update_value(enum mcu_ipc_region id, u32 mbx_num,
				u32 msg, u32 mask, u32 pos)
{
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(&mcu_dat[id].lock, flags);

	if (mbx_num < 64) {
		val = mbox_get_value(id, mbx_num);
		val &= ~(mask << pos);
		val |= (msg & mask) << pos;
		mbox_set_value(id, mbx_num, val);
	}

	spin_unlock_irqrestore(&mcu_dat[id].lock, flags);
}
EXPORT_SYMBOL(mbox_update_value);

void mbox_sw_reset(enum mcu_ipc_region id)
{
	u32 reg_val;

	printk("Reset Mailbox registers\n");

	reg_val = mcu_ipc_readl(id, EXYNOS_MCU_IPC_MCUCTLR);
	reg_val |= (0x1 << MCU_IPC_MCUCTLR_MSWRST);

	mcu_ipc_writel(id, reg_val, EXYNOS_MCU_IPC_MCUCTLR) ;

	udelay(5);
}
EXPORT_SYMBOL(mbox_sw_reset);

static void mcu_ipc_clear_all_interrupt(enum mcu_ipc_region id)
{
	mcu_ipc_writel(id, 0xFFFF, EXYNOS_MCU_IPC_INTCR1);
}

#ifdef CONFIG_ARGOS
static int set_irq_affinity(struct device *dev)
{
	struct mcu_argos_info *argos_info = dev->driver_data;

	if (argos_info != NULL) {
		dev_dbg(dev, "set default irq affinity (0x%x)\n", argos_info->affinity);
		return irq_set_affinity(argos_info->irq, cpumask_of(argos_info->affinity));
	} else {
		/* skip other mcu_ipc device that has no irq affinity : gnss, ... */
		return 0;
	}
}

#ifdef USE_FIXED_AFFINITY
static int set_fixed_affinity(struct device *dev, int irq, u32 mask)
{
	struct mcu_argos_info *argos_info;

	argos_info = devm_kzalloc(dev, sizeof(struct mcu_argos_info), GFP_KERNEL);
	if (!argos_info) {
		dev_err(dev, "Failed to alloc argos info sturct\n");
		return -ENOMEM;
	}
	argos_info->irq = irq;
	argos_info->affinity = mask;
	dev_set_drvdata(dev, argos_info);

	return set_irq_affinity(dev);
}
#else
static int set_runtime_affinity(enum mcu_ipc_region id, int irq, u32 mask)
{
	if (!zalloc_cpumask_var(&mcu_dat[id].dmask, GFP_KERNEL))
		return -ENOMEM;
	if (!zalloc_cpumask_var(&mcu_dat[id].imask, GFP_KERNEL))
		return -ENOMEM;

	cpumask_or(mcu_dat[id].imask, mcu_dat[id].imask, cpumask_of(mask));
	cpumask_copy(mcu_dat[id].dmask, get_default_cpu_mask());

	return argos_irq_affinity_setup_label(irq, "IPC", mcu_dat[id].imask,
			mcu_dat[id].dmask);
}
#endif

static int mcu_ipc_set_affinity(enum mcu_ipc_region id, struct device *dev, int irq)
{
	struct device_node *np = dev->of_node;
	u32 irq_affinity_mask = 0;

	if (!np)	{
		dev_err(dev, "non-DT project, can't set irq affinity\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "mcu,irq_affinity_mask",
			&irq_affinity_mask))	{
		dev_err(dev, "Failed to get affinity mask\n");
		return -ENODEV;
	}

	dev_info(dev, "irq_affinity_mask = 0x%x\n", irq_affinity_mask);

#ifdef USE_FIXED_AFFINITY
	return set_fixed_affinity(dev, irq, irq_affinity_mask);
#else
	return set_runtime_affinity(id, irq, irq_affinity_mask);
#endif
}
#else
static int mcu_ipc_set_affinity(enum mcu_ipc_region id, struct device *dev, int irq)
{
	return 0;
}
#endif

#ifdef CONFIG_MCU_IPC_TEST
static void test_without_dev(enum mcu_ipc_region id)
{
	int i;
	char *region;

	switch(id) {
	case MCU_CP:
		region = "CP";
		break;
	case MCU_GNSS:
		region = "GNSS";
		break;
	default:
		region = NULL;
	}

	for (i = 0; i < 64; i++) {
		mbox_set_value(id, i, 64 - i);
		mdelay(50);
		dev_err(mcu_dat[id].mcu_ipc_dev,
				"Test without %s(%d): Read mbox value[%d]: %d\n",
				region, id, i, mbox_get_value(i));
	}
}
#endif

static int mcu_ipc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res = NULL;
	int mcu_ipc_irq;
	int err = 0;
	u32 id = 0;

	dev_err(&pdev->dev, "%s: mcu_ipc probe start.\n", __func__);

	err = of_property_read_u32(dev->of_node, "mcu,id", &id);
	if (err) {
		dev_err(&pdev->dev, "MCU IPC parse error! [id]\n");
		return err;
	}

	if (id >= MCU_MAX) {
		dev_err(&pdev->dev, "MCU IPC Invalid ID [%d]\n", id);
		return -EINVAL;
	}

	mcu_dat[id].id = id;
	mcu_dat[id].mcu_ipc_dev = &pdev->dev;

	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
	if (!pdev->dev.coherent_dma_mask)
		pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	if (dev->of_node) {
		mcu_dt_read_string(dev->of_node, "mcu,name", mcu_dat[id].name);
		if (IS_ERR(&mcu_dat[id])) {
			dev_err(&pdev->dev, "MCU IPC parse error!\n");
			return PTR_ERR(&mcu_dat[id]);
		}
	}

	/* resource for mcu_ipc SFR region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mcu_dat[id].ioaddr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mcu_dat[id].ioaddr)) {
		dev_err(&pdev->dev, "failded to request memory resource\n");
		return PTR_ERR(mcu_dat[id].ioaddr);
	}

	/* Request IRQ */
	mcu_ipc_irq = platform_get_irq(pdev, 0);
	err = devm_request_irq(&pdev->dev, mcu_ipc_irq, mcu_ipc_handler, 0,
			pdev->name, &mcu_dat[id]);
	if (err) {
		dev_err(&pdev->dev, "Can't request MCU_IPC IRQ\n");
		return err;
	}

	mcu_ipc_clear_all_interrupt(id);

	/* set argos irq affinity */
	err = mcu_ipc_set_affinity(id, dev, mcu_ipc_irq);
	if (err)
		dev_err(dev, "Can't set IRQ affinity with(%d)\n", err);

#ifdef CONFIG_MCU_IPC_TEST
	test_without_dev(id);
#endif

	spin_lock_init(&mcu_dat[id].lock);
	spin_lock_init(&mcu_dat[id].reg_lock);

	dev_err(&pdev->dev, "%s: mcu_ipc probe done.\n", __func__);

	return 0;
}

static int __exit mcu_ipc_remove(struct platform_device *pdev)
{
	/* TODO */
	return 0;
}

#ifdef CONFIG_PM
static int mcu_ipc_suspend(struct device *dev)
{
	/* TODO */
	return 0;
}

static int mcu_ipc_resume(struct device *dev)
{
	/* TODO */
#ifdef CONFIG_ARGOS
	set_irq_affinity(dev);
#endif
	return 0;
}
#else
#define mcu_ipc_suspend NULL
#define mcu_ipc_resume NULL
#endif

static const struct dev_pm_ops mcu_ipc_pm_ops = {
	.suspend = mcu_ipc_suspend,
	.resume = mcu_ipc_resume,
};

static const struct of_device_id exynos_mcu_ipc_dt_match[] = {
		{ .compatible = "samsung,exynos7580-mailbox", },
		{ .compatible = "samsung,exynos7890-mailbox", },
		{ .compatible = "samsung,exynos8890-mailbox", },
		{ .compatible = "samsung,exynos7870-mailbox", },
		{ .compatible = "samsung,exynos-shd-ipc-mailbox", },
		{},
};
MODULE_DEVICE_TABLE(of, exynos_mcu_ipc_dt_match);

static struct platform_driver mcu_ipc_driver = {
	.probe		= mcu_ipc_probe,
	.remove		= mcu_ipc_remove,
	.driver		= {
		.name = "mcu_ipc",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(exynos_mcu_ipc_dt_match),
		.pm = &mcu_ipc_pm_ops,
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(mcu_ipc_driver);

MODULE_DESCRIPTION("MCU IPC driver");
MODULE_AUTHOR("<hy50.seo@samsung.com>");
MODULE_LICENSE("GPL");
