/*
 * exynos_tmu.c - Samsung EXYNOS TMU (Thermal Management Unit)
 *
 *  Copyright (C) 2014 Samsung Electronics
 *  Bartlomiej Zolnierkiewicz <b.zolnierkie@samsung.com>
 *  Lukasz Majewski <l.majewski@samsung.com>
 *
 *  Copyright (C) 2011 Samsung Electronics
 *  Donggeun Kim <dg77.kim@samsung.com>
 *  Amit Daniel Kachhap <amit.kachhap@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/suspend.h>
#include <linux/pm_qos.h>
#include <linux/threads.h>
#include <linux/thermal.h>
#include <linux/gpu_cooling.h>
#include <linux/isp_cooling.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/debug-snapshot.h>
#include <linux/cpuhotplug.h>
#include <soc/samsung/tmu.h>
#include <soc/samsung/ect_parser.h>
#ifdef CONFIG_EXYNOS_MCINFO
#include <soc/samsung/exynos-mcinfo.h>
#endif
#include <dt-bindings/thermal/thermal_exynos.h>

#include "exynos_tmu.h"
#include "../thermal_core.h"
#ifdef CONFIG_EXYNOS_ACPM_THERMAL
#include "exynos_acpm_tmu.h"
#include "soc/samsung/exynos-pmu.h"
#endif
#include <soc/samsung/exynos-cpuhp.h>

#ifdef CONFIG_SEC_PM
#include <linux/sec_class.h>
#endif

/* Exynos generic registers */
#if defined(CONFIG_SOC_EXYNOS9810)
/* Exynos9810 */
#define EXYNOS_TMU_REG_TRIMINFO7_0(p)	(((p) - 0) * 4)
#define EXYNOS_TMU_REG_TRIMINFO15_8(p)	(((p) - 8) * 4 + 0x400)
#define EXYNOS_TMU_REG_TRIMINFO(p)	(((p) <= 7) ? \
						EXYNOS_TMU_REG_TRIMINFO7_0(p) : \
						EXYNOS_TMU_REG_TRIMINFO15_8(p))
#define EXYNOS_TMU_REG_CONTROL		0x20
#define EXYNOS_TMU_REG_CONTROL1		0x24
#define EXYNOS_TMU_REG_STATUS		0x28
#define EXYNOS_TMU_REG_CURRENT_TEMP1_0	0x40
#define EXYNOS_TMU_REG_CURRENT_TEMP4_2  0x44
#define EXYNOS_TMU_REG_CURRENT_TEMP7_5  0x48
#define EXYNOS_TMU_REG_CURRENT_TEMP9_8		0x440
#define EXYNOS_TMU_REG_CURRENT_TEMP12_10	0x444
#define EXYNOS_TMU_REG_CURRENT_TEMP15_13	0x448
#define EXYNOS_TMU_REG_INTEN0		0x110
#define EXYNOS_TMU_REG_INTEN5		0x310
#define EXYNOS_TMU_REG_INTEN8		0x650
#define EXYNOS_TMU_REG_INTSTAT		0x74
#define EXYNOS_TMU_REG_INTCLEAR		0x78

#define EXYNOS_TMU_REG_THD_TEMP0		0x50
#define EXYNOS_TMU_REG_THD_TEMP1		0x170
#define EXYNOS_TMU_REG_THD_TEMP8		0x450

#define EXYNOS_THD_TEMP_RISE7_6			0x50
#define EXYNOS_THD_TEMP_FALL7_6			0x60
#define EXYNOS_THD_TEMP_R_OFFSET		0x100

#define EXYNOS_TMU_REG_TRIM0			(0x3C)

#define EXYNOS_TMU_REG_INTPEND0			(0x118)
#define EXYNOS_TMU_REG_INTPEND5			(0x318)
#define EXYNOS_TMU_REG_INTPEND8			(0x658)

#define EXYNOS_TMU_REG_THD(p)			((p == 0) ?					\
						EXYNOS_TMU_REG_THD_TEMP0 :			\
						((p < 8) ?					\
						(EXYNOS_TMU_REG_THD_TEMP1 + (p - 1) * 0x20) :	\
						(EXYNOS_TMU_REG_THD_TEMP8 + (p - 8) * 0x20)))
#define EXYNOS_TMU_REG_INTEN(p)			((p < 5) ?					\
						(EXYNOS_TMU_REG_INTEN0 + p * 0x10) :		\
						((p < 8) ?					\
						(EXYNOS_TMU_REG_INTEN5 + (p - 5) * 0x10) :	\
						(EXYNOS_TMU_REG_INTEN8 + (p - 8) * 0x10)))
#define EXYNOS_TMU_REG_INTPEND(p)		((p < 5) ?					\
						(EXYNOS_TMU_REG_INTPEND0 + p * 0x10) :		\
						((p < 8) ?					\
						(EXYNOS_TMU_REG_INTPEND5 + (p - 5) * 0x10) :	\
						(EXYNOS_TMU_REG_INTPEND8 + (p - 8) * 0x10)))

#define EXYNOS_TMU_REG_EMUL_CON			(0x160)

#define EXYNOS_TMU_REG_AVG_CON			(0x38)
#define EXYNOS_TMU_REG_COUNTER_VALUE0		(0x30)
#define EXYNOS_TMU_REG_COUNTER_VALUE1		(0x34)

#define EXYNOS_TMU_TEMP_SHIFT			(9)

#define EXYNOS_GPU_THERMAL_ZONE_ID		(2)
/* Exynos9810 */
#elif defined(CONFIG_SOC_EXYNOS9820)
/* Exynos9820 */
#define EXYNOS_TMU_REG_TRIMINFO(p)		(((p) - 0) * 4)
#define EXYNOS_TMU_REG_CONTROL			(0x50)
#define EXYNOS_TMU_REG_CONTROL1			(0x54)
#define EXYNOS_TMU_REG_STATUS			(0x80)
#define EXYNOS_TMU_REG_CURRENT_TEMP1_0		(0x84)

#define EXYNOS_TMU_REG_THD_TEMP0		(0xD0)
#define EXYNOS_TMU_REG_INTEN0			(0xF0)
#define EXYNOS_TMU_REG_INTPEND0			(0xF8)
#define EXYNOS_TMU_REG_SENSOR_OFFSET		(0x50)

#define EXYNOS_TMU_REG_THD(p)			(EXYNOS_TMU_REG_THD_TEMP0 + p * EXYNOS_TMU_REG_SENSOR_OFFSET)
#define EXYNOS_TMU_REG_INTPEND(p)		(EXYNOS_TMU_REG_INTPEND0 + p * EXYNOS_TMU_REG_SENSOR_OFFSET)
#define EXYNOS_TMU_REG_INTEN(p)			(EXYNOS_TMU_REG_INTEN0 + p * EXYNOS_TMU_REG_SENSOR_OFFSET)

#define EXYNOS_TMU_REG_TRIM0			(0x5C)
#define EXYNOS_TMU_REG_EMUL_CON			(0xB0)

#define EXYNOS_TMU_REG_AVG_CON			(0x58)
#define EXYNOS_TMU_REG_COUNTER_VALUE0		(0x74)
#define EXYNOS_TMU_REG_COUNTER_VALUE1		(0x78)

#define EXYNOS_TMU_TEMP_SHIFT			(16)

#define EXYNOS_GPU_THERMAL_ZONE_ID		(3)
/* Exynos9820 */
#endif
#include "linux/mcu_ipc.h"

#define EXYNOS_TMU_REF_VOLTAGE_SHIFT	24
#define EXYNOS_TMU_REF_VOLTAGE_MASK	0x1f
#define EXYNOS_TMU_BUF_SLOPE_SEL_MASK	0xf
#define EXYNOS_TMU_BUF_SLOPE_SEL_SHIFT	8
#define EXYNOS_TMU_CORE_EN_SHIFT	0
#define EXYNOS_TMU_MUX_ADDR_SHIFT	20
#define EXYNOS_TMU_MUX_ADDR_MASK	0x7
#define EXYNOS_TMU_PTAT_CON_SHIFT	20
#define EXYNOS_TMU_PTAT_CON_MASK	0x7
#define EXYNOS_TMU_BUF_CONT_SHIFT	12
#define EXYNOS_TMU_BUF_CONT_MASK	0xf

#define EXYNOS_TMU_TRIP_MODE_SHIFT	13
#define EXYNOS_TMU_TRIP_MODE_MASK	0x7
#define EXYNOS_TMU_THERM_TRIP_EN_SHIFT	12

#define EXYNOS_TMU_INTEN_RISE0_SHIFT	0
#define EXYNOS_TMU_INTEN_FALL0_SHIFT	16

#define EXYNOS_EMUL_TIME	0x57F0
#define EXYNOS_EMUL_TIME_MASK	0xffff
#define EXYNOS_EMUL_TIME_SHIFT	16
#define EXYNOS_EMUL_DATA_SHIFT	7
#define EXYNOS_EMUL_DATA_MASK	0x1FF
#define EXYNOS_EMUL_ENABLE	0x1

