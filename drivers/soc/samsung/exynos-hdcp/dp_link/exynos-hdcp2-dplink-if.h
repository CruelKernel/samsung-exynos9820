/* driver/soc/samsung/exynos-hdcp/dplink/exynos-hdcp2-dplink-if.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __EXYNOS_HDCP2_DPLINK_IF_H__
#define __EXYNOS_HDCP2_DPLINK_IF_H__

enum hdcp22_msg_name {
	HDCP22_MSG_RTX_W,
	HDCP22_MSG_TXCAPS_W,
	HDCP22_MSG_CERT_RX_R,
	HDCP22_MSG_RRX_R,
	HDCP22_MSG_RXCAPS_R,
	HDCP22_MSG_EKPUB_KM_W,
	HDCP22_MSG_EKH_KM_W,
	HDCP22_MSG_M_W,
	HDCP22_MSG_HPRIME_R,
	HDCP22_MSG_EKH_KM_R,
	HDCP22_MSG_RN_W,
	HDCP22_MSG_LPRIME_R,
	HDCP22_MSG_EDKEY_KS0_W,
	HDCP22_MSG_EDKEY_KS1_W,
	HDCP22_MSG_RIV_W,
	HDCP22_MSG_RXINFO_R,
	HDCP22_MSG_SEQ_NUM_V_R,
	HDCP22_MSG_VPRIME_R,
	HDCP22_MSG_RECV_ID_LIST_R,
	HDCP22_MSG_V_W,
	HDCP22_MSG_SEQ_NUM_M_W,
	HDCP22_MSG_K_W,
	HDCP22_MSG_STREAMID_TYPE_W,
	HDCP22_MSG_MPRIME_R,
	HDCP22_MSG_RXSTATUS_R,
	HDCP22_MSG_TYPE_W,
	NUM_HDCP22_MSG_NAME,
};

#define DP_HDCP22_ENABLE	1
#define DP_HDCP22_DISABLE	0

int hdcp_dplink_init(void);
void hdcp_dplink_config(int en);
int hdcp_dplink_is_enabled_hdcp22(void);
int hdcp_dplink_recv(uint32_t msg_name, uint8_t *data, uint32_t size);
int hdcp_dplink_send(uint32_t msg_name, uint8_t *data, uint32_t size);
int hdcp_dplink_get_stream_info(uint16_t *num, uint8_t *strm_id);

#endif
