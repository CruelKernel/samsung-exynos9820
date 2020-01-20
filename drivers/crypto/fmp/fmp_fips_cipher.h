/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _FMP_FIPS_CIPHER_H_
#define _FMP_FIPS_CIPHER_H_
int fmp_cipher_set_key(struct fmp_test_data *fdata,
		const u8 *key, uint32_t key_len);
int fmp_cipher_set_iv(struct fmp_test_data *fdata,
		const u8 *iv, uint32_t iv_len);
#endif
