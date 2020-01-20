/* drivers/soc/samsung/exynos-hdcp/dplink/exynos-hdcp2-dplink-auth.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/delay.h>
#include "../exynos-hdcp2.h"
#include "../exynos-hdcp2-misc.h"
#include "../exynos-hdcp2-log.h"
#include "exynos-hdcp2-dplink-auth.h"
#include "exynos-hdcp2-dplink-if.h"
#include "exynos-hdcp2-dplink.h"
#include "exynos-hdcp2-dplink-inter.h"

#define MAX_LC_RETRY 10

static uint8_t pairing_ready;
static uint8_t hprime_ready;
uint8_t rp_ready;
uint8_t rp_ready_s;
static uint8_t reauth_req;
static uint8_t integrity_fail;

static char *hdcp_msgid_str[] = {
       NULL,
       "Null message",
       "DP_AKE_Init",
       "DP_AKE_Send_Cert",
       "DP_AKE_No_Stored_km",
       "DP_AKE_Stored_km",
       "DP_AKE_Send_rrx",
       "DP_AKE_Send_H_prime",
       "DP_AKE_Send_Pairing_Info",
       "DP_LC_Init",
       "DP_LC_Send_L_prime",
       "DP_SKE_Send_Eks",
       "DP_RepeaterAuth_Send_ReceiverID_List",
       "DP_RTT_Ready",
       "DP_RTT_Challenge",
       "DP_RepeaterAuth_Send_Ack",
       "DP_RepeaterAuth_Stream_Manage",
       "DP_RepeaterAuth_Stream_Ready",
       "DP_Receiver_AuthStatus",
       "DP_AKE_Transmitter_Info",
       "DPAKE_Receiver_Info",
       NULL
};

struct dp_ake_init {
	uint8_t rtx[HDCP_AKE_RTX_BYTE_LEN];
	uint8_t txcaps[HDCP_CAPS_BYTE_LEN];
};

struct dp_ake_send_cert {
	uint8_t cert_rx[HDCP_RX_CERT_LEN];
	uint8_t rrx[HDCP_RRX_BYTE_LEN];
	uint8_t rxcaps[HDCP_CAPS_BYTE_LEN];
};

struct dp_ake_no_stored_km {
	uint8_t ekpub_km[HDCP_AKE_ENCKEY_BYTE_LEN];
};

struct dp_ake_stored_km {
	uint8_t ekh_km[HDCP_AKE_EKH_MKEY_BYTE_LEN];
	uint8_t m[HDCP_AKE_M_BYTE_LEN];
};

struct dp_ake_send_h_prime {
	uint8_t h_prime[HDCP_HMAC_SHA256_LEN];
};

struct dp_ake_send_pairing_info {
	uint8_t ekh_km[HDCP_AKE_MKEY_BYTE_LEN];
};

struct dp_lc_init {
	uint8_t rn[HDCP_RTX_BYTE_LEN];
};

struct dp_lc_send_l_prime {
	uint8_t l_prime[HDCP_HMAC_SHA256_LEN];
};

struct dp_ske_send_eks {
	uint8_t edkey_ks[HDCP_AKE_MKEY_BYTE_LEN];
	uint8_t riv[HDCP_RTX_BYTE_LEN];
};

struct dp_rp_send_rcvid_list {
	uint8_t rx_info[HDCP_RP_RX_INFO_LEN];
	uint8_t seq_num_v[HDCP_RP_SEQ_NUM_V_LEN];
	uint8_t v_prime[HDCP_RP_HMAC_V_LEN / 2];
	uint8_t rcvid_list[HDCP_RP_RCVID_LIST_LEN];
};

struct dp_rp_send_ack {
	uint8_t v[HDCP_RP_HMAC_V_LEN / 2];
};

struct dp_rp_stream_manage {
	uint8_t seq_num_m[HDCP_RP_SEQ_NUM_M_LEN];
	uint8_t k[HDCP_RP_K_LEN];
	uint8_t streamid_type[HDCP_RP_MAX_STREAMID_TYPE_LEN];
};

struct dp_rp_stream_ready {
	uint8_t m_prime[HDCP_RP_HMAC_M_LEN];
};

struct rxinfo {
	uint8_t depth;
	uint8_t dev_count;
	uint8_t max_dev_exd;
	uint8_t max_cascade_exd;
	uint8_t hdcp20_downstream;
	uint8_t hdcp1x_downstream;
};

static uint16_t dp_htons(uint16_t x)
{
	return (
		((x & 0x00FF) << 8) |
		((x & 0xFF00) >> 8)
	);
}

static void rxinfo_convert_arr2st(uint8_t *arr, struct rxinfo *st)
{
	uint16_t rxinfo_val;

	memcpy((uint8_t *)&rxinfo_val, arr, sizeof(rxinfo_val));
	/* convert to little endian */
	rxinfo_val = dp_htons(rxinfo_val);

	st->depth = (rxinfo_val >> DEPTH_SHIFT) & DEPTH_MASK;
	st->dev_count = (rxinfo_val >> DEV_COUNT_SHIFT) & DEV_COUNT_MASK;
	st->max_dev_exd = (rxinfo_val >> DEV_EXD_SHIFT) & DEV_EXD_MASK;
	st->max_cascade_exd = (rxinfo_val >> CASCADE_EXD_SHIFT) & CASCADE_EXD_MASK;
	st->hdcp20_downstream = (rxinfo_val >> HDCP20_DOWN_SHIFT) & HDCP20_DOWN_MASK;
	st->hdcp1x_downstream = (rxinfo_val >> HDCP1X_DOWN_SHIFT) & HDCP1X_DOWN_MASK;
}

