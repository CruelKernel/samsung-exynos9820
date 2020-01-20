/*
 * drivers/soc/samsung/exynos-hdcp/dplink/exynos-hdcp2-dplink-selftest.c
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
#include "../exynos-hdcp2-misc.h"
#include "../exynos-hdcp2-encrypt.h"
#include "../exynos-hdcp2.h"
#include "../exynos-hdcp2-log.h"
#include "exynos-hdcp2-dplink-protocol-msg.h"

#define DP_AKE_INIT_LEN			11
#define DP_AKE_NO_STORED_KM_LEN		128
#define DP_LC_INIT_LEN			8
#define DP_SKE_SEND_EKS_LEN		24

/* todo: define DP test vector */
extern uint8_t msg_rx_lc_send_l_prime_v22[32];
extern uint8_t msg_rx_send_h_prime_v22[32];
extern uint8_t msg_rx_send_pairing_info_v22[16];
extern uint8_t msg_rx_send_cert_v22[533];
extern unsigned char cert_v22[522];
extern uint8_t tv_emkey_v22[128];
extern uint8_t tv_ske_eskey_v22[16];
extern uint8_t tv_rcvid_list_v22[36];
extern uint8_t tv_rpauth_v_v22[16];
extern uint8_t tv_rpauth_stream_manage_v22[7];
extern uint8_t tv_rpauth_stream_ready_v22[32];

static struct hdcp_tx_ctx g_tx_ctx;
static struct hdcp_rx_ctx g_rx_ctx;
#if defined(TEST_VECTOR_1)
static int dp_utc_rpauth_stream_ready(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = decap_protocol_msg(DP_REPEATERAUTH_STREAM_READY,
			tv_rpauth_stream_ready_v22,
			sizeof(tv_rpauth_stream_ready_v22),
			HDCP_LINK_TYPE_DP,
			tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("RPAuth_Stream_Ready is failed with 0x%x\n", ret);
		return -1;
	}

	return 0;
}
static int dp_utc_rpauth_stream_manage(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	int ret;
	uint8_t m[525];
	size_t m_len;
	int i;

	ret = cap_protocol_msg(DP_REPEATERAUTH_STREAM_MANAGE,
				m,
				&m_len,
				HDCP_LINK_TYPE_DP,
				tx_ctx,
				rx_ctx);
	if (ret) {
		hdcp_err("RPAuth_Stream_Manage is failed with 0x%x\n", ret);
		return -1;
	}

	/* compare V with test vector */
	for (i = 0; i < m_len; i++)
		if (tv_rpauth_stream_manage_v22[i] != m[i])
			break;

	if (i != m_len) {
		hdcp_err("stream manage info doesn't match (%dth)\n", i);
		hdcp_err("stream_manage:\n");
		hdcp_hexdump(m, m_len);
		return -1;
	}

	return 0;
}
static int dp_utc_rpauth_send_revid_list(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = decap_protocol_msg(DP_REPEATERAUTH_SEND_RECEIVERID_LIST,
			tv_rcvid_list_v22,
			sizeof(tv_rcvid_list_v22),
			HDCP_LINK_TYPE_DP,
			tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("RPAuth_Send_ReceiverID_List is failed with 0x%x\n", ret);
		return -1;
	}

	return 0;
}

