/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __EXYNOS_ALT_H_
#define __EXYNOS_ALT_H_

#ifdef CONFIG_EXYNOS_ALT_DVFS
void exynos_alt_call_chain(void);
#else
#define exynos_alt_call_chain() do {} while(0)
#endif

#endif