#if defined(HDCP_DEBUG)
static void rxinfo_print_all_info(struct rxinfo *rx)
{
	hdcp_info("RxInfo:\n");
	hdcp_info("rxinfo.depth(%d)\n", rx->depth);
	hdcp_info("rxinfo.dev_count(%d)\n", rx->dev_count);
	hdcp_info("rxinfo.dev_exd(%d)\n", rx->max_dev_exd);
	hdcp_info("rxinfo.cascade_exd(%d)\n", rx->max_cascade_exd);
	hdcp_info("rxinfo.hdcp20_down(%d)\n", rx->hdcp20_downstream);
	hdcp_info("rxinfo.hdcp1x_down(%d)\n", rx->hdcp1x_downstream);
}
#endif
static int is_auth_aborted(void)
{
	/* todo: need mutex */
	if (integrity_fail || reauth_req) {
		/* clear flag */
		dplink_clear_irqflag_all();
		hdcp_err("Authentication is aborted\n");
		return 1;
	}

	return 0;
}

static int dp_send_protocol_msg(struct hdcp_link_data *lk, uint8_t msg_id, struct hdcp_msg_info *msg_info)
{
        int ret = TX_AUTH_SUCCESS;

        hdcp_info("Tx->Rx: %s\n", hdcp_msgid_str[msg_id]);
	ret = cap_protocol_msg(msg_id,
			msg_info->msg,
			(size_t *)&msg_info->msg_len,
			HDCP_LINK_TYPE_DP,
			&lk->tx_ctx,
			&lk->rx_ctx);
        if (ret) {
                hdcp_err("cap_protocol_msg() failed. ret(0x%08x)\n", ret);
                return -TX_AUTH_ERROR_MAKE_PROTO_MSG;
        }

        return TX_AUTH_SUCCESS;
}

int dp_recv_protocol_msg(struct hdcp_link_data *lk, uint8_t msg_id, struct hdcp_msg_info *msg_info)
{
	int ret = TX_AUTH_SUCCESS;

	if (msg_info->msg_len > 0) {
		/* parsing received message */
		ret = decap_protocol_msg(msg_id,
				msg_info->msg,
				msg_info->msg_len,
				HDCP_LINK_TYPE_DP,
				&lk->tx_ctx,
				&lk->rx_ctx);
		if (ret) {
			hdcp_err("decap_protocol_msg() failed. msg_id(%d), ret(0x%08x)\n", msg_id, ret);
			ret = -TX_AUTH_ERROR_WRONG_MSG;
		}
	}

	hdcp_info("Rx->Tx: %s\n", hdcp_msgid_str[msg_id]);

	return ret;
}

static int dp_ake_find_masterkey(int *found)
{
	int ret = TX_AUTH_SUCCESS;
	uint8_t ekh_mkey[HDCP_AKE_EKH_MKEY_BYTE_LEN] = {0};
	uint8_t m[HDCP_AKE_M_BYTE_LEN] = {0};

	ret = ake_find_masterkey(found,
		ekh_mkey, HDCP_AKE_EKH_MKEY_BYTE_LEN,
		m, HDCP_AKE_M_BYTE_LEN);
	if (ret) {
		hdcp_err("fail to find stored km: ret(%d)\n", ret);
		return -1;
	}

	return 0;
}

static int dp_get_hdcp_session_key(struct hdcp_link_data *lk)
{
	return 0;
}

static int dp_put_hdcp_session_key(struct hdcp_link_data *lk)
{
	return 0;
}

static int do_send_ake_init(struct hdcp_link_data *lk)
{
	int ret;
	struct hdcp_msg_info msg_info;
	struct dp_ake_init *m_init;

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	ret = dp_send_protocol_msg(lk, DP_AKE_INIT, &msg_info);
	if (ret < 0) {
		hdcp_err("AKE_Init failed: ret(%d)\n", ret);
		return -1;
	}

	m_init = (struct dp_ake_init *)msg_info.msg;
	ret = hdcp_dplink_send(HDCP22_MSG_RTX_W,
			m_init->rtx,
			sizeof(m_init->rtx));
	if (ret) {
		hdcp_err("rtx send fail: ret(%d)\n", ret);
		return -1;
	}

	ret = hdcp_dplink_send(HDCP22_MSG_TXCAPS_W,
			m_init->txcaps,
			sizeof(m_init->txcaps));
	if (ret) {
		hdcp_err("txcaps send fail: ret(%d)\n", ret);
		return -1;
	}

	return 0;
}

