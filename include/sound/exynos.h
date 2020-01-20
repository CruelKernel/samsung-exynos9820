/*
 * Samsung Audio Subsystem Interface
 *
 * Copyright (c) 2013 Samsung Electronics Co. Ltd.
 *	Yeongman Seo <yman.seo@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SOUND_EXYNOS_H
#define __SOUND_EXYNOS_H

/* Sub IPs of LPASS */
enum {
	LPASS_IP_DMA = 0,
	LPASS_IP_MEM,
	LPASS_IP_TIMER,
	LPASS_IP_I2S,
	LPASS_IP_PCM,
	LPASS_IP_UART,
	LPASS_IP_SLIMBUS,
	LPASS_IP_CA5
};

enum {
	LPASS_OP_RESET = 0,
	LPASS_OP_NORMAL,
};

/* Availability of power mode */
enum {
	AUD_PWR_SLEEP = 0,
	AUD_PWR_LPC,
	AUD_PWR_LPA,
	AUD_PWR_ALPA,
	AUD_PWR_AFTR,
};

#ifdef CONFIG_SCHED_HMP
#define USE_EXYNOS_AUD_SCHED
enum {
	AUD_MODE_DEFAULT = 0,
	AUD_MODE_NORM,
	AUD_MODE_UHQA,
};

extern void lpass_set_sched(pid_t pid, int mode);
#endif

#if defined(CONFIG_SCHED_HMP) && defined(CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG)
#define USE_EXYNOS_AUD_CPU_HOTPLUG
extern void lpass_get_cpu_hotplug(void);
extern void lpass_put_cpu_hotplug(void);
#endif

#ifdef CONFIG_SND_SAMSUNG_AUDSS
extern int exynos_check_aud_pwr(void);
#else
static inline int exynos_check_aud_pwr(void) { return -1; }
#endif


extern int lpass_register_subip(struct device *ip_dev, const char *ip_name);
extern int lpass_set_gpio_cb(struct device *ip_dev,
			void (*ip_cb)(struct device *dev));
extern void lpass_get_sync(struct device *ip_dev);
extern void lpass_put_sync(struct device *ip_dev);
extern struct iommu_domain *lpass_get_iommu_domain(void);

extern void lpass_add_stream(void); /*flag: true means compress stream else pcm */
extern void lpass_remove_stream(void);
extern void lpass_dma_enable(bool on);

extern void lpass_reset(int ip, int op);
extern void lpass_reset_toggle(int ip);

#endif /* __SOUND_EXYNOS_H */
