/*
 * exyswd-rng.c - Random Number Generator driver for the exynos
 *
 * Copyright (C) 2018 Samsung Electronics
 * Sehee Kim <sehi.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/hw_random.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/smc.h>

#define HWRNG_RET_OK			0
#define HWRNG_RET_INVALID_ERROR		1
#define HWRNG_RET_RETRY_ERROR		2
#define HWRNG_RET_INVALID_FLAG_ERROR	3
#define HWRNG_RET_TEST_ERROR		4
#define HWRNG_RET_START_UP_TEST_DONE	5
#define HWRNG_RET_TEST_KAT_ERROR	0xC

#define EXYRNG_MAX_FAILURES		25
#define EXYRNG_START_UP_SIZE		(4096 + 1)
#define EXYRNG_RETRY_MAX_COUNT		1000000
#define EXYRNG_START_UP_TEST_MAX_RETRY	2

uint32_t hwrng_read_flag;
static struct hwrng rng;

spinlock_t hwrandom_lock;
#if defined(CONFIG_EXYRNG_FAIL_POLICY_DISABLE)
static int hwrng_disabled;
#endif
static int start_up_test;

#ifdef CONFIG_EXYRNG_DEBUG
#define exyrng_debug(args...)	printk(KERN_INFO args)
#else
#define exyrng_debug(args...)
#endif

void exynos_swd_test_fail(void)
{
#if defined(CONFIG_EXYRNG_FAIL_POLICY_DISABLE)
	hwrng_disabled = 1;
	printk("[ExyRNG] disabled for test failures\n");
#else /* defined(CONFIG_EXYRNG_POLICY_RESET) */
	panic("[ExyRNG] It failed to health tests. It means that it detects "
	"the malfunction of TRNG(HW) which generates random numbers. If it "
	"doesn't offer enough entropy, it should not be used. The system "
	"reset could be a way to solve it. The health tests are designed "
	"to have the false positive rate of approximately once per billion "
	"based on min-entropy of TRNG.\n");
#endif
}

static int exynos_cm_smc(uint64_t *arg0, uint64_t *arg1,
			 uint64_t *arg2, uint64_t *arg3)
{
	register uint64_t reg0 __asm__("x0") = *arg0;
	register uint64_t reg1 __asm__("x1") = *arg1;
	register uint64_t reg2 __asm__("x2") = *arg2;
	register uint64_t reg3 __asm__("x3") = *arg3;

	__asm__ volatile (
		"smc    0\n"
		: "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3)
	);

	*arg0 = reg0;
	*arg1 = reg1;
	*arg2 = reg2;
	*arg3 = reg3;

	return *arg0;
}

static int exynos_swd_startup_test(void)
{
	uint64_t reg0;
	uint64_t reg1;
	uint64_t reg2;
	uint64_t reg3;
	uint32_t start_up_size;
	uint32_t retry_cnt;
	uint32_t test_cnt;
	int ret = HWRNG_RET_OK;

	start_up_size = EXYRNG_START_UP_SIZE;

	retry_cnt = 0;
	test_cnt = 1;
	while (start_up_size) {
		reg0 = SMC_CMD_RANDOM;
		reg1 = HWRNG_GET_DATA;
		reg2 = 1;
		reg3 = 0;

		ret = exynos_cm_smc(&reg0, &reg1, &reg2, &reg3);
		if (ret == HWRNG_RET_RETRY_ERROR) {
			if (retry_cnt++ > EXYRNG_RETRY_MAX_COUNT) {
				printk("[ExyRNG] exceed retry in start-up test\n");
				break;
			}
			usleep_range(50, 100);
			continue;
		}

		if (ret == HWRNG_RET_TEST_ERROR || ret == HWRNG_RET_TEST_KAT_ERROR) {
#ifndef CONFIG_EXYRNG_USE_CRYPTOMANAGER
			if (ret == HWRNG_RET_TEST_KAT_ERROR) {
				printk("[ExyRNG] start-up KAT test failed: %d\n", ret);
			} else if (test_cnt < EXYRNG_START_UP_TEST_MAX_RETRY) {
				start_up_size = EXYRNG_START_UP_SIZE;
				test_cnt++;
				printk("[ExyRNG] It performs start-up test "
				"again to detect the malfunction of TRNG with "
				"accuracy\n");
				continue;
			}
#endif
			exynos_swd_test_fail();
			return -EFAULT;
		}

		/* start-up test is performed already */
		if (ret == HWRNG_RET_START_UP_TEST_DONE) {
			ret = HWRNG_RET_OK;
			exyrng_debug("[ExyRNG] start-up test is already done\n");
			break;
		}

		if (ret != HWRNG_RET_OK) {
			exyrng_debug("[ExyRNG] failed to get random\n");
			return -EFAULT;
		}

		if (start_up_size >= 32)
			start_up_size -= 32;
		else
			start_up_size = 0;

		retry_cnt = 0;
	}

	return ret;
}

