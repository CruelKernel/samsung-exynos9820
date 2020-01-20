#include <linux/kernel.h>
#include <crypto/fmp.h>
#include "fmp_fips_integrity.h"

__attribute__ ((section(".rodata")))
const __s8 builtime_fmp_hmac[FIPS_HMAC_SIZE] = {0};

__attribute__ ((section(".rodata")))
const struct first_last integrity_fmp_addrs[FIPS_FMP_ADDRS_SIZE] = {{0}, };

__attribute__ ((section(".rodata")))
const __u64 fmp_buildtime_address = 10;
