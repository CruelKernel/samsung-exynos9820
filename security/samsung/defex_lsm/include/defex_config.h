/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
*/

#ifndef __CONFIG_SECURITY_DEFEX_CONFIG_H
#define __CONFIG_SECURITY_DEFEX_CONFIG_H

/* MAX VALUES */
#define MAX_LEN					256
#define MAX_EXCEPTIONS				1024
#define MAX_PROTECTED				1024

#ifdef DEFEX_PERMISSIVE_PED
#define GLOBAL_PED_STATUS		(FEATURE_CHECK_CREDS | FEATURE_CHECK_CREDS_SOFT)
#else
#define GLOBAL_PED_STATUS		FEATURE_CHECK_CREDS
#endif

#ifdef DEFEX_PERMISSIVE_SP
#define GLOBAL_SAFEPLACE_STATUS		(FEATURE_SAFEPLACE | FEATURE_SAFEPLACE_SOFT)
#else
#define GLOBAL_SAFEPLACE_STATUS		FEATURE_SAFEPLACE
#endif

#ifdef DEFEX_PERMISSIVE_IM
#define GLOBAL_IMMUTABLE_STATUS		(FEATURE_IMMUTABLE | FEATURE_IMMUTABLE_SOFT)
#else
#define GLOBAL_IMMUTABLE_STATUS		FEATURE_IMMUTABLE
#endif

/* Uncomment for Kernels, that require it */
#define STRICT_UID_TYPE_CHECKS			1

#if defined(DEFEX_PED_ENABLE) || defined(DEFEX_SAFEPLACE_ENABLE) || defined(DEFEX_IMMUTABLE_ENABLE)
#define DEFEX_FEATURE_ENABLE
#endif /* DEFEX_PED_ENABLE || DEFEX_SAFEPLACE_ENABLE || DEFEX_IMMUTABLE_ENABLE */

int defex_get_features(void);

#endif /* CONFIG_SECURITY_DEFEX_CONFIG_H */
