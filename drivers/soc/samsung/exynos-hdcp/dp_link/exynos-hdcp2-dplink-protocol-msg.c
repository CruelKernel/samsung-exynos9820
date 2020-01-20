/*
 * drivers/soc/samsung/exynos-hdcp/dp_link/exynos-hdcp2-dplink-protocol-msg.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>

#include "../exynos-hdcp2-config.h"
#include "../exynos-hdcp2-protocol-msg.h"
#include "../exynos-hdcp2.h"
#include "../exynos-hdcp2-misc.h"
#include "../exynos-hdcp2-log.h"
#include "exynos-hdcp2-dplink-protocol-msg.h"
#include "exynos-hdcp2-dplink-if.h"

/* HDCP protocol message capulate & decapulate functions for DP */
static int dp_cap_ake_init(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int dp_cap_ake_no_stored_km(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int dp_cap_ake_stored_km(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int dp_decap_ake_send_cert(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int dp_decap_ake_send_h_prime(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int dp_decap_ake_send_pairing_info(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int dp_cap_lc_init(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int dp_decap_lc_send_l_prime(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int dp_cap_ske_send_eks(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int dp_decap_rpauth_send_recvid_list(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int dp_cap_rpauth_send_ack(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int dp_cap_rpauth_stream_manage(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int dp_decap_rpauth_stream_ready(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
int (*proto_dp[])(uint8_t *, size_t *,
			struct hdcp_tx_ctx *,
			struct hdcp_rx_ctx *) = {
	dp_cap_ake_init,
	dp_decap_ake_send_cert,
	dp_cap_ake_no_stored_km,
	dp_cap_ake_stored_km,
	NULL, /* not defined in HDCP2.2 DP spec */
	dp_decap_ake_send_h_prime,
	dp_decap_ake_send_pairing_info,
	dp_cap_lc_init,
	dp_decap_lc_send_l_prime,
	dp_cap_ske_send_eks,
	dp_decap_rpauth_send_recvid_list,
	NULL, /* not defined in HDCP2.2 DP spec */
	NULL, /* not defined in HDCP2.2 DP spec */
	dp_cap_rpauth_send_ack,
	dp_cap_rpauth_stream_manage,
	dp_decap_rpauth_stream_ready,
	NULL, /* to do */
	NULL, /* not defined in HDCP2.2 DP spec */
	NULL, /* not defined in HDCP2.2 DP spec */
};

static int dp_rp_verify_stream_ready(uint8_t *m_prime)
{
	int ret;

	ret = teei_verify_m_prime(m_prime, NULL, 0);
	if (ret) {
		hdcp_err("teei_verify_m_prime() is failed with %x\n", ret);
		return ERR_RP_INVALID_M_PRIME;
	}

	return 0;
}

static int dp_rp_gen_stream_manage(struct dp_stream_info *strm)
{
	int ret;

	ret = teei_gen_stream_manage(strm->stream_num,
				strm->streamid,
				strm->seq_num_m,
				strm->k,
				strm->streamid_type);
	if (ret) {
		hdcp_err("teei_gen_stream_manage() is failed with %x\n", ret);
		return ERR_RP_GEN_STREAM_MG;
	}

	return 0;
}

static int dp_rp_set_rcvid_list(struct hdcp_rpauth_info *rp)
{
	int ret;

	ret = teei_set_rcvlist_info(rp->rx_info,
				rp->seq_num_v,
				rp->v_prime,
				rp->u_rcvid.lst,
				rp->v,
				&rp->valid);
	if (ret) {
		hdcp_err("teei_set_rcvid_list() is failed with %x\n", ret);
		return ERR_RP_VERIFY_RCVLIST;
	}

	return 0;
}

static int dp_ake_generate_rtx(uint8_t *rtx, size_t rtx_len,
				uint8_t *caps, uint32_t caps_len)
{
	int ret;

	ret = teei_gen_rtx(HDCP_LINK_TYPE_DP, rtx, rtx_len, caps, caps_len);
	if (ret) {
		hdcp_err("generate_none_secret_key() is failed with %x\n", ret);
		return ERR_GENERATE_NON_SECKEY;
	}

	return 0;
}

static int dp_cap_ake_init(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	size_t rtx_len;
	uint32_t caps_len;
	int ret;

	/* Buffer Check */
	if ((m == NULL) || (m_len == NULL) || (tx_ctx == NULL))
		return ERR_WRONG_BUFFER;

	rtx_len = HDCP_AKE_RTX_BYTE_LEN;
	caps_len = HDCP_CAPS_BYTE_LEN;

	/* Generate rtx */
	ret = dp_ake_generate_rtx(tx_ctx->rtx, rtx_len,
				tx_ctx->caps, caps_len);
	if (ret) {
		hdcp_err("failed to generate rtx. ret(%d)\n", ret);
		return ERR_GENERATE_RTX;
	}

	/* Make Message */
	memcpy(&m[0], tx_ctx->rtx, rtx_len);
	memcpy(&m[rtx_len], tx_ctx->caps, HDCP_CAPS_BYTE_LEN);
	*m_len = rtx_len + HDCP_CAPS_BYTE_LEN;

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("cap_ake_init(%lu) \n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

static int dp_cap_ake_no_stored_km(uint8_t *m, size_t *m_len,
				struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t enc_mkey[HDCP_AKE_ENCKEY_BYTE_LEN];
	int ret;

	if ((m == NULL) || (m_len == NULL) ||
		(tx_ctx == NULL) || (rx_ctx == NULL))
		return ERR_WRONG_BUFFER;

	/* Generate/Encrypt master key */
	ret = ake_generate_masterkey(HDCP_LINK_TYPE_DP,
					enc_mkey,
					sizeof(enc_mkey));
	/* Generate/Encrypt master key */
	if (ret)
		return ret;

	/* Make Message */
	memcpy(m, enc_mkey, HDCP_AKE_ENCKEY_BYTE_LEN);
	*m_len = HDCP_AKE_ENCKEY_BYTE_LEN;

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("cap_ake_no_stored_km(%lu) \n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

static int dp_cap_ake_stored_km(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	int found_km;
	int ret;

	if ((m == NULL) || (m_len == NULL) ||
		(tx_ctx == NULL) || (rx_ctx == NULL))
		return ERR_WRONG_BUFFER;

	ret = ake_find_masterkey(&found_km,
				tx_ctx->ekh_mkey, HDCP_AKE_EKH_MKEY_BYTE_LEN,
				tx_ctx->m, HDCP_AKE_M_BYTE_LEN);
	if (ret || !found_km) {
		hdcp_err("find_masterkey() is failed with ret(0x%x). found(%d)\n", ret, found_km);
		return -1;
	}

	/* Make Message */
	memcpy(m, tx_ctx->ekh_mkey, HDCP_AKE_EKH_MKEY_BYTE_LEN);
	memcpy(&m[HDCP_AKE_EKH_MKEY_BYTE_LEN], tx_ctx->m, HDCP_AKE_M_BYTE_LEN);
	*m_len = HDCP_AKE_EKH_MKEY_BYTE_LEN + HDCP_AKE_M_BYTE_LEN;

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("cap_ake_stored_km(%lu) \n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

static int dp_decap_ake_send_cert(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = dp_check_received_msg(m, *m_len, HDCP_RX_CERT_LEN +
					       HDCP_RRX_BYTE_LEN +
					       HDCP_CAPS_BYTE_LEN);
	if (ret)
		return ret;

	ret = ake_verify_cert(m, HDCP_RX_CERT_LEN,
				&m[HDCP_RX_CERT_LEN], HDCP_RRX_BYTE_LEN,
				&m[HDCP_RX_CERT_LEN + HDCP_RRX_BYTE_LEN], HDCP_CAPS_BYTE_LEN);
	if (ret) {
		hdcp_err("teei_verify_cert is failed with %x\n", ret);
		return ERR_VERIFY_CERT;
	}

	return 0;
}

static int dp_decap_ake_send_h_prime(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	/* compare H == H' */
	ret = ake_compare_hmac(m, HDCP_HMAC_SHA256_LEN);
	if (ret) {
		hdcp_err("decap_ake_send_h_prime is failed. ret(%d)\n", ret);
		return ret;
	}

	return 0;
}

static int dp_decap_ake_send_pairing_info(uint8_t *m, size_t *m_len,
				struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	memcpy(tx_ctx->ekh_mkey, &m[0], HDCP_AKE_EKH_MKEY_BYTE_LEN);

	/* Store the Key */
	ret = ake_store_master_key(tx_ctx->ekh_mkey, HDCP_AKE_EKH_MKEY_BYTE_LEN);
	if (ret) {
		hdcp_err("failed to store master key. ret(%d)\n", ret);
		return ret;
	}

	return 0;
}

int dp_cap_lc_init(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
		struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	if ((m == NULL) || (m_len == NULL) || (tx_ctx == NULL) ||
	    (rx_ctx == NULL))
		return ERR_WRONG_BUFFER;

	/* Generate rn */
	ret = lc_generate_rn(tx_ctx->rn, HDCP_RTX_BYTE_LEN);
	if (ret) {
		hdcp_err("failed to generate rtx. ret(%d)\n", ret);
		return ret;
	}

	/* Make Message */
	memcpy(m, tx_ctx->rn, HDCP_RTX_BYTE_LEN);
	*m_len = HDCP_RTX_BYTE_LEN;

	return 0;
}

static int dp_decap_lc_send_l_prime(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t *rx_hmac;
	size_t len;
	int ret;

	len = HDCP_HMAC_SHA256_LEN;

	/* No_precomputation: compare hmac
	   precomputation: compare the most significant 128bits of L & L' */
	rx_hmac = m;
	ret = lc_compare_hmac(rx_hmac, len);
	if (ret)
		return ret;

	return 0;
}

static int dp_cap_ske_send_eks(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	int ret;
	uint8_t enc_skey[HDCP_AKE_MKEY_BYTE_LEN];

	if ((m == NULL) || (m_len == NULL) || (tx_ctx == NULL) ||
	    (rx_ctx == NULL))
		return ERR_WRONG_BUFFER;

	/* Generate riv */
	if (!tx_ctx->share_skey) {
		ret = ske_generate_riv(tx_ctx->riv);
		if (ret)
			return ERR_GENERATE_RIV;
	}

	/* Generate encrypted Session Key */
	ret = ske_generate_sessionkey(HDCP_LINK_TYPE_DP,
					enc_skey,
					tx_ctx->share_skey);
	if (ret)
		return ret;

	/* Make Message */
	memcpy(m, enc_skey, HDCP_AKE_MKEY_BYTE_LEN);
	memcpy(&m[HDCP_AKE_MKEY_BYTE_LEN], tx_ctx->riv, HDCP_RTX_BYTE_LEN);
	*m_len = HDCP_AKE_MKEY_BYTE_LEN + HDCP_RTX_BYTE_LEN;

#ifdef HDCP_SKE_DEBUG
	hdcp_debug("SKE_Send_Eks(%lu)\n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

static int dp_decap_rpauth_send_recvid_list(uint8_t *m,
					size_t *m_len,
					struct hdcp_tx_ctx *tx_ctx,
					struct hdcp_rx_ctx *rx_ctx)
{
	int ret;
	struct hdcp_rpauth_info *rp;

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("RepeaterAuth_Send_ReceiverID_List(%lu):\n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif

	rp = &tx_ctx->rpauth_info;

	memcpy(rp->rx_info, &m[0], HDCP_RP_RX_INFO_LEN);
	memcpy(rp->seq_num_v, &m[HDCP_RP_RX_INFO_LEN], HDCP_RP_SEQ_NUM_V_LEN);
	memcpy(rp->v_prime, &m[HDCP_RP_RX_INFO_LEN + HDCP_RP_SEQ_NUM_V_LEN],
		HDCP_RP_HMAC_V_LEN / 2);
	memcpy(rp->u_rcvid.lst,
		&m[HDCP_RP_RX_INFO_LEN + HDCP_RP_SEQ_NUM_V_LEN + (HDCP_RP_HMAC_V_LEN / 2)],
		HDCP_RP_RCVID_LIST_LEN);

	/* set receiver id list */
	ret = dp_rp_set_rcvid_list(rp);
	if (ret) {
		hdcp_err("failed to set receiver id list. ret(%d)\n", ret);
		return ret;
	}

	return 0;
}

static int dp_cap_rpauth_send_ack(uint8_t *m,
				size_t *m_len,
				struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
        /* Make Message */
	if (tx_ctx->rpauth_info.valid == 0) {
		memcpy(m, tx_ctx->rpauth_info.v, HDCP_RP_HMAC_V_LEN / 2);
		*m_len = HDCP_RP_HMAC_V_LEN / 2;
	} else {
		*m_len = 0;
		return ERR_SEND_ACK;
	}

#ifdef HDCP_TX_REPEATER_DEBUG
	hdcp_debug("RepeaterAuth_Send_Ack(%u)\n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif

	return 0;
}

static int dp_cap_rpauth_stream_manage(uint8_t *m,
				size_t *m_len,
				struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	struct dp_stream_info *strm;
	int ret;

	strm = &tx_ctx->strm.dp;

	hdcp_dplink_get_stream_info(&strm->stream_num, strm->streamid);

	/* set receiver id list */
	ret = dp_rp_gen_stream_manage(strm);
	if (ret) {
		hdcp_err("failed to gen stream manage. ret(%d)\n", ret);
		return ret;
	}

	/* Make Message */
	memcpy(m, strm->seq_num_m, HDCP_RP_SEQ_NUM_M_LEN);
	memcpy(&m[HDCP_RP_SEQ_NUM_M_LEN], strm->k, HDCP_RP_K_LEN);
	memcpy(&m[HDCP_RP_SEQ_NUM_M_LEN + HDCP_RP_K_LEN],
		strm->streamid_type, HDCP_RP_STREAMID_TYPE_LEN * strm->stream_num);

	*m_len = HDCP_RP_SEQ_NUM_M_LEN + \
		HDCP_RP_K_LEN + \
		(HDCP_RP_STREAMID_TYPE_LEN * strm->stream_num);

#ifdef HDCP_TX_REPEATER_DEBUG
        hdcp_debug("RepeaterAuth_Stream_Manage(%u)\n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

static int dp_decap_rpauth_stream_ready(uint8_t *m,
					size_t *m_len,
					struct hdcp_tx_ctx *tx_ctx,
					struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t *m_prime;
	int ret;

	m_prime = m;
	ret = dp_rp_verify_stream_ready(m_prime);
	if (ret)
		hdcp_err("dp_rp_verify_stream_ready ret(%d)\n", ret);

	return 0;
}
