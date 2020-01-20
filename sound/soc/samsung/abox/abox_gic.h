/* sound/soc/samsung/abox/abox_gic.h
 *
 * ALSA SoC Audio Layer - Samsung Abox GIC driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_GIC_H
#define __SND_SOC_ABOX_GIC_H

#include <linux/interrupt.h>

#define ABOX_GIC_SGI_COUNT 16
#define ABOX_GIC_SPI_COUNT 103
#define ABOX_GIC_IRQ_COUNT (ABOX_GIC_SGI_COUNT + ABOX_GIC_SPI_COUNT)

struct abox_gic_irq_handler_t {
	irq_handler_t handler;
	void *dev_id;
};

struct abox_gic_data {
	void __iomem *gicd_base;
	void __iomem *gicc_base;
	phys_addr_t gicd_base_phys;
	phys_addr_t gicc_base_phys;
	int irq;
	struct abox_gic_irq_handler_t handler[ABOX_GIC_IRQ_COUNT];
	bool disabled;
};

enum abox_gic_target {
	ABOX_GIC_CORE0,
	ABOX_GIC_CP,
	ABOX_GIC_AP,
	ABOX_GIC_CORE1,
	ABOX_GIC_CORE2,
	ABOX_GIC_CORE3,
};

/**
 * Enable or disable an IRQ
 * @param[in]	dev	pointer to abox_gic device
 * @param[in]	irq	irq id
 * @param[in]	en	enable
 */
extern void abox_gic_enable(struct device *dev, unsigned int irq, bool en);

/**
 * Change target of an IRQ
 * @param[in]	dev	pointer to abox_gic device
 * @param[in]	irq	irq id
 * @param[in]	target	target
 */
extern void abox_gic_target(struct device *dev, unsigned int irq,
		enum abox_gic_target target);

/**
 * Change target of an IRQ to ap
 * @param[in]	dev	pointer to abox_gic device
 * @param[in]	irq	irq id
 */
static inline void abox_gic_target_ap(struct device *dev, unsigned int irq)
{
	abox_gic_target(dev, irq, ABOX_GIC_AP);
}

/**
 * Change target of an IRQ to core0
 * @param[in]	dev	pointer to abox_gic device
 * @param[in]	irq	irq id
 */
static inline void abox_gic_target_core0(struct device *dev, unsigned int irq)
{
	abox_gic_target(dev, irq, ABOX_GIC_CORE0);
}

/**
 * Generate interrupt
 * @param[in]	dev	pointer to abox_gic device
 * @param[in]	irq	irq number
 */
extern void abox_gic_generate_interrupt(struct device *dev, unsigned int irq);

/**
 * Register interrupt handler
 * @param[in]	dev	pointer to abox_gic device
 * @param[in]	irq	irq number
 * @param[in]	handler	function to be called on interrupt
 * @param[in]	dev_id	cookie for interrupt.
 * @return	error code or 0
 */
extern int abox_gic_register_irq_handler(struct device *dev,
		unsigned int irq, irq_handler_t handler, void *dev_id);

/**
 * Unregister interrupt handler
 * @param[in]	dev	pointer to abox_gic device
 * @param[in]	irq	irq number
 * @return	error code or 0
 */
extern int abox_gic_unregister_irq_handler(struct device *dev,
		unsigned int irq);

/**
 * Enable abox gic irq
 * @param[in]	dev	pointer to abox_gic device
 */
extern int abox_gic_enable_irq(struct device *dev);

/**
 * Disable abox gic irq
 * @param[in]	dev	pointer to abox_gic device
 */
extern int abox_gic_disable_irq(struct device *dev);

/**
 * Initialize abox gic
 * @param[in]	dev	pointer to abox_gic device
 */
extern void abox_gic_init_gic(struct device *dev);

#endif /* __SND_SOC_ABOX_GIC_H */
