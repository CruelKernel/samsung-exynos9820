/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * exynos fimc-is2 video functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <soc/samsung/exynos-pmu.h>

#include "fimc-is-config.h"
#include "fimc-is-type.h"
#include "fimc-is-regs.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-ischain.h"

#define PMU_CAM0_REG_OFFSET 0x4020
#define PMU_CAM1_REG_OFFSET 0x40A0
#define PMU_ISP0_REG_OFFSET 0x4140
#define PMU_ISP1_REG_OFFSET 0x4160
#define PMU_ARM_REG_OFFSET 0x2580

int fimc_is_sensor_runtime_suspend_pre(struct device *dev)
{
	int ret = 0;
	struct fimc_is_device_sensor *device;
	unsigned int state;

	device = (struct fimc_is_device_sensor *)dev_get_drvdata(dev);
	if (!device) {
		err("device is NULL");
		goto p_err;
	}

	if (device->instance == 0) {
		ret = exynos_pmu_read(PMU_CAM0_REG_OFFSET + 0x4, &state);
		if (ret) {
			warn("Couldn't get CAM0 PMU STATE register\n");
		} else if ((state & 0xF) != 0xF) {
			err("CAM0 power is not on(0x%08x)\n", state);
			ret = -EINVAL;
			goto p_err;
		}
	} else {
		ret = exynos_pmu_read(PMU_CAM1_REG_OFFSET + 0x4, &state);
		if (ret) {
			warn("Couldn't get CAM1 PMU STATE register\n");
		} else if ((state & 0xF) != 0xF) {
			err("CAM1 power is not on(0x%08x)\n", state);
			ret = -EINVAL;
			goto p_err;
		}
	}

p_err:
	return ret;
}

int fimc_is_sensor_runtime_resume_pre(struct device *dev)
{
	int ret = 0;
	struct fimc_is_device_sensor *device;

	device = (struct fimc_is_device_sensor *)dev_get_drvdata(dev);
	if (!device) {
		err("device is NULL");
		goto p_err;
	}

#ifndef CONFIG_PM
	{
		u32 try_count;
		unsigned int state;

		/* CAM0 */
		/* Power On */
		exynos_pmu_read(PMU_CAM0_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0xF)
			warn("CAM0 power is already on(0x%08x)\n", state);

		exynos_pmu_update(PMU_CAM0_REG_OFFSET + 0x8, 0x1 << 1, 0x1 << 1);
		exynos_pmu_update(PMU_CAM0_REG_OFFSET, 0xF << 0, 0xF << 0);

		for (try_count = 0; try_count < 10; ++try_count) {
			exynos_pmu_read(PMU_CAM0_REG_OFFSET + 0x4, &state);
			if ((state & 0xF) == 0xF) {
				minfo("CAM0 power on(0x%08x)\n", device, state);
				break;
			}
			mdelay(1);
		}

		if (try_count >= 10) {
			merr("CAM0 power on is fail", device);
			goto p_err;
		}

		/* CAM1 */
		/* Power On */
		exynos_pmu_read(PMU_CAM1_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0xF)
			warn("CAM1 power is already on(0x%08x)\n", state);

		exynos_pmu_update(PMU_CAM1_REG_OFFSET + 0x8, 0x1 << 1, 0x1 << 1);
		exynos_pmu_update(PMU_CAM1_REG_OFFSET, 0xF << 0, 0xF << 0);

		for (try_count = 0; try_count < 10; ++try_count) {
			exynos_pmu_read(PMU_CAM1_REG_OFFSET + 0x4, &state);
			if ((state & 0xF) == 0xF) {
				minfo("CAM1 power on(0x%08x)\n", device, state);
				break;
			}
			mdelay(1);
		}

		if (try_count >= 10) {
			merr("CAM1 power on is fail", device);
			goto p_err;
		}
	}
#endif

p_err:
	return ret;
}