void parse_rxcaps_info(uint8_t *rxcaps, struct hdcp_link_data *lk)
{
	memcpy(lk->rx_ctx.caps, rxcaps, sizeof(lk->rx_ctx.caps));
	if (rxcaps[2] & DP_RXCAPS_REPEATER) {
		hdcp_info("Rx is Repeater. rxcaps(0x%02x%02x%02x)\n",
			rxcaps[0], rxcaps[1], rxcaps[2]);
		lk->rx_ctx.repeater = 1;
	} else {
		hdcp_info("Rx is NOT Repeater. rxcaps(0x%02x%02x%02x)\n",
			rxcaps[0], rxcaps[1], rxcaps[2]);
		lk->rx_ctx.repeater = 0;
	}
}

static int do_recv_ake_send_cert(struct hdcp_link_data *lk)
{
	int ret;
	struct hdcp_msg_info msg_info;
	struct dp_ake_send_cert m_send_cert;

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	ret = hdcp_dplink_recv(HDCP22_MSG_CERT_RX_R,
			m_send_cert.cert_rx,
			sizeof(m_send_cert.cert_rx));
	if (ret) {
		hdcp_err("ake_send_cert cert recv fail. ret(%d)\n", ret);
		return -1;
	}

#if defined(HDCP_DEBUG)
	hdcp_debug("rx cert:\n");
	hdcp_hexdump(m_send_cert.cert_rx, sizeof(m_send_cert.cert_rx));
#endif
	ret = hdcp_dplink_recv(HDCP22_MSG_RRX_R,
			m_send_cert.rrx,
			sizeof(m_send_cert.rrx));
	if (ret) {
		hdcp_err("HDCP : ake_send_cert rrx recv fail: ret(%d)\n", ret);
		return -1;
	}

#if defined(HDCP_DEBUG)
	hdcp_debug("rx rrx:\n");
	hdcp_hexdump(m_send_cert.rrx,  sizeof(m_send_cert.rrx));
#endif
	ret = hdcp_dplink_recv(HDCP22_MSG_RXCAPS_R,
			m_send_cert.rxcaps,
			sizeof(m_send_cert.rxcaps));
	if (ret) {
		hdcp_err("ake_send_cert rxcaps recv fail: ret(%d)\n", ret);
		return -1;
	}

	parse_rxcaps_info(m_send_cert.rxcaps, lk);

#if defined(HDCP_DEBUG)
	hdcp_debug("rx caps\n");
	hdcp_hexdump(m_send_cert.rxcaps,  sizeof(m_send_cert.rxcaps));
#endif
	memcpy(msg_info.msg, &m_send_cert, sizeof(struct dp_ake_send_cert));
	msg_info.msg_len = sizeof(struct dp_ake_send_cert);
	ret = dp_recv_protocol_msg(lk, DP_AKE_SEND_CERT, &msg_info);
	if (ret < 0) {
		hdcp_err("recv AKE_Send_Cert failed\n");
		return -1;
	}

	return 0;
}

static int do_send_ake_nostored_km(struct hdcp_link_data *lk)
{
	int ret;
	struct hdcp_msg_info msg_info;
	struct dp_ake_no_stored_km *m_nostored_km;

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	ret = dp_send_protocol_msg(lk, DP_AKE_NO_STORED_KM, &msg_info);
	if (ret < 0) {
		hdcp_err("send AKE_No_Stored_km failed. ret(%d)\n", ret);
		return -1;
	}

	m_nostored_km = (struct dp_ake_no_stored_km *)msg_info.msg;
	ret = hdcp_dplink_send(HDCP22_MSG_EKPUB_KM_W,
			m_nostored_km->ekpub_km,
			sizeof(m_nostored_km->ekpub_km));
	if (ret) {
		hdcp_err("ake_no_stored_km send fail: ret(%d)\n", ret);
		return -1;
	}

	return 0;
}

static int do_send_ake_stored_km(struct hdcp_link_data *lk)
{
	int ret;
	struct hdcp_msg_info msg_info;
	struct dp_ake_stored_km *m_stored_km;

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	ret = dp_send_protocol_msg(lk, DP_AKE_STORED_KM, &msg_info);
	if (ret < 0) {
		hdcp_err("send AKE_stored_km failed. ret(%d)\n", ret);
		return -1;
	}

	m_stored_km = (struct dp_ake_stored_km *)msg_info.msg;
	ret = hdcp_dplink_send(HDCP22_MSG_EKH_KM_W,
			m_stored_km->ekh_km,
			sizeof(m_stored_km->ekh_km));
	if (ret) {
		hdcp_err("ake_stored_km send fail: ret(%d)\n", ret);
		return -1;
	}

	ret = hdcp_dplink_send(HDCP22_MSG_M_W,
			m_stored_km->m,
			sizeof(m_stored_km->m));
	if (ret) {
		hdcp_err("ake_stored_km send fail: ret(%d)\n", ret);
		return -1;
	}

	return 0;
}

