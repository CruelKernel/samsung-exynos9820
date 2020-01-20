/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Device Tree binding constants for Exynos9810 devfreq
 */

#ifndef _DT_BINDINGS_EXYNOS_9820_DEVFREQ_H
#define _DT_BINDINGS_EXYNOS_9820_DEVFREQ_H
/* DEVFREQ TYPE LIST */
#define DEVFREQ_MIF		0
#define DEVFREQ_INT		1
#define DEVFREQ_DISP		2
#define DEVFREQ_CAM		3
#define DEVFREQ_INTCAM		4
#define DEVFREQ_AUD		5
#define DEVFREQ_IVA		6
#define DEVFREQ_SCORE		7
#define DEVFREQ_MFC		8
#define DEVFREQ_NPU		9
#define DEVFREQ_TYPE_END	10

/* ESS FLAG LIST */
#define ESS_FLAG_INT	3
#define ESS_FLAG_MIF	4
#define ESS_FLAG_ISP	5
#define ESS_FLAG_DISP	6
#define ESS_FLAG_INTCAM	7
#define ESS_FLAG_AUD	8
#define ESS_FLAG_IVA	9
#define ESS_FLAG_SCORE	10
#define ESS_FLAG_MFC	12
#define ESS_FLAG_NPU	13

/* DEVFREQ GOV TYPE */
#define SIMPLE_INTERACTIVE 0

#endif