static int dp_utc_rpauth_send_ack(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	int ret;
	uint8_t m[525];
	size_t m_len;
	uint32_t v_len;
	int i;

	v_len = HDCP_RP_HMAC_V_LEN / 2;

	ret = cap_protocol_msg(DP_REPEATERAUTH_SEND_AKE,
				m,
				&m_len,
				HDCP_LINK_TYPE_DP,
				tx_ctx,
				rx_ctx);
	if (ret) {
		hdcp_err("RPAuth_Send_AKE is failed with 0x%x\n", ret);
		return -1;
	}

	/* compare V with test vector */
	for (i = 0; i < v_len; i++)
		if (tv_rpauth_v_v22[i] != m[i])
			break;

	if (i != v_len) {
		hdcp_err("m doesn't match (%dth)\n", i);
		return -1;
	}

	return 0;
}
#endif
static int dp_utc_ske_send_eks(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t m[525];
	size_t m_len;
	int ret;
	int i;

	tx_ctx->share_skey = 0;

	ret = cap_protocol_msg(SKE_SEND_EKS, m, &m_len, HDCP_LINK_TYPE_DP, tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("SKE_Send_Eks is failed with 0x%x\n", ret);
		return -1;
	}

	if (m_len != DP_SKE_SEND_EKS_LEN) {
		hdcp_err("Message LENGTH ERROR\n");
		return -1;
	}

	/* compare encrypted session key with test vector */
	for (i = 0; i < HDCP_AKE_MKEY_BYTE_LEN; i++)
		if (tv_ske_eskey_v22[i] != m[i])
			break;

	if (i != HDCP_AKE_MKEY_BYTE_LEN) {
		hdcp_err("m doesn't match (%dth)\n", i);
		return -1;
	}

	return 0;
}

/* Test SKE APIs */
static int dp_utc_ske(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx, int *cnt, int *fail)
{
	int ret;

	ret = dp_utc_ske_send_eks(tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("utc_ske_send_eks() is failed with 0x%x\n", ret);
		return -1;
	}

	return 0;
}
static int dp_utc_lc_send_l_prime(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = decap_protocol_msg(LC_SEND_L_PRIME, msg_rx_lc_send_l_prime_v22,
			sizeof(msg_rx_send_h_prime_v22),
			HDCP_LINK_TYPE_DP,
			tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("LC_Send_L_prime is failed with 0x%x\n", ret);
		return -1;
	}

	return 0;
}

static int dp_utc_lc_init(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t m[525];
	size_t m_len;
	int ret;

	ret = cap_protocol_msg(LC_INIT, m, &m_len, HDCP_LINK_TYPE_DP, tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("LC_Init is failed with 0x%x\n", ret);
		return -1;
	}

	if (m_len != DP_LC_INIT_LEN) {
		hdcp_err("Invalid Message length. len(%zu)\n", m_len);
		return -1;
	}

	return 0;
}

