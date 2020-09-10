/*
 * Copyright (C) 2018 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "fingerprint.h"
#include "qbt2000_common.h"

#if defined(CONFIG_TZDEV_BOOST) && defined(ENABLE_SENSORS_FPRINT_SECURE)
#include <../drivers/misc/tzdev/tz_boost.h>
#endif

int qbt2000_sec_spi_prepare(int speed)
{
	struct clk *fp_spi_pclk, *fp_spi_sclk;
	int ret;

	fp_spi_pclk = clk_get(NULL, "fp-spi-pclk");

	if (IS_ERR(fp_spi_pclk)) {
		pr_err("%s: Can't get fp_spi_pclk\n", __func__);
		return -1;
	}

	fp_spi_sclk = clk_get(NULL, "fp-spi-sclk");

	if (IS_ERR(fp_spi_sclk)) {
		pr_err("%s: Can't get fp_spi_sclk\n", __func__);
		return -1;
	}

	clk_prepare_enable(fp_spi_pclk);
	clk_prepare_enable(fp_spi_sclk);

	if (clk_get_rate(fp_spi_sclk) != (speed * 4)) {

		ret = clk_set_rate(fp_spi_sclk, speed * 4);
		if (ret < 0)
			pr_err("%s, SPI clk set failed: %d\n", __func__, ret);

		else
			pr_debug("%s, Set SPI clock rate: %u(%lu)\n",
				__func__, speed, clk_get_rate(fp_spi_sclk) / 4);
	} else
		pr_debug("%s, Set SPI clock rate: %u(%lu)\n",
			__func__, speed, clk_get_rate(fp_spi_sclk) / 4);

	clk_put(fp_spi_pclk);
	clk_put(fp_spi_sclk);

	return 0;
}

int qbt2000_sec_spi_unprepare(void)
{
	struct clk *fp_spi_pclk, *fp_spi_sclk;

	fp_spi_pclk = clk_get(NULL, "fp-spi-pclk");
	if (IS_ERR(fp_spi_pclk)) {
		pr_err("%s: Can't get fp_spi_pclk\n", __func__);
		return -1;
	}

	fp_spi_sclk = clk_get(NULL, "fp-spi-sclk");

	if (IS_ERR(fp_spi_sclk)) {
		pr_err("%s: Can't get fp_spi_sclk\n", __func__);
		return -1;
	}

	clk_disable_unprepare(fp_spi_pclk);
	clk_disable_unprepare(fp_spi_sclk);

	clk_put(fp_spi_pclk);
	clk_put(fp_spi_sclk);

	return 0;
}

int qbt2000_set_clk(struct qbt2000_drvdata *drvdata, bool onoff)
{
	int rc = 0;

	if (drvdata->enabled_clk == onoff) {
		pr_err("%s: already %s\n", __func__, onoff ? "enabled" : "disabled");
		return rc;
	}

	if (onoff) {
		rc = qbt2000_sec_spi_prepare(drvdata->spi_speed);
		if (rc < 0) {
			pr_err("%s: couldn't enable spi clk: %d\n", __func__, rc);
			return rc;
		}
		wake_lock(&drvdata->fp_spi_lock);
		drvdata->enabled_clk = true;
	} else {
		rc = qbt2000_sec_spi_unprepare();
		if (rc < 0) {
			pr_err("%s: couldn't disable spi clk: %d\n", __func__, rc);
			return rc;
		}
		wake_unlock(&drvdata->fp_spi_lock);
		drvdata->enabled_clk = false;
	}
	return rc;
}

int qbt2000_register_platform_variable(struct qbt2000_drvdata *drvdata)
{
	return 0;
}

int qbt2000_unregister_platform_variable(struct qbt2000_drvdata *drvdata)
{
	return 0;
}

int qbt2000_set_cpu_speedup(struct qbt2000_drvdata *drvdata, int onoff)
{
	int rc = 0;
#if defined(CONFIG_TZDEV_BOOST) && defined(ENABLE_SENSORS_FPRINT_SECURE)
	if (onoff) {
		pr_info("%s: SPEEDUP ON:%d\n", __func__, onoff);
		tz_boost_enable();
	} else {
		pr_info("%s: SPEEDUP OFF:%d\n", __func__, onoff);
		tz_boost_disable();
	}
#endif
	return rc;
}