int fimc_is_ischain_runtime_suspend_post(struct device *dev)
{
#ifndef CONFIG_PM
	u32 try_count;
	unsigned int state;

	/* CAM0 */
	/* Power Off */
	exynos_pmu_update(PMU_CAM0_REG_OFFSET + 0x8, 0x1 << 1, 0x1 << 1);
	exynos_pmu_update(PMU_CAM0_REG_OFFSET, 0xF << 0, 0x0 << 0);

	for (try_count = 0; try_count < 10; ++try_count) {
		exynos_pmu_read(PMU_CAM0_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0) {
			info("CAM0 power off(0x%08x)\n", state);
			break;
		}
		mdelay(1);
	}

	if (try_count >= 10)
		err("CAM0 power off is fail(%X)", state);

	/* ISP0 */
	/* Power Off */
	exynos_pmu_update(PMU_ISP0_REG_OFFSET + 0x8, 0x1 << 1, 0x1 << 1);
	exynos_pmu_update(PMU_ISP0_REG_OFFSET, 0xF << 0, 0x0 << 0);

	for (try_count = 0; try_count < 10; ++try_count) {
		exynos_pmu_read(PMU_ISP0_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0) {
			info("ISP0 power off(0x%08x)\n", state);
			break;
		}
		mdelay(1);
	}

	if (try_count >= 10)
		err("ISP0 power off is fail(%X)", state);

	/* ISP1 */
	/* Power Off */
	exynos_pmu_update(PMU_ISP1_REG_OFFSET + 0x8, 0x1 << 1, 0x1 << 1);
	exynos_pmu_update(PMU_ISP1_REG_OFFSET, 0xF << 0, 0x0 << 0);

	for (try_count = 0; try_count < 10; ++try_count) {
		exynos_pmu_read(PMU_ISP1_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0) {
			info("ISP1 power off(0x%08x)\n", state);
			break;
		}
		mdelay(1);
	}

	if (try_count >= 10)
		err("ISP1 power off is fail(%X)", state);

	/* CAM1 */
	/* Power Off */
	exynos_pmu_update(PMU_CAM1_REG_OFFSET + 0x8, 0x1FF << 0, 0x102 << 0);
	exynos_pmu_update(PMU_CAM1_REG_OFFSET, 0xF << 0, 0x0 << 0);

	for (try_count = 0; try_count < 2000; ++try_count) {
		exynos_pmu_read(PMU_CAM1_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0) {
			info("CAM1 power off(0x%08x)\n", state);
			break;
		}
		mdelay(1);
	}

	if (try_count >= 2000)
		err("CAM1 power off is fail(%X)", state);
#endif

	return 0;
}