static int exynos_swd_read(struct hwrng *rng, void *data, size_t max, bool wait)
{
	uint64_t reg0;
	uint64_t reg1;
	uint64_t reg2;
	uint64_t reg3;
	uint32_t *read_buf = data;
	uint32_t read_size = max;
	unsigned long flag;
	uint32_t retry_cnt;
	int ret = HWRNG_RET_OK;

#if defined(CONFIG_EXYRNG_FAIL_POLICY_DISABLE)
	if (hwrng_disabled)
		return -EPERM;
#endif
	retry_cnt = 0;
	do {
		spin_lock_irqsave(&hwrandom_lock, flag);
		if (hwrng_read_flag == 0) {
			reg0 = SMC_CMD_RANDOM;
			reg1 = HWRNG_INIT;
			reg2 = 0;
			reg3 = 0;

			ret = exynos_cm_smc(&reg0, &reg1, &reg2, &reg3);
			if (ret == HWRNG_RET_OK)
				hwrng_read_flag = 1;
			spin_unlock_irqrestore(&hwrandom_lock, flag);

			if (ret == HWRNG_RET_RETRY_ERROR) {
				if (retry_cnt++ > EXYRNG_RETRY_MAX_COUNT) {
					printk("[ExyRNG] exceed retry in init\n");
					break;
				}
				usleep_range(50, 100);
			} else if (ret == HWRNG_RET_TEST_ERROR) {
				printk("[ExyRNG] health test fail after resume\n");
				ret = -EAGAIN;
				goto out;
			}
		} else {
			spin_unlock_irqrestore(&hwrandom_lock, flag);
			break;
		}
	} while (ret == HWRNG_RET_RETRY_ERROR);
	if (ret != HWRNG_RET_OK) {
		msleep(1);
		return -EFAULT;
	}

	if (start_up_test) {
		ret = exynos_swd_startup_test();
		if (ret != HWRNG_RET_OK)
			goto out;

		start_up_test = 0;
		printk("[ExyRNG] passed the start-up test\n");
	}

	retry_cnt = 0;
	while (read_size) {
		spin_lock_irqsave(&hwrandom_lock, flag);

		reg0 = SMC_CMD_RANDOM;
		reg1 = HWRNG_GET_DATA;
		reg2 = 0;
		reg3 = 0;

		ret = exynos_cm_smc(&reg0, &reg1, &reg2, &reg3);

		spin_unlock_irqrestore(&hwrandom_lock, flag);

		if (ret == HWRNG_RET_RETRY_ERROR) {
			if (retry_cnt++ > EXYRNG_RETRY_MAX_COUNT) {
				ret = -EFAULT;
				printk("[ExyRNG] exceed retry in read\n");
				goto out;
			}
			usleep_range(50, 100);
			continue;
		}

		if (ret == HWRNG_RET_TEST_ERROR) {
			exyrng_debug("[ExyRNG] failed to continuous test\n");
			ret = -EAGAIN;
			goto out;
		}

		if (ret != HWRNG_RET_OK) {
			ret = -EFAULT;
			goto out;
		}

		*(uint32_t*)(read_buf++) = (uint32_t)reg2;
		*(uint32_t*)(read_buf++) = (uint32_t)reg3;

		read_size -= 8;
		retry_cnt = 0;
	}

	ret = max;

out:
	retry_cnt = 0;
	do {
		spin_lock_irqsave(&hwrandom_lock, flag);

		reg0 = SMC_CMD_RANDOM;
		reg1 = HWRNG_EXIT;
		reg2 = 0;
		reg3 = 0;

		if (!exynos_cm_smc(&reg0, &reg1, &reg2, &reg3)) {
			hwrng_read_flag = 0;
			spin_unlock_irqrestore(&hwrandom_lock, flag);
			break;
		}
		spin_unlock_irqrestore(&hwrandom_lock, flag);

		if (retry_cnt++ > EXYRNG_RETRY_MAX_COUNT) {
			printk("[ExyRNG] exceed retry in exit\n");
			break;
		}
		usleep_range(50, 100);
	} while(1);

	return ret;
}