#define EXYNOS_THD_TEMP_RISE7_6_SHIFT		16
#define EXYNOS_TMU_INTEN_RISE0_SHIFT		0
#define EXYNOS_TMU_INTEN_RISE1_SHIFT		1
#define EXYNOS_TMU_INTEN_RISE2_SHIFT		2
#define EXYNOS_TMU_INTEN_RISE3_SHIFT		3
#define EXYNOS_TMU_INTEN_RISE4_SHIFT		4
#define EXYNOS_TMU_INTEN_RISE5_SHIFT		5
#define EXYNOS_TMU_INTEN_RISE6_SHIFT		6
#define EXYNOS_TMU_INTEN_RISE7_SHIFT		7

#define EXYNOS_TMU_CALIB_SEL_SHIFT		(23)
#define EXYNOS_TMU_CALIB_SEL_MASK		(0x1)
#define EXYNOS_TMU_TEMP_MASK			(0x1ff)
#define EXYNOS_TMU_TRIMINFO_85_P0_SHIFT		(9)
#define EXYNOS_TRIMINFO_ONE_POINT_TRIMMING	(0)
#define EXYNOS_TRIMINFO_TWO_POINT_TRIMMING	(1)
#define EXYNOS_TMU_T_BUF_VREF_SEL_SHIFT		(18)
#define EXYNOS_TMU_T_BUF_VREF_SEL_MASK		(0x1F)
#define EXYNOS_TMU_T_PTAT_CONT_MASK		(0x7)
#define EXYNOS_TMU_T_BUF_SLOPE_SEL_SHIFT	(18)
#define EXYNOS_TMU_T_BUF_SLOPE_SEL_MASK		(0xF)
#define EXYNOS_TMU_T_BUF_CONT_MASK		(0xF)

#define EXYNOS_TMU_T_TRIM0_SHIFT		(18)
#define EXYNOS_TMU_T_TRIM0_MASK			(0xF)
#define EXYNOS_TMU_BGRI_TRIM_SHIFT		(20)
#define EXYNOS_TMU_BGRI_TRIM_MASK		(0xF)
#define EXYNOS_TMU_VREF_TRIM_SHIFT		(12)
#define EXYNOS_TMU_VREF_TRIM_MASK		(0xF)
#define EXYNOS_TMU_VBEI_TRIM_SHIFT		(8)
#define EXYNOS_TMU_VBEI_TRIM_MASK		(0xF)

#define EXYNOS_TMU_AVG_CON_SHIFT		(18)
#define EXYNOS_TMU_AVG_CON_MASK			(0x3)
#define EXYNOS_TMU_AVG_MODE_MASK		(0x7)
#define EXYNOS_TMU_AVG_MODE_DEFAULT		(0x0)
#define EXYNOS_TMU_AVG_MODE_2			(0x5)
#define EXYNOS_TMU_AVG_MODE_4			(0x6)

#define EXYNOS_TMU_DEM_ENABLE			(1)
#define EXYNOS_TMU_DEM_SHIFT			(4)

#define EXYNOS_TMU_EN_TEMP_SEN_OFF_SHIFT	(0)
#define EXYNOS_TMU_EN_TEMP_SEN_OFF_MASK		(0xffff)
#define EXYNOS_TMU_CLK_SENSE_ON_SHIFT		(16)
#define EXYNOS_TMU_CLK_SENSE_ON_MASK		(0xffff)
#define EXYNOS_TMU_TEM1456X_SENSE_VALUE		(0x0A28)
#define EXYNOS_TMU_TEM1051X_SENSE_VALUE		(0x028A)

#define EXYNOS_TMU_NUM_PROBE_SHIFT		(16)
#define EXYNOS_TMU_NUM_PROBE_MASK		(0xf)
#define EXYNOS_TMU_LPI_MODE_SHIFT		(10)
#define EXYNOS_TMU_LPI_MODE_MASK		(1)

#define TOTAL_SENSORS		16
#define DEFAULT_BALANCE_OFFSET	20

#ifdef CONFIG_EXYNOS_ACPM_THERMAL
#if defined(CONFIG_SOC_EXYNOS9810)
#define PMUREG_AUD_STATUS			0x4004
#define PMUREG_AUD_STATUS_MASK			0xF
#elif defined(CONFIG_SOC_EXYNOS9820)
#define PMUREG_AUD_STATUS			0x1904
#define PMUREG_AUD_STATUS_MASK			0x1
#endif
static struct acpm_tmu_cap cap;
static unsigned int num_of_devices, suspended_count;
static bool cp_call_mode;
static bool is_aud_on(void)
{
	unsigned int val;

	exynos_pmu_read(PMUREG_AUD_STATUS, &val);

	pr_info("%s AUD_STATUS %d\n", __func__, val);

	return ((val & PMUREG_AUD_STATUS_MASK) == PMUREG_AUD_STATUS_MASK);
}

#ifdef CONFIG_MCU_IPC
#define CP_MBOX_NUM		3
#define CP_MBOX_MASK		1
#define CP_MBOX_SHIFT		25
static bool is_cp_net_conn(void)
{
	unsigned int val;

	val = mbox_extract_value(MCU_CP, CP_MBOX_NUM, CP_MBOX_MASK, CP_MBOX_SHIFT);

	pr_info("%s CP mobx value %d\n", __func__, val);

	return !!val;
}
#else
static bool is_cp_net_conn(void)
{
	return false;
}
#endif
#else
static bool suspended;
static DEFINE_MUTEX (thermal_suspend_lock);
#endif
static bool is_cpu_hotplugged_out;

/* list of multiple instance for each thermal sensor */
static LIST_HEAD(dtm_dev_list);

static u32 t_bgri_trim;
static u32 t_vref_trim;
static u32 t_vbei_trim;

static void exynos_report_trigger(struct exynos_tmu_data *p)
{
	struct thermal_zone_device *tz = p->tzd;

	if (!tz) {
		pr_err("No thermal zone device defined\n");
		return;
	}

	thermal_zone_device_update(tz, THERMAL_EVENT_UNSPECIFIED);
}

/*
 * TMU treats temperature with the index as a mapped temperature code.
 * The temperature is converted differently depending on the calibration type.
 */
static int temp_to_code_with_sensorinfo(struct exynos_tmu_data *data, u16 temp, struct sensor_info *info)
{
	struct exynos_tmu_platform_data *pdata = data->pdata;
	int temp_code;

	if (temp > EXYNOS_MAX_TEMP)
		temp = EXYNOS_MAX_TEMP;
	else if (temp < EXYNOS_MIN_TEMP)
		temp = EXYNOS_MIN_TEMP;

	switch (info->cal_type) {
		case TYPE_TWO_POINT_TRIMMING:
			temp_code = (temp - pdata->first_point_trim) *
				(info->temp_error2 - info->temp_error1) /
				(pdata->second_point_trim - pdata->first_point_trim) +
				info->temp_error1;
			break;
		case TYPE_ONE_POINT_TRIMMING:
			temp_code = temp + info->temp_error1 - pdata->first_point_trim;
			break;
		default:
			temp_code = temp + pdata->default_temp_offset;
			break;
	}

	return temp_code;
}

/*
 * Calculate a temperature value from a temperature code.
 * The unit of the temperature is degree Celsius.
 */
static int code_to_temp(struct exynos_tmu_data *data, u16 temp_code)
{
	struct exynos_tmu_platform_data *pdata = data->pdata;
	int temp;

	switch (pdata->cal_type) {
	case TYPE_TWO_POINT_TRIMMING:
		temp = (temp_code - data->temp_error1) *
			(pdata->second_point_trim - pdata->first_point_trim) /
			(data->temp_error2 - data->temp_error1) +
			pdata->first_point_trim;
		break;
	case TYPE_ONE_POINT_TRIMMING:
		temp = temp_code - data->temp_error1 + pdata->first_point_trim;
		break;
	default:
		temp = temp_code - pdata->default_temp_offset;
		break;
	}

	return temp;
}

/*
 * Calculate a temperature value with the index from a temperature code.
 * The unit of the temperature is degree Celsius.
 */
static int code_to_temp_with_sensorinfo(struct exynos_tmu_data *data, u16 temp_code, struct sensor_info *info)
{
	struct exynos_tmu_platform_data *pdata = data->pdata;
	int temp;

	switch (info->cal_type) {
	case TYPE_TWO_POINT_TRIMMING:
		temp = (temp_code - info->temp_error1) *
			(pdata->second_point_trim - pdata->first_point_trim) /
			(info->temp_error2 - info->temp_error1) +
			pdata->first_point_trim;
		break;
	case TYPE_ONE_POINT_TRIMMING:
		temp = temp_code - info->temp_error1 + pdata->first_point_trim;
		break;
	default:
		temp = temp_code - pdata->default_temp_offset;
		break;
	}

	/* temperature should range between minimum and maximum */
	if (temp > EXYNOS_MAX_TEMP)
		temp = EXYNOS_MAX_TEMP;
	else if (temp < EXYNOS_MIN_TEMP)
		temp = EXYNOS_MIN_TEMP;

	return temp;
}

