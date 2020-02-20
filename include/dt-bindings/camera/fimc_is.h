/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DT_BINDINGS_CAMERA_FIMC_IS_H
#define _DT_BINDINGS_CAMERA_FIMC_IS_H

#define F1_0	100
#define F1_4	140
#define F1_5	150
#define F1_7	170
#define F1_8	180
#define F1_9	190
#define F2_0	200
#define F2_1	210
#define F2_2	220
#define F2_4	240
#define F2_45	245
#define F2_6	260
#define F2_7	270
#define F2_8	280
#define F4_0	400
#define F5_6	560
#define F8_0	800
#define F11_0	1100
#define F16_0	1600
#define F22_0	2200
#define F32_0	3200

#define FLITE_ID_NOTHING	100

#define GPIO_SCENARIO_ON			0
#define GPIO_SCENARIO_OFF			1
#define GPIO_SCENARIO_STANDBY_ON		2
#define GPIO_SCENARIO_STANDBY_OFF		3
#define GPIO_SCENARIO_STANDBY_OFF_SENSOR	4
#define GPIO_SCENARIO_STANDBY_OFF_PREPROCESSOR	5
#define GPIO_SCENARIO_SENSOR_RETENTION_ON	6
#define GPIO_SCENARIO_MAX			7
#define GPIO_CTRL_MAX				32

#define SENSOR_SCENARIO_NORMAL		0
#define SENSOR_SCENARIO_VISION		1
#define SENSOR_SCENARIO_EXTERNAL	2
#define SENSOR_SCENARIO_OIS_FACTORY	3
#define SENSOR_SCENARIO_READ_ROM	4
#define SENSOR_SCENARIO_STANDBY	5
#define SENSOR_SCENARIO_SECURE		6
#define SENSOR_SCENARIO_HW_INIT		7
#define SENSOR_SCENARIO_ADDITIONAL_POWER	8
#define SENSOR_SCENARIO_VIRTUAL	9
#define SENSOR_SCENARIO_MAX		10

#define PIN_NONE	0
#define PIN_OUTPUT	1
#define PIN_INPUT	2
#define PIN_RESET	3
#define PIN_FUNCTION	4
#define PIN_REGULATOR	5
#define PIN_I2C		6
#define PIN_MCLK	7

#define DT_SET_PIN(p, n, a, v, t) \
			seq@__LINE__ { \
				pin = #p; \
				pname = #n; \
				act = <a>; \
				value = <v>; \
				delay = <t>; \
				voltage = <0>; \
			}

#define DT_SET_PIN_VOLTAGE(p, n, a, v, t, e) \
			seq@__LINE__ { \
				pin = #p; \
				pname = #n; \
				act = <a>; \
				value = <v>; \
				delay = <t>; \
				voltage = <e>; \
			}


#define VC_NOTHING		0
#define VC_TAILPDAF		1
#define VC_MIPISTAT		2
#define VC_EMBEDDED		3
#define VC_PRIVATE		4

#define SP_REAR		0
#define SP_FRONT	1
#define SP_REAR2	2
#define SP_FRONT2	3
#define SP_REAR3	4
#define SP_FRONT3	5
#define SP_REAR4	6
#define SP_FRONT4	7
#define SP_REAR_TOF	8
#define SP_FRONT_TOF	9
#define SP_SECURE	100
#define SP_VIRTUAL	101

#endif
