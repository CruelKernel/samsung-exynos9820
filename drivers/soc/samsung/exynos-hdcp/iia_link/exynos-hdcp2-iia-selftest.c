/*
 * drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-selftest.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>

#include "../exynos-hdcp2-config.h"
#include "../exynos-hdcp2-protocol-msg.h"
#include "../exynos-hdcp2-testvector.h"
#include "../exynos-hdcp2-misc.h"
#include "../exynos-hdcp2-encrypt.h"
#include "../exynos-hdcp2.h"
#include "../exynos-hdcp2-log.h"

#define AKE_INIT_LEN			9
#define AKE_TRANSMITTER_INFO_LEN	6
#define AKE_NO_STORED_KM_LEN		129
#define AKE_STORED_KM_LEN		33
#define AKE_SEND_PAIRING_INFO_LEN	17
#define LC_INIT_LEN			9
#define LC_RTT_CHALLENGE_LEN		17
#define SKE_SEND_EKS_LEN		25

struct hdcp_tx_ctx uc_tx_ctx;
struct hdcp_rx_ctx uc_rx_ctx;

/* Test HDCP Encryption */
static int utc_encryption(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t *input = NULL;
	u64 input_phy = 0;
	uint8_t *output = NULL;
	u64 output_phy = 0;
	int ret = 0;
	size_t packet_len;
	uint8_t pes_priv[HDCP_PRIVATE_DATA_LEN];

	packet_len = sizeof(tv_plain);

	/* Allocate enc in/out buffer for test */
	input = (uint8_t *)kzalloc(packet_len, GFP_KERNEL);
	if (!input) {
		hdcp_err("alloc enc input buffer is failed\n");
		return -ENOMEM;
	}

	output = (uint8_t *)kzalloc(packet_len, GFP_KERNEL);
	if (!output) {
		kfree(input);
		hdcp_err("alloc enc input buffer is failed\n");
		return -ENOMEM;
	}

	/* send physical address to SWd */
	input_phy = virt_to_phys((void *)input);
	output_phy = virt_to_phys((void *)output);

	/* set input param */
	memcpy(input, tv_plain, packet_len);
	/* set input counters */
	memset(&tx_ctx->input_ctr, 0x00, HDCP_INPUT_CTR_LEN);
	/* set output counters */
	memset(&tx_ctx->str_ctr, 0x00, HDCP_STR_CTR_LEN);

	__flush_dcache_area(input, packet_len);
	__flush_dcache_area(output, packet_len);
	ret = encrypt_packet(pes_priv,
		input_phy, packet_len,
		output_phy, packet_len,
		tx_ctx);
	if (ret) {
		kfree(input);
		kfree(output);
		hdcp_err("encrypt_packet() is failed with 0x%x\n", ret);
		return -1;
	}

	if (memcmp(output, tv_cipher, packet_len)) {
		hdcp_err("Wrong encrypted value.\n");
		hdcp_err("expected:\n");
		hdcp_hexdump(tv_cipher, packet_len);

		hdcp_err("actual:\n");
		hdcp_hexdump(output, packet_len);

		kfree(input);
		kfree(output);
		return -1;
	} else
		hdcp_info("HDCP:Encryption success.\n");

	kfree(input);
	kfree(output);
	return 0;
}

static int utc_ske_send_eks(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t m[525];
	size_t m_len;
	int ret;
	int i;

	tx_ctx->share_skey = 0;

	ret = cap_protocol_msg(SKE_SEND_EKS, m, &m_len, HDCP_LINK_TYPE_IIA, tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("SKE_Send_Eks() is failed with 0x%x\n", ret);
		return -1;
	}

	/* check message ID & length */
	if (m[0] != 11) {
		hdcp_err("Message ID ERROR\n");
		return -1;
	}

	if (m_len != SKE_SEND_EKS_LEN) {
		hdcp_err("Message LENGTH ERROR\n");
		return -1;
	}

	/* compare encrypted session key with test vector */
	for (i = 0; i < HDCP_AKE_MKEY_BYTE_LEN; i++)
		if (tv_eskey[i] != m[1+i])
			break;

	if (i != HDCP_AKE_MKEY_BYTE_LEN) {
		hdcp_err("m doesn't match (%dth)\n", i);
		return -1;
	}

	return 0;
}