static int exynos_tmu_initialize(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int ret;

	if (of_thermal_get_ntrips(data->tzd) > data->ntrip) {
		dev_info(&pdev->dev,
			 "More trip points than supported by this TMU.\n");
		dev_info(&pdev->dev,
			 "%d trip points should be configured in polling mode.\n",
			 (of_thermal_get_ntrips(data->tzd) - data->ntrip));
	}

	mutex_lock(&data->lock);
	ret = data->tmu_initialize(pdev);
	mutex_unlock(&data->lock);

	return ret;
}

static void exynos_tmu_control(struct platform_device *pdev, bool on)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);

	mutex_lock(&data->lock);
	data->tmu_control(pdev, on);
	data->enabled = on;
	mutex_unlock(&data->lock);
}

static int exynos98X0_tmu_initialize(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	struct thermal_zone_device *tz = data->tzd;
	struct exynos_tmu_platform_data *pdata = data->pdata;
	unsigned int trim_info, temp_error1, temp_error2;
	unsigned short cal_type;
	unsigned int rising_threshold, falling_threshold;
	unsigned int reg_off, bit_off;
	enum thermal_trip_type type;
	int temp, temp_hist, threshold_code;
	int i, sensor, count = 0, interrupt_count;

	trim_info = readl(data->base + EXYNOS_TMU_REG_TRIMINFO(0));
	cal_type = (trim_info >> EXYNOS_TMU_CALIB_SEL_SHIFT) & EXYNOS_TMU_CALIB_SEL_MASK;

	for (sensor = 0; sensor < TOTAL_SENSORS; sensor++) {

		if (!(data->sensors & (1 << sensor)))
			continue;

		/* Read the sensor error value from TRIMINFOX */
		trim_info = readl(data->base + EXYNOS_TMU_REG_TRIMINFO(sensor));
		temp_error1 = trim_info & EXYNOS_TMU_TEMP_MASK;
		temp_error2 = (trim_info >> EXYNOS_TMU_TRIMINFO_85_P0_SHIFT) & EXYNOS_TMU_TEMP_MASK;

		/* Save sensor id */
		data->sensor_info[count].sensor_num = sensor;
		dev_info(&pdev->dev, "Sensor number = %d\n", sensor);

		/* Check thermal calibration type */
		data->sensor_info[count].cal_type = cal_type;

		/* Check temp_error1 value */
		data->sensor_info[count].temp_error1 = temp_error1;
		if (!data->sensor_info[count].temp_error1)
			data->sensor_info[count].temp_error1 = pdata->efuse_value & EXYNOS_TMU_TEMP_MASK;

		/* Check temp_error2 if calibration type is TYPE_TWO_POINT_TRIMMING */
		if (data->sensor_info[count].cal_type == TYPE_TWO_POINT_TRIMMING) {
			data->sensor_info[count].temp_error2 = temp_error2;

			if (!data->sensor_info[count].temp_error2)
				data->sensor_info[count].temp_error2 =
					(pdata->efuse_value >> EXYNOS_TMU_TRIMINFO_85_P0_SHIFT) &
					EXYNOS_TMU_TEMP_MASK;
		}

		interrupt_count = 0;
		/* Write temperature code for rising and falling threshold */
		for (i = (of_thermal_get_ntrips(tz) - 1); i >= 0; i--) {
			tz->ops->get_trip_type(tz, i, &type);

			if (type == THERMAL_TRIP_PASSIVE)
				continue;

			reg_off = (interrupt_count / 2) * 4;
			bit_off = ((interrupt_count + 1) % 2) * 16;

			reg_off += EXYNOS_TMU_REG_THD(sensor);

			tz->ops->get_trip_temp(tz, i, &temp);
			temp /= MCELSIUS;

			tz->ops->get_trip_hyst(tz, i, &temp_hist);
			temp_hist = temp - (temp_hist / MCELSIUS);

			/* Set 9-bit temperature code for rising threshold levels */
			threshold_code = temp_to_code_with_sensorinfo(data, temp, &data->sensor_info[count]);
			rising_threshold = readl(data->base + reg_off);
			rising_threshold &= ~(EXYNOS_TMU_TEMP_MASK << bit_off);
			rising_threshold |= threshold_code << bit_off;
			writel(rising_threshold, data->base + reg_off);

			/* Set 9-bit temperature code for falling threshold levels */
			threshold_code = temp_to_code_with_sensorinfo(data, temp_hist, &data->sensor_info[count]);
			falling_threshold = readl(data->base + reg_off + 0x10);
			falling_threshold &= ~(EXYNOS_TMU_TEMP_MASK << bit_off);
			falling_threshold |= threshold_code << bit_off;
			writel(falling_threshold, data->base + reg_off + 0x10);
			interrupt_count++;
		}
		count++;
	}

	data->tmu_clear_irqs(data);

	return 0;
}

static void tmu_core_enable(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	unsigned int ctrl;

	/* set TRIP_EN, CORE_EN */
	ctrl = readl(data->base + EXYNOS_TMU_REG_CONTROL);
	ctrl |= (1 << EXYNOS_TMU_THERM_TRIP_EN_SHIFT);
	ctrl |= (1 << EXYNOS_TMU_CORE_EN_SHIFT);
	writel(ctrl, data->base + EXYNOS_TMU_REG_CONTROL);
}

static void tmu_core_disable(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	unsigned int ctrl;

	/* reset TRIP_EN, CORE_EN */
	ctrl = readl(data->base + EXYNOS_TMU_REG_CONTROL);
	ctrl &= ~(1 << EXYNOS_TMU_THERM_TRIP_EN_SHIFT);
	ctrl &= ~(1 << EXYNOS_TMU_CORE_EN_SHIFT);
	writel(ctrl, data->base + EXYNOS_TMU_REG_CONTROL);
}

static void tmu_irqs_enable(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	struct thermal_zone_device *tz = data->tzd;
	int i, offset, interrupt_count = 0;
	unsigned int interrupt_en = 0, bit_off;
	enum thermal_trip_type type;

	for (i = (of_thermal_get_ntrips(tz) - 1); i >= 0; i--) {
		tz->ops->get_trip_type(tz, i, &type);

		if (type == THERMAL_TRIP_PASSIVE)
			continue;

		bit_off = EXYNOS_TMU_INTEN_RISE7_SHIFT - interrupt_count;

		/* rising interrupt */
		interrupt_en |= of_thermal_is_trip_valid(tz, i) << bit_off;
		interrupt_count++;
	}

	/* falling interrupt */
	interrupt_en |= (interrupt_en << EXYNOS_TMU_INTEN_FALL0_SHIFT);

	for (i = 0; i < TOTAL_SENSORS; i++) {
		if (!(data->sensors & (1 << i)))
			continue;

		offset = EXYNOS_TMU_REG_INTEN(i);

		writel(interrupt_en, data->base + offset);
	}
}

static void tmu_irqs_disable(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int i, offset;

	for (i = 0; i < TOTAL_SENSORS; i++) {
		if (!(data->sensors & (1 << i)))
			continue;

		offset = EXYNOS_TMU_REG_INTEN(i);

		writel(0, data->base + offset);
	}
}

