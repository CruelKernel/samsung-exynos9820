/*
 * drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-crypto.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __EXYNOS_HDCP2_CRYPTO_H__
#define __EXYNOS_HDCP2_CRYPTO_H__

#define HDCP_SHA1_SIZE (160 / 8)

int hdcp_calc_sha1(u8 *digest, const u8 *buf, unsigned int buflen);

#endif
