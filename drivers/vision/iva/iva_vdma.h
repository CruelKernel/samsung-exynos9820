/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_VDMA_H__
#define __IVA_VDMA_H__

#include "iva_ctrl.h"

#if defined(CONFIG_SOC_EXYNOS8895)
#include "iva_vdma10.h"
#elif defined(CONFIG_SOC_EXYNOS9810)
#include "iva_vdma20.h"
#elif defined(CONFIG_SOC_EXYNOS9820)
#include "iva_vdma30.h"
#endif

#endif /* __IVA_VDMA_H__ */