static void exynos98X0_tmu_control(struct platform_device *pdev, bool on)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	unsigned int trim, ctrl, con1, avgc;
	unsigned int t_buf_vref_sel, t_buf_slope_sel, avg_mode;
	unsigned int counter_value;

	tmu_core_disable(pdev);
	tmu_irqs_disable(pdev);

	if (!on)
		return;

	trim = readl(data->base + EXYNOS_TMU_REG_TRIMINFO(0));
	t_buf_vref_sel = (trim >> EXYNOS_TMU_T_BUF_VREF_SEL_SHIFT)
				& EXYNOS_TMU_T_BUF_VREF_SEL_MASK;

	trim = readl(data->base + EXYNOS_TMU_REG_TRIMINFO(1));
	t_buf_slope_sel = (trim >> EXYNOS_TMU_T_BUF_SLOPE_SEL_SHIFT)
				& EXYNOS_TMU_T_BUF_SLOPE_SEL_MASK;

	trim = readl(data->base + EXYNOS_TMU_REG_TRIMINFO(2));
	avg_mode = (trim >> EXYNOS_TMU_AVG_CON_SHIFT) & EXYNOS_TMU_AVG_MODE_MASK;

	/* set BUf_VREF_SEL, BUF_SLOPE_SEL */
	ctrl = readl(data->base + EXYNOS_TMU_REG_CONTROL);
	ctrl &= ~(EXYNOS_TMU_REF_VOLTAGE_MASK << EXYNOS_TMU_REF_VOLTAGE_SHIFT);
	ctrl &= ~(EXYNOS_TMU_BUF_SLOPE_SEL_MASK << EXYNOS_TMU_BUF_SLOPE_SEL_SHIFT);
	ctrl |= t_buf_vref_sel << EXYNOS_TMU_REF_VOLTAGE_SHIFT;
	ctrl |= t_buf_slope_sel << EXYNOS_TMU_BUF_SLOPE_SEL_SHIFT;
	writel(ctrl, data->base + EXYNOS_TMU_REG_CONTROL);

	/* set NUM_PROBE, reset LPI_MODE_EN */
	con1 = readl(data->base + EXYNOS_TMU_REG_CONTROL1);
	con1 &= ~(EXYNOS_TMU_NUM_PROBE_MASK << EXYNOS_TMU_NUM_PROBE_SHIFT);
	con1 &= ~(EXYNOS_TMU_LPI_MODE_MASK << EXYNOS_TMU_LPI_MODE_SHIFT);
	con1 |= (data->num_probe << EXYNOS_TMU_NUM_PROBE_SHIFT);
	writel(con1, data->base + EXYNOS_TMU_REG_CONTROL1);

	/* set EN_DEM, AVG_MODE */
	avgc = readl(data->base + EXYNOS_TMU_REG_AVG_CON);
	avgc &= ~(EXYNOS_TMU_AVG_MODE_MASK);
	avgc &= ~(EXYNOS_TMU_DEM_ENABLE << EXYNOS_TMU_DEM_SHIFT);
	if (avg_mode) {
		avgc |= avg_mode;
		avgc |= (EXYNOS_TMU_DEM_ENABLE << EXYNOS_TMU_DEM_SHIFT);
	}
	writel(avgc, data->base + EXYNOS_TMU_REG_AVG_CON);

	/* set COUNTER_VALUE */
	counter_value = readl(data->base + EXYNOS_TMU_REG_COUNTER_VALUE0);
	counter_value &= ~(EXYNOS_TMU_EN_TEMP_SEN_OFF_MASK << EXYNOS_TMU_EN_TEMP_SEN_OFF_SHIFT);
	counter_value |= EXYNOS_TMU_TEM1051X_SENSE_VALUE << EXYNOS_TMU_EN_TEMP_SEN_OFF_SHIFT;
	writel(counter_value, data->base + EXYNOS_TMU_REG_COUNTER_VALUE0);

	counter_value = readl(data->base + EXYNOS_TMU_REG_COUNTER_VALUE1);
	counter_value &= ~(EXYNOS_TMU_CLK_SENSE_ON_MASK << EXYNOS_TMU_CLK_SENSE_ON_SHIFT);
	counter_value |= EXYNOS_TMU_TEM1051X_SENSE_VALUE << EXYNOS_TMU_CLK_SENSE_ON_SHIFT;
	writel(counter_value, data->base + EXYNOS_TMU_REG_COUNTER_VALUE1);

	/* set TRIM0 BGR_I/VREF/VBE_I */
	/* write TRIM0 values read from TMU_TOP to each TMU_TOP and TMU_SUB */
	ctrl = readl(data->base + EXYNOS_TMU_REG_TRIM0);
	ctrl &= ~(EXYNOS_TMU_BGRI_TRIM_MASK << EXYNOS_TMU_BGRI_TRIM_SHIFT);
	ctrl &= ~(EXYNOS_TMU_VREF_TRIM_MASK << EXYNOS_TMU_VREF_TRIM_SHIFT);
	ctrl &= ~(EXYNOS_TMU_VBEI_TRIM_MASK << EXYNOS_TMU_VBEI_TRIM_SHIFT);
	ctrl |= (t_bgri_trim << EXYNOS_TMU_BGRI_TRIM_SHIFT);
	ctrl |= (t_vref_trim << EXYNOS_TMU_VREF_TRIM_SHIFT);
	ctrl |= (t_vbei_trim << EXYNOS_TMU_VBEI_TRIM_SHIFT);
	writel(ctrl, data->base + EXYNOS_TMU_REG_TRIM0);

	tmu_irqs_enable(pdev);
	tmu_core_enable(pdev);
}

#define MCINFO_LOG_THRESHOLD	(4)

static int exynos_get_temp(void *p, int *temp)
{
	struct exynos_tmu_data *data = p;
#ifndef CONFIG_EXYNOS_ACPM_THERMAL
	struct thermal_cooling_device *cdev = NULL;
	struct thermal_zone_device *tz;
	struct thermal_instance *instance;
#endif
#ifdef CONFIG_EXYNOS_MCINFO
	unsigned int mcinfo_count;
	unsigned int mcinfo_result[4] = {0, 0, 0, 0};
	unsigned int mcinfo_logging = 0;
	unsigned int mcinfo_temp = 0;
	unsigned int i;
#endif

	if (!data || !data->tmu_read || !data->enabled)
		return -EINVAL;

	mutex_lock(&data->lock);

	if (data->num_of_sensors)
		*temp = data->tmu_read(data) * MCELSIUS;
	else
		*temp = code_to_temp(data, data->tmu_read(data)) * MCELSIUS;

	mutex_unlock(&data->lock);

#ifndef CONFIG_EXYNOS_ACPM_THERMAL
	tz = data->tzd;

	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		if (instance->cdev) {
			cdev = instance->cdev;
			break;
		}
	}

	if (!cdev)
		return 0;

	mutex_lock(&thermal_suspend_lock);

	if (cdev->ops->set_cur_temp && data->id != 1)
		cdev->ops->set_cur_temp(cdev, suspended, *temp / 1000);

	mutex_unlock(&thermal_suspend_lock);
#endif

	dbg_snapshot_thermal(data->pdata, *temp / 1000, data->tmu_name, 0);
#ifdef CONFIG_EXYNOS_MCINFO
	if (data->id == 0) {
		mcinfo_count = get_mcinfo_base_count();
		get_refresh_rate(mcinfo_result);

		for (i = 0; i < mcinfo_count; i++) {
			mcinfo_temp |= (mcinfo_result[i] & 0xf) << (8 * i);

			if (mcinfo_result[i] >= MCINFO_LOG_THRESHOLD)
				mcinfo_logging = 1;
		}

		if (mcinfo_logging == 1)
			dbg_snapshot_thermal(NULL, mcinfo_temp, "MCINFO", 0);
	}
#endif
	return 0;
}

#ifdef CONFIG_SEC_BOOTSTAT
void sec_bootstat_get_thermal(int *temp)
{
	struct exynos_tmu_data *data;

	list_for_each_entry(data, &dtm_dev_list, node) {
		if (!strncasecmp(data->tmu_name, "BIG", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[0]);
			temp[0] /= 1000;
		} else if (!strncasecmp(data->tmu_name, "MID", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[1]);
			temp[1] /= 1000;
		} else if (!strncasecmp(data->tmu_name, "LITTLE", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[2]);
			temp[2] /= 1000;
		} else if (!strncasecmp(data->tmu_name, "G3D", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[3]);
			temp[3] /= 1000;
		} else if (!strncasecmp(data->tmu_name, "ISP", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[4]);
			temp[4] /= 1000;
		} else
			continue;
	}
}
#endif

static int exynos_get_trend(void *p, int trip, enum thermal_trend *trend)
{
	struct exynos_tmu_data *data = p;
	struct thermal_zone_device *tz = data->tzd;
	int trip_temp, ret = 0;

	ret = tz->ops->get_trip_temp(tz, trip, &trip_temp);
	if (ret < 0)
		return ret;

	if (tz->temperature >= trip_temp)
		*trend = THERMAL_TREND_RAISE_FULL;
	else
		*trend = THERMAL_TREND_DROP_FULL;

	return 0;
}

#ifdef CONFIG_THERMAL_EMULATION
static void exynos98X0_tmu_set_emulation(struct exynos_tmu_data *data,
					 int temp)
{
	unsigned int val;
	u32 emul_con;

	emul_con = EXYNOS_TMU_REG_EMUL_CON;

	val = readl(data->base + emul_con);

	if (temp) {
		temp /= MCELSIUS;
		val &= ~(EXYNOS_EMUL_DATA_MASK << EXYNOS_EMUL_DATA_SHIFT);
		val |= (temp_to_code_with_sensorinfo(data, temp, &data->sensor_info[0]) << EXYNOS_EMUL_DATA_SHIFT)
			| EXYNOS_EMUL_ENABLE;
	} else {
		val &= ~EXYNOS_EMUL_ENABLE;
	}

	writel(val, data->base + emul_con);
}

static int exynos_tmu_set_emulation(void *drv_data, int temp)
{
	struct exynos_tmu_data *data = drv_data;
	int ret = -EINVAL;

	if (temp && temp < MCELSIUS)
		goto out;

	mutex_lock(&data->lock);
	data->tmu_set_emulation(data, temp);
	mutex_unlock(&data->lock);
	return 0;
out:
	return ret;
}
#else
static int exynos_tmu_set_emulation(void *drv_data, int temp)
	{ return -EINVAL; }
