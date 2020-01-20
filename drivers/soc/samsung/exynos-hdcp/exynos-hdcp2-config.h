/* todo: include/soc/samsung/exynos-hdcp2-config.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __EXYNOS_HDCP2_CONFIG_H__
#define __EXYNOS_HDCP2_CONFIG_H__

#define TEST_HDCP

#undef TEST_VECTOR_1
#define TEST_VECTOR_2
#undef HDCP_AKE_DEBUG
#undef HDCP_SKE_DEBUG
#undef HDCP_TX_REPEATER_DEBUG

#define HDCP_TX_VERSION_2_1
#define HDCP_TX_LC_PRECOMPUTE_SUPPORT

#endif
