/* drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-encrypt.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __EXYNOS_HDCP2_ENCRYPT_H__
#define __EXYNOS_HDCP2_ENCRYPT_H__

#include <linux/types.h>

#define HDCP_PRIVATE_DATA_LEN	16 /* PES priv_data length */

int encrypt_packet(uint8_t *priv_data,
	u64 input_addr, size_t input_len,
	u64 output_addr, size_t output_len,
	struct hdcp_tx_ctx *tx_ctx);

#endif