#endif /* CONFIG_THERMAL_EMULATION */

#if defined(CONFIG_SOC_EXYNOS9810)
static bool cpufreq_limited;
static struct pm_qos_request thermal_cpu_limit_request;
#endif

static int exynos98X0_tmu_read(struct exynos_tmu_data *data)
{
	int temp = 0, stat = 0;

#ifdef CONFIG_EXYNOS_ACPM_THERMAL
	exynos_acpm_tmu_set_read_temp(data->tzd->id, &temp, &stat);
#endif

#if defined(CONFIG_SOC_EXYNOS9810)
	if (data->hotplug_enable) {
		if ((stat == 2) && !cpufreq_limited) {
			pm_qos_update_request(&thermal_cpu_limit_request,
					data->limited_frequency);
			cpufreq_limited = true;
		} else if ((stat == 0 || stat == 1) && cpufreq_limited) {
			pm_qos_update_request(&thermal_cpu_limit_request,
					PM_QOS_CPU_FREQ_MAX_DEFAULT_VALUE);
			cpufreq_limited = false;
		}
	}
#endif
	return temp;
}

static void exynos_tmu_work(struct work_struct *work)
{
	struct exynos_tmu_data *data = container_of(work,
			struct exynos_tmu_data, irq_work);

	exynos_report_trigger(data);
	mutex_lock(&data->lock);

	data->tmu_clear_irqs(data);

	mutex_unlock(&data->lock);
	enable_irq(data->irq);
}

static void exynos98X0_tmu_clear_irqs(struct exynos_tmu_data *data)
{
	unsigned int i, val_irq;
	u32 pend_reg;

	for (i = 0; i < TOTAL_SENSORS; i++) {
		if (!(data->sensors & (1 << i)))
			continue;

		pend_reg = EXYNOS_TMU_REG_INTPEND(i);

		val_irq = readl(data->base + pend_reg);
		writel(val_irq, data->base + pend_reg);
	}
}

static irqreturn_t exynos_tmu_irq(int irq, void *id)
{
	struct exynos_tmu_data *data = id;

	disable_irq_nosync(irq);
	schedule_work(&data->irq_work);

	return IRQ_HANDLED;
}

#ifndef CONFIG_EXYNOS_ACPM_THERMAL
static int exynos_pm_notifier(struct notifier_block *notifier,
			unsigned long event, void *v)
{
	struct exynos_tmu_data *devnode;
	struct thermal_cooling_device *cdev = NULL;
	struct thermal_zone_device *tz;
	struct thermal_instance *instance;

	switch (event) {
	case PM_SUSPEND_PREPARE:
		mutex_lock(&thermal_suspend_lock);
		suspended = true;
		list_for_each_entry(devnode, &dtm_dev_list, node) {
			tz = devnode->tzd;
			list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
				if (instance->cdev) {
					cdev = instance->cdev;
					break;
				}
			}

			if (cdev && cdev->ops->set_cur_temp && devnode->id != 1)
				cdev->ops->set_cur_temp(cdev, suspended, 0);
		}
		mutex_unlock(&thermal_suspend_lock);
		break;
	case PM_POST_SUSPEND:
		mutex_lock(&thermal_suspend_lock);
		suspended = false;
		list_for_each_entry(devnode, &dtm_dev_list, node) {
			cdev = devnode->cool_dev;
			tz = devnode->tzd;
			list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
				if (instance->cdev) {
					cdev = instance->cdev;
					break;
				}
			}

			if (cdev && cdev->ops->set_cur_temp && devnode->id != 1)
				cdev->ops->set_cur_temp(cdev, suspended, devnode->tzd->temperature / 1000);
		}
		mutex_unlock(&thermal_suspend_lock);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_tmu_pm_notifier = {
	.notifier_call = exynos_pm_notifier,
};
#endif

static int exynos_tmu_boost_callback(unsigned int cpu)
{
	struct thermal_zone_device *tz;
	struct exynos_tmu_data *devnode;

	list_for_each_entry(devnode, &dtm_dev_list, node) {
		if (devnode->id == 0) {
			tz = devnode->tzd;
			thermal_zone_device_update(tz, THERMAL_DEVICE_POWER_CAPABILITY_CHANGED);
			break;
		}
	}

	return 0;
}

static const struct of_device_id exynos_tmu_match[] = {
	{ .compatible = "samsung,exynos9810-tmu", },
	{ .compatible = "samsung,exynos9820-tmu", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, exynos_tmu_match);

static int exynos_of_get_soc_type(struct device_node *np)
{
	if (of_device_is_compatible(np, "samsung,exynos9810-tmu"))
		return SOC_ARCH_EXYNOS9810;
	if (of_device_is_compatible(np, "samsung,exynos9820-tmu"))
		return SOC_ARCH_EXYNOS9820;

	return -EINVAL;
}

static int exynos_of_sensor_conf(struct device_node *np,
				 struct exynos_tmu_platform_data *pdata)
{
	u32 value;
	int ret;

	of_node_get(np);

	ret = of_property_read_u32(np, "samsung,tmu_gain", &value);
	pdata->gain = (u8)value;
	of_property_read_u32(np, "samsung,tmu_reference_voltage", &value);
	pdata->reference_voltage = (u8)value;
	of_property_read_u32(np, "samsung,tmu_noise_cancel_mode", &value);
	pdata->noise_cancel_mode = (u8)value;

	of_property_read_u32(np, "samsung,tmu_efuse_value",
			     &pdata->efuse_value);

	of_property_read_u32(np, "samsung,tmu_first_point_trim", &value);
	pdata->first_point_trim = (u8)value;
	of_property_read_u32(np, "samsung,tmu_second_point_trim", &value);
	pdata->second_point_trim = (u8)value;
	of_property_read_u32(np, "samsung,tmu_default_temp_offset", &value);
	pdata->default_temp_offset = (u8)value;

	of_property_read_u32(np, "samsung,tmu_default_trip_temp", &pdata->trip_temp);
	of_property_read_u32(np, "samsung,tmu_cal_type", &pdata->cal_type);

	if (of_property_read_u32(np, "samsung,tmu_sensor_type", &pdata->sensor_type))
	        pr_err("%s: failed to get thermel sensor type\n", __func__);

	of_node_put(np);
	return 0;
}

static int exynos_map_dt_data(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	struct exynos_tmu_platform_data *pdata;
	struct resource res;
	unsigned int i;
	const char *temp, *tmu_name;

	if (!data || !pdev->dev.of_node)
		return -ENODEV;

	data->np = pdev->dev.of_node;

	if (of_property_read_u32(pdev->dev.of_node, "id", &data->id)) {
		dev_err(&pdev->dev, "failed to get TMU ID\n");
		return -ENODEV;
	}

	data->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (data->irq <= 0) {
		dev_err(&pdev->dev, "failed to get IRQ\n");
		return -ENODEV;
	}

	if (of_address_to_resource(pdev->dev.of_node, 0, &res)) {
		dev_err(&pdev->dev, "failed to get Resource 0\n");
		return -ENODEV;
	}

	data->base = devm_ioremap(&pdev->dev, res.start, resource_size(&res));
	if (!data->base) {
		dev_err(&pdev->dev, "Failed to ioremap memory\n");
		return -EADDRNOTAVAIL;
	}

	/* If remote sensor is exist, parse it. Remote sensor is used when reading the temperature. */
	if (!of_property_read_u32(pdev->dev.of_node, "sensors", &data->sensors)) {
		for (i = 0; i < TOTAL_SENSORS; i++)
			if (data->sensors & (1 << i))
				data->num_of_sensors++;

		data->sensor_info = kzalloc(sizeof(struct sensor_info) * data->num_of_sensors, GFP_KERNEL);
	} else {
		dev_err(&pdev->dev, "failed to get sensors information \n");
		return -ENODEV;
	}

	if (of_property_read_string(pdev->dev.of_node, "tmu_name", &tmu_name)) {
		dev_err(&pdev->dev, "failed to get tmu_name\n");
	} else
		strncpy(data->tmu_name, tmu_name, THERMAL_NAME_LENGTH);

	if (of_property_read_string(pdev->dev.of_node, "sensing_mode", &temp))
	        dev_err(&pdev->dev, "failed to get sensing_mode of thermel sensor\n");
	else {
		for (i = 0; i<ARRAY_SIZE(sensing_method); i++)
			if (!strcasecmp(temp, sensing_method[i]))
				data->sensing_mode = (int)i;
	}

	data->balance_offset = DEFAULT_BALANCE_OFFSET;

	data->hotplug_enable = of_property_read_bool(pdev->dev.of_node, "hotplug_enable");
	if (data->hotplug_enable) {
		dev_info(&pdev->dev, "thermal zone use hotplug function \n");
		of_property_read_u32(pdev->dev.of_node, "hotplug_in_threshold",
					&data->hotplug_in_threshold);
		if (!data->hotplug_in_threshold)
			dev_err(&pdev->dev, "No input hotplug_in_threshold \n");

		of_property_read_u32(pdev->dev.of_node, "hotplug_out_threshold",
					&data->hotplug_out_threshold);
		if (!data->hotplug_out_threshold)
			dev_err(&pdev->dev, "No input hotplug_out_threshold \n");
	}

	pdata = devm_kzalloc(&pdev->dev, sizeof(struct exynos_tmu_platform_data), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	exynos_of_sensor_conf(pdev->dev.of_node, pdata);
	data->pdata = pdata;
	data->soc = exynos_of_get_soc_type(pdev->dev.of_node);

	switch (data->soc) {
	case SOC_ARCH_EXYNOS9810:
	case SOC_ARCH_EXYNOS9820:
		data->tmu_initialize = exynos98X0_tmu_initialize;
		data->tmu_control = exynos98X0_tmu_control;
		data->tmu_read = exynos98X0_tmu_read;
		data->tmu_set_emulation = exynos98X0_tmu_set_emulation;
		data->tmu_clear_irqs = exynos98X0_tmu_clear_irqs;
		data->ntrip = 8;
		break;
	default:
		dev_err(&pdev->dev, "Platform not supported\n");
		return -EINVAL;
	}

	return 0;
}

static int exynos_throttle_cpu_hotplug(void *p, int temp)
{
	struct exynos_tmu_data *data = p;
	int ret = 0;
	struct cpumask mask;

	temp = temp / MCELSIUS;

	if (is_cpu_hotplugged_out) {
		if (temp < data->hotplug_in_threshold) {
			/*
			 * If current temperature is lower than low threshold,
			 * call cluster1_cores_hotplug(false) for hotplugged out cpus.
			 */
			exynos_cpuhp_request("DTM", *cpu_possible_mask, 0);
			is_cpu_hotplugged_out = false;
		}
	} else {
		if (temp >= data->hotplug_out_threshold) {
			/*
			 * If current temperature is higher than high threshold,
			 * call cluster1_cores_hotplug(true) to hold temperature down.
			 */
			is_cpu_hotplugged_out = true;
			cpumask_and(&mask, cpu_possible_mask, cpu_coregroup_mask(0));
			exynos_cpuhp_request("DTM", mask, 0);
		}
	}

	return ret;
}

static const struct thermal_zone_of_device_ops exynos_hotplug_sensor_ops = {
	.get_temp = exynos_get_temp,
	.set_emul_temp = exynos_tmu_set_emulation,
	.throttle_cpu_hotplug = exynos_throttle_cpu_hotplug,
};

static const struct thermal_zone_of_device_ops exynos_sensor_ops = {
	.get_temp = exynos_get_temp,
	.set_emul_temp = exynos_tmu_set_emulation,
	.get_trend = exynos_get_trend,
};

static ssize_t
balance_offset_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->balance_offset);
}

