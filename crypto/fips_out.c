#include "internal.h"

__attribute__ ((section(".rodata")))
const __s8 builtime_crypto_hmac[FIPS_HMAC_SIZE] = {0};

__attribute__ ((section(".rodata")))
const struct first_last integrity_crypto_addrs[FIPS_CRYPTO_ADDRS_SIZE] = {{0},};

__attribute__ ((section(".rodata")))
const __u64 crypto_buildtime_address = 10;
