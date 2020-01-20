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

#ifndef MCU_IPC_H
#define MCU_IPC_H

/* FIXME: will be removed */
/* Shared register with 64 * 32 words */
#define MAX_MBOX_NUM	64

enum mcu_ipc_region {
	MCU_CP,
	MCU_GNSS,
	MCU_MAX,
};

struct mcu_ipc_ipc_handler {
	void *data;
	void (*handler)(void *);
};

struct mcu_ipc_drv_data {
	char *name;
	u32 id;

	void __iomem *ioaddr;
	u32 registered_irq;
	unsigned long unmasked_irq;

	/**
	 * irq affinity cpu mask
	 */
	cpumask_var_t dmask;	/* default cpu mask */
	cpumask_var_t imask;	/* irq affinity cpu mask */

	struct device *mcu_ipc_dev;
	struct mcu_ipc_ipc_handler hd[16];
	spinlock_t lock;
	spinlock_t reg_lock;

};

static struct mcu_ipc_drv_data mcu_dat[MCU_MAX];

static inline void mcu_ipc_writel(enum mcu_ipc_region id, u32 val, long reg)
{
	writel(val, mcu_dat[id].ioaddr + reg);
}

static inline u32 mcu_ipc_readl(enum mcu_ipc_region id, long reg)
{
	return readl(mcu_dat[id].ioaddr + reg);
}

#ifdef CONFIG_ARGOS
/* kernel team needs to provide argos header file. !!!
 * As of now, there's nothing to use. */
#ifdef CONFIG_SCHED_HMP
extern struct cpumask hmp_slow_cpu_mask;
extern struct cpumask hmp_fast_cpu_mask;

static inline struct cpumask *get_default_cpu_mask(void)
{
	return &hmp_slow_cpu_mask;
}
#else
static inline struct cpumask *get_default_cpu_mask(void)
{
	return cpu_all_mask;
}
#endif

struct mcu_argos_info {
	int irq;
	u32 affinity;
};

int argos_irq_affinity_setup_label(unsigned int irq, const char *label,
		struct cpumask *affinity_cpu_mask,
		struct cpumask *default_cpu_mask);
int argos_task_affinity_setup_label(struct task_struct *p, const char *label,
		struct cpumask *affinity_cpu_mask,
		struct cpumask *default_cpu_mask);
#endif

#define mcu_dt_read_enum(np, prop, dest) \
	do { \
		u32 val; \
		if (of_property_read_u32(np, prop, &val)) \
			return -EINVAL; \
		dest = (__typeof__(dest))(val); \
	} while (0)

#define mcu_dt_read_bool(np, prop, dest) \
	do { \
		u32 val; \
		if (of_property_read_u32(np, prop, &val)) \
			return -EINVAL; \
		dest = val ? true : false; \
	} while (0)

#define mcu_dt_read_string(np, prop, dest) \
	do { \
		if (of_property_read_string(np, prop, \
				(const char **)&dest)) \
			return -EINVAL; \
	} while (0)

#define mcu_dt_read_u32(np, prop, dest) \
	do { \
		u32 val; \
		if (of_property_read_u32(np, prop, &val)) \
			return -EINVAL; \
		dest = val; \
	} while (0)
#endif