/* Test SKE APIs */
static int utc_ske(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx, int *cnt, int *fail)
{
	return utc_ske_send_eks(tx_ctx, rx_ctx);
}

static int utc_lc_send_l_prime(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = decap_protocol_msg(LC_SEND_L_PRIME, msg_rx_lc_send_l_prime,
			sizeof(msg_rx_send_h_prime),
			HDCP_LINK_TYPE_IIA,
			tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("LC_Send_L_prime() is failed with 0x%x\n", ret);
		return -1;
	}

	return 0;
}

static int utc_rtt_challenge(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t m[525];
	size_t m_len;
	int ret;
	int i;

	ret = cap_protocol_msg(RTT_CHALLENGE, m, &m_len, HDCP_LINK_TYPE_IIA, tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("RTT_Challenge() is failed with 0x%x\n", ret);
		return -1;
	}

	/* check message ID & length */
	if (m[0] != 14) {
		hdcp_err("Message ID ERROR\n");
		return -1;
	}

	if (m_len != LC_RTT_CHALLENGE_LEN) {
		hdcp_err("Message LENGTH ERROR\n");
		return -1;
	}

	/* check ls128_hmac */
	for (i = 0; i < HDCP_HMAC_SHA256_LEN/2; i++)
		if (m[i+1] != tv_lc_lsb16_hmac[i])
			break;

	if (i != HDCP_HMAC_SHA256_LEN/2) {
		hdcp_err("failed to compare ls128 hmac\n");
		return -1;
	}

	return 0;
}

static int utc_rtt_ready(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = decap_protocol_msg(RTT_READY, msg_rx_rtt_ready,
			sizeof(msg_rx_rtt_ready),
			HDCP_LINK_TYPE_IIA,
			tx_ctx,
			rx_ctx);
	if (ret) {
		hdcp_err("RTT_Ready() is failed with 0x%x\n", ret);
		return -1;
	}

	return 0;
}

static int utc_lc_init(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t m[525];
	size_t m_len;
	int ret;
	int i;

	ret = cap_protocol_msg(LC_INIT, m, &m_len, HDCP_LINK_TYPE_IIA, tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("LC_Init() is failed with 0x%x\n", ret);
		return -1;
	}

	/* check message ID & length */
	if (m[0] != 9) {
		hdcp_err("Message ID ERROR\n");
		return -1;
	}

	if (m_len != LC_INIT_LEN) {
		hdcp_err("Message LENGTH ERROR\n");
		return -1;
	}

	if ((rx_ctx->version != HDCP_VERSION_2_0)
		&& tx_ctx->lc_precomp
		&& rx_ctx->lc_precomp) {
		for (i = 0; i < HDCP_HMAC_SHA256_LEN/2; i++)
			if (tx_ctx->lsb16_hmac[i] != tv_lc_hmac[i+16])
				break;
		if (i != HDCP_HMAC_SHA256_LEN/2) {
			hdcp_err("failed to compare lsb16 hmac\n");
			return -1;
		}
	} else {
		return 0;
	}

	return 0;
}