static int exyswd_rng_probe(struct platform_device *pdev)
{
	int ret;

	rng.name = "exyswd_rng";
	rng.read = exynos_swd_read;
	rng.quality = 500;

	spin_lock_init(&hwrandom_lock);
#if defined(CONFIG_EXYRNG_FIPS_COMPLIANCE)
	start_up_test = 1;
#endif

	ret = hwrng_register(&rng);
	if (ret)
		return ret;

	printk(KERN_INFO "ExyRNG: hwrng registered\n");

	return 0;
}

static int exyswd_rng_remove(struct platform_device *pdev)
{
	hwrng_unregister(&rng);

	return 0;
}

#if defined(CONFIG_PM_SLEEP) || defined(CONFIG_PM_RUNTIME)
static int exyswd_rng_suspend(struct device *dev)
{
	uint64_t reg0;
	uint64_t reg1;
	uint64_t reg2;
	uint64_t reg3;
	unsigned long flag;
	int ret = HWRNG_RET_OK;

	spin_lock_irqsave(&hwrandom_lock, flag);
	if (hwrng_read_flag) {
		reg0 = SMC_CMD_RANDOM;
		reg1 = HWRNG_EXIT;
		reg2 = 0;
		reg3 = 0;

		ret = exynos_cm_smc(&reg0, &reg1, &reg2, &reg3);
		if (ret != HWRNG_RET_OK)
			printk("[ExyRNG] failed to enter suspend with %d\n", ret);
	}
	spin_unlock_irqrestore(&hwrandom_lock, flag);

	return ret;
}

static int exyswd_rng_resume(struct device *dev)
{
	uint64_t reg0;
	uint64_t reg1;
	uint64_t reg2;
	uint64_t reg3;
	unsigned long flag;
	int ret = HWRNG_RET_OK;

	spin_lock_irqsave(&hwrandom_lock, flag);
#if defined(CONFIG_EXYRNG_FIPS_COMPLIANCE)
	reg0 = SMC_CMD_RANDOM;
	reg1 = HWRNG_RESUME;
	reg2 = 0;
	reg3 = 0;

	ret = exynos_cm_smc(&reg0, &reg1, &reg2, &reg3);
	if (ret != HWRNG_RET_OK)
		printk("[ExyRNG] failed to resume with %d\n", ret);
#endif
	if (hwrng_read_flag) {
		reg0 = SMC_CMD_RANDOM;
		reg1 = HWRNG_INIT;
		reg2 = 0;
		reg3 = 0;

		ret = exynos_cm_smc(&reg0, &reg1, &reg2, &reg3);
		if (ret != HWRNG_RET_OK)
			printk("[ExyRNG] failed to resume with %d\n", ret);
	}
	spin_unlock_irqrestore(&hwrandom_lock, flag);

	return ret;
}
#endif

static UNIVERSAL_DEV_PM_OPS(exyswd_rng_pm_ops, exyswd_rng_suspend, exyswd_rng_resume, NULL);

static struct platform_driver exyswd_rng_driver = {
	.probe		= exyswd_rng_probe,
	.remove		= exyswd_rng_remove,
	.driver		= {
		.name	= "exyswd_rng",
		.owner	= THIS_MODULE,
		.pm     = &exyswd_rng_pm_ops,
	},
};

static struct platform_device exyswd_rng_device = {
	.name = "exyswd_rng",
	.id = -1,
};

static int __init exyswd_rng_init(void)
{
	int ret;

	ret = platform_device_register(&exyswd_rng_device);
	if (ret)
		return ret;

	ret = platform_driver_register(&exyswd_rng_driver);
	if (ret) {
		platform_device_unregister(&exyswd_rng_device);
		return ret;
	}

	printk(KERN_INFO "ExyRNG driver, (c) 2014 Samsung Electronics\n");

	return 0;
}

static void __exit exyswd_rng_exit(void)
{
	platform_driver_unregister(&exyswd_rng_driver);
	platform_device_unregister(&exyswd_rng_device);
}

module_init(exyswd_rng_init);
module_exit(exyswd_rng_exit);

MODULE_DESCRIPTION("EXYNOS H/W Random Number Generator driver");
MODULE_AUTHOR("Sehee Kim <sehi.kim@samsung.com>");
MODULE_LICENSE("GPL");