static ssize_t
balance_offset_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int balance_offset = 0;

	mutex_lock(&data->lock);

	if (kstrtos32(buf, 10, &balance_offset)) {
		mutex_unlock(&data->lock);
		return -EINVAL;
	}

	data->balance_offset = balance_offset;

	mutex_unlock(&data->lock);

	return count;
}

static ssize_t
all_temp_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int i, len = 0;
	u32 reg_offset, bit_offset;
	u32 temp_code, temp_cel;

	for (i = 0; i < data->num_of_sensors; i++) {
#if defined(CONFIG_SOC_EXYNOS9810)
		if (data->sensor_info[i].sensor_num < 2) {
			reg_offset = 0;
			bit_offset = EXYNOS_TMU_TEMP_SHIFT * data->sensor_info[i].sensor_num;
		} else {
			reg_offset = ((data->sensor_info[i].sensor_num - 2) / 3 + 1) * 4;
			bit_offset = EXYNOS_TMU_TEMP_SHIFT * ((data->sensor_info[i].sensor_num - 2) % 3);
		}
#elif defined(CONFIG_SOC_EXYNOS9820)
		reg_offset = (data->sensor_info[i].sensor_num / 2) * 4;
		bit_offset = (data->sensor_info[i].sensor_num % 2) * EXYNOS_TMU_TEMP_SHIFT;
#endif
		temp_code = (readl(data->base + EXYNOS_TMU_REG_CURRENT_TEMP1_0 + reg_offset)
				>> bit_offset) & EXYNOS_TMU_TEMP_MASK;
		temp_cel = code_to_temp_with_sensorinfo(data, temp_code, &data->sensor_info[i]);

		len += snprintf(&buf[len], PAGE_SIZE, "%u, ", temp_cel);
	}

	len += snprintf(&buf[len], PAGE_SIZE, "\n");

	return len;
}

static ssize_t
hotplug_out_temp_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->hotplug_out_threshold);
}

static ssize_t
hotplug_out_temp_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int hotplug_out = 0;

	mutex_lock(&data->lock);

	if (kstrtos32(buf, 10, &hotplug_out)) {
		mutex_unlock(&data->lock);
		return -EINVAL;
	}

	data->hotplug_out_threshold = hotplug_out;

	mutex_unlock(&data->lock);

	return count;
}

static ssize_t
hotplug_in_temp_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->hotplug_in_threshold);
}

static ssize_t
hotplug_in_temp_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int hotplug_in = 0;

	mutex_lock(&data->lock);

	if (kstrtos32(buf, 10, &hotplug_in)) {
		mutex_unlock(&data->lock);
		return -EINVAL;
	}

	data->hotplug_in_threshold = hotplug_in;

	mutex_unlock(&data->lock);

	return count;
}

static DEVICE_ATTR(balance_offset, S_IWUSR | S_IRUGO, balance_offset_show,
		balance_offset_store);

static DEVICE_ATTR(all_temp, S_IRUGO, all_temp_show, NULL);

static DEVICE_ATTR(hotplug_out_temp, S_IWUSR | S_IRUGO, hotplug_out_temp_show,
		hotplug_out_temp_store);

static DEVICE_ATTR(hotplug_in_temp, S_IWUSR | S_IRUGO, hotplug_in_temp_show,
		hotplug_in_temp_store);

static struct attribute *exynos_tmu_attrs[] = {
	&dev_attr_balance_offset.attr,
	&dev_attr_all_temp.attr,
	&dev_attr_hotplug_out_temp.attr,
	&dev_attr_hotplug_in_temp.attr,
	NULL,
};

static const struct attribute_group exynos_tmu_attr_group = {
	.attrs = exynos_tmu_attrs,
};

#define PARAM_NAME_LENGTH	25
#define FRAC_BITS 10	/* FRAC_BITS should be same with power_allocator */

#if defined(CONFIG_ECT)
static int exynos_tmu_ect_get_param(struct ect_pidtm_block *pidtm_block, char *name)
{
	int i;
	int param_value = -1;

	for (i = 0; i < pidtm_block->num_of_parameter; i++) {
		if (!strncasecmp(pidtm_block->param_name_list[i], name, PARAM_NAME_LENGTH)) {
			param_value = pidtm_block->param_value_list[i];
			break;
		}
	}

	return param_value;
}