static int do_recv_ake_send_h_prime(struct hdcp_link_data *lk)
{
	int ret;
	struct hdcp_msg_info msg_info;
	struct dp_ake_send_h_prime m_hprime;

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	ret = hdcp_dplink_recv(HDCP22_MSG_HPRIME_R,
			m_hprime.h_prime,
			sizeof(m_hprime.h_prime));
	if (ret) {
		hdcp_err("ake_send_h_prime recv fail: ret(%d)\n", ret);
		return -1;
	}

#if defined(HDCP_DEBUG)
	hdcp_debug("h_prime\n");
	hdcp_hexdump(m_hprime.h_prime, sizeof(m_hprime.h_prime));
#endif

	memcpy(msg_info.msg, &m_hprime, sizeof(struct dp_ake_send_h_prime));
	msg_info.msg_len = sizeof(struct dp_ake_send_h_prime);
	ret = dp_recv_protocol_msg(lk, DP_AKE_SEND_H_PRIME, &msg_info);
	if (ret < 0) {
		hdcp_err("recv AKE_Send_H_Prime failed. ret(%d)\n", ret);
		return -1;
	}

	return 0;
}

static int do_recv_ake_send_pairing_info(struct hdcp_link_data *lk)
{
	int ret;
	struct hdcp_msg_info msg_info;
	struct dp_ake_send_pairing_info m_pairing;

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	ret = hdcp_dplink_recv(HDCP22_MSG_EKH_KM_R,
			m_pairing.ekh_km,
			sizeof(m_pairing.ekh_km));
	if (ret) {
		hdcp_err("ake_send_pairing_info recv fail: ret(%d)\n", ret);
		return -1;
	}

	memcpy(msg_info.msg, &m_pairing, sizeof(struct dp_ake_send_pairing_info));
	msg_info.msg_len = sizeof(struct dp_ake_send_pairing_info);
	ret = dp_recv_protocol_msg(lk, DP_AKE_SEND_PAIRING_INFO, &msg_info);
	if (ret < 0) {
		hdcp_err("recv AKE_Send_Pairing_Info failed. ret(%d)\n", ret);
		return -1;
	}

	return 0;
}

static int do_send_lc_init(struct hdcp_link_data *lk)
{
	int ret;
	struct hdcp_msg_info msg_info;
	struct dp_lc_init *m_lc_init;

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	ret = dp_send_protocol_msg(lk, DP_LC_INIT, &msg_info);
	if (ret < 0) {
		hdcp_err("send LC_init failed. ret(%d)\n", ret);
		return -1;
	}

	m_lc_init = (struct dp_lc_init *)msg_info.msg;
	ret = hdcp_dplink_send(HDCP22_MSG_RN_W,
			m_lc_init->rn,
			sizeof(m_lc_init->rn));
	if (ret) {
		hdcp_err("lc_init send fail: ret(%d)\n", ret);
		return -1;
	}

	return 0;
}

static int do_recv_lc_send_l_prime(struct hdcp_link_data *lk)
{
	int ret;
	struct hdcp_msg_info msg_info;
	struct dp_lc_send_l_prime m_lprime;

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	ret = hdcp_dplink_recv(HDCP22_MSG_LPRIME_R,
			m_lprime.l_prime,
			sizeof(m_lprime.l_prime));
	if (ret) {
		hdcp_err("lc_send_l_prime recv fail. ret(%d)\n", ret);
		return -1;
	}

	memcpy(msg_info.msg, &m_lprime, sizeof(struct dp_lc_send_l_prime));
	msg_info.msg_len = sizeof(struct dp_lc_send_l_prime);
	ret = dp_recv_protocol_msg(lk, DP_LC_SEND_L_PRIME, &msg_info);
	if (ret < 0) {
		hdcp_err("HDCP recv LC_Send_L_prime failed. ret(%d)\n", ret);
		return -1;
	}

	return 0;
}

