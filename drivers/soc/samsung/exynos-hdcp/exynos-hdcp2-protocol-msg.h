/* drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-protocol-msg.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __EXYNOS_HDCP2_PROTOCOL_MSG_H__
#define __EXYNOS_HDCP2_PROTOCOL_MSG_H__

#include <linux/types.h>
#include "exynos-hdcp2-teeif.h"

/* Error */
#define ERR_TLC_CONNECT			0x1001
#define ERR_WRONG_BUFFER		0x1002
#define ERR_WRONG_MESSAGE_LENGTH	0x1003
#define ERR_WRONG_MESSAGE_ID		0x1004
#define ERR_GENERATE_NON_SECKEY		0x1005
#define ERR_FILE_OPEN			0x1006

#define ERR_GENERATE_RTX		0x2001
#define ERR_VERIFY_CERT			0x2002
#define ERR_GENERATE_MASTERKEY		0x2003
#define ERR_COMPUTE_AKE_HMAC		0x2004
#define ERR_COMPARE_AKE_HMAC		0x2005
#define ERR_STORE_MASTERKEY		0x2006
#define ERR_CHECK_SRM			0x2007
#define ERR_WRAP_SRM			0x2008
#define ERR_SET_INFO			0x2009
#define ERR_MEMORY_ALLOCATION		0x200A
#define ERR_GET_TX_INFO			0x200B
#define ERR_SET_RX_INFO			0x200C
#define ERR_SET_RRX			0x200D
#define ERR_FIND_MASTERKEY		0x200E

#define ERR_GENERATE_RN			0x3001
#define ERR_COMPUTE_LC_HMAC		0x3002
#define ERR_COMPARE_LC_HMAC		0x3003

#define ERR_GENERATE_RIV		0x4001
#define ERR_GENERATE_SESSION_KEY	0x4002

#define ERR_HDCP_ENCRYPTION		0x5001

#define ERR_RP_VERIFY_RCVLIST           0x6001
#define ERR_EXCEED_MAX_STREAM           0x6002
#define ERR_VALIDATE_M   		0x6003
#define ERR_SEND_ACK	           	0x6004
#define ERR_RP_GEN_STREAM_MG           	0x6005
#define ERR_RP_INVALID_M_PRIME         	0x6006

#define HDCP_RP_RCV_LIST_PRE_META_LEN   2
#define HDCP_RP_RCV_LIST_META_LEN       23

#define HDCP_PROTO_MSG_ID_LEN   	1
#define HDCP_TX_REPEATER_MAX_STREAM     32
#define STREAM_ELEM_SIZE		5
#define STREAM_INFO_SIZE    		7
#define STREAM_MAX_LEN			1024

enum {
	HDCP_VERSION_2_0 = 0x00,
	HDCP_VERSION_2_1 = 0x01,
	HDCP_VERSION_2_2 = 0x01,
};

enum {
	LC_PRECOMPUTE_NOT_SUPPORT = 0x00,
	LC_PRECOMPUTE_SUPPORT = 0x01,
};

enum msg_id {
	AKE_INIT = 2,
	AKE_SEND_CERT,
	AKE_NO_STORED_KM,
	AKE_STORED_KM,
	AKE_SEND_RRX,
	AKE_SEND_H_PRIME,
	AKE_SEND_PAIRING_INFO,
	LC_INIT,
	LC_SEND_L_PRIME,
	SKE_SEND_EKS,
	REPEATERAUTH_SEND_RECEIVERID_LIST,
	RTT_READY,
	RTT_CHALLENGE,
	REPEATERAUTH_SEND_ACK,
	REPEATERAUTH_STREAM_MANAGE,
	REPEATERAUTH_STREAM_READY,
	RECEIVER_AUTHSTATUS,
	AKE_TRANSMITTER_INFO,
	AKE_RECEIVER_INFO,
};

struct stream_info {
	uint8_t type;
	uint16_t pid;
	uint32_t ctr;
};

struct contents_info {
	uint8_t str_num;
	struct stream_info str_info[HDCP_TX_REPEATER_MAX_STREAM];
};

struct hdcp_rpauth_info {
	uint8_t devs_exd;
	uint8_t cascade_exd;
	uint8_t devs_count;
	uint8_t depth;
	uint8_t hdcp2_down;
	uint8_t hdcp1_down;
	uint8_t rx_info[HDCP_RP_RX_INFO_LEN];
	uint8_t seq_num_v[HDCP_RP_SEQ_NUM_V_LEN];
	uint8_t v_prime[HDCP_RP_HMAC_V_LEN / 2];
	union {
		uint8_t arr[HDCP_RCV_DEVS_COUNT_MAX][HDCP_RCV_ID_LEN];
		uint8_t lst[HDCP_RP_RCVID_LIST_LEN];
	} u_rcvid;
	/* repeater auth result */
	uint8_t v[HDCP_RP_HMAC_V_LEN / 2];
	uint8_t valid;
};

