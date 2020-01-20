/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Device Tree binding constants for Exynos9810
*/

#ifndef _DT_BINDINGS_EXYNOS_9810_H
#define _DT_BINDINGS_EXYNOS_9810_H

/* NUMBER FOR DVFS MANAGER */
#define DM_CPU_CL0	0
#define DM_CPU_CL1	1
#define DM_MIF		2
#define DM_INT		3
#define DM_INTCAM	4
#define DM_FSYS0	5
#define DM_CAM		6
#define DM_DISP		7
#define DM_AUD		8
#define DM_IVA		9
#define DM_SCORE	10
#define DM_GPU		11

/* CONSTRAINT TYPE */
#define CONSTRAINT_MIN	0
#define CONSTRAINT_MAX	1

#endif	/* _DT_BINDINGS_EXYNOS_9810_H */