static int exynos_tmu_parse_ect(struct exynos_tmu_data *data)
{
	struct thermal_zone_device *tz = data->tzd;
	struct __thermal_zone *__tz;

	if (!tz)
		return -EINVAL;

	__tz = (struct __thermal_zone *)tz->devdata;

	if (strncasecmp(tz->tzp->governor_name, "power_allocator",
						THERMAL_NAME_LENGTH)) {
		/* if governor is not power_allocator */
		void *thermal_block;
		struct ect_ap_thermal_function *function;
		int i, temperature;
		int hotplug_threshold_temp = 0, hotplug_flag = 0;
		unsigned int freq;

		thermal_block = ect_get_block(BLOCK_AP_THERMAL);
		if (thermal_block == NULL) {
			pr_err("Failed to get thermal block");
			return -EINVAL;
		}

		pr_info("%s %d thermal zone_name = %s\n", __func__, __LINE__, tz->type);

		function = ect_ap_thermal_get_function(thermal_block, tz->type);
		if (function == NULL) {
			pr_err("Failed to get thermal block %s", tz->type);
			return -EINVAL;
		}

		__tz->ntrips = __tz->num_tbps = function->num_of_range;
		pr_info("Trip count parsed from ECT : %d, zone : %s", function->num_of_range, tz->type);

		for (i = 0; i < function->num_of_range; ++i) {
			temperature = function->range_list[i].lower_bound_temperature;
			freq = function->range_list[i].max_frequency;
			__tz->trips[i].temperature = temperature  * MCELSIUS;
			__tz->tbps[i].value = freq;

			pr_info("Parsed From ECT : [%d] Temperature : %d, frequency : %u\n",
					i, temperature, freq);

			if (function->range_list[i].flag != hotplug_flag) {
				if (function->range_list[i].flag != hotplug_flag) {
					hotplug_threshold_temp = temperature;
					hotplug_flag = function->range_list[i].flag;
					data->hotplug_out_threshold = temperature;

					if (i)
						data->hotplug_in_threshold = function->range_list[i-1].lower_bound_temperature;

					pr_info("[ECT]hotplug_threshold : %d\n", hotplug_threshold_temp);
					pr_info("[ECT]hotplug_in_threshold : %d\n", data->hotplug_in_threshold);
					pr_info("[ECT]hotplug_out_threshold : %d\n", data->hotplug_out_threshold);
				}
			}

			if (hotplug_threshold_temp != 0)
				data->hotplug_enable = true;
			else
				data->hotplug_enable = false;

		}
	} else {
		void *block;
		struct ect_pidtm_block *pidtm_block;
		int i, temperature, value;
		int hotplug_out_threshold = 0, hotplug_in_threshold = 0, limited_frequency = 0;

		block = ect_get_block(BLOCK_PIDTM);
		if (block == NULL) {
			pr_err("Failed to get PIDTM block");
			return -EINVAL;
		}

		pr_info("%s %d thermal zone_name = %s\n", __func__, __LINE__, tz->type);

		pidtm_block = ect_pidtm_get_block(block, tz->type);
		if (pidtm_block == NULL) {
			pr_err("Failed to get PIDTM block %s", tz->type);
			return -EINVAL;
		}

		__tz->ntrips = pidtm_block->num_of_temperature;

		for (i = 0; i < pidtm_block->num_of_temperature; ++i) {
			temperature = pidtm_block->temperature_list[i];
			__tz->trips[i].temperature = temperature  * MCELSIUS;

			pr_info("Parsed From ECT : [%d] Temperature : %d\n", i, temperature);
		}

		if ((value = exynos_tmu_ect_get_param(pidtm_block, "k_po")) != -1) {
			pr_info("Parse from ECT k_po: %d\n", value);
			tz->tzp->k_po = value << FRAC_BITS;
		} else
			pr_err("Fail to parse k_po parameter\n");

		if ((value = exynos_tmu_ect_get_param(pidtm_block, "k_pu")) != -1) {
			pr_info("Parse from ECT k_pu: %d\n", value);
			tz->tzp->k_pu = value << FRAC_BITS;
		} else
			pr_err("Fail to parse k_pu parameter\n");

		if ((value = exynos_tmu_ect_get_param(pidtm_block, "k_i")) != -1) {
			pr_info("Parse from ECT k_i: %d\n", value);
			tz->tzp->k_i = value << FRAC_BITS;
		} else
			pr_err("Fail to parse k_i parameter\n");

		if ((value = exynos_tmu_ect_get_param(pidtm_block, "i_max")) != -1) {
			pr_info("Parse from ECT i_max: %d\n", value);
			tz->tzp->integral_max = value;
		} else
			pr_err("Fail to parse i_max parameter\n");

		if ((value = exynos_tmu_ect_get_param(pidtm_block, "integral_cutoff")) != -1) {
			pr_info("Parse from ECT integral_cutoff: %d\n", value);
			tz->tzp->integral_cutoff = value;
		} else
			pr_err("Fail to parse integral_cutoff parameter\n");

		if ((value = exynos_tmu_ect_get_param(pidtm_block, "p_control_t")) != -1) {
			pr_info("Parse from ECT p_control_t: %d\n", value);
			tz->tzp->sustainable_power = value;
		} else
			pr_err("Fail to parse p_control_t parameter\n");

		if ((value = exynos_tmu_ect_get_param(pidtm_block, "hotplug_out_threshold")) != -1) {
			pr_info("Parse from ECT hotplug_out_threshold: %d\n", value);
			hotplug_out_threshold = value;
		}

		if ((value = exynos_tmu_ect_get_param(pidtm_block, "hotplug_in_threshold")) != -1) {
			pr_info("Parse from ECT hotplug_in_threshold: %d\n", value);
			hotplug_in_threshold = value;
		}

		if ((value = exynos_tmu_ect_get_param(pidtm_block, "limited_frequency")) != -1) {
			pr_info("Parse from ECT limited_frequency: %d\n", value);
			limited_frequency = value;
		}

		if (hotplug_out_threshold != 0 && hotplug_in_threshold != 0) {
			data->hotplug_out_threshold = hotplug_out_threshold;
			data->hotplug_in_threshold = hotplug_in_threshold;
			data->limited_frequency = limited_frequency;
			data->hotplug_enable = true;
		} else
			data->hotplug_enable = false;
	}
	return 0;
};
#endif

#ifdef CONFIG_MALI_DEBUG_KERNEL_SYSFS
struct exynos_tmu_data *gpu_thermal_data;
#endif

static int exynos_tmu_probe(struct platform_device *pdev)
{
	struct exynos_tmu_data *data;
	unsigned int ctrl;
	int ret;

	data = devm_kzalloc(&pdev->dev, sizeof(struct exynos_tmu_data),
					GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	platform_set_drvdata(pdev, data);
	mutex_init(&data->lock);

	ret = exynos_map_dt_data(pdev);
	if (ret)
		goto err_sensor;

	INIT_WORK(&data->irq_work, exynos_tmu_work);

	/*
	 * data->tzd must be registered before calling exynos_tmu_initialize(),
	 * requesting irq and calling exynos_tmu_control().
	 */
	if(data->hotplug_enable) {
		exynos_cpuhp_register("DTM", *cpu_online_mask, 0);
#if defined(CONFIG_SOC_EXYNOS9810)
		pm_qos_add_request(&thermal_cpu_limit_request,
					PM_QOS_CLUSTER1_FREQ_MAX,
					PM_QOS_CPU_FREQ_MAX_DEFAULT_VALUE);
#endif
	}

	data->tzd = thermal_zone_of_sensor_register(&pdev->dev, 0, data,
						    data->hotplug_enable ?
						    &exynos_hotplug_sensor_ops :
						    &exynos_sensor_ops);
	if (IS_ERR(data->tzd)) {
		ret = PTR_ERR(data->tzd);
		dev_err(&pdev->dev, "Failed to register sensor: %d\n", ret);
		goto err_sensor;
	}

#if defined(CONFIG_ECT)
	exynos_tmu_parse_ect(data);
#endif

	data->num_probe = (readl(data->base + EXYNOS_TMU_REG_CONTROL1) >> EXYNOS_TMU_NUM_PROBE_SHIFT)
				& EXYNOS_TMU_NUM_PROBE_MASK;

	if (data->id == 0) {
		ctrl = readl(data->base + EXYNOS_TMU_REG_TRIMINFO(3));
		t_bgri_trim = (ctrl >> EXYNOS_TMU_T_TRIM0_SHIFT) & EXYNOS_TMU_T_TRIM0_MASK;
		ctrl = readl(data->base + EXYNOS_TMU_REG_TRIMINFO(4));
		t_vref_trim = (ctrl >> EXYNOS_TMU_T_TRIM0_SHIFT) & EXYNOS_TMU_T_TRIM0_MASK;
		ctrl = readl(data->base + EXYNOS_TMU_REG_TRIMINFO(5));
		t_vbei_trim = (ctrl >> EXYNOS_TMU_T_TRIM0_SHIFT) & EXYNOS_TMU_T_TRIM0_MASK;
	}

	ret = exynos_tmu_initialize(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to initialize TMU\n");
		goto err_thermal;
	}

	ret = devm_request_irq(&pdev->dev, data->irq, exynos_tmu_irq,
				IRQF_SHARED | IRQF_GIC_MULTI_TARGET, dev_name(&pdev->dev), data);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq: %d\n", data->irq);
		goto err_thermal;
	}

	exynos_tmu_control(pdev, true);

	ret = sysfs_create_group(&pdev->dev.kobj, &exynos_tmu_attr_group);
	if (ret)
		dev_err(&pdev->dev, "cannot create exynos tmu attr group");

	mutex_lock(&data->lock);
	list_add_tail(&data->node, &dtm_dev_list);
	num_of_devices++;
	mutex_unlock(&data->lock);

	if (list_is_singular(&dtm_dev_list)) {
#ifdef CONFIG_EXYNOS_ACPM_THERMAL
		exynos_acpm_tmu_set_init(&cap);
#else
		register_pm_notifier(&exynos_tmu_pm_notifier);
#endif
		cpuhp_setup_state_nocalls(CPUHP_EXYNOS_BOOST_CTRL_POST, "dtm_boost_cb",
				exynos_tmu_boost_callback, exynos_tmu_boost_callback);
	}

	if (!IS_ERR(data->tzd))
		data->tzd->ops->set_mode(data->tzd, THERMAL_DEVICE_ENABLED);

#ifdef CONFIG_MALI_DEBUG_KERNEL_SYSFS
	if (data->id == EXYNOS_GPU_THERMAL_ZONE_ID)
		gpu_thermal_data = data;
#endif

	return 0;

err_thermal:
	thermal_zone_of_sensor_unregister(&pdev->dev, data->tzd);
err_sensor:
	return ret;
}

