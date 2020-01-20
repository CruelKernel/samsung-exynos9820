/* driver/soc/samsung/exynos-hdcp/dplink/exynos-hdcp2-protocol-msg.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __EXYNOS_HDCP2_DPLINK_PROTOCOL_MSG_H__
#define __EXYNOS_HDCP2_DPLINK_PROTOCOL_MSG_H__

#include <linux/types.h>

enum dp_msg_id {
	DP_AKE_INIT = 2,
	DP_AKE_SEND_CERT,
	DP_AKE_NO_STORED_KM,
	DP_AKE_STORED_KM,
	DP_AKE_SEND_RRX,
	DP_AKE_SEND_H_PRIME,
	DP_AKE_SEND_PAIRING_INFO,
	DP_LC_INIT,
	DP_LC_SEND_L_PRIME,
	DP_SKE_SEND_EKS,
	DP_REPEATERAUTH_SEND_RECEIVERID_LIST,
	DP_RTT_READY,
	DP_RTT_CHALLENGE,
	DP_REPEATERAUTH_SEND_AKE,
	DP_REPEATERAUTH_STREAM_MANAGE,
	DP_REPEATERAUTH_STREAM_READY,
	DP_RECEIVER_AUTHSTATUS,
	DP_AKE_TRANSMITTER_INFO,
	DP_AKE_RECEIVER_INFO,
};

#define DP_RXSTATUS_READY		(0x1 << 0)
#define DP_RXSTATUS_HPRIME_AVAILABLE	(0x1 << 1)
#define DP_RXSTATUS_PAIRING_AVAILABLE	(0x1 << 2)
#define DP_RXSTATUS_REAUTH_REQ		(0x1 << 3)
#define DP_RXSTATUS_LINK_INTEGRITY_FAIL	(0x1 << 4)

#define DP_RXCAPS_HDCP_CAPABLE		(0x1 << 1)
#define DP_RXCAPS_REPEATER		(0x1 << 0)
#define DP_RXCAPS_HDCP_VERSION_2	(0x2)

/* RxInfo */
#define DEPTH_SHIFT	(9)
#define DEPTH_MASK	(0x7)
#define DEV_COUNT_SHIFT	(4)
#define DEV_COUNT_MASK	(0x1F)
#define DEV_EXD_SHIFT	(3)
#define DEV_EXD_MASK	(0x1)
#define CASCADE_EXD_SHIFT	(2)
#define CASCADE_EXD_MASK	(0x1)
#define HDCP20_DOWN_SHIFT	(1)
#define HDCP20_DOWN_MASK	(0x1)
#define HDCP1X_DOWN_SHIFT	(0)
#define HDCP1X_DOWN_MASK	(0x1)

#define dp_check_received_msg(m, m_len, len) \
	((m == NULL) ? ERR_WRONG_BUFFER : \
	((m_len < len) ? ERR_WRONG_MESSAGE_LENGTH : 0))

#endif
