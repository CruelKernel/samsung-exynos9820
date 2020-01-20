/*
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _FMP_FIPS_FUNC_TEST_H_
#define _FMP_FIPS_FUNC_TEST_H_

#if defined(CONFIG_EXYNOS_FMP_FIPS)
#if defined(CONFIG_EXYNOS_FMP_FIPS_FUNC_TEST)
#define FMP_FUNCTEST_KAT_CASE_NUM 6
#define FMP_FUNCTEST_NO_TEST "NO_TEST"

int exynos_fmp_func_test_KAT_case(struct platform_device *pdev,
				struct exynos_fmp *fmp);
#else
inline int exynos_fmp_func_test_KAT_case(struct platform_device *pdev,
				struct exynos_fmp *fmp)
{
	if (fmp)
		fmp->test_vops = NULL;
	return 0;
}
#endif /* CONFIG_EXYNOS_FMP_FIPS_FUNC_TEST */
#endif /* CONFIG_EXYNOS_FMP_FIPS */
#endif /* _FMP_FIPS_FUNC_TEST_H_ */