static int do_send_ske_send_eks(struct hdcp_link_data *lk)
{
	int ret;
	struct hdcp_msg_info msg_info;
	struct dp_ske_send_eks *m_ske;
	/* todo:
	 * Currently, SST mode only use 0x00 as type value
	 * MST mode, HDCP driver get type value from DP driver
	 */
	/* uint8_t type = 0x00; */

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	ret = dp_send_protocol_msg(lk, DP_SKE_SEND_EKS, &msg_info);
	if (ret < 0) {
		hdcp_err("send SKE_SEND_EKS failed. ret(%d)\n", ret);
		return -1;
	}

	m_ske = (struct dp_ske_send_eks *)msg_info.msg;
	ret = hdcp_dplink_send(HDCP22_MSG_EDKEY_KS0_W,
			&m_ske->edkey_ks[0],
			sizeof(m_ske->edkey_ks)/2);
	if (ret) {
		hdcp_err("SKE_send_eks send fail. ret(%d)\n", ret);
		return -1;
	}

	ret = hdcp_dplink_send(HDCP22_MSG_EDKEY_KS1_W,
			&m_ske->edkey_ks[8],
			sizeof(m_ske->edkey_ks)/2);
	if (ret) {
		hdcp_err("SKE_send_eks send fail. ret(%x)\n", ret);
		return -1;
	}

	ret = hdcp_dplink_send(HDCP22_MSG_RIV_W,
			m_ske->riv,
			sizeof(m_ske->riv));
	if (ret) {
		hdcp_err("SKE_send_eks send fail. ret(%x)\n", ret);
		return -1;
	}

#if 0 /* todo: configure type */
	/* HDCP errata defined stream type.
	 * type info is send only for receiver
	 */
	if (!lk->rx_ctx.repeater) {
		ret = hdcp_dplink_send(HDCP22_MSG_TYPE_W,
				&type,
				sizeof(uint8_t));
		if (ret) {
			hdcp_err("HDCP : SKE_send_eks type send fail: %x\n", ret);
			return -1;
		}
	}
#endif

	return 0;
}

static int cal_rcvid_list_size(uint8_t *rxinfo, uint32_t *rcvid_size)
{
	struct rxinfo rx;
	int ret = 0;
	rxinfo_convert_arr2st(rxinfo, &rx);
#if defined(HDCP_DEBUG)
	rxinfo_print_all_info();
#endif

	/* rx_list check */
	if (rx.max_dev_exd || rx.max_cascade_exd)
		return 1;

	*rcvid_size = rx.dev_count * HDCP_RCV_ID_LEN;
	return ret;
}

static int do_recv_rcvid_list(struct hdcp_link_data *lk)
{
	int ret;
	struct dp_rp_send_rcvid_list m_rcvid;
	uint32_t rcvid_size = 0;
	struct hdcp_msg_info msg_info;

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	memset(&m_rcvid, 0x00, sizeof(m_rcvid));

	ret = hdcp_dplink_recv(HDCP22_MSG_RXINFO_R,
			m_rcvid.rx_info,
			sizeof(m_rcvid.rx_info));
	if (ret) {
		hdcp_err("verify_receiver_id_list rx_info rcv fail: ret(%d)\n", ret);
		return -1;
	}

	ret = hdcp_dplink_recv(HDCP22_MSG_SEQ_NUM_V_R,
			m_rcvid.seq_num_v,
			sizeof(m_rcvid.seq_num_v));

	if (ret) {
		hdcp_err("verify_receiver_id_list seq_num_v rcv fail: ret(%d)\n", ret);
		return -1;
	}

	if ((m_rcvid.seq_num_v[0] || m_rcvid.seq_num_v[1] || m_rcvid.seq_num_v[2]) && rp_ready < 2) {
		hdcp_err("Initial seq_num_v is non_zero.\n");
		return -1;
	}

	ret = hdcp_dplink_recv(HDCP22_MSG_VPRIME_R,
			m_rcvid.v_prime,
			sizeof(m_rcvid.v_prime));
	if (ret) {
		hdcp_err("verify_receiver_id_list seq_num_v rcv fail: ret(%d)\n", ret);
		return -1;
	}

	ret = cal_rcvid_list_size(m_rcvid.rx_info, &rcvid_size);
	if (ret) {
		hdcp_err("Cal_rcvid_list Fail ! (%d)\n", ret);
		return -1;
	}

	ret = hdcp_dplink_recv(HDCP22_MSG_RECV_ID_LIST_R,
			m_rcvid.rcvid_list,
			rcvid_size);
	if (ret) {
		hdcp_err("verify_receiver_id_list rcvid_list rcv fail: ret(%d)\n", ret);
		return -1;
	}

	memcpy(msg_info.msg, &m_rcvid, sizeof(struct dp_rp_send_rcvid_list));
	msg_info.msg_len = sizeof(struct dp_rp_send_rcvid_list);
	ret = dp_recv_protocol_msg(lk, DP_REPEATERAUTH_SEND_RECEIVERID_LIST, &msg_info);
	if (ret < 0) {
		hdcp_err("recv RepeaterAuth_Send_ReceiverID_List failed\n");
		return -1;
	}

	return 0;
}

static int do_send_rp_ack(struct hdcp_link_data *lk)
{
	int ret;
	struct hdcp_msg_info msg_info;
	struct dp_rp_send_ack *m_ack;

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	ret = dp_send_protocol_msg(lk, DP_REPEATERAUTH_SEND_AKE, &msg_info);
	if (ret < 0) {
		hdcp_err("send RepeaterAuth_Send_Ack failed. ret(%d)\n", ret);
		return -1;
	}

	m_ack = (struct dp_rp_send_ack *)msg_info.msg;
	ret = hdcp_dplink_send(HDCP22_MSG_V_W,
			m_ack->v,
			sizeof(m_ack->v));
	if (ret) {
		hdcp_err("V send fail: ret(%d)\n", ret);
		return -1;
	}

	return 0;
}

