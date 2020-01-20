/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _FMP_FIPS_MAIN_H_
#define _FMP_FIPS_MAIN_H_

#if defined(CONFIG_EXYNOS_FMP_FIPS)
int exynos_fmp_fips_init(struct exynos_fmp *fmp);
void exynos_fmp_fips_exit(struct exynos_fmp *fmp);
bool in_fmp_fips_err(void);
bool in_fmp_fips_init(void);
#else
inline int exynos_fmp_fips_init(struct exynos_fmp *fmp)
{
	return 0;
}

inline void exynos_fmp_fips_exit(struct exynos_fmp *fmp)
{
}

inline bool in_fmp_fips_err(void)
{
	return 0;
}

inline bool in_fmp_fips_init(void)
{
	return 0;
}
#endif /* CONFIG_EXYNOS_FMP_FIPS */
#endif /* _FMP_FIPS_MAIN_H_ */