/* Test LC APIs */
static int dp_utc_lc(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx,
		int *cnt, int *fail)
{
	int ret;

	ret = dp_utc_lc_init(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = dp_utc_lc_send_l_prime(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	return 0;
}

static int dp_utc_ake_send_h_prime(struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = decap_protocol_msg(AKE_SEND_H_PRIME, msg_rx_send_h_prime_v22,
			sizeof(msg_rx_send_h_prime_v22),
			HDCP_LINK_TYPE_DP,
			tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("AKE_Send_H_prime is failed with 0x%x\n", ret);
		return -1;
	}

	return 0;
}

static int dp_utc_ake_send_pairing_info(struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	/* todo: support pairing */
	int i;
	int ret;
	int found_km;

	ret = decap_protocol_msg(AKE_SEND_PAIRING_INFO, msg_rx_send_pairing_info_v22,
			sizeof(msg_rx_send_pairing_info_v22),
			HDCP_LINK_TYPE_DP,
			tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("AKE_Send_Pairing_Info is failed with 0x%x\n", ret);
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
			if (tx_ctx->ekh_mkey[i] != msg_rx_send_pairing_info_v22[i])
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

static int dp_utc_ake_no_stored_km(struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t m[525];
	size_t m_len;
	int ret;
	int i;

	memcpy(rx_ctx->cert, cert_v22, HDCP_RX_CERT_LEN);

	ret = cap_protocol_msg(AKE_NO_STORED_KM, m, &m_len, HDCP_LINK_TYPE_DP, tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("AKE_No_Stored_km is failed with 0x%x\n", ret);
		return -1;
	}

	if (m_len != DP_AKE_NO_STORED_KM_LEN) {
		hdcp_err("Invalid Message length. len(%zu)\n", m_len);
		return -1;
	}

	for (i = 0; i < 128; i++)
		if (m[i] != tv_emkey_v22[i])
			break;

	if (i != 128) {
		hdcp_err("Encryption Master Key ERROR\n");
		return -1;
	}

	return 0;
}

static int dp_utc_ake_send_cert(struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = decap_protocol_msg(AKE_SEND_CERT, msg_rx_send_cert_v22,
			sizeof(msg_rx_send_cert_v22),
			HDCP_LINK_TYPE_DP,
			tx_ctx, rx_ctx);
	if (ret) {
		hdcp_err("AKE_Send_Cert is failed with ret, 0x%x\n", ret);
		return -1;
	}

	return 0;
}

static int dp_utc_ake_init(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t m[525];
	size_t m_len;
	int ret;

	ret = cap_protocol_msg(DP_AKE_INIT, m, &m_len, HDCP_LINK_TYPE_DP, tx_ctx, NULL);
	if (ret) {
		hdcp_err("AKE_Init is failed with 0x%x\n", ret);
		return -1;
	}

	/* check message length */
	if (m_len != DP_AKE_INIT_LEN) {
		hdcp_err("Invalid Message Length. len(%zu)\n", m_len);
		return -1;
	}

	return 0;
}

/* Test AKE APIs */
static int dp_utc_ake(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx,
		int *cnt, int *fail)
{
	int ret;

	ret = dp_utc_ake_init(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = dp_utc_ake_send_cert(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = dp_utc_ake_no_stored_km(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = dp_utc_ake_send_h_prime(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = dp_utc_ake_send_pairing_info(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	return 0;
}

/* Test RP Auth APIs */
#if defined(TEST_VECTOR_1)
static int dp_utc_rpauth(struct hdcp_tx_ctx *tx_ctx, struct hdcp_rx_ctx *rx_ctx,
		int *cnt, int *fail)
{
	int ret;

	ret = dp_utc_rpauth_send_revid_list(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = dp_utc_rpauth_send_ack(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = dp_utc_rpauth_stream_manage(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	ret = dp_utc_rpauth_stream_ready(tx_ctx, rx_ctx);
	if (ret < 0)
		return ret;

	return 0;
}
#endif


/* Test HDCP API functions */
int dp_hdcp_protocol_self_test(void)
{
	int total_cnt = 0, fail_cnt = 0;

	hdcp_info("[ AKE UTC]\n");
	if (dp_utc_ake(&g_tx_ctx, &g_rx_ctx, &total_cnt, &fail_cnt) < 0) {
		hdcp_info("AKE UTC: fail\n");
		return -1;
	} else {
		hdcp_info("AKE UTC: success\n");
	}

	hdcp_info("\n[ LC UTC]\n");
	if (dp_utc_lc(&g_tx_ctx, &g_rx_ctx, &total_cnt, &fail_cnt) < 0) {
		hdcp_info("LC UTC: fail\n");
		return -1;
	} else {
		hdcp_info("LC UTC: success\n");
	}

	hdcp_info("\n[ SKE UTC]\n");
	if (dp_utc_ske(&g_tx_ctx, &g_rx_ctx, &total_cnt, &fail_cnt) < 0) {
		hdcp_info("SKE UTC: fail\n");
		return -1;
	} else {
		hdcp_info("SKE UTC: success\n");
	}

#if defined(TEST_VECTOR_1)
	hdcp_info("\n[ RP UTC]\n");
	if (dp_utc_rpauth(&g_tx_ctx, &g_rx_ctx, &total_cnt, &fail_cnt) < 0) {
		hdcp_info("RP UTC: fail\n");
		return -1;
	} else {
		hdcp_info("RP UTC: success\n");
	}
#endif

	return 0;
}
