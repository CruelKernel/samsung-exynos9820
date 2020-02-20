/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * Exynos - Support SoC specific handler
 * Author: Hosung Kim <hosung0.kim@samsung.com>
 *         Youngmin Nam <youngmin.nam@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/soc/samsung/exynos-soc.h>
#include <linux/interrupt.h>
#include <linux/debug-snapshot-helper.h>

extern struct dbg_snapshot_helper_ops *dss_soc_ops;

struct exynos_handler {
	int		irq;
	char		name[SZ_16];
	irqreturn_t	(*handle_irq)(int irq, void *data);
};

#define MAX_ERRIRQ 15
static struct exynos_handler ecc_handler[MAX_ERRIRQ];

static irqreturn_t exynos_ecc_handler(int irq, void *data)
{
	struct exynos_handler *ecc = (struct exynos_handler *)data;

	dss_soc_ops->soc_dump_info(NULL);

	panic("Detected ECC error: irq: %d, name: %s", ecc->irq, ecc->name);
	return 0;
}

static int __init exynos_handler_setup(struct device_node *np)
{
	int err = 0, i;
	int nr_irq;

	if (of_property_read_u32(np, "handler_nr_irq", &nr_irq)) {
		nr_irq = MAX_ERRIRQ;
		pr_err("%s: handler_nr_irq property is not defined in device tree\n", __func__);
	}

	pr_info("%s: nr_irq = %d\n", __func__, nr_irq);
	/* setup ecc_handler */
	for (i = 0; i < nr_irq; i++) {
		ecc_handler[i].irq = irq_of_parse_and_map(np, i);
		snprintf(ecc_handler[i].name, sizeof(ecc_handler[i].name),
				"ecc_handler%d", i);
		ecc_handler[i].handle_irq = exynos_ecc_handler;

		err = request_irq(ecc_handler[i].irq,
				ecc_handler[i].handle_irq,
				IRQF_TRIGGER_HIGH | IRQF_NOBALANCING | IRQF_GIC_MULTI_TARGET,
				ecc_handler[i].name, &ecc_handler[i]);
		if (err) {
			pr_err("unable to request irq%d for %s ecc handler\n",
					ecc_handler[i].irq, ecc_handler[i].name);
			break;
		} else {
			pr_info("Success to request irq%d for %s ecc handler\n",
					ecc_handler[i].irq, ecc_handler[i].name);
		}
	}

	of_node_put(np);
	return err;
}

static const struct of_device_id handler_of_match[] __initconst = {
	{ .compatible = "samsung,exynos-handler", .data = exynos_handler_setup},
	{},
};

typedef int (*handler_initcall_t)(const struct device_node *);
static int __init exynos_handler_init(void)
{
	struct device_node *np;
	const struct of_device_id *matched_np;
	handler_initcall_t init_fn;

	np = of_find_matching_node_and_match(NULL, handler_of_match, &matched_np);
	if (!np)
		return -ENODEV;

	init_fn = (handler_initcall_t)matched_np->data;

	return init_fn(np);
}
subsys_initcall(exynos_handler_init);
