/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Device Tree binding constants for Exynos9810 devfreq
 */

#ifndef _DT_BINDINGS_EXYNOS_9810_DEVFREQ_H
#define _DT_BINDINGS_EXYNOS_9810_DEVFREQ_H
/* DEVFREQ TYPE LIST */
#define DEVFREQ_MIF		0
#define DEVFREQ_INT		1
#define DEVFREQ_DISP		2
#define DEVFREQ_CAM		3
#define DEVFREQ_INTCAM		4
#define DEVFREQ_AUD		5
#define DEVFREQ_IVA		6
#define DEVFREQ_SCORE		7
#define DEVFREQ_FSYS0		8
#define DEVFREQ_TYPE_END	9

/* ESS FLAG LIST */
#define ESS_FLAG_INT	2
#define ESS_FLAG_MIF	3
#define ESS_FLAG_ISP	4
#define ESS_FLAG_DISP	5
#define ESS_FLAG_INTCAM	6
#define ESS_FLAG_AUD	7
#define ESS_FLAG_IVA	8
#define ESS_FLAG_SCORE	9
#define ESS_FLAG_FSYS0	10

/* DEVFREQ GOV TYPE */
#define SIMPLE_INTERACTIVE 0

#endif