static int exynos_tmu_remove(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	struct thermal_zone_device *tzd = data->tzd;
	struct exynos_tmu_data *devnode;

#ifndef CONFIG_EXYNOS_ACPM_THERMAL
	if (list_is_singular(&dtm_dev_list))
		unregister_pm_notifier(&exynos_tmu_pm_notifier);
#endif

	thermal_zone_of_sensor_unregister(&pdev->dev, tzd);
	exynos_tmu_control(pdev, false);

	mutex_lock(&data->lock);
	list_for_each_entry(devnode, &dtm_dev_list, node) {
		if (devnode->id == data->id) {
			list_del(&devnode->node);
			num_of_devices--;
			break;
		}
	}
	mutex_unlock(&data->lock);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int exynos_tmu_suspend(struct device *dev)
{
#ifdef CONFIG_EXYNOS_ACPM_THERMAL
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int cp_state;

	suspended_count++;
	disable_irq(data->irq);

	cp_call_mode = is_aud_on() && cap.acpm_irq;
	if (cp_call_mode) {
		if (suspended_count == num_of_devices) {
			exynos_acpm_tmu_set_cp_call();
			pr_info("%s: TMU suspend w/ AUD-on\n", __func__);
		}
	} else {
		exynos_tmu_control(pdev, false);
		if (suspended_count == num_of_devices) {
			cp_state = is_cp_net_conn();
			exynos_acpm_tmu_set_suspend(cp_state);
			pr_info("%s: TMU suspend w/ cp_state %d\n", __func__, cp_state);
		}
	}
#else
	exynos_tmu_control(to_platform_device(dev), false);
#endif

	return 0;
}

static int exynos_tmu_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
#ifdef CONFIG_EXYNOS_ACPM_THERMAL
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int temp, stat;

	if (suspended_count == num_of_devices)
		exynos_acpm_tmu_set_resume();

	if (!cp_call_mode) {
		exynos_tmu_initialize(pdev);
		exynos_tmu_control(pdev, true);
	}

	exynos_acpm_tmu_set_read_temp(data->tzd->id, &temp, &stat);

	pr_info("%s: thermal zone %d temp %d stat %d\n",
			__func__, data->tzd->id, temp, stat);

	enable_irq(data->irq);
	suspended_count--;

	if (!suspended_count)
		pr_info("%s: TMU resume complete\n", __func__);
#else
	exynos_tmu_initialize(pdev);
	exynos_tmu_control(pdev, true);
#endif

	return 0;
}

static SIMPLE_DEV_PM_OPS(exynos_tmu_pm,
			 exynos_tmu_suspend, exynos_tmu_resume);
#define EXYNOS_TMU_PM	(&exynos_tmu_pm)
#else
#define EXYNOS_TMU_PM	NULL
#endif

static struct platform_driver exynos_tmu_driver = {
	.driver = {
		.name   = "exynos-tmu",
		.pm     = EXYNOS_TMU_PM,
		.of_match_table = exynos_tmu_match,
		.suppress_bind_attrs = true,
	},
	.probe = exynos_tmu_probe,
	.remove	= exynos_tmu_remove,
};

module_platform_driver(exynos_tmu_driver);

#ifdef CONFIG_EXYNOS_ACPM_THERMAL
static void exynos_acpm_tmu_test_cp_call(bool mode)
{
	struct exynos_tmu_data *devnode;

	if (mode) {
		list_for_each_entry(devnode, &dtm_dev_list, node) {
			disable_irq(devnode->irq);
		}
		exynos_acpm_tmu_set_cp_call();
	} else {
		exynos_acpm_tmu_set_resume();
		list_for_each_entry(devnode, &dtm_dev_list, node) {
			enable_irq(devnode->irq);
		}
	}
}

static int emul_call_get(void *data, unsigned long long *val)
{
	*val = exynos_acpm_tmu_is_test_mode();

	return 0;
}

static int emul_call_set(void *data, unsigned long long val)
{
	int status = exynos_acpm_tmu_is_test_mode();

	if ((val == 0 || val == 1) && (val != status)) {
		exynos_acpm_tmu_set_test_mode(val);
		exynos_acpm_tmu_test_cp_call(val);
	}

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(emul_call_fops, emul_call_get, emul_call_set, "%llu\n");

static int log_print_set(void *data, unsigned long long val)
{
	if (val == 0 || val == 1)
		exynos_acpm_tmu_log(val);

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(log_print_fops, NULL, log_print_set, "%llu\n");

static ssize_t ipc_dump1_read(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	union {
		unsigned int dump[2];
		unsigned char val[8];
	} data;
	char buf[48];
	ssize_t ret;

	exynos_acpm_tmu_ipc_dump(0, data.dump);

	ret = snprintf(buf, sizeof(buf), "%3d %3d %3d %3d %3d %3d %3d\n",
			data.val[1], data.val[2], data.val[3],
			data.val[4], data.val[5], data.val[6], data.val[7]);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

static ssize_t ipc_dump2_read(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	union {
		unsigned int dump[2];
		unsigned char val[8];
	} data;
	char buf[48];
	ssize_t ret;

	exynos_acpm_tmu_ipc_dump(EXYNOS_GPU_THERMAL_ZONE_ID, data.dump);

	ret = snprintf(buf, sizeof(buf), "%3d %3d %3d %3d %3d %3d %3d\n",
			data.val[1], data.val[2], data.val[3],
			data.val[4], data.val[5], data.val[6], data.val[7]);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(user_buf, count, ppos, buf, ret);

}

static const struct file_operations ipc_dump1_fops = {
	.open = simple_open,
	.read = ipc_dump1_read,
	.llseek = default_llseek,
};

static const struct file_operations ipc_dump2_fops = {
	.open = simple_open,
	.read = ipc_dump2_read,
	.llseek = default_llseek,
};

#endif

static struct dentry *debugfs_root;

static int exynos_thermal_create_debugfs(void)
{
	debugfs_root = debugfs_create_dir("exynos-thermal", NULL);
	if (!debugfs_root) {
		pr_err("Failed to create exynos thermal debugfs\n");
		return 0;
	}

#ifdef CONFIG_EXYNOS_ACPM_THERMAL
	debugfs_create_file("emul_call", 0644, debugfs_root, NULL, &emul_call_fops);
	debugfs_create_file("log_print", 0644, debugfs_root, NULL, &log_print_fops);
	debugfs_create_file("ipc_dump1", 0644, debugfs_root, NULL, &ipc_dump1_fops);
	debugfs_create_file("ipc_dump2", 0644, debugfs_root, NULL, &ipc_dump2_fops);
#endif
	return 0;
}
arch_initcall(exynos_thermal_create_debugfs);

#ifdef CONFIG_SEC_PM

#define NR_THERMAL_SENSOR_MAX	10

static ssize_t exynos_tmu_curr_temp(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct exynos_tmu_data *data;
	int temp[NR_THERMAL_SENSOR_MAX] = {0, };
	int i, id_max = 0;
	ssize_t ret = 0;

	list_for_each_entry(data, &dtm_dev_list, node) {
		if (data->id < NR_THERMAL_SENSOR_MAX) {
			exynos_get_temp(data, &temp[data->id]);
			temp[data->id] /= 1000;

			if (id_max < data->id)
				id_max = data->id;
		} else {
			pr_err("%s: id:%d %s\n", __func__, data->id,
					data->tmu_name);
			continue;
		}
	}

	for (i = 0; i <= id_max; i++)
		ret += sprintf(buf + ret, "%d,", temp[i]);

	sprintf(buf + ret - 1, "\n");

	return ret;
}

static DEVICE_ATTR(curr_temp, 0444, exynos_tmu_curr_temp, NULL);

static struct attribute *exynos_tmu_sec_pm_attributes[] = {
	&dev_attr_curr_temp.attr,
	NULL
};

static const struct attribute_group exynos_tmu_sec_pm_attr_grp = {
	.attrs = exynos_tmu_sec_pm_attributes,
};

static int __init exynos_tmu_sec_pm_init(void)
{
	int ret = 0;
	struct device *dev;

	dev = sec_device_create(NULL, "exynos_tmu");

	if (IS_ERR(dev)) {
		pr_err("%s: failed to create device\n", __func__);
		return PTR_ERR(dev);
	}

	ret = sysfs_create_group(&dev->kobj, &exynos_tmu_sec_pm_attr_grp);
	if (ret) {
		pr_err("%s: failed to create sysfs group(%d)\n", __func__, ret);
		goto err_create_sysfs;
	}

	return ret;

err_create_sysfs:
	sec_device_destroy(dev->devt);

	return ret;
}

late_initcall(exynos_tmu_sec_pm_init);
#endif /* CONFIG_SEC_PM */

MODULE_DESCRIPTION("EXYNOS TMU Driver");
MODULE_AUTHOR("Donggeun Kim <dg77.kim@samsung.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:exynos-tmu");
