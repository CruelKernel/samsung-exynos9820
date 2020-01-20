/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DT_H
#define FIMC_IS_DT_H

#include "fimc-is-spi.h"
#include <exynos-fimc-is-module.h>
#include <exynos-fimc-is-sensor.h>

#define DT_READ_U32(node, key, value) do {\
		pprop = key; \
		temp = 0; \
		if (of_property_read_u32((node), key, &temp)) \
			pr_warn("%s: no property in the node.\n", pprop);\
		(value) = temp; \
	} while (0)

#define DT_READ_U32_DEFAULT(node, key, value, default_value) do {\
		pprop = key; \
		temp = 0; \
		if (of_property_read_u32((node), key, &temp)) {\
			pr_warn("%s: no property in the node.\n", pprop);\
			(value) = default_value;\
		} else {\
			(value) = temp; \
		}\
	} while (0)

#define DT_READ_STR(node, key, value) do {\
		pprop = key; \
		if (of_property_read_string((node), key, (const char **)&name)) \
			pr_warn("%s: no property in the node.\n", pprop);\
		(value) = name; \
	} while (0)

/* Deprecated. Use  fimc_is_moudle_callback */
typedef int (*fimc_is_moudle_dt_callback)(struct platform_device *pdev,
	struct exynos_platform_fimc_is_module *pdata);

/* New function for module callback. Use this instead of fimc_is_moudle_dt_callback */
typedef int (*fimc_is_moudle_callback)(struct device *dev,
	struct exynos_platform_fimc_is_module *pdata);

int fimc_is_parse_dt(struct platform_device *pdev);
int fimc_is_sensor_parse_dt(struct platform_device *pdev);
int fimc_is_preprocessor_parse_dt(struct platform_device *pdev);
/* Deprecated. Use  fimc_is_module_parse_dt */
int fimc_is_sensor_module_parse_dt(struct platform_device *pdev,
	fimc_is_moudle_dt_callback callback);
/* New function for module parse dt. Use this instead of fimc_is_sensor_module_parse_dt */
int fimc_is_module_parse_dt(struct device *dev,
	fimc_is_moudle_callback callback);
int fimc_is_spi_parse_dt(struct fimc_is_spi *spi);
#endif