static int do_send_rp_stream_manage(struct hdcp_link_data *lk)
{
	int ret;
	struct hdcp_msg_info msg_info;
	struct dp_rp_stream_manage *m_strm;
	uint16_t stream_num;

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	ret = dp_send_protocol_msg(lk, DP_REPEATERAUTH_STREAM_MANAGE, &msg_info);
	if (ret < 0) {
		hdcp_err("send RepeaterAuth_Stream_Manage. ret(%d)\n", ret);
		return -1;
	}

	m_strm = (struct dp_rp_stream_manage *)msg_info.msg;
	ret = hdcp_dplink_send(HDCP22_MSG_SEQ_NUM_M_W,
			m_strm->seq_num_m,
			sizeof(m_strm->seq_num_m));
	if (ret) {
		hdcp_err("seq_num_M send fail. ret(%d)\n", ret);
		return -1;
	}

	ret = hdcp_dplink_send(HDCP22_MSG_K_W,
			m_strm->k,
			sizeof(m_strm->k));
	if (ret) {
		hdcp_err("k send fail. ret(%x)\n", ret);
		return -1;
	}

	stream_num = lk->tx_ctx.strm.dp.stream_num;
	ret = hdcp_dplink_send(HDCP22_MSG_STREAMID_TYPE_W,
			m_strm->streamid_type,
			stream_num * HDCP_RP_STREAMID_TYPE_LEN);
	if (ret) {
		hdcp_err("Streamid_Type send fail. ret(%x)\n", ret);
		return -1;
	}

	return 0;
}

static int do_recv_rp_stream_ready(struct hdcp_link_data *lk)
{
	int ret;
	struct hdcp_msg_info msg_info;
	struct dp_rp_stream_ready m_strm_ready;

       /* check abort state firstly,
        * if session is abored by Rx, Tx stops Authentication process
        */
       if (is_auth_aborted())
               return -TX_AUTH_ERROR_ABORT;

	ret = hdcp_dplink_recv(HDCP22_MSG_MPRIME_R,
			m_strm_ready.m_prime,
			sizeof(m_strm_ready.m_prime));
	if (ret) {
		hdcp_err("M' recv fail. ret(%d)\n", ret);
		return -1;
	}

	memcpy(msg_info.msg, &m_strm_ready, sizeof(struct dp_rp_stream_ready));
	msg_info.msg_len = sizeof(struct dp_rp_stream_ready);
	ret = dp_recv_protocol_msg(lk, DP_REPEATERAUTH_STREAM_READY, &msg_info);
	if (ret < 0) {
		hdcp_err("HDCP recv RepeaterAuth_Stream_Ready failed. ret(%d)\n", ret);
		return -1;
	}

	return 0;
}

static int check_h_prime_ready(void)
{
	int i = 0;
	uint8_t status = 0;

	/* check abort state firstly,
	 * if session is abored by Rx, Tx stops Authentication process
	 */
	if (is_auth_aborted())
		return -TX_AUTH_ERROR_ABORT;

	msleep(100);

	/* HDCP spec is 1 sec. but we give margin 100ms */
	while (i < 10) {
		/* check abort state firstly,
		 * if session is abored by Rx, Tx stops Authentication process
		 */
		if (is_auth_aborted())
			return -TX_AUTH_ERROR_ABORT;

		/* received from CP_IRQ */
		if (hprime_ready) {
			/* reset flag */
			hprime_ready = 0;
			return 0;
		}

		/* check as polling mode */
		hdcp_dplink_get_rxinfo(&status);
		if (status & DP_RXSTATUS_HPRIME_AVAILABLE) {
			/* reset flag */
			hprime_ready = 0;
			return 0;
		}

		msleep(100);
		i++;
	}

	hdcp_err("hprime timeout(%dms)\n", (100 * i));
	return -1;
}

static int check_pairing_ready(void)
{
	int i = 0;
	uint8_t status = 0;

	/* check abort state firstly,
	 * if session is abored by Rx, Tx stops Authentication process
	 */
	if (is_auth_aborted())
		return -TX_AUTH_ERROR_ABORT;

	msleep(200);
	/* HDCP spec is 200ms. but we give margin 100ms */
	while (i < 2) {
		/* check abort state firstly,
		 * if session is abored by Rx, Tx stops Authentication process
		 */
		if (is_auth_aborted())
			return -TX_AUTH_ERROR_ABORT;

		/* received from CP_IRQ */
		if (pairing_ready) {
			/* reset flag */
			pairing_ready = 0;
			return 0;
		}

		/* check as polling mode */
		hdcp_dplink_get_rxinfo(&status);
		if (status & DP_RXSTATUS_PAIRING_AVAILABLE) {
			/* reset flag */
			pairing_ready = 0;
			return 0;
		}

		msleep(100);
		i++;
	}

	hdcp_err("pairing timeout(%dms)\n", (100 * i));
	return -1;
}

