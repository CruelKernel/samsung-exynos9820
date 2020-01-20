/*
 * Exynos FMP FIPS functional test
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/string.h>
#include <linux/printk.h>
#include <linux/device.h>

#include <crypto/fmp.h>

#include "fmp_fips_info.h"
#include "fmp_fips_main.h"
#include "fmp_fips_func_test.h"

static char *fips_functest_mode;

static char *fips_functest_KAT_list[] = {
	"aes-xts",
	"aes-cbc",
	"sha256",
	"hmac-sha256",
	"integrity",
	"zeroization"
};

void set_fmp_fips_functest_KAT_mode(const int num)
{
	if (num >= 0 && num < FMP_FUNCTEST_KAT_CASE_NUM)
		fips_functest_mode = fips_functest_KAT_list[num];
	else
		fips_functest_mode = FMP_FUNCTEST_NO_TEST;
}

char *get_fmp_fips_functest_mode(void)
{
	if (fips_functest_mode)
		return fips_functest_mode;
	else
		return FMP_FUNCTEST_NO_TEST;
}

static int fmp_func_test_integrity(HMAC_SHA256_CTX *desc,
				unsigned long *start_addr)
{
	int ret = 0;

	if (!strcmp("integrity", get_fmp_fips_functest_mode())) {
		pr_info("FIPS(%s): Failing Integrity Test\n", __func__);
		ret = hmac_sha256_update(desc,
				(unsigned char *)start_addr, 1);
	}

	return ret;
}

static int fmp_func_test_zeroization(struct fmp_table_setting *table,
					char *str)
{
	if (!table || !str) {
		pr_err("%s: invalid fmp table address or string\n", __func__);
		return -EINVAL;
	}

	if (!strcmp("zeroization", get_fmp_fips_functest_mode())) {
		pr_info("FIPS FMP descriptor zeroize (%s) ", str);
		print_hex_dump(KERN_ERR, "FIPS FMP descriptor zeroize: ",
			DUMP_PREFIX_NONE, 16, 1, &table->file_iv0,
			sizeof(__le32) * 20, false);
	}

	return 0;
}

static int fmp_func_test_hmac_sha256(char *digest, char *algorithm)
{
	char *str = get_fmp_fips_functest_mode();

	if (!digest || !algorithm) {
		pr_err("%s: invalid digest address or algorithm\n", __func__);
		return -EINVAL;
	}

	if (!strcmp(algorithm, str))
		digest[0] += 1;

	return 0;
}

static int fmp_func_test_aes(const int mode, char *key, unsigned char klen)
{
	char *str = get_fmp_fips_functest_mode();

	if (!key || (klen == 0)) {
		pr_err("%s: invalid fmp key or key length\n", __func__);
		return -EINVAL;
	}

	if ((!strcmp("aes-xts", str) && XTS_MODE == mode)
			|| (!strcmp("aes-cbc", str) && CBC_MODE == mode))
		key[0] += 1;

	return 0;
}

const struct exynos_fmp_fips_test_vops exynos_fmp_fips_test_ops = {
	.integrity	= fmp_func_test_integrity,
	.zeroization	= fmp_func_test_zeroization,
	.hmac_sha256	= fmp_func_test_hmac_sha256,
	.aes		= fmp_func_test_aes,
};

int exynos_fmp_func_test_KAT_case(struct platform_device *pdev,
				struct exynos_fmp *fmp)
{
	int i, ret;
	struct fmp_crypto_info data;
	struct fmp_request req;
	struct device *dev;

	if (!fmp || !fmp->dev) {
		pr_err("%s: invalid fmp context or device\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	dev = fmp->dev;
	fmp->test_vops = (struct exynos_fmp_fips_test_vops *)&exynos_fmp_fips_test_ops;

	for (i = 0; i < FMP_FUNCTEST_KAT_CASE_NUM; i++) {
		set_fmp_fips_functest_KAT_mode(i);
		dev_info(dev, "FIPS FUNC : -----------------------------------\n");
		dev_info(dev, "FIPS FUNC : Test case %d - [%s]\n",
				i + 1, get_fmp_fips_functest_mode());
		dev_info(dev, "FIPS FUNC : -----------------------------------\n");

		ret = exynos_fmp_fips_init(fmp);
		if (ret) {
			dev_err(dev, "FIPS FUNC : Fail to initialize fmp fips. ret(%d)",
					ret);
			exynos_fmp_fips_exit(fmp);
			goto out;
		}
		dev_info(dev, "FIPS FUNC : (%d-1) Selftest done. FMP FIPS status : %s\n",
				i + 1, in_fmp_fips_err() ? "FAILED" : "PASSED");

		memset(&data, 0, sizeof(struct fmp_crypto_info));
		memset(&req, 0, sizeof(struct fmp_request));
		dev_info(dev, "FIPS FUNC : (%d-2) Try to set config\n", i + 1);

		ret = exynos_fmp_crypt(&data, &req);
		if (ret)
			dev_info(dev,
				"FIPS FUNC : (%d-3) Fail FMP config as expected. ret(%d)\n",
				i + 1, ret);

		exynos_fmp_fips_exit(fmp);
	}

	set_fmp_fips_functest_KAT_mode(-1);

	dev_info(dev, "FIPS FUNC : Functional test end\n");
	dev_info(dev, "FIPS FUNC : -----------------------------------\n");
	dev_info(dev, "FIPS FUNC : Normal init start\n");
	dev_info(dev, "FIPS FUNC : -----------------------------------\n");

	ret = 0;
out:
	fmp->test_vops = NULL;
	return ret;
}
