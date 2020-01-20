/*
 * Exynos FMP integrity header
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __FMP_INTEGRITY_H__
#define __FMP_INTEGRITY_H__

#include <linux/kernel.h>

#define FIPS_HMAC_SIZE         (32)
#define FIPS_FMP_ADDRS_SIZE    (100)

struct first_last {
	aligned_u64 first;
	aligned_u64 last;
};

extern const __u64 fmp_buildtime_address;
extern const struct first_last integrity_fmp_addrs[FIPS_FMP_ADDRS_SIZE];
extern const __s8 builtime_fmp_hmac[FIPS_HMAC_SIZE];

int do_fmp_integrity_check(struct exynos_fmp *fmp);

#endif
