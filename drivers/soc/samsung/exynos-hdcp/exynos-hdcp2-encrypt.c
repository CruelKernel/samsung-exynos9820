/*
 * drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-encrypt.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include "exynos-hdcp2-protocol-msg.h"
#include "exynos-hdcp2-log.h"

static void OS2BN(uint32_t *pdRes, uint8_t *pbSrc, size_t uSrcLen)
{
	int i;

	for (i = 0; i < uSrcLen; i += 4)
		pdRes[i/4] = pbSrc[uSrcLen-i-1] ^ (pbSrc[uSrcLen-i-2]<<8)
				^ (pbSrc[uSrcLen-i-3]<<16)
				^ (pbSrc[uSrcLen-i-4]<<24);
}

static void BN2OS(uint8_t *pbRes, uint32_t *pdSrc, size_t uSrcLen)
{
	int i;

	for (i = 0; i < uSrcLen; i++) {
		pbRes[4*i+0] = (uint8_t) (pdSrc[uSrcLen-1-i] >> 24);
		pbRes[4*i+1] = (uint8_t) (pdSrc[uSrcLen-1-i] >> 16);
		pbRes[4*i+2] = (uint8_t) (pdSrc[uSrcLen-1-i] >>  8);
		pbRes[4*i+3] = (uint8_t) (pdSrc[uSrcLen-1-i]);
	}
}

static uint32_t sec_bn_Add(uint32_t *pdDst, uint32_t *pdSrc1,	size_t uSrcLen1,
			uint32_t *pdSrc2, size_t uSrcLen2)
{
	int i;
	uint32_t carry, tmp;

	for (carry = i = 0; i < uSrcLen2; i++) {
		if ((pdSrc2[i] == 0xff) && (carry == 1))
			pdDst[i] = pdSrc1[i];
		else {
			tmp = pdSrc2[i] + carry;
			pdDst[i] = pdSrc1[i] + tmp;
			carry = ((pdDst[i]) < tmp) ? 1 : 0;
		}
	}

	for (i = uSrcLen2; i < uSrcLen1; i++) {
		pdDst[i] += carry;
		if (pdDst[i] >= carry)
			carry = 0;
		else
			carry = 1;
	}

	return carry;
}

static int make_priv_data(uint8_t *priv_data, uint8_t *str_ctr, uint8_t *input_ctr)
{
	uint8_t marker_bit;

	marker_bit = 0x1;

	priv_data[0] = 0x0;
	priv_data[1] = (str_ctr[0] >> 5) | marker_bit;
	priv_data[2] = (str_ctr[0] << 2) ^ (str_ctr[1] >> 6);
	priv_data[3] = ((str_ctr[1] << 2) ^ (str_ctr[2] >> 6)) | marker_bit;
	priv_data[4] = (str_ctr[2] << 1) ^ (str_ctr[3] >> 7);
	priv_data[5] = (str_ctr[3] << 1) | marker_bit;
	priv_data[6] = 0x0;
	priv_data[7] = (input_ctr[0] >> 3) | marker_bit;
	priv_data[8] = (input_ctr[0] << 4) ^ (input_ctr[1] >> 4);
	priv_data[9] = ((input_ctr[1] << 4) ^ (input_ctr[2] >> 4)) | marker_bit;
	priv_data[10] = (input_ctr[2] << 3) ^ (input_ctr[3] >> 5);
	priv_data[11] = ((input_ctr[3] << 3) ^ (input_ctr[4] >> 5)) | marker_bit;
	priv_data[12] = (input_ctr[4] << 2) ^ (input_ctr[5] >> 6);
	priv_data[13] = ((input_ctr[5] << 2) ^ (input_ctr[6] >> 6)) | marker_bit;
	priv_data[14] = (input_ctr[6] << 1) ^ (input_ctr[7] >> 7);
	priv_data[15] = (input_ctr[7] << 1) | marker_bit;

	return 0;
}

int encrypt_packet(uint8_t *priv_data,
	u64 input_addr, size_t input_len,
	u64 output_addr, size_t output_len,
	struct hdcp_tx_ctx *tx_ctx)
{
	uint32_t bn_str_ctr;
	uint32_t bn_input_ctr[HDCP_INPUT_CTR_LEN / 4];
	uint32_t tmp;
	int ret;

	/* make private data including counters for PES header */
	make_priv_data(priv_data, tx_ctx->str_ctr, tx_ctx->input_ctr);

	/* HDCP Encryption */
	/* todo: consider 32bit address. currently only support 64bit address */
	ret = teei_encrypt_packet((uint8_t *)input_addr, input_len,
			(uint8_t *)output_addr, output_len,
			tx_ctx->str_ctr, HDCP_STR_CTR_LEN,
			tx_ctx->input_ctr, HDCP_INPUT_CTR_LEN);
	if (ret) {
		hdcp_err("hdcp_encryption() is failed with %x\n", ret);
		return ERR_HDCP_ENCRYPTION;
	}

	/* Update strCtr/inputCtr in tx_ctx */
	OS2BN(&bn_str_ctr, tx_ctx->str_ctr, HDCP_STR_CTR_LEN);
	OS2BN(bn_input_ctr, tx_ctx->input_ctr, HDCP_INPUT_CTR_LEN);

	bn_str_ctr++;

	tmp = input_len / 16;
	if (input_len % 16)
		tmp++;
	sec_bn_Add(bn_input_ctr, bn_input_ctr, HDCP_INPUT_CTR_LEN/4, &tmp, 1);

	BN2OS(tx_ctx->str_ctr, &bn_str_ctr, HDCP_STR_CTR_LEN/4);
	BN2OS(tx_ctx->input_ctr, bn_input_ctr, HDCP_INPUT_CTR_LEN/4);

#ifdef HDCP_ENC_DEBUG
	hdcp_debug("[strCtr]:\n");
	hdcp_hexdump(tx_ctx->str_ctr, HDCP_STR_CTR_LEN);

	hdcp_debug("[inputCtr]:\n");
	hdcp_hexdump(tx_ctx->input_ctr, HDCP_INPUT_CTR_LEN);
#endif
	return 0;
}