struct dp_stream_info {
	uint16_t stream_num;
	uint8_t streamid[HDCP_RP_MAX_STREAMID_NUM];
	uint8_t seq_num_m[HDCP_RP_SEQ_NUM_M_LEN];
	uint8_t k[HDCP_RP_K_LEN];
	uint8_t streamid_type[HDCP_RP_MAX_STREAMID_TYPE_LEN];
};

union stream_manage {
	struct dp_stream_info dp;
	/* todo: add IIA */
};

struct hdcp_tx_ctx {
	uint8_t version;
	uint8_t lc_precomp;

	/* master key */
	uint8_t wrap_mkey[HDCP_AKE_WKEY_BYTE_LEN];
	uint8_t caps[HDCP_CAPS_BYTE_LEN];
	uint8_t ekh_mkey[HDCP_AKE_EKH_MKEY_BYTE_LEN];
	uint8_t m[HDCP_AKE_M_BYTE_LEN];

	uint8_t rtx[HDCP_AKE_RTX_BYTE_LEN];
	uint8_t rn[HDCP_AKE_RTX_BYTE_LEN];
	uint8_t riv[HDCP_AKE_RTX_BYTE_LEN];
	uint8_t lsb16_hmac[HDCP_HMAC_SHA256_LEN / 2];

	/* session key */
	uint8_t str_ctr[HDCP_STR_CTR_LEN];
	uint8_t input_ctr[HDCP_INPUT_CTR_LEN];

	/* receiver list */
	struct hdcp_rpauth_info rpauth_info;

	/* stream manage info */
	union stream_manage strm;

	/* repeater reauth request */
	uint8_t rp_reauth;

	/* todo: move stream_ctrl */
	struct contents_info stream_ctrl;

	int share_skey;
	uint32_t seq_num_M;
	uint8_t strmsg[HDCP_TX_REPEATER_MAX_STREAM * 8];
	size_t strmsg_len;
};

struct hdcp_rx_ctx {
	uint8_t version;
	uint8_t lc_precomp;
	uint8_t repeater;
	uint8_t caps[HDCP_CAPS_BYTE_LEN]; /* only for DP */
	uint8_t receiver_id[RECEIVER_ID_BYTE_LEN];
	uint8_t rrx[HDCP_AKE_RTX_BYTE_LEN];
	uint8_t cert[HDCP_RX_CERT_LEN];
};

int cap_protocol_msg(uint8_t msg_id,
	uint8_t *msg,
	size_t *msg_len,
	uint32_t lk_type,
	struct hdcp_tx_ctx *tx_ctx,
	struct hdcp_rx_ctx *rx_ctx);

int decap_protocol_msg(uint8_t msg_id,
	uint8_t *msg,
	size_t msg_len,
	uint32_t lk_type,
	struct hdcp_tx_ctx *tx_ctx,
	struct hdcp_rx_ctx *rx_ctx);

int ake_find_masterkey(int *found_km,
		uint8_t *ekh_mkey, size_t ekh_mkey_len,
		uint8_t *m, size_t m_len);

#define check_received_msg(m, m_len, len, func_id) \
	((m == NULL) ? ERR_WRONG_BUFFER : \
	((m_len < len) ? ERR_WRONG_MESSAGE_LENGTH : \
	((m[0] != func_id) ? ERR_WRONG_MESSAGE_ID : 0)))

#endif

int ake_verify_cert(uint8_t *cert, size_t cert_len,
		uint8_t *rrx, size_t rrx_len,
		uint8_t *rx_caps, size_t rx_caps_len);
int ake_generate_masterkey(uint32_t lk_type,
			uint8_t *enc_mkey, size_t elen);
int ake_make_hmac(uint8_t *whmac, uint8_t *wrap_mkey,
			uint8_t *rtx, uint8_t repeater,
			uint8_t *hmac_append);
int ake_make_hmac_with_caps(uint8_t *whmac, uint8_t *wrap_mkey,
			uint8_t *rtx, uint8_t *rrx,
			uint8_t *tx_caps, uint8_t *rx_caps);
int ake_compare_hmac(uint8_t *rx_hmac, size_t rx_hmac_len);
int ake_store_master_key(uint8_t *ekh_mkey, size_t ekh_mkey_len);

int lc_make_hmac(struct hdcp_tx_ctx *tx_ctx,
		  struct hdcp_rx_ctx *rx_ctx, uint32_t lk_type);
int lc_generate_rn(uint8_t *out, size_t out_len);
int lc_compare_hmac(uint8_t *rx_hmac, size_t hmac_len);

int ske_generate_riv(uint8_t *out);
int ske_generate_sessionkey(uint32_t lk_type, uint8_t *enc_skey,
			int share_skey);
