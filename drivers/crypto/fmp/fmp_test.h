/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _FMP_TEST_H_
#define _FMP_TEST_H_

#define FMP_BLK_SIZE	(4096)

#define WRITE_MODE	1
#define READ_MODE	2

#define ENCRYPT		1
#define DECRYPT		2

struct fmp_test_data *fmp_test_init(struct exynos_fmp *fmp);
int fmp_cipher_run(struct exynos_fmp *fmp, struct fmp_test_data *fdata,
		uint8_t *data, uint32_t len, bool bypass, uint32_t write,
		void *priv, struct fmp_crypto_info *ci);
int fmp_test_crypt(struct exynos_fmp *fmp, struct fmp_test_data *fdata,
		uint8_t *src, uint8_t *dst, uint32_t len, uint32_t enc,
		void *priv, struct fmp_crypto_info *ci);
void fmp_test_exit(struct fmp_test_data *fdata);
#endif /* _FMP_TEST_H_ */