/* Test LC APIs */
static int utc_lc(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx,
		int *cnt, int *fail)
{
	int ret;

	ret = utc_lc_init(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = utc_lc_send_l_prime(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = utc_rtt_ready(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = utc_rtt_challenge(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	return 0;
}

static int utc_ake_send_h_prime(struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = decap_protocol_msg(AKE_SEND_H_PRIME, msg_rx_send_h_prime,
			sizeof(msg_rx_send_h_prime),
			HDCP_LINK_TYPE_IIA,
			tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("AKE_Send_H_prime() is failed with 0x%x\n", ret);
		return -1;
	}

	return 0;
}

static int utc_ake_send_rrx(struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	int ret;
	int i;

	ret = decap_protocol_msg(AKE_SEND_RRX, msg_rx_send_rrx, sizeof(msg_rx_send_rrx),
				HDCP_LINK_TYPE_IIA, tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("make_AKE_Send_rrx() is failed with 0x%x\n", ret);
		return -1;
	}

	/* compare rrx with test vector */
	for (i = 0; i < HDCP_RRX_BYTE_LEN; i++)
		if (rx_ctx->rrx[i] != tv_rrx[i])
			break;

	if (i != HDCP_RRX_BYTE_LEN) {
		hdcp_err("rrx doesn't match (%dth)\n", i);
		return -1;
	}

	return 0;
}

static int utc_ake_stored_km(struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
#if 0
	uint8_t m[525];
	size_t m_len;
	int ret;
	int i;

	ret = cap_protocol_msg(AKE_STORED_KM, m, &m_len, HDCP_LINK_TYPE_IIA, tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("AKE_Stored_km() is failed with 0x%x\n", ret);
		return -1;
	}

	if (m[0] != 5) {
		hdcp_err("Message ID ERROR, it has %d\n", m[0]);
		return -1;
	}

	if (m_len != AKE_STORED_KM_LEN) {
		hdcp_err("Message LENGTH ERROR\n");
		return -1;
	}


	for (i = 0; i < HDCP_AKE_MKEY_BYTE_LEN; i++)
		if (tv_pairing_ekh[i] != m[i+1])
			break;

	if (i != HDCP_AKE_MKEY_BYTE_LEN) {
		hdcp_err("ekh(m) doesn't match (%dth)\n", i);
		return -1;
	}


	for (i = 0; i < HDCP_AKE_MKEY_BYTE_LEN; i++)
		if (tv_pairing_m[i] != m[i + 1 + HDCP_AKE_MKEY_BYTE_LEN])
			break;

	if (i != HDCP_AKE_MKEY_BYTE_LEN) {
		hdcp_err("m doesn't match (%dth)\n", i);
		return -1;
	}
#endif

	return 0;
}

static int utc_ake_send_pairing_info(struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	int ret;
	int i;
	int found_km;

	/* Extract Receiver ID */
	for (i = 0; i < RECEIVER_ID_BYTE_LEN; i++)
		rx_ctx->receiver_id[i] = rx_ctx->cert[i];

	ret = decap_protocol_msg(AKE_SEND_PAIRING_INFO, msg_rx_send_pairing_info,
			sizeof(msg_rx_send_pairing_info),
			HDCP_LINK_TYPE_IIA,
			tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("AKE_SEND_PAIRING_INFO() is failed with 0x%x\n", ret);
		return -1;
	}

	ret = ake_find_masterkey(&found_km,
				tx_ctx->ekh_mkey, HDCP_AKE_EKH_MKEY_BYTE_LEN,
				tx_ctx->m, HDCP_AKE_M_BYTE_LEN);
	if (ret) {
		hdcp_err("find_masterkey() is failed with 0x%x\n", ret);
		return -1;
	}

	if (found_km) {
		for (i = 0; i < HDCP_AKE_MKEY_BYTE_LEN; i++)
			if (tx_ctx->ekh_mkey[i] != msg_rx_send_pairing_info[i + 1])
				break;

		if (i != HDCP_AKE_MKEY_BYTE_LEN) {
			hdcp_err("ekh(m) doesn't match (%dth)\n", i);
			return -1;
		}
	} else {
		hdcp_err("ekh(m) is not found\n");
		return -1;
	}

	return 0;
}

static int utc_ake_no_stored_km(struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t m[525];
	size_t m_len;
	int ret;
	int i;

	memcpy(rx_ctx->cert, cert, HDCP_RX_CERT_LEN);

	ret = cap_protocol_msg(AKE_NO_STORED_KM, m, &m_len, HDCP_LINK_TYPE_IIA, tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("make_AKE_No_Stored_km() is failed with 0x%x\n", ret);
		return -1;
	}

	if (m[0] != 4) {
		hdcp_err("Message ID ERROR, it has %d\n", m[0]);
		return -1;
	}

	if (m_len != AKE_NO_STORED_KM_LEN) {
		hdcp_err("Message LENGTH ERROR\n");
		return -1;
	}


	for (i = 0; i < 128; i++)
		if (m[i + 1] != tv_emkey[i])
			break;

	if (i != 128) {
		hdcp_err("Encryption Master Key ERROR\n");
		return -1;
	}

	return 0;
}

static int utc_ake_receiver_info(struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = decap_protocol_msg(AKE_RECEIVER_INFO,
				msg_rx_receiver_info,
				sizeof(msg_rx_receiver_info),
				HDCP_LINK_TYPE_IIA,
				tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("get_Receiver_Info() is failed with %x\n", ret);
		return -1;
	}

	return 0;
}

static int utc_ake_send_cert(struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = decap_protocol_msg(AKE_SEND_CERT, msg_rx_send_cert,
			sizeof(msg_rx_send_cert),
			HDCP_LINK_TYPE_IIA,
			tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("get_AKE_Send_Cert() is failed with ret, 0x%x\n",
		ret);
		return -1;
	}

	return 0;
}

static int utc_ake_transmitter_info(struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t m[525];
	size_t m_len;
	int ret;

	ret = cap_protocol_msg(AKE_TRANSMITTER_INFO, m, &m_len, HDCP_LINK_TYPE_IIA, tx_ctx, NULL);
	if (ret) {
		hdcp_err("cap_ake_transmitter_info() is failed with 0x%x\n", ret);
		return -1;
	}

	/* check message ID & length */
	if (m[0] != 19) {
		hdcp_err("Message ID ERROR\n");
		return -1;
	}

	if ((m_len != AKE_TRANSMITTER_INFO_LEN) || (m[2] < 6)) {
		hdcp_err("Message LENGTH ERROR\n");
		return -1;
	}

	/* Version & Precompuation Check */
#ifdef HDCP_TX_VERSION_2_1
	if (m[3] != HDCP_VERSION_2_1) {
#else
	if (m[3] == HDCP_VERSION_2_1) {
#endif
		hdcp_err("Version set wrong\n");
		return -1;
	}

	/* precomputation check */
#ifdef HDCP_TX_LC_PRECOMPUTE_SUPPORT
	if ((m[4] != 0) || (m[5] != 0x01)) {
#else
	if ((m[4] != 0) || (m[5] != 0)) {
#endif
		hdcp_err("Precomputation ERROR, it has 0x%x%x\n", m[4], m[5]);
		return -1;
	}

	return 0;
}

static int utc_ake_init(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t m[525];
	size_t m_len;
	int ret;

	ret = cap_protocol_msg(AKE_INIT, m, &m_len, HDCP_LINK_TYPE_IIA, tx_ctx, NULL);
	if (ret) {
		hdcp_err("cap_ake_init() is failed with 0x%x\n", ret);
		return -1;
	}

	/* check message ID & length */
	if (m[0] != 2) {
		hdcp_err("Message ID ERROR\n");
		return -1;
	}

	if (m_len != AKE_INIT_LEN) {
		hdcp_err("Message LENGTH ERROR\n");
		return -1;
	}

	return 0;
}

/* Test AKE APIs */
static int utc_ake(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx,
		int *cnt, int *fail)
{
	int ret;

	ret = utc_ake_init(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = utc_ake_transmitter_info(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = utc_ake_send_cert(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = utc_ake_receiver_info(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = utc_ake_no_stored_km(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = utc_ake_send_rrx(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = utc_ake_send_h_prime(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = utc_ake_send_pairing_info(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = utc_ake_stored_km(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	return 0;
}

/* Test HDCP API functions */
int iia_hdcp_protocol_self_test(void)
{
	int ake_cnt = 0, ake_fail = 0;
	int lc_cnt = 0, lc_fail = 0;
	int ske_cnt = 0, ske_fail = 0;
	int ret;

	hdcp_info("[ AKE UTC]\n");
	ret = utc_ake(&uc_tx_ctx, &uc_rx_ctx, &ake_cnt, &ake_fail);
	if (ret < 0) {
		hdcp_info("AKE UTC: fail\n");
		return ret;
	} else {
		hdcp_info("AKE UTC: success\n");
	}

	hdcp_info("\n[ LC UTC]\n");
	ret = utc_lc(&uc_tx_ctx, &uc_rx_ctx, &lc_cnt, &lc_fail);
	if (ret < 0) {
		hdcp_info("LC UTC: fail\n");
		return ret;
	} else {
		hdcp_info("LC UTC: success\n");
	}

	hdcp_info("\n[ SKE UTC]\n");
	ret = utc_ske(&uc_tx_ctx, &uc_rx_ctx, &ske_cnt, &ske_fail);
	if (ret < 0) {
		hdcp_info("SKE UTC: fail\n");
		return ret;
	} else {
		hdcp_info("SKE UTC: success\n");
	}

	hdcp_info("\n[ Encryption UTC]\n");
	ret = utc_encryption(&uc_tx_ctx, &uc_rx_ctx);
	if (ret < 0) {
		hdcp_info("Encrypt UTC: fail\n");
		return ret;
	} else {
		hdcp_info("Encrypt UTC: success\n");
	}

	return 0;
}
