/* todo: include/soc/samsung/hdcp2-if.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __HDCP2_IF_H__
#define __HDCP2_IF_H__

int exynos_hdcp2_authenticate(uint32_t *lk_id);
int exynos_hdcp2_encrypt(uint32_t link_id);

#endif

