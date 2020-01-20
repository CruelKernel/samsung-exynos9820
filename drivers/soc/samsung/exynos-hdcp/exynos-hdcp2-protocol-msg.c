/*
 * drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-protocol-msg.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>

#include "exynos-hdcp2-config.h"
#include "exynos-hdcp2-protocol-msg.h"
#include "exynos-hdcp2-teeif.h"
#include "exynos-hdcp2.h"
#include "exynos-hdcp2-log.h"
#include "exynos-hdcp2-misc.h"
#include "dp_link/exynos-hdcp2-dplink-protocol-msg.h"

/* HDCP protocol message capulate & decapulate functions for IIA */
static int cap_ake_init(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int cap_ake_transmitter_info(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int cap_ake_no_stored_km(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int cap_ake_stored_km(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int decap_ake_send_cert(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int decap_ake_receiver_info(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int decap_ake_send_rrx(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int decap_ake_send_h_prime(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int decap_ake_send_pairing_info(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int cap_lc_init(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int cap_rtt_challenge(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int decap_lc_send_l_prime(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int decap_rtt_ready(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int cap_ske_send_eks(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int decap_RepeaterAuth_send_ReceiverID_List(uint8_t *m,size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int cap_RepeaterAuth_Send_Ack(uint8_t *m,size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int decap_Receiver_AuthStatus(uint8_t *m,size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int cap_RepeaterAuth_Stream_Manage(uint8_t *m,size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
static int decap_RepeaterAuth_Stream_Ready(uint8_t *m,size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx);
int (*proto_iia[])(uint8_t *, size_t *,
			struct hdcp_tx_ctx *,
			struct hdcp_rx_ctx *) = {
	cap_ake_init,
	decap_ake_send_cert,
	cap_ake_no_stored_km,
	cap_ake_stored_km,
	decap_ake_send_rrx,
	decap_ake_send_h_prime,
	decap_ake_send_pairing_info,
	cap_lc_init,
	decap_lc_send_l_prime,
	cap_ske_send_eks,
	decap_RepeaterAuth_send_ReceiverID_List,
	decap_rtt_ready,
	cap_rtt_challenge,
	cap_RepeaterAuth_Send_Ack,
	cap_RepeaterAuth_Stream_Manage,
	decap_RepeaterAuth_Stream_Ready,
	decap_Receiver_AuthStatus,
	cap_ake_transmitter_info,
	decap_ake_receiver_info,
};

/* HDCP protocol message capulate & decapulate functions for DP_LINK */
extern int (*proto_dp[])(uint8_t *, size_t *,
			struct hdcp_tx_ctx *,
			struct hdcp_rx_ctx *);

int ske_generate_sessionkey(uint32_t lk_type, uint8_t *enc_skey, int share_skey)
{
	int ret;

	ret = teei_generate_skey(lk_type,
			enc_skey, HDCP_SKE_SKEY_LEN,
			share_skey);
	if (ret) {
		hdcp_err("generate_session_key() is failed with %x\n", ret);
		return ERR_GENERATE_SESSION_KEY;
	}

	return 0;
}

int ske_generate_riv(uint8_t *out)
{
	int ret;

	ret = teei_generate_riv(out, HDCP_RTX_BYTE_LEN);
	if (ret) {
		hdcp_err("teei_generate_riv() is failed with %x\n", ret);
		return ERR_GENERATE_NON_SECKEY;
	}

	return 0;
}

int lc_generate_rn(uint8_t *out, size_t out_len)
{
	int ret;

	ret = teei_gen_rn(out, out_len);
	if (ret) {
		hdcp_err("lc_generate_rn() is failed with %x\n", ret);
		return ERR_GENERATE_NON_SECKEY;
	}

	return 0;
}

int lc_make_hmac(struct hdcp_tx_ctx *tx_ctx,
		  struct hdcp_rx_ctx *rx_ctx, uint32_t lk_type)
{
	int ret;

	if ((rx_ctx->version != HDCP_VERSION_2_0) &&
			tx_ctx->lc_precomp && rx_ctx->lc_precomp) {
		ret = teei_gen_lc_hmac(lk_type,
				tx_ctx->lsb16_hmac);
		if (ret) {
			hdcp_err("compute_lc_whmac() is failed with %x\n", ret);
			return ERR_COMPUTE_LC_HMAC;
		}
	} else {
		hdcp_err("LC precomp is not supported\n");
		return ERR_COMPUTE_LC_HMAC;
	}

	return ret;
}

int lc_compare_hmac(uint8_t *rx_hmac, size_t hmac_len)
{
	int ret;

	ret = teei_compare_lc_hmac(rx_hmac, hmac_len);
	if (ret) {
		hdcp_err("compare_lc_hmac_val() is failed with %x\n", ret);
		return ERR_COMPARE_LC_HMAC;
	}

	return ret;
}


static int ake_generate_rtx(uint32_t lk_type, uint8_t *out, size_t out_len)
{
	int ret;

	/* IIA Spec does not use TxCaps */
	ret = teei_gen_rtx(lk_type, out, out_len, NULL, 0);
	if (ret) {
		hdcp_err("generate rtx is failed with 0x%x\n", ret);
		return ERR_GENERATE_NON_SECKEY;
	}

	return 0;
}

static int ake_set_tx_info(struct hdcp_tx_ctx *tx_ctx)
{
	int ret;
	uint8_t version[HDCP_VERSION_LEN];
	uint8_t caps_mask[HDCP_CAPABILITY_MASK_LEN];

	/* Init Version & Precomputation */
	ret = teei_get_txinfo(version, HDCP_VERSION_LEN,
			caps_mask, HDCP_CAPABILITY_MASK_LEN);
	if (ret) {
		hdcp_err("Get Tx info is failed with 0x%x\n", ret);
		return ERR_GET_TX_INFO;
	}
	hdcp_debug("Tx: version(0x%02x) Caps_maks(0x%02x%02x)\n",
		version[0], caps_mask[0], caps_mask[1]);

	tx_ctx->version = version[0];
	tx_ctx->lc_precomp = caps_mask[1];

	/* todo: check seq_num_M init position*/
	tx_ctx->seq_num_M = 0;
	return 0;
}

static int ake_set_rx_info(struct hdcp_rx_ctx *rx_ctx)
{
	int ret;
	uint8_t version[HDCP_VERSION_LEN];
	uint8_t caps_mask[HDCP_CAPABILITY_MASK_LEN];


	version[0] = rx_ctx->version;
	caps_mask[0] = 0x00; /* reserved field in IIA spec */
	caps_mask[1] = rx_ctx->lc_precomp;

	/* Init Version & Precomputation */
	ret = teei_set_rxinfo(version, HDCP_VERSION_LEN,
			caps_mask, HDCP_CAPABILITY_MASK_LEN);
	if (ret) {
		hdcp_err("Get Tx info is failed with 0x%x\n", ret);
		return ERR_SET_RX_INFO;
	}

	return 0;
}

static int ake_set_rrx(uint8_t *rrx, size_t rrx_len)
{
	int ret;

	/* set Rrx value */
	ret = teei_set_rrx(rrx, rrx_len);
	if (ret) {
		hdcp_err("set rrx is failed with 0x%x\n", ret);
		return ERR_SET_RRX;
	}

	return 0;
}

int ake_verify_cert(uint8_t *cert, size_t cert_len,
		uint8_t *rrx, size_t rrx_len,
		uint8_t *rx_caps, size_t rx_caps_len)
{
	int ret;

	ret = teei_verify_cert(cert, cert_len,
				rrx, rrx_len,
				rx_caps, rx_caps_len);
	if (ret) {
		hdcp_err("teei_verify_cert() is failed with %x\n", ret);
		return ERR_VERIFY_CERT;
	}

	return 0;
}

int ake_generate_masterkey(uint32_t lk_type, uint8_t *enc_mkey, size_t elen)
{
	int ret;

	/* Generate Encrypted & Wrapped Master Key */
	ret = teei_generate_master_key(lk_type, enc_mkey, elen);
	if (ret) {
		hdcp_err("generate_master_key() is failed with %x\n", ret);
		return ERR_GENERATE_MASTERKEY;
	}

	return ret;
}

int ake_compare_hmac(uint8_t *rx_hmac, size_t rx_hmac_len)
{
	int ret;

	ret = teei_compare_ake_hmac(rx_hmac, rx_hmac_len);
	if (ret) {
		hdcp_err("teei_compare_hmac() is failed with %x\n", ret);
		return ERR_COMPUTE_AKE_HMAC;
	}

	return 0;
}

int ake_store_master_key(uint8_t *ekh_mkey, size_t ekh_mkey_len)
{
	int ret;

	ret = teei_set_pairing_info(ekh_mkey, ekh_mkey_len);
	if (ret) {
		hdcp_err("teei_store_pairing_info() is failed with %x\n", ret);
		return ERR_STORE_MASTERKEY;
	}

	return 0;
}

int ake_find_masterkey(int *found_km,
		uint8_t *ekh_mkey, size_t ekh_mkey_len,
		uint8_t *m, size_t m_len)
{
	int ret;

	ret = teei_get_pairing_info(ekh_mkey, ekh_mkey_len, m, m_len);
	if (ret) {
		if (ret == E_HDCP_PRO_INVALID_RCV_ID) {
			hdcp_info("RCV id is not found\n");
			*found_km = 0;
			return 0;
		} else {
			*found_km = 0;
			hdcp_err("teei_store_pairing_info() is failed with %x\n", ret);
			return ERR_FIND_MASTERKEY;
		}
	}

	*found_km = 1;

	return 0;
}

static int cap_ake_init(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	size_t rtx_len;
	int ret;

	/* Buffer Check */
	if ((m == NULL) || (m_len == NULL) || (tx_ctx == NULL))
		return ERR_WRONG_BUFFER;

	rtx_len = HDCP_AKE_RTX_BYTE_LEN;

	/* Generate rtx */
	ret = ake_generate_rtx(HDCP_LINK_TYPE_IIA, tx_ctx->rtx, rtx_len);
	if (ret) {
		hdcp_err("failed to generate rtx\n");
		return ERR_GENERATE_RTX;
	}

	/* Make Message */
	m[0] = AKE_INIT;
	memcpy(&m[1], tx_ctx->rtx, rtx_len);
	*m_len = 1 + rtx_len;

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("HDCP: cap_ake_init(%lu) \n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

static int cap_ake_transmitter_info(uint8_t *m, size_t *m_len,
				struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	if ((m == NULL) || (m_len == NULL) || (tx_ctx == NULL))
		return ERR_WRONG_BUFFER;

	/* Set info */
	ret = ake_set_tx_info(tx_ctx);
	if (ret) {
		hdcp_err("failed to set info\n");
		return ERR_SET_INFO;
	}


	/* Make Message */
	m[0] = AKE_TRANSMITTER_INFO;
	m[1] = 0x0;
	m[2] = 0x06;
	m[3] = tx_ctx->version;
	m[4] = 0x0;
	m[5] = tx_ctx->lc_precomp;
	*m_len = 6;

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("cap_ake_transmitter_info(%lu) \n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

static int cap_ake_no_stored_km(uint8_t *m, size_t *m_len,
				struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t enc_mkey[HDCP_AKE_ENCKEY_BYTE_LEN];
	int ret;

	if ((m == NULL) || (m_len == NULL) || (tx_ctx == NULL) ||
	(rx_ctx == NULL))
		return ERR_WRONG_BUFFER;

	/* Verify Certificate */
	/* IIA spec does not define rrx and RxCaps with Cert message */
	ret = ake_verify_cert(rx_ctx->cert, sizeof(rx_ctx->cert),
				NULL, 0,
				&(rx_ctx->repeater), HDCP_RX_CAPS_LEN);
	if (ret)
		/* authentication failure &
		 * aborts the authentication protocol */
		return ret;

	/* Generate/Encrypt master key */
	ret = ake_generate_masterkey(HDCP_LINK_TYPE_IIA,
				enc_mkey, sizeof(enc_mkey));
	/* Generate/Encrypt master key */
	if (ret)
		return ret;

	/* Make Message */
	m[0] = AKE_NO_STORED_KM;
	memcpy(&m[1], enc_mkey, HDCP_AKE_ENCKEY_BYTE_LEN);
	*m_len = 1 + HDCP_AKE_ENCKEY_BYTE_LEN;

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("cap_ake_no_stored_km(%lu)\n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

static int cap_ake_stored_km(uint8_t *m, size_t *m_len,
			struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	int ret;
	int found_km;

	if ((m == NULL) || (m_len == NULL) || (tx_ctx == NULL))
		return ERR_WRONG_BUFFER;

	/* Generate/Encrypt master key */
	ret = ake_find_masterkey(&found_km,
				tx_ctx->ekh_mkey, HDCP_AKE_EKH_MKEY_BYTE_LEN,
				tx_ctx->m, HDCP_AKE_M_BYTE_LEN);
	if (ret || !found_km) {
		hdcp_err("find_masterkey() is failed with 0x%x\n", ret);
		return -1;
	}

	/* Make Message */
	m[0] = AKE_STORED_KM;
	memcpy(&m[1], tx_ctx->ekh_mkey, HDCP_AKE_EKH_MKEY_BYTE_LEN);
	memcpy(&m[1] + HDCP_AKE_EKH_MKEY_BYTE_LEN, tx_ctx->m, HDCP_AKE_M_BYTE_LEN);
	*m_len = 1 + HDCP_AKE_EKH_MKEY_BYTE_LEN + HDCP_AKE_M_BYTE_LEN;

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("cap_ake_stored_km(%lu) \n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

static int decap_ake_send_cert(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = check_received_msg(m, *m_len, 2 + HDCP_RX_CERT_LEN, AKE_SEND_CERT);
	if (ret)
		return ret;

	rx_ctx->repeater = m[1];
	memcpy(rx_ctx->cert, &m[2], HDCP_RX_CERT_LEN);

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("decap_ake_send_cert(%lu) \n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

static int decap_ake_receiver_info(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = check_received_msg(m, *m_len, 6, AKE_RECEIVER_INFO);
	if (ret)
		return ret;

	if ((m[2] < 6) && (m[1] == 0)) {
		hdcp_err("AKE_Receiver_Info, send wrong length\n");
		return ERR_WRONG_MESSAGE_LENGTH;
	}

	rx_ctx->version = m[3];
	rx_ctx->lc_precomp = m[5];

	ret = ake_set_rx_info(rx_ctx);
	if (ret) {
		hdcp_err("AKE set rx info failed. ret(0x%x)\n", ret);
		return ret;
	}

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("decap_ake_receiver_info()\n");
	hdcp_debug("version: 0x%02x\n", rx_ctx->version);
	hdcp_debug("HDCP lc_precomp: 0x%02x\n", rx_ctx->lc_precomp);
#endif
	return 0;
}

static int decap_ake_send_rrx(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	int ret;
	uint8_t rrx[HDCP_RRX_BYTE_LEN] = {0};

	ret = check_received_msg(m, *m_len, 1 + HDCP_RRX_BYTE_LEN, AKE_SEND_RRX);
	if (ret)
		return ret;

	memcpy(rx_ctx->rrx, &m[1], HDCP_RRX_BYTE_LEN);
	memcpy(rrx, &m[1], HDCP_RRX_BYTE_LEN);
	ret = ake_set_rrx(rrx, HDCP_RRX_BYTE_LEN);
	if (ret)
		return ret;

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("decap_ake_send_rrx\n");
	hdcp_hexdump(rx_ctx->rrx, HDCP_RRX_BYTE_LEN);
#endif
	return 0;
}

static int decap_ake_send_h_prime(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = check_received_msg(m, *m_len, 1 + HDCP_HMAC_SHA256_LEN,
				AKE_SEND_H_PRIME);
	if (ret)
		return ret;

	/* compare H == H' */
	ret = ake_compare_hmac(&m[1], HDCP_HMAC_SHA256_LEN);
	if (ret)
		return ret;

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("decap_ake_send_h_prime\n");
	hdcp_debug("given hmac:\n");
	hdcp_hexdump(&m[1], HDCP_HMAC_SHA256_LEN);
#endif
	return 0;
}

static int decap_ake_send_pairing_info(uint8_t *m, size_t *m_len,
				struct hdcp_tx_ctx *tx_ctx,
				struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = check_received_msg(m, *m_len, 1 + HDCP_AKE_EKH_MKEY_BYTE_LEN,
				AKE_SEND_PAIRING_INFO);
	if (ret)
		return ret;

	memcpy(tx_ctx->ekh_mkey, &m[1], HDCP_AKE_EKH_MKEY_BYTE_LEN);

	/* Store the Key */
	ret = ake_store_master_key(tx_ctx->ekh_mkey, HDCP_AKE_EKH_MKEY_BYTE_LEN);
	if (ret)
		return ret;

#ifdef HDCP_AKE_DEBUG
	hdcp_debug("decap_ake_send_pairing_info(%lu)\n", *m_len);
	hdcp_debug("rx_ctx->ekh_mkey:\n");
	hdcp_hexdump(tx_ctx->ekh_mkey, HDCP_AKE_MKEY_BYTE_LEN);
#endif

	return 0;
}

static int cap_lc_init(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
		struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	if ((m == NULL) || (m_len == NULL) || (tx_ctx == NULL) || (rx_ctx ==
	NULL))
		return ERR_WRONG_BUFFER;

	/* Generate rn */
	ret = lc_generate_rn(tx_ctx->rn, HDCP_RTX_BYTE_LEN);
	if (ret) {
		hdcp_err("failed to generate rtx\n");
		return ERR_GENERATE_RN;
	}

	/* Make Message */
	m[0] = LC_INIT;
	memcpy(&m[1], tx_ctx->rn, HDCP_RTX_BYTE_LEN);
	*m_len = 1 + HDCP_RTX_BYTE_LEN;

	if ((rx_ctx->version != HDCP_VERSION_2_0) &&
		tx_ctx->lc_precomp &&
		rx_ctx->lc_precomp) {
		/* compute HMAC,
		 * return the least significant 128-bits,
		 * the most significant 128-bits wrapped */
		ret = lc_make_hmac(tx_ctx, rx_ctx, 0); // last param 0 have to be checked, PKY
		if (ret)
			return ret;
	}

#ifdef HDCP_LC_DEBUG
	hdcp_debug("LC_Init(%u)\n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

static int cap_rtt_challenge(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	if ((m == NULL) || (m_len == NULL) || (tx_ctx == NULL))
		return ERR_WRONG_BUFFER;

	/* Make Message */
	m[0] = RTT_CHALLENGE;
	memcpy(&m[1], tx_ctx->lsb16_hmac, HDCP_HMAC_SHA256_LEN / 2);
	*m_len = 1 + (HDCP_HMAC_SHA256_LEN / 2);

#ifdef HDCP_LC_DEBUG
	hdcp_debug("RTT_Challenge(%lu)\n", *m_len);
	hdcp_hexdump(m, *m_lne);
#endif

	return 0;
}

static int decap_lc_send_l_prime(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
			struct hdcp_rx_ctx *rx_ctx)
{
	uint8_t *rx_hmac;
	size_t len;
	int ret;

	/* find length depending precomputation */
	if ((rx_ctx->version == HDCP_VERSION_2_0) || !tx_ctx->lc_precomp ||
	!rx_ctx->lc_precomp)
		len = HDCP_HMAC_SHA256_LEN;
	else
		len = HDCP_HMAC_SHA256_LEN/2;

	ret = check_received_msg(m, *m_len, 1 + len, LC_SEND_L_PRIME);
	if (ret)
		return ret;

	/* No_precomputation: compare hmac
	   precomputation: compare the most significant 128bits of L & L' */
	rx_hmac = &m[1];
	ret = lc_compare_hmac(rx_hmac, len);
	if (ret)
		return ret;

#ifdef HDCP_LC_DEBUG
	hdcp_debug("LC_Send_L_prime\n");
	hdcp_debug("rx_hmac:\n");
	hdcp_hexdump(rx_hmac, len);
#endif

	return 0;
}

static int decap_rtt_ready(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
		struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = check_received_msg(m, *m_len, 1, RTT_READY);
	if (ret)
		return ret;

#ifdef HDCP_LC_DEBUG
	hdcp_debug("RTT_Ready\n");
#endif

	return 0;
}

static int cap_ske_send_eks(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
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
	ret = ske_generate_sessionkey(0, enc_skey, tx_ctx->share_skey);
	if (ret)
		return ret;

	/* Make Message */
	m[0] = SKE_SEND_EKS;
	memcpy(&m[1], enc_skey, HDCP_AKE_MKEY_BYTE_LEN);
	memcpy(&m[1 + HDCP_AKE_MKEY_BYTE_LEN], tx_ctx->riv, HDCP_RTX_BYTE_LEN);
	*m_len = 1 + HDCP_AKE_MKEY_BYTE_LEN + HDCP_RTX_BYTE_LEN;

#ifdef HDCP_SKE_DEBUG
	hdcp_debug("SKE_Send_Eks(%lu)\n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

#ifdef TEST_HDCP_V2_0
int parse_rcvid_list(uint8_t *msg, struct hdcp_tx_ctx *tx_ctx)
{
        /* get PRE META values */
        tx_ctx->rcv_list.devs_exd = (uint8_t)*msg;
        tx_ctx->rcv_list.cascade_exd = (uint8_t)*(msg + 1);

        /* get META values */
        msg += HDCP_RP_RCV_LIST_PRE_META_LEN;
        tx_ctx->rcv_list.devs_count = (uint8_t)*msg;
        tx_ctx->rcv_list.depth = (uint8_t)*(msg + 1);
        memcpy(tx_ctx->rcv_list.hmac_prime, (uint8_t *)(msg + 2), 32);

        /* get receiver ID list */
        msg += 34;
        memcpy(tx_ctx->rcv_list.rcv_id, msg, tx_ctx->rcv_list.devs_count * HDCP_RCV_ID_LEN);

	return 0;
}

void convert_rcvlist2authmsg(struct hdcp_rcvlist *rcv_list, uint8_t *src_msg, size_t *msg_len)
{
        int i;
        *msg_len = 0;

        for (i = 0; i < rcv_list->devs_count; i++) {
                memcpy(src_msg + *msg_len, rcv_list->rcv_id[i], HDCP_RCV_ID_LEN);
                *msg_len += HDCP_RCV_ID_LEN;
        }

        /* concatinate DEPTH */
        memcpy(src_msg + *msg_len, &rcv_list->depth, 1);
        *msg_len += 1;

        /* concatinate DEVICE COUNT */
        memcpy(src_msg + *msg_len, &rcv_list->devs_count, 1);
        *msg_len += 1;

        /* concatinate MAX DEVS EXCEEDED */
        memcpy(src_msg + *msg_len, &rcv_list->devs_exd, 1);
        *msg_len += 1;

        /* concatinate MAX CASCADE EXCEEDED */
        memcpy(src_msg + *msg_len, &rcv_list->cascade_exd, 1);
        *msg_len += 1;
}
#else
int parse_rcvid_list(uint8_t *msg, struct hdcp_tx_ctx *tx_ctx)
{
        /* get PRE META values */
        tx_ctx->rpauth_info.devs_exd = (uint8_t)*msg;
        tx_ctx->rpauth_info.cascade_exd = (uint8_t)*(msg + 1);

        /* get META values */
        msg += HDCP_RP_RCV_LIST_PRE_META_LEN;
        tx_ctx->rpauth_info.devs_count = (uint8_t)*msg;
        tx_ctx->rpauth_info.depth = (uint8_t)*(msg + 1);
        tx_ctx->rpauth_info.hdcp2_down = (uint8_t)*(msg + 2);
        tx_ctx->rpauth_info.hdcp1_down = (uint8_t)*(msg + 3);
        memcpy(tx_ctx->rpauth_info.seq_num_v, (uint8_t *)(msg + 4), 3);
        memcpy(tx_ctx->rpauth_info.v_prime, (uint8_t *)(msg + 7), 16);

        /* get receiver ID list */
        msg += HDCP_RP_RCV_LIST_META_LEN;
	if (tx_ctx->rpauth_info.devs_count > HDCP_RCV_DEVS_COUNT_MAX) {
		hdcp_err("invalid DEVS count (%d)\n", tx_ctx->rpauth_info.devs_count);
		return -1;
	}

        memcpy(tx_ctx->rpauth_info.u_rcvid.arr, msg, tx_ctx->rpauth_info.devs_count * HDCP_RCV_ID_LEN);

	return 0;
}

void convert_rcvlist2authmsg(struct hdcp_rpauth_info *rpauth_info, uint8_t *src_msg, size_t *msg_len)
{
        int i;
        *msg_len = 0;

        for (i = 0; i < rpauth_info->devs_count; i++) {
                memcpy(src_msg + *msg_len, rpauth_info->u_rcvid.arr[i], HDCP_RCV_ID_LEN);
                *msg_len += HDCP_RCV_ID_LEN;
        }

        /* concatinate DEPTH */
        memcpy(src_msg + *msg_len, &rpauth_info->depth, 1);
        *msg_len += 1;

        /* concatinate DEVICE COUNT */
        memcpy(src_msg + *msg_len, &rpauth_info->devs_count, 1);
        *msg_len += 1;

        /* concatinate MAX DEVS EXCEEDED */
        memcpy(src_msg + *msg_len, &rpauth_info->devs_exd, 1);
        *msg_len += 1;

        /* concatinate MAX CASCADE EXCEEDED */
        memcpy(src_msg + *msg_len, &rpauth_info->cascade_exd, 1);
        *msg_len += 1;

        /* concatinate HDCP2 REPEATER DOWNSTREAM */
        memcpy(src_msg + *msg_len, &rpauth_info->hdcp2_down, 1);
        *msg_len += 1;

        /* concatinate HDCP1 DEVICE DOWNSTREAM */
        memcpy(src_msg + *msg_len, &rpauth_info->hdcp1_down, 1);
        *msg_len += 1;

        /* concatinate seq_num_v */
        memcpy(src_msg + *msg_len, &rpauth_info->seq_num_v, 3);
        *msg_len += 3;
}
#endif

static int decap_RepeaterAuth_send_ReceiverID_List(uint8_t *m,
		size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
		struct hdcp_rx_ctx *rx_ctx)
{
	size_t hmac_prime_len;
	size_t msg_len;
	uint8_t source_msg[256];
	int ret;

	if (rx_ctx->version == HDCP_VERSION_2_0)
		hmac_prime_len = HDCP_HMAC_SHA256_LEN;
	else
		hmac_prime_len = HDCP_HMAC_SHA256_LEN/2;

	ret = check_received_msg(m, *m_len, 0, REPEATERAUTH_SEND_RECEIVERID_LIST);
	if (ret)
		return ret;

	if (parse_rcvid_list(m + 1, tx_ctx))
		return -1;

	convert_rcvlist2authmsg(&tx_ctx->rpauth_info, source_msg, &msg_len);

	ret = teei_set_rcvlist_info(NULL, NULL, tx_ctx->rpauth_info.v_prime,
			source_msg, tx_ctx->rpauth_info.v, &(tx_ctx->rpauth_info.valid));

	tx_ctx->rpauth_info.valid = 0;
	if (ret) {
		tx_ctx->rpauth_info.valid = 1;
		return ret;
	}

#ifdef HDCP_TX_REPEATER_DEBUG
	hdcp_debug("decap_RepeaterAuth_send_ReceiverID_List valid (%u)\n", tx_ctx->rpauth_info.valid);
	hdcp_hexdump(tx_ctx->rpauth_info.v, HDCP_RP_HMAC_V_LEN / 2);
#endif
	return 0;
}

static int cap_RepeaterAuth_Send_Ack(uint8_t *m, size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
		struct hdcp_rx_ctx *rx_ctx)
{
	if ((m == NULL) || (m_len == NULL) || (tx_ctx == NULL))
		return ERR_WRONG_BUFFER;

	/* Make Message */
	m[0] = REPEATERAUTH_SEND_ACK;
	if(tx_ctx->rpauth_info.valid == 0)
		memcpy(&m[1], tx_ctx->rpauth_info.v, HDCP_RP_HMAC_V_LEN / 2);
	else
		return ERR_SEND_ACK;

	*m_len = 1 + (HDCP_RP_HMAC_V_LEN / 2);

#ifdef HDCP_TX_REPEATER_DEBUG
	hdcp_debug("make_RepeaterAuth_Send_Ack(%u)\n", *m_len);
	hdcp_hexdump(m, *m_len);
#endif
	return 0;
}

static int decap_Receiver_AuthStatus(uint8_t *m,
		size_t *m_len, struct hdcp_tx_ctx *tx_ctx,
		struct hdcp_rx_ctx *rx_ctx)
{
	int ret;

	ret = check_received_msg(m, (int)*m_len, 0,  RECEIVER_AUTHSTATUS);
	if (ret)
		return ret;

	tx_ctx->rp_reauth = m[3];
#ifdef HDCP_TX_REPEATER_DEBUG
	hdcp_debug("get_Receiver_AuthStatus(%u)\n", *m_len);
	hdcp_debug("receiver reauth req: %u\n", tx_ctx->rp_reauth);
#endif
	return 0;
}

int cap_RepeaterAuth_Stream_Manage(uint8_t *m, size_t *m_len,
		struct hdcp_tx_ctx *tx_ctx,
		struct hdcp_rx_ctx *rx_ctx)
{
	int i;
	uint8_t *dst;
	uint16_t stmp;
	uint32_t itmp;

	if ((m == NULL) || (m_len == NULL) || (tx_ctx == NULL))
		return ERR_WRONG_BUFFER;

	if (tx_ctx->stream_ctrl.str_num > HDCP_TX_REPEATER_MAX_STREAM)
		return ERR_EXCEED_MAX_STREAM;

	/* Make Message */
	m[0] = REPEATERAUTH_STREAM_MANAGE;
	itmp = htonl(tx_ctx->seq_num_M);
	memcpy(&m[1], (uint8_t *)&itmp + 1, 3);
	stmp = htons(tx_ctx->stream_ctrl.str_num);
	memcpy(&m[4], &stmp, sizeof(stmp));

	for (i = 0; i < tx_ctx->stream_ctrl.str_num; i++) {
		dst = (uint8_t *)(&m[6] + STREAM_INFO_SIZE * i);
		itmp = htonl(tx_ctx->stream_ctrl.str_info[i].ctr);
		memcpy(dst, &itmp, sizeof(itmp));
		stmp = htons(tx_ctx->stream_ctrl.str_info[i].pid);
		memcpy(dst + sizeof(itmp), &stmp, sizeof(stmp));
		dst[STREAM_INFO_SIZE - 1] = tx_ctx->stream_ctrl.str_info[i].type;
	}

	*m_len = HDCP_PROTO_MSG_ID_LEN + STREAM_ELEM_SIZE + STREAM_INFO_SIZE * i;

	/* seq_num_M++ */
	tx_ctx->seq_num_M++;

	/* save message to make M */
	memcpy(tx_ctx->strmsg, &m[6], *m_len - 6);
	memcpy(tx_ctx->strmsg + *m_len - 6, &m[1], 3);
	tx_ctx->strmsg_len = *m_len - 3;

#ifdef HDCP_TX_REPEATER_DEBUG
	hdcp_debug("strmsg(len: %d): \n", tx_ctx->strmsg_len);
	hdcp_hexdump(tx_ctx->strmsg, tx_ctx->strmsg_len);

	hdcp_debug("message(len: %d): \n", *m_len);
	hdcp_hexdump(m, *m_len);

	hdcp_debug("seq_num_M: %d\n", tx_ctx->seq_num_M);
#endif
	return 0;
}

int decap_RepeaterAuth_Stream_Ready(uint8_t *m, size_t *m_len,
		struct hdcp_tx_ctx *tx_ctx,
		struct hdcp_rx_ctx *rx_ctx)
{
	int ret = 0;
	/* Not support yet*/
#if 0
	ret = check_received_msg(m, *m_len, 1 + HDCP_HMAC_SHA256_LEN, REPEATERAUTH_STREAM_READY);
	if (ret)
		return ret;

	/* compute M and compare M == M' */
	ret = teei_verify_m_prime(&m[1], tx_ctx->strmsg, tx_ctx->strmsg_len);
	if (ret)
		return ret;
#endif
	return ret;
}

int cap_protocol_msg(uint8_t msg_id,
		uint8_t *msg,
		size_t *msg_len,
		uint32_t lk_type,
		struct hdcp_tx_ctx *tx_ctx,
		struct hdcp_rx_ctx *rx_ctx)
{
	/* todo: check upper boundary */
	int ret = 0;
	int (**proto_func)(uint8_t *, size_t *,
			struct hdcp_tx_ctx *,
			struct hdcp_rx_ctx *);

	if (lk_type == HDCP_LINK_TYPE_IIA)
		proto_func = proto_iia;
#if defined(CONFIG_HDCP2_DP_ENABLE)
	else if(lk_type == HDCP_LINK_TYPE_DP)
		proto_func = proto_dp;
#endif
	else {
		hdcp_err("invalid link type(%d)\n", lk_type);
		return -1;
	}

	if (msg_id > 1) {
		ret = proto_func[msg_id - 2](msg, msg_len, tx_ctx, rx_ctx);
		return ret;
	}
	else
		return -1;
}

int decap_protocol_msg(uint8_t msg_id,
		uint8_t *msg,
		size_t msg_len,
		uint32_t lk_type,
		struct hdcp_tx_ctx *tx_ctx,
		struct hdcp_rx_ctx *rx_ctx)
{
	int ret = 0;
	int (**proto_func)(uint8_t *, size_t *,
			struct hdcp_tx_ctx *,
			struct hdcp_rx_ctx *);

	/* todo: check upper boundary */
	if (lk_type == HDCP_LINK_TYPE_IIA)
		proto_func = proto_iia;
#if defined(CONFIG_HDCP2_DP_ENABLE)
	else if(lk_type == HDCP_LINK_TYPE_DP)
		proto_func = proto_dp;
#endif
	else {
		hdcp_err("HDCP: invalid link type(%d)\n", lk_type);
		return -1;
	}

	if (msg_id > 1) {
		ret = proto_func[msg_id - 2](msg, &msg_len, tx_ctx, rx_ctx);
		return ret;
	}

	else
		return -1;
}
