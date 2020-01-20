/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Device Tree binding constants for Exynos9810
*/

#ifndef _DT_BINDINGS_EXYNOS_9820_H
#define _DT_BINDINGS_EXYNOS_9820_H

/* NUMBER FOR DVFS MANAGER */
#define DM_CPU_CL0	0
#define DM_CPU_CL1	1
#define DM_CPU_CL2	2
#define DM_MIF		3
#define DM_INT		4
#define DM_INTCAM	5
#define DM_CAM		6
#define DM_IVA		7
#define DM_SCORE	8
#define DM_DISP		9
#define DM_AUD		10
#define DM_MFC		11
#define DM_NPU		12
#define DM_GPU		13

/* CONSTRAINT TYPE */
#define CONSTRAINT_MIN	0
#define CONSTRAINT_MAX	1

#endif	/* _DT_BINDINGS_EXYNOS_9820_H */