static int check_rcvidlist_ready(void)
{
	int i = 0;
	uint8_t status = 0;

	/* HDCP spec is 3 sec */
	while (i < 30) {
		/* check abort state firstly,
		 * if session is abored by Rx, Tx stops Authentication process
		 */
		if (is_auth_aborted())
			return -TX_AUTH_ERROR_ABORT;

		/* received from CP_IRQ */
		if (rp_ready > rp_ready_s) {
			/* reset flag */
			rp_ready_s = rp_ready;
			return 0;
		}

		/* check as polling mode */
		hdcp_dplink_get_rxinfo(&status);
		if (status & DP_RXSTATUS_READY) {
			rp_ready++;
			return 0;
		}

		msleep(100);
		i++;
	}


	hdcp_err("receiver ID list timeout(%dms)\n", (100 * i));
	return -TX_AUTH_ERROR_TIME_EXCEED;
}

int dplink_exchange_master_key(struct hdcp_link_data *lk)
{
	int rval = TX_AUTH_SUCCESS;
	int key_found;

	do {
		/* send Tx -> Rx: AKE_init */
		if (do_send_ake_init(lk) < 0) {
			hdcp_err("send_ake_int fail\n");
			rval = -TX_AUTH_ERROR_SEND_PROTO_MSG;
			break;
		}

		/* HDCP spec defined 100ms as min delay after write AKE_Init */
		msleep(100);

		/* recv Rx->Tx: AKE_Send_Cert message */
		if (do_recv_ake_send_cert(lk) < 0) {
			hdcp_err("recv_ake_send_cert fail\n");
			rval = -TX_AUTH_ERROR_RECV_PROTO_MSG;
			break;
		}

		/* send Tx->Rx: AKE_Stored_km or AKE_No_Stored_km message */
		if (dp_ake_find_masterkey(&key_found) < 0) {
			hdcp_err("find master key fail\n");
			rval = -TX_AUTH_ERROR_MAKE_PROTO_MSG;
			break;
		}
		if (!key_found) {
			if (do_send_ake_nostored_km(lk) < 0) {
				hdcp_err("ake_send_nostored_km fail\n");
				rval = -TX_AUTH_ERROR_SEND_PROTO_MSG;
				break;
			}
			lk->stored_km = HDCP_WITHOUT_STORED_KM;
		} else {
			if (do_send_ake_stored_km(lk) < 0) {
				hdcp_err("ake_send_stored_km fail\n");
				rval = -TX_AUTH_ERROR_SEND_PROTO_MSG;
				break;
			}
			lk->stored_km = HDCP_WITH_STORED_KM;
		}

		if (check_h_prime_ready() < 0) {
			hdcp_err("Cannot read H prime\n");
			rval = -TX_AUTH_ERROR_RECV_PROTO_MSG;
			break;
		}

		/* recv Rx->Tx: AKE_Send_H_Prime message */
		if (do_recv_ake_send_h_prime(lk) < 0) {
			hdcp_err("recv_ake_send_h_prime fail\n");
			rval = -TX_AUTH_ERROR_RECV_PROTO_MSG;
			break;
		}

		if (lk->stored_km == HDCP_WITHOUT_STORED_KM) {
			if (check_pairing_ready() < 0) {
				hdcp_err("Cannot read pairing info\n");
				rval = -TX_AUTH_ERROR_RECV_PROTO_MSG;
				break;
			}

			/* recv Rx->Tx: AKE_Send_Pairing_Info message */
			if (do_recv_ake_send_pairing_info(lk) < 0) {
				hdcp_err("recv_ake_send_h_prime fail\n");
				rval = -TX_AUTH_ERROR_RECV_PROTO_MSG;
				break;
			}
		}
	} while (0);

	return rval;
}

int dplink_locality_check(struct hdcp_link_data *lk)
{
	int i;

	for (i = 0; i < MAX_LC_RETRY; i++) {
		/* send Tx -> Rx: LC_init */
		if (do_send_lc_init(lk) < 0) {
			hdcp_err("send_lc_init fail\n");
			return -TX_AUTH_ERROR_SEND_PROTO_MSG;
		}

		/* wait until max dealy */
		msleep(7);

		/* recv Rx -> Tx: LC_Send_L_Prime */
		if (do_recv_lc_send_l_prime(lk) < 0) {
			hdcp_err("recv_lc_send_l_prime fail\n");
			/* retry */
			continue;
		} else {
			hdcp_debug("LC success. retryed(%d)\n", i);
			break;
		}
	}

	if (i == MAX_LC_RETRY) {
		hdcp_err("LC check fail. exceed retry count(%d)\n", i);
		return -TX_AUTH_ERROR_RETRYCOUNT_EXCEED;
	}

	return TX_AUTH_SUCCESS;
}