int fimc_is_ischain_runtime_resume_pre(struct device *dev)
{
	int ret = 0;
	u32 timeout;
	unsigned int state;

	timeout = 1000;
	do {
		exynos_pmu_read(PMU_CAM1_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0xF)
			break;

		timeout--;
		usleep_range(1000, 1000);
	} while (timeout);

	if (timeout == 0) {
		err("CAM1 power up fail(0x%08x)\n", state);
		ret = -ETIME;
		goto p_err;
	}
#ifndef CONFIG_PM
	{
		u32 try_count;
		unsigned int state;

		/* ISP0 */
		exynos_pmu_read(PMU_ISP0_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0xF)
			warn("ISP0 power is already on(0x%08x)\n", state);

		exynos_pmu_update(PMU_ISP0_REG_OFFSET + 0x8, 0x1 << 1, 0x1 << 1);
		exynos_pmu_update(PMU_ISP0_REG_OFFSET, 0xF << 0, 0xF << 0);

		for (try_count = 0; try_count < 10; ++try_count) {
			exynos_pmu_read(PMU_ISP0_REG_OFFSET + 0x4, &state);
			if ((state & 0xF) == 0xF) {
				info("ISP0 power on(0x%08x)\n", state);
				break;
			}
			mdelay(1);
		}

		if (try_count >= 10)
			err("ISP0 power on is fail");

		/* ISP1 */
		exynos_pmu_read(PMU_ISP1_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0xF)
			warn("ISP1 power is already on(0x%08x)\n", state);

		exynos_pmu_update(PMU_ISP1_REG_OFFSET + 0x8, 0x1 << 1, 0x1 << 1);
		exynos_pmu_update(PMU_ISP1_REG_OFFSET, 0xF << 0, 0xF << 0);

		for (try_count = 0; try_count < 10; ++try_count) {
			exynos_pmu_read(PMU_ISP1_REG_OFFSET + 0x4, &state);
			if ((state & 0xF) == 0xF) {
				info("ISP1 power on(0x%08x)\n", state);
				break;
			}
			mdelay(1);
		}

		if (try_count >= 10)
			err("ISP1 power on is fail");
	}
#endif

	timeout = 1000;
	do {
		exynos_pmu_read(PMU_ISP0_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0xF)
			break;

		timeout--;
		usleep_range(1000, 1000);
	} while (timeout);

	if (timeout == 0) {
		err("ISP0 power up fail(0x%08x)\n", state);
		ret = -ETIME;
		goto p_err;
	}

	timeout = 1000;
	do {
		exynos_pmu_read(PMU_ISP1_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0xF)
			break;

		timeout--;
		usleep_range(1000, 1000);
	} while (timeout);

	if (timeout == 0) {
		err("ISP1 power up fail(0x%08x)\n", state);
		ret = -ETIME;
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_ischain_runtime_resume_post(struct device *dev)
{
	int ret = 0;
	u32 timeout;
	unsigned int state;

	/* check power off */
	timeout = 1000;
	do {
		exynos_pmu_read(PMU_ARM_REG_OFFSET + 0x4, &state);
		if (state & 0x1) {
			warn("A5 is not power off");
			exynos_pmu_update(PMU_ARM_REG_OFFSET, 0x1, 0x0);
		} else {
			break;
		}
	} while (--timeout);

	if (!timeout) {
		err("A5 power off failed");
		ret = -EINVAL;
		goto p_err;
	}

	/* A5 power on*/
	exynos_pmu_update(PMU_ARM_REG_OFFSET, 0x1, 0x1);

	/* A5 Enable */
	exynos_pmu_update(PMU_ARM_REG_OFFSET + 0x8, 0x1 << 15, 0x1 << 15);

	/* check power on */
	timeout = 1000;
	do {
		exynos_pmu_read(PMU_ARM_REG_OFFSET + 0x4, &state);
		if ((state & 0x1) == 0x1)
			break;

		udelay(1);
	} while (--timeout);

	if (!timeout) {
		err("A5 power on failed");
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_runtime_suspend_post(struct device *dev)
{
	int ret = 0;
	u32 timeout;
	unsigned int state;

	timeout = 1000;
	do {
		exynos_pmu_read(PMU_CAM0_REG_OFFSET + 0x4, &state);
		if (!(state & 0xF))
			break;

		timeout--;
		usleep_range(1000, 1000);
	} while (timeout);

	if (timeout == 0) {
		err("CAM0 power down fail(0x%08x)", state);
		ret = -ETIME;
		goto p_err;
	}

	timeout = 1000;
	do {
		exynos_pmu_read(PMU_CAM1_REG_OFFSET + 0x4, &state);
		if (!(state & 0xF))
			break;

		timeout--;
		usleep_range(1000, 1000);
	} while (timeout);

	if (timeout == 0) {
		err("CAM1 power down fail(0x%08x)", state);
		ret = -ETIME;
		goto p_err;
	}

	timeout = 1000;
	do {
		exynos_pmu_read(PMU_ISP0_REG_OFFSET + 0x4, &state);
		if (!(state & 0xF))
			break;

		timeout--;
		usleep_range(1000, 1000);
	} while (timeout);

	if (timeout == 0) {
		err("ISP0 power down fail(0x%08x)", state);
		ret = -ETIME;
		goto p_err;
	}

	timeout = 1000;
	do {
		exynos_pmu_read(PMU_ISP1_REG_OFFSET + 0x4, &state);
		if (!(state & 0xF))
			break;

		timeout--;
		usleep_range(1000, 1000);
	} while (timeout);

	if (timeout == 0) {
		err("ISP1 power down fail(0x%08x)", state);
		ret = -ETIME;
		goto p_err;
	}

p_err:
	return ret;
}