int dplink_exchange_session_key(struct hdcp_link_data *lk)
{
	/* find session key from the session data */
	if (dp_get_hdcp_session_key(lk) < 0) {
		hdcp_err("dp_get_hdcp_session_key() failed\n");
		return -TX_AUTH_ERRRO_RESTORE_SKEY;
	}

	/* Send Tx -> Rx: SKE_Send_Eks */
	if (do_send_ske_send_eks(lk) < 0) {
		hdcp_err("send_ske_send_eks fail\n");
		return -TX_AUTH_ERROR_SEND_PROTO_MSG;
	}

	if (dp_put_hdcp_session_key(lk) < 0) {
		hdcp_err("put_hdcp_session_key() failed\n");
		return -TX_AUTH_ERROR_STORE_SKEY;
	}

	return TX_AUTH_SUCCESS;
}

int dplink_evaluate_repeater(struct hdcp_link_data *lk)
{
	if (lk->rx_ctx.repeater)
		return TRUE;
	else
		return FALSE;
}

int dplink_wait_for_receiver_id_list(struct hdcp_link_data *lk)
{
	int ret;

	ret = check_rcvidlist_ready();
	if (ret < 0)
		return ret;
	else
		return TX_AUTH_SUCCESS;
}

int dplink_verify_receiver_id_list(struct hdcp_link_data *lk)
{
	/* recv Rx->Tx: RepeaterAuth_ReceiverID_List message */
	if (do_recv_rcvid_list(lk) < 0) {
		hdcp_err("recv_receiverID_list fail\n");
		return -TX_AUTH_ERROR_RECV_PROTO_MSG;
	}

	return TX_AUTH_SUCCESS;
}

int dplink_send_receiver_id_list_ack(struct hdcp_link_data *lk)
{
	/* Send Tx -> Rx: RepeaterAuth_Send_Ack */
	if (do_send_rp_ack(lk) < 0) {
		hdcp_err("send_rp_ack fail\n");
		return -TX_AUTH_ERROR_SEND_PROTO_MSG;
	}

	return TX_AUTH_SUCCESS;
}

int dplink_determine_rx_hdcp_cap(struct hdcp_link_data *lk)
{
	int ret;
	uint8_t rxcaps[HDCP_CAPS_BYTE_LEN];

	ret = hdcp_dplink_recv(HDCP22_MSG_RXCAPS_R,
			rxcaps,
			sizeof(rxcaps));
	if (ret) {
		hdcp_err("check rx caps recv fail: ret(%d)\n", ret);
		return -1;
	}
#if defined(HDCP_DEBUG)
	hdcp_debug("rx caps\n");
	hdcp_hexdump(rxcaps,  sizeof(rxcaps));
#endif
	if (!(rxcaps[2] & DP_RXCAPS_HDCP_CAPABLE)) {
		hdcp_err("Rx is not support HDCP. rxcaps(0x%02x%02x%02x)\n",
			rxcaps[0], rxcaps[1], rxcaps[2]);
		return -1;
	}

	if (rxcaps[0] != DP_RXCAPS_HDCP_VERSION_2) {
		hdcp_err("Rx is not HDCP2.x. rxcaps(0x%02x%02x%02x)\n",
			rxcaps[0], rxcaps[1], rxcaps[2]);
		return -1;
	}

	return 0;
}

int dplink_stream_manage(struct hdcp_link_data *lk)
{
	/* Send Tx -> Rx: RepeaterAuth_Stream_Manage */
	if (do_send_rp_stream_manage(lk) < 0) {
		hdcp_err("send_rp_stream_manage fail\n");
		return -TX_AUTH_ERROR_SEND_PROTO_MSG;
	}

	/* HDCP spec define 100ms as min delay. But we give 100ms margin */
	msleep(200);

	/* recv Rx->Tx: RepeaterAuth_Stream_Ready message */
	if (do_recv_rp_stream_ready(lk) < 0) {
		hdcp_err("recv_rp_stream_ready fail\n");
		return -TX_AUTH_ERROR_RECV_PROTO_MSG;
	}

	return TX_AUTH_SUCCESS;
}

int dplink_set_paring_available(void)
{
	pairing_ready = 1;

	return 0;
}

int dplink_set_hprime_available(void)
{
	hprime_ready = 1;

	return 0;
}

int dplink_set_rp_ready(void)
{
	rp_ready++;
	return 0;
}

int dplink_set_reauth_req(void)
{
	reauth_req = 1;

	return 0;
}

int dplink_set_integrity_fail(void)
{
	dplink_clear_irqflag_all();
	integrity_fail = 1;

	return 0;
}

void dplink_clear_irqflag_all(void)
{
	pairing_ready = 0;
	hprime_ready = 0;
	rp_ready = 0;
	rp_ready_s = 0;
	reauth_req = 0;
	integrity_fail = 0;
}
